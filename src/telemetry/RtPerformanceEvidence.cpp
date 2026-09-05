#include "telemetry/RtPerformanceEvidence.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace horde::telemetry
{
namespace
{

template <typename Enum>
bool KnownEnumName(const char* (*name)(Enum), const Enum value) noexcept
{
    return name(value) != nullptr;
}

template <std::size_t Capacity>
bool CanonicalFixedText(const RtFixedText<Capacity>& text) noexcept
{
    const std::string_view view = RtFixedTextView(text);
    if (view.size() == Capacity)
    {
        return false;
    }
    for (std::size_t index = view.size() + 1u; index < Capacity; ++index)
    {
        if (text.value[index] != '\0')
        {
            return false;
        }
    }
    for (const unsigned char character : view)
    {
        if (character < 0x20u || character > 0x7eu)
        {
            return false;
        }
    }
    return true;
}

template <std::size_t Capacity>
bool ValidFixedText(const RtFixedText<Capacity>& text, const bool requireNonEmpty) noexcept
{
    return CanonicalFixedText(text) && (!requireNonEmpty || !RtFixedTextView(text).empty());
}

bool ValidSha256(const RtFixedText<65u>& hash) noexcept
{
    const std::string_view value = RtFixedTextView(hash);
    if (value.size() != 64u)
    {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](const char character) {
        return (character >= '0' && character <= '9') ||
               (character >= 'a' && character <= 'f');
    });
}

bool SameArtifact(const RtShaderArtifactIdentity& left,
                  const RtShaderArtifactIdentity& right) noexcept
{
    return left.key.value == right.key.value && left.sha256.value == right.sha256.value;
}

bool ValidPipelineIdentity(const RtPipelineEvidenceIdentity& pipeline,
                           RtEvidenceValidationError& error) noexcept
{
    if (!KnownEnumName(RtInstrumentationModeName, pipeline.instrumentation) ||
        !KnownEnumName(RtDielectricQualityName, pipeline.dielectricQuality) ||
        !KnownEnumName(RtMaterialStrategyName, pipeline.activeStrategy) ||
        !KnownEnumName(RtWaterQualityName, pipeline.waterQuality))
    {
        error = RtEvidenceValidationError::UnknownEnum;
        return false;
    }
    if (!CanonicalFixedText(pipeline.bundleKey) ||
        !CanonicalFixedText(pipeline.opaqueFast.key) ||
        !CanonicalFixedText(pipeline.genericDielectric.key) ||
        !CanonicalFixedText(pipeline.active.key))
    {
        error = RtEvidenceValidationError::UnterminatedText;
        return false;
    }
    if (!ValidFixedText(pipeline.bundleKey, true) ||
        !ValidFixedText(pipeline.opaqueFast.key, true) ||
        !ValidFixedText(pipeline.genericDielectric.key, true) ||
        !ValidFixedText(pipeline.active.key, true))
    {
        error = RtEvidenceValidationError::EmptyIdentity;
        return false;
    }
    if (!ValidSha256(pipeline.opaqueFast.sha256) ||
        !ValidSha256(pipeline.genericDielectric.sha256) ||
        !ValidSha256(pipeline.active.sha256))
    {
        error = RtEvidenceValidationError::MalformedHash;
        return false;
    }
    const RtShaderArtifactIdentity& selected =
        pipeline.activeStrategy == RtMaterialStrategy::OpaqueFast
            ? pipeline.opaqueFast
            : pipeline.genericDielectric;
    if (!SameArtifact(pipeline.active, selected))
    {
        error = RtEvidenceValidationError::InconsistentIdentity;
        return false;
    }
    return true;
}

bool ValidStageFrame(const RtStageFrameSample& stages,
                     RtEvidenceValidationError& error) noexcept
{
    if (!KnownEnumName(RtSampleStatusName, stages.status))
    {
        error = RtEvidenceValidationError::UnknownEnum;
        return false;
    }
    if (stages.status != RtSampleStatus::Valid && stages.status != RtSampleStatus::Error)
    {
        error = RtEvidenceValidationError::InvalidStageSample;
        return false;
    }
    bool anyOverflow = false;
    for (const RtStageValue& stage : stages.values)
    {
        anyOverflow = anyOverflow || stage.overflowed;
    }
    if ((stages.status == RtSampleStatus::Valid && anyOverflow) ||
        (stages.status == RtSampleStatus::Error && !anyOverflow))
    {
        error = RtEvidenceValidationError::InvalidStageSample;
        return false;
    }
    if (stages.status == RtSampleStatus::Valid)
    {
        const RtStageValue& total = stages.values[RtStageIndex(RtStage::Skin)];
        const RtStageValue& player = stages.values[RtStageIndex(RtStage::PlayerSkin)];
        const RtStageValue& character = stages.values[RtStageIndex(RtStage::CharacterSkin)];
        const auto exactSum = [](const std::uint64_t left,
                                 const std::uint64_t right,
                                 const std::uint64_t expected) {
            return right <= std::numeric_limits<std::uint64_t>::max() - left &&
                   left + right == expected;
        };
        if (!exactSum(player.durationNanoseconds,
                      character.durationNanoseconds,
                      total.durationNanoseconds) ||
            !exactSum(player.workInvocationCount,
                      character.workInvocationCount,
                      total.workInvocationCount) ||
            !exactSum(player.byteCount, character.byteCount, total.byteCount) ||
            !exactSum(player.operationCount, character.operationCount, total.operationCount))
        {
            error = RtEvidenceValidationError::InvalidStageSample;
            return false;
        }
    }
    return true;
}

bool ValidSceneFrame(const RtSceneFrameEvidence& scene,
                     RtEvidenceValidationError& error) noexcept
{
    if (!ValidPipelineIdentity(scene.pipeline, error))
    {
        return false;
    }
    if (!scene.dispatch.sceneReady || !scene.dispatch.rtDispatchRecorded ||
        !scene.dispatch.swapchainCopyRecorded)
    {
        error = RtEvidenceValidationError::InvalidDispatchState;
        return false;
    }
    return ValidStageFrame(scene.stages, error);
}

bool ValidRecordedScene(const RtRecordedSceneEvidence& scene,
                         RtEvidenceValidationError& error) noexcept
{
    if (!ValidPipelineIdentity(scene.pipeline, error))
    {
        return false;
    }
    if (!scene.dispatch.sceneReady || !scene.dispatch.rtDispatchRecorded ||
        !scene.dispatch.swapchainCopyRecorded)
    {
        error = RtEvidenceValidationError::InvalidDispatchState;
        return false;
    }
    return true;
}

bool ValidDiagnostic(const RtDiagnosticEvidence& diagnostic,
                     const RtInstrumentationMode instrumentation,
                     const std::uint64_t submissionSerial,
                     RtEvidenceValidationError& error) noexcept
{
    if (!KnownEnumName(RtSampleStatusName, diagnostic.status))
    {
        error = RtEvidenceValidationError::UnknownEnum;
        return false;
    }
    if (!CanonicalFixedText(diagnostic.detail))
    {
        error = RtEvidenceValidationError::UnterminatedText;
        return false;
    }
    const bool anyCounter = std::any_of(
        diagnostic.counters.begin(), diagnostic.counters.end(),
        [](const std::uint32_t value) { return value != 0u; });
    if (instrumentation == RtInstrumentationMode::Shipping)
    {
        if (diagnostic.status != RtSampleStatus::CompiledOut || diagnostic.compiled ||
            diagnostic.hasCounters || anyCounter || diagnostic.completedSubmissionSerial != 0u ||
            diagnostic.readCount != 0u || diagnostic.resetCount != 0u)
        {
            error = diagnostic.hasCounters || anyCounter
                ? RtEvidenceValidationError::NonValidNumericSample
                : RtEvidenceValidationError::InvalidDiagnosticState;
            return false;
        }
        return true;
    }

    if (!diagnostic.compiled)
    {
        error = RtEvidenceValidationError::InvalidDiagnosticState;
        return false;
    }
    if (diagnostic.status == RtSampleStatus::Valid)
    {
        if (!diagnostic.hasCounters || diagnostic.completedSubmissionSerial != submissionSerial ||
            diagnostic.readCount == 0u || diagnostic.resetCount == 0u)
        {
            error = RtEvidenceValidationError::InconsistentIdentity;
            return false;
        }
        return true;
    }
    if (diagnostic.hasCounters || anyCounter)
    {
        error = RtEvidenceValidationError::NonValidNumericSample;
        return false;
    }
    if (diagnostic.status == RtSampleStatus::Pending &&
        diagnostic.completedSubmissionSerial == 0u)
    {
        return true;
    }
    if (diagnostic.status == RtSampleStatus::Error &&
        diagnostic.completedSubmissionSerial == submissionSerial)
    {
        return true;
    }
    error = RtEvidenceValidationError::InvalidDiagnosticState;
    return false;
}

bool ValidGpu(const RtGpuTimingEvidence& gpu,
              const std::uint64_t submissionSerial,
              RtEvidenceValidationError& error) noexcept
{
    if (!KnownEnumName(RtSampleStatusName, gpu.status))
    {
        error = RtEvidenceValidationError::UnknownEnum;
        return false;
    }
    if (!CanonicalFixedText(gpu.detail))
    {
        error = RtEvidenceValidationError::UnterminatedText;
        return false;
    }
    if (gpu.status == RtSampleStatus::Valid)
    {
        if (!gpu.hasDuration || gpu.completedSubmissionSerial != submissionSerial ||
            gpu.sampleCount == 0u || gpu.timestampValidBits == 0u ||
            gpu.timestampValidBits > 64u || gpu.timestampPeriodPicoseconds == 0u)
        {
            error = RtEvidenceValidationError::InconsistentIdentity;
            return false;
        }
        return true;
    }
    if (gpu.hasDuration || gpu.durationNanoseconds != 0u)
    {
        error = RtEvidenceValidationError::NonValidNumericSample;
        return false;
    }
    if (gpu.status == RtSampleStatus::Error)
    {
        if (gpu.completedSubmissionSerial != submissionSerial)
        {
            error = RtEvidenceValidationError::InconsistentIdentity;
            return false;
        }
        return true;
    }
    if ((gpu.status == RtSampleStatus::Disabled ||
         gpu.status == RtSampleStatus::Unsupported ||
         gpu.status == RtSampleStatus::Pending) &&
        gpu.completedSubmissionSerial == 0u)
    {
        return true;
    }
    error = RtEvidenceValidationError::InvalidGpuState;
    return false;
}

bool SaturatingAdd(std::uint64_t& destination, const std::uint64_t value) noexcept
{
    if (value > std::numeric_limits<std::uint64_t>::max() - destination)
    {
        destination = std::numeric_limits<std::uint64_t>::max();
        return false;
    }
    destination += value;
    return true;
}

bool SameFrameToken(const RtFrameToken& left, const RtFrameToken& right) noexcept
{
    return left.sceneEpoch == right.sceneEpoch &&
           left.measurementGeneration == right.measurementGeneration &&
           left.recordAttemptSerial == right.recordAttemptSerial &&
           left.recordSerial == right.recordSerial &&
           left.simulationTick == right.simulationTick &&
           left.frameSlot == right.frameSlot;
}

bool SameSubmitted(const RtSubmittedFrameIdentity& left,
                   const RtSubmittedFrameIdentity& right) noexcept
{
    return SameFrameToken(left.frame, right.frame) &&
           left.submissionSerial == right.submissionSerial;
}

bool ValidInitialDiagnosticStatus(const RtSampleStatus status) noexcept
{
    return status == RtSampleStatus::NotReady || status == RtSampleStatus::CompiledOut ||
           status == RtSampleStatus::Pending;
}

bool ValidInitialGpuStatus(const RtSampleStatus status) noexcept
{
    return status == RtSampleStatus::NotReady || status == RtSampleStatus::Disabled ||
           status == RtSampleStatus::Unsupported || status == RtSampleStatus::Pending;
}

void WriteJsonEscaped(std::ostream& output, const std::string_view value)
{
    for (const char character : value)
    {
        switch (character)
        {
        case '\\': output << "\\\\"; break;
        case '"': output << "\\\""; break;
        case '\b': output << "\\b"; break;
        case '\f': output << "\\f"; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default: output << character; break;
        }
    }
}

void WriteMilliseconds(std::ostream& output, const std::uint64_t nanoseconds)
{
    const std::uint64_t whole = nanoseconds / 1'000'000u;
    const std::uint64_t fraction = nanoseconds % 1'000'000u;
    output << whole << '.' << std::setfill('0') << std::setw(6) << fraction << std::setfill(' ');
}

void WriteJsonStageArray(std::ostream& output,
                         const RtStageFrameSample& stages,
                         const std::uint8_t field)
{
    output << '[';
    for (std::size_t index = 0u; index < stages.values.size(); ++index)
    {
        if (index > 0u)
        {
            output << ',';
        }
        if (stages.status != RtSampleStatus::Valid)
        {
            output << "null";
            continue;
        }
        switch (field)
        {
        case 0u: output << stages.values[index].workInvocationCount; break;
        case 1u: output << stages.values[index].byteCount; break;
        default: output << stages.values[index].operationCount; break;
        }
    }
    output << ']';
}

bool IncrementSaturatingCounter(std::uint64_t& value) noexcept
{
    if (value == std::numeric_limits<std::uint64_t>::max())
    {
        return false;
    }
    ++value;
    return true;
}

} // namespace

const char* RtSampleStatusName(const RtSampleStatus status) noexcept
{
    switch (status)
    {
    case RtSampleStatus::NotReady: return "not-ready";
    case RtSampleStatus::CompiledOut: return "compiled-out";
    case RtSampleStatus::Disabled: return "disabled";
    case RtSampleStatus::Unsupported: return "unsupported";
    case RtSampleStatus::Pending: return "pending";
    case RtSampleStatus::Valid: return "valid";
    case RtSampleStatus::Error: return "error";
    default: return nullptr;
    }
}

const char* RtPresentationOutcomeName(const RtPresentationOutcome outcome) noexcept
{
    switch (outcome)
    {
    case RtPresentationOutcome::Presented: return "presented";
    case RtPresentationOutcome::NotPresentedNeedsRecreate:
        return "not-presented-needs-recreate";
    case RtPresentationOutcome::Failed: return "failed";
    case RtPresentationOutcome::NotAttempted: return "not-attempted";
    default: return nullptr;
    }
}

const char* RtInstrumentationModeName(const RtInstrumentationMode mode) noexcept
{
    switch (mode)
    {
    case RtInstrumentationMode::Shipping: return "shipping";
    case RtInstrumentationMode::Diagnostic: return "diagnostic";
    default: return nullptr;
    }
}

const char* RtDielectricQualityName(const RtDielectricQuality quality) noexcept
{
    switch (quality)
    {
    case RtDielectricQuality::Mobile: return "mobile";
    case RtDielectricQuality::High: return "high";
    default: return nullptr;
    }
}

const char* RtMaterialStrategyName(const RtMaterialStrategy strategy) noexcept
{
    switch (strategy)
    {
    case RtMaterialStrategy::OpaqueFast: return "opaque-fast";
    case RtMaterialStrategy::GenericDielectric: return "generic-dielectric";
    default: return nullptr;
    }
}

const char* RtWaterQualityName(const RtWaterQuality quality) noexcept
{
    switch (quality)
    {
    case RtWaterQuality::Off: return "off";
    case RtWaterQuality::Mobile: return "mobile";
    case RtWaterQuality::High: return "high";
    default: return nullptr;
    }
}

const char* RtStageMetricName(const RtStage stage) noexcept
{
    switch (stage)
    {
    case RtStage::SimulationStep: return "simulationStepCpuMs";
    case RtStage::Skin: return "skinCpuMs";
    case RtStage::PlayerSkin: return "playerSkinCpuMs";
    case RtStage::CharacterSkin: return "characterSkinCpuMs";
    case RtStage::DynamicUpload: return "dynamicUploadCpuMs";
    case RtStage::BlasRefitRecord: return "blasRefitRecordCpuMs";
    case RtStage::TlasUpdateRecord: return "tlasUpdateRecordCpuMs";
    case RtStage::TraceCopyRecord: return "traceCopyRecordCpuMs";
    case RtStage::FrameFenceWait: return "frameFenceWaitCpuMs";
    case RtStage::ImageAcquire: return "imageAcquireCpuMs";
    case RtStage::QueueSubmit: return "queueSubmitCpuMs";
    case RtStage::PresentCall: return "presentCallCpuMs";
    case RtStage::WholeFrameCycle: return "wholeFrameCycleCpuMs";
    default: return nullptr;
    }
}

bool CheckedMillisecondsToNanoseconds(const double milliseconds,
                                      std::uint64_t& nanoseconds) noexcept
{
    if (!std::isfinite(milliseconds) || milliseconds < 0.0)
    {
        return false;
    }
    const long double converted = static_cast<long double>(milliseconds) * 1'000'000.0L;
    if (converted > static_cast<long double>(std::numeric_limits<std::uint64_t>::max()))
    {
        return false;
    }
    const long double rounded = std::round(converted);
    if (rounded > static_cast<long double>(std::numeric_limits<std::uint64_t>::max()))
    {
        return false;
    }
    nanoseconds = static_cast<std::uint64_t>(rounded);
    return true;
}

bool AccumulateCommittedStage(RtStageAggregate& aggregate,
                              const RtStageValue& sample) noexcept
{
    aggregate.latestNanoseconds = sample.durationNanoseconds;
    aggregate.maxNanoseconds = std::max(aggregate.maxNanoseconds, sample.durationNanoseconds);
    bool valid = !sample.overflowed;
    valid = SaturatingAdd(aggregate.sumNanoseconds, sample.durationNanoseconds) && valid;
    valid = SaturatingAdd(aggregate.sampleCount, 1u) && valid;
    valid = SaturatingAdd(aggregate.workInvocationCount, sample.workInvocationCount) && valid;
    valid = SaturatingAdd(aggregate.byteCount, sample.byteCount) && valid;
    valid = SaturatingAdd(aggregate.operationCount, sample.operationCount) && valid;
    aggregate.overflowed = aggregate.overflowed || !valid;
    aggregate.statisticsValid = aggregate.statisticsValid && valid;
    return valid;
}

bool RtStageAccumulator::Begin() noexcept
{
    if (active_)
    {
        return false;
    }
    scratch_ = {};
    active_ = true;
    return true;
}

bool RtStageAccumulator::Accumulate(const RtStage stage,
                                    const std::uint64_t durationNanoseconds,
                                    const std::uint64_t workInvocationCount,
                                    const std::uint64_t byteCount,
                                    const std::uint64_t operationCount) noexcept
{
    const std::size_t index = RtStageIndex(stage);
    if (!active_ || index >= scratch_.size() || stage == RtStage::Skin)
    {
        return false;
    }
    const auto accumulateValue = [&](RtStageValue& value) {
        bool valueValid = true;
        valueValid = SaturatingAdd(value.durationNanoseconds, durationNanoseconds) && valueValid;
        valueValid = SaturatingAdd(value.workInvocationCount, workInvocationCount) && valueValid;
        valueValid = SaturatingAdd(value.byteCount, byteCount) && valueValid;
        valueValid = SaturatingAdd(value.operationCount, operationCount) && valueValid;
        value.overflowed = value.overflowed || !valueValid;
        return valueValid;
    };
    bool valid = accumulateValue(scratch_[index]);
    if (stage == RtStage::PlayerSkin || stage == RtStage::CharacterSkin)
    {
        valid = accumulateValue(scratch_[RtStageIndex(RtStage::Skin)]) && valid;
    }
    return valid;
}

bool RtStageAccumulator::Commit(RtStageFrameSample& output) noexcept
{
    if (!active_)
    {
        return false;
    }
    RtStageFrameSample candidate{};
    candidate.values = scratch_;
    bool valid = true;
    for (std::size_t index = 0u; index < scratch_.size(); ++index)
    {
        valid = AccumulateCommittedStage(aggregates_.values[index], scratch_[index]) && valid;
    }
    candidate.status = valid ? RtSampleStatus::Valid : RtSampleStatus::Error;
    output = candidate;
    scratch_ = {};
    active_ = false;
    return valid;
}

bool RtStageAccumulator::Abort() noexcept
{
    if (!active_)
    {
        return false;
    }
    scratch_ = {};
    active_ = false;
    return true;
}

bool RtStageAccumulator::ResetAggregates() noexcept
{
    if (active_)
    {
        return false;
    }
    aggregates_ = {};
    return true;
}

bool RtStageSampleCollector::Start(const std::uint64_t sceneEpoch,
                                   const std::uint64_t measurementGeneration) noexcept
{
    if (sceneEpoch == 0u || measurementGeneration == 0u)
    {
        return false;
    }
    RtStageSampleCollector candidate{};
    candidate.sceneEpoch_ = sceneEpoch;
    candidate.measurementGeneration_ = measurementGeneration;
    candidate.configured_ = true;
    *this = candidate;
    return true;
}

bool RtStageSampleCollector::Append(const RtCompletedStageSample& sample) noexcept
{
    const RtFrameToken& frame = sample.identity.submitted.frame;
    RtEvidenceValidationError stageError = RtEvidenceValidationError::None;
    const bool identityValid = configured_ && sample.benchmarkEligible &&
        frame.sceneEpoch == sceneEpoch_ &&
        frame.measurementGeneration == measurementGeneration_ &&
        frame.recordAttemptSerial > lastRecordAttemptSerial_ &&
        frame.recordSerial > lastRecordSerial_ &&
        sample.identity.submitted.submissionSerial > lastSubmissionSerial_ &&
        sample.identity.completionSerial > lastCompletionSerial_ &&
        frame.frameSlot < kRtMaximumFrameSlots &&
        ValidStageFrame(sample.stages, stageError) &&
        sample.stages.status == RtSampleStatus::Valid;
    if (!identityValid)
    {
        IncrementSaturatingCounter(identityRejectCount_);
        invalidRun_ = true;
        return false;
    }
    if (size_ == samples_.size())
    {
        IncrementSaturatingCounter(overflowCount_);
        invalidRun_ = true;
        return false;
    }
    samples_[size_] = sample;
    ++size_;
    lastRecordAttemptSerial_ = frame.recordAttemptSerial;
    lastRecordSerial_ = frame.recordSerial;
    lastSubmissionSerial_ = sample.identity.submitted.submissionSerial;
    lastCompletionSerial_ = sample.identity.completionSerial;
    return true;
}

bool RtStageSampleCollector::Statistics(const RtStage stage,
                                        RtStageStatistics& output) const noexcept
{
    const std::size_t stageIndex = RtStageIndex(stage);
    if (!CompleteReportEligible() || stageIndex >= kRtStageCount)
    {
        output = {};
        return false;
    }
    std::array<std::uint64_t, kRtEvidenceSampleCapacity> sorted{};
    long double sumNanoseconds = 0.0L;
    for (std::size_t index = 0u; index < size_; ++index)
    {
        const std::uint64_t duration = samples_[index].stages.values[stageIndex].durationNanoseconds;
        sorted[index] = duration;
        sumNanoseconds += static_cast<long double>(duration);
    }
    std::sort(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(size_));

    RtStageStatistics candidate{};
    candidate.valid = true;
    candidate.sampleCount = static_cast<std::uint64_t>(size_);
    candidate.meanMilliseconds = static_cast<double>(sumNanoseconds /
        static_cast<long double>(size_) / 1'000'000.0L);
    if ((size_ % 2u) == 0u)
    {
        const long double middle =
            (static_cast<long double>(sorted[size_ / 2u - 1u]) +
             static_cast<long double>(sorted[size_ / 2u])) /
            2.0L;
        candidate.medianMilliseconds = static_cast<double>(middle / 1'000'000.0L);
    }
    else
    {
        candidate.medianMilliseconds =
            static_cast<double>(sorted[size_ / 2u]) / 1'000'000.0;
    }
    const std::size_t p90Index = (90u * size_ + 99u) / 100u - 1u;
    const std::size_t p95Index = (95u * size_ + 99u) / 100u - 1u;
    candidate.p90Milliseconds = static_cast<double>(sorted[p90Index]) / 1'000'000.0;
    candidate.p95Milliseconds = static_cast<double>(sorted[p95Index]) / 1'000'000.0;

    const std::size_t slowCount = (size_ + 99u) / 100u;
    long double slowSum = 0.0L;
    for (std::size_t index = size_ - slowCount; index < size_; ++index)
    {
        slowSum += static_cast<long double>(sorted[index]);
    }
    const long double slowAverage = slowSum / static_cast<long double>(slowCount);
    candidate.onePercentLowFps = slowAverage > 0.0L
        ? static_cast<double>(1'000'000'000.0L / slowAverage)
        : 0.0;
    output = candidate;
    return true;
}

bool ValidateRtPerformanceEvidence(const RtPerformanceEvidenceSnapshot& snapshot,
                                   RtEvidenceValidationError& error) noexcept
{
    error = RtEvidenceValidationError::None;
    if (snapshot.schema != kRtPerformanceEvidenceSchema)
    {
        error = RtEvidenceValidationError::UnknownSchema;
        return false;
    }
    const RtFrameToken& frame = snapshot.identity.submitted.frame;
    if (frame.sceneEpoch == 0u || frame.measurementGeneration == 0u ||
        frame.recordAttemptSerial == 0u || frame.recordSerial == 0u ||
        snapshot.identity.submitted.submissionSerial == 0u ||
        snapshot.identity.completionSerial == 0u ||
        frame.frameSlot >= kRtMaximumFrameSlots)
    {
        error = RtEvidenceValidationError::InconsistentIdentity;
        return false;
    }
    if (!ValidSceneFrame(snapshot.scene, error) ||
        !ValidDiagnostic(snapshot.dielectric,
                         snapshot.scene.pipeline.instrumentation,
                         snapshot.identity.submitted.submissionSerial,
                         error) ||
        !ValidGpu(snapshot.gpu, snapshot.identity.submitted.submissionSerial, error))
    {
        return false;
    }
    if (!KnownEnumName(RtPresentationOutcomeName, snapshot.presentation.outcome))
    {
        error = RtEvidenceValidationError::UnknownEnum;
        return false;
    }
    if (snapshot.presentation.outcome == RtPresentationOutcome::NotAttempted &&
        !snapshot.presentation.finalIdleCompletion)
    {
        error = RtEvidenceValidationError::InvalidPresentationState;
        return false;
    }
    const bool presented = snapshot.presentation.outcome == RtPresentationOutcome::Presented;
    if (presented && snapshot.presentation.lastSuccessfulPresentSubmissionSerial <
                         snapshot.identity.submitted.submissionSerial)
    {
        error = RtEvidenceValidationError::InconsistentIdentity;
        return false;
    }
    const bool diagnosticBenchmarkValid =
        snapshot.dielectric.status == RtSampleStatus::CompiledOut ||
        snapshot.dielectric.status == RtSampleStatus::Valid;
    const bool gpuBenchmarkValid = snapshot.gpu.status == RtSampleStatus::Valid ||
                                   snapshot.gpu.status == RtSampleStatus::Disabled ||
                                   snapshot.gpu.status == RtSampleStatus::Unsupported;
    if (snapshot.benchmarkEligible && !diagnosticBenchmarkValid)
    {
        error = RtEvidenceValidationError::InvalidDiagnosticState;
        return false;
    }
    if (snapshot.benchmarkEligible && !gpuBenchmarkValid)
    {
        error = RtEvidenceValidationError::InvalidGpuState;
        return false;
    }
    if (snapshot.benchmarkEligible &&
        (!presented || snapshot.scene.stages.status != RtSampleStatus::Valid))
    {
        error = RtEvidenceValidationError::InvalidPresentationState;
        return false;
    }
    return true;
}

bool SerializeRtPerformanceEvidenceJson(const RtPerformanceEvidenceSnapshot& snapshot,
                                        std::string& output,
                                        RtEvidenceValidationError& error)
{
    output.clear();
    if (!ValidateRtPerformanceEvidence(snapshot, error))
    {
        return false;
    }

    const RtFrameToken& frame = snapshot.identity.submitted.frame;
    const RtPipelineEvidenceIdentity& pipeline = snapshot.scene.pipeline;
    std::ostringstream json;
    json << "{\"schema\":" << snapshot.schema
         << ",\"identity\":{\"sceneEpoch\":" << frame.sceneEpoch
         << ",\"measurementGeneration\":" << frame.measurementGeneration
         << ",\"recordAttemptSerial\":" << frame.recordAttemptSerial
         << ",\"recordSerial\":" << frame.recordSerial
         << ",\"submissionSerial\":" << snapshot.identity.submitted.submissionSerial
         << ",\"completionSerial\":" << snapshot.identity.completionSerial
         << ",\"simulationTick\":" << frame.simulationTick
         << ",\"frameSlot\":" << frame.frameSlot << "}"
         << ",\"pipeline\":{\"instrumentation\":\""
         << RtInstrumentationModeName(pipeline.instrumentation)
         << "\",\"dielectricQuality\":\"" << RtDielectricQualityName(pipeline.dielectricQuality)
         << "\",\"bundleKey\":\"";
    WriteJsonEscaped(json, RtFixedTextView(pipeline.bundleKey));
    json << "\",\"opaqueFast\":{\"key\":\"";
    WriteJsonEscaped(json, RtFixedTextView(pipeline.opaqueFast.key));
    json << "\",\"sha256\":\"" << RtFixedTextView(pipeline.opaqueFast.sha256)
         << "\"},\"genericDielectric\":{\"key\":\"";
    WriteJsonEscaped(json, RtFixedTextView(pipeline.genericDielectric.key));
    json << "\",\"sha256\":\"" << RtFixedTextView(pipeline.genericDielectric.sha256)
         << "\"},\"activeStrategy\":\"" << RtMaterialStrategyName(pipeline.activeStrategy)
         << "\",\"activeKey\":\"";
    WriteJsonEscaped(json, RtFixedTextView(pipeline.active.key));
    json << "\",\"activeSha256\":\"" << RtFixedTextView(pipeline.active.sha256)
         << "\",\"waterQuality\":\"" << RtWaterQualityName(pipeline.waterQuality) << "\"}"
         << ",\"dispatch\":{\"sceneReady\":"
         << (snapshot.scene.dispatch.sceneReady ? "true" : "false")
         << ",\"rtDispatchRecorded\":"
         << (snapshot.scene.dispatch.rtDispatchRecorded ? "true" : "false")
         << ",\"swapchainCopyRecorded\":"
         << (snapshot.scene.dispatch.swapchainCopyRecorded ? "true" : "false") << '}';

    const RtResourceInventory& resources = snapshot.scene.resources;
    json << ",\"resources\":{\"bufferCount\":" << resources.bufferCount
         << ",\"memoryAllocationCount\":" << resources.memoryAllocationCount
         << ",\"bottomLevelAccelerationStructureCount\":"
         << resources.bottomLevelAccelerationStructureCount
         << ",\"topLevelAccelerationStructureCount\":"
         << resources.topLevelAccelerationStructureCount
         << ",\"tlasInstanceCount\":" << resources.tlasInstanceCount
         << ",\"pipelineCount\":" << resources.pipelineCount
         << ",\"shaderBindingTableCount\":" << resources.shaderBindingTableCount
         << ",\"descriptorSetCount\":" << resources.descriptorSetCount
         << ",\"hostVisibleBytes\":" << resources.hostVisibleBytes
         << ",\"deviceLocalBytes\":" << resources.deviceLocalBytes << '}';

    const RtPlayerDiagnostics& player = snapshot.scene.player;
    json << ",\"player\":{\"skinCadenceHz\":" << player.skinCadenceHz
         << ",\"skinUpdateCount\":" << player.skinUpdateCount
         << ",\"maximumSocketErrorMicrometres\":"
         << player.maximumSocketErrorMicrometres
         << ",\"primaryPixelCount\":" << player.primaryPixelCount
         << ",\"primaryVisible\":" << (player.primaryVisible ? "true" : "false") << '}';

    const RtDiagnosticEvidence& diagnostic = snapshot.dielectric;
    json << ",\"dielectric\":{\"status\":\"" << RtSampleStatusName(diagnostic.status)
         << "\",\"available\":" << (RtSampleAvailable(diagnostic.status) ? "true" : "false")
         << ",\"compiled\":" << (diagnostic.compiled ? "true" : "false")
         << ",\"completedSubmissionSerial\":" << diagnostic.completedSubmissionSerial
         << ",\"readCount\":" << diagnostic.readCount
         << ",\"resetCount\":" << diagnostic.resetCount
         << ",\"counters\":";
    if (diagnostic.status == RtSampleStatus::Valid)
    {
        json << '[';
        for (std::size_t index = 0u; index < diagnostic.counters.size(); ++index)
        {
            if (index > 0u)
            {
                json << ',';
            }
            json << diagnostic.counters[index];
        }
        json << ']';
    }
    else
    {
        json << "null";
    }
    json << ",\"detail\":\"";
    WriteJsonEscaped(json, RtFixedTextView(diagnostic.detail));
    json << "\"}";

    const RtGpuTimingEvidence& gpu = snapshot.gpu;
    json << ",\"gpu\":{\"status\":\"" << RtSampleStatusName(gpu.status)
         << "\",\"available\":" << (RtSampleAvailable(gpu.status) ? "true" : "false")
         << ",\"completedSubmissionSerial\":" << gpu.completedSubmissionSerial
         << ",\"wholeRtGpuMs\":";
    if (gpu.status == RtSampleStatus::Valid)
    {
        WriteMilliseconds(json, gpu.durationNanoseconds);
    }
    else
    {
        json << "null";
    }
    json << ",\"timestampValidBits\":" << gpu.timestampValidBits
         << ",\"timestampPeriodPicoseconds\":" << gpu.timestampPeriodPicoseconds
         << ",\"sampleCount\":" << gpu.sampleCount
         << ",\"unavailableResultCount\":" << gpu.unavailableResultCount
         << ",\"errorCount\":" << gpu.errorCount << ",\"detail\":\"";
    WriteJsonEscaped(json, RtFixedTextView(gpu.detail));
    json << "\"}";

    json << ",\"stages\":{\"status\":\"" << RtSampleStatusName(snapshot.scene.stages.status)
         << '\"';
    for (std::size_t index = 0u; index < kRtStageCount; ++index)
    {
        json << ",\"" << RtStageMetricName(static_cast<RtStage>(index)) << "\":";
        if (snapshot.scene.stages.status == RtSampleStatus::Valid)
        {
            WriteMilliseconds(json, snapshot.scene.stages.values[index].durationNanoseconds);
        }
        else
        {
            json << "null";
        }
    }
    json << ",\"workInvocationCounts\":";
    WriteJsonStageArray(json, snapshot.scene.stages, 0u);
    json << ",\"byteCounts\":";
    WriteJsonStageArray(json, snapshot.scene.stages, 1u);
    json << ",\"operationCounts\":";
    WriteJsonStageArray(json, snapshot.scene.stages, 2u);
    json << '}';

    const bool presented = snapshot.presentation.outcome == RtPresentationOutcome::Presented;
    json << ",\"presentation\":{\"outcome\":\""
         << RtPresentationOutcomeName(snapshot.presentation.outcome)
         << "\",\"presented\":" << (presented ? "true" : "false")
         << ",\"lastSuccessfulPresentSubmissionSerial\":"
         << snapshot.presentation.lastSuccessfulPresentSubmissionSerial
         << ",\"finalIdleCompletion\":"
         << (snapshot.presentation.finalIdleCompletion ? "true" : "false") << '}'
         << ",\"benchmarkEligible\":" << (snapshot.benchmarkEligible ? "true" : "false")
         << "}\n";
    output = json.str();
    return true;
}

bool SerializeRtPerformanceEvidenceText(const RtPerformanceEvidenceSnapshot& snapshot,
                                        std::string& output,
                                        RtEvidenceValidationError& error)
{
    output.clear();
    if (!ValidateRtPerformanceEvidence(snapshot, error))
    {
        return false;
    }
    const RtFrameToken& frame = snapshot.identity.submitted.frame;
    const RtPipelineEvidenceIdentity& pipeline = snapshot.scene.pipeline;
    std::ostringstream text;
    text << "RT PERFORMANCE EVIDENCE schema=" << snapshot.schema << '\n'
         << "Frame: epoch=" << frame.sceneEpoch
         << " generation=" << frame.measurementGeneration
         << " attempt=" << frame.recordAttemptSerial
         << " record=" << frame.recordSerial
         << " submission=" << snapshot.identity.submitted.submissionSerial
         << " completion=" << snapshot.identity.completionSerial
         << " tick=" << frame.simulationTick
         << " slot=" << frame.frameSlot << '\n'
         << "Pipeline: " << RtInstrumentationModeName(pipeline.instrumentation) << '/'
         << RtDielectricQualityName(pipeline.dielectricQuality)
         << " pair=" << RtFixedTextView(pipeline.bundleKey)
         << " active=" << RtMaterialStrategyName(pipeline.activeStrategy) << ' '
         << RtFixedTextView(pipeline.active.key) << '@' << RtFixedTextView(pipeline.active.sha256)
         << '\n'
         << "Player: skin-cadence-hz=" << snapshot.scene.player.skinCadenceHz
         << " skin-updates=" << snapshot.scene.player.skinUpdateCount
         << " max-socket-error-um=" << snapshot.scene.player.maximumSocketErrorMicrometres
         << " primary-pixels=" << snapshot.scene.player.primaryPixelCount
         << " primary-visible=" << (snapshot.scene.player.primaryVisible ? "yes" : "no") << '\n'
         << "Presentation: " << RtPresentationOutcomeName(snapshot.presentation.outcome)
         << " last-successful-submission="
         << snapshot.presentation.lastSuccessfulPresentSubmissionSerial
         << " final-idle=" << (snapshot.presentation.finalIdleCompletion ? "yes" : "no")
         << " benchmark-eligible=" << (snapshot.benchmarkEligible ? "yes" : "no") << '\n'
         << "Dielectric diagnostics: " << RtSampleStatusName(snapshot.dielectric.status)
         << " counters=" << (snapshot.dielectric.status == RtSampleStatus::Valid ? "available" : "N/A")
         << " reads=" << snapshot.dielectric.readCount
         << " resets=" << snapshot.dielectric.resetCount << '\n'
         << "Whole RT GPU: " << RtSampleStatusName(snapshot.gpu.status) << " value=";
    if (snapshot.gpu.status == RtSampleStatus::Valid)
    {
        WriteMilliseconds(text, snapshot.gpu.durationNanoseconds);
        text << " ms";
    }
    else
    {
        text << "N/A";
    }
    text << "\nStages (CPU wall/record):";
    for (std::size_t index = 0u; index < kRtStageCount; ++index)
    {
        text << ' ' << RtStageMetricName(static_cast<RtStage>(index)) << '=';
        if (snapshot.scene.stages.status == RtSampleStatus::Valid)
        {
            WriteMilliseconds(text, snapshot.scene.stages.values[index].durationNanoseconds);
        }
        else
        {
            text << "N/A";
        }
    }
    text << '\n';
    output = text.str();
    return true;
}

void RtEvidenceLifecycle::ClearSlots() noexcept
{
    slots_ = {};
}

bool RtEvidenceLifecycle::ResetResources(const RtResourceResetReason reason,
                                         const bool running,
                                         const RtSampleStatus diagnosticStatus,
                                         const RtSampleStatus gpuStatus,
                                         RtLifecycleResetEffects& effects) noexcept
{
    if (seeds_.sceneEpoch == std::numeric_limits<std::uint64_t>::max() ||
        static_cast<std::uint8_t>(reason) >
            static_cast<std::uint8_t>(RtResourceResetReason::DestroyOrSurfaceStop) ||
        (running && reason == RtResourceResetReason::DestroyOrSurfaceStop) ||
        (!running && reason != RtResourceResetReason::DestroyOrSurfaceStop) ||
        !ValidInitialDiagnosticStatus(diagnosticStatus) ||
        !ValidInitialGpuStatus(gpuStatus))
    {
        return false;
    }
    ++seeds_.sceneEpoch;
    ClearSlots();
    lastSuccessfulPresentSubmissionSerial_ = 0u;
    published_ = {};
    published_.sceneEpoch = seeds_.sceneEpoch;
    published_.measurementGeneration = seeds_.measurementGeneration;
    published_.diagnosticStatus = running ? diagnosticStatus : RtSampleStatus::NotReady;
    published_.gpuStatus = running ? gpuStatus : RtSampleStatus::NotReady;
    published_.lastResourceReset = reason;
    published_.running = running;
    measurementSamplesEligible_ = running;
    effects = {};
    effects.epochChanged = true;
    effects.pendingSubmissionsInvalidated = true;
    effects.collectorInvalidated = true;
    effects.nextSampleEligible = running;
    return true;
}

bool RtEvidenceLifecycle::Initialise(const RtLifecycleSeeds& preservedSeeds,
                                     const std::uint32_t activeSlotCount,
                                     const RtSampleStatus initialDiagnosticStatus,
                                     const RtSampleStatus initialGpuStatus,
                                     RtLifecycleResetEffects& effects) noexcept
{
    const bool staleSeeds = initialised_ &&
        (preservedSeeds.sceneEpoch < seeds_.sceneEpoch ||
         preservedSeeds.measurementGeneration < seeds_.measurementGeneration ||
         preservedSeeds.recordAttemptSerial < seeds_.recordAttemptSerial ||
         preservedSeeds.successfulRecordSerial < seeds_.successfulRecordSerial ||
         preservedSeeds.submissionSerial < seeds_.submissionSerial ||
         preservedSeeds.completionSerial < seeds_.completionSerial);
    if ((initialised_ && published_.running) || staleSeeds ||
        preservedSeeds.sceneEpoch == 0u || preservedSeeds.measurementGeneration == 0u ||
        activeSlotCount == 0u || activeSlotCount > kRtMaximumFrameSlots ||
        !ValidInitialDiagnosticStatus(initialDiagnosticStatus) ||
        !ValidInitialGpuStatus(initialGpuStatus))
    {
        return false;
    }
    RtEvidenceLifecycle candidate{};
    candidate.seeds_ = preservedSeeds;
    candidate.activeSlotCount_ = activeSlotCount;
    candidate.initialised_ = true;
    RtLifecycleResetEffects candidateEffects{};
    if (!candidate.ResetResources(RtResourceResetReason::Initialise,
                                  true,
                                  initialDiagnosticStatus,
                                  initialGpuStatus,
                                  candidateEffects))
    {
        return false;
    }
    *this = candidate;
    effects = candidateEffects;
    return true;
}

bool RtEvidenceLifecycle::Recreate(const RtResourceResetReason reason,
                                   const RtSampleStatus initialDiagnosticStatus,
                                   const RtSampleStatus initialGpuStatus,
                                   RtLifecycleResetEffects& effects) noexcept
{
    if (!initialised_ || !published_.running)
    {
        return false;
    }
    if (reason == RtResourceResetReason::Initialise ||
        reason == RtResourceResetReason::DestroyOrSurfaceStop)
    {
        return false;
    }
    return ResetResources(reason, true, initialDiagnosticStatus, initialGpuStatus, effects);
}

bool RtEvidenceLifecycle::Destroy(RtLifecycleResetEffects& effects) noexcept
{
    if (!initialised_ || !published_.running)
    {
        return false;
    }
    return ResetResources(RtResourceResetReason::DestroyOrSurfaceStop,
                          false,
                          RtSampleStatus::NotReady,
                          RtSampleStatus::NotReady,
                          effects);
}

bool RtEvidenceLifecycle::ApplyEvent(const RtLifecycleEvent event,
                                     RtLifecycleResetEffects& effects) noexcept
{
    if (!initialised_ || !published_.running)
    {
        return false;
    }
    RtLifecycleResetEffects candidateEffects{};
    if (event == RtLifecycleEvent::Pause)
    {
        if (published_.paused)
        {
            return false;
        }
        published_.paused = true;
        measurementSamplesEligible_ = false;
        effects = candidateEffects;
        return true;
    }
    if (event == RtLifecycleEvent::Resume)
    {
        if (!published_.paused || seeds_.measurementGeneration ==
                                      std::numeric_limits<std::uint64_t>::max())
        {
            return false;
        }
        ++seeds_.measurementGeneration;
        published_.measurementGeneration = seeds_.measurementGeneration;
        published_.paused = false;
        measurementSamplesEligible_ = true;
        candidateEffects.measurementGenerationChanged = true;
        candidateEffects.collectorCleared = true;
        candidateEffects.nextSampleEligible = true;
        candidateEffects.restartFrameCycleClock = true;
        effects = candidateEffects;
        return true;
    }

    switch (event)
    {
    case RtLifecycleEvent::BenchmarkStart:
    case RtLifecycleEvent::WarmupToMeasure:
    case RtLifecycleEvent::RouteReset:
    case RtLifecycleEvent::Retry:
    case RtLifecycleEvent::CheckpointChange:
    case RtLifecycleEvent::GpuTimingEnabled:
    case RtLifecycleEvent::GpuTimingDisabled: break;
    default: return false;
    }
    if (seeds_.measurementGeneration == std::numeric_limits<std::uint64_t>::max())
    {
        return false;
    }
    ++seeds_.measurementGeneration;
    published_.measurementGeneration = seeds_.measurementGeneration;
    measurementSamplesEligible_ = event != RtLifecycleEvent::BenchmarkStart;
    candidateEffects.measurementGenerationChanged = true;
    candidateEffects.collectorCleared = true;
    candidateEffects.nextSampleEligible = event != RtLifecycleEvent::BenchmarkStart;
    if (event == RtLifecycleEvent::RouteReset || event == RtLifecycleEvent::Retry ||
        event == RtLifecycleEvent::CheckpointChange)
    {
        candidateEffects.collectorInvalidated = true;
    }
    if (event == RtLifecycleEvent::GpuTimingEnabled ||
        event == RtLifecycleEvent::GpuTimingDisabled)
    {
        candidateEffects.clearGpuAggregates = true;
        published_.gpuStatus = event == RtLifecycleEvent::GpuTimingEnabled
            ? RtSampleStatus::Pending
            : RtSampleStatus::Disabled;
    }
    effects = candidateEffects;
    return true;
}

bool RtEvidenceLifecycle::ValidTokenForSlot(const RtFrameToken& token,
                                            const SlotPhase phase) const noexcept
{
    if (token.frameSlot >= activeSlotCount_ || token.frameSlot >= slots_.size())
    {
        return false;
    }
    const SlotState& slot = slots_[token.frameSlot];
    return slot.phase == phase && SameFrameToken(slot.token, token);
}

bool RtEvidenceLifecycle::BeginRecord(const std::uint32_t frameSlot,
                                      const std::uint64_t simulationTick,
                                      RtFrameToken& attempt) noexcept
{
    if (!initialised_ || !published_.running || published_.paused ||
        published_.diagnosticStatus == RtSampleStatus::Error ||
        frameSlot >= activeSlotCount_ || slots_[frameSlot].phase != SlotPhase::Empty ||
        seeds_.recordAttemptSerial == std::numeric_limits<std::uint64_t>::max())
    {
        return false;
    }
    RtFrameToken candidate{};
    candidate.sceneEpoch = seeds_.sceneEpoch;
    candidate.measurementGeneration = seeds_.measurementGeneration;
    candidate.recordAttemptSerial = seeds_.recordAttemptSerial + 1u;
    candidate.simulationTick = simulationTick;
    candidate.frameSlot = frameSlot;
    ++seeds_.recordAttemptSerial;
    slots_[frameSlot].phase = SlotPhase::Recording;
    slots_[frameSlot].token = candidate;
    attempt = candidate;
    return true;
}

bool RtEvidenceLifecycle::FinishRecord(const RtFrameToken& attempt,
                                       const RtRecordedSceneEvidence& evidence,
                                       RtFrameToken& recorded) noexcept
{
    RtEvidenceValidationError error = RtEvidenceValidationError::None;
    if (!ValidTokenForSlot(attempt, SlotPhase::Recording) || attempt.recordSerial != 0u ||
        seeds_.successfulRecordSerial == std::numeric_limits<std::uint64_t>::max() ||
        !ValidRecordedScene(evidence, error))
    {
        return false;
    }
    RtFrameToken candidate = attempt;
    candidate.recordSerial = seeds_.successfulRecordSerial + 1u;
    ++seeds_.successfulRecordSerial;
    SlotState& slot = slots_[attempt.frameSlot];
    slot.phase = SlotPhase::Recorded;
    slot.token = candidate;
    slot.scene = evidence;
    recorded = candidate;
    return true;
}

bool RtEvidenceLifecycle::AbortRecord(const RtFrameToken& attempt) noexcept
{
    if (!ValidTokenForSlot(attempt, SlotPhase::Recording) || attempt.recordSerial != 0u)
    {
        return false;
    }
    slots_[attempt.frameSlot] = {};
    return true;
}

bool RtEvidenceLifecycle::FailDiagnosticReset(const RtFrameToken& attempt,
                                              RtLifecycleResetEffects& effects) noexcept
{
    if (!ValidTokenForSlot(attempt, SlotPhase::Recording) || attempt.recordSerial != 0u ||
        (published_.diagnosticStatus != RtSampleStatus::Pending &&
         published_.diagnosticStatus != RtSampleStatus::Valid))
    {
        return false;
    }
    slots_[attempt.frameSlot] = {};
    published_.diagnosticStatus = RtSampleStatus::Error;
    measurementSamplesEligible_ = false;
    effects = {};
    effects.collectorInvalidated = true;
    return true;
}

bool RtEvidenceLifecycle::FailSubmit(const RtFrameToken& recorded) noexcept
{
    if (!ValidTokenForSlot(recorded, SlotPhase::Recorded) || recorded.recordSerial == 0u)
    {
        return false;
    }
    slots_[recorded.frameSlot] = {};
    return true;
}

bool RtEvidenceLifecycle::Submit(const RtFrameToken& recorded,
                                 RtSubmittedFrameIdentity& submitted) noexcept
{
    if (!ValidTokenForSlot(recorded, SlotPhase::Recorded) || recorded.recordSerial == 0u ||
        seeds_.submissionSerial == std::numeric_limits<std::uint64_t>::max())
    {
        return false;
    }
    RtSubmittedFrameIdentity candidate{};
    candidate.frame = recorded;
    candidate.submissionSerial = seeds_.submissionSerial + 1u;
    ++seeds_.submissionSerial;
    SlotState& slot = slots_[recorded.frameSlot];
    slot.phase = SlotPhase::Submitted;
    slot.submissionSerial = candidate.submissionSerial;
    submitted = candidate;
    return true;
}

bool RtEvidenceLifecycle::AttachPresentation(const RtSubmittedFrameIdentity& submitted,
                                             const RtPresentationOutcome outcome) noexcept
{
    if (!KnownEnumName(RtPresentationOutcomeName, outcome) ||
        outcome == RtPresentationOutcome::NotAttempted ||
        !ValidTokenForSlot(submitted.frame, SlotPhase::Submitted))
    {
        return false;
    }
    SlotState& slot = slots_[submitted.frame.frameSlot];
    if (slot.submissionSerial != submitted.submissionSerial || slot.presentationAttached)
    {
        return false;
    }
    slot.presentationAttached = true;
    slot.presentation = outcome;
    if (outcome == RtPresentationOutcome::Presented)
    {
        lastSuccessfulPresentSubmissionSerial_ =
            std::max(lastSuccessfulPresentSubmissionSerial_, submitted.submissionSerial);
    }
    return true;
}

bool RtEvidenceLifecycle::Complete(const RtSubmittedFrameIdentity& submitted,
                                   const RtSubmittedStageSample& stages,
                                   const RtDiagnosticEvidence& diagnostic,
                                   const RtGpuTimingEvidence& gpu,
                                   const bool finalIdle,
                                   RtPerformanceEvidenceSnapshot& completed) noexcept
{
    RtEvidenceValidationError stageError = RtEvidenceValidationError::None;
    if (!ValidTokenForSlot(submitted.frame, SlotPhase::Submitted) ||
        !SameSubmitted(stages.identity, submitted) ||
        !ValidStageFrame(stages.stages, stageError) ||
        seeds_.completionSerial == std::numeric_limits<std::uint64_t>::max())
    {
        return false;
    }
    const SlotState& slot = slots_[submitted.frame.frameSlot];
    if (slot.submissionSerial != submitted.submissionSerial ||
        (!slot.presentationAttached && !finalIdle))
    {
        return false;
    }
    const RtPresentationOutcome outcome = slot.presentationAttached
        ? slot.presentation
        : RtPresentationOutcome::NotAttempted;
    if (!finalIdle && outcome == RtPresentationOutcome::NotAttempted)
    {
        return false;
    }

    RtPerformanceEvidenceSnapshot candidate{};
    candidate.identity.submitted = submitted;
    candidate.identity.completionSerial = seeds_.completionSerial + 1u;
    candidate.scene.pipeline = slot.scene.pipeline;
    candidate.scene.resources = slot.scene.resources;
    candidate.scene.player = slot.scene.player;
    candidate.scene.dispatch = slot.scene.dispatch;
    candidate.scene.stages = stages.stages;
    candidate.dielectric = diagnostic;
    candidate.gpu = gpu;
    candidate.presentation.outcome = outcome;
    candidate.presentation.lastSuccessfulPresentSubmissionSerial =
        lastSuccessfulPresentSubmissionSerial_;
    candidate.presentation.finalIdleCompletion = finalIdle;
    const bool diagnosticUsable = diagnostic.status == RtSampleStatus::CompiledOut ||
                                  diagnostic.status == RtSampleStatus::Valid;
    const bool gpuUsable = gpu.status == RtSampleStatus::Valid ||
                           gpu.status == RtSampleStatus::Disabled ||
                           gpu.status == RtSampleStatus::Unsupported;
    candidate.benchmarkEligible =
        measurementSamplesEligible_ && !published_.paused &&
        submitted.frame.measurementGeneration == seeds_.measurementGeneration &&
        outcome == RtPresentationOutcome::Presented &&
        candidate.scene.stages.status == RtSampleStatus::Valid &&
        diagnosticUsable && gpuUsable;

    RtEvidenceValidationError error = RtEvidenceValidationError::None;
    if (!ValidateRtPerformanceEvidence(candidate, error))
    {
        return false;
    }

    ++seeds_.completionSerial;
    if (outcome == RtPresentationOutcome::Presented)
    {
        lastSuccessfulPresentSubmissionSerial_ =
            std::max(lastSuccessfulPresentSubmissionSerial_, submitted.submissionSerial);
    }
    slots_[submitted.frame.frameSlot] = {};
    published_.diagnosticStatus = diagnostic.status;
    published_.gpuStatus = gpu.status;
    published_.presented = outcome == RtPresentationOutcome::Presented;
    published_.hasCompletedEvidence = true;
    published_.completedEvidence = candidate;
    completed = candidate;
    return true;
}

bool RtEvidenceLifecycle::CompleteFence(const RtSubmittedFrameIdentity& submitted,
                                        const RtSubmittedStageSample& stages,
                                        const RtDiagnosticEvidence& diagnostic,
                                        const RtGpuTimingEvidence& gpu,
                                        RtPerformanceEvidenceSnapshot& completed) noexcept
{
    return Complete(submitted, stages, diagnostic, gpu, false, completed);
}

bool RtEvidenceLifecycle::CompleteFinalIdle(const RtSubmittedFrameIdentity& submitted,
                                            const RtSubmittedStageSample& stages,
                                            const RtDiagnosticEvidence& diagnostic,
                                            const RtGpuTimingEvidence& gpu,
                                            RtPerformanceEvidenceSnapshot& completed) noexcept
{
    return Complete(submitted, stages, diagnostic, gpu, true, completed);
}

} // namespace horde::telemetry
