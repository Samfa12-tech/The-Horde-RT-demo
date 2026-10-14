#pragma once
#include "vulkan/raytracing/RtStaticMeshSlot.h"
#include "world/streaming/GeometryPartition.h"

namespace horde::vulkan::raytracing {
struct RtRegionGeometrySourceResult {
    std::unique_ptr<RtStaticMeshSlot> base;
    std::shared_ptr<const horde::world::streaming::GeometryPayload> detail;
};
// Loader/startup adapter from admitted native geometry to independent owners.
// No Vulkan work or contributor/collision readiness is inferred here. The byte
// allowance admits requested preparation storage and actual returned capacities,
// including copied shared CPU records and remap scratch. Vector allocation may
// transiently over-allocate before capacity rejection; this is not a hard heap
// limit. Allocator bookkeeping, immutable input and previous output are separate.
class RtRegionGeometrySource {
public:
    static bool Prepare(const RtStaticMeshSlot&,std::uint32_t exclusiveInstance,
        std::span<const std::uint32_t> triangles,std::uint64_t preparationByteLimit,
        RtRegionGeometrySourceResult&,std::string&);
};
}
