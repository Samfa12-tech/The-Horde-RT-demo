#pragma once

#include "gameplay/items/HeldItemState.h"

#include <string_view>
#include <cstdint>

namespace horde::platform::windows
{

constexpr bool HasExpectedPlayerCaptureVisibility(
    const bool dedicatedPlayerOwnership,
    const bool primaryPlayerVisible,
    const bool pixelCountersAvailable,
    const std::uint32_t primaryArmPixels,
    const bool primaryArmsMayBeOutsideFrame)
{
    return dedicatedPlayerOwnership && primaryPlayerVisible &&
        (!pixelCountersAvailable || primaryArmPixels != 0u ||
         primaryArmsMayBeOutsideFrame);
}

struct ClaimedRewardCapturePixelPolicy
{
    bool requirePlayerPixels = true;
    bool requireRewardRingPixels = false;
    bool requireRewardBodyPixels = true;
    bool requireSwordPixels = false;
    bool permitsCompleteWallRetraction = false;
};

constexpr bool IsCaptureSwordFullyStowed(
    const horde::gameplay::items::HeldItemState& sword)
{
    using namespace horde::gameplay::items;
    return sword.id == HeldItemId::Sword && sword.hand == HeldHand::RightHand &&
        sword.parentMode == HeldItemParentMode::BodyStow && !sword.detached &&
        !sword.transition.active && sword.visualStowBlend == 1.0f &&
        sword.visualGripBlend == 0.0f;
}

constexpr ClaimedRewardCapturePixelPolicy ClaimedRewardCapturePolicy(
    const std::string_view checkpointName, const bool swordFullyStowed = false)
{
    if (checkpointName == "finale-roof")
    {
        // The authored finale now occurs after the route-local reward claim.
        // Prove that the reward replaced only the failed torch while the
        // skinned hand, top ring and hanging body remain visible. A fully
        // body-stowed sword can be outside primary view; a held sword must remain visible.
        return {
            .requirePlayerPixels = true,
            .requireRewardRingPixels = true,
            .requireRewardBodyPixels = true,
            .requireSwordPixels = !swordFullyStowed,
            .permitsCompleteWallRetraction = false,
        };
    }
    if (checkpointName == "lantern-wall-high" ||
        checkpointName == "lantern-wall-low")
    {
        // These checkpoints stage the exact maximum-clearance collision stop.
        // The shared carry path may retract the hand, ring, and body fully out
        // of the primary camera while the sword remains as a visible negative
        // control when held. Stable body stow may put it outside primary view.
        // Instance masks and GripRing authority are still mandatory.
        return {
            .requirePlayerPixels = false,
            .requireRewardRingPixels = false,
            .requireRewardBodyPixels = false,
            .requireSwordPixels = !swordFullyStowed,
            .permitsCompleteWallRetraction = true,
        };
    }
    if (checkpointName == "lantern-chest-held-high")
    {
        // Exact live post-claim stand-off: the chest collision footprint must
        // retract the carry safely without hiding the anatomical-left arm,
        // top ring, hanging body, or right-hand sword on a phone viewport.
        return {
            .requirePlayerPixels = true,
            .requireRewardRingPixels = true,
            .requireRewardBodyPixels = true,
            .requireSwordPixels = !swordFullyStowed,
            .permitsCompleteWallRetraction = false,
        };
    }
    return {};
}

} // namespace horde::platform::windows
