// Debug A/B/A only. Every sample owns a completed, successfully presented RT
// submission. Per-frame device idle makes these serialized GPU observations,
// not an ordinary-play frame-pacing or sustained performance benchmark.
bool MeasureStaticGeometryPhase(VulkanSurfaceContext& context,
    const std::filesystem::path& directory, const VkClearColorValue& clear,
    const char* phase, std::string& error)
{
    constexpr unsigned warmFrames = 12, sampleFrames = 60;
    const auto generation = context.rtScene.StaticGeometryGeneration();
    const auto initial = context.simulation.Snapshot();
    const auto extent = context.rtScene.DispatchExtent();
    const auto surface = context.swapchainExtent;
    const auto backend = context.rtScene.ExecutionBackend();
    const auto opaque = std::string(context.rtScene.SelectedOpaqueFastSha256());
    const auto generic = std::string(context.rtScene.SelectedGenericDielectricSha256());
    const auto ordinary = context.rtScene.OrdinaryStaticDetailEvidence();
    const auto resources = context.rtScene.ResourceInventory();
    const auto started = std::chrono::steady_clock::now();
    std::uint64_t lastSerial = 0, sceneEpoch = 0, measurementGeneration = 0;
    unsigned samples = 0;
    std::ostringstream rows;
    rows.imbue(std::locale::classic()); rows << std::setprecision(9);
    const auto write = [&](bool complete) {
        std::ostringstream json; json.imbue(std::locale::classic()); json << std::setprecision(9);
        json << "{\"schema\":1,\"complete\":" << (complete ? "true" : "false")
             << ",\"scope\":\"Debug serialized completed RT GPU samples; no pacing or sustained acceptance\""
             << ",\"phase\":\"" << phase << "\",\"staticGeometryGeneration\":" << generation
             << ",\"warmFrames\":" << warmFrames << ",\"expectedSampleCount\":" << sampleFrames
             << ",\"sampleCount\":" << samples << ",\"dispatchWidth\":" << extent.width
             << ",\"dispatchHeight\":" << extent.height << ",\"renderScale\":" << context.renderScale
             << ",\"error\":" << (complete ? "null" : "\"" + JsonEscape(error) + "\"")
             << ",\"samples\":[" << rows.str() << "]}\n";
        return WriteReportFile(directory/(std::string("static-timing-") + phase + ".json"),json.str());
    };
    const auto fail = [&](const std::string& why) { error=why; (void)write(false); return false; };
    if(!context.simulationPaused || !gShowcaseCaptureHidden || IsWindowVisible(context.windowHandle) ||
       GetForegroundWindow()==context.windowHandle || GetFocus()==context.windowHandle)
        return fail("Static timing requires a frozen, unshown, unfocused Debug capture.");
    for(unsigned frame=0;frame<warmFrames+sampleFrames;++frame) {
        if(std::chrono::steady_clock::now()-started>std::chrono::seconds(45))
            return fail("Static timing phase exceeded its finite 45-second deadline.");
        const auto begin=std::chrono::steady_clock::now(); bool presented=false;
        if(!RenderFrame(context,clear,presented) || !presented ||
           context.lastFramePresentation!=horde::telemetry::RtPresentationOutcome::Presented)
            return fail("Static timing lost successful RT presentation: "+context.lastRtFrameError);
        const auto rendered=std::chrono::steady_clock::now();
        const VkResult idle=vkDeviceWaitIdle(context.device);
        if(idle!=VK_SUCCESS || !CompleteRtEvidenceAfterDeviceIdle(context,idle))
            return fail("Static timing could not complete its submitted owner.");
        const auto publication=context.rtFrameEvidence.PublishedStateByValue();
        const auto& completed=publication.completedEvidence;
        const auto& submitted=completed.identity.submitted;
        if(!publication.presented || !publication.hasCompletedEvidence ||
           submitted.submissionSerial<=lastSerial ||
           submitted.frame.sceneEpoch!=publication.sceneEpoch ||
           submitted.frame.measurementGeneration!=publication.measurementGeneration ||
           submitted.frame.simulationTick!=initial.tickIndex ||
           completed.presentation.outcome!=horde::telemetry::RtPresentationOutcome::Presented ||
           completed.presentation.lastSuccessfulPresentSubmissionSerial!=submitted.submissionSerial ||
           !completed.scene.dispatch.sceneReady || !completed.scene.dispatch.rtDispatchRecorded ||
           !completed.scene.dispatch.swapchainCopyRecorded ||
           !horde::telemetry::RtSampleAvailable(completed.gpu.status) || !completed.gpu.hasDuration ||
           !completed.gpu.durationNanoseconds || completed.gpu.completedSubmissionSerial!=submitted.submissionSerial)
            return fail("Static timing lacks a unique current completed RT dispatch/copy/present/GPU sample.");
        if(frame==0) {sceneEpoch=publication.sceneEpoch;measurementGeneration=publication.measurementGeneration;}
        const auto& pose=context.simulation.Snapshot();
        const auto& currentOrdinary=context.rtScene.OrdinaryStaticDetailEvidence();
        const auto currentResources=context.rtScene.ResourceInventory();
        if(publication.sceneEpoch!=sceneEpoch || publication.measurementGeneration!=measurementGeneration ||
           context.rtScene.StaticGeometryGeneration()!=generation ||
           context.rtScene.DispatchExtent().width!=extent.width || context.rtScene.DispatchExtent().height!=extent.height ||
           context.swapchainExtent.width!=surface.width || context.swapchainExtent.height!=surface.height ||
           context.rtScene.ExecutionBackend()!=backend ||
           std::string(context.rtScene.SelectedOpaqueFastSha256())!=opaque ||
           std::string(context.rtScene.SelectedGenericDielectricSha256())!=generic ||
           pose.tickIndex!=initial.tickIndex || pose.playerX!=initial.playerX || pose.playerZ!=initial.playerZ ||
           pose.playerYawRadians!=initial.playerYawRadians || pose.playerPitchRadians!=initial.playerPitchRadians ||
           currentOrdinary.unloads!=ordinary.unloads || currentOrdinary.restores!=ordinary.restores ||
           currentResources.deviceLocalBytes!=resources.deviceLocalBytes ||
           currentResources.hostVisibleBytes!=resources.hostVisibleBytes)
            return fail("Static timing scene, pose, surface, shader, resource or ordinary-detail identity changed.");
        std::string canonical; horde::telemetry::RtEvidenceValidationError validation{};
        if(!horde::telemetry::SerializeRtPerformanceEvidenceJson(completed,canonical,validation))
            return fail("Static timing failed canonical completed-frame validation.");
        if(frame>=warmFrames) {
            rows << (samples ? "," : "") << "{\"index\":" << samples
                 << ",\"renderCallCpuMs\":" << std::chrono::duration<double,std::milli>(rendered-begin).count()
                 << ",\"completedFrame\":" << canonical << "}";
            ++samples;
        }
        lastSerial=submitted.submissionSerial;
    }
    error.clear();
    if(!write(true)) {error="Could not write static timing evidence.";return false;}
    return true;
}
