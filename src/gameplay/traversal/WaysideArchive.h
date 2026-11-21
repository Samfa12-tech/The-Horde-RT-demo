#pragma once

#include "gameplay/traversal/HingedYawPose.h"
#include "../../../assets/models/world/source/bellwether-tavern-v1/staging.generated.h"

namespace horde::gameplay::bellwether {

enum class ArchiveStage : std::uint8_t { Closed, Inspected, Opening, Open };
enum class ArchiveAction : std::uint8_t { None, Inspect, Open, Listen, Recap, TakeRegister };

struct ArchivePose : traversal::HingedYawPose {
    ArchivePose():HingedYawPose{{
        1.f,0.f,0.f,kWaysideHinge[0], 0.f,1.f,0.f,kWaysideHinge[1], 0.f,0.f,1.f,kWaysideHinge[2]
    }} {}

    static ArchivePose AtOpening(float progress) {
        ArchivePose pose;
        pose.worldFromModel=At(kWaysideHinge,-progress*1.57079632679f).worldFromModel;
        return pose;
    }
};

struct WaysideArchive {
    ArchiveStage stage=ArchiveStage::Closed;
    float opening=0;
    ArchivePose pose{};
    bool lanternRevealStarted=false;
    bool registerRecovered=false;
    bool registerInterpreted=false;
};

} // namespace horde::gameplay::bellwether
