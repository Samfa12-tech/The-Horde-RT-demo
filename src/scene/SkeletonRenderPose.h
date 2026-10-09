#pragma once

#include <algorithm>
#include <array>
#include <cmath>

#include "gameplay/CombatTimeline.h"
#include "gameplay/ShowcaseRoute.h"
#include "gameplay/simulation/SimulationSnapshot.h"
#include "scene/assets/SkinnedMeshAsset.h"

namespace horde::scene
{

// Platform-neutral pose values consumed by the character renderer and host
// diagnostics. The transform is row-major 3x4, matching Vulkan's instance
// transform layout without exposing Vulkan types to portable test targets.
struct SkeletonRenderPose
{
    SkeletonClip clip = SkeletonClip::Idle;
    float time = 0.0f;
    std::array<float, 12u> transform{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f}};
};

namespace detail
{

inline float SkeletonStaggerRecoil(const float actionTime)
{
    constexpr float impactDuration = 0.14f;
    constexpr float staggerDuration = 0.80f;
    const float elapsed = std::clamp(actionTime, 0.0f, staggerDuration);
    const auto smoothStep = [](const float value) {
        const float clamped = std::clamp(value, 0.0f, 1.0f);
        return clamped * clamped * (3.0f - 2.0f * clamped);
    };
    if (elapsed <= impactDuration)
    {
        return smoothStep(elapsed / impactDuration);
    }
    return 1.0f - smoothStep((elapsed - impactDuration) /
                             (staggerDuration - impactDuration));
}

} // namespace detail

inline SkeletonRenderPose EvaluateSkeletonRenderPose(
    const gameplay::simulation::SkeletonEnemySnapshot& skeleton,
    const float deadClipDuration)
{
    using Action = gameplay::EnemyCombatAction;
    using Animation = gameplay::EnemyAnimation;

    SkeletonRenderPose pose;
    if (skeleton.animation == Animation::Dead)
    {
        pose.clip = SkeletonClip::Dead;
    }
    else
    {
        switch (skeleton.action)
        {
        case Action::AttackWindup:
        case Action::AttackActive:
        case Action::AttackRecovery:
        case Action::Staggered:
            pose.clip = SkeletonClip::Attack;
            break;
        case Action::Dead:
            pose.clip = SkeletonClip::Dead;
            break;
        case Action::Locomotion:
        default:
            pose.clip = skeleton.animation == Animation::Walking
                ? SkeletonClip::Walking
                : SkeletonClip::Idle;
            break;
        }
    }

    // Preserve CharacterRenderSlot's original timing precedence: a dead
    // animation overrides the action clock only when a usable dead duration is
    // known; otherwise the action switch below still supplies its sample time.
    if (skeleton.animation == Animation::Dead && deadClipDuration > 0.0f)
    {
        pose.time = std::min(skeleton.animationTime, deadClipDuration);
    }
    else
    {
        switch (skeleton.action)
        {
        case Action::AttackWindup:
            pose.time = std::clamp(skeleton.actionTime, 0.0f,
                gameplay::CombatTimeline::kSkeletonAttackWindupSeconds);
            break;
        case Action::AttackActive:
            pose.time = gameplay::CombatTimeline::kSkeletonAttackWindupSeconds +
                std::clamp(skeleton.actionTime, 0.0f,
                    gameplay::CombatTimeline::kSkeletonAttackActiveSeconds);
            break;
        case Action::AttackRecovery:
            pose.time = gameplay::CombatTimeline::kSkeletonAttackWindupSeconds +
                gameplay::CombatTimeline::kSkeletonAttackActiveSeconds +
                std::clamp(skeleton.actionTime, 0.0f,
                    gameplay::CombatTimeline::kSkeletonAttackRecoverySeconds);
            break;
        case Action::Staggered:
            // The 1.20-second sample begins renderer-only stagger recovery;
            // normal attack contact uses the shared 1.12-second windup timeline.
            pose.time = gameplay::CombatTimeline::kSkeletonStaggerRecoverySampleSeconds +
                (gameplay::CombatTimeline::kSkeletonAttackRecoverySampleEndSeconds -
                 gameplay::CombatTimeline::kSkeletonStaggerRecoverySampleSeconds) *
                    std::clamp(skeleton.actionTime / 0.80f, 0.0f, 1.0f);
            break;
        case Action::Dead:
            pose.time = deadClipDuration > 0.0f
                ? std::min(skeleton.animationTime, deadClipDuration)
                : skeleton.animationTime;
            break;
        case Action::Locomotion:
        default:
            pose.time = skeleton.animationTime * 0.90f;
            break;
        }
    }
    if (skeleton.action == Action::Locomotion && skeleton.animation == Animation::Idle)
    {
        pose.time += skeleton.idlePhaseSeconds;
    }

    const float recoil = skeleton.action == Action::Staggered
        ? detail::SkeletonStaggerRecoil(skeleton.actionTime)
        : 0.0f;
    const float x = skeleton.x - std::sin(skeleton.facingRadians) * recoil * 0.20f;
    const float z = skeleton.z - std::cos(skeleton.facingRadians) * recoil * 0.20f;
    const float cosine = std::cos(skeleton.facingRadians);
    const float sine = std::sin(skeleton.facingRadians);
    const float lean = -0.30f * recoil;
    const float leanCosine = std::cos(lean);
    const float leanSine = std::sin(lean);
    pose.transform = {{
        cosine, sine * leanSine, sine * leanCosine, x,
        0.0f, leanCosine, -leanSine, gameplay::kRouteFloorWorldY + recoil * 0.055f,
        -sine, cosine * leanSine, cosine * leanCosine, z}};
    return pose;
}

} // namespace horde::scene
