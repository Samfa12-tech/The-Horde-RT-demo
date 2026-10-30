#pragma once
#include "scene/assets/StaticMeshAsset.h"
#include <algorithm>
#include <limits>
#include <utility>

namespace horde::scene::assets {
// Build a separately owned candidate. Region labels never authorize eviction:
// caller must prove contributor retention before using a partial GPU candidate.
// Shared/unclassified triangles cannot be removed by this API.
inline bool SelectStaticTriangleRegions(const StaticMeshAsset& source,
    std::uint8_t retainedRegions, StaticMeshAsset& out, std::string& diagnostic) {
    const auto reject=[&](const char* reason){diagnostic=reason;return false;};
    if(&source==&out || retainedRegions>kAllStaticTriangleRegions || !source.HasValidTriangleProvenance() ||
       source.indices.size()>UINT32_MAX || source.vertices.size()>UINT32_MAX || source.primitives.empty() ||
       source.materials.empty())
        return reject("Static triangle selection rejected invalid ownership, regions or geometry.");
    StaticMeshAsset selected;
    selected.materials=source.materials;selected.nodeTransforms=source.nodeTransforms;
    selected.sockets=source.sockets;selected.bounds=source.bounds;
    std::size_t consumed=0;
    for(std::size_t p=0;p<source.primitives.size();++p) {
        const auto& primitive=source.primitives[p];
        const auto end=p+1<source.primitives.size()?source.primitives[p+1].vertexOffset:source.vertices.size();
        if((p==0 && primitive.vertexOffset!=0) || primitive.vertexOffset>=end || end>source.vertices.size() ||
           primitive.indexOffset!=consumed || !primitive.indexCount || primitive.indexCount%3u ||
           consumed>source.indices.size() || primitive.indexCount>source.indices.size()-consumed ||
           primitive.materialIndex>=source.materials.size() || primitive.nodeTransformIndex>=source.nodeTransforms.size())
            return reject("Static triangle selection rejected malformed primitive spans.");
        const auto regionAt=[&](std::size_t index){return index/3u<source.triangleRegions.size()
            ?source.triangleRegions[index/3u]:StaticTriangleRegion::Shared;};
        const auto keep=[&](std::size_t index){const auto region=static_cast<std::uint8_t>(regionAt(index));
            return region==0u || (retainedRegions & region)!=0u;};
        std::vector<bool> used(end-primitive.vertexOffset,false);
        std::size_t kept=0;
        for(std::size_t i=consumed;i<consumed+primitive.indexCount;i+=3u) {
            for(unsigned corner=0;corner<3;++corner) {
                const auto local=source.indices[i+corner];
                if(local>=used.size())return reject("Static triangle selection rejected invalid local indices.");
                if(keep(i))used[local]=true;
            }
            kept+=keep(i);
        }
        consumed+=primitive.indexCount;
        if(!kept)continue;
        auto result=primitive;
        result.vertexOffset=static_cast<std::uint32_t>(selected.vertices.size());
        result.indexOffset=static_cast<std::uint32_t>(selected.indices.size());
        std::vector<std::uint32_t> remap(used.size(),UINT32_MAX);
        if(kept==primitive.indexCount/3u)std::fill(used.begin(),used.end(),true);
        for(std::size_t i=0;i<used.size();++i)if(used[i]) {
            remap[i]=static_cast<std::uint32_t>(selected.vertices.size())-result.vertexOffset;
            selected.vertices.push_back(source.vertices[primitive.vertexOffset+i]);
        }
        for(std::size_t i=primitive.indexOffset;i<consumed;i+=3u)if(keep(i)) {
            for(unsigned corner=0;corner<3;++corner)selected.indices.push_back(remap[source.indices[i+corner]]);
            selected.triangleRegions.push_back(regionAt(i));
        }
        result.indexCount=static_cast<std::uint32_t>(selected.indices.size())-result.indexOffset;
        selected.primitives.push_back(result);
    }
    if(consumed!=source.indices.size() || selected.primitives.empty())
        return reject("Static triangle selection rejected unowned indices or an empty candidate.");
    out=std::move(selected);diagnostic.clear();return true;
}
}
