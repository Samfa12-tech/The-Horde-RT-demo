// Opening benchmark adapter. Resource/gameplay work stays on the
// render owner; JNI publishes only bounded immutable commands under its mutex.
bool ReadOpeningBenchmarkActual(const SwapchainContext& context,
    horde::gameplay::BenchmarkProfileConfiguration& actual,bool fixed)
{
    using namespace horde;
    if(!context.useRtPath || !context.rtScene.IsReady() || !context.rtFrameEvidenceInitialised)return false;
    const auto p=context.rtFrameEvidence.PublishedStateByValue();
    const auto& s=p.completedEvidence;const auto& owner=s.identity.submitted;
    if(!p.hasCompletedEvidence || !p.presented || !s.identity.completionSerial ||
       owner.frame.sceneEpoch!=p.sceneEpoch || owner.frame.measurementGeneration!=p.measurementGeneration ||
       s.presentation.outcome!=telemetry::RtPresentationOutcome::Presented ||
       !s.scene.dispatch.sceneReady || !s.scene.dispatch.rtDispatchRecorded || !s.scene.dispatch.swapchainCopyRecorded ||
       !s.scene.dimensions || !telemetry::ValidRtFrameDimensionsEvidence(*s.scene.dimensions) ||
       !s.scene.fireQuality || !s.scene.shadowQuality || !s.scene.actualUploadedMistEnabled || !s.scene.actualUploadedDustQuality)return false;
    const auto& d=*s.scene.dimensions;const auto dispatch=context.rtScene.DispatchExtent();
    if(dispatch.width!=d.internalWidth || dispatch.height!=d.internalHeight ||
       context.swapchainExtent.width!=d.outputWidth || context.swapchainExtent.height!=d.outputHeight ||
       d.presentationTransform!=graphics::RtPresentationTransformIndex(context.rtPresentationTransform))return false;
    actual={};actual.platform=graphics::GraphicsPlatform::Android;
    switch(s.scene.pipeline.executionMode) {
    case telemetry::RtExecutionMode::RayTracingPipeline:actual.backend=graphics::GraphicsBackend::RayTracingPipeline;break;
    case telemetry::RtExecutionMode::RayQueryCompute:actual.backend=graphics::GraphicsBackend::RayQueryCompute;break;
    default:return false;
    }
    if((actual.backend==graphics::GraphicsBackend::RayTracingPipeline && context.executionBackend!=vulkan::RtExecutionBackend::RayTracingPipeline) ||
       (actual.backend==graphics::GraphicsBackend::RayQueryCompute && context.executionBackend!=vulkan::RtExecutionBackend::RayQueryCompute))return false;
    switch(s.scene.pipeline.dielectricQuality) {
    case telemetry::RtDielectricQuality::Mobile:actual.optical=graphics::OpticalProfile::Mobile;break;
    case telemetry::RtDielectricQuality::High:actual.optical=graphics::OpticalProfile::High;break;
    default:return false;
    }
    actual.graphics=context.graphicsSettings;
    switch(s.scene.pipeline.waterQuality) {
    case telemetry::RtWaterQuality::Off:actual.graphics.waterQuality=graphics::WaterQuality::Off;break;
    case telemetry::RtWaterQuality::Mobile:actual.graphics.waterQuality=graphics::WaterQuality::Mobile;break;
    case telemetry::RtWaterQuality::High:actual.graphics.waterQuality=graphics::WaterQuality::High;break;
    default:return false;
    }
    switch(s.scene.fireQuality->quality) {
    case telemetry::RtFireQuality::Mobile:actual.graphics.fireDetail=graphics::FireDetail::Mobile;break;
    case telemetry::RtFireQuality::High:actual.graphics.fireDetail=graphics::FireDetail::High;break;
    case telemetry::RtFireQuality::Low:actual.graphics.fireDetail=graphics::FireDetail::Low;break;
    default:return false;
    }
    switch(s.scene.shadowQuality->mode) {
    case telemetry::RtShadowMode::Current:actual.graphics.shadowQuality=graphics::ShadowQuality::Current;break;
    case telemetry::RtShadowMode::Higher:actual.graphics.shadowQuality=graphics::ShadowQuality::Higher;break;
    default:return false;
    }
    actual.graphics.mistEnabled=*s.scene.actualUploadedMistEnabled;
    actual.graphics.dustQuality=*s.scene.actualUploadedDustQuality;
    actual.graphics.glassEnabled=context.rtScene.GlassEnabled();
    actual.internal={d.internalWidth,d.internalHeight};actual.output={d.outputWidth,d.outputHeight};
    actual.cameraAspect=d.cameraAspect;actual.presentationTransform=d.presentationTransform;
    actual.dynamicResolutionEnabled=d.dynamicResolutionEnabled;
    actual.outputPolicy=fixed?gameplay::BenchmarkOutputPolicy::FixedBufferPreserveAspect:gameplay::BenchmarkOutputPolicy::Native;
    return gameplay::ValidBenchmarkProfileConfiguration(actual);
}

void AnnotateOpeningBenchmarkReport(std::string& text,std::string& json)
{
    std::lock_guard lock(gOpeningBenchmarkMutex);
    if(!gOpeningBenchmarkProvisional)return;
    text+="\nRequested profile: mobile-rt-provisional-v1\nClassification: provisional-unranked (not eligible for public ranking).\n";
    if(const auto end=json.rfind('}');end!=std::string::npos)
        json.insert(end,",\n\"androidBenchmarkProfileAttempt\":{\"requestedProfile\":\"mobile-rt-provisional-v1\","
            "\"classification\":\"provisional-unranked\",\"ranked\":false,\"fixedOutputAcknowledged\":"+
            std::string(gOpeningBenchmarkOutput.Ready()?"true":"false")+",\"restorationPending\":true}\n");
}

std::string OpeningBenchmarkFrameJson(const SwapchainContext& context)
{
    const auto p=context.rtFrameEvidence.PublishedStateByValue();
    if(!p.hasCompletedEvidence || !p.completedEvidence.scene.dimensions)return "null";
    const auto& id=p.completedEvidence.identity;
    std::ostringstream out;out<<"{\"sceneEpoch\":"<<id.submitted.frame.sceneEpoch<<
        ",\"measurementGeneration\":"<<id.submitted.frame.measurementGeneration<<
        ",\"submissionSerial\":"<<id.submitted.submissionSerial<<",\"completionSerial\":"<<id.completionSerial<<",\"dimensions\":";
    horde::telemetry::WriteRtFrameDimensionsEvidenceJson(out,*p.completedEvidence.scene.dimensions);
    out<<'}';return out.str();
}

void FinishOpeningBenchmarkOutputReceipt(const SwapchainContext& context)
{
    std::string json,applied,restored;std::array<std::int64_t,8> state{};
    {
        std::lock_guard lock(gOpeningBenchmarkMutex);
        applied=gOpeningBenchmarkAppliedFrameJson.empty()?"null":gOpeningBenchmarkAppliedFrameJson;
        restored=gOpeningBenchmarkRestoredFrameJson.empty()?"null":gOpeningBenchmarkRestoredFrameJson;
        json=gOpeningBenchmarkProfileJson;
        state=gOpeningBenchmarkOutput.Snapshot();
    }
    const std::string receipt="{\"requestedProfile\":\"mobile-rt-provisional-v1\",\"classification\":\"provisional-unranked\","
        "\"ranked\":false,\"requestSerial\":"+std::to_string(state[0])+",\"originSurfaceGeneration\":"+std::to_string(state[1])+
        ",\"returnedSurfaceGeneration\":"+std::to_string(state[2])+",\"actualRestorationAcknowledged\":true,\"applied\":"+applied+",\"restored\":"+restored+"}\n";
    WriteTextFile(context.reportDirectory+"/HordeLanternRT-benchmark-profile-latest.json",receipt);
    if(!json.empty()) {
        const std::string pending="\"restorationPending\":true";
        if(const auto at=json.find(pending);at!=std::string::npos)
            json.replace(at,pending.size(),"\"restorationPending\":false,\"actualRestorationAcknowledged\":true,\"applied\":"+applied+",\"restored\":"+restored);
        WriteTextFile(context.reportDirectory+"/HordeLanternRT-benchmark-latest.json",json);
    }
}

horde::graphics::OpeningBenchmarkEligibility OpeningBenchmarkEligibilityOnOwner(const SwapchainContext& context)
{
    horde::graphics::OpeningBenchmarkEligibility e;
    {
        std::lock_guard lock(gGraphicsMutex);
        e.entryVisible=gEntryControls.enabled && !gEntryControls.play &&
            horde::graphics::CurrentEntryMenuControls(gEntryControls,context.surfaceGeneration);
        e.entryScene=gSurfaceSessions.IsCurrent(context.surfaceGeneration) && !gSurfaceSessions.IsSuspended(context.surfaceGeneration) &&
            context.sceneProfile==horde::vulkan::raytracing::RtSceneProfile::EntryMenu &&
            gEntryState[0]==static_cast<jlong>(context.surfaceGeneration) && gEntryState[1]==2 &&
            gEntryState[2]==0 && gEntryState[4]==1 && gAppliedGraphics.rtPresented;
        e.playHandoff=gEntryControls.play || context.entryHandoff.PlayRequested();
        e.graphicsTransition=gPreviewControls.enabled || context.previewTransitionFailed ||
            gRequestedGraphics.serial!=context.graphicsSerial ||
            (gGraphicsEdit && gGraphicsEdit->State()!=horde::graphics::GraphicsEditState::Committed);
    }
    {
        std::lock_guard lock(gCampaignMutex);e.pendingCampaign=gCampaignCommand.has_value();
    }
    if(gCampaignSession) {
        const auto state=gCampaignSession->State();e.activeCampaign=state.activeSlot.has_value();
        e.pendingCampaign=e.pendingCampaign || state.outcome==campaign::PlayOutcome::Pending;
    }
    e.pendingCampaign=e.pendingCampaign || gCampaignPreparation.has_value() || bool(gCampaignPreparedScene) ||
        gCampaignBarrier.State().loaded || gCampaignPauseGate.load(std::memory_order_acquire) ||
        gCampaignShowcaseOverride.load(std::memory_order_acquire) || gCampaignRetirementUnproved;
    e.benchmarkBusy=context.inAppBenchmark.IsRunning() || gInAppBenchmarkRequested.load(std::memory_order_acquire) ||
        gInAppBenchmarkStatus.load(std::memory_order_acquire)==1 ||
        gBenchmarkCheckpointRequested.load(std::memory_order_acquire)!=-1 ||
        gCaptureCheckpointRequested.load(std::memory_order_acquire)!=-1 ||
        gRouteReplayRequested.load(std::memory_order_acquire) || gDebugCaptureActive.load(std::memory_order_acquire) ||
        gCombatPracticeRequested.load(std::memory_order_acquire)!=-1 || gCombatPracticeActive.load(std::memory_order_acquire) ||
        context.captureActive || context.routeReplayActive || context.environmentSurvey ||
        gGameSimulation.Snapshot().playerMountProfile!=horde::gameplay::items::PlayerMountProfile::AnatomicalBody;
    return e;
}

void PrepareOpeningBenchmarkOnOwner(SwapchainContext& context)
{
    std::lock_guard admission(gDebugRequestAdmissionMutex);
    using Owner=horde::platform::android::AndroidOpeningBenchmarkOwner;
    const auto eligibility=OpeningBenchmarkEligibilityOnOwner(context);
    bool cancel=false;
    {
        std::lock_guard lock(gOpeningBenchmarkMutex);
        if(gOpeningBenchmarkOwner.State()==Owner::Phase::Queued && gOpeningBenchmarkOwner.Prepare(eligibility)) {
            gOpeningBenchmarkOriginalSettings=context.graphicsSettings;
            gOpeningBenchmarkGraphicsSerial=context.graphicsSerial;
            gOpeningBenchmarkOriginalRoute=gRtLabBenchmarkRoute.load(std::memory_order_acquire);
            if(gOpeningBenchmarkProvisional) {
                horde::gameplay::BenchmarkProfileConfiguration original;
                const auto identity=horde::graphics::RtPresentationTransform::Identity;
                const bool supported=context.rtPresentationTransform==identity &&
                    context.surfaceCurrentTransform==VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR &&
                    ReadOpeningBenchmarkActual(context,original,false) &&
                    original.optical==horde::graphics::OpticalProfile::Mobile;
                const auto s=gOpeningBenchmarkOwner.Snapshot();
                if(!supported || !gOpeningBenchmarkOutput.Begin(s[0],s[5],context.graphicsSettings))
                    gOpeningBenchmarkOwner.BeginReturn(4); // Unavailable, never silently Custom.
            }
        }
        if(gOpeningBenchmarkProvisional && gOpeningBenchmarkOutput.Cancelled() &&
           gOpeningBenchmarkOwner.Busy() && !gOpeningBenchmarkOwner.Returning()) {
            const auto s=gOpeningBenchmarkOwner.Snapshot();gOpeningBenchmarkOwner.Cancel(s[0],s[5]);
        }
        cancel=gOpeningBenchmarkOwner.Busy() && gOpeningBenchmarkOwner.Cancelled();
        if(!gOpeningBenchmarkProvisional && gOpeningBenchmarkOwner.WantsShowcase() && gOpeningBenchmarkOriginalSettings &&
           (context.graphicsSettings!=*gOpeningBenchmarkOriginalSettings || context.graphicsSerial!=gOpeningBenchmarkGraphicsSerial)) {
            const auto s=gOpeningBenchmarkOwner.Snapshot();
            gOpeningBenchmarkOwner.Cancel(s[0],s[5]);cancel=true;
        }
    }
    if(cancel) {
        gInAppBenchmarkRequested.store(false,std::memory_order_release);
        CancelActiveInAppBenchmark(context);
        OpeningBenchmarkFinished(3);
    }
}

// Returns false while quiescent: no new GPU acquisition/submission is permitted
// between proving the drain and observing/recreating the UI's actual buffer.
bool ProcessOpeningBenchmarkOutputOnOwner(SwapchainContext& context)
{
    using T=horde::platform::android::AndroidBenchmarkOutputTransaction;
    T::Phase phase;
    {
        std::lock_guard lock(gOpeningBenchmarkMutex);
        if(!gOpeningBenchmarkProvisional)return true;
        if(gOpeningBenchmarkOwner.Returning())gOpeningBenchmarkOutput.Restore(gOpeningBenchmarkOwner.Cancelled());
        phase=gOpeningBenchmarkOutput.State();
        if(phase==T::Phase::Idle || phase==T::Phase::Restored)return true;
        if(phase==T::Phase::Failed) {gOpeningBenchmarkOwner.Block();return false;}
    }
    if(phase==T::Phase::DrainApply || phase==T::Phase::DrainRestore) {
        const bool acquireDrained=ConsumePendingImageAcquire(context);
        const auto idle=acquireDrained?vkDeviceWaitIdle(context.device):VK_ERROR_DEVICE_LOST;
        const bool drained=CompleteRtEvidenceAfterDeviceIdle(context,idle) &&
            context.presentCompletionFences.Drain()==VK_SUCCESS;
        std::lock_guard lock(gOpeningBenchmarkMutex);
        if(!gOpeningBenchmarkOutput.Drained(drained))gOpeningBenchmarkOwner.Block();
        return false;
    }
    if(phase==T::Phase::AwaitApply || phase==T::Phase::AwaitRestore)return false;
    if(phase==T::Phase::RecreateApply || phase==T::Phase::RecreateRestore) {
        bool applying=false;std::uint32_t width=0,height=0;
        {
            std::lock_guard lock(gOpeningBenchmarkMutex);
            applying=gOpeningBenchmarkOutput.Applying();
            context.graphicsSettings=applying?gOpeningBenchmarkOutput.Requested():gOpeningBenchmarkOutput.Original();
            width=gOpeningBenchmarkOutput.Width();height=gOpeningBenchmarkOutput.Height();
        }
        context.renderScale=context.graphicsSettings.renderScalePercent/100.0f;
        const bool holderMatches=ANativeWindow_getWidth(context.window)==static_cast<int>(width) &&
            ANativeWindow_getHeight(context.window)==static_cast<int>(height);
        if(!holderMatches && applying) {
            // A structural callback raced the owner. Old work was already
            // drained; cancel and request restoration rather than rebuild a
            // different size under this provisional request.
            std::lock_guard lock(gOpeningBenchmarkMutex);
            gOpeningBenchmarkOwner.BeginReturn(4);gOpeningBenchmarkOutput.Restore();return false;
        }
        const bool recreated=holderMatches && RecreateSwapchain(context);
        const auto p=context.rtFrameEvidence.PublishedStateByValue();
        {
            std::lock_guard lock(gOpeningBenchmarkMutex);
            if(!gOpeningBenchmarkOutput.Recreated(p.sceneEpoch,p.measurementGeneration,recreated)) {
                gOpeningBenchmarkOwner.Block();return false;
            }
            if(applying && context.rtPresentationTransform!=horde::graphics::RtPresentationTransform::Identity) {
                gOpeningBenchmarkOwner.BeginReturn(4);gOpeningBenchmarkOutput.Restore();return false;
            }
            gOpeningBenchmarkOutputFrameAttempts=0;
        }
        PublishGraphicsApplied(context,true,false);
    }
    return true;
}

bool OpeningBenchmarkResetAllowedOnOwner()
{
    if(!OpeningBenchmarkBusy())return true; // Preserve existing explicit unattended/motion behavior.
    bool pending=false;
    {std::lock_guard lock(gCampaignMutex);pending=gCampaignCommand.has_value();}
    const bool active=gCampaignSession && (gCampaignSession->State().activeSlot.has_value() ||
        gCampaignSession->State().outcome==campaign::PlayOutcome::Pending);
    const bool allowed=!pending && !active && !gCampaignPreparation && !gCampaignPreparedScene &&
        !gCampaignBarrier.State().loaded &&
        gGameSimulation.Snapshot().playerMountProfile==horde::gameplay::items::PlayerMountProfile::AnatomicalBody;
    if(!allowed) {std::lock_guard lock(gOpeningBenchmarkMutex);gOpeningBenchmarkOwner.Block();}
    return allowed;
}

void OpeningBenchmarkSceneReadyOnOwner(SwapchainContext& context)
{
    using Profile=horde::vulkan::raytracing::RtSceneProfile;
    std::lock_guard lock(gOpeningBenchmarkMutex);
    if(gOpeningBenchmarkOwner.NeedsEntryRetirement()) {
        if(context.previewTransitionFailed || context.sceneProfile!=Profile::Showcase)gOpeningBenchmarkOwner.Block();
        else gOpeningBenchmarkOwner.EntryRetired(true); // ApplySceneProfile proved device-idle retirement.
    }
    if(gOpeningBenchmarkOwner.Returning() && context.sceneProfile==Profile::EntryMenu) {
        const bool unchanged=!gOpeningBenchmarkOriginalSettings || context.graphicsSettings==*gOpeningBenchmarkOriginalSettings;
        gOpeningBenchmarkOwner.ReturnRetired(unchanged,!context.previewTransitionFailed);
    }
}

void ObserveOpeningBenchmarkOnOwner(SwapchainContext& context,bool actuallyPresented)
{
    using Profile=horde::vulkan::raytracing::RtSceneProfile;
    actuallyPresented=actuallyPresented && gSurfaceSessions.IsCurrent(context.surfaceGeneration) &&
        !gSurfaceSessions.IsSuspended(context.surfaceGeneration);
    {
        std::lock_guard lock(gGraphicsMutex);
        actuallyPresented=actuallyPresented && gAppliedGraphics.rtPresented &&
            gAppliedGraphics.lifecycleGeneration==context.surfaceGeneration &&
            gAppliedGraphics.scene==GraphicsSceneForProfile(context.sceneProfile);
    }
    const auto eligibility=OpeningBenchmarkEligibilityOnOwner(context);
    bool start=false,writeOutputReceipt=false;
    {
        std::lock_guard lock(gOpeningBenchmarkMutex);
        gOpeningBenchmarkEligibility=eligibility;
        bool outputReady=!gOpeningBenchmarkProvisional;
        bool outputRestored=!gOpeningBenchmarkProvisional || gOpeningBenchmarkOutput.State()==
            horde::platform::android::AndroidBenchmarkOutputTransaction::Phase::Idle;
        if(gOpeningBenchmarkProvisional && actuallyPresented && gOpeningBenchmarkOutput.Active()) {
            const auto p=context.rtFrameEvidence.PublishedStateByValue();
            horde::gameplay::BenchmarkProfileConfiguration actual;
            if(ReadOpeningBenchmarkActual(context,actual,!gOpeningBenchmarkOwner.Returning())) {
                // This is the actual current owner after scene/profile recreation,
                // never an earlier completion belonging to the retired Entry.
                gOpeningBenchmarkOutput.ExpectCurrentOwner(p.sceneEpoch,p.measurementGeneration);
                const auto& id=p.completedEvidence.identity;
                gOpeningBenchmarkOutput.Observe(actual,id.submitted.frame.sceneEpoch,
                    id.submitted.frame.measurementGeneration,id.submitted.submissionSerial,id.completionSerial);
                if(gOpeningBenchmarkOutput.Ready() && !horde::gameplay::MatchesBenchmarkProfile(
                    horde::gameplay::BenchmarkProfileId::MobileProvisionalV1,actual)) {
                    const auto s=gOpeningBenchmarkOwner.Snapshot();gOpeningBenchmarkOwner.Cancel(s[0],s[5]);
                    gInAppBenchmarkCancelRequested.store(true,std::memory_order_release);
                }
                if(gOpeningBenchmarkOutput.Ready() && gOpeningBenchmarkAppliedFrameJson.empty())
                    gOpeningBenchmarkAppliedFrameJson=OpeningBenchmarkFrameJson(context);
                if(gOpeningBenchmarkOutput.Restored())gOpeningBenchmarkRestoredFrameJson=OpeningBenchmarkFrameJson(context);
            }
            outputReady=gOpeningBenchmarkOutput.Ready();outputRestored=gOpeningBenchmarkOutput.Restored();
            using OP=horde::platform::android::AndroidBenchmarkOutputTransaction::Phase;
            const auto phase=gOpeningBenchmarkOutput.State();
            if(phase==OP::AwaitAppliedFrame || phase==OP::AwaitRestoredFrame) {
                if(++gOpeningBenchmarkOutputFrameAttempts>kMaxFramesInFlight+4) {
                    // Bounded proof attempt, not a tolerance: missing or mismatched
                    // actual evidence never starts an approximate profile.
                    if(phase==OP::AwaitAppliedFrame) {
                        gOpeningBenchmarkOwner.BeginReturn(4);gOpeningBenchmarkOutput.Restore();
                    } else gOpeningBenchmarkOwner.Block();
                }
            }
        }
        if(actuallyPresented && outputReady && context.sceneProfile==Profile::Showcase) {
            gOpeningBenchmarkOwner.ShowcasePresented(context.surfaceGeneration);
            start=gOpeningBenchmarkOwner.TakeStart();
        }
        if(actuallyPresented && outputRestored && context.sceneProfile==Profile::EntryMenu && gOpeningBenchmarkOwner.Returning()) {
            const bool unchanged=!gOpeningBenchmarkOriginalSettings || context.graphicsSettings==*gOpeningBenchmarkOriginalSettings;
            if(!unchanged)gOpeningBenchmarkOwner.Block();
            else if(gOpeningBenchmarkOwner.EntryPresented(context.surfaceGeneration,true)) {
                gRtLabBenchmarkRoute.store(gOpeningBenchmarkOriginalRoute,std::memory_order_release);
                writeOutputReceipt=gOpeningBenchmarkProvisional && gOpeningBenchmarkOutput.Restored();
            }
        }
    }
    if(writeOutputReceipt)FinishOpeningBenchmarkOutputReceipt(context);
    if(start) {
        std::lock_guard lock(gOpeningBenchmarkMutex);
        if(gOpeningBenchmarkOwner.Cancelled() || !gOpeningBenchmarkOwner.WantsShowcase())return;
        // Schedule the unchanged route runner for the NEXT owner frame, after
        // genuine Showcase presentation (and completed profile proof if requested).
        gRtLabBenchmarkRoute.store(true,std::memory_order_release);
        gInAppBenchmarkCancelRequested.store(false,std::memory_order_release);
        gInAppBenchmarkStatus.store(1,std::memory_order_release);
        gInAppBenchmarkRequested.store(true,std::memory_order_release);
    }
}
