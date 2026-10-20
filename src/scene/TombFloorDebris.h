#pragma once
#include "gameplay/ShowcaseRoute.h"
#include "scene/TombStoneBevels.h"

namespace horde::scene {
struct StoneFragmentSpec {
    float x,z,width,length,height,yaw;
};
// Original closed, fractured masonry chips. One intentional fall pocket below
// the existing broken right roof shoulder, in front of the opaque collapse wall.
// Chips are cosmetic and do not change floor support, collision or save state.
inline constexpr std::array<StoneFragmentSpec,6> kTombFloorFragments{{
    {1.670f,3.190f,.055f,.075f,.055f,-.30f},
    {1.750f,3.200f,.065f,.080f,.045f, .25f},
    {1.672f,3.260f,.060f,.055f,.030f,-.55f},
    {1.758f,3.265f,.045f,.070f,.060f, .50f},
    {1.670f,3.325f,.035f,.055f,.040f, .70f},
    {1.750f,3.330f,.055f,.050f,.025f,-.30f},
}};
inline void AppendStoneFragment(std::vector<assets::StaticRtVertex>& vertices,
                                std::vector<std::uint32_t>& indices,
                                const StoneFragmentSpec& stone) {
    if(vertices.size()>UINT32_MAX-36u || indices.size()>UINT32_MAX-36u)
        throw std::invalid_argument("Stone fragment exceeds index addressing capacity.");
    for(float value:{stone.x,stone.z,stone.width,stone.length,stone.height,stone.yaw})
        if(!std::isfinite(value))throw std::invalid_argument("Stone fragment must be finite.");
    if(stone.width<=0 || stone.length<=0 || stone.height<=0 ||
       stone.width>.25f || stone.length>.25f || stone.height>.10f)
        throw std::invalid_argument("Stone fragment exceeds the bounded chip dimensions.");
    using namespace stone_bevel;
    constexpr std::array<Point,8> shape{{
        {-.50f,0,-.50f},{.50f,0,-.43f},{.40f,0,.50f},{-.50f,0,.32f},
        {-.34f,.72f,-.36f},{.32f,1,-.32f},{.20f,.55f,.36f},{-.38f,.72f,.21f},
    }};
    std::array<Point,8> p{};
    const float c=std::cos(stone.yaw),s=std::sin(stone.yaw);
    constexpr float floor=gameplay::kRouteFloorWorldY-.001f;
    for(unsigned i=0;i<p.size();++i) {
        const float x=shape[i][0]*stone.width,z=shape[i][2]*stone.length;
        p[i]={stone.x+c*x+s*z,floor+shape[i][1]*stone.height,stone.z-s*x+c*z};
    }
    const Point inside{stone.x,floor+stone.height*.3f,stone.z};
    Quad(vertices,indices,p[0],p[1],p[2],p[3],inside);
    Quad(vertices,indices,p[4],p[5],p[6],p[7],inside);
    for(unsigned i=0;i<4;++i)Quad(vertices,indices,p[i],p[(i+1)%4],p[4+(i+1)%4],p[4+i],inside);
}
inline void AppendTombFloorDebris(std::vector<assets::StaticRtVertex>& vertices,
                                 std::vector<std::uint32_t>& indices) {
    for(const auto& stone:kTombFloorFragments)AppendStoneFragment(vertices,indices,stone);
}
}
