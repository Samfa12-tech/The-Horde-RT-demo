#include "gameplay/validation/MotionEvidenceScenario.h"
#include "telemetry/MotionEvidenceLedger.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

namespace
{
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
using namespace horde::gameplay::validation;
using namespace horde::telemetry;
int failures = 0;
void Check(bool condition, const char* message)
{
    if (!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}

void Run(MotionScenario kind, int rate, const char* receiptPath)
{
    GameSimulation simulation(ProductionGameSimulationConfig());
    MotionEvidenceScenario scenario;
    MotionEvidenceLedger ledger;
    std::uint64_t now = 1'000'000'000ull;
    Check(scenario.Begin(kind, simulation, now), "actual production scenario starts from an admitted checkpoint seed");
    Check(ledger.Begin("cpu_only_input_evidence", kind), "bounded ledger accepts a safe local run ID");
    InputSnapshot publication;
    // No current RT output: same production paused-input path, no commands.
    const auto notReady = scenario.BuildInput(simulation.Snapshot(), publication, now, false);
    simulation.AdvanceFrame(notReady, 1.0 / rate, 1u);
    scenario.ObserveAdvance(simulation.Snapshot(), simulation.Events().Events());
    Check(simulation.Snapshot().tickIndex == 0u && notReady.paused && !notReady.hasAuthoritativePlayerPose &&
          notReady.commands.attack == 0u && scenario.SimulationSeconds() == 0.0,
          "unpresented output defers live simulation input instead of forcing frame time");
    simulation.ClearEvents();
    bool pauseChecked = false;
    bool walkingSeen = false;
    bool retryRecognitionSeen = false;
    bool chargeSeen = false;
    std::uint64_t publicationSequence = 1u;
    for (int frame = 0; frame < rate * 120 && !scenario.Complete() && !scenario.Failed(); ++frame)
    {
        now += static_cast<std::uint64_t>(1'000'000'000ull / static_cast<unsigned>(rate));
        if (kind == MotionScenario::KeeperFirstEntry && !pauseChecked && simulation.Snapshot().lich.revealElapsedSeconds > 2.0f)
        {
            publication.paused = true;
            const auto before = simulation.Snapshot().lich.revealElapsedSeconds;
            const auto beforeCommands = publication.commands;
            const auto beforeTime = scenario.SimulationSeconds();
            for (int pausedFrame = 0; pausedFrame < rate; ++pausedFrame)
            {
                now += static_cast<std::uint64_t>(1'000'000'000ull / static_cast<unsigned>(rate));
                auto input = scenario.BuildInput(simulation.Snapshot(), publication, now, true);
                simulation.AdvanceFrame(input, 1.0 / rate, ++publicationSequence);
                scenario.ObserveAdvance(simulation.Snapshot(), simulation.Events().Events());
                Check(simulation.Snapshot().lich.revealElapsedSeconds == before && scenario.SimulationSeconds() == beforeTime &&
                      input.commands.attack == beforeCommands.attack,
                      "real paused publication freezes keeper clock and scheduled edges");
                simulation.ClearEvents();
            }
            publication.paused = false;
            pauseChecked = true;
        }
        const auto input = scenario.BuildInput(simulation.Snapshot(), publication, now, true);
        Check(!input.hasAuthoritativePlayerPose && std::abs(input.moveForward) <= 1.00001f && std::abs(input.moveStrafe) <= 1.00001f,
              "measured motion uses only bounded normal input axes");
        simulation.AdvanceFrame(input, 1.0 / rate, ++publicationSequence);
        scenario.ObserveAdvance(simulation.Snapshot(), simulation.Events().Events());
        if (!ledger.AppendState(now, simulation.Snapshot(), input, scenario, simulation.Events().Events()))
        { std::cerr << ledger.Failure() << '\n'; Check(false, "actual state/event ledger admission"); break; }
        publication = input;
        walkingSeen = walkingSeen || simulation.Snapshot().walkAmount > 0.5f;
        retryRecognitionSeen = retryRecognitionSeen || simulation.Snapshot().lich.revealPhase == KeeperRevealPhase::RetryRecognition;
        chargeSeen = chargeSeen || simulation.Snapshot().lich.phase == LichPhase::Charging;
        simulation.ClearEvents();
    }
    std::cout << MotionScenarioName(kind) << " rate=" << rate << " stage=" << MotionStageName(scenario.Stage())
              << " simulationSeconds=" << scenario.SimulationSeconds() << " states=" << ledger.States().size()
              << " events=" << ledger.Events().size() << " reservedLedgerBytes=" << ledger.ReservedBytes()
              << " reason=" << scenario.Failure() << '\n';
    Check(scenario.Complete() && !ledger.Failed() && walkingSeen, "real simulation reaches the bounded scenario outcome");
    if (kind == MotionScenario::KeeperFirstEntry)
        Check(pauseChecked && chargeSeen && scenario.EventCounts()[17] == 1u && scenario.EventCounts()[18] == 1u && scenario.EventCounts()[19] == 1u,
              "six-second keeper motion includes pause, ordered once-only cues and a full actual charge");
    if (kind == MotionScenario::KeeperRetryReward)
        Check(retryRecognitionSeen && chargeSeen && simulation.Snapshot().retryGeneration == 1u &&
              scenario.EventCounts()[10] == 1u && scenario.EventCounts()[13] == 1u && scenario.EventCounts()[14] == 1u &&
              scenario.EventCounts()[15] == 1u && scenario.EventCounts()[11] == 1u && scenario.EventCounts()[19] == 2u &&
              scenario.EventCounts()[6] == 3u,
              "actual death/retry/accepted hits/chest/claim/finale remain single ordered semantic transitions");
    if (receiptPath != nullptr)
    {
        std::ofstream file(receiptPath);
        ledger.WriteJson(file, scenario);
        Check(file.good(), "CPU-only audit receipt streams to the explicitly supplied local path");
    }
}

void TestAdmissionAndFrameBinding()
{
    MotionScenario parsed{};
    Check(ParseMotionScenario("keeper-first-entry", parsed) && parsed == MotionScenario::KeeperFirstEntry &&
          !ParseMotionScenario("keeper-first-entry ", parsed) && !ParseMotionScenario("../torch-low-opening", parsed),
          "exact scenario parser cannot silently broaden unsupported launch input");
    GameSimulation legacy;
    MotionEvidenceScenario rejected;
    Check(!rejected.Begin(MotionScenario::TorchLowOpening, legacy, 1u), "legacy profile cannot certify the production anatomical motion path");
    GameSimulation simulation(ProductionGameSimulationConfig());
    MotionEvidenceScenario scenario;
    Check(scenario.Begin(MotionScenario::TorchLowOpening, simulation, 1'000u), "deadline fixture admitted");
    auto input = scenario.BuildInput(simulation.Snapshot(), {}, 999u, true);
    Check(scenario.Failed() && input.paused, "regressed monotonic clock fails closed");
    Check(scenario.Begin(MotionScenario::TorchLowOpening, simulation, 1'000u), "deadline fixture restarted before measured evidence");
    input = scenario.BuildInput(simulation.Snapshot(), {}, 1'000u + MotionEvidenceScenario::kMaximumWallNanoseconds + 1u, true);
    Check(scenario.Failed() && input.paused, "wall deadline remains bounded when frames or input stall");

    Check(scenario.Begin(MotionScenario::TorchLowOpening, simulation, 1'000u), "identity fixture starts");
    MotionEvidenceLedger ledger;
    Check(!ledger.Begin("unsafe/id", MotionScenario::TorchLowOpening), "unsafe IDs cannot enter local report paths");
    Check(ledger.Begin("owned_identity_fixture", MotionScenario::TorchLowOpening), "identity ledger reset");
    input = scenario.BuildInput(simulation.Snapshot(), {}, 1'001u, true);
    simulation.AdvanceFrame(input, 1.0 / 60.0, 1u);
    scenario.ObserveAdvance(simulation.Snapshot(), simulation.Events().Events());
    Check(ledger.AppendState(1'001u, simulation.Snapshot(), input, scenario, simulation.Events().Events()), "actual current state row admitted");
    simulation.ClearEvents();
    // Actual evidence lifecycle protocol with CPU-only injected graphics facts.
    // This fixture owns no Vulkan device and establishes no RT rendering pass.
    RtEvidenceLifecycle lifecycle;
    RtLifecycleResetEffects effects;
    RtLifecycleSeeds seeds; seeds.sceneEpoch = 1u; seeds.measurementGeneration = 1u;
    Check(lifecycle.Initialise(seeds, 3u, RtSampleStatus::CompiledOut, RtSampleStatus::Disabled, effects), "existing evidence lifecycle initializes");
    const auto initialScope = lifecycle.PublishedStateByValue();
    Check(ledger.ObserveScope(5u, initialScope.sceneEpoch, initialScope.measurementGeneration), "actual initial graphics scope observed");
    RtFrameToken token;
    Check(lifecycle.BeginRecord(0u, simulation.Snapshot().tickIndex, token), "existing owner creates record token");
    RtRecordedSceneEvidence recorded;
    recorded.dispatch = {true, true, true};
    Check(AssignRtFixedText(recorded.pipeline.bundleKey, "cpu_protocol_fixture_pair") &&
          AssignRtFixedText(recorded.pipeline.opaqueFast.key, "cpu_protocol_fixture") &&
          AssignRtFixedText(recorded.pipeline.genericDielectric.key, "cpu_protocol_fixture_generic") &&
          AssignRtFixedText(recorded.pipeline.opaqueFast.sha256, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa") &&
          AssignRtFixedText(recorded.pipeline.genericDielectric.sha256, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"), "fixture shader identity");
    recorded.pipeline.active = recorded.pipeline.opaqueFast;
    Check(lifecycle.FinishRecord(token, recorded, token), "existing owner finishes record");
    RtSubmittedFrameIdentity submitted;
    Check(lifecycle.Submit(token, submitted), "existing owner commits submission identity");
    Check(ledger.BindSubmittedFrame(5u, submitted), "ledger binds exact committed identity to current state row");
    Check(ledger.HasPendingSubmissions(), "submitted graphics work remains visibly pending before owning completion");
    Check(lifecycle.AttachPresentation(submitted, RtPresentationOutcome::Presented), "existing owner attaches presentation fact");
    RtSubmittedStageSample stages; stages.identity = submitted; stages.stages.status = RtSampleStatus::Valid;
    RtDiagnosticEvidence diagnostic; diagnostic.status = RtSampleStatus::CompiledOut;
    RtGpuTimingEvidence gpu; gpu.status = RtSampleStatus::Disabled;
    RtPerformanceEvidenceSnapshot completed;
    Check(lifecycle.CompleteFence(submitted, stages, diagnostic, gpu, completed), "existing owning completion protocol accepts exact submission");
    auto publication = lifecycle.PublishedStateByValue();
    Check(ledger.AppendCompletedFrame(5u, publication) && ledger.AppendCompletedFrame(5u, publication) && ledger.Frames().size() == 1u &&
          !ledger.HasPendingSubmissions(),
          "exact completed frame joins once despite duplicate output polls");
    Check(ledger.HasCurrentPresentedFrame(5u, publication.sceneEpoch, publication.measurementGeneration) &&
          !ledger.HasCurrentPresentedFrame(6u, publication.sceneEpoch, publication.measurementGeneration), "new surface generation cannot inherit prior readiness");
    auto staleLedger = ledger;
    auto stalePublication = publication; stalePublication.sceneEpoch += 1u;
    Check(!staleLedger.AppendCompletedFrame(5u, stalePublication), "stale old epoch completion cannot certify undeclared newly allocated RT resources");

    // A real measurement reset retains submitted frames. A later accepted old
    // completion stays historical; it cannot establish readiness in new scope.
    RtFrameToken oldAttempt, oldRecorded;
    RtSubmittedFrameIdentity oldSubmitted;
    Check(lifecycle.BeginRecord(1u, simulation.Snapshot().tickIndex, oldAttempt) &&
          lifecycle.FinishRecord(oldAttempt, recorded, oldRecorded) && lifecycle.Submit(oldRecorded, oldSubmitted) &&
          ledger.BindSubmittedFrame(5u, oldSubmitted) && lifecycle.AttachPresentation(oldSubmitted, RtPresentationOutcome::Presented),
          "actual owner retains an outstanding old-scope accepted presentation");
    Check(lifecycle.ApplyEvent(RtLifecycleEvent::Retry, effects), "actual normal retry changes measurement scope");
    publication = lifecycle.PublishedStateByValue();
    Check(ledger.ObserveScope(5u, publication.sceneEpoch, publication.measurementGeneration) &&
          !ledger.HasCurrentPresentedFrame(5u, publication.sceneEpoch, publication.measurementGeneration),
          "retry explicitly closes previous output readiness without dropping accepted evidence");
    stages.identity = oldSubmitted;
    Check(lifecycle.CompleteFence(oldSubmitted, stages, diagnostic, gpu, completed) &&
          ledger.AppendCompletedFrame(5u, lifecycle.PublishedStateByValue()) && ledger.Frames().size() == 2u &&
          !ledger.HasCurrentPresentedFrame(5u, publication.sceneEpoch, publication.measurementGeneration),
          "actual old accepted completion is retained historically and never certifies current retry resources");

    // Resource recreation really cancels the exact unpresented pending record.
    RtFrameToken cancelledAttempt, cancelledRecorded;
    RtSubmittedFrameIdentity cancelled;
    Check(lifecycle.BeginRecord(2u, simulation.Snapshot().tickIndex, cancelledAttempt) &&
          lifecycle.FinishRecord(cancelledAttempt, recorded, cancelledRecorded) && lifecycle.Submit(cancelledRecorded, cancelled) &&
          ledger.BindSubmittedFrame(5u, cancelled), "actual owner has a distinct unpresented pending submission");
    Check(lifecycle.Recreate(RtResourceResetReason::RenderScaleChange, RtSampleStatus::CompiledOut, RtSampleStatus::Disabled, effects),
          "actual owning resource recreation retires its pending slots");
    publication = lifecycle.PublishedStateByValue();
    Check(ledger.ObserveScope(5u, publication.sceneEpoch, publication.measurementGeneration), "recreated current scope observed");
    auto mismatchLedger = ledger;
    auto mismatch = cancelled; ++mismatch.submissionSerial;
    Check(!mismatchLedger.RetireSubmittedFrame(5u, mismatch), "mismatched retirement cannot free another owned submission");
    Check(ledger.RetireSubmittedFrame(5u, cancelled) && ledger.Frames().size() == 2u,
          "exact actual cancellation releases the old binding without adding an RT success");
    auto duplicateRetirementLedger = ledger;
    Check(!duplicateRetirementLedger.RetireSubmittedFrame(5u, cancelled), "duplicate retirement cannot invent a second cancellation");

    RtFrameToken newAttempt, newRecorded;
    RtSubmittedFrameIdentity newSubmitted;
    Check(lifecycle.BeginRecord(2u, simulation.Snapshot().tickIndex, newAttempt) &&
          lifecycle.FinishRecord(newAttempt, recorded, newRecorded) && lifecycle.Submit(newRecorded, newSubmitted) &&
          ledger.BindSubmittedFrame(5u, newSubmitted) && lifecycle.AttachPresentation(newSubmitted, RtPresentationOutcome::Presented),
          "slot reuses only after the actual owner and exact old binding both retired");
    auto illegalCurrentRetirement = ledger;
    Check(!illegalCurrentRetirement.RetireSubmittedFrame(5u, newSubmitted), "current pending output cannot be disguised as obsolete cancellation");
    stages.identity = newSubmitted;
    Check(lifecycle.CompleteFence(newSubmitted, stages, diagnostic, gpu, completed) &&
          ledger.AppendCompletedFrame(5u, lifecycle.PublishedStateByValue()) &&
          ledger.HasCurrentPresentedFrame(5u, publication.sceneEpoch, publication.measurementGeneration),
          "only newly accepted completed output restores readiness after real scope recreation");

    std::ostringstream json;
    ledger.WriteJson(json, scenario);
    Check(json.str().find("cpu_protocol_fixture") != std::string::npos &&
          json.str().find("not-established-by-this-ledger") != std::string::npos,
          "streamed report retains exact identity and explicit audio acceptance limitation");
    MotionEvidenceLedger unbound;
    Check(unbound.Begin("unbound_fixture", MotionScenario::TorchLowOpening) &&
          unbound.AppendState(1'001u, simulation.Snapshot(), input, scenario, {}) &&
          unbound.ObserveScope(5u, publication.sceneEpoch, publication.measurementGeneration), "unbound negative fixture admitted state and actual scope only");
    publication = lifecycle.PublishedStateByValue();
    Check(!unbound.AppendCompletedFrame(5u, publication), "last state and presentation flag alone cannot fabricate a committed frame join");
}
}

int main(int argc, char** argv)
{
    if (argc != 1 && argc != 3) { std::cerr << "Optional --cpu-receipt path\n"; return 2; }
    if (argc == 3 && std::string_view(argv[1]) != "--cpu-receipt") return 2;
    TestAdmissionAndFrameBinding();
    for (int rate : {15, 60, 120})
        for (const auto kind : {MotionScenario::TorchLowOpening, MotionScenario::ShaftUp,
                               MotionScenario::KeeperFirstEntry, MotionScenario::KeeperRetryReward})
            Run(kind, rate, argc == 3 && rate == 60 && kind == MotionScenario::KeeperRetryReward ? argv[2] : nullptr);
    return failures == 0 ? 0 : 1;
}
