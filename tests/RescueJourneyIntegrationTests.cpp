#include "gameplay/simulation/GameSimulation.h"
#include "scene/RescueJourneyGeometry.h"
#include "vulkan/raytracing/SimulationFrameAdapter.h"
#include "vulkan/raytracing/DevelopmentWorldSceneAdapter.h"
#include <cmath>
#include <iostream>
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
using namespace horde::gameplay::traversal;
int main() {
 int cases=0,failures=0;
 auto check=[&](bool b,const char* m){++cases;if(!b){++failures;std::cerr<<m<<'\n';}};
 InputSnapshot input;input.damageEnabled=false;
 // The real development launch does not seed missing health, even while paused
 // or after a new-game reset. HUD rendering must consume this published tuple.
 GameSimulation launch(ProductionGameSimulationConfig());
 launch.SetDevelopmentRescueJourney(true);
 InputSnapshot launchInput;launchInput.tutorialEnabled=true;launchInput.tutorialSlowdownEnabled=true;
 check(launch.Snapshot().playerVitals.vitality==3 && launch.Snapshot().playerVitals.maxVitality==3,
       "development journey launch did not start with three hearts");
 launchInput.paused=true;launch.AdvanceFrame(launchInput,.25,1);
 check(launch.Snapshot().playerVitals.vitality==3 && launch.Snapshot().playerVitals.maxVitality==3,
       "paused initial publication lost a heart");
 launchInput.paused=false;
 for(int frame=0;frame<60;++frame)launch.AdvanceFrame(launchInput,1.0/60.0,frame+2);
 check(launch.Snapshot().playerVitals.vitality==3,"idle opening route damaged the player");
 launchInput.commands.routeReset=1;launch.StepFixed(launchInput);
 check(launch.Snapshot().playerVitals.vitality==3 && launch.Snapshot().playerVitals.maxVitality==3,
       "development journey reset did not restore three hearts");

 GameSimulation baseline,disabled;
 disabled.SetDevelopmentRescueJourney(false);
 for(int i=0;i<60;++i) {baseline.StepFixed(input);disabled.StepFixed(input);}
 check(baseline.Snapshot().playerX==disabled.Snapshot().playerX && baseline.Snapshot().playerSupportWorldY==disabled.Snapshot().playerSupportWorldY,"default production support changed");
 GameSimulationConfig config=ProductionGameSimulationConfig();
 config.playerStartX=kLowerLanding.x;config.playerStartZ=kLowerLanding.z;
 GameSimulation sim(config);sim.SetDevelopmentRescueJourney(true);
 check(!sim.Snapshot().rescue.ropeDeployed && sim.Snapshot().rescuePrompt==RescuePrompt::None,"reward bypass before ownership");
 interactions::ChestRewardSnapshot chest;chest.phase=interactions::ChestRewardPhase::LanternClaimed;chest.lidOpenProgress=1;
 interactions::InteractionState interaction;interactions::ResetInteractionState(interaction,interactions::HeldLightKind::RewardLantern);
 interactions::FinaleSequenceSnapshot finale;finale.lichDefeated=true;
 check(sim.ApplyShowcaseCheckpoint(11),"actual Keeper-victory checkpoint unavailable");
 check(sim.Snapshot().skeletonEnemyCount==2,
       "production Keeper-victory checkpoint did not retain both waterfall guards");
 sim.ImportRewardCheckpoint(chest,interaction,finale);
 check(sim.Snapshot().rescue.claimCount==1 && sim.Snapshot().rescue.deploymentCount==1,"claimed import did not seed one owned deployment");
 horde::vulkan::raytracing::RtSceneTuning overrides;overrides.finaleRoofOpenOverride=0;overrides.finaleDawnRevealOverride=1;
 const auto protectedFrame=horde::vulkan::raytracing::BuildRtSceneFrameInputs(sim.Snapshot(),.92f,overrides);
 check(protectedFrame.lich.finaleSkylightOpenProgress==1 && protectedFrame.lich.finaleDawnRevealProgress==0,
       "legacy diagnostic overrides closed an admitted rescue opening or ended the night");
 // Import only the already-open chest for this narrow claim-site test. The
 // actual interaction must own deployment, not the test/import itself.
 GameSimulationConfig claimConfig;claimConfig.playerStartX=kRewardChestRoutePosition.x+1.30f;claimConfig.playerStartZ=kRewardChestRoutePosition.z;
 GameSimulation claimSim(claimConfig);claimSim.SetDevelopmentRescueJourney(true);
 auto available=chest;available.phase=interactions::ChestRewardPhase::LanternAvailable;
 interactions::InteractionState beforeClaim;interactions::ResetInteractionState(beforeClaim,interactions::HeldLightKind::None);
 claimSim.ImportRewardCheckpoint(available,beforeClaim,finale);
 check(!claimSim.Snapshot().rescue.ropeDeployed,"open chest imported an unowned rope");
 InputSnapshot claimInput;claimInput.damageEnabled=false;claimInput.yawRadians=-1.57079632679f;claimInput.commands.interact=1;
 claimSim.StepFixed(claimInput);
 std::cout<<"claim-site position="<<claimSim.Snapshot().playerX<<","<<claimSim.Snapshot().playerZ
          <<" prompt="<<static_cast<int>(claimSim.Snapshot().chestPrompt)<<" phase="<<static_cast<int>(claimSim.Snapshot().chestReward.phase)<<'\n';
 check(claimSim.Snapshot().chestReward.phase==interactions::ChestRewardPhase::LanternClaimed &&
       claimSim.Snapshot().rescue.claimCount==1 && claimSim.Snapshot().rescue.deploymentCount==1 &&
       claimSim.Snapshot().rescue.deploymentPhase==RopeDeploymentPhase::WaitingForOpening &&
       !claimSim.Snapshot().rescue.ropeDeployed,
       "actual lantern interaction did not begin one opening-gated owned rope deployment");
 ++claimInput.commands.interact;claimSim.StepFixed(claimInput);
 check(claimSim.Snapshot().rescue.deploymentCount==1,"repeated claim intent deployed twice");
 bool sawThrowWarning=false;
 for(int tick=0;tick<420&&!sim.Snapshot().rescue.ropeReady;++tick) {
  sim.StepFixed(input);
   if(sim.Snapshot().rescue.deploymentPhase==RopeDeploymentPhase::WarningBeforeThrow)
    sawThrowWarning=true;
 }
 check(sawThrowWarning,"imported claim skipped the distinct pre-throw warning interval");
 check(sim.Snapshot().rescue.ropeReady && sim.Snapshot().rescue.deploymentPhase==RopeDeploymentPhase::Ready,
       "imported owned rope never completed its opening-gated physical deployment");
 input.commands.interact=1;sim.StepFixed(input);
 check(!sim.Snapshot().rescue.equipmentStowed && sim.Snapshot().playerSupportWorldY==kLowerSupportWorldY,"unready destination committed");
 horde::vulkan::raytracing::PresentableTinyRtScene notReady;
 horde::vulkan::raytracing::PublishDevelopmentWorldReadiness(sim,notReady);
 check(sim.Snapshot().worldRoute.readiness[1]!=ZoneReadiness::Ready,"unbuilt renderer falsely admitted destination");
 const auto token=WorldZoneToken{sim.Snapshot().worldRoute.generation,WorldZoneId::TombExterior};
 check(sim.PublishWorldZoneReadiness(token,ZoneReadiness::Ready),"current readiness could not be admitted");
 for(int cycle=0;cycle<2;++cycle) {
  ++input.commands.interact;++input.commands.attack;sim.StepFixed(input);
  check(sim.Snapshot().rescue.equipmentStowed,"ascent failed to commit from lower endpoint");
  check(sim.Snapshot().playerCombat.action==PlayerCombatAction::Idle,
      "simultaneous traversal intent leaked a combat action before taking ownership");
  input.commands.attack+=3;input.commands.parry+=2;input.commands.dodge+=2;input.commands.toggleHeldLightPose+=2;
  input.moveForward=1;input.runHeld=true;input.yawRadians=1.1f;
  for(int tick=0;tick<1200 && sim.Snapshot().rescue.equipmentStowed;++tick) {
   sim.StepFixed(input);
   const auto s=sim.Snapshot();
   if(tick==40 && s.rescue.equipmentStowed) {
    input.paused=true;sim.StepFixed(input);const auto held=sim.Snapshot();
    check(held.rescue.ropeNodes==s.rescue.ropeNodes && held.playerSupportWorldY==s.playerSupportWorldY &&
          held.rewardLanternWorldFromHinge==s.rewardLanternWorldFromHinge,
          "paused traversal moved the loaded rope, support or independent lantern hinge");
    check(sim.AdvanceFrame(input,.25)==0 && sim.Snapshot().rescue.ropeNodes==s.rescue.ropeNodes,
          "paused zero-tick frame advanced physical traversal");
    input.paused=false;
   }
   check(s.playerSupportWorldY==s.rescue.supportWorldY,"root not authoritative loaded rope support");
   check(!s.finaleComplete && s.lich.finaleDawnRevealProgress==0,"legacy ending interrupted campaign night");
   if(s.rescue.equipmentStowed) check(s.playerAnimation.swordStowBlend==1 && s.playerAnimation.swordHandGripBlend==0 &&
       s.heldItems[1].parentMode==items::HeldItemParentMode::BodyStow && s.heldItems[1].visualStowBlend==1,
       "sword did not free both hands with authoritative body ownership");
   const auto f=horde::vulkan::raytracing::BuildRtSceneFrameInputs(s,.92f,horde::vulkan::raytracing::WaterQuality::High);
   check(f.rescue.ropeNodes==s.rescue.ropeNodes && f.playerSupportWorldY==s.playerSupportWorldY,"frame adapter lost loaded rope/root");
  }
  check(sim.Snapshot().rescue.exteriorSide && !sim.Snapshot().rescue.equipmentStowed,"ascent never reached exterior safe side");
  std::cout<<"cycle="<<cycle<<" ascent-end phase="<<static_cast<int>(sim.Snapshot().rescue.phase)
           <<" root="<<sim.Snapshot().playerX<<","<<sim.Snapshot().playerSupportWorldY<<","<<sim.Snapshot().playerZ<<'\n';
  check(std::abs(sim.Snapshot().playerSupportWorldY-kUpperSupportWorldY)<.001f,"upper landing support incorrect");
  // The upper apron/connector is real retained support, not the isolated WP2
  // centreline. A free hand must return to its ordinary carry depth here.
  input.moveForward=0;input.runHeld=false;input.yawRadians=2.250f;
  sim.StepFixed(input);
  const auto& upperCarry=sim.Snapshot();
  std::cout<<"upper lantern hand depth="<<upperCarry.heldItemKinematics.leftHandLocal[2]
           <<" localY="<<upperCarry.heldItemKinematics.leftHandLocal[1]<<'\n';
  check(upperCarry.heldItemKinematics.leftHandLocal[2]>.45f,
        "upper landing falsely retracts restored lantern against a missing route wall");
  check(upperCarry.rewardLanternWorldFromHinge[13]>upperCarry.playerSupportWorldY+1.25f &&
        upperCarry.interaction.heldLightKind==interactions::HeldLightKind::RewardLantern,
        "upper safe landing must restore claimed lantern to physical free hand");
  const auto exteriorFrame=horde::vulkan::raytracing::BuildRtSceneFrameInputs(sim.Snapshot(),.92f,horde::vulkan::raytracing::WaterQuality::High);
  const auto plan=horde::vulkan::raytracing::EvaluateCharacterFramePlan(exteriorFrame.skeletonEnemies,
      exteriorFrame.skeletonEnemyCount,exteriorFrame.roster,exteriorFrame.lich,1);
  check(plan.lichVisible && plan.skeletonCount==2 &&
        exteriorFrame.skeletonEnemyCount==sim.Snapshot().skeletonEnemyCount,
      "exterior handoff discards the off-camera Keeper or either authoritative waterfall guard");
  for(std::size_t enemy=0;enemy<2;++enemy)
      check(exteriorFrame.skeletonEnemies[enemy].x==sim.Snapshot().skeletonEnemies[enemy].x &&
            exteriorFrame.skeletonEnemies[enemy].z==sim.Snapshot().skeletonEnemies[enemy].z,
            "exterior presentation substitutes a synthetic actor for a waterfall guard");
  input.moveForward=0;input.runHeld=false;
  const auto paused=sim.Snapshot();input.paused=true;sim.StepFixed(input);
  check(sim.Snapshot().playerX==paused.playerX && sim.Snapshot().playerSupportWorldY==paused.playerSupportWorldY,"pause moved safe side");input.paused=false;
  if(cycle==0) {
   const auto walkTo=[&](const WorldRoutePoint& destination) {
    for(int tick=0;tick<2600;++tick) {
     const auto before=sim.Snapshot();const float dx=destination.x-before.playerX,dz=destination.z-before.playerZ;
     const float distance=std::hypot(dx,dz);if(distance<.03f) return true;
     input.yawRadians=std::atan2(dx,-dz);input.moveForward=std::min(1.0f,distance/.055f);input.moveStrafe=0;input.runHeld=distance>.3f;
     sim.StepFixed(input);const auto after=sim.Snapshot();const auto support=RescueExteriorSupport(after.playerX,after.playerZ);
     check(support.grounded && std::abs(support.worldY-after.playerSupportWorldY)<.0001f,"walking support differs from actual retained triangles");
     check(std::abs(after.playerSupportWorldY-before.playerSupportWorldY)<=kWorldRouteMaximumStep+.0001f,"connector walk accepted an unbounded step");
    }
    std::cerr<<"route blocked at "<<sim.Snapshot().playerX<<","<<sim.Snapshot().playerZ<<" toward "<<destination.x<<","<<destination.z<<'\n';return false;
   };
   const auto edge=sim.Snapshot();input.moveForward=1;input.yawRadians=std::atan2(5.0f,-7.0f);
   auto lastSafe=edge;
   for(int tick=0;tick<120;++tick) {
    sim.StepFixed(input);
    if(sim.Snapshot().worldRoute.blocked) break;
    check(OnRescueLanding(sim.Snapshot().playerX,sim.Snapshot().playerZ) &&
          sim.Snapshot().worldRoute.current==WorldZoneId::TombExterior,
          "unready forest permits movement only on the actually ready landing");
    lastSafe=sim.Snapshot();
   }
   std::cout<<"unready-edge start="<<edge.playerX<<","<<edge.playerZ<<" end="<<sim.Snapshot().playerX<<","<<sim.Snapshot().playerZ
            <<" blocked="<<sim.Snapshot().worldRoute.blocked<<'\n';
   check(sim.Snapshot().worldRoute.blocked &&
         sim.Snapshot().playerX==lastSafe.playerX && sim.Snapshot().playerZ==lastSafe.playerZ &&
         OnRescueLanding(sim.Snapshot().playerX,sim.Snapshot().playerZ),
       "unprepared forest crossing escaped its last safe side");
   for(auto zone:{WorldZoneId::ForestApproach,WorldZoneId::Lookout})
    check(sim.PublishWorldZoneReadiness({sim.Snapshot().worldRoute.generation,zone},ZoneReadiness::Ready),"current forest host token rejected");
   // These positive CPU tokens are harness inputs. Real renderer admission is
   // separately guarded above and cannot be inferred from this host traversal.
   for(std::size_t i=1;i<kRescueConnector.size();++i) check(walkTo(kRescueConnector[i]),"actual simulation rescue-to-clue trail walk failed");
   check(walkTo(kWorldRoutePoints.back()),"actual simulation lookout approach failed");
   check(sim.Snapshot().worldRoute.current==WorldZoneId::Lookout,"journey did not reach lookout");
   const auto lookout=sim.Snapshot();input.moveForward=0;input.runHeld=false;input.paused=true;
   check(sim.AdvanceFrame(input,.25)==0 && sim.Snapshot().playerX==lookout.playerX,"paused route publication moved the lookout");input.paused=false;
   for(std::size_t i=kWorldRoutePoints.size()-1;i-->2;)check(walkTo(kWorldRoutePoints[i]),"actual simulation lookout backtrack failed");
   input.moveForward=0;input.runHeld=false;
  }
  ++input.commands.interact;sim.StepFixed(input);
  check(sim.Snapshot().rescue.equipmentStowed,"descent did not commit");
  if(cycle==0) {
   for(int tick=0;tick<8;++tick)sim.StepFixed(input);
   const auto stale=WorldZoneToken{sim.Snapshot().worldRoute.generation,WorldZoneId::TombExterior};
   sim.InvalidateWorldZoneReadiness();
   check(sim.Snapshot().rescue.exteriorSide && !sim.Snapshot().rescue.equipmentStowed && sim.Snapshot().playerSupportWorldY==kUpperSupportWorldY,
       "interrupted descent did not restore exterior/equipment ownership");
   check(!sim.PublishWorldZoneReadiness(stale,ZoneReadiness::Ready),"stale descent reconstruction callback accepted");
   sim.PublishWorldZoneReadiness({sim.Snapshot().worldRoute.generation,WorldZoneId::TombExterior},ZoneReadiness::Ready);
   ++input.commands.interact;sim.StepFixed(input);
   check(sim.Snapshot().rescue.equipmentStowed,"fresh descent edge failed after reconstruction");
  }
  for(int tick=0;tick<1200 && sim.Snapshot().rescue.equipmentStowed;++tick) sim.StepFixed(input);
  check(!sim.Snapshot().rescue.exteriorSide && sim.Snapshot().playerSupportWorldY==kLowerSupportWorldY,"descent lost safe lower landing");
  check(sim.Snapshot().rescue.claimCount==1 && sim.Snapshot().rescue.deploymentCount==1,"repeat traversal duplicated reward/deployment");
 }
 ++input.commands.interact;sim.StepFixed(input);for(int i=0;i<25;++i)sim.StepFixed(input);
 const auto old=WorldZoneToken{sim.Snapshot().worldRoute.generation,WorldZoneId::TombExterior};
 sim.InvalidateWorldZoneReadiness();
 check(!sim.Snapshot().rescue.exteriorSide && sim.Snapshot().playerSupportWorldY==kLowerSupportWorldY,"interrupted ascent did not return lower");
 check(!sim.PublishWorldZoneReadiness(old,ZoneReadiness::Ready),"late reconstruction callback admitted");
 const auto paidRopeBeforeRetry=sim.Snapshot().rescue.ropeNodes;
 sim.RetryEncounter();check(sim.Snapshot().rescue.claimCount==1 && sim.Snapshot().rescue.ropeReady &&
       sim.Snapshot().rescue.ropeNodes==paidRopeBeforeRetry,
       "logical retry lost or replayed the already paid-out rope deployment");
 sim.ResetRoute();check(!sim.Snapshot().rescue.ropeDeployed && sim.Snapshot().rescue.claimCount==0,"new route kept stale deployment");
 auto resident=horde::scene::PrepareDevelopmentWorldGeometry(false,true),staged=horde::scene::PrepareDevelopmentWorldGeometry(true,true);
 horde::scene::AppendRescueJourneyGeometry(resident);horde::scene::AppendRescueJourneyGeometry(staged);
 check(resident.valid&&staged.valid&&resident.triangles.size()==staged.triangles.size(),"combined geometry failed admission");
 check(resident.triangles.size()==horde::scene::PrepareDevelopmentWorldGeometry(false,true).triangles.size()+
       (horde::scene::kRescueBlockoutBoxes.size()+2)*12+
       horde::scene::kOutdoorEffectMarkerTriangleCount,
       "combined real scene is missing counted rim, anchor or labelled experiment contributors");
 const auto preLanding=horde::gameplay::traversal::RescueExteriorSupport(
     kWorldRoutePoints[1].x-1.6f,(kWorldRoutePoints[1].z+kWorldRoutePoints[2].z)*.5f);
 check(!preLanding.grounded,
       "nonplayable roof-overlapping segment 1 retained rescue support outside the F01 apron");
 const auto apron=horde::gameplay::traversal::RescueExteriorSupport(kExteriorLanding.x,kExteriorLanding.z);
 check(apron.grounded&&std::abs(apron.worldY-kUpperSupportWorldY)<.0001f,
       "rescue landing apron lost its authored safe support");
 check(!RescueExteriorSupport(-33.7f,-15.2f).grounded,
       "wooded terrain incorrectly closes the actual rope shaft aperture");
 for(const auto& triangle:resident.triangles) {
    // A vertical ray through the open shaft must not hit route-bank triangles
    // between the lower room and the rim. Actual rim boxes are outside this ray.
    const auto a=triangle.points[0],b=triangle.points[1],c=triangle.points[2];
    const float den=(b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2]);
    if(std::abs(den)<1e-6f)continue;
    const float u=((b[2]-c[2])*(-33.7f-c[0])+(c[0]-b[0])*(-15.2f-c[2]))/den;
    const float v=((c[2]-a[2])*(-33.7f-c[0])+(a[0]-c[0])*(-15.2f-c[2]))/den;
    if(u>=0&&v>=0&&u+v<=1) {
       const float y=u*a[1]+v*b[1]+(1-u-v)*c[1];
       check(y<kLowerSupportWorldY||y>kUpperSupportWorldY,
             "actual combined terrain triangles obstruct the open shaft");
    }
 }
 for(const auto& marker:horde::scene::kOutdoorEffectRegionMarkers)
  check(!marker.effectImplemented && std::string_view(marker.label).find("NOT IMPLEMENTED")!=std::string_view::npos,
        "geometry marker falsely declared an implemented outdoor effect");
 for(std::size_t i=0;i<resident.triangles.size();++i)check(resident.triangles[i].points==staged.triangles[i].points,"staging dropped ray contributors");
 for(int i=0;i<=600;++i) {
  const float t=i/600.0f;
  for(std::size_t s=0;s+1<kRescueConnector.size();++s) {
   const auto a=kRescueConnector[s],b=kRescueConnector[s+1];
   const float x=a.x+(b.x-a.x)*t,z=a.z+(b.z-a.z)*t;
   const auto support=RescueExteriorSupport(x,z);
   check(support.grounded,"connector support gap");
   bool renderedTop=false;
   for(const auto& triangle:resident.triangles) {
    if(triangle.normal!=0u)continue;
    const auto p=triangle.points[0],q=triangle.points[1],r=triangle.points[2];
    const float den=(q[2]-r[2])*(p[0]-r[0])+(r[0]-q[0])*(p[2]-r[2]);
    if(std::abs(den)<1e-6f)continue;
    const float u=((q[2]-r[2])*(x-r[0])+(r[0]-q[0])*(z-r[2]))/den;
    const float v=((r[2]-p[2])*(x-r[0])+(p[0]-r[0])*(z-r[2]))/den;
    if(u>=-.00001f&&v>=-.00001f&&u+v<=1.00001f) {
     const float y=u*p[1]+v*q[1]+(1-u-v)*r[1];
     renderedTop|=std::abs(y-support.worldY)<.0001f;
    }
   }
   check(renderedTop,"connector/apron support has no matching emitted top triangle");
  }
 }
 {
  const auto a=kWorldRoutePoints[2],b=kWorldRoutePoints[3];
  const float dx=b.x-a.x,dz=b.z-a.z,length=std::hypot(dx,dz);
  const float cx=a.x+.65f*dx,cz=a.z+.65f*dz,nx=-dz/length,nz=dx/length;
  for(float side:{-1.0f,1.0f}) {
   const float insideX=cx+side*nx*11.0f,insideZ=cz+side*nz*11.0f;
   const auto inside=RescueExteriorSupport(insideX,insideZ);
   check(inside.grounded,"wide rescue approach bank lost actual rendered support");
   const float outsideX=cx+side*nx*13.0f,outsideZ=cz+side*nz*13.0f;
   check(!RescueExteriorSupport(outsideX,outsideZ).grounded,
         "rescue support extended beyond the actual exterior terrain edge");
   check(!WorldRouteTerrainMovementClear(cx,cz,outsideX,outsideZ,inside.worldY,.35f),
         "rescue capsule sweep crossed the actual exterior terrain edge");
  }
 }
 std::cout<<"Combined CPU preparation resident_ns="<<resident.preparationCpuNanoseconds
          <<" staged_ns="<<staged.preparationCpuNanoseconds
          <<" resident_retained_bytes="<<resident.retainedCpuBytes
          <<" staged_retained_bytes="<<staged.retainedCpuBytes
          <<" resident_peak_capacity_bytes="<<resident.peakPreparationCpuBytes
          <<" staged_peak_capacity_bytes="<<staged.peakPreparationCpuBytes
          <<" gpu_residency_comparison=NOT_MEASURED"<<'\n';
 std::cout<<"Rescue journey integration cases="<<cases<<" failures="<<failures<<" retained_triangles="<<resident.triangles.size()<<'\n';return failures?1:0;
}
