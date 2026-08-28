#pragma once

#include "gameplay/ShowcaseRoute.h"
#include "gameplay/items/HeldItemState.h"

namespace horde::vulkan::raytracing
{

inline constexpr horde::gameplay::items::HeldItemTransform
    kProductionRewardChestStageWorldFromBase{{
        0.50f, 0.0f, -0.8660254f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.8660254f, 0.0f, 0.50f, 0.0f,
        horde::gameplay::kRewardChestRoutePosition.x,
        horde::gameplay::kRouteFloorWorldY,
        horde::gameplay::kRewardChestRoutePosition.z,
        1.0f}};

static_assert(kProductionRewardChestStageWorldFromBase[13] == horde::gameplay::kRouteFloorWorldY,
              "production reward chest staging must remain floor-authored");

} // namespace horde::vulkan::raytracing
