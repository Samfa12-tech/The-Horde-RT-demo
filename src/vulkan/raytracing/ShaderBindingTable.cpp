#include "vulkan/raytracing/ShaderBindingTable.h"

namespace horde::vulkan::raytracing
{

bool ValidateShaderBindingTablePlan(const RtGeometryConfig& config, std::string& diagnostic)
{
    if (config.width == 0 || config.height == 0)
    {
        diagnostic = "Dispatch resolution must be non-zero for shader binding table emission planning.";
        return false;
    }

    if (config.width % 4 != 0 || config.height % 4 != 0)
    {
        diagnostic = "Tiny scene dispatch resolution must be 4-aligned for this phase.";
        return false;
    }

    diagnostic.clear();
    return true;
}

} // namespace horde::vulkan::raytracing
