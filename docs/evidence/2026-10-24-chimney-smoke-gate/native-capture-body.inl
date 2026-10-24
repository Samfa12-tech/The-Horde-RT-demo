// Temporary existing hidden-capture extension, not a shipping hook.
if(std::getenv("HORDE_LOCAL_SMOKE_CAPTURE")) {
    using namespace horde::gameplay::simulation;
    context.simulationPaused=true;context.simulationInput.paused=true;context.frameDeltaSeconds=0;
    std::ofstream rows(outputDirectory/"frames.jsonstream",std::ios::binary);
    if(!rows)return fail("Cannot open bounded smoke receipt.");rows.precision(10);
    unsigned captureIndex=0,renderedTicks=0;
    const auto render = [&](unsigned phase, unsigned motorTick, bool save) {
        const auto before = context.simulation.Snapshot();
        MirrorSimulationSnapshot(context);
        bool presented = false;
        const bool frameOk=RenderFrame(context,clearColor,presented);
        if(!frameOk||!presented||context.lastFramePresentation!=horde::telemetry::RtPresentationOutcome::Presented) {
            std::cerr<<"SMOKE_REJECT render ok="<<frameOk<<" presented="<<presented<<" presentation="<<int(context.lastFramePresentation)<<" detail="<<context.lastRtFrameError<<std::endl;return false;
        }
        ++renderedTicks;
        const auto& after = context.simulation.Snapshot();
        if (after.tickIndex != before.tickIndex || after.playerAnimation != before.playerAnimation ||
            after.dodgeActive != before.dodgeActive || after.playerX != before.playerX ||
            after.playerZ != before.playerZ || after.dodgeElapsedSeconds != before.dodgeElapsedSeconds)
        { std::cerr<<"SMOKE_REJECT pose before="<<before.tickIndex<<" after="<<after.tickIndex<<" animationEqual="<<(after.playerAnimation==before.playerAnimation)<<" positions="<<before.playerX<<","<<before.playerZ<<"/"<<after.playerX<<","<<after.playerZ<<std::endl;return false; }
        const auto idle = vkDeviceWaitIdle(context.device);
        if (!CompleteRtEvidenceAfterDeviceIdle(context, idle)) {std::cerr<<"SMOKE_REJECT idle="<<int(idle)<<std::endl;return false;}
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
        {std::cerr<<"SMOKE_REJECT evidence presented="<<published.presented<<" completed="<<published.hasCompletedEvidence<<" ticks="<<id.frame.simulationTick<<"/"<<before.tickIndex<<" fire="<<bool(completed.scene.fireLighting)<<" ready="<<completed.scene.dispatch.sceneReady<<std::endl;return false;}
        std::string evidence, error, hash;
        horde::telemetry::RtEvidenceValidationError validation{};
        if (!horde::telemetry::SerializeRtPerformanceEvidenceJson(completed, evidence, validation)) return false;
        std::string file;
        if(save) {
            horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture pixels;
            file="motion-"+std::to_string(captureIndex++)+".png";
            if(!context.rtScene.CaptureStorageImage(pixels,error)||
               !WriteRgbaPng(outputDirectory/file,pixels,error)||
               !Sha256File(outputDirectory/file,hash,error))return false;
        }
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
        if(!(bool(rows)&&counters&&visible&&arms>0))std::cerr<<"SMOKE_REJECT visibility counters="<<counters<<" visible="<<visible<<" arms="<<arms<<std::endl;
        return bool(rows) && counters && visible && arms > 0;
    };
    // Use the normal owner-thread viewport publication before manual inputs.
    SynchronizeSimulationViewportAspect(context);
    const auto start=context.simulation.Snapshot();
    for(unsigned tick=0;tick<188;++tick) {
        auto input=context.simulationInput;input.paused=false;input.damageEnabled=false;input.tutorialEnabled=false;
        input.yawRadians=start.playerYawRadians;input.pitchRadians=start.playerPitchRadians;
        input.moveForward=0;input.moveStrafe=0;input.runHeld=false;
        context.simulation.StepFixed(input);
        const auto& now=context.simulation.Snapshot();
        if(std::abs(now.playerX-start.playerX)>.001f||std::abs(now.playerZ-start.playerZ)>.001f)
            return fail("Smoke fixed-workload camera moved; no comparable measurement.");
        if(!render(tick<48?0:tick<168?1:2,tick,tick==48||tick==108||tick==167||tick==187))
            return fail("Smoke diagnostic rejected owning RT frame or visible-player gate.");
    }
    return WriteReportFile(outputDirectory/"diagnostic-only.json",
        "{\"complete\":true,\"renderedFrames\":188,\"warmup\":48,\"measured\":120,\"fixedTicks\":true,\"pacedFps\":false,\"naturalJourney\":false}\n")?0:1;
}
