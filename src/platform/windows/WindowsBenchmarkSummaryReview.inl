// Included in the ordinary Windows owner, independently of Debug capture modes.
// The existing schema1 playtest identity helper remains unchanged. This UUIDv4
// helper is shared only by the new local benchmark run and report identities.
bool MakeWindowsBenchmarkUuid(std::string& id);

bool WindowsBenchmarkSummaryInteractive(const VulkanSurfaceContext& context) noexcept
{
    return !context.unattendedBenchmark && !context.graphicsPreviewCapture &&
        !context.outputResizeValidation && !context.nativeMotionValidation;
}

horde::telemetry::BenchmarkSummaryConfiguration WindowsBenchmarkSummaryConfiguration(
    const VulkanSurfaceContext& context, const horde::vulkan::DeviceCapabilities& capabilities)
{
    horde::telemetry::BenchmarkSummaryConfiguration configuration;
    configuration.platform = horde::telemetry::BenchmarkSummaryPlatform::Windows;
    configuration.metadata = BuildBenchmarkMetadata(context, capabilities);
    const auto applied = CurrentGraphicsSettings(context);
    configuration.metadata.renderScalePercent = static_cast<std::uint32_t>(applied.renderScalePercent);
    configuration.water = static_cast<horde::telemetry::RtWaterQuality>(applied.waterQuality);
    if (!context.rtScene.IsReady() || !context.rtScene.HasUploadedQualityControls()) return {};
    switch (context.rtScene.UploadedFireQuality())
    {
    case horde::vulkan::raytracing::FireEmitterQuality::Mobile:
        configuration.fire = horde::telemetry::BenchmarkSummaryFireQuality::Mobile; break;
    case horde::vulkan::raytracing::FireEmitterQuality::High:
        configuration.fire = horde::telemetry::BenchmarkSummaryFireQuality::High; break;
    case horde::vulkan::raytracing::FireEmitterQuality::Low:
        configuration.fire = horde::telemetry::BenchmarkSummaryFireQuality::Low; break;
    default: return {};
    }
    const auto fireBudget = horde::vulkan::raytracing::ResolveFireEmitterQualityBudget(context.rtScene.UploadedFireQuality());
    configuration.uploadedFireQuality = horde::telemetry::RtFireQualityEvidence{
        static_cast<horde::telemetry::RtFireQuality>(configuration.fire), fireBudget.volumeSteps, fireBudget.reflectionSamples};
    const auto& quality = context.rtScene.QualityControls().controls;
    configuration.shadowQuality = horde::telemetry::RtShadowQualityEvidence{
        static_cast<horde::telemetry::RtShadowMode>(quality[0]), quality[1], quality[2], 0u};
    configuration.actualUploadedMistEnabled = context.rtScene.UploadedMistEnabled();
    if (!configuration.actualUploadedMistEnabled) return {};
    configuration.actualUploadedDustQuality = context.rtScene.UploadedDustQuality();
    if (!configuration.actualUploadedDustQuality) return {};
    const auto optical = context.rtScene.SelectedDielectricQualityName();
    if (optical != "High" && optical != "Mobile") return {};
    configuration.dielectric = optical == "High" ?
        horde::telemetry::RtDielectricQuality::High : horde::telemetry::RtDielectricQuality::Mobile;
    configuration.glassEnabled = context.rtScene.GlassEnabled();
    const auto state = context.rtFrameEvidence.PublishedStateByValue();
    configuration.sceneEpoch = state.sceneEpoch;
    configuration.measurementGeneration = state.measurementGeneration;
    return configuration;
}

void ArmWindowsBenchmarkSummary(VulkanSurfaceContext& context) noexcept
{
    if (!WindowsBenchmarkSummaryInteractive(context)) return;
    context.benchmarkSummaryArm.reset();
    // One bounded OPTIONAL metadata copy occurs on the interactive arm frame.
    // Its cost remains inside the existing timing scope; no per-frame metadata
    // copies or additional unattended observer work are introduced.
    try
    {
        if (!context.capabilitySnapshot || !context.rtScene.IsReady() || !context.rtScene.HasUploadedQualityControls() ||
            context.rtScene.Profile() != horde::vulkan::raytracing::RtSceneProfile::Showcase ||
            context.graphicsCommand || context.renderScaleDirty || context.sceneProfileDirty || context.glassGeometryDirty ||
            context.benchmarkEvidence.Status() != horde::telemetry::RtBenchmarkRunStatus::Measuring) return;
        WindowsBenchmarkSummaryArm arm;
        arm.configuration = WindowsBenchmarkSummaryConfiguration(context, *context.capabilitySnapshot);
        arm.applied = CurrentGraphicsSettings(context); arm.profile = context.rtScene.Profile();
        if (arm.configuration.sceneEpoch != context.benchmarkEvidence.SceneEpoch() ||
            arm.configuration.measurementGeneration != context.benchmarkEvidence.MeasurementGeneration()) return;
        context.benchmarkSummaryArm.emplace(std::move(arm));
    }
    catch (...) { context.benchmarkSummaryArm.reset(); } // Optional allocation is never a benchmark failure.
}

void FreezeWindowsBenchmarkSummary(VulkanSurfaceContext& context,
                                  const horde::vulkan::DeviceCapabilities& capabilities) noexcept
{
    context.benchmarkSummary.reset();
    if (!WindowsBenchmarkSummaryInteractive(context) || !context.benchmarkSummaryArm) return;
    try
    {
        const auto& arm = *context.benchmarkSummaryArm;
        if (!context.benchmark.Passed() || context.benchmarkEvidence.Status() != horde::telemetry::RtBenchmarkRunStatus::Complete ||
            !context.rtScene.IsReady() || context.rtScene.Profile() != arm.profile ||
            !(CurrentGraphicsSettings(context) == arm.applied) || context.graphicsCommand ||
            context.renderScaleDirty || context.sceneProfileDirty || context.glassGeometryDirty) return;
        const auto completion = WindowsBenchmarkSummaryConfiguration(context, capabilities);
        std::string runUuid;
        if (!MakeWindowsBenchmarkUuid(runUuid)) return;
        auto frozen = horde::telemetry::CaptureBenchmarkSummary(context.benchmark, context.benchmarkEvidence,
            arm.configuration, completion, runUuid, "Unknown", horde::telemetry::BenchmarkSummaryCooling::Unknown);
        if (frozen.IsReady()) context.benchmarkSummary.emplace(std::move(frozen));
    }
    catch (...) { context.benchmarkSummary.reset(); } // Legacy reports and outcome remain authoritative.
}

#include "platform/windows/WindowsBenchmarkSummaryReviewUi.inl"
