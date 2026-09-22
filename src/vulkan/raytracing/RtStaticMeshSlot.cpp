#include "vulkan/raytracing/RtStaticMeshSlot.h"

#include <algorithm>
#include <limits>
#include <map>
#include <unordered_map>
#include <utility>

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
    std::unordered_map<const horde::scene::assets::StaticMeshAsset*, std::size_t>
        firstRegistrationIndex;
    std::unordered_map<const horde::scene::assets::StaticMeshAsset*,
                       const horde::scene::assets::StaticMeshAsset*> textureSources;
    for (std::size_t registrationIndex = 0u;
         registrationIndex < registrations.size(); ++registrationIndex)
    {
        const StaticRtAssetRegistration& registration = registrations[registrationIndex];
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
        firstRegistrationIndex.emplace(registration.asset, registrationIndex);
        const auto [sourceAsset, insertedSource] = textureSources.emplace(
            registration.asset, registration.textureSource);
        if (!insertedSource && sourceAsset->second != registration.textureSource)
        {
            diagnostic = "RtStaticMeshSlot repeated registration of one asset must keep a consistent texture source.";
            return false;
        }
        if (registration.textureSource != nullptr)
        {
            const auto sourceFirst = firstRegistrationIndex.find(registration.textureSource);
            if (sourceFirst == firstRegistrationIndex.end())
            {
                const bool appearsLater = std::any_of(
                    registrations.begin() + registrationIndex + 1u,
                    registrations.end(),
                    [&registration](const StaticRtAssetRegistration& candidate) {
                        return candidate.asset == registration.textureSource;
                    });
                diagnostic = appearsLater
                    ? "RtStaticMeshSlot texture source must be registered earlier than its consumer."
                    : "RtStaticMeshSlot texture source is not registered by this slot.";
                return false;
            }
            if (sourceFirst->second >= registrationIndex)
            {
                diagnostic = "RtStaticMeshSlot texture source must be registered earlier than its consumer.";
                return false;
            }
            const auto sourceAlias = textureSources.find(registration.textureSource);
            if (sourceAlias != textureSources.end() && sourceAlias->second != nullptr)
            {
                diagnostic = "RtStaticMeshSlot texture source alias chains are not allowed; provider must self-own its groups.";
                return false;
            }
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
        std::map<std::int32_t, std::array<bool, 4u>> groupPresence;
        std::map<std::int32_t, std::array<std::uint32_t, 4u>> groupLayers;
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
        std::map<std::int32_t, std::array<bool, 4u>> groupPresence;
        std::map<std::int32_t, std::array<std::uint32_t, 4u>> groupLayers;
        const auto textureSource = textureSources.at(uniqueAssets[assetIndex]);
        if (textureSource != nullptr)
        {
            const auto sourceRoute = routes.find(textureSource);
            if (sourceRoute == routes.end())
            {
                diagnostic = "RtStaticMeshSlot texture source route was not built before its consumer.";
                return false;
            }
            groupPresence = sourceRoute->second.groupPresence;
            groupLayers = sourceRoute->second.groupLayers;
        }
        else
        {
            // Explicit asset-local groups are allocated canonically before the
            // ordinary per-texture routes. Visibility-only material duplication
            // cannot exchange body/gauntlet atlas layers by changing material order.
            for (const auto& material : asset.materials)
            {
                if (material.textureGroup < 0) continue;
                const std::array<bool, 4u> present{{material.baseColorTexture >= 0,
                    material.normalTexture >= 0, material.ormTexture >= 0, material.emissiveTexture >= 0}};
                const auto [entry, inserted] = groupPresence.try_emplace(material.textureGroup, present);
                if (!inserted && entry->second != present)
                {
                    diagnostic = "RtStaticMeshSlot texture group has conflicting texture presence.";
                    return false;
                }
            }
            constexpr std::array<const char*, 4u> categoryNames{{"baseColor", "normal", "ORM", "emissive"}};
            for (const auto& [group, present] : groupPresence)
            {
                auto& layers = groupLayers[group];
                for (std::size_t category = 0; category < layers.size(); ++category)
                {
                    if (!present[category]) continue;
                    if (nextTextureLayers[category] >= kRtTextureLayerCapacity)
                    {
                        diagnostic = std::string("RtStaticMeshSlot capacity overflow: ") +
                            categoryNames[category] + " texture layers exceed 16.";
                        return false;
                    }
                    layers[category] = nextTextureLayers[category]++;
                }
            }
        }
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
            if (textureSource != nullptr)
            {
                const std::array<bool, 4u> present{{sourceMaterial.baseColorTexture >= 0,
                    sourceMaterial.normalTexture >= 0, sourceMaterial.ormTexture >= 0,
                    sourceMaterial.emissiveTexture >= 0}};
                if (sourceMaterial.textureGroup < 0)
                {
                    if (std::any_of(present.begin(), present.end(), [](bool value) { return value; }))
                    {
                        diagnostic = "RtStaticMeshSlot aliased material with textures must declare an explicit texture group.";
                        return false;
                    }
                }
                else
                {
                    const auto presence = groupPresence.find(sourceMaterial.textureGroup);
                    const auto route = groupLayers.find(sourceMaterial.textureGroup);
                    if (presence == groupPresence.end() || route == groupLayers.end())
                    {
                        diagnostic = "RtStaticMeshSlot aliased material references a texture group absent from its provider.";
                        return false;
                    }
                    if (presence->second != present)
                    {
                        diagnostic = "RtStaticMeshSlot aliased texture group has conflicting four-category texture presence.";
                        return false;
                    }
                    layers = route->second;
                }
            }
            else if (sourceMaterial.textureGroup >= 0)
            {
                layers = groupLayers.at(sourceMaterial.textureGroup);
            }
            else
            {
                if (!routeTexture(0u, sourceMaterial.baseColorTexture, "baseColor", layers[0]) ||
                    !routeTexture(1u, sourceMaterial.normalTexture, "normal", layers[1]) ||
                    !routeTexture(2u, sourceMaterial.ormTexture, "ORM", layers[2]) ||
                    !routeTexture(3u, sourceMaterial.emissiveTexture, "emissive", layers[3]))
                    return false;
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
            static_cast<std::uint32_t>(asset.primitives.size()),
            std::move(groupPresence), std::move(groupLayers)});
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
