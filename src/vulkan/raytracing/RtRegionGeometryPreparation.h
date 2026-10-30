#pragma once
#include "vulkan/raytracing/RtGpuResources.h"
#include "vulkan/raytracing/RtGpuWorkLease.h"
#include "world/streaming/GeometryPartition.h"
#include <memory>
#include <vector>

namespace horde::vulkan::raytracing {
struct RtRegionGeometryBuildContext {
    RtGpuWorkApi work;
    PFN_vkGetAccelerationStructureBuildSizesKHR sizes=nullptr;
    PFN_vkCreateAccelerationStructureKHR create=nullptr;
    PFN_vkGetAccelerationStructureDeviceAddressKHR address=nullptr;
    PFN_vkCmdBuildAccelerationStructuresKHR build=nullptr;
    VkDeviceSize scratchAlignment=0;
    std::uint64_t maxPrimitiveCount=0,maxGeometryCount=0;
    // Exact admitted backend stages; compute-only devices must not receive a
    // ray-tracing-pipeline stage bit. The owning device supplies this contract.
    VkPipelineStageFlags consumerStages=0;
};
struct RtRegionGeometryOwner {
    RtRegionGeometryOwner()=default;
    RtRegionGeometryOwner(const RtRegionGeometryOwner&)=delete;
    RtRegionGeometryOwner& operator=(const RtRegionGeometryOwner&)=delete;
    RtRegionGeometryOwner(RtRegionGeometryOwner&&) noexcept;
    RtRegionGeometryOwner& operator=(RtRegionGeometryOwner&&)=delete;
    horde::world::streaming::Token token;
    // Immutable CPU cache is separate from actual GPU allocation accounting.
    std::shared_ptr<const horde::world::streaming::GeometryPayload> payload;
    std::array<RtGpuBuffer,3> buffers; // vertices, indices, private primitive rows
    RtAccelerationStructure blas;
    bool Empty() const noexcept;
    bool TransferTo(RtRegionGeometryOwner&) noexcept;
    bool ResetEmpty() noexcept; // All GPU handles must already be retired/destroyed.
    void AccumulateResourceInventory(horde::telemetry::RtResourceInventory&) const noexcept;
};

// Offside immutable page preparation. No descriptors/TLAS/live scene are changed.
// The renderer must keep this owner alive until Take, completed cancellation or
// external successful-device-idle cleanup. Quarantined work is never freed by
// polling, generation invalidation or object destruction. The VkDevice/context
// outlives this object. As with scene-owned Vulkan handles, explicit cleanup is
// mandatory; destroying the wrapper is not a GPU retirement proof.
class RtRegionGeometryPreparation {
public:
    enum class Phase { Idle, Upload, Build, BuildWait, Ready, CancelPending, Quarantined, Failed };
    RtRegionGeometryPreparation(const RtGpuResources&,RtRegionGeometryBuildContext);
    ~RtRegionGeometryPreparation();
    RtRegionGeometryPreparation(const RtRegionGeometryPreparation&)=delete;
    RtRegionGeometryPreparation& operator=(const RtRegionGeometryPreparation&)=delete;
    bool Begin(horde::world::streaming::Token,
        std::shared_ptr<const horde::world::streaming::GeometryPayload>,
        std::span<const RtMaterialGpu> immutableSharedMaterials,std::string&);
    // At most one upload row/copy budget or one AS submission/poll per call.
    // Allocation/driver latency is measured separately, not time-bounded.
    bool Advance(horde::world::streaming::Token,VkDeviceSize copyBytes,std::string&);
    bool RequestCancel(horde::world::streaming::Token,std::string&);
    bool Take(horde::world::streaming::Token,RtRegionGeometryOwner&,std::string&);
    bool ResetFailed(horde::world::streaming::Token) noexcept;
    // Call only after platform proves successful VkDeviceWaitIdle, before device
    // teardown. This also owns every quarantined scratch/fence/source reference.
    void DestroyAfterDeviceIdle() noexcept;
    Phase Status() const noexcept {return phase_;}
    void AccumulateResourceInventory(horde::telemetry::RtResourceInventory&) const noexcept;
    VkDeviceSize LastCopiedBytes() const noexcept {return lastCopiedBytes_;}
private:
    bool Build(std::string&);
    void DestroyUnsubmitted() noexcept;
    RtGpuResources resources_;
    RtRegionGeometryBuildContext context_;
    RtGpuBufferPreparation uploads_;
    RtRegionGeometryOwner owner_;
    RtGpuBuffer scratch_;
    RtGpuWorkLease work_;
    std::vector<VkAccelerationStructureGeometryKHR> geometries_;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges_;
    VkAccelerationStructureBuildGeometryInfoKHR buildInfo_{};
    Phase phase_=Phase::Idle;
    VkDeviceSize lastCopiedBytes_=0;
    horde::world::streaming::Token failedToken_;
};
}
