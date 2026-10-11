#pragma once
#include "vulkan/raytracing/RtChapterTextureLayout.h"
#include "vulkan/raytracing/RtSceneProfile.h"

namespace horde::vulkan::raytracing
{
struct RtStaticTextureSelection
{
    std::span<const std::uint32_t> baseColor;
    std::span<const std::uint32_t> normalOrm;
    std::span<const RtTextureLayerAlias> aliases;
    bool appendTerrain = false;
};

inline RtStaticTextureSelection SelectRtStaticTextureLayers(
    RtSceneProfile profile, bool productionAssets, bool rescueJourney)
{
    static constexpr std::array<std::uint32_t,3> preview{1,8,9};
    static constexpr std::array<std::uint32_t,2> entry{8,9};
    // The packaged atlas also holds forest maps. Ordinary startup (including
    // Android before a pending checkpoint is applied) still owns only these
    // unchanged leading rows; it must not require a physically smaller file.
    static constexpr std::array<std::uint32_t,14> legacyBase{0,1,2,3,4,5,6,7,8,9,10,11,12,13};
    static constexpr auto legacyNormalOrm = legacyBase;
    if (profile == RtSceneProfile::GraphicsPreview) return {preview,preview,{},false};
    if (profile == RtSceneProfile::EntryMenu) return {entry,entry,{},false};
    if (rescueJourney) return {kChapterBaseColorSourceLayers,kChapterNormalOrmSourceLayers,
                               kChapterTextureAliases,true};
    if (productionAssets) return {legacyBase,legacyNormalOrm,{},false};
    return {};
}
}
