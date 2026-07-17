#include <iostream>
#include <string>

#include "gameplay/ShowcaseBenchmark.h"

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
    metadata.materialEncoding = "test materials";
    const std::string text = benchmark.BuildTextReport(metadata);
    const std::string json = benchmark.BuildJsonReport(metadata);
    check(text.find("Integrity: COMPLETE") != std::string::npos, "text report must expose integrity result");
    check(text.find("Laps completed: 2/2") != std::string::npos, "text report must expose lap count");
    check(json.find("\"result\": \"complete\"") != std::string::npos, "JSON report must expose integrity result");

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
    cancelled.Start();
    cancelled.Cancel();
    check(cancelled.Status() == ShowcaseBenchmarkStatus::Cancelled && !cancelled.Passed(),
          "cancel must leave an invalid non-running session");

    if (!passed)
    {
        return 1;
    }
    std::cout << "Benchmark smoke passed: two-lap course, timing aggregation, and reports.\n";
    return 0;
}
