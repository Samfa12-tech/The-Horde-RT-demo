#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "gameplay/SwordCombat.h"
#include "gameplay/ShowcaseGameplay.h"
#include "vulkan/raytracing/CharacterRenderSlot.h"
#include "vulkan/raytracing/RtGpuResources.h"

namespace horde::vulkan::raytracing
{

struct RtSceneFrameInputs
{
    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    float lanternStrength = 1.0f;
    float walkTime = 0.0f;
    float cameraX = 0.0f;
    float cameraZ = 1.85f;
    float walkAmount = 0.0f;
    float outputExposure = 0.92f;
    horde::gameplay::CombatSnapshot combat{};
    horde::gameplay::LanternSnapshot lantern{};
    horde::gameplay::EnemyRosterSnapshot roster{};
    horde::gameplay::LichSnapshot lich{};
};

class PresentableTinyRtScene
{
public:
    struct StorageImageCapture
    {
        std::uint32_t width = 0u;
        std::uint32_t height = 0u;
        bool redBlueSwapNormalised = false;
        std::vector<std::uint8_t> rgba;
    };

    static constexpr std::uint32_t kBlasCount = 8u;
    static constexpr std::uint32_t kTlasCount = 1u;
    static constexpr std::uint32_t kTlasInstanceCount = 18u;

    PresentableTinyRtScene() = default;
    ~PresentableTinyRtScene();

    PresentableTinyRtScene(const PresentableTinyRtScene&) = delete;
    PresentableTinyRtScene& operator=(const PresentableTinyRtScene&) = delete;
    PresentableTinyRtScene(PresentableTinyRtScene&& other) noexcept;
    PresentableTinyRtScene& operator=(PresentableTinyRtScene&& other) noexcept;

    bool Initialise(VkInstance instance,
                    VkPhysicalDevice physicalDevice,
                    VkDevice device,
                    VkQueue queue,
                    VkCommandPool commandPool,
                    VkExtent2D dispatchExtent,
                    VkFormat presentationFormat,
                    const std::string& skeletonAssetPath,
                    const std::string& lichAssetPath,
                    const std::string& materialAssetDirectory,
                    const std::string& lichTextureDirectory,
                    std::string& diagnostic);

    void Destroy();

    bool IsReady() const { return ready_; }
    VkExtent2D DispatchExtent() const { return dispatchExtent_; }
    const std::string& MaterialEncoding() const { return materialEncoding_; }
    std::uint32_t BlasCount() const { return ready_ ? kBlasCount : 0u; }
    std::uint32_t TlasCount() const { return ready_ ? kTlasCount : 0u; }
    std::uint32_t TlasInstanceCount() const { return ready_ ? kTlasInstanceCount : 0u; }

    bool RecordTraceAndCopy(VkCommandBuffer commandBuffer,
                            VkImage swapchainImage,
                            VkImageLayout& swapchainImageLayout,
                            VkExtent2D swapchainExtent,
                            const RtSceneFrameInputs& frame,
                            std::string& diagnostic);

    // Synchronously reads the last RT-produced storage image. The returned
    // bytes are canonical RGBA even when the presentation push constant had
    // swapped red/blue for a raw copy to a BGRA swapchain.
    bool CaptureStorageImage(StorageImageCapture& capture, std::string& diagnostic);

private:
    using Buffer = RtGpuBuffer;
    using AccelerationStructure = RtAccelerationStructure;

    struct TextureArray
    {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };

    bool LoadEntryPoints(std::string& diagnostic);
    bool CreateBuffer(VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags memoryFlags,
                      bool deviceAddress,
                      Buffer& out,
                      std::string& diagnostic) const;
    bool WriteBuffer(const Buffer& buffer,
                     const void* data,
                     VkDeviceSize size,
                     const char* label,
                     std::string& diagnostic) const;
    bool CreateStorageImage(std::string& diagnostic);
    bool CreateTextureArray(const std::string& path, VkFormat format, TextureArray& texture, std::string& diagnostic);
    bool CreateTexture(const std::string& path,
                       VkFormat format,
                       std::uint32_t width,
                       std::uint32_t height,
                       std::uint32_t layers,
                       TextureArray& texture,
                       std::string& diagnostic);
    bool SupportsTextureArrayFormat(VkFormat format) const;
    bool CreateMaterialTextures(const std::string& directory, std::string& diagnostic);
    bool CreateLichTextures(const std::string& directory, std::string& diagnostic);
    bool BuildAccelerationStructures(std::string& diagnostic);
    bool CreateDescriptors(std::string& diagnostic);
    bool CreatePipeline(std::string& diagnostic);
    bool CreateShaderBindingTable(std::string& diagnostic);
    bool UpdateDynamicInstances(VkCommandBuffer commandBuffer,
                                const RtSceneFrameInputs& frame,
                                std::string& diagnostic);
    bool RunOneTimeCommands(void (*record)(VkCommandBuffer, void*), void* userData, std::string& diagnostic) const;
    void DestroyBuffer(Buffer& buffer) const;
    void DestroyAccelerationStructure(AccelerationStructure& accelerationStructure);
    void DestroyTextureArray(TextureArray& texture);

    VkDeviceAddress BufferAddress(VkBuffer buffer) const;

    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue queue_ = VK_NULL_HANDLE;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    VkExtent2D dispatchExtent_{};
    bool presentationUsesBgra_ = false;
    bool scaledBlitSupported_ = false;

    VkImage storageImage_ = VK_NULL_HANDLE;
    VkDeviceMemory storageImageMemory_ = VK_NULL_HANDLE;
    VkImageView storageImageView_ = VK_NULL_HANDLE;
    VkImageLayout storageImageLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    bool lastOutputRedBlueSwapApplied_ = false;
    TextureArray materialDiffuse_;
    TextureArray materialNormal_;
    TextureArray materialArm_;
    TextureArray lichBaseColor_;
    TextureArray lichEmissive_;
    VkSampler materialSampler_ = VK_NULL_HANDLE;
    std::string materialEncoding_;

    Buffer vertexBuffer_;
    Buffer indexBuffer_;
    Buffer transformBuffer_;
    Buffer instanceBuffer_;
    Buffer worldSurfaceBuffer_;
    AccelerationStructure blas_;
    AccelerationStructure finaleRoofBlas_;
    AccelerationStructure torchBlas_;
    AccelerationStructure swordBlas_;
    AccelerationStructure playerBodyBlas_;
    AccelerationStructure playerLimbBlas_;
    AccelerationStructure tlas_;
    Buffer tlasUpdateScratch_;
    RtGpuResources gpuResources_;
    CharacterRenderSlot characterSlot_;

    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    Buffer shaderBindingTable_;
    VkStridedDeviceAddressRegionKHR raygenRegion_{};
    VkStridedDeviceAddressRegionKHR missRegion_{};
    VkStridedDeviceAddressRegionKHR hitRegion_{};
    VkStridedDeviceAddressRegionKHR callableRegion_{};

    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR_ = nullptr;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR_ = nullptr;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR_ = nullptr;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR_ = nullptr;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR_ = nullptr;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR_ = nullptr;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR_ = nullptr;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR_ = nullptr;
    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR_ = nullptr;

    bool ready_ = false;
};

} // namespace horde::vulkan::raytracing
