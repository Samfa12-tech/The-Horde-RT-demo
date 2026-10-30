#pragma once
#include "scene/CorridorStaticDetail.h"
#include "scene/ChapterRegionalWater.h"
#include <bit>
#include <cstdint>
#include <span>

namespace horde::scene {
// Water reflection/transmission queries reach 12m; ordinary secondary moon
// shadows reach another 18m. 40mm includes their bounded origin offsets.
inline constexpr double kCorridorRegionalWaterQueryReach = 30.04;
// This is an additional closure condition, never a replacement for the exact
// retained-face/source-prism certificate and source/owner/generation guards.
// Pass the bounds of EVERY actual omitted registry triangle and the exact
// regional procedural faces that remain in the traced immutable world owner.
inline bool DetailRegionalWaterQueriesSeparated(const DetailBox& actualOmittedBounds,
    std::span<const ChapterWaterTriangle> actualWaterFaces) {
    if(!DetailForestReceiverEnvelopeBlocked(actualOmittedBounds))return false;
    const auto& authoritative=ChapterRegionalWaterTriangles();
    if(actualWaterFaces.size()!=authoritative.size())return false;
    for(std::size_t i=0;i<actualWaterFaces.size();++i) {
        DetailBox water{{INFINITY,INFINITY,INFINITY},{-INFINITY,-INFINITY,-INFINITY}};
        for(std::size_t corner=0;corner<3;++corner)for(std::size_t axis=0;axis<3;++axis) {
            const float value=actualWaterFaces[i][corner][axis];
            if(!std::isfinite(value) || std::bit_cast<std::uint32_t>(value)!=
                std::bit_cast<std::uint32_t>(authoritative[i][corner][axis]))return false;
            water.min[axis]=std::min(water.min[axis],double(value));
            water.max[axis]=std::max(water.max[axis],double(value));
        }
        // Per-face AABBs enclose the complete continuous triangles, not sampled
        // rays/vertices. AABB separation is a conservative lower distance bound.
        double lowerSquared=0;
        for(std::size_t axis=0;axis<3;++axis) {
            const double roundedGap=std::max({0.,water.min[axis]-actualOmittedBounds.max[axis],
                                                 actualOmittedBounds.min[axis]-water.max[axis]});
            // Round each bound down, including subtraction/product/sum error.
            const double gap=std::nextafter(roundedGap,0.);
            const double term=std::nextafter(gap*gap,0.);
            lowerSquared=std::nextafter(lowerSquared+term,0.);
        }
        const double guardedSquared=std::nextafter(kCorridorRegionalWaterQueryReach*kCorridorRegionalWaterQueryReach,INFINITY);
        if(!std::isfinite(lowerSquared) || lowerSquared<=guardedSquared)return false;
    }
    // Primary paths before reaching water retain the old camera->omission
    // blocker proof. From water, no bounded bounce or its moon shadow reaches
    // an omission. Point/area-light shadows have arbitrary range but reverse
    // to the already certified source-prism->omission segments; if a shadow
    // intersects an omitted face it has an earlier retained opaque blocker.
    // This deliberately makes no claim that all regional water is invisible.
    return true;
}
} // namespace horde::scene
