#pragma once
#ifndef HORDE_RT_FRAME_STAGE_PROBE
#define HORDE_RT_FRAME_STAGE_PROBE 0
#endif
#if HORDE_RT_FRAME_STAGE_PROBE
#include "vulkan/raytracing/RtSceneGpuStageProbe.h"
#endif
#ifndef HORDE_RT_CHAPTER_MEASUREMENT
#define HORDE_RT_CHAPTER_MEASUREMENT 0
#endif
#ifndef HORDE_RT_REGION_PAGE_PROTOTYPE
#define HORDE_RT_REGION_PAGE_PROTOTYPE 0
#endif
#if HORDE_RT_REGION_PAGE_PROTOTYPE
#include "vulkan/raytracing/RtRegionGeometryPreparation.h"
#include "vulkan/raytracing/RtRegionPreparationRecovery.h"
#include "vulkan/raytracing/RtRegionGeometrySource.h"
#include "world/streaming/GeometryPageTable.h"
#endif
#if HORDE_RT_DISPATCH_CLOCK_PROBE
#include "vulkan/raytracing/RtDispatchClockProbe.h"
#endif
#include "vulkan/raytracing/RtGpuWorkLease.h"
#include "scene/atmosphere/IndoorDust.h"
#include "scene/DevelopmentWorldGeometry.h"
#include "scene/ChapterRegionalWater.h"
#include "scene/ForestDressing.h"
#include "gameplay/simulation/TriangleSupportSurface.h"
#include "gameplay/traversal/RescueTraversal.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <span>

#include <vulkan/vulkan.h>

#include "gameplay/SwordCombat.h"
#include "graphics/RtPresentationTransform.h"
#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/items/HeldItemKinematics.h"
#include "gameplay/items/HeldItemState.h"
#include "gameplay/items/HeldLightState.h"
#include "gameplay/interactions/ChestRewardSequence.h"
#include "gameplay/interactions/FinaleSequence.h"
#include "gameplay/interactions/InteractionState.h"
#include "gameplay/items/LanternPendulum.h"
#include "vulkan/raytracing/CharacterRenderSlot.h"
#include "scene/assets/SkinnedPbrActor.h"
#if HORDE_RT_ACTOR_WORKER_EXPERIMENT
#include "scene/assets/SkinnedPbrActorWorker.h"
#endif
#include "vulkan/raytracing/FireEmitterBuffer.h"
#include "vulkan/raytracing/HeldItemBlasMeasurements.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"
#include "vulkan/raytracing/RtGpuResources.h"
#include "vulkan/raytracing/RtPipelineBundle.h"
#include "vulkan/raytracing/RtExecutionPolicy.h"
#include "vulkan/raytracing/RtSceneTuning.h"
#include "vulkan/raytracing/RtSceneProfile.h"
#include "vulkan/raytracing/RtStaticMeshSlot.h"
#include "vulkan/raytracing/RtTextureLayerSubset.h"
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
#include "vulkan/raytracing/experimental/StagedPrimaryPass.h"
#endif

namespace horde::graphics { class GraphicsPreviewSession; class EntryMenuSession; }
namespace horde::gameplay::simulation { class GameSimulation; }

namespace horde::vulkan::raytracing
{

struct PresentableTinyRtScenePreflightTestAccess;
struct PresentableTinyRtSceneObservationTestAccess;
struct RtDiagnosticCounterPayload;

[[nodiscard]] bool TryMakeRtPipelineEvidenceIdentity(
    const RtPipelineBundleRequest& request,
    const RtPipelineVariantArtifact& opaqueFast,
    const RtPipelineVariantArtifact& genericDielectric,
    horde::telemetry::RtPipelineEvidenceIdentity& identity) noexcept;

// Read-only provenance of an already selected strategy. No policy request or
// shader words cross the platform boundary through this metadata snapshot.
struct RtSelectedPipelineArtifactMetadata
{
    std::string_view canonicalKey;
    std::string_view artifactPath;
    std::string_view spirvSha256;
    std::string_view includeSha256;
    std::size_t expectedWordCount;
    bool hasDiagnosticsBinding;
};

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

#if defined(_DEBUG)
// Hidden Windows validation only. Slots belong to a separate inspection actor;
// they do not add gameplay animations or change the production clip set.
struct ChapterActorInspectionPose {
    enum class Clip : unsigned { Idle, Walk, TurnLeft, TurnRight };
    Clip clip=Clip::Idle,blendClip=Clip::Walk;
    float seconds=0,turnPhase=0,blendSeconds=0,weight=0;
    bool hasBlend=false;
    bool operator==(const ChapterActorInspectionPose&) const=default;
};
#endif
struct RtSceneFrameInputs
{
#if defined(_DEBUG)
    // Frozen Windows light attribution only. Physical roof/lid state, ordinary
    // gameplay and Shipping rendering continue to use the authoritative value.
    bool diagnosticSuppressFinaleApertureLighting = false;
    std::optional<ChapterActorInspectionPose> diagnosticChapterActor;
#endif
    bool developmentRescueJourney = false;
    horde::gameplay::traversal::RescueTraversalSnapshot rescue{};
    horde::gameplay::dialogue::CompanionSnapshot chapterCompanion{};
    float playerSupportWorldY = horde::gameplay::kRouteFloorWorldY;
    float playerCrouchDropMetres=0;
    std::uint64_t tickIndex = 0u;
    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    horde::graphics::RtPresentationTransform presentationTransform =
        horde::graphics::RtPresentationTransform::Identity;
    float torchLightStrength = 1.0f;
    float walkTime = 0.0f;
    float cameraX = 0.0f;
    float cameraZ = 1.85f;
    float walkAmount = 0.0f;
    float outputExposure = 0.92f;
    WaterQuality waterQuality = WaterQuality::High;
    // Legacy fixtures inherit water detail; Graphics always supplies a real tier.
    std::optional<FireEmitterQuality> fireDetail;
    // Absence is the explicitly retained diagnostic LegacyWorkload policy.
    std::optional<horde::graphics::ShadowQuality> shadowQuality;
    bool previewMotion = false;
    RtSceneTuning tuning{};
    horde::gameplay::CombatSnapshot combat{};
    horde::gameplay::PlayerCombatSnapshot playerCombat{};
    std::array<horde::gameplay::simulation::SkeletonEnemySnapshot,
               horde::gameplay::simulation::kSkeletonEnemyCapacity> skeletonEnemies{};
    std::size_t skeletonEnemyCount = 0u;
    // CPU-only optional local-joint clip transition; ordinary adapter stays empty.
    SkeletonPoseBlendRequests skeletonPoseBlends{};
    horde::gameplay::TorchFailureSnapshot torchFailure{};
    horde::gameplay::items::HeldItemStates heldItems{};
    horde::gameplay::items::HeldItemKinematicsState heldItemKinematics{};
    horde::gameplay::animation::PlayerAnimationSnapshot playerAnimation{};
    PlayerRenderRoute playerRenderRoute = PlayerRenderRoute::Procedural;
    horde::gameplay::items::PlayerMountProfile playerMountProfile =
        horde::gameplay::items::PlayerMountProfile::LegacyViewRelative;
    horde::gameplay::items::HeldLightState heldLight{};
    horde::gameplay::interactions::InteractionState interaction{};
    horde::gameplay::interactions::ChestRewardSnapshot chestReward{};
    horde::gameplay::interactions::FinaleSequenceSnapshot finale{};
    horde::gameplay::interactions::LanternPendulumSnapshot lanternPendulum{};
    horde::gameplay::items::HeldItemTransform rewardLanternWorldFromHinge{};
    horde::gameplay::EnemyRosterSnapshot roster{};
    horde::gameplay::LichSnapshot lich{};
    horde::gameplay::ShowcaseZone zone = horde::gameplay::ShowcaseZone::Opening;
    // Authoritative fixed-step water contact state is passed through unchanged
    // so the renderer can consume bounded droplets/ripples without sampling
    // or advancing gameplay state of its own.
    horde::gameplay::effects::WaterContactSnapshot waterContact{};
    std::array<horde::gameplay::effects::FireEmitterState,
               horde::gameplay::effects::kFireEmitterCapacity> fireEmitters{};
    std::size_t fireEmitterCount = 0u;
};

class PresentableTinyRtScene
{
public:
    struct InitialisationStageMeasurement
    {
        std::string_view name{};
        // Steady-clock elapsed time on the initializing CPU thread, not GPU time.
        std::uint64_t cpuNanoseconds = 0u;
        bool attempted = false;
        bool succeeded = false;
    };
    struct InitialisationPipelineMeasurement
    {
        std::string_view strategy{};
        RtExecutionBackend backend = RtExecutionBackend::Unsupported;
        VkResult result = VK_NOT_READY;
        std::uint64_t cpuNanoseconds = 0u;
        bool attempted = false;
        bool pipelineCacheWasNull = false;
    };
    struct InitialisationSbtMeasurement
    {
        std::string_view strategy{};
        std::uint64_t cpuNanoseconds = 0u;
        bool attempted = false;
        bool succeeded = false;
    };
    struct InitialisationMeasurements
    {
        std::vector<InitialisationStageMeasurement> stages;
        std::vector<InitialisationPipelineMeasurement> pipelines;
        std::vector<InitialisationSbtMeasurement> shaderBindingTables;
        // From immediately after the prior-resource Destroy through return;
        // failed-attempt cleanup is included, initial prior-resource cleanup is not.
        std::uint64_t totalCpuNanoseconds = 0u;
        bool succeeded = false;
    };

#if defined(_DEBUG) || (defined(HORDE_RT_DEBUG_CHECKPOINTS) && HORDE_RT_DEBUG_CHECKPOINTS)
    // Process-local opaque identities, read on the render owner thread after
    // completion. No device addresses, resource contents or ownership transfer.
    struct ResourceHandleSnapshot
    {
        bool ready = false;
        std::vector<std::uint64_t> bottomLevelAccelerationStructures;
        std::vector<std::uint64_t> topLevelAccelerationStructures;
        std::vector<std::uint64_t> pipelines;
        std::vector<std::uint64_t> shaderBindingTableBuffers;
        std::vector<std::uint64_t> descriptorSets;
        std::vector<std::uint64_t> textureImages;
        std::uint64_t outputImage = 0u;
        std::uint64_t outputMemory = 0u;
        std::uint64_t outputView = 0u;
        bool operator==(const ResourceHandleSnapshot&) const = default;
    };
    [[nodiscard]] ResourceHandleSnapshot CaptureResourceHandles() const;
#endif
    struct StorageImageCapture
    {
        std::uint32_t width = 0u;
        std::uint32_t height = 0u;
        bool redBlueSwapNormalised = false;
        std::vector<std::uint8_t> rgba;
    };

    // Owner maxima, not live counts. The explicit forest proof adds two masters.
    static constexpr std::uint32_t kOrdinaryBlasCount = 21u;
    static constexpr std::uint32_t kBlasCount = kOrdinaryBlasCount + 2u;
    static constexpr std::uint32_t kCollapseInstanceIndex = 21u;
    static constexpr std::uint32_t kPlayerTorchInstanceIndex = 22u;
    static constexpr std::uint32_t kPlayerSwordScabbardMetadataIndex = 23u;
    static constexpr std::uint32_t kPlayerSwordScabbardInstanceIndex = 25u;
    static constexpr std::uint32_t kWaterDropletMetadataIndex = 24u;
    static constexpr std::uint32_t kWaterDropletInstanceIndex = 26u;
    static constexpr std::uint32_t kKeeperMetadataIndex = CharacterRenderSlot::kKeeperMetadataIndex;
    static constexpr std::uint32_t kKeeperInstanceIndex = CharacterRenderSlot::kKeeperTlasInstanceIndex;
    static constexpr std::uint32_t kTlasCount = 1u;
    static constexpr std::uint32_t kOrdinaryTlasInstanceCount = kRtOrdinaryShowcaseInstanceCount;
    // Storage capacity; builds and updates use the separate live count.
    static constexpr std::uint32_t kTlasInstanceCount = kRtTlasInstanceCapacity;
    // Keep the two existing Keeper body instances at their original TLAS slots.
    // Their custom index remains the old torch metadata index1.
    static constexpr std::uint32_t kKeeperTorchFirstTlasInstance = 23u;
    static constexpr std::uint32_t kKeeperTorchInstanceCount = 2u;
    static_assert(kKeeperTorchFirstTlasInstance + kKeeperTorchInstanceCount ==
                  kPlayerSwordScabbardInstanceIndex);
    static_assert(kPlayerSwordScabbardMetadataIndex < kRtInstanceMetadataCapacity);
    static_assert(kWaterDropletMetadataIndex < kRtInstanceMetadataCapacity);
    static_assert(kKeeperMetadataIndex < kRtInstanceMetadataCapacity);
    static_assert(kOrdinaryTlasInstanceCount == 28u);
    static_assert(kOrdinaryTlasInstanceCount == kKeeperInstanceIndex + 1u);
    static_assert(kTlasInstanceCount == kOrdinaryTlasInstanceCount +
                  horde::gameplay::simulation::kForestTreePlacementContract.size() + 12u);

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
                    const std::string& productionAssetRoot = {},
                    RtExecutionBackend executionBackend = RtExecutionBackend::RayTracingPipeline,
                    RtSceneProfile sceneProfile = RtSceneProfile::Showcase,
                    bool glassEnabled = true,
                    VkPipelineCache pipelineCache = VK_NULL_HANDLE,
                    RtBundleCompiledPipelineCache* compiledPipelineCache = nullptr);

    void Destroy();
#if defined(_DEBUG)
    enum class ForestRouteProofMode { Disabled, BakedResident, SharedExisting64, SharedF01Sector76, SharedRetention64 };
    bool SetForestRouteProofMode(ForestRouteProofMode mode, std::string& diagnostic);
    [[nodiscard]] std::string ForestRouteProofJson() const;
#endif
    // Validation only: compare the last successfully submitted definitions with
    // their current shared owners. No gameplay state or resource is changed.
    bool ValidateForestRouteProof(std::string& diagnostic,
        std::optional<bool> expectedRetentionFrame = std::nullopt) const;
    [[nodiscard]] std::uint32_t SharedForestMaskedInstanceCount() const noexcept { return forestRetentionOmittedCount_; }
#if defined(_DEBUG)
    // Explicit isolated GraphicsPreview proof only. Configuration survives
    // Initialise's initial Destroy; the aliasing handle owns the admitted master.
    bool SetDiagnosticPreviewMeshPair(std::shared_ptr<const horde::scene::assets::StaticMeshAsset> master,
        const std::array<horde::gameplay::items::HeldItemTransform,2>& transforms,
        bool sharedInstances, std::string& diagnostic);
    struct DiagnosticPreviewMeshPairEvidence {
        bool active=false, sharedInstances=false, definitionsValid=false;
        std::uint32_t physicalCount=0, primitiveCount=0;
        std::uint64_t vertexBytes=0,indexBytes=0,blasBytes=0;
        std::uint64_t buildScratchAllocationBytes=0;
        double buildMilliseconds=0.;
        std::array<VkTransformMatrixKHR,2> transforms{};
        std::array<horde::gameplay::items::HeldItemTransform,2> authoredTransforms{};
        std::array<bool,2> referencesOwnedBlas{};
        std::array<std::array<std::uint32_t,4>,3> textureLayers{};
    };
    DiagnosticPreviewMeshPairEvidence DiagnosticPreviewMeshPair() const;
#endif
    std::optional<horde::gameplay::dialogue::CompanionMotionPose> ChapterCompanionPublishedMotion() const {
        return thirdActorIsCompanion_?chapterActorMotion_:std::nullopt;
    }
#if defined(_DEBUG)
    bool LoadChapterActorInspection(const std::filesystem::path& directory,std::string& diagnostic);
    bool ReleaseChapterActorInspection(std::string& diagnostic);
    std::optional<ChapterActorInspectionPose> ChapterActorInspectionPublishedPose() const {return chapterInspectionLast_;}
#endif

    // Set before Initialise; geometry admission is explicit and development-only.
    void SetDevelopmentSupportFixture(bool enabled) { developmentSupportFixture_ = enabled; }
    void SetDevelopmentRescueJourney(bool enabled) { developmentRescueJourney_=enabled; }
    // Explicit representation fallback; select before initialisation. This does
    // not change settings, geometry quality, placement, collision or save state.
    bool SetSharedForestGeometryEnabled(bool enabled) {
        if((ready_ || device_!=VK_NULL_HANDLE) && enabled!=sharedForestGeometryEnabled_)return false;
        sharedForestGeometryEnabled_=enabled;return true;
    }
    [[nodiscard]] bool SharedForestGeometryActive() const noexcept { return sharedForestGeometryActive_; }
#ifndef NDEBUG
    // Survey A/B only; call before initialization. Does not change settings.
    bool SetRegionalWaterDiagnosticEnabled(bool enabled) {
        if (ready_ || device_ != VK_NULL_HANDLE) return false;
        regionalWaterDiagnosticEnabled_ = enabled;
        return true;
    }
    bool SetFloorDebrisDiagnosticEnabled(bool enabled) {
        if (ready_ || device_ != VK_NULL_HANDLE) return false;
        floorDebrisEnabled_ = enabled;
        return true;
    }
#endif
    void SetDevelopmentWorldRoute(bool enabled, bool staged = false) { developmentWorldRoute_=enabled; stagedWorldPreparation_=staged; }
    horde::gameplay::simulation::ZoneReadiness WorldZoneReadiness(horde::gameplay::simulation::WorldZoneToken token) const;
    const horde::scene::DevelopmentWorldGeometry& WorldRouteGeometry() const { return worldRouteGeometry_; }
    std::shared_ptr<const horde::gameplay::simulation::TriangleSupportSurface> ChapterTerrainSurface() const { return chapterTerrainSurface_; }
    bool IsReady() const { return ready_; }
#if defined(_DEBUG)
    // Inspection only: the pinned complete CPU source, never the mutable active
    // GPU candidate. No mutable aliases or ownership operations cross this seam.
    std::shared_ptr<const RtStaticMeshSlot> DebugCachedCompleteStaticWorldSource() const noexcept {
        return ready_ && developmentRescueJourney_ && sceneProfile_ == RtSceneProfile::Showcase
            ? fullStaticGeometrySource_ : nullptr;
    }
    const horde::scene::assets::StaticMeshAsset* DebugCollapseStaticAsset() const noexcept {
        return ready_ && developmentRescueJourney_ && sceneProfile_ == RtSceneProfile::Showcase
            ? &collapseStaticAsset_ : nullptr;
    }
#endif
    // Offside GPU preparation only. The caller supplies a conservative retained
    // world-family set; an empty set restores the complete admitted geometry.
    // Shared textures, materials, player buffers and unchanged BLAS stay owned.
    std::uint64_t StaticGeometryGeneration() const noexcept { return staticGeometryGeneration_; }
    bool StaticGeometryHasCompleteContributors() const noexcept {
        return staticGeometryRetainedAllFamilies_ && staticGeometryRetainedAllTriangles_ &&
            staticGeometryRetainedTriangleRegions_ == horde::scene::assets::kAllStaticTriangleRegions;
    }
    bool PrepareStaticGeometryReplacement(std::uint64_t expectedGeneration,
        std::span<const std::uint32_t> retainedWorldFamilies, std::string& diagnostic);
    // CPU admission against the scene's pinned complete source, with current
    // generation identity. No GPU allocation, publication or visibility proof.
    std::shared_ptr<const RtStaticTriangleOmissions> CreateStaticGeometryTriangleOmissions(
        std::uint64_t expectedGeneration, std::span<const std::uint32_t> triangles,
        std::string& diagnostic) const;
    bool BeginStaticGeometryReplacement(std::uint64_t expectedGeneration,
        std::span<const std::uint32_t> retainedWorldFamilies, std::string& diagnostic,
        std::uint8_t retainedTriangleRegions = horde::scene::assets::kAllStaticTriangleRegions,
        std::shared_ptr<const RtStaticTriangleOmissions> omissions = {});
    // One bounded CPU/upload step or one owned AS submit/nonblocking fence
    // poll. Allocation and command-record latency are not time guarantees.
    bool AdvanceStaticGeometryReplacement(std::uint64_t expectedGeneration,
        VkDeviceSize maxCopyBytes, std::string& diagnostic);
    bool StaticGeometryReplacementReady() const noexcept;
    bool StaticGeometryReplacementHasGpuWork() const noexcept;
    enum class StaticDetailFrame { Render, WaitForRestore, Cutover };
    bool AdvanceOrdinaryStaticDetail(horde::gameplay::simulation::GameSimulation& simulation,
        const RtSceneFrameInputs& frame, bool allowEviction, StaticDetailFrame& decision, std::string& diagnostic);
    void FinishOrdinaryStaticDetail(horde::gameplay::simulation::GameSimulation& simulation);
    struct OrdinaryStaticDetailMeasurements {
        std::uint64_t omittedTriangles=0, unloads=0, restores=0, cancelled=0, waitFrames=0;
        std::uint64_t savedOwnerBytes=0, peakSavedOwnerBytes=0, peakOverlapOwnerBytes=0, preparationFailures=0;
    };
    const OrdinaryStaticDetailMeasurements& OrdinaryStaticDetailEvidence() const { return ordinaryStaticDetailEvidence_; }
    bool OrdinaryStaticDetailAvailable() const { return ordinaryStaticFacesVerified_ && ordinaryStaticOmissions_; }
    bool RefreshStaticGeometryReplacement(std::uint64_t expectedGeneration,
        std::string& diagnostic);
    bool CancelStaticGeometryReplacement(std::string& diagnostic);
    std::uint64_t CachedStaticGeometryPayloadBytes() const noexcept {
        return fullStaticGeometrySource_ ? fullStaticGeometrySource_->PayloadBytes() : 0u;
    }
    // Proof must complete GPU/evidence work and reset ALL recorded frame command
    // buffers. Failed proof retains both owners; no descriptors are published.
    bool CommitStaticGeometryReplacement(std::uint64_t expectedGeneration,
        bool (*retireReferences)(void*, std::string&), void* user, std::string& diagnostic);
    bool StaticGeometryCompletionUnproved() const noexcept { return staticGeometryCompletionUnproved_; }
    struct StaticGeometryReplacementMeasurements {
        double prepareMilliseconds = 0.0;
        double cutoverMilliseconds = 0.0;
        double packMilliseconds = 0.0;
        double beginMilliseconds = 0.0;
        double maximumCpuCopyAdvanceMilliseconds = 0.0;
        double maximumAdvanceMilliseconds = 0.0;
        std::uint32_t uploadAdvanceCount = 0;
        std::uint32_t asSubmissionCount = 0, asPollCount = 0, asPendingPollCount = 0;
        std::uint64_t uploadedBytes = 0;
        std::uint32_t cpuCopyAdvanceCount = 0;
        std::uint32_t cpuRegionScanAdvanceCount = 0;
        std::uint64_t scannedCpuPayloadBytes = 0;
        std::uint64_t copiedCpuPayloadBytes = 0;
        std::uint64_t oldOwnerAllocationBytes = 0;
        std::uint64_t candidateOwnerAllocationBytes = 0;
        std::uint64_t overlapOwnerAllocationBytes = 0;
        std::uint32_t oldPrimitiveCount = 0;
        std::uint32_t newPrimitiveCount = 0;
    };
    const StaticGeometryReplacementMeasurements& StaticGeometryReplacementEvidence() const noexcept {
        return staticGeometryReplacementMeasurements_;
    }
    // CPU-side timings for the most recent Initialise attempt. Failed attempts
    // retain partial measurements across cleanup until the next attempt begins.
    const InitialisationMeasurements& InitialiseMeasurements() const noexcept
    {
        return initialiseMeasurements_;
    }
    bool ReusedCompiledPipelines() const noexcept
    {
        return pipelineBundle_.ReusedCompiledPipelines();
    }
    RtSceneProfile Profile() const { return sceneProfile_; }
    bool GlassEnabled() const { return glassEnabled_; }
    void SetMistEnabled(const bool enabled) noexcept { mistEnabled_ = enabled; }
    bool MistEnabled() const noexcept { return mistEnabled_; }
    void SetDustQuality(horde::graphics::DustQuality quality) noexcept {
        if (quality == dustQuality_) return;
        dustQuality_ = quality;
        dustCache_.Invalidate();
        dustWork_ = {};
    }
    horde::graphics::DustQuality DustQuality() const noexcept { return dustQuality_; }
    std::optional<horde::graphics::DustQuality> UploadedDustQuality() const noexcept {
        return uploadedQualityControlsValid_ ? ResolveUploadedDustQuality(uploadedQualityControls_) : std::nullopt;
    }
    horde::scene::atmosphere::DustWork DustWork() const noexcept { return dustWork_; }

    // Raw successful upload only; owning completion/presentation is separate.
    std::optional<bool> UploadedMistEnabled() const noexcept
    {
        return uploadedQualityControlsValid_
            ? ResolveUploadedMistEnabled(uploadedQualityControls_) : std::nullopt;
    }
    const RtQualityControlsGpu& QualityControls() const noexcept { return uploadedQualityControls_; }
    bool HasUploadedQualityControls() const noexcept { return uploadedQualityControlsValid_; }
    FireEmitterQuality UploadedFireQuality() const noexcept { return uploadedFireQuality_; }
    // Raw last successful scene upload, not a completed-frame/presentation ACK.
    const FireEmitterUpload& UploadedFireEmitters() const noexcept { return uploadedFireEmitters_; }
    bool HasUploadedFireEmitters() const noexcept { return uploadedFireEmittersValid_; }
    bool ConfigurePreviewFireSockets(horde::graphics::GraphicsPreviewSession& session,
                                     std::string& diagnostic) const;
    bool ConfigureEntryMenu(horde::graphics::EntryMenuSession& session,
                            std::string& diagnostic) const;
    RtExecutionBackend ExecutionBackend() const
    {
        return pipelineBundle_.HasSelection() ? pipelineBundle_.Request().executionBackend
                                              : RtExecutionBackend::Unsupported;
    }
    VkExtent2D DispatchExtent() const { return dispatchExtent_; }
    const std::string& MaterialEncoding() const { return materialEncoding_; }
    std::uint32_t BlasCount() const
    {
        return ready_ ? static_cast<std::uint32_t>(ResourceInventory().bottomLevelAccelerationStructureCount) : 0u;
    }
    std::uint32_t TlasCount() const { return ready_ ? kTlasCount : 0u; }
    std::uint32_t TlasInstanceCount() const { return ready_ ? tlasInstanceCount_ : 0u; }
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
    std::uint32_t LastPlayerWorldBodyInstanceFlags() const { return lastPlayerWorldBodyInstanceFlags_; }
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
    [[nodiscard]] std::string_view SelectedDielectricQualityName() const noexcept
    {
        if (!pipelineBundle_.HasSelection()) return {};
        switch (pipelineBundle_.Request().quality)
        {
        case DielectricQuality::Mobile: return "Mobile";
        case DielectricQuality::High: return "High";
        default: return {};
        }
    }
    std::string_view SelectedOpaqueFastKey() const
    {
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
        if (stagedPrimary_) return experimental::StagedPrimaryPass::PairKey(RtMaterialStrategy::OpaqueFast);
#endif
        return pipelineBundle_.OpaqueFastKey();
    }
    [[nodiscard]] std::optional<RtSelectedPipelineArtifactMetadata>
    SelectedPipelineArtifactMetadata(RtMaterialStrategy strategy) const noexcept
    {
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
        if (stagedPrimary_) return std::nullopt;
#endif
        if (!pipelineBundle_.HasSelection() ||
            (strategy != RtMaterialStrategy::OpaqueFast && strategy != RtMaterialStrategy::GenericDielectric))
            return std::nullopt;
        const auto& artifact = pipelineBundle_.Strategy(strategy).artifact;
        return RtSelectedPipelineArtifactMetadata{artifact.canonicalKey, artifact.artifactPath,
            artifact.spirvSha256, artifact.includeSha256, artifact.expectedWordCount, artifact.hasDiagnosticsBinding};
    }
    std::string_view SelectedGenericDielectricKey() const
    {
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
        if (stagedPrimary_) return experimental::StagedPrimaryPass::PairKey(RtMaterialStrategy::GenericDielectric);
#endif
        return pipelineBundle_.GenericDielectricKey();
    }
    std::string_view SelectedOpaqueFastSha256() const
    {
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
        if (stagedPrimary_) return experimental::StagedPrimaryPass::PairSha256(RtMaterialStrategy::OpaqueFast);
#endif
        return pipelineBundle_.OpaqueFastSha256();
    }
    std::string_view SelectedGenericDielectricSha256() const
    {
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
        if (stagedPrimary_) return experimental::StagedPrimaryPass::PairSha256(RtMaterialStrategy::GenericDielectric);
#endif
        return pipelineBundle_.GenericDielectricSha256();
    }
    std::string SelectedPipelineBundleIdentity() const
    {
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
        if (stagedPrimary_) return std::string(SelectedOpaqueFastKey()) + "@" + std::string(SelectedOpaqueFastSha256()) +
            "|" + std::string(SelectedGenericDielectricKey()) + "@" + std::string(SelectedGenericDielectricSha256());
#endif
        return pipelineBundle_.FullPairIdentity();
    }
    std::string SelectedPipelineBundleDisplayIdentity() const
    {
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
        if (stagedPrimary_) return "StagedPrimaryV1Investigation";
#endif
        return pipelineBundle_.ShortPairIdentity();
    }
    std::string ExecutionOrganisationJson() const
    {
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
        if (stagedPrimary_) return stagedPrimary_->MetadataJson();
#endif
        return "{\"organisation\":\"Monolithic\"}";
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
    [[nodiscard]] horde::telemetry::RtResourceInventory ResourceInventory() const noexcept;
    [[nodiscard]] bool CollectCompletedDiagnostic(
        RtDiagnosticCounterPayload& payload,
        std::string& diagnostic);
    void PublishCompletedDiagnostic(
        const RtDiagnosticCounterPayload& payload) noexcept;

#if HORDE_RT_DISPATCH_CLOCK_PROBE
    std::uint32_t DispatchClockTicket() const noexcept { return dispatchClockTicket_; }
    bool CollectDispatchClocks(std::uint32_t ticket,std::vector<RtDispatchClockSample>&,
                              std::array<std::uint64_t,8>&,std::string&) const;
#endif
    bool RecordTraceAndCopy(VkCommandBuffer commandBuffer,
                            VkImage swapchainImage,
                            VkImageLayout& swapchainImageLayout,
                            VkExtent2D swapchainExtent,
                            const RtSceneFrameInputs& frame,
                            std::string& diagnostic,
                            RtSceneRecordObservation* observation = nullptr
#if HORDE_RT_STAGED_PRIMARY_TIMING
                            , experimental::StagedPrimaryTiming* stagedTiming = nullptr,
                            std::uint32_t timingFrameSlot = 0u
#endif
#if HORDE_RT_FRAME_STAGE_PROBE
                            , RtSceneGpuStageProbe* gpuProbe = nullptr
#endif
                            );

    // A completed offside owner can be adopted while poses keep moving only
    // through this combined path. Proof must retire previous submitted work and
    // reset prior frame references before this command's first scene record,
    // while preserving the supplied command's already-begun recording state.
    // Failure after publication retains both owners for platform teardown.
    bool RecordTraceAndCopyWithStaticReplacement(std::uint64_t expectedGeneration,
                            bool (*retireReferences)(void*, std::string&), void* user,
                            VkCommandBuffer commandBuffer, VkImage swapchainImage,
                            VkImageLayout& swapchainImageLayout, VkExtent2D swapchainExtent,
                            const RtSceneFrameInputs& frame, std::string& diagnostic,
                            RtSceneRecordObservation* observation = nullptr
#if HORDE_RT_STAGED_PRIMARY_TIMING
                            , experimental::StagedPrimaryTiming* stagedTiming = nullptr,
                            std::uint32_t timingFrameSlot = 0u
#endif
#if HORDE_RT_FRAME_STAGE_PROBE
                            , RtSceneGpuStageProbe* gpuProbe = nullptr
#endif
                            );

    // Owning render thread, only after successful submission of the recorded RT
    // command buffer. One frame in flight: pending TLAS definitions never become
    // the UPDATE baseline merely because a BUILD was recorded.
    void NotifyFrameSubmitted() noexcept;

    // Before this frame binds any scene descriptor. The callback must prove
    // the preceding graphics submission complete and reset all older recorded
    // references; it must preserve this frame's unbound recording, if begun.
    // Only the Keeper vertex allocation changes. Shared AS/textures stay owned.
    bool PrepareThirdActorVertexLifetime(const RtSceneFrameInputs& frame,
        bool (*retireReferences)(void*, std::string&), void* user,
        std::string& diagnostic, RtSceneRecordObservation* observation = nullptr);
    // Includes the private Keeper texture pair. Shared AS and static texture
    // families remain owned; texture aliases are valid while Kit owns the slot.
    bool PrepareThirdActorResourceLifetime(const RtSceneFrameInputs& frame,
        bool (*retireReferences)(void*, std::string&), void* user,
        std::string& diagnostic, RtSceneRecordObservation* observation = nullptr);
    struct KeeperVertexLifetimeEvidence {
        std::uint32_t retirementCount = 0u, restorationCount = 0u;
        std::uint64_t retiredAllocationBytes = 0u;
        VkMemoryPropertyFlags retiredMemoryFlags = 0u;
        bool retired = false;
    };
    KeeperVertexLifetimeEvidence KeeperVertexLifetime() const noexcept {
        return keeperVertexLifetime_;
    }
    struct KeeperTextureLifetimeEvidence {
        std::uint32_t retirementCount=0,restorationCount=0;
        std::uint64_t retiredDeviceLocalBytes=0,retiredHostVisibleBytes=0;
        VkMemoryPropertyFlags baseMemoryFlags=0,emissiveMemoryFlags=0;
        bool retired=false;
    };
    KeeperTextureLifetimeEvidence KeeperTextureLifetime() const noexcept { return keeperTextureLifetime_; }

    // Owning render thread only, after successful device-idle/evidence completion.
    // Replaces only the extent-dependent output image; caller must reset and
    // re-record command buffers invalidated by the descriptor update.
    // Allocation/preflight failure leaves the previous output resources intact.
    bool ResizeOutputAfterDeviceIdle(VkExtent2D extent, std::string& diagnostic);

    // Submit a successfully recorded frame before calling. Synchronously reads
    // the last RT-produced storage image; rejects a new output with no frame.
    // The returned
    // bytes are canonical RGBA even when the presentation push constant had
    // swapped red/blue for a raw copy to a BGRA swapchain.
    bool CaptureStorageImage(StorageImageCapture& capture, std::string& diagnostic);
#if !defined(NDEBUG) || HORDE_RT_CHAPTER_MEASUREMENT
    // Opt-in asset inspection: model-space copy of the exact current upload.
    // This is CPU geometry evidence, not a GPU readback or a second pose solve.
    bool CaptureViewmodelMesh(const std::string& path, std::string& diagnostic) const;
    // Opt-in asset inspection: model-space copy of the exact current world-body
    // upload and its actual TLAS transform. This is CPU geometry evidence, not readback.
    bool CapturePlayerWorldBodyMesh(const std::string& path, std::string& diagnostic) const;
#endif

private:
    friend struct PresentableTinyRtScenePreflightTestAccess;
    friend struct PresentableTinyRtSceneObservationTestAccess;
    friend struct PresentableTinyRtSceneOutputResizeTestAccess;
    friend struct PresentableTinyRtSceneStaticCutoverTestAccess;

    using Buffer = RtGpuBuffer;
    using AccelerationStructure = RtAccelerationStructure;
    struct TextureArray;
    struct ThirdActorTextureLifetimeApi {
        void* user=nullptr;
        bool (*retireReferences)(void*,std::string&)=nullptr;
        bool (*prepareKeeper)(void*,TextureArray&,TextureArray&,std::string&)=nullptr;
        void (*bindImages)(void*,const std::array<VkDescriptorImageInfo,2>&) noexcept=nullptr;
        void (*destroyImage)(void*,TextureArray&) noexcept=nullptr;
    };
    bool PrepareThirdActorTextureLifetimeWithApi(const RtSceneFrameInputs&,
        const ThirdActorTextureLifetimeApi&,std::string&);
    std::array<VkDescriptorImageInfo, 2> KeeperTextureDescriptors(bool preview = false) const noexcept;
    struct ThirdActorVertexLifetimeApi {
        void* user = nullptr;
        bool (*retireReferences)(void*, std::string&) = nullptr;
        bool (*prepareKeeper)(void*, Buffer&, std::string&) = nullptr;
        void (*bindVertices)(void*, const VkDescriptorBufferInfo&) noexcept = nullptr;
        void (*destroyVertices)(void*, Buffer&) noexcept = nullptr;
    };
    bool WantsChapterCompanion(const RtSceneFrameInputs&) const noexcept;
    bool PrepareThirdActorVertexLifetimeWithApi(const RtSceneFrameInputs&,
        const ThirdActorVertexLifetimeApi&, std::string&);

    struct PreparedStaticGeometry {
        enum class Phase { Copy, Upload, Collapse, CollapseWait, Tlas, TlasWait, Ready };
        std::uint64_t sourceGeneration = 0;
        std::uint8_t retainedTriangleRegions = horde::scene::assets::kAllStaticTriangleRegions;
        bool retainedAllFamilies = true;
        bool retainedAllTriangles = true;
        // Sparse index replacement retains the exact vertex address domain.
        // This is a borrow from the current generation, never a second owner.
        VkDeviceAddress sharedVertexAddress = 0;
        bool ready = false;
        Phase phase = Phase::Upload;
        std::chrono::steady_clock::time_point prepareStart;
        std::unique_ptr<RtGpuBufferPreparation> uploads;
        std::unique_ptr<RtStaticMeshCopy> copy;
        std::uint64_t collapseBuildOwnerBytes = 0;
        RtStaticMeshSlot slot;
        Buffer vertices, indices, transforms, instances, primitives, tlasInstances;
        AccelerationStructure collapse, tlas;
        Buffer collapseScratch, tlasScratch;
        RtGpuWorkLease buildWork;
        std::array<VkAccelerationStructureInstanceKHR, kTlasInstanceCount> definitions{};
    };
    struct StaticCutoverApi {
        void* user = nullptr;
        bool (*retireReferences)(void*, std::string&) = nullptr;
        bool (*publishDescriptors)(void*, PresentableTinyRtScene&, std::string&) = nullptr;
        void (*destroyRetired)(void*, PresentableTinyRtScene&, PreparedStaticGeometry&) noexcept = nullptr;
        // Optional mandatory first-frame recorder. Only this path admits a
        // completed candidate with an older submitted pose: the real dynamic
        // upload/TLAS record must precede its trace and old-owner retirement.
        bool (*recordCurrentFrame)(void*, PresentableTinyRtScene&, std::string&) = nullptr;
    };
    bool CommitStaticGeometryWithApi(std::uint64_t expectedGeneration,
        const StaticCutoverApi& api, std::string& diagnostic);
    bool PackLoadedStaticAssets(RtStaticMeshSlot& out,
        std::span<const std::uint32_t> retainedWorldFamilies, std::string& diagnostic,
        std::uint8_t retainedTriangleRegions = horde::scene::assets::kAllStaticTriangleRegions);
    void DestroyPreparedStaticGeometry(PreparedStaticGeometry& owner) noexcept;
    void StageTlasSubmissionDefinitions(
        const std::array<VkAccelerationStructureInstanceKHR,kTlasInstanceCount>& definitions) noexcept;
    bool BeginPreparedStaticUploads(PreparedStaticGeometry& owner, std::string& diagnostic);
    std::shared_ptr<const RtStaticMeshSlot> fullStaticGeometrySource_;
#if HORDE_RT_REGION_PAGE_PROTOTYPE
    friend struct PresentableTinyRtSceneRegionPageTestAccess;
    // Chapter packing retires the legacy glass-fixture registration when Kit
    // owns its atlas family. Metadata9 is free and outside legacy player10-13.
    static constexpr std::uint32_t kRegionPageTlasSlot=9,kRegionPageMetadataSlot=9;
    std::shared_ptr<const horde::world::streaming::GeometryPayload> regionPageSource_;
    RtRegionGeometryOwner regionPageOwner_;
    std::unique_ptr<RtRegionGeometryPreparation> regionPagePreparation_;
    horde::world::streaming::RegionStreaming regionStreaming_;
    enum class RegionCutover { None, Disable, Adopt };
    RegionCutover regionCutover_=RegionCutover::None;
    horde::world::streaming::Token regionCutoverToken_{};
    bool regionPageVisible_=true,regionRetirementAwaitingFrame_=false;
    RtRegionPreparationRecovery regionPreparationRecovery_;
    std::uint64_t regionPreparationPeakBytes_=0;
    bool PrepareRegionPageSource(std::string&);
    bool CreateInitialRegionPage(std::string&);
    bool ApplyRegionPage(std::array<VkAccelerationStructureInstanceKHR,kTlasInstanceCount>&,
        std::array<RtInstanceMetadata,kRtInstanceMetadataCapacity>&,
        horde::world::streaming::GeometryPageTable&,std::string&) const;
    void DestroyRegionPage() noexcept;
    bool AdvanceRegionPage(horde::gameplay::simulation::GameSimulation&,const RtSceneFrameInputs&,
        bool safe,bool inner,bool allowEviction,StaticDetailFrame&,std::string&);
    bool RetireCompletedRegionPage(bool (*)(void*,std::string&),void*,std::string&);
#endif
    void AdmitOrdinaryStaticDetail();
    bool RecoverOrdinaryStaticPreparation(bool safe, StaticDetailFrame& decision, std::string& diagnostic);
    std::shared_ptr<const RtStaticTriangleOmissions> ordinaryStaticOmissions_;
    bool ordinaryStaticFacesVerified_=false, ordinaryStaticCandidate_=false;
    bool ordinaryStaticCandidateReduced_=false, ordinaryStaticReduced_=false;
    bool ordinaryStaticRearm_=true;
    unsigned ordinaryStaticRestoreFailures_=0;
    OrdinaryStaticDetailMeasurements ordinaryStaticDetailEvidence_{};
    std::unique_ptr<PreparedStaticGeometry> preparedStaticGeometry_;
    std::uint64_t staticGeometryGeneration_ = 1;
    std::uint8_t staticGeometryRetainedTriangleRegions_ = horde::scene::assets::kAllStaticTriangleRegions;
    bool staticGeometryRetainedAllFamilies_ = true;
    bool staticGeometryRetainedAllTriangles_ = true;
    bool staticGeometryCompletionUnproved_ = false;
    StaticGeometryReplacementMeasurements staticGeometryReplacementMeasurements_{};

    struct OutputImageResources
    {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkDeviceSize allocationSize = 0u;
        VkMemoryPropertyFlags memoryFlags = 0u;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    };
    // Focused transaction seam for failure/ownership tests; production callbacks
    // allocate, bind only storage-image binding1, then retire the old image.
    struct OutputResizeApi
    {
        void* user = nullptr;
        bool (*create)(void*, VkExtent2D, OutputImageResources&, std::string&) = nullptr;
        void (*writeDescriptor)(void*, VkImageView) noexcept = nullptr;
        void (*destroy)(void*, OutputImageResources&) noexcept = nullptr;
    };
    bool ResizeOutputWithApi(VkExtent2D extent, std::array<std::uint32_t, 3u> groups,
                             const OutputResizeApi& api, std::string& diagnostic);
    bool CreateOutputImage(VkExtent2D extent, OutputImageResources& out,
                           std::string& diagnostic);
    void DestroyOutputImage(OutputImageResources& image) const noexcept;
    OutputImageResources OutputImage() const noexcept;
    void AdoptOutputImage(const OutputImageResources& image) noexcept;

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
        VkDeviceSize allocationSize = 0u;
        VkMemoryPropertyFlags memoryPropertyFlags = 0u;
        std::uint32_t mipLevels = 0u;
    };
    struct TextureUploadRecovery {
        Buffer staging;
        RtGpuWorkLease work;
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
        const InitialiseOrchestrationApi& api,
        RtSceneProfile sceneProfile = RtSceneProfile::Showcase,
        bool glassEnabled = true,
        VkPipelineCache pipelineCache = VK_NULL_HANDLE,
        RtBundleCompiledPipelineCache* compiledPipelineCache = nullptr);
    void ApplyGlassFixtureVisibility(std::span<VkAccelerationStructureInstanceKHR> instances) const;
    void CleanupFailedInitialisation();
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
                      std::string& diagnostic,
                      VkMemoryPropertyFlags preferredMemoryFlags = 0u) const;
    bool WriteBuffer(const Buffer& buffer,
                     const void* data,
                     VkDeviceSize size,
                     const char* label,
                     std::string& diagnostic,
                     RtSceneRecordObservation* observation = nullptr) const;
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
                       std::string& diagnostic,
                       std::span<const std::uint32_t> sourceLayers = {},
                       VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                       std::span<const RtTextureLayerAlias> sourceAliases = {},
                       std::span<const std::string> appendedArrayPaths = {},
                       TextureUploadRecovery* recovery = nullptr,
                       RtSceneRecordObservation* observation = nullptr);
    VkAccelerationStructureGeometryKHR ChapterActorGeometry() const;
    bool SupportsTextureArrayFormat(VkFormat format) const;
    bool CreateMaterialTextures(const std::string& directory, std::string& diagnostic);
    bool CreateEnvironmentTexture(const std::string& productionRoot, std::string& diagnostic);
    bool CreateLichTextures(const std::string& directory, std::string& diagnostic);
    bool CreateKeeperTexturePair(const std::string& directory, TextureArray& base,
        TextureArray& emissive, std::string& diagnostic,
        TextureUploadRecovery* recovery = nullptr, RtSceneRecordObservation* observation = nullptr);
    bool LoadStaticHeldItemAssets(const std::string& developmentDirectory,
                                  const std::string& productionAssetRoot,
                                  std::string& diagnostic);
    bool CreateStaticMeshResources(std::string& diagnostic);
    bool LoadPreviewStaticAssets(const std::string& productionAssetRoot,
                                 std::string& diagnostic);
    bool ResolvePreviewPropTransforms(std::string& diagnostic);
    bool BuildPreviewAccelerationStructures(std::string& diagnostic);
    bool BuildProfileAccelerationStructure(
        std::span<const VkAccelerationStructureGeometryKHR> geometries,
        std::span<const VkAccelerationStructureBuildRangeInfoKHR> ranges,
        VkAccelerationStructureTypeKHR type, VkBuildAccelerationStructureFlagsKHR flags,
        AccelerationStructure& out, Buffer* retainedScratch,
        std::string& diagnostic, bool* completionUnproved = nullptr,
        RtGpuWorkLease* queuedWork = nullptr);
    bool UpdatePreviewInstances(VkCommandBuffer commandBuffer,
                                const RtSceneFrameInputs& frame,
                                std::string& diagnostic,
                                RtSceneRecordObservation* observation);
    const Buffer& VertexBufferForRole(RtGeometryRole role) const;
    bool BuildAccelerationStructures(std::string& diagnostic);
    bool CreateScratchBuffer(VkDeviceSize usableSize, Buffer& out,
                             std::string& diagnostic) const;
    bool CreateSelectedPipelineBundle(std::string& diagnostic);
    [[nodiscard]] std::optional<RtCompiledPipelineKey> MakeCompiledPipelineKey(
        const RtPipelineBundlePreflight& preflight) const;
    [[nodiscard]] bool CapturePipelineEvidenceIdentity() noexcept;
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
    bool CreateBundleEntryShaderModule(const RtPipelineVariantArtifact& artifact,
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
    bool PackPlayerWorldBodyPoseUpload(std::string& diagnostic,
                                      RtSceneRecordObservation* observation = nullptr);
    bool UpdateDynamicInstances(VkCommandBuffer commandBuffer,
                                const RtSceneFrameInputs& frame,
                                std::string& diagnostic,
                                RtSceneRecordObservation* observation = nullptr);
    void ApplyKeeperTorchBodyInstances(
        std::array<VkAccelerationStructureInstanceKHR, kTlasInstanceCount>& instances) const;
    bool RunOneTimeCommands(void (*record)(VkCommandBuffer, void*), void* userData, std::string& diagnostic, bool* completionUnproved = nullptr) const;
    void DestroyBuffer(Buffer& buffer) const;
    void DestroyAccelerationStructure(AccelerationStructure& accelerationStructure);
    void DestroyTextureArray(TextureArray& texture);

    VkDeviceAddress BufferAddress(VkBuffer buffer) const;

    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue queue_ = VK_NULL_HANDLE;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    // Borrowed from the logical-device owner; never destroyed by this scene.
    VkPipelineCache pipelineCache_ = VK_NULL_HANDLE;
    // Device-owned cache; leases held by pipelineBundle_ cover all scene use.
    RtBundleCompiledPipelineCache* compiledPipelineCache_ = nullptr;
    VkExtent2D dispatchExtent_{};
    bool presentationUsesBgra_ = false;
    bool scaledBlitSupported_ = false;

    VkImage storageImage_ = VK_NULL_HANDLE;
    VkDeviceMemory storageImageMemory_ = VK_NULL_HANDLE;
    VkImageView storageImageView_ = VK_NULL_HANDLE;
    VkDeviceSize storageImageAllocationSize_ = 0u;
    VkMemoryPropertyFlags storageImageMemoryPropertyFlags_ = 0u;
    VkImageLayout storageImageLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    bool storageImageFrameRecorded_ = false;
    bool lastOutputRedBlueSwapApplied_ = false;
    TextureArray materialDiffuse_;
    TextureArray materialNormal_;
    TextureArray materialArm_;
    TextureArray environmentTexture_;
    VkSampler environmentSampler_ = VK_NULL_HANDLE;
    TextureArray lichBaseColor_;
    TextureArray lichEmissive_;
    KeeperTextureLifetimeEvidence keeperTextureLifetime_{};
    std::array<TextureArray,2> keeperTextureRestoreQuarantine_{};
    TextureUploadRecovery keeperTextureUploadRecovery_{};
    std::string keeperTextureDirectory_;
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
    // Exact last successful submission, including transform-only UPDATEs;
    // also supplies the discrete BUILD-vs-UPDATE comparison baseline.
    std::array<VkAccelerationStructureInstanceKHR, kTlasInstanceCount> tlasBuiltInstances_{};
    bool tlasInstanceDefinitionsValid_ = false;
    std::array<VkAccelerationStructureInstanceKHR, kTlasInstanceCount> tlasPendingInstances_{};
    bool tlasPendingDefinitionsValid_ = false;
    Buffer heldLightBuffer_;
    Buffer fireEmitterBuffer_;
    Buffer qualityControlsBuffer_;
    Buffer waterContactRippleBuffer_;
    RtQualityControlsGpu uploadedQualityControls_{};
    FireEmitterQuality uploadedFireQuality_ = FireEmitterQuality::Mobile;
    FireEmitterUpload uploadedFireEmitters_{};
    bool uploadedFireEmittersValid_ = false;
    bool uploadedQualityControlsValid_ = false;
    Buffer worldSurfaceBuffer_;
    Buffer staticVertexBuffer_;
    Buffer worldPlayerVertexBuffer_;
    Buffer viewmodelVertexBuffer_;
    Buffer staticIndexBuffer_;
    Buffer staticGeometryTransformBuffer_;
    Buffer instanceMetadataBuffer_;
    Buffer primitiveMetadataBuffer_;
    Buffer materialMetadataBuffer_;
    AccelerationStructure blas_;
    AccelerationStructure waterfallBlas_;
    AccelerationStructure finaleRoofBlas_;
    AccelerationStructure torchBlas_;
    AccelerationStructure worldTorchBodyBlas_;
    AccelerationStructure swordBlas_;
    AccelerationStructure playerSwordScabbardBlas_;
    AccelerationStructure gothicChestBaseBlas_;
    AccelerationStructure gothicChestLidBlas_;
    AccelerationStructure rewardLanternRingBlas_;
    AccelerationStructure rewardLanternBodyBlas_;
    AccelerationStructure dielectricFixtureBlas_;
    AccelerationStructure collapseBlas_;
    AccelerationStructure waterDropletBlas_;
    Buffer waterDropletBlasUpdateScratch_;
    AccelerationStructure playerBodyBlas_;
    AccelerationStructure playerLimbBlas_;
    AccelerationStructure skinnedPlayerBlas_;
    Buffer skinnedPlayerBlasUpdateScratch_;
    AccelerationStructure viewmodelBlas_;
    Buffer viewmodelBlasUpdateScratch_;
    AccelerationStructure tlas_;
    Buffer tlasUpdateScratch_;
    RtGpuResources gpuResources_;
    bool developmentSupportFixture_ = false;
    bool developmentWorldRoute_ = false;
    bool stagedWorldPreparation_ = false;
    bool developmentRescueJourney_ = false;
    bool regionalWaterDiagnosticEnabled_ = true;
    bool floorDebrisEnabled_ = true;
    std::vector<horde::scene::ChapterWaterTriangle> regionalWaterFaces_;
    Buffer rescueWorldUpdateScratch_;
    std::vector<std::array<float,3>> rescueWorldVertices_;
    std::vector<std::uint32_t> rescueWorldSurfaceCodes_;
    std::size_t rescueRopeVertexOffset_ = 0;
    std::uint32_t rescueWorldPrimitiveCount_ = 0;
    std::uint32_t rescueWorldMaxVertex_ = 0;
    std::uint32_t finaleRoofSurfaceBase_ = 0, finaleRoofSurfaceCount_ = 0;
    horde::scene::DevelopmentWorldGeometry worldRouteGeometry_{};
    std::shared_ptr<const horde::gameplay::simulation::TriangleSupportSurface> chapterTerrainSurface_;
    RtSceneProfile sceneProfile_ = RtSceneProfile::Showcase;
    bool glassEnabled_ = true;
    bool mistEnabled_ = true;
    horde::graphics::DustQuality dustQuality_ = horde::graphics::DustQuality::Off;
    horde::scene::atmosphere::DustWork dustWork_{};
    horde::scene::atmosphere::DustFrameCache dustCache_{};
    bool WriteDustQuality(const RtQualityControlsGpu&, const RtSceneFrameInputs&, std::string&, RtSceneRecordObservation*);
    std::vector<RtMaterialGpu> sceneMaterials_;
    std::uint32_t worldMaterialBase_ = 0u;
    std::uint32_t tlasInstanceCount_ = kOrdinaryTlasInstanceCount;
    std::array<horde::gameplay::items::HeldItemTransform, 5u> previewTransforms_{};
    std::array<horde::gameplay::effects::FireEmitterFixedStepInput, 2u> previewFireInputs_{};
#if defined(_DEBUG)
    ForestRouteProofMode forestRouteProofMode_ = ForestRouteProofMode::Disabled;
#endif
    // Candidate remains opt-in until normal-path validation admits it.
    static constexpr bool kDefaultSharedForestGeometryEnabled=false;
    bool sharedForestGeometryEnabled_=kDefaultSharedForestGeometryEnabled;
    bool sharedForestGeometryActive_=false;
    bool SelectSharedForestGeometry() const noexcept;
    bool UsesSharedForestGeometry() const noexcept { return sharedForestGeometryActive_; }
    bool UsesSharedForestRetention() const noexcept;
    bool UsesSupplementalForestProof() const noexcept;
    void AdmitSharedForestRetentionProof();
    void ApplySharedForestRetentionProof(std::span<VkAccelerationStructureInstanceKHR>, const RtSceneFrameInputs&);
    bool ForestRetentionIdentityMatches() const;
    struct ForestRetentionProof {
        std::shared_ptr<const RtStaticMeshSlot> source;
        std::shared_ptr<const horde::scene::PreparedForestDressing> prepared;
        std::array<horde::scene::ForestDressingMasterRange,2> ranges{};
        std::array<horde::scene::ForestDressingBounds,64> bounds{};
        std::array<bool,64> eligible{};
        std::uint64_t generation=0;
        bool admitted=false;
        std::uint32_t eligibleCount=0;
    } forestRetentionProof_;
    std::array<bool,64> forestRetentionLastOmitted_{};
    bool forestRetentionFrameEligible_=false;
    std::uint64_t forestRetentionFrameGeneration_=0;
    std::uint32_t forestRetentionOmittedCount_=0;
    horde::scene::ForestDressingSharedLayout forestSharedLayout_{};
    std::array<AccelerationStructure, 2u> forestMasterBlas_{};
    Buffer forestMasterBuildScratch_{};
    RtGpuWorkLease forestMasterBuildWork_{};
#if defined(_DEBUG)
    static constexpr std::uint32_t kDiagnosticPreviewPairMetadataIndex = 3u;
    std::shared_ptr<const horde::scene::assets::StaticMeshAsset> diagnosticPreviewPairMaster_;
    std::array<horde::gameplay::items::HeldItemTransform,2> diagnosticPreviewPairTransforms_{};
    bool diagnosticPreviewPairShared_=false,diagnosticPreviewPairActive_=false;
    horde::scene::assets::StaticMeshAsset diagnosticPreviewPairBaked_;
    AccelerationStructure diagnosticPreviewPairBlas_{};
    Buffer diagnosticPreviewPairScratch_{};
    RtGpuWorkLease diagnosticPreviewPairBuild_{};
    VkDeviceSize diagnosticPreviewPairScratchPeak_=0;
    double diagnosticPreviewPairBuildMilliseconds_=0.;
#endif
    VkDeviceSize scratchAddressAlignment_ = 0u;
    CharacterRenderSlot characterSlot_;
    horde::scene::assets::SkinnedPbrActor chapterActor_;
#if defined(_DEBUG)
    std::unique_ptr<horde::scene::assets::SkinnedPbrActor> chapterInspectionActor_;
    std::optional<ChapterActorInspectionPose> chapterInspectionLast_;
    bool chapterWorkerUsesInspection_=false;
#endif
#if HORDE_RT_ACTOR_WORKER_EXPERIMENT
    // Retired before actor reload/move/destruction; every frame joins before
    // any actor asset read/upload. No GPU ownership crosses this CPU lane.
    std::unique_ptr<horde::scene::assets::SkinnedPbrActorWorker> chapterPoseWorker_;
#endif
    bool chapterActorReady_ = false;
    bool thirdActorIsCompanion_ = false;
    bool thirdActorSubmittedCompanion_ = false;
    KeeperVertexLifetimeEvidence keeperVertexLifetime_{};
    bool thirdActorNeedsBuild_ = false;
    bool chapterActorNeedsRefit_ = false;
    std::uint64_t chapterActorPoseGeneration_ = UINT64_MAX;
    float chapterActorPoseSeconds_ = -1.0f;
    bool chapterActorWalking_ = false;
    horde::gameplay::dialogue::CompanionActingPose chapterActorActing_{};
    horde::gameplay::dialogue::CompanionLocomotionPose chapterActorLocomotion_{};
    std::optional<horde::gameplay::dialogue::CompanionMotionPose> chapterActorMotion_;
    PlayerRenderSlot playerRenderSlot_;
    horde::scene::assets::StaticMeshAsset developmentStaticAsset_;
    horde::scene::assets::StaticMeshAsset collapseStaticAsset_;
    horde::scene::assets::StaticMeshAsset productionTorchAsset_;
    horde::scene::assets::StaticMeshAsset playerTorchAsset_;
    horde::scene::assets::StaticMeshAsset playerSwordScabbardAsset_;
    horde::scene::assets::StaticMeshAsset productionPlayerAsset_;
    horde::scene::assets::StaticMeshAsset viewmodelAsset_;
    horde::scene::SkinnedMeshAsset viewmodelSkin_;
    std::vector<horde::scene::TexturedSkinnedRtVertex> viewmodelPoseVertices_;
    std::vector<horde::scene::SkinnedPbrTangent> viewmodelPoseTangents_;
    std::vector<horde::scene::assets::StaticRtVertex> viewmodelUpload_;
    bool viewmodelAvailable_ = false;
    bool playerBodyRemainderAvailable_ = false;
    bool viewmodelPoseCurrent_ = false;
#if !defined(NDEBUG) || HORDE_RT_CHAPTER_MEASUREMENT
    VkTransformMatrixKHR viewmodelCaptureTransform_{};
    VkTransformMatrixKHR playerWorldBodyCaptureTransform_{};
    bool playerWorldBodyPoseCurrent_ = false;
#endif
    horde::scene::assets::StaticMeshAsset gothicChestBaseAsset_;
    horde::scene::assets::StaticMeshAsset gothicChestLidAsset_;
    horde::scene::assets::StaticMeshAsset rewardLanternRingAsset_;
    horde::scene::assets::StaticMeshAsset rewardLanternBodyAsset_;
    horde::scene::assets::StaticMeshAsset productionDielectricFixtureAsset_;
    horde::scene::assets::StaticMeshAsset waterDropletAsset_;
    bool waterDropletGeometryVisible_ = false;
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
    std::uint32_t lastPlayerWorldBodyInstanceFlags_ = 0u;
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
#if HORDE_RT_DISPATCH_CLOCK_PROBE
    std::uint32_t dispatchClockTicket_=0;
#endif
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
    std::unique_ptr<experimental::StagedPrimaryPass> stagedPrimary_;
#endif
    RtExecutionPolicy executionPolicy_{};
    std::array<std::uint32_t, 3u> computeDispatchGroups_{};
    horde::telemetry::RtPipelineEvidenceIdentity pipelineEvidenceIdentity_{};
    bool pipelineEvidenceIdentityValid_ = false;
    horde::telemetry::RtPipelineEvidenceIdentity framePipelineEvidence_{};
    bool framePipelineEvidenceValid_ = false;

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
    InitialisationMeasurements initialiseMeasurements_{};
};

} // namespace horde::vulkan::raytracing
