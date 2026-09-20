#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

#include "gameplay/LanternBenchmarkScenario.h"
#include "gameplay/ShowcaseBenchmark.h"

int main()
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::interactions;
    bool passed = true;
    const auto check = [&](bool value, const char* message) {
        if (!value) { passed = false; std::cerr << message << '\n'; }
    };
    BenchmarkWorkload parsed = BenchmarkWorkload::ShowcaseRoute;
    check(!ParseBenchmarkWorkload("lantern", parsed), "Unknown/ambiguous case must fail closed");
    for (const auto workload : kBenchmarkWorkloads)
    {
        check(ParseBenchmarkWorkload(BenchmarkWorkloadName(workload), parsed) && parsed == workload,
              "Versioned workload identity must round-trip");
        if (!IsLanternBenchmark(workload)) continue;
        simulation::GameSimulation game;
        ShowcaseBenchmarkRun run;
        run.Start(2, workload);
        std::vector<simulation::SimulationSnapshot> warmup;
        unsigned frames = 0;
        unsigned stageCount = 0;
        unsigned phaseMask = 0;
        while (run.IsRunning() && frames < 1300)
        {
            const auto advance = run.Advance();
            if (advance.frameInLap == 1)
            {
                check(StageLanternBenchmark(game, workload), "Scenario must stage via shared simulation");
                ++stageCount;
                const auto& initial = game.Snapshot();
                check(initial.interaction.heldLightKind == HeldLightKind::RewardLantern &&
                      initial.interaction.heldLightPoseProgress == 1.0f &&
                      initial.chestReward.phase == ChestRewardPhase::LanternClaimed &&
                      initial.chestReward.lidOpenProgress == 1.0f && initial.finale.lanternClaimed &&
                      initial.finale.lichDefeated, "Production reward/held-item ownership must agree");
                check(initial.interaction.heldLightPose == (workload == BenchmarkWorkload::LanternHeldLow
                    ? HeldLightPose::Low : HeldLightPose::High), "High/low pose must be gameplay-owned");
                check(initial.lanternPendulum.initialized, "Pendulum must start from an owned hinge");
                for (float value : initial.rewardLanternWorldFromHinge)
                    check(std::isfinite(value), "Hinge transform must be finite");
                for (float value : initial.lanternPendulum.worldFromBody)
                    check(std::isfinite(value), "Pendulum transform must be finite");
                if (workload == BenchmarkWorkload::LanternMotionExtreme)
                    check(initial.lanternPendulum.forwardAngularVelocity == 2.40f &&
                          initial.lanternPendulum.forwardAngleRadians == 0.82f,
                          "Extreme pose must preserve authored physical state");
            }
            simulation::InputSnapshot input{};
            input.damageEnabled = false;
            input.hasAuthoritativePlayerPose = true;
            input.authoritativePlayerX = advance.replay.x;
            input.authoritativePlayerZ = advance.replay.z;
            input.yawRadians = advance.replay.yaw;
            input.pitchRadians = kLanternBenchmarkPitch;
            input.torchLightStrength = 1.8f;
            game.AdvanceFrame(input, IsFrozenBenchmark(workload) ? 0.0 : 1.0/60.0,
                              game.Snapshot().inputPublicationSequence + 1u);
            const auto& snapshot = game.Snapshot();
            check(snapshot.interaction.heldLightKind == HeldLightKind::RewardLantern,
                  "Every measured frame must retain the real lantern");
            if (IsFrozenBenchmark(workload))
                check(snapshot.finale.phase == FinaleSequencePhase::RevealingLantern &&
                      snapshot.finale.skylightOpenProgress == 0.0f,
                      "Static glass cases must not quietly progress into cheap dawn frames");
            phaseMask |= 1u << static_cast<unsigned>(snapshot.finale.phase);
            if (run.CurrentLap() == 1) warmup.push_back(snapshot);
            else
            {
                const auto& original = warmup[advance.frameInLap - 1];
                check(original.finale == snapshot.finale &&
                      original.lanternPendulum.worldFromBody == snapshot.lanternPendulum.worldFromBody &&
                      original.rewardLanternWorldFromHinge == snapshot.rewardLanternWorldFromHinge,
                      "Measured lap must reproduce the warm-up state sequence exactly");
            }
            run.RecordFrame(12.0, true);
            ++frames;
        }
        check(frames == 1200 && stageCount == 2 && run.Passed() &&
              run.OverallStatistics().frames == 600, "Two complete symmetric 600-frame laps required");
        if (workload == BenchmarkWorkload::LanternRevealSequence)
        {
            for (auto phase : {FinaleSequencePhase::RaisingLantern, FinaleSequencePhase::RevealingLantern,
                 FinaleSequencePhase::SkylightOpening, FinaleSequencePhase::DawnRevealed,
                 FinaleSequencePhase::Complete})
                check((phaseMask & (1u << static_cast<unsigned>(phase))) != 0, "Live reveal must cover every phase");
        }
        const auto json = run.BuildJsonReport({});
        check(run.ReachedWaypoints() == 0 &&
              json.find("\"routeTraversalComplete\": false") != std::string::npos &&
              json.find("\"workloadComplete\": true") != std::string::npos,
              "A lantern case must not claim that the corridor route was traversed");
        check(json.find(BenchmarkWorkloadName(workload)) != std::string::npos &&
              json.find(IsFrozenBenchmark(workload) ? "frozen-authored-snapshot" : "fixed-step-60hz") != std::string::npos,
              "Report must distinguish workload and frozen/live simulation");
        game.ResetRoute();
        check(game.Snapshot().interaction.heldLightKind == HeldLightKind::Torch &&
              !game.Snapshot().finale.lanternClaimed && !game.Snapshot().finaleComplete &&
              game.Snapshot().playerX == kPlayerSpawn.x && game.Snapshot().playerZ == kPlayerSpawn.z,
              "Benchmark cancellation cleanup must restore ordinary spawn/torch state");
    }
    ShowcaseBenchmarkRun invalid;
    invalid.Start(2, static_cast<BenchmarkWorkload>(255));
    check(!invalid.IsRunning() && !invalid.Passed(), "Invalid enum must not run as the ordinary route");
    simulation::GameSimulation game;
    check(!StageLanternBenchmark(game, BenchmarkWorkload::ShowcaseRoute), "Route must not silently stage a lantern");
    return passed ? 0 : 1;
}
