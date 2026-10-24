// Optional capture observation; query and graphics ownership remain on this thread.
#if HORDE_RT_FRAME_STAGE_PROBE
void ResetWindowsGpuStagesAfterIdle(VulkanSurfaceContext& ctx)
{
    for(auto& timer:ctx.frameStageTimers)timer.ResetAfterDeviceIdle();
    ctx.pendingStageOwners={};ctx.completedStageObservation={};
}

void CollectWindowsGpuStages(VulkanSurfaceContext& ctx,std::uint32_t slot,
    const horde::vulkan::raytracing::RtFrameEvidenceCompletionResult& completion,
    const horde::telemetry::RtPerformanceEvidenceSnapshot& snapshot)
{
    // Initial signalled fences own nothing. Even a discarded evidence identity
    // must consume this graphics submission's query slots before their reuse.
    if(!completion.ownedGraphicsSubmission || slot>=ctx.pendingStageOwners.size())return;
    auto observed=ctx.pendingStageOwners[slot];ctx.pendingStageOwners[slot]={};
    for(unsigned i=0;i<ctx.frameStageTimers.size();++i)
        if(ctx.frameStageTimers[i].Supported())
            observed.collections[i]=ctx.frameStageTimers[i].CollectCompleted(slot);
    ctx.completedStageObservation={};
    if(completion.completedEvidence)ctx.completedStageObservation=observed;
    // The CSV join checks the complete identity and all three outcomes. Optional
    // unsupported/unavailable/error timing never stops an otherwise valid frame.
    (void)snapshot;
}
#endif

void WriteWindowsGpuStageCsv(std::ostream& rows,const VulkanSurfaceContext& ctx,
    const horde::telemetry::RtSubmittedFrameIdentity& completedOwner)
{
#if HORDE_RT_FRAME_STAGE_PROBE
    const auto& observed=ctx.completedStageObservation;
    const bool matched=ctx.captureGpuStages &&
        horde::vulkan::raytracing::RtSceneGpuStagesMatchCompletedFrame(observed,completedOwner);
    const auto& owner=observed.submitted;
    rows<<','<<matched<<','<<owner.frame.sceneEpoch<<','<<owner.frame.measurementGeneration
        <<','<<owner.frame.recordAttemptSerial<<','<<owner.frame.recordSerial<<','<<owner.frame.simulationTick
        <<','<<owner.submissionSerial<<','<<owner.frame.frameSlot;
    for(const auto& stage:observed.collections) {
        rows<<','<<static_cast<unsigned>(stage.status)<<','<<stage.submissionSequence<<',';
        if(matched)rows<<static_cast<std::uint64_t>(std::llround(stage.sample.milliseconds*1e6));
    }
#else
    (void)ctx;(void)completedOwner;
    rows<<",0,0,0,0,0,0,0,0,0,0,,0,0,,0,0,";
#endif
}
