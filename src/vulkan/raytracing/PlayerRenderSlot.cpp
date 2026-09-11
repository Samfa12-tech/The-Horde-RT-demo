#include "vulkan/raytracing/PlayerRenderSlot.h"
#include "vulkan/raytracing/RtSceneRecordObservation.h"

#include "gameplay/ShowcaseRoute.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace horde::vulkan::raytracing
{

std::array<float, 3u> EvaluatePlayerTorsoAnchorLocal(
    const horde::gameplay::animation::PlayerAnimationSnapshot& animation)
{
    // Both ordinary shoulders are authored around z=0.39. Keep the torso ten
    // millimetres forward of that nominal socket plane so closed-glove arm
    // poses retain a measured primary-camera clearance margin while the hands
    // continue to resolve to their exact gameplay-owned Grip targets.
    return {{0.0f, animation.rightIk.shoulder[1], 0.40f}};
}

std::array<float, 3u> GroundPlayerRootOnRouteFloor(
    const std::array<float, 3u>& shoulderAnchoredRootWorld,
    const float routeFloorWorldY,
    const float assetGroundingOffsetMetres)
{
    return {{shoulderAnchoredRootWorld[0],
             routeFloorWorldY + assetGroundingOffsetMetres,
             shoulderAnchoredRootWorld[2]}};
}

PlayerModelWorldBasis BuildPlayerModelWorldBasis(
    const std::array<float, 3u>& gameplayRightInWorld,
    const std::array<float, 3u>& gameplayForwardInWorld)
{
    PlayerModelWorldBasis result;
    result.modelXInWorld = {{-gameplayRightInWorld[0],
                             -gameplayRightInWorld[1],
                             -gameplayRightInWorld[2]}};
    result.modelZInWorld = gameplayForwardInWorld;
    return result;
}

std::array<float, 3u> PlayerModelVectorToWorld(
    const PlayerModelWorldBasis& basis,
    const std::array<float, 3u>& modelVector)
{
    return {{basis.modelXInWorld[0] * modelVector[0] +
                 basis.modelYInWorld[0] * modelVector[1] +
                 basis.modelZInWorld[0] * modelVector[2],
             basis.modelXInWorld[1] * modelVector[0] +
                 basis.modelYInWorld[1] * modelVector[1] +
                 basis.modelZInWorld[1] * modelVector[2],
             basis.modelXInWorld[2] * modelVector[0] +
                 basis.modelYInWorld[2] * modelVector[1] +
                 basis.modelZInWorld[2] * modelVector[2]}};
}

std::array<float, 3u> WorldVectorToPlayerModel(
    const PlayerModelWorldBasis& basis,
    const std::array<float, 3u>& worldVector)
{
    const auto dot = [&worldVector](const std::array<float, 3u>& axis) {
        return worldVector[0] * axis[0] + worldVector[1] * axis[1] +
               worldVector[2] * axis[2];
    };
    return {{dot(basis.modelXInWorld), dot(basis.modelYInWorld),
             dot(basis.modelZInWorld)}};
}

float PlayerModelWorldBasisDeterminant(const PlayerModelWorldBasis& basis)
{
    const auto& x = basis.modelXInWorld;
    const auto& y = basis.modelYInWorld;
    const auto& z = basis.modelZInWorld;
    return x[0] * (y[1] * z[2] - y[2] * z[1]) -
           y[0] * (x[1] * z[2] - x[2] * z[1]) +
           z[0] * (x[1] * y[2] - x[2] * y[1]);
}

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
    if (route == PlayerRenderRoute::HybridBlockPrimary)
    {
        // Slot 4 retains the complete skinned character for reflection/shadow
        // rays. Reward props own TLAS slots 5-8 and the dielectric fixture owns
        // metadata index 9, so the bounded fallback arms use the otherwise
        // available procedural slots/custom indices 10-13.
        result.instanceMasks[4] = 0x10u;
        for (std::size_t slot = 10u; slot <= 13u; ++slot)
            result.instanceMasks[slot] = 0x04u;
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
    // body), which are the legacy procedural arm slots. The owner-deferred
    // skinned first-person hands therefore use the bounded hybrid route:
    // skinned body in reflections/shadows, stable block arms in free slots
    // 10-13. This is intentionally one route switch, not a second animation or
    // socket authority.
    result.playerRoute = input.requestedPlayerRoute == PlayerRenderRoute::Skinned
        ? PlayerRenderRoute::Skinned
        : ((result.rewardWorldVisible || input.glassFixtureVisible)
            ? PlayerRenderRoute::HybridBlockPrimary
            : input.requestedPlayerRoute);
    const PlayerRouteMasks playerMasks = BuildPlayerRouteMasks(result.playerRoute);
    // The claimed reward is the active left-hand light and replaces the
    // ordinary torch. Keep the normal-route torch before the claim, but never
    // render both rigid props through the same final skinned grip.
    result.torchMask = (input.productionInspection || input.rewardLanternClaimed)
        ? 0u : 0x02u;
    result.swordMask = input.productionInspection ? 0u : 0x02u;
    result.playerMask = input.productionInspection
        ? 0u : playerMasks.instanceMasks[4];
    result.playerPrimaryVisible = !input.productionInspection &&
        std::any_of(playerMasks.instanceMasks.begin(),
                    playerMasks.instanceMasks.end(),
                    [](const std::uint8_t mask) { return (mask & 0x04u) != 0u; });
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
    leftHandFromGripSocket_ = horde::gameplay::items::IdentityHeldItemTransform();
    rightHandFromGripSocket_ = horde::gameplay::items::IdentityHeldItemTransform();
    stableGripBasesReady_ = false;
    bootGroundingProfilesReady_ = false;
    if (!asset_.LoadClips(runtimeGlbPath, horde::scene::PlayerLocomotionClipSet(), diagnostic))
        return false;
    return DeriveAssetGripSockets(diagnostic) &&
           BuildBootGroundingProfiles(diagnostic);
}

bool PlayerRenderSlot::BuildBootGroundingProfiles(std::string& diagnostic)
{
    float idle = 0.0f;
    float walking = 0.0f;
    if (!asset_.BootGroundingMinimumY(
            horde::scene::SkinnedClip::Idle, 0.0f, idle, diagnostic) ||
        !asset_.BootGroundingMinimumY(
            horde::scene::SkinnedClip::Walking, 0.0f, walking, diagnostic))
        return false;
    bootGroundingProfilesReady_ = true;
    diagnostic.clear();
    return true;
}

float PlayerRenderSlot::BootGroundingOffsetMetres(
    const horde::gameplay::animation::PlayerAnimationSnapshot& animation) const
{
    if (!bootGroundingProfilesReady_)
        return kPlayerBootGroundingSafetyMetres;
    const horde::scene::SkinnedClip clip = animation.locomotionClip ==
            horde::gameplay::animation::PlayerLocomotionClip::Walk
        ? horde::scene::SkinnedClip::Walking
        : horde::scene::SkinnedClip::Idle;
    float minimumY = 0.0f;
    std::string ignoredDiagnostic;
    if (!asset_.BootGroundingMinimumY(
            clip, animation.locomotionTime, minimumY, ignoredDiagnostic))
        return kPlayerBootGroundingSafetyMetres;
    return -minimumY + kPlayerBootGroundingSafetyMetres;
}

bool PlayerRenderSlot::DeriveAssetGripSockets(std::string& diagnostic)
{
    using namespace horde::gameplay::items;
    horde::scene::SkinnedNodeTransform namedLeftHand{};
    horde::scene::SkinnedNodeTransform namedLeftGrip{};
    horde::scene::SkinnedNodeTransform namedRightHand{};
    horde::scene::SkinnedNodeTransform namedRightGrip{};
    if (!asset_.NodeTransform(horde::scene::SkinnedClip::Idle, 0.0f,
                              "LeftHand", namedLeftHand, diagnostic) ||
        !asset_.NodeTransform(horde::scene::SkinnedClip::Idle, 0.0f,
                              "LeftGrip", namedLeftGrip, diagnostic) ||
        !asset_.NodeTransform(horde::scene::SkinnedClip::Idle, 0.0f,
                              "RightHand", namedRightHand, diagnostic) ||
        !asset_.NodeTransform(horde::scene::SkinnedClip::Idle, 0.0f,
                              "RightGrip", namedRightGrip, diagnostic))
        return false;

    // Preserve anatomical identity end to end. The renderer's proper
    // model-to-world rotation maps model +X (anatomical Left) to gameplay
    // left, so no opposite-hand socket substitution is valid here.
    leftHandFromGripSocket_ = MultiplyHeldItemTransforms(
        InverseRigid(RigidPlayerBoneInModel(namedLeftHand)),
        RigidPlayerBoneInModel(namedLeftGrip));
    rightHandFromGripSocket_ = MultiplyHeldItemTransforms(
        InverseRigid(RigidPlayerBoneInModel(namedRightHand)),
        RigidPlayerBoneInModel(namedRightGrip));
    if (!ValidateHeldItemSocketTransform(leftHandFromGripSocket_, diagnostic) ||
        !ValidateHeldItemSocketTransform(rightHandFromGripSocket_, diagnostic))
        return false;
    const float leftPivotOffset = std::hypot(
        std::hypot(leftHandFromGripSocket_[12], leftHandFromGripSocket_[13]),
        leftHandFromGripSocket_[14]);
    const float rightPivotOffset = std::hypot(
        std::hypot(rightHandFromGripSocket_[12], rightHandFromGripSocket_[13]),
        rightHandFromGripSocket_[14]);
    if (leftPivotOffset < 0.035f || leftPivotOffset > 0.090f ||
        rightPivotOffset < 0.035f || rightPivotOffset > 0.090f)
    {
        diagnostic = "Asset-owned palm Grip must be 35-90 mm from its wrist joint: left=" +
                     std::to_string(leftPivotOffset) + " right=" +
                     std::to_string(rightPivotOffset) + ".";
        return false;
    }
    stableGripBasesReady_ = true;
    diagnostic.clear();
    return true;
}

bool PlayerRenderSlot::ResolveHeldItemVisuals(
    const horde::gameplay::items::HeldItemStates& authoritativeItems,
    const horde::gameplay::items::HeldItemTransform& worldFromLeftGrip,
    const horde::gameplay::items::HeldItemTransform& worldFromRightGrip,
    horde::gameplay::items::HeldItemStates& renderItems,
    std::string& diagnostic)
{
    using namespace horde::gameplay::items;
    if (!stableGripBasesReady_)
    {
        diagnostic = "Skinned player render slot has no validated asset-owned palm Grip sockets.";
        return false;
    }
    if (!ValidateHeldItemSocketTransform(worldFromLeftGrip, diagnostic) ||
        !ValidateHeldItemSocketTransform(worldFromRightGrip, diagnostic))
        return false;
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
    std::string& diagnostic,
    RtSceneRecordObservation* observation)
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
                           const HeldItemTransform& handFromGripSocket) {
        horde::scene::SkinnedArmIkTarget result;
        result.pole = source.pole;
        result.shoulder = source.shoulder;
        result.shoulderTargetEnabled = false;
        // PresentableTinyRtScene converts the gameplay view frame through the
        // proper 180-degree player-model rotation. The resulting Grip frame is
        // already right-handed and must not be reflected a second time.
        const HeldItemTransform desiredGrip = TransformFromAxes(
            source.gripX, source.gripY, source.gripZ, source.target);
        const HeldItemTransform desiredHand =
            horde::gameplay::items::MultiplyHeldItemTransforms(
                desiredGrip, InverseRigid(handFromGripSocket));
        result.target = {{desiredHand[12], desiredHand[13], desiredHand[14]}};
        result.handOrientation = desiredHand;
        result.handOrientationTargetEnabled = true;
        return result;
    };
    const horde::scene::SkinnedArmIkTarget left = target(
        animation.leftIk, leftHandFromGripSocket_);
    const horde::scene::SkinnedArmIkTarget right = target(
        animation.rightIk, rightHandFromGripSocket_);
    if (!horde::gameplay::items::ValidateHeldItemSocketTransform(
            left.handOrientation, diagnostic) ||
        !horde::gameplay::items::ValidateHeldItemSocketTransform(
            right.handOrientation, diagnostic))
    {
        const auto determinant = [](const HeldItemTransform& transform) {
            return transform[0] *
                       (transform[5] * transform[10] -
                        transform[9] * transform[6]) -
                   transform[4] *
                       (transform[1] * transform[10] -
                        transform[9] * transform[2]) +
                   transform[8] *
                       (transform[1] * transform[6] -
                        transform[5] * transform[2]);
        };
        diagnostic = "Gameplay-authored player Grip basis produced a non-rigid hand target: " +
                     diagnostic + " determinants=" +
                     std::to_string(determinant(left.handOrientation)) + "/" +
                     std::to_string(determinant(right.handOrientation)) + ".";
        return false;
    }
    RtSceneStageScope skinScope(observation, horde::telemetry::RtStage::PlayerSkin);
    if (!asset_.SkinPlayerUniqueTextured(
            clip, animation.locomotionTime, left, right, uniqueVertices_,
            uniqueTangents_, sockets_, diagnostic))
    {
        skinScope.Cancel();
        return false;
    }
    skinScope.Complete(1u);
    const auto socketError = [](const horde::scene::SkinnedNodeTransform& socket,
                                const horde::gameplay::animation::PlayerArmIkTarget& intended) {
        return std::hypot(std::hypot(socket[12] - intended.target[0],
                                    socket[13] - intended.target[1]),
                          socket[14] - intended.target[2]);
    };
    leftSocketErrorMetres_ = socketError(sockets_.leftGrip, animation.leftIk);
    rightSocketErrorMetres_ = socketError(sockets_.rightGrip, animation.rightIk);
    if (leftSocketErrorMetres_ > kPlayerGripSocketToleranceMetres ||
        rightSocketErrorMetres_ > kPlayerGripSocketToleranceMetres)
    {
        diagnostic = "Skinned player palm Grip socket exceeded the 15 mm grip tolerance: left=" +
                     std::to_string(leftSocketErrorMetres_) + " right=" +
                     std::to_string(rightSocketErrorMetres_) +
                     " leftTarget=" + std::to_string(animation.leftIk.target[0]) + "," +
                     std::to_string(animation.leftIk.target[1]) + "," +
                     std::to_string(animation.leftIk.target[2]) +
                     " leftSocket=" + std::to_string(sockets_.leftGrip[12]) + "," +
                     std::to_string(sockets_.leftGrip[13]) + "," +
                     std::to_string(sockets_.leftGrip[14]) +
                     " leftHandTarget=" + std::to_string(left.target[0]) + "," +
                     std::to_string(left.target[1]) + "," +
                     std::to_string(left.target[2]) +
                     " leftHandSocket=" + std::to_string(sockets_.leftHand[12]) + "," +
                     std::to_string(sockets_.leftHand[13]) + "," +
                     std::to_string(sockets_.leftHand[14]) +
                     " rightHandTarget=" + std::to_string(right.target[0]) + "," +
                     std::to_string(right.target[1]) + "," +
                     std::to_string(right.target[2]) +
                     " rightHandSocket=" + std::to_string(sockets_.rightHand[12]) + "," +
                     std::to_string(sockets_.rightHand[13]) + "," +
                     std::to_string(sockets_.rightHand[14]) + ".";
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
