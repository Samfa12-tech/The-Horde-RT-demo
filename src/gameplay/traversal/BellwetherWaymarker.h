#pragma once

#include "../../../assets/models/world/source/bellwether-square-waymarker-v1/geometry.generated.h"
#include "../../../assets/models/world/source/bellwether-house-b-ledger-v1/geometry.generated.h"

#include <array>
#include <cmath>

namespace horde::gameplay::bellwether {

enum class LocalReading { None, SquareFingerpost, HouseBStockLedger };

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

inline bool HouseBStockLedgerReadableFrom(float playerX, float eyeY, float playerZ, float yawRadians) {
    const auto [x, y, z] = kHouseBLedgerPosition;
    const float dx = x - playerX, dz = z - playerZ;
    const float distance = std::hypot(dx, dz);
    return distance > .001f && distance <= 1.55f && std::abs(eyeY-y) <= 1.50f &&
        std::sin(yawRadians)*dx - std::cos(yawRadians)*dz >= .65f * distance;
}

inline constexpr LocalReading StepLocalReading(LocalReading target, LocalReading open, bool interactPressed) {
    if (interactPressed) return target == open ? LocalReading::None : target;
    return target == open ? open : LocalReading::None;
}

} // namespace horde::gameplay::bellwether
