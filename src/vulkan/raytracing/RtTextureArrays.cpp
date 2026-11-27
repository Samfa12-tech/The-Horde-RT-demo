#include "vulkan/raytracing/RtTextureArrays.h"

#include "vulkan/raytracing/RtSceneAbi.generated.h"

namespace horde::vulkan::raytracing
{

bool RtTextureArrays::Validate(const RtTextureArrayCounts& counts, std::string& diagnostic)
{
    if (counts.baseColor > kRtTextureLayerCapacity)
        diagnostic = "RtTextureArrays capacity overflow: baseColor layers exceed " + std::to_string(kRtTextureLayerCapacity) + ".";
    else if (counts.normal > kRtTextureLayerCapacity)
        diagnostic = "RtTextureArrays capacity overflow: normal layers exceed " + std::to_string(kRtTextureLayerCapacity) + ".";
    else if (counts.orm > kRtTextureLayerCapacity)
        diagnostic = "RtTextureArrays capacity overflow: ORM layers exceed " + std::to_string(kRtTextureLayerCapacity) + ".";
    else if (counts.emissive > kRtTextureLayerCapacity)
        diagnostic = "RtTextureArrays capacity overflow: emissive layers exceed " + std::to_string(kRtTextureLayerCapacity) + ".";
    else
    {
        diagnostic.clear();
        return true;
    }
    return false;
}

} // namespace horde::vulkan::raytracing
