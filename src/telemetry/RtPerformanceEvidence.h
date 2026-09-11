#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace horde::telemetry
{

inline constexpr std::uint32_t kRtPerformanceEvidenceSchema = 1u;
inline constexpr std::size_t kRtDielectricCounterCount = 41u;
inline constexpr std::size_t kRtMaximumFrameSlots = 4u;
inline constexpr std::size_t kRtEvidenceSampleCapacity = 128u;

template <std::size_t Capacity>
struct RtFixedText
{
    static_assert(Capacity > 0u);
    std::array<char, Capacity> value{};
};

template <std::size_t Capacity>
[[nodiscard]] bool AssignRtFixedText(RtFixedText<Capacity>& destination,
                                     const std::string_view value) noexcept
{
    if (value.size() >= Capacity)
    {
        return false;
    }
    for (const char character : value)
    {
        if (character == '\0')
        {
            return false;
        }
    }
    RtFixedText<Capacity> candidate{};
    for (std::size_t index = 0u; index < value.size(); ++index)
    {
        candidate.value[index] = value[index];
    }
    destination = candidate;
    return true;
}

template <std::size_t Capacity>
[[nodiscard]] std::string_view RtFixedTextView(const RtFixedText<Capacity>& value) noexcept
{
    std::size_t length = 0u;
    while (length < Capacity && value.value[length] != '\0')
    {
        ++length;
    }
    return std::string_view(value.value.data(), length);
}

enum class RtSampleStatus : std::uint8_t
{
    NotReady,
    CompiledOut,
    Disabled,
    Unsupported,
    Pending,
    Valid,
    Error,
};

enum class RtPresentationOutcome : std::uint8_t
{
    Presented,
    NotPresentedNeedsRecreate,
    Failed,
    NotAttempted,
};

enum class RtInstrumentationMode : std::uint8_t
{
    Shipping,
    Diagnostic,
};

enum class RtDielectricQuality : std::uint8_t
{
    Mobile,
    High,
};

enum class RtMaterialStrategy : std::uint8_t
{
    OpaqueFast,
    GenericDielectric,
};

enum class RtWaterQuality : std::uint8_t
{
    Off,
    Mobile,
    High,
};

[[nodiscard]] const char* RtSampleStatusName(RtSampleStatus status) noexcept;
[[nodiscard]] const char* RtPresentationOutcomeName(RtPresentationOutcome outcome) noexcept;
[[nodiscard]] const char* RtInstrumentationModeName(RtInstrumentationMode mode) noexcept;
[[nodiscard]] const char* RtDielectricQualityName(RtDielectricQuality quality) noexcept;
[[nodiscard]] const char* RtMaterialStrategyName(RtMaterialStrategy strategy) noexcept;
[[nodiscard]] const char* RtWaterQualityName(RtWaterQuality quality) noexcept;
[[nodiscard]] constexpr bool RtSampleAvailable(const RtSampleStatus status) noexcept
{
    return status == RtSampleStatus::Valid;
}

struct RtFrameToken
{
    std::uint64_t sceneEpoch = 0u;
    std::uint64_t measurementGeneration = 0u;
    std::uint64_t recordAttemptSerial = 0u;
    std::uint64_t recordSerial = 0u;
    std::uint64_t simulationTick = 0u;
    std::uint32_t frameSlot = 0u;
};

struct RtSubmittedFrameIdentity
{
    RtFrameToken frame{};
    std::uint64_t submissionSerial = 0u;
};

struct RtCompletedFrameIdentity
{
    RtSubmittedFrameIdentity submitted{};
    std::uint64_t completionSerial = 0u;
};

struct RtShaderArtifactIdentity
{
    RtFixedText<96u> key{};
    RtFixedText<65u> sha256{};
};

struct RtPipelineEvidenceIdentity
{
    RtInstrumentationMode instrumentation = RtInstrumentationMode::Shipping;
    RtDielectricQuality dielectricQuality = RtDielectricQuality::Mobile;
    RtMaterialStrategy activeStrategy = RtMaterialStrategy::OpaqueFast;
    RtWaterQuality waterQuality = RtWaterQuality::Off;
    // Canonical pair key only; exact per-artifact and active identities remain separate below.
    RtFixedText<96u> bundleKey{};
    RtShaderArtifactIdentity opaqueFast{};
    RtShaderArtifactIdentity genericDielectric{};
    RtShaderArtifactIdentity active{};
};

struct RtResourceInventory
{
    std::uint32_t bufferCount = 0u;
    std::uint32_t memoryAllocationCount = 0u;
    std::uint32_t bottomLevelAccelerationStructureCount = 0u;
    std::uint32_t topLevelAccelerationStructureCount = 0u;
    std::uint32_t tlasInstanceCount = 0u;
    std::uint32_t pipelineCount = 0u;
    std::uint32_t shaderBindingTableCount = 0u;
    std::uint32_t descriptorSetCount = 0u;
    std::uint64_t hostVisibleBytes = 0u;
    std::uint64_t deviceLocalBytes = 0u;
};

struct RtPlayerDiagnostics
{
    std::uint32_t skinCadenceHz = 0u;
    std::uint64_t skinUpdateCount = 0u;
    std::uint64_t maximumSocketErrorMicrometres = 0u;
    std::uint32_t primaryPixelCount = 0u;
    bool primaryVisible = false;
};

enum class RtStage : std::uint8_t
{
    SimulationStep,
    Skin,
    PlayerSkin,
    CharacterSkin,
    DynamicUpload,
    BlasRefitRecord,
    TlasUpdateRecord,
    TraceCopyRecord,
    FrameFenceWait,
    ImageAcquire,
    QueueSubmit,
    PresentCall,
    WholeFrameCycle,
    Count,
};

inline constexpr std::size_t kRtStageCount = static_cast<std::size_t>(RtStage::Count);

[[nodiscard]] constexpr std::size_t RtStageIndex(const RtStage stage) noexcept
{
    return static_cast<std::size_t>(stage);
}

[[nodiscard]] const char* RtStageMetricName(RtStage stage) noexcept;

struct RtStageValue
{
    std::uint64_t durationNanoseconds = 0u;
    std::uint64_t workInvocationCount = 0u;
    std::uint64_t byteCount = 0u;
    std::uint64_t operationCount = 0u;
    bool overflowed = false;
};

struct RtStageFrameSample
{
    RtSampleStatus status = RtSampleStatus::NotReady;
    std::array<RtStageValue, kRtStageCount> values{};
};

struct RtSubmittedStageSample
{
    RtSubmittedFrameIdentity identity{};
    RtStageFrameSample stages{};
};

struct RtStageAggregate
{
    std::uint64_t latestNanoseconds = 0u;
    std::uint64_t sumNanoseconds = 0u;
    std::uint64_t sampleCount = 0u;
    std::uint64_t maxNanoseconds = 0u;
    std::uint64_t workInvocationCount = 0u;
    std::uint64_t byteCount = 0u;
    std::uint64_t operationCount = 0u;
    bool overflowed = false;
    bool statisticsValid = true;
};

struct RtStageAggregateSet
{
    std::array<RtStageAggregate, kRtStageCount> values{};
};

[[nodiscard]] bool CheckedMillisecondsToNanoseconds(double milliseconds,
                                                    std::uint64_t& nanoseconds) noexcept;
[[nodiscard]] bool AccumulateCommittedStage(RtStageAggregate& aggregate,
                                            const RtStageValue& sample) noexcept;

class RtStageAccumulator
{
public:
    [[nodiscard]] bool Begin() noexcept;
    [[nodiscard]] bool Accumulate(RtStage stage,
                                  std::uint64_t durationNanoseconds,
                                  std::uint64_t workInvocationCount = 0u,
                                  std::uint64_t byteCount = 0u,
                                  std::uint64_t operationCount = 0u) noexcept;
    [[nodiscard]] bool Commit(RtStageFrameSample& output) noexcept;
    [[nodiscard]] bool Abort() noexcept;
    [[nodiscard]] bool ResetAggregates() noexcept;
    [[nodiscard]] RtStageAggregateSet AggregatesByValue() const noexcept { return aggregates_; }
    [[nodiscard]] bool Active() const noexcept { return active_; }

private:
    std::array<RtStageValue, kRtStageCount> scratch_{};
    RtStageAggregateSet aggregates_{};
    bool active_ = false;
};

struct RtDispatchEvidence
{
    bool sceneReady = false;
    bool rtDispatchRecorded = false;
    bool swapchainCopyRecorded = false;
};

struct RtRecordedSceneEvidence
{
    RtPipelineEvidenceIdentity pipeline{};
    RtResourceInventory resources{};
    RtPlayerDiagnostics player{};
    RtDispatchEvidence dispatch{};
};

struct RtSceneFrameEvidence
{
    RtPipelineEvidenceIdentity pipeline{};
    RtResourceInventory resources{};
    RtPlayerDiagnostics player{};
    RtStageFrameSample stages{};
    RtDispatchEvidence dispatch{};
};

struct RtDiagnosticEvidence
{
    RtSampleStatus status = RtSampleStatus::NotReady;
    bool compiled = false;
    bool hasCounters = false;
    std::uint64_t completedSubmissionSerial = 0u;
    std::uint64_t readCount = 0u;
    std::uint64_t resetCount = 0u;
    std::array<std::uint32_t, kRtDielectricCounterCount> counters{};
    RtFixedText<96u> detail{};
};

struct RtGpuTimingEvidence
{
    RtSampleStatus status = RtSampleStatus::NotReady;
    bool hasDuration = false;
    std::uint64_t durationNanoseconds = 0u;
    std::uint64_t completedSubmissionSerial = 0u;
    std::uint64_t timestampPeriodPicoseconds = 0u;
    std::uint64_t sampleCount = 0u;
    std::uint64_t unavailableResultCount = 0u;
    std::uint64_t errorCount = 0u;
    std::uint32_t timestampValidBits = 0u;
    RtFixedText<96u> detail{};
};

struct RtPresentationEvidence
{
    RtPresentationOutcome outcome = RtPresentationOutcome::NotAttempted;
    std::uint64_t lastSuccessfulPresentSubmissionSerial = 0u;
    bool finalIdleCompletion = false;
};

struct RtPerformanceEvidenceSnapshot
{
    std::uint32_t schema = kRtPerformanceEvidenceSchema;
    RtCompletedFrameIdentity identity{};
    RtSceneFrameEvidence scene{};
    RtDiagnosticEvidence dielectric{};
    RtGpuTimingEvidence gpu{};
    RtPresentationEvidence presentation{};
    bool benchmarkEligible = false;
};

struct RtCompletedStageSample
{
    RtCompletedFrameIdentity identity{};
    RtStageFrameSample stages{};
    bool benchmarkEligible = false;
};

struct RtStageStatistics
{
    bool valid = false;
    std::uint64_t sampleCount = 0u;
    double meanMilliseconds = 0.0;
    double medianMilliseconds = 0.0;
    double p90Milliseconds = 0.0;
    double p95Milliseconds = 0.0;
    double onePercentLowFps = 0.0;
};

class RtStageSampleCollector
{
public:
    [[nodiscard]] bool Start(std::uint64_t sceneEpoch,
                             std::uint64_t measurementGeneration) noexcept;
    [[nodiscard]] bool Append(const RtCompletedStageSample& sample) noexcept;
    [[nodiscard]] bool Statistics(RtStage stage, RtStageStatistics& output) const noexcept;
    void Invalidate() noexcept { invalidRun_ = true; }

    [[nodiscard]] std::size_t Size() const noexcept { return size_; }
    [[nodiscard]] std::uint64_t OverflowCount() const noexcept { return overflowCount_; }
    [[nodiscard]] std::uint64_t IdentityRejectCount() const noexcept { return identityRejectCount_; }
    [[nodiscard]] bool InvalidRun() const noexcept { return invalidRun_; }
    [[nodiscard]] bool CompleteReportEligible() const noexcept
    {
        return configured_ && size_ > 0u && !invalidRun_;
    }

private:
    std::array<RtCompletedStageSample, kRtEvidenceSampleCapacity> samples_{};
    std::uint64_t sceneEpoch_ = 0u;
    std::uint64_t measurementGeneration_ = 0u;
    std::uint64_t lastRecordAttemptSerial_ = 0u;
    std::uint64_t lastRecordSerial_ = 0u;
    std::uint64_t lastSubmissionSerial_ = 0u;
    std::uint64_t lastCompletionSerial_ = 0u;
    std::uint64_t overflowCount_ = 0u;
    std::uint64_t identityRejectCount_ = 0u;
    std::size_t size_ = 0u;
    bool configured_ = false;
    bool invalidRun_ = false;
};

enum class RtEvidenceValidationError : std::uint8_t
{
    None,
    UnknownSchema,
    UnknownEnum,
    UnterminatedText,
    EmptyIdentity,
    MalformedHash,
    InconsistentIdentity,
    InvalidDiagnosticState,
    InvalidGpuState,
    NonValidNumericSample,
    InvalidStageSample,
    InvalidDispatchState,
    InvalidPresentationState,
};

[[nodiscard]] bool ValidateRtPerformanceEvidence(
    const RtPerformanceEvidenceSnapshot& snapshot,
    RtEvidenceValidationError& error) noexcept;
[[nodiscard]] bool SerializeRtPerformanceEvidenceJson(
    const RtPerformanceEvidenceSnapshot& snapshot,
    std::string& output,
    RtEvidenceValidationError& error);
[[nodiscard]] bool SerializeRtPerformanceEvidenceText(
    const RtPerformanceEvidenceSnapshot& snapshot,
    std::string& output,
    RtEvidenceValidationError& error);

struct RtLifecycleSeeds
{
    std::uint64_t sceneEpoch = 0u;
    std::uint64_t measurementGeneration = 0u;
    std::uint64_t recordAttemptSerial = 0u;
    std::uint64_t successfulRecordSerial = 0u;
    std::uint64_t submissionSerial = 0u;
    std::uint64_t completionSerial = 0u;
};

struct RtLifecycleResetEffects
{
    bool epochChanged = false;
    bool measurementGenerationChanged = false;
    bool pendingSubmissionsInvalidated = false;
    bool collectorCleared = false;
    bool collectorInvalidated = false;
    bool clearGpuAggregates = false;
    bool nextSampleEligible = false;
    bool restartFrameCycleClock = false;
};

enum class RtLifecycleEvent : std::uint8_t
{
    BenchmarkStart,
    WarmupToMeasure,
    RouteReset,
    Retry,
    CheckpointChange,
    GpuTimingEnabled,
    GpuTimingDisabled,
    Pause,
    Resume,
};

enum class RtResourceResetReason : std::uint8_t
{
    Initialise,
    SwapchainRecreate,
    RenderScaleChange,
    SurfaceReplacement,
    DeviceReplacement,
    DiagnosticResourceReplacement,
    TimestampResourceReplacement,
    DestroyOrSurfaceStop,
};

struct RtLifecyclePublishedState
{
    std::uint64_t sceneEpoch = 0u;
    std::uint64_t measurementGeneration = 0u;
    RtSampleStatus diagnosticStatus = RtSampleStatus::NotReady;
    RtSampleStatus gpuStatus = RtSampleStatus::NotReady;
    RtResourceResetReason lastResourceReset = RtResourceResetReason::Initialise;
    bool running = false;
    bool paused = false;
    bool presented = false;
    bool hasCompletedEvidence = false;
    RtPerformanceEvidenceSnapshot completedEvidence{};
};

class RtEvidenceLifecycle
{
public:
    [[nodiscard]] bool Initialise(const RtLifecycleSeeds& preservedSeeds,
                                  std::uint32_t activeSlotCount,
                                  RtSampleStatus initialDiagnosticStatus,
                                  RtSampleStatus initialGpuStatus,
                                  RtLifecycleResetEffects& effects) noexcept;
    [[nodiscard]] bool Recreate(RtResourceResetReason reason,
                                RtSampleStatus initialDiagnosticStatus,
                                RtSampleStatus initialGpuStatus,
                                RtLifecycleResetEffects& effects) noexcept;
    [[nodiscard]] bool Destroy(RtLifecycleResetEffects& effects) noexcept;
    [[nodiscard]] bool ApplyEvent(RtLifecycleEvent event,
                                  RtLifecycleResetEffects& effects) noexcept;

    [[nodiscard]] bool BeginRecord(std::uint32_t frameSlot,
                                   std::uint64_t simulationTick,
                                   RtFrameToken& attempt) noexcept;
    [[nodiscard]] bool FinishRecord(const RtFrameToken& attempt,
                                    const RtRecordedSceneEvidence& evidence,
                                    RtFrameToken& recorded) noexcept;
    [[nodiscard]] bool AbortRecord(const RtFrameToken& attempt) noexcept;
    [[nodiscard]] bool FailDiagnosticReset(const RtFrameToken& attempt,
                                           RtLifecycleResetEffects& effects) noexcept;
    [[nodiscard]] bool FailSubmit(const RtFrameToken& recorded) noexcept;
    [[nodiscard]] bool Submit(const RtFrameToken& recorded,
                              RtSubmittedFrameIdentity& submitted) noexcept;
    [[nodiscard]] bool AttachPresentation(const RtSubmittedFrameIdentity& submitted,
                                          RtPresentationOutcome outcome) noexcept;
    [[nodiscard]] bool CompleteFence(const RtSubmittedFrameIdentity& submitted,
                                     const RtSubmittedStageSample& stages,
                                     const RtDiagnosticEvidence& diagnostic,
                                     const RtGpuTimingEvidence& gpu,
                                     RtPerformanceEvidenceSnapshot& completed) noexcept;
    [[nodiscard]] bool CompleteFinalIdle(const RtSubmittedFrameIdentity& submitted,
                                         const RtSubmittedStageSample& stages,
                                         const RtDiagnosticEvidence& diagnostic,
                                         const RtGpuTimingEvidence& gpu,
                                         RtPerformanceEvidenceSnapshot& completed) noexcept;

    [[nodiscard]] RtLifecyclePublishedState PublishedStateByValue() const noexcept
    {
        return published_;
    }
    [[nodiscard]] RtPerformanceEvidenceSnapshot CompletedEvidenceByValue() const noexcept
    {
        return published_.completedEvidence;
    }
    [[nodiscard]] RtLifecycleSeeds SeedsByValue() const noexcept { return seeds_; }

private:
    enum class SlotPhase : std::uint8_t
    {
        Empty,
        Recording,
        Recorded,
        Submitted,
    };

    struct SlotState
    {
        SlotPhase phase = SlotPhase::Empty;
        RtFrameToken token{};
        RtRecordedSceneEvidence scene{};
        std::uint64_t submissionSerial = 0u;
        bool presentationAttached = false;
        RtPresentationOutcome presentation = RtPresentationOutcome::NotAttempted;
    };

    [[nodiscard]] bool ResetResources(RtResourceResetReason reason,
                                      bool running,
                                      RtSampleStatus diagnosticStatus,
                                      RtSampleStatus gpuStatus,
                                      RtLifecycleResetEffects& effects) noexcept;
    [[nodiscard]] bool Complete(const RtSubmittedFrameIdentity& submitted,
                                const RtSubmittedStageSample& stages,
                                const RtDiagnosticEvidence& diagnostic,
                                const RtGpuTimingEvidence& gpu,
                                bool finalIdle,
                                RtPerformanceEvidenceSnapshot& completed) noexcept;
    [[nodiscard]] bool ValidTokenForSlot(const RtFrameToken& token,
                                         SlotPhase phase) const noexcept;
    void ClearSlots() noexcept;

    RtLifecycleSeeds seeds_{};
    std::array<SlotState, kRtMaximumFrameSlots> slots_{};
    std::uint32_t activeSlotCount_ = 0u;
    std::uint64_t lastSuccessfulPresentSubmissionSerial_ = 0u;
    RtLifecyclePublishedState published_{};
    bool measurementSamplesEligible_ = false;
    bool diagnosticFaultLatched_ = false;
    bool initialised_ = false;
};

} // namespace horde::telemetry
