#pragma once

namespace horde::vulkan
{

// Execution choice, not raw hardware capability or proof of presentation.
enum class RtExecutionBackend
{
    Unsupported,
    RayTracingPipeline,
    RayQueryCompute,
};

} // namespace horde::vulkan
