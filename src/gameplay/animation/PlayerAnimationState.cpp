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
    snapshot_.leftIk.shoulder = input.heldItemKinematics.leftShoulderLocal;
    snapshot_.leftIk.target = input.heldItemKinematics.leftHandLocal;
    snapshot_.leftIk.pole = {{-0.95f, -0.1f, 0.42f}};
    snapshot_.leftIk.gripX = {{1.0f, 0.0f, 0.0f}};
    snapshot_.leftIk.gripY = {{0.0f, 1.0f, 0.0f}};
    snapshot_.leftIk.gripZ = {{0.0f, 0.0f, -1.0f}};
    snapshot_.rightIk.shoulder = input.heldItemKinematics.rightShoulderLocal;
    snapshot_.rightIk.target = input.heldItemKinematics.rightHandLocal;
    snapshot_.rightIk.pole = {{0.95f, -0.1f, 0.42f}};
    const auto swordGrip = horde::gameplay::items::EvaluateSwordGripBasisInView(
        input.heldItemKinematics.swordRadians,
        input.heldItemKinematics.swordForwardRadians,
        horde::gameplay::items::kSwordGripRollRadians);
    snapshot_.rightIk.gripX = swordGrip.edgeDirection;
    snapshot_.rightIk.gripY = swordGrip.bladeAxis;
    snapshot_.rightIk.gripZ = swordGrip.flatNormal;
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
