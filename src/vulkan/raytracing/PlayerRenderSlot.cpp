#include "vulkan/raytracing/PlayerRenderSlot.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace horde::vulkan::raytracing
{

namespace
{

using HeldItemTransform = horde::gameplay::items::HeldItemTransform;
using Vec3 = std::array<float, 3u>;

Vec3 Add(const Vec3& left, const Vec3& right)
{
    return {{left[0] + right[0], left[1] + right[1], left[2] + right[2]}};
}

Vec3 Subtract(const Vec3& left, const Vec3& right)
{
    return {{left[0] - right[0], left[1] - right[1], left[2] - right[2]}};
}

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

Vec3 Column(const HeldItemTransform& transform, const std::size_t column)
{
    const std::size_t offset = column * 4u;
    return {{transform[offset], transform[offset + 1u], transform[offset + 2u]}};
}

horde::gameplay::items::HeldItemTransform InverseRigid(
    const horde::gameplay::items::HeldItemTransform& transform)
{
    using horde::gameplay::items::HeldItemTransform;
    HeldItemTransform result = horde::gameplay::items::IdentityHeldItemTransform();
    for (std::size_t column = 0u; column < 3u; ++column)
        for (std::size_t row = 0u; row < 3u; ++row)
            result[column * 4u + row] = transform[row * 4u + column];
    for (std::size_t row = 0u; row < 3u; ++row)
    {
        result[12u + row] = -(result[row] * transform[12] +
                              result[4u + row] * transform[13] +
                              result[8u + row] * transform[14]);
    }
    return result;
}

HeldItemTransform RigidPlayerBoneToWorld(
    const horde::scene::SkinnedNodeTransform& bone,
    const Vec3& bodyRight,
    const Vec3& bodyForward,
    const Vec3& playerRootWorld)
{
    const auto vectorToWorld = [&bodyRight, &bodyForward](const Vec3& local) {
        return Add(Add(Scale(bodyRight, local[0]), {{0.0f, local[1], 0.0f}}),
                   Scale(bodyForward, local[2]));
    };
    Vec3 x = Normalise(vectorToWorld({{bone[0], bone[1], bone[2]}}));
    const Vec3 rawY = vectorToWorld({{bone[4], bone[5], bone[6]}});
    Vec3 y = Normalise(Subtract(rawY, Scale(x, Dot(rawY, x))));
    Vec3 z = Normalise(Cross(x, y));
    const Vec3 rawZ = vectorToWorld({{bone[8], bone[9], bone[10]}});
    if (Dot(z, rawZ) < 0.0f)
    {
        y = Scale(y, -1.0f);
        z = Scale(z, -1.0f);
    }
    const Vec3 position = Add(
        playerRootWorld, vectorToWorld({{bone[12], bone[13], bone[14]}}));
    HeldItemTransform result = horde::gameplay::items::IdentityHeldItemTransform();
    result[0] = x[0]; result[1] = x[1]; result[2] = x[2];
    result[4] = y[0]; result[5] = y[1]; result[6] = y[2];
    result[8] = z[0]; result[9] = z[1]; result[10] = z[2];
    result[12] = position[0]; result[13] = position[1]; result[14] = position[2];
    return result;
}

HeldItemTransform RigidPlayerBoneInModel(
    const horde::scene::SkinnedNodeTransform& bone)
{
    Vec3 x = Normalise({{bone[0], bone[1], bone[2]}});
    const Vec3 rawY{{bone[4], bone[5], bone[6]}};
    Vec3 y = Normalise(Subtract(rawY, Scale(x, Dot(rawY, x))));
    Vec3 z = Normalise(Cross(x, y));
    const Vec3 rawZ{{bone[8], bone[9], bone[10]}};
    if (Dot(z, rawZ) < 0.0f)
    {
        y = Scale(y, -1.0f);
        z = Scale(z, -1.0f);
    }
    HeldItemTransform result = horde::gameplay::items::IdentityHeldItemTransform();
    result[0] = x[0]; result[1] = x[1]; result[2] = x[2];
    result[4] = y[0]; result[5] = y[1]; result[6] = y[2];
    result[8] = z[0]; result[9] = z[1]; result[10] = z[2];
    result[12] = bone[12]; result[13] = bone[13]; result[14] = bone[14];
    return result;
}

HeldItemTransform TransformFromAxes(const Vec3& x,
                                    const Vec3& y,
                                    const Vec3& z,
                                    const Vec3& position = {})
{
    return {{x[0], x[1], x[2], 0.0f,
             y[0], y[1], y[2], 0.0f,
             z[0], z[1], z[2], 0.0f,
             position[0], position[1], position[2], 1.0f}};
}

float GripOrientationError(const HeldItemTransform& left,
                           const HeldItemTransform& right)
{
    float maximum = 0.0f;
    for (std::size_t column = 0u; column < 3u; ++column)
    {
        const std::size_t offset = column * 4u;
        const Vec3 leftAxis{{left[offset], left[offset + 1u], left[offset + 2u]}};
        const Vec3 rightAxis{{right[offset], right[offset + 1u], right[offset + 2u]}};
        const float cosine = std::clamp(
            Dot(Normalise(leftAxis), Normalise(rightAxis)),
            -1.0f, 1.0f);
        maximum = std::max(maximum, std::acos(cosine));
    }
    return maximum;
}

} // namespace

PlayerRouteMasks BuildPlayerRouteMasks(const PlayerRenderRoute route)
{
    PlayerRouteMasks result;
    if (route == PlayerRenderRoute::Skinned)
    {
        // One skinned full body participates in primary-body and reflection
        // rays. Head/near-face primary exclusion is primitive metadata, not a
        // second hidden instance.
        result.instanceMasks[4] = 0x14u;
        return result;
    }
    result.instanceMasks[4] = 0x10u;
    for (std::size_t slot = 5u; slot <= 15u; ++slot)
    {
        result.instanceMasks[slot] = 0x04u;
    }
    result.instanceMasks[16] = 0x10u;
    return result;
}

ProductionSceneVisibility BuildProductionSceneVisibility(
    const ProductionSceneVisibilityInput& input)
{
    ProductionSceneVisibility result;
    result.rewardWorldVisible = !input.glassFixtureVisible;
    result.inspectionOverride = input.productionInspection;
    // The production reward world owns TLAS slots 5-8 (chest, lid, ring,
    // body), which are the legacy procedural arm slots. Keep the single
    // skinned player in slot 4 whenever either production world is present;
    // otherwise normal opening frames silently lose both arms even though the
    // torch and sword remain visible.
    result.playerRoute = (result.rewardWorldVisible || input.glassFixtureVisible)
        ? PlayerRenderRoute::Skinned
        : input.requestedPlayerRoute;
    const PlayerRouteMasks playerMasks = BuildPlayerRouteMasks(result.playerRoute);
    result.torchMask = input.productionInspection ? 0u : 0x02u;
    result.swordMask = input.productionInspection ? 0u : 0x02u;
    result.playerMask = input.productionInspection
        ? 0u : playerMasks.instanceMasks[4];
    result.playerPrimaryVisible = result.playerRoute == PlayerRenderRoute::Procedural
        ? !input.productionInspection
        : (result.playerMask & 0x04u) != 0u;
    result.playerReflectionVisible = (result.playerMask & 0x10u) != 0u;
    return result;
}

std::vector<PlayerPrimitiveVisibility> BuildPlayerPrimitiveVisibility(
    const std::initializer_list<PlayerPrimitiveSemantic> semantics)
{
    std::vector<PlayerPrimitiveVisibility> result;
    result.reserve(semantics.size());
    for (const PlayerPrimitiveSemantic semantic : semantics)
    {
        PlayerPrimitiveVisibility visibility;
        visibility.primaryVisible = semantic == PlayerPrimitiveSemantic::Body;
        result.push_back(visibility);
    }
    return result;
}

PlayerSocketPlan EvaluatePlayerSocketPlan(
    const horde::gameplay::animation::PlayerAnimationSnapshot& animation)
{
    using namespace horde::gameplay::animation;
    PlayerSocketPlan result;
    result.leftArm = SolveTwoBoneIk(
        animation.leftIk.shoulder,
        animation.leftIk.target,
        animation.leftIk.pole,
        animation.leftIk.upperArmLength,
        animation.leftIk.lowerArmLength);
    result.rightArm = SolveTwoBoneIk(
        animation.rightIk.shoulder,
        animation.rightIk.target,
        animation.rightIk.pole,
        animation.rightIk.upperArmLength,
        animation.rightIk.lowerArmLength);
    result.localFromLeftHand = BuildHandBoneSocketTransform(
        result.leftArm.hand, {{0.0f, 0.0f, 1.0f}}, {{0.0f, 1.0f, 0.0f}});
    result.localFromRightHand = BuildHandBoneSocketTransform(
        result.rightArm.hand, {{0.0f, 0.0f, 1.0f}}, {{0.0f, 1.0f, 0.0f}});
    result.leftErrorMetres = MeasureHandSocketError(
        result.localFromLeftHand, animation.leftIk.target);
    result.rightErrorMetres = MeasureHandSocketError(
        result.localFromRightHand, animation.rightIk.target);
    return result;
}

bool ResolvePlayerHeldItemVisuals(
    const horde::gameplay::items::HeldItemStates& authoritativeItems,
    const horde::gameplay::items::HeldItemTransform& worldFromLeftHandBone,
    const horde::gameplay::items::HeldItemTransform& worldFromRightHandBone,
    horde::gameplay::items::HeldItemStates& renderItems,
    std::string& diagnostic)
{
    using namespace horde::gameplay::items;
    renderItems = authoritativeItems;
    for (HeldItemState& item : renderItems)
    {
        // Once a held item detaches, GameSimulation's authored trajectory
        // remains the sole transform authority. Only attached visuals consume
        // the final rig socket; semantic light/audio state is unchanged.
        if (item.parentMode != HeldItemParentMode::HandSocket) continue;
        const HeldItemTransform& worldFromHand = SelectHandSocketTransform(
            item.hand, worldFromLeftHandBone, worldFromRightHandBone);
        const HeldItemTransform itemFromGrip = item.id == HeldItemId::OriginalTorch
            ? OriginalTorchGripSocketTransform()
            : SwordGripSocketTransform();
        if (!ComposeWorldFromItem(worldFromHand, itemFromGrip,
                                  item.worldFromItem, diagnostic))
            return false;
    }
    diagnostic.clear();
    return true;
}

PlayerGripAgreement MeasurePlayerGripAgreement(
    const horde::gameplay::items::HeldItemState& authoritativeItem,
    const horde::gameplay::items::HeldItemState& renderedItem)
{
    using namespace horde::gameplay::items;
    const HeldItemTransform itemFromGrip = authoritativeItem.id ==
            HeldItemId::OriginalTorch
        ? OriginalTorchGripSocketTransform()
        : SwordGripSocketTransform();
    const HeldItemTransform intendedGrip = MultiplyHeldItemTransforms(
        authoritativeItem.worldFromItem, itemFromGrip);
    const HeldItemTransform finalGrip = MultiplyHeldItemTransforms(
        renderedItem.worldFromItem, itemFromGrip);
    return MeasureTransformAgreement(intendedGrip, finalGrip);
}

PlayerGripAgreement MeasureTransformAgreement(
    const HeldItemTransform& intended,
    const HeldItemTransform& rendered)
{
    PlayerGripAgreement result;
    result.positionErrorMetres = std::hypot(
        std::hypot(intended[12] - rendered[12], intended[13] - rendered[13]),
        intended[14] - rendered[14]);
    result.orientationErrorRadians = GripOrientationError(intended, rendered);
    return result;
}

HeldItemTransform InverseRigidHeldItemTransform(const HeldItemTransform& transform)
{
    return InverseRigid(transform);
}

bool ComposeClaimedRewardLanternVisuals(
    const HeldItemTransform& worldFromFinalLeftGrip,
    const HeldItemTransform& ringFromGripRing,
    const HeldItemTransform& ringFromHinge,
    const HeldItemTransform& authoritativeWorldFromHinge,
    const HeldItemTransform& authoritativeWorldFromBody,
    const float uniformScale,
    RewardLanternVisualTransforms& output,
    std::string& diagnostic)
{
    using namespace horde::gameplay::items;
    if (!ValidateHeldItemSocketTransform(worldFromFinalLeftGrip, diagnostic) ||
        !ValidateHeldItemSocketTransform(ringFromGripRing, diagnostic) ||
        !ValidateHeldItemSocketTransform(ringFromHinge, diagnostic) ||
        !ValidateHeldItemSocketTransform(authoritativeWorldFromHinge, diagnostic) ||
        !ValidateHeldItemSocketTransform(authoritativeWorldFromBody, diagnostic) ||
        !std::isfinite(uniformScale) || uniformScale <= 0.0f)
    {
        diagnostic = "Claimed reward lantern requires finite rigid Grip/Hinge/body transforms and positive scale.";
        return false;
    }
    HeldItemTransform scale = IdentityHeldItemTransform();
    scale[0] = uniformScale;
    scale[5] = uniformScale;
    scale[10] = uniformScale;
    output.worldFromRing = MultiplyHeldItemTransforms(
        MultiplyHeldItemTransforms(worldFromFinalLeftGrip, scale),
        InverseRigid(ringFromGripRing));
    const HeldItemTransform scaledWorldFromHinge = MultiplyHeldItemTransforms(
        output.worldFromRing, ringFromHinge);
    output.worldFromHinge = TransformFromAxes(
        Normalise(Column(scaledWorldFromHinge, 0u)),
        Normalise(Column(scaledWorldFromHinge, 1u)),
        Normalise(Column(scaledWorldFromHinge, 2u)),
        {{scaledWorldFromHinge[12], scaledWorldFromHinge[13],
          scaledWorldFromHinge[14]}});
    const HeldItemTransform bodyFromAuthoritativeHinge = MultiplyHeldItemTransforms(
        InverseRigid(authoritativeWorldFromHinge), authoritativeWorldFromBody);
    output.worldFromBody = MultiplyHeldItemTransforms(
        output.worldFromHinge, bodyFromAuthoritativeHinge);
    const HeldItemTransform composedGrip = MultiplyHeldItemTransforms(
        output.worldFromRing, ringFromGripRing);
    output.gripAgreement = MeasureTransformAgreement(
        worldFromFinalLeftGrip, composedGrip);
    if (output.gripAgreement.positionErrorMetres > kPlayerGripSocketToleranceMetres ||
        output.gripAgreement.orientationErrorRadians >
            kPlayerGripOrientationToleranceRadians)
    {
        diagnostic = "Final composed reward GripRing exceeded position/orientation tolerance.";
        return false;
    }
    diagnostic.clear();
    return true;
}

PlayerCpuSkinCadence ChoosePlayerCpuCadence(
    const PlayerCpuCadenceMeasurements& measurements,
    const float acceptedMotionErrorMetres,
    const double acceptedCpuCostMilliseconds)
{
    const auto finiteCost = [](const double value) {
        return std::isfinite(value) ? value : std::numeric_limits<double>::infinity();
    };
    if (measurements.hz30MaximumMotionErrorMetres <= acceptedMotionErrorMetres &&
        finiteCost(measurements.hz30CostMilliseconds) <= acceptedCpuCostMilliseconds)
    {
        return PlayerCpuSkinCadence::Hz30;
    }
    if (measurements.hz60MaximumMotionErrorMetres <= acceptedMotionErrorMetres &&
        finiteCost(measurements.hz60CostMilliseconds) <= acceptedCpuCostMilliseconds)
    {
        return PlayerCpuSkinCadence::Hz60;
    }
    return PlayerCpuSkinCadence::RequiresReviewedBackend;
}

bool PlayerPoseNeedsRefresh(const std::uint64_t tickIndex,
                            const std::uint64_t lastSkinnedTick,
                            const PlayerCpuSkinCadence cadence)
{
    if (lastSkinnedTick == std::numeric_limits<std::uint64_t>::max() || tickIndex < lastSkinnedTick)
    {
        return true;
    }
    if (cadence == PlayerCpuSkinCadence::Hz60)
    {
        return tickIndex != lastSkinnedTick;
    }
    if (cadence == PlayerCpuSkinCadence::Hz30)
    {
        return tickIndex - lastSkinnedTick >= 2u;
    }
    return false;
}

bool PlayerRenderSlot::LoadAsset(const std::string& runtimeGlbPath,
                                 std::string& diagnostic)
{
    uniqueVertices_.clear();
    sockets_ = {};
    lastSkinnedTick_ = std::numeric_limits<std::uint64_t>::max();
    lastPreparedAnimation_ = {};
    hasLastPreparedAnimation_ = false;
    leftSocketErrorMetres_ = 0.0f;
    rightSocketErrorMetres_ = 0.0f;
    leftGripAgreement_ = {};
    rightGripAgreement_ = {};
    finalWorldFromLeftGrip_ = horde::gameplay::items::IdentityHeldItemTransform();
    leftBoneFromGripSocket_ = horde::gameplay::items::IdentityHeldItemTransform();
    rightBoneFromGripSocket_ = horde::gameplay::items::IdentityHeldItemTransform();
    leftRestHandOrientation_ = horde::gameplay::items::IdentityHeldItemTransform();
    rightRestHandOrientation_ = horde::gameplay::items::IdentityHeldItemTransform();
    leftRestGripBasisInPlayer_ = horde::gameplay::items::IdentityHeldItemTransform();
    rightRestGripBasisInPlayer_ = horde::gameplay::items::IdentityHeldItemTransform();
    stableGripBasesReady_ = false;
    if (!asset_.LoadClips(runtimeGlbPath, horde::scene::PlayerLocomotionClipSet(), diagnostic))
        return false;
    return DeriveStableRestGripBases(diagnostic);
}

bool PlayerRenderSlot::DeriveStableRestGripBases(std::string& diagnostic)
{
    using namespace horde::gameplay::animation;
    using namespace horde::gameplay::items;

    // The asset-owned bone-to-Grip basis is derived once from the approved
    // owner-feedback rest checkpoint composition. It never depends on the
    // first live/capture pose delivered to the renderer.
    HeldItemFixedStepInput restInput;
    restInput.playerX = 0.0f;
    restInput.playerZ = 1.85f;
    restInput.playerYawRadians = 0.0f;
    restInput.playerPitchRadians = -0.28f;
    HeldItemStates restItems = MakeDefaultHeldItemStates();
    HeldItemFixedStepState restHeldState;
    if (!ResolveHeldItemsFixedStep(restItems, restInput, 0u, restHeldState, diagnostic))
        return false;

    PlayerAnimationState restAnimationState;
    restAnimationState.StepFixed(
        {0.0f, 0.0f, restInput.playerCombat, restHeldState.kinematics, 1.0f}, 0.0f);
    PlayerAnimationSnapshot rigAnimation = restAnimationState.Snapshot();

    horde::scene::SkinnedNodeTransform leftArm{};
    horde::scene::SkinnedNodeTransform rightArm{};
    if (!asset_.NodeTransform(horde::scene::SkinnedClip::Idle, 0.0f,
                              "LeftArm", leftArm, diagnostic) ||
        !asset_.NodeTransform(horde::scene::SkinnedClip::Idle, 0.0f,
                              "RightArm", rightArm, diagnostic))
        return false;
    const Vec3 rigShoulderCenter{{
        (leftArm[12] + rightArm[12]) * 0.5f,
        (leftArm[13] + rightArm[13]) * 0.5f,
        (leftArm[14] + rightArm[14]) * 0.5f}};

    constexpr Vec3 bodyRight{{1.0f, 0.0f, 0.0f}};
    constexpr Vec3 bodyForward{{0.0f, 0.0f, -1.0f}};
    constexpr Vec3 worldUp{{0.0f, 1.0f, 0.0f}};
    const Vec3 viewForward = Normalise(
        {{0.0f, -0.05f + restInput.playerPitchRadians, -1.0f}});
    const Vec3 viewRight = Normalise(Cross(viewForward, worldUp));
    const Vec3 viewUp = Normalise(Cross(viewRight, viewForward));
    const Vec3 eye{{restInput.playerX, 0.58f, restInput.playerZ}};
    const auto toWorld = [&eye, &viewRight, &viewUp, &viewForward](const Vec3& local) {
        return Add(Add(Add(eye, Scale(viewRight, local[0])),
                       Scale(viewUp, local[1])),
                   Scale(viewForward, local[2]));
    };
    const Vec3 leftShoulder = toWorld(rigAnimation.leftIk.shoulder);
    const Vec3 rightShoulder = toWorld(rigAnimation.rightIk.shoulder);
    const Vec3 leftHand = toWorld(rigAnimation.leftIk.target);
    const Vec3 rightHand = toWorld(rigAnimation.rightIk.target);
    const Vec3 intendedShoulderCenter = Scale(Add(leftShoulder, rightShoulder), 0.5f);
    const Vec3 playerRootWorld = Subtract(
        intendedShoulderCenter,
        Add(Add(Scale(bodyRight, rigShoulderCenter[0]),
                {{0.0f, rigShoulderCenter[1], 0.0f}}),
            Scale(bodyForward, rigShoulderCenter[2])));
    const auto worldPointToPlayer = [&playerRootWorld, &bodyRight, &bodyForward](
                                        const Vec3& worldPoint) {
        const Vec3 delta = Subtract(worldPoint, playerRootWorld);
        return Vec3{{Dot(delta, bodyRight), delta[1], Dot(delta, bodyForward)}};
    };
    const auto viewVectorToPlayer = [&viewRight, &viewUp, &viewForward,
                                     &bodyRight, &bodyForward](const Vec3& viewVector) {
        const Vec3 worldVector = Add(Add(Scale(viewRight, viewVector[0]),
                                        Scale(viewUp, viewVector[1])),
                                     Scale(viewForward, viewVector[2]));
        return Vec3{{Dot(worldVector, bodyRight), worldVector[1],
                     Dot(worldVector, bodyForward)}};
    };
    rigAnimation.leftIk.shoulder = worldPointToPlayer(leftShoulder);
    rigAnimation.leftIk.target = worldPointToPlayer(leftHand);
    rigAnimation.leftIk.pole = viewVectorToPlayer(rigAnimation.leftIk.pole);
    rigAnimation.leftIk.gripX = viewVectorToPlayer(rigAnimation.leftIk.gripX);
    rigAnimation.leftIk.gripY = viewVectorToPlayer(rigAnimation.leftIk.gripY);
    rigAnimation.leftIk.gripZ = viewVectorToPlayer(rigAnimation.leftIk.gripZ);
    rigAnimation.rightIk.shoulder = worldPointToPlayer(rightShoulder);
    rigAnimation.rightIk.target = worldPointToPlayer(rightHand);
    rigAnimation.rightIk.pole = viewVectorToPlayer(rigAnimation.rightIk.pole);
    rigAnimation.rightIk.gripX = viewVectorToPlayer(rigAnimation.rightIk.gripX);
    rigAnimation.rightIk.gripY = viewVectorToPlayer(rigAnimation.rightIk.gripY);
    rigAnimation.rightIk.gripZ = viewVectorToPlayer(rigAnimation.rightIk.gripZ);

    const auto armTarget = [](const PlayerArmIkTarget& source) {
        horde::scene::SkinnedArmIkTarget result;
        result.target = source.target;
        result.pole = source.pole;
        result.shoulder = source.shoulder;
        result.shoulderTargetEnabled = true;
        return result;
    };
    std::vector<horde::scene::TexturedSkinnedRtVertex> restVertices;
    horde::scene::SkinnedPlayerSockets restSockets;
    if (!asset_.SkinPlayerUniqueTextured(
            horde::scene::SkinnedClip::Idle, 0.0f,
            armTarget(rigAnimation.leftIk), armTarget(rigAnimation.rightIk),
            restVertices, restSockets, diagnostic))
        return false;

    leftRestHandOrientation_ = RigidPlayerBoneInModel(restSockets.leftHand);
    rightRestHandOrientation_ = RigidPlayerBoneInModel(restSockets.rightHand);
    leftRestGripBasisInPlayer_ = TransformFromAxes(
        rigAnimation.leftIk.gripX, rigAnimation.leftIk.gripY,
        rigAnimation.leftIk.gripZ);
    rightRestGripBasisInPlayer_ = TransformFromAxes(
        rigAnimation.rightIk.gripX, rigAnimation.rightIk.gripY,
        rigAnimation.rightIk.gripZ);

    const HeldItemTransform worldFromLeftBone = RigidPlayerBoneToWorld(
        restSockets.leftHand, bodyRight, bodyForward, playerRootWorld);
    const HeldItemTransform worldFromRightBone = RigidPlayerBoneToWorld(
        restSockets.rightHand, bodyRight, bodyForward, playerRootWorld);
    const HeldItemTransform intendedWorldFromLeftGrip = MultiplyHeldItemTransforms(
        restItems[0].worldFromItem, OriginalTorchGripSocketTransform());
    const HeldItemTransform intendedWorldFromRightGrip = MultiplyHeldItemTransforms(
        restItems[1].worldFromItem, SwordGripSocketTransform());
    leftBoneFromGripSocket_ = MultiplyHeldItemTransforms(
        InverseRigid(worldFromLeftBone), intendedWorldFromLeftGrip);
    rightBoneFromGripSocket_ = MultiplyHeldItemTransforms(
        InverseRigid(worldFromRightBone), intendedWorldFromRightGrip);
    if (!ValidateHeldItemSocketTransform(leftBoneFromGripSocket_, diagnostic) ||
        !ValidateHeldItemSocketTransform(rightBoneFromGripSocket_, diagnostic))
        return false;
    const float leftPivotOffset = std::hypot(
        std::hypot(leftBoneFromGripSocket_[12], leftBoneFromGripSocket_[13]),
        leftBoneFromGripSocket_[14]);
    const float rightPivotOffset = std::hypot(
        std::hypot(rightBoneFromGripSocket_[12], rightBoneFromGripSocket_[13]),
        rightBoneFromGripSocket_[14]);
    if (leftPivotOffset > kPlayerGripSocketToleranceMetres ||
        rightPivotOffset > kPlayerGripSocketToleranceMetres)
    {
        diagnostic = "Deterministic rest bone-to-Grip basis exceeded the 15 mm pivot tolerance.";
        return false;
    }
    stableGripBasesReady_ = true;
    diagnostic.clear();
    return true;
}

bool PlayerRenderSlot::ResolveHeldItemVisuals(
    const horde::gameplay::items::HeldItemStates& authoritativeItems,
    const horde::gameplay::items::HeldItemTransform& worldFromLeftHandBone,
    const horde::gameplay::items::HeldItemTransform& worldFromRightHandBone,
    horde::gameplay::items::HeldItemStates& renderItems,
    std::string& diagnostic)
{
    using namespace horde::gameplay::items;
    if (!stableGripBasesReady_)
    {
        diagnostic = "Skinned player render slot has no validated rest bone-to-Grip basis.";
        return false;
    }
    if (!ValidateHeldItemSocketTransform(worldFromLeftHandBone, diagnostic) ||
        !ValidateHeldItemSocketTransform(worldFromRightHandBone, diagnostic))
        return false;
    const HeldItemTransform worldFromLeftGrip = MultiplyHeldItemTransforms(
        worldFromLeftHandBone, leftBoneFromGripSocket_);
    const HeldItemTransform worldFromRightGrip = MultiplyHeldItemTransforms(
        worldFromRightHandBone, rightBoneFromGripSocket_);
    finalWorldFromLeftGrip_ = worldFromLeftGrip;
    if (!ResolvePlayerHeldItemVisuals(
        authoritativeItems, worldFromLeftGrip, worldFromRightGrip,
        renderItems, diagnostic))
        return false;
    leftGripAgreement_ = MeasurePlayerGripAgreement(
        authoritativeItems[0], renderItems[0]);
    rightGripAgreement_ = MeasurePlayerGripAgreement(
        authoritativeItems[1], renderItems[1]);
    if (leftGripAgreement_.positionErrorMetres > kPlayerGripSocketToleranceMetres ||
        rightGripAgreement_.positionErrorMetres > kPlayerGripSocketToleranceMetres ||
        leftGripAgreement_.orientationErrorRadians > kPlayerGripOrientationToleranceRadians ||
        rightGripAgreement_.orientationErrorRadians > kPlayerGripOrientationToleranceRadians)
    {
        diagnostic = "Final composed player item Grip exceeded position/orientation tolerance: left=" +
                     std::to_string(leftGripAgreement_.positionErrorMetres) + "m/" +
                     std::to_string(leftGripAgreement_.orientationErrorRadians) + "rad right=" +
                     std::to_string(rightGripAgreement_.positionErrorMetres) + "m/" +
                     std::to_string(rightGripAgreement_.orientationErrorRadians) + "rad.";
        return false;
    }
    diagnostic.clear();
    return true;
}

bool PlayerRenderSlot::PreparePose(
    const horde::gameplay::animation::PlayerAnimationSnapshot& animation,
    const std::uint64_t tickIndex,
    const PlayerCpuSkinCadence cadence,
    bool& poseUpdated,
    std::string& diagnostic)
{
    poseUpdated = false;
    if (!asset_.IsLoaded())
    {
        diagnostic = "Skinned player render slot has no loaded asset.";
        return false;
    }
    const bool importedPoseAtSameTick = hasLastPreparedAnimation_ &&
        tickIndex == lastSkinnedTick_ && animation != lastPreparedAnimation_;
    if (!PlayerPoseNeedsRefresh(tickIndex, lastSkinnedTick_, cadence) &&
        !importedPoseAtSameTick)
    {
        diagnostic.clear();
        return true;
    }
    if (cadence == PlayerCpuSkinCadence::RequiresReviewedBackend)
    {
        diagnostic = "Measured player CPU skinning requires an explicitly reviewed backend.";
        return false;
    }

    const horde::scene::SkinnedClip clip =
        animation.locomotionClip == horde::gameplay::animation::PlayerLocomotionClip::Walk
        ? horde::scene::SkinnedClip::Walking
        : horde::scene::SkinnedClip::Idle;
    // The renderer adapter supplies gameplay-owned grip targets transformed
    // into the player's +Z-forward model space. The rig solves to those exact
    // points; imported clips never dictate item or damage timing.
    const auto target = [](const horde::gameplay::animation::PlayerArmIkTarget& source,
                           const HeldItemTransform& restGripBasis,
                           const HeldItemTransform& restHandOrientation) {
        horde::scene::SkinnedArmIkTarget result;
        result.target = source.target;
        result.pole = source.pole;
        result.shoulder = source.shoulder;
        result.shoulderTargetEnabled = true;
        const HeldItemTransform currentGripBasis = TransformFromAxes(
            source.gripX, source.gripY, source.gripZ);
        const HeldItemTransform gripDelta = horde::gameplay::items::MultiplyHeldItemTransforms(
            currentGripBasis, InverseRigid(restGripBasis));
        result.handOrientation = horde::gameplay::items::MultiplyHeldItemTransforms(
            gripDelta, restHandOrientation);
        result.handOrientationTargetEnabled = true;
        return result;
    };
    const horde::scene::SkinnedArmIkTarget left = target(
        animation.leftIk, leftRestGripBasisInPlayer_, leftRestHandOrientation_);
    const horde::scene::SkinnedArmIkTarget right = target(
        animation.rightIk, rightRestGripBasisInPlayer_, rightRestHandOrientation_);
    if (!horde::gameplay::items::ValidateHeldItemSocketTransform(
            left.handOrientation, diagnostic) ||
        !horde::gameplay::items::ValidateHeldItemSocketTransform(
            right.handOrientation, diagnostic))
    {
        diagnostic = "Gameplay-authored player Grip basis produced a non-rigid hand target.";
        return false;
    }
    if (!asset_.SkinPlayerUniqueTextured(clip, animation.locomotionTime,
                                          left, right, uniqueVertices_, sockets_,
                                          diagnostic))
        return false;
    const auto socketError = [](const horde::scene::SkinnedNodeTransform& socket,
                                const horde::scene::SkinnedArmIkTarget& intended) {
        return std::hypot(std::hypot(socket[12] - intended.target[0],
                                    socket[13] - intended.target[1]),
                          socket[14] - intended.target[2]);
    };
    leftSocketErrorMetres_ = socketError(sockets_.leftHand, left);
    rightSocketErrorMetres_ = socketError(sockets_.rightHand, right);
    if (leftSocketErrorMetres_ > kPlayerGripSocketToleranceMetres ||
        rightSocketErrorMetres_ > kPlayerGripSocketToleranceMetres)
    {
        diagnostic = "Skinned player hand bone socket exceeded the 15 mm grip tolerance: left=" +
                     std::to_string(leftSocketErrorMetres_) + " right=" +
                     std::to_string(rightSocketErrorMetres_) +
                     " leftTarget=" + std::to_string(left.target[0]) + "," +
                     std::to_string(left.target[1]) + "," + std::to_string(left.target[2]) +
                     " leftSocket=" + std::to_string(sockets_.leftHand[12]) + "," +
                     std::to_string(sockets_.leftHand[13]) + "," +
                     std::to_string(sockets_.leftHand[14]) + ".";
        return false;
    }
    lastSkinnedTick_ = tickIndex;
    lastPreparedAnimation_ = animation;
    hasLastPreparedAnimation_ = true;
    poseUpdated = true;
    diagnostic.clear();
    return true;
}

bool PlayerRenderSlot::ShoulderCenter(
    const horde::gameplay::animation::PlayerAnimationSnapshot& animation,
    std::array<float, 3u>& center,
    std::string& diagnostic) const
{
    const horde::scene::SkinnedClip clip =
        animation.locomotionClip == horde::gameplay::animation::PlayerLocomotionClip::Walk
        ? horde::scene::SkinnedClip::Walking
        : horde::scene::SkinnedClip::Idle;
    horde::scene::SkinnedNodeTransform left{};
    horde::scene::SkinnedNodeTransform right{};
    if (!asset_.NodeTransform(clip, animation.locomotionTime, "LeftArm", left,
                              diagnostic) ||
        !asset_.NodeTransform(clip, animation.locomotionTime, "RightArm", right,
                              diagnostic))
        return false;
    center = {{(left[12] + right[12]) * 0.5f,
               (left[13] + right[13]) * 0.5f,
               (left[14] + right[14]) * 0.5f}};
    diagnostic.clear();
    return true;
}

} // namespace horde::vulkan::raytracing
