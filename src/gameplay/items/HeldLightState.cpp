#include "gameplay/items/HeldLightState.h"

#include "gameplay/items/HeldItemKinematics.h"

#include <algorithm>
#include <cmath>

namespace horde::gameplay::items
{

bool ComposeHeldLightState(const HeldItemTransform& worldFromItem,
                           const HeldItemTransform& itemFromFlame,
                           const HeldItemTransform& itemFromLight,
                           const float flameStrength,
                           HeldLightState& state,
                           std::string& diagnostic)
{
    if (!ValidateHeldItemSocketTransform(worldFromItem, diagnostic) ||
        !ValidateHeldItemSocketTransform(itemFromFlame, diagnostic) ||
        !ValidateHeldItemSocketTransform(itemFromLight, diagnostic))
    {
        return false;
    }
    state.worldFromFlame = MultiplyHeldItemTransforms(worldFromItem, itemFromFlame);
    state.worldFromLight = MultiplyHeldItemTransforms(worldFromItem, itemFromLight);
    state.flameStrength = std::isfinite(flameStrength) ? std::clamp(flameStrength, 0.0f, 1.0f) : 0.0f;
    state.active = state.flameStrength > 0.0f;
    diagnostic.clear();
    return true;
}

} // namespace horde::gameplay::items
