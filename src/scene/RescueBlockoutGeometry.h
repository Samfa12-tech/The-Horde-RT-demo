#pragma once

#include <array>
#include <cstddef>

namespace horde::scene
{

// Development-only geometry recipe for a safe cap over the existing rescue
// shaft and its upper landing. Coordinates are world metres; the existing
// lower aperture remains x[-34.9,-32.5], z[-16.6,-13.8].
struct RescueBlockoutBox
{
    std::array<float, 3u> minimum;
    std::array<float, 3u> maximum;
    unsigned materialCode; // Existing world SurfaceDryStone=0, SurfaceMossyStone=2.
};

inline constexpr std::array<RescueBlockoutBox, 12u> kRescueBlockoutBoxes{{
    // Shaft returns rise from the lower roof at y=1.30 to the safe rim at 2.05.
    {{{-35.05f, 1.30f, -16.75f}}, {{-34.88f, 2.05f, -13.65f}}, 2u},
    {{{-32.52f, 1.30f, -16.75f}}, {{-32.35f, 2.05f, -13.65f}}, 2u},
    {{{-34.88f, 1.30f, -16.75f}}, {{-32.52f, 2.05f, -16.58f}}, 2u},
    {{{-34.88f, 1.30f, -13.82f}}, {{-32.52f, 2.05f, -13.65f}}, 2u},
    // A 0.18 m coping keeps the aperture edge visible and physically closed.
    {{{-35.12f, 2.05f, -16.82f}}, {{-32.28f, 2.23f, -16.64f}}, 0u},
    {{{-35.12f, 2.05f, -13.76f}}, {{-32.28f, 2.23f, -13.58f}}, 0u},
    {{{-35.12f, 2.05f, -16.64f}}, {{-34.94f, 2.23f, -13.76f}}, 0u},
    {{{-32.46f, 2.05f, -16.64f}}, {{-32.28f, 2.23f, -13.76f}}, 0u},
    // Original rear-post frame anchors to the coping behind the rope line.
    {{{-34.45f, 2.23f, -16.78f}}, {{-34.27f, 3.72f, -16.66f}}, 0u},
    {{{-33.13f, 2.23f, -16.78f}}, {{-32.95f, 3.72f, -16.66f}}, 0u},
    {{{-34.36f, 3.62f, -16.78f}}, {{-33.04f, 3.78f, -16.66f}}, 0u},
    // The raised cantilever clears the body capsule and contains the anchor at z=-15.5.
    {{{-33.82f, 3.72f, -16.72f}}, {{-33.58f, 3.96f, -15.40f}}, 0u},
}};

// The solid lid spans the shaft opening at the rim height; the landing joins
// the north edge and contains the existing upper landing anchor (-33.7,2.05,-12.8).
inline constexpr RescueBlockoutBox kRescueBlockoutLid{
    {{-34.94f, 2.05f, -16.64f}}, {{-32.46f, 2.19f, -13.76f}}, 0u};
inline constexpr RescueBlockoutBox kRescueBlockoutLanding{
    {{-35.12f, 1.92f, -14.95f}}, {{-32.28f, 2.05f, -11.92f}}, 0u};

struct RescueBlockoutCollisionRect
{
    float minX;
    float maxX;
    float minZ;
    float maxZ;
    float topY;
};

inline constexpr RescueBlockoutCollisionRect kRescueBlockoutLandingCollision{
    -35.12f, -32.28f, -14.95f, -11.92f, 2.05f};

constexpr bool RescueLandingContains(float x, float z)
{
    const auto& r = kRescueBlockoutLandingCollision;
    return x >= r.minX && x <= r.maxX && z >= r.minZ && z <= r.maxZ;
}

} // namespace horde::scene
