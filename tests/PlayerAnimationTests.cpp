#include "gameplay/animation/PlayerAnimationState.h"
#include "gameplay/animation/PlayerIkTargets.h"
#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/items/LanternPendulum.h"
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
    const auto layeredSwordBasis = horde::gameplay::items::EvaluateSwordGripBasisInView(
        input.heldItemKinematics.swordRadians,
        input.heldItemKinematics.swordForwardRadians,
        horde::gameplay::items::kSwordGripRollRadians);
    if (!Require(layered.leftIk.gripX == input.heldItemKinematics.leftGripXInView &&
                 layered.leftIk.gripY == input.heldItemKinematics.leftGripYInView &&
                 layered.leftIk.gripZ == input.heldItemKinematics.leftGripZInView &&
                 layered.rightIk.gripX == layeredSwordBasis.edgeDirection &&
                 layered.rightIk.gripY == layeredSwordBasis.bladeAxis &&
                 layered.rightIk.gripZ == layeredSwordBasis.flatNormal,
                 "fixed-step animation authority must publish both deterministic Grip bases"))
        return 1;
    if (!Require(!layered.visibility.headPrimaryVisible &&
                 !layered.visibility.nearFacePrimaryVisible &&
                 layered.visibility.shadowVisible && layered.visibility.reflectionVisible,
                 "head masking must affect first-person primary rays only")) return 1;

    const auto ownerFeedbackPose = horde::gameplay::items::EvaluateHeldItemKinematics({});
    const float shoulderWidth = ownerFeedbackPose.rightShoulderLocal[0] -
                                ownerFeedbackPose.leftShoulderLocal[0];
    const float handSeparation = ownerFeedbackPose.rightHandLocal[0] -
                                 ownerFeedbackPose.leftHandLocal[0];
    if (!Require(shoulderWidth >= 0.68f && handSeparation >= 0.20f &&
                     handSeparation <= 0.30f,
                 "first-person rest pose must retain lateral shoulder roots while the forearms bend inward to portrait-safe prop grips")) return 1;
    const float leftGripDeterminant =
        ownerFeedbackPose.leftGripXInView[0] *
            (ownerFeedbackPose.leftGripYInView[1] * ownerFeedbackPose.leftGripZInView[2] -
             ownerFeedbackPose.leftGripYInView[2] * ownerFeedbackPose.leftGripZInView[1]) -
        ownerFeedbackPose.leftGripYInView[0] *
            (ownerFeedbackPose.leftGripXInView[1] * ownerFeedbackPose.leftGripZInView[2] -
             ownerFeedbackPose.leftGripXInView[2] * ownerFeedbackPose.leftGripZInView[1]) +
        ownerFeedbackPose.leftGripZInView[0] *
            (ownerFeedbackPose.leftGripXInView[1] * ownerFeedbackPose.leftGripYInView[2] -
             ownerFeedbackPose.leftGripXInView[2] * ownerFeedbackPose.leftGripYInView[1]);
    if (!Require(leftGripDeterminant < -0.999f &&
                 ownerFeedbackPose.leftGripXInView[0] > 0.999f &&
                 ownerFeedbackPose.leftGripZInView[2] < -0.999f,
                 "left torch grip must retain the authored broadside palm instead of collapsing the fingers into an end-on fist blob"))
        return 1;
    const TwoBoneIkSolution ownerLeftArm = SolveTwoBoneIk(
        ownerFeedbackPose.leftShoulderLocal, ownerFeedbackPose.leftHandLocal,
        layered.leftIk.pole, layered.leftIk.upperArmLength,
        layered.leftIk.lowerArmLength);
    const TwoBoneIkSolution ownerRightArm = SolveTwoBoneIk(
        ownerFeedbackPose.rightShoulderLocal, ownerFeedbackPose.rightHandLocal,
        layered.rightIk.pole, layered.rightIk.upperArmLength,
        layered.rightIk.lowerArmLength);
    if (!Require(ownerLeftArm.elbow[0] >=
                     ownerFeedbackPose.leftShoulderLocal[0] - 0.04f &&
                 ownerRightArm.elbow[0] <=
                     ownerFeedbackPose.rightShoulderLocal[0] + 0.04f &&
                 ownerLeftArm.elbow[1] <= ownerFeedbackPose.leftHandLocal[1] - 0.32f &&
                 ownerRightArm.elbow[1] <= ownerFeedbackPose.rightHandLocal[1] - 0.32f,
                 "first-person upper arms must drop below and slightly inward from the shoulders before the forearms bend across to the grips")) return 1;
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

    horde::gameplay::items::HeldItemKinematicsInput downwardWindupInput{};
    downwardWindupInput.playerCombat.action = PlayerCombatAction::SwingWindup;
    downwardWindupInput.playerCombat.actionTime = SwordCombat::kSwingWindupDuration;
    const auto downwardWindupPose =
        horde::gameplay::items::EvaluateHeldItemKinematics(downwardWindupInput);
    horde::gameplay::items::HeldItemKinematicsInput downwardInput{};
    downwardInput.playerCombat.action = PlayerCombatAction::SwingActive;
    downwardInput.playerCombat.actionTime = SwordCombat::kSwingActiveDuration;
    downwardInput.swordSwingRadians = -SwordCombat::kDownwardSwingAmplitude;
    const auto downwardPose = horde::gameplay::items::EvaluateHeldItemKinematics(downwardInput);
    if (!Require(downwardWindupPose.rightHandLocal[1] -
                     downwardPose.rightHandLocal[1] >= 0.50f &&
                 std::abs(downwardWindupPose.rightHandLocal[0] -
                          downwardPose.rightHandLocal[0]) <= 0.30f &&
                 downwardPose.swordRadians >= 0.75f &&
                 downwardPose.swordForwardRadians >= 0.98f,
                 "first attack must be an unmistakable vertical downward cut, not a small lateral hand shift"))
        return 1;
    const auto downwardFrame = horde::gameplay::items::EvaluateOwnerFeedbackPortraitSafeFrame(
        downwardPose, 1440.0f / 3120.0f);
    std::cout << "Downward portrait safe-frame NDC x=[" << downwardFrame.minimumNdcX
              << ", " << downwardFrame.maximumNdcX << "]\n";
    if (!Require(downwardFrame.minimumNdcX >= -0.94f &&
                 downwardFrame.maximumNdcX <= 0.94f,
                 "downward-cut blade bounds must remain inside the 75% portrait safe frame"))
        return 1;

    horde::gameplay::items::HeldItemKinematicsInput upwardStartInput{};
    upwardStartInput.playerCombat.action = PlayerCombatAction::UpwardSliceWindup;
    upwardStartInput.playerCombat.actionTime = 0.0f;
    const auto upwardStartPose =
        horde::gameplay::items::EvaluateHeldItemKinematics(upwardStartInput);
    horde::gameplay::items::HeldItemKinematicsInput upwardInput{};
    upwardInput.playerCombat.action = PlayerCombatAction::UpwardSliceActive;
    upwardInput.playerCombat.actionTime = SwordCombat::kUpwardSliceActiveDuration;
    upwardInput.swordSwingRadians = SwordCombat::kUpwardSliceEndRadians;
    const auto upwardPose = horde::gameplay::items::EvaluateHeldItemKinematics(upwardInput);
    if (!Require(Distance(downwardPose.rightHandLocal,
                          upwardStartPose.rightHandLocal) <= 0.025f &&
                 std::abs(downwardPose.swordRadians -
                          upwardStartPose.swordRadians) <= 0.05f &&
                 upwardPose.rightHandLocal[1] - upwardStartPose.rightHandLocal[1] >= 0.54f &&
                 upwardPose.swordRadians <= -0.25f,
                 "queued second press must continue from the down-cut and drive a smooth, readable upward slice"))
        return 1;
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
        if (amount < 0.5f)
        {
            arcInput.playerCombat.action = PlayerCombatAction::SwingActive;
            arcInput.playerCombat.actionTime =
                SwordCombat::kSwingActiveDuration * amount * 2.0f;
        }
        else
        {
            arcInput.playerCombat.action = PlayerCombatAction::UpwardSliceActive;
            arcInput.playerCombat.actionTime =
                SwordCombat::kUpwardSliceActiveDuration * (amount - 0.5f) * 2.0f;
        }
        arcInput.swordSwingRadians = -SwordCombat::kDownwardSwingAmplitude +
            (SwordCombat::kUpwardSliceEndRadians + SwordCombat::kDownwardSwingAmplitude) * amount;
        const auto arcPose = horde::gameplay::items::EvaluateHeldItemKinematics(arcInput);
        const auto arcFrame = horde::gameplay::items::EvaluateOwnerFeedbackPortraitSafeFrame(
            arcPose, 1440.0f / 3120.0f);
        if (arcFrame.minimumNdcX < -0.94f || arcFrame.maximumNdcX > 0.94f)
        {
            std::cout << "Unsafe attack sample " << sample << " NDC x=["
                      << arcFrame.minimumNdcX << ", "
                      << arcFrame.maximumNdcX << "] hand="
                      << arcPose.rightHandLocal[0] << ','
                      << arcPose.rightHandLocal[1] << ','
                      << arcPose.rightHandLocal[2] << " angles="
                      << arcPose.swordRadians << '/'
                      << arcPose.swordForwardRadians << '\n';
        }
        if (!Require(arcFrame.minimumNdcX >= -0.94f &&
                     arcFrame.maximumNdcX <= 0.94f,
                     "the full down/up attack arc must stay inside the portrait safe frame"))
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
    const PlayerModelWorldBasis modelBasis = BuildPlayerModelWorldBasis(
        {{1.0f, 0.0f, 0.0f}}, {{0.0f, 0.0f, -1.0f}});
    const auto anatomicalLeftInWorld = PlayerModelVectorToWorld(
        modelBasis, {{1.0f, 0.0f, 0.0f}});
    const auto gameplayLeftInModel = WorldVectorToPlayerModel(
        modelBasis, {{-1.0f, 0.0f, 0.0f}});
    if (!Require(PlayerModelWorldBasisDeterminant(modelBasis) > 0.999f &&
                 anatomicalLeftInWorld[0] < -0.999f &&
                 gameplayLeftInModel[0] > 0.999f,
                 "the +Z player rig must use a proper 180-degree rotation that keeps anatomical Left on gameplay left"))
        return 1;
    const PlayerRouteMasks proceduralMasks = BuildPlayerRouteMasks(PlayerRenderRoute::Procedural);
    const PlayerRouteMasks skinnedMasks = BuildPlayerRouteMasks(PlayerRenderRoute::Skinned);
    const PlayerRouteMasks hybridMasks =
        BuildPlayerRouteMasks(PlayerRenderRoute::HybridBlockPrimary);
    if (!Require(proceduralMasks.instanceMasks[4] == 0x10u &&
                 proceduralMasks.instanceMasks[5] == 0x04u &&
                 proceduralMasks.instanceMasks[16] == 0x10u &&
                 skinnedMasks.instanceMasks[4] == 0x14u &&
                 hybridMasks.instanceMasks[4] == 0x10u &&
                 hybridMasks.instanceMasks[9] == 0u &&
                 hybridMasks.instanceMasks[10] == 0x04u &&
                 hybridMasks.instanceMasks[11] == 0x04u &&
                 hybridMasks.instanceMasks[12] == 0x04u &&
                 hybridMasks.instanceMasks[13] == 0x04u &&
                 hybridMasks.instanceMasks[14] == 0u,
                 "developer A/B routes must preserve intended primary/reflection masks")) return 1;
    for (std::size_t slot = 5u; slot <= 16u; ++slot)
    {
        if (!Require(skinnedMasks.instanceMasks[slot] == 0u,
                     "skinned route must mask every procedural player slot")) return 1;
    }

    const ProductionSceneVisibility normalRewardWorld =
        BuildProductionSceneVisibility({PlayerRenderRoute::Procedural,
                                        false,
                                        false,
                                        false});
    if (!Require(normalRewardWorld.rewardWorldVisible &&
                 !normalRewardWorld.inspectionOverride &&
                 normalRewardWorld.playerRoute == PlayerRenderRoute::HybridBlockPrimary &&
                 normalRewardWorld.torchMask == 0x02u &&
                 normalRewardWorld.swordMask == 0x02u &&
                 normalRewardWorld.playerMask == 0x10u &&
                 normalRewardWorld.playerPrimaryVisible &&
                 normalRewardWorld.playerReflectionVisible,
                 "ordinary reward-world existence must retain torch, sword, block-primary arms, and the reflected skinned body"))
        return 1;
    const ProductionSceneVisibility claimedReward =
        BuildProductionSceneVisibility({PlayerRenderRoute::Procedural,
                                        false,
                                        false,
                                        true});
    if (!Require(claimedReward.rewardWorldVisible &&
                 claimedReward.playerRoute == PlayerRenderRoute::HybridBlockPrimary &&
                 claimedReward.torchMask == 0u &&
                 claimedReward.swordMask == 0x02u &&
                 claimedReward.playerMask == 0x10u &&
                 claimedReward.playerPrimaryVisible &&
                 claimedReward.playerReflectionVisible,
                 "claimed reward frames must replace only the ordinary torch while retaining the sword, block-primary arms, and reflected skinned body"))
        return 1;
    const ProductionSceneVisibility skinnedDevelopment =
        BuildProductionSceneVisibility({PlayerRenderRoute::Skinned,
                                        false,
                                        false,
                                        false});
    if (!Require(skinnedDevelopment.playerRoute == PlayerRenderRoute::Skinned &&
                 skinnedDevelopment.playerMask == 0x14u &&
                 skinnedDevelopment.playerPrimaryVisible &&
                 skinnedDevelopment.playerReflectionVisible,
                 "an explicit development A/B request must retain the skinned primary route"))
        return 1;
    const ProductionSceneVisibility inspectionReward =
        BuildProductionSceneVisibility({PlayerRenderRoute::Skinned,
                                        false,
                                        true,
                                        false});
    if (!Require(inspectionReward.rewardWorldVisible &&
                 inspectionReward.inspectionOverride &&
                 inspectionReward.torchMask == 0u &&
                 inspectionReward.swordMask == 0u &&
                 inspectionReward.playerMask == 0u,
                 "the isolated Task 7 inspection override must not leak into ordinary reward-world visibility"))
        return 1;

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
    HeldItemStates gripAlignedItems = MakeDefaultHeldItemStates();
    if (!Require(ComposeWorldFromItem(
                     leftBone, OriginalTorchGripSocketTransform(),
                     gripAlignedItems[0].worldFromItem, socketDiagnostic) &&
                 ComposeWorldFromItem(
                     rightBone, SwordGripSocketTransform(),
                     gripAlignedItems[1].worldFromItem, socketDiagnostic) &&
                 ResolvePlayerHeldItemVisuals(
                     gripAlignedItems, leftBone, rightBone,
                     renderItems, socketDiagnostic),
                 "aligned authoritative Grip fixture must compose"))
        return 1;
    const PlayerGripAgreement leftGripAgreement = MeasurePlayerGripAgreement(
        gripAlignedItems[0], renderItems[0]);
    const PlayerGripAgreement rightGripAgreement = MeasurePlayerGripAgreement(
        gripAlignedItems[1], renderItems[1]);
    if (!Require(leftGripAgreement.positionErrorMetres <=
                     kPlayerGripSocketToleranceMetres &&
                 rightGripAgreement.positionErrorMetres <=
                     kPlayerGripSocketToleranceMetres &&
                 leftGripAgreement.orientationErrorRadians <= 0.0005f &&
                 rightGripAgreement.orientationErrorRadians <= 0.0005f,
                 "post-composition Grip metric must include final position and orientation"))
        return 1;

    HeldItemTransform finalSkinnedLeftGrip = IdentityHeldItemTransform();
    finalSkinnedLeftGrip[12] = -0.42f;
    finalSkinnedLeftGrip[13] = 0.31f;
    finalSkinnedLeftGrip[14] = 0.74f;
    HeldItemTransform authoredGripRing = IdentityHeldItemTransform();
    authoredGripRing[13] = 0.085f;
    HeldItemTransform authoredHinge = IdentityHeldItemTransform();
    authoredHinge[13] = -0.012f;
    HeldItemTransform authoritativeHinge = IdentityHeldItemTransform();
    authoritativeHinge[12] = -0.42f;
    authoritativeHinge[13] = 0.31f;
    authoritativeHinge[14] = 0.74f;
    const HeldItemTransform authoritativeBody =
        horde::gameplay::interactions::ComposeLanternPendulumBodyTransform(
        authoritativeHinge, 0.22f, -0.18f);
    RewardLanternVisualTransforms rewardVisuals;
    if (!Require(ComposeClaimedRewardLanternVisuals(
                     finalSkinnedLeftGrip,
                     authoredGripRing,
                     authoredHinge,
                     authoritativeHinge,
                     authoritativeBody,
                     horde::gameplay::items::kClaimedRewardLanternScale,
                     rewardVisuals,
                     socketDiagnostic),
                 socketDiagnostic.c_str()))
        return 1;
    const HeldItemTransform composedGripRing = MultiplyHeldItemTransforms(
        rewardVisuals.worldFromRing, authoredGripRing);
    const PlayerGripAgreement rewardGripAgreement = MeasureTransformAgreement(
        finalSkinnedLeftGrip, composedGripRing);
    if (!Require(rewardGripAgreement.positionErrorMetres <=
                     kPlayerGripSocketToleranceMetres &&
                 rewardGripAgreement.orientationErrorRadians <=
                     kPlayerGripOrientationToleranceRadians &&
                 Near(rewardVisuals.worldFromHinge[13],
                      finalSkinnedLeftGrip[13] -
                          horde::gameplay::items::kClaimedRewardLanternScale *
                              0.097f),
                 "claimed reward ring must attach through authored GripRing and derive Hinge below the final skinned hand"))
        return 1;
    const HeldItemTransform rewardBodyLocal = MultiplyHeldItemTransforms(
        InverseRigidHeldItemTransform(rewardVisuals.worldFromHinge),
        rewardVisuals.worldFromBody);
    const HeldItemTransform authoritativeBodyLocal = MultiplyHeldItemTransforms(
        InverseRigidHeldItemTransform(authoritativeHinge), authoritativeBody);
    if (!Require(MeasureTransformAgreement(authoritativeBodyLocal, rewardBodyLocal)
                         .orientationErrorRadians <= 0.0005f,
                 "final skinned Grip alignment must preserve the authoritative swing/torsion body rotation"))
        return 1;
    authoritativeItems[0].parentMode = HeldItemParentMode::AuthoredWorldTrajectory;
    authoritativeItems[0].worldFromItem[12] = 4.0f;
    if (!Require(ResolvePlayerHeldItemVisuals(authoritativeItems, leftBone, rightBone,
                                               renderItems, socketDiagnostic) &&
                 Near(renderItems[0].worldFromItem[12], 4.0f),
                 "detached torch visuals must retain the authoritative world trajectory"))
        return 1;

    if (!Require(ChoosePlayerCpuCadence({0.18, 0.31, 0.018f, 0.004f}) ==
                     PlayerCpuSkinCadence::Hz60 &&
                 ChoosePlayerCpuCadence({0.18, 0.31, 0.004f, 0.003f}) ==
                     PlayerCpuSkinCadence::Hz30,
                 "CPU cadence selection must keep 30 Hz only when measured motion passes")) return 1;

    std::cout << "Player animation, IK, socket, reset/import, visibility, and delivery contracts passed\n";
    return 0;
}
