#pragma once

#include <cstdint>

#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/SpatialAudio.h"
#include "gameplay/SwordCombat.h"
#include "gameplay/simulation/FixedStepRunner.h"
#include "gameplay/simulation/GameplayEvent.h"
#include "gameplay/simulation/InputSnapshot.h"
#include "gameplay/simulation/SimulationSnapshot.h"

namespace horde::gameplay::simulation
{

struct GameSimulationConfig
{
    float playerStartX = 0.0f;
    float playerStartZ = 1.85f;
    float playerStartYawRadians = 0.0f;
    float playerStartPitchRadians = 0.0f;
    float movementSpeedMetresPerSecond = 1.9f;
};

class GameSimulation
{
public:
    explicit GameSimulation(GameSimulationConfig config = {});

    std::uint32_t AdvanceFrame(const InputSnapshot& input,
                               double frameDeltaSeconds,
                               std::uint64_t inputPublicationSequence = 0u);
    void StepFixed(const InputSnapshot& input,
                   float fixedDeltaSeconds = static_cast<float>(FixedStepRunner::kFixedDeltaSeconds),
                   std::uint64_t inputPublicationSequence = 0u);

    void ResetRoute();
    void RetryEncounter();
    bool ApplyShowcaseCheckpoint(std::int32_t checkpointId, bool countAsRetry = false);
    void ResetTiming();
    void ClearEvents();

    const SimulationSnapshot& Snapshot() const { return snapshot_; }
    const BoundedGameplayEventQueue& Events() const { return events_; }
    const FixedStepRunner& Timing() const { return fixedStepRunner_; }

private:
    static EntityId EntityForEnemy(EnemyKind kind);
    void IngestCommands(const InputSnapshot& input);
    bool ConsumeWorldCommand();
    bool ApplyCheckpoint(std::int32_t checkpointId, bool isRetry);
    void UpdateMovement(const InputSnapshot& input, float deltaSeconds);
    void UpdateEncounters(const InputSnapshot& input, float deltaSeconds);
    void Emit(GameplayEventType type,
              EntityId source,
              EntityId target,
              float x,
              float z,
              float intensity = 1.0f,
              std::int32_t payload = 0);
    void RefreshSnapshot(const InputSnapshot& input);

    GameSimulationConfig config_{};
    FixedStepRunner fixedStepRunner_{};
    BoundedGameplayEventQueue events_{};
    SimulationSnapshot snapshot_{};
    InputSnapshot lastInput_{};

    LanternSequence lantern_{};
    EnemyDirector enemyDirector_{};
    SwordCombat swordCombat_{};
    LichEncounter lichEncounter_{};
    PlayerVitals playerVitals_{};
    TravelFootstepCadence playerFootsteps_{};
    PlayerFootstepCadence enemyFootsteps_{};
    CombatSnapshot combatSnapshot_{};
    LanternSnapshot lanternSnapshot_{};
    EnemyKind activeEnemyKind_ = EnemyKind::Skeleton;

    float playerX_ = 0.0f;
    float playerZ_ = 1.85f;
    float playerYawRadians_ = 0.0f;
    float playerPitchRadians_ = -0.05f;
    float walkTime_ = 0.0f;
    float walkVisualAmount_ = 0.0f;
    float playerAttackCooldownRemaining_ = 0.0f;
    std::int32_t retryCheckpoint_ = 0;
    std::uint32_t retryGeneration_ = 0u;
    std::uint64_t tickIndex_ = 0u;
    std::uint64_t inputPublicationSequence_ = 0u;
    std::uint64_t latestAttackSequence_ = 0u;
    std::uint64_t latestRouteResetSequence_ = 0u;
    std::uint64_t latestRetrySequence_ = 0u;
    std::uint64_t lastConsumedAttackSequence_ = 0u;
    std::uint64_t lastConsumedRouteResetSequence_ = 0u;
    std::uint64_t lastConsumedRetrySequence_ = 0u;
    std::uint64_t pendingAttackCommands_ = 0u;
    std::uint64_t pendingRouteResetCommands_ = 0u;
    std::uint64_t pendingRetryCommands_ = 0u;
    bool finaleCompletionEmitted_ = false;
};

} // namespace horde::gameplay::simulation
