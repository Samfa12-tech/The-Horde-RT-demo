#include "gameplay/animation/PlayerAnimationState.h"
#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/validation/PlayerDodgePoseFixture.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <tuple>

namespace
{
using namespace horde::gameplay;
using namespace horde::gameplay::animation;
using namespace horde::gameplay::simulation;
constexpr float kStep=1.f/60.f,kDuration=.20f,kYaw=.40f;
int failures=0,checks=0;
void Check(bool value,const char* message)
{
    ++checks;
    if(!value){++failures;std::cerr<<"FAIL: "<<message<<'\n';}
}
bool Near(float a,float b,float tolerance=.0001f){return std::abs(a-b)<=tolerance;}
bool Neutral(const PlayerDodgePresentation& pose){return pose==PlayerDodgePresentation{};}
GameSimulationConfig Chamber()
{
    auto config=ProductionGameSimulationConfig();
    config.playerStartX=-5.5f;config.playerStartZ=-13.5f;config.playerStartYawRadians=kYaw;
    config.waterfallSkeletonEncounter=false;config.swordStartsStowed=false;
    return config;
}
InputSnapshot Input()
{
    InputSnapshot input;input.damageEnabled=false;input.tutorialEnabled=false;
    input.yawRadians=kYaw;return input;
}
// Independent model-to-world planar rotation: native model +X is gameplay
// left and +Z is forward. The renderer includes the existing torso twist.
void CheckWorldDirection(const SimulationSnapshot& state,float acceptedX,float acceptedZ)
{
    const auto& pose=state.playerAnimation.dodge;
    Check(std::isfinite(pose.weight) && pose.weight>=0 && pose.weight<=1 &&
          std::isfinite(pose.modelX) && std::isfinite(pose.modelZ),"published dodge pose is finite and bounded");
    if(pose.weight<=.000001f)return;
    const float angle=state.playerYawRadians+EvaluateLowerBodyPose(state.walkTime,state.walkAmount).torsoTwistRadians;
    const float x=-pose.modelX*std::cos(angle)+pose.modelZ*std::sin(angle),
        z=-pose.modelX*std::sin(angle)-pose.modelZ*std::cos(angle);
    Check(Near(std::hypot(pose.modelX,pose.modelZ),1) && Near(x,acceptedX) && Near(z,acceptedZ),
          "published model direction reconstructs the accepted world direction through aim and torso twist");
}
bool SameEvents(const BoundedGameplayEventQueue& a,const BoundedGameplayEventQueue& b)
{
    if(a.Size()!=b.Size() || a.OverflowCount()!=b.OverflowCount())return false;
    for(std::size_t i=0;i<a.Size();++i){
        const auto& x=a[i];const auto& y=b[i];
        // Aim changes the event-time listener yaw by design; event identity,
        // gameplay source, placement, listener position and payload stay equal.
        const auto fields=[](const auto& e){return std::tie(e.sequence,e.tickIndex,e.type,e.source,e.target,
            e.worldX,e.worldY,e.worldZ,e.listenerX,e.listenerY,e.listenerZ,e.intensity,e.payload);};
        if(fields(x)!=fields(y))return false;
    }
    return true;
}

void TestEnvelopeAndAxes()
{
    constexpr float pi=3.14159265358979323846f;
    struct Axis{float forward,strafe,modelX,modelZ;};
    constexpr std::array<Axis,4> axes{{{1,0,0,1},{-1,0,0,-1},{0,-1,1,0},{0,1,-1,0}}};
    for(float yaw:{0.f,pi*.5f,pi,-pi*.5f})for(const auto& axis:axes){
        const float x=std::sin(yaw)*axis.forward+std::cos(yaw)*axis.strafe,
            z=-std::cos(yaw)*axis.forward+std::sin(yaw)*axis.strafe;
        const auto peak=EvaluatePlayerDodgePresentation(true,.10f,kDuration,x,z,yaw,0);
        Check(Near(peak.weight,1) && Near(peak.modelX,axis.modelX) && Near(peak.modelZ,axis.modelZ),
              "forward/back/left/right have correct native model axes at every cardinal aim");
        const auto scaled=EvaluatePlayerDodgePresentation(true,.10f,kDuration,x*2.5f,z*2.5f,yaw,0);
        Check(Near(scaled.modelX,peak.modelX) && Near(scaled.modelZ,peak.modelZ),
              "accepted world direction is normalized independently of input magnitude");
    }
    const auto quarter=EvaluatePlayerDodgePresentation(true,.05f,kDuration,0,-1,0,0),
        threeQuarter=EvaluatePlayerDodgePresentation(true,.15f,kDuration,0,-1,0,0);
    Check(Near(quarter.weight,.5625f) && Near(quarter.weight,threeQuarter.weight),
          "quartic presentation has symmetric onset/recovery and its specified quarter-phase value");
    for(float time:{-.01f,0.f,kDuration,.21f})Check(Neutral(EvaluatePlayerDodgePresentation(true,time,kDuration,0,-1,0,0)),
        "outside the accepted timer the entire presentation is exactly neutral");
    for(float duration:{0.f,-.20f})Check(Neutral(EvaluatePlayerDodgePresentation(true,.1f,duration,0,-1,0,0)),
        "nonpositive duration cannot produce a pose");
    Check(Neutral(EvaluatePlayerDodgePresentation(false,.1f,kDuration,0,-1,0,0)) &&
          Neutral(EvaluatePlayerDodgePresentation(true,.1f,kDuration,0,0,0,0)),
          "inactive or missing accepted direction cannot produce a pose");
    for(unsigned field=0;field<6;++field)for(float invalid:{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
        std::array<float,6> values{.1f,kDuration,0,-1,0,.055f};values[field]=invalid;
        Check(Neutral(EvaluatePlayerDodgePresentation(true,values[0],values[1],values[2],values[3],values[4],values[5])),
              "every nonfinite presentation argument rejects to exact neutral");
    }
    Check(EvaluatePlayerDodgePresentation(true,.00001f,kDuration,0,-1,0,0).weight<.000001f &&
          EvaluatePlayerDodgePresentation(true,kDuration-.00001f,kDuration,0,-1,0,0).weight<.000001f,
          "quartic envelope approaches both neutral endpoints continuously");
}

void TestFourDirectionsAndChangingAim()
{
    constexpr std::array<std::array<float,2>,4> directions{{{1,0},{-1,0},{0,-1},{0,1}}};
    for(const auto& direction:directions){
        auto fixed=std::make_unique<GameSimulation>(Chamber()),turned=std::make_unique<GameSimulation>(Chamber());
        auto fixedInput=Input(),turnedInput=Input();
        fixedInput.moveForward=turnedInput.moveForward=direction[0];
        fixedInput.moveStrafe=turnedInput.moveStrafe=direction[1];
        fixedInput.commands.dodge=turnedInput.commands.dodge=1;
        fixedInput.commands.runToggle=turnedInput.commands.runToggle=1;
        const auto start=fixed->Snapshot();
        Check(Near(start.playerX,-5.5f) && Near(start.playerZ,-13.5f),"dodge fixture uses the admitted open chamber");
        const float acceptedX=std::sin(kYaw)*direction[0]+std::cos(kYaw)*direction[1],
            acceptedZ=-std::cos(kYaw)*direction[0]+std::sin(kYaw)*direction[1];
        for(int tick=1;tick<=16;++tick){
            if(tick>1){
                fixedInput.moveForward=fixedInput.moveStrafe=turnedInput.moveForward=turnedInput.moveStrafe=0;
                turnedInput.yawRadians=kYaw+.13f*(tick-1);
            }
            fixed->ClearEvents();turned->ClearEvents();
            fixed->StepFixed(fixedInput,kStep,static_cast<std::uint64_t>(tick));
            turned->StepFixed(turnedInput,kStep,static_cast<std::uint64_t>(tick));
            const auto& a=fixed->Snapshot();const auto& b=turned->Snapshot();
            Check(Near(a.playerX,b.playerX) && Near(a.playerZ,b.playerZ) &&
                  Near(a.dodgeElapsedSeconds,b.dodgeElapsedSeconds) && a.dodgeActive==b.dodgeActive &&
                  a.acceptedDodgeSequence==1 && b.acceptedDodgeSequence==1,
                  "released stick and changing aim preserve accepted dodge travel, timer and identity");
            Check(a.dodgeInvulnerable==(tick>=2 && tick<10) && b.dodgeInvulnerable==a.dodgeInvulnerable &&
                  a.dodgeProtectedHitCount==0 && b.dodgeProtectedHitCount==0,
                  "visual dodge preserves the existing half-open protection window without contact tests");
            Check(!a.runActive && !a.runToggleActive && !b.runActive && !b.runToggleActive,
                  "dodge retains existing run cancellation");
            Check(SameEvents(fixed->Events(),turned->Events()),"aim-only pose rebasing leaves gameplay event semantics unchanged");
            CheckWorldDirection(a,acceptedX,acceptedZ);CheckWorldDirection(b,acceptedX,acceptedZ);
            if(tick<12)Check(a.playerAnimation.dodge.weight>0 && b.playerAnimation.dodge.weight>0,
                "every active interior dodge phase publishes its visible presentation");
            if(tick==6)Check(Near(a.playerAnimation.dodge.weight,1) && Near(b.playerAnimation.dodge.weight,1),
                "shared fixed-step midpoint reaches the presentation peak");
        }
        const auto& result=turned->Snapshot();
        Check(Near(result.playerX,start.playerX+.90f*acceptedX,.0002f) &&
              Near(result.playerZ,start.playerZ+.90f*acceptedZ,.0002f) && !result.dodgeActive &&
              Neutral(result.playerAnimation.dodge),"all four released-input dodges retain exactly 0.90 metre travel and neutral landing");
        const auto settled=result;
        for(int tick=0;tick<40;++tick)turned->StepFixed(turnedInput);
        Check(Near(turned->Snapshot().playerX,settled.playerX) && Near(turned->Snapshot().playerZ,settled.playerZ) &&
              Neutral(turned->Snapshot().playerAnimation.dodge) && turned->Snapshot().acceptedDodgeSequence==1,
              "repeated consumed publication cannot restart movement or presentation after cooldown");
    }
}

void TestPauseAndCrouchCancellation()
{
    for(unsigned pausePath=0;pausePath<3;++pausePath){
        auto sim=std::make_unique<GameSimulation>(Chamber());auto input=Input();
        input.moveForward=1;input.commands.dodge=1;
        for(int tick=0;tick<4;++tick){sim->StepFixed(input);input.moveForward=0;}
        Check(sim->Snapshot().playerAnimation.dodge.weight>0,"pause fixture begins inside a real accepted dodge");
        const auto before=sim->Snapshot();input.paused=true;input.commands.dodge=2;
        if(pausePath==0)sim->AdvanceFrame(input,0,20);
        if(pausePath==1)sim->StepFixed(input,0,20);
        if(pausePath==2)sim->SynchronizePausedInput(input,20,PausedInputPolicy::DiscardAllCommands);
        const auto cancelled=sim->Snapshot();
        Check(Neutral(cancelled.playerAnimation.dodge) && !cancelled.dodgeActive && !cancelled.dodgeInvulnerable &&
              cancelled.lastConsumedDodgeSequence==2 && Near(cancelled.playerX,before.playerX) && Near(cancelled.playerZ,before.playerZ),
              "frame pause, zero-delta direct pause and lifecycle barrier cancel pose/protection and consume new edges");
        input.paused=false;
        for(int tick=0;tick<40;++tick)sim->StepFixed(input);
        Check(Neutral(sim->Snapshot().playerAnimation.dodge) && !sim->Snapshot().dodgeActive &&
              Near(sim->Snapshot().playerX,cancelled.playerX) && Near(sim->Snapshot().playerZ,cancelled.playerZ),
              "resume cannot resurrect a cancelled dodge or a paused command");
        input.commands.dodge=3;sim->StepFixed(input);
        Check(sim->Snapshot().acceptedDodgeSequence==3 && sim->Snapshot().playerAnimation.dodge.weight>0,
              "a fresh post-resume edge remains available after ordinary cooldown");
    }
    auto sim=std::make_unique<GameSimulation>(Chamber());auto input=Input();
    input.crouchHeld=true;input.commands.dodge=1;sim->StepFixed(input);
    Check(sim->Snapshot().crouchActive && !sim->Snapshot().dodgeActive && Neutral(sim->Snapshot().playerAnimation.dodge) &&
          sim->Snapshot().lastConsumedDodgeSequence==1 && sim->Snapshot().acceptedDodgeSequence==0,
          "crouch rejects a simultaneous dodge without publishing or buffering its pose");
    input.crouchHeld=false;for(int tick=0;tick<40;++tick)sim->StepFixed(input);
    Check(!sim->Snapshot().crouchActive && Neutral(sim->Snapshot().playerAnimation.dodge) && !sim->Snapshot().dodgeActive,
          "standing again cannot replay the rejected crouched dodge");
    input.commands.dodge=2;sim->StepFixed(input);
    Check(sim->Snapshot().playerAnimation.dodge.weight>0,"fresh standing dodge starts normally");
    input.crouchHeld=true;sim->StepFixed(input);
    Check(!sim->Snapshot().dodgeActive && !sim->Snapshot().dodgeInvulnerable && Neutral(sim->Snapshot().playerAnimation.dodge),
          "crouch onset cancels an admitted dodge presentation immediately");
    sim->ResetRoute();
    Check(Neutral(sim->Snapshot().playerAnimation.dodge) && !sim->Snapshot().dodgeActive && !sim->Snapshot().dodgeInvulnerable,
          "route reset leaves no stale crouch/dodge presentation");
}

void TestZeroDeltaAndCaptureFreshness()
{
    auto sim=std::make_unique<GameSimulation>(Chamber());auto input=Input();
    const auto initial=sim->Snapshot();input.moveForward=1;input.commands.dodge=1;
    Check(sim->AdvanceFrame(input,0,10)==0 && sim->Snapshot().inputPublicationSequence==10 &&
          sim->Snapshot().acceptedDodgeSequence==0 && Neutral(sim->Snapshot().playerAnimation.dodge),
          "zero-tick publication cannot invent an accepted pose before the simulation consumes its edge");
    sim->StepFixed(input,0,10);
    Check(sim->Snapshot().acceptedDodgeSequence==1 && sim->Snapshot().dodgeActive &&
          sim->Snapshot().dodgeElapsedSeconds==0 && Neutral(sim->Snapshot().playerAnimation.dodge) &&
          Near(sim->Snapshot().playerX,initial.playerX) && Near(sim->Snapshot().playerZ,initial.playerZ),
          "zero-delta acceptance stays exactly at the neutral start without movement");
    input.moveForward=0;for(int tick=0;tick<4;++tick)sim->StepFixed(input,kStep,11+tick);
    const auto active=sim->Snapshot();const auto sequence=sim->Events().NextSequence();
    for(int repeat=0;repeat<3;++repeat){
        Check(sim->AdvanceFrame(input,0,14)==0 && sim->Snapshot().tickIndex==active.tickIndex &&
              sim->Snapshot().playerAnimation.dodge==active.playerAnimation.dodge &&
              sim->Snapshot().dodgeElapsedSeconds==active.dodgeElapsedSeconds && sim->Events().NextSequence()==sequence,
              "re-reading one coherent publication at zero delta preserves the exact current pose and event floor");
    }
    input.hasAuthoritativePlayerPose=true;input.authoritativePlayerX=active.playerX;input.authoritativePlayerZ=active.playerZ;
    sim->StepFixed(input,0,15);
    Check(Neutral(sim->Snapshot().playerAnimation.dodge) && !sim->Snapshot().dodgeActive && !sim->Snapshot().dodgeInvulnerable &&
          Near(sim->Snapshot().playerX,active.playerX) && Near(sim->Snapshot().playerZ,active.playerZ),
          "frozen authoritative capture publishes neutral after final pose overrides without advancing position");
    input.hasAuthoritativePlayerPose=false;sim->AdvanceFrame(input,0,15);sim->StepFixed(input,kStep,16);
    Check(Neutral(sim->Snapshot().playerAnimation.dodge) && !sim->Snapshot().dodgeActive,
          "leaving a frozen capture cannot resurrect its cancelled presentation from stale input");
    sim->ResetRoute();
    Check(Neutral(sim->Snapshot().playerAnimation.dodge),"reset publication independently clears the prior capture pose");
}
} // namespace

int main()
{
    TestEnvelopeAndAxes();TestFourDirectionsAndChangingAim();
    TestPauseAndCrouchCancellation();TestZeroDeltaAndCaptureFreshness();
    const auto frames=horde::gameplay::validation::BuildPlayerDodgePoseFixture();
    Check(frames.size()==64,"native inspection has exactly its bounded 64 authentic shared-simulation snapshots");
    unsigned peaks=0,meshes=0,paused=0;
    for(const auto& frame:frames){
        const auto& s=frame.snapshot;meshes+=frame.mesh;
        Check(s.playerAlive && !s.developmentRescueJourney && s.playerMountProfile==items::PlayerMountProfile::AnatomicalBody,
              "inspection uses noncombat shared production mount without campaign progression");
        const bool lantern=frame.name.starts_with("lantern-") && !frame.name.ends_with("-reset");
        Check((s.interaction.heldLightKind==interactions::HeldLightKind::RewardLantern)==lantern,
              "equipment follows its fixture family until Reset restores the ordinary torch");
        if(frame.name.ends_with("-6")){++peaks;Check(Near(s.playerAnimation.dodge.weight,1),"each direction/equipment capture includes its actual peak");}
        if(s.paused){++paused;Check(Neutral(s.playerAnimation.dodge),"paused native snapshot retains existing dodge cancellation");}
    }
    Check(peaks==8 && meshes==16 && paused==2,"native fixture covers every direction/equipment peak, body mesh and pause family");
    std::cout<<"Player dodge presentation: "<<checks<<" checks, "<<failures<<" failures\n";
    return failures?1:0;
}
