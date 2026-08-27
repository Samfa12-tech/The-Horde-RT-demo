#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "gameplay/SwordCombat.h"
#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/items/HeldItemKinematics.h"
#include "gameplay/items/HeldItemState.h"
#include "gameplay/items/HeldLightState.h"
#include "vulkan/raytracing/CharacterRenderSlot.h"
#include "vulkan/raytracing/FireEmitterBuffer.h"
#include "vulkan/raytracing/HeldItemBlasMeasurements.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"
#include "vulkan/raytracing/RtGpuResources.h"
#include "vulkan/raytracing/RtSceneTuning.h"
#include "vulkan/raytracing/RtStaticMeshSlot.h"

namespace horde::vulkan::raytracing
{

enum class WaterQuality : std::uint32_t
{
    Off = 0u,
    Mobile = 1u,
    High = 2u,
};

struct PlayerWeaponRenderPose
{
    std::array<float, 3u> rightHandLocal{};
    float swordRadians = 0.0f;
    float parryBlend = 0.0f;
    float successJolt = 0.0f;
};

PlayerWeaponRenderPose EvaluatePlayerWeaponRenderPose(
    const horde::gameplay::PlayerCombatSnapshot& playerCombat,
    float swordSwingRadians,
    float heldPropDepth);

struct RtSceneFrameInputs
{
    std::uint64_t tickIndex = 0u;
    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    float torchLightStrength = 1.0f;
    float walkTime = 0.0f;
    float cameraX = 0.0f;
    float cameraZ = 1.85f;
    float walkAmount = 0.0f;
    float outputExposure = 0.92f;
    WaterQuality waterQuality = WaterQuality::High;
    RtSceneTuning tuning{};
    horde::gameplay::CombatSnapshot combat{};
    horde::gameplay::PlayerCombatSnapshot playerCombat{};
    std::array<horde::gameplay::simulation::SkeletonEnemySnapshot,
               horde::gameplay::simulation::kSkeletonEnemyCapacity> skeletonEnemies{};
    std::size_t skeletonEnemyCount = 0u;
    horde::gameplay::TorchFailureSnapshot torchFailure{};
    horde::gameplay::items::HeldItemStates heldItems{};
    horde::gameplay::items::HeldItemKinematicsState heldItemKinematics{};
    horde::gameplay::animation::PlayerAnimationSnapshot playerAnimation{};
    PlayerRenderRoute playerRenderRoute = PlayerRenderRoute::Procedural;
    horde::gameplay::items::HeldLightState heldLight{};
    horde::gameplay::EnemyRosterSnapshot roster{};
    horde::gameplay::LichSnapshot lich{};
    horde::gameplay::ShowcaseZone zone = horde::gameplay::ShowcaseZone::Opening;
    std::array<horde::gameplay::effects::FireEmitterState,
               horde::gameplay::effects::kFireEmitterCapacity> fireEmitters{};
    std::size_t fireEmitterCount = 0u;
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

    static constexpr std::uint32_t kBlasCount = 16u;
    static constexpr std::uint32_t kTlasCount = 1u;
    static constexpr std::uint32_t kTlasInstanceCount = 20u;

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
                    std::string& diagnostic,
                    const std::string& developmentStaticAssetDirectory = {},
                    const std::string& productionAssetRoot = {});

    void Destroy();

    bool IsReady() const { return ready_; }
    VkExtent2D DispatchExtent() const { return dispatchExtent_; }
    const std::string& MaterialEncoding() const { return materialEncoding_; }
    std::uint32_t BlasCount() const { return ready_ ? kBlasCount : 0u; }
    std::uint32_t TlasCount() const { return ready_ ? kTlasCount : 0u; }
    std::uint32_t TlasInstanceCount() const { return ready_ ? kTlasInstanceCount : 0u; }
    std::size_t SkeletonPoseBucketCount() const { return characterSlot_.SkeletonPoseBucketCount(); }
    std::uint32_t PlayerSkinCadenceHz() const { return 60u; }
    std::uint64_t PlayerSkinUpdateCount() const { return playerSkinUpdateCount_; }
    double PlayerSkinAverageMilliseconds() const
    {
        return playerSkinUpdateCount_ == 0u
            ? 0.0 : playerSkinTotalMilliseconds_ / static_cast<double>(playerSkinUpdateCount_);
    }
    float PlayerMaxSocketErrorMetres() const { return playerMaxSocketErrorMetres_; }
    std::uint32_t DielectricTransportOverflowCount() const
    {
        return dielectricTransportOverflowCount_;
    }
    std::uint32_t DielectricShadowOverflowCount() const
    {
        return dielectricShadowOverflowCount_;
    }
    std::uint32_t DielectricSecondaryRejectCount() const
    {
        return dielectricSecondaryRejectCount_;
    }
    std::uint32_t DielectricUnclosedVolumeCount() const
    {
        return dielectricUnclosedVolumeCount_;
    }
    std::uint32_t DielectricPrimaryUnclosedVolumeCount() const
    {
        return dielectricPrimaryUnclosedVolumeCount_;
    }
    std::uint32_t DielectricShadowUnclosedVolumeCount() const
    {
        return dielectricShadowUnclosedVolumeCount_;
    }
    std::uint32_t ProductionPaneStackFailureCount() const
    {
        return productionPaneStackFailureCount_;
    }
    std::uint32_t ProductionPaneSecondaryRejectCount() const
    {
        return productionPaneSecondaryRejectCount_;
    }
    bool GenericStaticAssetEnabled() const { return genericStaticAssetEnabled_; }
    const RtStaticMeshMeasurements& StaticMeshMeasurements() const { return staticMeshSlot_.Measurements(); }
    VkDeviceSize StaticMeshBlasBytes() const { return staticMeshBlasBytes_; }
    VkDeviceSize StaticMeshSwordBlasBytes() const { return heldItemBlasMeasurements_.swordBytes; }
    VkDeviceSize StaticMeshTorchBlasBytes() const { return heldItemBlasMeasurements_.torchBytes; }
    VkDeviceSize StaticTextureBytes() const { return staticTextureBytes_; }
    double StaticMeshBlasBuildMilliseconds() const { return staticMeshBlasBuildMilliseconds_; }
    double StaticMeshSwordBlasBuildMilliseconds() const
    {
        return heldItemBlasMeasurements_.swordBuildMilliseconds;
    }
    double StaticMeshTorchBlasBuildMilliseconds() const
    {
        return heldItemBlasMeasurements_.torchBuildMilliseconds;
    }
    VkDeviceSize ProductionPropBlasBytes() const { return productionPropBlasBytes_; }
    double ProductionPropBlasBuildMilliseconds() const
    {
        return productionPropBlasBuildMilliseconds_;
    }

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
    bool ReadBuffer(const Buffer& buffer,
                    VkDeviceSize offset,
                    void* data,
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
    bool LoadStaticHeldItemAssets(const std::string& developmentDirectory,
                                  const std::string& productionAssetRoot,
                                  std::string& diagnostic);
    bool CreateStaticMeshResources(std::string& diagnostic);
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
    TextureArray staticBaseColor_;
    TextureArray staticNormal_;
    TextureArray staticOrm_;
    TextureArray staticEmissive_;
    VkSampler materialSampler_ = VK_NULL_HANDLE;
    std::string materialEncoding_;

    Buffer vertexBuffer_;
    Buffer indexBuffer_;
    Buffer transformBuffer_;
    Buffer instanceBuffer_;
    Buffer heldLightBuffer_;
    Buffer fireEmitterBuffer_;
    Buffer worldSurfaceBuffer_;
    Buffer staticVertexBuffer_;
    Buffer staticIndexBuffer_;
    Buffer staticGeometryTransformBuffer_;
    Buffer instanceMetadataBuffer_;
    Buffer primitiveMetadataBuffer_;
    Buffer materialMetadataBuffer_;
    Buffer dielectricDiagnosticsBuffer_;
    AccelerationStructure blas_;
    AccelerationStructure waterfallBlas_;
    AccelerationStructure finaleRoofBlas_;
    AccelerationStructure torchBlas_;
    AccelerationStructure swordBlas_;
    AccelerationStructure gothicChestBaseBlas_;
    AccelerationStructure gothicChestLidBlas_;
    AccelerationStructure rewardLanternRingBlas_;
    AccelerationStructure rewardLanternBodyBlas_;
    AccelerationStructure dielectricFixtureBlas_;
    AccelerationStructure playerBodyBlas_;
    AccelerationStructure playerLimbBlas_;
    AccelerationStructure skinnedPlayerBlas_;
    Buffer skinnedPlayerBlasUpdateScratch_;
    AccelerationStructure tlas_;
    Buffer tlasUpdateScratch_;
    RtGpuResources gpuResources_;
    CharacterRenderSlot characterSlot_;
    PlayerRenderSlot playerRenderSlot_;
    horde::scene::assets::StaticMeshAsset developmentStaticAsset_;
    horde::scene::assets::StaticMeshAsset productionTorchAsset_;
    horde::scene::assets::StaticMeshAsset productionPlayerAsset_;
    horde::scene::assets::StaticMeshAsset gothicChestBaseAsset_;
    horde::scene::assets::StaticMeshAsset gothicChestLidAsset_;
    horde::scene::assets::StaticMeshAsset rewardLanternRingAsset_;
    horde::scene::assets::StaticMeshAsset rewardLanternBodyAsset_;
    horde::scene::assets::StaticMeshAsset productionDielectricFixtureAsset_;
    std::vector<horde::scene::assets::StaticRtVertex> skinnedPlayerUpload_;
    std::uint32_t playerStaticVertexBase_ = 0u;
    std::uint32_t dielectricFixtureMaterialIndex_ = 0u;
    PlayerCpuSkinCadence playerCpuSkinCadence_ = PlayerCpuSkinCadence::Hz60;
    PlayerRenderRoute measuredPlayerRoute_ = PlayerRenderRoute::Procedural;
    std::uint64_t playerSkinUpdateCount_ = 0u;
    double playerSkinTotalMilliseconds_ = 0.0;
    float playerMaxSocketErrorMetres_ = 0.0f;
    std::uint32_t dielectricTransportOverflowCount_ = 0u;
    std::uint32_t dielectricShadowOverflowCount_ = 0u;
    std::uint32_t dielectricSecondaryRejectCount_ = 0u;
    std::uint32_t dielectricUnclosedVolumeCount_ = 0u;
    std::uint32_t dielectricPrimaryUnclosedVolumeCount_ = 0u;
    std::uint32_t dielectricShadowUnclosedVolumeCount_ = 0u;
    std::uint32_t productionPaneStackFailureCount_ = 0u;
    std::uint32_t productionPaneSecondaryRejectCount_ = 0u;
    std::string developmentStaticAssetDirectory_;
    std::string staticTextureDirectory_;
    RtStaticMeshSlot staticMeshSlot_;
    bool genericStaticAssetEnabled_ = false;
    bool genericTransmissionActive_ = false;
    bool productionHeldItemAssetsEnabled_ = false;
    VkDeviceSize staticMeshBlasBytes_ = 0u;
    VkDeviceSize staticTextureBytes_ = 0u;
    double staticMeshBlasBuildMilliseconds_ = 0.0;
    VkDeviceSize productionPropBlasBytes_ = 0u;
    double productionPropBlasBuildMilliseconds_ = 0.0;
    HeldItemBlasMeasurements heldItemBlasMeasurements_{};

    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipeline legacyPipeline_ = VK_NULL_HANDLE;
    Buffer shaderBindingTable_;
    Buffer legacyShaderBindingTable_;
    VkStridedDeviceAddressRegionKHR raygenRegion_{};
    VkStridedDeviceAddressRegionKHR missRegion_{};
    VkStridedDeviceAddressRegionKHR hitRegion_{};
    VkStridedDeviceAddressRegionKHR callableRegion_{};
    VkStridedDeviceAddressRegionKHR legacyRaygenRegion_{};
    VkStridedDeviceAddressRegionKHR legacyMissRegion_{};
    VkStridedDeviceAddressRegionKHR legacyHitRegion_{};
    VkStridedDeviceAddressRegionKHR legacyCallableRegion_{};

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
