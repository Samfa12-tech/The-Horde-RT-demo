#pragma once

#include <vulkan/vulkan.h>
#include "vulkan/raytracing/RtGpuResources.h"
#include "vulkan/raytracing/RtStaticMeshSlot.h"

namespace horde::vulkan::raytracing
{

// A binding to the current packed owner. Re-resolve after a static buffer
// cutover. Source positions already include the imported node hierarchy.
class RtIndexedActorGeometry
{
public:
    bool Resolve(const RtStaticMeshSlot& slot, std::uint32_t metadataIndex,
                 const horde::scene::assets::StaticMeshAsset& source,
                 const RtGpuBuffer& vertices, const RtGpuBuffer& indices,
                 std::string& diagnostic)
    {
        count_ = 0;
        constexpr auto vertexStride = sizeof(horde::scene::assets::StaticRtVertex);
        const auto fail = [&](const char* reason) {
            diagnostic = reason;
            return false;
        };
        if (metadataIndex >= slot.InstanceMetadata().size() ||
            !vertices.address || !indices.address || source.vertices.empty() || source.indices.empty() ||
            source.primitives.empty())
            return fail("Indexed actor is missing its source or current GPU buffer owner.");
        const auto& instance = slot.InstanceMetadata()[metadataIndex];
        const auto& packed = slot.PrimitiveMetadata();
        const auto& vertexCounts = slot.PrimitiveVertexCounts();
        if (instance.geometryRole != static_cast<std::uint32_t>(RtGeometryRole::Static) ||
            instance.flags != static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr) ||
            instance.primitiveCount != source.primitives.size() ||
            instance.primitiveCount > geometries_.size() || instance.primitiveBase >= packed.size() ||
            instance.primitiveCount > packed.size() - instance.primitiveBase ||
            vertexCounts.size() != packed.size())
            return fail("Indexed actor requires one complete opaque static-PBR metadata registration.");

        const auto& first = packed[instance.primitiveBase];
        if (source.primitives.front().vertexOffset != 0 ||
            first.indexOffset < source.primitives.front().indexOffset)
            return fail("Indexed actor does not start at its own contiguous payload.");
        const auto vertexOffset = VkDeviceSize(first.vertexOffset) * vertexStride;
        const auto vertexBytes = VkDeviceSize(source.vertices.size()) * vertexStride;
        const auto indexBase = first.indexOffset - source.primitives.front().indexOffset;
        const auto indexOffset = VkDeviceSize(indexBase) * sizeof(std::uint32_t);
        const auto indexBytes = VkDeviceSize(source.indices.size()) * sizeof(std::uint32_t);
        if (vertexOffset > vertices.size || vertexBytes > vertices.size - vertexOffset ||
            indexOffset > indices.size || indexBytes > indices.size - indexOffset)
            return fail("Indexed actor payload exceeds its current GPU buffer.");

        for (std::uint32_t local = 0; local < instance.primitiveCount; ++local)
        {
            const auto row = instance.primitiveBase + local;
            const auto& primitive = source.primitives[local];
            const auto& destination = packed[row];
            const auto vertexEnd = local + 1 < instance.primitiveCount ?
                std::size_t(source.primitives[local + 1].vertexOffset) : source.vertices.size();
            if (vertexEnd > source.vertices.size() || vertexEnd <= primitive.vertexOffset ||
                destination.vertexOffset != std::uint64_t(first.vertexOffset) + primitive.vertexOffset ||
                vertexCounts[row] != vertexEnd - primitive.vertexOffset ||
                destination.indexOffset != std::uint64_t(indexBase) + primitive.indexOffset ||
                destination.indexCount != primitive.indexCount || !destination.indexCount ||
                destination.indexCount % 3 != 0 || primitive.indexOffset > source.indices.size() ||
                primitive.indexCount > source.indices.size() - primitive.indexOffset ||
                destination.materialIndex >= slot.Materials().size() ||
                (slot.Materials()[destination.materialIndex].materialFlags[0] &
                    static_cast<std::uint32_t>(RtMaterialFlag::Transmission)))
                return fail("Indexed actor primitive does not match its packed opaque geometry range.");

            auto& geometry = geometries_[local];
            geometry = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
            geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
            auto& triangle = geometry.geometry.triangles;
            triangle.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            triangle.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            triangle.vertexData.deviceAddress = vertices.address + VkDeviceSize(destination.vertexOffset) * vertexStride;
            triangle.vertexStride = vertexStride;
            triangle.maxVertex = vertexCounts[row] - 1;
            triangle.indexType = VK_INDEX_TYPE_UINT32;
            triangle.indexData.deviceAddress = indices.address + VkDeviceSize(destination.indexOffset) * sizeof(std::uint32_t);
            const auto triangleCount = destination.indexCount / 3;
            triangleCounts_[local] = triangleCount;
            ranges_[local] = {triangleCount, 0, 0, 0};
        }
        count_ = instance.primitiveCount;
        vertexOffset_ = vertexOffset;
        vertexBytes_ = vertexBytes;
        diagnostic.clear();
        return true;
    }

    std::span<const VkAccelerationStructureGeometryKHR> Geometries() const
    {
        return {geometries_.data(), count_};
    }

    std::span<const std::uint32_t> TriangleCounts() const
    {
        return {triangleCounts_.data(), count_};
    }

    VkDeviceSize VertexOffset() const { return count_ ? vertexOffset_ : 0; }
    VkDeviceSize VertexBytes() const { return count_ ? vertexBytes_ : 0; }

    void Record(VkCommandBuffer commandBuffer, PFN_vkCmdBuildAccelerationStructuresKHR build,
                VkAccelerationStructureBuildGeometryInfoKHR info) const
    {
        info.geometryCount = count_;
        info.pGeometries = geometries_.data();
        std::array<const VkAccelerationStructureBuildRangeInfoKHR*, kRtPrimitiveMetadataCapacity> ranges;
        for (std::uint32_t i = 0; i < count_; ++i) ranges[i] = &ranges_[i];
        build(commandBuffer, 1, &info, ranges.data());
    }

private:
    std::array<VkAccelerationStructureGeometryKHR, kRtPrimitiveMetadataCapacity> geometries_;
    std::array<VkAccelerationStructureBuildRangeInfoKHR, kRtPrimitiveMetadataCapacity> ranges_;
    std::array<std::uint32_t, kRtPrimitiveMetadataCapacity> triangleCounts_;
    std::uint32_t count_ = 0;
    VkDeviceSize vertexOffset_ = 0;
    VkDeviceSize vertexBytes_ = 0;
};

} // namespace horde::vulkan::raytracing
