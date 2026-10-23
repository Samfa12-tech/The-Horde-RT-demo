// Local diagnostic, never a shipping hook. Fixed ordinary input, isolated spawn.
if (std::getenv("HORDE_LOCAL_BACKWARD_FIRST_PERSON") != nullptr) {
    using namespace horde::gameplay::simulation;
    auto config = ProductionGameSimulationConfig();
    config.playerStartX = 0.f; config.playerStartZ = -4.9f;
    config.playerStartYawRadians = 0.f; config.playerStartPitchRadians = -.15f;
    context.simulation = GameSimulation(config);
    context.simulation.SetPresentationAspect(float(context.swapchainExtent.width) /
                                            context.swapchainExtent.height);
    context.simulationPaused = true; context.simulationInput.paused = true;
    context.frameDeltaSeconds = 0.f;
    std::ofstream rows(outputDirectory / "frames.jsonstream", std::ios::binary);
    if (!rows) return fail("First-person motion receipt could not be opened.");
    rows.precision(10);
    unsigned captureIndex = 0, renderedTicks = 0;
    bool sawBackwardTravel = false, sawReversePhase = false, sawDodge = false;
    bool sawDodgeEnd = false, sawResume = false;
    const auto render = [&](unsigned phase, unsigned motorTick, bool save) {
        const auto before = context.simulation.Snapshot();
        MirrorSimulationSnapshot(context);
        bool presented = false;
        if (!RenderFrame(context, clearColor, presented) || !presented ||
            context.lastFramePresentation != horde::telemetry::RtPresentationOutcome::Presented)
            return false;
        ++renderedTicks;
        const auto& after = context.simulation.Snapshot();
        if (after.tickIndex != before.tickIndex || after.playerAnimation != before.playerAnimation ||
            after.dodgeActive != before.dodgeActive || after.playerX != before.playerX ||
            after.playerZ != before.playerZ || after.dodgeElapsedSeconds != before.dodgeElapsedSeconds)
            return false; // Renderer must not replace the pose under inspection.
        if (!save) return true;
        const auto idle = vkDeviceWaitIdle(context.device);
        if (!CompleteRtEvidenceAfterDeviceIdle(context, idle)) return false;
        const auto published = context.rtFrameEvidence.PublishedStateByValue();
        const auto& completed = published.completedEvidence;
        const auto& id = completed.identity.submitted;
        if (!published.presented || !published.hasCompletedEvidence ||
            id.frame.simulationTick != before.tickIndex || id.frame.sceneEpoch != published.sceneEpoch ||
            id.frame.measurementGeneration != published.measurementGeneration ||
            !completed.scene.fireLighting || !completed.scene.dispatch.sceneReady ||
            !completed.scene.dispatch.rtDispatchRecorded || !completed.scene.dispatch.swapchainCopyRecorded ||
            completed.presentation.outcome != horde::telemetry::RtPresentationOutcome::Presented ||
            id.submissionSerial != completed.presentation.lastSuccessfulPresentSubmissionSerial)
            return false;
        std::string evidence, error, hash;
        horde::telemetry::RtEvidenceValidationError validation{};
        if (!horde::telemetry::SerializeRtPerformanceEvidenceJson(completed, evidence, validation)) return false;
        horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture pixels;
        const auto file = "motion-" + std::to_string(captureIndex++) + ".png";
        if (!context.rtScene.CaptureStorageImage(pixels, error) ||
            !WriteRgbaPng(outputDirectory / file, pixels, error) ||
            !Sha256File(outputDirectory / file, hash, error)) return false;
        const auto masks = context.rtScene.LastInstanceMasks();
        const auto flags = context.rtScene.LastPlayerWorldBodyInstanceFlags();
        const bool dedicated = horde::vulkan::raytracing::HasDedicatedPlayerPrimaryOwnership(masks, flags);
        const bool counters = context.rtScene.DiagnosticsAvailability() ==
            horde::vulkan::raytracing::RtDiagnosticAvailability::Available;
        const auto arms = context.rtScene.PrimaryPlayerPixelCount();
        const bool visible = horde::platform::windows::HasExpectedPlayerCaptureVisibility(
            dedicated, context.rtScene.LastPlayerPrimaryVisible(), counters, arms, false, nullptr);
        const auto& a = before.playerAnimation;
        rows << "{\"phase\":" << phase << ",\"motorTick\":" << motorTick
             << ",\"simulationTick\":" << before.tickIndex << ",\"file\":\"" << file
             << "\",\"sha256\":\"" << hash << "\",\"camera\":[" << before.playerX << ','
             << before.playerSupportWorldY << ',' << before.playerZ << ',' << before.playerYawRadians
             << ',' << before.playerPitchRadians << "],\"grounded\":" << before.playerGrounded
             << ",\"travelled\":" << before.playerTravelledThisTick
             << ",\"legacyLocomotionTime\":" << a.locomotionTime
             << ",\"phaseOffset\":" << a.locomotionPhaseOffsetSeconds
             << ",\"effectivePhase\":" << (double(a.locomotionTime) + a.locomotionPhaseOffsetSeconds)
             << ",\"walkBlend\":" << a.locomotionBlend << ",\"dodgeActive\":" << before.dodgeActive
             << ",\"dodgeElapsed\":" << before.dodgeElapsedSeconds << ",\"dodgeWeight\":" << a.dodge.weight
             << ",\"acceptedDodgeSequence\":" << before.acceptedDodgeSequence
             << ",\"equipmentStowed\":" << before.rescue.equipmentStowed
             << ",\"armPixels\":" << arms << ",\"dedicatedOwnership\":" << dedicated
             << ",\"diagnosticCounters\":" << counters << ",\"visibilityAccepted\":" << visible
             << ",\"worldBodyFlags\":" << flags << ",\"completedFrame\":" << evidence << "}\n";
        rows.flush();
        // Preserve failed image/receipt before rejection; no outside-frame exemption.
        return bool(rows) && counters && visible && arms > 0;
    };
    constexpr std::array<unsigned, 7> counts{{1, 24, 12, 12, 24, 12, 12}};
    constexpr std::array<float, 7> directions{{0.f, -1.f, 1.f, -1.f, -1.f, -1.f, 0.f}};
    for (unsigned phase = 0; phase < counts.size(); ++phase) {
        for (unsigned tick = 1; tick <= counts[phase]; ++tick) {
            InputSnapshot input;
            input.damageEnabled = false; input.tutorialEnabled = false;
            input.yawRadians = 0.f; input.pitchRadians = -.15f;
            input.moveForward = directions[phase];
            if (phase == 4 && tick == 1)
                input.commands.dodge = context.simulation.Snapshot().lastConsumedDodgeSequence + 1;
            else if (phase >= 4) input.commands.dodge = context.simulation.Snapshot().lastConsumedDodgeSequence;
            const auto prior = context.simulation.Snapshot();
            context.simulation.StepFixed(input);
            const auto& s = context.simulation.Snapshot();
            if (directions[phase] < 0 && !s.dodgeActive && s.playerZ > prior.playerZ && s.playerTravelledThisTick > 0)
                sawBackwardTravel = true;
            if (phase == 1 && double(s.playerAnimation.locomotionTime) + s.playerAnimation.locomotionPhaseOffsetSeconds <
                double(prior.playerAnimation.locomotionTime) + prior.playerAnimation.locomotionPhaseOffsetSeconds)
                sawReversePhase = true;
            sawDodge = sawDodge || (s.dodgeActive && s.playerAnimation.dodge.weight > 0 && s.acceptedDodgeSequence > 0);
            sawDodgeEnd = sawDodgeEnd || (phase >= 4 && sawDodge && !s.dodgeActive);
            sawResume = sawResume || (phase == 5 && !s.dodgeActive && s.playerTravelledThisTick > 0 &&
                s.playerAnimation.locomotionPhaseOffsetSeconds < prior.playerAnimation.locomotionPhaseOffsetSeconds);
            const bool save = phase == 0 || tick % 6 == 0 || (phase == 4 && tick == 1);
            if (!render(phase, tick, save)) return fail("Dynamic first-person diagnostic rejected frame ownership, pose continuity or visible arms; retained receipts identify the last sample.");
        }
    }
    if (!sawBackwardTravel || !sawReversePhase || !sawDodge || !sawDodgeEnd || !sawResume)
        return fail("Dynamic first-person diagnostic did not exercise all required ordinary motion and dodge states.");
    return WriteReportFile(outputDirectory / "diagnostic-only.json",
        "{\"complete\":true,\"captures\":" + std::to_string(captureIndex) +
        ",\"renderedFixedTicks\":" + std::to_string(renderedTicks) +
        ",\"scope\":\"isolated current-rules first-person fixed motor sequence; no natural journey, physical control, listening, pacing or phone acceptance\"}\n") ? 0 : 1;
}
