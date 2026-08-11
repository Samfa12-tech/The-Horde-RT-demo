#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>

namespace horde::vulkan
{

struct GpuTimestampDuration
{
    std::uint64_t elapsedTicks = 0u;
    double milliseconds = 0.0;
};

// Timestamp counters may expose fewer than 64 valid bits. Mask both samples
// and subtract modulo the valid counter width so a single wrap between the
// beginning and end of a frame is handled deterministically.
inline std::optional<GpuTimestampDuration> ComputeGpuTimestampDuration(
    const std::uint64_t begin,
    const std::uint64_t end,
    const std::uint32_t validBits,
    const double timestampPeriodNanoseconds)
{
    if (validBits == 0u || validBits > 64u ||
        !std::isfinite(timestampPeriodNanoseconds) || timestampPeriodNanoseconds <= 0.0)
    {
        return std::nullopt;
    }

    const std::uint64_t validMask = validBits == 64u
        ? std::numeric_limits<std::uint64_t>::max()
        : (std::uint64_t{1u} << validBits) - 1u;
    const std::uint64_t elapsedTicks = ((end & validMask) - (begin & validMask)) & validMask;
    const double milliseconds = static_cast<double>(elapsedTicks) * timestampPeriodNanoseconds / 1'000'000.0;
    if (!std::isfinite(milliseconds))
    {
        return std::nullopt;
    }

    return GpuTimestampDuration{elapsedTicks, milliseconds};
}

} // namespace horde::vulkan
