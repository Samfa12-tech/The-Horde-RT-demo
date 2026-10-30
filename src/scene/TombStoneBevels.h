#pragma once

#include "scene/ShowcaseOverheadGeometry.h"
#include "scene/assets/StaticMeshAsset.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace horde::scene
{
// Original, deterministic stone geometry. Metres; one planar chamfer, no
// subdivision, new material, instance, shader response or change to collision.
inline constexpr float kTombStoneBevel = 0.03f;
struct StoneBevelBox { std::array<float, 3> minimum, maximum; };
inline constexpr auto TombBevelBoxes()
{
    std::array<StoneBevelBox, 12> boxes{{
        {{{-1.20f,-0.95f,-3.55f}},{{-0.78f,0.95f,-3.25f}}},
        {{{0.78f,-0.95f,-3.55f}},{{1.20f,0.95f,-3.25f}}},
        {{{-1.08f,-0.95f,-6.52f}},{{-0.90f,0.82f,-6.28f}}},
        {{{0.90f,-0.95f,-6.52f}},{{1.08f,0.82f,-6.28f}}},
    }};
    constexpr std::array<unsigned, 6> overhead{{0,2,3,4,5,1}};
    for (unsigned i=0; i<overhead.size(); ++i)
    {
        const auto& volume=kShowcaseLowOverheadVolumes[overhead[i]];
        boxes[4+i] = {{{volume.footprint[1][0],volume.bottomY,volume.footprint[1][1]}},
                      {{volume.footprint[3][0],overhead[i]==1 ? kShowcaseRouteCeilingWorldY : volume.topY,
                        volume.footprint[3][1]}}};
    }
    // Closed entry-wall returns replace both the front paper faces and their
    // hidden rear shell. Backtracking therefore sees real outward normals.
    boxes[10]={{{-1.85f,-0.95f,-6.47f}},{{-0.90f,1.35f,-6.40f}}};
    boxes[11]={{{0.90f,-0.95f,-6.47f}},{{1.85f,1.35f,-6.40f}}};
    return boxes;
}

// Closed room corners only, away from niches, grate and shaft openings. The
// 30 mm diagonal stays well inside existing capsule-wall separation.
struct StoneCorner { float x,z,intoX,intoZ; };
inline constexpr std::array<StoneCorner,16> kTombStoneCorners{{
    {-1.85f,-6.4f,1,1},{1.85f,-6.4f,-1,1},
    {-1.2f,-10,1,1},{0,-11.2f,1,1},{4.8f,-8.8f,-1,-1},
    {6,-10,-1,-1},{6,-15.2f,-1,1},{4.8f,-16.4f,-1,1},
    {-8.5f,-18,1,1},{-2.5f,-18,-1,1},{-2.5f,-12.4f,-1,-1},{-8.5f,-12.4f,1,-1},
    {-36.9f,-18.4f,1,1},{-30.5f,-18.4f,-1,1},
    {-30.5f,-12,-1,-1},{-36.9f,-12,1,-1}
}};

namespace stone_bevel
{
using Point=std::array<float,3>;
inline Point Sub(Point a,Point b) { return {a[0]-b[0],a[1]-b[1],a[2]-b[2]}; }
inline float Dot(Point a,Point b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
inline Point Cross(Point a,Point b) { return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}; }
inline Point Unit(Point a) { const float n=std::sqrt(Dot(a,a)); for(auto& v:a) v/=n; return a; }

inline void Triangle(std::vector<assets::StaticRtVertex>& vertices,
                     std::vector<std::uint32_t>& indices,Point a,Point b,Point c,Point inside)
{
    auto n=Cross(Sub(b,a),Sub(c,a));
    if(Dot(n,Sub(a,inside))<0) { std::swap(b,c); for(auto& v:n) v=-v; }
    n=Unit(n);
    // Stable planar mapping at the existing masonry's 0.42 repeats/metre.
    const auto tangent=Unit(std::abs(n[1])<0.9f ? Cross({0,1,0},n) : Cross({0,0,1},n));
    const auto bitangent=Cross(n,tangent);
    for(const auto p:{a,b,c})
    {
        assets::StaticRtVertex v{};
        v.position={p[0],p[1],p[2],1}; v.normal={n[0],n[1],n[2],0};
        v.tangent={tangent[0],tangent[1],tangent[2],1};
        v.uv0={Dot(p,tangent)*0.42f,Dot(p,bitangent)*0.42f,0,0};
        indices.push_back(static_cast<std::uint32_t>(vertices.size())); vertices.push_back(v);
    }
}
inline void Quad(std::vector<assets::StaticRtVertex>& v,std::vector<std::uint32_t>& i,
                 Point a,Point b,Point c,Point d,Point inside)
{ Triangle(v,i,a,b,c,inside); Triangle(v,i,a,c,d,inside); }
} // namespace stone_bevel

inline void AppendChamferedStoneBox(std::vector<assets::StaticRtVertex>& vertices,
                                    std::vector<std::uint32_t>& indices,
                                    const StoneBevelBox& box,float radius=kTombStoneBevel)
{
    using namespace stone_bevel;
    Point centre{}, half{};
    for(unsigned a=0;a<3;++a)
    {
        if(!std::isfinite(radius)||radius<=0||!std::isfinite(box.minimum[a])||
           !std::isfinite(box.maximum[a])||box.maximum[a]-box.minimum[a]<=2*radius)
            throw std::invalid_argument("Stone bevel requires finite bounds and room for its radius.");
        centre[a]=(box.minimum[a]+box.maximum[a])*0.5f;
        half[a]=(box.maximum[a]-box.minimum[a])*0.5f;
    }
    const auto point=[&](Point p) { for(unsigned a=0;a<3;++a) p[a]+=centre[a]; return p; };
    for(unsigned axis=0;axis<3;++axis)
    {
        const unsigned u=(axis+1)%3, w=(axis+2)%3;
        for(float sign:{-1.0f,1.0f})
        {
            std::array<Point,4> p{};
            constexpr std::array<std::array<float,2>,4> signs{{{-1,-1},{1,-1},{1,1},{-1,1}}};
            for(unsigned k=0;k<4;++k)
            { p[k][axis]=sign*half[axis];p[k][u]=signs[k][0]*(half[u]-radius);p[k][w]=signs[k][1]*(half[w]-radius);p[k]=point(p[k]); }
            Quad(vertices,indices,p[0],p[1],p[2],p[3],centre);
        }
        // Four edges parallel to this axis, each a genuine diagonal face.
        for(float su:{-1.0f,1.0f}) for(float sw:{-1.0f,1.0f})
        {
            std::array<Point,4> p{};
            for(unsigned k=0;k<4;++k)
            { p[k][axis]=(k<2?-1:1)*(half[axis]-radius);
              p[k][u]=su*(half[u]-((k==1||k==2)?radius:0));
              p[k][w]=sw*(half[w]-((k==0||k==3)?radius:0));p[k]=point(p[k]); }
            Quad(vertices,indices,p[0],p[1],p[2],p[3],centre);
        }
    }
    for(float x:{-1.0f,1.0f}) for(float y:{-1.0f,1.0f}) for(float z:{-1.0f,1.0f})
    {
        const Point sign{x,y,z}; std::array<Point,3> p{};
        for(unsigned k=0;k<3;++k) { for(unsigned a=0;a<3;++a) p[k][a]=sign[a]*(half[a]-(a==k?0:radius)); p[k]=point(p[k]); }
        Triangle(vertices,indices,p[0],p[1],p[2],centre);
    }
}

inline void AppendTombStoneBevels(std::vector<assets::StaticRtVertex>& vertices,
                                 std::vector<std::uint32_t>& indices)
{
    using namespace stone_bevel;
    for(const auto& box:TombBevelBoxes()) AppendChamferedStoneBox(vertices,indices,box);
    for(const auto& corner:kTombStoneCorners)
    {
        const Point a{corner.x,-0.95f,corner.z}, b{corner.x+corner.intoX*kTombStoneBevel,-0.95f,corner.z},
                    c{corner.x,-0.95f,corner.z+corner.intoZ*kTombStoneBevel};
        auto d=a,e=b,f=c; d[1]=e[1]=f[1]=kShowcaseRouteCeilingWorldY;
        const Point inside{(a[0]+b[0]+c[0])/3,0.2f,(a[2]+b[2]+c[2])/3};
        Triangle(vertices,indices,a,b,c,inside); Triangle(vertices,indices,d,e,f,inside);
        Quad(vertices,indices,a,b,e,d,inside); Quad(vertices,indices,b,c,f,e,inside); Quad(vertices,indices,c,a,d,f,inside);
    }
}
} // namespace horde::scene
