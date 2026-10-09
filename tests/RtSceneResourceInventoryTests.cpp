#include "vulkan/raytracing/PresentableTinyRtScene.h"
#include "vulkan/raytracing/RtFrameEvidenceCoordinator.h"
#include "vulkan/raytracing/RtSceneRecordObservation.h"
#include "vulkan/raytracing/RtExecutionPolicy.h"
#include "vulkan/raytracing/TlasInstanceRefresh.h"
#include "vulkan/raytracing/RtDescriptorSetLayoutBindings.h"
#include "gameplay/effects/KeeperTorchLighting.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{

template <typename Handle>
Handle FakeHandle(const std::uintptr_t value)
{
    if constexpr (std::is_pointer_v<Handle>)
    {
        return reinterpret_cast<Handle>(value);
    }
    else
    {
        return static_cast<Handle>(value);
    }
}

template <typename Handle>
std::uint64_t HandleBits(const Handle value)
{
    if constexpr (std::is_pointer_v<Handle>)
    {
        return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(value));
    }
    else
    {
        return static_cast<std::uint64_t>(value);
    }
}

bool Require(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
    }
    return condition;
}

bool CheckTlasInstanceRefresh()
{
    using horde::vulkan::raytracing::RequiresTlasInstanceRebuild;
    std::array<VkAccelerationStructureInstanceKHR, horde::vulkan::raytracing::PresentableTinyRtScene::kTlasInstanceCount> built{};
    for (std::size_t i = 0u; i < built.size(); ++i)
    {
        built[i].instanceCustomIndex = static_cast<std::uint32_t>(i);
        built[i].accelerationStructureReference = 0x100u + i;
    }
    auto next = built;
    bool ok = Require(!RequiresTlasInstanceRebuild(built, next),
                      "unchanged instance definitions must retain UPDATE");
    next[20].transform.matrix[0][3] = 25.0f;
    next[18].transform.matrix[2][3] = -15.0f;
    ok &= Require(!RequiresTlasInstanceRebuild(built, next),
                  "ordinary player/enemy movement must not force TLAS BUILD");
    next[20].mask = 64u;
    next[18].mask = 1u;
    ok &= Require(RequiresTlasInstanceRebuild(built, next),
                  "viewmodel/second-enemy zero-mask visibility change must rebuild");
    built = next;
    ok &= Require(!RequiresTlasInstanceRebuild(built, next),
                  "after BUILD, stable definitions must return to UPDATE");
    next[20].mask = 0u;
    ok &= Require(RequiresTlasInstanceRebuild(built, next),
                  "visibility removal must rebuild too");
    next = built;
    next[4].accelerationStructureReference += 1u;
    ok &= Require(RequiresTlasInstanceRebuild(built, next),
                  "world-body BLAS ownership change must rebuild");
    next = built;
    next[4].accelerationStructureReference = 0u;
    ok &= Require(RequiresTlasInstanceRebuild(built, next),
                  "reference-zero activation-state change must never UPDATE");
    next = built;
    next[8].instanceCustomIndex += 1u;
    ok &= Require(RequiresTlasInstanceRebuild(built, next), "custom-index change must rebuild");
    next = built;
    next[8].instanceShaderBindingTableRecordOffset += 1u;
    ok &= Require(RequiresTlasInstanceRebuild(built, next), "SBT-index change must rebuild");
    next = built;
    next[8].flags = VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
    ok &= Require(RequiresTlasInstanceRebuild(built, next), "instance-flag change must rebuild");
    ok &= Require(RequiresTlasInstanceRebuild(built, std::span(next).first(20u)),
                  "instance-count change must rebuild");
    return ok;
}

struct NoWorkClock
{
    std::size_t reads = 0u;
};

enum class ExecutedRtCommand : std::uint8_t
{
    HostWriteBarrier,
    PlayerBlas,
    SkeletonBlas0,
    SkeletonBlas1,
    LichBlas,
    BlasToTlasBarrier,
    TlasUpdate,
    TlasToTraceBarrier,
    Trace,
    CopyOrBlit,
};

struct RtCommandExecutionLog
{
    std::array<ExecutedRtCommand, 10u> commands{};
    std::size_t count = 0u;
    bool observationsFollowCommands = true;

    void Push(const ExecutedRtCommand command)
    {
        if (count < commands.size())
        {
            commands[count++] = command;
        }
    }
};

std::uint64_t ReadNoWorkClock(void* user) noexcept
{
    return static_cast<NoWorkClock*>(user)->reads++;
}

std::string ReadCompactSource(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    std::string source((std::istreambuf_iterator<char>(input)),
                       std::istreambuf_iterator<char>());
    source.erase(std::remove_if(source.begin(), source.end(), [](const unsigned char value) {
                     return std::isspace(value) != 0;
                 }),
                 source.end());
    return source;
}

} // namespace

namespace horde::vulkan::raytracing
{

struct PresentableTinyRtSceneObservationTestAccess
{
    static bool CheckMistControlsAndLifetime()
    {
        bool ok = true;
        for (const auto shadow : {std::optional<horde::graphics::ShadowQuality>{},
                                  std::optional{horde::graphics::ShadowQuality::Lower},
                                  std::optional{horde::graphics::ShadowQuality::Current},
                                  std::optional{horde::graphics::ShadowQuality::Higher}})
        for (const auto workload : {RtWorkloadPreset::Lean, RtWorkloadPreset::Authored, RtWorkloadPreset::Max})
        for (const bool high : {false, true})
        {
            const auto defaultOn = ResolveRtQualityControls(shadow, workload, high);
            const auto explicitOn = ResolveRtQualityControls(shadow, workload, high, true);
            const auto off = ResolveRtQualityControls(shadow, workload, high, false);
            ok &= defaultOn && explicitOn && off && defaultOn->controls == explicitOn->controls &&
                  defaultOn->controls[3] == 0u && off->controls[3] == 1u;
            for (std::size_t index = 0u; index < 3u; ++index)
                ok &= defaultOn->controls[index] == off->controls[index];
            ok &= ResolveUploadedMistEnabled(*defaultOn) == true && ResolveUploadedMistEnabled(*off) == false;
            auto unknown = *off; unknown.controls[3] = 6u;
            ok &= !ResolveUploadedMistEnabled(unknown).has_value();
        }
        PresentableTinyRtScene scene;
        ok &= scene.MistEnabled() && !scene.UploadedMistEnabled().has_value();
        ok &= scene.DustQuality() == horde::graphics::DustQuality::Off && !scene.UploadedDustQuality();
        scene.SetDustQuality(horde::graphics::DustQuality::Standard);
        scene.SetMistEnabled(false);
        ok &= !scene.MistEnabled() && !scene.UploadedMistEnabled().has_value();
        scene.uploadedQualityControls_ = *ResolveRtQualityControls(
            horde::graphics::ShadowQuality::Current, RtWorkloadPreset::Authored, false, false, horde::graphics::DustQuality::Low);
        scene.uploadedQualityControlsValid_ = true;
        scene.SetMistEnabled(true);
        ok &= scene.MistEnabled() && scene.UploadedMistEnabled() == false;
        ok &= scene.UploadedDustQuality() == horde::graphics::DustQuality::Low;
        PresentableTinyRtScene moved(std::move(scene));
        ok &= moved.MistEnabled() && moved.UploadedMistEnabled() == false &&
              scene.MistEnabled() && !scene.UploadedMistEnabled().has_value();
        ok &= moved.DustQuality() == horde::graphics::DustQuality::Standard &&
            moved.UploadedDustQuality() == horde::graphics::DustQuality::Low && scene.DustQuality() == horde::graphics::DustQuality::Off;
        moved.SetMistEnabled(false);
        PresentableTinyRtScene assigned;
        assigned = std::move(moved);
        ok &= !assigned.MistEnabled() && assigned.UploadedMistEnabled() == false &&
              moved.MistEnabled() && !moved.UploadedMistEnabled().has_value();
        assigned.Destroy();
        ok &= assigned.DustQuality() == horde::graphics::DustQuality::Off && !assigned.UploadedDustQuality();
        ok &= assigned.MistEnabled() && !assigned.UploadedMistEnabled().has_value();
        return ok;
    }

    static bool CheckKeeperTorchBodyAliases()
    {
        PresentableTinyRtScene scene;
        scene.worldTorchBodyBlas_.address = 0xB0D1u;
        scene.torchBlas_.address = 0xF1A0u; // Distinct owner contains the held emissive core.
        std::array<VkAccelerationStructureInstanceKHR, PresentableTinyRtScene::kTlasInstanceCount> instances{};
        instances[0].mask = 0x01u;
        instances[0].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instances[0].accelerationStructureReference = 0x1234u;
        instances[PresentableTinyRtScene::kPlayerSwordScabbardInstanceIndex].instanceCustomIndex =
            PresentableTinyRtScene::kPlayerSwordScabbardMetadataIndex;
        instances[PresentableTinyRtScene::kPlayerSwordScabbardInstanceIndex].mask = 0u;
        instances[PresentableTinyRtScene::kPlayerSwordScabbardInstanceIndex].flags =
            VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instances[PresentableTinyRtScene::kPlayerSwordScabbardInstanceIndex].accelerationStructureReference = 0x5CA8u;
        instances[PresentableTinyRtScene::kPlayerSwordScabbardInstanceIndex].transform.matrix[0][3] = 1.25f;
        const auto original = instances;
        scene.ApplyKeeperTorchBodyInstances(instances);
        bool ok = true;
        for (std::size_t index = 0u; index < PresentableTinyRtScene::kKeeperTorchInstanceCount; ++index)
        {
            const auto& instance = instances[PresentableTinyRtScene::kKeeperTorchFirstTlasInstance + index];
            const auto& anchor = horde::gameplay::effects::kKeeperTorchAnchors[index];
            ok &= instance.instanceCustomIndex == 1u && instance.instanceCustomIndex < kRtInstanceMetadataCapacity &&
                  instance.mask == 0x01u && instance.accelerationStructureReference == 0xB0D1u &&
                  instance.accelerationStructureReference != scene.torchBlas_.address &&
                  instance.transform.matrix[0][3] == anchor.position[0] &&
                  instance.transform.matrix[1][3] == anchor.position[1] &&
                  instance.transform.matrix[2][3] == anchor.position[2] &&
                  instance.transform.matrix[0][0] == 1.0f && instance.transform.matrix[1][1] == 1.0f &&
                  instance.transform.matrix[2][2] == 1.0f;
        }
        // Keeper physical slots23/24 keep aliasing old torch metadata1. The
        // independent player Rag torch owns metadata22 at held TLAS slot1,
        // while the scabbard at slot25 keeps its own metadata23/BLAS. Compare
        // every non-Keeper TLAS slot; the metadata capacity includes slot23
        // and therefore is not a valid prefix length for this ownership check.
        for (std::size_t index = 0u; index < instances.size(); ++index)
        {
            if (index == PresentableTinyRtScene::kKeeperTorchFirstTlasInstance ||
                index == PresentableTinyRtScene::kKeeperTorchFirstTlasInstance + 1u)
            {
                continue;
            }
            ok &= std::memcmp(&instances[index], &original[index], sizeof(instances[index])) == 0;
        }
        instances[0].mask = 0u;
        scene.ApplyKeeperTorchBodyInstances(instances);
        ok &= instances[23].mask == 0u && instances[24].mask == 0u &&
              std::memcmp(&instances[PresentableTinyRtScene::kPlayerSwordScabbardInstanceIndex],
                          &original[PresentableTinyRtScene::kPlayerSwordScabbardInstanceIndex],
                          sizeof(instances[0])) == 0;
        scene.worldTorchBodyBlas_ = {};
        scene.torchBlas_ = {};
        return ok;
    }

    static void MarkFireUpload(PresentableTinyRtScene& scene)
    {
        scene.uploadedFireEmitters_ = {};
        scene.uploadedFireEmitters_.activeCount = 2u;
        for (std::size_t index = 0u; index < 2u; ++index)
        {
            const auto id = static_cast<std::uint32_t>(3u + index);
            scene.uploadedFireEmitters_.selectedStableIds[index] = id;
            scene.uploadedFireEmitters_.emitters[index].identity[0] = id;
            scene.uploadedFireEmitters_.emitters[index].lightPositionStrength = {{-35.5f,0.99f,-16.325f,0.8f}};
            scene.uploadedFireEmitters_.emitters[index].colourIntensity = {{1.0f,0.5f,0.1f,0.8f}};
        }
        scene.uploadedFireEmittersValid_ = true;
    }
    static bool CheckPersistentReadback()
    {
        PresentableTinyRtScene scene; // No Vulkan device: a remap would be invalid.
        std::array<std::uint8_t, 8u> uploaded{11, 23, 37, 41, 53, 67, 79, 83};
        RtGpuBuffer buffer;
        buffer.memory = FakeHandle<VkDeviceMemory>(0xCAFEu);
        buffer.size = uploaded.size();
        buffer.mappedWriteData = uploaded.data();
        buffer.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        std::array<std::uint8_t, 4u> output{};
        std::string diagnostic = "stale";
        bool ok = Require(scene.ReadBuffer(buffer, 2u, output.data(), output.size(),
                                           "fixture", diagnostic) &&
                          output == std::array<std::uint8_t, 4u>{37, 41, 53, 67} &&
                          diagnostic.empty(), "persistent read uses exact offset bytes");
        uploaded[7] = 97;
        ok &= Require(scene.ReadBuffer(buffer, 4u, output.data(), output.size(),
                                       "fixture", diagnostic) && output[3] == 97 &&
                      buffer.mappedWriteData == uploaded.data(),
                      "repeat read observes current bytes and preserves mapping ownership");
        const auto reject = [&](VkDeviceSize offset, VkDeviceSize count, void* destination)
        {
            output.fill(0xABu);
            return !scene.ReadBuffer(buffer, offset, destination, count, "fixture", diagnostic) &&
                   !diagnostic.empty() &&
                   std::all_of(output.begin(), output.end(), [](auto v) { return v == 0xABu; });
        };
        ok &= Require(reject(5u, 4u, output.data()) &&
                      reject(~VkDeviceSize{0u}, 1u, output.data()) &&
                      reject(0u, 0u, output.data()) && reject(0u, 1u, nullptr),
                      "invalid reads fail without touching destination or Vulkan");
        buffer.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        ok &= Require(reject(0u, 1u, output.data()), "noncoherent mapping fails closed");
        buffer.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        ok &= Require(reject(0u, 1u, output.data()), "nonvisible mapping fails closed");
        buffer.memory = VK_NULL_HANDLE;
        ok &= Require(reject(0u, 1u, output.data()), "missing owner memory fails closed");
        return ok;
    }

    static bool CheckGlassVisibility(PresentableTinyRtScene& scene, bool enabled)
    {
        scene.glassEnabled_ = enabled;
        scene.dielectricFixtureBlas_.address = 0xD1E1u;
        std::array<VkAccelerationStructureInstanceKHR, 4u> instances{};
        instances[0].instanceCustomIndex = 9u;
        instances[0].accelerationStructureReference = scene.dielectricFixtureBlas_.address;
        instances[0].mask = 0xFFu;
        // The procedural arm can have the same custom index. Its own BLAS,
        // lantern mixed body, and ordinary opaque world must remain admitted.
        instances[1].instanceCustomIndex = 9u;
        instances[1].accelerationStructureReference = 0xA2A2u;
        instances[1].mask = 0x04u;
        instances[2].instanceCustomIndex = 8u;
        instances[2].accelerationStructureReference = 0x1A17u;
        instances[2].mask = 0x01u;
        instances[3].instanceCustomIndex = 0u;
        instances[3].accelerationStructureReference = 0xB1A5u;
        instances[3].mask = 0xFFu;
        const auto before = instances;
        for (const auto profile : {RtSceneProfile::Showcase, RtSceneProfile::GraphicsPreview})
        {
            scene.sceneProfile_ = profile;
            instances = before;
            scene.ApplyGlassFixtureVisibility(instances);
            for (unsigned rayBit = 0u; rayBit < 8u; ++rayBit)
                if (((instances[0].mask & (1u << rayBit)) != 0u) != enabled) return false;
            if (instances[0].accelerationStructureReference != before[0].accelerationStructureReference ||
                instances[0].instanceCustomIndex != 9u) return false;
            for (std::size_t index = 1u; index < instances.size(); ++index)
                if (instances[index].mask != before[index].mask ||
                    instances[index].accelerationStructureReference != before[index].accelerationStructureReference)
                    return false;
            // A later frame/RT Lab can request all-ray visibility again; Off
            // must enforce zero anew, retaining the immutable resource owner.
            instances[0].mask = 0xFFu;
            scene.ApplyGlassFixtureVisibility(instances);
            if (instances[0].mask != (enabled ? 0xFFu : 0u)) return false;
        }
        scene.dielectricFixtureBlas_.address = 0u;
        return true;
    }

    static void AdmitPreviewInventoryFixture(PresentableTinyRtScene& scene)
    {
        // Synthetic admitted owner: public topology getters intentionally
        // require readiness. No device is bound, so this fixture never invokes
        // Vulkan; allocation inventory remains independently observable.
        scene.ready_ = true;
        scene.uploadedQualityControls_ = {{2u, 4u, 2u, 0u}};
        scene.uploadedFireQuality_ = FireEmitterQuality::Low;
        scene.uploadedQualityControlsValid_ = true;
        MarkFireUpload(scene);
        scene.sceneProfile_ = RtSceneProfile::GraphicsPreview;
        scene.tlasInstanceCount_ = 7u;
        scene.tlas_.handle = FakeHandle<VkAccelerationStructureKHR>(0x987u);
        scene.environmentTexture_.memory = FakeHandle<VkDeviceMemory>(0x988u);
        scene.environmentTexture_.allocationSize = 768u;
        scene.environmentTexture_.memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        scene.environmentTexture_.mipLevels = 10u;
    }

    static void MarkTlasDefinitions(PresentableTinyRtScene& scene)
    {
        scene.tlasInstanceDefinitionsValid_ = true;
        scene.tlasBuiltInstances_[20].accelerationStructureReference = 42u;
    }

    static void MarkPendingTlasDefinitions(PresentableTinyRtScene& scene)
    {
        scene.tlasPendingInstances_ = scene.tlasBuiltInstances_;
        scene.tlasPendingInstances_[20].accelerationStructureReference = 99u;
        scene.tlasPendingDefinitionsValid_ = true;
    }

    static bool HasTlasDefinitions(const PresentableTinyRtScene& scene,
                                   const std::uint64_t reference = 42u)
    {
        return scene.tlasInstanceDefinitionsValid_ &&
               scene.tlasBuiltInstances_[20].accelerationStructureReference == reference;
    }

    static RtGpuBuffer MakeBuffer(std::uintptr_t& next)
    {
        RtGpuBuffer buffer{};
        buffer.buffer = FakeHandle<VkBuffer>(next++);
        buffer.memory = FakeHandle<VkDeviceMemory>(next++);
        buffer.address = next++;
        buffer.size = 32u;
        buffer.allocationSize = 64u;
        buffer.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        return buffer;
    }

    static void Populate(PresentableTinyRtScene& scene)
    {
        std::uintptr_t next = 0x100u;
        const auto populateBuffer = [&next](RtGpuBuffer& buffer) {
            buffer = MakeBuffer(next);
        };
        const auto populateBlas = [&next](RtAccelerationStructure& accelerationStructure) {
            accelerationStructure.backing = MakeBuffer(next);
            accelerationStructure.handle = FakeHandle<VkAccelerationStructureKHR>(next++);
            accelerationStructure.address = next++;
        };

        for (RtGpuBuffer* buffer : std::array{
                 &scene.vertexBuffer_, &scene.indexBuffer_, &scene.transformBuffer_,
                 &scene.instanceBuffer_, &scene.heldLightBuffer_, &scene.fireEmitterBuffer_, &scene.qualityControlsBuffer_,
                 &scene.worldSurfaceBuffer_, &scene.staticVertexBuffer_,
                 &scene.worldPlayerVertexBuffer_, &scene.viewmodelVertexBuffer_,
                 &scene.staticIndexBuffer_, &scene.staticGeometryTransformBuffer_,
                 &scene.instanceMetadataBuffer_, &scene.primitiveMetadataBuffer_,
                 &scene.materialMetadataBuffer_})
        {
            populateBuffer(*buffer);
        }
        for (RtAccelerationStructure* accelerationStructure :
             std::array{
                 &scene.blas_, &scene.waterfallBlas_, &scene.finaleRoofBlas_,
                 &scene.torchBlas_, &scene.worldTorchBodyBlas_, &scene.swordBlas_,
                 &scene.playerSwordScabbardBlas_, &scene.gothicChestBaseBlas_,
                 &scene.gothicChestLidBlas_, &scene.rewardLanternRingBlas_,
                 &scene.rewardLanternBodyBlas_, &scene.dielectricFixtureBlas_,
                 &scene.playerBodyBlas_, &scene.playerLimbBlas_,
                 &scene.skinnedPlayerBlas_, &scene.viewmodelBlas_, &scene.collapseBlas_})
        {
            populateBlas(*accelerationStructure);
        }
        populateBuffer(scene.skinnedPlayerBlasUpdateScratch_);
        populateBuffer(scene.viewmodelBlasUpdateScratch_);
        scene.ready_ = true;
        scene.uploadedQualityControls_ = {{2u, 4u, 2u, 0u}};
        scene.uploadedFireQuality_ = FireEmitterQuality::Low;
        scene.uploadedQualityControlsValid_ = true;
        populateBlas(scene.tlas_);
        MarkFireUpload(scene);
        populateBuffer(scene.tlasUpdateScratch_);

        for (std::size_t bucket = 0u;
             bucket < CharacterRenderSlot::kMaximumSkeletonPoseBuckets; ++bucket)
        {
            auto& gpu = scene.characterSlot_.SkeletonGpu(bucket);
            populateBuffer(gpu.vertices);
            populateBlas(gpu.accelerationStructure);
            populateBuffer(gpu.updateScratch);
        }
        auto& lich = scene.characterSlot_.LichGpu();
        populateBuffer(lich.vertices);
        populateBlas(lich.accelerationStructure);
        populateBuffer(lich.updateScratch);

        for (const RtMaterialStrategy strategy : {
                 RtMaterialStrategy::OpaqueFast,
                 RtMaterialStrategy::GenericDielectric})
        {
            auto& resources = scene.pipelineBundle_.Strategy(strategy);
            resources.pipeline = FakeHandle<VkPipeline>(next++);
            populateBuffer(resources.shaderBindingTable);
        }
        populateBuffer(scene.pipelineBundle_.diagnosticBuffer);
        scene.pipelineBundle_.descriptorSet = FakeHandle<VkDescriptorSet>(next++);

        scene.storageImage_ = FakeHandle<VkImage>(next++);
        scene.storageImageMemory_ = FakeHandle<VkDeviceMemory>(next++);
        scene.storageImageView_ = FakeHandle<VkImageView>(next++);
        scene.storageImageAllocationSize_ = 128u;
        scene.storageImageMemoryPropertyFlags_ =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        for (PresentableTinyRtScene::TextureArray* texture :
             std::array<PresentableTinyRtScene::TextureArray*, 9u>{
                 &scene.materialDiffuse_, &scene.materialNormal_, &scene.materialArm_,
                 &scene.lichBaseColor_, &scene.lichEmissive_, &scene.staticBaseColor_,
                 &scene.staticNormal_, &scene.staticOrm_, &scene.staticEmissive_})
        {
            texture->image = FakeHandle<VkImage>(next++);
            texture->memory = FakeHandle<VkDeviceMemory>(next++);
            texture->view = FakeHandle<VkImageView>(next++);
            texture->allocationSize = 128u;
            texture->memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }
    }

    static void RemoveDiagnosticBuffer(PresentableTinyRtScene& scene)
    {
        scene.pipelineBundle_.diagnosticBuffer = {};
    }

#ifndef NDEBUG
    static StaticRtVertex MakeCaptureVertex(
        const std::array<float, 3u>& position,
        const std::array<float, 3u>& normal,
        const std::array<float, 2u>& uv)
    {
        StaticRtVertex vertex{};
        vertex.position = {position[0], position[1], position[2], 1.0f};
        vertex.normal = {normal[0], normal[1], normal[2], 0.0f};
        vertex.tangent = {1.0f, 0.0f, 0.0f, 1.0f};
        vertex.uv0 = {uv[0], uv[1], 0.0f, 1.0f};
        return vertex;
    }

    static void ConfigureCaptureFixture(PresentableTinyRtScene& scene,
                                        const bool ready,
                                        const bool poseCurrent)
    {
        scene.ready_ = ready;
        scene.viewmodelPoseCurrent_ = poseCurrent;
        scene.viewmodelCaptureTransform_ = {{-1.0f, 0.0f, 0.0f, 2.0f,
                                             0.0f, 1.0f, 0.0f, -0.75f,
                                             0.0f, 0.0f, -1.0f, 3.0f}};
        scene.viewmodelUpload_ = {
            MakeCaptureVertex({1.0f, 2.0f, 3.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}),
            MakeCaptureVertex({4.0f, 5.0f, 6.0f}, {0.0f, 1.0f, 0.0f}, {0.25f, 0.5f}),
            MakeCaptureVertex({-1.0f, -2.0f, -3.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}),
            MakeCaptureVertex({7.0f, 8.0f, 9.0f}, {0.0f, 0.0f, -1.0f}, {0.125f, 0.875f}),
            MakeCaptureVertex({-4.0f, -5.0f, -6.0f}, {0.0f, -1.0f, 0.0f}, {0.75f, 0.25f}),
            MakeCaptureVertex({10.0f, 11.0f, 12.0f}, {-1.0f, 0.0f, 0.0f}, {0.625f, 0.375f}),
        };
        scene.viewmodelAsset_.vertices = scene.viewmodelUpload_;
        scene.viewmodelAsset_.indices = {0u, 1u, 2u, 2u, 0u, 1u};
        scene.viewmodelAsset_.primitives = {
            {0u, 0u, 3u, 0u, 0u},
            {3u, 3u, 3u, 1u, 0u},
        };
        scene.viewmodelAsset_.materials.clear();
        scene.viewmodelAsset_.materials.push_back({});
        scene.viewmodelAsset_.materials.push_back({});
        scene.viewmodelAsset_.materials[0].name = "ViewmodelSleeves";
        scene.viewmodelAsset_.materials[1].name = "ViewmodelGauntlets";

        scene.playerWorldBodyPoseCurrent_ = poseCurrent;
        scene.playerWorldBodyCaptureTransform_ = {{1.0f, 0.0f, 0.0f, -2.0f,
                                                   0.0f, 1.0f, 0.0f, 0.75f,
                                                   0.0f, 0.0f, 1.0f, -3.0f}};
        scene.skinnedPlayerUpload_ = scene.viewmodelUpload_;
        scene.productionPlayerAsset_.vertices = scene.viewmodelUpload_;
        scene.productionPlayerAsset_.indices = {0u, 1u, 2u, 2u, 0u, 1u};
        scene.productionPlayerAsset_.primitives = {
            {0u, 0u, 3u, 0u, 0u},
            {3u, 3u, 3u, 1u, 0u},
        };
        scene.productionPlayerAsset_.materials.clear();
        scene.productionPlayerAsset_.materials.push_back({});
        scene.productionPlayerAsset_.materials.push_back({});
        scene.productionPlayerAsset_.materials[0].name = "BodyPrimaryVisible";
        scene.productionPlayerAsset_.materials[1].name = "NearFacePrimaryMasked";
    }

    static void SetFirstWorldBodyIndex(PresentableTinyRtScene& scene,
                                       const std::uint32_t index)
    {
        scene.productionPlayerAsset_.indices[0] = index;
    }

    static void SetWorldBodyPrimitives(PresentableTinyRtScene& scene,
                                       const bool present)
    {
        scene.productionPlayerAsset_.primitives = present
            ? std::vector<horde::scene::assets::StaticPrimitiveRecord>{
                  {0u, 0u, 3u, 0u, 0u}, {3u, 3u, 3u, 1u, 0u}}
            : std::vector<horde::scene::assets::StaticPrimitiveRecord>{};
    }

    static void ForceUnsupportedScaledPresentation(
        PresentableTinyRtScene& scene)
    {
        scene.dispatchExtent_ = {1u, 1u};
        scene.scaledBlitSupported_ = false;
    }
#endif
};

struct PresentableTinyRtSceneOutputResizeTestAccess
{
    using Scene = PresentableTinyRtScene;
    using Output = Scene::OutputImageResources;

    enum class CreateProgress : std::uint8_t
    {
        None,
        Image,
        Memory,
        View,
        Complete,
    };

    struct CallbackState
    {
        Scene* scene = nullptr;
        CreateProgress progress = CreateProgress::Complete;
        VkExtent2D requestedExtent{};
        Output oldOutput{};
        Output createdOutput{};
        Output destroyedOutput{};
        std::array<char, 4u> events{};
        std::size_t eventCount = 0u;
        bool eventOverflow = false;
        std::size_t createCount = 0u;
        std::size_t writeCount = 0u;
        std::size_t destroyCount = 0u;
        VkImageView writtenView = VK_NULL_HANDLE;
        bool descriptorSawOldState = false;
        bool destroySawPublishedState = false;
        std::string diagnostic = "transaction failure sentinel";
    };

    static void RecordEvent(CallbackState& state, const char event) noexcept
    {
        if (state.eventCount < state.events.size())
            state.events[state.eventCount] = event;
        else
            state.eventOverflow = true;
        ++state.eventCount;
    }

    static void Configure(Scene& scene)
    {
        PresentableTinyRtSceneObservationTestAccess::Populate(scene);
        scene.device_ = FakeHandle<VkDevice>(0x90000u);
        scene.storageImageView_ = FakeHandle<VkImageView>(0x90001u);
        scene.pipelineBundle_.descriptorSetLayout = FakeHandle<VkDescriptorSetLayout>(0x90002u);
        scene.pipelineBundle_.descriptorPool = FakeHandle<VkDescriptorPool>(0x90003u);
        scene.pipelineBundle_.pipelineLayout = FakeHandle<VkPipelineLayout>(0x90004u);
        scene.materialSampler_ = FakeHandle<VkSampler>(0x90005u);
        scene.dispatchExtent_ = {640u, 360u};
        scene.computeDispatchGroups_ = {80u, 45u, 1u};
        scene.storageImageLayout_ = VK_IMAGE_LAYOUT_GENERAL;
        scene.storageImageFrameRecorded_ = true;
        scene.lastOutputRedBlueSwapApplied_ = true;
        scene.pipelineEvidenceIdentityValid_ = true;
        scene.framePipelineEvidenceValid_ = true;
        scene.pipelineEvidenceIdentity_ = {};
        scene.pipelineEvidenceIdentity_.executionMode =
            horde::telemetry::RtExecutionMode::RayQueryCompute;
        scene.pipelineEvidenceIdentity_.instrumentation =
            horde::telemetry::RtInstrumentationMode::Diagnostic;
        scene.pipelineEvidenceIdentity_.activeStrategy =
            horde::telemetry::RtMaterialStrategy::GenericDielectric;
        (void)horde::telemetry::AssignRtFixedText(
            scene.pipelineEvidenceIdentity_.bundleKey, "resize-test-bundle");
        scene.pipelineEvidenceIdentity_.opaqueFast.key = {};
        (void)horde::telemetry::AssignRtFixedText(
            scene.pipelineEvidenceIdentity_.opaqueFast.key, "opaque-test");
        (void)horde::telemetry::AssignRtFixedText(
            scene.pipelineEvidenceIdentity_.opaqueFast.sha256, "opaque-sha");
        scene.pipelineEvidenceIdentity_.genericDielectric.key = {};
        (void)horde::telemetry::AssignRtFixedText(
            scene.pipelineEvidenceIdentity_.genericDielectric.key, "generic-test");
        (void)horde::telemetry::AssignRtFixedText(
            scene.pipelineEvidenceIdentity_.genericDielectric.sha256, "generic-sha");
        scene.pipelineEvidenceIdentity_.active =
            scene.pipelineEvidenceIdentity_.genericDielectric;
        scene.framePipelineEvidence_ = scene.pipelineEvidenceIdentity_;
    }

    static bool SameIdentity(
        const horde::telemetry::RtPipelineEvidenceIdentity& left,
        const horde::telemetry::RtPipelineEvidenceIdentity& right)
    {
        return left.executionMode == right.executionMode &&
               left.instrumentation == right.instrumentation &&
               left.dielectricQuality == right.dielectricQuality &&
               left.activeStrategy == right.activeStrategy &&
               left.waterQuality == right.waterQuality &&
               left.bundleKey.value == right.bundleKey.value &&
               left.opaqueFast.key.value == right.opaqueFast.key.value &&
               left.opaqueFast.sha256.value == right.opaqueFast.sha256.value &&
               left.genericDielectric.key.value == right.genericDielectric.key.value &&
               left.genericDielectric.sha256.value == right.genericDielectric.sha256.value &&
               left.active.key.value == right.active.key.value &&
               left.active.sha256.value == right.active.sha256.value;
    }

    static bool SameOutput(const Output& left, const Output& right)
    {
        return left.image == right.image && left.memory == right.memory &&
               left.view == right.view && left.allocationSize == right.allocationSize &&
               left.memoryFlags == right.memoryFlags && left.layout == right.layout;
    }

    static std::vector<std::uint64_t> OwnershipSnapshot(const Scene& scene)
    {
        std::vector<std::uint64_t> values;
        const auto addBuffer = [&values](const RtGpuBuffer& buffer) {
            values.push_back(HandleBits(buffer.buffer));
            values.push_back(HandleBits(buffer.memory));
            values.push_back(buffer.address);
            values.push_back(buffer.size);
            values.push_back(buffer.allocationSize);
            values.push_back(buffer.memoryPropertyFlags);
        };
        const auto addAs = [&values, &addBuffer](const RtAccelerationStructure& as) {
            addBuffer(as.backing);
            values.push_back(HandleBits(as.handle));
            values.push_back(as.address);
        };

        for (const RtGpuBuffer* buffer : std::array{
                 &scene.vertexBuffer_, &scene.indexBuffer_, &scene.transformBuffer_,
                 &scene.instanceBuffer_, &scene.heldLightBuffer_, &scene.fireEmitterBuffer_, &scene.qualityControlsBuffer_,
                 &scene.worldSurfaceBuffer_, &scene.staticVertexBuffer_,
                 &scene.worldPlayerVertexBuffer_, &scene.viewmodelVertexBuffer_,
                 &scene.staticIndexBuffer_, &scene.staticGeometryTransformBuffer_,
                 &scene.instanceMetadataBuffer_, &scene.primitiveMetadataBuffer_,
                 &scene.materialMetadataBuffer_, &scene.skinnedPlayerBlasUpdateScratch_,
                 &scene.viewmodelBlasUpdateScratch_, &scene.tlasUpdateScratch_,
                 &scene.pipelineBundle_.diagnosticBuffer})
        {
            addBuffer(*buffer);
        }
        for (const RtAccelerationStructure* as : std::array{
                 &scene.blas_, &scene.waterfallBlas_, &scene.finaleRoofBlas_,
                 &scene.torchBlas_, &scene.swordBlas_, &scene.gothicChestBaseBlas_,
                 &scene.gothicChestLidBlas_, &scene.rewardLanternRingBlas_,
                 &scene.rewardLanternBodyBlas_, &scene.dielectricFixtureBlas_,
                 &scene.playerBodyBlas_, &scene.playerLimbBlas_,
                 &scene.skinnedPlayerBlas_, &scene.viewmodelBlas_, &scene.collapseBlas_, &scene.tlas_})
        {
            addAs(*as);
        }
        for (std::size_t index = 0u;
             index < CharacterRenderSlot::kMaximumSkeletonPoseBuckets; ++index)
        {
            const auto& skeleton = scene.characterSlot_.SkeletonGpu(index);
            addBuffer(skeleton.vertices);
            addAs(skeleton.accelerationStructure);
            addBuffer(skeleton.updateScratch);
        }
        const auto& lich = scene.characterSlot_.LichGpu();
        addBuffer(lich.vertices);
        addAs(lich.accelerationStructure);
        addBuffer(lich.updateScratch);

        values.push_back(HandleBits(scene.pipelineBundle_.descriptorSetLayout));
        values.push_back(HandleBits(scene.pipelineBundle_.descriptorPool));
        values.push_back(HandleBits(scene.pipelineBundle_.descriptorSet));
        values.push_back(HandleBits(scene.pipelineBundle_.pipelineLayout));
        values.push_back(HandleBits(scene.materialSampler_));
        for (const RtMaterialStrategy strategy : {
                 RtMaterialStrategy::OpaqueFast, RtMaterialStrategy::GenericDielectric})
        {
            const auto& pipeline = scene.pipelineBundle_.Strategy(strategy);
            values.push_back(HandleBits(pipeline.pipeline));
            addBuffer(pipeline.shaderBindingTable);
        }
        for (const Scene::TextureArray* texture : std::array<const Scene::TextureArray*, 9u>{
                 &scene.materialDiffuse_, &scene.materialNormal_, &scene.materialArm_,
                 &scene.lichBaseColor_, &scene.lichEmissive_, &scene.staticBaseColor_,
                 &scene.staticNormal_, &scene.staticOrm_, &scene.staticEmissive_})
        {
            values.push_back(HandleBits(texture->image));
            values.push_back(HandleBits(texture->memory));
            values.push_back(HandleBits(texture->view));
            values.push_back(texture->allocationSize);
            values.push_back(texture->memoryPropertyFlags);
        }
        return values;
    }

    static bool Create(void* user, VkExtent2D extent, Output& output,
                       std::string& diagnostic)
    {
        auto& state = *static_cast<CallbackState*>(user);
        ++state.createCount;
        state.requestedExtent = extent;
        RecordEvent(state, 'C');
        if (state.progress >= CreateProgress::Image)
            output.image = FakeHandle<VkImage>(0xA001u);
        if (state.progress >= CreateProgress::Memory)
            output.memory = FakeHandle<VkDeviceMemory>(0xA002u);
        if (state.progress >= CreateProgress::View)
            output.view = FakeHandle<VkImageView>(0xA003u);
        if (state.progress == CreateProgress::Complete)
        {
            output.allocationSize = 4096u;
            output.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            output.layout = VK_IMAGE_LAYOUT_GENERAL;
            state.createdOutput = output;
            diagnostic.clear();
            return true;
        }
        diagnostic = "synthetic partial output allocation failure";
        return false;
    }

    static void WriteDescriptor(void* user, VkImageView view) noexcept
    {
        auto& state = *static_cast<CallbackState*>(user);
        ++state.writeCount;
        RecordEvent(state, 'W');
        state.writtenView = view;
        state.descriptorSawOldState =
            SameOutput(state.scene->OutputImage(), state.oldOutput) &&
            state.scene->dispatchExtent_.width == 640u &&
            state.scene->dispatchExtent_.height == 360u;
    }

    static void Destroy(void* user, Output& output) noexcept
    {
        auto& state = *static_cast<CallbackState*>(user);
        ++state.destroyCount;
        RecordEvent(state, 'D');
        state.destroyedOutput = output;
        if (SameOutput(output, state.oldOutput))
        {
            const horde::telemetry::RtPipelineEvidenceIdentity defaultIdentity{};
            state.destroySawPublishedState =
                SameOutput(state.scene->OutputImage(), state.createdOutput) &&
                state.scene->dispatchExtent_.width == 1280u &&
                state.scene->dispatchExtent_.height == 720u &&
                state.scene->computeDispatchGroups_[0] == 160u &&
                state.scene->computeDispatchGroups_[1] == 90u &&
                state.scene->computeDispatchGroups_[2] == 1u &&
                !state.scene->framePipelineEvidenceValid_ &&
                SameIdentity(state.scene->framePipelineEvidence_, defaultIdentity);
        }
        output = {};
    }

    static bool Resize(Scene& scene, CallbackState& state, VkExtent2D extent,
                       std::array<std::uint32_t, 3u> groups,
                       const bool includeDestroy = true,
                       const bool includeWrite = true)
    {
        state.scene = &scene;
        state.oldOutput = scene.OutputImage();
        Scene::OutputResizeApi api{};
        api.user = &state;
        api.create = Create;
        api.writeDescriptor = includeWrite ? WriteDescriptor : nullptr;
        api.destroy = includeDestroy ? Destroy : nullptr;
        return scene.ResizeOutputWithApi(extent, groups, api, state.diagnostic);
    }

    static bool FailurePreservesScene(const CreateProgress progress)
    {
        Scene scene;
        Configure(scene);
        const auto oldOutput = scene.OutputImage();
        const auto oldExtent = scene.dispatchExtent_;
        const auto oldGroups = scene.computeDispatchGroups_;
        const auto oldPipelineIdentity = scene.pipelineEvidenceIdentity_;
        const auto oldFrameIdentity = scene.framePipelineEvidence_;
        const auto oldOwners = OwnershipSnapshot(scene);
        CallbackState state{};
        state.progress = progress;
        const bool resized = Resize(scene, state, {1280u, 720u}, {160u, 90u, 1u});
        const auto partial = state.destroyedOutput;
        const bool hasExpectedPartialOwnership =
            (progress >= CreateProgress::Image) == (partial.image != VK_NULL_HANDLE) &&
            (progress >= CreateProgress::Memory) == (partial.memory != VK_NULL_HANDLE) &&
            (progress >= CreateProgress::View) == (partial.view != VK_NULL_HANDLE);
        const bool preserved =
            !resized && state.createCount == 1u && state.writeCount == 0u &&
            state.destroyCount == 1u && state.eventCount == 2u && !state.eventOverflow &&
            state.events[0] == 'C' && state.events[1] == 'D' &&
            hasExpectedPartialOwnership &&
            state.diagnostic == "synthetic partial output allocation failure" &&
            SameOutput(scene.OutputImage(), oldOutput) &&
            scene.dispatchExtent_.width == oldExtent.width &&
            scene.dispatchExtent_.height == oldExtent.height &&
            scene.computeDispatchGroups_ == oldGroups && scene.framePipelineEvidenceValid_ &&
            scene.storageImageFrameRecorded_ && scene.lastOutputRedBlueSwapApplied_ &&
            SameIdentity(scene.pipelineEvidenceIdentity_, oldPipelineIdentity) &&
            SameIdentity(scene.framePipelineEvidence_, oldFrameIdentity) &&
            OwnershipSnapshot(scene) == oldOwners;
        scene.device_ = VK_NULL_HANDLE;
        return Require(preserved,
                      "partial output allocation failure must destroy only the partial candidate and preserve the published scene");
    }

    static bool RunTests()
    {
        bool ok = true;
        ok &= FailurePreservesScene(CreateProgress::Image);
        ok &= FailurePreservesScene(CreateProgress::Memory);
        ok &= FailurePreservesScene(CreateProgress::View);

        {
            Scene unready;
            Configure(unready);
            unready.ready_ = false;
            CallbackState state{};
            const bool resized = Resize(unready, state, {1280u, 720u}, {160u, 90u, 1u});
            const bool rejected = !resized && state.createCount == 0u &&
                                  state.writeCount == 0u && state.destroyCount == 0u;
            unready.device_ = VK_NULL_HANDLE;
            ok &= Require(rejected,
                          "an unready scene must reject resize before invoking allocation callbacks");
        }
        {
            Scene zeroExtent;
            Configure(zeroExtent);
            CallbackState state{};
            const bool resized = Resize(zeroExtent, state, {0u, 720u}, {160u, 90u, 1u});
            const bool rejected = !resized && state.createCount == 0u &&
                                  state.writeCount == 0u && state.destroyCount == 0u;
            zeroExtent.device_ = VK_NULL_HANDLE;
            ok &= Require(rejected,
                          "a zero extent must reject resize before invoking allocation callbacks");
        }
        {
            Scene invalidApi;
            Configure(invalidApi);
            CallbackState state{};
            const bool resized = Resize(
                invalidApi, state, {1280u, 720u}, {160u, 90u, 1u}, true, false);
            const bool rejected = !resized && state.createCount == 0u &&
                                  state.writeCount == 0u && state.destroyCount == 0u;
            invalidApi.device_ = VK_NULL_HANDLE;
            ok &= Require(rejected,
                          "an incomplete transaction API must reject before invoking allocation callbacks");
        }
        {
            Scene noOp;
            Configure(noOp);
            const auto oldOutput = noOp.OutputImage();
            const auto oldGroups = noOp.computeDispatchGroups_;
            const auto oldFrameIdentity = noOp.framePipelineEvidence_;
            CallbackState state{};
            const bool resized = Resize(noOp, state, {640u, 360u}, {80u, 45u, 1u});
            const bool unchanged = resized && state.createCount == 0u &&
                                   state.writeCount == 0u && state.destroyCount == 0u &&
                                   state.diagnostic.empty() &&
                                   SameOutput(noOp.OutputImage(), oldOutput) &&
                                   noOp.dispatchExtent_.width == 640u &&
                                   noOp.dispatchExtent_.height == 360u &&
                                   noOp.computeDispatchGroups_ == oldGroups &&
                                   noOp.framePipelineEvidenceValid_ &&
                                   noOp.storageImageFrameRecorded_ && noOp.lastOutputRedBlueSwapApplied_ &&
                                   SameIdentity(noOp.framePipelineEvidence_, oldFrameIdentity);
            noOp.device_ = VK_NULL_HANDLE;
            ok &= Require(unchanged,
                          "an equal extent must be a true no-op, preserving descriptors, groups, and frame evidence");
        }
        {
            Scene scene;
            Configure(scene);
            const auto oldOutput = scene.OutputImage();
            const auto oldPipelineIdentity = scene.pipelineEvidenceIdentity_;
            const auto oldOwners = OwnershipSnapshot(scene);
            CallbackState state{};
            const bool resized = Resize(scene, state, {1280u, 720u}, {160u, 90u, 1u});
            const horde::telemetry::RtPipelineEvidenceIdentity defaultFrameIdentity{};
            const bool committed = resized && state.createCount == 1u &&
                                   state.writeCount == 1u && state.destroyCount == 1u &&
                                   state.eventCount == 3u && state.events[0] == 'C' &&
                                   !state.eventOverflow && state.events[1] == 'W' &&
                                   state.events[2] == 'D' &&
                                   state.requestedExtent.width == 1280u &&
                                   state.requestedExtent.height == 720u &&
                                   state.writtenView == state.createdOutput.view &&
                                   state.descriptorSawOldState && state.destroySawPublishedState &&
                                   SameOutput(state.destroyedOutput, oldOutput) &&
                                   !SameOutput(scene.OutputImage(), oldOutput) &&
                                   SameOutput(scene.OutputImage(), state.createdOutput) &&
                                   scene.dispatchExtent_.width == 1280u &&
                                   scene.dispatchExtent_.height == 720u &&
                                   scene.computeDispatchGroups_ ==
                                       std::array<std::uint32_t, 3u>{160u, 90u, 1u} &&
                                   !scene.framePipelineEvidenceValid_ &&
                                   !scene.storageImageFrameRecorded_ && !scene.lastOutputRedBlueSwapApplied_ &&
                                   SameIdentity(scene.framePipelineEvidence_, defaultFrameIdentity) &&
                                   scene.pipelineEvidenceIdentityValid_ &&
                                   SameIdentity(scene.pipelineEvidenceIdentity_, oldPipelineIdentity) &&
                                   OwnershipSnapshot(scene) == oldOwners;
            Scene::StorageImageCapture unwrittenCapture;
            unwrittenCapture.width = 99u;
            unwrittenCapture.rgba = {1u, 2u, 3u, 4u};
            std::string captureDiagnostic;
            ok &= Require(!scene.CaptureStorageImage(unwrittenCapture, captureDiagnostic) &&
                          unwrittenCapture.width == 0u && unwrittenCapture.rgba.empty() &&
                          captureDiagnostic == "RT storage image is not ready for capture.",
                          "a resized but unwritten output must reject capture before Vulkan readback");
            scene.device_ = VK_NULL_HANDLE;
            ok &= Require(committed,
                          "successful resize must update only output ownership, refresh groups, invalidate old frame evidence, then retire the old image");
        }
        return ok;
    }
};

} // namespace horde::vulkan::raytracing

int main()
{
    using namespace horde::vulkan::raytracing;

    bool ok = PresentableTinyRtSceneObservationTestAccess::CheckPersistentReadback();
    ok &= Require(PresentableTinyRtSceneObservationTestAccess::CheckMistControlsAndLifetime(),
                  "mist control must preserve default-On/shadow policy and distinguish requested from owned uploaded state");
    ok &= Require(PresentableTinyRtSceneObservationTestAccess::CheckKeeperTorchBodyAliases(),
                  "two physical world torch slots must share dark body geometry and valid static metadata aliases");
    for (const auto instrumentation : {RtInstrumentation::Diagnostic, RtInstrumentation::Shipping})
    {
        const auto contract = TryMakeRtDescriptorIoContract(instrumentation);
        for (const auto backend : {horde::vulkan::RtExecutionBackend::RayTracingPipeline,
                                  horde::vulkan::RtExecutionBackend::RayQueryCompute})
        {
            const auto policy = TryMakeRtExecutionPolicy(backend);
            const auto layout = TryMakeRtDescriptorSetLayoutBindings(*contract,
                policy->pushConstantStages, policy->shaderStage);
            const auto count = instrumentation == RtInstrumentation::Diagnostic ? 27u : 26u;
            ok &= Require(layout && layout->count == count,
                          "both real backend layouts must fit the full Diagnostic/Shipping rosters");
            if (!layout) continue;
            ok &= Require(layout->values[count - 1u].binding == 26u &&
                          layout->values[count - 1u].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                          "the final quality-control descriptor must survive layout construction");
            for (std::uint32_t i = 0u; i < count; ++i)
                ok &= Require(layout->values[i].binding == contract->bindings[i].binding &&
                              layout->values[i].descriptorCount == 1u &&
                              layout->values[i].stageFlags == (i == 0u ? policy->pushConstantStages : policy->shaderStage),
                              "actual layout roster and backend stage ownership must be preserved");
        }
        auto malformed = *contract;
        malformed.bindingCount = static_cast<std::uint32_t>(malformed.bindings.size() + 1u);
        ok &= Require(!TryMakeRtDescriptorSetLayoutBindings(malformed, VK_SHADER_STAGE_COMPUTE_BIT, VK_SHADER_STAGE_COMPUTE_BIT),
                      "oversized descriptor count must fail before any array access");
        malformed = *contract;
        malformed.bindings[0].kind = static_cast<RtDescriptorResourceKind>(999u);
        ok &= Require(!TryMakeRtDescriptorSetLayoutBindings(malformed, VK_SHADER_STAGE_COMPUTE_BIT, VK_SHADER_STAGE_COMPUTE_BIT),
                      "unknown resource kind must not silently become a storage descriptor");
    }
    ok &= CheckTlasInstanceRefresh();
    ok &= PresentableTinyRtSceneOutputResizeTestAccess::RunTests();
    const auto pipelinePolicy = TryMakeRtExecutionPolicy(
        horde::vulkan::RtExecutionBackend::RayTracingPipeline);
    const auto computePolicy = TryMakeRtExecutionPolicy(
        horde::vulkan::RtExecutionBackend::RayQueryCompute);
    ok &= Require(pipelinePolicy &&
                      pipelinePolicy->shaderPipelineStage == VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR &&
                      pipelinePolicy->shaderStage == VK_SHADER_STAGE_RAYGEN_BIT_KHR &&
                      pipelinePolicy->pushConstantStages ==
                          (VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) &&
                      pipelinePolicy->bindPoint == VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR &&
                      pipelinePolicy->requiresShaderBindingTable,
                  "pipeline execution must retain its exact stage/bind/push/SBT contract");
    ok &= Require(computePolicy &&
                      computePolicy->shaderPipelineStage == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT &&
                      computePolicy->shaderStage == VK_SHADER_STAGE_COMPUTE_BIT &&
                      computePolicy->pushConstantStages == VK_SHADER_STAGE_COMPUTE_BIT &&
                      computePolicy->bindPoint == VK_PIPELINE_BIND_POINT_COMPUTE &&
                      !computePolicy->requiresShaderBindingTable &&
                      !TryMakeRtExecutionPolicy(horde::vulkan::RtExecutionBackend::Unsupported) &&
                      !TryMakeRtExecutionPolicy(static_cast<horde::vulkan::RtExecutionBackend>(99)),
                  "compute must require only compute stages and unsupported must not become raster");
    VkPhysicalDeviceLimits dispatchLimits{};
    dispatchLimits.maxComputeWorkGroupInvocations = 64u;
    dispatchLimits.maxComputeWorkGroupSize[0] = 8u;
    dispatchLimits.maxComputeWorkGroupSize[1] = 8u;
    dispatchLimits.maxComputeWorkGroupSize[2] = 1u;
    dispatchLimits.maxComputeWorkGroupCount[0] = 65535u;
    dispatchLimits.maxComputeWorkGroupCount[1] = 65535u;
    dispatchLimits.maxComputeWorkGroupCount[2] = 1u;
    const auto oddDispatch = TryMakeRtComputeDispatch({961u, 541u}, dispatchLimits);
    const auto exactDispatch = TryMakeRtComputeDispatch({960u, 540u}, dispatchLimits);
    ok &= Require(oddDispatch && (*oddDispatch == std::array<std::uint32_t, 3u>{121u, 68u, 1u}) &&
                      exactDispatch && (*exactDispatch == std::array<std::uint32_t, 3u>{120u, 68u, 1u}) &&
                      !TryMakeRtComputeDispatch({0u, 540u}, dispatchLimits) &&
                      !TryMakeRtComputeDispatch({UINT32_MAX, 540u}, dispatchLimits),
                  "compute dispatch must round up 8x8 safely and reject zero/unsupported group extents");
    dispatchLimits.maxComputeWorkGroupInvocations = 63u;
    ok &= Require(!TryMakeRtComputeDispatch({960u, 540u}, dispatchLimits),
                  "compute dispatch must reject a device below the fixed workgroup requirement");
    RtPipelineBundlePreflight selectedPreflight{};
    std::string preflightFailure;
    ok &= Require(ResolveCompiledRtPipelineBundlePreflight(
                      selectedPreflight, preflightFailure),
                  "compiled pair must resolve for immutable evidence identity fixture");
    horde::telemetry::RtPipelineEvidenceIdentity fixedPairIdentity{};
    ok &= Require(TryMakeRtPipelineEvidenceIdentity(
                      selectedPreflight.request,
                      selectedPreflight.strategies[0],
                      selectedPreflight.strategies[1],
                      fixedPairIdentity) &&
                      !horde::telemetry::RtFixedTextView(
                           fixedPairIdentity.bundleKey).empty() &&
                      horde::telemetry::RtFixedTextView(
                           fixedPairIdentity.opaqueFast.key) ==
                          selectedPreflight.strategies[0].canonicalKey &&
                      horde::telemetry::RtFixedTextView(
                           fixedPairIdentity.genericDielectric.key) ==
                          selectedPreflight.strategies[1].canonicalKey,
                  "fixed pair identity must retain both exact selected artifacts without truncation");
    const std::string oversizedKey(96u, 'x');
    RtPipelineBundlePreflight computePreflight{};
    horde::telemetry::RtPipelineEvidenceIdentity computeIdentity{};
    ok &= Require(ResolveCompiledRtPipelineBundlePreflight(
                      computePreflight, preflightFailure,
                      horde::vulkan::RtExecutionBackend::RayQueryCompute) &&
                      TryMakeRtPipelineEvidenceIdentity(
                          computePreflight.request, computePreflight.strategies[0],
                          computePreflight.strategies[1], computeIdentity) &&
                      computeIdentity.executionMode == horde::telemetry::RtExecutionMode::RayQueryCompute &&
                      horde::telemetry::RtFixedTextView(computeIdentity.bundleKey).starts_with("rayquery_compute_") &&
                      horde::telemetry::RtFixedTextView(computeIdentity.opaqueFast.key).starts_with("rayquery_compute_"),
                  "scene evidence must identify the actual compute backend and exact selected pair");
    horde::telemetry::RtPipelineEvidenceIdentity crossBackendIdentity{};
    ok &= Require(!TryMakeRtPipelineEvidenceIdentity(
                      computePreflight.request, selectedPreflight.strategies[0],
                      selectedPreflight.strategies[1], crossBackendIdentity),
                  "scene evidence must reject an artifact pair from the other backend");
    RtPipelineVariantArtifact oversizedArtifact = selectedPreflight.strategies[0];
    oversizedArtifact.canonicalKey = oversizedKey;
    horde::telemetry::RtPipelineEvidenceIdentity rejectedIdentity{};
    ok &= Require(!TryMakeRtPipelineEvidenceIdentity(
                      selectedPreflight.request,
                      oversizedArtifact,
                      selectedPreflight.strategies[1],
                      rejectedIdentity) &&
                      horde::telemetry::RtFixedTextView(
                          rejectedIdentity.bundleKey).empty(),
                  "identity construction must reject rather than truncate an oversized key");
    const auto executeFixedCommands = [](
        RtSceneRecordObservation& observation,
        RtCommandExecutionLog& execution,
        const std::array<bool, 4u>& dynamicBlasWork) {
        ExecuteObservedRtSceneCommand(
            &observation, RtSceneCommandEvent::HostWriteBarrier,
            [&execution]() { execution.Push(ExecutedRtCommand::HostWriteBarrier); });
        std::uint64_t completedBlasCommands = 0u;
        const std::uint64_t blasCount = ExecuteObservedDynamicBlasCommands(
            &observation, dynamicBlasWork,
            [&observation, &execution, &completedBlasCommands](const std::size_t index) {
                static constexpr std::array<ExecutedRtCommand, 4u> commands{{
                    ExecutedRtCommand::PlayerBlas,
                    ExecutedRtCommand::SkeletonBlas0,
                    ExecutedRtCommand::SkeletonBlas1,
                    ExecutedRtCommand::LichBlas,
                }};
                execution.observationsFollowCommands =
                    execution.observationsFollowCommands &&
                    observation.commands->BlasUpdateCount() == completedBlasCommands;
                execution.Push(commands[index]);
                ++completedBlasCommands;
            },
            [&execution]() {
                execution.Push(ExecutedRtCommand::BlasToTlasBarrier);
            });
        ExecuteObservedTlasUpdateCommands(
            &observation,
            [&observation, &execution]() {
                execution.observationsFollowCommands =
                    execution.observationsFollowCommands &&
                    observation.commands->TlasUpdateCount() == 0u;
                execution.Push(ExecutedRtCommand::TlasUpdate);
            },
            [&observation, &execution]() {
                execution.observationsFollowCommands =
                    execution.observationsFollowCommands &&
                    observation.commands->TlasUpdateCount() == 1u;
                execution.Push(ExecutedRtCommand::TlasToTraceBarrier);
            });
        ExecuteObservedTraceCopyCommands(
            &observation,
            [&observation, &execution]() {
                execution.observationsFollowCommands =
                    execution.observationsFollowCommands &&
                    observation.commands->TraceCount() == 0u;
                execution.Push(ExecutedRtCommand::Trace);
            },
            [&observation, &execution]() {
                execution.observationsFollowCommands =
                    execution.observationsFollowCommands &&
                    observation.commands->TraceCount() == 1u &&
                    observation.commands->CopyCount() == 0u;
                execution.Push(ExecutedRtCommand::CopyOrBlit);
            });
        return blasCount;
    };

    RtSceneCommandObservation zeroBlasCommands{};
    RtSceneRecordObservation zeroBlasObservation{};
    zeroBlasObservation.commands = &zeroBlasCommands;
    RtCommandExecutionLog zeroBlasExecution{};
    const std::uint64_t zeroBlasCount = executeFixedCommands(
        zeroBlasObservation, zeroBlasExecution, {false, false, false, false});
    constexpr std::array<ExecutedRtCommand, 5u> expectedZeroBlas{{
        ExecutedRtCommand::HostWriteBarrier,
        ExecutedRtCommand::TlasUpdate,
        ExecutedRtCommand::TlasToTraceBarrier,
        ExecutedRtCommand::Trace,
        ExecutedRtCommand::CopyOrBlit,
    }};
    ok &= Require(zeroBlasObservation.healthy && zeroBlasCommands.ValidCompleted() &&
                      zeroBlasCount == 0u && zeroBlasCommands.BlasUpdateCount() == 0u &&
                      zeroBlasExecution.observationsFollowCommands &&
                      zeroBlasExecution.count == expectedZeroBlas.size() &&
                      std::equal(expectedZeroBlas.begin(), expectedZeroBlas.end(),
                                 zeroBlasExecution.commands.begin()),
                  "production command routing must execute no BLAS work/barrier for a zero-work frame");

    RtSceneCommandObservation fourBlasCommands{};
    RtSceneRecordObservation fourBlasObservation{};
    fourBlasObservation.commands = &fourBlasCommands;
    RtCommandExecutionLog fourBlasExecution{};
    const std::uint64_t fourBlasCount = executeFixedCommands(
        fourBlasObservation, fourBlasExecution, {true, true, true, true});
    constexpr std::array<ExecutedRtCommand, 10u> expectedFourBlas{{
        ExecutedRtCommand::HostWriteBarrier,
        ExecutedRtCommand::PlayerBlas,
        ExecutedRtCommand::SkeletonBlas0,
        ExecutedRtCommand::SkeletonBlas1,
        ExecutedRtCommand::LichBlas,
        ExecutedRtCommand::BlasToTlasBarrier,
        ExecutedRtCommand::TlasUpdate,
        ExecutedRtCommand::TlasToTraceBarrier,
        ExecutedRtCommand::Trace,
        ExecutedRtCommand::CopyOrBlit,
    }};
    ok &= Require(fourBlasObservation.healthy && fourBlasCommands.ValidCompleted() &&
                      fourBlasCount == 4u && fourBlasCommands.BlasUpdateCount() == 4u &&
                      fourBlasCommands.TlasUpdateCount() == 1u &&
                      fourBlasCommands.TraceCount() == 1u &&
                      fourBlasCommands.CopyCount() == 1u &&
                      fourBlasExecution.observationsFollowCommands &&
                      fourBlasExecution.count == expectedFourBlas.size() &&
                      fourBlasExecution.commands == expectedFourBlas,
                  "production command routing must execute and observe each BLAS, one dependency barrier, TLAS, trace, and copy in order");

    const std::string sceneSource = ReadCompactSource(
        std::filesystem::path(HORDE_RT_SOURCE_DIR) /
        "src/vulkan/raytracing/PresentableTinyRtScene.cpp");
    constexpr std::string_view diagnosticResetPrefix =
        "!WriteBuffer(pipelineBundle_.diagnosticBuffer,&clearedDielectricDiagnostics,"
        "sizeof(clearedDielectricDiagnostics),\"dielectricdiagnosticsreset\",diagnostic";
    ok &= Require(
        sceneSource.find(std::string(diagnosticResetPrefix) + ",observation") ==
            std::string::npos &&
            sceneSource.find(std::string(diagnosticResetPrefix) + "))") !=
                std::string::npos,
        "Diagnostic reset must not change the observed DynamicUpload set");
    const std::size_t updateStart = sceneSource.find(
        "boolPresentableTinyRtScene::UpdateDynamicInstances(");
    const std::size_t recordStart = sceneSource.find(
        "boolPresentableTinyRtScene::RecordTraceAndCopy(");
    ok &= Require(updateStart != std::string::npos &&
                      recordStart > updateStart &&
                      sceneSource.substr(updateStart, recordStart - updateStart).find(
                          "ReadBuffer(pipelineBundle_.diagnosticBuffer") ==
                          std::string::npos,
                  "dynamic recording must not read the prior Diagnostic submission");

    PresentableTinyRtScene notReadyScene;
    PresentableTinyRtSceneObservationTestAccess::MarkTlasDefinitions(notReadyScene);
    PresentableTinyRtSceneObservationTestAccess::MarkPendingTlasDefinitions(notReadyScene);
    ok &= Require(PresentableTinyRtSceneObservationTestAccess::HasTlasDefinitions(notReadyScene),
                  "recorded-but-unsubmitted BUILD must not change the committed definition baseline");
    horde::telemetry::RtStageAccumulator notReadyStages;
    NoWorkClock noWorkClock;
    RtSceneRecordObservation notReadyObservation{
        &notReadyStages, &noWorkClock, ReadNoWorkClock};
    VkImageLayout notReadyLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    std::string notReadyDiagnostic;
    ok &= Require(notReadyStages.Begin(),
                  "not-ready record observation attempt did not begin");
    ok &= Require(
        !notReadyScene.RecordTraceAndCopy(
            VK_NULL_HANDLE, VK_NULL_HANDLE, notReadyLayout, VkExtent2D{},
            RtSceneFrameInputs{}, notReadyDiagnostic, &notReadyObservation) &&
            noWorkClock.reads == 0u,
        "record rejection before renderer work must not sample observation clocks");
    notReadyScene.NotifyFrameSubmitted();
    ok &= Require(PresentableTinyRtSceneObservationTestAccess::HasTlasDefinitions(notReadyScene),
                  "new/rejected recording must discard a prior unsubmitted definition change");
    ok &= Require(notReadyStages.Abort(),
                  "not-ready record observation attempt did not abort");

    PresentableTinyRtScene previewInventoryScene;
    ok &= Require(PresentableTinyRtSceneObservationTestAccess::CheckGlassVisibility(previewInventoryScene, true),
                  "Glass On must preserve fixture, mixed lantern body and procedural arm ray admission");
    ok &= Require(PresentableTinyRtSceneObservationTestAccess::CheckGlassVisibility(previewInventoryScene, false),
                  "Glass Off masks the fixture on all ray bits for either profile and repeated frame requests");
    PresentableTinyRtSceneObservationTestAccess::AdmitPreviewInventoryFixture(previewInventoryScene);
    const auto previewInventory = previewInventoryScene.ResourceInventory();
    ok &= Require(previewInventory.tlasInstanceCount == 7u &&
                      previewInventory.topLevelAccelerationStructureCount == 1u &&
                      previewInventory.bufferCount == 0u && previewInventory.memoryAllocationCount == 1u &&
                      previewInventory.deviceLocalBytes == 768u && previewInventory.hostVisibleBytes == 0u,
                  "preview evidence must use admitted topology and actual environment allocation");
    PresentableTinyRtScene movedPreview(std::move(previewInventoryScene));
    ok &= Require(movedPreview.IsReady() && !movedPreview.GlassEnabled() && previewInventoryScene.GlassEnabled() &&
                      movedPreview.Profile() == RtSceneProfile::GraphicsPreview && movedPreview.TlasInstanceCount() == 7u &&
                      movedPreview.ResourceInventory().tlasInstanceCount == previewInventory.tlasInstanceCount &&
                      movedPreview.ResourceInventory().memoryAllocationCount == previewInventory.memoryAllocationCount &&
                      movedPreview.ResourceInventory().deviceLocalBytes == previewInventory.deviceLocalBytes &&
                      !previewInventoryScene.IsReady() && previewInventoryScene.TlasInstanceCount() == 0u &&
                      previewInventoryScene.ResourceInventory().topLevelAccelerationStructureCount == 0u &&
                      previewInventoryScene.ResourceInventory().memoryAllocationCount == 0u,
                  "preview topology and panorama allocation must move to exactly one owner");
    PresentableTinyRtScene assignedPreview;
    assignedPreview = std::move(movedPreview);
    ok &= Require(assignedPreview.IsReady() && !assignedPreview.GlassEnabled() && movedPreview.GlassEnabled() &&
                      assignedPreview.Profile() == RtSceneProfile::GraphicsPreview &&
                      assignedPreview.TlasInstanceCount() == 7u &&
                      assignedPreview.ResourceInventory().tlasInstanceCount == previewInventory.tlasInstanceCount &&
                      assignedPreview.ResourceInventory().memoryAllocationCount == previewInventory.memoryAllocationCount &&
                      assignedPreview.ResourceInventory().deviceLocalBytes == previewInventory.deviceLocalBytes &&
                      !movedPreview.IsReady() && movedPreview.TlasInstanceCount() == 0u &&
                      movedPreview.ResourceInventory().topLevelAccelerationStructureCount == 0u &&
                      movedPreview.ResourceInventory().memoryAllocationCount == 0u,
                  "preview move assignment must retain one ready topology and panorama allocation owner");

    PresentableTinyRtScene scene;
    PresentableTinyRtSceneObservationTestAccess::Populate(scene);
#ifndef NDEBUG
    const auto originalHandles = scene.CaptureResourceHandles();
    ok &= Require(originalHandles.ready &&
                      originalHandles.bottomLevelAccelerationStructures.size() == 20u &&
                      originalHandles.topLevelAccelerationStructures.size() == 1u &&
                      originalHandles.pipelines.size() == 2u &&
                      originalHandles.shaderBindingTableBuffers.size() == 2u &&
                      originalHandles.descriptorSets.size() == 1u &&
                      originalHandles.textureImages.size() == 9u &&
                      originalHandles.outputImage != 0u &&
                      originalHandles.outputMemory != 0u &&
                      originalHandles.outputView != 0u &&
                      scene.CaptureResourceHandles() == originalHandles,
                  "Debug identity snapshot must include all actual owners and be read-only");
#endif
    RtDiagnosticCounterPayload completedDiagnostic{};
    for (std::size_t index = 0u; index < completedDiagnostic.counters.size(); ++index)
    {
        completedDiagnostic.counters[index] = static_cast<std::uint32_t>(index + 1u);
    }
    scene.PublishCompletedDiagnostic(completedDiagnostic);
    ok &= Require(scene.DielectricTransportOverflowCount() == 1u &&
                      scene.PrimaryPlayerPixelCount() == 39u &&
                      scene.PrimaryRewardBodyPixelCount() == 41u,
                  "legacy getters must project one explicitly published completed record");
    const auto diagnostic = scene.ResourceInventory();
    ok &= Require(diagnostic.bufferCount == 49u &&
                      diagnostic.memoryAllocationCount == 59u &&
                      diagnostic.bottomLevelAccelerationStructureCount == 20u &&
                      scene.BlasCount() == 20u &&
                      diagnostic.topLevelAccelerationStructureCount == 1u &&
                      diagnostic.tlasInstanceCount == 26u &&
                      diagnostic.pipelineCount == 2u &&
                      diagnostic.shaderBindingTableCount == 2u &&
                      diagnostic.descriptorSetCount == 1u,
                  "live inventory must include direct, character, image, and both SBT owners");
    ok &= Require(diagnostic.hostVisibleBytes == 3264u &&
                      diagnostic.deviceLocalBytes == 4416u,
                  "host-visible and device-local bytes must use inclusive allocation classes");

    PresentableTinyRtSceneObservationTestAccess::RemoveDiagnosticBuffer(scene);
    const auto shipping = scene.ResourceInventory();
    ok &= Require(shipping.bufferCount == 48u &&
                      shipping.memoryAllocationCount == 58u &&
                      shipping.hostVisibleBytes == 3200u &&
                      shipping.deviceLocalBytes == 4352u,
                  "inventory must count only a genuinely live Diagnostic buffer");

    PresentableTinyRtSceneObservationTestAccess::MarkTlasDefinitions(scene);
    PresentableTinyRtSceneObservationTestAccess::MarkPendingTlasDefinitions(scene);
    PresentableTinyRtScene moved(std::move(scene));
#ifndef NDEBUG
    const auto movedHandles = moved.CaptureResourceHandles();
    const auto relinquishedHandles = scene.CaptureResourceHandles();
    ok &= Require(movedHandles == originalHandles &&
                      !relinquishedHandles.ready &&
                      relinquishedHandles.bottomLevelAccelerationStructures.empty() &&
                      relinquishedHandles.topLevelAccelerationStructures.empty() &&
                      relinquishedHandles.pipelines.empty() &&
                      relinquishedHandles.shaderBindingTableBuffers.empty() &&
                      relinquishedHandles.descriptorSets.empty() &&
                      relinquishedHandles.textureImages.empty() &&
                      relinquishedHandles.outputImage == 0u &&
                      relinquishedHandles.outputMemory == 0u &&
                      relinquishedHandles.outputView == 0u,
                  "Debug opaque identities must transfer to exactly one owner on move");
#endif
    ok &= Require(!PresentableTinyRtSceneObservationTestAccess::HasTlasDefinitions(scene) &&
                      PresentableTinyRtSceneObservationTestAccess::HasTlasDefinitions(moved),
                  "TLAS definition cache must follow its sole scene owner on move");
    moved.NotifyFrameSubmitted();
    scene.NotifyFrameSubmitted();
    ok &= Require(PresentableTinyRtSceneObservationTestAccess::HasTlasDefinitions(moved, 99u) &&
                      !PresentableTinyRtSceneObservationTestAccess::HasTlasDefinitions(scene, 99u),
                  "only the moved owner may commit its pending BUILD after successful submission");
    moved.NotifyFrameSubmitted();
    ok &= Require(PresentableTinyRtSceneObservationTestAccess::HasTlasDefinitions(moved, 99u),
                  "submission notification with no pending BUILD must be idempotent");
    PresentableTinyRtScene emptyScene;
    PresentableTinyRtSceneObservationTestAccess::MarkTlasDefinitions(emptyScene);
    PresentableTinyRtSceneObservationTestAccess::MarkPendingTlasDefinitions(emptyScene);
    PresentableTinyRtSceneObservationTestAccess::MarkFireUpload(emptyScene);
    emptyScene.Destroy();
    ok &= Require(!emptyScene.HasUploadedFireEmitters() && emptyScene.UploadedFireEmitters().activeCount == 0u,
                  "no-device destruction must invalidate and clear the last raw fire upload");
    emptyScene.NotifyFrameSubmitted();
    ok &= Require(!PresentableTinyRtSceneObservationTestAccess::HasTlasDefinitions(emptyScene),
                  "even partial/no-device destruction must invalidate cached TLAS definitions");
    const auto movedFrom = scene.ResourceInventory();
    const auto movedTo = moved.ResourceInventory();
    ok &= Require(moved.HasUploadedFireEmitters() && !scene.HasUploadedFireEmitters() &&
                      moved.UploadedFireEmitters().activeCount == 2u &&
                      moved.UploadedFireEmitters().selectedStableIds[0] == 3u &&
                      moved.UploadedFireEmitters().selectedStableIds[1] == 4u &&
                      scene.UploadedFireEmitters().activeCount == 0u,
                  "packed raw fire upload must move to exactly one owner without becoming a frame ACK");
    ok &= Require(moved.HasUploadedQualityControls() && !scene.HasUploadedQualityControls() &&
                      moved.QualityControls().controls == std::array<std::uint32_t, 4u>{{2u, 4u, 2u, 0u}} &&
                      moved.UploadedFireQuality() == horde::vulkan::raytracing::FireEmitterQuality::Low,
                  "actual uploaded policy must transfer to exactly one owner with its buffer");
    ok &= Require(scene.BlasCount() == 0u && moved.BlasCount() == 20u &&
                      movedFrom.bufferCount == 0u &&
                      movedFrom.memoryAllocationCount == 0u &&
                      movedFrom.hostVisibleBytes == 0u &&
                      movedFrom.deviceLocalBytes == 0u,
                  "scene move must clear resource handles and allocation facts from the source");
    ok &= Require(movedTo.bufferCount == shipping.bufferCount &&
                      movedTo.memoryAllocationCount == shipping.memoryAllocationCount &&
                      movedTo.hostVisibleBytes == shipping.hostVisibleBytes &&
                      movedTo.deviceLocalBytes == shipping.deviceLocalBytes,
                  "scene move must preserve one live inventory owner");

#ifndef NDEBUG
    const auto captureBase = std::filesystem::absolute(
        std::filesystem::temp_directory_path()).lexically_normal();
    auto normalizedCaptureBase = captureBase;
    if (!normalizedCaptureBase.has_filename())
        normalizedCaptureBase = normalizedCaptureBase.parent_path();
    const auto captureRoot = normalizedCaptureBase /
        ("horde-rt-viewmodel-capture-tests-" +
         std::to_string(reinterpret_cast<std::uintptr_t>(&ok)));
    const bool captureRootSafe = captureRoot.parent_path().lexically_normal() ==
            normalizedCaptureBase &&
        captureRoot.filename().string().starts_with("horde-rt-viewmodel-capture-tests-");
    std::error_code captureError;
    bool captureRootCreated = false;
    if (captureRootSafe)
        captureRootCreated = std::filesystem::create_directory(captureRoot, captureError);
    if (!(captureRootSafe && captureRootCreated && !captureError))
    {
        std::cerr << "FAIL: viewmodel capture test directory creation failed: base='"
                  << normalizedCaptureBase.string() << "' root='" << captureRoot.string()
                  << "' safe=" << captureRootSafe << " created=" << captureRootCreated
                  << " error='" << captureError.message() << "'\n";
        ok = false;
    }
    if (captureRootSafe && captureRootCreated && !captureError)
    {
        const auto notReadyPath = captureRoot / "not-ready.obj";
        std::string captureDiagnostic;
        PresentableTinyRtScene notReady;
        ok &= Require(!notReady.CaptureViewmodelMesh(
                          notReadyPath.string(), captureDiagnostic) &&
                          !std::filesystem::exists(notReadyPath),
                      "viewmodel capture must reject a scene that is not ready");
        const auto worldNotReadyPath = captureRoot / "world-not-ready.obj";
        captureDiagnostic.clear();
        ok &= Require(!notReady.CapturePlayerWorldBodyMesh(
                          worldNotReadyPath.string(), captureDiagnostic) &&
                          !std::filesystem::exists(worldNotReadyPath),
                      "world-body capture must reject a scene that is not ready");

        const auto notCurrentPath = captureRoot / "not-current.obj";
        PresentableTinyRtScene notCurrent;
        PresentableTinyRtSceneObservationTestAccess::ConfigureCaptureFixture(
            notCurrent, true, false);
        captureDiagnostic.clear();
        ok &= Require(!notCurrent.CaptureViewmodelMesh(
                          notCurrentPath.string(), captureDiagnostic) &&
                          !std::filesystem::exists(notCurrentPath),
                      "viewmodel capture must reject a non-current pose");
        const auto worldNotCurrentPath = captureRoot / "world-not-current.obj";
        captureDiagnostic.clear();
        ok &= Require(!notCurrent.CapturePlayerWorldBodyMesh(
                          worldNotCurrentPath.string(), captureDiagnostic) &&
                          !std::filesystem::exists(worldNotCurrentPath),
                      "world-body capture must reject a stale or absent upload");

        PresentableTinyRtScene rejectedRecord;
        PresentableTinyRtSceneObservationTestAccess::ConfigureCaptureFixture(
            rejectedRecord, true, true);
        PresentableTinyRtSceneObservationTestAccess::ForceUnsupportedScaledPresentation(
            rejectedRecord);
        VkImageLayout rejectedLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        captureDiagnostic.clear();
        ok &= Require(!rejectedRecord.RecordTraceAndCopy(
                          VK_NULL_HANDLE, VK_NULL_HANDLE, rejectedLayout,
                          VkExtent2D{2u, 2u}, RtSceneFrameInputs{},
                          captureDiagnostic) &&
                          captureDiagnostic.find("cannot linearly upscale") !=
                              std::string::npos,
                      "unsupported scaled presentation must return before renderer/Vulkan work");
        const auto afterRejectedRecordPath =
            captureRoot / "world-after-rejected-record.obj";
        captureDiagnostic.clear();
        ok &= Require(!rejectedRecord.CapturePlayerWorldBodyMesh(
                          afterRejectedRecordPath.string(), captureDiagnostic) &&
                          !std::filesystem::exists(afterRejectedRecordPath),
                      "early RecordTraceAndCopy failure must invalidate prior world-body capture evidence");

        const auto outputPath = captureRoot / "synthetic.obj";
        PresentableTinyRtScene synthetic;
        PresentableTinyRtSceneObservationTestAccess::ConfigureCaptureFixture(
            synthetic, true, true);
        captureDiagnostic.clear();
        ok &= Require(synthetic.CaptureViewmodelMesh(
                          outputPath.string(), captureDiagnostic) &&
                          captureDiagnostic.empty(),
                      "viewmodel capture must write a current synthetic upload");
        std::ifstream output(outputPath, std::ios::binary);
        const std::string outputText((std::istreambuf_iterator<char>(output)),
                                     std::istreambuf_iterator<char>());
        const std::string expectedText =
            "# Exact CPU viewmodel upload, model-space metres; not GPU readback.\n"
            "# model_to_world_row_major_3x4 -1 0 0 2 0 1 0 -0.75 0 0 -1 3\n"
            "v 1 2 3\nv 4 5 6\nv -1 -2 -3\nv 7 8 9\nv -4 -5 -6\nv 10 11 12\n"
            "vt 0 0\nvt 0.25 0.5\nvt 1 0\nvt 0.125 0.875\nvt 0.75 0.25\nvt 0.625 0.375\n"
            "vn 0 0 1\nvn 0 1 0\nvn 1 0 0\nvn 0 0 -1\nvn 0 -1 0\nvn -1 0 0\n"
            "g ViewmodelSleeves\n"
            "f 1/1/1 2/2/2 3/3/3\n"
            "g ViewmodelGauntlets\n"
            "f 6/6/6 4/4/4 5/5/5\n";
        ok &= Require(outputText == expectedText,
                      "viewmodel capture must preserve exact upload attributes, groups, and indices");

        const auto worldOutputPath = captureRoot / "synthetic-world-body.obj";
        captureDiagnostic.clear();
        ok &= Require(synthetic.CapturePlayerWorldBodyMesh(
                          worldOutputPath.string(), captureDiagnostic) &&
                          captureDiagnostic.empty(),
                      "world-body capture must write the current skinned upload");
        std::ifstream worldOutput(worldOutputPath, std::ios::binary);
        const std::string worldOutputText((std::istreambuf_iterator<char>(worldOutput)),
                                          std::istreambuf_iterator<char>());
        const std::string expectedWorldText =
            "# Exact CPU PlayerWorldBody upload, model-space metres; not GPU readback.\n"
            "# model_to_world_row_major_3x4 1 0 0 -2 0 1 0 0.75 0 0 1 -3\n"
            "v 1 2 3\nv 4 5 6\nv -1 -2 -3\nv 7 8 9\nv -4 -5 -6\nv 10 11 12\n"
            "vt 0 0\nvt 0.25 0.5\nvt 1 0\nvt 0.125 0.875\nvt 0.75 0.25\nvt 0.625 0.375\n"
            "vn 0 0 1\nvn 0 1 0\nvn 1 0 0\nvn 0 0 -1\nvn 0 -1 0\nvn -1 0 0\n"
            "g BodyPrimaryVisible\nf 1/1/1 2/2/2 3/3/3\n"
            "g NearFacePrimaryMasked\nf 6/6/6 4/4/4 5/5/5\n";
        ok &= Require(worldOutputText == expectedWorldText,
                      "world-body capture must preserve uploaded pose, TLAS transform, ranges and indices");

        const auto worldBadIndexPath = captureRoot / "world-bad-index.obj";
        PresentableTinyRtSceneObservationTestAccess::SetFirstWorldBodyIndex(
            synthetic, 3u);
        captureDiagnostic.clear();
        ok &= Require(!synthetic.CapturePlayerWorldBodyMesh(
                          worldBadIndexPath.string(), captureDiagnostic) &&
                          !std::filesystem::exists(worldBadIndexPath),
                      "world-body capture must reject indices outside the primitive vertex range");
        PresentableTinyRtSceneObservationTestAccess::SetFirstWorldBodyIndex(
            synthetic, 0u);
        const auto worldEmptyRosterPath = captureRoot / "world-empty-roster.obj";
        PresentableTinyRtSceneObservationTestAccess::SetWorldBodyPrimitives(
            synthetic, false);
        captureDiagnostic.clear();
        ok &= Require(!synthetic.CapturePlayerWorldBodyMesh(
                          worldEmptyRosterPath.string(), captureDiagnostic) &&
                          !std::filesystem::exists(worldEmptyRosterPath),
                      "world-body capture must reject an empty primitive roster");
        PresentableTinyRtSceneObservationTestAccess::SetWorldBodyPrimitives(
            synthetic, true);

        const std::string sentinel = "do-not-overwrite\n";
        {
            std::ofstream existing(outputPath, std::ios::binary | std::ios::trunc);
            existing << sentinel;
        }
        captureDiagnostic.clear();
        ok &= Require(!synthetic.CaptureViewmodelMesh(
                          outputPath.string(), captureDiagnostic),
                      "viewmodel capture must reject an existing output path");
        std::ifstream preserved(outputPath, std::ios::binary);
        const std::string preservedText((std::istreambuf_iterator<char>(preserved)),
                                         std::istreambuf_iterator<char>());
        ok &= Require(preservedText == sentinel,
                      "existing viewmodel capture output must remain unchanged");

        const std::string worldSentinel = "do-not-overwrite-world\n";
        {
            std::ofstream existing(worldOutputPath, std::ios::binary | std::ios::trunc);
            existing << worldSentinel;
        }
        captureDiagnostic.clear();
        ok &= Require(!synthetic.CapturePlayerWorldBodyMesh(
                          worldOutputPath.string(), captureDiagnostic),
                      "world-body capture must reject an existing output path");
        std::ifstream worldPreserved(worldOutputPath, std::ios::binary);
        const std::string preservedWorldText(
            (std::istreambuf_iterator<char>(worldPreserved)),
            std::istreambuf_iterator<char>());
        ok &= Require(preservedWorldText == worldSentinel,
                      "existing world-body capture output must remain unchanged");
    }
    if (captureRootCreated)
        std::filesystem::remove_all(captureRoot, captureError);
#endif

    return ok ? 0 : 1;
}
