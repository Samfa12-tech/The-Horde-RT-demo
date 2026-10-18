#include "gameplay/campaign/CampaignSession.h"
#include "gameplay/simulation/InputMailbox.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/assets/AssetManifest.h"
#include <chrono>
#include <fstream>
#include <iostream>

namespace cp=horde::gameplay::campaign;
namespace sim=horde::gameplay::simulation;
namespace io=cp::storage;
namespace fs=std::filesystem;
namespace {
int checks=0,failures=0;
void Check(bool value,const char* message){++checks;if(!value){++failures;std::cerr<<message<<'\n';}}
struct Directory {
    fs::path path=fs::temp_directory_path()/("horde-session-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Directory(){if(!fs::create_directory(path))throw std::runtime_error("exclusive test root failed");std::ofstream(path/"owned.txt")<<"fixture";}
    ~Directory(){std::error_code ec;if(path.filename().string().starts_with("horde-session-test-") &&
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
bool Finish(cp::Session& session){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    do {session.Poll();const auto phase=session.State().phase;
        if(phase==cp::SessionPhase::Idle || phase==cp::SessionPhase::LoadReady || phase==cp::SessionPhase::Failed)return true;
        std::this_thread::yield();}while(std::chrono::steady_clock::now()<end);return false;}
io::StoredCheckpoint Read(const fs::path& root,unsigned slot,cp::SaveKind kind){io::StoredCheckpoint record;
    Check(io::Read(root,cp::SaveBasename(*cp::SaveTarget(slot,kind)),record).status==io::Status::Read,"real slot file reads");return record;}
std::vector<std::byte> Raw(const fs::path& path){std::ifstream f(path,std::ios::binary);
    std::vector<std::byte> bytes;char c;while(f.get(c))bytes.push_back(std::byte(static_cast<unsigned char>(c)));return bytes;}
}
int main(){
    const auto assets=fs::path(HORDE_RT_SOURCE_DIR)/"assets/models/world/runtime/chapter-terrain-v1";
    horde::scene::assets::AssetManifest manifest;horde::scene::assets::StaticMeshAsset mesh;std::string error;
    if(!horde::scene::assets::AssetManifest::Load(assets/"asset.manifest.json",manifest,error) ||
       !horde::scene::assets::StaticMeshAsset::Load(assets/"chapter-terrain-lod0.runtime.glb",manifest,mesh,error)){
        std::cerr<<error;return 1;}
    const auto terrain=horde::scene::PrepareChapterTerrainSurface(mesh,error);if(!terrain){std::cerr<<error;return 1;}
    Directory directory;
    auto config=sim::ProductionGameSimulationConfig();config.movementSpeedMetresPerSecond=1.4f;
    sim::GameSimulation game(config);sim::InputSnapshot controls;sim::InputMailbox mailbox;
    {
        cp::Session session(game,directory.path);
        Check(!session.Save(cp::SaveKind::Manual,cp::CheckpointId::LowerClaimed) && !session.SelectSlot(3),"closed session and invalid fourth slot reject");
        for(unsigned slot=0;slot<3;++slot){
            Check(session.SelectSlot(slot) && Seed(game,Record(1),terrain),"selected real slot starts from actual restored safe simulation");
            Check(session.Save(cp::SaveKind::Manual,cp::CheckpointId::LowerClaimed) && Finish(session) && session.State().phase==cp::SessionPhase::Idle,
                "manual captures actual simulation, reads expected version, writes and acknowledges disk");
            const auto manual=Read(directory.path,slot,cp::SaveKind::Manual).encoded;
            Check(Seed(game,Record(2,true),terrain),"gameplay advances to another safe checkpoint");
            Check(session.Save(cp::SaveKind::Autosave,cp::CheckpointId::SafeExit) && Finish(session) && session.State().phase==cp::SessionPhase::Idle,
                "latest autosave uses independent target");
            Check(Read(directory.path,slot,cp::SaveKind::Manual).encoded==manual &&
                Read(directory.path,slot,cp::SaveKind::Autosave).checkpoint.vitality==2,"autosaving never replaces manual bytes");
            Check(Seed(game,Record(3,true),terrain) && session.Save(cp::SaveKind::Autosave,cp::CheckpointId::SafeExit) && Finish(session),
                "later autosave updates only latest auto rather than adding history");
            Check(Read(directory.path,slot,cp::SaveKind::Manual).encoded==manual &&
                Read(directory.path,slot,cp::SaveKind::Autosave).checkpoint.vitality==3,"manual survives repeated autosaves in every slot");
        }
        Check(session.SelectSlot(0) && session.Load(cp::SaveKind::Manual) && Finish(session),"manual load worker reads requested slot");
        const auto before=game.Snapshot();controls.moveForward=1;controls.runHeld=true;controls.yawRadians=2;controls.pitchRadians=.2f;
        controls.commands.attack=90;controls.commands.interact=91;controls.commands.toggleHeldLightPose=92;
        controls.dialogueCompletionGeneration=77;controls.dialogueCompletionLine=5;
        const auto publication=mailbox.Publish(controls);const auto plan=session.PrepareLoad();
        Check(plan && game.Snapshot().tickIndex==before.tickIndex && game.Snapshot().playerX==before.playerX,
            "load preparation leaves the current playable snapshot intact");
        if(plan){
            const auto original=controls;const auto wrong=sim::WorldZoneToken{plan->destination.generation-1,plan->destination.zone};
            Check(!session.CommitLoad(*plan,wrong,terrain,controls,publication) &&
                !session.CommitLoad(*plan,plan->destination,nullptr,controls,publication),"wrong readiness and failed resources cannot commit");
            Check(game.Snapshot().tickIndex==before.tickIndex && game.Snapshot().playerVitals.vitality==before.playerVitals.vitality &&
                controls.moveForward==original.moveForward && controls.yawRadians==original.yawRadians &&
                controls.commands.attack==original.commands.attack,"failed preparation preserves old simulation, camera and coherent controls");
            sim::InputSnapshot waiting;waiting.paused=true;
            for(int tick=0;tick<5;++tick)game.StepFixed(waiting,1.0f/60.0f,publication);
            const auto newestTick=game.Snapshot().tickIndex;
            Check(session.CommitLoad(*plan,plan->destination,terrain,controls,publication),"real file read transaction adopts after actual terrain/simulation preparation");
            const auto loaded=game.Snapshot();
            Check(loaded.tickIndex>newestTick,"load derives fresh identities from current live state after resource delay");
            Check(loaded.playerVitals.vitality==1 && loaded.worldRoute.current==sim::WorldZoneId::Dungeon && loaded.paused &&
                controls.moveForward==0 && !controls.runHeld && controls.yawRadians==loaded.playerYawRadians && controls.pitchRadians==loaded.playerPitchRadians &&
                controls.commands.attack==90 && controls.commands.interact==91 && controls.dialogueCompletionGeneration==0,
                "successful swap atomically restores manual gameplay and rebases camera/input while preserving command floors");
            Check(!session.CommitLoad(*plan,plan->destination,terrain,controls,publication),"same load cannot apply twice");
            game.AdvanceFrame(original,.02,publication);
            Check(game.Snapshot().paused && game.Snapshot().playerX==loaded.playerX,"old held publication cannot move the loaded player");
            auto paused=controls;paused.paused=true;auto p=mailbox.Publish(paused);game.AdvanceFrame(paused,.02,p);
            p=mailbox.Publish(controls);game.AdvanceFrame(controls,.02,p);
            Check(!game.Snapshot().paused && game.Snapshot().playerCombat.action==horde::gameplay::PlayerCombatAction::Idle,
                "new coherent neutral camera publication resumes with no attack debt");
            controls.moveForward=1;p=mailbox.Publish(controls);game.AdvanceFrame(controls,.05,p);
            Check(game.Snapshot().playerMovementSpeedMetresPerSecond<=1.401f && game.Snapshot().playerMovementSpeedMetresPerSecond>0,
                "load preserves the caller's movement configuration and accepts fresh movement");
        }
        Check(session.Load(cp::SaveKind::Autosave) && Finish(session),"auto load can be prepared separately");
        const auto stale=session.PrepareLoad();Check(stale.has_value(),"auto preparation exists");
        const auto old=game.Snapshot();session.SelectSlot(1);
        Check(stale && !session.CommitLoad(*stale,stale->destination,terrain,controls,mailbox.PublishedSequence()) &&
            game.Snapshot().tickIndex==old.tickIndex,"slot switch cancels only owned load and rejects stale ready resources");
        // Never overwrite corrupt/unsupported files to make saving succeed.
        const auto path=directory.path/cp::SaveBasename(*cp::SaveTarget(1,cp::SaveKind::Manual));
        {std::ofstream f(path,std::ios::binary|std::ios::trunc);f<<"unsupported owner-like fixture";}
        const auto corrupt=Raw(path);Check(Seed(game,Record(2),terrain),"corrupt-slot save attempt uses actual safe gameplay");
        Check(session.Save(cp::SaveKind::Manual,cp::CheckpointId::LowerClaimed) && Finish(session) && session.State().phase==cp::SessionPhase::Failed &&
            Raw(path)==corrupt,"invalid existing record remains byte-identical and save reports failure");
        Check(session.Load(cp::SaveKind::Autosave) && Finish(session),"cancellation ownership setup reads valid auto");
        const auto abandoned=session.PrepareLoad();const auto independent=game.BeginCampaignRestore(Record(2));
        session.Cancel();
        Check(abandoned && independent && game.CommitCampaignRestore(*independent,
            {independent->worldGeneration,sim::WorldZoneId::Dungeon},terrain,controls,mailbox.PublishedSequence()),
            "cancelling an older load cannot cancel another owner's newer simulation preparation");
        session.Close();
    }
    { // Fresh controller/process-like reconstruction reads persisted auto state.
        cp::Session fresh(game,directory.path);fresh.SelectSlot(2);
        Check(fresh.Load(cp::SaveKind::Autosave) && Finish(fresh),"new controller finds persistent latest auto");
        const auto p=fresh.PrepareLoad();controls.moveForward=0;
        Check(p && fresh.CommitLoad(*p,p->destination,terrain,controls,mailbox.PublishedSequence()) &&
            game.Snapshot().playerVitals.vitality==3 && game.Snapshot().rescue.exteriorSide,
            "new controller reconstructs saved exterior via real disk, terrain, simulation and controls");
    }
    unsigned files=0;for(const auto& f:fs::directory_iterator(directory.path))if(f.path().extension()==".hrd")++files;
    Check(files==6,"three slots retain exactly manual and latest autosave, no history");
    std::cout<<checks<<" campaign session checks, "<<failures<<" failures\n";return failures?1:0;
}
