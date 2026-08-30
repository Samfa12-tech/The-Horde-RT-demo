#pragma once

#include "gameplay/ShowcaseRoute.h"
#include "gameplay/interactions/ChestRewardSequence.h"

#include <array>

namespace horde::vulkan::raytracing
{

struct RtGuidanceLight
{
    std::array<float, 3u> position{{0.0f, 0.0f, 0.0f}};
    float strength = 0.0f;
};

// The reward cue is renderer data derived from shared deterministic gameplay
// state. Locked and seal-breaking frames remain genuinely dark; the same
// phase transition that emits ChestUnlocked activates this world-space light.
inline constexpr RtGuidanceLight ResolveChestGuidanceLight(
    const horde::gameplay::interactions::ChestRewardSnapshot& chest)
{
    using horde::gameplay::interactions::ChestRewardPhase;
    constexpr std::array<float, 3u> kOverheadChestLampPosition{{
        horde::gameplay::kRewardChestRoutePosition.x,
        1.02f,
        horde::gameplay::kRewardChestRoutePosition.z}};
    constexpr float kOverheadChestLampStrength = 1.65f;
    return {kOverheadChestLampPosition,
            chest.phase == ChestRewardPhase::Locked
                ? 0.0f
                : kOverheadChestLampStrength};
}

} // namespace horde::vulkan::raytracing
