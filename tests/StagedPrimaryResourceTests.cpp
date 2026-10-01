#include "vulkan/raytracing/experimental/StagedPrimaryPass.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

using namespace horde::vulkan::raytracing;
using namespace horde::vulkan::raytracing::experimental;

namespace {
template <typename Handle>
Handle FakeHandle(std::uintptr_t value)
{
    if constexpr (std::is_pointer_v<Handle>) return reinterpret_cast<Handle>(value);
    else return static_cast<Handle>(value);
}
template <typename Handle>
std::uintptr_t HandleValue(Handle value)
{
    if constexpr (std::is_pointer_v<Handle>) return reinterpret_cast<std::uintptr_t>(value);
    else return static_cast<std::uintptr_t>(value);
}

struct Context {
    std::uintptr_t nextHandle = 100u;
    int bufferCreateCalls = 0;
    int failBufferCall = 0;
    int bufferDestroyCalls = 0;
    std::map<std::uintptr_t, int> destroyedBuffers;
    int layoutCreateCalls = 0;
    int failLayoutCreate = 0;
    int layoutDestroyCalls = 0;
    int poolCreateCalls = 0;
    int failPoolCreate = 0;
    int poolDestroyCalls = 0;
    int descriptorSetAllocCalls = 0;
    int failDescriptorSetAlloc = 0;
    int descriptorSetUpdates = 0;
    std::vector<std::array<VkBuffer, kStagedPageCount>> descriptorPageHistory;
    int pipelineLayoutCreateCalls = 0;
    int pipelineLayoutDestroyCalls = 0;
    int pipelineDestroyCalls = 0;
    int entryModuleCreates = 0;
    int sharedModuleCreates = 0;
    int moduleDestroyCalls = 0;
    int pipelineCreateCalls = 0;
    int failPipelineCreate = 0;
    int sbtCreateCalls = 0;
    int failSbtCreate = 0;
    RtGpuResources* resources = nullptr;
};

Context* gContext = nullptr;

struct CheckState {
    int failures = 0;
    void Check(bool ok, const char* message)
    {
        if (!ok) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    }
};

bool CreateShared(void* user, VkShaderModule& miss, VkShaderModule& hit, std::string&)
{
    auto& c = *static_cast<Context*>(user);
    ++c.sharedModuleCreates;
    miss = FakeHandle<VkShaderModule>(++c.nextHandle);
    hit = FakeHandle<VkShaderModule>(++c.nextHandle);
    return true;
}
bool CreateEntry(void* user, const RtPipelineVariantArtifact& artifact,
                 VkShaderModule& module, std::string& diagnostic)
{
    auto& c = *static_cast<Context*>(user);
    ++c.entryModuleCreates;
    if (artifact.words.empty()) { diagnostic = "empty test shader header"; return false; }
    module = FakeHandle<VkShaderModule>(++c.nextHandle);
    return true;
}
bool CreatePipeline(void* user, RtMaterialStrategy, VkShaderModule, VkShaderModule,
                    VkShaderModule, VkPipelineLayout, VkPipeline& pipeline,
                    std::string& diagnostic)
{
    auto& c = *static_cast<Context*>(user);
    ++c.pipelineCreateCalls;
    pipeline = FakeHandle<VkPipeline>(++c.nextHandle); // partial output is owned and must be cleaned
    if (c.pipelineCreateCalls == c.failPipelineCreate) {
        diagnostic = "injected strategy pipeline failure";
        return false;
    }
    return true;
}
void DestroyModule(void* user, VkShaderModule& module) noexcept
{
    auto& c = *static_cast<Context*>(user);
    if (module != VK_NULL_HANDLE) ++c.moduleDestroyCalls;
    module = VK_NULL_HANDLE;
}
bool CreateSbt(void* user, RtMaterialStrategy, VkPipeline pipeline, RtGpuBuffer& buffer,
               std::array<VkStridedDeviceAddressRegionKHR, 4u>& regions, std::string& diagnostic)
{
    auto& c = *static_cast<Context*>(user);
    ++c.sbtCreateCalls;
    if (!c.resources->CreateBuffer(256u, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, buffer, diagnostic)) return false;
    for (std::size_t i = 0u; i < regions.size(); ++i) {
        regions[i].deviceAddress = buffer.address + i * 64u;
        regions[i].stride = 32u;
        regions[i].size = 32u;
    }
    if (c.sbtCreateCalls == c.failSbtCreate) {
        diagnostic = "injected SBT failure after partial buffer output";
        return false;
    }
    return pipeline != VK_NULL_HANDLE;
}
VKAPI_ATTR void VKAPI_CALL FakeTrace(VkCommandBuffer, const VkStridedDeviceAddressRegionKHR*,
    const VkStridedDeviceAddressRegionKHR*, const VkStridedDeviceAddressRegionKHR*,
    const VkStridedDeviceAddressRegionKHR*, std::uint32_t, std::uint32_t, std::uint32_t) {}

RtPipelineBundleBuildApi BuildApi(Context& c)
{
    return {&c, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            &CreateShared, &CreateEntry, &CreatePipeline, &DestroyModule, &CreateSbt};
}

std::unique_ptr<StagedPrimaryPass> CreateReady(Context& c, RtGpuResources& resources,
    std::string& diagnostic, VkExtent2D extent = {64u, 32u})
{
    c.resources = &resources;
    c.pipelineCreateCalls = 0;
    c.sbtCreateCalls = 0;
    return StagedPrimaryPass::Create(FakeHandle<VkPhysicalDevice>(1u), FakeHandle<VkDevice>(2u),
        resources, FakeHandle<VkDescriptorSetLayout>(3u), FakeHandle<VkDescriptorSet>(4u),
        extent, 32u, &FakeTrace, BuildApi(c), diagnostic);
}

void TestInvalidPreflight(CheckState& check, Context& c, RtGpuResources& resources)
{
    std::string diagnostic;
    const auto api = BuildApi(c);
    const auto result = StagedPrimaryPass::Create(VK_NULL_HANDLE, FakeHandle<VkDevice>(2u), resources,
        FakeHandle<VkDescriptorSetLayout>(3u), FakeHandle<VkDescriptorSet>(4u), {8u, 8u},
        32u, nullptr, api, diagnostic);
    check.Check(!result && diagnostic.find("preflight") != std::string::npos &&
                c.bufferCreateCalls == 0 && c.layoutCreateCalls == 0,
                "invalid handles reject before any allocation");
}

void TestPartialPageAndDescriptorFailures(CheckState& check, Context& c, RtGpuResources& resources)
{
    std::string diagnostic;
    c.failBufferCall = c.bufferCreateCalls + 2;
    const int destroyedBefore = c.bufferDestroyCalls;
    const auto pageFailure = CreateReady(c, resources, diagnostic);
    check.Check(!pageFailure && c.bufferDestroyCalls == destroyedBefore + 1 &&
                c.destroyedBuffers.size() == static_cast<std::size_t>(destroyedBefore + 1),
                "partial page creation destroys the single created buffer once");

    c.failBufferCall = 0;
    const int bufferCallsBefore = c.bufferDestroyCalls;
    const int layoutDiedBefore = c.layoutDestroyCalls;
    c.failPoolCreate = c.poolCreateCalls + 1;
    const auto poolFailure = CreateReady(c, resources, diagnostic);
    check.Check(!poolFailure && c.layoutDestroyCalls == layoutDiedBefore + 1 &&
                c.bufferDestroyCalls == bufferCallsBefore + 3,
                "descriptor-pool failure cleans all pages and the partial descriptor layout once");
    c.failPoolCreate = 0;
}

void TestPipelineAndSbtFailures(CheckState& check, Context& c, RtGpuResources& resources)
{
    std::string diagnostic;
    const int pipeDiedBefore = c.pipelineDestroyCalls;
    const int buffersDiedBefore = c.bufferDestroyCalls;
    const int modulesDiedBefore = c.moduleDestroyCalls;
    c.failPipelineCreate = 3; // CreateReady resets per-attempt call numbering.
    const auto pipelineFailure = CreateReady(c, resources, diagnostic);
    check.Check(!pipelineFailure && c.pipelineDestroyCalls == pipeDiedBefore + 3 &&
                c.bufferDestroyCalls == buffersDiedBefore + 5 &&
                c.moduleDestroyCalls == modulesDiedBefore + 5,
                "pipeline failure after prior strategies destroys partial pipelines, SBTs and modules once");

    c.failPipelineCreate = 0;
    const int pipeDiedSbtBefore = c.pipelineDestroyCalls;
    const int buffersDiedSbtBefore = c.bufferDestroyCalls;
    c.failSbtCreate = 2; // Fail the second SBT after its partial buffer output.
    const auto sbtFailure = CreateReady(c, resources, diagnostic);
    check.Check(!sbtFailure && c.pipelineDestroyCalls == pipeDiedSbtBefore + 2 &&
                c.bufferDestroyCalls == buffersDiedSbtBefore + 5,
                "SBT failure after partial buffer output destroys that buffer and all owned predecessors once");
    c.failSbtCreate = 0;
}

void TestResizeInventoryAndUniqueOwner(CheckState& check, Context& c, RtGpuResources& resources)
{
    std::string diagnostic;
    c.failBufferCall = 0;
    const int destroyedBefore = c.bufferDestroyCalls;
    auto owner = CreateReady(c, resources, diagnostic, {64u, 32u});
    check.Check(owner != nullptr, "fixture creates a complete Shipping/Mobile owner");
    if (!owner) return;
    const auto initial = owner->ExtentContract();
    const auto initialBytes = owner->IntermediateAllocationBytes();
    horde::telemetry::RtResourceInventory inventory{};
    owner->AccumulateResourceInventory(inventory);
    check.Check(initial.pixelCount == 2048u && initial.pageBytes == 683u * 128u &&
                initialBytes == 3u * (initial.pageBytes + 64u) &&
                inventory.bufferCount == 7u && inventory.memoryAllocationCount == 7u &&
                inventory.deviceLocalBytes == initialBytes + 4u * (256u + 64u) &&
                inventory.hostVisibleBytes == 0u &&
                inventory.pipelineCount == 4u && inventory.descriptorSetCount == 1u,
                "successful resource inventory reports three pages, four SBTs and four pipelines");
    check.Check(owner->MetadataJson().find("\"dramTrafficMeasured\":false") != std::string::npos,
                "metadata explicitly avoids claiming measured DRAM traffic");
    const auto oldPages = c.descriptorPageHistory.back();
    const int updateCount = c.descriptorSetUpdates;

    c.failBufferCall = c.bufferCreateCalls + 2;
    const int afterOldOnly = c.bufferDestroyCalls;
    check.Check(!owner->PrepareResizeAfterDeviceIdle({80u, 40u}, diagnostic) &&
                owner->ExtentContract().pixelCount == initial.pixelCount &&
                c.descriptorSetUpdates == updateCount && c.bufferDestroyCalls == afterOldOnly + 1,
                "failed resize allocation preserves old extent and descriptor pages, cleaning only partial replacement");
    check.Check(c.descriptorPageHistory.back() == oldPages,
                "failed resize does not rewrite descriptors");

    c.failBufferCall = 0;
    check.Check(owner->PrepareResizeAfterDeviceIdle({80u, 40u}, diagnostic),
                "replacement pages can be prepared before output transaction commits");
    owner->CancelResizeAfterDeviceIdle(); // caller reports output-image replacement failure
    check.Check(owner->ExtentContract().pixelCount == initial.pixelCount &&
                c.descriptorSetUpdates == updateCount && c.bufferDestroyCalls == afterOldOnly + 4 &&
                c.descriptorPageHistory.back() == oldPages,
                "cancel after output failure preserves original pages/descriptors and retires replacement pages");

    check.Check(owner->PrepareResizeAfterDeviceIdle({80u, 40u}, diagnostic),
                "successful resize preparation allocates replacement pages");
    owner->CommitResizeAfterDeviceIdle();
    const auto replacement = owner->ExtentContract();
    check.Check(replacement.pixelCount == 3200u && replacement.pageBytes == 1067u * 128u &&
                c.descriptorSetUpdates == updateCount + 1 &&
                c.descriptorPageHistory.back() != oldPages && c.bufferDestroyCalls == afterOldOnly + 7,
                "commit publishes new extent/descriptors before retiring all old pages");

    auto moved = std::move(owner);
    check.Check(!owner && moved && moved->ExtentContract().pixelCount == 3200u,
                "unique owner transfers without duplicating ownership");
    const int beforeFinalDestroy = c.bufferDestroyCalls;
    moved.reset();
    check.Check(c.bufferDestroyCalls == beforeFinalDestroy + 7,
                "final unique-owner destruction retires three pages and four SBTs exactly once");
    check.Check(c.destroyedBuffers.size() == static_cast<std::size_t>(c.bufferDestroyCalls) &&
                std::all_of(c.destroyedBuffers.begin(), c.destroyedBuffers.end(),
                    [](const auto& item) { return item.second == 1; }),
                "every fake buffer handle has exactly one destruction");
    (void)destroyedBefore;
}
} // namespace

namespace horde::vulkan::raytracing {
// Test-only doubles: production StagedPrimaryPass is linked unchanged, while
// buffer ownership is deterministic and never calls a Vulkan driver.
std::optional<RtDescriptorIoContract> TryMakeRtDescriptorIoContract(RtInstrumentation instrumentation) noexcept
{
    if (instrumentation != RtInstrumentation::Shipping && instrumentation != RtInstrumentation::Diagnostic)
        return std::nullopt;
    RtDescriptorIoContract contract{};
    contract.instrumentation = instrumentation;
    contract.bindingCount = instrumentation == RtInstrumentation::Shipping ? 24u : 25u;
    contract.storageBufferDescriptorCount = 13u;
    return contract;
}

bool RtGpuResources::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags, VkMemoryPropertyFlags flags,
                                  bool deviceAddress, RtGpuBuffer& out, std::string& diagnostic) const
{
    auto& c = *gContext;
    ++c.bufferCreateCalls;
    out = {};
    if (c.bufferCreateCalls == c.failBufferCall) {
        diagnostic = "injected buffer allocation failure";
        return false;
    }
    out.buffer = FakeHandle<VkBuffer>(++c.nextHandle);
    out.memory = FakeHandle<VkDeviceMemory>(++c.nextHandle);
    out.address = deviceAddress ? c.nextHandle * 256u : 0u;
    out.size = size;
    out.allocationSize = size + 64u;
    out.memoryPropertyFlags = flags;
    diagnostic.clear();
    return true;
}
void RtGpuResources::DestroyBuffer(RtGpuBuffer& buffer) const
{
    if (buffer.buffer != VK_NULL_HANDLE || buffer.memory != VK_NULL_HANDLE) {
        ++gContext->bufferDestroyCalls;
        ++gContext->destroyedBuffers[HandleValue(buffer.buffer)];
    }
    buffer = {};
}
} // namespace horde::vulkan::raytracing

extern "C" {
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(VkPhysicalDevice, VkPhysicalDeviceProperties* p)
{
    *p = {};
    p->limits.maxStorageBufferRange = 64u * 1024u * 1024u;
    p->limits.maxBoundDescriptorSets = 8u;
    p->limits.maxPerStageDescriptorStorageBuffers = 64u;
    p->limits.maxDescriptorSetStorageBuffers = 64u;
    p->limits.maxPerStageResources = 128u;
    p->limits.maxPushConstantsSize = 256u;
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorSetLayout(VkDevice, const VkDescriptorSetLayoutCreateInfo*,
    const VkAllocationCallbacks*, VkDescriptorSetLayout* out)
{
    auto& c = *gContext; ++c.layoutCreateCalls;
    *out = FakeHandle<VkDescriptorSetLayout>(++c.nextHandle);
    return c.layoutCreateCalls == c.failLayoutCreate ? VK_ERROR_OUT_OF_HOST_MEMORY : VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorSetLayout(VkDevice, VkDescriptorSetLayout, const VkAllocationCallbacks*)
{ ++gContext->layoutDestroyCalls; }
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorPool(VkDevice, const VkDescriptorPoolCreateInfo*,
    const VkAllocationCallbacks*, VkDescriptorPool* out)
{
    auto& c = *gContext; ++c.poolCreateCalls;
    *out = FakeHandle<VkDescriptorPool>(++c.nextHandle);
    return c.poolCreateCalls == c.failPoolCreate ? VK_ERROR_OUT_OF_HOST_MEMORY : VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorPool(VkDevice, VkDescriptorPool, const VkAllocationCallbacks*)
{ ++gContext->poolDestroyCalls; }
VKAPI_ATTR VkResult VKAPI_CALL vkAllocateDescriptorSets(VkDevice, const VkDescriptorSetAllocateInfo*, VkDescriptorSet* out)
{
    auto& c = *gContext; ++c.descriptorSetAllocCalls;
    *out = FakeHandle<VkDescriptorSet>(++c.nextHandle);
    return c.descriptorSetAllocCalls == c.failDescriptorSetAlloc ? VK_ERROR_OUT_OF_POOL_MEMORY : VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSets(VkDevice, std::uint32_t count, const VkWriteDescriptorSet* writes,
    std::uint32_t, const VkCopyDescriptorSet*)
{
    auto& c = *gContext;
    ++c.descriptorSetUpdates;
    std::array<VkBuffer, kStagedPageCount> pages{};
    for (std::uint32_t i = 0u; i < count && i < pages.size(); ++i)
        pages[writes[i].dstBinding] = writes[i].pBufferInfo->buffer;
    c.descriptorPageHistory.push_back(pages);
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineLayout(VkDevice, const VkPipelineLayoutCreateInfo*,
    const VkAllocationCallbacks*, VkPipelineLayout* out)
{ ++gContext->pipelineLayoutCreateCalls; *out = FakeHandle<VkPipelineLayout>(++gContext->nextHandle); return VK_SUCCESS; }
VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineLayout(VkDevice, VkPipelineLayout, const VkAllocationCallbacks*)
{ ++gContext->pipelineLayoutDestroyCalls; }
VKAPI_ATTR void VKAPI_CALL vkDestroyPipeline(VkDevice, VkPipeline, const VkAllocationCallbacks*)
{ ++gContext->pipelineDestroyCalls; }
VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorSets(VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout,
    std::uint32_t, std::uint32_t, const VkDescriptorSet*, std::uint32_t, const std::uint32_t*) {}
VKAPI_ATTR void VKAPI_CALL vkCmdBindPipeline(VkCommandBuffer, VkPipelineBindPoint, VkPipeline) {}
VKAPI_ATTR void VKAPI_CALL vkCmdPushConstants(VkCommandBuffer, VkPipelineLayout, VkShaderStageFlags,
    std::uint32_t, std::uint32_t, const void*) {}
VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier(VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags,
    VkDependencyFlags, std::uint32_t, const VkMemoryBarrier*, std::uint32_t, const VkBufferMemoryBarrier*,
    std::uint32_t, const VkImageMemoryBarrier*) {}
} // extern "C"

int main()
{
    CheckState check;
    Context context;
    gContext = &context;
    RtGpuResources resources;
    TestInvalidPreflight(check, context, resources);
    TestPartialPageAndDescriptorFailures(check, context, resources);
    TestPipelineAndSbtFailures(check, context, resources);
    TestResizeInventoryAndUniqueOwner(check, context, resources);
    gContext = nullptr;
    if (check.failures == 0) std::cout << "PASS staged-primary owner fault injection (fake Vulkan; Shipping/Mobile fixture)\n";
    return check.failures == 0 ? 0 : 1;
}
