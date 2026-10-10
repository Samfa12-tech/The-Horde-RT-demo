#include "gameplay/simulation/GameSimulation.h"
#include <cmath>
namespace horde::gameplay::simulation {
void GameSimulation::UpdateChapterDialogue(const InputSnapshot& input,float seconds) {
    // Historical replay/capture fixtures keep their old timing and semantics.
    if(!config_.combatFoundation1_7 || legacyCombatCheckpoint_) return;
    using namespace horde::gameplay::dialogue;
    Context c;
    c.player={playerX_,playerSupport_.worldY,playerZ_};
    c.listener={playerX_,kShowcaseEyeWorldY+PlayerHeightDelta(playerSupport_.worldY),playerZ_};
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
    chapterDialogue_.Step(seconds,c);
    chapterCompanion_.Step(seconds,playerX_,playerZ_,c.exterior,
        chapterDialogue_.State().proofAccepted,c.paused);
}
bool GameSimulation::AcknowledgeChapterPresentation(std::uint64_t tick,std::uint64_t generation) {
    if(tick!=snapshot_.tickIndex||generation!=chapterDialogue_.State().generation||
        snapshot_.paused||!snapshot_.rescue.exteriorSide||snapshot_.rescue.equipmentStowed||
        snapshot_.interaction.heldLightPose!=interactions::HeldLightPose::High) return false;
    chapterVisibleRaise_=snapshot_.lastConsumedToggleHeldLightPoseSequence;
    return true;
}
} // namespace horde::gameplay::simulation
