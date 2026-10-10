#include "gameplay/simulation/GameSimulation.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "gameplay/CorridorCollision.h"
#include "gameplay/ShowcaseCheckpoints.h"
#include "gameplay/effects/KeeperTorchLighting.h"

namespace horde::gameplay::simulation
{
namespace
{

constexpr float kMinimumPitch = -0.32f;
constexpr float kMaximumPitch = 0.28f;
constexpr float kDodgeDurationSeconds = 0.20f;
constexpr float kDodgeDistanceMetres = 0.90f;
constexpr float kDodgeCooldownSeconds = 0.55f;
constexpr float kRunSpeedMetresPerSecond = 3.2f;
constexpr float kMovementAccelerationMetresPerSecondSquared = 8.0f;

float FiniteOr(float value, float fallback)
{
    return std::isfinite(value) ? value : fallback;
}

std::uint64_t SequenceDelta(std::uint64_t newer, std::uint64_t older)
{
    return newer >= older ? newer - older : 0u;
}

std::uint64_t SequenceFor(const SimulationCommandSequences& commands,
                          const CombatInputEdgeKind kind)
{
    switch (kind)
    {
    case CombatInputEdgeKind::Attack: return commands.attack;
    case CombatInputEdgeKind::Parry: return commands.parry;
    case CombatInputEdgeKind::Dodge: return commands.dodge;
    }
    return 0u;
}

std::size_t CombatKindIndex(const CombatInputEdgeKind kind)
{
    switch (kind)
    {
    case CombatInputEdgeKind::Attack: return 0u;
    case CombatInputEdgeKind::Parry: return 1u;
    case CombatInputEdgeKind::Dodge: return 2u;
    }
    return 0u;
}

EntityId SkeletonEntity(std::size_t index)
{
    return index == 0u ? EntityId::SkeletonA : EntityId::SkeletonB;
}

} // namespace

GameSimulation::GameSimulation(GameSimulationConfig config)
    : config_(config),
      playerX_(config.playerStartX),
      playerZ_(config.playerStartZ),
      playerYawRadians_(config.playerStartYawRadians),
      playerPitchRadians_(std::clamp(FiniteOr(config.playerStartPitchRadians, 0.0f),
                                    kMinimumPitch, kMaximumPitch))
{
    if (!IsShowcasePlayerPositionWalkable(playerX_, playerZ_))
    {
        playerX_ = kPlayerSpawn.x;
        playerZ_ = kPlayerSpawn.z;
    }
    config_.movementSpeedMetresPerSecond = std::max(0.0f, config_.movementSpeedMetresPerSecond);
    for (std::size_t index = 0u; index < effects::kKeeperTorchAnchors.size(); ++index)
    {
        const auto& anchor = effects::kKeeperTorchAnchors[index];
        fireEmitters_[index + 1u] = effects::MakeWorldTorchFireEmitter(anchor.stableId, anchor.seed);
    }
    combatTeaching_.Reset(config_.combatFoundation1_7);
    lichEncounter_.SetReadableCombat(CurrentCombatRules());
    enemyDirector_.Reset();
    activeEnemyKind_ = enemyDirector_.Snapshot().selectedEnemy;
    if (config_.waterfallSkeletonEncounter)
    {
        swordCombat_.Reset(kSkeletonEnemyCapacity, kWaterfallSkeletonPairCenter,
                           &kWaterfallSkeletonGuardSpawns, CurrentCombatRules() ? 2 : 1);
        combatSnapshot_ = swordCombat_.Snapshot();
    }
    else if (CurrentCombatRules())
    {
        swordCombat_.Reset(kSkeletonEnemyCapacity, {0.0f, -4.65f}, nullptr, 2);
    }
    combatSnapshot_ = swordCombat_.Snapshot();
    torchFailureSnapshot_ = torchFailure_.Snapshot();
    ResetSwordEquipment();
    ResolveHeldItems();
    lanternPendulum_.Reset(
        heldItemFixedStepState_.worldFromLeftHand,
        heldItemFixedStepState_.kinematics.rewardLanternPresentationYawRadians);
    ResolvePlayerAnimation(0.0f);
    ResolveFireEmitters(0.0f);
    RefreshSnapshot(lastInput_);
}

std::uint32_t GameSimulation::AdvanceFrame(const InputSnapshot& input,
                                           double frameDeltaSeconds,
                                           std::uint64_t inputPublicationSequence,
                                           std::uint64_t ownerAdvanceSteadyNs)
{
    lastInput_ = input;
    inputPublicationSequence_ = inputPublicationSequence;
    const bool presentationWasActive = combatPresentation_.Snapshot().parrySuccessActive;
    combatPresentation_.AdvanceFrame(frameDeltaSeconds, input.paused);
    const std::size_t eventsBeforeFrame = events_.Size();
    snapshot_.eventsEmittedThisTick = 0u;
    snapshot_.eventsEmittedThisFrame = 0u;
    if (ownerAdvanceSteadyNs != 0u)
    {
        ScheduleTimestampedCombatEdges(input, frameDeltaSeconds,
                                       ownerAdvanceSteadyNs,
                                       inputPublicationSequence);
        previousOwnerAdvanceSteadyNs_ = std::max(previousOwnerAdvanceSteadyNs_,
                                                  ownerAdvanceSteadyNs);
        IngestCommands(input, false);
    }
    else
    {
        if (previousOwnerAdvanceSteadyNs_ != 0u)
        {
            ClearScheduledCombatEdges(true);
            pendingAttackCommands_ = 0u;
            pendingParryCommands_ = 0u;
            pendingDodgeCommands_ = 0u;
        }
        previousOwnerAdvanceSteadyNs_ = 0u;
        IngestCommands(input);
    }
    if (ConsumeWorldCommand())
    {
        RefreshSnapshot(input);
        snapshot_.simulationTicksThisFrame = 0u;
        snapshot_.fixedStepAccumulatorSeconds = fixedStepRunner_.AccumulatorSeconds();
        snapshot_.catchUpOverrunCount = fixedStepRunner_.OverrunCount();
        return 0u;
    }
    if (input.paused)
    {
        combatContactTrace_.Clear();
        gameplayTimeScale_ = 1.0f;
        ClearQueuedDrawAttack();
        ClearScheduledCombatEdges(true);
        lastConsumedAttackSequence_ = std::max(lastConsumedAttackSequence_, latestAttackSequence_);
        pendingAttackCommands_ = 0u;
        lastConsumedParrySequence_ = std::max(lastConsumedParrySequence_, latestParrySequence_);
        pendingParryCommands_ = 0u;
        lastConsumedDodgeSequence_ = std::max(lastConsumedDodgeSequence_, latestDodgeSequence_);
        pendingDodgeCommands_ = 0u;
        lastConsumedInteractSequence_ += pendingInteractCommands_;
        pendingInteractCommands_ = 0u;
        lastConsumedToggleHeldLightPoseSequence_ += pendingToggleHeldLightPoseCommands_;
        pendingToggleHeldLightPoseCommands_ = 0u;
        lastConsumedRunToggleSequence_ = std::max(
            lastConsumedRunToggleSequence_, latestRunToggleSequence_);
        lastConsumedClearRunIntentSequence_ = std::max(
            lastConsumedClearRunIntentSequence_, latestClearRunIntentSequence_);
        pendingRunToggleCommands_ = 0u;
        pendingClearRunIntentCommands_ = 0u;
        ClearRunIntent();
        dodgeRemainingSeconds_ = 0.0f;
        CancelCombatTransients();
        walkVisualAmount_ = 0.0f;
        snapshot_.playerTravelledThisTick = 0.0f;
        playerFootsteps_.Reset();
        for (PlayerFootstepCadence& cadence : enemyFootsteps_)
        {
            cadence.Reset();
        }
        lanternPendulum_.Reset(
            RescueLanternHinge(),
            heldItemFixedStepState_.kinematics.rewardLanternPresentationYawRadians);
        lanternPendulumResetPending_ = true;
    }
    const std::uint32_t ticks = fixedStepRunner_.Advance(
        frameDeltaSeconds,
        input.paused,
        [this, &input, inputPublicationSequence](float fixedDeltaSeconds)
        {
            StepFixedTick(input, fixedDeltaSeconds, inputPublicationSequence);
        });
    // Ticked frames already resolved the final pose. Refresh only when a
    // zero-tick frame ages or cancels an existing presentation envelope.
    if (ticks == 0u && (presentationWasActive || presentationPoseDirty_))
    {
        // Synchronize zero-tick/paused frames to current input and viewport
        // without advancing a simulation step.
        ResolveHeldItems();
        ResolvePlayerAnimation(0.0f);
    }
    presentationPoseDirty_ = false;
    RefreshSnapshot(input);
    snapshot_.simulationTicksThisFrame = ticks;
    snapshot_.fixedStepAccumulatorSeconds = fixedStepRunner_.AccumulatorSeconds();
    snapshot_.catchUpOverrunCount = fixedStepRunner_.OverrunCount();
    snapshot_.eventsEmittedThisFrame = events_.Size() >= eventsBeforeFrame
        ? events_.Size() - eventsBeforeFrame
        : 0u;
    return ticks;
}

void GameSimulation::StepFixed(const InputSnapshot& input,
                               float fixedDeltaSeconds,
                               std::uint64_t inputPublicationSequence)
{
    // Direct deterministic stepping is also a valid presentation boundary.
    combatPresentation_.AdvanceFrame(fixedDeltaSeconds, input.paused);
    StepFixedTick(input, fixedDeltaSeconds, inputPublicationSequence, false);
}

void GameSimulation::StepFixedTick(const InputSnapshot& input,
                                   float fixedDeltaSeconds,
                                   std::uint64_t inputPublicationSequence,
                                   const bool activateTimestampedEdges)
{
    fixedDeltaSeconds = std::clamp(fixedDeltaSeconds, 0.0f, 0.05f);
    lastInput_ = input;
    inputPublicationSequence_ = inputPublicationSequence;
    ++tickIndex_;
    const std::size_t eventsBefore = events_.Size();
    snapshot_.eventsEmittedThisTick = 0u;

    IngestCommands(input);
    if (activateTimestampedEdges)
        ActivateScheduledCombatEdges();
    if (ConsumeWorldCommand())
    {
        RefreshSnapshot(input);
        snapshot_.eventsEmittedThisTick = 0u;
        return;
    }

    combatTeaching_.Update(CurrentCombatRules() && input.tutorialEnabled,
        input.tutorialSlowdownEnabled, input.paused ||
            playerVitals_.Snapshot().phase != PlayerLifePhase::Alive ||
            IsKeeperRevealing(lichEncounter_.Snapshot().revealPhase),
        combatSnapshot_, lichEncounter_.Snapshot(), activeEnemyKind_);
    gameplayTimeScale_ = combatTeaching_.Snapshot().simulationTimeScale;
    fixedDeltaSeconds *= gameplayTimeScale_;
    const bool wasAlive = playerVitals_.Snapshot().phase == PlayerLifePhase::Alive;
    playerVitals_.Update(fixedDeltaSeconds);
    const bool playerAlive = playerVitals_.Snapshot().phase == PlayerLifePhase::Alive;
    if (input.paused || !playerAlive)
    {
        ClearQueuedDrawAttack();
        if (!playerAlive && heldItems_[1].transition.active)
        {
            horde::gameplay::items::InterruptHeldItemTransition(heldItems_[1], tickIndex_);
            automaticSwordDrawBlocksDefense_ = false;
        }
        ClearScheduledCombatEdges(true);
        lastConsumedAttackSequence_ = std::max(lastConsumedAttackSequence_, latestAttackSequence_);
        pendingAttackCommands_ = 0u;
        lastConsumedParrySequence_ = std::max(lastConsumedParrySequence_, latestParrySequence_);
        pendingParryCommands_ = 0u;
        lastConsumedDodgeSequence_ = std::max(lastConsumedDodgeSequence_, latestDodgeSequence_);
        pendingDodgeCommands_ = 0u;
        lastConsumedInteractSequence_ += pendingInteractCommands_;
        pendingInteractCommands_ = 0u;
        lastConsumedToggleHeldLightPoseSequence_ += pendingToggleHeldLightPoseCommands_;
        pendingToggleHeldLightPoseCommands_ = 0u;
        lastConsumedRunToggleSequence_ = std::max(
            lastConsumedRunToggleSequence_, latestRunToggleSequence_);
        lastConsumedClearRunIntentSequence_ = std::max(
            lastConsumedClearRunIntentSequence_, latestClearRunIntentSequence_);
        pendingRunToggleCommands_ = 0u;
        pendingClearRunIntentCommands_ = 0u;
        ClearRunIntent();
        dodgeRemainingSeconds_ = 0.0f;
        CancelCombatTransients();
    }
    if (!input.paused && playerAlive)
    {
        walkTime_ += fixedDeltaSeconds;
        walkCycleTime_ += fixedDeltaSeconds;
        StepRescueJourney(fixedDeltaSeconds<=0);
        if (!rescueMovementSuppressedThisTick_)
            UpdateMovement(input, fixedDeltaSeconds);
        AdvanceSwordEquipment(fixedDeltaSeconds);
        const bool torchWasTriggered = torchFailureSnapshot_.triggered;
        torchFailureSnapshot_ = torchFailure_.Update(fixedDeltaSeconds,
                                           playerX_,
                                           playerZ_,
                                           playerYawRadians_,
                                           playerPitchRadians_);
        if (!torchWasTriggered && torchFailureSnapshot_.triggered)
        {
            Emit(GameplayEventType::TorchExtinguished,
                 EntityId::Player,
                 EntityId::Invalid,
                 torchFailureSnapshot_.droppedX,
                 torchFailureSnapshot_.droppedZ);
        }
        UpdateEncounters(input, fixedDeltaSeconds);
        UpdateRewardSequence(fixedDeltaSeconds, true);
        ResolveHeldItems();
        RecordSwordContactTrace();
        if (interactionState_.heldLightKind ==
            horde::gameplay::interactions::HeldLightKind::RewardLantern)
        {
            if (lanternPendulumResetPending_)
            {
                lanternPendulum_.Reset(
                    RescueLanternHinge(),
                    heldItemFixedStepState_.kinematics.rewardLanternPresentationYawRadians);
                lanternPendulumResetPending_ = false;
            }
            else
            {
                lanternPendulum_.StepFixed(
                    RescueLanternHinge(), fixedDeltaSeconds,
                    heldItemFixedStepState_.kinematics.rewardLanternPresentationYawRadians);
            }
        }
        else
        {
            lanternPendulum_.Reset(
                RescueLanternHinge(),
                heldItemFixedStepState_.kinematics.rewardLanternPresentationYawRadians);
            lanternPendulumResetPending_ = true;
        }
        ResolvePlayerAnimation(fixedDeltaSeconds);
        ResolveFireEmitters(fixedDeltaSeconds);
    }
    else
    {
        StepRescueJourney(true);
        snapshot_.playerTravelledThisTick = 0.0f;
        walkVisualAmount_ = 0.0f;
        playerFootsteps_.Reset();
        for (PlayerFootstepCadence& cadence : enemyFootsteps_)
        {
            cadence.Reset();
        }
        lanternPendulum_.Reset(
            RescueLanternHinge(),
            heldItemFixedStepState_.kinematics.rewardLanternPresentationYawRadians);
        lanternPendulumResetPending_ = true;
    }

    if (wasAlive && playerVitals_.Snapshot().phase != PlayerLifePhase::Alive)
    {
        ClearRunIntent();
        ClearQueuedDrawAttack();
        if (heldItems_[1].transition.active)
        {
            horde::gameplay::items::InterruptHeldItemTransition(heldItems_[1], tickIndex_);
            automaticSwordDrawBlocksDefense_ = false;
        }
        ClearScheduledCombatEdges(true);
        pendingAttackCommands_ = 0u;
        pendingParryCommands_ = 0u;
        pendingDodgeCommands_ = 0u;
        pendingInteractCommands_ = 0u;
        pendingToggleHeldLightPoseCommands_ = 0u;
        dodgeRemainingSeconds_ = 0.0f;
        CancelCombatTransients();
    }

    RefreshSnapshot(input);
    snapshot_.eventsEmittedThisTick = events_.Size() - eventsBefore;
}

void GameSimulation::SynchronizePausedInput(const InputSnapshot& input,
                                            const std::uint64_t inputPublicationSequence,
                                            const PausedInputPolicy policy)
{
    lastInput_ = input;
    lastInput_.paused = true;
    combatContactTrace_.Clear();
    ClearQueuedDrawAttack();
    combatPresentation_.Reset();
    ClearScheduledCombatEdges(false);
    previousOwnerAdvanceSteadyNs_ = 0u;
    inputPublicationSequence_ = std::max(inputPublicationSequence_,
                                         inputPublicationSequence);

    const auto synchronize = [](const std::uint64_t incoming,
                                std::uint64_t& latest,
                                std::uint64_t& consumed) {
        latest = std::max(latest, incoming);
        consumed = std::max(consumed, latest);
    };
    synchronize(input.commands.attack, latestAttackSequence_,
                lastConsumedAttackSequence_);
    synchronize(input.commands.parry, latestParrySequence_,
                lastConsumedParrySequence_);
    synchronize(input.commands.dodge, latestDodgeSequence_,
                lastConsumedDodgeSequence_);
    if (policy == PausedInputPolicy::PreserveWorldCommands)
    {
        pendingTutorialSkipCommands_ = SaturatingAdd(pendingTutorialSkipCommands_,
            SequenceDelta(input.commands.tutorialSkip, latestTutorialSkipSequence_));
        pendingTutorialReplayCommands_ = SaturatingAdd(pendingTutorialReplayCommands_,
            SequenceDelta(input.commands.tutorialReplay, latestTutorialReplaySequence_));
        pendingRouteResetCommands_ += SequenceDelta(input.commands.routeReset, latestRouteResetSequence_);
        pendingRetryCommands_ += SequenceDelta(input.commands.retry, latestRetrySequence_);
        latestRouteResetSequence_ = std::max(latestRouteResetSequence_, input.commands.routeReset);
        latestRetrySequence_ = std::max(latestRetrySequence_, input.commands.retry);
    }
    else
    {
        synchronize(input.commands.routeReset, latestRouteResetSequence_,
                    lastConsumedRouteResetSequence_);
        synchronize(input.commands.retry, latestRetrySequence_,
                    lastConsumedRetrySequence_);
        pendingRouteResetCommands_ = 0u;
        pendingRetryCommands_ = 0u;
        pendingTutorialSkipCommands_ = 0u;
        pendingTutorialReplayCommands_ = 0u;
    }
    latestTutorialSkipSequence_ = std::max(latestTutorialSkipSequence_, input.commands.tutorialSkip);
    latestTutorialReplaySequence_ = std::max(latestTutorialReplaySequence_, input.commands.tutorialReplay);
    synchronize(input.commands.interact, latestInteractSequence_,
                lastConsumedInteractSequence_);
    synchronize(input.commands.toggleHeldLightPose,
                latestToggleHeldLightPoseSequence_,
                lastConsumedToggleHeldLightPoseSequence_);
    synchronize(input.commands.runToggle, latestRunToggleSequence_,
                lastConsumedRunToggleSequence_);
    synchronize(input.commands.clearRunIntent, latestClearRunIntentSequence_,
                lastConsumedClearRunIntentSequence_);

    pendingAttackCommands_ = 0u;
    pendingParryCommands_ = 0u;
    pendingDodgeCommands_ = 0u;
    pendingInteractCommands_ = 0u;
    pendingToggleHeldLightPoseCommands_ = 0u;
    pendingRunToggleCommands_ = 0u;
    pendingClearRunIntentCommands_ = 0u;
    pendingDodgeForward_ = 0.0f;
    pendingDodgeStrafe_ = 0.0f;
    dodgeRemainingSeconds_ = 0.0f;
    CancelCombatTransients();
    ClearRunIntent();
    walkVisualAmount_ = 0.0f;
    playerFootsteps_.Reset();
    for (PlayerFootstepCadence& cadence : enemyFootsteps_)
        cadence.Reset();
    fixedStepRunner_.ResetAccumulator();
    lanternPendulum_.Reset(
        heldItemFixedStepState_.worldFromLeftHand,
        heldItemFixedStepState_.kinematics.rewardLanternPresentationYawRadians);
    lanternPendulumResetPending_ = true;
    events_.Clear();

    RefreshSnapshot(lastInput_);
    snapshot_.simulationTicksThisFrame = 0u;
    snapshot_.fixedStepAccumulatorSeconds = fixedStepRunner_.AccumulatorSeconds();
    snapshot_.catchUpOverrunCount = fixedStepRunner_.OverrunCount();
    snapshot_.eventsEmittedThisTick = 0u;
    snapshot_.eventsEmittedThisFrame = 0u;
    ResolveHeldItems();
    ResolvePlayerAnimation(0.0f);
    RefreshSnapshot(lastInput_);
}

void GameSimulation::SetPresentationAspect(const float logicalViewAspect)
{
    const float aspect = std::isfinite(logicalViewAspect) && logicalViewAspect > 0.0f
        ? std::clamp(logicalViewAspect, 0.25f, 4.0f)
        : 1.0f;
    if (aspect == presentationAspect_)
    {
        return;
    }

    presentationAspect_ = aspect;
    presentationPoseDirty_ = true;
    // Viewport changes affect only presentation kinematics. Re-resolve the
    // shared hand/item/animation pose immediately so zero-tick, paused, and
    // imported-checkpoint frames publish a coherent pose without advancing
    // simulation time or any gameplay authority.
    ResolveHeldItems();
    ResolvePlayerAnimation(0.0f);
    RefreshSnapshot(lastInput_);
}

void GameSimulation::ResetPlayerSupport()
{
    playerSupport_ = {};
}
void GameSimulation::ResolvePlayerSupport()
{
    if (config_.developmentRescueJourney && rescueTraversal_.IsActive()) return;
    if (UsesRescueExterior()) {
        playerSupport_ = horde::gameplay::traversal::RescueExteriorSupport(playerX_,playerZ_); return;
    }
    if (config_.developmentWorldRoute)
    {
        const auto support=ResolveWorldRouteSupport(playerX_,playerZ_);
        const auto projection=ProjectWorldRoute(playerX_,playerZ_);
        const auto zone=projection.valid?ZoneForRouteSegment(projection.segment):WorldZoneId::TombExterior;
        playerSupport_ = worldRoute_.readiness[static_cast<std::size_t>(zone)] == ZoneReadiness::Ready ? support : worldRoute_.safeSupport;
        // Reconstruction retains only the last admitted support. Movement is
        // blocked until the renderer completes the new resource generation.
        return;
    }
    playerSupport_ = ResolveDevelopmentPlayerSupport(playerX_, playerZ_,
        config_.developmentSupportFixture, supportGeneration_, supportGeneration_);
}
void GameSimulation::ResolveMovementCollision(float previousX, float previousZ)
{
    if (UsesRescueExterior()) {
        const auto support=horde::gameplay::traversal::RescueExteriorSupport(playerX_,playerZ_);
        const auto p=ProjectWorldRoute(playerX_,playerZ_);
        const auto zone=p.valid&&p.distance<=kWorldRouteHalfWidth?ZoneForRouteSegment(p.segment):WorldZoneId::TombExterior;
        const bool ready=worldRoute_.readiness[static_cast<std::size_t>(zone)]==ZoneReadiness::Ready;
        if(!support.grounded||!ready||std::abs(support.worldY-playerSupport_.worldY)>kWorldRouteMaximumStep) {
            playerX_=previousX;playerZ_=previousZ;worldRoute_.blocked=true;++worldRoute_.rollbackCount;
        } else {
            worldRoute_.safeX=playerX_;worldRoute_.safeZ=playerZ_;worldRoute_.safeSupport=support;worldRoute_.current=zone;worldRoute_.blocked=false;
        }
        return;
    }
    if (!config_.developmentWorldRoute)
    {
        ResolveCorridorPlayerCollision(previousX,previousZ,playerX_,playerZ_);
        return;
    }
    auto projection=ProjectWorldRoute(playerX_,playerZ_);
    if (projection.valid && projection.distance > kWorldRouteHalfWidth-kPlayerCollisionRadius)
    {
        const float scale=(kWorldRouteHalfWidth-kPlayerCollisionRadius)/projection.distance;
        playerX_=projection.x+(playerX_-projection.x)*scale;
        playerZ_=projection.z+(playerZ_-projection.z)*scale;
        projection=ProjectWorldRoute(playerX_,playerZ_);
    }
    const auto support=ResolveWorldRouteSupport(playerX_,playerZ_);
    const auto zone=projection.valid?ZoneForRouteSegment(projection.segment):WorldZoneId::TombExterior;
    const bool valid=projection.valid && support.grounded &&
        worldRoute_.readiness[static_cast<std::size_t>(zone)]==ZoneReadiness::Ready &&
        std::abs(support.worldY-playerSupport_.worldY)<=kWorldRouteMaximumStep;
    worldRoute_.blocked=!valid;
    if(!valid)
    {
        playerX_=worldRoute_.safeX; playerZ_=worldRoute_.safeZ;
        ++worldRoute_.rollbackCount;
    }
    else
    {
        worldRoute_.current=zone;
        worldRoute_.safeX=playerX_; worldRoute_.safeZ=playerZ_; worldRoute_.safeSupport=support;
    }
}
void GameSimulation::SetDevelopmentWorldRoute(bool enabled, bool stagedPreparation)
{
    config_.developmentWorldRoute=enabled;
    stagedWorldPreparation_=stagedPreparation;
    worldRoute_.Invalidate();
    if(enabled) { playerX_=worldRoute_.safeX; playerZ_=worldRoute_.safeZ; playerYawRadians_=3.14159265359f; }
    else { playerX_=config_.playerStartX; playerZ_=config_.playerStartZ; playerYawRadians_=config_.playerStartYawRadians; }
    ResetPlayerSupport(); ClearEvents(); ResolveHeldItems(); ResolvePlayerAnimation(0);
    ResolveFireEmitters(0); RefreshSnapshot(lastInput_);
}
bool GameSimulation::PublishWorldZoneReadiness(WorldZoneToken token, ZoneReadiness readiness)
{
    if((!config_.developmentWorldRoute && !config_.developmentRescueJourney) || !worldRoute_.Publish(token,readiness)) return false;
    RefreshSnapshot(lastInput_); return true;
}
void GameSimulation::InvalidateWorldZoneReadiness()
{
    if(config_.developmentRescueJourney) { RecoverRescueJourney(); return; }
    if(!config_.developmentWorldRoute) return;
    worldRoute_.Invalidate(true); playerX_=worldRoute_.safeX; playerZ_=worldRoute_.safeZ;
    playerSupport_=worldRoute_.safeSupport; ClearRunIntent(); ClearEvents(); ResolveHeldItems(); ResolvePlayerAnimation(0);
    ResolveFireEmitters(0); RefreshSnapshot(lastInput_);
}
void GameSimulation::SetDevelopmentSupportFixture(bool enabled, std::uint64_t generation)
{
    if (generation != 0u && generation < supportGeneration_) return;
    config_.developmentSupportFixture = enabled && generation != 0u;
    supportGeneration_ = std::max(supportGeneration_, generation);
    ResetPlayerSupport();
    ResolveHeldItems();
    lanternPendulum_.Reset(heldItemFixedStepState_.worldFromLeftHand,
        heldItemFixedStepState_.kinematics.rewardLanternPresentationYawRadians);
    lanternPendulumResetPending_ = true;
    ResolvePlayerAnimation(0.0f);
    ResolveFireEmitters(0.0f);
    RefreshSnapshot(lastInput_);
}

void GameSimulation::ResetRoute()
{
    if(config_.developmentRescueJourney) { rescueSavedSwordValid_=false;rescueTraversal_.Reset();rescueOpeningSeconds_=0;worldRoute_.Invalidate(); }
    legacyCombatCheckpoint_ = false;
    combatTeaching_.Reset(config_.combatFoundation1_7 && lastInput_.tutorialEnabled);
    if (!ApplyCheckpoint(0, false))
    {
        return;
    }

    if(config_.developmentWorldRoute) {
        playerX_=worldRoute_.safeX;playerZ_=worldRoute_.safeZ;
        playerYawRadians_=3.14159265359f;
    } else {
        playerX_ = FiniteOr(config_.playerStartX, kPlayerSpawn.x);
        playerZ_ = FiniteOr(config_.playerStartZ, kPlayerSpawn.z);
        if (!IsShowcasePlayerPositionWalkable(playerX_, playerZ_))
        {
            playerX_ = kPlayerSpawn.x;
            playerZ_ = kPlayerSpawn.z;
        }
        playerYawRadians_ = FiniteOr(config_.playerStartYawRadians, 0.0f);
    }
    playerPitchRadians_ = std::clamp(FiniteOr(config_.playerStartPitchRadians, 0.0f),
                                     kMinimumPitch,
                                     kMaximumPitch);
    swordCombat_.Reset(kSkeletonEnemyCapacity,
        config_.waterfallSkeletonEncounter
            ? kWaterfallSkeletonPairCenter
            : RoutePosition{0.0f, -4.65f},
        config_.waterfallSkeletonEncounter
            ? &kWaterfallSkeletonGuardSpawns
            : nullptr, CurrentCombatRules() ? 2 : 1);
    waterfallWarningEmitted_ = false;
    ResetSwordEquipment();
    combatPresentation_.Reset();
    skeletonIdlePhasesEnabled_ = true;
    combatSnapshot_ = swordCombat_.Update(0.0f,
                                           playerX_,
                                           playerZ_,
                                           playerYawRadians_,
                                           config_.waterfallSkeletonEncounter,
                                           config_.waterfallSkeletonEncounter &&
                                               IsWaterfallSkeletonArena(playerX_, playerZ_),
                                           config_.waterfallSkeletonEncounter);
    playerAnimationState_.Reset();
    ResolveHeldItems();
    lanternPendulum_.Reset(
        heldItemFixedStepState_.worldFromLeftHand,
        heldItemFixedStepState_.kinematics.rewardLanternPresentationYawRadians);
    lanternPendulumResetPending_ = true;
    ResolvePlayerAnimation(0.0f);
    for (std::size_t index = 0u; index < fireEmitterCount_; ++index)
        horde::gameplay::effects::ResetFireEmitter(fireEmitters_[index]);
    ResolveFireEmitters(0.0f);
    RefreshSnapshot(lastInput_);
}

void GameSimulation::RetryEncounter()
{
    if(config_.developmentRescueJourney && rescueTraversal_.Snapshot().claimCount) { RecoverRescueJourney();return; }
    legacyCombatCheckpoint_ = false;
    ApplyCheckpoint(retryCheckpoint_, true);
}

bool GameSimulation::ApplyShowcaseCheckpoint(std::int32_t checkpointId, bool countAsRetry)
{
    // Frozen evidence imports retain their historical contact/durability/cast rules.
    if (FindShowcaseCheckpoint(checkpointId) == nullptr) return false;
    legacyCombatCheckpoint_ = true;
    return ApplyCheckpoint(checkpointId, countAsRetry);
}

void GameSimulation::BeginCombatPractice(EnemyKind encounter)
{
    legacyCombatCheckpoint_ = false;
    combatTeaching_.Reset(config_.combatFoundation1_7 && lastInput_.tutorialEnabled);
    ApplyCheckpoint(encounter == EnemyKind::Lich ? 9 : 12,
                    encounter == EnemyKind::Lich);
    RefreshSnapshot(lastInput_);
}

bool GameSimulation::BeginMotionEvidenceEquipmentSeed()
{
    if (motionEvidenceEquipmentSeedActive_ ||
        config_.playerMountProfile != horde::gameplay::items::PlayerMountProfile::AnatomicalBody)
        return false;

    motionEvidencePreviousSwordStartsStowed_ = config_.swordStartsStowed;
    motionEvidencePreviousWaterfallSkeletonEncounter_ = config_.waterfallSkeletonEncounter;
    config_.swordStartsStowed = true;
    config_.waterfallSkeletonEncounter = true;
    if (!ApplyShowcaseCheckpoint(2, false))
    {
        config_.swordStartsStowed = motionEvidencePreviousSwordStartsStowed_;
        config_.waterfallSkeletonEncounter = motionEvidencePreviousWaterfallSkeletonEncounter_;
        return false;
    }
    motionEvidenceEquipmentSeedActive_ = true;
    return true;
}

void GameSimulation::EndMotionEvidenceEquipmentSeed()
{
    if (!motionEvidenceEquipmentSeedActive_) return;
    config_.swordStartsStowed = motionEvidencePreviousSwordStartsStowed_;
    config_.waterfallSkeletonEncounter = motionEvidencePreviousWaterfallSkeletonEncounter_;
    motionEvidenceEquipmentSeedActive_ = false;
    // Reset through the shared route path so temporary equipment and guards do
    // not leak into ordinary play. The route reset preserves monotonic floors.
    ResetRoute();
    ClearEvents();
}

void GameSimulation::ImportRewardCheckpoint(
    const horde::gameplay::interactions::ChestRewardSnapshot& chestReward,
    const horde::gameplay::interactions::InteractionState& interaction,
    const horde::gameplay::interactions::FinaleSequenceSnapshot& finale,
    const horde::gameplay::interactions::LanternPendulumSnapshot* pendulum)
{
    events_.Clear();
    combatPresentation_.Reset();
    dodgeRemainingSeconds_ = 0.0f;
    CancelCombatTransients();
    combatContactTrace_.Clear();
    const bool wasRaised = playerSupport_.worldY != kRouteFloorWorldY;
    if(config_.developmentWorldRoute) { worldRoute_.Invalidate(); playerX_=worldRoute_.safeX; playerZ_=worldRoute_.safeZ; }
    ResetPlayerSupport();
    chestRewardSequence_.Import(chestReward);
    interactionState_ = interaction;
    if(config_.developmentRescueJourney) {
        RestoreRescueEquipment();rescueTraversal_.Reset();worldRoute_.Invalidate();worldRoute_.current=WorldZoneId::Dungeon;
        rescueOpeningSeconds_=0;
        if(chestReward.phase==interactions::ChestRewardPhase::LanternClaimed) {
            rescueTraversal_.NotifyLanternClaimed();rescueTraversal_.DeployOwned();rescueOpeningSeconds_=1;
            playerX_=traversal::kLowerLanding.x;playerZ_=traversal::kLowerLanding.z;
        }
        ResetPlayerSupport();
    }
    finaleSequence_.Import(finale);
    pendingInteractCommands_ = 0u;
    pendingToggleHeldLightPoseCommands_ = 0u;
    ClearQueuedDrawAttack();
    finaleCompletionEmitted_ =
        finaleSequence_.Snapshot().endingPhase ==
        horde::gameplay::interactions::FinaleEndingPhase::Complete;
    ResolveHeldItems();
    lanternPendulum_.Reset(
        heldItemFixedStepState_.worldFromLeftHand,
        heldItemFixedStepState_.kinematics.rewardLanternPresentationYawRadians);
    if (pendulum != nullptr && !wasRaised)
    {
        lanternPendulum_.Import(*pendulum);
        lanternPendulumResetPending_ = false;
    }
    else
    {
        lanternPendulumResetPending_ = true;
    }
    ResolvePlayerAnimation(0.0f);
    ResolveFireEmitters(0.0f);
    RefreshSnapshot(lastInput_);
}

void GameSimulation::ResetTiming()
{
    ClearScheduledCombatEdges(true);
    previousOwnerAdvanceSteadyNs_ = 0u;
    pendingAttackCommands_ = 0u;
    pendingParryCommands_ = 0u;
    pendingDodgeCommands_ = 0u;
    ClearQueuedDrawAttack();
    fixedStepRunner_.ResetAccumulator();
}

void GameSimulation::ClearEvents()
{
    events_.Clear();
    RefreshSnapshot(lastInput_);
    snapshot_.eventsEmittedThisTick = 0u;
    snapshot_.eventsEmittedThisFrame = 0u;
}

EntityId GameSimulation::EntityForEnemy(EnemyKind kind)
{
    switch (kind)
    {
    case EnemyKind::Skeleton: return EntityId::SkeletonA;
    case EnemyKind::Lich: return EntityId::Lich;
    default: return EntityId::Invalid;
    }
}

void GameSimulation::IngestCommands(const InputSnapshot& input, const bool ingestCombatEdges)
{
    if (ingestCombatEdges)
    {
        pendingAttackCommands_ = SaturatingAdd(
            pendingAttackCommands_, SequenceDelta(input.commands.attack, latestAttackSequence_));
        pendingParryCommands_ = SaturatingAdd(
            pendingParryCommands_, SequenceDelta(input.commands.parry, latestParrySequence_));
    }
    const std::uint64_t dodgeDelta = SequenceDelta(input.commands.dodge, latestDodgeSequence_);
    if (ingestCombatEdges && dodgeDelta > 0u)
    {
        pendingDodgeCommands_ = SaturatingAdd(pendingDodgeCommands_, dodgeDelta);
        // Capture the coherent left-stick publication associated with the
        // button edge; releasing the stick before the next fixed tick cannot
        // change the requested dodge direction.
        pendingDodgeForward_ = std::clamp(FiniteOr(input.moveForward, 0.0f), -1.0f, 1.0f);
        pendingDodgeStrafe_ = std::clamp(FiniteOr(input.moveStrafe, 0.0f), -1.0f, 1.0f);
    }
    pendingRouteResetCommands_ = SaturatingAdd(pendingRouteResetCommands_, SequenceDelta(input.commands.routeReset, latestRouteResetSequence_));
    pendingRetryCommands_ = SaturatingAdd(pendingRetryCommands_, SequenceDelta(input.commands.retry, latestRetrySequence_));
    pendingInteractCommands_ = SaturatingAdd(pendingInteractCommands_, SequenceDelta(input.commands.interact, latestInteractSequence_));
    pendingToggleHeldLightPoseCommands_ = SaturatingAdd(pendingToggleHeldLightPoseCommands_, SequenceDelta(
        input.commands.toggleHeldLightPose, latestToggleHeldLightPoseSequence_));
    pendingRunToggleCommands_ = SaturatingAdd(pendingRunToggleCommands_,
        SequenceDelta(input.commands.runToggle, latestRunToggleSequence_));
    pendingClearRunIntentCommands_ = SaturatingAdd(pendingClearRunIntentCommands_,
        SequenceDelta(input.commands.clearRunIntent, latestClearRunIntentSequence_));
    pendingTutorialSkipCommands_ = SaturatingAdd(pendingTutorialSkipCommands_,
        SequenceDelta(input.commands.tutorialSkip, latestTutorialSkipSequence_));
    pendingTutorialReplayCommands_ = SaturatingAdd(pendingTutorialReplayCommands_,
        SequenceDelta(input.commands.tutorialReplay, latestTutorialReplaySequence_));
    latestTutorialSkipSequence_ = std::max(latestTutorialSkipSequence_, input.commands.tutorialSkip);
    latestTutorialReplaySequence_ = std::max(latestTutorialReplaySequence_, input.commands.tutorialReplay);
    latestAttackSequence_ = std::max(latestAttackSequence_, input.commands.attack);
    latestParrySequence_ = std::max(latestParrySequence_, input.commands.parry);
    latestDodgeSequence_ = std::max(latestDodgeSequence_, input.commands.dodge);
    latestRouteResetSequence_ = std::max(latestRouteResetSequence_, input.commands.routeReset);
    latestRetrySequence_ = std::max(latestRetrySequence_, input.commands.retry);
    latestInteractSequence_ = std::max(latestInteractSequence_, input.commands.interact);
    latestToggleHeldLightPoseSequence_ = std::max(
        latestToggleHeldLightPoseSequence_, input.commands.toggleHeldLightPose);
    latestRunToggleSequence_ = std::max(latestRunToggleSequence_,
                                        input.commands.runToggle);
    latestClearRunIntentSequence_ = std::max(latestClearRunIntentSequence_,
        input.commands.clearRunIntent);
}

void GameSimulation::AddCombatTimingTrace(
    const CombatInputEdge& edge,
    const std::uint64_t publicationSequence,
    const std::uint64_t targetTick,
    const CombatInputTimingDisposition disposition)
{
    CombatInputTimingSnapshot& timing = snapshot_.combatInputTiming;
    const std::uint32_t index = timing.nextTraceIndex;
    if (timing.traceCount == kCombatInputTimingTraceCapacity)
    {
        timing.traceOverwriteCount = SaturatingAdd(timing.traceOverwriteCount, 1u);
    }
    else
    {
        ++timing.traceCount;
    }
    timing.traces[index] = {
        edge.kind, disposition, CombatInputTimingStatus::Scheduled,
        edge.commandSequence, 1u, 0u, edge.steadyTimeNanoseconds,
        publicationSequence, targetTick, 0u, 0u, 0u};
    timing.nextTraceIndex =
        (index + 1u) % static_cast<std::uint32_t>(kCombatInputTimingTraceCapacity);
}

void GameSimulation::ScheduleTimestampedCombatEdges(
    const InputSnapshot& input,
    const double frameDeltaSeconds,
    const std::uint64_t ownerAdvanceSteadyNs,
    const std::uint64_t publicationSequence)
{
    snapshot_.combatInputTiming.inputHistoryOverwriteCount = input.combatEdgeHistory.overwriteCount;
    const std::array<CombatInputEdgeKind, 3u> kinds{{
        CombatInputEdgeKind::Attack,
        CombatInputEdgeKind::Parry,
        CombatInputEdgeKind::Dodge}};
    std::array<std::uint64_t, 3u> latest{{
        latestAttackSequence_, latestParrySequence_, latestDodgeSequence_}};
    std::array<std::uint64_t, 3u> deltas{};
    std::array<std::array<CombatInputEdge, kCombatInputEdgeHistoryCapacity>, 3u> found{};
    std::array<std::uint32_t, 3u> foundCount{};
    std::array<bool, 3u> covered{};
    for (std::size_t kindIndex = 0u; kindIndex < kinds.size(); ++kindIndex)
    {
        deltas[kindIndex] = SequenceDelta(SequenceFor(input.commands, kinds[kindIndex]),
                                          latest[kindIndex]);
        covered[kindIndex] = deltas[kindIndex] == 0u;
    }

    const CombatInputEdgeHistory& history = input.combatEdgeHistory;
    const std::uint32_t count = std::min<std::uint32_t>(
        history.count, static_cast<std::uint32_t>(kCombatInputEdgeHistoryCapacity));
    const std::uint32_t oldest = (history.nextIndex +
        static_cast<std::uint32_t>(kCombatInputEdgeHistoryCapacity) - count) %
        static_cast<std::uint32_t>(kCombatInputEdgeHistoryCapacity);
    for (std::uint32_t offset = 0u; offset < count; ++offset)
    {
        const CombatInputEdge& edge = history.edges[
            (oldest + offset) % static_cast<std::uint32_t>(kCombatInputEdgeHistoryCapacity)];
        for (std::size_t kindIndex = 0u; kindIndex < kinds.size(); ++kindIndex)
        {
            if (edge.kind == kinds[kindIndex] &&
                edge.commandSequence > latest[kindIndex] &&
                edge.commandSequence <= SequenceFor(input.commands, kinds[kindIndex]) &&
                foundCount[kindIndex] < kCombatInputEdgeHistoryCapacity)
            {
                found[kindIndex][foundCount[kindIndex]++] = edge;
            }
        }
    }

    if (previousOwnerAdvanceSteadyNs_ != 0u)
    {
        for (std::size_t kindIndex = 0u; kindIndex < kinds.size(); ++kindIndex)
        {
            if (deltas[kindIndex] == 0u || deltas[kindIndex] >
                kCombatInputEdgeHistoryCapacity || foundCount[kindIndex] != deltas[kindIndex])
                continue;
            std::uint64_t expected = latest[kindIndex];
            bool sequenceComplete = true;
            for (std::uint32_t i = 0u; i < foundCount[kindIndex]; ++i)
            {
                if (expected == UINT64_MAX || found[kindIndex][i].commandSequence != ++expected)
                {
                    sequenceComplete = false;
                    break;
                }
            }
            covered[kindIndex] = sequenceComplete;
        }
    }

    std::array<CombatInputEdge, kCombatInputEdgeHistoryCapacity> ordered{};
    std::uint32_t orderedCount = 0u;
    for (std::size_t kindIndex = 0u; kindIndex < kinds.size(); ++kindIndex)
    {
        if (covered[kindIndex] && previousOwnerAdvanceSteadyNs_ != 0u)
        {
            for (std::uint32_t i = 0u; i < foundCount[kindIndex]; ++i)
                ordered[orderedCount++] = found[kindIndex][i];
        }
        else if (deltas[kindIndex] != 0u)
        {
            const CombatInputTimingDisposition disposition = previousOwnerAdvanceSteadyNs_ == 0u
                ? CombatInputTimingDisposition::FirstTimestampFallback
                : CombatInputTimingDisposition::MissingMetadataFallback;
            CombatInputEdge fallback{};
            fallback.kind = kinds[kindIndex];
            fallback.commandSequence = SequenceFor(input.commands, kinds[kindIndex]);
            fallback.steadyTimeNanoseconds = 0u;
            std::uint64_t fallbackTarget = SaturatingAdd(tickIndex_, 1u);
            bool followsScheduledEdge = overflowCombatCommandCounts_[kindIndex] > 0u;
            if (followsScheduledEdge)
                fallbackTarget = std::max(fallbackTarget,
                    overflowCombatTargetTicks_[kindIndex]);
            for (std::uint32_t queued = 0u; queued < scheduledCombatEdgeCount_; ++queued)
            {
                if (scheduledCombatEdges_[queued].edge.kind == kinds[kindIndex])
                {
                    followsScheduledEdge = true;
                    fallbackTarget = std::max(fallbackTarget,
                        scheduledCombatEdges_[queued].targetTick);
                }
            }
            AddCombatTimingTrace(fallback, publicationSequence,
                                 fallbackTarget, disposition);
            CombatInputTimingTrace& trace = snapshot_.combatInputTiming.traces[
                (snapshot_.combatInputTiming.nextTraceIndex + kCombatInputTimingTraceCapacity - 1u) %
                kCombatInputTimingTraceCapacity];
            trace.commandCount = deltas[kindIndex];
            snapshot_.combatInputTiming.timestampFallbackCount = SaturatingAdd(
                snapshot_.combatInputTiming.timestampFallbackCount, deltas[kindIndex]);
            if (followsScheduledEdge)
            {
                overflowCombatCommandCounts_[kindIndex] = SaturatingAdd(
                    overflowCombatCommandCounts_[kindIndex], deltas[kindIndex]);
                overflowCombatTargetTicks_[kindIndex] = fallbackTarget;
                if (kinds[kindIndex] == CombatInputEdgeKind::Dodge)
                {
                    overflowDodgeForward_ = std::clamp(FiniteOr(input.moveForward, 0.0f), -1.0f, 1.0f);
                    overflowDodgeStrafe_ = std::clamp(FiniteOr(input.moveStrafe, 0.0f), -1.0f, 1.0f);
                }
            }
            else if (kinds[kindIndex] == CombatInputEdgeKind::Attack)
            {
                pendingAttackCommands_ = SaturatingAdd(pendingAttackCommands_, deltas[kindIndex]);
            }
            else if (kinds[kindIndex] == CombatInputEdgeKind::Parry)
            {
                pendingParryCommands_ = SaturatingAdd(pendingParryCommands_, deltas[kindIndex]);
            }
            else
            {
                pendingDodgeCommands_ = SaturatingAdd(pendingDodgeCommands_, deltas[kindIndex]);
                pendingDodgeForward_ = std::clamp(FiniteOr(input.moveForward, 0.0f), -1.0f, 1.0f);
                pendingDodgeStrafe_ = std::clamp(FiniteOr(input.moveStrafe, 0.0f), -1.0f, 1.0f);
            }
        }
    }

    // The history is globally ordered; a tiny insertion sort is bounded by 32.
    for (std::uint32_t i = 1u; i < orderedCount; ++i)
    {
        const CombatInputEdge value = ordered[i];
        std::uint32_t j = i;
        while (j > 0u && ordered[j - 1u].order > value.order)
        {
            ordered[j] = ordered[j - 1u];
            --j;
        }
        ordered[j] = value;
    }

    const double acceptedDelta = std::isfinite(frameDeltaSeconds)
        ? std::clamp(frameDeltaSeconds, 0.0,
                     FixedStepRunner::kMaximumFrameContributionSeconds)
        : 0.0;
    const double projected = fixedStepRunner_.AccumulatorSeconds() + acceptedDelta;
    const double projectedSteps = std::floor(
        (projected + 1.0e-12) / FixedStepRunner::kFixedDeltaSeconds);
    const std::uint32_t ticksProduced = static_cast<std::uint32_t>(std::clamp(
        projectedSteps, 0.0,
        static_cast<double>(FixedStepRunner::kMaximumStepsPerAdvance)));

    for (std::uint32_t i = 0u; i < orderedCount; ++i)
    {
        const CombatInputEdge& edge = ordered[i];
        const CombatInputScheduleResult schedule = ScheduleCombatInputEdge(
            edge.steadyTimeNanoseconds, previousOwnerAdvanceSteadyNs_,
            ownerAdvanceSteadyNs, frameDeltaSeconds,
            fixedStepRunner_.AccumulatorSeconds(), tickIndex_, ticksProduced);
        std::uint64_t targetTick = schedule.targetTick;
        for (std::uint32_t queued = 0u; queued < scheduledCombatEdgeCount_; ++queued)
        {
            const ScheduledCombatEdge& earlier = scheduledCombatEdges_[queued];
            if (earlier.edge.kind == edge.kind &&
                earlier.edge.commandSequence < edge.commandSequence)
                targetTick = std::max(targetTick, earlier.targetTick);
        }
        const std::size_t kindIndex = CombatKindIndex(edge.kind);
        if (overflowCombatCommandCounts_[kindIndex] > 0u)
            targetTick = std::max(targetTick, overflowCombatTargetTicks_[kindIndex]);

        CombatInputTimingDisposition disposition = schedule.disposition;
        if (scheduledCombatEdgeCount_ < scheduledCombatEdges_.size())
        {
            scheduledCombatEdges_[scheduledCombatEdgeCount_++] = {
                edge, targetTick, publicationSequence};
            AddCombatTimingTrace(edge, publicationSequence, targetTick, disposition);
        }
        else
        {
            disposition = CombatInputTimingDisposition::QueueOverflowFallback;
            const std::size_t kindIndex = CombatKindIndex(edge.kind);
            for (std::uint32_t queued = 0u; queued < scheduledCombatEdgeCount_; ++queued)
            {
                if (scheduledCombatEdges_[queued].edge.kind == edge.kind)
                    targetTick = std::max(targetTick, scheduledCombatEdges_[queued].targetTick);
            }
            if (overflowCombatCommandCounts_[kindIndex] > 0u)
                targetTick = std::max(targetTick, overflowCombatTargetTicks_[kindIndex]);
            AddCombatTimingTrace(edge, publicationSequence,
                                 targetTick, disposition);
            overflowCombatCommandCounts_[kindIndex] = SaturatingAdd(
                overflowCombatCommandCounts_[kindIndex], 1u);
            overflowCombatTargetTicks_[kindIndex] = targetTick;
            if (edge.kind == CombatInputEdgeKind::Dodge)
            {
                overflowDodgeForward_ = edge.moveForward;
                overflowDodgeStrafe_ = edge.moveStrafe;
            }
            snapshot_.combatInputTiming.timestampFallbackCount = SaturatingAdd(
                snapshot_.combatInputTiming.timestampFallbackCount, 1u);
        }
    }
    std::uint64_t scheduledCount = scheduledCombatEdgeCount_;
    for (const std::uint64_t count : overflowCombatCommandCounts_)
        scheduledCount = SaturatingAdd(scheduledCount, count);
    snapshot_.combatInputTiming.scheduledEdgeCount = static_cast<std::uint32_t>(
        std::min<std::uint64_t>(scheduledCount, UINT32_MAX));
}

void GameSimulation::ActivateScheduledCombatEdges()
{
    std::uint32_t retained = 0u;
    for (std::uint32_t i = 0u; i < scheduledCombatEdgeCount_; ++i)
    {
        const ScheduledCombatEdge& scheduled = scheduledCombatEdges_[i];
        if (scheduled.targetTick > tickIndex_)
        {
            scheduledCombatEdges_[retained++] = scheduled;
            continue;
        }
        if (scheduled.edge.kind == CombatInputEdgeKind::Attack)
            pendingAttackCommands_ = SaturatingAdd(pendingAttackCommands_, 1u);
        else if (scheduled.edge.kind == CombatInputEdgeKind::Parry)
            pendingParryCommands_ = SaturatingAdd(pendingParryCommands_, 1u);
        else
        {
            pendingDodgeCommands_ = SaturatingAdd(pendingDodgeCommands_, 1u);
            pendingDodgeForward_ = scheduled.edge.moveForward;
            pendingDodgeStrafe_ = scheduled.edge.moveStrafe;
        }
    }
    for (std::size_t kindIndex = 0u; kindIndex < overflowCombatCommandCounts_.size(); ++kindIndex)
    {
        const std::uint64_t count = overflowCombatCommandCounts_[kindIndex];
        if (count == 0u || overflowCombatTargetTicks_[kindIndex] > tickIndex_)
            continue;
        if (kindIndex == CombatKindIndex(CombatInputEdgeKind::Attack))
            pendingAttackCommands_ = SaturatingAdd(pendingAttackCommands_, count);
        else if (kindIndex == CombatKindIndex(CombatInputEdgeKind::Parry))
            pendingParryCommands_ = SaturatingAdd(pendingParryCommands_, count);
        else
        {
            pendingDodgeCommands_ = SaturatingAdd(pendingDodgeCommands_, count);
            pendingDodgeForward_ = overflowDodgeForward_;
            pendingDodgeStrafe_ = overflowDodgeStrafe_;
        }
        overflowCombatCommandCounts_[kindIndex] = 0u;
        overflowCombatTargetTicks_[kindIndex] = 0u;
    }
    scheduledCombatEdgeCount_ = retained;
    std::uint64_t scheduledCount = retained;
    for (const std::uint64_t count : overflowCombatCommandCounts_)
        scheduledCount = SaturatingAdd(scheduledCount, count);
    snapshot_.combatInputTiming.scheduledEdgeCount = static_cast<std::uint32_t>(
        std::min<std::uint64_t>(scheduledCount, UINT32_MAX));
}

void GameSimulation::ClearScheduledCombatEdges(const bool discardAndConsume)
{
    parrySourceCommandSequence_ = 0u;
    for (std::uint32_t i = 0u; i < snapshot_.combatInputTiming.traceCount; ++i)
    {
        const std::uint32_t index = (snapshot_.combatInputTiming.nextTraceIndex +
            static_cast<std::uint32_t>(kCombatInputTimingTraceCapacity) -
            snapshot_.combatInputTiming.traceCount + i) %
            static_cast<std::uint32_t>(kCombatInputTimingTraceCapacity);
        CombatInputTimingTrace& trace = snapshot_.combatInputTiming.traces[index];
        if (trace.status == CombatInputTimingStatus::Scheduled)
            trace.status = CombatInputTimingStatus::Discarded;
    }
    if (discardAndConsume)
    {
        lastConsumedAttackSequence_ = std::max(lastConsumedAttackSequence_, latestAttackSequence_);
        lastConsumedParrySequence_ = std::max(lastConsumedParrySequence_, latestParrySequence_);
        lastConsumedDodgeSequence_ = std::max(lastConsumedDodgeSequence_, latestDodgeSequence_);
    }
    scheduledCombatEdgeCount_ = 0u;
    scheduledCombatEdges_.fill({});
    overflowCombatCommandCounts_.fill(0u);
    overflowCombatTargetTicks_.fill(0u);
    snapshot_.combatInputTiming.scheduledEdgeCount = 0u;
}

void GameSimulation::MarkCombatTimingConsumed(const CombatInputEdgeKind kind,
                                               const std::uint64_t oldConsumed,
                                               const std::uint64_t newConsumed)
{
    if (newConsumed <= oldConsumed)
        return;
    for (std::uint32_t i = 0u; i < snapshot_.combatInputTiming.traceCount; ++i)
    {
        const std::uint32_t index = (snapshot_.combatInputTiming.nextTraceIndex +
            static_cast<std::uint32_t>(kCombatInputTimingTraceCapacity) -
            snapshot_.combatInputTiming.traceCount + i) %
            static_cast<std::uint32_t>(kCombatInputTimingTraceCapacity);
        CombatInputTimingTrace& trace = snapshot_.combatInputTiming.traces[index];
        if (trace.kind != kind || trace.status != CombatInputTimingStatus::Scheduled)
            continue;
        const std::uint64_t rangeStart = trace.commandCount > trace.commandSequence
            ? 1u : trace.commandSequence - trace.commandCount + 1u;
        const std::uint64_t overlapStart = std::max(
            rangeStart, SaturatingAdd(oldConsumed, 1u));
        const std::uint64_t overlapEnd = std::min(trace.commandSequence, newConsumed);
        if (overlapEnd >= overlapStart)
        {
            trace.consumedCount = SaturatingAdd(
                trace.consumedCount, overlapEnd - overlapStart + 1u);
            trace.actualTick = tickIndex_;
            if (trace.consumedCount >= trace.commandCount)
                trace.status = CombatInputTimingStatus::Consumed;
        }
    }
}

void GameSimulation::LinkCombatTimingSemanticEvent(
    const CombatInputEdgeKind kind,
    const std::uint64_t commandSequence,
    const std::uint64_t eventSequence,
    const std::uint64_t eventTick)
{
    if (commandSequence == 0u || eventSequence == 0u)
        return;
    for (std::uint32_t i = 0u; i < snapshot_.combatInputTiming.traceCount; ++i)
    {
        const std::uint32_t index = (snapshot_.combatInputTiming.nextTraceIndex +
            static_cast<std::uint32_t>(kCombatInputTimingTraceCapacity) -
            snapshot_.combatInputTiming.traceCount + i) %
            static_cast<std::uint32_t>(kCombatInputTimingTraceCapacity);
        CombatInputTimingTrace& trace = snapshot_.combatInputTiming.traces[index];
        if (trace.kind == kind && trace.commandCount == 1u &&
            trace.commandSequence == commandSequence)
        {
            trace.semanticEventSequence = eventSequence;
            trace.semanticEventTick = eventTick;
            return;
        }
    }
}

bool GameSimulation::ConsumeWorldCommand()
{
    if (pendingRouteResetCommands_ > 0u)
    {
        --pendingRouteResetCommands_;
        ++lastConsumedRouteResetSequence_;
        ResetRoute();
        return true;
    }
    if (pendingRetryCommands_ > 0u)
    {
        --pendingRetryCommands_;
        ++lastConsumedRetrySequence_;
        RetryEncounter();
        return true;
    }
    if (pendingTutorialReplayCommands_ > 0u)
    {
        pendingTutorialReplayCommands_ = 0u;
        pendingTutorialSkipCommands_ = 0u;
        ResetRoute();
        return true;
    }
    if (pendingTutorialSkipCommands_ > 0u)
    {
        pendingTutorialSkipCommands_ = 0u;
        combatTeaching_.Skip();
    }
    return false;
}

bool GameSimulation::ApplyCheckpoint(std::int32_t checkpointId, bool isRetry)
{
    const ShowcaseCheckpoint* checkpoint = FindShowcaseCheckpoint(checkpointId);
    if (checkpoint == nullptr)
    {
        return false;
    }

    pendingTutorialReplayCommands_ = 0u;
    pendingTutorialSkipCommands_ = 0u;
    gameplayTimeScale_ = 1.0f;
    ClearRunIntent();
    const auto encounterCheckpoint = ShowcaseCheckpointForEncounter(
        *checkpoint, config_.waterfallSkeletonEncounter);
    checkpoint = &encounterCheckpoint;
    ClearScheduledCombatEdges(true);

    events_.Clear();
    ResetPlayerSupport();
    combatContactTrace_.Clear();
    combatPresentation_.Reset();
    ShowcaseCheckpointState state = BuildShowcaseCheckpointState(*checkpoint);
    playerX_ = checkpoint->x;
    playerZ_ = checkpoint->z;
    playerYawRadians_ = checkpoint->yaw;
    playerPitchRadians_ = checkpoint->pitch;
    if(config_.developmentWorldRoute)
    {
        worldRoute_.Invalidate(); playerX_=worldRoute_.safeX; playerZ_=worldRoute_.safeZ;
        playerYawRadians_=3.14159265359f;
    }
    walkTime_ = 0.0f;
    walkCycleTime_ = 0.0f;
    walkVisualAmount_ = 0.0f;
    torchFailure_ = state.torchFailure;
    torchFailureSnapshot_ = torchFailure_.Snapshot();
    horde::gameplay::items::ImportHeldItemCheckpoint(
        heldItems_, torchFailureSnapshot_.heldByPlayer, tickIndex_);
    ResetSwordEquipment();
    waterfallWarningEmitted_ = false;
    enemyDirector_ = state.enemyDirector;
    if (config_.waterfallSkeletonEncounter && IsWaterfallSkeletonRoom(playerX_, playerZ_))
        enemyDirector_.Update(playerX_, playerZ_, EnemyKind::Skeleton);
    activeEnemyKind_ = enemyDirector_.Snapshot().selectedEnemy;
    lichEncounter_ = state.lichEncounter;
    lichEncounter_.SetReadableCombat(CurrentCombatRules());
    if (isRetry && !config_.developmentWorldRoute && checkpoint->preset == ShowcaseCheckpointPreset::LichActive)
    {
        playerX_ = kKeeperRetryPosition.x;
        playerZ_ = kKeeperRetryPosition.z;
        playerYawRadians_ = -1.57079632679f;
        playerPitchRadians_ = 0.0f;
        lichEncounter_.BeginRetryRecognition();
    }
    skeletonIdlePhasesEnabled_ = isRetry;
    skeletonIncidentalIdleSeconds_.fill(0.0f);
    skeletonIncidentalNextSeconds_ = {{12.0f, 18.0f}};
    skeletonIncidentalSpacingSeconds_ = 0.0f;
    lichAttackEligible_ = false;
    lichRevealAttackSequenceFloor_ = latestAttackSequence_;
    chestRewardSequence_ = state.chestRewardSequence;
    finaleSequence_ = state.finaleSequence;
    interactionState_ = state.interactionState;
    if (!torchFailureSnapshot_.heldByPlayer &&
        interactionState_.heldLightKind ==
            horde::gameplay::interactions::HeldLightKind::Torch)
    {
        interactionState_.heldLightKind =
            horde::gameplay::interactions::HeldLightKind::None;
    }
    const bool pairCheckpoint = checkpoint->preset == ShowcaseCheckpointPreset::TwoSkeletonCombat;
    // Explicit reset/retry/import owns the whole encounter state. Keep the
    // authored pair in its room even when a bay/Keeper checkpoint currently
    // selects the Lich; later room selection does not respawn enemies.
    const bool productionSkeletonEncounter = config_.waterfallSkeletonEncounter;
    const RoutePosition combatSpawnCenter = productionSkeletonEncounter
        ? kWaterfallSkeletonPairCenter
        : RoutePosition{0.0f, -4.65f};
    swordCombat_.Reset(productionSkeletonEncounter
                           ? kSkeletonEnemyCapacity
                           : ((isRetry || pairCheckpoint) ? kSkeletonEnemyCapacity : 1u),
                       combatSpawnCenter,
                       productionSkeletonEncounter ? &kWaterfallSkeletonGuardSpawns : nullptr,
                       CurrentCombatRules() ? 2 : 1);
    combatSnapshot_ = swordCombat_.Update(0.0f,
                                           playerX_,
                                           playerZ_,
                                           playerYawRadians_,
                                           config_.waterfallSkeletonEncounter,
                                           config_.waterfallSkeletonEncounter &&
                                               IsWaterfallSkeletonArena(playerX_, playerZ_),
                                           config_.waterfallSkeletonEncounter);
    const bool lichHasLineOfSight = !IsRouteAudioObstructed(playerX_,
                                                             playerZ_,
                                                             lichEncounter_.Snapshot().x,
                                                             lichEncounter_.Snapshot().z);
    const bool finaleActive = activeEnemyKind_ == EnemyKind::Lich &&
                              QueryShowcaseZone(playerX_, playerZ_) == ShowcaseZone::Finale;
    lichEncounter_.Update(0.0f,
                          playerX_,
                          playerZ_,
                          lichHasLineOfSight,
                          finaleActive);
    playerVitals_.ResetForEncounter();
    playerFootsteps_.Reset();
    for (PlayerFootstepCadence& cadence : enemyFootsteps_)
    {
        cadence.Reset();
    }
    // A reset/retry wins this tick, but every coherent monotonic edge that
    // arrived with it is still consumed exactly once. Advancing the consumed
    // sequences prevents pause/Home/ending polling from replaying a discarded
    // attack after the imported world state resumes.
    lastConsumedAttackSequence_ = std::max(lastConsumedAttackSequence_, latestAttackSequence_);
    lastConsumedParrySequence_ = std::max(lastConsumedParrySequence_, latestParrySequence_);
    lastConsumedDodgeSequence_ = std::max(lastConsumedDodgeSequence_, latestDodgeSequence_);
    lastConsumedInteractSequence_ += pendingInteractCommands_;
    lastConsumedToggleHeldLightPoseSequence_ += pendingToggleHeldLightPoseCommands_;
    pendingAttackCommands_ = 0u;
    pendingParryCommands_ = 0u;
    pendingDodgeCommands_ = 0u;
    pendingInteractCommands_ = 0u;
    pendingToggleHeldLightPoseCommands_ = 0u;
    dodgeRemainingSeconds_ = 0.0f;
    CancelCombatTransients();
    dodgeCooldownRemainingSeconds_ = 0.0f;
    retryCheckpoint_ = activeEnemyKind_ == EnemyKind::Lich ? 9 : 0;
    finaleCompletionEmitted_ = false;
    if (isRetry)
    {
        ++retryGeneration_;
    }
    fixedStepRunner_.ResetAccumulator();
    playerAnimationState_.Reset();
    ResolveHeldItems();
    lanternPendulum_.Reset(
        heldItemFixedStepState_.worldFromLeftHand,
        heldItemFixedStepState_.kinematics.rewardLanternPresentationYawRadians);
    lanternPendulumResetPending_ = true;
    ResolvePlayerAnimation(0.0f);
    for (std::size_t index = 0u; index < fireEmitterCount_; ++index)
        horde::gameplay::effects::ResetFireEmitter(fireEmitters_[index]);
    ResolveFireEmitters(0.0f);
    RefreshSnapshot(lastInput_);
    snapshot_.eventsEmittedThisTick = 0u;
    return true;
}

void GameSimulation::ResolveHeldItems()
{
    std::string diagnostic;
    const PlayerCombatSnapshot presentationCombat =
        combatPresentation_.PlayerCombatForPresentation(combatSnapshot_.player);
    const horde::gameplay::items::HeldItemFixedStepInput input{
        playerX_,
        playerZ_,
        config_.developmentRescueJourney && rescueTraversal_.Snapshot().equipmentStowed
            ?rescueTraversal_.Snapshot().bodyYawRadians:playerYawRadians_,
        config_.developmentRescueJourney && rescueTraversal_.Snapshot().equipmentStowed ?0.0f:playerPitchRadians_,
        walkTime_,
        walkVisualAmount_,
        torchFailureSnapshot_,
        presentationCombat,
        combatSnapshot_.swordSwingRadians,
        interactionState_,
        config_.playerMountProfile,
        &heldItems_[1],
        presentationAspect_,
        playerSupport_.worldY,config_.developmentWorldRoute || UsesRescueExterior()};
    // Simulation owns the transition/visual blend. Kinematics keeps a stable
    // hand-endpoint matrix for gameplay; PlayerRenderSlot composes its single
    // rendered matrix from that endpoint and the animated Hips mount.
    const horde::gameplay::items::HeldItemState swordAuthority = heldItems_[1];
    auto resolvedItems = heldItems_;
    // Every socket contract is a checked rigid transform. A failure would
    // indicate a source-code contract violation; preserve the last immutable
    // state rather than publishing a renderer-authored fallback.
    horde::gameplay::items::ResolveHeldItemsFixedStep(
        resolvedItems, input, tickIndex_, heldItemFixedStepState_, diagnostic);
    heldItems_[0] = resolvedItems[0];
    heldItems_[1] = swordAuthority;
    heldItems_[1].worldFromItem = resolvedItems[1].worldFromItem;
    heldItems_[1].worldFromDetach = resolvedItems[1].worldFromDetach;
    heldItems_[1].detachTick = resolvedItems[1].detachTick;
    ApplyRescuePresentation();
}

void GameSimulation::RecordSwordContactTrace()
{
    if (!CurrentCombatRules()) return;
    const auto action = combatSnapshot_.player.action;
    const auto cut = action == PlayerCombatAction::SwingActive
        ? PlayerAttackCut::DownwardCut
        : action == PlayerCombatAction::UpwardSliceActive
            ? PlayerAttackCut::UpwardSlice : PlayerAttackCut::None;
    if (cut == PlayerAttackCut::None) return;
    CombatContactSample sample;
    sample.tick = tickIndex_;
    sample.attackId = combatSnapshot_.activePlayerAttackId;
    sample.commandSequence = currentCutCommandSequence_;
    sample.consumedTick = currentCutConsumedTick_;
    sample.cut = cut; sample.phase = action;
    sample.phaseSeconds = combatSnapshot_.player.actionTime;
    sample.supportWorldY = playerSupport_.worldY;
    sample.target = swordHitTargetThisTick_;
    sample.semanticEventSequence = swordHitEventThisTick_;
    sample.outcome = swordHitEventThisTick_ != 0
        ? CombatContactOutcome::AcceptedSwordContact : CombatContactOutcome::NoContact;
    if (sample.target == EntityId::Lich)
    {
        sample.targetX = lichEncounter_.Snapshot().x;
        sample.targetZ = lichEncounter_.Snapshot().z;
    }
    else if (sample.target == EntityId::SkeletonA || sample.target == EntityId::SkeletonB)
    {
        const auto index = sample.target == EntityId::SkeletonA ? 0u : 1u;
        sample.targetX = combatSnapshot_.combatants[index].x;
        sample.targetZ = combatSnapshot_.combatants[index].z;
    }
    sample.hasSwordTransform = true;
    sample.worldFromSword = heldItems_[1].worldFromItem;
    combatContactTrace_.Push(sample);
}

bool GameSimulation::SwordDefenseReady() const
{
    const horde::gameplay::items::HeldItemState& sword = heldItems_[1];
    return sword.id == horde::gameplay::items::HeldItemId::Sword &&
           !sword.detached &&
           sword.parentMode == horde::gameplay::items::HeldItemParentMode::HandSocket &&
           !sword.transition.active;
}

bool GameSimulation::SwordDrawBlocksDefense() const
{
    const horde::gameplay::items::HeldItemState& sword = heldItems_[1];
    return automaticSwordDrawBlocksDefense_ && sword.transition.active &&
           sword.transition.kind == horde::gameplay::items::HeldItemTransitionKind::Draw;
}

bool GameSimulation::RequestSwordDraw(const std::int32_t reasonPayload,
                                     const bool blocksDefenseDuringDraw)
{
    using namespace horde::gameplay::items;
    if (!config_.swordStartsStowed || SwordDefenseReady())
        return false;

    HeldItemState& sword = heldItems_[1];
    const HeldItemTransitionRequestResult request = RequestHeldItemTransition(
        sword, HeldItemTransitionKind::Draw, tickIndex_);
    if (request.status != HeldItemTransitionRequestStatus::Started &&
        request.status != HeldItemTransitionRequestStatus::InterruptedAndStarted)
    {
        return false;
    }
    automaticSwordDrawBlocksDefense_ = blocksDefenseDuringDraw;
    Emit(GameplayEventType::PlayerSwordDrawStarted,
         EntityId::Player,
         EntityId::Invalid,
         playerX_,
         playerZ_,
         1.0f,
         reasonPayload);
    return true;
}

void GameSimulation::ResetSwordEquipment()
{
    using namespace horde::gameplay::items;
    HeldItemState& sword = heldItems_[1];
    const std::uint64_t sequence = sword.transition.semanticEdgeSequence;
    sword = MakeHeldItemState(
        HeldItemId::Sword, HeldHand::RightHand,
        config_.swordStartsStowed ? HeldItemParentMode::BodyStow
                                  : HeldItemParentMode::HandSocket);
    sword.transition.semanticEdgeSequence = sequence;
    automaticSwordDrawBlocksDefense_ = false;
    ClearQueuedDrawAttack();
}

void GameSimulation::ClearQueuedDrawAttack()
{
    queuedDrawAttack_ = false;
    queuedDrawAttackCommandSequence_ = 0u;
}

void GameSimulation::AdvanceSwordEquipment(const float fixedDeltaSeconds)
{
    const horde::gameplay::items::HeldItemTransitionAdvanceResult result =
        horde::gameplay::items::AdvanceHeldItemTransition(
            heldItems_[1], tickIndex_, fixedDeltaSeconds, false);
    if (result.attachmentChanged)
        EmitSwordAttachmentChange();
    if (!heldItems_[1].transition.active)
        automaticSwordDrawBlocksDefense_ = false;
}

void GameSimulation::EmitSwordAttachmentChange()
{
    Emit(GameplayEventType::PlayerSwordAttachmentChanged,
         EntityId::Player,
         EntityId::Invalid,
         playerX_,
         playerZ_,
         1.0f,
         static_cast<std::int32_t>(heldItems_[1].parentMode));
}

void GameSimulation::UpdateRewardSequence(const float deltaSeconds,
                                          const bool commandsAvailable)
{
    using namespace horde::gameplay::interactions;

    if (interactionState_.heldLightKind == HeldLightKind::Torch &&
        !torchFailureSnapshot_.heldByPlayer)
    {
        interactionState_.heldLightKind = HeldLightKind::None;
    }

    const InteractionQuery query{playerX_, playerZ_, playerYawRadians_};
    while (pendingInteractCommands_ > 0u)
    {
        --pendingInteractCommands_;
        ++lastConsumedInteractSequence_;
        if (!commandsAvailable)
        {
            continue;
        }
        if(config_.developmentRescueJourney && chestRewardSequence_.Snapshot().phase == ChestRewardPhase::LanternClaimed) {
            (void)TryRescueInteraction(); continue;
        }
        const ChestRewardAction action = chestRewardSequence_.TryInteract(query);
        if (action == ChestRewardAction::OpeningStarted)
        {
            Emit(GameplayEventType::ChestOpened,
                 EntityId::Player,
                 EntityId::RewardChest,
                 kRewardChestInteractionPosition.x,
                 kRewardChestInteractionPosition.z);
        }
        else if (action == ChestRewardAction::LanternClaimed)
        {
            EquipRewardLantern(interactionState_);
            finaleSequence_.NotifyLanternClaimed();
            if(config_.developmentRescueJourney) { rescueTraversal_.NotifyLanternClaimed();rescueTraversal_.DeployOwned(); }
            Emit(GameplayEventType::LanternClaimed,
                 EntityId::Player,
                 EntityId::RewardLantern,
                 kRewardChestInteractionPosition.x,
                 kRewardChestInteractionPosition.z);
        }
    }
    while (pendingToggleHeldLightPoseCommands_ > 0u)
    {
        --pendingToggleHeldLightPoseCommands_;
        ++lastConsumedToggleHeldLightPoseSequence_;
        if (commandsAvailable)
        {
            RequestHeldLightPoseToggle(interactionState_);
        }
    }

    const ChestRewardPhase chestPhaseBeforeUpdate =
        chestRewardSequence_.Snapshot().phase;
    chestRewardSequence_.Update(deltaSeconds);
    if (chestPhaseBeforeUpdate == ChestRewardPhase::Locked &&
        chestRewardSequence_.Snapshot().phase ==
            ChestRewardPhase::ClosedUnlocked)
    {
        Emit(GameplayEventType::ChestUnlocked,
             EntityId::Lich,
             EntityId::RewardChest,
             kRewardChestInteractionPosition.x,
             kRewardChestInteractionPosition.z);
    }
    AdvanceHeldLightPose(interactionState_, deltaSeconds);
    finaleSequence_.Update(deltaSeconds);

    if (!config_.developmentRescueJourney && finaleSequence_.Snapshot().endingPhase ==
            horde::gameplay::interactions::FinaleEndingPhase::Complete &&
        !finaleCompletionEmitted_)
    {
        finaleCompletionEmitted_ = true;
        Emit(GameplayEventType::FinaleCompleted,
             EntityId::Player,
             EntityId::Lich,
             playerX_,
             playerZ_);
    }
}

void GameSimulation::ResolvePlayerAnimation(const float fixedDeltaSeconds)
{
    const bool carryingOriginalTorch =
        interactionState_.heldLightKind ==
            horde::gameplay::interactions::HeldLightKind::Torch &&
        torchFailureSnapshot_.heldByPlayer;
    const float leftArmWeight = interactionState_.heldLightKind ==
        horde::gameplay::interactions::HeldLightKind::RewardLantern
        ? 1.0f
        : 1.0f - std::clamp(torchFailureSnapshot_.leftArmLowerBlend, 0.0f, 1.0f);
    playerAnimationState_.StepFixed(
        {walkVisualAmount_,
         walkCycleTime_,
         combatPresentation_.PlayerCombatForPresentation(combatSnapshot_.player),
         heldItemFixedStepState_.kinematics,
         leftArmWeight,
         interactionState_.heldLightKind == horde::gameplay::interactions::HeldLightKind::RewardLantern,
         lanternPendulum_.Snapshot().forwardAngleRadians,
         lanternPendulum_.Snapshot().strafeAngleRadians,
         carryingOriginalTorch},
        fixedDeltaSeconds);

}

void GameSimulation::ResolveFireEmitters(const float fixedDeltaSeconds)
{
    horde::gameplay::effects::StepFireEmitterFixed(
        fireEmitters_[0],
        {heldItemFixedStepState_.light.worldFromFlame,
         heldItemFixedStepState_.light.worldFromLight,
         torchFailureSnapshot_.flameStrength,
         1.0f,
         QueryShowcaseZone(playerX_, playerZ_)},
        fixedDeltaSeconds);

    // Resolve after the existing encounter update: the triggering reveal tick
    // publishes its event, movement hold and both flames in the same snapshot.
    // Death is still visible after health reaches zero, including the earlier
    // chest unlock. Only the actual Dead clip completion extinguishes them.
    const auto& keeper = lichEncounter_.Snapshot();
    const float strength = keeper.revealStarted && !keeper.deathAnimationComplete ? 1.0f : 0.0f;
    for (std::size_t index = 0u; index < effects::kKeeperTorchAnchors.size(); ++index)
    {
        const auto worldFromItem = effects::KeeperTorchWorldFromItem(effects::kKeeperTorchAnchors[index]);
        effects::StepFireEmitterFixed(
            fireEmitters_[index + 1u],
            {items::MultiplyHeldItemTransforms(worldFromItem, items::OriginalTorchFlameSocketTransform()),
             items::MultiplyHeldItemTransforms(worldFromItem, items::OriginalTorchLightSocketTransform()),
             strength, 1.0f, ShowcaseZone::Finale},
            fixedDeltaSeconds);
    }
}

void GameSimulation::ClearRunIntent()
{
    runToggleActive_ = false;
    runActive_ = false;
    playerMovementSpeed_ = 0.0f;
    runInputBlockedUntilRelease_ = true;
    pendingRunToggleCommands_ = 0u;
    pendingClearRunIntentCommands_ = 0u;
    lastConsumedRunToggleSequence_ = std::max(
        lastConsumedRunToggleSequence_, latestRunToggleSequence_);
    lastConsumedClearRunIntentSequence_ = std::max(
        lastConsumedClearRunIntentSequence_, latestClearRunIntentSequence_);
}

void GameSimulation::UpdateMovement(const InputSnapshot& input, float deltaSeconds)
{
    playerYawRadians_ = FiniteOr(input.yawRadians, playerYawRadians_);
    playerPitchRadians_ = std::clamp(FiniteOr(input.pitchRadians, playerPitchRadians_),
                                     kMinimumPitch,
                                     kMaximumPitch);
    const float previousX = playerX_;
    const float previousZ = playerZ_;

    if (!input.runHeld)
        runInputBlockedUntilRelease_ = false;
    if (pendingClearRunIntentCommands_ > 0u)
    {
        ClearRunIntent();
    }
    else if (pendingRunToggleCommands_ > 0u)
    {
        if ((pendingRunToggleCommands_ & 1u) != 0u)
            runToggleActive_ = !runToggleActive_;
        lastConsumedRunToggleSequence_ = SaturatingAdd(
            lastConsumedRunToggleSequence_, pendingRunToggleCommands_);
        pendingRunToggleCommands_ = 0u;
    }

    dodgeCooldownRemainingSeconds_ = std::max(
        0.0f, dodgeCooldownRemainingSeconds_ - deltaSeconds);
    if (IsKeeperRevealing(lichEncounter_.Snapshot().revealPhase))
    {
        // Looking and lifecycle commands remain responsive. Translation and
        // dodge edges cannot escape or queue behind the presentation hold.
        const std::uint64_t oldDodgeConsumed = lastConsumedDodgeSequence_;
        lastConsumedDodgeSequence_ = SaturatingAdd(lastConsumedDodgeSequence_, pendingDodgeCommands_);
        MarkCombatTimingConsumed(CombatInputEdgeKind::Dodge, oldDodgeConsumed,
                                 lastConsumedDodgeSequence_);
        pendingDodgeCommands_ = 0u;
        dodgeRemainingSeconds_ = 0.0f;
        CancelCombatTransients();
        ClearRunIntent();
        snapshot_.playerTravelledThisTick = 0.0f;
        walkVisualAmount_ = 0.0f;
        playerFootsteps_.Reset();
        return;
    }
    if (pendingDodgeCommands_ > 0u)
    {
        const std::uint64_t oldDodgeConsumed = lastConsumedDodgeSequence_;
        lastConsumedDodgeSequence_ = SaturatingAdd(lastConsumedDodgeSequence_, pendingDodgeCommands_);
        MarkCombatTimingConsumed(CombatInputEdgeKind::Dodge, oldDodgeConsumed,
                                 lastConsumedDodgeSequence_);
        pendingDodgeCommands_ = 0u;
        if (dodgeRemainingSeconds_ <= 0.0f && dodgeCooldownRemainingSeconds_ <= 0.0f)
        {
            float forward = pendingDodgeForward_;
            float strafe = pendingDodgeStrafe_;
            float magnitude = std::hypot(forward, strafe);
            if (magnitude < 0.16f)
            {
                forward = 1.0f;
                strafe = 0.0f;
                magnitude = 1.0f;
            }
            forward /= magnitude;
            strafe /= magnitude;
            const float forwardX = std::sin(playerYawRadians_);
            const float forwardZ = -std::cos(playerYawRadians_);
            const float rightX = std::cos(playerYawRadians_);
            const float rightZ = std::sin(playerYawRadians_);
            dodgeDirectionX_ = forwardX * forward + rightX * strafe;
            dodgeDirectionZ_ = forwardZ * forward + rightZ * strafe;
            dodgeRemainingSeconds_ = kDodgeDurationSeconds;
            dodgeProtectionAccepted_ = CurrentCombatRules();
            acceptedDodgeSequence_ = lastConsumedDodgeSequence_;
            acceptedDodgeConsumedTick_ = tickIndex_;
            dodgeCooldownRemainingSeconds_ = kDodgeCooldownSeconds;
            ClearRunIntent();
        }
    }

    // A combat action takes priority over run intent. A held key must be
    // released before it can request run again after the action completes.
    const bool actionInterruptsRun = input.hasAuthoritativePlayerPose ||
        combatSnapshot_.player.action != PlayerCombatAction::Idle;
    if (actionInterruptsRun)
        ClearRunIntent();

    const bool runRequested = !runInputBlockedUntilRelease_ &&
        (runToggleActive_ || input.runHeld) && !actionInterruptsRun;
    runActive_ = false;

    float movementIntent = 0.0f;
    if (input.hasAuthoritativePlayerPose)
    {
        playerX_ = FiniteOr(input.authoritativePlayerX, playerX_);
        playerZ_ = FiniteOr(input.authoritativePlayerZ, playerZ_);
        playerMovementSpeed_ = 0.0f;
        dodgeRemainingSeconds_ = 0.0f;
        CancelCombatTransients();
    }
    else
    {
        if (dodgeRemainingSeconds_ > 0.0f)
        {
            const float dodgeStepSeconds = std::min(deltaSeconds, dodgeRemainingSeconds_);
            const float dodgeSpeed = kDodgeDistanceMetres / kDodgeDurationSeconds;
            playerX_ += dodgeDirectionX_ * dodgeSpeed * dodgeStepSeconds;
            playerZ_ += dodgeDirectionZ_ * dodgeSpeed * dodgeStepSeconds;
            dodgeRemainingSeconds_ = std::max(0.0f, dodgeRemainingSeconds_ - dodgeStepSeconds);
            movementIntent = 1.0f;
            playerMovementSpeed_ = 0.0f;
        }
        else
        {
            float forward = std::clamp(FiniteOr(input.moveForward, 0.0f), -1.0f, 1.0f);
            float strafe = std::clamp(FiniteOr(input.moveStrafe, 0.0f), -1.0f, 1.0f);
            const float magnitude = std::sqrt(forward * forward + strafe * strafe);
            movementIntent = std::min(1.0f, magnitude);
            if (magnitude > 0.0001f)
            {
                forward /= magnitude;
                strafe /= magnitude;
            }

            const float forwardX = std::sin(playerYawRadians_);
            const float forwardZ = -std::cos(playerYawRadians_);
            const float rightX = std::cos(playerYawRadians_);
            const float rightZ = std::sin(playerYawRadians_);
            const float maximumSpeed = runRequested
                ? kRunSpeedMetresPerSecond
                : config_.movementSpeedMetresPerSecond;
            const float targetSpeed = magnitude > 0.001f
                ? maximumSpeed * std::min(1.0f, magnitude)
                : 0.0f;
            const bool rampMovement = runRequested || config_.developmentWorldRoute ||
                playerMovementSpeed_ > config_.movementSpeedMetresPerSecond;
            if (rampMovement)
            {
                playerMovementSpeed_ += std::clamp(
                    targetSpeed - playerMovementSpeed_,
                    -kMovementAccelerationMetresPerSecondSquared * deltaSeconds,
                     kMovementAccelerationMetresPerSecondSquared * deltaSeconds);
            }
            else
            {
                // Keep ordinary, no-run 1.6.2 movement timing exact. Smooth
                // ramps are limited to the new run transition and opted-in
                // development route; coming down from run remains smooth.
                playerMovementSpeed_ = targetSpeed;
            }
            movementIntent = maximumSpeed > 0.001f
                ? std::clamp(playerMovementSpeed_ / maximumSpeed, 0.0f, 1.0f)
                : std::min(1.0f, magnitude); // Existing stationary pose diagnostics retain walk intent.
            const float travelledStep = playerMovementSpeed_ * deltaSeconds;
            playerX_ += (forwardX * forward + rightX * strafe) * travelledStep;
            playerZ_ += (forwardZ * forward + rightZ * strafe) * travelledStep;
        }
        ResolveMovementCollision(previousX, previousZ);
        const LichSnapshot& keeper = lichEncounter_.Snapshot();
        if (!config_.developmentWorldRoute &&
            !keeper.revealComplete && keeper.phase != LichPhase::Dead)
        {
            ResolveMovementAgainstCircle({keeper.x, keeper.z},
                kKeeperPresentationCollisionRadius + kPlayerCollisionRadius,
                previousX, previousZ, playerX_, playerZ_);
            ResolveMovementCollision(previousX, previousZ);
        }
    }

    ResolvePlayerSupport();
    const float travelled = std::hypot(playerX_ - previousX, playerZ_ - previousZ);
    if (!input.hasAuthoritativePlayerPose && dodgeRemainingSeconds_ <= 0.0f &&
        (runRequested || config_.developmentWorldRoute ||
         playerMovementSpeed_ > config_.movementSpeedMetresPerSecond))
    {
        const float walkSpeed = std::max(config_.movementSpeedMetresPerSecond, 0.001f);
        // Phase follows collision-resolved path length. Blocked commanded
        // speed cannot create a faster leg stride against a wall.
        walkCycleTime_ += travelled / walkSpeed - deltaSeconds;
    }
    if (input.hasAuthoritativePlayerPose)
    {
        movementIntent = travelled > 0.00001f ? 1.0f : 0.0f;
    }
    snapshot_.playerTravelledThisTick = travelled;
    runActive_ = runRequested && travelled > 0.00001f && dodgeRemainingSeconds_ <= 0.0f;
    const float blend = std::clamp(deltaSeconds * 8.0f, 0.0f, 1.0f);
    walkVisualAmount_ += (movementIntent - walkVisualAmount_) * blend;
    if (playerFootsteps_.Update(travelled, movementIntent > 0.02f))
    {
        Emit(GameplayEventType::PlayerFootstep,
             EntityId::Player,
             EntityId::Invalid,
             playerX_,
             playerZ_);
    }
}

bool GameSimulation::CurrentCombatRules() const
{
    return config_.combatFoundation1_7 && !legacyCombatCheckpoint_;
}

void GameSimulation::CancelCombatTransients()
{
    dodgeProtectionAccepted_ = false;
    keeperRepelRemainingSeconds_ = 0.0f;
    combatTeaching_.CancelTransient();
    currentCutCommandSequence_ = currentCutConsumedTick_ = 0;
    queuedUpCutCommandSequence_ = queuedUpCutConsumedTick_ = 0;
    swordHitTargetThisTick_ = EntityId::Invalid;
    swordHitEventThisTick_ = 0;
}

void GameSimulation::BeginKeeperRepel()
{
    const auto& keeper = lichEncounter_.Snapshot();
    const float dx = playerX_ - keeper.x, dz = playerZ_ - keeper.z;
    const float distance = std::hypot(dx, dz);
    keeperRepelDirectionX_ = distance > 0.0001f ? dx / distance : -std::sin(playerYawRadians_);
    keeperRepelDirectionZ_ = distance > 0.0001f ? dz / distance : std::cos(playerYawRadians_);
    keeperRepelRemainingSeconds_ = LichEncounter::kRepelDuration;
    keeperRepelTravelledMetres_ = 0.0f;
    ClearRunIntent(); // Dodge/parry and looking remain available.
}

void GameSimulation::ApplyKeeperRepel(float deltaSeconds)
{
    if (keeperRepelRemainingSeconds_ <= 0.0f) return;
    if (!CurrentCombatRules() || lichEncounter_.Snapshot().phase == LichPhase::Dead ||
        playerVitals_.Snapshot().phase != PlayerLifePhase::Alive)
    { keeperRepelRemainingSeconds_ = 0.0f; return; }
    const float seconds = std::min(deltaSeconds, keeperRepelRemainingSeconds_);
    const float stepDistance = LichEncounter::kRepelDistance * seconds / LichEncounter::kRepelDuration;
    const float oldX = playerX_, oldZ = playerZ_, oldY = playerSupport_.worldY;
    const auto previousWorldRoute = worldRoute_;
    playerX_ += keeperRepelDirectionX_ * stepDistance;
    playerZ_ += keeperRepelDirectionZ_ * stepDistance;
    ResolveMovementCollision(oldX, oldZ);
    ResolvePlayerSupport();
    // Repel cannot force a support transition, fall, or height snap.
    if (!playerSupport_.grounded || std::abs(playerSupport_.worldY - oldY) > 0.0001f)
    {
        playerX_ = oldX; playerZ_ = oldZ;
        worldRoute_ = previousWorldRoute;
        ResolvePlayerSupport();
    }
    const float travelled = std::hypot(playerX_ - oldX, playerZ_ - oldZ);
    keeperRepelTravelledMetres_ += travelled;
    snapshot_.playerTravelledThisTick += travelled;
    keeperRepelRemainingSeconds_ = std::max(0.0f, keeperRepelRemainingSeconds_ - seconds);
}

void GameSimulation::UpdateEncounters(const InputSnapshot& input, float deltaSeconds)
{
    ApplyKeeperRepel(deltaSeconds);
    const auto encounterOverride = config_.waterfallSkeletonEncounter &&
        IsWaterfallSkeletonRoom(playerX_, playerZ_)
            ? EnemyKind::Skeleton : EnemyKind::None;
    enemyDirector_.Update(playerX_, playerZ_, encounterOverride);
    const EnemyKind selectedEnemy = enemyDirector_.Snapshot().selectedEnemy;
    if (selectedEnemy != activeEnemyKind_)
    {
        activeEnemyKind_ = selectedEnemy;
        if (!lichEncounter_.Snapshot().revealStarted)
            playerVitals_.ResetForEncounter();
        retryCheckpoint_ = activeEnemyKind_ == EnemyKind::Lich ? 9 : 0;
        // Opening enemies and the keeper persist through route selection.
        // Only explicit reset/retry/checkpoint import may initialise them.
    }

    if (config_.waterfallSkeletonEncounter &&
        activeEnemyKind_ == EnemyKind::Skeleton && !SwordDefenseReady())
    {
        const float distanceToWaterfall = std::hypot(
            playerX_ - kWaterfallSkeletonPairCenter.x,
            playerZ_ - kWaterfallSkeletonPairCenter.z);
        const bool inEarlyCueRange = distanceToWaterfall <= kWaterfallSwordCueRadius;
        const bool canSeeEncounter = !IsRouteAudioObstructed(
            playerX_, playerZ_,
            kWaterfallSkeletonPairCenter.x, kWaterfallSkeletonPairCenter.z);
        if (inEarlyCueRange && canSeeEncounter && !waterfallWarningEmitted_)
        {
            Emit(GameplayEventType::SkeletonEncounterWarning,
                 EntityId::SkeletonA,
                 EntityId::Player,
                 kWaterfallSkeletonPairCenter.x,
                 kWaterfallSkeletonPairCenter.z,
                 0.72f);
            waterfallWarningEmitted_ = true;
        }
        if ((inEarlyCueRange && canSeeEncounter) ||
            IsWaterfallSkeletonArena(playerX_, playerZ_))
        {
            // Semantic event only; audio selection remains an application
            // mapping decision. Arena fallback covers an occluded approach.
            RequestSwordDraw(3, true);
        }
    }

    const bool finaleActive = QueryShowcaseZone(playerX_, playerZ_) == ShowcaseZone::Finale;
    const auto& keeperBeforeActions = lichEncounter_.Snapshot();
    const bool keeperHoldsActions = IsKeeperRevealing(keeperBeforeActions.revealPhase) ||
        (!keeperBeforeActions.revealStarted && activeEnemyKind_ == EnemyKind::Lich &&
         finaleActive && torchFailureSnapshot_.phase == TorchFailurePhase::Settled &&
         HasReachedKeeperArrivalThreshold(playerX_, playerZ_));
    if (keeperHoldsActions)
    {
        // Include the triggering and final reveal ticks, so neither an old cut
        // nor any buffered input becomes a swing at the combat boundary.
        const std::uint64_t oldAttackConsumed = lastConsumedAttackSequence_;
        lastConsumedAttackSequence_ = SaturatingAdd(lastConsumedAttackSequence_, pendingAttackCommands_);
        MarkCombatTimingConsumed(CombatInputEdgeKind::Attack, oldAttackConsumed,
                                 lastConsumedAttackSequence_);
        pendingAttackCommands_ = 0u;
        const std::uint64_t oldParryConsumed = lastConsumedParrySequence_;
        lastConsumedParrySequence_ = SaturatingAdd(lastConsumedParrySequence_, pendingParryCommands_);
        MarkCombatTimingConsumed(CombatInputEdgeKind::Parry, oldParryConsumed,
                                 lastConsumedParrySequence_);
        pendingParryCommands_ = 0u;
        dodgeRemainingSeconds_ = 0.0f;
        CancelCombatTransients();
        swordCombat_.CancelPlayerActions();
        lichAttackEligible_ = false;
        lichRevealAttackSequenceFloor_ = latestAttackSequence_;
    }
    const bool parryAvailable = SwordDefenseReady() && swordCombat_.CanAcceptParry();
    bool playerActionAccepted = false;
    const auto acceptAttack = [&](const std::uint64_t commandSequence)
    {
        if (!SwordDefenseReady())
            return false;
        const PlayerAttackCut acceptedCut = swordCombat_.RequestAttack();
        if (acceptedCut == PlayerAttackCut::None)
            return false;
        if (acceptedCut == PlayerAttackCut::DownwardCut)
        {
            currentCutCommandSequence_ = commandSequence;
            currentCutConsumedTick_ = tickIndex_;
            queuedUpCutCommandSequence_ = queuedUpCutConsumedTick_ = 0;
            lichAttackEligible_ = lichEncounter_.Snapshot().revealComplete &&
                commandSequence > lichRevealAttackSequenceFloor_;
        }
        else
        {
            // A buffered continuation must not relabel the still-running
            // downward cut. Promote its source only when the upward cut starts.
            queuedUpCutCommandSequence_ = commandSequence;
            queuedUpCutConsumedTick_ = tickIndex_;
            lichAttackEligible_ = lichAttackEligible_ &&
                lichEncounter_.Snapshot().revealComplete &&
                commandSequence > lichRevealAttackSequenceFloor_;
        }
        const std::uint64_t eventSequence = Emit(
            GameplayEventType::PlayerSwing,
            EntityId::Player,
            EntityId::Invalid,
            playerX_,
            playerZ_,
            1.0f,
            static_cast<std::int32_t>(acceptedCut));
        LinkCombatTimingSemanticEvent(CombatInputEdgeKind::Attack,
                                     commandSequence,
                                     eventSequence, tickIndex_);
        return true;
    };
    if (queuedDrawAttack_ && SwordDefenseReady() && !keeperHoldsActions)
    {
        const std::uint64_t queuedSequence = queuedDrawAttackCommandSequence_;
        ClearQueuedDrawAttack();
        playerActionAccepted = acceptAttack(queuedSequence);
    }
    if (pendingAttackCommands_ > 0u)
    {
        // Consume one monotonic edge per fixed tick. A coherent publication
        // may legitimately jump 0 -> 2 when both clicks arrive between ticks;
        // collapsing the whole delta into one RequestAttack silently erased
        // the upward continuation.
        --pendingAttackCommands_;
        const std::uint64_t oldAttackConsumed = lastConsumedAttackSequence_;
        lastConsumedAttackSequence_ = SaturatingAdd(lastConsumedAttackSequence_, 1u);
        MarkCombatTimingConsumed(CombatInputEdgeKind::Attack, oldAttackConsumed,
                                 lastConsumedAttackSequence_);
        if (!SwordDefenseReady())
        {
            RequestSwordDraw(1, false);
            const auto& draw = heldItems_[1].transition;
            if (!queuedDrawAttack_ && draw.active &&
                draw.kind == horde::gameplay::items::HeldItemTransitionKind::Draw)
            {
                queuedDrawAttack_ = true;
                queuedDrawAttackCommandSequence_ = lastConsumedAttackSequence_;
            }
        }
        else if (!playerActionAccepted)
        {
            playerActionAccepted = acceptAttack(lastConsumedAttackSequence_);
        }
    }
    if (pendingParryCommands_ > 0u)
    {
        const std::uint64_t oldParryConsumed = lastConsumedParrySequence_;
        lastConsumedParrySequence_ = SaturatingAdd(lastConsumedParrySequence_, pendingParryCommands_);
        MarkCombatTimingConsumed(CombatInputEdgeKind::Parry, oldParryConsumed,
                                 lastConsumedParrySequence_);
        pendingParryCommands_ = 0u;
        if (!SwordDefenseReady())
        {
            if (!playerActionAccepted)
                RequestSwordDraw(2, false);
            // Consume and discard this edge. A parry pressed before the hand
            // attachment edge cannot become a later, buffered defense.
        }
        else if (parryAvailable && !playerActionAccepted)
        {
            swordCombat_.RequestParry();
            parrySourceCommandSequence_ = lastConsumedParrySequence_;
        }
    }

    const auto previousSkeletons = combatSnapshot_.combatants;
    swordHitTargetThisTick_ = EntityId::Invalid;
    swordHitEventThisTick_ = 0;
    combatSnapshot_ = swordCombat_.Update(deltaSeconds,
                                           playerX_,
                                           playerZ_,
                                           playerYawRadians_,
                                           config_.waterfallSkeletonEncounter,
                                           config_.waterfallSkeletonEncounter &&
                                               IsWaterfallSkeletonArena(playerX_, playerZ_),
                                           config_.waterfallSkeletonEncounter);
    EntityId skeletonDamageSource = EntityId::Invalid;
    if (queuedUpCutCommandSequence_ != 0 &&
        (combatSnapshot_.player.action == PlayerCombatAction::UpwardSliceWindup ||
         combatSnapshot_.player.action == PlayerCombatAction::UpwardSliceActive ||
         combatSnapshot_.player.action == PlayerCombatAction::UpwardSliceRecovery))
    {
        currentCutCommandSequence_ = queuedUpCutCommandSequence_;
        currentCutConsumedTick_ = queuedUpCutConsumedTick_;
        queuedUpCutCommandSequence_ = queuedUpCutConsumedTick_ = 0;
    }
    skeletonIncidentalSpacingSeconds_ = std::max(
        0.0, skeletonIncidentalSpacingSeconds_ - deltaSeconds);
    for (std::size_t index = 0u; index < combatSnapshot_.combatantCount; ++index)
    {
        const SkeletonCombatantSnapshot& previous = previousSkeletons[index];
        const SkeletonCombatantSnapshot& current = combatSnapshot_.combatants[index];
        const EntityId entity = SkeletonEntity(index);
        if (activeEnemyKind_ == EnemyKind::Skeleton &&
            previous.animation != EnemyAnimation::Attack &&
            current.animation == EnemyAnimation::Attack)
        {
            Emit(GameplayEventType::EnemyAttackStarted,
                 entity,
                 EntityId::Player,
                 current.x,
                 current.z);
        }
        if (activeEnemyKind_ == EnemyKind::Skeleton && current.health < previous.health)
        {
            swordHitTargetThisTick_ = entity;
            swordHitEventThisTick_ = Emit(GameplayEventType::EnemyHit,
                 EntityId::Player,
                 entity,
                 current.x,
                 current.z);
        }
        if (activeEnemyKind_ == EnemyKind::Skeleton && previous.health > 0 && current.health <= 0)
        {
            Emit(GameplayEventType::EnemyDefeated,
                 EntityId::Player,
                 entity,
                 current.x,
                 current.z);
        }
        if (CurrentCombatRules() && activeEnemyKind_ == EnemyKind::Skeleton &&
            current.action == EnemyCombatAction::AttackWindup &&
            previous.actionTime < CombatTimeline::kSkeletonAttackWindupSeconds - 0.26f &&
            current.actionTime >= CombatTimeline::kSkeletonAttackWindupSeconds - 0.26f)
            Emit(GameplayEventType::ParryPrepareCue, entity, EntityId::Player,
                 current.x, current.z, 0.45f);
        if (current.playerHitPulse)
        {
            skeletonDamageSource = entity;
        }
        if (current.parrySuccessPulse)
        {
            if (CurrentCombatRules()) combatTeaching_.ParrySucceeded();
            const std::uint64_t eventSequence = Emit(
                GameplayEventType::PlayerParrySucceeded,
                EntityId::Player,
                entity,
                current.x,
                 current.z);
            combatPresentation_.BeginParrySuccess(eventSequence, tickIndex_, entity);
            LinkCombatTimingSemanticEvent(CombatInputEdgeKind::Parry,
                                         parrySourceCommandSequence_,
                                         eventSequence, tickIndex_);
            parrySourceCommandSequence_ = 0u;
        }
        const bool skeletonWalking = activeEnemyKind_ == EnemyKind::Skeleton &&
                                     current.animation == EnemyAnimation::Walking;
        if (enemyFootsteps_[index].Update(deltaSeconds, skeletonWalking))
        {
            Emit(GameplayEventType::EnemyFootstep,
                 entity,
                 EntityId::Invalid,
                 current.x,
                 current.z);
        }
        const bool incidentalEligible = skeletonIdlePhasesEnabled_ &&
            activeEnemyKind_ == EnemyKind::Skeleton && current.health > 0 &&
            current.action == EnemyCombatAction::Locomotion &&
            current.animation == EnemyAnimation::Idle;
        if (incidentalEligible)
        {
            // Sparse semantic cues share simulation/lifecycle ownership with
            // combat. Idle accumulation never changes walking/contact clocks.
            skeletonIncidentalIdleSeconds_[index] += deltaSeconds;
            if (skeletonIncidentalSpacingSeconds_ <= 0.000001 &&
                skeletonIncidentalIdleSeconds_[index] + 0.000001 >=
                    skeletonIncidentalNextSeconds_[index])
            {
                Emit(GameplayEventType::SkeletonIncidental, entity,
                     EntityId::Invalid, current.x, current.z, 0.30f);
                skeletonIncidentalNextSeconds_[index] =
                    skeletonIncidentalIdleSeconds_[index] + 28.0f;
                skeletonIncidentalSpacingSeconds_ = 6.0f;
            }
        }
    }
    if (activeEnemyKind_ == EnemyKind::Skeleton && combatSnapshot_.encounterComplete)
    {
        enemyDirector_.MarkSelectedDead();
    }

    const LichSnapshot previousLich = lichEncounter_.Snapshot();
    const LichPhase previousLichPhase = previousLich.phase;
    if (!previousLich.revealComplete)
    {
        lichAttackEligible_ = false;
        lichRevealAttackSequenceFloor_ = latestAttackSequence_;
    }
    const bool lineOfSight = !IsRouteAudioObstructed(playerX_,
                                                      playerZ_,
                                                      lichEncounter_.Snapshot().x,
                                                      lichEncounter_.Snapshot().z);
    const LichSnapshot& lich = lichEncounter_.Update(
        activeEnemyKind_ == EnemyKind::Lich || IsKeeperRevealing(previousLich.revealPhase) ||
            previousLich.phase == LichPhase::Dead ? deltaSeconds : 0.0f,
        playerX_,
        playerZ_,
        lineOfSight,
        activeEnemyKind_ == EnemyKind::Lich && finaleActive,
        torchFailureSnapshot_.phase == TorchFailurePhase::Settled &&
            HasReachedKeeperArrivalThreshold(playerX_, playerZ_));

    if (!previousLich.revealStarted && lich.revealStarted)
    {
        Emit(GameplayEventType::KeeperRevealStarted, EntityId::Lich, EntityId::Player, lich.x, lich.z);
    }
    if (previousLich.revealPhase == KeeperRevealPhase::Awakening &&
        (lich.revealPhase == KeeperRevealPhase::Warning || lich.revealPhase == KeeperRevealPhase::Ready))
    {
        Emit(GameplayEventType::KeeperWarning, EntityId::Lich, EntityId::Player, lich.x, lich.z);
    }
    if (!previousLich.revealComplete && lich.revealComplete)
    {
        lichAttackEligible_ = false;
        lichRevealAttackSequenceFloor_ = latestAttackSequence_;
        Emit(GameplayEventType::KeeperCombatReady, EntityId::Lich, EntityId::Player, lich.x, lich.z);
    }

    if (activeEnemyKind_ == EnemyKind::Lich && lichAttackEligible_ && combatSnapshot_.playerAttackPulse &&
        !IsRouteAudioObstructed(playerX_, playerZ_, lich.x, lich.z) &&
        SwordCombat::IsPlayerTargetInRangeCone(playerX_,
                                                playerZ_,
                                                playerYawRadians_,
                                                lich.x,
                                                lich.z) &&
        lichEncounter_.TryAcceptPlayerHit(playerX_, playerZ_))
    {
        swordHitTargetThisTick_ = EntityId::Lich;
        swordHitEventThisTick_ = Emit(GameplayEventType::EnemyHit,
             EntityId::Player,
             EntityId::Lich,
             lich.x,
             lich.z);
        if (CurrentCombatRules() && lichEncounter_.Snapshot().phase == LichPhase::Repelling)
            BeginKeeperRepel();
        if (lichEncounter_.Snapshot().phase == LichPhase::Dead)
        {
            keeperRepelRemainingSeconds_ = 0.0f;
            Emit(GameplayEventType::LichDefeated,
                 EntityId::Player,
                 EntityId::Lich,
                 lich.x,
                 lich.z);
            if (finaleSequence_.NotifyLichDefeated())
            {
                chestRewardSequence_.BeginUnlockCountdown();
            }
        }
    }
    if (activeEnemyKind_ == EnemyKind::Lich &&
        previousLichPhase != LichPhase::Charging &&
        lichEncounter_.Snapshot().phase == LichPhase::Charging)
    {
        Emit(GameplayEventType::LichChargeStarted,
             EntityId::Lich,
             EntityId::Player,
             lich.x,
             lich.z,
             lich.staffLightStrength);
    }
    if (CurrentCombatRules() && activeEnemyKind_ == EnemyKind::Lich &&
        lichEncounter_.Snapshot().dischargeWarningPulse)
        Emit(GameplayEventType::LichDischargeWarning, EntityId::Lich, EntityId::Player,
             lich.x, lich.z, 0.45f);
    if (activeEnemyKind_ == EnemyKind::Lich && lichEncounter_.Snapshot().damagePulse)
    {
        Emit(GameplayEventType::LichImpact,
             EntityId::Lich,
             EntityId::Player,
             playerX_,
             playerZ_);
    }

    const bool playerDamagePulse =
        (activeEnemyKind_ == EnemyKind::Skeleton && skeletonDamageSource != EntityId::Invalid) ||
        (activeEnemyKind_ == EnemyKind::Lich && lichEncounter_.Snapshot().damagePulse);
    const bool keeperRevealInvulnerable = IsKeeperRevealing(previousLich.revealPhase) ||
        IsKeeperRevealing(lich.revealPhase);
    if (input.damageEnabled && playerDamagePulse && !keeperRevealInvulnerable &&
        !SwordDrawBlocksDefense())
    {
        const bool dodgeProtected = CurrentCombatRules() &&
            DodgeProtectionWindow::Contains(kDodgeDurationSeconds - dodgeRemainingSeconds_,
                dodgeProtectionAccepted_ && dodgeRemainingSeconds_ > 0.0f);
        PlayerDamageResult damageResult = PlayerDamageResult::Ignored;
        CombatContactSample damageSample;
        damageSample.tick = tickIndex_;
        if (dodgeRemainingSeconds_ > 0 && dodgeProtectionAccepted_)
        {
            damageSample.commandSequence = acceptedDodgeSequence_;
            damageSample.consumedTick = acceptedDodgeConsumedTick_;
        }
        damageSample.phase = combatSnapshot_.player.action;
        damageSample.phaseSeconds = combatSnapshot_.player.actionTime;
        damageSample.target = activeEnemyKind_ == EnemyKind::Skeleton
            ? skeletonDamageSource : EntityId::Lich;
        damageSample.supportWorldY = playerSupport_.worldY;
        damageSample.dodgeElapsedSeconds = dodgeRemainingSeconds_ > 0
            ? kDodgeDurationSeconds - dodgeRemainingSeconds_ : 0;
        if (activeEnemyKind_ == EnemyKind::Lich)
        {
            damageSample.targetX = lich.x;
            damageSample.targetZ = lich.z;
        }
        else
        {
            const auto index = skeletonDamageSource == EntityId::SkeletonA ? 0u : 1u;
            damageSample.targetX = combatSnapshot_.combatants[index].x;
            damageSample.targetZ = combatSnapshot_.combatants[index].z;
        }
        damageSample.outcome = CombatContactOutcome::PostHitProtected;
        if (dodgeProtected)
        {
            damageSample.outcome = CombatContactOutcome::DodgeProtected;
            ++dodgeProtectedHitCount_;
            if (playerVitals_.Snapshot().invulnerabilityRemaining <= 0.00001f)
                combatTeaching_.DodgeSucceeded();
        }
        else if (CurrentCombatRules() && combatTeaching_.Snapshot().safePractice)
        {
            damageSample.outcome = CombatContactOutcome::PracticeMiss;
            combatTeaching_.PracticeMiss();
        }
        else
            damageResult = playerVitals_.TryApplyDamage();
        if (damageResult == PlayerDamageResult::Damaged)
        {
            damageSample.outcome = CombatContactOutcome::DamageAccepted;
            damageSample.semanticEventSequence = Emit(GameplayEventType::PlayerDamaged,
                 activeEnemyKind_ == EnemyKind::Skeleton
                     ? skeletonDamageSource
                     : EntityForEnemy(activeEnemyKind_),
                 EntityId::Player,
                 playerX_,
                 playerZ_);
        }
        if (damageResult == PlayerDamageResult::Killed)
        {
            combatPresentation_.Reset();
            retryCheckpoint_ = activeEnemyKind_ == EnemyKind::Lich ? 9 : 0;
            pendingAttackCommands_ = 0u;
            damageSample.outcome = CombatContactOutcome::DamageAccepted;
            damageSample.semanticEventSequence = Emit(GameplayEventType::PlayerKilled,
                 activeEnemyKind_ == EnemyKind::Skeleton
                     ? skeletonDamageSource
                     : EntityForEnemy(activeEnemyKind_),
                 EntityId::Player,
                 playerX_,
                 playerZ_);
        }
        if (CurrentCombatRules()) combatContactTrace_.Push(damageSample);
    }

    combatTeaching_.Update(CurrentCombatRules() && input.tutorialEnabled,
        input.tutorialSlowdownEnabled, input.paused ||
            playerVitals_.Snapshot().phase != PlayerLifePhase::Alive ||
            IsKeeperRevealing(lichEncounter_.Snapshot().revealPhase),
        combatSnapshot_, lichEncounter_.Snapshot(), activeEnemyKind_);
    if (activeEnemyKind_ == EnemyKind::Lich && lichEncounter_.Snapshot().deathAnimationComplete)
    {
        enemyDirector_.MarkSelectedDead();
    }
}

std::uint64_t GameSimulation::Emit(GameplayEventType type,
                                   EntityId source,
                                   EntityId target,
                                   float x,
                                   float z,
                                   float intensity,
                                   std::int32_t payload)
{
    GameplayEvent event;
    event.tickIndex = tickIndex_;
    event.type = type;
    event.source = source;
    event.target = target;
    event.worldX = x;
    event.worldZ = z;
    event.listenerY = PlayerEyeWorldY(playerSupport_.worldY);
    event.listenerX = playerX_;
    event.listenerZ = playerZ_;
    event.listenerYawRadians = playerYawRadians_;
    event.intensity = intensity;
    event.payload = payload;
    const std::uint64_t sequence = events_.NextSequence();
    return events_.Push(event) ? sequence : 0u;
}

void GameSimulation::RefreshSnapshot(const InputSnapshot& input)
{
    snapshot_.tickIndex = tickIndex_;
    snapshot_.combatContactTrace = combatContactTrace_;
    snapshot_.inputPublicationSequence = inputPublicationSequence_;
    snapshot_.inputMoveForward = input.moveForward;
    snapshot_.inputMoveStrafe = input.moveStrafe;
    snapshot_.lastConsumedAttackSequence = lastConsumedAttackSequence_;
    snapshot_.lastConsumedParrySequence = lastConsumedParrySequence_;
    snapshot_.lastConsumedDodgeSequence = lastConsumedDodgeSequence_;
    snapshot_.lastConsumedRouteResetSequence = lastConsumedRouteResetSequence_;
    snapshot_.lastConsumedRetrySequence = lastConsumedRetrySequence_;
    snapshot_.lastConsumedInteractSequence = lastConsumedInteractSequence_;
    snapshot_.lastConsumedToggleHeldLightPoseSequence =
        lastConsumedToggleHeldLightPoseSequence_;
    snapshot_.lastConsumedRunToggleSequence = lastConsumedRunToggleSequence_;
    snapshot_.lastConsumedClearRunIntentSequence = lastConsumedClearRunIntentSequence_;
    snapshot_.developmentWorldRoute=config_.developmentWorldRoute;
    snapshot_.stagedWorldPreparation=stagedWorldPreparation_;
    snapshot_.worldRoute=worldRoute_;
    snapshot_.playerSupportSurface=playerSupport_.surface;
    snapshot_.playerSupportWorldY = playerSupport_.worldY;
    snapshot_.playerHeightDelta = PlayerHeightDelta(playerSupport_.worldY);
    snapshot_.playerSupportId = playerSupport_.id;
    snapshot_.playerGrounded = playerSupport_.grounded;
    snapshot_.playerSupportGeneration = config_.developmentWorldRoute ? worldRoute_.generation : supportGeneration_;
    snapshot_.developmentSupportFixture = config_.developmentSupportFixture;
    snapshot_.playerX = playerX_;
    snapshot_.playerZ = playerZ_;
    snapshot_.playerYawRadians = playerYawRadians_;
    snapshot_.playerPitchRadians = playerPitchRadians_;
    snapshot_.movementForward = FiniteOr(input.moveForward, 0.0f);
    snapshot_.movementStrafe = FiniteOr(input.moveStrafe, 0.0f);
    snapshot_.playerMovementSpeedMetresPerSecond = playerMovementSpeed_;
    snapshot_.walkTime = walkTime_;
    snapshot_.walkAmount = walkVisualAmount_;
    snapshot_.torchLightStrength = std::clamp(FiniteOr(input.torchLightStrength, 1.8f), 0.65f, 2.4f);
    snapshot_.dodgeActive = dodgeRemainingSeconds_ > 0.0f;
    snapshot_.dodgeElapsedSeconds = snapshot_.dodgeActive
        ? kDodgeDurationSeconds - dodgeRemainingSeconds_ : 0.0f;
    snapshot_.modernCombatRules = CurrentCombatRules();
    snapshot_.dodgeInvulnerable = CurrentCombatRules() && DodgeProtectionWindow::Contains(
        snapshot_.dodgeElapsedSeconds, dodgeProtectionAccepted_ && snapshot_.dodgeActive);
    snapshot_.acceptedDodgeSequence = acceptedDodgeSequence_;
    snapshot_.dodgeProtectedHitCount = dodgeProtectedHitCount_;
    snapshot_.combatTeaching = combatTeaching_.Snapshot();
    snapshot_.keeperRepelRemainingSeconds = keeperRepelRemainingSeconds_;
    snapshot_.keeperRepelTravelledMetres = keeperRepelTravelledMetres_;
    snapshot_.gameplayTimeScale = gameplayTimeScale_;
    snapshot_.runActive = runActive_;
    snapshot_.runToggleActive = runToggleActive_;
    snapshot_.dodgeCooldownRemainingSeconds = dodgeCooldownRemainingSeconds_;
    snapshot_.zone = QueryShowcaseZone(playerX_, playerZ_);
    snapshot_.activeEnemyKind = activeEnemyKind_;
    snapshot_.skeletonEnemyCount = combatSnapshot_.combatantCount;
    snapshot_.activeSkeletonCount = combatSnapshot_.aliveCount;
    snapshot_.skeletonAttackerId = combatSnapshot_.attackerIndex >= 0
        ? SkeletonEntity(static_cast<std::size_t>(combatSnapshot_.attackerIndex))
        : EntityId::Invalid;
    snapshot_.openingEncounterComplete = combatSnapshot_.encounterComplete;
    for (std::size_t index = 0u; index < combatSnapshot_.combatants.size(); ++index)
    {
        const SkeletonCombatantSnapshot& source = combatSnapshot_.combatants[index];
        SkeletonEnemySnapshot& target = snapshot_.skeletonEnemies[index];
        target.id = SkeletonEntity(index);
        target.x = source.x;
        target.z = source.z;
        target.facingRadians = source.facingRadians;
        target.animationTime = source.animationTime;
        target.idlePhaseSeconds = skeletonIdlePhasesEnabled_ && index == 1u ? 0.73f : 0.0f;
        target.damageFlash = source.damageFlash;
        target.health = source.health;
        target.animation = source.animation;
        target.action = source.action;
        target.reaction = source.reaction;
        target.actionTime = source.actionTime;
        target.reactionTime = source.reactionTime;
        target.dead = source.health <= 0;
        target.playerHitPulse = source.playerHitPulse;
        target.parrySuccessPulse = source.parrySuccessPulse;
        target.forceCurrentCombatPose = CurrentCombatRules() &&
            ((source.action != EnemyCombatAction::Locomotion && source.action != EnemyCombatAction::Dead) ||
             combatSnapshot_.player.action == PlayerCombatAction::SwingActive ||
             combatSnapshot_.player.action == PlayerCombatAction::UpwardSliceActive);
    }
    snapshot_.activeEnemyId = EntityForEnemy(activeEnemyKind_);
    if (activeEnemyKind_ == EnemyKind::Skeleton)
    {
        if (snapshot_.skeletonAttackerId != EntityId::Invalid)
        {
            snapshot_.activeEnemyId = snapshot_.skeletonAttackerId;
        }
        else if (snapshot_.skeletonEnemies[0].dead && !snapshot_.skeletonEnemies[1].dead)
        {
            snapshot_.activeEnemyId = EntityId::SkeletonB;
        }
    }
    snapshot_.retryCheckpoint = retryCheckpoint_;
    snapshot_.retryGeneration = retryGeneration_;
    snapshot_.paused = input.paused;
    snapshot_.playerAlive = playerVitals_.Snapshot().phase == PlayerLifePhase::Alive;
    snapshot_.finaleComplete = !config_.developmentRescueJourney && finaleSequence_.Snapshot().endingPhase ==
        horde::gameplay::interactions::FinaleEndingPhase::Complete;
    snapshot_.interaction = interactionState_;
    snapshot_.chestReward = chestRewardSequence_.Snapshot();
    snapshot_.chestPrompt = chestRewardSequence_.QueryPrompt({
        playerX_, playerZ_, playerYawRadians_});
    snapshot_.finale = finaleSequence_.Snapshot();
    snapshot_.lanternPendulum = lanternPendulum_.Snapshot();
    snapshot_.rewardLanternWorldFromHinge = RescueLanternHinge();
    snapshot_.torchFailure = torchFailureSnapshot_;
    snapshot_.heldItems = heldItems_;
    snapshot_.automaticSwordDrawBlocksDefense = SwordDrawBlocksDefense();
    snapshot_.heldItemKinematics = heldItemFixedStepState_.kinematics;
    snapshot_.playerAnimation = playerAnimationState_.Snapshot();
    snapshot_.heldLight = heldItemFixedStepState_.light;
    snapshot_.enemyRoster = enemyDirector_.Snapshot();
    snapshot_.swordCombat = combatSnapshot_;
    snapshot_.playerCombat = combatSnapshot_.player;
    snapshot_.combatPresentation = combatPresentation_.Snapshot();
    snapshot_.combatInputTiming.inputHistoryOverwriteCount = input.combatEdgeHistory.overwriteCount;
    std::uint64_t scheduledCombatCount = scheduledCombatEdgeCount_;
    for (const std::uint64_t count : overflowCombatCommandCounts_)
        scheduledCombatCount = SaturatingAdd(scheduledCombatCount, count);
    snapshot_.combatInputTiming.scheduledEdgeCount = static_cast<std::uint32_t>(
        std::min<std::uint64_t>(scheduledCombatCount, UINT32_MAX));
    snapshot_.lich = lichEncounter_.Snapshot();
    snapshot_.lich.finaleSkylightOpenProgress = snapshot_.finale.skylightOpenProgress;
    snapshot_.lich.finaleDawnRevealProgress = snapshot_.finale.dawnRevealProgress;
    snapshot_.lich.finaleEndingPhase = static_cast<FinaleEndingPhase>(
        snapshot_.finale.endingPhase);
    snapshot_.playerVitals = playerVitals_.Snapshot();
    snapshot_.fireEmitters = fireEmitters_;
    snapshot_.fireEmitterCount = fireEmitterCount_;
    snapshot_.playerMountProfile = config_.playerMountProfile;
    snapshot_.fixedStepAccumulatorSeconds = fixedStepRunner_.AccumulatorSeconds();
    snapshot_.catchUpOverrunCount = fixedStepRunner_.OverrunCount();
    snapshot_.queuedEventCount = events_.Size();
    snapshot_.eventQueueHighWaterMark = events_.HighWaterMark();
    snapshot_.eventQueueOverflowCount = events_.OverflowCount();
    PublishRescueSnapshot();
}

} // namespace horde::gameplay::simulation
