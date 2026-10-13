#include "scene/GraveyardDressing.h"
#include "scene/assets/AssetManifest.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <string_view>

namespace horde::scene {
namespace {
using Surface=gameplay::simulation::TriangleSupportSurface;
using namespace assets;
struct Spec {const char* name; const char* path; std::size_t triangles;};
constexpr std::array<Spec,6> kSources{{
    {"round marker","models/world/runtime/grave-markers-r03/grave-marker-round.runtime.glb",292},
    {"damaged marker","models/world/runtime/grave-markers-r03/grave-marker-damaged.runtime.glb",272},
    {"T03 displaced cover","models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_displaced_lid.glb",496},
    {"T03 offering bowl","models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_offering_bowl.glb",960},
    {"T03 broken urn","models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_urn_broken_base.glb",1212},
    {"T03 cold candle","models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_candle_stub_1.glb",664},
}};
struct Placement {unsigned source; float x,z,yaw,scale;};
// Irregular old burial patch around the grave lid/rope, away from the trail.
// Covers remain flat worn pieces; no new loot, lore, light or flame.
constexpr std::array<Placement,12> kPlacements{{
    {0,-36.8f,-12.8f,1.37f,1}, {1,-37.0f,-16.0f,1.5f,1},
    {0,-34.8f,-19.1f,.35f,.92f}, {1,-32.5f,-18.9f,-.25f,1},
    {0,-30.9f,-13.2f,-1.57f,.92f}, {1,-34.8f,-8.5f,2.85f,.85f},
    {2,-36.0f,-12.8f,1.37f,.75f}, {2,-36.2f,-16.0f,1.5f,.75f},
    {3,-34.55f,-18.75f,.10f,.65f}, {4,-32.8f,-18.8f,.35f,.60f},
    {5,-30.75f,-12.95f,-.20f,.85f}, {5,-34.9f,-8.20f,.12f,.85f},
}};
constexpr std::array<std::string_view,4> kMaterials{{"MedievalWall02","TombBoneWax","TombEarthenware","TombCharredWick"}};
unsigned Family(unsigned source,const StaticMaterial& material) {
    if(source<=2)return 0;
    if(source==3 || source==4)return 2;
    return material.name=="Original_charred_wick"?3:1;
}
Surface::Point Rotate(Surface::Point p,float yaw) {
    const float c=std::cos(yaw),s=std::sin(yaw);
    return {c*p[0]+s*p[2],p[1],-s*p[0]+c*p[2]};
}
void Normalize(Surface::Point& p) {
    const float length=std::sqrt(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);
    for(auto& value:p)value/=length;
}
}
bool AppendPreparedGraveyard(const std::filesystem::path& root, StaticMeshAsset& owner,
    std::shared_ptr<const Surface>& support, GraveyardDressingReport& report,std::string& diagnostic) {
    if(!support || support->NonWalkableTriangleCount() || owner.nodeTransforms.empty() ||
        owner.primitives.empty() || owner.primitives.size()+4>32 || owner.materials.size()>32 ||
        !owner.HasValidTriangleProvenance()) {
        diagnostic="Graveyard requires an unmodified admitted terrain/static owner within existing capacity.";return false;
    }
    std::array<std::uint32_t,4> materials{};
    for(std::size_t i=0;i<kMaterials.size();++i) {
        const auto found=std::find_if(owner.materials.begin(),owner.materials.end(),
            [&](const StaticMaterial& m){return m.name==kMaterials[i];});
        if(found==owner.materials.end()) {diagnostic="Graveyard requires the admitted tomb material families.";return false;}
        materials[i]=static_cast<std::uint32_t>(found-owner.materials.begin());
    }
    std::array<StaticMeshAsset,kSources.size()> loaded;
    for(std::size_t i=0;i<loaded.size();++i) {
        AssetManifest manifest;manifest.schema=1;manifest.assetName=kSources[i].name;
        manifest.metresPerUnit=1;manifest.upAxis="+Y";manifest.forwardAxis="+Z";
        manifest.budgets={10000,30000,4,2,1};manifest.lods.push_back({"lod0",10000});
        manifest.textureProfile={"astc","rgba8",true};
        if(i<2) {
            auto path=root/kSources[i].path;
            path.replace_filename(i==0?"grave-marker-round.manifest.json":"grave-marker-damaged.manifest.json");
            if(!AssetManifest::Load(path,manifest,diagnostic))return false;
        }
        if(!StaticMeshAsset::Load(root/kSources[i].path,manifest,loaded[i],diagnostic))return false;
        if(loaded[i].indices.size()/3!=kSources[i].triangles || loaded[i].materials.empty()) {
            diagnostic="Graveyard source triangle/material evidence changed.";return false;
        }
        for(const auto& m:loaded[i].materials)
            if(m.baseColorTexture>=0 || m.normalTexture>=0 || m.ormTexture>=0 || m.emissiveTexture>=0 ||
                m.metallicFactor!=0 || m.transmissionFactor!=0 || m.emissiveFactor!=std::array<float,3>{}) {
                diagnostic="Graveyard source requires unadmitted texture, emissive or optical features.";return false;
            }
    }
    std::array<std::vector<StaticRtVertex>,4> vertices;
    std::array<std::vector<std::uint32_t>,4> indices;
    std::vector<Surface::Triangle> contacts;
    GraveyardDressingReport candidate;
    for(const auto& authored:kPlacements) {
        const auto& source=loaded[authored.source];
        const auto centre=support->Query(authored.x,authored.z);
        const auto east=support->Query(authored.x+.20f,authored.z),west=support->Query(authored.x-.20f,authored.z);
        const auto north=support->Query(authored.x,authored.z+.20f),south=support->Query(authored.x,authored.z-.20f);
        if(!centre.found || !centre.walkable || !east.found || !west.found || !north.found || !south.found) {
            diagnostic="Graveyard footprint has missing or nonwalkable ground.";return false;
        }
        // Local plane follows actual exported ground, never a flat floor proxy.
        const float gx=(east.worldY-west.worldY)/.40f,gz=(north.worldY-south.worldY)/.40f;
        float burial=.03f;
        for(const auto& v:source.vertices) {
            auto p=Rotate({v.position[0]*authored.scale,v.position[1]*authored.scale,v.position[2]*authored.scale},authored.yaw);
            const auto ground=support->Query(authored.x+p[0],authored.z+p[2]);
            if(!ground.found || !ground.walkable) {diagnostic="Graveyard piece extends beyond valid terrain.";return false;}
            burial=std::max(burial,centre.worldY+gx*p[0]+gz*p[2]-ground.worldY+.03f);
        }
        if(burial>.20f) {diagnostic="Graveyard placement needs excessive burial: "+std::string(kSources[authored.source].name)+
            " at "+std::to_string(authored.x)+","+std::to_string(authored.z)+" needs "+std::to_string(burial)+"m.";return false;}
        GraveyardPlacement placement;
        placement.asset=kSources[authored.source].name;placement.yaw=authored.yaw;placement.burial=burial;
        placement.origin={authored.x,centre.worldY-source.bounds.minimum[1]*authored.scale-burial,authored.z};
        placement.minimum.fill(std::numeric_limits<float>::max());placement.maximum.fill(std::numeric_limits<float>::lowest());
        for(std::size_t pi=0;pi<source.primitives.size();++pi) {
            const auto& primitive=source.primitives[pi];
            const auto end=pi+1<source.primitives.size()?source.primitives[pi+1].vertexOffset:source.vertices.size();
            const auto family=Family(authored.source,source.materials[primitive.materialIndex]);
            const auto base=static_cast<std::uint32_t>(vertices[family].size());
            for(auto vi=primitive.vertexOffset;vi<end;++vi) {
                auto v=source.vertices[vi];
                auto p=Rotate({v.position[0]*authored.scale,v.position[1]*authored.scale,v.position[2]*authored.scale},authored.yaw);
                p[1]+=gx*p[0]+gz*p[2];
                auto n=Rotate({v.normal[0],v.normal[1],v.normal[2]},authored.yaw);
                n[0]-=gx*n[1];n[2]-=gz*n[1];Normalize(n);
                auto t=Rotate({v.tangent[0],v.tangent[1],v.tangent[2]},authored.yaw);
                t[1]+=gx*t[0]+gz*t[2];Normalize(t);
                for(unsigned axis=0;axis<3;++axis) {
                    v.position[axis]=p[axis]+placement.origin[axis];v.normal[axis]=n[axis];v.tangent[axis]=t[axis];
                    placement.minimum[axis]=std::min(placement.minimum[axis],v.position[axis]);
                    placement.maximum[axis]=std::max(placement.maximum[axis],v.position[axis]);
                }
                vertices[family].push_back(v);
            }
            for(std::size_t n=0;n<primitive.indexCount;n+=3) {
                Surface::Triangle triangle;triangle.surface=gameplay::simulation::SupportSurface::Stone;
                for(unsigned corner=0;corner<3;++corner) {
                    const auto local=source.indices[primitive.indexOffset+n+corner];
                    if(local>=end-primitive.vertexOffset) {diagnostic="Graveyard source index span is invalid.";return false;}
                    const auto index=base+local;indices[family].push_back(index);
                    const auto& p=vertices[family][index].position;triangle.points[corner]={p[0],p[1],p[2]};
                }
                contacts.push_back(triangle);
            }
        }
        candidate.placements.push_back(placement);
    }
    auto prepared=Surface::WithNonWalkableGeometry(*support,contacts,diagnostic);
    if(!prepared)return false;
    auto staged=owner;
    for(std::size_t family=0;family<4;++family) {
        if(vertices[family].empty())continue;
        if(staged.vertices.size()>std::numeric_limits<std::uint32_t>::max()-vertices[family].size() ||
            staged.indices.size()>std::numeric_limits<std::uint32_t>::max()-indices[family].size()) {
            diagnostic="Graveyard geometry exceeds native address capacity.";return false;
        }
        StaticPrimitiveRecord primitive;
        primitive.vertexOffset=static_cast<std::uint32_t>(staged.vertices.size());
        primitive.indexOffset=static_cast<std::uint32_t>(staged.indices.size());
        primitive.indexCount=static_cast<std::uint32_t>(indices[family].size());primitive.materialIndex=materials[family];
        primitive.nodeTransformIndex=static_cast<std::uint32_t>(staged.nodeTransforms.size());
        staged.triangleRegions.resize(staged.indices.size()/3u,StaticTriangleRegion::Shared);
        staged.triangleRegions.insert(staged.triangleRegions.end(),primitive.indexCount/3u,
            StaticTriangleRegion::ExteriorDressing);
        staged.vertices.insert(staged.vertices.end(),vertices[family].begin(),vertices[family].end());
        staged.indices.insert(staged.indices.end(),indices[family].begin(),indices[family].end());
        staged.primitives.push_back(primitive);
        staged.nodeTransforms.push_back({"graveyard-world-baked",{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}}});
    }
    for(const auto& placement:candidate.placements)for(unsigned axis=0;axis<3;++axis) {
        staged.bounds.minimum[axis]=std::min(staged.bounds.minimum[axis],placement.minimum[axis]);
        staged.bounds.maximum[axis]=std::max(staged.bounds.maximum[axis],placement.maximum[axis]);
    }
    candidate.triangles=contacts.size();candidate.contactTriangles=prepared->NonWalkableTriangleCount();
    owner=std::move(staged);support=std::move(prepared);report=std::move(candidate);
    diagnostic.clear();return true;
}
}
