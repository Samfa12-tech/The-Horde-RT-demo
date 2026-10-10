#pragma once

#include "gameplay/effects/WaterContact.h"
#include "scene/assets/StaticMeshAsset.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace horde::vulkan::raytracing
{

// Each fixed slot owns one closed, indexed octahedron. This keeps both the
// primitive topology and AS build-size input constant while contact moves.
inline constexpr std::size_t kWaterDropletVerticesPerSlot = 6u;
inline constexpr std::size_t kWaterDropletIndicesPerSlot = 24u;
inline constexpr std::size_t kWaterDropletVertexCount =
    horde::gameplay::effects::kWaterDropletCapacity * kWaterDropletVerticesPerSlot;
inline constexpr std::size_t kWaterDropletIndexCount =
    horde::gameplay::effects::kWaterDropletCapacity * kWaterDropletIndicesPerSlot;
inline constexpr float kWaterDropletMinRadiusMetres = 0.007f;
inline constexpr float kWaterDropletMaxRadiusMetres = 0.019f;
inline constexpr float kWaterDropletWorldLimitMetres = 256.0f;
inline constexpr float kWaterRippleLifetimeSeconds = 0.90f;

struct alignas(16) WaterContactRippleGpu
{
    std::array<float, 4u> value{}; // x, z, age, strength; strength zero disables contact ripples.
};
static_assert(sizeof(WaterContactRippleGpu) == 16u);

inline WaterContactRippleGpu MakeWaterContactRippleGpu(
    const horde::gameplay::effects::WaterContactSnapshot& snapshot) noexcept
{
    WaterContactRippleGpu result{};
    const horde::gameplay::effects::WaterRipple* newest = nullptr;
    for (const auto& ripple : snapshot.ripples)
    {
        if (!ripple.active || !std::isfinite(ripple.position[0]) ||
            !std::isfinite(ripple.position[2]) || !std::isfinite(ripple.age) ||
            !std::isfinite(ripple.strength) || ripple.age < 0.0f ||
            ripple.age >= kWaterRippleLifetimeSeconds ||
            ripple.strength <= 0.0f)
            continue;
        if (newest == nullptr || ripple.age < newest->age) newest = &ripple;
    }
    if (newest != nullptr)
        result.value = {{newest->position[0], newest->position[2],
                         newest->age, std::clamp(newest->strength, 0.0f, 1.0f)}};
    return result;
}

inline horde::scene::assets::StaticMeshAsset MakeWaterDropletStaticAsset()
{
    using namespace horde::scene::assets;
    StaticMeshAsset asset;
    asset.vertices.resize(kWaterDropletVertexCount);
    asset.indices.reserve(kWaterDropletIndexCount);
    constexpr std::array<std::array<float, 3u>, 6u> directions{{
        {{0.0f, 1.0f, 0.0f}}, {{0.0f, -1.0f, 0.0f}},
        {{1.0f, 0.0f, 0.0f}}, {{-1.0f, 0.0f, 0.0f}},
        {{0.0f, 0.0f, 1.0f}}, {{0.0f, 0.0f, -1.0f}},
    }};
    constexpr std::array<std::uint32_t, kWaterDropletIndicesPerSlot> faces{{
        0u, 4u, 2u, 0u, 2u, 5u, 0u, 5u, 3u, 0u, 3u, 4u,
        1u, 2u, 4u, 1u, 5u, 2u, 1u, 3u, 5u, 1u, 4u, 3u,
    }};
    for (std::size_t slot = 0u; slot < horde::gameplay::effects::kWaterDropletCapacity; ++slot)
    {
        for (std::size_t vertex = 0u; vertex < directions.size(); ++vertex)
        {
            auto& out = asset.vertices[slot * kWaterDropletVerticesPerSlot + vertex];
            out.position = {{10000.0f, 10000.0f, 10000.0f, 1.0f}};
            out.normal = {{directions[vertex][0], directions[vertex][1],
                           directions[vertex][2], 0.0f}};
            out.tangent = {{1.0f, 0.0f, 0.0f, 1.0f}};
            out.uv0 = {{0.0f, 0.0f, 0.0f, 0.0f}};
        }
        const auto base = static_cast<std::uint32_t>(slot * kWaterDropletVerticesPerSlot);
        for (const std::uint32_t index : faces) asset.indices.push_back(base + index);
    }
    StaticPrimitiveRecord primitive{};
    primitive.indexCount = static_cast<std::uint32_t>(asset.indices.size());
    primitive.materialIndex = 0u;
    asset.primitives.push_back(primitive);
    asset.nodeTransforms.push_back({"Water contact droplets", {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f}}});
    StaticMaterial material{};
    material.name = "Runtime thin water contact";
    material.baseColorFactor = {{0.68f, 0.86f, 0.92f, 1.0f}};
    material.attenuationColor = {{0.78f, 0.93f, 0.98f}};
    material.metallicFactor = 0.0f;
    material.roughnessFactor = 0.16f;
    material.transmissionFactor = 0.88f;
    material.ior = 1.333f;
    material.thicknessFactor = 0.018f;
    material.attenuationDistance = 1.0f;
    material.flags = static_cast<std::uint32_t>(
        horde::vulkan::raytracing::RtMaterialFlag::Transmission) |
        static_cast<std::uint32_t>(horde::vulkan::raytracing::RtMaterialFlag::ThinWall);
    asset.materials.push_back(material);
    asset.bounds.minimum = {{10000.0f - kWaterDropletMaxRadiusMetres,
                             10000.0f - kWaterDropletMaxRadiusMetres,
                             10000.0f - kWaterDropletMaxRadiusMetres}};
    asset.bounds.maximum = {{10000.0f + kWaterDropletMaxRadiusMetres,
                             10000.0f + kWaterDropletMaxRadiusMetres,
                             10000.0f + kWaterDropletMaxRadiusMetres}};
    return asset;
}

inline bool UpdateWaterDropletVertices(
    const horde::gameplay::effects::WaterContactSnapshot& snapshot,
    std::array<horde::scene::assets::StaticRtVertex, kWaterDropletVertexCount>& out,
    const bool visible = true) noexcept
{
    using namespace horde::gameplay::effects;
    constexpr std::array<std::array<float, 3u>, 6u> directions{{
        {{0.0f, 1.0f, 0.0f}}, {{0.0f, -1.0f, 0.0f}},
        {{1.0f, 0.0f, 0.0f}}, {{-1.0f, 0.0f, 0.0f}},
        {{0.0f, 0.0f, 1.0f}}, {{0.0f, 0.0f, -1.0f}},
    }};
    WaterPoint fallback{{10000.0f, 10000.0f, 10000.0f}};
    for (const WaterDroplet& droplet : snapshot.droplets)
    {
        if (visible && droplet.active && std::isfinite(droplet.position[0]) &&
            std::isfinite(droplet.position[1]) && std::isfinite(droplet.position[2]) &&
            std::abs(droplet.position[0]) <= kWaterDropletWorldLimitMetres &&
            std::abs(droplet.position[1]) <= kWaterDropletWorldLimitMetres &&
            std::abs(droplet.position[2]) <= kWaterDropletWorldLimitMetres)
        {
            fallback = droplet.position;
            break;
        }
    }
    for (std::size_t slot = 0u; slot < snapshot.droplets.size(); ++slot)
    {
        const WaterDroplet& droplet = snapshot.droplets[slot];
        if (visible && droplet.active &&
            (!std::isfinite(droplet.position[0]) || !std::isfinite(droplet.position[1]) ||
             !std::isfinite(droplet.position[2]) ||
             std::abs(droplet.position[0]) > kWaterDropletWorldLimitMetres ||
             std::abs(droplet.position[1]) > kWaterDropletWorldLimitMetres ||
             std::abs(droplet.position[2]) > kWaterDropletWorldLimitMetres ||
             !std::isfinite(droplet.age) ||
             !std::isfinite(droplet.lifetime) || droplet.lifetime <= 0.0f))
            return false;
        const bool active = visible && droplet.active;
        const float life = active
            ? std::clamp(1.0f - droplet.age / droplet.lifetime, 0.0f, 1.0f) : 0.0f;
        const float radius = active
            ? kWaterDropletMinRadiusMetres +
                (kWaterDropletMaxRadiusMetres - kWaterDropletMinRadiusMetres) * life
            : 0.0f;
        const WaterPoint centre = active ? droplet.position : fallback;
        for (std::size_t vertex = 0u; vertex < directions.size(); ++vertex)
        {
            auto& outVertex = out[slot * kWaterDropletVerticesPerSlot + vertex];
            for (std::size_t axis = 0u; axis < 3u; ++axis)
            {
                const float position = centre[axis] + directions[vertex][axis] * radius;
                if (!std::isfinite(position)) return false;
                outVertex.position[axis] = position;
                outVertex.normal[axis] = directions[vertex][axis];
            }
            outVertex.position[3] = 1.0f;
            outVertex.normal[3] = 0.0f;
            outVertex.tangent = {{1.0f, 0.0f, 0.0f, 1.0f}};
            outVertex.uv0 = {{0.0f, 0.0f, 0.0f, 0.0f}};
        }
    }
    return true;
}

} // namespace horde::vulkan::raytracing
