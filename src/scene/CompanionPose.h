#pragma once
#include "gameplay/dialogue/CompanionActing.h"
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
} // namespace horde::scene
