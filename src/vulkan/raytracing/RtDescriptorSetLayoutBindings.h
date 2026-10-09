#pragma once

#include "vulkan/raytracing/RtPipelineBundleContracts.h"
#include <vulkan/vulkan.h>
#include <array>
#include <optional>
#include <tuple>

namespace horde::vulkan::raytracing {

struct RtDescriptorSetLayoutBindings {
    std::array<VkDescriptorSetLayoutBinding,
        std::tuple_size_v<decltype(RtDescriptorIoContract::bindings)>> values{};
    std::uint32_t count = 0u;
};

// Both instrumentation profiles use the admitted roster's capacity. Reject an
// oversized count before touching either array, including in unchecked builds.
[[nodiscard]] inline std::optional<RtDescriptorSetLayoutBindings>
TryMakeRtDescriptorSetLayoutBindings(const RtDescriptorIoContract& contract,
    VkShaderStageFlags accelerationStructureStages, VkShaderStageFlags resourceStage)
{
    RtDescriptorSetLayoutBindings result;
    if (contract.bindingCount == 0u || contract.bindingCount > result.values.size())
        return std::nullopt;
    for (std::uint32_t index = 0u; index < contract.bindingCount; ++index) {
        const auto& selected = contract.bindings[index];
        VkDescriptorType type;
        switch (selected.kind) {
        case RtDescriptorResourceKind::AccelerationStructure:
            type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR; break;
        case RtDescriptorResourceKind::StorageImage:
            type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; break;
        case RtDescriptorResourceKind::CombinedImageSampler:
            type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; break;
        case RtDescriptorResourceKind::StorageBuffer:
            type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
        default: return std::nullopt;
        }
        result.values[index] = {selected.binding, type, 1u,
            selected.binding == 0u ? accelerationStructureStages : resourceStage, nullptr};
    }
    result.count = contract.bindingCount;
    return result;
}

} // namespace horde::vulkan::raytracing
