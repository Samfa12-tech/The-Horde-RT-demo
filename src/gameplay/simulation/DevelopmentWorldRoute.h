#pragma once
#include "gameplay/simulation/DevelopmentSupportFixture.h"
#include <algorithm>
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
// The pinned plan's old blockout transform placed F01 75 m beyond the physical
// rope landing. Apply one translation to its runtime XY plane: X -73.7 m,
// Z -12.8 m. This maps F01 exactly to the real exterior landing and preserves
// every planned inter-landmark distance, heading and elevation. Apply this
// same translation to the future Bellwether shell; do not translate the shaft.
inline constexpr float kWorldPlanTranslateX = -73.7f;
inline constexpr float kWorldPlanTranslateZ = -12.8f;
inline constexpr std::array<WorldRoutePoint, 8> kWorldRoutePoints{{
    {40+kWorldPlanTranslateX, -8+kWorldPlanTranslateZ, kRouteFloorWorldY},
    {40+kWorldPlanTranslateX, -.50f+kWorldPlanTranslateZ, 2.05f},
    {40+kWorldPlanTranslateX, 0+kWorldPlanTranslateZ, 2.05f},
    {45+kWorldPlanTranslateX, 7+kWorldPlanTranslateZ, 1.45f},
    {58+kWorldPlanTranslateX, 13+kWorldPlanTranslateZ, .55f},
    {56+kWorldPlanTranslateX, 28+kWorldPlanTranslateZ, -.55f},
    {70+kWorldPlanTranslateX, 33+kWorldPlanTranslateZ, -.25f},
    {87+kWorldPlanTranslateX, 49+kWorldPlanTranslateZ, -1.15f}}};
inline constexpr float kWorldRouteHalfWidth = 12.0f;
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
        constexpr std::size_t crossLanes=6;
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
        std::array<WorldRouteSurfaceTriangle,(kWorldRoutePoints.size()-1)*crossLanes*2> result{};
        for(std::size_t i=0;i+1<kWorldRoutePoints.size();++i)
        {
            auto a=kWorldRoutePoints[i],b=kWorldRoutePoints[i+1];
            const float dx=b.x-a.x,dz=b.z-a.z,dy=b.y-a.y,length=std::hypot(dx,dz);
            if(i==0) { a.x-=dx/length*.6f;a.z-=dz/length*.6f;a.y-=dy/length*.6f; }
            if(i+2==kWorldRoutePoints.size()) { b.x+=dx/length*.6f;b.z+=dz/length*.6f;b.y+=dy/length*.6f; }
            const auto u=offsets[i],v=offsets[i+1];
            const auto lanePoint=[&](WorldRoutePoint centre,std::array<float,2> normal,
                                     float distance) {
                // Gentle cross-fall rolls into a low outer bank. Every value
                // comes from the rendered mesh, so support cannot float over
                // a different analytical heightfield.
                const float crossHeight=.025f*distance+.0015f*distance*distance;
                const float scale=distance/kWorldRouteHalfWidth;
                return Point{centre.x+normal[0]*scale,centre.y+crossHeight,
                             centre.z+normal[1]*scale};
            };
            for(std::size_t lane=0;lane<crossLanes;++lane) {
                const float left=-kWorldRouteHalfWidth+
                    (2.0f*kWorldRouteHalfWidth*static_cast<float>(lane)/crossLanes);
                const float right=-kWorldRouteHalfWidth+
                    (2.0f*kWorldRouteHalfWidth*static_cast<float>(lane+1)/crossLanes);
                const Point p=lanePoint(a,u,left),q=lanePoint(b,v,left),
                            r=lanePoint(b,v,right),t=lanePoint(a,u,right);
                const std::size_t index=(i*crossLanes+lane)*2;
                result[index]={{{p,q,r}},i}; result[index+1]={{{p,r,t}},i};
            }
        }
        return result;
    }();
    return triangles;
}
struct WorldRouteTerrainTriangle
{
    std::array<std::array<float,3>,3> points;
    std::size_t segment = 0;
    std::uint32_t normal = 0;
};
struct WorldRouteBlockoutBox
{
    std::array<float,3> minimum{};
    std::array<float,3> maximum{};
    std::uint32_t material = 0;
    std::size_t segment = 0;
};
inline const auto& WorldRouteBlockoutBoxes()
{
    static const auto boxes=[] {
        std::array<WorldRouteBlockoutBox,20> result{};
        std::size_t out=0;
        for(std::size_t i=2;i<7;++i) {
            const auto p=kWorldRoutePoints[i];
            for(float side:{-1.0f,1.0f}) {
                const float x=p.x+side*3.5f,z=p.z;
                result[out++]={{{x-.18f,p.y-.3f,z-.18f}},{{x+.18f,p.y+4.0f,z+.18f}},0u,i};
                result[out++]={{{x-1.1f,p.y+2.6f,z-1.1f}},{{x+1.1f,p.y+5.0f,z+1.1f}},2u,i};
            }
        }
        return result;
    }();
    return boxes;
}
// Close the terrain ribbon with a continuous underside, two outer cut banks,
// and end caps. It is a bounded ground volume in the RT mesh, not a zero-thick
// camera-facing sheet. Support is resolved only against the exposed top mesh.
inline const auto& WorldRouteTerrainShellTriangles()
{
    static const auto triangles=[] {
        constexpr std::size_t crossLanes=6;
        constexpr std::size_t segments=kWorldRoutePoints.size()-1;
        constexpr float undersideY=-5.0f;
        const auto& top=WorldRouteSurfaceTriangles();
        std::array<WorldRouteTerrainTriangle,segments*2*crossLanes+segments*4+crossLanes*4> result{};
        std::size_t out=0;
        const auto addQuad=[&](auto a,auto b,auto c,auto d,std::size_t segment,std::uint32_t normal) {
            result[out++]={{{a,b,c}},segment,normal};
            result[out++]={{{a,c,d}},segment,normal};
        };
        // Flat closed underside, split to the same retained top topology.
        for(const auto& triangle:top) {
            auto a=triangle.points;
            for(auto& p:a) p[1]=undersideY;
            // Reverse winding so the exterior bottom faces down.
            result[out++]={{{a[0],a[2],a[1]}},triangle.segment,1u};
        }
        for(std::size_t segment=0;segment<segments;++segment) {
            for(std::size_t lane=0;lane<crossLanes;++lane) {
                const auto first=top[(segment*crossLanes+lane)*2];
                const auto second=top[(segment*crossLanes+lane)*2+1];
                const auto left0=first.points[0],left1=first.points[1];
                const auto right1=first.points[2],right0=second.points[2];
                auto l0=left0,l1=left1,r0=right0,r1=right1;
                l0[1]=l1[1]=r0[1]=r1[1]=undersideY;
                if(lane==0) addQuad(left0,left1,l1,l0,segment,3u);
                if(lane+1==crossLanes) addQuad(right1,right0,r0,r1,segment,2u);
                if(segment==0) addQuad(left0,right0,r0,l0,segment,5u);
                if(segment+1==segments) addQuad(left1,l1,r1,right1,segment,4u);
            }
        }
        return result;
    }();
    return triangles;
}
template<class Visitor> inline void VisitWorldRouteSurfaceTriangles(Visitor&& visitor)
{
    for(const auto& triangle:WorldRouteSurfaceTriangles()) visitor(triangle.points,triangle.segment);
}
template<class Visitor> inline void VisitWorldRouteTerrainShellTriangles(Visitor&& visitor)
{
    for(const auto& triangle:WorldRouteTerrainShellTriangles())
        visitor(triangle.points,triangle.segment,triangle.normal);
}
inline PlayerSupportResolution ResolveWorldRouteSupport(float x, float z)
{
    const auto projection=ProjectWorldRoute(x,z);
    if(!projection.valid)
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
    const bool step=std::abs(x-(40+kWorldPlanTranslateX))<=.65f &&
        z>=-7+kWorldPlanTranslateZ && z<=-6+kWorldPlanTranslateZ;
    return {highest+(step?.12f:0.0f),step?PlayerSupportId::WorldRouteStep:static_cast<PlayerSupportId>(100+segment),
        true,kWorldZones[static_cast<std::size_t>(ZoneForRouteSegment(segment))].surface};
}
// Original low-cost wooded silhouettes used by the combined workload. The
// opaque trunk AABBs are grounded on the same terrain and own their collision.
inline const auto& WorldRouteSilhouetteTrunks() {
    static const auto boxes=[] {
        std::array<WorldRouteBlockoutBox,15> out{};
        for(unsigned band=0;band<3;++band) for(unsigned i=0;i<5;++i) {
            float x=44+band*16.0f+(i%2?-6.0f:6.0f)+kWorldPlanTranslateX;
            float z=7+band*12.0f+i*2.5f+kWorldPlanTranslateZ;
            // Retain wooded banks without blocking the authored approach.
            const auto route=ProjectWorldRoute(x,z);
            if(route.distance<3.0f) {
                const auto a=kWorldRoutePoints[route.segment],b=kWorldRoutePoints[route.segment+1];
                const float length=std::hypot(b.x-a.x,b.z-a.z);
                const float side=(x-route.x)*(-(b.z-a.z))+(z-route.z)*(b.x-a.x)<0?-1.0f:1.0f;
                x=route.x-side*(b.z-a.z)/length*4.5f;
                z=route.z+side*(b.x-a.x)/length*4.5f;
            }
            const auto ground=ResolveWorldRouteSupport(x,z);
            const float y=ground.grounded?ground.worldY:kWorldRoutePoints[band+3].y;
            out[band*5+i]={{{x-.16f,y-.12f,z-.16f}},{{x+.16f,y+4,z+.16f}},0u,band==0?2u:5u};
        }
        return out;
    }(); return boxes;
}
// Bounded capsule sweep used by the shared simulation adapter. The centre and
// eight edge samples query the same top triangles used for RT; this lets the
// broad ground perimeter, rather than an unrelated route-width constant, own
// support and movement rejection. Dynamic actor/prop collisions remain with
// their existing owners.
inline bool WorldRouteTerrainMovementClear(float fromX,float fromZ,float toX,float toZ,
                                           float startingSupportY,float radius)
{
    if(!std::isfinite(fromX)||!std::isfinite(fromZ)||!std::isfinite(toX)||
       !std::isfinite(toZ)||!std::isfinite(startingSupportY)||
       !std::isfinite(radius)||radius<=0.0f) return false;
    const float distance=std::hypot(toX-fromX,toZ-fromZ);
    const int samples=std::max(1,static_cast<int>(std::ceil(distance/.10f)));
    float previousY=startingSupportY;
    for(int sample=0;sample<=samples;++sample) {
        const float t=static_cast<float>(sample)/samples;
        const float x=fromX+(toX-fromX)*t,z=fromZ+(toZ-fromZ)*t;
        const auto centre=ResolveWorldRouteSupport(x,z);
        if(!centre.grounded||std::abs(centre.worldY-previousY)>kWorldRouteMaximumStep+.0001f)
            return false;
        previousY=centre.worldY;
        for(int edge=0;edge<8;++edge) {
            const float angle=static_cast<float>(edge)*.7853981633974483f;
            if(!ResolveWorldRouteSupport(x+std::cos(angle)*radius,
                                         z+std::sin(angle)*radius).grounded)
                return false;
        }
    }
    return true;
}
// Collision uses the exact retained opaque tree-trunk AABBs. Their material-
// 2 crown boxes begin above the player capsule and remain visual contributors.
inline bool WorldRouteBlockoutMovementClear(float fromX,float fromZ,float toX,float toZ,
                                            float supportY,float radius,bool combined=false)
{
    if(!std::isfinite(fromX)||!std::isfinite(fromZ)||!std::isfinite(toX)||
       !std::isfinite(toZ)||!std::isfinite(supportY)||!std::isfinite(radius)||radius<=0)
        return false;
    const auto blocks=[&](const auto& boxes) {
    for(const auto& box:boxes) {
        if(box.material!=0u||box.maximum[1]<=supportY+.001f||
           box.minimum[1]>=supportY+(kShowcaseEyeWorldY-kRouteFloorWorldY)) continue;
        float enter=0.0f,leave=1.0f;
        const auto axis=[&](float from,float to,float minimum,float maximum) {
            const float delta=to-from;
            if(std::abs(delta)<1e-7f) return from>=minimum&&from<=maximum;
            float a=(minimum-from)/delta,b=(maximum-from)/delta;
            if(a>b) std::swap(a,b);
            enter=std::max(enter,a);leave=std::min(leave,b);
            return enter<=leave;
        };
        if(axis(fromX,toX,box.minimum[0]-radius,box.maximum[0]+radius)&&
           axis(fromZ,toZ,box.minimum[2]-radius,box.maximum[2]+radius)) return false;
    }
    return true; };
    return blocks(WorldRouteBlockoutBoxes())&&(!combined||blocks(WorldRouteSilhouetteTrunks()));
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
