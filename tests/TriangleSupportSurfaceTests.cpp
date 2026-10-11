#include "gameplay/simulation/TriangleSupportSurface.h"
#include <iostream>

using namespace horde::gameplay::simulation;
using Surface=TriangleSupportSurface;
using P=Surface::Point;
int main()
{
    unsigned failures=0;
    const auto check=[&](bool condition,const char* message){if(!condition){++failures;std::cerr<<message<<'\n';}};
    const auto quad=[](float x0,float x1,float z0,float z1,float y0,float y1) {
        const P a{x0,y0,z0},b{x0,y0,z1},c{x1,y1,z1},d{x1,y1,z0};
        return std::array<Surface::Triangle,2>{{{{a,b,c},SupportSurface::Earth},{{a,c,d},SupportSurface::Earth}}};
    };
    std::string error;
    auto ramp=quad(-2,2,-2,2,0,2);
    auto terrain=Surface::Build(ramp,error);
    check(bool(terrain),"finite upward ramp was rejected");if(!terrain)return 1;
    auto sample=terrain->Query(0,0);
    check(sample.found&&sample.walkable&&std::abs(sample.worldY-1)<1e-6&&sample.surface==SupportSurface::Earth,
          "support must interpolate the rendered plane on its shared diagonal");
    check(terrain->GroundSweep(-1,0,1,0,.5f,.24f),"ordinary ramp movement was rejected");
    check(!terrain->GroundSweep(1.7f,0,2,0,1.85f,.24f),"capsule crossed an unsupported outer edge");
    check(!terrain->Query(3,0).found,"query invented a floor beyond the mesh");
    check(!terrain->Query(0,0,1.1f,2).found,"vertical range accepted a different surface");
    const auto steep=Surface::Build(quad(-2,2,-2,2,0,4),error);
    check(steep&&steep->Query(0,0).found&&!steep->Query(0,0).walkable,"steep bank is not a walkable slope");
    auto reversed=ramp;for(auto& t:reversed)std::swap(t.points[1],t.points[2]);
    const auto underside=Surface::Build(reversed,error);
    check(underside&&!underside->Query(0,0).found,"downward shell became support");
    auto invalid=ramp;invalid[0].points[0][0]=std::numeric_limits<float>::quiet_NaN();
    check(!Surface::Build(invalid,error),"NaN admitted");
    invalid=ramp;invalid[0].points[2]=invalid[0].points[1];
    check(!Surface::Build(invalid,error),"degenerate face admitted");
    check(!Surface::Build({},error),"empty terrain admitted");
    invalid=ramp;invalid[0].surface=static_cast<SupportSurface>(255);
    check(!Surface::Build(invalid,error),"unknown surface classification admitted");
    const std::vector<Surface::Triangle> tooMany(Surface::kMaximumTriangles+1,ramp[0]);
    check(!Surface::Build(tooMany,error),"triangle intake overflow admitted");
    const float nan=std::numeric_limits<float>::quiet_NaN();
    check(!terrain->Query(nan,0).found&&!terrain->Query(0,0,2,1).found&&
          !terrain->Query(0,0,0,nan).found,"invalid query parameters accepted");
    check(!terrain->GroundSweep(0,0,1,0,1,0)&&!terrain->GroundSweep(0,0,1,0,1,.24f,0)&&
          !terrain->GroundSweep(0,0,nan,0,1,.24f),"invalid sweep parameters accepted");
    check(!terrain->GroundSweep(0,0,100,0,1,.24f),"unbounded move accepted");
    // A five-millimetre crack lies between 10cm sample positions. Coverage
    // must be continuous, rather than passing because no probe lands in it.
    std::vector<Surface::Triangle> cracked;
    for(const auto extent:std::array<std::array<float,2>,2>{{{{-2,.053f}},{{.058f,2}}}})
        for(const auto& t:quad(extent[0],extent[1],-2,2,0,0))cracked.push_back(t);
    const auto crack=Surface::Build(cracked,error);
    check(crack&&!crack->GroundSweep(-.3f,0,.3f,0,0,.24f),"thin unsupported crack was skipped by point sampling");
    check(crack&&!crack->GroundSweep(.3f,0,-.3f,0,0,.24f),"reverse sweep skipped thin crack");
    auto sliver=std::vector<Surface::Triangle>();
    for(const auto extent:std::array<std::array<float,3>,3>{{{{-2,.053f,0}},{{.053f,.058f,.3f}},{{.058f,2,0}}}})
        for(const auto& t:quad(extent[0],extent[1],-2,2,extent[2],extent[2]))sliver.push_back(t);
    const auto raisedSliver=Surface::Build(sliver,error);
    check(raisedSliver&&!raisedSliver->GroundSweep(-.3f,0,.3f,0,0,.24f),"thin excessive step was skipped");
    for(auto& triangle:sliver)for(auto& p:triangle.points)if(p[1]>.1f)p[1]=.1f;
    const auto smallStep=Surface::Build(sliver,error);
    check(smallStep&&smallStep->GroundSweep(-.3f,0,.3f,0,0,.24f),"bounded small step was rejected");
    check(terrain->GroundSweep(0,0,0,0,1,.24f),"stationary grounded footprint was rejected");
    std::vector<Surface::Triangle> capacity(Surface::kMaximumSweepIntervals,
        Surface::Triangle{{P{-10,0,-10},P{-10,0,20},P{20,0,-10}},SupportSurface::Earth});
    const auto full=Surface::Build(capacity,error);
    check(full&&full->GroundSweep(-.3f,0,.3f,0,0,.24f),"exact sweep capacity failed");
    capacity.push_back(capacity.front());
    const auto overflow=Surface::Build(capacity,error);
    check(overflow&&!overflow->GroundSweep(-.3f,0,.3f,0,0,.24f),"sweep interval overflow was hidden");
    // Upper nonwalkable geometry cannot be hidden by a valid lower floor.
    auto layered=std::vector<Surface::Triangle>(ramp.begin(),ramp.end());
    const auto upper=quad(-2,2,-2,2,4,8);layered.insert(layered.end(),upper.begin(),upper.end());
    const auto layers=Surface::Build(layered,error);
    check(layers&&!layers->Query(0,0).walkable&&layers->Query(0,0,0,2).walkable,
          "highest candidate or bounded-height selection is inconsistent");
    std::cout<<"Triangle support failures="<<failures<<'\n';return failures?1:0;
}
