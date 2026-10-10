#pragma once
#include "gameplay/simulation/DevelopmentWorldRoute.h"
#include <algorithm>
#include <cmath>
namespace horde::gameplay::dialogue {
enum class CompanionWait { Reunion,PlayerLagging,PlayerInTomb,Obstacle,Lookout,Paused };
struct CompanionSnapshot {
    // Logical engineering fixture only. No admitted mesh, enemy slot or GPU
    // skinning allocation is implied by this reusable route contract.
    bool visualAdmitted=false,walking=false; CompanionWait wait=CompanionWait::Reunion;
    float x=simulation::kWorldRoutePoints[3].x+1.0f;
    float z=simulation::kWorldRoutePoints[3].z;
    float y=simulation::kWorldRoutePoints[3].y,yaw=0;
    std::size_t waypoint=4;std::uint64_t generation=1;
};
class CompanionRouteFixture {
public:
    const CompanionSnapshot& State() const {return state_;}
    void Reset() { const auto gen=state_.generation;state_={};state_.generation=gen==UINT64_MAX?gen:gen+1; }
    void Step(float dt,float playerX,float playerZ,bool exterior,bool proof,bool paused) {
        state_.walking=false;
        if(paused) {state_.wait=CompanionWait::Paused;return;}
        if(!exterior) {state_.wait=CompanionWait::PlayerInTomb;return;}
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
        const auto support=simulation::ResolveWorldRouteSupport(x,z);
        if(!support.grounded||!simulation::WorldRouteBlockoutMovementClear(state_.x,state_.z,x,z,state_.y,.24f,true)) {
            state_.wait=CompanionWait::Obstacle;return;
        }
        state_.walking=travel>0;state_.x=x;state_.z=z;state_.y=support.worldY;
        state_.yaw=std::atan2(dx,-dz);
    }
private: CompanionSnapshot state_{};
};
} // namespace horde::gameplay::dialogue
