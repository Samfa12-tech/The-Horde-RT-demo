#pragma once
#include "gameplay/simulation/DevelopmentWorldRoute.h"
#include "gameplay/traversal/RescueTraversal.h"
#include "scene/RescueBlockoutGeometry.h"
#include <array>
#include <algorithm>
#include <cmath>

namespace horde::gameplay::traversal {
// Reversible development connector. Existing forest/lookout centreline stays
// fixed; this bridge reconciles its isolated WP2 porch with the real tomb roof.
inline constexpr std::array<simulation::WorldRoutePoint,5> kRescueConnector{{
    {-33.7f,-12.8f,2.05f},{-29.0f,-9.0f,2.05f},{-10.0f,-6.0f,2.05f},
    {20.0f,-3.0f,2.05f},{40.0f,0.0f,2.05f}}};
inline constexpr float kConnectorHalfWidth=1.35f;
inline bool OnRescueLanding(float x,float z) {
    return std::isfinite(x)&&std::isfinite(z)&&horde::scene::RescueLandingContains(x,z);
}
inline simulation::WorldRouteProjection ProjectRescueConnector(float x,float z) {
    simulation::WorldRouteProjection out;
    if(!std::isfinite(x)||!std::isfinite(z)) return out;
    for(std::size_t i=0;i+1<kRescueConnector.size();++i) {
        const auto a=kRescueConnector[i],b=kRescueConnector[i+1];
        const float dx=b.x-a.x,dz=b.z-a.z;
        const float t=std::clamp(((x-a.x)*dx+(z-a.z)*dz)/(dx*dx+dz*dz),0.0f,1.0f);
        const float px=a.x+t*dx,pz=a.z+t*dz,d=std::hypot(x-px,z-pz);
        if(d<out.distance) out={true,d,px,pz,a.y+t*(b.y-a.y),i,t};
    }
    return out;
}
template<class Visitor> void VisitRescueConnectorTriangles(Visitor&& visit) {
    using P=std::array<float,3>;
    std::array<std::array<float,2>,kRescueConnector.size()> offsets{};
    const auto normal=[](std::size_t i){auto a=kRescueConnector[i],b=kRescueConnector[i+1];float l=std::hypot(b.x-a.x,b.z-a.z);return std::array<float,2>{-(b.z-a.z)/l,(b.x-a.x)/l};};
    for(std::size_t i=0;i<offsets.size();++i) {
        const auto a=normal(i?i-1:0),b=normal(i+1<offsets.size()?i:i-1);
        const float l=std::hypot(a[0]+b[0],a[1]+b[1]);
        const float x=(a[0]+b[0])/l,z=(a[1]+b[1])/l;
        const float scale=kConnectorHalfWidth/(x*b[0]+z*b[1]); offsets[i]={x*scale,z*scale};
    }
    for(std::size_t i=0;i+1<offsets.size();++i) {
        const auto a=kRescueConnector[i],b=kRescueConnector[i+1]; auto u=offsets[i],v=offsets[i+1];
        const P p{a.x+u[0],a.y,a.z+u[1]},q{b.x+v[0],b.y,b.z+v[1]},r{b.x-v[0],b.y,b.z-v[1]},s{a.x-u[0],a.y,a.z-u[1]};
        visit(std::array<P,3>{p,q,r}); visit(std::array<P,3>{p,r,s});
    }
}
inline simulation::PlayerSupportResolution RescueExteriorSupport(float x,float z) {
    using namespace simulation;
    if(OnRescueLanding(x,z)) return {kUpperSupportWorldY,static_cast<PlayerSupportId>(130),true};
    bool contains=false;
    VisitRescueConnectorTriangles([&](const auto& t) {
        const auto a=t[0],b=t[1],c=t[2];
        const float d=(b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2]);
        const float u=((b[2]-c[2])*(x-c[0])+(c[0]-b[0])*(z-c[2]))/d;
        const float v=((c[2]-a[2])*(x-c[0])+(a[0]-c[0])*(z-c[2]))/d;
        contains=contains || (u>=0 && v>=0 && u+v<=1);
    });
    if(contains) return {kUpperSupportWorldY,static_cast<PlayerSupportId>(131),true};
    return ResolveWorldRouteSupport(x,z);
}
inline bool AtRopeEndpoint(float x,float z,bool exterior) {
    const auto p=exterior?kExteriorLanding:kLowerLanding;
    return std::isfinite(x)&&std::isfinite(z)&&
        std::hypot(x-p.x,z-p.z)<=kRescueApproachRadius;
}
inline bool ValidRescueCapsulePath(const RescueTraversalSnapshot& s) {
    const auto p=s.playerPosition;
    if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z) ||
       p.x<-34.94f+.20f || p.x>-32.46f-.20f || p.z<-16.64f+.20f || p.z>-11.92f-.20f)
        return false;
    // Bounded authored capsule clearance through the front coping; no general
    // 3D physics or sword occlusion claim follows from this route validator.
    if(p.z>=-13.76f-.20f && p.z<=-13.58f+.20f && p.y<2.23f+.04f) return false;
    return p.y>=kLowerSupportWorldY-.001f && p.y<=2.28f+.001f;
}
inline bool CanApproachRopeEndpoint(Vec3 start,float supportWorldY,bool exterior) {
    const Vec3 endpoint=exterior?kExteriorLanding:kLowerLanding;
    const float expectedSupport=exterior?kUpperSupportWorldY:kLowerSupportWorldY;
    if(!std::isfinite(start.x)||!std::isfinite(start.y)||!std::isfinite(start.z)||
       !std::isfinite(supportWorldY)||std::abs(supportWorldY-expectedSupport)>.10f||
       std::hypot(start.x-endpoint.x,start.z-endpoint.z)>kRescueApproachRadius)
        return false;
    // Validate the complete swept capsule centre line, including the current
    // pose, against the shaft/coping envelope and retained upper support mesh.
    // In particular, a nominally nearby player at y=2.05 cannot walk through
    // the front coping just because the endpoint itself is safe.
    constexpr int samples=24;
    for(int i=0;i<=samples;++i) {
        const float t=static_cast<float>(i)/samples;
        RescueTraversalSnapshot pose;
        pose.playerPosition={start.x+(endpoint.x-start.x)*t,expectedSupport,
                             start.z+(endpoint.z-start.z)*t};
        pose.supportWorldY=expectedSupport;
        if(!ValidRescueCapsulePath(pose)) return false;
        if(exterior) {
            const auto retained=RescueExteriorSupport(pose.playerPosition.x,pose.playerPosition.z);
            if(!retained.grounded||std::abs(retained.worldY-expectedSupport)>.10f) return false;
        }
    }
    return true;
}
enum class RescuePrompt : unsigned { None, Climb, Descend, Preparing, Traversing };
} // namespace horde::gameplay::traversal
