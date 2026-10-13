// Debug-only scoped RT capture; no foreground input or saved-setting writes.
bool RunChapterRoundTripCapture(VulkanSurfaceContext& context,const std::filesystem::path& output,
    std::string& error)
{
    using namespace horde::gameplay::validation;
    ChapterRoundTripProbe probe;
    if(!probe.Start(context.simulation.Snapshot())) {
        error="Two-way capture requires the admitted, low-lantern reunion.";return false;
    }
    const auto directory=output/"roundtrip";
    std::error_code pathError;
    if(!std::filesystem::create_directory(directory,pathError)||pathError) {
        error="Two-way output directory must be new and writable.";return false;
    }
    std::ofstream rows(directory/"frames.csv"),manifest(directory/"gallery.json");
    if(!rows||!manifest) {error="Two-way evidence files could not open.";return false;}
    rows<<"frame,phase,station,submitted_tick,simulation_ms,render_cycle_ms,completed_tick,completed_serial,gpu_status,gpu_ns,device_local_bytes,host_visible_bytes,keeper_retired_bytes,keeper_retired_memory_flags\n";
    manifest<<"{\"schema\":1,\"scope\":\"moving fixed-step validation after staged Keeper death and first ascent; Diagnostic, static scene resident, normal Keeper vertex lifetime active; not owner or Shipping performance acceptance\",\"maximumSimulationTicksPerRender\":4,\"simulationHz\":60,\"captures\":[\n";
    struct Restore {
        VulkanSurfaceContext& ctx;bool paused,active;int sfx,voice,music;
        ~Restore() {ctx.simulationPaused=paused;ctx.sfxVolumePercent=sfx;ctx.voiceVolumePercent=voice;ctx.musicVolumePercent=music;
            ctx.chapterRoundTripCaptureActive=active;
            (void)ctx.rtFrameEvidence.SetPaused(paused);}
    } restore{context,context.simulationPaused,context.chapterRoundTripCaptureActive,context.sfxVolumePercent,context.voiceVolumePercent,context.musicVolumePercent};
    context.simulationPaused=false;
    context.chapterRoundTripCaptureActive=true;
    context.sfxVolumePercent=context.voiceVolumePercent=context.musicVolumePercent=0;
    (void)context.rtFrameEvidence.SetPaused(false);
    const VkClearColorValue clear{{0,0,0,1}};
    unsigned captures=0,frames=0;bool hadCapture=false,kitWalkCaptured=false;
    const auto capture=[&](const std::string& label) {
        const auto idle=vkDeviceWaitIdle(context.device);
        if(!CompleteRtEvidenceAfterDeviceIdle(context,idle)) {error="Two-way capture cannot prove submitted work complete.";return false;}
        const auto publication=context.rtFrameEvidence.PublishedStateByValue();
        const auto& complete=publication.completedEvidence;
        const auto& state=context.simulation.Snapshot();
        if(!publication.hasCompletedEvidence || !publication.presented ||
           complete.identity.submitted.frame.sceneEpoch!=publication.sceneEpoch ||
           complete.identity.submitted.frame.measurementGeneration!=publication.measurementGeneration ||
           complete.identity.submitted.frame.simulationTick!=state.tickIndex ||
           complete.presentation.outcome!=horde::telemetry::RtPresentationOutcome::Presented ||
           complete.identity.submitted.submissionSerial!=complete.presentation.lastSuccessfulPresentSubmissionSerial) {
            error="Two-way readback lacks matching successful RT presentation.";return false;
        }
        std::string evidence;horde::telemetry::RtEvidenceValidationError validation{};
        if(!horde::telemetry::SerializeRtPerformanceEvidenceJson(complete,evidence,validation)) {
            error="Two-way completed evidence failed canonical validation.";return false;
        }
        const std::string filename=std::to_string(++captures)+"-"+label+".png";
        horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
        if(!context.rtScene.CaptureStorageImage(image,error)||!WriteRgbaPng(directory/filename,image,error))return false;
        manifest<<(hadCapture?",\n":"")<<"{\"file\":\""<<filename<<"\",\"frame\":"<<frames
            <<",\"phase\":\""<<ChapterProbePhaseName(probe.Phase())<<"\",\"station\":"<<probe.Station()
            <<",\"camera\":["<<state.playerX<<','<<state.playerSupportWorldY<<','<<state.playerZ<<','
            <<state.playerYawRadians<<','<<state.playerPitchRadians<<"],\"ropePhase\":"<<static_cast<unsigned>(state.rescue.phase)
            <<",\"claimCount\":"<<state.rescue.claimCount<<",\"deploymentCount\":"<<state.rescue.deploymentCount
            <<",\"equipmentStowed\":"<<(state.rescue.equipmentStowed?"true":"false")
            <<",\"companion\":{\"grounded\":"<<(state.chapterCompanion.grounded?"true":"false")
            <<",\"walking\":"<<(state.chapterCompanion.walking?"true":"false")
            <<",\"wait\":"<<static_cast<unsigned>(state.chapterCompanion.wait)
            <<",\"position\":["<<state.chapterCompanion.x<<','<<state.chapterCompanion.y<<','<<state.chapterCompanion.z<<']'
            <<",\"yaw\":"<<state.chapterCompanion.yaw<<",\"animationSeconds\":"<<state.chapterCompanion.animationSeconds
            <<",\"generation\":"<<state.chapterCompanion.generation<<'}'
            <<",\"completedFrame\":"<<evidence<<'}';
        hadCapture=true;manifest.flush();rows.flush();return bool(manifest)&&bool(rows);
    };
    if(!capture("initial-reunion"))return false;
    const auto start=std::chrono::steady_clock::now();
    while(frames++<10000 && probe.Phase()!=ChapterProbePhase::Complete) {
        if(std::chrono::steady_clock::now()-start>std::chrono::seconds(150)) {
            error="Two-way capture exceeded its bounded 150-second run.";return false;
        }
        const auto phase=probe.Phase();const auto station=probe.Station();
        const auto quarter=probe.SweepQuarter();
        const auto beforeTick=context.simulation.Snapshot().tickIndex;
        const auto before=std::chrono::steady_clock::now();
        unsigned steps=0;
        // A slow render can consume multiple authoritative60Hz ticks, exactly
        // as ordinary fixed-step play does. Keep this finite scripted cadence
        // explicit: it is not a lower shader quality or resolution setting.
        for(;steps<4 && probe.Phase()!=ChapterProbePhase::Complete;++steps) {
            const auto input=probe.Next(context.simulation.Snapshot());
            if(probe.Phase()==ChapterProbePhase::Failed) {
                error="Two-way probe rejected its lifecycle, ownership or progress.";return false;
            }
            context.simulationInput=input;
            context.simulation.StepFixed(input,1.f/60.f,context.simulation.Snapshot().inputPublicationSequence+1);
        }
        const auto simulated=std::chrono::steady_clock::now();
        if(context.simulation.Snapshot().tickIndex!=beforeTick+steps || steps==0) {
            error="Two-way probe did not produce its exact bounded authoritative ticks.";return false;
        }
        bool presented=false;
        if(!RenderFrame(context,clear,presented)||!presented ||
           context.lastFramePresentation!=horde::telemetry::RtPresentationOutcome::Presented) {
            error="Two-way moving frame failed stable RT presentation: "+context.lastRtFrameError;return false;
        }
        const auto rendered=std::chrono::steady_clock::now();
        if(context.simulation.Snapshot().tickIndex!=beforeTick+steps) {
            error="Platform rendering advanced the probe's already-owned simulation tick.";return false;
        }
        const auto completed=context.rtFrameEvidence.PublishedStateByValue();
        const auto inventory=context.rtScene.ResourceInventory();
        const auto keeper=context.rtScene.KeeperVertexLifetime();
        rows<<frames<<','<<ChapterProbePhaseName(phase)<<','<<station<<','<<context.simulation.Snapshot().tickIndex<<','
            <<std::chrono::duration<double,std::milli>(simulated-before).count()<<','
            <<std::chrono::duration<double,std::milli>(rendered-simulated).count()<<','
            <<(completed.hasCompletedEvidence?completed.completedEvidence.identity.submitted.frame.simulationTick:0)<<','
            <<(completed.hasCompletedEvidence?completed.completedEvidence.identity.submitted.submissionSerial:0)<<','
            <<static_cast<unsigned>(completed.completedEvidence.gpu.status)<<','
            <<(completed.hasCompletedEvidence&&completed.completedEvidence.gpu.hasDuration?completed.completedEvidence.gpu.durationNanoseconds:0)<<','
            <<inventory.deviceLocalBytes<<','<<inventory.hostVisibleBytes<<','
            <<keeper.retiredAllocationBytes<<','<<keeper.retiredMemoryFlags<<'\n';
        if(frames%120==0)rows.flush();
        const bool transition=phase!=probe.Phase() || station!=probe.Station();
        const bool quadrant=(phase==ChapterProbePhase::Landing || phase==ChapterProbePhase::Lookout ||
            phase==ChapterProbePhase::Lower) && quarter!=probe.SweepQuarter();
        const auto& actor=context.simulation.Snapshot().chapterCompanion;
        if(!kitWalkCaptured && phase==ChapterProbePhase::Raise && actor.walking && actor.animationSeconds>=.3f) {
            if(!capture("kit-walking"))return false;
            kitWalkCaptured=true;
        }
        if((transition||quadrant) && !capture(std::string(ChapterProbePhaseName(phase))+"-"+std::to_string(station)))return false;
    }
    if(probe.Phase()!=ChapterProbePhase::Complete) {error="Two-way capture exhausted its finite frame roster.";return false;}
    const auto& last=context.simulation.Snapshot();
    if(!last.rescue.exteriorSide||last.rescue.equipmentStowed||!last.rescue.sawDescent||!last.rescue.sawLanding||
       last.rescue.claimCount!=1||last.rescue.deploymentCount!=1||!last.chapterDialogue.proofAccepted||!kitWalkCaptured) {
        error="Two-way capture lacks final progression/equipment witnesses.";return false;
    }
    const auto retired=context.rtScene.KeeperVertexLifetime();
    if(!retired.retired || retired.retirementCount!=1 || retired.restorationCount!=0) {
        error="Two-way traversal lacks exactly one completed Keeper vertex retirement.";return false;
    }
    // Exercise ordinary route-reset input after the repeated ascent. The same
    // device/AS/descriptor owners must safely restore Keeper storage.
    auto reset=context.simulationInput;reset.moveForward=reset.moveStrafe=0;
    ++reset.commands.routeReset;reset.paused=false;
    context.simulationInput=reset;
    context.simulation.StepFixed(reset,1.f/60.f,context.simulation.Snapshot().inputPublicationSequence+1);
    bool resetPresented=false;
    if(!RenderFrame(context,clear,resetPresented)||!resetPresented||
       !capture("reset-opening")) {
        error="Chapter reset failed current-pose RT restoration: "+context.lastRtFrameError;return false;
    }
    const auto restored=context.rtScene.KeeperVertexLifetime();
    const auto& resetState=context.simulation.Snapshot();
    if(restored.retired || restored.retirementCount!=1 || restored.restorationCount!=1 ||
       resetState.lich.phase==horde::gameplay::LichPhase::Dead ||
       resetState.chapterCompanion.grounded || resetState.rescue.claimCount!=0) {
        error="Ordinary reset left stale companion or retired Keeper storage.";return false;
    }
    manifest<<"\n],\"frames\":"<<frames-1<<",\"normalPlayStaticRegionResidencySavingBytes\":0,"
        <<"\"keeperVertexLifetime\":{\"retiredAllocationBytes\":"<<retired.retiredAllocationBytes
        <<",\"retiredMemoryFlags\":"<<retired.retiredMemoryFlags
        <<",\"retirements\":"<<restored.retirementCount<<",\"restorations\":"<<restored.restorationCount
        <<",\"resetRestored\":true},\"complete\":true}\n";
    rows.flush();manifest.flush();
    if(!rows||!manifest) {error="Two-way evidence write failed.";return false;}
    error.clear();return true;
}
