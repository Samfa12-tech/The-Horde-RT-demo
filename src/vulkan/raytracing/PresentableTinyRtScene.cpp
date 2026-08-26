#include "vulkan/raytracing/PresentableTinyRtScene.h"

#include "gameplay/items/HeldItemKinematics.h"
#include "gameplay/items/HeldLightState.h"
#include "vulkan/raytracing/HeldItemRenderSlot.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <utility>
#include <vector>

#include "gameplay/CorridorCollision.h"
#include "scene/assets/AssetManifest.h"

namespace horde::vulkan::raytracing
{

namespace
{

constexpr VkFormat kStorageImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

struct ScenePushConstants
{
    float yaw = 0.0f;
    float pitch = 0.0f;
    float torchLight = 1.0f;
    float time = 0.0f;
    float cameraX = 0.0f;
    float cameraZ = 1.85f;
    float walkAmount = 0.0f;
    float outputRedBlueSwap = 0.0f;
    float outputExposure = 0.92f;
    float damageFlash = 0.0f;
    float enemyKind = 0.0f;
    float staffLightStrength = 0.0f;
    float staffX = -32.2f;
    float staffY = 0.55f;
    float staffZ = -13.1f;
    float finaleSkylightOpen = 0.0f;
    float finaleDawnReveal = 0.0f;
    float heldPropDepth = 1.05f;
    float waterQuality = 2.0f;
    float waterfallWidthScale = 1.0f;
    float fogDensityScale = 1.0f;
    float torchHueDegrees = 0.0f;
    float torchIntensityScale = 1.0f;
    float skylightHueDegrees = 0.0f;
    float skylightIntensityScale = 1.0f;
    float passageHueDegrees = 0.0f;
    float passageIntensityScale = 1.0f;
    float staffHueDegrees = 0.0f;
    float staffIntensityScale = 1.0f;
    float workloadPreset = 1.0f;
};

static_assert(sizeof(ScenePushConstants) == 120u,
              "CPU and raygen push-constant ABI must remain 30 packed floats");
static_assert(offsetof(ScenePushConstants, waterQuality) == 72u,
              "water quality must stay appended after every released push field");
static_assert(offsetof(ScenePushConstants, waterfallWidthScale) == 76u,
              "RT lab tuning must append after the released water-quality field");

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

PlayerWeaponRenderPose EvaluatePlayerWeaponRenderPose(
    const horde::gameplay::PlayerCombatSnapshot& playerCombat,
    const float swordSwingRadians,
    const float heldPropDepth)
{
    const horde::gameplay::items::HeldSwordPose sharedPose =
        horde::gameplay::items::EvaluateHeldSwordPose(
            playerCombat, swordSwingRadians, heldPropDepth);
    return {sharedPose.rightHandLocal,
            sharedPose.swordRadians,
            sharedPose.parryBlend,
            sharedPose.successJolt};
}

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
    scaledBlitSupported_ = std::exchange(other.scaledBlitSupported_, false);
    storageImage_ = std::exchange(other.storageImage_, VK_NULL_HANDLE);
    storageImageMemory_ = std::exchange(other.storageImageMemory_, VK_NULL_HANDLE);
    storageImageView_ = std::exchange(other.storageImageView_, VK_NULL_HANDLE);
    storageImageLayout_ = std::exchange(other.storageImageLayout_, VK_IMAGE_LAYOUT_UNDEFINED);
    lastOutputRedBlueSwapApplied_ = std::exchange(other.lastOutputRedBlueSwapApplied_, false);
    materialDiffuse_ = std::exchange(other.materialDiffuse_, TextureArray{});
    materialNormal_ = std::exchange(other.materialNormal_, TextureArray{});
    materialArm_ = std::exchange(other.materialArm_, TextureArray{});
    lichBaseColor_ = std::exchange(other.lichBaseColor_, TextureArray{});
    lichEmissive_ = std::exchange(other.lichEmissive_, TextureArray{});
    staticBaseColor_ = std::exchange(other.staticBaseColor_, TextureArray{});
    staticNormal_ = std::exchange(other.staticNormal_, TextureArray{});
    staticOrm_ = std::exchange(other.staticOrm_, TextureArray{});
    staticEmissive_ = std::exchange(other.staticEmissive_, TextureArray{});
    materialSampler_ = std::exchange(other.materialSampler_, VK_NULL_HANDLE);
    materialEncoding_ = std::move(other.materialEncoding_);
    vertexBuffer_ = std::exchange(other.vertexBuffer_, Buffer{});
    indexBuffer_ = std::exchange(other.indexBuffer_, Buffer{});
    transformBuffer_ = std::exchange(other.transformBuffer_, Buffer{});
    instanceBuffer_ = std::exchange(other.instanceBuffer_, Buffer{});
    heldLightBuffer_ = std::exchange(other.heldLightBuffer_, Buffer{});
    worldSurfaceBuffer_ = std::exchange(other.worldSurfaceBuffer_, Buffer{});
    staticVertexBuffer_ = std::exchange(other.staticVertexBuffer_, Buffer{});
    staticIndexBuffer_ = std::exchange(other.staticIndexBuffer_, Buffer{});
    staticGeometryTransformBuffer_ = std::exchange(other.staticGeometryTransformBuffer_, Buffer{});
    instanceMetadataBuffer_ = std::exchange(other.instanceMetadataBuffer_, Buffer{});
    primitiveMetadataBuffer_ = std::exchange(other.primitiveMetadataBuffer_, Buffer{});
    materialMetadataBuffer_ = std::exchange(other.materialMetadataBuffer_, Buffer{});
    blas_ = std::exchange(other.blas_, AccelerationStructure{});
    waterfallBlas_ = std::exchange(other.waterfallBlas_, AccelerationStructure{});
    finaleRoofBlas_ = std::exchange(other.finaleRoofBlas_, AccelerationStructure{});
    torchBlas_ = std::exchange(other.torchBlas_, AccelerationStructure{});
    swordBlas_ = std::exchange(other.swordBlas_, AccelerationStructure{});
    playerBodyBlas_ = std::exchange(other.playerBodyBlas_, AccelerationStructure{});
    playerLimbBlas_ = std::exchange(other.playerLimbBlas_, AccelerationStructure{});
    tlas_ = std::exchange(other.tlas_, AccelerationStructure{});
    tlasUpdateScratch_ = std::exchange(other.tlasUpdateScratch_, Buffer{});
    characterSlot_ = std::move(other.characterSlot_);
    other.characterSlot_ = {};
    developmentStaticAsset_ = std::move(other.developmentStaticAsset_);
    productionTorchAsset_ = std::move(other.productionTorchAsset_);
    developmentStaticAssetDirectory_ = std::move(other.developmentStaticAssetDirectory_);
    staticTextureDirectory_ = std::move(other.staticTextureDirectory_);
    staticMeshSlot_ = std::move(other.staticMeshSlot_);
    genericStaticAssetEnabled_ = std::exchange(other.genericStaticAssetEnabled_, false);
    productionHeldItemAssetsEnabled_ =
        std::exchange(other.productionHeldItemAssetsEnabled_, false);
    staticMeshBlasBytes_ = std::exchange(other.staticMeshBlasBytes_, 0u);
    staticTextureBytes_ = std::exchange(other.staticTextureBytes_, 0u);
    staticMeshBlasBuildMilliseconds_ = std::exchange(other.staticMeshBlasBuildMilliseconds_, 0.0);
    heldItemBlasMeasurements_ = std::exchange(
        other.heldItemBlasMeasurements_, HeldItemBlasMeasurements{});
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
    gpuResources_.Bind(physicalDevice_, device_, vkDestroyAccelerationStructureKHR_, vkGetBufferDeviceAddressKHR_);
    other.gpuResources_.Reset();
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
                                        const std::string& lichAssetPath,
                                        const std::string& materialAssetDirectory,
                                        const std::string& lichTextureDirectory,
                                        std::string& diagnostic,
                                        const std::string& developmentStaticAssetDirectory,
                                        const std::string& productionAssetRoot)
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
    VkFormatProperties storageFormatProperties{};
    VkFormatProperties presentationFormatProperties{};
    vkGetPhysicalDeviceFormatProperties(physicalDevice_, kStorageImageFormat, &storageFormatProperties);
    vkGetPhysicalDeviceFormatProperties(physicalDevice_, presentationFormat, &presentationFormatProperties);
    const VkFormatFeatureFlags storageBlitFeatures = VK_FORMAT_FEATURE_BLIT_SRC_BIT |
                                                     VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
    scaledBlitSupported_ = (storageFormatProperties.optimalTilingFeatures & storageBlitFeatures) == storageBlitFeatures &&
                           (presentationFormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) != 0u;
    if (!characterSlot_.LoadAssets(skeletonAssetPath, lichAssetPath, diagnostic) ||
        !LoadEntryPoints(diagnostic))
    {
        Destroy();
        return false;
    }
    gpuResources_.Bind(physicalDevice_, device_, vkDestroyAccelerationStructureKHR_, vkGetBufferDeviceAddressKHR_);
    if (!LoadStaticHeldItemAssets(
            developmentStaticAssetDirectory, productionAssetRoot, diagnostic) ||
        !CreateStorageImage(diagnostic) ||
        !CreateMaterialTextures(materialAssetDirectory, diagnostic) ||
        !CreateLichTextures(lichTextureDirectory, diagnostic) ||
        !CreateStaticMeshResources(diagnostic) ||
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
    characterSlot_.DestroyGpuResources(gpuResources_);
    DestroyAccelerationStructure(playerLimbBlas_);
    DestroyAccelerationStructure(playerBodyBlas_);
    DestroyAccelerationStructure(swordBlas_);
    DestroyAccelerationStructure(torchBlas_);
    DestroyAccelerationStructure(finaleRoofBlas_);
    DestroyAccelerationStructure(waterfallBlas_);
    DestroyAccelerationStructure(blas_);
    DestroyBuffer(worldSurfaceBuffer_);
    DestroyBuffer(materialMetadataBuffer_);
    DestroyBuffer(primitiveMetadataBuffer_);
    DestroyBuffer(instanceMetadataBuffer_);
    DestroyBuffer(staticIndexBuffer_);
    DestroyBuffer(staticGeometryTransformBuffer_);
    DestroyBuffer(staticVertexBuffer_);
    DestroyBuffer(heldLightBuffer_);
    DestroyBuffer(instanceBuffer_);
    DestroyBuffer(transformBuffer_);
    DestroyBuffer(indexBuffer_);
    DestroyBuffer(vertexBuffer_);

    if (materialSampler_ != VK_NULL_HANDLE)
    {
        vkDestroySampler(device_, materialSampler_, nullptr);
        materialSampler_ = VK_NULL_HANDLE;
    }
    DestroyTextureArray(materialArm_);
    DestroyTextureArray(materialNormal_);
    DestroyTextureArray(materialDiffuse_);
    DestroyTextureArray(lichEmissive_);
    DestroyTextureArray(lichBaseColor_);
    DestroyTextureArray(staticEmissive_);
    DestroyTextureArray(staticOrm_);
    DestroyTextureArray(staticNormal_);
    DestroyTextureArray(staticBaseColor_);
    materialEncoding_.clear();
    developmentStaticAsset_ = {};
    productionTorchAsset_ = {};
    developmentStaticAssetDirectory_.clear();
    staticTextureDirectory_.clear();
    staticMeshSlot_ = {};
    genericStaticAssetEnabled_ = false;
    productionHeldItemAssetsEnabled_ = false;
    staticMeshBlasBytes_ = 0u;
    staticTextureBytes_ = 0u;
    staticMeshBlasBuildMilliseconds_ = 0.0;
    heldItemBlasMeasurements_ = {};

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
    storageImageLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    lastOutputRedBlueSwapApplied_ = false;

    raygenRegion_ = {};
    missRegion_ = {};
    hitRegion_ = {};
    callableRegion_ = {};
    scaledBlitSupported_ = false;
    ready_ = false;
    gpuResources_.Reset();
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

bool PresentableTinyRtScene::CreateBuffer(VkDeviceSize size,
                                          VkBufferUsageFlags usage,
                                          VkMemoryPropertyFlags memoryFlags,
                                          bool deviceAddress,
                                          Buffer& out,
                                          std::string& diagnostic) const
{
    return gpuResources_.CreateBuffer(size, usage, memoryFlags, deviceAddress, out, diagnostic);
}

void PresentableTinyRtScene::DestroyBuffer(Buffer& buffer) const
{
    gpuResources_.DestroyBuffer(buffer);
}

bool PresentableTinyRtScene::WriteBuffer(const Buffer& buffer,
                                         const void* data,
                                         const VkDeviceSize size,
                                         const char* label,
                                         std::string& diagnostic) const
{
    return gpuResources_.WriteBuffer(buffer, data, size, label, diagnostic);
}

VkDeviceAddress PresentableTinyRtScene::BufferAddress(VkBuffer buffer) const
{
    return gpuResources_.BufferAddress(buffer);
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
    const std::uint32_t memoryType = gpuResources_.FindMemoryType(
        requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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
    return CreateTexture(path, format, 512u, 512u, 5u, texture, diagnostic);
}

bool PresentableTinyRtScene::CreateTexture(const std::string& path,
                                           VkFormat format,
                                           std::uint32_t width,
                                           std::uint32_t height,
                                           std::uint32_t layers,
                                           TextureArray& texture,
                                           std::string& diagnostic)
{
    const bool ktx2 = path.ends_with(".ktx2");
    const bool astc4 = format == VK_FORMAT_ASTC_4x4_UNORM_BLOCK || format == VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
    const bool astc6 = format == VK_FORMAT_ASTC_6x6_UNORM_BLOCK || format == VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
    const VkDeviceSize layerByteSize = astc4
        ? static_cast<VkDeviceSize>((width + 3u) / 4u) * ((height + 3u) / 4u) * 16u
        : (astc6
               ? static_cast<VkDeviceSize>((width + 5u) / 6u) * ((height + 5u) / 6u) * 16u
               : static_cast<VkDeviceSize>(width) * height * 4u);
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
    struct MipPayload
    {
        VkDeviceSize bufferOffset = 0u;
        VkDeviceSize layerByteSize = 0u;
        std::uint32_t width = 0u;
        std::uint32_t height = 0u;
    };
    std::vector<MipPayload> mipPayloads;
    std::vector<std::uint8_t> pixels;
    std::uint32_t mipLevels = 1u;
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
        mipLevels = read32(40u);
        const bool validHeader = mipLevels > 0u && mipLevels <= 16u &&
                                 fileBytes.size() >= 80u + static_cast<std::size_t>(mipLevels) * 24u &&
                                 std::equal(identifier.begin(), identifier.end(), fileBytes.begin()) &&
                                 read32(12u) == static_cast<std::uint32_t>(format) && read32(16u) == 1u &&
                                 read32(20u) == width && read32(24u) == height && read32(28u) == 0u &&
                                 (read32(32u) == layers || (layers == 1u && read32(32u) == 0u)) &&
                                 read32(36u) == 1u &&
                                 read32(44u) == 0u;
        if (!validHeader)
        {
            diagnostic = "PBR KTX2 array has an unsupported header, format, or payload size: " + path;
            return false;
        }
        for (std::uint32_t level = 0u; level < mipLevels; ++level)
        {
            const std::size_t entry = 80u + static_cast<std::size_t>(level) * 24u;
            const std::uint64_t levelOffset = read64(entry);
            const std::uint64_t levelLength = read64(entry + 8u);
            const std::uint64_t levelUncompressedLength = read64(entry + 16u);
            const std::uint32_t mipWidth = std::max(width >> level, 1u);
            const std::uint32_t mipHeight = std::max(height >> level, 1u);
            const VkDeviceSize mipLayerBytes = astc4
                ? static_cast<VkDeviceSize>((mipWidth + 3u) / 4u) * ((mipHeight + 3u) / 4u) * 16u
                : (astc6
                       ? static_cast<VkDeviceSize>((mipWidth + 5u) / 6u) * ((mipHeight + 5u) / 6u) * 16u
                       : static_cast<VkDeviceSize>(mipWidth) * mipHeight * 4u);
            const VkDeviceSize expectedLevelBytes = mipLayerBytes * layers;
            if (levelLength != expectedLevelBytes || levelUncompressedLength != expectedLevelBytes ||
                levelOffset > fileBytes.size() || levelLength > fileBytes.size() - static_cast<std::size_t>(levelOffset))
            {
                diagnostic = "PBR KTX2 array has an unsupported mip payload size: " + path;
                return false;
            }
            const VkDeviceSize destinationOffset = pixels.size();
            pixels.insert(pixels.end(),
                          fileBytes.begin() + static_cast<std::ptrdiff_t>(levelOffset),
                          fileBytes.begin() + static_cast<std::ptrdiff_t>(levelOffset + levelLength));
            mipPayloads.push_back({destinationOffset, mipLayerBytes, mipWidth, mipHeight});
        }
    }
    else
    {
        if (fileBytes.size() != static_cast<std::size_t>(byteSize))
        {
            diagnostic = "Raw PBR texture array has the wrong size: " + path;
            return false;
        }
        pixels = std::move(fileBytes);
        mipPayloads.push_back({0u, layerByteSize, width, height});
    }

    Buffer staging;
    const VkDeviceSize uploadByteSize = pixels.size();
    if (!CreateBuffer(uploadByteSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false, staging, diagnostic)) return false;
    if (!WriteBuffer(staging, pixels.data(), uploadByteSize, "PBR texture staging", diagnostic))
    {
        DestroyBuffer(staging);
        return false;
    }

    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent = {width, height, 1u};
    imageInfo.mipLevels = mipLevels;
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
    const std::uint32_t memoryType = gpuResources_.FindMemoryType(
        requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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
        const std::vector<MipPayload>* mipPayloads;
        std::uint32_t mipLevels;
        std::uint32_t layers;
    } upload{this, &staging, &texture, &mipPayloads, mipLevels, layers};
    const auto recordUpload = [](VkCommandBuffer commandBuffer, void* userData) {
        auto* data = static_cast<UploadData*>(userData);
        VkImageMemoryBarrier toTransfer{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.image = data->texture->image;
        toTransfer.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, data->mipLevels, 0u, data->layers};
        toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, nullptr, 0u, nullptr, 1u, &toTransfer);
        std::vector<VkBufferImageCopy> copies;
        copies.reserve(static_cast<std::size_t>(data->mipLevels) * data->layers);
        for (std::uint32_t level = 0u; level < data->mipLevels; ++level)
        {
            const MipPayload& mip = (*data->mipPayloads)[level];
            for (std::uint32_t layer = 0u; layer < data->layers; ++layer)
            {
                VkBufferImageCopy copy{};
                copy.bufferOffset = mip.bufferOffset + static_cast<VkDeviceSize>(layer) * mip.layerByteSize;
                copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, layer, 1u};
                copy.imageExtent = {mip.width, mip.height, 1u};
                copies.push_back(copy);
            }
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
    viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, mipLevels, 0u, layers};
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
    sampler.maxLod = VK_LOD_CLAMP_NONE;
    if (vkCreateSampler(device_, &sampler, nullptr, &materialSampler_) != VK_SUCCESS)
    {
        diagnostic = "Failed to create PBR material sampler.";
        return false;
    }
    return true;
}

bool PresentableTinyRtScene::CreateLichTextures(const std::string& directory, std::string& diagnostic)
{
    const auto path = [&directory](const char* name) { return directory + "/" + name; };
#if defined(__ANDROID__)
    if (!SupportsTextureArrayFormat(VK_FORMAT_ASTC_6x6_SRGB_BLOCK))
    {
        diagnostic = "The Android lich path requires sampled ASTC 6x6 support; no uncompressed mobile fallback is allowed.";
        return false;
    }
    const std::string basePath = path("base-color-2048-astc6x6.ktx2");
    const std::string emissivePath = path("emissive-2048-astc6x6.ktx2");
    if (!CreateTexture(basePath, VK_FORMAT_ASTC_6x6_SRGB_BLOCK, 2048u, 2048u, 1u, lichBaseColor_, diagnostic) ||
        !CreateTexture(emissivePath, VK_FORMAT_ASTC_6x6_SRGB_BLOCK, 2048u, 2048u, 1u, lichEmissive_, diagnostic))
    {
        return false;
    }
    materialEncoding_ += " + strict ASTC 6x6 lich";
#else
    const std::string basePath = path("base-color-2048-rgba8.ktx2");
    const std::string emissivePath = path("emissive-2048-rgba8.ktx2");
    if (!CreateTexture(basePath, VK_FORMAT_R8G8B8A8_SRGB, 2048u, 2048u, 1u, lichBaseColor_, diagnostic) ||
        !CreateTexture(emissivePath, VK_FORMAT_R8G8B8A8_SRGB, 2048u, 2048u, 1u, lichEmissive_, diagnostic))
    {
        return false;
    }
    materialEncoding_ += " + raw RGBA8 KTX2 lich";
#endif
    return true;
}

bool PresentableTinyRtScene::LoadStaticHeldItemAssets(
    const std::string& developmentDirectory,
    const std::string& productionAssetRoot,
    std::string& diagnostic)
{
    (void)developmentDirectory;
    genericStaticAssetEnabled_ = !productionAssetRoot.empty();
    productionHeldItemAssetsEnabled_ = !productionAssetRoot.empty();
    developmentStaticAssetDirectory_.clear();
    staticTextureDirectory_.clear();
    developmentStaticAsset_ = {};
    productionTorchAsset_ = {};
    if (!productionHeldItemAssetsEnabled_)
    {
        diagnostic = "Production held-item asset root is required; procedural sword/torch bodies are retired.";
        return false;
    }

    const std::filesystem::path root(productionAssetRoot);
    const auto swordDirectory = root / "models/weapons/runtime";
    const auto torchDirectory = root / "models/props/runtime";
    horde::scene::assets::AssetManifest swordManifest;
    horde::scene::assets::AssetManifest torchManifest;
    if (!horde::scene::assets::AssetManifest::Load(
            swordDirectory / "asset.manifest.json", swordManifest, diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(
            swordDirectory / "gothic-arming-sword-rh-lod0.runtime.glb",
            swordManifest,
            developmentStaticAsset_,
            diagnostic) ||
        !horde::scene::assets::AssetManifest::Load(
            torchDirectory / "asset.manifest.json", torchManifest, diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(
            torchDirectory / "gothic-hand-torch-lod0.runtime.glb",
            torchManifest,
            productionTorchAsset_,
            diagnostic))
        return false;
    staticTextureDirectory_ = (root / "textures/held-items/runtime").string();
    std::array<StaticRtAssetRegistration, 2u> registrations{{
        {3u, 0x53574f52u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),
         0u, &developmentStaticAsset_},
        {1u, 0x544f5243u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),
         1u, &productionTorchAsset_},
    }};
    return staticMeshSlot_.Initialize(registrations, diagnostic);
}

bool PresentableTinyRtScene::CreateStaticMeshResources(std::string& diagnostic)
{
    const VkMemoryPropertyFlags uploadMemory =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    const auto& instances = staticMeshSlot_.InstanceMetadata();
    const auto& primitives = staticMeshSlot_.PrimitiveMetadata();
    const auto& materials = staticMeshSlot_.Materials();
    const auto& vertices = staticMeshSlot_.Vertices();
    const auto& indices = staticMeshSlot_.Indices();
    const auto& geometryTransforms = staticMeshSlot_.GeometryTransforms();
    const auto createAndWrite = [this, uploadMemory, &diagnostic](
        const void* data,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        bool address,
        const char* label,
        Buffer& buffer) {
        const std::array<std::uint8_t, 16u> zeros{};
        const VkDeviceSize actualSize = std::max<VkDeviceSize>(size, zeros.size());
        if (!CreateBuffer(actualSize, usage, uploadMemory, address, buffer, diagnostic)) return false;
        return WriteBuffer(buffer, size == 0u ? zeros.data() : data, actualSize, label, diagnostic);
    };

    const VkBufferUsageFlags storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    const VkBufferUsageFlags geometry = storage |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    if (!createAndWrite(instances.data(), sizeof(instances), storage, false,
                        "RT instance metadata", instanceMetadataBuffer_) ||
        !createAndWrite(primitives.data(), primitives.size() * sizeof(RtPrimitiveMetadata),
                        storage, false, "RT primitive metadata", primitiveMetadataBuffer_) ||
        !createAndWrite(materials.data(), materials.size() * sizeof(RtMaterialGpu),
                        storage, false, "RT material metadata", materialMetadataBuffer_) ||
        !createAndWrite(vertices.data(),
                        vertices.size() * sizeof(horde::scene::assets::StaticRtVertex),
                        geometry, true, "static RT vertices", staticVertexBuffer_) ||
        !createAndWrite(indices.data(), indices.size() * sizeof(std::uint32_t),
                        geometry, true, "static RT indices", staticIndexBuffer_) ||
        !createAndWrite(geometryTransforms.data(),
                        geometryTransforms.size() * sizeof(geometryTransforms.front()),
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                        true, "static RT geometry transforms",
                        staticGeometryTransformBuffer_))
    {
        return false;
    }

    if (!genericStaticAssetEnabled_) return true;
    const RtTextureArrayCounts textureCounts = staticMeshSlot_.TextureArrayCounts();
    const std::array<std::uint32_t, 4u> actualTextureLayers{{
        std::max(textureCounts.baseColor, 1u),
        std::max(textureCounts.normal, 1u),
        std::max(textureCounts.orm, 1u),
        std::max(textureCounts.emissive, 1u)}};
    const std::uint32_t textureDimension = productionHeldItemAssetsEnabled_ ? 1024u : 512u;
    for (std::uint32_t dimension = textureDimension; dimension > 0u; dimension >>= 1u)
        staticTextureBytes_ += static_cast<VkDeviceSize>(dimension) * dimension * 4u *
            (actualTextureLayers[0] + actualTextureLayers[1] +
             actualTextureLayers[2] + actualTextureLayers[3]);
    const std::filesystem::path assetDirectory(staticTextureDirectory_);
    const auto path = [&assetDirectory](const char* name) {
        return (assetDirectory / name).string();
    };
#if defined(__ANDROID__)
    if (!productionHeldItemAssetsEnabled_)
    {
        diagnostic = "Development static assets cannot be enabled in an Android package.";
        return false;
    }
    if (!SupportsTextureArrayFormat(VK_FORMAT_ASTC_6x6_SRGB_BLOCK) ||
        !SupportsTextureArrayFormat(VK_FORMAT_ASTC_4x4_UNORM_BLOCK) ||
        !SupportsTextureArrayFormat(VK_FORMAT_ASTC_6x6_UNORM_BLOCK))
    {
        diagnostic = "Production held-item PBR requires sampled ASTC 4x4/6x6 support; no uncompressed Android fallback is allowed.";
        return false;
    }
    if (!CreateTexture(path("base-color.android.ktx2"), VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
                       textureDimension, textureDimension, actualTextureLayers[0], staticBaseColor_, diagnostic) ||
        !CreateTexture(path("normal.android.ktx2"), VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
                       textureDimension, textureDimension, actualTextureLayers[1], staticNormal_, diagnostic) ||
        !CreateTexture(path("orm.android.ktx2"), VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
                       textureDimension, textureDimension, actualTextureLayers[2], staticOrm_, diagnostic) ||
        !CreateTexture(path("emissive.android.ktx2"), VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
                       textureDimension, textureDimension, actualTextureLayers[3], staticEmissive_, diagnostic))
        return false;
#else
    if (!CreateTexture(path("base-color.windows.ktx2"), VK_FORMAT_R8G8B8A8_SRGB,
                       textureDimension, textureDimension, actualTextureLayers[0], staticBaseColor_, diagnostic) ||
        !CreateTexture(path("normal.windows.ktx2"), VK_FORMAT_R8G8B8A8_UNORM,
                       textureDimension, textureDimension, actualTextureLayers[1], staticNormal_, diagnostic) ||
        !CreateTexture(path("orm.windows.ktx2"), VK_FORMAT_R8G8B8A8_UNORM,
                       textureDimension, textureDimension, actualTextureLayers[2], staticOrm_, diagnostic) ||
        !CreateTexture(path("emissive.windows.ktx2"), VK_FORMAT_R8G8B8A8_SRGB,
                       textureDimension, textureDimension, actualTextureLayers[3], staticEmissive_, diagnostic))
    {
        return false;
    }
#endif
    return true;
}

void PresentableTinyRtScene::DestroyAccelerationStructure(AccelerationStructure& accelerationStructure)
{
    gpuResources_.DestroyAccelerationStructure(accelerationStructure);
}

bool PresentableTinyRtScene::BuildAccelerationStructures(std::string& diagnostic)
{
    struct Vertex
    {
        float position[3];
    };
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<Vertex> waterfallVertices;
    std::vector<std::uint32_t> waterfallIndices;
    std::vector<std::uint32_t> worldSurfaceCodes;
    vertices.reserve(3072u);
    indices.reserve(4608u);
    worldSurfaceCodes.reserve(1536u);
    waterfallVertices.reserve(144u);
    waterfallIndices.reserve(240u);

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
        // Keep the existing CPU/shader material ABI stable. Water is appended
        // because every world primitive reads this numeric code in raygen.
        SurfaceWater = 10u,
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
    const auto addWaterfallQuad = [&waterfallVertices, &waterfallIndices](
                                      const Vertex& a,
                                      const Vertex& b,
                                      const Vertex& c,
                                      const Vertex& d) {
        const std::uint32_t base = static_cast<std::uint32_t>(waterfallVertices.size());
        waterfallVertices.push_back(a);
        waterfallVertices.push_back(b);
        waterfallVertices.push_back(c);
        waterfallVertices.push_back(d);
        waterfallIndices.insert(
            waterfallIndices.end(), {base, base + 1u, base + 2u, base, base + 2u, base + 3u});
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
    const auto addWorldTriangle = [&addTriangle, &worldSurfaceCodes, &surfaceCode](
                                      const Vertex& a,
                                      const Vertex& b,
                                      const Vertex& c,
                                      SurfaceMaterial material,
                                      SurfaceNormal normal) {
        addTriangle(a, b, c);
        worldSurfaceCodes.push_back(surfaceCode(material, normal));
    };
    const auto addWorldBox = [&addWorldQuad](float minX, float minY, float minZ,
                                             float maxX, float maxY, float maxZ,
                                             SurfaceMaterial material) {
        addWorldQuad({{minX, minY, minZ}}, {{minX, maxY, minZ}}, {{maxX, maxY, minZ}}, {{maxX, minY, minZ}}, material, SurfaceForward);
        addWorldQuad({{maxX, minY, maxZ}}, {{maxX, maxY, maxZ}}, {{minX, maxY, maxZ}}, {{minX, minY, maxZ}}, material, SurfaceBack);
        addWorldQuad({{minX, minY, maxZ}}, {{minX, maxY, maxZ}}, {{minX, maxY, minZ}}, {{minX, minY, minZ}}, material, SurfaceRight);
        addWorldQuad({{maxX, minY, minZ}}, {{maxX, maxY, minZ}}, {{maxX, maxY, maxZ}}, {{maxX, minY, maxZ}}, material, SurfaceLeft);
        addWorldQuad({{minX, maxY, minZ}}, {{minX, maxY, maxZ}}, {{maxX, maxY, maxZ}}, {{maxX, maxY, minZ}}, material, SurfaceDown);
        addWorldQuad({{minX, minY, maxZ}}, {{minX, minY, minZ}}, {{maxX, minY, minZ}}, {{maxX, minY, maxZ}}, material, SurfaceUp);
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
    // Close the starting chamber behind the player. This is real RT geometry,
    // preventing a 180-degree turn at spawn from exposing the exterior sky.
    addWorldQuad({{-1.85f, -0.95f, 3.4f}}, {{-1.85f, 1.35f, 3.4f}}, {{1.85f, 1.35f, 3.4f}}, {{1.85f, -0.95f, 3.4f}}, SurfaceMossyStone, SurfaceBack);
    // The former sealed far wall is split around a 1.8 m doorway into the
    // extended showcase route. The matching hidden shell is split below too.
    addWorldQuad({{-1.85f, -0.95f, -6.4f}}, {{-0.90f, -0.95f, -6.4f}}, {{-0.90f, 1.35f, -6.4f}}, {{-1.85f, 1.35f, -6.4f}}, SurfaceMossyStone, SurfaceForward);
    addWorldQuad({{0.90f, -0.95f, -6.4f}}, {{1.85f, -0.95f, -6.4f}}, {{1.85f, 1.35f, -6.4f}}, {{0.90f, 1.35f, -6.4f}}, SurfaceMossyStone, SurfaceForward);
    addWorldQuad({{-0.90f, 0.85f, -6.4f}}, {{0.90f, 0.85f, -6.4f}}, {{0.90f, 1.35f, -6.4f}}, {{-0.90f, 1.35f, -6.4f}}, SurfaceMossyStone, SurfaceForward);
    // Give the room-two portal real RT depth instead of three paper-thin cards.
    addWorldBox(-1.20f, -0.95f, -3.55f, -0.78f, 0.95f, -3.25f, SurfaceMossyStone);
    addWorldBox(0.78f, -0.95f, -3.55f, 1.20f, 0.95f, -3.25f, SurfaceMossyStone);
    addWorldBox(-1.20f, 0.78f, -3.55f, 1.20f, 1.18f, -3.25f, SurfaceMossyStone);
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
    // One bounded thin clear pane closes the irregular roof breach. Primary
    // rays route through the glass material while visibility rays treat it as
    // non-occluding, preserving the physically open moon direction.
    addWorldQuad({{-0.55f, 1.33f, -3.45f}}, {{-0.72f, 1.33f, -5.20f}}, {{0.62f, 1.33f, -5.05f}}, {{0.32f, 1.33f, -3.55f}}, SurfaceClearGlass, SurfaceDown);
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
    // It leaves the room-two roof breach unobstructed. Appending it here
    // preserves every existing material index.
    addWorldQuad({{-1.92f, -1.02f, 3.4f}}, {{-1.92f, -1.02f, -6.47f}}, {{-1.92f, 1.42f, -6.47f}}, {{-1.92f, 1.42f, 3.4f}}, SurfaceHiddenShell, SurfaceRight);
    addWorldQuad({{1.92f, -1.02f, -6.47f}}, {{1.92f, -1.02f, 3.4f}}, {{1.92f, 1.42f, 3.4f}}, {{1.92f, 1.42f, -6.47f}}, SurfaceHiddenShell, SurfaceLeft);
    addWorldQuad({{-1.92f, -1.02f, 3.4f}}, {{1.92f, -1.02f, 3.4f}}, {{1.92f, -1.02f, -6.47f}}, {{-1.92f, -1.02f, -6.47f}}, SurfaceHiddenShell, SurfaceUp);
    addWorldQuad({{-1.92f, -1.02f, 3.47f}}, {{-1.92f, 1.42f, 3.47f}}, {{1.92f, 1.42f, 3.47f}}, {{1.92f, -1.02f, 3.47f}}, SurfaceHiddenShell, SurfaceBack);
    addWorldQuad({{-1.92f, -1.02f, -6.47f}}, {{-0.90f, -1.02f, -6.47f}}, {{-0.90f, 1.42f, -6.47f}}, {{-1.92f, 1.42f, -6.47f}}, SurfaceHiddenShell, SurfaceForward);
    addWorldQuad({{0.90f, -1.02f, -6.47f}}, {{1.92f, -1.02f, -6.47f}}, {{1.92f, 1.42f, -6.47f}}, {{0.90f, 1.42f, -6.47f}}, SurfaceHiddenShell, SurfaceForward);
    addWorldQuad({{-0.90f, 0.85f, -6.47f}}, {{0.90f, 0.85f, -6.47f}}, {{0.90f, 1.42f, -6.47f}}, {{-0.90f, 1.42f, -6.47f}}, SurfaceHiddenShell, SurfaceForward);

    // Slice A extends the room with static, geometry-only RT proof spaces. It
    // deliberately adds no new flame, glass or mirror surface: the brackets
    // are unlit, the transmission frame is empty and the final mirror frame
    // has only its dry-stone wall behind it.
    constexpr float routeFloor = -0.95f;
    constexpr float routeCeiling = 1.35f;
    const auto addRouteFloor = [&addWorldQuad](float minX, float minZ, float maxX, float maxZ, SurfaceMaterial material) {
        addWorldQuad({{minX, routeFloor, maxZ}}, {{maxX, routeFloor, maxZ}},
                     {{maxX, routeFloor, minZ}}, {{minX, routeFloor, minZ}}, material, SurfaceUp);
    };
    const auto addRouteCeiling = [&addWorldQuad](float minX, float minZ, float maxX, float maxZ) {
        addWorldQuad({{minX, routeCeiling, maxZ}}, {{minX, routeCeiling, minZ}},
                     {{maxX, routeCeiling, minZ}}, {{maxX, routeCeiling, maxZ}}, SurfaceDryStone, SurfaceDown);
    };
    const auto addRouteWallX = [&addWorldQuad](float x, float minZ, float maxZ, SurfaceNormal normal) {
        if (normal == SurfaceRight)
        {
            addWorldQuad({{x, routeFloor, maxZ}}, {{x, routeFloor, minZ}},
                         {{x, routeCeiling, minZ}}, {{x, routeCeiling, maxZ}}, SurfaceMossyStone, normal);
        }
        else
        {
            addWorldQuad({{x, routeFloor, minZ}}, {{x, routeFloor, maxZ}},
                         {{x, routeCeiling, maxZ}}, {{x, routeCeiling, minZ}}, SurfaceMossyStone, normal);
        }
    };
    const auto addRouteWallZ = [&addWorldQuad](float z, float minX, float maxX, SurfaceNormal normal) {
        if (normal == SurfaceForward)
        {
            addWorldQuad({{minX, routeFloor, z}}, {{maxX, routeFloor, z}},
                         {{maxX, routeCeiling, z}}, {{minX, routeCeiling, z}}, SurfaceMossyStone, normal);
        }
        else
        {
            addWorldQuad({{minX, routeFloor, z}}, {{minX, routeCeiling, z}},
                         {{maxX, routeCeiling, z}}, {{maxX, routeFloor, z}}, SurfaceMossyStone, normal);
        }
    };

    // A shallow stone surround gives the new 1.8 m opening real RT depth. Its
    // clear width stays exactly x=-0.9..0.9 throughout the player's height.
    addWorldBox(-1.08f, -0.95f, -6.52f, -0.90f, 0.82f, -6.28f, SurfaceMossyStone);
    addWorldBox(0.90f, -0.95f, -6.52f, 1.08f, 0.82f, -6.28f, SurfaceMossyStone);
    addWorldBox(-1.08f, 0.82f, -6.52f, -0.48f, 1.12f, -6.28f, SurfaceMossyStone);
    addWorldBox(0.48f, 0.82f, -6.52f, 1.08f, 1.12f, -6.28f, SurfaceMossyStone);
    addWorldBox(-0.48f, 0.96f, -6.52f, 0.48f, 1.20f, -6.28f, SurfaceMossyStone);

    // Four overlapping 2.4 m legs form the three-turn shadow corridor. The
    // coplanar overlaps carry the same material metadata and prevent cracks at
    // the bends without introducing thin filler triangles.
    addRouteFloor(-1.20f, -10.0f, 1.20f, -6.4f, SurfaceWetCobble);
    addRouteCeiling(-1.20f, -10.0f, 1.20f, -6.4f);
    addRouteFloor(0.0f, -11.2f, 4.80f, -8.8f, SurfaceWetCobble);
    addRouteCeiling(0.0f, -11.2f, 4.80f, -8.8f);
    addRouteFloor(3.60f, -15.2f, 6.0f, -10.0f, SurfaceWetCobble);
    addRouteCeiling(3.60f, -15.2f, 6.0f, -10.0f);
    addRouteFloor(-2.50f, -16.4f, 4.80f, -14.0f, SurfaceWetCobble);
    // A broken roof slot turns the former abstract torch failure into a
    // physical drench at the final zig-zag exit. The rim is deliberately
    // inside the existing route envelope so collision and replay remain
    // unchanged while primary, reflection, and transmission rays see depth.
    // Extend the irregular breach to contain the actual kMoonDirection
    // footprint at the catchment. The moon ray now reaches the cobbles through
    // geometry and can be blocked by the player; no water-only light is added.
    constexpr float waterSlotMinX = -2.90f;
    constexpr float waterSlotMaxX = -1.58f;
    constexpr float waterSlotMinZ = -16.10f;
    constexpr float waterSlotMaxZ = -14.72f;
    addRouteCeiling(-2.50f, -16.4f, waterSlotMinX, -14.0f);
    addRouteCeiling(waterSlotMaxX, -16.4f, 4.80f, -14.0f);
    addRouteCeiling(waterSlotMinX, -16.4f, waterSlotMaxX, waterSlotMinZ);
    addRouteCeiling(waterSlotMinX, waterSlotMaxZ, waterSlotMaxX, -14.0f);

    // Outer perimeter of the corridor union. Open seams at x=-2.5 and x=-8.5
    // connect directly into the skylight room and torch passage respectively.
    addRouteWallX(-1.20f, -10.0f, -6.4f, SurfaceRight);
    addRouteWallZ(-10.0f, -1.20f, 0.0f, SurfaceForward);
    addRouteWallX(0.0f, -11.2f, -10.0f, SurfaceRight);
    addRouteWallX(1.20f, -8.8f, -6.4f, SurfaceLeft);
    addRouteWallZ(-8.8f, 1.20f, 2.05f, SurfaceBack);
    addRouteWallZ(-8.8f, 3.10f, 4.80f, SurfaceBack);
    addRouteWallX(4.80f, -10.0f, -8.8f, SurfaceLeft);
    addRouteWallZ(-11.2f, 0.0f, 3.60f, SurfaceForward);
    addRouteWallZ(-10.0f, 4.80f, 6.0f, SurfaceBack);
    addRouteWallX(3.60f, -14.0f, -11.2f, SurfaceRight);
    addRouteWallX(6.0f, -15.2f, -10.0f, SurfaceLeft);
    addRouteWallZ(-15.2f, 4.80f, 6.0f, SurfaceForward);
    addRouteWallZ(-14.0f, -2.50f, 3.60f, SurfaceBack);
    addRouteWallX(4.80f, -16.4f, -15.2f, SurfaceLeft);
    addRouteWallZ(-16.4f, -2.50f, 4.80f, SurfaceForward);

    constexpr float waterShaftBase = routeCeiling - 0.02f;
    addWorldBox(waterSlotMinX - 0.16f, waterShaftBase, waterSlotMinZ - 0.16f,
                waterSlotMinX, 2.18f, waterSlotMaxZ + 0.16f, SurfaceMossyStone);
    addWorldBox(waterSlotMaxX, waterShaftBase, waterSlotMinZ - 0.16f,
                waterSlotMaxX + 0.16f, 2.18f, waterSlotMaxZ + 0.16f, SurfaceMossyStone);
    addWorldBox(waterSlotMinX, waterShaftBase, waterSlotMinZ - 0.16f,
                waterSlotMaxX, 2.18f, waterSlotMinZ, SurfaceMossyStone);
    addWorldBox(waterSlotMinX, waterShaftBase, waterSlotMaxZ,
                waterSlotMaxX, 2.18f, waterSlotMaxZ + 0.16f, SurfaceMossyStone);

    // Only the three falling streams live in this dedicated local-space mesh.
    // Catchment, runnel and drain stay in the static world and collision is
    // unchanged. The TLAS transform scales local X around the authored centre.
    // The closed, faceted streams replace the former broad water cards. The
    // eight-sided elliptical rings have real depth for Fresnel/refraction and
    // real air gaps between them; no shader coverage mask or overlay is used.
    // The main thin stream brushes the authored z=-15.20 route while two narrow
    // satellites break up its silhouette without sealing the doorway. All three
    // share a linear taper: falling water accelerates and narrows toward the pool,
    // and the shader can solve the matching elliptical exit surface exactly.
    struct WaterStream
    {
        float centreZ;
        float radiusX;
        float radiusZ;
    };
    constexpr std::array<float, 6u> waterStreamY{{2.16f, 1.58f, 1.01f, 0.43f, -0.18f, -0.91f}};
    constexpr std::array<float, 8u> ringCos{{
        1.0f, 0.70710678f, 0.0f, -0.70710678f,
        -1.0f, -0.70710678f, 0.0f, 0.70710678f,
    }};
    constexpr std::array<float, 8u> ringSin{{
        0.0f, 0.70710678f, 1.0f, 0.70710678f,
        0.0f, -0.70710678f, -1.0f, -0.70710678f,
    }};
    constexpr std::array<WaterStream, 3u> waterStreams{{
        {-15.26f, 0.006f, 0.065f},
        {-15.44f, 0.003f, 0.014f},
        {-15.06f, 0.003f, 0.012f},
    }};
    static_assert(waterStreamY.size() == 6u && ringCos.size() == 8u &&
                  waterStreams.size() == 3u,
                  "falling water must retain six rings and three eight-facet streams");
    static_assert((-15.26f - 0.065f) - (-15.44f + 0.014f) > 0.10f &&
                  (-15.06f - 0.012f) - (-15.26f + 0.065f) > 0.10f,
                  "falling water streams must retain real ten-centimetre air gaps");
    for (const WaterStream& stream : waterStreams)
    {
        std::array<std::array<Vertex, 8u>, 6u> rings{};
        for (std::size_t level = 0u; level < waterStreamY.size(); ++level)
        {
            const float fallProgress = std::clamp(
                (waterStreamY[level] + 0.91f) / (2.16f + 0.91f), 0.0f, 1.0f);
            const float radiusScale = 0.70f + fallProgress * 0.30f;
            for (std::size_t facet = 0u; facet < ringCos.size(); ++facet)
            {
                rings[level][facet] = Vertex{{
                    ringCos[facet] * stream.radiusX * radiusScale,
                    waterStreamY[level],
                    (stream.centreZ + 15.26f) + ringSin[facet] * stream.radiusZ * radiusScale,
                }};
            }
        }
        for (std::size_t segment = 0u; segment + 1u < waterStreamY.size(); ++segment)
        {
            for (std::size_t facet = 0u; facet < ringCos.size(); ++facet)
            {
                const std::size_t nextFacet = (facet + 1u) % ringCos.size();
                addWaterfallQuad(rings[segment + 1u][facet], rings[segment][facet],
                                 rings[segment][nextFacet], rings[segment + 1u][nextFacet]);
            }
        }
    }
    // A rounded, slightly irregular catchment sits over the wet cobbles. The
    // sixteen-triangle fan keeps a natural curved silhouette without a texture,
    // alpha card, collision change, or screen-space mask.
    constexpr Vertex waterImpactCentre{{-2.32f, -0.925f, -15.26f}};
    constexpr std::array<std::array<float, 2u>, 16u> roundedCatchmentRim{{
        {{-2.32f, -14.72f}}, {{-2.13f, -14.77f}},
        {{-1.96f, -14.90f}}, {{-1.85f, -15.06f}},
        {{-1.82f, -15.26f}}, {{-1.86f, -15.47f}},
        {{-1.98f, -15.65f}}, {{-2.15f, -15.76f}},
        {{-2.35f, -15.80f}}, {{-2.55f, -15.75f}},
        {{-2.73f, -15.61f}}, {{-2.83f, -15.42f}},
        {{-2.86f, -15.21f}}, {{-2.79f, -14.99f}},
        {{-2.65f, -14.83f}}, {{-2.48f, -14.74f}},
    }};
    for (std::size_t point = 0u; point < roundedCatchmentRim.size(); ++point)
    {
        const std::size_t next = (point + 1u) % roundedCatchmentRim.size();
        const Vertex rimA{{roundedCatchmentRim[point][0], -0.925f,
                           roundedCatchmentRim[point][1]}};
        const Vertex rimB{{roundedCatchmentRim[next][0], -0.925f,
                           roundedCatchmentRim[next][1]}};
        addWorldTriangle(waterImpactCentre, rimA, rimB, SurfaceWater, SurfaceUp);
    }

    // The catchment spills into a shallow stone runnel that narrows toward a
    // real recessed drain in the skylight chamber. These connected surfaces
    // remain above the collision floor and therefore do not alter traversal.
    addWorldQuad({{-2.48f, -0.924f, -14.96f}}, {{-2.48f, -0.924f, -15.54f}},
                 {{-3.10f, -0.926f, -15.48f}}, {{-3.10f, -0.926f, -15.02f}},
                 SurfaceWater, SurfaceUp);
    addWorldQuad({{-3.10f, -0.926f, -15.02f}}, {{-3.10f, -0.926f, -15.48f}},
                 {{-3.72f, -0.928f, -15.46f}}, {{-3.72f, -0.928f, -14.98f}},
                 SurfaceWater, SurfaceUp);
    addWorldQuad({{-3.72f, -0.928f, -14.98f}}, {{-3.72f, -0.928f, -15.46f}},
                 {{-4.35f, -0.930f, -15.40f}}, {{-4.35f, -0.930f, -15.05f}},
                 SurfaceWater, SurfaceUp);
    addWorldQuad({{-4.35f, -0.930f, -15.05f}}, {{-4.35f, -0.930f, -15.40f}},
                 {{-4.88f, -0.931f, -15.36f}}, {{-4.88f, -0.931f, -15.08f}},
                 SurfaceWater, SurfaceUp);
    constexpr float runoffDrainLipX = -5.30f;
    addWorldQuad({{-4.88f, -0.931f, -15.08f}}, {{-4.88f, -0.931f, -15.36f}},
                 {{runoffDrainLipX, -0.932f, -15.33f}},
                 {{runoffDrainLipX, -0.932f, -15.11f}}, SurfaceWater, SurfaceUp);
    // The final film folds below the grate instead of being buried beneath the
    // cobbles before it arrives. Its upper edge overlaps the drain throat.
    addWorldQuad({{runoffDrainLipX, -0.932f, -15.11f}},
                 {{runoffDrainLipX, -0.932f, -15.33f}},
                 {{-5.38f, -1.08f, -15.30f}}, {{-5.38f, -1.08f, -15.14f}},
                 SurfaceWater, SurfaceUp);

    // Dark throat and metal crossbars make the sink legible through the clear
    // runoff while the final sloped water surface visibly disappears below
    // the floor plane.
    addWorldQuad({{-5.30f, -0.944f, -14.94f}}, {{-4.91f, -0.944f, -14.94f}},
                 {{-4.91f, -0.944f, -15.46f}}, {{-5.30f, -0.944f, -15.46f}},
                 SurfaceDarkFigure, SurfaceUp);
    for (std::uint32_t bar = 0u; bar < 4u; ++bar)
    {
        const float drainZ = -15.40f + static_cast<float>(bar) * 0.14f;
        addWorldBox(-5.31f, -0.922f, drainZ, -4.90f, -0.900f, drainZ + 0.028f,
                    SurfaceAgedMetal);
    }

    // A shallow barred recess at the first bend creates strong moving shadow
    // lines while remaining outside the shared walkable rectangles.
    addRouteWallZ(-8.35f, 2.05f, 3.10f, SurfaceBack);
    addRouteWallX(2.05f, -8.8f, -8.35f, SurfaceRight);
    addRouteWallX(3.10f, -8.8f, -8.35f, SurfaceLeft);
    for (std::uint32_t i = 0u; i < 4u; ++i)
    {
        const float x = 2.20f + static_cast<float>(i) * 0.25f;
        addWorldBox(x, -0.78f, -8.82f, x + 0.045f, 0.82f, -8.76f, SurfaceAgedMetal);
    }

    // Keep the turns open and let the wall returns plus barred recess cast the
    // large torch shadows. Earlier low lintel boxes crossed the route walls,
    // producing the dark overhead slabs and coplanar striping seen in validation.

    // The skylight chamber floor is damp stone. Four roof slabs leave the
    // planned x=-6.7..-4.3, z=-16.6..-13.8 aperture physically open to sky.
    addRouteFloor(-8.50f, -18.0f, -2.50f, -12.4f, SurfaceDampGround);
    addRouteCeiling(-8.50f, -18.0f, -6.70f, -12.4f);
    addRouteCeiling(-4.30f, -18.0f, -2.50f, -12.4f);
    addRouteCeiling(-6.70f, -13.8f, -4.30f, -12.4f);
    addRouteCeiling(-6.70f, -18.0f, -4.30f, -16.6f);
    // A raised masonry well gives the aperture a readable 1.1 m depth from
    // oblique views. The inner clear opening remains exactly the planned
    // x=-6.7..-4.3, z=-16.6..-13.8 footprint all the way to open sky.
    // Sink the bases below the ceiling plane so the join stays closed without
    // leaving coplanar bottom faces to shimmer against the roof slabs.
    constexpr float shaftBase = routeCeiling - 0.02f;
    addWorldBox(-6.86f, shaftBase, -16.76f, -6.70f, 2.45f, -13.64f, SurfaceMossyStone);
    addWorldBox(-4.30f, shaftBase, -16.76f, -4.14f, 2.45f, -13.64f, SurfaceMossyStone);
    addWorldBox(-6.70f, shaftBase, -16.76f, -4.30f, 2.45f, -16.60f, SurfaceMossyStone);
    addWorldBox(-6.70f, shaftBase, -13.80f, -4.30f, 2.45f, -13.64f, SurfaceMossyStone);
    addRouteWallZ(-12.4f, -8.50f, -2.50f, SurfaceBack);
    addRouteWallZ(-18.0f, -8.50f, -2.50f, SurfaceForward);
    addRouteWallX(-2.50f, -18.0f, -16.4f, SurfaceLeft);
    addRouteWallX(-2.50f, -14.0f, -12.4f, SurfaceLeft);
    addRouteWallX(-8.50f, -18.0f, -16.8f, SurfaceRight);
    addRouteWallX(-8.50f, -13.6f, -12.4f, SurfaceRight);

    // One straight passage contains four five-metre bays. Brackets are aged
    // metal only: no SurfaceFlame primitive and no coloured illumination are
    // introduced in this blockout slice.
    addRouteFloor(-28.50f, -16.8f, -8.50f, -13.6f, SurfaceWetCobble);
    addRouteCeiling(-28.50f, -16.8f, -8.50f, -13.6f);
    addRouteFloor(-30.50f, -16.8f, -28.50f, -13.6f, SurfaceDryStone);
    addRouteCeiling(-30.50f, -16.8f, -28.50f, -13.6f);
    addRouteWallZ(-13.6f, -30.50f, -8.50f, SurfaceBack);
    addRouteWallZ(-16.8f, -30.50f, -8.50f, SurfaceForward);
    const std::array<float, 4u> torchBayCenters{{-11.0f, -16.0f, -21.0f, -26.0f}};
    for (float x : torchBayCenters)
    {
        addWorldBox(x - 0.12f, 0.24f, -13.66f, x + 0.12f, 0.82f, -13.54f, SurfaceAgedMetal);
        addWorldBox(x - 0.035f, 0.38f, -13.98f, x + 0.035f, 0.46f, -13.62f, SurfaceAgedMetal);
        addWorldBox(x - 0.12f, 0.42f, -14.04f, x + 0.12f, 0.50f, -13.92f, SurfaceAgedMetal);
        // The route-light selector activates one bay's direct-light estimate at
        // a time, but every sconce retains a small physical emissive flame.
        addWorldBox(x - 0.055f, 0.50f, -14.015f, x + 0.055f, 0.78f, -13.945f, SurfaceFlame);
    }

    // The authored threshold remains open. Its narrow jambs sit inside the
    // collision wall inset and the high lintel leaves the full central walking
    // lane unobstructed.
    addWorldBox(-29.62f, -0.95f, -16.80f, -29.38f, 0.88f, -16.62f, SurfaceMossyStone);
    addWorldBox(-29.62f, -0.95f, -13.78f, -29.38f, 0.88f, -13.60f, SurfaceMossyStone);
    addWorldBox(-29.62f, 0.88f, -16.80f, -29.38f, 1.20f, -13.60f, SurfaceMossyStone);

    // Dry final reveal room. The far-wall metal surround is an empty hero
    // mirror frame; its centre remains ordinary dry stone in Slice A.
    addRouteFloor(-36.90f, -18.4f, -30.50f, -12.0f, SurfaceDryStone);
    // Four fixed roof slabs leave a real finale aperture. A separate BLAS panel
    // below closes it until the defeated lich's authored roof sequence slides
    // the slab west under the surrounding masonry.
    addRouteCeiling(-36.90f, -18.4f, -34.90f, -12.0f);
    addRouteCeiling(-32.50f, -18.4f, -30.50f, -12.0f);
    addRouteCeiling(-34.90f, -18.4f, -32.50f, -16.6f);
    addRouteCeiling(-34.90f, -13.8f, -32.50f, -12.0f);
    constexpr float finaleShaftBase = routeCeiling - 0.02f;
    addWorldBox(-35.05f, finaleShaftBase, -16.75f, -34.90f, 1.72f, -13.65f, SurfaceMossyStone);
    addWorldBox(-32.50f, finaleShaftBase, -16.75f, -32.35f, 1.72f, -13.65f, SurfaceMossyStone);
    addWorldBox(-34.90f, finaleShaftBase, -16.75f, -32.50f, 1.72f, -16.60f, SurfaceMossyStone);
    addWorldBox(-34.90f, finaleShaftBase, -13.80f, -32.50f, 1.72f, -13.65f, SurfaceMossyStone);
    addRouteWallZ(-12.0f, -36.90f, -30.50f, SurfaceBack);
    addRouteWallZ(-18.4f, -36.90f, -30.50f, SurfaceForward);
    addRouteWallX(-30.50f, -18.4f, -16.8f, SurfaceLeft);
    addRouteWallX(-30.50f, -13.6f, -12.0f, SurfaceLeft);
    addRouteWallX(-36.90f, -18.4f, -12.0f, SurfaceRight);
    addWorldBox(-36.88f, -0.62f, -16.62f, -36.74f, 0.86f, -16.46f, SurfaceAgedMetal);
    addWorldBox(-36.88f, -0.62f, -13.94f, -36.74f, 0.86f, -13.78f, SurfaceAgedMetal);
    addWorldBox(-36.88f, -0.62f, -16.62f, -36.74f, -0.46f, -13.78f, SurfaceAgedMetal);
    addWorldBox(-36.88f, 0.70f, -16.62f, -36.74f, 0.86f, -13.78f, SurfaceAgedMetal);
    addWorldQuad({{-36.72f, -0.44f, -16.44f}}, {{-36.72f, 0.68f, -16.44f}},
                 {{-36.72f, 0.68f, -13.96f}}, {{-36.72f, -0.44f, -13.96f}},
                 SurfaceMirror, SurfaceRight);

    const std::uint32_t sceneIndexCount = static_cast<std::uint32_t>(indices.size());
    if (worldSurfaceCodes.size() != sceneIndexCount / 3u)
    {
        diagnostic = "World surface metadata does not match the world triangle count.";
        return false;
    }

    const std::uint32_t waterfallFirstVertex = static_cast<std::uint32_t>(vertices.size());
    const std::uint32_t waterfallIndexOffset = static_cast<std::uint32_t>(indices.size());
    vertices.insert(vertices.end(), waterfallVertices.begin(), waterfallVertices.end());
    indices.insert(indices.end(), waterfallIndices.begin(), waterfallIndices.end());
    const std::uint32_t waterfallIndexCount = static_cast<std::uint32_t>(indices.size());

    // Closed-position sliding roof slab. Its TLAS transform moves west after
    // the lich's death animation, physically exposing the sky to primary and
    // visibility rays rather than fading a ceiling texture away.
    addBox(-34.90f, 1.30f, -16.60f, -32.50f, 1.42f, -13.80f);
    const std::uint32_t finaleRoofIndexCount = static_cast<std::uint32_t>(indices.size());

    // The production torch body comes through the generic static GLB/PBR slot.
    // Retain only the temporary engine-owned faceted flame core for Task 4.
    horde::gameplay::items::HeldItemTransform itemFromEngineFlame =
        horde::gameplay::items::IdentityHeldItemTransform();
    const auto transformFlameVertex = [&itemFromEngineFlame](const Vertex& vertex) {
        const auto& p = vertex.position;
        return Vertex{{
            itemFromEngineFlame[0] * p[0] + itemFromEngineFlame[4] * p[1] +
                itemFromEngineFlame[8] * p[2] + itemFromEngineFlame[12],
            itemFromEngineFlame[1] * p[0] + itemFromEngineFlame[5] * p[1] +
                itemFromEngineFlame[9] * p[2] + itemFromEngineFlame[13],
            itemFromEngineFlame[2] * p[0] + itemFromEngineFlame[6] * p[1] +
                itemFromEngineFlame[10] * p[2] + itemFromEngineFlame[14]}};
    };
    const auto addFacetedFlame = [&addTriangle, &transformFlameVertex](
        float radius, float bottom, float waist, float top) {
        const Vertex lower = transformFlameVertex(Vertex{{0.0f, bottom, 0.0f}});
        const Vertex upper = transformFlameVertex(Vertex{{0.0f, top, 0.0f}});
        const std::array<Vertex, 4u> ring{{
            transformFlameVertex(Vertex{{radius, waist, 0.0f}}),
            transformFlameVertex(Vertex{{0.0f, waist, radius}}),
            transformFlameVertex(Vertex{{-radius, waist, 0.0f}}),
            transformFlameVertex(Vertex{{0.0f, waist, -radius}})}};
        for (std::size_t i = 0u; i < ring.size(); ++i)
        {
            const Vertex& current = ring[i];
            const Vertex& next = ring[(i + 1u) % ring.size()];
            addTriangle(lower, next, current);
            addTriangle(upper, current, next);
        }
    };
    if (productionHeldItemAssetsEnabled_)
    {
        const auto* flameSocket = horde::gameplay::items::FindHeldItemSocket(
            productionTorchAsset_.sockets, "Flame");
        if (flameSocket == nullptr)
        {
            diagnostic = "Production torch is missing the exact Flame socket.";
            return false;
        }
        itemFromEngineFlame = flameSocket->world;
    }
    if (productionHeldItemAssetsEnabled_)
    {
        addFacetedFlame(0.095f, -0.15f, 0.0f, 0.27f); // orange outer: 8 triangles
        addFacetedFlame(0.050f, -0.12f, -0.01f, 0.16f); // bright inner: 8 triangles
    }
    else
    {
        addFacetedFlame(0.095f, 0.16f, 0.31f, 0.58f);
        addFacetedFlame(0.050f, 0.19f, 0.30f, 0.47f);
    }
    const std::uint32_t torchIndexCount = static_cast<std::uint32_t>(indices.size());

    // The procedural sword proof is retired; instance 3 always uses the
    // production static GLB/PBR registration.
    const std::uint32_t swordIndexCount = static_cast<std::uint32_t>(indices.size());

    // A layered low-poly travelling coat replaces the original two-box torso
    // without adding a new BLAS or TLAS instance. The two main sections taper
    // away from the camera instead of presenting a broad chest slab, while
    // primitive ranges stay stable for raygen material assignment.
    const auto addTaperedCoatSection = [&addQuad](float topHalfWidth,
                                                  float bottomHalfWidth,
                                                  float topY,
                                                  float bottomY,
                                                  float backZ,
                                                  float frontZ) {
        const Vertex backTopLeft{{-topHalfWidth, topY, backZ}};
        const Vertex backTopRight{{topHalfWidth, topY, backZ}};
        const Vertex backBottomLeft{{-bottomHalfWidth, bottomY, backZ}};
        const Vertex backBottomRight{{bottomHalfWidth, bottomY, backZ}};
        const Vertex frontTopLeft{{-topHalfWidth, topY, frontZ}};
        const Vertex frontTopRight{{topHalfWidth, topY, frontZ}};
        const Vertex frontBottomLeft{{-bottomHalfWidth, bottomY, frontZ}};
        const Vertex frontBottomRight{{bottomHalfWidth, bottomY, frontZ}};
        addQuad(backBottomLeft, backBottomRight, backTopRight, backTopLeft);
        addQuad(frontBottomRight, frontBottomLeft, frontTopLeft, frontTopRight);
        addQuad(backBottomLeft, backTopLeft, frontTopLeft, frontBottomLeft);
        addQuad(frontBottomRight, frontTopRight, backTopRight, backBottomRight);
        addQuad(backTopLeft, backTopRight, frontTopRight, frontTopLeft);
        addQuad(frontBottomLeft, frontBottomRight, backBottomRight, backBottomLeft);
    };
    addTaperedCoatSection(0.245f, 0.16f, -0.49f, -0.82f, 0.39f, 0.56f); // 0-11 lower coat
    addTaperedCoatSection(0.22f, 0.28f, -0.22f, -0.56f, 0.36f, 0.56f);   // 12-23 chest
    addBox(-0.34f, -0.50f, 0.37f, -0.20f, -0.30f, 0.58f); // 24-35 left shoulder
    addBox(0.20f, -0.50f, 0.37f, 0.34f, -0.30f, 0.58f);   // 36-47 right shoulder
    addBox(-0.245f, -0.60f, 0.33f, 0.245f, -0.51f, 0.57f); // 48-59 belt
    addBox(-0.055f, -0.61f, 0.565f, 0.055f, -0.49f, 0.605f); // 60-71 buckle
    addBox(-0.17f, -0.31f, 0.36f, -0.035f, -0.17f, 0.55f); // 72-83 left collar
    addBox(0.035f, -0.31f, 0.36f, 0.17f, -0.17f, 0.55f);   // 84-95 right collar
    addBox(-0.13f, -0.98f, 0.50f, -0.035f, -0.70f, 0.59f); // 96-107 left tail
    addBox(0.035f, -0.98f, 0.50f, 0.13f, -0.70f, 0.59f);   // 108-119 right tail
    addQuad({{-0.20f, -0.19f, 0.585f}}, {{-0.13f, -0.19f, 0.585f}},
            {{0.20f, -0.48f, 0.585f}}, {{0.13f, -0.48f, 0.585f}}); // 120-121 strap
    const std::uint32_t playerBodyIndexCount = static_cast<std::uint32_t>(indices.size());

    // A six-sided bevel-ended capsule along +Z gives every articulated limb a
    // recognisable silhouette while retaining one shared phone-cheap limb BLAS.
    constexpr std::size_t limbSides = 6u;
    constexpr std::array<float, 4u> limbRingZ{0.0f, 0.13f, 0.87f, 1.0f};
    constexpr std::array<float, 4u> limbRingRadius{0.72f, 1.0f, 1.0f, 0.72f};
    constexpr float fullTurn = 6.28318530717958647692f;
    std::array<std::array<Vertex, limbSides>, limbRingZ.size()> limbRings{};
    for (std::size_t ring = 0u; ring < limbRingZ.size(); ++ring)
    {
        for (std::size_t side = 0u; side < limbSides; ++side)
        {
            const float angle = fullTurn * static_cast<float>(side) / static_cast<float>(limbSides);
            limbRings[ring][side] = Vertex{{std::cos(angle) * limbRingRadius[ring],
                                                   std::sin(angle) * limbRingRadius[ring],
                                                   limbRingZ[ring]}};
        }
    }
    for (std::size_t ring = 0u; ring + 1u < limbRingZ.size(); ++ring)
    {
        for (std::size_t side = 0u; side < limbSides; ++side)
        {
            const std::size_t next = (side + 1u) % limbSides;
            addQuad(limbRings[ring][side], limbRings[ring][next],
                    limbRings[ring + 1u][next], limbRings[ring + 1u][side]);
        }
    }
    const Vertex limbBase{{0.0f, 0.0f, 0.0f}};
    const Vertex limbTip{{0.0f, 0.0f, 1.0f}};
    for (std::size_t side = 0u; side < limbSides; ++side)
    {
        const std::size_t next = (side + 1u) % limbSides;
        addTriangle(limbBase, limbRings[0u][next], limbRings[0u][side]);
        addTriangle(limbTip, limbRings[3u][side], limbRings[3u][next]);
    }
    const std::uint32_t waterfallPrimitiveCount =
        (waterfallIndexCount - waterfallIndexOffset) / 3u;
    const std::uint32_t finaleRoofPrimitiveCount =
        (finaleRoofIndexCount - waterfallIndexCount) / 3u;
    const std::uint32_t torchPrimitiveCount = (torchIndexCount - finaleRoofIndexCount) / 3u;
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
        !CreateBuffer(sizeof(RtHeldLightGpu), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      uploadMemory, false, heldLightBuffer_, diagnostic) ||
        !CreateBuffer(worldSurfaceBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, uploadMemory, false, worldSurfaceBuffer_, diagnostic))
    {
        return false;
    }

    const RtHeldLightGpu initialHeldLight{};
    if (!WriteBuffer(vertexBuffer_, vertices.data(), vertexBufferSize, "world vertex", diagnostic) ||
        !WriteBuffer(indexBuffer_, indices.data(), indexBufferSize, "world index", diagnostic) ||
        !WriteBuffer(transformBuffer_, &transform, sizeof(transform), "world transform", diagnostic) ||
        !WriteBuffer(heldLightBuffer_, &initialHeldLight, sizeof(initialHeldLight),
                     "held light", diagnostic) ||
        !WriteBuffer(worldSurfaceBuffer_, worldSurfaceCodes.data(), worldSurfaceBufferSize,
                     "world surface metadata", diagnostic))
    {
        return false;
    }

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

    VkAccelerationStructureBuildRangeInfoKHR waterfallRange{};
    waterfallRange.primitiveCount = waterfallPrimitiveCount;
    waterfallRange.primitiveOffset = waterfallIndexOffset * sizeof(std::uint32_t);
    waterfallRange.firstVertex = waterfallFirstVertex;
    VkAccelerationStructureBuildGeometryInfoKHR waterfallBuildInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    waterfallBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    waterfallBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    waterfallBuildInfo.geometryCount = 1u;
    waterfallBuildInfo.pGeometries = &blasGeometry;
    VkAccelerationStructureBuildSizesInfoKHR waterfallSizes{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                             &waterfallBuildInfo, &waterfallPrimitiveCount,
                                             &waterfallSizes);
    if (!CreateBuffer(waterfallSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      waterfallBlas_.backing,
                      diagnostic))
    {
        return false;
    }
    VkAccelerationStructureCreateInfoKHR waterfallCreateInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    waterfallCreateInfo.buffer = waterfallBlas_.backing.buffer;
    waterfallCreateInfo.size = waterfallSizes.accelerationStructureSize;
    waterfallCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &waterfallCreateInfo, nullptr,
                                          &waterfallBlas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create dedicated waterfall BLAS.";
        return false;
    }
    Buffer waterfallScratch;
    if (!CreateBuffer(waterfallSizes.buildScratchSize,
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      waterfallScratch,
                      diagnostic))
    {
        return false;
    }
    waterfallBuildInfo.dstAccelerationStructure = waterfallBlas_.handle;
    waterfallBuildInfo.scratchData.deviceAddress = waterfallScratch.address;
    const VkAccelerationStructureBuildRangeInfoKHR* waterfallRanges[] = {&waterfallRange};
    BlasBuildData waterfallBuildData{this, &waterfallBuildInfo, waterfallRanges};
    if (!RunOneTimeCommands(buildBlas, &waterfallBuildData, diagnostic))
    {
        DestroyBuffer(waterfallScratch);
        return false;
    }
    DestroyBuffer(waterfallScratch);
    VkAccelerationStructureDeviceAddressInfoKHR waterfallAddressInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    waterfallAddressInfo.accelerationStructure = waterfallBlas_.handle;
    waterfallBlas_.address =
        vkGetAccelerationStructureDeviceAddressKHR_(device_, &waterfallAddressInfo);

    VkAccelerationStructureBuildRangeInfoKHR finaleRoofRange{};
    finaleRoofRange.primitiveCount = finaleRoofPrimitiveCount;
    finaleRoofRange.primitiveOffset = waterfallIndexCount * sizeof(std::uint32_t);
    finaleRoofRange.firstVertex = 0u;
    VkAccelerationStructureBuildGeometryInfoKHR finaleRoofBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    finaleRoofBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    finaleRoofBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    finaleRoofBuildInfo.geometryCount = 1u;
    finaleRoofBuildInfo.pGeometries = &blasGeometry;
    VkAccelerationStructureBuildSizesInfoKHR finaleRoofSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                             &finaleRoofBuildInfo, &finaleRoofPrimitiveCount, &finaleRoofSizes);
    if (!CreateBuffer(finaleRoofSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      finaleRoofBlas_.backing,
                      diagnostic))
    {
        return false;
    }
    VkAccelerationStructureCreateInfoKHR finaleRoofCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    finaleRoofCreateInfo.buffer = finaleRoofBlas_.backing.buffer;
    finaleRoofCreateInfo.size = finaleRoofSizes.accelerationStructureSize;
    finaleRoofCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &finaleRoofCreateInfo, nullptr, &finaleRoofBlas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create finale sliding-roof BLAS.";
        return false;
    }
    Buffer finaleRoofScratch;
    if (!CreateBuffer(finaleRoofSizes.buildScratchSize,
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      finaleRoofScratch,
                      diagnostic))
    {
        return false;
    }
    finaleRoofBuildInfo.dstAccelerationStructure = finaleRoofBlas_.handle;
    finaleRoofBuildInfo.scratchData.deviceAddress = finaleRoofScratch.address;
    const VkAccelerationStructureBuildRangeInfoKHR* finaleRoofRanges[] = {&finaleRoofRange};
    BlasBuildData finaleRoofBuildData{this, &finaleRoofBuildInfo, finaleRoofRanges};
    if (!RunOneTimeCommands(buildBlas, &finaleRoofBuildData, diagnostic))
    {
        DestroyBuffer(finaleRoofScratch);
        return false;
    }
    DestroyBuffer(finaleRoofScratch);
    VkAccelerationStructureDeviceAddressInfoKHR finaleRoofAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    finaleRoofAddressInfo.accelerationStructure = finaleRoofBlas_.handle;
    finaleRoofBlas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &finaleRoofAddressInfo);

    const auto appendStaticGeometries = [this, &diagnostic](
        const std::uint32_t instanceCustomIndex,
        std::vector<VkAccelerationStructureGeometryKHR>& geometries,
        std::vector<VkAccelerationStructureBuildRangeInfoKHR>& ranges,
        std::vector<std::uint32_t>& primitiveCounts) {
        const RtInstanceMetadata instance =
            staticMeshSlot_.InstanceMetadata()[instanceCustomIndex];
        const auto& primitiveMetadata = staticMeshSlot_.PrimitiveMetadata();
        const auto& staticVertices = staticMeshSlot_.Vertices();
        for (std::uint32_t localIndex = 0u; localIndex < instance.primitiveCount; ++localIndex)
        {
            const std::uint32_t geometryIndex = instance.primitiveBase + localIndex;
            const RtPrimitiveMetadata& primitive = primitiveMetadata[geometryIndex];
            const std::uint32_t nextVertexOffset = geometryIndex + 1u < primitiveMetadata.size()
                ? primitiveMetadata[geometryIndex + 1u].vertexOffset
                : static_cast<std::uint32_t>(staticVertices.size());
            if (nextVertexOffset <= primitive.vertexOffset || primitive.indexCount == 0u)
            {
                diagnostic = "Static RT BLAS primitive has an empty geometry range.";
                return false;
            }
            VkAccelerationStructureGeometryKHR geometry{
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
            geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
            geometry.geometry.triangles.sType =
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            geometry.geometry.triangles.vertexData.deviceAddress =
                staticVertexBuffer_.address +
                static_cast<VkDeviceSize>(primitive.vertexOffset) *
                    sizeof(horde::scene::assets::StaticRtVertex);
            geometry.geometry.triangles.vertexStride =
                sizeof(horde::scene::assets::StaticRtVertex);
            geometry.geometry.triangles.maxVertex =
                nextVertexOffset - primitive.vertexOffset - 1u;
            geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
            geometry.geometry.triangles.indexData.deviceAddress =
                staticIndexBuffer_.address +
                static_cast<VkDeviceSize>(primitive.indexOffset) * sizeof(std::uint32_t);
            geometry.geometry.triangles.transformData.deviceAddress =
                staticGeometryTransformBuffer_.address +
                static_cast<VkDeviceSize>(geometryIndex) * sizeof(VkTransformMatrixKHR);
            geometries.push_back(geometry);
            VkAccelerationStructureBuildRangeInfoKHR range{};
            range.primitiveCount = primitive.indexCount / 3u;
            ranges.push_back(range);
            primitiveCounts.push_back(range.primitiveCount);
        }
        return true;
    };

    std::vector<VkAccelerationStructureGeometryKHR> torchGeometries;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> torchRanges;
    std::vector<std::uint32_t> torchPrimitiveCounts;
    if (productionHeldItemAssetsEnabled_)
    {
        if (!appendStaticGeometries(1u, torchGeometries, torchRanges, torchPrimitiveCounts))
            return false;
        // Task 4 owns final fire. Retain only the existing 16-triangle engine
        // flame core as a separate geometry after the authored PBR body.
        constexpr std::uint32_t engineFlamePrimitiveCount = 16u;
        torchGeometries.push_back(blasGeometry);
        VkAccelerationStructureBuildRangeInfoKHR flameRange{};
        flameRange.primitiveCount = engineFlamePrimitiveCount;
        flameRange.primitiveOffset =
            (torchIndexCount - engineFlamePrimitiveCount * 3u) * sizeof(std::uint32_t);
        torchRanges.push_back(flameRange);
        torchPrimitiveCounts.push_back(engineFlamePrimitiveCount);
    }
    else
    {
        torchGeometries.push_back(blasGeometry);
        VkAccelerationStructureBuildRangeInfoKHR torchRange{};
        torchRange.primitiveCount = torchPrimitiveCount;
        torchRange.primitiveOffset = finaleRoofIndexCount * sizeof(std::uint32_t);
        torchRanges.push_back(torchRange);
        torchPrimitiveCounts.push_back(torchPrimitiveCount);
    }

    VkAccelerationStructureBuildGeometryInfoKHR torchBlasBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    torchBlasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    torchBlasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    torchBlasBuildInfo.geometryCount = static_cast<std::uint32_t>(torchGeometries.size());
    torchBlasBuildInfo.pGeometries = torchGeometries.data();
    VkAccelerationStructureBuildSizesInfoKHR torchBlasSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                             &torchBlasBuildInfo, torchPrimitiveCounts.data(), &torchBlasSizes);
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
    std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> torchBlasRangePointers;
    torchBlasRangePointers.reserve(torchRanges.size());
    for (const auto& range : torchRanges) torchBlasRangePointers.push_back(&range);
    BlasBuildData torchBlasBuildData{
        this, &torchBlasBuildInfo, torchBlasRangePointers.data()};
    const auto torchBlasBuildStart = std::chrono::steady_clock::now();
    if (!RunOneTimeCommands(buildBlas, &torchBlasBuildData, diagnostic))
    {
        DestroyBuffer(torchBlasScratch);
        return false;
    }
    if (productionHeldItemAssetsEnabled_)
    {
        heldItemBlasMeasurements_.RecordTorch(
            torchBlasSizes.accelerationStructureSize,
            std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - torchBlasBuildStart).count());
    }
    DestroyBuffer(torchBlasScratch);

    VkAccelerationStructureDeviceAddressInfoKHR torchBlasAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    torchBlasAddressInfo.accelerationStructure = torchBlas_.handle;
    torchBlas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &torchBlasAddressInfo);

    std::vector<VkAccelerationStructureGeometryKHR> swordGeometries;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> swordRanges;
    std::vector<std::uint32_t> swordPrimitiveCounts;
    if (genericStaticAssetEnabled_)
    {
        if (!appendStaticGeometries(
                3u, swordGeometries, swordRanges, swordPrimitiveCounts))
            return false;
    }
    else
    {
        swordGeometries.push_back(blasGeometry);
        VkAccelerationStructureBuildRangeInfoKHR range{};
        range.primitiveCount = swordPrimitiveCount;
        range.primitiveOffset = torchIndexCount * sizeof(std::uint32_t);
        swordRanges.push_back(range);
        swordPrimitiveCounts.push_back(swordPrimitiveCount);
    }
    VkAccelerationStructureBuildGeometryInfoKHR swordBlasBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    swordBlasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    swordBlasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    swordBlasBuildInfo.geometryCount = static_cast<std::uint32_t>(swordGeometries.size());
    swordBlasBuildInfo.pGeometries = swordGeometries.data();
    VkAccelerationStructureBuildSizesInfoKHR swordBlasSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                             &swordBlasBuildInfo, swordPrimitiveCounts.data(),
                                             &swordBlasSizes);
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
    std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> swordBlasRangePointers;
    swordBlasRangePointers.reserve(swordRanges.size());
    for (const auto& range : swordRanges) swordBlasRangePointers.push_back(&range);
    BlasBuildData swordBlasBuildData{this, &swordBlasBuildInfo, swordBlasRangePointers.data()};
    const auto staticBlasBuildStart = std::chrono::steady_clock::now();
    if (!RunOneTimeCommands(buildBlas, &swordBlasBuildData, diagnostic))
    {
        DestroyBuffer(swordBlasScratch);
        return false;
    }
    if (genericStaticAssetEnabled_)
    {
        heldItemBlasMeasurements_.RecordSword(
            swordBlasSizes.accelerationStructureSize,
            std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - staticBlasBuildStart).count());
        staticMeshBlasBytes_ = heldItemBlasMeasurements_.TotalBytes();
        staticMeshBlasBuildMilliseconds_ =
            heldItemBlasMeasurements_.TotalBuildMilliseconds();
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

    if (!characterSlot_.PrepareInitialGeometry(diagnostic))
    {
        return false;
    }
    auto& lichGpu = characterSlot_.LichGpu();
    auto& lichVertexBuffer_ = lichGpu.vertices;
    auto& lichBlas_ = lichGpu.accelerationStructure;
    auto& lichBlasUpdateScratch_ = lichGpu.updateScratch;
    const auto& lichSkinnedVertices_ = characterSlot_.LichVertices();
    lichGpu.vertexStride = sizeof(horde::scene::TexturedSkinnedRtVertex);
    lichGpu.vertexCount = static_cast<std::uint32_t>(lichSkinnedVertices_.size());
    for (std::size_t bucket = 0u; bucket < CharacterRenderSlot::kMaximumSkeletonPoseBuckets; ++bucket)
    {
        auto& skeletonGpu = characterSlot_.SkeletonGpu(bucket);
        const auto& skeletonVertices = characterSlot_.SkeletonVertices(bucket);
        skeletonGpu.vertexStride = sizeof(horde::scene::SkinnedRtVertex);
        skeletonGpu.vertexCount = static_cast<std::uint32_t>(skeletonVertices.size());
        const VkDeviceSize skeletonVertexBufferSize =
            sizeof(horde::scene::SkinnedRtVertex) * skeletonVertices.size();
        if (!CreateBuffer(skeletonVertexBufferSize,
                          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          uploadMemory,
                          true,
                          skeletonGpu.vertices,
                          diagnostic) ||
            !WriteBuffer(skeletonGpu.vertices,
                         skeletonVertices.data(),
                         skeletonVertexBufferSize,
                         bucket == 0u ? "skeleton pose 0 vertex" : "skeleton pose 1 vertex",
                         diagnostic))
        {
            return false;
        }

        VkAccelerationStructureGeometryKHR skeletonGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        skeletonGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        skeletonGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        skeletonGeometry.geometry.triangles.sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        skeletonGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        skeletonGeometry.geometry.triangles.vertexData.deviceAddress = skeletonGpu.vertices.address;
        skeletonGeometry.geometry.triangles.vertexStride = sizeof(horde::scene::SkinnedRtVertex);
        skeletonGeometry.geometry.triangles.maxVertex =
            static_cast<std::uint32_t>(skeletonVertices.size() - 1u);
        skeletonGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
        const std::uint32_t skeletonPrimitiveCount =
            static_cast<std::uint32_t>(skeletonVertices.size() / 3u);
        VkAccelerationStructureBuildGeometryInfoKHR skeletonBuildInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        skeletonBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        skeletonBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                                  VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        skeletonBuildInfo.geometryCount = 1u;
        skeletonBuildInfo.pGeometries = &skeletonGeometry;
        VkAccelerationStructureBuildSizesInfoKHR skeletonSizes{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        vkGetAccelerationStructureBuildSizesKHR_(device_,
                                                 VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                                 &skeletonBuildInfo,
                                                 &skeletonPrimitiveCount,
                                                 &skeletonSizes);
        if (!CreateBuffer(skeletonSizes.accelerationStructureSize,
                          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          true,
                          skeletonGpu.accelerationStructure.backing,
                          diagnostic))
        {
            return false;
        }
        VkAccelerationStructureCreateInfoKHR skeletonCreateInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        skeletonCreateInfo.buffer = skeletonGpu.accelerationStructure.backing.buffer;
        skeletonCreateInfo.size = skeletonSizes.accelerationStructureSize;
        skeletonCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        if (vkCreateAccelerationStructureKHR_(device_,
                                              &skeletonCreateInfo,
                                              nullptr,
                                              &skeletonGpu.accelerationStructure.handle) != VK_SUCCESS)
        {
            diagnostic = "Failed to create animated skeleton pose BLAS.";
            return false;
        }
        if (!CreateBuffer(std::max(skeletonSizes.buildScratchSize, skeletonSizes.updateScratchSize),
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          true,
                          skeletonGpu.updateScratch,
                          diagnostic))
        {
            return false;
        }
        VkAccelerationStructureBuildRangeInfoKHR skeletonRange{};
        skeletonRange.primitiveCount = skeletonPrimitiveCount;
        skeletonBuildInfo.dstAccelerationStructure = skeletonGpu.accelerationStructure.handle;
        skeletonBuildInfo.scratchData.deviceAddress = skeletonGpu.updateScratch.address;
        const VkAccelerationStructureBuildRangeInfoKHR* skeletonRanges[] = {&skeletonRange};
        BlasBuildData skeletonBuildData{this, &skeletonBuildInfo, skeletonRanges};
        if (!RunOneTimeCommands(buildBlas, &skeletonBuildData, diagnostic)) return false;
        VkAccelerationStructureDeviceAddressInfoKHR skeletonAddressInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
        skeletonAddressInfo.accelerationStructure = skeletonGpu.accelerationStructure.handle;
        skeletonGpu.accelerationStructure.address =
            vkGetAccelerationStructureDeviceAddressKHR_(device_, &skeletonAddressInfo);
    }

    const VkDeviceSize lichVertexBufferSize = sizeof(horde::scene::TexturedSkinnedRtVertex) * lichSkinnedVertices_.size();
    if (!CreateBuffer(lichVertexBufferSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      uploadMemory,
                      true,
                      lichVertexBuffer_,
                      diagnostic))
    {
        return false;
    }
    if (!WriteBuffer(lichVertexBuffer_, lichSkinnedVertices_.data(), lichVertexBufferSize,
                     "lich vertex", diagnostic))
    {
        return false;
    }

    VkAccelerationStructureGeometryKHR lichGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    lichGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    // Meshy's blended flag is unnecessary for this opaque placeholder and
    // would make every ray pay candidate-confirmation costs.
    lichGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    lichGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    lichGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    lichGeometry.geometry.triangles.vertexData.deviceAddress = lichVertexBuffer_.address;
    lichGeometry.geometry.triangles.vertexStride = sizeof(horde::scene::TexturedSkinnedRtVertex);
    lichGeometry.geometry.triangles.maxVertex = static_cast<std::uint32_t>(lichSkinnedVertices_.size() - 1u);
    lichGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
    const std::uint32_t lichPrimitiveCount = static_cast<std::uint32_t>(lichSkinnedVertices_.size() / 3u);
    VkAccelerationStructureBuildGeometryInfoKHR lichBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    lichBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    lichBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    lichBuildInfo.geometryCount = 1u;
    lichBuildInfo.pGeometries = &lichGeometry;
    VkAccelerationStructureBuildSizesInfoKHR lichSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                             &lichBuildInfo, &lichPrimitiveCount, &lichSizes);
    if (!CreateBuffer(lichSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      lichBlas_.backing,
                      diagnostic))
    {
        return false;
    }
    VkAccelerationStructureCreateInfoKHR lichCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    lichCreateInfo.buffer = lichBlas_.backing.buffer;
    lichCreateInfo.size = lichSizes.accelerationStructureSize;
    lichCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &lichCreateInfo, nullptr, &lichBlas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create animated lich BLAS.";
        return false;
    }
    if (!CreateBuffer(std::max(lichSizes.buildScratchSize, lichSizes.updateScratchSize),
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      lichBlasUpdateScratch_,
                      diagnostic))
    {
        return false;
    }
    VkAccelerationStructureBuildRangeInfoKHR lichRange{};
    lichRange.primitiveCount = lichPrimitiveCount;
    lichBuildInfo.dstAccelerationStructure = lichBlas_.handle;
    lichBuildInfo.scratchData.deviceAddress = lichBlasUpdateScratch_.address;
    const VkAccelerationStructureBuildRangeInfoKHR* lichRanges[] = {&lichRange};
    BlasBuildData lichBuildData{this, &lichBuildInfo, lichRanges};
    if (!RunOneTimeCommands(buildBlas, &lichBuildData, diagnostic)) return false;
    VkAccelerationStructureDeviceAddressInfoKHR lichAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    lichAddressInfo.accelerationStructure = lichBlas_.handle;
    lichBlas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &lichAddressInfo);

    std::array<VkAccelerationStructureInstanceKHR, PresentableTinyRtScene::kTlasInstanceCount> instances{};
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
    instances[2].accelerationStructureReference = characterSlot_.SkeletonGpu(0u).accelerationStructure.address;
    instances[2].transform = {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, -0.95f,
        0.0f, 0.0f, 1.0f, -4.65f}};
    instances[3] = instances[1];
    instances[3].instanceCustomIndex = 3u;
    instances[3].mask = 0x02u;
    instances[3].accelerationStructureReference = swordBlas_.address;
    instances[4] = instances[0];
    instances[4].instanceCustomIndex = 4u;
    // The complete coat remains visible in mirror/reflection rays. Keeping its
    // chest out of first-person primary rays avoids a near-camera slab while
    // articulated arms, pelvis, legs and boots stay visible on mask 0x04.
    instances[4].mask = 0x10u;
    instances[4].accelerationStructureReference = playerBodyBlas_.address;
    instances[4].transform = {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.58f,
        0.0f, 0.0f, 1.0f, 0.0f}};
    for (std::size_t i = 5u; i <= 16u; ++i)
    {
        instances[i] = instances[4];
        instances[i].instanceCustomIndex = static_cast<std::uint32_t>(i);
        instances[i].mask = 0x04u;
        instances[i].accelerationStructureReference = playerLimbBlas_.address;
        instances[i].transform = {{
            0.07f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.07f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.40f, 0.0f}};
    }
    instances[9].accelerationStructureReference = playerLimbBlas_.address;
    instances[16].mask = 0x10u;
    instances[17] = instances[0];
    instances[17].instanceCustomIndex = 17u;
    instances[17].mask = 0x20u;
    instances[17].accelerationStructureReference = finaleRoofBlas_.address;
    instances[18] = instances[2];
    instances[18].instanceCustomIndex = CharacterRenderSlot::kSecondSkeletonTlasInstanceIndex;
    instances[18].mask = 0u;
    instances[19] = instances[0];
    instances[19].instanceCustomIndex = 19u;
    instances[19].mask = 0x01u;
    instances[19].accelerationStructureReference = waterfallBlas_.address;
    instances[19].transform = {{
        1.0f, 0.0f, 0.0f, -2.32f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, -15.26f}};
    if (!CreateBuffer(sizeof(instances), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, uploadMemory, true, instanceBuffer_, diagnostic))
    {
        return false;
    }
    if (!WriteBuffer(instanceBuffer_, instances.data(), sizeof(instances), "TLAS instance", diagnostic))
    {
        return false;
    }

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
    const auto& skeletonVertexBuffer_ = characterSlot_.SkeletonGpu(0u).vertices;
    const auto& secondSkeletonVertexBuffer = characterSlot_.SkeletonGpu(1u).vertices;
    const auto& lichVertexBuffer_ = characterSlot_.LichGpu().vertices;
    const std::array<VkDescriptorSetLayoutBinding, 21u> bindings{{
        {0u, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, nullptr},
        {1u, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {2u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {3u, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {4u, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {5u, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {6u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {7u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {8u, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {9u, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {10u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {kRtBindingInstanceMetadata, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {kRtBindingPrimitiveMetadata, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {kRtBindingMaterials, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {kRtBindingStaticVertices, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {kRtBindingStaticIndices, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {kRtBindingBaseColorTextures, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {kRtBindingNormalTextures, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {kRtBindingOrmTextures, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {kRtBindingEmissiveTextures, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {kRtBindingHeldLight, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
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
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10u},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 9u},
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

    VkDescriptorBufferInfo secondSkeletonBufferInfo{};
    secondSkeletonBufferInfo.buffer = secondSkeletonVertexBuffer.buffer;
    secondSkeletonBufferInfo.offset = 0u;
    secondSkeletonBufferInfo.range = secondSkeletonVertexBuffer.size;
    VkWriteDescriptorSet secondSkeletonWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    secondSkeletonWrite.dstSet = descriptorSet_;
    secondSkeletonWrite.dstBinding = 10u;
    secondSkeletonWrite.descriptorCount = 1u;
    secondSkeletonWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    secondSkeletonWrite.pBufferInfo = &secondSkeletonBufferInfo;

    const auto bufferWrite = [this](std::uint32_t binding,
                                    const VkDescriptorBufferInfo* info) {
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = descriptorSet_;
        write.dstBinding = binding;
        write.descriptorCount = 1u;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = info;
        return write;
    };
    const VkDescriptorBufferInfo instanceMetadataInfo{
        instanceMetadataBuffer_.buffer, 0u, instanceMetadataBuffer_.size};
    const VkDescriptorBufferInfo primitiveMetadataInfo{
        primitiveMetadataBuffer_.buffer, 0u, primitiveMetadataBuffer_.size};
    const VkDescriptorBufferInfo materialMetadataInfo{
        materialMetadataBuffer_.buffer, 0u, materialMetadataBuffer_.size};
    const VkDescriptorBufferInfo staticVertexInfo{
        staticVertexBuffer_.buffer, 0u, staticVertexBuffer_.size};
    const VkDescriptorBufferInfo staticIndexInfo{
        staticIndexBuffer_.buffer, 0u, staticIndexBuffer_.size};
    const VkDescriptorBufferInfo heldLightInfo{
        heldLightBuffer_.buffer, 0u, heldLightBuffer_.size};

    VkDescriptorBufferInfo lichBufferInfo{};
    lichBufferInfo.buffer = lichVertexBuffer_.buffer;
    lichBufferInfo.offset = 0u;
    lichBufferInfo.range = lichVertexBuffer_.size;
    VkWriteDescriptorSet lichBufferWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    lichBufferWrite.dstSet = descriptorSet_;
    lichBufferWrite.dstBinding = 7u;
    lichBufferWrite.descriptorCount = 1u;
    lichBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    lichBufferWrite.pBufferInfo = &lichBufferInfo;

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
    const VkDescriptorImageInfo lichBaseInfo{materialSampler_, lichBaseColor_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo lichEmissiveInfo{materialSampler_, lichEmissive_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo staticBaseInfo{
        materialSampler_, genericStaticAssetEnabled_ ? staticBaseColor_.view : materialDiffuse_.view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo staticNormalInfo{
        materialSampler_, genericStaticAssetEnabled_ ? staticNormal_.view : materialNormal_.view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo staticOrmInfo{
        materialSampler_, genericStaticAssetEnabled_ ? staticOrm_.view : materialArm_.view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo staticEmissiveInfo{
        materialSampler_, genericStaticAssetEnabled_ ? staticEmissive_.view : lichEmissive_.view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const auto sampledWrite = [this](std::uint32_t binding, const VkDescriptorImageInfo* info) {
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = descriptorSet_;
        write.dstBinding = binding;
        write.descriptorCount = 1u;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = info;
        return write;
    };
    const std::array<VkWriteDescriptorSet, 21u> writes{{accelerationStructureWrite, imageWrite, skeletonWrite,
                                                       sampledWrite(3u, &diffuseInfo), sampledWrite(4u, &normalInfo), sampledWrite(5u, &armInfo),
                                                       worldSurfaceWrite, lichBufferWrite,
                                                       sampledWrite(8u, &lichBaseInfo), sampledWrite(9u, &lichEmissiveInfo),
                                                       secondSkeletonWrite,
                                                       bufferWrite(kRtBindingInstanceMetadata, &instanceMetadataInfo),
                                                       bufferWrite(kRtBindingPrimitiveMetadata, &primitiveMetadataInfo),
                                                       bufferWrite(kRtBindingMaterials, &materialMetadataInfo),
                                                       bufferWrite(kRtBindingStaticVertices, &staticVertexInfo),
                                                       bufferWrite(kRtBindingStaticIndices, &staticIndexInfo),
                                                       sampledWrite(kRtBindingBaseColorTextures, &staticBaseInfo),
                                                       sampledWrite(kRtBindingNormalTextures, &staticNormalInfo),
                                                       sampledWrite(kRtBindingOrmTextures, &staticOrmInfo),
                                                       sampledWrite(kRtBindingEmissiveTextures, &staticEmissiveInfo),
                                                       bufferWrite(kRtBindingHeldLight, &heldLightInfo)}};
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

    std::vector<std::uint8_t> sbtData(sbtSize, 0u);
    for (std::uint32_t group = 0u; group < groupCount; ++group)
    {
        std::memcpy(sbtData.data() + (regionSize * group), handles.data() + (handleSize * group), handleSize);
    }
    if (!WriteBuffer(shaderBindingTable_, sbtData.data(), sbtSize, "RT shader binding table", diagnostic))
    {
        return false;
    }

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
                                                     const RtSceneFrameInputs& frame,
                                                     std::string& diagnostic)
{
    const float cameraYaw = frame.cameraYaw;
    const float cameraPitch = frame.cameraPitch;
    const float walkTime = frame.walkTime;
    const float cameraX = frame.cameraX;
    const float cameraZ = frame.cameraZ;
    const float walkAmount = frame.walkAmount;
    const horde::gameplay::CombatSnapshot& combat = frame.combat;
    const horde::gameplay::PlayerCombatSnapshot& playerCombat = frame.playerCombat;
    const horde::gameplay::TorchFailureSnapshot& torchFailure = frame.torchFailure;
    const horde::gameplay::EnemyRosterSnapshot& roster = frame.roster;
    const horde::gameplay::LichSnapshot& lich = frame.lich;
    const WaterfallCurtainScale waterfallScale = ResolveWaterfallCurtainScale(frame.tuning);
    const auto& skeletonGpu = characterSlot_.SkeletonGpu(0u);
    const auto& secondSkeletonGpu = characterSlot_.SkeletonGpu(1u);
    const auto& lichGpu = characterSlot_.LichGpu();
    if (instanceBuffer_.memory == VK_NULL_HANDLE || heldLightBuffer_.memory == VK_NULL_HANDLE ||
        skeletonGpu.vertices.memory == VK_NULL_HANDLE ||
        secondSkeletonGpu.vertices.memory == VK_NULL_HANDLE ||
        skeletonGpu.accelerationStructure.handle == VK_NULL_HANDLE ||
        secondSkeletonGpu.accelerationStructure.handle == VK_NULL_HANDLE ||
        lichGpu.accelerationStructure.handle == VK_NULL_HANDLE ||
        finaleRoofBlas_.handle == VK_NULL_HANDLE || waterfallBlas_.handle == VK_NULL_HANDLE ||
        swordBlas_.handle == VK_NULL_HANDLE ||
        playerBodyBlas_.handle == VK_NULL_HANDLE || playerLimbBlas_.handle == VK_NULL_HANDLE ||
        tlas_.handle == VK_NULL_HANDLE || skeletonGpu.updateScratch.address == 0u ||
        secondSkeletonGpu.updateScratch.address == 0u ||
        lichGpu.updateScratch.address == 0u || tlasUpdateScratch_.address == 0u)
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
    const Vec3 worldUp{0.0f, 1.0f, 0.0f};
    const Vec3 eye{cameraX, 0.58f, cameraZ};
    const Vec3 bodyForward{std::sin(cameraYaw), 0.0f, -std::cos(cameraYaw)};
    const Vec3 bodyRight{std::cos(cameraYaw), 0.0f, std::sin(cameraYaw)};
    const horde::gameplay::LowerBodyPoseState lowerBodyPose =
        horde::gameplay::EvaluateLowerBodyPose(walkTime, walkAmount);
    const float torsoCos = std::cos(lowerBodyPose.torsoTwistRadians);
    const float torsoSin = std::sin(lowerBodyPose.torsoTwistRadians);
    const Vec3 animatedBodyForward = normalize(add(scaled(bodyForward, torsoCos), scaled(bodyRight, torsoSin)));
    const Vec3 animatedBodyRight = normalize(subtract(scaled(bodyRight, torsoCos), scaled(bodyForward, torsoSin)));
    const Vec3 animatedBodyOrigin = add(add(eye, scaled(bodyRight, lowerBodyPose.pelvisSway)),
                                        Vec3{0.0f, lowerBodyPose.pelvisBob * 0.65f, 0.0f});
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

    const horde::gameplay::items::HeldItemKinematicsState& heldKinematics =
        frame.heldItemKinematics;
    const Vec3 leftShoulderLocal = heldKinematics.leftShoulderLocal;
    const Vec3 leftHandLocal = heldKinematics.leftHandLocal;
    const Vec3 rightShoulderLocal = heldKinematics.rightShoulderLocal;
    const Vec3 rightHandLocal = heldKinematics.rightHandLocal;
    const Vec3 leftElbowLocal = solveElbow(leftShoulderLocal, leftHandLocal, 0.53f, 0.53f, Vec3{-1.0f, -0.15f, 0.08f});
    const Vec3 rightElbowLocal = solveElbow(rightShoulderLocal, rightHandLocal, 0.53f, 0.53f, Vec3{1.0f, -0.20f, 0.10f});
    const Vec3 leftShoulder = toWorld(leftShoulderLocal);
    const Vec3 leftElbow = toWorld(leftElbowLocal);
    const Vec3 leftHand = toWorld(leftHandLocal);
    const Vec3 rightShoulder = toWorld(rightShoulderLocal);
    const Vec3 rightElbow = toWorld(rightElbowLocal);
    const Vec3 rightHand = toWorld(rightHandLocal);

    if (!characterSlot_.PrepareFrame(frame.skeletonEnemies,
                                     frame.skeletonEnemyCount,
                                     roster,
                                     lich,
                                     gpuResources_,
                                     diagnostic))
    {
        return false;
    }
    const CharacterBlasRefit pendingCharacterRefit = characterSlot_.PendingRefit();
    const bool updateSkeletonPose0 =
        HasCharacterBlasRefit(pendingCharacterRefit, CharacterBlasRefit::SkeletonPose0);
    const bool updateSkeletonPose1 =
        HasCharacterBlasRefit(pendingCharacterRefit, CharacterBlasRefit::SkeletonPose1);
    const bool updateLich = HasCharacterBlasRefit(pendingCharacterRefit, CharacterBlasRefit::Lich);
    const auto& lichVertexBuffer_ = lichGpu.vertices;
    const auto& lichBlas_ = lichGpu.accelerationStructure;
    const auto& lichBlasUpdateScratch_ = lichGpu.updateScratch;
    const auto& lichSkinnedVertices_ = characterSlot_.LichVertices();

    const auto heldItemInstanceTransform = [](
        const horde::gameplay::items::HeldItemState& state) {
        const auto renderTransform = HeldItemRenderSlot::BuildInstanceTransform(state);
        return VkTransformMatrixKHR{{
            renderTransform[0], renderTransform[1], renderTransform[2], renderTransform[3],
            renderTransform[4], renderTransform[5], renderTransform[6], renderTransform[7],
            renderTransform[8], renderTransform[9], renderTransform[10], renderTransform[11]}};
    };

    std::array<VkAccelerationStructureInstanceKHR, PresentableTinyRtScene::kTlasInstanceCount> instances{};
    instances[0].transform = {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f}};
    instances[0].instanceCustomIndex = 0u;
    instances[0].mask = 0x01u;
    instances[0].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instances[0].accelerationStructureReference = blas_.address;
    instances[1] = instances[0];
    instances[1].transform = heldItemInstanceTransform(frame.heldItems[0]);
    instances[1].instanceCustomIndex = 1u;
    instances[1].mask = 0x02u;
    instances[1].accelerationStructureReference = torchBlas_.address;
    const auto characterInstances = characterSlot_.BuildActiveInstances();
    instances[CharacterRenderSlot::kTlasInstanceIndex] = characterInstances[0];
    instances[3] = instances[1];
    instances[3].instanceCustomIndex = 3u;
    instances[3].mask = 0x02u;
    instances[3].accelerationStructureReference = swordBlas_.address;
    instances[3].transform = heldItemInstanceTransform(frame.heldItems[1]);
    instances[4] = instances[0];
    instances[4].instanceCustomIndex = 4u;
    // The complete coat remains visible in mirror/reflection rays. Keeping its
    // chest out of first-person primary rays avoids a near-camera slab while
    // articulated arms, pelvis, legs and boots stay visible on mask 0x04.
    instances[4].mask = 0x10u;
    instances[4].accelerationStructureReference = playerBodyBlas_.address;
    instances[4].transform = {{
        animatedBodyRight[0], 0.0f, animatedBodyForward[0], animatedBodyOrigin[0],
        animatedBodyRight[1], 1.0f, animatedBodyForward[1], animatedBodyOrigin[1],
        animatedBodyRight[2], 0.0f, animatedBodyForward[2], animatedBodyOrigin[2]}};
    for (std::size_t i = 5u; i <= 16u; ++i)
    {
        instances[i] = instances[4];
        instances[i].instanceCustomIndex = static_cast<std::uint32_t>(i);
        instances[i].mask = 0x04u;
        instances[i].accelerationStructureReference = playerLimbBlas_.address;
    }
    instances[5].transform = segmentTransform(leftShoulder, leftElbow, 0.065f);
    instances[6].transform = segmentTransform(leftElbow, leftHand, 0.055f);
    instances[7].transform = segmentTransform(rightShoulder, rightElbow, 0.065f);
    instances[8].transform = segmentTransform(rightElbow, rightHand, 0.055f);

    // Complete the silhouette with a leather pelvis, articulated thighs/shins,
    // lifted feet and heel-to-toe roll. All reuse the shared limb BLAS on mask
    // 0x04; no new skinned enemy slot or per-frame resource is introduced.
    const auto legPoint = [&animatedBodyOrigin, &animatedBodyRight, &animatedBodyForward, &add, &scaled](float side, float y, float forward) {
        return add(add(add(animatedBodyOrigin, scaled(animatedBodyRight, side)), Vec3{0.0f, y, 0.0f}),
                   scaled(animatedBodyForward, forward));
    };
    const Vec3 leftHip = legPoint(-0.14f, -0.66f, 0.01f);
    const Vec3 rightHip = legPoint(0.14f, -0.66f, 0.01f);
    const Vec3 pelvisLeft = legPoint(-0.19f, -0.62f, 0.02f);
    const Vec3 pelvisRight = legPoint(0.19f, -0.62f, 0.02f);
    const Vec3 leftKnee = legPoint(-0.14f,
                                   -1.04f + 0.04f * lowerBodyPose.leftKneeBend,
                                   0.08f * lowerBodyPose.leftStride + 0.05f * lowerBodyPose.leftKneeBend);
    const Vec3 rightKnee = legPoint(0.14f,
                                    -1.04f + 0.04f * lowerBodyPose.rightKneeBend,
                                    0.08f * lowerBodyPose.rightStride + 0.05f * lowerBodyPose.rightKneeBend);
    const Vec3 leftAnkle = legPoint(-0.14f,
                                    -1.43f + 0.095f * lowerBodyPose.leftFootLift,
                                    0.18f * lowerBodyPose.leftStride);
    const Vec3 rightAnkle = legPoint(0.14f,
                                     -1.43f + 0.095f * lowerBodyPose.rightFootLift,
                                     0.18f * lowerBodyPose.rightStride);
    const Vec3 leftToe = add(add(leftAnkle, scaled(animatedBodyForward, 0.26f)),
                             Vec3{0.0f, 0.055f * lowerBodyPose.leftToeRoll, 0.0f});
    const Vec3 rightToe = add(add(rightAnkle, scaled(animatedBodyForward, 0.26f)),
                              Vec3{0.0f, 0.055f * lowerBodyPose.rightToeRoll, 0.0f});
    instances[9].transform = segmentTransform(pelvisLeft, pelvisRight, 0.16f);
    instances[10].transform = segmentTransform(leftHip, leftKnee, 0.105f);
    instances[11].transform = segmentTransform(leftKnee, leftAnkle, 0.09f);
    instances[12].transform = segmentTransform(rightHip, rightKnee, 0.105f);
    instances[13].transform = segmentTransform(rightKnee, rightAnkle, 0.09f);
    instances[14].transform = segmentTransform(leftAnkle, leftToe, 0.11f);
    instances[15].transform = segmentTransform(rightAnkle, rightToe, 0.11f);
    const Vec3 headBase = add(add(animatedBodyOrigin, Vec3{0.0f, -0.16f, 0.0f}), scaled(animatedBodyForward, 0.20f));
    const Vec3 headTop = add(add(animatedBodyOrigin, Vec3{0.0f, 0.15f, 0.0f}), scaled(animatedBodyForward, 0.20f));
    instances[16].transform = segmentTransform(headBase, headTop, 0.145f);
    instances[16].mask = 0x10u; // Hero-mirror reflection only; never first-person primary.
    instances[17] = instances[0];
    instances[17].instanceCustomIndex = 17u;
    instances[17].mask = 0x20u;
    instances[17].accelerationStructureReference = finaleRoofBlas_.address;
    instances[17].transform = {{
        1.0f, 0.0f, 0.0f, -2.72f * std::clamp(lich.finaleSkylightOpenProgress, 0.0f, 1.0f),
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f}};
    instances[CharacterRenderSlot::kSecondSkeletonTlasInstanceIndex] = characterInstances[1];
    instances[19] = instances[0];
    instances[19].instanceCustomIndex = 19u;
    instances[19].mask = 0x01u;
    instances[19].accelerationStructureReference = waterfallBlas_.address;
    instances[19].transform = {{
        waterfallScale.depth, 0.0f, 0.0f, -2.32f,
        0.0f, waterfallScale.vertical, 0.0f, 0.0f,
        0.0f, 0.0f, waterfallScale.crossLane, -15.26f}};
    const RtHeldLightGpu heldLightGpu{{
        frame.heldLight.worldFromLight[12],
        frame.heldLight.worldFromLight[13],
        frame.heldLight.worldFromLight[14],
        frame.heldLight.active ? frame.torchLightStrength : 0.0f}};
    if (!WriteBuffer(heldLightBuffer_, &heldLightGpu, sizeof(heldLightGpu),
                     "held light", diagnostic) ||
        !WriteBuffer(instanceBuffer_, instances.data(), sizeof(instances),
                     "animated TLAS instance", diagnostic))
    {
        return false;
    }

    VkMemoryBarrier hostWriteBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    hostWriteBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    hostWriteBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_HOST_BIT,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR |
                             VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                         0u,
                         1u,
                         &hostWriteBarrier,
                         0u,
                         nullptr,
                         0u,
                         nullptr);

    for (std::size_t bucket = 0u; bucket < CharacterRenderSlot::kMaximumSkeletonPoseBuckets; ++bucket)
    {
        const bool updateBucket = bucket == 0u ? updateSkeletonPose0 : updateSkeletonPose1;
        if (!updateBucket)
        {
            continue;
        }
        const auto& skeletonBucketGpu = characterSlot_.SkeletonGpu(bucket);
        const auto& skeletonVertices = characterSlot_.SkeletonVertices(bucket);
        VkAccelerationStructureGeometryKHR skeletonGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        skeletonGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        skeletonGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        skeletonGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        skeletonGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        skeletonGeometry.geometry.triangles.vertexData.deviceAddress = skeletonBucketGpu.vertices.address;
        skeletonGeometry.geometry.triangles.vertexStride = sizeof(horde::scene::SkinnedRtVertex);
        skeletonGeometry.geometry.triangles.maxVertex = static_cast<std::uint32_t>(skeletonVertices.size() - 1u);
        skeletonGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
        VkAccelerationStructureBuildGeometryInfoKHR skeletonUpdateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        skeletonUpdateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        skeletonUpdateInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        skeletonUpdateInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
        skeletonUpdateInfo.srcAccelerationStructure = skeletonBucketGpu.accelerationStructure.handle;
        skeletonUpdateInfo.dstAccelerationStructure = skeletonBucketGpu.accelerationStructure.handle;
        skeletonUpdateInfo.geometryCount = 1u;
        skeletonUpdateInfo.pGeometries = &skeletonGeometry;
        skeletonUpdateInfo.scratchData.deviceAddress = skeletonBucketGpu.updateScratch.address;
        VkAccelerationStructureBuildRangeInfoKHR skeletonRange{};
        skeletonRange.primitiveCount = static_cast<std::uint32_t>(skeletonVertices.size() / 3u);
        const VkAccelerationStructureBuildRangeInfoKHR* skeletonRanges[] = {&skeletonRange};
        vkCmdBuildAccelerationStructuresKHR_(commandBuffer, 1u, &skeletonUpdateInfo, skeletonRanges);

    }

    if (updateSkeletonPose0 || updateSkeletonPose1)
    {
        VkMemoryBarrier skeletonBuildBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        skeletonBuildBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        skeletonBuildBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR |
                                 VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             0u,
                             1u,
                             &skeletonBuildBarrier,
                             0u,
                             nullptr,
                             0u,
                             nullptr);
    }

    if (updateLich)
    {
        VkAccelerationStructureGeometryKHR lichGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        lichGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        lichGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        lichGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        lichGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        lichGeometry.geometry.triangles.vertexData.deviceAddress = lichVertexBuffer_.address;
        lichGeometry.geometry.triangles.vertexStride = sizeof(horde::scene::TexturedSkinnedRtVertex);
        lichGeometry.geometry.triangles.maxVertex = static_cast<std::uint32_t>(lichSkinnedVertices_.size() - 1u);
        lichGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
        VkAccelerationStructureBuildGeometryInfoKHR lichUpdateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        lichUpdateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        lichUpdateInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        lichUpdateInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
        lichUpdateInfo.srcAccelerationStructure = lichBlas_.handle;
        lichUpdateInfo.dstAccelerationStructure = lichBlas_.handle;
        lichUpdateInfo.geometryCount = 1u;
        lichUpdateInfo.pGeometries = &lichGeometry;
        lichUpdateInfo.scratchData.deviceAddress = lichBlasUpdateScratch_.address;
        VkAccelerationStructureBuildRangeInfoKHR lichRange{};
        lichRange.primitiveCount = static_cast<std::uint32_t>(lichSkinnedVertices_.size() / 3u);
        const VkAccelerationStructureBuildRangeInfoKHR* lichRanges[] = {&lichRange};
        vkCmdBuildAccelerationStructuresKHR_(commandBuffer, 1u, &lichUpdateInfo, lichRanges);

        VkMemoryBarrier lichBuildBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        lichBuildBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        lichBuildBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             0u,
                             1u,
                             &lichBuildBarrier,
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
                                                const RtSceneFrameInputs& frame,
                                                std::string& diagnostic)
{
    if (!ready_)
    {
        diagnostic = "RT scene is not ready.";
        return false;
    }
    const bool scaledPresentation = dispatchExtent_.width != swapchainExtent.width ||
                                    dispatchExtent_.height != swapchainExtent.height;
    if (scaledPresentation && !scaledBlitSupported_)
    {
        diagnostic = "This Vulkan device cannot linearly upscale the RT storage format to the presentation format.";
        return false;
    }

    if (!UpdateDynamicInstances(commandBuffer, frame, diagnostic))
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
    const std::array<float, 3u> staffWorldPosition = characterSlot_.LichStaffWorldPosition(frame.lich);
    const float heldPropDepth = frame.heldItemKinematics.heldPropDepth;
    const RtSceneTuning tuning = ClampRtSceneTuning(frame.tuning);
    const RtLightTuning& torchTuning = tuning.lights[static_cast<std::size_t>(RtLightGroup::Torch)];
    const RtLightTuning& skylightTuning = tuning.lights[static_cast<std::size_t>(RtLightGroup::Skylight)];
    const RtLightTuning& passageTuning = tuning.lights[static_cast<std::size_t>(RtLightGroup::Passage)];
    const RtLightTuning& staffTuning = tuning.lights[static_cast<std::size_t>(RtLightGroup::Staff)];
    const ScenePushConstants pushConstants{frame.cameraYaw,
                                           frame.cameraPitch,
                                           frame.torchLightStrength,
                                           frame.walkTime,
                                           frame.cameraX,
                                           frame.cameraZ,
                                           frame.walkAmount,
                                            presentationUsesBgra_ && !scaledPresentation ? 1.0f : 0.0f,
                                            std::clamp(frame.outputExposure, 0.2f, 1.4f),
                                            std::clamp(frame.combat.damageFlash, 0.0f, 1.0f),
                                            frame.roster.selectedEnemy == horde::gameplay::EnemyKind::Lich ? 1.0f : 0.0f,
                                            std::clamp(frame.lich.staffLightStrength, 0.0f, 2.2f),
                                            staffWorldPosition[0],
                                            staffWorldPosition[1],
                                            staffWorldPosition[2],
                                             std::clamp(frame.lich.finaleSkylightOpenProgress, 0.0f, 1.0f),
                                             std::clamp(frame.lich.finaleDawnRevealProgress, 0.0f, 1.0f),
                                             heldPropDepth,
                                             static_cast<float>(frame.waterQuality),
                                             tuning.waterfallWidthScale,
                                             tuning.fogDensityScale,
                                             torchTuning.hueDegrees,
                                             torchTuning.intensityScale,
                                             skylightTuning.hueDegrees,
                                             skylightTuning.intensityScale,
                                             passageTuning.hueDegrees,
                                             passageTuning.intensityScale,
                                             staffTuning.hueDegrees,
                                             staffTuning.intensityScale,
                                             static_cast<float>(tuning.workloadPreset)};
    lastOutputRedBlueSwapApplied_ = pushConstants.outputRedBlueSwap > 0.5f;
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

    if (scaledPresentation)
    {
        VkImageBlit blitRegion{};
        blitRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
        blitRegion.srcOffsets[1] = {static_cast<std::int32_t>(dispatchExtent_.width),
                                    static_cast<std::int32_t>(dispatchExtent_.height), 1};
        blitRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
        blitRegion.dstOffsets[1] = {static_cast<std::int32_t>(swapchainExtent.width),
                                    static_cast<std::int32_t>(swapchainExtent.height), 1};
        vkCmdBlitImage(commandBuffer,
                       storageImage_,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       swapchainImage,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1u,
                       &blitRegion,
                       VK_FILTER_LINEAR);
    }
    else
    {
        VkImageCopy copyRegion{};
        copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
        copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
        copyRegion.extent = {dispatchExtent_.width, dispatchExtent_.height, 1u};
        vkCmdCopyImage(commandBuffer,
                       storageImage_,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       swapchainImage,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1u,
                       &copyRegion);
    }

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

bool PresentableTinyRtScene::CaptureStorageImage(StorageImageCapture& capture, std::string& diagnostic)
{
    capture = {};
    if (!ready_ || storageImage_ == VK_NULL_HANDLE || storageImageLayout_ != VK_IMAGE_LAYOUT_GENERAL)
    {
        diagnostic = "RT storage image is not ready for capture.";
        return false;
    }

    const VkDeviceSize byteSize = static_cast<VkDeviceSize>(dispatchExtent_.width) *
                                  static_cast<VkDeviceSize>(dispatchExtent_.height) * 4u;
    Buffer readback;
    if (!CreateBuffer(byteSize,
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      false,
                      readback,
                      diagnostic))
    {
        diagnostic = "Failed to create RT capture readback buffer: " + diagnostic;
        return false;
    }

    struct CaptureCommands
    {
        VkImage image;
        VkBuffer buffer;
        VkExtent2D extent;
    } commands{storageImage_, readback.buffer, dispatchExtent_};
    const auto record = [](VkCommandBuffer commandBuffer, void* userData) {
        const auto* captureCommands = static_cast<const CaptureCommands*>(userData);
        SetImageBarrier(commandBuffer,
                        captureCommands->image,
                        VK_IMAGE_LAYOUT_GENERAL,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_ACCESS_SHADER_WRITE_BIT,
                        VK_ACCESS_TRANSFER_READ_BIT);

        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
        copy.imageExtent = {captureCommands->extent.width, captureCommands->extent.height, 1u};
        vkCmdCopyImageToBuffer(commandBuffer,
                               captureCommands->image,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               captureCommands->buffer,
                               1u,
                               &copy);

        SetImageBarrier(commandBuffer,
                        captureCommands->image,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_IMAGE_LAYOUT_GENERAL,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                        VK_ACCESS_TRANSFER_READ_BIT,
                        VK_ACCESS_SHADER_WRITE_BIT);
    };
    if (!RunOneTimeCommands(record, &commands, diagnostic))
    {
        DestroyBuffer(readback);
        diagnostic = "Failed to read back RT storage image: " + diagnostic;
        return false;
    }

    void* mapped = nullptr;
    if (vkMapMemory(device_, readback.memory, 0u, byteSize, 0u, &mapped) != VK_SUCCESS || mapped == nullptr)
    {
        DestroyBuffer(readback);
        diagnostic = "Failed to map RT capture readback memory.";
        return false;
    }

    capture.width = dispatchExtent_.width;
    capture.height = dispatchExtent_.height;
    capture.redBlueSwapNormalised = lastOutputRedBlueSwapApplied_;
    capture.rgba.resize(static_cast<std::size_t>(byteSize));
    std::memcpy(capture.rgba.data(), mapped, capture.rgba.size());
    vkUnmapMemory(device_, readback.memory);
    DestroyBuffer(readback);

    if (capture.redBlueSwapNormalised)
    {
        for (std::size_t offset = 0; offset < capture.rgba.size(); offset += 4u)
        {
            std::swap(capture.rgba[offset], capture.rgba[offset + 2u]);
        }
    }

    diagnostic.clear();
    return true;
}

} // namespace horde::vulkan::raytracing
