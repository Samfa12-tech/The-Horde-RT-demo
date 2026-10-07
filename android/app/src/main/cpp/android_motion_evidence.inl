// Included only by the Debug render owner. No second simulation or submit serial.
bool CompleteRtEvidenceAfterDeviceIdle(SwapchainContext&, VkResult);

horde::platform::android::AndroidMotionEvidenceScope AndroidMotionScope(const SwapchainContext& context)
{
    const auto p = context.rtFrameEvidence.PublishedStateByValue();
    return {context.surfaceGeneration, p.sceneEpoch, p.measurementGeneration,
            context.swapchainExtent.width, context.swapchainExtent.height};
}

bool WriteAndroidMotion(SwapchainContext& context)
{
    auto& run = *context.motion;
    std::ofstream ledger(run.path + "-ledger.tmp", std::ios::binary | std::ios::trunc);
    run.ledger.WriteJson(ledger, run.scenario);
    ledger.flush();
    ledger.close();
    if (!ledger || std::rename((run.path + "-ledger.tmp").c_str(), (run.path + "-ledger.json").c_str()) != 0)
        return false;
    std::ostringstream manifest;
    manifest << "{\"schema\":1,\"runId\":" << JsonUtf8String(run.id)
        << ",\"scenario\":" << JsonUtf8String(horde::gameplay::validation::MotionScenarioName(run.selected))
        << ",\"buildId\":" << JsonUtf8String(HORDE_RT_BUILD_ID)
        << ",\"finished\":" << (run.finished ? "true" : "false")
        << ",\"complete\":" << (run.finished && run.scenario.Complete() && !run.ledger.Failed() ? "true" : "false")
        << ",\"armed\":" << (run.armed ? "true" : "false")
        << ",\"backend\":" << JsonUtf8String(horde::vulkan::ToString(context.executionBackend))
        << ",\"scale\":" << run.settings.renderScalePercent
        << ",\"water\":" << static_cast<int>(run.settings.waterQuality)
        << ",\"fire\":" << static_cast<int>(run.settings.fireDetail)
        << ",\"cap\":" << run.settings.previewFrameCap
        << ",\"glass\":" << (run.settings.glassEnabled ? "true" : "false")
        << ",\"shadow\":" << static_cast<int>(run.settings.shadowQuality)
        << ",\"mist\":" << (run.settings.mistEnabled ? "true" : "false")
        << ",\"compiledQuality\":" << JsonUtf8String(context.rtScene.SelectedDielectricQualityName())
        << ",\"inputOrigin\":\"harness-generated axes and timestamped edges; not touch latency\""
        << ",\"limits\":{\"armingSeconds\":30,\"wallSeconds\":120,\"captures\":64}"
        << ",\"isolation\":{\"secondSimulation\":false,\"fixedDeltaOverride\":false,"
           "\"phaseForced\":false,\"preferencesWritten\":false,\"ownerAcceptance\":false,"
           "\"timingScope\":\"moving scenario with readbacks; not sustained FPS or scanout\"}"
        << ",\"captures\":" << run.captures << "]}";
    return WriteTextFile(run.path + "-manifest.tmp", manifest.str()) &&
        std::rename((run.path + "-manifest.tmp").c_str(), (run.path + "-manifest.json").c_str()) == 0;
}

void FailAndroidMotion(SwapchainContext& context, std::string_view reason)
{
    if (!context.motion || context.motion->finished) return;
    context.motion->scenario.Fail(reason);
    context.motion->finished = true;
    (void)WriteAndroidMotion(context);
    gMotionStatus.store(4, std::memory_order_release);
    __android_log_print(ANDROID_LOG_ERROR, kTag, "HORDE_MOTION failed %.*s", static_cast<int>(reason.size()), reason.data());
}

void ObserveAndroidMotionCompletion(SwapchainContext& context, bool completed)
{
    if (!context.motion || !context.motion->armed || context.motion->finished || !completed) return;
    if (!context.motion->ledger.AppendCompletedFrame(context.surfaceGeneration,
            context.rtFrameEvidence.PublishedStateByValue()))
        FailAndroidMotion(context, context.motion->ledger.Failure());
}

void ResetAndroidMotionIfRequested(SwapchainContext& context)
{
    if ((context.motion && !context.motion->finished) || !gMotionReleaseRequested.exchange(false)) return;
    std::lock_guard<std::mutex> lock(gInputPublisherMutex);
    const auto& state = gGameSimulation.Snapshot();
    auto& commands = gInputPublisherState.commands;
    commands.attack = std::max(commands.attack, state.lastConsumedAttackSequence);
    commands.parry = std::max(commands.parry, state.lastConsumedParrySequence);
    commands.dodge = std::max(commands.dodge, state.lastConsumedDodgeSequence);
    commands.retry = std::max(commands.retry, state.lastConsumedRetrySequence);
    commands.routeReset = std::max(commands.routeReset, state.lastConsumedRouteResetSequence);
    commands.interact = std::max(commands.interact, state.lastConsumedInteractSequence);
    commands.toggleHeldLightPose = std::max(commands.toggleHeldLightPose, state.lastConsumedToggleHeldLightPoseSequence);
    gInputPublisherState.combatEdgeHistory = {};
    gInputPublisherState.paused = true;
    gInputPublisherState.moveForward = gInputPublisherState.moveStrafe = 0.0f;
    PublishInputLocked();
    context.motion.reset();
    gMotionStatus.store(0, std::memory_order_release);
}

void UpdateAndroidMotionScope(SwapchainContext& context, horde::gameplay::simulation::InputSnapshot& input)
{
    if (!context.motion || !context.motion->armed || context.motion->finished) return;
    auto& run = *context.motion;
    const auto scope = AndroidMotionScope(context);
    if (scope == run.scope) return;
    // Only the scenario's ordinary retry may advance scene/measurement scope.
    if (!run.retryPending || run.ledger.HasPendingSubmissions() ||
        scope.surfaceGeneration != run.scope.surfaceGeneration ||
        scope.outputWidth != run.scope.outputWidth || scope.outputHeight != run.scope.outputHeight ||
        scope.sceneEpoch <= run.scope.sceneEpoch || scope.measurementGeneration < run.scope.measurementGeneration ||
        !run.ledger.ObserveScope(scope.surfaceGeneration, scope.sceneEpoch, scope.measurementGeneration))
    { FailAndroidMotion(context, "Motion resource scope changed without its ordinary drained retry."); return; }
    run.scope = scope;
    run.retryPending = false;
    input.paused = true; // The new retry scope must produce its own accepted frame before moving.
    input.moveForward = input.moveStrafe = 0.0f;
}

void BuildAndroidMotionInput(SwapchainContext& context,
    horde::gameplay::simulation::InputSnapshot& input, std::uint64_t now)
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::simulation;
    if (!context.motion && gMotionStatus.load(std::memory_order_acquire) == 1)
    {
        std::lock_guard<std::mutex> lock(gMotionRequestMutex);
        context.motion = std::make_unique<AndroidMotionRun>();
        auto& run = *context.motion;
        run.id = gMotionRequestedId;
        run.selected = gMotionRequestedScenario;
        run.path = context.reportDirectory + "/android-motion-" + run.id;
        run.requestedNs = now;
        run.settings = context.graphicsSettings;
        run.externalCommands = input.commands;
        run.externalYaw = input.yawRadians; run.externalPitch = input.pitchRadians; run.externalTorch = input.torchLightStrength;
        run.commands = input.commands;
        if (std::ifstream(run.path + "-manifest.json").good() ||
            std::ifstream(run.path + "-ledger.json").good() ||
            std::ifstream(run.path + "-manifest.tmp").good() || std::ifstream(run.path + "-ledger.tmp").good())
        { run.finished = true; gMotionStatus.store(4); return; } // never overwrite an unrelated run
        for (unsigned index = 0u; index < 64u; ++index)
            if (std::ifstream(run.path + '-' + std::to_string(index) + ".rgba").good())
            { run.finished = true; gMotionStatus.store(4); return; }
        if (!run.ledger.Begin(run.id, gMotionRequestedScenario))
        { FailAndroidMotion(context, run.ledger.Failure()); return; }
    }
    if (!context.motion) return;
    auto& run = *context.motion;
    if (run.finished) { input.paused = true; input.moveForward = input.moveStrafe = 0.0f; return; }
    const bool isolated = !context.captureActive && !context.benchmarkSampling && !context.routeReplayActive &&
        !context.inAppBenchmark.IsRunning() && context.graphicsSettings == run.settings &&
        context.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::Showcase &&
        context.useRtPath && context.rtScene.IsReady() && context.rtFrameEvidenceInitialised &&
        gSurfaceSessions.IsCurrent(context.surfaceGeneration) &&
        input.commands.attack == run.externalCommands.attack && input.commands.parry == run.externalCommands.parry &&
        input.commands.dodge == run.externalCommands.dodge && input.commands.retry == run.externalCommands.retry &&
        input.commands.routeReset == run.externalCommands.routeReset && input.commands.interact == run.externalCommands.interact &&
        input.commands.toggleHeldLightPose == run.externalCommands.toggleHeldLightPose &&
        input.yawRadians == run.externalYaw && input.pitchRadians == run.externalPitch && input.torchLightStrength == run.externalTorch &&
        input.moveForward == 0.0f && input.moveStrafe == 0.0f;
    if (!isolated || now == 0u || now < run.requestedNs || (run.armed && input.paused))
    { FailAndroidMotion(context, "Motion interrupted by UI/input/profile/resource ownership."); input.paused = true; return; }
    if (!run.armed)
    {
        const auto publication = context.rtFrameEvidence.PublishedStateByValue();
        if (now - run.requestedNs > 30'000'000'000ull)
        { FailAndroidMotion(context, "Current foreground RT frame did not arm before deadline."); input.paused = true; return; }
        if (input.paused || !publication.running || !publication.presented || !publication.hasCompletedEvidence ||
            publication.completedEvidence.presentation.outcome != horde::telemetry::RtPresentationOutcome::Presented ||
            publication.completedEvidence.identity.submitted.frame.sceneEpoch != publication.sceneEpoch ||
            publication.completedEvidence.identity.submitted.frame.measurementGeneration != publication.measurementGeneration ||
            !publication.completedEvidence.scene.dispatch.sceneReady ||
            !publication.completedEvidence.scene.dispatch.rtDispatchRecorded ||
            !publication.completedEvidence.scene.dispatch.swapchainCopyRecorded)
            return;
        if (!run.scenario.Begin(run.selected, gGameSimulation, now) ||
            !context.rtFrameEvidence.ApplyEvent(horde::telemetry::RtLifecycleEvent::CheckpointChange))
        { FailAndroidMotion(context, "Motion could not apply its one accepted checkpoint seed."); input.paused = true; return; }
        run.scope = AndroidMotionScope(context);
        if (!horde::platform::android::AndroidMotionScopeValid(run.scope) ||
            !run.ledger.ObserveScope(run.scope.surfaceGeneration, run.scope.sceneEpoch, run.scope.measurementGeneration))
        { FailAndroidMotion(context, "Motion seed has no current evidence scope."); input.paused = true; return; }
        gGameSimulation.ResetTiming(); gGameSimulation.ClearEvents();
        context.lastInputOwnerSteadyNs = now;
        run.armed = true;
        gMotionStatus.store(2, std::memory_order_release);
        __android_log_print(ANDROID_LOG_INFO, kTag, "HORDE_MOTION armed id=%s", run.id.c_str());
    }
    input.commands = run.commands; // retain previous generated edges, never merge real intervention
    input = run.scenario.BuildInput(gGameSimulation.Snapshot(), input, now,
        run.ledger.HasCurrentPresentedFrame(run.scope.surfaceGeneration, run.scope.sceneEpoch, run.scope.measurementGeneration));
    if (!horde::platform::android::StampMotionCombatEdges(input, run.commands, now, run.edges))
    { FailAndroidMotion(context, "Generated combat edge timestamp admission failed."); input.paused = true; return; }
    run.retryPending = input.commands.retry > gGameSimulation.Snapshot().lastConsumedRetrySequence;
    if (run.retryPending && run.ledger.HasPendingSubmissions())
    {
        input.commands.retry = gGameSimulation.Snapshot().lastConsumedRetrySequence;
        input.paused = true;
        FailAndroidMotion(context, "Retry cannot mutate an evidence scope with pending graphics ownership.");
    }
    run.commands = input.commands;
}

void ObserveAndroidMotionAdvance(SwapchainContext& context,
    const horde::gameplay::simulation::InputSnapshot& input, std::uint64_t now)
{
    if (!context.motion || !context.motion->armed || context.motion->finished) return;
    auto& run = *context.motion;
    const auto& snapshot = gGameSimulation.Snapshot();
    const auto events = gGameSimulation.Events().Events();
    run.scenario.ObserveAdvance(snapshot, events); // before the established semantic drain
    if (!run.ledger.AppendState(now, snapshot, input, run.scenario, events))
        FailAndroidMotion(context, run.ledger.Failure());
    else if (run.scenario.Failed()) FailAndroidMotion(context, run.scenario.Failure());
}

void AfterAndroidMotionPresent(SwapchainContext& context)
{
    if (!context.motion || !context.motion->armed || context.motion->finished) return;
    auto& run = *context.motion;
    const auto stage = run.scenario.Stage();
    const double seconds = run.scenario.SimulationSeconds();
    if (!gSurfaceSessions.IsCurrent(context.surfaceGeneration) || AndroidMotionScope(context) != run.scope)
    { FailAndroidMotion(context, "Motion present lost its foreground resource scope."); return; }
    if (stage == run.lastCaptureStage && seconds - run.lastCaptureSeconds < 2.0 && !run.scenario.Complete()) return;
    horde::telemetry::RtSubmittedFrameIdentity submitted{};
    if (!context.rtFrameEvidence.TryGetCommittedIdentity(context.currentFrame, submitted) ||
        !CompleteRtEvidenceAfterDeviceIdle(context, vkDeviceWaitIdle(context.device)) || run.finished ||
        !gSurfaceSessions.IsCurrent(context.surfaceGeneration))
    { FailAndroidMotion(context, "Motion milestone did not drain its graphics owner."); return; }
    const auto frames = run.ledger.Frames();
    if (frames.empty() || frames.back().identity.submitted.submissionSerial != submitted.submissionSerial ||
        frames.back().identity.submitted.frame.sceneEpoch != submitted.frame.sceneEpoch ||
        frames.back().identity.submitted.frame.recordSerial != submitted.frame.recordSerial ||
        frames.back().surfaceGeneration != run.scope.surfaceGeneration || run.captureCount >= 64u)
    { FailAndroidMotion(context, "Motion image lacks its exact completed current frame binding."); return; }
    horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
    std::string error;
    if (!context.rtScene.CaptureStorageImage(image, error) || image.width == 0u || image.height == 0u ||
        image.rgba.size() != static_cast<std::size_t>(image.width) * image.height * 4u ||
        !gSurfaceSessions.IsCurrent(context.surfaceGeneration) || AndroidMotionScope(context) != run.scope)
    { FailAndroidMotion(context, "Actual motion image readback failed."); return; }
    const std::string file = "android-motion-" + run.id + '-' + std::to_string(run.captureCount) + ".rgba";
    std::ofstream raw(context.reportDirectory + '/' + file, std::ios::binary | std::ios::trunc);
    raw.write(reinterpret_cast<const char*>(image.rgba.data()), static_cast<std::streamsize>(image.rgba.size()));
    raw.flush();
    if (!raw) { FailAndroidMotion(context, "Motion image file write failed."); return; }
    if (run.captureCount++) run.captures += ',';
    std::ostringstream row;
    row << "{\"file\":" << JsonUtf8String(file) << ",\"width\":" << image.width << ",\"height\":" << image.height
        << ",\"bytes\":" << image.rgba.size() << ",\"stateRow\":" << frames.back().stateRow
        << ",\"rtRow\":" << frames.size() - 1u << '}';
    run.captures += row.str(); run.lastCaptureStage = stage; run.lastCaptureSeconds = seconds;
    if (run.scenario.Complete())
    {
        run.finished = !run.ledger.HasPendingSubmissions();
        if (!run.finished) { FailAndroidMotion(context, "Terminal motion frame still has pending graphics work."); return; }
    }
    if (!WriteAndroidMotion(context))
    { run.finished = false; FailAndroidMotion(context, "Motion receipt write failed."); }
    else if (run.finished) gMotionStatus.store(3, std::memory_order_release);
}
