#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "pocket_audio/PcmWave.h"

namespace horde::audio
{

using MusicPcmWaveStatus = pocket_audio::PcmWaveStatus;

// Horde naming adapter; all parsing lives in Pocket Audio Core.
// Decodes one strict 48 kHz, stereo, signed PCM16 RIFF/WAVE clip. The expected
// frame count must be in 1..576000. Input bytes are parsed explicitly as
// little-endian; no alignment or host-endianness assumptions are made.
//
// Output is transactional: it is empty on every failure and contains
// interleaved caller-owned samples only on success. The container may add at
// most 64 KiB beyond the expected PCM payload. This routine allocates and is
// intended for asset loading off the audio thread, never from Render().
[[nodiscard]] MusicPcmWaveStatus DecodeMusicPcmWave(
    std::span<const std::uint8_t> fileBytes,
    std::uint32_t expectedFrameCount,
    std::vector<std::int16_t>& outputSamples) noexcept;

} // namespace horde::audio
