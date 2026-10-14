#pragma once

#include "scene/assets/StaticMeshAsset.h"
#include "vulkan/raytracing/RtSceneAbi.generated.h"
#include "vulkan/raytracing/RtTextureArrays.h"

#include <array>
#include <cstdint>
#include <memory>
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
    // Candidate preparation may select disconnected source-local families for
    // ONE BLAS owner without consuming another instance/asset/primitive row.
    // Borrowed only during InitializeSelectedGeometry; strictly increasing,
    // mutually exclusive with the contiguous range, immutable StaticPbr only.
    // The legacy packer rejects this rather than silently ignoring selection.
    std::span<const std::uint32_t> primitiveSelection{};
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
    const std::vector<horde::scene::assets::StaticRtVertex>& Vertices() const { return Vertices(RtGeometryRole::Static); }
    const std::vector<horde::scene::assets::StaticRtVertex>& Vertices(RtGeometryRole role) const
    {
        if (vertexSource_) return vertexSource_->Vertices(role);
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
    const std::vector<horde::scene::assets::StaticTriangleRegion>& TriangleRegions() const { return triangleRegions_; }
    const std::vector<std::array<float, 12u>>& GeometryTransforms() const
    {
        return geometryTransforms_;
    }
    const RtTextureArrayCounts& TextureArrayCounts() const { return textureArrayCounts_; }
    const RtStaticMeshMeasurements& Measurements() const { return measurements_; }
    std::uint64_t PayloadBytes() const noexcept;
    // An omission certificate must enumerate every possible long-continuation
    // owner. Material flags are checked even if transmission is currently zero.
    bool TransmissionOwnersWithin(std::span<const std::uint32_t> allowed) const noexcept;
    // Exact immutable vertex identity, not merely equal sizes or asset labels.
    bool SharesVertexPayloadWith(const RtStaticMeshSlot& other) const noexcept
    {
        return (vertexSource_ ? vertexSource_.get() : this) ==
            (other.vertexSource_ ? other.vertexSource_.get() : &other);
    }

private:
    friend class RtRegionGeometrySource;
    friend class RtStaticMeshCopy;
    friend class RtStaticTriangleOmissions;
    bool admitted_ = false;
    std::shared_ptr<const RtStaticMeshSlot> vertexSource_;
    std::array<RtInstanceMetadata, kRtInstanceMetadataCapacity> instanceMetadata_{};
    std::vector<RtPrimitiveMetadata> primitiveMetadata_;
    std::vector<std::uint32_t> primitiveVertexCounts_;
    std::vector<RtMaterialGpu> materials_;
    std::vector<horde::scene::assets::StaticRtVertex> vertices_;
    std::vector<horde::scene::assets::StaticRtVertex> worldBodyVertices_;
    std::vector<horde::scene::assets::StaticRtVertex> viewmodelVertices_;
    std::vector<std::uint32_t> indices_;
    std::vector<horde::scene::assets::StaticTriangleRegion> triangleRegions_; // CPU only.
    std::vector<std::array<float, 12u>> geometryTransforms_;
    RtTextureArrayCounts textureArrayCounts_{};
    RtStaticMeshMeasurements measurements_{};
};

// Immutable CPU selection bound to one admitted source and one exclusive BLAS
// owner. IDs are global source triangle indices, strictly increasing. Shared
// triangles and empty primitive rows are forbidden. This is NOT a visibility
// certificate: callers must separately prove every omission irrelevant to the
// destination's primary, secondary and light queries before normal adoption.
// Creation validates/copies the list; that admission cost is outside Advance's
// byte budget. As with RtStaticMeshCopy, the source must have no mutable aliases.
class RtStaticTriangleOmissions
{
public:
    static std::shared_ptr<const RtStaticTriangleOmissions> Create(
        std::shared_ptr<const RtStaticMeshSlot> source, std::uint32_t selectedInstance,
        std::span<const std::uint32_t> triangles, std::string& diagnostic);
    std::span<const std::uint32_t> Triangles() const noexcept { return triangles_; }
private:
    friend class RtStaticMeshCopy;
    RtStaticTriangleOmissions() = default;
    std::shared_ptr<const RtStaticMeshSlot> source_;
    std::uint32_t selectedInstance_ = UINT32_MAX;
    std::vector<std::uint32_t> triangles_;
};

// Copies an already admitted immutable source across application turns. This
// pins CPU source data only; no Vulkan work or zone-readiness claim occurs.
// Caller must own an immutable source: no mutable aliases may write to it
// throughout the copy. A const shared pointer alone does not enforce this.
// Each turn copies at most maxBytes of vector payload, in complete records.
// Reserve/allocator latency is measured separately from the byte guarantee.
// Optional owner selection then scans at most maxBytes per turn and compacts
// indices only. Vertex bytes stay exact, including now-unreferenced vertices;
// this trades some residency saving for bounded CPU work and stable BLAS-local
// geometry. Labels never certify an omitted triangle's ray/light irrelevance.
class RtStaticMeshCopy
{
public:
    bool Begin(std::shared_ptr<const RtStaticMeshSlot> source, std::string& diagnostic,
        std::uint32_t selectedInstance = UINT32_MAX,
        std::uint8_t retainedRegions = horde::scene::assets::kAllStaticTriangleRegions,
        std::shared_ptr<const RtStaticTriangleOmissions> omissions = {},
        bool shareImmutableVertices = false);
    bool Advance(std::size_t maxBytes, std::string& diagnostic);
    bool Ready() const noexcept { return output_ && row_ == 9u &&
        ((!omissions_ && retainedRegions_ == horde::scene::assets::kAllStaticTriangleRegions) || filtered_); }
    std::unique_ptr<RtStaticMeshSlot> Take() noexcept;
    void Cancel() noexcept;
    std::size_t LastCopiedBytes() const noexcept { return lastBytes_; }
    std::uint64_t CopiedBytes() const noexcept { return copiedBytes_; }
    std::size_t LastScannedBytes() const noexcept { return lastScannedBytes_; }
    std::uint64_t ScannedBytes() const noexcept { return scannedBytes_; }
private:
    bool FilterIndices(std::size_t maxBytes, std::string& diagnostic);
    struct Boundary { std::uint32_t index, primitive; bool end; };
    std::shared_ptr<const RtStaticMeshSlot> source_;
    std::shared_ptr<const RtStaticTriangleOmissions> omissions_;
    std::unique_ptr<RtStaticMeshSlot> output_;
    std::size_t row_ = 0u, offset_ = 0u, lastBytes_ = 0u;
    std::uint64_t copiedBytes_ = 0u;
    std::vector<Boundary> boundaries_;
    std::size_t boundary_ = 0u, readIndex_ = 0u, writeIndex_ = 0u, lastScannedBytes_ = 0u;
    std::uint64_t scannedBytes_ = 0u;
    std::uint32_t selectedBegin_ = 0u, selectedEnd_ = 0u;
    std::size_t omission_ = 0u;
    std::uint8_t retainedRegions_ = horde::scene::assets::kAllStaticTriangleRegions;
    bool filtered_ = false;
};

} // namespace horde::vulkan::raytracing
