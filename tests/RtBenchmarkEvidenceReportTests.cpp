#include "telemetry/RtBenchmarkEvidenceReport.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <locale>
#include <optional>
#include <string>
#include <string_view>

#include "gameplay/ShowcaseRoute.h"
#include "gameplay/ShowcaseBenchmark.h"
#include "telemetry/RtBenchmarkEvidenceRun.h"
#include "telemetry/RtPerformanceEvidence.h"

namespace
{

using namespace horde::gameplay;
using namespace horde::telemetry;

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

template <std::size_t Capacity>
void SetText(RtFixedText<Capacity>& output, const std::string_view value)
{
    if (!AssignRtFixedText(output, value))
    {
        std::abort();
    }
}

RtPipelineEvidenceIdentity MakePipelineIdentity()
{
    RtPipelineEvidenceIdentity pipeline{};
    pipeline.executionMode = RtExecutionMode::RayTracingPipeline;
    pipeline.instrumentation = RtInstrumentationMode::Shipping;
    pipeline.dielectricQuality = RtDielectricQuality::Mobile;
    pipeline.activeStrategy = RtMaterialStrategy::OpaqueFast;
    pipeline.waterQuality = RtWaterQuality::Mobile;
    SetText(pipeline.bundleKey, "shipping_mobile_pair");
    SetText(pipeline.opaqueFast.key, "shipping_mobile_opaque");
    SetText(pipeline.opaqueFast.sha256,
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    SetText(pipeline.genericDielectric.key, "shipping_mobile_generic");
    SetText(pipeline.genericDielectric.sha256,
            "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    pipeline.active = pipeline.opaqueFast;
    return pipeline;
}

RtStageFrameSample MakeStages(const std::uint64_t baseNanoseconds)
{
    RtStageFrameSample stages{};
    stages.status = RtSampleStatus::Valid;
    for (std::size_t index = 0u; index < stages.values.size(); ++index)
    {
        stages.values[index].durationNanoseconds =
            baseNanoseconds + static_cast<std::uint64_t>(index) * 100'000u;
        stages.values[index].operationCount = 1u;
    }
    const RtStageValue& player = stages.values[RtStageIndex(RtStage::PlayerSkin)];
    const RtStageValue& character = stages.values[RtStageIndex(RtStage::CharacterSkin)];
    RtStageValue& skin = stages.values[RtStageIndex(RtStage::Skin)];
    skin.durationNanoseconds = player.durationNanoseconds + character.durationNanoseconds;
    skin.operationCount = player.operationCount + character.operationCount;
    return stages;
}

RtPerformanceEvidenceSnapshot MakeSnapshot(
    const std::uint64_t serial,
    const std::uint64_t epoch,
    const std::uint64_t generation,
    const std::uint32_t slot,
    const std::uint64_t cpuBaseNanoseconds,
    const RtSampleStatus gpuStatus,
    const std::uint64_t gpuNanoseconds,
    const bool cpuEligible = true,
    const RtPresentationOutcome presentation = RtPresentationOutcome::Presented)
{
    RtPerformanceEvidenceSnapshot snapshot{};
    snapshot.identity.submitted.frame.sceneEpoch = epoch;
    snapshot.identity.submitted.frame.measurementGeneration = generation;
    snapshot.identity.submitted.frame.recordAttemptSerial = 100u + serial;
    snapshot.identity.submitted.frame.recordSerial = 200u + serial;
    snapshot.identity.submitted.frame.simulationTick = 300u + serial;
    snapshot.identity.submitted.frame.frameSlot = slot;
    snapshot.identity.submitted.submissionSerial = 400u + serial;
    snapshot.identity.completionSerial = 500u + serial;
    snapshot.scene.pipeline = MakePipelineIdentity();
    snapshot.scene.stages = MakeStages(cpuBaseNanoseconds);
    snapshot.scene.dispatch.sceneReady = true;
    snapshot.scene.dispatch.rtDispatchRecorded = true;
    snapshot.scene.dispatch.swapchainCopyRecorded = true;
    snapshot.dielectric.status = RtSampleStatus::CompiledOut;
    snapshot.gpu.status = gpuStatus;
    if (gpuStatus == RtSampleStatus::Valid)
    {
        snapshot.gpu.hasDuration = true;
        snapshot.gpu.durationNanoseconds = gpuNanoseconds;
        snapshot.gpu.completedSubmissionSerial = snapshot.identity.submitted.submissionSerial;
        snapshot.gpu.timestampValidBits = 64u;
        snapshot.gpu.timestampPeriodPicoseconds = 1'000u;
        snapshot.gpu.sampleCount = 1u;
    }
    else if (gpuStatus == RtSampleStatus::Error)
    {
        snapshot.gpu.completedSubmissionSerial = snapshot.identity.submitted.submissionSerial;
        snapshot.gpu.errorCount = 1u;
        SetText(snapshot.gpu.detail, "query-read-error");
    }
    snapshot.presentation.outcome = presentation;
    if (RtPresentationSucceeded(presentation))
    {
        snapshot.presentation.lastSuccessfulPresentSubmissionSerial =
            snapshot.identity.submitted.submissionSerial;
    }
    snapshot.cpuBenchmarkEligible = cpuEligible;
    snapshot.benchmarkEligible = cpuEligible &&
        (gpuStatus == RtSampleStatus::Valid || gpuStatus == RtSampleStatus::Disabled ||
         gpuStatus == RtSampleStatus::Unsupported);
    return snapshot;
}

std::optional<std::size_t> ExpectAndBind(TestContext& context,
                                         RtBenchmarkEvidenceRun& run,
                                         const RtBenchmarkFrameTag tag,
                                         const RtPerformanceEvidenceSnapshot& snapshot)
{
    const std::optional<std::size_t> index = run.ExpectFrame(tag);
    context.Check(index.has_value(), "fixture frame must enter the intended ledger");
    if (index.has_value())
    {
        context.Check(run.BindSubmitted(*index, snapshot.identity.submitted),
                      "fixture frame must retain its actual committed identity");
    }
    return index;
}

std::string ObjectForKey(const std::string& json, const std::string_view key)
{
    const std::string marker = "\"" + std::string(key) + "\":";
    const std::size_t keyPosition = json.find(marker);
    if (keyPosition == std::string::npos)
    {
        return {};
    }
    const std::size_t objectStart = json.find('{', keyPosition + marker.size());
    if (objectStart == std::string::npos)
    {
        return {};
    }
    std::size_t depth = 0u;
    for (std::size_t index = objectStart; index < json.size(); ++index)
    {
        if (json[index] == '{')
        {
            ++depth;
        }
        else if (json[index] == '}' && --depth == 0u)
        {
            return json.substr(objectStart, index - objectStart + 1u);
        }
    }
    return {};
}

std::string ArrayObjectWith(const std::string& json, const std::string_view needle)
{
    const std::size_t needlePosition = json.find(needle);
    if (needlePosition == std::string::npos)
    {
        return {};
    }
    const std::size_t objectStart = json.rfind('{', needlePosition);
    if (objectStart == std::string::npos)
    {
        return {};
    }
    std::size_t depth = 0u;
    for (std::size_t index = objectStart; index < json.size(); ++index)
    {
        if (json[index] == '{')
        {
            ++depth;
        }
        else if (json[index] == '}' && --depth == 0u)
        {
            return json.substr(objectStart, index - objectStart + 1u);
        }
    }
    return {};
}

class CommaDecimalFacet final : public std::numpunct<char>
{
protected:
    char do_decimal_point() const override { return ','; }
    char do_thousands_sep() const override { return '.'; }
    std::string do_grouping() const override { return "\3"; }
};

void TestCompleteRunReportsStableStagesZonesAndActualIdentities(TestContext& context)
{
    RtBenchmarkEvidenceRun run;
    context.Check(run.Start(2u) && run.ArmMeasurement(71u, 81u),
                  "complete report fixture must allocate and arm");
    const RtPerformanceEvidenceSnapshot first = MakeSnapshot(
        1u, 71u, 81u, 0u, 1'000'000u, RtSampleStatus::Valid, 4'000'000u);
    const RtPerformanceEvidenceSnapshot second = MakeSnapshot(
        2u, 71u, 81u, 1u, 2'000'000u, RtSampleStatus::Valid, 6'000'000u);
    ExpectAndBind(context, run, {static_cast<std::uint32_t>(ShowcaseZone::Opening), 2u}, first);
    context.Check(run.Complete(first), "first complete report row must resolve");
    ExpectAndBind(context, run, {99u, 3u}, second);
    context.Check(run.Complete(second) && run.RecordOwnerDrainResult(true) && run.Finalize(),
                  "fully accounted report fixture must finalize complete");

    const std::string json = BuildRtBenchmarkEvidenceJson(run);
    context.Check(json.find("\"schema\": 1") != std::string::npos &&
                      json.find("\"status\": \"complete\"") != std::string::npos &&
                      json.find("\"sceneEpoch\": 71") != std::string::npos &&
                      json.find("\"measurementGeneration\": 81") != std::string::npos &&
                      json.find("\"capacity\": 2") != std::string::npos,
                  "JSON must identify schema, finalized state, generation and bounded capacity");
    context.Check(json.find("\"expected\": 2") != std::string::npos &&
                      json.find("\"completed\": 2") != std::string::npos &&
                      json.find("\"rejected\": 0") != std::string::npos &&
                      json.find("\"cancelled\": 0") != std::string::npos &&
                      json.find("\"cpuAccepted\": 2") != std::string::npos &&
                      json.find("\"cpuRejected\": 0") != std::string::npos &&
                      json.find("\"outstanding\": 0") != std::string::npos,
                  "JSON counts must expose the intended denominator and independent CPU admission");
    const std::string gpuCounts = ObjectForKey(json, "gpuStatusCounts");
    context.Check(gpuCounts.find("\"denominator\": 2") != std::string::npos &&
                      gpuCounts.find("\"valid\": 2") != std::string::npos &&
                      gpuCounts.find("\"missing\": 0") != std::string::npos,
                  "GPU status counts must state their intended-frame denominator");

    const std::string simulation = ObjectForKey(json, "simulationStepCpuMs");
    const std::string wholeFrame = ObjectForKey(json, "wholeFrameCycleCpuMs");
    const std::string gpu = ObjectForKey(json, "gpuRtDurationMs");
    context.Check(!simulation.empty() && simulation.find("\"sampleCount\": 2") != std::string::npos &&
                      simulation.find("\"meanMilliseconds\": 1.5000") != std::string::npos &&
                      simulation.find("onePercentLowFps") == std::string::npos,
                  "every stable CPU stage must report shared statistics without a substage FPS proxy");
    context.Check(!wholeFrame.empty() &&
                      wholeFrame.find("\"slowestOnePercentMeanMilliseconds\": 3.2000") !=
                          std::string::npos &&
                      wholeFrame.find("\"onePercentLowFps\": 312.5000") != std::string::npos,
                  "whole-frame stage must expose raw slowest-one-percent mean and its allowed FPS proxy");
    context.Check(!gpu.empty() && gpu.find("\"meanMilliseconds\": 5.0000") != std::string::npos &&
                      gpu.find("onePercentLowFps") == std::string::npos,
                  "GPU RT duration must use shared statistics without claiming frame rate");

    context.Check(json.find("\"zone\": 1, \"name\": \"opening\"") != std::string::npos &&
                      json.find("\"zone\": 99, \"name\": \"unknown-99\"") != std::string::npos,
                  "known zones must use Showcase names while retained unknown numeric zones stay visible");
    const std::string unknownZone = ArrayObjectWith(json, "\"name\": \"unknown-99\"");
    context.Check(unknownZone.find("\"intended\": 1") != std::string::npos &&
                      unknownZone.find("\"cpuAccepted\": 1") != std::string::npos,
                  "per-zone entries must expose intended and independently accepted counts");

    context.Check(json.find("\"lap\": 2") != std::string::npos &&
                      json.find("\"lap\": 3") != std::string::npos &&
                      json.find("\"disposition\": \"completed\"") != std::string::npos &&
                      json.find("\"failure\": \"none\"") != std::string::npos &&
                      json.find("\"recordAttemptSerial\": 101") != std::string::npos &&
                      json.find("\"recordSerial\": 201") != std::string::npos &&
                      json.find("\"simulationTick\": 301") != std::string::npos &&
                      json.find("\"submissionSerial\": 401") != std::string::npos &&
                      json.find("\"completionSerial\": 501") != std::string::npos &&
                      json.find("\"presentationOutcome\": \"presented\"") != std::string::npos &&
                      json.find("\"cpuStageStatus\": \"valid\"") != std::string::npos &&
                      json.find("\"diagnosticStatus\": \"compiled-out\"") != std::string::npos &&
                      json.find("\"gpuDurationNanoseconds\": 4000000") != std::string::npos,
                  "ledger rows must emit saved tags, canonical statuses and actual retained identities");

    const std::string text = BuildRtBenchmarkEvidenceText(run);
    context.Check(text.find("Status: complete") != std::string::npos &&
                      text.find("Rows: expected 2, completed 2") != std::string::npos &&
                      text.find("unknown-99 (99)") != std::string::npos &&
                      text.find("recordAttemptSerial") == std::string::npos,
                  "text report must stay concise and omit the per-row ledger");
    horde::gameplay::ShowcaseBenchmarkRun route;
    route.Start();
    while (route.IsRunning())
    {
        route.Advance();
        route.RecordFrame(12.5, true);
    }
    context.Check(route.Passed() && route.Frames().size() == 1838u,
                  "cross-report fixture must finish the actual intended route");
    const horde::gameplay::ShowcaseBenchmarkMetadata metadata{};
    context.Check(route.BuildTextReport(metadata, &run).find("Integrity: INVALID") != std::string::npos &&
                      route.BuildJsonReport(metadata, &run).find("\"result\": \"invalid\"") != std::string::npos,
                  "a complete two-frame evidence subset must not certify the 1838-frame player route");
}

void TestAllocatedCancelledAndMissingCompletionAreExplicit(TestContext& context)
{
    RtBenchmarkEvidenceRun allocated;
    context.Check(allocated.Start(4u), "allocated fixture must start");
    const std::string allocatedJson = BuildRtBenchmarkEvidenceJson(allocated);
    context.Check(allocatedJson.find("\"status\": \"allocated\"") != std::string::npos &&
                      ObjectForKey(allocatedJson, "simulationStepCpuMs").find("null") ==
                          std::string::npos &&
                      allocatedJson.find("\"simulationStepCpuMs\": null") != std::string::npos &&
                      allocatedJson.find("\"gpuRtDurationMs\": null") != std::string::npos,
                  "allocated zero-sample runs must emit null distributions, never measured zeroes");
    const std::string allocatedText = BuildRtBenchmarkEvidenceText(allocated);
    context.Check(allocatedText.find("Status: allocated") != std::string::npos &&
                      allocatedText.find("simulationStepCpuMs: N/A") != std::string::npos &&
                      allocatedText.find("gpuRtDurationMs: N/A") != std::string::npos,
                  "text must use N/A for unavailable allocated-run distributions");

    RtBenchmarkEvidenceRun cancelled;
    context.Check(cancelled.Start(1u) && cancelled.ArmMeasurement(72u, 82u) &&
                      cancelled.ExpectFrame({static_cast<std::uint32_t>(ShowcaseZone::Finale), 2u})
                          .has_value(),
                  "cancelled fixture must retain one intended row");
    cancelled.Cancel();
    const std::string cancelledJson = BuildRtBenchmarkEvidenceJson(cancelled);
    context.Check(cancelledJson.find("\"status\": \"cancelled\"") != std::string::npos &&
                      cancelledJson.find("\"cancelled\": 1") != std::string::npos &&
                      cancelledJson.find("\"disposition\": \"cancelled\"") != std::string::npos &&
                      cancelledJson.find("\"failure\": \"cancelled\"") != std::string::npos &&
                      cancelledJson.find("\"submittedIdentity\": null") != std::string::npos &&
                      cancelledJson.find("\"completionIdentity\": null") != std::string::npos,
                  "cancelled rows must preserve missing identities as null without invented serials");

    RtBenchmarkEvidenceRun missing;
    context.Check(missing.Start(2u) && missing.ArmMeasurement(73u, 83u),
                  "missing-completion fixture must start");
    const auto first = MakeSnapshot(
        1u, 73u, 83u, 0u, 1'000'000u, RtSampleStatus::Valid, 2'000'000u);
    const auto second = MakeSnapshot(
        2u, 73u, 83u, 1u, 2'000'000u, RtSampleStatus::Valid, 3'000'000u);
    ExpectAndBind(context, missing,
                  {static_cast<std::uint32_t>(ShowcaseZone::Opening), 2u}, first);
    context.Check(missing.Complete(first), "N-1 completion must resolve before final drain");
    ExpectAndBind(context, missing,
                  {static_cast<std::uint32_t>(ShowcaseZone::SkeletonRoom), 2u}, second);
    context.Check(missing.RecordOwnerDrainResult(true) && !missing.Finalize(),
                  "final drain must reject the missing Nth completion");
    const std::string missingJson = BuildRtBenchmarkEvidenceJson(missing);
    context.Check(missingJson.find("\"status\": \"incomplete\"") != std::string::npos &&
                      missingJson.find("\"completed\": 1") != std::string::npos &&
                      missingJson.find("\"rejected\": 1") != std::string::npos &&
                      missingJson.find("\"missing-completion\": 1") != std::string::npos &&
                      missingJson.find("\"gpuRtDurationMs\": null") != std::string::npos,
                  "final-drain missing completion must be an explicit failure and invalidate distribution");
    const std::string skeletonZone =
        ArrayObjectWith(missingJson, "\"name\": \"skeleton-room\"");
    context.Check(skeletonZone.find("\"intended\": 1") != std::string::npos &&
                      skeletonZone.find("\"rejected\": 1") != std::string::npos &&
                      skeletonZone.find("\"missing\": 1") != std::string::npos,
                  "zone-boundary N-1 loss must remain attributed to the intended next zone");
}

void TestCpuAndGpuEligibilityRemainIndependent(TestContext& context)
{
    RtBenchmarkEvidenceRun cpuValid;
    context.Check(cpuValid.Start(2u) && cpuValid.ArmMeasurement(74u, 84u),
                  "CPU-valid mixed-GPU fixture must start");
    const auto pending = MakeSnapshot(
        1u, 74u, 84u, 0u, 1'000'000u, RtSampleStatus::Pending, 0u);
    const auto error = MakeSnapshot(
        2u, 74u, 84u, 1u, 3'000'000u, RtSampleStatus::Error, 0u);
    ExpectAndBind(context, cpuValid,
                  {static_cast<std::uint32_t>(ShowcaseZone::Opening), 2u}, pending);
    context.Check(cpuValid.Complete(pending), "Pending GPU must not reject valid CPU evidence");
    ExpectAndBind(context, cpuValid,
                  {static_cast<std::uint32_t>(ShowcaseZone::Opening), 2u}, error);
    context.Check(cpuValid.Complete(error) && cpuValid.RecordOwnerDrainResult(true) &&
                      cpuValid.Finalize(),
                  "GPU Error must not make a CPU-complete run incomplete");
    const std::string cpuValidJson = BuildRtBenchmarkEvidenceJson(cpuValid);
    const std::string mixedGpu = ObjectForKey(cpuValidJson, "gpuStatusCounts");
    context.Check(ObjectForKey(cpuValidJson, "simulationStepCpuMs")
                              .find("\"meanMilliseconds\": 2.0000") != std::string::npos &&
                      cpuValidJson.find("\"gpuRtDurationMs\": null") != std::string::npos &&
                      mixedGpu.find("\"pending\": 1") != std::string::npos &&
                      mixedGpu.find("\"error\": 1") != std::string::npos,
                  "valid CPU distributions must survive Pending/Error GPU while GPU aggregate stays null");

    RtBenchmarkEvidenceRun gpuValid;
    context.Check(gpuValid.Start(1u) && gpuValid.ArmMeasurement(75u, 85u),
                  "GPU-valid CPU-ineligible fixture must start");
    const auto cpuIneligible = MakeSnapshot(
        1u, 75u, 85u, 0u, 2'000'000u, RtSampleStatus::Valid, 7'000'000u, false);
    ExpectAndBind(context, gpuValid,
                  {static_cast<std::uint32_t>(ShowcaseZone::Finale), 2u}, cpuIneligible);
    context.Check(gpuValid.Complete(cpuIneligible) && gpuValid.RecordOwnerDrainResult(true) &&
                      !gpuValid.Finalize(),
                  "CPU-ineligible completion must produce an incomplete CPU run");
    const std::string gpuValidJson = BuildRtBenchmarkEvidenceJson(gpuValid);
    context.Check(gpuValidJson.find("\"simulationStepCpuMs\": null") != std::string::npos &&
                      ObjectForKey(gpuValidJson, "gpuRtDurationMs")
                              .find("\"meanMilliseconds\": 7.0000") != std::string::npos &&
                      gpuValidJson.find("\"failure\": \"cpu-ineligible\"") !=
                          std::string::npos,
                  "valid GPU distribution must survive independent CPU rejection");

    RtBenchmarkEvidenceRun failedPresentation;
    context.Check(failedPresentation.Start(1u) &&
                      failedPresentation.ArmMeasurement(76u, 86u),
                  "failed-presentation fixture must start");
    const auto failed = MakeSnapshot(1u, 76u, 86u, 0u, 2'000'000u,
                                     RtSampleStatus::Valid, 8'000'000u, false,
                                     RtPresentationOutcome::Failed);
    ExpectAndBind(context, failedPresentation,
                  {static_cast<std::uint32_t>(ShowcaseZone::TransmissionThreshold), 2u},
                  failed);
    context.Check(failedPresentation.Complete(failed) &&
                      failedPresentation.RecordOwnerDrainResult(true) &&
                      !failedPresentation.Finalize(),
                  "failed-presentation completion must remain an accounted row");
    const std::string failedJson = BuildRtBenchmarkEvidenceJson(failedPresentation);
    context.Check(failedJson.find("\"presentationOutcome\": \"failed\"") !=
                          std::string::npos &&
                      failedJson.find("\"failure\": \"presentation-failed\"") !=
                          std::string::npos &&
                      failedJson.find("\"presentation-failed\": 1") !=
                          std::string::npos,
                  "failed presentation must retain both the raw outcome and canonical failure count");

    RtBenchmarkEvidenceRun presentedNeedsRecreate;
    context.Check(presentedNeedsRecreate.Start(1u) &&
                      presentedNeedsRecreate.ArmMeasurement(78u, 88u),
                  "presented-needs-recreate fixture must start");
    const auto suboptimal = MakeSnapshot(
        1u, 78u, 88u, 0u, 2'000'000u, RtSampleStatus::Valid, 8'000'000u, false,
        RtPresentationOutcome::PresentedNeedsRecreate);
    ExpectAndBind(context, presentedNeedsRecreate,
                  {static_cast<std::uint32_t>(ShowcaseZone::TransmissionThreshold), 2u},
                  suboptimal);
    context.Check(presentedNeedsRecreate.Complete(suboptimal) &&
                      presentedNeedsRecreate.RecordOwnerDrainResult(true) &&
                      !presentedNeedsRecreate.Finalize(),
                  "successful SUBOPTIMAL presentation must remain an accounted ineligible row");
    const std::string suboptimalJson = BuildRtBenchmarkEvidenceJson(presentedNeedsRecreate);
    context.Check(suboptimalJson.find(
                      "\"presentationOutcome\": \"presented-needs-recreate\"") !=
                          std::string::npos &&
                      suboptimalJson.find("\"failure\": \"presented-needs-recreate\"") !=
                          std::string::npos &&
                      suboptimalJson.find("\"presented-needs-recreate\": 1") !=
                          std::string::npos &&
                      suboptimalJson.find("\"presentation-failed\": 0") !=
                          std::string::npos,
                  "successful SUBOPTIMAL must serialize distinctly from actual presentation failure");
}

void TestReportsUseClassicLocale(TestContext& context)
{
    RtBenchmarkEvidenceRun run;
    context.Check(run.Start(1u) && run.ArmMeasurement(77u, 87u),
                  "locale fixture must start");
    const auto snapshot = MakeSnapshot(
        1u, 77u, 87u, 0u, 1'500'000u, RtSampleStatus::Valid, 2'500'000u);
    ExpectAndBind(context, run, {static_cast<std::uint32_t>(ShowcaseZone::Opening), 2u},
                  snapshot);
    context.Check(run.Complete(snapshot) && run.RecordOwnerDrainResult(true) && run.Finalize(),
                  "locale fixture must finalize");

    const std::locale previous = std::locale();
    std::locale::global(std::locale(previous, new CommaDecimalFacet));
    const std::string json = BuildRtBenchmarkEvidenceJson(run);
    const std::string text = BuildRtBenchmarkEvidenceText(run);
    std::locale::global(previous);
    context.Check(json.find("\"meanMilliseconds\": 1.5000") != std::string::npos &&
                      json.find("1,5000") == std::string::npos &&
                      text.find("mean=1.500 ms") != std::string::npos &&
                      text.find("1,500 ms") == std::string::npos,
                  "JSON and text numeric formatting must remain classic-locale under hostile globals");
}

} // namespace

int main()
{
    TestContext context;
    TestCompleteRunReportsStableStagesZonesAndActualIdentities(context);
    TestAllocatedCancelledAndMissingCompletionAreExplicit(context);
    TestCpuAndGpuEligibilityRemainIndependent(context);
    TestReportsUseClassicLocale(context);
    if (context.failures == 0)
    {
        std::cout << "RT benchmark evidence report tests passed.\n";
    }
    return context.failures == 0 ? 0 : 1;
}
