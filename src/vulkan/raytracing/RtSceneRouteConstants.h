#pragma once

#include "gameplay/items/HeldItemState.h"

namespace horde::vulkan::raytracing
{

// Single world-space floor authority for the authored corridor route and any
// deterministic props staged on it.
inline constexpr float kRouteFloorWorldY = -0.95f;

inline constexpr horde::gameplay::items::HeldItemTransform
    kProductionRewardChestStageWorldFromBase{{
        0.50f, 0.0f, -0.8660254f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.8660254f, 0.0f, 0.50f, 0.0f,
        -12.50f, kRouteFloorWorldY, -15.42f, 1.0f}};

static_assert(kProductionRewardChestStageWorldFromBase[13] == kRouteFloorWorldY,
              "production reward chest staging must remain floor-authored");

} // namespace horde::vulkan::raytracing
