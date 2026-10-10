#pragma once

#include "scene/assets/StaticMeshAsset.h"
#include <limits>

namespace horde::scene::assets {

// Imported static vertices already contain their node/world transforms. Merge
// only adjacent index ranges using the very same material. Vertex bytes,
// triangle order, winding, UVs, normals, sockets and material records stay intact.
// Do not apply to independently animated/skinned geometry.
inline bool CoalesceAdjacentWorldBakedPrimitives(StaticMeshAsset& asset,
                                               std::string& diagnostic) {
    auto indices = asset.indices;
    std::vector<StaticPrimitiveRecord> primitives;
    if (asset.vertices.size() > std::numeric_limits<std::uint32_t>::max()) {
        diagnostic = "Static primitive grouping rejected an unaddressable vertex array.";
        return false;
    }
    for (std::size_t i = 0; i < asset.primitives.size(); ++i) {
        const auto& next = asset.primitives[i];
        const auto end = i + 1 < asset.primitives.size()
            ? asset.primitives[i + 1].vertexOffset : asset.vertices.size();
        if (next.materialIndex >= asset.materials.size() ||
            next.nodeTransformIndex >= asset.nodeTransforms.size() ||
            next.vertexOffset >= end || end > asset.vertices.size() ||
            next.indexOffset > indices.size() ||
            next.indexCount > indices.size() - next.indexOffset || next.indexCount % 3 != 0) {
            diagnostic = "Static primitive grouping rejected an invalid primitive span.";
            return false;
        }
        const auto indexEnd = static_cast<std::size_t>(next.indexOffset) + next.indexCount;
        for (std::size_t n = next.indexOffset; n < indexEnd; ++n)
            if (indices[n] >= end - next.vertexOffset) {
                diagnostic = "Static primitive grouping rejected an out-of-range local index.";
                return false;
            }
        if (!primitives.empty() && primitives.back().materialIndex == next.materialIndex &&
            static_cast<std::uint64_t>(primitives.back().indexOffset) + primitives.back().indexCount == next.indexOffset) {
            auto& previous = primitives.back();
            const auto offset = next.vertexOffset - previous.vertexOffset;
            if (static_cast<std::uint64_t>(previous.indexCount) + next.indexCount >
                std::numeric_limits<std::uint32_t>::max()) {
                diagnostic = "Static primitive grouping rejected index-count overflow.";
                return false;
            }
            for (std::size_t n = next.indexOffset; n < indexEnd; ++n)
                indices[n] += offset; // validated global index remains below end
            previous.indexCount += next.indexCount;
        } else primitives.push_back(next);
    }
    asset.indices = std::move(indices);
    asset.primitives = std::move(primitives);
    diagnostic.clear();
    return true;
}
} // namespace horde::scene::assets
