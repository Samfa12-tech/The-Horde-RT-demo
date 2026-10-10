#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/traversal/DevelopmentRescueJourney.h"
#include "scene/DevelopmentWorldGeometry.h"
#include "vulkan/raytracing/DevelopmentWorldSceneAdapter.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>

using namespace horde::gameplay::simulation;
int main()
{
    int checks=0,failures=0;
    auto check=[&](bool result,const char* why){ ++checks;if(!result){++failures;std::cerr<<why<<'\n';} };
    const auto near=[](float a,float b){return std::abs(a-b)<.0001f;};
    const auto resident=horde::scene::PrepareDevelopmentWorldGeometry(false);
    const auto staged=horde::scene::PrepareDevelopmentWorldGeometry(true);
    check(resident.valid && staged.valid && resident.triangles.size()==staged.triangles.size(),"real mesh preparation failed");
    const auto rescue= horde::gameplay::traversal::kExteriorLanding;
    check(std::hypot(kWorldRoutePoints[2].x-rescue.x,kWorldRoutePoints[2].z-rescue.z)<.01f &&
          near(kWorldRoutePoints[2].y,horde::gameplay::traversal::kUpperSupportWorldY),
          "planned F01 rescue rim is not joined to the real exterior rope landing");
    float trailLength=0;
    for(std::size_t i=2;i+1<kWorldRoutePoints.size();++i)
        trailLength+=std::hypot(kWorldRoutePoints[i+1].x-kWorldRoutePoints[i].x,
                                kWorldRoutePoints[i+1].z-kWorldRoutePoints[i].z);
    check(trailLength>=40.0f && trailLength<=80.0f,
          "F01-to-lookout centerline is outside the bounded 40-80 m trail intent");
    check(kWorldRouteHalfWidth>=12.0f,
          "playable terrain remains a narrow route ribbon");
    check(WorldRouteSurfaceTriangles().size()>14,
          "rendered/support terrain has no cross-slope tessellation beyond the route ribbon");
    check(WorldRouteTerrainShellTriangles().size()==136,
          "bounded terrain is missing the closed underside, outer banks, or end caps");
    bool hasUnderside=false;
    VisitWorldRouteTerrainShellTriangles([&](const auto& tri,std::size_t,std::uint32_t normal) {
        if(normal==1u && tri[0][1]==-5.0f && tri[1][1]==-5.0f && tri[2][1]==-5.0f)
            hasUnderside=true;
    });
    check(hasUnderside,"terrain upload has no real underside triangles");
    bool hasSolidBank=false;
    VisitWorldRouteTerrainShellTriangles([&](const auto& tri,std::size_t,std::uint32_t normal) {
        hasSolidBank |= (normal==2u||normal==3u) &&
            ((tri[0][1]==-5.0f&&tri[1][1]==-5.0f)||(tri[1][1]==-5.0f&&tri[2][1]==-5.0f));
    });
    check(hasSolidBank,"terrain perimeter has no solid cut-bank geometry");
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
    const auto walkTo=[&](const WorldRoutePoint& destination){
        for(int tick=0;tick<3500;++tick) {
            const auto before=sim.Snapshot(); const float dx=destination.x-before.playerX,dz=destination.z-before.playerZ;
            if(std::hypot(dx,dz)<.000025f) return true;
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
    const auto lookout=kWorldRoutePoints.back();
    const auto previous=kWorldRoutePoints[kWorldRoutePoints.size()-2];
    const float lastDx=lookout.x-previous.x,lastDz=lookout.z-previous.z,lastLength=std::hypot(lastDx,lastDz);
    const WorldRoutePoint bank{lookout.x-lastDz/lastLength*6.0f,lookout.z+lastDx/lastLength*6.0f,lookout.y};
    check(walkTo(bank),"player could not walk six metres across the rendered lookout bank");
    check(std::hypot(sim.Snapshot().playerX-lookout.x,sim.Snapshot().playerZ-lookout.z)>5.5f,
          "simulation kept the player inside the old narrow route ribbon");
    check(walkTo(lookout),"player could not return from the supported lookout bank");
    for(std::size_t i=kWorldRoutePoints.size()-1;i-->0;) check(walkTo(kWorldRoutePoints[i]),"continuous backtrack failed");
    check(near(sim.Snapshot().playerSupportWorldY,horde::gameplay::kRouteFloorWorldY),"backtrack lost tomb floor");
    const auto paused=sim.Snapshot();input.paused=true;
    check(sim.AdvanceFrame(input,.25)==0 && sim.Snapshot().playerSupportWorldY==paused.playerSupportWorldY,"paused publication advances support");
    sim.ResetRoute();
    check(!sim.PublishWorldZoneReadiness(token,ZoneReadiness::Ready),"late pre-reset renderer completion accepted");
    check(sim.Snapshot().playerHeightDelta==0 && near(sim.Snapshot().playerX,kWorldRoutePoints[0].x) &&
          near(sim.Snapshot().playerZ,kWorldRoutePoints[0].z),
        "reset retained raised support or escaped the opt-in route");
    check(sim.ApplyShowcaseCheckpoint(9,true) && near(sim.Snapshot().playerX,kWorldRoutePoints[0].x) &&
          near(sim.Snapshot().playerZ,kWorldRoutePoints[0].z),
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
    for(std::size_t i=2;i+1<kWorldRoutePoints.size();++i) {
        const auto a=kWorldRoutePoints[i],b=kWorldRoutePoints[i+1];
        const float dx=b.x-a.x,dz=b.z-a.z,length=std::hypot(dx,dz);
        const float nx=-dz/length,nz=dx/length;
        const float t=.43f,x=a.x+t*dx+nx*6.0f,z=a.z+t*dz+nz*6.0f;
        const auto support=ResolveWorldRouteSupport(x,z);
        check(support.grounded && support.surface==
                  kWorldZones[static_cast<std::size_t>(ZoneForRouteSegment(i))].surface,
              "wide forest bank support missing six metres off the actual centerline");
        const auto oppositeBank=ResolveWorldRouteSupport(a.x+t*dx-nx*6.0f,a.z+t*dz-nz*6.0f);
        check(oppositeBank.grounded && oppositeBank.surface==support.surface,
              "opposite side of the wide terrain bank lost its shared support classification");
        bool renderedMatch=false;
        VisitWorldRouteSurfaceTriangles([&](const auto& tri,std::size_t segment) {
            if(segment!=i) return;
            const auto p=tri[0],q=tri[1],r=tri[2];
            const float d=(q[2]-r[2])*(p[0]-r[0])+(r[0]-q[0])*(p[2]-r[2]);
            if(std::abs(d)<1e-7f) return;
            const float u=((q[2]-r[2])*(x-r[0])+(r[0]-q[0])*(z-r[2]))/d;
            const float v=((r[2]-p[2])*(x-r[0])+(p[0]-r[0])*(z-r[2]))/d;
            if(u>=-.00001f&&v>=-.00001f&&u+v<=1.00001f) {
                const float y=u*p[1]+v*q[1]+(1-u-v)*r[1];
                renderedMatch=near(y,support.worldY);
            }
        });
        check(renderedMatch,"support elevation disagrees with a rendered terrain triangle away from centerline");
    }
    const auto terrainA=kWorldRoutePoints[6],terrainB=kWorldRoutePoints[7];
    const float terrainDx=terrainB.x-terrainA.x,terrainDz=terrainB.z-terrainA.z;
    const float terrainLength=std::hypot(terrainDx,terrainDz);
    const float terrainX=terrainA.x+.45f*terrainDx,terrainZ=terrainA.z+.45f*terrainDz;
    const float terrainNx=-terrainDz/terrainLength,terrainNz=terrainDx/terrainLength;
    const auto terrainStart=ResolveWorldRouteSupport(terrainX,terrainZ);
    check(WorldRouteTerrainMovementClear(terrainX,terrainZ,terrainX+terrainNx*6.0f,
              terrainZ+terrainNz*6.0f,terrainStart.worldY,.35f),
          "actual mesh capsule sweep rejected a supported cross-slope bank");
    check(!WorldRouteTerrainMovementClear(terrainX,terrainZ,terrainX+terrainNx*13.0f,
              terrainZ+terrainNz*13.0f,terrainStart.worldY,.35f),
          "actual mesh capsule sweep crossed the solid terrain bank boundary");
    const auto obstacle=std::find_if(WorldRouteBlockoutBoxes().begin(),WorldRouteBlockoutBoxes().end(),
        [](const auto& box){return box.material==0u;});
    check(obstacle!=WorldRouteBlockoutBoxes().end(),"shared route mesh has no solid trunk fixture");
    if(obstacle!=WorldRouteBlockoutBoxes().end()) {
        const float z=(obstacle->minimum[2]+obstacle->maximum[2])*.5f;
        const float supportY=obstacle->minimum[1]+.3f;
        check(!WorldRouteBlockoutMovementClear(obstacle->minimum[0]-1.0f,z,
                  obstacle->maximum[0]+1.0f,z,supportY,.35f),
              "capsule sweep crossed an actual rendered opaque trunk box");
        const auto owner=static_cast<WorldZoneId>(ZoneForRouteSegment(obstacle->segment));
        bool renderedBox=false;
        for(const auto& tri:resident.triangles) if(tri.owner==owner)
            for(const auto& point:tri.points)
                renderedBox |= near(point[0],obstacle->minimum[0])&&near(point[1],obstacle->minimum[1])&&
                               near(point[2],obstacle->minimum[2]);
        check(renderedBox,"collision trunk box is absent from the uploaded RT geometry");
    }
    sim.SetDevelopmentWorldRoute(false);sim.StepFixed({});
    check(!sim.Snapshot().developmentWorldRoute && sim.Snapshot().playerHeightDelta==0 && sim.Snapshot().playerSupportId==PlayerSupportId::RouteFloor,"default dungeon adapter changed");
    std::cout<<"World route checks="<<checks<<" failures="<<failures<<" triangles="<<resident.triangles.size()
        <<" resident_cpu_ns="<<resident.preparationCpuNanoseconds<<" staged_cpu_ns="<<staged.preparationCpuNanoseconds
        <<" resident_retained_bytes="<<resident.retainedCpuBytes<<" staged_peak_cpu_bytes="<<staged.peakPreparationCpuBytes<<" directions=2 GPU_cost=not-measured\n";
    return failures?1:0;
}
