#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace horde::gameplay
{

struct DevelopmentCheckpoint
{
    std::int32_t id;
    std::string_view name;
    std::int32_t baseShowcaseCheckpointId;
    float cameraX;
    float cameraZ;
    float yaw;
    float pitch;
};

inline constexpr std::array<DevelopmentCheckpoint, 2u> kDevelopmentCheckpoints{{
    {100, "pbr-sword-closeup", 0, 0.0f, 1.85f, 0.0f, -0.18f},
    {101, "pbr-torch-fire", 0, 0.0f, 1.85f, 0.0f, -0.14f},
}};

constexpr const DevelopmentCheckpoint* FindDevelopmentCheckpoint(std::string_view name)
{
    for (const DevelopmentCheckpoint& checkpoint : kDevelopmentCheckpoints)
    {
        if (checkpoint.name == name) return &checkpoint;
    }
    return nullptr;
}

} // namespace horde::gameplay
