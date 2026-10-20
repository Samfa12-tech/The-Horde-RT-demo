#include "gameplay/dialogue/CompanionRouteFixture.h"
#include <iostream>
#include <limits>

using namespace horde::gameplay::dialogue;
namespace {
int checks=0,failures=0;
void Check(bool value,const char* message) {++checks;if(!value){++failures;std::cerr<<"FAIL: "<<message<<'\n';}}
bool Neutral(const CompanionActingPose& p) {return p==CompanionActingPose{};}
bool Bounded(const CompanionActingPose& p) {
    return std::isfinite(p.headPitch) && std::isfinite(p.headYaw) && std::isfinite(p.chestPitch) &&
        std::abs(p.headPitch)<=.176f && std::abs(p.headYaw)<=.026f && std::abs(p.chestPitch)<=.048f;
}
}
int main() {
    constexpr float dt=1.f/60;
    for(const auto& spec:kLines) {
        CompanionActing actor;Snapshot speech;speech.line=spec.id;speech.duration=spec.duration;speech.proofAccepted=true;
        bool moved=false,recognition=false;
        for(int i=0;i<35;++i) {
            speech.elapsed=i*dt;actor.Step(dt,speech,true,true,true,false,false);
            moved|=!Neutral(actor.State());recognition|=actor.State().recognitionActive;
            if(!Bounded(actor.State())) {Check(false,"all catalogue speech poses stay finite and restrained");break;}
        }
        Check(moved==(spec.id>=Line::ReunionQuestion),"only visible conversational Kit lines animate the upper body");
        Check(!recognition,"accepted story or restored active proof speech alone cannot replay recognition");
    }
    for(unsigned invalid=0;invalid<4;++invalid) {
        CompanionActing actor;Snapshot s;s.proofAccepted=invalid!=0;
        actor.Step(dt,s,invalid!=1,invalid!=2,invalid!=3,false,true);
        Check(Neutral(actor.State()),"recognition requires accepted live proof and an exterior alive grounded body");
    }
    CompanionActing actor;Snapshot speech;speech.proofAccepted=true;
    actor.Step(dt,speech,true,true,true,false,true);
    Check(actor.State().recognitionActive,"one verified fresh proof transaction begins recognition");
    float peak=actor.State().headPitch;
    for(int i=1;i<36;++i) {
        actor.Step(dt,speech,true,true,true,false,false);peak=std::max(peak,actor.State().headPitch);
        Check(Bounded(actor.State()),"recognition stays within small upper-body bounds");
    }
    Check(peak>.12f && actor.State().chestPitch>.02f,"fresh proof produces readable head and chest recognition motion");
    const auto paused=actor.State();speech.paused=true;
    for(int i=0;i<120;++i)actor.Step(.05f,speech,true,true,true,false,false);
    Check(actor.State()==paused,"pause freezes the exact pose and recognition clock");speech.paused=false;
    for(float bad:{0.f,-1.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
        actor.Step(bad,speech,true,true,true,false,false);
        Check(actor.State()==paused,"zero or invalid elapsed time cannot advance or poison a pose");
    }
    for(int i=0;i<150;++i)actor.Step(dt,speech,true,true,true,false,false);
    Check(Neutral(actor.State()),"one-shot recognition finishes and returns to idle without persistent motion");
    for(int i=0;i<120;++i)actor.Step(dt,speech,true,true,true,false,false);
    Check(Neutral(actor.State()),"retained accepted proof never loops the recognition beat");

    speech.line=Line::ReunionQuestion;speech.duration=Spec(speech.line).duration;
    for(int i=0;i<40;++i){speech.elapsed=i*dt;actor.Step(dt,speech,true,true,true,false,false);}
    Check(!Neutral(actor.State()),"ordinary active speech has a restrained conversational pose");
    speech.line=Line::None;
    auto previous=actor.State();
    for(int i=0;i<60;++i) {
        actor.Step(dt,speech,true,true,true,false,false);
        Check(std::abs(actor.State().headPitch-previous.headPitch)<=.65f*dt+1e-6f &&
            std::abs(actor.State().headYaw-previous.headYaw)<=.35f*dt+1e-6f &&
            std::abs(actor.State().chestPitch-previous.chestPitch)<=.25f*dt+1e-6f,
            "speech skip/interruption blends back without angular snapping");previous=actor.State();
    }
    Check(Neutral(actor.State()),"interrupted speech returns cleanly to idle");
    actor.Step(dt,speech,true,true,true,false,true);actor.Reset();
    Check(Neutral(actor.State()),"reset cancels transient recognition and pose");

    CompanionActing walking;speech.line=Line::ForestVillage;speech.duration=Spec(speech.line).duration;
    for(int i=0;i<50;++i){speech.elapsed=i*dt;walking.Step(dt,speech,true,true,true,true,false);}
    Check(walking.State().chestPitch==0 && std::abs(walking.State().headPitch)<=.015f &&
        std::abs(walking.State().headYaw)<=.011f,"walking talk retains the route clip with smaller head-only motion");
    speech.elapsed=speech.duration=std::numeric_limits<float>::max();
    walking.Step(dt,speech,true,true,true,false,false);
    Check(Bounded(walking.State()),"finite extreme speech metadata cannot produce nonfinite rotations");

    CompanionRouteFixture route;const auto start=route.State();
    route.Step(.05f,start.x,start.z,true,true,false);
    speech={};speech.proofAccepted=true;
    route.AdvanceActing(.05f,speech,true,true,true);
    Check(route.State().acting.recognitionActive,"route publishes the transient acting state in its immutable snapshot");
    const auto placement=route.State();route.AdvanceActing(.05f,speech,true,true,false);
    Check(route.State().x==placement.x && route.State().y==placement.y && route.State().z==placement.z &&
        route.State().waypoint==placement.waypoint && route.State().yaw==placement.yaw,
        "upper-body acting cannot move the simulation root or alter route progress");
    route.RestoreCampaignReunion(false,0,0,nullptr,route.State().generation);
    Check(Neutral(route.State().acting),"campaign reunion restore clears transient acting");
    route.RestoreCampaignLookout(0,0,nullptr,route.State().generation);
    Check(Neutral(route.State().acting),"completed lookout restore cannot replay a recognition beat");
    route.Reset();Check(Neutral(route.State().acting),"route reset clears all acting state");
    std::cout<<checks<<" companion acting checks, "<<failures<<" failures\n";return failures?1:0;
}
