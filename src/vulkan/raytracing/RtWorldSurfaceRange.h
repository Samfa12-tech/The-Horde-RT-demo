#pragma once
#include "vulkan/raytracing/RtSceneAbi.generated.h"
#include <string>

namespace horde::vulkan::raytracing {
// Non-StaticPbr metadata indexes the packed procedural surface-code buffer.
// It owns no static primitive row, material, texture or descriptor.
inline bool SetWorldSurfaceRange(RtInstanceMetadata& metadata, std::uint32_t base,
    std::uint32_t count, std::size_t totalCodes, std::string& diagnostic) {
    if(metadata.flags!=0 || !count || base>totalCodes || count>totalCodes-base) {
        diagnostic="Procedural surface range overlaps a static owner or exceeds its code buffer.";
        return false;
    }
    metadata.primitiveBase=base;metadata.primitiveCount=count;
    metadata.reserved1=1; // Procedural local-coordinate projection marker; no StaticPbr ownership.
    diagnostic.clear();return true;
}
}
