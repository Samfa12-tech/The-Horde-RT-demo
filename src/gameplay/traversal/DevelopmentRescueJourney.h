#pragma once
#include "gameplay/simulation/DevelopmentWorldRoute.h"
#include "gameplay/traversal/RescueTraversal.h"
#include "scene/RescueBlockoutGeometry.h"
#include <array>
#include <algorithm>
#include <cmath>

namespace horde::gameplay::traversal {
// Reversible development connector from the rescue landing through the rebased
// F01/F02/F03 route points. The single planned-world translation in
// DevelopmentWorldRoute now puts F01 directly on the actual rope landing.
inline constexpr std::array<simulation::WorldRoutePoint,5> kRescueConnector{{
    simulation::kWorldRoutePoints[2],simulation::kWorldRoutePoints[3],
    simulation::kWorldRoutePoints[4],simulation::kWorldRoutePoints[5],
    simulation::kWorldRoutePoints[6]}};
inline bool OnRescueLanding(float x,float z) {
    return std::isfinite(x)&&std::isfinite(z)&&horde::scene::RescueLandingContains(x,z);
}
inline simulation::WorldRouteProjection ProjectRescueConnector(float x,float z) {
    return simulation::ProjectWorldRoute(x,z);
}
inline simulation::PlayerSupportResolution RescueExteriorSupport(float x,float z) {
    using namespace simulation;
    if(OnRescueLanding(x,z)) return {kUpperSupportWorldY,static_cast<PlayerSupportId>(130),true};
    const auto p=ProjectWorldRoute(x,z);
    if(!p.valid||p.segment<1) return {kRouteFloorWorldY,PlayerSupportId::RouteFloor,false};
    return ResolveWorldRouteSupport(x,z);
}
inline bool RescueExteriorMovementClear(float fromX,float fromZ,float toX,float toZ,
                                        float supportY,float radius) {
    if(!std::isfinite(fromX)||!std::isfinite(fromZ)||!std::isfinite(toX)||
       !std::isfinite(toZ)||!std::isfinite(supportY)||!std::isfinite(radius)||radius<=0)
        return false;
    // Sweep the conservative horizontal capsule envelope through the actual
    // retained blockout boxes. Geometry at/below the feet is support, and
    // geometry above the baseline body height is not an obstacle. This does
    // not add a general step/fall solver or apply to the traversal handoff.
    for(const auto& box:horde::scene::kRescueBlockoutBoxes) {
        if(box.maximum[1]<=supportY+.001f ||
           box.minimum[1]>=supportY+(kShowcaseEyeWorldY-kRouteFloorWorldY)) continue;
        float enter=0.0f,leave=1.0f;
        const auto axis=[&](float from,float to,float minimum,float maximum) {
            const float delta=to-from;
            if(std::abs(delta)<1e-7f) return from>=minimum && from<=maximum;
            float a=(minimum-from)/delta,b=(maximum-from)/delta;
            if(a>b) std::swap(a,b);
            enter=std::max(enter,a);leave=std::min(leave,b);
            return enter<=leave;
        };
        if(axis(fromX,toX,box.minimum[0]-radius,box.maximum[0]+radius) &&
           axis(fromZ,toZ,box.minimum[2]-radius,box.maximum[2]+radius)) return false;
    }
    return true;
}
inline bool AtRopeEndpoint(float x,float z,bool exterior) {
    const auto p=exterior?kExteriorLanding:kLowerLanding;
    return std::isfinite(x)&&std::isfinite(z)&&
        std::hypot(x-p.x,z-p.z)<=kRescueApproachRadius;
}
inline bool ValidRescueCapsulePath(const RescueTraversalSnapshot& s) {
    const auto p=s.playerPosition;
    if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z) ||
       p.x<-34.94f+.20f || p.x>-32.46f-.20f || p.z<-16.64f+.20f || p.z>-11.92f-.20f)
        return false;
    // Bounded authored capsule clearance through the front coping; no general
    // 3D physics or sword occlusion claim follows from this route validator.
    if(p.z>=-13.76f-.20f && p.z<=-13.58f+.20f && p.y<2.23f+.04f) return false;
    return p.y>=kLowerSupportWorldY-.001f && p.y<=2.28f+.001f;
}
inline bool CanApproachRopeEndpoint(Vec3 start,float supportWorldY,bool exterior) {
    const Vec3 endpoint=exterior?kExteriorLanding:kLowerLanding;
    const float expectedSupport=exterior?kUpperSupportWorldY:kLowerSupportWorldY;
    if(!std::isfinite(start.x)||!std::isfinite(start.y)||!std::isfinite(start.z)||
       !std::isfinite(supportWorldY)||std::abs(supportWorldY-expectedSupport)>.10f||
       std::hypot(start.x-endpoint.x,start.z-endpoint.z)>kRescueApproachRadius)
        return false;
    // Validate the complete swept capsule centre line, including the current
    // pose, against the shaft/coping envelope and retained upper support mesh.
    // In particular, a nominally nearby player at y=2.05 cannot walk through
    // the front coping just because the endpoint itself is safe.
    constexpr int samples=24;
    for(int i=0;i<=samples;++i) {
        const float t=static_cast<float>(i)/samples;
        RescueTraversalSnapshot pose;
        pose.playerPosition={start.x+(endpoint.x-start.x)*t,expectedSupport,
                             start.z+(endpoint.z-start.z)*t};
        pose.supportWorldY=expectedSupport;
        if(!ValidRescueCapsulePath(pose)) return false;
        if(exterior) {
            const auto retained=RescueExteriorSupport(pose.playerPosition.x,pose.playerPosition.z);
            if(!retained.grounded||std::abs(retained.worldY-expectedSupport)>.10f) return false;
        }
    }
    return true;
}
enum class RescuePrompt : unsigned { None, Climb, Descend, Preparing, Traversing };
} // namespace horde::gameplay::traversal
