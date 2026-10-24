#pragma once
#include "vulkan/GpuFrameTimer.h"
#include "telemetry/RtPerformanceEvidence.h"
#include <array>
namespace horde::vulkan::raytracing {
// Completed AS/dispatch/transfer collections in the caller's existing order.
// The saved identity comes from the successful submission, not from whichever
// frame happens to be current when its owning fence is collected.
struct RtSceneGpuStageCompletedObservation {
    horde::telemetry::RtSubmittedFrameIdentity submitted{};
    std::array<GpuFrameTimingCollection,3> collections{};
};

// Pure attribution join only. The caller must already have completed the real
// owning fence and accepted the completed evidence snapshot. This neither reads
// queries nor establishes fence completion, presentation or timestamp support.
[[nodiscard]] inline bool RtSceneGpuStagesMatchCompletedFrame(
    const RtSceneGpuStageCompletedObservation& observation,
    const horde::telemetry::RtSubmittedFrameIdentity& completedOwner) noexcept
{
    const auto& saved=observation.submitted;const auto& a=saved.frame;const auto& b=completedOwner.frame;
    if(!a.sceneEpoch || !a.measurementGeneration || !a.recordAttemptSerial || !a.recordSerial || !saved.submissionSerial ||
       a.sceneEpoch!=b.sceneEpoch || a.measurementGeneration!=b.measurementGeneration ||
       a.recordAttemptSerial!=b.recordAttemptSerial || a.recordSerial!=b.recordSerial ||
       a.simulationTick!=b.simulationTick || a.frameSlot!=b.frameSlot ||
       saved.submissionSerial!=completedOwner.submissionSerial)return false;
    for(const auto& collection:observation.collections)
        if(!collection.consumed || collection.status!=GpuFrameTimingCollectionStatus::Valid ||
           !collection.hasSample || collection.result!=VK_SUCCESS ||
           collection.frameSlot!=a.frameSlot || collection.submissionSequence!=saved.submissionSerial ||
           collection.sample.frameSlot!=a.frameSlot || collection.sample.submissionSequence!=saved.submissionSerial)return false;
    return true;
}

// Optional observation only; the caller retains query/fence/submission ownership.
// Reuses the September30 AS/dispatch probe. Transfer starts after the existing
// storage-image shader-to-transfer barrier. Intervals are not additive counters.
struct RtSceneGpuStageProbe {
    std::array<GpuFrameTimer*,3> timers{};
    std::uint32_t frameSlot=0;
    std::array<bool,3> recorded{};
    RtSceneGpuStageProbe(std::array<GpuFrameTimer*,3> owners,std::uint32_t slot):timers(owners),frameSlot(slot){}
    RtSceneGpuStageProbe(const RtSceneGpuStageProbe&)=delete;
    RtSceneGpuStageProbe& operator=(const RtSceneGpuStageProbe&)=delete;
    ~RtSceneGpuStageProbe() { for(auto* timer:timers)if(timer)timer->CancelRecording(frameSlot); }

    bool Begin(VkCommandBuffer command,unsigned index,VkPipelineStageFlagBits stage) {
        return index<timers.size() && timers[index] && timers[index]->RecordBegin(command,frameSlot,stage);
    }
    void End(VkCommandBuffer command,unsigned index,VkPipelineStageFlagBits stage) {
        if(index<timers.size() && timers[index])recorded[index]=timers[index]->RecordEnd(command,frameSlot,stage);
    }
};
}
