// Temporary hidden diagnostic; original static mesh and viewmodel remain unchanged.
if (std::getenv("HORDE_LOCAL_RUN_RT_PROOF") != nullptr) {
    using namespace horde::gameplay::simulation;
    context.simulationPaused=true;context.simulationInput.paused=true;context.frameDeltaSeconds=0.f;
    std::ofstream rows(outputDirectory / "frames.jsonstream",std::ios::binary);
    if(!rows)return fail("Cannot open isolated Run receipt.");rows.precision(10);
    unsigned captureIndex=0,renderedTicks=0;
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
             << ",\"speed\":" << before.playerMovementSpeedMetresPerSecond << ",\"travelled\":" << before.playerTravelledThisTick
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
    for(unsigned view=0;view<2;++view) {
        auto config=ProductionGameSimulationConfig();config.swordStartsStowed=false;
        config.playerStartX=view?-30.5f:0.f;config.playerStartZ=view?-15.2f:1.85f;
        config.playerStartYawRadians=view?-1.57079632679f:0.f;config.playerStartPitchRadians=view?-.1f:-.95f;
        context.simulation=GameSimulation(config);
        context.simulation.SetPresentationAspect(float(context.swapchainExtent.width)/context.swapchainExtent.height);
        bool fullSpeed=false;
        for(unsigned tick=0;tick<90;++tick) {
            InputSnapshot input{};input.damageEnabled=false;input.tutorialEnabled=false;
            input.yawRadians=config.playerStartYawRadians;input.pitchRadians=config.playerStartPitchRadians;
            input.moveForward=(tick>0&&tick<=72)?1.f:0.f;input.runHeld=tick>0&&tick<=72;
            input.paused=tick==89;context.simulation.StepFixed(input);
            fullSpeed|=context.simulation.Snapshot().playerMovementSpeedMetresPerSecond>3.19f;
            if(!render(view,tick,tick%6==0||tick==89))return fail("Run diagnostic rejected ownership or visible-player gate; failure retained.");
        }
        if(!fullSpeed)return fail("Run fixture failed to reach unchanged3.2m/s.");
    }
    return WriteReportFile(outputDirectory/"diagnostic-only.json","{\"complete\":true,\"frames\":"+std::to_string(renderedTicks)+",\"captures\":"+std::to_string(captureIndex)+",\"admitted\":false,\"scope\":\"isolated unpaced RT presentation, not natural play/performance/phone acceptance\"}\n")?0:1;
}
