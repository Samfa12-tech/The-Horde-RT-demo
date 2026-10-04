// Debug-only, included after the production renderer and capture helpers.
using OutputResizeHandleSnapshot = horde::vulkan::raytracing::PresentableTinyRtScene::ResourceHandleSnapshot;
struct OutputResizeValidationRecord
{
    std::string workload, filename, pngSha256, beforePublicationJson, afterPublicationJson;
    int previousScale = 100, scale = 100;
    bool baseline = false, redBlueSwapNormalised = false;
    std::uint64_t requestNanoseconds = 0, firstPresentNanoseconds = 0;
    double idleAndResizeMilliseconds = 0.0, readbackMilliseconds = 0.0;
    horde::graphics::GraphicsAppliedSnapshot graphics{};
    OutputResizeHandleSnapshot handlesBefore{}, handlesAfter{};
    horde::telemetry::RtResourceInventory resourcesBefore{}, resourcesAfter{};
    horde::telemetry::RtLifecyclePublishedState publicationBefore{}, publicationAfter{};
    horde::gameplay::simulation::SimulationSnapshot simulation{};
};
bool OutputResizeStableOwners(const OutputResizeHandleSnapshot& before, const OutputResizeHandleSnapshot& after)
{
    return before.ready && after.ready && !before.bottomLevelAccelerationStructures.empty() &&
        before.bottomLevelAccelerationStructures == after.bottomLevelAccelerationStructures &&
        before.topLevelAccelerationStructures == after.topLevelAccelerationStructures &&
        before.pipelines == after.pipelines && before.shaderBindingTableBuffers == after.shaderBindingTableBuffers &&
        before.descriptorSets == after.descriptorSets && before.textureImages == after.textureImages;
}
bool OutputResizeInventoryMatches(const horde::telemetry::RtResourceInventory& recorded,
                                  const horde::telemetry::RtResourceInventory& live)
{
    return recorded.bufferCount == live.bufferCount && recorded.memoryAllocationCount == live.memoryAllocationCount &&
        recorded.bottomLevelAccelerationStructureCount == live.bottomLevelAccelerationStructureCount &&
        recorded.topLevelAccelerationStructureCount == live.topLevelAccelerationStructureCount && recorded.tlasInstanceCount == live.tlasInstanceCount &&
        recorded.pipelineCount == live.pipelineCount && recorded.shaderBindingTableCount == live.shaderBindingTableCount &&
        recorded.descriptorSetCount == live.descriptorSetCount && recorded.hostVisibleBytes == live.hostVisibleBytes && recorded.deviceLocalBytes == live.deviceLocalBytes;
}
bool WriteOutputResizeValidationManifest(const std::filesystem::path& directory,
    const VulkanSurfaceContext& context, const horde::vulkan::DeviceCapabilities& capabilities,
    const std::string& executableSha256, const std::vector<OutputResizeValidationRecord>& records,
    const bool complete, const std::string& error)
{
    std::ostringstream json;
    json.imbue(std::locale::classic()); json << std::fixed << std::setprecision(6);
    const auto handlesJson = [&json](const OutputResizeHandleSnapshot& handles) {
        const auto vectorJson = [&json](const std::vector<std::uint64_t>& values) {
            json << '[';
            for (std::size_t index = 0; index < values.size(); ++index)
            { if (index) json << ','; json << "\"0x" << std::hex << values[index] << std::dec << '\"'; }
            json << ']';
        };
        json << "{\"ready\":" << (handles.ready ? "true" : "false") << ",\"blas\":";
        vectorJson(handles.bottomLevelAccelerationStructures); json << ",\"tlas\":"; vectorJson(handles.topLevelAccelerationStructures);
        json << ",\"pipelines\":"; vectorJson(handles.pipelines); json << ",\"sbtBuffers\":"; vectorJson(handles.shaderBindingTableBuffers);
        json << ",\"descriptorSets\":"; vectorJson(handles.descriptorSets); json << ",\"textureImages\":"; vectorJson(handles.textureImages);
        json << ",\"outputImage\":\"0x" << std::hex << handles.outputImage << "\",\"outputMemory\":\"0x" << handles.outputMemory
             << "\",\"outputView\":\"0x" << handles.outputView << std::dec << "\"}";
    };
    json << "{\n\"schemaVersion\":1,\"complete\":" << (complete ? "true" : "false")
         << ",\"sceneProfile\":\"Showcase\",\"source\":\"production-RT-storage-image\",\"sceneOnly\":true,\"overlaysIncluded\":false"
         << ",\"buildId\":\"" << JsonEscape(HORDE_RT_BUILD_ID) << "\",\"executableSha256\":\"" << executableSha256
         << "\",\"displayVersion\":\"" << JsonEscape(HORDE_RT_DISPLAY_VERSION)
         << "\",\"materialEncoding\":\"" << JsonEscape(context.rtScene.MaterialEncoding())
         << "\",\"executionBackend\":\"" << horde::vulkan::ToString(context.rtScene.ExecutionBackend())
         << "\",\"swapchainFormat\":" << static_cast<unsigned>(context.swapchainFormat)
         << ",\"outputExposure\":" << context.outputExposure
         << ",\"presentationRetirement\":\"" << horde::vulkan::PresentCompletionDiagnostic(context.presentCompletionMode)
         << "\",\"device\":{\"gpuName\":\"" << JsonEscape(capabilities.identity.gpuName)
         << "\",\"vendorId\":" << capabilities.identity.vendorId << ",\"deviceId\":" << capabilities.identity.deviceId
         << ",\"driverVersion\":" << capabilities.identity.driverVersion << ",\"apiVersion\":" << capabilities.identity.vulkanApiVersion
         << "},\"selectedCompiledCatalog\":{\"bundleIdentity\":\"" << JsonEscape(context.rtScene.SelectedPipelineBundleIdentity())
         << "\",\"artifacts\":[";
    unsigned artifactIndex = 0;
    for (const auto strategy : {horde::vulkan::raytracing::RtMaterialStrategy::OpaqueFast,
                               horde::vulkan::raytracing::RtMaterialStrategy::GenericDielectric})
    {
        if (artifactIndex++) json << ',';
        const auto artifact = context.rtScene.SelectedPipelineArtifactMetadata(strategy);
        if (!artifact) { json << "null"; continue; }
        json << "{\"key\":\"" << JsonEscape(std::string(artifact->canonicalKey)) << "\",\"spirvSha256\":\"" << artifact->spirvSha256
             << "\",\"includeSha256\":\"" << artifact->includeSha256 << "\",\"artifactPath\":\"" << JsonEscape(std::string(artifact->artifactPath))
             << "\",\"wordCount\":" << artifact->expectedWordCount << ",\"diagnosticsBinding\":" << (artifact->hasDiagnosticsBinding ? "true" : "false") << '}';
    }
    json << "]},\"isolation\":{\"userPreferencesLoaded\":false,\"userPreferencesWritten\":false,\"audioStarted\":false,"
         << "\"gameplayAdvancedDuringMeasurements\":false,\"gameplayEventsConsumedDuringMeasurements\":false,\"profileSwitched\":false}"
         << ",\"limits\":{\"frameFenceAndAcquireTimeoutSeconds\":2,\"maximumInitialSettleRecreates\":8,\"measuredRecreationPermitted\":false,"
         << "\"timingScope\":\"native request immediately before QueueGraphicsCommand through first ordinary successful vkQueuePresentKHR return; not scanout\","
         << "\"idleScope\":\"device idle plus evidence/timer reset plus output allocation/descriptor publication; excludes following render/present\","
         << "\"handleScope\":\"opaque process-local Vulkan handle identities; no memory contents or resident VRAM claim\","
         << "\"allocationScope\":\"tracked allocations; host/device property categories may overlap; no peak/budget/residency claim\","
         << "\"driverCalls\":\"device-idle/readback retain production behavior; launcher must enforce an external process deadline\","
         << "\"remainingGates\":[\"native UI and accessibility acceptance\",\"allocation failure/interruption recovery\",\"owner scene/audio acceptance\",\"Android device resize acceptance\"]}"
         << ",\"error\":" << (error.empty() ? "null" : "\"" + JsonEscape(error) + "\"") << ",\"measurements\":[\n";
    for (std::size_t index = 0; index < records.size(); ++index)
    {
        const auto& record = records[index];
        if (index) json << ",\n";
        const auto& snapshot = record.graphics; const auto& simulation = record.simulation;
        json << "{\"workload\":\"" << record.workload << "\",\"simulationPolicy\":\"frozen-production-authored-state\",\"baseline\":" << (record.baseline ? "true" : "false")
             << ",\"previousScalePercent\":" << record.previousScale << ",\"requestedScalePercent\":" << record.scale
             << ",\"resize_stall_ms\":";
        if (record.baseline) json << "null";
        else json << static_cast<double>(record.firstPresentNanoseconds - record.requestNanoseconds) / 1'000'000.0;
        json << ",\"requestSteadyNanoseconds\":" << record.requestNanoseconds << ",\"firstOrdinaryPresentReturnSteadyNanoseconds\":" << record.firstPresentNanoseconds
             << ",\"idle_and_resize_ms\":"; if (record.baseline) json << "null"; else json << record.idleAndResizeMilliseconds;
        json << ",\"readbackMs\":" << record.readbackMilliseconds << ",\"file\":\"" << record.filename << "\",\"pngSha256\":\"" << record.pngSha256
             << "\",\"redBlueSwapNormalised\":" << (record.redBlueSwapNormalised ? "true" : "false")
             << ",\"requestedGraphics\":{\"scale\":" << snapshot.requested.renderScalePercent << ",\"water\":" << static_cast<unsigned>(snapshot.requested.waterQuality)
             << ",\"fire\":" << static_cast<unsigned>(snapshot.requested.fireDetail)
             << ",\"shadow\":" << static_cast<unsigned>(snapshot.requested.shadowQuality)
             << ",\"glassEnabled\":" << (snapshot.requested.glassEnabled ? "true" : "false")
             << ",\"previewFrameCap\":" << snapshot.requested.previewFrameCap
             << "},\"effectiveGraphics\":{\"scale\":" << snapshot.effective.renderScalePercent
             << ",\"water\":" << static_cast<unsigned>(snapshot.effective.waterQuality) << ",\"fire\":" << static_cast<unsigned>(snapshot.effective.fireDetail)
             << ",\"shadow\":" << static_cast<unsigned>(snapshot.effective.shadowQuality)
             << ",\"glassEnabled\":" << (snapshot.effective.glassEnabled ? "true" : "false")
             << ",\"previewFrameCap\":" << snapshot.effective.previewFrameCap
             << ",\"internalWidth\":" << snapshot.internalExtent.width << ",\"internalHeight\":" << snapshot.internalExtent.height
             << ",\"outputWidth\":" << snapshot.outputExtent.width << ",\"outputHeight\":" << snapshot.outputExtent.height
             << ",\"opticalProfile\":\"" << (snapshot.opticalProfile == horde::graphics::OpticalProfile::High ? "High" : "Mobile")
             << "\",\"commandSerial\":" << snapshot.serial << ",\"rtPresented\":" << (snapshot.rtPresented ? "true" : "false")
             << "},\"gameplay\":{\"tick\":" << simulation.tickIndex << ",\"inputPublicationSequence\":" << simulation.inputPublicationSequence
             << ",\"retryGeneration\":" << simulation.retryGeneration << ",\"cameraX\":" << simulation.playerX << ",\"cameraZ\":" << simulation.playerZ
             << ",\"yaw\":" << simulation.playerYawRadians << ",\"pitch\":" << simulation.playerPitchRadians
             << ",\"playerMountProfile\":\"" << (simulation.playerMountProfile == horde::gameplay::items::PlayerMountProfile::AnatomicalBody ? "AnatomicalBody" : "LegacyViewRelative")
             << "\",\"heldLightKind\":" << static_cast<unsigned>(simulation.interaction.heldLightKind)
             << ",\"heldLightPose\":" << static_cast<unsigned>(simulation.interaction.heldLightPose)
             << ",\"heldLightPoseProgress\":" << simulation.interaction.heldLightPoseProgress << ",\"fireEmitterCount\":" << simulation.fireEmitterCount << "}"
             << ",\"handlesBefore\":"; handlesJson(record.handlesBefore);
        json << ",\"handlesAfter\":"; handlesJson(record.handlesAfter);
        json << ",\"stableResolutionIndependentOwners\":true,\"outputReplaced\":" << (record.baseline ? "false" : "true")
             << ",\"publicationBefore\":" << record.beforePublicationJson << ",\"publicationAfter\":" << record.afterPublicationJson << '}';
    }
    json << "\n]}\n";
    return WriteReportFile(directory / "output-resize-validation-manifest.json", json.str());
}
int RunOutputResizeValidation(VulkanSurfaceContext& context, horde::vulkan::DeviceCapabilities& capabilities,
                              const std::filesystem::path& outputDirectory)
{
    std::vector<OutputResizeValidationRecord> records;
    std::string executableSha256, diagnostic;
    const auto fail = [&](const std::string& reason) {
        (void)WriteOutputResizeValidationManifest(outputDirectory, context, capabilities, executableSha256, records, false, reason);
        std::cerr << "Output resize validation failed: " << reason << '\n'; return 1;
    };
    for (const auto& entry : std::filesystem::directory_iterator(outputDirectory))
        if (!entry.is_regular_file() || (entry.path().filename() != kTextReportFilename && entry.path().filename() != kJsonReportFilename))
            return fail("Output contains an unrelated pre-existing file.");
    std::array<wchar_t, 32768> executablePath{};
    const auto length = GetModuleFileNameW(nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));
    if (!length || length >= executablePath.size() || !Sha256File(std::filesystem::path(executablePath.data()), executableSha256, diagnostic))
        return fail("Actual executable SHA256 unavailable: " + diagnostic);
    if (!context.outputResizeValidation || !context.useRtPath || !context.rtScene.IsReady() || !context.rtFrameEvidenceInitialised ||
        context.rtScene.Profile() != horde::vulkan::raytracing::RtSceneProfile::Showcase)
        return fail("A ready production full Showcase hardware RT scene and owning observer are required.");
    const auto clearColor = ClearColorForMode(capabilities.rtMode);
    const auto initialBackend = context.rtScene.ExecutionBackend();
    const auto productionGraphics = horde::graphics::BaselineGraphicsSettings(horde::graphics::GraphicsPlatform::Windows);
    if (context.rtScene.SelectedPipelineBundleIdentity().empty() ||
        !context.rtScene.SelectedPipelineArtifactMetadata(horde::vulkan::raytracing::RtMaterialStrategy::OpaqueFast) ||
        !context.rtScene.SelectedPipelineArtifactMetadata(horde::vulkan::raytracing::RtMaterialStrategy::GenericDielectric))
        return fail("Production baseline or exact selected shader artifact metadata is unavailable.");
    const auto serialise = [&](const horde::telemetry::RtLifecyclePublishedState& publication, std::string& json) {
        std::string text, error;
        return horde::telemetry::SerializeRtEvidencePublication(publication,
            horde::telemetry::RtEvidencePublicationSource::ActiveObserver, json, text, error);
    };
    const auto drain = [&]() {
        const auto idle = vkDeviceWaitIdle(context.device);
        const bool completed = CompleteRtEvidenceAfterDeviceIdle(context, idle);
        return idle == VK_SUCCESS && completed;
    };
    std::vector<double> timingSamples;
    for (const auto workload : horde::platform::windows::kOutputResizeWorkloads)
    {
        if (workload == "opening") ApplyCaptureCheckpoint(context, horde::gameplay::kShowcaseCheckpoints.front());
        else
        {
            if (!horde::gameplay::StageLanternBenchmark(context.simulation, horde::gameplay::BenchmarkWorkload::LanternHeldHigh))
                return fail("Production held-high scenario could not stage.");
            if (!context.rtFrameEvidence.ApplyEvent(horde::telemetry::RtLifecycleEvent::CheckpointChange))
                return fail("Production held-high scope could not reset current measurement evidence.");
        }
        context.simulation.ResetTiming(); context.simulation.ClearEvents();
        context.frameDeltaSeconds = 0.0f; context.simulationPaused = true; context.simulationInput.paused = true;
        context.simulationInput.damageEnabled = false; context.simulationInput.hasAuthoritativePlayerPose = false;
        MirrorSimulationSnapshot(context);
        const auto frozen = context.simulation.Snapshot();
        const auto frozenEvents = context.simulation.Events().NextSequence(); const auto frozenEventCount = context.simulation.Events().Size();
        const auto unchanged = [&]() {
            const auto& current = context.simulation.Snapshot();
            return current.tickIndex == frozen.tickIndex && current.inputPublicationSequence == frozen.inputPublicationSequence &&
                current.retryGeneration == frozen.retryGeneration && current.playerX == frozen.playerX && current.playerZ == frozen.playerZ &&
                current.playerYawRadians == frozen.playerYawRadians && current.playerPitchRadians == frozen.playerPitchRadians &&
                current.walkTime == frozen.walkTime && current.walkAmount == frozen.walkAmount &&
                current.interaction == frozen.interaction &&
                current.rewardLanternWorldFromHinge == frozen.rewardLanternWorldFromHinge &&
                current.torchLightStrength == frozen.torchLightStrength &&
                current.playerMountProfile == frozen.playerMountProfile && current.fireEmitterCount == frozen.fireEmitterCount &&
                current.finaleComplete == frozen.finaleComplete && context.simulation.Events().NextSequence() == frozenEvents &&
                context.simulation.Events().Size() == frozenEventCount;
        };
        unsigned recreateAttempts = 0;
        for (int frame = 0; frame < kCaptureSettlingFrames; ++frame)
        {
            bool presented = false;
            if (!RenderFrame(context, clearColor, presented)) return fail("Initial production settling render failed: " + context.lastRtFrameError);
            if (context.lastFramePresentation == horde::telemetry::RtPresentationOutcome::PresentedNeedsRecreate ||
                context.lastFramePresentation == horde::telemetry::RtPresentationOutcome::NotPresentedNeedsRecreate)
            { if (++recreateAttempts > 8) return fail("Initial settling repeatedly recreated output."); frame = -1; continue; }
            if (!presented || context.lastFramePresentation != horde::telemetry::RtPresentationOutcome::Presented) return fail("Initial frame was not ordinarily RT presented.");
        }
        if (!drain()) return fail("Initial owning completion drain failed.");
        if (!unchanged()) return fail("Frozen gameplay/events changed during initial presentation.");
        if (!(CurrentGraphicsSettings(context) == productionGraphics))
            return fail("Ordinary initial RT presentation did not upload the complete production baseline policy.");
        context.graphicsEdit.emplace(productionGraphics, context.graphicsSerialFloor);
        for (const auto scale : horde::platform::windows::kOutputResizeScaleSequence)
        {
            OutputResizeValidationRecord record; record.workload = std::string(workload); record.scale = scale;
            record.baseline = scale == horde::platform::windows::kOutputResizeScaleSequence.front() && records.size() % 4 == 0;
            record.previousScale = CurrentGraphicsSettings(context).renderScalePercent;
            record.simulation = frozen;
            record.handlesBefore = context.rtScene.CaptureResourceHandles(); record.resourcesBefore = context.rtScene.ResourceInventory();
            record.publicationBefore = context.rtFrameEvidence.PublishedStateByValue();
            const auto& beforeCompleted = record.publicationBefore.completedEvidence;
            const auto& beforeOwner = beforeCompleted.identity.submitted;
            if (!record.publicationBefore.hasCompletedEvidence ||
                beforeCompleted.presentation.outcome != horde::telemetry::RtPresentationOutcome::Presented ||
                beforeOwner.frame.sceneEpoch != record.publicationBefore.sceneEpoch ||
                beforeOwner.frame.measurementGeneration != record.publicationBefore.measurementGeneration ||
                beforeOwner.frame.simulationTick != frozen.tickIndex ||
                !beforeCompleted.scene.dispatch.sceneReady || !beforeCompleted.scene.dispatch.rtDispatchRecorded ||
                !beforeCompleted.scene.dispatch.swapchainCopyRecorded ||
                !OutputResizeInventoryMatches(beforeCompleted.scene.resources, record.resourcesBefore) ||
                !serialise(record.publicationBefore, record.beforePublicationJson))
                return fail("Before transition lacks valid completed owning evidence.");
            if (!record.baseline)
            {
                auto settings = productionGraphics; settings.renderScalePercent = scale;
                if (!context.graphicsEdit->Stage(settings)) return fail("Production settings draft rejected.");
                const auto request = context.graphicsEdit->RequestApply(1u);
                if (!request) return fail("Production Apply request rejected.");
                context.resizeFirstPresentNanoseconds = 0; context.resizePresentTimestampArmed = true;
                record.requestNanoseconds = horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr);
                if (!QueueGraphicsCommand(context, request) || !ApplyPendingOutputResize(context, capabilities, timingSamples))
                    return fail("Production output transaction failed.");
                record.idleAndResizeMilliseconds = context.lastOutputResizeIdleMilliseconds;
                bool presented = false;
                if (!RenderFrame(context, clearColor, presented) || !presented ||
                    context.lastFramePresentation != horde::telemetry::RtPresentationOutcome::Presented)
                    return fail("Measured transition recreated, failed, or did not ordinarily present current RT output.");
                record.firstPresentNanoseconds = context.resizeFirstPresentNanoseconds;
                horde::telemetry::RtSubmittedFrameIdentity submitted;
                if (!record.firstPresentNanoseconds || record.firstPresentNanoseconds < record.requestNanoseconds ||
                    !context.rtFrameEvidence.TryGetCommittedIdentity(context.currentFrame, submitted))
                    return fail("First ordinary presentation has no timestamp or committed owner.");
                FinishGraphicsFrame(context, presented);
                if (context.graphicsCommand || !context.graphicsEdit->Effective()) return fail("Apply did not acknowledge the first current ordinary RT presentation.");
                record.graphics = *context.graphicsEdit->Effective();
                if (!drain()) return fail("First presented submission could not drain its owning evidence.");
                record.publicationAfter = context.rtFrameEvidence.PublishedStateByValue();
                const auto& completed = record.publicationAfter.completedEvidence;
                const auto& owner = completed.identity.submitted;
                if (!record.publicationAfter.hasCompletedEvidence ||
                    !horde::platform::windows::IsCurrentOutputResizeCompletion(
                        {submitted.frame.sceneEpoch, submitted.frame.measurementGeneration, submitted.submissionSerial, submitted.frame.simulationTick,
                         submitted.frame.recordAttemptSerial, submitted.frame.recordSerial, submitted.frame.frameSlot},
                        {owner.frame.sceneEpoch, owner.frame.measurementGeneration, owner.submissionSerial, owner.frame.simulationTick,
                         owner.frame.recordAttemptSerial, owner.frame.recordSerial, owner.frame.frameSlot}, completed.identity.completionSerial) ||
                    completed.presentation.outcome != horde::telemetry::RtPresentationOutcome::Presented ||
                    owner.frame.sceneEpoch != record.publicationAfter.sceneEpoch || owner.frame.measurementGeneration != record.publicationAfter.measurementGeneration)
                    return fail("Drained publication does not own the first new-output presentation.");
                if (!context.graphicsEdit->Confirm()) return fail("In-memory confirmation rejected after actual resource acknowledgement.");
                context.savedGraphics = context.graphicsEdit->Committed();
            }
            else
            {
                const horde::graphics::GraphicsCommand current{0u, 1u, horde::graphics::GraphicsCommandKind::Apply, productionGraphics};
                record.graphics = GraphicsSnapshot(context, current); record.graphics.rtPresented = true;
                record.publicationAfter = record.publicationBefore;
            }
            record.handlesAfter = context.rtScene.CaptureResourceHandles(); record.resourcesAfter = context.rtScene.ResourceInventory();
            const auto& before = record.resourcesBefore; const auto& after = record.resourcesAfter;
            const auto& afterCompleted = record.publicationAfter.completedEvidence;
            if (context.rtScene.Profile() != horde::vulkan::raytracing::RtSceneProfile::Showcase || context.rtScene.ExecutionBackend() != initialBackend ||
                !afterCompleted.scene.dispatch.sceneReady || !afterCompleted.scene.dispatch.rtDispatchRecorded ||
                !afterCompleted.scene.dispatch.swapchainCopyRecorded ||
                !OutputResizeInventoryMatches(afterCompleted.scene.resources, record.resourcesAfter) ||
                record.handlesBefore.bottomLevelAccelerationStructures.size() != before.bottomLevelAccelerationStructureCount ||
                record.handlesAfter.bottomLevelAccelerationStructures.size() != after.bottomLevelAccelerationStructureCount ||
                !OutputResizeStableOwners(record.handlesBefore, record.handlesAfter) ||
                before.bufferCount != after.bufferCount || before.memoryAllocationCount != after.memoryAllocationCount ||
                before.bottomLevelAccelerationStructureCount != after.bottomLevelAccelerationStructureCount ||
                before.topLevelAccelerationStructureCount != after.topLevelAccelerationStructureCount || before.tlasInstanceCount != after.tlasInstanceCount ||
                before.pipelineCount != after.pipelineCount || before.shaderBindingTableCount != after.shaderBindingTableCount ||
                before.descriptorSetCount != after.descriptorSetCount)
                return fail("Resolution-independent actual owners/inventory changed during output resize.");
            if (!record.baseline && (record.handlesBefore.outputImage == record.handlesAfter.outputImage ||
                record.handlesBefore.outputMemory == record.handlesAfter.outputMemory || record.handlesBefore.outputView == record.handlesAfter.outputView ||
                record.publicationBefore.sceneEpoch == record.publicationAfter.sceneEpoch))
                return fail("Output was not replaced with a new owning scene epoch.");
            if (!record.handlesAfter.outputImage || !record.handlesAfter.outputMemory || !record.handlesAfter.outputView || !unchanged())
                return fail("Output handles are invalid or frozen gameplay/events changed.");
            if (!serialise(record.publicationAfter, record.afterPublicationJson)) return fail("Completed packet serialization rejected current evidence.");
            const auto actualExtent = context.rtScene.DispatchExtent();
            const auto expectedExtent = ScaledRenderExtent(context.swapchainExtent, scale / 100.0f);
            if (actualExtent.width != expectedExtent.width || actualExtent.height != expectedExtent.height ||
                record.graphics.effective.renderScalePercent != scale || !record.graphics.rtPresented ||
                record.graphics.effective.waterQuality != productionGraphics.waterQuality || record.graphics.effective.fireDetail != productionGraphics.fireDetail ||
                record.graphics.effective.shadowQuality != productionGraphics.shadowQuality ||
                record.graphics.effective.glassEnabled != productionGraphics.glassEnabled ||
                record.graphics.effective.previewFrameCap != productionGraphics.previewFrameCap)
                return fail("Actual acknowledged dimensions/quality differ from requested production output.");
            capabilities.rtScene.presented = true;
            capabilities.rtScene.executionBackend = context.rtScene.ExecutionBackend();
            horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
            const auto readbackStart = std::chrono::steady_clock::now();
            if (!context.rtScene.CaptureStorageImage(image, diagnostic) || image.width != actualExtent.width || image.height != actualExtent.height)
                return fail("Current RT output readback failed or dimensions differ: " + diagnostic);
            record.readbackMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - readbackStart).count();
            record.filename = horde::platform::windows::OutputResizeCaptureFilename(record.workload, scale, record.baseline);
            if (!WriteRgbaPng(outputDirectory / record.filename, image, diagnostic) || !Sha256File(outputDirectory / record.filename, record.pngSha256, diagnostic))
                return fail("PNG/hash failed: " + diagnostic);
            record.redBlueSwapNormalised = image.redBlueSwapNormalised;
            records.push_back(std::move(record));
            if (!WriteOutputResizeValidationManifest(outputDirectory, context, capabilities, executableSha256, records, false, "Validation in progress."))
                return fail("Could not checkpoint measurement evidence.");
        }
        context.graphicsEdit.reset(); context.graphicsCommand.reset();
    }
    if (!WriteOutputResizeValidationManifest(outputDirectory, context, capabilities, executableSha256, records, true, {}))
        return fail("Could not write complete output resize manifest.");
    std::cout << "Full Showcase output resize validation complete: frozen opening and held-high, six real scale transitions.\n";
    return 0;
}
