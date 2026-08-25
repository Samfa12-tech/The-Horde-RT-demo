#pragma once

#include <cstdint>
#include <string>

namespace horde::vulkan::raytracing
{

struct RtTextureArrayCounts
{
    std::uint32_t baseColor = 0u;
    std::uint32_t normal = 0u;
    std::uint32_t orm = 0u;
    std::uint32_t emissive = 0u;
};

class RtTextureArrays
{
public:
    static bool Validate(const RtTextureArrayCounts& counts, std::string& diagnostic);
};

} // namespace horde::vulkan::raytracing
