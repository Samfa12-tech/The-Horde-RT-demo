// Debug-only finite renderer survey. No gameplay/campaign commands, simulation
// input, checkpoint staging, event drain or resumed ordinary render loop.
void RunAndRetireAndroidEnvironmentSurvey(SwapchainContext& context)
{
    using namespace horde::platform::android;
    constexpr unsigned deadlineSeconds=60;
    const auto serial=context.environmentSurveySerial,generation=context.surfaceGeneration;
    const auto id=context.environmentSurveyId,path=context.reportDirectory+"/environment-survey-"+id;
    const bool disabled=context.environmentSurveyWaterDisabled;
    const auto graphics=context.graphicsSettings;
    const auto gpu=context.capabilities.identity.gpuName;
    const auto driver=context.capabilities.identity.driverVersion;
    const auto backend=context.executionBackend;
    const auto initial=gGameSimulation.Snapshot();
    std::array<unsigned char,sizeof(initial)> initialSnapshotBytes{};
    std::memcpy(initialSnapshotBytes.data(),&gGameSimulation.Snapshot(),initialSnapshotBytes.size());
    bool snapshotBytesUnchanged=true;
    const auto terrain=context.rtScene.ChapterTerrainSurface();
    const auto extent=context.rtScene.DispatchExtent(),surfaceExtent=context.swapchainExtent;
    auto camera=RiverBendSurveyCamera();
    horde::gameplay::simulation::TriangleSupportSurface::Sample shore{};
    const auto started=std::chrono::steady_clock::now();
    std::string error,frames,images;unsigned attempted=0,imageCount=0,frameCount=0;
    std::uint64_t lastSubmission=0,scene=0,measurement=0;
    const auto fail=[&](const char* reason){if(error.empty())error=reason;};
    if(!terrain || !context.useRtPath || !context.rtScene.IsReady() ||
       !InitialiseRtEvidenceOnOwnerThread(context,false))fail("native-terrain-or-RT-evidence-unavailable");
    if(error.empty()) {
        shore=terrain->Query(camera.x,camera.z);
        if(!shore.found || !std::isfinite(shore.worldY))fail("native-terrain-shore-query-failed");
        else {
            camera.supportY=shore.worldY;
            const auto target=horde::scene::ChapterWaterFromAtlas(58,-25,6);
            const float eye=horde::gameplay::simulation::PlayerEyeWorldY(camera.supportY)-initial.playerCrouchDropMetres;
            camera.pitch=.05f+(target[1]-eye)/std::hypot(target[0]-camera.x,target[2]-camera.z);
            if(camera.pitch<-.32f || camera.pitch>.28f)fail("survey-camera-outside-existing-pitch-limits");
            context.environmentSurveyCamera=camera;
        }
    }
    for(unsigned frame=0;error.empty() && frame<kEnvironmentSurveyFrames;++frame) {
        attempted=frame+1;
        if(!gSwapchainRunning.load(std::memory_order_acquire) || !gSurfaceSessions.IsCurrent(generation) ||
           gSurfaceSessions.IsSuspended(generation)) {fail("survey-surface-or-lifecycle-cancelled");break;}
        {
            std::lock_guard lock(gEnvironmentSurveyMutex);
            if(gEnvironmentSurveyPolicy.Phase()!=1 || gEnvironmentSurveyPolicy.Serial()!=serial)
                fail("survey-request-cancelled");
        }
        if(!error.empty())break;
        if(std::chrono::steady_clock::now()-started>std::chrono::seconds(deadlineSeconds))
            {fail("survey-deadline-exceeded");break;}
        context.environmentSurveyTime=EnvironmentSurveyTime(frame);
        const auto begin=std::chrono::steady_clock::now();bool presented=false,recreated=false;
        if(!RenderFrame(context,presented,recreated) || !presented || recreated)
            {fail("survey-real-RT-present-failed-or-output-recreated");break;}
        const double renderMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
        const auto idle=vkDeviceWaitIdle(context.device);
        if(idle!=VK_SUCCESS || !CompleteRtEvidenceAfterDeviceIdle(context,idle))
            {fail("survey-GPU-completion-failed");break;}
        const auto publication=context.rtFrameEvidence.PublishedStateByValue();
        const auto& completed=publication.completedEvidence;
        const auto& submitted=completed.identity.submitted;
        if(!publication.presented || !publication.hasCompletedEvidence ||
           submitted.frame.sceneEpoch!=publication.sceneEpoch ||
           submitted.frame.measurementGeneration!=publication.measurementGeneration ||
           submitted.frame.simulationTick!=initial.tickIndex || submitted.submissionSerial<=lastSubmission ||
           completed.presentation.outcome!=horde::telemetry::RtPresentationOutcome::Presented ||
           completed.presentation.lastSuccessfulPresentSubmissionSerial!=submitted.submissionSerial ||
           !completed.scene.dispatch.sceneReady || !completed.scene.dispatch.rtDispatchRecorded ||
           !completed.scene.dispatch.swapchainCopyRecorded || !completed.gpu.hasDuration || !completed.gpu.durationNanoseconds ||
           !horde::telemetry::RtSampleAvailable(completed.gpu.status) ||
           completed.gpu.completedSubmissionSerial!=submitted.submissionSerial)
            {fail("survey-current-completed-GPU-presentation-evidence-mismatch");break;}
        if(frame==0){scene=publication.sceneEpoch;measurement=publication.measurementGeneration;}
        if(scene!=publication.sceneEpoch || measurement!=publication.measurementGeneration ||
           context.rtScene.ChapterTerrainSurface()!=terrain || context.rtScene.DispatchExtent().width!=extent.width ||
           context.rtScene.DispatchExtent().height!=extent.height || context.graphicsSettings!=graphics)
            {fail("survey-resource-settings-or-scope-changed");break;}
        const auto& now=gGameSimulation.Snapshot();
        if(std::memcmp(initialSnapshotBytes.data(),&now,initialSnapshotBytes.size())!=0)
            {snapshotBytesUnchanged=false;fail("survey-mutated-published-snapshot-bytes");break;}
        if(now.tickIndex!=initial.tickIndex || now.playerX!=initial.playerX || now.playerZ!=initial.playerZ ||
           now.lich.health!=initial.lich.health || now.chestReward.phase!=initial.chestReward.phase ||
           now.rescue.firstAscentCompleted!=initial.rescue.firstAscentCompleted)
            {fail("survey-mutated-gameplay-state");break;}
        std::string completedJson;horde::telemetry::RtEvidenceValidationError validation{};
        if(!horde::telemetry::SerializeRtPerformanceEvidenceJson(completed,completedJson,validation))
            {fail("survey-canonical-frame-validation-failed");break;}
        if(frameCount++)frames+=',';
        std::ostringstream row;row.imbue(std::locale::classic());row<<std::setprecision(9)
            <<"{\"index\":"<<frame<<",\"renderTimeSeconds\":"<<context.environmentSurveyTime
            <<",\"renderCallCpuMs\":"<<renderMs<<",\"completedFrame\":"<<completedJson<<'}';frames+=row.str();
        {
            std::lock_guard lock(gEnvironmentSurveyMutex);
            if(!gEnvironmentSurveyPolicy.AcceptFrame(generation,scene,measurement,submitted.submissionSerial))
                fail("survey-request-frame-identity-rejected");
        }
        if(!error.empty())break;
        lastSubmission=submitted.submissionSerial;
        if((frame+1)%10==0) {
            horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture pixels;std::string diagnostic;
            if(imageCount>=kEnvironmentSurveyImages || !context.rtScene.CaptureStorageImage(pixels,diagnostic) ||
               !pixels.width || !pixels.height || pixels.rgba.size()!=std::size_t(pixels.width)*pixels.height*4 ||
               pixels.rgba.size()>64u*1024u*1024u) {fail("survey-native-RGBA-readback-failed");break;}
            const auto file="environment-survey-"+id+"-"+std::to_string(imageCount)+".rgba";
            std::ofstream raw(context.reportDirectory+'/'+file,std::ios::binary|std::ios::trunc);
            raw.write(reinterpret_cast<const char*>(pixels.rgba.data()),static_cast<std::streamsize>(pixels.rgba.size()));raw.flush();
            if(!raw){fail("survey-native-image-write-failed");break;}
            if(imageCount++)images+=',';
            std::ostringstream image;image<<"{\"file\":"<<JsonUtf8String(file)<<",\"width\":"<<pixels.width
                <<",\"height\":"<<pixels.height<<",\"bytes\":"<<pixels.rgba.size()<<",\"frameIndex\":"<<frame<<'}';images+=image.str();
        }
        gSurfaceSessions.Publish(generation,1);
    }
    gSwapchainRunning.store(false,std::memory_order_release);
    const bool retired=DestroySwapchainContext(context);
    gSurfaceRetirementBlocked.store(!retired,std::memory_order_release);
    if(!retired) {fail("survey-GPU-retirement-unproved");gSurfaceSessions.Publish(generation,3);}
    {
        std::lock_guard lock(gEnvironmentSurveyMutex);
        if(gEnvironmentSurveyPolicy.Phase()!=1 || gEnvironmentSurveyPolicy.Serial()!=serial)
            fail("survey-cancelled-before-publication");
    }
    const bool complete=error.empty() && frameCount==kEnvironmentSurveyFrames && imageCount==kEnvironmentSurveyImages && retired;
    const float eyeY=horde::gameplay::simulation::PlayerEyeWorldY(camera.supportY)-initial.playerCrouchDropMetres;
    const auto water=SurveyWaterFacts(camera,eyeY);
    std::ostringstream report;report.imbue(std::locale::classic());report<<std::setprecision(9)
        <<"{\"schema\":1,\"requestId\":"<<JsonUtf8String(id)<<",\"profile\":\"river-bend\",\"diagnosticClass\":\"environment-survey\""
        <<",\"result\":\""<<(complete?"complete":"failed")<<"\",\"waterDisabled\":"<<(disabled?"true":"false")
        <<",\"evidenceClass\":\"Debug render-only; simulation frozen; no earned progress/combat/equipment/frame-pacing acceptance\""
        <<",\"gpu\":"<<JsonUtf8String(gpu)<<",\"driverVersion\":"<<driver<<",\"backend\":"<<JsonUtf8String(horde::vulkan::ToString(backend))
        <<",\"surfaceGeneration\":"<<generation<<",\"simulationTick\":"<<initial.tickIndex<<",\"attemptedFrames\":"<<attempted
        <<",\"simulationSnapshotObjectBytesUnchanged\":"<<(snapshotBytesUnchanged?"true":"false")
        <<",\"nominalAnimationSeconds\":2,\"deadlineSeconds\":"<<deadlineSeconds<<",\"retirementProven\":"<<(retired?"true":"false")
        <<",\"dispatchWidth\":"<<extent.width<<",\"dispatchHeight\":"<<extent.height
        <<",\"surfaceWidth\":"<<surfaceExtent.width<<",\"surfaceHeight\":"<<surfaceExtent.height
        <<",\"settings\":{\"scale\":"<<graphics.renderScalePercent<<",\"water\":"<<unsigned(graphics.waterQuality)
        <<",\"fire\":"<<unsigned(graphics.fireDetail)<<",\"cap\":"<<graphics.previewFrameCap
        <<",\"glass\":"<<(graphics.glassEnabled?"true":"false")<<",\"shadow\":"<<unsigned(graphics.shadowQuality)
        <<",\"mist\":"<<(graphics.mistEnabled?"true":"false")<<",\"dust\":"<<unsigned(graphics.dustQuality)<<'}'
        <<",\"camera\":{\"atlasEast\":56,\"atlasNorth\":-5,\"x\":"<<camera.x<<",\"z\":"<<camera.z<<",\"supportY\":"<<camera.supportY
        <<",\"yaw\":"<<camera.yaw<<",\"pitch\":"<<camera.pitch<<",\"eyeY\":"<<eyeY
        <<",\"crouchDropMetres\":"<<initial.playerCrouchDropMetres
        <<",\"targetAtlasEast\":58,\"targetAtlasNorth\":-25,\"targetAtlasUp\":6"
        <<",\"terrainFound\":"<<(shore.found?"true":"false")<<",\"terrainWalkable\":"<<(shore.walkable?"true":"false")
        <<",\"terrainTriangle\":"<<shore.triangle<<",\"testedTriangles\":"<<shore.testedTriangles
        <<",\"terrainNormal\":["<<shore.normal[0]<<','<<shore.normal[1]<<','<<shore.normal[2]<<']'
        <<",\"footToWaterFaceDistance\":"<<water.footDistance<<",\"eyeToWaterFaceDistance\":"<<water.eyeDistance
        <<",\"waterUnderCamera\":"<<(water.waterAboveOrBelowCamera?"true":"false");
    if(water.waterAboveOrBelowCamera)report<<",\"waterY\":"<<water.waterY<<",\"eyeVerticalWaterClearance\":"<<eyeY-water.waterY;
    report<<'}'
        <<",\"error\":"<<(error.empty()?"null":JsonUtf8String(error))<<",\"images\":["<<images<<"],\"frames\":["<<frames<<"]}\n";
    const bool written=WriteTextFile(path+".json",report.str());
    {
        std::lock_guard lock(gEnvironmentSurveyMutex);
        if(retired)gEnvironmentSurveyActiveSurface.store(0,std::memory_order_release);
        if(complete && written){if(!gEnvironmentSurveyPolicy.Complete())gEnvironmentSurveyPolicy.Cancel();}
        else gEnvironmentSurveyPolicy.Cancel();
    }
    __android_log_print(complete && written?ANDROID_LOG_INFO:ANDROID_LOG_ERROR,kTag,
        "HORDE_ENVIRONMENT_SURVEY id=%s frames=%u images=%u retired=%d complete=%d error=%s",id.c_str(),frameCount,imageCount,
        retired?1:0,complete && written?1:0,error.c_str());
}
