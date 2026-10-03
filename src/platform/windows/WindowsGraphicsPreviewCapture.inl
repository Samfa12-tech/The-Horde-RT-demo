// Included only in DiagnosticWindow.cpp's Debug capture section. These helpers
// use that application's production Vulkan owner, renderer and readback path.
struct GraphicsPreviewCaptureRecord
{
    std::string name, filename, pngSha256;
    horde::graphics::GraphicsPreviewSnapshot pose{};
    horde::graphics::GraphicsAppliedSnapshot settings{};
    horde::telemetry::RtResourceInventory resources{};
    horde::telemetry::RtLifecyclePublishedState publication{};
    std::vector<double> renderCallMilliseconds;
    double readbackMilliseconds = 0.0;
    bool redBlueSwapNormalised = false;
};
struct GraphicsPreviewCaptureTransaction
{
    std::string phase;
    horde::graphics::GraphicsAppliedSnapshot snapshot{};
    bool confirmedInMemory = false;
};

bool WriteGraphicsPreviewCaptureManifest(
    const std::filesystem::path& outputDirectory, const VulkanSurfaceContext& context,
    const horde::vulkan::DeviceCapabilities& capabilities,
    const std::string& executableSha256,
    const std::vector<GraphicsPreviewCaptureRecord>& captures,
    const std::vector<GraphicsPreviewCaptureTransaction>& transactions,
    const bool complete, const std::string& error)
{
    std::ostringstream manifest;
    manifest.imbue(std::locale::classic());
    manifest << std::fixed << std::setprecision(6);
    const auto settingsJson = [&manifest](const horde::graphics::GraphicsSettings& settings) {
        manifest << "{\"renderScalePercent\":" << settings.renderScalePercent
                 << ",\"waterQuality\":" << static_cast<unsigned>(settings.waterQuality)
                 << ",\"fireDetail\":" << static_cast<unsigned>(settings.fireDetail)
                 << ",\"previewFrameCap\":" << settings.previewFrameCap << '}';
    };
    const auto appliedJson = [&manifest, &settingsJson](const horde::graphics::GraphicsAppliedSnapshot& snapshot) {
        manifest << "{\"serial\":" << snapshot.serial << ",\"lifecycleGeneration\":" << snapshot.lifecycleGeneration
                 << ",\"requested\":"; settingsJson(snapshot.requested);
        manifest << ",\"effective\":"; settingsJson(snapshot.effective);
        manifest << ",\"compiledOpticalProfile\":\""
                 << (snapshot.opticalProfile == horde::graphics::OpticalProfile::High ? "High" : "Mobile")
                 << "\",\"internalWidth\":" << snapshot.internalExtent.width
                 << ",\"internalHeight\":" << snapshot.internalExtent.height
                 << ",\"outputWidth\":" << snapshot.outputExtent.width
                 << ",\"outputHeight\":" << snapshot.outputExtent.height
                 << ",\"rtPresented\":" << (snapshot.rtPresented ? "true" : "false") << '}';
    };
    manifest << "{\n  \"schemaVersion\":1,\n  \"complete\":" << (complete ? "true" : "false")
             << ",\n  \"source\":\"rt-storage-image\",\n  \"sceneOnly\":true,\n  \"overlaysIncluded\":false,"
             << "\n  \"sceneProfile\":\"GraphicsPreview\",\n  \"buildId\":\"" << JsonEscape(HORDE_RT_BUILD_ID)
             << "\",\n  \"displayVersion\":\"" << JsonEscape(HORDE_RT_DISPLAY_VERSION)
             << "\",\n  \"executableSha256\":\"" << executableSha256
             << "\",\n  \"executionBackend\":\"" << horde::vulkan::ToString(context.rtScene.ExecutionBackend())
             << "\",\n  \"presentationRetirement\":\"" << horde::vulkan::PresentCompletionDiagnostic(context.presentCompletionMode)
             << "\",\n  \"outputExposure\":" << context.outputExposure
             << ",\n  \"swapchainFormat\":" << static_cast<unsigned>(context.swapchainFormat)
             << ",\n  \"device\":{\"gpuName\":\"" << JsonEscape(capabilities.identity.gpuName)
             << "\",\"vendorId\":" << capabilities.identity.vendorId << ",\"deviceId\":" << capabilities.identity.deviceId
             << ",\"driverVersion\":" << capabilities.identity.driverVersion << ",\"apiVersion\":" << capabilities.identity.vulkanApiVersion
             << "},\n  \"selectedCompiledCatalog\":{\"bundleIdentity\":\""
             << JsonEscape(context.rtScene.SelectedPipelineBundleIdentity()) << "\",\"artifacts\":[";
    unsigned artifactIndex = 0u;
    for (const auto material : {horde::vulkan::raytracing::RtMaterialStrategy::OpaqueFast,
                               horde::vulkan::raytracing::RtMaterialStrategy::GenericDielectric})
    {
        if (artifactIndex++) manifest << ',';
        const auto artifact = context.rtScene.SelectedPipelineArtifactMetadata(material);
        if (!artifact) { manifest << "null"; continue; }
        manifest << "{\"key\":\"" << JsonEscape(std::string(artifact->canonicalKey))
                 << "\",\"spirvSha256\":\"" << artifact->spirvSha256 << "\",\"includeSha256\":\"" << artifact->includeSha256
                 << "\",\"artifactPath\":\"" << JsonEscape(std::string(artifact->artifactPath))
                 << "\",\"wordCount\":" << artifact->expectedWordCount
                 << ",\"diagnosticsBinding\":" << (artifact->hasDiagnosticsBinding ? "true" : "false") << '}';
    }
    manifest << "]},\n  \"isolation\":{\"userPreferencesLoaded\":false,\"userPreferencesWritten\":false,"
             << "\"gameplayAdvanced\":false,\"gameplayEventsConsumed\":false,\"audioStarted\":false,"
             << "\"fullShowcaseGpuSceneAllocated\":false},\n  \"limits\":{\"settlingFramesPerPose\":" << kCaptureSettlingFrames
             << ",\"maxRecreateAttemptsPerPose\":8,\"frameFenceAndAcquireTimeoutSeconds\":2,"
             << "\"capturePacing\":\"unpaced validation; configured preview cap is not an FPS measurement\","
             << "\"allocationAccounting\":\"tracked Vulkan allocations; host/device flags may overlap; not resident VRAM\","
             << "\"remainingGates\":[\"native UI visual and interaction acceptance\",\"changed-quality output resize and failure acceptance\","
             << "\"disk persistence and interruption recovery\",\"owner scene and changed-audio acceptance\",\"Android exact device acceptance\"]},"
             << "\n  \"transactions\":[";
    for (std::size_t index = 0u; index < transactions.size(); ++index)
    {
        if (index) manifest << ',';
        const auto& transaction = transactions[index];
        manifest << "{\"phase\":\"" << transaction.phase << "\",\"confirmedInMemory\":"
                 << (transaction.confirmedInMemory ? "true" : "false") << ",\"applied\":";
        appliedJson(transaction.snapshot); manifest << '}';
    }
    manifest << "],\n  \"error\":" << (error.empty() ? "null" : "\"" + JsonEscape(error) + "\"") << ",\n  \"captures\":[\n";
    for (std::size_t index = 0u; index < captures.size(); ++index)
    {
        const auto& capture = captures[index]; const auto& pose = capture.pose;
        const auto& resources = capture.resources; const auto& publication = capture.publication;
        const auto& evidence = publication.completedEvidence;
        const bool currentGpu = publication.hasCompletedEvidence && evidence.gpu.hasDuration &&
            evidence.identity.submitted.frame.sceneEpoch == publication.sceneEpoch &&
            evidence.identity.submitted.frame.measurementGeneration == publication.measurementGeneration &&
            evidence.identity.submitted.frame.simulationTick == pose.tick &&
            evidence.gpu.completedSubmissionSerial == evidence.identity.submitted.submissionSerial;
        manifest << "    {\"name\":\"" << capture.name << "\",\"file\":\"" << capture.filename
                 << "\",\"pngSha256\":\"" << capture.pngSha256
                 << "\",\"honestlyPresentedRtFrame\":true,\"outputRedBlueSwapAppliedAndNormalised\":"
                 << (capture.redBlueSwapNormalised ? "true" : "false")
                 << ",\"previewTimeline\":{\"epoch\":" << pose.timelineEpoch << ",\"tick\":" << pose.tick
                 << ",\"timeSeconds\":" << pose.timeSeconds << ",\"paused\":" << (pose.paused ? "true" : "false")
                 << ",\"motion\":" << (pose.motionTest ? "true" : "false") << "},\"camera\":{\"x\":" << pose.camera.x
                 << ",\"z\":" << pose.camera.z << ",\"yaw\":" << pose.camera.yaw << ",\"pitch\":" << pose.camera.pitch
                 << "},\"skeleton\":{\"animation\":\"Idle\",\"animationTime\":" << pose.skeleton.animationTime
                 << "},\"firePhases\":[" << pose.fireEmitters[0].phase << ',' << pose.fireEmitters[1].phase
                 << "],\"graphics\":"; appliedJson(capture.settings);
        manifest << ",\"currentEvidence\":{\"sceneEpoch\":" << publication.sceneEpoch
                 << ",\"measurementGeneration\":" << publication.measurementGeneration
                 << ",\"hasCompletedEvidence\":" << (publication.hasCompletedEvidence ? "true" : "false")
                 << ",\"submissionSerial\":" << evidence.identity.submitted.submissionSerial
                 << ",\"recordedPreviewTick\":" << evidence.identity.submitted.frame.simulationTick
                 << ",\"completionSerial\":" << evidence.identity.completionSerial << "},\"resources\":{\"buffers\":" << resources.bufferCount
                 << ",\"memoryAllocations\":" << resources.memoryAllocationCount
                 << ",\"blasCount\":" << resources.bottomLevelAccelerationStructureCount
                 << ",\"tlasCount\":" << resources.topLevelAccelerationStructureCount
                 << ",\"tlasInstanceCount\":" << resources.tlasInstanceCount
                 << ",\"pipelineCount\":" << resources.pipelineCount << ",\"sbtCount\":" << resources.shaderBindingTableCount
                 << ",\"descriptorSetCount\":" << resources.descriptorSetCount
                 << ",\"deviceLocalBytes\":" << resources.deviceLocalBytes << ",\"hostVisibleBytes\":" << resources.hostVisibleBytes
                 << "},\"timing\":{\"successfulPresentCount\":" << capture.renderCallMilliseconds.size()
                 << ",\"renderCallMeanMs\":" << CaptureMeanMs(capture.renderCallMilliseconds)
                 << ",\"renderCallMedianMs\":" << CaptureMedianMs(capture.renderCallMilliseconds)
                 << ",\"readbackMs\":" << capture.readbackMilliseconds << ",\"currentCompletedGpuMs\":";
        if (currentGpu) manifest << static_cast<double>(evidence.gpu.durationNanoseconds) / 1'000'000.0;
        else manifest << "null";
        manifest << "}}" << (index + 1u == captures.size() ? "\n" : ",\n");
    }
    manifest << "  ]\n}\n";
    return WriteReportFile(outputDirectory / "graphics-preview-capture-manifest.json", manifest.str());
}

int RunGraphicsPreviewCapture(VulkanSurfaceContext& context,
                              horde::vulkan::DeviceCapabilities& capabilities,
                              const std::filesystem::path& outputDirectory)
{
    std::error_code directoryError;
    std::filesystem::create_directories(outputDirectory, directoryError);
    if (directoryError) { std::cerr << directoryError.message() << '\n'; return 1; }
    // RunDiagnosticWindow checked the directory before writing its two native
    // capability reports. Refuse any additional pre-existing capture payload.
    for (const auto& entry : std::filesystem::directory_iterator(outputDirectory, directoryError))
    {
        if (!entry.is_regular_file() ||
            (entry.path().filename() != kTextReportFilename && entry.path().filename() != kJsonReportFilename))
        { std::cerr << "Graphics preview output contains an unrelated file.\n"; return 1; }
    }
    if (directoryError) { std::cerr << directoryError.message() << '\n'; return 1; }
    std::vector<GraphicsPreviewCaptureRecord> captures;
    std::vector<GraphicsPreviewCaptureTransaction> transactions;
    std::string executableSha256, diagnostic;
    const auto fail = [&](const std::string& reason) {
        (void)WriteGraphicsPreviewCaptureManifest(outputDirectory, context, capabilities, executableSha256, captures, transactions, false, reason);
        std::cerr << "Graphics preview capture failed: " << reason << '\n'; return 1;
    };
    std::array<wchar_t, 32768u> executablePath{};
    const DWORD pathLength = GetModuleFileNameW(nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));
    if (pathLength == 0u || pathLength >= executablePath.size() ||
        !Sha256File(std::filesystem::path(executablePath.data()), executableSha256, diagnostic))
        return fail("Could not hash the actual capture executable: " + diagnostic);
    if (!context.graphicsPreviewCapture || !context.useRtPath || !context.rtScene.IsReady() ||
        context.rtScene.Profile() != horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview)
        return fail("Only the ready production compact hardware RT profile may produce these captures.");
    const auto simulationBefore = context.simulation.Snapshot();
    const auto nextEventBefore = context.simulation.Events().NextSequence();
    const auto eventCountBefore = context.simulation.Events().Size();
    const auto clearColor = ClearColorForMode(capabilities.rtMode);
    // Every readback follows ordinary successful presentation of the current
    // output, never a SUBOPTIMAL frame whose resources were just recreated.
    const auto settle = [&](const int frameCount, std::vector<double>& timings) {
        timings.clear(); unsigned retries = 0u;
        for (int frame = 0; frame < frameCount; ++frame)
        {
            bool presented = false; const auto start = std::chrono::steady_clock::now();
            if (!RenderFrame(context, clearColor, presented)) return false;
            if (context.lastFramePresentation == horde::telemetry::RtPresentationOutcome::PresentedNeedsRecreate ||
                context.lastFramePresentation == horde::telemetry::RtPresentationOutcome::NotPresentedNeedsRecreate)
            {
                if (++retries > 8u) return false;
                frame = -1; timings.clear(); continue;
            }
            if (!presented || context.lastFramePresentation != horde::telemetry::RtPresentationOutcome::Presented) return false;
            timings.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
            FinishGraphicsFrame(context, presented);
            capabilities.rtScene.presented = true;
            capabilities.rtScene.executionBackend = context.rtScene.ExecutionBackend();
        }
        return true;
    };
    context.graphicsEdit.emplace(context.savedGraphics);
    const auto command = [&](const std::string& phase, const std::optional<horde::graphics::GraphicsCommand>& request,
                             const bool confirm) {
        if (!QueueGraphicsCommand(context, request)) return false;
        std::vector<double> timings;
        if (!settle(2, timings) || context.graphicsCommand || !context.graphicsEdit->Effective()) return false;
        const auto snapshot = *context.graphicsEdit->Effective();
        if (confirm && !context.graphicsEdit->Confirm()) return false;
        transactions.push_back({phase, snapshot, confirm}); return true;
    };
    auto candidate = context.savedGraphics; candidate.previewFrameCap = 60;
    if (!context.graphicsEdit->Stage(candidate) || !command("apply-cap-60", context.graphicsEdit->RequestApply(1u), false) ||
        !command("revert-cap-30", context.graphicsEdit->RequestRevert(1u), false) ||
        !context.graphicsEdit->Stage(candidate) || !command("keep-cap-60-memory-only", context.graphicsEdit->RequestApply(1u), true) ||
        !context.graphicsEdit->Stage(context.savedGraphics) || !command("restore-baseline-memory-only", context.graphicsEdit->RequestApply(1u), true))
        return fail("Production Apply/Revert/Keep did not acknowledge actual current RT presentation.");
    context.graphicsEdit.reset(); context.graphicsCommand.reset();
    context.graphicsPreviewDelta = 0.0;
    for (const auto& pose : horde::platform::windows::kGraphicsPreviewCapturePoses)
    {
        if (!horde::platform::windows::StageGraphicsPreviewCapturePose(context.graphicsPreview, pose))
            return fail("Shared deterministic preview timeline or paused pose changed unexpectedly.");
        GraphicsPreviewCaptureRecord record; record.name = pose.name;
        record.pose = context.graphicsPreview.Snapshot();
        if (!settle(kCaptureSettlingFrames, record.renderCallMilliseconds))
            return fail(record.name + " did not reach stable ordinary RT presentation: " + context.lastRtFrameError);
        const auto renderedPose = context.graphicsPreview.Snapshot();
        if (renderedPose.tick != pose.tick || renderedPose.timelineEpoch != record.pose.timelineEpoch)
            return fail(record.name + " changed the frozen authored timeline during rendering.");
        const VkResult idle = vkDeviceWaitIdle(context.device);
        if (!CompleteRtEvidenceAfterDeviceIdle(context, idle)) return fail(record.name + " could not drain current RT evidence.");
        horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
        const auto readbackStart = std::chrono::steady_clock::now();
        if (!context.rtScene.CaptureStorageImage(image, diagnostic)) return fail(record.name + " readback failed: " + diagnostic);
        record.readbackMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - readbackStart).count();
        record.filename = record.name + ".png";
        if (!WriteRgbaPng(outputDirectory / record.filename, image, diagnostic) ||
            !Sha256File(outputDirectory / record.filename, record.pngSha256, diagnostic))
            return fail(record.name + " PNG/hash failed: " + diagnostic);
        const horde::graphics::GraphicsCommand current{0u, 1u, horde::graphics::GraphicsCommandKind::Apply, context.savedGraphics};
        record.settings = GraphicsSnapshot(context, current); record.settings.rtPresented = true;
        if (image.width != record.settings.internalExtent.width || image.height != record.settings.internalExtent.height ||
            !(record.settings.effective == context.savedGraphics)) return fail("Capture dimensions or baseline graphics differ from actual presented resources.");
        record.redBlueSwapNormalised = image.redBlueSwapNormalised;
        record.resources = context.rtScene.ResourceInventory();
        record.publication = context.rtFrameEvidence.PublishedStateByValue();
        if (record.resources.topLevelAccelerationStructureCount != 1u || record.resources.tlasInstanceCount != 7u)
            return fail("Compact preview did not retain exactly its seven production TLAS roles.");
        captures.push_back(std::move(record));
        std::cout << "Captured graphics preview " << pose.name << " at tick " << pose.tick << '\n';
    }
    const auto& simulationAfter = context.simulation.Snapshot();
    if (simulationAfter.tickIndex != simulationBefore.tickIndex ||
        simulationAfter.inputPublicationSequence != simulationBefore.inputPublicationSequence ||
        simulationAfter.retryGeneration != simulationBefore.retryGeneration ||
        simulationAfter.playerX != simulationBefore.playerX || simulationAfter.playerZ != simulationBefore.playerZ ||
        simulationAfter.finaleComplete != simulationBefore.finaleComplete ||
        context.simulation.Events().NextSequence() != nextEventBefore || context.simulation.Events().Size() != eventCountBefore)
        return fail("The preview capture changed authoritative gameplay or consumed feedback events.");
    if (!WriteGraphicsPreviewCaptureManifest(outputDirectory, context, capabilities, executableSha256, captures, transactions, true, {}))
        return fail("Could not write the complete graphics preview capture manifest.");
    std::cout << "Graphics preview scene-only captures complete; UI/device/owner acceptance remains separate.\n";
    return 0;
}
