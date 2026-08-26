#pragma once

#include "gameplay/items/HeldItemState.h"

#include <string>

namespace horde::gameplay::items
{

struct HeldLightState
{
    HeldItemTransform worldFromFlame{};
    HeldItemTransform worldFromLight{};
    float flameStrength = 0.0f;
    bool active = false;
};

bool ComposeHeldLightState(const HeldItemTransform& worldFromItem,
                           const HeldItemTransform& itemFromFlame,
                           const HeldItemTransform& itemFromLight,
                           float flameStrength,
                           HeldLightState& state,
                           std::string& diagnostic);

} // namespace horde::gameplay::items
