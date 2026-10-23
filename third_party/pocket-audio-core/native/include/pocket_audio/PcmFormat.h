#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace pocket_audio
{

inline constexpr std::uint32_t kPcmSampleRate = 48'000u;
inline constexpr std::uint64_t kPcmBytesPerFrame = 4u;
inline constexpr std::uint64_t kPcmDefaultMaximumFrames = 576'000u;
// Opt-in ceiling: 120 seconds, independent of a consumer's smaller asset budget.
inline constexpr std::uint64_t kPcmHardMaximumFrames = 5'760'000u;
inline constexpr std::uint64_t kPcmHardMaximumBytes =
    kPcmHardMaximumFrames * kPcmBytesPerFrame;
inline constexpr std::size_t kPcmWaveMaximumContainerOverheadBytes = 65'536u;

// Shared admission policy for one decoded WAV / one body or tail span. Both
// limits apply, including on platforms where size_t is narrower than uint64_t.
// Configuration is checked before any size multiplication or allocation.
struct PcmClipLimits
{
    std::uint64_t maximumFrames = kPcmDefaultMaximumFrames;
    std::uint64_t maximumPcmBytes = kPcmDefaultMaximumFrames * kPcmBytesPerFrame;

    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return maximumFrames > 0u && maximumFrames <= kPcmHardMaximumFrames &&
               maximumPcmBytes > 0u && maximumPcmBytes <= kPcmHardMaximumBytes &&
               maximumPcmBytes % kPcmBytesPerFrame == 0u;
    }

    [[nodiscard]] constexpr bool AllowsFrames(const std::uint64_t frames) const noexcept
    {
        return IsValid() && frames > 0u && frames <= maximumFrames &&
               frames <= maximumPcmBytes / kPcmBytesPerFrame &&
               frames <= ((std::numeric_limits<std::size_t>::max)() -
                   kPcmWaveMaximumContainerOverheadBytes) / kPcmBytesPerFrame;
    }
};

} // namespace pocket_audio
