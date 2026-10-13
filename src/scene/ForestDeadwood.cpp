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
}
bool AppendPreparedForestDeadwood(const std::filesystem::path& root,assets::StaticMeshAsset& owner,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>& support,
    ForestDeadwoodReport& report,std::string& diagnostic) {
    report={};constexpr const char* nodeName="prepared-deadwood-world-baked";
    const auto family=std::find_if(owner.materials.begin(),owner.materials.end(),[](const auto& m){return m.name=="ForestBark";});
    if(!owner.HasValidTriangleProvenance() || !support || family==owner.materials.end() || family->metallicFactor!=0 || family->transmissionFactor!=0 ||
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
    auto prepared=gameplay::simulation::TriangleSupportSurface::WithAdditionalNonWalkableGeometry(*support,contact,diagnostic);
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
    owner=std::move(staged);support=std::move(prepared);report=std::move(candidate);diagnostic.clear();return true;
}
}
