#pragma once

#include "gameplay/SwordCombat.h"
#include "gameplay/items/HeldItemState.h"

namespace horde::platform::windows
{

// Only the actual presented snapshot contributes a milestone. In particular,
// completed draw progress must not stand in for an unobserved intermediate pose.
inline unsigned NativeEquipmentCaptureMilestones(
    gameplay::items::HeldItemTransitionKind transition, bool transitionActive,
    float progress, gameplay::PlayerCombatAction action) noexcept
{
    unsigned mask = 0u;
    if (transitionActive && transition == gameplay::items::HeldItemTransitionKind::Draw)
        for (unsigned threshold = 0u; threshold < 3u; ++threshold)
            if (progress >= 0.25f * static_cast<float>(threshold + 1u))
                mask |= 1u << threshold;
    switch (action)
    {
    case gameplay::PlayerCombatAction::SwingWindup: mask |= 1u << 3u; break;
    case gameplay::PlayerCombatAction::SwingActive: mask |= 1u << 4u; break;
    case gameplay::PlayerCombatAction::UpwardSliceWindup: mask |= 1u << 5u; break;
    case gameplay::PlayerCombatAction::UpwardSliceActive: mask |= 1u << 6u; break;
    case gameplay::PlayerCombatAction::ParryActive: mask |= 1u << 7u; break;
    default: break;
    }
    return mask;
}

inline bool NativeMotionNeedsCapture(bool stageChanged, double secondsSinceCapture,
    unsigned torchMilestones, unsigned capturedTorchMilestones,
    unsigned equipmentMilestones, unsigned capturedEquipmentMilestones) noexcept
{
    return stageChanged || secondsSinceCapture >= 2.0 ||
        (torchMilestones & ~capturedTorchMilestones) != 0u ||
        (equipmentMilestones & ~capturedEquipmentMilestones) != 0u;
}

} // namespace horde::platform::windows
