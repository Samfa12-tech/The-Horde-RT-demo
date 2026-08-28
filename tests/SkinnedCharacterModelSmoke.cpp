#include "scene/SkeletonBipedModel.h"
#include "scene/assets/AssetManifest.h"
#include "scene/assets/StaticMeshAsset.h"
#include "gameplay/DevelopmentCheckpointSimulation.h"
#include "gameplay/simulation/GameSimulation.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace
{

std::filesystem::path FindRepoRoot()
{
    std::filesystem::path candidate = std::filesystem::current_path();
    for (int depth = 0; depth < 6; ++depth)
    {
        if (std::filesystem::exists(candidate / "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb")) return candidate;
        if (!candidate.has_parent_path()) break;
        candidate = candidate.parent_path();
    }
    return {};
}

bool Require(bool condition, const char* message)
{
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}

bool FiniteTexturedVertices(const std::vector<horde::scene::TexturedSkinnedRtVertex>& vertices)
{
    if (vertices.empty()) return false;
    bool sawDistinctUv = false;
    const float firstU = vertices.front().texcoord[0];
    const float firstV = vertices.front().texcoord[1];
    for (const auto& vertex : vertices)
    {
        for (float value : vertex.position) if (!std::isfinite(value)) return false;
        for (float value : vertex.normal) if (!std::isfinite(value)) return false;
        for (float value : vertex.texcoord) if (!std::isfinite(value)) return false;
        sawDistinctUv = sawDistinctUv || std::abs(vertex.texcoord[0] - firstU) > 0.0001f || std::abs(vertex.texcoord[1] - firstV) > 0.0001f;
        if (vertex.texcoord[2] != 0.0f || vertex.texcoord[3] != 0.0f) return false;
    }
    return sawDistinctUv;
}

using Vec3 = std::array<float, 3u>;
using HeldItemTransform = horde::gameplay::items::HeldItemTransform;

Vec3 Scale(const Vec3& value, const float scale)
{
    return {{value[0] * scale, value[1] * scale, value[2] * scale}};
}

float Dot(const Vec3& left, const Vec3& right)
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

Vec3 Cross(const Vec3& left, const Vec3& right)
{
    return {{left[1] * right[2] - left[2] * right[1],
             left[2] * right[0] - left[0] * right[2],
             left[0] * right[1] - left[1] * right[0]}};
}

Vec3 Normalise(const Vec3& value)
{
    const float length = std::sqrt(std::max(Dot(value, value), 0.0000001f));
    return Scale(value, 1.0f / length);
}

struct TestRigFrame
{
    horde::gameplay::animation::PlayerAnimationSnapshot animation{};
    Vec3 bodyRight{};
    Vec3 bodyForward{};
    Vec3 playerRootWorld{};
};

TestRigFrame BuildRigFrame(
    const horde::gameplay::simulation::SimulationSnapshot& source,
    const horde::scene::SkinnedNodeTransform& leftArmBase,
    const horde::scene::SkinnedNodeTransform& rightArmBase)
{
    TestRigFrame result;
    result.animation = source.playerAnimation;
    const Vec3 worldUp{{0.0f, 1.0f, 0.0f}};
    const Vec3 bodyForward{{std::sin(source.playerYawRadians), 0.0f,
                            -std::cos(source.playerYawRadians)}};
    const Vec3 bodyRight{{std::cos(source.playerYawRadians), 0.0f,
                          std::sin(source.playerYawRadians)}};
    const auto lowerBody = horde::gameplay::EvaluateLowerBodyPose(
        source.walkTime, source.walkAmount);
    result.bodyForward = Normalise({{
        bodyForward[0] * std::cos(lowerBody.torsoTwistRadians) +
            bodyRight[0] * std::sin(lowerBody.torsoTwistRadians),
        0.0f,
        bodyForward[2] * std::cos(lowerBody.torsoTwistRadians) +
            bodyRight[2] * std::sin(lowerBody.torsoTwistRadians)}});
    result.bodyRight = Normalise({{
        bodyRight[0] * std::cos(lowerBody.torsoTwistRadians) -
            bodyForward[0] * std::sin(lowerBody.torsoTwistRadians),
        0.0f,
        bodyRight[2] * std::cos(lowerBody.torsoTwistRadians) -
            bodyForward[2] * std::sin(lowerBody.torsoTwistRadians)}});
    const Vec3 eye{{source.playerX, 0.58f, source.playerZ}};
    const Vec3 viewForward = Normalise({{
        std::sin(source.playerYawRadians),
        -0.05f + std::clamp(source.playerPitchRadians, -0.32f, 0.28f),
        -std::cos(source.playerYawRadians)}});
    const Vec3 viewRight = Normalise(Cross(viewForward, worldUp));
    const Vec3 viewUp = Normalise(Cross(viewRight, viewForward));
    const auto toWorld = [&eye, &viewRight, &viewUp, &viewForward](const Vec3& local) {
        return Vec3{{eye[0] + viewRight[0] * local[0] + viewUp[0] * local[1] +
                         viewForward[0] * local[2],
                     eye[1] + viewRight[1] * local[0] + viewUp[1] * local[1] +
                         viewForward[1] * local[2],
                     eye[2] + viewRight[2] * local[0] + viewUp[2] * local[1] +
                         viewForward[2] * local[2]}};
    };
    const Vec3 leftShoulder = toWorld(source.playerAnimation.leftIk.shoulder);
    const Vec3 rightShoulder = toWorld(source.playerAnimation.rightIk.shoulder);
    const Vec3 leftHand = toWorld(source.playerAnimation.leftIk.target);
    const Vec3 rightHand = toWorld(source.playerAnimation.rightIk.target);
    const Vec3 rigCenter{{
        (leftArmBase[12] + rightArmBase[12]) * 0.5f,
        (leftArmBase[13] + rightArmBase[13]) * 0.5f,
        (leftArmBase[14] + rightArmBase[14]) * 0.5f}};
    const Vec3 intendedCenter = Scale(
        {{leftShoulder[0] + rightShoulder[0],
          leftShoulder[1] + rightShoulder[1],
          leftShoulder[2] + rightShoulder[2]}}, 0.5f);
    result.playerRootWorld = {{
        intendedCenter[0] - result.bodyRight[0] * rigCenter[0] -
            result.bodyForward[0] * rigCenter[2],
        intendedCenter[1] - rigCenter[1],
        intendedCenter[2] - result.bodyRight[2] * rigCenter[0] -
            result.bodyForward[2] * rigCenter[2]}};
    const auto worldPointToPlayer = [&result](const Vec3& world) {
        const Vec3 delta{{world[0] - result.playerRootWorld[0],
                          world[1] - result.playerRootWorld[1],
                          world[2] - result.playerRootWorld[2]}};
        return Vec3{{Dot(delta, result.bodyRight), delta[1],
                     Dot(delta, result.bodyForward)}};
    };
    const auto viewVectorToPlayer = [&result, &viewRight, &viewUp,
                                     &viewForward](const Vec3& view) {
        const Vec3 world{{viewRight[0] * view[0] + viewUp[0] * view[1] +
                              viewForward[0] * view[2],
                          viewRight[1] * view[0] + viewUp[1] * view[1] +
                              viewForward[1] * view[2],
                          viewRight[2] * view[0] + viewUp[2] * view[1] +
                              viewForward[2] * view[2]}};
        return Vec3{{Dot(world, result.bodyRight), world[1],
                     Dot(world, result.bodyForward)}};
    };
    result.animation.leftIk.shoulder = worldPointToPlayer(leftShoulder);
    result.animation.leftIk.target = worldPointToPlayer(leftHand);
    result.animation.leftIk.pole = viewVectorToPlayer(source.playerAnimation.leftIk.pole);
    result.animation.leftIk.gripX = viewVectorToPlayer(source.playerAnimation.leftIk.gripX);
    result.animation.leftIk.gripY = viewVectorToPlayer(source.playerAnimation.leftIk.gripY);
    result.animation.leftIk.gripZ = viewVectorToPlayer(source.playerAnimation.leftIk.gripZ);
    result.animation.rightIk.shoulder = worldPointToPlayer(rightShoulder);
    result.animation.rightIk.target = worldPointToPlayer(rightHand);
    result.animation.rightIk.pole = viewVectorToPlayer(source.playerAnimation.rightIk.pole);
    result.animation.rightIk.gripX = viewVectorToPlayer(source.playerAnimation.rightIk.gripX);
    result.animation.rightIk.gripY = viewVectorToPlayer(source.playerAnimation.rightIk.gripY);
    result.animation.rightIk.gripZ = viewVectorToPlayer(source.playerAnimation.rightIk.gripZ);
    return result;
}

HeldItemTransform GripTransform(const Vec3& x,
                                const Vec3& y,
                                const Vec3& z,
                                const Vec3& position)
{
    return {{x[0], x[1], x[2], 0.0f,
             y[0], y[1], y[2], 0.0f,
             z[0], z[1], z[2], 0.0f,
             position[0], position[1], position[2], 1.0f}};
}

HeldItemTransform RigidWorldBoneTransform(
    const horde::scene::SkinnedNodeTransform& bone,
    const TestRigFrame& frame)
{
    const auto toWorld = [&frame](const Vec3& local) {
        return Vec3{{frame.bodyRight[0] * local[0] + frame.bodyForward[0] * local[2],
                     local[1],
                     frame.bodyRight[2] * local[0] + frame.bodyForward[2] * local[2]}};
    };
    Vec3 x = Normalise(toWorld({{bone[0], bone[1], bone[2]}}));
    const Vec3 rawY = toWorld({{bone[4], bone[5], bone[6]}});
    Vec3 y = Normalise({{rawY[0] - x[0] * Dot(rawY, x),
                         rawY[1] - x[1] * Dot(rawY, x),
                         rawY[2] - x[2] * Dot(rawY, x)}});
    Vec3 z = Normalise(Cross(x, y));
    const Vec3 rawZ = toWorld({{bone[8], bone[9], bone[10]}});
    if (Dot(z, rawZ) < 0.0f)
    {
        y = Scale(y, -1.0f);
        z = Scale(z, -1.0f);
    }
    const Vec3 localPosition{{bone[12], bone[13], bone[14]}};
    const Vec3 worldOffset = toWorld(localPosition);
    const Vec3 position{{frame.playerRootWorld[0] + worldOffset[0],
                         frame.playerRootWorld[1] + worldOffset[1],
                         frame.playerRootWorld[2] + worldOffset[2]}};
    HeldItemTransform result = GripTransform(x, y, z, position);
    return result;
}

HeldItemTransform FinalGrip(const horde::gameplay::items::HeldItemState& item)
{
    return horde::gameplay::items::MultiplyHeldItemTransforms(
        item.worldFromItem,
        item.id == horde::gameplay::items::HeldItemId::OriginalTorch
            ? horde::gameplay::items::OriginalTorchGripSocketTransform()
            : horde::gameplay::items::SwordGripSocketTransform());
}

float PositionError(const HeldItemTransform& left, const HeldItemTransform& right)
{
    return std::hypot(std::hypot(left[12] - right[12], left[13] - right[13]),
                      left[14] - right[14]);
}

float OrientationError(const HeldItemTransform& left, const HeldItemTransform& right)
{
    float maximum = 0.0f;
    for (std::size_t column = 0u; column < 3u; ++column)
    {
        const std::size_t offset = column * 4u;
        const float cosine = std::clamp(
            left[offset] * right[offset] + left[offset + 1u] * right[offset + 1u] +
                left[offset + 2u] * right[offset + 2u],
            -1.0f, 1.0f);
        maximum = std::max(maximum, std::acos(cosine));
    }
    return maximum;
}

} // namespace

int main()
{
    using namespace horde::scene;
    static_assert(sizeof(SkinnedRtVertex) == 32u);
    static_assert(sizeof(TexturedSkinnedRtVertex) == 48u);

    const std::filesystem::path root = FindRepoRoot();
    if (!Require(!root.empty(), "repo assets were not found")) return 1;

    std::string diagnostic;
    SkinnedCharacterModel skeleton;
    const auto skeletonPath = root / "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb";
    if (!Require(skeleton.LoadCombatClips(skeletonPath.string(), diagnostic), diagnostic.c_str())) return 1;
    if (!Require(skeleton.HasTexcoords(), "skeleton TEXCOORD_0 was not imported")) return 1;
    if (!Require(skeleton.ExpandedVertexCount() == 28206u, "skeleton expanded vertex count changed")) return 1;
    if (!Require(skeleton.HasNode("LeftHand") && skeleton.HasNode("RightHand"),
                 "general skinned asset must expose actual left/right hand bones")) return 1;
    SkinnedNodeTransform leftHandSocket{};
    SkinnedNodeTransform rightHandSocket{};
    if (!Require(skeleton.NodeTransform(SkinnedClip::Walking, 0.25f, "LeftHand", leftHandSocket, diagnostic),
                 diagnostic.c_str()) ||
        !Require(skeleton.NodeTransform(SkinnedClip::Walking, 0.25f, "RightHand", rightHandSocket, diagnostic),
                 diagnostic.c_str())) return 1;
    if (!Require(std::isfinite(leftHandSocket[12]) && std::isfinite(rightHandSocket[12]) &&
                 std::abs(leftHandSocket[12] - rightHandSocket[12]) > 0.2f,
                 "actual hand bone socket transforms must be finite and side-stable")) return 1;

    std::vector<SkinnedRtVertex> skeletonPlain;
    std::vector<TexturedSkinnedRtVertex> skeletonTextured;
    if (!Require(skeleton.Skin(SkinnedClip::Idle, 0.25f, skeletonPlain, diagnostic), diagnostic.c_str())) return 1;
    if (!Require(skeleton.SkinTextured(SkinnedClip::Idle, 0.25f, skeletonTextured, diagnostic), diagnostic.c_str())) return 1;
    if (!Require(skeletonPlain.size() == skeletonTextured.size(), "textured skinning changed skeleton vertex count")) return 1;
    if (!Require(FiniteTexturedVertices(skeletonTextured), "skeleton textured vertices are invalid")) return 1;

    SkinnedCharacterModel player;
    const auto playerPath = root / "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb";
    if (!Require(player.LoadClips(playerPath.string(), PlayerLocomotionClipSet(), diagnostic),
                 diagnostic.c_str())) return 1;
    if (!Require(player.HasTexcoords() && player.ExpandedVertexCount() == 46785u,
                 "player three-primitive textured skin layout changed")) return 1;
    const auto& playerPrimitives = player.PrimitiveRanges();
    if (!Require(playerPrimitives.size() == 3u &&
                 playerPrimitives[0].materialName == "BodyPrimaryVisible" &&
                 playerPrimitives[1].materialName == "HeadPrimaryMasked" &&
                 playerPrimitives[2].materialName == "NearFacePrimaryMasked" &&
                 playerPrimitives[0].expandedVertexCount == 40602u &&
                 playerPrimitives[1].expandedVertexCount == 5439u &&
                 playerPrimitives[2].expandedVertexCount == 744u,
                 "player primitive semantics or triangle ranges changed")) return 1;
    if (!Require(player.HasNode("LeftHand") && player.HasNode("RightHand") &&
                 player.ClipDuration(SkinnedClip::Idle) > 0.9f &&
                 player.ClipDuration(SkinnedClip::Walking) > 0.75f &&
                 player.ClipDuration(SkinnedClip::Attack) == 0.0f &&
                 player.ClipDuration(SkinnedClip::Dead) == 0.0f,
                 "player joint or idle/walk-only clip contract changed")) return 1;
    std::vector<TexturedSkinnedRtVertex> playerIdle;
    std::vector<TexturedSkinnedRtVertex> playerWalking;
    if (!Require(player.SkinTextured(SkinnedClip::Idle, 0.25f, playerIdle, diagnostic),
                 diagnostic.c_str()) ||
        !Require(player.SkinTextured(SkinnedClip::Walking, 0.25f, playerWalking, diagnostic),
                 diagnostic.c_str()) ||
        !Require(FiniteTexturedVertices(playerIdle) && FiniteTexturedVertices(playerWalking) &&
                 playerIdle.size() == playerWalking.size(),
                 "player idle/walk skinning output is invalid")) return 1;

    horde::scene::assets::AssetManifest playerManifest;
    horde::scene::assets::StaticMeshAsset playerStatic;
    if (!Require(horde::scene::assets::AssetManifest::Load(
                     root / "assets/models/player/runtime/asset.manifest.json",
                     playerManifest, diagnostic), diagnostic.c_str()) ||
        !Require(horde::scene::assets::StaticMeshAsset::Load(
                     playerPath, playerManifest, playerStatic, diagnostic), diagnostic.c_str()) ||
        !Require(player.UniqueVertexCount() == playerStatic.vertices.size(),
                 "static-PBR and skinned player vertex streams must have identical unique ordering")) return 1;

    SkinnedNodeTransform leftArmBase{};
    SkinnedNodeTransform rightArmBase{};
    if (!Require(player.NodeTransform(SkinnedClip::Idle, 0.0f, "LeftArm", leftArmBase, diagnostic),
                 diagnostic.c_str()) ||
        !Require(player.NodeTransform(SkinnedClip::Idle, 0.0f, "RightArm", rightArmBase, diagnostic),
                 diagnostic.c_str())) return 1;
    const SkinnedArmIkTarget leftTarget{{{leftArmBase[12] - 0.12f, leftArmBase[13] - 0.20f,
                                           leftArmBase[14] + 0.28f}},
                                         {{-1.0f, -0.2f, 0.2f}}};
    const SkinnedArmIkTarget rightTarget{{{rightArmBase[12] + 0.12f, rightArmBase[13] - 0.20f,
                                            rightArmBase[14] + 0.28f}},
                                          {{1.0f, -0.2f, 0.2f}}};
    std::vector<TexturedSkinnedRtVertex> playerSolved;
    SkinnedPlayerSockets playerSockets;
    if (!Require(player.SkinPlayerUniqueTextured(SkinnedClip::Idle, 0.25f,
                                                  leftTarget, rightTarget,
                                                  playerSolved, playerSockets,
                                                  diagnostic), diagnostic.c_str()) ||
        !Require(playerSolved.size() == playerStatic.vertices.size() &&
                 FiniteTexturedVertices(playerSolved),
                 "player IK skin output must remain finite and static-PBR-addressable")) return 1;
    const auto socketDistance = [](const SkinnedNodeTransform& socket,
                                   const SkinnedArmIkTarget& target) {
        return std::hypot(std::hypot(socket[12] - target.target[0],
                                    socket[13] - target.target[1]),
                          socket[14] - target.target[2]);
    };
    if (!Require(socketDistance(playerSockets.leftHand, leftTarget) <= 0.015f &&
                 socketDistance(playerSockets.rightHand, rightTarget) <= 0.015f,
                 "actual LeftHand/RightHand bone sockets must solve within 15 mm")) return 1;

    horde::gameplay::simulation::GameSimulation simulation;
    horde::gameplay::simulation::InputSnapshot walkingInput{};
    walkingInput.moveForward = 1.0f;
    simulation.StepFixed(walkingInput);
    horde::vulkan::raytracing::PlayerRenderSlot playerSlot;
    bool poseUpdated = false;
    const auto authoritativeRigSnapshot = simulation.Snapshot().playerAnimation;
    auto rigSnapshot = authoritativeRigSnapshot;
    for (std::size_t component = 0u; component < 3u; ++component)
    {
        rigSnapshot.leftIk.shoulder[component] = leftArmBase[12u + component];
        rigSnapshot.leftIk.target[component] = rigSnapshot.leftIk.shoulder[component] +
            authoritativeRigSnapshot.leftIk.target[component] -
            authoritativeRigSnapshot.leftIk.shoulder[component];
        rigSnapshot.rightIk.shoulder[component] = rightArmBase[12u + component];
        rigSnapshot.rightIk.target[component] = rigSnapshot.rightIk.shoulder[component] +
            authoritativeRigSnapshot.rightIk.target[component] -
            authoritativeRigSnapshot.rightIk.shoulder[component];
    }
    const auto sameArmDelta = [](const auto& modelArm, const auto& authoritativeArm) {
        for (std::size_t component = 0u; component < 3u; ++component)
        {
            const float modelDelta = modelArm.target[component] - modelArm.shoulder[component];
            const float authoritativeDelta = authoritativeArm.target[component] -
                                             authoritativeArm.shoulder[component];
            if (std::abs(modelDelta - authoritativeDelta) > 0.0001f) return false;
        }
        return true;
    };
    if (!sameArmDelta(rigSnapshot.leftIk, authoritativeRigSnapshot.leftIk) ||
        !sameArmDelta(rigSnapshot.rightIk, authoritativeRigSnapshot.rightIk))
    {
        std::cerr << "FAIL: isolated player fixture mixed view/model spaces: leftBase="
                  << leftArmBase[12] << ',' << leftArmBase[13] << ',' << leftArmBase[14]
                  << " viewShoulder=" << authoritativeRigSnapshot.leftIk.shoulder[0] << ','
                  << authoritativeRigSnapshot.leftIk.shoulder[1] << ','
                  << authoritativeRigSnapshot.leftIk.shoulder[2]
                  << " modelTarget=" << rigSnapshot.leftIk.target[0] << ','
                  << rigSnapshot.leftIk.target[1] << ',' << rigSnapshot.leftIk.target[2]
                  << '\n';
        return 1;
    }
    if (!playerSlot.LoadAsset(playerPath.string(), diagnostic))
    {
        std::cerr << "FAIL: " << diagnostic << '\n';
        return 1;
    }
    if (!playerSlot.PreparePose(
            rigSnapshot,
            simulation.Snapshot().tickIndex,
            horde::vulkan::raytracing::PlayerCpuSkinCadence::Hz60,
            poseUpdated, diagnostic))
    {
        std::cerr << "FAIL: " << diagnostic << '\n';
        return 1;
    }
    if (
        !Require(poseUpdated &&
                 playerSlot.LeftSocketErrorMetres() <= 0.015f &&
                 playerSlot.RightSocketErrorMetres() <= 0.015f,
                 "authoritative held-item targets must drive the final rig bone sockets")) return 1;

    const auto* restCheckpoint = horde::gameplay::FindDevelopmentCheckpoint(106);
    const auto* downCheckpoint = horde::gameplay::FindDevelopmentCheckpoint(107);
    const auto* upCheckpoint = horde::gameplay::FindDevelopmentCheckpoint(108);
    const auto* rewardHighCheckpoint = horde::gameplay::FindDevelopmentCheckpoint(116);
    const auto* rewardLowCheckpoint = horde::gameplay::FindDevelopmentCheckpoint(117);
    horde::gameplay::simulation::GameSimulation restSimulation;
    horde::gameplay::simulation::GameSimulation downSimulation;
    horde::gameplay::simulation::GameSimulation upSimulation;
    horde::gameplay::simulation::GameSimulation rewardHighSimulation;
    horde::gameplay::simulation::GameSimulation rewardLowSimulation;
    if (!Require(restCheckpoint != nullptr && downCheckpoint != nullptr &&
                 upCheckpoint != nullptr && rewardHighCheckpoint != nullptr &&
                 rewardLowCheckpoint != nullptr &&
                 horde::gameplay::StageDevelopmentCheckpointSimulation(
                     restSimulation, *restCheckpoint) &&
                 horde::gameplay::StageDevelopmentCheckpointSimulation(
                     downSimulation, *downCheckpoint) &&
                 horde::gameplay::StageDevelopmentCheckpointSimulation(
                     upSimulation, *upCheckpoint) &&
                 horde::gameplay::StageDevelopmentCheckpointSimulation(
                     rewardHighSimulation, *rewardHighCheckpoint) &&
                 horde::gameplay::StageDevelopmentCheckpointSimulation(
                     rewardLowSimulation, *rewardLowCheckpoint),
                 "rest/down/up and reward high/low direct checkpoints must import and freeze deterministically"))
        return 1;

    struct ResolvedPlayerPose
    {
        horde::gameplay::items::HeldItemStates authoritative{};
        horde::gameplay::items::HeldItemStates rendered{};
        float maximumPositionErrorMetres = 0.0f;
        float maximumOrientationErrorRadians = 0.0f;
    };
    const auto resolvePlayerPose = [&](horde::vulkan::raytracing::PlayerRenderSlot& slot,
                                       const horde::gameplay::simulation::SimulationSnapshot& source,
                                       const std::uint64_t tick,
                                       ResolvedPlayerPose& result) {
        const TestRigFrame rig = BuildRigFrame(source, leftArmBase, rightArmBase);
        bool updated = false;
        if (!slot.PreparePose(rig.animation, tick,
                              horde::vulkan::raytracing::PlayerCpuSkinCadence::Hz60,
                              updated, diagnostic) || !updated)
            return false;
        result.authoritative = source.heldItems;
        const auto& bones = slot.BoneSockets();
        if (!slot.ResolveHeldItemVisuals(
                result.authoritative,
                RigidWorldBoneTransform(bones.leftHand, rig),
                RigidWorldBoneTransform(bones.rightHand, rig),
                result.rendered, diagnostic))
            return false;
        result.maximumPositionErrorMetres = 0.0f;
        result.maximumOrientationErrorRadians = 0.0f;
        for (std::size_t item = 0u; item < result.rendered.size(); ++item)
        {
            const HeldItemTransform intendedGrip = FinalGrip(result.authoritative[item]);
            const HeldItemTransform finalGrip = FinalGrip(result.rendered[item]);
            result.maximumPositionErrorMetres = std::max(
                result.maximumPositionErrorMetres,
                PositionError(intendedGrip, finalGrip));
            result.maximumOrientationErrorRadians = std::max(
                result.maximumOrientationErrorRadians,
                OrientationError(intendedGrip, finalGrip));
        }
        return true;
    };

    horde::vulkan::raytracing::PlayerRenderSlot restFirstSlot;
    horde::vulkan::raytracing::PlayerRenderSlot downFirstSlot;
    horde::vulkan::raytracing::PlayerRenderSlot upFirstSlot;
    horde::vulkan::raytracing::PlayerRenderSlot resetSlot;
    horde::vulkan::raytracing::PlayerRenderSlot frozenSlot;
    horde::vulkan::raytracing::PlayerRenderSlot sameTickSlot;
    if (!restFirstSlot.LoadAsset(playerPath.string(), diagnostic) ||
        !downFirstSlot.LoadAsset(playerPath.string(), diagnostic) ||
        !upFirstSlot.LoadAsset(playerPath.string(), diagnostic) ||
        !resetSlot.LoadAsset(playerPath.string(), diagnostic) ||
        !frozenSlot.LoadAsset(playerPath.string(), diagnostic) ||
        !sameTickSlot.LoadAsset(playerPath.string(), diagnostic))
    {
        std::cerr << "FAIL: stable rest basis load: " << diagnostic << '\n';
        return 1;
    }

    const TestRigFrame highRig = BuildRigFrame(
        rewardHighSimulation.Snapshot(), leftArmBase, rightArmBase);
    const TestRigFrame lowRig = BuildRigFrame(
        rewardLowSimulation.Snapshot(), leftArmBase, rightArmBase);
    bool sameTickUpdated = false;
    horde::gameplay::items::HeldItemStates sameTickRendered{};
    if (!sameTickSlot.PreparePose(
            highRig.animation, 77u,
            horde::vulkan::raytracing::PlayerCpuSkinCadence::Hz60,
            sameTickUpdated, diagnostic) || !sameTickUpdated ||
        !sameTickSlot.ResolveHeldItemVisuals(
            rewardHighSimulation.Snapshot().heldItems,
            RigidWorldBoneTransform(sameTickSlot.BoneSockets().leftHand, highRig),
            RigidWorldBoneTransform(sameTickSlot.BoneSockets().rightHand, highRig),
            sameTickRendered, diagnostic))
    {
        std::cerr << "FAIL: initial same-tick high pose: " << diagnostic << '\n';
        return 1;
    }
    const HeldItemTransform highFinalGrip = sameTickSlot.FinalWorldFromLeftGrip();
    sameTickUpdated = false;
    if (!sameTickSlot.PreparePose(
            lowRig.animation, 77u,
            horde::vulkan::raytracing::PlayerCpuSkinCadence::Hz60,
            sameTickUpdated, diagnostic) || !sameTickUpdated ||
        !sameTickSlot.ResolveHeldItemVisuals(
            rewardLowSimulation.Snapshot().heldItems,
            RigidWorldBoneTransform(sameTickSlot.BoneSockets().leftHand, lowRig),
            RigidWorldBoneTransform(sameTickSlot.BoneSockets().rightHand, lowRig),
            sameTickRendered, diagnostic))
    {
        std::cerr << "FAIL: changed same-tick low pose: " << diagnostic << '\n';
        return 1;
    }
    const HeldItemTransform lowFinalGrip = sameTickSlot.FinalWorldFromLeftGrip();
    sameTickUpdated = true;
    if (!Require(PositionError(highFinalGrip, lowFinalGrip) >= 0.20f,
                 "same-tick changed animation must refresh skinning and publish the changed final left Grip") ||
        !Require(sameTickSlot.PreparePose(
                     lowRig.animation, 77u,
                     horde::vulkan::raytracing::PlayerCpuSkinCadence::Hz60,
                     sameTickUpdated, diagnostic) && !sameTickUpdated &&
                     PositionError(lowFinalGrip,
                                   sameTickSlot.FinalWorldFromLeftGrip()) <= 0.000001f,
                 "same-tick unchanged animation must reuse the cached skin and final Grip"))
        return 1;

    ResolvedPlayerPose restInitial;
    ResolvedPlayerPose downInitial;
    ResolvedPlayerPose upInitial;
    ResolvedPlayerPose ignoredResetInitial;
    ResolvedPlayerPose frozenDownInitial;
    if (!resolvePlayerPose(restFirstSlot, restSimulation.Snapshot(), 1u, restInitial) ||
        !resolvePlayerPose(downFirstSlot, downSimulation.Snapshot(), 1u, downInitial) ||
        !resolvePlayerPose(upFirstSlot, upSimulation.Snapshot(), 1u, upInitial) ||
        !resolvePlayerPose(resetSlot, downSimulation.Snapshot(), 1u,
                           ignoredResetInitial) ||
        !resolvePlayerPose(frozenSlot, downSimulation.Snapshot(), 1u,
                           frozenDownInitial))
    {
        std::cerr << "FAIL: initial stable-basis pose: " << diagnostic << '\n';
        return 1;
    }

    if (!resetSlot.LoadAsset(playerPath.string(), diagnostic))
    {
        std::cerr << "FAIL: stable-basis reset: " << diagnostic << '\n';
        return 1;
    }
    ResolvedPlayerPose restAfterRestFirst;
    ResolvedPlayerPose restAfterDownFirst;
    ResolvedPlayerPose restAfterUpFirst;
    ResolvedPlayerPose restAfterReset;
    ResolvedPlayerPose frozenDownRepeated;
    if (!resolvePlayerPose(restFirstSlot, restSimulation.Snapshot(), 2u,
                           restAfterRestFirst) ||
        !resolvePlayerPose(downFirstSlot, restSimulation.Snapshot(), 2u,
                           restAfterDownFirst) ||
        !resolvePlayerPose(upFirstSlot, restSimulation.Snapshot(), 2u,
                           restAfterUpFirst) ||
        !resolvePlayerPose(resetSlot, restSimulation.Snapshot(), 2u,
                           restAfterReset) ||
        !resolvePlayerPose(frozenSlot, downSimulation.Snapshot(), 2u,
                           frozenDownRepeated))
    {
        std::cerr << "FAIL: ordered stable-basis rest pose: " << diagnostic << '\n';
        return 1;
    }

    const auto agreesWithAuthoritativeGrip = [](const ResolvedPlayerPose& pose) {
        return pose.maximumPositionErrorMetres <= 0.015f &&
               pose.maximumOrientationErrorRadians <=
                   horde::vulkan::raytracing::kPlayerGripOrientationToleranceRadians;
    };
    if (!Require(agreesWithAuthoritativeGrip(restInitial) &&
                 agreesWithAuthoritativeGrip(downInitial) &&
                 agreesWithAuthoritativeGrip(upInitial) &&
                 agreesWithAuthoritativeGrip(restAfterRestFirst) &&
                 agreesWithAuthoritativeGrip(restAfterReset),
                 "final composed item Grip must agree with the authoritative hand pivot in position and orientation"))
        return 1;
    std::cout << "Stable final Grip agreement rest="
              << restInitial.maximumPositionErrorMetres << "m/"
              << restInitial.maximumOrientationErrorRadians << "rad down="
              << downInitial.maximumPositionErrorMetres << "m/"
              << downInitial.maximumOrientationErrorRadians << "rad up="
              << upInitial.maximumPositionErrorMetres << "m/"
              << upInitial.maximumOrientationErrorRadians << "rad\n";

    const auto sameFinalGrips = [](const ResolvedPlayerPose& left,
                                   const ResolvedPlayerPose& right) {
        for (std::size_t item = 0u; item < left.rendered.size(); ++item)
        {
            const HeldItemTransform leftGrip = FinalGrip(left.rendered[item]);
            const HeldItemTransform rightGrip = FinalGrip(right.rendered[item]);
            if (PositionError(leftGrip, rightGrip) > 0.0001f ||
                OrientationError(leftGrip, rightGrip) > 0.001f)
                return false;
        }
        return true;
    };
    if (!agreesWithAuthoritativeGrip(restAfterDownFirst) ||
        !agreesWithAuthoritativeGrip(restAfterUpFirst) ||
        !sameFinalGrips(restAfterRestFirst, restAfterDownFirst) ||
        !sameFinalGrips(restAfterRestFirst, restAfterUpFirst) ||
        !sameFinalGrips(restAfterRestFirst, restAfterReset) ||
        !sameFinalGrips(frozenDownInitial, frozenDownRepeated))
    {
        std::cerr << "FAIL: player rest Grip basis depends on first rendered checkpoint: "
                  << "rest=" << restAfterRestFirst.maximumOrientationErrorRadians
                  << " down-first=" << restAfterDownFirst.maximumOrientationErrorRadians
                  << " up-first=" << restAfterUpFirst.maximumOrientationErrorRadians
                  << " reset=" << restAfterReset.maximumOrientationErrorRadians << '\n';
        return 1;
    }

    SkinnedCharacterModel lich;
    const auto lichPath = root / "assets/models/enemies/meshy/lich_placeholder_merged_animations_v01.glb";
    if (!Require(lich.LoadClips(lichPath.string(), LichPlaceholderClipSet(), diagnostic), diagnostic.c_str())) return 1;
    if (!Require(lich.HasTexcoords(), "lich TEXCOORD_0 was not imported")) return 1;
    if (!Require(lich.ExpandedVertexCount() == 27564u, "lich expanded vertex count changed")) return 1;
    if (!Require(lich.ClipDuration(SkinnedClip::Idle) > 2.3f, "lich idle mapping is wrong")) return 1;
    if (!Require(lich.ClipDuration(SkinnedClip::Walking) > 1.0f, "lich walking mapping is wrong")) return 1;
    if (!Require(lich.ClipDuration(SkinnedClip::Attack) == 0.0f, "lich attack must remain explicitly unmapped")) return 1;
    if (!Require(lich.ClipDuration(SkinnedClip::Dead) > 2.9f, "lich death mapping is wrong")) return 1;

    std::vector<TexturedSkinnedRtVertex> lichTextured;
    if (!Require(lich.SkinTextured(SkinnedClip::Idle, 0.5f, lichTextured, diagnostic), diagnostic.c_str())) return 1;
    if (!Require(FiniteTexturedVertices(lichTextured), "lich textured vertices are invalid")) return 1;
    std::ifstream emissionFile(root / "assets/textures/meshy/lich_placeholder_v01/emissive-2048-rgba8.ktx2",
                               std::ios::binary | std::ios::ate);
    if (!Require(static_cast<bool>(emissionFile), "derived lich emission audit KTX2 is missing")) return 1;
    const std::size_t emissionFileSize = static_cast<std::size_t>(emissionFile.tellg());
    std::vector<unsigned char> emissionKtx(emissionFileSize);
    emissionFile.seekg(0, std::ios::beg);
    emissionFile.read(reinterpret_cast<char*>(emissionKtx.data()), static_cast<std::streamsize>(emissionKtx.size()));
    const auto readU32 = [&emissionKtx](std::size_t offset) {
        return static_cast<std::uint32_t>(emissionKtx[offset]) |
               (static_cast<std::uint32_t>(emissionKtx[offset + 1u]) << 8u) |
               (static_cast<std::uint32_t>(emissionKtx[offset + 2u]) << 16u) |
               (static_cast<std::uint32_t>(emissionKtx[offset + 3u]) << 24u);
    };
    const auto readU64 = [&readU32](std::size_t offset) {
        return static_cast<std::uint64_t>(readU32(offset)) |
               (static_cast<std::uint64_t>(readU32(offset + 4u)) << 32u);
    };
    constexpr unsigned char kKtx2Identifier[12] = {
        0xABu, 0x4Bu, 0x54u, 0x58u, 0x20u, 0x32u, 0x30u, 0xBBu, 0x0Du, 0x0Au, 0x1Au, 0x0Au};
    bool validIdentifier = emissionKtx.size() >= 104u;
    for (std::size_t i = 0; validIdentifier && i < 12u; ++i)
    {
        validIdentifier = emissionKtx[i] == kKtx2Identifier[i];
    }
    if (!Require(validIdentifier && readU32(12u) == 43u && readU32(20u) == 2048u &&
                 readU32(24u) == 2048u && readU32(40u) == 1u,
                 "derived lich emission audit KTX2 header changed")) return 1;
    const std::uint64_t levelOffset = readU64(80u);
    const std::uint64_t levelLength = readU64(88u);
    if (!Require(levelLength == 2048ull * 2048ull * 4ull &&
                 levelOffset <= emissionKtx.size() && levelLength <= emissionKtx.size() - levelOffset,
                 "derived lich emission KTX2 payload is invalid")) return 1;
    std::vector<unsigned char> emissionPixels(
        emissionKtx.begin() + static_cast<std::ptrdiff_t>(levelOffset),
        emissionKtx.begin() + static_cast<std::ptrdiff_t>(levelOffset + levelLength));
    std::size_t emissiveVertexCount = 0u;
    std::size_t outerEmissiveVertexCount = 0u;
    float outerX = 0.0f, outerY = 0.0f, outerZ = 0.0f;
    float emissiveMinX = 1.0e9f, emissiveMaxX = -1.0e9f;
    for (std::size_t vertexIndex = 0; vertexIndex < lichTextured.size(); ++vertexIndex)
    {
        const auto& vertex = lichTextured[vertexIndex];
        const float wrappedU = vertex.texcoord[0] - std::floor(vertex.texcoord[0]);
        const float wrappedV = vertex.texcoord[1] - std::floor(vertex.texcoord[1]);
        const std::size_t x = std::min<std::size_t>(2047u, static_cast<std::size_t>(wrappedU * 2048.0f));
        const std::size_t y = std::min<std::size_t>(2047u, static_cast<std::size_t>(wrappedV * 2048.0f));
        const std::size_t offset = (y * 2048u + x) * 4u;
        if (offset + 2u < emissionPixels.size() &&
            (emissionPixels[offset] || emissionPixels[offset + 1u] || emissionPixels[offset + 2u]))
        {
            ++emissiveVertexCount;
            emissiveMinX = std::min(emissiveMinX, vertex.position[0]);
            emissiveMaxX = std::max(emissiveMaxX, vertex.position[0]);
            if (vertex.position[0] > 0.55f)
            {
                ++outerEmissiveVertexCount;
                outerX += vertex.position[0];
                outerY += vertex.position[1];
                outerZ += vertex.position[2];
            }
        }
    }
    if (!Require(emissiveVertexCount > 0u, "no emissive lich vertices were found by UV audit")) return 1;
    if (!Require(outerEmissiveVertexCount == 40u, "audited staff crystal vertex set changed")) return 1;
    if (!Require(outerX / 40.0f > 0.90f && outerY / 40.0f > 0.70f,
                 "audited staff crystal sample moved into the robe or eye cluster")) return 1;
    std::vector<SkinnedRtVertex> unavailableAttack;
    if (!Require(!lich.Skin(SkinnedClip::Attack, 0.5f, unavailableAttack, diagnostic), "unmapped lich attack unexpectedly skinned")) return 1;

    std::cout << "Skinned character model smoke passed: skeleton=" << skeleton.ExpandedVertexCount()
              << " player=" << player.ExpandedVertexCount()
              << " lich=" << lich.ExpandedVertexCount() << " textured std430 stride=" << sizeof(TexturedSkinnedRtVertex)
              << " emissiveVertices=" << emissiveVertexCount << " emissiveX=" << emissiveMinX << ".." << emissiveMaxX
              << " outer=" << outerEmissiveVertexCount << " avg="
              << outerX / static_cast<float>(std::max<std::size_t>(1u, outerEmissiveVertexCount)) << ','
              << outerY / static_cast<float>(std::max<std::size_t>(1u, outerEmissiveVertexCount)) << ','
              << outerZ / static_cast<float>(std::max<std::size_t>(1u, outerEmissiveVertexCount)) << '\n';
    return 0;
}
