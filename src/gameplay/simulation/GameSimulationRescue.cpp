#include "gameplay/simulation/GameSimulation.h"
#include <algorithm>
#include <cmath>

namespace horde::gameplay::simulation {
using namespace horde::gameplay::traversal;

bool GameSimulation::UsesRescueExterior() const {
    return config_.developmentRescueJourney && rescueTraversal_.Snapshot().exteriorSide;
}
void GameSimulation::SetDevelopmentRescueJourney(bool enabled) {
    RestoreRescueEquipment();
    config_.developmentRescueJourney=enabled;
    rescueTraversal_.Reset(); rescueOpeningSeconds_=0;worldRoute_.Invalidate();
    worldRoute_.current=WorldZoneId::Dungeon;
    if(enabled) {
        config_.developmentWorldRoute=false;
        config_.playerStartYawRadians=3.14159265359f;
        playerYawRadians_=config_.playerStartYawRadians;
        if(chestRewardSequence_.Snapshot().phase==interactions::ChestRewardPhase::LanternClaimed) {
            rescueTraversal_.NotifyLanternClaimed(); rescueTraversal_.DeployOwned(); rescueOpeningSeconds_=1;
        }
    }
    if(!enabled) {playerX_=config_.playerStartX;playerZ_=config_.playerStartZ;}
    ResetPlayerSupport();ClearRunIntent();ClearEvents();ResolveHeldItems();
    ResolvePlayerAnimation(0);ResolveFireEmitters(0);RefreshSnapshot(lastInput_);
}
void GameSimulation::RecoverRescueJourney() {
    // Only traversal owns a rollback destination. Ordinary play (including
    // pre-claim play) retains the simulation's current support and zone.
    const bool traversalOwned=rescueTraversal_.IsActive();
    rescueTraversal_.RecoverForReconstruction();RestoreRescueEquipment();worldRoute_.Invalidate(true);
    const auto& s=rescueTraversal_.Snapshot();
    if(traversalOwned) {
        playerX_=s.playerPosition.x;playerZ_=s.playerPosition.z;
        playerSupport_={s.supportWorldY,static_cast<PlayerSupportId>(130),true};
        worldRoute_.current=s.exteriorSide?WorldZoneId::TombExterior:WorldZoneId::Dungeon;
    }
    worldRoute_.safeX=playerX_;worldRoute_.safeZ=playerZ_;worldRoute_.safeSupport=playerSupport_;
    ClearRunIntent();CancelCombatTransients();ClearScheduledCombatEdges(true);ClearEvents();
    SynchronizePausedInput(lastInput_);ResolveHeldItems();ResolvePlayerAnimation(0);
    ResolveFireEmitters(0);RefreshSnapshot(lastInput_);
}
void GameSimulation::StepRescueJourney(bool paused) {
    rescueMovementSuppressedThisTick_=false;
    if(!config_.developmentRescueJourney || !rescueTraversal_.Snapshot().deploymentCount) return;
    const bool wasActive=rescueTraversal_.IsActive();
    if(!paused) rescueOpeningSeconds_=std::min(1.0f,rescueOpeningSeconds_+1.0f/60.0f);
    rescueTraversal_.Step({{playerX_,playerSupport_.worldY,playerZ_},playerSupport_.worldY,
        kLowerSupportWorldY,paused,false,rescueOpeningSeconds_>=1.0f});
    if(!paused && chestRewardSequence_.Snapshot().phase==interactions::ChestRewardPhase::LanternClaimed) {
        // Revalidate and consume the one contextual intent before movement and
        // combat, so simultaneous edges cannot act before traversal ownership.
        while(pendingInteractCommands_>0) {
            --pendingInteractCommands_;++lastConsumedInteractSequence_;
            TryRescueInteraction();
        }
    }
    if(rescueTraversal_.IsActive() && !ValidRescueCapsulePath(rescueTraversal_.Snapshot()))
        rescueTraversal_.RecoverForReconstruction();
    rescueMovementSuppressedThisTick_=wasActive||rescueTraversal_.IsActive();
    if(rescueMovementSuppressedThisTick_) {
        const auto& s=rescueTraversal_.Snapshot();
        playerX_=s.playerPosition.x;playerZ_=s.playerPosition.z;
        playerSupport_={s.supportWorldY,static_cast<PlayerSupportId>(132),!rescueTraversal_.IsActive()};
        playerYawRadians_=std::isfinite(lastInput_.yawRadians)?lastInput_.yawRadians:playerYawRadians_;
        playerPitchRadians_=std::clamp(lastInput_.pitchRadians,-.32f,.28f);
        // Consume edges while the motor owns movement. They cannot replay on release.
        ClearRunIntent();CancelCombatTransients();ClearScheduledCombatEdges(true);
        pendingAttackCommands_=0;
        pendingParryCommands_=0;
        pendingDodgeCommands_=0;
        lastConsumedToggleHeldLightPoseSequence_=latestToggleHeldLightPoseSequence_;pendingToggleHeldLightPoseCommands_=0;
        walkVisualAmount_=0;playerMovementSpeed_=0;snapshot_.playerTravelledThisTick=0;
        if(!rescueTraversal_.IsActive()) {
            RestoreRescueEquipment();
            worldRoute_.current=s.exteriorSide?WorldZoneId::TombExterior:WorldZoneId::Dungeon;
            worldRoute_.safeX=playerX_;worldRoute_.safeZ=playerZ_;worldRoute_.safeSupport=playerSupport_;
        }
    }
}
bool GameSimulation::TryRescueInteraction() {
    const auto& s=rescueTraversal_.Snapshot();
    const Vec3 player{playerX_,playerSupport_.worldY,playerZ_};
    if(rescueTraversal_.IsActive() || !AtRopeEndpoint(playerX_,playerZ_,s.exteriorSide) ||
       !CanApproachRopeEndpoint(player,playerSupport_.worldY,s.exteriorSide) ||
       rescueOpeningSeconds_<1 || !rescueTraversal_.CanInteractAt(player,playerSupport_.worldY)) return false;
    const auto zone=s.exteriorSide?WorldZoneId::Dungeon:WorldZoneId::TombExterior;
    const bool ready=worldRoute_.readiness[static_cast<std::size_t>(zone)]==ZoneReadiness::Ready;
    if(!ready) return false;
    if(!rescueTraversal_.Request()) return false;
    const auto generation=rescueTraversal_.Snapshot().generation;
    rescueTraversal_.PublishReadiness(generation,ready);
    const bool committed=rescueTraversal_.TryBegin(generation,
        chestRewardSequence_.Snapshot().phase==interactions::ChestRewardPhase::LanternClaimed,ready);
    if(committed) {
        ClearRunIntent();CancelCombatTransients();ClearScheduledCombatEdges(true);
        rescueSavedSword_=heldItems_[1];rescueSavedSwordValid_=true;
        heldItems_[1]=items::MakeHeldItemState(items::HeldItemId::Sword,items::HeldHand::RightHand,items::HeldItemParentMode::BodyStow);
        heldItems_[1].worldFromItem=rescueSavedSword_.worldFromItem;
        EmitSwordAttachmentChange();
    }
    return committed;
}
void GameSimulation::RestoreRescueEquipment() {
    if(!rescueSavedSwordValid_) return;
    const auto parent=rescueSavedSword_.parentMode;
    heldItems_[1]=items::MakeHeldItemState(items::HeldItemId::Sword,items::HeldHand::RightHand,parent);
    heldItems_[1].worldFromItem=rescueSavedSword_.worldFromItem;
    rescueSavedSwordValid_=false;EmitSwordAttachmentChange();
}
items::HeldItemTransform GameSimulation::RescueLanternHinge() const {
    if(!config_.developmentRescueJourney || !rescueTraversal_.Snapshot().equipmentStowed)
        return heldItemFixedStepState_.worldFromLeftHand;
    // Original development carry attachment on the anatomical left hip. No
    // camera-follow offset: body heading is fixed to the rope, eye/look is free.
    auto m=items::IdentityHeldItemTransform();
    const float yaw=rescueTraversal_.Snapshot().bodyYawRadians,c=std::cos(yaw),s=std::sin(yaw);
    m[0]=c;m[2]=-s;m[8]=s;m[10]=c;
    // Rearward hip mount clears the solid apron while the rope still supports
    // the body below its top; the real ring/body GLB bounds validate this offset.
    m[12]=playerX_-.32f*c-.35f*s;m[13]=playerSupport_.worldY+.82f;m[14]=playerZ_+.32f*s-.35f*c;
    return m;
}
void GameSimulation::ApplyRescuePresentation() {
    if(!config_.developmentRescueJourney || !rescueTraversal_.Snapshot().equipmentStowed) return;
    heldItemFixedStepState_.kinematics.swordStowBlend=1;
    heldItemFixedStepState_.kinematics.swordHandGripBlend=0;
    // The legacy left-item record is OriginalTorch, not the claimed lantern.
    // The reward lantern has its own physical hip/pendulum rig. Never attach
    // an inactive original-torch record to a wrist that now grips the rope.
    // Its existing detached trajectory remains untouched; the ordinary fixed
    // step resolver restores the original ownership when traversal ends.
    if(interactionState_.heldLightKind==interactions::HeldLightKind::RewardLantern &&
       heldItems_[0].parentMode==items::HeldItemParentMode::HandSocket)
        heldItems_[0].parentMode=items::HeldItemParentMode::WorldObject;
    // Keep the ordinary rigid free-arm bases for the grounded handoff. Loaded
    // rope targets are applied to both actual rig arms by RescuePlayerRig;
    // the independent lantern hinge never takes ownership of either wrist.
}
void GameSimulation::PublishRescueSnapshot() {
    snapshot_.developmentRescueJourney=config_.developmentRescueJourney;
    snapshot_.rescue=rescueTraversal_.Snapshot();snapshot_.rescuePrompt=RescuePrompt::None;
    if(!config_.developmentRescueJourney) return;
    snapshot_.playerSupportGeneration=worldRoute_.generation;
    // Night and route continuation are a mode boundary, not changes to frozen
    // legacy dawn captures or the Keeper's death/torch/reward ordering.
    snapshot_.lich.finaleDawnRevealProgress=0;
    snapshot_.lich.finaleSkylightOpenProgress=rescueOpeningSeconds_;
    snapshot_.finaleComplete=false;
    if(rescueTraversal_.IsActive()) {
        snapshot_.rescuePrompt=RescuePrompt::Traversing;
        snapshot_.playerAnimation.swordHandGripBlend=0;
        snapshot_.playerAnimation.swordStowBlend=1;
    } else if(snapshot_.rescue.deploymentCount && AtRopeEndpoint(playerX_,playerZ_,snapshot_.rescue.exteriorSide) &&
              CanApproachRopeEndpoint({playerX_,playerSupport_.worldY,playerZ_},playerSupport_.worldY,
                                      snapshot_.rescue.exteriorSide)) {
        const auto zone=snapshot_.rescue.exteriorSide?WorldZoneId::Dungeon:WorldZoneId::TombExterior;
        snapshot_.rescuePrompt=rescueOpeningSeconds_>=1 &&
            rescueTraversal_.CanInteractAt({playerX_,playerSupport_.worldY,playerZ_},playerSupport_.worldY) &&
            worldRoute_.readiness[static_cast<std::size_t>(zone)]==ZoneReadiness::Ready
            ?(snapshot_.rescue.exteriorSide?RescuePrompt::Descend:RescuePrompt::Climb):RescuePrompt::Preparing;
    }
}
} // namespace horde::gameplay::simulation
