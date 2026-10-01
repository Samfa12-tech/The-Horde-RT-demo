#pragma once

#include <span>
#include <vulkan/vulkan.h>

namespace horde::vulkan::raytracing
{

// Conservative, backend-neutral policy: rebuild on discrete instance-definition
// changes, refit on transform/BLAS-content motion. TLAS BUILD restored missing
// instance visibility in the tested S24 comparison; its underlying cause is not
// proven. Do not turn this into a device-name quality workaround or an every-frame rebuild.
inline bool RequiresTlasInstanceRebuild(
    std::span<const VkAccelerationStructureInstanceKHR> built,
    std::span<const VkAccelerationStructureInstanceKHR> next) noexcept
{
    if (built.size() != next.size()) return true;
    for (std::size_t i = 0u; i < next.size(); ++i)
    {
        if (built[i].mask != next[i].mask ||
            built[i].accelerationStructureReference != next[i].accelerationStructureReference ||
            built[i].instanceCustomIndex != next[i].instanceCustomIndex ||
            built[i].instanceShaderBindingTableRecordOffset != next[i].instanceShaderBindingTableRecordOffset ||
            built[i].flags != next[i].flags)
            return true;
    }
    return false;
}

} // namespace horde::vulkan::raytracing
