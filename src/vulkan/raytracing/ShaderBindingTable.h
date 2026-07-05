#pragma once

#include <cstdint>
#include <string>

namespace horde::vulkan::raytracing
{

struct RtGeometryConfig
{
    std::uint32_t width = 256;
    std::uint32_t height = 256;
};

bool ValidateShaderBindingTablePlan(const RtGeometryConfig& config, std::string& diagnostic);

} // namespace horde::vulkan::raytracing
