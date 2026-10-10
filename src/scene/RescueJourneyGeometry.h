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
inline void AppendRescueJourneyGeometry(DevelopmentWorldGeometry& out) {
    const auto preparationBegin=std::chrono::steady_clock::now();
    const auto previousBytes=out.triangles.capacity()*sizeof(DevelopmentWorldTriangle);
    out.triangles.reserve(out.triangles.size()+kRescueBlockoutBoxes.size()*12+12+8+420+kOutdoorEffectMarkerTriangleCount);
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
inline std::vector<std::array<float,3>> RescueRopeTriangleVertices(
    const horde::gameplay::traversal::RescueTraversalSnapshot& rope) {
    std::vector<std::array<float,3>> vertices;
    constexpr unsigned sides=8; constexpr float radius=.026f;
    vertices.reserve((rope.ropeNodes.size()-1)*sides*6);
    for(std::size_t i=0;i+1<rope.ropeNodes.size();++i) for(unsigned side=0;side<sides;++side) {
        const float a=side*.7853981634f,b=(side+1)*.7853981634f;
        const auto p=rope.ropeNodes[i],q=rope.ropeNodes[i+1];
        const std::array<float,3> v{p.x+radius*std::cos(a),p.y,p.z+radius*std::sin(a)},
          w{q.x+radius*std::cos(a),q.y,q.z+radius*std::sin(a)},
          x{q.x+radius*std::cos(b),q.y,q.z+radius*std::sin(b)},
          y{p.x+radius*std::cos(b),p.y,p.z+radius*std::sin(b)};
        vertices.insert(vertices.end(),{v,w,x,v,x,y});
    }
    return vertices;
}
} // namespace horde::scene
