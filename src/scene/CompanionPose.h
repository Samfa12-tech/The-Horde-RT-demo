#pragma once
#include "gameplay/dialogue/CompanionActing.h"
#include "gameplay/dialogue/CompanionLocomotion.h"
#include "scene/assets/SkinnedMeshAsset.h"
#include <array>
#include <cmath>

namespace horde::scene {
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
inline std::array<SkinnedNodeRotation,8> CompanionPoseRotations(
    const gameplay::dialogue::CompanionActingPose& acting,
    const gameplay::dialogue::CompanionLocomotionPose& motion) {
    const auto upper=CompanionUpperBodyRotations(acting);
    const auto flex=[](const char* name,float radians) {
        return SkinnedNodeRotation{name,{std::sin(radians*.5f),0,0,std::cos(radians*.5f)}};
    };
    // The admitted +Z-forward Warden rig uses local X for knee flexion.
    // Opposing hip/shin/ankle angles lift a boot without a walking sweep.
    const float left=.25f*motion.leftTurnStep,right=.25f*motion.rightTurnStep;
    return {{upper[0],upper[1],flex("LeftUpLeg",-left),flex("LeftLeg",2*left),flex("LeftFoot",-left),
             flex("RightUpLeg",-right),flex("RightLeg",2*right),flex("RightFoot",-right)}};
}
} // namespace horde::scene
