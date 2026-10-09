#include "gameplay/simulation/GameSimulation.h"
#include "scene/DevelopmentWorldGeometry.h"
#include "vulkan/raytracing/DevelopmentWorldSceneAdapter.h"
#include <chrono>
#include <iostream>
#include <limits>

using namespace horde::gameplay::simulation;
int main()
{
    int checks=0,failures=0;
    auto check=[&](bool result,const char* why){ ++checks;if(!result){++failures;std::cerr<<why<<'\n';} };
    const auto resident=horde::scene::PrepareDevelopmentWorldGeometry(false);
    const auto staged=horde::scene::PrepareDevelopmentWorldGeometry(true);
    check(resident.valid && staged.valid && resident.triangles.size()==staged.triangles.size(),"real mesh preparation failed");
    for(std::size_t i=0;i<resident.triangles.size();++i)
        check(resident.triangles[i].points==staged.triangles[i].points && resident.triangles[i].owner==staged.triangles[i].owner,"staged preparation drops or alters retained contributors");
    check(staged.peakPreparationCpuBytes>=resident.retainedCpuBytes,"staged overlap omitted");
    auto corrupted=resident;
    corrupted.triangles[0].points[0][1]=std::numeric_limits<float>::quiet_NaN();
    check(!horde::scene::ValidateDevelopmentWorldGeometry(corrupted),"nonfinite actual upload batch admitted");
    corrupted=resident;corrupted.triangles[0].points[1]=corrupted.triangles[0].points[0];
    check(!horde::scene::ValidateDevelopmentWorldGeometry(corrupted),"degenerate upload batch admitted");
    corrupted=resident;corrupted.triangles[0].owner=static_cast<WorldZoneId>(99);
    check(!horde::scene::ValidateDevelopmentWorldGeometry(corrupted),"unknown upload owner admitted");
    GameSimulation sim(horde::gameplay::simulation::ProductionGameSimulationConfig());
    sim.SetDevelopmentWorldRoute(true);
    InputSnapshot input;input.damageEnabled=false;input.yawRadians=3.14159265359f;input.moveForward=1;
    const auto start=sim.Snapshot();
    sim.StepFixed(input);
    check(sim.Snapshot().playerX==start.playerX && sim.Snapshot().playerZ==start.playerZ && sim.Snapshot().worldRoute.blocked,"unready crossing did not retain safe side");
    horde::vulkan::raytracing::PresentableTinyRtScene unavailableScene;
    horde::vulkan::raytracing::PublishDevelopmentWorldReadiness(sim,unavailableScene);
    check(sim.Snapshot().worldRoute.readiness[1]!=ZoneReadiness::Ready,"adapter admitted an uninitialised real renderer");
    // Host admission uses actually prepared CPU geometry. GPU completion is a
    // separate gate above and on-device; never label this as a real RT pass.
    auto admit=[&](WorldZoneId zone){return sim.PublishWorldZoneReadiness({sim.Snapshot().worldRoute.generation,zone},ZoneReadiness::Ready);};
    for(std::size_t i=1;i<kWorldZones.size();++i) check(admit(static_cast<WorldZoneId>(i)),"prepared host descriptor rejected");
    // Admit only the tomb side, then reject crossing into unprepared forest.
    sim.ResetRoute();admit(WorldZoneId::TombExterior);
    const auto token=WorldZoneToken{sim.Snapshot().worldRoute.generation,WorldZoneId::Lookout};
    const auto near=[](float a,float b){return std::abs(a-b)<.0001f;};
    const auto walkTo=[&](const WorldRoutePoint& destination){
        for(int tick=0;tick<3500;++tick) {
            const auto before=sim.Snapshot(); const float dx=destination.x-before.playerX,dz=destination.z-before.playerZ;
            if(std::hypot(dx,dz)<.025f) return true;
            input.yawRadians=std::atan2(dx,-dz);input.moveForward=std::min(1.0f,std::hypot(dx,dz)/.032f);input.moveStrafe=0;
            sim.StepFixed(input);const auto& after=sim.Snapshot();
            const auto surface=ResolveWorldRouteSupport(after.playerX,after.playerZ);
            check(surface.grounded && near(surface.worldY,after.playerSupportWorldY),"actual triangle support disagrees with fixed snapshot");
            check(std::abs(after.playerSupportWorldY-before.playerSupportWorldY)<=kWorldRouteMaximumStep+.0001f,"unbounded step accepted");
            check(near(PlayerEyeWorldY(after.playerSupportWorldY)-after.playerSupportWorldY,1.65f),"eye/support convention lost");
        }
        std::cerr<<"route blocked at x="<<sim.Snapshot().playerX<<" z="<<sim.Snapshot().playerZ<<" support="<<sim.Snapshot().playerSupportWorldY<<" toward x="<<destination.x<<" z="<<destination.z<<"\n";
        return false;
    };
    check(walkTo(kWorldRoutePoints[2]),"tomb ramp approach failed");
    const auto safeSide=sim.Snapshot();
    input.yawRadians=std::atan2(5.0f,-7.0f);input.moveForward=1;
    for(int tick=0;tick<20;++tick) sim.StepFixed(input);
    check(sim.Snapshot().worldRoute.blocked && sim.Snapshot().playerX==safeSide.playerX && sim.Snapshot().playerZ==safeSide.playerZ,
        "unprepared forest crossing did not roll back to actual safe side");
    const auto reconstruction=sim.Snapshot();sim.InvalidateWorldZoneReadiness();
    check(sim.Snapshot().playerX==reconstruction.playerX && sim.Snapshot().playerSupportWorldY==reconstruction.playerSupportWorldY,
        "resource reconstruction lost admitted safe transform");
    for(std::size_t i=1;i<kWorldZones.size();++i) admit(static_cast<WorldZoneId>(i));
    for(std::size_t i=3;i<kWorldRoutePoints.size();++i) check(walkTo(kWorldRoutePoints[i]),"continuous forward route failed");
    check(sim.Snapshot().worldRoute.current==WorldZoneId::Lookout,"lookout state missing");
    for(std::size_t i=kWorldRoutePoints.size()-1;i-->0;) check(walkTo(kWorldRoutePoints[i]),"continuous backtrack failed");
    check(near(sim.Snapshot().playerSupportWorldY,horde::gameplay::kRouteFloorWorldY),"backtrack lost tomb floor");
    const auto paused=sim.Snapshot();input.paused=true;
    check(sim.AdvanceFrame(input,.25)==0 && sim.Snapshot().playerSupportWorldY==paused.playerSupportWorldY,"paused publication advances support");
    sim.ResetRoute();
    check(!sim.PublishWorldZoneReadiness(token,ZoneReadiness::Ready),"late pre-reset renderer completion accepted");
    check(sim.Snapshot().playerHeightDelta==0 && sim.Snapshot().playerX==40 && sim.Snapshot().playerZ==-8,
        "reset retained raised support or escaped the opt-in route");
    check(sim.ApplyShowcaseCheckpoint(9,true) && sim.Snapshot().playerX==40 && sim.Snapshot().playerZ==-8,
        "Keeper checkpoint import escaped the opt-in route");
    const auto retryToken=WorldZoneToken{sim.Snapshot().worldRoute.generation,WorldZoneId::ForestApproach};
    sim.RetryEncounter();check(!sim.PublishWorldZoneReadiness(retryToken,ZoneReadiness::Ready),"late retry completion accepted");
    const auto generation=sim.Snapshot().worldRoute.generation;
    sim.InvalidateWorldZoneReadiness();check(sim.Snapshot().worldRoute.generation>generation && !sim.PublishWorldZoneReadiness({generation,WorldZoneId::TombExterior},ZoneReadiness::Ready),"reconstruction generation reused");
    check(sim.PublishWorldZoneReadiness({sim.Snapshot().worldRoute.generation,WorldZoneId::ForestApproach},ZoneReadiness::Failed),"failed prepare rejected");
    check(!admit(WorldZoneId::ForestApproach),"failed generation resurrected");
    check(!sim.PublishWorldZoneReadiness({sim.Snapshot().worldRoute.generation,static_cast<WorldZoneId>(99)},ZoneReadiness::Ready),"invalid zone accepted");
    check(!ResolveWorldRouteSupport(std::numeric_limits<float>::quiet_NaN(),0).grounded,"invalid support grounded");
    check(!ResolveWorldRouteSupport(152,139).grounded,"nonplayable town acquired support");
    sim.SetDevelopmentWorldRoute(false);sim.StepFixed({});
    check(!sim.Snapshot().developmentWorldRoute && sim.Snapshot().playerHeightDelta==0 && sim.Snapshot().playerSupportId==PlayerSupportId::RouteFloor,"default dungeon adapter changed");
    std::cout<<"World route checks="<<checks<<" failures="<<failures<<" triangles="<<resident.triangles.size()
        <<" resident_cpu_ns="<<resident.preparationCpuNanoseconds<<" staged_cpu_ns="<<staged.preparationCpuNanoseconds
        <<" resident_retained_bytes="<<resident.retainedCpuBytes<<" staged_peak_cpu_bytes="<<staged.peakPreparationCpuBytes<<" directions=2 GPU_cost=not-measured\n";
    return failures?1:0;
}
