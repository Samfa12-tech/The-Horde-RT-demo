#include "gameplay/simulation/GameSimulation.h"
#include "audio/KitAnkleAudio.h"
#include <cmath>
namespace horde::gameplay::simulation {
void GameSimulation::UpdateChapterDialogue(const InputSnapshot& input,float seconds) {
    // Historical replay/capture fixtures keep their old timing and semantics.
    if(!config_.combatFoundation1_7 || legacyCombatCheckpoint_) return;
    using namespace horde::gameplay::dialogue;
    Context c;
    c.player={playerX_,playerSupport_.worldY,playerZ_};
    c.listener={playerX_,kShowcaseEyeWorldY+PlayerHeightDelta(playerSupport_.worldY)-posture_.drop,playerZ_};
    c.yaw=playerYawRadians_;c.paused=input.paused;
    c.alive=playerVitals_.Snapshot().phase==PlayerLifePhase::Alive;
    c.combat=combatSnapshot_.attackerIndex>=0 || lichEncounter_.Snapshot().phase==LichPhase::Charging;
    c.claimed=chestRewardSequence_.Snapshot().phase==interactions::ChestRewardPhase::LanternClaimed;
    c.exterior=config_.developmentRescueJourney&&rescueTraversal_.Snapshot().exteriorSide;
    c.traversing=config_.developmentRescueJourney&&rescueTraversal_.IsActive();
    if(c.exterior&&!c.traversing&&!chapterFirstExterior_) {
        chapterFirstExterior_=true;
        interactionState_.heldLightPose=interactions::HeldLightPose::Low;
        interactionState_.heldLightPoseProgress=0;
        chapterVisibleRaise_=0;
        ResolveHeldItems();
    }
    c.lanternLow=interactionState_.heldLightPose==interactions::HeldLightPose::Low;
    c.lanternHigh=interactionState_.heldLightPose==interactions::HeldLightPose::High;
    c.flaskGuideEnabled=config_.healingFlask1_7;
    c.flaskCharges=healingFlask_.Snapshot().resources.charges;
    c.manualRaiseSequence=lastConsumedToggleHeldLightPoseSequence_;
    // Platform feedback can only match a consumed manual edge. The internal
    // acknowledgement is the preferred path and never trusts a stale sequence.
    c.visibleRaiseSequence=chapterVisibleRaise_;
    if(!input.paused && input.dialogueCompletionGeneration)
        chapterDialogue_.Complete(static_cast<Line>(input.dialogueCompletionLine),input.dialogueCompletionGeneration);
    if(input.commands.dialogueSkip>chapterSkipFloor_) {
        chapterSkipFloor_=input.commands.dialogueSkip;
        if(!input.paused) chapterDialogue_.Skip();
    }
    const bool proofWasAccepted=chapterDialogue_.State().proofAccepted;
    const auto bindCompanion=[&] {
        const auto& kit=chapterCompanion_.State();
        c.kitAtChair=kit.grounded && kit.bellwetherArrival>=BellwetherArrivalStage::ChairTurn;
        c.kitSeated=c.kitAtChair && kit.bellwetherArrival==BellwetherArrivalStage::Seated;
        const auto hearthMark=campaign::kBellwetherCheckpointPosition;
        c.tavernInReach=c.kitAtChair && playerSupport_.grounded &&
            std::abs(playerSupport_.worldY-hearthMark.y)<.001f &&
            std::hypot(playerX_-hearthMark.x,playerZ_-hearthMark.z)<=1.75f;
        c.companionSource.reset();
        if(c.exterior && kit.grounded) {
            const float lowering=kit.motion.sitPlaybackSeconds/kTavernSitDurationSeconds;
            c.companionSource=Tuple{kit.x,kit.y+kCompanionSpeechHeight-.27f*lowering,kit.z};
        }
        const auto basin=kWorldRoutePoints[2];
        c.companionAtBasin=kit.basinGuide && kit.basinArrived && kit.grounded;
        c.playerNearBasin=c.exterior &&
            std::hypot(playerX_-basin.x,playerZ_-basin.z)<=3.f;
    };
    bindCompanion();
    chapterDialogue_.Step(seconds,c);
    const bool guidePending=config_.healingFlask1_7 && chapterDialogue_.BasinGuidePending();
    const bool readyToLead=chapterDialogue_.State().line!=Line::ReunionProof &&
        chapterDialogue_.State().line!=Line::ReunionFirstPiece;
    const auto* terrain=chapterTerrainGeneration_==worldRoute_.generation?chapterTerrain_.get():nullptr;
    if(c.exterior && c.alive && !c.traversing && !c.paused && terrain &&
       campaignBoundary_ && campaignBoundary_->id==campaign::CheckpointId::ForestEndpoint &&
       chapterDialogue_.State().line==Line::None && playerSupport_.grounded)
        chapterCompanion_.TryStartBellwetherArrival(playerX_,playerZ_,*terrain);
    const float previousAnkleTime=chapterCompanion_.State().motion.anklePlaybackSeconds;
    chapterCompanion_.Step(seconds,playerX_,playerZ_,c.exterior,
        chapterDialogue_.State().proofAccepted && (!guidePending || readyToLead),c.paused,
        terrain,
        config_.authoredChapterTerrain,c.alive&&!c.traversing,
        guidePending && readyToLead);
    const auto& kit=chapterCompanion_.State();
    if(kit.grounded && kit.bellwetherArrival==BellwetherArrivalStage::AnkleAssessment &&
       kit.motion.anklePlaybackSeconds>previousAnkleTime) {
        const float headingX=std::sin(kit.yaw), headingZ=std::cos(kit.yaw);
        for(const auto& beat:audio::kKitAnkleBeats) {
            if(previousAnkleTime<beat.seconds && kit.motion.anklePlaybackSeconds>=beat.seconds)
                Emit(GameplayEventType::KitAnkleFoley,EntityId::Kit,EntityId::Invalid,
                    kit.x+headingX*beat.forward+headingZ*beat.right,
                    kit.z-headingZ*beat.forward+headingX*beat.right,beat.gain,
                    static_cast<std::int32_t>(beat.cue),kit.y+beat.height);
        }
    }
    bindCompanion();
    chapterDialogue_.RefreshCompanionSpatial(c);
    chapterCompanion_.AdvanceActing(seconds,chapterDialogue_.State(),c.exterior,c.alive,
        !proofWasAccepted && chapterDialogue_.State().proofAccepted);
}
bool GameSimulation::AcknowledgeChapterPresentation(std::uint64_t tick,std::uint64_t generation) {
    if(tick!=snapshot_.tickIndex||generation!=chapterDialogue_.State().generation||
        snapshot_.paused||!snapshot_.rescue.exteriorSide||snapshot_.rescue.equipmentStowed||
        snapshot_.interaction.heldLightPose!=interactions::HeldLightPose::High) return false;
    chapterVisibleRaise_=snapshot_.lastConsumedToggleHeldLightPoseSequence;
    return true;
}
} // namespace horde::gameplay::simulation
