#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/CorridorCollision.h"
#include "gameplay/traversal/ChapterTerrainMovement.h"
#include "scene/TombDressingContact.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace horde::gameplay::simulation {
namespace {
bool SafeSupport(const campaign::Checkpoint& c, const TriangleSupportSurface& terrain) {
    if (c.id == campaign::CheckpointId::LowerClaimed)
        return IsShowcasePlayerPositionWalkable(c.position.x,c.position.z) &&
            scene::TombDressingMovementClear(c.position.x,c.position.z,c.position.x,c.position.z);
    const auto support=traversal::ChapterExteriorSupport(&terrain,c.position.x,c.position.z);
    return support.grounded && std::abs(support.worldY-c.position.y)<.001f &&
        traversal::ChapterExteriorMovementClear(terrain,c.position.x,c.position.z,
            c.position.x,c.position.z,c.position.y,kPlayerCollisionRadius);
}
}

std::optional<campaign::Checkpoint> GameSimulation::CaptureCampaignCheckpoint(campaign::CheckpointId id) const {
    using namespace interactions;
    const auto& s=snapshot_;
    const bool upper=id==campaign::CheckpointId::SafeExit;
    if (!config_.combatFoundation1_7 || !config_.waterfallSkeletonEncounter ||
        !config_.developmentRescueJourney || !config_.authoredChapterTerrain || legacyCombatCheckpoint_ ||
        !chapterTerrain_ || chapterTerrainGeneration_!=worldRoute_.generation ||
        staticDetailLease_.resourceGeneration || pendingCampaignRestore_ ||
        !s.playerAlive || !s.playerGrounded || posture_.Active() || crouchToggleActive_ || lastInput_.crouchHeld || s.rescue.equipmentStowed || rescueTraversal_.IsActive() ||
        s.rescue.exteriorSide!=upper || !s.rescue.ropeReady || rescueOpeningSeconds_<1.0f ||
        s.lich.phase!=LichPhase::Dead || !s.lich.deathAnimationComplete ||
        s.lich.deathDissolveProgress<1.0f || s.lich.roomMistDensityScale>0.0f ||
        s.chestReward.phase!=ChestRewardPhase::LanternClaimed ||
        s.interaction.heldLightKind!=HeldLightKind::RewardLantern || s.interaction.heldLightPoseProgress<1.0f ||
        s.skeletonEnemyCount!=2 || s.playerCombat.action!=PlayerCombatAction::Idle ||
        s.dodgeActive || s.runActive || s.playerMovementSpeedMetresPerSecond>.0001f ||
        heldItems_[1].transition.active ||
        (heldItems_[1].parentMode!=items::HeldItemParentMode::HandSocket &&
         heldItems_[1].parentMode!=items::HeldItemParentMode::BodyStow) ||
        pendingAttackCommands_ || pendingParryCommands_ || pendingDodgeCommands_ ||
        pendingInteractCommands_ || pendingToggleHeldLightPoseCommands_ || pendingRouteResetCommands_ || pendingRetryCommands_)
        return std::nullopt;
    const auto zone=campaign::Destination(id);
    if(worldRoute_.readiness[static_cast<std::size_t>(zone)]!=ZoneReadiness::Ready) return std::nullopt;
    campaign::Checkpoint c;
    c.id=id;c.zone=zone;c.position={s.playerX,s.playerSupportWorldY,s.playerZ};
    c.yaw=s.playerYawRadians;c.pitch=s.playerPitchRadians;c.vitality=s.playerVitals.vitality;
    c.firstAscentCompleted=s.rescue.firstAscentCompleted;
    c.swordStowed=heldItems_[1].parentMode==items::HeldItemParentMode::BodyStow;
    c.lanternHigh=s.interaction.heldLightPose==HeldLightPose::High;
    c.torchDropX=s.torchFailure.droppedX;c.torchDropZ=s.torchFailure.droppedZ;
    c.torchDropYaw=s.torchFailure.droppedYawRadians;c.torchDropPitch=s.torchFailure.droppedViewPitchRadians;
    c.dialogue=chapterDialogue_.ExportProgress();
    for(std::size_t i=0;i<c.guards.size();++i) {
        if(!s.skeletonEnemies[i].dead || s.skeletonEnemies[i].health!=0) return std::nullopt;
        c.guards[i]={s.skeletonEnemies[i].x,s.skeletonEnemies[i].z,s.skeletonEnemies[i].facingRadians};
    }
    if(!campaign::Validate(c) || !SafeSupport(c,*chapterTerrain_)) return std::nullopt;
    return c;
}

std::optional<campaign::RestoreToken> GameSimulation::BeginCampaignRestore(const campaign::Checkpoint& c) {
    // An invalid request does not cancel an already validated pending request.
    if(!campaign::Validate(c) || !config_.combatFoundation1_7 || !config_.waterfallSkeletonEncounter ||
       config_.playerMountProfile!=items::PlayerMountProfile::AnatomicalBody ||
       staticDetailLease_.resourceGeneration || campaignRequestSequence_==UINT64_MAX ||
       std::max(worldRoute_.generation,campaignDestinationGeneration_)==UINT64_MAX)
        return std::nullopt;
    campaignDestinationGeneration_=std::max(worldRoute_.generation,campaignDestinationGeneration_)+1;
    campaign::RestoreToken token{++campaignRequestSequence_,campaignDestinationGeneration_};
    pendingCampaignRestore_=PendingCampaignRestore{c,token,worldRoute_.generation};
    return token;
}

bool GameSimulation::CommitCampaignRestore(campaign::RestoreToken token, WorldZoneToken readyDestination,
    std::shared_ptr<const TriangleSupportSurface> terrain, const InputSnapshot& currentInput,
    std::uint64_t currentInputPublicationSequence) {
    if(!pendingCampaignRestore_ || token!=pendingCampaignRestore_->token ||
       pendingCampaignRestore_->sourceGeneration!=worldRoute_.generation || staticDetailLease_.resourceGeneration ||
       token.worldGeneration!=readyDestination.generation ||
       readyDestination.zone!=pendingCampaignRestore_->checkpoint.zone ||
       !terrain || !terrain->TriangleCount() || tickIndex_==UINT64_MAX ||
       chapterDialogue_.State().generation>=UINT64_MAX-2 ||
       chapterCompanion_.State().generation==UINT64_MAX || rescueTraversal_.Snapshot().generation==UINT64_MAX)
        return false;
    const auto c=pendingCampaignRestore_->checkpoint;
    if(!campaign::Validate(c) || !SafeSupport(c,*terrain)) return false;
    const bool upper=c.id==campaign::CheckpointId::SafeExit;
    auto config=config_;
    config.developmentRescueJourney=true;config.authoredChapterTerrain=true;
    config.developmentWorldRoute=false;config.developmentSupportFixture=false;
    config.swordStartsStowed=c.swordStowed;
    GameSimulation next(config);
    next.worldRoute_.generation=token.worldGeneration;
    next.worldRoute_.current=c.zone;
    next.chapterTerrain_=std::move(terrain);next.chapterTerrainGeneration_=token.worldGeneration;
    if(!next.worldRoute_.Publish(readyDestination,ZoneReadiness::Ready)) return false;
    next.playerX_=c.position.x;next.playerZ_=c.position.z;
    next.playerYawRadians_=c.yaw;next.playerPitchRadians_=c.pitch;
    next.playerSupport_={c.position.y,upper?static_cast<PlayerSupportId>(130):PlayerSupportId::RouteFloor,true};
    next.worldRoute_.safeX=c.position.x;next.worldRoute_.safeZ=c.position.z;next.worldRoute_.safeSupport=next.playerSupport_;
    next.tickIndex_=tickIndex_+1; // old presentation acknowledgements cannot match
    next.presentationAspect_=presentationAspect_;
    next.campaignRequestSequence_=campaignRequestSequence_;
    next.campaignDestinationGeneration_=campaignDestinationGeneration_;
    next.rescueTraversal_.RestoreCampaignSafeSide(upper,c.firstAscentCompleted,rescueTraversal_.Snapshot().generation);
    next.rescueOpeningSeconds_=1.0f;
    next.chapterFirstExterior_=upper;
    next.lichEncounter_.RestoreCampaignDefeated();
    next.combatTeaching_.Skip();
    next.waterfallWarningEmitted_=true;
    std::array<SkeletonSpawnPose,kSkeletonCombatantCapacity> guards{};
    for(std::size_t i=0;i<guards.size();++i) guards[i]={{c.guards[i].x,c.guards[i].z},c.guards[i].yaw,0};
    next.swordCombat_.RestoreCampaignDefeated(guards);next.combatSnapshot_=next.swordCombat_.Snapshot();
    next.enemyDirector_.MarkSelectedDead();
    next.enemyDirector_.Update(c.position.x,c.position.z,EnemyKind::Lich);next.enemyDirector_.MarkSelectedDead();
    next.activeEnemyKind_=EnemyKind::Lich;
    next.playerVitals_.RestoreCampaignVitality(c.vitality);
    interactions::ChestRewardSnapshot chest;
    chest.phase=interactions::ChestRewardPhase::LanternClaimed;chest.lidOpenProgress=1;
    next.chestRewardSequence_.Import(chest);
    interactions::FinaleSequenceSnapshot finale;
    finale.lichDefeated=finale.lanternClaimed=true;
    next.finaleSequence_.Import(finale);
    interactions::EquipRewardLantern(next.interactionState_);
    next.interactionState_.heldLightPose=c.lanternHigh?interactions::HeldLightPose::High:interactions::HeldLightPose::Low;
    next.interactionState_.heldLightPoseProgress=1.0f;
    next.torchFailure_.RestoreCampaignSettled(c.torchDropX,c.torchDropZ,c.torchDropYaw,c.torchDropPitch);
    next.torchFailureSnapshot_=next.torchFailure_.Snapshot();
    items::ImportHeldItemCheckpoint(next.heldItems_,false,next.tickIndex_);next.ResetSwordEquipment();
    // Saved equipment belongs to this session. A later New Game/reset keeps
    // the caller's configured opening equipment policy.
    next.config_.swordStartsStowed=config_.swordStartsStowed;
    next.chapterCompanion_.RestoreCampaignReunion(upper,c.position.x,c.position.z,
        next.chapterTerrain_.get(),chapterCompanion_.State().generation);
    if(upper && !next.chapterCompanion_.State().grounded) return false;
    // Consume the live session's published edges, never saved input identities.
    next.SynchronizePausedInput(lastInput_,inputPublicationSequence_);
    next.SynchronizePausedInput(currentInput,currentInputPublicationSequence);
    next.campaignInputPublicationFloor_=std::max(inputPublicationSequence_,currentInputPublicationSequence);
    InputSnapshot neutral=next.lastInput_;
    neutral.moveForward=neutral.moveStrafe=0;neutral.runHeld=false;neutral.crouchHeld=false;neutral.hasAuthoritativePlayerPose=false;
    neutral.authoritativePlayerX=neutral.authoritativePlayerZ=0;
    neutral.yawRadians=c.yaw;neutral.pitchRadians=c.pitch;
    neutral.dialogueCompletionLine=0;neutral.dialogueCompletionGeneration=0;neutral.combatEdgeHistory={};
    next.lastInput_=neutral;
    dialogue::Context context;
    context.player={c.position.x,c.position.y,c.position.z};
    context.listener={c.position.x,kShowcaseEyeWorldY+PlayerHeightDelta(c.position.y),c.position.z};
    context.yaw=c.yaw;context.paused=true;context.claimed=true;context.exterior=upper;
    context.lanternLow=!c.lanternHigh;context.lanternHigh=c.lanternHigh;
    context.manualRaiseSequence=next.lastConsumedToggleHeldLightPoseSequence_;
    if(!next.chapterDialogue_.RestoreProgress(c.dialogue,context,chapterDialogue_.State().generation,
        next.lastConsumedToggleHeldLightPoseSequence_)) return false;
    next.ResolveHeldItems();
    next.lanternPendulum_.Reset(next.RescueLanternHinge(),
        next.heldItemFixedStepState_.kinematics.rewardLanternPresentationYawRadians);
    next.lanternPendulumResetPending_=true;
    next.ResolvePlayerAnimation(0);next.ResolveFireEmitters(0);next.RefreshSnapshot(neutral);
    next.ClearEvents();
    *this=std::move(next);
    return true;
}

InputSnapshot GameSimulation::FilterCampaignInput(const InputSnapshot& input, std::uint64_t publicationSequence) {
    if(!campaignInputPublicationFloor_) return input;
    // The platform owner must release held controls and rebase its absolute
    // camera before resuming. Repeating an old coherent publication, including
    // a replay-only authoritative pose, must never move a freshly loaded player.
    const bool fresh=publicationSequence>*campaignInputPublicationFloor_ ||
        (publicationSequence==0 && *campaignInputPublicationFloor_==0);
    const bool neutral=input.moveForward==0 && input.moveStrafe==0 && !input.runHeld && !input.crouchHeld &&
        !input.hasAuthoritativePlayerPose && input.yawRadians==playerYawRadians_ &&
        input.pitchRadians==playerPitchRadians_;
    if(fresh && neutral) {
        campaignInputPublicationFloor_.reset();
        return input;
    }
    auto blocked=input;
    blocked.paused=true;blocked.moveForward=blocked.moveStrafe=0;blocked.runHeld=false;blocked.crouchHeld=false;
    blocked.hasAuthoritativePlayerPose=false;
    blocked.yawRadians=playerYawRadians_;blocked.pitchRadians=playerPitchRadians_;
    blocked.dialogueCompletionLine=0;blocked.dialogueCompletionGeneration=0;
    return blocked;
}
} // namespace horde::gameplay::simulation
