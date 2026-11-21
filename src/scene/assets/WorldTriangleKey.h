#pragma once

#include "scene/assets/StaticMeshAsset.h"
#include <algorithm>
#include <cmath>

namespace horde::scene::assets {

using WorldTriangleKey=std::array<std::array<long long,3>,3>;

// Asset registration uses a tenth-millimetre grid and retains face winding.
inline WorldTriangleKey KeyWorldTriangle(const StaticMeshAsset& asset,const StaticPrimitiveRecord& primitive,
                                         std::size_t localIndex) {
    const auto corner=[&](std::size_t offset) {
        const auto index=asset.indices[primitive.indexOffset+localIndex+offset];
        const auto& p=asset.vertices[primitive.vertexOffset+index].position;
        constexpr double unitsPerMetre=10000;
        return std::array{std::llround(p[0]*unitsPerMetre),std::llround(p[1]*unitsPerMetre),
                          std::llround(p[2]*unitsPerMetre)};
    };
    const auto a=corner(0),b=corner(1),c=corner(2);
    return std::min({WorldTriangleKey{a,b,c},WorldTriangleKey{b,c,a},WorldTriangleKey{c,a,b}});
}

} // namespace horde::scene::assets
