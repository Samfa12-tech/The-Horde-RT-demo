#pragma once

#include "vulkan/raytracing/DielectricContactGeometry.h"
#include "vulkan/raytracing/RtSceneAbi.generated.h"

namespace horde::vulkan::raytracing
{

// Certify the actual indexed source triangle, not its packed shading-normal
// label alone. This immutable metadata does not admit/consume a contact hit.
inline RtWorldSurfaceGpu MakeWorldSurfaceRecord(std::uint32_t code,
    const std::array<Vec3, 3u>& triangle, const Vec3& authoredOutwardNormal)
{
    RtWorldSurfaceGpu record;
    record.code = code;
    if (const auto plane = CertifyAxisContactPlane(triangle, authoredOutwardNormal))
    {
        record.planeCoordinateBits = std::bit_cast<std::uint32_t>(plane->coordinate);
        record.contactPlaneFlags = (plane->axis + 1u) |
            (plane->outwardSign < 0.0f ? kRtWorldSurfaceContactNegativeWinding : 0u);
    }
    return record;
}

} // namespace horde::vulkan::raytracing
