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
        const auto from=ModelXZ(x0,z0);
        const auto to=ModelXZ(x1,z1);
        return contact.HorizontalVolumeClear(from[0],from[1],to[0],to[1],
            y0-worldFromModel[7],y1-worldFromModel[7],radius);
    }

    // The poses share a hinge and differ by at most a half turn. In model
    // space the stationary body traces the inverse arc. The chord plus its
    // maximum deviation encloses that sweep without a tip-speed margin.
    bool RotatingVolumeClear(const simulation::TriangleSupportSurface& contact,const HingedYawPose& end,
                            float angleDelta,float x,float z,float y0,float y1,float radius) const {
        const auto from=ModelXZ(x,z);
        const auto to=end.ModelXZ(x,z);
        const float quarterSine=std::sin(angleDelta*.25f);
        const float arcDeviation=2.f*std::hypot(from[0],from[1])*quarterSine*quarterSine;
        return contact.HorizontalVolumeClear(from[0],from[1],to[0],to[1],
            y0-worldFromModel[7],y1-worldFromModel[7],radius+arcDeviation);
    }

private:
    std::array<float,2> ModelXZ(float x,float z) const {
        const auto& m=worldFromModel;
        const float dx=x-m[3],dz=z-m[11];
        return {m[0]*dx+m[8]*dz,m[2]*dx+m[10]*dz};
    }
};

} // namespace horde::gameplay::traversal
