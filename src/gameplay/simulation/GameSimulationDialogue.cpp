#include "gameplay/simulation/GameSimulation.h"
#include "audio/KitAnkleAudio.h"
#include <cmath>
namespace horde::gameplay::simulation {
void GameSimulation::InteractWithClaimedChapter() {
    if(TryRescueInteraction() || TryFlaskUpgradeInteraction())return;
    const auto context=ChapterInteractionContext(false);
    if(config_.bellwetherTavernKeeperAdmitted && chapterDialogue_.TryStartTavernConversation(context))return;
    switch(WaysideArchiveAction(context)) {
    case bellwether::ArchiveAction::Inspect:
        if(chapterDialogue_.TryStartShrineSpeech(dialogue::ShrineSpeech::Help,context))
            waysideArchive_.stage=bellwether::ArchiveStage::Inspected;
        return;
    case bellwether::ArchiveAction::Open:
        waysideArchive_.stage=bellwether::ArchiveStage::Opening;
        return;
    case bellwether::ArchiveAction::Listen:
        TryStartShrineReveal(context);
        return;
    case bellwether::ArchiveAction::TakeRegister:
        waysideArchive_.registerRecovered=true;
        chapterDialogue_.TryStartShrineSpeech(dialogue::ShrineSpeech::RegisterReturn,context);
        return;
    case bellwether::ArchiveAction::Recap:
        chapterDialogue_.TryStartShrineSpeech(dialogue::ShrineSpeech::Recap,context);
        return;
    default:
        chapterDialogue_.ShowTavernDirections(context);
    }
}

bellwether::ArchiveAction GameSimulation::WaysideArchiveAction(const dialogue::Context& c) const {
    using namespace bellwether;
    const auto stage=waysideArchive_.stage;
    if(!c.shrineLanternSource || c.paused || chapterDialogue_.State().line!=dialogue::Line::None ||
       stage==ArchiveStage::Opening)
        return ArchiveAction::None;
    const float dx=kWaysideMechanism[0]-c.player.x,dz=kWaysideMechanism[2]-c.player.z;
    const float distance=std::hypot(dx,dz);
    // Leave room for the entire outward sweep of the lower stone.
    if(-dz<1.60f || distance>2.7f || std::abs(dx)>.8f ||
       (std::sin(c.yaw)*dx-std::cos(c.yaw)*dz)<.90f*distance ||
       std::abs(c.listener.y-kWaysideMechanism[1])>.9f) return ArchiveAction::None;
    const float y=kWaysideMechanism[1];
    if(!chapterTerrain_->NonWalkableVolumeClear(c.player.x,c.player.z,kWaysideMechanism[0],
            kWaysideMechanism[2]+.08f,y-.06f,y+.06f,.025f))return ArchiveAction::None;
    if(stage==ArchiveStage::Closed)return ArchiveAction::Inspect;
    if(stage==ArchiveStage::Inspected)return ArchiveAction::Open;
    if(!waysideArchive_.lanternRevealStarted)return ArchiveAction::Listen;
    if(waysideArchive_.registerRecovered)return ArchiveAction::Recap;
    const auto& mark=kWaysideRoadRegister;
    if(!chapterTerrain_->NonWalkableVolumeClear(c.player.x,c.player.z,mark[0],mark[2],
            mark[1],mark[1]+.025f,.025f) ||
       !ArchiveContactClear(c.player.x,c.player.z,mark[0],mark[2],mark[1],mark[1]+.025f,.025f))
        return ArchiveAction::None;
    return ArchiveAction::TakeRegister;
}

void GameSimulation::AdvanceWaysideArchive(float seconds) {
    using namespace bellwether;
    if(waysideArchive_.stage!=ArchiveStage::Opening || seconds<=0 || !chapterTerrain_ ||
       chapterTerrainGeneration_!=worldRoute_.generation || !chapterTerrain_->HingedContact())return;
    const float progress=std::min(1.f,waysideArchive_.opening+seconds);
    const auto candidate=ArchivePose::AtOpening(progress);
    // Expand the body query by the greatest point travel between poses.
    const float swingMargin=(progress-waysideArchive_.opening)*1.57079632679f*1.30f;
    if(UsesRescueExterior() && !candidate.VolumeClear(*chapterTerrain_->HingedContact(),
            playerX_,playerZ_,playerX_,playerZ_,playerSupport_.worldY+.001f,
            playerSupport_.worldY+posture_.Height(),kPlayerCollisionRadius+swingMargin))return;
    waysideArchive_.opening=progress;
    waysideArchive_.pose=candidate;
    if(progress>=1.f) {
        waysideArchive_.stage=ArchiveStage::Open;
        TryStartShrineReveal(ChapterInteractionContext(false));
    }
}

bool GameSimulation::ArchiveContactClear(float x0,float z0,float x1,float z1,float y0,float y1,float radius) const {
    if(!chapterTerrain_ || chapterTerrainGeneration_!=worldRoute_.generation ||
       !chapterTerrain_->HingedContact())return true;
    return waysideArchive_.pose.VolumeClear(*chapterTerrain_->HingedContact(),x0,z0,x1,z1,y0,y1,radius);
}

void GameSimulation::TryStartShrineReveal(const dialogue::Context& context) {
    if(waysideArchive_.stage==bellwether::ArchiveStage::Open && !waysideArchive_.lanternRevealStarted &&
       chapterDialogue_.TryStartShrineSpeech(dialogue::ShrineSpeech::Reveal,context))
        waysideArchive_.lanternRevealStarted=true;
}

std::optional<dialogue::Tuple> GameSimulation::HeldRewardLanternSource() const {
    const auto& lantern=lanternPendulum_.Snapshot();
    if(rescueTraversal_.Snapshot().equipmentStowed ||
       interactionState_.heldLightKind!=interactions::HeldLightKind::RewardLantern || !lantern.initialized)
        return std::nullopt;
    const auto& body=lantern.worldFromBody;
    constexpr float lightBelowHinge=items::kRewardLanternLightBelowHinge*items::kClaimedRewardLanternScale;
    return dialogue::Tuple{body[12]-body[4]*lightBelowHinge,body[13]-body[5]*lightBelowHinge,
                           body[14]-body[6]*lightBelowHinge};
}

dialogue::Context GameSimulation::ChapterInteractionContext(bool paused) const {
    using namespace horde::gameplay::dialogue;
    Context c;
    c.player={playerX_,playerSupport_.worldY,playerZ_};
    c.listener={playerX_,kShowcaseEyeWorldY+PlayerHeightDelta(playerSupport_.worldY)-posture_.drop,playerZ_};
    c.yaw=playerYawRadians_;c.paused=paused;
    c.alive=playerVitals_.Snapshot().phase==PlayerLifePhase::Alive;
    c.combat=combatSnapshot_.attackerIndex>=0 || lichEncounter_.Snapshot().phase==LichPhase::Charging;
    c.claimed=chestRewardSequence_.Snapshot().phase==interactions::ChestRewardPhase::LanternClaimed;
    c.exterior=config_.developmentRescueJourney&&rescueTraversal_.Snapshot().exteriorSide;
    c.traversing=config_.developmentRescueJourney&&rescueTraversal_.IsActive();
    const auto& kit=chapterCompanion_.State();
    c.kitAtChair=kit.grounded && kit.bellwetherArrival>=BellwetherArrivalStage::ChairTurn;
    c.kitSeated=c.kitAtChair && kit.bellwetherArrival==BellwetherArrivalStage::Seated;
    const auto hearthMark=campaign::kBellwetherCheckpointPosition;
    c.tavernInReach=c.kitAtChair && playerSupport_.grounded &&
        std::abs(playerSupport_.worldY-hearthMark.y)<.001f &&
        std::hypot(playerX_-hearthMark.x,playerZ_-hearthMark.z)<=1.75f;
    if(c.exterior && kit.grounded) {
        const float lowering=kit.motion.sitPlaybackSeconds/kTavernSitDurationSeconds;
        c.companionSource=Tuple{kit.x,kit.y+kCompanionSpeechHeight-.27f*lowering,kit.z};
    }
    const auto& keeper=tavernKeeper_.Pose();
    if(keeper && c.exterior && chapterTerrain_ &&
       chapterTerrainGeneration_==worldRoute_.generation && config_.bellwetherTavernKeeperAdmitted &&
       config_.combatFoundation1_7 && !legacyCombatCheckpoint_) {
        c.tavernKeeperSource=Tuple{keeper->foot[0],keeper->foot[1]+kCompanionSpeechHeight,keeper->foot[2]};
        c.keeperAtHearth=keeper->stage==TavernKeeperStage::AtHearth;
    }
    const auto& mark=bellwether::kWaysideMechanism;
    if(config_.combatFoundation1_7 && !legacyCombatCheckpoint_ && c.alive && c.claimed && c.exterior &&
       !c.combat && !c.traversing && c.kitSeated && playerSupport_.grounded && chapterTerrain_ &&
       chapterTerrainGeneration_==worldRoute_.generation && chapterTerrain_->HingedContact() &&
       chapterDialogue_.State().hearthLinesResolved==kHearthLines.size() &&
       std::hypot(c.player.x-mark[0],c.player.z-mark[2])<=4.f && std::abs(c.listener.y-mark[1])<=1.5f)
        c.shrineLanternSource=HeldRewardLanternSource();
    return c;
}

void GameSimulation::UpdateChapterDialogue(const InputSnapshot& input,float seconds) {
    // Historical replay/capture fixtures keep their old timing and semantics.
    if(!config_.combatFoundation1_7 || legacyCombatCheckpoint_) return;
    using namespace horde::gameplay::dialogue;
    const auto* terrain=chapterTerrainGeneration_==worldRoute_.generation?chapterTerrain_.get():nullptr;
    const auto arrival=chapterCompanion_.State().bellwetherArrival;
    if(config_.bellwetherTavernKeeperAdmitted && terrain && !input.paused &&
       rescueTraversal_.Snapshot().exteriorSide && !rescueTraversal_.IsActive() &&
       playerVitals_.Snapshot().phase==PlayerLifePhase::Alive && arrival!=BellwetherArrivalStage::AtLookout)
        tavernKeeper_.Step(seconds,arrival==BellwetherArrivalStage::Seated,*terrain);
    auto c=ChapterInteractionContext(input.paused);
    if(terrain && c.tavernKeeperSource && tavernKeeper_.Pose()->stage==TavernKeeperStage::AtDoor &&
       !c.paused && c.alive && !c.traversing && !c.combat) {
        const auto door=traversal::kBellwetherTavernRoute[traversal::kBellwetherSouthDoorStation];
        const auto& kit=chapterCompanion_.State();
        const bool approaching=arrival==BellwetherArrivalStage::ProtectiveApproach &&
            kit.bellwetherStation>traversal::kBellwetherSouthApproachStation &&
            kit.bellwetherStation<=traversal::kBellwetherSouthDoorStation+1 &&
            std::hypot(kit.x-door.x,kit.z-door.z)<=2.5f && c.companionSource;
        const auto audibleFromDoor=[&](Tuple other) {
            const auto& source=*c.tavernKeeperSource;
            const float height=std::min(source.y,other.y);
            return std::abs(source.y-other.y)<=1.4f && std::hypot(other.x-source.x,other.z-source.z)<=4.5f &&
                terrain->NonWalkableVolumeClear(source.x,source.z,other.x,other.z,
                    height-.025f,height+.025f,.025f);
        };
        if(approaching || chapterDialogue_.State().line==Line::TavernWelcome)
            c.tavernDoorInReach=playerSupport_.grounded && audibleFromDoor(c.listener);
        c.kitAtSouthDoor=c.tavernDoorInReach && approaching && audibleFromDoor(*c.companionSource);
    }
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
        const bool assessing=kit.bellwetherArrival==BellwetherArrivalStage::AnkleAssessment;
        c.kitCaughtAnkle=assessing && kit.motion.anklePlaybackSeconds>=audio::kKitAnkleBeats.back().seconds;
        c.kitReadyForProtectiveWalk=assessing && kit.motion.anklePlaybackSeconds>=kAnkleAssessmentDurationSeconds;
        c.roadInReach=(assessing || kit.bellwetherArrival==BellwetherArrivalStage::ProtectiveApproach) &&
            playerSupport_.grounded && std::abs(playerSupport_.worldY-kit.y)<=1.6f &&
            std::hypot(playerX_-kit.x,playerZ_-kit.z)<=5.f;
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
    if(c.exterior && c.alive && !c.traversing && !c.paused && terrain &&
       campaignBoundary_ && campaignBoundary_->id==campaign::CheckpointId::ForestEndpoint &&
       chapterDialogue_.State().line==Line::None && playerSupport_.grounded)
        chapterCompanion_.TryStartBellwetherArrival(playerX_,playerZ_,*terrain);
    const float previousAnkleTime=chapterCompanion_.State().motion.anklePlaybackSeconds;
    chapterCompanion_.Step(seconds,playerX_,playerZ_,c.exterior,
        chapterDialogue_.State().proofAccepted && (!guidePending || readyToLead),c.paused,
        terrain,
        config_.authoredChapterTerrain,c.alive&&!c.traversing,
        guidePending && readyToLead,chapterDialogue_.HoldingAnkleAssessment());
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
    chapterDialogue_.RefreshCompanionSpatial(ChapterInteractionContext(input.paused));
    const auto& speech=chapterDialogue_.State();
    const auto lanternPosition=speech.line==Line::TavernIntroduce?HeldRewardLanternSource():std::nullopt;
    chapterCompanion_.AdvanceActing(seconds,speech,c.exterior,c.alive,
        !proofWasAccepted && speech.proofAccepted,lanternPosition);
}
bool GameSimulation::AcknowledgeChapterPresentation(std::uint64_t tick,std::uint64_t generation) {
    if(tick!=snapshot_.tickIndex||generation!=chapterDialogue_.State().generation||
        snapshot_.paused||!snapshot_.rescue.exteriorSide||snapshot_.rescue.equipmentStowed||
        snapshot_.interaction.heldLightPose!=interactions::HeldLightPose::High) return false;
    chapterVisibleRaise_=snapshot_.lastConsumedToggleHeldLightPoseSequence;
    return true;
}
} // namespace horde::gameplay::simulation
