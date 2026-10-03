#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "audio/MusicPcmAssets.h"

namespace horde::audio
{

// Non-owning interleaved stereo PCM16. The caller retains both spans for the
// complete lifetime of MusicPcmStream. No decoding or file access occurs here.
struct MusicPcmClip
{
    std::span<const std::int16_t> body;
    std::span<const std::int16_t> tail;
};

using MusicPcmStatus = pocket_audio::PcmStatus;

// Thin Horde cue/asset adapter to Pocket Audio Core's native PCM cursor/mixer.
// Horde owns the nine MusicCue rows and C->D/G->H handoff policy, not PCM mixing.
// SetSelection, SetVolumePercent, Render, and Reset require one owner thread; this
// class makes no concurrent-call guarantee. Render performs no allocation,
// locks, or I/O and reads only the caller-owned immutable clip spans.
// Normal cue changes retain at most one 250 ms outgoing stream; a newer cue
// revision replaces that outgoing stream rather than accumulating old audio.
// C->D/G->H preserve the remaining exact tail only after the one-shot body has
// actually ended. An earlier interruption follows the normal bounded fade.
class MusicPcmStream
{
public:
    explicit MusicPcmStream(std::span<const MusicPcmClip> clips) noexcept;

    [[nodiscard]] bool IsValid() const noexcept { return core_.IsValid(); }
    [[nodiscard]] MusicPcmStatus SetSelection(const MusicSelection& selection) noexcept;
    [[nodiscard]] bool SetVolumePercent(float percent) noexcept;
    [[nodiscard]] MusicPcmStatus Render(std::span<float> interleavedStereoOutput) noexcept;
    void Reset() noexcept;

private:
    pocket_audio::PcmLoopStream core_;
    MusicCue previousCue_ = MusicCue::None;
};

} // namespace horde::audio
