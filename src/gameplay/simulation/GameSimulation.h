#pragma once

#include <array>
#include <cstdint>

#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/SpatialAudio.h"
#include "gameplay/SwordCombat.h"
#include "gameplay/effects/FireEmitterState.h"
#include "gameplay/animation/PlayerAnimationState.h"
#include "gameplay/items/HeldItemState.h"
#include "gameplay/items/HeldItemKinematics.h"
#include "gameplay/interactions/ChestRewardSequence.h"
#include "gameplay/interactions/FinaleSequence.h"
#include "gameplay/interactions/InteractionState.h"
#include "gameplay/items/LanternPendulum.h"
#include "gameplay/simulation/CombatPresentation.h"
#include "gameplay/simulation/CombatInputTiming.h"
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
    horde::gameplay::items::PlayerMountProfile playerMountProfile =
        horde::gameplay::items::PlayerMountProfile::LegacyViewRelative;
    // Historical fixtures remain ready-handed. Opt-in until the renderer
    // consumes the shared BodyStow transition state in production.
    bool swordStartsStowed = false;
    bool waterfallSkeletonEncounter = false;
};

// Keep the historical constructor configuration available for deterministic
// legacy fixtures. Applications explicitly select the owner-accepted profile.
inline constexpr GameSimulationConfig ProductionGameSimulationConfig()
{
    GameSimulationConfig config;
    config.playerMountProfile = horde::gameplay::items::PlayerMountProfile::AnatomicalBody;
    return config;
}

enum class PausedInputPolicy
{
    DiscardAllCommands,
    PreserveWorldCommands,
};

class GameSimulation
{
public:
    explicit GameSimulation(GameSimulationConfig config = {});

    std::uint32_t AdvanceFrame(const InputSnapshot& input,
                               double frameDeltaSeconds,
                               std::uint64_t inputPublicationSequence = 0u,
                               std::uint64_t ownerAdvanceSteadyNs = 0u);
    void StepFixed(const InputSnapshot& input,
                   float fixedDeltaSeconds = static_cast<float>(FixedStepRunner::kFixedDeltaSeconds),
                   std::uint64_t inputPublicationSequence = 0u);
    // Owner-thread barrier: real lifecycle transitions discard every stale edge.
    // Ordinary menu transitions may retain explicit reset/retry commands for
    // the owner to apply, while still discarding all combat/interaction edges.
    void SynchronizePausedInput(const InputSnapshot& input,
                                std::uint64_t inputPublicationSequence = 0u,
                                PausedInputPolicy policy = PausedInputPolicy::DiscardAllCommands);

    void ResetRoute();
    void RetryEncounter();
    bool ApplyShowcaseCheckpoint(std::int32_t checkpointId, bool countAsRetry = false);
    void ImportRewardCheckpoint(
        const horde::gameplay::interactions::ChestRewardSnapshot& chestReward,
        const horde::gameplay::interactions::InteractionState& interaction,
        const horde::gameplay::interactions::FinaleSequenceSnapshot& finale,
        const horde::gameplay::interactions::LanternPendulumSnapshot* pendulum = nullptr);
    void ResetTiming();
    void ClearEvents();

    const SimulationSnapshot& Snapshot() const { return snapshot_; }
    const BoundedGameplayEventQueue& Events() const { return events_; }
    const FixedStepRunner& Timing() const { return fixedStepRunner_; }

private:
    void StepFixedTick(const InputSnapshot& input, float fixedDeltaSeconds,
                       std::uint64_t inputPublicationSequence,
                       bool activateTimestampedEdges = true);
    static EntityId EntityForEnemy(EnemyKind kind);
    void IngestCommands(const InputSnapshot& input, bool ingestCombatEdges = true);
    void ScheduleTimestampedCombatEdges(const InputSnapshot& input,
                                        double frameDeltaSeconds,
                                        std::uint64_t ownerAdvanceSteadyNs,
                                        std::uint64_t inputPublicationSequence);
    void ActivateScheduledCombatEdges();
    void ClearScheduledCombatEdges(bool discardAndConsume);
    void AddCombatTimingTrace(const CombatInputEdge& edge,
                              std::uint64_t publicationSequence,
                              std::uint64_t targetTick,
                              CombatInputTimingDisposition disposition);
    void MarkCombatTimingConsumed(CombatInputEdgeKind kind,
                                  std::uint64_t oldConsumed,
                                  std::uint64_t newConsumed);
    void LinkCombatTimingSemanticEvent(CombatInputEdgeKind kind,
                                       std::uint64_t commandSequence,
                                       std::uint64_t eventSequence,
                                       std::uint64_t eventTick);
    bool ConsumeWorldCommand();
    bool ApplyCheckpoint(std::int32_t checkpointId, bool isRetry);
    void UpdateMovement(const InputSnapshot& input, float deltaSeconds);
    void UpdateEncounters(const InputSnapshot& input, float deltaSeconds);
    void UpdateRewardSequence(float deltaSeconds, bool commandsAvailable);
    void ResolveHeldItems();
    bool SwordDefenseReady() const;
    bool SwordDrawBlocksDefense() const;
    bool RequestSwordDraw(std::int32_t reasonPayload, bool blocksDefenseDuringDraw);
    void ResetSwordEquipment();
    void ClearQueuedDrawAttack();
    void AdvanceSwordEquipment(float fixedDeltaSeconds);
    void EmitSwordAttachmentChange();
    void ResolvePlayerAnimation(float fixedDeltaSeconds);
    void ResolveFireEmitters(float fixedDeltaSeconds);
    std::uint64_t Emit(GameplayEventType type,
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
    CombatPresentationTimeline combatPresentation_{};
    SimulationSnapshot snapshot_{};
    InputSnapshot lastInput_{};

    TorchFailureSequence torchFailure_{};
    EnemyDirector enemyDirector_{};
    SwordCombat swordCombat_{};
    LichEncounter lichEncounter_{};
    horde::gameplay::interactions::ChestRewardSequence chestRewardSequence_{};
    horde::gameplay::interactions::FinaleSequence finaleSequence_{};
    horde::gameplay::interactions::InteractionState interactionState_{};
    horde::gameplay::interactions::LanternPendulum lanternPendulum_{};
    PlayerVitals playerVitals_{};
    TravelFootstepCadence playerFootsteps_{};
    std::array<PlayerFootstepCadence, kSkeletonEnemyCapacity> enemyFootsteps_{};
    std::array<double, kSkeletonEnemyCapacity> skeletonIncidentalIdleSeconds_{};
    std::array<double, kSkeletonEnemyCapacity> skeletonIncidentalNextSeconds_{{12.0, 18.0}};
    double skeletonIncidentalSpacingSeconds_ = 0.0;
    CombatSnapshot combatSnapshot_{};
    TorchFailureSnapshot torchFailureSnapshot_{};
    horde::gameplay::items::HeldItemStates heldItems_ =
        horde::gameplay::items::MakeDefaultHeldItemStates();
    horde::gameplay::items::HeldItemFixedStepState heldItemFixedStepState_{};
    horde::gameplay::animation::PlayerAnimationState playerAnimationState_{};
    std::array<horde::gameplay::effects::FireEmitterState,
               horde::gameplay::effects::kFireEmitterCapacity> fireEmitters_{{
        horde::gameplay::effects::MakeOpeningTorchFireEmitter()}};
    std::size_t fireEmitterCount_ = 3u;
    EnemyKind activeEnemyKind_ = EnemyKind::Skeleton;

    float playerX_ = 0.0f;
    float playerZ_ = 1.85f;
    float playerYawRadians_ = 0.0f;
    float playerPitchRadians_ = -0.05f;
    float walkTime_ = 0.0f;
    float walkVisualAmount_ = 0.0f;
    std::int32_t retryCheckpoint_ = 0;
    std::uint32_t retryGeneration_ = 0u;
    std::uint64_t tickIndex_ = 0u;
    std::uint64_t inputPublicationSequence_ = 0u;
    std::uint64_t latestAttackSequence_ = 0u;
    std::uint64_t latestParrySequence_ = 0u;
    std::uint64_t latestDodgeSequence_ = 0u;
    std::uint64_t latestRouteResetSequence_ = 0u;
    std::uint64_t latestRetrySequence_ = 0u;
    std::uint64_t latestInteractSequence_ = 0u;
    std::uint64_t latestToggleHeldLightPoseSequence_ = 0u;
    std::uint64_t lastConsumedAttackSequence_ = 0u;
    std::uint64_t lastConsumedParrySequence_ = 0u;
    std::uint64_t lastConsumedDodgeSequence_ = 0u;
    std::uint64_t lastConsumedRouteResetSequence_ = 0u;
    std::uint64_t lastConsumedRetrySequence_ = 0u;
    std::uint64_t lastConsumedInteractSequence_ = 0u;
    std::uint64_t lastConsumedToggleHeldLightPoseSequence_ = 0u;
    std::uint64_t pendingAttackCommands_ = 0u;
    std::uint64_t pendingParryCommands_ = 0u;
    std::uint64_t pendingDodgeCommands_ = 0u;
    std::uint64_t pendingRouteResetCommands_ = 0u;
    std::uint64_t pendingRetryCommands_ = 0u;
    std::uint64_t pendingInteractCommands_ = 0u;
    std::uint64_t pendingToggleHeldLightPoseCommands_ = 0u;
    float pendingDodgeForward_ = 0.0f;
    float pendingDodgeStrafe_ = 0.0f;
    float dodgeDirectionX_ = 0.0f;
    float dodgeDirectionZ_ = -1.0f;
    float dodgeRemainingSeconds_ = 0.0f;
    float dodgeCooldownRemainingSeconds_ = 0.0f;
    bool finaleCompletionEmitted_ = false;
    bool lanternPendulumResetPending_ = true;
    bool skeletonIdlePhasesEnabled_ = true;
    bool waterfallWarningEmitted_ = false;
    bool automaticSwordDrawBlocksDefense_ = false;
    bool lichAttackEligible_ = false;
    std::uint64_t lichRevealAttackSequenceFloor_ = 0u;
    struct ScheduledCombatEdge
    {
        CombatInputEdge edge{};
        std::uint64_t targetTick = 0u;
        std::uint64_t publicationSequence = 0u;
    };
    std::array<ScheduledCombatEdge, kCombatInputEdgeHistoryCapacity> scheduledCombatEdges_{};
    std::uint32_t scheduledCombatEdgeCount_ = 0u;
    std::array<std::uint64_t, 3u> overflowCombatCommandCounts_{};
    std::array<std::uint64_t, 3u> overflowCombatTargetTicks_{};
    float overflowDodgeForward_ = 0.0f;
    float overflowDodgeStrafe_ = 0.0f;
    std::uint64_t previousOwnerAdvanceSteadyNs_ = 0u;
    std::uint64_t parrySourceCommandSequence_ = 0u;
    bool queuedDrawAttack_ = false;
    std::uint64_t queuedDrawAttackCommandSequence_ = 0u;
};

} // namespace horde::gameplay::simulation
