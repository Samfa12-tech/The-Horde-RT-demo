#pragma once
#include "gameplay/simulation/DevelopmentSupportFixture.h"
#include <array>
#include <cmath>
#include <limits>

namespace horde::gameplay::simulation
{
// Logical identities outlive renderer handles. This is a bounded development
// route, not a campaign save format or general streaming framework.
enum class WorldZoneId : std::uint32_t { Dungeon, TombExterior, ForestApproach, Lookout };
enum class ZoneReadiness : std::uint32_t { Unprepared, Preparing, Ready, Failed };
struct WorldZoneToken
{
    std::uint64_t generation = 0;
    WorldZoneId zone = WorldZoneId::Dungeon;
};
struct WorldZoneDescriptor
{
    WorldZoneId id;
    const char* name;
    SupportSurface surface;
};
inline constexpr std::array<WorldZoneDescriptor, 4> kWorldZones{{
    {WorldZoneId::Dungeon, "resident-dungeon", SupportSurface::Stone},
    {WorldZoneId::TombExterior, "tomb-exterior", SupportSurface::Stone},
    {WorldZoneId::ForestApproach, "forest-approach", SupportSurface::Earth},
    {WorldZoneId::Lookout, "lookout", SupportSurface::Wood}}};
struct WorldRoutePoint { float x, z, y; };
// Planning east/north/up -> runtime (east+152, 139-north, up-31.55).
// Points 2..7 preserve the pinned forest centreline, including the lookout.
// Points 0..1 are a reversible development tomb porch/ramp, not the WP5 shaft.
inline constexpr std::array<WorldRoutePoint, 8> kWorldRoutePoints{{
    {40, -8, kRouteFloorWorldY}, {40, -4, kRouteFloorWorldY},
    {40, 0, 2.05f}, {45, 7, 1.45f}, {58, 13, .55f},
    {56, 28, -.55f}, {70, 33, -.25f}, {87, 49, -1.15f}}};
inline constexpr float kWorldRouteHalfWidth = 1.2f;
inline constexpr float kWorldRouteMaximumStep = .18f;
inline constexpr WorldZoneId ZoneForRouteSegment(std::size_t segment)
{
    return segment < 2 ? WorldZoneId::TombExterior :
           segment < 5 ? WorldZoneId::ForestApproach : WorldZoneId::Lookout;
}
struct WorldRouteProjection
{
    bool valid = false;
    float distance = std::numeric_limits<float>::infinity();
    float x = 0, z = 0, y = kRouteFloorWorldY;
    std::size_t segment = 0;
    float fraction = 0;
};
inline WorldRouteProjection ProjectWorldRoute(float x, float z)
{
    WorldRouteProjection result;
    if (!std::isfinite(x) || !std::isfinite(z)) return result;
    for (std::size_t i = 0; i + 1 < kWorldRoutePoints.size(); ++i)
    {
        const auto a = kWorldRoutePoints[i], b = kWorldRoutePoints[i+1];
        const float dx = b.x-a.x, dz = b.z-a.z;
        const float t = std::clamp(((x-a.x)*dx+(z-a.z)*dz)/(dx*dx+dz*dz), 0.0f, 1.0f);
        const float px=a.x+t*dx, pz=a.z+t*dz;
        const float distance=std::hypot(x-px,z-pz);
        if (distance < result.distance)
            result={true,distance,px,pz,a.y+t*(b.y-a.y),i,t};
    }
    return result;
}
struct WorldRouteSurfaceTriangle { std::array<std::array<float,3>,3> points; std::size_t segment; };
inline const auto& WorldRouteSurfaceTriangles()
{
    static const auto triangles=[] {
        using Point=std::array<float,3>;
        std::array<std::array<float,2>,kWorldRoutePoints.size()> offsets{};
        const auto normal=[](std::size_t segment) {
            const auto a=kWorldRoutePoints[segment],b=kWorldRoutePoints[segment+1];
            const float length=std::hypot(b.x-a.x,b.z-a.z);
            return std::array<float,2>{-(b.z-a.z)/length,(b.x-a.x)/length};
        };
        for(std::size_t i=0;i<kWorldRoutePoints.size();++i)
        {
            const auto previous=normal(i==0?0:i-1),next=normal(i+1==kWorldRoutePoints.size()?i-1:i);
            const float x=previous[0]+next[0],z=previous[1]+next[1],length=std::hypot(x,z);
            const float nx=x/length,nz=z/length;
            const float scale=kWorldRouteHalfWidth/(nx*next[0]+nz*next[1]);
            offsets[i]={nx*scale,nz*scale};
        }
        std::array<WorldRouteSurfaceTriangle,(kWorldRoutePoints.size()-1)*2> result{};
        for(std::size_t i=0;i+1<kWorldRoutePoints.size();++i)
        {
            const auto a=kWorldRoutePoints[i],b=kWorldRoutePoints[i+1];
            const auto u=offsets[i],v=offsets[i+1];
            const Point p{a.x+u[0],a.y,a.z+u[1]},q{b.x+v[0],b.y,b.z+v[1]},
                r{b.x-v[0],b.y,b.z-v[1]},t{a.x-u[0],a.y,a.z-u[1]};
            result[i*2]={{{p,q,r}},i}; result[i*2+1]={{{p,r,t}},i};
        }
        return result;
    }();
    return triangles;
}
template<class Visitor> inline void VisitWorldRouteSurfaceTriangles(Visitor&& visitor)
{
    for(const auto& triangle:WorldRouteSurfaceTriangles()) visitor(triangle.points,triangle.segment);
}
inline PlayerSupportResolution ResolveWorldRouteSupport(float x, float z)
{
    const auto projection=ProjectWorldRoute(x,z);
    if(!projection.valid || projection.distance>kWorldRouteHalfWidth+.0001f)
        return {kRouteFloorWorldY,PlayerSupportId::RouteFloor,false};
    float highest=-std::numeric_limits<float>::infinity(); std::size_t segment=projection.segment;
    // Actual upward triangles shared with the RT world BLAS, including shared mitered joins.
    VisitWorldRouteSurfaceTriangles([&](const auto& triangle,std::size_t i){
        const auto a=triangle[0],b=triangle[1],c=triangle[2];
        const float denominator=(b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2]);
        if(std::abs(denominator)<1e-6f) return;
        const float u=((b[2]-c[2])*(x-c[0])+(c[0]-b[0])*(z-c[2]))/denominator;
        const float v=((c[2]-a[2])*(x-c[0])+(a[0]-c[0])*(z-c[2]))/denominator;
        if(u>=-.00001f && v>=-.00001f && u+v<=1.00001f) {
            const float y=u*a[1]+v*b[1]+(1-u-v)*c[1];
            if(y>highest) { highest=y;segment=i; }
        }
    });
    if(!std::isfinite(highest)) return {kRouteFloorWorldY,PlayerSupportId::RouteFloor,false};
    const bool step=std::abs(x-40)<=.65f && z>=-7 && z<=-6;
    return {highest+(step?.12f:0.0f),step?PlayerSupportId::WorldRouteStep:static_cast<PlayerSupportId>(100+segment),
        true,kWorldZones[static_cast<std::size_t>(ZoneForRouteSegment(segment))].surface};
}
struct WorldRouteState
{
    std::uint64_t generation = 1;
    WorldZoneId current = WorldZoneId::TombExterior;
    std::array<ZoneReadiness,4> readiness{};
    float safeX = kWorldRoutePoints[0].x, safeZ = kWorldRoutePoints[0].z;
    PlayerSupportResolution safeSupport{kRouteFloorWorldY,PlayerSupportId::RouteFloor,false};
    bool blocked = false;
    std::uint64_t rollbackCount = 0;
    void Invalidate(bool retainSafeSide = false)
    {
        if (generation != UINT64_MAX) ++generation;
        readiness.fill(ZoneReadiness::Unprepared);
        readiness[0]=ZoneReadiness::Ready; // existing dungeon adapter
        if(!retainSafeSide) {
            current=WorldZoneId::TombExterior;
            safeX=kWorldRoutePoints[0].x; safeZ=kWorldRoutePoints[0].z;
            safeSupport={kRouteFloorWorldY,PlayerSupportId::RouteFloor,false};
        }
        blocked=false;
    }
    bool Publish(WorldZoneToken token, ZoneReadiness state)
    {
        const auto index=static_cast<std::size_t>(token.zone);
        if (token.generation != generation || token.generation == 0 || index >= readiness.size() ||
            state == ZoneReadiness::Unprepared) return false;
        // Completion cannot resurrect a failed generation or regress ready data.
        if (readiness[index] == ZoneReadiness::Failed || readiness[index] == ZoneReadiness::Ready)
            return readiness[index] == state;
        readiness[index]=state; return true;
    }
};
} // namespace horde::gameplay::simulation
