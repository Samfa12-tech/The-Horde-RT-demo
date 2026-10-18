#include "gameplay/campaign/CampaignPlaySession.h"
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
int ExportFixtures(const fs::path& requested) {
    std::error_code error;
    const auto directory=fs::absolute(requested,error).lexically_normal();
    if(error || !directory.filename().string().starts_with("horde-campaign-fixtures-") ||
       !fs::is_directory(directory,error) || error || fs::is_symlink(fs::symlink_status(directory,error)) || error ||
       !fs::is_empty(directory,error) || error){std::cerr<<"Explicit caller-created empty horde-campaign-fixtures-* directory required.\n";return 2;}
    const auto parent=fs::canonical(directory.parent_path(),error);if(error)return 2;
    const auto reports=fs::canonical(fs::path(HORDE_RT_SOURCE_DIR)/"reports",error);if(error)return 2;
    const auto temporary=fs::canonical(fs::temp_directory_path(),error);if(error || (parent!=reports && parent!=temporary)){
        std::cerr<<"Fixture directory must be a direct child of repository reports or system temporary directory.\n";return 2;}
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
// No checkpoint imports, restore seeds, authoritative poses or direct logical
// mutation: movement, sword edges, chest interaction and rope motor only.
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
    const auto step=[&](){game.StepFixed(input,1.f/60.f,++publication);game.ClearEvents();};
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
}
int main(int argc,char** argv){
    if(argc==3 && std::string_view(argv[1])=="--export-fixtures")return ExportFixtures(argv[2]);
    if(argc!=1)return 2;
    const auto assets=fs::path(HORDE_RT_SOURCE_DIR)/"assets/models/world/runtime/chapter-terrain-v1";
    horde::scene::assets::AssetManifest manifest;horde::scene::assets::StaticMeshAsset mesh;std::string error;
    if(!horde::scene::assets::AssetManifest::Load(assets/"asset.manifest.json",manifest,error) ||
       !horde::scene::assets::StaticMeshAsset::Load(assets/"chapter-terrain-lod0.runtime.glb",manifest,mesh,error)){
        std::cerr<<error;return 1;}
    const auto terrain=horde::scene::PrepareChapterTerrainSurface(mesh,error);if(!terrain){std::cerr<<error;return 1;}
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
    std::cout<<checks<<" campaign play session checks, "<<failures<<" failures\n";return failures?1:0;
}
