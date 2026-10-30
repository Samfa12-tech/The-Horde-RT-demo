#pragma once

#include <cstdint>
#include <algorithm>
#include <array>
#include <limits>
#include <span>
#include <vector>

namespace horde::vulkan::raytracing
{
using RtTextureLayerAlias = std::array<std::uint32_t, 2u>;

// Admission can share a layer only when its complete encoded mip is identical.
// This checks the actual upload bytes, including ASTC blocks, not source names.
inline bool ValidateRtTextureLayerAliases(std::span<const std::uint8_t> source,
                                         std::uint32_t sourceLayerCount,
                                         std::uint64_t layerBytes,
                                         std::span<const RtTextureLayerAlias> aliases)
{
    if (sourceLayerCount == 0u || layerBytes == 0u ||
        layerBytes > std::numeric_limits<std::size_t>::max() / sourceLayerCount ||
        source.size() != layerBytes * sourceLayerCount) return false;
    for (const auto& alias : aliases)
    {
        if (alias[0] >= sourceLayerCount || alias[1] >= sourceLayerCount) return false;
        const auto first = source.subspan(static_cast<std::size_t>(alias[0] * layerBytes),
                                         static_cast<std::size_t>(layerBytes));
        const auto second = source.subspan(static_cast<std::size_t>(alias[1] * layerBytes),
                                          static_cast<std::size_t>(layerBytes));
        if (!std::equal(first.begin(), first.end(), second.begin())) return false;
    }
    return true;
}
// Copy complete array layers; compressed blocks and mip contents stay byte exact.
// Validate before appending so failure cannot leave a partial upload payload.
inline bool AppendRtTextureLayerSubset(const std::span<const std::uint8_t> source,
                                      const std::uint32_t sourceLayerCount,
                                      const std::uint64_t layerBytes,
                                      const std::span<const std::uint32_t> selection,
                                      std::vector<std::uint8_t>& destination,
                                      const std::span<const RtTextureLayerAlias> aliases = {})
{
    if (sourceLayerCount == 0u || layerBytes == 0u || selection.empty() ||
        layerBytes > std::numeric_limits<std::size_t>::max() / sourceLayerCount ||
        source.size() != layerBytes * sourceLayerCount ||
        layerBytes > (std::numeric_limits<std::size_t>::max() - destination.size()) / selection.size())
        return false;
    if (!ValidateRtTextureLayerAliases(source, sourceLayerCount, layerBytes, aliases)) return false;
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
