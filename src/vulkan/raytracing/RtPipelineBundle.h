#pragma once

#include "vulkan/raytracing/RtGpuResources.h"
#include "vulkan/raytracing/RtPipelineBundleContracts.h"

#include <array>
#include <string>
#include <string_view>

#include <vulkan/vulkan.h>

namespace horde::vulkan::raytracing {

enum class RtPipelineOwnedBuffer {
    OpaqueFastSbt,
    GenericDielectricSbt,
    Diagnostics,
};

struct RtStrategyPipelineResources {
    RtPipelineVariantArtifact artifact{};
    VkPipeline pipeline = VK_NULL_HANDLE;
    RtGpuBuffer shaderBindingTable{};
    std::array<VkStridedDeviceAddressRegionKHR, 4u> sbtRegions{};
};

struct RtPipelineBundleDestroyApi {
    void* user = nullptr;
    RtGpuResources* gpuResources = nullptr;
    void (*destroyBuffer)(void*, RtGpuResources*, RtGpuBuffer&,
                          RtPipelineOwnedBuffer) noexcept = nullptr;
    void (*destroyPipeline)(void*, VkPipeline&, RtMaterialStrategy) noexcept = nullptr;
    void (*destroyPipelineLayout)(void*, VkPipelineLayout&) noexcept = nullptr;
    void (*destroyDescriptorPool)(void*, VkDescriptorPool&) noexcept = nullptr;
    void (*destroyDescriptorSetLayout)(void*, VkDescriptorSetLayout&) noexcept = nullptr;

    [[nodiscard]] bool Complete() const noexcept;
};

enum class RtPipelineBundleBuildStep {
    None,
    DescriptorSetLayout,
    DescriptorPool,
    DescriptorSet,
    DiagnosticBuffer,
    DescriptorWrites,
    PipelineLayout,
    SharedShaderModules,
    OpaqueFastShaderModule,
    OpaqueFastPipeline,
    GenericDielectricShaderModule,
    GenericDielectricPipeline,
    OpaqueFastSbt,
    GenericDielectricSbt,
};

struct RtPipelineBundleBuildApi {
    void* user = nullptr;
    bool (*createDescriptorSetLayout)(void*, const RtDescriptorIoContract&,
                                      VkDescriptorSetLayout&, std::string&) = nullptr;
    bool (*createDescriptorPool)(void*, const RtDescriptorIoContract&,
                                 VkDescriptorPool&, std::string&) = nullptr;
    bool (*allocateDescriptorSet)(void*, VkDescriptorPool, VkDescriptorSetLayout,
                                  VkDescriptorSet&, std::string&) = nullptr;
    bool (*createDiagnosticBuffer)(void*, RtGpuBuffer&, std::string&) = nullptr;
    bool (*writeDescriptors)(void*, class RtPipelineBundle&, std::string&) = nullptr;
    bool (*createPipelineLayout)(void*, VkDescriptorSetLayout,
                                 VkPipelineLayout&, std::string&) = nullptr;
    bool (*createSharedShaderModules)(void*, VkShaderModule&, VkShaderModule&,
                                      std::string&) = nullptr;
    bool (*createRaygenShaderModule)(void*, const RtPipelineVariantArtifact&,
                                     VkShaderModule&, std::string&) = nullptr;
    bool (*createStrategyPipeline)(void*, RtMaterialStrategy, VkShaderModule,
                                   VkShaderModule, VkShaderModule, VkPipelineLayout,
                                   VkPipeline&, std::string&) = nullptr;
    void (*destroyShaderModule)(void*, VkShaderModule&) noexcept = nullptr;
    bool (*createStrategySbt)(void*, RtMaterialStrategy, VkPipeline, RtGpuBuffer&,
                              std::array<VkStridedDeviceAddressRegionKHR, 4u>&,
                              std::string&) = nullptr;

    [[nodiscard]] bool Complete() const noexcept;
};

class RtPipelineBundle final {
public:
    RtPipelineBundle() = default;
    ~RtPipelineBundle();
    RtPipelineBundle(const RtPipelineBundle&) = delete;
    RtPipelineBundle& operator=(const RtPipelineBundle&) = delete;
    RtPipelineBundle(RtPipelineBundle&& other) noexcept;
    RtPipelineBundle& operator=(RtPipelineBundle&& other) noexcept;

    [[nodiscard]] bool AdoptPreflight(RtPipelineBundlePreflight preflight,
                                      RtPipelineBundleDestroyApi destroyApi,
                                      std::string& diagnostic);
    void RebindDestroyContext(void* user, RtGpuResources* gpuResources) noexcept;
    void Reset() noexcept;

    [[nodiscard]] bool HasSelection() const noexcept { return selected_; }
    [[nodiscard]] bool HasLiveResources() const noexcept;
    [[nodiscard]] const RtPipelineBundleRequest& Request() const noexcept { return preflight_.request; }
    [[nodiscard]] const RtDescriptorIoContract& DescriptorIo() const noexcept {
        return preflight_.descriptorIo;
    }
    [[nodiscard]] RtDiagnosticAvailability DiagnosticAvailability() const noexcept {
        return diagnosticAvailability_;
    }
    [[nodiscard]] std::string_view OpaqueFastKey() const noexcept;
    [[nodiscard]] std::string_view GenericDielectricKey() const noexcept;
    [[nodiscard]] std::string_view OpaqueFastSha256() const noexcept;
    [[nodiscard]] std::string_view GenericDielectricSha256() const noexcept;

    [[nodiscard]] RtStrategyPipelineResources& Strategy(RtMaterialStrategy strategy) noexcept;
    [[nodiscard]] const RtStrategyPipelineResources& Strategy(
        RtMaterialStrategy strategy) const noexcept;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    RtGpuBuffer diagnosticBuffer{};

private:
    friend bool BuildRtPipelineBundleResources(RtPipelineBundle&,
                                                const RtPipelineBundleBuildApi&,
                                                std::string&);
    void MoveFrom(RtPipelineBundle&& other) noexcept;

    RtPipelineBundlePreflight preflight_{};
    std::array<RtStrategyPipelineResources, 2u> strategies_{};
    RtPipelineBundleDestroyApi destroyApi_{};
    RtDiagnosticAvailability diagnosticAvailability_ =
        RtDiagnosticAvailability::Unavailable;
    bool selected_ = false;
};

[[nodiscard]] bool BuildRtPipelineBundleResources(
    RtPipelineBundle& bundle,
    const RtPipelineBundleBuildApi& api,
    std::string& diagnostic);

} // namespace horde::vulkan::raytracing
