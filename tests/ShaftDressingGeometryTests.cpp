#include "scene/ShaftDressingGeometry.h"
#include "scene/ShowcaseOverheadGeometry.h"
#include "gameplay/ShowcaseRoute.h"
#include "gameplay/DevelopmentCheckpoints.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <vector>

namespace {
using Point=horde::scene::DressingPoint;
using Face=horde::scene::OpaqueDressingQuad;
constexpr float pi=3.14159265358979323846f;
constexpr std::array<Point,6> axes{{{0,1,0},{0,-1,0},{1,0,0},{-1,0,0},{0,0,1},{0,0,-1}}};
constexpr std::array<std::array<unsigned,3>,2> triangles{{{0,1,2},{0,2,3}}};
Point Difference(const Point& a,const Point& b) { return {a[0]-b[0],a[1]-b[1],a[2]-b[2]}; }
Point Cross(const Point& a,const Point& b)
{ return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}; }
float Dot(const Point& a,const Point& b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
Point Normalise(Point a) { const float length=std::sqrt(Dot(a,a)); for(auto& v:a) v/=length; return a; }
struct Bounds { Point lo{{1e9f,1e9f,1e9f}},hi{{-1e9f,-1e9f,-1e9f}}; };
Bounds SolidBounds(const std::vector<Face>& faces,std::size_t begin)
{
    Bounds bounds;
    for(std::size_t i=begin;i<begin+6u;++i) for(const auto& p:faces[i].vertices)
        for(unsigned axis=0;axis<3;++axis)
        { bounds.lo[axis]=std::min(bounds.lo[axis],p[axis]); bounds.hi[axis]=std::max(bounds.hi[axis],p[axis]); }
    return bounds;
}
bool Contains(const Bounds& b,const Point& p)
{ for(unsigned a=0;a<3;++a) if(p[a]<b.lo[a]||p[a]>b.hi[a]) return false; return true; }
bool Overlap(const Bounds& a,const Bounds& b)
{
    for(unsigned axis=0;axis<3;++axis)
        if(!(std::min(a.hi[axis],b.hi[axis])-std::max(a.lo[axis],b.lo[axis])>1e-5f)) return false;
    return true;
}
bool ClosedSolid(const std::vector<Face>& faces,std::size_t begin)
{
    // Every actual solid edge must occur twice in opposite directions. This
    // catches open faces as well as the earlier leaf hemisphere regression.
    struct EdgeUse { unsigned count=0; int direction=0; };
    std::map<std::array<Point,2>,EdgeUse> edges;
    for(std::size_t i=begin;i<begin+6u;++i)
    {
        const auto& face=faces[i];
        if(face.normalCode>=axes.size()) return false;
        for(const auto& p:face.vertices) for(const auto v:p) if(!std::isfinite(v)) return false;
        for(const auto& c:triangles)
        {
            const auto n=Cross(Difference(face.vertices[c[1]],face.vertices[c[0]]),
                               Difference(face.vertices[c[2]],face.vertices[c[0]]));
            const float length=std::sqrt(Dot(n,n));
            if(!(length>1e-8f)||Dot(n,axes[face.normalCode])/length<0.75f) return false;
        }
        for(unsigned e=0;e<4;++e)
        {
            auto a=face.vertices[e],b=face.vertices[(e+1u)%4u]; const int direction=a<b?1:-1;
            if(b<a) std::swap(a,b);
            auto& use=edges[{{a,b}}]; ++use.count; use.direction+=direction;
        }
    }
    for(const auto& edge:edges) if(edge.second.count!=2u||edge.second.direction!=0) return false;
    return true;
}
bool IsStem(const std::vector<Face>& faces,std::size_t begin)
{
    // Wall-facing leaves also start with a Back face; identify an actual
    // six-axis box rather than assuming the first normal describes its solid.
    constexpr std::array<unsigned,6> stemNormals{{5,4,3,2,0,1}};
    for(unsigned i=0;i<6;++i) if(faces[begin+i].normalCode!=stemNormals[i]) return false;
    return true;
}
bool CheckSprig(const horde::scene::HangingSprigPlacement& placement)
{
    const auto faces=horde::scene::MakeHangingSprig(placement.attachment,placement.length,placement.phase,placement.leafPlane);
    if(faces.size()!=84u) return false;
    unsigned stems=0,leaves=0; Bounds lastStem;
    for(std::size_t i=0;i<faces.size();i+=6u)
    {
        if(!ClosedSolid(faces,i)) { std::cerr<<"closed solid/winding failed at face "<<i<<'\n'; return false; }
        if(IsStem(faces,i))
        {
            const auto bounds=SolidBounds(faces,i);
            if((stems==0u&&!Contains(bounds,placement.attachment))||(stems>0u&&!Overlap(lastStem,bounds)))
            { std::cerr<<"stem disconnected at segment "<<stems<<'\n'; return false; }
            lastStem=bounds; ++stems;
        }
        else
        {
            // A real leaf root vertex, rather than just its bounding box,
            // must be inside the closed segment it grows from.
            bool attached=false;
            for(const auto& p:faces[i].vertices) attached=Contains(lastStem,p)||attached;
            if(!attached) return false;
            ++leaves;
        }
    }
    return stems==8u&&leaves==6u;
}
float TriangleDistance(const Point& origin,const Point& direction,const Point& a,const Point& b,const Point& c)
{
    const auto e1=Difference(b,a),e2=Difference(c,a),p=Cross(direction,e2);
    const float determinant=Dot(e1,p);
    if(std::abs(determinant)<1e-8f) return 1e9f;
    const auto offset=Difference(origin,a),q=Cross(offset,e1);
    const float u=Dot(offset,p)/determinant,v=Dot(direction,q)/determinant,t=Dot(e2,q)/determinant;
    return u>=0&&v>=0&&u+v<=1&&t>0.002f?t:1e9f;
}
float FaceDistance(const Point& origin,const Point& direction,const Face& face)
{
    float nearest=1e9f;
    for(const auto& c:triangles) nearest=std::min(nearest,TriangleDistance(origin,direction,
        face.vertices[c[0]],face.vertices[c[1]],face.vertices[c[2]]));
    return nearest;
}
float BoxDistance(const Point& origin,const Point& direction,const Bounds& b)
{
    float nearT=0,farT=1e9f;
    for(unsigned axis=0;axis<3;++axis)
    {
        if(std::abs(direction[axis])<1e-8f)
        { if(origin[axis]<b.lo[axis]||origin[axis]>b.hi[axis]) return 1e9f; continue; }
        float a=(b.lo[axis]-origin[axis])/direction[axis],c=(b.hi[axis]-origin[axis])/direction[axis];
        if(a>c) std::swap(a,c);
        nearT=std::max(nearT,a); farT=std::min(farT,c);
    }
    return farT>=nearT&&farT>0.002f?nearT:1e9f;
}
float RoofAndWallDistance(const Point& origin,const Point& direction)
{
    float nearest=1e9f;
    // Actual production ceiling triangles, including patch8's overlap of the
    // left aperture edge. An aperture-only bounds test would miss this roof.
    for(std::size_t i=8;i<=11;++i)
    {
        const auto& patch=horde::scene::kShowcaseCeilingPatches[i]; Face face{};
        for(unsigned c=0;c<4;++c) face.vertices[c]={patch.footprint[c][0],patch.bottomY,patch.footprint[c][1]};
        nearest=std::min(nearest,FaceDistance(origin,direction,face));
    }
    // Exact four shaft solids from PresentableTinyRtScene's world builder.
    constexpr float base=horde::scene::kShowcaseRouteCeilingWorldY-0.02f;
    const float top=horde::scene::kWaterShaftTopWorldY;
    const std::array<Bounds,4> walls{{
        {{{-3.06f,base,-16.26f}},{{-2.90f,top,-14.56f}}},
        {{{-1.58f,base,-16.26f}},{{-1.42f,top,-14.56f}}},
        {{{-2.90f,base,-16.26f}},{{-1.58f,top,-16.10f}}},
        {{{-2.90f,base,-14.72f}},{{-1.58f,top,-14.56f}}}}};
    for(const auto& wall:walls) nearest=std::min(nearest,BoxDistance(origin,direction,wall));
    return nearest;
}
bool ClearOfWater(const Point& origin,const Point& direction,float distance)
{
    // Reject even the enclosing boxes of the three real transparent streams;
    // accepted leaf rays need no inference about transmitted visibility.
    const std::array<Bounds,3> streams{{
        {{{-2.326f,-0.91f,-15.325f}},{{-2.314f,2.16f,-15.195f}}},
        {{{-2.323f,-0.91f,-15.454f}},{{-2.317f,2.16f,-15.426f}}},
        {{{-2.323f,-0.91f,-15.072f}},{{-2.317f,2.16f,-15.048f}}}}};
    for(const auto& stream:streams) if(BoxDistance(origin,direction,stream)<distance) return false;
    return true;
}
struct Camera { Point eye,forward,right,up; float aspect; unsigned width,height; };
Camera ProductionCamera(float seconds,float walk,unsigned width,unsigned height)
{
    // Exact rt_frame.glsl projection with ordinary gait bob/sway, unchanged
    // pitch clamp and dry-side authored parallax route (no camera concessions).
    const float phase=seconds*pi/3,step=seconds*6.2f;
    const float yaw=-pi*0.5f+0.06f*std::sin(phase);
    const float pitch=std::clamp(0.275f+0.005f*std::cos(phase)+std::sin(step)*0.012f*walk,-0.32f,0.28f);
    Camera camera{};
    camera.eye={-1.66f+std::sin(step*0.5f)*0.035f*walk,
                0.70f+std::abs(std::sin(step))*0.035f*walk,-15.35f+0.26f*std::sin(phase)};
    camera.forward=Normalise({std::sin(yaw),-0.05f+pitch,-std::cos(yaw)});
    camera.right=Normalise(Cross(camera.forward,{0,1,0})); camera.up=Normalise(Cross(camera.right,camera.forward));
    camera.aspect=static_cast<float>(width)/height; camera.width=width; camera.height=height;
    return camera;
}
unsigned LeafPixelHits(const std::vector<Face>& leaves,const Camera& camera,
                       const std::vector<Face>& stems={})
{
    std::set<unsigned> pixels;
    for(const auto& face:leaves)
    {
        Point center{};
        for(const auto& p:face.vertices) for(unsigned a=0;a<3;++a) center[a]+=p[a]*0.25f;
        const auto delta=Difference(center,camera.eye); const float depth=Dot(delta,camera.forward);
        if(!(depth>0)) continue;
        const float u=0.5f+0.5f*1.22f*Dot(delta,camera.right)/(depth*camera.aspect);
        const float v=0.5f-0.5f*1.22f*Dot(delta,camera.up)/(depth*0.74f);
        if(u<0||u>=1||v<0||v>=1) continue;
        const int centerX=static_cast<int>(u*camera.width),centerY=static_cast<int>(v*camera.height);
        // Actual pixel-center rays near each leaf centroid must intersect its
        // emitted triangles before opaque roof/wall and outside every stream.
        for(int dy=-2;dy<=2;++dy) for(int dx=-2;dx<=2;++dx)
        {
            const int x=centerX+dx,y=centerY+dy;
            if(x<0||y<0||x>=static_cast<int>(camera.width)||y>=static_cast<int>(camera.height)) continue;
            const float sx=((static_cast<float>(x)+0.5f)/camera.width*2-1)*camera.aspect;
            const float sy=((static_cast<float>(y)+0.5f)/camera.height*2-1)*-0.74f;
            Point direction{};
            for(unsigned a=0;a<3;++a) direction[a]=camera.forward[a]*1.22f+camera.right[a]*sx+camera.up[a]*sy;
            direction=Normalise(direction);
            const float leaf=FaceDistance(camera.eye,direction,face);
            float obstruction=RoofAndWallDistance(camera.eye,direction);
            for(const auto& stem:stems) obstruction=std::min(obstruction,FaceDistance(camera.eye,direction,stem));
            if(leaf<1e8f&&leaf<obstruction-0.001f&&ClearOfWater(camera.eye,direction,leaf))
                pixels.insert(static_cast<unsigned>(y)*camera.width+static_cast<unsigned>(x));
        }
    }
    return static_cast<unsigned>(pixels.size());
}
template<std::size_t N>
std::vector<Face> LeafFaces(const std::array<horde::scene::HangingSprigPlacement,N>& placements)
{
    std::vector<Face> leaves;
    for(const auto& placement:placements)
    {
        const auto faces=horde::scene::MakeHangingSprig(placement.attachment,placement.length,placement.phase,placement.leafPlane);
        for(std::size_t i=0;i<faces.size();i+=6)
            if(!IsStem(faces,i)) { leaves.push_back(faces[i]); leaves.push_back(faces[i+1]); }
    }
    return leaves;
}
std::vector<Face> StemFaces()
{
    std::vector<Face> stems;
    for(const auto& placement:horde::scene::kWaterShaftSprigs)
    {
        const auto faces=horde::scene::MakeHangingSprig(placement.attachment,placement.length,placement.phase);
        for(std::size_t i=0;i<faces.size();i+=6)
            if(IsStem(faces,i)) stems.insert(stems.end(),faces.begin()+i,faces.begin()+i+6);
    }
    return stems;
}
Bounds VolumeBounds(const horde::scene::OverheadVolume& v)
{
    Bounds b;
    for(const auto& p:v.footprint) {
        b.lo[0]=std::min(b.lo[0],p[0]);b.hi[0]=std::max(b.hi[0],p[0]);
        b.lo[2]=std::min(b.lo[2],p[1]);b.hi[2]=std::max(b.hi[2],p[1]);
    }
    b.lo[1]=v.bottomY;b.hi[1]=v.topY;return b;
}
float GridDistance(Point origin,Point direction)
{
    float distance=1e9f;
    for(const auto& bar:horde::scene::kShowcaseSkylightGrid)
        distance=std::min(distance,BoxDistance(origin,direction,VolumeBounds(bar)));
    return distance;
}
bool ImpassableGrid(const std::vector<Bounds>& bars)
{
    using namespace horde::scene;
    for(unsigned axis:{0u,2u}) {
        const float lo=axis==0?kLargeSkylightMinX:kLargeSkylightMinZ;
        const float hi=axis==0?kLargeSkylightMaxX:kLargeSkylightMaxZ;
        std::vector<std::array<float,2>> intervals;
        for(const auto& b:bars) if(b.hi[axis]-b.lo[axis]<.1f) intervals.push_back({b.lo[axis],b.hi[axis]});
        std::sort(intervals.begin(),intervals.end());float previous=lo;
        for(const auto& interval:intervals) {
            if(interval[0]-previous>=2*horde::gameplay::kPlayerCollisionRadius) return false;
            previous=interval[1];
        }
        if(hi-previous>=2*horde::gameplay::kPlayerCollisionRadius) return false;
    }
    return true;
}
bool CheckGridAndCeiling()
{
    using namespace horde::scene;
    std::vector<Bounds> bars;
    for(unsigned i=0;i<kShowcaseSkylightGrid.size();++i) {
        const auto b=VolumeBounds(kShowcaseSkylightGrid[i]);bars.push_back(b);
        for(unsigned a=0;a<3;++a) if(!(b.hi[a]>b.lo[a])) return false;
        if(b.lo[1]!=2.39f||b.hi[1]!=2.43f) return false;
        const unsigned shortAxis=i<6?0u:2u,longAxis=i<6?2u:0u;
        if(std::abs(b.hi[shortAxis]-b.lo[shortAxis]-.035f)>1e-5f) return false;
        const float rimLo=longAxis==0?kLargeSkylightMinX:kLargeSkylightMinZ;
        const float rimHi=longAxis==0?kLargeSkylightMaxX:kLargeSkylightMaxZ;
        if(!(b.lo[longAxis]<rimLo-.019f&&b.hi[longAxis]>rimHi+.019f)) return false;
        Point center{};for(unsigned a=0;a<3;++a) center[a]=(b.lo[a]+b.hi[a])*.5f;
        center[1]=.7f;if(GridDistance(center,{0,1,0})>2) return false;
    }
    if(bars.size()!=13||!ImpassableGrid(bars)) return false;
    auto missing=bars;missing.erase(missing.begin()+2);
    if(ImpassableGrid(missing)) return false; // omitted bar creates body-sized gap
    // Real vertical gap ray remains open. Neither preserved aperture has bars.
    if(GridDistance({-6.55f,.7f,-16.45f},{0,1,0})<1e8f ||
       GridDistance({-2.32f,.7f,-15.26f},{0,1,0})<1e8f ||
       GridDistance({-33.7f,.7f,-15.2f},{0,1,0})<1e8f) return false;
    const auto roofHit=[](Point origin,bool closure) {
        float distance=1e9f;
        for(unsigned i:{1u,2u,3u,4u,22u}) {
            if(i==22&&!closure) continue;Face f{};
            const auto& patch=kShowcaseCeilingPatches[i];
            for(unsigned c=0;c<4;++c) f.vertices[c]={patch.footprint[c][0],patch.bottomY,patch.footprint[c][1]};
            distance=std::min(distance,FaceDistance(origin,{0,1,0},f));
        }
        return distance;
    };
    if(roofHit({0,.7f,-4.3f},false)<1e8f) return false; // original physical hole
    for(unsigned x=0;x<=24;++x) for(unsigned z=0;z<=32;++z)
        if(std::abs(roofHit({-.8f+static_cast<float>(x)*.065f,.7f,
            -5.4f+static_cast<float>(z)*.065f},true)-.65f)>1e-5f) return false;
    const auto& patch=kShowcaseCeilingPatches[22];
    std::array<Point,4> p{};for(unsigned i=0;i<4;++i) p[i]={patch.footprint[i][0],patch.bottomY,patch.footprint[i][1]};
    for(const auto& triangle:triangles) if(Dot(Cross(Difference(p[triangle[1]],p[triangle[0]]),
        Difference(p[triangle[2]],p[triangle[0]])),axes[1])<=0) return false;
    return true;
}
unsigned PanelLeafPixelHits()
{
    const auto* pose=horde::gameplay::FindDevelopmentCheckpoint("layout-c-wall-panel");
    if(!pose) return 0;
    // Current native identification pose, ordinary rt_frame projection. Rays
    // include the actual recessed panel walls/bars and the approach ceiling.
    const Point eye{pose->cameraX,.70f,pose->cameraZ};
    const auto forward=Normalise({std::sin(pose->yaw),-.05f+pose->pitch,-std::cos(pose->yaw)});
    const auto right=Normalise(Cross(forward,{0,1,0})),up=Normalise(Cross(right,forward));
    const auto leaves=LeafFaces(horde::scene::kWallPanelSprigs);
    std::vector<Face> obstructions;
    const auto wall=[&](Point a,Point b,Point c,Point d) { obstructions.push_back({{a,b,c,d},0}); };
    wall({2.05f,-.95f,-8.8f},{2.05f,1.35f,-8.8f},{2.05f,1.35f,-8.35f},{2.05f,-.95f,-8.35f});
    wall({3.10f,-.95f,-8.8f},{3.10f,1.35f,-8.8f},{3.10f,1.35f,-8.35f},{3.10f,-.95f,-8.35f});
    wall({2.05f,-.95f,-8.35f},{2.05f,1.35f,-8.35f},{3.10f,1.35f,-8.35f},{3.10f,-.95f,-8.35f});
    wall({0,1.35f,-8.8f},{0,1.35f,-11.2f},{4.8f,1.35f,-11.2f},{4.8f,1.35f,-8.8f});
    for(const auto& placement:horde::scene::kWallPanelSprigs) {
        const auto faces=horde::scene::MakeHangingSprig(placement.attachment,placement.length,placement.phase,placement.leafPlane);
        for(std::size_t i=0;i<faces.size();i+=6) if(IsStem(faces,i))
            obstructions.insert(obstructions.end(),faces.begin()+i,faces.begin()+i+6);
    }
    std::set<unsigned> hits;
    for(const auto& face:leaves) {
        Point center{};for(const auto& p:face.vertices) for(unsigned a=0;a<3;++a) center[a]+=p[a]*.25f;
        const auto delta=Difference(center,eye);const float depth=Dot(delta,forward);if(depth<=0) continue;
        const float u=.5f+.5f*1.22f*Dot(delta,right)/(depth*(960.0f/540.0f));
        const float v=.5f-.5f*1.22f*Dot(delta,up)/(depth*.74f);
        if(u<0||u>=1||v<0||v>=1) continue;
        const int cx=static_cast<int>(u*960),cy=static_cast<int>(v*540);
        for(int y=cy-2;y<=cy+2;++y) for(int x=cx-2;x<=cx+2;++x) {
            if(x<0||x>=960||y<0||y>=540) continue;
            const float sx=((static_cast<float>(x)+.5f)/960*2-1)*(960.0f/540.0f);
            const float sy=((static_cast<float>(y)+.5f)/540*2-1)*-.74f;
            Point dir{};for(unsigned a=0;a<3;++a) dir[a]=forward[a]*1.22f+right[a]*sx+up[a]*sy;
            dir=Normalise(dir);const float leaf=FaceDistance(eye,dir,face);float opaque=1e9f;
            for(const auto& f:obstructions) opaque=std::min(opaque,FaceDistance(eye,dir,f));
            for(unsigned i=0;i<4;++i) {
                const float bx=2.20f+static_cast<float>(i)*.25f;
                opaque=std::min(opaque,BoxDistance(eye,dir,{{{bx,-.78f,-8.82f}},{{bx+.045f,.82f,-8.76f}}}));
            }
            if(leaf<opaque-.001f) hits.insert(static_cast<unsigned>(y*960+x));
        }
    }
    return static_cast<unsigned>(hits.size());
}
} // namespace
int main()
{
    bool passed=true; std::size_t totalFaces=0;
    for(const auto& placement:horde::scene::kWaterShaftSprigs)
    {
        passed=CheckSprig(placement)&&passed;
        totalFaces+=horde::scene::MakeHangingSprig(placement.attachment,placement.length,placement.phase).size();
        passed=placement.attachment[1]==5.58f&&passed;
    }
    // Four Vec3 vertices, six uint indices and two uint surface codes per quad:
    // exact unchanged runtime geometry allocation, not just an upper ceiling.
    const auto meshBytes=totalFaces*(4u*3u*sizeof(float)+6u*sizeof(unsigned)+2u*sizeof(unsigned));
    passed=totalFaces*2==504u&&meshBytes==20160u&&horde::scene::kWaterShaftTopWorldY==5.60f&&passed;
    passed=CheckSprig({{1.25f,3.0f,-2.0f},0.90f,1.6f})&&passed;
    passed=CheckSprig({{-4.0f,5.5f,-8.0f},2.30f,5.9f})&&passed;
    passed=CheckGridAndCeiling()&&passed;
    for(const auto& placement:horde::scene::kWallPanelSprigs) {
        passed=CheckSprig(placement)&&passed;
        const auto faces=horde::scene::MakeHangingSprig(placement.attachment,placement.length,placement.phase,placement.leafPlane);
        const auto root=SolidBounds(faces,0);
        const float jamb=placement.attachment[0]<2.55f?2.05f:3.10f;
        passed=root.lo[0]<jamb&&root.hi[0]>jamb&&root.lo[2]<-8.8f&&root.hi[2]>-8.8f&&passed;
        for(const auto& face:faces) for(const auto& point:face.vertices)
            passed=(point[0]<2.25f||point[0]>2.90f)&&passed; // central grate is not sealed
    }
    const auto panelHits=PanelLeafPixelHits();
    passed=panelHits>=20u&&passed;
    auto broken=horde::scene::MakeHangingSprig({0,3,0},2,0.4f);
    std::swap(broken[0].vertices[1],broken[0].vertices[3]);
    passed=!ClosedSolid(broken,0)&&passed;
    auto detached=SolidBounds(broken,6); detached.lo[0]+=1; detached.hi[0]+=1;
    passed=!Overlap(SolidBounds(broken,0),detached)&&passed;
    const auto leaves=LeafFaces(horde::scene::kWaterShaftSprigs);
    const auto stems=StemFaces();
    unsigned minimumHits=std::numeric_limits<unsigned>::max(),cameraStates=0;
    for(const auto size:std::array<std::array<unsigned,2>,2>{{{960,540},{540,960}}})
        for(unsigned sample=0;sample<=60;++sample) for(const float walk:std::array<float,2>{{0,1}})
        {
            const auto hits=LeafPixelHits(leaves,ProductionCamera(static_cast<float>(sample)*0.2f,walk,size[0],size[1]),stems);
            minimumHits=std::min(minimumHits,hits); ++cameraStates;
            if(hits<2)
            {
                std::cerr<<"no readable unoccluded leaf pixels: "<<size[0]<<'x'<<size[1]<<" sample "<<sample
                         <<" walk "<<walk<<" hits "<<hits<<'\n'; passed=false;
            }
        }
    // Negative witness: original upper-only lengths do not pass the actual
    // production ray guard even though their mesh remains closed/nondegenerate.
    const std::array<horde::scene::HangingSprigPlacement,3> original{{
        {{-2.87f,5.58f,-15.91f},2.12f,0.4f},{{-1.64f,5.58f,-15.64f},1.62f,2.1f},
        {{-2.65f,5.58f,-14.78f},2.28f,4.0f}}};
    passed=LeafPixelHits(LeafFaces(original),ProductionCamera(0,0,960,540))==0&&passed;
    std::cout<<"shaft faces="<<totalFaces<<" triangles="<<totalFaces*2<<" bytes="<<meshBytes
             <<" cameraStates="<<cameraStates<<" minimumUnoccludedLeafPixelHits="<<minimumHits
             <<" panelLeafPixelHits="<<panelHits<<" gridBars=13 gridTriangles=156 panelTriangles=336\n";
    return passed?0:1;
}
