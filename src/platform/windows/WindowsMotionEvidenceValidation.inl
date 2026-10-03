// Debug owner only. Simulation, submit and completion use ordinary native paths.
struct NativeMotionCaptureRow { std::string file, sha256; std::size_t stateRow=0, rtRow=0; };
int RunNativeMotionEvidence(VulkanSurfaceContext& context, horde::vulkan::DeviceCapabilities& capabilities,
                            const std::filesystem::path& directory)
{
    using horde::gameplay::validation::MotionStage;
    std::vector<NativeMotionCaptureRow> captures; captures.reserve(64);
    std::string executableSha256, error;
    const auto write = [&]() {
        std::ofstream ledger(directory/"native-motion-ledger.json",std::ios::binary|std::ios::trunc);
        if (!ledger) return false;
        context.motionLedger.WriteJson(ledger,context.motionScenario); ledger.flush();
        if (!ledger) return false;
        std::ostringstream manifest;
        manifest << "{\"schema\":1,\"complete\":"
            << (error.empty() && context.motionScenario.Complete() && !context.motionLedger.Failed() ? "true" : "false")
            << ",\"processId\":" << GetCurrentProcessId() << ",\"scenario\":\""
            << horde::gameplay::validation::MotionScenarioName(context.motionRequestedScenario)
            << "\",\"executableSha256\":\"" << executableSha256 << "\",\"gpu\":\""
            << JsonEscape(capabilities.identity.gpuName) << "\",\"apiVersion\":" << capabilities.identity.vulkanApiVersion
            << ",\"driverVersion\":" << capabilities.identity.driverVersion << ",\"backend\":\""
            << (context.executionBackend==horde::vulkan::RtExecutionBackend::RayQueryCompute ? "RayQueryCompute" : "RayTracingPipeline")
            << "\",\"presentationRetirement\":\"" << horde::vulkan::PresentCompletionDiagnostic(context.presentCompletionMode)
            << "\",\"isolation\":{\"preferencesLoaded\":false,\"preferencesWritten\":false,\"audioStarted\":false,"
            << "\"automatedMutedLane\":true,\"audioAcceptance\":false,\"ownerVisualAcceptance\":false,"
            << "\"secondSimulation\":false,\"phaseForced\":false,\"fixedDeltaOverride\":false}"
            << ",\"focusArmed\":" << (context.motionArmed ? "true" : "false")
            << ",\"limits\":{\"armingWallSeconds\":30,\"wallSeconds\":120,\"maximumCaptures\":64,\"fenceAcquireTimeoutSeconds\":2,"
            << "\"externalOwnedPidDeadlineRequired\":true,\"driverIdleReadbackCallsRemainProduction\":true,"
            << "\"frameScope\":\"RT-produced successful swapchain presentation and owning graphics completion, not scanout\","
            << "\"timingScope\":\"scripted run with milestone readback; not sustained performance comparison\"}"
            << ",\"error\":" << (error.empty() ? "null" : "\"" + JsonEscape(error) + "\"") << ",\"captures\":[";
        for (std::size_t i=0;i<captures.size();++i)
        {
            if(i) manifest << ',';
            manifest << "{\"file\":\"" << captures[i].file << "\",\"sha256\":\"" << captures[i].sha256
                << "\",\"stateRow\":" << captures[i].stateRow << ",\"rtRow\":" << captures[i].rtRow << '}';
        }
        manifest << "]}\n";
        return WriteReportFile(directory/"native-motion-manifest.json",manifest.str());
    };
    const auto fail = [&](std::string reason) {
        error=std::move(reason); context.motionScenario.Fail(error);
        (void)write(); std::cerr << "Native motion failed: " << error << '\n'; return 1;
    };
    unsigned existing=0;
    for(const auto& entry:std::filesystem::directory_iterator(directory))
        if(++existing>2 || !entry.is_regular_file() ||
           (entry.path().filename()!=kTextReportFilename && entry.path().filename()!=kJsonReportFilename))
        {
            std::cerr << "Native motion directory contains unrelated pre-existing output.\n";
            return 2; // Admission failed: never overwrite an unrelated report/ledger.
        }
    std::array<wchar_t,32768> executable{};
    const DWORD length=GetModuleFileNameW(nullptr,executable.data(),static_cast<DWORD>(executable.size()));
    std::string diagnostic;
    if(!length || length>=executable.size() || !Sha256File(executable.data(),executableSha256,diagnostic))
        return fail("Actual executable identity is unavailable: "+diagnostic);
    if(!context.nativeMotionValidation || !context.useRtPath || !context.rtScene.IsReady() ||
       !context.rtFrameEvidenceInitialised || context.rtScene.Profile()!=horde::vulkan::raytracing::RtSceneProfile::Showcase)
        return fail("A ready full Showcase hardware RT owner is required.");
    SetWindowTextA(context.windowHandle,"Horde Lantern RT - motion evidence: click this window to begin (30s)");
    const auto armingStart=std::chrono::steady_clock::now();
    for (;;)
    {
        MSG message{}; unsigned messages=0;
        while(PeekMessageA(&message,nullptr,0,0,PM_REMOVE))
        {
            if(++messages>1024) return fail("Native motion arming message backlog exceeded its bounded budget.");
            if(message.message==WM_QUIT) return fail("Native motion window was closed before arming.");
            TranslateMessage(&message); DispatchMessageA(&message);
        }
        if(!IsWindow(context.windowHandle)) return fail("Native motion window was destroyed before arming.");
        if(std::chrono::steady_clock::now()-armingStart>=std::chrono::seconds(30))
            return fail("Native motion was not armed by actual visible-window foreground focus within30s.");
        if(IsWindowVisible(context.windowHandle) && !IsIconic(context.windowHandle) &&
           GetForegroundWindow()==context.windowHandle) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    if(!context.motionScenario.Begin(context.motionRequestedScenario,context.simulation,
            horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr)))
        return fail(std::string(context.motionScenario.Failure()));
    context.motionRetryGeneration=context.simulation.Snapshot().retryGeneration;
    context.simulationPaused=false; context.simulationInput.paused=false;
    MirrorSimulationSnapshot(context);
    context.motionArmed=true;
    SetWindowTextA(context.windowHandle,"Horde Lantern RT - motion evidence running (keep this window focused)");
    auto publication=context.rtFrameEvidence.PublishedStateByValue();
    if(!context.motionLedger.ObserveScope(context.motionSurfaceGeneration,publication.sceneEpoch,publication.measurementGeneration))
        return fail(std::string(context.motionLedger.Failure()));
    const auto extent=context.swapchainExtent;
    const auto owningSwapchain=context.swapchain;
    auto owningSceneEpoch=publication.sceneEpoch;
    const auto clearColor=ClearColorForMode(capabilities.rtMode);
    double lastCaptureSeconds=-10;
    MotionStage lastCaptureStage=MotionStage::NotStarted;
    const auto drain = [&]() {
        const auto idle=vkDeviceWaitIdle(context.device);
        return CompleteRtEvidenceAfterDeviceIdle(context,idle) && !context.motionLedger.HasPendingSubmissions();
    };
    while(!context.motionScenario.Complete() && !context.motionScenario.Failed() && !context.motionLedger.Failed())
    {
        MSG message{}; unsigned messages=0;
        while(PeekMessageA(&message,nullptr,0,0,PM_REMOVE))
        {
            if(++messages>1024) return fail("Native motion message backlog exceeded its bounded owner budget.");
            if(message.message==WM_QUIT) return fail("Native motion window was closed.");
            TranslateMessage(&message); DispatchMessageA(&message);
        }
        if(!IsWindow(context.windowHandle) || IsIconic(context.windowHandle) || GetForegroundWindow()!=context.windowHandle)
            return fail("Native motion lost its active, visible native window.");
        RECT client{}; GetClientRect(context.windowHandle,&client);
        if(context.swapchain!=owningSwapchain ||
           context.rtFrameEvidence.PublishedStateByValue().sceneEpoch!=owningSceneEpoch ||
           client.right-client.left!=static_cast<LONG>(extent.width) ||
           client.bottom-client.top!=static_cast<LONG>(extent.height) || context.renderScaleDirty ||
           context.sceneProfileDirty || context.graphicsCommand || context.pauseMenuVisible || context.settingsVisible ||
           context.graphicsVisible || context.rtLabVisible || context.benchmark.IsRunning())
            return fail("Native motion was interrupted by resize, menu or rendering configuration changes.");
        if(context.motionScenario.Failed()) break;
        bool presented=false;
        const auto submittedSlot=context.currentFrame;
        if(!RenderFrame(context,clearColor,presented)) return fail("Ordinary native RT rendering failed: "+context.lastRtFrameError);
        if(!presented || context.lastFramePresentation!=horde::telemetry::RtPresentationOutcome::Presented)
            return fail("Ordinary current-resource RT presentation was interrupted/recreated.");
        capabilities.rtScene.presented=true;
        horde::telemetry::RtSubmittedFrameIdentity submitted{};
        if(!context.rtFrameEvidence.TryGetCommittedIdentity(submittedSlot,submitted))
            return fail("Actual submitted frame owner was unavailable.");
        const auto stage=context.motionScenario.Stage();
        const auto seconds=context.motionScenario.SimulationSeconds();
        const bool capture=stage!=lastCaptureStage || seconds-lastCaptureSeconds>=2.0;
        if(context.motionRetryPending || capture || context.motionScenario.Complete())
        {
            if(!drain()) return fail("Actual submitted motion frame did not drain its graphics ownership.");
            const auto frames=context.motionLedger.Frames();
            const auto owningFrame=std::find_if(frames.begin(),frames.end(),[&](const auto& row) {
                const auto& owner=row.identity.submitted;
                return owner.submissionSerial==submitted.submissionSerial &&
                    owner.frame.sceneEpoch==submitted.frame.sceneEpoch &&
                    owner.frame.measurementGeneration==submitted.frame.measurementGeneration &&
                    owner.frame.recordAttemptSerial==submitted.frame.recordAttemptSerial &&
                    owner.frame.recordSerial==submitted.frame.recordSerial &&
                    owner.frame.simulationTick==submitted.frame.simulationTick &&
                    owner.frame.frameSlot==submitted.frame.frameSlot &&
                    row.surfaceGeneration==context.motionSurfaceGeneration;
            });
            if(owningFrame==frames.end())
                return fail("Drained motion completion does not belong to the just-submitted frame.");
            if(capture)
            {
                if(captures.size()>=64) return fail("Native motion capture budget exceeded.");
                horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
                if(!context.rtScene.CaptureStorageImage(image,diagnostic)) return fail("Actual motion readback failed: "+diagnostic);
                std::ostringstream name; name<<std::setw(2)<<std::setfill('0')<<captures.size()<<'-'
                    <<horde::gameplay::validation::MotionStageName(stage)<<".png";
                NativeMotionCaptureRow row{name.str(),{},owningFrame->stateRow,
                    static_cast<std::size_t>(owningFrame-frames.begin())};
                if(!WriteRgbaPng(directory/row.file,image,diagnostic) || !Sha256File(directory/row.file,row.sha256,diagnostic))
                    return fail("Motion milestone PNG/identity failed: "+diagnostic);
                captures.push_back(std::move(row)); lastCaptureSeconds=seconds; lastCaptureStage=stage;
            }
            if(context.motionRetryPending)
            {
                // Retry already changed the real snapshot. Preserve the completed
                // old-scope frame, advance normal RT lifecycle once, then require
                // a new-scope completed presentation before resuming the inputs.
                if(!context.rtFrameEvidence.ApplyEvent(horde::telemetry::RtLifecycleEvent::Retry))
                    return fail("Actual retry could not advance RT lifecycle.");
                publication=context.rtFrameEvidence.PublishedStateByValue();
                owningSceneEpoch=publication.sceneEpoch; // Only the declared, actual Retry may advance this scope.
                if(!context.motionLedger.ObserveScope(context.motionSurfaceGeneration,publication.sceneEpoch,publication.measurementGeneration))
                    return fail(std::string(context.motionLedger.Failure()));
                context.motionRetryPending=false;
            }
        }
    }
    if(context.motionScenario.Failed()) return fail(std::string(context.motionScenario.Failure()));
    if(context.motionLedger.Failed()) return fail(std::string(context.motionLedger.Failure()));
    if(!drain()) return fail("Final graphics ownership did not complete.");
    publication=context.rtFrameEvidence.PublishedStateByValue();
    if(!context.motionLedger.HasCurrentPresentedFrame(context.motionSurfaceGeneration,publication.sceneEpoch,publication.measurementGeneration))
        return fail("Final actual scope has no completed RT-produced presentation.");
    if(!write()) return fail("Bounded motion evidence could not be written.");
    return 0;
}
