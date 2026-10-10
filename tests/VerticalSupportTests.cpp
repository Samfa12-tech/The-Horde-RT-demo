#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/DevelopmentCheckpointSimulation.h"
#include "vulkan/raytracing/SimulationFrameAdapter.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <cstring>
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
using namespace horde::vulkan::raytracing;
int main()
{
    int failures = 0, cases = 0;
    const auto check = [&](bool pass, const char* text) { ++cases; if (!pass) { ++failures; std::cerr << text << '\n'; } };
    const auto near = [](float a, float b) { return std::abs(a-b)<0.00001f; };
    GameSimulation baseline, proof;
    InputSnapshot input; input.damageEnabled = false; input.pitchRadians = -0.05f;
    proof.SetDevelopmentSupportFixture(true, 2);
    const auto compareFloor = [&] {
        const auto& a=baseline.Snapshot();const auto& b=proof.Snapshot();
        check(a.playerX==b.playerX && a.playerZ==b.playerZ && a.playerYawRadians==b.playerYawRadians && a.tickIndex==b.tickIndex, "floor XZ/yaw/tick ordering changed");
        check(a.playerSupportWorldY==kRouteFloorWorldY && b.playerHeightDelta==0 && b.playerGrounded && b.playerSupportId==PlayerSupportId::RouteFloor, "legacy support convention");
        check(a.heldItems[0].worldFromItem==b.heldItems[0].worldFromItem && a.heldItems[1].worldFromItem==b.heldItems[1].worldFromItem && a.heldLight.worldFromLight==b.heldLight.worldFromLight, "zero-offset actual equipment/light regression");
    };
    compareFloor();
    baseline.StepFixed(input); proof.StepFixed(input); compareFloor();
    input.moveForward=1;
    float previous=kRouteFloorWorldY;
    for(int i=0;i<59;++i) {
        baseline.StepFixed(input);proof.StepFixed(input);
        const auto& a=baseline.Snapshot();const auto& b=proof.Snapshot();
        check(a.playerX==b.playerX && a.playerZ==b.playerZ && a.playerYawRadians==b.playerYawRadians, "support alters planar movement/collision");
        check(b.playerSupportWorldY>=previous && b.playerGrounded, "ascent support not monotonic/grounded"); previous=b.playerSupportWorldY;
        const auto frame=BuildRtSceneFrameInputs(b, .92f);
        const auto gpu=BuildPlayerFrameLight({1,2,3,4},frame.playerSupportWorldY);
        check(near(gpu.playerTransform[0],b.playerHeightDelta) && gpu.positionStrength==std::array<float,4>{1,2,3,4}, "actual camera GPU packing changes physical light");
    }
    check(near(proof.Snapshot().playerSupportWorldY,kRouteFloorWorldY+kProofSupportHeight), "raised platform not reached by fixed-step walk");
    const auto raised=proof.Snapshot();
    const auto body=GroundPlayerRootOnRouteFloor({3,8,5},raised.playerSupportWorldY,.002f);
    check(near(body[1],raised.playerSupportWorldY+.002f) && body[0]==3 && body[2]==5, "actual body root grounding composes height twice");
    check(near(PlayerEyeWorldY(raised.playerSupportWorldY)-raised.playerSupportWorldY,1.65f),"absolute eye/support convention");
    input.moveForward=0;input.commands.attack=1;proof.ClearEvents();
    for(int i=0;i<8;++i)proof.StepFixed(input);
    bool emitted=false; GameplayEvent delayed;
    for(const auto& event:proof.Events().Events()) if(event.type==GameplayEventType::PlayerSwing){delayed=event;emitted=true;}
    check(emitted && near(delayed.listenerY,PlayerEyeWorldY(proof.Snapshot().playerSupportWorldY)),"emitted event did not freeze raised listener Y");
    input.moveForward=-1;previous=proof.Snapshot().playerSupportWorldY;
    for(int i=0;i<59;++i) {
        proof.StepFixed(input);check(proof.Snapshot().playerSupportWorldY<=previous,"descent support not monotonic");previous=proof.Snapshot().playerSupportWorldY;
    }
    check(near(proof.Snapshot().playerSupportWorldY,kRouteFloorWorldY),"ground return not reached");
    check(near(delayed.listenerY,kShowcaseEyeWorldY+kProofSupportHeight) && delayed.listenerX==raised.playerX && near(delayed.listenerZ,raised.playerZ),"delayed event tuple changed after descent");
    const auto* checkpoint=FindDevelopmentCheckpoint(171);
    check(checkpoint && StageDevelopmentCheckpointSimulation(proof,*checkpoint),"shared development raised checkpoint staging");
    input={};input.paused=true;const auto beforePause=proof.Snapshot();
    check(proof.AdvanceFrame(input,.25)==0 && proof.Snapshot().playerSupportWorldY==beforePause.playerSupportWorldY,"pause moved support");
    proof.SetDevelopmentSupportFixture(true, 10);
    check(proof.Snapshot().playerHeightDelta==0,"new generation retained stale raised height");
    proof.SetDevelopmentSupportFixture(true,9);check(proof.Snapshot().playerSupportGeneration==10,"stale fixture callback accepted");
    proof.AdvanceFrame(input,0);check(proof.Snapshot().playerHeightDelta==0,"zero-tick publication restored stale support");
    input.paused=false;proof.StepFixed(input);check(proof.Snapshot().playerHeightDelta>0,"new generation failed fixed-step reacquisition");
    proof.SetDevelopmentSupportFixture(false,11);check(proof.Snapshot().playerHeightDelta==0,"fixture disable leaves raised camera");
    proof.StepFixed(input);check(proof.Snapshot().playerHeightDelta==0,"disabled/default route acquired fixture");
    for(int action=0;action<3;++action) {
        StageDevelopmentCheckpointSimulation(proof,*checkpoint);
        if(action==0)proof.ResetRoute();else if(action==1)proof.RetryEncounter();else proof.ImportRewardCheckpoint({}, {}, {});
        check(proof.Snapshot().playerHeightDelta==0 && proof.Events().Empty(),"reset/retry/import retained height or delayed events");
    }
    const auto rejected=[&](float x,float z,bool enabled,std::uint64_t gen,std::uint64_t expected){const auto r=ResolveDevelopmentPlayerSupport(x,z,enabled,gen,expected);return r.worldY==kRouteFloorWorldY && r.id==PlayerSupportId::RouteFloor && r.grounded;};
    check(rejected(0,0,true,4,5),"stale support generation not rejected");
    check(rejected(0,0,true,0,0),"missing support generation not rejected");
    check(rejected(std::numeric_limits<float>::quiet_NaN(),0,true,1,1),"nonfinite support position not rejected");
    check(rejected(2,0,true,1,1),"missing footprint support not rejected");
    check(rejected(0,0,false,1,1),"disabled support not rejected");
    const auto preview=BuildEntryMenuFrameInputs({},.92f,FireEmitterQuality::High,horde::graphics::ShadowQuality::Higher);
    check(preview.playerSupportWorldY==kRouteFloorWorldY && BuildPlayerFrameLight({},preview.playerSupportWorldY).playerTransform[0]==0,"menu baseline transform");
    std::cout<<"Vertical support proof cases="<<cases<<" failures="<<failures<<" fixed-walk directions=2\n";
    return failures?1:0;
}
