#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/dialogue/SubtitleLayout.h"
#include "platform/windows/WindowsChapterDialogue.h"
#include <cstdlib>
#include <iostream>
using namespace horde::gameplay;
using namespace horde::gameplay::dialogue;
void Check(bool value,const char* msg) { if(!value) {std::cerr<<msg<<'\n';std::exit(1);} }
int main() {
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
    SubtitlePlacementLatch latch;request={};request.position=SubtitlePosition::Auto;
    Check(ResolveWindowsSubtitleLayout(request,3,latch,SubtitlePosition::Auto).region==SubtitlePosition::Bottom,"Windows Auto bottom");
    request.touch=true;Check(ResolveWindowsSubtitleLayout(request,3,latch,SubtitlePosition::Auto).region==SubtitlePosition::Bottom,"incidental input does not move active line");
    request.position=SubtitlePosition::Top;
    Check(ResolveWindowsSubtitleLayout(request,3,latch,SubtitlePosition::Top).region==SubtitlePosition::Top,"explicit mid-line preference reflows deliberately");
    request.position=SubtitlePosition::Bottom;
    Check(ResolveWindowsSubtitleLayout(request,4,latch,SubtitlePosition::Bottom).region==SubtitlePosition::Bottom,"new line preserves explicit bottom");
    Check(WindowsDialogueSourceGain(500)==1&&WindowsDialogueSourceGain(-5)==0&&WindowsSfxSourceGain(-1,80)==0,"volume clamps");
    CompanionRouteFixture companion;auto before=companion.State();
    companion.Step(.05f,before.x,before.z,true,false,false);
    Check(!companion.State().walking&&companion.State().wait==CompanionWait::Reunion,"companion cannot depart before fresh proof");
    companion.Step(.05f,before.x,before.z,true,true,false);
    Check(companion.State().walking&&!companion.State().visualAdmitted,"actual terrain movement is a labelled logical fixture");
    before=companion.State();companion.Step(.05f,before.x+20,before.z,true,true,false);
    Check(companion.State().x==before.x&&companion.State().wait==CompanionWait::PlayerLagging,"lagging player holds companion without teleport");
    companion.Step(.05f,before.x,before.z,false,true,false);
    Check(companion.State().x==before.x&&companion.State().wait==CompanionWait::PlayerInTomb,"returning to tomb holds companion");
    companion.Step(.05f,before.x,before.z,true,true,true);Check(companion.State().x==before.x,"pause freezes companion");

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
