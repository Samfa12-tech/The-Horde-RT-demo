#pragma once

#include "gameplay/simulation/TriangleSupportSurface.h"
#include <array>
#include <cmath>

namespace horde::gameplay::traversal {

struct HingedYawPose {
    std::array<float,12> worldFromModel;

    static HingedYawPose At(const std::array<float,3>& hinge,float angle) {
        const float c=std::cos(angle),s=std::sin(angle);
        return {{{c,0.f,s,hinge[0], 0.f,1.f,0.f,hinge[1], -s,0.f,c,hinge[2]}}};
    }

    bool VolumeClear(const simulation::TriangleSupportSurface& contact,
                     float x0,float z0,float x1,float z1,float y0,float y1,float radius) const {
        const auto& m=worldFromModel;
        const float dx0=x0-m[3],dz0=z0-m[11],dx1=x1-m[3],dz1=z1-m[11];
        return contact.HorizontalVolumeClear(m[0]*dx0+m[8]*dz0,m[2]*dx0+m[10]*dz0,
            m[0]*dx1+m[8]*dz1,m[2]*dx1+m[10]*dz1,y0-m[7],y1-m[7],radius);
    }
};

} // namespace horde::gameplay::traversal
