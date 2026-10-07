#include "scene/ShowcaseIndoorDust.h"
#include <iostream>
#include <limits>
#include <cstring>
using namespace horde::scene::atmosphere;
using horde::graphics::DustQuality;
int main(){
    unsigned failures=0; auto check=[&](bool pass,const char* message){if(!pass){++failures;std::cerr<<message<<'\n';}};
    auto zones=horde::scene::kShowcaseIndoorDust;
    check(ValidateDustZones(zones),"authored zones admitted");
    auto bad=zones; bad[0].minimum[0]=bad[0].maximum[0]; check(!ValidateDustZones(bad),"empty bounds rejected");
    bad=zones;bad[0].drift[0]=std::numeric_limits<float>::infinity();check(!ValidateDustZones(bad),"invalid drift rejected");
    bad=zones;bad[1].id=bad[0].id;check(!ValidateDustZones(bad),"duplicate identity rejected");
    std::array<IndoorDustZone,5> overflow{};check(!ValidateDustZones(overflow),"zone capacity enforced");
    for(auto z:zones) for(unsigned n=0;n<32;++n)for(double t:{0.,1.,60.,3600.}){
        auto p=DustPosition(z,n,t);float norm=0;
        for(unsigned a=0;a<3;++a){check(p[a]>z.minimum[a]&&p[a]<z.maximum[a],"motion stays in bounds");
            const float q=(p[a]-(z.minimum[a]+z.maximum[a])*0.5f)/((z.maximum[a]-z.minimum[a])*0.5f);norm+=q*q;}
        if(z.shape==DustZoneShape::Ellipsoid)check(norm<1,"ellipsoid clips conservative support");
        check(p==DustPosition(z,n,t),"motion deterministic");
    }
    DustCamera camera{{0,0.7f,0},0,0,16.0f/9}; DustFrame frame{}, repeat{};DustWork work;
    std::memset(&frame,0x35,sizeof(frame));repeat=frame;
    check(BuildDustFrame(bad,DustQuality::Off,{},-1,frame,work)&&std::memcmp(&frame,&repeat,sizeof(frame))==0 &&
        work.generatedMotes==0&&work.tileReferences==0,"Off skips even invalid data and leaves upload bytes untouched");
    check(BuildDustFrame(zones,DustQuality::Low,camera,1,frame,work)&&work.generatedMotes==16&&work.projectedMotes>0,"Low selected zone and motes");
    DustWork again;check(BuildDustFrame(zones,DustQuality::Low,camera,1,repeat,again)&&
        std::memcmp(&frame,&repeat,sizeof(frame))==0,"paused frame identical");
    for(auto tile:frame.tiles)for(auto index:tile)check(index==kDustEmptyCandidate||index<16,"candidate bounds");
    check(BuildDustFrame(zones,DustQuality::Standard,camera,1,frame,work)&&work.generatedMotes==32,"Standard scales population");
    check(!BuildDustFrame(zones,static_cast<DustQuality>(3),camera,0,frame,work),"invalid quality rejected");
    DustProjection projection;check(ProjectDustSphere({0,0.7f,-2},0.012f,camera,projection),"visible sphere projects");
    check(!ProjectDustSphere({0,0.7f,2},0.012f,camera,projection),"behind camera rejected");
    check(!ProjectDustSphere({0,0.7f,-0.1f},0.012f,camera,projection),"near plane rejected");
    for(float aspect:{0.5f,1.0f,1.77778f,2.5f})for(float yaw:{-3.0f,0.0f,1.2f})for(float pitch:{-0.32f,0.0f,0.28f}) {
        auto c=camera;c.aspect=aspect;c.yaw=yaw;c.pitch=pitch;
        Vec3 f{std::sin(yaw),-0.05f+pitch,-std::cos(yaw)};
        float len=std::sqrt(Dot(f,f));for(float& v:f)v/=len;
        Vec3 center{c.origin[0]+f[0]*2,c.origin[1]+f[1]*2,c.origin[2]+f[2]*2};
        DustProjection projected;check(ProjectDustSphere(center,0.012f,c,projected),"camera rotation and aspect admitted");
        check(projected.left<0.5f&&projected.right>0.5f&&projected.top<0.5f&&projected.bottom>0.5f,
            "conservative sphere contains actual forward-ray UV at all pitch/aspect cases");
    }
    std::array<IndoorDustZone,4> overlaps;for(unsigned i=0;i<4;++i){overlaps[i]=zones[0];overlaps[i].id=4-i;}
    check(BuildDustFrame(overlaps,DustQuality::Standard,camera,0,frame,work)&&work.admittedZones==2&&work.generatedMotes==64,"overlap capped to two stable IDs");
    for(auto tile:frame.tiles)for(auto index:tile)check(index==kDustEmptyCandidate||index<64,"overflow never produces invalid index");
    auto moved=camera;moved.origin[0]=0.1f;BuildDustFrame(zones,DustQuality::Low,moved,1,repeat,again);
    // Compare same seed/quality, independently of overlap fixture above.
    BuildDustFrame(zones,DustQuality::Low,camera,1,frame,work);
    check(repeat.motes[0].positionRadius==frame.motes[0].positionRadius,"camera never translates motes");
    DustFrameCache cache;
    check(cache.Build(zones,DustQuality::Low,camera,1,frame,work)==DustUploadDecision::Changed,"first upload needed");
    check(cache.Build(zones,DustQuality::Low,camera,1,frame,work)==DustUploadDecision::Changed,"failed/uncommitted upload cannot seed reuse");
    cache.Commit(zones,DustQuality::Low,camera,1);
    check(cache.Build(zones,DustQuality::Low,camera,1,frame,work)==DustUploadDecision::Unchanged&&work.generatedMotes==0,"pause reuses uploaded data with no owned updates");
    check(cache.Build(zones,DustQuality::Off,camera,1,frame,work)==DustUploadDecision::Disabled&&work.generatedMotes==0,"Off invalidates safely without generating");
    check(cache.Build(zones,DustQuality::Low,camera,1,frame,work)==DustUploadDecision::Changed,"re-enable repopulates safely");
    cache.Commit(zones,DustQuality::Low,camera,1);cache.Invalidate();
    check(cache.Build(zones,DustQuality::Low,camera,1,frame,work)==DustUploadDecision::Changed,"lifecycle reset never reuses stale storage");
    std::cout<<"IndoorDust failures="<<failures<<'\n'; return failures?1:0;
}
