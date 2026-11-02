// Included by DiagnosticWindow.cpp and the focused production-transition fixture.
// Simulation, feedback and native window operations remain supplied by their owner.

void ShowDeathMenu(VulkanSurfaceContext& context)
{
    if(context.lookoutObjective)context.lookoutObjective->Close();
    if (context.deathOverlayVisible && context.pauseMenuVisible)
    {
        return;
    }
    context.deathOverlayVisible = true;
    context.pauseMenuVisible = true;
    context.settingsVisible = false;
    context.diagnosticsVisible = false;
    context.benchmarkReportVisible = false;
    ApplyOverlayState(context);
    RECT clientRect{};
    GetClientRect(context.windowHandle, &clientRect);
    LayoutOverlayControls(context.windowHandle,
                          clientRect.right - clientRect.left,
                          clientRect.bottom - clientRect.top);
    FocusGameControl(GetDlgItem(context.windowHandle, kResumeButtonId));
}


bool ApplyPlayerRetryCheckpoint(VulkanSurfaceContext& context, const std::int32_t checkpointId)
{
    if (checkpointId != 0 && checkpointId != 9)
    {
        return false;
    }
    const horde::gameplay::ShowcaseCheckpoint* checkpoint =
        horde::gameplay::FindShowcaseCheckpoint(checkpointId);
    if (checkpoint == nullptr)
    {
        return false;
    }
    if(context.campaignConfirmedRetry && context.campaignPause)return false;
    const auto& beforeRetry=context.simulation.Snapshot();
    const bool confirmedRetry=beforeRetry.healingFlaskEnabled &&
        beforeRetry.developmentRescueJourney && beforeRetry.rescue.claimCount!=0;
    const auto beforeRetryGeneration=beforeRetry.retryGeneration;
    ClearDesktopInput(context); // Retire held mouse/controller/menu input before a new gameplay lifetime.
    StopPrivateLichRevivalAudio(context);
    context.delayedFeedback.Clear();
    context.simulation.RetryEncounter();
    if(confirmedRetry) {
        // The campaign owner activates the confirmed-record barrier next pump.
        // Alive alone must not close the death pause or install a legacy camera.
        context.campaignPause=true;context.pauseMenuVisible=true;
        context.simulationInput.paused=true;ApplyOverlayState(context);
        return true;
    }
    const auto& retried=context.simulation.Snapshot();
    if(retried.playerVitals.phase!=horde::gameplay::PlayerLifePhase::Alive ||
       retried.playerVitals.vitality<=0 || retried.retryGeneration==beforeRetryGeneration) {
        // A failed/deferred native retry cannot dismiss the existing death UI.
        if(context.deathOverlayVisible)ShowDeathMenu(context);
        return false;
    }
    context.deathOverlayVisible=false;
    context.benchmarkEvidence.Cancel();
    if (context.rtFrameEvidenceInitialised)
    {
        (void)context.rtFrameEvidence.ApplyEvent(
            horde::telemetry::RtLifecycleEvent::Retry);
    }
    context.simulation.ClearEvents();
    context.simulationInput.hasAuthoritativePlayerPose = false;
    context.simulationInput.paused = false;
    MirrorSimulationSnapshot(context);
    context.debugEnemyOverride = horde::gameplay::EnemyKind::None;
    context.pauseMenuVisible = false;
    context.settingsVisible = false;
    context.diagnosticsVisible = false;
    context.benchmarkReportVisible = false;
    ApplyOverlayState(context);
    UpdateVitalityHud(context);
    FocusGameControl(context.windowHandle);
    return true;
}
