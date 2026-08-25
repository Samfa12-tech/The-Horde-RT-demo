#include "scene/assets/StaticMeshAsset.h"

#include "scene/assets/AssetValidation.h"
#include "scene/assets/GltfDocument.h"
#include "third_party/cgltf/cgltf.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string_view>

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

std::int32_t TextureLayer(const cgltf_data& data, const cgltf_texture_view& view)
{
    return view.texture == nullptr
        ? -1
        : static_cast<std::int32_t>(view.texture - data.textures);
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

StaticMaterial ConvertMaterial(const cgltf_data& data, const cgltf_material& source)
{
    StaticMaterial result;
    if (source.name != nullptr) result.name = source.name;
    if (source.has_pbr_metallic_roughness)
    {
        std::copy_n(source.pbr_metallic_roughness.base_color_factor, 4u,
                    result.baseColorFactor.begin());
        result.metallicFactor = source.pbr_metallic_roughness.metallic_factor;
        result.roughnessFactor = source.pbr_metallic_roughness.roughness_factor;
        result.baseColorTexture = TextureLayer(data, source.pbr_metallic_roughness.base_color_texture);
        result.ormTexture = TextureLayer(data, source.pbr_metallic_roughness.metallic_roughness_texture);
    }
    std::copy_n(source.emissive_factor, 3u, result.emissiveFactor.begin());
    result.normalTexture = TextureLayer(data, source.normal_texture);
    result.emissiveTexture = TextureLayer(data, source.emissive_texture);
    result.occlusionStrength = source.occlusion_texture.texture != nullptr
        ? source.occlusion_texture.scale : 1.0f;
    if (result.ormTexture < 0) result.ormTexture = TextureLayer(data, source.occlusion_texture);
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

    asset.materials.reserve(data.materials_count);
    for (std::size_t materialIndex = 0u; materialIndex < data.materials_count; ++materialIndex)
    {
        StaticMaterial material = ConvertMaterial(data, data.materials[materialIndex]);
        for (const MaterialOverride& materialOverride : manifest.materialOverrides)
        {
            if (materialOverride.material == material.name)
                material.emissiveStrength = materialOverride.emissiveStrength;
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
            for (std::size_t vertexIndex = 0u; vertexIndex < positions->count; ++vertexIndex)
            {
                StaticRtVertex vertex;
                for (std::size_t axis = 0u; axis < 3u; ++axis)
                {
                    vertex.position[axis] = unpackedPositions[vertexIndex * 3u + axis] *
                                            manifest.metresPerUnit;
                    vertex.normal[axis] = unpackedNormals[vertexIndex * 3u + axis];
                    asset.bounds.minimum[axis] = std::min(asset.bounds.minimum[axis], vertex.position[axis]);
                    asset.bounds.maximum[axis] = std::max(asset.bounds.maximum[axis], vertex.position[axis]);
                }
                vertex.position[3] = 1.0f;
                vertex.normal[3] = 0.0f;
                if (tangents != nullptr)
                    std::copy_n(unpackedTangents.data() + vertexIndex * 4u, 4u, vertex.tangent.begin());
                else
                    vertex.tangent = {{1.0f, 0.0f, 0.0f, 1.0f}};
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
    diagnostic.clear();
    return true;
}

} // namespace horde::scene::assets
