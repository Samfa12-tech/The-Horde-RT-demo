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
    float fromX,float fromZ,float toX,float toZ,float minimumY,float maximumY,float radius,
    bool includeSteps=true)
{
    if(!RescueExteriorVolumeClear(fromX,fromZ,toX,toZ,minimumY,maximumY,radius))return false;
    if(!(includeSteps?terrain.NonWalkableVolumeClear(fromX,fromZ,toX,toZ,minimumY,maximumY,radius):
                     terrain.UnsteppableVolumeClear(fromX,fromZ,toX,toZ,minimumY,maximumY,radius)))return false;
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
// Separate opt-in step support: bare ground remains the terrain/tree-placement
// authority. This footprint maximum is deliberately conservative around roots.
inline simulation::PlayerSupportResolution ChapterExteriorStepSupport(
    const simulation::TriangleSupportSurface& terrain,float x,float z,float radius)
{
    auto support=ChapterExteriorSupport(&terrain,x,z);
    if(!support.grounded || OnRescueLanding(x,z))return support;
    const auto step=terrain.StepSupport(x,z,x,z,support.worldY,
        support.worldY+simulation::kWorldRouteMaximumStep,radius);
    if(step.found && step.worldY>support.worldY &&
       terrain.StepVolumeClear(x,z,x,z,step.worldY+.001f,step.worldY+1.8f,radius)) {
        support.worldY=step.worldY;support.surface=step.surface;
        // Distinct from bare ground and the authored landing identities.
        support.id=static_cast<simulation::PlayerSupportId>(100000+step.triangle);
    }
    return support;
}
struct ChapterStepMovement {
    bool accepted=false;
    simulation::PlayerSupportResolution support{};
};
inline ChapterStepMovement ResolveChapterStepMovement(
    const simulation::TriangleSupportSurface& terrain,float fromX,float fromZ,float toX,float toZ,
    float previousFootY,float radius,float bodyHeight=1.8f)
{
    using namespace simulation;
    ChapterStepMovement result;
    if(!std::isfinite(bodyHeight)||bodyHeight<.5f||bodyHeight>2.5f||!std::isfinite(previousFootY))return result;
    const auto start=ChapterExteriorSupport(&terrain,fromX,fromZ);
    const auto end=ChapterExteriorSupport(&terrain,toX,toZ);
    if(!start.grounded||!end.grounded || OnRescueLanding(fromX,fromZ)||OnRescueLanding(toX,toZ))return result;
    const auto current=ChapterExteriorStepSupport(terrain,fromX,fromZ,radius);
    if(std::abs(current.worldY-previousFootY)>.001f ||
       !terrain.GroundSweep(fromX,fromZ,toX,toZ,start.worldY,radius,kWorldRouteMaximumStep))return result;
    result.support=ChapterExteriorStepSupport(terrain,toX,toZ,radius);
    const float limit=std::min(previousFootY+kWorldRouteMaximumStep,
                              std::min(start.worldY,end.worldY)+kWorldRouteMaximumStep);
    const auto swept=terrain.StepSupport(fromX,fromZ,toX,toZ,std::min(start.worldY,end.worldY),limit,radius);
    const float raisedFoot=std::max({previousFootY,result.support.worldY,swept.found?swept.worldY:previousFootY});
    if(std::abs(result.support.worldY-previousFootY)>kWorldRouteMaximumStep+.0001f ||
       raisedFoot-previousFootY>kWorldRouteMaximumStep+.0001f)return result;
    // Never exempt an entire step object: taller/steep/downward faces remain
    // solid above the raised foot. Ordinary props cannot be stepped over even
    // when their height happens to fit below the raised horizontal sweep.
    const float bottom=std::min(previousFootY,result.support.worldY)+.001f,top=raisedFoot+bodyHeight;
    result.accepted=ChapterExteriorObstacleClear(terrain,fromX,fromZ,toX,toZ,bottom,top,radius,false) &&
        terrain.StepVolumeClear(fromX,fromZ,toX,toZ,raisedFoot+.001f,top,radius) &&
        terrain.HorizontalBodyVolumeClear(fromX,fromZ,toX,toZ,raisedFoot,bodyHeight,radius);
    return result;
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
