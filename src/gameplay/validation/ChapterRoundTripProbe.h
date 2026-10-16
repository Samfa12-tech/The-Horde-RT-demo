#pragma once
#include "gameplay/simulation/InputSnapshot.h"
#include "gameplay/simulation/SimulationSnapshot.h"
#include "gameplay/traversal/RescueTraversal.h"
#include <cmath>
#include <algorithm>
#include <string_view>

namespace horde::gameplay::validation {
enum class ChapterProbePhase { Landing, Raise, Outbound, Lookout, Return, Descent, Lower, TombReturn, Passage, RopeReturn, Ascent, Complete, Failed };
inline constexpr std::string_view ChapterProbePhaseName(ChapterProbePhase phase) {
    switch(phase) {
    case ChapterProbePhase::Landing:return "landing-sweep";
    case ChapterProbePhase::Raise:return "fresh-raise";
    case ChapterProbePhase::Outbound:return "outbound";
    case ChapterProbePhase::Lookout:return "lookout-sweep";
    case ChapterProbePhase::Return:return "return";
    case ChapterProbePhase::Descent:return "descent";
    case ChapterProbePhase::Lower:return "lower-sweep";
    case ChapterProbePhase::TombReturn:return "tomb-backtrack";
    case ChapterProbePhase::Passage:return "passage-sweep";
    case ChapterProbePhase::RopeReturn:return "rope-return";
    case ChapterProbePhase::Ascent:return "repeat-ascent";
    case ChapterProbePhase::Complete:return "complete";
    case ChapterProbePhase::Failed:return "failed";
    }
    return "invalid";
}
// Validation input producer only. It observes immutable snapshots and emits
// ordinary axes/context edges; it cannot seed progression, readiness or poses.
// Native callers must render each advanced snapshot and acknowledge the fresh
// raise only through the existing successful-presentation authority.
class ChapterRoundTripProbe {
public:
    bool Start(const simulation::SimulationSnapshot& s,bool regionCycle=false) {
        *this={};
        for(unsigned zone=1;zone<4;++zone)
            if(s.worldRoute.readiness[zone]!=simulation::ZoneReadiness::Ready)return false;
        if(!s.developmentRescueJourney || !s.rescue.exteriorSide || s.rescue.equipmentStowed ||
           !s.rescue.firstAscentCompleted || !s.rescue.ropeReady || s.rescue.claimCount!=1 ||
           s.rescue.deploymentCount!=1 || !s.worldRoute.generation || !Finite(s) ||
           s.interaction.heldLightKind!=interactions::HeldLightKind::RewardLantern ||
           s.interaction.heldLightPose!=interactions::HeldLightPose::Low ||
           s.lich.deathDissolveProgress!=1 || s.chapterDialogue.proofAccepted ||
           s.lastConsumedInteractSequence==UINT64_MAX ||
           s.lastConsumedToggleHeldLightPoseSequence==UINT64_MAX ||
           s.lastConsumedClearRunIntentSequence==UINT64_MAX) return false;
        generation_=s.worldRoute.generation;lastTick_=phaseTick_=s.tickIndex;
        interact_=s.lastConsumedInteractSequence;raise_=s.lastConsumedToggleHeldLightPoseSequence;
        clearRun_=s.lastConsumedClearRunIntentSequence+1;
        // The focused residency test starts at the same genuine ready reunion,
        // then walks straight back to the rope. Full chapter coverage is separate.
        phase_=regionCycle?ChapterProbePhase::Return:ChapterProbePhase::Landing;station_=2;return true;
    }
    simulation::InputSnapshot Next(const simulation::SimulationSnapshot& s,bool paused=false,unsigned inputTicks=1) {
        inputTicks_=std::clamp(inputTicks,1u,8u);
        simulation::InputSnapshot input;
        input.damageEnabled=false;input.tutorialEnabled=false;
        input.yawRadians=s.playerYawRadians;input.pitchRadians=-.15f;
        for(unsigned zone=1;zone<4;++zone)
            if(s.worldRoute.readiness[zone]==simulation::ZoneReadiness::Failed)phase_=ChapterProbePhase::Failed;
        if(phase_==ChapterProbePhase::Failed || !Finite(s) || !s.developmentRescueJourney ||
           s.worldRoute.generation!=generation_ || s.tickIndex<lastTick_ ||
           s.rescue.claimCount!=1 || s.rescue.deploymentCount!=1 ||
           s.tickIndex-phaseTick_>3200) {phase_=ChapterProbePhase::Failed;input.paused=true;return input;}
        lastTick_=s.tickIndex;
        // Pausing emits no fresh edges and cannot advance a probe phase. The
        // platform's pause barrier remains the owner of real input cancellation.
        if(paused || phase_==ChapterProbePhase::Complete) {input.paused=true;return input;}
        input.commands.interact=interact_;input.commands.toggleHeldLightPose=raise_;
        input.commands.clearRunIntent=clearRun_;
        using namespace traversal;
        switch(phase_) {
        case ChapterProbePhase::Landing:
            if(!Walk(s,input,simulation::kWorldRoutePoints[2]))break;
            if(Sweep(s,input,3.14159265359f,240))SetPhase(ChapterProbePhase::Raise,s,3);
            break;
        case ChapterProbePhase::Raise:
            if(!Walk(s,input,simulation::kWorldRoutePoints[3]))break;
            input.yawRadians=std::atan2(s.chapterCompanion.x-s.playerX,s.playerZ-s.chapterCompanion.z);
            input.pitchRadians=-.10f;
            if(!raiseIssued_) {++raise_;input.commands.toggleHeldLightPose=raise_;raiseIssued_=true;}
            // Hold the actual reunion view for one simulated second after its
            // accepted presentation, so the available walking clip is inspected
            // before the running probe can outpace the companion.
            if(s.chapterDialogue.proofAccepted) {
                if(!proofTick_)proofTick_=s.tickIndex;
                if(s.tickIndex-proofTick_>=60)SetPhase(ChapterProbePhase::Outbound,s,4);
            }
            break;
        case ChapterProbePhase::Outbound:
            if(Walk(s,input,simulation::kWorldRoutePoints[station_])) {
                if(station_==7)SetPhase(ChapterProbePhase::Lookout,s,7);
                else SetPhase(phase_,s,station_+1);
            }
            break;
        case ChapterProbePhase::Lookout:
            if(Sweep(s,input,2.51f,240))SetPhase(ChapterProbePhase::Return,s,6);
            break;
        case ChapterProbePhase::Return:
            if(Walk(s,input,simulation::kWorldRoutePoints[station_])) {
                if(station_==2)SetPhase(ChapterProbePhase::Descent,s,2);
                else SetPhase(phase_,s,station_-1);
            }
            break;
        case ChapterProbePhase::Descent:
            input.yawRadians=3.14159265359f;
            if(!interactionIssued_ && s.rescuePrompt==RescuePrompt::Descend) {
                ++interact_;input.commands.interact=interact_;interactionIssued_=true;
            }
            if(interactionIssued_ && !s.rescue.exteriorSide && !s.rescue.equipmentStowed && s.rescue.sawLanding)
                SetPhase(ChapterProbePhase::Lower,s,0);
            break;
        case ChapterProbePhase::Lower:
            if(Sweep(s,input,3.14159265359f,120)) {
                lowerReturn_={s.playerX,s.playerZ,s.playerSupportWorldY};
                SetPhase(ChapterProbePhase::TombReturn,s,0);
            }
            break;
        case ChapterProbePhase::TombReturn:
            if(Walk(s,input,{-22.f,-15.2f,kRouteFloorWorldY}))SetPhase(ChapterProbePhase::Passage,s,0);
            break;
        case ChapterProbePhase::Passage:
            if(Sweep(s,input,1.57079632679f,360))SetPhase(ChapterProbePhase::RopeReturn,s,0);
            break;
        case ChapterProbePhase::RopeReturn:
            if(Walk(s,input,lowerReturn_))SetPhase(ChapterProbePhase::Ascent,s,2);
            break;
        case ChapterProbePhase::Ascent:
            input.yawRadians=3.14159265359f;
            if(!interactionIssued_ && s.rescuePrompt==RescuePrompt::Climb) {
                ++interact_;input.commands.interact=interact_;interactionIssued_=true;
            }
            if(interactionIssued_ && s.rescue.exteriorSide && !s.rescue.equipmentStowed &&
               s.rescue.sawDescent && s.rescue.sawLanding)SetPhase(ChapterProbePhase::Complete,s,2);
            break;
        default:break;
        }
        return input;
    }
    ChapterProbePhase Phase() const { return phase_; }
    unsigned Station() const { return station_; }
    unsigned SweepQuarter() const {
        return sweepStarted_?static_cast<unsigned>(std::min<std::uint64_t>(4,(lastTick_-sweepTick_)*4/sweepTicks_)):0;
    }
private:
    static bool Finite(const simulation::SimulationSnapshot& s) {
        return std::isfinite(s.playerX)&&std::isfinite(s.playerZ)&&
            std::isfinite(s.playerSupportWorldY)&&std::isfinite(s.playerYawRadians);
    }
    void SetPhase(ChapterProbePhase phase,const simulation::SimulationSnapshot& s,unsigned station) {
        phase_=phase;station_=station;phaseTick_=s.tickIndex;sweepStarted_=false;interactionIssued_=false;
    }
    bool Walk(const simulation::SimulationSnapshot& s,simulation::InputSnapshot& input,
              const simulation::WorldRoutePoint destination) const {
        if(s.rescue.equipmentStowed)return false;
        const float dx=destination.x-s.playerX,dz=destination.z-s.playerZ,distance=std::hypot(dx,dz);
        if(distance<.025f)return true;
        input.yawRadians=std::atan2(dx,-dz);input.moveForward=std::min(1.f,distance/(.055f*inputTicks_));
        input.runHeld=distance>std::max(.3f,.11f*inputTicks_);return false;
    }
    bool Sweep(const simulation::SimulationSnapshot& s,simulation::InputSnapshot& input,
               float yaw,unsigned ticks) {
        if(!sweepStarted_) {sweepTick_=s.tickIndex;sweepTicks_=ticks;sweepStarted_=true;}
        const auto elapsed=std::min<std::uint64_t>(ticks,s.tickIndex-sweepTick_);
        input.yawRadians=yaw+6.28318530718f*static_cast<float>(elapsed)/ticks;
        return elapsed==ticks;
    }
    ChapterProbePhase phase_=ChapterProbePhase::Failed;
    unsigned station_=0;
    unsigned inputTicks_=1;
    unsigned sweepTicks_=1;
    std::uint64_t generation_=0,lastTick_=0,phaseTick_=0,sweepTick_=0;
    std::uint64_t interact_=0,raise_=0,clearRun_=0;
    std::uint64_t proofTick_=0;
    bool sweepStarted_=false,raiseIssued_=false,interactionIssued_=false;
    simulation::WorldRoutePoint lowerReturn_{};
};
}
