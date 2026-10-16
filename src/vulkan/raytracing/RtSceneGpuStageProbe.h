#pragma once
#include "vulkan/GpuFrameTimer.h"
#include <array>
namespace horde::vulkan::raytracing {
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
