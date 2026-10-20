#include "gameplay/dialogue/CompanionRouteFixture.h"
#include <iostream>
#include <limits>
using namespace horde::gameplay;
using namespace dialogue;
namespace {
int checks=0,failures=0;
void Check(bool ok,const char* message){++checks;if(!ok){++failures;std::cerr<<"FAIL: "<<message<<'\n';}}
}
int main(){
    constexpr float dt=1.f/60;
    CompanionLocomotion motion;
    motion.Step(dt,.03f,true);
    Check(motion.State().walkWeight>0 && motion.State().walkWeight<.1f,"first moving step blends rather than switching directly to walking");
    Check(std::abs(motion.State().walkPhase-.03f/CompanionLocomotion::kWalkCycleDistanceMetres)<1e-6f,"cadence follows actual route distance in metres");
    for(int i=1;i<11;++i)motion.Step(dt,.03f,true);
    Check(motion.State().walkWeight==1,"walking reaches its unmodified authored clip within eleven fixed steps");
    const auto moving=motion.State();motion.Step(dt,0,false);
    Check(motion.State().walkWeight>0 && motion.State().walkWeight<1 && motion.State().walkPhase==moving.walkPhase,
        "stopping retains the last walk phase and blends smoothly to idle");
    for(int i=1;i<11;++i)motion.Step(dt,0,false);
    Check(motion.State().walkWeight==0 && motion.State().walkPhase==moving.walkPhase,"stationary idle does not pedal or reset feet");
    motion.Step(dt,.03f,true);
    Check(motion.State().walkPhase>moving.walkPhase,"restart resumes the persistent gait phase");
    const auto valid=motion.State();
    for(float bad:{0.f,-1.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()})motion.Step(bad,0,false);
    motion.Step(dt,-1,true);motion.Step(dt,std::numeric_limits<float>::infinity(),true);
    Check(motion.State()==valid,"invalid time or travel cannot poison or advance the pose");
    motion.Reset();motion.Step(.05f,CompanionLocomotion::kWalkCycleDistanceMetres,true);
    Check(std::abs(motion.State().walkPhase)<1e-6f,"one measured stride wraps exactly one gait cycle");
    for(int i=0;i<600;++i)motion.Step(dt,.03f,true);
    Check(motion.State().walkPhase>=0 && motion.State().walkPhase<1 && motion.State().walkWeight==1,"long walking keeps a finite bounded gait phase");

    using Terrain=simulation::TriangleSupportSurface;
    const std::array<Terrain::Triangle,2> land{{
        {{{{-1000,7,-1000},{-1000,7,1000},{1000,7,1000}}}},
        {{{{-1000,7,-1000},{1000,7,1000},{1000,7,-1000}}}}
    }};
    std::string error;const auto terrain=Terrain::Build(land,error);Check(bool(terrain),"route fixture native flat support imports");
    CompanionRouteFixture route;const auto origin=route.State();
    route.Step(dt,origin.x,origin.z,true,false,false,terrain.get(),true);
    Check(route.State().locomotion==CompanionLocomotionPose{},"unproved reunion stays visually idle");
    const auto before=route.State();route.Step(dt,before.x,before.z,true,true,false,terrain.get(),true);
    const auto walking=route.State();
    Check(walking.walking && std::abs(std::hypot(walking.x-before.x,walking.z-before.z)-1.8f*dt)<2e-6f,
        "visual blend does not delay or accelerate the 1.8 metre per second route motor");
    Check(walking.locomotion.walkWeight>0 && walking.locomotion.walkWeight<1 && walking.animationSeconds>before.animationSeconds,
        "route publishes a partial blend with a continuous idle clock");
    route.Step(dt,walking.x,walking.z,true,true,true,terrain.get(),true);
    Check(route.State().locomotion==walking.locomotion && route.State().animationSeconds==walking.animationSeconds &&
        route.State().yaw==walking.yaw && route.State().x==walking.x && route.State().z==walking.z,"pause freezes exact movement pose clocks root and facing");
    route.Step(0,walking.x,walking.z,true,true,false,terrain.get(),true);
    Check(route.State().locomotion==walking.locomotion,"zero-delta snapshot finalization freezes the gait");
    bool boundary=false;unsigned ticks=0;
    for(;ticks<4000 && route.State().waypoint<simulation::kWorldRoutePoints.size();++ticks){
        const auto old=route.State();route.Step(dt,old.x,old.z,true,true,false,terrain.get(),true);
        const auto now=route.State();
        const float distance=std::hypot(now.x-old.x,now.z-old.z);
        Check(distance<=1.8f*dt+3e-6f && now.grounded,"every route step retains the original speed ceiling and native support");
        if(now.waypoint!=old.waypoint){
            boundary=true;Check(now.locomotion.walkPhase==old.locomotion.walkPhase &&
                now.locomotion.walkWeight>.8f,"one waypoint bookkeeping tick retains gait phase and nearly full walking weight");
        }
    }
    Check(boundary && route.State().waypoint==simulation::kWorldRoutePoints.size(),"unchanged motor reaches all four remaining landmarks");
    const auto end=simulation::kWorldRoutePoints.back();
    Check(std::hypot(route.State().x-(end.x+1),route.State().z-end.z)<.06f,"lookout arrival remains the authored offset and tolerance");
    const auto stopped=route.State();
    for(int i=0;i<12;++i)route.Step(dt,stopped.x-1,stopped.z,true,true,false,terrain.get(),true);
    Check(route.State().locomotion.walkWeight==0 && route.State().x==stopped.x && route.State().z==stopped.z,
        "completed route blends to stationary idle without changing its endpoint");
    const auto gen=route.State().generation;
    route.RestoreCampaignLookout(end.x,end.z,terrain.get(),gen);
    Check(route.State().locomotion==CompanionLocomotionPose{} && route.State().animationSeconds==0 && route.State().generation>gen,
        "restored saves start neutral transient movement without adding serialized state");
    route.Reset();Check(route.State().locomotion==CompanionLocomotionPose{},"reset clears movement blend and phase");
    std::cout<<"companion locomotion checks="<<checks<<" failures="<<failures<<" routeTicks="<<ticks<<'\n';
    return failures?1:0;
}
