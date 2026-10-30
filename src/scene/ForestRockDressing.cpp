#include "scene/ForestRockDressing.h"
#include "gameplay/simulation/DevelopmentWorldRoute.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace horde::scene {
namespace {
using Point=std::array<float,3>;
struct Placement {unsigned source,segment;float fraction,side,yaw,scale;};
constexpr std::array<Placement,12> kPlacements{{
    {0,2,.45f,-3.6f,.2f,1.f},{1,2,.61f,-4.1f,1.3f,.8f},{2,2,.72f,-4.3f,2.1f,.9f},
    {1,3,.23f,3.5f,.6f,.85f},{0,3,.48f,3.8f,1.4f,1.f},{2,3,.73f,-4.0f,2.3f,.9f},
    {2,4,.27f,3.7f,.5f,.95f},{0,4,.54f,3.8f,1.1f,1.f},{1,4,.76f,-3.8f,2.3f,.85f},
    {0,5,.31f,-3.5f,.4f,1.f},{2,6,.30f,3.8f,1.9f,.9f},{1,6,.57f,-3.8f,2.7f,.85f}}};
Point Transform(Point p,float yaw,Point up) {
    const float c=std::cos(yaw),s=std::sin(yaw);
    const Point q{c*p[0]+s*p[2],p[1],-s*p[0]+c*p[2]};
    const float len=std::hypot(up[0],up[1]);const Point right{up[1]/len,-up[0]/len,0};
    const Point forward{right[1]*up[2],-right[0]*up[2],right[0]*up[1]-right[1]*up[0]};
    return {right[0]*q[0]+up[0]*q[1]+forward[0]*q[2],right[1]*q[0]+up[1]*q[1]+forward[1]*q[2],
        right[2]*q[0]+up[2]*q[1]+forward[2]*q[2]};
}
}
bool AppendPreparedForestRocks(const std::filesystem::path& root,assets::StaticMeshAsset& owner,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>& support,
    ForestRockReport& report,std::string& diagnostic) {
    report={};
    constexpr const char* nodeName="prepared-forest-rocks-world-baked";
    const auto family=std::find_if(owner.materials.begin(),owner.materials.end(),[](const auto& m){return m.name=="Boulder01Rock";});
    if(!owner.HasValidTriangleProvenance() || !support || family==owner.materials.end() || owner.nodeTransforms.empty() || owner.primitives.empty() ||
       owner.primitives.size()>=32 || std::any_of(owner.nodeTransforms.begin(),owner.nodeTransforms.end(),
            [&](const auto& n){return n.name==nodeName;})) {
        diagnostic="Prepared rocks require the existing boulder family, ground and a fresh bounded admission.";return false;
    }
    const auto dir=root/"models/world/runtime/forest-rocks-v1";
    assets::AssetManifest manifest;if(!assets::AssetManifest::Load(dir/"asset.manifest.json",manifest,diagnostic))return false;
    constexpr std::array<const char*,3> files{{"horde-rock-low_boulder.glb","horde-rock-upright_crag.glb","horde-rock-split_shelf.glb"}};
    constexpr std::array<std::size_t,3> counts{{378,376,788}};
    std::array<assets::StaticMeshAsset,3> sources;
    for(std::size_t i=0;i<sources.size();++i) {
        auto& source=sources[i];
        if(!assets::StaticMeshAsset::Load(dir/files[i],manifest,source,diagnostic))return false;
        if(source.materials.size()!=1 || source.primitives.size()!=1 || source.indices.size()!=counts[i]*3 ||
           source.primitives[0].vertexOffset!=0 || source.primitives[0].indexOffset!=0 ||
           source.primitives[0].indexCount!=source.indices.size() || source.materials[0].transmissionFactor!=0 ||
           source.materials[0].metallicFactor!=0 || source.materials[0].emissiveFactor!=Point{}) {
            diagnostic="Prepared rock source lost its bounded opaque native identity.";return false;
        }
    }
    ForestRockReport candidate;
    std::vector<assets::StaticRtVertex> vertices;std::vector<std::uint32_t> indices;
    std::vector<gameplay::simulation::TriangleSupportSurface::Triangle> contact;
    for(const auto& authored:kPlacements) {
        const auto& source=sources[authored.source];
        const auto a=gameplay::simulation::kWorldRoutePoints[authored.segment],b=gameplay::simulation::kWorldRoutePoints[authored.segment+1];
        const float dx=b.x-a.x,dz=b.z-a.z,len=std::hypot(dx,dz);
        const float x=a.x+dx*authored.fraction-dz/len*authored.side,z=a.z+dz*authored.fraction+dx/len*authored.side;
        const auto ground=support->Query(x,z);
        if(!ground.found || ground.normal[1]<.5f || gameplay::simulation::ProjectWorldRoute(x,z).distance<=2.4f) {
            diagnostic="Prepared rock has missing support, excessive slope or a blocked trail.";return false;
        }
        ForestRockPlacement placed;placed.source=authored.source;placed.origin={x,ground.worldY-source.bounds.minimum[1]*authored.scale,z};
        // Survey the actual bottom perimeter against the exported terrain.
        // Bury that perimeter by 30mm rather than floating on a centre-only sample.
        float burial=0;
        for(const auto& v:source.vertices)if(v.position[1]<=source.bounds.minimum[1]+.12f*(source.bounds.maximum[1]-source.bounds.minimum[1])) {
            auto p=Transform({v.position[0]*authored.scale,v.position[1]*authored.scale,v.position[2]*authored.scale},authored.yaw,ground.normal);
            const auto under=support->Query(x+p[0],z+p[2]);
            if(!under.found){diagnostic="Rock perimeter leaves the real ground.";return false;}
            burial=std::max(burial,placed.origin[1]+p[1]-under.worldY);
        }
        placed.origin[1]-=burial+.03f;
        placed.minimum.fill(std::numeric_limits<float>::max());placed.maximum.fill(std::numeric_limits<float>::lowest());
        const auto base=static_cast<std::uint32_t>(vertices.size());
        for(auto v:source.vertices) {
            const auto p=Transform({v.position[0]*authored.scale,v.position[1]*authored.scale,v.position[2]*authored.scale},authored.yaw,ground.normal);
            const auto n=Transform({v.normal[0],v.normal[1],v.normal[2]},authored.yaw,ground.normal);
            const auto t=Transform({v.tangent[0],v.tangent[1],v.tangent[2]},authored.yaw,ground.normal);
            for(unsigned axis=0;axis<3;++axis) {
                v.position[axis]=p[axis]+placed.origin[axis];v.normal[axis]=n[axis];v.tangent[axis]=t[axis];
                placed.minimum[axis]=std::min(placed.minimum[axis],v.position[axis]);placed.maximum[axis]=std::max(placed.maximum[axis],v.position[axis]);
            }
            if(gameplay::simulation::ProjectWorldRoute(v.position[0],v.position[2]).distance<1.35f) {
                diagnostic="Rock geometry intrudes on the clear trail.";return false;
            }
            vertices.push_back(v);
        }
        for(std::size_t i=0;i<source.indices.size();i+=3) {
            gameplay::simulation::TriangleSupportSurface::Triangle triangle;
            triangle.surface=gameplay::simulation::SupportSurface::Stone;
            for(unsigned corner=0;corner<3;++corner) {
                const auto index=base+source.indices[i+corner];indices.push_back(index);
                const auto& p=vertices[index].position;triangle.points[corner]={p[0],p[1],p[2]};
            }
            contact.push_back(triangle);
        }
        candidate.triangles+=counts[authored.source];candidate.placements.push_back(placed);
    }
    auto prepared=gameplay::simulation::TriangleSupportSurface::WithAdditionalNonWalkableGeometry(*support,contact,diagnostic);
    if(!prepared)return false;
    if(owner.vertices.size()>UINT32_MAX-vertices.size() || owner.indices.size()>UINT32_MAX-indices.size()) {
        diagnostic="Prepared rocks exceed native addressing.";return false;
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
    owner=std::move(staged);support=std::move(prepared);report=std::move(candidate);diagnostic.clear();return true;
}
}
