#include "gameplay/simulation/GameSimulation.h"
#include "platform/android/GameplayEventMetadata.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/assets/AssetManifest.h"
#include <iostream>
#include <limits>
#include <memory>
#include <vector>
using namespace horde::gameplay::simulation;
namespace {
int checks=0,failures=0;
void Check(bool v,const char* m){++checks;if(!v){++failures;std::cerr<<m<<'\n';}}
SimulationSnapshot Exterior() {
    SimulationSnapshot s;s.developmentRescueJourney=true;s.rescue.exteriorSide=true;
    s.chapterDialogue.proofAccepted=true;s.worldRoute.current=WorldZoneId::ForestApproach;
    s.worldRoute.readiness[2]=ZoneReadiness::Ready;return s;
}
}
int main() {
    std::cout<<std::unitbuf;
    static_assert(static_cast<unsigned>(GameplayEventType::WaterfallContact)==27);
    static_assert(static_cast<unsigned>(GameplayEventType::ForestBird)==28);
    static_assert(static_cast<unsigned>(GameplayEventType::ForestBirdStopped)==29);
    static_assert(static_cast<unsigned>(GameplayEventType::ForestCrickets)==30);
    static_assert(static_cast<unsigned>(GameplayEventType::PondFrogs)==31);
    static_assert(static_cast<unsigned>(GameplayEventType::ForestFoliage)==35);
    static_assert(static_cast<unsigned>(GameplayEventType::LichRevivalCry)==36);
    static_assert(kGameplayEventTypeCount==37);
    auto s=Exterior();Check(ForestBirdAmbience::Eligible(s),"ready grounded same-night forest admitted");
    for(unsigned mode=0;mode<9;++mode) {
        auto bad=s;
        switch(mode){
        case 0:bad.paused=true;break;case 1:bad.playerAlive=false;break;
        case 2:bad.rescue.exteriorSide=false;break;case 3:bad.developmentRescueJourney=false;break;
        case 4:bad.rescue.equipmentStowed=true;break;case 5:bad.chapterDialogue.line=horde::gameplay::dialogue::Line::ForestNight;break;
        case 6:bad.chapterDialogue.proofAccepted=false;break;case 7:bad.worldRoute.readiness[2]=ZoneReadiness::Preparing;break;
        case 8:bad.playerGrounded=false;break;}
        Check(!ForestBirdAmbience::Eligible(bad),"pause/death/tomb/legacy/traversal/VO/proof/readiness/airborne suppress cue");
        const auto river=horde::scene::ChapterWaterFromAtlas(60,-110,2.3f);
        bad.playerX=river[0];bad.playerSupportWorldY=river[1];bad.playerZ=river[2];
        Check(!ForestBirdAmbience::PondEligible(bad),"same lifecycle and dialogue gates suppress pond cue beside actual river");
    }
    s.worldRoute.current=WorldZoneId::TombExterior;Check(!ForestBirdAmbience::Eligible(s),"rescue clearing is not forest ambience");
    s.worldRoute.current=WorldZoneId::Lookout;s.worldRoute.readiness[3]=ZoneReadiness::Ready;
    Check(ForestBirdAmbience::Eligible(s),"lookout remains part of admitted forest");
    {
        // Policy fixture only: these explicit snapshots test actual authored
        // surface coordinates, not a claim that gameplay reached the lake.
        auto pond=Exterior();pond.worldRoute.current=WorldZoneId::TombExterior;
        pond.worldRoute.readiness[1]=ZoneReadiness::Ready;
        const auto shore=horde::scene::ChapterWaterFromAtlas(166,-349,0);
        pond.playerX=shore[0];pond.playerSupportWorldY=shore[1]+1;pond.playerZ=shore[2];
        Check(!ForestBirdAmbience::Eligible(pond) && ForestBirdAmbience::PondEligible(pond),
            "pond uses actual shore in a ready same-night exterior without requiring forest zone");
        const auto river=horde::scene::ChapterWaterFromAtlas(60,-110,2.3f);
        pond.playerX=river[0];pond.playerSupportWorldY=river[1]-1;pond.playerZ=river[2];
        Check(ForestBirdAmbience::PondEligible(pond),"actual regional ribbon admits nearby grounded exterior pond ambience");
        pond.playerSupportWorldY=river[1]+ForestBirdAmbience::kPondProximityMetres+1;
        Check(!ForestBirdAmbience::PondEligible(pond),"vertical separation excludes a high ledge over the same river XZ");
        pond.playerSupportWorldY=river[1];pond.worldRoute.current=WorldZoneId::Dungeon;
        Check(!ForestBirdAmbience::PondEligible(pond),"dungeon cannot borrow regional exterior water even at matching coordinates");
        pond.worldRoute.current=WorldZoneId::TombExterior;pond.worldRoute.readiness[1]=ZoneReadiness::Preparing;
        Check(!ForestBirdAmbience::PondEligible(pond),"unready exterior cannot admit pond ambience");
        pond=Exterior();Check(!ForestBirdAmbience::PondEligible(pond),"ordinary forest coordinate is not an invented nearby pond");
        const auto notch=horde::scene::ChapterWaterFromAtlas(-15,-275,0);
        pond.playerX=notch[0];pond.playerSupportWorldY=notch[1];pond.playerZ=notch[2];
        Check(!horde::scene::IsNearChapterRegionalWater(pond.playerX,pond.playerSupportWorldY,pond.playerZ,1.f),
            "water query retains actual concave shore rather than a lake bounding box");
        pond.playerSupportWorldY=std::numeric_limits<float>::quiet_NaN();
        Check(!ForestBirdAmbience::PondEligible(pond),"nonfinite support cannot admit pond cue");
    }
    for(unsigned rate:{15,30,60,120}) {
        ForestBirdAmbience bird;FixedStepRunner runner;std::vector<unsigned> ticks,crickets;unsigned tick=0;
        for(unsigned frame=0;frame<rate*100;++frame)runner.Advance(1.0/rate,false,[&](float){
            const auto cue=bird.Step(true);
            if(cue==ForestBirdAmbience::Cue::Play)ticks.push_back(tick+1);
            if(cue==ForestBirdAmbience::Cue::PlayCrickets)crickets.push_back(tick+1);++tick;});
        Check(ticks==std::vector<unsigned>({720,3120,5520}),"render catchup grouping cannot change cadence or emit a burst");
        Check(crickets==std::vector<unsigned>({1440,3840}),"crickets occupy the quiet gap without overlapping eight-second owl cuts");
        Check(bird.Step(false)==ForestBirdAmbience::Cue::Stop && bird.Step(false)==ForestBirdAmbience::Cue::None,
            "ineligible transition stops exactly once");
        unsigned plays=0;for(unsigned tick=0;tick<719;++tick)plays+=bird.Step(true)==ForestBirdAmbience::Cue::Play;
        Check(plays==0 && bird.Step(true)==ForestBirdAmbience::Cue::Play,"reentry requires a new twelve-second quiet interval");
        bird.Reset();Check(bird.Step(true)==ForestBirdAmbience::Cue::None,"reset/load must not replay last cue");
        ForestBirdAmbience pond;FixedStepRunner pondRunner;std::vector<unsigned> pondTicks;unsigned pondTick=0;bool foreignCue=false;
        for(unsigned frame=0;frame<rate*100;++frame)pondRunner.Advance(1.0/rate,false,[&](float){
            const auto cue=pond.Step(false,true);++pondTick;
            if(cue==ForestBirdAmbience::Cue::PlayPondFrogs)pondTicks.push_back(pondTick);
            foreignCue|=cue==ForestBirdAmbience::Cue::Play || cue==ForestBirdAmbience::Cue::PlayCrickets;});
        Check(!foreignCue,"pond-only exterior never borrows forest owl or cricket eligibility");
        Check(pondTicks==std::vector<unsigned>({720,1440,3120,3840,5520}),"pond cadence remains sparse and independent of render grouping");
        Check(pond.Step(true,false)==ForestBirdAmbience::Cue::Stop,"leaving water immediately stops pond voice even while forest remains eligible");
        Check(pond.Step(true,false)==ForestBirdAmbience::Cue::None,"leaving water produces only one stop and no replacement burst");
        pond.Reset();unsigned resetPond=0;
        for(unsigned i=0;i<719;++i)resetPond+=pond.Step(false,true)!=ForestBirdAmbience::Cue::None;
        Check(resetPond==0 && pond.Step(false,true)==ForestBirdAmbience::Cue::PlayPondFrogs,"pond reset/load requires full quiet interval");
        Check(pond.Step(false,false)==ForestBirdAmbience::Cue::Stop,"all pond eligibility loss stops finite voice");
    }
    GameplayEvent event;event.type=GameplayEventType::ForestBird;event.sequence=57;
    const auto packed=horde::platform::android::PackGameplayEventMetadata(event);
    Check((packed&255)==28 && (packed>>32)==57,"appended cue preserves compact ordered Android transport");
    event.type=GameplayEventType::ForestCrickets;
    Check((horde::platform::android::PackGameplayEventMetadata(event)&255)==30,"cricket event uses its appended transport ID");
    event.type=GameplayEventType::PondFrogs;
    Check((horde::platform::android::PackGameplayEventMetadata(event)&255)==31,"pond event uses append-only transport ID31");
    auto game=std::make_unique<GameSimulation>(ProductionGameSimulationConfig());
    InputSnapshot input;input.damageEnabled=false;
    unsigned cues=0;for(unsigned tick=0;tick<800;++tick){game->StepFixed(input);
        for(const auto& e:game->Events().Events())cues+=e.type==GameplayEventType::ForestBird || e.type==GameplayEventType::ForestCrickets || e.type==GameplayEventType::PondFrogs;game->ClearEvents();}
    Check(cues==0,"real ordinary tomb fixed ticks never generate an exterior cue");
    std::cout<<"forest-bird ordinary journey starting\n";
    // Actual ordinary-input journey. Damage/teaching disabled CPU fixture;
    // readiness is imported geometry, not a GPU or owner-completion claim.
    const auto directory=std::filesystem::path(HORDE_RT_SOURCE_DIR)/"assets/models/world/runtime/chapter-terrain-v1";
    horde::scene::assets::AssetManifest manifest;horde::scene::assets::StaticMeshAsset mesh;std::string error;
    if(!horde::scene::assets::AssetManifest::Load(directory/"asset.manifest.json",manifest,error) ||
       !horde::scene::assets::StaticMeshAsset::Load(directory/"chapter-terrain-lod0.runtime.glb",manifest,mesh,error)){
        std::cerr<<error;return 1;}
    const auto terrain=horde::scene::PrepareChapterTerrainSurface(mesh,error);if(!terrain){std::cerr<<error;return 1;}
    game->StartLiveDevelopmentRescueJourney(true);
    for(unsigned i=1;i<4;++i)Check(game->PublishChapterTerrain({game->Snapshot().worldRoute.generation,static_cast<WorldZoneId>(i)},terrain),"CPU fixture publishes imported terrain for native movement");
    input.tutorialEnabled=false;
    const auto step=[&](){
        if(game->Snapshot().chapterDialogue.line!=horde::gameplay::dialogue::Line::None)++input.commands.dialogueSkip;
        game->StepFixed(input);game->ClearEvents();
    };
    const auto aim=[&](float x,float z,float stop=.03f){
        const auto& state=game->Snapshot();const float dx=x-state.playerX,dz=z-state.playerZ,d=std::hypot(dx,dz);
        input.yawRadians=std::atan2(dx,-dz);input.pitchRadians=0;input.moveStrafe=0;
        const float scale=d>.0001f?std::min(d,.30f)/d:0.f;
        input.crouchHeld=!AuthoredBodySweepClear(state.playerX,state.playerZ,state.playerX+dx*scale,state.playerZ+dz*scale,
            state.playerSupportWorldY,kStandingBodyHeight,horde::gameplay::kPlayerCollisionRadius);
        input.moveForward=d>stop?std::min(1.f,d/.15f):0.f;return d;
    };
    const auto routeWalk=[&](float x,float z){
        for(unsigned tick=0;tick<2400;++tick){if(aim(x,z)<.035f){input.moveForward=0;step();return true;}step();}
        std::cerr<<"forest ambience route blocked at "<<game->Snapshot().playerX<<','<<game->Snapshot().playerZ<<'\n';return false;
    };
    for(auto p:std::array<std::array<float,2>,8>{{{{0,-1}},{{0,-4.8f}},{{0,-9.4f}},{{4.2f,-9.4f}},
        {{4.2f,-14.6f}},{{1,-14.6f}},{{-1.8f,-15.2f}},{{-3.2f,-15.2f}}}})
        if(!routeWalk(p[0],p[1])){Check(false,"ordinary route reaches guards");return 1;}
    for(unsigned tick=0;tick<2400;++tick){
        const auto state=game->Snapshot();int target=-1;float nearest=100;
        for(unsigned i=0;i<state.skeletonEnemyCount;++i)if(!state.skeletonEnemies[i].dead){
            const auto& enemy=state.skeletonEnemies[i];const float d=std::hypot(enemy.x-state.playerX,enemy.z-state.playerZ);
            if(d<nearest){target=int(i);nearest=d;}}
        if(target<0)break;const auto& enemy=state.skeletonEnemies[target];aim(enemy.x,enemy.z,1.05f);
        if(nearest<1.55f && state.playerCombat.action==horde::gameplay::PlayerCombatAction::Idle)++input.commands.attack;
        step();
    }
    Check(game->Snapshot().skeletonEnemies[0].dead && game->Snapshot().skeletonEnemies[1].dead,"ordinary sword edges defeat both guards");
    std::cout<<"forest-bird guards tick="<<game->Snapshot().tickIndex<<'\n';
    if(!game->Snapshot().skeletonEnemies[0].dead || !game->Snapshot().skeletonEnemies[1].dead)return 1;
    if(!routeWalk(-31.2f,-15.2f)){Check(false,"ordinary route reaches Keeper");return 1;}
    for(unsigned tick=0;tick<2400 && game->Snapshot().lich.phase!=horde::gameplay::LichPhase::Dead;++tick){
        const auto state=game->Snapshot();const float d=aim(state.lich.x,state.lich.z,1.15f);
        if(state.lich.revealComplete && state.lich.hitCooldownRemaining<=0 && d<1.8f &&
           state.playerCombat.action==horde::gameplay::PlayerCombatAction::Idle)++input.commands.attack;
        step();
    }
    Check(game->Snapshot().lich.phase==horde::gameplay::LichPhase::Dead,"ordinary sword edges defeat Keeper");
    std::cout<<"forest-bird Keeper tick="<<game->Snapshot().tickIndex<<'\n';
    if(game->Snapshot().lich.phase!=horde::gameplay::LichPhase::Dead)return 1;
    if(!routeWalk(-34.f,-17.55f)){Check(false,"ordinary route reaches chest");return 1;}
    input.moveForward=0;input.yawRadians=-1.57079632679f;
    for(unsigned tick=0;tick<600 && game->Snapshot().chestReward.phase!=horde::gameplay::interactions::ChestRewardPhase::LanternClaimed;++tick){
        const auto prompt=game->Snapshot().chestPrompt;
        if(prompt==horde::gameplay::interactions::ChestRewardPrompt::OpenChest || prompt==horde::gameplay::interactions::ChestRewardPrompt::ClaimLantern)++input.commands.interact;
        step();
    }
    for(unsigned tick=0;tick<600 && !game->Snapshot().rescue.ropeReady;++tick)step();
    if(!routeWalk(-33.7f,-15.9f)){Check(false,"ordinary route reaches rope");return 1;}
    input.moveForward=0;++input.commands.interact;step();
    for(unsigned tick=0;tick<1800 && game->Snapshot().rescue.equipmentStowed;++tick)step();
    Check(game->Snapshot().rescue.exteriorSide,"actual native ascent reaches exterior");
    std::cout<<"forest-bird exterior tick="<<game->Snapshot().tickIndex<<'\n';
    for(unsigned tick=0;tick<90;++tick)step();
    ++input.commands.toggleHeldLightPose;
    for(unsigned tick=0;tick<600 && !game->Snapshot().chapterDialogue.proofAccepted;++tick){step();
        const auto& state=game->Snapshot();game->AcknowledgeChapterPresentation(state.tickIndex,state.chapterDialogue.generation);}
    Check(game->Snapshot().chapterDialogue.proofAccepted,"consumed manual raise and CPU presentation acknowledgement admit reunion");
    const auto walk=[&](float x,float z) {
        for(unsigned tick=0;tick<1800;++tick) {
            const auto& state=game->Snapshot();const float dx=x-state.playerX,dz=z-state.playerZ;
            const float distance=std::hypot(dx,dz);
            if(distance<.035f){input.moveForward=0;for(int i=0;i<20;++i)game->StepFixed(input);return true;}
            input.yawRadians=std::atan2(dx,-dz);input.moveForward=std::min(1.f,distance/.1f);
            step();
            if(game->Snapshot().worldRoute.blocked)return false;
        }return false;
    };
    Check(walk(kWorldRoutePoints[3].x,kWorldRoutePoints[3].z) && walk(kWorldRoutePoints[4].x,kWorldRoutePoints[4].z),
        "ordinary collision-resolved axes reach actual forest after earned ascent");
    for(unsigned tick=0;tick<200;++tick)step();
    Check(ForestBirdAmbience::Eligible(game->Snapshot()),"native forest snapshot admits quiet night ambience policy");
    std::cout<<"forest-bird forest tick="<<game->Snapshot().tickIndex<<" eligible="<<ForestBirdAmbience::Eligible(game->Snapshot())<<'\n';
    game->SynchronizePausedInput(input);game->ClearEvents();cues=0;
    for(unsigned tick=0;tick<720;++tick){game->StepFixed(input);
        for(const auto& e:game->Events().Events())if(e.type==GameplayEventType::ForestBird){++cues;
            Check(e.worldX==e.listenerX && e.worldZ==e.listenerZ && e.tickIndex==game->Snapshot().tickIndex,
                "real semantic cue carries event-time listener and tick, without fabricated bird geometry");}
        game->ClearEvents();}
    Check(cues==1,"native 60Hz production simulation emits exactly one finite request after quiet interval");
    unsigned cricketCues=0;
    for(unsigned tick=0;tick<720;++tick){game->StepFixed(input);
        for(const auto& e:game->Events().Events())if(e.type==GameplayEventType::ForestCrickets){++cricketCues;
            Check(e.worldX==e.listenerX && e.worldZ==e.listenerZ && e.tickIndex==game->Snapshot().tickIndex,
                  "real cricket event carries current fixed-step listener/tick");}
        game->ClearEvents();}
    Check(cricketCues==1,"ordinary forest simulation emits exactly one later cricket cue");
    input.paused=true;game->StepFixed(input);cues=0;
    input.paused=false;for(unsigned tick=0;tick<719;++tick){game->StepFixed(input);
        for(const auto& e:game->Events().Events())cues+=e.type==GameplayEventType::ForestBird || e.type==GameplayEventType::ForestCrickets || e.type==GameplayEventType::PondFrogs;game->ClearEvents();}
    Check(cues==0,"native pause/resume restarts quiet interval without an old cue burst");
    const auto earned=game->CaptureAuthoredCampaignBoundary(horde::gameplay::campaign::CheckpointId::SafeExit);
    Check(earned.has_value(),"recovery record comes from the actual completed ordinary-input journey");
    if(earned) {
        auto fresh=std::make_unique<GameSimulation>(ProductionGameSimulationConfig());
        const auto token=fresh->BeginCampaignRestore(*earned);
        Check(token && fresh->CommitCampaignRestore(*token,{token->worldGeneration,earned->zone},terrain,{}),
            "fresh session loads only an actually earned logical boundary");
        InputSnapshot neutral;neutral.damageEnabled=false;neutral.yawRadians=earned->yaw;neutral.pitchRadians=earned->pitch;
        unsigned replay=0;for(unsigned tick=0;tick<800;++tick){fresh->StepFixed(neutral);
            for(const auto& e:fresh->Events().Events())replay+=e.type==GameplayEventType::ForestBird || e.type==GameplayEventType::ForestCrickets || e.type==GameplayEventType::PondFrogs;fresh->ClearEvents();}
        Check(replay==0,"earned load does not replay forest audio debt at the safe exterior clearing");
    }
    Check(walk(kWorldRoutePoints[5].x,kWorldRoutePoints[5].z),
        "ordinary forest axes reach the actual route-pine listening radius");
    for(unsigned tick=0;tick<200;++tick)step();
    Check(ForestBirdAmbience::FoliageEligible(game->Snapshot()),
        "real earned-ascent journey reaches the bounded foliage emitter through native terrain movement");
    game->SynchronizePausedInput(input);game->ClearEvents();
    unsigned foliageCues=0;std::uint64_t previousFoliage=0;
    for(unsigned tick=0;tick<5160;++tick) {
        game->StepFixed(input);
        for(const auto& e:game->Events().Events())if(e.type==GameplayEventType::ForestFoliage) {
            ++foliageCues;const auto& current=game->Snapshot();
            Check(e.tickIndex==current.tickIndex && e.worldX==ForestBirdAmbience::kFoliagePosition.x &&
                  e.worldZ==ForestBirdAmbience::kFoliagePosition.z && e.worldY==ForestBirdAmbience::kFoliagePosition.y &&
                  e.listenerX==current.playerX && e.listenerZ==current.playerZ &&
                  std::abs(e.listenerY-(PlayerEyeWorldY(current.playerSupportWorldY)-current.playerCrouchDropMetres))<.0001f &&
                  e.worldX!=e.listenerX && e.intensity>0 && e.intensity<=ForestBirdAmbience::kFoliageGain &&
                  std::abs(e.intensity-ForestBirdAmbience::kFoliageGain*ForestBirdAmbience::FoliageRangeGain(current))<.0001f,
                  "real foliage event retains distinct fixed source, current listener/height and one range-scaled source gain");
            Check(!previousFoliage || e.tickIndex-previousFoliage==2400,
                  "actual shared simulation emits separated finite foliage requests without transition restarts");
            previousFoliage=e.tickIndex;
        }
        game->ClearEvents();
    }
    Check(foliageCues==2,"actual forest wait emits two foliage cues at 44/84 seconds rather than a repeating bed");
    std::cout<<checks<<" forest bird policy/transport/simulation checks, "<<failures<<" failures; listening and device mix unaccepted\n";
    return failures?1:0;
}
