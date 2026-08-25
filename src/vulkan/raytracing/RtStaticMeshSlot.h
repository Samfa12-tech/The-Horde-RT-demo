#pragma once

#include "scene/assets/StaticMeshAsset.h"
#include "vulkan/raytracing/RtSceneAbi.generated.h"

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

    const std::array<RtInstanceMetadata, kRtInstanceMetadataCapacity>& InstanceMetadata() const
    {
        return instanceMetadata_;
    }
    const std::vector<RtPrimitiveMetadata>& PrimitiveMetadata() const { return primitiveMetadata_; }
    const std::vector<RtMaterialGpu>& Materials() const { return materials_; }
    const std::vector<horde::scene::assets::StaticRtVertex>& Vertices() const { return vertices_; }
    const std::vector<std::uint32_t>& Indices() const { return indices_; }
    const std::vector<std::array<float, 12u>>& GeometryTransforms() const
    {
        return geometryTransforms_;
    }
    const RtStaticMeshMeasurements& Measurements() const { return measurements_; }

private:
    std::array<RtInstanceMetadata, kRtInstanceMetadataCapacity> instanceMetadata_{};
    std::vector<RtPrimitiveMetadata> primitiveMetadata_;
    std::vector<RtMaterialGpu> materials_;
    std::vector<horde::scene::assets::StaticRtVertex> vertices_;
    std::vector<std::uint32_t> indices_;
    std::vector<std::array<float, 12u>> geometryTransforms_;
    RtStaticMeshMeasurements measurements_{};
};

} // namespace horde::vulkan::raytracing
