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
inline bool ChapterExteriorObstacleClear(const simulation::TriangleSupportSurface& terrain,
    float fromX,float fromZ,float toX,float toZ,float minimumY,float maximumY,float radius)
{
    if(!RescueExteriorVolumeClear(fromX,fromZ,toX,toZ,minimumY,maximumY,radius))return false;
    if(!terrain.NonWalkableVolumeClear(fromX,fromZ,toX,toZ,minimumY,maximumY,radius))return false;
    // Trunk horizontal bounds remain the measured admitted bark bounds. The
    // vertical origin follows the very same terrain sample used for rendering.
    for(const auto& tree:simulation::kForestTreePlacementContract) {
        const auto ground=terrain.Query(tree.x,tree.z);
        if(!ground.found)return false;
        if(ground.worldY+2<=minimumY || ground.worldY>=maximumY)continue;
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
inline bool ChapterExteriorMovementClear(const simulation::TriangleSupportSurface& terrain,
    float fromX,float fromZ,float toX,float toZ,float supportY,float radius)
{
    return ChapterExteriorObstacleClear(terrain,fromX,fromZ,toX,toZ,supportY+.001f,
        supportY+(kShowcaseEyeWorldY-kRouteFloorWorldY),radius)&&
        terrain.GroundSweep(fromX,fromZ,toX,toZ,supportY,radius,simulation::kWorldRouteMaximumStep);
}
inline bool ChapterExteriorCarryClear(const simulation::TriangleSupportSurface& terrain,
    float fromX,float fromZ,float toX,float toZ,float supportY,float radius)
{
    // Bounded conservative upright carry band. It clears the 18cm coping and
    // the ground beneath the cage, while retaining raised banks and real
    // trunks/shaft fittings. Body movement still requires continuous support.
    constexpr float bottom=.25f,top=2.30f;
    return ChapterExteriorObstacleClear(terrain,fromX,fromZ,toX,toZ,supportY+bottom,
        supportY+top,radius)&&terrain.HorizontalVolumeClear(fromX,fromZ,toX,toZ,
        supportY+bottom,supportY+top,radius);
}
}
