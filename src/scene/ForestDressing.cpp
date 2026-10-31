#include "scene/ForestDressing.h"

#include "gameplay/simulation/ForestTreePlacementContract.h"
#include "gameplay/simulation/DevelopmentWorldRoute.h"
#include "scene/assets/AssetManifest.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace horde::scene
{
namespace
{

constexpr std::size_t kMaximumPrimitiveCount = 32u;
constexpr std::size_t kMaximumMaterialCount = 32u;
constexpr float kTrunkCollisionHeight = 2.0f;

enum class TreeId : std::size_t { Pine, Alder, Count };
enum class MaterialFamily : std::size_t { Bark, Pine, Moss, Alder, Count };

struct AssetSpec
{
    const char* name;
    const char* relativePath;
    std::size_t expectedTriangles;
    std::size_t expectedMaterials;
};

constexpr std::array<AssetSpec, static_cast<std::size_t>(TreeId::Count)> kAssets{{
    {"pine", "models/world/runtime/forest-tree-pair-v1/horde-irregular-pine-v1-lod1.glb", 5000u, 3u},
    {"alder", "models/world/runtime/forest-tree-pair-v1/horde-upright-alder-v1-lod1.glb", 5102u, 3u},
}};

constexpr std::array<std::string_view, static_cast<std::size_t>(MaterialFamily::Count)> kFamilyNames{{
    "ForestBark", "ForestPine", "ForestMoss", "ForestAlder",
}};
constexpr std::array<std::int32_t, static_cast<std::size_t>(MaterialFamily::Count)> kTextureGroups{{
    0, 1, 2, 3,
}};
constexpr std::array<std::string_view, 2u> kSharedMasterNodes{{
    "forest-dressing-shared-pine", "forest-dressing-shared-alder",
}};
constexpr std::array<float, 16u> kIdentity{{
    1.f,0.f,0.f,0.f, 0.f,1.f,0.f,0.f, 0.f,0.f,1.f,0.f, 0.f,0.f,0.f,1.f,
}};
struct F01AuthoringRoot {std::size_t master;float x,z,yaw;};
// Exact bounded diagnostic proposal from HORDE_1_7_F01_BACKGROUND_SECTOR_2026_10_24.
// These do not amend the existing64 collision contract or introduce tree assets.
constexpr std::array<F01AuthoringRoot,12u> kF01BackgroundRoots{{
    {0,-39,-37,.2f},{1,-30,-36,1.1f},{0,-47,-33,2.f},{0,-54,-26,.8f},
    {1,-58,-17,2.5f},{0,-57,-8,1.7f},{0,-42,-45,1.4f},{0,-29,-44,2.9f},
    {1,-55,-39,.4f},{0,-64,-30,2.4f},{0,-67,-17,1.f},{1,-66,-5,3.1f},
}};
// Separate bounded composition study: same twelve references/species budget,
// grouped into three stands rather than the retained sparse arc above. This
// does not move any original64 root or claim supplemental collision admission.
constexpr std::array<F01AuthoringRoot,12u> kF01GroupedRootsR4{{
    {0,-39,-34,.2f},{1,-42,-36,1.1f},{0,-37,-39,2.f},{0,-43,-41,.8f},
    {1,-54,-17,2.5f},{0,-57,-20,1.7f},{0,-58,-14,1.4f},{0,-61,-18,2.9f},
    {1,-53,-32,.4f},{0,-57,-34,2.4f},{0,-58,-30,1.f},{1,-60,-36,3.1f},
}};

// Independent four-root prototype authoring. Older76 layouts above remain
// frozen controls; selection is exclusive even though the north stand coincides.
constexpr std::array<F01AuthoringRoot,4u> kF01NorthGroveRoots{{
    {0,-39,-34,.2f},{1,-42,-36,1.1f},{0,-37,-39,2.f},{0,-43,-41,.8f},
}};

bool SameForestMaterial(const assets::StaticMaterial& a,const assets::StaticMaterial& b)
{
    const auto fields=[](const assets::StaticMaterial& m) {
        return std::tie(m.name,m.baseColorFactor,m.emissiveFactor,m.attenuationColor,m.emissiveStrength,
            m.metallicFactor,m.roughnessFactor,m.occlusionStrength,m.normalScale,m.textureScale,
            m.transmissionFactor,m.ior,m.thicknessFactor,m.attenuationDistance,m.numericalSpawnMinimumWidth,
            m.numericalSpawnGeometryError,m.baseColorTexture,m.normalTexture,m.ormTexture,m.emissiveTexture,
            m.textureGroup,m.deferTextureAllocation,m.flags);
    };
    return fields(a)==fields(b);
}

bool ValidForestFamilies(const PreparedForestDressing& prepared,std::string& diagnostic)
{
    for(std::size_t family=0;family<kFamilyNames.size();++family) {
        const auto& m=prepared.materialFamilies[family];
        if(m.name!=kFamilyNames[family] || m.textureGroup!=kTextureGroups[family] || !m.deferTextureAllocation ||
           m.baseColorTexture!=0 || m.emissiveTexture!=-1 || m.metallicFactor!=0.f || m.transmissionFactor!=0.f ||
           m.emissiveFactor!=std::array<float,3>{} ||
           m.normalTexture!=(family==0?0:-1) || m.ormTexture!=(family==0?0:-1)) {
            diagnostic="Prepared forest has an invalid canonical material family.";return false;
        }
    }
    return true;
}

bool ValidSharedForestOwner(const assets::StaticMeshAsset& owner,std::string& diagnostic)
{
    if(!owner.HasValidTriangleProvenance() || owner.vertices.empty() || owner.indices.empty() ||
       owner.primitives.empty() || owner.primitives.size()>kMaximumPrimitiveCount || owner.materials.empty() || owner.nodeTransforms.empty() ||
       owner.vertices.size()>UINT32_MAX || owner.indices.size()>UINT32_MAX || owner.materials.size()>kMaximumMaterialCount) {
        diagnostic="Shared forest requires an existing bounded non-empty static owner.";return false;
    }
    for(std::size_t axis=0;axis<3;++axis)if(!std::isfinite(owner.bounds.minimum[axis]) ||
        !std::isfinite(owner.bounds.maximum[axis]) || owner.bounds.minimum[axis]>owner.bounds.maximum[axis]) {
        diagnostic="Shared forest requires finite common-world bounds.";return false;
    }
    return true;
}

bool ValidSharedForestPrepared(const PreparedForestDressing& prepared,std::string& diagnostic)
{
    if(!ValidForestFamilies(prepared,diagnostic))return false;
    for(std::size_t tree=0;tree<prepared.masters.size();++tree) {
        const auto& source=prepared.masters[tree];
        if(source.vertices.empty() || source.indices.size()!=kAssets[tree].expectedTriangles*3u ||
           !source.HasValidTriangleProvenance() || source.primitives.size()!=3u || source.materials.size()!=3u ||
           source.nodeTransforms.empty()) {
            diagnostic="Shared forest master violates its admitted local payload contract.";return false;
        }
        ForestDressingBounds bounds;
        bounds.minimum.fill(std::numeric_limits<float>::max());bounds.maximum.fill(std::numeric_limits<float>::lowest());
        const auto finite=[](const auto& values){return std::all_of(values.begin(),values.end(),[](float x){return std::isfinite(x);});};
        for(const auto& vertex:source.vertices) {
            if(!finite(vertex.position) || !finite(vertex.normal) || !finite(vertex.tangent) || !finite(vertex.uv0)) {
                diagnostic="Shared forest master contains a non-finite vertex basis.";return false;
            }
            for(std::size_t axis=0;axis<3;++axis) {
                bounds.minimum[axis]=std::min(bounds.minimum[axis],vertex.position[axis]);
                bounds.maximum[axis]=std::max(bounds.maximum[axis],vertex.position[axis]);
            }
        }
        if(bounds.minimum!=source.bounds.minimum || bounds.maximum!=source.bounds.maximum) {
            diagnostic="Shared forest master local bounds do not match its normalized vertices.";return false;
        }
        for(const auto& node:source.nodeTransforms)if(!finite(node.world)) {
            diagnostic="Shared forest source has a non-finite imported node transform.";return false;
        }
        std::size_t consumed=0;
        for(std::size_t i=0;i<source.primitives.size();++i) {
            const auto& p=source.primitives[i];
            const auto end=i+1<source.primitives.size()?source.primitives[i+1].vertexOffset:source.vertices.size();
            const auto family=prepared.primitiveFamilies[tree][i];
            if((i==0 && p.vertexOffset!=0) || p.vertexOffset>=end || end>source.vertices.size() ||
               p.indexOffset!=consumed || !p.indexCount || p.indexCount%3u ||
               p.indexOffset>source.indices.size() || p.indexCount>source.indices.size()-p.indexOffset ||
               p.nodeTransformIndex>=source.nodeTransforms.size() || p.materialIndex>=source.materials.size() ||
               family>=prepared.materialFamilies.size() ||
               !SameForestMaterial(source.materials[p.materialIndex],prepared.materialFamilies[family]) ||
               std::any_of(source.indices.begin()+p.indexOffset,source.indices.begin()+p.indexOffset+p.indexCount,
                   [&](std::uint32_t index){return index>=end-p.vertexOffset;})) {
                diagnostic="Shared forest master has an invalid primitive span or canonical material mapping.";return false;
            }
            consumed+=p.indexCount;
        }
        if(consumed!=source.indices.size()) {
            diagnostic="Shared forest master has unowned trailing indices.";return false;
        }
    }
    for(std::size_t i=0;i<prepared.placements.size();++i) {
        const auto& p=prepared.placements[i];const auto& contract=gameplay::simulation::kForestTreePlacementContract[i];
        const auto master=contract.species==gameplay::simulation::ForestTreeSpecies::Pine?0u:1u;
        if(p.masterIndex!=master || p.origin[0]!=contract.x || p.origin[2]!=contract.z ||
           p.yawRadians!=contract.yawRadians || !std::isfinite(p.supportY) ||
           p.origin[1]!=p.supportY-prepared.masters[master].bounds.minimum[1]-contract.rootBurialMetres) {
            diagnostic="Shared forest placement is not the admitted immutable root transform.";return false;
        }
    }
    return true;
}

MaterialFamily ClassifyMaterial(const assets::StaticMaterial& material,
                               std::string& diagnostic)
{
    const std::string_view name = material.name;
    if (name == "Original charcoal-gray ridged bark") return MaterialFamily::Bark;
    if (name == "Original opaque pine needle sprays") return MaterialFamily::Pine;
    if (name == "Original opaque hanging woodland moss") return MaterialFamily::Moss;
    if (name == "Original opaque alder leaves") return MaterialFamily::Alder;
    diagnostic = "Forest source contains an unclassified material '" + material.name + "'.";
    return MaterialFamily::Count;
}

std::array<float, 3u> RotateY(const std::array<float, 3u>& value, float radians)
{
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    return {cosine * value[0] + sine * value[2], value[1],
            -sine * value[0] + cosine * value[2]};
}

struct ForestRootMeasurement
{
    float unburiedOriginY=0.f,fittedOriginY=0.f;
    std::size_t tipCount=0;
};

bool MeasureForestRootFootprint(const assets::StaticMeshAsset& source,float x,float z,float yaw,float supportY,
    const gameplay::simulation::TriangleSupportSurface& terrain,ForestRootMeasurement& measured,std::string& diagnostic)
{
    measured={supportY-source.bounds.minimum[1],supportY-source.bounds.minimum[1],0};
    for(const auto& vertex:source.vertices) {
        if(vertex.position[1]>source.bounds.minimum[1]+.02f)continue;
        const auto tip=RotateY({{vertex.position[0],vertex.position[1],vertex.position[2]}},yaw);
        const auto ground=terrain.Query(x+tip[0],z+tip[2]);
        if(!ground.found) {diagnostic="Forest root footprint has missing terrain support.";return false;}
        if(!std::isfinite(ground.worldY)) {diagnostic="Forest root footprint has non-finite terrain support.";return false;}
        measured.fittedOriginY=std::min(measured.fittedOriginY,ground.worldY-tip[1]-.02f);
        ++measured.tipCount;
    }
    return true;
}

bool AdmittedForestRootFootprint(const ForestRootMeasurement& measured)
{
    return std::isfinite(measured.unburiedOriginY) && std::isfinite(measured.fittedOriginY) &&
        measured.tipCount>=18u && measured.unburiedOriginY-measured.fittedOriginY<=.65f;
}

void IncludePoint(ForestDressingBounds& bounds, const std::array<float, 3u>& point)
{
    for (std::size_t axis = 0u; axis < 3u; ++axis)
    {
        bounds.minimum[axis] = std::min(bounds.minimum[axis], point[axis]);
        bounds.maximum[axis] = std::max(bounds.maximum[axis], point[axis]);
    }
}

std::vector<std::array<float,3u>> ClipTrunkTriangle(
                          std::vector<std::array<float,3u>> polygon, float floor)
{
    // Clip actual bark surfaces to the collision slab. Merely dropping
    // below-ground vertices misses edges that cross the terrain plane.
    for (int plane=0;plane<2;++plane) {
        const float height=floor+(plane==0 ? 0.f : kTrunkCollisionHeight);
        std::vector<std::array<float,3u>> clipped;
        for(std::size_t i=0;i<polygon.size();++i) {
            const auto a=polygon[i],b=polygon[(i+1)%polygon.size()];
            const bool inA=plane==0 ? a[1]>=height : a[1]<=height;
            const bool inB=plane==0 ? b[1]>=height : b[1]<=height;
            if(inA)clipped.push_back(a);
            if(inA!=inB) {
                const float t=(height-a[1])/(b[1]-a[1]);
                clipped.push_back({a[0]+t*(b[0]-a[0]),height,a[2]+t*(b[2]-a[2])});
            }
        }
        polygon=std::move(clipped);
    }
    return polygon;
}
void IncludeTrunkTriangle(ForestDressingBounds& bounds,
                          std::vector<std::array<float,3u>> polygon,float floor)
{
    for(const auto& point:ClipTrunkTriangle(std::move(polygon),floor))IncludePoint(bounds,point);
}

bool MeasureForestPlacementBounds(const PreparedForestDressing& prepared,
    ForestDressingSupplementalPlacement& receipt,std::string& diagnostic)
{
    const auto& placed=receipt.placement;const auto& source=prepared.masters[placed.masterIndex];
    receipt.worldBounds.minimum.fill(std::numeric_limits<float>::max());
    receipt.worldBounds.maximum.fill(std::numeric_limits<float>::lowest());
    receipt.trunkBounds=receipt.worldBounds;
    const auto worldPoint=[&](const assets::StaticRtVertex& vertex) {
        auto p=RotateY({{vertex.position[0],vertex.position[1],vertex.position[2]}},placed.yawRadians);
        for(std::size_t axis=0;axis<3;++axis)p[axis]+=placed.origin[axis];
        return p;
    };
    for(const auto& vertex:source.vertices)IncludePoint(receipt.worldBounds,worldPoint(vertex));
    for(std::size_t i=0;i<source.primitives.size();++i) {
        if(prepared.primitiveFamilies[placed.masterIndex][i]!=static_cast<std::uint8_t>(MaterialFamily::Bark))continue;
        const auto& primitive=source.primitives[i];
        for(std::size_t index=0;index<primitive.indexCount;index+=3) {
            std::vector<std::array<float,3>> triangle;
            for(std::size_t corner=0;corner<3;++corner)
                triangle.push_back(worldPoint(source.vertices[primitive.vertexOffset+source.indices[primitive.indexOffset+index+corner]]));
            IncludeTrunkTriangle(receipt.trunkBounds,std::move(triangle),placed.supportY);
        }
    }
    for(const auto* bounds:{&receipt.worldBounds,&receipt.trunkBounds})for(std::size_t axis=0;axis<3;++axis)
        if(!std::isfinite(bounds->minimum[axis]) || !std::isfinite(bounds->maximum[axis]) ||
           bounds->minimum[axis]>bounds->maximum[axis]) {
            diagnostic="Forest authoring root has invalid measured world/trunk bounds.";return false;
        }
    return true;
}

assets::StaticMaterial MakeFamilyMaterial(const assets::StaticMaterial& source,
                                         MaterialFamily family)
{
    assets::StaticMaterial material = source;
    const std::size_t index = static_cast<std::size_t>(family);
    material.name = std::string(kFamilyNames[index]);
    // Array layer identity is supplied by the canonical family group. Source
    // texture indices are asset-local and are not global array layer numbers.
    material.baseColorTexture = source.baseColorTexture >= 0 ? 0 : -1;
    material.normalTexture = source.normalTexture >= 0 ? 0 : -1;
    material.ormTexture = source.ormTexture >= 0 ? 0 : -1;
    material.emissiveTexture = -1;
    material.textureGroup = kTextureGroups[index];
    material.deferTextureAllocation = true;
    return material;
}

bool CanAppendForest(const assets::StaticMeshAsset& existingOwner,std::string& diagnostic)
{
    if (!existingOwner.HasValidTriangleProvenance() || existingOwner.vertices.empty() || existingOwner.indices.empty() ||
        existingOwner.primitives.empty() || existingOwner.materials.empty() ||
        existingOwner.nodeTransforms.empty())
    {
        diagnostic = "Forest dressing requires the existing non-empty collapsed static owner.";
        return false;
    }
    if (existingOwner.materials.size() + kFamilyNames.size() > kMaximumMaterialCount ||
        existingOwner.primitives.size() + kFamilyNames.size() > kMaximumPrimitiveCount)
    {
        diagnostic = "Forest dressing cannot reserve four material and primitive groups within the shared 32-entry caps.";
        return false;
    }
    if (std::any_of(kFamilyNames.begin(), kFamilyNames.end(), [&](std::string_view name) {
            return std::any_of(existingOwner.materials.begin(), existingOwner.materials.end(),
                [name](const assets::StaticMaterial& material) { return material.name == name; });
        }))
    {
        diagnostic = "Forest dressing has already been appended to this static asset owner.";
        return false;
    }

    return true;
}

} // namespace

bool PrepareForestDressing(const std::filesystem::path& assetRoot,
                          std::shared_ptr<const PreparedForestDressing>& prepared,
                          std::string& diagnostic,
                          const gameplay::simulation::TriangleSupportSurface* terrain)
{
    auto candidate = std::make_shared<PreparedForestDressing>();
    auto& sources = candidate->masters;
    for (std::size_t tree = 0u; tree < kAssets.size(); ++tree)
    {
        const auto& spec = kAssets[tree];
        assets::StaticMeshAsset& source = sources[tree];
        const auto path = assetRoot / spec.relativePath;
        assets::AssetManifest manifest;
        if (!assets::AssetManifest::Load(path.parent_path() / "asset.manifest.json", manifest, diagnostic) ||
            !assets::StaticMeshAsset::Load(path, manifest, source, diagnostic))
        {
            diagnostic = "Forest native import failed for " + std::string(spec.name) + ": " + diagnostic;
            return false;
        }
        std::size_t triangles = 0u;
        for (const auto& primitive : source.primitives) triangles += primitive.indexCount / 3u;
        if (triangles != spec.expectedTriangles || source.materials.size() != spec.expectedMaterials ||
            source.primitives.size() != spec.expectedMaterials)
        {
            diagnostic = "Forest source triangle/material/primitive evidence changed for " + std::string(spec.name) + ".";
            return false;
        }
        for (const auto& material : source.materials)
        {
            const auto family = ClassifyMaterial(material, diagnostic);
            if (family == MaterialFamily::Count) return false;
            const bool bark = family == MaterialFamily::Bark;
            if (material.baseColorTexture < 0 || (bark != (material.normalTexture >= 0)) ||
                (bark != (material.ormTexture >= 0)) || material.emissiveTexture >= 0 ||
                material.metallicFactor != 0.0f || material.transmissionFactor != 0.0f ||
                material.emissiveFactor != std::array<float, 3u>{})
            {
                diagnostic = "Forest material texture presence or opaque dielectric PBR contract changed.";
                return false;
            }
        }
    }

    auto& familyMaterials = candidate->materialFamilies;
    std::array<bool, static_cast<std::size_t>(MaterialFamily::Count)> hasMaterial{};
    for (const auto& source : sources)
    {
        for (const auto& material : source.materials)
        {
            const auto family = ClassifyMaterial(material, diagnostic);
            if (family == MaterialFamily::Count) return false;
            const std::size_t index = static_cast<std::size_t>(family);
            if (!hasMaterial[index])
            {
                familyMaterials[index] = MakeFamilyMaterial(material, family);
                hasMaterial[index] = true;
            }
        }
    }
    if (std::any_of(hasMaterial.begin(), hasMaterial.end(), [](bool present) { return !present; }))
    {
        diagnostic = "Forest source pair did not provide each of the four approved material families.";
        return false;
    }

    for (std::size_t tree=0;tree<sources.size();++tree) {
        auto& source=sources[tree];
        for(std::size_t index=0;index<source.primitives.size();++index) {
            const auto& primitive=source.primitives[index];
            if(primitive.materialIndex>=source.materials.size() || primitive.nodeTransformIndex>=source.nodeTransforms.size()) {
                diagnostic="Forest native primitive references invalid material/transform data.";return false;
            }
            const auto family=ClassifyMaterial(source.materials[primitive.materialIndex],diagnostic);
            if(family==MaterialFamily::Count)return false;
            candidate->primitiveFamilies[tree][index]=static_cast<std::uint8_t>(family);
        }
        for(auto& material:source.materials) {
            const auto family=ClassifyMaterial(material,diagnostic);
            if(family==MaterialFamily::Count)return false;
            material=familyMaterials[static_cast<std::size_t>(family)];
        }
    }
    for (std::size_t placementIndex = 0u;
         placementIndex < gameplay::simulation::kForestTreePlacementContract.size();
         ++placementIndex)
    {
        const auto& contract = gameplay::simulation::kForestTreePlacementContract[placementIndex];
        auto support = gameplay::simulation::ResolveWorldRouteSupport(contract.x, contract.z);
        if(terrain) {
            const auto ground=terrain->Query(contract.x,contract.z);
            support.worldY=ground.worldY;support.grounded=ground.found;
        }
        if (!support.grounded || !std::isfinite(support.worldY))
        {
            diagnostic = "Forest tree placement has no measured native route support surface.";
            return false;
        }
        const TreeId treeId = contract.species == gameplay::simulation::ForestTreeSpecies::Pine
            ? TreeId::Pine : TreeId::Alder;
        const std::size_t treeIndex = static_cast<std::size_t>(treeId);
        const auto& source = sources[treeIndex];
        const float yaw = contract.yawRadians;
        float originY = support.worldY - source.bounds.minimum[1] - contract.rootBurialMetres;
        if (terrain) {
            // Keep each tree upright and rigid. Embed the actual imported root
            // tips in the terrain across their footprint, not only at the trunk
            // centre. A bad steep placement must be reauthored, never sunk metres.
            ForestRootMeasurement measured;
            if(!MeasureForestRootFootprint(source,contract.x,contract.z,yaw,support.worldY,*terrain,measured,diagnostic))return false;
            const float fittedOriginY=measured.fittedOriginY;
            if(!AdmittedForestRootFootprint(measured) ||
               std::abs(fittedOriginY-originY)>.002f) {
                diagnostic="Forest root footprint needs reauthoring at placement " +
                    std::to_string(placementIndex) + "; required burial=" +
                    std::to_string(measured.unburiedOriginY-measured.fittedOriginY) +
                    " m (maximum 0.65 m; source collision receipt must match within 2mm).";
                return false;
            }
            // Use the same measured transform as the shared collision bounds;
            // native recomputation above rejects a stale placement receipt.
        }
        auto& placement=candidate->placements[placementIndex];
        placement.masterIndex=treeIndex;
        placement.origin={{contract.x,originY,contract.z}};
        placement.yawRadians=yaw;
        placement.supportY=support.worldY;
    }
    prepared=std::move(candidate);
    diagnostic.clear();
    return true;
}

bool MeasurePreparedForestInstanceBounds(const PreparedForestDressing& prepared,
    std::array<ForestDressingBounds, gameplay::simulation::kForestTreePlacementContract.size()>& bounds,
    std::string& diagnostic)
{
    if(!ValidSharedForestPrepared(prepared,diagnostic))return false;
    // A fixed-size value stages publication without copying either master.
    std::array<ForestDressingBounds, gameplay::simulation::kForestTreePlacementContract.size()> staged{};
    for(std::size_t i=0;i<prepared.placements.size();++i) {
        const auto& placement=prepared.placements[i];
        const auto& local=prepared.masters[placement.masterIndex].bounds;
        const float c=std::cos(placement.yawRadians),s=std::sin(placement.yawRadians);
        std::array<double,3> minimum,maximum,magnitude;
        minimum.fill(std::numeric_limits<double>::max());
        maximum.fill(std::numeric_limits<double>::lowest());
        const double x=std::max(std::abs(double(local.minimum[0])),std::abs(double(local.maximum[0])));
        const double y=std::max(std::abs(double(local.minimum[1])),std::abs(double(local.maximum[1])));
        const double z=std::max(std::abs(double(local.minimum[2])),std::abs(double(local.maximum[2])));
        magnitude={{std::abs(double(placement.origin[0]))+std::abs(double(c))*x+std::abs(double(s))*z,
                    std::abs(double(placement.origin[1]))+y,
                    std::abs(double(placement.origin[2]))+std::abs(double(s))*x+std::abs(double(c))*z}};
        for(unsigned corner=0;corner<8;++corner) {
            const double px=(corner&1)?local.maximum[0]:local.minimum[0];
            const double py=(corner&2)?local.maximum[1]:local.minimum[1];
            const double pz=(corner&4)?local.maximum[2]:local.minimum[2];
            const std::array<double,3> world{{double(c)*px+double(s)*pz+placement.origin[0],
                py+placement.origin[1],-double(s)*px+double(c)*pz+placement.origin[2]}};
            for(std::size_t axis=0;axis<3;++axis) {
                minimum[axis]=std::min(minimum[axis],world[axis]);
                maximum[axis]=std::max(maximum[axis],world[axis]);
            }
        }
        for(std::size_t axis=0;axis<3;++axis) {
            // The same float c/s coefficients as VkTransformMatrixKHR above.
            // Eight float epsilons of the absolute row sum exceed the rounding
            // bound for its <=3 products/3 adds, including reordered/FMA paths.
            // max(1,...) also reserves absolute space for tiny/subnormal inputs.
            const double reserve=8.0*std::numeric_limits<float>::epsilon()*std::max(1.0,magnitude[axis]);
            const double low=minimum[axis]-reserve,high=maximum[axis]+reserve;
            if(!std::isfinite(low) || !std::isfinite(high) || low<-double(std::numeric_limits<float>::max()) ||
               high>double(std::numeric_limits<float>::max())) {
                diagnostic="Prepared forest instance bounds exceed finite float world space.";return false;
            }
            staged[i].minimum[axis]=std::nextafter(float(low),-std::numeric_limits<float>::infinity());
            staged[i].maximum[axis]=std::nextafter(float(high),std::numeric_limits<float>::infinity());
            if(!std::isfinite(staged[i].minimum[axis]) || !std::isfinite(staged[i].maximum[axis]) ||
               staged[i].minimum[axis]>staged[i].maximum[axis]) {
                diagnostic="Prepared forest instance bounds exceed finite float world space.";return false;
            }
        }
    }
    bounds=staged;diagnostic.clear();return true;
}

bool SeedPreparedForestMaterialFamilies(const PreparedForestDressing& prepared,
                                       assets::StaticMeshAsset& existingOwner,
                                       std::string& diagnostic)
{
    if(!ValidSharedForestOwner(existingOwner,diagnostic) || !ValidForestFamilies(prepared,diagnostic))return false;
    if(existingOwner.materials.size()>kMaximumMaterialCount-prepared.materialFamilies.size() ||
       std::any_of(existingOwner.materials.begin(),existingOwner.materials.end(),[](const auto& material) {
           return std::find(kFamilyNames.begin(),kFamilyNames.end(),material.name)!=kFamilyNames.end();
       })) {
        diagnostic="Shared forest cannot seed duplicate families or exceed the existing 32 material rows.";return false;
    }
    auto materials=existingOwner.materials;
    materials.insert(materials.end(),prepared.materialFamilies.begin(),prepared.materialFamilies.end());
    existingOwner.materials=std::move(materials);
    diagnostic.clear();return true;
}

bool AppendPreparedForestMasters(std::shared_ptr<const PreparedForestDressing> prepared,
                                 assets::StaticMeshAsset& existingOwner,
                                 ForestDressingSharedLayout& layout,
                                 std::string& diagnostic)
{
    if(!prepared) {diagnostic="Shared forest requires its immutable prepared ownership handle.";return false;}
    if(!ValidSharedForestOwner(existingOwner,diagnostic) || !ValidSharedForestPrepared(*prepared,diagnostic))return false;
    if(existingOwner.primitives.size()>kMaximumPrimitiveCount-6u || existingOwner.nodeTransforms.size()>UINT32_MAX-2u ||
       std::any_of(existingOwner.nodeTransforms.begin(),existingOwner.nodeTransforms.end(),[](const auto& node) {
           return node.name=="forest-dressing-world-baked" ||
               std::find(kSharedMasterNodes.begin(),kSharedMasterNodes.end(),node.name)!=kSharedMasterNodes.end();
       })) {
        diagnostic="Shared forest local masters are already present or exceed primitive/address capacity.";return false;
    }
    std::vector<std::uint32_t> seen;
    for(const auto& p:existingOwner.primitives) {
        if(p.materialIndex>=existingOwner.materials.size() || p.nodeTransformIndex>=existingOwner.nodeTransforms.size() ||
           std::find(seen.begin(),seen.end(),p.materialIndex)!=seen.end()) {
            diagnostic="Append shared masters only after common world-family consolidation.";return false;
        }
        seen.push_back(p.materialIndex);
    }
    std::array<std::uint32_t,4> materialIndices{};
    for(std::size_t family=0;family<kFamilyNames.size();++family) {
        std::size_t found=existingOwner.materials.size();
        for(std::size_t i=0;i<existingOwner.materials.size();++i)if(existingOwner.materials[i].name==kFamilyNames[family]) {
            if(found!=existingOwner.materials.size() ||
               !SameForestMaterial(existingOwner.materials[i],prepared->materialFamilies[family])) {
                diagnostic="Shared forest canonical material rows are ambiguous or changed.";return false;
            }
            found=i;
        }
        if(found==existingOwner.materials.size()) {
            diagnostic="Shared forest requires the four seeded canonical material rows.";return false;
        }
        materialIndices[family]=static_cast<std::uint32_t>(found);
    }
    auto staged=existingOwner;
    ForestDressingSharedLayout candidate;
    candidate.commonPrimitiveCount=existingOwner.primitives.size();
    for(std::size_t tree=0;tree<prepared->masters.size();++tree) {
        const auto& source=prepared->masters[tree];
        if(source.vertices.size()>UINT32_MAX-staged.vertices.size() || source.indices.size()>UINT32_MAX-staged.indices.size()) {
            diagnostic="Shared forest local master exceeds static vertex/index address capacity.";return false;
        }
        const auto vertexBase=static_cast<std::uint32_t>(staged.vertices.size());
        const auto indexBase=static_cast<std::uint32_t>(staged.indices.size());
        const auto node=static_cast<std::uint32_t>(staged.nodeTransforms.size());
        auto& range=candidate.masters[tree];
        range.primitiveOffset=staged.primitives.size();range.primitiveCount=source.primitives.size();
        range.localBounds={source.bounds.minimum,source.bounds.maximum};
        staged.vertices.insert(staged.vertices.end(),source.vertices.begin(),source.vertices.end());
        staged.triangleRegions.resize(staged.indices.size()/3u,assets::StaticTriangleRegion::Shared);
        staged.indices.insert(staged.indices.end(),source.indices.begin(),source.indices.end());
        staged.triangleRegions.insert(staged.triangleRegions.end(),source.indices.size()/3u,assets::StaticTriangleRegion::ExteriorDressing);
        staged.nodeTransforms.push_back({std::string(kSharedMasterNodes[tree]),kIdentity});
        for(std::size_t i=0;i<source.primitives.size();++i) {
            auto p=source.primitives[i];p.vertexOffset+=vertexBase;p.indexOffset+=indexBase;
            p.materialIndex=materialIndices[prepared->primitiveFamilies[tree][i]];p.nodeTransformIndex=node;
            staged.primitives.push_back(p);
        }
    }
    // Bounds deliberately remain the common world's bounds, not a union with
    // unrelated local coordinates. Range bounds are carried separately above.
    candidate.prepared=std::move(prepared);
    existingOwner=std::move(staged);layout=std::move(candidate);
    diagnostic.clear();return true;
}

bool PrepareF01BackgroundForestDressing(ForestDressingSharedLayout& layout,
                                       const gameplay::simulation::TriangleSupportSurface& terrain,
                                        std::string& diagnostic,F01BackgroundLayout composition)
{
    if(composition!=F01BackgroundLayout::OriginalArc && composition!=F01BackgroundLayout::GroupedR4) {
        diagnostic="Unknown bounded F01 composition.";return false;
    }
    if(!layout.prepared || !layout.supplementalPlacements.empty() || !layout.commonPrimitiveCount ||
       layout.masters[0].primitiveCount!=3u || layout.masters[1].primitiveCount!=3u ||
       layout.masters[0].primitiveOffset!=layout.commonPrimitiveCount ||
       layout.masters[1].primitiveOffset!=layout.masters[0].primitiveOffset+3u) {
        diagnostic="F01 authoring supplement requires fresh disjoint shared-master ownership.";return false;
    }
    if(!ValidSharedForestPrepared(*layout.prepared,diagnostic))return false;
    const auto& roots=composition==F01BackgroundLayout::GroupedR4?kF01GroupedRootsR4:kF01BackgroundRoots;
    std::vector<ForestDressingSupplementalPlacement> staged;
    staged.reserve(roots.size());
    for(std::size_t index=0;index<roots.size();++index) {
        const auto& root=roots[index];const auto& source=layout.prepared->masters[root.master];
        const auto support=terrain.Query(root.x,root.z);
        if(!support.found || !std::isfinite(support.worldY)) {
            diagnostic="F01 authoring root has missing or non-finite centre support at "+std::to_string(index)+".";return false;
        }
        ForestRootMeasurement measured;
        if(!MeasureForestRootFootprint(source,root.x,root.z,root.yaw,support.worldY,terrain,measured,diagnostic))return false;
        if(!AdmittedForestRootFootprint(measured)) {
            diagnostic="F01 root footprint needs reauthoring at "+std::to_string(index)+"; required burial="+
                std::to_string(measured.unburiedOriginY-measured.fittedOriginY)+" m (maximum 0.65 m; at least 18 native tips).";
            return false;
        }
        ForestDressingSupplementalPlacement receipt;
        receipt.placement={root.master,{{root.x,measured.fittedOriginY,root.z}},root.yaw,support.worldY};
        receipt.rootBurialMetres=measured.unburiedOriginY-measured.fittedOriginY;
        receipt.rootTipSamples=measured.tipCount;
        if(!MeasureForestPlacementBounds(*layout.prepared,receipt,diagnostic))return false;
        staged.push_back(receipt);
    }
    layout.supplementalPlacements=std::move(staged);
    diagnostic.clear();return true;
}

namespace {
bool AppendForestPopulation(const PreparedForestDressing& prepared,
                                 std::span<const ForestDressingSupplementalPlacement> extra,
                                 assets::StaticMeshAsset& existingOwner,
                                 ForestDressingBuildReport& report,
                                 std::string& diagnostic)
{
    report={};
    if(!CanAppendForest(existingOwner,diagnostic) || !ValidSharedForestPrepared(prepared,diagnostic))return false;
    if(!extra.empty() && extra.size()!=4u) { diagnostic="F01 prototype requires exactly four extra roots.";return false; }
    const auto& sources=prepared.masters;
    const auto& familyMaterials=prepared.materialFamilies;
    for(std::size_t tree=0;tree<sources.size();++tree) {
        const auto& source=sources[tree];
        if(source.vertices.empty() || source.indices.empty() || !source.HasValidTriangleProvenance() ||
           source.primitives.size()!=kAssets[tree].expectedMaterials || source.materials.size()!=kAssets[tree].expectedMaterials) {
            diagnostic="Prepared forest master is empty or violates its admitted primitive/material contract.";return false;
        }
    }
    assets::StaticMeshAsset staged = existingOwner;
    std::array<std::uint32_t, static_cast<std::size_t>(MaterialFamily::Count)> materialIndices{};
    for (std::size_t family = 0u; family < familyMaterials.size(); ++family)
    {
        materialIndices[family] = static_cast<std::uint32_t>(staged.materials.size());
        staged.materials.push_back(familyMaterials[family]);
    }

    std::array<std::vector<assets::StaticRtVertex>, static_cast<std::size_t>(MaterialFamily::Count)> groupedVertices;
    std::array<std::vector<std::uint32_t>, static_cast<std::size_t>(MaterialFamily::Count)> groupedIndices;
    ForestDressingBuildReport candidate{};
    candidate.placements.resize(prepared.placements.size()+extra.size());
    candidate.addedBounds.minimum.fill(std::numeric_limits<float>::max());
    candidate.addedBounds.maximum.fill(std::numeric_limits<float>::lowest());

    for (std::size_t placementIndex=0;placementIndex<candidate.placements.size();++placementIndex)
    {
        const bool supplemental=placementIndex>=prepared.placements.size();
        const auto extraIndex=supplemental?placementIndex-prepared.placements.size():0u;
        const auto& rigid=supplemental?extra[extraIndex].placement:prepared.placements[placementIndex];
        gameplay::simulation::ForestTreePlacementContract contract{};
        if(supplemental) {
            const auto& root=kF01NorthGroveRoots[extraIndex];
            contract.species=root.master==0?gameplay::simulation::ForestTreeSpecies::Pine:gameplay::simulation::ForestTreeSpecies::Alder;
            contract.x=root.x;contract.z=root.z;contract.yawRadians=root.yaw;
            contract.rootBurialMetres=extra[extraIndex].rootBurialMetres;
        }else contract=gameplay::simulation::kForestTreePlacementContract[placementIndex];
        const auto treeIndex=rigid.masterIndex;
        const auto expected=contract.species==gameplay::simulation::ForestTreeSpecies::Pine?0u:1u;
        if(treeIndex>=sources.size() || treeIndex!=expected || rigid.origin[0]!=contract.x || rigid.origin[2]!=contract.z ||
           rigid.yawRadians!=contract.yawRadians || !std::isfinite(rigid.supportY) ||
           rigid.origin[1]!=rigid.supportY-sources[treeIndex].bounds.minimum[1]-contract.rootBurialMetres) {
            diagnostic="Prepared forest placement is not the admitted rigid root transform.";return false;
        }
        const auto& source=sources[treeIndex];const auto& spec=kAssets[treeIndex];
        const float yaw=rigid.yawRadians;
        ForestDressingPlacement& placement = candidate.placements[placementIndex];
        placement.asset = spec.name;
        placement.origin = rigid.origin;
        placement.yawRadians = yaw;
        placement.supportY = rigid.supportY;
        placement.worldBounds.minimum.fill(std::numeric_limits<float>::max());
        placement.worldBounds.maximum.fill(std::numeric_limits<float>::lowest());
        placement.trunkBounds.minimum.fill(std::numeric_limits<float>::max());
        placement.trunkBounds.maximum.fill(std::numeric_limits<float>::lowest());

            for (std::size_t primitiveIndex=0;primitiveIndex<source.primitives.size();++primitiveIndex)
            {
                const auto& primitive=source.primitives[primitiveIndex];
                if (primitive.materialIndex >= source.materials.size() ||
                    primitive.nodeTransformIndex >= source.nodeTransforms.size())
                {
                    diagnostic = "Forest native primitive references invalid material/transform data.";
                    return false;
                }
                const auto familyIndex=prepared.primitiveFamilies[treeIndex][primitiveIndex];
                if(familyIndex>=familyMaterials.size() || source.materials[primitive.materialIndex].name!=familyMaterials[familyIndex].name ||
                   familyMaterials[familyIndex].name!=kFamilyNames[familyIndex]) {
                    diagnostic="Prepared forest material family mapping is invalid.";return false;
                }
                const auto family=static_cast<MaterialFamily>(familyIndex);
                auto& vertices = groupedVertices[familyIndex];
                auto& indices = groupedIndices[familyIndex];
                const std::size_t vertexBase = vertices.size();
                const std::size_t indexBase = indices.size();
                std::size_t vertexEnd = source.vertices.size();
                for (const auto& next : source.primitives)
                    if (next.vertexOffset > primitive.vertexOffset)
                        vertexEnd = std::min(vertexEnd, static_cast<std::size_t>(next.vertexOffset));
                if (primitive.indexCount == 0 || primitive.indexCount % 3u != 0 ||
                    vertexEnd <= primitive.vertexOffset || vertexEnd > source.vertices.size() ||
                    primitive.indexOffset > source.indices.size() ||
                    primitive.indexCount > source.indices.size() - primitive.indexOffset)
                {
                    diagnostic = "Forest source primitive spans exceed imported vertex/index bounds.";
                    return false;
                }
                for (std::size_t vertexIndex = primitive.vertexOffset; vertexIndex < vertexEnd; ++vertexIndex)
                {
                    auto vertex = source.vertices[vertexIndex];
                    auto position = RotateY({{vertex.position[0], vertex.position[1], vertex.position[2]}}, yaw);
                    auto normal = RotateY({{vertex.normal[0], vertex.normal[1], vertex.normal[2]}}, yaw);
                    auto tangent = RotateY({{vertex.tangent[0], vertex.tangent[1], vertex.tangent[2]}}, yaw);
                    position[0] += placement.origin[0];
                    position[1] += placement.origin[1];
                    position[2] += placement.origin[2];
                    for (std::size_t axis = 0u; axis < 3u; ++axis)
                    {
                        if (!std::isfinite(position[axis]) || !std::isfinite(normal[axis]) ||
                            !std::isfinite(tangent[axis]))
                        {
                            diagnostic = "Forest placement generated a non-finite vertex basis.";
                            return false;
                        }
                        vertex.position[axis] = position[axis];
                        vertex.normal[axis] = normal[axis];
                        vertex.tangent[axis] = tangent[axis];
                    }
                    IncludePoint(placement.worldBounds, position);
                    IncludePoint(candidate.addedBounds, position);
                    vertices.push_back(vertex);
                }
                for (std::size_t index = 0u; index < primitive.indexCount; ++index)
                {
                    const std::uint32_t sourceIndex = source.indices[primitive.indexOffset + index];
                    if (sourceIndex >= vertexEnd - primitive.vertexOffset ||
                        vertexBase + sourceIndex > std::numeric_limits<std::uint32_t>::max())
                    {
                        diagnostic = "Forest source index is outside its primitive vertex span.";
                        return false;
                    }
                    indices.push_back(static_cast<std::uint32_t>(vertexBase + sourceIndex));
                }
                if(family==MaterialFamily::Bark) {
                    for(std::size_t i=indexBase;i<indices.size();i+=3) {
                        std::vector<std::array<float,3u>> triangle;
                        for(std::size_t corner=0;corner<3;++corner) {
                            const auto& p=vertices[indices[i+corner]].position;
                            triangle.push_back({p[0],p[1],p[2]});
                        }
                        IncludeTrunkTriangle(placement.trunkBounds,std::move(triangle),placement.supportY);
                    }
                }
                if (vertices.size() - vertexBase != vertexEnd - primitive.vertexOffset ||
                    indices.size() - indexBase != primitive.indexCount)
                {
                    diagnostic = "Forest merge accounting failed for a material-family primitive.";
                    return false;
                }
            }
            if (std::abs(placement.worldBounds.minimum[1] + contract.rootBurialMetres - placement.supportY) > 0.0001f)
            {
                diagnostic = "Forest tree root does not meet the resolved route support height.";
                return false;
            }
        ++candidate.placedInstances;
    }

    if (std::any_of(groupedVertices.begin(), groupedVertices.end(),
                    [](const auto& vertices) { return vertices.empty(); }) ||
        std::any_of(groupedIndices.begin(), groupedIndices.end(),
                    [](const auto& indices) { return indices.empty(); }))
    {
        diagnostic = "Forest tree pair failed to produce all four material groups.";
        return false;
    }

    for (std::size_t family = 0u; family < groupedVertices.size(); ++family)
    {
        if (staged.primitives.size() >= kMaximumPrimitiveCount ||
            staged.vertices.size() > std::numeric_limits<std::uint32_t>::max() - groupedVertices[family].size() ||
            staged.indices.size() > std::numeric_limits<std::uint32_t>::max() - groupedIndices[family].size())
        {
            diagnostic = "Forest geometry exceeds the existing static primitive/address capacity.";
            return false;
        }
        assets::StaticPrimitiveRecord primitive;
        primitive.vertexOffset = static_cast<std::uint32_t>(staged.vertices.size());
        primitive.indexOffset = static_cast<std::uint32_t>(staged.indices.size());
        primitive.indexCount = static_cast<std::uint32_t>(groupedIndices[family].size());
        primitive.materialIndex = materialIndices[family];
        primitive.nodeTransformIndex = static_cast<std::uint32_t>(staged.nodeTransforms.size());
        staged.vertices.insert(staged.vertices.end(), groupedVertices[family].begin(), groupedVertices[family].end());
        // CPU ownership is independent of shared materials; it does not certify
        // that these trees are irrelevant to any current ray or light path.
        staged.triangleRegions.resize(staged.indices.size()/3u, assets::StaticTriangleRegion::Shared);
        staged.triangleRegions.insert(staged.triangleRegions.end(), primitive.indexCount/3u,
            assets::StaticTriangleRegion::ExteriorDressing);
        staged.indices.insert(staged.indices.end(), groupedIndices[family].begin(), groupedIndices[family].end());
        staged.primitives.push_back(primitive);
        staged.nodeTransforms.push_back({"forest-dressing-world-baked", {{
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        }}});
        candidate.addedVertices += groupedVertices[family].size();
        candidate.addedIndices += groupedIndices[family].size();
        candidate.addedTriangles += groupedIndices[family].size() / 3u;
        ++candidate.addedPrimitives;
    }
    if (staged.primitives.size() > kMaximumPrimitiveCount || staged.materials.size() > kMaximumMaterialCount)
    {
        diagnostic = "Forest dressing exceeds the shared static primitive/material capacity of 32.";
        return false;
    }
    candidate.addedMaterials = staged.materials.size() - existingOwner.materials.size();
    for (std::size_t axis = 0u; axis < 3u; ++axis)
    {
        staged.bounds.minimum[axis] = std::min(staged.bounds.minimum[axis], candidate.addedBounds.minimum[axis]);
        staged.bounds.maximum[axis] = std::max(staged.bounds.maximum[axis], candidate.addedBounds.maximum[axis]);
    }
    existingOwner = std::move(staged);
    report = candidate;
    diagnostic.clear();
    return true;
}

} // namespace
bool PrepareF01NorthGrovePrototype(std::shared_ptr<const PreparedForestDressing> prepared,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface> terrain,
    std::shared_ptr<const F01NorthGrovePrototype>& result,std::string& diagnostic)
{
    if(result || !prepared || !terrain || !ValidSharedForestPrepared(*prepared,diagnostic)) {
        diagnostic="F01 north prototype requires fresh original64 preparation and actual terrain.";return false;
    }
    // The original64 must also match this terrain, not merely internally
    // consistent transforms prepared against another support snapshot.
    for(const auto& p:prepared->placements) {
        const auto ground=terrain->Query(p.origin[0],p.origin[2]);
        ForestRootMeasurement measured;
        if(!ground.found || std::abs(ground.worldY-p.supportY)>.0001f ||
           !MeasureForestRootFootprint(prepared->masters[p.masterIndex],p.origin[0],p.origin[2],p.yawRadians,
               ground.worldY,*terrain,measured,diagnostic) || !AdmittedForestRootFootprint(measured) ||
           std::abs(measured.fittedOriginY-p.origin[1])>.002f) {
            diagnostic="Original64 preparation does not match the prototype terrain snapshot.";return false;
        }
    }
    auto candidate=std::shared_ptr<F01NorthGrovePrototype>(new F01NorthGrovePrototype);
    candidate->prepared_=prepared;
    using Surface=gameplay::simulation::TriangleSupportSurface;
    std::vector<Surface::Triangle> collision;
    for(std::size_t i=0;i<candidate->placements_.size();++i) {
        const auto& root=kF01NorthGroveRoots[i];
        for(const auto& old:prepared->placements)if(old.origin[0]==root.x && old.origin[2]==root.z) {
            diagnostic="F01 prototype duplicates an original root.";return false;
        }
        for(std::size_t j=0;j<i;++j)if(kF01NorthGroveRoots[j].x==root.x && kF01NorthGroveRoots[j].z==root.z) {
            diagnostic="F01 prototype contains duplicate roots.";return false;
        }
        const auto ground=terrain->Query(root.x,root.z);
        ForestRootMeasurement measured;
        const auto& master=prepared->masters[root.master];
        if(!ground.found || !MeasureForestRootFootprint(master,root.x,root.z,root.yaw,ground.worldY,*terrain,measured,diagnostic) ||
            !AdmittedForestRootFootprint(measured)) {
            diagnostic="F01 prototype root is not admitted by its actual terrain footprint.";return false;
        }
        auto& receipt=candidate->placements_[i];
        receipt.placement={root.master,{root.x,measured.fittedOriginY,root.z},root.yaw,ground.worldY};
        receipt.rootBurialMetres=measured.unburiedOriginY-measured.fittedOriginY;
        receipt.rootTipSamples=measured.tipCount;
        if(!MeasureForestPlacementBounds(*prepared,receipt,diagnostic))return false;
        const float c=std::cos(root.yaw),s=std::sin(root.yaw);
        const auto world=[&](std::size_t index) {
            const auto& v=master.vertices[index].position;
            return Surface::Point{c*v[0]+s*v[2]+root.x,v[1]+receipt.placement.origin[1],-s*v[0]+c*v[2]+root.z};
        };
        const auto start=collision.size();
        for(std::size_t p=0;p<master.primitives.size();++p) {
            if(prepared->primitiveFamilies[root.master][p]!=static_cast<unsigned>(MaterialFamily::Bark))continue;
            const auto& primitive=master.primitives[p];
            for(std::size_t n=0;n<primitive.indexCount;n+=3) {
                std::vector<Surface::Point> triangle;
                for(unsigned k=0;k<3;++k)triangle.push_back(world(primitive.vertexOffset+master.indices[primitive.indexOffset+n+k]));
                const auto polygon=ClipTrunkTriangle(std::move(triangle),ground.worldY);
                for(std::size_t k=1;k+1<polygon.size();++k) {
                    const auto a=polygon[0],b=polygon[k],d=polygon[k+1];
                    const double ux=b[0]-a[0],uy=b[1]-a[1],uz=b[2]-a[2],vx=d[0]-a[0],vy=d[1]-a[1],vz=d[2]-a[2];
                    if(std::hypot(std::hypot(uy*vz-uz*vy,uz*vx-ux*vz),ux*vy-uy*vx)<=1e-10)continue;
                    collision.push_back({{a,b,d},gameplay::simulation::SupportSurface::Earth});
                }
            }
        }
        if(collision.size()==start) {diagnostic="F01 prototype has no grounded trunk contact triangles.";return false;}
    }
    candidate->terrain_=Surface::WithAdditionalNonWalkableGeometry(*terrain,collision,diagnostic);
    if(!candidate->terrain_)return false;
    candidate->collisionTriangles_=collision.size();
    result=std::move(candidate);diagnostic.clear();return true;
}

bool AppendPreparedForestDressing(const PreparedForestDressing& prepared,
    assets::StaticMeshAsset& owner,ForestDressingBuildReport& report,std::string& diagnostic)
{
    return AppendForestPopulation(prepared,{},owner,report,diagnostic);
}
bool AppendPreparedF01NorthGrove(const F01NorthGrovePrototype& prototype,
    assets::StaticMeshAsset& owner,ForestDressingBuildReport& report,std::string& diagnostic)
{
    return AppendForestPopulation(prototype.Prepared(),prototype.Placements(),owner,report,diagnostic);
}

bool AppendPreparedForestDressing(const std::filesystem::path& assetRoot,
                                 assets::StaticMeshAsset& existingOwner,
                                 ForestDressingBuildReport& report,
                                 std::string& diagnostic,
                                 const gameplay::simulation::TriangleSupportSurface* terrain)
{
    report={};
    if(!CanAppendForest(existingOwner,diagnostic))return false;
    std::shared_ptr<const PreparedForestDressing> prepared;
    return PrepareForestDressing(assetRoot,prepared,diagnostic,terrain) &&
        AppendPreparedForestDressing(*prepared,existingOwner,report,diagnostic);
}

} // namespace horde::scene
