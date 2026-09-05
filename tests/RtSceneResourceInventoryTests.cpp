#include "vulkan/raytracing/PresentableTinyRtScene.h"
#include "vulkan/raytracing/RtSceneRecordObservation.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

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

bool Require(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
    }
    return condition;
}

struct NoWorkClock
{
    std::size_t reads = 0u;
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

        for (RtGpuBuffer* buffer : std::array<RtGpuBuffer*, 13u>{
                 &scene.vertexBuffer_, &scene.indexBuffer_, &scene.transformBuffer_,
                 &scene.instanceBuffer_, &scene.heldLightBuffer_, &scene.fireEmitterBuffer_,
                 &scene.worldSurfaceBuffer_, &scene.staticVertexBuffer_,
                 &scene.staticIndexBuffer_, &scene.staticGeometryTransformBuffer_,
                 &scene.instanceMetadataBuffer_, &scene.primitiveMetadataBuffer_,
                 &scene.materialMetadataBuffer_})
        {
            populateBuffer(*buffer);
        }
        for (RtAccelerationStructure* accelerationStructure :
             std::array<RtAccelerationStructure*, 13u>{
                 &scene.blas_, &scene.waterfallBlas_, &scene.finaleRoofBlas_,
                 &scene.torchBlas_, &scene.swordBlas_, &scene.gothicChestBaseBlas_,
                 &scene.gothicChestLidBlas_, &scene.rewardLanternRingBlas_,
                 &scene.rewardLanternBodyBlas_, &scene.dielectricFixtureBlas_,
                 &scene.playerBodyBlas_, &scene.playerLimbBlas_,
                 &scene.skinnedPlayerBlas_})
        {
            populateBlas(*accelerationStructure);
        }
        populateBuffer(scene.skinnedPlayerBlasUpdateScratch_);
        populateBlas(scene.tlas_);
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
            texture->allocationSize = 128u;
            texture->memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }
    }

    static void RemoveDiagnosticBuffer(PresentableTinyRtScene& scene)
    {
        scene.pipelineBundle_.diagnosticBuffer = {};
    }
};

} // namespace horde::vulkan::raytracing

int main()
{
    using namespace horde::vulkan::raytracing;

    bool ok = true;
    const std::string sceneSource = ReadCompactSource(
        std::filesystem::path(HORDE_RT_SOURCE_DIR) /
        "src/vulkan/raytracing/PresentableTinyRtScene.cpp");
    constexpr std::string_view diagnosticResetPrefix =
        "!WriteBuffer(pipelineBundle_.diagnosticBuffer,&clearedDielectricDiagnostics,"
        "sizeof(clearedDielectricDiagnostics),\"dielectricdiagnosticsreset\",diagnostic";
    ok &= Require(
        sceneSource.find(std::string(diagnosticResetPrefix) + ",observation") ==
            std::string::npos &&
            sceneSource.find(std::string(diagnosticResetPrefix) + ")))") !=
                std::string::npos,
        "Diagnostic reset must not change the observed DynamicUpload set");

    PresentableTinyRtScene notReadyScene;
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
    ok &= Require(notReadyStages.Abort(),
                  "not-ready record observation attempt did not abort");

    PresentableTinyRtScene scene;
    PresentableTinyRtSceneObservationTestAccess::Populate(scene);
    const auto diagnostic = scene.ResourceInventory();
    ok &= Require(diagnostic.bufferCount == 41u &&
                      diagnostic.memoryAllocationCount == 51u &&
                      diagnostic.bottomLevelAccelerationStructureCount == 16u &&
                      diagnostic.topLevelAccelerationStructureCount == 1u &&
                      diagnostic.tlasInstanceCount == 20u &&
                      diagnostic.pipelineCount == 2u &&
                      diagnostic.shaderBindingTableCount == 2u &&
                      diagnostic.descriptorSetCount == 1u,
                  "live inventory must include direct, character, image, and both SBT owners");
    ok &= Require(diagnostic.hostVisibleBytes == 2752u &&
                      diagnostic.deviceLocalBytes == 3904u,
                  "host-visible and device-local bytes must use inclusive allocation classes");

    PresentableTinyRtSceneObservationTestAccess::RemoveDiagnosticBuffer(scene);
    const auto shipping = scene.ResourceInventory();
    ok &= Require(shipping.bufferCount == 40u &&
                      shipping.memoryAllocationCount == 50u &&
                      shipping.hostVisibleBytes == 2688u &&
                      shipping.deviceLocalBytes == 3840u,
                  "inventory must count only a genuinely live Diagnostic buffer");

    PresentableTinyRtScene moved(std::move(scene));
    const auto movedFrom = scene.ResourceInventory();
    const auto movedTo = moved.ResourceInventory();
    ok &= Require(movedFrom.bufferCount == 0u &&
                      movedFrom.memoryAllocationCount == 0u &&
                      movedFrom.hostVisibleBytes == 0u &&
                      movedFrom.deviceLocalBytes == 0u,
                  "scene move must clear resource handles and allocation facts from the source");
    ok &= Require(movedTo.bufferCount == shipping.bufferCount &&
                      movedTo.memoryAllocationCount == shipping.memoryAllocationCount &&
                      movedTo.hostVisibleBytes == shipping.hostVisibleBytes &&
                      movedTo.deviceLocalBytes == shipping.deviceLocalBytes,
                  "scene move must preserve one live inventory owner");

    return ok ? 0 : 1;
}
