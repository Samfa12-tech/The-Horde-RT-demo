#pragma once

#include "gameplay/animation/PlayerAnimationState.h"
#include "gameplay/traversal/RescueTraversal.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"

#include <array>
#include <cmath>

namespace horde::vulkan::raytracing
{

// Apply the world-space rope hand targets to the imported rig's model-space
// IK contract. Body heading follows the rope (yaw 0 faces -Z) and deliberately
// does not follow camera yaw/pitch. The final endpoints remain sourced from
// solved rope particles in the gameplay snapshot.
inline bool ApplyRescueRopeRigTargets(
    horde::gameplay::animation::PlayerAnimationSnapshot& animation,
    const horde::gameplay::traversal::RescueTraversalSnapshot& rescue,
    const PlayerModelWorldBasis& modelBasis,
    const std::array<float, 3u>& rootWorld,
    const std::array<float, 3u>& eyeWorld)
{
    const auto finite = [](const std::array<float, 3u>& v) {
        return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]);
    };
    if (!finite(rootWorld) || !finite(eyeWorld) ||
        !finite(modelBasis.modelXInWorld) || !finite(modelBasis.modelYInWorld) ||
        !finite(modelBasis.modelZInWorld) || !std::isfinite(rescue.bodyYawRadians) ||
        !std::isfinite(rescue.grippingHandTargets[0].x) ||
        !std::isfinite(rescue.grippingHandTargets[0].y) ||
        !std::isfinite(rescue.grippingHandTargets[0].z) ||
        !std::isfinite(rescue.grippingHandTargets[1].x) ||
        !std::isfinite(rescue.grippingHandTargets[1].y) ||
        !std::isfinite(rescue.grippingHandTargets[1].z))
        return false;

    const float yaw = rescue.bodyYawRadians;
    const std::array<float, 3u> bodyForward{{std::sin(yaw), 0.0f, -std::cos(yaw)}};
    const std::array<float, 3u> bodyRight{{std::cos(yaw), 0.0f, std::sin(yaw)}};
    const std::array<float, 3u> worldUp{{0.0f, 1.0f, 0.0f}};
    // Held-item and imported Grip bases are right-handed: Z = X cross Y.
    // With the body facing -Z this is -bodyForward, not bodyForward.
    const std::array<float, 3u> gripZ{{-bodyForward[0], -bodyForward[1], -bodyForward[2]}};
    const auto worldVectorToModel = [&modelBasis](const std::array<float, 3u>& v) {
        return WorldVectorToPlayerModel(modelBasis, v);
    };
    const auto worldPointToModel = [&rootWorld, &worldVectorToModel](
                                       const std::array<float, 3u>& p) {
        return worldVectorToModel({{p[0] - rootWorld[0], p[1] - rootWorld[1],
                                   p[2] - rootWorld[2]}});
    };

    auto candidate = animation;
    // Walking poses must not continue driving the legs while traversal owns
    // movement. This shared pose is also used by the actual imported-rig test.
    candidate.locomotionClip = horde::gameplay::animation::PlayerLocomotionClip::Idle;
    candidate.locomotionBlend = 0.0f;
    candidate.locomotionTime = 0.0f;
    candidate.swordStowBlend = 1.0f;
    candidate.swordHandGripBlend = 0.0f;
    const std::array<float, 2u> sides{{-1.0f, 1.0f}};
    std::array<horde::gameplay::animation::PlayerArmIkTarget*, 2u> arms{{
        &candidate.leftIk, &candidate.rightIk}};
    for (std::size_t hand = 0u; hand < arms.size(); ++hand)
    {
        auto& arm = *arms[hand];
        const auto shoulder = std::array<float, 3u>{{
            eyeWorld[0] + bodyRight[0] * sides[hand] * 0.19f,
            eyeWorld[1] - 0.27f,
            eyeWorld[2] + bodyRight[2] * sides[hand] * 0.19f}};
        arm.shoulder = worldPointToModel(shoulder);
        arm.pole = worldVectorToModel({{
            bodyRight[0] * sides[hand] + bodyForward[0],
            bodyRight[1] * sides[hand] + bodyForward[1],
            bodyRight[2] * sides[hand] + bodyForward[2]}});
        const std::array<float, 3u> handRight{{
            bodyRight[0] * sides[hand], 0.0f, bodyRight[2] * sides[hand]}};
        const std::array<float, 3u> handZ{{
            handRight[1] * worldUp[2] - handRight[2] * worldUp[1],
            handRight[2] * worldUp[0] - handRight[0] * worldUp[2],
            handRight[0] * worldUp[1] - handRight[1] * worldUp[0]}};
        if (rescue.ropeHandsActive)
        {
            const auto& target = rescue.grippingHandTargets[hand];
            arm.target = worldPointToModel({{target.x, target.y, target.z}});
            arm.gripX = worldVectorToModel(bodyRight);
            arm.gripZ = worldVectorToModel(gripZ);
        }
        else
        {
            // Supported handoff poses have no rope or item ownership. Use a
            // body-relative free carry target, independent of the camera's
            // view-space IK that the renderer converted before this helper.
            const std::array<float, 3u> freeTarget{{
                rootWorld[0] + bodyRight[0] * sides[hand] * 0.22f + bodyForward[0] * 0.15f,
                eyeWorld[1] - 0.65f,
                rootWorld[2] + bodyRight[2] * sides[hand] * 0.22f + bodyForward[2] * 0.15f}};
            arm.target = worldPointToModel(freeTarget);
            arm.gripX = worldVectorToModel(handRight);
            arm.gripZ = worldVectorToModel(handZ);
        }
        arm.gripY = worldVectorToModel(worldUp);
    }
    animation = candidate;
    return true;
}

} // namespace horde::vulkan::raytracing
