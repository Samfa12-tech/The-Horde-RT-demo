#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <vulkan/vulkan.h>

#include "gameplay/SwordCombat.h"
#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/items/HeldItemKinematics.h"
#include "gameplay/items/HeldItemState.h"
#include "gameplay/items/HeldLightState.h"
#include "gameplay/interactions/ChestRewardSequence.h"
#include "gameplay/interactions/FinaleSequence.h"
#include "gameplay/interactions/InteractionState.h"
#include "gameplay/items/LanternPendulum.h"
#include "vulkan/raytracing/CharacterRenderSlot.h"
#include "vulkan/raytracing/FireEmitterBuffer.h"
#include "vulkan/raytracing/HeldItemBlasMeasurements.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"
#include "vulkan/raytracing/RtGpuResources.h"
#include "vulkan/raytracing/RtPipelineBundle.h"
#include "vulkan/raytracing/RtSceneTuning.h"
#include "vulkan/raytracing/RtStaticMeshSlot.h"

namespace horde::vulkan::raytracing
{

struct PresentableTinyRtScenePreflightTestAccess;

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
    horde::gameplay::interactions::InteractionState interaction{};
    horde::gameplay::interactions::ChestRewardSnapshot chestReward{};
    horde::gameplay::interactions::FinaleSequenceSnapshot finale{};
    horde::gameplay::interactions::LanternPendulumSnapshot lanternPendulum{};
    horde::gameplay::items::HeldItemTransform rewardLanternWorldFromHinge{};
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
    const std::array<std::uint8_t, kTlasInstanceCount>& LastInstanceMasks() const
    {
        return lastInstanceMasks_;
    }
    bool LastPlayerPrimaryVisible() const { return lastPlayerPrimaryVisible_; }
    const PlayerGripAgreement& RewardLanternGripAgreement() const
    {
        return rewardLanternGripAgreement_;
    }
    const PlayerGripAgreement& RewardLanternAuthorityAgreement() const
    {
        return rewardLanternAuthorityAgreement_;
    }
    const std::array<float, 3u>& RewardLanternFinalGripPosition() const
    {
        return rewardLanternFinalGripPosition_;
    }
    const std::array<float, 3u>& RewardLanternRingGripPosition() const
    {
        return rewardLanternRingGripPosition_;
    }
    const std::array<float, 3u>& RewardLanternBodyPosition() const
    {
        return rewardLanternBodyPosition_;
    }
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
    std::uint32_t ProductionPaneSecondaryOriginCount() const
    {
        return productionPaneSecondaryOriginCount_;
    }
    std::uint32_t ProductionPaneSecondaryTerminalCount() const
    {
        return productionPaneSecondaryTerminalCount_;
    }
    std::uint32_t ProductionPaneSecondarySameMediumCount() const
    {
        return productionPaneSecondarySameMediumCount_;
    }
    std::uint32_t ProductionPaneSecondaryDifferentMediumCount() const
    {
        return productionPaneSecondaryDifferentMediumCount_;
    }
    std::uint32_t SecondaryNearSelfHitCount() const { return secondaryNearSelfHitCount_; }
    std::uint32_t PrimaryOpenMissCount() const { return primaryOpenMissCount_; }
    std::uint32_t PrimaryOpenOpaqueCount() const { return primaryOpenOpaqueCount_; }
    std::uint32_t PrimaryMismatchedExitCount() const { return primaryMismatchedExitCount_; }
    std::uint32_t PrimaryInterfaceBudgetCount() const { return primaryInterfaceBudgetCount_; }
    std::uint32_t PrimaryVolumeBudgetCount() const { return primaryVolumeBudgetCount_; }
    std::uint32_t ShadowOpenMissCount() const { return shadowOpenMissCount_; }
    std::uint32_t ShadowMismatchedExitCount() const { return shadowMismatchedExitCount_; }
    std::uint32_t PrimaryTirCount() const { return primaryTirCount_; }
    std::uint32_t PrimaryInterfaceBudgetOpenVolumeCount() const
    {
        return primaryInterfaceBudgetOpenVolumeCount_;
    }
    std::uint32_t PrimaryInterfaceBudgetClosedVolumeCount() const
    {
        return primaryInterfaceBudgetClosedVolumeCount_;
    }
    std::uint32_t ShadowMismatchEmptyCount() const { return shadowMismatchEmptyCount_; }
    std::uint32_t ShadowImplicitOriginExitCount() const
    {
        return shadowImplicitOriginExitCount_;
    }
    std::uint32_t SecondaryDielectricTerminalCount() const
    {
        return secondaryDielectricTerminalCount_;
    }
    std::uint32_t PrimaryTirTerminationCount() const
    {
        return primaryTirTerminationCount_;
    }
    std::uint32_t ShadowFiniteEndpointVolumeCount() const
    {
        return shadowFiniteEndpointVolumeCount_;
    }
    std::uint32_t PrimaryOpenOpaqueSameInstanceDifferentMaterialCount() const
    {
        return primaryOpenOpaqueSameInstanceDifferentMaterialCount_;
    }
    std::uint32_t PrimaryOpenOpaqueAfterTirCount() const
    {
        return primaryOpenOpaqueAfterTirCount_;
    }
    std::uint32_t PrimaryOpenOpaqueTerminalInstanceMask() const
    {
        return primaryOpenOpaqueTerminalInstanceMask_;
    }
    std::uint32_t PrimaryOpenOpaqueVolumeInstanceMask() const
    {
        return primaryOpenOpaqueVolumeInstanceMask_;
    }
    std::uint32_t PrimaryOpenOpaqueTerminalMaterialMask() const
    {
        return primaryOpenOpaqueTerminalMaterialMask_;
    }
    std::uint32_t PrimaryClosedVolumeAbsorptionCount() const
    {
        return primaryClosedVolumeAbsorptionCount_;
    }
    std::uint32_t PrimaryCertifiedClosedVolumeRecoveryCount() const
    {
        return primaryCertifiedClosedVolumeRecoveryCount_;
    }
    std::uint32_t ShadowCertifiedClosedVolumeRecoveryCount() const
    {
        return shadowCertifiedClosedVolumeRecoveryCount_;
    }
    std::uint32_t CertifiedClosedVolumeRecoveryReasonMask() const
    {
        return certifiedClosedVolumeRecoveryReasonMask_;
    }
    std::uint32_t PrimaryTorchPixelCount() const { return primaryTorchPixelCount_; }
    std::uint32_t PrimarySwordPixelCount() const { return primarySwordPixelCount_; }
    std::uint32_t PrimaryPlayerPixelCount() const { return primaryPlayerPixelCount_; }
    std::uint32_t PrimaryRewardRingPixelCount() const
    {
        return primaryRewardRingPixelCount_;
    }
    std::uint32_t PrimaryRewardBodyPixelCount() const
    {
        return primaryRewardBodyPixelCount_;
    }
    RtDiagnosticAvailability DiagnosticsAvailability() const
    {
        return pipelineBundle_.DiagnosticAvailability();
    }
    std::string_view SelectedOpaqueFastKey() const { return pipelineBundle_.OpaqueFastKey(); }
    std::string_view SelectedGenericDielectricKey() const
    {
        return pipelineBundle_.GenericDielectricKey();
    }
    std::string_view SelectedOpaqueFastSha256() const
    {
        return pipelineBundle_.OpaqueFastSha256();
    }
    std::string_view SelectedGenericDielectricSha256() const
    {
        return pipelineBundle_.GenericDielectricSha256();
    }
    std::string SelectedPipelineBundleIdentity() const
    {
        return pipelineBundle_.FullPairIdentity();
    }
    std::string SelectedPipelineBundleDisplayIdentity() const
    {
        return pipelineBundle_.ShortPairIdentity();
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
    friend struct PresentableTinyRtScenePreflightTestAccess;

    using Buffer = RtGpuBuffer;
    using AccelerationStructure = RtAccelerationStructure;

    struct InitialiseOrchestrationApi
    {
        void* user = nullptr;
        bool (*resolvePreflight)(void*, RtPipelineBundlePreflight&, std::string&) = nullptr;
        bool (*continueAfterPreflight)(
            void*, PresentableTinyRtScene&, VkFormat, const std::string&,
            const std::string&, const std::string&, const std::string&,
            const std::string&, const std::string&, std::string&) = nullptr;
    };

    struct TextureArray
    {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };

    bool InitialiseWithOrchestration(
        VkInstance instance,
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
        const std::string& productionAssetRoot,
        const InitialiseOrchestrationApi& api);
    bool ContinueInitialiseAfterPreflight(
        VkFormat presentationFormat,
        const std::string& skeletonAssetPath,
        const std::string& lichAssetPath,
        const std::string& materialAssetDirectory,
        const std::string& lichTextureDirectory,
        const std::string& developmentStaticAssetDirectory,
        const std::string& productionAssetRoot,
        std::string& diagnostic);

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
    bool CreateSelectedPipelineBundle(std::string& diagnostic);
    bool CreateBundleDescriptorSetLayout(const RtDescriptorIoContract& contract,
                                         VkDescriptorSetLayout& out,
                                         std::string& diagnostic);
    bool CreateBundleDescriptorPool(const RtDescriptorIoContract& contract,
                                    VkDescriptorPool& out,
                                    std::string& diagnostic);
    bool AllocateBundleDescriptorSet(VkDescriptorPool pool,
                                     VkDescriptorSetLayout layout,
                                     VkDescriptorSet& out,
                                     std::string& diagnostic);
    bool CreateBundleDiagnosticBuffer(Buffer& out, std::string& diagnostic);
    bool WriteBundleDescriptors(RtPipelineBundle& bundle, std::string& diagnostic);
    bool CreateBundlePipelineLayout(VkDescriptorSetLayout descriptorSetLayout,
                                    VkPipelineLayout& out,
                                    std::string& diagnostic);
    bool CreateBundleSharedShaderModules(VkShaderModule& miss,
                                         VkShaderModule& hit,
                                         std::string& diagnostic);
    bool CreateBundleRaygenShaderModule(const RtPipelineVariantArtifact& artifact,
                                        VkShaderModule& out,
                                        std::string& diagnostic);
    bool CreateBundleStrategyPipeline(RtMaterialStrategy strategy,
                                      VkShaderModule raygen,
                                      VkShaderModule miss,
                                      VkShaderModule hit,
                                      VkPipelineLayout layout,
                                      VkPipeline& out,
                                      std::string& diagnostic);
    bool CreateBundleStrategySbt(RtMaterialStrategy strategy,
                                 VkPipeline pipeline,
                                 Buffer& out,
                                 std::array<VkStridedDeviceAddressRegionKHR, 4u>& regions,
                                 std::string& diagnostic);
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
    std::array<std::uint8_t, kTlasInstanceCount> lastInstanceMasks_{};
    bool lastPlayerPrimaryVisible_ = false;
    PlayerGripAgreement rewardLanternGripAgreement_{};
    PlayerGripAgreement rewardLanternAuthorityAgreement_{};
    std::array<float, 3u> rewardLanternFinalGripPosition_{};
    std::array<float, 3u> rewardLanternRingGripPosition_{};
    std::array<float, 3u> rewardLanternBodyPosition_{};
    std::uint32_t dielectricTransportOverflowCount_ = 0u;
    std::uint32_t dielectricShadowOverflowCount_ = 0u;
    std::uint32_t dielectricSecondaryRejectCount_ = 0u;
    std::uint32_t dielectricUnclosedVolumeCount_ = 0u;
    std::uint32_t dielectricPrimaryUnclosedVolumeCount_ = 0u;
    std::uint32_t dielectricShadowUnclosedVolumeCount_ = 0u;
    std::uint32_t productionPaneStackFailureCount_ = 0u;
    std::uint32_t productionPaneSecondaryOriginCount_ = 0u;
    std::uint32_t productionPaneSecondaryTerminalCount_ = 0u;
    std::uint32_t productionPaneSecondarySameMediumCount_ = 0u;
    std::uint32_t productionPaneSecondaryDifferentMediumCount_ = 0u;
    std::uint32_t secondaryNearSelfHitCount_ = 0u;
    std::uint32_t primaryOpenMissCount_ = 0u;
    std::uint32_t primaryOpenOpaqueCount_ = 0u;
    std::uint32_t primaryMismatchedExitCount_ = 0u;
    std::uint32_t primaryInterfaceBudgetCount_ = 0u;
    std::uint32_t primaryVolumeBudgetCount_ = 0u;
    std::uint32_t shadowOpenMissCount_ = 0u;
    std::uint32_t shadowMismatchedExitCount_ = 0u;
    std::uint32_t primaryTirCount_ = 0u;
    std::uint32_t primaryInterfaceBudgetOpenVolumeCount_ = 0u;
    std::uint32_t primaryInterfaceBudgetClosedVolumeCount_ = 0u;
    std::uint32_t shadowMismatchEmptyCount_ = 0u;
    std::uint32_t shadowImplicitOriginExitCount_ = 0u;
    std::uint32_t secondaryDielectricTerminalCount_ = 0u;
    std::uint32_t primaryTirTerminationCount_ = 0u;
    std::uint32_t shadowFiniteEndpointVolumeCount_ = 0u;
    std::uint32_t primaryOpenOpaqueSameInstanceDifferentMaterialCount_ = 0u;
    std::uint32_t primaryOpenOpaqueAfterTirCount_ = 0u;
    std::uint32_t primaryOpenOpaqueTerminalInstanceMask_ = 0u;
    std::uint32_t primaryOpenOpaqueVolumeInstanceMask_ = 0u;
    std::uint32_t primaryOpenOpaqueTerminalMaterialMask_ = 0u;
    std::uint32_t primaryClosedVolumeAbsorptionCount_ = 0u;
    std::uint32_t primaryCertifiedClosedVolumeRecoveryCount_ = 0u;
    std::uint32_t shadowCertifiedClosedVolumeRecoveryCount_ = 0u;
    std::uint32_t certifiedClosedVolumeRecoveryReasonMask_ = 0u;
    std::uint32_t primaryTorchPixelCount_ = 0u;
    std::uint32_t primarySwordPixelCount_ = 0u;
    std::uint32_t primaryPlayerPixelCount_ = 0u;
    std::uint32_t primaryRewardRingPixelCount_ = 0u;
    std::uint32_t primaryRewardBodyPixelCount_ = 0u;
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

    RtPipelineBundle pipelineBundle_;

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
