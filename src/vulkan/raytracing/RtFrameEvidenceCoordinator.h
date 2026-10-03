#pragma once

#include "telemetry/RtPerformanceEvidence.h"
#include "vulkan/GpuFrameTimer.h"
#include "vulkan/raytracing/RtSceneRecordObservation.h"

#include <array>
#include <cstdint>
#include <string>

namespace horde::vulkan::raytracing
{

class PresentableTinyRtScene;

struct RtGpuTimerStateSnapshot
{
    GpuFrameTimerStatus status = GpuFrameTimerStatus::Uninitialised;
    float timestampPeriodNanoseconds = 0.0f;
    std::uint32_t timestampValidBits = 0u;
    std::uint64_t sampleCount = 0u;
    std::uint64_t unavailableResultCount = 0u;
    std::uint64_t errorCount = 0u;
};

// Fixed owner-thread adapter. Production wraps GpuFrameTimer once; tests may
// inject bounded outcomes without a second submission counter or fake queue.
struct RtGpuFrameTimerIo
{
    void* user = nullptr;
    bool (*markSubmitted)(void*, std::uint32_t, std::uint64_t) noexcept = nullptr;
    void (*cancelRecording)(void*, std::uint32_t) noexcept = nullptr;
    GpuFrameTimingCollection (*collectCompleted)(void*, std::uint32_t) noexcept = nullptr;
    RtGpuTimerStateSnapshot (*snapshot)(void*) noexcept = nullptr;
};

[[nodiscard]] RtGpuFrameTimerIo MakeRtGpuFrameTimerIo(GpuFrameTimer& timer) noexcept;

struct RtDiagnosticCounterPayload
{
    std::array<std::uint32_t, horde::telemetry::kRtDielectricCounterCount> counters{};
};

// Diagnostic callbacks are deliberately output-only. Publication occurs only
// after the lifecycle accepts the exact completed identity.
struct RtDiagnosticFrameIo
{
    void* user = nullptr;
    bool (*collectCompleted)(void*, RtDiagnosticCounterPayload&, std::string&) = nullptr;
    void (*publishCompleted)(void*, const RtDiagnosticCounterPayload&) noexcept = nullptr;
};

[[nodiscard]] RtDiagnosticFrameIo MakeRtDiagnosticFrameIo(
    PresentableTinyRtScene& scene) noexcept;

struct RtEvidenceSubmitTransaction
{
    horde::telemetry::RtEvidenceLifecycle candidate{};
    horde::telemetry::RtSubmittedFrameIdentity identity{};
    bool valid = false;
};

struct RtFrameEvidenceCompletionResult
{
    GpuFrameTimingCollection gpuCollection{};
    bool ownedGraphicsSubmission = false;
    bool gpuCollectionAttempted = false;
    bool completedEvidence = false;
    bool fatalDiagnosticIoFailure = false;
};

class RtFrameEvidenceCoordinator final
{
public:
    [[nodiscard]] bool Initialise(
        const horde::telemetry::RtLifecycleSeeds& preservedSeeds,
        std::uint32_t activeSlotCount,
        horde::telemetry::RtInstrumentationMode instrumentation,
        horde::telemetry::RtSampleStatus initialGpuStatus,
        bool initiallyPaused) noexcept;

    [[nodiscard]] bool Recreate(
        horde::telemetry::RtResourceResetReason reason,
        horde::telemetry::RtSampleStatus initialGpuStatus) noexcept;
    [[nodiscard]] bool Destroy() noexcept;

    [[nodiscard]] bool SetPaused(bool paused) noexcept;
    [[nodiscard]] bool ApplyEvent(
        horde::telemetry::RtLifecycleEvent event,
        horde::telemetry::RtLifecycleResetEffects* effects = nullptr) noexcept;

    [[nodiscard]] bool BeginFrame(
        std::uint32_t frameSlot,
        RtSceneRecordObservation& observation) noexcept;
    [[nodiscard]] bool BeginRecord(std::uint64_t simulationTick) noexcept;
    // False means optional evidence identity was lost. The caller must keep the
    // successfully recorded graphics path alive.
    [[nodiscard]] bool FinishRecord(const RtSceneRecordObservation& observation) noexcept;
    void FailRecord(const RtSceneRecordObservation& observation) noexcept;

    [[nodiscard]] RtEvidenceSubmitTransaction PrevalidateSubmit() noexcept;
    void FailGraphicsSubmit(bool timerRecording, const RtGpuFrameTimerIo& gpuIo) noexcept;
    void CommitGraphicsSubmit(const RtEvidenceSubmitTransaction& transaction,
                              bool timingEnabled,
                              bool timerRecording,
                              const RtGpuFrameTimerIo& gpuIo) noexcept;
    [[nodiscard]] bool AttachPresentation(
        horde::telemetry::RtPresentationOutcome outcome) noexcept;
    void FinalizeSubmittedFrame(const RtSceneRecordObservation& observation) noexcept;
    void AbortFrame() noexcept;

    // Optional output is cleared on entry and filled only by exact lifecycle acceptance.
    // No pointer is retained; callers consume the copied snapshot before resetting an epoch.
    [[nodiscard]] RtFrameEvidenceCompletionResult CompleteFence(
        std::uint32_t frameSlot,
        const RtGpuFrameTimerIo& gpuIo,
        const RtDiagnosticFrameIo& diagnosticIo,
        horde::telemetry::RtPerformanceEvidenceSnapshot* completedOutput = nullptr) noexcept;
    [[nodiscard]] RtFrameEvidenceCompletionResult CompleteFinalIdle(
        std::uint32_t frameSlot,
        const RtGpuFrameTimerIo& gpuIo,
        const RtDiagnosticFrameIo& diagnosticIo,
        horde::telemetry::RtPerformanceEvidenceSnapshot* completedOutput = nullptr) noexcept;
    // Failed device idle owns no host reads. Pending facts are cleared only by
    // the following Recreate/Destroy epoch transition.
    void NoteFailedDeviceIdle() noexcept;

    [[nodiscard]] horde::telemetry::RtLifecycleSeeds SeedsByValue() const noexcept;
    [[nodiscard]] horde::telemetry::RtLifecyclePublishedState PublishedStateByValue() const noexcept
    {
        return lifecycle_.PublishedStateByValue();
    }
    [[nodiscard]] horde::telemetry::RtStageAggregateSet StageAggregatesByValue() const noexcept
    {
        return stageAccumulator_.AggregatesByValue();
    }
    [[nodiscard]] bool ObserverAvailable() const noexcept { return observerAvailable_; }
    [[nodiscard]] bool HasSuccessfulGraphicsSubmission(std::uint32_t frameSlot) const noexcept;
    [[nodiscard]] bool HasSubmittedIdentity(std::uint32_t frameSlot) const noexcept;
    // Prevalidation is not a commit. Failure clears the caller's prior identity.
    [[nodiscard]] bool TryGetCommittedIdentity(
        std::uint32_t frameSlot,
        horde::telemetry::RtSubmittedFrameIdentity& output) const noexcept;
    [[nodiscard]] bool FrameActive() const noexcept { return frameActive_; }

private:
    enum class GpuDisposition : std::uint8_t
    {
        Disabled,
        Unsupported,
        AwaitingTimer,
        Error,
    };

    struct PendingSlot
    {
        bool hasSuccessfulGraphicsSubmission = false;
        bool hasIdentity = false;
        bool hasStages = false;
        horde::telemetry::RtSubmittedFrameIdentity identity{};
        horde::telemetry::RtSubmittedStageSample stages{};
        GpuDisposition gpuDisposition = GpuDisposition::Disabled;
    };

    [[nodiscard]] RtFrameEvidenceCompletionResult CompleteOwnedSlot(
        std::uint32_t frameSlot,
        bool finalIdle,
        const RtGpuFrameTimerIo& gpuIo,
        const RtDiagnosticFrameIo& diagnosticIo,
        horde::telemetry::RtPerformanceEvidenceSnapshot* completedOutput) noexcept;
    [[nodiscard]] horde::telemetry::RtGpuTimingEvidence BuildGpuEvidence(
        const PendingSlot& pending,
        const GpuFrameTimingCollection* collection,
        const RtGpuTimerStateSnapshot& timer) noexcept;
    void ApplyDeferredAggregateReset() noexcept;
    void LoseCurrentIdentity() noexcept;
    void ResetAttemptState() noexcept;
    [[nodiscard]] horde::telemetry::RtSampleStatus InitialDiagnosticStatus() const noexcept;

    horde::telemetry::RtEvidenceLifecycle lifecycle_{};
    // Monotonic floor supplied by the owning platform. It is not advanced by
    // the coordinator; it only prevents a failed pure-lifecycle initialise
    // from returning default/lower seeds across Android context movement.
    horde::telemetry::RtLifecycleSeeds preservedSeedFloor_{};
    horde::telemetry::RtStageAccumulator stageAccumulator_{};
    RtSceneCommandObservation commandObservation_{};
    std::array<PendingSlot, horde::telemetry::kRtMaximumFrameSlots> pending_{};
    horde::telemetry::RtRecordedSceneEvidence recordedSceneScratch_{};
    horde::telemetry::RtFrameToken attempt_{};
    horde::telemetry::RtFrameToken recorded_{};
    std::uint64_t diagnosticReadCount_ = 0u;
    std::uint64_t diagnosticResetCount_ = 0u;
    std::uint32_t activeSlotCount_ = 0u;
    std::uint32_t activeFrameSlot_ = 0u;
    horde::telemetry::RtInstrumentationMode instrumentation_ =
        horde::telemetry::RtInstrumentationMode::Shipping;
    bool frameActive_ = false;
    bool attemptValid_ = false;
    bool recordedValid_ = false;
    bool deferredAggregateReset_ = false;
    bool observerAvailable_ = false;
    bool desiredPaused_ = false;
    bool initialised_ = false;
};

[[nodiscard]] horde::telemetry::RtSampleStatus InitialRtGpuEvidenceStatus(
    bool timingEnabled,
    const RtGpuTimerStateSnapshot& timer) noexcept;

} // namespace horde::vulkan::raytracing
