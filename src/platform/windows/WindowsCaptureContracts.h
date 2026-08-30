#pragma once

#include <string_view>

namespace horde::platform::windows
{

struct ClaimedRewardCapturePixelPolicy
{
    bool requirePlayerPixels = true;
    bool requireRewardRingPixels = false;
    bool requireRewardBodyPixels = true;
    bool requireSwordPixels = false;
    bool permitsCompleteWallRetraction = false;
};

constexpr ClaimedRewardCapturePixelPolicy ClaimedRewardCapturePolicy(
    const std::string_view checkpointName)
{
    if (checkpointName == "finale-roof")
    {
        // The authored finale now occurs after the route-local reward claim.
        // Prove that the reward replaced only the failed torch while the
        // skinned hand, top ring, hanging body, and sword remain visible.
        return {
            .requirePlayerPixels = true,
            .requireRewardRingPixels = true,
            .requireRewardBodyPixels = true,
            .requireSwordPixels = true,
            .permitsCompleteWallRetraction = false,
        };
    }
    if (checkpointName == "lantern-wall-high" ||
        checkpointName == "lantern-wall-low")
    {
        // These checkpoints stage the exact maximum-clearance collision stop.
        // The shared carry path may retract the hand, ring, and body fully out
        // of the primary camera while the sword remains as a visible negative
        // control. Instance masks and GripRing authority are still mandatory.
        return {
            .requirePlayerPixels = false,
            .requireRewardRingPixels = false,
            .requireRewardBodyPixels = false,
            .requireSwordPixels = true,
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
            .requireSwordPixels = true,
            .permitsCompleteWallRetraction = false,
        };
    }
    return {};
}

} // namespace horde::platform::windows
