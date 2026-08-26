#include "gameplay/animation/PlayerAnimationState.h"
#include "gameplay/animation/PlayerIkTargets.h"
#include "gameplay/simulation/GameSimulation.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"

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
    combat.action = PlayerCombatAction::UpwardSliceWindup;
    combat.actionTime = kPlayerUpwardSliceWindupSeconds;
    const PlayerCombatLayer upwardWindupEnd = EvaluatePlayerCombatLayer(combat);
    combat.action = PlayerCombatAction::UpwardSliceActive;
    combat.actionTime = 0.0f;
    const PlayerCombatLayer upwardActiveStart = EvaluatePlayerCombatLayer(combat);
    if (!Require(upwardWindupEnd.action == PlayerUpperBodyAction::UpwardSlice &&
                 upwardActiveStart.action == PlayerUpperBodyAction::UpwardSlice &&
                 Near(upwardWindupEnd.normalizedActionTime,
                      upwardActiveStart.normalizedActionTime),
                 "upward slice layer must be continuous across windup/active")) return 1;
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

    const auto ownerFeedbackPose = horde::gameplay::items::EvaluateHeldItemKinematics({});
    const float shoulderWidth = ownerFeedbackPose.rightShoulderLocal[0] -
                                ownerFeedbackPose.leftShoulderLocal[0];
    const float handSeparation = ownerFeedbackPose.rightHandLocal[0] -
                                 ownerFeedbackPose.leftHandLocal[0];
    if (!Require(shoulderWidth >= 0.68f && handSeparation >= 0.48f,
                 "first-person rest pose must retain lateral shoulder roots and torso space")) return 1;
    const TwoBoneIkSolution ownerLeftArm = SolveTwoBoneIk(
        ownerFeedbackPose.leftShoulderLocal, ownerFeedbackPose.leftHandLocal,
        {{-0.9f, -0.1f, 0.4f}}, 0.53f, 0.53f);
    const TwoBoneIkSolution ownerRightArm = SolveTwoBoneIk(
        ownerFeedbackPose.rightShoulderLocal, ownerFeedbackPose.rightHandLocal,
        {{0.9f, -0.1f, 0.4f}}, 0.53f, 0.53f);
    if (!Require(ownerLeftArm.elbow[0] <= -0.30f && ownerRightArm.elbow[0] >= 0.30f,
                 "first-person upper arms must remain separate left/right chains")) return 1;
    const auto swordAxis = horde::gameplay::items::EvaluateSwordBladeAxisInView(
        ownerFeedbackPose.swordRadians, ownerFeedbackPose.swordForwardRadians);
    if (!Require(swordAxis[0] <= -0.12f && swordAxis[2] >= 0.10f && swordAxis[1] > 0.90f,
                 "resting sword axis must carry a restrained inward and forward combat cant")) return 1;
    const auto swordGripBasis = horde::gameplay::items::EvaluateSwordGripBasisInView(
        ownerFeedbackPose.swordRadians, ownerFeedbackPose.swordForwardRadians,
        horde::gameplay::items::kSwordGripRollRadians);
    if (!Require(swordGripBasis.edgeDirection[2] >= 0.94f &&
                 std::abs(swordGripBasis.flatNormal[2]) <= 0.30f,
                 "authored Grip basis must roll the sharp edge, not the blade flat, forward")) return 1;
    for (const float pitch : {-0.32f, -0.05f, 0.28f})
    {
        // Match the renderer's pitched view basis and measure the authored
        // edge against its resulting world-space forward direction.
        const float viewForwardY = -0.05f + pitch;
        const float inverseViewLength = 1.0f / std::hypot(viewForwardY, 1.0f);
        const std::array<float, 3u> viewForward{{
            0.0f, viewForwardY * inverseViewLength, -inverseViewLength}};
        const std::array<float, 3u> viewUp{{
            0.0f, -viewForward[2], viewForward[1]}};
        const std::array<float, 3u> worldEdge{{
            swordGripBasis.edgeDirection[0],
            viewUp[1] * swordGripBasis.edgeDirection[1] +
                viewForward[1] * swordGripBasis.edgeDirection[2],
            viewUp[2] * swordGripBasis.edgeDirection[1] +
                viewForward[2] * swordGripBasis.edgeDirection[2]}};
        const float edgeForwardProjection = worldEdge[1] * viewForward[1] +
                                            worldEdge[2] * viewForward[2];
        if (!Require(edgeForwardProjection >= 0.90f,
                     "sword edge-forward basis must remain stable across camera pitch"))
            return 1;
    }
    horde::gameplay::items::HeldItemKinematicsInput wallInput{};
    wallInput.cameraZ = -9.70f;
    const auto wallPose = horde::gameplay::items::EvaluateHeldItemKinematics(wallInput);
    const auto wallBasis = horde::gameplay::items::EvaluateSwordGripBasisInView(
        wallPose.swordRadians, wallPose.swordForwardRadians,
        horde::gameplay::items::kSwordGripRollRadians);
    if (!Require(wallPose.heldPropDepth <= 0.38f &&
                 wallBasis.edgeDirection[2] >= 0.90f,
                 "wall retraction must preserve the RightHand edge-forward Grip basis")) return 1;
    const auto portraitFrame = horde::gameplay::items::EvaluateOwnerFeedbackPortraitSafeFrame(
        ownerFeedbackPose, 1440.0f / 3120.0f);
    std::cout << "Owner portrait safe-frame NDC x=[" << portraitFrame.minimumNdcX
              << ", " << portraitFrame.maximumNdcX << "]\n";
    if (!Require(portraitFrame.minimumNdcX >= -0.94f &&
                 portraitFrame.maximumNdcX <= 0.94f &&
                 portraitFrame.includesTorchGrip && portraitFrame.includesFlame &&
                 portraitFrame.includesLight && portraitFrame.includesSwordGrip &&
                 portraitFrame.includesBladeBounds,
                 "75% portrait contract must keep torch markers, both grips, and blade bounds inside the safe frame"))
        return 1;

    horde::gameplay::items::HeldItemKinematicsInput downwardInput{};
    downwardInput.playerCombat.action = PlayerCombatAction::SwingActive;
    downwardInput.swordSwingRadians = -SwordCombat::kDownwardSwingAmplitude;
    const auto downwardPose = horde::gameplay::items::EvaluateHeldItemKinematics(downwardInput);
    const auto downwardFrame = horde::gameplay::items::EvaluateOwnerFeedbackPortraitSafeFrame(
        downwardPose, 1440.0f / 3120.0f);
    std::cout << "Downward portrait safe-frame NDC x=[" << downwardFrame.minimumNdcX
              << ", " << downwardFrame.maximumNdcX << "]\n";
    if (!Require(downwardFrame.minimumNdcX >= -0.94f &&
                 downwardFrame.maximumNdcX <= 0.94f,
                 "downward-cut blade bounds must remain inside the 75% portrait safe frame"))
        return 1;

    horde::gameplay::items::HeldItemKinematicsInput upwardInput{};
    upwardInput.playerCombat.action = PlayerCombatAction::UpwardSliceActive;
    upwardInput.swordSwingRadians = SwordCombat::kUpwardSliceEndRadians;
    const auto upwardPose = horde::gameplay::items::EvaluateHeldItemKinematics(upwardInput);
    const auto upwardFrame = horde::gameplay::items::EvaluateOwnerFeedbackPortraitSafeFrame(
        upwardPose, 1440.0f / 3120.0f);
    std::cout << "Upward portrait safe-frame NDC x=[" << upwardFrame.minimumNdcX
              << ", " << upwardFrame.maximumNdcX << "]\n";
    if (!Require(upwardFrame.minimumNdcX >= -0.94f &&
                 upwardFrame.maximumNdcX <= 0.94f,
                 "upward-slice blade bounds must remain inside the 75% portrait safe frame"))
        return 1;
    for (int sample = 0; sample <= 24; ++sample)
    {
        const float amount = static_cast<float>(sample) / 24.0f;
        horde::gameplay::items::HeldItemKinematicsInput arcInput{};
        arcInput.playerCombat.action = amount < 0.5f
            ? PlayerCombatAction::SwingActive : PlayerCombatAction::UpwardSliceActive;
        arcInput.swordSwingRadians = -SwordCombat::kDownwardSwingAmplitude +
            (SwordCombat::kUpwardSliceEndRadians + SwordCombat::kDownwardSwingAmplitude) * amount;
        const auto arcPose = horde::gameplay::items::EvaluateHeldItemKinematics(arcInput);
        const auto arcFrame = horde::gameplay::items::EvaluateOwnerFeedbackPortraitSafeFrame(
            arcPose, 1440.0f / 3120.0f);
        const auto arcBasis = horde::gameplay::items::EvaluateSwordGripBasisInView(
            arcPose.swordRadians, arcPose.swordForwardRadians,
            horde::gameplay::items::kSwordGripRollRadians);
        if (!Require(arcFrame.minimumNdcX >= -0.94f &&
                     arcFrame.maximumNdcX <= 0.94f &&
                     arcBasis.edgeDirection[2] >= 0.90f,
                     "the full down/up attack arc must stay portrait-safe and edge-forward"))
            return 1;
    }

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

    using namespace horde::vulkan::raytracing;
    const PlayerRouteMasks proceduralMasks = BuildPlayerRouteMasks(PlayerRenderRoute::Procedural);
    const PlayerRouteMasks skinnedMasks = BuildPlayerRouteMasks(PlayerRenderRoute::Skinned);
    if (!Require(proceduralMasks.instanceMasks[4] == 0x10u &&
                 proceduralMasks.instanceMasks[5] == 0x04u &&
                 proceduralMasks.instanceMasks[16] == 0x10u &&
                 skinnedMasks.instanceMasks[4] == 0x14u,
                 "developer A/B routes must preserve intended primary/reflection masks")) return 1;
    for (std::size_t slot = 5u; slot <= 16u; ++slot)
    {
        if (!Require(skinnedMasks.instanceMasks[slot] == 0u,
                     "skinned route must mask every procedural player slot")) return 1;
    }

    const auto primitiveVisibility = BuildPlayerPrimitiveVisibility({
        PlayerPrimitiveSemantic::Body,
        PlayerPrimitiveSemantic::Head,
        PlayerPrimitiveSemantic::NearFace,
    });
    if (!Require(primitiveVisibility[0].primaryVisible &&
                 !primitiveVisibility[1].primaryVisible &&
                 !primitiveVisibility[2].primaryVisible &&
                 primitiveVisibility[1].shadowVisible &&
                 primitiveVisibility[2].reflectionVisible,
                 "material/primitive metadata must hide only head/near-face primary hits")) return 1;

    const PlayerSocketPlan sockets = EvaluatePlayerSocketPlan(authoritative.playerAnimation);
    if (!Require(sockets.leftErrorMetres <= kPlayerGripSocketToleranceMetres &&
                 sockets.rightErrorMetres <= kPlayerGripSocketToleranceMetres,
                 "final left/right socket transforms must remain inside grip tolerance")) return 1;

    using namespace horde::gameplay::items;
    HeldItemStates authoritativeItems = MakeDefaultHeldItemStates();
    authoritativeItems[0].worldFromItem = IdentityHeldItemTransform();
    authoritativeItems[1].worldFromItem = IdentityHeldItemTransform();
    HeldItemTransform leftBone = IdentityHeldItemTransform();
    HeldItemTransform rightBone = IdentityHeldItemTransform();
    leftBone[12] = -0.41f;
    leftBone[13] = 0.22f;
    leftBone[14] = 0.76f;
    rightBone[12] = 0.38f;
    rightBone[13] = 0.18f;
    rightBone[14] = 0.81f;
    HeldItemStates renderItems{};
    std::string socketDiagnostic;
    if (!Require(ResolvePlayerHeldItemVisuals(authoritativeItems, leftBone, rightBone,
                                               renderItems, socketDiagnostic) &&
                 Near(renderItems[0].worldFromItem[12], leftBone[12]) &&
                 Near(renderItems[0].worldFromItem[13], leftBone[13] - 0.24f) &&
                 Near(renderItems[1].worldFromItem[12], rightBone[12]) &&
                 Near(renderItems[1].worldFromItem[13], rightBone[13] - 0.135f),
                 "attached held-item visuals must compose from final LeftHand/RightHand bone sockets"))
        return 1;
    authoritativeItems[0].parentMode = HeldItemParentMode::AuthoredWorldTrajectory;
    authoritativeItems[0].worldFromItem[12] = 4.0f;
    if (!Require(ResolvePlayerHeldItemVisuals(authoritativeItems, leftBone, rightBone,
                                               renderItems, socketDiagnostic) &&
                 Near(renderItems[0].worldFromItem[12], 4.0f),
                 "detached torch visuals must retain the authoritative world trajectory"))
        return 1;

    authoritativeItems = MakeDefaultHeldItemStates();
    PlayerRenderSlot calibratedSlot;
    if (!Require(calibratedSlot.ResolveHeldItemVisuals(
                     authoritativeItems, leftBone, rightBone,
                     renderItems, socketDiagnostic) &&
                 Near(renderItems[0].worldFromItem[12], 0.0f) &&
                 Near(renderItems[1].worldFromItem[12], 0.0f),
                 "bone-local grip correction must preserve the established visual basis at calibration"))
        return 1;
    leftBone[12] += 0.06f;
    rightBone[12] -= 0.04f;
    if (!Require(calibratedSlot.ResolveHeldItemVisuals(
                     authoritativeItems, leftBone, rightBone,
                     renderItems, socketDiagnostic) &&
                 Near(renderItems[0].worldFromItem[12], 0.06f) &&
                 Near(renderItems[1].worldFromItem[12], -0.04f),
                 "calibrated held-item visuals must follow subsequent final bone motion"))
        return 1;
    if (!Require(ChoosePlayerCpuCadence({0.18, 0.31, 0.018f, 0.004f}) ==
                     PlayerCpuSkinCadence::Hz60 &&
                 ChoosePlayerCpuCadence({0.18, 0.31, 0.004f, 0.003f}) ==
                     PlayerCpuSkinCadence::Hz30,
                 "CPU cadence selection must keep 30 Hz only when measured motion passes")) return 1;

    std::cout << "Player animation, IK, socket, reset/import, visibility, and delivery contracts passed\n";
    return 0;
}
