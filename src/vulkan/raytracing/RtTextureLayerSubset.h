#pragma once

#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace horde::vulkan::raytracing
{
// Copy complete array layers; compressed blocks and mip contents stay byte exact.
// Validate before appending so failure cannot leave a partial upload payload.
inline bool AppendRtTextureLayerSubset(const std::span<const std::uint8_t> source,
                                      const std::uint32_t sourceLayerCount,
                                      const std::uint64_t layerBytes,
                                      const std::span<const std::uint32_t> selection,
                                      std::vector<std::uint8_t>& destination)
{
    if (sourceLayerCount == 0u || layerBytes == 0u || selection.empty() ||
        layerBytes > std::numeric_limits<std::size_t>::max() / sourceLayerCount ||
        source.size() != layerBytes * sourceLayerCount ||
        layerBytes > (std::numeric_limits<std::size_t>::max() - destination.size()) / selection.size())
        return false;
    for (const auto layer : selection)
        if (layer >= sourceLayerCount) return false;
    for (const auto layer : selection)
    {
        const auto start = static_cast<std::size_t>(layerBytes * layer);
        destination.insert(destination.end(), source.begin() + start,
                           source.begin() + start + static_cast<std::size_t>(layerBytes));
    }
    return true;
}
}
