#pragma once
#include "gameplay/dialogue/CompanionActing.h"
#include "gameplay/dialogue/CompanionLocomotion.h"
#include "gameplay/dialogue/CompanionTurnProfile.h"
#include "gameplay/simulation/DevelopmentWorldRoute.h"
#include "gameplay/traversal/ChapterTerrainMovement.h"
#include "gameplay/traversal/BellwetherTavernRoute.h"
#include "../../../assets/models/world/source/bellwether-tavern-v1/staging.generated.h"
#include <algorithm>
#include <cmath>
namespace horde::gameplay::dialogue {
enum class CompanionWait { Reunion,PlayerLagging,PlayerInTomb,Obstacle,Lookout,Paused,Preparing,Tavern };
enum class BellwetherArrivalStage { AtLookout, OnRoad, AnkleAssessment, ProtectiveApproach, ChairTurn, Lowering, Seated };
inline constexpr float kTavernSitDurationSeconds=82.f/60.f;
struct CompanionSnapshot {
    // Logical engineering fixture only. No admitted mesh, enemy slot or GPU
    // skinning allocation is implied by this reusable route contract.
    bool visualAdmitted=false,walking=false,grounded=false; CompanionWait wait=CompanionWait::Reunion;
    float x=simulation::kWorldRoutePoints[3].x+1.0f;
    float z=simulation::kWorldRoutePoints[3].z;
    float y=simulation::kWorldRoutePoints[3].y,yaw=0,animationSeconds=0;
    std::size_t waypoint=4;std::uint64_t generation=1;
    bool basinGuide=false,basinArrived=false;
    CompanionActingPose acting{};
    CompanionLocomotionPose locomotion{};
    CompanionMotionPose motion{}; // Transient shared recipe, never serialized.
    BellwetherArrivalStage bellwetherArrival=BellwetherArrivalStage::AtLookout;
    std::size_t bellwetherStation=1;
};
class CompanionRouteFixture {
public:
    explicit CompanionRouteFixture(bool turnStartEnabled=false):turnStartEnabled_(turnStartEnabled) {ClearMotion();}
    const CompanionSnapshot& State() const {return state_;}
    void Reset() { const auto gen=state_.generation;state_={};acting_.Reset();locomotion_.Reset();ClearMotion();ankleAssessmentSeconds_=0;state_.generation=gen==UINT64_MAX?gen:gen+1; }
    void AdvanceActing(float seconds,const Snapshot& speech,bool exterior,bool alive,bool freshVisibleProof) {
        if(TavernSitActive())return;
        acting_.Step(seconds,speech,exterior,alive,state_.grounded,state_.walking,freshVisibleProof);
        state_.acting=acting_.State();
    }
    void RestoreCampaignReunion(bool exterior, float playerX, float playerZ,
        const simulation::TriangleSupportSurface* terrain, std::uint64_t generationFloor) {
        state_={};acting_.Reset();locomotion_.Reset();ClearMotion();state_.generation=generationFloor+1;
        if(exterior) Step(1.0f/60.0f,playerX,playerZ,true,false,false,terrain,true);
    }
    void RestoreCampaignLookout(float playerX, float playerZ,
        const simulation::TriangleSupportSurface* terrain, std::uint64_t generationFloor) {
        state_={};acting_.Reset();locomotion_.Reset();ClearMotion();state_.generation=generationFloor+1;
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
    void RestoreCampaignBasin(float playerX,float playerZ,
        const simulation::TriangleSupportSurface* terrain,std::uint64_t generationFloor) {
        state_={};acting_.Reset();locomotion_.Reset();ClearMotion();state_.generation=generationFloor+1;
        const auto basin=simulation::kWorldRoutePoints[2];
        state_.x=basin.x+1.45f;state_.z=basin.z+.8f;state_.waypoint=3;
        state_.basinGuide=state_.basinArrived=true;
        if(!terrain) {state_.grounded=false;state_.wait=CompanionWait::Preparing;return;}
        const auto support=traversal::ChapterExteriorStepSupport(*terrain,state_.x,state_.z,.24f);
        state_.grounded=support.grounded && traversal::ResolveChapterStepMovement(
            *terrain,state_.x,state_.z,state_.x,state_.z,support.worldY,.24f).accepted;
        state_.y=support.worldY;state_.wait=state_.grounded?CompanionWait::Reunion:CompanionWait::Obstacle;
        state_.yaw=std::atan2(playerX-state_.x,state_.z-playerZ);
    }
    bool BeginTavernSit() {
        using namespace bellwether;
        if(TavernSitActive() || !state_.grounded || state_.walking ||
           std::hypot(state_.x-kChairSitRoot[0],state_.z-kChairSitRoot[2])>.015f ||
           std::abs(state_.y-kChairSitRoot[1])>.01f ||
           std::abs(std::remainder(state_.yaw-kChairFacingRadians,6.28318530718f))>.02f)return false;
        locomotion_.Reset();acting_.Reset();ClearMotion();
        state_.locomotion=locomotion_.State();state_.acting=acting_.State();
        state_.motion.mode=CompanionMotionMode::Sitting;
        state_.wait=CompanionWait::Tavern;
        return true;
    }
    // Explicit diagnostic seed for the arrival handoff. The placement is
    // validated against the same scene as travel, before fixed-step playback.
    bool PlaceForTavernSitInspection(const simulation::TriangleSupportSurface& terrain) {
        using namespace bellwether;
        const auto [x,y,z]=kChairSitRoot;
        constexpr float radius=.24f;
        const auto support=traversal::ChapterExteriorStepSupport(terrain,x,z,radius);
        if(!support.grounded || std::abs(support.worldY-y)>.001f ||
           !traversal::ResolveChapterStepMovement(terrain,x,z,x,z,support.worldY,radius).accepted ||
           terrain.NonWalkableVolumeClear(kChairSeat[0],kChairSeat[2],kChairSeat[0],kChairSeat[2],
               kChairSeat[1]-.001f,kChairSeat[1]+.001f,.025f))return false;
        Reset();
        state_.x=x;state_.y=support.worldY;state_.z=z;
        state_.yaw=kChairFacingRadians;state_.grounded=true;
        return BeginTavernSit();
    }
    void Step(float dt,float playerX,float playerZ,bool exterior,bool proof,bool paused,
              const simulation::TriangleSupportSurface* terrain=nullptr,bool requireTerrain=false,bool active=true,
              bool guideBasin=false) {
        if(TavernSitActive()) {
            if(exterior && active && !paused && std::isfinite(dt) && dt>0 &&
               state_.motion.mode==CompanionMotionMode::Sitting) {
                auto& seconds=state_.motion.sitPlaybackSeconds;
                seconds+=std::min(dt,.05f);
                if(seconds>=kTavernSitDurationSeconds-.00001f) {
                    seconds=kTavernSitDurationSeconds;
                    state_.motion.mode=CompanionMotionMode::Seated;
                    if(state_.bellwetherArrival==BellwetherArrivalStage::Lowering)
                        state_.bellwetherArrival=BellwetherArrivalStage::Seated;
                }
            }
            return;
        }
        if(state_.bellwetherArrival!=BellwetherArrivalStage::AtLookout) {
            if(exterior && active && !paused && terrain && std::isfinite(dt) && dt>0)
                StepBellwetherArrival(std::min(dt,.05f),*terrain);
            return;
        }
        if(guideBasin && active && exterior && proof) {
            StepBasinGuide(dt,playerX,playerZ,paused,terrain,requireTerrain);return;
        }
        if(state_.basinGuide && !guideBasin) {
            // Resume the ordinary mapped route from the rope approach after
            // Kit has shown the earned basin. No new route or teleport.
            state_.basinGuide=false;state_.waypoint=3;ClearMotion();
        }
        if(turnStartEnabled_) {StepTurnStart(dt,playerX,playerZ,exterior,proof,paused,terrain,requireTerrain,active);return;}
        if(paused) {state_.wait=CompanionWait::Paused;return;}
        if(!std::isfinite(dt) || dt<=0) {
            if(requireTerrain&&!terrain) {state_.grounded=false;state_.walking=false;state_.wait=CompanionWait::Preparing;}
            return;
        }
        const auto oldYaw=state_.yaw;
        const auto oldX=state_.x,oldZ=state_.z;
        const auto oldWaypoint=state_.waypoint;
        StepRoute(dt,playerX,playerZ,exterior,proof,paused,terrain,requireTerrain);
        if(paused || !exterior || !state_.grounded || !std::isfinite(dt) || dt<=0) {
            state_.yaw=oldYaw;return;
        }
        dt=std::min(dt,.05f);
        // Reaching a waypoint consumes one bookkeeping tick without travel.
        // Keep its travel facing; ordinary idle still turns toward the player.
        const float target=(state_.walking || state_.waypoint!=oldWaypoint)?state_.yaw:
            (std::hypot(playerX-state_.x,playerZ-state_.z)>.01f?
                std::atan2(playerX-state_.x,state_.z-playerZ):oldYaw);
        const float difference=std::remainder(target-oldYaw,6.28318530718f);
        state_.yaw=oldYaw+std::clamp(difference,-2.4f*dt,2.4f*dt);
        state_.animationSeconds+=dt;
        locomotion_.Step(dt,std::hypot(state_.x-oldX,state_.z-oldZ),state_.walking);
        state_.locomotion=locomotion_.State();
    }
    bool TryStartBellwetherArrival(float playerX,float playerZ,const simulation::TriangleSupportSurface& terrain) {
        using namespace traversal;
        if(state_.bellwetherArrival!=BellwetherArrivalStage::AtLookout ||
           !state_.grounded || state_.waypoint!=simulation::kWorldRoutePoints.size() ||
           !std::isfinite(playerX) || !std::isfinite(playerZ))return false;
        const auto lookout=kBellwetherTavernRoute[0];
        const auto turnaround=kBellwetherTavernRoute[1];
        const float dx=turnaround.x-lookout.x,dz=turnaround.z-lookout.z;
        const float along=((playerX-lookout.x)*dx+(playerZ-lookout.z)*dz)/(dx*dx+dz*dz);
        if(along<.16f || along>1.12f || std::hypot(playerX-(lookout.x+dx*along),
            playerZ-(lookout.z+dz*along))>2.0f)return false;

        // Admitted ground and solids are the authority for the road opening.
        // A historical scene with the old turnaround rails cannot start it.
        const auto road=kBellwetherTavernRoute[2];
        const auto roadX=[&](float z){return std::lerp(turnaround.x,road.x,(z-turnaround.z)/(road.z-turnaround.z));};
        constexpr float beforeRails=52.8f,afterRails=55.6f;
        const auto support=ChapterExteriorSupport(&terrain,roadX(beforeRails),beforeRails);
        if(!support.grounded || !ChapterExteriorMovementClear(terrain,roadX(beforeRails),beforeRails,
                roadX(afterRails),afterRails,support.worldY,.24f))return false;
        state_.bellwetherArrival=BellwetherArrivalStage::OnRoad;
        state_.wait=CompanionWait::Tavern;
        return true;
    }
private:
    bool TavernSitActive() const {
        return state_.motion.mode==CompanionMotionMode::Sitting || state_.motion.mode==CompanionMotionMode::Seated;
    }
#include "gameplay/dialogue/CompanionTurnPrototype.inl"
#include "gameplay/dialogue/CompanionBellwetherArrival.inl"
    void StepBasinGuide(float dt,float playerX,float playerZ,bool paused,
                        const simulation::TriangleSupportSurface* terrain,bool requireTerrain) {
        state_.basinGuide=true;state_.walking=false;
        if(paused) {state_.wait=CompanionWait::Paused;return;}
        if(!std::isfinite(dt)||dt<=0 || !std::isfinite(playerX)||!std::isfinite(playerZ)) return;
        if(requireTerrain&&!terrain) {state_.grounded=false;state_.wait=CompanionWait::Preparing;return;}
        const auto under=requireTerrain?traversal::ChapterExteriorStepSupport(*terrain,state_.x,state_.z,.24f):
            simulation::ResolveWorldRouteSupport(state_.x,state_.z);
        state_.grounded=under.grounded;
        if(!state_.grounded) {state_.wait=CompanionWait::Obstacle;return;}
        state_.y=under.worldY;
        // This guide uses the distance-driven walk recipe, never a retained
        // stationary turn/start pose from the ordinary route.
        ClearMotion();
        if(state_.basinArrived) {
            state_.wait=CompanionWait::Reunion;
            locomotion_.Step(std::min(dt,.05f),0,false);state_.locomotion=locomotion_.State();
            state_.animationSeconds+=std::min(dt,.05f);return;
        }
        if(std::hypot(playerX-state_.x,playerZ-state_.z)>6.f) {
            state_.wait=CompanionWait::PlayerLagging;
            locomotion_.Step(std::min(dt,.05f),0,false);state_.locomotion=locomotion_.State();return;
        }
        // F01 (index 2) is the rope landing. The landing itself is reserved
        // for traversal; Kit stops just outside its boundary, beside the
        // visible basin, while the player retains the central approach.
        auto target=simulation::kWorldRoutePoints[2];
        target.x+=1.45f;target.z+=.8f;
        const float dx=target.x-state_.x,dz=target.z-state_.z,distance=std::hypot(dx,dz);
        if(distance<.08f) {
            state_.basinArrived=true;
            return;
        }
        const float travel=std::min(distance,std::min(dt,.05f)*1.8f);
        const float x=state_.x+dx/distance*travel,z=state_.z+dz/distance*travel;
        const auto step=requireTerrain?traversal::ResolveChapterStepMovement(
            *terrain,state_.x,state_.z,x,z,state_.y,.24f):traversal::ChapterStepMovement{};
        const auto support=requireTerrain?step.support:simulation::ResolveWorldRouteSupport(x,z);
        const bool clear=requireTerrain?step.accepted:
            simulation::WorldRouteBlockoutMovementClear(state_.x,state_.z,x,z,state_.y,.24f,true);
        if(!support.grounded||!clear) {
            state_.wait=CompanionWait::Obstacle;
            locomotion_.Step(std::min(dt,.05f),0,false);state_.locomotion=locomotion_.State();return;
        }
        const float oldX=state_.x,oldZ=state_.z;
        state_.walking=travel>0;state_.grounded=true;state_.x=x;state_.z=z;state_.y=support.worldY;
        state_.motion.mode=CompanionMotionMode::Walking;state_.motion.speed=travel/std::min(dt,.05f);
        state_.yaw=std::atan2(dx,-dz);state_.wait=CompanionWait::Reunion;
        state_.animationSeconds+=std::min(dt,.05f);
        locomotion_.Step(std::min(dt,.05f),std::hypot(x-oldX,z-oldZ),state_.walking);
        state_.locomotion=locomotion_.State();
    }
    void StepRoute(float dt,float playerX,float playerZ,bool exterior,bool proof,bool paused,
                   const simulation::TriangleSupportSurface* terrain,bool requireTerrain) {
        state_.walking=false;
        if(requireTerrain&&!terrain) {state_.grounded=false;state_.wait=CompanionWait::Preparing;return;}
        if(paused) {state_.wait=CompanionWait::Paused;return;}
        if(!exterior) {state_.wait=CompanionWait::PlayerInTomb;return;}
        if(requireTerrain) {
            const auto under=traversal::ChapterExteriorStepSupport(*terrain,state_.x,state_.z,.24f);
            state_.grounded=under.grounded&&traversal::ResolveChapterStepMovement(
                *terrain,state_.x,state_.z,state_.x,state_.z,under.worldY,.24f).accepted;
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
        // Use the same bounded STEP lane as the player. Ordinary props, trunks,
        // headroom and unsupported edges remain solid; no per-root exception.
        const auto step=requireTerrain?traversal::ResolveChapterStepMovement(
            *terrain,state_.x,state_.z,x,z,state_.y,.24f):traversal::ChapterStepMovement{};
        const auto support=requireTerrain?step.support:simulation::ResolveWorldRouteSupport(x,z);
        const bool clear=requireTerrain?step.accepted:
            simulation::WorldRouteBlockoutMovementClear(state_.x,state_.z,x,z,state_.y,.24f,true);
        if(!support.grounded||!clear) {
            state_.wait=CompanionWait::Obstacle;return;
        }
        state_.walking=travel>0;state_.grounded=true;state_.x=x;state_.z=z;state_.y=support.worldY;
        state_.motion.mode=CompanionMotionMode::Walking;state_.motion.speed=travel/std::min(dt,.05f);
        state_.yaw=std::atan2(dx,-dz);
    }
    CompanionSnapshot state_{};
    CompanionActing acting_{};
    CompanionLocomotion locomotion_{};
    bool turnStartEnabled_=false;
    double turnElapsed_=0,entryElapsed_=0;
    float turnStartYaw_=0,entryStartWeight_=0;
    float ankleAssessmentSeconds_=0;
};
} // namespace horde::gameplay::dialogue
