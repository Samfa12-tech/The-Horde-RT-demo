#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/items/HeldLightState.h"
#include "gameplay/items/HeldItemKinematics.h"
#include "scene/DevelopmentWorldGeometry.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

namespace
{
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
using namespace horde::gameplay::animation;
using namespace horde::gameplay::items;
using namespace horde::vulkan::raytracing;
using Vec3 = std::array<float, 3u>;

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

Vec3 Add(const Vec3& a, const Vec3& b)
{
    return {{a[0] + b[0], a[1] + b[1], a[2] + b[2]}};
}
Vec3 Scale(const Vec3& value, const float scale)
{
    return {{value[0] * scale, value[1] * scale, value[2] * scale}};
}
float Dot(const Vec3& a, const Vec3& b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
bool TransformsNear(const HeldItemTransform& left, const HeldItemTransform& right,
                    const float tolerance = 0.0001f)
{
    for (std::size_t i = 0; i < left.size(); ++i)
        if (!Near(left[i], right[i], tolerance)) return false;
    return true;
}
Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return {{a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
             a[0] * b[1] - a[1] * b[0]}};
}
Vec3 Unit(const Vec3& value)
{
    return Scale(value, 1.0f / std::sqrt(std::max(Dot(value, value), 1.0e-8f)));
}
HeldItemTransform WorldSocket(const std::array<float, 16u>& bone,
                              const PlayerModelWorldBasis& basis,
                              const Vec3& root)
{
    const auto convert = [&basis](const Vec3& vector) {
        return PlayerModelVectorToWorld(basis, vector);
    };
    const Vec3 x = Unit(convert({{bone[0], bone[1], bone[2]}}));
    const Vec3 rawY = convert({{bone[4], bone[5], bone[6]}});
    Vec3 y = Unit(Add(rawY, Scale(x, -Dot(rawY, x))));
    Vec3 z = Unit(Cross(x, y));
    if (Dot(z, convert({{bone[8], bone[9], bone[10]}})) < 0.0f)
    {
        y = Scale(y, -1.0f);
        z = Scale(z, -1.0f);
    }
    const Vec3 position = Add(root, convert({{bone[12], bone[13], bone[14]}}));
    HeldItemTransform result = IdentityHeldItemTransform();
    for (std::size_t axis = 0; axis < 3u; ++axis)
    {
        result[axis] = x[axis]; result[4u + axis] = y[axis]; result[8u + axis] = z[axis];
        result[12u + axis] = position[axis];
    }
    return result;
}

bool PrepareRenderedPose(PlayerRenderSlot& rig,
                         const SimulationSnapshot& snapshot,
                         const std::uint64_t tick,
                         bool& poseUpdated,
                         std::string& diagnostic,
                         HeldItemStates* resolvedItems = nullptr,
                         HeldLightState* resolvedLight = nullptr)
{
    const auto& animation = snapshot.playerAnimation;
    const Vec3 worldUp{{0.0f, 1.0f, 0.0f}};
    const Vec3 eye{{snapshot.playerX,
        horde::gameplay::simulation::PlayerEyeWorldY(snapshot.playerSupportWorldY),
        snapshot.playerZ}};
    const auto lower = horde::gameplay::EvaluateLowerBodyPose(snapshot.walkTime, snapshot.walkAmount);
    const Vec3 bodyForward{{std::sin(snapshot.playerYawRadians), 0.0f, -std::cos(snapshot.playerYawRadians)}};
    const Vec3 bodyRight{{std::cos(snapshot.playerYawRadians), 0.0f, std::sin(snapshot.playerYawRadians)}};
    const float cosine = std::cos(lower.torsoTwistRadians), sine = std::sin(lower.torsoTwistRadians);
    const PlayerModelWorldBasis basis = BuildPlayerModelWorldBasis(
        Unit(Add(Scale(bodyRight, cosine), Scale(bodyForward, -sine))),
        Unit(Add(Scale(bodyForward, cosine), Scale(bodyRight, sine))));
    const Vec3 bodyOrigin = Add(Add(eye, Scale(bodyRight, lower.pelvisSway)),
                                Vec3{{0.0f, lower.pelvisBob * 0.65f, 0.0f}});
    const float pitch = std::clamp(snapshot.playerPitchRadians, -0.32f, 0.28f);
    const Vec3 viewForward = Unit({{std::sin(snapshot.playerYawRadians), -0.05f + pitch,
                                    -std::cos(snapshot.playerYawRadians)}});
    const Vec3 viewRight = Unit(Cross(viewForward, worldUp));
    const Vec3 viewUp = Unit(Cross(viewRight, viewForward));
    const auto viewToWorld = [&](const Vec3& value) {
        return Add(Add(Scale(viewRight, value[0]), Scale(viewUp, value[1])), Scale(viewForward, value[2]));
    };
    const Vec3 torsoCenterWorld = Add(eye, viewToWorld(EvaluatePlayerTorsoAnchorLocal(animation)));
    Vec3 rigShoulderCenter{};
    if (!rig.ShoulderCenter(animation, rigShoulderCenter, diagnostic)) return false;
    Vec3 root = GroundPlayerRootOnRouteFloor(
        Add(torsoCenterWorld, Scale(PlayerModelVectorToWorld(basis, rigShoulderCenter), -1.0f)),
        snapshot.playerSupportWorldY, rig.BootGroundingOffsetMetres(animation));
    if (snapshot.playerMountProfile == PlayerMountProfile::AnatomicalBody)
        root = GroundPlayerRootOnRouteFloor(bodyOrigin, snapshot.playerSupportWorldY,
                                            rig.BootGroundingOffsetMetres(animation));

    const auto worldPointToModel = [&](const Vec3& point) {
        return WorldVectorToPlayerModel(basis,
            Add(Add(eye, viewToWorld(point)), Scale(root, -1.0f)));
    };
    const auto viewVectorToModel = [&](const Vec3& vector) {
        return WorldVectorToPlayerModel(basis, viewToWorld(vector));
    };
    PlayerAnimationSnapshot rigAnimation = animation;
    for (auto* arm : {&rigAnimation.leftIk, &rigAnimation.rightIk})
    {
        arm->shoulder = worldPointToModel(arm->shoulder);
        arm->target = worldPointToModel(arm->target);
        arm->pole = viewVectorToModel(arm->pole);
        arm->gripX = viewVectorToModel(arm->gripX);
        arm->gripY = viewVectorToModel(arm->gripY);
        arm->gripZ = viewVectorToModel(arm->gripZ);
    }
    HeldItemTransform worldFromHips{};
    if (!rig.AnimatedHipsWorldTransform(animation, basis, root, worldFromHips, diagnostic)) return false;
    const HeldItemTransform bodyStow = MultiplyHeldItemTransforms(
        worldFromHips, SwordBodyStowFromHips());
    if (animation.swordStowBlend > 0.0f)
    {
        const auto expectedWorldFromItem = BlendHeldItemTransformsAtGrip(
            bodyStow, snapshot.heldItems[1].worldFromItem, SwordGripSocketTransform(),
            1.0f - animation.swordStowBlend);
        const auto expectedWorldFromGrip = MultiplyHeldItemTransforms(
            expectedWorldFromItem, SwordGripSocketTransform());
        auto itemArm = rigAnimation.rightIk;
        itemArm.target = worldPointToModel(
            {{expectedWorldFromGrip[12], expectedWorldFromGrip[13], expectedWorldFromGrip[14]}});
        itemArm.gripX = WorldVectorToPlayerModel(basis,
            {{expectedWorldFromGrip[0], expectedWorldFromGrip[1], expectedWorldFromGrip[2]}});
        itemArm.gripY = WorldVectorToPlayerModel(basis,
            {{expectedWorldFromGrip[4], expectedWorldFromGrip[5], expectedWorldFromGrip[6]}});
        itemArm.gripZ = WorldVectorToPlayerModel(basis,
            {{expectedWorldFromGrip[8], expectedWorldFromGrip[9], expectedWorldFromGrip[10]}});
        HeldItemTransform itemGrip = IdentityHeldItemTransform();
        for (std::size_t axis = 0u; axis < 3u; ++axis)
        {
            itemGrip[axis] = itemArm.gripX[axis];
            itemGrip[4u + axis] = itemArm.gripY[axis];
            itemGrip[8u + axis] = itemArm.gripZ[axis];
            itemGrip[12u + axis] = itemArm.target[axis];
        }
        rigAnimation.rightIk = BlendPlayerArmGripTarget(
            rigAnimation.rightIk, itemGrip, animation.swordHandGripBlend);
    }
    if (!rig.PreparePose(rigAnimation, tick, PlayerCpuSkinCadence::Hz60,
                         poseUpdated, diagnostic)) return false;

    const auto leftGrip = WorldSocket(rig.BoneSockets().leftGrip, basis, root);
    const auto rightGrip = WorldSocket(rig.BoneSockets().rightGrip, basis, root);
    HeldItemStates renderItems{};
    if (!rig.ResolveHeldItemVisuals(snapshot.heldItems, leftGrip, rightGrip, bodyStow,
                                    renderItems, diagnostic)) return false;
    HeldLightState light{};
    if (!ComposeHeldLightState(renderItems[0].worldFromItem,
            PlayerRagTorchFlameSocketTransform(), PlayerRagTorchLightSocketTransform(),
            1.0f, light, diagnostic) || !light.active) return false;
    if (resolvedItems != nullptr) *resolvedItems = renderItems;
    if (resolvedLight != nullptr) *resolvedLight = light;
    return true;
}
}

int main(int argc, char** argv)
{
    if (!Require(argc == 2, "directional rig test requires the explicit source asset root")) return 1;
    const std::filesystem::path root(argv[1]);
    const auto rigPath = root / "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb";
    PlayerRenderSlot rig;
    std::string diagnostic;
    if (!Require(rig.LoadAsset(rigPath.string(), diagnostic), diagnostic.c_str())) return 1;

    GameSimulation runSimulation(ProductionGameSimulationConfig());
    InputSnapshot runInput{};
    runInput.damageEnabled = false;
    runInput.yawRadians = 0.37f;
    runInput.pitchRadians = 0.18f;
    runInput.commands.runToggle = 1u;
    constexpr std::array<std::array<float, 2u>, 4u> directions{{
        {{1.0f, 0.0f}}, {{-1.0f, 0.0f}}, {{0.0f, -1.0f}}, {{0.0f, 1.0f}}}};
    std::uint64_t rigTick = 1u;
    for (const auto& direction : directions)
    {
        runInput.moveForward = direction[0];
        runInput.moveStrafe = direction[1];
        for (int tick = 0; tick < 12; ++tick)
        {
            runSimulation.StepFixed(runInput);
            const auto& snapshot = runSimulation.Snapshot();
            bool poseUpdated = false;
            if (!PrepareRenderedPose(rig, snapshot, rigTick++, poseUpdated, diagnostic))
                return Require(false, diagnostic.c_str()) ? 0 : 1;
            if (!poseUpdated || !rig.SolvedPose().IsValid() || rig.UniqueVertices().empty())
                return Require(false, "rights-cleared runtime rig must skin each directional stride") ? 0 : 1;
            const float leftError = rig.LeftSocketErrorMetres();
            const float rightError = rig.RightSocketErrorMetres();
            if (leftError > kPlayerGripSocketToleranceMetres ||
                rightError > kPlayerGripSocketToleranceMetres)
            {
                std::cerr << "Directional rig socket error at tick " << rigTick - 1u
                          << " clip=" << static_cast<int>(snapshot.playerAnimation.locomotionClip)
                          << " blend=" << snapshot.playerAnimation.locomotionBlend
                          << " left=" << leftError << " right=" << rightError
                          << " tolerance=" << kPlayerGripSocketToleranceMetres << '\n';
                return Require(false, "runtime hand sockets must retain the admitted grip tolerance") ? 0 : 1;
            }
            if (!Require(Near(snapshot.playerYawRadians, runInput.yawRadians) &&
                         Near(snapshot.playerPitchRadians, runInput.pitchRadians) &&
                         Near(snapshot.movementForward, direction[0]) &&
                         Near(snapshot.movementStrafe, direction[1]) &&
                         snapshot.playerAnimation.locomotionClip == MapPlayerLocomotionClip(
                             snapshot.playerAnimation.locomotionBlend),
                         "directional movement stays independent from aim and maps through supported Idle/Walking slots")) return 1;
        }
    }

    GameSimulation measuredWalk;
    InputSnapshot walkInput{};
    walkInput.damageEnabled = false;
    walkInput.moveForward = 1.0f;
    for (int tick = 0; tick < 48; ++tick) measuredWalk.StepFixed(walkInput);
    if (!Require(runSimulation.Snapshot().runActive &&
                 runSimulation.Snapshot().playerAnimation.locomotionTime >
                     measuredWalk.Snapshot().playerAnimation.locomotionTime &&
                 runSimulation.Snapshot().playerAnimation.combatLayer.action ==
                     PlayerUpperBodyAction::None,
                 "run changes actual shared locomotion phase without taking combat-layer priority")) return 1;

    for (const auto& direction : directions)
    {
        auto dodgeConfig = ProductionGameSimulationConfig();
        // Measure the authored dodge in the open chamber so the legacy narrow
        // corridor opening cannot clip its endpoint and obscure the distance
        // disposition this proof is checking.
        dodgeConfig.playerStartX = -5.5f;
        dodgeConfig.playerStartZ = -13.5f;
        dodgeConfig.waterfallSkeletonEncounter = false;
        dodgeConfig.swordStartsStowed = false;
        GameSimulation dodgeSimulation(dodgeConfig);
        InputSnapshot dodgeInput{};
        dodgeInput.damageEnabled = false;
        dodgeInput.yawRadians = runInput.yawRadians;
        dodgeInput.pitchRadians = runInput.pitchRadians;
        dodgeInput.moveForward = direction[0];
        dodgeInput.moveStrafe = direction[1];
        dodgeInput.commands.runToggle = 1u;
        dodgeInput.commands.dodge = 1u;
        const auto start = dodgeSimulation.Snapshot();
        if (!Near(start.playerX, dodgeConfig.playerStartX) ||
            !Near(start.playerZ, dodgeConfig.playerStartZ))
        {
            std::cerr << "Directional dodge chamber spawn rejected; actual start="
                      << start.playerX << ',' << start.playerZ << '\n';
            return Require(false, "dodge proof starts in the open chamber") ? 0 : 1;
        }
        for (int tick = 0; tick < 12; ++tick)
        {
            dodgeSimulation.StepFixed(dodgeInput);
            dodgeInput.moveForward = 0.0f;
            dodgeInput.moveStrafe = 0.0f;
            const auto& snapshot = dodgeSimulation.Snapshot();
            bool poseUpdated = false;
            const auto poseTick = rigTick++;
            if (!PrepareRenderedPose(rig, snapshot, poseTick, poseUpdated, diagnostic))
            {
                std::cerr << "Directional dodge pose rejected: " << diagnostic << '\n';
                return Require(false, "actual directional dodge pose must pass renderer rig admission") ? 0 : 1;
            }
            if (!poseUpdated || !rig.SolvedPose().IsValid() ||
                rig.LeftSocketErrorMetres() > kPlayerGripSocketToleranceMetres ||
                rig.RightSocketErrorMetres() > kPlayerGripSocketToleranceMetres)
            {
                std::cerr << "Directional dodge socket failure at tick " << poseTick
                          << " clip=" << static_cast<int>(snapshot.playerAnimation.locomotionClip)
                          << " blend=" << snapshot.playerAnimation.locomotionBlend
                          << " updated=" << poseUpdated
                          << " left=" << rig.LeftSocketErrorMetres()
                          << " right=" << rig.RightSocketErrorMetres()
                          << " tolerance=" << kPlayerGripSocketToleranceMetres << '\n';
                return Require(false, "directional dodge retains the admitted real-rig socket tolerance") ? 0 : 1;
            }
        }
        const auto& result = dodgeSimulation.Snapshot();
        const float distance = std::hypot(result.playerX - start.playerX,
                                          result.playerZ - start.playerZ);
        if (!(distance > 0.82f && distance < 0.98f &&
              !result.runToggleActive && !result.runActive &&
              result.lastConsumedDodgeSequence == 1u))
        {
            std::cerr << "Directional dodge disposition distance=" << distance
                      << " dodgeSequence=" << result.lastConsumedDodgeSequence
                      << " runToggle=" << result.runToggleActive
                      << " runActive=" << result.runActive
                      << " direction=" << direction[0] << ',' << direction[1]
                      << " action=" << static_cast<int>(result.playerCombat.action) << '\n';
            return Require(false, "forward/back/left/right dodges keep authored distance with run cancelled") ? 0 : 1;
        }
    }

    // Exercise the real route support profile against the same prepared host
    // geometry consumed by the renderer. This is CPU geometry/readiness
    // admission only; it makes no claim about BLAS upload or RT presentation.
    const auto worldGeometry = horde::scene::PrepareDevelopmentWorldGeometry(false);
    if (!Require(worldGeometry.valid &&
                 horde::scene::ValidateDevelopmentWorldGeometry(worldGeometry),
                 "directional rig route proof requires valid prepared world geometry")) return 1;
    auto routeConfig = ProductionGameSimulationConfig();
    auto routeSimulation = std::make_unique<GameSimulation>(routeConfig);
    routeSimulation->SetDevelopmentWorldRoute(true);
    bool cpuGeometryReadinessAdmitted = true;
    for (std::size_t zone = 1u; zone < kWorldZones.size(); ++zone)
    {
        cpuGeometryReadinessAdmitted &= routeSimulation->PublishWorldZoneReadiness(
            {routeSimulation->Snapshot().worldRoute.generation,
             static_cast<WorldZoneId>(zone)}, ZoneReadiness::Ready);
    }
    if (!Require(cpuGeometryReadinessAdmitted,
                 "prepared CPU geometry must admit the route host-readiness descriptors")) return 1;

    InputSnapshot routeInput{};
    routeInput.damageEnabled = false;
    routeInput.yawRadians = 3.14159265359f;
    routeInput.moveForward = 1.0f;
    bool sawStepSupport = false;
    bool sawRaisedSlope = false;
    std::uint64_t routePoseSamples = 0u;
    auto inspectRoutePose = [&]() {
        const auto& snapshot = routeSimulation->Snapshot();
        bool updated = false;
        HeldItemStates items{};
        HeldLightState light{};
        if (!PrepareRenderedPose(rig, snapshot, rigTick++, updated, diagnostic,
                                 &items, &light))
        {
            std::cerr << "World-route pose rejected: " << diagnostic << '\n';
            return false;
        }
        if (!updated || !rig.SolvedPose().IsValid() || rig.UniqueVertices().empty() ||
            rig.LeftSocketErrorMetres() > kPlayerGripSocketToleranceMetres ||
            rig.RightSocketErrorMetres() > kPlayerGripSocketToleranceMetres)
        {
            std::cerr << "World-route rig failure tick=" << snapshot.tickIndex
                      << " support=" << snapshot.playerSupportWorldY
                      << " left=" << rig.LeftSocketErrorMetres()
                      << " right=" << rig.RightSocketErrorMetres() << '\n';
            return false;
        }
        const auto composedFlame = MultiplyHeldItemTransforms(
            items[0].worldFromItem, PlayerRagTorchFlameSocketTransform());
        const auto composedLight = MultiplyHeldItemTransforms(
            items[0].worldFromItem, PlayerRagTorchLightSocketTransform());
        if (items[0].id != HeldItemId::OriginalTorch || items[1].id != HeldItemId::Sword ||
            items[0].parentMode != HeldItemParentMode::HandSocket ||
            items[1].parentMode != HeldItemParentMode::BodyStow || !light.active ||
            !TransformsNear(composedFlame, light.worldFromFlame) ||
            !TransformsNear(composedLight, light.worldFromLight))
        {
            std::cerr << "World-route equipment attachment failure tick=" << snapshot.tickIndex
                      << " torchParent=" << static_cast<int>(items[0].parentMode)
                      << " swordParent=" << static_cast<int>(items[1].parentMode)
                      << " lightActive=" << light.active << '\n';
            return false;
        }
        ++routePoseSamples;
        return true;
    };
    const auto walkRouteTo = [&](const WorldRoutePoint& target, const float forward) {
        routeInput.yawRadians = 3.14159265359f;
        routeInput.moveForward = forward;
        auto previousSupportId = routeSimulation->Snapshot().playerSupportId;
        for (std::uint32_t tick = 0u; tick < 1800u; ++tick)
        {
            const auto& before = routeSimulation->Snapshot();
            if (std::hypot(target.x - before.playerX, target.z - before.playerZ) < 0.035f)
                return true;
            routeSimulation->StepFixed(routeInput);
            const auto& snapshot = routeSimulation->Snapshot();
            if (!snapshot.playerGrounded ||
                (snapshot.playerSupportSurface != SupportSurface::Stone &&
                 snapshot.playerSupportSurface != SupportSurface::Earth))
            {
                std::cerr << "World-route support lost tick=" << snapshot.tickIndex
                          << " x=" << snapshot.playerX << " z=" << snapshot.playerZ
                          << " y=" << snapshot.playerSupportWorldY << '\n';
                return false;
            }
            sawStepSupport |= snapshot.playerSupportId == PlayerSupportId::WorldRouteStep &&
                Near(snapshot.playerHeightDelta, 0.12f, 0.002f);
            sawRaisedSlope |= snapshot.playerSupportWorldY > kRouteFloorWorldY + 1.0f &&
                snapshot.playerSupportSurface == SupportSurface::Stone;
            if (tick % 20u == 19u || snapshot.playerSupportId != previousSupportId)
            {
                if (!inspectRoutePose()) return false;
            }
            previousSupportId = snapshot.playerSupportId;
        }
        std::cerr << "World-route travel timed out at x=" << routeSimulation->Snapshot().playerX
                  << " z=" << routeSimulation->Snapshot().playerZ
                  << " support=" << routeSimulation->Snapshot().playerSupportWorldY
                  << " target=" << target.x << ',' << target.z << '\n';
        return false;
    };
    if (!Require(walkRouteTo(kWorldRoutePoints[2], 1.0f),
                 "real-rig route travel must reach the sloped raised support")) return 1;
    if (!Require(sawStepSupport && sawRaisedSlope &&
                 Near(routeSimulation->Snapshot().playerSupportWorldY,
                      kWorldRoutePoints[2].y, 0.03f) &&
                 routeSimulation->Snapshot().playerSupportSurface == SupportSurface::Stone,
                 "real-rig route travel must resolve both the 12 cm step and raised slope")) return 1;
    if (!Require(walkRouteTo(kWorldRoutePoints[0], -1.0f),
                 "real-rig route travel must return to the ground support")) return 1;
    if (!Require(Near(routeSimulation->Snapshot().playerSupportWorldY,
                      kWorldRoutePoints[0].y, 0.002f) &&
                 Near(routeSimulation->Snapshot().playerHeightDelta, 0.0f, 0.002f) &&
                 routeSimulation->Snapshot().playerSupportSurface == SupportSurface::Stone &&
                 routePoseSamples >= 10u,
                 "real-rig route return must finish grounded after bounded grip samples")) return 1;
    if (!inspectRoutePose())
        return Require(false, "returned ground pose retains real equipment and light attachments") ? 0 : 1;

    std::cout << "Directional runtime rig, aim independence, run stride, dodge disposition, route support poses, and attachment sockets passed; dedicated run/back/strafe/dodge clips remain absent; route_pose_samples="
              << routePoseSamples << " GPU_cost=not-measured.\n";
    return 0;
}
