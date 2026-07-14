#include "vulkan/raytracing/PresentableTinyRtScene.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <utility>
#include <vector>

namespace horde::vulkan::raytracing
{

namespace
{

constexpr VkFormat kStorageImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

struct ScenePushConstants
{
    float yaw = 0.0f;
    float pitch = 0.0f;
    float lantern = 1.0f;
    float time = 0.0f;
    float cameraX = 0.0f;
    float cameraZ = 4.7f;
    float walkAmount = 0.0f;
    float outputRedBlueSwap = 0.0f;
    float outputExposure = 0.92f;
    float damageFlash = 0.0f;
};

// Generated from shaders/raytracing/minimal.rgen with glslangValidator -V -Os.
// Keep this embedded so the Android RT scene remains a self-contained native build.
constexpr std::uint32_t kMinimalRayGenShader[] = {
#include "vulkan/raytracing/MinimalRayGenShader.inc"
};

constexpr std::uint32_t kMinimalMissShader[] = {
    0x07230203u, 0x00010500u, 0x0008000bu, 0x00000069u, 0x00000000u, 0x00020011u, 0x0000117fu, 0x0006000au,
    0x5f565053u, 0x5f52484bu, 0x5f796172u, 0x63617274u, 0x00676e69u, 0x0006000bu, 0x00000001u, 0x4c534c47u,
    0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu, 0x00000000u, 0x00000001u, 0x0007000fu, 0x000014c5u,
    0x00000004u, 0x6e69616du, 0x00000000u, 0x0000000bu, 0x00000045u, 0x00030003u, 0x00000002u, 0x000001ccu,
    0x00060004u, 0x455f4c47u, 0x725f5458u, 0x745f7961u, 0x69636172u, 0x0000676eu, 0x00040005u, 0x00000004u,
    0x6e69616du, 0x00000000u, 0x00030005u, 0x00000009u, 0x00000064u, 0x00080005u, 0x0000000bu, 0x575f6c67u,
    0x646c726fu, 0x44796152u, 0x63657269u, 0x6e6f6974u, 0x00545845u, 0x00040005u, 0x0000000fu, 0x6e6f6f6du,
    0x00000000u, 0x00040005u, 0x0000001au, 0x69726f68u, 0x006e6f7au, 0x00030005u, 0x00000022u, 0x00796b73u,
    0x00040005u, 0x00000045u, 0x6c796170u, 0x0064616fu, 0x00040047u, 0x0000000bu, 0x0000000bu, 0x000014cau,
    0x00020013u, 0x00000002u, 0x00030021u, 0x00000003u, 0x00000002u, 0x00030016u, 0x00000006u, 0x00000020u,
    0x00040017u, 0x00000007u, 0x00000006u, 0x00000003u, 0x00040020u, 0x00000008u, 0x00000007u, 0x00000007u,
    0x00040020u, 0x0000000au, 0x00000001u, 0x00000007u, 0x0004003bu, 0x0000000au, 0x0000000bu, 0x00000001u,
    0x00040020u, 0x0000000eu, 0x00000007u, 0x00000006u, 0x0004002bu, 0x00000006u, 0x00000011u, 0xbe8f4d7du,
    0x0004002bu, 0x00000006u, 0x00000012u, 0x3effe5cdu, 0x0004002bu, 0x00000006u, 0x00000013u, 0xbf51d609u,
    0x0006002cu, 0x00000007u, 0x00000014u, 0x00000011u, 0x00000012u, 0x00000013u, 0x0004002bu, 0x00000006u,
    0x00000016u, 0x00000000u, 0x0004002bu, 0x00000006u, 0x00000018u, 0x42800000u, 0x0004002bu, 0x00000006u,
    0x0000001bu, 0xbe6147aeu, 0x0004002bu, 0x00000006u, 0x0000001cu, 0x3ee66666u, 0x00040015u, 0x0000001du,
    0x00000020u, 0x00000000u, 0x0004002bu, 0x0000001du, 0x0000001eu, 0x00000001u, 0x0004002bu, 0x00000006u,
    0x00000023u, 0x3c75c28fu, 0x0004002bu, 0x00000006u, 0x00000024u, 0x3c9374bcu, 0x0004002bu, 0x00000006u,
    0x00000025u, 0x3cc49ba6u, 0x0006002cu, 0x00000007u, 0x00000026u, 0x00000023u, 0x00000024u, 0x00000025u,
    0x0004002bu, 0x00000006u, 0x00000027u, 0x3d23d70au, 0x0004002bu, 0x00000006u, 0x00000028u, 0x3d851eb8u,
    0x0004002bu, 0x00000006u, 0x00000029u, 0x3dd70a3du, 0x0006002cu, 0x00000007u, 0x0000002au, 0x00000027u,
    0x00000028u, 0x00000029u, 0x0004002bu, 0x00000006u, 0x0000002eu, 0x3e800000u, 0x0004002bu, 0x00000006u,
    0x0000002fu, 0x3ea8f5c3u, 0x0004002bu, 0x00000006u, 0x00000030u, 0x3ef5c28fu, 0x0006002cu, 0x00000007u,
    0x00000031u, 0x0000002eu, 0x0000002fu, 0x00000030u, 0x0004002bu, 0x00000006u, 0x00000036u, 0x3db851ecu,
    0x0004002bu, 0x00000006u, 0x00000037u, 0x3d0f5c29u, 0x0004002bu, 0x00000006u, 0x00000038u, 0x3c449ba6u,
    0x0006002cu, 0x00000007u, 0x00000039u, 0x00000036u, 0x00000037u, 0x00000038u, 0x0004002bu, 0x00000006u,
    0x0000003au, 0xbeb33333u, 0x0004002bu, 0x00000006u, 0x0000003bu, 0x3e19999au, 0x00040017u, 0x00000043u,
    0x00000006u, 0x00000004u, 0x00040020u, 0x00000044u, 0x000014deu, 0x00000043u, 0x0004003bu, 0x00000044u,
    0x00000045u, 0x000014deu, 0x0004002bu, 0x0000001du, 0x00000046u, 0x00000003u, 0x00040020u, 0x00000047u,
    0x000014deu, 0x00000006u, 0x0004002bu, 0x00000006u, 0x0000004au, 0xbfc00000u, 0x00020014u, 0x0000004bu,
    0x0004002bu, 0x00000006u, 0x00000050u, 0x3f400000u, 0x0004002bu, 0x00000006u, 0x00000052u, 0x3ca3d70au,
    0x0004002bu, 0x00000006u, 0x00000053u, 0x3cf5c28fu, 0x0006002cu, 0x00000007u, 0x00000054u, 0x00000052u,
    0x00000025u, 0x00000053u, 0x0004002bu, 0x00000006u, 0x00000056u, 0x3f800000u, 0x0004002bu, 0x00000006u,
    0x0000005eu, 0xbf000000u, 0x0007002cu, 0x00000043u, 0x00000062u, 0x00000056u, 0x00000056u, 0x00000056u,
    0x00000016u, 0x00050036u, 0x00000002u, 0x00000004u, 0x00000000u, 0x00000003u, 0x000200f8u, 0x00000005u,
    0x0004003bu, 0x00000008u, 0x00000009u, 0x00000007u, 0x0004003bu, 0x0000000eu, 0x0000000fu, 0x00000007u,
    0x0004003bu, 0x0000000eu, 0x0000001au, 0x00000007u, 0x0004003bu, 0x00000008u, 0x00000022u, 0x00000007u,
    0x0004003du, 0x00000007u, 0x0000000cu, 0x0000000bu, 0x0006000cu, 0x00000007u, 0x0000000du, 0x00000001u,
    0x00000045u, 0x0000000cu, 0x0003003eu, 0x00000009u, 0x0000000du, 0x0004003du, 0x00000007u, 0x00000010u,
    0x00000009u, 0x00050094u, 0x00000006u, 0x00000015u, 0x00000010u, 0x00000014u, 0x0007000cu, 0x00000006u,
    0x00000017u, 0x00000001u, 0x00000028u, 0x00000015u, 0x00000016u, 0x0007000cu, 0x00000006u, 0x00000019u,
    0x00000001u, 0x0000001au, 0x00000017u, 0x00000018u, 0x0003003eu, 0x0000000fu, 0x00000019u, 0x00050041u,
    0x0000000eu, 0x0000001fu, 0x00000009u, 0x0000001eu, 0x0004003du, 0x00000006u, 0x00000020u, 0x0000001fu,
    0x0008000cu, 0x00000006u, 0x00000021u, 0x00000001u, 0x00000031u, 0x0000001bu, 0x0000001cu, 0x00000020u,
    0x0003003eu, 0x0000001au, 0x00000021u, 0x0004003du, 0x00000006u, 0x0000002bu, 0x0000001au, 0x00060050u,
    0x00000007u, 0x0000002cu, 0x0000002bu, 0x0000002bu, 0x0000002bu, 0x0008000cu, 0x00000007u, 0x0000002du,
    0x00000001u, 0x0000002eu, 0x00000026u, 0x0000002au, 0x0000002cu, 0x0003003eu, 0x00000022u, 0x0000002du,
    0x0004003du, 0x00000006u, 0x00000032u, 0x0000000fu, 0x0005008eu, 0x00000007u, 0x00000033u, 0x00000031u,
    0x00000032u, 0x0004003du, 0x00000007u, 0x00000034u, 0x00000022u, 0x00050081u, 0x00000007u, 0x00000035u,
    0x00000034u, 0x00000033u, 0x0003003eu, 0x00000022u, 0x00000035u, 0x00050041u, 0x0000000eu, 0x0000003cu,
    0x00000009u, 0x0000001eu, 0x0004003du, 0x00000006u, 0x0000003du, 0x0000003cu, 0x0004007fu, 0x00000006u,
    0x0000003eu, 0x0000003du, 0x0008000cu, 0x00000006u, 0x0000003fu, 0x00000001u, 0x00000031u, 0x0000003au,
    0x0000003bu, 0x0000003eu, 0x0005008eu, 0x00000007u, 0x00000040u, 0x00000039u, 0x0000003fu, 0x0004003du,
    0x00000007u, 0x00000041u, 0x00000022u, 0x00050081u, 0x00000007u, 0x00000042u, 0x00000041u, 0x00000040u,
    0x0003003eu, 0x00000022u, 0x00000042u, 0x00050041u, 0x00000047u, 0x00000048u, 0x00000045u, 0x00000046u,
    0x0004003du, 0x00000006u, 0x00000049u, 0x00000048u, 0x000500b8u, 0x0000004bu, 0x0000004cu, 0x00000049u,
    0x0000004au, 0x000300f7u, 0x0000004eu, 0x00000000u, 0x000400fau, 0x0000004cu, 0x0000004du, 0x0000004eu,
    0x000200f8u, 0x0000004du, 0x0004003du, 0x00000007u, 0x0000004fu, 0x00000022u, 0x0005008eu, 0x00000007u,
    0x00000051u, 0x0000004fu, 0x00000050u, 0x00050081u, 0x00000007u, 0x00000055u, 0x00000051u, 0x00000054u,
    0x00050051u, 0x00000006u, 0x00000057u, 0x00000055u, 0x00000000u, 0x00050051u, 0x00000006u, 0x00000058u,
    0x00000055u, 0x00000001u, 0x00050051u, 0x00000006u, 0x00000059u, 0x00000055u, 0x00000002u, 0x00070050u,
    0x00000043u, 0x0000005au, 0x00000057u, 0x00000058u, 0x00000059u, 0x00000056u, 0x0003003eu, 0x00000045u,
    0x0000005au, 0x000100fdu, 0x000200f8u, 0x0000004eu, 0x00050041u, 0x00000047u, 0x0000005cu, 0x00000045u,
    0x00000046u, 0x0004003du, 0x00000006u, 0x0000005du, 0x0000005cu, 0x000500b8u, 0x0000004bu, 0x0000005fu,
    0x0000005du, 0x0000005eu, 0x000300f7u, 0x00000061u, 0x00000000u, 0x000400fau, 0x0000005fu, 0x00000060u,
    0x00000061u, 0x000200f8u, 0x00000060u, 0x0003003eu, 0x00000045u, 0x00000062u, 0x000100fdu, 0x000200f8u,
    0x00000061u, 0x0004003du, 0x00000007u, 0x00000064u, 0x00000022u, 0x00050051u, 0x00000006u, 0x00000065u,
    0x00000064u, 0x00000000u, 0x00050051u, 0x00000006u, 0x00000066u, 0x00000064u, 0x00000001u, 0x00050051u,
    0x00000006u, 0x00000067u, 0x00000064u, 0x00000002u, 0x00070050u, 0x00000043u, 0x00000068u, 0x00000065u,
    0x00000066u, 0x00000067u, 0x00000056u, 0x0003003eu, 0x00000045u, 0x00000068u, 0x000100fdu, 0x00010038u
};

constexpr std::uint32_t kMinimalClosestHitShader[] = {
    0x07230203u, 0x00010500u, 0x0008000bu, 0x0000000fu, 0x00000000u, 0x00020011u, 0x0000117fu, 0x0006000au,
    0x5f565053u, 0x5f52484bu, 0x5f796172u, 0x63617274u, 0x00676e69u, 0x0006000bu, 0x00000001u, 0x4c534c47u,
    0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu, 0x00000000u, 0x00000001u, 0x0006000fu, 0x000014c4u,
    0x00000004u, 0x6e69616du, 0x00000000u, 0x00000009u, 0x00030003u, 0x00000002u, 0x000001ccu, 0x00060004u,
    0x455f4c47u, 0x725f5458u, 0x745f7961u, 0x69636172u, 0x0000676eu, 0x00040005u, 0x00000004u, 0x6e69616du,
    0x00000000u, 0x00040005u, 0x00000009u, 0x6c796170u, 0x0064616fu, 0x00020013u, 0x00000002u, 0x00030021u,
    0x00000003u, 0x00000002u, 0x00030016u, 0x00000006u, 0x00000020u, 0x00040017u, 0x00000007u, 0x00000006u,
    0x00000004u, 0x00040020u, 0x00000008u, 0x000014deu, 0x00000007u, 0x0004003bu, 0x00000008u, 0x00000009u,
    0x000014deu, 0x0004002bu, 0x00000006u, 0x0000000au, 0x3ca3d70au, 0x0004002bu, 0x00000006u, 0x0000000bu,
    0x3d75c28fu, 0x0004002bu, 0x00000006u, 0x0000000cu, 0x3df5c28fu, 0x0004002bu, 0x00000006u, 0x0000000du,
    0x3f800000u, 0x0007002cu, 0x00000007u, 0x0000000eu, 0x0000000au, 0x0000000bu, 0x0000000cu, 0x0000000du,
    0x00050036u, 0x00000002u, 0x00000004u, 0x00000000u, 0x00000003u, 0x000200f8u, 0x00000005u, 0x0003003eu,
    0x00000009u, 0x0000000eu, 0x000100fdu, 0x00010038u
};

std::uint32_t AlignUp(const std::uint32_t value, const std::uint32_t alignment)
{
    return alignment == 0u ? value : ((value + alignment - 1u) / alignment) * alignment;
}

void SetImageBarrier(VkCommandBuffer commandBuffer,
                     VkImage image,
                     VkImageLayout oldLayout,
                     VkImageLayout newLayout,
                     VkPipelineStageFlags srcStage,
                     VkPipelineStageFlags dstStage,
                     VkAccessFlags srcAccess,
                     VkAccessFlags dstAccess)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1u;
    barrier.subresourceRange.layerCount = 1u;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;
    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0u, 0u, nullptr, 0u, nullptr, 1u, &barrier);
}

bool CreateShaderModule(VkDevice device, const std::uint32_t* code, std::size_t byteSize, VkShaderModule& shaderModule)
{
    const VkShaderModuleCreateInfo createInfo{
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        nullptr,
        0u,
        byteSize,
        code};
    return vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) == VK_SUCCESS;
}

} // namespace

PresentableTinyRtScene::~PresentableTinyRtScene()
{
    Destroy();
}

PresentableTinyRtScene::PresentableTinyRtScene(PresentableTinyRtScene&& other) noexcept
{
    *this = std::move(other);
}

PresentableTinyRtScene& PresentableTinyRtScene::operator=(PresentableTinyRtScene&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Destroy();

    physicalDevice_ = std::exchange(other.physicalDevice_, nullptr);
    instance_ = std::exchange(other.instance_, nullptr);
    device_ = std::exchange(other.device_, nullptr);
    queue_ = std::exchange(other.queue_, nullptr);
    commandPool_ = std::exchange(other.commandPool_, VK_NULL_HANDLE);
    dispatchExtent_ = std::exchange(other.dispatchExtent_, VkExtent2D{});
    presentationUsesBgra_ = std::exchange(other.presentationUsesBgra_, false);
    storageImage_ = std::exchange(other.storageImage_, VK_NULL_HANDLE);
    storageImageMemory_ = std::exchange(other.storageImageMemory_, VK_NULL_HANDLE);
    storageImageView_ = std::exchange(other.storageImageView_, VK_NULL_HANDLE);
    storageImageLayout_ = std::exchange(other.storageImageLayout_, VK_IMAGE_LAYOUT_UNDEFINED);
    materialDiffuse_ = std::exchange(other.materialDiffuse_, TextureArray{});
    materialNormal_ = std::exchange(other.materialNormal_, TextureArray{});
    materialArm_ = std::exchange(other.materialArm_, TextureArray{});
    materialSampler_ = std::exchange(other.materialSampler_, VK_NULL_HANDLE);
    materialEncoding_ = std::move(other.materialEncoding_);
    vertexBuffer_ = std::exchange(other.vertexBuffer_, Buffer{});
    indexBuffer_ = std::exchange(other.indexBuffer_, Buffer{});
    transformBuffer_ = std::exchange(other.transformBuffer_, Buffer{});
    instanceBuffer_ = std::exchange(other.instanceBuffer_, Buffer{});
    skeletonVertexBuffer_ = std::exchange(other.skeletonVertexBuffer_, Buffer{});
    worldSurfaceBuffer_ = std::exchange(other.worldSurfaceBuffer_, Buffer{});
    blas_ = std::exchange(other.blas_, AccelerationStructure{});
    torchBlas_ = std::exchange(other.torchBlas_, AccelerationStructure{});
    swordBlas_ = std::exchange(other.swordBlas_, AccelerationStructure{});
    playerBodyBlas_ = std::exchange(other.playerBodyBlas_, AccelerationStructure{});
    playerLimbBlas_ = std::exchange(other.playerLimbBlas_, AccelerationStructure{});
    skeletonBlas_ = std::exchange(other.skeletonBlas_, AccelerationStructure{});
    tlas_ = std::exchange(other.tlas_, AccelerationStructure{});
    skeletonBlasUpdateScratch_ = std::exchange(other.skeletonBlasUpdateScratch_, Buffer{});
    tlasUpdateScratch_ = std::exchange(other.tlasUpdateScratch_, Buffer{});
    skeletonModel_ = std::move(other.skeletonModel_);
    skeletonSkinnedVertices_ = std::move(other.skeletonSkinnedVertices_);
    lastSkeletonUpdateTime_ = std::exchange(other.lastSkeletonUpdateTime_, -1.0f);
    lastSkeletonClip_ = std::exchange(other.lastSkeletonClip_, -1);
    descriptorSetLayout_ = std::exchange(other.descriptorSetLayout_, VK_NULL_HANDLE);
    descriptorPool_ = std::exchange(other.descriptorPool_, VK_NULL_HANDLE);
    descriptorSet_ = std::exchange(other.descriptorSet_, VK_NULL_HANDLE);
    pipelineLayout_ = std::exchange(other.pipelineLayout_, VK_NULL_HANDLE);
    pipeline_ = std::exchange(other.pipeline_, VK_NULL_HANDLE);
    shaderBindingTable_ = std::exchange(other.shaderBindingTable_, Buffer{});
    raygenRegion_ = std::exchange(other.raygenRegion_, VkStridedDeviceAddressRegionKHR{});
    missRegion_ = std::exchange(other.missRegion_, VkStridedDeviceAddressRegionKHR{});
    hitRegion_ = std::exchange(other.hitRegion_, VkStridedDeviceAddressRegionKHR{});
    callableRegion_ = std::exchange(other.callableRegion_, VkStridedDeviceAddressRegionKHR{});
    vkCreateAccelerationStructureKHR_ = other.vkCreateAccelerationStructureKHR_;
    vkDestroyAccelerationStructureKHR_ = other.vkDestroyAccelerationStructureKHR_;
    vkGetAccelerationStructureBuildSizesKHR_ = other.vkGetAccelerationStructureBuildSizesKHR_;
    vkGetAccelerationStructureDeviceAddressKHR_ = other.vkGetAccelerationStructureDeviceAddressKHR_;
    vkCmdBuildAccelerationStructuresKHR_ = other.vkCmdBuildAccelerationStructuresKHR_;
    vkCreateRayTracingPipelinesKHR_ = other.vkCreateRayTracingPipelinesKHR_;
    vkGetRayTracingShaderGroupHandlesKHR_ = other.vkGetRayTracingShaderGroupHandlesKHR_;
    vkCmdTraceRaysKHR_ = other.vkCmdTraceRaysKHR_;
    vkGetBufferDeviceAddressKHR_ = other.vkGetBufferDeviceAddressKHR_;
    ready_ = std::exchange(other.ready_, false);

    return *this;
}

bool PresentableTinyRtScene::Initialise(VkInstance instance,
                                        VkPhysicalDevice physicalDevice,
                                        VkDevice device,
                                        VkQueue queue,
                                        VkCommandPool commandPool,
                                        VkExtent2D dispatchExtent,
                                        VkFormat presentationFormat,
                                        const std::string& skeletonAssetPath,
                                        const std::string& materialAssetDirectory,
                                        std::string& diagnostic)
{
    Destroy();

    instance_ = instance;
    physicalDevice_ = physicalDevice;
    device_ = device;
    queue_ = queue;
    commandPool_ = commandPool;
    dispatchExtent_ = dispatchExtent;
    presentationUsesBgra_ = presentationFormat == VK_FORMAT_B8G8R8A8_UNORM || presentationFormat == VK_FORMAT_B8G8R8A8_SRGB;

    if (instance_ == VK_NULL_HANDLE || physicalDevice_ == VK_NULL_HANDLE || device_ == VK_NULL_HANDLE || queue_ == VK_NULL_HANDLE || commandPool_ == VK_NULL_HANDLE)
    {
        diagnostic = "Invalid Vulkan handles supplied to presentable RT scene.";
        return false;
    }
    if (dispatchExtent_.width == 0u || dispatchExtent_.height == 0u)
    {
        diagnostic = "RT dispatch extent is zero.";
        return false;
    }
    if (!skeletonModel_.LoadCombatClips(skeletonAssetPath, diagnostic) ||
        !LoadEntryPoints(diagnostic) ||
        !CreateStorageImage(diagnostic) ||
        !CreateMaterialTextures(materialAssetDirectory, diagnostic) ||
        !BuildAccelerationStructures(diagnostic) ||
        !CreateDescriptors(diagnostic) ||
        !CreatePipeline(diagnostic) ||
        !CreateShaderBindingTable(diagnostic))
    {
        Destroy();
        return false;
    }

    ready_ = true;
    diagnostic.clear();
    return true;
}

void PresentableTinyRtScene::Destroy()
{
    if (device_ == VK_NULL_HANDLE)
    {
        ready_ = false;
        return;
    }

    if (pipeline_ != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device_, pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }
    if (pipelineLayout_ != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }
    if (descriptorPool_ != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
        descriptorPool_ = VK_NULL_HANDLE;
        descriptorSet_ = VK_NULL_HANDLE;
    }
    if (descriptorSetLayout_ != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
        descriptorSetLayout_ = VK_NULL_HANDLE;
    }

    DestroyBuffer(shaderBindingTable_);
    DestroyAccelerationStructure(tlas_);
    DestroyBuffer(tlasUpdateScratch_);
    DestroyAccelerationStructure(skeletonBlas_);
    DestroyBuffer(skeletonBlasUpdateScratch_);
    DestroyAccelerationStructure(playerLimbBlas_);
    DestroyAccelerationStructure(playerBodyBlas_);
    DestroyAccelerationStructure(swordBlas_);
    DestroyAccelerationStructure(torchBlas_);
    DestroyAccelerationStructure(blas_);
    DestroyBuffer(worldSurfaceBuffer_);
    DestroyBuffer(skeletonVertexBuffer_);
    DestroyBuffer(instanceBuffer_);
    DestroyBuffer(transformBuffer_);
    DestroyBuffer(indexBuffer_);
    DestroyBuffer(vertexBuffer_);
    lastSkeletonUpdateTime_ = -1.0f;
    lastSkeletonClip_ = -1;

    if (materialSampler_ != VK_NULL_HANDLE)
    {
        vkDestroySampler(device_, materialSampler_, nullptr);
        materialSampler_ = VK_NULL_HANDLE;
    }
    DestroyTextureArray(materialArm_);
    DestroyTextureArray(materialNormal_);
    DestroyTextureArray(materialDiffuse_);
    materialEncoding_.clear();

    if (storageImageView_ != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device_, storageImageView_, nullptr);
        storageImageView_ = VK_NULL_HANDLE;
    }
    if (storageImage_ != VK_NULL_HANDLE)
    {
        vkDestroyImage(device_, storageImage_, nullptr);
        storageImage_ = VK_NULL_HANDLE;
    }
    if (storageImageMemory_ != VK_NULL_HANDLE)
    {
        vkFreeMemory(device_, storageImageMemory_, nullptr);
        storageImageMemory_ = VK_NULL_HANDLE;
    }

    raygenRegion_ = {};
    missRegion_ = {};
    hitRegion_ = {};
    callableRegion_ = {};
    storageImageLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    ready_ = false;
}

bool PresentableTinyRtScene::LoadEntryPoints(std::string& diagnostic)
{
    vkCreateAccelerationStructureKHR_ = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device_, "vkCreateAccelerationStructureKHR"));
    vkDestroyAccelerationStructureKHR_ = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device_, "vkDestroyAccelerationStructureKHR"));
    vkGetAccelerationStructureBuildSizesKHR_ = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device_, "vkGetAccelerationStructureBuildSizesKHR"));
    vkGetAccelerationStructureDeviceAddressKHR_ = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device_, "vkGetAccelerationStructureDeviceAddressKHR"));
    vkCmdBuildAccelerationStructuresKHR_ = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device_, "vkCmdBuildAccelerationStructuresKHR"));
    vkCreateRayTracingPipelinesKHR_ = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device_, "vkCreateRayTracingPipelinesKHR"));
    vkGetRayTracingShaderGroupHandlesKHR_ = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device_, "vkGetRayTracingShaderGroupHandlesKHR"));
    vkCmdTraceRaysKHR_ = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device_, "vkCmdTraceRaysKHR"));
    vkGetBufferDeviceAddressKHR_ = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device_, "vkGetBufferDeviceAddressKHR"));

    if (!vkCreateAccelerationStructureKHR_ || !vkDestroyAccelerationStructureKHR_ ||
        !vkGetAccelerationStructureBuildSizesKHR_ || !vkGetAccelerationStructureDeviceAddressKHR_ ||
        !vkCmdBuildAccelerationStructuresKHR_ || !vkCreateRayTracingPipelinesKHR_ ||
        !vkGetRayTracingShaderGroupHandlesKHR_ || !vkCmdTraceRaysKHR_ || !vkGetBufferDeviceAddressKHR_)
    {
        diagnostic = "Required Vulkan RT entry points are unavailable.";
        return false;
    }

    diagnostic.clear();
    return true;
}

std::uint32_t PresentableTinyRtScene::FindMemoryType(std::uint32_t typeBits, VkMemoryPropertyFlags flags) const
{
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties);
    for (std::uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        if ((typeBits & (1u << i)) != 0u &&
            (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
        {
            return i;
        }
    }
    return UINT32_MAX;
}

bool PresentableTinyRtScene::CreateBuffer(VkDeviceSize size,
                                          VkBufferUsageFlags usage,
                                          VkMemoryPropertyFlags memoryFlags,
                                          bool deviceAddress,
                                          Buffer& out,
                                          std::string& diagnostic) const
{
    out = {};
    if (deviceAddress)
    {
        usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }

    const VkBufferCreateInfo bufferInfo{
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        nullptr,
        0u,
        size,
        usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0u,
        nullptr};
    if (vkCreateBuffer(device_, &bufferInfo, nullptr, &out.buffer) != VK_SUCCESS)
    {
        diagnostic = "Failed to create RT buffer.";
        return false;
    }

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device_, out.buffer, &requirements);
    const std::uint32_t memoryType = FindMemoryType(requirements.memoryTypeBits, memoryFlags);
    if (memoryType == UINT32_MAX)
    {
        diagnostic = "No compatible memory type for RT buffer.";
        DestroyBuffer(out);
        return false;
    }

    VkMemoryAllocateFlagsInfo flagsInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
    flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    VkMemoryAllocateInfo allocateInfo{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        deviceAddress ? &flagsInfo : nullptr,
        requirements.size,
        memoryType};
    if (vkAllocateMemory(device_, &allocateInfo, nullptr, &out.memory) != VK_SUCCESS ||
        vkBindBufferMemory(device_, out.buffer, out.memory, 0u) != VK_SUCCESS)
    {
        diagnostic = "Failed to allocate RT buffer memory.";
        DestroyBuffer(out);
        return false;
    }

    out.size = size;
    out.address = deviceAddress ? BufferAddress(out.buffer) : 0;
    diagnostic.clear();
    return true;
}

void PresentableTinyRtScene::DestroyBuffer(Buffer& buffer) const
{
    if (device_ == VK_NULL_HANDLE)
    {
        buffer = {};
        return;
    }
    if (buffer.buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device_, buffer.buffer, nullptr);
    }
    if (buffer.memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device_, buffer.memory, nullptr);
    }
    buffer = {};
}

VkDeviceAddress PresentableTinyRtScene::BufferAddress(VkBuffer buffer) const
{
    VkBufferDeviceAddressInfo addressInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    addressInfo.buffer = buffer;
    return vkGetBufferDeviceAddressKHR_(device_, &addressInfo);
}

bool PresentableTinyRtScene::RunOneTimeCommands(void (*record)(VkCommandBuffer, void*), void* userData, std::string& diagnostic) const
{
    VkCommandBufferAllocateInfo allocateInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        commandPool_,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1u};
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(device_, &allocateInfo, &commandBuffer) != VK_SUCCESS)
    {
        diagnostic = "Failed to allocate one-time RT command buffer.";
        return false;
    }

    VkCommandBufferBeginInfo beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr};
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        vkFreeCommandBuffers(device_, commandPool_, 1u, &commandBuffer);
        diagnostic = "Failed to begin one-time RT command buffer.";
        return false;
    }

    record(commandBuffer, userData);

    VkResult result = vkEndCommandBuffer(commandBuffer);
    if (result == VK_SUCCESS)
    {
        const VkSubmitInfo submitInfo{
            VK_STRUCTURE_TYPE_SUBMIT_INFO,
            nullptr,
            0u,
            nullptr,
            nullptr,
            1u,
            &commandBuffer,
            0u,
            nullptr};
        result = vkQueueSubmit(queue_, 1u, &submitInfo, VK_NULL_HANDLE);
    }
    if (result == VK_SUCCESS)
    {
        result = vkQueueWaitIdle(queue_);
    }

    vkFreeCommandBuffers(device_, commandPool_, 1u, &commandBuffer);
    if (result != VK_SUCCESS)
    {
        diagnostic = "Failed to submit one-time RT command buffer.";
        return false;
    }

    diagnostic.clear();
    return true;
}

bool PresentableTinyRtScene::CreateStorageImage(std::string& diagnostic)
{
    const VkImageCreateInfo imageInfo{
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr,
        0u,
        VK_IMAGE_TYPE_2D,
        kStorageImageFormat,
        {dispatchExtent_.width, dispatchExtent_.height, 1u},
        1u,
        1u,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0u,
        nullptr,
        VK_IMAGE_LAYOUT_UNDEFINED};
    if (vkCreateImage(device_, &imageInfo, nullptr, &storageImage_) != VK_SUCCESS)
    {
        diagnostic = "Failed to create RT storage image.";
        return false;
    }

    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device_, storageImage_, &requirements);
    const std::uint32_t memoryType = FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memoryType == UINT32_MAX)
    {
        diagnostic = "No compatible memory type for RT storage image.";
        return false;
    }

    const VkMemoryAllocateInfo allocateInfo{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        requirements.size,
        memoryType};
    if (vkAllocateMemory(device_, &allocateInfo, nullptr, &storageImageMemory_) != VK_SUCCESS ||
        vkBindImageMemory(device_, storageImage_, storageImageMemory_, 0u) != VK_SUCCESS)
    {
        diagnostic = "Failed to allocate RT storage image memory.";
        return false;
    }

    const VkImageViewCreateInfo viewInfo{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0u,
        storageImage_,
        VK_IMAGE_VIEW_TYPE_2D,
        kStorageImageFormat,
        {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u}};
    if (vkCreateImageView(device_, &viewInfo, nullptr, &storageImageView_) != VK_SUCCESS)
    {
        diagnostic = "Failed to create RT storage image view.";
        return false;
    }

    struct TransitionData
    {
        VkImage image;
    } data{storageImage_};
    const auto record = [](VkCommandBuffer commandBuffer, void* userData) {
        const auto* transition = static_cast<const TransitionData*>(userData);
        SetImageBarrier(commandBuffer,
                        transition->image,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_GENERAL,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                        0u,
                        VK_ACCESS_SHADER_WRITE_BIT);
    };
    if (!RunOneTimeCommands(record, &data, diagnostic))
    {
        return false;
    }

    storageImageLayout_ = VK_IMAGE_LAYOUT_GENERAL;
    diagnostic.clear();
    return true;
}

void PresentableTinyRtScene::DestroyTextureArray(TextureArray& texture)
{
    if (device_ != VK_NULL_HANDLE && texture.view != VK_NULL_HANDLE) vkDestroyImageView(device_, texture.view, nullptr);
    if (device_ != VK_NULL_HANDLE && texture.image != VK_NULL_HANDLE) vkDestroyImage(device_, texture.image, nullptr);
    if (device_ != VK_NULL_HANDLE && texture.memory != VK_NULL_HANDLE) vkFreeMemory(device_, texture.memory, nullptr);
    texture = {};
}

bool PresentableTinyRtScene::CreateTextureArray(const std::string& path, VkFormat format, TextureArray& texture, std::string& diagnostic)
{
    constexpr std::uint32_t width = 512u;
    constexpr std::uint32_t height = 512u;
    constexpr std::uint32_t layers = 5u;
    const bool ktx2 = path.ends_with(".ktx2");
    const VkDeviceSize layerByteSize = ktx2
        ? (format == VK_FORMAT_ASTC_4x4_UNORM_BLOCK || format == VK_FORMAT_ASTC_4x4_SRGB_BLOCK
               ? static_cast<VkDeviceSize>((width + 3u) / 4u) * ((height + 3u) / 4u) * 16u
               : static_cast<VkDeviceSize>((width + 5u) / 6u) * ((height + 5u) / 6u) * 16u)
        : static_cast<VkDeviceSize>(width) * height * 4u;
    const VkDeviceSize byteSize = layerByteSize * layers;
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
    {
        diagnostic = "PBR texture array is missing: " + path;
        return false;
    }
    const std::size_t fileSize = static_cast<std::size_t>(stream.tellg());
    stream.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> fileBytes(fileSize);
    if (!stream.read(reinterpret_cast<char*>(fileBytes.data()), static_cast<std::streamsize>(fileBytes.size())))
    {
        diagnostic = "Failed to read PBR texture array: " + path;
        return false;
    }
    std::vector<std::uint8_t> pixels;
    if (ktx2)
    {
        constexpr std::array<std::uint8_t, 12u> identifier{{0xABu, 0x4Bu, 0x54u, 0x58u, 0x20u, 0x32u, 0x30u, 0xBBu, 0x0Du, 0x0Au, 0x1Au, 0x0Au}};
        const auto read32 = [&fileBytes](std::size_t offset) {
            std::uint32_t value = 0u;
            if (offset + sizeof(value) <= fileBytes.size()) std::memcpy(&value, fileBytes.data() + offset, sizeof(value));
            return value;
        };
        const auto read64 = [&fileBytes](std::size_t offset) {
            std::uint64_t value = 0u;
            if (offset + sizeof(value) <= fileBytes.size()) std::memcpy(&value, fileBytes.data() + offset, sizeof(value));
            return value;
        };
        const std::uint64_t levelOffset = read64(80u);
        const std::uint64_t levelLength = read64(88u);
        const std::uint64_t levelUncompressedLength = read64(96u);
        const bool validHeader = fileBytes.size() >= 104u &&
                                 std::equal(identifier.begin(), identifier.end(), fileBytes.begin()) &&
                                 read32(12u) == static_cast<std::uint32_t>(format) && read32(16u) == 1u &&
                                 read32(20u) == width && read32(24u) == height && read32(28u) == 0u &&
                                 read32(32u) == layers && read32(36u) == 1u && read32(40u) == 1u &&
                                 read32(44u) == 0u && levelLength == byteSize &&
                                 levelUncompressedLength == byteSize && levelOffset <= fileBytes.size() &&
                                 levelLength <= fileBytes.size() - static_cast<std::size_t>(levelOffset);
        if (!validHeader)
        {
            diagnostic = "PBR KTX2 array has an unsupported header, format, or payload size: " + path;
            return false;
        }
        pixels.assign(fileBytes.begin() + static_cast<std::ptrdiff_t>(levelOffset),
                      fileBytes.begin() + static_cast<std::ptrdiff_t>(levelOffset + levelLength));
    }
    else
    {
        if (fileBytes.size() != static_cast<std::size_t>(byteSize))
        {
            diagnostic = "Raw PBR texture array has the wrong size: " + path;
            return false;
        }
        pixels = std::move(fileBytes);
    }

    Buffer staging;
    if (!CreateBuffer(byteSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false, staging, diagnostic)) return false;
    void* mapped = nullptr;
    if (vkMapMemory(device_, staging.memory, 0u, byteSize, 0u, &mapped) != VK_SUCCESS || mapped == nullptr)
    {
        DestroyBuffer(staging);
        diagnostic = "Failed to map PBR texture staging memory.";
        return false;
    }
    std::memcpy(mapped, pixels.data(), pixels.size());
    vkUnmapMemory(device_, staging.memory);

    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent = {width, height, 1u};
    imageInfo.mipLevels = 1u;
    imageInfo.arrayLayers = layers;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device_, &imageInfo, nullptr, &texture.image) != VK_SUCCESS)
    {
        DestroyBuffer(staging);
        diagnostic = "Failed to create PBR texture array image.";
        return false;
    }
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device_, texture.image, &requirements);
    const std::uint32_t memoryType = FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memoryType;
    if (memoryType == UINT32_MAX || vkAllocateMemory(device_, &allocation, nullptr, &texture.memory) != VK_SUCCESS || vkBindImageMemory(device_, texture.image, texture.memory, 0u) != VK_SUCCESS)
    {
        DestroyBuffer(staging);
        DestroyTextureArray(texture);
        diagnostic = "Failed to allocate PBR texture array memory.";
        return false;
    }
    struct UploadData
    {
        PresentableTinyRtScene* scene;
        Buffer* staging;
        TextureArray* texture;
        VkDeviceSize layerByteSize;
    } upload{this, &staging, &texture, layerByteSize};
    const auto recordUpload = [](VkCommandBuffer commandBuffer, void* userData) {
        auto* data = static_cast<UploadData*>(userData);
        VkImageMemoryBarrier toTransfer{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.image = data->texture->image;
        toTransfer.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 5u};
        toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, nullptr, 0u, nullptr, 1u, &toTransfer);
        std::array<VkBufferImageCopy, 5u> copies{};
        for (std::uint32_t layer = 0u; layer < copies.size(); ++layer)
        {
            copies[layer].bufferOffset = static_cast<VkDeviceSize>(layer) * data->layerByteSize;
            copies[layer].imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, layer, 1u};
            copies[layer].imageExtent = {512u, 512u, 1u};
        }
        vkCmdCopyBufferToImage(commandBuffer, data->staging->buffer, data->texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<std::uint32_t>(copies.size()), copies.data());
        VkImageMemoryBarrier toShader = toTransfer;
        toShader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toShader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        toShader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toShader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0u, 0u, nullptr, 0u, nullptr, 1u, &toShader);
    };
    const bool uploaded = RunOneTimeCommands(recordUpload, &upload, diagnostic);
    DestroyBuffer(staging);
    if (!uploaded) { DestroyTextureArray(texture); return false; }

    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = texture.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    viewInfo.format = format;
    viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, layers};
    if (vkCreateImageView(device_, &viewInfo, nullptr, &texture.view) != VK_SUCCESS)
    {
        DestroyTextureArray(texture);
        diagnostic = "Failed to create PBR texture array view.";
        return false;
    }
    return true;
}

bool PresentableTinyRtScene::SupportsTextureArrayFormat(VkFormat format) const
{
    VkFormatProperties formatProperties{};
    vkGetPhysicalDeviceFormatProperties(physicalDevice_, format, &formatProperties);
    constexpr VkFormatFeatureFlags required = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
                                               VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
                                               VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    if ((formatProperties.optimalTilingFeatures & required) != required)
    {
        return false;
    }
    VkImageFormatProperties imageProperties{};
    const VkResult result = vkGetPhysicalDeviceImageFormatProperties(physicalDevice_,
                                                                      format,
                                                                      VK_IMAGE_TYPE_2D,
                                                                      VK_IMAGE_TILING_OPTIMAL,
                                                                      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                                      0u,
                                                                      &imageProperties);
    return result == VK_SUCCESS && imageProperties.maxExtent.width >= 512u &&
           imageProperties.maxExtent.height >= 512u && imageProperties.maxArrayLayers >= 5u;
}

bool PresentableTinyRtScene::CreateMaterialTextures(const std::string& directory, std::string& diagnostic)
{
    const auto path = [&directory](const char* name) { return directory + "/" + name; };
    const bool astcSupported = SupportsTextureArrayFormat(VK_FORMAT_ASTC_6x6_SRGB_BLOCK) &&
                               SupportsTextureArrayFormat(VK_FORMAT_ASTC_4x4_UNORM_BLOCK) &&
                               SupportsTextureArrayFormat(VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
    const std::string compressedDiffuse = path("diff-array-512-astc6x6.ktx2");
    const std::string compressedNormal = path("normal-array-512-astc4x4.ktx2");
    const std::string compressedArm = path("arm-array-512-astc6x6.ktx2");
    const bool compressedAssetsPresent = std::ifstream(compressedDiffuse, std::ios::binary).good() &&
                                         std::ifstream(compressedNormal, std::ios::binary).good() &&
                                         std::ifstream(compressedArm, std::ios::binary).good();
    if (astcSupported && compressedAssetsPresent)
    {
        if (!CreateTextureArray(compressedDiffuse, VK_FORMAT_ASTC_6x6_SRGB_BLOCK, materialDiffuse_, diagnostic) ||
            !CreateTextureArray(compressedNormal, VK_FORMAT_ASTC_4x4_UNORM_BLOCK, materialNormal_, diagnostic) ||
            !CreateTextureArray(compressedArm, VK_FORMAT_ASTC_6x6_UNORM_BLOCK, materialArm_, diagnostic)) return false;
        materialEncoding_ = "ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2)";
    }
    else
    {
        const std::string rawDiffuse = path("diff-array-512.rgba");
        const std::string rawNormal = path("normal-array-512.rgba");
        const std::string rawArm = path("arm-array-512.rgba");
        const bool rawAssetsPresent = std::ifstream(rawDiffuse, std::ios::binary).good() &&
                                      std::ifstream(rawNormal, std::ios::binary).good() &&
                                      std::ifstream(rawArm, std::ios::binary).good();
        if (!rawAssetsPresent)
        {
            diagnostic = astcSupported
                ? "ASTC texture arrays are missing and no raw fallback is packaged."
                : "ASTC LDR sampled-array linear filtering is unsupported and no raw mobile fallback is packaged.";
            return false;
        }
        if (!CreateTextureArray(rawDiffuse, VK_FORMAT_R8G8B8A8_SRGB, materialDiffuse_, diagnostic) ||
            !CreateTextureArray(rawNormal, VK_FORMAT_R8G8B8A8_UNORM, materialNormal_, diagnostic) ||
            !CreateTextureArray(rawArm, VK_FORMAT_R8G8B8A8_UNORM, materialArm_, diagnostic)) return false;
        materialEncoding_ = "RGBA8 raw fallback";
    }
    VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.maxAnisotropy = 1.0f;
    if (vkCreateSampler(device_, &sampler, nullptr, &materialSampler_) != VK_SUCCESS)
    {
        diagnostic = "Failed to create PBR material sampler.";
        return false;
    }
    return true;
}

void PresentableTinyRtScene::DestroyAccelerationStructure(AccelerationStructure& accelerationStructure)
{
    if (accelerationStructure.handle != VK_NULL_HANDLE && vkDestroyAccelerationStructureKHR_)
    {
        vkDestroyAccelerationStructureKHR_(device_, accelerationStructure.handle, nullptr);
    }
    DestroyBuffer(accelerationStructure.backing);
    accelerationStructure = {};
}

bool PresentableTinyRtScene::BuildAccelerationStructures(std::string& diagnostic)
{
    struct Vertex
    {
        float position[3];
    };
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<std::uint32_t> worldSurfaceCodes;
    vertices.reserve(512u);
    indices.reserve(768u);
    worldSurfaceCodes.reserve(96u);

    enum SurfaceMaterial : std::uint32_t
    {
        SurfaceDryStone = 0u,
        SurfaceWetCobble = 1u,
        SurfaceMossyStone = 2u,
        SurfaceDampGround = 3u,
        SurfaceAgedMetal = 4u,
        SurfaceFlame = 5u,
        SurfaceDarkFigure = 6u,
        SurfaceHiddenShell = 7u,
        SurfaceMirror = 8u,
        SurfaceClearGlass = 9u,
        SurfaceStainedGlass = 10u,
    };
    enum SurfaceNormal : std::uint32_t
    {
        SurfaceUp = 0u,
        SurfaceDown = 1u,
        SurfaceRight = 2u,
        SurfaceLeft = 3u,
        SurfaceForward = 4u,
        SurfaceBack = 5u,
        SurfaceGalleryCant = 6u,
    };
    const auto surfaceCode = [](SurfaceMaterial material, SurfaceNormal normal) {
        return static_cast<std::uint32_t>(material) | (static_cast<std::uint32_t>(normal) << 8u);
    };

    const auto addQuad = [&vertices, &indices](const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& d) {
        const std::uint32_t base = static_cast<std::uint32_t>(vertices.size());
        vertices.push_back(a);
        vertices.push_back(b);
        vertices.push_back(c);
        vertices.push_back(d);
        indices.insert(indices.end(), {base, base + 1u, base + 2u, base, base + 2u, base + 3u});
    };
    const auto addTriangle = [&vertices, &indices](const Vertex& a, const Vertex& b, const Vertex& c) {
        const std::uint32_t base = static_cast<std::uint32_t>(vertices.size());
        vertices.push_back(a);
        vertices.push_back(b);
        vertices.push_back(c);
        indices.insert(indices.end(), {base, base + 1u, base + 2u});
    };
    const auto addWorldQuad = [&addQuad, &worldSurfaceCodes, &surfaceCode](const Vertex& a,
                                                                          const Vertex& b,
                                                                          const Vertex& c,
                                                                          const Vertex& d,
                                                                          SurfaceMaterial material,
                                                                          SurfaceNormal normal) {
        addQuad(a, b, c, d);
        const std::uint32_t code = surfaceCode(material, normal);
        worldSurfaceCodes.push_back(code);
        worldSurfaceCodes.push_back(code);
    };
    const auto addBox = [&addQuad](float minX, float minY, float minZ, float maxX, float maxY, float maxZ) {
        // Fixed face order for the raygen material normal lookup:
        // -Z, +Z, -X, +X, +Y, -Y (two triangles per face).
        addQuad({{minX, minY, minZ}}, {{minX, maxY, minZ}}, {{maxX, maxY, minZ}}, {{maxX, minY, minZ}});
        addQuad({{maxX, minY, maxZ}}, {{maxX, maxY, maxZ}}, {{minX, maxY, maxZ}}, {{minX, minY, maxZ}});
        addQuad({{minX, minY, maxZ}}, {{minX, maxY, maxZ}}, {{minX, maxY, minZ}}, {{minX, minY, minZ}});
        addQuad({{maxX, minY, minZ}}, {{maxX, maxY, minZ}}, {{maxX, maxY, maxZ}}, {{maxX, minY, maxZ}});
        addQuad({{minX, maxY, minZ}}, {{minX, maxY, maxZ}}, {{maxX, maxY, maxZ}}, {{maxX, maxY, minZ}});
        addQuad({{minX, minY, maxZ}}, {{minX, minY, minZ}}, {{maxX, minY, minZ}}, {{maxX, minY, maxZ}});
    };

    addWorldQuad({{-1.85f, -0.95f, 3.4f}}, {{1.85f, -0.95f, 3.4f}}, {{1.85f, -0.95f, -6.4f}}, {{-1.85f, -0.95f, -6.4f}}, SurfaceWetCobble, SurfaceUp);
    addWorldQuad({{-1.85f, 1.35f, 3.4f}}, {{-1.85f, 1.35f, -0.2f}}, {{1.85f, 1.35f, -0.2f}}, {{1.85f, 1.35f, 3.4f}}, SurfaceDryStone, SurfaceDown);
    addWorldQuad({{-1.85f, -0.95f, 3.4f}}, {{-1.85f, -0.95f, -6.4f}}, {{-1.85f, 1.35f, -6.4f}}, {{-1.85f, 1.35f, 3.4f}}, SurfaceMossyStone, SurfaceRight);
    addWorldQuad({{1.85f, -0.95f, -6.4f}}, {{1.85f, -0.95f, 3.4f}}, {{1.85f, 1.35f, 3.4f}}, {{1.85f, 1.35f, -6.4f}}, SurfaceMossyStone, SurfaceLeft);
    addWorldQuad({{-1.85f, -0.95f, -6.4f}}, {{1.85f, -0.95f, -6.4f}}, {{1.85f, 1.35f, -6.4f}}, {{-1.85f, 1.35f, -6.4f}}, SurfaceMossyStone, SurfaceForward);
    addWorldQuad({{-1.2f, -0.95f, -3.4f}}, {{-0.78f, -0.95f, -3.4f}}, {{-0.78f, 0.95f, -3.4f}}, {{-1.2f, 0.95f, -3.4f}}, SurfaceMossyStone, SurfaceForward);
    addWorldQuad({{0.78f, -0.95f, -3.4f}}, {{1.2f, -0.95f, -3.4f}}, {{1.2f, 0.95f, -3.4f}}, {{0.78f, 0.95f, -3.4f}}, SurfaceMossyStone, SurfaceForward);
    addWorldQuad({{-1.2f, 0.78f, -3.4f}}, {{1.2f, 0.78f, -3.4f}}, {{1.2f, 1.18f, -3.4f}}, {{-1.2f, 1.18f, -3.4f}}, SurfaceMossyStone, SurfaceForward);
    addWorldQuad({{-1.86f, -0.28f, 1.12f}}, {{-1.86f, 0.46f, 1.12f}}, {{-1.86f, 0.46f, 0.62f}}, {{-1.86f, -0.28f, 0.62f}}, SurfaceFlame, SurfaceRight);
    addWorldQuad({{1.86f, -0.35f, -1.98f}}, {{1.86f, -0.35f, -1.48f}}, {{1.86f, 0.38f, -1.48f}}, {{1.86f, 0.38f, -1.98f}}, SurfaceFlame, SurfaceLeft);
    addWorldQuad({{-1.84f, -0.32f, -0.82f}}, {{-1.84f, 0.24f, -0.62f}}, {{-1.84f, 0.42f, -1.12f}}, {{-1.84f, -0.12f, -1.34f}}, SurfaceMirror, SurfaceRight);
    addWorldQuad({{1.84f, -0.44f, 0.24f}}, {{1.84f, -0.02f, 0.5f}}, {{1.84f, 0.28f, 0.1f}}, {{1.84f, -0.18f, -0.2f}}, SurfaceMirror, SurfaceLeft);
    addWorldQuad({{-0.52f, -0.94f, -0.86f}}, {{0.34f, -0.94f, -0.64f}}, {{0.64f, -0.94f, -1.18f}}, {{-0.38f, -0.94f, -1.42f}}, SurfaceAgedMetal, SurfaceUp);
    // Four physical roof slabs surround one broken opening in room two. The
    // moon query sees this real breach, producing one composed floor/wall patch
    // instead of several ruler-straight stripes across the entire room.
    addWorldQuad({{-1.85f, 1.35f, -0.2f}}, {{-1.85f, 1.35f, -6.4f}}, {{-0.72f, 1.35f, -5.20f}}, {{-0.55f, 1.35f, -3.45f}}, SurfaceDryStone, SurfaceDown);
    addWorldQuad({{0.32f, 1.35f, -3.55f}}, {{0.62f, 1.35f, -5.05f}}, {{1.85f, 1.35f, -6.4f}}, {{1.85f, 1.35f, -0.2f}}, SurfaceDryStone, SurfaceDown);
    addWorldQuad({{-1.85f, 1.35f, -0.2f}}, {{-0.55f, 1.35f, -3.45f}}, {{0.32f, 1.35f, -3.55f}}, {{1.85f, 1.35f, -0.2f}}, SurfaceDryStone, SurfaceDown);
    addWorldQuad({{-0.72f, 1.35f, -5.20f}}, {{-1.85f, 1.35f, -6.4f}}, {{1.85f, 1.35f, -6.4f}}, {{0.62f, 1.35f, -5.05f}}, SurfaceDryStone, SurfaceDown);
    for (std::uint32_t i = 0u; i < 8u; ++i)
    {
        const float x = -1.05f + static_cast<float>(i % 4u) * 0.7f + (i >= 4u ? 0.18f : 0.0f);
        const float z = -2.55f - static_cast<float>(i / 4u) * 1.1f - static_cast<float>(i % 2u) * 0.24f;
        const float h = 0.58f + static_cast<float>(i % 3u) * 0.12f;
        addWorldQuad({{x - 0.18f, -0.95f, z}}, {{x + 0.18f, -0.95f, z}}, {{x + 0.14f, -0.95f + h, z}}, {{x - 0.14f, -0.95f + h, z}}, SurfaceDarkFigure, SurfaceForward);
    }

    // A shallow gallery table runs along the left wall, leaving the central
    // lane open. Five canted swatches reuse the existing ASTC material layers.
    constexpr float galleryMinX = -1.55f;
    constexpr float galleryMaxX = -0.72f;
    constexpr float galleryMinY = -0.95f;
    constexpr float galleryMaxY = -0.58f;
    constexpr float galleryMinZ = 0.05f;
    constexpr float galleryMaxZ = 2.35f;
    addWorldQuad({{galleryMinX, galleryMinY, galleryMinZ}}, {{galleryMinX, galleryMaxY, galleryMinZ}}, {{galleryMaxX, galleryMaxY, galleryMinZ}}, {{galleryMaxX, galleryMinY, galleryMinZ}}, SurfaceDryStone, SurfaceBack);
    addWorldQuad({{galleryMaxX, galleryMinY, galleryMaxZ}}, {{galleryMaxX, galleryMaxY, galleryMaxZ}}, {{galleryMinX, galleryMaxY, galleryMaxZ}}, {{galleryMinX, galleryMinY, galleryMaxZ}}, SurfaceDryStone, SurfaceForward);
    addWorldQuad({{galleryMinX, galleryMinY, galleryMaxZ}}, {{galleryMinX, galleryMaxY, galleryMaxZ}}, {{galleryMinX, galleryMaxY, galleryMinZ}}, {{galleryMinX, galleryMinY, galleryMinZ}}, SurfaceDryStone, SurfaceLeft);
    addWorldQuad({{galleryMaxX, galleryMinY, galleryMinZ}}, {{galleryMaxX, galleryMaxY, galleryMinZ}}, {{galleryMaxX, galleryMaxY, galleryMaxZ}}, {{galleryMaxX, galleryMinY, galleryMaxZ}}, SurfaceDryStone, SurfaceRight);
    addWorldQuad({{galleryMinX, galleryMaxY, galleryMinZ}}, {{galleryMinX, galleryMaxY, galleryMaxZ}}, {{galleryMaxX, galleryMaxY, galleryMaxZ}}, {{galleryMaxX, galleryMaxY, galleryMinZ}}, SurfaceDryStone, SurfaceUp);
    addWorldQuad({{galleryMinX, galleryMinY, galleryMaxZ}}, {{galleryMinX, galleryMinY, galleryMinZ}}, {{galleryMaxX, galleryMinY, galleryMinZ}}, {{galleryMaxX, galleryMinY, galleryMaxZ}}, SurfaceDryStone, SurfaceDown);
    const std::array<SurfaceMaterial, 5u> galleryMaterials{{SurfaceDryStone, SurfaceWetCobble, SurfaceMossyStone, SurfaceDampGround, SurfaceAgedMetal}};
    for (std::size_t i = 0u; i < galleryMaterials.size(); ++i)
    {
        const float z = 2.05f - static_cast<float>(i) * 0.43f;
        addWorldQuad({{-0.80f, -0.54f, z + 0.16f}}, {{-0.80f, -0.54f, z - 0.16f}}, {{-1.22f, -0.08f, z - 0.16f}}, {{-1.22f, -0.08f, z + 0.16f}}, galleryMaterials[i], SurfaceGalleryCant);
    }

    // A thin hidden shell behind the zero-thickness room planes catches rays
    // that start near a join and skip the adjoining face because of ray tMin.
    // It stops at the open entrance and leaves the room-two roof breach
    // unobstructed. Appending it here preserves every existing material index.
    addWorldQuad({{-1.92f, -1.02f, 3.4f}}, {{-1.92f, -1.02f, -6.47f}}, {{-1.92f, 1.42f, -6.47f}}, {{-1.92f, 1.42f, 3.4f}}, SurfaceHiddenShell, SurfaceRight);
    addWorldQuad({{1.92f, -1.02f, -6.47f}}, {{1.92f, -1.02f, 3.4f}}, {{1.92f, 1.42f, 3.4f}}, {{1.92f, 1.42f, -6.47f}}, SurfaceHiddenShell, SurfaceLeft);
    addWorldQuad({{-1.92f, -1.02f, 3.4f}}, {{1.92f, -1.02f, 3.4f}}, {{1.92f, -1.02f, -6.47f}}, {{-1.92f, -1.02f, -6.47f}}, SurfaceHiddenShell, SurfaceUp);
    addWorldQuad({{-1.92f, -1.02f, -6.47f}}, {{1.92f, -1.02f, -6.47f}}, {{1.92f, 1.42f, -6.47f}}, {{-1.92f, 1.42f, -6.47f}}, SurfaceHiddenShell, SurfaceForward);

    const std::uint32_t sceneIndexCount = static_cast<std::uint32_t>(indices.size());
    if (worldSurfaceCodes.size() != sceneIndexCount / 3u)
    {
        diagnostic = "World surface metadata does not match the world triangle count.";
        return false;
    }

    // The camera-held props share upload buffers but use separate BLAS instances
    // so the sword can swing without moving the torch or its light estimate.
    addQuad({{-0.045f, -0.42f, 0.0f}}, {{0.045f, -0.42f, 0.0f}}, {{0.045f, 0.10f, 0.0f}}, {{-0.045f, 0.10f, 0.0f}});
    addTriangle({{-0.13f, 0.08f, 0.0f}}, {{0.13f, 0.08f, 0.0f}}, {{0.0f, 0.52f, 0.0f}});
    addTriangle({{0.0f, 0.08f, -0.13f}}, {{0.0f, 0.08f, 0.13f}}, {{0.0f, 0.52f, 0.0f}});
    const std::uint32_t torchIndexCount = static_cast<std::uint32_t>(indices.size());

    // Low-poly player sword proof, angled inward from the right hand. The
    // textured 12k LOD replaces this when static GLB/PBR upload is available.
    constexpr float swordZ = 0.025f;
    addQuad({{1.43f, -0.66f, swordZ}}, {{1.51f, -0.66f, swordZ}}, {{1.51f, -0.31f, swordZ}}, {{1.43f, -0.31f, swordZ}});
    addQuad({{1.18f, -0.34f, swordZ}}, {{1.72f, -0.34f, swordZ}}, {{1.70f, -0.25f, swordZ}}, {{1.20f, -0.25f, swordZ}});
    addQuad({{1.37f, -0.26f, swordZ}}, {{1.51f, -0.26f, swordZ}}, {{1.14f, 1.12f, swordZ}}, {{1.04f, 1.09f, swordZ}});
    addTriangle({{1.04f, 1.09f, swordZ}}, {{1.14f, 1.12f, swordZ}}, {{1.00f, 1.34f, swordZ}});
    addQuad({{1.51f, -0.66f, -swordZ}}, {{1.43f, -0.66f, -swordZ}}, {{1.43f, -0.31f, -swordZ}}, {{1.51f, -0.31f, -swordZ}});
    addQuad({{1.72f, -0.34f, -swordZ}}, {{1.18f, -0.34f, -swordZ}}, {{1.20f, -0.25f, -swordZ}}, {{1.70f, -0.25f, -swordZ}});
    addQuad({{1.51f, -0.26f, -swordZ}}, {{1.37f, -0.26f, -swordZ}}, {{1.04f, 1.09f, -swordZ}}, {{1.14f, 1.12f, -swordZ}});
    addTriangle({{1.14f, 1.12f, -swordZ}}, {{1.04f, 1.09f, -swordZ}}, {{1.00f, 1.34f, -swordZ}});

    const std::uint32_t swordIndexCount = static_cast<std::uint32_t>(indices.size());

    // The torso remains in body/world space. Arms are articulated separately
    // from one reusable unit limb BLAS below, following the first-person view.
    addBox(-0.23f, -0.94f, -0.12f, 0.23f, -0.40f, 0.14f);  // tunic torso
    const std::uint32_t playerBodyIndexCount = static_cast<std::uint32_t>(indices.size());

    // Unit square prism along +Z. Four TLAS instances scale/rotate this one
    // BLAS into the upper/lower segments produced by the two-bone IK solve.
    addBox(-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    const std::uint32_t torchPrimitiveCount = (torchIndexCount - sceneIndexCount) / 3u;
    const std::uint32_t swordPrimitiveCount = (swordIndexCount - torchIndexCount) / 3u;
    const std::uint32_t playerBodyPrimitiveCount = (playerBodyIndexCount - swordIndexCount) / 3u;
    const std::uint32_t playerLimbPrimitiveCount = (static_cast<std::uint32_t>(indices.size()) - playerBodyIndexCount) / 3u;
    const VkTransformMatrixKHR transform{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f}};

    const VkMemoryPropertyFlags uploadMemory = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    const VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
    const VkDeviceSize indexBufferSize = sizeof(std::uint32_t) * indices.size();
    const VkDeviceSize worldSurfaceBufferSize = sizeof(std::uint32_t) * worldSurfaceCodes.size();
    if (!CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, uploadMemory, true, vertexBuffer_, diagnostic) ||
        !CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, uploadMemory, true, indexBuffer_, diagnostic) ||
        !CreateBuffer(sizeof(transform), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, uploadMemory, true, transformBuffer_, diagnostic) ||
        !CreateBuffer(worldSurfaceBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, uploadMemory, false, worldSurfaceBuffer_, diagnostic))
    {
        return false;
    }

    void* mapped = nullptr;
    vkMapMemory(device_, vertexBuffer_.memory, 0u, vertexBufferSize, 0u, &mapped);
    std::memcpy(mapped, vertices.data(), static_cast<std::size_t>(vertexBufferSize));
    vkUnmapMemory(device_, vertexBuffer_.memory);
    vkMapMemory(device_, indexBuffer_.memory, 0u, indexBufferSize, 0u, &mapped);
    std::memcpy(mapped, indices.data(), static_cast<std::size_t>(indexBufferSize));
    vkUnmapMemory(device_, indexBuffer_.memory);
    vkMapMemory(device_, transformBuffer_.memory, 0u, sizeof(transform), 0u, &mapped);
    std::memcpy(mapped, &transform, sizeof(transform));
    vkUnmapMemory(device_, transformBuffer_.memory);
    vkMapMemory(device_, worldSurfaceBuffer_.memory, 0u, worldSurfaceBufferSize, 0u, &mapped);
    std::memcpy(mapped, worldSurfaceCodes.data(), static_cast<std::size_t>(worldSurfaceBufferSize));
    vkUnmapMemory(device_, worldSurfaceBuffer_.memory);

    VkAccelerationStructureGeometryKHR blasGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    blasGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    blasGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    blasGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    blasGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    blasGeometry.geometry.triangles.vertexData.deviceAddress = vertexBuffer_.address;
    blasGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
    blasGeometry.geometry.triangles.maxVertex = static_cast<std::uint32_t>(vertices.size() - 1u);
    blasGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    blasGeometry.geometry.triangles.indexData.deviceAddress = indexBuffer_.address;
    blasGeometry.geometry.triangles.transformData.deviceAddress = transformBuffer_.address;

    VkAccelerationStructureBuildGeometryInfoKHR blasBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    blasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    blasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    blasBuildInfo.geometryCount = 1u;
    blasBuildInfo.pGeometries = &blasGeometry;

    std::uint32_t primitiveCount = sceneIndexCount / 3u;
    VkAccelerationStructureBuildSizesInfoKHR blasSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &blasBuildInfo, &primitiveCount, &blasSizes);

    if (!CreateBuffer(blasSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      blas_.backing,
                      diagnostic))
    {
        return false;
    }

    VkAccelerationStructureCreateInfoKHR blasCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    blasCreateInfo.buffer = blas_.backing.buffer;
    blasCreateInfo.size = blasSizes.accelerationStructureSize;
    blasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &blasCreateInfo, nullptr, &blas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create BLAS.";
        return false;
    }

    Buffer blasScratch;
    if (!CreateBuffer(blasSizes.buildScratchSize,
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      blasScratch,
                      diagnostic))
    {
        return false;
    }

    VkAccelerationStructureBuildRangeInfoKHR blasRange{};
    blasRange.primitiveCount = primitiveCount;
    blasBuildInfo.dstAccelerationStructure = blas_.handle;
    blasBuildInfo.scratchData.deviceAddress = blasScratch.address;
    const VkAccelerationStructureBuildRangeInfoKHR* blasRanges[] = {&blasRange};
    struct BlasBuildData
    {
        PresentableTinyRtScene* scene;
        VkAccelerationStructureBuildGeometryInfoKHR* buildInfo;
        const VkAccelerationStructureBuildRangeInfoKHR** ranges;
    } blasBuildData{this, &blasBuildInfo, blasRanges};
    const auto buildBlas = [](VkCommandBuffer commandBuffer, void* userData) {
        auto* buildData = static_cast<BlasBuildData*>(userData);
        buildData->scene->vkCmdBuildAccelerationStructuresKHR_(commandBuffer, 1u, buildData->buildInfo, buildData->ranges);
    };
    if (!RunOneTimeCommands(buildBlas, &blasBuildData, diagnostic))
    {
        DestroyBuffer(blasScratch);
        return false;
    }
    DestroyBuffer(blasScratch);

    VkAccelerationStructureDeviceAddressInfoKHR blasAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    blasAddressInfo.accelerationStructure = blas_.handle;
    blas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &blasAddressInfo);

    VkAccelerationStructureBuildRangeInfoKHR torchRange{};
    torchRange.primitiveCount = torchPrimitiveCount;
    torchRange.primitiveOffset = sceneIndexCount * sizeof(std::uint32_t);
    // addQuad writes absolute indices into the shared vertex buffer, so no vertex offset belongs here.
    torchRange.firstVertex = 0u;

    VkAccelerationStructureBuildGeometryInfoKHR torchBlasBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    torchBlasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    torchBlasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    torchBlasBuildInfo.geometryCount = 1u;
    torchBlasBuildInfo.pGeometries = &blasGeometry;
    VkAccelerationStructureBuildSizesInfoKHR torchBlasSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &torchBlasBuildInfo, &torchPrimitiveCount, &torchBlasSizes);
    if (!CreateBuffer(torchBlasSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      torchBlas_.backing,
                      diagnostic))
    {
        return false;
    }

    VkAccelerationStructureCreateInfoKHR torchBlasCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    torchBlasCreateInfo.buffer = torchBlas_.backing.buffer;
    torchBlasCreateInfo.size = torchBlasSizes.accelerationStructureSize;
    torchBlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &torchBlasCreateInfo, nullptr, &torchBlas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create held-torch BLAS.";
        return false;
    }

    Buffer torchBlasScratch;
    if (!CreateBuffer(torchBlasSizes.buildScratchSize,
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      torchBlasScratch,
                      diagnostic))
    {
        return false;
    }
    torchBlasBuildInfo.dstAccelerationStructure = torchBlas_.handle;
    torchBlasBuildInfo.scratchData.deviceAddress = torchBlasScratch.address;
    const VkAccelerationStructureBuildRangeInfoKHR* torchBlasRanges[] = {&torchRange};
    BlasBuildData torchBlasBuildData{this, &torchBlasBuildInfo, torchBlasRanges};
    if (!RunOneTimeCommands(buildBlas, &torchBlasBuildData, diagnostic))
    {
        DestroyBuffer(torchBlasScratch);
        return false;
    }
    DestroyBuffer(torchBlasScratch);

    VkAccelerationStructureDeviceAddressInfoKHR torchBlasAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    torchBlasAddressInfo.accelerationStructure = torchBlas_.handle;
    torchBlas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &torchBlasAddressInfo);

    VkAccelerationStructureBuildRangeInfoKHR swordRange{};
    swordRange.primitiveCount = swordPrimitiveCount;
    swordRange.primitiveOffset = torchIndexCount * sizeof(std::uint32_t);
    swordRange.firstVertex = 0u;
    VkAccelerationStructureBuildGeometryInfoKHR swordBlasBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    swordBlasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    swordBlasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    swordBlasBuildInfo.geometryCount = 1u;
    swordBlasBuildInfo.pGeometries = &blasGeometry;
    VkAccelerationStructureBuildSizesInfoKHR swordBlasSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &swordBlasBuildInfo, &swordPrimitiveCount, &swordBlasSizes);
    if (!CreateBuffer(swordBlasSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      swordBlas_.backing,
                      diagnostic))
    {
        return false;
    }
    VkAccelerationStructureCreateInfoKHR swordBlasCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    swordBlasCreateInfo.buffer = swordBlas_.backing.buffer;
    swordBlasCreateInfo.size = swordBlasSizes.accelerationStructureSize;
    swordBlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &swordBlasCreateInfo, nullptr, &swordBlas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create held-sword BLAS.";
        return false;
    }
    Buffer swordBlasScratch;
    if (!CreateBuffer(swordBlasSizes.buildScratchSize,
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      swordBlasScratch,
                      diagnostic))
    {
        return false;
    }
    swordBlasBuildInfo.dstAccelerationStructure = swordBlas_.handle;
    swordBlasBuildInfo.scratchData.deviceAddress = swordBlasScratch.address;
    const VkAccelerationStructureBuildRangeInfoKHR* swordBlasRanges[] = {&swordRange};
    BlasBuildData swordBlasBuildData{this, &swordBlasBuildInfo, swordBlasRanges};
    if (!RunOneTimeCommands(buildBlas, &swordBlasBuildData, diagnostic))
    {
        DestroyBuffer(swordBlasScratch);
        return false;
    }
    DestroyBuffer(swordBlasScratch);
    VkAccelerationStructureDeviceAddressInfoKHR swordBlasAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    swordBlasAddressInfo.accelerationStructure = swordBlas_.handle;
    swordBlas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &swordBlasAddressInfo);

    VkAccelerationStructureBuildRangeInfoKHR playerBodyRange{};
    playerBodyRange.primitiveCount = playerBodyPrimitiveCount;
    playerBodyRange.primitiveOffset = swordIndexCount * sizeof(std::uint32_t);
    playerBodyRange.firstVertex = 0u;
    VkAccelerationStructureBuildGeometryInfoKHR playerBodyBlasBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    playerBodyBlasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    playerBodyBlasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    playerBodyBlasBuildInfo.geometryCount = 1u;
    playerBodyBlasBuildInfo.pGeometries = &blasGeometry;
    VkAccelerationStructureBuildSizesInfoKHR playerBodyBlasSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &playerBodyBlasBuildInfo, &playerBodyPrimitiveCount, &playerBodyBlasSizes);
    if (!CreateBuffer(playerBodyBlasSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      playerBodyBlas_.backing,
                      diagnostic))
    {
        return false;
    }
    VkAccelerationStructureCreateInfoKHR playerBodyBlasCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    playerBodyBlasCreateInfo.buffer = playerBodyBlas_.backing.buffer;
    playerBodyBlasCreateInfo.size = playerBodyBlasSizes.accelerationStructureSize;
    playerBodyBlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &playerBodyBlasCreateInfo, nullptr, &playerBodyBlas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create first-person player-body BLAS.";
        return false;
    }
    Buffer playerBodyBlasScratch;
    if (!CreateBuffer(playerBodyBlasSizes.buildScratchSize,
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      playerBodyBlasScratch,
                      diagnostic))
    {
        return false;
    }
    playerBodyBlasBuildInfo.dstAccelerationStructure = playerBodyBlas_.handle;
    playerBodyBlasBuildInfo.scratchData.deviceAddress = playerBodyBlasScratch.address;
    const VkAccelerationStructureBuildRangeInfoKHR* playerBodyBlasRanges[] = {&playerBodyRange};
    BlasBuildData playerBodyBlasBuildData{this, &playerBodyBlasBuildInfo, playerBodyBlasRanges};
    if (!RunOneTimeCommands(buildBlas, &playerBodyBlasBuildData, diagnostic))
    {
        DestroyBuffer(playerBodyBlasScratch);
        return false;
    }
    DestroyBuffer(playerBodyBlasScratch);
    VkAccelerationStructureDeviceAddressInfoKHR playerBodyBlasAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    playerBodyBlasAddressInfo.accelerationStructure = playerBodyBlas_.handle;
    playerBodyBlas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &playerBodyBlasAddressInfo);

    VkAccelerationStructureBuildRangeInfoKHR playerLimbRange{};
    playerLimbRange.primitiveCount = playerLimbPrimitiveCount;
    playerLimbRange.primitiveOffset = playerBodyIndexCount * sizeof(std::uint32_t);
    playerLimbRange.firstVertex = 0u;
    VkAccelerationStructureBuildGeometryInfoKHR playerLimbBlasBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    playerLimbBlasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    playerLimbBlasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    playerLimbBlasBuildInfo.geometryCount = 1u;
    playerLimbBlasBuildInfo.pGeometries = &blasGeometry;
    VkAccelerationStructureBuildSizesInfoKHR playerLimbBlasSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &playerLimbBlasBuildInfo, &playerLimbPrimitiveCount, &playerLimbBlasSizes);
    if (!CreateBuffer(playerLimbBlasSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      playerLimbBlas_.backing,
                      diagnostic))
    {
        return false;
    }
    VkAccelerationStructureCreateInfoKHR playerLimbBlasCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    playerLimbBlasCreateInfo.buffer = playerLimbBlas_.backing.buffer;
    playerLimbBlasCreateInfo.size = playerLimbBlasSizes.accelerationStructureSize;
    playerLimbBlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &playerLimbBlasCreateInfo, nullptr, &playerLimbBlas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create reusable first-person limb BLAS.";
        return false;
    }
    Buffer playerLimbBlasScratch;
    if (!CreateBuffer(playerLimbBlasSizes.buildScratchSize,
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      playerLimbBlasScratch,
                      diagnostic))
    {
        return false;
    }
    playerLimbBlasBuildInfo.dstAccelerationStructure = playerLimbBlas_.handle;
    playerLimbBlasBuildInfo.scratchData.deviceAddress = playerLimbBlasScratch.address;
    const VkAccelerationStructureBuildRangeInfoKHR* playerLimbBlasRanges[] = {&playerLimbRange};
    BlasBuildData playerLimbBlasBuildData{this, &playerLimbBlasBuildInfo, playerLimbBlasRanges};
    if (!RunOneTimeCommands(buildBlas, &playerLimbBlasBuildData, diagnostic))
    {
        DestroyBuffer(playerLimbBlasScratch);
        return false;
    }
    DestroyBuffer(playerLimbBlasScratch);
    VkAccelerationStructureDeviceAddressInfoKHR playerLimbBlasAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    playerLimbBlasAddressInfo.accelerationStructure = playerLimbBlas_.handle;
    playerLimbBlas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &playerLimbBlasAddressInfo);

    if (!skeletonModel_.Skin(horde::scene::SkeletonClip::Idle, 0.0f, skeletonSkinnedVertices_, diagnostic) || skeletonSkinnedVertices_.empty())
    {
        if (diagnostic.empty()) diagnostic = "Skeleton produced no skinned vertices.";
        return false;
    }
    const VkDeviceSize skeletonVertexBufferSize = sizeof(horde::scene::SkinnedRtVertex) * skeletonSkinnedVertices_.size();
    if (!CreateBuffer(skeletonVertexBufferSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      uploadMemory,
                      true,
                      skeletonVertexBuffer_,
                      diagnostic))
    {
        return false;
    }
    vkMapMemory(device_, skeletonVertexBuffer_.memory, 0u, skeletonVertexBufferSize, 0u, &mapped);
    std::memcpy(mapped, skeletonSkinnedVertices_.data(), static_cast<std::size_t>(skeletonVertexBufferSize));
    vkUnmapMemory(device_, skeletonVertexBuffer_.memory);

    VkAccelerationStructureGeometryKHR skeletonGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    skeletonGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    skeletonGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    skeletonGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    skeletonGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    skeletonGeometry.geometry.triangles.vertexData.deviceAddress = skeletonVertexBuffer_.address;
    skeletonGeometry.geometry.triangles.vertexStride = sizeof(horde::scene::SkinnedRtVertex);
    skeletonGeometry.geometry.triangles.maxVertex = static_cast<std::uint32_t>(skeletonSkinnedVertices_.size() - 1u);
    skeletonGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
    skeletonGeometry.geometry.triangles.transformData.deviceAddress = 0u;
    const std::uint32_t skeletonPrimitiveCount = static_cast<std::uint32_t>(skeletonSkinnedVertices_.size() / 3u);
    VkAccelerationStructureBuildGeometryInfoKHR skeletonBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    skeletonBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    skeletonBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    skeletonBuildInfo.geometryCount = 1u;
    skeletonBuildInfo.pGeometries = &skeletonGeometry;
    VkAccelerationStructureBuildSizesInfoKHR skeletonSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &skeletonBuildInfo, &skeletonPrimitiveCount, &skeletonSizes);
    if (!CreateBuffer(skeletonSizes.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, skeletonBlas_.backing, diagnostic))
    {
        return false;
    }
    VkAccelerationStructureCreateInfoKHR skeletonCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    skeletonCreateInfo.buffer = skeletonBlas_.backing.buffer;
    skeletonCreateInfo.size = skeletonSizes.accelerationStructureSize;
    skeletonCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &skeletonCreateInfo, nullptr, &skeletonBlas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create animated skeleton BLAS.";
        return false;
    }
    if (!CreateBuffer(std::max(skeletonSizes.buildScratchSize, skeletonSizes.updateScratchSize), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, skeletonBlasUpdateScratch_, diagnostic))
    {
        return false;
    }
    VkAccelerationStructureBuildRangeInfoKHR skeletonRange{};
    skeletonRange.primitiveCount = skeletonPrimitiveCount;
    skeletonBuildInfo.dstAccelerationStructure = skeletonBlas_.handle;
    skeletonBuildInfo.scratchData.deviceAddress = skeletonBlasUpdateScratch_.address;
    const VkAccelerationStructureBuildRangeInfoKHR* skeletonRanges[] = {&skeletonRange};
    BlasBuildData skeletonBuildData{this, &skeletonBuildInfo, skeletonRanges};
    if (!RunOneTimeCommands(buildBlas, &skeletonBuildData, diagnostic)) return false;
    VkAccelerationStructureDeviceAddressInfoKHR skeletonAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    skeletonAddressInfo.accelerationStructure = skeletonBlas_.handle;
    skeletonBlas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &skeletonAddressInfo);

    std::array<VkAccelerationStructureInstanceKHR, 9u> instances{};
    instances[0].transform = transform;
    instances[0].instanceCustomIndex = 0u;
    instances[0].mask = 0x01u;
    instances[0].instanceShaderBindingTableRecordOffset = 0u;
    instances[0].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instances[0].accelerationStructureReference = blas_.address;
    instances[1] = instances[0];
    instances[1].instanceCustomIndex = 1u;
    instances[1].mask = 0x02u;
    instances[1].accelerationStructureReference = torchBlas_.address;
    instances[1].transform = {{
        1.0f, 0.0f, 0.0f, -0.32f,
        0.0f, 1.0f, 0.0f, -0.36f,
        0.0f, 0.0f, -1.0f, 3.82f}};
    instances[2] = instances[0];
    instances[2].instanceCustomIndex = 2u;
    instances[2].mask = 0x01u;
    instances[2].accelerationStructureReference = skeletonBlas_.address;
    instances[2].transform = {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, -0.95f,
        0.0f, 0.0f, 1.0f, -2.35f}};
    instances[3] = instances[1];
    instances[3].instanceCustomIndex = 3u;
    instances[3].mask = 0x02u;
    instances[3].accelerationStructureReference = swordBlas_.address;
    instances[4] = instances[0];
    instances[4].instanceCustomIndex = 4u;
    instances[4].mask = 0x04u;
    instances[4].accelerationStructureReference = playerBodyBlas_.address;
    instances[4].transform = {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.58f,
        0.0f, 0.0f, 1.0f, 0.0f}};
    for (std::size_t i = 5u; i < instances.size(); ++i)
    {
        instances[i] = instances[4];
        instances[i].accelerationStructureReference = playerLimbBlas_.address;
        instances[i].transform = {{
            0.07f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.07f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.40f, 0.0f}};
    }
    if (!CreateBuffer(sizeof(instances), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, uploadMemory, true, instanceBuffer_, diagnostic))
    {
        return false;
    }
    vkMapMemory(device_, instanceBuffer_.memory, 0u, sizeof(instances), 0u, &mapped);
    std::memcpy(mapped, instances.data(), sizeof(instances));
    vkUnmapMemory(device_, instanceBuffer_.memory);

    VkAccelerationStructureGeometryKHR tlasGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    tlasGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    tlasGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    tlasGeometry.geometry.instances.data.deviceAddress = instanceBuffer_.address;

    VkAccelerationStructureBuildGeometryInfoKHR tlasBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    tlasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    tlasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    tlasBuildInfo.geometryCount = 1u;
    tlasBuildInfo.pGeometries = &tlasGeometry;

    VkAccelerationStructureBuildSizesInfoKHR tlasSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    std::uint32_t instanceCount = static_cast<std::uint32_t>(instances.size());
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &tlasBuildInfo, &instanceCount, &tlasSizes);
    if (!CreateBuffer(tlasSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      tlas_.backing,
                      diagnostic))
    {
        return false;
    }

    VkAccelerationStructureCreateInfoKHR tlasCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    tlasCreateInfo.buffer = tlas_.backing.buffer;
    tlasCreateInfo.size = tlasSizes.accelerationStructureSize;
    tlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &tlasCreateInfo, nullptr, &tlas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create TLAS.";
        return false;
    }

    if (!CreateBuffer(std::max(tlasSizes.buildScratchSize, tlasSizes.updateScratchSize),
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      tlasUpdateScratch_,
                      diagnostic))
    {
        return false;
    }

    VkAccelerationStructureBuildRangeInfoKHR tlasRange{};
    tlasRange.primitiveCount = instanceCount;
    tlasBuildInfo.dstAccelerationStructure = tlas_.handle;
    tlasBuildInfo.scratchData.deviceAddress = tlasUpdateScratch_.address;
    const VkAccelerationStructureBuildRangeInfoKHR* tlasRanges[] = {&tlasRange};
    BlasBuildData tlasBuildData{this, &tlasBuildInfo, tlasRanges};
    if (!RunOneTimeCommands(buildBlas, &tlasBuildData, diagnostic))
    {
        return false;
    }

    VkAccelerationStructureDeviceAddressInfoKHR tlasAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    tlasAddressInfo.accelerationStructure = tlas_.handle;
    tlas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &tlasAddressInfo);

    diagnostic.clear();
    return true;
}

bool PresentableTinyRtScene::CreateDescriptors(std::string& diagnostic)
{
    const std::array<VkDescriptorSetLayoutBinding, 7u> bindings{{
        {0u, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, nullptr},
        {1u, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {2u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {3u, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {4u, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {5u, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {6u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
    }};
    const VkDescriptorSetLayoutCreateInfo layoutInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr,
        0u,
        static_cast<std::uint32_t>(bindings.size()),
        bindings.data()};
    if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &descriptorSetLayout_) != VK_SUCCESS)
    {
        diagnostic = "Failed to create RT descriptor set layout.";
        return false;
    }

    const std::array<VkDescriptorPoolSize, 4u> poolSizes{{
        {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1u},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1u},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3u},
    }};
    const VkDescriptorPoolCreateInfo poolInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        nullptr,
        0u,
        1u,
        static_cast<std::uint32_t>(poolSizes.size()),
        poolSizes.data()};
    if (vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS)
    {
        diagnostic = "Failed to create RT descriptor pool.";
        return false;
    }

    const VkDescriptorSetAllocateInfo allocateInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        descriptorPool_,
        1u,
        &descriptorSetLayout_};
    if (vkAllocateDescriptorSets(device_, &allocateInfo, &descriptorSet_) != VK_SUCCESS)
    {
        diagnostic = "Failed to allocate RT descriptor set.";
        return false;
    }

    VkWriteDescriptorSetAccelerationStructureKHR asWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
    asWrite.accelerationStructureCount = 1u;
    asWrite.pAccelerationStructures = &tlas_.handle;
    VkWriteDescriptorSet accelerationStructureWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    accelerationStructureWrite.pNext = &asWrite;
    accelerationStructureWrite.dstSet = descriptorSet_;
    accelerationStructureWrite.dstBinding = 0u;
    accelerationStructureWrite.descriptorCount = 1u;
    accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo.imageView = storageImageView_;
    VkWriteDescriptorSet imageWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    imageWrite.dstSet = descriptorSet_;
    imageWrite.dstBinding = 1u;
    imageWrite.descriptorCount = 1u;
    imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imageWrite.pImageInfo = &imageInfo;

    VkDescriptorBufferInfo skeletonBufferInfo{};
    skeletonBufferInfo.buffer = skeletonVertexBuffer_.buffer;
    skeletonBufferInfo.offset = 0u;
    skeletonBufferInfo.range = skeletonVertexBuffer_.size;
    VkWriteDescriptorSet skeletonWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    skeletonWrite.dstSet = descriptorSet_;
    skeletonWrite.dstBinding = 2u;
    skeletonWrite.descriptorCount = 1u;
    skeletonWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    skeletonWrite.pBufferInfo = &skeletonBufferInfo;

    VkDescriptorBufferInfo worldSurfaceBufferInfo{};
    worldSurfaceBufferInfo.buffer = worldSurfaceBuffer_.buffer;
    worldSurfaceBufferInfo.offset = 0u;
    worldSurfaceBufferInfo.range = worldSurfaceBuffer_.size;
    VkWriteDescriptorSet worldSurfaceWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    worldSurfaceWrite.dstSet = descriptorSet_;
    worldSurfaceWrite.dstBinding = 6u;
    worldSurfaceWrite.descriptorCount = 1u;
    worldSurfaceWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    worldSurfaceWrite.pBufferInfo = &worldSurfaceBufferInfo;

    const VkDescriptorImageInfo diffuseInfo{materialSampler_, materialDiffuse_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo normalInfo{materialSampler_, materialNormal_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo armInfo{materialSampler_, materialArm_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const auto sampledWrite = [this](std::uint32_t binding, const VkDescriptorImageInfo* info) {
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = descriptorSet_;
        write.dstBinding = binding;
        write.descriptorCount = 1u;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = info;
        return write;
    };
    const std::array<VkWriteDescriptorSet, 7u> writes{{accelerationStructureWrite, imageWrite, skeletonWrite,
                                                       sampledWrite(3u, &diffuseInfo), sampledWrite(4u, &normalInfo), sampledWrite(5u, &armInfo),
                                                       worldSurfaceWrite}};
    vkUpdateDescriptorSets(device_, static_cast<std::uint32_t>(writes.size()), writes.data(), 0u, nullptr);

    diagnostic.clear();
    return true;
}

bool PresentableTinyRtScene::CreatePipeline(std::string& diagnostic)
{
    VkShaderModule raygenModule = VK_NULL_HANDLE;
    VkShaderModule missModule = VK_NULL_HANDLE;
    VkShaderModule hitModule = VK_NULL_HANDLE;
    if (!CreateShaderModule(device_, kMinimalRayGenShader, sizeof(kMinimalRayGenShader), raygenModule) ||
        !CreateShaderModule(device_, kMinimalMissShader, sizeof(kMinimalMissShader), missModule) ||
        !CreateShaderModule(device_, kMinimalClosestHitShader, sizeof(kMinimalClosestHitShader), hitModule))
    {
        if (raygenModule != VK_NULL_HANDLE) vkDestroyShaderModule(device_, raygenModule, nullptr);
        if (missModule != VK_NULL_HANDLE) vkDestroyShaderModule(device_, missModule, nullptr);
        if (hitModule != VK_NULL_HANDLE) vkDestroyShaderModule(device_, hitModule, nullptr);
        diagnostic = "Failed to create RT shader modules.";
        return false;
    }

    const std::array<VkPipelineShaderStageCreateInfo, 3u> stages{{
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, raygenModule, "main", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0u, VK_SHADER_STAGE_MISS_BIT_KHR, missModule, "main", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0u, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, hitModule, "main", nullptr},
    }};

    std::array<VkRayTracingShaderGroupCreateInfoKHR, 3u> groups{};
    groups[0].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    groups[0].generalShader = 0u;
    groups[0].closestHitShader = VK_SHADER_UNUSED_KHR;
    groups[0].anyHitShader = VK_SHADER_UNUSED_KHR;
    groups[0].intersectionShader = VK_SHADER_UNUSED_KHR;
    groups[1].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    groups[1].generalShader = 1u;
    groups[1].closestHitShader = VK_SHADER_UNUSED_KHR;
    groups[1].anyHitShader = VK_SHADER_UNUSED_KHR;
    groups[1].intersectionShader = VK_SHADER_UNUSED_KHR;
    groups[2].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[2].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    groups[2].generalShader = VK_SHADER_UNUSED_KHR;
    groups[2].closestHitShader = 2u;
    groups[2].anyHitShader = VK_SHADER_UNUSED_KHR;
    groups[2].intersectionShader = VK_SHADER_UNUSED_KHR;

    const VkPushConstantRange pushConstantRange{
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        0u,
        sizeof(ScenePushConstants)};
    const VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        nullptr,
        0u,
        1u,
        &descriptorSetLayout_,
        1u,
        &pushConstantRange};
    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS)
    {
        vkDestroyShaderModule(device_, raygenModule, nullptr);
        vkDestroyShaderModule(device_, missModule, nullptr);
        vkDestroyShaderModule(device_, hitModule, nullptr);
        diagnostic = "Failed to create RT pipeline layout.";
        return false;
    }

    VkRayTracingPipelineCreateInfoKHR pipelineInfo{VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
    pipelineInfo.stageCount = static_cast<std::uint32_t>(stages.size());
    pipelineInfo.pStages = stages.data();
    pipelineInfo.groupCount = static_cast<std::uint32_t>(groups.size());
    pipelineInfo.pGroups = groups.data();
    pipelineInfo.maxPipelineRayRecursionDepth = 1u;
    pipelineInfo.layout = pipelineLayout_;
    const VkResult result = vkCreateRayTracingPipelinesKHR_(device_, VK_NULL_HANDLE, VK_NULL_HANDLE, 1u, &pipelineInfo, nullptr, &pipeline_);

    vkDestroyShaderModule(device_, raygenModule, nullptr);
    vkDestroyShaderModule(device_, missModule, nullptr);
    vkDestroyShaderModule(device_, hitModule, nullptr);

    if (result != VK_SUCCESS)
    {
        diagnostic = "Failed to create RT pipeline.";
        return false;
    }

    diagnostic.clear();
    return true;
}

bool PresentableTinyRtScene::CreateShaderBindingTable(std::string& diagnostic)
{
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
    VkPhysicalDeviceProperties2 properties2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    properties2.pNext = &rtProperties;
    auto getPhysicalDeviceProperties2 = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
        vkGetInstanceProcAddr(instance_, "vkGetPhysicalDeviceProperties2"));
    if (getPhysicalDeviceProperties2 == nullptr)
    {
        getPhysicalDeviceProperties2 = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
            vkGetInstanceProcAddr(instance_, "vkGetPhysicalDeviceProperties2KHR"));
    }
    if (getPhysicalDeviceProperties2 == nullptr)
    {
        diagnostic = "vkGetPhysicalDeviceProperties2 is unavailable for RT pipeline properties.";
        return false;
    }
    getPhysicalDeviceProperties2(physicalDevice_, &properties2);

    constexpr std::uint32_t groupCount = 3u;
    const std::uint32_t handleSize = rtProperties.shaderGroupHandleSize;
    const std::uint32_t handleAlignment = rtProperties.shaderGroupHandleAlignment;
    const std::uint32_t baseAlignment = rtProperties.shaderGroupBaseAlignment;
    const std::uint32_t groupStride = AlignUp(handleSize, handleAlignment);
    const std::uint32_t regionSize = AlignUp(groupStride, baseAlignment);
    const std::uint32_t sbtSize = regionSize * groupCount;

    std::vector<std::uint8_t> handles(handleSize * groupCount);
    if (vkGetRayTracingShaderGroupHandlesKHR_(device_, pipeline_, 0u, groupCount, handles.size(), handles.data()) != VK_SUCCESS)
    {
        diagnostic = "Failed to fetch RT shader group handles.";
        return false;
    }

    if (!CreateBuffer(sbtSize,
                      VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      true,
                      shaderBindingTable_,
                      diagnostic))
    {
        return false;
    }

    void* mapped = nullptr;
    if (vkMapMemory(device_, shaderBindingTable_.memory, 0u, sbtSize, 0u, &mapped) != VK_SUCCESS)
    {
        diagnostic = "Failed to map RT shader binding table.";
        return false;
    }
    auto* output = static_cast<std::uint8_t*>(mapped);
    std::memset(output, 0, sbtSize);
    for (std::uint32_t group = 0u; group < groupCount; ++group)
    {
        std::memcpy(output + (regionSize * group), handles.data() + (handleSize * group), handleSize);
    }
    vkUnmapMemory(device_, shaderBindingTable_.memory);

    raygenRegion_.deviceAddress = shaderBindingTable_.address;
    raygenRegion_.stride = groupStride;
    raygenRegion_.size = groupStride;
    missRegion_.deviceAddress = shaderBindingTable_.address + regionSize;
    missRegion_.stride = groupStride;
    missRegion_.size = groupStride;
    hitRegion_.deviceAddress = shaderBindingTable_.address + (regionSize * 2u);
    hitRegion_.stride = groupStride;
    hitRegion_.size = groupStride;
    callableRegion_ = {};

    diagnostic.clear();
    return true;
}

bool PresentableTinyRtScene::UpdateDynamicInstances(VkCommandBuffer commandBuffer,
                                                     float cameraYaw,
                                                     float cameraPitch,
                                                     float walkTime,
                                                     float cameraX,
                                                     float cameraZ,
                                                     float walkAmount,
                                                     const horde::gameplay::CombatSnapshot& combat,
                                                     std::string& diagnostic)
{
    if (instanceBuffer_.memory == VK_NULL_HANDLE || skeletonVertexBuffer_.memory == VK_NULL_HANDLE ||
        skeletonBlas_.handle == VK_NULL_HANDLE || swordBlas_.handle == VK_NULL_HANDLE ||
        playerBodyBlas_.handle == VK_NULL_HANDLE || playerLimbBlas_.handle == VK_NULL_HANDLE ||
        tlas_.handle == VK_NULL_HANDLE || skeletonBlasUpdateScratch_.address == 0u || tlasUpdateScratch_.address == 0u)
    {
        diagnostic = "Combat skeleton or held-prop TLAS resources are unavailable.";
        return false;
    }

    using Vec3 = std::array<float, 3>;
    const auto add = [](const Vec3& a, const Vec3& b) { return Vec3{a[0] + b[0], a[1] + b[1], a[2] + b[2]}; };
    const auto subtract = [](const Vec3& a, const Vec3& b) { return Vec3{a[0] - b[0], a[1] - b[1], a[2] - b[2]}; };
    const auto scaled = [](const Vec3& v, float scale) { return Vec3{v[0] * scale, v[1] * scale, v[2] * scale}; };
    const auto dot = [](const Vec3& a, const Vec3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; };
    const auto cross = [](const Vec3& a, const Vec3& b) {
        return Vec3{a[1] * b[2] - a[2] * b[1],
                    a[2] * b[0] - a[0] * b[2],
                    a[0] * b[1] - a[1] * b[0]};
    };
    const auto normalize = [&dot](const Vec3& v) {
        const float length = std::sqrt(std::max(dot(v, v), 0.0000001f));
        return Vec3{v[0] / length, v[1] / length, v[2] / length};
    };
    const auto lerp = [](const Vec3& a, const Vec3& b, float amount) {
        return Vec3{a[0] + (b[0] - a[0]) * amount,
                    a[1] + (b[1] - a[1]) * amount,
                    a[2] + (b[2] - a[2]) * amount};
    };

    const Vec3 worldUp{0.0f, 1.0f, 0.0f};
    const Vec3 eye{cameraX, 0.58f, cameraZ};
    const Vec3 bodyForward{std::sin(cameraYaw), 0.0f, -std::cos(cameraYaw)};
    const Vec3 bodyRight{std::cos(cameraYaw), 0.0f, std::sin(cameraYaw)};
    const float viewPitch = std::clamp(cameraPitch, -0.32f, 0.28f);
    const Vec3 viewForward = normalize(Vec3{std::sin(cameraYaw), -0.05f + viewPitch, -std::cos(cameraYaw)});
    const Vec3 viewRight = normalize(cross(viewForward, worldUp));
    const Vec3 viewUp = normalize(cross(viewRight, viewForward));
    const auto toWorld = [&eye, &viewRight, &viewUp, &viewForward, &add, &scaled](const Vec3& local) {
        return add(add(add(eye, scaled(viewRight, local[0])), scaled(viewUp, local[1])), scaled(viewForward, local[2]));
    };
    const auto solveElbow = [&subtract, &add, &scaled, &dot, &normalize](const Vec3& shoulder,
                                                                        const Vec3& hand,
                                                                        float upperLength,
                                                                        float lowerLength,
                                                                        const Vec3& poleSeed) {
        const Vec3 delta = subtract(hand, shoulder);
        const float distance = std::sqrt(std::max(dot(delta, delta), 0.0000001f));
        const Vec3 direction = scaled(delta, 1.0f / distance);
        const Vec3 pole = normalize(subtract(poleSeed, scaled(direction, dot(poleSeed, direction))));
        const float along = std::clamp((upperLength * upperLength - lowerLength * lowerLength + distance * distance) / (2.0f * distance),
                                       0.0f,
                                       upperLength);
        const float height = std::sqrt(std::max(upperLength * upperLength - along * along, 0.0f));
        return add(add(shoulder, scaled(direction, along)), scaled(pole, height));
    };
    const auto segmentTransform = [&subtract, &cross, &normalize, &dot](const Vec3& start, const Vec3& end, float radius) {
        const Vec3 delta = subtract(end, start);
        const float length = std::sqrt(std::max(dot(delta, delta), 0.0000001f));
        const Vec3 zAxis = normalize(delta);
        const Vec3 reference = std::abs(zAxis[1]) < 0.95f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{1.0f, 0.0f, 0.0f};
        const Vec3 xAxis = normalize(cross(reference, zAxis));
        const Vec3 yAxis = cross(zAxis, xAxis);
        return VkTransformMatrixKHR{{
            xAxis[0] * radius, yAxis[0] * radius, zAxis[0] * length, start[0],
            xAxis[1] * radius, yAxis[1] * radius, zAxis[1] * length, start[1],
            xAxis[2] * radius, yAxis[2] * radius, zAxis[2] * length, start[2]}};
    };

    const float movement = std::max(walkAmount, 0.2f);
    constexpr float torchScale = 0.56f;
    const float torchSway = std::sin(walkTime * 6.2f) * 0.035f * movement;
    const float torchBob = std::abs(std::sin(walkTime * 6.2f)) * 0.025f * movement;
    // Keep the visible grip line above the bottom crop and far enough forward
    // that the held props read at a natural first-person scale on a tall phone.
    const Vec3 leftShoulderLocal{-0.24f, -0.44f, 0.06f};
    const Vec3 leftHandLocal{-0.34f - torchSway, -0.40f + torchBob, 1.05f};
    const float swingAmount = std::clamp(-combat.swordSwingRadians / 1.12f, 0.0f, 1.0f);
    const float smoothSwing = swingAmount * swingAmount * (3.0f - 2.0f * swingAmount);
    const Vec3 rightShoulderLocal{0.24f, -0.44f, 0.06f};
    const Vec3 rightHandLocal = lerp(Vec3{0.34f, -0.41f, 1.05f}, Vec3{-0.08f, -0.47f, 1.00f}, smoothSwing);
    const Vec3 leftElbowLocal = solveElbow(leftShoulderLocal, leftHandLocal, 0.53f, 0.53f, Vec3{-1.0f, -0.15f, 0.08f});
    const Vec3 rightElbowLocal = solveElbow(rightShoulderLocal, rightHandLocal, 0.53f, 0.53f, Vec3{1.0f, -0.20f, 0.10f});
    const Vec3 leftShoulder = toWorld(leftShoulderLocal);
    const Vec3 leftElbow = toWorld(leftElbowLocal);
    const Vec3 leftHand = toWorld(leftHandLocal);
    const Vec3 rightShoulder = toWorld(rightShoulderLocal);
    const Vec3 rightElbow = toWorld(rightElbowLocal);
    const Vec3 rightHand = toWorld(rightHandLocal);

    const Vec3 torchColumnX = scaled(viewRight, torchScale);
    const Vec3 torchColumnY = scaled(viewUp, torchScale);
    const Vec3 torchColumnZ = scaled(viewForward, torchScale);
    // Local torch grip is (0, -0.22, 0), so T = hand - M * grip.
    const Vec3 torchTranslation = add(leftHand, scaled(torchColumnY, 0.22f));

    const float swordCos = std::cos(combat.swordSwingRadians);
    const float swordSin = std::sin(combat.swordSwingRadians);
    const Vec3 swordColumnX = scaled(add(scaled(viewRight, swordCos), scaled(viewUp, swordSin)), torchScale);
    const Vec3 swordColumnY = scaled(add(scaled(viewRight, -swordSin), scaled(viewUp, swordCos)), torchScale);
    const Vec3 swordColumnZ = scaled(viewForward, torchScale);
    // Local sword grip is (1.47, -0.485, 0), so T = hand - M * grip.
    const Vec3 swordTranslation = add(add(rightHand, scaled(swordColumnX, -1.47f)), scaled(swordColumnY, 0.485f));

    const horde::scene::SkeletonClip skeletonClip = combat.enemyAnimation == horde::gameplay::EnemyAnimation::Walking
        ? horde::scene::SkeletonClip::Walking
        : (combat.enemyAnimation == horde::gameplay::EnemyAnimation::Attack
               ? horde::scene::SkeletonClip::Attack
               : (combat.enemyAnimation == horde::gameplay::EnemyAnimation::Dead
                      ? horde::scene::SkeletonClip::Dead
                      : horde::scene::SkeletonClip::Idle));
    constexpr float skeletonUpdateInterval = 1.0f / 30.0f;
    const int skeletonClipIndex = static_cast<int>(skeletonClip);
    const float skeletonTime = combat.enemyAnimationTime;
    const bool clipChanged = skeletonClipIndex != lastSkeletonClip_;
    const bool updateSkeleton = clipChanged || lastSkeletonUpdateTime_ < 0.0f || skeletonTime < lastSkeletonUpdateTime_ ||
                                (skeletonTime - lastSkeletonUpdateTime_) >= skeletonUpdateInterval;
    void* mapped = nullptr;
    if (updateSkeleton)
    {
        if (!skeletonModel_.Skin(skeletonClip, skeletonTime, skeletonSkinnedVertices_, diagnostic))
        {
            return false;
        }
        const VkDeviceSize skeletonVertexBufferSize = sizeof(horde::scene::SkinnedRtVertex) * skeletonSkinnedVertices_.size();
        if (vkMapMemory(device_, skeletonVertexBuffer_.memory, 0u, skeletonVertexBufferSize, 0u, &mapped) != VK_SUCCESS || mapped == nullptr)
        {
            diagnostic = "Failed to update animated skeleton vertex data.";
            return false;
        }
        std::memcpy(mapped, skeletonSkinnedVertices_.data(), static_cast<std::size_t>(skeletonVertexBufferSize));
        vkUnmapMemory(device_, skeletonVertexBuffer_.memory);
        lastSkeletonUpdateTime_ = skeletonTime;
        lastSkeletonClip_ = skeletonClipIndex;
    }

    std::array<VkAccelerationStructureInstanceKHR, 9u> instances{};
    instances[0].transform = {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f}};
    instances[0].instanceCustomIndex = 0u;
    instances[0].mask = 0x01u;
    instances[0].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instances[0].accelerationStructureReference = blas_.address;
    instances[1] = instances[0];
    instances[1].transform = {{
        torchColumnX[0], torchColumnY[0], torchColumnZ[0], torchTranslation[0],
        torchColumnX[1], torchColumnY[1], torchColumnZ[1], torchTranslation[1],
        torchColumnX[2], torchColumnY[2], torchColumnZ[2], torchTranslation[2]}};
    instances[1].instanceCustomIndex = 1u;
    instances[1].mask = 0x02u;
    instances[1].accelerationStructureReference = torchBlas_.address;
    instances[2] = instances[0];
    instances[2].instanceCustomIndex = 2u;
    instances[2].mask = 0x01u;
    instances[2].accelerationStructureReference = skeletonBlas_.address;
    const float enemyCos = std::cos(combat.enemyFacingRadians);
    const float enemySin = std::sin(combat.enemyFacingRadians);
    instances[2].transform = {{
        enemyCos, 0.0f, enemySin, combat.enemyX,
        0.0f, 1.0f, 0.0f, -0.95f,
        -enemySin, 0.0f, enemyCos, combat.enemyZ}};
    instances[3] = instances[1];
    instances[3].instanceCustomIndex = 3u;
    instances[3].mask = 0x02u;
    instances[3].accelerationStructureReference = swordBlas_.address;
    instances[3].transform = {{
        swordColumnX[0], swordColumnY[0], swordColumnZ[0], swordTranslation[0],
        swordColumnX[1], swordColumnY[1], swordColumnZ[1], swordTranslation[1],
        swordColumnX[2], swordColumnY[2], swordColumnZ[2], swordTranslation[2]}};
    instances[4] = instances[0];
    instances[4].instanceCustomIndex = 4u;
    instances[4].mask = 0x04u;
    instances[4].accelerationStructureReference = playerBodyBlas_.address;
    instances[4].transform = {{
        bodyRight[0], 0.0f, bodyForward[0], cameraX,
        bodyRight[1], 1.0f, bodyForward[1], 0.58f,
        bodyRight[2], 0.0f, bodyForward[2], cameraZ}};
    for (std::size_t i = 5u; i < instances.size(); ++i)
    {
        instances[i] = instances[4];
        instances[i].accelerationStructureReference = playerLimbBlas_.address;
    }
    instances[5].transform = segmentTransform(leftShoulder, leftElbow, 0.065f);
    instances[6].transform = segmentTransform(leftElbow, leftHand, 0.055f);
    instances[7].transform = segmentTransform(rightShoulder, rightElbow, 0.065f);
    instances[8].transform = segmentTransform(rightElbow, rightHand, 0.055f);

    if (vkMapMemory(device_, instanceBuffer_.memory, 0u, sizeof(instances), 0u, &mapped) != VK_SUCCESS || mapped == nullptr)
    {
        diagnostic = "Failed to update combat instance data.";
        return false;
    }
    std::memcpy(mapped, instances.data(), sizeof(instances));
    vkUnmapMemory(device_, instanceBuffer_.memory);

    VkMemoryBarrier hostWriteBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    hostWriteBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    hostWriteBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_HOST_BIT,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                         0u,
                         1u,
                         &hostWriteBarrier,
                         0u,
                         nullptr,
                         0u,
                         nullptr);

    if (updateSkeleton)
    {
        VkAccelerationStructureGeometryKHR skeletonGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        skeletonGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        skeletonGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        skeletonGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        skeletonGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        skeletonGeometry.geometry.triangles.vertexData.deviceAddress = skeletonVertexBuffer_.address;
        skeletonGeometry.geometry.triangles.vertexStride = sizeof(horde::scene::SkinnedRtVertex);
        skeletonGeometry.geometry.triangles.maxVertex = static_cast<std::uint32_t>(skeletonSkinnedVertices_.size() - 1u);
        skeletonGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
        VkAccelerationStructureBuildGeometryInfoKHR skeletonUpdateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        skeletonUpdateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        skeletonUpdateInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        skeletonUpdateInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
        skeletonUpdateInfo.srcAccelerationStructure = skeletonBlas_.handle;
        skeletonUpdateInfo.dstAccelerationStructure = skeletonBlas_.handle;
        skeletonUpdateInfo.geometryCount = 1u;
        skeletonUpdateInfo.pGeometries = &skeletonGeometry;
        skeletonUpdateInfo.scratchData.deviceAddress = skeletonBlasUpdateScratch_.address;
        VkAccelerationStructureBuildRangeInfoKHR skeletonRange{};
        skeletonRange.primitiveCount = static_cast<std::uint32_t>(skeletonSkinnedVertices_.size() / 3u);
        const VkAccelerationStructureBuildRangeInfoKHR* skeletonRanges[] = {&skeletonRange};
        vkCmdBuildAccelerationStructuresKHR_(commandBuffer, 1u, &skeletonUpdateInfo, skeletonRanges);

        VkMemoryBarrier skeletonBuildBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        skeletonBuildBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        skeletonBuildBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             0u,
                             1u,
                             &skeletonBuildBarrier,
                             0u,
                             nullptr,
                             0u,
                             nullptr);
    }

    VkAccelerationStructureGeometryKHR tlasGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    tlasGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    tlasGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    tlasGeometry.geometry.instances.data.deviceAddress = instanceBuffer_.address;

    VkAccelerationStructureBuildGeometryInfoKHR updateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    updateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    updateInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    updateInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
    updateInfo.srcAccelerationStructure = tlas_.handle;
    updateInfo.dstAccelerationStructure = tlas_.handle;
    updateInfo.geometryCount = 1u;
    updateInfo.pGeometries = &tlasGeometry;
    updateInfo.scratchData.deviceAddress = tlasUpdateScratch_.address;

    VkAccelerationStructureBuildRangeInfoKHR updateRange{};
    updateRange.primitiveCount = static_cast<std::uint32_t>(instances.size());
    const VkAccelerationStructureBuildRangeInfoKHR* updateRanges[] = {&updateRange};
    vkCmdBuildAccelerationStructuresKHR_(commandBuffer, 1u, &updateInfo, updateRanges);

    VkMemoryBarrier traceBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    traceBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    traceBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                         VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                         0u,
                         1u,
                         &traceBarrier,
                         0u,
                         nullptr,
                         0u,
                         nullptr);

    diagnostic.clear();
    return true;
}

bool PresentableTinyRtScene::RecordTraceAndCopy(VkCommandBuffer commandBuffer,
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
                                                 std::string& diagnostic)
{
    if (!ready_)
    {
        diagnostic = "RT scene is not ready.";
        return false;
    }

    if (!UpdateDynamicInstances(commandBuffer, cameraYaw, cameraPitch, walkTime, cameraX, cameraZ, walkAmount, combat, diagnostic))
    {
        return false;
    }

    if (storageImageLayout_ != VK_IMAGE_LAYOUT_GENERAL)
    {
        SetImageBarrier(commandBuffer,
                        storageImage_,
                        storageImageLayout_,
                        VK_IMAGE_LAYOUT_GENERAL,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                        VK_ACCESS_TRANSFER_READ_BIT,
                        VK_ACCESS_SHADER_WRITE_BIT);
        storageImageLayout_ = VK_IMAGE_LAYOUT_GENERAL;
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout_, 0u, 1u, &descriptorSet_, 0u, nullptr);
    const ScenePushConstants pushConstants{cameraYaw,
                                           cameraPitch,
                                           lanternStrength,
                                           walkTime,
                                           cameraX,
                                            cameraZ,
                                            walkAmount,
                                            presentationUsesBgra_ ? 1.0f : 0.0f,
                                            std::clamp(outputExposure, 0.2f, 1.4f),
                                            std::clamp(combat.damageFlash, 0.0f, 1.0f)};
    vkCmdPushConstants(commandBuffer,
                       pipelineLayout_,
                       VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
                       0u,
                       sizeof(pushConstants),
                       &pushConstants);
    vkCmdTraceRaysKHR_(commandBuffer,
                       &raygenRegion_,
                       &missRegion_,
                       &hitRegion_,
                       &callableRegion_,
                       dispatchExtent_.width,
                       dispatchExtent_.height,
                       1u);

    SetImageBarrier(commandBuffer,
                    storageImage_,
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_ACCESS_SHADER_WRITE_BIT,
                    VK_ACCESS_TRANSFER_READ_BIT);
    storageImageLayout_ = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    const VkPipelineStageFlags swapSrcStage = swapchainImageLayout == VK_IMAGE_LAYOUT_UNDEFINED
        ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
        : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SetImageBarrier(commandBuffer,
                    swapchainImage,
                    swapchainImageLayout,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    swapSrcStage,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0u,
                    VK_ACCESS_TRANSFER_WRITE_BIT);

    VkImageCopy copyRegion{};
    copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
    copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
    copyRegion.extent = {std::min(dispatchExtent_.width, swapchainExtent.width), std::min(dispatchExtent_.height, swapchainExtent.height), 1u};
    vkCmdCopyImage(commandBuffer,
                   storageImage_,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   swapchainImage,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1u,
                   &copyRegion);

    SetImageBarrier(commandBuffer,
                    swapchainImage,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    VK_ACCESS_TRANSFER_WRITE_BIT,
                    0u);
    swapchainImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    SetImageBarrier(commandBuffer,
                    storageImage_,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                    VK_ACCESS_TRANSFER_READ_BIT,
                    VK_ACCESS_SHADER_WRITE_BIT);
    storageImageLayout_ = VK_IMAGE_LAYOUT_GENERAL;

    diagnostic.clear();
    return true;
}

} // namespace horde::vulkan::raytracing
