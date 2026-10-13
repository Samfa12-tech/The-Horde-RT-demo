#include "scene/visibility/OpaqueSegmentCoverage.h"
#include "scene/visibility/OpaqueStaticQuadAdmission.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <algorithm>

using namespace horde::scene::visibility;
namespace
{
unsigned checks=0, failures=0;
void Check(bool value, const char* message) {
    ++checks; if(!value) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
const std::array<Point3,6u> wall{{{{0,-2,-2}},{{0,2,-2}},{{0,2,2}},
                               {{0,-2,-2}},{{0,2,2}},{{0,-2,2}}}};
const Bounds3 origin{{{1,-.25,-.25}},{{2,.25,.25}}};
const Bounds3 target{{{-2,-.25,-.25}},{{-1,.25,.25}}};
}
int main()
{
    horde::scene::assets::StaticMeshAsset source;
    for(const auto& p:wall) {
        horde::scene::assets::StaticRtVertex vertex;
        vertex.position={{float(p[0]),float(p[1]),float(p[2]),1.f}};source.vertices.push_back(vertex);
    }
    source.indices={0,1,2,3,4,5};source.primitives.push_back({0,0,6,0,0});source.materials.push_back({});
    const std::array<float,12> identity{{1,0,0,0,0,1,0,0,0,0,1,0}};
    Check(bool(AdmitRetainedStaticQuad(source,0,0,identity,0x01,7,7,true)),
          "actual immutable world-baked triangle/material/index records must admit a retained witness");
    for(unsigned mode=0;mode<12;++mode) {
        auto bad=source;auto transform=identity;
        if(mode==0) bad.materials[0].flags=2u;
        if(mode==1) bad.materials[0].transmissionFactor=.01f;
        if(mode==2) bad.materials[0].flags=256u;
        if(mode==3) bad.indices[5]=6000;
        if(mode==4) bad.primitives[0].materialIndex=2;
        if(mode==5) transform[3]=.01f;
        if(mode==6) bad.triangleRegions.assign(2,horde::scene::assets::StaticTriangleRegion::TombDressing);
        if(mode==7) bad.primitives[0].indexCount=3;
        if(mode==8) bad.materials[0].transmissionFactor=std::numeric_limits<float>::quiet_NaN();
        if(mode==10) bad.materials[0].emissiveFactor[0]=.1f;
        if(mode==11) bad.materials[0].emissiveTexture=0;
        Check(!AdmitRetainedStaticQuad(bad,0,0,transform,0x01,7,mode==9?8:7,true),
              "transparent/filtered, corrupt, moving, omitted or stale source records must reject");
    }
    Check(!AdmitRetainedStaticQuad(source,0,0,identity,0x01,7,7,false),"mutable owner must reject");
    const auto witness=OpaqueQuadWitness::Build(wall,0x20,7,true);
    Check(witness.has_value(),"two actual opaque triangles must admit their full quad");
    if(!witness) return 1;
    for(const auto mask : {0x23,0x27,0x35})
        Check(witness->CoversEverySegment(origin,target,mask,7,.01),
              "primary, bounce and shadow masks must retain a shared-world occluder");
    Check(witness->CoversEverySegment(target,origin,0x23,7,.01),"both sides of an opaque quad must block");
    Check(!witness->CoversEverySegment(origin,target,0x04,7,.01),"masked-out geometry cannot be a witness");
    Check(!witness->CoversEverySegment(origin,target,0,7,.01),"unknown ray mask must fail closed");
    Check(!witness->CoversEverySegment(origin,target,0x23,8,.01),"stale source generation must reject");
    Check(!witness->CoversEverySegment(origin,target,0x23,7,0),"missing numeric/spawn guard must reject");
    Check(!witness->CoversEverySegment(origin,origin,0x23,7,.01),"same-side endpoints must not be marked blocked");
    Bounds3 crossing=origin;crossing.minimum[0]=-.1;
    Check(!witness->CoversEverySegment(crossing,target,0x23,7,.01),"an origin crossing the occluder must reject");
    Bounds3 escaping=target;escaping.minimum[1]=-20;escaping.maximum[1]=20;
    Check(!witness->CoversEverySegment(origin,escaping,0x23,7,.01),"partial shadow coverage cannot certify all segments");
    Bounds3 edge{{{-1,3.99,0}},{{-1,3.99,0}}};
    Bounds3 point{{{1,0,0}},{{1,0,0}}};
    Check(!witness->CoversEverySegment(point,edge,0x23,7,.01),"edge grazing must preserve the guard");
    for(double hit : {1.988,1.99,1.992,2.0}) {
        Bounds3 end{{{-1,2*hit,2*hit}},{{-1,2*hit,2*hit}}};
        Check(witness->CoversEverySegment(point,end,0x23,7,.01)==(hit<=1.99),
              "exact diagonal guard boundary and either side must classify correctly");
    }
    Check(!witness->CoversEverySegment(origin,target,0x23,7,1e-30),
          "sub-float-precision guards must fail closed");
    auto largeWall=wall;
    for(auto& p:largeWall) {p[1]*=100000;p[2]*=100000;}
    const auto largeWitness=OpaqueQuadWitness::Build(largeWall,0x20,7,true);
    Check(largeWitness && !largeWitness->CoversEverySegment(origin,target,0x23,7,.01),
          "numeric guard must include source-quad scale, not just ray endpoint scale");
    // A vertex-only wall-box overlap is insufficient: large origins behind a
    // near target can still be covered, while one escaping corner invalidates it.
    Bounds3 wide{{{100,-100,-100}},{{101,100,100}}};
    Bounds3 near{{{-.01,-.01,-.01}},{{-.009,.01,.01}}};
    Check(witness->CoversEverySegment(wide,near,0x23,7,.001),"perspective coverage must use intersections, not projected AABB overlap");
    for(int mode=0;mode<6;++mode) {
        auto broken=wall;
        if(mode==0) broken[5][0]=1e-8; // nonplanar, including a thin defect
        if(mode==1) broken[5]=broken[2]; // missing fourth corner
        if(mode==2) { broken[3]=wall[0];broken[4]=wall[1];broken[5]=wall[5]; } // shared boundary edge
        if(mode==3) broken[1][1]=1.8; // incomplete rectangular coverage
        if(mode==4) broken[0][0]=std::numeric_limits<double>::quiet_NaN();
        if(mode==5) broken[2][2]=std::numeric_limits<double>::infinity();
        Check(!OpaqueQuadWitness::Build(broken,0x20,7,true),"holes, partial, nonplanar or nonfinite triangles must reject");
    }
    Check(!OpaqueQuadWitness::Build(wall,0x20,7,false),"moving or unproven opaque geometry must reject");
    Check(!OpaqueQuadWitness::Build(wall,0,7,true),"invisible geometry cannot occlude");
    Check(!OpaqueQuadWitness::Build(wall,0x20,0,true),"unowned geometry cannot occlude");
    Bounds3 invalid=target;invalid.minimum[2]=4;
    Check(!witness->CoversEverySegment(origin,invalid,0x23,7,.01),"inverted bounds must fail closed");
    // Vertex order, diagonal choice, and all three world axes are independent
    // of orientation. These are actual paired triangle vertices, not boxes.
    for(unsigned axis=0;axis<3;++axis) for(unsigned alternate=0;alternate<2;++alternate) {
        auto rotated=wall;
        if(alternate) rotated={{wall[0],wall[1],wall[5],wall[1],wall[2],wall[5]}};
        for(auto& p:rotated) std::rotate(p.begin(),p.begin()+axis,p.end());
        std::reverse(rotated.begin(),rotated.end());
        auto start=origin,end=target;
        for(auto* b:{&start,&end}) {
            std::rotate(b->minimum.begin(),b->minimum.begin()+axis,b->minimum.end());
            std::rotate(b->maximum.begin(),b->maximum.begin()+axis,b->maximum.end());
        }
        const auto quad=OpaqueQuadWitness::Build(rotated,0x20,7,true);
        Check(quad && quad->CoversEverySegment(start,end,0x37,7,.01),
              "both diagonals and reordered triangles must cover on all world axes");
    }
    // Deterministic independent dense segment oracle: if the box certificate
    // succeeds, every sampled interior segment must hit the inset rectangle.
    // This checks the analytic bound; samples never authorize an unload.
    unsigned seed=0x1937u;
    const auto random=[&]() {seed=seed*1664525u+1013904223u;return double(seed)/4294967296.0;};
    for(unsigned trial=0;trial<160;++trial) {
        Bounds3 a{{{.1,-3+6*random(),-3+6*random()}},{{1,-3+6*random(),-3+6*random()}}};
        Bounds3 b{{{-2,-3+6*random(),-3+6*random()}},{{-.1,-3+6*random(),-3+6*random()}}};
        for(auto* box:{&a,&b}) for(unsigned i=1;i<3;++i)
            if(box->minimum[i]>box->maximum[i]) std::swap(box->minimum[i],box->maximum[i]);
        if(!witness->CoversEverySegment(a,b,0x23,7,.01)) continue;
        bool denseCovered=true;
        for(unsigned sample=0;sample<256;++sample) {
            Point3 p{},q{};
            for(unsigned i=0;i<3;++i) {
                p[i]=a.minimum[i]+(a.maximum[i]-a.minimum[i])*random();
                q[i]=b.minimum[i]+(b.maximum[i]-b.minimum[i])*random();
            }
            const double fraction=p[0]/(p[0]-q[0]);
            for(unsigned i=1;i<3;++i) {
                const double hit=p[i]*(1-fraction)+q[i]*fraction;
                denseCovered=denseCovered && hit>=-1.99 && hit<=1.99;
            }
        }
        Check(denseCovered,"analytic accepted box must contain every independently sampled segment hit");
    }
    std::cout<<checks<<" segment-coverage checks, "<<failures<<" failures\n";
    return failures?1:0;
}
