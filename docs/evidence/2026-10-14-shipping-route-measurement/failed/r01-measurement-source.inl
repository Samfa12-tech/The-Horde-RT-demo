// Development measurement only. Production timing, inputs and shaders are not
// modified. Unlike the faster deterministic capture, the route follows elapsed
// wall time, retaining complete60Hz ticks behind a four-tick catch-up bound.

bool RunChapterShippingRouteMeasurement(VulkanSurfaceContext& context,
    const std::filesystem::path& output,std::string& error)
{
    using namespace horde::gameplay::validation;
    using namespace horde::telemetry;
    const auto initial=context.rtFrameEvidence.PublishedStateByValue();
    if(!initial.hasCompletedEvidence ||
       initial.completedEvidence.scene.pipeline.instrumentation!=RtInstrumentationMode::Shipping ||
       initial.completedEvidence.scene.pipeline.dielectricQuality!=RtDielectricQuality::High ||
       initial.completedEvidence.dielectric.compiled || initial.completedEvidence.dielectric.hasCounters ||
       context.inFlightFences.size()!=1) {
        error="Route measurement requires the actual Shipping/High artifact pair and no Diagnostic IO.";return false;
    }
    ChapterRoundTripProbe probe;
    if(!probe.Start(context.simulation.Snapshot())) {
        error="Shipping route requires the admitted low-lantern reunion.";return false;
    }
    const auto directory=output/"shipping-route";
    std::error_code pathError;
    if(!std::filesystem::create_directory(directory,pathError)||pathError) {
        error="Shipping route evidence directory must be new and writable.";return false;
    }
    std::ofstream csv(directory/"completed-frames.csv"),snapshots(directory/"identities.jsonl");
    if(!csv||!snapshots) {error="Shipping route evidence files could not open.";return false;}
    csv<<"elapsed_s,phase,station,simulation_step_cpu_ns,scene_epoch,generation,submission_serial,tick,gpu_status,gpu_ns,cpu_eligible,"
        <<"device_local_bytes,host_visible_bytes,strategy,shader_sha256";
    for(std::size_t i=0;i<kRtStageCount;++i) {
        const auto name=RtStageMetricName(static_cast<RtStage>(i));
        csv<<','<<name<<"_ns,"<<name<<"_invocations,"<<name<<"_bytes,"<<name<<"_operations,"<<name<<"_overflow";
    }
    csv<<'\n';
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
        std::uint64_t serial=0,tick=0,simulationNanoseconds=0;double elapsedSeconds=0;
    };
    // The sole in-flight graphics owner completes before reusing its slot.
    // Tags join that committed identity, never just a repeated simulation tick.
    // A missing/overwritten join fails closed instead of borrowing a later pose.
    std::array<Tag,16> tags{};
    const auto baseTick=context.simulation.Snapshot().tickIndex;
    const auto start=std::chrono::steady_clock::now();
    const auto elapsed=[&] {return std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();};
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
        csv<<tag.elapsedSeconds<<','<<ChapterProbePhaseName(tag.phase)<<','<<tag.station<<','
            <<tag.simulationNanoseconds<<','<<id.frame.sceneEpoch<<','
            <<id.frame.measurementGeneration<<','<<id.submissionSerial<<','<<id.frame.simulationTick<<','
            <<RtSampleStatusName(sample.gpu.status)<<','<<(sample.gpu.hasDuration?sample.gpu.durationNanoseconds:0)<<','
            <<sample.cpuBenchmarkEligible<<','<<sample.scene.resources.deviceLocalBytes<<','
            <<sample.scene.resources.hostVisibleBytes<<','<<RtMaterialStrategyName(sample.scene.pipeline.activeStrategy)<<','
            <<RtFixedTextView(sample.scene.pipeline.active.sha256);
        for(const auto& stage:sample.scene.stages.values)
            csv<<','<<stage.durationNanoseconds<<','<<stage.workInvocationCount<<','<<stage.byteCount<<','
                <<stage.operationCount<<','<<stage.overflowed;
        csv<<'\n';
        if(samples%120==0 || tag.phase!=lastPhase) {
            std::string json;
            if(!SerializeRtPerformanceEvidenceJson(sample,json,validation)) {
                error="Shipping route identity serialization failed.";return false;
            }
            snapshots<<"{\"phase\":\""<<ChapterProbePhaseName(tag.phase)<<"\",\"station\":"<<tag.station
                <<",\"elapsedSeconds\":"<<tag.elapsedSeconds<<",\"evidence\":"<<json<<"}\n";
            csv.flush();snapshots.flush();
        }
        lastSerial=id.submissionSerial;lastPhase=tag.phase;++samples;
        if(!csv||!snapshots) {error="Shipping route evidence write failed.";return false;}
        return true;
    };
    const VkClearColorValue clear{{0,0,0,1}};
    while(frames++<30000 && probe.Phase()!=ChapterProbePhase::Complete) {
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
        tags[submitted.submissionSerial%tags.size()]={probe.Phase(),static_cast<unsigned>(probe.Station()),
            submitted.submissionSerial,tick,simulationNs,elapsed()};
        if(!append())return false;
    }
    const auto finishSeconds=elapsed();
    const auto idle=vkDeviceWaitIdle(context.device);
    if(!CompleteRtEvidenceAfterDeviceIdle(context,idle)||!append()) {
        if(error.empty())error="Shipping route final GPU completion is unproved.";return false;
    }
    const auto& state=context.simulation.Snapshot();
    if(probe.Phase()!=ChapterProbePhase::Complete || finishSeconds<60 || !state.rescue.exteriorSide ||
       state.rescue.equipmentStowed || !state.rescue.sawDescent || !state.rescue.sawLanding ||
       state.rescue.claimCount!=1 || state.rescue.deploymentCount!=1 || !state.chapterDialogue.proofAccepted) {
        error="Shipping route lacks sustained duration or complete two-way progression.";return false;
    }
    horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
    if(!context.rtScene.CaptureStorageImage(image,error)||!WriteRgbaPng(directory/"final-repeat-ascent.png",image,error))return false;
    std::ostringstream summary;
    summary<<"{\"schema\":1,\"scope\":\"Debug CPU with actual Shipping High shaders; wall-clock two-way route after staged Keeper death; no owner or phone acceptance\","
        <<"\"durationSeconds\":"<<finishSeconds<<",\"renderedFrames\":"<<frames-1<<",\"completedSamples\":"<<samples
        <<",\"simulationTicks\":"<<state.tickIndex-baseTick<<",\"maximumPendingSimulationTicks\":"<<maximumDebt
        <<",\"simulationHz\":60,\"maximumTicksPerRender\":4,\"deadlineSeconds\":150,\"warmupSeconds\":5,"
        <<"\"normalStaticRegionResidencySavingBytes\":0,\"complete\":true}\n";
    csv.flush();snapshots.flush();
    if(!WriteReportFile(directory/"run.json",summary.str())) {error="Shipping route summary write failed.";return false;}
    error.clear();return true;
}
