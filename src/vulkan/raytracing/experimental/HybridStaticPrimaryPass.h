#pragma once
#include "vulkan/raytracing/RtPipelineBundle.h"
#include "vulkan/raytracing/RtStaticMeshSlot.h"
#include "vulkan/raytracing/RtExecutionPolicy.h"
#include "vulkan/raytracing/experimental/HybridStaticPrimaryControl.h"
#include <memory>
#include <vector>
namespace horde::vulkan::raytracing::experimental {
// Opt-in partial hybrid. Only immutable opaque StaticPbr instance21 is raster
// primary; original TLAS/materials/shading own every secondary ray. A separate
// remaining-scene TLAS preserves all other primary surfaces and gameplay.
class HybridStaticPrimaryPass final {
public:
    ~HybridStaticPrimaryPass();
    static std::unique_ptr<HybridStaticPrimaryPass> Create(VkInstance,VkPhysicalDevice,VkDevice,
        const RtGpuResources&,VkDescriptorSetLayout,VkDescriptorSet,VkExtent2D,
        const RtExecutionPolicy&,const RtPipelineBundleBuildApi&,const RtStaticMeshSlot&,
        const RtGpuBuffer& vertices,const RtGpuBuffer& indices,std::string&);
    bool PrepareFrame(std::span<const VkAccelerationStructureInstanceKHR>,
        const RtGpuBuffer&,const RtGpuBuffer&,std::string&,RtSceneRecordObservation* = nullptr);
    void Record(VkCommandBuffer,RtMaterialStrategy,std::span<const std::byte> controls,float eyeOffset) const;
    bool PrepareResizeAfterDeviceIdle(VkExtent2D,std::string&);
    void CommitResizeAfterDeviceIdle();
    void CancelResizeAfterDeviceIdle();
    void AccumulateResourceInventory(horde::telemetry::RtResourceInventory&) const;
    uint32_t RecordedDrawCount() const noexcept { return drawCount_; }
    uint64_t RecordedTriangleCount() const noexcept { uint64_t n=0;for(uint32_t i=0;i<drawCount_;++i)n+=counts_[i]/3;return n; }
    std::string_view Key(RtMaterialStrategy) const noexcept;
    std::string_view Sha(RtMaterialStrategy) const noexcept;
private:
    struct alignas(16) Draw {std::array<float,16> transform{};std::array<uint32_t,4> ids{};};
    struct Targets {VkImage visibility{},depth{};VkDeviceMemory visMemory{},depthMemory{};VkImageView visView{},depthView{};VkFramebuffer framebuffer{};VkExtent2D extent{};VkDeviceSize visBytes{},depthBytes{};};
    bool Initialise(VkInstance,VkPhysicalDevice,VkDescriptorSetLayout,VkExtent2D,const RtPipelineBundleBuildApi&,const RtStaticMeshSlot&,std::string&);
    bool MakeTargets(VkExtent2D,Targets&,std::string&);
    void DestroyTargets(Targets&);
    void WriteDescriptors();
    bool Fail(std::string& out,const char* what) const {out=what;return false;}
    VkDevice device_{};VkPhysicalDevice physical_{};RtGpuResources resources_{};RtExecutionPolicy policy_{};
    VkDescriptorSet sceneSet_{};VkDescriptorSetLayout extraLayout_{};VkDescriptorPool pool_{};VkDescriptorSet extraSet_{};
    VkPipelineLayout layout_{};VkRenderPass renderPass_{};VkPipeline raster_{};
    std::array<RtStrategyPipelineResources,2> shade_{};
    Targets targets_,replacement_;
    RtGpuBuffer vertices_,indices_; // borrowed, never destroyed
    RtGpuBuffer draws_,instances_,scratch_;RtAccelerationStructure remaining_;
    std::vector<Draw> drawRecords_;std::vector<uint32_t> counts_;std::vector<VkAccelerationStructureInstanceKHR> frameInstances_;
    std::string unsupportedReason_;
    uint32_t drawCount_=0,instanceCount_=0,maxInstances_=0;VkDeviceSize scratchAddress_=0;
    PFN_vkCmdBuildAccelerationStructuresKHR build_{};PFN_vkCmdTraceRaysKHR trace_{};
};
}
