#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/campaign/LookoutObjective.h"
#include "gameplay/DevelopmentCheckpointSimulation.h"
#include "gameplay/traversal/ChapterTerrainMovement.h"
#include "gameplay/SpatialAudio.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/assets/AssetManifest.h"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>

using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
namespace cp = horde::gameplay::campaign;
namespace dlg = horde::gameplay::dialogue;
namespace tr = horde::gameplay::traversal;
namespace {
int checks=0, failures=0;
void Check(bool result,const char* message) {
    ++checks;
    if(!result) {++failures;std::cerr<<message<<'\n';}
}
cp::Checkpoint Record(bool upper=false) {
    cp::Checkpoint c;
    c.guards={cp::GuardPose{-4.f,-15.2f,.3f},cp::GuardPose{-6.f,-16.f,-.7f}};
    c.dialogue.grateConsumed=true;c.dialogue.rescueConsumed=true;
    if(upper) {
        c.id=cp::CheckpointId::SafeExit;c.zone=WorldZoneId::TombExterior;
        c.position=tr::kExteriorLanding;c.firstAscentCompleted=true;
        c.dialogue.reunionStarted=true;c.dialogue.waitingForRaise=true;
    }
    return c;
}
cp::Checkpoint Endpoint(const TriangleSupportSurface& terrain) {
    auto c=Record(true);c.contentRevision=2;c.id=cp::CheckpointId::ForestEndpoint;
    c.zone=WorldZoneId::Lookout;const auto p=kWorldRoutePoints.back();
    const auto ground=terrain.Query(p.x,p.z);
    Check(ground.found && ground.walkable,"endpoint fixture uses actual imported lookout support");
    c.position={p.x,ground.worldY,p.z};c.yaw=2.51f;c.pitch=-.15f;
    c.dialogue.waitingForRaise=false;c.dialogue.proofAccepted=true;
    c.dialogue.forestNightConsumed=c.dialogue.forestWaystoneConsumed=true;
    c.dialogue.forestClueConsumed=c.dialogue.forestVillageConsumed=true;
    return c;
}
bool SameProgress(const dlg::Progress& a,const dlg::Progress& b) {
    return a.activeLine==b.activeLine && a.continuations==b.continuations &&
        a.continuationCount==b.continuationCount && a.grateConsumed==b.grateConsumed &&
        a.rescueConsumed==b.rescueConsumed && a.reunionStarted==b.reunionStarted &&
        a.waitingForRaise==b.waitingForRaise && a.proofAccepted==b.proofAccepted &&
        a.reminderConsumed==b.reminderConsumed && a.forestNightConsumed==b.forestNightConsumed &&
        a.forestWaystoneConsumed==b.forestWaystoneConsumed && a.forestClueConsumed==b.forestClueConsumed &&
        a.forestVillageConsumed==b.forestVillageConsumed;
}
bool SameRecord(const cp::Checkpoint& a,const cp::Checkpoint& b) {
    return a.schema==b.schema && a.contentRevision==b.contentRevision && a.id==b.id &&
        a.zone==b.zone && a.position==b.position && a.yaw==b.yaw && a.pitch==b.pitch &&
        a.vitality==b.vitality && a.defeatedGuards==b.defeatedGuards &&
        a.keeperDefeated==b.keeperDefeated && a.lanternClaimed==b.lanternClaimed &&
        a.lidOpen==b.lidOpen && a.ropeReady==b.ropeReady && a.night==b.night &&
        a.firstAscentCompleted==b.firstAscentCompleted && a.swordStowed==b.swordStowed &&
        a.lanternHigh==b.lanternHigh && a.torchDropX==b.torchDropX &&
        a.torchDropZ==b.torchDropZ && a.torchDropYaw==b.torchDropYaw &&
        a.torchDropPitch==b.torchDropPitch && a.guards==b.guards && SameProgress(a.dialogue,b.dialogue);
}
bool SameSession(const SimulationSnapshot& a,const SimulationSnapshot& b) {
    return a.tickIndex==b.tickIndex && a.worldRoute.generation==b.worldRoute.generation &&
        a.worldRoute.current==b.worldRoute.current && a.playerX==b.playerX && a.playerZ==b.playerZ &&
        a.playerSupportWorldY==b.playerSupportWorldY && a.playerVitals.vitality==b.playerVitals.vitality &&
        a.playerAlive==b.playerAlive && a.rescue.generation==b.rescue.generation &&
        a.rescue.phase==b.rescue.phase && a.chapterDialogue.generation==b.chapterDialogue.generation &&
        a.chapterDialogue.line==b.chapterDialogue.line && a.chapterCompanion.generation==b.chapterCompanion.generation &&
        a.queuedEventCount==b.queuedEventCount && a.lastConsumedAttackSequence==b.lastConsumedAttackSequence &&
        a.lastConsumedToggleHeldLightPoseSequence==b.lastConsumedToggleHeldLightPoseSequence;
}
bool Restore(GameSimulation& sim,const cp::Checkpoint& c,
             const std::shared_ptr<const TriangleSupportSurface>& terrain,InputSnapshot input={}) {
    const auto before=sim.Snapshot();const auto token=sim.BeginCampaignRestore(c);
    Check(bool(token),"valid logical record begins restore");
    Check(SameSession(before,sim.Snapshot()),"begin restore leaves live session unchanged");
    return token && sim.CommitCampaignRestore(*token,{token->worldGeneration,c.zone},terrain,input);
}
void CheckClean(const SimulationSnapshot& s) {
    Check(s.playerAlive && s.playerGrounded && s.playerVitals.phase==PlayerLifePhase::Alive &&
        s.playerVitals.invulnerabilityRemaining==0 && s.playerVitals.deathHoldRemaining==0,"restore is alive on grounded support");
    Check(s.playerCombat.action==PlayerCombatAction::Idle && !s.dodgeActive && !s.dodgeInvulnerable &&
        !s.runActive && !s.runToggleActive && s.playerMovementSpeedMetresPerSecond==0 &&
        s.fixedStepAccumulatorSeconds==0 && s.eventsEmittedThisFrame==0 && s.queuedEventCount==0,
        "restore has no combat, run, timing or semantic event debt");
    Check(s.lich.phase==LichPhase::Dead && s.lich.deathAnimationComplete &&
        s.lich.deathDissolveProgress==1 && s.lich.roomMistDensityScale==0 &&
        s.skeletonEnemyCount==2 && s.skeletonEnemies[0].dead && s.skeletonEnemies[1].dead &&
        s.skeletonEnemies[0].health==0 && s.skeletonEnemies[1].health==0,
        "restore retains completed Keeper and both defeated guards");
    Check(s.chestReward.phase==interactions::ChestRewardPhase::LanternClaimed &&
        s.interaction.heldLightKind==interactions::HeldLightKind::RewardLantern &&
        s.rescue.ropeReady && s.rescue.lanternClaimed && !s.rescue.equipmentStowed &&
        s.rescue.claimCount==1 && s.rescue.deploymentCount==1,
        "restore retains owned reward and usable deployed rope");
    Check(s.lich.finaleDawnRevealProgress==0 && s.lich.finaleSkylightOpenProgress==1 &&
        s.finale.dawnRevealProgress==0 && s.EndingPhaseForUi()==interactions::FinaleEndingPhase::Inactive,
        "logical chapter restore does not replay the legacy dawn ending");
}
}
int main() {
    const auto directory=std::filesystem::path(HORDE_RT_SOURCE_DIR)/"assets/models/world/runtime/chapter-terrain-v1";
    horde::scene::assets::AssetManifest manifest;horde::scene::assets::StaticMeshAsset mesh;std::string error;
    if(!horde::scene::assets::AssetManifest::Load(directory/"asset.manifest.json",manifest,error) ||
       !horde::scene::assets::StaticMeshAsset::Load(directory/"chapter-terrain-lod0.runtime.glb",manifest,mesh,error)) {
        std::cerr<<"real chapter asset load failed: "<<error<<'\n';return 1;
    }
    const auto terrain=horde::scene::PrepareChapterTerrainSurface(mesh,error);
    if(!terrain) {std::cerr<<"real chapter support preparation failed: "<<error<<'\n';return 1;}
    // The initial records are ordinary validated logical values. Neither side
    // of these round trips imports a development/debug capture checkpoint.
    for(bool upper:{false,true}) for(int vitality:{1,2,3}) for(bool stowed:{false,true}) for(bool high:{false,true}) {
        auto c=Record(upper);c.vitality=vitality;c.swordStowed=stowed;c.lanternHigh=high;
        c.yaw=.37f;c.pitch=-.12f;
        Check(cp::Validate(c),"authored safe-boundary record validates");
        GameSimulation first(ProductionGameSimulationConfig());
        if(!Restore(first,c,terrain)) {Check(false,"fresh production restore commits on actual terrain");continue;}
        CheckClean(first.Snapshot());
        Check(first.Snapshot().playerVitals.vitality==vitality,"restore preserves one, two or three hearts");
        Check(first.Snapshot().worldRoute.current==c.zone && first.Snapshot().rescue.exteriorSide==upper &&
            first.Snapshot().rescue.firstAscentCompleted==c.firstAscentCompleted,"restore selects the saved safe side");
        Check(first.Snapshot().heldItems[1].parentMode==(stowed?items::HeldItemParentMode::BodyStow:items::HeldItemParentMode::HandSocket) &&
            first.Snapshot().interaction.heldLightPose==(high?interactions::HeldLightPose::High:interactions::HeldLightPose::Low),
            "sword socket and lantern pose preserve saved equipment");
        if(upper) Check(first.Snapshot().chapterCompanion.grounded,"safe-exit Kit is grounded on imported terrain");
        const auto captured=first.CaptureCampaignCheckpoint(c.id);
        Check(captured && SameRecord(*captured,c),"actual restored simulation captures the complete logical record");
        Check(!first.CaptureCampaignCheckpoint(upper?cp::CheckpointId::LowerClaimed:cp::CheckpointId::SafeExit),
            "capture cannot substitute the other safe boundary");
        if(!captured) continue;
        GameSimulation second(ProductionGameSimulationConfig());
        Check(Restore(second,*captured,terrain),"captured record restores into a second fresh production simulation");
        const auto again=second.CaptureCampaignCheckpoint(c.id);
        Check(again && SameRecord(*again,*captured),"fresh simulation capture/restore/capture is lossless");
        CheckClean(second.Snapshot());
    }
    {
        auto config=ProductionGameSimulationConfig();
        config.playerStartX=-1.80f;config.playerStartZ=-15.2f;
        config.playerStartYawRadians=-1.57079632679f;
        GameSimulation walking(config);
        InputSnapshot in;in.damageEnabled=false;in.moveForward=1;
        in.yawRadians=config.playerStartYawRadians;in.pitchRadians=.1f;
        for(int tick=0;tick<90;++tick) walking.StepFixed(in);
        const auto drop=walking.Snapshot().torchFailure;
        Check(drop.phase==TorchFailurePhase::Settled && !drop.heldByPlayer &&
            !IsTorchFailureTriggerPosition(drop.droppedX,drop.droppedZ),
            "ordinary walking releases the torch beyond the drench trigger rectangle");
        for(bool upper:{false,true}) {
            auto c=Record(upper);c.torchDropX=drop.droppedX;c.torchDropZ=drop.droppedZ;
            c.torchDropYaw=drop.droppedYawRadians;c.torchDropPitch=drop.droppedViewPitchRadians;
            Check(cp::Validate(c),"ordinary moving torch release is a valid logical anchor");
            GameSimulation loaded(ProductionGameSimulationConfig());
            if(!Restore(loaded,c,terrain)) {Check(false,"moving-release checkpoint restores");continue;}
            const auto settled=loaded.Snapshot().torchFailure;
            Check(settled.triggered && !settled.heldByPlayer && settled.phase==TorchFailurePhase::Settled &&
                settled.flameStrength==0 && settled.fallProgress==1 && settled.leftArmLowerBlend==1 &&
                settled.droppedX==drop.droppedX && settled.droppedZ==drop.droppedZ &&
                settled.droppedYawRadians==drop.droppedYawRadians && settled.droppedViewPitchRadians==drop.droppedViewPitchRadians,
                "restore reconstructs a settled nonemitting torch at the actual moving release anchor");
            const auto captured=loaded.CaptureCampaignCheckpoint(c.id);
            Check(captured && SameRecord(*captured,c),"moving-release anchor survives actual safe-boundary capture");
        }
    }
    {
        GameSimulation sim(ProductionGameSimulationConfig());const auto c=Record();
        const auto token=sim.BeginCampaignRestore(c);InputSnapshot held;
        held.commands.attack=4;held.moveForward=1;
        Check(token && sim.CommitCampaignRestore(*token,{token->worldGeneration,c.zone},terrain,held,100),
            "replacement checkpoint setup commits behind a live input barrier");
        Check(sim.Snapshot().inputPublicationSequence==100 && sim.Snapshot().lastConsumedAttackSequence==4,
            "restore snapshot identifies the coherent current input whose commands it consumed");
        const auto* replacement=FindDevelopmentCheckpoint("chapter-keeper-death-held");
        Check(replacement!=nullptr,"accepted replacement fixture exists");
        if(replacement) {Check(StageDevelopmentCheckpointForScene(sim,*replacement,[&]{
            const auto generation=sim.Snapshot().worldRoute.generation;
            sim.PublishChapterTerrain({generation,WorldZoneId::TombExterior},terrain);
        }),"explicit authored checkpoint replacement stages successfully");
        Check(!sim.Snapshot().paused && sim.Snapshot().playerX==replacement->cameraX &&
            sim.Snapshot().playerZ==replacement->cameraZ && sim.Snapshot().playerYawRadians==replacement->yaw,
            "explicit authored replacement reaches its requested pose without the prior load input barrier");}
    }
    for(bool upper:{false,true}) {
        GameSimulation sim(ProductionGameSimulationConfig());const auto c=Record(upper);
        Check(Restore(sim,c,terrain),"interrupted traversal setup restores a committed safe side");
        const auto safe=sim.CaptureCampaignCheckpoint(c.id);
        const auto destination=upper?WorldZoneId::Dungeon:WorldZoneId::TombExterior;
        const WorldZoneToken ready{sim.Snapshot().worldRoute.generation,destination};
        Check(upper?sim.PublishWorldZoneReadiness(ready,ZoneReadiness::Ready):sim.PublishChapterTerrain(ready,terrain),
            "traversal destination receives its owner readiness publication");
        InputSnapshot in;in.damageEnabled=false;in.yawRadians=c.yaw;in.pitchRadians=c.pitch;
        ++in.commands.interact;sim.StepFixed(in);
        Check(sim.Snapshot().rescue.equipmentStowed,"context interaction begins real ascent or return descent");
        Check(!sim.CaptureCampaignCheckpoint(cp::CheckpointId::LowerClaimed) &&
            !sim.CaptureCampaignCheckpoint(cp::CheckpointId::SafeExit),"mid-rope state cannot become a safe checkpoint");
        Check(safe && Restore(sim,*safe,terrain,in),"interrupted traversal restores the retained committed side");
        Check(sim.Snapshot().rescue.exteriorSide==upper && sim.Snapshot().rescue.firstAscentCompleted==upper,
            "interrupted first ascent returns lower and interrupted return descent returns exterior");
        const WorldZoneToken nextReady{sim.Snapshot().worldRoute.generation,destination};
        Check(upper?sim.PublishWorldZoneReadiness(nextReady,ZoneReadiness::Ready):sim.PublishChapterTerrain(nextReady,terrain),
            "restored traversal prepares the other side again");
        ++in.commands.interact;sim.StepFixed(in);
        for(int tick=0;tick<1800 && sim.Snapshot().rescue.equipmentStowed;++tick) sim.StepFixed(in);
        Check(!sim.Snapshot().rescue.equipmentStowed && sim.Snapshot().rescue.exteriorSide!=upper &&
            sim.Snapshot().playerGrounded && sim.Snapshot().rescue.firstAscentCompleted &&
            sim.Snapshot().rescue.claimCount==1 && sim.Snapshot().rescue.deploymentCount==1,
            "reconstructed rope completes ordinary ascent or descent without duplicating claim/deployment");
    }
    {
        GameSimulation sim(ProductionGameSimulationConfig());auto c=Record();c.swordStowed=false;
        Check(Restore(sim,c,terrain),"drawn-sword checkpoint loads its saved equipment");
        sim.ResetRoute();
        Check(sim.Snapshot().heldItems[1].parentMode==items::HeldItemParentMode::BodyStow,
            "loading drawn equipment preserves the production new-game stowed policy");
    }
    {
        GameSimulation sim(ProductionGameSimulationConfig());const auto c=Record();
        InputSnapshot old;old.damageEnabled=false;old.moveForward=1;old.moveStrafe=1;old.runHeld=true;
        old.yawRadians=2;old.pitchRadians=.2f;old.hasAuthoritativePlayerPose=true;
        old.authoritativePlayerX=5;old.authoritativePlayerZ=5;old.commands.attack=9;
        const auto token=sim.BeginCampaignRestore(c);
        Check(token && sim.CommitCampaignRestore(*token,{token->worldGeneration,c.zone},terrain,old,100),
            "restore consumes coherent input publication identity");
        sim.StepFixed(old,static_cast<float>(FixedStepRunner::kFixedDeltaSeconds),100);
        sim.AdvanceFrame(old,.05,100);
        Check(sim.Snapshot().paused && sim.Snapshot().playerX==c.position.x && sim.Snapshot().playerZ==c.position.z &&
            sim.Snapshot().playerYawRadians==c.yaw && sim.Snapshot().playerPitchRadians==c.pitch,
            "repeated old publication cannot move, teleport or turn restored player");
        auto neutral=old;neutral.moveForward=neutral.moveStrafe=0;neutral.runHeld=false;
        neutral.hasAuthoritativePlayerPose=false;neutral.yawRadians=c.yaw;neutral.pitchRadians=c.pitch;
        sim.AdvanceFrame(neutral,.02,100);
        Check(sim.Snapshot().paused,"neutral controls cannot borrow the stale publication identity");
        sim.AdvanceFrame(neutral,.02,101);
        Check(!sim.Snapshot().paused && sim.Snapshot().playerX==c.position.x && sim.Snapshot().playerZ==c.position.z,
            "fresh neutral rebased publication resumes without movement debt");
        neutral.moveForward=.2f;sim.StepFixed(neutral,static_cast<float>(FixedStepRunner::kFixedDeltaSeconds),102);
        Check(sim.Snapshot().playerMovementSpeedMetresPerSecond>0,"new controls work after the restore input barrier");
    }
    {
        GameSimulation sim(ProductionGameSimulationConfig());const auto c=Record();
        Check(Restore(sim,c,terrain),"capture admission setup restores lower side");
        const auto pending=sim.BeginCampaignRestore(c);
        Check(pending && !sim.CaptureCampaignCheckpoint(c.id),"capture rejects pending restore");
        sim.CancelCampaignRestore();
        const auto generation=sim.Snapshot().worldRoute.generation;
        Check(sim.HoldStaticDetailLease({71,generation,c.position.x-1,c.position.x+1,c.position.z-1,c.position.z+1,false}),
            "capture admission acquires a real lower-side static resource lease");
        Check(!sim.CaptureCampaignCheckpoint(c.id) && !sim.BeginCampaignRestore(c),
            "capture and restore reject a live static resource lease");
        Check(sim.ReleaseStaticDetailLease(71),"capture admission releases its own resource lease");
        InputSnapshot move;move.damageEnabled=false;move.moveForward=.2f;move.yawRadians=c.yaw;
        auto neutral=move;neutral.moveForward=0;sim.StepFixed(neutral);
        sim.StepFixed(move);
        Check(sim.Snapshot().playerMovementSpeedMetresPerSecond>0 && !sim.CaptureCampaignCheckpoint(c.id),
            "capture rejects live movement even near the authored safe landing");
        sim.InvalidateWorldZoneReadiness();
        Check(!sim.CaptureCampaignCheckpoint(c.id),"invalidated terrain readiness cannot capture");
    }
    {
        GameSimulation sim(ProductionGameSimulationConfig());const auto c=Record(true);
        const auto before=sim.Snapshot();auto token=sim.BeginCampaignRestore(c);
        Check(bool(token),"readiness rejection setup begins");
        if(token) {
            const auto reject=[&](cp::RestoreToken request,WorldZoneToken ready,std::shared_ptr<const TriangleSupportSurface> surface) {
                Check(!sim.CommitCampaignRestore(request,ready,std::move(surface),{}),"wrong readiness/load identity or support rejects commit");
                Check(SameSession(before,sim.Snapshot()),"failed preparation preserves the live session");
            };
            auto stale=*token;++stale.request;reject(stale,{token->worldGeneration,c.zone},terrain);
            reject(*token,{token->worldGeneration-1,c.zone},terrain);
            reject(*token,{token->worldGeneration,WorldZoneId::Dungeon},terrain);
            reject(*token,{token->worldGeneration,c.zone},nullptr);
            const std::array<TriangleSupportSurface::Triangle,2> flat{{
                {{{{-100,7,-100},{-100,7,100},{100,7,100}}}},
                {{{{-100,7,-100},{100,7,100},{100,7,-100}}}}
            }};
            const auto wrongHeight=TriangleSupportSurface::Build(flat,error);
            Check(bool(wrongHeight),"wrong-support test surface builds");
            reject(*token,{token->worldGeneration,c.zone},wrongHeight);
            Check(sim.CommitCampaignRestore(*token,{token->worldGeneration,c.zone},terrain,{}),
                "rejected preparation leaves the valid pending restore usable");
            const auto committed=sim.Snapshot();
            Check(!sim.CommitCampaignRestore(*token,{token->worldGeneration,c.zone},terrain,{}) && SameSession(committed,sim.Snapshot()),
                "completed load token cannot apply twice");
        }
        const auto old=sim.BeginCampaignRestore(c),newer=sim.BeginCampaignRestore(Record());
        Check(old && newer && old->request!=newer->request,"replacement restore gets a new load identity");
        if(old&&newer) {
            const auto live=sim.Snapshot();
            Check(!sim.CommitCampaignRestore(*old,{old->worldGeneration,c.zone},terrain,{}) && SameSession(live,sim.Snapshot()),
                "replaced load request cannot mutate the session");
            Check(!sim.CommitCampaignRestore(*newer,{old->worldGeneration,WorldZoneId::Dungeon},terrain,{}) && SameSession(live,sim.Snapshot()),
                "new request cannot borrow a replaced request readiness generation");
            Check(sim.CommitCampaignRestore(*newer,{newer->worldGeneration,WorldZoneId::Dungeon},terrain,{}),
                "replacement lower-side restore remains usable");
        }
        auto resetToken=sim.BeginCampaignRestore(c);sim.ResetRoute();const auto reset=sim.Snapshot();
        Check(resetToken && !sim.CommitCampaignRestore(*resetToken,{resetToken->worldGeneration,c.zone},terrain,{}) && SameSession(reset,sim.Snapshot()),
            "route reset cancels pending restore and stale readiness");
        const auto cancel=sim.BeginCampaignRestore(c);sim.CancelCampaignRestore();
        Check(cancel && !sim.CommitCampaignRestore(*cancel,{cancel->worldGeneration,c.zone},terrain,{}),"explicit cancellation rejects late commit");
    }
    {
        GameSimulation sim(ProductionGameSimulationConfig());const auto good=Record();
        const auto pending=sim.BeginCampaignRestore(good);const auto before=sim.Snapshot();
        const auto reject=[&](const cp::Checkpoint& bad) {
            Check(!cp::Validate(bad) && !sim.BeginCampaignRestore(bad),"malformed record is rejected before preparation");
            Check(SameSession(before,sim.Snapshot()),"malformed record does not mutate live state");
        };
        auto bad=good;bad.schema=2;reject(bad);bad=good;bad.contentRevision=2;reject(bad);
        bad=good;bad.id=static_cast<cp::CheckpointId>(9);reject(bad);
        bad=good;bad.zone=WorldZoneId::ForestApproach;reject(bad);
        for(int vitality:{0,4,-1}) {bad=good;bad.vitality=vitality;reject(bad);}
        bad=good;bad.defeatedGuards=1;reject(bad);bad=good;bad.keeperDefeated=false;reject(bad);
        bad=good;bad.lanternClaimed=false;reject(bad);bad=good;bad.lidOpen=false;reject(bad);
        bad=good;bad.ropeReady=false;reject(bad);bad=good;bad.night=false;reject(bad);
        bad=Record(true);bad.firstAscentCompleted=false;reject(bad);
        bad=good;bad.dialogue=Record(true).dialogue;reject(bad);
        bad=good;bad.position.z+=.2f;reject(bad);bad=good;bad.position.y+=.1f;reject(bad);
        bad=good;bad.pitch=.5f;reject(bad);bad=good;bad.guards[0].x=-100;reject(bad);
        bad=good;bad.torchDropX=100;reject(bad);
        for(float value:{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
            bad=good;bad.position.x=value;reject(bad);bad=good;bad.position.y=value;reject(bad);
            bad=good;bad.position.z=value;reject(bad);bad=good;bad.yaw=value;reject(bad);
            bad=good;bad.pitch=value;reject(bad);bad=good;bad.guards[1].yaw=value;reject(bad);
            bad=good;bad.guards[0].z=value;reject(bad);bad=good;bad.torchDropPitch=value;reject(bad);
        }
        bad=good;bad.dialogue.continuationCount=9;reject(bad);
        Check(pending && sim.CommitCampaignRestore(*pending,{pending->worldGeneration,good.zone},terrain,{}),
            "invalid replacement requests do not cancel a validated pending restore");
    }
    {
        // Interrupted rescue speech must restart and retain the rope response.
        auto c=Record();c.dialogue.activeLine=dlg::Line::RescueFound;
        c.dialogue.continuations[0]=dlg::Line::RescueRope;c.dialogue.continuationCount=1;
        GameSimulation sim(ProductionGameSimulationConfig());
        Check(Restore(sim,c,terrain),"interrupted rescue logical speech restores");
        const auto captured=sim.CaptureCampaignCheckpoint(c.id);
        Check(captured && SameProgress(captured->dialogue,c.dialogue),"unfinished rescue retains its continuation queue");
        InputSnapshot in;in.damageEnabled=false;in.yawRadians=c.yaw;in.pitchRadians=c.pitch;
        ++in.commands.dialogueSkip;sim.StepFixed(in);
        Check(sim.Snapshot().chapterDialogue.line==dlg::Line::RescueRope,"restored rescue speech continues with rope instruction");
    }
    {
        auto c=Record(true);c.dialogue.activeLine=dlg::Line::ReunionQuestion;
        GameSimulation sim(ProductionGameSimulationConfig());
        Check(Restore(sim,c,terrain),"active Kit speech restores with actual imported terrain/actor");
        const auto loaded=sim.Snapshot();
        const auto anchored=[](const SimulationSnapshot& s) {
            const auto& a=s.chapterDialogue;const auto& kit=s.chapterCompanion;
            return kit.grounded && a.source.x==kit.x && a.source.z==kit.z &&
                a.source.y==kit.y+dlg::kCompanionSpeechHeight && a.listener.x==s.playerX &&
                a.listener.z==s.playerZ && a.listenerYaw==s.playerYawRadians;
        };
        Check(loaded.paused&&anchored(loaded),"first paused restore snapshot binds Kit rather than obsolete reunion point");
        const auto gains=[&](dlg::Tuple source) {
            const auto& a=loaded.chapterDialogue;
            return CalculateSpatialAudio({source.x,source.z,.36f,1,14,source.y},
                {a.listener.x,a.listener.z,a.listenerYaw,a.listener.y},SpatialAudioDomain::ExteriorRoute);
        };
        const auto before=gains({-32,3.6f,-10}),after=gains(loaded.chapterDialogue.source);
        Check(std::abs(after.distance-std::hypot(loaded.chapterCompanion.x-loaded.playerX,
            loaded.chapterCompanion.z-loaded.playerZ))<.00001f && !after.obstructed,
            "restored Kit distance comes from the actual actor even when it is farther than the old point");
        std::cout<<"Kit reunion anchor: listener=("<<loaded.playerX<<','<<loaded.playerZ<<") actor=("
            <<loaded.chapterCompanion.x<<','<<loaded.chapterCompanion.z<<") oldDistance="<<before.distance
            <<" newDistance="<<after.distance<<" oldLR="<<before.left<<','<<before.right
            <<" newLR="<<after.left<<','<<after.right<<'\n';
        InputSnapshot in;in.damageEnabled=false;in.yawRadians=c.yaw;in.pitchRadians=c.pitch;
        sim.StepFixed(in); // release restored input barrier through ordinary neutral input
        const auto started=sim.Snapshot();
        in.moveStrafe=.5f;in.yawRadians+=.4f;
        for(int tick=0;tick<15;++tick) {
            sim.StepFixed(in);
            Check(anchored(sim.Snapshot())&&sim.Snapshot().chapterDialogue.generation==started.chapterDialogue.generation,
                "ordinary live movement/turn updates Kit listener and source in the same snapshot without restarting");
        }
        const auto moved=sim.Snapshot();
        Check(moved.playerX!=started.playerX||moved.playerZ!=started.playerZ,"Kit live anchor check includes actual collision-resolved movement");
        Check(moved.chapterDialogue.elapsed>started.chapterDialogue.elapsed,"spatial changes do not reset the line clock");
        in.paused=true;sim.StepFixed(in);
        const auto paused=sim.Snapshot();
        Check(paused.chapterDialogue.elapsed==moved.chapterDialogue.elapsed &&
            paused.chapterDialogue.generation==moved.chapterDialogue.generation,
            "pause preserves active Kit cursor identity after movement");
        const auto captured=sim.CaptureAuthoredCampaignBoundary(c.id);
        Check(captured&&captured->dialogue.activeLine==dlg::Line::ReunionQuestion,
            "Kit spatial anchors do not alter logical checkpoint speech semantics");
        // Continue from the isolated restored landing with collision-resolved
        // input, not a relocation. This is a seeded post-Keeper host fixture,
        // not natural victory/ascent, endpoint playback or owner listening.
        Check(Restore(sim,c,terrain),"nearby speech comparison independently restores the canonical landing");
        // Restore admits only the destination zone. This CPU fixture already
        // loaded the full chapter terrain above; publish that owner's next-zone
        // readiness explicitly, as the scene adapter does. No story flag changes.
        Check(sim.Snapshot().worldRoute.readiness[static_cast<unsigned>(WorldZoneId::ForestApproach)]==ZoneReadiness::Unprepared,
            "safe-exit restore does not fabricate next-zone resource readiness");
        Check(sim.PublishWorldZoneReadiness({sim.Snapshot().worldRoute.generation,WorldZoneId::ForestApproach},ZoneReadiness::Ready),
            "nearby dialogue CPU owner admits its actual already-loaded chapter terrain");
        in={};in.damageEnabled=false;in.yawRadians=c.yaw;in.pitchRadians=c.pitch;
        sim.StepFixed(in);
        const auto reunion=kWorldRoutePoints[3];
        bool reached=false;
        for(int tick=0;tick<600;++tick) {
            const auto& s=sim.Snapshot();
            const float dx=reunion.x-s.playerX,dz=reunion.z-s.playerZ;
            const float distance=std::hypot(dx,dz);
            if(distance<.008f) {reached=true;break;}
            in.yawRadians=std::atan2(dx,-dz);in.moveForward=std::min(1.0f,distance/.055f);
            sim.StepFixed(in);
        }
        Check(reached,"Kit spatial fixture reaches actual reunion with ordinary collision-resolved movement");
        in.moveForward=0;in.runHeld=false;
        for(int tick=0;tick<600&&sim.Snapshot().chapterDialogue.line!=dlg::Line::ReunionHint;++tick)sim.StepFixed(in);
        const auto near=sim.Snapshot();
        Check(near.chapterDialogue.line==dlg::Line::ReunionHint&&anchored(near),
            "actual logical reminder at reunion starts with current Kit/listener positions");
        const auto mix=[&](dlg::Tuple source) {
            const auto& a=near.chapterDialogue;
            return CalculateSpatialAudio({source.x,source.z,.36f,1,14,source.y},
                {a.listener.x,a.listener.z,a.listenerYaw,a.listener.y},SpatialAudioDomain::ExteriorRoute);
        };
        const auto oldNear=mix({-32,3.6f,-10}),newNear=mix(near.chapterDialogue.source);
        Check(newNear.distance<1.01f&&oldNear.distance>5 && newNear.left+newNear.right>oldNear.left+oldNear.right,
            "reunion reminder no longer attenuates a one-metre Kit as an obsolete five-metre point");
        std::cout<<"Kit nearby reminder: listener=("<<near.playerX<<','<<near.playerZ<<") actor=("
            <<near.chapterCompanion.x<<','<<near.chapterCompanion.z<<") oldDistance="<<oldNear.distance
            <<" newDistance="<<newNear.distance<<" oldLR="<<oldNear.left<<','<<oldNear.right
            <<" newLR="<<newNear.left<<','<<newNear.right<<'\n';
    }
    {
        auto c=Record(true);c.lanternHigh=true;
        InputSnapshot in;in.damageEnabled=false;in.commands.attack=41;in.commands.parry=42;
        in.commands.dodge=43;in.commands.interact=44;in.commands.toggleHeldLightPose=45;
        in.commands.runToggle=46;in.commands.dialogueSkip=47;in.commands.routeReset=48;in.commands.retry=49;
        in.moveForward=1;in.moveStrafe=-1;in.runHeld=true;
        RecordCombatInputEdge(in,CombatInputEdgeKind::Attack,100);
        in.hasAuthoritativePlayerPose=true;in.authoritativePlayerX=5;in.authoritativePlayerZ=5;
        GameSimulation sim(ProductionGameSimulationConfig());
        Check(Restore(sim,c,terrain,in),"pending proof restores while consuming published input floors");
        const auto loaded=sim.Snapshot();CheckClean(loaded);
        Check(loaded.inputMoveForward==0 && loaded.inputMoveStrafe==0,"restore clears inherited continuous movement");
        sim.StepFixed(in);
        Check(sim.Snapshot().paused && sim.Snapshot().playerX==c.position.x && sim.Snapshot().playerZ==c.position.z,
            "unrebased direct-step input stays behind the restore barrier");
        Check(!sim.AcknowledgeChapterPresentation(loaded.tickIndex-1,loaded.chapterDialogue.generation),"old presentation tick cannot acknowledge restored proof");
        in.moveForward=in.moveStrafe=0;in.runHeld=false;in.yawRadians=c.yaw;in.pitchRadians=c.pitch;
        in.hasAuthoritativePlayerPose=false;
        sim.StepFixed(in);CheckClean(sim.Snapshot());
        Check(sim.Snapshot().chapterDialogue.waitingForRaise && !sim.Snapshot().chapterDialogue.proofAccepted,
            "old attack, interaction, skip, run, retry and raise edges do not replay after restore");
        auto shown=sim.Snapshot();
        Check(sim.AcknowledgeChapterPresentation(shown.tickIndex,shown.chapterDialogue.generation),"current high pose can be presented");
        sim.StepFixed(in);
        Check(!sim.Snapshot().chapterDialogue.proofAccepted,"presenting a saved raised pose alone cannot accept proof");
        ++in.commands.toggleHeldLightPose;sim.StepFixed(in); // lower
        const int poseTicks=static_cast<int>(std::ceil(interactions::kHeldLightPoseTransitionSeconds*60))+1;
        for(int tick=0;tick<poseTicks;++tick) sim.StepFixed(in);
        Check(sim.Snapshot().interaction.heldLightPose==interactions::HeldLightPose::Low,
            "saved high lantern completes ordinary lowering before a fresh raise");
        ++in.commands.toggleHeldLightPose;sim.StepFixed(in); // fresh manual raise
        for(int tick=0;tick<poseTicks;++tick) sim.StepFixed(in);
        Check(!sim.Snapshot().chapterDialogue.proofAccepted,"fresh manual raise still needs current presentation");
        shown=sim.Snapshot();
        Check(sim.AcknowledgeChapterPresentation(shown.tickIndex,shown.chapterDialogue.generation),"fresh raised pose accepts matching presentation identity");
        sim.StepFixed(in);
        Check(sim.Snapshot().chapterDialogue.proofAccepted && sim.Snapshot().chapterDialogue.line==dlg::Line::ReunionProof,
            "fresh manual raise plus presentation accepts restored pending proof");
    }
    {
        auto c=Record(true);c.dialogue.waitingForRaise=false;c.dialogue.proofAccepted=true;
        c.dialogue.activeLine=dlg::Line::ReunionProof;c.dialogue.continuations[0]=dlg::Line::ReunionFirstPiece;
        c.dialogue.continuations[1]=dlg::Line::ReunionDepart;c.dialogue.continuationCount=2;
        GameSimulation sim(ProductionGameSimulationConfig());Check(Restore(sim,c,terrain),"accepted unfinished proof restores");
        const auto old=sim.Snapshot();
        auto captured=sim.CaptureCampaignCheckpoint(c.id);
        Check(captured && SameProgress(captured->dialogue,c.dialogue),"accepted proof retains both ordered response continuations");
        InputSnapshot in;in.damageEnabled=false;in.commands.dialogueSkip=80;in.yawRadians=c.yaw;in.pitchRadians=c.pitch;
        Check(captured && Restore(sim,*captured,terrain,in),"same-session load refreshes callback and input identities");
        const auto fresh=sim.Snapshot();
        Check(fresh.chapterDialogue.generation>old.chapterDialogue.generation && fresh.tickIndex>old.tickIndex &&
            fresh.rescue.generation>old.rescue.generation && fresh.chapterCompanion.generation>old.chapterCompanion.generation,
            "load creates fresh dialogue, presentation, traversal and companion identities");
        Check(!sim.AcknowledgeChapterPresentation(old.tickIndex,old.chapterDialogue.generation),"pre-load presentation receipt is stale");
        in.dialogueCompletionLine=static_cast<std::uint32_t>(old.chapterDialogue.line);
        in.dialogueCompletionGeneration=old.chapterDialogue.generation;sim.StepFixed(in);
        Check(sim.Snapshot().chapterDialogue.line==dlg::Line::ReunionProof,"pre-load audio callback and skip do not finish restored line");
        in.dialogueCompletionGeneration=0;
        ++in.commands.dialogueSkip;sim.StepFixed(in);
        Check(sim.Snapshot().chapterDialogue.line==dlg::Line::ReunionFirstPiece,"accepted proof continues to first piece exactly once");
        ++in.commands.dialogueSkip;sim.StepFixed(in);
        Check(sim.Snapshot().chapterDialogue.line==dlg::Line::ReunionDepart,"accepted proof continues to departure in order");
        ++in.commands.dialogueSkip;sim.StepFixed(in);
        Check(sim.Snapshot().chapterDialogue.proofAccepted && sim.Snapshot().chapterDialogue.line==dlg::Line::None,
            "completed reunion retains accepted proof without replay");
        captured=sim.CaptureCampaignCheckpoint(c.id);
        GameSimulation completed(ProductionGameSimulationConfig());
        Check(captured && Restore(completed,*captured,terrain),"completed reunion round trips through fresh simulation");
        InputSnapshot neutral;neutral.yawRadians=c.yaw;neutral.pitchRadians=c.pitch;completed.StepFixed(neutral);
        Check(completed.Snapshot().chapterDialogue.proofAccepted && completed.Snapshot().chapterDialogue.line==dlg::Line::None,
            "completed reunion does not replay lesson after reload");
    }
    for(int vitality:{1,2,3}) for(bool stowed:{false,true}) for(bool high:{false,true}) {
        auto c=Endpoint(*terrain);c.vitality=vitality;c.swordStowed=stowed;c.lanternHigh=high;
        GameSimulation sim(ProductionGameSimulationConfig());
        if(!Restore(sim,c,terrain)) {Check(false,"forest endpoint restores into fresh production simulation");continue;}
        const auto loaded=sim.Snapshot();CheckClean(loaded);
        Check(cp::LookoutObjectiveAvailable(loaded,sim.LatestCampaignBoundary()),
            "completed grounded lookout restore permits an explicit objective request");
        auto unavailable=loaded;unavailable.playerAlive=false;
        Check(!cp::LookoutObjectiveAvailable(unavailable,sim.LatestCampaignBoundary()) &&
            !cp::LookoutObjectiveAvailable(loaded,std::nullopt),
            "death or an unearned boundary cannot expose a chapter-completion panel");
        unavailable=loaded;unavailable.chapterDialogue.line=dlg::Line::ForestVillage;
        Check(!cp::LookoutObjectiveAvailable(unavailable,sim.LatestCampaignBoundary()),
            "active final speech keeps the optional objective unavailable");
        unavailable=loaded;unavailable.playerX-=3.f;
        Check(!cp::LookoutObjectiveAvailable(unavailable,sim.LatestCampaignBoundary()),
            "the completed objective does not follow the player away from the lookout");
        unavailable=loaded;unavailable.worldRoute.readiness[static_cast<std::size_t>(WorldZoneId::Lookout)]=ZoneReadiness::Preparing;
        Check(!cp::LookoutObjectiveAvailable(unavailable,sim.LatestCampaignBoundary()),
            "unready resources cannot expose the lookout panel");
        const auto playerGround=tr::ChapterExteriorSupport(terrain.get(),c.position.x,c.position.z);
        const auto kitGround=terrain->Query(c.position.x+1.f,c.position.z);
        Check(loaded.worldRoute.current==WorldZoneId::Lookout && loaded.rescue.exteriorSide &&
            loaded.rescue.firstAscentCompleted && loaded.playerSupportWorldY==c.position.y &&
            loaded.playerVitals.vitality==vitality,"forest restore retains actual support, exterior progression and saved hearts");
        Check(playerGround.grounded && loaded.playerSupportId==playerGround.id &&
            loaded.playerSupportId!=static_cast<PlayerSupportId>(130) && loaded.playerSupportSurface==playerGround.surface,
            "restored forest support identity/surface comes from the actual triangle rather than summit id 130");
        Check(kitGround.found && loaded.chapterCompanion.grounded && !loaded.chapterCompanion.walking &&
            loaded.chapterCompanion.waypoint==kWorldRoutePoints.size() &&
            std::abs(loaded.chapterCompanion.x-(c.position.x+1.f))<.001f &&
            std::abs(loaded.chapterCompanion.z-c.position.z)<.001f &&
            std::abs(loaded.chapterCompanion.y-kitGround.worldY)<.001f,
            "forest restore places Kit at its completed grounded endpoint rather than the reunion");
        Check(loaded.heldItems[1].parentMode==(stowed?items::HeldItemParentMode::BodyStow:items::HeldItemParentMode::HandSocket) &&
            (loaded.interaction.heldLightPose==interactions::HeldLightPose::High)==high,
            "endpoint restore preserves sword attachment and lantern pose");
        const auto captured=sim.CaptureCampaignCheckpoint(c.id);
        Check(captured && SameRecord(*captured,c),"forest endpoint capture/restore preserves every logical value");
        InputSnapshot in;in.damageEnabled=false;in.yawRadians=c.yaw;in.pitchRadians=c.pitch;
        for(int tick=0;tick<180;++tick)sim.StepFixed(in);
        const auto after=sim.Snapshot();
        Check(after.chapterDialogue.line==dlg::Line::None && after.chapterDialogue.proofAccepted &&
            after.chapterDialogue.forestNightConsumed && after.chapterDialogue.forestWaystoneConsumed &&
            after.chapterDialogue.forestClueConsumed && after.chapterDialogue.forestVillageConsumed &&
            after.chapterCompanion.waypoint==kWorldRoutePoints.size() && !after.chapterCompanion.walking,
            "loaded endpoint neither replays completed speech nor sends Kit walking from the reunion");
    }
    for(bool upper:{false,true}) {
        auto c=Endpoint(*terrain);c.id=upper?cp::CheckpointId::SafeExit:cp::CheckpointId::LowerClaimed;
        c.zone=upper?WorldZoneId::TombExterior:WorldZoneId::Dungeon;
        c.position=upper?tr::kExteriorLanding:tr::kLowerLanding;c.yaw=upper?0.f:-1.57079632679f;c.pitch=0;
        GameSimulation sim(ProductionGameSimulationConfig());
        Check(Restore(sim,c,terrain),"completed forest content restores at either later rope safe side");
        const auto captured=sim.CaptureCampaignCheckpoint(c.id);
        Check(captured && SameRecord(*captured,c),"returned safe-side capture keeps completed forest content revision 2");
        Check(sim.Snapshot().rescue.exteriorSide==upper && sim.Snapshot().chapterCompanion.grounded &&
            sim.Snapshot().chapterCompanion.waypoint==kWorldRoutePoints.size() &&
            std::abs(sim.Snapshot().chapterCompanion.x-(kWorldRoutePoints.back().x+1.f))<.001f,
            "returned player does not drag completed Kit back from lookout to reunion or dungeon");
        InputSnapshot in;in.damageEnabled=false;in.yawRadians=c.yaw;in.pitchRadians=c.pitch;
        for(int tick=0;tick<180;++tick)sim.StepFixed(in);
        Check(sim.Snapshot().chapterDialogue.line==dlg::Line::None && sim.Snapshot().chapterDialogue.forestVillageConsumed,
            "returned safe-side restore retains completed forest speech without replay");
        auto bad=c;bad.dialogue.forestVillageConsumed=false;
        Check(!cp::Validate(bad),"revision-2 safe side cannot omit the completed forest boundary facts");
    }
    {
        auto c=Endpoint(*terrain);GameSimulation sim(ProductionGameSimulationConfig());
        const auto original=sim.Snapshot();
        const auto reject=[&](const cp::Checkpoint& bad) {
            Check(!cp::Validate(bad) && !sim.BeginCampaignRestore(bad),"incoherent forest record rejects before restore");
            Check(SameSession(original,sim.Snapshot()),"rejected forest record preserves live gameplay");
        };
        auto bad=c;bad.contentRevision=1;reject(bad);
        bad=c;bad.contentRevision=3;reject(bad);
        bad=c;bad.zone=WorldZoneId::TombExterior;reject(bad);
        bad=c;bad.firstAscentCompleted=false;reject(bad);
        bad=c;bad.dialogue.proofAccepted=false;reject(bad);
        bad=c;bad.dialogue.waitingForRaise=true;reject(bad);
        for(unsigned missing=0;missing<4;++missing) {
            bad=c;
            if(missing==0)bad.dialogue.forestNightConsumed=false;
            if(missing==1)bad.dialogue.forestWaystoneConsumed=false;
            if(missing==2)bad.dialogue.forestClueConsumed=false;
            if(missing==3)bad.dialogue.forestVillageConsumed=false;
            reject(bad);
        }
        bad=c;bad.dialogue.activeLine=dlg::Line::ForestVillage;reject(bad);
        bad=c;bad.dialogue.continuationCount=1;bad.dialogue.continuations[0]=dlg::Line::ForestVillage;reject(bad);
        bad=c;bad.position.x+=1.f;reject(bad);
        bad=c;bad.position.y+=.05f;
        const auto wrongHeight=sim.BeginCampaignRestore(bad);
        Check(!wrongHeight || !sim.CommitCampaignRestore(*wrongHeight,{wrongHeight->worldGeneration,bad.zone},terrain,{}),
            "forest restore cannot accept a serialized height above the actual terrain");
        Check(SameSession(original,sim.Snapshot()),"failed forest terrain admission leaves the old session intact");
        sim.CancelCampaignRestore();
        const auto kitGround=terrain->Query(c.position.x+1.f,c.position.z);
        const float x=c.position.x+1.f,z=c.position.z,y=kitGround.worldY+1.5f;
        const std::array<TriangleSupportSurface::Triangle,2> kitRoof{{
            {{{{x-.35f,y,z-.35f},{x+.35f,y,z+.35f},{x-.35f,y,z+.35f}}},SupportSurface::Wood},
            {{{{x-.35f,y,z-.35f},{x+.35f,y,z-.35f},{x+.35f,y,z+.35f}}},SupportSurface::Wood}}};
        const auto unsafeKit=TriangleSupportSurface::WithAdditionalNonWalkableGeometry(*terrain,kitRoof,error);
        const auto blockedKit=sim.BeginCampaignRestore(c);
        Check(unsafeKit && blockedKit &&
            !sim.CommitCampaignRestore(*blockedKit,{blockedKit->worldGeneration,c.zone},unsafeKit,{}) &&
            SameSession(original,sim.Snapshot()),
            "blocked standing clearance at Kit alone rejects the complete endpoint restore atomically");
        sim.CancelCampaignRestore();
        auto playerRoof=kitRoof;
        for(auto& triangle:playerRoof)for(auto& point:triangle.points) {
            point[0]-=1.f;point[1]=c.position.y+1.5f;
        }
        const auto unsafePlayer=TriangleSupportSurface::WithAdditionalNonWalkableGeometry(*terrain,playerRoof,error);
        const auto blockedPlayer=sim.BeginCampaignRestore(c);
        Check(unsafePlayer && blockedPlayer &&
            !sim.CommitCampaignRestore(*blockedPlayer,{blockedPlayer->worldGeneration,c.zone},unsafePlayer,{}) &&
            SameSession(original,sim.Snapshot()),
            "blocked standing clearance at the canonical player alone rejects endpoint restore atomically");
        sim.CancelCampaignRestore();
        Check(Restore(sim,c,terrain),"endpoint admission guard setup restores actual terrain");
        const auto pending=sim.BeginCampaignRestore(c);
        Check(pending && !sim.CaptureAuthoredCampaignBoundary(c.id),"pending restore cannot publish another forest boundary");
        sim.CancelCampaignRestore();
        InputSnapshot in;in.damageEnabled=false;in.yawRadians=c.yaw;in.pitchRadians=c.pitch;
        sim.StepFixed(in);in.crouchHeld=true;
        for(int tick=0;tick<18;++tick)sim.StepFixed(in);
        Check(sim.Snapshot().crouchActive && !sim.CaptureAuthoredCampaignBoundary(c.id),
            "forest boundary requires standing even at its safe anchor");
        in.crouchHeld=false;for(int tick=0;tick<18;++tick)sim.StepFixed(in);
        Check(bool(sim.CaptureAuthoredCampaignBoundary(c.id)),"standing again restores endpoint admission before invalidation");
        const auto generation=sim.Snapshot().worldRoute.generation;
        sim.InvalidateWorldZoneReadiness();
        Check(!sim.CaptureAuthoredCampaignBoundary(c.id) &&
            !sim.PublishChapterTerrain({generation,WorldZoneId::Lookout},terrain),
            "invalidated endpoint resources and stale Lookout readiness cannot publish a checkpoint");
    }
    {
        const auto c=Endpoint(*terrain);GameSimulation sim(ProductionGameSimulationConfig());
        InputSnapshot old;old.moveForward=1;old.runHeld=true;old.yawRadians=1.f;
        old.hasAuthoritativePlayerPose=true;old.authoritativePlayerX=-33.7f;old.authoritativePlayerZ=-12.8f;
        old.commands.attack=17;old.commands.toggleHeldLightPose=18;
        const auto before=sim.Snapshot();old.dialogueCompletionLine=static_cast<std::uint32_t>(dlg::Line::ForestVillage);
        old.dialogueCompletionGeneration=before.chapterDialogue.generation;
        const auto token=sim.BeginCampaignRestore(c);
        Check(token && sim.CommitCampaignRestore(*token,{token->worldGeneration,c.zone},terrain,old,100),
            "forest restore consumes current movement, action and callback floors");
        sim.StepFixed(old,1.f/60.f,100);
        Check(sim.Snapshot().paused && sim.Snapshot().playerX==c.position.x && sim.Snapshot().playerZ==c.position.z,
            "old held or authoritative pose publication cannot move a restored endpoint");
        auto neutral=old;neutral.moveForward=0;neutral.runHeld=false;neutral.hasAuthoritativePlayerPose=false;
        neutral.yawRadians=c.yaw;neutral.pitchRadians=c.pitch;
        sim.StepFixed(neutral,1.f/60.f,100);Check(sim.Snapshot().paused,"stale neutral endpoint publication cannot release load");
        sim.StepFixed(neutral,1.f/60.f,101);
        Check(!sim.Snapshot().paused && sim.Snapshot().chapterDialogue.line==dlg::Line::None &&
            sim.Snapshot().chapterDialogue.proofAccepted && sim.Snapshot().playerCombat.action==PlayerCombatAction::Idle,
            "fresh rebased endpoint input resumes without stale dialogue or combat debt");
        Check(!sim.AcknowledgeChapterPresentation(before.tickIndex,before.chapterDialogue.generation),
            "pre-load presentation cannot acknowledge a restored endpoint");
    }
    std::cout<<checks<<" campaign checkpoint checks, "<<failures<<" failures\n";
    return failures?1:0;
}
