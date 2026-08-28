#include "gameplay/interactions/InteractionState.h"

#include <algorithm>
#include <cmath>

namespace horde::gameplay::interactions
{

bool IsInteractionAvailable(const InteractionQuery& query,
                            const InteractionPosition& target)
{
    if (!std::isfinite(query.playerX) || !std::isfinite(query.playerZ) ||
        !std::isfinite(query.playerYawRadians) || !std::isfinite(target.x) ||
        !std::isfinite(target.z))
    {
        return false;
    }
    const float toTargetX = target.x - query.playerX;
    const float toTargetZ = target.z - query.playerZ;
    const float distance = std::hypot(toTargetX, toTargetZ);
    if (distance > kInteractionMaximumRangeMetres || distance <= 0.00001f)
    {
        return distance <= 0.00001f;
    }
    const float forwardX = std::sin(query.playerYawRadians);
    const float forwardZ = -std::cos(query.playerYawRadians);
    const float facing = (forwardX * toTargetX + forwardZ * toTargetZ) / distance;
    constexpr float radiansPerDegree = 0.01745329251994329577f;
    return facing + 0.000001f >=
        std::cos(kInteractionFacingMaximumDegrees * radiansPerDegree);
}

void ResetInteractionState(InteractionState& state, const HeldLightKind heldLightKind)
{
    state = {};
    state.heldLightKind = heldLightKind;
    state.heldLightPose = HeldLightPose::High;
    state.heldLightPoseProgress = 1.0f;
}

void EquipRewardLantern(InteractionState& state)
{
    state.heldLightKind = HeldLightKind::RewardLantern;
    state.heldLightPose = HeldLightPose::TransitioningToHigh;
    state.heldLightPoseProgress = 0.0f;
}

bool RequestHeldLightPoseToggle(InteractionState& state)
{
    if (state.heldLightKind != HeldLightKind::RewardLantern)
    {
        return false;
    }
    if (state.heldLightPose == HeldLightPose::High)
    {
        state.heldLightPose = HeldLightPose::TransitioningToLow;
        state.heldLightPoseProgress = 0.0f;
        return true;
    }
    if (state.heldLightPose == HeldLightPose::Low)
    {
        state.heldLightPose = HeldLightPose::TransitioningToHigh;
        state.heldLightPoseProgress = 0.0f;
        return true;
    }
    return false;
}

void AdvanceHeldLightPose(InteractionState& state, float fixedDeltaSeconds)
{
    if (!std::isfinite(fixedDeltaSeconds) || fixedDeltaSeconds <= 0.0f)
    {
        return;
    }
    if (state.heldLightPose != HeldLightPose::TransitioningToLow &&
        state.heldLightPose != HeldLightPose::TransitioningToHigh)
    {
        state.heldLightPoseProgress = 1.0f;
        return;
    }
    state.heldLightPoseProgress = std::min(
        1.0f,
        state.heldLightPoseProgress + fixedDeltaSeconds / kHeldLightPoseTransitionSeconds);
    if (state.heldLightPoseProgress + 0.000001f >= 1.0f)
    {
        state.heldLightPose = state.heldLightPose == HeldLightPose::TransitioningToLow
            ? HeldLightPose::Low
            : HeldLightPose::High;
        state.heldLightPoseProgress = 1.0f;
    }
}

} // namespace horde::gameplay::interactions
