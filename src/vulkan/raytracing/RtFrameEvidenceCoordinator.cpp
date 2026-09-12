#include "vulkan/raytracing/RtFrameEvidenceCoordinator.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace horde::vulkan::raytracing
{
namespace
{

using namespace horde::telemetry;

template <std::size_t Capacity>
void SetKnownDetail(RtFixedText<Capacity>& destination, const char* detail) noexcept
{
    const bool assigned = AssignRtFixedText(destination, detail);
    (void)assigned;
}

bool CheckedTimestampPeriodPicoseconds(const float nanoseconds,
                                       std::uint64_t& picoseconds) noexcept
{
    if (!std::isfinite(nanoseconds) || nanoseconds <= 0.0f)
    {
        return false;
    }
    const long double converted = static_cast<long double>(nanoseconds) * 1000.0L;
    const long double upper = std::ldexp(
        1.0L, std::numeric_limits<std::uint64_t>::digits);
    if (converted >= upper)
    {
        return false;
    }
    const long double rounded = std::round(converted);
    if (rounded <= 0.0L || rounded >= upper)
    {
        return false;
    }
    picoseconds = static_cast<std::uint64_t>(rounded);
    return true;
}

RtGpuTimerStateSnapshot EmptyTimerSnapshot() noexcept
{
    return {};
}

} // namespace

RtGpuFrameTimerIo MakeRtGpuFrameTimerIo(GpuFrameTimer& timer) noexcept
{
    RtGpuFrameTimerIo io{};
    io.user = &timer;
    io.markSubmitted = [](void* user, const std::uint32_t frameSlot,
                          const std::uint64_t serial) noexcept {
        return static_cast<GpuFrameTimer*>(user)->MarkSubmitted(frameSlot, serial);
    };
    io.cancelRecording = [](void* user, const std::uint32_t frameSlot) noexcept {
        static_cast<GpuFrameTimer*>(user)->CancelRecording(frameSlot);
    };
    io.collectCompleted = [](void* user, const std::uint32_t frameSlot) noexcept {
        return static_cast<GpuFrameTimer*>(user)->CollectCompleted(frameSlot);
    };
    io.snapshot = [](void* user) noexcept {
        const GpuFrameTimerTelemetry& timerState =
            static_cast<GpuFrameTimer*>(user)->Telemetry();
        return RtGpuTimerStateSnapshot{
            timerState.status,
            timerState.timestampPeriodNanoseconds,
            timerState.timestampValidBits,
            timerState.sampleCount,
            timerState.unavailableResultCount,
            timerState.errorCount};
    };
    return io;
}

RtSampleStatus InitialRtGpuEvidenceStatus(
    const bool timingEnabled,
    const RtGpuTimerStateSnapshot& timer) noexcept
{
    if (!timingEnabled)
    {
        return RtSampleStatus::Disabled;
    }
    switch (timer.status)
    {
    case GpuFrameTimerStatus::UnsupportedQueue:
        return RtSampleStatus::Unsupported;
    case GpuFrameTimerStatus::InitialisationFailed:
    case GpuFrameTimerStatus::QueryError:
        return RtSampleStatus::Error;
    case GpuFrameTimerStatus::Uninitialised:
        return RtSampleStatus::NotReady;
    default:
        return RtSampleStatus::Pending;
    }
}

RtSampleStatus RtFrameEvidenceCoordinator::InitialDiagnosticStatus() const noexcept
{
    return instrumentation_ == RtInstrumentationMode::Shipping
        ? RtSampleStatus::CompiledOut
        : RtSampleStatus::Pending;
}

bool RtFrameEvidenceCoordinator::Initialise(
    const RtLifecycleSeeds& preservedSeeds,
    const std::uint32_t activeSlotCount,
    const RtInstrumentationMode instrumentation,
    const RtSampleStatus initialGpuStatus,
    const bool initiallyPaused) noexcept
{
    if (activeSlotCount == 0u || activeSlotCount > kRtMaximumFrameSlots ||
        static_cast<std::uint8_t>(instrumentation) >
            static_cast<std::uint8_t>(RtInstrumentationMode::Diagnostic))
    {
        return false;
    }
    activeSlotCount_ = activeSlotCount;
    preservedSeedFloor_ = preservedSeeds;
    pending_ = {};
    stageAccumulator_ = {};
    diagnosticReadCount_ = 0u;
    diagnosticResetCount_ = 0u;
    frameActive_ = false;
    deferredAggregateReset_ = false;
    initialised_ = true;
    RtLifecycleResetEffects effects{};
    instrumentation_ = instrumentation;
    desiredPaused_ = initiallyPaused;
    observerAvailable_ = lifecycle_.Initialise(
        preservedSeeds, activeSlotCount, InitialDiagnosticStatus(),
        initialGpuStatus, effects);
    if (observerAvailable_ && initiallyPaused)
    {
        RtLifecycleResetEffects pauseEffects{};
        if (!lifecycle_.ApplyEvent(RtLifecycleEvent::Pause, pauseEffects))
        {
            observerAvailable_ = false;
        }
    }
    // Lifecycle/serial failure disables joined evidence only. The fixed
    // graphics-ownership shell must remain available for tokenless fence IO.
    return true;
}

bool RtFrameEvidenceCoordinator::Recreate(
    const RtResourceResetReason reason,
    const RtSampleStatus initialGpuStatus) noexcept
{
    if (!initialised_)
    {
        return false;
    }
    AbortFrame();
    RtLifecycleResetEffects effects{};
    const bool recreated = lifecycle_.Recreate(
        reason, InitialDiagnosticStatus(), initialGpuStatus, effects);
    pending_ = {};
    stageAccumulator_ = {};
    diagnosticReadCount_ = 0u;
    diagnosticResetCount_ = 0u;
    deferredAggregateReset_ = false;
    observerAvailable_ = recreated;
    if (recreated && desiredPaused_)
    {
        RtLifecycleResetEffects pauseEffects{};
        observerAvailable_ = lifecycle_.ApplyEvent(
            RtLifecycleEvent::Pause, pauseEffects);
    }
    // As at initialisation, lifecycle/serial exhaustion removes only the
    // optional joined identity. The recreated graphics shell still owns real
    // submissions and their fence-ordered Diagnostic drain.
    return true;
}

bool RtFrameEvidenceCoordinator::Destroy() noexcept
{
    AbortFrame();
    RtLifecycleResetEffects effects{};
    const bool destroyed = initialised_ && lifecycle_.Destroy(effects);
    pending_ = {};
    stageAccumulator_ = {};
    ResetAttemptState();
    observerAvailable_ = false;
    initialised_ = false;
    return destroyed;
}

bool RtFrameEvidenceCoordinator::SetPaused(const bool paused) noexcept
{
    desiredPaused_ = paused;
    if (!initialised_ || !observerAvailable_ ||
        !lifecycle_.PublishedStateByValue().running)
    {
        return true;
    }
    const bool current = lifecycle_.PublishedStateByValue().paused;
    if (current == paused)
    {
        return true;
    }
    return ApplyEvent(paused ? RtLifecycleEvent::Pause : RtLifecycleEvent::Resume);
}

bool RtFrameEvidenceCoordinator::ApplyEvent(
    const RtLifecycleEvent event,
    RtLifecycleResetEffects* const effects) noexcept
{
    if (!initialised_ || !observerAvailable_)
    {
        return false;
    }
    RtLifecycleResetEffects applied{};
    if (!lifecycle_.ApplyEvent(event, applied))
    {
        observerAvailable_ = false;
        return false;
    }
    if (event == RtLifecycleEvent::Pause)
    {
        desiredPaused_ = true;
    }
    else if (event == RtLifecycleEvent::Resume)
    {
        desiredPaused_ = false;
    }
    if (applied.measurementGenerationChanged)
    {
        if (stageAccumulator_.Active())
        {
            deferredAggregateReset_ = true;
        }
        else if (!stageAccumulator_.ResetAggregates())
        {
            observerAvailable_ = false;
        }
    }
    if (effects != nullptr)
    {
        *effects = applied;
    }
    return true;
}

bool RtFrameEvidenceCoordinator::BeginFrame(
    const std::uint32_t frameSlot,
    RtSceneRecordObservation& observation) noexcept
{
    observation = {};
    if (!initialised_ || frameSlot >= activeSlotCount_)
    {
        return false;
    }
    if (frameActive_ || stageAccumulator_.Active())
    {
        AbortFrame();
        observerAvailable_ = false;
    }
    const bool stageReady = stageAccumulator_.Begin();
    observerAvailable_ = observerAvailable_ && stageReady;
    frameActive_ = true;
    activeFrameSlot_ = frameSlot;
    recordedSceneScratch_ = {};
    commandObservation_ = {};
    attempt_ = {};
    recorded_ = {};
    attemptValid_ = false;
    recordedValid_ = false;
    observation.stages = stageReady ? &stageAccumulator_ : nullptr;
    observation.commands = &commandObservation_;
    observation.recordedScene = &recordedSceneScratch_;
    return true;
}

bool RtFrameEvidenceCoordinator::BeginRecord(const std::uint64_t simulationTick) noexcept
{
    if (!frameActive_ || !observerAvailable_ ||
        !lifecycle_.BeginRecord(activeFrameSlot_, simulationTick, attempt_))
    {
        observerAvailable_ = false;
        attemptValid_ = false;
        return false;
    }
    attemptValid_ = true;
    return true;
}

bool RtFrameEvidenceCoordinator::FinishRecord(
    const RtSceneRecordObservation& observation) noexcept
{
    if (instrumentation_ == RtInstrumentationMode::Diagnostic &&
        observation.diagnosticResetCompleted)
    {
        if (diagnosticResetCount_ == std::numeric_limits<std::uint64_t>::max())
        {
            LoseCurrentIdentity();
            return false;
        }
        ++diagnosticResetCount_;
    }
    if (!attemptValid_ || observation.recordedScene != &recordedSceneScratch_ ||
        observation.commands != &commandObservation_ ||
        !commandObservation_.ValidCompleted() ||
        !lifecycle_.FinishRecord(attempt_, recordedSceneScratch_, recorded_))
    {
        if (attemptValid_)
        {
            (void)lifecycle_.AbortRecord(attempt_);
        }
        LoseCurrentIdentity();
        return false;
    }
    attemptValid_ = false;
    recordedValid_ = true;
    return true;
}

void RtFrameEvidenceCoordinator::FailRecord(
    const RtSceneRecordObservation& observation) noexcept
{
    if (attemptValid_)
    {
        if (observation.failure == RtSceneRecordFailure::DiagnosticReset)
        {
            RtLifecycleResetEffects effects{};
            (void)lifecycle_.FailDiagnosticReset(attempt_, effects);
            observerAvailable_ = false;
        }
        else
        {
            (void)lifecycle_.AbortRecord(attempt_);
        }
    }
    AbortFrame();
}

RtEvidenceSubmitTransaction RtFrameEvidenceCoordinator::PrevalidateSubmit() noexcept
{
    RtEvidenceSubmitTransaction transaction{};
    if (!recordedValid_ || !observerAvailable_)
    {
        return transaction;
    }
    transaction.candidate = lifecycle_;
    if (!transaction.candidate.Submit(recorded_, transaction.identity))
    {
        (void)lifecycle_.FailSubmit(recorded_);
        LoseCurrentIdentity();
        return {};
    }
    transaction.valid = true;
    return transaction;
}

void RtFrameEvidenceCoordinator::FailGraphicsSubmit(
    const bool timerRecording,
    const RtGpuFrameTimerIo& gpuIo) noexcept
{
    if (timerRecording && gpuIo.cancelRecording != nullptr)
    {
        gpuIo.cancelRecording(gpuIo.user, activeFrameSlot_);
    }
    if (recordedValid_)
    {
        (void)lifecycle_.FailSubmit(recorded_);
    }
    AbortFrame();
}

void RtFrameEvidenceCoordinator::CommitGraphicsSubmit(
    const RtEvidenceSubmitTransaction& transaction,
    const bool timingEnabled,
    const bool timerRecording,
    const RtGpuFrameTimerIo& gpuIo) noexcept
{
    if (!frameActive_ || activeFrameSlot_ >= pending_.size())
    {
        if (timerRecording && gpuIo.cancelRecording != nullptr)
        {
            gpuIo.cancelRecording(gpuIo.user, activeFrameSlot_);
        }
        observerAvailable_ = false;
        return;
    }

    PendingSlot& pending = pending_[activeFrameSlot_];
    if (pending.hasSuccessfulGraphicsSubmission)
    {
        observerAvailable_ = false;
    }
    pending = {};
    pending.hasSuccessfulGraphicsSubmission = true;

    if (!transaction.valid)
    {
        if (timerRecording && gpuIo.cancelRecording != nullptr)
        {
            gpuIo.cancelRecording(gpuIo.user, activeFrameSlot_);
        }
        recordedValid_ = false;
        return;
    }

    lifecycle_ = transaction.candidate;
    pending.hasIdentity = true;
    pending.identity = transaction.identity;
    recordedValid_ = false;

    const RtGpuTimerStateSnapshot timer = gpuIo.snapshot != nullptr
        ? gpuIo.snapshot(gpuIo.user)
        : EmptyTimerSnapshot();
    if (!timingEnabled)
    {
        pending.gpuDisposition = GpuDisposition::Disabled;
        if (timerRecording && gpuIo.cancelRecording != nullptr)
        {
            gpuIo.cancelRecording(gpuIo.user, activeFrameSlot_);
        }
        return;
    }
    if (!timerRecording)
    {
        pending.gpuDisposition = timer.status == GpuFrameTimerStatus::UnsupportedQueue
            ? GpuDisposition::Unsupported
            : GpuDisposition::Error;
        return;
    }
    if (gpuIo.markSubmitted == nullptr ||
        !gpuIo.markSubmitted(gpuIo.user, activeFrameSlot_,
                             transaction.identity.submissionSerial))
    {
        pending.gpuDisposition = GpuDisposition::Error;
        if (gpuIo.cancelRecording != nullptr)
        {
            gpuIo.cancelRecording(gpuIo.user, activeFrameSlot_);
        }
        return;
    }
    pending.gpuDisposition = GpuDisposition::AwaitingTimer;
}

bool RtFrameEvidenceCoordinator::AttachPresentation(
    const RtPresentationOutcome outcome) noexcept
{
    if (!frameActive_ || activeFrameSlot_ >= pending_.size() ||
        !pending_[activeFrameSlot_].hasSuccessfulGraphicsSubmission)
    {
        return false;
    }
    PendingSlot& pending = pending_[activeFrameSlot_];
    if (!pending.hasIdentity)
    {
        return true;
    }
    if (!lifecycle_.AttachPresentation(pending.identity, outcome))
    {
        pending.hasIdentity = false;
        observerAvailable_ = false;
        return false;
    }
    return true;
}

void RtFrameEvidenceCoordinator::ApplyDeferredAggregateReset() noexcept
{
    if (!deferredAggregateReset_)
    {
        return;
    }
    if (stageAccumulator_.Active())
    {
        if (!stageAccumulator_.ResetAggregatesPreservingActive())
        {
            observerAvailable_ = false;
        }
    }
    else if (!stageAccumulator_.ResetAggregates())
    {
        observerAvailable_ = false;
    }
    deferredAggregateReset_ = false;
}

void RtFrameEvidenceCoordinator::FinalizeSubmittedFrame(
    const RtSceneRecordObservation& observation) noexcept
{
    if (!frameActive_)
    {
        return;
    }
    PendingSlot& pending = pending_[activeFrameSlot_];
    if (pending.hasSuccessfulGraphicsSubmission && pending.hasIdentity &&
        observation.healthy)
    {
        RtStageAccumulator candidate = stageAccumulator_;
        const bool candidateReady = !deferredAggregateReset_ ||
            candidate.ResetAggregatesPreservingActive();
        if (candidateReady)
        {
            RtStageFrameSample stages{};
            if (candidate.Commit(stages))
            {
                const RtStageValue& blas =
                    stages.values[RtStageIndex(RtStage::BlasRefitRecord)];
                const RtStageValue& tlas =
                    stages.values[RtStageIndex(RtStage::TlasUpdateRecord)];
                const RtStageValue& traceCopy =
                    stages.values[RtStageIndex(RtStage::TraceCopyRecord)];
                if (blas.workInvocationCount ==
                        commandObservation_.BlasUpdateCount() &&
                    tlas.workInvocationCount ==
                        commandObservation_.TlasUpdateCount() &&
                    traceCopy.workInvocationCount ==
                        commandObservation_.TraceCount() &&
                    commandObservation_.TraceCount() ==
                        commandObservation_.CopyCount())
                {
                    stageAccumulator_ = candidate;
                    pending.stages.identity = pending.identity;
                    pending.stages.stages = stages;
                    pending.hasStages = true;
                    deferredAggregateReset_ = false;
                    ResetAttemptState();
                    return;
                }
            }
        }
    }

    if (stageAccumulator_.Active())
    {
        (void)stageAccumulator_.Abort();
    }
    ApplyDeferredAggregateReset();
    if (pending.hasSuccessfulGraphicsSubmission && pending.hasIdentity)
    {
        pending.stages.identity = pending.identity;
        pending.stages.stages.status = RtSampleStatus::Error;
        pending.hasStages = true;
    }
    ResetAttemptState();
}

void RtFrameEvidenceCoordinator::AbortFrame() noexcept
{
    if (stageAccumulator_.Active())
    {
        (void)stageAccumulator_.Abort();
    }
    ApplyDeferredAggregateReset();
    ResetAttemptState();
}

RtGpuTimingEvidence RtFrameEvidenceCoordinator::BuildGpuEvidence(
    const PendingSlot& pending,
    const GpuFrameTimingCollection* const collection,
    const RtGpuTimerStateSnapshot& timer) noexcept
{
    RtGpuTimingEvidence evidence{};
    evidence.timestampValidBits = timer.timestampValidBits;
    evidence.sampleCount = timer.sampleCount;
    evidence.unavailableResultCount = timer.unavailableResultCount;
    evidence.errorCount = timer.errorCount;
    (void)CheckedTimestampPeriodPicoseconds(
        timer.timestampPeriodNanoseconds, evidence.timestampPeriodPicoseconds);

    switch (pending.gpuDisposition)
    {
    case GpuDisposition::Disabled:
        evidence.status = RtSampleStatus::Disabled;
        SetKnownDetail(evidence.detail, "GPU timestamp timing was disabled for this submission.");
        return evidence;
    case GpuDisposition::Unsupported:
        evidence.status = RtSampleStatus::Unsupported;
        SetKnownDetail(evidence.detail, "The graphics queue does not support timestamp queries.");
        return evidence;
    case GpuDisposition::Error:
        evidence.status = RtSampleStatus::Error;
        evidence.completedSubmissionSerial = pending.identity.submissionSerial;
        SetKnownDetail(evidence.detail, "GPU timestamp recording or submission failed.");
        return evidence;
    case GpuDisposition::AwaitingTimer:
        break;
    }

    if (collection == nullptr || !collection->consumed ||
        collection->frameSlot != pending.identity.frame.frameSlot ||
        collection->submissionSequence != pending.identity.submissionSerial)
    {
        evidence.status = RtSampleStatus::Error;
        evidence.completedSubmissionSerial = pending.identity.submissionSerial;
        SetKnownDetail(evidence.detail, "GPU timestamp result identity did not match its owning submission.");
        return evidence;
    }
    if (collection->status == GpuFrameTimingCollectionStatus::Unavailable)
    {
        evidence.status = RtSampleStatus::Pending;
        SetKnownDetail(evidence.detail, "The consumed GPU timestamp pair was unavailable.");
        return evidence;
    }
    if (collection->status != GpuFrameTimingCollectionStatus::Valid ||
        !collection->hasSample ||
        collection->sample.frameSlot != pending.identity.frame.frameSlot ||
        collection->sample.submissionSequence != pending.identity.submissionSerial ||
        timer.timestampValidBits == 0u ||
        timer.timestampValidBits > 64u ||
        evidence.timestampPeriodPicoseconds == 0u ||
        !CheckedMillisecondsToNanoseconds(
            collection->sample.milliseconds, evidence.durationNanoseconds))
    {
        evidence.status = RtSampleStatus::Error;
        evidence.completedSubmissionSerial = pending.identity.submissionSerial;
        evidence.durationNanoseconds = 0u;
        SetKnownDetail(evidence.detail, "GPU timestamp result could not produce a valid duration.");
        return evidence;
    }
    evidence.status = RtSampleStatus::Valid;
    evidence.hasDuration = true;
    evidence.completedSubmissionSerial = pending.identity.submissionSerial;
    SetKnownDetail(evidence.detail, "GPU RT command-buffer timing is available.");
    return evidence;
}

RtFrameEvidenceCompletionResult RtFrameEvidenceCoordinator::CompleteOwnedSlot(
    const std::uint32_t frameSlot,
    const bool finalIdle,
    const RtGpuFrameTimerIo& gpuIo,
    const RtDiagnosticFrameIo& diagnosticIo) noexcept
{
    RtFrameEvidenceCompletionResult result{};
    if (frameSlot >= activeSlotCount_ || frameSlot >= pending_.size())
    {
        observerAvailable_ = false;
        return result;
    }
    PendingSlot pending = pending_[frameSlot];
    if (!pending.hasSuccessfulGraphicsSubmission)
    {
        return result;
    }
    result.ownedGraphicsSubmission = true;

    const RtGpuTimerStateSnapshot timerBefore = gpuIo.snapshot != nullptr
        ? gpuIo.snapshot(gpuIo.user)
        : EmptyTimerSnapshot();
    RtGpuTimerStateSnapshot timerAfter = timerBefore;
    if (pending.gpuDisposition == GpuDisposition::AwaitingTimer)
    {
        result.gpuCollectionAttempted = true;
        if (gpuIo.collectCompleted != nullptr)
        {
            result.gpuCollection = gpuIo.collectCompleted(gpuIo.user, frameSlot);
        }
        else
        {
            result.gpuCollection.status = GpuFrameTimingCollectionStatus::Error;
            result.gpuCollection.frameSlot = frameSlot;
        }
        timerAfter = gpuIo.snapshot != nullptr
            ? gpuIo.snapshot(gpuIo.user)
            : EmptyTimerSnapshot();
    }

    RtDiagnosticCounterPayload diagnosticPayload{};
    if (instrumentation_ == RtInstrumentationMode::Diagnostic)
    {
        std::string diagnostic;
        if (diagnosticIo.collectCompleted == nullptr ||
            !diagnosticIo.collectCompleted(
                diagnosticIo.user, diagnosticPayload, diagnostic))
        {
            observerAvailable_ = false;
            result.fatalDiagnosticIoFailure = true;
        }
        else if (diagnosticReadCount_ == std::numeric_limits<std::uint64_t>::max())
        {
            pending_[frameSlot] = {};
            observerAvailable_ = false;
            return result;
        }
        else
        {
            ++diagnosticReadCount_;
        }
    }

    pending_[frameSlot] = {};
    if (!pending.hasIdentity)
    {
        return result;
    }

    if (!pending.hasStages)
    {
        pending.stages.identity = pending.identity;
        pending.stages.stages.status = RtSampleStatus::Error;
    }

    RtDiagnosticEvidence diagnosticEvidence{};
    if (instrumentation_ == RtInstrumentationMode::Shipping)
    {
        diagnosticEvidence.status = RtSampleStatus::CompiledOut;
    }
    else
    {
        diagnosticEvidence.status = result.fatalDiagnosticIoFailure
            ? RtSampleStatus::Error : RtSampleStatus::Valid;
        diagnosticEvidence.compiled = true;
        diagnosticEvidence.hasCounters = !result.fatalDiagnosticIoFailure;
        diagnosticEvidence.completedSubmissionSerial = pending.identity.submissionSerial;
        diagnosticEvidence.readCount = diagnosticReadCount_;
        diagnosticEvidence.resetCount = diagnosticResetCount_;
        if (!result.fatalDiagnosticIoFailure)
        {
            diagnosticEvidence.counters = diagnosticPayload.counters;
        }
        SetKnownDetail(diagnosticEvidence.detail,
                       result.fatalDiagnosticIoFailure
                           ? "Diagnostic counter read failed at the owning completion."
                           : "Diagnostic counters were collected after the owning fence.");
    }
    const RtGpuTimingEvidence gpuEvidence = BuildGpuEvidence(
        pending,
        result.gpuCollectionAttempted ? &result.gpuCollection : nullptr,
        timerAfter);
    RtPerformanceEvidenceSnapshot completed{};
    const bool lifecycleCompleted = finalIdle
        ? lifecycle_.CompleteFinalIdle(pending.identity, pending.stages,
                                       diagnosticEvidence, gpuEvidence, completed)
        : lifecycle_.CompleteFence(pending.identity, pending.stages,
                                   diagnosticEvidence, gpuEvidence, completed);
    if (!lifecycleCompleted)
    {
        observerAvailable_ = false;
        return result;
    }
    if (instrumentation_ == RtInstrumentationMode::Diagnostic &&
        !result.fatalDiagnosticIoFailure &&
        diagnosticIo.publishCompleted != nullptr)
    {
        diagnosticIo.publishCompleted(diagnosticIo.user, diagnosticPayload);
    }
    result.completedEvidence = true;
    return result;
}

RtFrameEvidenceCompletionResult RtFrameEvidenceCoordinator::CompleteFence(
    const std::uint32_t frameSlot,
    const RtGpuFrameTimerIo& gpuIo,
    const RtDiagnosticFrameIo& diagnosticIo) noexcept
{
    return CompleteOwnedSlot(frameSlot, false, gpuIo, diagnosticIo);
}

RtFrameEvidenceCompletionResult RtFrameEvidenceCoordinator::CompleteFinalIdle(
    const std::uint32_t frameSlot,
    const RtGpuFrameTimerIo& gpuIo,
    const RtDiagnosticFrameIo& diagnosticIo) noexcept
{
    return CompleteOwnedSlot(frameSlot, true, gpuIo, diagnosticIo);
}

void RtFrameEvidenceCoordinator::NoteFailedDeviceIdle() noexcept
{
    AbortFrame();
    observerAvailable_ = false;
}

bool RtFrameEvidenceCoordinator::HasSuccessfulGraphicsSubmission(
    const std::uint32_t frameSlot) const noexcept
{
    return frameSlot < pending_.size() &&
           pending_[frameSlot].hasSuccessfulGraphicsSubmission;
}

RtLifecycleSeeds RtFrameEvidenceCoordinator::SeedsByValue() const noexcept
{
    RtLifecycleSeeds seeds = lifecycle_.SeedsByValue();
    seeds.sceneEpoch = std::max(seeds.sceneEpoch, preservedSeedFloor_.sceneEpoch);
    seeds.measurementGeneration = std::max(
        seeds.measurementGeneration, preservedSeedFloor_.measurementGeneration);
    seeds.recordAttemptSerial = std::max(
        seeds.recordAttemptSerial, preservedSeedFloor_.recordAttemptSerial);
    seeds.successfulRecordSerial = std::max(
        seeds.successfulRecordSerial, preservedSeedFloor_.successfulRecordSerial);
    seeds.submissionSerial = std::max(
        seeds.submissionSerial, preservedSeedFloor_.submissionSerial);
    seeds.completionSerial = std::max(
        seeds.completionSerial, preservedSeedFloor_.completionSerial);
    return seeds;
}

bool RtFrameEvidenceCoordinator::HasSubmittedIdentity(
    const std::uint32_t frameSlot) const noexcept
{
    return frameSlot < pending_.size() && pending_[frameSlot].hasIdentity;
}

void RtFrameEvidenceCoordinator::LoseCurrentIdentity() noexcept
{
    attemptValid_ = false;
    recordedValid_ = false;
    observerAvailable_ = false;
}

void RtFrameEvidenceCoordinator::ResetAttemptState() noexcept
{
    recordedSceneScratch_ = {};
    commandObservation_ = {};
    attempt_ = {};
    recorded_ = {};
    attemptValid_ = false;
    recordedValid_ = false;
    frameActive_ = false;
    activeFrameSlot_ = 0u;
}

} // namespace horde::vulkan::raytracing
