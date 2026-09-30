#include "audio/MusicPcmStream.h"

#include <algorithm>
#include <cmath>

namespace horde::audio
{
namespace
{

constexpr std::size_t CueIndex(const MusicCue cue) noexcept
{
    return static_cast<std::size_t>(cue);
}

constexpr bool IsLoopingCue(const MusicCue cue) noexcept
{
    return cue == MusicCue::A || cue == MusicCue::B || cue == MusicCue::D ||
           cue == MusicCue::E || cue == MusicCue::F || cue == MusicCue::H;
}

constexpr std::uint64_t BodyFrames(const MusicCue cue) noexcept
{
    switch (cue)
    {
    case MusicCue::A:
    case MusicCue::B:
    case MusicCue::D:
    case MusicCue::E:
    case MusicCue::F:
    case MusicCue::H:
        return kMusicPcmLoopFrames;
    case MusicCue::C:
        return 144'000u;
    case MusicCue::G:
        return 288'000u;
    case MusicCue::None:
        return 0u;
    }
    return 0u;
}

bool IsKnownCue(const MusicCue cue) noexcept
{
    return CueIndex(cue) < kMusicPcmCueCount;
}

bool IsNaturalTailTransition(const MusicCue from, const MusicCue to) noexcept
{
    return (from == MusicCue::C && to == MusicCue::D) ||
           (from == MusicCue::G && to == MusicCue::H);
}

} // namespace

MusicPcmStream::MusicPcmStream(const std::span<const MusicPcmClip> clips) noexcept
    : clipsValid_(ValidateClips(clips))
{
}

bool MusicPcmStream::ValidateClips(const std::span<const MusicPcmClip> clips) noexcept
{
    if (clips.size() != kMusicPcmCueCount)
    {
        return false;
    }

    for (std::size_t index = 0u; index < clips.size(); ++index)
    {
        const MusicCue cue = static_cast<MusicCue>(index);
        const MusicPcmClip& clip = clips[index];
        if (cue == MusicCue::None)
        {
            if (!clip.body.empty() || !clip.tail.empty())
            {
                return false;
            }
            continue;
        }

        const std::uint64_t bodyFrames = BodyFrames(cue);
        if (clip.body.size() != bodyFrames * 2u ||
            clip.tail.size() != kMusicPcmTailFrames * 2u)
        {
            return false;
        }
        clips_[index] = clip;
    }
    return true;
}

bool MusicPcmStream::ValidateSelection(const MusicSelection& selection) const noexcept
{
    if (!IsKnownCue(selection.cue) || !std::isfinite(selection.positionSeconds) ||
        selection.positionSeconds < 0.0)
    {
        return false;
    }
    if (selection.cue == MusicCue::None)
    {
        return !selection.looping && selection.positionSeconds == 0.0;
    }
    if (selection.looping != IsLoopingCue(selection.cue))
    {
        return false;
    }

    const double durationSeconds = static_cast<double>(BodyFrames(selection.cue)) /
                                   static_cast<double>(kMusicPcmSampleRate);
    return selection.positionSeconds < durationSeconds;
}

MusicPcmStatus MusicPcmStream::SetSelection(const MusicSelection& selection) noexcept
{
    if (!clipsValid_)
    {
        ClearPlayback();
        return MusicPcmStatus::InvalidClips;
    }
    if (!ValidateSelection(selection) ||
        (selectionInitialized_ && selection.revision < selection_.revision) ||
        (selectionInitialized_ && selection.revision == selection_.revision &&
         (selection.cue != selection_.cue || selection.looping != selection_.looping)))
    {
        ClearPlayback();
        return MusicPcmStatus::InvalidSelection;
    }

    const bool firstSelection = !selectionInitialized_;
    const bool revisionChanged = firstSelection || selection.revision != selection_.revision;
    const bool discontinuityEdge = selection.discontinuity && !lastDiscontinuity_;
    const bool seek = revisionChanged || discontinuityEdge;
    const bool cueChanged = current_.cue != selection.cue ||
                            IsLoopingCue(current_.cue) != selection.looping;

    if (seek)
    {
        if (!firstSelection && cueChanged && current_.active &&
            IsNaturalTailTransition(current_.cue, selection.cue) &&
            current_.cursorFrames >= BodyFrames(current_.cue))
        {
            const std::uint64_t bodyFrames = BodyFrames(current_.cue);
            const std::uint64_t tailPosition = current_.cursorFrames > bodyFrames
                ? current_.cursorFrames - bodyFrames : 0u;
            if (tailPosition < kMusicPcmTailFrames)
            {
                outgoing_ = current_;
                outgoing_.cursorFrames = tailPosition;
                outgoing_.tailOnly = true;
                outgoing_.active = true;
                crossfadePosition_ = kMusicPcmCrossfadeFrames;
            }
            else
            {
                outgoing_ = {};
                crossfadePosition_ = kMusicPcmCrossfadeFrames;
            }
        }
        else if (!firstSelection && cueChanged && current_.active &&
                 current_.cue != MusicCue::None && selection.cue != MusicCue::None)
        {
            outgoing_ = current_;
            outgoing_.tailOnly = false;
            crossfadePosition_ = 0u;
        }
        else
        {
            outgoing_ = {};
            crossfadePosition_ = kMusicPcmCrossfadeFrames;
        }

        current_ = MakeState(selection);
    }

    selection_ = selection;
    selectionInitialized_ = true;
    lastDiscontinuity_ = selection.discontinuity;
    suspended_ = selection.suspended || !selection.clockValid;
    if (!selection.clockValid)
    {
        return MusicPcmStatus::ClockInvalid;
    }
    return suspended_ ? MusicPcmStatus::Suspended : MusicPcmStatus::Ok;
}

bool MusicPcmStream::SetVolumePercent(const float percent) noexcept
{
    if (!std::isfinite(percent))
    {
        return false;
    }
    volume_ = std::clamp(percent, 0.0f, 100.0f) / 100.0f;
    return true;
}

MusicPcmStatus MusicPcmStream::Render(
    const std::span<float> interleavedStereoOutput) noexcept
{
    std::fill(interleavedStereoOutput.begin(), interleavedStereoOutput.end(), 0.0f);
    if (interleavedStereoOutput.size() % 2u != 0u)
    {
        return MusicPcmStatus::InvalidOutput;
    }
    if (!clipsValid_)
    {
        return MusicPcmStatus::InvalidClips;
    }
    if (!selectionInitialized_)
    {
        return MusicPcmStatus::InvalidSelection;
    }
    if (!selection_.clockValid)
    {
        return MusicPcmStatus::ClockInvalid;
    }
    if (suspended_)
    {
        return MusicPcmStatus::Suspended;
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
        else if (crossfadePosition_ < kMusicPcmCrossfadeFrames)
        {
            const float outgoingGain = 1.0f -
                static_cast<float>(crossfadePosition_) /
                static_cast<float>(kMusicPcmCrossfadeFrames);
            const float currentGain = 1.0f - outgoingGain;
            mixedLeft = (hasOutgoing ? outgoingLeft * outgoingGain : 0.0f) +
                        currentLeft * currentGain;
            mixedRight = (hasOutgoing ? outgoingRight * outgoingGain : 0.0f) +
                         currentRight * currentGain;
            ++crossfadePosition_;
            if (crossfadePosition_ >= kMusicPcmCrossfadeFrames)
            {
                outgoing_ = {};
            }
        }
        else if (outgoing_.active)
        {
            outgoing_ = {};
        }

        const std::size_t sample = frame * 2u;
        interleavedStereoOutput[sample] = std::isfinite(mixedLeft * volume_)
            ? mixedLeft * volume_ : 0.0f;
        interleavedStereoOutput[sample + 1u] = std::isfinite(mixedRight * volume_)
            ? mixedRight * volume_ : 0.0f;
    }
    return MusicPcmStatus::Ok;
}

void MusicPcmStream::Reset() noexcept
{
    ClearPlayback();
}

MusicPcmStream::StreamState MusicPcmStream::MakeState(
    const MusicSelection& selection) const noexcept
{
    if (selection.cue == MusicCue::None)
    {
        return {};
    }

    const double requestedFrame = selection.positionSeconds *
                                  static_cast<double>(kMusicPcmSampleRate);
    const auto roundedFrame = static_cast<std::uint64_t>(std::llround(requestedFrame));
    const std::uint64_t bodyFrames = BodyFrames(selection.cue);
    return {
        .cue = selection.cue,
        .cursorFrames = selection.looping ? roundedFrame % bodyFrames : roundedFrame,
        .completedLoop = false,
        .active = true,
        .tailOnly = false,
    };
}

bool MusicPcmStream::ReadFrame(StreamState& state, float& left, float& right) const noexcept
{
    left = 0.0f;
    right = 0.0f;
    if (!state.active || state.cue == MusicCue::None)
    {
        return false;
    }

    const std::size_t index = CueIndex(state.cue);
    const MusicPcmClip& clip = clips_[index];
    if (state.tailOnly)
    {
        if (state.cursorFrames >= kMusicPcmTailFrames)
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

    const std::uint64_t bodyFrames = BodyFrames(state.cue);
    if (IsLoopingCue(state.cue))
    {
        if (state.cursorFrames >= bodyFrames)
        {
            state.cursorFrames %= bodyFrames;
            state.completedLoop = true;
        }
        const std::size_t sample = static_cast<std::size_t>(state.cursorFrames * 2u);
        left = static_cast<float>(clip.body[sample]) / 32768.0f;
        right = static_cast<float>(clip.body[sample + 1u]) / 32768.0f;
        if (state.completedLoop && state.cursorFrames < kMusicPcmTailFrames)
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
    if (tailFrame < kMusicPcmTailFrames)
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

void MusicPcmStream::ClearPlayback() noexcept
{
    current_ = {};
    outgoing_ = {};
    selection_ = {};
    crossfadePosition_ = kMusicPcmCrossfadeFrames;
    selectionInitialized_ = false;
    suspended_ = true;
    lastDiscontinuity_ = false;
}

} // namespace horde::audio
