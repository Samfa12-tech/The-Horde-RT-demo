#include "vulkan/raytracing/RtTextureLayerSubset.h"
#include "vulkan/raytracing/RtChapterTextureLayout.h"

#include <array>
#include <iostream>
#include <limits>

int main()
{
    using horde::vulkan::raytracing::AppendRtTextureLayerSubset;
    bool passed = true;
    const auto check = [&passed](bool condition, const char* label) {
        if (!condition) { std::cerr << label << "\n"; passed = false; }
    };
    const std::array<std::uint8_t, 12> source{10,11,12,13,20,21,22,23,30,31,32,33};
    const std::array<std::uint32_t, 3> reorder{2,0,2};
    std::vector<std::uint8_t> destination{99};
    check(AppendRtTextureLayerSubset(source, 3, 4, reorder, destination), "valid compressed layer selection");
    check(destination == std::vector<std::uint8_t>({99,30,31,32,33,10,11,12,13,30,31,32,33}), "byte exact reordered/repeated layers");
    const std::array<std::uint8_t, 3> nextMip{1,2,3};
    check(AppendRtTextureLayerSubset(nextMip, 3, 1, reorder, destination), "append subsequent mip");
    check(destination[destination.size()-3] == 3 && destination[destination.size()-2] == 1 && destination.back() == 3, "mip layer order retained");
    const auto saved = destination;
    using horde::scene::assets::StaticMeshAsset;
    using horde::scene::assets::StaticMaterial;
    const auto material=[](const char* name) {
        StaticMaterial m; m.name=name;m.baseColorTexture=0;m.normalTexture=1;m.ormTexture=2;
        return m;
    };
    StaticMeshAsset base,lid,ring,body;
    base.materials={material("ChestWood"),material("BlackIron")}; lid=base;
    ring.materials={material("BlackIron")};body=ring;
    std::string diagnostic;
    auto badBody=body;badBody.materials[0].emissiveTexture=3;
    check(!horde::vulkan::raytracing::ConfigureChapterTextureAliases(base,lid,ring,badBody,diagnostic),
          "unexpected map category cannot share a provider");
    check(base.materials[0].textureGroup==-1 && lid.materials[1].textureGroup==-1,
          "failed group admission cannot partially mutate preceding assets");
    badBody=body;badBody.materials[0].name="DifferentMetal";
    check(!horde::vulkan::raytracing::ConfigureChapterTextureAliases(base,lid,ring,badBody,diagnostic),
          "different authored material family cannot alias by accident");
    check(horde::vulkan::raytracing::ConfigureChapterTextureAliases(base,lid,ring,body,diagnostic),
          "admitted texture families receive explicit provider groups");
    check(base.materials[0].textureGroup==0 && base.materials[1].textureGroup==1 &&
          lid.materials[0].textureGroup==0 && lid.materials[1].textureGroup==1 &&
          ring.materials[0].textureGroup==0 && body.materials[0].textureGroup==0,
          "chest wood/iron and lantern iron keep distinct canonical groups");
    const std::array<std::uint8_t,12> duplicates{10,11,12,13,20,21,22,23,10,11,12,13};
    const std::array<horde::vulkan::raytracing::RtTextureLayerAlias,1> aliases{{{0,2}}};
    check(AppendRtTextureLayerSubset(duplicates,3,4,reorder,destination,aliases), "identical encoded layers alias");
    const auto aliasedSaved=destination;
    check(!AppendRtTextureLayerSubset(source,3,4,reorder,destination,aliases), "different encoded layers cannot alias");
    check(destination==aliasedSaved, "failed alias leaves payload unchanged");
    const std::array<horde::vulkan::raytracing::RtTextureLayerAlias,1> invalidAlias{{{0,3}}};
    check(!AppendRtTextureLayerSubset(duplicates,3,4,reorder,destination,invalidAlias), "alias bounds checked before access");
    const std::array<std::uint8_t,3> corruptLaterMip{1,2,3};
    check(!AppendRtTextureLayerSubset(corruptLaterMip,3,1,reorder,destination,aliases), "later mip disagreement cannot be hidden by mip zero equality");
    check(destination==aliasedSaved, "invalid alias and mip do not append partial data");
    destination=saved;
    const std::array<std::uint32_t, 2> badSelection{0,3};
    check(!AppendRtTextureLayerSubset(source, 3, 4, badSelection, destination), "reject invalid later layer before append");
    check(destination == saved, "invalid layer leaves destination unchanged");
    check(!AppendRtTextureLayerSubset(std::span<const std::uint8_t>(source).first(11), 3, 4, reorder, destination), "reject truncated full source");
    check(!AppendRtTextureLayerSubset(source, 3, 3, reorder, destination), "reject oversized full source");
    check(!AppendRtTextureLayerSubset(source, 0, 4, reorder, destination), "reject zero source layers");
    check(!AppendRtTextureLayerSubset(source, 3, 0, reorder, destination), "reject zero layer size");
    check(!AppendRtTextureLayerSubset(source, 3, 4, {}, destination), "reject empty admission");
    check(!AppendRtTextureLayerSubset(source, 3, std::numeric_limits<std::uint64_t>::max(), reorder, destination), "reject host size multiplication overflow");
    check(destination == saved, "all malformed payloads retain destination");
    return passed ? 0 : 1;
}
