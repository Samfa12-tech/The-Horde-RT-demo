#include "gameplay/campaign/CampaignSession.h"
#include "gameplay/campaign/CampaignCheckpointCodec.h"
#include "gameplay/campaign/CampaignPlaySession.h"
#include "gameplay/dialogue/BellwetherRoadStone.h"
#include "gameplay/simulation/InputMailbox.h"
#include "gameplay/DevelopmentCheckpointSimulation.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/BellwetherShell.h"
#include "scene/assets/AssetManifest.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <utility>

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
cp::Checkpoint ForestRecord(const sim::TriangleSupportSurface& terrain) {
    auto c=Record(3,true);c.contentRevision=2;c.id=cp::CheckpointId::ForestEndpoint;
    c.zone=sim::WorldZoneId::Lookout;const auto p=sim::kWorldRoutePoints.back();
    const auto ground=terrain.Query(p.x,p.z);
    Check(ground.found && ground.walkable,"forest session fixture uses admitted lookout terrain");
    c.position={p.x,ground.worldY,p.z};c.yaw=2.51f;c.pitch=-.15f;
    c.dialogue.waitingForRaise=false;c.dialogue.proofAccepted=true;
    c.dialogue.forestNightConsumed=c.dialogue.forestWaystoneConsumed=true;
    c.dialogue.forestClueConsumed=c.dialogue.forestVillageConsumed=true;return c;
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

void CheckTavernRoundTrip(const std::shared_ptr<const sim::TriangleSupportSurface>& base,
                          horde::scene::assets::StaticMeshAsset world) {
    using namespace horde::gameplay::dialogue;
    for(const auto* name:{"MedievalWall02","TombEarthenware","TombCharredWick"}) {
        if(std::none_of(world.materials.begin(),world.materials.end(),[&](const auto& material){return material.name==name;})) {
            horde::scene::assets::StaticMaterial material;material.name=name;world.materials.push_back(material);
        }
    }
    auto terrain=base;
    std::string diagnostic;
    const auto assets=fs::path(HORDE_RT_SOURCE_DIR)/"assets";
    const bool admitted=horde::scene::AppendBellwetherShell(assets,world,terrain,diagnostic);
    Check(admitted,"tavern restore uses the real imported chair and floor");
    if(!admitted)return;
    auto record=ForestRecord(*terrain);
    record.id=cp::CheckpointId::BellwetherTavern;record.contentRevision=cp::kBellwetherContentRevision;
    record.zone=cp::Destination(record.id);
    record.position=cp::kBellwetherCheckpointPosition;record.yaw=-1.40f;record.pitch=-.22f;
    record.vitality=2;record.swordStowed=true;
    Directory directory;
    {
        sim::GameSimulation game(sim::ProductionGameSimulationConfig());
        cp::Session session(game,directory.path);
        const bool saved=Seed(game,record,terrain) && session.SelectSlot(0) &&
            session.Save(cp::SaveKind::Autosave,record.id) && Finish(session) && session.State().phase==cp::SessionPhase::Idle;
        Check(saved,"the ordinary autosave worker writes the admitted tavern checkpoint");
        if(!saved)return;
    }
    const auto stored=Read(directory.path,0,cp::SaveKind::Autosave);
    Check(stored.checkpoint.id==record.id && stored.checkpoint.schema==record.schema &&
          stored.checkpoint.contentRevision==record.contentRevision,
          "disk records the new boundary and keeps its original save schema");
    sim::GameSimulation fresh(sim::ProductionGameSimulationConfig());
    cp::Session session(fresh,directory.path);
    sim::InputSnapshot input;
    if(!session.SelectSlot(0) || !session.Load(cp::SaveKind::Autosave) || !Finish(session)) {
        Check(false,"fresh session reads the tavern file");return;
    }
    const auto plan=session.PrepareLoad();
    Check(plan && !session.CommitLoad(*plan,plan->destination,base,input,0) && !fresh.LatestCampaignBoundary(),
          "bare terrain cannot admit a tavern reload or mutate the old game");
    using Terrain=sim::TriangleSupportSurface;
    const float floor=record.position.y;
    const Terrain::Point nw{80.f,floor,106.f},sw{80.f,floor,111.f},ne{88.f,floor,106.f},se{88.f,floor,111.f};
    const std::array floorOnly{Terrain::Triangle{{nw,sw,ne},sim::SupportSurface::Stone},
                               Terrain::Triangle{{sw,se,ne},sim::SupportSurface::Stone}};
    const auto withoutChair=Terrain::WithAdditionalGroundGeometry(*base,floorOnly,diagnostic);
    Check(plan && withoutChair && std::abs(withoutChair->Query(record.position.x,record.position.z).worldY-floor)<.001f &&
          !session.CommitLoad(*plan,plan->destination,withoutChair,input,0) && !fresh.LatestCampaignBoundary(),
          "a valid room floor still cannot restore Kit without his chair");
    if(!plan || !session.CommitLoad(*plan,plan->destination,terrain,input,0)) {
        Check(false,"loaded tavern commits through ordinary readiness and input rebasing");return;
    }
    const auto seated=[&] {
        const auto& kit=fresh.Snapshot().chapterCompanion;
        return kit.bellwetherArrival==BellwetherArrivalStage::Seated && kit.grounded &&
            kit.motion.mode==CompanionMotionMode::Seated && kit.motion.sitPlaybackSeconds==kTavernSitDurationSeconds;
    };
    Check(seated() && fresh.Snapshot().playerX==record.position.x && fresh.Snapshot().playerZ==record.position.z &&
          BellwetherRoadStoneWorldFromModel(fresh.Snapshot().chapterCompanion)==kBellwetherRoadStoneTrack.back() &&
          fresh.Snapshot().chapterDialogue.forestVillageConsumed,
          "fresh game restores the tavern position, settled Kit and road stone, and completed lookout");
    for(unsigned tick=0;tick<120;++tick)fresh.StepFixed(input,1.f/60.f,tick+1);
    Check(seated() && fresh.Snapshot().worldRoute.current==record.zone &&
          fresh.CaptureAuthoredCampaignBoundary(record.id).has_value() &&
          fresh.Snapshot().chapterDialogue.line==Line::TavernLimit &&
          fresh.Snapshot().dialogueBedGain==1.f &&
          fresh.Snapshot().chapterDialogue.hearthLinesResolved==1 && stored.encoded==
          Read(directory.path,0,cp::SaveKind::Autosave).encoded,
          "legacy tavern load keeps Kit seated and starts with his first seated line without rewriting the source save");

    cp::PlaySession play(fresh,directory.path);
    Check(play.BeginNewGame(1),"test owner activates a separate autosave slot");
    std::uint64_t publication=120;
    const auto awaitSave=[](cp::PlaySession& session,std::uint64_t operation) {
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
        do {
            session.Poll();
            const auto state=session.State();
            if(state.operationRevision>operation && state.outcome==cp::PlayOutcome::Succeeded)return true;
            std::this_thread::yield();
        } while(std::chrono::steady_clock::now()<deadline);
        return false;
    };
    ++input.commands.dialogueSkip;
    fresh.StepFixed(input,1.f/60.f,++publication);
    Check(play.ObserveSafeBoundary() && awaitSave(play,0),"advancing at the hearth writes the same tavern boundary");
    const auto partial=Read(directory.path,1,cp::SaveKind::Autosave).checkpoint;
    Check(partial.schema==cp::kHearthSpeechSchema && partial.dialogue.hearthLinesResolved==2 &&
          partial.dialogue.activeLine==Line::None,"dated autosave retains only the resolved hearth beats");
    for(unsigned tick=0;tick<50;++tick)fresh.StepFixed(input,1.f/60.f,++publication);
    Check(fresh.Snapshot().chapterDialogue.line==Line::TavernWork,"Kit's offer follows the seated pause");
    const auto previousSave=play.State().operationRevision;
    ++input.commands.dialogueSkip;
    fresh.StepFixed(input,1.f/60.f,++publication);
    Check(play.ObserveSafeBoundary() && awaitSave(play,previousSave),"another resolved line refreshes the existing autosave");
    const auto complete=Read(directory.path,1,cp::SaveKind::Autosave).checkpoint;
    sim::GameSimulation resumed(sim::ProductionGameSimulationConfig());
    Check(Seed(resumed,partial,terrain),"partial dialogue can restore from the saved hearth boundary");
    sim::InputSnapshot resumedInput;resumedInput.yawRadians=record.yaw;resumedInput.pitchRadians=record.pitch;
    resumed.StepFixed(resumedInput,1.f/60.f,1);
    Check(resumed.Snapshot().chapterDialogue.line==Line::TavernWork,"partial restore resumes the first unresolved caption");
    Check(complete.dialogue.hearthLinesResolved==3 && Seed(resumed,complete,terrain),"completed dialogue restores from disk");
    resumed.StepFixed(resumedInput,1.f/60.f,2);
    Check(resumed.Snapshot().chapterDialogue.line==Line::None,"completed hearth captions remain quiet on reload");
    ++resumedInput.commands.interact;
    resumed.StepFixed(resumedInput,1.f/60.f,3);
    Check(!resumed.Snapshot().tavernConversationAvailable && resumed.Snapshot().chapterDialogue.line==Line::None,
          "the real tavern remains quiet on Interact until a keeper is admitted");

    // Model the same-boundary update published by the simulation after the hand-over.
    auto afterTalk=complete;
    afterTalk.schema=cp::kTavernConversationSchema;
    afterTalk.dialogue.tavernConversationLinesResolved=kTavernConversation.size();
    const auto beforeTalkSave=play.State().operationRevision;
    const bool savedTalk=Seed(fresh,afterTalk,terrain) && play.ObserveSafeBoundary() && awaitSave(play,beforeTalkSave);
    Check(savedTalk,"the completed Talk is the baseline autosave for the rag favour");
    if(savedTalk) {
        auto delivered=afterTalk;
        delivered.schema=cp::kPrivyRagSchema;
        delivered.privyRagDelivered=true;
        const auto beforeDeliverySave=play.State().operationRevision;
        const bool savedDelivery=Seed(fresh,delivered,terrain) && play.ObserveSafeBoundary() &&
            awaitSave(play,beforeDeliverySave);
        Check(savedDelivery,"rag delivery alone refreshes the existing tavern autosave");
        if(savedDelivery) {
            const auto saved=Read(directory.path,1,cp::SaveKind::Autosave);
            Check(saved.checkpoint.schema==cp::kPrivyRagSchema && saved.checkpoint.privyRagDelivered &&
                  saved.checkpoint.id==afterTalk.id && saved.checkpoint.treasurePurse==afterTalk.treasurePurse &&
                  saved.checkpoint.flask==afterTalk.flask,
                  "the exact new disk record retains the tavern boundary and resources");
            const auto operation=play.State().operationRevision;
            for(unsigned tick=0;tick<16;++tick){play.ObserveSafeBoundary();play.Poll();}
            Check(play.State().operationRevision==operation,
                  "observing a delivered rag again does not queue another autosave");
        }
    }

    auto staffedConfig=sim::ProductionGameSimulationConfig();
    staffedConfig.bellwetherTavernKeeperAdmitted=true;
    for (const auto& [name,expected] : std::array{
            std::pair{"bellwether-keeper-reply",Line::TavernHordeArmy},
            std::pair{"bellwether-staffed-room",Line::None},
            std::pair{"bellwether-kit-lantern-glance",Line::TavernIntroduce}}) {
        sim::GameSimulation capture(staffedConfig);
        capture.SetDevelopmentRescueJourney(true, true);
        const auto generation = capture.Snapshot().worldRoute.generation;
        const bool ready = capture.PublishChapterTerrain({generation, sim::WorldZoneId::TombExterior}, terrain);
        const auto* view = horde::gameplay::FindDevelopmentCheckpoint(name);
        const bool staged = ready && view && horde::gameplay::StageDevelopmentCheckpointSimulation(capture, *view);
        Check(staged, "the staffed diagnostic uses the authored terrain, restore and fixed-step Talk interaction");
        if (staged) {
            const auto& frame = capture.Snapshot();
            Check(frame.tavernKeeper && frame.chapterDialogue.line == expected &&
                  std::hypot(frame.playerX - view->cameraX, frame.playerZ - view->cameraZ) < .01f,
                "each view holds its own speaker or the quiet room at the authored camera mark");
        }
    }
    sim::GameSimulation staffed(staffedConfig);
    Check(Seed(staffed,complete,terrain),"staffed tavern uses the same real room and campaign transaction");
    cp::PlaySession staffedPlay(staffed,directory.path);
    Check(staffedPlay.BeginNewGame(2),"the exchange test uses a separate autosave slot");
    sim::InputSnapshot talk;talk.yawRadians=record.yaw;talk.pitchRadians=record.pitch;
    staffed.StepFixed(talk);
    const auto& keeper=staffed.Snapshot().tavernKeeper;
    Check(keeper && keeper->stage==TavernKeeperStage::AtHearth &&
          keeper->foot[0]==horde::gameplay::bellwether::kKeeperHearth[0] &&
          staffed.Snapshot().tavernConversationAvailable && staffed.Snapshot().chapterDialogue.line==Line::None,
          "restored hearth offers Talk and waits for the player");
    ++talk.commands.interact;staffed.StepFixed(talk);
    Check(!staffed.Snapshot().tavernConversationAvailable && staffed.Snapshot().chapterDialogue.line==Line::TavernIntroduce,
          "normal fixed-step Interact starts Kit's introduction");
    ++talk.commands.dialogueSkip;staffed.StepFixed(talk);
    Check(staffed.Snapshot().chapterDialogue.line==Line::TavernHordeArmy &&
          staffed.Snapshot().chapterDialogue.source.x==horde::gameplay::bellwether::kKeeperHearth[0],
          "the keeper speaks from her admitted position");
    const auto beforeReply=staffed.LatestCampaignBoundary();
    Check(beforeReply && beforeReply->schema==cp::kTavernConversationSchema &&
          beforeReply->dialogue.tavernConversationLinesResolved==1 && beforeReply->dialogue.activeLine==Line::None,
          "the ordinary autosave boundary includes only the resolved introduction");
    Check(staffedPlay.ObserveSafeBoundary() && awaitSave(staffedPlay,0),"the conversation boundary reaches the save worker");
    const auto savedReply=Read(directory.path,2,cp::SaveKind::Autosave).checkpoint;
    Check(staffedPlay.TryEndActiveGame(),"the original session leaves the partial save available to reopen");
    sim::GameSimulation reopened(staffedConfig);
    cp::PlaySession reopenedPlay(reopened,directory.path);
    sim::InputSnapshot again;
    const auto loadReply=[&] {
        if(!reopenedPlay.Load(2,cp::SaveKind::Autosave))return false;
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
        do {
            reopenedPlay.Poll();
            if(const auto preparation=reopenedPlay.PrepareLoad())
                return reopenedPlay.CommitLoad(*preparation,preparation->destination,terrain,again,0);
            if(reopenedPlay.State().outcome==cp::PlayOutcome::Failed)return false;
            std::this_thread::yield();
        } while(std::chrono::steady_clock::now()<deadline);
        return false;
    };
    if(savedReply.dialogue.tavernConversationLinesResolved==1 && loadReply()) {
        reopened.StepFixed(again);
        Check(reopened.Snapshot().tavernConversationAvailable && reopened.Snapshot().chapterDialogue.line==Line::None,
              "reloading mid-exchange does not resume without the player's action");
        ++again.commands.interact;reopened.StepFixed(again);
        Check(reopened.Snapshot().chapterDialogue.line==Line::TavernHordeArmy,"Talk resumes the unfinished keeper reply");
        ++again.commands.dialogueSkip;reopened.StepFixed(again);
        Check(reopened.Snapshot().chapterDialogue.line==Line::TavernHoardAnswer,"the last line returns to Kit");
        const auto loadedRevision=reopenedPlay.State().operationRevision;
        Check(reopenedPlay.ObserveSafeBoundary(),"the restored slot observes a newly resolved keeper reply");
        ++again.commands.dialogueSkip;reopened.StepFixed(again);
        const auto done=reopened.LatestCampaignBoundary();
        const bool written=done && done->dialogue.tavernConversationLinesResolved==3 &&
            reopenedPlay.ObserveSafeBoundary() && !reopenedPlay.TryEndActiveGame() && awaitSave(reopenedPlay,loadedRevision);
        Check(written,"the same tavern autosave coalesces the remaining lines and finishes before quit");
        if(written) {
            const auto savedDone=Read(directory.path,2,cp::SaveKind::Autosave).checkpoint;
            sim::InputSnapshot idle;idle.yawRadians=record.yaw;idle.pitchRadians=record.pitch;
            const bool restored=Seed(staffed,savedDone,terrain);
            if(restored)staffed.StepFixed(idle);
            Check(restored && staffed.Snapshot().chapterDialogue.tavernConversationLinesResolved==3 &&
                  staffed.Snapshot().chapterDialogue.line==Line::None && !staffed.Snapshot().tavernConversationAvailable &&
                  staffed.Snapshot().kitTreasureTalkAvailable,
                  "the saved camera faces Kit and his optional question stays quiet until asked");
            const auto& keeper=horde::gameplay::bellwether::kKeeperHearth;
            idle.yawRadians=std::atan2(keeper[0]-staffed.Snapshot().playerX,
                                      staffed.Snapshot().playerZ-keeper[2]);
            staffed.StepFixed(idle);
            Check(staffed.Snapshot().tavernDirectionsAvailable && !staffed.Snapshot().kitTreasureTalkAvailable,
                  "looking back to the keeper offers the Directions action after reload");
            idle.commands.interact=staffed.Snapshot().lastConsumedInteractSequence+1;
            staffed.StepFixed(idle);
            Check(staffed.Snapshot().chapterDialogue.line==Line::TavernDirections &&
                  staffed.Snapshot().chapterDialogue.tavernConversationLinesResolved==3,
                  "the restored Directions action opens the lead without changing the saved exchange");
        }
    } else Check(false,"the partial exchange is restorable");
    {
        using namespace horde::gameplay::bellwether;
        auto archive=complete;
        archive.schema=cp::kWaysideArchiveSchema;
        archive.dialogue.hearthLinesResolved=3;
        archive.waysideArchive=cp::WaysideArchiveProgress::Open;
        cp::Checkpoint decoded;
        std::vector<std::byte> bytes;
        cp::codec::Error error{};
        sim::GameSimulation restoredArchive(staffedConfig);
        Check(cp::codec::Encode(archive,bytes,error) && cp::codec::Decode(bytes,decoded,error) &&
              decoded.waysideArchive==cp::WaysideArchiveProgress::Open && Seed(restoredArchive,decoded,terrain) &&
              restoredArchive.Snapshot().waysideArchive.stage==ArchiveStage::Open &&
              restoredArchive.Snapshot().waysideArchive.pose.worldFromModel==ArchivePose::AtOpening(1.f).worldFromModel,
              "a saved open archive restores the matching hinge and contact pose without replay");
        archive.schema=cp::kShrineLanternSchema;
        archive.waysideLanternRevealStarted=true;
        Check(Seed(restoredArchive,archive,terrain) && restoredArchive.Snapshot().waysideArchive.lanternRevealStarted &&
              restoredArchive.Snapshot().chapterDialogue.line==Line::None,
              "a started shrine reveal restores as consumed without restarting active speech");
        archive.schema=cp::kWaysideRegisterSchema;
        archive.waysideRegisterRecovered=true;
        Check(cp::codec::Encode(archive,bytes,error) && cp::codec::Decode(bytes,decoded,error) &&
              Seed(restoredArchive,decoded,terrain) && restoredArchive.Snapshot().waysideArchive.registerRecovered &&
              restoredArchive.Snapshot().chapterDialogue.line==Line::None,
              "the recovered register stays collected on reload without replaying its pickup caption");
        archive.schema=cp::kRegisterInterpretationSchema;
        archive.waysideRegisterInterpreted=true;
        Check(cp::codec::Encode(archive,bytes,error) && cp::codec::Decode(bytes,decoded,error) &&
              Seed(restoredArchive,decoded,terrain) && restoredArchive.Snapshot().waysideArchive.registerInterpreted &&
              restoredArchive.Snapshot().chapterDialogue.line==Line::None,
              "the expedition directions survive reload without replaying the church reading");
        Check(restoredArchive.Snapshot().chapterDialogue.tavernLanternLinesStarted==0,
              "an older interpreted register restores with Kit still awaiting the lantern");
        archive.schema=cp::kTavernLanternExchangeSchema;
        archive.dialogue.tavernLanternLinesStarted=1;
        Check(cp::codec::Encode(archive,bytes,error) && cp::codec::Decode(bytes,decoded,error) &&
              Seed(restoredArchive,decoded,terrain) &&
              restoredArchive.Snapshot().chapterDialogue.tavernLanternLinesStarted==1 &&
              restoredArchive.Snapshot().chapterDialogue.line==Line::None,
              "Kit keeps the first witnessed line on load without replaying speech");
        archive.schema=cp::kAirWardSchema;
        archive.waysideArchive=cp::WaysideArchiveProgress::Restored;
        Check(Seed(restoredArchive,archive,terrain) &&
              restoredArchive.Snapshot().waysideArchive.stage==ArchiveStage::Restored &&
              restoredArchive.Snapshot().waysideArchive.pose.worldFromModel==ArchivePose::AtOpening(0.f).worldFromModel &&
              !restoredArchive.Snapshot().waysideArchive.airWardLearned,
              "restored binding loads closed and still requires the keeper's lesson");
        archive.airWardLearned=true;
        Check(Seed(restoredArchive,archive,terrain) && restoredArchive.Snapshot().waysideArchive.airWardLearned &&
              restoredArchive.Snapshot().waysideArchive.registerRecovered &&
              restoredArchive.Snapshot().chapterDialogue.line==Line::None,
              "the ward lesson survives reload without replaying speech or putting the register back");
        archive.schema=cp::kTavernConversationSchema;
        Check(!cp::Validate(archive),"an older schema cannot silently discard earned archive progress");
    }
    {
        using horde::gameplay::bellwether::MillPreparation;
        Directory millDirectory;
        sim::GameSimulation millGame(staffedConfig);
        cp::PlaySession millPlay(millGame,millDirectory.path);
        auto mill=complete;
        mill.schema=cp::kTavernConversationSchema;
        mill.dialogue.tavernConversationLinesResolved=kTavernConversation.size();
        Check(millPlay.BeginNewGame(0) && Seed(millGame,mill,terrain) &&
              millPlay.ObserveSafeBoundary() && awaitSave(millPlay,0),
              "the mill autosave check starts with an ordinary completed tavern record");
        mill.schema=cp::kMillPreparationSchema;
        for(auto stage:{MillPreparation::BearingNeeded,MillPreparation::BearingRecovered,MillPreparation::FitChecked,
                        MillPreparation::BearingSeated,MillPreparation::GearingVerified,
                        MillPreparation::RigTrialReady,MillPreparation::RigTested}) {
            const auto previous=millPlay.State().operationRevision;
            mill.millPreparation=stage;
            Check(Seed(millGame,mill,terrain) && millPlay.ObserveSafeBoundary() && awaitSave(millPlay,previous),
                  "each mill step refreshes the same earned autosave");
            const auto saved=Read(millDirectory.path,0,cp::SaveKind::Autosave).checkpoint;
            Check(saved.millPreparation==stage && saved.position.x==complete.position.x &&
                  saved.position.z==complete.position.z && saved.flask==complete.flask &&
                  saved.treasurePurse==complete.treasurePurse,
                  "the actual save file contains the mill step with unchanged return point and resources");
        }
    }
    {
        Directory salvagerDirectory;
        sim::GameSimulation salvagerGame(staffedConfig);
        cp::PlaySession salvagerPlay(salvagerGame,salvagerDirectory.path);
        auto salvager=complete;
        salvager.schema=cp::kTavernConversationSchema;
        salvager.dialogue.tavernConversationLinesResolved=kTavernConversation.size();
        salvager.salvagerShorePracticeCompleted=false;
        Check(salvagerPlay.BeginNewGame(0) && Seed(salvagerGame,salvager,terrain) &&
              salvagerPlay.ObserveSafeBoundary() && awaitSave(salvagerPlay,0),
              "the salvager autosave check starts with a completed tavern record");
        salvager.schema=cp::kSalvagerShorePracticeSchema;
        salvager.salvagerShorePracticeCompleted=true;
        const auto previous=salvagerPlay.State().operationRevision;
        Check(Seed(salvagerGame,salvager,terrain) && salvagerPlay.ObserveSafeBoundary() &&
              awaitSave(salvagerPlay,previous),
              "completing shore practice refreshes the ordinary autosave");
        auto saved=Read(salvagerDirectory.path,0,cp::SaveKind::Autosave).checkpoint;
        Check(saved.schema==cp::kSalvagerShorePracticeSchema && saved.salvagerShorePracticeCompleted,
              "the shore completion byte is retained in the autosave");

        auto later=salvager;
        later.id=cp::CheckpointId::SafeExit;
        later.zone=cp::Destination(later.id);
        later.position=horde::gameplay::traversal::kExteriorLanding;
        const auto laterRevision=salvagerPlay.State().operationRevision;
        Check(Seed(salvagerGame,later,terrain) && salvagerPlay.ObserveSafeBoundary() &&
              awaitSave(salvagerPlay,laterRevision),
              "a later safe-region boundary still autosaves the shore completion");
        saved=Read(salvagerDirectory.path,0,cp::SaveKind::Autosave).checkpoint;
        Check(saved.id==cp::CheckpointId::SafeExit && saved.salvagerShorePracticeCompleted,
              "the later safe-region autosave preserves shore completion");
    }
    auto returned=complete;returned.id=cp::CheckpointId::SafeExit;returned.zone=cp::Destination(returned.id);
    returned.position=horde::gameplay::traversal::kExteriorLanding;
    Check(Seed(fresh,returned,terrain) && seated() && fresh.Snapshot().chapterDialogue.hearthLinesResolved==3,
          "a later safe-side save keeps completed hearth progress and Kit settled back at the tavern");
    auto lookout=ForestRecord(*terrain);lookout.contentRevision=cp::kBellwetherContentRevision;
    Check(Seed(fresh,lookout,terrain) && seated(),"returning to the old lookout anchor keeps the earned tavern state");
    const auto atLookout=fresh.CaptureCampaignCheckpoint(cp::CheckpointId::ForestEndpoint);
    Check(atLookout && atLookout->contentRevision==cp::kBellwetherContentRevision,
          "the lookout safe boundary remains available after Kit moves to town");
    Check(Seed(fresh,ForestRecord(*terrain),terrain) &&
          fresh.Snapshot().chapterCompanion.bellwetherArrival==BellwetherArrivalStage::AtLookout,
          "loading a genuine old lookout record still starts before the town journey");
}
}
int main(){
    const auto assets=fs::path(HORDE_RT_SOURCE_DIR)/"assets/models/world/runtime/chapter-terrain-v1";
    horde::scene::assets::AssetManifest manifest;horde::scene::assets::StaticMeshAsset mesh;std::string error;
    if(!horde::scene::assets::AssetManifest::Load(assets/"asset.manifest.json",manifest,error) ||
       !horde::scene::assets::StaticMeshAsset::Load(assets/"chapter-terrain-lod0.runtime.glb",manifest,mesh,error)){
        std::cerr<<error;return 1;}
    const auto terrain=horde::scene::PrepareChapterTerrainSurface(mesh,error);if(!terrain){std::cerr<<error;return 1;}
    CheckTavernRoundTrip(terrain,mesh);
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
        controls.crouchHeld=true;controls.commands.crouchToggle=93;controls.commands.clearCrouchIntent=94;
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
                controls.moveForward==0 && !controls.runHeld && !controls.crouchHeld && !loaded.crouchActive && controls.yawRadians==loaded.playerYawRadians && controls.pitchRadians==loaded.playerPitchRadians &&
                controls.commands.attack==90 && controls.commands.interact==91 && controls.dialogueCompletionGeneration==0,
                "successful swap atomically restores manual gameplay and rebases camera/input while preserving command floors");
            Check(!session.CommitLoad(*plan,plan->destination,terrain,controls,publication),"same load cannot apply twice");
            game.AdvanceFrame(original,.02,publication);
            Check(game.Snapshot().paused && game.Snapshot().playerX==loaded.playerX,"old held publication cannot move the loaded player");
            auto heldStance=controls;heldStance.crouchHeld=true;auto p=mailbox.Publish(heldStance);game.AdvanceFrame(heldStance,.02,p);
            Check(game.Snapshot().paused && !game.Snapshot().crouchActive,"fresh held stance alone cannot pass the load release barrier");
            auto paused=controls;paused.paused=true;p=mailbox.Publish(paused);game.AdvanceFrame(paused,.02,p);
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
    {
        const auto endpoint=ForestRecord(*terrain);
        std::vector<std::pair<fs::path,std::vector<std::byte>>> otherRecords;
        for(unsigned slot=0;slot<3;++slot)for(auto kind:{cp::SaveKind::Manual,cp::SaveKind::Autosave}) {
            if(slot==2 && kind==cp::SaveKind::Autosave)continue;
            const auto path=directory.path/cp::SaveBasename(*cp::SaveTarget(slot,kind));
            otherRecords.emplace_back(path,Raw(path));
        }
        {
            cp::Session endpointSession(game,directory.path);
            Check(endpointSession.SelectSlot(2) && Seed(game,endpoint,terrain) &&
                endpointSession.Save(cp::SaveKind::Autosave,cp::CheckpointId::ForestEndpoint) && Finish(endpointSession) &&
                endpointSession.State().phase==cp::SessionPhase::Idle,
                "forest checkpoint captures actual simulation and replaces only its selected latest auto");
        }
        const auto stored=Read(directory.path,2,cp::SaveKind::Autosave);
        Check(stored.checkpoint.id==cp::CheckpointId::ForestEndpoint && stored.checkpoint.contentRevision==2,
            "real disk record identifies forest endpoint content independently of old boundary records");
        cp::Session fresh(game,directory.path);
        Check(fresh.SelectSlot(2) && fresh.Load(cp::SaveKind::Autosave) && Finish(fresh),
            "fresh session reads persisted endpoint through the worker");
        const auto plan=fresh.PrepareLoad();const auto before=game.Snapshot();
        controls.moveForward=1;controls.runHeld=true;controls.hasAuthoritativePlayerPose=true;
        controls.authoritativePlayerX=-33.7f;controls.authoritativePlayerZ=-12.8f;
        controls.commands.attack=500;controls.commands.toggleHeldLightPose=501;
        controls.dialogueCompletionLine=static_cast<std::uint32_t>(horde::gameplay::dialogue::Line::ForestVillage);
        controls.dialogueCompletionGeneration=before.chapterDialogue.generation;
        const auto old=controls;const auto published=mailbox.Publish(controls);
        Check(plan && plan->destination.zone==sim::WorldZoneId::Lookout,
            "forest preparation requests real Lookout resources rather than summit residency");
        if(plan) {
            Check(!fresh.CommitLoad(*plan,{plan->destination.generation,sim::WorldZoneId::TombExterior},terrain,controls,published) &&
                game.Snapshot().tickIndex==before.tickIndex && controls.moveForward==old.moveForward,
                "summit readiness cannot substitute for the endpoint destination or mutate controls");
            Check(fresh.CommitLoad(*plan,plan->destination,terrain,controls,published),
                "matching Lookout preparation commits persisted endpoint atomically");
            const auto loaded=game.Snapshot();
            Check(loaded.worldRoute.current==sim::WorldZoneId::Lookout && loaded.playerSupportWorldY==endpoint.position.y &&
                loaded.chapterCompanion.grounded && loaded.chapterCompanion.waypoint==sim::kWorldRoutePoints.size() &&
                std::abs(loaded.chapterCompanion.x-(endpoint.position.x+1.f))<.001f &&
                loaded.chapterDialogue.line==horde::gameplay::dialogue::Line::None && loaded.chapterDialogue.forestVillageConsumed &&
                controls.moveForward==0 && !controls.runHeld && !controls.hasAuthoritativePlayerPose &&
                controls.dialogueCompletionGeneration==0,
                "persisted endpoint restores grounded player/Kit, completed speech and neutral owner input");
            game.StepFixed(old,1.f/60.f,published);
            Check(game.Snapshot().paused && game.Snapshot().playerX==endpoint.position.x,
                "old coherent publication cannot move or teleport the loaded forest player");
            const auto neutral=mailbox.Publish(controls);game.StepFixed(controls,1.f/60.f,neutral);
            Check(!game.Snapshot().paused && game.Snapshot().playerCombat.action==horde::gameplay::PlayerCombatAction::Idle &&
                game.Snapshot().chapterDialogue.line==horde::gameplay::dialogue::Line::None,
                "fresh neutral endpoint publication resumes without stale combat or speech");
        }
        Check(Read(directory.path,2,cp::SaveKind::Autosave).encoded==stored.encoded,
            "endpoint read/prepare/commit does not rewrite its source record");
        for(const auto& [path,bytes]:otherRecords)Check(Raw(path)==bytes,
            "endpoint persistence and restore leave every other manual/auto record byte-identical");
    }
    unsigned files=0;for(const auto& f:fs::directory_iterator(directory.path))if(f.path().extension()==".hrd")++files;
    Check(files==6,"three slots retain exactly manual and latest autosave, no history");
    std::cout<<checks<<" campaign session checks, "<<failures<<" failures\n";return failures?1:0;
}
