#pragma once

#include "scene/assets/StaticMeshAsset.h"
#include <algorithm>
#include <limits>

namespace horde::scene::assets {

// Consolidate disconnected ranges of the same material in first-appearance
// order. All inputs must already be world-baked static geometry. Copy full
// vertex bytes and local indices; leave materials, sockets and bounds intact.
// This prevents dressing reuse from consuming extra primitive metadata rows.
inline bool ConsolidateWorldBakedMaterialFamilies(StaticMeshAsset& asset,
                                                std::string& diagnostic) {
    if(asset.primitives.empty() || asset.vertices.size()>std::numeric_limits<std::uint32_t>::max() ||
        asset.indices.size()>std::numeric_limits<std::uint32_t>::max()) {
        diagnostic="Static material consolidation has empty/unaddressable geometry.";return false;
    }
    std::vector<std::uint32_t> families;
    std::size_t consumed=0;
    for(std::size_t i=0;i<asset.primitives.size();++i) {
        const auto& p=asset.primitives[i];
        const auto end=i+1<asset.primitives.size()?asset.primitives[i+1].vertexOffset:asset.vertices.size();
        if(p.materialIndex>=asset.materials.size() || p.nodeTransformIndex>=asset.nodeTransforms.size() ||
            (i==0 && p.vertexOffset!=0) || p.vertexOffset>=end || end>asset.vertices.size() ||
            p.indexOffset!=consumed || p.indexCount%3 || !p.indexCount ||
            p.indexOffset>asset.indices.size() || p.indexCount>asset.indices.size()-p.indexOffset) {
            diagnostic="Static material consolidation rejected an invalid primitive span.";return false;
        }
        for(std::size_t j=0;j<p.indexCount;++j)if(asset.indices[p.indexOffset+j]>=end-p.vertexOffset) {
            diagnostic="Static material consolidation rejected an invalid local index.";return false;
        }
        if(std::find(families.begin(),families.end(),p.materialIndex)==families.end())families.push_back(p.materialIndex);
        consumed+=p.indexCount;
    }
    if(consumed!=asset.indices.size()) {diagnostic="Static material consolidation rejected unowned indices.";return false;}
    std::vector<StaticRtVertex> vertices;vertices.reserve(asset.vertices.size());
    std::vector<std::uint32_t> indices;indices.reserve(asset.indices.size());
    std::vector<StaticPrimitiveRecord> primitives;
    for(const auto family:families) {
        StaticPrimitiveRecord combined;
        combined.vertexOffset=static_cast<std::uint32_t>(vertices.size());
        combined.indexOffset=static_cast<std::uint32_t>(indices.size());combined.materialIndex=family;
        combined.nodeTransformIndex=asset.primitives.front().nodeTransformIndex;
        for(std::size_t i=0;i<asset.primitives.size();++i) {
            const auto& p=asset.primitives[i];if(p.materialIndex!=family)continue;
            const auto end=i+1<asset.primitives.size()?asset.primitives[i+1].vertexOffset:asset.vertices.size();
            const auto localBase=static_cast<std::uint32_t>(vertices.size())-combined.vertexOffset;
            vertices.insert(vertices.end(),asset.vertices.begin()+p.vertexOffset,asset.vertices.begin()+end);
            for(std::size_t j=0;j<p.indexCount;++j)indices.push_back(localBase+asset.indices[p.indexOffset+j]);
        }
        combined.indexCount=static_cast<std::uint32_t>(indices.size())-combined.indexOffset;
        primitives.push_back(combined);
    }
    asset.vertices=std::move(vertices);asset.indices=std::move(indices);asset.primitives=std::move(primitives);
    diagnostic.clear();return true;
}

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
