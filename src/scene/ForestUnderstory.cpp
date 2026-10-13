#include "scene/ForestUnderstory.h"
#include "gameplay/simulation/DevelopmentWorldRoute.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace horde::scene {
namespace {
struct Clump {unsigned source,segment;float fraction,side,yaw,scale;};
// Uneven pockets at the clearing and bends, beyond the clear 2.4m trail.
constexpr std::array<Clump,18> kClumps{{
    {1,2,.54f,-3.4f,.2f,.65f},{1,2,.62f,-3.8f,1.2f,.72f},{0,2,.69f,-3.5f,2.3f,.85f},
    {0,3,.23f,3.4f,.8f,.80f},{1,3,.28f,3.9f,2.1f,.70f},{0,3,.34f,3.5f,2.8f,.85f},
    {0,3,.70f,-3.6f,.4f,.80f},{1,3,.76f,-4.0f,1.6f,.65f},{1,3,.83f,-3.8f,2.5f,.72f},
    {1,4,.30f,3.8f,1.1f,.75f},{0,4,.36f,3.4f,2.2f,.85f},{0,4,.43f,4.0f,2.9f,.80f},
    {0,5,.16f,-2.5f,.3f,.85f},{1,5,.20f,-2.9f,1.4f,.70f},{0,5,.25f,-2.7f,2.4f,.80f},
    {1,6,.46f,3.4f,.9f,.72f},{0,6,.53f,3.9f,1.9f,.85f},{1,6,.60f,3.6f,2.7f,.70f},
}};
using Point=std::array<float,3>;
Point Rotate(Point p,float yaw) {
    const float c=std::cos(yaw),s=std::sin(yaw);
    return {c*p[0]+s*p[2],p[1],-s*p[0]+c*p[2]};
}
Point OnSurface(Point p,Point up) {
    const float length=std::hypot(up[0],up[1]);
    const Point right={up[1]/length,-up[0]/length,0};
    const Point forward={right[1]*up[2],-right[0]*up[2],right[0]*up[1]-right[1]*up[0]};
    // right x up gives forward; flat terrain preserves local +Z and winding.
    return {right[0]*p[0]+up[0]*p[1]+forward[0]*p[2],
        right[1]*p[0]+up[1]*p[1]+forward[1]*p[2],
        right[2]*p[0]+up[2]*p[1]+forward[2]*p[2]};
}
}
bool AppendPreparedForestUnderstory(const std::filesystem::path& root, assets::StaticMeshAsset& owner,
    const gameplay::simulation::TriangleSupportSurface& support, ForestUnderstoryReport& report,
    std::string& diagnostic) {
    report={};
    const auto material=std::find_if(owner.materials.begin(),owner.materials.end(),
        [](const auto& m){return m.name=="ForestAlder";});
    if(!owner.HasValidTriangleProvenance() || material==owner.materials.end() || owner.primitives.empty() || owner.primitives.size()>=32 ||
       owner.nodeTransforms.empty() || material->textureGroup!=3 || !material->deferTextureAllocation) {
        diagnostic="Understory requires the existing admitted alder family and bounded world owner.";return false;
    }
    const auto materialIndex=static_cast<std::uint32_t>(material-owner.materials.begin());
    const auto directory=root/"models/world/runtime/forest-understory-v1";
    assets::AssetManifest manifest;
    if(!assets::AssetManifest::Load(directory/"asset.manifest.json",manifest,diagnostic))return false;
    constexpr std::array<const char*,2> names{{"horde-fern-open-clump.glb","horde-fern-small-clump.glb"}};
    constexpr std::array<std::size_t,2> counts{{1200,768}};
    std::array<assets::StaticMeshAsset,2> sources;
    for(std::size_t i=0;i<sources.size();++i) {
        auto& source=sources[i];
        if(!assets::StaticMeshAsset::Load(directory/names[i],manifest,source,diagnostic))return false;
        if(source.materials.size()!=1 || source.primitives.size()!=1 || source.indices.size()/3!=counts[i] ||
           source.primitives[0].vertexOffset!=0 || source.primitives[0].indexOffset!=0 ||
           source.primitives[0].indexCount!=source.indices.size()) {
            diagnostic="Understory source lost its exact bounded native geometry.";return false;
        }
        const auto& m=source.materials[0];
        if(m.name!="BH_Shared_Original_Leaf" || m.baseColorTexture!=0 || m.normalTexture>=0 ||
           m.ormTexture>=0 || m.emissiveTexture>=0 || m.transmissionFactor!=0 || m.metallicFactor!=0 ||
           m.emissiveFactor!=Point{}) {
            diagnostic="Understory source requires unadmitted material/optical features.";return false;
        }
    }
    std::vector<assets::StaticRtVertex> vertices;
    std::vector<std::uint32_t> indices;
    ForestUnderstoryReport candidate;
    for(const auto& clump:kClumps) {
        const auto& source=sources[clump.source];
        const auto a=gameplay::simulation::kWorldRoutePoints[clump.segment];
        const auto b=gameplay::simulation::kWorldRoutePoints[clump.segment+1];
        const float dx=b.x-a.x,dz=b.z-a.z,length=std::hypot(dx,dz);
        const float x=a.x+dx*clump.fraction-dz/length*clump.side;
        const float z=a.z+dz*clump.fraction+dx/length*clump.side;
        const auto ground=support.Query(x,z);
        const auto route=gameplay::simulation::ProjectWorldRoute(x,z);
        if(!ground.found || !route.valid || route.distance<=2.4f || ground.normal[1]<.5f) {
            diagnostic="Understory has missing terrain or intrudes on the clear trail.";return false;
        }
        ForestUnderstoryPlacement placement;
        placement.supportY=ground.worldY;placement.scale=clump.scale;
        placement.surfaceNormal=ground.normal;
        placement.origin={x,ground.worldY-source.bounds.minimum[1]*clump.scale-.03f,z};
        placement.minimum.fill(std::numeric_limits<float>::max());
        placement.maximum.fill(std::numeric_limits<float>::lowest());
        const auto base=static_cast<std::uint32_t>(vertices.size());
        for(auto v:source.vertices) {
            auto p=OnSurface(Rotate({v.position[0]*clump.scale,v.position[1]*clump.scale,v.position[2]*clump.scale},clump.yaw),ground.normal);
            const auto n=OnSurface(Rotate({v.normal[0],v.normal[1],v.normal[2]},clump.yaw),ground.normal);
            const auto t=OnSurface(Rotate({v.tangent[0],v.tangent[1],v.tangent[2]},clump.yaw),ground.normal);
            for(unsigned axis=0;axis<3;++axis) {
                p[axis]+=placement.origin[axis];v.position[axis]=p[axis];v.normal[axis]=n[axis];v.tangent[axis]=t[axis];
                placement.minimum[axis]=std::min(placement.minimum[axis],p[axis]);
                placement.maximum[axis]=std::max(placement.maximum[axis],p[axis]);
            }
            vertices.push_back(v);
        }
        for(const auto index:source.indices)indices.push_back(base+index);
        candidate.triangles+=counts[clump.source];candidate.placements.push_back(placement);
    }
    if(owner.vertices.size()>UINT32_MAX-vertices.size() || owner.indices.size()>UINT32_MAX-indices.size()) {
        diagnostic="Understory exceeds native vertex/index addressing.";return false;
    }
    auto staged=owner;
    staged.primitives.push_back({static_cast<std::uint32_t>(staged.vertices.size()),
        static_cast<std::uint32_t>(staged.indices.size()),static_cast<std::uint32_t>(indices.size()),
        materialIndex,static_cast<std::uint32_t>(staged.nodeTransforms.size())});
    staged.nodeTransforms.push_back({"original-understory-world-baked",{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}}});
    staged.vertices.insert(staged.vertices.end(),vertices.begin(),vertices.end());
    staged.triangleRegions.resize(staged.indices.size()/3u,assets::StaticTriangleRegion::Shared);
    staged.triangleRegions.insert(staged.triangleRegions.end(),indices.size()/3u,assets::StaticTriangleRegion::ExteriorDressing);
    staged.indices.insert(staged.indices.end(),indices.begin(),indices.end());
    for(const auto& p:candidate.placements)for(unsigned axis=0;axis<3;++axis) {
        staged.bounds.minimum[axis]=std::min(staged.bounds.minimum[axis],p.minimum[axis]);
        staged.bounds.maximum[axis]=std::max(staged.bounds.maximum[axis],p.maximum[axis]);
    }
    owner=std::move(staged);report=std::move(candidate);diagnostic.clear();return true;
}
}
