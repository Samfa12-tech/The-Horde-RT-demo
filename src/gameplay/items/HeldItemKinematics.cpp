#include "gameplay/items/HeldItemKinematics.h"

#include "gameplay/CorridorCollision.h"

#include <algorithm>
#include <cmath>

namespace horde::gameplay::items
{

namespace
{

constexpr float kSwordRestInwardRadians = 0.20f;
constexpr float kSwordRestForwardRadians = 0.14f;
// The authored left palm is broadside at zero roll. A quarter-turn presents
// the grip end-on and collapses its four fingers into the reported fist blob.
constexpr float kLeftGripRollRadians = 0.0f;

float DotColumn(const HeldItemTransform& transform,
                const std::size_t leftColumn,
                const std::size_t rightColumn)
{
    return transform[leftColumn * 4u] * transform[rightColumn * 4u] +
           transform[leftColumn * 4u + 1u] * transform[rightColumn * 4u + 1u] +
           transform[leftColumn * 4u + 2u] * transform[rightColumn * 4u + 2u];
}

HeldItemTransform InverseRigidTransform(const HeldItemTransform& transform)
{
    HeldItemTransform result = IdentityHeldItemTransform();
    for (std::size_t column = 0u; column < 3u; ++column)
    {
        for (std::size_t row = 0u; row < 3u; ++row)
        {
            result[column * 4u + row] = transform[row * 4u + column];
        }
    }
    for (std::size_t row = 0u; row < 3u; ++row)
    {
        result[12u + row] = -(result[row] * transform[12] +
                              result[4u + row] * transform[13] +
                              result[8u + row] * transform[14]);
    }
    return result;
}

using Vec3 = std::array<float, 3u>;

Vec3 Add(const Vec3& left, const Vec3& right)
{
    return {{left[0] + right[0], left[1] + right[1], left[2] + right[2]}};
}

Vec3 Scale(const Vec3& value, const float scale)
{
    return {{value[0] * scale, value[1] * scale, value[2] * scale}};
}

Vec3 Lerp(const Vec3& from, const Vec3& to, const float amount)
{
    return Add(from, Scale(Add(to, Scale(from, -1.0f)), amount));
}

Vec3 Cross(const Vec3& left, const Vec3& right)
{
    return {{left[1] * right[2] - left[2] * right[1],
             left[2] * right[0] - left[0] * right[2],
             left[0] * right[1] - left[1] * right[0]}};
}

Vec3 Normalize(const Vec3& value)
{
    const float length = std::sqrt(value[0] * value[0] + value[1] * value[1] +
                                   value[2] * value[2]);
    return length > 0.000001f ? Scale(value, 1.0f / length)
                              : Vec3{{0.0f, 1.0f, 0.0f}};
}

HeldItemTransform WorldFromAxes(const Vec3& x,
                                const Vec3& y,
                                const Vec3& z,
                                const Vec3& translation)
{
    return {{x[0], x[1], x[2], 0.0f,
             y[0], y[1], y[2], 0.0f,
             z[0], z[1], z[2], 0.0f,
             translation[0], translation[1], translation[2], 1.0f}};
}

Vec3 TranslationOf(const HeldItemTransform& transform)
{
    return {{transform[12], transform[13], transform[14]}};
}

Vec3 ColumnOf(const HeldItemTransform& transform, const std::size_t column)
{
    return {{transform[column * 4u], transform[column * 4u + 1u],
             transform[column * 4u + 2u]}};
}

float MapContinuousCarryDepth(const float forwardClearance,
                              const float openDepth)
{
    constexpr float minimumClearance = 0.30f;
    constexpr float maximumClearance = 2.70f;
    const float normalized = std::clamp(
        (forwardClearance - minimumClearance) /
            (maximumClearance - minimumClearance),
        0.0f, 1.0f);
    return minimumClearance + (openDepth - minimumClearance) * normalized;
}

} // namespace

float ComputeRewardLanternForwardClearance(const float cameraX,
                                           const float cameraZ,
                                           const float forwardX,
                                           const float forwardZ)
{
    if (!std::isfinite(cameraX) || !std::isfinite(cameraZ) ||
        !std::isfinite(forwardX) || !std::isfinite(forwardZ))
        return 0.0f;
    const float forwardLength = std::hypot(forwardX, forwardZ);
    if (forwardLength <= 0.000001f ||
        !IsShowcasePlayerPositionWalkable(cameraX, cameraZ))
        return 0.0f;
    const float unitForwardX = forwardX / forwardLength;
    const float unitForwardZ = forwardZ / forwardLength;
    constexpr float kMaximumClearance = 2.70f;
    constexpr float kSearchStride = 0.025f;
    float lastWalkable = 0.0f;
    for (int step = 1; step <=
         static_cast<int>(kMaximumClearance / kSearchStride); ++step)
    {
        const float distance = static_cast<float>(step) * kSearchStride;
        if (IsShowcasePlayerPositionWalkable(
                cameraX + unitForwardX * distance,
                cameraZ + unitForwardZ * distance))
        {
            lastWalkable = distance;
            continue;
        }

        // Resolve the first collision surface continuously. The predicate is
        // already the player-centre route contract and therefore already owns
        // the 24 cm player radius; offsetting it laterally by that radius would
        // double-count the body and create an early, grid-stepped retraction.
        float blocked = distance;
        for (int refinement = 0; refinement < 14; ++refinement)
        {
            const float midpoint = 0.5f * (lastWalkable + blocked);
            if (IsShowcasePlayerPositionWalkable(
                    cameraX + unitForwardX * midpoint,
                    cameraZ + unitForwardZ * midpoint))
                lastWalkable = midpoint;
            else
                blocked = midpoint;
        }
        return lastWalkable;
    }
    return kMaximumClearance;
}

HeldSwordPose EvaluateHeldSwordPose(const PlayerCombatSnapshot& playerCombat,
                                   const float swordSwingRadians,
                                   const float heldPropDepth)
{
    float parryBlend = 0.0f;
    switch (playerCombat.action)
    {
    case PlayerCombatAction::ParryStartup:
        parryBlend = std::clamp(playerCombat.actionTime / 0.04f, 0.0f, 1.0f);
        break;
    case PlayerCombatAction::ParryActive:
        parryBlend = 1.0f;
        break;
    case PlayerCombatAction::ParryRecovery:
        parryBlend = 1.0f - std::clamp(playerCombat.actionTime / 0.24f, 0.0f, 1.0f);
        break;
    default:
        break;
    }
    parryBlend = parryBlend * parryBlend * (3.0f - 2.0f * parryBlend);
    const float successJolt = playerCombat.reaction == CombatReaction::Parried
        ? std::clamp(playerCombat.reactionTime / 0.12f, 0.0f, 1.0f)
        : 0.0f;
    const auto smooth = [](const float value) {
        const float clamped = std::clamp(value, 0.0f, 1.0f);
        return clamped * clamped * (3.0f - 2.0f * clamped);
    };
    const auto blendHand = [](const Vec3& from, const Vec3& to,
                              const float amount) {
        return Lerp(from, to, amount);
    };
    const float swordGripDepth = heldPropDepth - 0.05f;
    const Vec3 restHand{{0.12f, -0.44f, swordGripDepth}};
    const Vec3 downwardWindupHand{{0.08f, -0.17f,
                                   std::max(0.56f, swordGripDepth - 0.01f)}};
    const Vec3 downwardImpactHand{{0.02f, -0.70f,
                                   std::max(0.48f, swordGripDepth - 0.18f)}};
    const Vec3 upwardStartHand{{0.01f, -0.72f,
                                std::max(0.48f, swordGripDepth - 0.16f)}};
    const Vec3 upwardEndHand{{0.06f, -0.15f,
                              std::max(0.52f, swordGripDepth - 0.08f)}};

    Vec3 swingHand = restHand;
    float swingInwardRadians = kSwordRestInwardRadians;
    float swingForwardRadians = kSwordRestForwardRadians;
    switch (playerCombat.action)
    {
    case PlayerCombatAction::SwingWindup:
    {
        const float amount = smooth(
            playerCombat.actionTime / SwordCombat::kSwingWindupDuration);
        swingHand = blendHand(restHand, downwardWindupHand, amount);
        swingInwardRadians = kSwordRestInwardRadians +
            (-0.05f - kSwordRestInwardRadians) * amount;
        swingForwardRadians = kSwordRestForwardRadians +
            (0.55f - kSwordRestForwardRadians) * amount;
        break;
    }
    case PlayerCombatAction::SwingActive:
    {
        const float amount = smooth(
            playerCombat.actionTime /
            SwordCombat::kDownwardCutTravelDuration);
        swingHand = blendHand(downwardWindupHand, downwardImpactHand, amount);
        // The blade tilts strongly into depth while the hand travels down.
        // This reads as a real camera-facing cut without sweeping the full
        // metre-long blade beyond a narrow phone's horizontal safe frame.
        swingInwardRadians = -0.05f + (0.78f + 0.05f) * amount;
        swingForwardRadians = 0.55f + (1.00f - 0.55f) * amount;
        break;
    }
    case PlayerCombatAction::SwingRecovery:
    {
        const float amount = smooth(
            playerCombat.actionTime / SwordCombat::kSwingRecoveryDuration);
        swingHand = blendHand(downwardImpactHand, restHand, amount);
        swingInwardRadians = 0.78f +
            (kSwordRestInwardRadians - 0.78f) * amount;
        swingForwardRadians = 1.00f +
            (kSwordRestForwardRadians - 1.00f) * amount;
        break;
    }
    case PlayerCombatAction::UpwardSliceWindup:
    {
        const float amount = smooth(
            playerCombat.actionTime / SwordCombat::kUpwardSliceWindupDuration);
        swingHand = blendHand(downwardImpactHand, upwardStartHand, amount);
        swingInwardRadians = 0.78f + 0.04f * amount;
        swingForwardRadians = 1.00f - 0.02f * amount;
        break;
    }
    case PlayerCombatAction::UpwardSliceActive:
    {
        const float amount = smooth(
            playerCombat.actionTime / SwordCombat::kUpwardSliceActiveDuration);
        swingHand = blendHand(upwardStartHand, upwardEndHand, amount);
        swingInwardRadians = 0.82f + (-0.30f - 0.82f) * amount;
        swingForwardRadians = 0.98f + (0.60f - 0.98f) * amount;
        break;
    }
    case PlayerCombatAction::UpwardSliceRecovery:
    {
        const float amount = smooth(
            playerCombat.actionTime / SwordCombat::kUpwardSliceRecoveryDuration);
        swingHand = blendHand(upwardEndHand, restHand, amount);
        swingInwardRadians = -0.30f +
            (kSwordRestInwardRadians + 0.30f) * amount;
        swingForwardRadians = 0.60f +
            (kSwordRestForwardRadians - 0.60f) * amount;
        break;
    }
    default:
        // Imported legacy captures may carry only the scalar swing. Keep a
        // bounded fallback without allowing that old sideways-only value to
        // override the explicit cut phases above.
        if (std::abs(swordSwingRadians) > 0.0001f)
        {
            const float amount = smooth(std::clamp(
                -swordSwingRadians / SwordCombat::kDownwardSwingAmplitude,
                0.0f, 1.0f));
            swingHand = blendHand(restHand, downwardImpactHand, amount);
            swingInwardRadians = kSwordRestInwardRadians +
                (0.78f - kSwordRestInwardRadians) * amount;
            swingForwardRadians = kSwordRestForwardRadians +
                (1.00f - kSwordRestForwardRadians) * amount;
        }
        break;
    }
    const std::array<float, 3u> parryHand{{
        -0.16f + 0.055f * successJolt,
        -0.29f + 0.025f * successJolt,
        std::min(heldPropDepth, 0.90f)}};

    HeldSwordPose pose;
    for (std::size_t axis = 0u; axis < pose.rightHandLocal.size(); ++axis)
    {
        pose.rightHandLocal[axis] = swingHand[axis] +
            (parryHand[axis] - swingHand[axis]) * parryBlend;
    }
    pose.swordRadians = swingInwardRadians +
                        parryBlend * (-0.62f + 0.14f * successJolt -
                                      swingInwardRadians);
    pose.swordForwardRadians = swingForwardRadians +
        parryBlend * (kSwordRestForwardRadians - swingForwardRadians);
    pose.parryBlend = parryBlend;
    pose.successJolt = successJolt;
    return pose;
}

std::array<float, 3u> EvaluateSwordBladeAxisInView(
    const float inwardRadians,
    const float forwardRadians)
{
    const float forwardCos = std::cos(forwardRadians);
    return {{-std::sin(inwardRadians) * forwardCos,
             std::cos(inwardRadians) * forwardCos,
             std::sin(forwardRadians)}};
}

SwordGripBasisInView EvaluateSwordGripBasisInView(
    const float inwardRadians,
    const float forwardRadians,
    const float gripRollRadians)
{
    const float swordCos = std::cos(inwardRadians);
    const float swordSin = std::sin(inwardRadians);
    const Vec3 unrolledEdge{{swordCos, swordSin, 0.0f}};
    const Vec3 bladeAxis = EvaluateSwordBladeAxisInView(inwardRadians, forwardRadians);
    // View right/up/forward is a left-handed coordinate frame because world
    // forward points down -Z. Negate the algebraic cross product so the
    // authored +X/+Y/+Z Grip basis remains right-handed once mapped to world.
    const Vec3 unrolledFlat = Scale(Normalize(Cross(unrolledEdge, bladeAxis)), -1.0f);
    const float rollCos = std::cos(gripRollRadians);
    const float rollSin = std::sin(gripRollRadians);
    SwordGripBasisInView result;
    result.edgeDirection = Normalize(Add(Scale(unrolledEdge, rollCos),
                                         Scale(unrolledFlat, -rollSin)));
    result.bladeAxis = bladeAxis;
    result.flatNormal = Normalize(Add(Scale(unrolledFlat, rollCos),
                                      Scale(unrolledEdge, rollSin)));
    return result;
}

FirstPersonSafeFrame EvaluateOwnerFeedbackPortraitSafeFrame(
    const HeldItemKinematicsState& kinematics,
    const float portraitAspect)
{
    FirstPersonSafeFrame result;
    result.minimumNdcX = 1.0e9f;
    result.maximumNdcX = -1.0e9f;
    const float aspect = std::max(portraitAspect, 0.1f);
    const auto include = [&](const Vec3& point)
    {
        const float depth = std::max(point[2], 0.05f);
        const float ndcX = 1.22f * point[0] / (depth * aspect);
        result.minimumNdcX = std::min(result.minimumNdcX, ndcX);
        result.maximumNdcX = std::max(result.maximumNdcX, ndcX);
    };

    constexpr float torchRadius = 0.068f;
    include({{kinematics.leftHandLocal[0] - torchRadius,
              kinematics.leftHandLocal[1], kinematics.leftHandLocal[2]}});
    include({{kinematics.leftHandLocal[0] + torchRadius,
              kinematics.leftHandLocal[1], kinematics.leftHandLocal[2]}});
    result.includesTorchGrip = true;
    include({{kinematics.leftHandLocal[0], kinematics.leftHandLocal[1] + 0.525f,
              kinematics.leftHandLocal[2]}});
    result.includesFlame = true;
    include({{kinematics.leftHandLocal[0], kinematics.leftHandLocal[1] + 0.495f,
              kinematics.leftHandLocal[2] - 0.025f}});
    result.includesLight = true;

    const SwordGripBasisInView basis = EvaluateSwordGripBasisInView(
        kinematics.swordRadians, kinematics.swordForwardRadians,
        kSwordGripRollRadians);
    const Vec3 grip = kinematics.rightHandLocal;
    include(grip);
    result.includesSwordGrip = true;
    // Runtime sword GLB audited bounds relative to its Grip: blade-long +Y
    // [-0.135, 0.915], edge +X +/-0.112, flat +Z +/-0.025 metres.
    for (const float edge : {-0.112f, 0.112f})
    {
        for (const float blade : {-0.135f, 0.915f})
        {
            for (const float flat : {-0.025f, 0.025f})
            {
                include(Add(grip, Add(Scale(basis.edgeDirection, edge),
                                      Add(Scale(basis.bladeAxis, blade),
                                          Scale(basis.flatNormal, flat)))));
            }
        }
    }
    result.includesBladeBounds = true;
    return result;
}

HeldItemKinematicsState EvaluateHeldItemKinematics(const HeldItemKinematicsInput& input)
{
    const float forwardX = std::sin(input.cameraYawRadians);
    const float forwardZ = -std::cos(input.cameraYawRadians);
    const float forwardClearance = ComputeRewardLanternForwardClearance(
        input.cameraX, input.cameraZ, forwardX, forwardZ);
    // All rigid hand props share the continuous collision clearance. The
    // legacy 7.5 cm sampled helper produced a visible one-tick sword/forearm
    // jump while approaching a wall and could feed a discontinuous pose into
    // the skinned BLAS refit. Start the response early enough that a normal
    // fixed walking tick changes hand depth by only a bounded amount.
    const float carriedPropDepth = MapContinuousCarryDepth(
        forwardClearance, 0.68f);
    const float swordPropDepth = MapContinuousCarryDepth(
        forwardClearance, 0.82f);
    const LowerBodyPoseState lowerBodyPose = EvaluateLowerBodyPose(input.walkTime, input.walkAmount);
    const float movement = std::max(std::clamp(input.walkAmount, 0.0f, 1.0f), 0.2f);
    const float torchSway = std::sin(input.walkTime * 6.2f) * 0.035f * movement;
    const float torchBob = std::abs(std::sin(input.walkTime * 6.2f)) * 0.025f * movement;
    const std::array<float, 3u> heldLeftHand{{
        -0.12f - torchSway, -0.41f + torchBob, carriedPropDepth}};
    const bool rewardLantern = input.interaction.heldLightKind ==
        horde::gameplay::interactions::HeldLightKind::RewardLantern;
    // The authored lantern is almost one metre tall. In open space it needs
    // more perspective distance than the compact torch so its full swing cone
    // remains readable on narrow portrait phones. Near route collision, the
    // same sampled clearance smoothly removes that advance and adds the body
    // radius inset needed to keep the complete authored cage camera-side of
    // the wall. This remains one shared carry contract on every platform.
    const float rewardForwardClearance = rewardLantern
        ? forwardClearance : 0.0f;
    // Retract across the complete measured 2.4 m clearance interval instead
    // of compressing the carry-to-wall response into the final metre. This
    // preserves the exact open/emergency endpoints while keeping a normal
    // fixed walking tick below the skinned-arm silhouette continuity bound.
    const float continuousHeldDepth = MapContinuousCarryDepth(
        rewardForwardClearance, 1.05f);
    const float rewardClearance = std::clamp(
        (continuousHeldDepth - 0.30f) / (1.05f - 0.30f), 0.0f, 1.0f);
    const float rewardClearanceBlend = rewardClearance * rewardClearance *
        (3.0f - 2.0f * rewardClearance);
    // Leave enough camera-side travel for the full authored body at the real
    // wall fixture even when downward look pitches the skinned grip forward.
    const float rewardWallInset =
        0.265f * (1.0f - rewardClearanceBlend) + 0.010f;
    // Keep the top ring inside the actual left-arm reach envelope. The
    // previous 1.20 m camera-relative extension required translating the
    // clavicle and, through the old root calculation, the whole player.
    const float rewardOpenAdvance = 0.0f;
    const float preferredRewardDepth =
        continuousHeldDepth - rewardWallInset + rewardOpenAdvance;
    // At the real wall fixture the player centre has only 30 cm of physical
    // camera-to-wall travel. Keep the ring 20 cm forward: this is outside the
    // near-camera exclusion while leaving the compact, sideways cage on the
    // camera side of the wall. The pendulum presentation retains raw motion
    // continuity while the clearance blend reaches this emergency pose.
    const float rewardDepth = std::max(
        0.20f,
        std::min(preferredRewardDepth,
                 std::max(0.20f, rewardForwardClearance - 0.30f)));
    // At the tight wall fixture there is not enough forward clearance for the
    // long cage to hang toward the wall. Centre the ring in the shallow view
    // cone; the authoritative collision presentation turns the cage sideways
    // below it. Open-space carry keeps the established shoulder-side target.
    // Keep the claimed prop on the anatomical left side. Centralising the
    // open-space ring at -5 cm made the correctly paired left hand cross the
    // torso even though the arm-chain mapping itself was fixed.
    const float rewardLateral =
        -0.45f + 0.32f * rewardClearanceBlend * rewardClearanceBlend;
    const float rewardHighVerticalWallOffset =
        -0.18f * (1.0f - rewardClearanceBlend);
    const float rewardLowVerticalWallOffset =
        -0.04f * (1.0f - rewardClearanceBlend);
    const std::array<float, 3u> rewardHighLeftHand{{
        rewardLateral - torchSway * 0.35f,
        -0.05f + rewardHighVerticalWallOffset + torchBob * 0.25f,
        rewardDepth}};
    const std::array<float, 3u> rewardLowLeftHand{{
        rewardLateral - torchSway * 0.35f,
        -0.24f + rewardLowVerticalWallOffset + torchBob * 0.25f,
        rewardDepth}};
    constexpr std::array<float, 3u> loweredLeftHand{{-0.36f, -0.92f, 0.27f}};
    float lowerBlend = std::clamp(input.torchFailure.leftArmLowerBlend, 0.0f, 1.0f);
    if (rewardLantern)
    {
        using horde::gameplay::interactions::HeldLightPose;
        switch (input.interaction.heldLightPose)
        {
        case HeldLightPose::Low:
            lowerBlend = 1.0f;
            break;
        case HeldLightPose::TransitioningToLow:
            lowerBlend = std::clamp(input.interaction.heldLightPoseProgress, 0.0f, 1.0f);
            break;
        case HeldLightPose::TransitioningToHigh:
            lowerBlend = 1.0f - std::clamp(
                input.interaction.heldLightPoseProgress, 0.0f, 1.0f);
            break;
        default:
            lowerBlend = 0.0f;
            break;
        }
    }
    const HeldSwordPose sword = EvaluateHeldSwordPose(
        input.playerCombat, input.swordSwingRadians, swordPropDepth);

    HeldItemKinematicsState result;
    // Props move the hand effector only. Keep the calibrated clavicle/shoulder
    // on the torso for torch, phone, sword, and reward carries alike; moving
    // the whole physical arm subtree to the raised reward hand was the source
    // of the near-camera horizontal polygons reported in owner playtesting.
    result.leftShoulderLocal = {{
        -0.36f,
        -0.44f + lowerBodyPose.pelvisBob * 0.35f,
        0.39f - lowerBodyPose.leftStride * 0.018f}};
    result.rightShoulderLocal = {{
        0.36f,
        -0.44f + lowerBodyPose.pelvisBob * 0.35f,
        0.39f + lowerBodyPose.leftStride * 0.018f}};
    for (std::size_t axis = 0u; axis < result.leftHandLocal.size(); ++axis)
    {
        const auto& highTarget = rewardLantern ? rewardHighLeftHand : heldLeftHand;
        const auto& lowTarget = rewardLantern ? rewardLowLeftHand : loweredLeftHand;
        result.leftHandLocal[axis] = highTarget[axis] +
            (lowTarget[axis] - highTarget[axis]) * lowerBlend;
    }
    result.rightHandLocal = sword.rightHandLocal;
    const float leftGripRollCos = std::cos(kLeftGripRollRadians);
    const float leftGripRollSin = std::sin(kLeftGripRollRadians);
    result.leftGripXInView = {{leftGripRollCos, 0.0f, leftGripRollSin}};
    result.leftGripYInView = {{0.0f, 1.0f, 0.0f}};
    result.leftGripZInView = {{leftGripRollSin, 0.0f, -leftGripRollCos}};
    result.heldPropDepth = carriedPropDepth;
    result.rewardLanternPresentationYawRadians = rewardLantern
        ? 1.57079632679f * (1.0f - rewardClearanceBlend)
        : 0.0f;
    result.swordRadians = sword.swordRadians;
    result.swordForwardRadians = sword.swordForwardRadians;
    result.parryBlend = sword.parryBlend;
    result.successJolt = sword.successJolt;
    return result;
}

const horde::scene::assets::StaticSocket* FindHeldItemSocket(
    const std::span<const horde::scene::assets::StaticSocket> sockets,
    const std::string_view name)
{
    for (const auto& socket : sockets)
    {
        if (socket.name == name) return &socket;
    }
    return nullptr;
}

bool ValidateHeldItemSocketTransform(const HeldItemTransform& transform,
                                     std::string& diagnostic)
{
    constexpr float tolerance = 0.001f;
    const bool affine = std::abs(transform[3]) <= tolerance &&
                        std::abs(transform[7]) <= tolerance &&
                        std::abs(transform[11]) <= tolerance &&
                        std::abs(transform[15] - 1.0f) <= tolerance;
    const bool unit = std::abs(DotColumn(transform, 0u, 0u) - 1.0f) <= tolerance &&
                      std::abs(DotColumn(transform, 1u, 1u) - 1.0f) <= tolerance &&
                      std::abs(DotColumn(transform, 2u, 2u) - 1.0f) <= tolerance;
    const bool orthogonal = std::abs(DotColumn(transform, 0u, 1u)) <= tolerance &&
                            std::abs(DotColumn(transform, 0u, 2u)) <= tolerance &&
                            std::abs(DotColumn(transform, 1u, 2u)) <= tolerance;
    const float determinant =
        transform[0] * (transform[5] * transform[10] - transform[9] * transform[6]) -
        transform[4] * (transform[1] * transform[10] - transform[9] * transform[2]) +
        transform[8] * (transform[1] * transform[6] - transform[5] * transform[2]);
    if (!affine || !unit || !orthogonal || std::abs(determinant - 1.0f) > tolerance)
    {
        diagnostic = "Held-item socket transform must be rigid and unit scale.";
        return false;
    }
    diagnostic.clear();
    return true;
}

HeldItemTransform MultiplyHeldItemTransforms(const HeldItemTransform& left,
                                             const HeldItemTransform& right)
{
    HeldItemTransform result{};
    for (std::size_t column = 0u; column < 4u; ++column)
    {
        for (std::size_t row = 0u; row < 4u; ++row)
        {
            for (std::size_t inner = 0u; inner < 4u; ++inner)
            {
                result[column * 4u + row] +=
                    left[inner * 4u + row] * right[column * 4u + inner];
            }
        }
    }
    return result;
}

bool ComposeWorldFromItem(const HeldItemTransform& worldFromHandSocket,
                          const HeldItemTransform& itemFromGrip,
                          HeldItemTransform& worldFromItem,
                          std::string& diagnostic)
{
    if (!ValidateHeldItemSocketTransform(worldFromHandSocket, diagnostic) ||
        !ValidateHeldItemSocketTransform(itemFromGrip, diagnostic))
    {
        return false;
    }
    worldFromItem = MultiplyHeldItemTransforms(
        worldFromHandSocket, InverseRigidTransform(itemFromGrip));
    diagnostic.clear();
    return true;
}

HeldItemTransform SelectHandSocketTransform(const HeldHand hand,
                                            const HeldItemTransform& worldFromLeftHand,
                                            const HeldItemTransform& worldFromRightHand)
{
    return hand == HeldHand::LeftHand ? worldFromLeftHand : worldFromRightHand;
}

bool ResolveHeldItemsFixedStep(HeldItemStates& items,
                               const HeldItemFixedStepInput& input,
                               const std::uint64_t tick,
                               HeldItemFixedStepState& state,
                               std::string& diagnostic)
{
    state.kinematics = EvaluateHeldItemKinematics({
        input.playerX,
        input.playerZ,
        input.playerYawRadians,
        input.walkTime,
        input.walkAmount,
        input.torchFailure,
        input.playerCombat,
        input.swordSwingRadians,
        input.interaction});

    constexpr Vec3 worldUp{{0.0f, 1.0f, 0.0f}};
    const float pitch = std::clamp(input.playerPitchRadians, -0.32f, 0.28f);
    const Vec3 viewForward = Normalize({{
        std::sin(input.playerYawRadians), -0.05f + pitch,
        -std::cos(input.playerYawRadians)}});
    const Vec3 viewRight = Normalize(Cross(viewForward, worldUp));
    const Vec3 viewUp = Normalize(Cross(viewRight, viewForward));
    const Vec3 eye{{input.playerX, horde::gameplay::kShowcaseEyeWorldY,
                    input.playerZ}};
    const auto toWorld = [&](const std::array<float, 3u>& local) {
        return Add(Add(Add(eye, Scale(viewRight, local[0])),
                       Scale(viewUp, local[1])),
                   Scale(viewForward, local[2]));
    };

    const Vec3 leftHand = toWorld(state.kinematics.leftHandLocal);
    const Vec3 rightHand = toWorld(state.kinematics.rightHandLocal);
    const auto gripAxisToWorld = [&](const Vec3& axisInView) {
        return Normalize(Add(Add(Scale(viewRight, axisInView[0]),
                                 Scale(viewUp, axisInView[1])),
                             Scale(viewForward, axisInView[2])));
    };
    const HeldItemTransform worldFromLeftHand = WorldFromAxes(
        gripAxisToWorld(state.kinematics.leftGripXInView),
        gripAxisToWorld(state.kinematics.leftGripYInView),
        gripAxisToWorld(state.kinematics.leftGripZInView), leftHand);
    state.worldFromLeftHand = worldFromLeftHand;
    HeldItemTransform heldWorldFromTorch{};
    if (!ComposeWorldFromItem(worldFromLeftHand,
                              OriginalTorchGripSocketTransform(),
                              heldWorldFromTorch,
                              diagnostic))
    {
        return false;
    }

    if (input.torchFailure.heldByPlayer)
    {
        UpdateHeldItemParent(items[0], HeldItemParentMode::HandSocket,
                             tick, heldWorldFromTorch);
    }
    else if (items[0].parentMode == HeldItemParentMode::HandSocket)
    {
        // The transition tick deliberately publishes the previous resolved
        // hand attachment as both the detach basis and current trajectory
        // matrix. This makes the sampled boundary exactly continuous even
        // when gait sway/bob was non-zero on the previous fixed tick.
        UpdateHeldItemParent(items[0], HeldItemParentMode::AuthoredWorldTrajectory,
                             tick, items[0].worldFromItem);
    }
    else
    {
        const float progress = std::clamp(input.torchFailure.fallProgress, 0.0f, 1.0f);
        const float releaseYawCos = std::cos(input.torchFailure.droppedYawRadians);
        const float releaseYawSin = std::sin(input.torchFailure.droppedYawRadians);
        const Vec3 releaseBodyForward{{releaseYawSin, 0.0f, -releaseYawCos}};
        const Vec3 releaseBodyRight{{releaseYawCos, 0.0f, releaseYawSin}};
        const float pitchCos = std::cos(input.torchFailure.droppedPitchRadians);
        const float pitchSin = std::sin(input.torchFailure.droppedPitchRadians);
        const Vec3 restingX = releaseBodyRight;
        const Vec3 restingY = Normalize(Add(Scale(worldUp, pitchCos),
                                            Scale(releaseBodyForward, pitchSin)));
        const Vec3 restingZ = Normalize(Add(Scale(releaseBodyForward, pitchCos),
                                            Scale(worldUp, -pitchSin)));
        const Vec3 fallingX = Normalize(Lerp(ColumnOf(items[0].worldFromDetach, 0u),
                                             restingX, progress));
        const Vec3 fallingY = Normalize(Lerp(ColumnOf(items[0].worldFromDetach, 1u),
                                             restingY, progress));
        const Vec3 fallingZ = Normalize(Lerp(ColumnOf(items[0].worldFromDetach, 2u),
                                             Scale(restingZ, -1.0f), progress));
        Vec3 settledPosition = Add(
            Add(Vec3{{input.torchFailure.droppedX,
                      input.torchFailure.droppedY,
                      input.torchFailure.droppedZ}},
                Scale(worldUp, 0.13f)),
            Add(Scale(releaseBodyRight, -0.34f),
                Scale(releaseBodyForward, 0.78f)));
        settledPosition[0] = std::clamp(settledPosition[0], -2.28f, 4.58f);
        settledPosition[2] = std::clamp(settledPosition[2], -16.18f, -14.22f);
        const HeldItemTransform worldFromTorch = WorldFromAxes(
            fallingX, fallingY, fallingZ,
            Lerp(TranslationOf(items[0].worldFromDetach), settledPosition, progress));
        UpdateHeldItemParent(items[0], HeldItemParentMode::AuthoredWorldTrajectory,
                             tick, worldFromTorch);
    }

    const SwordGripBasisInView swordBasis = EvaluateSwordGripBasisInView(
        state.kinematics.swordRadians, state.kinematics.swordForwardRadians,
        kSwordGripRollRadians);
    const Vec3 swordEdge = Normalize(Add(
        Add(Scale(viewRight, swordBasis.edgeDirection[0]),
            Scale(viewUp, swordBasis.edgeDirection[1])),
        Scale(viewForward, swordBasis.edgeDirection[2])));
    const auto& swordBladeInView = swordBasis.bladeAxis;
    const Vec3 swordAxisY = Normalize(Add(
        Add(Scale(viewRight, swordBladeInView[0]), Scale(viewUp, swordBladeInView[1])),
        Scale(viewForward, swordBladeInView[2])));
    const Vec3 swordFlat = Normalize(Add(
        Add(Scale(viewRight, swordBasis.flatNormal[0]),
            Scale(viewUp, swordBasis.flatNormal[1])),
        Scale(viewForward, swordBasis.flatNormal[2])));
    HeldItemTransform worldFromSword{};
    if (!ComposeWorldFromItem(
            WorldFromAxes(swordEdge, swordAxisY, swordFlat, rightHand),
            SwordGripSocketTransform(), worldFromSword, diagnostic))
    {
        return false;
    }
    UpdateHeldItemParent(items[1], HeldItemParentMode::HandSocket,
                         tick, worldFromSword);

    return ComposeHeldLightState(items[0].worldFromItem,
                                 OriginalTorchFlameSocketTransform(),
                                 OriginalTorchLightSocketTransform(),
                                 input.torchFailure.flameStrength,
                                 state.light,
                                 diagnostic);
}

} // namespace horde::gameplay::items
