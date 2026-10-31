#include "scene/ForestSurroundings.h"
#include "scene/assets/StaticPrimitiveGrouping.h"
#include "../../assets/models/world/source/forest-surroundings-v1/native-layout.generated.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <string_view>

namespace horde::scene {
namespace {
using Asset=assets::StaticMeshAsset;
using Surface=gameplay::simulation::TriangleSupportSurface;
using Point=Surface::Point;
constexpr std::array<const char*,5> kNames{
    "pine-middle.glb","alder-middle.glb","pine-far.glb","alder-far.glb","boundary-panel.glb"};
constexpr std::array<std::size_t,5> kTriangles{1474,1390,164,180,60};
constexpr std::array<std::string_view,4> kSourceFamilies{
    "Original charcoal-gray ridged bark","Original opaque pine needle sprays",
    "Original opaque hanging woodland moss","Original opaque alder leaves"};
constexpr std::array<float,16> kIdentity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
Point Normalize(Point p) {
    const float n=std::sqrt(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);
    if(!(n>1e-8f))return {};
    for(auto& c:p)c/=n;return p;
}
bool Blocks(const Surface& ground, float x,float z,float nx,float nz) {
    const auto a=ground.Query(x-nx*.75f,z-nz*.75f);
    const auto b=ground.Query(x+nx*.75f,z+nz*.75f);
    if(!a.found || !b.found)return false; // absent support is not a solid-bank certificate
    // Crouched/dodging height is the conservative boundary witness. Terrain
    // and attached rendered solids must stop both crossing directions.
    auto blocked=[&](float sign,const Surface::Sample& s) {
        const float ax=x-sign*nx*.75f,az=z-sign*nz*.75f,bx=x+sign*nx*.75f,bz=z+sign*nz*.75f;
        return !ground.GroundSweep(ax,az,bx,bz,s.worldY,.22f,.18f) ||
            !ground.HorizontalBodyVolumeClear(ax,az,bx,bz,s.worldY,.75f,.22f) ||
            !ground.NonWalkableVolumeClear(ax,az,bx,bz,s.worldY+.05f,s.worldY+.75f,.22f);
    };
    return blocked(1,a)&&blocked(-1,b);
}
bool EdgeBlocked(const Surface& terrain,const surroundings_layout::Edge& e,std::size_t& probes) {
    const float dx=e.b[0]-e.a[0],dz=e.b[2]-e.a[2],length=std::hypot(dx,dz);
    if(!(length>.05f && length<2.5f))return false;
    const auto n=static_cast<unsigned>(std::ceil(length/.10f));
    for(unsigned i=0;i<=n;++i) {
        const float t=float(i)/float(n);++probes;
        if(!Blocks(terrain,e.a[0]+dx*t,e.a[2]+dz*t,-dz/length,dx/length))return false;
    }
    return true;
}
}

bool AppendPreparedForestSurroundings(const std::filesystem::path& root,
    const PreparedForestDressing& original,Asset& owner,
    std::shared_ptr<const Surface>& terrain,
    std::shared_ptr<const PreparedForestSurroundings>& result,std::string& diagnostic) {
    const auto fail=[&](const std::string& why){diagnostic="Forest surroundings: "+why;return false;};
    if(!terrain || !owner.HasValidTriangleProvenance() || owner.vertices.empty() ||
       original.placements.size()!=64 || owner.materials.size()>32 || owner.primitives.size()>32)
        return fail("requires complete original scene and immutable imported ground.");
    if(std::any_of(owner.nodeTransforms.begin(),owner.nodeTransforms.end(),[](const auto& n){return n.name=="forest-surroundings-world-baked";}))
        return fail("candidate was already appended; overlapping populations are forbidden.");
    std::array<std::uint32_t,4> destination{};
    for(std::size_t f=0;f<destination.size();++f) {
        const auto found=std::find_if(owner.materials.begin(),owner.materials.end(),[&](const auto& m){
            return m.name==original.materialFamilies[f].name && m.textureGroup==int(f) && m.deferTextureAllocation;
        });
        if(found==owner.materials.end())return fail("missing existing canonical material family.");
        destination[f]=static_cast<std::uint32_t>(found-owner.materials.begin());
    }
    assets::AssetManifest manifest;
    const auto folder=root/"models/world/source/forest-surroundings-v1/candidates";
    if(!assets::AssetManifest::Load(folder/"asset.manifest.json",manifest,diagnostic))return false;
    std::array<Asset,5> sources;
    for(std::size_t i=0;i<sources.size();++i) {
        if(!Asset::Load(folder/kNames[i],manifest,sources[i],diagnostic))return false;
        const auto& a=sources[i];
        if(a.indices.size()!=kTriangles[i]*3 || a.vertices.size()>1500 || a.primitives.size()>2 ||
           !a.HasValidTriangleProvenance())return fail(std::string(kNames[i])+" intake differs from authored bounded asset.");
        for(const auto& p:a.primitives)if(p.materialIndex>=a.materials.size() ||
           std::find(kSourceFamilies.begin(),kSourceFamilies.end(),a.materials[p.materialIndex].name)==kSourceFamilies.end())
            return fail("unknown candidate material family.");
        if(i==4) {
            if(a.vertices.size()!=40 || a.primitives.size()!=1 || a.primitives[0].vertexOffset!=0)
                return fail("boundary component roster changed.");
            for(std::size_t v=0;v<16;++v)if(v<8?a.vertices[v].position[0]>.1f:a.vertices[v].position[0]<2.3f)
                return fail("boundary endpoint-post component order changed.");
        }
    }
    std::array<std::vector<assets::StaticRtVertex>,4> vertices;
    std::array<std::vector<std::uint32_t>,4> indices;
    std::vector<Surface::Triangle> collision;
    auto prepared=std::make_shared<PreparedForestSurroundings>();
    auto& report=prepared->report;
    // One source primitive at a time, preserving local indices and every basis
    // attribute. All temporary geometry stays CPU-side until final validation.
    const auto append=[&](const Asset& a,auto transform,bool fence) {
        for(const auto& p:a.primitives) {
            const auto family=std::find(kSourceFamilies.begin(),kSourceFamilies.end(),a.materials[p.materialIndex].name)-kSourceFamilies.begin();
            auto& v=vertices[family];auto& ix=indices[family];
            std::size_t end=a.vertices.size();
            for(const auto& next:a.primitives)if(next.vertexOffset>p.vertexOffset)end=std::min(end,std::size_t(next.vertexOffset));
            std::vector<std::uint32_t> remap(end-p.vertexOffset,UINT32_MAX);
            for(std::size_t t=0;t<p.indexCount;t+=3) {
                // A closed loop uses exactly one starting post per edge. Never
                // overlap the next edge's post with this module's end post.
                bool endingPost=fence;
                for(unsigned c=0;c<3;++c) {
                    const auto local=a.indices[p.indexOffset+t+c];
                    endingPost=endingPost && local>=8 && local<16;
                }
                if(endingPost)continue;
                Surface::Triangle tri;tri.surface=gameplay::simulation::SupportSurface::Wood;
                for(unsigned c=0;c<3;++c) {
                    const auto local=a.indices[p.indexOffset+t+c];
                    if(local>=remap.size())return false;
                    if(remap[local]==UINT32_MAX) {
                        auto out=transform(a.vertices[p.vertexOffset+local],fence && local<8);
                        for(unsigned axis=0;axis<3;++axis)if(!std::isfinite(out.position[axis])||!std::isfinite(out.normal[axis])||!std::isfinite(out.tangent[axis]))return false;
                        remap[local]=static_cast<std::uint32_t>(v.size());v.push_back(out);
                    }
                    ix.push_back(remap[local]);
                    const auto& out=v[remap[local]];tri.points[c]={out.position[0],out.position[1],out.position[2]};
                }
                if(fence) {
                    const auto& a0=a.vertices[p.vertexOffset+a.indices[p.indexOffset+t]].position;
                    const auto& a1=a.vertices[p.vertexOffset+a.indices[p.indexOffset+t+1]].position;
                    const auto& a2=a.vertices[p.vertexOffset+a.indices[p.indexOffset+t+2]].position;
                    // Containment uses vertical post faces plus the two broad
                    // rail faces. These are exact rendered triangles, not a
                    // filled invisible panel. Rail ends terminate inside the
                    // join posts; rail caps do not block horizontal crossing.
                    // This is not a general complete prop-contact certificate.
                    const bool horizontal=std::abs(a0[1]-a1[1])<1e-6f && std::abs(a0[1]-a2[1])<1e-6f;
                    if((a.indices[p.indexOffset+t]<8 && a.indices[p.indexOffset+t+1]<8 && a.indices[p.indexOffset+t+2]<8 && !horizontal) ||
                       (std::abs(a0[2]-a1[2])<1e-6f && std::abs(a0[2]-a2[2])<1e-6f))collision.push_back(tri);
                }
            }
        }
        return true;
    };
    for(const auto& p:surroundings_layout::trees) {
        if(p.asset>=4)return fail("invalid asset selection.");
        const auto& a=sources[p.asset];const auto support=terrain->Query(p.x,p.z);
        if(!support.found)return fail("missing tree support.");
        const float co=std::cos(p.yaw),si=std::sin(p.yaw);
        float fitted=support.worldY-a.bounds.minimum[1];std::size_t tips=0;
        for(const auto& v:a.vertices)if(v.position[1]<=a.bounds.minimum[1]+.03f) {
            const auto g=terrain->Query(p.x+co*v.position[0]+si*v.position[2],p.z-si*v.position[0]+co*v.position[2]);
            if(!g.found)return fail("root footprint lacks imported ground.");
            fitted=std::min(fitted,g.worldY-v.position[1]-.02f);++tips;
        }
        const float burial=support.worldY-a.bounds.minimum[1]-fitted;
        if(tips<4 || burial>.6501f || std::abs(fitted-p.y)>.001f)return fail("actual root fitting differs from source receipt.");
        report.maximumRootBurial=std::max(report.maximumRootBurial,burial);
        if(!append(a,[&](auto v,bool){
            const auto rotate=[&](auto& f){const float x=f[0],z=f[2];f[0]=co*x+si*z;f[2]=-si*x+co*z;};
            rotate(v.position);rotate(v.normal);rotate(v.tangent);
            v.position[0]+=p.x;v.position[1]+=fitted;v.position[2]+=p.z;return v;
        },false))return fail("invalid transformed tree basis.");
        ++report.extraTrees;report.addedTreeTriangles+=kTriangles[p.asset];
    }
    for(const auto& e:surroundings_layout::edges) {
        std::size_t probes=0;
        if(e.inspectBank && EdgeBlocked(*terrain,e,probes)) {++report.naturalBankRuns;continue;}
        const float dx=e.b[0]-e.a[0],dz=e.b[2]-e.a[2],length=std::hypot(dx,dz);
        if(!(length>.05f && length<2.5f))return fail("invalid boundary span.");
        const float ux=dx/length,uz=dz/length,slope=(e.b[1]-e.a[1])/length;
        if(!append(sources[4],[&](auto v,bool post){
            const float x=post?v.position[0]:v.position[0]*length/2.4f,z=v.position[2];
            v.position[0]=e.a[0]+ux*x-uz*z;v.position[1]+=e.a[1]+slope*x;v.position[2]=e.a[2]+uz*x+ux*z;
            // Inverse transpose for the sheared rail span, forward derivative
            // for tangents. Post widths remain physical, not length-scaled.
            const float scale=post?1.f:length/2.4f;
            Point n{v.normal[0]/scale-slope*v.normal[1],v.normal[1],v.normal[2]};n=Normalize(n);
            Point t{v.tangent[0]*scale,v.tangent[1]+slope*v.tangent[0]*scale,v.tangent[2]};t=Normalize(t);
            v.normal[0]=ux*n[0]-uz*n[2];v.normal[1]=n[1];v.normal[2]=uz*n[0]+ux*n[2];
            v.tangent[0]=ux*t[0]-uz*t[2];v.tangent[1]=t[1];v.tangent[2]=uz*t[0]+ux*t[2];return v;
        },true))return fail("invalid boundary geometry.");
        ++report.fenceRuns;
    }
    report.fenceTriangles=report.fenceRuns*48;
    report.boundaryCollisionTriangles=collision.size();
    if(terrain->TriangleCount()+terrain->NonWalkableTriangleCount()+terrain->StepTriangleCount()+collision.size()>Surface::kMaximumTriangles)
        return fail("boundary exceeds immutable support intake: ground="+std::to_string(terrain->TriangleCount())+
            " existingSolid="+std::to_string(terrain->NonWalkableTriangleCount())+" step="+std::to_string(terrain->StepTriangleCount())+
            " newFence="+std::to_string(collision.size())+" limit="+std::to_string(Surface::kMaximumTriangles)+".");
    auto combined=Surface::WithAdditionalNonWalkableGeometry(*terrain,collision,diagnostic);
    if(!combined)return false;
    for(const auto& e:surroundings_layout::edges)if(!EdgeBlocked(*combined,e,report.boundaryProbes))
        return fail("actual rendered boundary leaves a crouch/dodge crossing at "+std::to_string(&e-surroundings_layout::edges.data())+".");
    report.boundaryContained=true;
    Asset staged=owner;
    staged.triangleRegions.resize(staged.indices.size()/3,assets::StaticTriangleRegion::Shared);
    for(std::size_t f=0;f<4;++f)if(!indices[f].empty()) {
        if(staged.vertices.size()+vertices[f].size()>UINT32_MAX || staged.indices.size()+indices[f].size()>UINT32_MAX)return fail("geometry address overflow.");
        assets::StaticPrimitiveRecord p;
        p.vertexOffset=static_cast<std::uint32_t>(staged.vertices.size());p.indexOffset=static_cast<std::uint32_t>(staged.indices.size());
        p.indexCount=static_cast<std::uint32_t>(indices[f].size());p.materialIndex=destination[f];p.nodeTransformIndex=static_cast<std::uint32_t>(staged.nodeTransforms.size());
        staged.nodeTransforms.push_back({"forest-surroundings-world-baked",kIdentity});
        for(const auto& v:vertices[f])for(unsigned axis=0;axis<3;++axis) {
            staged.bounds.minimum[axis]=std::min(staged.bounds.minimum[axis],v.position[axis]);
            staged.bounds.maximum[axis]=std::max(staged.bounds.maximum[axis],v.position[axis]);
        }
        staged.vertices.insert(staged.vertices.end(),vertices[f].begin(),vertices[f].end());
        staged.indices.insert(staged.indices.end(),indices[f].begin(),indices[f].end());
        staged.triangleRegions.insert(staged.triangleRegions.end(),indices[f].size()/3,assets::StaticTriangleRegion::ExteriorDressing);
        staged.primitives.push_back(p);report.addedVertices+=vertices[f].size();report.addedIndices+=indices[f].size();
    }
    if(!assets::ConsolidateWorldBakedMaterialFamilies(staged,diagnostic) || staged.primitives.size()>32 || staged.materials.size()!=owner.materials.size())return fail("existing material/primitive ceiling exceeded.");
    prepared->terrain=combined;owner=std::move(staged);terrain=combined;result=std::move(prepared);diagnostic.clear();return true;
}
}
