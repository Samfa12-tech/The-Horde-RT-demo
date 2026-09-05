#include "vulkan/raytracing/RtGpuResources.h"
#include "vulkan/raytracing/RtSceneRecordObservation.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>

namespace
{

using namespace horde::vulkan::raytracing;
using horde::telemetry::RtStage;
using horde::telemetry::RtStageAccumulator;
using horde::telemetry::RtStageFrameSample;
using horde::telemetry::RtStageIndex;

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

struct FakeVulkanState
{
    VkMemoryRequirements requirements{};
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    std::array<std::byte, 512u> mappedBytes{};
    VkResult mapResult = VK_SUCCESS;
    std::uint32_t mapCount = 0u;
    std::uint32_t unmapCount = 0u;
    std::uint32_t destroyBufferCount = 0u;
    std::uint32_t freeMemoryCount = 0u;
};

FakeVulkanState gVulkan;

struct TestClock
{
    std::array<std::uint64_t, 8u> values{};
    std::size_t count = 0u;
    std::size_t reads = 0u;
};

std::uint64_t ReadTestClock(void* user) noexcept
{
    auto& clock = *static_cast<TestClock*>(user);
    const std::size_t index = clock.reads++;
    return index < clock.count ? clock.values[index] : 0u;
}

bool Require(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
    }
    return condition;
}

} // namespace

extern "C"
{

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice, VkPhysicalDeviceMemoryProperties* properties)
{
    *properties = gVulkan.memoryProperties;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer(
    VkDevice, const VkBufferCreateInfo*, const VkAllocationCallbacks*, VkBuffer* buffer)
{
    *buffer = FakeHandle<VkBuffer>(0x101u);
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements(
    VkDevice, VkBuffer, VkMemoryRequirements* requirements)
{
    *requirements = gVulkan.requirements;
}

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory(
    VkDevice, const VkMemoryAllocateInfo*, const VkAllocationCallbacks*, VkDeviceMemory* memory)
{
    *memory = FakeHandle<VkDeviceMemory>(0x202u);
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory(
    VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize)
{
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory(
    VkDevice, VkDeviceMemory, const VkDeviceSize offset, VkDeviceSize,
    VkMemoryMapFlags, void** mapped)
{
    ++gVulkan.mapCount;
    if (gVulkan.mapResult != VK_SUCCESS)
    {
        *mapped = nullptr;
        return gVulkan.mapResult;
    }
    *mapped = gVulkan.mappedBytes.data() + static_cast<std::size_t>(offset);
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkUnmapMemory(VkDevice, VkDeviceMemory)
{
    ++gVulkan.unmapCount;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(
    VkDevice, VkBuffer, const VkAllocationCallbacks*)
{
    ++gVulkan.destroyBufferCount;
}

VKAPI_ATTR void VKAPI_CALL vkFreeMemory(
    VkDevice, VkDeviceMemory, const VkAllocationCallbacks*)
{
    ++gVulkan.freeMemoryCount;
}

} // extern "C"

int main()
{
    bool ok = true;
    gVulkan = {};
    gVulkan.requirements.size = 256u;
    gVulkan.requirements.alignment = 64u;
    gVulkan.requirements.memoryTypeBits = 0x2u;
    gVulkan.memoryProperties.memoryTypeCount = 2u;
    gVulkan.memoryProperties.memoryTypes[0].propertyFlags =
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    constexpr VkMemoryPropertyFlags kSelectedUmaFlags =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    gVulkan.memoryProperties.memoryTypes[1].propertyFlags = kSelectedUmaFlags;

    RtGpuResources resources;
    resources.Bind(FakeHandle<VkPhysicalDevice>(0x11u), FakeHandle<VkDevice>(0x22u),
                   nullptr, nullptr);
    RtGpuBuffer buffer{};
    std::string diagnostic;
    ok &= Require(resources.CreateBuffer(
                      96u, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      false, buffer, diagnostic),
                  "mocked RT buffer allocation failed");
    ok &= Require(buffer.size == 96u && buffer.allocationSize == 256u,
                  "RT buffer must retain logical and allocated sizes separately");
    ok &= Require(buffer.memoryPropertyFlags == kSelectedUmaFlags,
                  "RT buffer must retain actual selected UMA memory flags");

    RtStageAccumulator accumulator;
    TestClock clock{{100u, 145u, 200u, 240u}, 4u, 0u};
    RtSceneRecordObservation observation{&accumulator, &clock, ReadTestClock};
    ok &= Require(accumulator.Begin(), "upload observation attempt did not begin");
    const std::array<std::uint8_t, 12u> payload{
        1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u};
    ok &= Require(resources.WriteBufferRange(
                      buffer, 4u, payload.data(), payload.size(),
                      "observed fixture", diagnostic, &observation),
                  "observed upload failed");
    RtStageFrameSample uploadSample{};
    ok &= Require(accumulator.Commit(uploadSample), "observed upload did not commit");
    const auto& upload = uploadSample.values[RtStageIndex(RtStage::DynamicUpload)];
    ok &= Require(upload.durationNanoseconds == 45u &&
                      upload.workInvocationCount == 1u &&
                      upload.byteCount == payload.size() &&
                      upload.operationCount == 1u &&
                      gVulkan.mapCount == 1u && gVulkan.unmapCount == 1u &&
                      std::memcmp(gVulkan.mappedBytes.data() + 4u,
                                  payload.data(), payload.size()) == 0,
                  "successful map/copy/unmap must report one exact upload operation");

    const std::size_t readsBeforeOmitted = clock.reads;
    ok &= Require(accumulator.Begin(), "omitted upload attempt did not begin");
    ok &= Require(resources.WriteBufferRange(
                      buffer, 0u, payload.data(), payload.size(),
                      "unobserved fixture", diagnostic),
                  "unobserved upload failed");
    RtStageFrameSample omittedSample{};
    ok &= Require(accumulator.Commit(omittedSample) &&
                      omittedSample.values[RtStageIndex(RtStage::DynamicUpload)].operationCount == 0u &&
                      clock.reads == readsBeforeOmitted,
                  "omitting the observer must not sample its clock or attribute work");

    const auto aggregatesBeforeFailure = accumulator.AggregatesByValue();
    ok &= Require(accumulator.Begin(), "failed upload attempt did not begin");
    gVulkan.mapResult = VK_ERROR_MEMORY_MAP_FAILED;
    ok &= Require(!resources.WriteBufferRange(
                      buffer, 0u, payload.data(), payload.size(),
                      "failing fixture", diagnostic, &observation),
                  "injected map failure unexpectedly succeeded");
    ok &= Require(accumulator.Abort(), "failed upload attempt was not discarded");
    const auto aggregatesAfterFailure = accumulator.AggregatesByValue();
    ok &= Require(
        aggregatesAfterFailure.values[RtStageIndex(RtStage::DynamicUpload)].sampleCount ==
                aggregatesBeforeFailure.values[RtStageIndex(RtStage::DynamicUpload)].sampleCount &&
            aggregatesAfterFailure.values[RtStageIndex(RtStage::DynamicUpload)].operationCount ==
                aggregatesBeforeFailure.values[RtStageIndex(RtStage::DynamicUpload)].operationCount,
        "failed renderer upload must remain discardable by the record owner");

    resources.DestroyBuffer(buffer);
    ok &= Require(buffer.buffer == VK_NULL_HANDLE && buffer.memory == VK_NULL_HANDLE &&
                      buffer.address == 0u && buffer.size == 0u &&
                      buffer.allocationSize == 0u && buffer.memoryPropertyFlags == 0u &&
                      gVulkan.destroyBufferCount == 1u && gVulkan.freeMemoryCount == 1u,
                  "destroy must clear handles and all retained allocation facts");

    return ok ? 0 : 1;
}
