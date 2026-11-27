#pragma once

#include "../../../assets/models/world/source/bellwether-square-waymarker-v1/geometry.generated.h"

#include <array>
#include <cmath>

namespace horde::gameplay::bellwether {

inline constexpr float kSquareWaymarkerReadRange=4.f;
inline constexpr float kSquareWaymarkerReadFacingCosine=.65f;
inline constexpr float kSquareWaymarkerReadableHeight=2.1f;

// The authored Windows route stands 3.62 m from the post to read the boards.
// A radial reach around the physical assembly also admits a natural approach
// from the well/tavern side without adding a waypoint or route station.
inline bool SquareWaymarkerReadableFrom(float playerX,float eyeY,float playerZ,float yawRadians) {
    if(!std::isfinite(playerX) || !std::isfinite(eyeY) || !std::isfinite(playerZ) ||
       !std::isfinite(yawRadians))return false;
    const float dx=kSquareWaymarkerPost[0]-playerX;
    const float dz=kSquareWaymarkerPost[2]-playerZ;
    const float distance=std::hypot(dx,dz);
    if(distance>.001f && distance<=kSquareWaymarkerReadRange &&
       std::sin(yawRadians)*dx-std::cos(yawRadians)*dz>=kSquareWaymarkerReadFacingCosine*distance) {
        const std::array<float,4> signHeights{{kSquareWaymarkerTavern[1],kSquareWaymarkerDealer[1],
            kSquareWaymarkerMill[1],kSquareWaymarkerSmith[1]}};
        for(const float signY:signHeights)
            if(std::abs(eyeY-signY)<=kSquareWaymarkerReadableHeight)return true;
    }
    return false;
}

// This is a local caption latch only: no dialogue identity, voice or save bit.
inline bool StepSquareWaymarkerTextVisible(bool readable,bool paused,bool dialogueActive,
    bool interactPressed,bool previouslyVisible) {
    if(!readable || paused || dialogueActive)return false;
    return interactPressed?!previouslyVisible:previouslyVisible;
}

} // namespace horde::gameplay::bellwether
