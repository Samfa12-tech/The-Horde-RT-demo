#include <iostream>
#include <clocale>
#include <locale>
#include <string>

#include "gameplay/ShowcaseBenchmark.h"
#include "telemetry/RtPerformanceEvidence.h"
#include "telemetry/RtBenchmarkEvidenceRun.h"
#include "vulkan/RtCapabilityReport.h"

namespace
{
class NonJsonNumberPunctuation final : public std::numpunct<char>
{
    char do_decimal_point() const override { return ','; }
    char do_thousands_sep() const override { return '_'; }
    std::string do_grouping() const override { return "\3"; }
};
}

int main()
{
    using namespace horde::gameplay;
    ShowcaseBenchmarkRun benchmark;
    benchmark.Start();
    int frames = 0;
    int warmUpFrames = 0;
    int measuredFrames = 0;
    while (benchmark.IsRunning() && frames < 8000)
    {
        benchmark.Advance();
        const double frameTimeMs = (frames % 100) == 0 ? 30.0 :
                                   (frames % 100) == 1 ? 20.0 : 10.0;
        benchmark.RecordFrame(frameTimeMs, true);
        if (benchmark.CurrentLap() == 1u) ++warmUpFrames;
        else ++measuredFrames;
        ++frames;
    }

    bool passed = true;
    const auto check = [&passed](const bool condition, const char* message) {
        if (!condition)
        {
            std::cerr << "Benchmark smoke failed: " << message << '\n';
            passed = false;
        }
    };
    check(benchmark.Passed(), "two-lap deterministic course must pass");
    check(benchmark.CompletedLaps() == 2u, "two laps must complete");
    check(benchmark.ReachedWaypoints() == 2u * kShowcaseReplayPath.size(),
          "both laps must reach every waypoint");
    check(benchmark.OverallStatistics().frames > 0u, "frame samples must be retained");
    check(warmUpFrames == measuredFrames, "warm-up and measured laps must render symmetric routes");
    check(benchmark.OverallStatistics().onePercentLowFps < 40.0,
          "1% low must average the slowest one percent rather than invert a single percentile");
    check(benchmark.ZoneStatistics(ShowcaseZone::Finale).frames > 0u,
          "finale samples must be represented");

    ShowcaseBenchmarkMetadata metadata;
    metadata.timestampUtc = "2026-07-17T00:00:00Z";
    metadata.buildIdentity = "smoke";
    metadata.shaderIdentity = "shader";
    metadata.gpuName = "test gpu";
    metadata.vulkanApi = "1.4.0";
    metadata.rtMode = "RayTracingPipeline";
    metadata.executionBackend = "RayQueryCompute";
    metadata.materialEncoding = "test materials";
    const std::string text = benchmark.BuildTextReport(metadata);
    const std::string json = benchmark.BuildJsonReport(metadata);
    check(text.find("Integrity: COMPLETE") != std::string::npos, "text report must expose integrity result");
    check(text.find("Laps completed: 2/2") != std::string::npos, "text report must expose lap count");
    check(json.find("\"result\": \"complete\"") != std::string::npos, "JSON report must expose integrity result");
    horde::telemetry::RtBenchmarkEvidenceRun unarmedEvidence;
    check(unarmedEvidence.Start(ShowcaseBenchmarkRun::kMaximumFramesPerLap),
          "report fixture must allocate an unarmed evidence run");
    metadata.legacyFrameTimingScope = "windows-render-plus-rtlab-telemetry";
    const auto incompleteEvidenceText = benchmark.BuildTextReport(metadata, &unarmedEvidence);
    const auto incompleteEvidenceJson = benchmark.BuildJsonReport(metadata, &unarmedEvidence);
    check(incompleteEvidenceText.find("Integrity: INVALID") != std::string::npos &&
              incompleteEvidenceJson.find("\"result\": \"invalid\"") != std::string::npos &&
              incompleteEvidenceJson.find("\"routeTraversalComplete\": true") != std::string::npos,
          "finished gameplay route must not certify unarmed or missing completed-frame evidence");
    check(incompleteEvidenceJson.find("\"schema\": 2") != std::string::npos &&
              incompleteEvidenceJson.find("\"completedFrameEvidence\": {") != std::string::npos &&
              incompleteEvidenceJson.find("windows-render-plus-rtlab-telemetry") != std::string::npos,
          "new evidence must be versioned separately and preserve the named legacy platform clock");
    check(text.find("RT mode: RayTracingPipeline") != std::string::npos &&
              text.find("Execution backend: RayQueryCompute") != std::string::npos,
          "benchmark text must separate raw RT capability mode from actual execution backend");
    check(json.find("\"rtMode\": \"RayTracingPipeline\"") != std::string::npos &&
              json.find("\"executionBackend\": \"RayQueryCompute\"") != std::string::npos,
          "benchmark JSON must separate raw RT capability mode from actual execution backend");

    horde::vulkan::DeviceCapabilities capabilities;
    capabilities.rtMode = horde::vulkan::RtMode::RayTracingPipeline;
    capabilities.rtScene.executionBackend = horde::vulkan::RtExecutionBackend::RayQueryCompute;
    capabilities.rtScene.presented = true;
    const std::string capabilityText =
        horde::vulkan::BuildCapabilityTextReport(capabilities);
    const std::string capabilityJson =
        horde::vulkan::BuildCapabilityJsonReport(capabilities);
    check(capabilityText.find("RT mode: RayTracingPipeline") != std::string::npos &&
              capabilityText.find("Execution backend: RayQueryCompute") != std::string::npos,
          "capability text must preserve raw mode beside the actual scene backend");
    check(capabilityJson.find("\"rtMode\": \"RayTracingPipeline\"") != std::string::npos &&
              capabilityJson.find("\"executionBackend\": \"RayQueryCompute\"") !=
                  std::string::npos &&
              capabilityJson.find("\"presented\": true") != std::string::npos,
          "capability JSON must preserve raw mode, actual scene backend, and presentation proof");
    check(capabilityText.find("RT scene presented: yes") != std::string::npos,
          "capability text must preserve the existing presentation proof");
    check(horde::vulkan::ToString(horde::vulkan::RtExecutionBackend::RayTracingPipeline) ==
                  "RayTracingPipeline" &&
              horde::vulkan::ToString(horde::vulkan::RtExecutionBackend::RayQueryCompute) ==
                  "RayQueryCompute" &&
              horde::vulkan::ToString(horde::vulkan::RtExecutionBackend::Unsupported) ==
                  "Unsupported",
          "execution backends must have stable report identities");
    const std::string unattemptedCapabilityJson =
        horde::vulkan::BuildCapabilityJsonReport(horde::vulkan::DeviceCapabilities{});
    check(unattemptedCapabilityJson.find("\"executionBackend\": \"Unsupported\"") !=
                  std::string::npos &&
              unattemptedCapabilityJson.find("\"presented\": false") != std::string::npos,
          "an unattempted scene must not invent a backend or presentation proof");
    capabilities.rtScene.dispatchWidth = 960u;
    capabilities.rtScene.dispatchHeight = 540u;
    horde::vulkan::BeginRtBackendSelection(
        capabilities.rtScene, horde::vulkan::RtExecutionBackend::RayQueryCompute);
    const std::string selectedNotPresented = horde::vulkan::BuildCapabilityJsonReport(capabilities);
    check(selectedNotPresented.find("\"executionBackend\": \"RayQueryCompute\"") != std::string::npos &&
              selectedNotPresented.find("\"presented\": false") != std::string::npos &&
              capabilities.rtScene.dispatchWidth == 0u && capabilities.rtScene.dispatchHeight == 0u,
          "new selection must preserve its backend before first present and discard prior scene proof");
    horde::vulkan::BeginRtBackendSelection(
        capabilities.rtScene, horde::vulkan::RtExecutionBackend::Unsupported);
    check(capabilities.rtScene.executionBackend == horde::vulkan::RtExecutionBackend::Unsupported &&
              !capabilities.rtScene.presented,
          "rejected backend selection must not retain a previously selected backend");

    auto escapedMetadata = metadata;
    escapedMetadata.buildIdentity = "quote\" slash\\ control";
    escapedMetadata.buildIdentity.push_back('\x01');
    escapedMetadata.internalWidth = 1234u;
    capabilities.identity.gpuName = escapedMetadata.buildIdentity;
    capabilities.identity.vendorId = 1234u;
    capabilities.performance.gpuRt.timestampPeriodNanoseconds = 1250.5f;
    const std::locale previousLocale;
    std::locale::global(std::locale(previousLocale, new NonJsonNumberPunctuation));
    const std::string localizedBenchmark = benchmark.BuildJsonReport(escapedMetadata);
    const std::string localizedCapability = horde::vulkan::BuildCapabilityJsonReport(capabilities);
    std::locale::global(previousLocale);
    check(localizedBenchmark.find("quote\\\" slash\\\\ control\\u0001") != std::string::npos &&
              localizedCapability.find("quote\\\" slash\\\\ control\\u0001") != std::string::npos,
          "both JSON reports must escape quotes, slashes and all low control characters");
    check(localizedBenchmark.find("1_234") == std::string::npos &&
              localizedBenchmark.find("\"width\": 1234") != std::string::npos &&
              localizedCapability.find("\"vendorId\": 1234") != std::string::npos &&
              localizedCapability.find("\"timestampPeriodNanoseconds\": 1250.5") != std::string::npos,
          "JSON numeric output must remain locale-independent rather than use grouped/comma decimals");
    const std::string previousNumericLocale = std::setlocale(LC_NUMERIC, nullptr);
    bool commaLocaleAvailable = std::setlocale(LC_NUMERIC, "French_France.1252") != nullptr;
    if (!commaLocaleAvailable)
        commaLocaleAvailable = std::setlocale(LC_NUMERIC, "fr_FR.UTF-8") != nullptr;
    if (commaLocaleAvailable)
    {
        capabilities.performance.fps = 12.5f;
        capabilities.performance.frameTimeMs = 80.0f;
        capabilities.performance.gpuRt.valid = true;
        capabilities.performance.gpuRt.latestMs = 75.5f;
        capabilities.performance.gpuRt.averageMs = 76.5f;
        const std::string cLocaleCapability = horde::vulkan::BuildCapabilityJsonReport(capabilities);
        std::setlocale(LC_NUMERIC, previousNumericLocale.c_str());
        check(cLocaleCapability.find("\"fps\": 12.500000") != std::string::npos &&
                  cLocaleCapability.find("\"latestMs\": 75.500000") != std::string::npos &&
                  cLocaleCapability.find("\"averageMs\": 76.500000") != std::string::npos,
              "legacy float fields must not inherit the C locale through std::to_string");
    }
    std::cout << "Comma C-locale coverage: " << (commaLocaleAvailable ? "executed" : "unavailable") << '\n';
    horde::telemetry::RtLifecyclePublishedState pendingPublication{};
    pendingPublication.sceneEpoch = 12u;
    pendingPublication.measurementGeneration = 34u;
    pendingPublication.running = true;
    pendingPublication.gpuStatus = horde::telemetry::RtSampleStatus::Pending;
    pendingPublication.diagnosticStatus = horde::telemetry::RtSampleStatus::CompiledOut;
    const auto pendingCapabilityJson =
        horde::vulkan::BuildCapabilityJsonReport(capabilities, &pendingPublication);
    const auto pendingCapabilityText =
        horde::vulkan::BuildCapabilityTextReport(capabilities, &pendingPublication);
    check(pendingCapabilityJson.find("\"rtFrameEvidence\": {\"version\":1") != std::string::npos &&
              pendingCapabilityJson.find("\"sceneEpoch\":12,\"measurementGeneration\":34") != std::string::npos &&
              pendingCapabilityJson.find("\"completedFrame\":null") != std::string::npos &&
              pendingCapabilityText.find("RT EVIDENCE PUBLICATION version=1") != std::string::npos,
          "capability reports must embed the canonical publication rather than reconstruct frame counters");
    check(horde::vulkan::BuildCapabilityJsonReport(capabilities).find(
              "\"observerAvailable\":false") != std::string::npos,
          "probe-only capability reporting must mark joined frame evidence unavailable");

    ShowcaseBenchmarkRun presentationFailure;
    presentationFailure.Start();
    bool injectedFailure = false;
    frames = 0;
    while (presentationFailure.IsRunning() && frames < 8000)
    {
        presentationFailure.Advance();
        const bool presented = injectedFailure || presentationFailure.CurrentLap() < 2u;
        presentationFailure.RecordFrame(10.0, presented);
        if (!presented) injectedFailure = true;
        ++frames;
    }
    check(presentationFailure.Status() == ShowcaseBenchmarkStatus::Complete,
          "course traversal can complete after a presentation-integrity failure");
    check(!presentationFailure.Passed(), "a missing measured RT presentation must invalidate the result");
    check(presentationFailure.BuildTextReport(metadata).find("Integrity: INVALID") != std::string::npos,
          "invalid presentation must be explicit in the text report");

    ShowcaseBenchmarkRun cancelled;
    cancelled.Start(1u);
    cancelled.Advance();
    cancelled.RecordFrame(10.0, true);
    const auto samplesBeforeCancellation = cancelled.Frames().size();
    cancelled.Cancel();
    cancelled.RecordFrame(999.0, true);
    check(cancelled.Status() == ShowcaseBenchmarkStatus::Cancelled && !cancelled.Passed(),
          "cancel must leave an invalid non-running session");
    check(samplesBeforeCancellation == 1u &&
              cancelled.Frames().size() == samplesBeforeCancellation,
          "a cancelled interrupted measurement must reject even a successfully presented interval");

    // Advance can mark the final lap Complete before its frame is presented.
    // Recreate on that final present must still cancel admission of the interval.
    const auto completedSamplesBeforeCancellation = benchmark.Frames().size();
    benchmark.Cancel();
    benchmark.RecordFrame(999.0, true);
    check(benchmark.Status() == ShowcaseBenchmarkStatus::Cancelled && !benchmark.Passed() &&
              benchmark.Frames().size() == completedSamplesBeforeCancellation,
          "presentation interruption must reject the final interval even after replay completion");

    if (!passed)
    {
        return 1;
    }
    std::cout << "Benchmark smoke passed: two-lap course, timing aggregation, and reports.\n";
    return 0;
}
