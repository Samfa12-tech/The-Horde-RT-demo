#pragma once

#include <array>
#include <cstdint>
#include <iosfwd>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "gameplay/validation/MotionEvidenceScenario.h"
#include "telemetry/RtPerformanceEvidence.h"

namespace horde::telemetry
{

struct MotionStateRow
{
    std::uint64_t wallNanoseconds = 0u;
    std::uint64_t tick = 0u;
    std::uint64_t publicationSequence = 0u;
    double simulationSeconds = 0.0;
    horde::gameplay::validation::MotionStage stage{};
    horde::gameplay::simulation::InputSnapshot input{};
    std::array<float, 6u> player{}; // X,Z,yaw,pitch,walkTime,walkAmount
    horde::gameplay::LichSnapshot keeper{};
    horde::gameplay::TorchFailureSnapshot torch{};
    horde::gameplay::PlayerCombatSnapshot combat{};
    struct HeldSwordEvidence
    {
        horde::gameplay::items::HeldItemParentMode parent{};
        horde::gameplay::items::HeldItemTransitionKind transition{};
        float progress = 0.0f;
        float gripBlend = 0.0f;
        float stowBlend = 0.0f;
        bool active = false;
    } sword{};
    horde::gameplay::interactions::ChestRewardSnapshot chest{};
    horde::gameplay::interactions::InteractionState heldLight{};
    horde::gameplay::interactions::FinaleSequenceSnapshot finale{};
    std::array<std::array<float, 4u>, 4u> fire{}; // unchanged: phase,leanX,leanZ,motionTurbulence
    struct SimulationFireSource
    {
        std::uint32_t stableId = 0u;
        float strength = 0.0f;
        float fuel = 0.0f;
    };
    // Configured simulation sources include dormant torches; this differs from
    // the completed upload's packed active selection below.
    std::optional<std::uint32_t> simulationFireEmitterCount{};
    std::array<SimulationFireSource, 4u> simulationFireSources{};
    horde::gameplay::simulation::SimulationCommandSequences consumed{};
    std::uint64_t overrunCount = 0u;
    std::uint32_t ticksThisFrame = 0u;
    std::uint32_t retryGeneration = 0u;
    int vitality = 0;
    bool paused = false;
    bool playerAlive = false;
};
struct MotionEventRow
{
    std::size_t stateRow = 0u;
    horde::gameplay::simulation::GameplayEvent event{};
};
struct MotionRtRow
{
    std::size_t stateRow = 0u;
    std::size_t pipelineIndex = 0u;
    std::uint64_t surfaceGeneration = 0u;
    RtCompletedFrameIdentity identity{};
    RtResourceInventory resources{};
    RtPlayerDiagnostics player{};
    RtSampleStatus gpuStatus = RtSampleStatus::NotReady;
    RtSampleStatus cpuStatus = RtSampleStatus::NotReady;
    std::uint64_t gpuNanoseconds = 0u;
    std::uint64_t wholeFrameCpuNanoseconds = 0u;
    std::uint64_t simulationCpuNanoseconds = 0u;
    bool gpuDurationAvailable = false;
    std::optional<RtFireLightingEvidence> fireLighting{};
    // Owning successful upload only; absence is historical/unavailable, false is Off.
    std::optional<bool> actualUploadedMistEnabled{};
};
struct MotionScopeRow
{
    std::uint64_t surfaceGeneration = 0u;
    std::uint64_t sceneEpoch = 0u;
    std::uint64_t measurementGeneration = 0u;
    std::size_t nextStateRow = 0u;
};
struct MotionRetirementRow
{
    RtSubmittedFrameIdentity identity{};
    std::uint64_t surfaceGeneration = 0u;
    std::size_t stateRow = 0u;
};

// Debug evidence collector only. Pending bindings use the existing committed
// graphics identity, never a new submission counter or last-snapshot guess.
class MotionEvidenceLedger
{
public:
    static constexpr std::size_t kMaximumStateRows = 16'384u;
    static constexpr std::size_t kMaximumEventRows = 1'024u;
    static constexpr std::size_t kMaximumRtRows = 16'384u;
    static constexpr std::size_t kMaximumPipelines = 8u;
    static constexpr std::size_t kMaximumScopes = 32u;
    static constexpr std::size_t kMaximumRetirements = kMaximumScopes * kRtMaximumFrameSlots;

    [[nodiscard]] bool Begin(std::string_view runId, horde::gameplay::validation::MotionScenario scenario);
    [[nodiscard]] bool AppendState(std::uint64_t now,
        const horde::gameplay::simulation::SimulationSnapshot& snapshot,
        const horde::gameplay::simulation::InputSnapshot& input,
        const horde::gameplay::validation::MotionEvidenceScenario& scenario,
        std::span<const horde::gameplay::simulation::GameplayEvent> events);
    // Publish the actual owner scope after initialization/retry/resize/surface
    // changes. This resets readiness, never drops a frame or pending binding.
    [[nodiscard]] bool ObserveScope(std::uint64_t surfaceGeneration,
                                   std::uint64_t sceneEpoch, std::uint64_t measurementGeneration);
    // Immediately AFTER successful graphics submit, use the existing
    // coordinator's TryGetCommittedIdentity(frameSlot, identity).
    [[nodiscard]] bool BindSubmittedFrame(std::uint64_t surfaceGeneration,
                                         const RtSubmittedFrameIdentity& identity);
    // ONLY after the graphics owner actually retires/cancels this exact old-
    // scope submission. This records cancellation, never RT presentation.
    // Accepted completed frames go through AppendCompletedFrame instead.
    [[nodiscard]] bool RetireSubmittedFrame(std::uint64_t surfaceGeneration,
                                           const RtSubmittedFrameIdentity& identity);
    // AFTER actual owning fence/idle completion and current-scope publication.
    // A duplicate poll of the exact latest completed frame is harmless.
    [[nodiscard]] bool AppendCompletedFrame(std::uint64_t surfaceGeneration,
                                            const RtLifecyclePublishedState& publication);
    [[nodiscard]] bool HasCurrentPresentedFrame(std::uint64_t surfaceGeneration,
        std::uint64_t sceneEpoch, std::uint64_t measurementGeneration) const noexcept;
    [[nodiscard]] bool HasPendingSubmissions() const noexcept;
    [[nodiscard]] bool Failed() const noexcept { return !failure_.empty(); }
    [[nodiscard]] std::string_view Failure() const noexcept { return failure_; }
    [[nodiscard]] std::span<const MotionStateRow> States() const noexcept { return states_; }
    [[nodiscard]] std::span<const MotionEventRow> Events() const noexcept { return events_; }
    [[nodiscard]] std::span<const MotionRtRow> Frames() const noexcept { return frames_; }
    [[nodiscard]] std::size_t ReservedBytes() const noexcept;
    // Streaming avoids an unbounded second full-report allocation. Caller owns
    // the local file and checks stream success; no network/report-send effects.
    void WriteJson(std::ostream& output, const horde::gameplay::validation::MotionEvidenceScenario& scenario) const;

private:
    bool Reject(std::string_view reason);
    struct Pending
    {
        RtSubmittedFrameIdentity identity{};
        std::uint64_t surfaceGeneration = 0u;
        std::size_t stateRow = 0u;
        bool valid = false;
    };
    std::string runId_;
    std::string failure_;
    horde::gameplay::validation::MotionScenario scenario_{};
    std::vector<MotionStateRow> states_;
    std::vector<MotionEventRow> events_;
    std::vector<MotionRtRow> frames_;
    std::vector<RtPipelineEvidenceIdentity> pipelines_;
    std::vector<MotionScopeRow> scopes_;
    std::vector<MotionRetirementRow> retirements_;
    std::array<Pending, kRtMaximumFrameSlots> pending_{};
    std::uint64_t latestEventSequence_ = 0u;
    bool currentScopePresented_ = false;
};

} // namespace horde::telemetry
