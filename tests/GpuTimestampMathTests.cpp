#include "vulkan/GpuTimestampMath.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

namespace
{

bool Near(const double actual, const double expected, const double tolerance = 1.0e-12)
{
    return std::abs(actual - expected) <= tolerance;
}

} // namespace

int main()
{
    int failures = 0;
    const auto check = [&failures](const bool condition, const std::string& message) {
        if (!condition)
        {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    };

    const auto ordinary = horde::vulkan::ComputeGpuTimestampDuration(100u, 160u, 64u, 2.0);
    check(ordinary.has_value(), "ordinary 64-bit duration must be valid");
    check(ordinary && ordinary->elapsedTicks == 60u, "ordinary duration must retain elapsed ticks");
    check(ordinary && Near(ordinary->milliseconds, 0.00012), "ordinary duration must convert nanoseconds to milliseconds");

    const auto wrap32 = horde::vulkan::ComputeGpuTimestampDuration(0xfffffff0u, 0x20u, 32u, 1.0);
    check(wrap32 && wrap32->elapsedTicks == 48u, "32-bit counter wrap must be handled modulo its valid width");

    const std::uint64_t mask36 = (std::uint64_t{1u} << 36u) - 1u;
    const auto wrap36 = horde::vulkan::ComputeGpuTimestampDuration(mask36 - 5u, 9u, 36u, 1.0);
    check(wrap36 && wrap36->elapsedTicks == 15u, "36-bit counter wrap must be handled");

    const std::uint64_t mask48 = (std::uint64_t{1u} << 48u) - 1u;
    const auto wrap48 = horde::vulkan::ComputeGpuTimestampDuration(mask48 - 2u, 4u, 48u, 1.0);
    check(wrap48 && wrap48->elapsedTicks == 7u, "48-bit counter wrap must be handled");

    const auto wrap64 = horde::vulkan::ComputeGpuTimestampDuration(
        std::numeric_limits<std::uint64_t>::max() - 15u,
        31u,
        64u,
        1.0);
    check(wrap64 && wrap64->elapsedTicks == 47u, "64-bit unsigned wrap must not shift by the counter width");

    const auto masked = horde::vulkan::ComputeGpuTimestampDuration(0x1234fff0u, 0x98760020u, 16u, 1.0);
    check(masked && masked->elapsedTicks == 48u, "undefined timestamp bits above the valid width must be ignored");

    const auto zero = horde::vulkan::ComputeGpuTimestampDuration(42u, 42u, 64u, 1.0);
    check(zero && zero->elapsedTicks == 0u && zero->milliseconds == 0.0,
          "equal timestamps must produce a valid zero duration");

    check(!horde::vulkan::ComputeGpuTimestampDuration(0u, 1u, 0u, 1.0),
          "zero valid timestamp bits must be rejected");
    check(!horde::vulkan::ComputeGpuTimestampDuration(0u, 1u, 65u, 1.0),
          "timestamp widths greater than 64 bits must be rejected");
    check(!horde::vulkan::ComputeGpuTimestampDuration(0u, 1u, 64u, 0.0),
          "zero timestamp period must be rejected");
    check(!horde::vulkan::ComputeGpuTimestampDuration(0u, 1u, 64u, -1.0),
          "negative timestamp period must be rejected");
    check(!horde::vulkan::ComputeGpuTimestampDuration(
              0u, 1u, 64u, std::numeric_limits<double>::quiet_NaN()),
          "NaN timestamp period must be rejected");
    check(!horde::vulkan::ComputeGpuTimestampDuration(
              0u, 1u, 64u, std::numeric_limits<double>::infinity()),
          "infinite timestamp period must be rejected");
    check(!horde::vulkan::ComputeGpuTimestampDuration(
              0u,
              std::numeric_limits<std::uint64_t>::max(),
              64u,
              std::numeric_limits<double>::max()),
          "non-finite converted duration must be rejected");

    if (failures == 0)
    {
        std::cout << "GPU timestamp math tests passed.\n";
    }
    return failures == 0 ? 0 : 1;
}
