#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace horde::scene {
struct DetailBox { std::array<double,3> min, max; };
// See the exact full-face coverage certificate. These are source coordinates,
// not a camera visibility/distance heuristic. Both portal ends remain open.
inline constexpr DetailBox kCorridorDetailSources{{-28.9,-.14,-16.789},{-15.1,1.32,-13.611}};
inline constexpr DetailBox kCorridorDetailCell{{-30.5,double(-.95f),double(-16.8f)},
                                             {-8.5,double(1.35f),double(-13.6f)}};
inline bool DetailSourceSphereInside(const std::array<float,3>& centre,double radius) {
    if(!std::isfinite(radius) || radius<0)return false;
    for(unsigned i=0;i<3;++i)
        if(!std::isfinite(centre[i]) || double(centre[i])-radius<kCorridorDetailSources.min[i] ||
           double(centre[i])+radius>kCorridorDetailSources.max[i])return false;
    return true;
}
// Every source-target segment crosses +Z. If all corner intersections stay
// strictly inside the X ends, its first cell exit is north, roof or floor.
// Ratios are monotone in each independent box coordinate (positive Z span),
// so the 64 corner pairs enclose the continuous boxes, not sampled rays.
// The 1 mm guard exceeds double-rounding error for this metre-scale domain.
inline bool DetailTargetBehindCoveredFaces(const DetailBox& target) {
    for(unsigned i=0;i<3;++i)
        if(!std::isfinite(target.min[i]) || !std::isfinite(target.max[i]) ||
           target.min[i]>target.max[i])return false;
    if(target.min[2]<=kCorridorDetailCell.max[2]+.001)return false;
    for(unsigned s=0;s<8;++s)for(unsigned t=0;t<8;++t) {
        const auto sx=(s&1)?kCorridorDetailSources.max[0]:kCorridorDetailSources.min[0];
        const auto sz=(s&4)?kCorridorDetailSources.max[2]:kCorridorDetailSources.min[2];
        const auto tx=(t&1)?target.max[0]:target.min[0];
        const auto tz=(t&4)?target.max[2]:target.min[2];
        const double f=(kCorridorDetailCell.max[2]-sz)/(tz-sz);
        const double x=sx+f*(tx-sx);
        if(!std::isfinite(x) || x<=kCorridorDetailCell.min[0]+.001 || x>=kCorridorDetailCell.max[0]-.001)return false;
    }
    return true;
}
inline bool DetailForestReceiverEnvelopeBlocked(DetailBox target) {
    // All bounded transport is <=12 m. Include origin offsets; sweep opposite
    // the real 18 m directional moon query before expanding receiver reach.
    constexpr std::array<double,3> moon{{-.180027,.930140,-.320048}};
    for(unsigned i=0;i<3;++i) {
        const double d=-18.0*moon[i];
        target.min[i]+=std::min(0.0,d)-12.02;
        target.max[i]+=std::max(0.0,d)+12.02;
    }
    return DetailTargetBehindCoveredFaces(target);
}
}
