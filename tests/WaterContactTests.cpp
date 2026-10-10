#include "gameplay/effects/WaterContact.h"
#include "gameplay/simulation/GameSimulation.h"
#include <iostream>
#include <cmath>
using namespace horde::gameplay::effects;
int main() {
    int failures=0,count=0;
    auto check=[&](bool c,const char* m) {++count;if(!c){++failures;std::cerr<<m<<'\n';}};
    float y=0;check(WaterSurfaceAt(-2.32f,-15.26f,y)&&std::abs(y+.925f)<1e-6f,"real catchment membership");
    check(WaterSurfaceAt(-4.6f,-15.2f,y),"real narrow runnel membership");
    check(!WaterSurfaceAt(-4.6f,-14.95f,y),"damp dry stone excluded");
    check(!BodyTouchesWaterfall(-2.32f,-.95f,-14.7f,.25f),"narrow transformed stream rejects broad rectangle");
    check(BodyTouchesWaterfall(-2.32f,-.95f,-14.7f,2.f),"wide transformed side stream touches body");
    WaterContact contact;
    auto result=contact.Step(1.f/60,-4.6f,-.95f,-15.2f,true,true,1.9f,1);
    check(result.wetStep&&contact.Snapshot().stepCount==1,"resolved wet step emits once");
    for(int i=0;i<60;++i) contact.Step(1.f/60,-4.6f,-.95f,-15.2f,true,false,0,1);
    check(contact.Snapshot().stepCount==1,"stationary puddle emits no repeating steps");
    result=contact.Step(1.f/60,-2.32f,-.95f,-15.26f,true,false,0,1);
    check(result.streamEntry&&contact.Snapshot().entryCount==1,"actual stream entry emits burst");
    for(int i=0;i<60;++i) contact.Step(1.f/60,-2.32f,-.95f,-15.26f,true,false,0,1);
    check(contact.Snapshot().entryCount==1&&contact.Snapshot().sprayCount>0,"stationary stream has bounded continuing spray");
    const auto before=contact.Snapshot();
    contact.Step(1.f/60,-2.32f,-.95f,-15.26f,true,true,1,1,true);
    check(contact.Snapshot().stepCount==before.stepCount&&contact.Snapshot().sprayCount==before.sprayCount,"pause stops emission and age");
    contact.Step(1.f/60,-1,-.95f,-15,true,false,0,1);
    contact.Step(1.f/60,-2.32f,-.95f,-15.26f,true,false,0,1);
    check(contact.Snapshot().entryCount==2,"repeated stream entry independent from one-shot torch");
    for(int i=0;i<180;++i)contact.Step(1.f/60,0,-.95f,0,true,false,0,1);
    for(const auto& d:contact.Snapshot().droplets)check(!d.active,"finite droplet lifetime outside stream");
    contact.Reset();check(contact.Snapshot().entryCount==0&&!contact.Snapshot().inStream,"reset cancels transient water state");
    for(int i=0;i<30;++i) contact.Step(1.f/60,-2.32f,-.95f,-15.26f,true,true,5,1);
    check(contact.Snapshot().droppedEmissions>0,"pool exhaustion drops newest emission without unbounded growth");
    for(const auto& d:contact.Snapshot().droplets) {
        check(!d.active||(std::isfinite(d.position[1])&&d.lifetime<=.65f),"active droplets retain finite gravity/lifetime bounds");
    }
    contact.Reset();contact.Step(1.f/60,-2.32f,-.95f,-15.26f,true,false,0,1);
    const auto dropBefore=contact.Snapshot().droplets[0];
    contact.Step(1.f/60,0,-.95f,0,true,false,0,1);
    check(contact.Snapshot().droplets[0].velocity[1]<dropBefore.velocity[1],"actual drop velocity is pulled down by gravity");
    for(const auto& ripple:contact.Snapshot().ripples) if(ripple.active)
        check(WaterSurfaceAt(ripple.position[0],ripple.position[2],y),"disturbances are clipped to real water membership");

    using namespace horde::gameplay::simulation;
    auto config=ProductionGameSimulationConfig();
    config.playerStartX=-4.70f;config.playerStartZ=-15.20f;
    GameSimulation sim(config);InputSnapshot input{};
    input.tutorialEnabled=false;input.damageEnabled=false;
    int wetEvents=0;
    for(int tick=0;tick<120;++tick) {
        input.moveStrafe=(tick/15)%2==0 ? 1.f:-1.f;
        sim.StepFixed(input);
        for(const auto& event:sim.Events().Events()) if(event.type==GameplayEventType::PlayerWetFootstep) {
            ++wetEvents;
            check(WaterSurfaceAt(event.worldX,event.worldZ,y),"wet event source comes from resolved contact inside the water mesh");
            check(std::abs(event.worldY+.95f)<1e-5f&&std::abs(event.listenerY-.70f)<.08f,
                  "wet event freezes physical source and listener eye heights");
        }
        sim.ClearEvents();
    }
    check(wetEvents>0&&sim.Snapshot().waterContact.stepCount==static_cast<unsigned>(wetEvents),
          "real fixed-step movement consumes cadence into ordered wet events and immutable snapshot");
    const auto stepsBefore=sim.Snapshot().waterContact.stepCount;
    input.moveStrafe=0;
    for(int tick=0;tick<60;++tick)sim.StepFixed(input);
    check(sim.Snapshot().waterContact.stepCount==stepsBefore,"stationary simulation does not invent contact steps");
    input.paused=true;sim.AdvanceFrame(input,1);
    check(sim.Snapshot().waterContact.stepCount==0&&!sim.Snapshot().waterContact.inStream,
          "pause cancels water transients through the shared lifecycle path");
    sim.RetryEncounter();check(sim.Snapshot().waterContact.entryCount==0,"retry removes stale water state");
    sim.ResetRoute();check(sim.Snapshot().waterContact.stepCount==0,"reset removes stale water state");
    sim.ApplyShowcaseCheckpoint(2);check(sim.Snapshot().waterContact.entryCount==0,"existing checkpoint import clears transient water");
    auto streamConfig=ProductionGameSimulationConfig();
    streamConfig.playerStartX=-2.32f;streamConfig.playerStartZ=-15.26f;
    GameSimulation crossing(streamConfig);InputSnapshot crossingInput{};
    crossingInput.tutorialEnabled=false;crossingInput.damageEnabled=false;
    int extinguishEvents=0;
    for(int tick=0;tick<480;++tick) {
        crossingInput.moveStrafe = tick < 120 ? 0.f : ((tick/60)%2==0 ? 1.f : -1.f);
        crossing.StepFixed(crossingInput);
        for(const auto& event:crossing.Events().Events())
            if(event.type==GameplayEventType::TorchExtinguished) ++extinguishEvents;
        crossing.ClearEvents();
    }
    check(crossing.Snapshot().waterContact.entryCount > 1,
          "actual movement repeatedly exits and re-enters transformed falling stream");
    check(extinguishEvents==1 && crossing.Snapshot().torchFailure.triggered &&
          !crossing.Snapshot().torchFailure.heldByPlayer,
          "repeated wet contact never duplicates the original one-shot torch gutter and drop");
    std::cout<<count<<" water checks, "<<failures<<" failures\n";return failures?1:0;
}
