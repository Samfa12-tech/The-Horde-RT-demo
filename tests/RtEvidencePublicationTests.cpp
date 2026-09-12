#include "telemetry/RtEvidencePublication.h"

#include <cstdint>
#include <iostream>
#include <locale>
#include <string>
#include <string_view>

namespace
{

using namespace horde::telemetry;

int failures = 0;

void Check(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

class GroupedNumberPunct final : public std::numpunct<char>
{
protected:
    [[nodiscard]] char do_thousands_sep() const override { return ','; }
    [[nodiscard]] std::string do_grouping() const override { return "\3"; }
};

class ScopedGlobalLocale final
{
public:
    explicit ScopedGlobalLocale(const std::locale& replacement)
        : previous_(std::locale())
    {
        std::locale::global(replacement);
    }

    ~ScopedGlobalLocale() { std::locale::global(previous_); }

    ScopedGlobalLocale(const ScopedGlobalLocale&) = delete;
    ScopedGlobalLocale& operator=(const ScopedGlobalLocale&) = delete;

private:
    std::locale previous_;
};

RtPipelineEvidenceIdentity MakePipelineIdentity(const RtInstrumentationMode instrumentation)
{
    RtPipelineEvidenceIdentity pipeline{};
    pipeline.executionMode = instrumentation == RtInstrumentationMode::Diagnostic
        ? RtExecutionMode::RayQueryCompute
        : RtExecutionMode::RayTracingPipeline;
    pipeline.instrumentation = instrumentation;
    pipeline.dielectricQuality = RtDielectricQuality::Mobile;
    pipeline.activeStrategy = RtMaterialStrategy::OpaqueFast;
    pipeline.waterQuality = RtWaterQuality::Mobile;
    (void)AssignRtFixedText(
        pipeline.bundleKey,
        instrumentation == RtInstrumentationMode::Diagnostic
            ? "diagnostic_mobile_pair"
            : "shipping_mobile_pair");
    (void)AssignRtFixedText(pipeline.opaqueFast.key, "opaque_fast");
    (void)AssignRtFixedText(pipeline.genericDielectric.key, "generic_dielectric");
    (void)AssignRtFixedText(pipeline.opaqueFast.sha256, std::string(64u, 'a'));
    (void)AssignRtFixedText(pipeline.genericDielectric.sha256, std::string(64u, 'b'));
    pipeline.active = pipeline.opaqueFast;
    return pipeline;
}

RtStageFrameSample MakeStages()
{
    RtStageFrameSample stages{};
    stages.status = RtSampleStatus::Valid;
    stages.values[RtStageIndex(RtStage::SimulationStep)].durationNanoseconds = 1'000'000u;
    stages.values[RtStageIndex(RtStage::PlayerSkin)].durationNanoseconds = 2'000u;
    stages.values[RtStageIndex(RtStage::CharacterSkin)].durationNanoseconds = 3'000u;
    stages.values[RtStageIndex(RtStage::Skin)].durationNanoseconds = 5'000u;
    stages.values[RtStageIndex(RtStage::TraceCopyRecord)].durationNanoseconds = 750'000u;
    stages.values[RtStageIndex(RtStage::WholeFrameCycle)].durationNanoseconds = 16'000'000u;
    return stages;
}

RtSceneFrameEvidence MakeScene(const RtInstrumentationMode instrumentation)
{
    RtSceneFrameEvidence scene{};
    scene.pipeline = MakePipelineIdentity(instrumentation);
    scene.resources.bufferCount = 12u;
    scene.resources.memoryAllocationCount = 4u;
    scene.resources.bottomLevelAccelerationStructureCount = 9u;
    scene.resources.topLevelAccelerationStructureCount = 1u;
    scene.resources.tlasInstanceCount = 20u;
    scene.resources.pipelineCount = 2u;
    scene.resources.shaderBindingTableCount =
        instrumentation == RtInstrumentationMode::Diagnostic ? 0u : 2u;
    scene.resources.descriptorSetCount = 1u;
    scene.resources.hostVisibleBytes = 4096u;
    scene.resources.deviceLocalBytes = 8192u;
    scene.player.skinCadenceHz = 60u;
    scene.player.skinUpdateCount = 8u;
    scene.player.maximumSocketErrorMicrometres = 11u;
    scene.stages = MakeStages();
    scene.dispatch = {true, true, true};
    return scene;
}

RtDiagnosticEvidence MakeDiagnostic(const RtInstrumentationMode instrumentation,
                                    const std::uint64_t submissionSerial)
{
    RtDiagnosticEvidence diagnostic{};
    if (instrumentation == RtInstrumentationMode::Shipping)
    {
        diagnostic.status = RtSampleStatus::CompiledOut;
        return diagnostic;
    }
    diagnostic.status = RtSampleStatus::Valid;
    diagnostic.compiled = true;
    diagnostic.hasCounters = true;
    diagnostic.completedSubmissionSerial = submissionSerial;
    diagnostic.readCount = 1u;
    diagnostic.resetCount = 1u;
    for (std::size_t index = 0u; index < diagnostic.counters.size(); ++index)
    {
        diagnostic.counters[index] = static_cast<std::uint32_t>(index + 1u);
    }
    return diagnostic;
}

RtGpuTimingEvidence MakeGpu(const RtInstrumentationMode instrumentation,
                            const std::uint64_t submissionSerial)
{
    RtGpuTimingEvidence gpu{};
    if (instrumentation == RtInstrumentationMode::Shipping)
    {
        gpu.status = RtSampleStatus::Disabled;
        return gpu;
    }
    gpu.status = RtSampleStatus::Valid;
    gpu.hasDuration = true;
    gpu.durationNanoseconds = 2'500'000u;
    gpu.completedSubmissionSerial = submissionSerial;
    gpu.timestampPeriodPicoseconds = 1000u;
    gpu.sampleCount = 1u;
    gpu.timestampValidBits = 64u;
    return gpu;
}

RtPerformanceEvidenceSnapshot MakeCompletedEvidence(
    const RtInstrumentationMode instrumentation,
    const std::uint64_t sceneEpoch = 11u,
    const std::uint64_t measurementGeneration = 20u)
{
    RtPerformanceEvidenceSnapshot snapshot{};
    snapshot.identity.submitted.frame.sceneEpoch = sceneEpoch;
    snapshot.identity.submitted.frame.measurementGeneration = measurementGeneration;
    snapshot.identity.submitted.frame.recordAttemptSerial = 1u;
    snapshot.identity.submitted.frame.recordSerial = 1u;
    snapshot.identity.submitted.frame.simulationTick = 101u;
    snapshot.identity.submitted.frame.frameSlot = 0u;
    snapshot.identity.submitted.submissionSerial = 1u;
    snapshot.identity.completionSerial = 1u;
    snapshot.scene = MakeScene(instrumentation);
    snapshot.dielectric = MakeDiagnostic(instrumentation, 1u);
    if (snapshot.dielectric.status == RtSampleStatus::Valid)
    {
        snapshot.scene.player.primaryPixelCountAvailable = true;
        snapshot.scene.player.primaryPixelCount =
            snapshot.dielectric.counters[kRtPrimaryPlayerPixelCounterIndex];
        snapshot.scene.player.primaryVisible = true;
    }
    snapshot.gpu = MakeGpu(instrumentation, 1u);
    snapshot.presentation.outcome = RtPresentationOutcome::Presented;
    snapshot.presentation.lastSuccessfulPresentSubmissionSerial = 1u;
    snapshot.cpuBenchmarkEligible = true;
    snapshot.benchmarkEligible = true;
    return snapshot;
}

RtLifecyclePublishedState MakePublished(const RtPerformanceEvidenceSnapshot& completed)
{
    RtLifecyclePublishedState state{};
    state.sceneEpoch = completed.identity.submitted.frame.sceneEpoch;
    state.measurementGeneration =
        completed.identity.submitted.frame.measurementGeneration;
    state.diagnosticStatus = completed.dielectric.status;
    state.gpuStatus = completed.gpu.status;
    state.running = true;
    state.presented = true;
    state.hasCompletedEvidence = true;
    state.completedEvidence = completed;
    return state;
}

RtRecordedSceneEvidence MakeRecordedScene(const RtSceneFrameEvidence& scene)
{
    RtRecordedSceneEvidence recorded{};
    recorded.pipeline = scene.pipeline;
    recorded.resources = scene.resources;
    recorded.player = scene.player;
    recorded.player.primaryPixelCountAvailable = false;
    recorded.player.primaryPixelCount = 0u;
    recorded.player.primaryVisible = false;
    recorded.dispatch = scene.dispatch;
    return recorded;
}

void TestPendingAndRecreatedPublication()
{
    RtEvidenceLifecycle lifecycle;
    RtLifecycleSeeds seeds{};
    seeds.sceneEpoch = 10u;
    seeds.measurementGeneration = 20u;
    RtLifecycleResetEffects effects{};
    Check(lifecycle.Initialise(
              seeds, 1u, RtSampleStatus::Pending, RtSampleStatus::Disabled, effects),
          "real lifecycle must initialise a first pending publication");

    std::string json;
    std::string text;
    std::string reason = "stale";
    Check(SerializeRtEvidencePublication(
              lifecycle.PublishedStateByValue(), true, json, text, reason),
          "first pending publication must serialize");
    const std::string expectedJson =
        "{\"version\":1,\"observerAvailable\":true,\"sceneEpoch\":11,"
        "\"measurementGeneration\":20,\"running\":true,\"paused\":false,"
        "\"presented\":false,\"diagnosticStatus\":\"pending\","
        "\"gpuStatus\":\"disabled\",\"completedFrameStatus\":\"pending\","
        "\"completedFrameReason\":\"no-accepted-completed-frame\","
        "\"completedFrame\":null}\n";
    const std::string expectedText =
        "RT EVIDENCE PUBLICATION version=1\n"
        "Observer: available\n"
        "Lifecycle: epoch=11 generation=20 running=yes paused=no presented=no\n"
        "Diagnostic status: pending\n"
        "GPU status: disabled\n"
        "Completed frame: N/A (no-accepted-completed-frame)\n";
    Check(json == expectedJson && text == expectedText && reason.empty(),
          "pending projection must be canonical and contain no invented frame values");

    Check(lifecycle.Recreate(RtResourceResetReason::SwapchainRecreate,
                             RtSampleStatus::Pending,
                             RtSampleStatus::Disabled,
                             effects),
          "real lifecycle must recreate into a new scene epoch");
    Check(SerializeRtEvidencePublication(
              lifecycle.PublishedStateByValue(), true, json, text, reason) &&
              json.find("\"sceneEpoch\":12") != std::string::npos &&
              json.find("\"completedFrame\":null") != std::string::npos,
          "recreated publication must not expose evidence from the retired epoch");
}

void TestObserverUnavailable()
{
    RtLifecyclePublishedState ignored{};
    ignored.diagnosticStatus = static_cast<RtSampleStatus>(255u);
    ignored.gpuStatus = static_cast<RtSampleStatus>(254u);
    ignored.hasCompletedEvidence = true;
    ignored.completedEvidence = MakeCompletedEvidence(RtInstrumentationMode::Diagnostic);
    std::string json;
    std::string text;
    std::string reason = "stale";
    Check(SerializeRtEvidencePublication(ignored, false, json, text, reason),
          "missing optional observer must be an honest non-error projection");
    Check(json ==
              "{\"version\":1,\"observerAvailable\":false,\"sceneEpoch\":null,"
              "\"measurementGeneration\":null,\"running\":null,\"paused\":null,"
              "\"presented\":null,\"diagnosticStatus\":\"unavailable\","
              "\"gpuStatus\":\"unavailable\",\"completedFrameStatus\":\"unavailable\","
              "\"completedFrameReason\":\"observer-unavailable\","
              "\"completedFrame\":null}\n" &&
              text ==
                  "RT EVIDENCE PUBLICATION version=1\n"
                  "Observer: unavailable\n"
                  "Lifecycle: N/A\n"
                  "Diagnostic status: unavailable\n"
                  "GPU status: unavailable\n"
                  "Completed frame: N/A (observer-unavailable)\n" &&
              reason.empty(),
          "missing observer must use unavailable/null rather than default numeric zero");
}

void TestCanonicalCompletedEvidence()
{
    for (const RtInstrumentationMode instrumentation : {
             RtInstrumentationMode::Shipping,
             RtInstrumentationMode::Diagnostic})
    {
        const RtPerformanceEvidenceSnapshot completed =
            MakeCompletedEvidence(instrumentation);
        RtEvidenceValidationError canonicalError = RtEvidenceValidationError::None;
        std::string canonicalJson;
        std::string canonicalText;
        Check(SerializeRtPerformanceEvidenceJson(
                  completed, canonicalJson, canonicalError) &&
                  SerializeRtPerformanceEvidenceText(
                      completed, canonicalText, canonicalError),
              "completed fixture must satisfy canonical evidence validation");

        std::string publicationJson;
        std::string publicationText;
        std::string reason;
        Check(SerializeRtEvidencePublication(
                  MakePublished(completed), true,
                  publicationJson, publicationText, reason),
              "valid completed evidence must project");
        Check(publicationJson.find(
                  "\"completedFrameStatus\":\"available\","
                  "\"completedFrameReason\":null,\"completedFrame\":" +
                  canonicalJson) != std::string::npos,
              "completed JSON subobject must be the byte-exact canonical serialization");
        Check(publicationText.find("Completed frame: available\n" + canonicalText) !=
                  std::string::npos && reason.empty(),
              "completed text body must be the byte-exact canonical serialization");
    }
}

void TestCurrentLifecycleDiffersFromHistoricalFrame()
{
    const RtPerformanceEvidenceSnapshot completed =
        MakeCompletedEvidence(RtInstrumentationMode::Shipping);
    RtLifecyclePublishedState state = MakePublished(completed);
    state.paused = true;
    state.presented = false;
    std::string json;
    std::string text;
    std::string reason;
    Check(SerializeRtEvidencePublication(state, true, json, text, reason),
          "a paused lifecycle may retain a previously presented completed frame");
    Check(json.find("\"running\":true,\"paused\":true,\"presented\":false") !=
              std::string::npos &&
              json.find("\"presentation\":{\"outcome\":\"presented\","
                        "\"presented\":true") != std::string::npos,
          "current lifecycle flags must stay distinct from historical frame presentation");
}

void TestPreviousGenerationIsPendingNotError()
{
    RtEvidenceLifecycle lifecycle;
    RtLifecycleSeeds seeds{};
    seeds.sceneEpoch = 10u;
    seeds.measurementGeneration = 20u;
    RtLifecycleResetEffects effects{};
    Check(lifecycle.Initialise(
              seeds, 1u, RtSampleStatus::CompiledOut, RtSampleStatus::Disabled, effects),
          "shipping lifecycle must initialise");
    const RtSceneFrameEvidence scene = MakeScene(RtInstrumentationMode::Shipping);
    RtFrameToken attempt{};
    RtFrameToken recorded{};
    RtSubmittedFrameIdentity submitted{};
    Check(lifecycle.BeginRecord(0u, 101u, attempt) &&
              lifecycle.FinishRecord(attempt, MakeRecordedScene(scene), recorded) &&
              lifecycle.Submit(recorded, submitted) &&
              lifecycle.AttachPresentation(submitted, RtPresentationOutcome::Presented),
          "real lifecycle must submit a presented frame");
    RtSubmittedStageSample stages{};
    stages.identity = submitted;
    stages.stages = scene.stages;
    RtPerformanceEvidenceSnapshot completed{};
    Check(lifecycle.CompleteFence(
              submitted,
              stages,
              MakeDiagnostic(RtInstrumentationMode::Shipping, submitted.submissionSerial),
              MakeGpu(RtInstrumentationMode::Shipping, submitted.submissionSerial),
              completed),
          "real lifecycle must accept the completed frame");
    Check(lifecycle.ApplyEvent(RtLifecycleEvent::WarmupToMeasure, effects),
          "warmup transition must advance the current generation");
    const RtLifecyclePublishedState current = lifecycle.PublishedStateByValue();
    Check(current.hasCompletedEvidence &&
              current.completedEvidence.identity.submitted.frame.measurementGeneration <
                  current.measurementGeneration,
          "real transition must retain historical completed evidence");

    std::string json;
    std::string text;
    std::string reason;
    Check(SerializeRtEvidencePublication(current, true, json, text, reason),
          "previous-generation completion must be a legitimate pending projection");
    Check(json.find("\"measurementGeneration\":21") != std::string::npos &&
              json.find("\"completedFrameStatus\":\"pending\","
                        "\"completedFrameReason\":\"previous-measurement-generation\","
                        "\"completedFrame\":null") != std::string::npos &&
              text.find("Completed frame: N/A (previous-measurement-generation)") !=
                  std::string::npos &&
              reason.empty(),
          "old-generation metrics must not be promoted into the current generation");
}

void ExpectInvalid(RtLifecyclePublishedState state,
                   const std::string_view expectedReason,
                   const std::string_view label)
{
    std::string json = "stale-json";
    std::string text = "stale-text";
    std::string reason;
    const bool serialized = SerializeRtEvidencePublication(
        state, true, json, text, reason);
    Check(!serialized && reason.find(expectedReason) != std::string::npos,
          std::string(label) + " must return a validation reason");
    Check(json.find("\"diagnosticStatus\":\"error\",\"gpuStatus\":\"error\"") !=
              std::string::npos &&
              json.find("\"completedFrameStatus\":\"error\"") != std::string::npos &&
              json.find("\"completedFrame\":null") != std::string::npos &&
              text.find("Completed frame: N/A (invalid-publication)") !=
                  std::string::npos,
          std::string(label) + " must emit Error/null without stale frame passthrough");
}

void TestInvalidPublications()
{
    RtLifecyclePublishedState state =
        MakePublished(MakeCompletedEvidence(RtInstrumentationMode::Shipping));
    RtLifecyclePublishedState invalid = state;
    invalid.sceneEpoch = 0u;
    ExpectInvalid(invalid, "scene epoch", "zero scene epoch");

    invalid = state;
    invalid.measurementGeneration = 0u;
    ExpectInvalid(invalid, "measurement generation", "zero measurement generation");

    invalid = state;
    invalid.diagnosticStatus = static_cast<RtSampleStatus>(255u);
    ExpectInvalid(invalid, "diagnostic status", "unknown diagnostic status");

    invalid = state;
    invalid.gpuStatus = static_cast<RtSampleStatus>(255u);
    ExpectInvalid(invalid, "GPU status", "unknown GPU status");

    invalid = state;
    ++invalid.completedEvidence.identity.submitted.frame.sceneEpoch;
    ExpectInvalid(invalid, "scene epoch", "different-epoch completion");

    invalid = state;
    ++invalid.completedEvidence.identity.submitted.frame.measurementGeneration;
    ExpectInvalid(invalid, "future measurement generation", "future-generation completion");

    invalid = state;
    invalid.completedEvidence.schema = kRtPerformanceEvidenceSchema + 1u;
    ExpectInvalid(invalid, "canonical", "canonically malformed completion");

    invalid = state;
    ++invalid.measurementGeneration;
    invalid.completedEvidence.schema = kRtPerformanceEvidenceSchema + 1u;
    ExpectInvalid(invalid, "canonical", "malformed previous-generation completion");

    invalid = state;
    ++invalid.measurementGeneration;
    invalid.completedEvidence.identity.submitted.frame.measurementGeneration = 0u;
    ExpectInvalid(invalid, "canonical", "zero-generation historical completion");
}

void TestClassicLocaleProjection()
{
    RtLifecyclePublishedState state{};
    state.sceneEpoch = 1'234'567u;
    state.measurementGeneration = 7'654'321u;
    state.diagnosticStatus = RtSampleStatus::Pending;
    state.gpuStatus = RtSampleStatus::Unsupported;
    state.running = true;
    std::string json;
    std::string text;
    std::string reason;
    bool serialized = false;
    {
        const ScopedGlobalLocale grouped(
            std::locale(std::locale::classic(), new GroupedNumberPunct));
        serialized = SerializeRtEvidencePublication(
            state, true, json, text, reason);
    }
    Check(serialized &&
              json.find("\"sceneEpoch\":1234567,\"measurementGeneration\":7654321") !=
                  std::string::npos &&
              json.find("1,234,567") == std::string::npos &&
              text.find("epoch=1234567 generation=7654321") != std::string::npos,
          "publication JSON/text numbers must ignore the process-global locale");
}

} // namespace

int main()
{
    TestPendingAndRecreatedPublication();
    TestObserverUnavailable();
    TestCanonicalCompletedEvidence();
    TestCurrentLifecycleDiffersFromHistoricalFrame();
    TestPreviousGenerationIsPendingNotError();
    TestInvalidPublications();
    TestClassicLocaleProjection();
    if (failures == 0)
    {
        std::cout << "RT evidence publication tests passed.\n";
    }
    return failures == 0 ? 0 : 1;
}
