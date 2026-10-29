#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/dialogue/SubtitleLayout.h"
#include "platform/windows/WindowsChapterDialogue.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace horde::gameplay;
using namespace horde::gameplay::dialogue;
void Check(bool value,const char* msg) { if(!value) {std::cerr<<msg<<'\n';std::exit(1);} }
int main() {
    simulation::SimulationSnapshot guidance;
    guidance.developmentRescueJourney=true;
    guidance.chapterDialogue.waitingForRaise=true;
    guidance.interaction.heldLightKind=interactions::HeldLightKind::RewardLantern;
    guidance.interaction.heldLightPose=interactions::HeldLightPose::Low;
    guidance.rescue.phase=traversal::Phase::UpperSafe;
    Check(guidance.NeedsRaiseLanternPrompt(),"grounded pending proof requests explicit raise guidance");
    guidance.chapterDialogue.line=Line::None;
    Check(guidance.NeedsRaiseLanternPrompt(),"objective survives the spoken question ending");
    for(auto phase:{traversal::Phase::LowerSafe,traversal::Phase::Ascent,traversal::Phase::Descent}) {
        guidance.rescue.phase=phase;
        Check(!guidance.NeedsRaiseLanternPrompt(),"no raise instruction during rope traversal or below ground");
    }
    guidance.rescue.phase=traversal::Phase::UpperSafe;
    guidance.interaction.heldLightPose=interactions::HeldLightPose::High;
    Check(!guidance.NeedsRaiseLanternPrompt(),"raised pose removes the redundant raise instruction");
    guidance.interaction.heldLightPose=interactions::HeldLightPose::Low;
    guidance.chapterDialogue.proofAccepted=true;
    Check(!guidance.NeedsRaiseLanternPrompt(),"accepted proof cannot return as an objective on lowering");
    guidance.chapterDialogue.proofAccepted=false;
    guidance.interaction.heldLightKind=interactions::HeldLightKind::Torch;
    Check(!guidance.NeedsRaiseLanternPrompt(),"wrong equipment never offers reward-lantern proof");
    const std::filesystem::path assetRoot = std::filesystem::path(HORDE_RT_SOURCE_DIR) / "assets";
    for (const auto& spec : kLines) {
        if (spec.id == Line::None) continue;
        const auto path = horde::platform::windows::WindowsDialogueAudioPath(assetRoot, spec.audio);
        Check(!path.empty() && std::filesystem::is_regular_file(path),
            "every Kit and Keeper voice resolves to actual bytes from the Windows assets root");
        Check(path.lexically_relative(assetRoot).generic_string().starts_with("audio/"),
            "dialogue root is joined once rather than duplicating assets/assets");
    }
    Check(horde::platform::windows::WindowsDialogueAudioPath(assetRoot,"assets/../private.wav").empty() &&
          horde::platform::windows::WindowsDialogueAudioPath(assetRoot,"C:/private.wav").empty(),
          "invalid catalogue paths cannot escape the supplied asset root");
    Director d;Context c;c.player={2.6f,-.95f,-8.6f};c.listener={2.6f,.7f,-8.6f};
    d.Step(.016f,c);Check(d.State().line==Line::Grate,"eligible passing approach triggers without looking");
    auto old=d.State();c.player.x=50;d.Step(.016f,c);
    Check(d.State().line==Line::None&&!d.State().grateConsumed,"stale local call cancels; eligible return retained");
    Check(!d.Complete(old.line,old.generation),"stale completion rejected");
    c.player.x=2.6f;d.Step(.016f,c);c.combat=true;d.Step(.016f,c);
    Check(d.State().grateConsumed&&d.State().line==Line::None,"combat interruption consumes grate");
    c.combat=false;c.claimed=true;d.Step(.016f,c);Check(d.State().line==Line::RescueFound,"renewed rescue contact");
    old=d.State();d.Complete(old.line,old.generation);d.Step(.016f,c);
    Check(d.State().line==Line::RescueRope,"ordered bounded rescue continuation");
    d.Skip();c.exterior=true;c.lanternLow=true;c.manualRaiseSequence=12;d.Step(.016f,c);
    Check(d.State().waitingForRaise,"grounded lowered reunion arms manual proof");
    c.lanternHigh=true;c.lanternLow=false;c.visibleRaiseSequence=12;d.Step(.016f,c);
    Check(!d.State().proofAccepted,"old pre-climb raise cannot react");
    c.manualRaiseSequence=13;d.Step(.016f,c);Check(!d.State().proofAccepted,"fresh but not presented cannot react");
    c.visibleRaiseSequence=13;d.Step(.016f,c);Check(d.State().proofAccepted,"fresh visibly presented raise accepts");
    Check(d.State().line==Line::ReunionProof,"visible lantern proof starts its response");
    d.Skip();d.Step(.016f,c);Check(d.State().line==Line::ReunionFirstPiece,"proof response continues to first piece");
    d.Skip();d.Step(.016f,c);Check(d.State().line==Line::ReunionDepart,"first piece continues to departure");
    d.Skip();d.Step(.016f,c);Check(d.State().line==Line::None,"departure continuation is bounded");
    old=d.State();c.paused=true;d.Step(.05f,c);Check(d.State().elapsed==old.elapsed,"pause freezes fallback");
    d.Reset(true);Check(d.State().grateConsumed&&d.State().proofAccepted,"reconstruction preserves logical once flags");
    Check(!d.Complete(old.line,old.generation),"reset callback cannot finish a new generation");
    d.Reset();Check(!d.State().grateConsumed&&!d.State().proofAccepted,"new game resets once flags");
    c={};d.Request(Line::KeeperSense,{1,2,3},c);old=d.State();c.listener={8,9,10};d.Step(.05f,c);
    Check(d.State().listener.x==old.listener.x&&d.State().source.y==2,"source/listener tuple freezes at emission");
    // Continuous visible-companion speech is distinct from event-time calls.
    for(unsigned n=static_cast<unsigned>(Line::ReunionQuestion);n<=static_cast<unsigned>(Line::ForestVillage);++n) {
        Director live;Context lc;lc.companionSource=Tuple{4,3,2};lc.listener={1,2,3};
        live.Request(static_cast<Line>(n),{-32,3.6f,-10},lc);
        const auto started=live.State();const auto progress=live.ExportProgress();
        Check(started.source.x==4&&started.source.y==3,"Kit starts at the supplied current grounded actor");
        lc.companionSource=Tuple{5,4,1};lc.listener={2,3,4};lc.yaw=.75f;
        live.RefreshCompanionSpatial(lc);
        Check(live.State().source.x==5&&live.State().source.y==4&&live.State().listener.x==2&&
            live.State().listenerYaw==.75f&&live.State().generation==started.generation&&
            live.State().elapsed==started.elapsed&&live.ExportProgress().activeLine==progress.activeLine,
            "Kit spatial refresh follows actor/listener without restarting speech or story");
        lc.paused=true;lc.listener.x=9;lc.companionSource=Tuple{9,9,9};
        live.RefreshCompanionSpatial(lc);
        Check(live.State().source.x==5&&live.State().listener.x==2,"paused Kit anchors remain frozen");
        lc.paused=false;lc.companionSource.reset();live.RefreshCompanionSpatial(lc);
        Check(live.State().source.x==5&&live.State().listener.x==9,"unavailable actor retains last valid source while listener follows");
        lc.companionSource=Tuple{std::numeric_limits<float>::quiet_NaN(),4,1};lc.listener.x=10;
        live.RefreshCompanionSpatial(lc);
        Check(live.State().source.x==5&&live.State().listener.x==9,"invalid Kit spatial tuple cannot publish partial/nonfinite data");
        lc.companionSource=Tuple{6,4,1};lc.alive=false;live.RefreshCompanionSpatial(lc);
        Check(live.State().source.x==5,"dead player cannot refresh active Kit speech");
    }
    c.companionSource=Tuple{7,8,9};d.RefreshCompanionSpatial(c);
    Check(d.State().source.x==1&&d.State().listener.x==old.listener.x,
        "Kit refresh cannot change Keeper emission tuples");
    for(int i=0;i<100;i++)d.Step(.05f,c);Check(d.State().line==Line::None,"missing/failed clip has bounded fallback");
    Director interrupted;Context claim;claim.claimed=true;
    interrupted.Step(.016f,claim);
    Check(interrupted.State().line==Line::RescueFound,"priority fixture begins rescue contact");
    const auto interruptedRescue=interrupted.State();
    interrupted.Request(Line::KeeperSense,{1,2,3},claim);
    Check(interrupted.State().line==Line::KeeperSense &&
        !interrupted.Complete(interruptedRescue.line,interruptedRescue.generation),
        "priority preemption invalidates the interrupted callback");
    interrupted.Skip();interrupted.Step(.016f,claim);
    Check(interrupted.State().line==Line::RescueRope,
        "priority preemption must retain the queued once-only rescue continuation");
    const auto validLine=interrupted.State();
    interrupted.Request(static_cast<Line>(kLines.size()),{1,2,3},claim);
    Check(interrupted.State().line==validLine.line && interrupted.State().generation==validLine.generation,
        "unknown line at the catalog boundary cannot replace a valid line");
    // Logical recovery restarts only unfinished speech, with fresh identities
    // and current spatial tuples; no audio clock or presentation receipt loads.
    Director saved;Context recovery;recovery.claimed=true;
    saved.Step(.05f,recovery);
    const auto savedFound=saved.State();const auto foundProgress=saved.ExportProgress();
    Check(Director::ValidateProgress(foundProgress),"actual rescue progress validates");
    Director restored;recovery.listener={8,9,10};recovery.yaw=.4f;
    Check(restored.RestoreProgress(foundProgress,recovery,savedFound.generation,70),
        "unfinished rescue restores into a fresh director");
    Check(restored.State().line==Line::RescueFound && restored.State().elapsed==0 &&
        restored.State().generation>savedFound.generation && restored.State().listener.x==8 &&
        restored.State().source.x==-33.7f && restored.State().listenerYaw==.4f,
        "restored speech has fresh callback identity and current listener tuple");
    Check(!restored.Complete(savedFound.line,savedFound.generation),"old matching line completion is stale after restore");
    restored.Skip();restored.Step(.016f,recovery);
    Check(restored.State().line==Line::RescueRope,"restored rescue keeps its ordered rope response");
    restored.Skip();recovery.exterior=true;recovery.lanternLow=true;
    recovery.manualRaiseSequence=70;restored.Step(.016f,recovery);
    const auto pendingProof=restored.ExportProgress();
    Check(pendingProof.waitingForRaise && Director::ValidateProgress(pendingProof),"pending proof exports logical waiting state");
    Director pending;recovery.lanternLow=false;recovery.lanternHigh=true;
    recovery.manualRaiseSequence=80;recovery.visibleRaiseSequence=80;
    Check(pending.RestoreProgress(pendingProof,recovery,restored.State().generation,80),"pending proof restores");
    pending.Step(.016f,recovery);
    Check(!pending.State().proofAccepted,"restored visible held pose cannot satisfy a fresh manual raise");
    recovery.manualRaiseSequence=81;recovery.visibleRaiseSequence=80;pending.Step(.016f,recovery);
    Check(!pending.State().proofAccepted,"restored proof rejects an unpresented fresh raise");
    recovery.visibleRaiseSequence=81;pending.Step(.016f,recovery);
    const auto proofProgress=pending.ExportProgress();
    Check(proofProgress.proofAccepted && proofProgress.activeLine==Line::ReunionProof &&
        proofProgress.continuationCount==2 && Director::ValidateProgress(proofProgress),
        "accepted proof retains its unfinished response chain");
    Director accepted;
    Check(accepted.RestoreProgress(proofProgress,recovery,pending.State().generation,81),"accepted proof restores");
    accepted.Skip();accepted.Step(.016f,recovery);
    Check(accepted.State().line==Line::ReunionFirstPiece,"accepted proof recovery keeps first piece");
    accepted.Skip();accepted.Step(.016f,recovery);
    Check(accepted.State().line==Line::ReunionDepart,"accepted proof recovery keeps departure");
    accepted.Skip();accepted.Step(.016f,recovery);
    const auto completedProgress=accepted.ExportProgress();Director completed;
    Check(completed.RestoreProgress(completedProgress,recovery,accepted.State().generation,81),"completed reunion restores");
    completed.Step(.016f,recovery);
    Check(completed.State().proofAccepted && completed.State().line==Line::None,
        "completed reunion does not replay proof or response lines");
    const auto beforeInvalid=completed.State();
    const auto rejects=[&](const Progress& bad) {
        Check(!Director::ValidateProgress(bad) &&
            !completed.RestoreProgress(bad,recovery,100,81) &&
            completed.State().generation==beforeInvalid.generation &&
            completed.State().line==beforeInvalid.line && completed.State().proofAccepted,
            "malformed logical progress fails without mutation");
    };
    auto bad=proofProgress;bad.waitingForRaise=true;rejects(bad);
    bad=proofProgress;bad.continuationCount=9;rejects(bad);
    bad=proofProgress;bad.continuations[1]=bad.continuations[0];rejects(bad);
    bad=proofProgress;bad.continuations[2]=Line::ForestNight;rejects(bad);
    bad=proofProgress;bad.activeLine=static_cast<Line>(kLines.size());rejects(bad);
    bad=proofProgress;bad.continuations[0]=Line::ReunionDepart;bad.continuations[1]=Line::ReunionFirstPiece;rejects(bad);
    bad=proofProgress;bad.proofAccepted=false;rejects(bad);
    bad=proofProgress;bad.continuations={};bad.continuationCount=0;rejects(bad);
    bad=proofProgress;bad.continuations[1]=Line::None;bad.continuationCount=1;rejects(bad);
    bad=proofProgress;bad.activeLine=Line::ReunionFirstPiece;
    bad.continuations={};bad.continuationCount=0;rejects(bad);
    bad=proofProgress;bad.activeLine=Line::None;bad.continuations[0]=Line::ReunionProof;
    bad.continuations[1]=Line::ReunionDepart;rejects(bad);
    auto queuedProof=proofProgress;queuedProof.activeLine=Line::None;
    queuedProof.continuations[0]=Line::ReunionProof;queuedProof.continuations[1]=Line::ReunionFirstPiece;
    queuedProof.continuations[2]=Line::ReunionDepart;queuedProof.continuationCount=3;
    Check(Director::ValidateProgress(queuedProof),"queued interrupted proof retains its complete required response suffix");
    bad=completedProgress;bad.forestClueConsumed=true;rejects(bad);
    auto forestProgress=completedProgress;forestProgress.activeLine=Line::ForestNight;
    forestProgress.forestNightConsumed=true;
    forestProgress.continuations[0]=Line::ForestWaystone;
    forestProgress.continuations[1]=Line::ForestClue;forestProgress.continuationCount=2;
    Director forestRecovery;
    Check(forestRecovery.RestoreProgress(forestProgress,recovery,100,81) &&
        forestRecovery.State().line==Line::ForestNight && forestRecovery.State().forestNightConsumed,
        "forest once flag set at line start does not drop interrupted active speech");
    forestRecovery.Skip();forestRecovery.Step(.016f,recovery);
    Check(forestRecovery.State().line==Line::ForestWaystone,"forest recovery preserves queued waystone order");
    forestRecovery.Skip();forestRecovery.Step(.016f,recovery);
    Check(forestRecovery.State().line==Line::ForestClue,"forest recovery preserves queued clue order");
    auto invalidContext=recovery;invalidContext.listener.x=std::numeric_limits<float>::infinity();
    Check(!completed.RestoreProgress(completedProgress,invalidContext,100,81) &&
        completed.State().generation==beforeInvalid.generation,
        "nonfinite restoration context fails without mutation");
    Check(!completed.RestoreProgress(completedProgress,recovery,UINT64_MAX-1,81) &&
        completed.State().generation==beforeInvalid.generation,
        "exhausted callback generation cannot restore by reusing identity");
    auto config=simulation::ProductionGameSimulationConfig();config.developmentRescueJourney=true;
    simulation::GameSimulation sim(config);simulation::InputSnapshot input;
    input.damageEnabled=false;input.hasAuthoritativePlayerPose=true;
    input.authoritativePlayerX=2.6f;input.authoritativePlayerZ=-8.6f;
    sim.StepFixed(input);Check(sim.Snapshot().chapterDialogue.line==Line::Grate,"actual simulation publishes grate line");
    const auto grateLine=sim.Snapshot().chapterDialogue;
    input.paused=true;input.dialogueCompletionLine=static_cast<std::uint32_t>(grateLine.line);
    input.dialogueCompletionGeneration=grateLine.generation;
    sim.StepFixed(input);
    Check(sim.Snapshot().chapterDialogue.line==grateLine.line &&
        sim.Snapshot().chapterDialogue.generation==grateLine.generation,
        "paused direct fixed step must reject a matching audio completion");
    input.paused=false;input.dialogueCompletionLine=0;input.dialogueCompletionGeneration=0;
    input.paused=true;
    sim.SynchronizePausedInput(input,1,simulation::PausedInputPolicy::DiscardAllCommands);
    Check(sim.Snapshot().chapterDialogue.line==grateLine.line &&
        sim.Snapshot().chapterDialogue.generation==grateLine.generation &&
        !sim.Snapshot().chapterDialogue.grateConsumed,"focus/surface pause retains the active grate line without consuming it");
    input.paused=false;
    input.commands.dialogueSkip++;sim.StepFixed(input);
    Check(sim.Snapshot().chapterDialogue.grateConsumed&&sim.Snapshot().lastConsumedAttackSequence==0&&
        sim.Snapshot().lastConsumedInteractSequence==0,"skip independent from attack/interact");
    sim.ResetRoute();Check(!sim.Snapshot().chapterDialogue.grateConsumed,"route reset republishes fresh narrative");
    SubtitleRequest request;request.width=1280;request.height=720;request.lines=3;
    auto layout=LayoutSubtitle(request);Check(layout.fits&&layout.region==SubtitlePosition::Bottom,"desktop Auto bottom");
    request.touch=true;layout=LayoutSubtitle(request);Check(layout.region==SubtitlePosition::Top,"touch Auto top");
    request.position=SubtitlePosition::Bottom;request.height=640;request.width=360;
    request.fontPixels=32;request.lines=5;request.bottomReserved=240;
    layout=LayoutSubtitle(request);Check(layout.region==SubtitlePosition::Bottom,"explicit preference not flipped");
    request.lines=100;Check(!LayoutSubtitle(request).fits,"extreme size exposes limitation rather than clipping");
    using namespace horde::platform::windows;
    Check(WindowsSfxSourceGain(.5f,0)==0&&WindowsDialogueSourceGain(70)>.69f,"SFX mute leaves voice independent");
    Check(WindowsDialogueSourceGain(0)==0&&WindowsSfxSourceGain(.5f,70)>0,"voice mute leaves effects independent");
    Check(WindowsSubtitleCaption("Keeper","Come closer")=="Keeper: Come closer",
        "normal subtitle contains only speaker and spoken words, no modal/footer paragraphs");
    request={};request.lines=1;request.bottomReserved=WindowsSubtitleBottomReserved(false,100);request.margin=12;
    layout=LayoutSubtitle(request);
    Check(layout.fits&&layout.y>request.height*4/5&&layout.height<request.height/8,
        "desktop single-line subtitle stays low and leaves combat view clear");
    request.bottomReserved=WindowsSubtitleBottomReserved(true,100);
    layout=LayoutSubtitle(request);Check(layout.y+layout.height<request.height-94,
        "context prompt remains clear above its actual bottom band");
    SubtitlePlacementLatch latch;request={};request.position=SubtitlePosition::Auto;
    Check(ResolveWindowsSubtitleLayout(request,3,latch,SubtitlePosition::Auto).region==SubtitlePosition::Bottom,"Windows Auto bottom");
    request.touch=true;Check(ResolveWindowsSubtitleLayout(request,3,latch,SubtitlePosition::Auto).region==SubtitlePosition::Bottom,"incidental input does not move active line");
    request.position=SubtitlePosition::Top;
    Check(ResolveWindowsSubtitleLayout(request,3,latch,SubtitlePosition::Top).region==SubtitlePosition::Top,"explicit mid-line preference reflows deliberately");
    request.position=SubtitlePosition::Bottom;
    Check(ResolveWindowsSubtitleLayout(request,4,latch,SubtitlePosition::Bottom).region==SubtitlePosition::Bottom,"new line preserves explicit bottom");
    Check(WindowsDialogueSourceGain(500)==1&&WindowsDialogueSourceGain(-5)==0&&WindowsSfxSourceGain(-1,80)==0,"volume clamps");
    // A high flat native surface deliberately disagrees with the old ribbon.
    using Terrain=simulation::TriangleSupportSurface;
    const std::array<Terrain::Triangle,2> land{{
        {{{{-1000,7,-1000},{-1000,7,1000},{1000,7,1000}}}},
        {{{{-1000,7,-1000},{1000,7,1000},{1000,7,-1000}}}}
    }};
    std::string terrainError;auto terrain=Terrain::Build(land,terrainError);
    Check(bool(terrain),"companion native test surface imports");
    CompanionRouteFixture nativeCompanion;auto nativeBefore=nativeCompanion.State();
    nativeCompanion.Step(.05f,nativeBefore.x,nativeBefore.z,true,false,false,terrain.get(),true);
    Check(std::abs(nativeCompanion.State().y-7.f)<.001f&&!nativeCompanion.State().walking,
        "companion must stand on actual native ground while waiting for lantern proof");
    nativeCompanion.Step(.05f,nativeBefore.x,nativeBefore.z,true,true,false,terrain.get(),true);
    Check(nativeCompanion.State().walking&&std::abs(nativeCompanion.State().y-7.f)<.001f,
        "companion movement must follow native ground, not ribbon elevation");
    nativeBefore=nativeCompanion.State();
    nativeCompanion.Step(0,nativeBefore.x,nativeBefore.z,true,true,false,terrain.get(),true);
    Check(nativeCompanion.State().walking && nativeCompanion.State().animationSeconds==nativeBefore.animationSeconds,
        "zero-delta snapshot finalization must preserve the walking clip and pose clock");
    nativeCompanion.Step(.05f,nativeBefore.x,nativeBefore.z,true,true,false,nullptr,true);
    Check(!nativeCompanion.State().walking&&nativeCompanion.State().x==nativeBefore.x&&nativeCompanion.State().y==nativeBefore.y,
        "missing or stale native support must stop companion without fallback or teleport");
    nativeCompanion.Step(.05f,nativeBefore.x,nativeBefore.z,true,true,true,terrain.get(),true);
    Check(nativeCompanion.State().x==nativeBefore.x,"native companion pause preserves position");
    nativeCompanion.Reset();
    Check(nativeCompanion.State().generation>nativeBefore.generation&&!nativeCompanion.State().walking,
        "companion reset invalidates prior route generation");

    CompanionRouteFixture companion;auto before=companion.State();
    companion.Step(.05f,before.x,before.z,true,false,false);
    Check(!companion.State().walking&&companion.State().wait==CompanionWait::Reunion,"companion cannot depart before fresh proof");
    companion.Step(.05f,before.x,before.z,true,true,false);
    Check(companion.State().walking&&!companion.State().visualAdmitted,"actual terrain movement is a labelled logical fixture");
    before=companion.State();companion.Step(.05f,before.x+20,before.z,true,true,false);
    Check(companion.State().x==before.x&&companion.State().wait==CompanionWait::PlayerLagging,"lagging player holds companion without teleport");
    companion.Step(.05f,before.x,before.z,false,true,false);
    Check(companion.State().x==before.x&&companion.State().wait==CompanionWait::PlayerInTomb,"returning to tomb holds companion");
    before=companion.State();
    companion.Step(.05f,before.x,before.z,true,true,true);
    Check(companion.State().x==before.x && companion.State().yaw==before.yaw &&
          companion.State().animationSeconds==before.animationSeconds && companion.State().walking==before.walking,
          "pause freezes companion placement, pose clock and clip");
    CompanionRouteFixture turning;
    const auto startTurn=turning.State();
    turning.Step(.05f,startTurn.x-1,startTurn.z,true,true,false);
    Check(std::abs(turning.State().yaw-startTurn.yaw)<=.12001f,"walking turn is bounded per simulation step");
    turning.Step(.05f,startTurn.x-1,startTurn.z,true,false,false);
    for(int tick=0;tick<40;++tick)turning.Step(.05f,turning.State().x-1,turning.State().z,true,false,false);
    Check(std::abs(turning.State().yaw+1.57079632679f)<.001f,"waiting companion turns to face the player");

    // Outdoor subtitle fixtures use the same translated landmarks as terrain.
    const auto f02=simulation::kWorldRoutePoints[3];
    const auto f03=simulation::kWorldRoutePoints[6];
    const auto f04=simulation::kWorldRoutePoints[7];
    Director forest;Context fc;fc.exterior=true;fc.player={f02.x,f02.y,f02.z};
    fc.listener={f02.x,3.7f,f02.z};fc.lanternLow=true;fc.manualRaiseSequence=40;
    forest.Step(.016f,fc);Check(forest.State().line==Line::ReunionQuestion,"forest fixture starts after reunion setup");
    fc.lanternLow=false;fc.lanternHigh=true;fc.manualRaiseSequence=41;fc.visibleRaiseSequence=40;
    forest.Step(.016f,fc);Check(!forest.State().proofAccepted,"forest progression rejects an unpresented raise");
    fc.visibleRaiseSequence=41;forest.Step(.016f,fc);
    Check(forest.State().proofAccepted&&forest.State().line==Line::ReunionProof,"forest sequence requires fresh visible proof");
    forest.Skip();forest.Step(.016f,fc);
    Check(forest.State().line==Line::ReunionFirstPiece,"forest sequence begins with first piece");
    forest.Skip();forest.Step(.016f,fc);
    Check(forest.State().line==Line::ReunionDepart,"forest sequence preserves departure order");
    forest.Skip();forest.Step(.016f,fc);
    Check(forest.State().line==Line::None,"forest fixture waits until player actually leaves F02");
    fc.player={f02.x,f02.y,f02.z};forest.Step(.016f,fc);
    Check(forest.State().line==Line::None&&!forest.State().forestNightConsumed,"F02 arrival alone does not trigger night line");
    fc.player={f02.x+(f03.x-f02.x)*.65f,f02.y+(f03.y-f02.y)*.65f,
        f02.z+(f03.z-f02.z)*.65f};fc.listener={7,8,9};
    forest.Step(.016f,fc);
    Check(forest.State().line==Line::ForestNight&&forest.State().forestNightConsumed,
        "leaving F02 toward F03 starts the night line once");
    Check(std::abs(forest.State().source.x-f02.x)<.001f &&
        std::abs(forest.State().source.y-(f02.y+1.4f))<.001f &&
        std::abs(forest.State().source.z-f02.z)<.001f && forest.State().listener.x==7 &&
        forest.State().listener.y==8 && forest.State().listener.z==9,
        "forest source uses translated F02 and freezes listener at emission");
    fc.player={f03.x,f03.y,f03.z};fc.listener={10,11,12};forest.Step(.016f,fc);
    Check(forest.State().line==Line::ForestNight,"F03 proximity does not interrupt the night line");
    forest.Skip();forest.Step(.016f,fc);
    Check(forest.State().line==Line::ForestWaystone&&forest.State().forestWaystoneConsumed,
        "F03 proximity starts the waystone line");
    Check(std::abs(forest.State().source.x-f03.x)<.001f &&
        std::abs(forest.State().source.y-(f03.y+1.4f))<.001f &&
        std::abs(forest.State().source.z-f03.z)<.001f && forest.State().listener.x==10 &&
        forest.State().listener.y==11 && forest.State().listener.z==12,
        "waystone source uses translated F03 and freezes listener");
    forest.Skip();forest.Step(.016f,fc);
    Check(forest.State().line==Line::ForestClue&&forest.State().forestClueConsumed,
        "waystone continues to its clue even when skipped");
    forest.Skip();fc.player={f04.x,f04.y,f04.z};forest.Step(.016f,fc);
    Check(forest.State().line==Line::ForestVillage&&forest.State().forestVillageConsumed,
        "F04 proximity starts the village line once");
    old=forest.State();forest.Reset(true);
    Check(forest.State().forestNightConsumed&&forest.State().forestWaystoneConsumed&&
        forest.State().forestClueConsumed&&forest.State().forestVillageConsumed,
        "reconstruction preserves outdoor once flags");
    Check(!forest.Complete(old.line,old.generation),"outdoor reset invalidates stale completion");
    forest.Step(.016f,fc);Check(forest.State().line==Line::None,"reconstruction does not replay consumed F04 line");
    forest.Reset();Check(!forest.State().forestNightConsumed&&!forest.State().forestWaystoneConsumed&&
        !forest.State().forestClueConsumed&&!forest.State().forestVillageConsumed,
        "new route clears outdoor once flags");
    std::cout<<"chapter dialogue: local/ordered/once/fallback/stale/manual-presented/real-simulation/layout cases passed\n";
}
