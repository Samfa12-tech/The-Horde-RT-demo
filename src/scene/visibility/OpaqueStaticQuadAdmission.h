#pragma once
#include "scene/visibility/OpaqueSegmentCoverage.h"
#include "scene/assets/StaticMeshAsset.h"

namespace horde::scene::visibility
{
// The admitted static loader world-bakes its vertices. This adapter may only
// witness Shared triangles in a pinned immutable owner with identity TLAS
// transform. Moving/unproved ownership and potentially filtered materials fail
// closed. Shared is guaranteed retained only by the triangle-region selection
// path; arbitrary primitive-family omissions need their own retention proof.
// The result is still NOT a ray-origin closure/unload certificate.
inline std::optional<OpaqueQuadWitness> AdmitRetainedStaticQuad(
    const assets::StaticMeshAsset& source,std::size_t primitiveIndex,std::size_t localIndex,
    const std::array<float,12>& worldFromInstance,std::uint8_t mask,
    std::uint64_t sourceGeneration,std::uint64_t expectedGeneration,bool immutableOwner)
{
    constexpr std::array<float,12> identity{{1,0,0,0,0,1,0,0,0,0,1,0}};
    if(!immutableOwner || !sourceGeneration || sourceGeneration!=expectedGeneration ||
       worldFromInstance!=identity || primitiveIndex>=source.primitives.size() ||
       source.indices.size()%3 || source.triangleRegions.size()>source.indices.size()/3 || localIndex%3)
        return std::nullopt;
    const auto& primitive=source.primitives[primitiveIndex];
    if(primitive.materialIndex>=source.materials.size() || primitive.indexCount%3 ||
       primitive.indexOffset%3 || localIndex>primitive.indexCount || primitive.indexCount-localIndex<6 ||
       primitive.indexOffset>source.indices.size() || primitive.indexCount>source.indices.size()-primitive.indexOffset)
        return std::nullopt;
    const auto& material=source.materials[primitive.materialIndex];
    // Only ordinary opaque shading flags are admitted. No alpha, transmission,
    // primary filtering, thin-volume or unknown semantics may certify coverage.
    constexpr std::uint32_t ordinaryShadingFlags=1u|8u|16u|32u;
    if((material.flags&~ordinaryShadingFlags)!=0 || material.transmissionFactor!=0.0f ||
       material.emissiveTexture>=0 || std::any_of(material.emissiveFactor.begin(),material.emissiveFactor.end(),
            [](float value){return value!=0.0f;}))
        return std::nullopt;
    const std::size_t vertexEnd=primitiveIndex+1<source.primitives.size()
        ? source.primitives[primitiveIndex+1].vertexOffset : source.vertices.size();
    if(primitive.vertexOffset>=vertexEnd || vertexEnd>source.vertices.size()) return std::nullopt;
    const std::size_t first=primitive.indexOffset+localIndex;
    for(std::size_t triangle=first/3;triangle<(first+6)/3;++triangle)
        if(triangle<source.triangleRegions.size() && source.triangleRegions[triangle]!=assets::StaticTriangleRegion::Shared)
            return std::nullopt;
    std::array<Point3,6> points{};
    for(unsigned i=0;i<6;++i) {
        const auto local=source.indices[first+i];
        if(local>=vertexEnd-primitive.vertexOffset) return std::nullopt;
        const auto& position=source.vertices[primitive.vertexOffset+local].position;
        if(position[3]!=1.f) return std::nullopt;
        for(unsigned axis=0;axis<3;++axis) points[i][axis]=position[axis];
    }
    return OpaqueQuadWitness::Build(points,mask,sourceGeneration,true);
}
}
