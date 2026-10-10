#include "scene/TombDressing.h"

#include "scene/assets/AssetManifest.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

namespace horde::scene
{

namespace
{

constexpr float kPi = 3.14159265358979323846f;
constexpr float kFloorY = -0.95f;
constexpr std::size_t kMaximumPrimitiveCount = 32u;
constexpr std::size_t kMaximumMaterialCount = 32u;

enum class AssetId : std::size_t
{
    RectNiche,
    ArchedNiche,
    SkullJaw,
    Femur,
    Humerus,
    DisplacedLid,
    OfferingBowl,
    BrokenUrn,
    UrnShard,
    CandleOne,
    CandleTwo,
    CandleThree,
    Count,
};

enum class MaterialFamily : std::size_t
{
    Masonry,
    BoneWax,
    Earthenware,
    CharredWick,
    Count,
};

struct AssetSpec
{
    const char* name;
    std::string_view relativePath;
    std::size_t expectedTriangles;
};

const std::array<AssetSpec, static_cast<std::size_t>(AssetId::Count)> kAssets{{
    {"rect niche", "models/world/runtime/tomb-dressing-v01/tomb-niche-rect/tomb-niche-rect-lod0.runtime.glb", 1188u},
    {"arched niche", "models/world/runtime/tomb-dressing-v01/tomb-niche-arched/tomb-niche-arched-lod0.runtime.glb", 1280u},
    {"T02 skull/jaw", "models/world/runtime/prepared-funerary-v01/t02-native-import-candidates/skull-jaw/tomb-skull-jaw-lod0.runtime.glb", 5564u},
    {"T02 femur", "models/world/runtime/prepared-funerary-v01/t02-native-import-candidates/femur/tomb-femur-lod0.runtime.glb", 396u},
    {"T02 humerus", "models/world/runtime/prepared-funerary-v01/t02-native-import-candidates/humerus/tomb-humerus-lod0.runtime.glb", 304u},
    {"T03 displaced lid", "models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_displaced_lid.glb", 496u},
    {"T03 offering bowl", "models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_offering_bowl.glb", 960u},
    {"T03 broken urn", "models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_urn_broken_base.glb", 1212u},
    {"T03 urn rim shard", "models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_urn_rim_shard.glb", 800u},
    {"T03 candle 1", "models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_candle_stub_1.glb", 664u},
    {"T03 candle 2", "models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_candle_stub_2.glb", 664u},
    {"T03 candle 3", "models/world/runtime/prepared-funerary-v01/t03-native-import-candidates/t03_candle_stub_3.glb", 664u},
}};

struct Placement
{
    AssetId asset;
    std::array<float, 3u> position;
    float yaw;
    float rollZ;
};

// The side-wall niches face into the route and sit mostly behind its walk boundary,
// presenting their real .29-.30 m deep returns with at most 5 cm of frame reveal.
// Their lower bounds sit 0.30 m above the route floor. All loose pieces stay
// at the room/passage edges; the centre walk lane, right-wall grate, portal,
// guards, reward chest, and rescue-rope approaches remain clear.
constexpr std::array<Placement, 13u> kPlacements{{
    // Three self-contained recessed burial modules.
    {AssetId::RectNiche, {-2.0475f, -0.65f, -0.70f}, kPi * 0.5f, 0.0f},
    {AssetId::ArchedNiche, {-2.0475f, -0.65f, -2.60f}, kPi * 0.5f, 0.0f},
    {AssetId::RectNiche, {6.1975f, -0.65f, -12.40f}, -kPi * 0.5f, 0.0f},
    // One cold stub on the first genuine niche shelf.
    {AssetId::CandleOne, {-2.0625f, -0.3675f, -0.49f}, 0.0f, 0.0f},
    // Skull and two laid-out long-bone remains on the second shelf.
    {AssetId::SkullJaw, {-1.9275f, -0.3675f, -2.58f}, kPi * 0.5f, 0.0f},
    {AssetId::Femur, {-2.1075f, -0.314633f, -2.34f}, kPi * 0.5f, -kPi * 0.5f},
    {AssetId::Humerus, {-2.1075f, -0.329427f, -2.02f}, kPi * 0.5f, -kPi * 0.5f},
    // Two extinguished stubs in the outer route east-wall recess at x=6.
    {AssetId::CandleTwo, {6.1825f, -0.3675f, -12.60f}, 0.0f, 0.0f},
    {AssetId::CandleThree, {6.1825f, -0.3675f, -12.15f}, 0.0f, 0.0f},
    // The displaced lid rests entirely on the existing collidable stone bier.
    {AssetId::DisplacedLid, {-1.135f, -0.58f, 1.20f}, 0.0f, 0.0f},
    // Ceremonial offerings and fragments sit in Keeper's rear corner, away from routes.
    {AssetId::OfferingBowl, {-30.90f, kFloorY, -17.95f}, 0.0f, 0.0f},
    {AssetId::BrokenUrn, {-36.45f, kFloorY, -17.95f}, 0.15f, 0.0f},
    {AssetId::UrnShard, {-36.75f, kFloorY, -18.12f}, -0.45f, 0.0f},
}};

assets::AssetManifest MakeIntakeManifest(std::string_view name)
{
    assets::AssetManifest manifest;
    manifest.schema = 1u;
    manifest.assetName = std::string(name);
    manifest.metresPerUnit = 1.0f;
    manifest.upAxis = "+Y";
    manifest.forwardAxis = "+Z";
    manifest.budgets = {1000000u, 3000000u, 128u, 32u, 16u};
    manifest.lods.push_back({"lod0", 100000u});
    manifest.textureProfile = {"astc", "rgba8", true};
    return manifest;
}

std::array<float, 3u> RotateZ(std::array<float, 3u> value, float radians)
{
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    return {cosine * value[0] - sine * value[1],
            sine * value[0] + cosine * value[1], value[2]};
}

std::array<float, 3u> RotateY(std::array<float, 3u> value, float radians)
{
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    return {cosine * value[0] + sine * value[2], value[1],
            -sine * value[0] + cosine * value[2]};
}

std::array<float, 3u> TransformPoint(const assets::StaticRtVertex& vertex,
                                    const Placement& placement)
{
    // StaticMeshAsset::Load has already baked the GLB node-world transform.
    std::array<float, 3u> point{{vertex.position[0], vertex.position[1], vertex.position[2]}};
    point = RotateZ(point, placement.rollZ);
    point = RotateY(point, placement.yaw);
    for (std::size_t axis = 0u; axis < 3u; ++axis) point[axis] += placement.position[axis];
    return point;
}

std::array<float, 3u> TransformDirection(const std::array<float, 4u>& source,
                                         const Placement& placement)
{
    std::array<float, 3u> direction{{source[0], source[1], source[2]}};
    direction = RotateZ(direction, placement.rollZ);
    direction = RotateY(direction, placement.yaw);
    const float length = std::sqrt(direction[0] * direction[0] +
                                   direction[1] * direction[1] +
                                   direction[2] * direction[2]);
    if (std::isfinite(length) && length > 1.0e-8f)
        for (float& component : direction) component /= length;
    return direction;
}

MaterialFamily ClassifyMaterial(AssetId asset, const assets::StaticMaterial& material,
                                std::string& diagnostic)
{
    if (asset == AssetId::RectNiche || asset == AssetId::ArchedNiche ||
        asset == AssetId::DisplacedLid)
        return MaterialFamily::Masonry;
    const std::string_view name = material.name;
    if (asset == AssetId::SkullJaw || asset == AssetId::Femur || asset == AssetId::Humerus ||
        name == "Original_aged_beeswax")
        return MaterialFamily::BoneWax;
    if (name == "Original_unglazed_earthenware" || name == "Original_exposed_clay_fracture")
        return MaterialFamily::Earthenware;
    if (name == "Original_charred_wick") return MaterialFamily::CharredWick;
    diagnostic = "Tomb dressing candidate has an unclassified source material '" + material.name + "'.";
    return MaterialFamily::Count;
}

} // namespace

bool AppendPreparedTombDressing(const std::filesystem::path& assetRoot,
                                assets::StaticMeshAsset& existingOwner,
                                TombDressingBuildReport& report,
                                std::string& diagnostic)
{
    report = {};
    if (existingOwner.vertices.empty() || existingOwner.indices.empty() ||
        existingOwner.primitives.empty() || existingOwner.materials.empty() ||
        existingOwner.nodeTransforms.empty())
    {
        diagnostic = "Tomb dressing requires the caller's already-loaded non-empty collapsed static owner.";
        return false;
    }
    if (existingOwner.materials.size() >= kMaximumMaterialCount ||
        existingOwner.primitives.size() >= kMaximumPrimitiveCount)
    {
        diagnostic = "Tomb dressing cannot reserve its bounded static material/primitive groups.";
        return false;
    }
    if (std::any_of(existingOwner.materials.begin(), existingOwner.materials.end(),
                    [](const assets::StaticMaterial& material) {
                        return material.name == "TombBoneWax" || material.name == "TombEarthenware" ||
                               material.name == "TombCharredWick";
                    }))
    {
        diagnostic = "Tomb dressing has already been appended to this static asset owner.";
        return false;
    }

    const auto masonry = std::find_if(existingOwner.materials.begin(), existingOwner.materials.end(),
        [](const assets::StaticMaterial& material) { return material.name == "MedievalWall02"; });
    if (masonry == existingOwner.materials.end())
    {
        diagnostic = "Tomb dressing requires the existing MedievalWall02 masonry factor.";
        return false;
    }
    const std::uint32_t masonryIndex = static_cast<std::uint32_t>(masonry - existingOwner.materials.begin());

    std::array<assets::StaticMeshAsset, static_cast<std::size_t>(AssetId::Count)> loadedAssets{};
    for (std::size_t i = 0u; i < kAssets.size(); ++i)
    {
        const AssetSpec& spec = kAssets[i];
        const auto path = assetRoot / std::filesystem::path(spec.relativePath);
        const auto manifest = MakeIntakeManifest(spec.name);
        assets::StaticMeshAsset& loaded = loadedAssets[i];
        if (!assets::StaticMeshAsset::Load(path, manifest, loaded, diagnostic))
        {
            diagnostic = "Tomb dressing native import failed for " + std::string(spec.name) + ": " + diagnostic;
            return false;
        }
        std::size_t triangles = 0u;
        for (const auto& primitive : loaded.primitives) triangles += primitive.indexCount / 3u;
        if (triangles != spec.expectedTriangles)
        {
            diagnostic = "Tomb dressing triangle-count evidence changed for " + std::string(spec.name) + ".";
            return false;
        }
        if (loaded.materials.empty() || loaded.primitives.empty())
        {
            diagnostic = "Tomb dressing candidate has no native material or primitive: " + std::string(spec.name) + ".";
            return false;
        }
        for (const auto& material : loaded.materials)
        {
            if (material.baseColorTexture >= 0 || material.normalTexture >= 0 ||
                material.ormTexture >= 0 || material.emissiveTexture >= 0 ||
                material.metallicFactor != 0.0f || material.transmissionFactor != 0.0f ||
                material.emissiveFactor != std::array<float, 3u>{})
            {
                diagnostic = "Tomb dressing candidates must remain texture-free opaque dielectric static PBR.";
                return false;
            }
        }
    }

    assets::StaticMeshAsset staged = existingOwner;
    assets::StaticMaterial boneWax = loadedAssets[static_cast<std::size_t>(AssetId::SkullJaw)].materials.front();
    boneWax.name = "TombBoneWax";
    boneWax.baseColorFactor = {{0.515f, 0.4125f, 0.26f, 1.0f}};
    boneWax.roughnessFactor = 0.73f;
    boneWax.metallicFactor = 0.0f;
    boneWax.emissiveFactor = {};
    boneWax.emissiveStrength = 1.0f;
    boneWax.baseColorTexture = boneWax.normalTexture = boneWax.ormTexture = boneWax.emissiveTexture = -1;
    boneWax.transmissionFactor = 0.0f;
    boneWax.ior = 1.5f;
    boneWax.thicknessFactor = 0.0f;
    boneWax.attenuationDistance = 0.0f;
    boneWax.numericalSpawnMinimumWidth = boneWax.numericalSpawnGeometryError = 0.0f;
    boneWax.textureGroup = -1;
    boneWax.flags = 0u;

    assets::StaticMaterial earthenware = loadedAssets[static_cast<std::size_t>(AssetId::OfferingBowl)].materials.front();
    earthenware.name = "TombEarthenware";
    earthenware.baseColorTexture = earthenware.normalTexture = earthenware.ormTexture = earthenware.emissiveTexture = -1;
    earthenware.emissiveFactor = {};
    earthenware.emissiveStrength = 1.0f;
    earthenware.metallicFactor = 0.0f;
    earthenware.transmissionFactor = 0.0f;
    earthenware.ior = 1.5f;
    earthenware.thicknessFactor = 0.0f;
    earthenware.attenuationDistance = 0.0f;
    earthenware.textureGroup = -1;
    earthenware.flags = 0u;

    assets::StaticMaterial charredWick = loadedAssets[static_cast<std::size_t>(AssetId::CandleOne)].materials.back();
    charredWick.name = "TombCharredWick";
    charredWick.baseColorTexture = charredWick.normalTexture = charredWick.ormTexture = charredWick.emissiveTexture = -1;
    charredWick.emissiveFactor = {};
    charredWick.emissiveStrength = 1.0f;
    charredWick.metallicFactor = 0.0f;
    charredWick.transmissionFactor = 0.0f;
    charredWick.ior = 1.5f;
    charredWick.thicknessFactor = 0.0f;
    charredWick.attenuationDistance = 0.0f;
    charredWick.textureGroup = -1;
    charredWick.flags = 0u;

    const std::uint32_t boneWaxIndex = static_cast<std::uint32_t>(staged.materials.size());
    staged.materials.push_back(boneWax);
    const std::uint32_t earthenwareIndex = static_cast<std::uint32_t>(staged.materials.size());
    staged.materials.push_back(earthenware);
    const std::uint32_t charredWickIndex = static_cast<std::uint32_t>(staged.materials.size());
    staged.materials.push_back(charredWick);
    if (staged.materials.size() > kMaximumMaterialCount)
    {
        diagnostic = "Tomb dressing material groups exceed the shared static material capacity of 32.";
        return false;
    }

    const std::array<std::uint32_t, static_cast<std::size_t>(MaterialFamily::Count)> materialIndices{{
        masonryIndex, boneWaxIndex, earthenwareIndex, charredWickIndex,
    }};
    std::array<std::vector<assets::StaticRtVertex>, static_cast<std::size_t>(MaterialFamily::Count)> groupedVertices;
    std::array<std::vector<std::uint32_t>, static_cast<std::size_t>(MaterialFamily::Count)> groupedIndices;
    TombDressingBuildReport candidateReport{};
    candidateReport.addedBounds.minimum.fill(std::numeric_limits<float>::max());
    candidateReport.addedBounds.maximum.fill(std::numeric_limits<float>::lowest());
    std::array<bool, static_cast<std::size_t>(AssetId::Count)> placed{};

    for (std::size_t placementIndex = 0u; placementIndex < kPlacements.size(); ++placementIndex)
    {
        const Placement& placement = kPlacements[placementIndex];
        placed[static_cast<std::size_t>(placement.asset)] = true;
        TombDressingPlacementBounds& instanceBounds = candidateReport.placementBounds[placementIndex];
        instanceBounds.asset = kAssets[static_cast<std::size_t>(placement.asset)].name;
        instanceBounds.worldBounds.minimum.fill(std::numeric_limits<float>::max());
        instanceBounds.worldBounds.maximum.fill(std::numeric_limits<float>::lowest());
        const auto& source = loadedAssets[static_cast<std::size_t>(placement.asset)];
        for (const auto& primitive : source.primitives)
        {
            if (primitive.materialIndex >= source.materials.size() ||
                primitive.nodeTransformIndex >= source.nodeTransforms.size())
            {
                diagnostic = "Tomb dressing native primitive references invalid material/transform data.";
                return false;
            }
            const auto materialFamily = ClassifyMaterial(
                placement.asset, source.materials[primitive.materialIndex], diagnostic);
            if (materialFamily == MaterialFamily::Count) return false;
            const std::size_t family = static_cast<std::size_t>(materialFamily);
            const std::size_t nextVertexBase = groupedVertices[family].size();
            const std::size_t nextIndexBase = groupedIndices[family].size();
            const std::size_t vertexEnd = [&]() {
                for (const auto& next : source.primitives)
                    if (next.vertexOffset > primitive.vertexOffset) return static_cast<std::size_t>(next.vertexOffset);
                return source.vertices.size();
            }();
            if (vertexEnd <= primitive.vertexOffset || vertexEnd > source.vertices.size() ||
                primitive.indexOffset > source.indices.size() ||
                primitive.indexCount > source.indices.size() - primitive.indexOffset)
            {
                diagnostic = "Tomb dressing primitive spans are invalid.";
                return false;
            }
            for (std::size_t vertexIndex = primitive.vertexOffset; vertexIndex < vertexEnd; ++vertexIndex)
            {
                assets::StaticRtVertex vertex = source.vertices[vertexIndex];
                const auto position = TransformPoint(vertex, placement);
                const auto normal = TransformDirection(vertex.normal, placement);
                const auto tangent = TransformDirection(vertex.tangent, placement);
                for (std::size_t axis = 0u; axis < 3u; ++axis)
                {
                    if (!std::isfinite(position[axis]) || !std::isfinite(normal[axis]) ||
                        !std::isfinite(tangent[axis]))
                    {
                        diagnostic = "Tomb dressing transform produced a non-finite vertex basis.";
                        return false;
                    }
                    vertex.position[axis] = position[axis];
                    vertex.normal[axis] = normal[axis];
                    vertex.tangent[axis] = tangent[axis];
                    candidateReport.addedBounds.minimum[axis] = std::min(candidateReport.addedBounds.minimum[axis], position[axis]);
                    candidateReport.addedBounds.maximum[axis] = std::max(candidateReport.addedBounds.maximum[axis], position[axis]);
                    instanceBounds.worldBounds.minimum[axis] = std::min(instanceBounds.worldBounds.minimum[axis], position[axis]);
                    instanceBounds.worldBounds.maximum[axis] = std::max(instanceBounds.worldBounds.maximum[axis], position[axis]);
                }
                groupedVertices[family].push_back(vertex);
            }
            const std::size_t groupVertexCount = groupedVertices[family].size() - nextVertexBase;
            for (std::size_t index = 0u; index < primitive.indexCount; ++index)
            {
                const std::uint32_t sourceIndex = source.indices[primitive.indexOffset + index];
                if (sourceIndex >= vertexEnd - primitive.vertexOffset ||
                    nextVertexBase + sourceIndex > std::numeric_limits<std::uint32_t>::max())
                {
                    diagnostic = "Tomb dressing source index is outside its primitive vertex span.";
                    return false;
                }
                groupedIndices[family].push_back(static_cast<std::uint32_t>(nextVertexBase + sourceIndex));
            }
            if (groupedVertices[family].size() - nextVertexBase != groupVertexCount ||
                groupedIndices[family].size() - nextIndexBase != primitive.indexCount)
            {
                diagnostic = "Tomb dressing merge accounting failed.";
                return false;
            }
        }
        ++candidateReport.placedInstances;
    }
    if (std::any_of(placed.begin(), placed.end(), [](bool value) { return !value; }))
    {
        diagnostic = "Tomb dressing bounded placement set is missing a selected source family.";
        return false;
    }

    const std::array<std::string_view, static_cast<std::size_t>(MaterialFamily::Count)> familyNames{{
        "MedievalWall02", "TombBoneWax", "TombEarthenware", "TombCharredWick",
    }};
    for (std::size_t family = 0u; family < groupedVertices.size(); ++family)
    {
        if (groupedVertices[family].empty() || groupedIndices[family].empty()) continue;
        if (staged.primitives.size() >= kMaximumPrimitiveCount ||
            staged.vertices.size() > std::numeric_limits<std::uint32_t>::max() - groupedVertices[family].size() ||
            staged.indices.size() > std::numeric_limits<std::uint32_t>::max() - groupedIndices[family].size())
        {
            diagnostic = "Tomb dressing merged geometry exceeds the shared static primitive/address capacity.";
            return false;
        }
        assets::StaticPrimitiveRecord primitive;
        primitive.vertexOffset = static_cast<std::uint32_t>(staged.vertices.size());
        primitive.indexOffset = static_cast<std::uint32_t>(staged.indices.size());
        primitive.indexCount = static_cast<std::uint32_t>(groupedIndices[family].size());
        primitive.materialIndex = materialIndices[family];
        primitive.nodeTransformIndex = static_cast<std::uint32_t>(staged.nodeTransforms.size());
        staged.vertices.insert(staged.vertices.end(), groupedVertices[family].begin(), groupedVertices[family].end());
        staged.indices.insert(staged.indices.end(), groupedIndices[family].begin(), groupedIndices[family].end());
        staged.primitives.push_back(primitive);
        staged.nodeTransforms.push_back({"tomb-dressing-world-baked", {{
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        }}});
        candidateReport.addedVertices += groupedVertices[family].size();
        candidateReport.addedIndices += groupedIndices[family].size();
        candidateReport.addedTriangles += groupedIndices[family].size() / 3u;
        ++candidateReport.addedPrimitives;
    }
    if (staged.primitives.size() > kMaximumPrimitiveCount || staged.materials.size() > kMaximumMaterialCount)
    {
        diagnostic = "Tomb dressing exceeds the shared static primitive/material capacity of 32.";
        return false;
    }
    candidateReport.addedMaterials = staged.materials.size() - existingOwner.materials.size();
    for (std::size_t axis = 0u; axis < 3u; ++axis)
    {
        staged.bounds.minimum[axis] = std::min(staged.bounds.minimum[axis], candidateReport.addedBounds.minimum[axis]);
        staged.bounds.maximum[axis] = std::max(staged.bounds.maximum[axis], candidateReport.addedBounds.maximum[axis]);
    }
    existingOwner = std::move(staged);
    report = candidateReport;
    diagnostic.clear();
    return true;
}

} // namespace horde::scene
