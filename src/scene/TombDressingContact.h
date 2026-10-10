#pragma once
#include "gameplay/ShowcaseRoute.h"
#include <algorithm>
#include <array>
#include <cmath>
namespace horde::scene {
// Fitted to measured native world bounds of the two floor vessels. Niche
// returns remain outside the legacy wall capsule; the lid fits its old bier.
inline constexpr std::array<horde::gameplay::RouteRect, 2> kTombDressingFloorSolids{{
    {-36.641f, -36.259f, -18.144f, -17.763f},
    {-31.133f, -30.661f, -18.188f, -17.715f}
}};
inline bool TombDressingMovementClear(float oldX, float oldZ, float x, float z) {
    if (!std::isfinite(oldX) || !std::isfinite(oldZ) || !std::isfinite(x) || !std::isfinite(z)) return false;
    const float distance = std::hypot(x-oldX,z-oldZ);
    const int steps = std::max(1, static_cast<int>(std::ceil(distance/.10f)));
    for (int i=1; i<=steps; ++i) {
        const float t=static_cast<float>(i)/steps;
        const float px=oldX+(x-oldX)*t, pz=oldZ+(z-oldZ)*t;
        for (const auto& solid : kTombDressingFloorSolids) {
            constexpr float radius=horde::gameplay::kPlayerCollisionRadius;
            if (px>solid.minX-radius && px<solid.maxX+radius &&
                pz>solid.minZ-radius && pz<solid.maxZ+radius) return false;
        }
    }
    return true;
}
} // namespace horde::scene
