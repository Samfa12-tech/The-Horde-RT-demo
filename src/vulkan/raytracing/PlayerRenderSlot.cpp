#include "vulkan/raytracing/PlayerRenderSlot.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace horde::vulkan::raytracing
{

namespace
{

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
    leftSocketErrorMetres_ = 0.0f;
    rightSocketErrorMetres_ = 0.0f;
    leftBoneFromGripSocket_ = horde::gameplay::items::IdentityHeldItemTransform();
    rightBoneFromGripSocket_ = horde::gameplay::items::IdentityHeldItemTransform();
    leftSocketCalibrated_ = false;
    rightSocketCalibrated_ = false;
    return asset_.LoadClips(runtimeGlbPath, horde::scene::PlayerLocomotionClipSet(), diagnostic);
}

bool PlayerRenderSlot::ResolveHeldItemVisuals(
    const horde::gameplay::items::HeldItemStates& authoritativeItems,
    const horde::gameplay::items::HeldItemTransform& worldFromLeftHandBone,
    const horde::gameplay::items::HeldItemTransform& worldFromRightHandBone,
    horde::gameplay::items::HeldItemStates& renderItems,
    std::string& diagnostic)
{
    using namespace horde::gameplay::items;
    if (!ValidateHeldItemSocketTransform(worldFromLeftHandBone, diagnostic) ||
        !ValidateHeldItemSocketTransform(worldFromRightHandBone, diagnostic))
        return false;

    const auto calibrate = [&authoritativeItems](
                               const HeldHand hand,
                               const HeldItemTransform& worldFromBone,
                               HeldItemTransform& boneFromGripSocket,
                               bool& calibrated) {
        if (calibrated) return;
        const auto item = std::find_if(
            authoritativeItems.begin(), authoritativeItems.end(),
            [hand](const HeldItemState& candidate) {
                return candidate.hand == hand &&
                       candidate.parentMode == HeldItemParentMode::HandSocket;
            });
        if (item == authoritativeItems.end()) return;
        const HeldItemTransform itemFromGrip = item->id == HeldItemId::OriginalTorch
            ? OriginalTorchGripSocketTransform()
            : SwordGripSocketTransform();
        const HeldItemTransform intendedWorldFromGrip =
            MultiplyHeldItemTransforms(item->worldFromItem, itemFromGrip);
        boneFromGripSocket = MultiplyHeldItemTransforms(
            InverseRigid(worldFromBone), intendedWorldFromGrip);
        calibrated = true;
    };
    calibrate(HeldHand::LeftHand, worldFromLeftHandBone,
              leftBoneFromGripSocket_, leftSocketCalibrated_);
    calibrate(HeldHand::RightHand, worldFromRightHandBone,
              rightBoneFromGripSocket_, rightSocketCalibrated_);
    const HeldItemTransform worldFromLeftGrip = MultiplyHeldItemTransforms(
        worldFromLeftHandBone, leftBoneFromGripSocket_);
    const HeldItemTransform worldFromRightGrip = MultiplyHeldItemTransforms(
        worldFromRightHandBone, rightBoneFromGripSocket_);
    return ResolvePlayerHeldItemVisuals(
        authoritativeItems, worldFromLeftGrip, worldFromRightGrip,
        renderItems, diagnostic);
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
    if (!PlayerPoseNeedsRefresh(tickIndex, lastSkinnedTick_, cadence))
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
    const auto target = [](const horde::gameplay::animation::PlayerArmIkTarget& source) {
        horde::scene::SkinnedArmIkTarget result;
        result.target = source.target;
        result.pole = source.pole;
        return result;
    };
    const horde::scene::SkinnedArmIkTarget left = target(animation.leftIk);
    const horde::scene::SkinnedArmIkTarget right = target(animation.rightIk);
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
