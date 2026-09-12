#include "vulkan/DeviceCapabilities.h"

namespace horde::vulkan
{

std::string ToString(const RtMode mode)
{
    switch (mode)
    {
    case RtMode::RayTracingPipeline:
        return "RayTracingPipeline";
    case RtMode::RayQuery:
        return "RayQuery";
    case RtMode::Unsupported:
    default:
        return "Unsupported";
    }
}

std::string ToString(const RtExecutionBackend backend)
{
    switch (backend)
    {
    case RtExecutionBackend::RayTracingPipeline:
        return "RayTracingPipeline";
    case RtExecutionBackend::RayQueryCompute:
        return "RayQueryCompute";
    case RtExecutionBackend::Unsupported:
    default:
        return "Unsupported";
    }
}

void BeginRtBackendSelection(RtSceneSnapshot& scene, const RtExecutionBackend backend)
{
    scene = {};
    scene.executionBackend = backend;
    scene.status = backend == RtExecutionBackend::Unsupported
        ? "No supported execution backend selected"
        : "Execution backend selected; presentation pending";
}

} // namespace horde::vulkan
