// Debug-only immutable shared-simulation fixture inspection. The application
// simulation, input, campaign, preferences and audio are never advanced here.
int RunPlayerDodgeCapture(VulkanSurfaceContext& context,
    horde::vulkan::DeviceCapabilities& capabilities,const std::filesystem::path& directory)
{
    std::vector<std::string> rows;std::string error,diagnostic,executableSha;
    const auto started=std::chrono::steady_clock::now();
    std::array<unsigned char,sizeof(context.simulation.Snapshot())> initial{};
    std::memcpy(initial.data(),&context.simulation.Snapshot(),initial.size());
    const auto frames=horde::gameplay::validation::BuildPlayerDodgePoseFixture();
    struct ResetInspection {VulkanSurfaceContext& context;
        ~ResetInspection(){context.playerDodgeInspectionSnapshot.reset();}} reset{context};
    const auto write=[&]() {
        std::ostringstream json;json.imbue(std::locale::classic());json<<std::setprecision(9);
        json<<"{\"schema\":1,\"profile\":\"player-dodge\",\"complete\":"
            <<(error.empty() && rows.size()==frames.size()?"true":"false")
            <<",\"evidenceClass\":\"seeded noncombat immutable shared-simulation pose snapshots; first-person RT and exact CPU-upload mesh, not live input, earned progression, contact or sustained performance\""
            <<",\"hidden\":true,\"dodgePoseRequested\":"<<(context.playerDodgeInspectionDisabled?"false":"true")
            <<",\"executableSha256\":\""<<executableSha<<"\",\"gpu\":\""<<JsonEscape(capabilities.identity.gpuName)
            <<"\",\"driverVersion\":"<<capabilities.identity.driverVersion
            <<",\"backend\":\""<<horde::vulkan::ToString(context.rtScene.ExecutionBackend())
            <<"\",\"renderScale\":"<<context.renderScale<<",\"dispatchWidth\":"<<context.rtScene.DispatchExtent().width
            <<",\"dispatchHeight\":"<<context.rtScene.DispatchExtent().height
            <<",\"snapshotFrameBudget\":64,\"deadlineSeconds\":90,\"applicationSnapshotObjectBytesUnchanged\":"
            <<(std::memcmp(initial.data(),&context.simulation.Snapshot(),initial.size())==0?"true":"false")
            <<",\"error\":"<<(error.empty()?"null":"\""+JsonEscape(error)+"\"")<<",\"frames\":[";
        for(std::size_t i=0;i<rows.size();++i){if(i)json<<',';json<<rows[i];}
        json<<"]}\n";return WriteReportFile(directory/"player-dodge-survey.json",json.str());
    };
    const auto fail=[&](const std::string& why){error=why;(void)write();
        std::cerr<<"Player dodge inspection failed: "<<why<<'\n';return 1;};
    if(!gShowcaseCaptureHidden || !context.environmentSurveyActive ||
       context.environmentSurveyProfile!="player-dodge" || IsWindowVisible(context.windowHandle) ||
       GetForegroundWindow()==context.windowHandle || GetFocus()==context.windowHandle ||
       !context.useRtPath || !context.rtScene.IsReady() || frames.size()!=64)
        return fail("A bounded unshown native RT player-dodge owner is required.");
    std::array<char,32768> executable{};
    if(!GetModuleFileNameA(nullptr,executable.data(),static_cast<DWORD>(executable.size())) ||
       !Sha256File(executable.data(),executableSha,diagnostic))return fail("Executable identity unavailable: "+diagnostic);
    context.frameDeltaSeconds=0;context.simulationPaused=true;
    std::uint64_t lastSerial=0;const auto clear=ClearColorForMode(capabilities.rtMode);
    for(const auto& fixture:frames) {
        if(std::chrono::steady_clock::now()-started>std::chrono::seconds(90))return fail("Finite inspection deadline exceeded.");
        if(IsWindowVisible(context.windowHandle) || GetForegroundWindow()==context.windowHandle ||
           GetFocus()==context.windowHandle)return fail("Hidden window/input ownership changed.");
        context.playerDodgeInspectionSnapshot=fixture.snapshot;
        if(context.playerDodgeInspectionDisabled)context.playerDodgeInspectionSnapshot->playerAnimation.dodge={};
        bool presented=false;
        if(!RenderFrame(context,clear,presented) || !presented)return fail("Native RT frame did not present: "+context.lastRtFrameError);
        const auto idle=vkDeviceWaitIdle(context.device);
        if(idle!=VK_SUCCESS || !CompleteRtEvidenceAfterDeviceIdle(context,idle))return fail("Owning graphics completion failed.");
        const auto publication=context.rtFrameEvidence.PublishedStateByValue();
        const auto& completed=publication.completedEvidence;
        const auto serial=completed.identity.submitted.submissionSerial;
        if(!publication.presented || !publication.hasCompletedEvidence || serial<=lastSerial ||
           completed.identity.submitted.frame.simulationTick!=fixture.snapshot.tickIndex ||
           completed.presentation.outcome!=horde::telemetry::RtPresentationOutcome::Presented ||
           completed.presentation.lastSuccessfulPresentSubmissionSerial!=serial ||
           !completed.scene.dispatch.sceneReady || !completed.scene.dispatch.rtDispatchRecorded ||
           !completed.scene.dispatch.swapchainCopyRecorded || !horde::telemetry::RtSampleAvailable(completed.gpu.status) ||
           !completed.gpu.hasDuration || completed.gpu.completedSubmissionSerial!=serial)
            return fail("Current immutable pose lacks matching completed RT evidence.");
        if(std::memcmp(initial.data(),&context.simulation.Snapshot(),initial.size())!=0)
            return fail("Inspection changed the application's published simulation bytes.");
        horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
        const auto png=fixture.name+".png";std::string pngSha,meshSha;
        if(!context.rtScene.CaptureStorageImage(image,diagnostic) || !WriteRgbaPng(directory/png,image,diagnostic) ||
           !Sha256File(directory/png,pngSha,diagnostic))return fail("Native RT readback failed: "+diagnostic);
        const auto mesh=fixture.name+".obj";
        if(fixture.mesh && (!context.rtScene.CapturePlayerWorldBodyMesh((directory/mesh).string(),diagnostic) ||
           !Sha256File(directory/mesh,meshSha,diagnostic)))return fail("Exact CPU-upload mesh capture failed: "+diagnostic);
        std::string evidence;horde::telemetry::RtEvidenceValidationError evidenceError{};
        if(!horde::telemetry::SerializeRtPerformanceEvidenceJson(completed,evidence,evidenceError))
            return fail("Canonical completed-frame serialization failed.");
        std::ostringstream row;row.imbue(std::locale::classic());row<<std::setprecision(9);
        const auto& s=fixture.snapshot;
        row<<"{\"name\":\""<<fixture.name<<"\",\"simulationTick\":"<<s.tickIndex
            <<",\"position\":["<<s.playerX<<','<<s.playerSupportWorldY<<','<<s.playerZ<<"]"
            <<",\"yaw\":"<<s.playerYawRadians<<",\"paused\":"<<(s.paused?"true":"false")
            <<",\"heldLightKind\":"<<unsigned(s.interaction.heldLightKind)
            <<",\"dodgeActive\":"<<(s.dodgeActive?"true":"false")<<",\"dodgeElapsedSeconds\":"<<s.dodgeElapsedSeconds
            <<",\"publishedPose\":["<<s.playerAnimation.dodge.weight<<','<<s.playerAnimation.dodge.modelX<<','<<s.playerAnimation.dodge.modelZ<<']'
            <<",\"png\":\""<<png<<"\",\"pngSha256\":\""<<pngSha<<"\",\"cpuUploadMesh\":"
            <<(fixture.mesh?"\""+mesh+"\"":"null")<<",\"meshSha256\":"<<(fixture.mesh?"\""+meshSha+"\"":"null")
            <<",\"completedFrame\":"<<evidence<<'}';
        rows.push_back(row.str());lastSerial=serial;
    }
    if(!write())return fail("Inspection manifest write failed.");return 0;
}
