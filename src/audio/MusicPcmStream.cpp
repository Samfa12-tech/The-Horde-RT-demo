#include "audio/MusicPcmStream.h"

namespace horde::audio
{
namespace
{

// Core copies only the immutable spans/metadata during construction. The
// temporary fixed array need not survive; the caller-owned PCM samples must.
std::array<pocket_audio::PcmClip, kMusicPcmCueCount> CoreClips(
    const std::span<const MusicPcmClip> clips) noexcept
{
    std::array<pocket_audio::PcmClip, kMusicPcmCueCount> result{};
    if (clips.size() != kMusicPcmCueCount ||
        !clips[0].body.empty() || !clips[0].tail.empty())
    {
        return {}; // Missing non-silence bodies make Core reject the bank.
    }
    for (std::size_t index = 1u; index < clips.size(); ++index)
    {
        const auto& asset = kMusicPcmAssets[index];
        if (clips[index].body.size() != asset.bodyFrames * 2u ||
            clips[index].tail.size() != kMusicPcmTailFrames * 2u)
        {
            return {};
        }
        result[index] = {clips[index].body, clips[index].tail, asset.looping};
    }
    return result;
}

bool IsNaturalTailTransition(const MusicCue from, const MusicCue to) noexcept
{
    return (from == MusicCue::C && to == MusicCue::D) ||
           (from == MusicCue::G && to == MusicCue::H);
}

} // namespace

MusicPcmStream::MusicPcmStream(const std::span<const MusicPcmClip> clips) noexcept
    : core_(CoreClips(clips), kMusicPcmCrossfadeFrames)
{
}

MusicPcmStatus MusicPcmStream::SetSelection(const MusicSelection& selection) noexcept
{
    const auto status = core_.SetSelection({
        .clipIndex = static_cast<std::uint8_t>(selection.cue),
        .looping = selection.looping,
        .suspended = selection.suspended,
        .discontinuity = selection.discontinuity,
        .clockValid = selection.clockValid,
        .positionSeconds = selection.positionSeconds,
        .revision = selection.revision,
        .naturalTailHandoff = IsNaturalTailTransition(previousCue_, selection.cue),
    });
    previousCue_ = status == MusicPcmStatus::InvalidClips ||
                           status == MusicPcmStatus::InvalidSelection
        ? MusicCue::None : selection.cue;
    return status;
}

bool MusicPcmStream::SetVolumePercent(const float percent) noexcept
{
    return core_.SetVolumePercent(percent);
}

MusicPcmStatus MusicPcmStream::Render(const std::span<float> output) noexcept
{
    return core_.Render(output);
}

void MusicPcmStream::Reset() noexcept
{
    core_.Reset();
    previousCue_ = MusicCue::None;
}

} // namespace horde::audio
