// Exact ten-triangle full-face certificate; see the corridor detail admission
// report and focused coverage tests. Check actual runtime bytes before use.
// XYZ bits are original IEEE-754 binary32 values; IDs are zero-based immutable procedural triangle records.
#pragma once
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace horde::scene::corridor_detail_faces {
struct ProceduralTriangleBits {
    std::uint32_t proceduralTriangleIndex;
    std::array<std::uint32_t, 9> xyzBits;
    std::uint32_t materialId;
};
static_assert(sizeof(float) == sizeof(std::uint32_t) && std::numeric_limits<float>::is_iec559);
inline constexpr std::size_t kExpectedImmutableTriangleCount = 2036u;

inline constexpr std::array<ProceduralTriangleBits, 2> kNorthPlusZ{{
    {1602u, {0xc1f40000u, 0xbf733333u, 0xc159999au, 0xc1f40000u, 0x3faccccdu, 0xc159999au, 0xc1080000u, 0x3faccccdu, 0xc159999au}, 2u},
    {1603u, {0xc1f40000u, 0xbf733333u, 0xc159999au, 0xc1080000u, 0x3faccccdu, 0xc159999au, 0xc1080000u, 0xbf733333u, 0xc159999au}, 2u},
}};

inline constexpr std::array<ProceduralTriangleBits, 4> kRoofPlusY{{
    {1596u, {0xc1e40000u, 0x3faccccdu, 0xc159999au, 0xc1e40000u, 0x3faccccdu, 0xc1866666u, 0xc1080000u, 0x3faccccdu, 0xc1866666u}, 0u},
    {1597u, {0xc1e40000u, 0x3faccccdu, 0xc159999au, 0xc1080000u, 0x3faccccdu, 0xc1866666u, 0xc1080000u, 0x3faccccdu, 0xc159999au}, 0u},
    {1600u, {0xc1f40000u, 0x3faccccdu, 0xc159999au, 0xc1f40000u, 0x3faccccdu, 0xc1866666u, 0xc1e40000u, 0x3faccccdu, 0xc1866666u}, 0u},
    {1601u, {0xc1f40000u, 0x3faccccdu, 0xc159999au, 0xc1e40000u, 0x3faccccdu, 0xc1866666u, 0xc1e40000u, 0x3faccccdu, 0xc159999au}, 0u},
}};

inline constexpr std::array<ProceduralTriangleBits, 4> kFloorMinusY{{
    {1594u, {0xc1e40000u, 0xbf733333u, 0xc159999au, 0xc1080000u, 0xbf733333u, 0xc159999au, 0xc1080000u, 0xbf733333u, 0xc1866666u}, 1u},
    {1595u, {0xc1e40000u, 0xbf733333u, 0xc159999au, 0xc1080000u, 0xbf733333u, 0xc1866666u, 0xc1e40000u, 0xbf733333u, 0xc1866666u}, 1u},
    {1598u, {0xc1f40000u, 0xbf733333u, 0xc159999au, 0xc1e40000u, 0xbf733333u, 0xc159999au, 0xc1e40000u, 0xbf733333u, 0xc1866666u}, 0u},
    {1599u, {0xc1f40000u, 0xbf733333u, 0xc159999au, 0xc1e40000u, 0xbf733333u, 0xc1866666u, 0xc1f40000u, 0xbf733333u, 0xc1866666u}, 0u},
}};

template <class Vertex, std::size_t N>
inline bool Matches(const std::array<ProceduralTriangleBits, N>& certified,
                   std::span<const Vertex> vertices,
                   std::span<const std::uint32_t> indices,
                   std::span<const std::uint32_t> worldSurfaceCodes,
                   std::size_t immutableTriangleCount,
                   std::size_t immutableVertexCount,
                   bool actualBlockerBlasGeometryOpaque) noexcept {
    // Pass true only when this exact static-world geometry is in the traced BLAS/TLAS and its geometry flags contain VK_GEOMETRY_OPAQUE_BIT_KHR.
    if (!actualBlockerBlasGeometryOpaque || immutableTriangleCount != kExpectedImmutableTriangleCount) return false;
    for (const auto& item : certified) {
        const std::size_t tri = item.proceduralTriangleIndex;
        if (tri >= immutableTriangleCount || tri >= worldSurfaceCodes.size() ||
            tri >= indices.size() / 3u) return false;
        if ((worldSurfaceCodes[tri] & 0xffu) != item.materialId) return false;
        for (std::size_t corner = 0; corner < 3u; ++corner) {
            const std::uint32_t vertex = indices[tri * 3u + corner];
            if (vertex >= immutableVertexCount || vertex >= vertices.size()) return false;
            for (std::size_t axis = 0; axis < 3u; ++axis)
                if (std::bit_cast<std::uint32_t>(vertices[vertex].position[axis]) != item.xyzBits[corner * 3u + axis]) return false;
        }
    }
    return true;
}

template <class Vertex>
inline bool MatchesAll(std::span<const Vertex> vertices,
                       std::span<const std::uint32_t> indices,
                       std::span<const std::uint32_t> worldSurfaceCodes,
                       std::size_t immutableTriangleCount,
                       std::size_t immutableVertexCount,
                       bool actualBlockerBlasGeometryOpaque) noexcept {
    return Matches(kNorthPlusZ, vertices, indices, worldSurfaceCodes, immutableTriangleCount, immutableVertexCount, actualBlockerBlasGeometryOpaque) &&
           Matches(kRoofPlusY, vertices, indices, worldSurfaceCodes, immutableTriangleCount, immutableVertexCount, actualBlockerBlasGeometryOpaque) &&
           Matches(kFloorMinusY, vertices, indices, worldSurfaceCodes, immutableTriangleCount, immutableVertexCount, actualBlockerBlasGeometryOpaque);
}
} // namespace horde::scene::corridor_detail_faces
