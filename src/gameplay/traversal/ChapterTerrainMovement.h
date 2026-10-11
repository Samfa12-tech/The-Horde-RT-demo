#pragma once
#include "gameplay/traversal/DevelopmentRescueJourney.h"
#include "gameplay/simulation/TriangleSupportSurface.h"

namespace horde::gameplay::traversal
{
inline simulation::PlayerSupportResolution ChapterExteriorSupport(
    const simulation::TriangleSupportSurface* terrain,float x,float z)
{
    using namespace simulation;
    if(!terrain)return {kRouteFloorWorldY,PlayerSupportId::RouteFloor,false};
    if(OnRescueLanding(x,z))return {kUpperSupportWorldY,static_cast<PlayerSupportId>(130),true};
    const auto ground=terrain->Query(x,z);
    return {ground.worldY,static_cast<PlayerSupportId>(1000+ground.triangle),
            ground.found&&ground.walkable,ground.surface};
}
inline bool ChapterExteriorMovementClear(const simulation::TriangleSupportSurface& terrain,
    float fromX,float fromZ,float toX,float toZ,float supportY,float radius)
{
    if(!RescueExteriorMovementClear(fromX,fromZ,toX,toZ,supportY,radius) ||
       !terrain.GroundSweep(fromX,fromZ,toX,toZ,supportY,radius,simulation::kWorldRouteMaximumStep))return false;
    // Trunk horizontal bounds remain the measured admitted bark bounds. The
    // vertical origin follows the very same terrain sample used for rendering.
    for(const auto& tree:simulation::kForestTreePlacementContract) {
        const auto ground=terrain.Query(tree.x,tree.z);
        if(!ground.found)return false;
        if(ground.worldY+2<=supportY+.001f || ground.worldY>=supportY+(kShowcaseEyeWorldY-kRouteFloorWorldY))continue;
        float enter=0,leave=1;
        const auto axis=[&](float a,float b,float low,float high) {
            const float d=b-a;
            if(std::abs(d)<1e-7f)return a>=low&&a<=high;
            float t0=(low-a)/d,t1=(high-a)/d;if(t0>t1)std::swap(t0,t1);
            enter=std::max(enter,t0);leave=std::min(leave,t1);return enter<=leave;
        };
        if(axis(fromX,toX,tree.trunkMinX-radius,tree.trunkMaxX+radius)&&
           axis(fromZ,toZ,tree.trunkMinZ-radius,tree.trunkMaxZ+radius))return false;
    }
    return true;
}
}
