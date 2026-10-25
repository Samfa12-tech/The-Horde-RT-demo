// Explicit Debug-only finite hidden proof. Current practice rules advance the
// first real charge/discharge/recovery; no phase import or incoming player damage.
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
if (!gShowcaseCaptureHidden || IsWindowVisible(context.windowHandle) ||
    GetForegroundWindow() == context.windowHandle || GetFocus() == context.windowHandle ||
    checkpoints.size() != 1u || context.developmentCheckpoint.empty() || forestProof ||
    context.simulation.Snapshot().playerMountProfile != items::PlayerMountProfile::AnatomicalBody)
    return fail("Keeper cast fixture requires one explicit unshown production-body capture.");
context.simulation.ResetRoute();
context.simulation.BeginCombatPractice(EnemyKind::Lich);
SynchronizeSimulationViewportAspect(context);
context.simulation.ClearEvents();
context.simulation.ResetTiming();
context.simulationPaused = true;
context.simulationInput.paused = true;
context.frameDeltaSeconds = 0;
const auto start = context.simulation.Snapshot();
if (!start.playerAlive || start.lich.health != 3 || start.enemyRoster.selectedEnemy != EnemyKind::Lich ||
    std::hypot(start.playerX - kKeeperRetryPosition.x, start.playerZ - kKeeperRetryPosition.z) > 0.001f)
    return fail("Current Keeper practice did not reach its existing retry camera.");
InputSnapshot input;
input.damageEnabled = false;
input.tutorialEnabled = false;
input.yawRadians = start.playerYawRadians;
input.pitchRadians = -0.05f;
bool neutralSeen = false, chargeSeen = false, dischargeSeen = false, returnedNeutral = false;
unsigned frames = 0, images = 0, warningPulses = 0, damagePulses = 0;
unsigned chargeStartTick = 0, warningTick = 0, dischargeTick = 0, neutralReturnTick = 0;
unsigned chargeCaptureMask = 0, recoveryCaptureMask = 0;
auto previousPhase = start.lich.phase;
const auto began = std::chrono::steady_clock::now();
std::ofstream rows(outputDirectory / "keeper-cast.jsonstream", std::ios::binary);
if (!rows) return fail("Cannot open bounded Keeper cast receipt.");
rows.precision(10);
for (unsigned tick = 0; tick < 360 && !returnedNeutral; ++tick)
{
    if (std::chrono::steady_clock::now() - began > std::chrono::seconds(90))
        return fail("Keeper cast exceeded its finite 90-second capture budget.");
    context.simulation.StepFixed(input);
    const auto before = context.simulation.Snapshot();
    const auto& keeper = before.lich;
    const auto phase = keeper.phase;
    if (!chargeSeen && phase != LichPhase::Charging && phase != LichPhase::Recovering && phase != LichPhase::Dead)
        neutralSeen = true;
    if (phase == LichPhase::Charging && !chargeSeen) {chargeSeen = true; chargeStartTick = tick;}
    if (keeper.dischargeWarningPulse) {++warningPulses; warningTick = tick;}
    if (keeper.damagePulse) ++damagePulses;
    if (chargeSeen && phase == LichPhase::Recovering && !dischargeSeen)
    {dischargeSeen = true; dischargeTick = tick;}
    if (dischargeSeen && phase == LichPhase::MaintainingRange)
    {returnedNeutral = true; neutralReturnTick = tick;}
    MirrorSimulationSnapshot(context);
    if (!context.simulationPaused || context.frameDeltaSeconds != 0.0f || context.developmentCheckpoint.empty())
        return fail("Keeper fixture lost the existing frozen-development render owner.");
    bool presented = false;
    if (!RenderFrame(context, clearColor, presented) || !presented ||
        context.lastFramePresentation != horde::telemetry::RtPresentationOutcome::Presented)
        return fail("Keeper frame lacks successful real RT swapchain presentation.");
    const auto& after = context.simulation.Snapshot();
    if (after.tickIndex != before.tickIndex || after.playerAnimation != before.playerAnimation ||
        after.playerX != before.playerX || after.playerZ != before.playerZ ||
        after.lich.phase != phase || after.lich.phaseTime != keeper.phaseTime)
        return fail("Keeper renderer advanced the frozen owning gameplay pose.");
    if (!CompleteRtEvidenceAfterDeviceIdle(context, vkDeviceWaitIdle(context.device)))
        return fail("Keeper frame lacks successful owning RT completion.");
    const auto published = context.rtFrameEvidence.PublishedStateByValue();
    const auto& complete = published.completedEvidence;
    const auto& identity = complete.identity.submitted;
    if (!published.presented || !published.hasCompletedEvidence ||
        identity.frame.simulationTick != before.tickIndex || identity.frame.sceneEpoch != published.sceneEpoch ||
        identity.frame.measurementGeneration != published.measurementGeneration ||
        !complete.scene.dispatch.sceneReady || !complete.scene.dispatch.rtDispatchRecorded ||
        !complete.scene.dispatch.swapchainCopyRecorded ||
        complete.presentation.outcome != horde::telemetry::RtPresentationOutcome::Presented ||
        identity.submissionSerial != complete.presentation.lastSuccessfulPresentSubmissionSerial)
        return fail("Keeper frame evidence does not own its presented submission.");
    bool milestone = false;
    if (phase == LichPhase::Charging)
    {
        constexpr std::array<float, 4> thresholds{{0.36f, 0.60f, 0.96f, 1.18f}};
        for (unsigned i = 0; i < thresholds.size(); ++i)
            if (!(chargeCaptureMask & (1u << i)) && keeper.phaseTime + 0.00001f >= thresholds[i])
            {chargeCaptureMask |= 1u << i; milestone = true;}
    }
    if (phase == LichPhase::Recovering)
    {
        constexpr std::array<float, 5> thresholds{{0.10f, 0.18f, 0.35f, 0.60f, 0.90f}};
        for (unsigned i = 0; i < thresholds.size(); ++i)
            if (!(recoveryCaptureMask & (1u << i)) && keeper.phaseTime + 0.00001f >= thresholds[i])
            {recoveryCaptureMask |= 1u << i; milestone = true;}
    }
    std::string imageFile, hash, error, evidence;
    if (tick % 3 == 0 || phase != previousPhase || milestone)
    {
        horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture pixels;
        imageFile = "keeper-" + std::to_string(tick) + ".png";
        if (!context.rtScene.CaptureStorageImage(pixels, error) ||
            !WriteRgbaPng(outputDirectory / imageFile, pixels, error) ||
            !Sha256File(outputDirectory / imageFile, hash, error)) return fail(error);
        ++images;
    }
    const auto masks = context.rtScene.LastInstanceMasks();
    const auto flags = context.rtScene.LastPlayerWorldBodyInstanceFlags();
    const bool dedicated = horde::vulkan::raytracing::HasDedicatedPlayerPrimaryOwnership(masks, flags);
    const bool counters = context.rtScene.DiagnosticsAvailability() ==
        horde::vulkan::raytracing::RtDiagnosticAvailability::Available;
    const auto arms = context.rtScene.PrimaryPlayerPixelCount();
    const bool visible = horde::platform::windows::HasExpectedPlayerCaptureVisibility(
        dedicated, context.rtScene.LastPlayerPrimaryVisible(), counters, arms, false, nullptr);
    const auto pose = effects::EvaluateKeeperCastPose(keeper);
    horde::telemetry::RtEvidenceValidationError validation{};
    if (!horde::telemetry::SerializeRtPerformanceEvidenceJson(complete, evidence, validation))
        return fail("Keeper frame serialization rejected actual owning evidence.");
    rows << "{\"motorTick\":" << tick << ",\"simulationTick\":" << before.tickIndex
         << ",\"phase\":" << static_cast<unsigned>(phase) << ",\"phaseTime\":" << keeper.phaseTime
         << ",\"revealComplete\":" << keeper.revealComplete << ",\"readableCast\":" << keeper.readableCastPresentation
         << ",\"warningPulse\":" << keeper.dischargeWarningPulse << ",\"damagePulse\":" << keeper.damagePulse
         << ",\"staffLightStrength\":" << keeper.staffLightStrength
         << ",\"staffLiftRadians\":" << pose.staffLiftRadians << ",\"staffCastRadians\":" << pose.staffCastRadians
         << ",\"upperBodyPitchRadians\":" << pose.upperBodyPitchRadians
         << ",\"file\":\"" << imageFile << "\",\"sha256\":\"" << hash << "\",\"armPixels\":" << arms
         << ",\"dedicatedOwnership\":" << dedicated << ",\"visibilityAccepted\":" << visible
         << ",\"completedFrame\":" << evidence << "}\n";
    rows.flush(); // Failed image/evidence is retained before strict rejection.
    if (!rows || !counters || !visible || arms == 0 || !before.playerAlive || keeper.health != 3 ||
        before.playerVitals.vitality != start.playerVitals.vitality ||
        before.playerVitals.halfHeart != start.playerVitals.halfHeart ||
        before.lastConsumedAttackSequence != start.lastConsumedAttackSequence ||
        before.lastConsumedParrySequence != start.lastConsumedParrySequence ||
        before.lastConsumedDodgeSequence != start.lastConsumedDodgeSequence ||
        std::abs(before.playerX - start.playerX) > 0.001f || std::abs(before.playerZ - start.playerZ) > 0.001f)
        return fail("Keeper capture lost strict primary arms, untouched combat input, fixed camera or vitality.");
    if (chargeSeen && !keeper.readableCastPresentation)
        return fail("Keeper proof silently selected a preserved legacy cast.");
    ++frames;
    previousPhase = phase;
}
if (!neutralSeen || !returnedNeutral || !chargeSeen || !dischargeSeen || warningPulses != 1 || damagePulses != 1 ||
    chargeCaptureMask != 0x0fu || recoveryCaptureMask != 0x1fu ||
    dischargeTick - chargeStartTick != 72 || dischargeTick - warningTick != 18 ||
    neutralReturnTick - dischargeTick != 108)
    return fail("Keeper sequence lacks exact authoritative charge/contact/recovery and all required visual witnesses.");
std::ostringstream summary;
summary << "{\"complete\":true,\"evidenceClass\":\"Debug hidden fixed-input live Keeper practice; incoming damage disabled; not natural victory, balance, performance or phone acceptance\","
        << "\"castStyle\":" << static_cast<unsigned>(effects::kKeeperCastPoseStyle)
        << ",\"frames\":" << frames << ",\"images\":" << images << ",\"fixedSimulationHz\":60,\"unpaced\":true,"
        << "\"chargeStartMotorTick\":" << chargeStartTick << ",\"warningMotorTick\":" << warningTick
        << ",\"dischargeMotorTick\":" << dischargeTick
        << ",\"neutralReturnMotorTick\":" << neutralReturnTick
        << ",\"warningPulses\":1,\"damagePulses\":1}\n";
return WriteReportFile(outputDirectory / "keeper-cast.json", summary.str()) ? 0 : 1;
