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
constexpr float kSwordDurationSeconds = 0.56f;

float FiniteOr(float value, float fallback)
{
    return std::isfinite(value) ? value : fallback;
}

std::uint64_t SequenceDelta(std::uint64_t newer, std::uint64_t older)
{
    return newer >= older ? newer - older : 0u;
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
    lanternSnapshot_ = lantern_.Snapshot();
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
        enemyFootsteps_.Reset();
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
    if (!input.paused && playerAlive)
    {
        walkTime_ += fixedDeltaSeconds;
        UpdateMovement(input, fixedDeltaSeconds);
        lanternSnapshot_ = lantern_.Update(fixedDeltaSeconds,
                                           playerX_,
                                           playerZ_,
                                           playerYawRadians_,
                                           playerPitchRadians_);
        UpdateEncounters(input, fixedDeltaSeconds);
    }
    else
    {
        snapshot_.playerTravelledThisTick = 0.0f;
        walkVisualAmount_ = 0.0f;
        playerFootsteps_.Reset();
        enemyFootsteps_.Reset();
    }

    if (wasAlive && playerVitals_.Snapshot().phase != PlayerLifePhase::Alive)
    {
        pendingAttackCommands_ = 0u;
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
    case EnemyKind::Skeleton: return EntityId::Skeleton;
    case EnemyKind::Lich: return EntityId::Lich;
    default: return EntityId::Invalid;
    }
}

void GameSimulation::IngestCommands(const InputSnapshot& input)
{
    pendingAttackCommands_ += SequenceDelta(input.commands.attack, latestAttackSequence_);
    pendingRouteResetCommands_ += SequenceDelta(input.commands.routeReset, latestRouteResetSequence_);
    pendingRetryCommands_ += SequenceDelta(input.commands.retry, latestRetrySequence_);
    latestAttackSequence_ = std::max(latestAttackSequence_, input.commands.attack);
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
    lantern_ = state.lantern;
    lanternSnapshot_ = lantern_.Snapshot();
    enemyDirector_ = state.enemyDirector;
    activeEnemyKind_ = state.activeEnemyKind;
    lichEncounter_ = state.lichEncounter;
    swordCombat_ = {};
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
    enemyFootsteps_.Reset();
    playerAttackCooldownRemaining_ = 0.0f;
    pendingAttackCommands_ = 0u;
    retryCheckpoint_ = activeEnemyKind_ == EnemyKind::Lich ? 9 : 0;
    finaleCompletionEmitted_ = false;
    if (isRetry)
    {
        ++retryGeneration_;
    }
    fixedStepRunner_.ResetAccumulator();
    RefreshSnapshot(lastInput_);
    snapshot_.eventsEmittedThisTick = 0u;
    return true;
}

void GameSimulation::UpdateMovement(const InputSnapshot& input, float deltaSeconds)
{
    playerYawRadians_ = FiniteOr(input.yawRadians, playerYawRadians_);
    playerPitchRadians_ = std::clamp(FiniteOr(input.pitchRadians, playerPitchRadians_),
                                     kMinimumPitch,
                                     kMaximumPitch);
    const float previousX = playerX_;
    const float previousZ = playerZ_;

    float movementIntent = 0.0f;
    if (input.hasAuthoritativePlayerPose)
    {
        playerX_ = FiniteOr(input.authoritativePlayerX, playerX_);
        playerZ_ = FiniteOr(input.authoritativePlayerZ, playerZ_);
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
            swordCombat_ = {};
            combatSnapshot_ = {};
        }
        else if (activeEnemyKind_ == EnemyKind::Lich)
        {
            lichEncounter_.Reset();
        }
    }

    playerAttackCooldownRemaining_ = std::max(0.0f, playerAttackCooldownRemaining_ - deltaSeconds);
    bool lichHitRequested = false;
    if (pendingAttackCommands_ > 0u && playerAttackCooldownRemaining_ <= 0.00001f)
    {
        --pendingAttackCommands_;
        ++lastConsumedAttackSequence_;
        swordCombat_.RequestAttack();
        playerAttackCooldownRemaining_ = kSwordDurationSeconds;
        lichHitRequested = activeEnemyKind_ == EnemyKind::Lich;
        Emit(GameplayEventType::PlayerSwing,
             EntityId::Player,
             EntityForEnemy(activeEnemyKind_),
             playerX_,
             playerZ_);
    }

    const EnemyAnimation previousEnemyAnimation = combatSnapshot_.enemyAnimation;
    combatSnapshot_ = swordCombat_.Update(deltaSeconds,
                                           playerX_,
                                           playerZ_,
                                           playerYawRadians_);
    if (activeEnemyKind_ == EnemyKind::Skeleton &&
        previousEnemyAnimation != EnemyAnimation::Attack &&
        combatSnapshot_.enemyAnimation == EnemyAnimation::Attack)
    {
        Emit(GameplayEventType::EnemyAttackStarted,
             EntityId::Skeleton,
             EntityId::Player,
             combatSnapshot_.enemyX,
             combatSnapshot_.enemyZ);
    }
    if (activeEnemyKind_ == EnemyKind::Skeleton &&
        previousEnemyAnimation != EnemyAnimation::Dead &&
        combatSnapshot_.enemyAnimation == EnemyAnimation::Dead)
    {
        Emit(GameplayEventType::EnemyHit,
             EntityId::Player,
             EntityId::Skeleton,
             combatSnapshot_.enemyX,
             combatSnapshot_.enemyZ);
        Emit(GameplayEventType::EnemyDefeated,
             EntityId::Player,
             EntityId::Skeleton,
             combatSnapshot_.enemyX,
             combatSnapshot_.enemyZ);
    }
    const bool skeletonWalking = activeEnemyKind_ == EnemyKind::Skeleton &&
                                 combatSnapshot_.enemyAnimation == EnemyAnimation::Walking;
    if (enemyFootsteps_.Update(deltaSeconds, skeletonWalking))
    {
        Emit(GameplayEventType::EnemyFootstep,
             EntityId::Skeleton,
             EntityId::Invalid,
             combatSnapshot_.enemyX,
             combatSnapshot_.enemyZ);
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

    if (lichHitRequested && lichEncounter_.TryAcceptPlayerHit(playerX_, playerZ_))
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
        (activeEnemyKind_ == EnemyKind::Skeleton && combatSnapshot_.playerHitPulse) ||
        (activeEnemyKind_ == EnemyKind::Lich && lichEncounter_.Snapshot().damagePulse);
    if (input.damageEnabled && playerDamagePulse)
    {
        const PlayerDamageResult damageResult = playerVitals_.TryApplyDamage();
        if (damageResult == PlayerDamageResult::Damaged)
        {
            Emit(GameplayEventType::PlayerDamaged,
                 EntityForEnemy(activeEnemyKind_),
                 EntityId::Player,
                 playerX_,
                 playerZ_);
        }
        if (damageResult == PlayerDamageResult::Killed)
        {
            retryCheckpoint_ = activeEnemyKind_ == EnemyKind::Lich ? 9 : 0;
            pendingAttackCommands_ = 0u;
            Emit(GameplayEventType::PlayerKilled,
                 EntityForEnemy(activeEnemyKind_),
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
    event.intensity = intensity;
    event.payload = payload;
    events_.Push(event);
}

void GameSimulation::RefreshSnapshot(const InputSnapshot& input)
{
    snapshot_.tickIndex = tickIndex_;
    snapshot_.inputPublicationSequence = inputPublicationSequence_;
    snapshot_.lastConsumedAttackSequence = lastConsumedAttackSequence_;
    snapshot_.lastConsumedRouteResetSequence = lastConsumedRouteResetSequence_;
    snapshot_.lastConsumedRetrySequence = lastConsumedRetrySequence_;
    snapshot_.playerX = playerX_;
    snapshot_.playerZ = playerZ_;
    snapshot_.playerYawRadians = playerYawRadians_;
    snapshot_.playerPitchRadians = playerPitchRadians_;
    snapshot_.walkTime = walkTime_;
    snapshot_.walkAmount = walkVisualAmount_;
    snapshot_.lanternStrength = std::clamp(FiniteOr(input.lanternStrength, 1.8f), 0.65f, 2.4f);
    snapshot_.zone = QueryShowcaseZone(playerX_, playerZ_);
    snapshot_.activeEnemyKind = activeEnemyKind_;
    snapshot_.activeEnemyId = EntityForEnemy(activeEnemyKind_);
    snapshot_.retryCheckpoint = retryCheckpoint_;
    snapshot_.retryGeneration = retryGeneration_;
    snapshot_.paused = input.paused;
    snapshot_.playerAlive = playerVitals_.Snapshot().phase == PlayerLifePhase::Alive;
    snapshot_.finaleComplete = lichEncounter_.Snapshot().finaleEndingPhase == FinaleEndingPhase::Complete;
    snapshot_.lantern = lanternSnapshot_;
    snapshot_.enemyRoster = enemyDirector_.Snapshot();
    snapshot_.swordCombat = combatSnapshot_;
    snapshot_.lich = lichEncounter_.Snapshot();
    snapshot_.playerVitals = playerVitals_.Snapshot();
    snapshot_.fixedStepAccumulatorSeconds = fixedStepRunner_.AccumulatorSeconds();
    snapshot_.catchUpOverrunCount = fixedStepRunner_.OverrunCount();
    snapshot_.queuedEventCount = events_.Size();
    snapshot_.eventQueueHighWaterMark = events_.HighWaterMark();
    snapshot_.eventQueueOverflowCount = events_.OverflowCount();
}

} // namespace horde::gameplay::simulation
