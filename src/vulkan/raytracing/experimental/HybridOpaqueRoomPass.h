#pragma once
#include "vulkan/raytracing/RtPipelineBundle.h"
#include "vulkan/raytracing/RtStaticMeshSlot.h"
#include "vulkan/raytracing/RtExecutionPolicy.h"
#include <memory>
#include <vector>
namespace horde::vulkan::raytracing::experimental {
// Static architectural comparison. Both modes use exactly the same opaque world
// and asset meshes, direct lights and hardware-query shadows. Forward mode has
// no camera ray queries, visibility buffer or full-screen shading resolve.
class HybridOpaqueRoomPass final {
public:
    ~HybridOpaqueRoomPass();
    static std::unique_ptr<HybridOpaqueRoomPass> Create(VkInstance,VkPhysicalDevice,VkDevice,
        const RtGpuResources&,VkDescriptorSetLayout,VkDescriptorSet,VkExtent2D,
        const RtExecutionPolicy&,const RtPipelineBundleBuildApi&,const RtStaticMeshSlot&,
        const RtGpuBuffer&,const RtGpuBuffer&,const RtGpuBuffer& worldVertices,
        std::span<const uint32_t> worldIndices,std::span<const uint32_t> worldCodes,std::string&);
    bool PrepareFrame(std::span<const VkAccelerationStructureInstanceKHR>,
        const RtGpuBuffer&,const RtGpuBuffer&,std::string&,RtSceneRecordObservation* = nullptr);
    void Record(VkCommandBuffer,RtMaterialStrategy,std::span<const std::byte>,float,VkImage);
    bool ScenePrimed() const noexcept { return built_; }
    void NotifyFrameSubmitted() noexcept { if(recordedBuild_)built_=true;recordedBuild_=false; }
    bool PrepareResizeAfterDeviceIdle(VkExtent2D,std::string&);
    void CommitResizeAfterDeviceIdle();
    void CancelResizeAfterDeviceIdle();
    void AccumulateResourceInventory(horde::telemetry::RtResourceInventory&) const;
    uint32_t RecordedDrawCount() const noexcept { return rasterSelected_?static_cast<uint32_t>(drawsCpu_.size()):0; }
    uint64_t RecordedTriangleCount() const noexcept { uint64_t n=0;if(rasterSelected_)for(auto c:counts_)n+=c/3;return n; }
    std::string_view Key(RtMaterialStrategy) const noexcept;
    std::string_view Sha(RtMaterialStrategy) const noexcept;
private:
    struct alignas(16) Draw {std::array<float,16> transform{};std::array<uint32_t,4> ids{};};
    struct Targets {VkImage color{},depth{};VkDeviceMemory colorMemory{},depthMemory{};VkImageView colorView{},depthView{};
        VkFramebuffer framebuffer{};VkExtent2D extent{};VkDeviceSize colorBytes{},depthBytes{};};
    bool Initialise(VkInstance,VkDescriptorSetLayout,VkExtent2D,const RtPipelineBundleBuildApi&,
        const RtStaticMeshSlot&,std::span<const uint32_t>,std::span<const uint32_t>,std::string&);
    bool MakeTargets(VkExtent2D,Targets&,std::string&);
    void DestroyTargets(Targets&);
    bool MakeAs(VkAccelerationStructureTypeKHR,const VkAccelerationStructureGeometryKHR&,uint32_t,
        RtAccelerationStructure&,RtGpuBuffer&,std::string&);
    bool Fail(std::string& out,const char* what) const {out=what;return false;}
    VkDevice device_{};VkPhysicalDevice physical_{};RtGpuResources resources_{};RtExecutionPolicy policy_{};
    VkDescriptorSet sceneSet_{},extraSet_{};VkDescriptorSetLayout extraLayout_{};VkDescriptorPool pool_{};
    VkPipelineLayout layout_{};VkRenderPass renderPass_{};VkPipeline raster_{};
    RtStrategyPipelineResources reference_{};Targets targets_,replacement_;
    RtGpuBuffer vertices_,indices_,worldVertices_; // borrowed
    RtGpuBuffer drawBuffer_,worldIndices_,worldMap_,instances_,worldScratch_,tlasScratch_;
    RtAccelerationStructure worldBlas_,roomTlas_;
    std::vector<Draw> drawsCpu_;std::vector<uint32_t> counts_,indicesCpu_,mapCpu_;
    VkDeviceSize scratchAlignment_=1;
    bool built_=false,recordedBuild_=false,rasterSelected_=false;
    uint32_t instanceCount_=0,excludedTriangles_=0;
    PFN_vkGetAccelerationStructureBuildSizesKHR sizes_{};
    PFN_vkCreateAccelerationStructureKHR createAs_{};
    PFN_vkGetAccelerationStructureDeviceAddressKHR asAddress_{};
    PFN_vkCmdBuildAccelerationStructuresKHR build_{};
    PFN_vkCmdTraceRaysKHR trace_{};
};
using HybridStaticPrimaryPass = HybridOpaqueRoomPass;
}
