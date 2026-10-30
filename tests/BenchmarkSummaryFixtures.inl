// Synthetic completed-owner fixture; include inside the test namespace after Check.
constexpr std::string_view kRunUuid = "11111111-1111-4111-8111-111111111111";
constexpr std::string_view kReportUuid = "22222222-2222-4222-9222-222222222222";

BenchmarkSummaryConfiguration Configuration()
{
    BenchmarkSummaryConfiguration c;
    c.sceneEpoch = 3u; c.measurementGeneration = 8u;
    c.metadata.buildIdentity = "1.6.2-local";
    c.metadata.shaderIdentity = "opaqueFast:shipping_mobile_opaque@" + std::string(64u, 'a') +
        "|genericDielectric:shipping_mobile_generic@" + std::string(64u, 'b');
    c.metadata.gpuName = "Test GPU"; c.metadata.vulkanApi = "1.3.0";
    c.metadata.executionBackend = "RayTracingPipeline";
    c.metadata.presentMode = "FIFO"; c.metadata.materialEncoding = "RGBA8 raw fallback";
    c.metadata.legacyFrameTimingScope = "windows-render-plus-rtlab-telemetry";
    c.metadata.internalWidth = 960u; c.metadata.internalHeight = 540u;
    c.metadata.presentationWidth = 960u; c.metadata.presentationHeight = 540u;
    // Deliberately private/unbounded-looking unused legacy fields MUST NOT escape.
    c.metadata.runId = "private-account-run";
    c.metadata.timestampUtc = "private-original-file-label";
    c.metadata.rtMode = "private-full-diagnostic-report";
    return c;
}

RtPerformanceEvidenceSnapshot Snapshot(const std::uint64_t serial, const RtSampleStatus gpu)
{
    RtPerformanceEvidenceSnapshot s;
    s.identity.submitted.frame = {3u, 8u, serial, serial, 1000u + serial, 0u};
    s.identity.submitted.submissionSerial = serial; s.identity.completionSerial = serial;
    auto& p = s.scene.pipeline;
    p.executionMode = RtExecutionMode::RayTracingPipeline;
    p.instrumentation = RtInstrumentationMode::Shipping;
    p.dielectricQuality = RtDielectricQuality::Mobile;
    p.activeStrategy = RtMaterialStrategy::OpaqueFast; p.waterQuality = RtWaterQuality::Mobile;
    Check(AssignRtFixedText(p.bundleKey, "shipping_mobile_pair"), "fixture bundle key");
    Check(AssignRtFixedText(p.opaqueFast.key, "shipping_mobile_opaque"), "fixture opaque key");
    Check(AssignRtFixedText(p.opaqueFast.sha256, std::string(64u, 'a')), "fixture opaque hash");
    Check(AssignRtFixedText(p.genericDielectric.key, "shipping_mobile_generic"), "fixture generic key");
    Check(AssignRtFixedText(p.genericDielectric.sha256, std::string(64u, 'b')), "fixture generic hash");
    p.active = p.opaqueFast;
    s.scene.stages.status = RtSampleStatus::Valid;
    for (auto& stage : s.scene.stages.values) { stage.durationNanoseconds = 1000u; stage.operationCount = 1u; }
    auto& skin = s.scene.stages.values[RtStageIndex(RtStage::Skin)];
    skin.durationNanoseconds = 2000u; skin.operationCount = 2u;
    s.scene.stages.values[RtStageIndex(RtStage::WholeFrameCycle)].durationNanoseconds = 10'000'000u + serial * 1000u;
    s.scene.dispatch.sceneReady = s.scene.dispatch.rtDispatchRecorded = s.scene.dispatch.swapchainCopyRecorded = true;
    s.dielectric.status = RtSampleStatus::CompiledOut;
    s.gpu.status = gpu;
    if (gpu == RtSampleStatus::Valid)
    {
        s.gpu.hasDuration = true; s.gpu.durationNanoseconds = 5'000'000u + serial * 2000u;
        s.gpu.completedSubmissionSerial = serial; s.gpu.timestampValidBits = 64u;
        s.gpu.timestampPeriodPicoseconds = 1000u; s.gpu.sampleCount = 1u;
    }
    s.presentation.outcome = RtPresentationOutcome::Presented;
    s.presentation.lastSuccessfulPresentSubmissionSerial = serial;
    s.cpuBenchmarkEligible = true;
    s.benchmarkEligible = gpu == RtSampleStatus::Valid || gpu == RtSampleStatus::Disabled || gpu == RtSampleStatus::Unsupported;
    return s;
}

struct CompletedOwners
{
    std::optional<RtFrameDimensionsEvidence> submittedDimensions{};
    unsigned dimensionsDrift = 0;
    ShowcaseBenchmarkRun benchmark;
    RtBenchmarkEvidenceRun evidence;
    void Run(const RtSampleStatus gpu = RtSampleStatus::Valid, const bool wrongZone = false,
        const bool leaveLastPending = false, const bool mixedUnsupported = false,
        const BenchmarkSummaryConfiguration* uploaded = nullptr, const bool middleQualityChange = false,
        const bool middleShadowChange = false, const bool middleMistChange = false,
        const bool middleMistUnavailable = false)
    {
        benchmark.Start(2u, BenchmarkWorkload::LanternHeldHigh);
        Check(evidence.Start(kLanternBenchmarkFramesPerLap), "allocate real bounded evidence owner");
        bool armed = false;
        std::uint64_t serial = 0u;
        while (benchmark.IsRunning())
        {
            const auto advance = benchmark.Advance();
            if (benchmark.CurrentLap() == 2u)
            {
                if (!armed) { Check(evidence.ArmMeasurement(3u, 8u), "arm measurement after warmup"); armed = true; }
                ++serial;
                auto s = Snapshot(serial, mixedUnsupported && serial % 2u == 0u ? RtSampleStatus::Unsupported : gpu);
                s.scene.dimensions = submittedDimensions;
                if (serial == kLanternBenchmarkFramesPerLap / 2u) {
                    if (dimensionsDrift == 1 && s.scene.dimensions) {
                        --s.scene.dimensions->internalWidth;
                        s.scene.dimensions->linearBlit = true;
                        s.scene.dimensions->cameraAspect = float(s.scene.dimensions->internalWidth) / s.scene.dimensions->internalHeight;
                    }
                    if (dimensionsDrift == 2 && s.scene.dimensions) {
                        ++s.scene.dimensions->outputWidth; s.scene.dimensions->linearBlit = true;
                    }
                    if (dimensionsDrift == 3 && s.scene.dimensions) {
                        s.scene.dimensions->presentationTransform = 1;
                        s.scene.dimensions->cameraAspect = 1.0f / s.scene.dimensions->cameraAspect;
                    }
                    if (dimensionsDrift == 4 && s.scene.dimensions) s.scene.dimensions->dynamicResolutionEnabled = true;
                    if (dimensionsDrift == 5) s.scene.dimensions.reset();
                    if (dimensionsDrift == 6) s.scene.dimensions = RtFrameDimensionsEvidence{960,540,960,540,0,960.0f/540,false,false};
                }
                if (uploaded)
                {
                    s.scene.shadowQuality = uploaded->shadowQuality;
                    s.scene.fireQuality = uploaded->uploadedFireQuality;
                    s.scene.actualUploadedMistEnabled = uploaded->actualUploadedMistEnabled;
                    s.scene.actualUploadedDustQuality = uploaded->actualUploadedDustQuality;
                    if (middleMistChange && serial == kLanternBenchmarkFramesPerLap / 2u)
                        s.scene.actualUploadedMistEnabled = !uploaded->actualUploadedMistEnabled.value_or(true);
                    if (middleMistUnavailable && serial == kLanternBenchmarkFramesPerLap / 2u)
                        s.scene.actualUploadedMistEnabled.reset();
                    if (middleQualityChange && serial == kLanternBenchmarkFramesPerLap / 2u)
                        s.scene.fireQuality = RtFireQualityEvidence{RtFireQuality::Mobile, 4u, 1u};
                    if (middleShadowChange && serial == kLanternBenchmarkFramesPerLap / 2u)
                        s.scene.shadowQuality = RtShadowQualityEvidence{RtShadowMode::Current, 1u, 1u, 0u};
                }
                const auto zone = wrongZone ? ShowcaseZone::Opening : advance.replay.zone;
                const auto index = evidence.ExpectFrame({static_cast<std::uint32_t>(zone), 2u});
                Check(index.has_value(), "expected frame owned");
                if (index)
                {
                    Check(evidence.BindSubmitted(*index, s.identity.submitted), "bind exact submitted fixture identity");
                    if (!leaveLastPending || serial != kLanternBenchmarkFramesPerLap)
                        Check(evidence.Complete(s), "canonical fixture completes on real ledger seam");
                }
            }
            benchmark.RecordFrame(16.0 + static_cast<double>(serial % 7u), true);
        }
        if (!leaveLastPending)
        {
            Check(evidence.RecordOwnerDrainResult(true), "actual owner drain result fixture");
            Check(evidence.Finalize(), "full ledger finalization fixture");
        }
    Check(benchmark.Passed(), "real deterministic benchmark course completed");
    }
};

BenchmarkSummaryReportApproval Approval(const bool hardware = false)
{
    return {true, hardware, kReportUuid, "2026-10-03T12:34:56Z"};
}
