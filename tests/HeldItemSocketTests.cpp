#include "gameplay/items/HeldItemKinematics.h"
#include "gameplay/items/HeldLightState.h"
#include "gameplay/items/HeldItemState.h"
#include "gameplay/interactions/InteractionState.h"
#include "gameplay/simulation/GameSimulation.h"
#include "vulkan/raytracing/HeldItemRenderSlot.h"
#include "vulkan/raytracing/HeldItemBlasMeasurements.h"
#include "vulkan/raytracing/RtSceneAbi.generated.h"
#include "vulkan/raytracing/RtStaticMeshSlot.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace
{

using horde::gameplay::items::HeldHand;
using horde::gameplay::items::HeldItemId;
using horde::gameplay::items::HeldItemParentMode;
using horde::gameplay::items::HeldItemState;
using horde::gameplay::items::HeldItemTransform;

int failures = 0;

void Check(const bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

bool Near(const float actual, const float expected, const float tolerance = 0.0001f)
{
    return std::abs(actual - expected) <= tolerance;
}

HeldItemTransform Translation(const float x, const float y, const float z)
{
    HeldItemTransform result = horde::gameplay::items::IdentityHeldItemTransform();
    result[12] = x;
    result[13] = y;
    result[14] = z;
    return result;
}

std::array<float, 3u> Add(const std::array<float, 3u>& left,
                          const std::array<float, 3u>& right)
{
    return {{left[0] + right[0], left[1] + right[1], left[2] + right[2]}};
}

std::array<float, 3u> Scale(const std::array<float, 3u>& value, const float scale)
{
    return {{value[0] * scale, value[1] * scale, value[2] * scale}};
}

std::array<float, 3u> Cross(const std::array<float, 3u>& left,
                            const std::array<float, 3u>& right)
{
    return {{left[1] * right[2] - left[2] * right[1],
             left[2] * right[0] - left[0] * right[2],
             left[0] * right[1] - left[1] * right[0]}};
}

std::array<float, 3u> Normalize(const std::array<float, 3u>& value)
{
    const float length = std::sqrt(value[0] * value[0] + value[1] * value[1] +
                                   value[2] * value[2]);
    return Scale(value, 1.0f / length);
}

bool TransformNear(const HeldItemTransform& actual,
                   const HeldItemTransform& expected,
                   const float tolerance = 0.00001f)
{
    for (std::size_t i = 0u; i < actual.size(); ++i)
    {
        if (!Near(actual[i], expected[i], tolerance)) return false;
    }
    return true;
}

HeldItemTransform ExpectedHeldTorchFromFixedSnapshot(
    const horde::gameplay::simulation::SimulationSnapshot& snapshot,
    const HeldItemTransform& itemFromGrip)
{
    const std::array<float, 3u> worldUp{{0.0f, 1.0f, 0.0f}};
    const auto forward = Normalize(std::array<float, 3u>{{
        std::sin(snapshot.playerYawRadians),
        -0.05f + std::clamp(snapshot.playerPitchRadians, -0.32f, 0.28f),
        -std::cos(snapshot.playerYawRadians)}});
    const auto right = Normalize(Cross(forward, worldUp));
    const auto up = Normalize(Cross(right, forward));
    const float movement = std::max(std::clamp(snapshot.walkAmount, 0.0f, 1.0f), 0.2f);
    const float gait = snapshot.walkTime * 6.2f;
    const float sway = std::sin(gait) * 0.035f * movement;
    const float bob = std::abs(std::sin(gait)) * 0.025f * movement;
    const float heldDepth = horde::gameplay::ComputeShowcaseHeldPropDepth(
        snapshot.playerX, snapshot.playerZ,
        std::sin(snapshot.playerYawRadians), -std::cos(snapshot.playerYawRadians));
    const std::array<float, 3u> localHand{{-0.24f - sway, -0.40f + bob, heldDepth}};
    const std::array<float, 3u> eye{{snapshot.playerX, 0.58f, snapshot.playerZ}};
    const auto hand = Add(Add(Add(eye, Scale(right, localHand[0])),
                              Scale(up, localHand[1])),
                          Scale(forward, localHand[2]));
    const auto itemZ = Scale(forward, -1.0f);
    const auto translation = Add(
        Add(Add(hand, Scale(right, -itemFromGrip[12])),
            Scale(up, -itemFromGrip[13])),
        Scale(itemZ, -itemFromGrip[14]));
    return {{right[0], right[1], right[2], 0.0f,
             up[0], up[1], up[2], 0.0f,
             itemZ[0], itemZ[1], itemZ[2], 0.0f,
             translation[0], translation[1], translation[2], 1.0f}};
}

bool LoadProductionHeldAssets(horde::scene::assets::StaticMeshAsset& sword,
                              horde::scene::assets::StaticMeshAsset& torch,
                              std::string& diagnostic)
{
    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    horde::scene::assets::AssetManifest swordManifest;
    horde::scene::assets::AssetManifest torchManifest;
    const auto swordDirectory = root / "assets/models/weapons/runtime";
    const auto torchDirectory = root / "assets/models/props/runtime";
    return horde::scene::assets::AssetManifest::Load(
               swordDirectory / "asset.manifest.json", swordManifest, diagnostic) &&
           horde::scene::assets::StaticMeshAsset::Load(
               swordDirectory / "gothic-arming-sword-rh-lod0.runtime.glb",
               swordManifest, sword, diagnostic) &&
           horde::scene::assets::AssetManifest::Load(
               torchDirectory / "asset.manifest.json", torchManifest, diagnostic) &&
           horde::scene::assets::StaticMeshAsset::Load(
               torchDirectory / "gothic-hand-torch-lod0.runtime.glb",
               torchManifest, torch, diagnostic);
}

void TestSocketLookupIsNamedAndOrderIndependent()
{
    const std::vector<horde::scene::assets::StaticSocket> sockets{
        {"Flame", 3u, Translation(0.0f, 0.8f, 0.0f)},
        {"Grip", 1u, Translation(0.0f, -0.2f, 0.0f)},
        {"Light", 4u, Translation(0.0f, 0.9f, 0.0f)},
    };
    const auto* grip = horde::gameplay::items::FindHeldItemSocket(sockets, "Grip");
    Check(grip != nullptr && grip->nodeTransformIndex == 1u && Near(grip->world[13], -0.2f),
          "Grip lookup must use the exact socket name rather than vector order");
    Check(horde::gameplay::items::FindHeldItemSocket(sockets, "grip") == nullptr,
          "socket names must retain the authored case-sensitive contract");
}

void TestWorldFromItemUsesRequiredCompositionOrder()
{
    HeldItemTransform worldFromRightHand{{
        0.0f, 1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        4.0f, 5.0f, 6.0f, 1.0f}};
    const HeldItemTransform itemFromGrip = Translation(0.0f, -2.0f, 0.0f);

    HeldItemTransform worldFromItem{};
    std::string diagnostic;
    Check(horde::gameplay::items::ComposeWorldFromItem(
              worldFromRightHand, itemFromGrip, worldFromItem, diagnostic),
          "a rigid hand and grip transform must compose");
    Check(Near(worldFromItem[12], 2.0f) && Near(worldFromItem[13], 5.0f) &&
              Near(worldFromItem[14], 6.0f),
          "worldFromItem must equal worldFromHandSocket * inverse(itemFromGrip)");
}

void TestScaledGripSocketIsRejected()
{
    HeldItemTransform scaledGrip = Translation(0.0f, -0.2f, 0.0f);
    scaledGrip[0] = 1.25f;
    std::string diagnostic;
    Check(!horde::gameplay::items::ValidateHeldItemSocketTransform(scaledGrip, diagnostic) &&
              diagnostic == "Held-item socket transform must be rigid and unit scale.",
          "scaled Grip sockets must be rejected before attachment");
}

void TestLeftAndRightHandsCannotBeSwapped()
{
    const HeldItemTransform left = Translation(-3.0f, 1.0f, 2.0f);
    const HeldItemTransform right = Translation(7.0f, 1.0f, 2.0f);
    const auto selectedLeft = horde::gameplay::items::SelectHandSocketTransform(
        HeldHand::LeftHand, left, right);
    const auto selectedRight = horde::gameplay::items::SelectHandSocketTransform(
        HeldHand::RightHand, left, right);
    Check(Near(selectedLeft[12], -3.0f) && Near(selectedRight[12], 7.0f),
          "LeftHand and RightHand must retain distinct transforms");
}

void TestWallRetractionMovesHandAndAttachedItemTogether()
{
    const HeldItemTransform grip = Translation(0.0f, -0.25f, 0.0f);
    const HeldItemTransform nominalHand = Translation(-0.34f, 0.18f, -1.05f);
    const HeldItemTransform retractedHand = Translation(-0.34f, 0.18f, -0.62f);
    HeldItemTransform nominalItem{};
    HeldItemTransform retractedItem{};
    std::string diagnostic;
    Check(horde::gameplay::items::ComposeWorldFromItem(
              nominalHand, grip, nominalItem, diagnostic) &&
              horde::gameplay::items::ComposeWorldFromItem(
                  retractedHand, grip, retractedItem, diagnostic),
          "wall retraction fixtures must compose");
    Check(Near(retractedHand[14] - nominalHand[14], 0.43f) &&
              Near(retractedItem[14] - nominalItem[14], 0.43f) &&
              Near(retractedItem[13] - retractedHand[13], 0.25f),
          "wall retraction must move the shared hand target, preserving Grip attachment");
}

void TestRealFixedTickTorchDetachIsIndependentlyTransformContinuous()
{
    horde::scene::assets::StaticMeshAsset sword;
    horde::scene::assets::StaticMeshAsset torchAsset;
    std::string diagnostic;
    Check(LoadProductionHeldAssets(sword, torchAsset, diagnostic),
          "production held assets must load for the fixed-tick detach contract");
    const auto* grip = horde::gameplay::items::FindHeldItemSocket(torchAsset.sockets, "Grip");
    Check(grip != nullptr, "production torch Grip must exist for detach continuity");
    if (grip == nullptr) return;

    horde::gameplay::simulation::GameSimulation simulation;
    horde::gameplay::simulation::InputSnapshot input;
    input.hasAuthoritativePlayerPose = true;
    input.authoritativePlayerX = -2.16f;
    input.authoritativePlayerZ = -15.14f;
    input.yawRadians = 0.43f;
    input.pitchRadians = 0.17f;

    horde::gameplay::simulation::SimulationSnapshot beforeRelease{};
    horde::gameplay::simulation::SimulationSnapshot released{};
    for (std::size_t step = 0u; step < 80u; ++step)
    {
        const auto before = simulation.Snapshot();
        simulation.StepFixed(input);
        const auto after = simulation.Snapshot();
        if (after.heldItems[0].detached)
        {
            beforeRelease = before;
            released = after;
            break;
        }
    }

    Check(released.heldItems[0].detached &&
              released.heldItems[0].parentMode == HeldItemParentMode::AuthoredWorldTrajectory &&
              released.heldItems[0].detachTick == released.tickIndex,
          "the real torch failure sequence must detach once on its shared fixed tick");
    const HeldItemTransform expectedHeld = ExpectedHeldTorchFromFixedSnapshot(
        beforeRelease, grip->world);
    Check(std::abs(expectedHeld[12] - (-2.16f - 0.24f)) > 0.001f &&
              std::abs(expectedHeld[13] - (0.58f - 0.40f)) > 0.001f,
          "the independent pre-release fixture must contain real pitch plus sway/bob");
    Check(TransformNear(beforeRelease.heldItems[0].worldFromItem, expectedHeld),
          "shared fixed-step state must own the independently derived held torch matrix");
    Check(TransformNear(released.heldItems[0].worldFromItem,
                        beforeRelease.heldItems[0].worldFromItem, 0.000001f) &&
              TransformNear(released.heldItems[0].worldFromDetach,
                            beforeRelease.heldItems[0].worldFromItem, 0.000001f),
          "the first independently resolved released matrix must exactly equal the last held matrix");

    const std::uint64_t detachTick = released.heldItems[0].detachTick;
    simulation.StepFixed(input);
    Check(simulation.Snapshot().heldItems[0].detachTick == detachTick,
          "subsequent authored trajectory ticks must not detach the torch again");
}

void TestSharedFixedStepOwnsItemsKinematicsAndSocketLight()
{
    horde::scene::assets::StaticMeshAsset swordAsset;
    horde::scene::assets::StaticMeshAsset torchAsset;
    std::string diagnostic;
    Check(LoadProductionHeldAssets(swordAsset, torchAsset, diagnostic),
          "production held assets must load for shared ownership checks");
    const auto* flame = horde::gameplay::items::FindHeldItemSocket(torchAsset.sockets, "Flame");
    const auto* light = horde::gameplay::items::FindHeldItemSocket(torchAsset.sockets, "Light");
    Check(flame != nullptr && light != nullptr,
          "production torch Flame and Light sockets must exist for shared ownership checks");
    if (flame == nullptr || light == nullptr) return;

    horde::gameplay::simulation::GameSimulation simulation;
    horde::gameplay::simulation::InputSnapshot input;
    input.yawRadians = -0.37f;
    input.pitchRadians = 0.11f;
    input.moveForward = 0.72f;
    for (std::size_t tick = 0u; tick < 9u; ++tick) simulation.StepFixed(input);
    const auto& snapshot = simulation.Snapshot();
    Check(!TransformNear(snapshot.heldItems[0].worldFromItem,
                         horde::gameplay::items::IdentityHeldItemTransform()) &&
              !TransformNear(snapshot.heldItems[1].worldFromItem,
                             horde::gameplay::items::IdentityHeldItemTransform()),
          "shared fixed-step snapshots must carry resolved sword and torch world matrices");
    Check(Near(snapshot.heldItemKinematics.leftHandLocal[2],
               snapshot.heldItemKinematics.heldPropDepth),
          "shared snapshots must carry the same wall-aware hand target used for item resolution");
    const HeldItemTransform expectedFlame = horde::gameplay::items::MultiplyHeldItemTransforms(
        snapshot.heldItems[0].worldFromItem, flame->world);
    const HeldItemTransform expectedLight = horde::gameplay::items::MultiplyHeldItemTransforms(
        snapshot.heldItems[0].worldFromItem, light->world);
    Check(TransformNear(snapshot.heldLight.worldFromFlame, expectedFlame) &&
              TransformNear(snapshot.heldLight.worldFromLight, expectedLight) &&
              Near(snapshot.heldLight.flameStrength, snapshot.torchFailure.flameStrength),
          "exact full Flame and Light socket transforms must be immutable shared simulation output");
}

void TestHeldItemSnapshotsAreRenderDeliveryInvariant()
{
    const auto run = [](const std::uint32_t renderRate) {
        horde::gameplay::simulation::GameSimulation simulation;
        horde::gameplay::simulation::InputSnapshot input;
        input.hasAuthoritativePlayerPose = true;
        input.authoritativePlayerX = -2.16f;
        input.authoritativePlayerZ = -15.14f;
        input.yawRadians = 0.43f;
        input.pitchRadians = 0.17f;
        const std::uint32_t frameCount = renderRate * 2u;
        for (std::uint32_t frame = 0u; frame < frameCount; ++frame)
            simulation.AdvanceFrame(input, 1.0 / static_cast<double>(renderRate), frame + 1u);
        return simulation.Snapshot();
    };
    const auto at30 = run(30u);
    const auto at60 = run(60u);
    const auto at120 = run(120u);
    Check(at30.tickIndex == at60.tickIndex && at60.tickIndex == at120.tickIndex &&
              TransformNear(at30.heldItems[0].worldFromItem, at60.heldItems[0].worldFromItem) &&
              TransformNear(at60.heldItems[0].worldFromItem, at120.heldItems[0].worldFromItem) &&
              TransformNear(at30.heldItems[1].worldFromItem, at120.heldItems[1].worldFromItem) &&
              TransformNear(at30.heldLight.worldFromLight, at120.heldLight.worldFromLight),
          "30/60/120 Hz renderer delivery must expose identical fixed-step item and light state");
}

void TestHeldItemBlasMeasurementsIncludeBothProductionAssets()
{
    horde::vulkan::raytracing::HeldItemBlasMeasurements measurements;
    measurements.RecordTorch(4096u, 1.25);
    measurements.RecordSword(8192u, 2.50);
    Check(measurements.torchBytes == 4096u && measurements.swordBytes == 8192u &&
              measurements.TotalBytes() == 12288u &&
              std::abs(measurements.TotalBuildMilliseconds() - 3.75) < 0.000001,
          "production held-item BLAS evidence must sum torch and sword bytes and timings");
}

void TestHeldLightGpuAbiAppendsWithoutChangingReleasedBindings()
{
    Check(horde::vulkan::raytracing::kRtBindingInstanceMetadata == 11u &&
              horde::vulkan::raytracing::kRtBindingEmissiveTextures == 19u &&
              horde::vulkan::raytracing::kRtBindingHeldLight == 20u &&
              sizeof(horde::vulkan::raytracing::RtHeldLightGpu) == 16u,
          "held light GPU state must append at binding 20 without changing bindings 0-19");
}

void TestResetAndCheckpointImportRestoreParentContracts()
{
    auto items = horde::gameplay::items::MakeDefaultHeldItemStates();
    horde::gameplay::items::ImportHeldItemCheckpoint(items, false, 900u);
    Check(items[0].parentMode == HeldItemParentMode::AuthoredWorldTrajectory &&
              items[0].detached && items[0].detachTick == 900u &&
              items[1].parentMode == HeldItemParentMode::HandSocket,
          "post-drop checkpoint import must detach only the original torch");
    horde::gameplay::items::ResetHeldItemStates(items);
    Check(items[0].id == HeldItemId::OriginalTorch &&
              items[0].parentMode == HeldItemParentMode::HandSocket && !items[0].detached &&
              items[1].id == HeldItemId::Sword &&
              items[1].parentMode == HeldItemParentMode::HandSocket,
          "route reset must restore both canonical held-item attachments");
}

void TestRenderSlotConvertsGenericTransformWithoutItemBranches()
{
    HeldItemState sword = horde::gameplay::items::MakeHeldItemState(
        HeldItemId::Sword, HeldHand::RightHand);
    sword.worldFromItem = Translation(2.0f, 3.0f, 4.0f);
    const auto instance = horde::vulkan::raytracing::HeldItemRenderSlot::BuildInstanceTransform(sword);
    Check(Near(instance[3], 2.0f) && Near(instance[7], 3.0f) && Near(instance[11], 4.0f),
          "generic held-item render slot must preserve matrix translation in Vulkan 3x4 order");
}

void TestFlameAndLightSocketsFollowTheComposedItem()
{
    const HeldItemTransform worldFromTorch = Translation(10.0f, 2.0f, -4.0f);
    const HeldItemTransform itemFromFlame = Translation(0.0f, 0.7f, 0.0f);
    const HeldItemTransform itemFromLight = Translation(0.0f, 0.8f, 0.1f);
    horde::gameplay::items::HeldLightState light{};
    std::string diagnostic;
    Check(horde::gameplay::items::ComposeHeldLightState(
              worldFromTorch, itemFromFlame, itemFromLight, 0.65f, light, diagnostic),
          "rigid Flame and Light sockets must compose from the generic item transform");
    Check(Near(light.worldFromFlame[12], 10.0f) && Near(light.worldFromFlame[13], 2.7f) &&
              Near(light.worldFromLight[13], 2.8f) && Near(light.worldFromLight[14], -3.9f) &&
              Near(light.flameStrength, 0.65f) && light.active,
          "engine flame and light state must follow authored sockets without flame geometry");
}

void TestSimulationOwnsResetAndCheckpointParentState()
{
    horde::gameplay::simulation::GameSimulation simulation;
    Check(simulation.Snapshot().heldItems[0].parentMode == HeldItemParentMode::HandSocket &&
              simulation.Snapshot().heldItems[1].parentMode == HeldItemParentMode::HandSocket,
          "fresh shared simulation must author both held attachments");
    Check(simulation.ApplyShowcaseCheckpoint(4),
          "the original post-drop showcase checkpoint must import");
    Check(simulation.Snapshot().heldItems[0].parentMode ==
              HeldItemParentMode::AuthoredWorldTrajectory &&
              simulation.Snapshot().heldItems[0].detached &&
              simulation.Snapshot().heldItems[1].parentMode == HeldItemParentMode::HandSocket,
          "post-drop checkpoint must import torch parent state through shared simulation");
    simulation.ResetRoute();
    Check(simulation.Snapshot().heldItems[0].parentMode == HeldItemParentMode::HandSocket &&
              !simulation.Snapshot().heldItems[0].detached,
          "shared route reset must reattach the original torch");
}

void TestSharedKinematicsOwnsWallDepthHandsAndSwordPose()
{
    horde::gameplay::items::HeldItemKinematicsInput input;
    input.cameraX = 0.0f;
    input.cameraZ = 1.85f;
    input.cameraYawRadians = 0.0f;
    input.walkTime = 0.0f;
    input.walkAmount = 0.0f;
    const auto idle = horde::gameplay::items::EvaluateHeldItemKinematics(input);
    Check(Near(idle.heldPropDepth, 0.975f) &&
              Near(idle.leftHandLocal[0], -0.24f) && Near(idle.leftHandLocal[1], -0.40f) &&
              Near(idle.leftHandLocal[2], idle.heldPropDepth) &&
              Near(idle.rightHandLocal[0], 0.25f) && Near(idle.rightHandLocal[1], -0.41f) &&
              Near(idle.rightHandLocal[2], idle.heldPropDepth),
          "shared kinematics must own the safe-frame wall-aware idle hand targets");

    input.torchFailure.leftArmLowerBlend = 1.0f;
    input.playerCombat.action = horde::gameplay::PlayerCombatAction::ParryActive;
    const auto loweredAndParrying = horde::gameplay::items::EvaluateHeldItemKinematics(input);
    Check(Near(loweredAndParrying.leftHandLocal[0], -0.36f) &&
              Near(loweredAndParrying.leftHandLocal[1], -0.92f) &&
              Near(loweredAndParrying.leftHandLocal[2], 0.27f) &&
              Near(loweredAndParrying.rightHandLocal[0], -0.16f) &&
              Near(loweredAndParrying.swordRadians, -0.62f),
          "torch lowering and sword parry must share the authored hand-target evaluator");
}

void TestRewardLanternHighLowUsesSharedLeftArmTarget()
{
    using namespace horde::gameplay::interactions;
    horde::gameplay::items::HeldItemKinematicsInput input{};
    input.cameraX = -11.0f;
    input.cameraZ = -15.2f;
    input.cameraYawRadians = -1.57079632679f;
    input.torchFailure.heldByPlayer = false;
    input.torchFailure.leftArmLowerBlend = 1.0f;
    input.interaction.heldLightKind = HeldLightKind::RewardLantern;
    input.interaction.heldLightPose = HeldLightPose::High;
    input.interaction.heldLightPoseProgress = 1.0f;
    const auto high = horde::gameplay::items::EvaluateHeldItemKinematics(input);

    input.interaction.heldLightPose = HeldLightPose::Low;
    const auto low = horde::gameplay::items::EvaluateHeldItemKinematics(input);
    input.interaction.heldLightPose = HeldLightPose::TransitioningToLow;
    input.interaction.heldLightPoseProgress = 0.5f;
    const auto midpoint = horde::gameplay::items::EvaluateHeldItemKinematics(input);

    Check(high.leftHandLocal[0] < -0.04f && high.leftHandLocal[0] > -0.18f &&
              low.leftHandLocal[0] < -0.04f && low.leftHandLocal[0] > -0.18f,
          "reward high/low targets must preserve the accepted inward phone-arm placement");
    Check(high.leftHandLocal[1] > 0.45f &&
              low.leftHandLocal[1] <= high.leftHandLocal[1] - 0.22f &&
              low.leftHandLocal[1] > 0.20f,
          "reward high/low carry must visibly raise and lower the real left arm without using the failed-torch pose");
    for (std::size_t axis = 0u; axis < midpoint.leftHandLocal.size(); ++axis)
    {
        Check(Near(midpoint.leftHandLocal[axis],
                   0.5f * (high.leftHandLocal[axis] + low.leftHandLocal[axis]), 0.0002f),
              "the 0.65 second high/low transition must remain continuous in shared kinematics");
    }
    horde::gameplay::items::HeldItemKinematicsInput wallInput = input;
    wallInput.cameraX = 0.0f;
    wallInput.cameraZ = -9.70f;
    wallInput.cameraYawRadians = 0.0f;
    const auto wall = horde::gameplay::items::EvaluateHeldItemKinematics(wallInput);
    std::cout << "reward carry open held/high/low x="
              << high.heldPropDepth << '/' << high.leftHandLocal[2] << '/'
              << low.leftHandLocal[2] << '/' << high.leftHandLocal[0]
              << " wall held/hand/x=" << wall.heldPropDepth << '/'
              << wall.leftHandLocal[2] << '/' << wall.leftHandLocal[0] << '\n';
    Check(Near(high.heldPropDepth, low.heldPropDepth) &&
              high.leftHandLocal[2] >= high.heldPropDepth + 1.10f &&
              high.leftHandLocal[2] <= high.heldPropDepth + 1.25f &&
              std::abs(high.leftHandLocal[2] - low.leftHandLocal[2]) <= 0.002f &&
              wall.leftHandLocal[2] >= -0.03f &&
              wall.leftHandLocal[2] <= 0.01f &&
              wall.leftHandLocal[0] <= -0.30f &&
              wall.leftHandLocal[2] < low.leftHandLocal[2] - 1.50f,
          "high/low carry must share the portrait-readable open advance and collapse to the wall-safe depth/lateral target");

    float previousDepth = high.leftHandLocal[2];
    float previousLateral = high.leftHandLocal[0];
    for (int step = 1; step <= 16; ++step)
    {
        auto approachInput = input;
        approachInput.interaction.heldLightPose = HeldLightPose::High;
        approachInput.interaction.heldLightPoseProgress = 1.0f;
        approachInput.cameraX = 0.0f;
        approachInput.cameraZ = -8.50f - static_cast<float>(step) * 0.075f;
        approachInput.cameraYawRadians = 0.0f;
        const auto approach =
            horde::gameplay::items::EvaluateHeldItemKinematics(approachInput);
        Check(approach.leftHandLocal[2] <= previousDepth + 0.0002f &&
                  previousDepth - approach.leftHandLocal[2] <= 0.30f &&
                  approach.leftHandLocal[0] <= previousLateral + 0.0002f &&
                  previousLateral - approach.leftHandLocal[0] <= 0.10f,
              "collision-derived reward advance and lateral offset must collapse monotonically without a carry pop");
        previousDepth = approach.leftHandLocal[2];
        previousLateral = approach.leftHandLocal[0];
    }
}

void TestProductionSwordAssetMeetsGenericSocketAndPbrBudget()
{
    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    const auto directory = root / "assets/models/weapons/runtime";
    horde::scene::assets::AssetManifest manifest;
    horde::scene::assets::StaticMeshAsset asset;
    std::string diagnostic;
    Check(horde::scene::assets::AssetManifest::Load(
              directory / "asset.manifest.json", manifest, diagnostic),
          "production sword manifest must load through the real schema parser");
    Check(horde::scene::assets::StaticMeshAsset::Load(
              directory / "gothic-arming-sword-rh-lod0.runtime.glb",
              manifest,
              asset,
              diagnostic),
          "production sword GLB must load through the generic static PBR reader");
    if (!asset.indices.empty())
    {
        const std::size_t triangles = asset.indices.size() / 3u;
        Check(triangles >= 8000u && triangles <= 12500u && asset.materials.size() <= 2u,
              "production sword must stay inside the approved runtime triangle/material budget");
        const auto* grip = horde::gameplay::items::FindHeldItemSocket(asset.sockets, "Grip");
        Check(grip != nullptr &&
                  horde::gameplay::items::ValidateHeldItemSocketTransform(
                      grip->world, diagnostic),
              "production sword must retain an exact rigid Grip socket");
        Check(asset.materials[0].emissiveTexture < 0 &&
                  asset.materials[0].emissiveFactor == std::array<float, 3u>{},
              "production sword must not carry emissive or magical material data");
    }
}

void TestProductionTorchAssetMeetsGenericSocketAndPbrBudget()
{
    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    const auto directory = root / "assets/models/props/runtime";
    horde::scene::assets::AssetManifest manifest;
    horde::scene::assets::StaticMeshAsset asset;
    std::string diagnostic;
    Check(horde::scene::assets::AssetManifest::Load(
              directory / "asset.manifest.json", manifest, diagnostic),
          "production torch manifest must load through the real schema parser");
    Check(horde::scene::assets::StaticMeshAsset::Load(
              directory / "gothic-hand-torch-lod0.runtime.glb",
              manifest,
              asset,
              diagnostic),
          "production torch GLB must load through the generic static PBR reader");
    if (!asset.indices.empty())
    {
        const std::size_t triangles = asset.indices.size() / 3u;
        Check(triangles >= 3000u && triangles <= 6000u && asset.materials.size() <= 2u,
              "production torch must stay inside the approved runtime triangle/material budget");
        for (const char* socketName : {"Grip", "Flame", "Light"})
        {
            const auto* socket = horde::gameplay::items::FindHeldItemSocket(asset.sockets, socketName);
            Check(socket != nullptr &&
                      horde::gameplay::items::ValidateHeldItemSocketTransform(
                          socket->world, diagnostic),
                  "production torch must retain exact rigid Grip, Flame, and Light sockets");
        }
        Check(asset.materials[0].emissiveTexture < 0 &&
                  asset.materials[0].emissiveFactor == std::array<float, 3u>{},
              "production torch body must not carry flame geometry or emissive material data");
    }
}

void TestProductionAssetsShareOneGenericStaticSlot()
{
    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    horde::scene::assets::AssetManifest swordManifest;
    horde::scene::assets::AssetManifest torchManifest;
    horde::scene::assets::AssetManifest playerManifest;
    horde::scene::assets::StaticMeshAsset sword;
    horde::scene::assets::StaticMeshAsset torch;
    horde::scene::assets::StaticMeshAsset player;
    std::string diagnostic;
    const auto swordDirectory = root / "assets/models/weapons/runtime";
    const auto torchDirectory = root / "assets/models/props/runtime";
    const auto playerDirectory = root / "assets/models/player/runtime";
    Check(horde::scene::assets::AssetManifest::Load(
              swordDirectory / "asset.manifest.json", swordManifest, diagnostic) &&
              horde::scene::assets::StaticMeshAsset::Load(
                  swordDirectory / "gothic-arming-sword-rh-lod0.runtime.glb",
                  swordManifest, sword, diagnostic) &&
              horde::scene::assets::AssetManifest::Load(
                  torchDirectory / "asset.manifest.json", torchManifest, diagnostic) &&
              horde::scene::assets::StaticMeshAsset::Load(
                  torchDirectory / "gothic-hand-torch-lod0.runtime.glb",
                  torchManifest, torch, diagnostic) &&
              horde::scene::assets::AssetManifest::Load(
                  playerDirectory / "asset.manifest.json", playerManifest, diagnostic) &&
              horde::scene::assets::StaticMeshAsset::Load(
                  playerDirectory / "gothic-traveller-lod0.runtime.glb",
                  playerManifest, player, diagnostic),
          "all three production PBR assets must load before generic slot registration");
    std::array<horde::vulkan::raytracing::StaticRtAssetRegistration, 3u> registrations{{
        {3u, 0x53574f52u,
         static_cast<std::uint32_t>(horde::vulkan::raytracing::RtInstanceFlag::StaticPbr),
         0u, &sword},
        {1u, 0x544f5243u,
         static_cast<std::uint32_t>(horde::vulkan::raytracing::RtInstanceFlag::StaticPbr),
         1u, &torch},
        {4u, 0x504c4159u,
         static_cast<std::uint32_t>(horde::vulkan::raytracing::RtInstanceFlag::StaticPbr),
         0u, &player},
    }};
    horde::vulkan::raytracing::RtStaticMeshSlot slot;
    Check(slot.Initialize(registrations, diagnostic),
          "sword and torch must register through one generic static PBR slot");
    const auto& metadata = slot.InstanceMetadata();
    Check(metadata[3].primitiveCount == sword.primitives.size() &&
              metadata[1].primitiveCount == torch.primitives.size() &&
              metadata[4].primitiveCount == 3u &&
              metadata[1].emitterIndex == 1u &&
              metadata[3].emitterIndex == 0u,
          "generic registrations must retain stable TLAS routes and engine-emitter ownership");
    const auto counts = slot.TextureArrayCounts();
    Check(counts.baseColor == 3u && counts.normal == 3u && counts.orm == 3u &&
              counts.emissive == 0u,
          "generic material routing must include the player in audited shared texture array layers");
}

void TestProductionSocketsMatchSharedFixedStepContracts()
{
    horde::scene::assets::StaticMeshAsset sword;
    horde::scene::assets::StaticMeshAsset torch;
    std::string diagnostic;
    Check(LoadProductionHeldAssets(sword, torch, diagnostic),
          "production GLBs must load before comparing shared socket contracts");
    const auto* swordGrip = horde::gameplay::items::FindHeldItemSocket(sword.sockets, "Grip");
    const auto* torchGrip = horde::gameplay::items::FindHeldItemSocket(torch.sockets, "Grip");
    const auto* flame = horde::gameplay::items::FindHeldItemSocket(torch.sockets, "Flame");
    const auto* light = horde::gameplay::items::FindHeldItemSocket(torch.sockets, "Light");
    Check(swordGrip != nullptr && torchGrip != nullptr && flame != nullptr && light != nullptr &&
              TransformNear(swordGrip->world,
                            horde::gameplay::items::SwordGripSocketTransform()) &&
              TransformNear(torchGrip->world,
                            horde::gameplay::items::OriginalTorchGripSocketTransform()) &&
              TransformNear(flame->world,
                            horde::gameplay::items::OriginalTorchFlameSocketTransform()) &&
              TransformNear(light->world,
                            horde::gameplay::items::OriginalTorchLightSocketTransform()),
          "actual GLB Grip/Flame/Light transforms must exactly match shared fixed-step contracts");
}

} // namespace

int main()
{
    TestSocketLookupIsNamedAndOrderIndependent();
    TestWorldFromItemUsesRequiredCompositionOrder();
    TestScaledGripSocketIsRejected();
    TestLeftAndRightHandsCannotBeSwapped();
    TestWallRetractionMovesHandAndAttachedItemTogether();
    TestRealFixedTickTorchDetachIsIndependentlyTransformContinuous();
    TestSharedFixedStepOwnsItemsKinematicsAndSocketLight();
    TestHeldItemSnapshotsAreRenderDeliveryInvariant();
    TestHeldItemBlasMeasurementsIncludeBothProductionAssets();
    TestHeldLightGpuAbiAppendsWithoutChangingReleasedBindings();
    TestResetAndCheckpointImportRestoreParentContracts();
    TestRenderSlotConvertsGenericTransformWithoutItemBranches();
    TestFlameAndLightSocketsFollowTheComposedItem();
    TestSimulationOwnsResetAndCheckpointParentState();
    TestSharedKinematicsOwnsWallDepthHandsAndSwordPose();
    TestRewardLanternHighLowUsesSharedLeftArmTarget();
    TestProductionSwordAssetMeetsGenericSocketAndPbrBudget();
    TestProductionTorchAssetMeetsGenericSocketAndPbrBudget();
    TestProductionAssetsShareOneGenericStaticSlot();
    TestProductionSocketsMatchSharedFixedStepContracts();
    if (failures == 0)
    {
        std::cout << "Held-item socket contracts passed.\n";
    }
    return failures == 0 ? 0 : 1;
}
