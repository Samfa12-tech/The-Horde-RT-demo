#include "scene/ForestRoadClue.h"
#include "gameplay/simulation/DevelopmentWorldRoute.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace horde::scene {
namespace {
using Point=std::array<float,3>;
struct Placement {unsigned source,segment;float fromClue,side;};
// Four short pairs follow the actual incoming/outgoing trail, with a gap at
// the bend. The separate marked stone faces the existing F03 listener point.
constexpr std::array<Placement,9> kPlacements{{
    {0,5,-2.3f,-1.85f},{0,5,-2.3f,1.85f},
    {0,5,-1.3f,-1.85f},{0,5,-1.3f,1.85f},
    {0,6,1.3f,-1.85f},{0,6,1.3f,1.85f},
    {0,6,2.3f,-1.85f},{0,6,2.3f,1.85f},
    {1,6,2.8f,2.15f}}};

Point Transform(Point p,float yaw,Point up) {
    const float c=std::cos(yaw),s=std::sin(yaw);
    const Point q{c*p[0]+s*p[2],p[1],-s*p[0]+c*p[2]};
    const float len=std::hypot(up[0],up[1]);
    const Point right{up[1]/len,-up[0]/len,0};
    const Point forward{right[1]*up[2],-right[0]*up[2],right[0]*up[1]-right[1]*up[0]};
    return {right[0]*q[0]+up[0]*q[1]+forward[0]*q[2],
        right[1]*q[0]+up[1]*q[1]+forward[1]*q[2],
        right[2]*q[0]+up[2]*q[1]+forward[2]*q[2]};
}
}

bool AppendForestRoadClue(const std::filesystem::path& root,assets::StaticMeshAsset& owner,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>& support,
    ForestRoadClueReport& report,std::string& diagnostic) {
    report={};
    constexpr const char* nodeName="original-forest-road-clue-world-baked";
    const auto family=std::find_if(owner.materials.begin(),owner.materials.end(),
        [](const auto& m){return m.name=="MedievalWall02";});
    if(!owner.HasValidTriangleProvenance() || !support || family==owner.materials.end() ||
       owner.primitives.empty() || owner.nodeTransforms.empty() || owner.primitives.size()>=32 ||
       std::any_of(owner.nodeTransforms.begin(),owner.nodeTransforms.end(),
           [&](const auto& n){return n.name==nodeName;})) {
        diagnostic="Road clue requires fresh bounded masonry ownership and actual terrain.";return false;
    }
    const auto dir=root/"models/world/runtime/forest-road-clue-v1";
    assets::AssetManifest manifest;
    if(!assets::AssetManifest::Load(dir/"asset.manifest.json",manifest,diagnostic))return false;
    constexpr std::array<const char*,2> files{{"horde-road-kerb.glb","horde-road-waystone.glb"}};
    constexpr std::array<std::size_t,2> counts{{12,48}};
    std::array<assets::StaticMeshAsset,2> sources;
    for(std::size_t i=0;i<sources.size();++i) {
        auto& source=sources[i];
        if(!assets::StaticMeshAsset::Load(dir/files[i],manifest,source,diagnostic))return false;
        if(source.materials.size()!=1 || source.primitives.size()!=1 || source.indices.size()!=counts[i]*3 ||
           source.primitives[0].vertexOffset!=0 || source.primitives[0].indexOffset!=0 ||
           source.primitives[0].indexCount!=source.indices.size()) {
            diagnostic="Road clue source lost its closed bounded static roster.";return false;
        }
        const auto& m=source.materials[0];
        if(m.name!="HordeRoadMasonry" || m.baseColorTexture>=0 || m.normalTexture>=0 ||
           m.ormTexture>=0 || m.emissiveTexture>=0 || m.metallicFactor!=0 ||
           m.transmissionFactor!=0 || m.emissiveFactor!=Point{} || source.bounds.minimum[1]!=0) {
            diagnostic="Road clue must reuse existing opaque masonry without new maps or lighting.";return false;
        }
    }
    ForestRoadClueReport candidate;
    candidate.minimumTrailClearance=std::numeric_limits<float>::max();
    candidate.minimumBottomGap=std::numeric_limits<float>::max();
    candidate.maximumBottomGap=std::numeric_limits<float>::lowest();
    std::vector<assets::StaticRtVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<gameplay::simulation::TriangleSupportSurface::Triangle> contact;
    const auto clue=gameplay::simulation::kWorldRoutePoints[6];
    for(const auto& authored:kPlacements) {
        const auto& source=sources[authored.source];
        const auto a=gameplay::simulation::kWorldRoutePoints[authored.segment];
        const auto b=gameplay::simulation::kWorldRoutePoints[authored.segment+1];
        const float dx=b.x-a.x,dz=b.z-a.z,len=std::hypot(dx,dz),fx=dx/len,fz=dz/len;
        const float x=clue.x+fx*authored.fromClue-fz*authored.side;
        const float z=clue.z+fz*authored.fromClue+fx*authored.side;
        const auto ground=support->Query(x,z);
        if(!ground.found || ground.normal[1]<.5f) {
            diagnostic="Road clue leaves actual terrain or exceeds its fitted slope.";return false;
        }
        const Point up=authored.source==1 ? Point{0,1,0} : ground.normal;
        const float yaw=authored.source==1 ? std::atan2(x-clue.x,z-clue.z) : std::atan2(dx,dz);
        ForestRoadCluePlacement placed;placed.source=authored.source;placed.origin={x,0,z};
        float fittedY=std::numeric_limits<float>::infinity();
        for(const auto& v:source.vertices)if(v.position[1]<=source.bounds.minimum[1]+.00001f) {
            const auto p=Transform({v.position[0],v.position[1],v.position[2]},yaw,up);
            const auto under=support->Query(x+p[0],z+p[2]);
            if(!under.found){diagnostic="Road clue footprint has missing native support.";return false;}
            fittedY=std::min(fittedY,under.worldY-p[1]-.03f);
        }
        if(!std::isfinite(fittedY)){diagnostic="Road clue has no authored bottom perimeter.";return false;}
        placed.origin[1]=fittedY;
        placed.minimum.fill(std::numeric_limits<float>::max());
        placed.maximum.fill(std::numeric_limits<float>::lowest());
        const auto base=static_cast<std::uint32_t>(vertices.size());
        for(auto v:source.vertices) {
            const bool bottom=v.position[1]<=source.bounds.minimum[1]+.00001f;
            const auto p=Transform({v.position[0],v.position[1],v.position[2]},yaw,up);
            const auto n=Transform({v.normal[0],v.normal[1],v.normal[2]},yaw,up);
            const auto t=Transform({v.tangent[0],v.tangent[1],v.tangent[2]},yaw,up);
            for(unsigned axis=0;axis<3;++axis) {
                v.position[axis]=p[axis]+placed.origin[axis];v.normal[axis]=n[axis];v.tangent[axis]=t[axis];
                placed.minimum[axis]=std::min(placed.minimum[axis],v.position[axis]);
                placed.maximum[axis]=std::max(placed.maximum[axis],v.position[axis]);
            }
            const float clearance=gameplay::simulation::ProjectWorldRoute(v.position[0],v.position[2]).distance;
            candidate.minimumTrailClearance=std::min(candidate.minimumTrailClearance,clearance);
            if(clearance<1.44f){diagnostic="Road clue narrows the authored trail plus capsule margin.";return false;}
            if(bottom) {
                const auto under=support->Query(v.position[0],v.position[2]);
                if(!under.found || v.position[1]<under.worldY-.30f) {
                    diagnostic="Road clue footprint is buried beyond its bounded terrain fit.";return false;
                }
                const Point position{v.position[0],v.position[1],v.position[2]};
                placed.bottomContacts.push_back(position);
                candidate.minimumBottomGap=std::min(candidate.minimumBottomGap,v.position[1]-under.worldY);
                candidate.maximumBottomGap=std::max(candidate.maximumBottomGap,v.position[1]-under.worldY);
            }
            vertices.push_back(v);
        }
        if(placed.maximum[1]<ground.worldY+(authored.source==1 ? .65f : .04f)) {
            diagnostic="Road clue is buried beyond its visible authored height.";return false;
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
        candidate.triangles+=counts[authored.source];candidate.placements.push_back(std::move(placed));
    }
    auto prepared=gameplay::simulation::TriangleSupportSurface::WithAdditionalNonWalkableGeometry(*support,contact,diagnostic);
    if(!prepared)return false;
    if(owner.vertices.size()>UINT32_MAX-vertices.size() || owner.indices.size()>UINT32_MAX-indices.size()) {
        diagnostic="Road clue exceeds existing native addressing.";return false;
    }
    auto staged=owner;
    staged.primitives.push_back({static_cast<std::uint32_t>(staged.vertices.size()),static_cast<std::uint32_t>(staged.indices.size()),
        static_cast<std::uint32_t>(indices.size()),static_cast<std::uint32_t>(family-owner.materials.begin()),
        static_cast<std::uint32_t>(staged.nodeTransforms.size())});
    staged.nodeTransforms.push_back({nodeName,{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}}});
    staged.vertices.insert(staged.vertices.end(),vertices.begin(),vertices.end());
    staged.triangleRegions.resize(staged.indices.size()/3u,assets::StaticTriangleRegion::Shared);
    staged.triangleRegions.insert(staged.triangleRegions.end(),indices.size()/3u,assets::StaticTriangleRegion::ExteriorDressing);
    staged.indices.insert(staged.indices.end(),indices.begin(),indices.end());
    for(const auto& placed:candidate.placements)for(unsigned axis=0;axis<3;++axis) {
        staged.bounds.minimum[axis]=std::min(staged.bounds.minimum[axis],placed.minimum[axis]);
        staged.bounds.maximum[axis]=std::max(staged.bounds.maximum[axis],placed.maximum[axis]);
    }
    candidate.vertexBytes=vertices.size()*sizeof(assets::StaticRtVertex);
    candidate.indexBytes=indices.size()*sizeof(std::uint32_t);
    owner=std::move(staged);support=std::move(prepared);report=std::move(candidate);diagnostic.clear();return true;
}
}
