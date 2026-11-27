#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/traversal/BellwetherWaymarker.h"
#include "gameplay/campaign/LookoutObjective.h"
#include "audio/BellwetherVoiceFixture.h"
#include "gameplay/dialogue/SubtitleLayout.h"
#include "platform/windows/WindowsChapterDialogue.h"
#include "gameplay/SpatialAudio.h"
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>
using namespace horde::gameplay;
using namespace horde::gameplay::dialogue;
void Check(bool value,const char* msg) { if(!value) {std::cerr<<msg<<'\n';std::exit(1);} }
int main() {
    using horde::gameplay::bellwether::SquareWaymarkerReadableFrom;
    using horde::gameplay::bellwether::StepSquareWaymarkerTextVisible;
    constexpr auto& post=horde::gameplay::bellwether::kSquareWaymarkerPost;
    constexpr auto& witness=horde::gameplay::bellwether::kSquareWaymarkerView;
    const auto yawToPost=[](float x,float z) {
        return std::atan2(post[0]-x,-(post[2]-z));
    };
    Check(SquareWaymarkerReadableFrom(witness[0],
              horde::gameplay::bellwether::kSquareWaymarkerTavern[1],witness[1],yawToPost(witness[0],witness[1])),
          "the authored west-side sign view is readable at its measured 3.62 m distance");
    constexpr float squareApproachX=82.3f,squareApproachZ=131.7f;
    Check(SquareWaymarkerReadableFrom(squareApproachX,
              horde::gameplay::bellwether::kSquareWaymarkerDealer[1],squareApproachZ,
              yawToPost(squareApproachX,squareApproachZ)),
          "the sign can also be read naturally from the tavern-side square approach");
    Check(!SquareWaymarkerReadableFrom(witness[0],horde::gameplay::bellwether::kSquareWaymarkerTavern[1],
              witness[1],yawToPost(witness[0],witness[1])+3.14159265f),
          "turning away from the fingerpost removes its read action");
    constexpr float distantX=post[0]+4.25f;
    Check(!SquareWaymarkerReadableFrom(distantX,horde::gameplay::bellwether::kSquareWaymarkerTavern[1],post[2],
              yawToPost(distantX,post[2])),
          "the read action is unavailable outside the authored four metre sign reach");
    Check(!SquareWaymarkerReadableFrom(witness[0],horde::gameplay::bellwether::kSquareWaymarkerTavern[1]+3.f,
              witness[1],yawToPost(witness[0],witness[1])),
          "the sign is not readable when the view is vertically separated from its boards");
    const auto& inscription=horde::gameplay::campaign::kSquareFingerpostInscription;
    Check(std::string_view(inscription).find("Tankard - Tavern, up the oak-railed ramp.")!=std::string_view::npos &&
          std::string_view(inscription).find("Scales - West to the dealer and gardens.")!=std::string_view::npos &&
          std::string_view(inscription).find("Wheel - East to the mill and river fork.")!=std::string_view::npos &&
          std::string_view(inscription).find("Anvil - Southeast to the smith yard.")!=std::string_view::npos,
          "the local inscription names the four physical symbols and their authored destinations");
    Check(StepSquareWaymarkerTextVisible(true,false,false,true,false) &&
          StepSquareWaymarkerTextVisible(true,false,false,false,true) &&
          !StepSquareWaymarkerTextVisible(true,false,false,true,true) &&
          !StepSquareWaymarkerTextVisible(false,false,false,false,true) &&
          !StepSquareWaymarkerTextVisible(true,true,false,false,true) &&
          !StepSquareWaymarkerTextVisible(true,false,true,true,false),
          "reading toggles locally and clears on departure, pause or competing dialogue");
    horde::gameplay::simulation::SimulationSnapshot waymarkerPrompt;
    waymarkerPrompt.squareWaymarkerReadable=true;
    Check(waymarkerPrompt.HasChapterInteractPrompt(),
          "a readable fingerpost participates in the shared interaction prompt");
    // Current-rule reveal, without the historical checkpoint import which
    // deliberately selects legacy rules and cannot validate current speech.
    auto keeperConfig=simulation::ProductionGameSimulationConfig();
    keeperConfig.developmentRescueJourney=true;
    keeperConfig.playerStartX=-2.1f;keeperConfig.playerStartZ=kKeeperRetryPosition.z;
    simulation::GameSimulation keeperSpeech(keeperConfig);simulation::InputSnapshot keeperInput;
    keeperInput.yawRadians=-1.57079632679f;
    int senseFrames=0,closerFrames=0;
    for(int tick=0;tick<1800;++tick) {
        keeperInput.moveForward=keeperSpeech.Snapshot().playerX>kKeeperRetryPosition.x?1.f:0.f;
        keeperSpeech.StepFixed(keeperInput);
        const auto& speech=keeperSpeech.Snapshot().chapterDialogue;
        if(speech.line==Line::KeeperSense || speech.line==Line::KeeperCloser) {
            if(speech.line==Line::KeeperSense)++senseFrames;else ++closerFrames;
            const auto gains=CalculateSpatialAudio({speech.source.x,speech.source.z,.36f,1.f,14.f,speech.source.y},
                {speech.listener.x,speech.listener.z,speech.listenerYaw,speech.listener.y});
            Check(gains.left+gains.right>0.f,"published Keeper speech retains a nonzero current spatial mix");
        }
        keeperSpeech.ClearEvents();
    }
    if(!senseFrames || !closerFrames)std::cerr<<"Keeper speech witness sense="<<senseFrames<<" closer="<<closerFrames
        <<" reveal="<<keeperSpeech.Snapshot().lich.revealStarted<<" phase="<<static_cast<int>(keeperSpeech.Snapshot().lich.revealPhase)
        <<" torch="<<static_cast<int>(keeperSpeech.Snapshot().torchFailure.phase)<<" modern="<<keeperSpeech.Snapshot().modernCombatRules
        <<" player="<<keeperSpeech.Snapshot().playerX<<","<<keeperSpeech.Snapshot().playerZ<<'\n';
    Check(senseFrames>0 && closerFrames>0,"current reveal publishes both Keeper voice identities without importing legacy state");
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
        if(IsBellwetherLine(spec.id)) {
            Check(path.empty() && spec.text[0]!='\0' && RequiresChapterCaption(spec.id),
                "Bellwether retains timed captions when the local voice is unavailable");
            continue;
        }
        Check(!RequiresChapterCaption(spec.id), "existing voiced chapter lines respect the subtitle preference");
        Check(!path.empty() && std::filesystem::is_regular_file(path),
            "every admitted Kit and Keeper voice resolves to actual bytes from the Windows assets root");
        Check(path.lexically_relative(assetRoot).generic_string().starts_with("audio/"),
            "dialogue root is joined once rather than duplicating assets/assets");
    }
    const auto& privySpec=Spec(Line::PrivyRagRequest);
    Check(static_cast<std::uint32_t>(Line::PrivyRagRequest)==static_cast<std::uint32_t>(Line::RelicDealerOrdinarySale)+1u &&
          std::string_view(privySpec.stableId)=="hub.privy_request" &&
          std::string_view(privySpec.speaker)=="Privy occupant" &&
          std::string_view(privySpec.text)=="Can someone PLEASE get me some rag?!" &&
          privySpec.audio[0]=='\0' && std::abs(privySpec.duration-3.6f)<.000001f &&
          RequiresChapterCaption(Line::PrivyRagRequest) && IsBellwetherLine(Line::PrivyRagRequest),
          "the appended privy request is an exact timed local caption with a stable project identity");
    const auto& privyThanks=Spec(Line::PrivyRagThanks);
    Check(static_cast<std::uint32_t>(Line::PrivyRagThanks)==static_cast<std::uint32_t>(Line::PrivyRagRequest)+1u &&
          std::string_view(privyThanks.text)=="Thank you." && privyThanks.audio[0]=='\0' &&
          RequiresChapterCaption(Line::PrivyRagThanks),
          "the provisional local thanks stays caption-only and follows the need cue in the stable line table");
    for (const auto& cut : horde::audio::kBellwetherVoiceCuts) {
        const auto& spec = Spec(cut.line);
        Check(std::string_view(spec.speaker) == "Kit" &&
            std::abs(spec.duration - static_cast<float>(cut.frames) / 24000.f) < .000001f,
            "the shared clock covers the exact five approved Kit waveforms");
    }
    for (const auto& cut : horde::audio::kBellwetherArrivalAuditionCuts) {
        const auto& spec = Spec(cut.line);
        Check(!horde::audio::HasBellwetherVoiceCut(cut.line) &&
            horde::audio::HasBellwetherVoiceCut(cut.line, true) && RequiresChapterCaption(cut.line) &&
            std::abs(spec.duration - static_cast<float>(cut.frames) / 24000.f) < .000001f,
            "arrival audition uses the measured WAV and keeps the required timed caption");
    }
    constexpr std::array<std::uint32_t, 6> followOnLineIds{38, 39, 44, 45, 47, 55};
    for (std::size_t i = 0; i < followOnLineIds.size(); ++i) {
        const auto& cut = horde::audio::kBellwetherFollowOnAuditionCuts[i];
        const auto& spec = Spec(cut.line);
        Check(static_cast<std::uint32_t>(cut.line) == followOnLineIds[i] &&
            !horde::audio::HasBellwetherVoiceCut(cut.line, true, true) &&
            horde::audio::HasBellwetherVoiceCut(cut.line, false, false, true) &&
            RequiresChapterCaption(cut.line) && static_cast<float>(cut.frames) / 24000.f <= spec.duration,
            "the private follow-on cut fits the existing caption window and agrees with the Android line ID");
    }
    constexpr std::array<std::string_view, 2> hearthLineIds{"hub.kit_stake", "kit.personal_stake.key"};
    for (std::size_t i = 0; i < hearthLineIds.size(); ++i) {
        const auto& cut = horde::audio::kBellwetherHearthAuditionCuts[i];
        const auto& spec = Spec(cut.line);
        Check(std::string_view(spec.stableId) == hearthLineIds[i] &&
            std::string_view(spec.speaker) == "Kit" &&
            std::abs(spec.duration - static_cast<float>(cut.frames) / 24000.f) < .000001f &&
            !horde::audio::HasBellwetherVoiceCut(cut.line, true, true, true) &&
            horde::audio::HasBellwetherVoiceCut(cut.line, false, false, false, true) &&
            RequiresChapterCaption(cut.line),
            "the private Kit hearth pair is exact, independently admitted, and retains captions");
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
    for(unsigned charges:{0u,1u,2u}) {
        Director guided;Context gc;gc.claimed=true;
        gc.flaskGuideEnabled=true;gc.flaskCharges=charges;
        guided.Step(.016f,gc);guided.Skip();guided.Step(.016f,gc);guided.Skip();
        gc.exterior=true;gc.lanternLow=true;guided.Step(.016f,gc);
        gc.manualRaiseSequence=1;gc.visibleRaiseSequence=1;gc.lanternLow=false;gc.lanternHigh=true;
        guided.Step(.016f,gc);
        Check(guided.State().proofAccepted,"fresh reunion proof admits basin guidance");
        guided.Skip();guided.Step(.016f,gc);guided.Skip();guided.Step(.016f,gc);
        Check(guided.State().line==Line::ReunionDepart,"basin cue follows existing Kit story");
        const auto waiting=guided.ExportProgress();
        Check(waiting.continuationCount==1 && waiting.continuations[0]==
            (charges==0?Line::FlaskEmpty:Line::FlaskTopup),
            "one conditional basin cue survives a campaign save before delivery");
        auto duplicated=waiting;
        duplicated.continuations[1]=Line::FlaskTopup;
        duplicated.continuationCount=2;
        Check(!Director::ValidateProgress(duplicated),
            "corrupt progress cannot queue both conditional basin cues");
        Director restored;
        Check(restored.RestoreProgress(waiting,gc,100,1),"queued basin cue restores safely");
        restored.Skip();restored.Step(.016f,gc);
        Check(restored.State().line==Line::None && restored.BasinGuidePending(),
            "Kit waits for the player at the actual basin before giving the refill cue");
        gc.companionAtBasin=true;restored.Step(.016f,gc);
        Check(restored.State().line==Line::None,
            "Kit cannot call here while the player is still away from the basin");
        const auto far=simulation::kWorldRoutePoints[6];gc.player={far.x,far.y,far.z};
        restored.Step(.016f,gc);
        Check(restored.ExportProgress().continuationCount==1 && restored.BasinGuidePending(),
            "running ahead cannot queue forest lines after the pending basin lesson");
        gc.player={};
        gc.playerNearBasin=true;restored.Step(.016f,gc);
        Check(restored.State().line==(charges==0?Line::FlaskEmpty:Line::FlaskTopup),
            "restored Kit delivers the charge-appropriate guidance once");
        restored.Skip();restored.Step(.016f,gc);
        Check(restored.State().line==Line::None && restored.ExportProgress().continuationCount==0,
            "completed basin cue does not repeat on a nearby tick");
    }
    {
        Director changed;Context gc;gc.claimed=true;gc.flaskGuideEnabled=true;gc.flaskCharges=2;
        changed.Step(.016f,gc);changed.Skip();changed.Step(.016f,gc);changed.Skip();
        gc.exterior=true;gc.lanternLow=true;changed.Step(.016f,gc);
        gc.lanternLow=false;gc.lanternHigh=true;gc.manualRaiseSequence=1;gc.visibleRaiseSequence=1;
        changed.Step(.016f,gc);
        for(unsigned n=0;n<3;++n) {changed.Skip();changed.Step(.016f,gc);}
        Check(changed.BasinGuidePending() && changed.State().line==Line::None,
            "Kit keeps a pending basin lesson while walking");
        gc.flaskCharges=0;gc.companionAtBasin=gc.playerNearBasin=true;
        changed.Step(.016f,gc);
        Check(changed.State().line==Line::FlaskEmpty,
            "spoken basin line uses charges at delivery, not the old proof-time count");
    }
    CompanionRouteFixture basinGuide(true);
    basinGuide.Step(1.f/60.f,basinGuide.State().x,basinGuide.State().z,
        true,true,false,nullptr,false,true,true);
    const auto interruptedGuide=basinGuide.State();
    basinGuide.Step(1.f/60.f,interruptedGuide.x,interruptedGuide.z,
        true,true,false,nullptr,false,false,true);
    Check(basinGuide.State().basinGuide &&
        basinGuide.State().x==interruptedGuide.x && basinGuide.State().z==interruptedGuide.z,
        "temporary death or traversal interruption retains the unfinished basin guide");
    bool reachedBasin=false;
    for(unsigned tick=0;tick<1000;++tick) {
        const auto before=basinGuide.State();
        basinGuide.Step(1.f/60.f,before.x,before.z,true,true,false,nullptr,false,true,true);
        if(basinGuide.State().basinArrived) {reachedBasin=true;break;}
    }
    Check(reachedBasin && basinGuide.State().grounded &&
        std::hypot(basinGuide.State().x-(simulation::kWorldRoutePoints[2].x+1.45f),
                   basinGuide.State().z-(simulation::kWorldRoutePoints[2].z+.8f))<.08f,
        "Kit walks to the upper-rope basin without entering the reserved landing");
    const auto guideRoot=basinGuide.State();
    basinGuide.Step(1.f/60.f,guideRoot.x,guideRoot.z,true,true,false,nullptr,false,true,false);
    Check(!basinGuide.State().basinGuide && basinGuide.State().waypoint==3,
        "after the basin lesson Kit resumes the original mapped route");
    old=d.State();c.paused=true;d.Step(.05f,c);Check(d.State().elapsed==old.elapsed,"pause freezes fallback");
    d.Reset(true);Check(d.State().grateConsumed&&d.State().proofAccepted,"reconstruction preserves logical once flags");
    Check(!d.Complete(old.line,old.generation),"reset callback cannot finish a new generation");
    d.Reset();Check(!d.State().grateConsumed&&!d.State().proofAccepted,"new game resets once flags");
    c={};d.Request(Line::KeeperSense,{1,2,3},c);old=d.State();c.listener={8,9,10};d.Step(.05f,c);
    Check(d.State().listener.x==old.listener.x&&d.State().source.y==2,"source/listener tuple freezes at emission");
    // Continuous visible-companion speech is distinct from event-time calls.
    for(unsigned n=static_cast<unsigned>(Line::ReunionQuestion);n<=static_cast<unsigned>(Line::FlaskTopup);++n) {
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
    Check(WindowsSubtitleCaption("",Spec(Line::TavernDirections).text)==Spec(Line::TavernDirections).text,
          "a directions note has no speaker label");
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
    {
        Director road;
        Progress progress;
        progress.grateConsumed=progress.rescueConsumed=progress.reunionStarted=progress.proofAccepted=true;
        progress.forestNightConsumed=progress.forestWaystoneConsumed=
            progress.forestClueConsumed=progress.forestVillageConsumed=true;
        Context trail;
        trail.exterior=trail.claimed=trail.roadInReach=true;
        trail.companionSource=Tuple{59.3f,-8.45f,108.2f};
        Check(road.RestoreProgress(progress,trail,1,0),"completed lookout can start the roadside scene");
        road.Step(.05f,trail);
        Check(road.State().line==Line::None,"Kit cannot speak before the ankle catch");
        trail.kitCaughtAnkle=true;
        road.Step(.05f,trail);
        const auto stop=road.State();
        auto forged=road.ExportProgress();
        forged.activeLine=Line::RoadStop;
        Check(stop.line==Line::RoadStop && road.HoldingAnkleAssessment() &&
              Spec(stop.line).audio[0]=='\0' && road.ExportProgress().activeLine==Line::None &&
              !Director::ValidateProgress(forged),"the roadside beat is a transient positional caption");
        road.Step(.05f,trail);
        const auto beforePause=road.State();
        trail.paused=true;
        road.Step(.05f,trail);
        Check(road.State().elapsed==beforePause.elapsed,"pause freezes the roadside cadence");
        trail.paused=false;
        road.Skip();road.Step(.05f,trail);
        Check(road.State().line==Line::RoadAnkle && !road.Complete(stop.line,stop.generation),
              "skip advances once and rejects a stale completion");
        road.Skip();road.Step(.05f,trail);
        Check(road.State().line==Line::None && road.HoldingAnkleAssessment(),
              "rapid skips still leave time for the authored weight test");
        trail.kitReadyForProtectiveWalk=true;
        road.Step(.05f,trail);
        Check(road.State().line==Line::RoadSlow && !road.HoldingAnkleAssessment(),
              "the last line releases Kit into his protective walk");
        trail.kitCaughtAnkle=trail.kitReadyForProtectiveWalk=false;
        trail.companionSource->x+=.5f;
        road.Step(.05f,trail);
        Check(road.State().line==Line::RoadSlow && road.State().source.x==trail.companionSource->x,
              "the last caption follows Kit onto the road");

        Check(road.RestoreProgress(progress,trail,1,0),"an old lookout save starts before the incident");
        trail.kitCaughtAnkle=true;
        road.Step(.05f,trail);
        const auto departed=road.State();
        trail.roadInReach=false;
        road.Step(.05f,trail);
        trail.roadInReach=true;
        road.Step(.05f,trail);
        Check(departed.line==Line::RoadStop && road.State().line==Line::None &&
              !road.HoldingAnkleAssessment() && !road.Complete(departed.line,departed.generation),
              "leaving mid-line releases the route and doubling back never repeats the injury speech");
    }
    {
        Director door;
        Progress progress;
        progress.grateConsumed=progress.rescueConsumed=progress.reunionStarted=progress.proofAccepted=true;
        progress.forestNightConsumed=progress.forestWaystoneConsumed=
            progress.forestClueConsumed=progress.forestVillageConsumed=true;
        Context entrance;
        entrance.exterior=entrance.claimed=entrance.kitAtSouthDoor=true;
        entrance.tavernKeeperSource=Tuple{83.78f,-7.31f,118.2f};
        Check(door.RestoreProgress(progress,entrance,1,0),"lookout progress permits the later doorway cue");
        door.Step(.05f,entrance);
        Check(door.State().line==Line::None,"the keeper stays quiet when the player is out of earshot");
        entrance.tavernDoorInReach=true;entrance.paused=true;
        door.Step(.05f,entrance);
        Check(door.State().line==Line::None,"the doorway cue does not start while paused");
        entrance.paused=false;
        door.Step(.05f,entrance);
        const auto greeting=door.State();
        auto fabricated=door.ExportProgress();
        fabricated.activeLine=Line::TavernWelcome;
        Check(greeting.line==Line::TavernWelcome && greeting.source.x==83.78f &&
              door.ExportProgress().activeLine==Line::None && !Director::ValidateProgress(fabricated),
              "the doorway cue originates at the keeper and stays out of saves");
        entrance.tavernDoorInReach=false;
        door.Step(.05f,entrance);
        entrance.tavernDoorInReach=true;
        door.Step(.05f,entrance);
        Check(door.State().line==Line::None && !door.Complete(greeting.line,greeting.generation),
              "walking out of earshot clears playback and returning never repeats the greeting");
    }
    {
        Director hearth;
        Progress progress;
        progress.grateConsumed=progress.rescueConsumed=progress.reunionStarted=progress.proofAccepted=true;
        progress.forestNightConsumed=progress.forestWaystoneConsumed=
            progress.forestClueConsumed=progress.forestVillageConsumed=true;
        Context room;
        room.exterior=room.claimed=room.kitAtChair=true;
        room.companionSource=Tuple{82.46f,-7.31f,107.3f};
        Check(hearth.RestoreProgress(progress,room,1,0),"completed lookout prepares the hearth story");
        hearth.Step(.05f,room);
        Check(hearth.State().line==Line::None,"room dialogue cannot reach a player outside the hearth area");
        room.tavernInReach=true;
        hearth.Step(.05f,room);const auto sit=hearth.State();
        Check(sit.line==Line::TavernSit && sit.source.x==82.46f && Spec(sit.line).audio[0]=='\0' &&
              hearth.ExportProgress().activeLine==Line::None,
              "the chair starts the named caption from Kit without serializing its playback state");
        room.kitSeated=room.paused=true;
        hearth.Step(.05f,room);
        Check(hearth.State().line==Line::TavernSit && hearth.State().elapsed==sit.elapsed,"pause freezes the hearth line");
        room.paused=false;
        room.companionSource->y-=.27f;
        hearth.Step(.05f,room);
        Check(hearth.State().line==Line::TavernSit && hearth.State().source.y==room.companionSource->y &&
              hearth.Complete(sit.line,sit.generation),"the settling caption follows Kit down into the chair");
        hearth.Step(.05f,room);const auto limit=hearth.State();
        Check(limit.line==Line::TavernLimit && hearth.ExportProgress().hearthLinesResolved==1,
              "the seated line follows the sit without repeating it");
        room.tavernInReach=false;hearth.Step(.05f,room);
        Check(hearth.State().line==Line::None && !hearth.Complete(limit.line,limit.generation),
              "leaving cancels the incomplete caption and rejects its stale completion");
        room.tavernInReach=true;hearth.Step(.05f,room);
        Check(hearth.State().line==Line::TavernLimit,"return begins at the unresolved line");
        hearth.Skip();hearth.Step(.05f,room);
        Check(hearth.State().line==Line::None && hearth.ExportProgress().hearthLinesResolved==2,
              "the last line leaves a short pause after Kit explains his limits");
        for(int i=0;i<12;++i)hearth.Step(.05f,room);
        Check(hearth.State().line==Line::TavernWork,"Kit offers practical help after the pause");
        for(int i=0;i<110;++i)hearth.Step(.05f,room);
        Check(hearth.State().line==Line::None && hearth.ExportProgress().hearthLinesResolved==3,
              "natural subtitle completion resolves the last beat without a voice callback");
        hearth.Step(.05f,room);
        Check(hearth.State().line==Line::None,"the resolved hearth sequence remains quiet");
        Check(!hearth.TryStartTavernConversation(room),"a missing tavern keeper cannot offer the exchange");
        Check(!hearth.CanShowTavernDirections(room),"directions require the completed exchange");
        room.tavernKeeperSource=Tuple{85.5f,-7.31f,109.0f};
        room.keeperAtHearth=true;
        hearth.Step(.05f,room);
        Check(hearth.CanStartTavernConversation(room) && hearth.State().line==Line::None &&
              hearth.TryStartTavernConversation(room),"after settling, only deliberate interaction starts the exchange");
        Check(hearth.State().line==Line::TavernIntroduce && hearth.State().source.x==room.companionSource->x &&
              !hearth.TryStartTavernConversation(room),"the first line belongs to Kit and cannot restart on a second interact");
        hearth.Skip();hearth.Step(.05f,room);
        const auto keeper=hearth.State();
        Check(keeper.line==Line::TavernHordeArmy && keeper.source.x==room.tavernKeeperSource->x &&
              hearth.ExportProgress().activeLine==Line::None && hearth.ExportProgress().tavernConversationLinesResolved==1,
              "the keeper has a distinct source and the save carries only resolved lines");
        room.tavernInReach=false;hearth.Step(.05f,room);
        room.tavernInReach=true;hearth.Step(.05f,room);
        Check(hearth.State().line==Line::None && !hearth.Complete(keeper.line,keeper.generation) &&
              hearth.TryStartTavernConversation(room) && hearth.State().line==keeper.line,
              "leaving cancels playback and deliberate return resumes the unfinished reply");
        hearth.Skip();hearth.Step(.05f,room);
        Check(hearth.State().line==Line::TavernHoardAnswer && hearth.State().source.x==room.companionSource->x,
              "Kit owns the final response");
        hearth.Skip();hearth.Step(.05f,room);
        Check(!hearth.CanStartTavernConversation(room) && hearth.ExportProgress().tavernConversationLinesResolved==3,
              "the completed exchange cannot replay");
        Check(hearth.State().line==Line::None && hearth.CanShowTavernDirections(room),
              "directions are offered after the exchange without starting another line");
        hearth.ShowTavernDirections(room);
        const auto directions=hearth.State();
        const auto directionsProgress=hearth.ExportProgress();
        Check(IsGuidanceLine(directions.line) && directionsProgress.activeLine==Line::None &&
              directionsProgress.tavernConversationLinesResolved==kTavernConversation.size(),
              "directions are a temporary note rather than a fourth conversation line");
        room.tavernInReach=false;hearth.Step(.05f,room);
        Check(hearth.State().line==Line::None && !hearth.CanShowTavernDirections(room) &&
              !hearth.Complete(directions.line,directions.generation),
              "moving away dismisses the note and invalidates its generation");
        room.tavernInReach=true;hearth.Step(.05f,room);
        Check(hearth.State().line==Line::None && hearth.CanShowTavernDirections(room),
              "returning allows rereading without automatically replaying anything");
    }
    {
        Director shrine;
        Context site;
        site.claimed=site.exterior=site.kitSeated=true;
        Progress arrived;
        arrived.grateConsumed=arrived.rescueConsumed=true;
        Check(shrine.RestoreProgress(arrived,site,1,0),"the fixture has passed the rescue scene");
        Check(!shrine.TryStartShrineSpeech(ShrineSpeech::Help,site),"shrine speech requires the held lantern in reach");
        site.shrineLanternSource=Tuple{103.f,-2.7f,33.f};
        site.companionSource=Tuple{85.f,-8.f,108.f};
        Check(shrine.TryStartShrineSpeech(ShrineSpeech::Reveal,site) &&
              shrine.State().line==Line::LanternFirstIdentity && shrine.State().source.x==103.f &&
              WindowsSubtitleCaption(Spec(shrine.State().line).speaker,Spec(shrine.State().line).text)
                  .starts_with("Lantern voice:"),
              "the reveal uses the held lantern's source and labels the speaker without endorsing the claim");
        const auto identity=shrine.State();
        site.paused=true;shrine.Step(.05f,site);
        Check(shrine.State().generation==identity.generation && shrine.State().elapsed==identity.elapsed,
              "pausing keeps the shrine line and its place");
        site.paused=false;site.shrineLanternSource.reset();shrine.Step(.05f,site);
        Check(shrine.State().line==Line::None && !shrine.Complete(identity.line,identity.generation),
              "leaving the shrine cancels its queued story and invalidates stale callbacks");
        site.shrineLanternSource=Tuple{103.f,-2.7f,33.f};shrine.Step(.05f,site);
        Check(shrine.State().line==Line::None && shrine.TryStartShrineSpeech(ShrineSpeech::Recap,site) &&
              shrine.State().line==Line::LanternFirstRequest,
              "a return stays quiet until a deliberate practical recap");
    }
    {
        Director records;
        Context church;
        church.claimed=church.exterior=church.kitSeated=true;
        church.recordKeeperInReach=true;
        Progress arrival;
        arrival.grateConsumed=arrival.rescueConsumed=arrival.reunionStarted=arrival.proofAccepted=true;
        arrival.forestNightConsumed=arrival.forestWaystoneConsumed=arrival.forestClueConsumed=arrival.forestVillageConsumed=true;
        arrival.hearthLinesResolved=kHearthLines.size();
        Check(records.RestoreProgress(arrival,church,1,0),"record keeper fixture restores completed hearth progress");
        Check(!records.TryStartRecordKeeper(RecordKeeperAction::ShowRegister,church),"an absent keeper cannot read the register");
        church.recordKeeperSource=Tuple{71.3f,-1.3f,75.2f};
        Check(records.TryStartRecordKeeper(RecordKeeperAction::ShowRegister,church) &&
              records.State().line==Line::RecordKeeperRegister &&
              records.State().source.x==church.recordKeeperSource->x &&
              RequiresChapterCaption(records.State().line),
              "the reading starts at the physical keeper and displays an essential caption");
        const auto before=records.State();
        church.recordKeeperInReach=false;
        records.Skip();records.Step(.05f,church);
        Check(records.State().line==Line::RecordKeeperTwoRoads,
              "turning away ends interaction reach while the audible reading continues");
        church.paused=true;records.Step(.05f,church);
        Check(records.State().elapsed==0.f,"pausing holds the next record keeper caption");
        church.paused=false;church.recordKeeperSource.reset();records.Step(.05f,church);
        Check(records.State().line==Line::None && records.ExportProgress().activeLine==Line::None &&
              !records.Complete(before.line,before.generation),
              "leaving earshot cancels the remaining reading and its stale callbacks");
    }
    {
        Director tavern;
        Context room;
        room.claimed=room.exterior=room.kitSeated=room.tavernInReach=true;
        room.companionSource=Tuple{2,1,0};
        Progress arrival;
        arrival.grateConsumed=arrival.rescueConsumed=arrival.reunionStarted=arrival.proofAccepted=true;
        arrival.forestNightConsumed=arrival.forestWaystoneConsumed=arrival.forestClueConsumed=arrival.forestVillageConsumed=true;
        arrival.hearthLinesResolved=kHearthLines.size();
        Check(tavern.RestoreProgress(arrival,room,1,0),"the seated scene restores without Kit's new evidence");
        room.lanternHigh=true;room.manualRaiseSequence=room.visibleRaiseSequence=1;
        tavern.Step(.02f,room);
        room.tavernLanternSource=Tuple{.25f,1.4f,0};
        tavern.Step(.02f,room);
        Check(tavern.State().line==Line::None,"a lantern already held high cannot introduce itself on return");
        room.lanternHigh=false;room.lanternLow=true;room.manualRaiseSequence=2;
        tavern.Step(.02f,room);
        room.lanternHigh=true;room.lanternLow=false;room.manualRaiseSequence=3;
        tavern.Step(.02f,room);
        Check(tavern.State().line==Line::None,"the new raise must reach a presented frame");
        room.visibleRaiseSequence=3;tavern.Step(.02f,room);
        Check(tavern.State().line==Line::TavernLanternIdentity && tavern.State().source.x==.25f &&
              RequiresChapterCaption(tavern.State().line),"the claim comes from the raised lantern in hearing range");
        const auto identity=tavern.State();
        room.tavernInReach=false;tavern.Step(.02f,room);
        Check(!tavern.Complete(identity.line,identity.generation) && tavern.State().line==Line::None &&
              tavern.ExportProgress().tavernLanternLinesStarted==1,
              "leaving after the claim retains just the witnessed utterance");
        Director restored;
        room.tavernInReach=true;
        Check(restored.RestoreProgress(tavern.ExportProgress(),room,20,3),"the first utterance survives reload");
        restored.Step(.02f,room);
        Check(restored.State().line==Line::None,"loading a raised lantern never resumes the exchange");
        room.lanternHigh=false;room.lanternLow=true;room.manualRaiseSequence=4;
        restored.Step(.02f,room);
        room.lanternHigh=true;room.lanternLow=false;room.manualRaiseSequence=room.visibleRaiseSequence=5;
        restored.Step(.02f,room);
        for(std::size_t i=1;i<kTavernLanternExchange.size();++i) {
            const auto line=kTavernLanternExchange[i];
            Check(restored.State().line==line && restored.State().source.x==(i==1?.25f:2.f) &&
                  RequiresChapterCaption(line),"the seals and Kit's replies use their own sources in order");
            restored.Skip();restored.Step(.02f,room);
        }
        Check(restored.State().line==Line::None && !restored.CanOfferTavernLantern(room) &&
              restored.ExportProgress().tavernLanternLinesStarted==kTavernLanternExchange.size(),
              "the witnessed exchange completes once without automatic repetition");
    }
    {
        Progress arrived;
        arrived.grateConsumed=arrived.rescueConsumed=arrived.reunionStarted=arrived.proofAccepted=true;
        arrived.forestNightConsumed=arrived.forestWaystoneConsumed=arrived.forestClueConsumed=arrived.forestVillageConsumed=true;
        arrived.hearthLinesResolved=kHearthLines.size();
        arrived.tavernConversationLinesResolved=kTavernConversation.size();
        Context local;
        local.claimed=local.exterior=local.kitSeated=local.tavernInReach=local.privyInReach=true;
        local.companionSource=Tuple{2.f,1.f,0.f};
        local.privySource=Tuple{76.65f,-9.37f,112.f};
        local.listener=Tuple{75.5f,-8.f,112.f};

        auto grate=local;
        grate.privySource.reset();
        grate.gardenGrateInReach=true;
        grate.gardenGrateSource=Tuple{39.31f,-10.96f,130.18f};
        Director visitor;
        Check(visitor.RestoreProgress(arrived,grate,1,0) &&
              RequiresChapterCaption(Line::GardenGrateTalk) && visitor.CanStartGardenGrateTalk(grate),
              "the completed tavern opens the direct garden conversation");
        visitor.Step(.02f,grate);
        Check(visitor.State().line==Line::None,"standing near the grate does not start the optional line");
        grate.gardenGrateInReach=false;
        Check(!visitor.TryStartGardenGrateTalk(grate),"the direct conversation requires facing the grate in range");
        grate.gardenGrateInReach=true;
        Check(visitor.TryStartGardenGrateTalk(grate) &&
              visitor.State().source.x==grate.gardenGrateSource->x &&
              visitor.ExportProgress().activeLine==Line::None &&
              !visitor.TryStartGardenGrateTalk(grate),
              "a direct line uses its authored source, stays out of saves and cannot stack");
        const auto elapsed=visitor.State().elapsed;
        grate.paused=true;
        visitor.Step(.04f,grate);
        Check(visitor.State().line==Line::GardenGrateTalk && visitor.State().elapsed==elapsed,
              "pause holds the conversation in place");
        grate.paused=false;
        grate.gardenGrateSource.reset();
        visitor.Step(.02f,grate);
        Check(visitor.State().line==Line::None,"leaving earshot ends the local caption");
        Director beforeExchange;
        Progress unfinished=arrived;unfinished.tavernConversationLinesResolved=0;
        Check(beforeExchange.RestoreProgress(unfinished,local,1,0) &&
              !beforeExchange.CanStartPrivyRagRequest(local),
              "the local request is ineligible until the first regular tavern conversation resolves");
        for(const auto disqualifier:{0,1,2,3,4,5}) {
            Director gated;Check(gated.RestoreProgress(arrived,local,1,0),"privy gate fixture restores");
            auto context=local;
            switch(disqualifier) {
            case 0: context.claimed=false;break;
            case 1: context.alive=false;break;
            case 2: context.exterior=false;break;
            case 3: context.kitSeated=false;break;
            case 4: context.combat=true;break;
            default: context.traversing=true;break;
            }
            Check(!gated.CanStartPrivyRagRequest(context),
                  "the local request requires the live claimed seated exterior and traversal-safe session");
        }

        Director queued;
        Progress ordered=arrived;
        ordered.activeLine=Line::ReunionProof;
        ordered.continuations[0]=Line::ReunionFirstPiece;
        ordered.continuations[1]=Line::ReunionDepart;
        ordered.continuationCount=2;
        Check(queued.RestoreProgress(ordered,local,1,0) && !queued.CanStartPrivyRagRequest(local),
              "an active proof and its ordered response queue keep the optional local call silent");
        queued.Skip();queued.Step(.01f,local);
        Check(queued.State().line==Line::ReunionFirstPiece,"proof retains its first required response");
        queued.Skip();queued.Step(.01f,local);
        Check(queued.State().line==Line::ReunionDepart,"proof retains its departure response");
        queued.Skip();queued.Step(.01f,local);
        Check(queued.State().line==Line::PrivyRagRequest && queued.State().source.x==local.privySource->x,
              "the fixed privy source starts only after all current and queued speech has ended");
        const auto request=queued.State();
        Check(queued.ExportProgress().activeLine==Line::None && queued.ExportProgress().continuationCount==0,
              "the optional caption is excluded from campaign dialogue progress");
        local.paused=true;queued.Step(.05f,local);
        Check(queued.State().line==request.line && queued.State().generation==request.generation,
              "pause freezes the local request without consuming or rearming it");
        local.paused=false;
        auto lookingAway=local;lookingAway.privyInReach=false;
        queued.Step(.05f,lookingAway);
        Check(queued.State().line==request.line && queued.State().elapsed>.04f,
              "looking away within the same clear front pocket does not cancel the active local caption");
        local.traversing=true;queued.Step(.01f,local);
        Check(queued.State().line==Line::None && !queued.Complete(request.line,request.generation),
              "traversal cancels the local request and rejects its stale completion");
        local.traversing=false;queued.Step(.01f,local);
        Check(queued.State().line==Line::None,"returning to range does not repeat the request within one session");
        queued.Reset(true);queued.Step(.01f,local);
        Check(queued.State().line==Line::None,"ordinary preserving refresh does not rearm the local request");

        const auto saved=queued.ExportProgress();
        Director newSession;
        Check(newSession.RestoreProgress(saved,local,100,0),"fresh loaded session restores ordinary story progress");
        newSession.Step(.01f,local);
        Check(newSession.State().line==Line::PrivyRagRequest,
              "the optional local call is session-scoped and does not add a campaign save field");
        for(int tick=0;tick<80;++tick)newSession.Step(.05f,local);
        Check(newSession.State().line==Line::None,"the local request finishes on its measured caption clock");
        newSession.Step(.01f,local);newSession.Reset(true);newSession.Step(.01f,local);
        Check(newSession.State().line==Line::None,
              "completion and a preserving refresh keep the request consumed for the loaded session");

        Director thanks;
        Check(thanks.RestoreProgress(arrived,local,1,0),"privy thanks fixture restores");
        thanks.Step(.01f,local);const auto requestCue=thanks.State();
        Check(requestCue.line==Line::PrivyRagRequest && thanks.TryStartPrivyRagThanks(local,*local.privySource) &&
              thanks.State().line==Line::PrivyRagThanks && !thanks.Complete(requestCue.line,requestCue.generation),
              "delivery replaces only the active need cue with the local thanks and invalidates its callback");
        auto nearShelf=local;nearShelf.privyInReach=false;
        thanks.Step(.05f,nearShelf);
        Check(thanks.State().line==Line::PrivyRagThanks && thanks.State().source.x==local.privySource->x &&
              thanks.State().elapsed>.04f,
              "the local feedback stays bound and advances when the player looks away in the same clear pocket");
        auto leftPocket=nearShelf;leftPocket.privySource.reset();thanks.Step(.01f,leftPocket);
        Check(thanks.State().line==Line::None,"leaving the clear privy pocket cancels the provisional thanks");
        thanks.Step(.01f,local);
        Check(thanks.State().line==Line::None,"a delivered need cue stays quiet when the player returns");

        Director unrelated;
        Check(unrelated.RestoreProgress(arrived,local,1,0),"unrelated speech fixture restores");
        unrelated.ShowTavernDirections(local);
        const auto directions=unrelated.State();
        Check(!unrelated.CanInteractPrivyRag(local) &&
              !unrelated.TryStartPrivyRagThanks(local,*local.privySource) &&
              unrelated.State().line==directions.line && unrelated.State().generation==directions.generation,
              "the provisional response never interrupts unrelated dialogue");

        for(int interruption=0;interruption<2;++interruption) {
            Director canceled;Check(canceled.RestoreProgress(arrived,local,1,0),"privy interruption fixture restores");
            canceled.Step(.01f,local);const auto active=canceled.State();
            auto away=local;
            if(interruption==0)away.combat=true;else away.alive=false;
            canceled.Step(.01f,away);
            Check(canceled.State().line==Line::None && !canceled.Complete(active.line,active.generation),
                  "combat or death cancels the local request and invalidates its callback");
            canceled.Step(.01f,local);
            Check(canceled.State().line==Line::None,"an interrupted local request does not repeat this session");
        }
    }
    {
        Check(static_cast<std::uint32_t>(Line::KitTreasureRoof)==59u &&
              static_cast<std::uint32_t>(Line::KitTreasureKey)==60u &&
              std::string_view(Spec(Line::KitTreasureRoof).stableId)=="hub.kit_stake" &&
              std::string_view(Spec(Line::KitTreasureRoof).text)==
                  "I'd like a roof that doesn't fall in. Treasure still seems a fair way to get one." &&
              std::string_view(Spec(Line::KitTreasureKey).stableId)=="kit.personal_stake.key" &&
              std::string_view(Spec(Line::KitTreasureKey).text)==
                  "A door with my own key. I’ve had worse reasons for going digging." &&
              std::string_view(Spec(Line::KitTreasureRoof).speaker)=="Kit" &&
              std::string_view(Spec(Line::KitTreasureKey).speaker)=="Kit" &&
              std::abs(Spec(Line::KitTreasureRoof).duration-136784.f/24000.f)<.000001f &&
              std::abs(Spec(Line::KitTreasureKey).duration-113744.f/24000.f)<.000001f &&
              RequiresChapterCaption(Line::KitTreasureRoof) && RequiresChapterCaption(Line::KitTreasureKey) &&
              IsBellwetherLine(Line::KitTreasureRoof) && IsBellwetherLine(Line::KitTreasureKey),
              "the optional Kit topic appends exact stable authored lines with measured caption clocks");

        Progress settled;
        settled.grateConsumed=settled.rescueConsumed=settled.reunionStarted=settled.proofAccepted=true;
        settled.forestNightConsumed=settled.forestWaystoneConsumed=settled.forestClueConsumed=
            settled.forestVillageConsumed=true;
        settled.hearthLinesResolved=kHearthLines.size();
        settled.tavernConversationLinesResolved=kTavernConversation.size();
        Context room;
        room.claimed=room.exterior=room.kitSeated=room.tavernInReach=room.kitTreasureTopicInReach=true;
        room.companionSource=Tuple{12.4f,1.4f,-3.8f};
        room.listener=Tuple{11.3f,1.7f,-3.1f};
        Director topic;
        Check(topic.RestoreProgress(settled,room,1,0) && topic.CanAskKitAboutTreasure(room),
              "the optional question is available after Kit settles and the initial tavern exchange completes");
        for(int condition=0;condition<7;++condition) {
            Director gated;Check(gated.RestoreProgress(settled,room,1,0),"Kit topic gate fixture restores");
            auto context=room;
            switch(condition) {
            case 0: context.claimed=false;break;
            case 1: context.alive=false;break;
            case 2: context.exterior=false;break;
            case 3: context.kitSeated=false;break;
            case 4: context.combat=true;break;
            case 5: context.traversing=true;break;
            default: context.kitTreasureTopicInReach=false;break;
            }
            Check(!gated.CanAskKitAboutTreasure(context),
                  "asking requires the settled live Kit and explicit close-facing interaction affordance");
        }
        Progress beforeExchange=settled;beforeExchange.tavernConversationLinesResolved--;
        Director notYet;Check(notYet.RestoreProgress(beforeExchange,room,1,0) &&
              !notYet.CanAskKitAboutTreasure(room),
              "the optional Kit topic stays unavailable until all initial tavern lines resolve");
        Director busy;Check(busy.RestoreProgress(settled,room,1,0),"busy Kit topic fixture restores");
        busy.ShowTavernDirections(room);
        const auto unrelated=busy.State();
        Check(!busy.CanAskKitAboutTreasure(room) && !busy.TryStartKitTreasureTalk(room) &&
              busy.State().line==unrelated.line && busy.State().generation==unrelated.generation,
              "the optional Kit topic cannot interrupt other chapter dialogue");

        Check(topic.TryStartKitTreasureTalk(room) && topic.State().line==Line::KitTreasureRoof &&
              topic.State().source.x==room.companionSource->x &&
              !topic.TryStartKitTreasureTalk(room),
              "an explicit ask starts the first Kit line at Kit's companion source and cannot stack");
        const auto first=topic.State();
        const auto initialProgress=topic.ExportProgress();
        Check(initialProgress.activeLine==Line::None &&
              initialProgress.hearthLinesResolved==settled.hearthLinesResolved &&
              initialProgress.tavernConversationLinesResolved==settled.tavernConversationLinesResolved,
              "the repeatable local conversation adds no saved flag or story tracker progress");
        room.paused=true;topic.Step(.05f,room);
        Check(topic.State().line==first.line && topic.State().elapsed==first.elapsed,
              "pause freezes Kit's optional line");
        room.paused=false;
        for(int tick=0;tick<120 && topic.State().line==Line::KitTreasureRoof;++tick)topic.Step(.05f,room);
        Check(topic.State().line==Line::KitTreasureKey && topic.State().source.x==room.companionSource->x,
              "the first caption's measured clock advances within the same explicit conversation to Kit's key line");
        topic.Skip();
        Check(topic.State().line==Line::None && !topic.Complete(first.line,first.generation),
              "the ordinary interaction skip ends the current optional line and invalidates its callback");
        topic.Step(.01f,room);
        Check(topic.State().line==Line::None && topic.CanAskKitAboutTreasure(room) &&
              topic.ExportProgress().tavernConversationLinesResolved==settled.tavernConversationLinesResolved,
              "a completed question stays quiet and can be asked again without advancing story progress");

        Check(topic.TryStartKitTreasureTalk(room),"a completed optional conversation can be explicitly repeated");
        const auto retry=topic.State();
        auto away=room;away.tavernInReach=false;away.kitTreasureTopicInReach=false;
        topic.Step(.01f,away);
        Check(topic.State().line==Line::None && !topic.Complete(retry.line,retry.generation),
              "walking out of Kit's hearing range cancels the pending conversation and rejects stale completion");
        topic.Step(.01f,room);
        Check(topic.State().line==Line::None && topic.CanAskKitAboutTreasure(room),
              "returning to the tavern does not auto-start the canceled optional conversation");
        Director suspended;
        Check(suspended.RestoreProgress(settled,room,1,0) && suspended.TryStartKitTreasureTalk(room),
              "lifecycle cancellation fixture starts the optional Kit conversation");
        const auto beforeSuspend=suspended.State();
        suspended.Suspend();suspended.Step(.01f,room);
        Check(suspended.State().line==Line::None && !suspended.Complete(beforeSuspend.line,beforeSuspend.generation) &&
              suspended.CanAskKitAboutTreasure(room),
              "leaving or quitting cancels the pending conversation and requires a fresh explicit ask");
    }
    std::cout<<"chapter dialogue: local/ordered/once/fallback/stale/manual-presented/real-simulation/layout cases passed\n";
}
