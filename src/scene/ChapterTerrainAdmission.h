#pragma once
#include "scene/ChapterTerrainSurface.h"
#include "scene/assets/AssetManifest.h"
#include <filesystem>
#include <limits>

namespace horde::scene
{
// Same native-imported world-space vertices supply render and support. Batches
// remain one existing static owner; this is admission, not zone streaming.
inline bool AppendChapterTerrain(const std::filesystem::path& root,
                                assets::StaticMeshAsset& owner,
                                std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>& support,
                                std::string& diagnostic)
{
    const auto directory=root/"models/world/runtime/chapter-terrain-v1";
    assets::AssetManifest manifest;assets::StaticMeshAsset terrain;
    if(!assets::AssetManifest::Load(directory/"asset.manifest.json",manifest,diagnostic) ||
       !assets::StaticMeshAsset::Load(directory/"chapter-terrain-lod0.runtime.glb",manifest,terrain,diagnostic))return false;
    auto prepared=PrepareChapterTerrainSurface(terrain,diagnostic);
    if(!prepared)return false;
    if(terrain.materials.size()!=2 || terrain.primitives.size()!=2 ||
       terrain.materials[0].name!="ChapterTrailDampSand" || terrain.materials[1].name!="ChapterForestGround" ||
       owner.materials.size()+2>32 || owner.primitives.size()+2>32 ||
       owner.vertices.size()>std::numeric_limits<std::uint32_t>::max()-terrain.vertices.size() ||
       owner.indices.size()>std::numeric_limits<std::uint32_t>::max()-terrain.indices.size() ||
       owner.vertices.empty() || owner.indices.empty() || owner.nodeTransforms.empty()) {
        diagnostic="Chapter terrain native roster or existing static-owner capacity differs.";return false;
    }
    for(const auto& m:owner.materials)if(m.name==terrain.materials[0].name || m.name==terrain.materials[1].name) {
        diagnostic="Chapter terrain was already admitted to this owner.";return false;
    }
    for(std::size_t i=0;i<terrain.materials.size();++i) {
        auto& material=terrain.materials[i];
        if(material.baseColorTexture<0 || material.normalTexture<0 || material.ormTexture<0 ||
           material.emissiveTexture>=0 || material.transmissionFactor!=0 || material.metallicFactor!=0) {
            diagnostic="Chapter terrain must retain its admitted opaque ground PBR maps.";return false;
        }
        material.textureGroup=static_cast<std::int32_t>(4+i); // after the four forest groups
        material.deferTextureAllocation=true;
    }
    assets::StaticMeshAsset staged=owner;
    const auto vertices=static_cast<std::uint32_t>(staged.vertices.size());
    const auto indices=static_cast<std::uint32_t>(staged.indices.size());
    const auto materials=static_cast<std::uint32_t>(staged.materials.size());
    const auto transforms=static_cast<std::uint32_t>(staged.nodeTransforms.size());
    staged.vertices.insert(staged.vertices.end(),terrain.vertices.begin(),terrain.vertices.end());
    staged.indices.insert(staged.indices.end(),terrain.indices.begin(),terrain.indices.end());
    staged.materials.insert(staged.materials.end(),terrain.materials.begin(),terrain.materials.end());
    staged.nodeTransforms.insert(staged.nodeTransforms.end(),terrain.nodeTransforms.begin(),terrain.nodeTransforms.end());
    for(auto primitive:terrain.primitives) {
        primitive.vertexOffset+=vertices;primitive.indexOffset+=indices;
        primitive.materialIndex+=materials;primitive.nodeTransformIndex+=transforms;
        staged.primitives.push_back(primitive);
    }
    for(std::size_t axis=0;axis<3;++axis) {
        staged.bounds.minimum[axis]=std::min(staged.bounds.minimum[axis],terrain.bounds.minimum[axis]);
        staged.bounds.maximum[axis]=std::max(staged.bounds.maximum[axis],terrain.bounds.maximum[axis]);
    }
    owner=std::move(staged);support=std::move(prepared);diagnostic.clear();return true;
}
}
