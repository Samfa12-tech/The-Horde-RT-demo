#include "scene/assets/AssetManifest.h"
#include "scene/assets/StaticMeshAsset.h"
#include "scene/ShowcaseOverheadGeometry.h"
#include "vulkan/raytracing/RtStaticMeshSlot.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace
{
using namespace horde::scene::assets;
using namespace horde::vulkan::raytracing;
using Vec3 = std::array<float, 3>;
int failures = 0;
const std::filesystem::path root{HORDE_RT_SOURCE_ASSET_ROOT};
void Check(bool ok, const std::string& message)
{
    if (!ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
StaticMeshAsset Load(const std::filesystem::path& directory, const char* name)
{
    AssetManifest manifest;
    StaticMeshAsset result;
    std::string diagnostic;
    const bool loaded = AssetManifest::Load(root / directory / "asset.manifest.json", manifest, diagnostic) &&
        StaticMeshAsset::Load(root / directory / name, manifest, result, diagnostic);
    Check(loaded, directory.string() + ": " + diagnostic);
    return result;
}
Vec3 Sub(Vec3 a, Vec3 b) { return {a[0]-b[0], a[1]-b[1], a[2]-b[2]}; }
Vec3 Cross(Vec3 a, Vec3 b) { return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]}; }
float Dot(Vec3 a, Vec3 b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
bool RayOccluded(const StaticMeshAsset& asset, Vec3 origin, Vec3 ray)
{
    for (const auto& primitive : asset.primitives)
        for (std::uint32_t offset = 0; offset < primitive.indexCount; offset += 3)
        {
            const auto position = [&](unsigned corner) {
                const auto& p = asset.vertices.at(primitive.vertexOffset +
                    asset.indices.at(primitive.indexOffset + offset + corner)).position;
                return Vec3{p[0], p[1], p[2]};
            };
            const Vec3 a = position(0), edge1 = Sub(position(1), a), edge2 = Sub(position(2), a);
            const Vec3 p = Cross(ray, edge2);
            const float determinant = Dot(edge1, p);
            if (std::abs(determinant) < 1.0e-7f) continue;
            const float inverse = 1.0f / determinant;
            const Vec3 t = Sub(origin, a);
            const float u = Dot(t, p) * inverse;
            const Vec3 q = Cross(t, edge1);
            const float v = Dot(ray, q) * inverse;
            const float distance = Dot(edge2, q) * inverse;
            if (u >= 0 && v >= 0 && u + v <= 1 && distance > 0 && distance < 30)
                return true;
        }
    return false;
}
}

int main()
{
    std::vector<StaticMeshAsset> assets;
    assets.reserve(10);
    assets.push_back(Load("models/weapons/runtime", "gothic-arming-sword-rh-lod0.runtime.glb"));
    assets.push_back(Load("models/props/runtime", "gothic-hand-torch-lod0.runtime.glb"));
    assets.push_back(Load("models/player/runtime", "gothic-traveller-lod0.runtime.glb"));
    assets.push_back(Load("models/props/runtime/dielectric-fixture", "closed-glass-lod0.runtime.glb"));
    assets.push_back(Load("models/props/runtime/gothic-chest-base", "gothic-chest-base-lod0.runtime.glb"));
    assets.push_back(Load("models/props/runtime/gothic-chest-lid", "gothic-chest-lid-lod0.runtime.glb"));
    assets.push_back(Load("models/props/runtime/reward-lantern-ring", "reward-lantern-ring-lod0.runtime.glb"));
    assets.push_back(Load("models/props/runtime/reward-lantern-body", "reward-lantern-body-lod0.runtime.glb"));
    std::vector<StaticRtAssetRegistration> registrations;
    constexpr std::array<std::uint32_t, 8> ids{3,1,4,9,5,6,7,8};
    for (unsigned i=0; i<ids.size(); ++i)
        registrations.push_back({ids[i], ids[i]+100, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),
            i==1 ? 1u : 0u, &assets[i], nullptr,
            i==2 ? RtGeometryRole::PlayerWorldBody : RtGeometryRole::Static});
    if (std::filesystem::exists(root / "models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb"))
    {
        assets.push_back(Load("models/player/viewmodel/runtime", "gothic-traveller-viewmodel.runtime.glb"));
        registrations.push_back({20, 120, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),
            0, &assets.back(), &assets[2], RtGeometryRole::PlayerViewmodel});
    }
    RtStaticMeshSlot baseline;
    std::string diagnostic;
    Check(baseline.Initialize(registrations, diagnostic), "unchanged static assets initialize: " + diagnostic);
    assets.push_back(Load("models/world/runtime/collapsed-entry", "collapsed-entry-lod0.runtime.glb"));
    const auto& collapse = assets.back();
    if (failures) return 1;
    Check(collapse.primitives.size()==2 && collapse.materials.size()==2 && collapse.indices.size()==8422u*3u &&
          collapse.vertices.size()<=25266u, "approved core has exactly two primitives/families and 8422 triangles within split-vertex bound");
    if (failures) return 1;
    Check(collapse.materials[0].name=="Boulder01Rock" && collapse.materials[1].name=="MedievalWall02",
          "material order owns appended rock10 and masonry11 texture layers");
    Check(std::abs(collapse.materials[0].normalScale-0.42f)<0.00001f &&
          std::abs(collapse.materials[1].normalScale-0.34f)<0.00001f,
          "accepted restrained normal strengths survive real GLB admission");
    bool framesValid=true;
    for (const auto& vertex : collapse.vertices)
    {
        const Vec3 n{vertex.normal[0],vertex.normal[1],vertex.normal[2]};
        const Vec3 t{vertex.tangent[0],vertex.tangent[1],vertex.tangent[2]};
        framesValid &= std::isfinite(Dot(n,n)) && std::abs(Dot(n,n)-1)<0.005f &&
            std::abs(Dot(t,t)-1)<0.005f && std::abs(Dot(n,t))<0.005f &&
            std::abs(vertex.tangent[3])==1 && std::isfinite(vertex.uv0[0]) && std::isfinite(vertex.uv0[1]);
    }
    Check(framesValid, "real imported world-space normals/tangents/handedness/UVs remain valid");
    for (const auto& material : collapse.materials)
        Check(material.metallicFactor==0 && material.transmissionFactor==0 &&
              material.emissiveFactor==Vec3{} && material.baseColorTexture>=0 &&
              material.normalTexture>=0 && material.ormTexture>=0,
              "collapse is ordinary opaque, nonmetal PBR with all three authored map categories");
    Check(collapse.bounds.minimum[2]>1.85f && collapse.bounds.maximum[2]<=17.49f &&
          collapse.bounds.minimum[0]>=-2.01f && collapse.bounds.maximum[0]<=2.01f &&
          collapse.bounds.maximum[1]>12.0f, "world-baked enclosure preserves spawn and bounded deep stairwell");
    bool sealed=true;
    for (int x=-6; x<=6; ++x)
        for (int y=0; y<=7; ++y)
            sealed &= RayOccluded(collapse, {0,0.70f,1.85f}, {x*0.10f,y*0.05f,1.0f});
    Check(sealed, "real collapse/stair enclosure physically stops rearward opening rays without flat cap or inspection floor");
    registrations.push_back({21,0x434f4c4cu,static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),0,&collapse});
    RtStaticMeshSlot admitted;
    Check(admitted.Initialize(registrations,diagnostic), "full static registry admits collapse without special role: " + diagnostic);
    if (failures) return 1;
    const auto& metadata=admitted.InstanceMetadata()[21];
    Check(metadata.primitiveCount==2 && metadata.geometryRole==static_cast<std::uint32_t>(RtGeometryRole::Static) &&
          metadata.stableObjectId==0x434f4c4cu && metadata.emitterIndex==0,
          "appended instance21 retains ordinary static shading and physical shadow identity");
    if (failures) return 1;
    const auto firstMaterial=admitted.PrimitiveMetadata().at(metadata.primitiveBase).materialIndex -
        collapse.primitives[0].materialIndex;
    for (unsigned i=0;i<2;++i)
        Check(admitted.Materials().at(firstMaterial+i).textureLayers[0]==10+i &&
              admitted.Materials().at(firstMaterial+i).textureLayers[1]==10+i &&
              admitted.Materials().at(firstMaterial+i).textureLayers[2]==10+i,
              "all imported PBR categories match canonical appended atlas layers");
    Check(admitted.TextureArrayCounts().baseColor==12 && admitted.TextureArrayCounts().normal==12 &&
          admitted.TextureArrayCounts().orm==12 && admitted.Materials().size()+6<=kRtMaterialCapacity,
          "two families remain within unchanged texture/material bounds including dungeon materials");
    bool preserved=true;
    for (unsigned i=0;i<baseline.Materials().size();++i)
        preserved &= admitted.Materials()[i].textureLayers==baseline.Materials()[i].textureLayers;
    Check(preserved, "existing asset texture routes are unchanged after append");
    std::cout << "Collapse static admission: triangles=" << collapse.indices.size()/3 << " vertices=" << collapse.vertices.size()
              << " registry materials=" << admitted.Materials().size() << " primitives=" << admitted.PrimitiveMetadata().size() << '\n';
    return failures ? 1 : 0;
}
