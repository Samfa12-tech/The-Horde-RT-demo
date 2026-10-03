// Included only in DiagnosticWindow.cpp's Debug capture section. These helpers
// use that application's production Vulkan owner, renderer and readback path.
struct GraphicsPreviewCaptureRecord
{
    std::string name, filename, pngSha256;
    horde::graphics::GraphicsPreviewSnapshot pose{};
    horde::graphics::GraphicsAppliedSnapshot settings{};
    horde::telemetry::RtResourceInventory resources{};
    horde::telemetry::RtLifecyclePublishedState publication{};
    bool sceneGlassEnabled = true;
    std::vector<std::uint8_t> instanceMasksByCustomIndex;
    std::vector<double> renderCallMilliseconds;
    double readbackMilliseconds = 0.0;
    bool redBlueSwapNormalised = false;
};
struct GraphicsPreviewCaptureTransaction
{
    std::string phase;
    horde::graphics::GraphicsAppliedSnapshot snapshot{};
    bool confirmedInMemory = false;
    horde::telemetry::RtLifecyclePublishedState before{}, after{};
    bool sceneGlassEnabled = true;
    std::vector<std::uint8_t> instanceMasksByCustomIndex;
};

bool CurrentCompletedGraphicsPreviewPresent(const horde::telemetry::RtLifecyclePublishedState& publication)
{
    const auto& evidence = publication.completedEvidence;
    return publication.hasCompletedEvidence && evidence.identity.completionSerial != 0u &&
        evidence.identity.submitted.submissionSerial != 0u &&
        evidence.identity.submitted.frame.sceneEpoch == publication.sceneEpoch &&
        evidence.identity.submitted.frame.measurementGeneration == publication.measurementGeneration &&
        evidence.presentation.outcome == horde::telemetry::RtPresentationOutcome::Presented &&
        evidence.presentation.lastSuccessfulPresentSubmissionSerial >= evidence.identity.submitted.submissionSerial;
}

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
                 << ",\"previewFrameCap\":" << settings.previewFrameCap
                 << ",\"glassEnabled\":" << (settings.glassEnabled ? "true" : "false") << '}';
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
                 << ",\"executionBackend\":\""
                 << (snapshot.backend == horde::graphics::GraphicsBackend::RayQueryCompute ? "RayQueryCompute" :
                     snapshot.backend == horde::graphics::GraphicsBackend::RayTracingPipeline ? "RayTracingPipeline" : "Unsupported")
                 << "\",\"sceneProfile\":\""
                 << (snapshot.scene == horde::graphics::GraphicsScene::Preview ? "GraphicsPreview" : "Showcase")
                 << "\",\"rtPresented\":" << (snapshot.rtPresented ? "true" : "false") << '}';
    };
    const auto masksJson = [&manifest](const std::vector<std::uint8_t>& masks) {
        manifest << '[';
        for (std::size_t index = 0u; index < masks.size(); ++index)
        { if (index) manifest << ','; manifest << static_cast<unsigned>(masks[index]); }
        manifest << ']';
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
             << "\"allocationAccounting\":\"tracked Vulkan allocations; host/device flags may overlap; not resident VRAM; Glass Off retains fixed BLAS/TLAS roles and implies no allocation savings\","
             << "\"glassValidationScope\":\"compact preview fixture custom-index 9 all-ray mask; full Showcase roof and lantern geometry remain a separate acceptance gate\","
             << "\"remainingGates\":[\"native UI visual and interaction acceptance\",\"changed-quality output resize and failure acceptance\","
             << "\"disk persistence and interruption recovery\",\"owner scene and changed-audio acceptance\",\"Android exact device acceptance\"]},"
             << "\n  \"transactions\":[";
    for (std::size_t index = 0u; index < transactions.size(); ++index)
    {
        if (index) manifest << ',';
        const auto& transaction = transactions[index];
        manifest << "{\"phase\":\"" << transaction.phase << "\",\"confirmedInMemory\":"
                 << (transaction.confirmedInMemory ? "true" : "false") << ",\"applied\":";
        appliedJson(transaction.snapshot);
        const auto& completed = transaction.after.completedEvidence;
        manifest << ",\"sceneEpochBefore\":" << transaction.before.sceneEpoch
                 << ",\"sceneEpochAfter\":" << transaction.after.sceneEpoch
                 << ",\"measurementGenerationBefore\":" << transaction.before.measurementGeneration
                 << ",\"measurementGenerationAfter\":" << transaction.after.measurementGeneration
                 << ",\"sceneGlassEnabled\":" << (transaction.sceneGlassEnabled ? "true" : "false")
                 << ",\"instanceMasksByCustomIndex\":"; masksJson(transaction.instanceMasksByCustomIndex);
        manifest << ",\"customGlassIndex\":9,\"customGlassMask\":"
                 << static_cast<unsigned>(transaction.instanceMasksByCustomIndex[9u])
                 << ",\"currentCompletedPresent\":" << (CurrentCompletedGraphicsPreviewPresent(transaction.after) ? "true" : "false")
                 << ",\"submittedSceneEpoch\":" << completed.identity.submitted.frame.sceneEpoch
                 << ",\"submittedMeasurementGeneration\":" << completed.identity.submitted.frame.measurementGeneration
                 << ",\"submissionSerial\":" << completed.identity.submitted.submissionSerial
                 << ",\"completionSerial\":" << completed.identity.completionSerial << '}';
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
        manifest << ",\"sceneGlassEnabled\":" << (capture.sceneGlassEnabled ? "true" : "false")
                 << ",\"instanceMasksByCustomIndex\":"; masksJson(capture.instanceMasksByCustomIndex);
        manifest << ",\"customGlassIndex\":9,\"customGlassMask\":" << static_cast<unsigned>(capture.instanceMasksByCustomIndex[9u])
                 << ",\"currentEvidence\":{\"sceneEpoch\":" << publication.sceneEpoch
                 << ",\"measurementGeneration\":" << publication.measurementGeneration
                 << ",\"hasCompletedEvidence\":" << (publication.hasCompletedEvidence ? "true" : "false")
                 << ",\"currentCompletedPresent\":" << (CurrentCompletedGraphicsPreviewPresent(publication) ? "true" : "false")
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
    const auto baseline = context.savedGraphics;
    const auto baselineBackend = context.rtScene.ExecutionBackend();
    const auto baselineProfile = context.rtScene.Profile();
    const auto expectedBackend = baselineBackend == horde::vulkan::RtExecutionBackend::RayQueryCompute ?
        horde::graphics::GraphicsBackend::RayQueryCompute : horde::graphics::GraphicsBackend::RayTracingPipeline;
    if (!baseline.glassEnabled || !(CurrentGraphicsSettings(context) == baseline))
        return fail("Capture must begin with the ordinary Glass On baseline, without saved preferences.");
    context.graphicsPreviewDelta = 0.0;
    // Every readback follows ordinary successful presentation of the current
    // output, never a SUBOPTIMAL frame whose resources were just recreated.
    const auto settle = [&](const int frameCount, std::vector<double>& timings) {
        timings.clear(); unsigned retries = 0u;
        for (int frame = 0; frame < frameCount; ++frame)
        {
            std::vector<double> replacementTimingSamples;
            if (!ApplyPendingSceneReplacement(context, capabilities, replacementTimingSamples) ||
                !ApplyPendingOutputResize(context, capabilities, replacementTimingSamples)) return false;
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
    const auto capturePose = [&](const std::string& name,
                                 const horde::platform::windows::GraphicsPreviewCapturePose& pose,
                                 const horde::graphics::GraphicsSettings& expectedSettings) {
        const auto captureFailure = [&](const std::string& reason) { diagnostic = reason; return false; };
        if (!horde::platform::windows::StageGraphicsPreviewCapturePose(context.graphicsPreview, pose))
            return captureFailure("Shared deterministic preview timeline or paused pose changed unexpectedly.");
        GraphicsPreviewCaptureRecord record; record.name = name;
        record.pose = context.graphicsPreview.Snapshot();
        if (!settle(kCaptureSettlingFrames, record.renderCallMilliseconds))
            return captureFailure(record.name + " did not reach stable ordinary RT presentation: " + context.lastRtFrameError);
        const auto renderedPose = context.graphicsPreview.Snapshot();
        if (renderedPose.tick != pose.tick || renderedPose.timelineEpoch != record.pose.timelineEpoch)
            return captureFailure(record.name + " changed the frozen authored timeline during rendering.");
        const VkResult idle = vkDeviceWaitIdle(context.device);
        if (idle != VK_SUCCESS || !CompleteRtEvidenceAfterDeviceIdle(context, idle))
            return captureFailure(record.name + " could not drain current RT evidence.");
        horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
        const auto readbackStart = std::chrono::steady_clock::now();
        if (!context.rtScene.CaptureStorageImage(image, diagnostic)) return captureFailure(record.name + " readback failed: " + diagnostic);
        record.readbackMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - readbackStart).count();
        record.filename = record.name + ".png";
        if (!WriteRgbaPng(outputDirectory / record.filename, image, diagnostic) ||
            !Sha256File(outputDirectory / record.filename, record.pngSha256, diagnostic))
            return captureFailure(record.name + " PNG/hash failed: " + diagnostic);
        const horde::graphics::GraphicsCommand current{0u, 1u, horde::graphics::GraphicsCommandKind::Apply, expectedSettings};
        record.settings = GraphicsSnapshot(context, current); record.settings.rtPresented = true;
        if (image.width != record.settings.internalExtent.width || image.height != record.settings.internalExtent.height ||
            !(record.settings.effective == expectedSettings)) return captureFailure("Capture dimensions or selected graphics differ from actual presented resources.");
        record.redBlueSwapNormalised = image.redBlueSwapNormalised;
        record.resources = context.rtScene.ResourceInventory();
        record.publication = context.rtFrameEvidence.PublishedStateByValue();
        if (record.resources.topLevelAccelerationStructureCount != 1u || record.resources.tlasInstanceCount != 7u)
            return captureFailure("Compact preview did not retain exactly its seven production TLAS roles.");
        record.sceneGlassEnabled = context.rtScene.GlassEnabled();
        const auto& masks = context.rtScene.LastInstanceMasks();
        record.instanceMasksByCustomIndex.assign(masks.begin(), masks.end());
        if (record.sceneGlassEnabled != expectedSettings.glassEnabled ||
            (expectedSettings.glassEnabled ? masks[9u] == 0u : masks[9u] != 0u) ||
            context.rtScene.Profile() != baselineProfile || context.rtScene.ExecutionBackend() != baselineBackend ||
            !CurrentCompletedGraphicsPreviewPresent(record.publication) ||
            record.publication.completedEvidence.identity.submitted.frame.simulationTick != pose.tick)
            return captureFailure(record.name + " lacks current completed RT presentation or the selected live glass mask/profile/backend.");
        captures.push_back(std::move(record));
        std::cout << "Captured graphics preview " << name << " at tick " << pose.tick << '\n';
        return true;
    };
    context.graphicsEdit.emplace(context.savedGraphics);
    const auto command = [&](const std::string& phase, const std::optional<horde::graphics::GraphicsCommand>& request,
                             const bool confirm) {
        if (!request) return false;
        const auto before = context.rtFrameEvidence.PublishedStateByValue();
        const bool glassChanged = request->requested.glassEnabled != context.rtScene.GlassEnabled();
        if (!QueueGraphicsCommand(context, request)) return false;
        std::vector<double> timings;
        if (!settle(2, timings) || context.graphicsCommand || !context.graphicsEdit->Effective()) return false;
        const auto snapshot = *context.graphicsEdit->Effective();
        const VkResult idle = vkDeviceWaitIdle(context.device);
        if (idle != VK_SUCCESS || !CompleteRtEvidenceAfterDeviceIdle(context, idle)) return false;
        const auto after = context.rtFrameEvidence.PublishedStateByValue();
        const auto& masks = context.rtScene.LastInstanceMasks();
        if (snapshot.serial != request->serial || snapshot.lifecycleGeneration != request->lifecycleGeneration ||
            !(snapshot.requested == request->requested) || !(snapshot.effective == request->requested) ||
            !snapshot.rtPresented || snapshot.backend != expectedBackend ||
            snapshot.scene != horde::graphics::GraphicsScene::Preview ||
            context.rtScene.ExecutionBackend() != baselineBackend || context.rtScene.Profile() != baselineProfile ||
            context.rtScene.GlassEnabled() != request->requested.glassEnabled ||
            context.renderScaleDirty || context.sceneProfileDirty || context.glassGeometryDirty ||
            !CurrentCompletedGraphicsPreviewPresent(after) ||
            after.completedEvidence.identity.submitted.submissionSerial <= before.completedEvidence.identity.submitted.submissionSerial ||
            (glassChanged && (after.sceneEpoch <= before.sceneEpoch ||
                              after.measurementGeneration < before.measurementGeneration)) ||
            (request->requested.glassEnabled ? masks[9u] == 0u : masks[9u] != 0u)) return false;
        if (confirm && !context.graphicsEdit->Confirm()) return false;
        GraphicsPreviewCaptureTransaction transaction;
        transaction.phase = phase; transaction.snapshot = snapshot; transaction.confirmedInMemory = confirm;
        transaction.before = before; transaction.after = after;
        transaction.sceneGlassEnabled = context.rtScene.GlassEnabled();
        transaction.instanceMasksByCustomIndex.assign(masks.begin(), masks.end());
        transactions.push_back(std::move(transaction)); return true;
    };
    auto candidate = context.savedGraphics; candidate.previewFrameCap = 60;
    if (!context.graphicsEdit->Stage(candidate) || !command("apply-cap-60", context.graphicsEdit->RequestApply(1u), false) ||
        !command("revert-cap-30", context.graphicsEdit->RequestRevert(1u), false) ||
        !context.graphicsEdit->Stage(candidate) || !command("keep-cap-60-memory-only", context.graphicsEdit->RequestApply(1u), true) ||
        !context.graphicsEdit->Stage(context.savedGraphics) || !command("restore-baseline-memory-only", context.graphicsEdit->RequestApply(1u), true))
        return fail("Production Apply/Revert/Keep did not acknowledge actual current RT presentation.");
    const horde::platform::windows::GraphicsPreviewCapturePose glassPose{
        "glass", horde::graphics::GraphicsPreviewCamera::Glass, 120u, false, false};
    auto glassOff = baseline; glassOff.glassEnabled = false;
    if (!capturePose("glass-on", glassPose, baseline)) return fail(diagnostic);
    if (!context.graphicsEdit->Stage(glassOff) ||
        !command("apply-glass-off", context.graphicsEdit->RequestApply(1u), false))
        return fail("Glass Off Apply did not complete a fresh current-resource RT presentation.");
    if (!capturePose("glass-off", glassPose, glassOff)) return fail(diagnostic);
    const auto& glassOnPose = captures[captures.size() - 2u].pose;
    const auto& glassOffPose = captures.back().pose;
    if (glassOnPose.tick != glassOffPose.tick || glassOnPose.camera.x != glassOffPose.camera.x ||
        glassOnPose.camera.z != glassOffPose.camera.z || glassOnPose.camera.yaw != glassOffPose.camera.yaw ||
        glassOnPose.camera.pitch != glassOffPose.camera.pitch || !glassOnPose.paused || !glassOffPose.paused ||
        glassOnPose.motionTest || glassOffPose.motionTest ||
        glassOnPose.skeleton.animationTime != glassOffPose.skeleton.animationTime ||
        glassOnPose.fireEmitters[0].phase != glassOffPose.fireEmitters[0].phase ||
        glassOnPose.fireEmitters[1].phase != glassOffPose.fireEmitters[1].phase)
        return fail("Glass On/Off readbacks differ in their shared authored paused camera/actor/fire pose.");
    if (!command("revert-glass-on", context.graphicsEdit->RequestRevert(1u), false) ||
        !context.graphicsEdit->Stage(glassOff) ||
        !command("keep-glass-off-memory-only", context.graphicsEdit->RequestApply(1u), true) ||
        !context.graphicsEdit->Stage(baseline) ||
        !command("restore-glass-on-memory-only", context.graphicsEdit->RequestApply(1u), true))
        return fail("Glass Revert/Keep/restore On did not acknowledge the exact fresh RT resources.");
    context.graphicsEdit.reset(); context.graphicsCommand.reset();
    context.graphicsPreviewDelta = 0.0;
    for (const auto& pose : horde::platform::windows::kGraphicsPreviewCapturePoses)
        if (!capturePose(std::string(pose.name), pose, baseline)) return fail(diagnostic);
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
