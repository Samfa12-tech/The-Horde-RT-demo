#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "pocket_audio/PcmFormat.h"

namespace pocket_audio
{

// Legacy default, retained for source compatibility; not the opt-in ceiling.
inline constexpr std::uint32_t kPcmWaveMaximumFrameCount =
    static_cast<std::uint32_t>(kPcmDefaultMaximumFrames);

enum class PcmWaveStatus : std::uint8_t
{
    Ok,
    InvalidExpectedFrameCount,
    FileTooLarge,
    InvalidRiffHeader,
    InvalidRiffLength,
    InvalidChunk,
    InvalidChunkPadding,
    DuplicateFormatChunk,
    DuplicateDataChunk,
    MissingFormatChunk,
    MissingDataChunk,
    InvalidFormatChunk,
    UnsupportedEncoding,
    WrongChannelCount,
    WrongSampleRate,
    WrongBitDepth,
    WrongBlockAlignment,
    WrongByteRate,
    OddDataLength,
    InvalidDataAlignment,
    WrongFrameCount,
    AllocationFailure,
    InvalidLimits,
};

// Decode one strict 48 kHz stereo signed PCM16 RIFF/WAVE clip. Expected frames
// default to 1..576000. Parsing is explicitly little-endian and independent
// of input alignment and host byte order. The caller-owned output is empty on
// every failure and replaced only on success. Call off the audio thread.
[[nodiscard]] PcmWaveStatus DecodePcmWave(
    std::span<const std::uint8_t> fileBytes,
    std::uint32_t expectedFrameCount,
    std::vector<std::int16_t>& outputSamples) noexcept;

// Explicit bounded admission, shared with PcmLoopStream. Invalid policy is
// rejected before parsing/allocation; expected frames must satisfy both limits.
[[nodiscard]] PcmWaveStatus DecodePcmWave(
    std::span<const std::uint8_t> fileBytes,
    std::uint32_t expectedFrameCount,
    std::vector<std::int16_t>& outputSamples,
    PcmClipLimits limits) noexcept;

} // namespace pocket_audio
