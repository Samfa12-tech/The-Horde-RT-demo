#pragma once
#include "vulkan/raytracing/RtSceneAbi.generated.h"
#include "scene/ChapterRegionalWater.h"
#include <type_traits>

namespace horde::vulkan::raytracing {
inline constexpr float kRegionalWaterFlowMetresPerSecond=.45f; // authored phase speed, not fluid simulation
// Binding27 retains the existing ripple vec4 prefix. Only that prefix changes
// per frame; profiles are immutable for this scene/geometry identity.
struct alignas(16) RegionalWaterRenderData {
    std::array<float,4> ripple{};
    std::array<std::uint32_t,4> metadata{}; // x: admitted surface count; yzw reserved zero
    std::array<RtWaterSurfaceGpu,scene::kChapterRegionalWaterTriangleCount> surfaces{};
};
static_assert(sizeof(RegionalWaterRenderData)==4736);
static_assert(offsetof(RegionalWaterRenderData,metadata)==16);
static_assert(offsetof(RegionalWaterRenderData,surfaces)==32);
static_assert(std::is_trivially_copyable_v<RegionalWaterRenderData>);

namespace regional_water_render_detail {
inline void Vertex(RtWaterSurfaceGpu& record,std::size_t i,const scene::ChapterWaterPoint& p,
    float along,const std::array<float,2>& flow,float lateral,float speed) {
    auto& a=i==0?record.aAlong:i==1?record.bAlong:record.cAlong;
    auto& f=i==0?record.aFlow:i==1?record.bFlow:record.cFlow;
    a={p[0],p[1],p[2],along};f={flow[0],flow[1],lateral,speed};
}
inline void Normal(RtWaterSurfaceGpu& record,const scene::ChapterWaterTriangle& t,float kind) {
    const auto a=scene::chapter_water_detail::Sub(t[1],t[0]);
    const auto b=scene::chapter_water_detail::Sub(t[2],t[0]);
    const std::array<double,3> n{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
    const double length=std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);
    record.normalKind={float(n[0]/length),float(n[1]/length),float(n[2]/length),kind};
}
template<std::size_t N>
inline void Ribbon(RegionalWaterRenderData& data,const std::array<scene::ChapterWaterPoint,N>& atlas,
    std::size_t first,float downstreamSign) {
    std::array<float,N> cumulative{};
    std::array<std::array<float,2>,N> flows{};
    for(std::size_t i=1;i<N;++i) {
        const auto delta=scene::chapter_water_detail::Sub(atlas[i],atlas[i-1]);
        cumulative[i]=cumulative[i-1]+float(std::sqrt(scene::chapter_water_detail::Dot(delta,delta)));
    }
    for(std::size_t i=0;i<N;++i) {
        const auto direction=[&](std::size_t a,std::size_t b){
            const float dx=atlas[b][0]-atlas[a][0],dz=atlas[a][1]-atlas[b][1];
            const float length=std::hypot(dx,dz);
            return std::array<float,2>{dx/length,dz/length};
        };
        const auto before=direction(i?i-1:0,i?i:1);
        const auto after=direction(i+1<N?i:N-2,i+1<N?i+1:N-1);
        const float dx=before[0]+after[0],dz=before[1]+after[1],length=std::hypot(dx,dz);
        flows[i]={downstreamSign*dx/length,downstreamSign*dz/length};
    }
    const auto& triangles=scene::ChapterRegionalWaterTriangles();
    for(std::size_t i=0;i+1<N;++i)for(std::size_t half=0;half<2;++half) {
        const std::size_t index=first+i*2+half;
        auto& record=data.surfaces[index];const auto& triangle=triangles[index];
        // Geometry ribbon vertices: L[i],R[i+1],R[i] / L[i],L[i+1],R[i+1].
        const std::array<std::size_t,3> stations=half?std::array<std::size_t,3>{i,i+1,i+1}:
            std::array<std::size_t,3>{i,i+1,i};
        const std::array<float,3> sides=half?std::array<float,3>{1,1,-1}:
            std::array<float,3>{1,-1,-1};
        for(std::size_t v=0;v<3;++v) {
            const auto station=stations[v];
            const float along=downstreamSign<0?cumulative.back()-cumulative[station]:cumulative[station];
            Vertex(record,v,triangle[v],along,flows[station],
                sides[v]*downstreamSign*scene::kChapterRiverHalfWidth,kRegionalWaterFlowMetresPerSecond);
        }
        Normal(record,triangle,1);
    }
}
} // namespace regional_water_render_detail

inline RegionalWaterRenderData MakeRegionalWaterRenderData(bool enabled) {
    RegionalWaterRenderData result{};
    if(!enabled)return result;
    result.metadata[0]=static_cast<std::uint32_t>(result.surfaces.size());
    const auto& triangles=scene::ChapterRegionalWaterTriangles();
    for(std::size_t i=0;i<scene::kChapterLakeWaterTriangles;++i) {
        auto& record=result.surfaces[i];
        // Standing lake uses its actual world XYZ for phase; no invented flow.
        for(std::size_t v=0;v<3;++v)
            regional_water_render_detail::Vertex(record,v,triangles[i][v],0,{0,0},0,0);
        regional_water_render_detail::Normal(record,triangles[i],0);
    }
    regional_water_render_detail::Ribbon(result,scene::kChapterRiverAtlas,14,-1);
    regional_water_render_detail::Ribbon(result,scene::kChapterOutflowAtlas,38,1);
    return result;
}
} // namespace horde::vulkan::raytracing
