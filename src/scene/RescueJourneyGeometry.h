#pragma once
#include "scene/DevelopmentWorldGeometry.h"
#include "scene/RescueBlockoutGeometry.h"
#include "gameplay/traversal/DevelopmentRescueJourney.h"
namespace horde::scene {
struct OutdoorEffectRegionMarker {
    const char* label;
    std::array<float,3> centre;
    bool effectImplemented;
};
// Original solid M/F letter markers. Their triangles measure only blockout
// geometry; they are neither mist/fireflies nor evidence of those effects' cost.
inline constexpr std::array<OutdoorEffectRegionMarker,2> kOutdoorEffectRegionMarkers{{
    {"M: OUTDOOR MIST NOT IMPLEMENTED",{61.0f,.55f,13.0f},false},
    {"F: FIREFLIES NOT IMPLEMENTED",{59.0f,-.55f,28.0f},false}}};
inline constexpr std::size_t kOutdoorEffectMarkerTriangleCount=76;
// Original blockout fitting joins the solver's clear underside attachment to
// the retained cantilever. It is geometry, not a renderer-only rope anchor.
inline constexpr RescueBlockoutBox kRescueAnchorCollar{
    {{-33.74f,3.67f,-15.54f}},{{-33.66f,3.73f,-15.46f}},0u};
inline constexpr std::size_t kRescueAnchorCollarTriangleCount=12;
inline void AppendRescueJourneyGeometry(DevelopmentWorldGeometry& out) {
    const auto preparationBegin=std::chrono::steady_clock::now();
    const auto previousBytes=out.triangles.capacity()*sizeof(DevelopmentWorldTriangle);
    out.triangles.reserve(out.triangles.size()+kRescueBlockoutBoxes.size()*12+12+8+420+kOutdoorEffectMarkerTriangleCount+kRescueAnchorCollarTriangleCount);
    out.peakPreparationCpuBytes=std::max(out.peakPreparationCpuBytes,
        previousBytes+out.triangles.capacity()*sizeof(DevelopmentWorldTriangle));
    using P=std::array<float,3>; using namespace horde::gameplay::simulation;
    auto quad=[&](P a,P b,P c,P d,unsigned m,unsigned n) {
        out.triangles.push_back({{a,b,c},m,n,WorldZoneId::TombExterior});
        out.triangles.push_back({{a,c,d},m,n,WorldZoneId::TombExterior});
    };
    auto box=[&](const RescueBlockoutBox& b) {
        const auto a=b.minimum,c=b.maximum; const auto m=b.materialCode;
        quad({a[0],a[1],a[2]},{a[0],c[1],a[2]},{c[0],c[1],a[2]},{c[0],a[1],a[2]},m,5);
        quad({c[0],a[1],c[2]},{c[0],c[1],c[2]},{a[0],c[1],c[2]},{a[0],a[1],c[2]},m,4);
        quad({a[0],a[1],c[2]},{a[0],c[1],c[2]},{a[0],c[1],a[2]},{a[0],a[1],a[2]},m,3);
        quad({c[0],a[1],a[2]},{c[0],c[1],a[2]},{c[0],c[1],c[2]},{c[0],a[1],c[2]},m,2);
        quad({a[0],c[1],a[2]},{a[0],c[1],c[2]},{c[0],c[1],c[2]},{c[0],c[1],a[2]},m,0);
        quad({a[0],a[1],c[2]},{a[0],a[1],a[2]},{c[0],a[1],a[2]},{c[0],a[1],c[2]},m,1);
    };
    for(const auto& b:kRescueBlockoutBoxes) box(b);
    box(kRescueBlockoutLanding);
    box(kRescueAnchorCollar);
    horde::gameplay::traversal::VisitRescueConnectorTriangles([&](const auto& t) {
        out.triangles.push_back({t,0,0,WorldZoneId::TombExterior});
    });
    // Original faceted silhouettes at three distances. These are a measurement
    // workload, not admitted final forest art or a capacity certification.
    for(unsigned band=0;band<3;++band) for(unsigned i=0;i<5;++i) {
        const float x=44+band*16.0f+(i%2? -6.0f:6.0f), z=7+band*12.0f+i*2.5f;
        const float floor=band==0?1.45f:band==1?-.55f:-.25f;
        box({{{x-.16f,floor,z-.16f}},{{x+.16f,floor+4,z+.16f}},0});
        const float radius=1.2f+band*.2f;
        for(unsigned ring=0;ring<2;++ring) for(unsigned side=0;side<8;++side) {
            const float a=side*.7853981634f,b=(side+1)*.7853981634f;
            const P v{x+radius*std::cos(a),floor+2.3f+ring,z+radius*std::sin(a)};
            const P w{x+radius*std::cos(b),floor+2.3f+ring,z+radius*std::sin(b)};
            const P top{x,floor+4.6f+ring,z};
            out.triangles.push_back({{v,w,top},2,side%2?2u:4u,band==0?WorldZoneId::ForestApproach:WorldZoneId::Lookout});
        }
    }
    // Readable original M and F silhouettes beside (not on) the support route.
    for(std::size_t i=0;i<kOutdoorEffectRegionMarkers.size();++i) {
        const auto p=kOutdoorEffectRegionMarkers[i].centre;const float x=p[0],y=p[1],z=p[2];
        box({{{x-.50f,y,z-.06f}},{{x-.38f,y+1.2f,z+.06f}},2});
        if(i==0) {
            box({{{x+.38f,y,z-.06f}},{{x+.50f,y+1.2f,z+.06f}},2});
            // Two sloping, double-sided solid bars form the M's valley.
            for(float side:{-1.0f,1.0f}) {
                const P a{x+side*.38f,y+1.2f,z-.06f},b{x,y+.55f,z-.06f},
                    c{x,y+.73f,z-.06f},d{x+side*.38f,y+1.38f,z-.06f};
                const auto back=[](P v){v[2]+=.12f;return v;};
                quad(a,b,c,d,2,5);quad(back(d),back(c),back(b),back(a),2,4);
                quad(a,back(a),back(b),b,2,1);quad(d,c,back(c),back(d),2,0);
            }
        } else {
            box({{{x-.38f,y+1.08f,z-.06f}},{{x+.50f,y+1.20f,z+.06f}},2});
            box({{{x-.38f,y+.55f,z-.06f}},{{x+.30f,y+.67f,z+.06f}},2});
        }
    }
    out.preparationCpuNanoseconds+=std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now()-preparationBegin).count();
    out.valid=ValidateDevelopmentWorldGeometry(out);
    out.retainedCpuBytes=out.triangles.capacity()*sizeof(DevelopmentWorldTriangle);
    out.peakPreparationCpuBytes=std::max(out.peakPreparationCpuBytes,out.retainedCpuBytes);
}
// Fixed topology permits a Vulkan UPDATE without changing primitive counts.
// Every vertex is derived from the exact fixed-step loaded rope nodes.
using RescueRopePoint = std::array<float, 3>;
inline RescueRopePoint RopeSubtract(RescueRopePoint a, RescueRopePoint b) {
    return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};
}
inline float RopeDot(RescueRopePoint a, RescueRopePoint b) {
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
inline RescueRopePoint RopeCross(RescueRopePoint a, RescueRopePoint b) {
    return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
}
inline RescueRopePoint RopeUnit(RescueRopePoint p, RescueRopePoint fallback) {
    const float length=std::sqrt(RopeDot(p,p));
    return std::isfinite(length)&&length>1e-6f
        ?RescueRopePoint{p[0]/length,p[1]/length,p[2]/length}:fallback;
}
// Actual outward triangle geometry selects the existing six-direction world
// normal transport. This remains an approximation; no shader/ABI is changed.
inline unsigned RescueRopeTriangleNormalCode(RescueRopePoint a, RescueRopePoint b, RescueRopePoint c) {
    const auto n=RopeCross(RopeSubtract(b,a),RopeSubtract(c,a));
    const float x=std::abs(n[0]),y=std::abs(n[1]),z=std::abs(n[2]);
    if(y>=x && y>=z) return n[1]>=0?0u:1u;
    if(x>=z) return n[0]>=0?2u:3u;
    return n[2]>=0?4u:5u;
}
inline std::vector<RescueRopePoint> RescueRopeTriangleVertices(
    const horde::gameplay::traversal::RescueTraversalSnapshot& rope) {
    constexpr unsigned sides=8; constexpr float radius=.026f;
    std::array<std::array<RescueRopePoint,sides>,horde::gameplay::traversal::kRopeNodeCount> rings{};
    RescueRopePoint basis{1,0,0};
    for(std::size_t i=0;i<rope.ropeNodes.size();++i) {
        const auto p=rope.ropeNodes[i];
        const auto before=rope.ropeNodes[i?i-1:i],after=rope.ropeNodes[i+1<rope.ropeNodes.size()?i+1:i];
        const auto tangent=RopeUnit({after.x-before.x,after.y-before.y,after.z-before.z},{0,-1,0});
        const float projection=RopeDot(basis,tangent);
        auto next=RescueRopePoint{basis[0]-tangent[0]*projection,basis[1]-tangent[1]*projection,basis[2]-tangent[2]*projection};
        if(RopeDot(next,next)<1e-8f) {
            const RescueRopePoint reference=std::abs(tangent[0])<.8f?RescueRopePoint{1,0,0}:RescueRopePoint{0,0,1};
            next=RopeCross(tangent,reference);
        }
        basis=RopeUnit(next,{1,0,0});
        const auto across=RopeUnit(RopeCross(tangent,basis),{0,0,1});
        for(unsigned side=0;side<sides;++side) {
            const float angle=side*.7853981634f,c=std::cos(angle)*radius,s=std::sin(angle)*radius;
            rings[i][side]={p.x+basis[0]*c+across[0]*s,p.y+basis[1]*c+across[1]*s,p.z+basis[2]*c+across[2]*s};
        }
    }
    std::vector<RescueRopePoint> vertices;vertices.reserve((rope.ropeNodes.size()-1)*sides*6);
    for(std::size_t i=0;i+1<rings.size();++i) for(unsigned side=0;side<sides;++side) {
        const auto v=rings[i][side],w=rings[i+1][side],x=rings[i+1][(side+1)%sides],y=rings[i][(side+1)%sides];
        vertices.insert(vertices.end(),{v,x,w,v,y,x});
    }
    return vertices;
}
} // namespace horde::scene
