#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace horde::gameplay
{

enum class DevelopmentCombatPose : std::uint8_t
{
    Rest,
    DownwardCutActive,
    UpwardSliceActive,
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
};

inline constexpr std::array<DevelopmentCheckpoint, 14u> kDevelopmentCheckpoints{{
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
