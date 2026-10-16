#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/DevelopmentCheckpointSimulation.h"
#include "scene/RescueJourneyGeometry.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/assets/AssetManifest.h"
#include "gameplay/traversal/ChapterTerrainMovement.h"
#include "vulkan/raytracing/SimulationFrameAdapter.h"
#include "vulkan/raytracing/DevelopmentWorldSceneAdapter.h"
#include "platform/windows/WindowsCaptureContracts.h"
#include "gameplay/validation/ChapterRoundTripProbe.h"
#include "gameplay/validation/ChapterRouteMeasurementClock.h"
#include "gameplay/validation/ChapterRouteHostPhases.h"
#include "gameplay/validation/MotionEvidenceScenario.h"
#include <cmath>
#include <numeric>
#include <iostream>
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
using namespace horde::gameplay::traversal;
int main(int argc,char** argv) {
 const bool authored=argc==2 && std::string_view(argv[1])=="--authored-terrain";
 if(argc!=1&&!authored)return 2;
 std::shared_ptr<const TriangleSupportSurface> terrain;
 if(authored) {
  const auto directory=std::filesystem::path(HORDE_RT_SOURCE_DIR)/"assets/models/world/runtime/chapter-terrain-v1";
  horde::scene::assets::AssetManifest manifest;horde::scene::assets::StaticMeshAsset mesh;std::string error;
  if(!horde::scene::assets::AssetManifest::Load(directory/"asset.manifest.json",manifest,error)||
     !horde::scene::assets::StaticMeshAsset::Load(directory/"chapter-terrain-lod0.runtime.glb",manifest,mesh,error)||
     !(terrain=horde::scene::PrepareChapterTerrainSurface(mesh,error))) {std::cerr<<error<<'\n';return 1;}
 }
 int cases=0,failures=0;
 auto check=[&](bool b,const char* m){++cases;if(!b){++failures;std::cerr<<m<<'\n';}};
 {
  auto config=ProductionGameSimulationConfig();
  config.developmentRescueJourney=true;config.playerStartX=-22;config.playerStartZ=-15.2f;
  GameSimulation leased(config);
  const auto initial=leased.Snapshot();
  StaticDetailLease lease{41,initial.worldRoute.generation,-27.5f,-16.6f,-16.55f,-13.85f,false};
  auto bad=lease;bad.worldGeneration++;
  check(!leased.HoldStaticDetailLease(bad),"stale world cannot acquire a spatial resource lease");
  bad=lease;bad.minX=bad.maxX;
  check(!leased.HoldStaticDetailLease(bad),"degenerate lease cannot restrict traversal");
  check(leased.HoldStaticDetailLease(lease) && !leased.HoldStaticDetailLease(lease),
        "only one current renderer lease can own the safe-side gate");
  InputSnapshot move;move.damageEnabled=false;move.hasAuthoritativePlayerPose=true;
  move.authoritativePlayerX=-40;move.authoritativePlayerZ=-15.2f;move.yawRadians=1.2f;
  leased.StepFixed(move);
  check(leased.Snapshot().playerX==initial.playerX && leased.Snapshot().staticDetailLease.restoreRequested &&
        leased.Snapshot().playerYawRadians==move.yawRadians,
        "discontinuous movement requests restore and keeps translation safe while look remains responsive");
  check(!leased.ReleaseStaticDetailLease(40),"stale release cannot remove the safe-side gate");
  const auto beforeReset=leased.Snapshot();leased.ResetRoute();
  check(leased.Snapshot().worldRoute.generation==beforeReset.worldRoute.generation &&
        leased.Snapshot().staticDetailLease.restoreRequested,"reset defers until complete renderer ownership returns");
  check(leased.ReleaseStaticDetailLease(41) && !leased.Snapshot().staticDetailLease.resourceGeneration &&
        leased.Snapshot().worldRoute.generation==beforeReset.worldRoute.generation,
        "completed release cannot mutate gameplay from a rendering callback");
  move.hasAuthoritativePlayerPose=false;leased.StepFixed(move);
  check(leased.Snapshot().worldRoute.generation>beforeReset.worldRoute.generation,
        "deferred reset executes once on the next simulation turn");
  check(!leased.ReleaseStaticDetailLease(41),"a consumed lease cannot release a later owner");
 }
 {
  using horde::gameplay::validation::ChapterRouteMeasurementClock;
  ChapterRouteMeasurementClock clock;
  check(clock.Consume(0)==0 && clock.Consume(.01)==0,
        "fast render frames must not invent a60Hz simulation tick");
  check(clock.Consume(.051)==3 && clock.ConsumedTicks()==3,
        "wall-clock route must consume every complete authoritative tick");
  check(clock.Consume(.051)==0,"same time must not repeat an input/step");
  check(clock.Consume(.251)==4 && clock.PendingTicks()==8,
        "slow frame must retain debt behind the four-tick per-render bound");
  check(clock.Consume(.251)==4 && clock.Consume(.251)==4 && clock.ConsumedTicks()==15,
        "bounded catch-up must neither skip nor duplicate route actions");
  check(!clock.Consume(.2) && !clock.Consume(-1) &&
        !clock.Consume(std::numeric_limits<double>::infinity()) &&
        !clock.Consume(std::numeric_limits<double>::quiet_NaN()) && !clock.Consume(150.01),
        "clock rewind, invalid time and the unchanged150-second boundary fail closed");
  check(clock.Consume(.267)==1 && clock.ConsumedTicks()==16,
        "rejected clock samples must leave the valid cadence intact");
  ChapterRouteMeasurementClock reset;
  check(reset.Consume(0)==0 && reset.ConsumedTicks()==0 && reset.PendingTicks()==0,
        "new measurement must have no inherited time or input debt");
  using horde::gameplay::validation::MeasureChapterRouteHostPhases;
  const std::array<std::uint64_t,7> marks{{100,110,120,720,730,740,750}};
  const auto host=MeasureChapterRouteHostPhases(marks,5,true);
  check(host && host->available && host->nanoseconds[2]==600 &&
        std::accumulate(host->nanoseconds.begin(),host->nanoseconds.end(),std::uint64_t{})==650 &&
        host->previousEvidenceNanoseconds==5,
        "host clocks locate an injected message delay and exactly partition the present interval");
  auto rewind=marks;rewind[3]=119;
  check(!MeasureChapterRouteHostPhases(rewind,5,true) && !MeasureChapterRouteHostPhases(marks,11,true),
        "rewound clocks and evidence outside its owning tail fail closed");
  const auto first=MeasureChapterRouteHostPhases({},0,false);
  check(first && !first->available && first->nanoseconds==std::array<std::uint64_t,6>{},
        "first present cannot borrow an earlier bootstrap interval");
 }
 if(authored) {
  unsigned chapterSelections=0;
  for(const auto& checkpoint:kDevelopmentCheckpoints) {
   const bool expected=checkpoint.id==192 || checkpoint.name.starts_with("chapter-");
   check(DevelopmentCheckpointRequiresAuthoredChapter(checkpoint)==expected,
         "canonical chapter selections must request their actual authored GPU scene");
   chapterSelections+=expected;
  }
  check(chapterSelections==28,"live journey and all27 chapter views retain authored-scene admission");
  const auto* landing=FindDevelopmentCheckpoint("chapter-landing-north");
  GameSimulation fresh(ProductionGameSimulationConfig());
  check(!StageDevelopmentCheckpointForScene(fresh,*landing,[]{}),
        "missing renderer readiness must reject the landing capture instead of naming a start frame ready");
  const auto staleGeneration=fresh.Snapshot().worldRoute.generation;
  check(!StageDevelopmentCheckpointForScene(fresh,*landing,[&]{
    fresh.PublishChapterTerrain({staleGeneration,WorldZoneId::TombExterior},terrain);
   }),"stale scene generation must not admit a frozen landing");
  unsigned publications=0;
  const bool staged=StageDevelopmentCheckpointForScene(fresh,*landing,[&]{
   ++publications;
   for(std::size_t zone=1;zone<kWorldZones.size();++zone)
    check(fresh.PublishChapterTerrain({fresh.Snapshot().worldRoute.generation,
     static_cast<WorldZoneId>(zone)},terrain),"ready exported terrain must publish against the current selected scene");
  });
  check(staged && publications==1,"fresh production profile must stage the real landing only after current readiness");
  if(staged) {
   const auto& snapshot=fresh.Snapshot();
   check(snapshot.developmentRescueJourney && snapshot.rescue.exteriorSide && snapshot.tickIndex>400 &&
         snapshot.interaction.heldLightKind==interactions::HeldLightKind::RewardLantern &&
         std::hypot(snapshot.playerX-landing->cameraX,snapshot.playerZ-landing->cameraZ)<.02f,
         "admitted landing must prove actual journey, camera, progression and equipment rather than a checkpoint label");
  }
 }
 if(authored) {
  auto captureConfig=ProductionGameSimulationConfig();
  captureConfig.developmentRescueJourney=true;
  captureConfig.authoredChapterTerrain=true;
  GameSimulation capture(captureConfig);
  for(std::size_t zone=1;zone<kWorldZones.size();++zone)
   check(capture.PublishChapterTerrain({capture.Snapshot().worldRoute.generation,static_cast<WorldZoneId>(zone)},terrain),
         "Kit capture terrain preparation failed");
  const auto* reunion=FindDevelopmentCheckpoint("chapter-kit-reunion");
  check(reunion && StageDevelopmentCheckpointSimulation(capture,*reunion),"Kit capture native ascent failed");
  const auto& state=capture.Snapshot();
  check(state.chapterCompanion.grounded && !state.chapterCompanion.walking &&
        state.lich.deathDissolveProgress==1 && state.rescue.exteriorSide && !state.rescue.equipmentStowed,
        "Kit reunion requires retired Keeper, grounded companion and completed physical ascent");
  check(state.interaction.heldLightPose==interactions::HeldLightPose::Low,
        "First reunion capture must preserve fresh player-controlled lantern raise");
  {
   using namespace horde::gameplay::validation;
   GameSimulation moving(captureConfig);
   for(std::size_t zone=1;zone<kWorldZones.size();++zone)
    moving.PublishChapterTerrain({moving.Snapshot().worldRoute.generation,static_cast<WorldZoneId>(zone)},terrain);
   check(StageDevelopmentCheckpointSimulation(moving,*reunion),"moving probe must start from an actual admitted ascent");
   ChapterRoundTripProbe probe;
   const bool started=probe.Start(moving.Snapshot());
   check(started,"rendered two-way probe must accept the native grounded reunion");
   if(started) {
    const auto paused=moving.Snapshot();
    for(unsigned i=0;i<12;++i) {
     const auto command=probe.Next(moving.Snapshot(),true);
     moving.AdvanceFrame(command,.25,moving.Snapshot().inputPublicationSequence+1);
     check(command.paused && command.moveForward==0 && !command.runHeld &&
       moving.Snapshot().playerX==paused.playerX && probe.Phase()==ChapterProbePhase::Landing,
       "paused probe must retain its phase without advancing input or moving the player");
    }
    unsigned frames=0,raises=0;bool walkingReunion=false;std::uint64_t lastRaise=moving.Snapshot().lastConsumedToggleHeldLightPoseSequence;
    std::array<bool,static_cast<unsigned>(ChapterProbePhase::Failed)+1> seen{};
    while(frames++<10000 && probe.Phase()!=ChapterProbePhase::Complete && probe.Phase()!=ChapterProbePhase::Failed) {
     seen[static_cast<unsigned>(probe.Phase())]=true;
     walkingReunion|=probe.Phase()==ChapterProbePhase::Raise && moving.Snapshot().chapterCompanion.walking &&
        moving.Snapshot().chapterCompanion.animationSeconds>=.3f;
     const auto command=probe.Next(moving.Snapshot());
     check(!command.hasAuthoritativePlayerPose && !command.commands.attack && !command.commands.parry &&
       !command.commands.dodge,"two-way probe must use normal movement/context input without pose injection or combat edges");
     if(moving.Snapshot().rescue.equipmentStowed)
      check(command.moveForward==0 && !command.runHeld,"probe movement must not compete with the rope motor");
     if(command.commands.toggleHeldLightPose>lastRaise) {++raises;lastRaise=command.commands.toggleHeldLightPose;}
     moving.StepFixed(command,1.f/60.f,moving.Snapshot().inputPublicationSequence+1);
     // Host-only acknowledgement; native probe must use its actual submitted present.
     if(frames%4==0)moving.AcknowledgeChapterPresentation(moving.Snapshot().tickIndex,moving.Snapshot().chapterDialogue.generation);
    }
    check(probe.Phase()==ChapterProbePhase::Complete && frames<10000,"actual terrain two-way probe stalled or failed");
    check(walkingReunion,"probe did not inspect the available walking clip after a real fresh raise");
    const auto& s=moving.Snapshot();
    check(raises==1 && s.chapterDialogue.proofAccepted && s.rescue.sawDescent && s.rescue.firstAscentCompleted &&
      s.rescue.exteriorSide && !s.rescue.equipmentStowed && s.rescue.claimCount==1 && s.rescue.deploymentCount==1,
      "probe must prove one fresh raise, lookout/return, real descent and repeat ascent without duplicate ownership");
    for(unsigned phase=0;phase<static_cast<unsigned>(ChapterProbePhase::Complete);++phase)
        check(seen[phase],"probe skipped a required moving journey phase");
    std::cout<<"native-terrain moving probe frames="<<frames<<'\n';
   }
   for(unsigned ticksPerFrame:{1u,3u,4u,6u,5u}) {
    const auto selected=ticksPerFrame==5?MotionScenario::ChapterStreamingCycle:MotionScenario::ChapterRoundTrip;
    GameSimulation phone(captureConfig);
    for(std::size_t zone=1;zone<kWorldZones.size();++zone)
     phone.PublishChapterTerrain({phone.Snapshot().worldRoute.generation,static_cast<WorldZoneId>(zone)},terrain);
    check(StageDevelopmentCheckpointSimulation(phone,*reunion),"phone motion requires existing admitted reunion");
    MotionEvidenceScenario motion;
    std::uint64_t now=1'000'000'000ull;unsigned frame=0;
    const auto generation=phone.Snapshot().worldRoute.generation,tick=phone.Snapshot().tickIndex;
    check(motion.Begin(selected,phone,now) &&
          phone.Snapshot().worldRoute.generation==generation && phone.Snapshot().tickIndex==tick,
          "phone route admission must not reseed or mutate the existing rendered chapter");
    for(unsigned n=0;n<3;++n) {
     const auto command=motion.BuildInput(phone.Snapshot(),{},now,false);
     check(command.paused && command.moveForward==0,"phone route cannot move before real current presentation");
    }
    InputSnapshot published;
    while(++frame<8000 && !motion.Complete() && !motion.Failed()) {
     now+=ticksPerFrame*16'666'667ull;
     const auto command=motion.BuildInput(phone.Snapshot(),published,now,true);
     check(!command.hasAuthoritativePlayerPose && !command.commands.attack && !command.commands.dodge,
           "phone chapter measurement must use ordinary movement/context input");
     phone.AdvanceFrame(command,static_cast<double>(ticksPerFrame)/60,frame);
     motion.ObserveAdvance(phone.Snapshot(),phone.Events().Events());phone.ClearEvents();
     phone.AcknowledgeChapterPresentation(phone.Snapshot().tickIndex,phone.Snapshot().chapterDialogue.generation);
    }
    std::cout<<"phone chapter mode="<<MotionScenarioName(selected)<<" ticks/frame="<<ticksPerFrame<<" frames="<<frame<<" stage="<<MotionStageName(motion.Stage())
             <<" phase="<<ChapterProbePhaseName(motion.ChapterPhase())<<" failure="<<motion.Failure()<<'\n';
    check(motion.Complete() && phone.Snapshot().rescue.sawDescent && phone.Snapshot().rescue.exteriorSide,
          "phone wall-clock multi-tick route must complete descent, passage and return ascent");
    MotionEvidenceScenario invalidMotion;
    GameSimulation wrong(captureConfig);
    check(!invalidMotion.Begin(MotionScenario::ChapterRoundTrip,wrong,now),"unready phone chapter cannot be fabricated by measurement");
   }
   ChapterRoundTripProbe invalid;
   auto bad=state;bad.rescue.equipmentStowed=true;
   check(!invalid.Start(bad),"probe cannot start from a traversing or unready side");
   bad=state;bad.interaction.heldLightPose=interactions::HeldLightPose::High;
   check(!invalid.Start(bad),"probe cannot count an already raised lantern as fresh input");
   bad=state;bad.worldRoute.readiness[2]=ZoneReadiness::Failed;
   check(!invalid.Start(bad),"failed destination cannot start the moving probe");
   ChapterRoundTripProbe stale;
   check(stale.Start(state),"valid stale-generation fixture failed to start");
   bad=state;++bad.worldRoute.generation;
   const auto rejected=stale.Next(bad);
   check(stale.Phase()==ChapterProbePhase::Failed && rejected.paused && rejected.moveForward==0 &&
     !rejected.commands.interact,"stale reconstruction must fail the probe before issuing movement or interaction");
  }
  for(const auto name:{"chapter-stake-ascent","chapter-stake-pullup","chapter-stake-descent","chapter-stake-repeat-ascent"}) {
   GameSimulation motorCapture(captureConfig);
   for(std::size_t zone=1;zone<kWorldZones.size();++zone)
    check(motorCapture.PublishChapterTerrain({motorCapture.Snapshot().worldRoute.generation,
          static_cast<WorldZoneId>(zone)},terrain),"stake capture requires actual native destination terrain readiness");
   const auto* checkpoint=FindDevelopmentCheckpoint(name);
   check(checkpoint && StageDevelopmentCheckpointSimulation(motorCapture,*checkpoint),
         "ground stake inspection must stage the real repeatable fixed-step traversal");
   const auto& s=motorCapture.Snapshot();
   check(s.rescue.equipmentStowed && s.rescue.ropeHandsActive && s.rescue.claimCount==1 &&
         s.rescue.deploymentCount==1 && s.chestReward.phase==interactions::ChestRewardPhase::LanternClaimed,
         "frozen stake capture must own real grips and one reward/deployment");
   check(s.playerAnimation.plantedPelvisDropMetres==s.rescue.plantedPelvisDropMetres,
         "zero-delta capture lost the published traversal stance");
   using namespace horde::platform::windows;
   check(HasRescueBodyStowCaptureWitness(s),"actual loaded rope capture lacks authoritative body-stow witness");
   const auto policy=ClaimedRewardCapturePolicy(name,IsCaptureSwordFullyStowed(s.heldItems[1]),&s);
   check(policy.requirePlayerPixels && !policy.requireRewardBodyPixels &&
         !policy.requireRewardRingPixels && !policy.requireSwordPixels &&
         !policy.permitsCompleteWallRetraction,"loaded capture must preserve arm visibility without requiring rear lantern in primary view");
   for(unsigned invalid=0;invalid<18u;++invalid) {
    auto bad=s;
    if(invalid==0)bad.rescue.ropeHandsActive=false;
    if(invalid==1)bad.rescue.phase=Phase::UpperSafe;
    if(invalid==2)bad.rescue.equipmentStowed=false;
    if(invalid==3)bad.rescue.claimCount=2u;
    if(invalid==4)bad.rescue.generation=0u;
    if(invalid==5)bad.rescue.grippingHandTargets[0].x+=.01f;
    if(invalid==6)bad.rescue.grippingRopeNodeIndices[1]=12.f;
    if(invalid==7)bad.playerX+=.01f;
    if(invalid==8)bad.lanternPendulum.initialized=false;
    if(invalid==9)bad.heldItems[1].visualStowBlend=.9f;
    if(invalid==10)bad.chestReward.phase=interactions::ChestRewardPhase::LanternAvailable;
    if(invalid==11)bad.heldItems[0].parentMode=items::HeldItemParentMode::HandSocket;
    if(invalid==12)bad.rescue.plantedPelvisDropMetres=std::numeric_limits<float>::quiet_NaN();
    if(invalid==13)bad.rewardLanternWorldFromHinge[12]=std::numeric_limits<float>::infinity();
    if(invalid==14)bad.rescue.ropeNodes[0].y=std::numeric_limits<float>::quiet_NaN();
    if(invalid==15)bad.playerAnimation.plantedPelvisDropMetres+=.01f;
    if(invalid==16)bad.developmentRescueJourney=false;
    if(invalid==17)bad.interaction.heldLightKind=interactions::HeldLightKind::Torch;
    check(!HasRescueBodyStowCaptureWitness(bad),"invalid stow/grip/ownership must not admit rear-lantern capture");
    check(ClaimedRewardCapturePolicy(name,true,&bad).requireRewardBodyPixels,
          "invalid loaded evidence must keep ordinary held-lantern visibility requirements");
   }
   if(checkpoint && checkpoint->rescueTraversalCapture==4u)
    check(s.rescue.firstAscentCompleted && s.rescue.sawDescent && s.rescue.sawLanding,
          "repeat-ascent inspection must execute the first round trip before freezing");
  }
  horde::gameplay::dialogue::CompanionRouteFixture companion;
  for(int tick=0;tick<6000&&companion.State().wait!=horde::gameplay::dialogue::CompanionWait::Lookout;++tick) {
   const auto before=companion.State();
   companion.Step(1.f/60.f,before.x,before.z,true,true,false,terrain.get(),true);
   if(companion.State().wait==horde::gameplay::dialogue::CompanionWait::Obstacle) {
    std::cerr<<"native companion blocked at "<<before.x<<","<<before.y<<","<<before.z
             <<" waypoint="<<before.waypoint<<'\n';break;
   }
  }
  check(companion.State().wait==horde::gameplay::dialogue::CompanionWait::Lookout,
        "native companion could not follow the complete authored route to lookout");
  check(companion.State().grounded,"native companion lost support at lookout");
  // The lookout continues over native terrain beyond the old ribbon end cap.
  // Looking toward Bellwether must not retract the hands against that absent wall.
  const auto lookout=kWorldRoutePoints.back();
  const auto ground=terrain->Query(lookout.x,lookout.z);
  const float fx=std::sin(2.51f),fz=-std::cos(2.51f);
  check(ground.found && ChapterExteriorMovementClear(*terrain,lookout.x,lookout.z,
      lookout.x+fx*2.7f,lookout.z+fz*2.7f,ground.worldY,kPlayerCollisionRadius),
      "lookout fixture must have a clear native ground path ahead");
  const float clearance=items::ComputeRewardLanternForwardClearance(
      lookout.x,lookout.z,fx,fz,false,true,ground.worldY,terrain.get());
  std::cout<<"lookout held-item clearance="<<clearance<<'\n';
  check(clearance>2.69f,"lookout hands retracted by obsolete ribbon support boundary");
  // Continuous full-reach optimization must still reject actual solids/holes.
  for(const auto& tree:kForestTreePlacementContract) {
   const auto under=terrain->Query(tree.x,tree.z);
   check(items::ComputeRewardLanternForwardClearance(tree.x,tree.z,1,0,false,true,
       under.worldY,terrain.get())==0.f,"held-item clearance ignored a real native tree trunk");
  }
  check(items::ComputeRewardLanternForwardClearance(-33.7f,-15.2f,0,1,false,true,
      kUpperSupportWorldY,terrain.get())==0.f,"held-item clearance accepted the open shaft");
  const float shaftClearance=items::ComputeRewardLanternForwardClearance(kExteriorLanding.x,kExteriorLanding.z,
      0,-1,false,true,kUpperSupportWorldY,terrain.get());
  std::cout<<"shaft carry clearance="<<shaftClearance<<'\n';
  // The short solid stake is now in this low carry sweep. Retraction is
  // legitimate there; the adjacent diagonal ray remains fully clear below.
  check(shaftClearance>1.67f && shaftClearance<1.69f,
      "landing carry must stop at the actual stake footprint rather than an absent overhead frame");
  check(items::ComputeRewardLanternForwardClearance(kExteriorLanding.x,kExteriorLanding.z,
      -.35f,-.93675f,false,true,kUpperSupportWorldY,terrain.get())>2.69f,
      "grounded landing carry treats the empty shaft as a full-height wall");
  // Macro terrain must be actual imported triangles in every direction, not
  // merely an enlarged support rectangle or the clamped local planning sheet.
  for(float radius:{25.f,75.f,250.f,750.f,2500.f,9000.f})for(int bearing=0;bearing<8;++bearing) {
   const float angle=bearing*.785398163397f;
   check(terrain->Query(-33.7f+radius*std::cos(angle),-12.8f+radius*std::sin(angle)).found,
         "regional terrain missing on a landing compass bearing");
  }
  for(const auto point:std::array<std::array<float,2>,4>{{{{-71.7f,-313.8f}},{{488.3f,-313.8f}},
                                                                     {{-71.7f,636.2f}},{{488.3f,636.2f}}}})
   check(terrain->Query(point[0],point[1]).found,"connected-region map corner has no native ground");
  const auto lake=terrain->Query(128.3f,476.2f),ridge=terrain->Query(384.3f,-181.8f);
  check(lake.found && lake.worldY < -34.f,"regional lake basin no longer lies below proposed water datum");
  check(ridge.found && ridge.worldY > 40.f,"regional northeast ridge was flattened to the local sheet edge");
  check(terrain->TriangleCount()<=39618u,"regional land exceeded the existing terrain derivative triangle envelope");
 }
 InputSnapshot input;input.damageEnabled=false;
 // The real development launch does not seed missing health, even while paused
 // or after a new-game reset. HUD rendering must consume this published tuple.
 GameSimulation launch(ProductionGameSimulationConfig());
 check(launch.ApplyShowcaseCheckpoint(0),"legacy capture seed unavailable");
 launch.StartLiveDevelopmentRescueJourney();
 InputSnapshot launchInput;launchInput.tutorialEnabled=true;launchInput.tutorialSlowdownEnabled=true;
 check(launch.Snapshot().playerVitals.vitality==3 && launch.Snapshot().playerVitals.maxVitality==3,
       "development journey launch did not start with three hearts");
 launchInput.paused=true;launch.AdvanceFrame(launchInput,.25,1);
 check(launch.Snapshot().playerVitals.vitality==3 && launch.Snapshot().playerVitals.maxVitality==3,
       "paused initial publication lost a heart");
 launchInput.paused=false;
 for(int frame=0;frame<60;++frame)launch.AdvanceFrame(launchInput,1.0/60.0,frame+2);
 check(launch.Snapshot().playerVitals.vitality==3,"idle opening route damaged the player");
 check(launch.Snapshot().combatTeaching.enabled && launch.Snapshot().lich.readableCastPresentation,
       "live chapter startup retained historical capture combat/teaching rules");
 auto grateConfig=ProductionGameSimulationConfig();grateConfig.playerStartX=0.0f;grateConfig.playerStartZ=-8.6f;
 GameSimulation grateLaunch(grateConfig);grateLaunch.StartLiveDevelopmentRescueJourney();
 check(std::abs(grateLaunch.Snapshot().playerZ+8.6f)<.001f,"grate fixture is not on a valid player surface");
 for(int frame=0;frame<30;++frame)grateLaunch.StepFixed(launchInput);
 check(grateLaunch.Snapshot().chapterDialogue.line==horde::gameplay::dialogue::Line::Grate,
       "live chapter startup suppressed the actual grate dialogue trigger");
 GameSimulation frozen(ProductionGameSimulationConfig());
 check(frozen.ApplyShowcaseCheckpoint(0),"frozen capture seed unavailable");
 frozen.SetDevelopmentRescueJourney(true);
 for(int frame=0;frame<30;++frame)frozen.StepFixed(launchInput);
 check(!frozen.Snapshot().combatTeaching.enabled && !frozen.Snapshot().lich.readableCastPresentation,
       "live launch correction changed historical frozen capture rules");
 auto ending=launch.Snapshot();ending.finale.endingPhase=interactions::FinaleEndingPhase::Complete;
 check(ending.EndingPhaseForUi()==interactions::FinaleEndingPhase::Inactive,
       "chapter exposed the legacy Dawn Returns overlay");
 ending.developmentRescueJourney=false;
 check(ending.EndingPhaseForUi()==interactions::FinaleEndingPhase::Complete,
       "ordinary demo lost its accepted ending overlay");
 launchInput.commands.routeReset=1;launch.StepFixed(launchInput);
 check(launch.Snapshot().playerVitals.vitality==3 && launch.Snapshot().playerVitals.maxVitality==3,
       "development journey reset did not restore three hearts");

 GameSimulation baseline,disabled;
 disabled.SetDevelopmentRescueJourney(false);
 for(int i=0;i<60;++i) {baseline.StepFixed(input);disabled.StepFixed(input);}
 check(baseline.Snapshot().playerX==disabled.Snapshot().playerX && baseline.Snapshot().playerSupportWorldY==disabled.Snapshot().playerSupportWorldY,"default production support changed");
 GameSimulationConfig config=ProductionGameSimulationConfig();
 config.playerStartX=kLowerLanding.x;config.playerStartZ=kLowerLanding.z;
 GameSimulation sim(config);sim.SetDevelopmentRescueJourney(true,authored);
 const auto publish=[&](WorldZoneToken token) {return authored?sim.PublishChapterTerrain(token,terrain):sim.PublishWorldZoneReadiness(token,ZoneReadiness::Ready);};
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
 if(authored)check(!sim.PublishWorldZoneReadiness(token,ZoneReadiness::Ready),"readiness admitted without authored support");
 check(publish(token),"current readiness could not be admitted");
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
     sim.StepFixed(input);const auto after=sim.Snapshot();const auto support=authored?
        ChapterExteriorSupport(terrain.get(),after.playerX,after.playerZ):RescueExteriorSupport(after.playerX,after.playerZ);
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
    check(publish({sim.Snapshot().worldRoute.generation,zone}),"current forest host token rejected");
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
   check(!publish(stale),"stale descent reconstruction callback accepted");
   check(publish({sim.Snapshot().worldRoute.generation,WorldZoneId::TombExterior}),"fresh terrain publication rejected after reconstruction");
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
 check(!publish(old),"late reconstruction callback admitted");
 const auto paidRopeBeforeRetry=sim.Snapshot().rescue.ropeNodes;
 sim.RetryEncounter();check(sim.Snapshot().rescue.claimCount==1 && sim.Snapshot().rescue.ropeReady &&
       sim.Snapshot().rescue.ropeNodes==paidRopeBeforeRetry,
       "logical retry lost or replayed the already paid-out rope deployment");
 sim.ResetRoute();check(!sim.Snapshot().rescue.ropeDeployed && sim.Snapshot().rescue.claimCount==0,"new route kept stale deployment");
 if(authored) {
  const WorldZoneToken failed{sim.Snapshot().worldRoute.generation,WorldZoneId::TombExterior};
  check(!sim.PublishChapterTerrain(failed,{}),"empty terrain published");
  check(sim.PublishWorldZoneReadiness(failed,ZoneReadiness::Failed),"failed generation not recorded");
  check(!publish(failed),"late terrain resurrected failed preparation");
  check(!publish(old),"reset accepted old terrain callback");
 }
 auto resident=horde::scene::PrepareDevelopmentWorldGeometry(false,true),staged=horde::scene::PrepareDevelopmentWorldGeometry(true,true);
 horde::scene::AppendRescueJourneyGeometry(resident);horde::scene::AppendRescueJourneyGeometry(staged);
 auto chapter=horde::scene::PrepareDevelopmentWorldGeometry(false,true);
 horde::scene::AppendRescueJourneyGeometry(chapter,false);
 check(chapter.valid && chapter.triangles.size()+horde::scene::kOutdoorEffectMarkerTriangleCount==resident.triangles.size(),
       "authored chapter omits only optional experiment letters, retaining rim/anchor geometry");
 for(std::size_t i=0;i<chapter.triangles.size();++i)check(chapter.triangles[i].points==resident.triangles[i].points &&
     chapter.triangles[i].material==resident.triangles[i].material && chapter.triangles[i].normal==resident.triangles[i].normal,
     "omitting experiment markers changed real rescue geometry");
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
