// Custom-only opening benchmark adapter. Resource/gameplay work stays on the
// render owner; JNI publishes only bounded immutable commands under its mutex.
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
        }
        cancel=gOpeningBenchmarkOwner.Busy() && gOpeningBenchmarkOwner.Cancelled();
        if(gOpeningBenchmarkOwner.WantsShowcase() && gOpeningBenchmarkOriginalSettings &&
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
    bool start=false;
    {
        std::lock_guard lock(gOpeningBenchmarkMutex);
        gOpeningBenchmarkEligibility=eligibility;
        if(actuallyPresented && context.sceneProfile==Profile::Showcase) {
            gOpeningBenchmarkOwner.ShowcasePresented(context.surfaceGeneration);
            start=gOpeningBenchmarkOwner.TakeStart();
        }
        if(actuallyPresented && context.sceneProfile==Profile::EntryMenu && gOpeningBenchmarkOwner.Returning()) {
            const bool unchanged=!gOpeningBenchmarkOriginalSettings || context.graphicsSettings==*gOpeningBenchmarkOriginalSettings;
            if(!unchanged)gOpeningBenchmarkOwner.Block();
            else if(gOpeningBenchmarkOwner.EntryPresented(context.surfaceGeneration,true))
                gRtLabBenchmarkRoute.store(gOpeningBenchmarkOriginalRoute,std::memory_order_release);
        }
    }
    if(start) {
        // Schedule the unchanged route runner for the NEXT owner frame, after
        // genuine Showcase presentation. This is Custom/unranked, not a profile.
        gRtLabBenchmarkRoute.store(true,std::memory_order_release);
        gInAppBenchmarkCancelRequested.store(false,std::memory_order_release);
        gInAppBenchmarkStatus.store(1,std::memory_order_release);
        gInAppBenchmarkRequested.store(true,std::memory_order_release);
    }
}
