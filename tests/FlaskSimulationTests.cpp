#include "gameplay/simulation/GameSimulation.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/assets/AssetManifest.h"
#include <filesystem>
#include <iostream>
#include <vector>

using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
namespace cp=horde::gameplay::campaign;
namespace tr=horde::gameplay::traversal;
namespace heal=horde::gameplay::healing;
namespace {
int checks=0,failures=0;
void Check(bool value,const char* message) {
    ++checks;if(!value){++failures;std::cerr<<message<<'\n';}
}
GameSimulationConfig Config(bool enabled=true) {
    auto c=ProductionGameSimulationConfig();c.healingFlask1_7=enabled;return c;
}
InputSnapshot Neutral(const GameSimulation& game) {
    InputSnapshot input;input.yawRadians=game.Snapshot().playerYawRadians;
    input.pitchRadians=game.Snapshot().playerPitchRadians;
    input.tutorialEnabled=false;input.damageEnabled=false;return input;
}
void Tick(GameSimulation& game,const InputSnapshot& input,int count=1) {
    for(int tick=0;tick<count;++tick)game.StepFixed(input);
}
int Health(const GameSimulation& game) {
    const auto& v=game.Snapshot().playerVitals;return v.vitality*2-(v.halfHeart?1:0);
}
cp::Checkpoint Record(cp::CheckpointId id,const TriangleSupportSurface& terrain) {
    cp::Checkpoint c;c.id=id;c.zone=cp::Destination(id);c.vitality=1;
    c.dialogue.grateConsumed=c.dialogue.rescueConsumed=true;
    if(id!=cp::CheckpointId::LowerClaimed) {
        c.position=tr::kExteriorLanding;c.firstAscentCompleted=true;c.yaw=0;
        c.dialogue.reunionStarted=c.dialogue.waitingForRaise=true;
    }
    if(id==cp::CheckpointId::ForestEndpoint) {
        c.schema=2;c.halfHeart=true;c.contentRevision=2;
        const auto endpoint=kWorldRoutePoints.back();const auto support=terrain.Query(endpoint.x,endpoint.z);
        Check(support.found&&support.walkable,"F04 uses actual imported support");
        c.position={endpoint.x,support.worldY,endpoint.z};c.yaw=2.51f;c.pitch=-.15f;
        c.dialogue.waitingForRaise=false;c.dialogue.proofAccepted=true;
        c.dialogue.forestNightConsumed=c.dialogue.forestWaystoneConsumed=true;
        c.dialogue.forestClueConsumed=c.dialogue.forestVillageConsumed=true;
    }
    return c;
}
bool Restore(GameSimulation& game,const cp::Checkpoint& c,
             const std::shared_ptr<const TriangleSupportSurface>& terrain,InputSnapshot input={}) {
    input.yawRadians=c.yaw;input.pitchRadians=c.pitch;
    const auto token=game.BeginCampaignRestore(c);
    Check(bool(token),"valid diagnostic restore begins");
    return token && game.CommitCampaignRestore(*token,{token->worldGeneration,c.zone},terrain,input);
}
}
int main() {
    const auto root=std::filesystem::path(HORDE_RT_SOURCE_DIR)/"assets/models/world/runtime/chapter-terrain-v1";
    horde::scene::assets::AssetManifest manifest;horde::scene::assets::StaticMeshAsset mesh;std::string error;
    if(!horde::scene::assets::AssetManifest::Load(root/"asset.manifest.json",manifest,error) ||
       !horde::scene::assets::StaticMeshAsset::Load(root/"chapter-terrain-lod0.runtime.glb",manifest,mesh,error)) {
        std::cerr<<error<<'\n';return 1;
    }
    const auto terrain=horde::scene::PrepareChapterTerrainSurface(mesh,error);
    if(!terrain){std::cerr<<error<<'\n';return 1;}
    Check(!ProductionGameSimulationConfig().healingFlask1_7,"normal gameplay does not admit the diagnostic flask");
    for(auto id:{cp::CheckpointId::LowerClaimed,cp::CheckpointId::SafeExit,cp::CheckpointId::ForestEndpoint}) {
        GameSimulation game(Config());const auto old=Record(id,*terrain);
        Check(Restore(game,old,terrain),"each old safe boundary restores on actual terrain");
        auto input=Neutral(game);Tick(game,input);
        Check(game.Snapshot().healingFlask.resources==heal::FlaskResources{false,0} &&
              game.Snapshot().flaskRestAnchor==static_cast<unsigned>(id),
              "legacy resources are empty and the matching earned Rest is available");
        const auto before=game.Snapshot();const auto revision=game.CampaignBoundaryRevision();
        ++input.commands.rest;Tick(game,input,119);
        Check(Health(game)==old.vitality*2-(old.halfHeart?1:0) &&
              game.Snapshot().healingFlask.resources.charges==0,
              "Rest cannot heal/refill before two seconds");
        Tick(game,input);
        const auto& rested=game.Snapshot();
        Check(Health(game)==6 && rested.healingFlask.resources==heal::FlaskResources{} &&
              rested.completedRestGeneration==1 && rested.healingFlask.action==heal::FlaskAction::Idle,
              "Rest completes health/resources atomically once without a free old-save fill");
        Check(rested.playerVitals.invulnerabilityRemaining==0 && rested.rescue.claimCount==before.rescue.claimCount &&
              rested.rescue.firstAscentCompleted==before.rescue.firstAscentCompleted &&
              rested.lich.phase==LichPhase::Dead && rested.skeletonEnemies[0].dead && rested.skeletonEnemies[1].dead &&
              rested.interaction.heldLightKind==before.interaction.heldLightKind,
              "Rest grants no invulnerability, enemy respawn, reward, ascent or left-light ownership change");
        const auto saved=game.CaptureCampaignCheckpoint(id);
        Check(saved && saved->schema==3 && saved->flask==heal::FlaskResources{} && saved->vitality==3 &&
              !saved->halfHeart && game.CampaignBoundaryRevision()>revision,
              "completed Rest refreshes exact schema3 logical state, including retained F04");
        const auto restGeneration=rested.completedRestGeneration;
        if(saved) {
            Check(Restore(game,*saved,terrain),"new exact resource record restores");
            Check(game.Snapshot().completedRestGeneration==restGeneration,
                  "load preserves the completion floor rather than replaying a Rest");
        }
        input=Neutral(game);Tick(game,input,3);
        Check(game.Snapshot().completedRestGeneration==restGeneration,
              "old input/completed timer cannot generate repeated recovery");
        if(id==cp::CheckpointId::ForestEndpoint) {
            input.moveForward=-1;Tick(game,input,85);input.moveForward=0;Tick(game,input);
            Check(game.LatestCampaignBoundary() && game.LatestCampaignBoundary()->id==id,
                  "backtracking away never downgrades the earned endpoint");
        }
    }
    {
        cp::Checkpoint legacy;legacy.vitality=1;
        legacy.torchDropX=-2.48f;legacy.torchDropZ=-16.4f; // old accepted trigger edge, not a player spawn
        GameSimulation game(Config());Check(Restore(game,legacy,terrain),"legacy trigger-edge record restores without moving progress");
        auto input=Neutral(game);Tick(game,input);++input.commands.rest;Tick(game,input);
        Check(game.Snapshot().chapterDialogue.line!=dialogue::Line::None &&
              game.Snapshot().healingFlask.resources.charges==0 &&
              game.Snapshot().healingFlask.action==heal::FlaskAction::Idle,
              "Rest does not interrupt or skip a restored safe-point conversation");
        for(int line=0;line<4 && game.Snapshot().chapterDialogue.line!=dialogue::Line::None;++line) {
            input.dialogueCompletionLine=static_cast<unsigned>(game.Snapshot().chapterDialogue.line);
            input.dialogueCompletionGeneration=game.Snapshot().chapterDialogue.generation;Tick(game,input);
        }
        input.dialogueCompletionLine=0;input.dialogueCompletionGeneration=0;
        Check(game.Snapshot().chapterDialogue.line==dialogue::Line::None &&
              game.Snapshot().flaskRestAnchor==static_cast<unsigned>(cp::CheckpointId::LowerClaimed),
              "ordinary matching dialogue completions expose recovery without new story flags");
        ++input.commands.rest;Tick(game,input,120);
        Check(Health(game)==6 && game.Snapshot().healingFlask.resources.charges==2 &&
              game.Snapshot().torchFailure.droppedX==legacy.torchDropX &&
              game.Snapshot().torchFailure.droppedZ==legacy.torchDropZ,
              "legacy edge data retains the same earned reachable Rest route");
    }
    {
        auto c=Record(cp::CheckpointId::LowerClaimed,*terrain);c.schema=3;c.halfHeart=true;c.flask={true,1};
        GameSimulation game(Config());Check(Restore(game,c,terrain),"upgraded partial record restores exactly");
        auto input=Neutral(game);Tick(game,input);const auto initial=game.Snapshot().healingFlask.resources;
        ++input.commands.drinkFlask;Tick(game,input,74);
        Check(Health(game)==1 && game.Snapshot().healingFlask.resources==initial,
              "drink starts with no immediate health/resource grant");
        input.guardHeld=true;++input.commands.parry;Tick(game,input);
        Check(Health(game)==1 && game.Snapshot().healingFlask.resources==initial &&
              game.Snapshot().healingFlask.action==heal::FlaskAction::Idle,
              "fresh defensive intent cancels before a drink completion tie");
        input.guardHeld=false;Tick(game,input,40);
        ++input.commands.drinkFlask;Tick(game,input,75);
        Check(Health(game)==3 && game.Snapshot().healingFlask.resources==heal::FlaskResources{true,0},
              "a fresh successful drink heals one heart, preserves fraction and consumes once");
        const auto after=game.Snapshot();Tick(game,input,90);
        Check(Health(game)==3 && game.Snapshot().healingFlask.resources==after.healingFlask.resources,
              "unchanged request counter cannot repeat a drink");
        ++input.commands.rest;Tick(game,input,40);input.paused=true;game.AdvanceFrame(input,0);
        Check(game.Snapshot().healingFlask.action==heal::FlaskAction::Idle &&
              game.Snapshot().healingFlask.resources.charges==0,
              "zero-tick pause cancels recovery without a refill");
        input.paused=false;Tick(game,input,125);
        Check(Health(game)==3 && game.Snapshot().healingFlask.resources.charges==0,
              "paused request/timer never resumes after unpause");
        ++input.commands.rest;Tick(game,input,30);game.ResetTiming();
        Check(game.Snapshot().healingFlask.action==heal::FlaskAction::Idle,
              "timing reconstruction publishes cancellation without another gameplay tick");
        Tick(game,input,125);
        Check(game.Snapshot().healingFlask.resources.charges==0,
              "timing reconstruction cancels but never refills");
        // The previously displayed Rest is now stale because movement intent
        // owns this tick. Rejection must not select an implicit Drink instead.
        input.moveStrafe=1;++input.commands.rest;Tick(game,input);
        input.moveStrafe=0;Tick(game,input,125);
        Check(Health(game)==3 && game.Snapshot().healingFlask.resources.charges==0,
              "stale Rest after competing movement rejects without falling back to Drink");
        const auto retryBefore=game.Snapshot();game.RetryEncounter();
        Check(game.Snapshot().campaignRetryRequestGeneration==retryBefore.campaignRetryRequestGeneration+1 &&
              Health(game)==3 && game.Snapshot().healingFlask.resources==retryBefore.healingFlask.resources,
              "post-claim Retry publishes only an owner request, never a fabricated heal/refill");
        const auto retryGeneration=game.Snapshot().campaignRetryRequestGeneration;
        InputSnapshot restoreInput;restoreInput.commands.drinkFlask=restoreInput.commands.rest=9;
        Check(Restore(game,c,terrain,restoreInput),"restoring after owner retry is an explicit admitted record operation");
        auto replay=Neutral(game);replay.commands.drinkFlask=replay.commands.rest=9;Tick(game,replay,125);
        Check(game.Snapshot().healingFlask.resources==c.flask && Health(game)==1 &&
              game.Snapshot().healingFlask.action==heal::FlaskAction::Idle,
              "load rebases distinct utility sequences and rejects their old publications");
        Check(game.Snapshot().campaignRetryRequestGeneration==retryGeneration,
              "restore preserves retry request generation floor");
        game.BeginCombatPractice(EnemyKind::Skeleton);
        Check(game.Snapshot().healingFlask.resources==c.flask,
              "generic encounter selection preserves charges/upgrade without a refill");
        game.ResetRoute();
        Check(game.Snapshot().healingFlask.resources==heal::FlaskResources{} &&
              game.Snapshot().campaignRetryRequestGeneration==retryGeneration,
              "New Game restores base two full charges without rolling owner generations backwards");
    }
    {
        // Forecast the deterministic contact schedule, then establish actual
        // damage on the same tick as a75-step drink completion. This is a host
        // fixture with explicit damage gating, not a natural combat playthrough.
        // Use the same established live melee setup as CombatFoundationTests,
        // rather than assuming a stationary waterfall practice start is in range.
        auto combat=Config();combat.waterfallSkeletonEncounter=false;
        combat.swordStartsStowed=false;combat.playerStartX=0;combat.playerStartZ=-3.75f;
        GameSimulation forecast(combat);
        auto input=Neutral(forecast);std::vector<int> contact;
        for(int tick=1;tick<=500 && contact.size()<3;++tick) {
            Tick(forecast,input);if(forecast.Snapshot().swordCombat.playerHitPulse)contact.push_back(tick);
        }
        Check(contact.size()>=2,"bounded skeleton fixture establishes current damage contacts");
        if(contact.size()>=2) {
            const int first=contact.front();int target=0;
            for(int hit:contact)if(hit-first>75){target=hit;break;}
            Check(target>0,"a later contact allows a full drink duration after the first wound");
            if(target) {
                GameSimulation trial(combat);input=Neutral(trial);
                for(int tick=1;tick<=target;++tick) {
                    input.damageEnabled=tick==first || tick==target;
                    if(tick==target-74)++input.commands.drinkFlask;
                    Tick(trial,input);
                }
                Check(Health(trial)==2 && trial.Snapshot().healingFlask.resources.charges==2 &&
                      trial.Snapshot().healingFlask.action==heal::FlaskAction::Idle &&
                      trial.Snapshot().flaskCompletionTick==0,
                      "actual accepted damage wins the drink completion tick without consuming");
                input.damageEnabled=false;++input.commands.drinkFlask;Tick(trial,input,75);
                Check(trial.Snapshot().healingFlask.resources.charges==1,
                      "a later fresh drink can consume after the real interruption");
                trial.BeginCombatPractice(EnemyKind::Lich);
                Check(trial.Snapshot().healingFlask.resources.charges==1,
                      "changing the selected encounter never refills the flask");
                trial.RetryEncounter();
                Check(trial.Snapshot().healingFlask.resources.charges==2,
                      "only explicit pre-claim Retry refills the current capacity");
            }
        }
    }
    {
        auto c=Record(cp::CheckpointId::LowerClaimed,*terrain);c.schema=3;c.flask={false,2};
        GameSimulation game(Config());Check(Restore(game,c,terrain),"blocked dodge fixture restores a wounded player");
        auto input=Neutral(game);Tick(game,input); // release the restored input rebase barrier
        input.yawRadians=0;input.moveForward=1;Tick(game,input,180);
        Check(game.Snapshot().playerTravelledThisTick<=.00001f && game.Snapshot().playerGrounded,
              "blocked dodge fixture reaches the actual lower-room wall without a teleport");
        input.moveForward=0;Tick(game,input);++input.commands.dodge;Tick(game,input);
        ++input.commands.drinkFlask;Tick(game,input);
        Check(game.Snapshot().dodgeActive && game.Snapshot().playerTravelledThisTick<=.00001f &&
              game.Snapshot().playerCombat.action==PlayerCombatAction::Idle,
              "collision-blocked dodge remains a live motor action despite idle sword and zero travel");
        Check(!game.Snapshot().flaskDrinkEligible && game.Snapshot().healingFlask.action==heal::FlaskAction::Idle &&
              Health(game)==2 && game.Snapshot().healingFlask.resources.charges==2,
              "active blocked dodge rejects a drink request without healing or consuming");
    }
    {
        GameSimulation off(Config(false));const auto old=Record(cp::CheckpointId::LowerClaimed,*terrain);
        Check(Restore(off,old,terrain),"feature-off legacy restore remains available");
        auto input=Neutral(off);++input.commands.rest;++input.commands.drinkFlask;Tick(off,input,125);
        const auto saved=off.CaptureCampaignCheckpoint(old.id);
        Check(Health(off)==2 && !off.Snapshot().healingFlaskEnabled && saved && saved->schema==1,
              "feature-off requests change neither health nor legacy captured schema");
        auto resources=old;resources.schema=3;
        Check(!off.BeginCampaignRestore(resources),"feature-off runtime still rejects unowned schema3 resources");
    }
    std::cout<<"flask simulation checks="<<checks<<" failures="<<failures<<'\n';
    return failures?1:0;
}
