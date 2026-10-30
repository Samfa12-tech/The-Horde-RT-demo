#pragma once
#include "gameplay/ShowcaseRoute.h"
#include "scene/ShowcaseOverheadGeometry.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>
#include <utility>

namespace horde::scene {
using TerrainPoint = std::array<float,3>;

// Reserved occupied interior for EXTERIOR terrain admission, not a replacement
// for authored tomb collision, niches or overhead geometry. Roof/shaft solids
// above this volume retain their separate owners. No renderer mask is changed.
inline bool ExteriorTriangleEntersOccupiedTomb(const std::array<TerrainPoint,3>& triangle,
                                               const horde::gameplay::RouteRect& room) {
    constexpr float boundaryInset=.005f;
    const TerrainPoint minimum{room.minX+boundaryInset,
        horde::gameplay::kRouteFloorWorldY+boundaryInset,room.minZ+boundaryInset};
    const TerrainPoint maximum{room.maxX-boundaryInset,
        kShowcaseRouteCeilingWorldY-boundaryInset,room.maxZ-boundaryInset};
    std::array<TerrainPoint,16> polygon{},next{};
    std::size_t count=3;
    for(std::size_t i=0;i<count;++i) {
        polygon[i]=triangle[i];
        for(float coordinate:polygon[i]) if(!std::isfinite(coordinate)) return true;
    }
    // Clip the actual triangle against all six interior planes. Vertex-only or
    // sparse barycentric probes can miss a thin diagonal crossing a passage.
    for(std::size_t axis=0;axis<3;++axis) for(int side=0;side<2;++side) {
        const float limit=side==0?minimum[axis]:maximum[axis];
        std::size_t written=0;
        for(std::size_t i=0;i<count;++i) {
            const auto a=polygon[i],b=polygon[(i+1)%count];
            const bool aInside=side==0?a[axis]>=limit:a[axis]<=limit;
            const bool bInside=side==0?b[axis]>=limit:b[axis]<=limit;
            if(aInside) next[written++]=a;
            if(aInside!=bInside) {
                const float t=(limit-a[axis])/(b[axis]-a[axis]);
                TerrainPoint intersection{};
                for(std::size_t component=0;component<3;++component)
                    intersection[component]=a[component]+t*(b[component]-a[component]);
                next[written++]=intersection;
            }
        }
        if(written==0) return false;
        polygon=next;count=written;
    }
    return count!=0;
}
inline bool ExteriorTriangleEntersOccupiedTomb(const std::array<TerrainPoint,3>& triangle) {
    for(const auto& room:horde::gameplay::kShowcaseWalkableRects)
        if(ExteriorTriangleEntersOccupiedTomb(triangle,room)) return true;
    return false;
}
// Bounded face subtraction for the existing development exterior. Retained
// tomb walls/roof close these interfaces; do not emit a second interior shell.
// This repairs the old bank closures, not Eric's separate landform authoring.
inline bool ExteriorTriangleOutsideTomb(const std::array<TerrainPoint,3>& source,
                                        std::vector<std::array<TerrainPoint,3>>& output) {
    constexpr std::size_t fragmentCapacity=128;
    for(const auto& point:source) for(float coordinate:point)
        if(!std::isfinite(coordinate)) return false;
    if(!ExteriorTriangleEntersOccupiedTomb(source)) { output={source};return true; }
    std::vector<std::array<TerrainPoint,3>> fragments{source},next;
    const auto nondegenerate=[](const auto& triangle) {
        const auto a=triangle[0],b=triangle[1],c=triangle[2];
        const float ux=b[0]-a[0],uy=b[1]-a[1],uz=b[2]-a[2],
                    vx=c[0]-a[0],vy=c[1]-a[1],vz=c[2]-a[2];
        const float x=uy*vz-uz*vy,y=uz*vx-ux*vz,z=ux*vy-uy*vx;
        return x*x+y*y+z*z>1e-10f;
    };
    for(const auto& room:horde::gameplay::kShowcaseWalkableRects) {
        next.clear();
        for(const auto& fragment:fragments) {
            if(!ExteriorTriangleEntersOccupiedTomb(fragment,room)) {
                next.push_back(fragment);continue;
            }
            const TerrainPoint minimum{room.minX,horde::gameplay::kRouteFloorWorldY,room.minZ};
            const TerrainPoint maximum{room.maxX,kShowcaseRouteCeilingWorldY,room.maxZ};
            std::vector<TerrainPoint> inside(fragment.begin(),fragment.end());
            for(std::size_t axis=0;axis<3&&!inside.empty();++axis) for(int side=0;side<2&&!inside.empty();++side) {
                const float limit=side==0?minimum[axis]:maximum[axis];
                std::vector<TerrainPoint> retained,outside;
                for(std::size_t i=0;i<inside.size();++i) {
                    const auto a=inside[i],b=inside[(i+1)%inside.size()];
                    const bool ai=side==0?a[axis]>=limit:a[axis]<=limit;
                    const bool bi=side==0?b[axis]>=limit:b[axis]<=limit;
                    (ai?retained:outside).push_back(a);
                    if(ai!=bi) {
                        const float t=(limit-a[axis])/(b[axis]-a[axis]);
                        TerrainPoint p{};
                        for(std::size_t k=0;k<3;++k) p[k]=a[k]+t*(b[k]-a[k]);
                        p[axis]=limit;retained.push_back(p);outside.push_back(p);
                    }
                }
                for(std::size_t i=1;i+1<outside.size();++i) {
                    std::array<TerrainPoint,3> triangle{outside[0],outside[i],outside[i+1]};
                    if(nondegenerate(triangle)) next.push_back(triangle);
                    if(next.size()>fragmentCapacity) return false;
                }
                inside=std::move(retained);
            }
            // The final inside polygon is illegal exterior geometry and is
            // removed. Its cut boundary meets the existing tomb owner exactly.
        }
        if(next.size()>fragmentCapacity) return false;
        fragments.swap(next);
    }
    output=std::move(fragments);return true;
}
inline bool RetainedTombRoofAt(float x,float z,float& worldY) {
    for(const auto& patch:kShowcaseCeilingPatches) {
        for(std::size_t i=1;i+1<patch.footprint.size();++i) {
            const auto a=patch.footprint[0],b=patch.footprint[i],c=patch.footprint[i+1];
            const float d=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1]);
            if(std::abs(d)<1e-7f) continue;
            const float u=((b[1]-c[1])*(x-c[0])+(c[0]-b[0])*(z-c[1]))/d;
            const float v=((c[1]-a[1])*(x-c[0])+(a[0]-c[0])*(z-c[1]))/d;
            if(u>=0&&v>=0&&u+v<=1) {worldY=patch.bottomY;return true;}
        }
    }
    return false;
}
} // namespace horde::scene
