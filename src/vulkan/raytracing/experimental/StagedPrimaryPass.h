#pragma once

#include "vulkan/raytracing/experimental/StagedPrimaryContract.h"
#include "vulkan/raytracing/RtPipelineBundle.h"

#include <memory>
#include <span>

namespace horde::vulkan::raytracing::experimental {
class StagedPrimaryTiming;

// Investigation-only owner, built only with an explicitly selected shader dir.
// The platform must make the device idle before destroying/replacing resources.
// No primary-instance/material/physical-transport authority lives here.
class StagedPrimaryPass final {
public:
    ~StagedPrimaryPass();
    StagedPrimaryPass(const StagedPrimaryPass&) = delete;
    StagedPrimaryPass& operator=(const StagedPrimaryPass&) = delete;

    static std::unique_ptr<StagedPrimaryPass> Create(
        VkPhysicalDevice physicalDevice, VkDevice device, const RtGpuResources& resources,
        VkDescriptorSetLayout sceneLayout, VkDescriptorSet sceneSet, VkExtent2D extent,
        std::uint32_t pushConstantBytes, PFN_vkCmdTraceRaysKHR trace,
        const RtPipelineBundleBuildApi& pipelineApi, std::string& diagnostic);

    void Record(VkCommandBuffer command, RtMaterialStrategy strategy,
                std::span<const std::byte> pushConstants,
                StagedPrimaryTiming* timing = nullptr, std::uint32_t frameSlot = 0u) const noexcept;
    bool PrepareResizeAfterDeviceIdle(VkExtent2D extent, std::string& diagnostic);
    void CommitResizeAfterDeviceIdle() noexcept;
    void CancelResizeAfterDeviceIdle() noexcept;
    void AccumulateResourceInventory(horde::telemetry::RtResourceInventory& inventory) const noexcept;
    const StagedPrimaryExtent& ExtentContract() const noexcept { return extentContract_; }
    std::uint64_t IntermediateAllocationBytes() const noexcept;
    std::string MetadataJson() const;
    static std::string_view PairKey(RtMaterialStrategy strategy) noexcept;
    static std::string_view PairSha256(RtMaterialStrategy strategy) noexcept;

private:
    StagedPrimaryPass() = default;
    bool Initialise(VkPhysicalDevice physicalDevice, VkDescriptorSetLayout sceneLayout,
                    VkExtent2D extent, std::uint32_t pushConstantBytes,
                    const RtPipelineBundleBuildApi& api, std::string& diagnostic);
    bool MakePages(VkExtent2D extent, std::array<RtGpuBuffer, kStagedPageCount>& pages,
                   StagedPrimaryExtent& contract, std::string& diagnostic);
    void WritePageDescriptors() noexcept;
    void DestroyPages(std::array<RtGpuBuffer, kStagedPageCount>& pages) noexcept;
    void Reset() noexcept;

    VkDevice device_ = VK_NULL_HANDLE;
    RtGpuResources resources_{}; // Non-owning device seam copied by value.
    VkDescriptorSet sceneSet_ = VK_NULL_HANDLE; // Borrowed; never destroyed here.
    VkDescriptorSetLayout pageLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool pagePool_ = VK_NULL_HANDLE;
    VkDescriptorSet pageSet_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    PFN_vkCmdTraceRaysKHR trace_ = nullptr;
    std::array<std::array<RtStrategyPipelineResources, 2u>, 2u> passes_{};
    std::array<RtGpuBuffer, kStagedPageCount> pages_{};
    std::array<RtGpuBuffer, kStagedPageCount> replacementPages_{};
    StagedPrimaryExtent extentContract_{};
    StagedPrimaryExtent replacementContract_{};
    VkExtent2D extent_{};
    VkExtent2D replacementExtent_{};
    std::uint64_t maxStorageBufferRange_ = 0u;
};

} // namespace horde::vulkan::raytracing::experimental
