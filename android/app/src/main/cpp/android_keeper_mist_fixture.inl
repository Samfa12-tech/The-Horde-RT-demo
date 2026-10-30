// Closed Debug fixed workload, independent from live input/campaign state.
void RunAndRetireAndroidKeeperMistFixture(SwapchainContext& context)
{
    using namespace horde::gameplay::validation;
    const auto serial=context.environmentSurveySerial,generation=context.surfaceGeneration;
    const auto id=context.environmentSurveyId,path=context.reportDirectory+"/environment-survey-"+id;
    const auto graphics=context.graphicsSettings;
    const auto fixture=*context.keeperMistSnapshot;
    const auto setupTicks=context.keeperMistSetupTicks;
    const auto owner=gGameSimulation.Snapshot();
    std::vector<unsigned char> ownerBytes(sizeof(owner));
    std::memcpy(ownerBytes.data(),&gGameSimulation.Snapshot(),ownerBytes.size());
    const auto extent=context.rtScene.DispatchExtent(),surface=context.swapchainExtent;
    const auto gpu=context.capabilities.identity.gpuName;
    const auto driver=context.capabilities.identity.driverVersion;
    std::string error,frames,presentation="{}",image="null";
    unsigned count=0;std::uint64_t lastSerial=0,scene=0,measurement=0;
    const auto fail=[&](const char* why){if(error.empty())error=why;};
    const auto current=[&] {
        if(!gSwapchainRunning.load(std::memory_order_acquire) || !gSurfaceSessions.IsCurrent(generation) ||
           gSurfaceSessions.IsSuspended(generation))return false;
        std::lock_guard lock(gEnvironmentSurveyMutex);
        return gEnvironmentSurveyPolicy.Phase()==1 && gEnvironmentSurveyPolicy.Serial()==serial;
    };
    const auto frame=[&](bool retain) {
        if(!current()){fail("fixture-lifecycle-or-request-revoked");return false;}
        bool presented=false,recreated=false;
        if(!RenderFrame(context,presented,recreated) || !presented || recreated) {
            fail("fixture-real-RT-present-failed-or-recreated");return false;
        }
        // Exact owning completion. Per-frame idle/serialization means these
        // display intervals are a diagnostic workload, never gameplay FPS.
        const auto idle=vkDeviceWaitIdle(context.device);
        if(idle!=VK_SUCCESS || !CompleteRtEvidenceAfterDeviceIdle(context,idle)) {
            fail("fixture-GPU-completion-unproved");return false;
        }
        const auto publication=context.rtFrameEvidence.PublishedStateByValue();
        const auto& done=publication.completedEvidence;const auto& identity=done.identity.submitted;
        const auto& lights=done.scene.fireLighting;
        if(!publication.presented || !publication.hasCompletedEvidence ||
           !done.scene.dispatch.sceneReady || !done.scene.dispatch.rtDispatchRecorded || !done.scene.dispatch.swapchainCopyRecorded ||
           done.presentation.outcome!=horde::telemetry::RtPresentationOutcome::Presented ||
           done.presentation.lastSuccessfulPresentSubmissionSerial!=identity.submissionSerial ||
           identity.frame.simulationTick!=fixture.tickIndex || identity.submissionSerial<=lastSerial ||
           !done.gpu.hasDuration || !done.gpu.durationNanoseconds || !horde::telemetry::RtSampleAvailable(done.gpu.status) ||
           done.gpu.completedSubmissionSerial!=identity.submissionSerial ||
           !lights || lights->count!=2 || lights->emitters[0].stableId!=3 || lights->emitters[1].stableId!=4 ||
           lights->emitters[0].positionStrength[3]<=0 || lights->emitters[1].positionStrength[3]<=0 ||
           done.scene.actualUploadedMistEnabled!=std::optional<bool>(graphics.mistEnabled) ||
           context.graphicsSettings!=graphics || std::memcmp(ownerBytes.data(),&gGameSimulation.Snapshot(),ownerBytes.size())!=0) {
            fail("fixture-owning-frame-lights-mist-or-immutable-owner-mismatch");return false;
        }
        if(!scene){scene=identity.frame.sceneEpoch;measurement=identity.frame.measurementGeneration;}
        if(scene!=identity.frame.sceneEpoch || measurement!=identity.frame.measurementGeneration) {
            fail("fixture-evidence-scope-changed");return false;
        }
        std::string json;horde::telemetry::RtEvidenceValidationError validation{};
        if(!horde::telemetry::SerializeRtPerformanceEvidenceJson(done,json,validation)) {
            fail("fixture-canonical-evidence-rejected");return false;
        }
        lastSerial=identity.submissionSerial;
#if HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION || HORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING
        PollPresentTimingOnOwner(context);
#endif
        if(retain) {
            if(count)frames+=',';
            frames+="{\"index\":"+std::to_string(count)+",\"completedFrame\":"+json+"}";
            std::lock_guard lock(gEnvironmentSurveyMutex);
            if(!gEnvironmentSurveyPolicy.AcceptFrame(generation,scene,measurement,identity.submissionSerial)) {
                fail("fixture-frame-identity-admission-failed");return false;
            }
            ++count;
        }
        gSurfaceSessions.Publish(generation,1);return true;
    };
    if(!context.useRtPath || !context.rtScene.IsReady() || graphics.renderScalePercent!=40 ||
       !InitialiseRtEvidenceOnOwnerThread(context,false))fail("fixture-40percent-native-RT-unavailable");
#if HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION || HORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING
    if(!context.presentTiming || !context.presentTiming->Enabled() || !context.presentTiming->Bound())
        fail("fixture-actual-presentation-collector-unavailable");
#else
    fail("fixture-requires-existing-presentation-timing-build");
#endif
    for(unsigned i=0;i<12 && error.empty();++i)frame(false);
    if(error.empty()) {
        std::ostringstream ready;ready<<"{\"schema\":1,\"requestId\":"<<JsonUtf8String(id)
            <<",\"phase\":\"prepared\",\"setupTicks\":"<<setupTicks
            <<",\"twoUploadedLights\":[3,4],\"mist\":"<<(graphics.mistEnabled?"true":"false")
            <<",\"width\":"<<extent.width<<",\"height\":"<<extent.height<<"}";
        if(!WriteTextFile(path+"-ready.tmp",ready.str()) ||
           std::rename((path+"-ready.tmp").c_str(),(path+"-ready.json").c_str())!=0)fail("fixture-ready-publication-failed");
    }
    const auto preparedAt=std::chrono::steady_clock::now();bool armed=false;
    while(error.empty() && !armed) {
        if(std::chrono::steady_clock::now()-preparedAt>=std::chrono::seconds(30)) {
            fail("fixture-30second-arming-bound-reached");break;
        }
        std::ifstream arm(path+".arm");
        if(arm.good()) {std::string token;std::getline(arm,token);
            if(token!=id)fail("fixture-arm-request-identity-mismatch");else armed=true;
        } else frame(false);
    }
    const auto timedAt=std::chrono::steady_clock::now();
    while(error.empty() && count<kKeeperMistFrames) {
        if(std::chrono::steady_clock::now()-timedAt>=std::chrono::seconds(60)) {
            fail("fixture-60second-render-bound-reached");break;
        }
        frame(true);
    }
#if HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION || HORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING
    const auto drainAt=std::chrono::steady_clock::now();
    while(error.empty() && context.presentTiming && context.presentTiming->PendingCount()) {
        PollPresentTimingOnOwner(context);
        if(std::chrono::steady_clock::now()-drainAt>=std::chrono::seconds(2)) {
            fail("fixture-two-second-presentation-drain-failed");break;
        }
        std::this_thread::yield();
    }
    presentation=PresentTimingJson(context);
#endif
    // Single unmodified image only AFTER timed frames and the owning drain.
    if(error.empty()) {
        horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture pixels;std::string diagnostic;
        if(!context.rtScene.CaptureStorageImage(pixels,diagnostic) || pixels.width!=extent.width || pixels.height!=extent.height ||
           pixels.rgba.size()!=std::size_t(pixels.width)*pixels.height*4)fail("fixture-post-timing-image-unavailable");
        else {
            const auto name="environment-survey-"+id+"-final.rgba";
            std::ofstream bytes(context.reportDirectory+'/'+name,std::ios::binary|std::ios::trunc);
            bytes.write(reinterpret_cast<const char*>(pixels.rgba.data()),static_cast<std::streamsize>(pixels.rgba.size()));bytes.flush();
            if(!bytes)fail("fixture-post-timing-image-write-failed");
            else image="{\"file\":"+JsonUtf8String(name)+",\"width\":"+std::to_string(pixels.width)+",\"height\":"+std::to_string(pixels.height)+"}";
        }
    }
    gSwapchainRunning.store(false,std::memory_order_release);
    const bool retired=DestroySwapchainContext(context);gSurfaceRetirementBlocked.store(!retired,std::memory_order_release);
    if(!retired)fail("fixture-GPU-retirement-unproved");
    // Terminal commit is serialized with cancellation, after GPU retirement.
    // A pause wins before this lock, or observes the already committed result.
    std::unique_lock terminal(gEnvironmentSurveyMutex);
    if(gEnvironmentSurveyPolicy.Phase()!=1 || gEnvironmentSurveyPolicy.Serial()!=serial)
        fail("fixture-cancelled-before-terminal-commit");
    if(std::memcmp(ownerBytes.data(),&gGameSimulation.Snapshot(),ownerBytes.size())!=0)
        fail("fixture-owner-snapshot-changed-before-terminal-commit");
    const bool complete=error.empty() && armed && count==kKeeperMistFrames && retired;
    std::ostringstream report;report.imbue(std::locale::classic());report<<std::setprecision(9)
      <<"{\"schema\":1,\"requestId\":"<<JsonUtf8String(id)<<",\"profile\":\"keeper-mist\",\"result\":\""<<(complete?"complete":"failed")<<"\""
      <<",\"evidenceClass\":\"isolated frozen live-reveal state; fixed renderer GPU workload; per-frame idle/serialization; not moving fight or gameplay FPS\""
      <<",\"gpu\":"<<JsonUtf8String(gpu)<<",\"driverVersion\":"<<driver<<",\"surfaceGeneration\":"<<generation
      <<",\"setupTicks\":"<<setupTicks<<",\"simulationTick\":"<<fixture.tickIndex
      <<",\"ownerSimulationBytesUnchanged\":"<<(std::memcmp(ownerBytes.data(),&gGameSimulation.Snapshot(),ownerBytes.size())==0?"true":"false")
      <<",\"retirementProven\":"<<(retired?"true":"false")<<",\"armed\":"<<(armed?"true":"false")
      <<",\"warmupFrames\":"<<kKeeperMistWarmupFrames<<",\"sampleFrames\":"<<kKeeperMistSampleFrames
      <<",\"width\":"<<extent.width<<",\"height\":"<<extent.height<<",\"surfaceWidth\":"<<surface.width<<",\"surfaceHeight\":"<<surface.height
      <<",\"mist\":"<<(graphics.mistEnabled?"true":"false")<<",\"scale\":"<<graphics.renderScalePercent
      <<",\"graphics\":{\"scalePercent\":"<<graphics.renderScalePercent
      <<",\"water\":"<<int(graphics.waterQuality)<<",\"fire\":"<<int(graphics.fireDetail)
      <<",\"frameCap\":"<<graphics.previewFrameCap<<",\"glass\":"<<(graphics.glassEnabled?"true":"false")
      <<",\"shadow\":"<<int(graphics.shadowQuality)<<",\"mist\":"<<(graphics.mistEnabled?"true":"false")
      <<",\"dust\":"<<int(graphics.dustQuality)<<'}'
      <<",\"camera\":["<<fixture.playerX<<','<<fixture.playerSupportWorldY<<','<<fixture.playerZ<<','<<fixture.playerYawRadians<<','<<fixture.playerPitchRadians<<']'
      <<",\"keeper\":{\"x\":"<<fixture.lich.x<<",\"y\":"<<fixture.lich.y<<",\"z\":"<<fixture.lich.z
      <<",\"health\":"<<fixture.lich.health<<",\"readable\":"<<(fixture.lich.readableCastPresentation?"true":"false")
      <<",\"roomMistDensity\":"<<fixture.lich.roomMistDensityScale<<",\"staffLight\":"<<fixture.lich.staffLightStrength<<'}'
      <<",\"error\":"<<(error.empty()?"null":JsonUtf8String(error))<<",\"image\":"<<image
      <<",\"presentation\":"<<presentation<<",\"frames\":["<<frames<<"]}";
    const bool written=WriteTextFile(path+".json",report.str());
    {if(retired)gEnvironmentSurveyActiveSurface.store(0,std::memory_order_release);
     if(complete && written){if(!gEnvironmentSurveyPolicy.Complete())gEnvironmentSurveyPolicy.Cancel();}
     else if(gEnvironmentSurveyPolicy.Serial()==serial)gEnvironmentSurveyPolicy.Cancel();}
    terminal.unlock();
    __android_log_print(complete&&written?ANDROID_LOG_INFO:ANDROID_LOG_ERROR,kTag,
      "HORDE_KEEPER_MIST_FIXTURE id=%s frames=%u retired=%d complete=%d error=%s",id.c_str(),count,retired,complete&&written,error.c_str());
}
