#pragma once

#include "scene/assets/StaticMeshAsset.h"
#include "vulkan/raytracing/RtPipelineVariants.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace horde::vulkan::raytracing
{

// Apply once to the loaded canonical lantern, before static-slot registration
// and BLAS construction. Mobile has open apertures, not simulated transparency:
// pane primitives are absent from every ray's acceleration structure. High is
// an exact no-op. Retain source vertex/index/material/socket data and offsets so
// the remaining cage/flame geometry, texture routing and grips are unchanged.
[[nodiscard]] inline bool SelectLanternGeometryForQuality(
    horde::scene::assets::StaticMeshAsset& asset,
    DielectricQuality quality,
    std::string& diagnostic)
{
    diagnostic.clear();
    if (quality == DielectricQuality::High) return true;
    if (quality != DielectricQuality::Mobile)
    {
        diagnostic = "Lantern geometry requires a valid Mobile or High quality profile.";
        return false;
    }

    const auto glass = std::find_if(asset.materials.begin(), asset.materials.end(),
        [](const auto& material) { return material.name == "LanternGlass"; });
    if (glass == asset.materials.end() ||
        std::count_if(asset.materials.begin(), asset.materials.end(),
            [](const auto& material) { return material.name == "LanternGlass"; }) != 1 ||
        !std::isfinite(glass->transmissionFactor) || glass->transmissionFactor <= 0.0f ||
        (glass->flags & static_cast<std::uint32_t>(RtMaterialFlag::Transmission)) == 0u)
    {
        diagnostic = "Mobile lantern requires one named physical LanternGlass material before pane selection.";
        return false;
    }
    const auto glassIndex = static_cast<std::size_t>(glass - asset.materials.begin());
    std::size_t paneCount = 0u;
    for (const auto& primitive : asset.primitives)
    {
        if (primitive.materialIndex >= asset.materials.size())
        {
            diagnostic = "Mobile lantern pane selection rejected an invalid material reference.";
            return false;
        }
        if (primitive.materialIndex == glassIndex) ++paneCount;
    }
    if (paneCount == 0u || paneCount == asset.primitives.size())
    {
        diagnostic = "Mobile lantern pane selection requires both pane and retained body geometry.";
        return false;
    }
    // Stable compaction keeps geometryIndexEXT and static primitive metadata in
    // agreement. Do not mask the entire lantern instance or alter optical data.
    std::erase_if(asset.primitives,
        [glassIndex](const auto& primitive) { return primitive.materialIndex == glassIndex; });
    return true;
}

} // namespace horde::vulkan::raytracing
