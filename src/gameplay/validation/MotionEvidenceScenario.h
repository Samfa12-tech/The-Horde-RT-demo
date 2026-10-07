#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

#include "gameplay/simulation/GameSimulation.h"

namespace horde::gameplay::validation
{

// Admission to these input schedules belongs to a Debug-only platform parser.
// This helper has no renderer, audio, preference, JNI or filesystem authority.
enum class MotionScenario : std::uint8_t
{
    TorchLowOpening,
    ShaftUp,
    KeeperFirstEntry,
    KeeperRetryReward,
    WaterfallEquipment,
    TorchDrench,
};

enum class MotionStage : std::uint8_t
{
    NotStarted, Approach, TorchMotion, ShaftMotion, EquipmentDraw, EquipmentAttack,
    EquipmentParry, Reveal, FirstTelegraph,
    AwaitDeath, RetryRequested, Recognition, RetryTelegraph, Fight,
    Reward, TerminalReentry, Complete, Failed,
    TorchDrenchGuttering, TorchDrenchFalling, TorchDrenchSettled,
};

[[nodiscard]] bool ParseMotionScenario(std::string_view text, MotionScenario& scenario) noexcept;
[[nodiscard]] const char* MotionScenarioName(MotionScenario scenario) noexcept;
[[nodiscard]] const char* MotionStageName(MotionStage stage) noexcept;

class MotionEvidenceScenario
{
public:
    static constexpr std::uint64_t kMaximumWallNanoseconds = 120'000'000'000ull;

    // Owner thread only. One accepted checkpoint seed, never a phase import.
    [[nodiscard]] bool Begin(MotionScenario scenario, simulation::GameSimulation& simulation,
                             std::uint64_t monotonicNowNanoseconds) noexcept;
    // Release the temporary waterfall equipment seed on every owner exit.
    // Safe to call repeatedly and for scenarios that do not own a seed.
    void End(simulation::GameSimulation& simulation) noexcept;
    [[nodiscard]] bool OwnsEquipmentSeed() const noexcept { return equipmentSeedOwned_; }
    // currentOutputReady is supplied by the platform's current-generation real
    // RT presentation/completion guard. Until ready, publish a paused tuple.
    [[nodiscard]] simulation::InputSnapshot BuildInput(
        const simulation::SimulationSnapshot& state,
        const simulation::InputSnapshot& published,
        std::uint64_t monotonicNowNanoseconds, bool currentOutputReady) noexcept;
    // Observe BEFORE the one established platform event drain. Never consume,
    // clear, insert or synthesize a semantic event in this observer.
    void ObserveAdvance(const simulation::SimulationSnapshot& state,
                        std::span<const simulation::GameplayEvent> events) noexcept;
    void Fail(std::string_view reason) noexcept;

    [[nodiscard]] MotionScenario Scenario() const noexcept { return scenario_; }
    [[nodiscard]] MotionStage Stage() const noexcept { return stage_; }
    [[nodiscard]] bool Complete() const noexcept { return stage_ == MotionStage::Complete; }
    [[nodiscard]] bool Failed() const noexcept { return stage_ == MotionStage::Failed; }
    [[nodiscard]] std::string_view Failure() const noexcept;
    [[nodiscard]] double SimulationSeconds() const noexcept { return simulationSeconds_; }
    [[nodiscard]] std::span<const std::uint32_t> EventCounts() const noexcept { return eventCounts_; }

private:
    void Enter(MotionStage stage) noexcept;
    void MoveTowards(simulation::InputSnapshot& input,
                     const simulation::SimulationSnapshot& state, float x, float z) const noexcept;
    void AimAt(simulation::InputSnapshot& input,
               const simulation::SimulationSnapshot& state, float x, float z) const noexcept;
    void PublishEdge(std::uint64_t& sequence) noexcept;

    MotionScenario scenario_ = MotionScenario::TorchLowOpening;
    MotionStage stage_ = MotionStage::NotStarted;
    simulation::SimulationCommandSequences commands_{};
    std::array<std::uint32_t, simulation::kGameplayEventTypeCount> eventCounts_{};
    std::array<char, 128u> failure_{};
    std::uint64_t startWall_ = 0u;
    std::uint64_t latestWall_ = 0u;
    std::uint64_t previousTick_ = 0u;
    std::uint64_t previousEventSequence_ = 0u;
    double simulationSeconds_ = 0.0;
    double stageStartSeconds_ = 0.0;
    float previousRevealElapsed_ = 0.0f;
    float telegraphInitialSeconds_ = 0.0f;
    std::uint32_t initialRetryGeneration_ = 0u;
    int initialVitality_ = 0;
    int previousKeeperHealth_ = 3;
    bool firstAttackSent_ = false;
    bool secondAttackSent_ = false;
    bool parrySent_ = false;
    bool openSent_ = false;
    bool claimSent_ = false;
    bool lowSent_ = false;
    bool highSent_ = false;
    bool retrySent_ = false;
    bool terminalExitSeen_ = false;
    bool terminalReturnSeen_ = false;
    bool nearPortalSeen_ = false;
    bool farPortalSeen_ = false;
    bool attackPoseSeen_ = false;
    bool parryPoseSeen_ = false;
    bool upLookSeen_ = false;
    bool downLookSeen_ = false;
    float revealHeldX_ = 0.0f;
    float revealHeldZ_ = 0.0f;
    bool revealHeldMoveSeen_ = false;
    bool revealLookAwaySeen_ = false;
    bool revealLookReturnSeen_ = false;
    bool shaftOpeningSeen_ = false;
    bool shaftParallaxSeen_ = false;
    bool rearLookSeen_ = false;
    bool rearParallaxSeen_ = false;
    bool rearReturnSeen_ = false;
    bool equipmentDrawTransitionSeen_ = false;
    bool equipmentAttackPoseSeen_ = false;
    bool equipmentParryPoseSeen_ = false;
    bool torchDrenchWalkingAfterRelease_ = false;
    bool torchDrenchStepBackLookSeen_ = false;
    bool torchDrenchFallingSeen_ = false;
    bool torchDrenchSettledSeen_ = false;
    float torchDrenchReleaseX_ = 0.0f;
    float torchDrenchReleaseZ_ = 0.0f;
    float torchDrenchReleaseYaw_ = 0.0f;
    float torchDrenchReleaseViewPitch_ = 0.0f;
    float torchDrenchPreviousFallProgress_ = 0.0f;
    float torchDrenchPreviousDroppedY_ = 0.0f;
    std::uint64_t torchExtinguishedSequence_ = 0u;
    std::uint64_t waterfallWarningSequence_ = 0u;
    std::uint64_t swordDrawSequence_ = 0u;
    std::uint64_t swordAttachmentSequence_ = 0u;
    std::uint64_t swordSwingSequence_ = 0u;
    bool equipmentSeedOwned_ = false;
};

} // namespace horde::gameplay::validation
