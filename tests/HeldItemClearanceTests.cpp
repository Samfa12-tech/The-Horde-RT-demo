#include "gameplay/items/HeldItemKinematics.h"
#include "scene/ShowcaseOverheadGeometry.h"
#include "scene/assets/StaticMeshAsset.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

namespace
{
using namespace horde::gameplay;
using namespace horde::gameplay::items;
using Vec3 = std::array<float, 3u>;
int failures = 0;
horde::scene::assets::StaticMeshAsset playerTorch;
HeldItemTransform playerFlameSocket{};
HeldItemTransform playerLightSocket{};

void Check(bool condition, const char* message)
{
    if (!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}

Vec3 TransformPoint(const HeldItemTransform& transform, Vec3 point)
{
    Vec3 result{};
    for (std::size_t axis = 0u; axis < 3u; ++axis)
        result[axis] = transform[12u + axis] + transform[axis] * point[0] +
            transform[4u + axis] * point[1] + transform[8u + axis] * point[2];
    return result;
}

float Distance(Vec3 a, Vec3 b)
{
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) +
                     (a[1] - b[1]) * (a[1] - b[1]) +
                     (a[2] - b[2]) * (a[2] - b[2]));
}

bool InsideFootprint(const horde::scene::OverheadVolume& volume, Vec3 point)
{
    int firstSide = 0;
    for (std::size_t edge = 0u; edge < 4u; ++edge)
    {
        const auto& a = volume.footprint[edge];
        const auto& b = volume.footprint[(edge + 1u) % 4u];
        const float cross = (b[0] - a[0]) * (point[2] - a[1]) -
                            (b[1] - a[1]) * (point[0] - a[0]);
        const int side = cross < -0.00001f ? -1 : cross > 0.00001f ? 1 : 0;
        if (side != 0 && firstSide != 0 && side != firstSide) return false;
        if (side != 0) firstSide = side;
    }
    return true;
}

bool PointHasClearance(Vec3 point)
{
    const auto clear = [&](const auto& volumes) {
        for (const auto& volume : volumes)
            if (InsideFootprint(volume, point) &&
                point[1] > volume.bottomY +
                    volume.bottomGradientXZ[0] * (point[0] - volume.bottomAnchorXZ[0]) +
                    volume.bottomGradientXZ[1] * (point[2] - volume.bottomAnchorXZ[1]) -
                    kHeldTorchOverheadGap + 0.00001f)
                return false;
        return true;
    };
    return clear(horde::scene::kShowcaseLowOverheadVolumes) &&
           clear(horde::scene::kShowcaseCeilingPatches) &&
           clear(horde::scene::kShowcaseImportedOverheadVolumes) &&
           clear(std::array{horde::scene::kShowcaseCollapseRoofSeam});
}

bool FullTorchEnvelopeHasClearance(const HeldItemFixedStepState& state)
{
    // Import the actual player asset bounds, independently of the kinematics
    // constants. The Keeper's former sockets remain a separate asset contract.
    HeldItemTransform worldFromItem{};
    std::string diagnostic;
    const auto* grip = FindHeldItemSocket(playerTorch.sockets, "Grip");
    if (grip == nullptr || !ComposeWorldFromItem(state.worldFromLeftHand,
            grip->world, worldFromItem, diagnostic)) return false;
    for (float x : {playerTorch.bounds.minimum[0], playerTorch.bounds.maximum[0]})
        for (float z : {playerTorch.bounds.minimum[2], playerTorch.bounds.maximum[2]})
            for (float y : {playerTorch.bounds.minimum[1], playerTorch.bounds.maximum[1]})
                if (!PointHasClearance(TransformPoint(worldFromItem, {{x, y, z}})))
                    return false;
    // Diagonal points remain inside the admitted .15 m circular radius.
    for (float x : {-0.105f, 0.105f})
        for (float z : {-0.105f, 0.105f})
            for (float y : {0.0f, 0.4f})
                if (!PointHasClearance(TransformPoint(state.light.worldFromFlame, {{x, y, z}})))
                    return false;
    return true;
}

void TestPortalApproachesAndLookAngles()
{
    bool clear = true;
    bool coherent = true;
    float maximumLowering = 0.0f;
    std::uint64_t cases = 0u;
    for (const auto profile : {PlayerMountProfile::LegacyViewRelative, PlayerMountProfile::AnatomicalBody})
        for (float yaw : {-3.14159265f, -1.5707963f, 0.0f, 1.5707963f})
            for (float pitch : {-0.32f, 0.0f, 0.28f})
                for (int portal = 0; portal < 4; ++portal)
                    for (int step = 0; step <= 100; ++step)
                    {
                        HeldItemFixedStepInput input;
                        input.playerMountProfile = profile;
                        input.playerYawRadians = yaw;
                        input.playerPitchRadians = pitch;
                        input.walkTime = step / 60.0f;
                        input.walkAmount = 1.0f;
                        input.playerX = portal == 2 ? -28.3f - 0.025f * step : 0.0f;
                        input.playerZ = portal == 3 ? 1.95f + 0.0121f * step : portal == 2 ? -15.2f :
                            (portal == 0 ? -2.0f : -5.2f) - 0.025f * step;
                        input.playerCombat.action = step % 2 == 0
                            ? PlayerCombatAction::SwingActive : PlayerCombatAction::ParryActive;
                        input.playerCombat.actionTime = 0.04f;
                        auto items = MakeDefaultHeldItemStates();
                        HeldItemFixedStepState state;
                        std::string diagnostic;
                        coherent &= ResolveHeldItemsFixedStep(items, input, ++cases, state, diagnostic);
                        clear &= FullTorchEnvelopeHasClearance(state);
                        maximumLowering = std::max(maximumLowering, state.kinematics.torchOverheadLowering);
                        const auto flame = TransformPoint(items[0].worldFromItem,
                            {{playerFlameSocket[12], playerFlameSocket[13], playerFlameSocket[14]}});
                        const auto light = TransformPoint(items[0].worldFromItem,
                            {{playerLightSocket[12], playerLightSocket[13], playerLightSocket[14]}});
                        coherent &= Distance(flame, TransformPoint(state.light.worldFromFlame, {})) < 0.00001f &&
                                    Distance(light, TransformPoint(state.light.worldFromLight, {})) < 0.00001f;
                    }
    Check(clear, "torch mesh and complete tilted flame envelope must clear three low portals and the imported roof transition at the unchanged route limit");
    Check(coherent, "clearance hand/item/flame/light must retain exact shared socket composition through attack and parry");
    Check(maximumLowering > 0.4f && maximumLowering < 0.8f,
          "low-lintel response must be real bounded carry lowering");
    std::cout << "Portal cases=" << cases << " maximum world-down carry=" << maximumLowering << " m\n";
}

void TestContinuousWalkAndLookResponse()
{
    float maximumStep = 0.0f;
    bool clear = true;
    Vec3 previous{};
    float previousRetraction = 0.0f;
    float previousLowering = 0.0f;
    int maximumStepIndex = 0;
    float maximumStepRetraction = 0.0f;
    float maximumStepLowering = 0.0f;
    HeldItemFixedStepInput maximumStepInput{};
    float previousStepRetraction = 0.0f;
    float previousStepLowering = 0.0f;
    float previousStepZ = 0.0f;
    float previousStepYaw = 0.0f;
    float previousStepPitch = 0.0f;
    for (int step = 0; step <= 400; ++step)
    {
        HeldItemFixedStepInput input;
        input.playerMountProfile = PlayerMountProfile::AnatomicalBody;
        input.playerZ = -1.5f - 0.01f * step;
        input.playerPitchRadians = 0.28f * std::sin(step * 0.015f);
        input.playerYawRadians = 0.30f * std::sin(step * 0.02f);
        input.walkTime = step / 60.0f;
        input.walkAmount = 1.0f;
        auto items = MakeDefaultHeldItemStates();
        HeldItemFixedStepState state;
        std::string diagnostic;
        Check(ResolveHeldItemsFixedStep(items, input, step, state, diagnostic), "walking carry must resolve");
        const Vec3 hand = TransformPoint(state.worldFromLeftHand, {});
        if (step != 0) {
            const float distance = Distance(hand, previous);
            if (distance > maximumStep) {
                maximumStep = distance;
                maximumStepIndex = step;
                maximumStepRetraction = state.kinematics.torchOverheadRetraction;
                maximumStepLowering = state.kinematics.torchOverheadLowering;
                maximumStepInput = input;
                previousStepRetraction = previousRetraction;
                previousStepLowering = previousLowering;
                previousStepZ = input.playerZ + 0.01f;
                previousStepYaw = 0.30f * std::sin((step - 1) * 0.02f);
                previousStepPitch = 0.28f * std::sin((step - 1) * 0.015f);
            }
        }
        previous = hand;
        previousRetraction = state.kinematics.torchOverheadRetraction;
        previousLowering = state.kinematics.torchOverheadLowering;
        clear &= FullTorchEnvelopeHasClearance(state);
    }
    Check(clear, "continuous walk/look sweep must retain full overhead clearance");
    Check(maximumStep < 0.04f, "one fixed walking/look step must not pop the shared hand target");
    std::cout << "Maximum sampled walk/look hand step=" << maximumStep << " m at step "
              << maximumStepIndex << " previous z/yaw/pitch=" << previousStepZ << '/'
              << previousStepYaw << '/' << previousStepPitch << " current="
              << maximumStepInput.playerZ << '/' << maximumStepInput.playerYawRadians << '/'
              << maximumStepInput.playerPitchRadians << " retreat/lowering="
              << previousStepRetraction << '/' << previousStepLowering << " -> "
              << maximumStepRetraction << '/' << maximumStepLowering << '\n';
}

void TestOwnershipAndLoweredPoses()
{
    HeldItemKinematicsInput input;
    // The high grip is just entering the lintel footprint, not already beyond
    // its far face. The low grip is camera-side and needs at most residual care.
    input.cameraZ = -2.85f;
    input.playerMountProfile = PlayerMountProfile::AnatomicalBody;
    const auto held = EvaluateHeldItemKinematics(input);
    input.torchFailure.leftArmLowerBlend = 1.0f;
    const auto lowered = EvaluateHeldItemKinematics(input);
    std::cout << "Rag torch held/lowered residual=" << held.torchOverheadLowering << '/'
              << lowered.torchOverheadLowering << " m; lowered hand Y=" << lowered.leftHandLocal[1] << '\n';
    const float ragEnvelopeGrowth = kPlayerRagTorchEnvelopeTopFromGrip - kHeldTorchEnvelopeTopFromGrip;
    Check(held.torchOverheadLowering > 0.3f && lowered.torchOverheadLowering < 0.08f + ragEnvelopeGrowth &&
          lowered.leftHandLocal[1] <= -0.8199f,
          "already lowered torch must retain its authored lowering with only residual safety clearance");
    input.torchFailure.heldByPlayer = false;
    Check(EvaluateHeldItemKinematics(input).torchOverheadLowering == 0.0f,
          "dropped torch must retain world-trajectory ownership");
    input.interaction.heldLightKind = interactions::HeldLightKind::RewardLantern;
    input.interaction.heldLightPose = interactions::HeldLightPose::High;
    Check(EvaluateHeldItemKinematics(input).torchOverheadLowering == 0.0f,
          "reward lantern must retain its separate cage/pendulum carry contract");
}

void TestSwordBreathingAndCombatBoundaries()
{
    PlayerCombatSnapshot combat;
    const auto rest = EvaluateHeldSwordPose(combat, 0.0f, 0.65f, false, 0.0f);
    const auto breath = EvaluateHeldSwordPose(combat, 0.0f, 0.65f, false, 1.0f);
    Check(Distance(rest.rightHandLocal, breath.rightHandLocal) > 0.003f &&
          Distance(rest.rightHandLocal, breath.rightHandLocal) < 0.005f &&
          rest.swordRadians == breath.swordRadians && rest.swordForwardRadians == breath.swordForwardRadians,
          "idle breathing must visibly move only the shared rest hand by millimetres");
    for (const auto action : {PlayerCombatAction::SwingActive, PlayerCombatAction::UpwardSliceActive,
                              PlayerCombatAction::ParryActive})
    {
        combat.action = action;
        combat.actionTime = 0.04f;
        const auto zero = EvaluateHeldSwordPose(combat, 0.0f, 0.65f, false, 0.0f);
        const auto moving = EvaluateHeldSwordPose(combat, 0.0f, 0.65f, false, 1.0f);
        Check(zero.rightHandLocal == moving.rightHandLocal && zero.swordRadians == moving.swordRadians &&
              zero.swordForwardRadians == moving.swordForwardRadians,
              "breathing must leave authored active cut and guard trajectories exact");
    }
    combat.action = PlayerCombatAction::Idle;
    const auto idle = EvaluateHeldSwordPose(combat, 0.0f, 0.65f, false, 1.0f);
    combat.action = PlayerCombatAction::SwingWindup;
    combat.actionTime = 0.0f;
    Check(EvaluateHeldSwordPose(combat, 0.0f, 0.65f, false, 1.0f).rightHandLocal == idle.rightHandLocal,
          "attack startup must depart continuously from breathing rest");
    combat.action = PlayerCombatAction::SwingRecovery;
    combat.actionTime = SwordCombat::kSwingRecoveryDuration;
    Check(Distance(EvaluateHeldSwordPose(combat, 0.0f, 0.65f, false, 1.0f).rightHandLocal,
                   idle.rightHandLocal) < 0.00001f, "recovery must return continuously to breathing rest");
}
} // namespace

int main()
{
    const auto directory = std::filesystem::path(HORDE_RT_SOURCE_DIR) /
        "assets/models/props/runtime/player-rag-torch";
    horde::scene::assets::AssetManifest manifest;
    std::string diagnostic;
    if (!horde::scene::assets::AssetManifest::Load(directory / "asset.manifest.json",
            manifest, diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(
            directory / "rag-torch-player-lod0.runtime.glb", manifest, playerTorch, diagnostic))
    {
        std::cerr << "FAIL: actual player Rag torch must import: " << diagnostic << '\n';
        return 1;
    }
    const auto* flame = FindHeldItemSocket(playerTorch.sockets, "Flame");
    const auto* light = FindHeldItemSocket(playerTorch.sockets, "Light");
    if (flame == nullptr || light == nullptr)
    {
        std::cerr << "FAIL: actual player Rag torch must have Flame and Light sockets\n";
        return 1;
    }
    playerFlameSocket = flame->world;
    playerLightSocket = light->world;
    TestPortalApproachesAndLookAngles();
    TestContinuousWalkAndLookResponse();
    TestOwnershipAndLoweredPoses();
    TestSwordBreathingAndCombatBoundaries();
    return failures == 0 ? 0 : 1;
}
