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
    ParryActive,
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
    bool stagesUnlockedChest = false;
    // Capture framing only; dedicated modelled primary ownership stays mandatory.
    bool primaryArmsMayBeOutsideFrame = false;
    // Explicit development capture only; production encounter defaults stay unchanged.
    bool stagesWaterfallGuards = false;
    std::uint32_t waterfallGuardFixedTicks = 0u;
};

inline constexpr std::array<DevelopmentCheckpoint, 56u> kDevelopmentCheckpoints{{
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
    {.id = 114,
     .name = "lantern-chest-unlock",
     .baseShowcaseCheckpointId = 5,
     .cameraX = kRewardChestRoutePosition.x + 1.85f,
     .cameraZ = kRewardChestRoutePosition.z,
     .yaw = -1.57079632679f,
     .pitch = -0.35f,
     .usesProductionRewardProps = true,
     .stagesUnlockedChest = true},
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
    {134, "lantern-held-look-up", 5, -10.65f, -15.20f,
     -1.57079632679f, 0.28f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldHigh},
    {135, "lantern-chest-held-high", 10,
     kRewardChestRoutePosition.x + 1.30f, kRewardChestRoutePosition.z,
     -1.57079632679f, -0.05f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldHigh},
    {136, "player-viewmodel-grips", 0, 0.0f, 1.85f, 0.0f, -0.32f},
    {137, "player-viewmodel-forward", 0, 0.0f, 1.85f, 0.0f, -0.05f},
    {138, "player-viewmodel-downward-cut", 0, 0.0f, 1.85f, 0.0f, -0.28f,
     DevelopmentCombatPose::DownwardCutActive},
    {139, "player-viewmodel-upward-slice", 0, 0.0f, 1.85f, 0.0f, -0.28f,
     DevelopmentCombatPose::UpwardSliceActive},
    {140, "player-viewmodel-look-up", 0, 0.0f, 1.85f, 0.0f, 0.28f},
    {141, "player-viewmodel-look-down", 0, 0.0f, 1.85f, 0.0f, -0.32f},
    {142, "player-viewmodel-lantern-high", 5, -10.65f, -15.20f, -1.57079632679f, -0.30f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldHigh},
    {143, "player-viewmodel-lantern-low", 5, -10.65f, -15.20f, -1.57079632679f, -0.30f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldLow},
    {144, "player-viewmodel-lantern-low-parry", 5, -10.65f, -15.20f,
     -1.57079632679f, -0.30f,
     DevelopmentCombatPose::ParryActive, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldLow},
    {145, "player-viewmodel-lantern-low-look-down", 5, -10.65f, -15.20f,
     -1.57079632679f, -0.32f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldLow},
    {146, "player-viewmodel-lantern-high-look-up", 5, -10.65f, -15.20f,
     -1.57079632679f, 0.28f,
     DevelopmentCombatPose::Rest, false, 1.0f, {{0.72f, 0.90f, 1.0f}}, 2.4f,
     true, false, DevelopmentRewardPose::HeldHigh},
    // Source-anchored layout identity views; fresh checkpoint two keeps the
    // ordinary held torch/player state for C/D/A/B without fixture overrides.
    // A farther stand-off reduces ordinary wall-clearance retraction in this
    // level identity view; native arm ownership/visibility stays capture-gated.
    {147, "layout-c-wall-panel", 2, 2.55f, -10.60f, 3.14159265359f, 0.0f},
    {148, "layout-d-entry-breach", 2, 0.0f, -2.55f, 0.0f, 0.28f},
    // The legal upward view shows A's lower aperture and deep inner wall;
    // its 5.6 m rim is beyond this camera's accepted pitch/FOV, unlike B.
    {149, "layout-a-waterfall-own-hole", 2, -1.15f, -15.20f, -1.57079632679f, 0.28f},
    {150, "layout-b-large-skylight", 2, -6.50f, -15.20f, 1.57079632679f, 0.28f},
    {151, "layout-e-finale-opening", 11, -35.30f, -15.20f, 1.57079632679f, 0.28f},
    // C147's authored state, with a radius-safe nearer approach and maximum
    // legal upward pitch. This admits a partial upper/lintel enclosure view;
    // the 4.10 m rim remains beyond the production camera's vertical FOV.
    {.id = 152,
     .name = "layout-c-wall-panel-upward",
     .baseShowcaseCheckpointId = 2,
     .cameraX = 2.55f,
     .cameraZ = -9.45f,
     .yaw = 3.14159265359f,
     .pitch = 0.28f,
     .primaryArmsMayBeOutsideFrame = true},
    {.id = 153,
     .name = "waterfall-guards-entry",
     .baseShowcaseCheckpointId = 2,
     .cameraX = -3.00f,
     .cameraZ = -15.20f,
     .yaw = -1.57079632679f,
     .pitch = -0.06f,
     .stagesWaterfallGuards = true},
    {.id = 154,
     .name = "waterfall-guards-walk-early",
     .baseShowcaseCheckpointId = 2,
     .cameraX = -3.45f,
     .cameraZ = -15.20f,
     .yaw = -1.57079632679f,
     .pitch = -0.06f,
     .stagesWaterfallGuards = true,
     .waterfallGuardFixedTicks = 6u},
    {.id = 155,
     .name = "waterfall-guards-walk-later",
     .baseShowcaseCheckpointId = 2,
     .cameraX = -3.45f,
     .cameraZ = -15.20f,
     .yaw = -1.57079632679f,
     .pitch = -0.06f,
     .stagesWaterfallGuards = true,
     .waterfallGuardFixedTicks = 18u},
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
