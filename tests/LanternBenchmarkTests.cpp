#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "gameplay/LanternBenchmarkScenario.h"
#include "gameplay/ShowcaseBenchmark.h"

int main()
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::interactions;
    bool passed = true;
    const auto check = [&](const bool value, const char* message) {
        if (!value)
        {
            passed = false;
            std::cerr << message << '\n';
        }
    };

    BenchmarkWorkload parsed = BenchmarkWorkload::ShowcaseRoute;
    check(!ParseBenchmarkWorkload("lantern", parsed),
          "Unknown workload names must fail closed");
    for (const auto workload : kBenchmarkWorkloads)
    {
        check(ParseBenchmarkWorkload(BenchmarkWorkloadName(workload), parsed) &&
                  parsed == workload,
              "Versioned workload names must round-trip");
    }

    ShowcaseBenchmarkRun route;
    route.Start();
    unsigned routeFrames = 0u;
    while (route.IsRunning() && routeFrames < 8000u)
    {
        route.Advance();
        route.RecordFrame(12.0, true);
        ++routeFrames;
    }
    check(route.Passed() && route.CompletedLaps() == 2u,
          "Default route must retain its two-lap integrity contract");
    check(route.ReachedWaypoints() == 2u * kShowcaseReplayPath.size(),
          "Default route must reach every waypoint on both laps");
    check(route.OverallStatistics().frames == 1838u,
          "Default route must retain the 1838-frame measured interval");
    const std::string routeText = route.BuildTextReport({});
    const std::string routeJson = route.BuildJsonReport({});
    check(routeText.find("Preset: deterministic 13-waypoint complete showcase route") !=
              std::string::npos &&
              routeText.find("Waypoints reached: 26/26") != std::string::npos &&
              routeText.find("Measured frames: 1838") != std::string::npos &&
              routeText.find("Workload: ") == std::string::npos &&
              routeText.find("Simulation policy: ") == std::string::npos,
          "Default route text must retain legacy labels and omit lantern metadata");
    check(routeJson.find("\"waypointsReached\": 26") != std::string::npos &&
              routeJson.find("\"measuredFrames\": 1838") != std::string::npos &&
              routeJson.find("\"workload\":") == std::string::npos &&
              routeJson.find("\"simulationPolicy\":") == std::string::npos,
          "Default route JSON must retain legacy counts and omit lantern metadata");

    for (const auto workload : kBenchmarkWorkloads)
    {
        if (!IsLanternBenchmark(workload))
        {
            continue;
        }

        simulation::GameSimulation simulation;
        ShowcaseBenchmarkRun run;
        run.Start(2u, workload);
        std::vector<simulation::SimulationSnapshot> warmup;
        unsigned frames = 0u;
        unsigned stageCount = 0u;
        unsigned phaseMask = 0u;
        while (run.IsRunning() && frames < 1300u)
        {
            const auto advance = run.Advance();
            if (advance.frameInLap == 1u)
            {
                check(StageLanternBenchmark(simulation, workload),
                      "Lantern workload must stage through shared simulation");
                ++stageCount;
                const auto& initial = simulation.Snapshot();
                check(initial.interaction.heldLightKind == HeldLightKind::RewardLantern &&
                          initial.interaction.heldLightPoseProgress == 1.0f &&
                          initial.chestReward.phase == ChestRewardPhase::LanternClaimed &&
                          initial.chestReward.lidOpenProgress == 1.0f &&
                          initial.finale.lanternClaimed && initial.finale.lichDefeated,
                      "Lantern staging must preserve production ownership");
                check(initial.interaction.heldLightPose ==
                              (workload == BenchmarkWorkload::LanternHeldLow
                                   ? HeldLightPose::Low : HeldLightPose::High),
                      "High/low pose must remain gameplay-owned");
                check(initial.lanternPendulum.initialized,
                      "Lantern pendulum must start from its owned hinge");
                for (const float value : initial.rewardLanternWorldFromHinge)
                {
                    check(std::isfinite(value), "Hinge transform must be finite");
                }
                for (const float value : initial.lanternPendulum.worldFromBody)
                {
                    check(std::isfinite(value), "Pendulum transform must be finite");
                }
                if (workload == BenchmarkWorkload::LanternMotionExtreme)
                {
                    check(initial.lanternPendulum.forwardAngularVelocity == 2.40f &&
                              initial.lanternPendulum.forwardAngleRadians == 0.82f,
                          "Extreme authored pendulum state must be preserved");
                }
            }

            simulation::InputSnapshot input{};
            input.damageEnabled = false;
            input.hasAuthoritativePlayerPose = true;
            input.authoritativePlayerX = advance.replay.x;
            input.authoritativePlayerZ = advance.replay.z;
            input.yawRadians = advance.replay.yaw;
            input.pitchRadians = kLanternBenchmarkPitch;
            input.torchLightStrength = 1.8f;
            simulation.AdvanceFrame(
                input,
                IsFrozenBenchmark(workload) ? 0.0 : 1.0 / 60.0,
                simulation.Snapshot().inputPublicationSequence + 1u);
            const auto& snapshot = simulation.Snapshot();
            check(snapshot.interaction.heldLightKind == HeldLightKind::RewardLantern,
                  "Every benchmark frame must retain the real lantern");
            if (IsFrozenBenchmark(workload))
            {
                check(snapshot.finale.phase == FinaleSequencePhase::RevealingLantern &&
                          snapshot.finale.skylightOpenProgress == 0.0f,
                      "Frozen cases must not drift into completed-finale frames");
            }
            phaseMask |= 1u << static_cast<unsigned>(snapshot.finale.phase);
            if (run.CurrentLap() == 1u)
            {
                warmup.push_back(snapshot);
            }
            else
            {
                const auto& original = warmup[advance.frameInLap - 1u];
                check(original.finale == snapshot.finale &&
                          original.lanternPendulum.worldFromBody ==
                              snapshot.lanternPendulum.worldFromBody &&
                          original.rewardLanternWorldFromHinge ==
                              snapshot.rewardLanternWorldFromHinge,
                      "Measured lap must reproduce the warm-up state sequence");
            }
            run.RecordFrame(12.0, true);
            ++frames;
        }

        check(frames == 1200u && stageCount == 2u && run.Passed() &&
                  run.OverallStatistics().frames == kLanternBenchmarkFramesPerLap,
              "Lantern workloads require two complete 600-frame laps");
        if (workload == BenchmarkWorkload::LanternRevealSequence)
        {
            for (const auto phase : {FinaleSequencePhase::RaisingLantern,
                                     FinaleSequencePhase::RevealingLantern,
                                     FinaleSequencePhase::SkylightOpening,
                                     FinaleSequencePhase::DawnRevealed,
                                     FinaleSequencePhase::Complete})
            {
                check((phaseMask & (1u << static_cast<unsigned>(phase))) != 0u,
                      "Live reveal must cover every finale phase");
            }
        }
        const std::string json = run.BuildJsonReport({});
        const std::string text = run.BuildTextReport({});
        const std::string workloadName(BenchmarkWorkloadName(workload));
        const std::string simulationPolicy = IsFrozenBenchmark(workload)
            ? "frozen-authored-snapshot" : "fixed-step-60hz";
        check(run.ReachedWaypoints() == 0u &&
                  json.find("\"routeTraversalComplete\": false") != std::string::npos &&
                  json.find("\"workloadComplete\": true") != std::string::npos &&
                  json.find("\"workload\": \"" + workloadName + "\"") !=
                      std::string::npos &&
                  json.find("\"simulationPolicy\": \"" + simulationPolicy + "\"") !=
                      std::string::npos &&
                  text.find("Workload: " + workloadName + "\n") != std::string::npos &&
                  text.find("Simulation policy: " + simulationPolicy + "\n") !=
                      std::string::npos,
              "Lantern reports must identify the exact workload and policy");
        check(!StageLanternBenchmark(simulation, BenchmarkWorkload::ShowcaseRoute),
              "Ordinary route must not silently stage a lantern");
    }

    ShowcaseBenchmarkRun invalid;
    invalid.Start(2u, static_cast<BenchmarkWorkload>(255u));
    check(!invalid.IsRunning() && !invalid.Passed(),
          "Invalid workload enum must fail closed");
    return passed ? 0 : 1;
}
