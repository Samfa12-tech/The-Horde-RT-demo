#pragma once
#include "scene/assets/StaticMeshAsset.h"
#include "vulkan/raytracing/RtTextureLayerSubset.h"
#include <string_view>

namespace horde::vulkan::raytracing
{
// The immutable packaged atlas remains unchanged. The chapter upload omits
// only duplicate chest-lid and lantern-body layers, preserving all texels/mips.
// Runtime validates these aliases against every mip before creating an image.
inline constexpr std::array<RtTextureLayerAlias, 3> kChapterTextureAliases{{
    {4,6}, {5,7}, {8,9}}};
inline constexpr std::array<std::uint32_t, 15> kChapterBaseColorSourceLayers{{
    0,1,2,3,4,5,8,10,11,12,13,14,15,16,17}};
inline constexpr std::array<std::uint32_t, 12> kChapterNormalOrmSourceLayers{{
    0,1,2,3,4,5,8,10,11,12,13,14}};

inline bool ConfigureChapterTextureAliases(
    horde::scene::assets::StaticMeshAsset& chestBase,
    horde::scene::assets::StaticMeshAsset& chestLid,
    horde::scene::assets::StaticMeshAsset& lanternRing,
    horde::scene::assets::StaticMeshAsset& lanternBody, std::string& diagnostic)
{
    const std::array<horde::scene::assets::StaticMeshAsset*,4> assets{{
        &chestBase,&chestLid,&lanternRing,&lanternBody}};
    // Validate the complete authored roster before changing any material.
    for (std::size_t index=0;index<assets.size();++index) {
        bool wood=false,iron=false;
        for (const auto& material:assets[index]->materials) {
            const bool textured=material.baseColorTexture>=0 || material.normalTexture>=0 ||
                material.ormTexture>=0 || material.emissiveTexture>=0;
            if (!textured) continue; // Physical untextured glass remains independent.
            if ((material.name!="BlackIron" && (index>=2 || material.name!="ChestWood")) ||
                material.baseColorTexture<0 || material.normalTexture<0 ||
                material.ormTexture<0 || material.emissiveTexture>=0 || material.deferTextureAllocation) {
                diagnostic="Chapter chest/lantern texture alias roster or map presence changed.";
                return false;
            }
            wood |= material.name=="ChestWood"; iron |= material.name=="BlackIron";
        }
        if (!iron || (index<2 && !wood)) {
            diagnostic="Chapter chest/lantern texture alias provider is incomplete.";
            return false;
        }
    }
    for (std::size_t index=0;index<assets.size();++index)
        for (auto& material:assets[index]->materials)
            if (material.baseColorTexture>=0)
                material.textureGroup=index<2 && material.name=="BlackIron" ? 1 : 0;
    diagnostic.clear();
    return true;
}
}
