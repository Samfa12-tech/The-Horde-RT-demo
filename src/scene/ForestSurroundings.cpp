#include "scene/ForestSurroundings.h"
#include "scene/assets/StaticPrimitiveGrouping.h"
#include "gameplay/simulation/DevelopmentWorldRoute.h"
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
constexpr std::array<const char*,6> kNames{
    "pine-middle.glb","alder-middle.glb","pine-far.glb","alder-far.glb","boundary-panel.glb","woodland-low-fern.glb"};
constexpr std::array<std::size_t,6> kTriangles{1474,1390,164,180,60,588};
constexpr std::array<std::string_view,4> kSourceFamilies{
    "Original charcoal-gray ridged bark","Original opaque pine needle sprays",
    "Original opaque hanging woodland moss","Original opaque alder leaves"};
constexpr std::array<float,16> kIdentity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
Point Normalize(Point p) {
    const float n=std::sqrt(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);
    if(!(n>1e-8f))return {};
    for(auto& c:p)c/=n;return p;
}
// Lightweight identity/ground/contact guard only. Expensive actual movement,
// seam and flood admission lives in the host tests against this exact geometry.
// Natural section choices are immutable authored data bound to the terrain.
bool EdgeHasContact(const Surface& ground,const surroundings_layout::Edge& e,
                    std::size_t& probes,bool naturalOnly) {
    const float dx=e.b[0]-e.a[0],dz=e.b[2]-e.a[2],length=std::hypot(dx,dz);
    if(!(length>.05f && length<2.5f))return false;
    const float nx=-dz/length,nz=dx/length;
    const auto count=static_cast<unsigned>(std::ceil(length/.10f));
    for(unsigned i=0;i<=count;++i) {
        ++probes;const float t=float(i)/count,x=e.a[0]+dx*t,z=e.a[2]+dz*t;
        const auto middle=ground.Query(x,z);if(!middle.found)return false;
        if(!middle.walkable)continue;
        if(naturalOnly) {
            if(ground.NonWalkableVolumeClear(x,z,x,z,middle.worldY+.001f,middle.worldY+.75f,.22f))return false;
        } else {
            const auto a=ground.Query(x-nx*3,z-nz*3),b=ground.Query(x+nx*3,z+nz*3);
            if(!a.found||!b.found)return false;
            const float low=std::min({middle.worldY,a.worldY,b.worldY}),high=std::max({middle.worldY,a.worldY,b.worldY});
            if(ground.NonWalkableVolumeClear(x-nx*3,z-nz*3,x+nx*3,z+nz*3,low+.001f,high+.75f,.22f))return false;
        }
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
    std::array<std::uint32_t,5> destination{};
    for(std::size_t f=0;f<destination.size();++f) {
        const auto found=std::find_if(owner.materials.begin(),owner.materials.end(),[&](const auto& m){
            return f==4 ? m.name=="Boulder01Rock" : m.name==original.materialFamilies[f].name && m.textureGroup==int(f) && m.deferTextureAllocation;
        });
        if(found==owner.materials.end())return fail("missing existing canonical material family.");
        destination[f]=static_cast<std::uint32_t>(found-owner.materials.begin());
    }
    assets::AssetManifest manifest;
    const auto folder=root/"models/world/runtime/forest-surroundings-v1";
    if(!assets::AssetManifest::Load(folder/"asset.manifest.json",manifest,diagnostic))return false;
    std::array<Asset,6> sources;
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
    std::array<std::vector<assets::StaticRtVertex>,5> vertices;
    std::array<std::vector<std::uint32_t>,5> indices;
    std::vector<Surface::Triangle> collision;
    auto prepared=std::make_shared<PreparedForestSurroundings>();
    auto& report=prepared->report;
    // One source primitive at a time, preserving local indices and every basis
    // attribute. All temporary geometry stays CPU-side until final validation.
    const auto append=[&](const Asset& a,auto transform,bool fence,bool keepEndingPost=false,bool treeContact=false,bool rockContact=false) {
        for(const auto& p:a.primitives) {
            const auto family=rockContact?4:std::find(kSourceFamilies.begin(),kSourceFamilies.end(),a.materials[p.materialIndex].name)-kSourceFamilies.begin();
            auto& v=vertices[family];auto& ix=indices[family];
            std::size_t end=a.vertices.size();
            for(const auto& next:a.primitives)if(next.vertexOffset>p.vertexOffset)end=std::min(end,std::size_t(next.vertexOffset));
            std::vector<std::uint32_t> remap(end-p.vertexOffset,UINT32_MAX);
            for(std::size_t t=0;t<p.indexCount;t+=3) {
                // A closed loop uses exactly one starting post per edge. Never
                // overlap the next edge's post with this module's end post.
                bool endingPost=fence && !keepEndingPost;
                for(unsigned c=0;c<3;++c) {
                    const auto local=a.indices[p.indexOffset+t+c];
                    endingPost=endingPost && local>=8 && local<16;
                }
                if(endingPost)continue;
                Surface::Triangle tri;tri.surface=rockContact?gameplay::simulation::SupportSurface::Stone:gameplay::simulation::SupportSurface::Wood;
                for(unsigned c=0;c<3;++c) {
                    const auto local=a.indices[p.indexOffset+t+c];
                    if(local>=remap.size())return false;
                    if(remap[local]==UINT32_MAX) {
                        auto out=transform(a.vertices[p.vertexOffset+local],fence && local<16);
                        for(unsigned axis=0;axis<3;++axis)if(!std::isfinite(out.position[axis])||!std::isfinite(out.normal[axis])||!std::isfinite(out.tangent[axis]))return false;
                        remap[local]=static_cast<std::uint32_t>(v.size());v.push_back(out);
                    }
                    ix.push_back(remap[local]);
                    const auto& out=v[remap[local]];tri.points[c]={out.position[0],out.position[1],out.position[2]};
                }
                if(rockContact)collision.push_back(tri);
                if(treeContact && family==0) {
                    float low=std::numeric_limits<float>::max();
                    for(unsigned c=0;c<3;++c)low=std::min(low,a.vertices[p.vertexOffset+a.indices[p.indexOffset+t+c]].position[1]);
                    // Exact rendered wood faces intersecting the lower 3m:
                    // roots/trunk/low branches, no proxy box or invisible sheet.
                    if(low<3.f) {collision.push_back(tri);++report.treeContactTriangles;}
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
                    if((a.indices[p.indexOffset+t]<16 && a.indices[p.indexOffset+t+1]<16 && a.indices[p.indexOffset+t+2]<16 && !horizontal) ||
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
        },false,false,true))return fail("invalid transformed tree basis.");
        ++report.extraTrees;report.addedTreeTriangles+=kTriangles[p.asset];
    }
    for(const auto& p:surroundings_layout::understory) {
        const auto& a=sources[5];const auto centre=terrain->Query(p.x,p.z);
        if(!centre.found)return fail("understory lacks ground");
        const float co=std::cos(p.yaw),si=std::sin(p.yaw);
        float fitted=centre.worldY-a.bounds.minimum[1]-.03f;
        for(const auto& v:a.vertices) {
            const float x=p.x+co*v.position[0]+si*v.position[2],z=p.z-si*v.position[0]+co*v.position[2];
            const auto under=terrain->Query(x,z);
            if(!under.found)return fail("understory full footprint leaves imported ground");
            if(v.position[1]<=a.bounds.minimum[1]+.03f)fitted=std::min(fitted,under.worldY-v.position[1]-.03f);
            if(gameplay::simulation::ProjectWorldRoute(x,z).distance<1.35f ||
               std::hypot(x+33.7f,z+17.f)<4.1f || std::hypot(x+28.7f,z+5.8f)<3.f || std::hypot(x-13.3f,z-36.2f)<3.2f)
                return fail("actual understory footprint obstructs route or protected pocket");
        }
        if(!append(a,[&](auto v,bool){
            auto rotate=[&](auto& f){const float x=f[0],z=f[2];f[0]=co*x+si*z;f[2]=-si*x+co*z;};
            rotate(v.position);rotate(v.normal);rotate(v.tangent);
            v.position[0]+=p.x;v.position[1]+=fitted;v.position[2]+=p.z;return v;
        },false))return fail("invalid understory basis");
        ++report.understoryClumps;report.understoryTriangles+=kTriangles[5];
    }
    // Resolve visible natural boundaries before assembling runs. A short fence
    // ending at a bank or the arrival portal owns its terminal post; joined
    // modules still share exactly one upright post.
    assets::AssetManifest rockManifest;std::array<Asset,2> rockSources;
    const auto rockFolder=root/"models/world/runtime/forest-rocks-v1";
    if(!assets::AssetManifest::Load(rockFolder/"asset.manifest.json",rockManifest,diagnostic))return false;
    constexpr std::array<const char*,2> rockFiles{"horde-rock-upright_crag.glb","horde-rock-low_boulder.glb"};
    for(unsigned i=0;i<2;++i) {
        if(!Asset::Load(rockFolder/rockFiles[i],rockManifest,rockSources[i],diagnostic))return false;
        if(rockSources[i].indices.size()!=(i?378:376)*3 || rockSources[i].primitives.size()!=1)return fail("boundary rock identity changed");
    }
    std::array<bool,surroundings_layout::edges.size()> fences{};
    std::array<bool,surroundings_layout::edges.size()> crags{};
    for(std::size_t i=0;i<fences.size();++i) {
        const auto& e=surroundings_layout::edges[i];
        if(e.arrivalPortal) {++report.arrivalPortalRuns;continue;}
        std::size_t probes=0;
        if(e.terrainBoundaryApproved) {
            if(!EdgeHasContact(*terrain,e,probes,true))return fail("authored natural boundary lost its terrain/contact identity");
            ++report.naturalBankRuns;continue;
        }
        if(!e.fenceRequested) {crags[i]=true;continue;}
        fences[i]=true;
    }
    for(std::size_t i=0;i<crags.size();++i)if(crags[i]) {
        const auto& rock=rockSources[i%3==0?0:1];
        const float flip=i%2?-1.f:1.f;
        const auto& e=surroundings_layout::edges[i];
        const float dx=e.b[0]-e.a[0],dz=e.b[2]-e.a[2],len=std::hypot(dx,dz),ux=dx/len,uz=dz/len;
        // Overlap the tapered ends of neighbouring existing crag meshes. Each
        // shape is visibly grounded; the exact rendered faces own all contact.
        const float sx=(len+2.0f)/(rock.bounds.maximum[0]-rock.bounds.minimum[0]);
        const float sz=(1.4f+.15f*float(i%3))/(rock.bounds.maximum[2]-rock.bounds.minimum[2]);
        float sy=(1.0f+.14f*float(i%4))/(rock.bounds.maximum[1]-rock.bounds.minimum[1]);
        const float x=(e.a[0]+e.b[0])*.5f,z=(e.a[2]+e.b[2])*.5f;
        float base=std::numeric_limits<float>::max(),highest=std::numeric_limits<float>::lowest();
        for(const auto& v:rock.vertices) {
            const auto g=terrain->Query(x+flip*(ux*v.position[0]*sx-uz*v.position[2]*sz),z+flip*(uz*v.position[0]*sx+ux*v.position[2]*sz));
            if(!g.found)return fail("boundary crag footprint lacks ground");
            base=std::min(base,g.worldY-.30f);highest=std::max(highest,g.worldY);
        }
        sy+=(highest-base-.30f)/(rock.bounds.maximum[1]-rock.bounds.minimum[1]);
        if(!append(rock,[&](auto v,bool){
            const float px=v.position[0]*sx*flip,pz=v.position[2]*sz*flip;
            v.position[0]=x+ux*px-uz*pz;v.position[1]=base+v.position[1]*sy;v.position[2]=z+uz*px+ux*pz;
            auto n=Normalize({v.normal[0]/sx*flip,v.normal[1]/sy,v.normal[2]/sz*flip});
            auto t=Normalize({v.tangent[0]*sx*flip,v.tangent[1]*sy,v.tangent[2]*sz*flip});
            v.normal[0]=ux*n[0]-uz*n[2];v.normal[1]=n[1];v.normal[2]=uz*n[0]+ux*n[2];
            v.tangent[0]=ux*t[0]-uz*t[2];v.tangent[1]=t[1];v.tangent[2]=uz*t[0]+ux*t[2];return v;
        },false,false,false,true))return fail("boundary crag transform invalid");
        ++report.rockRuns;report.rockTriangles+=rock.indices.size()/3;
    }
    for(std::size_t i=0;i<fences.size();++i) {
        if(!fences[i])continue;
        const auto& e=surroundings_layout::edges[i];
        const bool terminal=!fences[(i+1)%fences.size()];
        const float dx=e.b[0]-e.a[0],dz=e.b[2]-e.a[2],length=std::hypot(dx,dz);
        if(!(length>.05f && length<2.5f))return fail("invalid boundary span.");
        const float ux=dx/length,uz=dz/length,slope=(e.b[1]-e.a[1])/length;
        if(!append(sources[4],[&](auto v,bool post){
            const float x=post?(v.position[0]>1.2f?length+v.position[0]-2.4f:v.position[0]):v.position[0]*length/2.4f,z=v.position[2];
            v.position[0]=e.a[0]+ux*x-uz*z;v.position[1]+=e.a[1]+slope*x;v.position[2]=e.a[2]+uz*x+ux*z;
            // Inverse transpose for the sheared rail span, forward derivative
            // for tangents. Post widths remain physical, not length-scaled.
            const float scale=post?1.f:length/2.4f;
            Point n{v.normal[0]/scale-slope*v.normal[1],v.normal[1],v.normal[2]};n=Normalize(n);
            Point t{v.tangent[0]*scale,v.tangent[1]+slope*v.tangent[0]*scale,v.tangent[2]};t=Normalize(t);
            v.normal[0]=ux*n[0]-uz*n[2];v.normal[1]=n[1];v.normal[2]=uz*n[0]+ux*n[2];
            v.tangent[0]=ux*t[0]-uz*t[2];v.tangent[1]=t[1];v.tangent[2]=uz*t[0]+ux*t[2];return v;
        },true,terminal))return fail("invalid boundary geometry.");
        ++report.fenceRuns;if(terminal)++report.terminalFencePosts;
    }
    report.fenceTriangles=report.fenceRuns*48+report.terminalFencePosts*12;
    report.boundaryCollisionTriangles=collision.size()-report.treeContactTriangles;
    if(terrain->TriangleCount()+terrain->NonWalkableTriangleCount()+terrain->StepTriangleCount()+collision.size()>Surface::kMaximumTriangles)
        return fail("boundary exceeds immutable support intake: ground="+std::to_string(terrain->TriangleCount())+
            " existingSolid="+std::to_string(terrain->NonWalkableTriangleCount())+" step="+std::to_string(terrain->StepTriangleCount())+
            " newFence="+std::to_string(collision.size())+" limit="+std::to_string(Surface::kMaximumTriangles)+".");
    auto combined=Surface::WithAdditionalNonWalkableGeometry(*terrain,collision,diagnostic);
    if(!combined)return false;
    for(const auto& e:surroundings_layout::edges)if(!e.arrivalPortal && !EdgeHasContact(*combined,e,report.boundaryProbes,false))
        return fail("actual rendered boundary lacks authored contact at "+std::to_string(&e-surroundings_layout::edges.data())+".");
    report.boundarySectionsContained=true;
    report.boundaryContained=report.arrivalPortalRuns==0 && report.openWoodlandRuns==0;
    Asset staged=owner;
    staged.triangleRegions.resize(staged.indices.size()/3,assets::StaticTriangleRegion::Shared);
    for(std::size_t f=0;f<5;++f)if(!indices[f].empty()) {
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
