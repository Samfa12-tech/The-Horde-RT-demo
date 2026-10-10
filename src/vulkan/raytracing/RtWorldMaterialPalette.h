#pragma once

#include "vulkan/raytracing/RtSceneAbi.generated.h"

namespace horde::vulkan::raytracing {

// The first five authored surface types differ in the packed surface code,
// not in their material record. Share the identical record; the leaf tint is
// the sole distinct authored override. This does not change texture selection,
// surface classification, ray masks or the shader/ABI.
struct RtWorldMaterialPalette {
    std::array<RtMaterialGpu,2> records{};
    std::uint32_t count=1;
};

inline RtWorldMaterialPalette MakeRtWorldMaterialPalette(bool withLeaves) {
    RtWorldMaterialPalette result;
    result.records[0].baseColorFactor={{1.f,1.f,1.f,1.f}};
    result.records[0].normalScaleUvScaleBlend={{1.f,.42f,.42f,.34f}};
    result.records[1]=result.records[0];
    result.records[1].baseColorFactor={{.24f,.52f,.19f,1.f}};
    result.records[1].normalScaleUvScaleBlend[0]=0.f;
    result.count=withLeaves?2u:1u;
    return result;
}

inline std::uint32_t AuthoredWorldMaterialIndexPlusOne(std::uint32_t base,
                                                      std::uint32_t surfaceType) {
    return surfaceType<5u?base+1u:0u;
}
} // namespace horde::vulkan::raytracing
