#pragma once

#include "gameplay/traversal/HingedYawPose.h"
#include "../../../assets/models/world/source/bellwether-tavern-v1/staging.generated.h"
#include "../../../assets/models/world/source/bellwether-tavern-threshold-v1/geometry.generated.h"

namespace horde::gameplay::bellwether {

enum class DoorAction { None, Open, Close };

struct TavernDoor {
    float opening=1.f;
    float destination=1.f;

    static constexpr float kPullX=1.82f+kSouthDoorAddedWidth;
    static constexpr float kSwingRadius=kSouthDoorSwingRadius+kSouthDoorAddedWidth;

    static traversal::HingedYawPose PoseAt(float opening) {
        return traversal::HingedYawPose::At(kSouthDoorHinge,opening*kSouthDoorOpenRadians);
    }
};

} // namespace horde::gameplay::bellwether
