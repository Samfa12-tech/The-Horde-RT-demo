#include "vulkan/raytracing/experimental/StagedPrimaryPass.h"
#include "StagedPrimaryShaders.generated.h"
#if HORDE_RT_STAGED_PRIMARY_TIMING
#include "vulkan/raytracing/experimental/StagedPrimaryTiming.h"
#endif

#include <sstream>
#include <locale>

namespace horde::vulkan::raytracing::experimental {
static_assert(kInstrumentation == HORDE_RT_SELECTED_INSTRUMENTATION);
static_assert(kQuality == HORDE_RT_SELECTED_DIELECTRIC_QUALITY);

StagedPrimaryPass::~StagedPrimaryPass() { Reset(); }

std::unique_ptr<StagedPrimaryPass> StagedPrimaryPass::Create(
    VkPhysicalDevice physicalDevice, VkDevice device, const RtGpuResources& resources,
    VkDescriptorSetLayout sceneLayout, VkDescriptorSet sceneSet, VkExtent2D extent,
    std::uint32_t pushConstantBytes, PFN_vkCmdTraceRaysKHR trace,
    const RtPipelineBundleBuildApi& api, std::string& diagnostic)
{
    auto candidate = std::unique_ptr<StagedPrimaryPass>(new StagedPrimaryPass);
    candidate->device_ = device;
    candidate->resources_ = resources;
    candidate->sceneSet_ = sceneSet;
    candidate->trace_ = trace;
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE ||
        sceneSet == VK_NULL_HANDLE || sceneLayout == VK_NULL_HANDLE || trace == nullptr ||
        api.createSharedShaderModules == nullptr || api.createEntryShaderModule == nullptr ||
        api.createStrategyPipeline == nullptr || api.createStrategySbt == nullptr ||
        api.destroyShaderModule == nullptr) {
        diagnostic = "Invalid staged-primary investigation resource preflight.";
        return nullptr;
    }
    if (!candidate->Initialise(physicalDevice, sceneLayout, extent, pushConstantBytes, api, diagnostic))
        return nullptr; // Partial resource owner cleans up exactly once.
    return candidate;
}

bool StagedPrimaryPass::Initialise(
    VkPhysicalDevice physicalDevice, VkDescriptorSetLayout sceneLayout, VkExtent2D extent,
    std::uint32_t pushConstantBytes, const RtPipelineBundleBuildApi& api, std::string& diagnostic)
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    maxStorageBufferRange_ = properties.limits.maxStorageBufferRange;
    // Existing set0 is already checked by the normal bundle. Add only three
    // storage descriptors, conservatively admitting the complete layout roster.
    const auto sceneContract = TryMakeRtDescriptorIoContract(kInstrumentation == 0
        ? RtInstrumentation::Shipping : RtInstrumentation::Diagnostic);
    const auto sceneStorageBuffers = sceneContract->storageBufferDescriptorCount;
    if (properties.limits.maxBoundDescriptorSets < 2u ||
        properties.limits.maxPerStageDescriptorStorageBuffers < sceneStorageBuffers + kStagedPageCount ||
        properties.limits.maxDescriptorSetStorageBuffers < sceneStorageBuffers + kStagedPageCount ||
        properties.limits.maxPerStageResources < sceneContract->bindingCount + kStagedPageCount ||
        properties.limits.maxPushConstantsSize < pushConstantBytes) {
        diagnostic = "Device descriptor/push limits do not admit the unchanged staged-primary prototype.";
        return false;
    }
    if (!MakePages(extent, pages_, extentContract_, diagnostic)) return false;
    extent_ = extent;
    std::array<VkDescriptorSetLayoutBinding, kStagedPageCount> bindings{};
    for (std::uint32_t index = 0u; index < kStagedPageCount; ++index)
        bindings[index] = {index, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr};
    VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = kStagedPageCount;
    layoutInfo.pBindings = bindings.data();
    if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &pageLayout_) != VK_SUCCESS) {
        diagnostic = "Failed to create staged-primary page descriptor layout."; return false;
    }
    VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, kStagedPageCount};
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.maxSets = 1u; poolInfo.poolSizeCount = 1u; poolInfo.pPoolSizes = &poolSize;
    if (vkCreateDescriptorPool(device_, &poolInfo, nullptr, &pagePool_) != VK_SUCCESS) {
        diagnostic = "Failed to create staged-primary page descriptor pool."; return false;
    }
    VkDescriptorSetAllocateInfo setInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    setInfo.descriptorPool = pagePool_; setInfo.descriptorSetCount = 1u; setInfo.pSetLayouts = &pageLayout_;
    if (vkAllocateDescriptorSets(device_, &setInfo, &pageSet_) != VK_SUCCESS) {
        diagnostic = "Failed to allocate staged-primary page descriptor set."; return false;
    }
    WritePageDescriptors();
    const std::array<VkDescriptorSetLayout, 2u> layouts{sceneLayout, pageLayout_};
    const VkPushConstantRange range{VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
                                   0u, pushConstantBytes};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.setLayoutCount = 2u; pipelineLayoutInfo.pSetLayouts = layouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1u; pipelineLayoutInfo.pPushConstantRanges = &range;
    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        diagnostic = "Failed to create staged-primary two-set pipeline layout."; return false;
    }
    VkShaderModule miss = VK_NULL_HANDLE, hit = VK_NULL_HANDLE;
    const auto destroyShared = [&] {
        api.destroyShaderModule(api.user, miss); api.destroyShaderModule(api.user, hit);
    };
    if (!api.createSharedShaderModules(api.user, miss, hit, diagnostic)) { destroyShared(); return false; }
    const std::array<std::array<std::span<const std::uint32_t>, 2u>, 2u> words{{
        {kPrimaryOpaqueWords, kShadeOpaqueWords}, {kPrimaryGenericWords, kShadeGenericWords}}};
    for (std::size_t strategy = 0u; strategy < passes_.size(); ++strategy) {
        const auto material = strategy == 0u ? RtMaterialStrategy::OpaqueFast : RtMaterialStrategy::GenericDielectric;
        for (std::size_t pass = 0u; pass < 2u; ++pass) {
            auto& target = passes_[strategy][pass];
            VkShaderModule entry = VK_NULL_HANDLE;
            RtPipelineVariantArtifact artifact{};
            artifact.words = words[strategy][pass];
            artifact.canonicalKey = PairKey(material);
            const bool built = api.createEntryShaderModule(api.user, artifact, entry, diagnostic) &&
                api.createStrategyPipeline(api.user, material, entry, miss, hit,
                                           pipelineLayout_, target.pipeline, diagnostic) &&
                api.createStrategySbt(api.user, material, target.pipeline, target.shaderBindingTable,
                                     target.sbtRegions, diagnostic);
            api.destroyShaderModule(api.user, entry);
            if (!built) { destroyShared(); return false; }
        }
    }
    destroyShared(); diagnostic.clear(); return true;
}

bool StagedPrimaryPass::MakePages(VkExtent2D extent,
    std::array<RtGpuBuffer, kStagedPageCount>& pages, StagedPrimaryExtent& contract,
    std::string& diagnostic)
{
    const auto admitted = TryMakeStagedPrimaryExtent(extent.width, extent.height, maxStorageBufferRange_);
    if (!admitted) { diagnostic = "Staged-primary extent exceeds uint addressing or device SSBO range."; return false; }
    for (auto& page : pages) {
        if (!resources_.CreateBuffer(admitted->pageBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false, page, diagnostic)) {
            DestroyPages(pages); return false;
        }
    }
    contract = *admitted; return true;
}

void StagedPrimaryPass::WritePageDescriptors() noexcept
{
    std::array<VkDescriptorBufferInfo, kStagedPageCount> buffers{};
    std::array<VkWriteDescriptorSet, kStagedPageCount> writes{};
    for (std::uint32_t index = 0u; index < kStagedPageCount; ++index) {
        buffers[index] = {pages_[index].buffer, 0u, pages_[index].size};
        writes[index] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        writes[index].dstSet = pageSet_; writes[index].dstBinding = index;
        writes[index].descriptorCount = 1u; writes[index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[index].pBufferInfo = &buffers[index];
    }
    vkUpdateDescriptorSets(device_, kStagedPageCount, writes.data(), 0u, nullptr);
}

void StagedPrimaryPass::Record(VkCommandBuffer command, RtMaterialStrategy strategy,
                             std::span<const std::byte> pushConstants,
                             StagedPrimaryTiming* timing, std::uint32_t frameSlot) const noexcept
{
    const auto& selected = passes_[strategy == RtMaterialStrategy::OpaqueFast ? 0u : 1u];
    const std::array<VkDescriptorSet, 2u> sets{sceneSet_, pageSet_};
    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                            pipelineLayout_, 0u, 2u, sets.data(), 0u, nullptr);
    vkCmdPushConstants(command, pipelineLayout_,
                       VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
                       0u, static_cast<std::uint32_t>(pushConstants.size()), pushConstants.data());
    const auto dispatch = [&](const RtStrategyPipelineResources& pass) {
        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pass.pipeline);
        trace_(command, &pass.sbtRegions[0], &pass.sbtRegions[1], &pass.sbtRegions[2], &pass.sbtRegions[3],
               extent_.width, extent_.height, 1u);
    };
#if HORDE_RT_STAGED_PRIMARY_TIMING
    bool timingRecorded = timing != nullptr && timing->RecordBegin(command, frameSlot);
#else
    (void)timing; (void)frameSlot;
#endif
    dispatch(selected[0]);
#if HORDE_RT_STAGED_PRIMARY_TIMING
    if (timingRecorded && !timing->RecordPrimaryEnd(command, frameSlot)) {
        timing->CancelRecording(frameSlot); timingRecorded = false;
    }
#endif
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT; barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                         VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0u,
                         1u, &barrier, 0u, nullptr, 0u, nullptr);
    dispatch(selected[1]);
#if HORDE_RT_STAGED_PRIMARY_TIMING
    if (timingRecorded && !timing->RecordEnd(command, frameSlot)) timing->CancelRecording(frameSlot);
#endif
}

bool StagedPrimaryPass::PrepareResizeAfterDeviceIdle(VkExtent2D extent, std::string& diagnostic)
{
    CancelResizeAfterDeviceIdle();
    if (extent.width == extent_.width && extent.height == extent_.height) return true;
    if (!MakePages(extent, replacementPages_, replacementContract_, diagnostic)) return false;
    replacementExtent_ = extent; return true;
}
void StagedPrimaryPass::CommitResizeAfterDeviceIdle() noexcept
{
    if (replacementExtent_.width == 0u) return;
    auto old = pages_; pages_ = replacementPages_; replacementPages_ = {};
    extent_ = replacementExtent_; extentContract_ = replacementContract_;
    replacementExtent_ = {}; replacementContract_ = {};
    WritePageDescriptors(); DestroyPages(old);
}
void StagedPrimaryPass::CancelResizeAfterDeviceIdle() noexcept
{
    DestroyPages(replacementPages_); replacementExtent_ = {}; replacementContract_ = {};
}
void StagedPrimaryPass::DestroyPages(std::array<RtGpuBuffer, kStagedPageCount>& pages) noexcept
{
    for (auto& page : pages) resources_.DestroyBuffer(page);
}
void StagedPrimaryPass::Reset() noexcept
{
    if (device_ == VK_NULL_HANDLE) return;
    // Descriptors/pipelines must not outlive their layout; resources are idle.
    for (auto& strategy : passes_) for (auto& pass : strategy) {
        resources_.DestroyBuffer(pass.shaderBindingTable);
        if (pass.pipeline != VK_NULL_HANDLE) vkDestroyPipeline(device_, pass.pipeline, nullptr);
        pass.pipeline = VK_NULL_HANDLE;
    }
    if (pipelineLayout_ != VK_NULL_HANDLE) vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
    if (pagePool_ != VK_NULL_HANDLE) vkDestroyDescriptorPool(device_, pagePool_, nullptr);
    if (pageLayout_ != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device_, pageLayout_, nullptr);
    DestroyPages(pages_); CancelResizeAfterDeviceIdle(); device_ = VK_NULL_HANDLE;
}
void StagedPrimaryPass::AccumulateResourceInventory(horde::telemetry::RtResourceInventory& inventory) const noexcept
{
    for (const auto& page : pages_) AccumulateRtGpuBuffer(inventory, page);
    for (const auto& strategy : passes_) for (const auto& pass : strategy) {
        AccumulateRtGpuBuffer(inventory, pass.shaderBindingTable);
        if (pass.pipeline != VK_NULL_HANDLE) AccumulateRtResourceCount(inventory.pipelineCount);
    }
    if (pageSet_ != VK_NULL_HANDLE) AccumulateRtResourceCount(inventory.descriptorSetCount);
}
std::uint64_t StagedPrimaryPass::IntermediateAllocationBytes() const noexcept
{
    std::uint64_t bytes = 0u;
    for (const auto& page : pages_) AccumulateRtAllocationBytes(bytes, page.allocationSize);
    return bytes;
}
std::string_view StagedPrimaryPass::PairKey(RtMaterialStrategy strategy) noexcept
{
    return strategy == RtMaterialStrategy::OpaqueFast ? kOpaquePairKey : kGenericPairKey;
}
std::string_view StagedPrimaryPass::PairSha256(RtMaterialStrategy strategy) noexcept
{
    return strategy == RtMaterialStrategy::OpaqueFast ? kOpaquePairSha256 : kGenericPairSha256;
}
std::string StagedPrimaryPass::MetadataJson() const
{
    std::ostringstream out; out.imbue(std::locale::classic());
    out << "{\"organisation\":\"StagedPrimaryV1Investigation\",\"traceDispatchesPerFrame\":2,\"recordBytes\":128,\"pages\":3"
#if HORDE_RT_STAGED_PRIMARY_TIMING
        << ",\"passProfilingCompiled\":true"
#else
        << ",\"passProfilingCompiled\":false"
#endif
        << ",\"logicalBytes\":" << extentContract_.logicalBytes
        << ",\"paddedBufferBytes\":" << extentContract_.paddedBufferBytes
        << ",\"allocationBytes\":" << IntermediateAllocationBytes()
        << ",\"logicalReadWriteBytesPerFrame\":" << extentContract_.logicalReadWriteBytes
        << ",\"dramTrafficMeasured\":false,\"memoryFlags\":[";
    for (std::size_t index = 0u; index < pages_.size(); ++index) {
        if (index != 0u) out << ',';
        out << pages_[index].memoryPropertyFlags;
    }
    out << "],\"moduleSha256\":[\"" << kPrimaryOpaqueSha256 << "\",\"" << kShadeOpaqueSha256
        << "\",\"" << kPrimaryGenericSha256 << "\",\"" << kShadeGenericSha256 << "\"]}";
    return out.str();
}
} // namespace horde::vulkan::raytracing::experimental
