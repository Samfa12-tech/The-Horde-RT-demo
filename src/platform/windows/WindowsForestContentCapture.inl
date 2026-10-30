// One named candidate preflight, reusing ordinary lease scheduling, renderer,
// readback and recreation. No progression awards, explicit GPU cutover or saves.
bool RunForestContentPreflight(VulkanSurfaceContext& context,
    const std::filesystem::path& output,std::string& error)
{
    using Scene=horde::vulkan::raytracing::PresentableTinyRtScene;
    const auto directory=output/"forest-content-preflight";
    std::error_code ec;
    if(!std::filesystem::create_directory(directory,ec)||ec) {
        error="Forest preflight output must be fresh and writable.";return false;
    }
    struct Restore {
        VulkanSurfaceContext& ctx;bool paused,active;
        ~Restore(){ctx.simulationPaused=paused;ctx.chapterRoundTripCaptureActive=active;
            (void)ctx.rtFrameEvidence.SetPaused(paused);}
    } restore{context,context.simulationPaused,context.chapterRoundTripCaptureActive};
    context.simulationPaused=false;
    context.chapterRoundTripCaptureActive=true; // This fixture owns every 60Hz tick.
    (void)context.rtFrameEvidence.SetPaused(false);
    const auto start=std::chrono::steady_clock::now();
    const VkClearColorValue clear{{0,0,0,1}};
    unsigned frames=0,unpresented=0;
    std::uint64_t unloads=0,restores=0,savedBytes=0;
    std::uint64_t epochBefore=0,epochAfter=0;
    float exitX=context.simulation.Snapshot().playerX;
    bool reducedEqual=false,recreatedEqual=false;
    const auto write=[&](bool complete) {
        std::ostringstream json;json<<std::setprecision(9)
            <<"{\"schema\":1,\"scope\":\"staged lower-corridor ordinary lease, movement exit and swapchain recreation; not an earned journey or performance result\""
            <<",\"complete\":"<<(complete?"true":"false")<<",\"candidate\":\""
            <<ParseCaptureLaunchOptions().forestContentCandidate<<"\",\"frames\":"<<frames
            <<",\"unpresentedRestoreFrames\":"<<unpresented<<",\"unloads\":"<<unloads
            <<",\"restores\":"<<restores<<",\"savedOwnerBytesDuringReduction\":"<<savedBytes
            <<",\"exitX\":"<<exitX<<",\"fullReducedPixelsEqual\":"<<(reducedEqual?"true":"false")
            <<",\"recreationPixelsEqual\":"<<(recreatedEqual?"true":"false")
            <<",\"sceneEpochBefore\":"<<epochBefore<<",\"sceneEpochAfter\":"<<epochAfter
            <<",\"error\":"<<(complete?"null":"\""+JsonEscape(error)+"\"")<<"}\n";
        return WriteReportFile(directory/"receipt.json",json.str());
    };
    const auto fail=[&](const std::string& why){error=why;(void)write(false);return false;};
    const auto frame=[&] {
        if(++frames>1024 || std::chrono::steady_clock::now()-start>std::chrono::seconds(90)) {
            error="Forest preflight exceeded its finite frame/time budget.";return false;
        }
        const auto tick=context.simulation.Snapshot().tickIndex;
        const auto waitsBefore=context.rtScene.OrdinaryStaticDetailEvidence().waitFrames;
        bool presented=false;
        if(!RenderFrame(context,clear,presented)) {error="Forest RT frame failed: "+context.lastRtFrameError;return false;}
        if(context.simulation.Snapshot().tickIndex!=tick) {error="Renderer advanced fixture-owned gameplay ticks.";return false;}
        if(!presented) {
            ++unpresented;
            if(context.rtScene.OrdinaryStaticDetailEvidence().waitFrames<=waitsBefore) {
                error="Forest frame lacked presentation outside bounded restoration.";return false;
            }
        } else if(context.lastFramePresentation!=horde::telemetry::RtPresentationOutcome::Presented) {
            error="Forest preflight received an unstable presentation.";return false;
        }
        return true;
    };
    const auto read=[&](const char* name,Scene::StorageImageCapture& image) {
        if(!CompleteRtEvidenceAfterDeviceIdle(context,vkDeviceWaitIdle(context.device))) {
            error="Forest readback cannot prove submission completion.";return false;
        }
        const auto p=context.rtFrameEvidence.PublishedStateByValue();
        if(!p.presented || !p.hasCompletedEvidence ||
           p.completedEvidence.identity.submitted.frame.sceneEpoch!=p.sceneEpoch ||
           p.completedEvidence.identity.submitted.frame.measurementGeneration!=p.measurementGeneration ||
           p.completedEvidence.identity.submitted.frame.simulationTick!=context.simulation.Snapshot().tickIndex ||
           p.completedEvidence.presentation.outcome!=horde::telemetry::RtPresentationOutcome::Presented ||
           p.completedEvidence.identity.submitted.submissionSerial!=p.completedEvidence.presentation.lastSuccessfulPresentSubmissionSerial) {
            error="Forest readback lacks matching current presented evidence.";return false;
        }
        return context.rtScene.CaptureStorageImage(image,error)&&WriteRgbaPng(directory/name,image,error);
    };
    const auto equal=[](const auto& a,const auto& b){return a.width==b.width && a.height==b.height && a.rgba==b.rgba;};
    if(!context.rtScene.OrdinaryStaticDetailAvailable() ||
       !context.rtScene.StaticGeometryHasCompleteContributors() ||
       context.simulation.Snapshot().staticDetailLease.resourceGeneration)
        return fail("Preflight requires an admitted complete owner without an existing lease.");
    Scene::StorageImageCapture full,reduced,beforeRecreate,afterRecreate;
    if(!frame() || !context.rtScene.StaticGeometryHasCompleteContributors() || !read("00-full.png",full))
        return fail("Initial full-owner witness failed: "+error);
    for(unsigned i=0;i<512 && !context.rtScene.OrdinaryStaticDetailEvidence().unloads;++i)
        if(!frame())return fail(error);
    auto detail=context.rtScene.OrdinaryStaticDetailEvidence();
    unloads=detail.unloads;savedBytes=detail.savedOwnerBytes;
    if(unloads!=1 || !savedBytes || detail.omittedTriangles!=horde::platform::windows::kForestContentCandidateCount ||
       context.rtScene.StaticGeometryHasCompleteContributors() ||
       !context.simulation.Snapshot().staticDetailLease.resourceGeneration || !read("01-reduced.png",reduced))
        return fail("Ordinary candidate did not acquire exactly one reduced owner and live lease: "+error);
    reducedEqual=equal(full,reduced);
    if(!reducedEqual)return fail("Full/reduced current RT pixels differ at the same authoritative pose.");
    // Cross the restore threshold through collision-authoritative simulation.
    // Never teleport out of the lease or manufacture completion flags.
    for(unsigned i=0;i<256 && context.simulation.Snapshot().playerX> -28.95f;++i) {
        auto input=context.simulationInput;
        input.paused=false;input.hasAuthoritativePlayerPose=false;
        input.moveForward=1;input.moveStrafe=0;input.guardHeld=false;input.runHeld=false;
        input.yawRadians=-1.57079632679f;
        for(unsigned tick=0;tick<4;++tick)
            context.simulation.StepFixed(input,1.f/60.f,context.simulation.Snapshot().inputPublicationSequence+1);
        context.simulationInput=input;
        if(!frame())return fail(error);
        const auto& state=context.simulation.Snapshot();
        if(state.playerX< -27.5f && (!context.rtScene.StaticGeometryHasCompleteContributors() ||
           state.staticDetailLease.resourceGeneration))return fail("Movement escaped the lease before complete restoration.");
    }
    exitX=context.simulation.Snapshot().playerX;
    detail=context.rtScene.OrdinaryStaticDetailEvidence();restores=detail.restores;
    if(exitX> -28.95f || restores!=unloads || detail.savedOwnerBytes ||
       !context.rtScene.StaticGeometryHasCompleteContributors() ||
       context.simulation.Snapshot().staticDetailLease.resourceGeneration)
        return fail("Ordinary movement exit did not restore the complete owner and release its lease.");
    context.simulationInput.moveForward=context.simulationInput.moveStrafe=0;
    context.simulationPaused=true;
    context.simulation.SynchronizePausedInput(context.simulationInput,context.simulation.Snapshot().inputPublicationSequence);
    MirrorSimulationSnapshot(context);(void)context.rtFrameEvidence.SetPaused(true);
    for(unsigned i=0;i<16;++i)if(!frame())return fail(error);
    if(!read("02-before-recreate.png",beforeRecreate))return fail(error);
    epochBefore=context.rtFrameEvidence.PublishedStateByValue().sceneEpoch;
    if(!RecreateSwapchain(context))
        return fail("Candidate recreation failed: "+context.lastRtFrameError+error);
    for(unsigned i=0;i<64;++i)if(!frame())return fail(error);
    if(!context.rtScene.OrdinaryStaticDetailAvailable() ||
       !context.rtScene.StaticGeometryHasCompleteContributors() || !read("03-after-recreate.png",afterRecreate))
        return fail("Recreated source did not independently regain its full candidate proof: "+error);
    epochAfter=context.rtFrameEvidence.PublishedStateByValue().sceneEpoch;
    if(epochAfter<=epochBefore)return fail("Recreation did not establish a new scene epoch.");
    recreatedEqual=equal(beforeRecreate,afterRecreate);
    if(!recreatedEqual)return fail("Recreation changed current RT pixels at an unchanged paused pose.");
    error.clear();return write(true);
}
