#include "gameplay/ShowcaseRoute.h"
#include "gameplay/interactions/InteractionState.h"
#include "gameplay/items/LanternPendulum.h"
#include "gameplay/interactions/FinaleSequence.h"
#include "gameplay/items/HeldItemKinematics.h"
#include "gameplay/simulation/DevelopmentSupportFixture.h"
#include "gameplay/simulation/GameSimulation.h"
#include "scene/assets/AssetManifest.h"
#include "scene/assets/StaticMeshAsset.h"
#include "scene/RescueBlockoutGeometry.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"
#include "vulkan/raytracing/RescuePlayerRig.h"
#include "vulkan/raytracing/SimulationFrameAdapter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>

namespace
{
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
using namespace horde::gameplay::traversal;
using namespace horde::gameplay::items;
using namespace horde::vulkan::raytracing;
using Transform = HeldItemTransform;
using Vec3 = std::array<float, 3u>;

int cases = 0;
int failures = 0;
float maximumLeftReach = 0.0f;
float maximumRightReach = 0.0f;
Phase maximumLeftReachPhase = Phase::LowerSafe;
Phase maximumRightReachPhase = Phase::LowerSafe;

void Check(const bool condition, const char* message)
{
    ++cases;
    if (!condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

float Distance(const Vec3& a, const Vec3& b)
{
    return std::hypot(std::hypot(a[0] - b[0], a[1] - b[1]), a[2] - b[2]);
}

bool Finite(const Transform& transform)
{
    return std::all_of(transform.begin(), transform.end(),
                       [](const float v) { return std::isfinite(v); });
}

std::filesystem::path FindRepoRoot()
{
    auto candidate = std::filesystem::current_path();
    for (int depth = 0; depth < 8; ++depth)
    {
        if (std::filesystem::exists(candidate /
                "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb") &&
            std::filesystem::exists(candidate /
                "assets/models/props/runtime/reward-lantern-ring/asset.manifest.json"))
            return candidate;
        if (!candidate.has_parent_path()) break;
        candidate = candidate.parent_path();
    }
    return {};
}

bool LoadStaticAsset(const std::filesystem::path& directory,
                     const std::filesystem::path& glbName,
                     horde::scene::assets::StaticMeshAsset& asset,
                     std::string& diagnostic)
{
    horde::scene::assets::AssetManifest manifest;
    return horde::scene::assets::AssetManifest::Load(
               directory / "asset.manifest.json", manifest, diagnostic) &&
        horde::scene::assets::StaticMeshAsset::Load(
               directory / glbName, manifest, asset, diagnostic);
}

Transform WorldFromAxes(const Vec3& x, const Vec3& y, const Vec3& z,
                        const Vec3& position)
{
    return {{x[0], x[1], x[2], 0.0f,
             y[0], y[1], y[2], 0.0f,
             z[0], z[1], z[2], 0.0f,
             position[0], position[1], position[2], 1.0f}};
}

Vec3 TransformPoint(const Transform& transform, const Vec3& point)
{
    return {{transform[0] * point[0] + transform[4] * point[1] +
                 transform[8] * point[2] + transform[12],
             transform[1] * point[0] + transform[5] * point[1] +
                 transform[9] * point[2] + transform[13],
             transform[2] * point[0] + transform[6] * point[1] +
                 transform[10] * point[2] + transform[14]}};
}

struct Bounds3
{
    Vec3 minimum{{std::numeric_limits<float>::infinity(),
                  std::numeric_limits<float>::infinity(),
                  std::numeric_limits<float>::infinity()}};
    Vec3 maximum{{-std::numeric_limits<float>::infinity(),
                  -std::numeric_limits<float>::infinity(),
                  -std::numeric_limits<float>::infinity()}};
};

Bounds3 WorldBounds(const horde::scene::assets::StaticBounds& bounds,
                    const Transform& transform)
{
    Bounds3 result;
    for (unsigned mask = 0u; mask < 8u; ++mask)
    {
        const Vec3 local{{(mask & 1u) ? bounds.maximum[0] : bounds.minimum[0],
                          (mask & 2u) ? bounds.maximum[1] : bounds.minimum[1],
                          (mask & 4u) ? bounds.maximum[2] : bounds.minimum[2]}};
        const Vec3 world = TransformPoint(transform, local);
        for (std::size_t axis = 0u; axis < 3u; ++axis)
        {
            result.minimum[axis] = std::min(result.minimum[axis], world[axis]);
            result.maximum[axis] = std::max(result.maximum[axis], world[axis]);
        }
    }
    return result;
}

bool Overlaps(const Bounds3& a, const Bounds3& b)
{
    for (std::size_t axis = 0u; axis < 3u; ++axis)
        if (a.maximum[axis] <= b.minimum[axis] || b.maximum[axis] <= a.minimum[axis])
            return false;
    return true;
}

Bounds3 BoxBounds(const horde::scene::RescueBlockoutBox& box)
{
    return {box.minimum, box.maximum};
}

bool MakeRescueRigPose(const RtSceneFrameInputs& frame,
                       PlayerRenderSlot& slot,
                       horde::gameplay::animation::PlayerAnimationSnapshot& animation,
                       PlayerModelWorldBasis& modelBasis,
                       Vec3& rootWorld,
                       std::string& diagnostic)
{
    const auto& rescue = frame.rescue;
    const float yaw = rescue.bodyYawRadians;
    const Vec3 bodyForward{{std::sin(yaw), 0.0f, -std::cos(yaw)}};
    const Vec3 bodyRight{{std::cos(yaw), 0.0f, std::sin(yaw)}};
    modelBasis = BuildPlayerModelWorldBasis(bodyRight, bodyForward);

    // The rescue world-body renderer is rooted on the gameplay support plane;
    // camera eye height is the established support-relative 1.65 m.
    const Vec3 eye{{frame.cameraX,
                    PlayerEyeWorldY(frame.playerSupportWorldY),
                    frame.cameraZ}};
    rootWorld = {{frame.cameraX,
                  frame.playerSupportWorldY +
                      slot.BootGroundingOffsetMetres(frame.playerAnimation),
                  frame.cameraZ}};
    animation = frame.playerAnimation;
    animation.locomotionClip = horde::gameplay::animation::PlayerLocomotionClip::Idle;
    animation.locomotionBlend = 0.0f;
    animation.locomotionTime = 0.0f;
    if (!ApplyRescueRopeRigTargets(animation, rescue, modelBasis,
                                   rootWorld, eye))
    {
        diagnostic = "Rescue snapshot did not produce finite imported-rig targets.";
        return false;
    }
    return true;
}

Transform WorldGripFromSnapshot(const RescueTraversalSnapshot& rescue,
                                const std::size_t hand)
{
    const float yaw = rescue.bodyYawRadians;
    const Vec3 right{{std::cos(yaw), 0.0f, std::sin(yaw)}};
    const Vec3 forward{{std::sin(yaw), 0.0f, -std::cos(yaw)}};
    const Vec3 gripZ{{-forward[0], -forward[1], -forward[2]}};
    const Vec3 up{{0.0f, 1.0f, 0.0f}};
    const auto& target = rescue.grippingHandTargets[hand];
    return WorldFromAxes(right, up, gripZ, {{target.x, target.y, target.z}});
}

bool ResolveRigAndAttachments(const SimulationSnapshot& source,
                              const std::uint64_t tick,
                              PlayerRenderSlot& slot,
                              const horde::scene::assets::StaticMeshAsset& rewardRing,
                              const horde::scene::assets::StaticMeshAsset& rewardBody,
                              std::string& diagnostic)
{
    const auto frame = BuildRtSceneFrameInputs(source, 0.92f);
    auto lookChangedFrame = frame;
    lookChangedFrame.cameraYaw = frame.cameraYaw + 1.7f;
    lookChangedFrame.cameraPitch = std::clamp(frame.cameraPitch + 0.22f, -0.32f, 0.28f);

    horde::gameplay::animation::PlayerAnimationSnapshot ropePose;
    PlayerModelWorldBasis basis;
    Vec3 root{};
    if (!MakeRescueRigPose(frame, slot, ropePose, basis, root, diagnostic))
        return false;
    horde::gameplay::animation::PlayerAnimationSnapshot lookChangedRopePose;
    PlayerModelWorldBasis lookChangedBasis;
    Vec3 lookChangedRoot{};
    if (!MakeRescueRigPose(lookChangedFrame, slot, lookChangedRopePose,
                           lookChangedBasis, lookChangedRoot, diagnostic))
        return false;
    Check(ropePose.leftIk == lookChangedRopePose.leftIk &&
              ropePose.rightIk == lookChangedRopePose.rightIk &&
              ropePose.swordStowBlend == lookChangedRopePose.swordStowBlend &&
              ropePose.swordHandGripBlend == lookChangedRopePose.swordHandGripBlend &&
              basis.modelXInWorld == lookChangedBasis.modelXInWorld &&
              root == lookChangedRoot,
          "camera look must not rotate either rope grip or the rescue body frame");
    Check(ropePose.swordStowBlend == 1.0f && ropePose.swordHandGripBlend == 0.0f,
          "both rope hands must take ownership while the sword is stowed");

    const auto shoulderWorld = [&](const horde::gameplay::animation::PlayerIkVector& shoulder) {
        const Vec3 local = PlayerModelVectorToWorld(basis, shoulder);
        return Vec3{{root[0] + local[0], root[1] + local[1], root[2] + local[2]}};
    };
    const auto ikTargetWorld = [&](const horde::gameplay::animation::PlayerIkVector& target) {
        const Vec3 local = PlayerModelVectorToWorld(basis, target);
        return Vec3{{root[0] + local[0], root[1] + local[1], root[2] + local[2]}};
    };
    const auto targetWorld = [&](std::size_t hand) {
        const auto& target = frame.rescue.grippingHandTargets[hand];
        return Vec3{{target.x, target.y, target.z}};
    };
    if (!frame.rescue.ropeHandsActive)
    {
        const Vec3 eyeWorld{{frame.cameraX, PlayerEyeWorldY(frame.playerSupportWorldY),
                             frame.cameraZ}};
        const std::array<float, 2u> sides{{-1.0f, 1.0f}};
        const std::array<const horde::gameplay::animation::PlayerArmIkTarget*, 2u> arms{{
            &ropePose.leftIk, &ropePose.rightIk}};
        for (std::size_t hand = 0u; hand < arms.size(); ++hand)
        {
            const Vec3 expected{{root[0] + std::cos(frame.rescue.bodyYawRadians) * sides[hand] * 0.22f +
                                     std::sin(frame.rescue.bodyYawRadians) * 0.15f,
                                 eyeWorld[1] - 0.65f,
                                 root[2] + std::sin(frame.rescue.bodyYawRadians) * sides[hand] * 0.22f -
                                     std::cos(frame.rescue.bodyYawRadians) * 0.15f}};
            const Vec3 actual = ikTargetWorld(arms[hand]->target);
            const float freeReach = Distance(shoulderWorld(arms[hand]->shoulder), actual);
            Check(Distance(actual, expected) <= 0.001f,
                  "hands-free apron transition must use the body-relative rescue carry pose");
            Check(Distance(actual, targetWorld(hand)) > 0.10f,
                  "hands-free support pose must not claim a solved rope grip");
            Check(freeReach <= kPlayerAnatomicalHandReachLimitMetres,
                  "hands-free support pose must remain within admitted anatomical reach");
        }
    }
    if (frame.rescue.ropeHandsActive)
    {
        Check(frame.heldItems[0].detached ||
                  frame.heldItems[0].parentMode != HeldItemParentMode::HandSocket,
              "the left hand-held light must relinquish hand ownership while the left arm grips the rope");
        const float leftReach = Distance(shoulderWorld(ropePose.leftIk.shoulder), targetWorld(0u));
        const float rightReach = Distance(shoulderWorld(ropePose.rightIk.shoulder), targetWorld(1u));
        if (leftReach > maximumLeftReach) {
            maximumLeftReach = leftReach;
            maximumLeftReachPhase = frame.rescue.phase;
        }
        if (rightReach > maximumRightReach) {
            maximumRightReach = rightReach;
            maximumRightReachPhase = frame.rescue.phase;
        }
        Check(leftReach <= kPlayerAnatomicalHandReachLimitMetres &&
                  rightReach <= kPlayerAnatomicalHandReachLimitMetres,
              "both solved rope grips must remain within the admitted imported forearm reach");
        const auto& anchorBeam = horde::scene::kRescueBlockoutBoxes[11];
        for (std::size_t hand = 0u; hand < frame.rescue.grippingHandTargets.size(); ++hand)
        {
            const Vec3 target = targetWorld(hand);
            const bool insideBeamFootprint = target[0] >= anchorBeam.minimum[0] - 0.04f &&
                target[0] <= anchorBeam.maximum[0] + 0.04f &&
                target[2] >= anchorBeam.minimum[2] - 0.04f &&
                target[2] <= anchorBeam.maximum[2] + 0.04f;
            Check(!insideBeamFootprint || target[1] + 0.04f <= anchorBeam.minimum[1] ||
                      target[1] - 0.04f >= anchorBeam.maximum[1],
                  "active rope hand target must clear the solid cantilever beam by 40 mm");
        }
    }
    Check(std::hypot(frame.rescue.ropeNodes[0].x - root[0],
                     frame.rescue.ropeNodes[0].z - root[2]) >= 0.28f,
          "the cantilever anchor and rope must clear the admitted body radius and margin");

    bool poseUpdated = false;
    if (!slot.PreparePose(ropePose, tick,
                         PlayerCpuSkinCadence::Hz60,
                         poseUpdated, diagnostic))
        return false;
    if (!poseUpdated)
    {
        diagnostic = "Rescue rig target update did not skin a new imported-player pose.";
        return false;
    }
    Check(slot.LeftSocketErrorMetres() <= kPlayerGripSocketToleranceMetres &&
              slot.RightSocketErrorMetres() <= kPlayerGripSocketToleranceMetres,
          "actual imported left/right Grip sockets must meet the authored rope or hands-free targets");

    const auto leftGrip = WorldGripFromSnapshot(frame.rescue, 0u);
    const auto rightGrip = WorldGripFromSnapshot(frame.rescue, 1u);
    auto worldFromHips = IdentityHeldItemTransform();
    if (!slot.AnimatedHipsWorldTransform(ropePose, basis, root,
                                         worldFromHips, diagnostic))
        return false;
    HeldItemStates renderedItems{};
    if (!slot.ResolveHeldItemVisuals(frame.heldItems, leftGrip, rightGrip,
                                     worldFromHips, renderedItems, diagnostic))
        return false;
    Check(Finite(renderedItems[1].worldFromItem) &&
              renderedItems[1].parentMode == HeldItemParentMode::BodyStow &&
              renderedItems[1].visualStowBlend >= 0.999f,
          "the production sword must resolve to its animated body-stow mount while both hands grip");

    const auto* ringGrip = FindHeldItemSocket(rewardRing.sockets, "GripRing");
    const auto* ringHinge = FindHeldItemSocket(rewardRing.sockets, "Hinge");
    const auto* flameSocket = FindHeldItemSocket(rewardBody.sockets, "Flame");
    const auto* lightSocket = FindHeldItemSocket(rewardBody.sockets, "Light");
    if (ringGrip == nullptr || ringHinge == nullptr || flameSocket == nullptr ||
        lightSocket == nullptr)
    {
        diagnostic = "Admitted reward GLBs lost GripRing/Hinge/Flame/Light sockets.";
        return false;
    }
    const Transform scale = [] {
        auto result = IdentityHeldItemTransform();
        result[0] = result[5] = result[10] = kClaimedRewardLanternScale;
        return result;
    }();
    RewardLanternVisualTransforms visuals;
    // During rescue the authoritative hinge is the real hip mount; the ring's
    // GripRing is intentionally aligned to that mount instead of either hand.
    if (!ComposeClaimedRewardLanternVisuals(
            frame.rewardLanternWorldFromHinge, ringGrip->world,
            ringHinge->world, frame.rewardLanternWorldFromHinge,
            frame.lanternPendulum.worldFromBody,
            kClaimedRewardLanternScale, visuals, diagnostic))
        return false;
    Check(visuals.gripAgreement.positionErrorMetres <= kPlayerGripSocketToleranceMetres &&
              visuals.gripAgreement.orientationErrorRadians <=
                  kPlayerGripOrientationToleranceRadians,
          "reward GLB GripRing must remain rigid to the physical rescue hip hinge");
    const Transform worldFromBody = MultiplyHeldItemTransforms(visuals.worldFromBody, scale);
    const Transform worldFromFlame = MultiplyHeldItemTransforms(worldFromBody, flameSocket->world);
    const Transform worldFromLight = MultiplyHeldItemTransforms(worldFromBody, lightSocket->world);
    Check(Finite(worldFromFlame) && Finite(worldFromLight) &&
              Distance({{worldFromFlame[12], worldFromFlame[13], worldFromFlame[14]}},
                       {{worldFromLight[12], worldFromLight[13], worldFromLight[14]}}) > 0.01f,
          "physical lantern fire and light mounts must resolve from the scaled body GLB sockets");

    // Test actual scaled imported reward envelopes against every support box
    // and coping while the lantern is stowed. This checks only CPU geometry.
    if (frame.rescue.equipmentStowed)
    {
        const Bounds3 ringBounds = WorldBounds(rewardRing.bounds, visuals.worldFromRing);
        const Bounds3 bodyBounds = WorldBounds(rewardBody.bounds, worldFromBody);
        const std::array<horde::scene::RescueBlockoutBox, 5u> supportAndCoping{{
            horde::scene::kRescueBlockoutBoxes[4], horde::scene::kRescueBlockoutBoxes[5],
            horde::scene::kRescueBlockoutBoxes[6], horde::scene::kRescueBlockoutBoxes[7],
            horde::scene::kRescueBlockoutLanding}};
        for (const auto& box : supportAndCoping)
        {
            const Bounds3 obstacle = BoxBounds(box);
            Check(!Overlaps(ringBounds, obstacle) && !Overlaps(bodyBounds, obstacle),
                  "mounted reward lantern ring/body must clear the actual apron and solid coping throughout traversal");
        }
    }

    diagnostic.clear();
    return true;
}

} // namespace

int main()
{
    const auto root = FindRepoRoot();
    if (root.empty())
    {
        std::cerr << "FAIL: could not find runtime player/reward asset fixture root.\n";
        return 1;
    }
    std::string diagnostic;
    PlayerRenderSlot playerSlot;
    if (!playerSlot.LoadAsset((root / "assets/models/player/runtime/"
                                    "gothic-traveller-lod0.runtime.glb").string(),
                              diagnostic))
    {
        std::cerr << "FAIL: admitted player rig: " << diagnostic << '\n';
        return 1;
    }
    horde::scene::assets::StaticMeshAsset rewardRing, rewardBody;
    if (!LoadStaticAsset(root / "assets/models/props/runtime/reward-lantern-ring",
                         "reward-lantern-ring-lod0.runtime.glb", rewardRing, diagnostic) ||
        !LoadStaticAsset(root / "assets/models/props/runtime/reward-lantern-body",
                         "reward-lantern-body-lod0.runtime.glb", rewardBody, diagnostic))
    {
        std::cerr << "FAIL: admitted player/reward static GLBs: " << diagnostic << '\n';
        return 1;
    }

    GameSimulationConfig config;
    config.playerStartX = kLowerLanding.x;
    config.playerStartZ = kLowerLanding.z;
    config.playerMountProfile = PlayerMountProfile::AnatomicalBody;
    GameSimulation simulation(config);
    Check(simulation.ApplyShowcaseCheckpoint(11),
          "actual Keeper victory checkpoint must seed the rescue attachment test");
    const auto keeperVictory = simulation.Snapshot();
    Check(keeperVictory.torchFailure.triggered && !keeperVictory.torchFailure.heldByPlayer &&
              keeperVictory.torchFailure.phase == horde::gameplay::TorchFailurePhase::Settled &&
              keeperVictory.heldItems[0].detached,
          "rig test must preserve the actual extinguished torch world trajectory");
    simulation.SetDevelopmentRescueJourney(true);
    simulation.ImportRewardCheckpoint(keeperVictory.chestReward,
                                      keeperVictory.interaction,
                                      keeperVictory.finale);
    const auto torchWorldFromItemAtStart = simulation.Snapshot().heldItems[0].worldFromItem;
    Check(simulation.Snapshot().heldItems[0].detached,
          "rescue setup must keep the extinguished torch out of hand ownership");
    InputSnapshot input;
    input.damageEnabled = false;
    input.commands.interact = 1u;
    simulation.StepFixed(input);
    Check(!simulation.Snapshot().rescue.equipmentStowed,
          "a prior failed interaction must not commit asynchronously when readiness changes");
    const WorldZoneToken ascentToken{
        simulation.Snapshot().worldRoute.generation, WorldZoneId::TombExterior};
    Check(simulation.PublishWorldZoneReadiness(ascentToken, ZoneReadiness::Ready),
          "the traversal destination must be rights-admitted before testing the rig");
    ++input.commands.interact;
    input.yawRadians = -0.7f;
    input.pitchRadians = 0.2f;
    simulation.StepFixed(input);
    Check(simulation.Snapshot().rescue.equipmentStowed,
          "a new ready interaction must commit the claimed rescue route");

    std::uint64_t samples = 0u;
    const auto sampleCurrent = [&] {
        ++samples;
        if (!ResolveRigAndAttachments(simulation.Snapshot(),
                simulation.Snapshot().tickIndex + samples, playerSlot,
                rewardRing, rewardBody, diagnostic))
        {
            std::cerr << "FAIL: rescue rig/attachment resolution: " << diagnostic << '\n';
            ++failures;
            ++cases;
        }
    };

    for (int tick = 0; tick < 3; ++tick)
    {
        input.yawRadians += 0.45f;
        simulation.StepFixed(input);
    }
    Check(simulation.Snapshot().rescue.phase == Phase::Ascent,
          "first rope sample must come from a loaded ascent snapshot");
    sampleCurrent();

    int ascentTicks = 0;
    while (simulation.Snapshot().rescue.phase == Phase::Ascent && ascentTicks < 900)
    {
        simulation.StepFixed(input);
        ++ascentTicks;
        if (simulation.Snapshot().rescue.ropeHandsActive)
            sampleCurrent();
    }
    Check(simulation.Snapshot().rescue.phase == Phase::PullUp,
          "loaded rope must resolve the player to its authored pull-up phase");
    sampleCurrent();

    int ascentFinishTicks = 0;
    while (simulation.Snapshot().rescue.equipmentStowed && ascentFinishTicks < 900)
    {
        simulation.StepFixed(input);
        ++ascentFinishTicks;
        if (simulation.Snapshot().rescue.phase == Phase::PullUp)
            sampleCurrent();
    }
    Check(simulation.Snapshot().rescue.phase == Phase::UpperSafe &&
              simulation.Snapshot().rescue.exteriorSide,
          "pull-up must finish on the exterior support");

    const WorldZoneToken descentToken{
        simulation.Snapshot().worldRoute.generation, WorldZoneId::Dungeon};
    simulation.PublishWorldZoneReadiness(descentToken, ZoneReadiness::Ready);
    input.commands.interact += 1u;
    simulation.StepFixed(input);
    Check(simulation.Snapshot().rescue.equipmentStowed,
          "ready upper interaction must commit descent");
    int descentTicks = 0;
    while (simulation.Snapshot().rescue.phase != Phase::Descent && descentTicks < 4)
    {
        simulation.StepFixed(input);
        ++descentTicks;
    }
    Check(simulation.Snapshot().rescue.phase == Phase::Descent,
          "second rope sample must come from an active descent snapshot");
    sampleCurrent();
    int descentCrossingTicks = 0;
    while ((simulation.Snapshot().rescue.phase == Phase::Descent ||
            simulation.Snapshot().rescue.phase == Phase::Landing) &&
           descentCrossingTicks < 900)
    {
        simulation.StepFixed(input);
        ++descentCrossingTicks;
        if (simulation.Snapshot().rescue.ropeHandsActive ||
            simulation.Snapshot().rescue.phase == Phase::Descent)
            sampleCurrent();
    }
    Check(simulation.Snapshot().heldItems[0].detached &&
              simulation.Snapshot().heldItems[0].worldFromItem == torchWorldFromItemAtStart,
          "restoring traversal ownership must preserve the already dropped torch trajectory");

    std::cout << "Rescue rig attachment cases=" << cases
              << " failures=" << failures << " sampled_poses=" << samples
              << " max_left_reach=" << maximumLeftReach << "@phase="
              << static_cast<unsigned>(maximumLeftReachPhase)
              << " max_right_reach=" << maximumRightReach << "@phase="
              << static_cast<unsigned>(maximumRightReachPhase) << '\n';
    return failures == 0 ? 0 : 1;
}
