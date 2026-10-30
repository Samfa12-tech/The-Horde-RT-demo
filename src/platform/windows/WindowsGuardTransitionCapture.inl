// Included only inside the explicit Debug hidden-showcase guard fixture branch.
// Fixed inputs exercise ordinary production combat, not imported combat phases.
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
if (!gShowcaseCaptureHidden || IsWindowVisible(context.windowHandle) ||
    GetForegroundWindow() == context.windowHandle || GetFocus() == context.windowHandle ||
    checkpoints.size() != 1u || context.developmentCheckpoint.empty() || forestProof ||
    context.simulation.Snapshot().playerMountProfile != items::PlayerMountProfile::AnatomicalBody)
    return fail("Guard transition needs one explicit unshown production-body capture.");

// Legacy checkpoint imports deliberately disable current combat. ResetRoute is
// the existing New Game path: no private phase/health/progression mutation.
context.simulation.ResetRoute();
bool mediaPreview = false;
if (const char* media = std::getenv("HORDE_GUARD_MEDIA_CAPTURE"))
{
    if (std::string_view(media) != "1") return fail("Guard media preview accepts only explicit value 1.");
    mediaPreview = true;
    // Existing large-skylight staging, through the supported diagnostic pose
    // input. No imported combat phase, progression, lighting or quality change.
    InputSnapshot staging;
    staging.damageEnabled = false; staging.tutorialEnabled = false;
    staging.hasAuthoritativePlayerPose = true;
    staging.authoritativePlayerX = -6.50f; staging.authoritativePlayerZ = -15.20f;
    staging.yawRadians = 1.57079632679f; staging.pitchRadians = -0.05f;
    context.simulation.StepFixed(staging);
    const auto& positioned = context.simulation.Snapshot();
    if (std::hypot(positioned.playerX + 6.50f, positioned.playerZ + 15.20f) > 0.001f ||
        positioned.healingFlaskEnabled)
        return fail("Guard media preview could not stage its accepted source view without flask experimentation.");
}
SynchronizeSimulationViewportAspect(context);
context.simulation.ClearEvents();
context.simulation.ResetTiming();
context.simulationPaused = true;
context.simulationInput.paused = true;
context.frameDeltaSeconds = 0;
// RenderFrame's established frozenDevelopmentCheckpoint condition suppresses
// UpdateDesktopSceneControls/AdvanceFrame for this exact nonempty checkpoint,
// paused owner and zero delta. Manual StepFixed below is the sole tick owner;
// pause cancellation must not be invoked between a pose and its rendered frame.
const auto start = context.simulation.Snapshot();
if (!start.playerAlive || !start.torchFailure.heldByPlayer || start.torchFailure.triggered ||
    start.interaction.heldLightKind != interactions::HeldLightKind::Torch)
    return fail("Guard fixture requires the ordinary live original-torch start.");

InputSnapshot input;
input.damageEnabled = false;
input.tutorialEnabled = false;
input.yawRadians = start.playerYawRadians;
input.pitchRadians = mediaPreview ? -0.05f : -0.10f;
input.commands.attack = std::max(start.lastConsumedAttackSequence, context.simulationInput.commands.attack);
input.commands.parry = std::max(start.lastConsumedParrySequence, context.simulationInput.commands.parry);
const auto attackFloor = input.commands.attack;
const auto parryFloor = input.commands.parry;
if (attackFloor > UINT64_MAX - 2 || parryFloor == UINT64_MAX)
    return fail("Guard fixture input sequence cannot advance safely.");
enum class GuardPhase { Draw, Hold, Attack, Return, Release };
GuardPhase phase = GuardPhase::Draw;
unsigned phaseTicks = 0, frames = 0, images = 0, parryWindows = 0;
bool drawSwingSeen = false, startupSeen = false, activeSeen = false, recoverySeen = false;
bool guardSeen = false, returnedGuard = false, released = false;
auto previousAction = PlayerCombatAction::Idle;
const auto began = std::chrono::steady_clock::now();
std::ofstream rows(outputDirectory / "guard-transition.jsonstream", std::ios::binary);
if (!rows) return fail("Cannot open bounded guard transition receipt.");
rows.precision(10);
const unsigned imageStride = mediaPreview ? 2u : 4u;
for (unsigned tick = 0; tick < (mediaPreview ? 720u : 240u) && !released; ++tick)
{
    if (std::chrono::steady_clock::now() - began > std::chrono::seconds(90))
        return fail("Guard transition exceeded its finite 90-second capture budget.");
    input.guardHeld = phase != GuardPhase::Draw && phase != GuardPhase::Release;
    if (mediaPreview) input.yawRadians = start.playerYawRadians + 0.10f * std::sin(float(tick) * 0.01f);
    if (phaseTicks == 0)
    {
        if (phase == GuardPhase::Draw || phase == GuardPhase::Attack) ++input.commands.attack;
        else if (phase == GuardPhase::Hold) ++input.commands.parry;
    }
    context.simulation.StepFixed(input);
    const auto before = context.simulation.Snapshot();
    const auto action = before.playerCombat.action;
    if (action == PlayerCombatAction::ParryActive && previousAction != action) ++parryWindows;
    MirrorSimulationSnapshot(context);
    if (!context.simulationPaused || context.frameDeltaSeconds != 0.0f ||
        context.developmentCheckpoint.empty())
        return fail("Guard capture lost the existing frozen-development render owner.");
    bool presented = false;
    if (!RenderFrame(context, clearColor, presented) || !presented ||
        context.lastFramePresentation != horde::telemetry::RtPresentationOutcome::Presented)
        return fail("Guard frame lacks successful real RT swapchain presentation.");
    const auto& after = context.simulation.Snapshot();
    if (after.tickIndex != before.tickIndex || after.playerAnimation != before.playerAnimation ||
        after.playerX != before.playerX || after.playerZ != before.playerZ ||
        after.playerCombat.action != before.playerCombat.action)
        return fail("Guard renderer advanced the frozen owning gameplay pose.");
    if (!CompleteRtEvidenceAfterDeviceIdle(context, vkDeviceWaitIdle(context.device)))
        return fail("Guard frame lacks successful owning RT completion.");
    const auto published = context.rtFrameEvidence.PublishedStateByValue();
    const auto& complete = published.completedEvidence;
    const auto& identity = complete.identity.submitted;
    if (!published.presented || !published.hasCompletedEvidence ||
        identity.frame.simulationTick != before.tickIndex ||
        identity.frame.sceneEpoch != published.sceneEpoch ||
        identity.frame.measurementGeneration != published.measurementGeneration ||
        !complete.scene.dispatch.sceneReady || !complete.scene.dispatch.rtDispatchRecorded ||
        !complete.scene.dispatch.swapchainCopyRecorded ||
        complete.presentation.outcome != horde::telemetry::RtPresentationOutcome::Presented ||
        identity.submissionSerial != complete.presentation.lastSuccessfulPresentSubmissionSerial)
        return fail("Guard frame evidence does not own its actual presented submission.");
    std::string imageFile, hash, error, evidence;
    if (tick % imageStride == 0 || action != previousAction)
    {
        horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture pixels;
        imageFile = "guard-" + std::to_string(tick) + ".png";
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
    horde::telemetry::RtEvidenceValidationError validation{};
    if (!horde::telemetry::SerializeRtPerformanceEvidenceJson(complete, evidence, validation))
        return fail("Guard frame serialization rejected actual owning evidence.");
    rows << "{\"motorTick\":" << tick << ",\"phase\":" << static_cast<unsigned>(phase)
         << ",\"simulationTick\":" << before.tickIndex << ",\"action\":" << static_cast<unsigned>(action)
         << ",\"actionTime\":" << before.playerCombat.actionTime
         << ",\"yaw\":" << before.playerYawRadians << ",\"pitch\":" << before.playerPitchRadians
         << ",\"guardPoseWeight\":" << before.playerCombat.guardPoseWeight
         << ",\"guardHeld\":" << input.guardHeld << ",\"attackSequence\":" << before.lastConsumedAttackSequence
         << ",\"parrySequence\":" << before.lastConsumedParrySequence
         << ",\"file\":\"" << imageFile << "\",\"sha256\":\"" << hash
         << "\",\"armPixels\":" << arms << ",\"dedicatedOwnership\":" << dedicated
         << ",\"visibilityAccepted\":" << visible << ",\"completedFrame\":" << evidence << "}\n";
    rows.flush(); // Preserve a failed receipt/image before its strict rejection.
    if (!rows || !counters || !visible || arms == 0 || !before.playerAlive ||
        before.playerVitals.vitality != start.playerVitals.vitality ||
        before.playerVitals.halfHeart != start.playerVitals.halfHeart ||
        std::abs(before.playerX - start.playerX) > 0.001f || std::abs(before.playerZ - start.playerZ) > 0.001f)
        return fail("Guard capture lost strict primary arms, fixed position or unchanged vitality.");
    ++frames; ++phaseTicks;
    if (phase == GuardPhase::Draw)
    {
        drawSwingSeen = drawSwingSeen || action == PlayerCombatAction::SwingActive;
        if (drawSwingSeen && action == PlayerCombatAction::Idle &&
            before.heldItems[1].parentMode == items::HeldItemParentMode::HandSocket &&
            !before.heldItems[1].transition.active) {phase = GuardPhase::Hold; phaseTicks = 0;}
    }
    else if (phase == GuardPhase::Hold)
    {
        guardSeen = guardSeen || action == PlayerCombatAction::Guard;
        if (guardSeen && phaseTicks >= (mediaPreview ? 240u : 42u)) {phase = GuardPhase::Attack; phaseTicks = 0;}
    }
    else if (phase == GuardPhase::Attack)
    {
        startupSeen = startupSeen || action == PlayerCombatAction::SwingWindup;
        activeSeen = activeSeen || action == PlayerCombatAction::SwingActive;
        recoverySeen = recoverySeen || action == PlayerCombatAction::SwingRecovery;
        if (startupSeen && activeSeen && recoverySeen && action == PlayerCombatAction::Guard)
        {returnedGuard = true; phase = GuardPhase::Return; phaseTicks = 0;}
    }
    else if (phase == GuardPhase::Return && phaseTicks >= (mediaPreview ? 240u : 12u))
    {phase = GuardPhase::Release; phaseTicks = 0;}
    else if (phase == GuardPhase::Release && phaseTicks >= (mediaPreview ? 90u : 24u) && action == PlayerCombatAction::Idle &&
             before.playerCombat.guardPoseWeight == 0.0f) released = true;
    previousAction = action;
}
const auto& finalState = context.simulation.Snapshot();
if (!released || !returnedGuard || parryWindows != 1 ||
    finalState.lastConsumedAttackSequence != attackFloor + 2 ||
    finalState.lastConsumedParrySequence != parryFloor + 1)
    return fail("Guard sequence did not prove draw, one parry, full attack recovery, held return and release.");
std::ostringstream summary;
summary << "{\"complete\":true,\"evidenceClass\":\"Debug hidden fixed-input production-combat original-torch joins; not natural progression, damage, pacing, performance or phone acceptance\","
        << "\"frames\":" << frames << ",\"images\":" << images
        << ",\"fixedSimulationHz\":60,\"imageStrideTicks\":" << imageStride
        << ",\"mediaPreview\":" << mediaPreview << ",\"unpaced\":true,"
        << "\"parryWindows\":" << parryWindows << ",\"returnedGuard\":true,\"released\":true}\n";
return WriteReportFile(outputDirectory / "guard-transition.json", summary.str()) ? 0 : 1;
