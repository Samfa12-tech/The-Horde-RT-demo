#include "gameplay/simulation/GameSimulation.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "gameplay/CorridorCollision.h"
#include "gameplay/ShowcaseCheckpoints.h"

namespace horde::gameplay::simulation
{
namespace
{

constexpr float kMinimumPitch = -0.32f;
constexpr float kMaximumPitch = 0.28f;
constexpr float kDodgeDurationSeconds = 0.20f;
constexpr float kDodgeDistanceMetres = 0.90f;
constexpr float kDodgeCooldownSeconds = 0.55f;

float FiniteOr(float value, float fallback)
{
    return std::isfinite(value) ? value : fallback;
}

std::uint64_t SequenceDelta(std::uint64_t newer, std::uint64_t older)
{
    return newer >= older ? newer - older : 0u;
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
      playerPitchRadians_(config.playerStartPitchRadians)
{
    if (!IsShowcasePlayerPositionWalkable(playerX_, playerZ_))
    {
        playerX_ = kPlayerSpawn.x;
        playerZ_ = kPlayerSpawn.z;
    }
    config_.movementSpeedMetresPerSecond = std::max(0.0f, config_.movementSpeedMetresPerSecond);
    enemyDirector_.Reset();
    activeEnemyKind_ = enemyDirector_.Snapshot().selectedEnemy;
    combatSnapshot_ = swordCombat_.Snapshot();
    torchFailureSnapshot_ = torchFailure_.Snapshot();
    ResolveHeldItems();
    RefreshSnapshot(lastInput_);
}

std::uint32_t GameSimulation::AdvanceFrame(const InputSnapshot& input,
                                           double frameDeltaSeconds,
                                           std::uint64_t inputPublicationSequence)
{
    lastInput_ = input;
    inputPublicationSequence_ = inputPublicationSequence;
    const std::size_t eventsBeforeFrame = events_.Size();
    snapshot_.eventsEmittedThisTick = 0u;
    snapshot_.eventsEmittedThisFrame = 0u;
    IngestCommands(input);
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
        walkVisualAmount_ = 0.0f;
        snapshot_.playerTravelledThisTick = 0.0f;
        playerFootsteps_.Reset();
        for (PlayerFootstepCadence& cadence : enemyFootsteps_)
        {
            cadence.Reset();
        }
    }
    const std::uint32_t ticks = fixedStepRunner_.Advance(
        frameDeltaSeconds,
        input.paused,
        [this, &input, inputPublicationSequence](float fixedDeltaSeconds)
        {
            StepFixed(input, fixedDeltaSeconds, inputPublicationSequence);
        });
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
    fixedDeltaSeconds = std::clamp(fixedDeltaSeconds, 0.0f, 0.05f);
    lastInput_ = input;
    inputPublicationSequence_ = inputPublicationSequence;
    ++tickIndex_;
    const std::size_t eventsBefore = events_.Size();
    snapshot_.eventsEmittedThisTick = 0u;

    IngestCommands(input);
    if (ConsumeWorldCommand())
    {
        RefreshSnapshot(input);
        snapshot_.eventsEmittedThisTick = 0u;
        return;
    }

    const bool wasAlive = playerVitals_.Snapshot().phase == PlayerLifePhase::Alive;
    playerVitals_.Update(fixedDeltaSeconds);
    const bool playerAlive = playerVitals_.Snapshot().phase == PlayerLifePhase::Alive;
    if (input.paused || !playerAlive)
    {
        lastConsumedAttackSequence_ += pendingAttackCommands_;
        pendingAttackCommands_ = 0u;
        lastConsumedParrySequence_ += pendingParryCommands_;
        pendingParryCommands_ = 0u;
        lastConsumedDodgeSequence_ += pendingDodgeCommands_;
        pendingDodgeCommands_ = 0u;
        dodgeRemainingSeconds_ = 0.0f;
    }
    if (!input.paused && playerAlive)
    {
        walkTime_ += fixedDeltaSeconds;
        UpdateMovement(input, fixedDeltaSeconds);
        torchFailureSnapshot_ = torchFailure_.Update(fixedDeltaSeconds,
                                           playerX_,
                                           playerZ_,
                                           playerYawRadians_,
                                           playerPitchRadians_);
        UpdateEncounters(input, fixedDeltaSeconds);
        ResolveHeldItems();
    }
    else
    {
        snapshot_.playerTravelledThisTick = 0.0f;
        walkVisualAmount_ = 0.0f;
        playerFootsteps_.Reset();
        for (PlayerFootstepCadence& cadence : enemyFootsteps_)
        {
            cadence.Reset();
        }
    }

    if (wasAlive && playerVitals_.Snapshot().phase != PlayerLifePhase::Alive)
    {
        pendingAttackCommands_ = 0u;
        pendingParryCommands_ = 0u;
        pendingDodgeCommands_ = 0u;
        dodgeRemainingSeconds_ = 0.0f;
    }

    RefreshSnapshot(input);
    snapshot_.eventsEmittedThisTick = events_.Size() - eventsBefore;
}

void GameSimulation::ResetRoute()
{
    if (!ApplyShowcaseCheckpoint(0, false))
    {
        return;
    }

    playerX_ = FiniteOr(config_.playerStartX, kPlayerSpawn.x);
    playerZ_ = FiniteOr(config_.playerStartZ, kPlayerSpawn.z);
    if (!IsShowcasePlayerPositionWalkable(playerX_, playerZ_))
    {
        playerX_ = kPlayerSpawn.x;
        playerZ_ = kPlayerSpawn.z;
    }
    playerYawRadians_ = FiniteOr(config_.playerStartYawRadians, 0.0f);
    playerPitchRadians_ = std::clamp(FiniteOr(config_.playerStartPitchRadians, 0.0f),
                                     kMinimumPitch,
                                     kMaximumPitch);
    swordCombat_.Reset(kSkeletonEnemyCapacity);
    combatSnapshot_ = swordCombat_.Update(0.0f,
                                           playerX_,
                                           playerZ_,
                                           playerYawRadians_);
    ResolveHeldItems();
    RefreshSnapshot(lastInput_);
}

void GameSimulation::RetryEncounter()
{
    ApplyShowcaseCheckpoint(retryCheckpoint_, true);
}

bool GameSimulation::ApplyShowcaseCheckpoint(std::int32_t checkpointId, bool countAsRetry)
{
    return ApplyCheckpoint(checkpointId, countAsRetry);
}

void GameSimulation::ResetTiming()
{
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

void GameSimulation::IngestCommands(const InputSnapshot& input)
{
    pendingAttackCommands_ += SequenceDelta(input.commands.attack, latestAttackSequence_);
    pendingParryCommands_ += SequenceDelta(input.commands.parry, latestParrySequence_);
    const std::uint64_t dodgeDelta = SequenceDelta(input.commands.dodge, latestDodgeSequence_);
    if (dodgeDelta > 0u)
    {
        pendingDodgeCommands_ += dodgeDelta;
        // Capture the coherent left-stick publication associated with the
        // button edge; releasing the stick before the next fixed tick cannot
        // change the requested dodge direction.
        pendingDodgeForward_ = std::clamp(FiniteOr(input.moveForward, 0.0f), -1.0f, 1.0f);
        pendingDodgeStrafe_ = std::clamp(FiniteOr(input.moveStrafe, 0.0f), -1.0f, 1.0f);
    }
    pendingRouteResetCommands_ += SequenceDelta(input.commands.routeReset, latestRouteResetSequence_);
    pendingRetryCommands_ += SequenceDelta(input.commands.retry, latestRetrySequence_);
    latestAttackSequence_ = std::max(latestAttackSequence_, input.commands.attack);
    latestParrySequence_ = std::max(latestParrySequence_, input.commands.parry);
    latestDodgeSequence_ = std::max(latestDodgeSequence_, input.commands.dodge);
    latestRouteResetSequence_ = std::max(latestRouteResetSequence_, input.commands.routeReset);
    latestRetrySequence_ = std::max(latestRetrySequence_, input.commands.retry);
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
        ApplyCheckpoint(retryCheckpoint_, true);
        return true;
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

    events_.Clear();
    ShowcaseCheckpointState state = BuildShowcaseCheckpointState(*checkpoint);
    playerX_ = checkpoint->x;
    playerZ_ = checkpoint->z;
    playerYawRadians_ = checkpoint->yaw;
    playerPitchRadians_ = checkpoint->pitch;
    walkTime_ = 0.0f;
    walkVisualAmount_ = 0.0f;
    torchFailure_ = state.torchFailure;
    torchFailureSnapshot_ = torchFailure_.Snapshot();
    horde::gameplay::items::ImportHeldItemCheckpoint(
        heldItems_, torchFailureSnapshot_.heldByPlayer, tickIndex_);
    enemyDirector_ = state.enemyDirector;
    activeEnemyKind_ = state.activeEnemyKind;
    lichEncounter_ = state.lichEncounter;
    const bool pairCheckpoint = checkpoint->preset == ShowcaseCheckpointPreset::TwoSkeletonCombat;
    swordCombat_.Reset((isRetry || pairCheckpoint) ? kSkeletonEnemyCapacity : 1u);
    combatSnapshot_ = swordCombat_.Update(0.0f,
                                           playerX_,
                                           playerZ_,
                                           playerYawRadians_);
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
    pendingAttackCommands_ = 0u;
    pendingParryCommands_ = 0u;
    pendingDodgeCommands_ = 0u;
    dodgeRemainingSeconds_ = 0.0f;
    dodgeCooldownRemainingSeconds_ = 0.0f;
    retryCheckpoint_ = activeEnemyKind_ == EnemyKind::Lich ? 9 : 0;
    finaleCompletionEmitted_ = false;
    if (isRetry)
    {
        ++retryGeneration_;
    }
    fixedStepRunner_.ResetAccumulator();
    ResolveHeldItems();
    RefreshSnapshot(lastInput_);
    snapshot_.eventsEmittedThisTick = 0u;
    return true;
}

void GameSimulation::ResolveHeldItems()
{
    std::string diagnostic;
    const horde::gameplay::items::HeldItemFixedStepInput input{
        playerX_,
        playerZ_,
        playerYawRadians_,
        playerPitchRadians_,
        walkTime_,
        walkVisualAmount_,
        torchFailureSnapshot_,
        combatSnapshot_.player,
        combatSnapshot_.swordSwingRadians};
    // Every socket contract is a checked rigid transform. A failure would
    // indicate a source-code contract violation; preserve the last immutable
    // state rather than publishing a renderer-authored fallback.
    horde::gameplay::items::ResolveHeldItemsFixedStep(
        heldItems_, input, tickIndex_, heldItemFixedStepState_, diagnostic);
}

void GameSimulation::UpdateMovement(const InputSnapshot& input, float deltaSeconds)
{
    playerYawRadians_ = FiniteOr(input.yawRadians, playerYawRadians_);
    playerPitchRadians_ = std::clamp(FiniteOr(input.pitchRadians, playerPitchRadians_),
                                     kMinimumPitch,
                                     kMaximumPitch);
    const float previousX = playerX_;
    const float previousZ = playerZ_;

    dodgeCooldownRemainingSeconds_ = std::max(
        0.0f, dodgeCooldownRemainingSeconds_ - deltaSeconds);
    if (pendingDodgeCommands_ > 0u)
    {
        lastConsumedDodgeSequence_ += pendingDodgeCommands_;
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
            dodgeCooldownRemainingSeconds_ = kDodgeCooldownSeconds;
        }
    }

    float movementIntent = 0.0f;
    if (input.hasAuthoritativePlayerPose)
    {
        playerX_ = FiniteOr(input.authoritativePlayerX, playerX_);
        playerZ_ = FiniteOr(input.authoritativePlayerZ, playerZ_);
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
        }
        else
        {
            float forward = std::clamp(FiniteOr(input.moveForward, 0.0f), -1.0f, 1.0f);
            float strafe = std::clamp(FiniteOr(input.moveStrafe, 0.0f), -1.0f, 1.0f);
            const float magnitude = std::sqrt(forward * forward + strafe * strafe);
            movementIntent = std::min(1.0f, magnitude);
            if (magnitude > 1.0f)
            {
                forward /= magnitude;
                strafe /= magnitude;
            }

            const float forwardX = std::sin(playerYawRadians_);
            const float forwardZ = -std::cos(playerYawRadians_);
            const float rightX = std::cos(playerYawRadians_);
            const float rightZ = std::sin(playerYawRadians_);
            playerX_ += (forwardX * forward + rightX * strafe) *
                        config_.movementSpeedMetresPerSecond * deltaSeconds;
            playerZ_ += (forwardZ * forward + rightZ * strafe) *
                        config_.movementSpeedMetresPerSecond * deltaSeconds;
        }
        ResolveCorridorPlayerCollision(previousX, previousZ, playerX_, playerZ_);
    }

    const float travelled = std::hypot(playerX_ - previousX, playerZ_ - previousZ);
    if (input.hasAuthoritativePlayerPose)
    {
        movementIntent = travelled > 0.00001f ? 1.0f : 0.0f;
    }
    snapshot_.playerTravelledThisTick = travelled;
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

void GameSimulation::UpdateEncounters(const InputSnapshot& input, float deltaSeconds)
{
    enemyDirector_.Update(playerX_, playerZ_);
    const EnemyKind selectedEnemy = enemyDirector_.Snapshot().selectedEnemy;
    if (selectedEnemy != activeEnemyKind_)
    {
        activeEnemyKind_ = selectedEnemy;
        playerVitals_.ResetForEncounter();
        retryCheckpoint_ = activeEnemyKind_ == EnemyKind::Lich ? 9 : 0;
        if (activeEnemyKind_ == EnemyKind::Skeleton)
        {
            // The opening enemies persist across route selection changes. Only
            // an explicit retry, route reset, or checkpoint import respawns them.
        }
        else if (activeEnemyKind_ == EnemyKind::Lich)
        {
            lichEncounter_.Reset();
        }
    }

    const bool playerActionAvailable = swordCombat_.CanAcceptPlayerAction();
    bool playerActionAccepted = false;
    if (pendingAttackCommands_ > 0u)
    {
        lastConsumedAttackSequence_ += pendingAttackCommands_;
        pendingAttackCommands_ = 0u;
        if (playerActionAvailable)
        {
            swordCombat_.RequestAttack();
            playerActionAccepted = true;
            Emit(GameplayEventType::PlayerSwing,
                 EntityId::Player,
                 EntityId::Invalid,
                 playerX_,
                 playerZ_);
        }
    }
    if (pendingParryCommands_ > 0u)
    {
        lastConsumedParrySequence_ += pendingParryCommands_;
        pendingParryCommands_ = 0u;
        if (playerActionAvailable && !playerActionAccepted)
        {
            swordCombat_.RequestParry();
        }
    }

    const auto previousSkeletons = combatSnapshot_.combatants;
    combatSnapshot_ = swordCombat_.Update(deltaSeconds,
                                           playerX_,
                                           playerZ_,
                                           playerYawRadians_);
    EntityId skeletonDamageSource = EntityId::Invalid;
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
        if (activeEnemyKind_ == EnemyKind::Skeleton && previous.health > 0 && current.health <= 0)
        {
            Emit(GameplayEventType::EnemyHit,
                 EntityId::Player,
                 entity,
                 current.x,
                 current.z);
            Emit(GameplayEventType::EnemyDefeated,
                 EntityId::Player,
                 entity,
                 current.x,
                 current.z);
        }
        if (current.playerHitPulse)
        {
            skeletonDamageSource = entity;
        }
        if (current.parrySuccessPulse)
        {
            Emit(GameplayEventType::PlayerParrySucceeded,
                 EntityId::Player,
                 entity,
                 current.x,
                 current.z);
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
    }
    if (activeEnemyKind_ == EnemyKind::Skeleton && combatSnapshot_.encounterComplete)
    {
        enemyDirector_.MarkSelectedDead();
    }

    const bool finaleActive = QueryShowcaseZone(playerX_, playerZ_) == ShowcaseZone::Finale;
    const LichPhase previousLichPhase = lichEncounter_.Snapshot().phase;
    const bool lineOfSight = !IsRouteAudioObstructed(playerX_,
                                                      playerZ_,
                                                      lichEncounter_.Snapshot().x,
                                                      lichEncounter_.Snapshot().z);
    const LichSnapshot& lich = lichEncounter_.Update(
        activeEnemyKind_ == EnemyKind::Lich ? deltaSeconds : 0.0f,
        playerX_,
        playerZ_,
        lineOfSight,
        activeEnemyKind_ == EnemyKind::Lich && finaleActive);

    if (activeEnemyKind_ == EnemyKind::Lich && combatSnapshot_.playerAttackPulse &&
        SwordCombat::IsPlayerTargetInRangeCone(playerX_,
                                                playerZ_,
                                                playerYawRadians_,
                                                lich.x,
                                                lich.z) &&
        lichEncounter_.TryAcceptPlayerHit(playerX_, playerZ_))
    {
        Emit(GameplayEventType::EnemyHit,
             EntityId::Player,
             EntityId::Lich,
             lich.x,
             lich.z);
        if (lichEncounter_.Snapshot().phase == LichPhase::Dead)
        {
            Emit(GameplayEventType::LichDefeated,
                 EntityId::Player,
                 EntityId::Lich,
                 lich.x,
                 lich.z);
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
    if (input.damageEnabled && playerDamagePulse)
    {
        const PlayerDamageResult damageResult = playerVitals_.TryApplyDamage();
        if (damageResult == PlayerDamageResult::Damaged)
        {
            Emit(GameplayEventType::PlayerDamaged,
                 activeEnemyKind_ == EnemyKind::Skeleton
                     ? skeletonDamageSource
                     : EntityForEnemy(activeEnemyKind_),
                 EntityId::Player,
                 playerX_,
                 playerZ_);
        }
        if (damageResult == PlayerDamageResult::Killed)
        {
            retryCheckpoint_ = activeEnemyKind_ == EnemyKind::Lich ? 9 : 0;
            pendingAttackCommands_ = 0u;
            Emit(GameplayEventType::PlayerKilled,
                 activeEnemyKind_ == EnemyKind::Skeleton
                     ? skeletonDamageSource
                     : EntityForEnemy(activeEnemyKind_),
                 EntityId::Player,
                 playerX_,
                 playerZ_);
        }
    }

    if (activeEnemyKind_ == EnemyKind::Lich && lichEncounter_.Snapshot().deathAnimationComplete)
    {
        enemyDirector_.MarkSelectedDead();
    }
    if (lichEncounter_.Snapshot().finaleEndingPhase == FinaleEndingPhase::Complete &&
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

void GameSimulation::Emit(GameplayEventType type,
                          EntityId source,
                          EntityId target,
                          float x,
                          float z,
                          float intensity,
                          std::int32_t payload)
{
    GameplayEvent event;
    event.type = type;
    event.source = source;
    event.target = target;
    event.worldX = x;
    event.worldZ = z;
    event.listenerX = playerX_;
    event.listenerZ = playerZ_;
    event.listenerYawRadians = playerYawRadians_;
    event.intensity = intensity;
    event.payload = payload;
    events_.Push(event);
}

void GameSimulation::RefreshSnapshot(const InputSnapshot& input)
{
    snapshot_.tickIndex = tickIndex_;
    snapshot_.inputPublicationSequence = inputPublicationSequence_;
    snapshot_.lastConsumedAttackSequence = lastConsumedAttackSequence_;
    snapshot_.lastConsumedParrySequence = lastConsumedParrySequence_;
    snapshot_.lastConsumedDodgeSequence = lastConsumedDodgeSequence_;
    snapshot_.lastConsumedRouteResetSequence = lastConsumedRouteResetSequence_;
    snapshot_.lastConsumedRetrySequence = lastConsumedRetrySequence_;
    snapshot_.playerX = playerX_;
    snapshot_.playerZ = playerZ_;
    snapshot_.playerYawRadians = playerYawRadians_;
    snapshot_.playerPitchRadians = playerPitchRadians_;
    snapshot_.walkTime = walkTime_;
    snapshot_.walkAmount = walkVisualAmount_;
    snapshot_.torchLightStrength = std::clamp(FiniteOr(input.torchLightStrength, 1.8f), 0.65f, 2.4f);
    snapshot_.dodgeActive = dodgeRemainingSeconds_ > 0.0f;
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
    snapshot_.finaleComplete = lichEncounter_.Snapshot().finaleEndingPhase == FinaleEndingPhase::Complete;
    snapshot_.torchFailure = torchFailureSnapshot_;
    snapshot_.heldItems = heldItems_;
    snapshot_.heldItemKinematics = heldItemFixedStepState_.kinematics;
    snapshot_.heldLight = heldItemFixedStepState_.light;
    snapshot_.enemyRoster = enemyDirector_.Snapshot();
    snapshot_.swordCombat = combatSnapshot_;
    snapshot_.playerCombat = combatSnapshot_.player;
    snapshot_.lich = lichEncounter_.Snapshot();
    snapshot_.playerVitals = playerVitals_.Snapshot();
    snapshot_.fixedStepAccumulatorSeconds = fixedStepRunner_.AccumulatorSeconds();
    snapshot_.catchUpOverrunCount = fixedStepRunner_.OverrunCount();
    snapshot_.queuedEventCount = events_.Size();
    snapshot_.eventQueueHighWaterMark = events_.HighWaterMark();
    snapshot_.eventQueueOverflowCount = events_.OverflowCount();
}

} // namespace horde::gameplay::simulation
