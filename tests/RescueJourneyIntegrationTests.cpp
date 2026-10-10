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
 GameSimulation baseline,disabled;
 disabled.SetDevelopmentRescueJourney(false);
 for(int i=0;i<60;++i) {baseline.StepFixed(input);disabled.StepFixed(input);}
 check(baseline.Snapshot().playerX==disabled.Snapshot().playerX && baseline.Snapshot().playerSupportWorldY==disabled.Snapshot().playerSupportWorldY,"default production support changed");
 GameSimulationConfig config;config.playerStartX=kLowerLanding.x;config.playerStartZ=kLowerLanding.z;
 config.playerMountProfile=items::PlayerMountProfile::AnatomicalBody;
 GameSimulation sim(config);sim.SetDevelopmentRescueJourney(true);
 check(!sim.Snapshot().rescue.ropeDeployed && sim.Snapshot().rescuePrompt==RescuePrompt::None,"reward bypass before ownership");
 interactions::ChestRewardSnapshot chest;chest.phase=interactions::ChestRewardPhase::LanternClaimed;chest.lidOpenProgress=1;
 interactions::InteractionState interaction;interactions::ResetInteractionState(interaction,interactions::HeldLightKind::RewardLantern);
 interactions::FinaleSequenceSnapshot finale;finale.lichDefeated=true;
 check(sim.ApplyShowcaseCheckpoint(11),"actual Keeper-victory checkpoint unavailable");
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
       claimSim.Snapshot().rescue.claimCount==1 && claimSim.Snapshot().rescue.deploymentCount==1,
       "actual lantern interaction did not deploy exactly one owned rope");
 ++claimInput.commands.interact;claimSim.StepFixed(claimInput);
 check(claimSim.Snapshot().rescue.deploymentCount==1,"repeated claim intent deployed twice");
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
  const auto exteriorFrame=horde::vulkan::raytracing::BuildRtSceneFrameInputs(sim.Snapshot(),.92f,horde::vulkan::raytracing::WaterQuality::High);
  const auto plan=horde::vulkan::raytracing::EvaluateCharacterFramePlan(exteriorFrame.skeletonEnemies,
      exteriorFrame.skeletonEnemyCount,exteriorFrame.roster,exteriorFrame.lich,1,true);
  check(plan.selectedLich && plan.skeletonCount==1 && plan.skeletons[0].poseBucket==1,
      "combined workload discarded the off-camera Keeper or selected the wrong skeleton buffer");
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
   // These positive CPU tokens are harness inputs. Real renderer admission is
   // separately guarded above and cannot be inferred from this host traversal.
   for(std::size_t i=1;i<kRescueConnector.size();++i) check(walkTo(kRescueConnector[i]),"actual simulation connector walk failed");
   const auto edge=sim.Snapshot();input.moveForward=1;input.yawRadians=std::atan2(5.0f,-7.0f);
   for(int tick=0;tick<20;++tick)sim.StepFixed(input);
   std::cout<<"unready-edge start="<<edge.playerX<<","<<edge.playerZ<<" end="<<sim.Snapshot().playerX<<","<<sim.Snapshot().playerZ
            <<" blocked="<<sim.Snapshot().worldRoute.blocked<<'\n';
   check(sim.Snapshot().worldRoute.blocked && std::hypot(sim.Snapshot().playerX-edge.playerX,sim.Snapshot().playerZ-edge.playerZ)<.15f,
       "unprepared forest crossing escaped its last safe side");
   for(auto zone:{WorldZoneId::ForestApproach,WorldZoneId::Lookout})
    check(sim.PublishWorldZoneReadiness({sim.Snapshot().worldRoute.generation,zone},ZoneReadiness::Ready),"current forest host token rejected");
   for(std::size_t i=3;i<kWorldRoutePoints.size();++i)check(walkTo(kWorldRoutePoints[i]),"actual simulation night route forward failed");
   check(sim.Snapshot().worldRoute.current==WorldZoneId::Lookout,"journey did not reach lookout");
   const auto lookout=sim.Snapshot();input.moveForward=0;input.runHeld=false;input.paused=true;
   check(sim.AdvanceFrame(input,.25)==0 && sim.Snapshot().playerX==lookout.playerX,"paused route publication moved the lookout");input.paused=false;
   for(std::size_t i=kWorldRoutePoints.size()-1;i-->2;)check(walkTo(kWorldRoutePoints[i]),"actual simulation lookout backtrack failed");
   for(std::size_t i=kRescueConnector.size()-1;i-->0;)check(walkTo(kRescueConnector[i]),"actual simulation shaft backtrack failed");
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
 sim.RetryEncounter();check(sim.Snapshot().rescue.claimCount==1,"logical retry lost ownership");
 sim.ResetRoute();check(!sim.Snapshot().rescue.ropeDeployed && sim.Snapshot().rescue.claimCount==0,"new route kept stale deployment");
 auto resident=horde::scene::PrepareDevelopmentWorldGeometry(false),staged=horde::scene::PrepareDevelopmentWorldGeometry(true);
 horde::scene::AppendRescueJourneyGeometry(resident);horde::scene::AppendRescueJourneyGeometry(staged);
 check(resident.valid&&staged.valid&&resident.triangles.size()==staged.triangles.size(),"combined geometry failed admission");
 check(resident.triangles.size()==934+horde::scene::kOutdoorEffectMarkerTriangleCount,
       "region markers were not included as separately counted original geometry");
 for(const auto& marker:horde::scene::kOutdoorEffectRegionMarkers)
  check(!marker.effectImplemented && std::string_view(marker.label).find("NOT IMPLEMENTED")!=std::string_view::npos,
        "geometry marker falsely declared an implemented outdoor effect");
 for(std::size_t i=0;i<resident.triangles.size();++i)check(resident.triangles[i].points==staged.triangles[i].points,"staging dropped ray contributors");
 for(int i=0;i<=600;++i) {
  const float t=i/600.0f;
  for(std::size_t s=0;s+1<kRescueConnector.size();++s) {
   const auto a=kRescueConnector[s],b=kRescueConnector[s+1];
   check(RescueExteriorSupport(a.x+(b.x-a.x)*t,a.z+(b.z-a.z)*t).grounded,"connector support gap");
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
