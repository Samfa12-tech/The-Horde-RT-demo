#pragma once

#include <array>
#include <algorithm>

namespace horde::scene
{

// Neutral authored geometry shared by RT construction and held-item clearance.
// Coordinates are world X/Z; the four corners preserve authored roof winding.
struct OverheadVolume
{
    std::array<std::array<float, 2u>, 4u> footprint{};
    float bottomY = 0.0f;
    float topY = 0.0f;
    std::array<float, 2u> bottomGradientXZ{};
    std::array<float, 2u> bottomAnchorXZ{};
};

// Conservative minimum of an authored underside plane over the horizontal
// held-item disk intersecting the footprint's bounds. Convex irregular roofs
// may conservatively include corners outside their footprint, never miss them.
constexpr float MinimumOverheadBottomY(const OverheadVolume& volume,
                                      float x, float z, float radius)
{
    float minX = volume.footprint[0][0], maxX = minX;
    float minZ = volume.footprint[0][1], maxZ = minZ;
    for (const auto& corner : volume.footprint)
    {
        minX = std::min(minX, corner[0]); maxX = std::max(maxX, corner[0]);
        minZ = std::min(minZ, corner[1]); maxZ = std::max(maxZ, corner[1]);
    }
    const float planeX = std::clamp(x + (volume.bottomGradientXZ[0] < 0.0f ? radius : -radius), minX, maxX);
    const float planeZ = std::clamp(z + (volume.bottomGradientXZ[1] < 0.0f ? radius : -radius), minZ, maxZ);
    return volume.bottomY +
        volume.bottomGradientXZ[0] * (planeX - volume.bottomAnchorXZ[0]) +
        volume.bottomGradientXZ[1] * (planeZ - volume.bottomAnchorXZ[1]);
}

constexpr OverheadVolume RectangularOverhead(float minX, float minZ,
                                            float maxX, float maxZ,
                                            float bottomY, float topY)
{
    return {{{{{minX, maxZ}}, {{minX, minZ}},
               {{maxX, minZ}}, {{maxX, maxZ}}}}, bottomY, topY};
}

inline constexpr float kShowcaseRouteCeilingWorldY = 1.35f;
// Close the vertical step between the native flat roof and the imported rising
// roof. A real solid joins both shells; no visual/light/shadow-only closure.
inline constexpr OverheadVolume kShowcaseCollapseRoofSeam =
    RectangularOverhead(-2.0f, 2.92f, 2.0f, 2.94f, 1.35f, 1.80f);

// The thin exit cap includes the real front wall and secondary hidden shell.
// Side uprights remain ordinary world/collision geometry. These are the six
// overhead solids, not six bespoke prop responses.
inline constexpr std::array<OverheadVolume, 6u> kShowcaseLowOverheadVolumes{{
    RectangularOverhead(-1.20f, -3.55f, 1.20f, -3.25f, 0.78f, 1.18f),
    RectangularOverhead(-0.90f, -6.47f, 0.90f, -6.40f, 0.85f, 1.42f),
    RectangularOverhead(-1.08f, -6.52f, -0.48f, -6.28f, 0.82f, 1.12f),
    RectangularOverhead(0.48f, -6.52f, 1.08f, -6.28f, 0.82f, 1.12f),
    RectangularOverhead(-0.48f, -6.52f, 0.48f, -6.28f, 0.96f, 1.20f),
    RectangularOverhead(-29.62f, -16.80f, -29.38f, -13.60f, 0.88f, 1.20f),
}};

// Exact static roof footprints, including the original irregular moon breach,
// drench slot and later open shafts. The moving finale seal is independently
// owned after the original torch's authored drench/drop; it is not a static roof.
inline constexpr std::array<OverheadVolume, 22u> kShowcaseCeilingPatches{{
    RectangularOverhead(-1.85f, -0.20f, 1.85f, 2.94f, 1.35f, 1.35f),
    {{{{{-1.85f, -0.20f}}, {{-1.85f, -6.40f}}, {{-0.72f, -5.20f}}, {{-0.55f, -3.45f}}}}, 1.35f, 1.35f},
    {{{{{0.32f, -3.55f}}, {{0.62f, -5.05f}}, {{1.85f, -6.40f}}, {{1.85f, -0.20f}}}}, 1.35f, 1.35f},
    {{{{{-1.85f, -0.20f}}, {{-0.55f, -3.45f}}, {{0.32f, -3.55f}}, {{1.85f, -0.20f}}}}, 1.35f, 1.35f},
    {{{{{-0.72f, -5.20f}}, {{-1.85f, -6.40f}}, {{1.85f, -6.40f}}, {{0.62f, -5.05f}}}}, 1.35f, 1.35f},
    RectangularOverhead(-1.20f, -10.0f, 1.20f, -6.40f, 1.35f, 1.35f),
    RectangularOverhead(0.00f, -11.2f, 4.80f, -8.80f, 1.35f, 1.35f),
    RectangularOverhead(3.60f, -15.2f, 6.00f, -10.0f, 1.35f, 1.35f),
    RectangularOverhead(-2.50f, -16.4f, -2.90f, -14.0f, 1.35f, 1.35f),
    RectangularOverhead(-1.58f, -16.4f, 4.80f, -14.0f, 1.35f, 1.35f),
    RectangularOverhead(-2.90f, -16.4f, -1.58f, -16.1f, 1.35f, 1.35f),
    RectangularOverhead(-2.90f, -14.72f, -1.58f, -14.0f, 1.35f, 1.35f),
    RectangularOverhead(-8.50f, -18.0f, -6.70f, -12.4f, 1.35f, 1.35f),
    RectangularOverhead(-4.30f, -18.0f, -2.50f, -12.4f, 1.35f, 1.35f),
    RectangularOverhead(-6.70f, -13.8f, -4.30f, -12.4f, 1.35f, 1.35f),
    RectangularOverhead(-6.70f, -18.0f, -4.30f, -16.6f, 1.35f, 1.35f),
    RectangularOverhead(-28.5f, -16.8f, -8.50f, -13.6f, 1.35f, 1.35f),
    RectangularOverhead(-30.5f, -16.8f, -28.5f, -13.6f, 1.35f, 1.35f),
    RectangularOverhead(-36.9f, -18.4f, -34.9f, -12.0f, 1.35f, 1.35f),
    RectangularOverhead(-32.5f, -18.4f, -30.5f, -12.0f, 1.35f, 1.35f),
    RectangularOverhead(-34.9f, -18.4f, -32.5f, -16.6f, 1.35f, 1.35f),
    RectangularOverhead(-34.9f, -13.8f, -32.5f, -12.0f, 1.35f, 1.35f),
}};

// Imported immutable roof: these planes describe the accepted closed wedges
// and feed shared clearance only. Their RT triangles come from the static PBR
// asset; drawing additional procedural roof cards would duplicate the asset.
inline constexpr std::array<OverheadVolume, 8u> kShowcaseImportedOverheadVolumes{{
    {{{{{-2.0f, 4.78f}}, {{-2.0f, 2.92f}}, {{2.0f, 2.92f}}, {{2.0f, 4.78f}}}},
     1.60f, 1.98f, {{0.0f, 0.20f / 1.86f}}, {{0.0f, 2.92f}}},
    {{{{{-2.0f, 17.48f}}, {{-2.0f, 4.78f}}, {{2.0f, 4.78f}}, {{2.0f, 17.48f}}}},
     1.80f, 12.24f, {{0.0f, 10.24f / 12.70f}}, {{0.0f, 4.78f}}},
    // Actual post-CSG shoulder undersides, not their wider uncut recipe bounds.
    {{{{{-2.0f, 2.92000008f}}, {{-1.96617031f, 2.92000008f}},
         {{-1.62085652f, 4.52402306f}}, {{-2.0f, 4.55999994f}}}},
     1.32f, 1.98f, {{-0.28f / 1.37f, 0.0f}}, {{-2.0f, 2.92f}}},
    {{{{{2.0f, 2.92000008f}}, {{1.59801006f, 2.92000008f}},
         {{2.0f, 4.37036991f}}, {{2.0f, 4.37036991f}}}},
     1.32f, 1.98f, {{0.28f / 1.30f, 0.0f}}, {{2.0f, 2.92f}}},
    RectangularOverhead(-2.0f, 2.92f, -1.85f, 3.68f, 1.35f, 1.98f),
    RectangularOverhead(1.85f, 2.92f, 2.0f, 3.68f, 1.35f, 1.98f),
    // Conservative minima of the accepted cut arch's real downward masonry
    // faces. Floor-base faces are excluded. These bounded jamb constraints
    // keep the shared hand/torch/fire/light pose below the fractured underside.
    RectangularOverhead(-1.60752666f, 3.40000010f, -0.85999995f, 3.65999985f,
                        0.87432492f, 1.42000008f),
    RectangularOverhead(0.80000001f, 3.40000010f, 1.57648420f, 3.65999985f,
                        1.14999998f, 1.42000008f),
}};

} // namespace horde::scene
