#include "audio/DialogueDucking.h"
#include "audio/MusicPlaybackSession.h"
#include "gameplay/dialogue/ChapterDialogue.h"
#include "gameplay/simulation/FixedStepRunner.h"
#include "gameplay/simulation/GameSimulation.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/assets/AssetManifest.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace {
using namespace horde::audio;
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
namespace dlg = horde::gameplay::dialogue;
namespace cp = horde::gameplay::campaign;
namespace tr = horde::gameplay::traversal;
constexpr float kTick = 1.f/60.f;
int checks = 0, failures = 0;
void Check(bool value, const char* message) {
    ++checks;
    if (!value) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
bool Near(double a, double b, double tolerance=.00001) { return std::abs(a-b) <= tolerance; }
void Ticks(DialogueDucking& duck, bool speaking, unsigned count) {
    for (unsigned i=0; i<count; ++i) duck.Step(speaking,kTick);
}
void Envelope() {
    DialogueDucking duck;
    Check(duck.Gain()==1.f, "new envelope starts at unity");
    duck.Step(true,.05f);
    Check(Near(duck.Gain(),.9453125), "quarter attack uses the specified smoothstep envelope");
    duck.Step(true,.05f);
    Check(Near(duck.Gain(),.825), "half attack has the exact intermediate gain");
    duck.Step(true,.05f); duck.Step(true,.05f);
    Check(duck.Gain()==DialogueDucking::kDuckedGain, "0.20 seconds of speech reaches the bounded 0.65 floor");
    Ticks(duck,true,120);
    Check(duck.Gain()==DialogueDucking::kDuckedGain, "long and consecutive speech cannot deepen attenuation");
    for (unsigned i=0; i<7; ++i) duck.Step(false,.05f);
    Check(Near(duck.Gain(),.65), "0.35-second quiet hold prevents between-line pumping");
    for (unsigned i=0; i<9; ++i) duck.Step(false,.05f);
    Check(Near(duck.Gain(),.825), "half of the 0.90-second release is smooth and bounded");
    const float releasing=duck.Gain(); duck.Step(true,kTick);
    Check(duck.Gain()<releasing && duck.Gain()>.65f, "renewed speech reverses release from its current gain without a unity pulse");
    Ticks(duck,true,12);
    Check(Near(duck.Gain(),.65), "reversed attack settles back to the same floor");
    Ticks(duck,false,90);
    Check(duck.Gain()==1.f, "silence fully restores gain rather than retaining permanent attenuation");

    duck.Reset(); duck.Step(true,.05f); const float interrupted=duck.Gain();
    for (unsigned i=0; i<6; ++i) duck.Step(false,.05f);
    Check(duck.Gain()==interrupted, "interrupted attack freezes during hold instead of ducking silence more deeply");
    Ticks(duck,false,90);
    Check(duck.Gain()==1.f, "interrupted short line still releases completely");
    duck.Step(true,.05f); const float beforeInvalid=duck.Gain();
    for (const float invalid : {0.f,-1.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
        duck.Step(true,invalid); duck.Step(false,invalid);
        Check(duck.Gain()==beforeInvalid, "zero, negative and nonfinite time cannot advance or poison the envelope");
    }
    duck.Reset(); duck.Step(true,10.f);
    Check(Near(duck.Gain(),.9453125), "large contributions retain the fixed owner's existing 50ms step bound");
    duck.Reset(); Check(duck.Gain()==1.f, "explicit reset clears all attack/hold/release debt");

    std::vector<float> reference;
    for (const unsigned rate : {15u,30u,60u,120u}) {
        DialogueDucking grouped; FixedStepRunner runner; unsigned tick=0;
        std::vector<float> gains; bool bounded=true, continuous=true;
        for (unsigned frame=0; frame<rate*4; ++frame) runner.Advance(1.0/rate,false,[&](float dt) {
            ++tick; const float previous=grouped.Gain();
            grouped.Step(tick<=60 || (tick>63 && tick<=120),dt);
            gains.push_back(grouped.Gain());
            bounded &= std::isfinite(grouped.Gain()) && grouped.Gain()>=.65f && grouped.Gain()<=1.f;
            continuous &= std::abs(grouped.Gain()-previous)<.045f;
        });
        if (reference.empty()) reference=gains;
        Check(gains==reference && tick==240 && bounded && continuous,
              "15/30/60/120Hz render grouping preserves the same bounded continuous 60Hz envelope");
        Check(Near(gains[62],.65) && Near(gains[63],.65) && gains.back()==1.f,
              "tiny consecutive-line gaps never pump while final silence returns to unity");
    }
}
void SnapshotGuard() {
    SimulationSnapshot state; state.developmentRescueJourney=true; state.dialogueBedGain=.65f;
    Check(DialogueBedGain(state)==.65f, "current living chapter publishes its valid shared bed gain");
    state.paused=true;
    Check(DialogueBedGain(state)==.65f, "paused output retains its already frozen gain");
    state.developmentRescueJourney=false;
    Check(DialogueBedGain(state)==1.f, "legacy showcase cannot inherit chapter attenuation");
    state.developmentRescueJourney=true; state.playerAlive=false;
    Check(DialogueBedGain(state)==1.f, "dead snapshots reject retained attenuation");
    state.playerAlive=true;
    for (const float invalid : {.64f,1.01f,-1.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
        state.dialogueBedGain=invalid;
        Check(DialogueBedGain(state)==1.f, "malformed multipliers fall back to unity without changing stored settings");
    }
}
void SharedDialogueChain() {
    dlg::Director director; dlg::Context context; context.claimed=true;
    DialogueDucking duck;
    const auto step=[&]() { director.Step(kTick,context); duck.Step(director.State().line!=dlg::Line::None,kTick); };
    step(); const auto first=director.State();
    Check(first.line==dlg::Line::RescueFound, "real dialogue authority begins the bounded rescue chain");
    for (unsigned i=0; i<20; ++i) step();
    Check(Near(duck.Gain(),.65), "active shared dialogue reaches its floor");
    Check(director.Complete(first.line,first.generation), "current completion advances the real shared dialogue owner");
    step(); const auto second=director.State();
    Check(second.line==dlg::Line::RescueRope && Near(duck.Gain(),.65),
          "consecutive shared continuation has no audible unity pulse");
    Check(!director.Complete(first.line,first.generation), "stale earlier completion cannot finish the continuation");
    context.paused=true; const auto before=director.State(); const float pausedGain=duck.Gain();
    director.Step(.05f,context); duck.Step(true,0);
    Check(director.State().generation==before.generation && director.State().elapsed==before.elapsed &&
          duck.Gain()==pausedGain, "paused dialogue and its shared envelope both freeze");
    context.paused=false; director.Skip();
    for (unsigned i=0; i<90; ++i) step();
    Check(director.State().line==dlg::Line::None && duck.Gain()==1.f,
          "skip/cancellation releases beds after the ordered response ends");
    director.Request(dlg::Line::KeeperSense,{1,2,3},context); step();
    director.Suspend(); context.alive=false;
    for (unsigned i=0; i<90; ++i) step();
    Check(director.State().line==dlg::Line::None && duck.Gain()==1.f &&
          !director.Complete(second.line,second.generation), "suspended dialogue cannot leave a stale duck or accept old completion");
}
void SharedGrateFinalization() {
    for (unsigned finish=0; finish<3; ++finish) {
        dlg::Director director; DialogueDucking duck;
        dlg::Context context; context.player={0,kRouteFloorWorldY,-9.4f};
        context.listener={0,kShowcaseEyeWorldY,-9.4f};
        const auto step=[&]() {
            director.Step(kTick,context);
            duck.Step(director.State().line!=dlg::Line::None,kTick);
        };
        // Multiple audible ticks exercise the pending Grate request held while
        // the current line plays, not merely a same-tick start/finish fixture.
        for (unsigned tick=0; tick<12; ++tick) step();
        const auto active=director.State();
        Check(active.line==dlg::Line::Grate && Near(duck.Gain(),.65),
              "direct Grate finalization fixture seeds an active audible request and completed attack");
        if (finish==0) director.Skip();
        if (finish==1)
            Check(director.Complete(active.line,active.generation),
                  "matching completion finishes the real active Grate line");
        if (finish==2) {
            unsigned ticks=0;
            while (director.State().line!=dlg::Line::None && ticks<600) { step(); ++ticks; }
            Check(ticks>0 && ticks<600, "missing audio callback still reaches bounded Grate duration completion");
        }
        Check(director.State().line==dlg::Line::None && director.State().grateConsumed &&
              !director.Complete(active.line,active.generation),
              "skip/current completion/duration finish consumes Grate and invalidates the old callback");
        step();
        Check(director.State().line==dlg::Line::None && Near(duck.Gain(),.65),
              "consumed Grate cannot restart from a stale pending request while the listener remains audible");
        bool quiet=true;
        for (unsigned tick=0; tick<90; ++tick) {
            step(); quiet &= director.State().line==dlg::Line::None;
        }
        Check(quiet && duck.Gain()==1.f,
              "every Grate completion route stays consumed and fully releases its presentation gain");
    }
}
GameSimulationConfig GrateConfig() {
    auto config=ProductionGameSimulationConfig();
    config.playerStartX=0; config.playerStartZ=-9.4f; config.playerStartYawRadians=0;
    config.developmentRescueJourney=true;
    return config;
}
InputSnapshot Neutral() { InputSnapshot input; input.damageEnabled=false; input.tutorialEnabled=false; return input; }
void RealGrateAndLifecycle() {
    GameSimulation game(GrateConfig()); auto input=Neutral();
    for (unsigned i=0; i<6; ++i) game.StepFixed(input);
    const auto active=game.Snapshot();
    Check(active.chapterDialogue.line==dlg::Line::Grate && active.dialogueBedGain<1.f && active.dialogueBedGain>.65f,
          "normal shared Grate proximity starts speech and a gradual campaign duck");
    input.paused=true; game.AdvanceFrame(input,0);
    for (unsigned i=0; i<120; ++i) game.StepFixed(input);
    Check(game.Snapshot().chapterDialogue.generation==active.chapterDialogue.generation &&
          game.Snapshot().chapterDialogue.elapsed==active.chapterDialogue.elapsed &&
          game.Snapshot().dialogueBedGain==active.dialogueBedGain, "zero-tick and long pause freeze the exact live line/gain");
    input.paused=false;
    for (unsigned i=0; i<12; ++i) game.StepFixed(input);
    Check(Near(game.Snapshot().dialogueBedGain,.65), "resuming the same line continues its shared attack");
    ++input.commands.dialogueSkip; game.StepFixed(input);
    Check(game.Snapshot().chapterDialogue.line==dlg::Line::None && Near(game.Snapshot().dialogueBedGain,.65),
          "real skip enters the quiet hold instead of immediately restoring full volume");
    for (unsigned i=0; i<90; ++i) game.StepFixed(input);
    Check(game.Snapshot().dialogueBedGain==1.f, "skipped Grate fully restores the bed with no permanent reduction");
    game.ResetRoute();
    Check(game.Snapshot().dialogueBedGain==1.f, "route reset discards the former envelope immediately");
    for (unsigned i=0; i<12; ++i) game.StepFixed(input);
    Check(game.Snapshot().dialogueBedGain<1.f, "new game may independently trigger its real Grate line");
    game.RetryEncounter();
    Check(game.Snapshot().dialogueBedGain==1.f, "encounter retry clears pre-retry presentation gain");
    Check(game.ApplyShowcaseCheckpoint(0) && game.Snapshot().dialogueBedGain==1.f,
          "historical checkpoint import clears chapter envelope without rewriting legacy timing");
    game.SetDevelopmentRescueJourney(false);
    Check(game.Snapshot().dialogueBedGain==1.f && DialogueBedGain(game.Snapshot())==1.f,
          "switching out of chapter mode retains legacy unity");

    GameSimulation leaving(GrateConfig()); input=Neutral();
    for (unsigned i=0; i<12; ++i) leaving.StepFixed(input);
    const auto oldLine=leaving.Snapshot().chapterDialogue;
    input.moveForward=-1;
    unsigned ticks=0;
    while (leaving.Snapshot().chapterDialogue.line==dlg::Line::Grate && ticks<240) { leaving.StepFixed(input); ++ticks; }
    Check(ticks>0 && ticks<240 && leaving.Snapshot().chapterDialogue.line==dlg::Line::None &&
          !leaving.Snapshot().chapterDialogue.grateConsumed,
          "ordinary player travel out of Grate range cancels an unheard call while retaining re-entry");
    input.moveForward=0; input.dialogueCompletionLine=static_cast<std::uint32_t>(oldLine.line);
    input.dialogueCompletionGeneration=oldLine.generation;
    for (unsigned i=0; i<90; ++i) leaving.StepFixed(input);
    Check(leaving.Snapshot().dialogueBedGain==1.f && leaving.Snapshot().chapterDialogue.line==dlg::Line::None,
          "out-of-range cancellation plus stale completion cannot retain attenuation or restart speech");
}

struct Clips {
    std::array<std::vector<std::int16_t>,kMusicPcmCueCount> body,tail;
    std::array<MusicPcmClip,kMusicPcmCueCount> clips{};
    Clips() { for (std::size_t i=1; i<clips.size(); ++i) {
        body[i].assign(kMusicPcmAssets[i].bodyFrames*2u,static_cast<std::int16_t>(100*i));
        tail[i].assign(kMusicPcmTailFrames*2u,static_cast<std::int16_t>(10*i));
        clips[i]={body[i],tail[i]};
    } }
};
void MusicOutputOnly() {
    SimulationSnapshot state; state.developmentRescueJourney=true;
    state.finale.lichDefeated=true; state.lich.phase=LichPhase::Dead;
    MusicDirector ducked,control;
    for (unsigned i=0; i<30; ++i) {
        state.dialogueBedGain=i<15 ? .65f : 1.f;
        const auto actual=ducked.Update(state,{},i*.1,1);
        auto unity=state; unity.dialogueBedGain=1.f;
        const auto expected=control.Update(unity,{},i*.1,1);
        Check(actual.cue==expected.cue && actual.looping==expected.looping && actual.revision==expected.revision &&
              Near(actual.positionSeconds,expected.positionSeconds) && Near(actual.revealGain,state.dialogueBedGain),
              "bed gain changes only the existing output envelope, never cue/cursor/revision");
    }
    state.finale={}; state.lich.phase=LichPhase::MaintainingRange;
    state.lich.revealPhase=KeeperRevealPhase::Awakening; state.lich.revealElapsedSeconds=2;
    state.dialogueBedGain=.65f;
    MusicDirector reveal,unityReveal; auto unity=state; unity.dialogueBedGain=1.f;
    const auto a=reveal.Update(state,{},0,2), b=unityReveal.Update(unity,{},0,2);
    Check(a.cue==b.cue && a.revision==b.revision && Near(a.revealGain,b.revealGain*.65f),
          "dialogue gain multiplies the accepted Keeper envelope instead of replacing it");

    const Clips clips; MusicPlaybackSession session(clips.clips), reference(clips.clips);
    MusicPlaybackInbox inbox; state={}; state.developmentRescueJourney=true;
    std::array<float,64> actual{},expected{};
    std::uint64_t epoch=0,revision=0;
    for (unsigned i=0; i<16; ++i) {
        state.tickIndex=i; state.dialogueBedGain=.65f+.35f*i/15.f;
        Check(inbox.Publish(state,{},50,0,false), "current immutable bed gain crosses the worker inbox");
        auto packet=inbox.Take();
        if (!i) epoch=packet.restartEpoch;
        auto neutral=packet; neutral.snapshot.dialogueBedGain=1.f;
        Check(session.Render(packet,actual)==MusicPcmStatus::Ok && reference.Render(neutral,expected)==MusicPcmStatus::Ok &&
              actual==expected && packet.restartEpoch==epoch &&
              Near(session.Selection().revealGain,state.dialogueBedGain),
              "gain updates leave synthetic Core PCM byte-for-byte equal and retain the existing session epoch");
        if (!i) revision=session.Selection().revision;
        Check(session.Selection().revision==revision, "ordinary speech gain publication does not restart a music bed");
    }
    auto packet=inbox.Take(); packet.externallySuspended=true;
    const auto frames=session.GeneratedFrames(); const auto gain=session.Selection().revealGain;
    Check(session.Render(packet,actual)==MusicPcmStatus::Suspended && session.GeneratedFrames()==frames &&
          session.Selection().revealGain==gain, "worker suspension freezes content and its last published output gain");
    packet.externallySuspended=false;
    Check(session.Observe(packet)==MusicPcmStatus::Ok && session.GeneratedFrames()==frames &&
          session.Selection().revision==revision, "ordinary resume observes gain without a cursor or revision reset");
}

cp::Checkpoint ActiveRecord() {
    cp::Checkpoint c; c.vitality=1;
    c.guards={cp::GuardPose{-4.f,-15.2f,.3f},cp::GuardPose{-6.f,-16.f,-.7f}};
    c.dialogue.grateConsumed=c.dialogue.rescueConsumed=true;
    c.dialogue.activeLine=dlg::Line::RescueFound;
    c.dialogue.continuations[0]=dlg::Line::RescueRope; c.dialogue.continuationCount=1;
    return c;
}
bool Restore(GameSimulation& game,const cp::Checkpoint& c,
             const std::shared_ptr<const TriangleSupportSurface>& terrain,
             const InputSnapshot& input,std::uint64_t publication) {
    const auto token=game.BeginCampaignRestore(c);
    return token && game.CommitCampaignRestore(*token,{token->worldGeneration,c.zone},terrain,input,publication);
}
void LogicalRestore(const std::shared_ptr<const TriangleSupportSurface>& terrain) {
    GameSimulation game(ProductionGameSimulationConfig()); const auto saved=ActiveRecord();
    auto input=Neutral(); input.yawRadians=saved.yaw; input.pitchRadians=saved.pitch;
    std::uint64_t publication=100;
    if (!Restore(game,saved,terrain,input,publication)) { Check(false,"active-dialogue logical restore must commit"); return; }
    Check(game.Snapshot().dialogueBedGain==1.f && game.Snapshot().chapterDialogue.line==dlg::Line::RescueFound &&
          game.Snapshot().playerVitals.vitality==1, "restore restarts unfinished speech with unity envelope and unchanged saved health");
    for (unsigned i=0; i<12; ++i) game.StepFixed(input,kTick,++publication);
    const auto old=game.Snapshot().chapterDialogue;
    Check(Near(game.Snapshot().dialogueBedGain,.65), "fresh resumed restored speech earns a new bounded attack");
    if (!Restore(game,saved,terrain,input,publication)) { Check(false,"replacement active-dialogue load must commit"); return; }
    const auto current=game.Snapshot().chapterDialogue;
    Check(current.generation>old.generation && current.elapsed==0 && game.Snapshot().dialogueBedGain==1.f,
          "repeated restore imports neither old callback identity nor old attenuation");
    input.dialogueCompletionLine=static_cast<std::uint32_t>(old.line); input.dialogueCompletionGeneration=old.generation;
    game.StepFixed(input,kTick,++publication);
    Check(game.Snapshot().chapterDialogue.line==dlg::Line::RescueFound && game.Snapshot().dialogueBedGain<1.f,
          "matching old line with stale generation cannot finish the current restored speech");
    const float before=game.Snapshot().dialogueBedGain;
    input.dialogueCompletionGeneration=current.generation; game.StepFixed(input,kTick,++publication);
    Check(game.Snapshot().chapterDialogue.line==dlg::Line::RescueRope && game.Snapshot().dialogueBedGain<=before,
          "current completion starts its retained continuation without a unity-volume gap");
    input.dialogueCompletionGeneration=0; input.dialogueCompletionLine=0;
    for (unsigned i=0; i<12; ++i) game.StepFixed(input,kTick,++publication);
    ++input.commands.dialogueSkip; game.StepFixed(input,kTick,++publication);
    for (unsigned i=0; i<90; ++i) game.StepFixed(input,kTick,++publication);
    Check(game.Snapshot().chapterDialogue.line==dlg::Line::None && game.Snapshot().dialogueBedGain==1.f,
          "skipped restored continuation releases completely without stale callback pumping");
    auto quiet=saved; quiet.dialogue.activeLine=dlg::Line::None; quiet.dialogue.continuations={}; quiet.dialogue.continuationCount=0;
    Check(Restore(game,quiet,terrain,input,publication) && game.Snapshot().dialogueBedGain==1.f,
          "quiet checkpoint replacement starts at unity with no saved ducking state");
}
std::string Read(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary); return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}
void OutputWiring(const std::filesystem::path& root) {
    const auto windows=Read(root/"src/platform/windows/DiagnosticWindow.cpp");
    const auto bridge=Read(root/"android/app/src/main/cpp/android_probe_bridge.cpp");
    const auto winMusic=Read(root/"src/platform/windows/WindowsMusicPlayback.cpp");
    const auto androidMusic=Read(root/"android/app/src/main/cpp/android_music_playback.cpp");
    const auto javaMusic=Read(root/"android/app/src/main/java/com/samfa12/hordelanternrt/HordeMusicPlayback.java");
    Check(windows.find("WindowsSfxSourceGain(horde::audio::DialogueBedGain(simulation), context.sfxVolumePercent)")!=std::string::npos &&
          windows.find("engine.StartOrUpdateLoop(loopKey, path, gains.left, gains.right, sfxGain)")!=std::string::npos,
          "Windows waterfall applies shared duck through the existing independently controlled SFX output gain");
    Check(bridge.find("PackStereoGains(waterfallGains.left*dialogueBedGain, waterfallGains.right*dialogueBedGain)")!=std::string::npos &&
          bridge.find("gWaterfallStereoGains.store(")!=std::string::npos,
          "Android publishes coherent stereo waterfall attenuation through its existing atomic tuple");
    Check(winMusic.find("sourceVoice_->SetVolume(static_cast<float>(percent) / 100.0f * revealGain)")!=std::string::npos &&
          androidMusic.find("music->session->Selection().revealGain")!=std::string::npos &&
          javaMusic.find("static float applyOutputGain(")!=std::string::npos &&
          javaMusic.find("output.setVolume(")!=std::string::npos,
          "both music outputs retain the existing user-volume multiplier stage rather than modifying canonical PCM");
}
} // namespace
int main(int argc,char** argv) {
    if (argc!=2) { std::cerr<<"usage: DialogueDuckingTests <source-root>\n"; return 2; }
    Envelope(); SnapshotGuard(); SharedDialogueChain(); SharedGrateFinalization(); RealGrateAndLifecycle(); MusicOutputOnly();
    const auto directory=std::filesystem::path(argv[1])/"assets/models/world/runtime/chapter-terrain-v1";
    horde::scene::assets::AssetManifest manifest; horde::scene::assets::StaticMeshAsset mesh; std::string diagnostic;
    if (!horde::scene::assets::AssetManifest::Load(directory/"asset.manifest.json",manifest,diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(directory/"chapter-terrain-lod0.runtime.glb",manifest,mesh,diagnostic)) {
        std::cerr<<"Actual chapter terrain intake failed: "<<diagnostic<<'\n'; return 1;
    }
    const auto terrain=horde::scene::PrepareChapterTerrainSurface(mesh,diagnostic);
    if (!terrain) { std::cerr<<diagnostic<<'\n'; return 1; }
    LogicalRestore(terrain); OutputWiring(argv[1]);
    std::cout<<checks<<" dialogue ducking host checks, "<<failures<<" failures\n";
    std::cout<<"Synthetic PCM and logical terrain-backed restores use no output device or owner saves. "
                 "Listening, physical lifecycle, voice intelligibility and perceived mix remain unaccepted.\n";
    return failures?1:0;
}
