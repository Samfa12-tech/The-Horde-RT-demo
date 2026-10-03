#include "vulkan/raytracing/RtFrameEvidenceCoordinator.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>

namespace
{

using namespace horde::telemetry;
using namespace horde::vulkan;
using namespace horde::vulkan::raytracing;

struct TestContext
{
    int failures = 0;
    void Check(const bool condition, const std::string_view message)
    {
        if (!condition)
        {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    }
};

struct FakeGpu
{
    RtGpuTimerStateSnapshot state{
        GpuFrameTimerStatus::AwaitingSubmission, 2.0f, 48u, 0u, 0u, 0u};
    GpuFrameTimingCollection nextCollection{};
    std::uint64_t markedSerial = 0u;
    std::uint32_t markCount = 0u;
    std::uint32_t cancelCount = 0u;
    std::uint32_t collectCount = 0u;
    bool markSucceeds = true;
};

RtGpuFrameTimerIo MakeFakeGpuIo(FakeGpu& fake)
{
    return {
        &fake,
        [](void* user, std::uint32_t, const std::uint64_t serial) noexcept {
            auto& state = *static_cast<FakeGpu*>(user);
            ++state.markCount;
            state.markedSerial = serial;
            return state.markSucceeds;
        },
        [](void* user, std::uint32_t) noexcept {
            ++static_cast<FakeGpu*>(user)->cancelCount;
        },
        [](void* user, std::uint32_t) noexcept {
            auto& state = *static_cast<FakeGpu*>(user);
            ++state.collectCount;
            if (state.nextCollection.status ==
                GpuFrameTimingCollectionStatus::Valid)
            {
                ++state.state.sampleCount;
            }
            else if (state.nextCollection.status ==
                     GpuFrameTimingCollectionStatus::Unavailable)
            {
                ++state.state.unavailableResultCount;
            }
            else if (state.nextCollection.status ==
                     GpuFrameTimingCollectionStatus::Error)
            {
                ++state.state.errorCount;
            }
            return state.nextCollection;
        },
        [](void* user) noexcept {
            return static_cast<FakeGpu*>(user)->state;
        }};
}

struct FakeDiagnostic
{
    RtDiagnosticCounterPayload next{};
    std::uint32_t collectCount = 0u;
    std::uint32_t publishCount = 0u;
    RtDiagnosticCounterPayload published{};
    bool collectSucceeds = true;
};

RtDiagnosticFrameIo MakeFakeDiagnosticIo(FakeDiagnostic& fake)
{
    return {
        &fake,
        [](void* user, RtDiagnosticCounterPayload& output,
           std::string& diagnostic) {
            auto& state = *static_cast<FakeDiagnostic*>(user);
            ++state.collectCount;
            if (!state.collectSucceeds)
            {
                diagnostic = "injected Diagnostic read failure";
                return false;
            }
            output = state.next;
            diagnostic.clear();
            return true;
        },
        [](void* user, const RtDiagnosticCounterPayload& value) noexcept {
            auto& state = *static_cast<FakeDiagnostic*>(user);
            ++state.publishCount;
            state.published = value;
        }};
}

RtLifecycleSeeds Seeds()
{
    RtLifecycleSeeds seeds{};
    seeds.sceneEpoch = 1u;
    seeds.measurementGeneration = 1u;
    return seeds;
}

bool SameSubmittedIdentity(const RtSubmittedFrameIdentity& left,
                           const RtSubmittedFrameIdentity& right)
{
    return left.frame.sceneEpoch == right.frame.sceneEpoch &&
           left.frame.measurementGeneration == right.frame.measurementGeneration &&
           left.frame.recordAttemptSerial == right.frame.recordAttemptSerial &&
           left.frame.recordSerial == right.frame.recordSerial &&
           left.frame.simulationTick == right.frame.simulationTick &&
           left.frame.frameSlot == right.frame.frameSlot &&
           left.submissionSerial == right.submissionSerial;
}

bool ClearedSubmittedIdentity(const RtSubmittedFrameIdentity& identity)
{
    return SameSubmittedIdentity(identity, RtSubmittedFrameIdentity{});
}

bool ClearedCompletedEvidence(const RtPerformanceEvidenceSnapshot& evidence)
{
    return ClearedSubmittedIdentity(evidence.identity.submitted) &&
           evidence.identity.completionSerial == 0u &&
           RtFixedTextView(evidence.scene.pipeline.bundleKey).empty() &&
           evidence.scene.resources.bufferCount == 0u &&
           evidence.scene.stages.status == RtSampleStatus::NotReady &&
           evidence.dielectric.status == RtSampleStatus::NotReady &&
           evidence.gpu.status == RtSampleStatus::NotReady &&
           evidence.presentation.outcome == RtPresentationOutcome::NotAttempted &&
           evidence.presentation.lastSuccessfulPresentSubmissionSerial == 0u &&
           !evidence.presentation.finalIdleCompletion &&
           !evidence.cpuBenchmarkEligible &&
           !evidence.benchmarkEligible;
}

void PopulateRecordedScene(RtSceneRecordObservation& observation,
                           const RtInstrumentationMode instrumentation,
                           const char hashCharacter)
{
    if (observation.commands != nullptr)
    {
        (void)observation.commands->Note(RtSceneCommandEvent::HostWriteBarrier);
        (void)observation.commands->Note(RtSceneCommandEvent::TlasUpdate);
        (void)observation.commands->Note(RtSceneCommandEvent::TlasToTraceBarrier);
        (void)observation.commands->Note(RtSceneCommandEvent::Trace);
        (void)observation.commands->Note(RtSceneCommandEvent::CopyOrBlit);
    }
    if (observation.stages != nullptr)
    {
        (void)observation.stages->Accumulate(
            RtStage::TlasUpdateRecord, 0u, 1u, 0u, 1u);
        (void)observation.stages->Accumulate(
            RtStage::TraceCopyRecord, 0u, 1u, 0u, 1u);
    }
    RtRecordedSceneEvidence scene{};
    scene.pipeline.instrumentation = instrumentation;
    scene.pipeline.dielectricQuality = RtDielectricQuality::High;
    scene.pipeline.activeStrategy = RtMaterialStrategy::OpaqueFast;
    scene.pipeline.waterQuality = RtWaterQuality::High;
    (void)AssignRtFixedText(scene.pipeline.bundleKey,
                           instrumentation == RtInstrumentationMode::Shipping
                               ? "shipping_high_pair"
                               : "diagnostic_high_pair");
    (void)AssignRtFixedText(scene.pipeline.opaqueFast.key,
                           "opaque_fast");
    (void)AssignRtFixedText(scene.pipeline.genericDielectric.key,
                           "generic_dielectric");
    const std::string opaqueHash(64u, hashCharacter);
    const std::string genericHash(64u, hashCharacter == 'a' ? 'b' : 'd');
    (void)AssignRtFixedText(scene.pipeline.opaqueFast.sha256, opaqueHash);
    (void)AssignRtFixedText(scene.pipeline.genericDielectric.sha256, genericHash);
    scene.pipeline.active = scene.pipeline.opaqueFast;
    scene.resources.bufferCount = 2u;
    scene.resources.memoryAllocationCount = 2u;
    scene.resources.bottomLevelAccelerationStructureCount = 1u;
    scene.resources.topLevelAccelerationStructureCount = 1u;
    scene.resources.tlasInstanceCount = 20u;
    scene.resources.pipelineCount = 2u;
    scene.resources.shaderBindingTableCount = 2u;
    scene.resources.descriptorSetCount = 1u;
    scene.player.skinCadenceHz = 60u;
    scene.dispatch.sceneReady = true;
    scene.dispatch.rtDispatchRecorded = true;
    scene.dispatch.swapchainCopyRecorded = true;
    *observation.recordedScene = scene;
}

RtEvidenceSubmitTransaction RecordAndPrevalidate(
    TestContext& test,
    RtFrameEvidenceCoordinator& coordinator,
    RtSceneRecordObservation& observation,
    const RtInstrumentationMode instrumentation,
    const std::uint64_t tick,
    const bool diagnosticReset = false)
{
    test.Check(coordinator.BeginRecord(tick),
               "coordinator record identity must begin");
    PopulateRecordedScene(observation, instrumentation,
                          instrumentation == RtInstrumentationMode::Shipping ? 'a' : 'c');
    observation.diagnosticResetCompleted = diagnosticReset;
    test.Check(coordinator.FinishRecord(observation),
               "coordinator record identity must finish");
    const RtEvidenceSubmitTransaction transaction = coordinator.PrevalidateSubmit();
    test.Check(transaction.valid,
               "coordinator submission transaction must prevalidate");
    return transaction;
}

void SubmitPresentedFrame(TestContext& test,
                          RtFrameEvidenceCoordinator& coordinator,
                          RtSceneRecordObservation& observation,
                          const RtInstrumentationMode instrumentation,
                          FakeGpu& gpu,
                          const bool timingEnabled = false,
                          const bool timerRecording = false,
                          const std::uint64_t tick = 1u,
                          const bool diagnosticReset = false)
{
    const RtEvidenceSubmitTransaction transaction = RecordAndPrevalidate(
        test, coordinator, observation, instrumentation, tick, diagnosticReset);
    coordinator.CommitGraphicsSubmit(
        transaction, timingEnabled, timerRecording, MakeFakeGpuIo(gpu));
    test.Check(coordinator.AttachPresentation(RtPresentationOutcome::Presented),
               "presented outcome must attach independently");
    coordinator.FinalizeSubmittedFrame(observation);
}

void TestCommittedIdentityHandoff(TestContext& test)
{
    RtFrameEvidenceCoordinator coordinator;
    FakeGpu gpu;
    test.Check(coordinator.Initialise(Seeds(), 1u,
                                      RtInstrumentationMode::Shipping,
                                      RtSampleStatus::Disabled, false),
               "committed-identity fixture must initialise");

    RtSubmittedFrameIdentity handedOff{};
    handedOff.submissionSerial = 999u;
    test.Check(!coordinator.TryGetCommittedIdentity(0u, handedOff) &&
                   ClearedSubmittedIdentity(handedOff),
               "empty slot must not leak a caller's old submitted identity");
    handedOff.submissionSerial = 999u;
    test.Check(!coordinator.TryGetCommittedIdentity(1u, handedOff) &&
                   ClearedSubmittedIdentity(handedOff),
               "invalid slot must clear output and expose no identity");

    RtSceneRecordObservation observation{};
    test.Check(coordinator.BeginFrame(0u, observation),
               "committed-identity frame must begin");
    const RtEvidenceSubmitTransaction transaction = RecordAndPrevalidate(
        test, coordinator, observation, RtInstrumentationMode::Shipping, 301u);
    handedOff.submissionSerial = 999u;
    test.Check(!coordinator.TryGetCommittedIdentity(0u, handedOff) &&
                   ClearedSubmittedIdentity(handedOff),
               "prevalidated identity must not be authoritative before actual commit");

    coordinator.CommitGraphicsSubmit(
        transaction, false, false, MakeFakeGpuIo(gpu));
    test.Check(coordinator.TryGetCommittedIdentity(0u, handedOff) &&
                   SameSubmittedIdentity(handedOff, transaction.identity) &&
                   handedOff.frame.simulationTick == 301u,
               "successful graphics commit must expose its exact accepted identity");

    test.Check(coordinator.Recreate(RtResourceResetReason::SwapchainRecreate,
                                    RtSampleStatus::Disabled),
               "recreation must invalidate the pending committed identity");
    handedOff = transaction.identity;
    test.Check(!coordinator.TryGetCommittedIdentity(0u, handedOff) &&
                   ClearedSubmittedIdentity(handedOff),
               "recreation must clear output instead of leaking the retired identity");

    RtSceneRecordObservation failedObservation{};
    test.Check(coordinator.BeginFrame(0u, failedObservation),
               "failed-submit identity frame must begin");
    (void)RecordAndPrevalidate(test, coordinator, failedObservation,
                              RtInstrumentationMode::Shipping, 302u);
    coordinator.FailGraphicsSubmit(false, MakeFakeGpuIo(gpu));
    handedOff.submissionSerial = 999u;
    test.Check(!coordinator.TryGetCommittedIdentity(0u, handedOff) &&
                   ClearedSubmittedIdentity(handedOff),
               "failed graphics submit must expose no committed identity");

    RtLifecycleSeeds exhausted = Seeds();
    exhausted.submissionSerial = std::numeric_limits<std::uint64_t>::max();
    RtFrameEvidenceCoordinator tokenless;
    test.Check(tokenless.Initialise(exhausted, 1u,
                                    RtInstrumentationMode::Shipping,
                                    RtSampleStatus::Disabled, false),
               "tokenless committed-identity fixture must initialise");
    RtSceneRecordObservation tokenlessObservation{};
    test.Check(tokenless.BeginFrame(0u, tokenlessObservation) &&
                   tokenless.BeginRecord(303u),
               "tokenless committed-identity frame must begin recording");
    PopulateRecordedScene(tokenlessObservation,
                          RtInstrumentationMode::Shipping, 'a');
    test.Check(tokenless.FinishRecord(tokenlessObservation),
               "tokenless committed-identity frame must finish recording");
    const RtEvidenceSubmitTransaction rejected = tokenless.PrevalidateSubmit();
    test.Check(!rejected.valid,
               "serial exhaustion must prevent optional identity prevalidation");
    tokenless.CommitGraphicsSubmit(
        rejected, false, false, MakeFakeGpuIo(gpu));
    handedOff.submissionSerial = 999u;
    test.Check(tokenless.HasSuccessfulGraphicsSubmission(0u) &&
                   !tokenless.TryGetCommittedIdentity(0u, handedOff) &&
                   ClearedSubmittedIdentity(handedOff),
               "tokenless successful graphics must retain ownership without inventing identity");
}

void TestCompletionOutputHandoff(TestContext& test)
{
    FakeGpu gpu;
    FakeDiagnostic diagnostic;
    RtFrameEvidenceCoordinator fenceCoordinator;
    test.Check(fenceCoordinator.Initialise(Seeds(), 1u,
                                           RtInstrumentationMode::Shipping,
                                           RtSampleStatus::Disabled, false),
               "fence completion-output fixture must initialise");
    RtSceneRecordObservation fenceObservation{};
    test.Check(fenceCoordinator.BeginFrame(0u, fenceObservation),
               "fence completion-output frame must begin");
    SubmitPresentedFrame(test, fenceCoordinator, fenceObservation,
                         RtInstrumentationMode::Shipping, gpu,
                         false, false, 401u);
    RtSubmittedFrameIdentity fenceIdentity{};
    test.Check(fenceCoordinator.TryGetCommittedIdentity(0u, fenceIdentity),
               "fence fixture must expose its committed identity before completion");

    RtPerformanceEvidenceSnapshot fenceOutput{};
    fenceOutput.identity.completionSerial = 999u;
    const RtFrameEvidenceCompletionResult fenceResult =
        fenceCoordinator.CompleteFence(
            0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(diagnostic),
            &fenceOutput);
    test.Check(fenceResult.completedEvidence &&
                   SameSubmittedIdentity(fenceOutput.identity.submitted,
                                         fenceIdentity) &&
                   fenceOutput.identity.completionSerial == 1u &&
                   fenceOutput.identity.submitted.frame.simulationTick == 401u &&
                   RtFixedTextView(fenceOutput.scene.pipeline.bundleKey) ==
                       "shipping_high_pair" &&
                   fenceOutput.dielectric.status == RtSampleStatus::CompiledOut &&
                   fenceOutput.gpu.status == RtSampleStatus::Disabled &&
                   fenceOutput.presentation.outcome ==
                       RtPresentationOutcome::Presented &&
                   !fenceOutput.presentation.finalIdleCompletion,
               "owning fence must return the exact lifecycle-accepted completion");

    RtPerformanceEvidenceSnapshot duplicateOutput = fenceOutput;
    const RtFrameEvidenceCompletionResult duplicate =
        fenceCoordinator.CompleteFence(
            0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(diagnostic),
            &duplicateOutput);
    test.Check(!duplicate.ownedGraphicsSubmission && !duplicate.completedEvidence &&
                   ClearedCompletedEvidence(duplicateOutput),
               "duplicate or empty fence completion must clear old output");

    RtFrameEvidenceCoordinator idleCoordinator;
    test.Check(idleCoordinator.Initialise(Seeds(), 1u,
                                          RtInstrumentationMode::Shipping,
                                          RtSampleStatus::Disabled, false),
               "final-idle completion-output fixture must initialise");
    RtSceneRecordObservation idleObservation{};
    test.Check(idleCoordinator.BeginFrame(0u, idleObservation),
               "final-idle completion-output frame must begin");
    SubmitPresentedFrame(test, idleCoordinator, idleObservation,
                         RtInstrumentationMode::Shipping, gpu,
                         false, false, 402u);
    RtSubmittedFrameIdentity idleIdentity{};
    test.Check(idleCoordinator.TryGetCommittedIdentity(0u, idleIdentity),
               "final-idle fixture must expose its committed identity");
    RtPerformanceEvidenceSnapshot idleOutput{};
    const RtFrameEvidenceCompletionResult idleResult =
        idleCoordinator.CompleteFinalIdle(
            0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(diagnostic),
            &idleOutput);
    test.Check(idleResult.completedEvidence &&
                   SameSubmittedIdentity(idleOutput.identity.submitted,
                                         idleIdentity) &&
                   idleOutput.identity.completionSerial == 1u &&
                   idleOutput.identity.submitted.frame.simulationTick == 402u &&
                   RtFixedTextView(idleOutput.scene.pipeline.bundleKey) ==
                       "shipping_high_pair" &&
                   idleOutput.presentation.outcome ==
                       RtPresentationOutcome::Presented &&
                   idleOutput.presentation.finalIdleCompletion,
               "successful final idle must return its exact accepted completion");

    RtLifecycleSeeds exhausted = Seeds();
    exhausted.submissionSerial = std::numeric_limits<std::uint64_t>::max();
    RtFrameEvidenceCoordinator tokenless;
    test.Check(tokenless.Initialise(exhausted, 1u,
                                    RtInstrumentationMode::Shipping,
                                    RtSampleStatus::Disabled, false),
               "tokenless completion-output fixture must initialise");
    RtSceneRecordObservation tokenlessObservation{};
    test.Check(tokenless.BeginFrame(0u, tokenlessObservation) &&
                   tokenless.BeginRecord(403u),
               "tokenless completion-output frame must begin recording");
    PopulateRecordedScene(tokenlessObservation,
                          RtInstrumentationMode::Shipping, 'a');
    test.Check(tokenless.FinishRecord(tokenlessObservation),
               "tokenless completion-output frame must finish recording");
    const RtEvidenceSubmitTransaction rejected = tokenless.PrevalidateSubmit();
    tokenless.CommitGraphicsSubmit(
        rejected, false, false, MakeFakeGpuIo(gpu));
    test.Check(tokenless.AttachPresentation(RtPresentationOutcome::Presented),
               "tokenless completion-output frame must retain presentation ownership");
    tokenless.FinalizeSubmittedFrame(tokenlessObservation);
    RtPerformanceEvidenceSnapshot tokenlessOutput = fenceOutput;
    const RtFrameEvidenceCompletionResult tokenlessResult =
        tokenless.CompleteFence(
            0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(diagnostic),
            &tokenlessOutput);
    test.Check(tokenlessResult.ownedGraphicsSubmission &&
                   !tokenlessResult.completedEvidence &&
                   ClearedCompletedEvidence(tokenlessOutput),
               "tokenless completion must clear output without leaking an older frame");
}

void TestSuboptimalPresentation(TestContext& test)
{
    RtFrameEvidenceCoordinator coordinator;
    FakeGpu gpu;
    FakeDiagnostic unused;
    test.Check(coordinator.Initialise(Seeds(), 1u, RtInstrumentationMode::Shipping,
                                      RtSampleStatus::Disabled, false),
               "suboptimal fixture must initialise");
    RtSceneRecordObservation observation{};
    test.Check(coordinator.BeginFrame(0u, observation), "suboptimal frame must begin");
    const auto transaction = RecordAndPrevalidate(
        test, coordinator, observation, RtInstrumentationMode::Shipping, 24u);
    coordinator.CommitGraphicsSubmit(transaction, false, false, MakeFakeGpuIo(gpu));
    test.Check(coordinator.AttachPresentation(RtPresentationOutcome::PresentedNeedsRecreate),
               "suboptimal is successful presentation needing recreation");
    coordinator.FinalizeSubmittedFrame(observation);
    const auto completion = coordinator.CompleteFence(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(unused));
    const auto state = coordinator.PublishedStateByValue();
    test.Check(completion.completedEvidence && state.presented &&
                   state.completedEvidence.presentation.lastSuccessfulPresentSubmissionSerial ==
                       transaction.identity.submissionSerial &&
                   !state.completedEvidence.benchmarkEligible,
               "suboptimal must count its successful present serial but exclude benchmark sample");
    std::string json;
    RtEvidenceValidationError error{};
    test.Check(SerializeRtPerformanceEvidenceJson(state.completedEvidence, json, error) &&
                   json.find("\"outcome\":\"presented-needs-recreate\",\"presented\":true") !=
                       std::string::npos,
               "suboptimal JSON must report honest successful presentation");
    auto invalidBenchmark = state.completedEvidence;
    invalidBenchmark.benchmarkEligible = true;
    test.Check(!ValidateRtPerformanceEvidence(invalidBenchmark, error),
               "suboptimal cannot be forged into an eligible benchmark sample");
}

void TestInitialFenceAndDiagnosticOwnership(TestContext& test)
{
    RtFrameEvidenceCoordinator coordinator;
    FakeGpu gpu;
    FakeDiagnostic diagnostic;
    diagnostic.next.counters[kRtPrimaryPlayerPixelCounterIndex] = 73u;
    test.Check(coordinator.Initialise(Seeds(), 1u,
                                      RtInstrumentationMode::Diagnostic,
                                      RtSampleStatus::Disabled, false),
               "Diagnostic coordinator must initialise");
    RtSceneRecordObservation observation{};
    test.Check(coordinator.BeginFrame(0u, observation),
               "Diagnostic frame scratch must begin before the initial fence");
    const RtFrameEvidenceCompletionResult initial = coordinator.CompleteFence(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(diagnostic));
    test.Check(!initial.ownedGraphicsSubmission && diagnostic.collectCount == 0u &&
                   gpu.collectCount == 0u,
               "initial signalled fence must own no GPU or Diagnostic IO");
    test.Check(observation.stages->Accumulate(
                   RtStage::FrameFenceWait, 7u, 1u, 0u, 1u),
               "fence duration must remain in current frame scratch");
    SubmitPresentedFrame(test, coordinator, observation,
                         RtInstrumentationMode::Diagnostic, gpu,
                         false, false, 11u, true);
    const RtFrameEvidenceCompletionResult completed = coordinator.CompleteFence(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(diagnostic));
    const RtLifecyclePublishedState published = coordinator.PublishedStateByValue();
    test.Check(completed.ownedGraphicsSubmission && completed.completedEvidence &&
                   !completed.fatalDiagnosticIoFailure &&
                   diagnostic.collectCount == 1u && diagnostic.publishCount == 1u &&
                   diagnostic.published.counters[kRtPrimaryPlayerPixelCounterIndex] == 73u,
               "owning fence must read then publish one exact Diagnostic payload");
    test.Check(published.hasCompletedEvidence &&
                   published.completedEvidence.identity.submitted.frame.simulationTick == 11u &&
                   published.completedEvidence.dielectric.counters[
                       kRtPrimaryPlayerPixelCounterIndex] == 73u &&
                   published.completedEvidence.scene.player.primaryPixelCountAvailable &&
                   published.completedEvidence.scene.player.primaryPixelCount == 73u &&
                   published.completedEvidence.scene.player.primaryVisible &&
                   published.completedEvidence.dielectric.readCount == 1u &&
                   published.completedEvidence.dielectric.resetCount == 1u,
               "collected Diagnostic counters must join their exact submitted identity");
}

void TestTokenlessSubmissionAndFatalDiagnosticRead(TestContext& test)
{
    RtLifecycleSeeds exhausted = Seeds();
    exhausted.submissionSerial = std::numeric_limits<std::uint64_t>::max();
    RtFrameEvidenceCoordinator coordinator;
    FakeGpu gpu;
    FakeDiagnostic diagnostic;
    test.Check(coordinator.Initialise(exhausted, 1u,
                                      RtInstrumentationMode::Diagnostic,
                                      RtSampleStatus::Disabled, false),
               "submission-exhaustion coordinator must initialise");
    RtSceneRecordObservation observation{};
    test.Check(coordinator.BeginFrame(0u, observation) &&
                   coordinator.BeginRecord(19u),
               "tokenless fixture must reach a recorded graphics attempt");
    PopulateRecordedScene(observation, RtInstrumentationMode::Diagnostic, 'c');
    observation.diagnosticResetCompleted = true;
    test.Check(coordinator.FinishRecord(observation),
               "tokenless fixture must finish its record identity");
    const RtEvidenceSubmitTransaction rejected = coordinator.PrevalidateSubmit();
    test.Check(!rejected.valid && !coordinator.ObserverAvailable(),
               "identity exhaustion must reject only optional evidence");
    coordinator.CommitGraphicsSubmit(
        rejected, false, false, MakeFakeGpuIo(gpu));
    test.Check(coordinator.HasSuccessfulGraphicsSubmission(0u) &&
                   !coordinator.HasSubmittedIdentity(0u) &&
                   coordinator.AttachPresentation(RtPresentationOutcome::Presented),
               "real graphics ownership must survive tokenless submission");
    coordinator.FinalizeSubmittedFrame(observation);
    const RtFrameEvidenceCompletionResult tokenless = coordinator.CompleteFence(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(diagnostic));
    test.Check(tokenless.ownedGraphicsSubmission && !tokenless.completedEvidence &&
                   diagnostic.collectCount == 1u && diagnostic.publishCount == 0u &&
                   !coordinator.PublishedStateByValue().hasCompletedEvidence,
               "tokenless fence must drain Diagnostic IO without stale publication");

    RtFrameEvidenceCoordinator failing;
    FakeDiagnostic failedRead;
    failedRead.collectSucceeds = false;
    test.Check(failing.Initialise(Seeds(), 1u,
                                  RtInstrumentationMode::Diagnostic,
                                  RtSampleStatus::Disabled, false),
               "Diagnostic read-failure fixture must initialise");
    RtSceneRecordObservation failedObservation{};
    test.Check(failing.BeginFrame(0u, failedObservation),
               "Diagnostic read-failure frame must begin");
    SubmitPresentedFrame(test, failing, failedObservation,
                         RtInstrumentationMode::Diagnostic, gpu,
                         false, false, 20u, true);
    RtPerformanceEvidenceSnapshot readFailureOutput{};
    const RtFrameEvidenceCompletionResult readFailure = failing.CompleteFence(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(failedRead),
        &readFailureOutput);
    test.Check(readFailure.fatalDiagnosticIoFailure &&
                   readFailure.completedEvidence && failedRead.publishCount == 0u &&
                   readFailureOutput.identity.submitted.frame.simulationTick == 20u &&
                   readFailureOutput.dielectric.status == RtSampleStatus::Error,
               "actual Diagnostic read IO failure must publish Error but no counter payload");
    const auto failedState = failing.PublishedStateByValue();
    test.Check(failedState.hasCompletedEvidence &&
                   failedState.diagnosticStatus == RtSampleStatus::Error &&
                   failedState.completedEvidence.identity.submitted.frame.simulationTick == 20u &&
                   failedState.completedEvidence.dielectric.status == RtSampleStatus::Error &&
                   !failedState.completedEvidence.dielectric.hasCounters &&
                   !failedState.completedEvidence.scene.player.primaryPixelCountAvailable &&
                   !failedState.completedEvidence.benchmarkEligible,
               "Diagnostic read failure must complete its exact owner as Error without stale pixels");
}

void TestDiagnosticReadBeforeNextResetOrdering(TestContext& test)
{
    RtFrameEvidenceCoordinator coordinator;
    FakeGpu gpu;
    FakeDiagnostic diagnostic;
    test.Check(coordinator.Initialise(Seeds(), 1u,
                                      RtInstrumentationMode::Diagnostic,
                                      RtSampleStatus::Disabled, false),
               "Diagnostic A/B fixture must initialise");

    RtSceneRecordObservation frameA{};
    test.Check(coordinator.BeginFrame(0u, frameA),
               "Diagnostic frame A must begin");
    SubmitPresentedFrame(test, coordinator, frameA,
                         RtInstrumentationMode::Diagnostic, gpu,
                         false, false, 101u, true);

    RtSceneRecordObservation frameB{};
    test.Check(coordinator.BeginFrame(0u, frameB),
               "Diagnostic frame B scratch must begin before collecting A");
    diagnostic.next.counters[kRtPrimaryPlayerPixelCounterIndex] = 11u;
    const RtFrameEvidenceCompletionResult completedA = coordinator.CompleteFence(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(diagnostic));
    test.Check(completedA.completedEvidence && diagnostic.collectCount == 1u &&
                   diagnostic.publishCount == 1u &&
                   coordinator.PublishedStateByValue().completedEvidence
                           .identity.submitted.frame.simulationTick == 101u &&
                   coordinator.PublishedStateByValue().completedEvidence
                           .scene.player.primaryPixelCount == 11u &&
                   frameB.stages != nullptr && frameB.stages->Active(),
               "A must collect into A identity while B host-frame scratch remains active");
    SubmitPresentedFrame(test, coordinator, frameB,
                         RtInstrumentationMode::Diagnostic, gpu,
                         false, false, 202u, true);

    RtSceneRecordObservation frameC{};
    test.Check(coordinator.BeginFrame(0u, frameC),
               "Diagnostic frame C scratch must begin before collecting B");
    diagnostic.next.counters[kRtPrimaryPlayerPixelCounterIndex] = 22u;
    const RtFrameEvidenceCompletionResult completedB = coordinator.CompleteFence(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(diagnostic));
    const RtPerformanceEvidenceSnapshot evidenceB =
        coordinator.PublishedStateByValue().completedEvidence;
    test.Check(completedB.completedEvidence && diagnostic.collectCount == 2u &&
                   diagnostic.publishCount == 2u &&
                   evidenceB.identity.submitted.frame.simulationTick == 202u &&
                   evidenceB.scene.player.primaryPixelCount == 22u &&
                   evidenceB.dielectric.readCount == 2u &&
                   evidenceB.dielectric.resetCount == 2u,
               "B must receive only B's post-fence Diagnostic record after A was consumed once");
    coordinator.AbortFrame();
}

void TestInitialAndBeginIdentityFailureStillDrain(TestContext& test)
{
    FakeGpu gpu;
    FakeDiagnostic diagnostic;
    RtLifecycleSeeds initialExhausted = Seeds();
    initialExhausted.sceneEpoch = std::numeric_limits<std::uint64_t>::max();
    RtFrameEvidenceCoordinator initialFailure;
    test.Check(initialFailure.Initialise(initialExhausted, 1u,
                                         RtInstrumentationMode::Diagnostic,
                                         RtSampleStatus::Disabled, false) &&
                   !initialFailure.ObserverAvailable(),
               "optional lifecycle initialization exhaustion must leave graphics ownership usable");
    RtSceneRecordObservation initialObservation{};
    test.Check(initialFailure.BeginFrame(0u, initialObservation) &&
                   !initialFailure.BeginRecord(30u),
               "observer-unavailable initialization must still begin a real graphics frame");
    PopulateRecordedScene(initialObservation, RtInstrumentationMode::Diagnostic, 'c');
    initialObservation.diagnosticResetCompleted = true;
    test.Check(!initialFailure.FinishRecord(initialObservation),
               "unavailable initial lifecycle must not fabricate a recorded token");
    const RtEvidenceSubmitTransaction noInitialIdentity =
        initialFailure.PrevalidateSubmit();
    initialFailure.CommitGraphicsSubmit(
        noInitialIdentity, false, false, MakeFakeGpuIo(gpu));
    test.Check(initialFailure.HasSuccessfulGraphicsSubmission(0u) &&
                   !initialFailure.HasSubmittedIdentity(0u) &&
                   initialFailure.AttachPresentation(RtPresentationOutcome::Presented),
               "real submission after optional initialization failure must retain fence ownership");
    initialFailure.FinalizeSubmittedFrame(initialObservation);
    const RtFrameEvidenceCompletionResult initialDrain =
        initialFailure.CompleteFence(
            0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(diagnostic));
    test.Check(initialDrain.ownedGraphicsSubmission &&
                   !initialDrain.completedEvidence &&
                   diagnostic.collectCount == 1u && diagnostic.publishCount == 0u,
               "initial identity failure must drain once without stale publication");
    const RtLifecycleSeeds retainedExhaustion = initialFailure.SeedsByValue();
    test.Check(retainedExhaustion.sceneEpoch ==
                   std::numeric_limits<std::uint64_t>::max(),
               "failed lifecycle initialization must retain the incoming monotonic seed floor");
    test.Check(initialFailure.Recreate(
                   RtResourceResetReason::SwapchainRecreate,
                   RtSampleStatus::Disabled) &&
                   !initialFailure.ObserverAvailable(),
               "resource recreation after optional lifecycle failure must preserve the graphics shell");
    RtSceneRecordObservation recreatedObservation{};
    test.Check(initialFailure.BeginFrame(0u, recreatedObservation) &&
                   !initialFailure.BeginRecord(30u),
               "recreated observer-unavailable shell must still accept a graphics frame");
    initialFailure.CommitGraphicsSubmit(
        initialFailure.PrevalidateSubmit(), false, false, MakeFakeGpuIo(gpu));
    test.Check(initialFailure.AttachPresentation(RtPresentationOutcome::Presented),
               "recreated tokenless shell must retain presentation ownership");
    initialFailure.FinalizeSubmittedFrame(recreatedObservation);
    const RtFrameEvidenceCompletionResult recreatedDrain =
        initialFailure.CompleteFence(
            0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(diagnostic));
    test.Check(recreatedDrain.ownedGraphicsSubmission &&
                   diagnostic.collectCount == 2u && diagnostic.publishCount == 0u,
               "recreated tokenless shell must drain exactly once without publication");
    (void)initialFailure.Destroy();
    RtFrameEvidenceCoordinator restarted;
    test.Check(restarted.Initialise(retainedExhaustion, 1u,
                                    RtInstrumentationMode::Diagnostic,
                                    RtSampleStatus::Disabled, false) &&
                   !restarted.ObserverAvailable(),
               "destroy/move/restart after exhaustion must not mint a reusable identity range");

    RtLifecycleSeeds beginExhausted = Seeds();
    beginExhausted.recordAttemptSerial =
        std::numeric_limits<std::uint64_t>::max();
    RtFrameEvidenceCoordinator beginFailure;
    FakeDiagnostic beginDiagnostic;
    test.Check(beginFailure.Initialise(beginExhausted, 1u,
                                       RtInstrumentationMode::Diagnostic,
                                       RtSampleStatus::Disabled, false),
               "record-attempt exhaustion fixture must initialize its ownership shell");
    RtSceneRecordObservation beginObservation{};
    test.Check(beginFailure.BeginFrame(0u, beginObservation) &&
                   !beginFailure.BeginRecord(31u),
               "BeginRecord exhaustion must disable only optional identity");
    PopulateRecordedScene(beginObservation, RtInstrumentationMode::Diagnostic, 'c');
    beginObservation.diagnosticResetCompleted = true;
    test.Check(!beginFailure.FinishRecord(beginObservation),
               "BeginRecord exhaustion must not acquire a later token");
    beginFailure.CommitGraphicsSubmit(
        beginFailure.PrevalidateSubmit(), false, false, MakeFakeGpuIo(gpu));
    test.Check(beginFailure.AttachPresentation(RtPresentationOutcome::Presented),
               "tokenless BeginRecord failure frame must retain presentation path");
    beginFailure.FinalizeSubmittedFrame(beginObservation);
    const RtFrameEvidenceCompletionResult beginDrain = beginFailure.CompleteFence(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(beginDiagnostic));
    test.Check(beginDrain.ownedGraphicsSubmission &&
                   beginDiagnostic.collectCount == 1u &&
                   beginDiagnostic.publishCount == 0u,
               "BeginRecord failure must drain Diagnostic IO once without a token");
}

void TestShippingPoisonedIoAndQueueFailure(TestContext& test)
{
    RtFrameEvidenceCoordinator coordinator;
    FakeGpu gpu;
    FakeDiagnostic poisoned;
    poisoned.collectSucceeds = false;
    test.Check(coordinator.Initialise(Seeds(), 1u,
                                      RtInstrumentationMode::Shipping,
                                      RtSampleStatus::Disabled, false),
               "Shipping coordinator must initialise");
    RtSceneRecordObservation observation{};
    test.Check(coordinator.BeginFrame(0u, observation),
               "Shipping frame must begin");
    SubmitPresentedFrame(test, coordinator, observation,
                         RtInstrumentationMode::Shipping, gpu);
    const RtFrameEvidenceCompletionResult completed = coordinator.CompleteFence(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(poisoned));
    test.Check(completed.completedEvidence && !completed.fatalDiagnosticIoFailure &&
                   poisoned.collectCount == 0u && poisoned.publishCount == 0u,
               "Shipping must execute zero Diagnostic IO even with poisoned callbacks");

    const RtStageAggregateSet beforeFailure = coordinator.StageAggregatesByValue();
    RtSceneRecordObservation failed{};
    test.Check(coordinator.BeginFrame(0u, failed),
               "queue failure scratch must begin");
    const RtEvidenceSubmitTransaction transaction = RecordAndPrevalidate(
        test, coordinator, failed, RtInstrumentationMode::Shipping, 2u);
    coordinator.FailGraphicsSubmit(false, MakeFakeGpuIo(gpu));
    const RtStageAggregateSet afterFailure = coordinator.StageAggregatesByValue();
    test.Check(transaction.valid && !coordinator.HasSuccessfulGraphicsSubmission(0u) &&
                   afterFailure.values[RtStageIndex(RtStage::WholeFrameCycle)].sampleCount ==
                       beforeFailure.values[RtStageIndex(RtStage::WholeFrameCycle)].sampleCount,
               "actual queue failure must own no completion or committed stage sample");
}

void TestRecordEndAndPresentationIdentityFailure(TestContext& test)
{
    for (const bool endFailure : {false, true})
    {
        RtFrameEvidenceCoordinator coordinator;
        FakeGpu gpu;
        FakeDiagnostic diagnostic;
        test.Check(coordinator.Initialise(Seeds(), 1u, RtInstrumentationMode::Diagnostic,
                                          RtSampleStatus::Disabled, false),
                   "record failure fixture must initialise");
        RtSceneRecordObservation observation{};
        test.Check(coordinator.BeginFrame(0u, observation) && coordinator.BeginRecord(30u),
                   "record failure fixture must start real attempt");
        if (endFailure)
        {
            PopulateRecordedScene(observation, RtInstrumentationMode::Diagnostic, 'c');
            observation.diagnosticResetCompleted = true;
            test.Check(coordinator.FinishRecord(observation),
                       "end failure happens after successful scene recording");
            coordinator.FailGraphicsSubmit(true, MakeFakeGpuIo(gpu));
        }
        else
        {
            coordinator.FailRecord(observation);
        }
        const auto completion = coordinator.CompleteFence(
            0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(diagnostic));
        test.Check(!completion.ownedGraphicsSubmission && !completion.completedEvidence &&
                       diagnostic.collectCount == 0u &&
                       coordinator.SeedsByValue().submissionSerial == 0u &&
                       !coordinator.FrameActive() &&
                       coordinator.StageAggregatesByValue().values[
                           RtStageIndex(RtStage::WholeFrameCycle)].sampleCount == 0u &&
                       gpu.cancelCount == (endFailure ? 1u : 0u),
                   "failed record/end must consume no submission, samples or Diagnostic read");
    }

    RtFrameEvidenceCoordinator coordinator;
    FakeGpu gpu;
    FakeDiagnostic diagnostic;
    test.Check(coordinator.Initialise(Seeds(), 1u, RtInstrumentationMode::Diagnostic,
                                      RtSampleStatus::Disabled, false),
               "corrupted presentation identity fixture must initialise");
    RtSceneRecordObservation observation{};
    test.Check(coordinator.BeginFrame(0u, observation), "corrupted frame must begin");
    auto transaction = RecordAndPrevalidate(
        test, coordinator, observation, RtInstrumentationMode::Diagnostic, 31u, true);
    // The candidate lifecycle still owns the original serial; corrupt only the
    // adapter copy to prove AttachPresentation cannot publish a mismatched join.
    ++transaction.identity.submissionSerial;
    coordinator.CommitGraphicsSubmit(transaction, false, false, MakeFakeGpuIo(gpu));
    test.Check(!coordinator.AttachPresentation(RtPresentationOutcome::Presented) &&
                   !coordinator.HasSubmittedIdentity(0u) &&
                   coordinator.HasSuccessfulGraphicsSubmission(0u),
               "corrupt optional present identity must preserve real graphics ownership");
    coordinator.FinalizeSubmittedFrame(observation);
    const auto completion = coordinator.CompleteFence(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(diagnostic));
    test.Check(completion.ownedGraphicsSubmission && !completion.completedEvidence &&
                   diagnostic.collectCount == 1u && diagnostic.publishCount == 0u,
               "corrupt presentation identity must drain once without stale publication");
}

void TestMidFrameGenerationPauseAndObserverFailure(TestContext& test)
{
    RtFrameEvidenceCoordinator coordinator;
    FakeGpu gpu;
    FakeDiagnostic unused;
    test.Check(coordinator.Initialise(Seeds(), 1u,
                                      RtInstrumentationMode::Shipping,
                                      RtSampleStatus::Disabled, false),
               "generation fixture must initialise");

    RtSceneRecordObservation oldFrame{};
    test.Check(coordinator.BeginFrame(0u, oldFrame) &&
                   oldFrame.stages->Accumulate(
                       RtStage::FrameFenceWait, 101u, 1u, 0u, 1u),
               "old-generation aggregate fixture must begin");
    SubmitPresentedFrame(test, coordinator, oldFrame,
                         RtInstrumentationMode::Shipping, gpu, false, false, 1u);
    (void)coordinator.CompleteFence(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(unused));

    RtSceneRecordObservation newFrame{};
    test.Check(coordinator.BeginFrame(0u, newFrame) &&
                   newFrame.stages->Accumulate(
                       RtStage::FrameFenceWait, 17u, 1u, 0u, 1u) &&
                   newFrame.stages->Accumulate(
                       RtStage::ImageAcquire, 23u, 1u, 0u, 1u),
               "mid-frame event must occur after fence/acquire timing began");
    const std::uint64_t oldGeneration =
        coordinator.PublishedStateByValue().measurementGeneration;
    test.Check(coordinator.ApplyEvent(RtLifecycleEvent::Retry) &&
                   coordinator.PublishedStateByValue().measurementGeneration ==
                       oldGeneration + 1u,
               "generation event must apply immediately while scratch remains active");
    SubmitPresentedFrame(test, coordinator, newFrame,
                         RtInstrumentationMode::Shipping, gpu, false, false, 2u);
    (void)coordinator.CompleteFence(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(unused));
    const RtPerformanceEvidenceSnapshot generationEvidence =
        coordinator.PublishedStateByValue().completedEvidence;
    const RtStageAggregateSet generationAggregates = coordinator.StageAggregatesByValue();
    test.Check(generationEvidence.identity.submitted.frame.measurementGeneration ==
                       oldGeneration + 1u &&
                   generationEvidence.scene.stages.values[
                       RtStageIndex(RtStage::FrameFenceWait)].durationNanoseconds == 17u &&
                   generationEvidence.scene.stages.values[
                       RtStageIndex(RtStage::ImageAcquire)].durationNanoseconds == 23u &&
                   generationAggregates.values[
                       RtStageIndex(RtStage::FrameFenceWait)].sampleCount == 1u &&
                   generationAggregates.values[
                       RtStageIndex(RtStage::FrameFenceWait)].sumNanoseconds == 17u,
               "new-generation identity must retain current intervals as aggregate sample one");

    RtSceneRecordObservation authoredFrozen{};
    test.Check(coordinator.BeginFrame(0u, authoredFrozen),
               "authored frozen checkpoint frame must begin without UI Pause");
    SubmitPresentedFrame(test, coordinator, authoredFrozen,
                         RtInstrumentationMode::Shipping, gpu, false, false, 3u);
    (void)coordinator.CompleteFence(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(unused));
    test.Check(coordinator.PublishedStateByValue().completedEvidence.benchmarkEligible,
               "authored frozen checkpoint alone must remain measurement-eligible");

    test.Check(coordinator.SetPaused(true),
               "owner UI pause must apply to measurement lifecycle");
    RtSceneRecordObservation paused{};
    test.Check(coordinator.BeginFrame(0u, paused),
               "paused graphics frame must still begin");
    SubmitPresentedFrame(test, coordinator, paused,
                         RtInstrumentationMode::Shipping, gpu, false, false, 4u);
    (void)coordinator.CompleteFence(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(unused));
    test.Check(!coordinator.PublishedStateByValue().completedEvidence.benchmarkEligible &&
                   coordinator.PublishedStateByValue().completedEvidence.presentation.outcome ==
                       RtPresentationOutcome::Presented,
               "UI-paused frame must present with identity but remain benchmark-ineligible");
    test.Check(coordinator.SetPaused(false),
               "owner UI resume must restore measurement generation");

    const RtStageAggregateSet beforeUnhealthy = coordinator.StageAggregatesByValue();
    RtSceneRecordObservation unhealthy{};
    test.Check(coordinator.BeginFrame(0u, unhealthy),
               "unhealthy observer graphics frame must begin");
    std::uint64_t reversedClock = 100u;
    unhealthy.clockUser = &reversedClock;
    unhealthy.readClock = [](void* user) noexcept {
        auto& clock = *static_cast<std::uint64_t*>(user);
        return clock--;
    };
    RtSceneStageScope reversedScope(&unhealthy, RtStage::ImageAcquire);
    reversedScope.Complete(1u, 0u, 1u);
    test.Check(!unhealthy.healthy,
               "reversed injected clock must mark the active observer unhealthy");
    SubmitPresentedFrame(test, coordinator, unhealthy,
                         RtInstrumentationMode::Shipping, gpu, false, false, 5u);
    (void)coordinator.CompleteFence(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(unused));
    const RtPerformanceEvidenceSnapshot unhealthyEvidence =
        coordinator.PublishedStateByValue().completedEvidence;
    const RtStageAggregateSet afterUnhealthy = coordinator.StageAggregatesByValue();
    test.Check(unhealthyEvidence.scene.stages.status == RtSampleStatus::Error &&
                   unhealthyEvidence.presentation.outcome == RtPresentationOutcome::Presented &&
                   !unhealthyEvidence.benchmarkEligible &&
                   afterUnhealthy.values[RtStageIndex(RtStage::FrameFenceWait)].sampleCount ==
                       beforeUnhealthy.values[RtStageIndex(RtStage::FrameFenceWait)].sampleCount,
               "observer failure must retain graphics identity but abort invalid aggregates");
}

void TestGpuToggleAndFinalIdle(TestContext& test)
{
    RtFrameEvidenceCoordinator coordinator;
    FakeGpu gpu;
    FakeDiagnostic unused;
    test.Check(coordinator.Initialise(Seeds(), 1u,
                                      RtInstrumentationMode::Shipping,
                                      RtSampleStatus::Pending, false),
               "GPU toggle fixture must initialise");
    RtSceneRecordObservation frame{};
    test.Check(coordinator.BeginFrame(0u, frame),
               "timed frame must begin");
    const RtEvidenceSubmitTransaction transaction = RecordAndPrevalidate(
        test, coordinator, frame, RtInstrumentationMode::Shipping, 7u);
    coordinator.CommitGraphicsSubmit(
        transaction, true, true, MakeFakeGpuIo(gpu));
    test.Check(gpu.markedSerial == transaction.identity.submissionSerial,
               "timer mark must use the lifecycle submission serial");
    test.Check(coordinator.AttachPresentation(RtPresentationOutcome::Presented),
               "timed frame presentation must attach");
    test.Check(coordinator.ApplyEvent(RtLifecycleEvent::GpuTimingDisabled),
               "GPU disable must change generation without stranding prior timer work");
    coordinator.FinalizeSubmittedFrame(frame);
    gpu.nextCollection.status = GpuFrameTimingCollectionStatus::Valid;
    gpu.nextCollection.consumed = true;
    gpu.nextCollection.hasSample = true;
    gpu.nextCollection.frameSlot = 0u;
    gpu.nextCollection.submissionSequence = transaction.identity.submissionSerial;
    gpu.nextCollection.sample = {
        0.25, 125'000u, transaction.identity.submissionSerial, 0u};
    const RtFrameEvidenceCompletionResult oldTimed = coordinator.CompleteFence(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(unused));
    const RtLifecyclePublishedState afterDisable = coordinator.PublishedStateByValue();
    test.Check(oldTimed.gpuCollectionAttempted && gpu.collectCount == 1u &&
                   afterDisable.completedEvidence.gpu.status == RtSampleStatus::Valid &&
                   afterDisable.gpuStatus == RtSampleStatus::Disabled &&
                   !afterDisable.completedEvidence.benchmarkEligible,
               "enabled submission must collect after disable without republishing old status");

    RtFrameEvidenceCoordinator disabledThenEnabled;
    FakeGpu noTimer;
    test.Check(disabledThenEnabled.Initialise(
                   Seeds(), 1u, RtInstrumentationMode::Shipping,
                   RtSampleStatus::Disabled, false),
               "disabled-to-enabled GPU fixture must initialise");
    RtSceneRecordObservation disabledFrame{};
    test.Check(disabledThenEnabled.BeginFrame(0u, disabledFrame),
               "disabled GPU frame must begin");
    const RtEvidenceSubmitTransaction disabledTransaction = RecordAndPrevalidate(
        test, disabledThenEnabled, disabledFrame,
        RtInstrumentationMode::Shipping, 71u);
    disabledThenEnabled.CommitGraphicsSubmit(
        disabledTransaction, false, false, MakeFakeGpuIo(noTimer));
    test.Check(disabledThenEnabled.AttachPresentation(
                   RtPresentationOutcome::Presented) &&
                   disabledThenEnabled.ApplyEvent(
                       RtLifecycleEvent::GpuTimingEnabled),
               "GPU enable must not retroactively add timing ownership to a disabled submission");
    disabledThenEnabled.FinalizeSubmittedFrame(disabledFrame);
    const RtFrameEvidenceCompletionResult oldDisabled =
        disabledThenEnabled.CompleteFence(
            0u, MakeFakeGpuIo(noTimer), MakeFakeDiagnosticIo(unused));
    const RtLifecyclePublishedState afterEnable =
        disabledThenEnabled.PublishedStateByValue();
    test.Check(oldDisabled.completedEvidence &&
                   !oldDisabled.gpuCollectionAttempted &&
                   noTimer.collectCount == 0u &&
                   afterEnable.completedEvidence.gpu.status ==
                       RtSampleStatus::Disabled &&
                   afterEnable.gpuStatus == RtSampleStatus::Pending &&
                   !afterEnable.completedEvidence.benchmarkEligible,
               "disabled submission must complete without timer IO after enable and must not replace the new generation status");

    RtFrameEvidenceCoordinator finalIdle;
    FakeDiagnostic diagnostic;
    test.Check(finalIdle.Initialise(Seeds(), 1u,
                                    RtInstrumentationMode::Diagnostic,
                                    RtSampleStatus::Disabled, false),
               "final-idle fixture must initialise");
    RtSceneRecordObservation pending{};
    test.Check(finalIdle.BeginFrame(0u, pending),
               "final-idle pending frame must begin");
    SubmitPresentedFrame(test, finalIdle, pending,
                         RtInstrumentationMode::Diagnostic, gpu,
                         false, false, 8u, true);
    const RtFrameEvidenceCompletionResult drained = finalIdle.CompleteFinalIdle(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(diagnostic));
    test.Check(drained.completedEvidence && diagnostic.collectCount == 1u &&
                   finalIdle.PublishedStateByValue().completedEvidence
                       .presentation.finalIdleCompletion,
               "successful final idle must drain and complete pending work once");

    RtFrameEvidenceCoordinator failedIdle;
    FakeDiagnostic noRead;
    test.Check(failedIdle.Initialise(Seeds(), 1u,
                                     RtInstrumentationMode::Diagnostic,
                                     RtSampleStatus::Disabled, false),
               "failed-idle fixture must initialise");
    RtSceneRecordObservation failedPending{};
    test.Check(failedIdle.BeginFrame(0u, failedPending),
               "failed-idle pending frame must begin");
    SubmitPresentedFrame(test, failedIdle, failedPending,
                         RtInstrumentationMode::Diagnostic, gpu,
                         false, false, 9u, true);
    const std::uint64_t failedIdleEpoch =
        failedIdle.PublishedStateByValue().sceneEpoch;
    failedIdle.NoteFailedDeviceIdle();
    test.Check(noRead.collectCount == 0u && gpu.collectCount == 1u &&
                   failedIdle.HasSuccessfulGraphicsSubmission(0u),
               "failed device idle must perform no new query or Diagnostic IO");
    test.Check(failedIdle.Recreate(RtResourceResetReason::SwapchainRecreate,
                                   RtSampleStatus::Disabled) &&
                   !failedIdle.HasSuccessfulGraphicsSubmission(0u) &&
                   failedIdle.PublishedStateByValue().sceneEpoch ==
                       failedIdleEpoch + 1u,
               "resource recreation must invalidate failed-idle pending ownership and advance its resource epoch");
}

void TestGpuNestedSampleIdentityMismatch(TestContext& test)
{
    RtFrameEvidenceCoordinator coordinator;
    FakeGpu gpu;
    FakeDiagnostic unused;
    test.Check(coordinator.Initialise(Seeds(), 1u,
                                      RtInstrumentationMode::Shipping,
                                      RtSampleStatus::Pending, false),
               "GPU nested-identity fixture must initialise");
    RtSceneRecordObservation frame{};
    test.Check(coordinator.BeginFrame(0u, frame),
               "GPU nested-identity frame must begin");
    const RtEvidenceSubmitTransaction transaction = RecordAndPrevalidate(
        test, coordinator, frame, RtInstrumentationMode::Shipping, 10u);
    coordinator.CommitGraphicsSubmit(
        transaction, true, true, MakeFakeGpuIo(gpu));
    test.Check(coordinator.AttachPresentation(RtPresentationOutcome::Presented),
               "GPU nested-identity frame must attach presentation");
    coordinator.FinalizeSubmittedFrame(frame);

    gpu.nextCollection.status = GpuFrameTimingCollectionStatus::Valid;
    gpu.nextCollection.consumed = true;
    gpu.nextCollection.hasSample = true;
    gpu.nextCollection.frameSlot = transaction.identity.frame.frameSlot;
    gpu.nextCollection.submissionSequence = transaction.identity.submissionSerial;
    gpu.nextCollection.sample = {
        0.25, 125'000u, transaction.identity.submissionSerial + 1u,
        transaction.identity.frame.frameSlot};
    const RtFrameEvidenceCompletionResult completed = coordinator.CompleteFence(
        0u, MakeFakeGpuIo(gpu), MakeFakeDiagnosticIo(unused));
    const RtGpuTimingEvidence evidence =
        coordinator.PublishedStateByValue().completedEvidence.gpu;
    test.Check(completed.completedEvidence &&
                   evidence.status == RtSampleStatus::Error &&
                   !evidence.hasDuration &&
                   evidence.completedSubmissionSerial ==
                       transaction.identity.submissionSerial,
               "a valid-duration payload with a mismatched nested identity must become an exact owning-serial Error");
}

void TestGpuUnavailableAndSetupErrorEvidence(TestContext& test)
{
    FakeDiagnostic unused;

    RtFrameEvidenceCoordinator unavailable;
    FakeGpu unavailableGpu;
    test.Check(unavailable.Initialise(Seeds(), 1u,
                                      RtInstrumentationMode::Shipping,
                                      RtSampleStatus::Pending, false),
               "GPU unavailable fixture must initialise");
    RtSceneRecordObservation unavailableFrame{};
    test.Check(unavailable.BeginFrame(0u, unavailableFrame),
               "GPU unavailable frame must begin");
    const RtEvidenceSubmitTransaction unavailableTransaction =
        RecordAndPrevalidate(test, unavailable, unavailableFrame,
                             RtInstrumentationMode::Shipping, 11u);
    unavailable.CommitGraphicsSubmit(
        unavailableTransaction, true, true, MakeFakeGpuIo(unavailableGpu));
    test.Check(unavailable.AttachPresentation(RtPresentationOutcome::Presented),
               "GPU unavailable frame must attach presentation");
    unavailable.FinalizeSubmittedFrame(unavailableFrame);
    unavailableGpu.nextCollection.status =
        GpuFrameTimingCollectionStatus::Unavailable;
    unavailableGpu.nextCollection.consumed = true;
    unavailableGpu.nextCollection.frameSlot =
        unavailableTransaction.identity.frame.frameSlot;
    unavailableGpu.nextCollection.submissionSequence =
        unavailableTransaction.identity.submissionSerial;
    const RtFrameEvidenceCompletionResult unavailableCompletion =
        unavailable.CompleteFence(
            0u, MakeFakeGpuIo(unavailableGpu), MakeFakeDiagnosticIo(unused));
    const RtGpuTimingEvidence pendingGpu =
        unavailable.PublishedStateByValue().completedEvidence.gpu;
    test.Check(unavailableCompletion.completedEvidence &&
                   unavailableCompletion.gpuCollectionAttempted &&
                   pendingGpu.status == RtSampleStatus::Pending &&
                   !pendingGpu.hasDuration &&
                   pendingGpu.durationNanoseconds == 0u &&
                   pendingGpu.completedSubmissionSerial == 0u &&
                   pendingGpu.unavailableResultCount == 1u,
               "a consumed unavailable timer pair must publish Pending/null once without a fabricated completion serial");

    RtFrameEvidenceCoordinator setupError;
    FakeGpu failedGpu;
    failedGpu.state.status = GpuFrameTimerStatus::InitialisationFailed;
    test.Check(InitialRtGpuEvidenceStatus(true, failedGpu.state) ==
                   RtSampleStatus::Error &&
                   setupError.Initialise(Seeds(), 1u,
                                         RtInstrumentationMode::Shipping,
                                         RtSampleStatus::Error, false),
               "optional GPU setup failure must initialise as Error without stopping graphics");
    RtSceneRecordObservation errorFrame{};
    test.Check(setupError.BeginFrame(0u, errorFrame),
               "GPU setup-error frame must begin");
    const RtEvidenceSubmitTransaction errorTransaction = RecordAndPrevalidate(
        test, setupError, errorFrame, RtInstrumentationMode::Shipping, 12u);
    setupError.CommitGraphicsSubmit(
        errorTransaction, true, false, MakeFakeGpuIo(failedGpu));
    test.Check(setupError.AttachPresentation(RtPresentationOutcome::Presented),
               "GPU setup-error frame must attach presentation");
    setupError.FinalizeSubmittedFrame(errorFrame);
    const RtFrameEvidenceCompletionResult errorCompletion =
        setupError.CompleteFence(
            0u, MakeFakeGpuIo(failedGpu), MakeFakeDiagnosticIo(unused));
    const RtGpuTimingEvidence errorGpu =
        setupError.PublishedStateByValue().completedEvidence.gpu;
    test.Check(errorCompletion.completedEvidence &&
                   !errorCompletion.gpuCollectionAttempted &&
                   failedGpu.collectCount == 0u &&
                   errorGpu.status == RtSampleStatus::Error &&
                   !errorGpu.hasDuration &&
                   errorGpu.completedSubmissionSerial ==
                       errorTransaction.identity.submissionSerial,
               "GPU setup Error must retain the exact graphics identity without issuing query IO or failing presentation");

    RtFrameEvidenceCoordinator markFailure;
    FakeGpu failedMarkGpu;
    failedMarkGpu.markSucceeds = false;
    test.Check(markFailure.Initialise(Seeds(), 1u,
                                      RtInstrumentationMode::Shipping,
                                      RtSampleStatus::Pending, false),
               "GPU mark-failure fixture must initialise");
    RtSceneRecordObservation markFrame{};
    test.Check(markFailure.BeginFrame(0u, markFrame),
               "GPU mark-failure frame must begin");
    const RtEvidenceSubmitTransaction markTransaction = RecordAndPrevalidate(
        test, markFailure, markFrame, RtInstrumentationMode::Shipping, 13u);
    markFailure.CommitGraphicsSubmit(
        markTransaction, true, true, MakeFakeGpuIo(failedMarkGpu));
    test.Check(markFailure.HasSuccessfulGraphicsSubmission(0u) &&
                   markFailure.AttachPresentation(RtPresentationOutcome::Presented),
               "timer mark failure must retain independent graphics ownership and presentation");
    markFailure.FinalizeSubmittedFrame(markFrame);
    const RtFrameEvidenceCompletionResult markCompletion =
        markFailure.CompleteFence(
            0u, MakeFakeGpuIo(failedMarkGpu), MakeFakeDiagnosticIo(unused));
    const RtGpuTimingEvidence markGpu =
        markFailure.PublishedStateByValue().completedEvidence.gpu;
    test.Check(markCompletion.completedEvidence &&
                   !markCompletion.gpuCollectionAttempted &&
                   failedMarkGpu.cancelCount == 1u &&
                   failedMarkGpu.collectCount == 0u &&
                   markGpu.status == RtSampleStatus::Error &&
                   markGpu.completedSubmissionSerial ==
                       markTransaction.identity.submissionSerial,
               "timer mark failure must become exact-serial GPU Error without converting graphics success to failure");

    RtFrameEvidenceCoordinator queryError;
    FakeGpu queryErrorGpu;
    test.Check(queryError.Initialise(Seeds(), 1u,
                                     RtInstrumentationMode::Shipping,
                                     RtSampleStatus::Pending, false),
               "GPU query-error fixture must initialise");
    RtSceneRecordObservation queryFrame{};
    test.Check(queryError.BeginFrame(0u, queryFrame),
               "GPU query-error frame must begin");
    const RtEvidenceSubmitTransaction queryTransaction = RecordAndPrevalidate(
        test, queryError, queryFrame, RtInstrumentationMode::Shipping, 14u);
    queryError.CommitGraphicsSubmit(
        queryTransaction, true, true, MakeFakeGpuIo(queryErrorGpu));
    test.Check(queryError.AttachPresentation(RtPresentationOutcome::Presented),
               "GPU query-error frame must attach presentation");
    queryError.FinalizeSubmittedFrame(queryFrame);
    queryErrorGpu.nextCollection.status = GpuFrameTimingCollectionStatus::Error;
    queryErrorGpu.nextCollection.consumed = true;
    queryErrorGpu.nextCollection.frameSlot =
        queryTransaction.identity.frame.frameSlot;
    queryErrorGpu.nextCollection.submissionSequence =
        queryTransaction.identity.submissionSerial;
    const RtFrameEvidenceCompletionResult queryCompletion =
        queryError.CompleteFence(
            0u, MakeFakeGpuIo(queryErrorGpu), MakeFakeDiagnosticIo(unused));
    const RtGpuTimingEvidence queryGpu =
        queryError.PublishedStateByValue().completedEvidence.gpu;
    test.Check(queryCompletion.completedEvidence &&
                   queryCompletion.gpuCollectionAttempted &&
                   queryErrorGpu.collectCount == 1u &&
                   queryGpu.status == RtSampleStatus::Error &&
                   queryGpu.completedSubmissionSerial ==
                       queryTransaction.identity.submissionSerial &&
                   queryGpu.errorCount == 1u,
               "consumed query failure must become one exact-serial GPU Error while preserving presentation");

    RtFrameEvidenceCoordinator unsupported;
    FakeGpu unsupportedGpu;
    unsupportedGpu.state.status = GpuFrameTimerStatus::UnsupportedQueue;
    test.Check(InitialRtGpuEvidenceStatus(true, unsupportedGpu.state) ==
                   RtSampleStatus::Unsupported &&
                   unsupported.Initialise(Seeds(), 1u,
                                          RtInstrumentationMode::Shipping,
                                          RtSampleStatus::Unsupported, false),
               "unsupported queue must remain a nonfatal explicit GPU status");
    RtSceneRecordObservation unsupportedFrame{};
    test.Check(unsupported.BeginFrame(0u, unsupportedFrame),
               "unsupported GPU frame must begin");
    const RtEvidenceSubmitTransaction unsupportedTransaction =
        RecordAndPrevalidate(test, unsupported, unsupportedFrame,
                             RtInstrumentationMode::Shipping, 15u);
    unsupported.CommitGraphicsSubmit(
        unsupportedTransaction, true, false, MakeFakeGpuIo(unsupportedGpu));
    test.Check(unsupported.AttachPresentation(RtPresentationOutcome::Presented),
               "unsupported GPU frame must attach presentation");
    unsupported.FinalizeSubmittedFrame(unsupportedFrame);
    const RtFrameEvidenceCompletionResult unsupportedCompletion =
        unsupported.CompleteFence(
            0u, MakeFakeGpuIo(unsupportedGpu), MakeFakeDiagnosticIo(unused));
    const RtPerformanceEvidenceSnapshot unsupportedEvidence =
        unsupported.PublishedStateByValue().completedEvidence;
    test.Check(unsupportedCompletion.completedEvidence &&
                   !unsupportedCompletion.gpuCollectionAttempted &&
                   unsupportedGpu.collectCount == 0u &&
                   unsupportedEvidence.gpu.status ==
                       RtSampleStatus::Unsupported &&
                   unsupportedEvidence.benchmarkEligible,
               "unsupported GPU timing must perform no query IO and must not disqualify an otherwise valid graphics frame");
}

} // namespace

int main()
{
    TestContext test;
    TestCommittedIdentityHandoff(test);
    TestCompletionOutputHandoff(test);
    TestSuboptimalPresentation(test);
    TestInitialFenceAndDiagnosticOwnership(test);
    TestTokenlessSubmissionAndFatalDiagnosticRead(test);
    TestDiagnosticReadBeforeNextResetOrdering(test);
    TestInitialAndBeginIdentityFailureStillDrain(test);
    TestShippingPoisonedIoAndQueueFailure(test);
    TestRecordEndAndPresentationIdentityFailure(test);
    TestMidFrameGenerationPauseAndObserverFailure(test);
    TestGpuToggleAndFinalIdle(test);
    TestGpuNestedSampleIdentityMismatch(test);
    TestGpuUnavailableAndSetupErrorEvidence(test);
    if (test.failures == 0)
    {
        std::cout << "RT frame evidence coordinator tests passed.\n";
    }
    return test.failures == 0 ? 0 : 1;
}
