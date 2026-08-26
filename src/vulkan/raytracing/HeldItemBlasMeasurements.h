#pragma once

#include <cstdint>

namespace horde::vulkan::raytracing
{

struct HeldItemBlasMeasurements
{
    std::uint64_t torchBytes = 0u;
    std::uint64_t swordBytes = 0u;
    double torchBuildMilliseconds = 0.0;
    double swordBuildMilliseconds = 0.0;

    void RecordTorch(const std::uint64_t bytes, const double buildMilliseconds)
    {
        torchBytes = bytes;
        torchBuildMilliseconds = buildMilliseconds;
    }

    void RecordSword(const std::uint64_t bytes, const double buildMilliseconds)
    {
        swordBytes = bytes;
        swordBuildMilliseconds = buildMilliseconds;
    }

    std::uint64_t TotalBytes() const { return torchBytes + swordBytes; }
    double TotalBuildMilliseconds() const
    {
        return torchBuildMilliseconds + swordBuildMilliseconds;
    }
};

} // namespace horde::vulkan::raytracing
