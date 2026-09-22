#include "vulkan/raytracing/PresentableTinyRtScene.h"
#include "vulkan/raytracing/RtFrameEvidenceCoordinator.h"
#include "vulkan/raytracing/RtSceneRecordObservation.h"
#include "vulkan/raytracing/RtExecutionPolicy.h"

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
                 &scene.instanceBuffer_, &scene.heldLightBuffer_, &scene.fireEmitterBuffer_,
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
                 &scene.torchBlas_, &scene.swordBlas_, &scene.gothicChestBaseBlas_,
                 &scene.gothicChestLidBlas_, &scene.rewardLanternRingBlas_,
                 &scene.rewardLanternBodyBlas_, &scene.dielectricFixtureBlas_,
                 &scene.playerBodyBlas_, &scene.playerLimbBlas_,
                 &scene.skinnedPlayerBlas_, &scene.viewmodelBlas_})
        {
            populateBlas(*accelerationStructure);
        }
        populateBuffer(scene.skinnedPlayerBlasUpdateScratch_);
        populateBuffer(scene.viewmodelBlasUpdateScratch_);
        scene.ready_ = true;
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
    ok &= Require(diagnostic.bufferCount == 45u &&
                      diagnostic.memoryAllocationCount == 55u &&
                      diagnostic.bottomLevelAccelerationStructureCount == 17u &&
                      scene.BlasCount() == 17u &&
                      diagnostic.topLevelAccelerationStructureCount == 1u &&
                      diagnostic.tlasInstanceCount == 21u &&
                      diagnostic.pipelineCount == 2u &&
                      diagnostic.shaderBindingTableCount == 2u &&
                      diagnostic.descriptorSetCount == 1u,
                  "live inventory must include direct, character, image, and both SBT owners");
    ok &= Require(diagnostic.hostVisibleBytes == 3008u &&
                      diagnostic.deviceLocalBytes == 4160u,
                  "host-visible and device-local bytes must use inclusive allocation classes");

    PresentableTinyRtSceneObservationTestAccess::RemoveDiagnosticBuffer(scene);
    const auto shipping = scene.ResourceInventory();
    ok &= Require(shipping.bufferCount == 44u &&
                      shipping.memoryAllocationCount == 54u &&
                      shipping.hostVisibleBytes == 2944u &&
                      shipping.deviceLocalBytes == 4096u,
                  "inventory must count only a genuinely live Diagnostic buffer");

    PresentableTinyRtScene moved(std::move(scene));
    const auto movedFrom = scene.ResourceInventory();
    const auto movedTo = moved.ResourceInventory();
    ok &= Require(scene.BlasCount() == 0u && moved.BlasCount() == 17u &&
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

    return ok ? 0 : 1;
}
