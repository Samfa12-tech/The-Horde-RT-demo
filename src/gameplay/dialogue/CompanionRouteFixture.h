#pragma once
#include "gameplay/simulation/DevelopmentWorldRoute.h"
#include "gameplay/traversal/ChapterTerrainMovement.h"
#include <algorithm>
#include <cmath>
namespace horde::gameplay::dialogue {
enum class CompanionWait { Reunion,PlayerLagging,PlayerInTomb,Obstacle,Lookout,Paused,Preparing };
struct CompanionSnapshot {
    // Logical engineering fixture only. No admitted mesh, enemy slot or GPU
    // skinning allocation is implied by this reusable route contract.
    bool visualAdmitted=false,walking=false,grounded=false; CompanionWait wait=CompanionWait::Reunion;
    float x=simulation::kWorldRoutePoints[3].x+1.0f;
    float z=simulation::kWorldRoutePoints[3].z;
    float y=simulation::kWorldRoutePoints[3].y,yaw=0,animationSeconds=0;
    std::size_t waypoint=4;std::uint64_t generation=1;
};
class CompanionRouteFixture {
public:
    const CompanionSnapshot& State() const {return state_;}
    void Reset() { const auto gen=state_.generation;state_={};state_.generation=gen==UINT64_MAX?gen:gen+1; }
    void RestoreCampaignReunion(bool exterior, float playerX, float playerZ,
        const simulation::TriangleSupportSurface* terrain, std::uint64_t generationFloor) {
        state_={};state_.generation=generationFloor+1;
        if(exterior) Step(1.0f/60.0f,playerX,playerZ,true,false,false,terrain,true);
    }
    void RestoreCampaignLookout(float playerX, float playerZ,
        const simulation::TriangleSupportSurface* terrain, std::uint64_t generationFloor) {
        state_={};state_.generation=generationFloor+1;
        const auto endpoint=simulation::kWorldRoutePoints.back();
        state_.x=endpoint.x+1.0f;state_.z=endpoint.z;
        state_.waypoint=simulation::kWorldRoutePoints.size();
        const auto support=traversal::ChapterExteriorSupport(terrain,state_.x,state_.z);
        state_.grounded=support.grounded && terrain && traversal::ChapterExteriorMovementClear(
            *terrain,state_.x,state_.z,state_.x,state_.z,support.worldY,.24f) &&
            terrain->HorizontalBodyVolumeClear(state_.x,state_.z,state_.x,state_.z,support.worldY,1.8f,.24f) &&
            traversal::ChapterExteriorObstacleClear(*terrain,state_.x,state_.z,state_.x,state_.z,
                support.worldY+.001f,support.worldY+1.8f,.24f);
        state_.y=support.worldY;state_.wait=state_.grounded?CompanionWait::Lookout:CompanionWait::Obstacle;
        state_.yaw=std::atan2(playerX-state_.x,state_.z-playerZ);
    }
    void Step(float dt,float playerX,float playerZ,bool exterior,bool proof,bool paused,
              const simulation::TriangleSupportSurface* terrain=nullptr,bool requireTerrain=false) {
        if(paused) {state_.wait=CompanionWait::Paused;return;}
        if(!std::isfinite(dt) || dt<=0) {
            if(requireTerrain&&!terrain) {state_.grounded=false;state_.walking=false;state_.wait=CompanionWait::Preparing;}
            return;
        }
        const auto oldYaw=state_.yaw;
        const bool wasWalking=state_.walking;
        StepRoute(dt,playerX,playerZ,exterior,proof,paused,terrain,requireTerrain);
        if(paused || !exterior || !state_.grounded || !std::isfinite(dt) || dt<=0) {
            state_.yaw=oldYaw;return;
        }
        dt=std::min(dt,.05f);
        const float target=state_.walking?state_.yaw:
            (std::hypot(playerX-state_.x,playerZ-state_.z)>.01f?
                std::atan2(playerX-state_.x,state_.z-playerZ):oldYaw);
        const float difference=std::remainder(target-oldYaw,6.28318530718f);
        state_.yaw=oldYaw+std::clamp(difference,-2.4f*dt,2.4f*dt);
        state_.animationSeconds=wasWalking==state_.walking?state_.animationSeconds+dt:0;
    }
private:
    void StepRoute(float dt,float playerX,float playerZ,bool exterior,bool proof,bool paused,
                   const simulation::TriangleSupportSurface* terrain,bool requireTerrain) {
        state_.walking=false;
        if(requireTerrain&&!terrain) {state_.grounded=false;state_.wait=CompanionWait::Preparing;return;}
        if(paused) {state_.wait=CompanionWait::Paused;return;}
        if(!exterior) {state_.wait=CompanionWait::PlayerInTomb;return;}
        if(requireTerrain) {
            const auto under=traversal::ChapterExteriorSupport(terrain,state_.x,state_.z);
            state_.grounded=under.grounded&&traversal::ChapterExteriorMovementClear(
                *terrain,state_.x,state_.z,state_.x,state_.z,under.worldY,.24f);
            if(!state_.grounded) {state_.wait=CompanionWait::Obstacle;return;}
            state_.y=under.worldY;
        }
        if(!proof) {state_.wait=CompanionWait::Reunion;return;}
        if(!std::isfinite(dt)||!std::isfinite(playerX)||!std::isfinite(playerZ)||
            std::hypot(playerX-state_.x,playerZ-state_.z)>6.0f) {
            state_.wait=CompanionWait::PlayerLagging;return;
        }
        if(state_.waypoint>=simulation::kWorldRoutePoints.size()) {state_.wait=CompanionWait::Lookout;return;}
        auto p=simulation::kWorldRoutePoints[state_.waypoint];p.x+=1.0f;
        const float dx=p.x-state_.x,dz=p.z-state_.z,distance=std::hypot(dx,dz);
        if(distance<.06f) {++state_.waypoint;return;}
        const float travel=std::min(distance,std::clamp(dt,0.0f,.05f)*1.8f);
        const float x=state_.x+dx/distance*travel,z=state_.z+dz/distance*travel;
        const auto support=requireTerrain?traversal::ChapterExteriorSupport(terrain,x,z):
            simulation::ResolveWorldRouteSupport(x,z);
        const bool clear=requireTerrain?traversal::ChapterExteriorMovementClear(
            *terrain,state_.x,state_.z,x,z,state_.y,.24f):
            simulation::WorldRouteBlockoutMovementClear(state_.x,state_.z,x,z,state_.y,.24f,true);
        if(!support.grounded||!clear) {
            state_.wait=CompanionWait::Obstacle;return;
        }
        state_.walking=travel>0;state_.grounded=true;state_.x=x;state_.z=z;state_.y=support.worldY;
        state_.yaw=std::atan2(dx,-dz);
    }
    CompanionSnapshot state_{};
};
} // namespace horde::gameplay::dialogue
