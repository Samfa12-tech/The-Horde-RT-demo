#include <iostream>
#include <string>

#include "gameplay/ShowcaseBenchmark.h"
#include "vulkan/RtCapabilityReport.h"

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
