#pragma once
#include "vulkan/raytracing/RtSceneAbi.generated.h"
#include "world/streaming/RegionStreaming.h"
#include <array>
#include <span>
#include <string>
#include <vector>

namespace horde::world::streaming {
using horde::vulkan::raytracing::StaticRtVertex;
using horde::vulkan::raytracing::RtPrimitiveMetadata;
using horde::vulkan::raytracing::RtInstanceMetadata;
using horde::vulkan::raytracing::RtMaterialGpu;
struct GeometryPartitionSource {
    std::span<const StaticRtVertex> vertices;
    std::span<const std::uint32_t> indices, vertexCounts;
    std::span<const RtPrimitiveMetadata> primitives;
    std::span<const RtInstanceMetadata> instances;
    std::span<const RtMaterialGpu> materials;
    std::span<const std::array<float,12>> geometryTransforms;
};
struct GeometryPayload {
    std::vector<StaticRtVertex> vertices;
    std::vector<std::uint32_t> indices, vertexCounts, sourceTriangles;
    std::vector<RtPrimitiveMetadata> primitives;
    Bounds bounds;
    std::uint64_t GeometryBytes() const noexcept {
        return vertices.size()*sizeof(StaticRtVertex)+indices.size()*sizeof(std::uint32_t)+
               primitives.size()*sizeof(RtPrimitiveMetadata);
    }
    std::uint64_t StorageBytes() const noexcept {
        return sizeof(*this)+vertices.capacity()*sizeof(StaticRtVertex)+
            (indices.capacity()+vertexCounts.capacity()+sourceTriangles.capacity())*sizeof(std::uint32_t)+
            primitives.capacity()*sizeof(RtPrimitiveMetadata);
    }
};
struct GeometryPartition { GeometryPayload base, detail; };
// CPU preparation for world-baked static geometry. The exclusive owner selection
// is source-global sorted triangle IDs. Retained primitive identities/materials
// stay unchanged; actor/player domains outside it preserve their entire layouts.
// No admission based on a region name, camera visibility, or material label.
// Allowance covers new result capacity, provenance and worst remap scratch.
// Input and previously admitted output remain caller-owned/accounted separately.
// Caller owns immutable input lifetime; run on the loader/startup, not RenderFrame.
// This does not establish query irrelevance, collision or GPU readiness.
bool PartitionGeometry(const GeometryPartitionSource&,std::uint32_t instance,
    std::span<const std::uint32_t> detailTriangles,std::uint64_t outputByteLimit,
    GeometryPartition& output,std::string& diagnostic);
}
