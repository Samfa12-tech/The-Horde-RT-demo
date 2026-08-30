#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "gameplay/ShowcaseRoute.h"

namespace horde::gameplay
{

enum class DevelopmentCombatPose : std::uint8_t
{
    Rest,
    DownwardCutActive,
    UpwardSliceActive,
};

enum class DevelopmentRewardPose : std::uint8_t
{
    None,
    HeldHigh,
    HeldLow,
    GlassTransmission,
    MotionExtreme,
};

struct DevelopmentCheckpoint
{
    std::int32_t id;
    std::string_view name;
    std::int32_t baseShowcaseCheckpointId;
    float cameraX;
    float cameraZ;
    float yaw;
    float pitch;
    DevelopmentCombatPose combatPose = DevelopmentCombatPose::Rest;
    bool usesGlassFixture = false;
    float glassDepthScale = 1.0f;
    std::array<float, 3u> glassAttenuationColor{{0.72f, 0.90f, 1.0f}};
    float glassAttenuationDistance = 2.4f;
    bool usesProductionRewardProps = false;
    bool productionLanternGlassOnly = false;
    DevelopmentRewardPose rewardPose = DevelopmentRewardPose::None;
    float rewardForwardAngleRadians = 0.0f;
    float rewardStrafeAngleRadians = 0.0f;
    float rewardForwardAngularVelocity = 0.0f;
    float rewardStrafeAngularVelocity = 0.0f;
    std::array<float, 3u> rewardPreviousPivotVelocity{{0.0f, 0.0f, 0.0f}};
    float rewardTorsionAngleRadians = 0.0f;
    float rewardTorsionAngularVelocity = 0.0f;
};

inline constexpr std::array<DevelopmentCheckpoint, 34u> kDevelopmentCheckpoints{{
    {100, "pbr-sword-closeup", 0, 0.0f, 1.85f, 0.0f, -0.18f},
    {101, "pbr-torch-fire", 0, 0.0f, 1.85f, 0.0f, -0.14f},
    {102, "player-body-grips", 0, 0.0f, 1.85f, 0.0f, -0.32f},
    {103, "player-body-forward", 0, 0.0f, 1.85f, 0.0f, -0.05f},
    {104, "player-fallback-forward", 0, 0.0f, 1.85f, 0.0f, -0.05f},
    {105, "player-fallback-grips", 0, 0.0f, 1.85f, 0.0f, -0.32f},
    {106, "player-body-owner-feedback", 0, 0.0f, 1.85f, 0.0f, -0.28f},
    {107, "player-body-downward-cut", 0, 0.0f, 1.85f, 0.0f, -0.28f,
     DevelopmentCombatPose::DownwardCutActive},
    {108, "player-body-upward-slice", 0, 0.0f, 1.85f, 0.0f, -0.28f,
     DevelopmentCombatPose::UpwardSliceActive},
    {109, "glass-transport", 4, -7.60f, -15.20f, -1.57079632679f, -0.05f,
     DevelopmentCombatPose::Rest, true},
    {110, "glass-fire-transport", 0, -7.60f, -15.20f, -1.57079632679f, -0.05f,
     DevelopmentCombatPose::Rest, true},
    {111, "glass-tinted-transport", 0, -7.60f, -15.20f, -1.57079632679f, -0.05f,
     DevelopmentCombatPose::Rest, true, 1.0f, {{0.12f, 0.82f, 0.28f}}, 0.30f},
    {112, "glass-millimetre-closed", 4, -7.60f, -15.20f, -1.57079632679f, -0.05f,
     DevelopmentCombatPose::Rest, true, 0.005f},
    {113, "glass-edge-fresnel", 4, -7.60f, -13.25f, -0.65f, -0.05f,
     DevelopmentCombatPose::Rest, true},
    {114, "lantern-chest-unlock", 5,
     kRewardChestRoutePosition.x + 1.85f, kRewardChestRoutePosition.z,
     -1.57079632679f, -0.35f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false},
    {115, "lantern-glass-production", 5,
     kRewardChestRoutePosition.x + 1.85f, kRewardChestRoutePosition.z,
     -1.57079632679f, -0.35f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, true},
    {116, "lantern-held-high", 5, -10.65f, -15.20f, -1.57079632679f, -0.30f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldHigh},
    {117, "lantern-held-low", 5, -10.65f, -15.20f, -1.57079632679f, -0.30f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldLow},
    {118, "lantern-glass-transmission", 5, -10.65f, -15.20f, -1.57079632679f, -0.30f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::GlassTransmission, 0.30f, -0.16f,
     0.0f, 0.0f, {{0.0f, 0.0f, 0.0f}}, 0.12f, -0.40f},
    {119, "lantern-motion-extreme", 5, -10.65f, -15.20f, -1.57079632679f, -0.30f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::MotionExtreme, 0.82f, -0.42f,
     2.40f, -1.60f, {{3.8f, 0.25f, -2.2f}}, 0.28f, -0.90f},
    {120, "lantern-sweep-high-forward", 5, -10.65f, -15.20f, -1.57079632679f, -0.30f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldHigh, 0.78539816339f, 0.0f},
    {121, "lantern-sweep-high-backward", 5, -10.65f, -15.20f, -1.57079632679f, -0.30f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldHigh, -0.78539816339f, 0.0f},
    {122, "lantern-sweep-high-left", 5, -10.65f, -15.20f, -1.57079632679f, -0.30f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldHigh, 0.0f, 0.78539816339f},
    {123, "lantern-sweep-high-right", 5, -10.65f, -15.20f, -1.57079632679f, -0.30f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldHigh, 0.0f, -0.78539816339f},
    {124, "lantern-sweep-high-diagonal", 5, -10.65f, -15.20f, -1.57079632679f, -0.30f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldHigh, 0.678f, 0.678f,
     0.0f, 0.0f, {{0.0f, 0.0f, 0.0f}}, 0.34906585040f, 0.0f},
    {125, "lantern-sweep-high-opposite", 5, -10.65f, -15.20f, -1.57079632679f, -0.30f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldHigh, -0.678f, -0.678f,
     0.0f, 0.0f, {{0.0f, 0.0f, 0.0f}}, -0.34906585040f, 0.0f},
    {126, "lantern-sweep-low-forward", 5, -10.65f, -15.20f, -1.57079632679f, -0.30f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldLow, 0.78539816339f, 0.0f},
    {127, "lantern-sweep-low-backward", 5, -10.65f, -15.20f, -1.57079632679f, -0.30f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldLow, -0.78539816339f, 0.0f},
    {128, "lantern-sweep-low-left", 5, -10.65f, -15.20f, -1.57079632679f, -0.30f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldLow, 0.0f, 0.78539816339f},
    {129, "lantern-sweep-low-right", 5, -10.65f, -15.20f, -1.57079632679f, -0.30f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldLow, 0.0f, -0.78539816339f},
    {130, "lantern-sweep-high-alt-camera", 5, -10.78f, -15.35f, -1.30f, -0.34f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldHigh, 0.45f, 0.45f,
     0.0f, 0.0f, {{0.0f, 0.0f, 0.0f}}, 0.20f, 0.60f},
    {131, "lantern-sweep-low-alt-camera", 5, -10.30f, -15.00f, -2.05f, -0.14f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldLow, -0.45f, 0.45f,
     0.0f, 0.0f, {{0.0f, 0.0f, 0.0f}}, -0.20f, -0.60f},
    {132, "lantern-wall-high", 2, 0.0f, -9.70f, 0.0f, -0.08f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldHigh},
    {133, "lantern-wall-low", 2, 0.0f, -9.70f, 0.0f, -0.08f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldLow, 0.0f, 0.52359877560f,
     0.0f, 0.0f, {{0.0f, 0.0f, 0.0f}}, 0.34906585040f, 0.0f},
}};

constexpr const DevelopmentCheckpoint* FindDevelopmentCheckpoint(std::string_view name)
{
    for (const DevelopmentCheckpoint& checkpoint : kDevelopmentCheckpoints)
    {
        if (checkpoint.name == name) return &checkpoint;
    }
    return nullptr;
}

constexpr const DevelopmentCheckpoint* FindDevelopmentCheckpoint(std::int32_t id)
{
    for (const DevelopmentCheckpoint& checkpoint : kDevelopmentCheckpoints)
    {
        if (checkpoint.id == id) return &checkpoint;
    }
    return nullptr;
}

} // namespace horde::gameplay
