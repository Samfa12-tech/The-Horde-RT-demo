#include "scene/ForestDeadwood.h"
#include "gameplay/simulation/DevelopmentWorldRoute.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace horde::scene {
namespace {
using Point=std::array<float,3>;
struct Placement {unsigned source,segment;float fraction,side,yaw,scale;};
constexpr std::array<Placement,10> kPlacements{{
    {0,2,.32f,4.8f,.5f,1.f},{1,2,.75f,-4.8f,1.6f,.9f},{2,3,.15f,-5.5f,.8f,1.f},
    {0,3,.60f,-5.0f,1.1f,1.f},{3,3,.86f,6.2f,1.5f,1.f},
    {1,4,.16f,-4.8f,.3f,1.f},{2,4,.66f,5.6f,1.7f,1.f},
    {0,5,.48f,5.0f,2.4f,.9f},{1,6,.20f,-4.8f,2.6f,.9f},{3,6,.65f,-6.0f,.8f,1.f}}};
Point Transform(Point p,float yaw,Point up) {
    const float c=std::cos(yaw),s=std::sin(yaw);
    const Point q{c*p[0]+s*p[2],p[1],-s*p[0]+c*p[2]};
    const float len=std::hypot(up[0],up[1]);const Point right{up[1]/len,-up[0]/len,0};
    const Point forward{right[1]*up[2],-right[0]*up[2],right[0]*up[1]-right[1]*up[0]};
    return {right[0]*q[0]+up[0]*q[1]+forward[0]*q[2],right[1]*q[0]+up[1]*q[1]+forward[1]*q[2],
        right[2]*q[0]+up[2]*q[1]+forward[2]*q[2]};
}
// Original Horde geometry: three tapered four-corner rings, closed with twenty
// flat-shaded triangles. Cross-trail X spans 1.10m; Z is the short approach axis.
// Ground placement is sampled from the same immutable imported terrain, never
// from route blockout heights or from a prop's eligible STEP top.
bool AppendTrailRoot(const gameplay::simulation::TriangleSupportSurface& ground,
    std::vector<assets::StaticRtVertex>& vertices,std::vector<std::uint32_t>& indices,
    std::vector<gameplay::simulation::TriangleSupportSurface::Triangle>& steps,
    ForestDeadwoodReport& report,std::string& diagnostic) {
    using Surface=gameplay::simulation::TriangleSupportSurface;
    const auto a=gameplay::simulation::kWorldRoutePoints[3],b=gameplay::simulation::kWorldRoutePoints[4];
    const float length=std::hypot(b.x-a.x,b.z-a.z),fx=(b.x-a.x)/length,fz=(b.z-a.z)/length;
    const float rx=fz,rz=-fx,cx=a.x+(b.x-a.x)*.40f,cz=a.z+(b.z-a.z)*.40f;
    const auto centre=ground.Query(cx,cz);
    if(!centre.found||!centre.walkable) {diagnostic="Trail root lacks walkable real terrain.";return false;}
    auto& placed=report.trailRoot;placed.source=4;placed.origin={cx,centre.worldY,cz};
    placed.minimum.fill(std::numeric_limits<float>::max());placed.maximum.fill(std::numeric_limits<float>::lowest());
    report.approachMinimumY=std::numeric_limits<float>::max();report.approachMaximumY=std::numeric_limits<float>::lowest();
    // Survey the root plus a capsule-wide approach/exit on both sides. A missing
    // patch, steep bank or discontinuity rejects both render and contact owners.
    for(int cross=-8;cross<=8;++cross)for(int along=-6;along<=6;++along) {
        const float x=cx+rx*float(cross)*.1f+fx*float(along)*.1f;
        const float z=cz+rz*float(cross)*.1f+fz*float(along)*.1f;
        const auto sample=ground.Query(x,z);
        if(!sample.found||!sample.walkable) {diagnostic="Trail root approach survey leaves walkable terrain.";return false;}
        report.approachMinimumY=std::min(report.approachMinimumY,sample.worldY);
        report.approachMaximumY=std::max(report.approachMaximumY,sample.worldY);
    }
    if(!ground.GroundSweep(cx-fx*.6f,cz-fz*.6f,cx+fx*.6f,cz+fz*.6f,
        ground.Query(cx-fx*.6f,cz-fz*.6f).worldY,.24f)) {
        diagnostic="Trail root approach has discontinuous terrain support.";return false;
    }
    std::array<Point,12> points{};
    constexpr std::array<float,3> xs{{-.55f,0,.55f}},widths{{.045f,.09f,.035f}},heights{{.035f,.10f,.025f}};
    for(unsigned ring=0;ring<3;++ring)for(unsigned corner=0;corner<4;++corner) {
        const bool top=corner==1||corner==2;
        // The visible steep sides terminate at the eligible top's same XZ
        // outline. A wider flared base would hit the capsule before that top
        // enters its footprint, correctly rejecting it as an unstep-safe side.
        const float z=(corner<2?-1.f:1.f)*widths[ring];
        auto& p=points[ring*4+corner];p={cx+rx*xs[ring]+fx*z,0,cz+rz*xs[ring]+fz*z};
        const auto sample=ground.Query(p[0],p[2]);
        if(!sample.found||!sample.walkable) {diagnostic="Trail root vertex lacks real terrain.";return false;}
        p[1]=sample.worldY+(top?heights[ring]:-.035f);
    }
    const auto append=[&](unsigned aIndex,unsigned bIndex,unsigned cIndex) {
        Surface::Triangle triangle{{points[aIndex],points[bIndex],points[cIndex]},gameplay::simulation::SupportSurface::Wood};
        const auto aPoint=triangle.points[0],bPoint=triangle.points[1],cPoint=triangle.points[2];
        Point u{},v{},normal{};float uLength=0;
        for(unsigned axis=0;axis<3;++axis){u[axis]=bPoint[axis]-aPoint[axis];v[axis]=cPoint[axis]-aPoint[axis];uLength+=u[axis]*u[axis];}
        uLength=std::sqrt(uLength);
        normal={u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};
        const float nLength=std::sqrt(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);
        const float along=(v[0]*u[0]+v[1]*u[1]+v[2]*u[2])/uLength;
        for(unsigned corner=0;corner<3;++corner) {
            assets::StaticRtVertex vertex{};
            for(unsigned axis=0;axis<3;++axis) {
                vertex.position[axis]=triangle.points[corner][axis];vertex.normal[axis]=normal[axis]/nLength;
                vertex.tangent[axis]=u[axis]/uLength;
                placed.minimum[axis]=std::min(placed.minimum[axis],vertex.position[axis]);
                placed.maximum[axis]=std::max(placed.maximum[axis],vertex.position[axis]);
            }
            vertex.tangent[3]=1;
            vertex.uv0={corner==1?uLength:corner==2?along:0,corner==2?nLength/uLength:0,0,0};
            indices.push_back(static_cast<std::uint32_t>(vertices.size()));vertices.push_back(vertex);
        }
        // Construct STEP triples from the finished RT positions. There is no
        // simplified box, independently transformed collider or invisible ramp.
        for(unsigned corner=0;corner<3;++corner) {
            const auto& p=vertices[vertices.size()-3+corner].position;triangle.points[corner]={p[0],p[1],p[2]};
        }
        steps.push_back(triangle);
    };
    for(unsigned ring=0;ring<2;++ring)for(unsigned face=0;face<4;++face) {
        const unsigned aIndex=ring*4+face,next=ring*4+(face+1)%4;
        append(aIndex,next,next+4);append(aIndex,next+4,aIndex+4);
    }
    append(0,2,1);append(0,3,2);append(8,9,10);append(8,10,11);
    // Survey every rendered face against the actual ground, including interior
    // barycentric samples across terrain triangle boundaries. Reject, never
    // silently lower/simplify a root that exceeds the authored 12cm rise limit.
    for(const auto& triangle:steps)for(unsigned u=0;u<=10;++u)for(unsigned v=0;u+v<=10;++v) {
        Point p{};for(unsigned axis=0;axis<3;++axis)p[axis]=(triangle.points[0][axis]*float(10-u-v)+
            triangle.points[1][axis]*float(u)+triangle.points[2][axis]*float(v))*.1f;
        const auto sample=ground.Query(p[0],p[2]);
        if(!sample.found||!sample.walkable||p[1]-sample.worldY>.12001f) {
            diagnostic="Trail root exceeds its surveyed 12cm physical rise.";return false;
        }
        report.maximumRootRise=std::max(report.maximumRootRise,p[1]-sample.worldY);
    }
    report.stepTriangles=steps.size();report.rootVertices=steps.size()*3;
    report.rootGeometryBytes=report.rootVertices*sizeof(assets::StaticRtVertex)+steps.size()*3*sizeof(std::uint32_t);
    return true;
}
}
bool AppendPreparedForestDeadwood(const std::filesystem::path& root,assets::StaticMeshAsset& owner,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>& support,
    ForestDeadwoodReport& report,std::string& diagnostic) {
    report={};constexpr const char* nodeName="prepared-deadwood-world-baked";
    const auto family=std::find_if(owner.materials.begin(),owner.materials.end(),[](const auto& m){return m.name=="ForestBark";});
    if(!owner.HasValidTriangleProvenance() || !support || support->StepTriangleCount()!=0 || family==owner.materials.end() || family->metallicFactor!=0 || family->transmissionFactor!=0 ||
       family->emissiveFactor!=Point{} || owner.nodeTransforms.empty() || owner.primitives.empty() ||
       owner.primitives.size()>=32 || std::any_of(owner.nodeTransforms.begin(),owner.nodeTransforms.end(),
            [&](const auto& n){return n.name==nodeName;})) {
        diagnostic="Prepared deadwood requires shared bark, ground and a fresh bounded admission.";return false;
    }
    const auto dir=root/"models/world/runtime/forest-deadwood-v1";
    assets::AssetManifest manifest;if(!assets::AssetManifest::Load(dir/"asset.manifest.json",manifest,diagnostic))return false;
    constexpr std::array<const char*,4> files{{"horde-fallen-log.glb","horde-root-tangle.glb","horde-gnarled-stump.glb","horde-dead-snag-lod2.glb"}};
    constexpr std::array<std::size_t,4> counts{{544,548,3106,1198}};
    std::array<assets::StaticMeshAsset,4> sources;
    for(std::size_t i=0;i<sources.size();++i) {
        auto& source=sources[i];
        if(!assets::StaticMeshAsset::Load(dir/files[i],manifest,source,diagnostic))return false;
        if(source.indices.size()!=counts[i]*3 || source.primitives.empty() || source.primitives.size()>2 ||
           source.materials.empty() || source.materials.size()>2 ||
           std::any_of(source.materials.begin(),source.materials.end(),[](const auto& m){
               return m.transmissionFactor!=0 || m.emissiveFactor!=Point{};})) {
            diagnostic="Prepared deadwood lost its bounded opaque native identity.";return false;
        }
    }
    ForestDeadwoodReport candidate;
    std::vector<assets::StaticRtVertex> vertices;std::vector<std::uint32_t> indices;
    std::vector<gameplay::simulation::TriangleSupportSurface::Triangle> contact;
    for(const auto& authored:kPlacements) {
        const auto& source=sources[authored.source];
        const auto a=gameplay::simulation::kWorldRoutePoints[authored.segment],b=gameplay::simulation::kWorldRoutePoints[authored.segment+1];
        const float dx=b.x-a.x,dz=b.z-a.z,len=std::hypot(dx,dz);
        const float x=a.x+dx*authored.fraction-dz/len*authored.side,z=a.z+dz*authored.fraction+dx/len*authored.side;
        const auto ground=support->Query(x,z);
        if(!ground.found || ground.normal[1]<.5f || gameplay::simulation::ProjectWorldRoute(x,z).distance<=2.4f) {
            diagnostic="Prepared deadwood has missing support, excessive slope or a blocked trail.";return false;
        }
        ForestDeadwoodPlacement placed;placed.source=authored.source;placed.origin={x,ground.worldY-source.bounds.minimum[1]*authored.scale,z};
        // Fallen pieces follow the slope. A standing tree grows upright instead
        // of rotating its entire 6.2m crown into the trail with the bank normal.
        const Point basisUp=authored.source==3 ? Point{0,1,0} : ground.normal;
        // Survey actual low geometry against the exported terrain. The snag is
        // an open mesh: exact nonwalkable triangles, never a closed-volume claim.
        float burial=0;
        for(const auto& v:source.vertices)if(v.position[1]<=source.bounds.minimum[1]+.12f*(source.bounds.maximum[1]-source.bounds.minimum[1])) {
            const auto p=Transform({v.position[0]*authored.scale,v.position[1]*authored.scale,v.position[2]*authored.scale},authored.yaw,basisUp);
            const auto under=support->Query(x+p[0],z+p[2]);
            if(!under.found){diagnostic="Deadwood perimeter leaves the real ground.";return false;}
            burial=std::max(burial,placed.origin[1]+p[1]-under.worldY);
        }
        placed.origin[1]-=burial+.03f;
        placed.minimum.fill(std::numeric_limits<float>::max());placed.maximum.fill(std::numeric_limits<float>::lowest());
        const auto base=static_cast<std::uint32_t>(vertices.size());
        for(auto v:source.vertices) {
            const auto p=Transform({v.position[0]*authored.scale,v.position[1]*authored.scale,v.position[2]*authored.scale},authored.yaw,basisUp);
            const auto n=Transform({v.normal[0],v.normal[1],v.normal[2]},authored.yaw,basisUp);
            const auto t=Transform({v.tangent[0],v.tangent[1],v.tangent[2]},authored.yaw,basisUp);
            for(unsigned axis=0;axis<3;++axis) {
                v.position[axis]=p[axis]+placed.origin[axis];v.normal[axis]=n[axis];v.tangent[axis]=t[axis];
                placed.minimum[axis]=std::min(placed.minimum[axis],v.position[axis]);placed.maximum[axis]=std::max(placed.maximum[axis],v.position[axis]);
            }
            if(gameplay::simulation::ProjectWorldRoute(v.position[0],v.position[2]).distance<1.35f) {
                diagnostic="Deadwood geometry intrudes on the clear trail: source "+std::to_string(authored.source)+
                    ", segment "+std::to_string(authored.segment)+", point "+std::to_string(v.position[0])+","+
                    std::to_string(v.position[1])+","+std::to_string(v.position[2]);return false;
            }
            vertices.push_back(v);
        }
        // Native indices are primitive-local. Preserve each primitive's vertex
        // offset while baking both original log/root materials into shared bark.
        for(const auto& primitive:source.primitives)for(std::size_t i=0;i<primitive.indexCount;i+=3) {
            gameplay::simulation::TriangleSupportSurface::Triangle triangle;
            triangle.surface=gameplay::simulation::SupportSurface::Stone;
            for(unsigned corner=0;corner<3;++corner) {
                const auto index=base+primitive.vertexOffset+source.indices[primitive.indexOffset+i+corner];indices.push_back(index);
                const auto& p=vertices[index].position;triangle.points[corner]={p[0],p[1],p[2]};
            }
            contact.push_back(triangle);
        }
        candidate.triangles+=counts[authored.source];candidate.placements.push_back(placed);
    }
    candidate.nonWalkableTriangles=candidate.triangles;
    candidate.rootVertexOffset=owner.vertices.size()+vertices.size();candidate.rootIndexOffset=owner.indices.size()+indices.size();
    std::vector<gameplay::simulation::TriangleSupportSurface::Triangle> steps;
    if(!AppendTrailRoot(*support,vertices,indices,steps,candidate,diagnostic))return false;
    candidate.triangles+=candidate.stepTriangles;
    auto prepared=gameplay::simulation::TriangleSupportSurface::WithAdditionalNonWalkableGeometry(*support,contact,diagnostic);
    if(!prepared)return false;
    prepared=gameplay::simulation::TriangleSupportSurface::WithStepGeometry(*prepared,steps,diagnostic);
    if(!prepared)return false;
    if(owner.vertices.size()>UINT32_MAX-vertices.size() || owner.indices.size()>UINT32_MAX-indices.size()) {
        diagnostic="Prepared deadwood exceeds native addressing.";return false;
    }
    auto staged=owner;
    staged.primitives.push_back({static_cast<std::uint32_t>(staged.vertices.size()),static_cast<std::uint32_t>(staged.indices.size()),
        static_cast<std::uint32_t>(indices.size()),static_cast<std::uint32_t>(family-owner.materials.begin()),static_cast<std::uint32_t>(staged.nodeTransforms.size())});
    staged.nodeTransforms.push_back({nodeName,{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}}});
    staged.vertices.insert(staged.vertices.end(),vertices.begin(),vertices.end());
    staged.triangleRegions.resize(staged.indices.size()/3u,assets::StaticTriangleRegion::Shared);
    staged.triangleRegions.insert(staged.triangleRegions.end(),indices.size()/3u,assets::StaticTriangleRegion::ExteriorDressing);
    staged.indices.insert(staged.indices.end(),indices.begin(),indices.end());
    for(const auto& placed:candidate.placements)for(unsigned axis=0;axis<3;++axis) {
        staged.bounds.minimum[axis]=std::min(staged.bounds.minimum[axis],placed.minimum[axis]);staged.bounds.maximum[axis]=std::max(staged.bounds.maximum[axis],placed.maximum[axis]);
    }
    for(unsigned axis=0;axis<3;++axis) {
        staged.bounds.minimum[axis]=std::min(staged.bounds.minimum[axis],candidate.trailRoot.minimum[axis]);
        staged.bounds.maximum[axis]=std::max(staged.bounds.maximum[axis],candidate.trailRoot.maximum[axis]);
    }
    owner=std::move(staged);support=std::move(prepared);report=std::move(candidate);diagnostic.clear();return true;
}
}
