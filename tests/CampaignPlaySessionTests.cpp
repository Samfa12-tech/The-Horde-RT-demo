#include "gameplay/campaign/CampaignPlaySession.h"
#include "gameplay/validation/RecordedReturnCheckpoint.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/assets/AssetManifest.h"
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace cp=horde::gameplay::campaign;
namespace sim=horde::gameplay::simulation;
namespace io=cp::storage;
namespace fs=std::filesystem;
namespace {
int checks=0,failures=0;
void Check(bool value,const char* message){++checks;if(!value){++failures;std::cerr<<"FAIL: "<<message<<'\n';}}
struct Directory {
    fs::path path=fs::temp_directory_path()/("horde-play-session-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Directory(){if(!fs::create_directory(path))throw std::runtime_error("exclusive test root failed");std::ofstream(path/"owned.txt")<<"fixture";}
    ~Directory(){std::error_code ec;if(path.filename().string().starts_with("horde-play-session-test-") &&
        fs::weakly_canonical(path.parent_path(),ec)==fs::weakly_canonical(fs::temp_directory_path(),ec) && !ec &&
        fs::is_regular_file(path/"owned.txt"))fs::remove_all(path,ec);}
};
cp::Checkpoint Record(int health,bool upper=false){cp::Checkpoint c;c.vitality=health;
    c.dialogue.grateConsumed=c.dialogue.rescueConsumed=true;
    if(upper){c.id=cp::CheckpointId::SafeExit;c.zone=sim::WorldZoneId::TombExterior;
        c.position=horde::gameplay::traversal::kExteriorLanding;c.firstAscentCompleted=true;
        c.dialogue.reunionStarted=c.dialogue.waitingForRaise=true;}
    return c;
}
bool Seed(sim::GameSimulation& game,const cp::Checkpoint& c,const std::shared_ptr<const sim::TriangleSupportSurface>& terrain){
    auto r=game.BeginCampaignRestore(c);return r && game.CommitCampaignRestore(*r,{r->worldGeneration,c.zone},terrain,{});
}
template<class Predicate>bool Until(cp::PlaySession& play,Predicate done){
    const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    do{play.Poll();if(done())return true;std::this_thread::yield();}while(std::chrono::steady_clock::now()<end);return false;
}
bool CatalogueDone(const cp::PlaySession& play){for(const auto& e:play.Catalogue())if(e.status==cp::CatalogueStatus::Checking)return false;return true;}
bool Done(cp::PlaySession& play){return Until(play,[&]{return play.State().outcome!=cp::PlayOutcome::Pending;});}
fs::path Path(const Directory& d,unsigned slot,cp::SaveKind kind){return d.path/cp::SaveBasename(*cp::SaveTarget(slot,kind));}
std::vector<std::byte> Raw(const fs::path& path){std::ifstream f(path,std::ios::binary);std::vector<std::byte> bytes;char c;
    while(f.get(c))bytes.push_back(std::byte(static_cast<unsigned char>(c)));return bytes;}
io::StoredCheckpoint Read(const Directory& d,unsigned slot,cp::SaveKind kind){io::StoredCheckpoint record;
    Check(io::Read(d.path,cp::SaveBasename(*cp::SaveTarget(slot,kind)),record).status==io::Status::Read,"saved target reads");return record;}
struct Faults:io::FaultInjector {
    std::atomic<bool> failRead=false,failWrite=false,failAfterPublish=false;
    bool Fail(io::Operation op,std::size_t)override {
        if(op==io::Operation::Read)return failRead.exchange(false);
        if(op==io::Operation::AfterPublish)return failAfterPublish.load();
        return op==io::Operation::Write && failWrite.load();
    }
};
int ExportFixtures(const fs::path& requested,const std::shared_ptr<const sim::TriangleSupportSurface>& lookout={},bool route=false) {
    std::error_code error;
    const auto directory=fs::absolute(requested,error).lexically_normal();
    if(error || !directory.filename().string().starts_with("horde-campaign-fixtures-") ||
       !fs::is_directory(directory,error) || error || fs::is_symlink(fs::symlink_status(directory,error)) || error ||
       !fs::is_empty(directory,error) || error){std::cerr<<"Explicit caller-created empty horde-campaign-fixtures-* directory required.\n";return 2;}
    const auto parent=fs::canonical(directory.parent_path(),error);if(error)return 2;
    const auto reports=fs::canonical(fs::path(HORDE_RT_SOURCE_DIR)/"reports",error);if(error)return 2;
    const auto temporary=fs::canonical(fs::temp_directory_path(),error);if(error || (parent!=reports && parent!=temporary)){
        std::cerr<<"Fixture directory must be a direct child of repository reports or system temporary directory.\n";return 2;}
    if(route) {
        const auto c=Record(2);
        if(io::Write(directory,"slot-1-manual.hrd",c).status!=io::Status::Committed)return 1;
        std::ofstream manifest(directory/"manifest.json",std::ios::binary);
        manifest<<"{\"evidenceClass\":\"seeded post-Keeper lower start; subsequent input-driven progression\",\"record\":\"slot-1-manual.hrd\",\"checkpoint\":\"LowerClaimed\",\"contentRevision\":1,\"hearts\":2}\n";
        manifest.close();return manifest?0:1;
    }
    if(lookout) {
        auto c=Record(2,true);c.id=cp::CheckpointId::ForestEndpoint;c.contentRevision=2;
        c.zone=sim::WorldZoneId::Lookout;
        const auto p=sim::kWorldRoutePoints.back();
        const auto ground=lookout->Query(p.x,p.z);
        if(!ground.found || !ground.walkable)return 1;
        c.position={p.x,ground.worldY,p.z};c.yaw=2.51f;c.pitch=-.15f;
        c.dialogue.waitingForRaise=false;c.dialogue.proofAccepted=true;
        c.dialogue.forestNightConsumed=c.dialogue.forestWaystoneConsumed=true;
        c.dialogue.forestClueConsumed=c.dialogue.forestVillageConsumed=true;
        sim::GameSimulation probe(sim::ProductionGameSimulationConfig());
        if(!Seed(probe,c,lookout))return 1;
        const auto name=cp::SaveBasename(*cp::SaveTarget(0,cp::SaveKind::Autosave));
        if(io::Write(directory,name,c).status!=io::Status::Committed)return 1;
        std::ofstream manifest(directory/"manifest.json",std::ios::binary);
        manifest<<"{\"evidenceClass\":\"seeded noncombat lookout recovery; not earned progression\","
            "\"record\":\""<<name<<"\",\"checkpoint\":\"ForestEndpoint\",\"contentRevision\":2,\"hearts\":2}\n";
        manifest.close();
        if(!manifest)return 1;
        std::cout<<"Exported one create-only seeded lookout autosave to "<<directory.string()<<'\n';return 0;
    }
    std::ofstream manifest(directory/"manifest.json",std::ios::binary);
    if(!manifest)return 2;
    manifest<<"{\"evidenceClass\":\"seeded persistence fixtures; not normal campaign completion\",\"hashAlgorithm\":\"fnv1a64\",\"records\":[";
    for(unsigned slot=0;slot<3;++slot)for(const auto kind:{cp::SaveKind::Manual,cp::SaveKind::Autosave}) {
        const bool upper=kind==cp::SaveKind::Autosave;
        auto checkpoint=Record(upper?3-static_cast<int>(slot):1+static_cast<int>(slot),upper);
        checkpoint.swordStowed=slot==1;checkpoint.lanternHigh=upper && slot==2;
        const auto name=cp::SaveBasename(*cp::SaveTarget(slot,kind));
        if(io::Write(directory,name,checkpoint).status!=io::Status::Committed){std::cerr<<"Create-only fixture export failed.\n";return 1;}
        io::StoredCheckpoint stored;
        if(io::Read(directory,name,stored).status!=io::Status::Read)return 1;
        std::uint64_t hash=14695981039346656037ull;
        for(auto byte:stored.encoded){hash^=std::to_integer<unsigned char>(byte);hash*=1099511628211ull;}
        if(slot || upper)manifest<<',';
        manifest<<"{\"name\":\""<<name<<"\",\"slot\":"<<slot+1<<",\"kind\":\""<<(upper?"autosave":"manual")
                <<"\",\"checkpoint\":\""<<(upper?"SafeExit":"LowerClaimed")<<"\",\"hearts\":"<<checkpoint.vitality
                <<",\"firstAscentCompleted\":"<<(checkpoint.firstAscentCompleted?"true":"false")
                <<",\"swordStowed\":"<<(checkpoint.swordStowed?"true":"false")
                <<",\"lanternHigh\":"<<(checkpoint.lanternHigh?"true":"false")
                <<",\"waitingForRaise\":"<<(checkpoint.dialogue.waitingForRaise?"true":"false")
                <<",\"bytes\":"<<stored.encoded.size()<<",\"hash\":\""<<std::hex<<std::setw(16)<<std::setfill('0')<<hash<<std::dec<<"\"}";
    }
    manifest<<"]}\n";manifest.close();if(!manifest)return 1;
    std::cout<<"Exported exactly six seeded .hrd records and manifest to "<<directory.string()<<'\n';return 0;
}
bool LoadEarnedBoundary(cp::PlaySession& play,sim::GameSimulation& game,cp::SaveKind kind,
    const io::StoredCheckpoint& earned,const std::shared_ptr<const sim::TriangleSupportSurface>& terrain,
    sim::InputSnapshot& input,std::uint64_t& publication) {
    const bool read=play.Load(0,kind) && Until(play,[&]{return play.State().phase==cp::SessionPhase::LoadReady;});
    Check(read,"fresh production session reads the chapter-earned record through the real worker");
    if(!read)return false;
    const auto preparation=play.PrepareLoad();
    Check(preparation.has_value(),"earned record prepares a fresh logical restore");
    if(!preparation)return false;
    input.damageEnabled=false;input.tutorialEnabled=false;
    input.moveForward=1;input.runHeld=true;input.crouchHeld=true;
    input.commands.attack=71;input.commands.interact=72;input.commands.parry=73;input.commands.dodge=74;
    const bool committed=play.CommitLoad(*preparation,preparation->destination,terrain,input,++publication);
    Check(committed,"matching CPU resource readiness commits the actual earned record, not a fixture import");
    if(!committed)return false;
    const auto recaptured=game.CaptureCampaignCheckpoint(earned.checkpoint.id);
    std::vector<std::byte> encoded;cp::codec::Error error{};
    Check(recaptured && cp::codec::Encode(*recaptured,encoded,error) && encoded==earned.encoded,
          "fresh restored simulation round trips every earned schema-1 checkpoint byte");
    const auto& restored=game.Snapshot();
    Check(restored.playerAlive && restored.playerGrounded && !restored.rescue.equipmentStowed &&
          restored.playerVitals.vitality==earned.checkpoint.vitality && restored.rescue.claimCount==1 &&
          restored.rescue.deploymentCount==1 && restored.skeletonEnemies[0].dead && restored.skeletonEnemies[1].dead &&
          restored.lich.phase==horde::gameplay::LichPhase::Dead &&
          restored.chestReward.phase==horde::gameplay::interactions::ChestRewardPhase::LanternClaimed,
          "earned load preserves living support, victory, one reward and one deployed rope");
    // CPU resource injection only: real imported terrain/collision is admitted,
    // while GPU residency/presentation still belongs to the platform adapter.
    const auto generation=restored.worldRoute.generation;
    Check(game.PublishWorldZoneReadiness({generation,sim::WorldZoneId::Dungeon},sim::ZoneReadiness::Ready),
          "resident CPU Dungeon readiness is republished for earned recovery");
    for(std::size_t zone=1;zone<sim::kWorldZones.size();++zone)
        Check(game.PublishChapterTerrain({generation,static_cast<sim::WorldZoneId>(zone)},terrain),
              "earned recovery admits actual imported terrain for CPU route continuation");
    game.StepFixed(input,1.f/60.f,++publication);game.ClearEvents();
    Check(input.moveForward==0 && !input.runHeld && !input.crouchHeld &&
          game.Snapshot().playerCombat.action==horde::gameplay::PlayerCombatAction::Idle && !game.Snapshot().dodgeActive &&
          !game.Snapshot().rescue.equipmentStowed,
          "fresh neutral publication releases earned restore without replaying old action or traversal debt");
    return true;
}
// Read the real lower-return record copied by the scoped native launcher. This
// CPU continuation grants no new story flags and never writes a checkpoint.
int CheckActualReturnRecovery(const fs::path& requested,const std::shared_ptr<const sim::TriangleSupportSurface>& terrain) {
    using namespace horde::gameplay;
    std::error_code error;const auto directory=fs::absolute(requested,error).lexically_normal();
    const auto reports=fs::canonical(fs::path(HORDE_RT_SOURCE_DIR)/"reports",error);
    if(error || !directory.filename().string().starts_with("horde-campaign-fixtures-") ||
       fs::canonical(directory,error)!=directory || error || fs::canonical(directory.parent_path(),error)!=reports || error)return 2;
    std::ifstream manifest(directory/"manifest.json",std::ios::binary);
    const std::string metadata((std::istreambuf_iterator<char>(manifest)),{});
    if(metadata.size()>4096 || metadata.find("recovery of actual R3 lower-return autosave; original post-Keeper seed remains explicit")==std::string::npos)return 2;
    io::StoredCheckpoint original;
    if(io::Read(directory,"slot-1-manual.hrd",original).status!=io::Status::Read ||
       !validation::IsRecordedR3LowerReturn(original.encoded) ||
       original.checkpoint.id!=cp::CheckpointId::LowerClaimed || original.checkpoint.contentRevision!=2 ||
       !cp::CompletedForestProgress(original.checkpoint.dialogue))return 2;
    const auto& saved=original.checkpoint;
    sim::GameSimulation game(sim::ProductionGameSimulationConfig());
    Check(Seed(game,saved,terrain),"actual copied lower-return record commits without new logical flags");
    const auto generation=game.Snapshot().worldRoute.generation;
    for(std::size_t zone=1;zone<sim::kWorldZones.size();++zone)
        Check(game.PublishChapterTerrain({generation,static_cast<sim::WorldZoneId>(zone)},terrain),"CPU continuation admits actual bare terrain, not native GPU readiness");
    sim::InputSnapshot input;input.damageEnabled=input.tutorialEnabled=false;
    input.yawRadians=saved.yaw;input.pitchRadians=saved.pitch;input.paused=true;
    std::uint64_t publication=0;
    game.SynchronizePausedInput(input,++publication);game.StepFixed(input,0,++publication);input.paused=false;
    const auto step=[&]{game.StepFixed(input,1.f/60.f,++publication);game.ClearEvents();};
    for(unsigned tick=0;tick<120;++tick)step();
    const auto coherent=[&]{const auto& s=game.Snapshot();return s.modernCombatRules && s.playerVitals.vitality==saved.vitality &&
        s.rescue.claimCount==1 && s.rescue.deploymentCount==1 && s.rescue.firstAscentCompleted &&
        s.chapterDialogue.proofAccepted && s.chapterDialogue.forestVillageConsumed && s.chapterDialogue.line==dialogue::Line::None &&
        s.chapterCompanion.grounded && s.chapterCompanion.waypoint==sim::kWorldRoutePoints.size();};
    Check(coherent() && !game.Snapshot().rescue.exteriorSide &&
        (game.Snapshot().interaction.heldLightPose==interactions::HeldLightPose::High)==saved.lanternHigh,
        "lower load preserves original health equipment and completed story before repeat ascent");
    Check(game.Snapshot().rescuePrompt==traversal::RescuePrompt::Climb,"actual loaded lower safe side offers ordinary Climb");
    ++input.commands.interact;bool stowed=false,ascent=false,pullUp=false;
    for(unsigned tick=0;tick<1800;++tick){step();const auto& s=game.Snapshot();
        stowed|=s.rescue.equipmentStowed;ascent|=s.rescue.sawAscent;pullUp|=s.rescue.sawPullUp;
        if(s.rescue.exteriorSide && !s.rescue.equipmentStowed)break;}
    for(unsigned tick=0;tick<120;++tick)step();
    const auto& s=game.Snapshot();
    Check(stowed && ascent && pullUp && s.rescue.exteriorSide && s.playerGrounded && coherent(),
        "ordinary repeated ascent retains grounded story and one reward without healing or combat");
    Check((s.interaction.heldLightPose==interactions::HeldLightPose::High)==saved.lanternHigh,
        "completed reunion return load must not reapply first-arrival lantern lowering");
    const auto upper=game.LatestCampaignBoundary();
    Check(upper && upper->id==cp::CheckpointId::SafeExit && upper->contentRevision==2 &&
        upper->lanternHigh==saved.lanternHigh && upper->vitality==saved.vitality && cp::CompletedForestProgress(upper->dialogue),
        "repeat ascent latches coherent upper revision two with the original held configuration");
    io::StoredCheckpoint retained;
    Check(io::Read(directory,"slot-1-manual.hrd",retained).status==io::Status::Read && retained.encoded==original.encoded,
        "CPU continuation preserves the actual source record bytes");
    std::cout<<checks<<" actual-return recovery checks, "<<failures<<" failures\n";return failures?1:0;
}
// Earning uses no checkpoint imports, restore seeds, authoritative poses or
// direct logical mutation: movement, sword edges, chest interaction and rope
// motor only. Recovery then loads only the records this journey really wrote.
void NormalBoundaryJourney(const std::shared_ptr<const sim::TriangleSupportSurface>& terrain) {
    using namespace horde::gameplay;
    namespace tr=horde::gameplay::traversal;
    Directory directory;
    sim::GameSimulation game(sim::ProductionGameSimulationConfig());
    game.StartLiveDevelopmentRescueJourney(true);
    for(std::size_t zone=1;zone<sim::kWorldZones.size();++zone)
        Check(game.PublishChapterTerrain({game.Snapshot().worldRoute.generation,static_cast<sim::WorldZoneId>(zone)},terrain),
              "ordinary journey admits current native terrain (CPU fixture, no GPU claim)");
    cp::PlaySession play(game,directory.path);Check(play.BeginNewGame(0),"ordinary journey selects new game slot");
    sim::InputSnapshot input;input.damageEnabled=false;input.tutorialEnabled=false;
    std::uint64_t publication=0;
    bool observedVillageSpeech=false;
    const auto step=[&](){
        game.StepFixed(input,1.f/60.f,++publication);game.ClearEvents();
        if(!observedVillageSpeech && game.Snapshot().chapterDialogue.line==dialogue::Line::ForestVillage) {
            observedVillageSpeech=true;
            Check(!game.CaptureAuthoredCampaignBoundary(cp::CheckpointId::ForestEndpoint),
                  "village consumed-at-start flag cannot publish endpoint while final speech is active");
        }
    };
    const auto aimMove=[&](float x,float z,float stop=.03f){
        const auto& s=game.Snapshot();const float dx=x-s.playerX,dz=z-s.playerZ,d=std::hypot(dx,dz);
        input.yawRadians=std::atan2(dx,-dz);input.pitchRadians=0;input.moveStrafe=0;
        const float lookahead=std::min(d,.30f),scale=d>.0001f?lookahead/d:0.f;
        input.crouchHeld=!sim::AuthoredBodySweepClear(s.playerX,s.playerZ,s.playerX+dx*scale,s.playerZ+dz*scale,
            s.playerSupportWorldY,sim::kStandingBodyHeight,kPlayerCollisionRadius);
        input.moveForward=d>stop?std::min(1.f,d/.15f):0.f;return d;
    };
    const auto walk=[&](float x,float z){
        for(int tick=0;tick<2400;++tick){if(aimMove(x,z)<.035f){input.moveForward=0;step();return true;}step();}
        std::cerr<<"ordinary route blocked at "<<game.Snapshot().playerX<<','<<game.Snapshot().playerZ<<" target="<<x<<','<<z<<'\n';return false;
    };
    for(auto p:std::array<std::array<float,2>,8>{{{{0,-1}},{{0,-4.8f}},{{0,-9.4f}},{{4.2f,-9.4f}},
          {{4.2f,-14.6f}},{{1,-14.6f}},{{-1.8f,-15.2f}},{{-3.2f,-15.2f}}}})
        if(!walk(p[0],p[1])){Check(false,"normal input opening/waterfall route reaches guards");return;}
    Check(!game.LatestCampaignBoundary(),"no pre-victory milestone can publish a checkpoint");
    for(int tick=0;tick<2400;++tick){
        const auto s=game.Snapshot();int target=-1;float nearest=100;
        for(unsigned i=0;i<s.skeletonEnemyCount;++i)if(!s.skeletonEnemies[i].dead){
            const auto& e=s.skeletonEnemies[i];const float d=std::hypot(e.x-s.playerX,e.z-s.playerZ);
            if(d<nearest){target=static_cast<int>(i);nearest=d;}}
        if(target<0)break;
        const auto& e=s.skeletonEnemies[target];aimMove(e.x,e.z,1.05f);
        if(nearest<1.55f && s.playerCombat.action==PlayerCombatAction::Idle)++input.commands.attack;
        step();
    }
    Check(game.Snapshot().skeletonEnemies[0].dead && game.Snapshot().skeletonEnemies[1].dead,"normal sword input defeats both guards");
    if(!game.Snapshot().skeletonEnemies[0].dead || !game.Snapshot().skeletonEnemies[1].dead)return;
    if(!walk(-31.2f,-15.2f)){Check(false,"normal input reaches Keeper reveal");return;}
    for(int tick=0;tick<2400 && game.Snapshot().lich.phase!=LichPhase::Dead;++tick){
        const auto s=game.Snapshot();const float d=aimMove(s.lich.x,s.lich.z,1.15f);
        if(s.lich.revealComplete && s.lich.hitCooldownRemaining<=0 && d<1.8f &&
           s.playerCombat.action==PlayerCombatAction::Idle)++input.commands.attack;
        step();
    }
    Check(game.Snapshot().lich.phase==LichPhase::Dead,"normal sword input defeats Keeper");
    if(game.Snapshot().lich.phase!=LichPhase::Dead)return;
    if(!walk(-34.f,-17.55f)){Check(false,"normal input reaches real chest interaction point");return;}
    input.moveForward=0;input.yawRadians=-1.57079632679f;
    for(int tick=0;tick<600 && game.Snapshot().chestReward.phase!=interactions::ChestRewardPhase::LanternClaimed;++tick){
        const auto prompt=game.Snapshot().chestPrompt;
        if(prompt==interactions::ChestRewardPrompt::OpenChest || prompt==interactions::ChestRewardPrompt::ClaimLantern)
            ++input.commands.interact;
        step();
    }
    Check(game.Snapshot().chestReward.phase==interactions::ChestRewardPhase::LanternClaimed,"normal chest edges claim reward");
    if(game.Snapshot().chestReward.phase!=interactions::ChestRewardPhase::LanternClaimed)return;
    for(int tick=0;tick<600 && !game.LatestCampaignBoundary();++tick)step();
    const auto lower=game.LatestCampaignBoundary();
    std::cout<<"ordinary lower live="<<game.Snapshot().playerX<<','<<game.Snapshot().playerZ
             <<" boundary="<<bool(lower)<<" rope="<<game.Snapshot().rescue.ropeReady<<'\n';
    Check(lower && lower->id==cp::CheckpointId::LowerClaimed && lower->position==tr::kLowerLanding &&
          !game.CaptureCampaignCheckpoint(cp::CheckpointId::LowerClaimed),
          "normal claimed milestone publishes canonical lower anchor without a 4cm live stop");
    if(!lower)return;
    Check(play.ObserveSafeBoundary() && play.LatestCheckpoint()->id==cp::CheckpointId::LowerClaimed,
          "facade consumes actual fixed-tick lower milestone");
    Check(Until(play,[&]{return play.State().operation==cp::PlayOperation::Autosave &&
          play.State().outcome==cp::PlayOutcome::Succeeded;}),"chapter-earned lower autosave completes real disk publication");
    const auto earnedLower=Read(directory,0,cp::SaveKind::Autosave);
    Check(earnedLower.checkpoint.id==cp::CheckpointId::LowerClaimed &&
          earnedLower.checkpoint.position==tr::kLowerLanding &&
          play.State().result.publication==io::Publication::Published,
          "earned lower autosave is the supported canonical safe anchor and has a real publication receipt");
    Check(play.SaveManual(0) && Done(play) && play.State().outcome==cp::PlayOutcome::Succeeded,
          "chapter-earned lower manual checkpoint completes real disk publication");
    const auto retainedManual=Raw(Path(directory,0,cp::SaveKind::Manual));
    Check(retainedManual==earnedLower.encoded,"earned lower manual and automatic records contain the same coherent checkpoint");
    {
        sim::GameSimulation fresh(sim::ProductionGameSimulationConfig());cp::PlaySession reloaded(fresh,directory.path);
        sim::InputSnapshot freshInput;std::uint64_t freshPublication=0;
        if(!LoadEarnedBoundary(reloaded,fresh,cp::SaveKind::Manual,earnedLower,terrain,freshInput,freshPublication))return;
        Check(!fresh.Snapshot().rescue.exteriorSide && !fresh.Snapshot().rescue.firstAscentCompleted &&
              fresh.Snapshot().rescuePrompt==tr::RescuePrompt::Climb,
              "actual earned lower save restores before first ascent with usable rope");
    }
    Check(!game.CaptureAuthoredCampaignBoundary(cp::CheckpointId::SafeExit),"claim cannot project an uncompleted ascent");
    if(!walk(-33.7f,-15.9f)){Check(false,"normal input reaches broad rope approach");return;}
    input.moveForward=0;++input.commands.interact;step();
    Check(game.Snapshot().rescue.equipmentStowed,"ordinary interaction commits ascent from broad approach");
    Check(!game.CaptureAuthoredCampaignBoundary(cp::CheckpointId::LowerClaimed) &&
          !game.CaptureAuthoredCampaignBoundary(cp::CheckpointId::SafeExit),"mid-climb cannot project either checkpoint");
    input.moveForward=1;input.yawRadians=0;
    // Poll facade only AFTER multi-tick frames and continued held movement.
    // An exact endpoint lasting one fixed tick must not be lost to frame rate.
    for(int frame=0;frame<450 && !game.Snapshot().rescue.exteriorSide;++frame)
        game.AdvanceFrame(input,1.0/15.0,++publication);
    for(int frame=0;frame<20;++frame)game.AdvanceFrame(input,1.0/15.0,++publication);
    const auto upper=game.LatestCampaignBoundary();
    std::cout<<"ordinary upper live="<<game.Snapshot().playerX<<','<<game.Snapshot().playerZ
             <<" speed="<<game.Snapshot().playerMovementSpeedMetresPerSecond<<" boundary="<<bool(upper)<<'\n';
    Check(game.Snapshot().rescue.firstAscentCompleted && upper && upper->id==cp::CheckpointId::SafeExit &&
          upper->position==tr::kExteriorLanding && !game.CaptureCampaignCheckpoint(cp::CheckpointId::SafeExit),
          "actual ascent boundary survives held movement and multi-tick AdvanceFrame");
    Check(play.ObserveSafeBoundary() && play.LatestCheckpoint()->id==cp::CheckpointId::SafeExit,
          "facade receives retained upper boundary after leaving exact endpoint");
    const auto beforeUpperAuto=play.State().operationRevision;
    Check(Until(play,[&]{return play.State().operationRevision>beforeUpperAuto &&
          play.State().operation==cp::PlayOperation::Autosave && play.State().outcome==cp::PlayOutcome::Succeeded;}),
          "chapter-earned upper autosave completes a new real disk publication");
    const auto earnedUpper=Read(directory,0,cp::SaveKind::Autosave);
    Check(earnedUpper.checkpoint.id==cp::CheckpointId::SafeExit && earnedUpper.checkpoint.firstAscentCompleted &&
          earnedUpper.checkpoint.position==tr::kExteriorLanding && retainedManual==Raw(Path(directory,0,cp::SaveKind::Manual)),
          "committed earned exterior autosave retains the earlier lower manual bytes");
    {
        sim::GameSimulation fresh(sim::ProductionGameSimulationConfig());cp::PlaySession reloaded(fresh,directory.path);
        sim::InputSnapshot freshInput;std::uint64_t freshPublication=0;
        if(!LoadEarnedBoundary(reloaded,fresh,cp::SaveKind::Autosave,earnedUpper,terrain,freshInput,freshPublication))return;
        Check(fresh.Snapshot().rescue.exteriorSide && fresh.Snapshot().rescuePrompt==tr::RescuePrompt::Descend,
              "actual earned upper autosave restores outside with usable return descent");
        ++freshInput.commands.interact;fresh.StepFixed(freshInput,1.f/60.f,++freshPublication);
        for(int tick=0;tick<60;++tick){fresh.StepFixed(freshInput,1.f/60.f,++freshPublication);reloaded.ObserveSafeBoundary();reloaded.Poll();}
        Check(fresh.Snapshot().rescue.equipmentStowed && fresh.LatestCampaignBoundary() &&
              fresh.LatestCampaignBoundary()->id==cp::CheckpointId::SafeExit &&
              Raw(Path(directory,0,cp::SaveKind::Autosave))==earnedUpper.encoded,
              "interrupted earned return descent retains the committed exterior record, never a mid-rope save");
    }
    {
        sim::GameSimulation fresh(sim::ProductionGameSimulationConfig());cp::PlaySession reloaded(fresh,directory.path);
        sim::InputSnapshot freshInput;std::uint64_t freshPublication=0;
        if(!LoadEarnedBoundary(reloaded,fresh,cp::SaveKind::Autosave,earnedUpper,terrain,freshInput,freshPublication))return;
        Check(fresh.Snapshot().rescue.exteriorSide && !fresh.Snapshot().rescue.equipmentStowed,
              "fresh recovery after interrupted return starts on the last committed exterior safe side");
        ++freshInput.commands.interact;fresh.StepFixed(freshInput,1.f/60.f,++freshPublication);
        for(int tick=0;tick<1800 && fresh.Snapshot().rescue.equipmentStowed;++tick)
            fresh.StepFixed(freshInput,1.f/60.f,++freshPublication);
        Check(!fresh.Snapshot().rescue.exteriorSide && !fresh.Snapshot().rescue.equipmentStowed &&
              fresh.Snapshot().rescue.firstAscentCompleted && fresh.Snapshot().playerGrounded &&
              fresh.Snapshot().rescue.sawDescent && fresh.Snapshot().rescue.sawLanding &&
              std::abs(fresh.Snapshot().playerSupportWorldY-tr::kLowerSupportWorldY)<.001f &&
              fresh.Snapshot().rescue.claimCount==1 && fresh.Snapshot().rescue.deploymentCount==1,
              "earned upper recovery completes actual ordinary-input descent and landing without duplicate reward or rope");
        Check(Raw(Path(directory,0,cp::SaveKind::Autosave))==earnedUpper.encoded &&
              Raw(Path(directory,0,cp::SaveKind::Manual))==retainedManual,
              "continuation alone cannot rewrite the stored earned records without a facade boundary observation");
    }
    // Continue the SAME normally earned campaign. Only the existing host
    // presentation protocol is acknowledged here; this is not GPU evidence.
    Check(!game.CaptureAuthoredCampaignBoundary(cp::CheckpointId::ForestEndpoint),
          "summit arrival cannot project an unearned forest endpoint");
    const auto reunion=sim::kWorldRoutePoints[3];
    if(!walk(reunion.x,reunion.z)){Check(false,"earned campaign walks to the actual reunion");return;}
    input.moveForward=0;input.runHeld=false;input.crouchHeld=false;
    for(int tick=0;tick<600 && !game.Snapshot().chapterDialogue.waitingForRaise;++tick)step();
    Check(game.Snapshot().chapterDialogue.waitingForRaise && !game.Snapshot().chapterDialogue.proofAccepted,
          "earned forest progression begins with the real fresh-raise lesson");
    ++input.commands.toggleHeldLightPose;
    for(int tick=0;tick<60;++tick)step();
    Check(!game.Snapshot().chapterDialogue.proofAccepted,"new lantern raise still requires matching presentation");
    const auto shown=game.Snapshot();
    Check(game.AcknowledgeChapterPresentation(shown.tickIndex,shown.chapterDialogue.generation),
          "host owner acknowledges only the current earned lantern publication");
    step();Check(game.Snapshot().chapterDialogue.proofAccepted,"fresh raised publication unlocks earned forest route");
    for(int tick=0;tick<900 && game.Snapshot().chapterDialogue.line!=dialogue::Line::None;++tick)step();
    for(std::size_t station=4;station<sim::kWorldRoutePoints.size();++station) {
        const auto p=sim::kWorldRoutePoints[station];
        if(!walk(p.x,p.z)){Check(false,"ordinary movement follows the full earned forest trail");return;}
    }
    input.moveForward=0;input.runHeld=false;input.crouchHeld=false;input.yawRadians=2.51f;input.pitchRadians=-.15f;
    for(int tick=0;tick<1800;++tick) {
        if(game.LatestCampaignBoundary() && game.LatestCampaignBoundary()->id==cp::CheckpointId::ForestEndpoint)break;
        step();
    }
    const auto endpoint=game.LatestCampaignBoundary();
    Check(observedVillageSpeech,"ordinary F04 arrival reaches the actual admitted village speech");
    Check(endpoint && endpoint->id==cp::CheckpointId::ForestEndpoint && endpoint->contentRevision==2 &&
          endpoint->zone==sim::WorldZoneId::Lookout && endpoint->dialogue.proofAccepted &&
          endpoint->dialogue.forestNightConsumed && endpoint->dialogue.forestWaystoneConsumed &&
          endpoint->dialogue.forestClueConsumed && endpoint->dialogue.forestVillageConsumed &&
          endpoint->dialogue.activeLine==dialogue::Line::None && endpoint->dialogue.continuationCount==0,
          "actual completed trail and quiescent speech earn one coherent forest checkpoint");
    if(!endpoint || endpoint->id!=cp::CheckpointId::ForestEndpoint)return;
    const auto ground=terrain->Query(sim::kWorldRoutePoints.back().x,sim::kWorldRoutePoints.back().z);
    Check(ground.found && std::abs(endpoint->position.y-ground.worldY)<.001f &&
          std::abs(endpoint->position.x-sim::kWorldRoutePoints.back().x)<.001f &&
          std::abs(endpoint->position.z-sim::kWorldRoutePoints.back().z)<.001f &&
          endpoint->yaw==2.51f && endpoint->pitch==-.15f && game.Snapshot().chapterCompanion.grounded &&
          game.Snapshot().chapterCompanion.waypoint==sim::kWorldRoutePoints.size(),
          "earned endpoint uses actual ground, authored view and completed grounded companion");
    const auto beforeEndpointAuto=play.State().operationRevision;
    Check(play.ObserveSafeBoundary() && Until(play,[&]{return play.State().operationRevision>beforeEndpointAuto &&
          play.State().operation==cp::PlayOperation::Autosave && play.State().outcome==cp::PlayOutcome::Succeeded;}),
          "earned endpoint publishes a fresh autosave through the real worker");
    const auto earnedEndpoint=Read(directory,0,cp::SaveKind::Autosave);
    Check(earnedEndpoint.checkpoint.id==cp::CheckpointId::ForestEndpoint &&
          Raw(Path(directory,0,cp::SaveKind::Manual))==retainedManual,
          "forest latest auto replaces summit while preserving the original lower manual bytes");
    const auto endpointOperation=play.State().operationRevision;
    const auto backtrack=sim::kWorldRoutePoints[6];
    if(!walk(backtrack.x,backtrack.z)){Check(false,"endpoint owner can keep exploring away from F04");return;}
    input.moveForward=0;
    for(int tick=0;tick<120;++tick){step();play.ObserveSafeBoundary();play.Poll();}
    Check(game.LatestCampaignBoundary() && game.LatestCampaignBoundary()->id==cp::CheckpointId::ForestEndpoint &&
          play.LatestCheckpoint() && play.LatestCheckpoint()->id==cp::CheckpointId::ForestEndpoint &&
          play.State().operationRevision==endpointOperation && Raw(Path(directory,0,cp::SaveKind::Autosave))==earnedEndpoint.encoded,
          "exterior exploration cannot downgrade the endpoint or repeatedly autosave it");
    {
        sim::GameSimulation fresh(sim::ProductionGameSimulationConfig());cp::PlaySession reloaded(fresh,directory.path);
        sim::InputSnapshot freshInput;std::uint64_t freshPublication=0;
        if(!LoadEarnedBoundary(reloaded,fresh,cp::SaveKind::Autosave,earnedEndpoint,terrain,freshInput,freshPublication))return;
        const auto operation=reloaded.State().operationRevision;
        for(int tick=0;tick<120;++tick){fresh.StepFixed(freshInput,1.f/60.f,++freshPublication);reloaded.ObserveSafeBoundary();reloaded.Poll();}
        Check(reloaded.State().operationRevision==operation && Raw(Path(directory,0,cp::SaveKind::Autosave))==earnedEndpoint.encoded &&
              fresh.Snapshot().chapterDialogue.line==dialogue::Line::None && fresh.Snapshot().chapterCompanion.grounded &&
              fresh.Snapshot().chapterCompanion.waypoint==sim::kWorldRoutePoints.size(),
              "fresh endpoint load resumes completed Kit/story without rewriting or autosaving the loaded record");
        const auto returnTo=[&](float x,float z) {
            for(int tick=0;tick<2400;++tick) {
                const auto s=fresh.Snapshot();const float dx=x-s.playerX,dz=z-s.playerZ,d=std::hypot(dx,dz);
                freshInput.yawRadians=std::atan2(dx,-dz);freshInput.moveStrafe=0;freshInput.moveForward=d<.035f?0:std::min(1.f,d/.15f);
                freshInput.runHeld=false;freshInput.crouchHeld=false;
                fresh.StepFixed(freshInput,1.f/60.f,++freshPublication);fresh.ClearEvents();
                if(d<.035f)return true;
            }
            return false;
        };
        for(std::size_t station=sim::kWorldRoutePoints.size()-1;station-->2;) {
            const auto p=sim::kWorldRoutePoints[station];
            if(!returnTo(p.x,p.z)){Check(false,"earned endpoint restore walks back along the actual forest route");return;}
        }
        const auto landing=tr::kExteriorLanding;
        if(!returnTo(landing.x,landing.z)){Check(false,"earned endpoint restore reaches the actual return rope");return;}
        ++freshInput.commands.interact;fresh.StepFixed(freshInput,1.f/60.f,++freshPublication);
        Check(fresh.Snapshot().rescue.equipmentStowed,"endpoint return begins actual descent with both items stowed");
        for(int tick=0;tick<1800 && fresh.Snapshot().rescue.equipmentStowed;++tick)
            fresh.StepFixed(freshInput,1.f/60.f,++freshPublication);
        for(int tick=0;tick<60;++tick)fresh.StepFixed(freshInput,1.f/60.f,++freshPublication);
        const auto lowerReturn=fresh.LatestCampaignBoundary();
        Check(fresh.Snapshot().rescue.sawLanding && !fresh.Snapshot().rescue.exteriorSide &&
              lowerReturn && lowerReturn->id==cp::CheckpointId::LowerClaimed && lowerReturn->contentRevision==2 &&
              lowerReturn->dialogue.proofAccepted && lowerReturn->dialogue.forestVillageConsumed &&
              fresh.Snapshot().chapterCompanion.waypoint==sim::kWorldRoutePoints.size(),
              "completed descent replaces endpoint with coherent lower safe side while Kit/story remain completed");
        // Do not observe/save this return: preserve the earned endpoint record
        // for independent failure/no-rewrite receipts above.
        Check(Raw(Path(directory,0,cp::SaveKind::Autosave))==earnedEndpoint.encoded,
              "unobserved return boundary does not itself perform filesystem IO");
    }
    Check(!fs::exists(Path(directory,1,cp::SaveKind::Manual)) && !fs::exists(Path(directory,1,cp::SaveKind::Autosave)) &&
          !fs::exists(Path(directory,2,cp::SaveKind::Manual)) && !fs::exists(Path(directory,2,cp::SaveKind::Autosave)),
          "chapter-earned persistence and recovery never touch unselected slots");
    const auto oldGeneration=game.Snapshot().worldRoute.generation;
    game.InvalidateWorldZoneReadiness();
    Check(!game.CaptureAuthoredCampaignBoundary(cp::CheckpointId::SafeExit) &&
          !game.PublishChapterTerrain({oldGeneration,sim::WorldZoneId::TombExterior},terrain),
          "stale/reconstructing resources cannot produce a new projected checkpoint");
    using Surface=sim::TriangleSupportSurface;
    const float x=tr::kExteriorLanding.x,z=tr::kExteriorLanding.z,y=tr::kExteriorLanding.y+1.5f;
    const std::array<Surface::Triangle,2> roof{{{{{{x-1,y,z-1},{x+1,y,z+1},{x-1,y,z+1}}},sim::SupportSurface::Wood},
                                             {{{{x-1,y,z-1},{x+1,y,z-1},{x+1,y,z+1}}},sim::SupportSurface::Wood}}};
    std::string diagnostic;
    const auto unsafe=Surface::WithAdditionalNonWalkableGeometry(*terrain,roof,diagnostic);
    Check(unsafe && game.PublishChapterTerrain({game.Snapshot().worldRoute.generation,sim::WorldZoneId::TombExterior},unsafe) &&
          !game.CaptureAuthoredCampaignBoundary(cp::CheckpointId::SafeExit),
          "standing respawn headroom is checked at canonical anchor even when current player has left it");
    game.ResetRoute();Check(!game.LatestCampaignBoundary(),"actual route reset clears shared checkpoint latch");
    play.ObserveSafeBoundary();Check(!play.LatestCheckpoint(),"facade discards old game latch after actual reset");
}
void EndpointWriteFailure(const std::shared_ptr<const sim::TriangleSupportSurface>& terrain) {
    Directory directory;Faults faults;sim::GameSimulation game(sim::ProductionGameSimulationConfig());
    cp::PlaySession play(game,directory.path,&faults);
    Check(play.BeginNewGame(0) && Seed(game,Record(2,true),terrain) && play.ObserveSafeBoundary() &&
          Until(play,[&]{return play.State().operation==cp::PlayOperation::Autosave && play.State().outcome==cp::PlayOutcome::Succeeded;}),
          "endpoint failure fixture commits a real prior summit autosave");
    Check(play.SaveManual(0) && Done(play) && play.State().outcome==cp::PlayOutcome::Succeeded,
          "endpoint failure fixture retains an independent prior manual");
    const auto priorAuto=Raw(Path(directory,0,cp::SaveKind::Autosave));
    const auto priorManual=Raw(Path(directory,0,cp::SaveKind::Manual));
    auto endpoint=Record(2,true);endpoint.contentRevision=2;endpoint.id=cp::CheckpointId::ForestEndpoint;
    endpoint.zone=sim::WorldZoneId::Lookout;const auto p=sim::kWorldRoutePoints.back();
    const auto ground=terrain->Query(p.x,p.z);
    Check(ground.found && ground.walkable,"endpoint failure fixture derives actual terrain support");
    endpoint.position={p.x,ground.worldY,p.z};endpoint.yaw=2.51f;endpoint.pitch=-.15f;
    endpoint.dialogue.waitingForRaise=false;endpoint.dialogue.proofAccepted=true;
    endpoint.dialogue.forestNightConsumed=endpoint.dialogue.forestWaystoneConsumed=true;
    endpoint.dialogue.forestClueConsumed=endpoint.dialogue.forestVillageConsumed=true;
    const auto oldOperation=play.State().operationRevision;
    faults.failWrite=true;
    Check(Seed(game,endpoint,terrain) && play.ObserveSafeBoundary() && Until(play,[&]{
        return play.State().operationRevision>oldOperation && play.State().operation==cp::PlayOperation::Autosave &&
               play.State().outcome==cp::PlayOutcome::Failed;}),
          "failed endpoint autosave has a fresh explicit failure receipt");
    Check(Raw(Path(directory,0,cp::SaveKind::Autosave))==priorAuto && Raw(Path(directory,0,cp::SaveKind::Manual))==priorManual,
          "failed endpoint autosave preserves both prior valid record byte strings");
    Check(play.SaveManual(0) && Done(play) && play.State().outcome==cp::PlayOutcome::Failed &&
          Raw(Path(directory,0,cp::SaveKind::Manual))==priorManual,
          "failed manual endpoint replacement cannot destroy or report the older manual as newly saved");
    faults.failWrite=false;
    Check(play.SaveManual(0) && Done(play) && play.State().outcome==cp::PlayOutcome::Succeeded &&
          Read(directory,0,cp::SaveKind::Manual).checkpoint.id==cp::CheckpointId::ForestEndpoint &&
          Raw(Path(directory,0,cp::SaveKind::Autosave))==priorAuto,
          "explicit endpoint manual retry succeeds without claiming the failed auto was replaced");
    for(unsigned slot=1;slot<3;++slot)for(auto kind:{cp::SaveKind::Manual,cp::SaveKind::Autosave})
        Check(!fs::exists(Path(directory,slot,kind)),"endpoint write failure/retry cannot create records in other slots");
}
}
int main(int argc,char** argv){
    if(argc==3 && std::string_view(argv[1])=="--export-fixtures")return ExportFixtures(argv[2]);
    if(argc==3 && std::string_view(argv[1])=="--export-route-fixture")return ExportFixtures(argv[2],{},true);
    const bool exportLookout=argc==3 && std::string_view(argv[1])=="--export-lookout-fixture";
    const bool actualReturn=argc==3 && std::string_view(argv[1])=="--check-actual-return-recovery";
    if(argc!=1 && !exportLookout && !actualReturn)return 2;
    const auto assets=fs::path(HORDE_RT_SOURCE_DIR)/"assets/models/world/runtime/chapter-terrain-v1";
    horde::scene::assets::AssetManifest manifest;horde::scene::assets::StaticMeshAsset mesh;std::string error;
    if(!horde::scene::assets::AssetManifest::Load(assets/"asset.manifest.json",manifest,error) ||
       !horde::scene::assets::StaticMeshAsset::Load(assets/"chapter-terrain-lod0.runtime.glb",manifest,mesh,error)){
        std::cerr<<error;return 1;}
    const auto terrain=horde::scene::PrepareChapterTerrainSurface(mesh,error);if(!terrain){std::cerr<<error;return 1;}
    if(exportLookout)return ExportFixtures(argv[2],terrain);
    if(actualReturn)return CheckActualReturnRecovery(argv[2],terrain);
    Directory directory;Faults faults;
    Check(io::Write(directory.path,cp::SaveBasename(*cp::SaveTarget(0,cp::SaveKind::Manual)),Record(1)).status==io::Status::Committed,"seed first manual");
    Check(io::Write(directory.path,cp::SaveBasename(*cp::SaveTarget(2,cp::SaveKind::Manual)),Record(2,true)).status==io::Status::Committed,"seed third manual");
    {std::ofstream f(Path(directory,1,cp::SaveKind::Manual),std::ios::binary);f<<"unsupported immutable fixture";}
    fs::create_directory(Path(directory,1,cp::SaveKind::Autosave));
    const auto corrupt=Raw(Path(directory,1,cp::SaveKind::Manual));
    const auto firstManual=Raw(Path(directory,0,cp::SaveKind::Manual));
    const auto thirdManual=Raw(Path(directory,2,cp::SaveKind::Manual));
    sim::GameSimulation game(sim::ProductionGameSimulationConfig());
    cp::PlaySession play(game,directory.path,&faults);
    Check(Until(play,[&]{return CatalogueDone(play);}),"all six catalogue reads finish");
    const auto& cat=play.Catalogue();
#ifdef _WIN32
    // CreateFile without BACKUP_SEMANTICS cannot open the directory as a save.
    constexpr auto directoryStatus=cp::CatalogueStatus::ReadError;
#else
    constexpr auto directoryStatus=cp::CatalogueStatus::Invalid;
#endif
    Check(cat[0].status==cp::CatalogueStatus::Valid && cat[0].checkpoint->vitality==1 &&
          cat[1].status==cp::CatalogueStatus::Empty && cat[2].status==cp::CatalogueStatus::Invalid &&
          cat[3].status==directoryStatus && cat[4].status==cp::CatalogueStatus::Valid &&
          cat[4].checkpoint->id==cp::CheckpointId::SafeExit && cat[5].status==cp::CatalogueStatus::Empty,
          "catalogue distinguishes six fixed records with actual checkpoint/hearts");
    Check(!play.State().activeSlot && game.Snapshot().tickIndex==0 && Raw(Path(directory,1,cp::SaveKind::Manual))==corrupt,
          "catalogue is read-only and does not select a game, apply a load or repair corrupt data");
    faults.failRead=true;play.RefreshCatalogue();
    Check(Until(play,[&]{return CatalogueDone(play);}) && play.Catalogue()[0].status==cp::CatalogueStatus::ReadError &&
          Raw(Path(directory,0,cp::SaveKind::Manual))==firstManual,"read failure is visible and preserves valid bytes");
    Check(Seed(game,Record(3),terrain) && play.ObserveSafeBoundary(),"strict boundary can be latched without active slot");
    for(int i=0;i<30;++i)play.Poll();
    Check(!play.State().activeSlot && !fs::exists(Path(directory,0,cp::SaveKind::Autosave)) &&
          !fs::exists(Path(directory,2,cp::SaveKind::Autosave)),"observing before NewGame/committed Load never autosaves");
    Check(play.BeginNewGame(2) && !play.LatestCheckpoint() && !play.SaveManual(2),"new game clears latch; saving before a boundary rejects");
    Check(Raw(Path(directory,2,cp::SaveKind::Manual))==thirdManual,"new game preserves existing records");
    Check(Seed(game,Record(3),terrain),"owner replaces simulation after NewGame before observing");
    Check(play.ObserveSafeBoundary() && Until(play,[&]{return play.State().operation==cp::PlayOperation::Autosave &&
        play.State().outcome==cp::PlayOutcome::Succeeded;}),"first strict boundary autosaves only after explicit active slot");
    Check(Read(directory,2,cp::SaveKind::Autosave).checkpoint.vitality==3 && Raw(Path(directory,2,cp::SaveKind::Manual))==thirdManual,
          "latest auto is separate from manual");
    const auto autoRevision=play.State().operationRevision;
    for(int i=0;i<30;++i){play.ObserveSafeBoundary();play.Poll();}
    Check(play.State().operationRevision==autoRevision,"same boundary does not cause endless autosaves");
    const auto latched=*play.LatestCheckpoint();
    sim::InputSnapshot input;input.damageEnabled=false;input.yawRadians=latched.yaw;input.pitchRadians=latched.pitch;
    game.StepFixed(input,1.f/60.f,1); // Release restored-input barrier.
    input.moveForward=1;
    for(int i=0;i<30;++i)game.StepFixed(input,1.f/60.f,2+i);
    Check(!game.CaptureCampaignCheckpoint(latched.id) && !play.ObserveSafeBoundary() && play.LatestCheckpoint()->position.x==latched.position.x,
          "movement cannot weaken strict capture or erase immutable latest boundary");
    input.moveForward=0;input.crouchHeld=true;
    for(int i=0;i<18;++i)game.StepFixed(input,1.f/60.f,40+i);
    Check(game.Snapshot().crouchActive && !play.ObserveSafeBoundary() && play.SaveManual(0) && Done(play) &&
          play.State().outcome==cp::PlayOutcome::Succeeded,"manual can save latest boundary after moving/crouching");
    Check(Read(directory,0,cp::SaveKind::Manual).checkpoint.vitality==3 && play.State().activeSlot==2,
          "explicit manual choice does not retarget current game's autosave");
    const auto savedReceipt=play.TakeCompletion();
    Check(savedReceipt && savedReceipt->outcome==cp::PlayOutcome::Succeeded && !play.TakeCompletion(),
          "successful completion is delivered once while State retains success");
    Check(play.Load(2,cp::SaveKind::Manual) && Until(play,[&]{return play.State().phase==cp::SessionPhase::LoadReady;}),"prepare another existing slot");
    const auto abandoned=play.PrepareLoad();const auto before=game.Snapshot();play.Cancel();
    Check(abandoned && play.State().activeSlot==2 && play.State().outcome==cp::PlayOutcome::Cancelled &&
          !play.CommitLoad(*abandoned,abandoned->destination,terrain,input,100) && game.Snapshot().tickIndex==before.tickIndex,
          "cancelled load leaves game active slot and live simulation unchanged");
    // Load target differs from active slot; cancellation cannot retarget auto.
    Check(play.Load(0,cp::SaveKind::Manual) && Until(play,[&]{return play.State().phase==cp::SessionPhase::LoadReady;}),"different load target reads");
    const auto plan=play.PrepareLoad();Check(plan && play.State().activeSlot==2,"load target is not active before commit");
    if(plan){
        const auto old=game.Snapshot();
        Check(!play.CommitLoad(*plan,{plan->destination.generation-1,plan->destination.zone},terrain,input,100) &&
              game.Snapshot().tickIndex==old.tickIndex && play.State().activeSlot==2,"wrong readiness cannot activate target slot");
        input.moveForward=1;input.runHeld=true;input.crouchHeld=true;input.commands.attack=100;input.yawRadians=2;
        Check(play.CommitLoad(*plan,plan->destination,terrain,input,100) && play.State().activeSlot==0 &&
              input.moveForward==0 && !input.runHeld && !input.crouchHeld && input.yawRadians==game.Snapshot().playerYawRadians,
              "commit atomically activates slot and rebases owner controls");
        const auto committedRevision=play.State().operationRevision;
        for(int i=0;i<30;++i){play.ObserveSafeBoundary();play.Poll();}
        Check(play.State().operationRevision==committedRevision && !fs::exists(Path(directory,0,cp::SaveKind::Autosave)),
              "load seeds strict latest latch without automatically writing loaded target");
    }
    Check(Seed(game,Record(2,true),terrain) && play.ObserveSafeBoundary(),"new actual upper boundary observed");
    Check(Until(play,[&]{return play.State().operation==cp::PlayOperation::Autosave && play.State().outcome==cp::PlayOutcome::Succeeded;}),
          "new authored boundary autosaves active loaded slot");
    Check(Read(directory,0,cp::SaveKind::Autosave).checkpoint.id==cp::CheckpointId::SafeExit &&
          Read(directory,0,cp::SaveKind::Manual).checkpoint.id==cp::CheckpointId::LowerClaimed,
          "upper autosave preserves lower manual checkpoint");
    Check(play.SaveManual(1) && Done(play) && play.State().outcome==cp::PlayOutcome::Failed &&
          Raw(Path(directory,1,cp::SaveKind::Manual))==corrupt,"corrupt manual never overwrites or reports success");
    const auto corruptReceipt=play.TakeCompletion();play.RefreshCatalogue();
    Check(corruptReceipt && corruptReceipt->outcome==cp::PlayOutcome::Failed &&
          Until(play,[&]{return CatalogueDone(play);}) && !play.TakeCompletion() && play.State().outcome==cp::PlayOutcome::Failed,
          "catalogue refresh finishes without replaying consumed failed outcome");
    faults.failWrite=true;
    Check(play.SaveManual(0) && Done(play) && play.State().outcome==cp::PlayOutcome::Failed &&
          Read(directory,0,cp::SaveKind::Manual).checkpoint.id==cp::CheckpointId::LowerClaimed,
          "failed write preserves old manual and cannot report saved");
    const auto failedReceipt=play.TakeCompletion();
    Check(failedReceipt && failedReceipt->outcome==cp::PlayOutcome::Failed && !play.TakeCompletion(),"injected failed write completion is single-use");
    faults.failWrite=false;
    Check(play.SaveManual(0) && Done(play),"explicit retry after write failure can run");
    const auto retryReceipt=play.TakeCompletion();
    Check(retryReceipt && retryReceipt->outcome==cp::PlayOutcome::Succeeded && failedReceipt &&
          retryReceipt->operationRevision>failedReceipt->operationRevision && !play.TakeCompletion(),"explicit retry yields one fresh success receipt");
    Check(Seed(game,Record(1,true),terrain) && play.ObserveSafeBoundary(),"new logical value prepares uncertain-publication fixture");
    faults.failAfterPublish=true;
    Check(play.SaveManual(0) && Done(play) && play.State().outcome==cp::PlayOutcome::Failed &&
          play.State().result.status==io::Status::PublishedDurabilityUnknown,"postpublish uncertainty cannot report saved");
    const auto uncertainReceipt=play.TakeCompletion();faults.failAfterPublish=false;play.RefreshCatalogue();
    Check(uncertainReceipt && uncertainReceipt->outcome==cp::PlayOutcome::Failed &&
          Until(play,[&]{return CatalogueDone(play);}) && !play.TakeCompletion(),"uncertain publication is delivered once, fresh catalogue is read-only");
    NormalBoundaryJourney(terrain);
    EndpointWriteFailure(terrain);
    std::cout<<checks<<" campaign play session checks, "<<failures<<" failures\n";return failures?1:0;
}
