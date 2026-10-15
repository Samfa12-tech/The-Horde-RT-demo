// Opt-in development measurement only. Production timing, inputs and shaders are not
// modified. Unlike the faster deterministic capture, the route follows elapsed
// wall time, retaining complete60Hz ticks behind a four-tick catch-up bound.

bool RunChapterShippingRouteMeasurement(VulkanSurfaceContext& context,
    const std::filesystem::path& output,std::string& error)
{
    using namespace horde::gameplay::validation;
    using namespace horde::telemetry;
    // Windows may reject SetForegroundWindow for an automation-launched
    // process. Allow the owner to activate this test window once; never attach
    // other input threads or repeatedly steal focus. This setup wait is inside
    // the unchanged 180-second process deadline, before the 150-second route.
    if(ParseCaptureLaunchOptions().measureChapterForeground && GetForegroundWindow()!=context.windowHandle) {
        SetWindowTextW(context.windowHandle,L"Horde 1.7 timing test - click the title bar to begin");
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
        while(GetForegroundWindow()!=context.windowHandle) {
            MSG message{};
            while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {
                if(message.message==WM_QUIT) {error="Timing setup closed by owner.";return false;}
                TranslateMessage(&message);DispatchMessageW(&message);
            }
            if(std::chrono::steady_clock::now()>=deadline) {
                error="Timing setup never acquired owner foreground; no performance samples taken.";return false;
            }
            Sleep(16);
        }
        SetWindowTextW(context.windowHandle,L"Horde 1.7 - approved route timing test");
    }
    const auto initial=context.rtFrameEvidence.PublishedStateByValue();
    if(!initial.hasCompletedEvidence ||
       initial.completedEvidence.scene.pipeline.instrumentation!=RtInstrumentationMode::Shipping ||
       initial.completedEvidence.scene.pipeline.dielectricQuality!=RtDielectricQuality::High ||
       initial.completedEvidence.dielectric.compiled || initial.completedEvidence.dielectric.hasCounters ||
       context.inFlightFences.size()!=1) {
        error="Route measurement requires the actual Shipping/High artifact pair and no Diagnostic IO.";return false;
    }
    ChapterRoundTripProbe probe;
    if(!context.rtScene.OrdinaryStaticDetailAvailable()) {
        error="Actual chapter geometry did not admit the corridor static-detail certificate.";return false;
    }
    if(!probe.Start(context.simulation.Snapshot())) {
        error="Shipping route requires the admitted low-lantern reunion.";return false;
    }
    const auto directory=output/"shipping-route";
    std::error_code pathError;
    if(!std::filesystem::create_directory(directory,pathError)||pathError) {
        error="Shipping route evidence directory must be new and writable.";return false;
    }
    std::ofstream csv(directory/"completed-frames.csv"),snapshots(directory/"identities.jsonl"),
        systemLoad(directory/"system-load.csv");
    if(!csv||!snapshots||!systemLoad) {error="Shipping route evidence files could not open.";return false;}
    systemLoad<<"elapsed_s,available,total_cpu_busy_percent\n";
    csv<<"elapsed_s,present_return_interval_ns,present_interval_available,unoccluded_client,foreground_client,phase,station,simulation_step_cpu_ns,scene_epoch,generation,submission_serial,tick,gpu_status,gpu_ns,cpu_eligible,"
        <<"device_local_bytes,host_visible_bytes,strategy,shader_sha256,static_omitted_triangles,static_unloads,static_restores,static_saved_owner_bytes,static_peak_owner_overlap_bytes,static_wait_frames";
    for(std::size_t i=0;i<kRtStageCount;++i) {
        const auto name=RtStageMetricName(static_cast<RtStage>(i));
        csv<<','<<name<<"_ns,"<<name<<"_invocations,"<<name<<"_bytes,"<<name<<"_operations,"<<name<<"_overflow";
    }
    for(const auto name:kChapterHostPhaseNames)csv<<','<<name;
    csv<<",host_previous_evidence_ns,host_phases_available\n";
    struct Restore {
        VulkanSurfaceContext& ctx;bool paused,active;int sfx,voice,music;
        ~Restore() {
            ctx.simulationPaused=paused;ctx.chapterRoundTripCaptureActive=active;
            ctx.sfxVolumePercent=sfx;ctx.voiceVolumePercent=voice;ctx.musicVolumePercent=music;
            (void)ctx.rtFrameEvidence.SetPaused(paused);
        }
    } restore{context,context.simulationPaused,context.chapterRoundTripCaptureActive,
        context.sfxVolumePercent,context.voiceVolumePercent,context.musicVolumePercent};
    context.simulationPaused=false;context.chapterRoundTripCaptureActive=true;
    context.sfxVolumePercent=context.voiceVolumePercent=context.musicVolumePercent=0;
    (void)context.rtFrameEvidence.SetPaused(false);
    struct Tag {
        ChapterProbePhase phase=ChapterProbePhase::Failed;unsigned station=0;
        std::uint64_t serial=0,tick=0,simulationNanoseconds=0,presentIntervalNanoseconds=0;
        double elapsedSeconds=0;bool intervalAvailable=false,unoccluded=false,foreground=false;
        horde::vulkan::raytracing::PresentableTinyRtScene::OrdinaryStaticDetailMeasurements detail{};
        ChapterRouteHostPhases host;
    };
    // The sole in-flight graphics owner completes before reusing its slot.
    // Tags join that committed identity, never just a repeated simulation tick.
    // A missing/overwritten join fails closed instead of borrowing a later pose.
    std::array<Tag,16> tags{};
    const auto baseTick=context.simulation.Snapshot().tickIndex;
    const auto start=std::chrono::steady_clock::now();
    const auto startNs=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        start.time_since_epoch()).count());
    horde::platform::windows::WindowsPresentIntervals presentIntervals(startNs);
    const auto elapsed=[&] {return std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();};
    const auto steadyNs=[] {return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());};
    std::uint64_t previousPresentReturnNs=0,previousEvidenceNs=0;
    // Read aggregate machine CPU time once per second, without process names,
    // desktop pixels or a second input owner. This describes contention context;
    // it cannot attribute an individual acquire stall to another task.
    std::array<std::uint64_t,3> previousSystemTimes{};
    double lastSystemLoadSeconds=-1;
    const auto sampleSystemLoad=[&] {
        const double seconds=elapsed();
        if(seconds-lastSystemLoadSeconds<1)return;
        FILETIME idle{},kernel{},user{};
        const auto integer=[](FILETIME f) {return (std::uint64_t(f.dwHighDateTime)<<32)|f.dwLowDateTime;};
        bool available=false;double busy=0;
        if(GetSystemTimes(&idle,&kernel,&user)) {
            const std::array<std::uint64_t,3> current{integer(idle),integer(kernel),integer(user)};
            if(previousSystemTimes[1] && current[0]>=previousSystemTimes[0] &&
               current[1]>=previousSystemTimes[1] && current[2]>=previousSystemTimes[2]) {
                const auto total=(current[1]-previousSystemTimes[1])+(current[2]-previousSystemTimes[2]);
                const auto idleDelta=current[0]-previousSystemTimes[0];
                if(total && idleDelta<=total) {available=true;busy=100.*double(total-idleDelta)/double(total);}
            }
            previousSystemTimes=current;
        } else previousSystemTimes={};
        systemLoad<<seconds<<','<<available<<','<<busy<<'\n';
        lastSystemLoadSeconds=seconds;
    };
    auto lastSerial=initial.completedEvidence.identity.submitted.submissionSerial;
    ChapterProbePhase lastPhase=ChapterProbePhase::Failed;
    unsigned frames=0,samples=0;std::uint64_t maximumDebt=0;
    ChapterRouteMeasurementClock clock;
    const auto append=[&] {
        const auto publication=context.rtFrameEvidence.PublishedStateByValue();
        if(!publication.hasCompletedEvidence)return true;
        const auto& sample=publication.completedEvidence;
        const auto& id=sample.identity.submitted;
        if(id.submissionSerial==lastSerial)return true;
        const auto tag=tags[id.submissionSerial%tags.size()];
        if(id.submissionSerial<lastSerial || tag.serial!=id.submissionSerial || tag.tick!=id.frame.simulationTick ||
           id.frame.sceneEpoch!=initial.sceneEpoch || id.frame.measurementGeneration!=initial.measurementGeneration ||
           sample.presentation.outcome!=RtPresentationOutcome::Presented ||
           sample.presentation.lastSuccessfulPresentSubmissionSerial!=id.submissionSerial ||
           sample.scene.pipeline.instrumentation!=RtInstrumentationMode::Shipping || sample.dielectric.compiled ||
           sample.dielectric.hasCounters || sample.dielectric.readCount || sample.dielectric.resetCount) {
            error="Shipping route lost completed identity, owning route tag or instrumentation isolation.";return false;
        }
        RtEvidenceValidationError validation{};
        if(!ValidateRtPerformanceEvidence(sample,validation)) {
            error="Shipping route completed sample failed canonical validation.";return false;
        }
        csv<<tag.elapsedSeconds<<','<<tag.presentIntervalNanoseconds<<','<<tag.intervalAvailable<<','<<tag.unoccluded<<','<<tag.foreground<<','
            <<ChapterProbePhaseName(tag.phase)<<','<<tag.station<<','
            <<tag.simulationNanoseconds<<','<<id.frame.sceneEpoch<<','
            <<id.frame.measurementGeneration<<','<<id.submissionSerial<<','<<id.frame.simulationTick<<','
            <<RtSampleStatusName(sample.gpu.status)<<','<<(sample.gpu.hasDuration?sample.gpu.durationNanoseconds:0)<<','
            <<sample.cpuBenchmarkEligible<<','<<sample.scene.resources.deviceLocalBytes<<','
            <<sample.scene.resources.hostVisibleBytes<<','<<RtMaterialStrategyName(sample.scene.pipeline.activeStrategy)<<','
            <<RtFixedTextView(sample.scene.pipeline.active.sha256)<<','<<tag.detail.omittedTriangles<<','
            <<tag.detail.unloads<<','<<tag.detail.restores<<','<<tag.detail.savedOwnerBytes<<','
            <<tag.detail.peakOverlapOwnerBytes<<','<<tag.detail.waitFrames;
        for(const auto& stage:sample.scene.stages.values)
            csv<<','<<stage.durationNanoseconds<<','<<stage.workInvocationCount<<','<<stage.byteCount<<','
                <<stage.operationCount<<','<<stage.overflowed;
        for(const auto duration:tag.host.nanoseconds)csv<<','<<duration;
        csv<<','<<tag.host.previousEvidenceNanoseconds<<','<<tag.host.available<<'\n';
        if(samples%120==0 || tag.phase!=lastPhase) {
            std::string json;
            if(!SerializeRtPerformanceEvidenceJson(sample,json,validation)) {
                error="Shipping route identity serialization failed.";return false;
            }
            // Canonical JSON escapes line breaks inside strings. Remove only
            // real formatting CR/LF so each journal record is one valid line.
            std::erase_if(json,[](char c){return c=='\r'||c=='\n';});
            snapshots<<"{\"phase\":\""<<ChapterProbePhaseName(tag.phase)<<"\",\"station\":"<<tag.station
                <<",\"elapsedSeconds\":"<<tag.elapsedSeconds<<",\"evidence\":"<<json<<"}\n";
            csv.flush();snapshots.flush();
        }
        lastSerial=id.submissionSerial;lastPhase=tag.phase;++samples;
        if(!csv||!snapshots) {error="Shipping route evidence write failed.";return false;}
        return true;
    };
    const VkClearColorValue clear{{0,0,0,1}};
    // Conservative Z-order check of the actual client. Do not infer pacing from
    // hidden/minimized/covered presentation. No window titles or desktop pixels
    // are collected. Only an explicitly requested foreground comparison may
    // activate or raise its window before the measurement starts.
    const auto unoccludedClient=[&] {
        const auto window=context.windowHandle;
        if(!IsWindow(window)||!IsWindowVisible(window)||IsIconic(window)){error="client hidden, minimized or destroyed";return false;}
        RECT client{};if(!GetClientRect(window,&client)||client.right<=0||client.bottom<=0){error="invalid client extent";return false;}
        POINT origin{};if(!ClientToScreen(window,&origin)){error="client coordinate conversion failed";return false;}
        OffsetRect(&client,origin.x,origin.y);
        MONITORINFO monitor{sizeof(MONITORINFO)};
        if(!GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONULL),&monitor) ||
           client.left<monitor.rcMonitor.left||client.top<monitor.rcMonitor.top||
           client.right>monitor.rcMonitor.right||client.bottom>monitor.rcMonitor.bottom){error="client outside monitor bounds";return false;}
        for(auto above=GetWindow(window,GW_HWNDPREV);above;above=GetWindow(above,GW_HWNDPREV)) {
            if(!IsWindowVisible(above)||IsIconic(above))continue;
            DWORD cloaked=0;
            const auto result=DwmGetWindowAttribute(above,DWMWA_CLOAKED,&cloaked,sizeof(cloaked));
            if(FAILED(result)){error="DWM visibility unavailable";return false;}
            if(cloaked)continue;
            RECT bounds{},intersection{};
            if(!GetWindowRect(above,&bounds)){error="Z-order rectangle unavailable";return false;}
            if(IntersectRect(&intersection,&client,&bounds)){error="visible Z-order overlap";return false;}
        }
        return true;
    };
    while(frames++<30000 && probe.Phase()!=ChapterProbePhase::Complete) {
        const auto iterationStartNs=steadyNs();
        sampleSystemLoad();
        const auto systemEndNs=steadyNs();
        MSG message{};
        while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {
            if(message.message==WM_QUIT) {error="Route measurement closed by owner.";return false;}
            TranslateMessage(&message);DispatchMessageW(&message);
        }
        const auto messagesEndNs=steadyNs();
        const bool visible=unoccludedClient();
        if(!visible) {error="Release route requires an uncovered visible client ("+error+"); no focus was acquired.";return false;}
        const bool foreground=GetForegroundWindow()==context.windowHandle;
        if(ParseCaptureLaunchOptions().measureChapterForeground && !foreground) {
            error="Owner-authorized foreground measurement lost foreground; no background FPS claim is allowed.";return false;
        }
        const auto visibilityEndNs=steadyNs();
        const auto ticks=clock.Consume(elapsed());
        if(!ticks) {error="Shipping route exceeded its bounded150-second clock.";return false;}
        maximumDebt=std::max(maximumDebt,clock.PendingTicks());
        const auto simulationStart=std::chrono::steady_clock::now();
        for(unsigned step=0;step<*ticks && probe.Phase()!=ChapterProbePhase::Complete;++step) {
            const auto input=probe.Next(context.simulation.Snapshot());
            if(probe.Phase()==ChapterProbePhase::Failed) {error="Shipping route lost progression/lifecycle ownership.";return false;}
            context.simulationInput=input;
            context.simulation.StepFixed(input,1.f/60.f,context.simulation.Snapshot().inputPublicationSequence+1);
        }
        const auto tick=context.simulation.Snapshot().tickIndex;
        const auto simulationNs=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now()-simulationStart).count());
        const auto simulationEndNs=steadyNs();
        bool presented=false;
        if(!RenderFrame(context,clear,presented) || !presented ||
           context.lastFramePresentation!=RtPresentationOutcome::Presented ||
           context.simulation.Snapshot().tickIndex!=tick) {
            error="Shipping route lost stable RT presentation or advanced input twice: "+context.lastRtFrameError;return false;
        }
        RtSubmittedFrameIdentity submitted;
        if(!context.rtFrameEvidence.TryGetCommittedIdentity(0,submitted) || submitted.frame.simulationTick!=tick) {
            error="Shipping route did not commit its current simulation identity.";return false;
        }
        // Captured immediately after the successful vkQueuePresentKHR return,
        // before RenderFrame bookkeeping or this evidence join. The first
        // sample has no prior successful route present and is unavailable.
        const auto presentReturnNs=context.lastSuccessfulPresentReturnSteadyNs;
        const auto interval=presentIntervals.Consume(presentReturnNs);
        if(!interval) {
            error="Shipping route successful present-return clock unavailable or nonmonotonic.";return false;
        }
        const auto host=MeasureChapterRouteHostPhases({previousPresentReturnNs,iterationStartNs,systemEndNs,
            messagesEndNs,visibilityEndNs,simulationEndNs,presentReturnNs},previousEvidenceNs,interval->available);
        if(!host){error="Shipping route host-phase clocks lost monotonic interval ownership.";return false;}
        tags[submitted.submissionSerial%tags.size()]={probe.Phase(),static_cast<unsigned>(probe.Station()),
            submitted.submissionSerial,tick,simulationNs,interval->nanoseconds,
            static_cast<double>(presentReturnNs-startNs)*1.e-9,interval->available,visible,foreground,
            context.rtScene.OrdinaryStaticDetailEvidence(),*host};
        const auto evidenceStartNs=steadyNs();
        if(!append())return false;
        previousEvidenceNs=steadyNs()-evidenceStartNs;previousPresentReturnNs=presentReturnNs;
    }
    const auto finishSeconds=elapsed();
    const auto idle=vkDeviceWaitIdle(context.device);
    if(!CompleteRtEvidenceAfterDeviceIdle(context,idle)||!append()) {
        if(error.empty())error="Shipping route final GPU completion is unproved.";return false;
    }
    const auto& state=context.simulation.Snapshot();
    const auto& detail=context.rtScene.OrdinaryStaticDetailEvidence();
    if(probe.Phase()!=ChapterProbePhase::Complete || finishSeconds<60 || !state.rescue.exteriorSide ||
       state.rescue.equipmentStowed || !state.rescue.sawDescent || !state.rescue.sawLanding ||
       state.rescue.claimCount!=1 || state.rescue.deploymentCount!=1 || !state.chapterDialogue.proofAccepted) {
        error="Shipping route lacks sustained duration or complete two-way progression.";return false;
    }
    if(!detail.unloads || detail.restores!=detail.unloads || detail.savedOwnerBytes ||
       !context.rtScene.StaticGeometryHasCompleteContributors() || state.staticDetailLease.resourceGeneration) {
        error="Ordinary route did not unload eligible static detail and restore the complete owner before return.";return false;
    }
    horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
    if(!context.rtScene.CaptureStorageImage(image,error)||!WriteRgbaPng(directory/"final-repeat-ascent.png",image,error))return false;
    std::ostringstream summary;
#if defined(_DEBUG)
    constexpr auto build="Debug CPU";
#else
    constexpr auto build="Optimized Release CPU";
#endif
    summary<<"{\"schema\":4,\"scope\":\""<<build<<" with actual Shipping High shaders; visible uncovered wall-clock two-way route after staged Keeper death; no owner or phone acceptance\","
        <<"\"durationSeconds\":"<<finishSeconds<<",\"renderedFrames\":"<<frames-1<<",\"completedSamples\":"<<samples
        <<",\"simulationTicks\":"<<state.tickIndex-baseTick<<",\"maximumPendingSimulationTicks\":"<<maximumDebt
        <<",\"simulationHz\":60,\"maximumTicksPerRender\":4,\"deadlineSeconds\":150,\"warmupSeconds\":5,"
        <<"\"unoccludedClientChecks\":true,\"intervalMeaning\":\"wall time between successful present API returns, not GPU-only or exact display scanout timestamps\","
        <<"\"foregroundRequested\":"<<(ParseCaptureLaunchOptions().measureChapterForeground?"true":"false")<<','
        <<"\"staticDetailUnloads\":"<<detail.unloads<<",\"staticDetailRestores\":"<<detail.restores
        <<",\"maximumNormalStaticOwnerSavingBytes\":"<<detail.peakSavedOwnerBytes
        <<",\"finalStaticDetailSavingBytes\":"<<detail.savedOwnerBytes<<",\"staticPeakOwnerOverlapBytes\":"<<detail.peakOverlapOwnerBytes
        <<",\"staticReadinessWaitFrames\":"<<detail.waitFrames<<",\"complete\":true}\n";
    csv.flush();snapshots.flush();systemLoad.flush();
    if(!WriteReportFile(directory/"run.json",summary.str())) {error="Shipping route summary write failed.";return false;}
    error.clear();return true;
}
