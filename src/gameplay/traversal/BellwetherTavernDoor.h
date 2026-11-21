#pragma once

#include "gameplay/traversal/HingedYawPose.h"
#include "../../../assets/models/world/source/bellwether-tavern-v1/staging.generated.h"

namespace horde::gameplay::bellwether {

enum class DoorAction { None, Open, Close };

struct TavernDoor {
    float opening=1.f;
    float destination=1.f;

    static traversal::HingedYawPose PoseAt(float opening) {
        return traversal::HingedYawPose::At(kSouthDoorHinge,opening*kSouthDoorOpenRadians);
    }
};

} // namespace horde::gameplay::bellwether
