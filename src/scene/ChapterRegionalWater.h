#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <string_view>
#include "scene/BellwetherMillrace.generated.h"

namespace horde::scene {
using ChapterWaterPoint = std::array<float,3>; // engine X/Y/Z metres
using ChapterWaterTriangle = std::array<ChapterWaterPoint,3>;
// Shared original surface definition, not collision/support or a save field.
// Source: d37f1638/docs/design/world/dungeons/baseline/shared-coordinate-layout.json
// plus the exact lake outline/headwater/outflow carved by RegionalTerrain.
inline constexpr std::string_view kChapterWaterAtlasSha256 =
    "0a4da29bf8d988145c0f7b3f1c7afaf415a5da9b14c34bbd53bd1e00e3a86e59";
// Finite coverage reaches the prescribed bank crest (carving is full through
// radius 5m). Actual visible width ends at the RT terrain intersection, not at
// the narrower radius-3m flat bed. It must not extend across the outside valley.
inline constexpr float kChapterRiverHalfWidth = 5.f;
inline constexpr float kChapterLakeWorldY = -31.55f; // atlas Up=0 lake datum
inline constexpr std::size_t kChapterLakeWaterTriangles = 14;
inline constexpr std::size_t kChapterOriginalRegionalWaterTriangleCount = 42;
inline constexpr std::size_t kChapterBellwetherMillraceTriangleCount = 18;
inline constexpr std::size_t kChapterRegionalWaterTriangleCount =
    kChapterOriginalRegionalWaterTriangleCount + kChapterBellwetherMillraceTriangleCount;
inline constexpr std::array<std::size_t,4> kChapterRegionalWaterBodyStarts{0,14,38,42};
inline constexpr ChapterWaterPoint ChapterWaterFromAtlas(float east,float north,float up) {
    return {east+78.3f,up-31.55f,126.2f-north};
}
// Atlas East/North. Keep the concave west inlet/notch; no bounding-box lake.
inline constexpr std::array<std::array<float,2>,16> kChapterLakeOutline{{
    {30,-240},{68,-246},{104,-262},{148,-300},{166,-349},{148,-400},
    {103,-444},{50,-459},{0,-448},{-38,-415},{-61,-365},{-59,-310},
    {-30,-274},{-8,-285},{2,-267},{-5,-252}}};
// First ten samples are the pinned atlas river_surface. Last three are the
// existing RegionalTerrain headwater extension, not a new tributary/pond.
inline constexpr std::array<ChapterWaterPoint,13> kChapterRiverAtlas{{
    {30,-240,0},{44,-170,.8f},{60,-110,2.3f},{62,-80,3},{58,-25,6},
    {68,25,10},{68,65,14},{78,105,20},{105,160,29},{135,220,40},
    {170,290,51},{193,360,65},{214,430,79}}};
inline constexpr std::array<ChapterWaterPoint,3> kChapterOutflowAtlas{{
    {0,-448,0},{-28,-478,-.4f},{-55,-510,-1.2f}}};

namespace chapter_water_detail {
template<std::size_t N,class CentreAt,class DirectionAt>
inline void AppendRibbon(std::size_t count,std::array<ChapterWaterTriangle,kChapterRegionalWaterTriangleCount>& out,
    std::size_t& next,float halfWidth,CentreAt centreAt,DirectionAt directionAt) {
    std::array<ChapterWaterPoint,N> left{},right{};
    for(std::size_t i=0;i<count;++i) {
        const auto centre=centreAt(i);
        const auto normal=[&](std::size_t a,std::size_t b) {
            const auto [dx,dz]=directionAt(a,b);
            const float length=std::hypot(dx,dz);
            return std::array<float,2>{-dz/length,dx/length};
        };
        const auto before=normal(i?i-1:0,i?i:1);
        const auto after=normal(i+1<count?i:count-2,i+1<count?i+1:count-1);
        // Shared miter vertices close every bend without separate overlapping
        // segment strips. These fixed gentle bends stay inside the carved bed.
        const float mx=before[0]+after[0],mz=before[1]+after[1];
        const float scale=halfWidth/(mx*before[0]+mz*before[1]);
        left[i]={centre[0]+mx*scale,centre[1],centre[2]+mz*scale};
        right[i]={centre[0]-mx*scale,centre[1],centre[2]-mz*scale};
    }
    for(std::size_t i=0;i+1<count;++i) {
        out[next++]={left[i],right[i+1],right[i]};
        out[next++]={left[i],left[i+1],right[i+1]};
    }
}
template<std::size_t N>
inline void AppendAtlasRibbon(const std::array<ChapterWaterPoint,N>& atlas,
    std::array<ChapterWaterTriangle,kChapterRegionalWaterTriangleCount>& out,std::size_t& next,float halfWidth) {
    AppendRibbon<N>(N,out,next,halfWidth,
        [&](std::size_t i){return ChapterWaterFromAtlas(atlas[i][0],atlas[i][1],atlas[i][2]);},
        [&](std::size_t a,std::size_t b){return std::array<float,2>{atlas[b][0]-atlas[a][0],atlas[a][1]-atlas[b][1]};});
}
template<std::size_t N>
inline void AppendEngineRibbon(const std::array<ChapterWaterPoint,N>& stations,
    std::array<ChapterWaterTriangle,kChapterRegionalWaterTriangleCount>& out,std::size_t& next,float halfWidth) {
    AppendRibbon<N>(N,out,next,halfWidth,
        [&](std::size_t i){return stations[i];},
        [&](std::size_t a,std::size_t b){return std::array<float,2>{stations[b][0]-stations[a][0],stations[b][2]-stations[a][2]};});
}
using DPoint=std::array<double,3>;
inline DPoint Sub(const ChapterWaterPoint& a,const ChapterWaterPoint& b) {
    return {double(a[0])-b[0],double(a[1])-b[1],double(a[2])-b[2]};
}
inline double Dot(const DPoint& a,const DPoint& b) {
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
inline double SegmentDistanceSquared(const ChapterWaterPoint& p,const ChapterWaterPoint& a,const ChapterWaterPoint& b) {
    const auto edge=Sub(b,a),offset=Sub(p,a);
    const double length=Dot(edge,edge);
    const double t=length>0?std::clamp(Dot(offset,edge)/length,0.,1.):0.;
    const DPoint distance{offset[0]-t*edge[0],offset[1]-t*edge[1],offset[2]-t*edge[2]};
    return Dot(distance,distance);
}
inline double TriangleDistanceSquared(const ChapterWaterPoint& p,const ChapterWaterTriangle& t) {
    const auto ab=Sub(t[1],t[0]),ac=Sub(t[2],t[0]),ap=Sub(p,t[0]);
    const double aa=Dot(ab,ab),bb=Dot(ab,ac),cc=Dot(ac,ac);
    const double d=Dot(ab,ap),e=Dot(ac,ap),denominator=aa*cc-bb*bb;
    if(denominator>0) {
        const double u=(cc*d-bb*e)/denominator,v=(aa*e-bb*d)/denominator;
        if(u>=0 && v>=0 && u+v<=1) {
            const DPoint delta{ap[0]-u*ab[0]-v*ac[0],ap[1]-u*ab[1]-v*ac[1],ap[2]-u*ab[2]-v*ac[2]};
            return Dot(delta,delta);
        }
    }
    return std::min({SegmentDistanceSquared(p,t[0],t[1]),SegmentDistanceSquared(p,t[1],t[2]),
                     SegmentDistanceSquared(p,t[2],t[0])});
}
inline std::optional<float> TriangleHeightAt(const ChapterWaterTriangle& t,float x,float z) {
    const auto& [a,b,c]=t;
    const double bx=double(b[0])-a[0],bz=double(b[2])-a[2];
    const double cx=double(c[0])-a[0],cz=double(c[2])-a[2];
    const double px=double(x)-a[0],pz=double(z)-a[2];
    const double area=bx*cz-bz*cx;
    if(area==0.)return std::nullopt;
    const double beta=(px*cz-pz*cx)/area,gamma=(bx*pz-bz*px)/area;
    if(beta<0. || gamma<0. || beta+gamma>1.)return std::nullopt;
    return static_cast<float>(a[1]+beta*(double(b[1])-a[1])+gamma*(double(c[1])-a[1]));
}
} // namespace chapter_water_detail

inline const std::array<ChapterWaterTriangle,kChapterRegionalWaterTriangleCount>& ChapterRegionalWaterTriangles() {
    static const auto triangles=[] {
        std::array<ChapterWaterTriangle,kChapterRegionalWaterTriangleCount> result{};
        // Ear-clipped CCW atlas indices: exactly the original concave polygon,
        // upward engine winding, 36,274 square metres, without a centre fan.
        constexpr std::array<std::array<std::size_t,3>,14> ears{{
            {15,1,0},{15,2,1},{15,3,2},{15,4,3},{15,5,4},{15,6,5},{6,8,7},
            {6,9,8},{6,10,9},{6,11,10},{6,12,11},{6,13,12},{6,14,13},{6,15,14}}};
        std::size_t next=0;
        for(const auto& ear:ears) {
            ChapterWaterTriangle triangle{};
            for(std::size_t i=0;i<3;++i) {
                const auto& p=kChapterLakeOutline[ear[i]];
                triangle[i]=ChapterWaterFromAtlas(p[0],p[1],0);
            }
            result[next++]=triangle;
        }
        chapter_water_detail::AppendAtlasRibbon(kChapterRiverAtlas,result,next,kChapterRiverHalfWidth);
        chapter_water_detail::AppendAtlasRibbon(kChapterOutflowAtlas,result,next,kChapterRiverHalfWidth);
        chapter_water_detail::AppendEngineRibbon(kBellwetherMillraceStations,result,next,
            kBellwetherMillraceChannelWidth*.5f);
        return result;
    }();
    return triangles;
}
struct ChapterWaterBounds { ChapterWaterPoint minimum{},maximum{}; };
// Exact aggregate surface bounds: lake, main river including headwaters, outflow.
// Broadphase admission may expand these by its own documented ray tolerance.
inline const std::array<ChapterWaterBounds,3>& ChapterRegionalWaterBounds() {
    static const auto bounds=[] {
        std::array<ChapterWaterBounds,3> result{};
        constexpr auto& starts=kChapterRegionalWaterBodyStarts;
        const auto& triangles=ChapterRegionalWaterTriangles();
        for(std::size_t body=0;body<3;++body) {
            result[body]={triangles[starts[body]][0],triangles[starts[body]][0]};
            for(std::size_t i=starts[body];i<starts[body+1];++i)for(const auto& p:triangles[i])
                for(std::size_t axis=0;axis<3;++axis) {
                    result[body].minimum[axis]=std::min(result[body].minimum[axis],p[axis]);
                    result[body].maximum[axis]=std::max(result[body].maximum[axis],p[axis]);
                }
        }
        return result;
    }();
    return bounds;
}

// The Bellwether millrace is a separate CPU range. It is intentionally absent
// from the three legacy broadphase bounds above; its full face bounds fit the
// existing main-river shader box and are checked in ChapterRegionalWaterTests.
inline const ChapterWaterBounds& ChapterBellwetherMillraceBounds() {
    static const auto bounds=[] {
        const auto& triangles=ChapterRegionalWaterTriangles();
        ChapterWaterBounds result{triangles[kChapterOriginalRegionalWaterTriangleCount][0],
                                  triangles[kChapterOriginalRegionalWaterTriangleCount][0]};
        for(std::size_t i=kChapterOriginalRegionalWaterTriangleCount;i<triangles.size();++i)
            for(const auto& p:triangles[i])for(std::size_t axis=0;axis<3;++axis) {
                result.minimum[axis]=std::min(result.minimum[axis],p[axis]);
                result.maximum[axis]=std::max(result.maximum[axis],p[axis]);
            }
        return result;
    }();
    return bounds;
}

// Vertical intersection against the same finite, sloped faces uploaded for RT.
inline std::optional<float> ChapterRegionalWaterSurfaceAt(float x,float z) {
    if(!std::isfinite(x) || !std::isfinite(z))return std::nullopt;
    const auto& bounds=ChapterRegionalWaterBounds();
    const auto& triangles=ChapterRegionalWaterTriangles();
    std::optional<float> surface;
    for(std::size_t body=0;body<bounds.size();++body) {
        const auto& box=bounds[body];
        if(x<box.minimum[0] || x>box.maximum[0] || z<box.minimum[2] || z>box.maximum[2])continue;
        for(std::size_t i=kChapterRegionalWaterBodyStarts[body];i<kChapterRegionalWaterBodyStarts[body+1];++i) {
            if(const auto y=chapter_water_detail::TriangleHeightAt(triangles[i],x,z);
                y && (!surface || *y>*surface))surface=*y;
        }
    }
    const auto& millrace=ChapterBellwetherMillraceBounds();
    if(x>=millrace.minimum[0] && x<=millrace.maximum[0] && z>=millrace.minimum[2] && z<=millrace.maximum[2])
        for(std::size_t i=kChapterOriginalRegionalWaterTriangleCount;i<triangles.size();++i) {
            if(const auto y=chapter_water_detail::TriangleHeightAt(triangles[i],x,z);
                y && (!surface || *y>*surface))surface=*y;
        }
    return surface;
}

// Euclidean distance to the actual triangulated faces (including shore edges),
// not lake AABB, terrain depression, infinite river line or tomb runoff.
// Does not establish renderer residency/visibility or automatically enable audio.
inline float ChapterRegionalWaterDistanceSquared(float x,float y,float z) {
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z))return std::numeric_limits<float>::infinity();
    const ChapterWaterPoint p{x,y,z};
    double nearest=std::numeric_limits<double>::infinity();
    for(const auto& t:ChapterRegionalWaterTriangles())
        nearest=std::min(nearest,chapter_water_detail::TriangleDistanceSquared(p,t));
    return static_cast<float>(nearest);
}
inline bool IsNearChapterRegionalWater(float x,float y,float z,float maximumDistance) {
    if(!std::isfinite(maximumDistance)||maximumDistance<0)return false;
    const float distance=ChapterRegionalWaterDistanceSquared(x,y,z);
    return std::isfinite(distance) && double(distance)<=double(maximumDistance)*maximumDistance;
}
} // namespace horde::scene
