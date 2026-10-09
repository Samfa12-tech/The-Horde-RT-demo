#include "gameplay/validation/MotionEvidenceScenario.h"
#include "telemetry/MotionEvidenceLedger.h"
#include "platform/android/AndroidMotionEvidencePolicy.h"
#include "scene/ShowcaseOverheadGeometry.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
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
bool UpwardRayEntersWaterShaft(const SimulationSnapshot& state)
{
    // Independent actual shader ray and physical roof-patch intersection,
    // including production camera bob. Elapsed time alone cannot certify the
    // waterfall rim when the camera is in the other chamber or finale room.
    const float step=state.walkTime*6.2f;
    const float pitch=std::clamp(state.playerPitchRadians+std::sin(step)*0.012f*state.walkAmount,-0.32f,0.28f)-0.05f;
    const float slope=(1.22f*pitch+0.592f)/(1.22f-0.592f*pitch);
    if (slope<=0.0f) return false;
    const float eye=kShowcaseEyeWorldY+std::abs(std::sin(step))*0.035f*state.walkAmount;
    const float distance=(horde::scene::kShowcaseRouteCeilingWorldY-eye)/slope;
    const float x=state.playerX+std::sin(step*0.5f)*0.035f*state.walkAmount+std::sin(state.playerYawRadians)*distance;
    const float z=state.playerZ-std::cos(state.playerYawRadians)*distance;
    if (x<=-2.9f || x>=-1.58f || z<=-16.1f || z>=-14.72f) return false;
    for (std::size_t patch=8;patch<12;++patch)
    {
        const auto& roof=horde::scene::kShowcaseCeilingPatches[patch];
        float minX=roof.footprint[0][0],maxX=minX,minZ=roof.footprint[0][1],maxZ=minZ;
        for (const auto& corner:roof.footprint)
        { minX=std::min(minX,corner[0]);maxX=std::max(maxX,corner[0]);minZ=std::min(minZ,corner[1]);maxZ=std::max(maxZ,corner[1]); }
        if (x>=minX && x<=maxX && z>=minZ && z<=maxZ) return false;
    }
    return state.torchFailure.heldByPlayer && !state.torchFailure.triggered &&
        state.interaction.heldLightKind==interactions::HeldLightKind::Torch;
}

void Run(MotionScenario kind, int rate, const char* receiptPath)
{
    GameSimulation simulation(ProductionGameSimulationConfig());
    MotionEvidenceScenario scenario;
    MotionEvidenceLedger ledger;
    std::uint64_t now = 1'000'000'000ull;
    Check(scenario.Begin(kind, simulation, now), "actual production scenario starts from an admitted checkpoint seed");
    if (kind==MotionScenario::ShaftUp)
        Check(simulation.Snapshot().torchFailure.heldByPlayer && !simulation.Snapshot().torchFailure.triggered &&
              simulation.Snapshot().interaction.heldLightKind==interactions::HeldLightKind::Torch,
              "water shaft route starts from an admitted fresh torch checkpoint before ordinary movement");
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
    bool rearViewSeen=false,rearParallaxSeen=false,rearReturnSeen=false;
    float shaftMinZ=simulation.Snapshot().playerZ,shaftMaxZ=shaftMinZ;
    unsigned shaftViewSamples=0;
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
        auto input = scenario.BuildInput(simulation.Snapshot(), publication, now, true);
        Check(!input.hasAuthoritativePlayerPose && std::abs(input.moveForward) <= 1.00001f && std::abs(input.moveStrafe) <= 1.00001f,
              "measured motion uses only bounded normal input axes");
        // Match the Android owner: ordinary world command admission is zero
        // delta, followed by a paused seed while the new RT scope is pending.
        const bool retrySeed = input.commands.retry > simulation.Snapshot().lastConsumedRetrySequence;
        if (retrySeed)
        {
            simulation.StepFixed(input, 0.0f, ++publicationSequence);
            input.paused = true;
            input.moveForward = input.moveStrafe = 0.0f;
        }
        simulation.AdvanceFrame(input, 1.0 / rate, ++publicationSequence);
        scenario.ObserveAdvance(simulation.Snapshot(), simulation.Events().Events());
        if (!ledger.AppendState(now, simulation.Snapshot(), input, scenario, simulation.Events().Events()))
        { std::cerr << ledger.Failure() << '\n'; Check(false, "actual state/event ledger admission"); break; }
        if (retrySeed && !scenario.Failed())
        {
            auto corruptObserver = scenario;
            auto corruptState = simulation.Snapshot();
            corruptState.lich.revealElapsedSeconds += 0.02f;
            corruptObserver.ObserveAdvance(corruptState, {});
            Check(corruptObserver.Failed() && corruptObserver.Failure() == "Keeper reveal advanced across paused input.",
                  "admitted zero-delta retry does not allow subsequent reveal-clock advancement while paused");
        }
        publication = input;
        if (retrySeed) publication.paused = false; // The platform's UI tuple remains unpaused.
        walkingSeen = walkingSeen || simulation.Snapshot().walkAmount > (kind==MotionScenario::ShaftUp ? 0.1f : 0.5f);
        retryRecognitionSeen = retryRecognitionSeen || simulation.Snapshot().lich.revealPhase == KeeperRevealPhase::RetryRecognition;
        chargeSeen = chargeSeen || simulation.Snapshot().lich.phase == LichPhase::Charging;
        if (kind==MotionScenario::TorchLowOpening && scenario.Stage()==MotionStage::Approach)
        {
            const auto& state=simulation.Snapshot();
            rearViewSeen=rearViewSeen || (state.playerZ>2.5f && std::cos(state.playerYawRadians)< -0.95f && state.torchFailure.heldByPlayer);
            rearParallaxSeen=rearParallaxSeen || (state.playerZ>2.5f && state.playerX>0.15f && state.walkAmount>0.1f);
            rearReturnSeen=rearReturnSeen || (rearViewSeen && rearParallaxSeen &&
                std::hypot(state.playerX-kPlayerSpawn.x,state.playerZ-kPlayerSpawn.z)<0.06f);
        }
        if (kind==MotionScenario::ShaftUp && !simulation.Snapshot().paused &&
            (scenario.Stage()==MotionStage::ShaftMotion || scenario.Complete()))
        {
            const auto& state=simulation.Snapshot();
            Check(UpwardRayEntersWaterShaft(state),"ordinary water shaft motion keeps actual upper-screen ray clear of fixed surrounding roof slabs");
            if (shaftViewSamples==0) shaftMinZ=shaftMaxZ=state.playerZ;
            shaftMinZ=std::min(shaftMinZ,state.playerZ);shaftMaxZ=std::max(shaftMaxZ,state.playerZ);++shaftViewSamples;
        }
        simulation.ClearEvents();
    }
    std::cout << MotionScenarioName(kind) << " rate=" << rate << " stage=" << MotionStageName(scenario.Stage())
              << " endpoint=(" << simulation.Snapshot().playerX << ',' << simulation.Snapshot().playerZ << ')'
              << " simulationSeconds=" << scenario.SimulationSeconds() << " states=" << ledger.States().size()
              << " events=" << ledger.Events().size() << " reservedLedgerBytes=" << ledger.ReservedBytes()
              << " reason=" << scenario.Failure() << '\n';
    Check(scenario.Complete() && !ledger.Failed() && walkingSeen, "real simulation reaches the bounded scenario outcome");
    if (kind==MotionScenario::TorchLowOpening)
        Check(rearViewSeen && rearParallaxSeen && rearReturnSeen && scenario.SimulationSeconds()>24.0,
              "torch run first observes actual rear collapse look and positive-Z parallax, returns to spawn, then completes unchanged portal sweep");
    if (kind==MotionScenario::ShaftUp)
    {
        std::cout<<"shaft coverage views="<<shaftViewSamples<<" zSpan="<<shaftMaxZ-shaftMinZ
            <<" retry="<<simulation.Snapshot().retryGeneration<<" finaleRoof="<<simulation.Snapshot().finale.skylightOpenProgress<<'\n';
        Check(shaftViewSamples>static_cast<unsigned>(rate)*10u && shaftMaxZ-shaftMinZ>0.40f &&
              simulation.Snapshot().retryGeneration==0u && simulation.Snapshot().finale.skylightOpenProgress==0.0f,
              "shaft evidence includes sustained waterfall-rim views and actual walking parallax without drench/retry/phase mutation");
    }
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

void TestEquipmentEventAdmission()
{
    // The observer must preserve newly appended ordinary equipment events.
    // These are observer-only fixtures, not manufactured gameplay evidence.
    for (const auto type : {GameplayEventType::PlayerSwordDrawStarted,
                            GameplayEventType::PlayerSwordAttachmentChanged,
                            GameplayEventType::SkeletonEncounterWarning,
                            static_cast<GameplayEventType>(24u)})
    {
        GameSimulation simulation(ProductionGameSimulationConfig());
        MotionEvidenceScenario scenario;
        MotionEvidenceLedger ledger;
        Check(scenario.Begin(MotionScenario::TorchLowOpening, simulation, 1u) &&
              ledger.Begin("equipment_event_fixture", MotionScenario::TorchLowOpening),
              "equipment observer fixture admitted");
        const std::array<GameplayEvent, 1u> events{{{.sequence = 1u, .tickIndex = 1u, .type = type}}};
        scenario.ObserveAdvance(simulation.Snapshot(), events);
        const bool accepted = ledger.AppendState(2u, simulation.Snapshot(), {}, scenario, events);
        const bool known = type != static_cast<GameplayEventType>(24u);
        Check(accepted == known && scenario.Failed() != known,
              "draw, attachment and waterfall warning events admitted; unknown event rejected");
        if (known && accepted && !scenario.Failed())
        {
            Check(scenario.EventCounts()[static_cast<std::size_t>(type)] == 1u &&
                  ledger.Events().size() == 1u && ledger.Events()[0].event.type == type,
                  "new equipment event retained exactly once with its original type");
            scenario.ObserveAdvance(simulation.Snapshot(), events);
            Check(scenario.Failed() && !ledger.AppendState(3u, simulation.Snapshot(), {}, scenario, events),
                  "duplicate equipment event remains rejected");
        }
    }
}

void TestSwordTransitionLedgerIsolation()
{
    GameSimulation simulation(ProductionGameSimulationConfig());
    MotionEvidenceScenario scenario;
    Check(scenario.Begin(MotionScenario::TorchLowOpening, simulation, 1u),
          "immutable equipment metadata fixture has an admitted seed");
    auto state = simulation.Snapshot();
    state.heldItems[1].transition.active = true;
    state.heldItems[1].transition.kind = items::HeldItemTransitionKind::Draw;
    state.heldItems[1].transition.progress = 0.5f;
    MotionEvidenceLedger ledger;
    Check(ledger.Begin("sword_metadata_fixture", MotionScenario::TorchLowOpening) &&
          ledger.AppendState(2u, state, {}, scenario, {}), "valid equipment metadata row admitted");
    state.heldItems[1].transition.progress = 0.75f;
    std::ostringstream json;
    ledger.WriteJson(json, scenario);
    Check(ledger.States().size() == 1u && ledger.States()[0].sword.active &&
          ledger.States()[0].sword.progress == 0.5f &&
          json.str().find("\"progress\":0.5") != std::string::npos,
          "later mutable poses cannot rewrite the captured state row's draw progress");
    for (float invalid : {-0.1f, 1.1f, std::numeric_limits<float>::quiet_NaN()})
    {
        MotionEvidenceLedger rejected;
        Check(rejected.Begin("invalid_sword_metadata", MotionScenario::TorchLowOpening), "negative equipment fixture starts");
        state.heldItems[1].transition.progress = invalid;
        Check(!rejected.AppendState(2u, state, {}, scenario, {}) && rejected.States().empty(),
              "out-of-range or nonfinite draw progress cannot enter completed-frame metadata");
    }
}

void TestAdmissionAndFrameBinding()
{
    MotionScenario parsed{};
    Check(ParseMotionScenario("keeper-first-entry", parsed) && parsed == MotionScenario::KeeperFirstEntry &&
          !ParseMotionScenario("keeper-first-entry ", parsed) && !ParseMotionScenario("../torch-low-opening", parsed),
          "exact scenario parser cannot silently broaden unsupported launch input");
    Check(ParseMotionScenario("torch-drench", parsed) && parsed == MotionScenario::TorchDrench &&
          !ParseMotionScenario("torch-drench/extra", parsed),
          "torch-drench is one exact allowlisted Debug validation scenario");
    GameSimulation legacy;
    MotionEvidenceScenario rejected;
    Check(!rejected.Begin(MotionScenario::TorchLowOpening, legacy, 1u), "legacy profile cannot certify the production anatomical motion path");
    GameSimulation wrongShaft(ProductionGameSimulationConfig());
    MotionEvidenceScenario shaft;
    Check(shaft.Begin(MotionScenario::ShaftUp,wrongShaft,1u) && wrongShaft.ApplyShowcaseCheckpoint(11),
          "rejected finale-roof negative fixture uses actual admitted checkpoint state");
    auto shaftInput=shaft.BuildInput(wrongShaft.Snapshot(),{},2u,true);
    wrongShaft.AdvanceFrame(shaftInput,1.0/60.0,1u);
    shaft.ObserveAdvance(wrongShaft.Snapshot(),wrongShaft.Events().Events());
    Check(shaft.Failed() && !UpwardRayEntersWaterShaft(wrongShaft.Snapshot()),
          "the accepted open finale roof cannot certify the different active-torch waterfall shaft");
    Check(wrongShaft.ApplyShowcaseCheckpoint(4) && !UpwardRayEntersWaterShaft(wrongShaft.Snapshot()),
          "old dark skylight chamber cannot certify the waterfall rim either");
    for (const bool invalidSword : {false, true})
    {
        GameSimulation held(ProductionGameSimulationConfig());
        MotionEvidenceScenario contract;
        std::uint64_t heldNow = 1'000'000u;
        Check(contract.Begin(MotionScenario::KeeperFirstEntry, held, heldNow),
              "intro hold negative fixture starts the real ordinary approach");
        InputSnapshot heldInput;
        for (int tick = 0; tick < 600 && contract.Stage() == MotionStage::Approach; ++tick)
        {
            heldNow += 16'666'667u;
            heldInput = contract.BuildInput(held.Snapshot(), heldInput, heldNow, true);
            held.AdvanceFrame(heldInput, 1.0 / 60.0);
            contract.ObserveAdvance(held.Snapshot(), held.Events().Events());
            held.ClearEvents();
        }
        Check(contract.Stage() == MotionStage::Reveal, "negative fixture reaches genuine awakening");
        heldNow += 16'666'667u;
        heldInput = contract.BuildInput(held.Snapshot(), heldInput, heldNow, true);
        held.AdvanceFrame(heldInput, 1.0 / 60.0);
        auto rejectedState = held.Snapshot();
        // Deliberately corrupt only the observer input. Gameplay state is
        // untouched; neither a moving nor swinging actor may be certified.
        if (invalidSword) rejectedState.playerCombat.action = PlayerCombatAction::SwingWindup;
        else rejectedState.playerX += 0.01f;
        contract.ObserveAdvance(rejectedState, held.Events().Events());
        Check(contract.Failed() && contract.Failure() ==
              "Keeper presentation did not hold translation and player actions.",
              "motion evidence rejects a reveal that moves or swings despite valid timing");
    }
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
    Check(ledger.States()[0].simulationFireEmitterCount == simulation.Snapshot().fireEmitterCount &&
          ledger.States()[0].simulationFireSources[1].stableId == 3u &&
          ledger.States()[0].simulationFireSources[2].stableId == 4u &&
          ledger.States()[0].simulationFireSources[1].strength == 0.0f &&
          ledger.States()[0].simulationFireSources[2].strength == 0.0f &&
          ledger.States()[0].simulationFireSources[3].stableId == 0u,
          "opening motion state retains three configured sources including dormant flank IDs without inventing an active light");
    auto malformedState = simulation.Snapshot();
    malformedState.fireEmitterCount = 5u;
    auto malformedLedger = ledger;
    Check(!malformedLedger.AppendState(1'001u, malformedState, input, scenario, {}) &&
          malformedLedger.States().size() == 1u,
          "oversized configured count rejects before indexing or adding a state row");
    malformedState = simulation.Snapshot();
    malformedState.fireEmitters[2].stableId = malformedState.fireEmitters[1].stableId;
    malformedLedger = ledger;
    Check(!malformedLedger.AppendState(1'001u, malformedState, input, scenario, {}) &&
          malformedLedger.States().size() == 1u,
          "duplicate configured IDs cannot fabricate two independently observed torches");
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
    recorded.fireLighting = RtFireLightingEvidence{};
    recorded.fireLighting->count = 2u;
    recorded.fireLighting->emitters[0] = {3u, {{-32.5f, 0.85f, -16.5f, 0.5f}}, {{1.0f, 0.5f, 0.25f, 0.5f}}};
    recorded.fireLighting->emitters[1] = {4u, {{-31.5f, 0.85f, -13.5f, 0.75f}}, {{1.0f, 0.5f, 0.25f, 0.75f}}};
    const auto owningLighting = recorded.fireLighting;
    recorded.actualUploadedMistEnabled = false;
    Check(lifecycle.FinishRecord(token, recorded, token), "existing owner finishes record");
    RtSubmittedFrameIdentity submitted;
    Check(lifecycle.Submit(token, submitted), "existing owner commits submission identity");
    recorded.actualUploadedMistEnabled = true;
    recorded.fireLighting = RtFireLightingEvidence{}; // later current upload must not overwrite committed copy
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
    Check(ledger.Frames()[0].actualUploadedMistEnabled == std::optional<bool>{false} &&
          ledger.Frames()[0].fireLighting == owningLighting &&
          ledger.Frames()[0].fireLighting != recorded.fireLighting,
          "motion completed row retains only its exact submitted upload despite a later getter changing");
    std::ostringstream fireJson;
    ledger.WriteJson(fireJson, scenario);
    Check(fireJson.str().find("\"actualUploadedMistEnabled\":false") != std::string::npos &&
          fireJson.str().find("\"fireLighting\":{\"count\":2") != std::string::npos &&
          fireJson.str().find("\"simulationFireEmitterCount\":") != std::string::npos &&
          fireJson.str().find("\"strength\":") != std::string::npos,
          "motion JSON separates configured source strength/fuel/ID from completed upload selection");
    auto invalidLightingLedger = ledger;
    auto invalidLightingPublication = publication;
    invalidLightingPublication.completedEvidence.scene.fireLighting->emitters[1].stableId = 3u;
    Check(!invalidLightingLedger.AppendCompletedFrame(5u, invalidLightingPublication) &&
          invalidLightingLedger.Frames().size() == 1u,
          "even a duplicate completion cannot smuggle invalid uploaded IDs into the motion rows");
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
    const horde::platform::android::AndroidMotionEvidenceScope beforeRetryScope{
        5u, oldSubmitted.frame.sceneEpoch, oldSubmitted.frame.measurementGeneration, 720u, 1490u};
    Check(horde::platform::android::AndroidMotionRetryScopeValid(beforeRetryScope,
              {5u, publication.sceneEpoch, publication.measurementGeneration, 720u, 1490u}, true, false),
          "Android retry admission agrees with the actual shared lifecycle's measurement-only reset");
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
    Check(!horde::platform::android::AndroidMotionRetryScopeValid(beforeRetryScope,
              {5u, publication.sceneEpoch, publication.measurementGeneration, 720u, 1490u}, true, false),
          "a real resource recreation cannot be passed as an ordinary retry");
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

void RunTorchDrench(int rate)
{
    GameSimulation simulation(ProductionGameSimulationConfig());
    MotionEvidenceScenario scenario;
    std::uint64_t now = 2'000'000'000ull;
    const auto retryBefore = simulation.Snapshot().retryGeneration;
    Check(scenario.Begin(MotionScenario::TorchDrench, simulation, now),
          "torch-drench begins from its admitted checkpoint-2 route");
    const auto checkpoint = simulation.Snapshot();
    Check(checkpoint.torchFailure.phase == TorchFailurePhase::Held &&
          checkpoint.torchFailure.heldByPlayer && !checkpoint.torchFailure.triggered &&
          checkpoint.interaction.heldLightKind == interactions::HeldLightKind::Torch &&
          checkpoint.retryGeneration == retryBefore && checkpoint.playerAlive,
          "checkpoint-2 starts with the production held torch and no retry or phase mutation");
    MotionEvidenceLedger ledger;
    Check(ledger.Begin("torch_drench_fields", MotionScenario::TorchDrench) &&
          ledger.AppendState(now, checkpoint, {}, scenario, {}),
          "real pre-trigger checkpoint state is admitted to the torch-drench ledger");
    Check(ObservedTorchCaptureMilestones(checkpoint.torchFailure) == 0u,
          "untriggered checkpoint cannot claim any torch capture milestone");

    InputSnapshot publication;
    bool pauseChecked = false;
    bool sawGuttering = false;
    bool sawFalling = false;
    bool sawSettled = false;
    bool sawReleasedFlamelessTorch = false;
    double inspectionStableSince = -1.0;
    std::uint64_t publicationSequence = 1u;
    for (int frame = 0; frame < rate * 20 && !scenario.Complete() && !scenario.Failed(); ++frame)
    {
        now += static_cast<std::uint64_t>(1'000'000'000ull / static_cast<unsigned>(rate));
        if (!pauseChecked && scenario.Stage() == MotionStage::Approach &&
            simulation.Snapshot().playerZ < -12.0f)
        {
            publication.paused = true;
            const auto before = simulation.Snapshot();
            const auto beforeSeconds = scenario.SimulationSeconds();
            for (int i = 0; i < std::max(1, rate / 4); ++i)
            {
                now += static_cast<std::uint64_t>(1'000'000'000ull / static_cast<unsigned>(rate));
                const auto input = scenario.BuildInput(simulation.Snapshot(), publication, now, true);
                simulation.AdvanceFrame(input, 1.0 / rate, ++publicationSequence);
                scenario.ObserveAdvance(simulation.Snapshot(), simulation.Events().Events());
                Check(ledger.AppendState(now, simulation.Snapshot(), input, scenario, simulation.Events().Events()),
                      "paused torch-drench state joins the actual ledger row");
                simulation.ClearEvents();
            }
            Check(simulation.Snapshot().playerX == before.playerX &&
                  simulation.Snapshot().playerZ == before.playerZ &&
                  simulation.Snapshot().torchFailure.triggered == before.torchFailure.triggered &&
                  scenario.SimulationSeconds() == beforeSeconds,
                  "published pause interrupts the route without advancing movement or drench timing");
            publication.paused = false;
            pauseChecked = true;
        }

        const auto input = scenario.BuildInput(simulation.Snapshot(), publication, now, true);
        Check(!input.hasAuthoritativePlayerPose && std::abs(input.moveForward) <= 1.00001f &&
              std::abs(input.moveStrafe) <= 1.00001f,
              "torch-drench uses bounded ordinary input axes without a forced player pose");
        simulation.AdvanceFrame(input, 1.0 / rate, ++publicationSequence);
        scenario.ObserveAdvance(simulation.Snapshot(), simulation.Events().Events());
        Check(ledger.AppendState(now, simulation.Snapshot(), input, scenario, simulation.Events().Events()),
              "actual torch-drench simulation state joins its ledger row");
        const auto& torch = simulation.Snapshot().torchFailure;
        sawGuttering = sawGuttering || torch.phase == TorchFailurePhase::Guttering;
        sawFalling = sawFalling || torch.phase == TorchFailurePhase::Falling;
        sawSettled = sawSettled || torch.phase == TorchFailurePhase::Settled;
        sawReleasedFlamelessTorch = sawReleasedFlamelessTorch ||
            (torch.triggered && !torch.heldByPlayer && torch.flameStrength == 0.0f && torch.fallProgress > 0.0f);
        const auto& inspectedState = simulation.Snapshot();
        const bool stationaryInspection = std::hypot(inspectedState.playerX + 5.5f,
                                                       inspectedState.playerZ + 15.2f) <= 0.06f &&
            inspectedState.walkAmount <= 0.05f && std::abs(inspectedState.inputMoveForward) <= 0.0001f &&
            std::abs(inspectedState.inputMoveStrafe) <= 0.0001f;
        if (stationaryInspection)
        {
            if (inspectionStableSince < 0.0) inspectionStableSince = scenario.SimulationSeconds();
        }
        else inspectionStableSince = -1.0;
        simulation.ClearEvents();
        publication = input;
    }
    const auto& final = simulation.Snapshot();
    std::cout << "torch-drench rate=" << rate << " stage=" << MotionStageName(scenario.Stage())
              << " endpoint=(" << final.playerX << ',' << final.playerZ << ") simulationSeconds="
              << scenario.SimulationSeconds() << " reason=" << scenario.Failure() << '\n';
    Check(pauseChecked && scenario.Complete() && !scenario.Failed() &&
          scenario.EventCounts()[static_cast<std::size_t>(GameplayEventType::TorchExtinguished)] == 1u &&
          final.torchFailure.phase == TorchFailurePhase::Settled && !final.torchFailure.heldByPlayer &&
          final.interaction.heldLightKind == interactions::HeldLightKind::None && final.playerAlive,
          "torch-drench observes one automatic event, actual release, floor settle and held-light loss");
    Check(std::hypot(final.playerX + 5.5f, final.playerZ + 15.2f) <= 0.06f &&
          final.walkAmount <= 0.05f && std::abs(final.inputMoveForward) <= 0.0001f &&
          std::abs(final.inputMoveStrafe) <= 0.0001f && inspectionStableSince >= 0.0 &&
          scenario.SimulationSeconds() - inspectionStableSince >= 0.35,
          "scenario finishes only after ordinary step-back reaches the farther floor-inspection endpoint and remains stationary for 0.35 seconds");
    Check(sawGuttering && sawFalling && sawSettled && sawReleasedFlamelessTorch,
          "host run at each display cadence samples genuine guttering, falling, settled and world-owned flame-off states");
    const unsigned settledMask = ObservedTorchCaptureMilestones(final.torchFailure);
    constexpr unsigned armAndFallThresholds = (1u << 1u) | (1u << 2u) | (1u << 3u) |
        (1u << 5u) | (1u << 6u) | (1u << 7u);
    Check((settledMask & armAndFallThresholds) == armAndFallThresholds &&
          (settledMask & ((1u << 0u) | (1u << 4u))) == 0u &&
          (settledMask & ((1u << 8u) | (1u << 9u))) == ((1u << 8u) | (1u << 9u)),
          "one catch-up settled observation reports all crossed arm/fall thresholds without inventing guttering or falling phases");

    const auto& row = ledger.States().back();
    std::ostringstream json;
    ledger.WriteJson(json, scenario);
    std::ostringstream expectedItem;
    expectedItem << "\"itemParent\":" << static_cast<int>(row.torchParent) << ",\"itemPosition\":["
                 << std::setprecision(9) << row.torchItemPosition[0] << ',' << row.torchItemPosition[1]
                 << ',' << row.torchItemPosition[2] << ']';
    Check(row.torchParent == items::HeldItemParentMode::AuthoredWorldTrajectory &&
          row.torchItemPosition[0] == final.heldItems[0].worldFromItem[12] &&
          row.torchItemPosition[1] == final.heldItems[0].worldFromItem[13] &&
          row.torchItemPosition[2] == final.heldItems[0].worldFromItem[14] &&
          json.str().find(expectedItem.str()) != std::string::npos &&
          json.str().find("\"flameStrength\":0") != std::string::npos,
          "real settled row JSON preserves world trajectory attachment, resolved item position and flame strength");

    auto malformed = final;
    malformed.heldItems[0].worldFromItem[12] = std::numeric_limits<float>::quiet_NaN();
    MotionEvidenceLedger invalidPosition;
    Check(invalidPosition.Begin("invalid_torch_position", MotionScenario::TorchDrench) &&
          !invalidPosition.AppendState(now + 1u, malformed, publication, scenario, {}) &&
          invalidPosition.States().empty(),
          "non-finite resolved world item position is rejected before row capture");
    malformed = final;
    malformed.torchFailure.flameStrength = 1.01f;
    MotionEvidenceLedger invalidStrength;
    Check(invalidStrength.Begin("invalid_torch_strength", MotionScenario::TorchDrench) &&
          !invalidStrength.AppendState(now + 1u, malformed, publication, scenario, {}) &&
          invalidStrength.States().empty(),
          "out-of-range torch flame strength is rejected before row capture");
}

void TestTorchDrenchFailureBeforeTrigger()
{
    GameSimulation simulation(ProductionGameSimulationConfig());
    MotionEvidenceScenario scenario;
    Check(scenario.Begin(MotionScenario::TorchDrench, simulation, 3'000'000'000ull),
          "negative drench fixture starts from the ordinary checkpoint");
    auto injected = simulation.Snapshot();
    injected.torchFailure.triggered = true;
    injected.torchFailure.heldByPlayer = false;
    injected.torchFailure.phase = TorchFailurePhase::Falling;
    injected.interaction.heldLightKind = interactions::HeldLightKind::None;
    scenario.ObserveAdvance(injected, {});
    Check(scenario.Failed() && scenario.Failure() == "Automatic torch-drench trigger lacked its single semantic event.",
          "trigger state without the production event fails before it can become accepted evidence");

    MotionEvidenceScenario duplicate;
    Check(duplicate.Begin(MotionScenario::TorchDrench, simulation, 3'000'000'003ull),
          "duplicate event fixture starts from the ordinary checkpoint");
    std::array<GameplayEvent, 2u> repeated{};
    repeated[0].sequence = 1u;
    repeated[1].sequence = 2u;
    for (auto& event : repeated) event.type = GameplayEventType::TorchExtinguished;
    duplicate.ObserveAdvance(injected, repeated);
    Check(duplicate.Failed() && duplicate.Failure() == "Automatic torch-drench emitted a duplicate extinguish event.",
          "duplicate drench semantic edges cannot certify a single torch drop");

    MotionEvidenceScenario interrupted;
    Check(interrupted.Begin(MotionScenario::TorchDrench, simulation, 3'000'000'001ull),
          "interruption fixture starts from the ordinary checkpoint");
    interrupted.Fail("injected owner interruption");
    const auto stopped = interrupted.BuildInput(simulation.Snapshot(), {}, 3'000'000'002ull, true);
    Check(interrupted.Failed() && stopped.paused && stopped.moveForward == 0.0f && stopped.moveStrafe == 0.0f,
          "interrupted owner publishes a paused zero-axis input");
}
void TestRearLookCannotBeSkipped()
{
    GameSimulation simulation(ProductionGameSimulationConfig());
    MotionEvidenceScenario scenario;
    std::uint64_t now=1'000'000'000ull;
    Check(scenario.Begin(MotionScenario::TorchLowOpening,simulation,now),"rear-look negative starts an ordinary production route");
    InputSnapshot publication;
    bool rearTravelSeen=false,portalStageSeen=false;
    for(unsigned frame=0;frame<60u*50u && !scenario.Failed() && !scenario.Complete();++frame)
    {
        now+=16'666'667ull;
        auto input=scenario.BuildInput(simulation.Snapshot(),publication,now,true);
        // Preserve ordinary world movement but omit the rear camera look.
        // Positive-Z travel alone must not certify a collapse-view milestone.
        const float dx=input.moveForward*std::sin(input.yawRadians)+input.moveStrafe*std::cos(input.yawRadians);
        const float dz=-input.moveForward*std::cos(input.yawRadians)+input.moveStrafe*std::sin(input.yawRadians);
        input.yawRadians=0;input.moveForward=-dz;input.moveStrafe=dx;
        simulation.AdvanceFrame(input,1.0/60.0,frame+1u);
        scenario.ObserveAdvance(simulation.Snapshot(),simulation.Events().Events());
        rearTravelSeen=rearTravelSeen || simulation.Snapshot().playerZ>2.5f;
        portalStageSeen=portalStageSeen || scenario.Stage()==MotionStage::TorchMotion;
        simulation.ClearEvents();publication=input;
    }
    Check(rearTravelSeen && scenario.Failed() && !scenario.Complete() && !portalStageSeen,
          "real rearward travel without rearward look fails instead of silently passing the collapse evidence stage");
}

void TestWaterfallEquipmentSeedOwnership(int rate)
{
    Check(horde::gameplay::IsRouteAudioObstructed(-2.5f, -10.4f,
              kWaterfallSkeletonPairCenter.x, kWaterfallSkeletonPairCenter.z) &&
          !horde::gameplay::IsRouteAudioObstructed(-2.5f, -14.5f,
              kWaterfallSkeletonPairCenter.x, kWaterfallSkeletonPairCenter.z),
          "shared route LOS predicate distinguishes a blocked approach ray from the open waterfall corridor");
    const auto productionConfig = ProductionGameSimulationConfig();
    Check(productionConfig.swordStartsStowed && productionConfig.waterfallSkeletonEncounter,
          "production starts stowed and retains the accepted waterfall guards");
    GameSimulation normalReference(productionConfig);
    normalReference.ResetRoute();
    const auto normalSnapshot = normalReference.Snapshot();
    GameSimulation simulation(productionConfig);
    const auto matchesNormalEncounter = [&]()
    {
        const auto& current = simulation.Snapshot();
        return current.skeletonEnemyCount == normalSnapshot.skeletonEnemyCount &&
            current.activeSkeletonCount == normalSnapshot.activeSkeletonCount &&
            current.skeletonEnemies[0].id == normalSnapshot.skeletonEnemies[0].id &&
            current.skeletonEnemies[0].health == normalSnapshot.skeletonEnemies[0].health &&
            current.skeletonEnemies[1].id == normalSnapshot.skeletonEnemies[1].id &&
            current.skeletonEnemies[1].health == normalSnapshot.skeletonEnemies[1].health;
    };
    InputSnapshot baseline;
    baseline.commands.attack = 2u;
    baseline.commands.parry = 3u;
    baseline.commands.dodge = 4u;
    simulation.StepFixed(baseline);
    const auto before = simulation.Snapshot();
    const auto eventSequenceFloor = simulation.Events().NextSequence();

    MotionEvidenceScenario scenario;
    Check(scenario.Begin(MotionScenario::WaterfallEquipment, simulation, 1'000'000'000u) &&
          scenario.OwnsEquipmentSeed(), "waterfall seed is scoped to its owning scenario");
    const auto seeded = simulation.Snapshot();
    Check(seeded.tickIndex == before.tickIndex && seeded.lastConsumedAttackSequence >= before.lastConsumedAttackSequence &&
          seeded.lastConsumedParrySequence >= before.lastConsumedParrySequence &&
          seeded.lastConsumedDodgeSequence >= before.lastConsumedDodgeSequence &&
          seeded.skeletonEnemyCount == 2u && seeded.activeSkeletonCount == 2u &&
          seeded.skeletonEnemies[0].id == EntityId::SkeletonA && seeded.skeletonEnemies[1].id == EntityId::SkeletonB &&
          seeded.heldItems[1].parentMode == items::HeldItemParentMode::BodyStow,
          "temporary checkpoint preserves monotonic command/tick floors and stages only the two waterfall guards");

    Check(!scenario.Begin(MotionScenario::WaterfallEquipment, simulation, 2'000'000'000u) &&
          simulation.Snapshot().skeletonEnemyCount == 2u && scenario.OwnsEquipmentSeed(),
          "repeated begin cannot erase the active scenario's cleanup ownership");

    InputSnapshot publication;
    const auto paused = scenario.BuildInput(simulation.Snapshot(), publication, 1'000'000'000u, false);
    simulation.AdvanceFrame(paused, 1.0 / 60.0, 1u);
    scenario.ObserveAdvance(simulation.Snapshot(), simulation.Events().Events());
    Check(paused.paused && simulation.Snapshot().tickIndex == before.tickIndex,
          "equipment route stays paused until the owner supplies current output readiness");
    simulation.ClearEvents();

    std::uint64_t now = 1'000'000'000u;
    std::uint64_t warning = 0u, draw = 0u, attachment = 0u, swing = 0u;
    bool farFromCueNoWarning = true, sawDrawTransition = false, sawHandAttachment = false;
    bool sawSwing = false, sawParry = false, warningHadLos = false;
    for (int frame = 0; frame < rate * 120 && !scenario.Complete() && !scenario.Failed(); ++frame)
    {
        now += static_cast<std::uint64_t>(1'000'000'000ull / static_cast<unsigned>(rate));
        auto input = scenario.BuildInput(simulation.Snapshot(), publication, now, true);
        simulation.AdvanceFrame(input, 1.0 / rate, static_cast<std::uint64_t>(frame + 2));
        const auto& state = simulation.Snapshot();
        const auto& sword = state.heldItems[1];
        const float distance = std::hypot(state.playerX - kWaterfallSkeletonPairCenter.x,
                                          state.playerZ - kWaterfallSkeletonPairCenter.z);
        if (distance > horde::gameplay::kWaterfallSwordCueRadius && warning == 0u)
            farFromCueNoWarning = farFromCueNoWarning &&
                scenario.EventCounts()[static_cast<std::size_t>(GameplayEventType::SkeletonEncounterWarning)] == 0u;
        sawDrawTransition = sawDrawTransition || (sword.parentMode == items::HeldItemParentMode::BodyStow &&
            sword.transition.kind == items::HeldItemTransitionKind::Draw && sword.transition.active);
        sawHandAttachment = sawHandAttachment || (sword.parentMode == items::HeldItemParentMode::HandSocket &&
            !sword.transition.active && attachment != 0u);
        sawSwing = sawSwing || state.playerCombat.action == PlayerCombatAction::SwingActive;
        sawParry = sawParry || state.playerCombat.action == PlayerCombatAction::ParryActive;
        for (const auto& event : simulation.Events().Events())
        {
            if (event.type == GameplayEventType::SkeletonEncounterWarning && warning == 0u)
            { warning = event.sequence; warningHadLos = distance <= horde::gameplay::kWaterfallSwordCueRadius &&
                !horde::gameplay::IsRouteAudioObstructed(state.playerX, state.playerZ,
                    kWaterfallSkeletonPairCenter.x, kWaterfallSkeletonPairCenter.z); }
            if (event.type == GameplayEventType::PlayerSwordDrawStarted && draw == 0u) draw = event.sequence;
            if (event.type == GameplayEventType::PlayerSwordAttachmentChanged && attachment == 0u) attachment = event.sequence;
            if (event.type == GameplayEventType::PlayerSwing && swing == 0u) swing = event.sequence;
        }
        scenario.ObserveAdvance(state, simulation.Events().Events());
        simulation.ClearEvents();
        publication = input;
    }

    Check(scenario.Complete() && !scenario.Failed(), "ordinary waterfall approach, draw, swing and parry complete");
    Check(farFromCueNoWarning && warningHadLos && warning != 0u && warning < draw && draw < attachment &&
          attachment < swing, "no early warning; actual range warning and ordered draw/attachment/swing events observed");
    Check(sawDrawTransition && sawHandAttachment && sawSwing && sawParry,
          "moving state samples include stowed draw transition, hand attachment and active swing/parry poses");
    const auto beforeEnd = simulation.Snapshot();
    const auto eventFloorBeforeEnd = simulation.Events().NextSequence();
    scenario.End(simulation);
    scenario.End(simulation);
    Check(!scenario.OwnsEquipmentSeed() && matchesNormalEncounter() &&
          simulation.Snapshot().heldItems[1].parentMode == items::HeldItemParentMode::BodyStow &&
          simulation.Snapshot().tickIndex == beforeEnd.tickIndex &&
          simulation.Snapshot().lastConsumedAttackSequence >= beforeEnd.lastConsumedAttackSequence &&
          simulation.Snapshot().lastConsumedParrySequence >= beforeEnd.lastConsumedParrySequence &&
          simulation.Snapshot().lastConsumedDodgeSequence >= beforeEnd.lastConsumedDodgeSequence &&
          simulation.Events().NextSequence() >= std::max(eventSequenceFloor, eventFloorBeforeEnd),
          "idempotent release restores the constructed normal route while preserving tick and sequence floors");

    MotionEvidenceScenario afterRelease;
    Check(afterRelease.Begin(MotionScenario::WaterfallEquipment, simulation, now + 1u),
          "released owner permits a later bounded equipment run");
    afterRelease.End(simulation);
    Check(matchesNormalEncounter() && simulation.Snapshot().heldItems[1].parentMode ==
          items::HeldItemParentMode::BodyStow,
          "failure-free early release also restores the ordinary production equipment and encounter state");

    MotionEvidenceScenario failedOwner;
    Check(failedOwner.Begin(MotionScenario::WaterfallEquipment, simulation, now + 2u) &&
          failedOwner.OwnsEquipmentSeed(), "post-seed failure fixture acquires cleanup ownership");
    failedOwner.Fail("injected owner interruption after seed");
    failedOwner.End(simulation);
    Check(failedOwner.Failed() && !failedOwner.OwnsEquipmentSeed() &&
          matchesNormalEncounter() &&
          simulation.Snapshot().heldItems[1].parentMode == items::HeldItemParentMode::BodyStow,
          "failure after seed still restores production route flags and equipment before a later Play");
}
}

int main(int argc, char** argv)
{
    if (argc != 1 && argc != 3) { std::cerr << "Optional --cpu-receipt path\n"; return 2; }
    if (argc == 3 && std::string_view(argv[1]) != "--cpu-receipt") return 2;
    TestEquipmentEventAdmission();
    TestSwordTransitionLedgerIsolation();
    TestAdmissionAndFrameBinding();
    TestTorchDrenchFailureBeforeTrigger();
    TestRearLookCannotBeSkipped();
    for (int rate : {15, 30, 60, 120})
    {
        TestWaterfallEquipmentSeedOwnership(rate);
        RunTorchDrench(rate);
        for (const auto kind : {MotionScenario::TorchLowOpening, MotionScenario::ShaftUp,
                               MotionScenario::KeeperFirstEntry, MotionScenario::KeeperRetryReward,
                               MotionScenario::WaterfallEquipment})
            Run(kind, rate, argc == 3 && rate == 60 && kind == MotionScenario::KeeperRetryReward ? argv[2] : nullptr);
    }
    return failures == 0 ? 0 : 1;
}
