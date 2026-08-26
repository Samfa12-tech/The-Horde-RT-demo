#include "scene/assets/StaticMeshAsset.h"

#include "scene/assets/AssetValidation.h"
#include "scene/assets/GltfDocument.h"
#include "third_party/cgltf/cgltf.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

namespace horde::scene::assets
{

namespace
{

const cgltf_accessor* FindAttribute(const cgltf_primitive& primitive,
                                    cgltf_attribute_type type,
                                    int index = 0)
{
    for (std::size_t i = 0u; i < primitive.attributes_count; ++i)
    {
        const cgltf_attribute& attribute = primitive.attributes[i];
        if (attribute.type == type && attribute.index == index) return attribute.data;
    }
    return nullptr;
}

std::size_t AccessorIndex(const cgltf_data& data, const cgltf_accessor* accessor)
{
    return accessor == nullptr ? data.accessors_count
                               : static_cast<std::size_t>(accessor - data.accessors);
}

std::int32_t TextureIndex(const cgltf_data& data, const cgltf_texture_view& view)
{
    return view.texture == nullptr
        ? -1
        : static_cast<std::int32_t>(view.texture - data.textures);
}

using TextureKey = std::pair<std::int32_t, std::int32_t>;
using TextureLayerRoutes = std::array<std::map<TextureKey, std::int32_t>, 4u>;

std::string AssetIdentityStem(const std::filesystem::path& path)
{
    std::string stem = path.stem().string();
    constexpr std::string_view runtimeSuffix = ".runtime";
    if (stem.ends_with(runtimeSuffix)) stem.resize(stem.size() - runtimeSuffix.size());
    return stem;
}

bool HasExactLodIdentitySuffix(std::string_view assetIdentity,
                               std::string_view lodName)
{
    if (lodName.empty()) return false;
    if (assetIdentity == lodName) return true;
    if (assetIdentity.size() <= lodName.size() ||
        !assetIdentity.ends_with(lodName))
    {
        return false;
    }
    const char delimiter = assetIdentity[assetIdentity.size() - lodName.size() - 1u];
    return delimiter == '.' || delimiter == '-' || delimiter == '_';
}

std::int32_t CategoryTextureLayer(std::map<TextureKey, std::int32_t>& routes,
                                  TextureKey key)
{
    if (key.first < 0 && key.second < 0) return -1;
    const auto [entry, inserted] = routes.try_emplace(
        key, static_cast<std::int32_t>(routes.size()));
    return entry->second;
}

bool ValidateAttribute(const cgltf_data& data,
                       const cgltf_accessor& accessor,
                       cgltf_type type,
                       std::string_view label,
                       std::string& diagnostic)
{
    if (accessor.component_type != cgltf_component_type_r_32f || accessor.type != type)
    {
        diagnostic = "Static GLB " + std::string(label) + " accessor layout is unsupported.";
        return false;
    }
    return ValidateAccessorRange(accessor, AccessorIndex(data, &accessor), diagnostic);
}

bool UnpackFloats(const cgltf_accessor& accessor,
                  std::size_t componentCount,
                  std::vector<float>& result,
                  std::string& diagnostic)
{
    result.resize(accessor.count * componentCount);
    const std::size_t unpacked = cgltf_accessor_unpack_floats(
        &accessor, result.data(), result.size());
    if (unpacked != result.size())
    {
        diagnostic = "Static GLB accessor data is out of range.";
        return false;
    }
    return true;
}

std::array<float, 3u> TransformPoint(const std::array<float, 16u>& matrix,
                                    const float* point,
                                    float metresPerUnit)
{
    return {{
        matrix[0] * point[0] * metresPerUnit +
            matrix[4] * point[1] * metresPerUnit +
            matrix[8] * point[2] * metresPerUnit + matrix[12],
        matrix[1] * point[0] * metresPerUnit +
            matrix[5] * point[1] * metresPerUnit +
            matrix[9] * point[2] * metresPerUnit + matrix[13],
        matrix[2] * point[0] * metresPerUnit +
            matrix[6] * point[1] * metresPerUnit +
            matrix[10] * point[2] * metresPerUnit + matrix[14]}};
}

std::array<float, 3u> Cross(const std::array<float, 3u>& left,
                           const std::array<float, 3u>& right)
{
    return {{left[1] * right[2] - left[2] * right[1],
             left[2] * right[0] - left[0] * right[2],
             left[0] * right[1] - left[1] * right[0]}};
}

float Dot(const std::array<float, 3u>& left, const std::array<float, 3u>& right)
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

bool Normalize(std::array<float, 3u>& vector)
{
    const float lengthSquared = Dot(vector, vector);
    if (!std::isfinite(lengthSquared) || lengthSquared <= 1.0e-20f) return false;
    const float inverseLength = 1.0f / std::sqrt(lengthSquared);
    for (float& component : vector) component *= inverseLength;
    return true;
}

bool TransformNormal(const std::array<float, 16u>& matrix,
                     const float* normal,
                     std::array<float, 3u>& transformed)
{
    const std::array<float, 3u> column0{{matrix[0], matrix[1], matrix[2]}};
    const std::array<float, 3u> column1{{matrix[4], matrix[5], matrix[6]}};
    const std::array<float, 3u> column2{{matrix[8], matrix[9], matrix[10]}};
    const auto cofactor0 = Cross(column1, column2);
    const auto cofactor1 = Cross(column2, column0);
    const auto cofactor2 = Cross(column0, column1);
    const float determinant = Dot(column0, cofactor0);
    if (!std::isfinite(determinant) || std::abs(determinant) <= 1.0e-20f) return false;
    const float inverseDeterminant = 1.0f / determinant;
    for (std::size_t axis = 0u; axis < 3u; ++axis)
    {
        transformed[axis] = (cofactor0[axis] * normal[0] +
                             cofactor1[axis] * normal[1] +
                             cofactor2[axis] * normal[2]) * inverseDeterminant;
    }
    return Normalize(transformed);
}

bool TransformTangent(const std::array<float, 16u>& matrix,
                      const float* tangent,
                      const std::array<float, 3u>& transformedNormal,
                      std::array<float, 3u>& transformed)
{
    transformed = {{
        matrix[0] * tangent[0] + matrix[4] * tangent[1] + matrix[8] * tangent[2],
        matrix[1] * tangent[0] + matrix[5] * tangent[1] + matrix[9] * tangent[2],
        matrix[2] * tangent[0] + matrix[6] * tangent[1] + matrix[10] * tangent[2]}};
    const float normalProjection = Dot(transformed, transformedNormal);
    for (std::size_t axis = 0u; axis < 3u; ++axis)
        transformed[axis] -= transformedNormal[axis] * normalProjection;
    return Normalize(transformed);
}

bool CheckTextureCapacity(std::int32_t layer,
                          std::uint32_t capacity,
                          std::string_view category,
                          std::string& diagnostic)
{
    if (layer >= 0 && static_cast<std::uint32_t>(layer) >= capacity)
    {
        diagnostic = "Static GLB exceeds manifest maxTextureLayersPerKind capacity for " +
                     std::string(category) + ".";
        return false;
    }
    return true;
}

StaticMaterial ConvertMaterial(const cgltf_data& data,
                               const cgltf_material& source,
                               TextureLayerRoutes& textureRoutes)
{
    StaticMaterial result;
    if (source.name != nullptr) result.name = source.name;
    if (source.has_pbr_metallic_roughness)
    {
        std::copy_n(source.pbr_metallic_roughness.base_color_factor, 4u,
                    result.baseColorFactor.begin());
        result.metallicFactor = source.pbr_metallic_roughness.metallic_factor;
        result.roughnessFactor = source.pbr_metallic_roughness.roughness_factor;
        result.baseColorTexture = CategoryTextureLayer(
            textureRoutes[0], {TextureIndex(data, source.pbr_metallic_roughness.base_color_texture), -1});
    }
    std::copy_n(source.emissive_factor, 3u, result.emissiveFactor.begin());
    result.normalTexture = CategoryTextureLayer(
        textureRoutes[1], {TextureIndex(data, source.normal_texture), -1});
    result.ormTexture = CategoryTextureLayer(
        textureRoutes[2],
        {source.has_pbr_metallic_roughness
             ? TextureIndex(data, source.pbr_metallic_roughness.metallic_roughness_texture)
             : -1,
         TextureIndex(data, source.occlusion_texture)});
    result.emissiveTexture = CategoryTextureLayer(
        textureRoutes[3], {TextureIndex(data, source.emissive_texture), -1});
    result.occlusionStrength = source.occlusion_texture.texture != nullptr
        ? source.occlusion_texture.scale : 1.0f;
    if (source.has_transmission) result.transmissionFactor = source.transmission.transmission_factor;
    if (source.has_ior) result.ior = source.ior.ior;
    if (source.has_volume)
    {
        result.thicknessFactor = source.volume.thickness_factor;
        result.attenuationDistance = source.volume.attenuation_distance;
        std::copy_n(source.volume.attenuation_color, 3u, result.attenuationColor.begin());
    }
    if (source.has_emissive_strength)
        result.emissiveStrength = source.emissive_strength.emissive_strength;
    if (source.double_sided) result.flags |= 1u;
    if (source.alpha_mode != cgltf_alpha_mode_opaque) result.flags |= 2u;
    if (result.transmissionFactor > 0.0f) result.flags |= 4u;
    if (result.name == "HeadPrimaryMasked") result.flags |= 128u;
    if (result.name == "NearFacePrimaryMasked") result.flags |= 256u;
    return result;
}

bool IsFiniteMaterial(const StaticMaterial& material)
{
    const auto finiteRange = [](const auto& values) {
        return std::all_of(values.begin(), values.end(), [](float value) { return std::isfinite(value); });
    };
    return finiteRange(material.baseColorFactor) && finiteRange(material.emissiveFactor) &&
           finiteRange(material.attenuationColor) && std::isfinite(material.emissiveStrength) &&
           std::isfinite(material.metallicFactor) && std::isfinite(material.roughnessFactor) &&
           std::isfinite(material.occlusionStrength) && std::isfinite(material.transmissionFactor) &&
           std::isfinite(material.ior) && std::isfinite(material.thicknessFactor) &&
           (std::isfinite(material.attenuationDistance) ||
            material.attenuationDistance == std::numeric_limits<float>::max());
}

using WeldCell = std::array<std::int64_t, 3u>;
using WeldVertexId = std::size_t;
using UndirectedEdge = std::pair<WeldVertexId, WeldVertexId>;

struct EdgeOrientationCount
{
    std::uint32_t ascending = 0u;
    std::uint32_t descending = 0u;
};

struct DielectricTriangle
{
    std::array<std::array<double, 3u>, 3u> positions{};
    std::array<WeldVertexId, 3u> welded{};
    std::size_t primitiveIndex = 0u;
    std::string nodeName;
};

struct EdgeReference
{
    std::size_t triangleIndex = 0u;
    bool ascending = false;
};

double DielectricWeldTolerance(const std::array<double, 3u>& minimum,
                               const std::array<double, 3u>& maximum)
{
    double extent = 0.0;
    for (std::size_t axis = 0u; axis < 3u; ++axis)
        extent = std::max(extent, maximum[axis] - minimum[axis]);
    // Baked positions are metres. One millionth of material extent, clamped
    // to 0.1-10 micrometres, welds transform-rounding equivalents while keeping
    // authored shells separated by more than the documented 10 um ceiling.
    return std::clamp(extent * 1.0e-6, 1.0e-7, 1.0e-5);
}

bool WeldCellForPosition(const std::array<double, 3u>& position,
                         const std::array<double, 3u>& origin,
                         double tolerance,
                         WeldCell& cell)
{
    constexpr double maximumCell =
        static_cast<double>(std::numeric_limits<std::int64_t>::max() - 1);
    for (std::size_t axis = 0u; axis < cell.size(); ++axis)
    {
        const double scaled = (position[axis] - origin[axis]) / tolerance;
        if (!std::isfinite(scaled) || scaled < 0.0 || scaled > maximumCell) return false;
        cell[axis] = static_cast<std::int64_t>(std::floor(scaled));
    }
    return true;
}

double SquaredDistance(const std::array<double, 3u>& left,
                       const std::array<double, 3u>& right)
{
    double result = 0.0;
    for (std::size_t axis = 0u; axis < left.size(); ++axis)
    {
        const double delta = left[axis] - right[axis];
        result += delta * delta;
    }
    return result;
}

bool BuildDielectricWelds(std::vector<DielectricTriangle>& triangles,
                          const std::array<double, 3u>& origin,
                          double tolerance,
                          std::vector<std::array<double, 3u>>& representatives,
                          std::string_view materialName,
                          std::string& diagnostic)
{
    struct Occurrence
    {
        std::array<double, 3u> position{};
        std::size_t triangleIndex = 0u;
        std::size_t corner = 0u;
    };

    std::vector<Occurrence> occurrences;
    occurrences.reserve(triangles.size() * 3u);
    for (std::size_t triangleIndex = 0u;
         triangleIndex < triangles.size(); ++triangleIndex)
    {
        for (std::size_t corner = 0u; corner < 3u; ++corner)
            occurrences.push_back(
                Occurrence{triangles[triangleIndex].positions[corner],
                           triangleIndex, corner});
    }
    std::sort(occurrences.begin(), occurrences.end(),
              [](const Occurrence& left, const Occurrence& right) {
                  if (left.position != right.position)
                      return left.position < right.position;
                  if (left.triangleIndex != right.triangleIndex)
                      return left.triangleIndex < right.triangleIndex;
                  return left.corner < right.corner;
              });

    // Only canonical representatives enter the spatial buckets. Processing
    // positions lexicographically and choosing the nearest representative
    // (lowest stable id on an exact tie) makes the result independent of
    // primitive traversal and prevents transitive proximity chains.
    std::map<WeldCell, std::vector<WeldVertexId>> buckets;
    const double toleranceSquared = tolerance * tolerance;
    for (const Occurrence& occurrence : occurrences)
    {
        WeldCell cell{};
        if (!WeldCellForPosition(occurrence.position, origin, tolerance, cell))
        {
            const DielectricTriangle& triangle = triangles[occurrence.triangleIndex];
            diagnostic = "Static GLB thick transmissive material '" +
                std::string(materialName) + "' (node '" + triangle.nodeName +
                "' primitive " + std::to_string(triangle.primitiveIndex) +
                ") exceeds the deterministic dielectric weld coordinate range.";
            return false;
        }

        WeldVertexId best = std::numeric_limits<WeldVertexId>::max();
        double bestDistanceSquared = std::numeric_limits<double>::max();
        for (std::int64_t x = -1; x <= 1; ++x)
        {
            for (std::int64_t y = -1; y <= 1; ++y)
            {
                for (std::int64_t z = -1; z <= 1; ++z)
                {
                    const std::array<std::int64_t, 3u> offset{{x, y, z}};
                    WeldCell neighbor{};
                    bool validNeighbor = true;
                    for (std::size_t axis = 0u; axis < neighbor.size(); ++axis)
                    {
                        if ((offset[axis] < 0 && cell[axis] == 0) ||
                            (offset[axis] > 0 &&
                             cell[axis] == std::numeric_limits<std::int64_t>::max()))
                        {
                            validNeighbor = false;
                            break;
                        }
                        neighbor[axis] = cell[axis] + offset[axis];
                    }
                    if (!validNeighbor) continue;
                    const auto bucket = buckets.find(neighbor);
                    if (bucket == buckets.end()) continue;
                    for (const WeldVertexId candidate : bucket->second)
                    {
                        const double distanceSquared =
                            SquaredDistance(occurrence.position,
                                            representatives[candidate]);
                        if (distanceSquared > toleranceSquared) continue;
                        if (distanceSquared < bestDistanceSquared ||
                            (distanceSquared == bestDistanceSquared && candidate < best))
                        {
                            best = candidate;
                            bestDistanceSquared = distanceSquared;
                        }
                    }
                }
            }
        }
        if (best == std::numeric_limits<WeldVertexId>::max())
        {
            best = representatives.size();
            representatives.push_back(occurrence.position);
            buckets[cell].push_back(best);
        }
        triangles[occurrence.triangleIndex].welded[occurrence.corner] = best;
    }
    return true;
}

std::string DielectricComponentSources(
    const std::vector<DielectricTriangle>& triangles,
    const std::vector<std::size_t>& component)
{
    std::set<std::pair<std::size_t, std::string>> uniqueSources;
    for (const std::size_t triangleIndex : component)
    {
        const DielectricTriangle& triangle = triangles[triangleIndex];
        uniqueSources.emplace(triangle.primitiveIndex, triangle.nodeName);
    }
    std::ostringstream stream;
    bool first = true;
    for (const auto& [primitiveIndex, nodeName] : uniqueSources)
    {
        if (!first) stream << ", ";
        first = false;
        stream << "node '" << (nodeName.empty() ? "<unnamed>" : nodeName)
               << "' primitive " << primitiveIndex;
    }
    return stream.str();
}

bool ValidateThickDielectricTopology(const StaticMeshAsset& asset,
                                     std::string& diagnostic)
{
    constexpr std::uint32_t thinWallFlag = 512u;
    for (std::size_t materialIndex = 0u; materialIndex < asset.materials.size(); ++materialIndex)
    {
        const StaticMaterial& material = asset.materials[materialIndex];
        if (material.transmissionFactor <= 0.0f || material.thicknessFactor <= 0.0f ||
            (material.flags & thinWallFlag) != 0u)
            continue;
        std::vector<DielectricTriangle> triangles;
        std::array<double, 3u> minimum{{
            std::numeric_limits<double>::max(),
            std::numeric_limits<double>::max(),
            std::numeric_limits<double>::max()}};
        std::array<double, 3u> maximum{{
            std::numeric_limits<double>::lowest(),
            std::numeric_limits<double>::lowest(),
            std::numeric_limits<double>::lowest()}};
        for (std::size_t primitiveIndex = 0u;
             primitiveIndex < asset.primitives.size(); ++primitiveIndex)
        {
            const StaticPrimitiveRecord& primitive = asset.primitives[primitiveIndex];
            if (primitive.materialIndex != materialIndex) continue;
            for (std::uint32_t index = 0u; index < primitive.indexCount; index += 3u)
            {
                DielectricTriangle triangle;
                triangle.primitiveIndex = primitiveIndex;
                triangle.nodeName =
                    primitive.nodeTransformIndex < asset.nodeTransforms.size()
                    ? asset.nodeTransforms[primitive.nodeTransformIndex].name
                    : std::string("<unknown>");
                for (std::size_t corner = 0u; corner < triangle.positions.size(); ++corner)
                {
                    const std::uint32_t localVertex =
                        asset.indices[primitive.indexOffset + index + corner];
                    const StaticRtVertex& vertex =
                        asset.vertices[primitive.vertexOffset + localVertex];
                    for (std::size_t axis = 0u; axis < 3u; ++axis)
                    {
                        triangle.positions[corner][axis] = vertex.position[axis];
                        minimum[axis] = std::min(minimum[axis],
                                                 triangle.positions[corner][axis]);
                        maximum[axis] = std::max(maximum[axis],
                                                 triangle.positions[corner][axis]);
                    }
                }
                triangles.push_back(std::move(triangle));
            }
        }
        if (triangles.empty()) continue;

        const double weldTolerance = DielectricWeldTolerance(minimum, maximum);
        std::vector<std::array<double, 3u>> weldRepresentatives;
        if (!BuildDielectricWelds(triangles, minimum, weldTolerance,
                                  weldRepresentatives, material.name,
                                  diagnostic))
            return false;
        std::map<UndirectedEdge, std::vector<EdgeReference>> allEdgeReferences;
        for (std::size_t triangleIndex = 0u;
             triangleIndex < triangles.size(); ++triangleIndex)
        {
            DielectricTriangle& triangle = triangles[triangleIndex];
            for (std::size_t edge = 0u; edge < triangle.welded.size(); ++edge)
            {
                WeldVertexId a = triangle.welded[edge];
                WeldVertexId b = triangle.welded[(edge + 1u) % triangle.welded.size()];
                if (a == b)
                {
                    diagnostic = "Static GLB thick transmissive material '" +
                        material.name + "' (node '" + triangle.nodeName +
                        "' primitive " + std::to_string(triangle.primitiveIndex) +
                        ") contains a degenerate triangle edge after the documented " +
                        std::to_string(weldTolerance) + " metre weld tolerance.";
                    return false;
                }
                const bool ascending = a < b;
                if (!ascending) std::swap(a, b);
                allEdgeReferences[{a, b}].push_back(
                    EdgeReference{triangleIndex, ascending});
            }
        }

        std::vector<std::vector<std::size_t>> adjacency(triangles.size());
        for (const auto& [edge, references] : allEdgeReferences)
        {
            (void)edge;
            for (std::size_t left = 0u; left < references.size(); ++left)
            {
                for (std::size_t right = left + 1u; right < references.size(); ++right)
                {
                    const std::size_t a = references[left].triangleIndex;
                    const std::size_t b = references[right].triangleIndex;
                    if (a == b) continue;
                    adjacency[a].push_back(b);
                    adjacency[b].push_back(a);
                }
            }
        }

        struct DielectricComponent
        {
            std::vector<std::size_t> triangles;
            std::vector<WeldVertexId> vertexKey;
        };
        std::vector<bool> visited(triangles.size(), false);
        std::vector<DielectricComponent> components;
        for (std::size_t firstTriangle = 0u;
             firstTriangle < triangles.size(); ++firstTriangle)
        {
            if (visited[firstTriangle]) continue;
            DielectricComponent component;
            std::vector<std::size_t> pending{firstTriangle};
            visited[firstTriangle] = true;
            while (!pending.empty())
            {
                const std::size_t triangleIndex = pending.back();
                pending.pop_back();
                component.triangles.push_back(triangleIndex);
                for (const std::size_t neighbor : adjacency[triangleIndex])
                {
                    if (visited[neighbor]) continue;
                    visited[neighbor] = true;
                    pending.push_back(neighbor);
                }
            }
            for (const std::size_t triangleIndex : component.triangles)
                component.vertexKey.insert(component.vertexKey.end(),
                                           triangles[triangleIndex].welded.begin(),
                                           triangles[triangleIndex].welded.end());
            std::sort(component.vertexKey.begin(), component.vertexKey.end());
            component.vertexKey.erase(
                std::unique(component.vertexKey.begin(), component.vertexKey.end()),
                component.vertexKey.end());
            std::sort(component.triangles.begin(), component.triangles.end(),
                      [&triangles](std::size_t left, std::size_t right) {
                          if (triangles[left].welded != triangles[right].welded)
                              return triangles[left].welded < triangles[right].welded;
                          if (triangles[left].positions != triangles[right].positions)
                              return triangles[left].positions < triangles[right].positions;
                          if (triangles[left].nodeName != triangles[right].nodeName)
                              return triangles[left].nodeName < triangles[right].nodeName;
                          return triangles[left].primitiveIndex <
                                 triangles[right].primitiveIndex;
                      });
            components.push_back(std::move(component));
        }
        std::sort(components.begin(), components.end(),
                  [](const DielectricComponent& left,
                     const DielectricComponent& right) {
                      return left.vertexKey < right.vertexKey;
                  });

        for (std::size_t componentIndex = 0u;
             componentIndex < components.size(); ++componentIndex)
        {
            const std::size_t componentNumber = componentIndex + 1u;
            const std::vector<std::size_t>& component =
                components[componentIndex].triangles;
            std::map<UndirectedEdge, EdgeOrientationCount> componentEdges;
            for (const std::size_t triangleIndex : component)
            {
                const DielectricTriangle& triangle = triangles[triangleIndex];
                for (std::size_t edge = 0u; edge < triangle.welded.size(); ++edge)
                {
                    WeldVertexId a = triangle.welded[edge];
                    WeldVertexId b =
                        triangle.welded[(edge + 1u) % triangle.welded.size()];
                    const bool ascending = a < b;
                    if (!ascending) std::swap(a, b);
                    EdgeOrientationCount& orientation = componentEdges[{a, b}];
                    if (ascending) ++orientation.ascending;
                    else ++orientation.descending;
                }
            }

            const std::string prefix = "Static GLB thick transmissive material '" +
                material.name + "' component " + std::to_string(componentNumber) +
                " (" + DielectricComponentSources(triangles, component) + ") ";
            for (const auto& [edge, orientation] : componentEdges)
            {
                (void)edge;
                const std::uint32_t referenceCount =
                    orientation.ascending + orientation.descending;
                if (referenceCount == 1u)
                {
                    diagnostic = prefix +
                        "is open: a boundary edge is referenced once. Use closed manifold geometry or set thinWall in the audited material override.";
                    return false;
                }
                if (referenceCount > 2u)
                {
                    diagnostic = prefix +
                        "is non-manifold: an edge is referenced more than twice. Repair the component topology before import.";
                    return false;
                }
                if (orientation.ascending != 1u || orientation.descending != 1u)
                {
                    diagnostic = prefix +
                        "has inconsistent winding: each shared edge must be used once in each direction.";
                    return false;
                }
            }

            const std::array<double, 3u>& origin =
                weldRepresentatives[components[componentIndex].vertexKey.front()];
            double signedSixTimesVolume = 0.0;
            for (const std::size_t triangleIndex : component)
            {
                std::array<std::array<double, 3u>, 3u> relative =
                    triangles[triangleIndex].positions;
                for (auto& position : relative)
                    for (std::size_t axis = 0u; axis < 3u; ++axis)
                        position[axis] -= origin[axis];
                signedSixTimesVolume +=
                    relative[0][0] * (relative[1][1] * relative[2][2] -
                                      relative[1][2] * relative[2][1]) +
                    relative[0][1] * (relative[1][2] * relative[2][0] -
                                      relative[1][0] * relative[2][2]) +
                    relative[0][2] * (relative[1][0] * relative[2][1] -
                                      relative[1][1] * relative[2][0]);
            }
            if (!std::isfinite(signedSixTimesVolume) ||
                signedSixTimesVolume <= 1.0e-18)
            {
                diagnostic = prefix +
                    "is inward-wound after baked node transforms; reverse every triangle winding so normals face outward.";
                return false;
            }
        }
    }
    return true;
}

bool ValidateUnsupportedFeatures(const cgltf_data& data, std::string& diagnostic)
{
    for (std::size_t accessorIndex = 0u; accessorIndex < data.accessors_count; ++accessorIndex)
    {
        if (data.accessors[accessorIndex].is_sparse)
        {
            diagnostic = "Static GLB accessor " + std::to_string(accessorIndex) +
                         " uses sparse data, which is unsupported.";
            return false;
        }
    }
    for (std::size_t viewIndex = 0u; viewIndex < data.buffer_views_count; ++viewIndex)
    {
        if (data.buffer_views[viewIndex].has_meshopt_compression)
        {
            diagnostic = "Static GLB buffer view " + std::to_string(viewIndex) +
                         " uses meshopt compression, which is unsupported.";
            return false;
        }
    }
    std::size_t primitiveIndex = 0u;
    for (std::size_t meshIndex = 0u; meshIndex < data.meshes_count; ++meshIndex)
    {
        const cgltf_mesh& mesh = data.meshes[meshIndex];
        for (std::size_t localPrimitive = 0u; localPrimitive < mesh.primitives_count;
             ++localPrimitive, ++primitiveIndex)
        {
            const cgltf_primitive& primitive = mesh.primitives[localPrimitive];
            if (primitive.targets_count != 0u)
            {
                diagnostic = "Static GLB primitive " + std::to_string(primitiveIndex) +
                             " uses morph targets, which are unsupported.";
                return false;
            }
            if (primitive.has_draco_mesh_compression)
            {
                diagnostic = "Static GLB primitive " + std::to_string(primitiveIndex) +
                             " uses Draco compression, which is unsupported.";
                return false;
            }
        }
    }
    for (std::size_t extensionIndex = 0u;
         extensionIndex < data.extensions_required_count; ++extensionIndex)
    {
        const char* extension = data.extensions_required[extensionIndex];
        if (!IsSupportedRequiredExtension(extension))
        {
            diagnostic = "Static GLB requires unsupported extension '" +
                         std::string(extension != nullptr ? extension : "<null>") + "'.";
            return false;
        }
    }
    return true;
}

} // namespace

bool StaticMeshAsset::Load(const std::filesystem::path& runtimeGlb,
                           const AssetManifest& manifest,
                           StaticMeshAsset& asset,
                           std::string& diagnostic)
{
    asset = {};
    GltfDocument document;
    if (!GltfDocument::Load(runtimeGlb, document, diagnostic)) return false;
    const cgltf_data& data = *document.Data();

    if (!ValidateUnsupportedFeatures(data, diagnostic)) return false;
    for (std::size_t nodeIndex = 0u; nodeIndex < data.nodes_count; ++nodeIndex)
    {
        if (!ValidateNodeTransform(data.nodes[nodeIndex], diagnostic)) return false;
    }
    if (data.materials_count > manifest.budgets.maxMaterials)
    {
        diagnostic = "Static GLB exceeds manifest maxMaterials capacity.";
        return false;
    }
    const AssetLodBudget* selectedLod = nullptr;
    if (manifest.lods.size() == 1u)
    {
        selectedLod = &manifest.lods.front();
    }
    else
    {
        const std::string assetIdentity = AssetIdentityStem(runtimeGlb.filename());
        for (const AssetLodBudget& lod : manifest.lods)
        {
            if (!HasExactLodIdentitySuffix(assetIdentity, lod.name)) continue;
            if (selectedLod != nullptr)
            {
                diagnostic = "Static RT asset filename must select exactly one manifest LOD identity.";
                return false;
            }
            selectedLod = &lod;
        }
    }
    if (selectedLod == nullptr)
    {
        diagnostic = "Static RT asset filename must select exactly one manifest LOD identity.";
        return false;
    }

    asset.materials.reserve(data.materials_count);
    TextureLayerRoutes textureRoutes;
    for (std::size_t materialIndex = 0u; materialIndex < data.materials_count; ++materialIndex)
    {
        StaticMaterial material = ConvertMaterial(data, data.materials[materialIndex], textureRoutes);
        for (const MaterialOverride& materialOverride : manifest.materialOverrides)
        {
            if (materialOverride.material == material.name)
            {
                if (materialOverride.hasEmissiveStrength)
                    material.emissiveStrength = materialOverride.emissiveStrength;
                if (materialOverride.hasTransmissionFactor)
                    material.transmissionFactor = materialOverride.transmissionFactor;
                if (materialOverride.hasIor) material.ior = materialOverride.ior;
                if (materialOverride.hasThicknessFactor)
                    material.thicknessFactor = materialOverride.thicknessFactor;
                if (materialOverride.hasAttenuationDistance)
                    material.attenuationDistance = materialOverride.attenuationDistance;
                if (materialOverride.hasAttenuationColor)
                    material.attenuationColor = materialOverride.attenuationColor;
                if (materialOverride.hasRoughnessFactor)
                    material.roughnessFactor = materialOverride.roughnessFactor;
                if (materialOverride.hasThinWall)
                {
                    if (materialOverride.thinWall) material.flags |= 512u;
                    else material.flags &= ~512u;
                }
            }
        }
        if (material.transmissionFactor > 0.0f)
        {
            material.flags |= 4u;
            // KHR_materials_volume defines zero thickness (including an absent
            // extension) as a surface-only transmission material. Keep that
            // glTF semantic deterministic even if a sidecar incorrectly asks
            // to treat a zero-thickness surface as a closed volume.
            if (material.thicknessFactor <= 0.0f) material.flags |= 512u;
        }
        else
        {
            material.flags &= ~4u;
            material.flags &= ~512u;
        }
        if (!IsFiniteMaterial(material))
        {
            diagnostic = "Static GLB material " + std::to_string(materialIndex) +
                         " contains a non-finite PBR factor.";
            return false;
        }
        if (!CheckTextureCapacity(material.baseColorTexture,
                                  manifest.budgets.maxTextureLayersPerKind,
                                  "baseColor", diagnostic) ||
            !CheckTextureCapacity(material.normalTexture,
                                  manifest.budgets.maxTextureLayersPerKind,
                                  "normal", diagnostic) ||
            !CheckTextureCapacity(material.ormTexture,
                                  manifest.budgets.maxTextureLayersPerKind,
                                  "ORM", diagnostic) ||
            !CheckTextureCapacity(material.emissiveTexture,
                                  manifest.budgets.maxTextureLayersPerKind,
                                  "emissive", diagnostic))
        {
            return false;
        }
        asset.materials.push_back(std::move(material));
    }

    asset.nodeTransforms.reserve(data.nodes_count);
    for (std::size_t nodeIndex = 0u; nodeIndex < data.nodes_count; ++nodeIndex)
    {
        StaticNodeTransform transform;
        if (data.nodes[nodeIndex].name != nullptr) transform.name = data.nodes[nodeIndex].name;
        cgltf_node_transform_world(&data.nodes[nodeIndex], transform.world.data());
        transform.world[12] *= manifest.metresPerUnit;
        transform.world[13] *= manifest.metresPerUnit;
        transform.world[14] *= manifest.metresPerUnit;
        if (!std::all_of(transform.world.begin(), transform.world.end(),
                         [](float value) { return std::isfinite(value); }))
        {
            diagnostic = "Static GLB node '" +
                         (transform.name.empty() ? std::string("<unnamed>") : transform.name) +
                         "' contains a non-finite transform.";
            return false;
        }
        asset.nodeTransforms.push_back(std::move(transform));
    }

    for (const std::string& requiredSocket : manifest.requiredSockets)
    {
        const auto node = std::find_if(
            asset.nodeTransforms.begin(), asset.nodeTransforms.end(),
            [&requiredSocket](const StaticNodeTransform& transform) {
                return transform.name == requiredSocket;
            });
        if (node == asset.nodeTransforms.end())
        {
            diagnostic = "Static GLB is missing required socket '" + requiredSocket + "'.";
            return false;
        }
        asset.sockets.push_back({requiredSocket,
            static_cast<std::uint32_t>(node - asset.nodeTransforms.begin()), node->world});
    }

    asset.bounds.minimum.fill(std::numeric_limits<float>::max());
    asset.bounds.maximum.fill(std::numeric_limits<float>::lowest());
    std::size_t primitiveIndex = 0u;
    std::uint64_t triangleCount = 0u;
    for (std::size_t nodeIndex = 0u; nodeIndex < data.nodes_count; ++nodeIndex)
    {
        const cgltf_node& node = data.nodes[nodeIndex];
        if (node.mesh == nullptr) continue;
        for (std::size_t localPrimitive = 0u; localPrimitive < node.mesh->primitives_count;
             ++localPrimitive, ++primitiveIndex)
        {
            const cgltf_primitive& primitive = node.mesh->primitives[localPrimitive];
            if (primitive.type != cgltf_primitive_type_triangles)
            {
                diagnostic = "Static GLB primitive " + std::to_string(primitiveIndex) +
                             " mode must be TRIANGLES.";
                return false;
            }
            const cgltf_accessor* positions = FindAttribute(primitive, cgltf_attribute_type_position);
            const cgltf_accessor* normals = FindAttribute(primitive, cgltf_attribute_type_normal);
            const cgltf_accessor* uv0 = FindAttribute(primitive, cgltf_attribute_type_texcoord, 0);
            const cgltf_accessor* tangents = FindAttribute(primitive, cgltf_attribute_type_tangent);
            if (positions == nullptr)
            {
                diagnostic = "Static GLB primitive " + std::to_string(primitiveIndex) +
                             " is missing POSITION.";
                return false;
            }
            if (normals == nullptr)
            {
                diagnostic = "Static GLB primitive " + std::to_string(primitiveIndex) +
                             " is missing NORMAL.";
                return false;
            }
            if (uv0 == nullptr)
            {
                diagnostic = "Static GLB primitive " + std::to_string(primitiveIndex) +
                             " is missing TEXCOORD_0.";
                return false;
            }
            const bool hasNormalTexture = primitive.material != nullptr &&
                                          primitive.material->normal_texture.texture != nullptr;
            if (tangents == nullptr && hasNormalTexture)
            {
                diagnostic = "Static GLB primitive " + std::to_string(primitiveIndex) +
                             " uses a normal texture but is missing TANGENT.";
                return false;
            }
            if (primitive.indices == nullptr ||
                primitive.indices->type != cgltf_type_scalar ||
                (primitive.indices->component_type != cgltf_component_type_r_16u &&
                 primitive.indices->component_type != cgltf_component_type_r_32u))
            {
                diagnostic = "Static GLB primitive " + std::to_string(primitiveIndex) +
                             " index layout must be unsigned 16-bit or 32-bit.";
                return false;
            }
            if (!ValidateAttribute(data, *positions, cgltf_type_vec3, "POSITION", diagnostic) ||
                !ValidateAttribute(data, *normals, cgltf_type_vec3, "NORMAL", diagnostic) ||
                !ValidateAttribute(data, *uv0, cgltf_type_vec2, "TEXCOORD_0", diagnostic) ||
                (tangents != nullptr &&
                 !ValidateAttribute(data, *tangents, cgltf_type_vec4, "TANGENT", diagnostic)) ||
                !ValidateAccessorRange(*primitive.indices,
                                       AccessorIndex(data, primitive.indices), diagnostic))
            {
                return false;
            }
            if (positions->count != normals->count || positions->count != uv0->count ||
                (tangents != nullptr && positions->count != tangents->count))
            {
                diagnostic = "Static GLB primitive " + std::to_string(primitiveIndex) +
                             " attribute counts do not match.";
                return false;
            }
            if ((primitive.indices->count % 3u) != 0u)
            {
                diagnostic = "Static GLB primitive " + std::to_string(primitiveIndex) +
                             " index count is not divisible by three.";
                return false;
            }
            if (primitive.material == nullptr)
            {
                diagnostic = "Static GLB primitive " + std::to_string(primitiveIndex) +
                             " is missing a material.";
                return false;
            }

            std::vector<float> unpackedPositions, unpackedNormals, unpackedUv, unpackedTangents;
            if (!UnpackFloats(*positions, 3u, unpackedPositions, diagnostic) ||
                !UnpackFloats(*normals, 3u, unpackedNormals, diagnostic) ||
                !UnpackFloats(*uv0, 2u, unpackedUv, diagnostic) ||
                (tangents != nullptr &&
                 !UnpackFloats(*tangents, 4u, unpackedTangents, diagnostic)))
            {
                return false;
            }

            StaticPrimitiveRecord record;
            record.vertexOffset = static_cast<std::uint32_t>(asset.vertices.size());
            record.indexOffset = static_cast<std::uint32_t>(asset.indices.size());
            record.indexCount = static_cast<std::uint32_t>(primitive.indices->count);
            record.materialIndex = static_cast<std::uint32_t>(primitive.material - data.materials);
            record.nodeTransformIndex = static_cast<std::uint32_t>(nodeIndex);
            const auto& nodeWorld = asset.nodeTransforms[nodeIndex].world;
            for (std::size_t vertexIndex = 0u; vertexIndex < positions->count; ++vertexIndex)
            {
                StaticRtVertex vertex;
                const auto transformedPosition = TransformPoint(
                    nodeWorld, unpackedPositions.data() + vertexIndex * 3u,
                    manifest.metresPerUnit);
                std::array<float, 3u> transformedNormal{};
                if (!TransformNormal(nodeWorld,
                                     unpackedNormals.data() + vertexIndex * 3u,
                                     transformedNormal))
                {
                    diagnostic = "Static GLB node '" +
                        (asset.nodeTransforms[nodeIndex].name.empty()
                            ? std::string("<unnamed>")
                            : asset.nodeTransforms[nodeIndex].name) +
                        "' has a non-invertible normal transform.";
                    return false;
                }
                const std::array<float, 4u> defaultTangent{{1.0f, 0.0f, 0.0f, 1.0f}};
                const float* sourceTangent = tangents != nullptr
                    ? unpackedTangents.data() + vertexIndex * 4u
                    : defaultTangent.data();
                std::array<float, 3u> transformedTangent{};
                if (!TransformTangent(nodeWorld, sourceTangent,
                                      transformedNormal, transformedTangent))
                {
                    diagnostic = "Static GLB primitive " + std::to_string(primitiveIndex) +
                                 " has a degenerate tangent after node transform.";
                    return false;
                }
                for (std::size_t axis = 0u; axis < 3u; ++axis)
                {
                    vertex.position[axis] = transformedPosition[axis];
                    vertex.normal[axis] = transformedNormal[axis];
                    vertex.tangent[axis] = transformedTangent[axis];
                    asset.bounds.minimum[axis] = std::min(asset.bounds.minimum[axis], vertex.position[axis]);
                    asset.bounds.maximum[axis] = std::max(asset.bounds.maximum[axis], vertex.position[axis]);
                }
                vertex.position[3] = 1.0f;
                vertex.normal[3] = 0.0f;
                vertex.tangent[3] = sourceTangent[3];
                vertex.uv0 = {{unpackedUv[vertexIndex * 2u],
                               unpackedUv[vertexIndex * 2u + 1u], 0.0f, 0.0f}};
                asset.vertices.push_back(vertex);
            }
            for (std::size_t index = 0u; index < primitive.indices->count; ++index)
            {
                const std::size_t sourceIndex = cgltf_accessor_read_index(primitive.indices, index);
                if (sourceIndex >= positions->count)
                {
                    diagnostic = "Static GLB index is out of range for primitive " +
                                 std::to_string(primitiveIndex) + ".";
                    return false;
                }
                asset.indices.push_back(static_cast<std::uint32_t>(sourceIndex));
            }
            asset.primitives.push_back(record);
            triangleCount += primitive.indices->count / 3u;

            if (asset.vertices.size() > manifest.budgets.maxVertices)
            {
                diagnostic = "Static GLB exceeds manifest maxVertices capacity.";
                return false;
            }
            if (asset.indices.size() > manifest.budgets.maxIndices)
            {
                diagnostic = "Static GLB exceeds manifest maxIndices capacity.";
                return false;
            }
            if (asset.primitives.size() > manifest.budgets.maxPrimitives)
            {
                diagnostic = "Static GLB exceeds manifest maxPrimitives capacity.";
                return false;
            }
            if (triangleCount > selectedLod->maxTriangles)
            {
                diagnostic = "Static GLB exceeds selected LOD '" + selectedLod->name +
                             "' maxTriangles capacity.";
                return false;
            }
        }
    }
    if (asset.primitives.empty())
    {
        diagnostic = "Static GLB contains no presentable triangle primitives.";
        return false;
    }
    if (cgltf_validate(const_cast<cgltf_data*>(&data)) != cgltf_result_success)
    {
        diagnostic = "Static GLB failed cgltf structural validation.";
        return false;
    }
    if (!ValidateThickDielectricTopology(asset, diagnostic)) return false;
    diagnostic.clear();
    return true;
}

} // namespace horde::scene::assets
