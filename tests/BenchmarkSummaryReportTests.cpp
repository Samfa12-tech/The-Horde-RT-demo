#include "reporting/BenchmarkSummaryReport.h"
#include "reporting/PublicBenchmarkReport.h"
#include "reporting/DeveloperReportSubmission.h"
#include "reporting/PlaytestReport.h"

#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
using namespace horde::telemetry;
using namespace horde::reporting;
using namespace horde::gameplay;
bool passed = true;
void Check(const bool value, const char* message)
{
    if (!value) { passed = false; std::cerr << "Benchmark summary: " << message << '\n'; }
}
constexpr std::string_view kRunUuid = "11111111-1111-4111-8111-111111111111";
constexpr std::string_view kReportUuid = "22222222-2222-4222-9222-222222222222";

BenchmarkSummaryConfiguration Configuration()
{
    BenchmarkSummaryConfiguration c;
    c.sceneEpoch = 3u; c.measurementGeneration = 8u;
    c.metadata.buildIdentity = "1.6.2-local";
    c.metadata.shaderIdentity = "opaqueFast:shipping_mobile_opaque@" + std::string(64u, 'a') +
        "|genericDielectric:shipping_mobile_generic@" + std::string(64u, 'b');
    c.metadata.gpuName = "Test GPU"; c.metadata.vulkanApi = "1.3.0";
    c.metadata.executionBackend = "RayTracingPipeline";
    c.metadata.presentMode = "FIFO"; c.metadata.materialEncoding = "RGBA8 raw fallback";
    c.metadata.legacyFrameTimingScope = "windows-render-plus-rtlab-telemetry";
    c.metadata.internalWidth = 960u; c.metadata.internalHeight = 540u;
    c.metadata.presentationWidth = 960u; c.metadata.presentationHeight = 540u;
    // Deliberately private/unbounded-looking unused legacy fields MUST NOT escape.
    c.metadata.runId = "private-account-run";
    c.metadata.timestampUtc = "private-original-file-label";
    c.metadata.rtMode = "private-full-diagnostic-report";
    return c;
}

RtPerformanceEvidenceSnapshot Snapshot(const std::uint64_t serial, const RtSampleStatus gpu)
{
    RtPerformanceEvidenceSnapshot s;
    s.identity.submitted.frame = {3u, 8u, serial, serial, 1000u + serial, 0u};
    s.identity.submitted.submissionSerial = serial; s.identity.completionSerial = serial;
    auto& p = s.scene.pipeline;
    p.executionMode = RtExecutionMode::RayTracingPipeline;
    p.instrumentation = RtInstrumentationMode::Shipping;
    p.dielectricQuality = RtDielectricQuality::Mobile;
    p.activeStrategy = RtMaterialStrategy::OpaqueFast; p.waterQuality = RtWaterQuality::Mobile;
    Check(AssignRtFixedText(p.bundleKey, "shipping_mobile_pair"), "fixture bundle key");
    Check(AssignRtFixedText(p.opaqueFast.key, "shipping_mobile_opaque"), "fixture opaque key");
    Check(AssignRtFixedText(p.opaqueFast.sha256, std::string(64u, 'a')), "fixture opaque hash");
    Check(AssignRtFixedText(p.genericDielectric.key, "shipping_mobile_generic"), "fixture generic key");
    Check(AssignRtFixedText(p.genericDielectric.sha256, std::string(64u, 'b')), "fixture generic hash");
    p.active = p.opaqueFast;
    s.scene.stages.status = RtSampleStatus::Valid;
    for (auto& stage : s.scene.stages.values) { stage.durationNanoseconds = 1000u; stage.operationCount = 1u; }
    auto& skin = s.scene.stages.values[RtStageIndex(RtStage::Skin)];
    skin.durationNanoseconds = 2000u; skin.operationCount = 2u;
    s.scene.stages.values[RtStageIndex(RtStage::WholeFrameCycle)].durationNanoseconds = 10'000'000u + serial * 1000u;
    s.scene.dispatch.sceneReady = s.scene.dispatch.rtDispatchRecorded = s.scene.dispatch.swapchainCopyRecorded = true;
    s.dielectric.status = RtSampleStatus::CompiledOut;
    s.gpu.status = gpu;
    if (gpu == RtSampleStatus::Valid)
    {
        s.gpu.hasDuration = true; s.gpu.durationNanoseconds = 5'000'000u + serial * 2000u;
        s.gpu.completedSubmissionSerial = serial; s.gpu.timestampValidBits = 64u;
        s.gpu.timestampPeriodPicoseconds = 1000u; s.gpu.sampleCount = 1u;
    }
    s.presentation.outcome = RtPresentationOutcome::Presented;
    s.presentation.lastSuccessfulPresentSubmissionSerial = serial;
    s.cpuBenchmarkEligible = true;
    s.benchmarkEligible = gpu == RtSampleStatus::Valid || gpu == RtSampleStatus::Disabled || gpu == RtSampleStatus::Unsupported;
    return s;
}

struct CompletedOwners
{
    std::optional<RtFrameDimensionsEvidence> submittedDimensions{};
    unsigned dimensionsDrift = 0;
    ShowcaseBenchmarkRun benchmark;
    RtBenchmarkEvidenceRun evidence;
    void Run(const RtSampleStatus gpu = RtSampleStatus::Valid, const bool wrongZone = false,
        const bool leaveLastPending = false, const bool mixedUnsupported = false,
        const BenchmarkSummaryConfiguration* uploaded = nullptr, const bool middleQualityChange = false,
        const bool middleShadowChange = false, const bool middleMistChange = false,
        const bool middleMistUnavailable = false)
    {
        benchmark.Start(2u, BenchmarkWorkload::LanternHeldHigh);
        Check(evidence.Start(kLanternBenchmarkFramesPerLap), "allocate real bounded evidence owner");
        bool armed = false;
        std::uint64_t serial = 0u;
        while (benchmark.IsRunning())
        {
            const auto advance = benchmark.Advance();
            if (benchmark.CurrentLap() == 2u)
            {
                if (!armed) { Check(evidence.ArmMeasurement(3u, 8u), "arm measurement after warmup"); armed = true; }
                ++serial;
                auto s = Snapshot(serial, mixedUnsupported && serial % 2u == 0u ? RtSampleStatus::Unsupported : gpu);
                s.scene.dimensions = submittedDimensions;
                if (serial == kLanternBenchmarkFramesPerLap / 2u) {
                    if (dimensionsDrift == 1 && s.scene.dimensions) {
                        --s.scene.dimensions->internalWidth;
                        s.scene.dimensions->linearBlit = true;
                        s.scene.dimensions->cameraAspect = float(s.scene.dimensions->internalWidth) / s.scene.dimensions->internalHeight;
                    }
                    if (dimensionsDrift == 2 && s.scene.dimensions) {
                        ++s.scene.dimensions->outputWidth; s.scene.dimensions->linearBlit = true;
                    }
                    if (dimensionsDrift == 3 && s.scene.dimensions) {
                        s.scene.dimensions->presentationTransform = 1;
                        s.scene.dimensions->cameraAspect = 1.0f / s.scene.dimensions->cameraAspect;
                    }
                    if (dimensionsDrift == 4 && s.scene.dimensions) s.scene.dimensions->dynamicResolutionEnabled = true;
                    if (dimensionsDrift == 5) s.scene.dimensions.reset();
                    if (dimensionsDrift == 6) s.scene.dimensions = RtFrameDimensionsEvidence{960,540,960,540,0,960.0f/540,false,false};
                }
                if (uploaded)
                {
                    s.scene.shadowQuality = uploaded->shadowQuality;
                    s.scene.fireQuality = uploaded->uploadedFireQuality;
                    s.scene.actualUploadedMistEnabled = uploaded->actualUploadedMistEnabled;
                    s.scene.actualUploadedDustQuality = uploaded->actualUploadedDustQuality;
                    if (middleMistChange && serial == kLanternBenchmarkFramesPerLap / 2u)
                        s.scene.actualUploadedMistEnabled = !uploaded->actualUploadedMistEnabled.value_or(true);
                    if (middleMistUnavailable && serial == kLanternBenchmarkFramesPerLap / 2u)
                        s.scene.actualUploadedMistEnabled.reset();
                    if (middleQualityChange && serial == kLanternBenchmarkFramesPerLap / 2u)
                        s.scene.fireQuality = RtFireQualityEvidence{RtFireQuality::Mobile, 4u, 1u};
                    if (middleShadowChange && serial == kLanternBenchmarkFramesPerLap / 2u)
                        s.scene.shadowQuality = RtShadowQualityEvidence{RtShadowMode::Current, 1u, 1u, 0u};
                }
                const auto zone = wrongZone ? ShowcaseZone::Opening : advance.replay.zone;
                const auto index = evidence.ExpectFrame({static_cast<std::uint32_t>(zone), 2u});
                Check(index.has_value(), "expected frame owned");
                if (index)
                {
                    Check(evidence.BindSubmitted(*index, s.identity.submitted), "bind exact submitted fixture identity");
                    if (!leaveLastPending || serial != kLanternBenchmarkFramesPerLap)
                        Check(evidence.Complete(s), "canonical fixture completes on real ledger seam");
                }
            }
            benchmark.RecordFrame(16.0 + static_cast<double>(serial % 7u), true);
        }
        if (!leaveLastPending)
        {
            Check(evidence.RecordOwnerDrainResult(true), "actual owner drain result fixture");
            Check(evidence.Finalize(), "full ledger finalization fixture");
        }
    Check(benchmark.Passed(), "real deterministic benchmark course completed");
    }
};

BenchmarkSummaryReportApproval Approval(const bool hardware = false)
{
    return {true, hardware, kReportUuid, "2026-10-03T12:34:56Z"};
}

void TestCompletedFreezePrivacyAndPopulation()
{
    CompletedOwners owners; owners.Run();
    auto config = Configuration();
    const auto summary = CaptureBenchmarkSummary(owners.benchmark, owners.evidence, config, config, kRunUuid, "Nonunique model");
    Check(summary.IsReady(), "completed matching measured owners freeze");
    Check(summary.Data().overall.frames == 600u && summary.Data().overall.gpu.sampleCount == 600u &&
        summary.Data().overall.cpu[RtStageIndex(RtStage::WholeFrameCycle)].sampleCount == 600u,
        "warmup excluded and CPU/GPU population denominators retained");
    RtStageStatistics actualGpu{};
    Check(owners.evidence.GpuStatistics(actualGpu) && actualGpu.p95Milliseconds == summary.Data().overall.gpu.p95Milliseconds &&
        summary.Data().overall.legacy.averageMs != actualGpu.meanMilliseconds, "retain independent legacy vs owning GPU distributions");
    auto approval = Approval(); approval.consentToPrepare = false;
    const auto off = PrepareBenchmarkSummaryReport(summary, approval);
    Check(off.Status() == BenchmarkSummaryReportStatus::ConsentRequired && off.Json().empty() && off.ReportId().empty(),
        "no consent yields no prepared bytes");
    const auto prepared = PrepareBenchmarkSummaryReport(summary, Approval());
    Check(prepared.IsReady() && prepared.Json().size() <= kBenchmarkSummaryReportMaxBytes, "schema2 bounded local summary ready");
    const auto json = prepared.Json();
    Check(!PreparePrivateDeveloperReport(prepared).IsReady(), "local benchmark approval is not private send consent");
    const auto remote = PreparePrivateDeveloperReport(prepared, true);
    Check(remote.IsReady() && remote.LocalJson() == json && remote.Kind() == DeveloperReportKind::Benchmark &&
        remote.Json().find("\"publishToBoard\":false") != std::string_view::npos,
        "typed private benchmark wrapper has no public-board authority");
    Check(BuildDeveloperReportRequest(remote, "").empty() && BuildDeveloperReportRequest(remote, "bad\ntoken").empty() &&
        !BuildDeveloperReportRequest(remote, "fresh-local-token").empty() &&
        remote.Json().find("turnstileToken") == std::string_view::npos,
        "one-time verification cannot mutate the reviewed benchmark or frozen private wrapper");
    Check(json.find("\"benchmarkProfile\":{\"id\":\"custom\",\"classification\":\"custom-unranked\",\"ranked\":false}") != std::string_view::npos &&
        json.find("provisional-v1") == std::string_view::npos,
        "current live report explicitly stays custom and unranked until fixed-surface observations exist");
    Check(json.find("\"reportKind\":\"benchmark-summary\"") != std::string_view::npos &&
        json.find("\"warmupLaps\":1") != std::string_view::npos && json.find("not-collected") != std::string_view::npos &&
        json.find("\"temperatureC\":null") != std::string_view::npos && json.find("\"scanoutFpsMeasured\":false") != std::string_view::npos,
        "kind/warmup/thermal unknown and no scanout measurement labels are explicit");
    for (const auto privateField : {"Nonunique model", "Test GPU", "private-account", "private-original", "private-full", "rows", "submissionSerial", "sceneEpoch", "turnstileToken", "screenshot"})
        Check(json.find(privateField) == std::string_view::npos, "private/raw/attachment fields absent by construction");
    const auto optIn = PrepareBenchmarkSummaryReport(summary, Approval(true));
    Check(optIn.IsReady() && optIn.Json().find("Nonunique model") != std::string_view::npos &&
        optIn.Json().find("Test GPU") != std::string_view::npos, "separate basic hardware opt-in changes only approved bytes");
    auto coolingApproval = Approval();
    coolingApproval.declaredCooling = BenchmarkSummaryCooling::ExternalDeclared;
    const auto coolingReport = PrepareBenchmarkSummaryReport(summary, coolingApproval);
    Check(coolingReport.IsReady() && coolingReport.Json().find("external-declared") != std::string_view::npos &&
        summary.Data().cooling == BenchmarkSummaryCooling::Unknown &&
        PrepareBenchmarkSummaryReport(summary, Approval()).Json() == json,
        "end-screen cooling declaration leaves immutable measured evidence and prior approved bytes unchanged");
    coolingApproval.declaredCooling = static_cast<BenchmarkSummaryCooling>(255u);
    const auto invalidCooling = PrepareBenchmarkSummaryReport(summary, coolingApproval);
    Check(invalidCooling.Status() == BenchmarkSummaryReportStatus::InvalidCooling && invalidCooling.Json().empty(),
        "invalid end-screen cooling enum yields no exportable bytes");
    owners.benchmark.Start(); Check(owners.evidence.Start(2u), "new owner run replaces local evidence");
    config.metadata.buildIdentity = "later-build";
    Check(PrepareBenchmarkSummaryReport(summary, Approval()).Json() == json, "freeze survives route/reset/source-owner changes");
}

void TestInvalidOwnersScopeAndIdentity()
{
    CompletedOwners partial; partial.Run(RtSampleStatus::Valid, false, true);
    auto c = Configuration();
    Check(!CaptureBenchmarkSummary(partial.benchmark, partial.evidence, c, c, kRunUuid).IsReady(), "pending final submission cannot send summary");
    Check(partial.evidence.RecordOwnerDrainResult(false), "failed drain recorded");
    Check(!partial.evidence.Finalize() && !CaptureBenchmarkSummary(partial.benchmark, partial.evidence, c, c, kRunUuid).IsReady(),
        "failed owner drain cannot certify completed summary");
    CompletedOwners wrong; wrong.Run(RtSampleStatus::Valid, true);
    Check(CaptureBenchmarkSummary(wrong.benchmark, wrong.evidence, c, c, kRunUuid).Status() == BenchmarkSummaryStatus::MismatchedPopulation,
        "equal frame counts with wrong row zones rejected");
    CompletedOwners owners; owners.Run();
    auto changed = c; changed.glassEnabled = false;
    Check(!CaptureBenchmarkSummary(owners.benchmark, owners.evidence, c, changed, kRunUuid).IsReady(), "measured effective tuple changed rejected");
    changed = c; changed.metadata.legacyFrameTimingScope = "display-fps";
    Check(!CaptureBenchmarkSummary(owners.benchmark, owners.evidence, changed, changed, kRunUuid).IsReady(), "FPS scope relabel rejected");
    changed = c; changed.measurementGeneration = 9u;
    Check(!CaptureBenchmarkSummary(owners.benchmark, owners.evidence, changed, changed, kRunUuid).IsReady(), "stale measurement scope rejected");
    Check(!CaptureBenchmarkSummary(owners.benchmark, owners.evidence, c, c, "device_serial_123").IsReady(), "non UUID run identity rejected");
    Check(!CaptureBenchmarkSummary(owners.benchmark, owners.evidence, c, c, kRunUuid, {}, static_cast<BenchmarkSummaryCooling>(255u)).IsReady(),
        "cooling has closed enum admission");
    changed = c; changed.metadata.renderScalePercent = 33u;
    Check(!CaptureBenchmarkSummary(owners.benchmark, owners.evidence, changed, changed, kRunUuid).IsReady(),
        "same incorrect start/end percentage with full internal extent cannot certify scaled result");
    changed.metadata.presentationWidth = 961u; changed.metadata.presentationHeight = 541u;
    changed.metadata.internalWidth = 317u; changed.metadata.internalHeight = 179u;
    Check(CaptureBenchmarkSummary(owners.benchmark, owners.evidence, changed, changed, kRunUuid).IsReady(),
        "isolated declared33 percentage accepts exact rounded odd extents independent of ordinary build admission");
    changed.metadata.internalHeight = 178u;
    Check(!CaptureBenchmarkSummary(owners.benchmark, owners.evidence, changed, changed, kRunUuid).IsReady(),
        "one-pixel incorrect scaled extent rejected");
    auto summary = CaptureBenchmarkSummary(owners.benchmark, owners.evidence, c, c, kRunUuid);
    auto a = Approval(); a.reportUuid = "22222222-2222-1222-9222-222222222222";
    Check(PrepareBenchmarkSummaryReport(summary, a).Status() == BenchmarkSummaryReportStatus::InvalidIdentity, "time UUID/nonrandom-version report identity rejected");
    a = Approval(); a.capturedAtUtc = "2026-02-30T12:34:56Z";
    Check(PrepareBenchmarkSummaryReport(summary, a).Status() == BenchmarkSummaryReportStatus::InvalidTimestamp, "reuse strict UTC calendar validation");
    c.metadata.buildIdentity.assign(129u, 'x');
    Check(!CaptureBenchmarkSummary(owners.benchmark, owners.evidence, c, c, kRunUuid).IsReady(), "oversize metadata hard rejected before copy");
    c = Configuration(); c.metadata.buildIdentity = "C:\\Users\\PrivateOwner\\build.exe";
    summary = CaptureBenchmarkSummary(owners.benchmark, owners.evidence, c, c, kRunUuid);
    const auto privateBuild = PrepareBenchmarkSummaryReport(summary, Approval());
    Check(privateBuild.Status() == BenchmarkSummaryReportStatus::InvalidLabel && privateBuild.Json().empty(),
        "private build path rejected without leaking prepared bytes");
    c = Configuration(); c.metadata.shaderIdentity = "mail@example.invalid";
    summary = CaptureBenchmarkSummary(owners.benchmark, owners.evidence, c, c, kRunUuid);
    Check(PrepareBenchmarkSummaryReport(summary, Approval()).Status() == BenchmarkSummaryReportStatus::InvalidLabel,
        "shader identity admits exact production key/hash pairs, not arbitrary email/text");
    c = Configuration(); c.platform = BenchmarkSummaryPlatform::Android;
    c.metadata.legacyFrameTimingScope = "android-render-entry-through-present";
    Check(CaptureBenchmarkSummary(owners.benchmark, owners.evidence, c, c, kRunUuid).IsReady(),
        "Android distinct legacy interval scope admitted without relabeling GPU or display FPS");
    owners.benchmark.Cancel();
    Check(!CaptureBenchmarkSummary(owners.benchmark, owners.evidence, c, c, kRunUuid).IsReady(),
        "cancelled route cannot reuse otherwise completed ledger as valid run");
}

void TestUnavailableHardwarePrivacyAndIdentityRetry()
{
    CompletedOwners owners; owners.Run(RtSampleStatus::Unsupported);
    const auto c = Configuration();
    const auto summary = CaptureBenchmarkSummary(owners.benchmark, owners.evidence, c, c, kRunUuid,
        "private@example.invalid", BenchmarkSummaryCooling::ExternalDeclared);
    const auto report = PrepareBenchmarkSummaryReport(summary, Approval());
    Check(report.IsReady() && report.Json().find("\"gpuRtDurationMs\":null") != std::string_view::npos &&
        report.Json().find("\"unsupported\":600") != std::string_view::npos &&
        report.Json().find("external-declared") != std::string_view::npos, "unavailable GPU has null summary/denominator; cooling is declared not measured");
    Check(PrepareBenchmarkSummaryReport(summary, Approval(true)).Status() == BenchmarkSummaryReportStatus::InvalidLabel,
        "email in optional model fails only if opted in, never leaks in unconsented body");
    auto otherApproval = Approval(); otherApproval.capturedAtUtc = "2026-10-03T12:34:57Z";
    Check(CompareBenchmarkSummaryContent(report, PrepareBenchmarkSummaryReport(summary, Approval())) == BenchmarkSummaryContentMatch::SameContent,
        "same ID/content canonical bytes stable");
    Check(CompareBenchmarkSummaryContent(report, PrepareBenchmarkSummaryReport(summary, otherApproval)) == BenchmarkSummaryContentMatch::Conflict,
        "same ID changed content cannot be a dedup retry");
    otherApproval = Approval(); otherApproval.reportUuid = "33333333-3333-4333-a333-333333333333";
    Check(CompareBenchmarkSummaryContent(report, PrepareBenchmarkSummaryReport(summary, otherApproval)) == BenchmarkSummaryContentMatch::DifferentId,
        "new report ID distinct from same-content resend");
    PlaytestReportDelivery delivery;
    PlaytestReportAttempt first, retry;
    Check(delivery.BeginBenchmarkSummary(report, first), "reuse typed local approval state machine without transport");
    Check(!delivery.BeginBenchmarkSummary(report, retry), "parallel/double Begin cannot resend frozen summary");
    Check(delivery.Complete(first.token, PlaytestReportDeliveryResult::RetryableFailure) && delivery.Retry(retry), "explicit local retry admitted");
    Check(retry.token != first.token && retry.reportId == first.reportId && retry.json == first.json, "new attempt retains exact UUID/bytes");
    Check(!delivery.Complete(first.token, PlaytestReportDeliveryResult::Accepted), "old attempt completion cannot certify retry");
    delivery.Cancel();
    Check(!delivery.Complete(retry.token, PlaytestReportDeliveryResult::Accepted) && !delivery.Retry(retry), "cancel invalidates stale success/retry");
    PlaytestReportDelivery accepted;
    Check(accepted.BeginBenchmarkSummary(report, first) && accepted.Complete(first.token, PlaytestReportDeliveryResult::Accepted) &&
        !accepted.BeginBenchmarkSummary(report, retry) && !accepted.Retry(retry), "accepted local report cannot restart or create duplicate attempt");
    Check(CompareBenchmarkSummaryContent(report, PreparedBenchmarkSummaryReport{}) == BenchmarkSummaryContentMatch::Invalid,
        "unprepared record cannot be dedup-certified");
    CompletedOwners mixed; mixed.Run(RtSampleStatus::Valid, false, false, true);
    const auto mixedSummary = CaptureBenchmarkSummary(mixed.benchmark, mixed.evidence, c, c, kRunUuid);
    RtStageStatistics strictAllRows{};
    Check(!mixed.evidence.GpuStatistics(strictAllRows), "original all-row GPU evidence API remains strict for mixed availability");
    std::array<std::uint64_t, 300u> actualValidDurations{};
    std::size_t actualValid = 0u, actualUnsupported = 0u;
    for (std::size_t i = 0u; i < mixed.evidence.ExpectedCount(); ++i)
    {
        RtExpectedFrameRecord row{};
        Check(mixed.evidence.TryGetExpectedFrame(i, row), "mixed canonical row exists");
        if (row.gpuStatus == RtSampleStatus::Valid)
        {
            Check(row.hasGpuDuration && actualValid < actualValidDurations.size(), "mixed valid row owns bounded duration");
            if (actualValid < actualValidDurations.size()) actualValidDurations[actualValid++] = row.gpuDurationNanoseconds;
        }
        else if (row.gpuStatus == RtSampleStatus::Unsupported) ++actualUnsupported;
    }
    std::cerr << "Mixed summary evidence: ownerStatus=" << static_cast<unsigned>(mixed.evidence.Status()) <<
        " summaryStatus=" << static_cast<unsigned>(mixedSummary.Status()) << " expected=" << mixed.evidence.ExpectedCount() <<
        " completed=" << mixed.evidence.CompletedCount() << " cpuAccepted=" << mixed.evidence.CpuAcceptedCount() <<
        " actualValid=" << actualValid << " actualUnsupported=" << actualUnsupported <<
        " summaryGpuSamples=" << mixedSummary.Data().overall.gpu.sampleCount << '\n';
    Check(mixedSummary.IsReady() && mixedSummary.Data().overall.frames == 600u &&
        mixedSummary.Data().overall.gpu.sampleCount == 300u && mixedSummary.Data().overall.gpuStatuses.valid == 300u &&
        mixedSummary.Data().overall.gpuStatuses.unsupported == 300u &&
        mixedSummary.Data().overall.cpu[RtStageIndex(RtStage::WholeFrameCycle)].sampleCount == 600u,
        "partial GPU availability retains its actual 300/600 population rather than synthetic complete timings");
    RtStageStatistics independentlyProjected{};
    Check(actualValid == 300u && actualUnsupported == 300u && ComputeRtDurationStatistics(actualValidDurations, independentlyProjected) &&
        mixedSummary.Data().overall.gpu.meanMilliseconds == independentlyProjected.meanMilliseconds &&
        mixedSummary.Data().overall.gpu.p95Milliseconds == independentlyProjected.p95Milliseconds &&
        mixedSummary.Data().overall.gpu.slowestOnePercentMeanMilliseconds == independentlyProjected.slowestOnePercentMeanMilliseconds,
        "mixed summary statistics equal independently projected actual valid completed durations");
}
void TestUploadedQualitySummary()
{
    auto configuration = Configuration();
    configuration.fire = BenchmarkSummaryFireQuality::Low;
    configuration.shadowQuality = RtShadowQualityEvidence{RtShadowMode::Higher, 2u, 2u, 0u};
    configuration.uploadedFireQuality = RtFireQualityEvidence{RtFireQuality::Low, 2u, 1u};
    CompletedOwners owners; owners.Run(RtSampleStatus::Valid, false, false, false, &configuration);
    const auto frozen = CaptureBenchmarkSummary(owners.benchmark, owners.evidence,
        configuration, configuration, kRunUuid);
    const auto report = PrepareBenchmarkSummaryReport(frozen, Approval());
    Check(frozen.IsReady() && report.IsReady() &&
        report.Json().find("\"fireQuality\":\"Low\"") != std::string_view::npos &&
        report.Json().find("\"shadowQuality\":{\"mode\":\"higher\",\"localPrimarySamples\":2,\"skyPrimarySamples\":2,\"reserved\":0}") != std::string_view::npos &&
        report.Json().find("\"uploadedFireQuality\":{\"volumeSteps\":2,\"reflectionSamples\":1,\"reflectedVolumeSteps\":2}") != std::string_view::npos,
        "local summary reports immutable actual Low fire and resolved uploaded shadow budgets");
    auto changed = configuration; changed.shadowQuality->mode = RtShadowMode::Lower;
    changed.shadowQuality->localPrimarySamples = changed.shadowQuality->skyPrimarySamples = 1u;
    Check(!CaptureBenchmarkSummary(owners.benchmark, owners.evidence, configuration, changed, kRunUuid).IsReady(),
        "different uploaded start/end shadow policy rejected");
    changed = configuration; changed.uploadedFireQuality.reset();
    Check(!CaptureBenchmarkSummary(owners.benchmark, owners.evidence, changed, changed, kRunUuid).IsReady(),
        "requested Low with no actual uploaded budget cannot certify summary");
    changed = configuration; changed.uploadedFireQuality->volumeSteps = 4u;
    Check(!CaptureBenchmarkSummary(owners.benchmark, owners.evidence, changed, changed, kRunUuid).IsReady(),
        "Low label cannot certify Mobile uploaded step budget");
    changed = configuration; changed.shadowQuality->reserved = 1u;
    Check(!CaptureBenchmarkSummary(owners.benchmark, owners.evidence, changed, changed, kRunUuid).IsReady(),
        "nonzero uploaded reserved field cannot enter summary");
    CompletedOwners middle; middle.Run(RtSampleStatus::Valid, false, false, false, &configuration, true);
    Check(CaptureBenchmarkSummary(middle.benchmark, middle.evidence, configuration, configuration, kRunUuid).Status() ==
        BenchmarkSummaryStatus::MismatchedPopulation,
        "valid mid-run uploaded fire change restored by completion is still rejected by exact owning rows");
    CompletedOwners middleShadow; middleShadow.Run(RtSampleStatus::Valid, false, false, false, &configuration, false, true);
    Check(CaptureBenchmarkSummary(middleShadow.benchmark, middleShadow.evidence, configuration, configuration, kRunUuid).Status() ==
        BenchmarkSummaryStatus::MismatchedPopulation,
        "valid mid-run uploaded shadow change restored by completion is rejected by exact owning rows");
    auto legacy = Configuration();
    CompletedOwners legacyOwners; legacyOwners.Run();
    const auto legacyReport = PrepareBenchmarkSummaryReport(CaptureBenchmarkSummary(legacyOwners.benchmark,
        legacyOwners.evidence, legacy, legacy, kRunUuid), Approval());
    Check(legacyReport.IsReady() && legacyReport.Json().find("shadowQuality") == std::string_view::npos &&
        legacyReport.Json().find("uploadedFireQuality") == std::string_view::npos,
        "legacy absent uploaded controls remain absent without fabricated Current budgets");
}

void TestOwningDustSummary()
{
    using horde::graphics::DustQuality;
    for (const auto quality : {DustQuality::Off, DustQuality::Low, DustQuality::Standard})
    {
        auto configuration = Configuration();
        configuration.actualUploadedDustQuality = quality;
        CompletedOwners owners;
        owners.Run(RtSampleStatus::Valid, false, false, false, &configuration);
        const auto frozen = CaptureBenchmarkSummary(owners.benchmark, owners.evidence,
            configuration, configuration, kRunUuid);
        const auto prepared = PrepareBenchmarkSummaryReport(frozen, Approval());
        const auto field = std::string("\"actualUploadedDustQuality\":") + std::to_string(static_cast<unsigned>(quality));
        Check(frozen.IsReady() && frozen.Data().configuration.actualUploadedDustQuality == quality &&
            prepared.IsReady() && prepared.Json().find(field) != std::string_view::npos,
            "summary retains actual owning Dust tier including Off");
        auto changed = configuration;
        changed.actualUploadedDustQuality = quality == DustQuality::Off ? DustQuality::Low : DustQuality::Off;
        Check(!CaptureBenchmarkSummary(owners.benchmark, owners.evidence, configuration, changed, kRunUuid).IsReady(),
            "different start/end Dust tier cannot enter a matched summary");
        changed.actualUploadedDustQuality.reset();
        Check(!CaptureBenchmarkSummary(owners.benchmark, owners.evidence, changed, changed, kRunUuid).IsReady(),
            "absent Dust configuration cannot replace measured owning rows");
        changed.actualUploadedDustQuality = static_cast<DustQuality>(3u);
        Check(!CaptureBenchmarkSummary(owners.benchmark, owners.evidence, changed, changed, kRunUuid).IsReady(),
            "invalid Dust configuration rejected");
    }
    auto legacy = Configuration();
    CompletedOwners owners; owners.Run();
    const auto report = PrepareBenchmarkSummaryReport(CaptureBenchmarkSummary(owners.benchmark,
        owners.evidence, legacy, legacy, kRunUuid), Approval());
    Check(report.IsReady() && report.Json().find("actualUploadedDustQuality") == std::string_view::npos,
        "legacy missing Dust state stays absent rather than fabricated Off");
}

void TestOwningMistSummary()
{
    for (const bool enabled : {true, false})
    {
        auto configuration = Configuration();
        configuration.actualUploadedMistEnabled = enabled;
        CompletedOwners owners;
        owners.Run(RtSampleStatus::Valid, false, false, false, &configuration);
        const auto frozen = CaptureBenchmarkSummary(owners.benchmark, owners.evidence,
            configuration, configuration, kRunUuid);
        const auto prepared = PrepareBenchmarkSummaryReport(frozen, Approval());
        Check(frozen.IsReady() && frozen.Data().configuration.actualUploadedMistEnabled == std::optional<bool>{enabled} &&
            prepared.IsReady() && prepared.Json().find(enabled ? "\"actualUploadedMistEnabled\":true" :
                "\"actualUploadedMistEnabled\":false") != std::string_view::npos,
            "frozen local summary identifies actual completed Mist On/Off without treating false as unavailable");
        const auto unavailable = Configuration();
        Check(CaptureBenchmarkSummary(owners.benchmark, owners.evidence, unavailable, unavailable, kRunUuid).Status() ==
            BenchmarkSummaryStatus::MismatchedPopulation,
            "legacy configuration cannot hide owning rows that actually uploaded a mist policy");
        auto changed = configuration;
        changed.actualUploadedMistEnabled = !enabled;
        Check(CaptureBenchmarkSummary(owners.benchmark, owners.evidence, configuration, changed, kRunUuid).Status() ==
            BenchmarkSummaryStatus::InvalidConfiguration,
            "different uploaded mist at arming and completion cannot certify an unchanged measured configuration");
        changed.actualUploadedMistEnabled.reset();
        Check(CaptureBenchmarkSummary(owners.benchmark, owners.evidence, configuration, changed, kRunUuid).Status() ==
            BenchmarkSummaryStatus::InvalidConfiguration,
            "available and unavailable mist configurations do not compare equal");
        CompletedOwners middle;
        middle.Run(RtSampleStatus::Valid, false, false, false, &configuration, false, false, true);
        Check(CaptureBenchmarkSummary(middle.benchmark, middle.evidence, configuration, configuration, kRunUuid).Status() ==
            BenchmarkSummaryStatus::MismatchedPopulation,
            "a single owning frame changed mid-run then restored is rejected despite matching mist endpoints");
        CompletedOwners missing;
        missing.Run(RtSampleStatus::Valid, false, false, false, &configuration, false, false, false, true);
        Check(CaptureBenchmarkSummary(missing.benchmark, missing.evidence, configuration, configuration, kRunUuid).Status() ==
            BenchmarkSummaryStatus::MismatchedPopulation,
            "one unavailable owning upload cannot inherit current mist state from configuration metadata");
    }
    const auto legacy = Configuration();
    CompletedOwners owners;
    owners.Run();
    const auto prepared = PrepareBenchmarkSummaryReport(CaptureBenchmarkSummary(owners.benchmark, owners.evidence,
        legacy, legacy, kRunUuid), Approval());
    Check(prepared.IsReady() && prepared.Json().find("actualUploadedMistEnabled") == std::string_view::npos,
        "historical summary absence remains absent without an inferred Mist On default");
}

void TestPublicProjectionConsentPrivacyAndEvidence()
{
    constexpr std::string_view publicId="33333333-3333-4333-a333-333333333333";
    CompletedOwners owners;
    owners.submittedDimensions=RtFrameDimensionsEvidence{960,540,960,540,0,960.0f/540,false,false};
    owners.Run();
    auto configuration=Configuration();
    const auto frozen=CaptureBenchmarkSummary(owners.benchmark,owners.evidence,configuration,configuration,kRunUuid,"Reviewed model");
    const auto local=PrepareBenchmarkSummaryReport(frozen,Approval(true));
    const auto privateSend=PreparePrivateDeveloperReport(local,true);
    Check(privateSend.IsReady(),"private sharing has independent approved content");
    auto approval=PublicBenchmarkReportApproval{false,false,publicId,"2026-10-03T12:34:56Z"};
    const auto declined=PreparePublicBenchmarkReport(frozen,approval);
    Check(declined.Status()==BenchmarkSummaryReportStatus::ConsentRequired && declined.Json().empty() && declined.ResultId().empty(),
        "private/local consent never authorizes public projection");
    approval.consentToPreparePublicPreview=true;
    const auto publicReport=PreparePublicBenchmarkReport(frozen,approval);
    const auto json=publicReport.Json();
    Check(publicReport.IsReady() && publicReport.ResultId()==publicId && json.size()<=kPublicBenchmarkReportMaxBytes,
        "typed public preview has separate bounded immutable identity");
    for (const auto privateValue : {kRunUuid,kReportUuid,std::string_view{"2026-10-03T12:34:56Z"},
         std::string_view{"private-account"},std::string_view{"private-original"},std::string_view{"private-full"},
         std::string_view{"runId"},std::string_view{"reportId"},std::string_view{"capturedAtUtc"},
         std::string_view{"submissionSerial"},std::string_view{"sceneEpoch"},std::string_view{"turnstileToken"},
         std::string_view{"consentToSubmit"},std::string_view{"Reviewed model"},std::string_view{"Test GPU"}})
        Check(json.find(privateValue)==json.npos,"public projection must omit private identity, timing, raw fields and unapproved hardware");
    Check(json.find("\"dateUtc\":\"2026-10-03\"")!=json.npos && json.find("\"model\":null")!=json.npos &&
        json.find("community-client-reported")!=json.npos && json.find("\"deviceCompatibilityVerified\":false")!=json.npos &&
        json.find("\"rankingEligible\":false")!=json.npos && json.find("custom-unranked")!=json.npos &&
        json.find("\"courseRevision\":null")!=json.npos && json.find("\"packageArtifactSha256\":null")!=json.npos,
        "public preview coarsens date and does not certify hardware, comparison buckets, profiles or artifact identity");
    Check(json.find("\"submittedDimensions\":{\"internalExtent\":[960,540]")!=json.npos &&
        json.find("\"dynamicResolutionEnabled\":false")!=json.npos &&
        json.find("\"measuredFrames\":600")!=json.npos && json.find("\"gpuStatusCounts\":{\"denominator\":600,\"valid\":600")!=json.npos &&
        json.find("\"scanoutFpsMeasured\":false")!=json.npos,
        "public projection preserves actual submitted pixels, DRS, measured population and measurement limits");
    approval.includeBasicHardware=true;
    const auto withHardware=PreparePublicBenchmarkReport(frozen,approval);
    Check(withHardware.IsReady() && withHardware.Json().find("Reviewed model")!=json.npos &&
        withHardware.Json().find("Test GPU")!=json.npos && publicReport.Json()==json,
        "optional reviewed hardware creates new public bytes without mutating earlier approval");
    const auto retry=PreparePublicBenchmarkReport(frozen,approval);
    Check(retry.Json()==withHardware.Json(),"same public approval/evidence retains exact review/retry bytes");
    approval.declaredCooling=BenchmarkSummaryCooling::ExternalDeclared;
    const auto cooling=PreparePublicBenchmarkReport(frozen,approval);
    Check(cooling.IsReady() && cooling.Json().find("external-declared")!=json.npos &&
        cooling.Json().find("\"temperatureC\":null")!=json.npos,"declared cooling remains separate from absent measured thermals");
    approval.resultUuid=kRunUuid;
    Check(PreparePublicBenchmarkReport(frozen,approval).Status()==BenchmarkSummaryReportStatus::InvalidIdentity,"a private measured run identity cannot be reused as a public result ID");
    approval.resultUuid="bad";
    Check(PreparePublicBenchmarkReport(frozen,approval).Status()==BenchmarkSummaryReportStatus::InvalidIdentity,"invalid public identity rejects");
    approval.resultUuid=publicId;approval.capturedAtUtc="2026-02-30T00:00:00Z";
    Check(PreparePublicBenchmarkReport(frozen,approval).Status()==BenchmarkSummaryReportStatus::InvalidTimestamp,"public coarse date requires a real validated calendar timestamp");
    approval.capturedAtUtc="2026-10-03T12:34:56Z";
    const auto unsafe=CaptureBenchmarkSummary(owners.benchmark,owners.evidence,configuration,configuration,kRunUuid,"person@example.com");
    Check(PreparePublicBenchmarkReport(unsafe,approval).Status()==BenchmarkSummaryReportStatus::InvalidLabel,"hardware opt-in cannot admit private/contact labels");
    approval.includeBasicHardware=false;
    Check(PreparePublicBenchmarkReport(unsafe,approval).IsReady(),"omitted hardware does not leak or unnecessarily block aggregate projection");
    Check(!PreparePublicBenchmarkReport(FrozenBenchmarkSummary{},approval).IsReady(),"incomplete benchmark cannot become a public result");
    owners.benchmark.Start();Check(owners.evidence.Start(2u),"replace measured owners after approved public capture");
    configuration.metadata.buildIdentity="later-build";
    Check(publicReport.Json()==json && privateSend.LocalJson()==local.Json(),"owner reset does not mutate either separately approved destination");
}

void TestSubmittedDimensionsSummary()
{
    const RtFrameDimensionsEvidence d{960,540,960,540,0,960.0f/540,false,false};
    const auto config = Configuration();
    CompletedOwners uniform; uniform.submittedDimensions = d; uniform.Run();
    const auto frozen = CaptureBenchmarkSummary(uniform.benchmark, uniform.evidence, config, config, kRunUuid);
    const auto prepared = PrepareBenchmarkSummaryReport(frozen, Approval());
    Check(frozen.IsReady() && frozen.Data().submittedDimensions == d && prepared.IsReady() &&
        prepared.Json().find("\"submittedDimensions\":{\"internalExtent\":[960,540]") != std::string_view::npos &&
        prepared.Json().find("\"dynamicResolutionEnabled\":false") != std::string_view::npos &&
        prepared.Json().find("custom-unranked") != std::string_view::npos,
        "aggregate report copies uniform actual completed-row dimensions without promoting the run");
    RtExpectedFrameRecord row{};
    Check(uniform.evidence.TryGetExpectedFrame(300, row) && row.dimensions == d,
        "ledger retains actual dimensions with its exact completed frame identity");
    for (unsigned drift = 1; drift <= 6; ++drift) {
        CompletedOwners owners;
        if (drift != 6) owners.submittedDimensions = d;
        owners.dimensionsDrift = drift; owners.Run();
        Check(CaptureBenchmarkSummary(owners.benchmark, owners.evidence, config, config, kRunUuid).Status() ==
            BenchmarkSummaryStatus::MismatchedPopulation,
            "one intermediate dimension/aspect/DRS change or unavailable observation cannot be hidden by matching endpoints");
    }
    auto wrongMetadata = config; wrongMetadata.metadata.internalWidth = 1280;
    wrongMetadata.metadata.internalHeight = 720; wrongMetadata.metadata.presentationWidth = 1280;
    wrongMetadata.metadata.presentationHeight = 720;
    Check(CaptureBenchmarkSummary(uniform.benchmark, uniform.evidence, wrongMetadata, wrongMetadata, kRunUuid).Status() ==
        BenchmarkSummaryStatus::MismatchedPopulation, "matching requested start/end dimensions cannot replace different actual completed pixels");
    CompletedOwners adaptive; adaptive.submittedDimensions = d;
    adaptive.submittedDimensions->dynamicResolutionEnabled = true; adaptive.Run();
    const auto customAdaptive = PrepareBenchmarkSummaryReport(CaptureBenchmarkSummary(adaptive.benchmark, adaptive.evidence,
        config, config, kRunUuid), Approval());
    Check(customAdaptive.IsReady() && customAdaptive.Json().find("\"dynamicResolutionEnabled\":true") != std::string_view::npos &&
        customAdaptive.Json().find("custom-unranked") != std::string_view::npos,
        "uniform adaptive custom evidence is declared honestly, never labeled a DRS-off standard");
    CompletedOwners historical; historical.Run();
    const auto old = PrepareBenchmarkSummaryReport(CaptureBenchmarkSummary(historical.benchmark, historical.evidence,
        config, config, kRunUuid), Approval());
    Check(old.IsReady() && old.Json().find("\"submittedDimensions\":null") != std::string_view::npos,
        "historical absence remains unavailable instead of copying requested dimensions into evidence");
}

} // namespace

int main(const int argc, const char* const* const argv)
{
    if (argc == 2 && (std::string_view{argv[1]} == "--wire-fixture" || std::string_view{argv[1]} == "--public-wire-fixture"))
    {
        // Synthetic owner evidence through the real immutable preparation API.
        // No device, personal labels or remote consent; optional hardware stays off.
        CompletedOwners owners;
        owners.Run(RtSampleStatus::Valid, false, false, true);
        const auto configuration = Configuration();
        const auto frozen = CaptureBenchmarkSummary(owners.benchmark, owners.evidence,
            configuration, configuration, kRunUuid);
        if (std::string_view{argv[1]} == "--public-wire-fixture") {
            const auto prepared=PreparePublicBenchmarkReport(frozen,
                {true,false,"33333333-3333-4333-a333-333333333333","2026-10-03T00:01:02.003Z"});
            if (!passed || !prepared.IsReady()) return 1;
            std::cout << prepared.Json() << '\n';return 0;
        }
        const auto prepared = PrepareBenchmarkSummaryReport(frozen,
            {true, false, kReportUuid, "2026-10-03T00:01:02.003Z"});
        if (!passed || !prepared.IsReady()) return 1;
        std::cout << prepared.Json() << '\n';
        return 0;
    }
    if (argc != 1) return 2;
    static_assert(std::is_same_v<decltype(std::declval<FrozenBenchmarkSummary>().Data()), const BenchmarkSummaryData&>);
    static_assert(std::is_same_v<decltype(std::declval<PreparedBenchmarkSummaryReport>().Json()), std::string_view>);
    TestCompletedFreezePrivacyAndPopulation();
    TestInvalidOwnersScopeAndIdentity();
    TestUploadedQualitySummary();
    TestOwningDustSummary();
    TestOwningMistSummary();
    TestSubmittedDimensionsSummary();
    TestPublicProjectionConsentPrivacyAndEvidence();
    TestUnavailableHardwarePrivacyAndIdentityRetry();
    return passed ? 0 : 1;
}
