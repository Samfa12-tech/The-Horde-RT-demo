// Debug-only scoped RT capture; no foreground input or saved-setting writes.
bool RunChapterRoundTripCapture(VulkanSurfaceContext& context,const std::filesystem::path& output,
    std::string& error,bool landingOnly=false)
{
    using namespace horde::gameplay::validation;
    const bool sharedForest=context.rtScene.SharedForestGeometryActive();
    unsigned sharedValidatedFrames=0,maximumSharedMasked=0;
    const auto validateShared=[&] {
        if(!sharedForest)return true;
        if(!context.rtScene.SharedForestGeometryActive() || !context.rtScene.ValidateForestRouteProof(error))return false;
        ++sharedValidatedFrames;
        maximumSharedMasked=std::max(maximumSharedMasked,context.rtScene.SharedForestMaskedInstanceCount());
        return true;
    };
    ChapterRoundTripProbe probe;
    if(!probe.Start(context.simulation.Snapshot())) {
        error="Two-way capture requires the admitted, low-lantern reunion.";return false;
    }
    const auto directory=output/(landingOnly?"landing":"roundtrip");
    std::error_code pathError;
    if(!std::filesystem::create_directory(directory,pathError)||pathError) {
        error="Two-way output directory must be new and writable.";return false;
    }
    std::ofstream rows(directory/"frames.csv"),manifest(directory/"gallery.json");
    if(!rows||!manifest) {error="Two-way evidence files could not open.";return false;}
    rows<<"frame,phase,station,submitted_tick,simulation_ms,render_cycle_ms,completed_tick,completed_serial,gpu_status,gpu_ns,device_local_bytes,host_visible_bytes,keeper_retired_bytes,keeper_retired_memory_flags,keeper_texture_retired_device_local_bytes,keeper_texture_retired_host_visible_bytes,static_omitted_triangles,static_unloads,static_restores,static_saved_owner_bytes,static_peak_owner_overlap_bytes,static_wait_frames,shared_forest_instances,shared_forest_masked";
    rows<<",submitted_x,submitted_y,submitted_z,submitted_yaw,submitted_pitch,stage_owner_match,stage_scene_epoch,stage_measurement_generation,stage_record_attempt_serial,stage_record_serial,stage_simulation_tick,stage_submission_serial,stage_frame_slot,gpu_as_status,gpu_as_serial,gpu_as_ns,gpu_dispatch_status,gpu_dispatch_serial,gpu_dispatch_ns,gpu_transfer_status,gpu_transfer_serial,gpu_transfer_ns";
    rows<<'\n';
    manifest<<"{\"schema\":2,\"scope\":\""<<(landingOnly?
        "landing-prefix attribution only; staged post-Keeper start; not full-route, owner, Shipping or phone acceptance":
        "moving fixed-step validation after staged Keeper death and first ascent; Diagnostic, automatic static detail and Keeper lifetimes; not owner or Shipping performance acceptance")
        <<"\",\"maximumSimulationTicksPerRender\":4,\"simulationHz\":60,\"captures\":[\n";
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
    {
        // Same fixed-pose warmup for control and candidate. Rendering owns no
        // gameplay ticks here; the existing probe below remains the only stepper.
        const auto tick=context.simulation.Snapshot().tickIndex;
        for(unsigned warm=0;warm<64;++warm) {
            bool presented=false;
            if(!RenderFrame(context,clear,presented) || !presented ||
               context.lastFramePresentation!=horde::telemetry::RtPresentationOutcome::Presented ||
               context.simulation.Snapshot().tickIndex!=tick) {
                error="Chapter warmup lost stable RT presentation or advanced gameplay.";return false;
            }
        }
    }
    if(!capture("initial-reunion"))return false;
    const auto start=std::chrono::steady_clock::now();
    while(frames++<(landingOnly?256u:10000u) && probe.Phase()!=ChapterProbePhase::Complete &&
          (!landingOnly || probe.Phase()==ChapterProbePhase::Landing)) {
        if(std::chrono::steady_clock::now()-start>std::chrono::seconds(landingOnly?30:150)) {
            error=landingOnly?"Landing observation exceeded its bounded 30-second run.":
                "Two-way capture exceeded its bounded 150-second run.";return false;
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
        if(!validateShared())return false;
        if(context.simulation.Snapshot().tickIndex!=beforeTick+steps) {
            error="Platform rendering advanced the probe's already-owned simulation tick.";return false;
        }
        const auto completed=context.rtFrameEvidence.PublishedStateByValue();
        const auto inventory=context.rtScene.ResourceInventory();
        const auto keeper=context.rtScene.KeeperVertexLifetime();
        const auto keeperTextures=context.rtScene.KeeperTextureLifetime();
        const auto detail=context.rtScene.OrdinaryStaticDetailEvidence();
        rows<<frames<<','<<ChapterProbePhaseName(phase)<<','<<station<<','<<context.simulation.Snapshot().tickIndex<<','
            <<std::chrono::duration<double,std::milli>(simulated-before).count()<<','
            <<std::chrono::duration<double,std::milli>(rendered-simulated).count()<<','
            <<(completed.hasCompletedEvidence?completed.completedEvidence.identity.submitted.frame.simulationTick:0)<<','
            <<(completed.hasCompletedEvidence?completed.completedEvidence.identity.submitted.submissionSerial:0)<<','
            <<static_cast<unsigned>(completed.completedEvidence.gpu.status)<<','
            <<(completed.hasCompletedEvidence&&completed.completedEvidence.gpu.hasDuration?completed.completedEvidence.gpu.durationNanoseconds:0)<<','
            <<inventory.deviceLocalBytes<<','<<inventory.hostVisibleBytes<<','
            <<keeper.retiredAllocationBytes<<','<<keeper.retiredMemoryFlags<<','
            <<keeperTextures.retiredDeviceLocalBytes<<','<<keeperTextures.retiredHostVisibleBytes<<','
            <<detail.omittedTriangles<<','<<detail.unloads<<','<<detail.restores<<','<<detail.savedOwnerBytes<<','
            <<detail.peakOverlapOwnerBytes<<','<<detail.waitFrames<<','<<(sharedForest?64u:0u)<<','
            <<context.rtScene.SharedForestMaskedInstanceCount();
        {
            const auto& state=context.simulation.Snapshot();
            rows<<','<<state.playerX<<','<<state.playerSupportWorldY<<','<<state.playerZ<<','
                <<state.playerYawRadians<<','<<state.playerPitchRadians;
            WriteWindowsGpuStageCsv(rows,context,completed.completedEvidence.identity.submitted);
        }
        rows<<'\n';
        if(frames%120==0)rows.flush();
        const bool transition=phase!=probe.Phase() || station!=probe.Station();
        const bool quadrant=(phase==ChapterProbePhase::Landing || phase==ChapterProbePhase::Lookout ||
            phase==ChapterProbePhase::Lower || phase==ChapterProbePhase::Passage) && quarter!=probe.SweepQuarter();
        const auto& actor=context.simulation.Snapshot().chapterCompanion;
        if(!kitWalkCaptured && phase==ChapterProbePhase::Raise && actor.walking && actor.animationSeconds>=.3f) {
            if(!capture("kit-walking"))return false;
            kitWalkCaptured=true;
        }
        if((transition||quadrant) && !capture(std::string(ChapterProbePhaseName(phase))+"-"+std::to_string(station)))return false;
    }
    if(landingOnly) {
        if(probe.Phase()!=ChapterProbePhase::Raise || !capture("landing-complete")) {
            error="Landing observation did not finish the original landing prefix: "+error;return false;
        }
        manifest<<"\n],\"frames\":"<<frames-1<<",\"warmupFrames\":64,\"fullRouteValidated\":false,\"complete\":true}\n";
        rows.flush();manifest.flush();
        if(!rows||!manifest) {error="Landing evidence write failed.";return false;}
        error.clear();return true;
    }
    if(probe.Phase()!=ChapterProbePhase::Complete) {error="Two-way capture exhausted its finite frame roster.";return false;}
    const auto& last=context.simulation.Snapshot();
    if(!last.rescue.exteriorSide||last.rescue.equipmentStowed||!last.rescue.sawDescent||!last.rescue.sawLanding||
       last.rescue.claimCount!=1||last.rescue.deploymentCount!=1||!last.chapterDialogue.proofAccepted||!kitWalkCaptured) {
        error="Two-way capture lacks final progression/equipment witnesses.";return false;
    }
    const auto retired=context.rtScene.KeeperVertexLifetime();
    const auto texturesRetired=context.rtScene.KeeperTextureLifetime();
    if(!retired.retired || retired.retirementCount!=1 || retired.restorationCount!=0 ||
       !texturesRetired.retired || texturesRetired.retirementCount!=1 || texturesRetired.restorationCount!=0 ||
       texturesRetired.retiredDeviceLocalBytes==0) {
        error="Two-way traversal lacks exactly one completed Keeper vertex/texture retirement.";return false;
    }
    // Exercise ordinary route-reset input after the repeated ascent. The same
    // device/AS/descriptor owners must safely restore Keeper storage.
    auto reset=context.simulationInput;reset.moveForward=reset.moveStrafe=0;
    ++reset.commands.routeReset;reset.paused=false;
    context.simulationInput=reset;
    context.simulation.StepFixed(reset,1.f/60.f,context.simulation.Snapshot().inputPublicationSequence+1);
    bool resetPresented=false;
    if(!RenderFrame(context,clear,resetPresented)||!resetPresented||!validateShared()||
       !capture("reset-opening")) {
        error="Chapter reset failed current-pose RT restoration: "+context.lastRtFrameError;return false;
    }
    const auto restored=context.rtScene.KeeperVertexLifetime();
    const auto texturesRestored=context.rtScene.KeeperTextureLifetime();
    const auto& resetState=context.simulation.Snapshot();
    if(restored.retired || restored.retirementCount!=1 || restored.restorationCount!=1 ||
       texturesRestored.retired || texturesRestored.retirementCount!=1 || texturesRestored.restorationCount!=1 ||
       resetState.lich.phase==horde::gameplay::LichPhase::Dead ||
       resetState.chapterCompanion.grounded || resetState.rescue.claimCount!=0) {
        error="Ordinary reset left stale companion or retired Keeper storage.";return false;
    }
    const auto detail=context.rtScene.OrdinaryStaticDetailEvidence();
    if(sharedForest) {
        // Resident shared masters use whole-instance decisions, not the baked
        // owner's eviction transaction. Keep the legacy assertion below intact.
        // Every route frame and reset above validates exact submitted owners and
        // masks; an alive Keeper at reset must retain every original placement.
        if(!sharedValidatedFrames || !context.rtScene.ValidateForestRouteProof(error,false) ||
           !context.rtScene.StaticGeometryHasCompleteContributors() ||
           context.simulation.Snapshot().staticDetailLease.resourceGeneration)return false;
    }else if(!detail.unloads || detail.restores!=detail.unloads || detail.savedOwnerBytes ||
             !context.rtScene.StaticGeometryHasCompleteContributors()) {
        error="Two-way capture lacks automatic static-detail unload and complete restoration.";return false;
    }
    manifest<<"\n],\"frames\":"<<frames-1
        <<",\"warmupFrames\":64,\"fullRouteValidated\":true,\"recreationTestRequiredSeparately\":true"
        <<",\"sharedForestResident\":"<<(sharedForest?"true":"false")
        <<",\"sharedForestValidatedFrames\":"<<sharedValidatedFrames
        <<",\"maximumSharedForestMaskedInstances\":"<<maximumSharedMasked
        <<",\"maximumNormalStaticOwnerSavingBytes\":"<<detail.peakSavedOwnerBytes
        <<",\"staticDetailUnloads\":"<<detail.unloads<<",\"staticDetailRestores\":"<<detail.restores<<','
        <<"\"keeperVertexLifetime\":{\"retiredAllocationBytes\":"<<retired.retiredAllocationBytes
        <<",\"retiredMemoryFlags\":"<<retired.retiredMemoryFlags
        <<",\"retirements\":"<<restored.retirementCount<<",\"restorations\":"<<restored.restorationCount
        <<",\"resetRestored\":true},\"keeperTextureLifetime\":{\"retiredDeviceLocalBytes\":"
        <<texturesRetired.retiredDeviceLocalBytes<<",\"retiredHostVisibleBytes\":"<<texturesRetired.retiredHostVisibleBytes
        <<",\"baseMemoryFlags\":"<<texturesRetired.baseMemoryFlags<<",\"emissiveMemoryFlags\":"<<texturesRetired.emissiveMemoryFlags
        <<",\"retirements\":"<<texturesRestored.retirementCount<<",\"restorations\":"<<texturesRestored.restorationCount
        <<",\"resetRestored\":true},\"complete\":true}\n";
    rows.flush();manifest.flush();
    if(!rows||!manifest) {error="Two-way evidence write failed.";return false;}
    error.clear();return true;
}
