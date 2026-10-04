#pragma once

#include <algorithm>
#include "gameplay/ShowcaseRoute.h"
#include "gameplay/items/HeldItemState.h"

#include <array>
#include <cstdint>
#include <vector>

namespace horde::graphics
{
using PreviewPoint = std::array<float, 3u>;
enum class GraphicsPreviewCamera : std::uint8_t { Overview, Materials, Glass, Water, Skeleton, Mirror };
struct PreviewCameraPose { float x, z, yaw, pitch; };
struct PreviewQuad
{
    std::array<PreviewPoint, 4u> vertices;
    std::uint32_t material = 0u;
    std::uint32_t normal = 0u;
    std::uint32_t SurfaceCode() const noexcept { return material | (normal << 8u); }
};
struct PreviewWaterStream
{
    float centreZ; // Local to waterOrigin; current shader world coordinates retained.
    float radiusX;
    float radiusZ;
};
struct PreviewSkeletonPose { float x, z, facing, animationTime; };
struct GraphicsPreviewDescription
{
    std::vector<PreviewQuad> worldQuads;
    std::vector<PreviewQuad> waterQuads; // World-space horizontal film, separate water role.
    PreviewPoint waterOrigin{-2.32f, 0.0f, -15.26f};
    std::array<float, 6u> waterRingHeights{2.16f, 1.58f, 1.01f, 0.43f, -0.18f, -0.91f};
    std::array<PreviewWaterStream, 3u> waterStreams{{{0.0f, 0.006f, 0.065f}, {-0.18f, 0.003f, 0.014f}, {0.20f, 0.003f, 0.012f}}};
    PreviewPoint torchPosition{-3.65f, 0.20f, -15.95f};
    PreviewPoint lanternPosition{-2.95f, -0.20f, -16.30f};
    PreviewPoint panePosition{-3.20f, 0.0f, -14.75f};
    // Actual shared static-PBR wood, admitted from existing licensed material;
    // world material codes do not acquire a preview-only wood shader branch.
    PreviewPoint woodPlinthMinimum{-3.45f, horde::gameplay::kRouteFloorWorldY, -16.80f};
    PreviewPoint woodPlinthMaximum{-2.55f, -0.45f, -16.00f};
    PreviewSkeletonPose skeleton{-4.10f, -16.10f, 0.0f, 0.0f};
};

inline PreviewCameraPose GraphicsPreviewCameraPose(const GraphicsPreviewCamera camera) noexcept
{
    // Production camera convention: +yaw looks toward +X, yaw=0 toward -Z.
    switch (camera)
    {
    case GraphicsPreviewCamera::Materials: return {-4.85f, -14.15f, -0.40f, -0.22f};
    case GraphicsPreviewCamera::Glass: return {-3.85f, -13.30f, 0.40f, -0.22f};
    // View the complete falling streams and floor film from the open east side;
    // the close glass-side pose placed the pool below the production frustum.
    case GraphicsPreviewCamera::Water: return {-0.65f, -12.60f, -0.56f, 0.01f};
    case GraphicsPreviewCamera::Skeleton: return {-4.30f, -13.80f, 0.08f, -0.08f};
    case GraphicsPreviewCamera::Mirror: return {-3.45f, -13.10f, -0.70f, -0.08f};
    default: return {-4.75f, -12.85f, 0.38f, -0.10f};
    }
}

inline GraphicsPreviewDescription MakeGraphicsPreviewDescription()
{
    GraphicsPreviewDescription result;
    constexpr float floor = horde::gameplay::kRouteFloorWorldY;
    // Material/normal codes are the established world-surface ABI: stone0,
    // wet1, moss2, ground3, metal4, mirror8, water10; up0/down1/+X2/-X3/+Z4/-Z5.
    const auto quad = [&result](std::array<PreviewPoint, 4u> points, const std::uint32_t material, const std::uint32_t normal) {
        result.worldQuads.push_back({points, material, normal});
    };
    quad({{{-5.3f,floor,-12.4f},{-0.4f,floor,-12.4f},{-0.4f,floor,-17.2f},{-5.3f,floor,-17.2f}}}, 0u, 0u);
    quad({{{-5.3f,2.4f,-17.2f},{-0.4f,2.4f,-17.2f},{-0.4f,2.4f,-12.4f},{-5.3f,2.4f,-12.4f}}}, 0u, 1u);
    quad({{{-5.3f,floor,-17.2f},{-0.4f,floor,-17.2f},{-0.4f,2.4f,-17.2f},{-5.3f,2.4f,-17.2f}}}, 0u, 4u);
    quad({{{-5.3f,floor,-12.4f},{-5.3f,floor,-17.2f},{-5.3f,2.4f,-17.2f},{-5.3f,2.4f,-12.4f}}}, 2u, 2u);
    quad({{{-0.4f,floor,-17.2f},{-0.4f,floor,-12.4f},{-0.4f,2.4f,-12.4f},{-0.4f,2.4f,-17.2f}}}, 0u, 3u);
    quad({{{-0.4f,floor,-12.4f},{-5.3f,floor,-12.4f},{-5.3f,2.4f,-12.4f},{-0.4f,2.4f,-12.4f}}}, 0u, 5u);
    // Angled material samples and mirror use actual native RT surfaces.
    quad({{{-5.28f,-0.6f,-15.8f},{-5.28f,-0.6f,-14.7f},{-5.28f,0.65f,-14.7f},{-5.28f,0.65f,-15.8f}}}, 8u, 2u);
    quad({{{-4.85f,floor+0.01f,-14.65f},{-4.10f,floor+0.01f,-14.65f},{-4.10f,floor+0.01f,-15.25f},{-4.85f,floor+0.01f,-15.25f}}}, 3u, 0u);
    quad({{{-4.95f,-0.65f,-17.18f},{-4.35f,-0.65f,-17.18f},{-4.35f,0.55f,-17.18f},{-4.95f,0.55f,-17.18f}}}, 4u, 4u);
    quad({{{-2.62f,floor+0.006f,-15.75f},{-2.02f,floor+0.006f,-15.75f},{-2.02f,floor+0.006f,-14.90f},{-2.62f,floor+0.006f,-14.90f}}}, 1u, 0u);
    result.waterQuads.push_back({{{{-2.60f,floor+0.012f,-15.65f},{-2.04f,floor+0.012f,-15.65f},{-2.04f,floor+0.012f,-14.96f},{-2.60f,floor+0.012f,-14.96f}}},10u,0u});
    return result;
}
} // namespace horde::graphics
