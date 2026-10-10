#include "gameplay/effects/WaterContact.h"
#include "gameplay/simulation/GameSimulation.h"
#include <iostream>
#include <cmath>
using namespace horde::gameplay::effects;
// Independent ray/triangle intersection witness for visible water contact.
// A downward ray chooses the nearest face, including the overlapping join.
float DownwardMeshHit(float x,float z,const std::array<WaterTriangle,28>& surfaces) {
    const auto subtract=[](WaterPoint a,WaterPoint b) {return WaterPoint{a[0]-b[0],a[1]-b[1],a[2]-b[2]};};
    const auto cross=[](WaterPoint a,WaterPoint b) {return WaterPoint{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};};
    const auto dot=[](WaterPoint a,WaterPoint b) {return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];};
    const WaterPoint origin{x,1,z}, direction{0,-1,0};
    float nearest=std::numeric_limits<float>::infinity();
    for(const auto& t:surfaces) {
        const auto e1=subtract(t[1],t[0]),e2=subtract(t[2],t[0]);
        const auto p=cross(direction,e2);const float determinant=dot(e1,p);
        if(std::abs(determinant)<1e-8f) continue;
        const float inverse=1/determinant;const auto offset=subtract(origin,t[0]);
        const float u=dot(offset,p)*inverse;const auto q=cross(offset,e1);
        const float v=dot(direction,q)*inverse,distance=dot(e2,q)*inverse;
        if(u>=-1e-6f && v>=-1e-6f && u+v<=1.000001f && distance>=0)
            nearest=std::min(nearest,distance);
    }
    return 1-nearest;
}
int main() {
    int failures=0,count=0;
    auto check=[&](bool c,const char* m) {++count;if(!c){++failures;std::cerr<<m<<'\n';}};
    const auto surfaces=WaterSurfaceTriangles();
    check(surfaces.size()==28,"rendered and contacted pool/runnel share the authored 28 triangles");
    for(const auto& triangle:surfaces) {
        const float x=(triangle[0][0]+triangle[1][0]+triangle[2][0])/3.f;
        const float z=(triangle[0][2]+triangle[1][2]+triangle[2][2])/3.f;
        float surfaceY=0;
        check(WaterSurfaceAt(x,z,surfaceY)&&
              std::abs(surfaceY-DownwardMeshHit(x,z,surfaces))<1e-5f,
              "every rendered water triangle centroid contacts its nearest actual downward-ray surface");
    }
    float y=0;check(WaterSurfaceAt(-2.32f,-15.26f,y)&&std::abs(y+.925f)<1e-6f,"real catchment membership");
    const auto& join=surfaces[16];
    const float joinX=(join[0][0]+join[1][0]+join[2][0])/3,
                joinZ=(join[0][2]+join[1][2]+join[2][2])/3;
    check(WaterSurfaceAt(joinX,joinZ,y) && y>-.925f &&
          std::abs(y-DownwardMeshHit(joinX,joinZ,surfaces))<1e-6f,
          "overlapping runoff selects the real higher face rather than the earlier catchment face");
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
