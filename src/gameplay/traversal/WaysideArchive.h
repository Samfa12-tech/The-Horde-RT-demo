#pragma once

#include "gameplay/simulation/TriangleSupportSurface.h"
#include "../../../assets/models/world/source/bellwether-tavern-v1/staging.generated.h"

namespace horde::gameplay::bellwether {

enum class ArchiveStage : std::uint8_t { Closed, Inspected, Opening, Open };
enum class ArchiveAction : std::uint8_t { None, Inspect, Open, Listen, Recap };

struct ArchivePose {
    std::array<float,12> worldFromModel{{
        1.f,0.f,0.f,kWaysideHinge[0], 0.f,1.f,0.f,kWaysideHinge[1], 0.f,0.f,1.f,kWaysideHinge[2]
    }};

    static ArchivePose AtOpening(float progress) {
        const float angle=-progress*1.57079632679f;
        const float c=std::cos(angle),s=std::sin(angle);
        return {{{c,0.f,s,kWaysideHinge[0], 0.f,1.f,0.f,kWaysideHinge[1], -s,0.f,c,kWaysideHinge[2]}}};
    }

    bool VolumeClear(const simulation::TriangleSupportSurface& contact,
                     float x0,float z0,float x1,float z1,float y0,float y1,float radius) const {
        const auto& m=worldFromModel;
        const float dx0=x0-m[3],dz0=z0-m[11],dx1=x1-m[3],dz1=z1-m[11];
        return contact.HorizontalVolumeClear(m[0]*dx0+m[8]*dz0,m[2]*dx0+m[10]*dz0,
            m[0]*dx1+m[8]*dz1,m[2]*dx1+m[10]*dz1,y0-m[7],y1-m[7],radius);
    }
};

struct WaysideArchive {
    ArchiveStage stage=ArchiveStage::Closed;
    float opening=0;
    ArchivePose pose{};
    bool lanternRevealStarted=false;
};

} // namespace horde::gameplay::bellwether
