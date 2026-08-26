#include "gameplay/animation/PlayerAnimationState.h"
#include "gameplay/animation/PlayerIkTargets.h"
#include "gameplay/simulation/GameSimulation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

namespace
{

bool Require(const bool condition, const char* message)
{
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}

bool Near(const float left, const float right, const float tolerance = 0.0001f)
{
    return std::abs(left - right) <= tolerance;
}

float Distance(const std::array<float, 3u>& left, const std::array<float, 3u>& right)
{
    return std::hypot(std::hypot(left[0] - right[0], left[1] - right[1]), left[2] - right[2]);
}

} // namespace

int main()
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::animation;

    if (!Require(MapPlayerLocomotionClip(0.0f) == PlayerLocomotionClip::Idle,
                 "zero locomotion must map to idle")) return 1;
    if (!Require(MapPlayerLocomotionClip(0.8f) == PlayerLocomotionClip::Walk,
                 "non-zero locomotion must map to walk")) return 1;

    PlayerCombatSnapshot combat{};
    combat.action = PlayerCombatAction::SwingWindup;
    combat.actionTime = kPlayerSwingWindupSeconds;
    const PlayerCombatLayer swingWindupEnd = EvaluatePlayerCombatLayer(combat);
    combat.action = PlayerCombatAction::SwingActive;
    combat.actionTime = 0.0f;
    const PlayerCombatLayer swingActiveStart = EvaluatePlayerCombatLayer(combat);
    if (!Require(swingWindupEnd.action == PlayerUpperBodyAction::Sword &&
                 swingActiveStart.action == PlayerUpperBodyAction::Sword &&
                 Near(swingWindupEnd.normalizedActionTime, swingActiveStart.normalizedActionTime),
                 "sword layer must be continuous across windup/active")) return 1;
    combat.action = PlayerCombatAction::ParryStartup;
    combat.actionTime = kPlayerParryStartupSeconds;
    const PlayerCombatLayer parryStartupEnd = EvaluatePlayerCombatLayer(combat);
    combat.action = PlayerCombatAction::ParryActive;
    combat.actionTime = 0.0f;
    const PlayerCombatLayer parryActiveStart = EvaluatePlayerCombatLayer(combat);
    if (!Require(parryStartupEnd.action == PlayerUpperBodyAction::Parry &&
                 Near(parryStartupEnd.normalizedActionTime, parryActiveStart.normalizedActionTime),
                 "parry layer must be continuous across startup/active")) return 1;

    PlayerAnimationState state;
    PlayerAnimationInput input{};
    input.walkAmount = 1.0f;
    input.walkTime = 0.5f;
    input.heldItemKinematics.leftShoulderLocal = {{-0.25f, -0.44f, 0.39f}};
    input.heldItemKinematics.rightShoulderLocal = {{0.25f, -0.44f, 0.39f}};
    input.heldItemKinematics.leftHandLocal = {{-0.34f, -0.40f, 1.05f}};
    input.heldItemKinematics.rightHandLocal = {{0.34f, -0.41f, 1.05f}};
    input.lanternPoseTarget = 1.0f;
    combat.action = PlayerCombatAction::SwingActive;
    combat.actionTime = 0.05f;
    input.playerCombat = combat;
    state.StepFixed(input, 1.0f / 60.0f);
    const PlayerAnimationSnapshot layered = state.Snapshot();
    if (!Require(layered.locomotionBlend > 0.0f &&
                 layered.combatLayer.action == PlayerUpperBodyAction::Sword,
                 "upper body combat must layer over, not replace, locomotion")) return 1;
    if (!Require(layered.leftIk.target == input.heldItemKinematics.leftHandLocal &&
                 layered.rightIk.target == input.heldItemKinematics.rightHandLocal,
                 "held-item kinematics must author both hand targets")) return 1;
    if (!Require(!layered.visibility.headPrimaryVisible &&
                 !layered.visibility.nearFacePrimaryVisible &&
                 layered.visibility.shadowVisible && layered.visibility.reflectionVisible,
                 "head masking must affect first-person primary rays only")) return 1;

    const float previousPose = layered.lanternPoseBlend;
    input.lanternPoseTarget = 0.0f;
    state.StepFixed(input, 1.0f / 60.0f);
    if (!Require(state.Snapshot().lanternPoseBlend < previousPose &&
                 previousPose - state.Snapshot().lanternPoseBlend <=
                     kLanternPoseBlendRatePerSecond / 60.0f + 0.0001f,
                 "high/low pose target must move continuously at the fixed-step rate")) return 1;

    const TwoBoneIkSolution reachable = SolveTwoBoneIk(
        {{0.0f, 0.0f, 0.0f}}, {{0.35f, -0.25f, 0.25f}}, {{0.0f, 0.0f, 1.0f}}, 0.42f, 0.40f);
    if (!Require(reachable.reachable && Distance(reachable.hand, {{0.35f, -0.25f, 0.25f}}) < 0.0001f,
                 "two-bone IK must reach an in-range hand target")) return 1;
    const TwoBoneIkSolution clamped = SolveTwoBoneIk(
        {{0.0f, 0.0f, 0.0f}}, {{2.0f, 0.0f, 0.0f}}, {{0.0f, 0.0f, 1.0f}}, 0.42f, 0.40f);
    if (!Require(!clamped.reachable && Near(Distance(clamped.shoulder, clamped.hand), 0.82f, 0.0002f),
                 "two-bone IK must clamp beyond total arm reach")) return 1;

    const auto socket = BuildHandBoneSocketTransform(reachable.hand, {{0.0f, 0.0f, 1.0f}}, {{0.0f, 1.0f, 0.0f}});
    if (!Require(MeasureHandSocketError(socket, reachable.hand) < 0.00001f,
                 "hand bone socket must remain at the solved hand")) return 1;

    const PlayerAnimationSnapshot imported = state.Snapshot();
    PlayerAnimationState restored;
    restored.Import(imported);
    if (!Require(restored.Snapshot() == imported, "animation snapshot import must be exact")) return 1;
    restored.Reset();
    if (!Require(restored.Snapshot() == PlayerAnimationSnapshot{}, "animation reset must restore defaults")) return 1;

    const auto deliver = [&input](const int renderRate) {
        PlayerAnimationState delivered;
        const int frames = renderRate;
        const int fixedTicksPerFrame = 60 / renderRate;
        for (int frame = 0; frame < frames; ++frame)
        {
            for (int tick = 0; tick < fixedTicksPerFrame; ++tick)
            {
                delivered.StepFixed(input, 1.0f / 60.0f);
            }
        }
        return delivered.Snapshot();
    };
    const PlayerAnimationSnapshot at30 = deliver(30);
    const PlayerAnimationSnapshot at60 = deliver(60);
    PlayerAnimationState at120State;
    for (int frame = 0; frame < 120; ++frame)
    {
        if ((frame & 1) == 1) at120State.StepFixed(input, 1.0f / 60.0f);
    }
    const PlayerAnimationSnapshot at120 = at120State.Snapshot();
    if (!Require(at30 == at60 && at60 == at120,
                 "30/60/120 render delivery must converge on the same 60 Hz animation snapshot")) return 1;

    horde::gameplay::simulation::GameSimulation simulation;
    horde::gameplay::simulation::InputSnapshot simulationInput{};
    simulationInput.moveForward = 1.0f;
    simulationInput.commands.attack = 1u;
    simulation.StepFixed(simulationInput);
    const auto& authoritative = simulation.Snapshot();
    if (!Require(authoritative.playerAnimation.combatLayer.action ==
                     PlayerUpperBodyAction::Sword &&
                 authoritative.playerAnimation.leftIk.target ==
                     authoritative.heldItemKinematics.leftHandLocal &&
                 authoritative.playerAnimation.rightIk.target ==
                     authoritative.heldItemKinematics.rightHandLocal,
                 "GameSimulation must publish authoritative action-following player animation and IK")) return 1;
    simulation.ResetRoute();
    if (!Require(simulation.Snapshot().playerAnimation.combatLayer.action ==
                     PlayerUpperBodyAction::None &&
                 Near(simulation.Snapshot().playerAnimation.locomotionBlend, 0.0f),
                 "route reset must reset authoritative player animation state")) return 1;

    std::cout << "Player animation, IK, socket, reset/import, visibility, and delivery contracts passed\n";
    return 0;
}
