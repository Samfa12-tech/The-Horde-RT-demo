#pragma once
#include "gameplay/ShowcaseRoute.h"
#include <cstdint>

namespace horde::gameplay::simulation
{
enum class PlayerSupportId : std::uint32_t { RouteFloor = 1, ProofRamp = 2, ProofPlatform = 3, WorldRouteFirst = 100, WorldRouteStep = 120 };
enum class SupportSurface : std::uint32_t { Stone, Earth, Wood };
// Resident validation fixture only. No general stair/slope/jump policy.
inline constexpr float kProofSupportHalfWidth = 0.70f;
inline constexpr float kProofSupportBackZ = -0.20f;
inline constexpr float kProofSupportTopZ = 0.40f;
inline constexpr float kProofSupportGroundZ = 1.40f;
inline constexpr float kProofSupportHeight = 0.35f;
struct PlayerSupportResolution
{
    float worldY = kRouteFloorWorldY;
    PlayerSupportId id = PlayerSupportId::RouteFloor;
    bool grounded = true;
    SupportSurface surface = SupportSurface::Stone;
};
inline PlayerSupportResolution ResolveDevelopmentPlayerSupport(
    float x, float z, bool enabled, std::uint64_t generation, std::uint64_t expectedGeneration)
{
    if (!enabled || generation == 0 || generation != expectedGeneration ||
        !std::isfinite(x) || !std::isfinite(z) ||
        std::abs(x) > kProofSupportHalfWidth || z < kProofSupportBackZ || z >= kProofSupportGroundZ)
        return {};
    if (z <= kProofSupportTopZ)
        return {kRouteFloorWorldY + kProofSupportHeight, PlayerSupportId::ProofPlatform, true};
    const float fraction = (kProofSupportGroundZ - z) / (kProofSupportGroundZ - kProofSupportTopZ);
    return {kRouteFloorWorldY + kProofSupportHeight * fraction, PlayerSupportId::ProofRamp, true};
}
inline constexpr float PlayerHeightDelta(float supportWorldY) { return supportWorldY - kRouteFloorWorldY; }
inline constexpr float PlayerEyeWorldY(float supportWorldY) { return kShowcaseEyeWorldY + PlayerHeightDelta(supportWorldY); }
} // namespace horde::gameplay::simulation
