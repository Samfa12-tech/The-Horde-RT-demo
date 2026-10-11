#pragma once

#include "scene/assets/StaticMeshAsset.h"
#include "vulkan/raytracing/RtSceneAbi.generated.h"
#include "vulkan/raytracing/RtTextureArrays.h"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace horde::vulkan::raytracing
{

struct StaticRtAssetRegistration
{
    std::uint32_t instanceCustomIndex = 0u;
    std::uint32_t stableObjectId = 0u;
    std::uint32_t flags = 0u;
    std::uint32_t emitterIndex = 0u;
    const horde::scene::assets::StaticMeshAsset* asset = nullptr;
    // Optional alias source. When set, it must be an earlier, self-owning
    // registration; only its named texture groups may be reused. Geometry and
    // material factors remain asset-local.
    const horde::scene::assets::StaticMeshAsset* textureSource = nullptr;
    RtGeometryRole geometryRole = RtGeometryRole::Static;
    // Optional asset-local primitive subrange for independent immutable BLAS
    // owners. Zero/zero preserves the complete asset route. Packing still
    // owns the asset once; a range does not duplicate geometry or textures.
    std::uint32_t primitiveOffset = 0u;
    std::uint32_t primitiveCount = 0u;
};

struct RtImmutableMaterialAlias
{
    std::uint32_t consumer = 0u;
    std::uint32_t provider = 0u;
};

struct RtStaticMeshMeasurements
{
    std::uint64_t vertexBytes = 0u;
    std::uint64_t indexBytes = 0u;
    std::uint64_t materialBytes = 0u;
    std::uint64_t instanceMetadataBytes = 0u;
    std::uint64_t primitiveMetadataBytes = 0u;
    std::uint32_t descriptorCount = 0u;
};

class RtStaticMeshSlot
{
public:
    bool Initialize(std::span<const StaticRtAssetRegistration> registrations,
                    std::string& diagnostic);

    // Opt-in candidate packing for zone resource replacement. Packs only the
    // union of registered immutable primitive ranges, once per source asset.
    // Material/texture identities and dynamic geometry stay unchanged. This is
    // CPU preparation, not GPU residency or a streaming completion signal.
    // Failure leaves the previously admitted slot intact.
    bool InitializeSelectedGeometry(std::span<const StaticRtAssetRegistration> registrations,
                                    std::string& diagnostic);

    // CPU admission only, before GPU upload. Caller explicitly warrants these
    // rows are immutable; mutable fixture materials must never be nominated.
    // Requires exact converted GPU records, and atomically remaps every primitive.
    bool ShareImmutableMaterialRows(std::span<const RtImmutableMaterialAlias> aliases,
                                    std::string& diagnostic);

    const std::array<RtInstanceMetadata, kRtInstanceMetadataCapacity>& InstanceMetadata() const
    {
        return instanceMetadata_;
    }
    const std::vector<RtPrimitiveMetadata>& PrimitiveMetadata() const { return primitiveMetadata_; }
    const std::vector<std::uint32_t>& PrimitiveVertexCounts() const
    {
        return primitiveVertexCounts_;
    }
    const std::vector<RtMaterialGpu>& Materials() const { return materials_; }
    const std::vector<horde::scene::assets::StaticRtVertex>& Vertices() const { return vertices_; }
    const std::vector<horde::scene::assets::StaticRtVertex>& Vertices(RtGeometryRole role) const
    {
        switch (role)
        {
        case RtGeometryRole::Static: return vertices_;
        case RtGeometryRole::PlayerWorldBody: return worldBodyVertices_;
        case RtGeometryRole::PlayerViewmodel: return viewmodelVertices_;
        default: break;
        }
        static const std::vector<horde::scene::assets::StaticRtVertex> empty;
        return empty;
    }
    const std::vector<std::uint32_t>& Indices() const { return indices_; }
    const std::vector<std::array<float, 12u>>& GeometryTransforms() const
    {
        return geometryTransforms_;
    }
    const RtTextureArrayCounts& TextureArrayCounts() const { return textureArrayCounts_; }
    const RtStaticMeshMeasurements& Measurements() const { return measurements_; }

private:
    std::array<RtInstanceMetadata, kRtInstanceMetadataCapacity> instanceMetadata_{};
    std::vector<RtPrimitiveMetadata> primitiveMetadata_;
    std::vector<std::uint32_t> primitiveVertexCounts_;
    std::vector<RtMaterialGpu> materials_;
    std::vector<horde::scene::assets::StaticRtVertex> vertices_;
    std::vector<horde::scene::assets::StaticRtVertex> worldBodyVertices_;
    std::vector<horde::scene::assets::StaticRtVertex> viewmodelVertices_;
    std::vector<std::uint32_t> indices_;
    std::vector<std::array<float, 12u>> geometryTransforms_;
    RtTextureArrayCounts textureArrayCounts_{};
    RtStaticMeshMeasurements measurements_{};
};

} // namespace horde::vulkan::raytracing
