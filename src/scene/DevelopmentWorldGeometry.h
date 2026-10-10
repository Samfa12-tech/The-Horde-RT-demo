#pragma once
#include "gameplay/simulation/DevelopmentWorldRoute.h"
#include "scene/ShowcaseOverheadGeometry.h"
#include <chrono>
#include <vector>

namespace horde::scene
{
inline constexpr OverheadVolume kDevelopmentRouteRoof = RectangularOverhead(
    38.8f+horde::gameplay::simulation::kWorldPlanTranslateX,
    -8+horde::gameplay::simulation::kWorldPlanTranslateZ,
    41.2f+horde::gameplay::simulation::kWorldPlanTranslateX,
    -5+horde::gameplay::simulation::kWorldPlanTranslateZ,1.02f,1.22f);
struct DevelopmentWorldTriangle
{
    std::array<std::array<float,3>,3> points;
    std::uint32_t material = 0, normal = 0;
    horde::gameplay::simulation::WorldZoneId owner = horde::gameplay::simulation::WorldZoneId::TombExterior;
};
struct DevelopmentWorldGeometry
{
    std::vector<DevelopmentWorldTriangle> triangles;
    std::array<std::uint64_t,4> zoneCpuNanoseconds{};
    std::uint64_t preparationCpuNanoseconds = 0;
    std::size_t retainedCpuBytes = 0, peakPreparationCpuBytes = 0;
    bool valid = false;
};
inline bool ValidateDevelopmentWorldGeometry(const DevelopmentWorldGeometry& geometry)
{
    using namespace horde::gameplay::simulation;
    if(geometry.triangles.empty()) return false;
    std::array<bool,kWorldZones.size()> owners{};
    for(const auto& triangle:geometry.triangles) {
        const auto owner=static_cast<std::size_t>(triangle.owner);
        if(owner==0 || owner>=owners.size() || triangle.material>3 || triangle.normal>5) return false;
        owners[owner]=true;
        for(const auto& point:triangle.points) for(float component:point)
            if(!std::isfinite(component)) return false;
        const auto a=triangle.points[0],b=triangle.points[1],c=triangle.points[2];
        const float ux=b[0]-a[0],uy=b[1]-a[1],uz=b[2]-a[2],
            vx=c[0]-a[0],vy=c[1]-a[1],vz=c[2]-a[2];
        const float x=uy*vz-uz*vy,y=uz*vx-ux*vz,z=ux*vy-uy*vx;
        if(x*x+y*y+z*z<1e-10f) return false;
    }
    return owners[1] && owners[2] && owners[3];
}
// Same triangles consumed by the real world BLAS and the native contract tests.
// Staged CPU batches retain their off-camera contributors until final admission;
// no frustum-based residency or simulated GPU cost is introduced here.
inline DevelopmentWorldGeometry PrepareDevelopmentWorldGeometry(bool staged,bool rescueJourney=false)
{
    using namespace horde::gameplay::simulation;
    using Point = std::array<float,3>;
    DevelopmentWorldGeometry out;
    const auto begin=std::chrono::steady_clock::now();
    auto quad=[&](Point a,Point b,Point c,Point d,std::uint32_t material,std::uint32_t normal,WorldZoneId owner) {
        out.triangles.push_back({{a,b,c},material,normal,owner});
        out.triangles.push_back({{a,c,d},material,normal,owner});
    };
    auto box=[&](Point a,Point b,std::uint32_t material,WorldZoneId owner) {
        quad({a[0],a[1],a[2]},{a[0],b[1],a[2]},{b[0],b[1],a[2]},{b[0],a[1],a[2]},material,5,owner);
        quad({b[0],a[1],b[2]},{b[0],b[1],b[2]},{a[0],b[1],b[2]},{a[0],a[1],b[2]},material,4,owner);
        quad({a[0],a[1],b[2]},{a[0],b[1],b[2]},{a[0],b[1],a[2]},{a[0],a[1],a[2]},material,3,owner);
        quad({b[0],a[1],a[2]},{b[0],b[1],a[2]},{b[0],b[1],b[2]},{b[0],a[1],b[2]},material,2,owner);
        quad({a[0],b[1],a[2]},{a[0],b[1],b[2]},{b[0],b[1],b[2]},{b[0],b[1],a[2]},material,0,owner);
        quad({a[0],a[1],b[2]},{a[0],a[1],a[2]},{b[0],a[1],a[2]},{b[0],a[1],b[2]},material,1,owner);
    };
    for (std::size_t zone=1;zone<kWorldZones.size();++zone)
    {
        const auto stageBegin=std::chrono::steady_clock::now();
        const auto owner=static_cast<WorldZoneId>(zone);
        VisitWorldRouteSurfaceTriangles([&](const auto& points,std::size_t segment) {
            if(ZoneForRouteSegment(segment)==owner && (!rescueJourney||segment>=1))
                out.triangles.push_back({points,zone==1?1u:3u,0u,owner});
        });
        VisitWorldRouteTerrainShellTriangles([&](const auto& points,std::size_t segment,std::uint32_t normal) {
            if(ZoneForRouteSegment(segment)==owner && (!rescueJourney||segment>=1))
                out.triangles.push_back({points,zone==1?1u:3u,normal,owner});
        });
        if (zone==1 && !rescueJourney)
        {
            box({39.35f+kWorldPlanTranslateX,horde::gameplay::kRouteFloorWorldY,-7+kWorldPlanTranslateZ},
                {40.65f+kWorldPlanTranslateX,horde::gameplay::kRouteFloorWorldY+.12f,-6+kWorldPlanTranslateZ},0,owner);
            box({38.8f+kWorldPlanTranslateX,1.02f,-8+kWorldPlanTranslateZ},
                {41.2f+kWorldPlanTranslateX,1.22f,-5+kWorldPlanTranslateZ},0,owner);
            box({38.45f+kWorldPlanTranslateX,-.95f,-8+kWorldPlanTranslateZ},
                {38.8f+kWorldPlanTranslateX,1.22f,-5+kWorldPlanTranslateZ},2,owner);
            box({41.2f+kWorldPlanTranslateX,-.95f,-8+kWorldPlanTranslateZ},
                {41.55f+kWorldPlanTranslateX,1.22f,-5+kWorldPlanTranslateZ},2,owner);
        }
        else if (zone!=1)
        {
            for(const auto& obstacle:WorldRouteBlockoutBoxes())
                if(ZoneForRouteSegment(obstacle.segment)==owner)
                    box(obstacle.minimum,obstacle.maximum,obstacle.material,owner);
        }
        if(zone==1 && rescueJourney) {
            const auto& top=WorldRouteSurfaceTriangles();
            for(std::size_t lane=0;lane<6;++lane) {
                const auto a=top[(6+lane)*2].points[0],b=top[(6+lane)*2+1].points[2];
                quad(a,b,{b[0],-5,b[2]},{a[0],-5,a[2]},1u,5u,owner);
            }
        }
        if(zone==3)
        {
            // Bellwether B01..B04 receive the same documented XY translation
            // as F01..F04. They remain closed, real geometry with no support.
            box({153+kWorldPlanTranslateX, -11.15f,116+kWorldPlanTranslateZ},
                {175+kWorldPlanTranslateX,-4.15f,132+kWorldPlanTranslateZ},0,owner);
            box({141+kWorldPlanTranslateX, -2.55f,62+kWorldPlanTranslateZ},
                {159+kWorldPlanTranslateX,9.45f,86+kWorldPlanTranslateZ},0,owner);
            box({101+kWorldPlanTranslateX,-10.75f,125+kWorldPlanTranslateZ},
                {117+kWorldPlanTranslateX,-4.75f,137+kWorldPlanTranslateZ},2,owner);
            box({97+kWorldPlanTranslateX,-6.95f,103+kWorldPlanTranslateZ},
                {113+kWorldPlanTranslateX,-.95f,117+kWorldPlanTranslateZ},0,owner);
        }
        out.zoneCpuNanoseconds[zone]=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-stageBegin).count();
        // Staged preparation owns an actual temporary batch for validation before
        // upload. Compare overlap cost, never invent Vulkan allocation metrics.
        if(staged)
        {
            std::vector<DevelopmentWorldTriangle> batch;
            for(const auto& triangle:out.triangles) if(triangle.owner==owner) batch.push_back(triangle);
            out.peakPreparationCpuBytes=std::max(out.peakPreparationCpuBytes,
                out.triangles.capacity()*sizeof(DevelopmentWorldTriangle)+batch.capacity()*sizeof(DevelopmentWorldTriangle));
        }
    }
    out.valid=ValidateDevelopmentWorldGeometry(out);
    out.retainedCpuBytes=out.triangles.capacity()*sizeof(DevelopmentWorldTriangle);
    out.peakPreparationCpuBytes=std::max(out.peakPreparationCpuBytes,out.retainedCpuBytes);
    out.preparationCpuNanoseconds=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-begin).count();
    return out;
}
} // namespace horde::scene
