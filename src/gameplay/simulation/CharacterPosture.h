#pragma once
#include "scene/ShowcaseOverheadGeometry.h"
#include "scene/DevelopmentWorldGeometry.h"
#include <cmath>

namespace horde::gameplay::simulation {
inline constexpr float kStandingBodyHeight=1.8f,kCrouchingBodyHeight=1.2f;
inline constexpr float kMaximumCrouchDrop=kStandingBodyHeight-kCrouchingBodyHeight;
struct CharacterPosture {
    float drop=0;
    bool standingBlocked=false;
    float Height() const {return kStandingBodyHeight-drop;}
    bool Active() const {return drop>.0001f;}
    template<class Clearance> void Step(bool requested,float seconds,Clearance&& clearStanding) {
        if(!std::isfinite(seconds)||seconds<=0)return;
        standingBlocked=!requested && Active() && !clearStanding(kStandingBodyHeight);
        const float target=requested?kMaximumCrouchDrop:standingBlocked?drop:0.f;
        drop+=std::clamp(target-drop,-2.4f*std::min(seconds,.05f),2.4f*std::min(seconds,.05f));
    }
};
// Shared authored roof volumes, including sloped undersides. Swept footprint
// bounds are conservative for irregular roofs; ground/side collision is separate.
inline bool AuthoredBodySweepClear(float x,float z,float toX,float toZ,float footY,float height,float radius) {
    for(float v:{x,z,toX,toZ,footY,height,radius})if(!std::isfinite(v))return false;
    if(height<.5f||height>2.5f||radius<=0||radius>2||std::hypot(toX-x,toZ-z)>16)return false;
    const auto clear=[&](const scene::OverheadVolume& roof) {
        if(roof.topY<=footY+.001f)return true;
        float minX=roof.footprint[0][0],maxX=minX,minZ=roof.footprint[0][1],maxZ=minZ;
        for(const auto& p:roof.footprint){minX=std::min(minX,p[0]);maxX=std::max(maxX,p[0]);minZ=std::min(minZ,p[1]);maxZ=std::max(maxZ,p[1]);}
        float enter=0,leave=1;
        const auto axis=[&](float a,float b,float low,float high){const float d=b-a;
            if(std::abs(d)<1e-7f)return a>=low&&a<=high;
            float t0=(low-a)/d,t1=(high-a)/d;if(t0>t1)std::swap(t0,t1);
            enter=std::max(enter,t0);leave=std::min(leave,t1);return enter<=leave;};
        if(!axis(x,toX,minX-radius,maxX+radius)||!axis(z,toZ,minZ-radius,maxZ+radius))return true;
        const float bottom=std::min(scene::MinimumOverheadBottomY(roof,x+(toX-x)*enter,z+(toZ-z)*enter,radius),
                                    scene::MinimumOverheadBottomY(roof,x+(toX-x)*leave,z+(toZ-z)*leave,radius));
        return footY+height<=bottom+.001f;
    };
    for(const auto& v:scene::kShowcaseLowOverheadVolumes)if(!clear(v))return false;
    for(const auto& v:scene::kShowcaseCeilingPatches)if(!clear(v))return false;
    for(const auto& v:scene::kShowcaseSkylightGrid)if(!clear(v))return false;
    for(const auto& v:scene::kShowcaseImportedOverheadVolumes)if(!clear(v))return false;
    return clear(scene::kShowcaseCollapseRoofSeam)&&clear(scene::kDevelopmentRouteRoof);
}
}
