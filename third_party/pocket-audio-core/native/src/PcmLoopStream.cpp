#include "pocket_audio/PcmLoopStream.h"

#include <algorithm>
#include <cmath>

namespace pocket_audio
{

PcmLoopStream::PcmLoopStream(const std::span<const PcmClip> clips,
                             const std::uint64_t crossfadeFrames) noexcept
    : crossfadeFrames_(crossfadeFrames),
      crossfadePosition_(crossfadeFrames)
{
    clipsValid_ = crossfadeFrames <= kPcmMaximumCrossfadeFrames && ValidateClips(clips);
}

bool PcmLoopStream::ValidateClips(const std::span<const PcmClip> clips) noexcept
{
    if (clips.empty() || clips.size() > kPcmMaximumClipCount ||
        !clips[0].body.empty() || !clips[0].tail.empty() || clips[0].looping)
    {
        return false;
    }

    clipCount_ = clips.size();
    clips_[0] = clips[0];
    for (std::size_t index = 1u; index < clips.size(); ++index)
    {
        const PcmClip& clip = clips[index];
        if (clip.body.empty() || (clip.body.size() & 1u) != 0u ||
            (clip.tail.size() & 1u) != 0u)
        {
            return false;
        }

        const std::uint64_t bodyFrames = static_cast<std::uint64_t>(clip.body.size() / 2u);
        const std::uint64_t tailFrames = static_cast<std::uint64_t>(clip.tail.size() / 2u);
        if (bodyFrames == 0u || bodyFrames > kPcmMaximumClipFrames ||
            tailFrames > bodyFrames)
        {
            return false;
        }
        clips_[index] = clip;
    }
    return true;
}

bool PcmLoopStream::ValidateSelection(const PcmSelection& selection) const noexcept
{
    if (selection.clipIndex >= clipCount_ || !std::isfinite(selection.positionSeconds) ||
        selection.positionSeconds < 0.0)
    {
        return false;
    }
    if (selection.clipIndex == 0u)
    {
        return !selection.looping && selection.positionSeconds == 0.0 &&
               !selection.naturalTailHandoff;
    }

    const PcmClip& clip = clips_[selection.clipIndex];
    if (selection.looping != clip.looping)
    {
        return false;
    }
    const double durationSeconds = static_cast<double>(BodyFrames(selection.clipIndex)) /
                                  static_cast<double>(kPcmSampleRate);
    return selection.positionSeconds < durationSeconds;
}

PcmStatus PcmLoopStream::SetSelection(const PcmSelection& selection) noexcept
{
    if (!clipsValid_)
    {
        ClearPlayback();
        return PcmStatus::InvalidClips;
    }
    if (!ValidateSelection(selection) ||
        (selectionInitialized_ && selection.revision < selection_.revision) ||
        (selectionInitialized_ && selection.revision == selection_.revision &&
         (selection.clipIndex != selection_.clipIndex ||
          selection.looping != selection_.looping)))
    {
        ClearPlayback();
        return PcmStatus::InvalidSelection;
    }

    const bool firstSelection = !selectionInitialized_;
    const bool revisionChanged = firstSelection || selection.revision != selection_.revision;
    const bool discontinuityEdge = selection.discontinuity && !lastDiscontinuity_;
    const bool seek = revisionChanged || discontinuityEdge;
    const bool clipChanged = current_.clipIndex != selection.clipIndex;

    if (seek)
    {
        if (!firstSelection && selection.naturalTailHandoff && clipChanged &&
            current_.active && current_.clipIndex != 0u &&
            !clips_[current_.clipIndex].looping &&
            current_.cursorFrames >= BodyFrames(current_.clipIndex))
        {
            const std::uint64_t bodyFrames = BodyFrames(current_.clipIndex);
            const std::uint64_t tailFrames = TailFrames(current_.clipIndex);
            if (current_.cursorFrames - bodyFrames < tailFrames)
            {
                outgoing_ = current_;
                outgoing_.cursorFrames -= bodyFrames;
                outgoing_.tailOnly = true;
                outgoing_.active = true;
                crossfadePosition_ = crossfadeFrames_;
            }
            else
            {
                outgoing_ = {};
                crossfadePosition_ = crossfadeFrames_;
            }
        }
        else if (!firstSelection && clipChanged && current_.active &&
                 current_.clipIndex != 0u && selection.clipIndex != 0u)
        {
            outgoing_ = current_;
            outgoing_.tailOnly = false;
            crossfadePosition_ = 0u;
        }
        else
        {
            outgoing_ = {};
            crossfadePosition_ = crossfadeFrames_;
        }

        current_ = MakeState(selection);
    }

    selection_ = selection;
    selectionInitialized_ = true;
    lastDiscontinuity_ = selection.discontinuity;
    suspended_ = selection.suspended || !selection.clockValid;
    if (!selection.clockValid)
    {
        return PcmStatus::ClockInvalid;
    }
    return suspended_ ? PcmStatus::Suspended : PcmStatus::Ok;
}

bool PcmLoopStream::SetVolumePercent(const float percent) noexcept
{
    if (!std::isfinite(percent))
    {
        return false;
    }
    volume_ = std::clamp(percent, 0.0f, 100.0f) / 100.0f;
    return true;
}

PcmStatus PcmLoopStream::Render(const std::span<float> interleavedStereoOutput) noexcept
{
    std::fill(interleavedStereoOutput.begin(), interleavedStereoOutput.end(), 0.0f);
    if ((interleavedStereoOutput.size() & 1u) != 0u)
    {
        return PcmStatus::InvalidOutput;
    }
    if (!clipsValid_)
    {
        return PcmStatus::InvalidClips;
    }
    if (!selectionInitialized_)
    {
        return PcmStatus::InvalidSelection;
    }
    if (!selection_.clockValid)
    {
        return PcmStatus::ClockInvalid;
    }
    if (suspended_)
    {
        return PcmStatus::Suspended;
    }

    for (std::size_t frame = 0u; frame < interleavedStereoOutput.size() / 2u; ++frame)
    {
        float currentLeft = 0.0f;
        float currentRight = 0.0f;
        (void)ReadFrame(current_, currentLeft, currentRight);

        float outgoingLeft = 0.0f;
        float outgoingRight = 0.0f;
        const bool hasOutgoing = outgoing_.active &&
                                 ReadFrame(outgoing_, outgoingLeft, outgoingRight);

        float mixedLeft = currentLeft;
        float mixedRight = currentRight;
        if (outgoing_.tailOnly)
        {
            mixedLeft += outgoingLeft;
            mixedRight += outgoingRight;
        }
        else if (crossfadePosition_ < crossfadeFrames_)
        {
            const float outgoingGain = 1.0f -
                static_cast<float>(crossfadePosition_) /
                static_cast<float>(crossfadeFrames_);
            const float currentGain = 1.0f - outgoingGain;
            mixedLeft = (hasOutgoing ? outgoingLeft * outgoingGain : 0.0f) +
                        currentLeft * currentGain;
            mixedRight = (hasOutgoing ? outgoingRight * outgoingGain : 0.0f) +
                         currentRight * currentGain;
            ++crossfadePosition_;
            if (crossfadePosition_ >= crossfadeFrames_)
            {
                outgoing_ = {};
            }
        }
        else if (outgoing_.active)
        {
            outgoing_ = {};
        }

        const std::size_t sample = frame * 2u;
        const float left = mixedLeft * volume_;
        const float right = mixedRight * volume_;
        interleavedStereoOutput[sample] = std::isfinite(left) ? left : 0.0f;
        interleavedStereoOutput[sample + 1u] = std::isfinite(right) ? right : 0.0f;
    }
    return PcmStatus::Ok;
}

void PcmLoopStream::Reset() noexcept
{
    ClearPlayback();
}

PcmLoopStream::StreamState PcmLoopStream::MakeState(const PcmSelection& selection) const noexcept
{
    if (selection.clipIndex == 0u)
    {
        return {};
    }

    const double requestedFrame = selection.positionSeconds *
                                  static_cast<double>(kPcmSampleRate);
    const auto roundedFrame = static_cast<std::uint64_t>(std::llround(requestedFrame));
    const std::uint64_t bodyFrames = BodyFrames(selection.clipIndex);
    return {
        .clipIndex = selection.clipIndex,
        .cursorFrames = selection.looping ? roundedFrame % bodyFrames : roundedFrame,
        .completedLoop = false,
        .active = true,
        .tailOnly = false,
    };
}

bool PcmLoopStream::ReadFrame(StreamState& state, float& left, float& right) const noexcept
{
    left = 0.0f;
    right = 0.0f;
    if (!state.active || state.clipIndex == 0u)
    {
        return false;
    }

    const PcmClip& clip = clips_[state.clipIndex];
    if (state.tailOnly)
    {
        if (state.cursorFrames >= TailFrames(state.clipIndex))
        {
            state.active = false;
            return false;
        }
        const std::size_t sample = static_cast<std::size_t>(state.cursorFrames * 2u);
        left = static_cast<float>(clip.tail[sample]) / 32768.0f;
        right = static_cast<float>(clip.tail[sample + 1u]) / 32768.0f;
        ++state.cursorFrames;
        return true;
    }

    const std::uint64_t bodyFrames = BodyFrames(state.clipIndex);
    if (clip.looping)
    {
        if (state.cursorFrames >= bodyFrames)
        {
            state.cursorFrames %= bodyFrames;
            state.completedLoop = true;
        }
        const std::size_t sample = static_cast<std::size_t>(state.cursorFrames * 2u);
        left = static_cast<float>(clip.body[sample]) / 32768.0f;
        right = static_cast<float>(clip.body[sample + 1u]) / 32768.0f;
        if (state.completedLoop && state.cursorFrames < TailFrames(state.clipIndex))
        {
            const std::size_t tailSample = static_cast<std::size_t>(state.cursorFrames * 2u);
            left += static_cast<float>(clip.tail[tailSample]) / 32768.0f;
            right += static_cast<float>(clip.tail[tailSample + 1u]) / 32768.0f;
        }
        ++state.cursorFrames;
        if (state.cursorFrames == bodyFrames)
        {
            state.cursorFrames = 0u;
            state.completedLoop = true;
        }
        return true;
    }

    if (state.cursorFrames < bodyFrames)
    {
        const std::size_t sample = static_cast<std::size_t>(state.cursorFrames * 2u);
        left = static_cast<float>(clip.body[sample]) / 32768.0f;
        right = static_cast<float>(clip.body[sample + 1u]) / 32768.0f;
        ++state.cursorFrames;
        return true;
    }

    const std::uint64_t tailFrame = state.cursorFrames - bodyFrames;
    if (tailFrame < TailFrames(state.clipIndex))
    {
        const std::size_t sample = static_cast<std::size_t>(tailFrame * 2u);
        left = static_cast<float>(clip.tail[sample]) / 32768.0f;
        right = static_cast<float>(clip.tail[sample + 1u]) / 32768.0f;
        ++state.cursorFrames;
        return true;
    }

    state.active = false;
    return false;
}

std::uint64_t PcmLoopStream::BodyFrames(const std::uint8_t index) const noexcept
{
    return static_cast<std::uint64_t>(clips_[index].body.size() / 2u);
}

std::uint64_t PcmLoopStream::TailFrames(const std::uint8_t index) const noexcept
{
    return static_cast<std::uint64_t>(clips_[index].tail.size() / 2u);
}

void PcmLoopStream::ClearPlayback() noexcept
{
    current_ = {};
    outgoing_ = {};
    selection_ = {};
    crossfadePosition_ = crossfadeFrames_;
    selectionInitialized_ = false;
    suspended_ = true;
    lastDiscontinuity_ = false;
}

} // namespace pocket_audio
