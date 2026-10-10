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

void IncludePoint(ForestDressingBounds& bounds, const std::array<float, 3u>& point)
{
    for (std::size_t axis = 0u; axis < 3u; ++axis)
    {
        bounds.minimum[axis] = std::min(bounds.minimum[axis], point[axis]);
        bounds.maximum[axis] = std::max(bounds.maximum[axis], point[axis]);
    }
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

} // namespace

bool AppendPreparedForestDressing(const std::filesystem::path& assetRoot,
                                  assets::StaticMeshAsset& existingOwner,
                                  ForestDressingBuildReport& report,
                                  std::string& diagnostic)
{
    report = {};
    if (existingOwner.vertices.empty() || existingOwner.indices.empty() ||
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

    std::array<assets::StaticMeshAsset, static_cast<std::size_t>(TreeId::Count)> sources{};
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

    std::array<assets::StaticMaterial, static_cast<std::size_t>(MaterialFamily::Count)> familyMaterials{};
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
    candidate.addedBounds.minimum.fill(std::numeric_limits<float>::max());
    candidate.addedBounds.maximum.fill(std::numeric_limits<float>::lowest());

    for (std::size_t placementIndex = 0u;
         placementIndex < gameplay::simulation::kForestTreePlacementContract.size();
         ++placementIndex)
    {
        const auto& contract = gameplay::simulation::kForestTreePlacementContract[placementIndex];
        const auto support = gameplay::simulation::ResolveWorldRouteSupport(contract.x, contract.z);
        if (!support.grounded || !std::isfinite(support.worldY))
        {
            diagnostic = "Forest tree placement has no measured native route support surface.";
            return false;
        }
        const TreeId treeId = contract.species == gameplay::simulation::ForestTreeSpecies::Pine
            ? TreeId::Pine : TreeId::Alder;
        const std::size_t treeIndex = static_cast<std::size_t>(treeId);
        const auto& source = sources[treeIndex];
        const AssetSpec& spec = kAssets[treeIndex];
        const float yaw = contract.yawRadians;
        const float originY = support.worldY - source.bounds.minimum[1];
        ForestDressingPlacement& placement = candidate.placements[placementIndex];
        placement.asset = spec.name;
        placement.origin = {{contract.x, originY, contract.z}};
        placement.yawRadians = yaw;
        placement.supportY = support.worldY;
        placement.worldBounds.minimum.fill(std::numeric_limits<float>::max());
        placement.worldBounds.maximum.fill(std::numeric_limits<float>::lowest());
        placement.trunkBounds.minimum.fill(std::numeric_limits<float>::max());
        placement.trunkBounds.maximum.fill(std::numeric_limits<float>::lowest());

            for (const auto& primitive : source.primitives)
            {
                if (primitive.materialIndex >= source.materials.size() ||
                    primitive.nodeTransformIndex >= source.nodeTransforms.size())
                {
                    diagnostic = "Forest native primitive references invalid material/transform data.";
                    return false;
                }
                const MaterialFamily family = ClassifyMaterial(source.materials[primitive.materialIndex], diagnostic);
                if (family == MaterialFamily::Count) return false;
                const std::size_t familyIndex = static_cast<std::size_t>(family);
                auto& vertices = groupedVertices[familyIndex];
                auto& indices = groupedIndices[familyIndex];
                const std::size_t vertexBase = vertices.size();
                const std::size_t indexBase = indices.size();
                std::size_t vertexEnd = source.vertices.size();
                for (const auto& next : source.primitives)
                    if (next.vertexOffset > primitive.vertexOffset)
                        vertexEnd = std::min(vertexEnd, static_cast<std::size_t>(next.vertexOffset));
                if (vertexEnd <= primitive.vertexOffset || vertexEnd > source.vertices.size() ||
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
                    if (family == MaterialFamily::Bark &&
                        source.vertices[vertexIndex].position[1] <=
                            source.bounds.minimum[1] + kTrunkCollisionHeight)
                        IncludePoint(placement.trunkBounds, position);
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
                if (vertices.size() - vertexBase != vertexEnd - primitive.vertexOffset ||
                    indices.size() - indexBase != primitive.indexCount)
                {
                    diagnostic = "Forest merge accounting failed for a material-family primitive.";
                    return false;
                }
            }
            if (std::abs(placement.worldBounds.minimum[1] - support.worldY) > 0.0001f)
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

} // namespace horde::scene
