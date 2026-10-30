#pragma once
#include "gameplay/simulation/TriangleSupportSurface.h"
#include "scene/assets/StaticMeshAsset.h"

namespace horde::scene
{
inline std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>
PrepareChapterTerrainSurface(const assets::StaticMeshAsset& asset,std::string& diagnostic)
{
    using Surface=gameplay::simulation::TriangleSupportSurface;
    std::vector<Surface::Triangle> triangles;
    if(asset.indices.size()%3 || asset.indices.size()/3>Surface::kMaximumTriangles) {
        diagnostic="Chapter terrain index count is invalid.";return {};
    }
    triangles.reserve(asset.indices.size()/3);
    std::size_t consumed=0;
    for(std::size_t primitiveIndex=0;primitiveIndex<asset.primitives.size();++primitiveIndex) {
        const auto& primitive=asset.primitives[primitiveIndex];
        const std::size_t vertexEnd=primitiveIndex+1<asset.primitives.size() ?
            asset.primitives[primitiveIndex+1].vertexOffset : asset.vertices.size();
        if(primitive.indexOffset!=consumed || primitive.indexCount%3 ||
           !primitive.indexCount || primitive.vertexOffset>=vertexEnd || vertexEnd>asset.vertices.size() ||
           primitive.indexOffset>asset.indices.size() || primitive.indexCount>asset.indices.size()-primitive.indexOffset ||
           primitive.materialIndex>=asset.materials.size()) {
            diagnostic="Chapter terrain primitive ranges or material identity are invalid.";return {};
        }
        const auto& name=asset.materials[primitive.materialIndex].name;
        if(name!="ChapterTrailDampSand" && name!="ChapterBankStone" && name!="ChapterForestGround") {
            diagnostic="Chapter terrain has an unadmitted support material.";return {};
        }
        for(std::size_t i=0;i<primitive.indexCount;i+=3) {
            Surface::Triangle triangle;
            triangle.surface=name!="ChapterBankStone" ? gameplay::simulation::SupportSurface::Earth :
                                                          gameplay::simulation::SupportSurface::Stone;
            for(std::size_t k=0;k<3;++k) {
                const auto local=asset.indices[primitive.indexOffset+i+k];
                if(local>=vertexEnd-primitive.vertexOffset) {
                    diagnostic="Chapter terrain vertex reference is invalid.";return {};
                }
                // Native import already baked node transforms into world-space
                // vertices. Applying them again would double elevation/scale.
                const auto& p=asset.vertices[primitive.vertexOffset+local].position;
                triangle.points[k]={p[0],p[1],p[2]};
            }
            triangles.push_back(triangle);
        }
        consumed+=primitive.indexCount;
    }
    if(consumed!=asset.indices.size()) {
        diagnostic="Chapter terrain has unowned indices.";return {};
    }
    return Surface::Build(triangles,diagnostic);
}
}
