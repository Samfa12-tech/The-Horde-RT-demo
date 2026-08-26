#include "vulkan/raytracing/RtStaticMeshSlot.h"

#include <algorithm>
#include <limits>
#include <unordered_map>

namespace horde::vulkan::raytracing
{

namespace
{

RtMaterialGpu ConvertMaterial(const horde::scene::assets::StaticMaterial& source,
                              const std::array<std::uint32_t, 4u>& textureLayers)
{
    RtMaterialGpu result{};
    result.baseColorFactor = source.baseColorFactor;
    result.emissiveFactorStrength = {{
        source.emissiveFactor[0], source.emissiveFactor[1],
        source.emissiveFactor[2], source.emissiveStrength}};
    result.metallicRoughnessOcclusionTransmission = {{
        source.metallicFactor, source.roughnessFactor,
        source.occlusionStrength, source.transmissionFactor}};
    result.iorThicknessAttenuationDistance = {{
        source.ior, source.thicknessFactor, source.attenuationDistance, 0.0f}};
    result.attenuationColor = {{
        source.attenuationColor[0], source.attenuationColor[1],
        source.attenuationColor[2], 0.0f}};
    result.textureLayers = textureLayers;
    result.materialFlags[0] = source.flags;
    if (source.baseColorTexture >= 0)
        result.materialFlags[0] |= static_cast<std::uint32_t>(RtMaterialFlag::BaseColorTexture);
    if (source.normalTexture >= 0)
        result.materialFlags[0] |= static_cast<std::uint32_t>(RtMaterialFlag::NormalTexture);
    if (source.ormTexture >= 0)
        result.materialFlags[0] |= static_cast<std::uint32_t>(RtMaterialFlag::OrmTexture);
    if (source.emissiveTexture >= 0)
        result.materialFlags[0] |= static_cast<std::uint32_t>(RtMaterialFlag::EmissiveTexture);
    return result;
}

} // namespace

bool RtStaticMeshSlot::Initialize(std::span<const StaticRtAssetRegistration> registrations,
                                  std::string& diagnostic)
{
    instanceMetadata_ = {};
    primitiveMetadata_.clear();
    materials_.clear();
    vertices_.clear();
    indices_.clear();
    geometryTransforms_.clear();
    textureArrayCounts_ = {};
    measurements_ = {};

    std::array<bool, kRtInstanceMetadataCapacity> occupied{};
    std::vector<const horde::scene::assets::StaticMeshAsset*> uniqueAssets;
    for (const StaticRtAssetRegistration& registration : registrations)
    {
        if (registration.instanceCustomIndex >= kRtInstanceMetadataCapacity)
        {
            diagnostic = "RtStaticMeshSlot capacity overflow: instanceCustomIndex exceeds RtInstanceMetadata[20].";
            return false;
        }
        if (occupied[registration.instanceCustomIndex])
        {
            diagnostic = "RtStaticMeshSlot registration duplicates instanceCustomIndex " +
                         std::to_string(registration.instanceCustomIndex) + ".";
            return false;
        }
        occupied[registration.instanceCustomIndex] = true;
        if (registration.asset == nullptr)
        {
            diagnostic = "RtStaticMeshSlot registration has no static asset.";
            return false;
        }
        if (std::find(uniqueAssets.begin(), uniqueAssets.end(), registration.asset) == uniqueAssets.end())
            uniqueAssets.push_back(registration.asset);
    }
    if (uniqueAssets.size() > kRtStaticAssetCapacity)
    {
        diagnostic = "RtStaticMeshSlot capacity overflow: static assets exceed 8.";
        return false;
    }

    struct AssetRoute
    {
        std::uint32_t assetIndex;
        std::uint32_t primitiveBase;
        std::uint32_t primitiveCount;
    };
    std::unordered_map<const horde::scene::assets::StaticMeshAsset*, AssetRoute> routes;
    std::array<std::uint32_t, 4u> nextTextureLayers{};
    for (std::size_t assetIndex = 0u; assetIndex < uniqueAssets.size(); ++assetIndex)
    {
        const auto& asset = *uniqueAssets[assetIndex];
        if (primitiveMetadata_.size() + asset.primitives.size() > kRtPrimitiveMetadataCapacity)
        {
            diagnostic = "RtStaticMeshSlot capacity overflow: primitives exceed 32.";
            return false;
        }
        if (materials_.size() + asset.materials.size() > kRtMaterialCapacity)
        {
            diagnostic = "RtStaticMeshSlot capacity overflow: materials exceed 32.";
            return false;
        }
        if (vertices_.size() + asset.vertices.size() > std::numeric_limits<std::uint32_t>::max() ||
            indices_.size() + asset.indices.size() > std::numeric_limits<std::uint32_t>::max())
        {
            diagnostic = "RtStaticMeshSlot geometry exceeds 32-bit addressable offsets.";
            return false;
        }
        const std::uint32_t vertexBase = static_cast<std::uint32_t>(vertices_.size());
        const std::uint32_t indexBase = static_cast<std::uint32_t>(indices_.size());
        const std::uint32_t materialBase = static_cast<std::uint32_t>(materials_.size());
        const std::uint32_t primitiveBase = static_cast<std::uint32_t>(primitiveMetadata_.size());
        vertices_.insert(vertices_.end(), asset.vertices.begin(), asset.vertices.end());
        indices_.insert(indices_.end(), asset.indices.begin(), asset.indices.end());
        std::array<std::unordered_map<std::int32_t, std::uint32_t>, 4u> textureRoutes;
        std::array<std::uint32_t, 4u> playerSharedLayers{};
        bool playerSharedLayersInitialised = false;
        const auto routeTexture = [&textureRoutes, &nextTextureLayers, &diagnostic](
            std::size_t category,
            std::int32_t sourceTexture,
            const char* categoryName,
            std::uint32_t& layer) {
            if (sourceTexture < 0)
            {
                layer = 0u;
                return true;
            }
            auto [entry, inserted] = textureRoutes[category].try_emplace(
                sourceTexture, nextTextureLayers[category]);
            if (inserted)
            {
                if (nextTextureLayers[category] >= kRtTextureLayerCapacity)
                {
                    diagnostic = std::string("RtStaticMeshSlot capacity overflow: ") +
                                 categoryName + " texture layers exceed 16.";
                    return false;
                }
                ++nextTextureLayers[category];
            }
            layer = entry->second;
            return true;
        };
        for (const auto& sourceMaterial : asset.materials)
        {
            std::array<std::uint32_t, 4u> layers{};
            const bool playerVisibilityMaterial =
                sourceMaterial.name == "BodyPrimaryVisible" ||
                sourceMaterial.name == "HeadPrimaryMasked" ||
                sourceMaterial.name == "NearFacePrimaryMasked";
            if (playerVisibilityMaterial && playerSharedLayersInitialised)
            {
                // The audited runtime player duplicates material records only
                // to carry primary-ray visibility semantics; all three records
                // intentionally reference the same PBR image payloads.
                layers = playerSharedLayers;
            }
            else
            {
                if (!routeTexture(0u, sourceMaterial.baseColorTexture, "baseColor", layers[0]) ||
                    !routeTexture(1u, sourceMaterial.normalTexture, "normal", layers[1]) ||
                    !routeTexture(2u, sourceMaterial.ormTexture, "ORM", layers[2]) ||
                    !routeTexture(3u, sourceMaterial.emissiveTexture, "emissive", layers[3]))
                    return false;
                if (playerVisibilityMaterial)
                {
                    playerSharedLayers = layers;
                    playerSharedLayersInitialised = true;
                }
            }
            materials_.push_back(ConvertMaterial(sourceMaterial, layers));
        }
        for (const auto& primitive : asset.primitives)
        {
            if (primitive.materialIndex >= asset.materials.size())
            {
                diagnostic = "RtStaticMeshSlot primitive references an out-of-range material.";
                return false;
            }
            primitiveMetadata_.push_back({
                vertexBase + primitive.vertexOffset,
                indexBase + primitive.indexOffset,
                primitive.indexCount,
                materialBase + primitive.materialIndex});
            if (primitive.nodeTransformIndex >= asset.nodeTransforms.size())
            {
                diagnostic = "RtStaticMeshSlot primitive references an out-of-range node transform.";
                return false;
            }
            geometryTransforms_.push_back({{
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f}});
        }
        routes.emplace(&asset, AssetRoute{
            static_cast<std::uint32_t>(assetIndex), primitiveBase,
            static_cast<std::uint32_t>(asset.primitives.size())});
    }

    for (const StaticRtAssetRegistration& registration : registrations)
    {
        const AssetRoute route = routes.at(registration.asset);
        instanceMetadata_[registration.instanceCustomIndex] = {
            route.primitiveBase, route.primitiveCount,
            registration.stableObjectId, registration.flags,
            registration.emitterIndex, route.assetIndex, 0u, 0u};
    }
    textureArrayCounts_ = {
        nextTextureLayers[0], nextTextureLayers[1],
        nextTextureLayers[2], nextTextureLayers[3]};
    measurements_.vertexBytes = vertices_.size() * sizeof(horde::scene::assets::StaticRtVertex);
    measurements_.indexBytes = indices_.size() * sizeof(std::uint32_t);
    measurements_.materialBytes = materials_.size() * sizeof(RtMaterialGpu);
    measurements_.instanceMetadataBytes = instanceMetadata_.size() * sizeof(RtInstanceMetadata);
    measurements_.primitiveMetadataBytes = primitiveMetadata_.size() * sizeof(RtPrimitiveMetadata);
    measurements_.descriptorCount = 9u;
    diagnostic.clear();
    return true;
}

} // namespace horde::vulkan::raytracing
