// Included after the ordinary frame/recreation APIs. This is a temporary,
// explicit launch owner: no GraphicsEditSession or persistence transaction.
horde::graphics::GraphicsSettings BenchmarkProfileIntent(const VulkanSurfaceContext& c)
{
    return {static_cast<int>(std::lround(c.renderScale * 100.0f)),
        static_cast<horde::graphics::WaterQuality>(c.waterQuality), c.fireDetail, c.graphicsPreviewFrameCap,
        c.requestedGlassEnabled, c.shadowQuality, c.requestedMistEnabled, c.requestedDustQuality};
}
void SetBenchmarkProfileIntent(VulkanSurfaceContext& c, const horde::graphics::GraphicsSettings& g)
{
    c.renderScale = static_cast<float>(g.renderScalePercent) / 100.0f;
    c.waterQuality = static_cast<horde::vulkan::raytracing::WaterQuality>(g.waterQuality);
    c.fireDetail = g.fireDetail; c.graphicsPreviewFrameCap = g.previewFrameCap;
    c.requestedGlassEnabled = g.glassEnabled; c.shadowQuality = g.shadowQuality;
    c.requestedMistEnabled = g.mistEnabled; c.requestedDustQuality = g.dustQuality;
    // Full swapchain recreation below consumes these values together.
    c.renderScaleDirty = c.sceneProfileDirty = c.glassGeometryDirty = false;
}
horde::platform::windows::OutputResizeOwnedFrame BenchmarkProfileOwner(
    const horde::telemetry::RtSubmittedFrameIdentity& i)
{
    return {i.frame.sceneEpoch, i.frame.measurementGeneration, i.submissionSerial, i.frame.simulationTick,
        i.frame.recordAttemptSerial, i.frame.recordSerial, i.frame.frameSlot};
}
bool ReadBenchmarkProfileActual(const VulkanSurfaceContext& c,
    horde::gameplay::BenchmarkProfileConfiguration& actual, const bool fixed)
{
    using namespace horde;
    const auto p = c.rtFrameEvidence.PublishedStateByValue();
    const auto& s = p.completedEvidence;
    const auto& owner = s.identity.submitted;
    if (!p.hasCompletedEvidence || !p.presented || owner.frame.sceneEpoch != p.sceneEpoch ||
        owner.frame.measurementGeneration != p.measurementGeneration || !s.identity.completionSerial ||
        s.presentation.outcome != telemetry::RtPresentationOutcome::Presented ||
        !s.scene.dispatch.sceneReady || !s.scene.dispatch.rtDispatchRecorded || !s.scene.dispatch.swapchainCopyRecorded ||
        !s.scene.dimensions || !telemetry::ValidRtFrameDimensionsEvidence(*s.scene.dimensions) ||
        (fixed && (s.scene.dimensions->presentationTransform != 0 || !telemetry::RtRecordedAspectPreserved(*s.scene.dimensions))) ||
        !s.scene.fireQuality || !s.scene.shadowQuality || !s.scene.actualUploadedMistEnabled ||
        !s.scene.actualUploadedDustQuality || !c.rtScene.IsReady() || !c.useRtPath ||
        c.sceneProfileDirty || c.glassGeometryDirty || c.renderScaleDirty || c.graphicsCommand) return false;
    const auto& d = *s.scene.dimensions;
    const auto live = c.rtScene.DispatchExtent();
    if (live.width != d.internalWidth || live.height != d.internalHeight ||
        c.swapchainExtent.width != d.outputWidth || c.swapchainExtent.height != d.outputHeight) return false;
    actual = {};
    actual.platform = graphics::GraphicsPlatform::Windows;
    switch (s.scene.pipeline.executionMode) {
    case telemetry::RtExecutionMode::RayTracingPipeline: actual.backend = graphics::GraphicsBackend::RayTracingPipeline; break;
    case telemetry::RtExecutionMode::RayQueryCompute: actual.backend = graphics::GraphicsBackend::RayQueryCompute; break;
    default: return false;
    }
    if ((actual.backend == graphics::GraphicsBackend::RayTracingPipeline && c.executionBackend != vulkan::RtExecutionBackend::RayTracingPipeline) ||
        (actual.backend == graphics::GraphicsBackend::RayQueryCompute && c.executionBackend != vulkan::RtExecutionBackend::RayQueryCompute)) return false;
    switch (s.scene.pipeline.dielectricQuality) {
    case telemetry::RtDielectricQuality::High: actual.optical = graphics::OpticalProfile::High; break;
    case telemetry::RtDielectricQuality::Mobile: actual.optical = graphics::OpticalProfile::Mobile; break;
    default: return false;
    }
    actual.graphics = CurrentGraphicsSettings(c);
    switch (s.scene.pipeline.waterQuality) {
    case telemetry::RtWaterQuality::Off: actual.graphics.waterQuality = graphics::WaterQuality::Off; break;
    case telemetry::RtWaterQuality::Mobile: actual.graphics.waterQuality = graphics::WaterQuality::Mobile; break;
    case telemetry::RtWaterQuality::High: actual.graphics.waterQuality = graphics::WaterQuality::High; break;
    default: return false;
    }
    switch (s.scene.fireQuality->quality) {
    case telemetry::RtFireQuality::Mobile: actual.graphics.fireDetail = graphics::FireDetail::Mobile; break;
    case telemetry::RtFireQuality::High: actual.graphics.fireDetail = graphics::FireDetail::High; break;
    case telemetry::RtFireQuality::Low: actual.graphics.fireDetail = graphics::FireDetail::Low; break;
    default: return false;
    }
    switch (s.scene.shadowQuality->mode) {
    case telemetry::RtShadowMode::Current: actual.graphics.shadowQuality = graphics::ShadowQuality::Current; break;
    case telemetry::RtShadowMode::Higher: actual.graphics.shadowQuality = graphics::ShadowQuality::Higher; break;
    default: return false;
    }
    actual.graphics.mistEnabled = *s.scene.actualUploadedMistEnabled;
    actual.graphics.dustQuality = *s.scene.actualUploadedDustQuality;
    // Glass is scene allocation state under this same completed scene epoch.
    actual.graphics.glassEnabled = c.rtScene.GlassEnabled();
    actual.internal = {d.internalWidth, d.internalHeight}; actual.output = {d.outputWidth, d.outputHeight};
    actual.cameraAspect = d.cameraAspect; actual.dynamicResolutionEnabled = d.dynamicResolutionEnabled;
    actual.presentationTransform = d.presentationTransform;
    actual.outputPolicy = fixed ? gameplay::BenchmarkOutputPolicy::FixedBufferPreserveAspect : gameplay::BenchmarkOutputPolicy::Native;
    actual.workload = gameplay::BenchmarkWorkload::ShowcaseRoute;
    return gameplay::ValidBenchmarkProfileConfiguration(actual);
}
bool PumpBenchmarkProfileMessages(VulkanSurfaceContext& c)
{
    MSG message{};
    while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            c.benchmarkProfileCancelRequested = c.benchmarkProfileCloseRequested = true;
            return false;
        }
        TranslateMessage(&message); DispatchMessageA(&message);
    }
    return IsWindow(c.windowHandle) != FALSE;
}
bool ObserveWindowsBenchmarkProfileMeasurement(VulkanSurfaceContext& c, const bool requireCompleted)
{
    if (!c.benchmarkProfile) return false;
    const auto p = c.rtFrameEvidence.PublishedStateByValue();
    const auto& owner = p.completedEvidence.identity.submitted.frame;
    if (!p.hasCompletedEvidence || owner.sceneEpoch != p.sceneEpoch ||
        owner.measurementGeneration != p.measurementGeneration) return !requireCompleted;
    horde::gameplay::BenchmarkProfileConfiguration actual;
    if (ReadBenchmarkProfileActual(c, actual, !c.benchmarkProfileCustom) && c.benchmarkProfile->ObserveMeasuring(actual)) return true;
    c.benchmarkProfileError = "Owning completed benchmark state changed from the acknowledged provisional tuple.";
    return false;
}
bool CaptureBenchmarkProfileCompletion(VulkanSurfaceContext& c, const VkClearColorValue& clear,
    const bool acknowledge, const bool fixed, const bool testStaleOwner = false)
{
    // Settle only WSI SUBOPTIMAL/out-of-date cases; never accept those as the
    // acknowledgement. A native-call/process watchdog remains the caller's job.
    for (unsigned attempt = 0; attempt < 4; ++attempt) {
        if (!PumpBenchmarkProfileMessages(c)) return false;
        const auto slot = c.currentFrame;
        bool presented = false;
        if (!RenderFrame(c, clear, presented)) return false;
        if (!presented || c.lastFramePresentation != horde::telemetry::RtPresentationOutcome::Presented) continue;
        horde::telemetry::RtSubmittedFrameIdentity submitted{};
        if (!c.rtFrameEvidence.TryGetCommittedIdentity(slot, submitted)) return false;
        const auto idle = vkDeviceWaitIdle(c.device);
        if (idle != VK_SUCCESS || !CompleteRtEvidenceAfterDeviceIdle(c, idle)) return false;
        const auto p = c.rtFrameEvidence.PublishedStateByValue();
        if (!p.hasCompletedEvidence || !horde::platform::windows::IsCurrentOutputResizeCompletion(
            BenchmarkProfileOwner(submitted), BenchmarkProfileOwner(p.completedEvidence.identity.submitted),
            p.completedEvidence.identity.completionSerial)) return false;
        horde::gameplay::BenchmarkProfileConfiguration actual;
        if (!ReadBenchmarkProfileActual(c, actual, fixed)) return false;
        if (!acknowledge) return true;
        if (!c.benchmarkProfile->Arm(BenchmarkProfileOwner(submitted))) return false;
        if (testStaleOwner) {
            auto stale = BenchmarkProfileOwner(p.completedEvidence.identity.submitted);
            ++stale.measurementGeneration;
            c.benchmarkProfileStaleOwnerRejected = !c.benchmarkProfile->Observe(actual, stale,
                p.completedEvidence.identity.completionSerial);
            if (!c.benchmarkProfileStaleOwnerRejected) return false;
        }
        return c.benchmarkProfile->Observe(actual, BenchmarkProfileOwner(p.completedEvidence.identity.submitted),
            p.completedEvidence.identity.completionSerial);
    }
    return false;
}
bool CaptureBenchmarkProfileImage(VulkanSurfaceContext& c, const std::filesystem::path& path)
{
    horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
    std::string error;
    return c.rtScene.CaptureStorageImage(image, error) && WriteRgbaPng(path, image, error);
}
bool ResizeBenchmarkProfileClient(VulkanSurfaceContext& c, const int width, const int height)
{
    MONITORINFO monitor{sizeof(MONITORINFO)};
    const bool monitorKnown = GetMonitorInfoA(MonitorFromWindow(c.windowHandle, MONITOR_DEFAULTTONEAREST), &monitor) != FALSE;
    c.benchmarkProfileMonitorRect = monitor.rcMonitor;
    const auto monitorWidth = monitor.rcMonitor.right - monitor.rcMonitor.left;
    const auto monitorHeight = monitor.rcMonitor.bottom - monitor.rcMonitor.top;
    if (!monitorKnown || width <= 0 || height <= 0 || width > monitorWidth || height > monitorHeight) {
        c.benchmarkProfileError = "Fixed client target " + std::to_string(width) + "x" + std::to_string(height) +
            " does not fit the actual monitor " + std::to_string(monitorWidth) + "x" + std::to_string(monitorHeight) + ".";
        return false;
    }
    const LONG left = monitor.rcMonitor.left + (monitorWidth - width) / 2;
    const LONG top = monitor.rcMonitor.top + (monitorHeight - height) / 2;
    c.benchmarkProfileTargetRect = {left, top, left + width, top + height};
    const auto setStyle = [&](const int index, const LONG_PTR value) {
        SetLastError(ERROR_SUCCESS);
        const auto previous = SetWindowLongPtrA(c.windowHandle, index, value);
        return previous != 0 || GetLastError() == ERROR_SUCCESS;
    };
    // Same borderless Win32 pattern as ToggleFullscreen, with an explicit fixed
    // client and preserved original style/menu rather than a display-mode change.
    c.benchmarkProfileInternalResize = true;
    const bool styleSet = setStyle(GWL_STYLE, WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN);
    const bool exStyleSet = setStyle(GWL_EXSTYLE, c.benchmarkProfileOriginalExStyle &
        ~static_cast<LONG_PTR>(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE));
    const bool menuSet = SetMenu(c.windowHandle, nullptr) != FALSE;
    const bool positioned = SetWindowPos(c.windowHandle, nullptr, left, top, width, height,
        SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED) != FALSE;
    c.fullscreen = true;
    c.benchmarkProfileAppliedStyle = GetWindowLongPtrA(c.windowHandle, GWL_STYLE);
    c.benchmarkProfileAppliedExStyle = GetWindowLongPtrA(c.windowHandle, GWL_EXSTYLE);
    c.benchmarkProfileTemporaryMenuDetached = GetMenu(c.windowHandle) == nullptr;
    const bool observed = GetClientRect(c.windowHandle, &c.benchmarkProfileObservedClient) &&
        GetWindowRect(c.windowHandle, &c.benchmarkProfileAppliedRect);
    const bool okay = styleSet && exStyleSet && menuSet && positioned && observed &&
        c.benchmarkProfileTemporaryMenuDetached &&
        c.benchmarkProfileObservedClient.right == width && c.benchmarkProfileObservedClient.bottom == height &&
        EqualRect(&c.benchmarkProfileAppliedRect, &c.benchmarkProfileTargetRect);
    c.benchmarkProfileInternalResize = false;
    if (!okay) c.benchmarkProfileError = "Borderless fixed client target " + std::to_string(width) + "x" + std::to_string(height) +
        " observed " + std::to_string(c.benchmarkProfileObservedClient.right) + "x" +
        std::to_string(c.benchmarkProfileObservedClient.bottom) + " on monitor " +
        std::to_string(monitorWidth) + "x" + std::to_string(monitorHeight) + ".";
    return okay;
}
bool BenchmarkProfilePlacementEquals(const WINDOWPLACEMENT& a, const WINDOWPLACEMENT& b)
{
    return a.length == b.length && a.flags == b.flags && a.showCmd == b.showCmd &&
        a.ptMinPosition.x == b.ptMinPosition.x && a.ptMinPosition.y == b.ptMinPosition.y &&
        a.ptMaxPosition.x == b.ptMaxPosition.x && a.ptMaxPosition.y == b.ptMaxPosition.y &&
        EqualRect(&a.rcNormalPosition, &b.rcNormalPosition);
}
bool ApplyWindowsBenchmarkProfile(VulkanSurfaceContext& c, const VkClearColorValue& clear,
    const std::string& validationCase)
{
    if (!c.benchmarkProfile) return false;
    c.benchmarkProfileActive = true;
    c.benchmarkProfileOriginalIntent = BenchmarkProfileIntent(c);
    c.benchmarkProfileOriginalSaved = c.savedGraphics;
    c.benchmarkProfileOriginalFullscreen = c.fullscreen;
    c.benchmarkProfileOriginalWindowedPlacement = c.windowedPlacement;
    c.benchmarkProfileOriginalMenu = GetMenu(c.windowHandle);
    SetLastError(ERROR_SUCCESS);
    c.benchmarkProfileOriginalStyle = GetWindowLongPtrA(c.windowHandle, GWL_STYLE);
    const bool styleKnown = c.benchmarkProfileOriginalStyle != 0 || GetLastError() == ERROR_SUCCESS;
    SetLastError(ERROR_SUCCESS);
    c.benchmarkProfileOriginalExStyle = GetWindowLongPtrA(c.windowHandle, GWL_EXSTYLE);
    const bool exStyleKnown = c.benchmarkProfileOriginalExStyle != 0 || GetLastError() == ERROR_SUCCESS;
    if (!styleKnown || !exStyleKnown || (!c.benchmarkProfileCustom && c.fullscreen) || c.graphicsEdit || c.graphicsCommand || c.sceneProfileDirty || c.renderScaleDirty ||
        c.glassGeometryDirty || !GetWindowPlacement(c.windowHandle, &c.benchmarkProfileOriginalPlacement) ||
        !GetWindowRect(c.windowHandle, &c.benchmarkProfileOriginalRect) ||
        !GetClientRect(c.windowHandle, &c.benchmarkProfileOriginalClient) ||
        !CaptureBenchmarkProfileCompletion(c, clear, false, false)) {
        c.benchmarkProfileError = "Original owning RT configuration/window state unavailable; profile not applied.";
        c.benchmarkProfileActive = false; return false;
    }
    c.benchmarkProfileBefore = c.rtFrameEvidence.PublishedStateByValue();
    horde::gameplay::BenchmarkProfileConfiguration original;
    if (!ReadBenchmarkProfileActual(c, original, false) || original.graphics != c.benchmarkProfileOriginalIntent ||
        c.benchmarkProfileCancelRequested || !c.benchmarkProfile->Begin(original, c.benchmarkProfileCustom ?
            horde::gameplay::BenchmarkProfileId::Custom : horde::gameplay::BenchmarkProfileId::WindowsProvisionalV1)) {
        c.benchmarkProfileError = "Original state is pending, cancelled, or compiled optics do not support the provisional profile.";
        c.benchmarkProfileActive = false; return false;
    }
    SetBenchmarkProfileIntent(c, c.benchmarkProfile->Working().graphics);
    const auto spec = horde::gameplay::BenchmarkProfileSpecification(horde::gameplay::BenchmarkProfileId::WindowsProvisionalV1);
    if (!c.benchmarkProfileCustom && (!ResizeBenchmarkProfileClient(c, static_cast<int>(spec->output.width), static_cast<int>(spec->output.height)) ||
        !RecreateSwapchain(c))) {
        if (c.benchmarkProfileError.empty()) c.benchmarkProfileError = "Fixed output recreation failed.";
        c.benchmarkProfileError += " Restoration required."; return false;
    }
    if (validationCase == "cancel-apply") {
        CancelBenchmark(c, false, "profile-validation-cancel-apply"); return false;
    }
    if (c.benchmarkProfileCancelRequested || !CaptureBenchmarkProfileCompletion(c, clear, true, !c.benchmarkProfileCustom, validationCase == "stale-owner")) {
        c.benchmarkProfileError = "Applied resources did not acknowledge the exact provisional RT tuple."; return false;
    }
    c.benchmarkProfileApplied = c.rtFrameEvidence.PublishedStateByValue();
    return !c.benchmarkProfileCancelRequested;
}
bool RestoreWindowsBenchmarkProfile(VulkanSurfaceContext& c, const VkClearColorValue& clear,
    const bool measurementPassed)
{
    if (!c.benchmarkProfile) return false;
    if (!c.benchmarkProfileActive) return c.benchmarkProfile->Phase() == horde::platform::windows::WindowsBenchmarkProfilePhase::Idle;
    (void)c.benchmarkProfile->BeginRestore(measurementPassed, c.benchmarkProfileCancelRequested);
    SetBenchmarkProfileIntent(c, c.benchmarkProfileOriginalIntent);
    c.benchmarkProfileInternalResize = true;
    bool windowRestored = false;
    if (IsWindow(c.windowHandle)) {
        const auto setStyle = [&](const int index, const LONG_PTR value) {
            SetLastError(ERROR_SUCCESS);
            const auto previous = SetWindowLongPtrA(c.windowHandle, index, value);
            return previous != 0 || GetLastError() == ERROR_SUCCESS;
        };
        // Attempt all original window-state restores even if an earlier API fails.
        const bool styleSet = setStyle(GWL_STYLE, c.benchmarkProfileOriginalStyle);
        const bool exStyleSet = setStyle(GWL_EXSTYLE, c.benchmarkProfileOriginalExStyle);
        const bool menuSet = SetMenu(c.windowHandle, c.benchmarkProfileOriginalMenu) != FALSE;
        const bool placementSet = SetWindowPlacement(c.windowHandle, &c.benchmarkProfileOriginalPlacement) != FALSE;
        const bool positioned = SetWindowPos(c.windowHandle, nullptr, c.benchmarkProfileOriginalRect.left, c.benchmarkProfileOriginalRect.top,
            c.benchmarkProfileOriginalRect.right - c.benchmarkProfileOriginalRect.left,
            c.benchmarkProfileOriginalRect.bottom - c.benchmarkProfileOriginalRect.top,
            SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED) != FALSE;
        windowRestored = styleSet && exStyleSet && menuSet && placementSet && positioned;
    }
    c.fullscreen = c.benchmarkProfileOriginalFullscreen;
    c.windowedPlacement = c.benchmarkProfileOriginalWindowedPlacement;
    c.benchmarkProfileInternalResize = false;
    RECT client{}, outer{};
    WINDOWPLACEMENT placement{sizeof(WINDOWPLACEMENT)};
    const bool exactWindow = windowRestored && GetClientRect(c.windowHandle, &client) &&
        GetWindowRect(c.windowHandle, &outer) && EqualRect(&client, &c.benchmarkProfileOriginalClient) &&
        EqualRect(&outer, &c.benchmarkProfileOriginalRect) && GetWindowPlacement(c.windowHandle, &placement) &&
        BenchmarkProfilePlacementEquals(placement, c.benchmarkProfileOriginalPlacement) &&
        GetWindowLongPtrA(c.windowHandle, GWL_STYLE) == c.benchmarkProfileOriginalStyle &&
        GetWindowLongPtrA(c.windowHandle, GWL_EXSTYLE) == c.benchmarkProfileOriginalExStyle &&
        GetMenu(c.windowHandle) == c.benchmarkProfileOriginalMenu;
    c.benchmarkProfileWindowStateRestored = exactWindow;
    const bool restored = exactWindow && c.savedGraphics == c.benchmarkProfileOriginalSaved &&
        c.device != VK_NULL_HANDLE && RecreateSwapchain(c) && CaptureBenchmarkProfileCompletion(c, clear, true, false) &&
        BenchmarkProfileIntent(c) == c.benchmarkProfileOriginalIntent;
    c.benchmarkProfileAfter = c.rtFrameEvidence.PublishedStateByValue();
    if (!restored) {
        c.benchmarkProfile->RestorationFailed();
        c.benchmarkProfileError += " Original output/quality RT restoration was not acknowledged.";
    }
    c.benchmarkProfileActive = false;
    return restored;
}
bool WriteWindowsBenchmarkProfileReceipt(VulkanSurfaceContext& c, const std::filesystem::path& directory,
    const std::string& validationCase)
{
    const auto publicationJson = [](const horde::telemetry::RtLifecyclePublishedState& p) {
        if (!p.hasCompletedEvidence) return std::string("null");
        std::string json, text, error;
        return horde::telemetry::SerializeRtEvidencePublication(p,
            horde::telemetry::RtEvidencePublicationSource::ActiveObserver, json, text, error) ? json : std::string("null");
    };
    std::ostringstream out;
    const auto rectJson = [&out](const RECT& r) {
        out << '[' << r.left << ',' << r.top << ',' << r.right << ',' << r.bottom << ']';
    };
    out << "{\"schema\":1,\"requestedProfile\":\"" << (c.benchmarkProfileCustom ? "custom" : "windows-rt-provisional-v1")
        << "\",\"classification\":\"custom-unranked\","
        "\"ranked\":false,\"validationCase\":\"" << JsonEscape(validationCase) << "\",\"actualApplyAcknowledged\":"
        << (c.benchmarkProfileApplied.hasCompletedEvidence ? "true" : "false") << ",\"actualRestorationAcknowledged\":"
        << (c.benchmarkProfile && c.benchmarkProfile->Restored() ? "true" : "false") << ",\"measurementAndRestorationPassed\":"
        << (c.benchmarkProfile && c.benchmarkProfile->Succeeded() ? "true" : "false") << ",\"cancelRequested\":"
        << (c.benchmarkProfileCancelRequested ? "true" : "false") << ",\"closeRequested\":"
        << (c.benchmarkProfileCloseRequested ? "true" : "false") << ",\"staleOwnerRejected\":"
        << (c.benchmarkProfileStaleOwnerRejected ? "true" : "false") << ",\"boundedAppliedFrameCount\":"
        << c.benchmarkProfileValidationFrames << ",\"boundedLifecycleValidationPassed\":"
        << (c.benchmarkProfileValidationPassed ? "true" : "false")
        << ",\"settingsWritesPermitted\":false,\"campaignAttached\":" << (c.campaign ? "true" : "false")
        << ",\"confirmedTupleUnchanged\":"
        << (c.savedGraphics == c.benchmarkProfileOriginalSaved ? "true" : "false") << ",\"error\":\""
        << JsonEscape(c.benchmarkProfileError) << "\",\"window\":{\"monitorRect\":";
    rectJson(c.benchmarkProfileMonitorRect); out << ",\"targetOuterRect\":";
    rectJson(c.benchmarkProfileTargetRect); out << ",\"observedAppliedClientRect\":";
    rectJson(c.benchmarkProfileObservedClient); out << ",\"appliedOuterRect\":";
    rectJson(c.benchmarkProfileAppliedRect); out << ",\"originalOuterRect\":";
    rectJson(c.benchmarkProfileOriginalRect); out << ",\"originalClientRect\":";
    rectJson(c.benchmarkProfileOriginalClient);
    out << ",\"originalStyleBits\":" << static_cast<DWORD>(c.benchmarkProfileOriginalStyle)
        << ",\"originalExStyleBits\":" << static_cast<DWORD>(c.benchmarkProfileOriginalExStyle)
        << ",\"appliedStyleBits\":" << static_cast<DWORD>(c.benchmarkProfileAppliedStyle)
        << ",\"appliedExStyleBits\":" << static_cast<DWORD>(c.benchmarkProfileAppliedExStyle)
        << ",\"originalMenuAttached\":" << (c.benchmarkProfileOriginalMenu ? "true" : "false")
        << ",\"temporaryMenuDetached\":" << (c.benchmarkProfileTemporaryMenuDetached ? "true" : "false")
        << ",\"menuRestored\":" << (IsWindow(c.windowHandle) && GetMenu(c.windowHandle) == c.benchmarkProfileOriginalMenu ? "true" : "false")
        << ",\"originalFullscreen\":" << (c.benchmarkProfileOriginalFullscreen ? "true" : "false")
        << ",\"fullscreenBookkeepingRestored\":" << (c.fullscreen == c.benchmarkProfileOriginalFullscreen &&
            BenchmarkProfilePlacementEquals(c.windowedPlacement, c.benchmarkProfileOriginalWindowedPlacement) ? "true" : "false")
        << ",\"exactWindowStateRestored\":" << (c.benchmarkProfileWindowStateRestored ? "true" : "false")
        << "},\"original\":" << publicationJson(c.benchmarkProfileBefore)
        << ",\"applied\":" << publicationJson(c.benchmarkProfileApplied) << ",\"restored\":"
        << publicationJson(c.benchmarkProfileAfter) << "}";
    const auto json = out.str();
    if (!c.benchmarkJsonReport.empty()) {
        if (const auto end = c.benchmarkJsonReport.rfind('}'); end != std::string::npos)
            c.benchmarkJsonReport.insert(end, ",\n\"windowsBenchmarkProfileAttempt\":" + json + "\n");
        if (!c.benchmarkProfileJsonPath.empty() && !WriteReportFile(c.benchmarkProfileJsonPath, c.benchmarkJsonReport)) return false;
    }
    return WriteReportFile(directory / "windows-benchmark-profile-lifecycle.json", json + "\n");
}
