// Included by motion-evidence render owners. No second simulation or submit serial.
bool CompleteRtEvidenceAfterDeviceIdle(SwapchainContext&, VkResult);
void FailAndroidMotion(SwapchainContext&, std::string_view);
#if HORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING
bool MotionTimingJoinEligible(const SwapchainContext&, const AndroidMotionRun&, std::size_t&, std::string&);
bool WriteAndroidMotionValidationReport(SwapchainContext&);
#endif

horde::platform::android::AndroidMotionEvidenceScope AndroidMotionScope(const SwapchainContext& context)
{
    const auto p = context.rtFrameEvidence.PublishedStateByValue();
    return {context.surfaceGeneration, p.sceneEpoch, p.measurementGeneration,
            context.swapchainExtent.width, context.swapchainExtent.height};
}

bool WriteAndroidMotion(SwapchainContext& context)
{
    auto& run = *context.motion;
    if(run.chapterCsv.is_open())run.chapterCsv.flush();
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
        << ",\"dust\":" << static_cast<int>(run.settings.dustQuality)
        << ",\"compiledQuality\":" << JsonUtf8String(context.rtScene.SelectedDielectricQualityName())
        << ",\"inputOrigin\":\"harness-generated axes and timestamped edges; not touch latency\""
        << ",\"limits\":{\"armingSeconds\":30,\"wallSeconds\":120,\"captures\":64}"
        << ",\"isolation\":{\"secondSimulation\":false,\"fixedDeltaOverride\":false,"
           "\"phaseForced\":false,\"preferencesWritten\":false,\"ownerAcceptance\":false,"
           "\"timingScope\":\"moving scenario; capture roster declares readbacks; not sustained FPS or scanout\"}"
        << ",\"captures\":" << run.captures << "]}";
    return WriteTextFile(run.path + "-manifest.tmp", manifest.str()) &&
        std::rename((run.path + "-manifest.tmp").c_str(), (run.path + "-manifest.json").c_str()) == 0;
}

#if HORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING
void PollAndroidChapterTerminalTimingDrain(SwapchainContext& context)
{
    auto& run=*context.motion;
    const auto now=GraphicsSteadyNs();
    if(!gSurfaceSessions.IsCurrent(context.surfaceGeneration) || AndroidMotionScope(context)!=run.scope ||
       !context.presentTiming || !context.presentTiming->Enabled() || !context.presentTiming->Bound()) {
        FailAndroidMotion(context,"Chapter terminal presentation collector lost its resource scope.");return;
    }
    PollPresentTimingOnOwner(context);
    if(context.presentTiming->PendingCount()==0) {
        const auto& s=gGameSimulation.Snapshot();const auto& d=context.rtScene.OrdinaryStaticDetailEvidence();
        if(!run.scenario.Complete() || run.ledger.HasPendingSubmissions() ||
           !horde::gameplay::validation::ChapterResidencyCountersValid(run.scenario.RetainChapterDetail() ||
               run.selected==horde::gameplay::validation::MotionScenario::KeeperTorchTransition,
               run.chapterBaseline.unloads,run.chapterBaseline.restores,d.unloads,d.restores) ||
           d.savedOwnerBytes || d.preparationFailures ||
           !context.rtScene.StaticGeometryHasCompleteContributors() || s.staticDetailLease.resourceGeneration) {
            FailAndroidMotion(context,"Chapter did not restore complete contributor ownership after its ordinary unload.");return;
        }
        run.finished=true;
        std::size_t matched=0;std::string reason;
        if(!MotionTimingJoinEligible(context,run,matched,reason)) {
            run.finished=false;FailAndroidMotion(context,reason);return;
        }
        if(!WriteTextFile(run.path+"-present-timing.json",PresentTimingJson(context)) ||
           !WriteAndroidMotion(context) || !WriteAndroidMotionValidationReport(context)) {
            run.finished=false;FailAndroidMotion(context,"Chapter terminal evidence write failed.");return;
        }
        gMotionStatus.store(3,std::memory_order_release);return;
    }
    if(!now || now<run.terminalTimingDrainStartedNs || now-run.terminalTimingDrainStartedNs>=2'000'000'000ull)
        FailAndroidMotion(context,"Chapter actual presentation tail exceeded the existing two-second drain.");
}
#endif

#if HORDE_RT_ANDROID_MOTION_VALIDATION || HORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING
void WritePresentTimingCounters(std::ostream& output,
    const horde::vulkan::PresentTimingEvidence::Counters& counters)
{
    output << "{\"preparedPresents\":" << counters.preparedPresents
        << ",\"acceptedPresents\":" << counters.acceptedPresents
        << ",\"rejectedPresents\":" << counters.rejectedPresents
        << ",\"invalidRegistrations\":" << counters.invalidRegistrations
        << ",\"invalidMetadata\":" << counters.invalidMetadata
        << ",\"pendingCapacityExhausted\":" << counters.pendingCapacityExhausted
        << ",\"queryCalls\":" << counters.queryCalls
        << ",\"queryErrors\":" << counters.queryErrors
        << ",\"incompleteQueries\":" << counters.incompleteQueries
        << ",\"zeroPresentTimestamps\":" << counters.zeroPresentTimestamps
        << ",\"zeroPresentIds\":" << counters.zeroPresentIds
        << ",\"unknownPresentIds\":" << counters.unknownPresentIds
        << ",\"duplicateTimings\":" << counters.duplicateTimings
        << ",\"rowCapacityExhausted\":" << counters.rowCapacityExhausted
        << ",\"missingOnRebind\":" << counters.missingOnRebind
        << ",\"missingOnUnbind\":" << counters.missingOnUnbind
        << ",\"abandonedPreparedPresents\":" << counters.abandonedPreparedPresents << '}';
}

void WritePresentTimingCounterDelta(std::ostream& output,
    const horde::vulkan::PresentTimingEvidence::Counters& after,
    const horde::vulkan::PresentTimingEvidence::Counters& before)
{
    const auto delta = [](std::uint64_t a, std::uint64_t b) { return a >= b ? a - b : UINT64_MAX; };
    output << "{\"preparedPresents\":" << delta(after.preparedPresents, before.preparedPresents)
        << ",\"acceptedPresents\":" << delta(after.acceptedPresents, before.acceptedPresents)
        << ",\"rejectedPresents\":" << delta(after.rejectedPresents, before.rejectedPresents)
        << ",\"invalidRegistrations\":" << delta(after.invalidRegistrations, before.invalidRegistrations)
        << ",\"invalidMetadata\":" << delta(after.invalidMetadata, before.invalidMetadata)
        << ",\"pendingCapacityExhausted\":" << delta(after.pendingCapacityExhausted, before.pendingCapacityExhausted)
        << ",\"queryCalls\":" << delta(after.queryCalls, before.queryCalls)
        << ",\"queryErrors\":" << delta(after.queryErrors, before.queryErrors)
        << ",\"incompleteQueries\":" << delta(after.incompleteQueries, before.incompleteQueries)
        << ",\"zeroPresentTimestamps\":" << delta(after.zeroPresentTimestamps, before.zeroPresentTimestamps)
        << ",\"zeroPresentIds\":" << delta(after.zeroPresentIds, before.zeroPresentIds)
        << ",\"unknownPresentIds\":" << delta(after.unknownPresentIds, before.unknownPresentIds)
        << ",\"duplicateTimings\":" << delta(after.duplicateTimings, before.duplicateTimings)
        << ",\"rowCapacityExhausted\":" << delta(after.rowCapacityExhausted, before.rowCapacityExhausted)
        << ",\"missingOnRebind\":" << delta(after.missingOnRebind, before.missingOnRebind)
        << ",\"missingOnUnbind\":" << delta(after.missingOnUnbind, before.missingOnUnbind)
        << ",\"abandonedPreparedPresents\":" << delta(after.abandonedPreparedPresents, before.abandonedPreparedPresents) << '}';
}

bool MotionTimingJoinEligible(const SwapchainContext& context, const AndroidMotionRun& run,
    std::size_t& matchedRows, std::string& reason)
{
    using namespace horde::telemetry;
    using namespace horde::vulkan;
    matchedRows = 0u;
    if (!context.presentTiming || !context.presentTiming->Enabled() || !context.presentTiming->Bound())
    { reason = "actual presentation timing collector is unavailable or unbound"; return false; }
    if (!run.finished || !run.scenario.Complete() || run.ledger.Failed() ||
        run.ledger.HasPendingSubmissions())
    { reason = "motion scenario or completed-frame ledger is incomplete"; return false; }
    if (!horde::platform::android::AndroidMotionTerminalOwnerReady(
            run.scenario.Complete(), false, context.presentTiming->PendingCount() != 0u))
    { reason = "actual image presentation timings remain unresolved at terminal drain"; return false; }

    const auto& counters = context.presentTiming->GetCounters();
    const auto& before = run.timingCountersBaseline;
    const auto cleanDelta = [](std::uint64_t now, std::uint64_t start) { return now >= start && now == start; };
    if (!cleanDelta(counters.rejectedPresents, before.rejectedPresents) ||
        !cleanDelta(counters.invalidRegistrations, before.invalidRegistrations) ||
        !cleanDelta(counters.invalidMetadata, before.invalidMetadata) ||
        !cleanDelta(counters.pendingCapacityExhausted, before.pendingCapacityExhausted) ||
        !cleanDelta(counters.queryErrors, before.queryErrors) ||
        !cleanDelta(counters.zeroPresentTimestamps, before.zeroPresentTimestamps) ||
        !cleanDelta(counters.zeroPresentIds, before.zeroPresentIds) ||
        !cleanDelta(counters.unknownPresentIds, before.unknownPresentIds) ||
        !cleanDelta(counters.duplicateTimings, before.duplicateTimings) ||
        !cleanDelta(counters.rowCapacityExhausted, before.rowCapacityExhausted) ||
        !cleanDelta(counters.missingOnRebind, before.missingOnRebind) ||
        !cleanDelta(counters.missingOnUnbind, before.missingOnUnbind) ||
        !cleanDelta(counters.abandonedPreparedPresents, before.abandonedPreparedPresents))
    { reason = "actual presentation timing has an adverse run counter delta"; return false; }

    using Key = std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t>;
    const auto motionKey = [](const MotionRtRow& row) -> Key
    {
        const auto& frame = row.identity.submitted.frame;
        return {frame.measurementGeneration, frame.sceneEpoch, frame.recordSerial,
                row.identity.submitted.submissionSerial, frame.simulationTick};
    };
    const auto timingKey = [](const PresentTimingEvidence::Row& row) -> Key
    {
        return {row.measurementGeneration, row.sceneEpoch, row.recordSerial,
                row.submissionSerial, row.simulationTick};
    };
    std::vector<const MotionRtRow*> motionRows;
    motionRows.reserve(run.ledger.Frames().size());
    for (const auto& row : run.ledger.Frames()) motionRows.push_back(&row);
    std::sort(motionRows.begin(), motionRows.end(), [&](const auto* a, const auto* b)
        { return motionKey(*a) < motionKey(*b); });
    if (motionRows.empty())
    { reason = "motion run has no completed RT frames"; return false; }
    const auto& firstFrame = *motionRows.front();
    for (const auto* row : motionRows)
    {
        const auto& frame = row->identity.submitted.frame;
        const auto& first = firstFrame.identity.submitted.frame;
        if (row->surfaceGeneration != firstFrame.surfaceGeneration ||
            frame.sceneEpoch != first.sceneEpoch || frame.measurementGeneration != first.measurementGeneration)
        { reason = "motion run spans multiple resource scopes; a single pacing interval is not eligible"; return false; }
    }
    std::vector<const PresentTimingEvidence::Row*> timingRows;
    timingRows.reserve(context.presentTiming->RowCount());
    for (std::size_t i = 0u; i < context.presentTiming->RowCount(); ++i)
        if (const auto* row = context.presentTiming->ObservedRow(i)) timingRows.push_back(row);
    std::sort(timingRows.begin(), timingRows.end(), [&](const auto* a, const auto* b)
        { return timingKey(*a) < timingKey(*b); });

    std::size_t motionIndex = 0u, timingIndex = 0u;
    while (motionIndex < motionRows.size() && timingIndex < timingRows.size())
    {
        const auto& frame = *motionRows[motionIndex];
        const auto& timing = *timingRows[timingIndex];
        const Key frameKey = motionKey(frame), timingRowKey = timingKey(timing);
        if (timingRowKey < frameKey)
        {
            // Setup rows may resolve after arming, but only rows ordered before
            // the first scenario identity can be ignored. Once the run begins,
            // every observed timing row must have a ledger owner.
            if (motionIndex != 0u || !(timingRowKey < motionKey(firstFrame)))
            { reason = "an actual presentation row within the motion interval has no ledger owner"; return false; }
            ++timingIndex; continue;
        }
        if (frameKey < timingRowKey)
        { reason = "a completed motion frame has no actual presentation timestamp"; return false; }
        const bool metadataMatches = timing.actualPresentTime != 0u && timing.presentID != 0u &&
            timing.surfaceGeneration == frame.surfaceGeneration &&
            timing.scalePercent == static_cast<std::uint32_t>(std::lround(context.renderScale * 100.0f)) &&
            timing.width == context.swapchainExtent.width && timing.height == context.swapchainExtent.height &&
            timing.backend == (context.executionBackend == horde::vulkan::RtExecutionBackend::RayQueryCompute
                ? PresentTimingBackend::RayQueryCompute : PresentTimingBackend::RayTracingPipeline);
        if (!metadataMatches)
        { reason = "actual presentation row metadata does not match its completed RT owner"; return false; }
        ++matchedRows; ++motionIndex; ++timingIndex;
        if (motionIndex < motionRows.size() && motionKey(*motionRows[motionIndex]) == frameKey)
        { reason = "motion ledger contains a duplicate completed-frame identity"; return false; }
        if (timingIndex < timingRows.size() && timingKey(*timingRows[timingIndex]) == timingRowKey)
        { reason = "presentation collector contains a duplicate completed-frame identity"; return false; }
    }
    if (motionIndex != motionRows.size())
    { reason = "a completed motion frame has no actual presentation timestamp"; return false; }
    if (timingIndex < timingRows.size() &&
        !(timingKey(*timingRows[timingIndex]) < motionKey(firstFrame)))
    { reason = "an actual presentation row at or after motion start has no ledger owner"; return false; }
    if (matchedRows == 0u)
    { reason = "motion run has no matched completed presentation rows"; return false; }
    reason = "all completed motion frames join one-to-one to actual image presentation timestamps";
    return true;
}

bool WriteAndroidMotionValidationReport(SwapchainContext& context)
{
    if (!context.motion) return false;
    auto& run = *context.motion;
    PollPresentTimingOnOwner(context); // One bounded two-call poll; never waits for the display clock.
    std::size_t matchedRows = 0u;
    std::string eligibilityReason;
    const bool eligible = MotionTimingJoinEligible(context, run, matchedRows, eligibilityReason);
    std::ostringstream evidence;
    run.ledger.WriteJson(evidence, run.scenario);
    std::ostringstream json;
    const VkExtent2D internalExtent = context.rtScene.DispatchExtent();
    json << "{\"schema\":1,\"result\":" << JsonUtf8String(eligible ? "complete" : "invalid")
         << ",\"runId\":" << JsonUtf8String(run.id)
         << ",\"workload\":" << JsonUtf8String(std::string("motion-") +
                horde::gameplay::validation::MotionScenarioName(run.selected) + "-v1")
         << ",\"scenario\":" << JsonUtf8String(horde::gameplay::validation::MotionScenarioName(run.selected))
         << ",\"buildId\":" << JsonUtf8String(HORDE_RT_BUILD_ID)
         << ",\"simulationPolicy\":\"shared GameSimulation.AdvanceFrame with raw monotonic input-owner delta and normal fixed-step catch-up; generated motion axes/edges enter through the existing InputMailbox owner; no per-render fixed-delta override\""
         << ",\"limits\":{\"wallSeconds\":120,\"presentationDrainMilliseconds\":2000,\"stateRows\":16384,\"eventRows\":1024,\"rtRows\":16384,\"captures\":64,\"presentationRows\":32768,\"presentationPending\":256}"
         << ",\"settings\":{\"scalePercent\":" << static_cast<unsigned>(std::lround(context.renderScale * 100.0f))
         << ",\"width\":" << context.swapchainExtent.width << ",\"height\":" << context.swapchainExtent.height
         << ",\"internalWidth\":" << internalExtent.width << ",\"internalHeight\":" << internalExtent.height
         << ",\"backend\":" << JsonUtf8String(horde::vulkan::ToString(context.executionBackend))
         << ",\"water\":" << static_cast<int>(run.settings.waterQuality)
         << ",\"fire\":" << static_cast<int>(run.settings.fireDetail)
         << ",\"cap\":" << run.settings.previewFrameCap
         << ",\"glass\":" << (run.settings.glassEnabled ? "true" : "false")
         << ",\"shadow\":" << static_cast<int>(run.settings.shadowQuality)
         << ",\"mist\":" << (run.settings.mistEnabled ? "true" : "false")
         << ",\"dust\":" << static_cast<int>(run.settings.dustQuality) << '}';
    const auto frames = run.ledger.Frames();
    json << ",\"startingFrameIdentity\":";
    if (!frames.empty())
    {
        const auto& row = frames.front(); const auto& frame = row.identity.submitted.frame;
        json << "{\"surfaceGeneration\":" << row.surfaceGeneration << ",\"measurementGeneration\":"
             << frame.measurementGeneration << ",\"sceneEpoch\":" << frame.sceneEpoch << ",\"recordSerial\":"
             << frame.recordSerial << ",\"submissionSerial\":" << row.identity.submitted.submissionSerial
             << ",\"simulationTick\":" << frame.simulationTick << '}';
    }
    else json << "null";
    json << ",\"motionManifest\":{\"schema\":1,\"runId\":" << JsonUtf8String(run.id)
         << ",\"scenario\":" << JsonUtf8String(horde::gameplay::validation::MotionScenarioName(run.selected))
         << ",\"finished\":" << (run.finished ? "true" : "false")
         << ",\"complete\":" << (run.finished && run.scenario.Complete() && !run.ledger.Failed() ? "true" : "false")
         << ",\"armed\":" << (run.armed ? "true" : "false")
         << ",\"surfaceGeneration\":" << run.scope.surfaceGeneration
         << ",\"sceneEpoch\":" << run.scope.sceneEpoch
         << ",\"measurementGeneration\":" << run.scope.measurementGeneration
         << ",\"backend\":" << JsonUtf8String(horde::vulkan::ToString(context.executionBackend))
         << ",\"scale\":" << run.settings.renderScalePercent
         << ",\"water\":" << static_cast<int>(run.settings.waterQuality)
         << ",\"fire\":" << static_cast<int>(run.settings.fireDetail)
         << ",\"cap\":" << run.settings.previewFrameCap
         << ",\"glass\":" << (run.settings.glassEnabled ? "true" : "false")
         << ",\"shadow\":" << static_cast<int>(run.settings.shadowQuality)
         << ",\"mist\":" << (run.settings.mistEnabled ? "true" : "false")
         << ",\"dust\":" << static_cast<int>(run.settings.dustQuality)
         << ",\"compiledQuality\":" << JsonUtf8String(context.rtScene.SelectedDielectricQualityName())
         << ",\"inputOrigin\":\"harness-generated axes and timestamped edges; not touch latency\""
         << ",\"limits\":{\"armingSeconds\":30,\"wallSeconds\":120,\"presentationDrainMilliseconds\":2000,\"captures\":64}"
         << ",\"isolation\":{\"secondSimulation\":false,\"fixedDeltaOverride\":false,\"phaseForced\":false,\"preferencesWritten\":false,\"ownerAcceptance\":false,\"timingScope\":\"moving scenario with actual image presentation timestamps; capture roster declares readbacks; not scanout, photons, or long-duration acceptance\"}"
         << ",\"captures\":" << run.captureCount << '}';
    json << ",\"motionEvidence\":" << evidence.str()
         << ",\"motionImageCaptures\":" << run.captures << ']'
         << ",\"timingEligibility\":{\"eligible\":" << (eligible ? "true" : "false")
         << ",\"matchedCompletedFrames\":" << matchedRows << ",\"expectedCompletedFrames\":" << frames.size()
         << ",\"reason\":" << JsonUtf8String(eligibilityReason) << '}';
    json << ",\"imagePresentationTiming\":{\"extensionEnabled\":"
         << (context.presentTimingExtensionEnabled ? "true" : "false")
         << ",\"queryCpuWallNanoseconds\":"
         << context.presentTimingPollCpuNs << ",\"rowIndexBeforeRun\":" << run.timingRowBaseline
         << ",\"counterBaseline\":";
    WritePresentTimingCounters(json, run.timingCountersBaseline);
    json << ",\"counterDelta\":";
    if (context.presentTiming)
        WritePresentTimingCounterDelta(json, context.presentTiming->GetCounters(), run.timingCountersBaseline);
    else json << "{}";
    json << ",\"capture\":";
    if (context.presentTiming) context.presentTiming->WriteJson(json);
    else json << "{\"schemaVersion\":1,\"status\":{\"enabled\":false,\"bound\":false,\"pendingCount\":0},\"counters\":{},\"rows\":[],\"unresolved\":[]}";
    json << "}}";
    const bool saved = WriteTextFile(context.reportDirectory + "/HordeLanternRT-benchmark-latest.json", json.str());
    const std::string text = "Run ID: " + run.id + "\nPreset: motion-" +
        std::string(horde::gameplay::validation::MotionScenarioName(run.selected)) + "-v1\n" +
        "Result: " + std::string(eligible && saved ? "complete" : "invalid") + "\n" +
        "Motion pacing scope: matched completed RT frames to VK_GOOGLE_display_timing actual image presentation timestamps; not sustained or photons.\n";
    const bool textSaved = WriteTextFile(context.reportDirectory + "/HordeLanternRT-benchmark-latest.txt", text);
    {
        std::lock_guard<std::mutex> lock(gReportMutex);
        gLatestBenchmarkReport = text;
        gLatestBenchmarkProgress = eligible && saved && textSaved ? "MOTION VALIDATION COMPLETE" : "MOTION VALIDATION INVALID";
    }
    const bool complete = eligible && saved && textSaved;
    gInAppBenchmarkStatus.store(complete ? 2 : 3, std::memory_order_release);
    return complete;
}
#endif

#if HORDE_RT_ANDROID_MOTION_VALIDATION
void PollAndroidMotionTerminalTimingDrain(SwapchainContext& context)
{
    if (!context.motion || !context.motion->terminalTimingDrainPending || context.motion->finished) return;
    auto& run = *context.motion;
    const std::uint64_t now = GraphicsSteadyNs();
    if (!gSurfaceSessions.IsCurrent(context.surfaceGeneration) || AndroidMotionScope(context) != run.scope)
    { FailAndroidMotion(context, "Motion terminal timing drain lost its foreground resource scope."); return; }
    if (!context.presentTiming || !context.presentTiming->Enabled() || !context.presentTiming->Bound())
    { FailAndroidMotion(context, "Motion terminal timing drain lost the actual presentation collector."); return; }

    PollPresentTimingOnOwner(context); // Bounded collector poll; no synthetic present or display wait.
    if (horde::platform::android::AndroidMotionTerminalOwnerReady(
            run.scenario.Complete(), run.ledger.HasPendingSubmissions(),
            context.presentTiming->PendingCount() != 0u))
    {
        run.finished = true;
        if (!WriteAndroidMotion(context))
        {
            run.finished = false;
            FailAndroidMotion(context, "Motion terminal owner receipt write failed.");
            return;
        }
        const bool complete = WriteAndroidMotionValidationReport(context);
        gMotionStatus.store(complete ? 3 : 4, std::memory_order_release);
        return;
    }
    constexpr std::uint64_t kTerminalTimingDrainDeadlineNs = 2'000'000'000ull;
    if (now == 0u || now < run.terminalTimingDrainStartedNs ||
        now - run.terminalTimingDrainStartedNs >= kTerminalTimingDrainDeadlineNs)
        FailAndroidMotion(context, "Actual presentation timestamps did not resolve before the bounded terminal drain deadline.");
}
#endif

void FailAndroidMotion(SwapchainContext& context, std::string_view reason)
{
    if (!context.motion || context.motion->finished) return;
    context.motion->scenario.Fail(reason);
    context.motion->finished = true;
#if HORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING
    if(horde::gameplay::validation::IsTimedMotionScenario(context.motion->selected))
        (void)WriteTextFile(context.motion->path+"-present-timing.json",PresentTimingJson(context));
#endif
    (void)WriteAndroidMotion(context);
#if HORDE_RT_ANDROID_MOTION_VALIDATION || HORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING
    if (context.motionValidationRun || horde::gameplay::validation::IsTimedMotionScenario(context.motion->selected))
        (void)WriteAndroidMotionValidationReport(context);
#endif
    gMotionStatus.store(4, std::memory_order_release);
    __android_log_print(ANDROID_LOG_ERROR, kTag, "HORDE_MOTION failed %.*s", static_cast<int>(reason.size()), reason.data());
}

void RestoreAndroidMotionEquipmentSeed(SwapchainContext& context)
{
    if (!context.motion || (!context.motion->equipmentSeedActive &&
        !horde::gameplay::validation::IsTimedMotionScenario(context.motion->selected))) return;
    std::lock_guard<std::mutex> lock(gInputPublisherMutex);
    if(context.motion->equipmentSeedActive)context.motion->scenario.End(gGameSimulation);
    context.motion->equipmentSeedActive = false;
    const auto& state = gGameSimulation.Snapshot();
    auto& commands = gInputPublisherState.commands;
    commands.attack = std::max(commands.attack, state.lastConsumedAttackSequence);
    commands.parry = std::max(commands.parry, state.lastConsumedParrySequence);
    commands.dodge = std::max(commands.dodge, state.lastConsumedDodgeSequence);
    commands.retry = std::max(commands.retry, state.lastConsumedRetrySequence);
    commands.routeReset = std::max(commands.routeReset, state.lastConsumedRouteResetSequence);
    commands.interact = std::max(commands.interact, state.lastConsumedInteractSequence);
    commands.toggleHeldLightPose = std::max(commands.toggleHeldLightPose, state.lastConsumedToggleHeldLightPoseSequence);
    commands.runToggle=std::max(commands.runToggle,state.lastConsumedRunToggleSequence);
    commands.clearRunIntent=std::max(commands.clearRunIntent,state.lastConsumedClearRunIntentSequence);
    gInputPublisherState.runHeld=false;
    gInputPublisherState.combatEdgeHistory = {};
    gInputPublisherState.paused = true;
    gInputPublisherState.moveForward = gInputPublisherState.moveStrafe = 0.0f;
    PublishInputLocked();
}

void ObserveAndroidMotionCompletion(SwapchainContext& context, bool completed)
{
    if (!context.motion || !context.motion->armed || context.motion->finished || !completed) return;
    if (!context.motion->ledger.AppendCompletedFrame(context.surfaceGeneration,
            context.rtFrameEvidence.PublishedStateByValue()))
        FailAndroidMotion(context, context.motion->ledger.Failure());
    else if(horde::gameplay::validation::IsTimedMotionScenario(context.motion->selected)) {
        auto& run=*context.motion;const auto frames=run.ledger.Frames();
        if(frames.empty())return;
        const auto& frame=frames.back();auto& tag=run.chapterTags[frame.identity.submitted.frame.frameSlot];
        if(frame.identity.submitted.submissionSerial==run.chapterLastSerial)return;
        if(tag.serial!=frame.identity.submitted.submissionSerial || !tag.returnNs ||
           (run.chapterLastReturnNs && tag.returnNs<=run.chapterLastReturnNs)) {
            FailAndroidMotion(context,"Chapter completion lost its submitted resource/return timestamp tag.");return;
        }
        const auto& d=tag.detail;
        run.chapterCsv<<tag.returnNs<<','<<(run.chapterLastReturnNs?tag.returnNs-run.chapterLastReturnNs:0)<<','
            <<frame.identity.submitted.submissionSerial<<','<<frame.identity.submitted.frame.simulationTick<<','
            <<(run.selected==horde::gameplay::validation::MotionScenario::KeeperTorchTransition
                ?horde::gameplay::validation::MotionStageName(tag.motionStage)
                :horde::gameplay::validation::ChapterProbePhaseName(tag.phase))<<','<<tag.x<<','<<tag.y<<','<<tag.z<<','
            <<frame.resources.deviceLocalBytes<<','<<frame.resources.hostVisibleBytes<<','<<d.omittedTriangles<<','
            <<d.unloads<<','<<d.restores<<','<<d.savedOwnerBytes<<','<<d.peakOverlapOwnerBytes<<','<<d.waitFrames<<','
            <<tag.yaw<<','<<tag.pitch<<','<<tag.viewTicks<<','<<tag.detailCpuNs<<','
            <<(d.unloads-run.chapterBaseline.unloads)<<','<<(d.restores-run.chapterBaseline.restores);
        run.chapterCsv<<','<<frame.resources.tlasInstanceCount<<','
            <<frame.resources.bottomLevelAccelerationStructureCount;
#if HORDE_RT_FRAME_STAGE_PROBE
        // This is called only after this exact slot's own successful fence/idle.
        for(unsigned i=0;i<context.frameStageTimers.size();++i) {
            const auto stage=context.frameStageTimers[i].CollectCompleted(frame.identity.submitted.frame.frameSlot);
            const bool matched=tag.stageSubmitted[i] && stage.consumed && stage.hasSample &&
                stage.status==horde::vulkan::GpuFrameTimingCollectionStatus::Valid &&
                stage.submissionSequence==tag.serial && stage.frameSlot==frame.identity.submitted.frame.frameSlot;
            run.chapterCsv<<','<<(matched?"valid":"unavailable-or-mismatched")<<','<<stage.submissionSequence<<',';
            if(matched)run.chapterCsv<<static_cast<std::uint64_t>(std::llround(stage.sample.milliseconds*1e6));
        }
#endif
#if HORDE_RT_DISPATCH_CLOCK_PROBE
        std::vector<horde::vulkan::raytracing::RtDispatchClockSample> clockSamples;
        std::array<std::uint64_t,8> clockTotals{};
        std::string clockError;
        const bool clocksValid=context.rtScene.CollectDispatchClocks(tag.clockTicket,clockSamples,clockTotals,clockError);
        run.chapterCsv<<','<<(clocksValid?"valid":"unavailable-or-mismatched")<<','<<tag.clockTicket<<',';
        if(clocksValid)run.chapterCsv<<clockSamples.size();
        for(auto ticks:clockTotals){run.chapterCsv<<',';if(clocksValid)run.chapterCsv<<ticks;}
        // A small raw witness per held view permits independent ticket/pixel/sum checks.
        constexpr std::array<std::string_view,4> clockViews{"landing-sweep","lower-sweep","passage-sweep","repeat-ascent"};
        for(unsigned i=0;i<clockViews.size();++i)if(clocksValid && !run.clockRawSaved[i] &&
            clockViews[i]==horde::gameplay::validation::ChapterProbePhaseName(tag.phase) &&
            tag.viewTicks>=180 && tag.viewTicks<=240) {
            std::ofstream raw(run.path+"-clock-"+std::string(clockViews[i])+"-"+std::to_string(tag.serial)+".bin",std::ios::binary);
            raw.write(reinterpret_cast<const char*>(clockSamples.data()),clockSamples.size()*sizeof(clockSamples.front()));
            run.clockRawSaved[i]=bool(raw);
        }
#endif
        run.chapterCsv<<'\n';
        run.chapterLastReturnNs=tag.returnNs;
        run.chapterLastSerial=tag.serial;
        if(!run.chapterCsv)FailAndroidMotion(context,"Chapter resource ledger write failed.");
    }
}

void ResetAndroidMotionIfRequested(SwapchainContext& context)
{
    if ((context.motion && !context.motion->finished) || !gMotionReleaseRequested.exchange(false)) return;
    std::lock_guard<std::mutex> lock(gInputPublisherMutex);
    if (context.motion && context.motion->equipmentSeedActive)
    {
        if(context.motion->equipmentSeedActive)context.motion->scenario.End(gGameSimulation);
        context.motion->equipmentSeedActive = false;
    }
    const auto& state = gGameSimulation.Snapshot();
    auto& commands = gInputPublisherState.commands;
    commands.attack = std::max(commands.attack, state.lastConsumedAttackSequence);
    commands.parry = std::max(commands.parry, state.lastConsumedParrySequence);
    commands.dodge = std::max(commands.dodge, state.lastConsumedDodgeSequence);
    commands.retry = std::max(commands.retry, state.lastConsumedRetrySequence);
    commands.routeReset = std::max(commands.routeReset, state.lastConsumedRouteResetSequence);
    commands.interact = std::max(commands.interact, state.lastConsumedInteractSequence);
    commands.toggleHeldLightPose = std::max(commands.toggleHeldLightPose, state.lastConsumedToggleHeldLightPoseSequence);
    commands.runToggle=std::max(commands.runToggle,state.lastConsumedRunToggleSequence);
    commands.clearRunIntent=std::max(commands.clearRunIntent,state.lastConsumedClearRunIntentSequence);
    gInputPublisherState.runHeld=false;
    gInputPublisherState.combatEdgeHistory = {};
    gInputPublisherState.paused = true;
    gInputPublisherState.moveForward = gInputPublisherState.moveStrafe = 0.0f;
    PublishInputLocked();
    context.motion.reset();
    context.motionValidationRun = false;
    gMotionStatus.store(0, std::memory_order_release);
}

void UpdateAndroidMotionScope(SwapchainContext& context, horde::gameplay::simulation::InputSnapshot& input)
{
    if (!context.motion || !context.motion->armed || context.motion->finished) return;
    auto& run = *context.motion;
    const auto scope = AndroidMotionScope(context);
    if (scope == run.scope) return;
    // The ordinary Retry event retains resources and advances measurement only.
    if (!horde::platform::android::AndroidMotionRetryScopeValid(
            run.scope, scope, run.retryPending, run.ledger.HasPendingSubmissions()) ||
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
        if(validation::IsTimedMotionScenario(run.selected)) {
#if HORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING
            // Resume an already admitted chapter (reunion, or live lower start
            // for the torch probe); this does not fabricate destination Ready.
            context.benchmarkSampling=false;
#else
            FailAndroidMotion(context,"Chapter route requires the explicit Shipping/Mobile timing candidate.");return;
#endif
        }
        run.path = context.reportDirectory + "/android-motion-" + run.id;
        if(validation::IsTimedMotionScenario(run.selected)) {
            if(std::ifstream(run.path+"-resources.csv").good()) {run.finished=true;gMotionStatus.store(4);return;}
            run.chapterCsv.open(run.path+"-resources.csv");
            run.chapterCsv<<"present_return_ns,interval_ns,serial,tick,phase,x,y,z,device_local_bytes,host_visible_bytes,omitted_triangles,unloads,restores,saved_owner_bytes,peak_overlap_bytes,wait_frames,yaw,pitch,view_ticks,detail_step_cpu_ns,run_unloads,run_restores"
                <<",tlas_instances,blas_count"
#if HORDE_RT_FRAME_STAGE_PROBE
                <<",as_gpu_status,as_gpu_serial,as_gpu_ns,dispatch_gpu_status,dispatch_gpu_serial,dispatch_gpu_ns,transfer_gpu_status,transfer_gpu_serial,transfer_gpu_ns"
#endif
#if HORDE_RT_DISPATCH_CLOCK_PROBE
                <<",clock_status,clock_ticket,clock_samples,clock_other,clock_trace,clock_material,clock_shadow,clock_surface,clock_fire,clock_mist,clock_dust"
#endif
                <<'\n';
            if(!run.chapterCsv){FailAndroidMotion(context,"Chapter resource ledger could not open.");return;}
        }
        run.requestedNs = now;
#if HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION || HORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING
        if (context.presentTiming)
        {
            run.timingRowBaseline = context.presentTiming->RowCount();
            run.timingCountersBaseline = context.presentTiming->GetCounters();
        }
#endif
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
    if (run.finished || run.terminalTimingDrainPending) { input.paused = true; input.moveForward = input.moveStrafe = 0.0f; return; }
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
        // Before the scenario's one accepted checkpoint seed, hold the ordinary
        // simulation paused so startup/menu input cannot advance an unmeasured scene.
        if (context.motionValidationRun)
        {
            input.paused = true;
            input.damageEnabled = false;
            input.moveForward = input.moveStrafe = 0.0f;
        }
        if (context.motionValidationRun)
        {
            const auto& consumed = gGameSimulation.Snapshot();
            const bool pendingOwnerCommand =
                input.commands.attack > consumed.lastConsumedAttackSequence ||
                input.commands.parry > consumed.lastConsumedParrySequence ||
                input.commands.dodge > consumed.lastConsumedDodgeSequence ||
                input.commands.retry > consumed.lastConsumedRetrySequence ||
                input.commands.routeReset > consumed.lastConsumedRouteResetSequence ||
                input.commands.interact > consumed.lastConsumedInteractSequence ||
                input.commands.toggleHeldLightPose > consumed.lastConsumedToggleHeldLightPoseSequence;
            if (pendingOwnerCommand || input.moveForward != 0.0f || input.moveStrafe != 0.0f)
            {
                FailAndroidMotion(context, "Motion benchmark started with pending gameplay input or non-neutral movement.");
                input.paused = true;
                return;
            }
        }
        const auto publication = context.rtFrameEvidence.PublishedStateByValue();
        if (now - run.requestedNs > 30'000'000'000ull)
        { FailAndroidMotion(context, "Current foreground RT frame did not arm before deadline."); input.paused = true; return; }
        if (!horde::platform::android::AndroidMotionCanArm(
                context.motionValidationRun, input.paused) ||
            !publication.running || !publication.presented || !publication.hasCompletedEvidence ||
            publication.completedEvidence.presentation.outcome != horde::telemetry::RtPresentationOutcome::Presented ||
            publication.completedEvidence.identity.submitted.frame.sceneEpoch != publication.sceneEpoch ||
            publication.completedEvidence.identity.submitted.frame.measurementGeneration != publication.measurementGeneration ||
            !publication.completedEvidence.scene.dispatch.sceneReady ||
            !publication.completedEvidence.scene.dispatch.rtDispatchRecorded ||
            !publication.completedEvidence.scene.dispatch.swapchainCopyRecorded)
            return;
        const bool scenarioSeeded = run.scenario.Begin(run.selected, gGameSimulation, now);
        if (scenarioSeeded)
            run.equipmentSeedActive = run.scenario.OwnsEquipmentSeed();
        if (!scenarioSeeded ||
            !context.rtFrameEvidence.ApplyEvent(horde::telemetry::RtLifecycleEvent::CheckpointChange))
        { FailAndroidMotion(context, "Motion could not apply its one accepted checkpoint seed."); input.paused = true; return; }
        run.scope = AndroidMotionScope(context);
        if (!horde::platform::android::AndroidMotionScopeValid(run.scope) ||
            !run.ledger.ObserveScope(run.scope.surfaceGeneration, run.scope.sceneEpoch, run.scope.measurementGeneration))
        { FailAndroidMotion(context, "Motion seed has no current evidence scope."); input.paused = true; return; }
        gGameSimulation.ResetTiming(); gGameSimulation.ClearEvents();
        context.lastInputOwnerSteadyNs = now;
        run.chapterBaseline=context.rtScene.OrdinaryStaticDetailEvidence();
        run.armed = true;
        if (context.motionValidationRun) input.paused = false;
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
    const auto previousStage=run.scenario.Stage();
    run.scenario.ObserveAdvance(snapshot, events); // before the established semantic drain
    if(run.selected==horde::gameplay::validation::MotionScenario::KeeperTorchTransition &&
       previousStage!=run.scenario.Stage())
        __android_log_print(ANDROID_LOG_INFO,kTag,"HORDE_TORCH_TRANSITION stage=%s tick=%llu",
            horde::gameplay::validation::MotionStageName(run.scenario.Stage()),
            static_cast<unsigned long long>(snapshot.tickIndex));
    if (!run.ledger.AppendState(now, snapshot, input, run.scenario, events))
        FailAndroidMotion(context, run.ledger.Failure());
    else if (run.scenario.Failed()) FailAndroidMotion(context, run.scenario.Failure());
}

void AfterAndroidMotionPresent(SwapchainContext& context)
{
    if (!context.motion || !context.motion->armed || context.motion->finished) return;
    auto& run = *context.motion;
    if(horde::gameplay::validation::IsTimedMotionScenario(run.selected)) {
        auto& tag=run.chapterTags[context.currentFrame];
        tag.returnNs=GraphicsSteadyNs();
        if(!run.scenario.Complete())return; // No mid-route wait-idle or image readback.
        if(!CompleteRtEvidenceAfterDeviceIdle(context,vkDeviceWaitIdle(context.device)) || run.finished) {
            FailAndroidMotion(context,"Chapter terminal graphics ownership did not drain.");return;
        }
        run.terminalTimingDrainPending=true;run.terminalTimingDrainStartedNs=GraphicsSteadyNs();return;
    }
    const auto stage = run.scenario.Stage();
    const double seconds = run.scenario.SimulationSeconds();
    const auto states = run.ledger.States();
    if (states.empty()) { FailAndroidMotion(context, "Motion present has no recorded simulation state."); return; }
    const auto action = states.back().combat.action;
    const bool actionMilestone = action != run.lastPresentedAction &&
        (action == horde::gameplay::PlayerCombatAction::SwingWindup ||
         action == horde::gameplay::PlayerCombatAction::SwingActive ||
         action == horde::gameplay::PlayerCombatAction::UpwardSliceWindup ||
         action == horde::gameplay::PlayerCombatAction::UpwardSliceActive ||
         action == horde::gameplay::PlayerCombatAction::ParryActive);
    run.lastPresentedAction = action;
    const auto& sword = states.back().sword;
    unsigned drawThresholds = 0u;
    if (sword.active && sword.transition == horde::gameplay::items::HeldItemTransitionKind::Draw)
        for (unsigned threshold = 0u; threshold < 3u; ++threshold)
            if (sword.progress >= 0.25f * static_cast<float>(threshold + 1u))
                drawThresholds |= 1u << threshold;
    const bool equipmentMilestone = (drawThresholds & ~run.capturedDrawThresholds) != 0u;
    const auto& torch = states.back().torch;
    const unsigned torchThresholds = horde::telemetry::ObservedTorchCaptureMilestones(torch);
    const bool torchMilestone = (torchThresholds & ~run.capturedTorchThresholds) != 0u;
    if (!gSurfaceSessions.IsCurrent(context.surfaceGeneration) || AndroidMotionScope(context) != run.scope)
    { FailAndroidMotion(context, "Motion present lost its foreground resource scope."); return; }
    if (stage == run.lastCaptureStage && seconds - run.lastCaptureSeconds < 2.0 &&
        !run.scenario.Complete() && !actionMilestone && !equipmentMilestone && !torchMilestone) return;
    horde::telemetry::RtSubmittedFrameIdentity submitted{};
    if (!context.rtFrameEvidence.TryGetCommittedIdentity(context.currentFrame, submitted) ||
        !CompleteRtEvidenceAfterDeviceIdle(context, vkDeviceWaitIdle(context.device)) || run.finished ||
        !gSurfaceSessions.IsCurrent(context.surfaceGeneration))
    { FailAndroidMotion(context, "Motion milestone did not drain its graphics owner."); return; }
    const auto frames = run.ledger.Frames();
    if (frames.empty() || frames.back().identity.submitted.submissionSerial != submitted.submissionSerial ||
        frames.back().stateRow != states.size() - 1u ||
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
        << ",\"rtRow\":" << frames.size() - 1u << ",\"actionMilestone\":"
        << (actionMilestone ? "true" : "false")
        << ",\"equipmentThresholdMask\":" << (drawThresholds & ~run.capturedDrawThresholds)
        << ",\"swordDrawProgress\":" << sword.progress
        << ",\"torchThresholdMask\":" << (torchThresholds & ~run.capturedTorchThresholds)
        << ",\"torchFallProgress\":" << torch.fallProgress
        << ",\"torchArmLowerBlend\":" << torch.leftArmLowerBlend << '}';
    // A hitch can cross several thresholds in one presented frame. Retain the
    // actual observed progress and one image, never invent skipped poses.
    run.capturedDrawThresholds |= drawThresholds;
    run.capturedTorchThresholds |= torchThresholds;
    run.captures += row.str(); run.lastCaptureStage = stage; run.lastCaptureSeconds = seconds;
    if (run.scenario.Complete())
    {
        run.finished = horde::platform::android::AndroidMotionTerminalOwnerReady(
            run.scenario.Complete(), run.ledger.HasPendingSubmissions(), false);
        if (!run.finished) { FailAndroidMotion(context, "Terminal motion frame still has pending graphics work."); return; }
#if HORDE_RT_ANDROID_MOTION_VALIDATION
        if (context.motionValidationRun)
        {
            run.finished = false;
            run.terminalTimingDrainPending = true;
            run.terminalTimingDrainStartedNs = GraphicsSteadyNs();
        }
#endif
    }
    if (!WriteAndroidMotion(context))
    { run.finished = false; FailAndroidMotion(context, "Motion receipt write failed."); }
    else if (run.finished)
    {
        gMotionStatus.store(3, std::memory_order_release);
#if HORDE_RT_ANDROID_MOTION_VALIDATION
        if (context.motionValidationRun) (void)WriteAndroidMotionValidationReport(context);
#endif
    }
}
