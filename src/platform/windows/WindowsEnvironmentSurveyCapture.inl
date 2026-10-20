// Included only by DiagnosticWindow.cpp's Debug section. This is an explicitly
// render-only environment survey, never a chapter staging/acceptance shortcut.
int RunEnvironmentSurveyCapture(VulkanSurfaceContext& context,
    horde::vulkan::DeviceCapabilities& capabilities,const std::filesystem::path& directory)
{
    using namespace horde::platform::windows;
    constexpr unsigned deadlineSeconds=60;
    struct Row {unsigned index=0;float seconds=0;double renderMs=0,readbackMs=0;
        std::string completed,file,sha256;};
    std::vector<Row> rows;rows.reserve(kEnvironmentSurveyFrames);
    std::string error,executableSha256,terrainSha256,diagnostic;
    unsigned attemptedFrames=0;
    std::array<char,32768> executablePath{};
    const auto started=std::chrono::steady_clock::now();
    // Publish the ordinary output aspect once before freezing the snapshot.
    // Survey frames then consume that same immutable publication.
    SynchronizeSimulationViewportAspect(context);
    const auto initial=context.simulation.Snapshot();
    std::array<unsigned char,sizeof(initial)> initialSnapshotBytes{};
    std::memcpy(initialSnapshotBytes.data(),&context.simulation.Snapshot(),initialSnapshotBytes.size());
    bool snapshotBytesUnchanged=true;
    const bool tomb=context.environmentSurveyProfile=="tomb-debris";
    auto camera=tomb?TombDebrisSurveyCamera(0):RiverBendSurveyCamera();
    horde::gameplay::simulation::TriangleSupportSurface::Sample shore{};
    const auto terrain=context.rtScene.ChapterTerrainSurface();
    const auto width=context.rtScene.DispatchExtent().width,height=context.rtScene.DispatchExtent().height;
    const std::string opaqueKey(context.rtScene.SelectedOpaqueFastKey());
    const std::string opaqueSha(context.rtScene.SelectedOpaqueFastSha256());
    const std::string genericKey(context.rtScene.SelectedGenericDielectricKey());
    const std::string genericSha(context.rtScene.SelectedGenericDielectricSha256());
    const auto write=[&]() {
        std::ostringstream json;json.imbue(std::locale::classic());json<<std::setprecision(9);
        json<<"{\"schema\":1,\"complete\":"<<(error.empty() && rows.size()==kEnvironmentSurveyFrames?"true":"false")
            <<",\"evidenceClass\":\"Debug render-only environment survey; no earned progress, equipment or frame-pacing acceptance\""
            <<",\"profile\":\""<<context.environmentSurveyProfile<<"\",\"hidden\":true,\"source\":\"rt-storage-image\""
            <<",\"animationClock\":\"render-only controls.time, fixed 1/60 s; simulation frozen\""
            <<",\"nominalAnimationSeconds\":2,\"frameBudget\":"<<kEnvironmentSurveyFrames
            <<",\"pngBudget\":12,\"attemptedFrames\":"<<attemptedFrames<<",\"deadlineSeconds\":"<<deadlineSeconds
            <<",\"executableSha256\":\""<<executableSha256<<"\",\"terrainSha256\":\""<<terrainSha256
            <<"\",\"atlasSha256\":\""<<horde::scene::kChapterWaterAtlasSha256
            <<"\",\"regionalWaterRequested\":"<<(context.environmentSurveyWaterDisabled?"false":"true")
            <<",\"floorDebrisRequested\":"<<(context.environmentSurveyDebrisDisabled?"false":"true")
            <<",\"simulationTick\":"<<initial.tickIndex
            <<",\"simulationSnapshotObjectBytesUnchanged\":"<<(snapshotBytesUnchanged?"true":"false")
            <<",\"gpu\":\""<<JsonEscape(capabilities.identity.gpuName)<<"\",\"driverVersion\":"<<capabilities.identity.driverVersion
            <<",\"backend\":\""<<horde::vulkan::ToString(context.rtScene.ExecutionBackend())
            <<"\",\"renderScale\":"<<context.renderScale<<",\"dispatchWidth\":"<<width<<",\"dispatchHeight\":"<<height
            <<",\"surfaceWidth\":"<<context.swapchainExtent.width<<",\"surfaceHeight\":"<<context.swapchainExtent.height
            <<",\"outputExposure\":"<<context.outputExposure<<",\"waterQuality\":"<<unsigned(context.waterQuality)
            <<",\"surveyFogDensityScale\":"<<(tomb?"null":"0.004")
            <<",\"shaderPair\":{\"opaque\":{\"key\":\""<<opaqueKey<<"\",\"sha256\":\""<<opaqueSha
            <<"\"},\"generic\":{\"key\":\""<<genericKey<<"\",\"sha256\":\""<<genericSha<<"\"}}"
            ;
        if(tomb) {
            json<<",\"supportSource\":\"resident dungeon shared floor and legal opening union; no imported terrain query\",\"cameras\":[";
            for(unsigned view=0;view<2;++view) {
                if(view)json<<',';const auto pose=TombDebrisSurveyCamera(view*60);
                json<<"{\"view\":\""<<(view?"debris":"opening")<<"\",\"x\":"<<pose.x<<",\"z\":"<<pose.z
                    <<",\"supportY\":"<<pose.supportY<<",\"yaw\":"<<pose.yaw<<",\"pitch\":"<<pose.pitch<<'}';
            }
            json<<']';
        } else {
            json
            <<",\"camera\":{\"atlasEast\":56,\"atlasNorth\":-5,\"x\":"<<camera.x<<",\"z\":"<<camera.z
            <<",\"supportY\":"<<camera.supportY<<",\"yaw\":"<<camera.yaw<<",\"pitch\":"<<camera.pitch
            <<",\"eyeY\":"<<horde::gameplay::simulation::PlayerEyeWorldY(camera.supportY)-initial.playerCrouchDropMetres
            <<",\"targetAtlasEast\":58,\"targetAtlasNorth\":-25,\"targetAtlasUp\":6"
            <<",\"terrainFound\":"<<(shore.found?"true":"false")<<",\"terrainWalkable\":"<<(shore.walkable?"true":"false")
            <<",\"terrainTriangle\":"<<shore.triangle<<",\"testedTriangles\":"<<shore.testedTriangles
            <<",\"terrainNormal\":["<<shore.normal[0]<<','<<shore.normal[1]<<','<<shore.normal[2]
            <<"]}";
        }
        json<<",\"error\":"<<(error.empty()?"null":"\""+JsonEscape(error)+"\"")<<",\"frames\":[";
        for(std::size_t i=0;i<rows.size();++i) {
            const auto& row=rows[i];if(i)json<<',';
            json<<"{\"index\":"<<row.index<<",\"view\":\""<<(tomb?(row.index<60?"opening":"debris"):"river")<<"\",\"renderTimeSeconds\":"<<row.seconds
                <<",\"renderCallCpuMs\":"<<row.renderMs<<",\"readbackCpuMs\":"<<row.readbackMs
                <<",\"file\":"<<(row.file.empty()?"null":"\""+row.file+"\"")
                <<",\"pngSha256\":"<<(row.sha256.empty()?"null":"\""+row.sha256+"\"")
                <<",\"completedFrame\":"<<row.completed<<'}';
        }
        json<<"]}\n";return WriteReportFile(directory/"environment-survey.json",json.str());
    };
    const auto fail=[&](const std::string& why) {error=why;(void)write();
        std::cerr<<"Environment survey failed: "<<why<<'\n';return 1;};
    if(!gShowcaseCaptureHidden || !context.environmentSurveyActive || IsWindowVisible(context.windowHandle) ||
       GetForegroundWindow()==context.windowHandle || GetFocus()==context.windowHandle)
        return fail("Environment survey requires an unshown, unfocused capture window.");
    if(!context.useRtPath || !context.rtScene.IsReady() || !context.rtFrameEvidenceInitialised || !terrain)
        return fail("A ready native terrain and hardware RT owner are required.");
    if(!GetModuleFileNameA(nullptr,executablePath.data(),static_cast<DWORD>(executablePath.size())) ||
       !Sha256File(executablePath.data(),executableSha256,diagnostic))
        return fail("Executable identity is unavailable: "+diagnostic);
    if(!Sha256File(ResolveAssetRoot()/"models/world/runtime/chapter-terrain-v1/chapter-terrain-lod0.runtime.glb",
            terrainSha256,diagnostic))return fail("Terrain identity is unavailable: "+diagnostic);
    if(tomb) {
        const auto detailCamera=TombDebrisSurveyCamera(60);
        if(!horde::gameplay::IsShowcasePlayerPositionWalkable(camera.x,camera.z) ||
           !horde::gameplay::IsShowcasePlayerPositionWalkable(detailCamera.x,detailCamera.z) ||
           camera.supportY!=horde::gameplay::kRouteFloorWorldY || detailCamera.supportY!=camera.supportY)
            return fail("Tomb survey cameras require legal shared-floor opening positions.");
    } else {
        shore=terrain->Query(camera.x,camera.z);
        if(!shore.found || !std::isfinite(shore.worldY))return fail("Actual imported terrain does not support the planned shore camera.");
        camera.supportY=shore.worldY;
        const auto target=horde::scene::ChapterWaterFromAtlas(58,-25,6);
        camera.pitch=.05f+(target[1]-(horde::gameplay::simulation::PlayerEyeWorldY(camera.supportY)-initial.playerCrouchDropMetres))/
            std::hypot(target[0]-camera.x,target[2]-camera.z);
        if(!std::isfinite(camera.pitch) || camera.pitch<-.32f || camera.pitch>.28f)
            return fail("Native queried survey aim exceeds existing production pitch limits.");
    }
    context.environmentSurveyCamera=camera;
    // Do not call StageDevelopmentCheckpointSimulation, AdvanceFrame, BuildInput,
    // campaign commands or a modelled-equipment exception. Only render inputs
    // below change; the normal full-capture equipment gates remain untouched.
    context.frameDeltaSeconds=0;context.simulationPaused=true;
    std::uint64_t lastSubmission=0,sceneEpoch=0,measurementGeneration=0;
    const auto clear=ClearColorForMode(capabilities.rtMode);
    for(unsigned frame=0;frame<kEnvironmentSurveyFrames;++frame)
    {
        attemptedFrames=frame+1;
        if(std::chrono::steady_clock::now()-started>std::chrono::seconds(deadlineSeconds))
            return fail("Environment survey exceeded its bounded deadline.");
        if(!IsWindow(context.windowHandle) || IsWindowVisible(context.windowHandle) ||
           GetForegroundWindow()==context.windowHandle || GetFocus()==context.windowHandle)
            return fail("Hidden window/input ownership changed.");
        context.environmentSurveyTime=EnvironmentSurveyTime(frame);
        if(tomb)context.environmentSurveyCamera=TombDebrisSurveyCamera(frame);
        const auto begin=std::chrono::steady_clock::now();bool presented=false;
        if(!RenderFrame(context,clear,presented) || !presented ||
           context.lastFramePresentation!=horde::telemetry::RtPresentationOutcome::Presented)
            return fail("A survey frame did not reach an actual successful RT presentation: "+context.lastRtFrameError);
        const auto renderEnd=std::chrono::steady_clock::now();
        const VkResult idle=vkDeviceWaitIdle(context.device);
        if(idle!=VK_SUCCESS || !CompleteRtEvidenceAfterDeviceIdle(context,idle))
            return fail("Survey submitted graphics ownership did not complete.");
        const auto publication=context.rtFrameEvidence.PublishedStateByValue();
        const auto& completed=publication.completedEvidence;
        const auto& submitted=completed.identity.submitted;
        if(!publication.presented || !publication.hasCompletedEvidence ||
           submitted.submissionSerial<=lastSubmission ||
           submitted.frame.sceneEpoch!=publication.sceneEpoch ||
           submitted.frame.measurementGeneration!=publication.measurementGeneration ||
           submitted.frame.simulationTick!=initial.tickIndex ||
           completed.presentation.outcome!=horde::telemetry::RtPresentationOutcome::Presented ||
           completed.presentation.lastSuccessfulPresentSubmissionSerial!=submitted.submissionSerial ||
           !completed.scene.dispatch.sceneReady || !completed.scene.dispatch.rtDispatchRecorded ||
           !completed.scene.dispatch.swapchainCopyRecorded ||
           !horde::telemetry::RtSampleAvailable(completed.gpu.status) || !completed.gpu.hasDuration || !completed.gpu.durationNanoseconds ||
           completed.gpu.completedSubmissionSerial!=submitted.submissionSerial)
            return fail("Survey lacks matching completed RT presentation and current per-frame GPU evidence.");
        if(frame==0){sceneEpoch=publication.sceneEpoch;measurementGeneration=publication.measurementGeneration;}
        if(publication.sceneEpoch!=sceneEpoch || publication.measurementGeneration!=measurementGeneration ||
           context.rtScene.ChapterTerrainSurface()!=terrain ||
           context.rtScene.DispatchExtent().width!=width || context.rtScene.DispatchExtent().height!=height ||
           std::string(context.rtScene.SelectedOpaqueFastSha256())!=opaqueSha ||
           std::string(context.rtScene.SelectedGenericDielectricSha256())!=genericSha)
            return fail("Survey resource/measurement/shader identity changed during the finite sample.");
        const auto& current=context.simulation.Snapshot();
        if(std::memcmp(initialSnapshotBytes.data(),&current,initialSnapshotBytes.size())!=0) {
            snapshotBytesUnchanged=false;
            return fail("Render-only survey changed the published simulation snapshot bytes.");
        }
        if(current.tickIndex!=initial.tickIndex || current.playerX!=initial.playerX || current.playerZ!=initial.playerZ ||
           current.lich.health!=initial.lich.health || current.rescue.firstAscentCompleted!=initial.rescue.firstAscentCompleted ||
           current.chestReward.phase!=initial.chestReward.phase)
            return fail("Render-only survey changed simulated gameplay state.");
        Row row;row.index=frame;row.seconds=context.environmentSurveyTime;
        row.renderMs=std::chrono::duration<double,std::milli>(renderEnd-begin).count();
        horde::telemetry::RtEvidenceValidationError evidenceError{};
        if(!horde::telemetry::SerializeRtPerformanceEvidenceJson(completed,row.completed,evidenceError))
            return fail("Survey completed-frame evidence failed canonical validation.");
        if((frame+1)%kEnvironmentSurveyCaptureStride==0)
        {
            const auto readbackStart=std::chrono::steady_clock::now();
            horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
            if(!context.rtScene.CaptureStorageImage(image,diagnostic))return fail("Actual survey readback failed: "+diagnostic);
            std::ostringstream name;name<<(tomb?"tomb-debris-":"river-bend-")<<std::setw(3)<<std::setfill('0')<<frame<<".png";row.file=name.str();
            if(!WriteRgbaPng(directory/row.file,image,diagnostic) || !Sha256File(directory/row.file,row.sha256,diagnostic))
                return fail("Survey PNG/identity failed: "+diagnostic);
            row.readbackMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-readbackStart).count();
        }
        rows.push_back(std::move(row));lastSubmission=submitted.submissionSerial;
    }
    if(!write())return fail("Survey manifest could not be written.");
    return 0;
}
