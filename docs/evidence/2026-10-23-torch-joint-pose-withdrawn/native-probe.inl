// Temporary investigation only. Not a capture-admission or physical-controls pass.
// No historical checkpoint import; the current chapter motor owns every move.
if (std::getenv("HORDE_LOCAL_DARK_PROBE") != nullptr) {
    using namespace horde::gameplay::simulation;
    context.simulation.StartLiveDevelopmentRescueJourney(true);
    horde::vulkan::raytracing::PublishDevelopmentWorldReadiness(context.simulation,context.rtScene);
    context.simulation.SetPresentationAspect(float(context.swapchainExtent.width)/context.swapchainExtent.height);
    context.simulationPaused=true;
    context.simulationInput.paused=true;
    const auto clear=ClearColorForMode(capabilities.rtMode);
    unsigned imageIndex=0,movementTicks=0;
    std::ofstream rows(outputDirectory/"diagnostic-rows.jsonl",std::ios::binary);
    if (!rows) return fail("Local dark diagnostic rows unavailable.");
    const auto render=[&]() {
        MirrorSimulationSnapshot(context);
        bool presented=false;
        if(!RenderFrame(context,clear,presented) || !presented ||
           context.lastFramePresentation!=horde::telemetry::RtPresentationOutcome::Presented)
            return false;
        return true;
    };
    const auto capture=[&](unsigned place,unsigned angle) {
        if(!render() || !render()) return false;
        const auto idle=vkDeviceWaitIdle(context.device);
        if(!CompleteRtEvidenceAfterDeviceIdle(context,idle))return false;
        const auto pub=context.rtFrameEvidence.PublishedStateByValue();
        const auto& completed=pub.completedEvidence;
        const auto& id=completed.identity.submitted;
        if(!pub.presented || !pub.hasCompletedEvidence ||
           id.frame.simulationTick!=context.simulation.Snapshot().tickIndex ||
           id.frame.sceneEpoch!=pub.sceneEpoch ||
           id.frame.measurementGeneration!=pub.measurementGeneration ||
           !completed.scene.fireLighting ||
           !completed.scene.dispatch.sceneReady || !completed.scene.dispatch.rtDispatchRecorded ||
           !completed.scene.dispatch.swapchainCopyRecorded ||
           completed.presentation.outcome!=horde::telemetry::RtPresentationOutcome::Presented ||
           id.submissionSerial==0 ||
           id.submissionSerial!=completed.presentation.lastSuccessfulPresentSubmissionSerial)return false;
        std::string evidence,error,hash;
        horde::telemetry::RtEvidenceValidationError evidenceError{};
        if(!horde::telemetry::SerializeRtPerformanceEvidenceJson(completed,evidence,evidenceError))return false;
        horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
        imageIndex=place==9?100+angle:place==2?angle:place==4?16+angle:place==7?32+angle:48;
        const auto name="diag-"+std::to_string(imageIndex++)+".png";
        if(!context.rtScene.CaptureStorageImage(image,error) ||
           !WriteRgbaPng(outputDirectory/name,image,error) ||
           !Sha256File(outputDirectory/name,hash,error))return false;
        const auto& s=context.simulation.Snapshot();
        rows << "{\"index\":" << imageIndex-1 << ",\"place\":" << place << ",\"angle\":" << angle
             << ",\"file\":\"" << name << "\",\"sha256\":\"" << hash << "\",\"camera\":["
             << s.playerX << ',' << s.playerSupportWorldY << ',' << s.playerZ << ','
             << s.playerYawRadians << ',' << s.playerPitchRadians << "],\"torchPhase\":\""
             << horde::gameplay::TorchFailurePhaseName(s.torchFailure.phase) << "\",\"flameStrength\":"
             << s.heldLight.flameStrength << ",\"torchLowering\":" << s.heldItemKinematics.torchOverheadLowering
             << ",\"torchRetraction\":" << s.heldItemKinematics.torchOverheadRetraction
             << ",\"leftHandTarget\":[" << s.heldItemKinematics.leftHandLocal[0] << "," << s.heldItemKinematics.leftHandLocal[1] << "," << s.heldItemKinematics.leftHandLocal[2] << "]"
             << ",\"gripUpInView\":[" << s.heldItemKinematics.leftGripYInView[0] << "," << s.heldItemKinematics.leftGripYInView[1] << "," << s.heldItemKinematics.leftGripYInView[2] << "]"
             << ",\"crouchDrop\":" << s.playerCrouchDropMetres
             << ",\"armPixels\":" << context.rtScene.PrimaryPlayerPixelCount()
             << ",\"completedFrame\":" << evidence << "}\n";
        rows.flush();return bool(rows);
    };
    constexpr std::array<horde::gameplay::RoutePosition,10> path{{
        {0,-1},{0,-4.9f},{1.60f,-4.9f},{0,-4.9f},{-1.60f,-4.9f},{0,-4.9f},{0,-7.7f},{.95f,-9.4f},{0,-4.9f},{1.60f,-4.9f}}};
    for(unsigned place=0;place<path.size();++place) {
        const auto target=path[place];
        unsigned ticks=0;
        for(;;) {
            const auto s=context.simulation.Snapshot();
            const float dx=target.x-s.playerX,dz=target.z-s.playerZ,d=std::hypot(dx,dz);
            if(d<.001f)break;
            if(++ticks>1200 || ++movementTicks>6000 || !s.playerAlive)return fail("Local dark diagnostic motor stalled; no pose override allowed.");
            InputSnapshot input;input.damageEnabled=false;input.tutorialEnabled=false;
            input.yawRadians=std::atan2(dx,-dz);input.pitchRadians=-.15f;
            input.moveForward=std::min(1.f,d/.25f);
            context.simulation.StepFixed(input);
            if(ticks%4==0 && !render())return fail("Local dark diagnostic route rendering failed.");
        }
        if(place==9) {
            for(unsigned turn=0;turn<=80;++turn) {
                InputSnapshot input;input.damageEnabled=false;input.tutorialEnabled=false;
                input.yawRadians=1.17f+.01f*turn;input.pitchRadians=-.15f;
                context.simulation.StepFixed(input);
                if(!render())return fail("Continuous-turn native rendering failed.");
                if(turn%20==0 && !capture(place,turn))return fail("Continuous-turn owning capture failed.");
            }
        }
        if(place!=2 && place!=4 && place!=7 && place!=8)continue;
        for(unsigned angle=0;angle<16;++angle) {
            if(place==8 && angle!=8)continue;
            InputSnapshot input;input.damageEnabled=false;input.tutorialEnabled=false;
            input.yawRadians=-3.14159265359f+angle*6.28318530718f/16.f;input.pitchRadians=-.15f;
            // Normal look input with positive fixed delta; no pose/progression import.
            for(unsigned settle=0;settle<12;++settle)context.simulation.StepFixed(input);
            if(!((place==2 && (angle==11 || angle==13)) || (place==4 && angle==5) || (place==7 && angle==8) || (place==8 && angle==8)))continue;
            if(!capture(place,angle))return fail("Local dark diagnostic capture completion failed.");
        }
    }
    return WriteReportFile(outputDirectory/"diagnostic-only.json",
        "{\"complete\":true,\"evidenceClass\":\"temporary input-driven current chapter diagnostic; five frozen views and 81 positive-delta turn steps; no physical controls, natural full route or phone acceptance\",\"historicalCheckpointImported\":false,\"captures\":10}\n")?0:1;
}
