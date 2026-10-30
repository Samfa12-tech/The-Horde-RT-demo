#include "gameplay/simulation/TriangleSupportSurface.h"
#include <iostream>
#include <limits>

using namespace horde::gameplay::simulation;
using Surface=TriangleSupportSurface;
using Point=Surface::Point;
namespace {
unsigned checks=0,failures=0;
void Check(bool value,const char* message) {
    ++checks;if(!value){++failures;std::cerr<<message<<'\n';}
}
std::array<Surface::Triangle,2> Quad(float x0,float x1,float z0,float z1,float y0,float y1) {
    const Point a{x0,y0,z0},b{x0,y0,z1},c{x1,y1,z1},d{x1,y1,z0};
    return {{{{a,b,c},SupportSurface::Wood},{{a,c,d},SupportSurface::Wood}}};
}
}
int main() {
    std::string error;
    const auto ground=Surface::Build(Quad(-3,3,-3,3,0,0),error);
    Check(bool(ground),"ground fixture builds");if(!ground)return 1;
    const auto root=Quad(.053f,.058f,-.5f,.5f,.12f,.12f);
    const auto tagged=Surface::WithStepGeometry(*ground,root,error);
    Check(bool(tagged),"explicit thin root geometry admits");if(!tagged)return 1;
    Check(tagged->StepTriangleCount()==2 && tagged->TriangleCount()==2 &&
        ground->StepTriangleCount()==0 && tagged->Query(.055f,0).worldY==0 &&
        tagged->Query(.055f,0).surface==ground->Query(.055f,0).surface,
        "step admission is immutable and does not change bare-ground/root-height authority");
    Check(tagged->RetainedBytes()>ground->RetainedBytes(),"retained bytes include explicit step geometry");
    for(const auto direction:std::array<std::array<float,2>,2>{{{{-.3f,.3f}},{{.3f,-.3f}}}}) {
        const auto support=tagged->StepSupport(direction[0],0,direction[1],0,0,.18f,.24f);
        Check(support.found && support.walkable && std::abs(support.worldY-.12f)<1e-6f &&
            support.normal[1]>.99f && support.surface==SupportSurface::Wood,
            "continuous swept footprint finds millimetre-wide eligible root in either direction");
        Check(!tagged->StepVolumeClear(direction[0],0,direction[1],0,.001f,1.8f,.24f) &&
            !tagged->NonWalkableVolumeClear(direction[0],0,direction[1],0,.001f,1.8f,.24f) &&
            tagged->UnsteppableVolumeClear(direction[0],0,direction[1],0,.001f,1.8f,.24f),
            "step lane remains solid to legacy/held-prop clearance without becoming an unsteppable prop");
    }
    Check(tagged->StepSupport(.055f,.73f,.055f,.73f,0,.18f,.24f).found &&
        !tagged->StepSupport(.055f,.75f,.055f,.75f,0,.18f,.24f).found,
        "step query includes circular footprint edge contact and excludes its clear side");
    Check(tagged->StepVolumeClear(-.3f,0,.3f,0,.121f,1.8f,.24f),
        "elevated fixed-height cylinder clears below-foot step faces");
    const auto untagged=Surface::WithNonWalkableGeometry(*ground,root,error);
    Check(untagged && !untagged->StepSupport(-.3f,0,.3f,0,0,.18f,.24f).found &&
        !untagged->UnsteppableVolumeClear(-.3f,0,.3f,0,.001f,1.8f,.24f),
        "arbitrary nonwalkable prop tops never become step support");
    const auto both=Surface::WithNonWalkableGeometry(*tagged,root,error);
    const auto later=both?Surface::WithAdditionalNonWalkableGeometry(*both,root,error):nullptr;
    Check(both && later && both->StepTriangleCount()==2 && both->NonWalkableTriangleCount()==2 &&
        later->StepTriangleCount()==2 && later->NonWalkableTriangleCount()==4 &&
        later->StepSupport(-.3f,0,.3f,0,0,.18f,.24f).found,
        "initial/additional nonwalkable admission preserves immutable step ownership");
    const auto reverse=untagged?Surface::WithStepGeometry(*untagged,root,error):nullptr;
    Check(reverse && reverse->StepTriangleCount()==2 && reverse->NonWalkableTriangleCount()==2,
        "step admission preserves preexisting nonwalkable ownership");
    Check(!Surface::WithStepGeometry(*tagged,root,error) && !Surface::WithStepGeometry(*ground,{},error),
        "duplicate and empty step admissions reject");
    auto malformed=root;malformed[0].points[0][1]=std::numeric_limits<float>::quiet_NaN();
    Check(!Surface::WithStepGeometry(*ground,malformed,error) && ground->StepTriangleCount()==0,
        "malformed step intake leaves previous owner unchanged");
    const auto tall=Surface::WithStepGeometry(*ground,Quad(-.2f,.2f,-.5f,.5f,.181f,.181f),error);
    const auto limit=Surface::WithStepGeometry(*ground,Quad(-.2f,.2f,-.5f,.5f,.18f,.18f),error);
    Check(tall && !tall->StepSupport(-.3f,0,.3f,0,0,.18f,.24f).found && limit &&
        limit->StepSupport(-.3f,0,.3f,0,0,.18f,.24f).found,
        "explicit maximum height includes 18cm and rejects a higher flat top");
    const auto steep=Surface::WithStepGeometry(*ground,Quad(-.2f,.2f,-.5f,.5f,0,.4f),error);
    auto downward=root;for(auto& triangle:downward)std::swap(triangle.points[1],triangle.points[2]);
    const auto underside=Surface::WithStepGeometry(*ground,downward,error);
    Check(steep && !steep->StepSupport(-.3f,0,.3f,0,0,.18f,.24f).found &&
        !steep->StepVolumeClear(-.3f,0,.3f,0,.001f,1.8f,.24f) && underside &&
        !underside->StepSupport(-.3f,0,.3f,0,0,.18f,.24f).found &&
        !underside->StepVolumeClear(-.3f,0,.3f,0,.001f,1.8f,.24f),
        "steep and downward faces remain solid but cannot supply step support");
    std::vector<Surface::Triangle> heights;
    for(int i=0;i<12;++i) {
        const float x=-.9f+.15f*i,height=.02f+.01f*i;
        const auto strip=Quad(x,x+.003f,-.3f,.3f,height,height);
        heights.insert(heights.end(),strip.begin(),strip.end());
    }
    const auto many=Surface::WithStepGeometry(*ground,heights,error);
    const auto highest=many?many->StepSupport(-1,0,1,0,0,.18f,.24f):Surface::Sample{};
    Check(many && highest.found && std::abs(highest.worldY-.13f)<1e-6f && highest.testedTriangles>8,
        "step BVH traversal selects the highest touched eligible face across nodes");
    // Step faces over a gap are not a grounded movement certificate. Preserve
    // the established independent continuous nine-probe ground contract.
    std::vector<Surface::Triangle> cracked;
    for(const auto extent:std::array<std::array<float,2>,2>{{{{-3,.053f}},{{.058f,3}}}}) {
        const auto part=Quad(extent[0],extent[1],-3,3,0,0);
        cracked.insert(cracked.end(),part.begin(),part.end());
    }
    const auto hole=Surface::Build(cracked,error);
    const auto overHole=hole?Surface::WithStepGeometry(*hole,root,error):nullptr;
    Check(overHole && overHole->StepSupport(-.3f,0,.3f,0,0,.18f,.24f).found &&
        !overHole->GroundSweep(-.3f,0,.3f,0,0,.24f),
        "explicit step support cannot manufacture continuous ground over an unsupported crack");
    const float nan=std::numeric_limits<float>::quiet_NaN();
    for(const auto args:std::array<std::array<float,7>,8>{{
        {{nan,0,1,0,0,.18f,.24f}},{{0,0,1,0,.2f,.18f,.24f}},{{0,0,1,0,0,.18f,0}},
        {{0,0,1,0,0,.18f,2.1f}},{{0,0,17,0,0,.18f,.24f}},{{0,0,1,0,0,nan,.24f}},
        {{1000001,0,1000001,0,0,.18f,.24f}},{{0,0,1,0,0,.18f,nan}}}}) {
        Check(!tagged->StepSupport(args[0],args[1],args[2],args[3],args[4],args[5],args[6]).found &&
            !tagged->StepVolumeClear(args[0],args[1],args[2],args[3],args[4],args[5],args[6]) &&
            !ground->StepVolumeClear(args[0],args[1],args[2],args[3],args[4],args[5],args[6]),
            "invalid/unbounded step query arguments fail closed even without a step lane");
    }
    const std::vector<Surface::Triangle> fullGround(Surface::kMaximumTriangles-4,Quad(-3,3,-3,3,0,0)[0]);
    const auto large=Surface::Build(fullGround,error);
    const auto capacity=large?Surface::WithStepGeometry(*large,root,error):nullptr;
    const auto exact=capacity?Surface::WithNonWalkableGeometry(*capacity,root,error):nullptr;
    Check(exact && exact->TriangleCount()+exact->StepTriangleCount()+exact->NonWalkableTriangleCount()==Surface::kMaximumTriangles &&
        !Surface::WithAdditionalNonWalkableGeometry(*exact,std::span(root).first(1),error),
        "all immutable geometry lanes share the exact total intake capacity");
    const std::vector<Surface::Triangle> overflow(Surface::kMaximumTriangles,root[0]);
    Check(!Surface::WithStepGeometry(*ground,overflow,error),"step intake cannot exceed the combined capacity");
    const auto fullContact=Surface::WithNonWalkableGeometry(*ground,std::span(overflow).first(Surface::kMaximumTriangles-2),error);
    Check(fullContact && !Surface::WithStepGeometry(*fullContact,std::span(root).first(1),error),
        "existing nonwalkable contact consumes step intake capacity");
    const auto floorSlope=Surface::Build(Quad(-.3f,.3f,-.5f,.5f,-.1f,.1f),error);
    Check(floorSlope && !floorSlope->HorizontalVolumeClear(-.2f,0,.2f,0,.001f,1.8f,.24f) &&
        floorSlope->HorizontalBodyVolumeClear(-.2f,0,.2f,0,0,1.8f,.24f),
        "body floor-band exemption admits a low walkable slope that intersects a flat-bottom cylinder");
    const auto highFloor=Surface::Build(Quad(-.3f,.3f,-.5f,.5f,.17f,.17f),error);
    Check(highFloor && !highFloor->HorizontalBodyVolumeClear(-.2f,0,.2f,0,0,1.8f,.24f),
        "walkable terrain above the permitted lower-body band remains a blocker");
    const auto extensiveSlope=Surface::Build(Quad(-1,1,-.5f,.5f,-.3f,.3f),error);
    Check(extensiveSlope && extensiveSlope->HorizontalBodyVolumeClear(-.2f,0,.2f,0,0,1.8f,.24f),
        "remote high vertices do not reject local shallow-ground contact below the floor band");
    Check(extensiveSlope && !extensiveSlope->HorizontalBodyVolumeClear(-.6f,0,.6f,0,0,1.8f,.24f),
        "the contacted part of a walkable triangle above the floor band still blocks");
    const std::array<Surface::Triangle,2> lowWall{{
        {{Point{0,0,-.5f},Point{0,.1f,-.5f},Point{0,.1f,.5f}}},
        {{Point{0,0,-.5f},Point{0,.1f,.5f},Point{0,0,.5f}}}}};
    const auto vertical=Surface::Build(lowWall,error);
    const auto steepFloor=Surface::Build(Quad(-.05f,.05f,-.5f,.5f,0,.1f),error);
    auto lowRoof=Quad(-.3f,.3f,-.5f,.5f,.1f,.1f);
    for(auto& triangle:lowRoof)std::swap(triangle.points[1],triangle.points[2]);
    const auto downwardFloor=Surface::Build(lowRoof,error);
    Check(vertical && !vertical->HorizontalBodyVolumeClear(-.2f,0,.2f,0,0,1.8f,.24f) &&
        steepFloor && !steepFloor->HorizontalBodyVolumeClear(-.2f,0,.2f,0,0,1.8f,.24f) &&
        downwardFloor && !downwardFloor->HorizontalBodyVolumeClear(-.2f,0,.2f,0,0,1.8f,.24f),
        "vertical/steep/downward terrain faces never receive the walkable-floor exemption");
    const auto ceiling=Surface::Build(Quad(-.3f,.3f,-.5f,.5f,1.5f,1.5f),error);
    Check(ceiling && !ceiling->HorizontalBodyVolumeClear(-.2f,0,.2f,0,0,1.8f,.24f) &&
        ceiling->HorizontalBodyVolumeClear(-.2f,0,.2f,0,0,1.4f,.24f),
        "higher upward terrain face blocks standing headroom but clears a shorter body");
    Check(tagged->HorizontalBodyVolumeClear(-.2f,0,.2f,0,0,1.8f,.24f) &&
        untagged->HorizontalBodyVolumeClear(-.2f,0,.2f,0,0,1.8f,.24f),
        "terrain-only body query intentionally leaves STEP/ordinary prop admission to separate checks");
    for(float height:{0.f,.49f,2.51f,nan})
        Check(!ground->HorizontalBodyVolumeClear(-.2f,0,.2f,0,0,height,.24f),
            "invalid/unbounded body height fails closed");
    Check(!ground->HorizontalBodyVolumeClear(nan,0,.2f,0,0,1.8f,.24f) &&
        !ground->HorizontalBodyVolumeClear(0,0,17,0,0,1.8f,.24f) &&
        !ground->HorizontalBodyVolumeClear(0,0,.2f,0,nan,1.8f,.24f) &&
        !ground->HorizontalBodyVolumeClear(0,0,.2f,0,0,1.8f,0),
        "invalid body coordinates, foot height, sweep and radius fail closed");
    std::cout<<checks<<" character step geometry checks, "<<failures<<" failures\n";
    return failures?1:0;
}
