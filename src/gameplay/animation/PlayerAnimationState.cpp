#include "gameplay/animation/PlayerAnimationState.h"

#include <algorithm>
#include <cmath>

namespace horde::gameplay::animation
{
namespace
{

float FiniteOr(const float value, const float fallback)
{
    return std::isfinite(value) ? value : fallback;
}

float MoveTowards(const float current, const float target, const float maximumDelta)
{
    return current + std::clamp(target - current, -maximumDelta, maximumDelta);
}

float Normalized(const float elapsed, const float total)
{
    return std::clamp(elapsed / total, 0.0f, 1.0f);
}

} // namespace

PlayerLocomotionClip MapPlayerLocomotionClip(const float locomotionBlend)
{
    return locomotionBlend > 0.01f ? PlayerLocomotionClip::Walk
                                   : PlayerLocomotionClip::Idle;
}

PlayerCombatLayer EvaluatePlayerCombatLayer(
    const horde::gameplay::PlayerCombatSnapshot& combat)
{
    PlayerCombatLayer layer;
    const float time = std::max(0.0f, FiniteOr(combat.actionTime, 0.0f));
    constexpr float swingTotal = kPlayerSwingWindupSeconds +
                                 kPlayerSwingActiveSeconds +
                                 kPlayerSwingRecoverySeconds;
    constexpr float parryTotal = kPlayerParryStartupSeconds +
                                 kPlayerParryActiveSeconds +
                                 kPlayerParryRecoverySeconds;
    constexpr float upwardSliceTotal = kPlayerUpwardSliceWindupSeconds +
                                       kPlayerUpwardSliceActiveSeconds +
                                       kPlayerUpwardSliceRecoverySeconds;
    switch (combat.action)
    {
    case horde::gameplay::PlayerCombatAction::SwingWindup:
        layer = {PlayerUpperBodyAction::Sword, Normalized(time, swingTotal), 1.0f};
        break;
    case horde::gameplay::PlayerCombatAction::SwingActive:
        layer = {PlayerUpperBodyAction::Sword,
                 Normalized(kPlayerSwingWindupSeconds + time, swingTotal), 1.0f};
        break;
    case horde::gameplay::PlayerCombatAction::SwingRecovery:
        layer = {PlayerUpperBodyAction::Sword,
                 Normalized(kPlayerSwingWindupSeconds + kPlayerSwingActiveSeconds + time,
                            swingTotal), 1.0f};
        break;
    case horde::gameplay::PlayerCombatAction::UpwardSliceWindup:
        layer = {PlayerUpperBodyAction::UpwardSlice,
                 Normalized(time, upwardSliceTotal), 1.0f};
        break;
    case horde::gameplay::PlayerCombatAction::UpwardSliceActive:
        layer = {PlayerUpperBodyAction::UpwardSlice,
                 Normalized(kPlayerUpwardSliceWindupSeconds + time,
                            upwardSliceTotal), 1.0f};
        break;
    case horde::gameplay::PlayerCombatAction::UpwardSliceRecovery:
        layer = {PlayerUpperBodyAction::UpwardSlice,
                 Normalized(kPlayerUpwardSliceWindupSeconds +
                                kPlayerUpwardSliceActiveSeconds + time,
                            upwardSliceTotal), 1.0f};
        break;
    case horde::gameplay::PlayerCombatAction::ParryStartup:
        layer = {PlayerUpperBodyAction::Parry, Normalized(time, parryTotal), 1.0f};
        break;
    case horde::gameplay::PlayerCombatAction::ParryActive:
        layer = {PlayerUpperBodyAction::Parry,
                 Normalized(kPlayerParryStartupSeconds + time, parryTotal), 1.0f};
        break;
    case horde::gameplay::PlayerCombatAction::ParryRecovery:
        layer = {PlayerUpperBodyAction::Parry,
                 Normalized(kPlayerParryStartupSeconds + kPlayerParryActiveSeconds + time,
                            parryTotal), 1.0f};
        break;
    default:
        break;
    }
    return layer;
}

void PlayerAnimationState::StepFixed(const PlayerAnimationInput& input,
                                     float fixedDeltaSeconds)
{
    fixedDeltaSeconds = std::clamp(FiniteOr(fixedDeltaSeconds, 0.0f), 0.0f, 0.05f);
    const float locomotionTarget = std::clamp(FiniteOr(input.walkAmount, 0.0f), 0.0f, 1.0f);
    snapshot_.locomotionBlend = MoveTowards(
        snapshot_.locomotionBlend, locomotionTarget, fixedDeltaSeconds * 8.0f);
    snapshot_.locomotionClip = MapPlayerLocomotionClip(snapshot_.locomotionBlend);
    snapshot_.locomotionTime = std::max(0.0f, FiniteOr(input.walkTime, 0.0f));
    snapshot_.combatLayer = EvaluatePlayerCombatLayer(input.playerCombat);
    snapshot_.reaction = input.playerCombat.reaction;
    snapshot_.reactionTime = std::max(0.0f, FiniteOr(input.playerCombat.reactionTime, 0.0f));
    snapshot_.lanternPoseBlend = MoveTowards(
        snapshot_.lanternPoseBlend,
        std::clamp(FiniteOr(input.lanternPoseTarget, 0.0f), 0.0f, 1.0f),
        fixedDeltaSeconds * kLanternPoseBlendRatePerSecond);
    snapshot_.swordStowBlend = std::clamp(
        FiniteOr(input.heldItemKinematics.swordStowBlend, 0.0f), 0.0f, 1.0f);
    snapshot_.swordHandGripBlend = std::clamp(
        FiniteOr(input.heldItemKinematics.swordHandGripBlend, 1.0f), 0.0f, 1.0f);
    snapshot_.leftIk.shoulder = input.heldItemKinematics.leftShoulderLocal;
    snapshot_.leftIk.target = input.heldItemKinematics.leftHandLocal;
    // A conventional first-person carry drops each upper arm beside the
    // torso, then bends the forearm inward and upward to the grip.  Keep the
    // pole predominantly down with only a small same-side bias; the old
    // equally down-and-out pole flared elbows beyond the shoulders and made
    // the complete sleeve read as one straight shoulder-to-hand bar.
    snapshot_.leftIk.pole = {{-0.12f, -1.0f, 0.0f}};
    snapshot_.leftIk.preferredElbowFlexionRadians = 0.0f;
    if (input.carryingRewardLantern)
    {
        // The authored chain is shorter than the held-grip envelope. Stretching
        // it only to straight-line reach locks the elbow regardless of pole.
        // Reserve a small bend, responding to existing movement/pendulum state
        // without feeding the solved arm back into the grip or lantern physics.
        constexpr float radiansPerDegree = 0.01745329252f;
        const float gait = snapshot_.locomotionBlend * std::sin(snapshot_.locomotionTime * 6.2f);
        const float forward = std::clamp(FiniteOr(input.lanternForwardAngleRadians, 0.0f), -1.0f, 1.0f);
        const float strafe = std::clamp(FiniteOr(input.lanternStrafeAngleRadians, 0.0f), -1.0f, 1.0f);
        const float swordLift = std::clamp(
            (FiniteOr(input.heldItemKinematics.rightHandLocal[1], -0.44f) + 0.44f) / 0.4f, -1.0f, 1.0f);
        snapshot_.leftIk.preferredElbowFlexionRadians =
            (22.0f + 3.0f * gait + 5.0f * forward + 2.0f * swordLift) * radiansPerDegree;
        snapshot_.leftIk.pole[0] += 0.10f * strafe;
    }
    snapshot_.leftIk.gripX = input.heldItemKinematics.leftGripXInView;
    snapshot_.leftIk.gripY = input.heldItemKinematics.leftGripYInView;
    snapshot_.leftIk.gripZ = input.heldItemKinematics.leftGripZInView;
    snapshot_.rightIk.shoulder = input.heldItemKinematics.rightShoulderLocal;
    snapshot_.rightIk.target = input.heldItemKinematics.rightHandLocal;
    snapshot_.rightIk.pole = {{0.12f, -1.0f, 0.0f}};
    const auto swordGrip = horde::gameplay::items::EvaluateSwordGripBasisInView(
        input.heldItemKinematics.swordRadians,
        input.heldItemKinematics.swordForwardRadians,
        horde::gameplay::items::kSwordGripRollRadians);
    snapshot_.rightIk.gripX = swordGrip.edgeDirection;
    snapshot_.rightIk.gripY = swordGrip.bladeAxis;
    snapshot_.rightIk.gripZ = swordGrip.flatNormal;
    // Releasing the sword changes the hand's target, not ownership of the arm.
    // Keep the shared empty-hand carry visible instead of dropping back to the
    // imported clip's out-of-view arm rest.
    snapshot_.rightIk.poseWeight = 1.0f;
}

void PlayerAnimationState::Reset()
{
    snapshot_ = {};
}

void PlayerAnimationState::Import(const PlayerAnimationSnapshot& snapshot)
{
    snapshot_ = snapshot;
}

} // namespace horde::gameplay::animation
