#pragma once
#include "gameplay/dialogue/CompanionActing.h"
#include "gameplay/dialogue/CompanionRouteFixture.h"
#include "scene/assets/SkinnedMeshAsset.h"
#include <array>
#include <cmath>

namespace horde::scene {
struct CompanionAnimationRecipe {
    SkinnedClip clip=SkinnedClip::Idle;
    float seconds=0;
    SkinnedClipBlend blend{SkinnedClip::Walking,0,0};
};
// One immutable two-clip recipe, shared by serial and worker preparation. The
// simulation has already applied the matched external yaw; no root yaw here.
inline CompanionAnimationRecipe CompanionAnimation(
    const gameplay::dialogue::CompanionSnapshot& snapshot,float walkDuration) {
    using namespace gameplay::dialogue;
    CompanionAnimationRecipe recipe;
    recipe.seconds=snapshot.animationSeconds;
    recipe.blend={SkinnedClip::Walking,snapshot.locomotion.walkPhase*walkDuration,snapshot.locomotion.walkWeight};
    const auto& m=snapshot.motion;
    if(!m.turnStartEnabled)return recipe;
    if(m.mode==CompanionMotionMode::Turning) {
        if(m.entryWeight<1)recipe.blend={SkinnedClip::Idle,0,m.entryWeight};
        else {
            recipe.seconds=0;
            recipe.blend={m.turnAngleRadians>0?SkinnedClip::TurnRight:SkinnedClip::TurnLeft,
                m.turnPhase*companion_turn::duration,std::abs(m.turnAngleRadians)/companion_turn::maxAngle};
        }
    } else if(m.mode==CompanionMotionMode::Starting)recipe.seconds=0;
    else if(m.mode==CompanionMotionMode::Idle && m.idleZeroWeight>0) {
        if(snapshot.locomotion.walkWeight>0)recipe.seconds=0;
        else recipe.blend={SkinnedClip::Idle,0,m.idleZeroWeight};
    }
    return recipe;
}
inline std::array<SkinnedNodeRotation,2> CompanionUpperBodyRotations(
    const gameplay::dialogue::CompanionActingPose& pose) {
    const float x=pose.headPitch*.5f,y=pose.headYaw*.5f;
    return {{{"Head",{std::sin(x)*std::cos(y),std::cos(x)*std::sin(y),
                      std::sin(x)*std::sin(y),std::cos(x)*std::cos(y)}},
             {"Spine",{std::sin(pose.chestPitch*.5f),0,0,std::cos(pose.chestPitch*.5f)}}}};
}
// Imported +Z forward must follow logical (sin(yaw),0,-cos(yaw)). This is a
// proper rotation, preserving metre scale, handedness and the simulation root.
inline float CompanionModelYaw(float logicalYaw) {return 3.14159265359f-logicalYaw;}
} // namespace horde::scene
