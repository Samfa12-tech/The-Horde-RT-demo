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

} // namespace

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
    const float swingAmount = std::clamp(
        -swordSwingRadians / SwordCombat::kDownwardSwingAmplitude, 0.0f, 1.0f);
    const float smoothSwing = swingAmount * swingAmount * (3.0f - 2.0f * swingAmount);
    const std::array<float, 3u> swingHand{{
        0.25f + (-0.08f - 0.25f) * smoothSwing,
        -0.41f + (-0.47f + 0.41f) * smoothSwing,
        heldPropDepth + (std::min(heldPropDepth, 1.00f) - heldPropDepth) * smoothSwing}};
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
    pose.swordRadians = kSwordRestInwardRadians + swordSwingRadians +
                        parryBlend * (-0.82f + 0.14f * successJolt);
    pose.swordForwardRadians = kSwordRestForwardRadians;
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
    const float heldPropDepth = ComputeShowcaseHeldPropDepth(
        input.cameraX, input.cameraZ, forwardX, forwardZ);
    const LowerBodyPoseState lowerBodyPose = EvaluateLowerBodyPose(input.walkTime, input.walkAmount);
    const float movement = std::max(std::clamp(input.walkAmount, 0.0f, 1.0f), 0.2f);
    const float torchSway = std::sin(input.walkTime * 6.2f) * 0.035f * movement;
    const float torchBob = std::abs(std::sin(input.walkTime * 6.2f)) * 0.025f * movement;
    const std::array<float, 3u> heldLeftHand{{
        -0.24f - torchSway, -0.40f + torchBob, heldPropDepth}};
    const std::array<float, 3u> rewardLowLeftHand{{
        -0.30f - torchSway * 0.35f,
        -0.70f + torchBob * 0.25f,
        heldPropDepth}};
    constexpr std::array<float, 3u> loweredLeftHand{{-0.36f, -0.92f, 0.27f}};
    float lowerBlend = std::clamp(input.torchFailure.leftArmLowerBlend, 0.0f, 1.0f);
    if (input.interaction.heldLightKind ==
        horde::gameplay::interactions::HeldLightKind::RewardLantern)
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
        input.playerCombat, input.swordSwingRadians, heldPropDepth);

    HeldItemKinematicsState result;
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
        const auto& lowTarget = input.interaction.heldLightKind ==
            horde::gameplay::interactions::HeldLightKind::RewardLantern
            ? rewardLowLeftHand
            : loweredLeftHand;
        result.leftHandLocal[axis] = heldLeftHand[axis] +
            (lowTarget[axis] - heldLeftHand[axis]) * lowerBlend;
    }
    result.rightHandLocal = sword.rightHandLocal;
    result.heldPropDepth = heldPropDepth;
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
    const Vec3 eye{{input.playerX, 0.58f, input.playerZ}};
    const auto toWorld = [&](const std::array<float, 3u>& local) {
        return Add(Add(Add(eye, Scale(viewRight, local[0])),
                       Scale(viewUp, local[1])),
                   Scale(viewForward, local[2]));
    };

    const Vec3 leftHand = toWorld(state.kinematics.leftHandLocal);
    const Vec3 rightHand = toWorld(state.kinematics.rightHandLocal);
    const HeldItemTransform worldFromLeftHand = WorldFromAxes(
        viewRight, viewUp, Scale(viewForward, -1.0f), leftHand);
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
