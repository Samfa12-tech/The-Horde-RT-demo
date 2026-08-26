#include "gameplay/items/HeldItemKinematics.h"

#include "gameplay/CorridorCollision.h"

#include <algorithm>
#include <cmath>

namespace horde::gameplay::items
{

namespace
{

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
    const float swingAmount = std::clamp(-swordSwingRadians / 1.12f, 0.0f, 1.0f);
    const float smoothSwing = swingAmount * swingAmount * (3.0f - 2.0f * swingAmount);
    const std::array<float, 3u> swingHand{{
        0.34f + (-0.08f - 0.34f) * smoothSwing,
        -0.41f + (-0.47f + 0.41f) * smoothSwing,
        heldPropDepth + (std::min(heldPropDepth, 1.00f) - heldPropDepth) * smoothSwing}};
    const std::array<float, 3u> parryHand{{
        -0.20f + 0.055f * successJolt,
        -0.29f + 0.025f * successJolt,
        std::min(heldPropDepth, 0.90f)}};

    HeldSwordPose pose;
    for (std::size_t axis = 0u; axis < pose.rightHandLocal.size(); ++axis)
    {
        pose.rightHandLocal[axis] = swingHand[axis] +
            (parryHand[axis] - swingHand[axis]) * parryBlend;
    }
    pose.swordRadians = swordSwingRadians + parryBlend * (-0.82f + 0.14f * successJolt);
    pose.parryBlend = parryBlend;
    pose.successJolt = successJolt;
    return pose;
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
        -0.34f - torchSway, -0.40f + torchBob, heldPropDepth}};
    constexpr std::array<float, 3u> loweredLeftHand{{-0.31f, -0.92f, 0.27f}};
    const float lowerBlend = std::clamp(input.torchFailure.leftArmLowerBlend, 0.0f, 1.0f);
    const HeldSwordPose sword = EvaluateHeldSwordPose(
        input.playerCombat, input.swordSwingRadians, heldPropDepth);

    HeldItemKinematicsState result;
    result.leftShoulderLocal = {{
        -0.25f,
        -0.44f + lowerBodyPose.pelvisBob * 0.35f,
        0.39f - lowerBodyPose.leftStride * 0.018f}};
    result.rightShoulderLocal = {{
        0.25f,
        -0.44f + lowerBodyPose.pelvisBob * 0.35f,
        0.39f + lowerBodyPose.leftStride * 0.018f}};
    for (std::size_t axis = 0u; axis < result.leftHandLocal.size(); ++axis)
    {
        result.leftHandLocal[axis] = heldLeftHand[axis] +
            (loweredLeftHand[axis] - heldLeftHand[axis]) * lowerBlend;
    }
    result.rightHandLocal = sword.rightHandLocal;
    result.heldPropDepth = heldPropDepth;
    result.swordRadians = sword.swordRadians;
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

} // namespace horde::gameplay::items
