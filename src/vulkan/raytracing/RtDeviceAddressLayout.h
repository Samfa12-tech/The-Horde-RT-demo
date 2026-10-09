#pragma once

#include <cstdint>
#include <limits>

namespace horde::vulkan::raytracing
{

// Device-address alignment is independent of the allocation's memory alignment.
inline bool IsDeviceAddressAlignment(const std::uint64_t alignment) noexcept
{
    return alignment != 0u && (alignment & (alignment - 1u)) == 0u;
}

inline bool TryPaddedDeviceAddressSize(const std::uint64_t size,
                                      const std::uint64_t alignment,
                                      std::uint64_t& paddedSize) noexcept
{
    paddedSize = 0u;
    if (size == 0u || !IsDeviceAddressAlignment(alignment) ||
        size > std::numeric_limits<std::uint64_t>::max() - (alignment - 1u))
        return false;
    paddedSize = size + alignment - 1u;
    return true;
}

inline bool TryAlignDeviceAddressRange(const std::uint64_t baseAddress,
                                       const std::uint64_t bufferSize,
                                       const std::uint64_t requiredSize,
                                       const std::uint64_t alignment,
                                       std::uint64_t& offset) noexcept
{
    offset = 0u;
    if (baseAddress == 0u || bufferSize == 0u || requiredSize == 0u ||
        !IsDeviceAddressAlignment(alignment) ||
        bufferSize - 1u > std::numeric_limits<std::uint64_t>::max() - baseAddress)
        return false;
    const auto remainder = baseAddress % alignment;
    const auto padding = remainder == 0u ? 0u : alignment - remainder;
    if (padding > bufferSize || requiredSize > bufferSize - padding)
        return false;
    offset = padding;
    return true;
}

struct RtShaderBindingTableLayout
{
    std::uint64_t stride = 0u;
    std::uint64_t regionSpacing = 0u;
    std::uint64_t size = 0u;
};

// Three single-record regions: raygen, miss, hit. Raygen size equals stride;
// each region starts at baseAlignment and each record fits wholly in the buffer.
inline bool TryShaderBindingTableLayout(const std::uint32_t handleSize,
                                        const std::uint32_t handleAlignment,
                                        const std::uint32_t baseAlignment,
                                        const std::uint32_t maxStride,
                                        RtShaderBindingTableLayout& out) noexcept
{
    out = {};
    if (handleSize == 0u || !IsDeviceAddressAlignment(handleAlignment) ||
        !IsDeviceAddressAlignment(baseAlignment))
        return false;
    const std::uint64_t stride =
        (static_cast<std::uint64_t>(handleSize) + handleAlignment - 1u) &
        ~(static_cast<std::uint64_t>(handleAlignment) - 1u);
    if (stride > maxStride)
        return false;
    const std::uint64_t spacing = (stride + baseAlignment - 1u) &
        ~(static_cast<std::uint64_t>(baseAlignment) - 1u);
    out = {stride, spacing, spacing * 3u};
    return true;
}

} // namespace horde::vulkan::raytracing
