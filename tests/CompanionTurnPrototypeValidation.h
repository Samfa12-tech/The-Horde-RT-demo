#pragma once
#include "gameplay/dialogue/CompanionRouteFixture.h"
#include "gameplay/simulation/GameSimulation.h"
#include "scene/CompanionPose.h"
#include <array>
#include <cmath>
#include <limits>

namespace companion_turn_validation {
using namespace horde::gameplay;
using namespace horde::gameplay::dialogue;
using Terrain=simulation::TriangleSupportSurface;
constexpr float dt=1.f/60.f,pi=3.14159265359f;
inline float Error(float a,float b) {return std::abs(std::remainder(a-b,2*pi));}
inline bool SameFrozenPose(const CompanionSnapshot& a,const CompanionSnapshot& b) {
    // Lifecycle/readiness may change wait, walking and grounded flags, but may
    // not advance the retained pose, motor, phase or generation while frozen.
    return a.x==b.x && a.y==b.y && a.z==b.z && a.yaw==b.yaw &&
        a.animationSeconds==b.animationSeconds && a.waypoint==b.waypoint &&
        a.generation==b.generation && a.motion==b.motion &&
        a.locomotion==b.locomotion && a.acting==b.acting;
}
inline float TravelBearing(const CompanionSnapshot& s) {
    const auto p=simulation::kWorldRoutePoints[s.waypoint];
    return std::atan2(p.x+1-s.x,s.z-p.z);
}

template<class Check> void Validate(const std::shared_ptr<const Terrain>& terrain,Check check) {
    if(!terrain) {check(false,"prototype tests require actual triangle support");return;}
    CompanionRouteFixture legacy,prototype(true);
    for(unsigned tick=0;tick<180;++tick) {
        const auto a=legacy.State(),b=prototype.State();
        legacy.Step(dt,a.x-7,a.z,true,true,false,terrain.get(),true);
        prototype.Step(dt,b.x-7,b.z,true,true,false,terrain.get(),true);
    }
    const auto beforeLegacy=legacy.State(),beforePrototype=prototype.State();
    const float bearing=TravelBearing(beforeLegacy);
    legacy.Step(dt,beforeLegacy.x-3,beforeLegacy.z,true,true,false,terrain.get(),true);
    const float legacyError=Error(legacy.State().yaw,bearing)*180/pi;
    check(!beforeLegacy.motion.diagnosticEnabled && legacy.State().walking && legacyError>150 && legacyError<156,
        "default path retains the roughly 153-degree first-moving-tick departure witness");
    bool moved=false,planted=true,aligned=true,ramped=true;
    unsigned departureTicks=0;
    for(;departureTicks<400 && !moved;++departureTicks) {
        const auto old=prototype.State();
        prototype.Step(dt,old.x-3,old.z,true,true,false,terrain.get(),true);
        const auto now=prototype.State();const float distance=std::hypot(now.x-old.x,now.z-old.z);
        if(distance>0) {
            moved=true;aligned=Error(now.yaw,TravelBearing(old))<=pi/180+1e-5f;
            ramped=distance<1.8f*dt && now.motion.speed>0 && now.motion.speed<=9.f*dt+1e-5f;
        } else planted&=now.x==beforePrototype.x && now.z==beforePrototype.z && !now.walking;
    }
    check(beforePrototype.motion.diagnosticEnabled && moved && planted && aligned && ramped,
        "prototype completes supported planted cycles before an aligned, ramped first departure");

    // A changed player-facing intent must not reverse a foot already in flight.
    // Both signed 20-degree partials finish at their original endpoint first.
    CompanionRouteFixture turningSeed(true);
    for(const float sign:{-1.f,1.f}) {
        CompanionRouteFixture turn(true);const auto root=turn.State();
        const float first=sign*pi/9,opposite=-sign*pi/9;
        bool active=false;
        for(unsigned tick=0;tick<90 && !active;++tick) {
            turn.Step(dt,root.x+3*std::sin(first),root.z-3*std::cos(first),true,false,false,terrain.get(),true);
            active=turn.State().motion.mode==CompanionMotionMode::Turning && turn.State().motion.turnPhase>.2f;
        }
        check(active && std::abs(turn.State().motion.turnAngleRadians-first)<1e-5f,
            "player-facing partial turn latches its signed supported angle");
        if(!active)continue;
        turningSeed=turn;
        const auto latch=turn.State();bool completed=false,held=true;
        float previousPhase=latch.motion.turnPhase;
        for(unsigned tick=0;tick<90 && !completed;++tick) {
            turn.Step(dt,root.x+3*std::sin(opposite),root.z-3*std::cos(opposite),true,false,false,terrain.get(),true);
            const auto now=turn.State();
            held&=now.x==root.x && now.z==root.z && !now.walking && now.locomotion.walkPhase==0;
            if(now.motion.turnAngleRadians!=latch.motion.turnAngleRadians || now.motion.mode!=CompanionMotionMode::Turning) {
                completed=true;held&=Error(now.yaw,first)<1e-5f && now.motion.turnAngleRadians*first<0;
            } else {
                held&=now.motion.turnPhase>=previousPhase;
                previousPhase=now.motion.turnPhase;
            }
        }
        check(completed && held,"opposite intent waits for the latched planted endpoint before a new signed cycle");
    }

    // Freeze each phase independently so one missing-readiness publication
    // cannot accidentally reset a turn before the later pause test observes it.
    const std::array<Terrain::Triangle,2> farLand{{
        {{{{100,7,100},{100,7,110},{110,7,110}}}},
        {{{{100,7,100},{110,7,110},{110,7,100}}}}
    }};
    std::string diagnostic;const auto unsupported=Terrain::Build(farLand,diagnostic);
    check(bool(unsupported),"unsupported-current-root triangle fixture builds");
    for(const auto& seed:{turningSeed,prototype}) for(unsigned reason=0;reason<6;++reason) {
        auto paused=seed,reference=seed;const auto before=seed.State();
        paused.Step(reason==1?0:dt,before.x,before.z,reason!=3,true,reason==0,
            reason==4?nullptr:reason==5?unsupported.get():terrain.get(),true,reason!=2);
        check(SameFrozenPose(paused.State(),before),
            "pause, zero delta, inactivity, interior, missing readiness and unsupported roots freeze transient motion");
        paused.Step(dt,before.x,before.z,true,true,false,terrain.get(),true);
        reference.Step(dt,before.x,before.z,true,true,false,terrain.get(),true);
        check(SameFrozenPose(paused.State(),reference.State()),
            "resuming a frozen turn or start consumes exactly one ordinary step without phase or speed debt");
    }
    for(const bool proof:{false,true}) {
        auto waiting=prototype;const auto before=waiting.State();
        waiting.Step(dt,before.x+(proof?7.f:0.f),before.z,true,proof,false,terrain.get(),true);
        check(waiting.State().x==before.x && waiting.State().z==before.z && !waiting.State().walking &&
            waiting.State().motion.speed==0 && waiting.State().locomotion.walkPhase==before.locomotion.walkPhase &&
            waiting.State().wait==(proof?CompanionWait::PlayerLagging:CompanionWait::Reunion),
            "proof withdrawal and the existing six-metre tether stop translation without advancing distance phase");
    }

    // A real nonwalkable wall across the intended path must remain solid.
    const auto wallRoot=prototype.State();const float heading=TravelBearing(wallRoot);
    const float fx=std::sin(heading),fz=-std::cos(heading);
    const float cx=wallRoot.x+.6f*fx,cz=wallRoot.z+.6f*fz;
    const Terrain::Point a{cx-fz*2,7,cz+fx*2},b{cx+fz*2,7,cz-fx*2},
        c{b[0],9,b[2]},d{a[0],9,a[2]};
    const std::array<Terrain::Triangle,2> wall{{{{a,b,c}},{{a,c,d}}}};
    const auto blocked=Terrain::WithNonWalkableGeometry(*terrain,wall,diagnostic);
    check(bool(blocked),"prototype wall fixture attaches through the actual terrain collision lane");
    if(blocked) {
        auto collision=prototype;bool sawObstacle=false,contained=true,noRejectedPhase=true;
        for(unsigned tick=0;tick<160;++tick) {
            const auto old=collision.State();collision.Step(dt,old.x,old.z,true,true,false,blocked.get(),true);
            const auto now=collision.State();
            contained&=(now.x-wallRoot.x)*fx+(now.z-wallRoot.z)*fz<.6f;
            if(now.wait==CompanionWait::Obstacle) {
                sawObstacle=true;noRejectedPhase&=now.x==old.x && now.z==old.z && now.motion.speed==0 &&
                    now.locomotion.walkPhase==old.locomotion.walkPhase;
            }
        }
        check(sawObstacle && contained && noRejectedPhase,
            "blocked motor never crosses a solid wall or charges gait phase for rejected travel");
    }

    CompanionRouteFixture fullRoute(true);unsigned routeTicks=0,arrivals=0;
    bool validRoute=true,arrivalBookkeeping=true,sawStarting=false,sawWalking=false;
    for(;routeTicks<5000 && fullRoute.State().waypoint<simulation::kWorldRoutePoints.size();++routeTicks) {
        const auto old=fullRoute.State();fullRoute.Step(dt,old.x-1,old.z,true,true,false,terrain.get(),true);
        const auto now=fullRoute.State();const float distance=std::hypot(now.x-old.x,now.z-old.z);
        const float expected=std::fmod(old.locomotion.walkPhase+distance/1.56f,1.f);
        validRoute&=now.grounded && std::isfinite(now.yaw) && now.motion.diagnosticEnabled &&
            now.motion.speed>=0 && now.motion.speed<=1.8f && distance<=1.8f*dt+4e-6f &&
            std::abs(std::remainder(now.locomotion.walkPhase-expected,1.f))<2e-5f;
        sawStarting|=now.motion.mode==CompanionMotionMode::Starting;
        sawWalking|=now.motion.mode==CompanionMotionMode::Walking;
        if(now.motion.mode==CompanionMotionMode::Turning)validRoute&=distance==0 &&
            std::abs(now.motion.turnAngleRadians)<=pi/4+1e-6f;
        if(now.waypoint!=old.waypoint) {
            ++arrivals;arrivalBookkeeping&=now.waypoint==old.waypoint+1 && distance==0 &&
                now.yaw==old.yaw && now.locomotion.walkPhase==old.locomotion.walkPhase;
        }
    }
    const auto endpoint=simulation::kWorldRoutePoints.back();
    check(routeTicks<5000 && arrivals==4 && validRoute && arrivalBookkeeping && sawStarting && sawWalking &&
        std::hypot(fullRoute.State().x-(endpoint.x+1),fullRoute.State().z-endpoint.z)<.06f,
        "bounded prototype reaches every authored corner and lookout with accepted-distance gait and unchanged arrival bookkeeping");

    const auto generation=turningSeed.State().generation;
    turningSeed.Reset();const CompanionRouteFixture fresh(true);
    check(turningSeed.State().motion==fresh.State().motion && turningSeed.State().locomotion==fresh.State().locomotion &&
        turningSeed.State().animationSeconds==0 && turningSeed.State().generation>generation,
        "reset preserves explicit diagnostic gating and clears all transient turn/start clocks");
    turningSeed.RestoreCampaignReunion(true,fresh.State().x,fresh.State().z,terrain.get(),generation+10);
    check(turningSeed.State().motion.diagnosticEnabled && turningSeed.State().motion.mode==CompanionMotionMode::Idle &&
        turningSeed.State().motion.turnPhase==0 && turningSeed.State().motion.speed==0 &&
        turningSeed.State().locomotion==CompanionLocomotionPose{} && turningSeed.State().generation>generation+10,
        "canonical reunion restore clears motion without activating a saved turn or losing the explicit gate");
    turningSeed.RestoreCampaignLookout(endpoint.x,endpoint.z,terrain.get(),generation+20);
    check(turningSeed.State().grounded && turningSeed.State().motion==fresh.State().motion &&
        turningSeed.State().locomotion==CompanionLocomotionPose{} && turningSeed.State().animationSeconds==0,
        "canonical lookout restore starts neutral on real support with the gate retained");

    // Real simulation reset/restore must carry configuration rather than derive
    // the diagnostic choice from any checkpoint field. Flat triangles suffice
    // here; imported-terrain restore admission remains the checkpoint suite.
    check(!simulation::ProductionGameSimulationConfig().diagnosticCompanionTurnPrototype,
        "production simulation configuration keeps the turn prototype disabled");
    for(const bool enabled:{false,true}) {
        auto config=simulation::ProductionGameSimulationConfig();config.diagnosticCompanionTurnPrototype=enabled;
        simulation::GameSimulation sim(config);const auto initial=sim.Snapshot().chapterCompanion;
        campaign::Checkpoint record;record.contentRevision=2;record.id=campaign::CheckpointId::ForestEndpoint;
        record.zone=simulation::WorldZoneId::Lookout;record.position={endpoint.x,7,endpoint.z};
        record.firstAscentCompleted=true;record.dialogue.grateConsumed=record.dialogue.rescueConsumed=true;
        record.dialogue.reunionStarted=record.dialogue.proofAccepted=true;
        record.dialogue.forestNightConsumed=record.dialogue.forestWaystoneConsumed=true;
        record.dialogue.forestClueConsumed=record.dialogue.forestVillageConsumed=true;
        const auto token=sim.BeginCampaignRestore(record);simulation::InputSnapshot input;input.paused=true;
        const bool loaded=token && sim.CommitCampaignRestore(*token,{token->worldGeneration,record.zone},terrain,input);
        check(initial.motion.diagnosticEnabled==enabled && loaded && sim.Snapshot().chapterCompanion.grounded &&
            sim.Snapshot().chapterCompanion.motion.diagnosticEnabled==enabled &&
            sim.Snapshot().chapterCompanion.motion.mode==CompanionMotionMode::Idle &&
            sim.Snapshot().chapterCompanion.motion.turnPhase==0 && sim.Snapshot().chapterCompanion.motion.speed==0,
            "real simulation canonical restore retains only the constructor gate and resets transient motion");
        sim.ResetRoute();
        check(sim.Snapshot().chapterCompanion.motion.diagnosticEnabled==enabled &&
            sim.Snapshot().chapterCompanion.motion.turnPhase==0 && sim.Snapshot().chapterCompanion.locomotion.walkPhase==0,
            "real simulation route reset preserves production-off or explicit-on configuration");
    }

    CompanionSnapshot pose;pose.animationSeconds=1.25f;pose.locomotion={.3f,.7f};
    auto recipe=horde::scene::CompanionAnimation(pose,5.f/6);
    check(recipe.clip==horde::scene::SkinnedClip::Idle && recipe.seconds==1.25f &&
        recipe.blend.clip==horde::scene::SkinnedClip::Walking && recipe.blend.weight==.7f &&
        recipe.blend.timeSeconds==.3f*(5.f/6),"legacy render recipe remains original Idle/Walk and original clocks");
    pose.motion.diagnosticEnabled=true;pose.motion.mode=CompanionMotionMode::Turning;pose.motion.entryWeight=.5f;
    recipe=horde::scene::CompanionAnimation(pose,5.f/6);
    check(recipe.seconds==1.25f && recipe.blend.clip==horde::scene::SkinnedClip::Idle &&
        recipe.blend.timeSeconds==0 && recipe.blend.weight==.5f,"turn entry settles nonzero idle time into Idle zero first");
    pose.motion.entryWeight=1;pose.motion.turnPhase=.5f;
    for(const float angle:{-pi/9,pi/9}) {
        pose.motion.turnAngleRadians=angle;recipe=horde::scene::CompanionAnimation(pose,5.f/6);
        check(recipe.seconds==0 && recipe.blend.clip==(angle>0?horde::scene::SkinnedClip::TurnRight:horde::scene::SkinnedClip::TurnLeft) &&
            recipe.blend.timeSeconds==.375f && std::abs(recipe.blend.weight-4.f/9)<1e-6f,
            "signed logical partial turn selects matched named joint slot, phase and amplitude exactly once");
    }
    pose.motion.mode=CompanionMotionMode::Starting;recipe=horde::scene::CompanionAnimation(pose,5.f/6);
    check(recipe.seconds==0 && recipe.blend.clip==horde::scene::SkinnedClip::Walking && recipe.blend.weight==.7f,
        "starting recipe joins Idle zero to the retained actual-distance walking phase");

    // Retiming changes only the shared phase clock. Both signs and partial/full
    // exchanges must still sample the entire original take and its matched yaw.
    for(float angle:{-pi/4,-pi/9,pi/9,pi/4}) {
        CompanionRouteFixture turn(true);const auto origin=turn.State();
        const float px=origin.x+3*std::sin(angle),pz=origin.z-3*std::cos(angle);
        for(unsigned i=0;i<30 && turn.State().motion.entryWeight<1;++i)
            turn.Step(dt,px,pz,true,false,false,terrain.get(),true);
        const auto entry=turn.State();bool coherent=entry.motion.mode==CompanionMotionMode::Turning &&
            entry.motion.entryWeight==1 && entry.motion.turnPhase==0;
        unsigned activeSteps=0;
        while(activeSteps<60 && turn.State().motion.mode==CompanionMotionMode::Turning) {
            turn.Step(dt,px,pz,true,false,false,terrain.get(),true);++activeSteps;
            const auto& s=turn.State();const auto& m=s.motion;
            const float phase=std::min(activeSteps*dt/companion_turn::playbackDuration,1.f);
            coherent&=s.x==origin.x && s.z==origin.z && !s.walking && s.locomotion.walkPhase==0 &&
                std::abs(m.turnPhase-phase)<1e-6f &&
                Error(s.yaw,entry.yaw-companion_turn::ModelYaw(angle>0,phase)*
                    (std::abs(entry.motion.turnAngleRadians)/companion_turn::maxAngle))<1e-6f;
            if(m.mode==CompanionMotionMode::Turning) {
                const auto selected=horde::scene::CompanionAnimation(s,5.f/6);
                coherent&=selected.blend.clip==(angle>0?horde::scene::SkinnedClip::TurnRight:horde::scene::SkinnedClip::TurnLeft) &&
                    selected.blend.timeSeconds==m.turnPhase*companion_turn::duration;
            }
        }
        check(coherent && activeSteps==static_cast<unsigned>(std::ceil(companion_turn::playbackDuration/dt-1e-5f)) &&
            turn.State().motion.mode==CompanionMotionMode::Idle && turn.State().motion.turnPhase==1 &&
            Error(turn.State().yaw,angle)<1e-5f,
            "retimed signed full/partial exchange keeps shared joint/yaw phase, supported root and complete authored endpoint");
    }
}
} // namespace companion_turn_validation
