#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "gameplay/SwordCombat.h"
#include "scene/SkeletonBipedModel.h"

namespace horde::vulkan::raytracing
{

class PresentableTinyRtScene
{
public:
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
                    const std::string& materialAssetDirectory,
                    std::string& diagnostic);

    void Destroy();

    bool IsReady() const { return ready_; }
    VkExtent2D DispatchExtent() const { return dispatchExtent_; }
    const std::string& MaterialEncoding() const { return materialEncoding_; }

    bool RecordTraceAndCopy(VkCommandBuffer commandBuffer,
                            VkImage swapchainImage,
                            VkImageLayout& swapchainImageLayout,
                            VkExtent2D swapchainExtent,
                            float cameraYaw,
                            float cameraPitch,
                            float lanternStrength,
                            float walkTime,
                             float cameraX,
                             float cameraZ,
                             float walkAmount,
                             float outputExposure,
                             const horde::gameplay::CombatSnapshot& combat,
                             std::string& diagnostic);

private:
    struct Buffer
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceAddress address = 0;
        VkDeviceSize size = 0;
    };

    struct AccelerationStructure
    {
        Buffer backing;
        VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
        VkDeviceAddress address = 0;
    };

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
    bool CreateStorageImage(std::string& diagnostic);
    bool CreateTextureArray(const std::string& path, VkFormat format, TextureArray& texture, std::string& diagnostic);
    bool SupportsTextureArrayFormat(VkFormat format) const;
    bool CreateMaterialTextures(const std::string& directory, std::string& diagnostic);
    bool BuildAccelerationStructures(std::string& diagnostic);
    bool CreateDescriptors(std::string& diagnostic);
    bool CreatePipeline(std::string& diagnostic);
    bool CreateShaderBindingTable(std::string& diagnostic);
    bool UpdateDynamicInstances(VkCommandBuffer commandBuffer,
                                float cameraYaw,
                                float cameraPitch,
                                float walkTime,
                                float cameraX,
                                float cameraZ,
                                float walkAmount,
                                const horde::gameplay::CombatSnapshot& combat,
                                std::string& diagnostic);
    bool RunOneTimeCommands(void (*record)(VkCommandBuffer, void*), void* userData, std::string& diagnostic) const;
    void DestroyBuffer(Buffer& buffer) const;
    void DestroyAccelerationStructure(AccelerationStructure& accelerationStructure);
    void DestroyTextureArray(TextureArray& texture);

    std::uint32_t FindMemoryType(std::uint32_t typeBits, VkMemoryPropertyFlags flags) const;
    VkDeviceAddress BufferAddress(VkBuffer buffer) const;

    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue queue_ = VK_NULL_HANDLE;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    VkExtent2D dispatchExtent_{};
    bool presentationUsesBgra_ = false;

    VkImage storageImage_ = VK_NULL_HANDLE;
    VkDeviceMemory storageImageMemory_ = VK_NULL_HANDLE;
    VkImageView storageImageView_ = VK_NULL_HANDLE;
    VkImageLayout storageImageLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    TextureArray materialDiffuse_;
    TextureArray materialNormal_;
    TextureArray materialArm_;
    VkSampler materialSampler_ = VK_NULL_HANDLE;
    std::string materialEncoding_;

    Buffer vertexBuffer_;
    Buffer indexBuffer_;
    Buffer transformBuffer_;
    Buffer instanceBuffer_;
    Buffer skeletonVertexBuffer_;
    AccelerationStructure blas_;
    AccelerationStructure torchBlas_;
    AccelerationStructure swordBlas_;
    AccelerationStructure playerBodyBlas_;
    AccelerationStructure playerLimbBlas_;
    AccelerationStructure skeletonBlas_;
    AccelerationStructure tlas_;
    Buffer skeletonBlasUpdateScratch_;
    Buffer tlasUpdateScratch_;
    horde::scene::SkeletonBipedModel skeletonModel_;
    std::vector<horde::scene::SkinnedRtVertex> skeletonSkinnedVertices_;
    float lastSkeletonUpdateTime_ = -1.0f;
    int lastSkeletonClip_ = -1;

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
