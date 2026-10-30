#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>

namespace horde::scene {

// Geometry-only presentation for an existing, non-indexed triangle stream.
// Every point remains inside its original triangle. Counts, normals, UVs and
// source masters are unchanged; the caller refits its existing BLAS normally.
// Completed actors must be masked before their resources can be retired.
template<class Vertex, std::size_t Extent>
bool ApplyTriangleDissolve(std::span<Vertex, Extent> vertices, float progress,
                          std::uint32_t seed = 0x9e3779b9u)
{
    if (!std::isfinite(progress) || vertices.size() % 3u != 0u) return false;
    if (progress <= 0.0f) return true; // Exact unchanged legacy/live skin bytes.
    progress = std::min(progress, 1.0f);
    for (std::size_t first = 0; first < vertices.size(); first += 3u) {
        std::uint32_t hash = static_cast<std::uint32_t>(first / 3u) ^ seed;
        hash ^= hash >> 16u; hash *= 0x7feb352du;
        hash ^= hash >> 15u; hash *= 0x846ca68bu; hash ^= hash >> 16u;
        const float start = static_cast<float>(hash & 0xffffu) / 65535.0f * 0.55f;
        const float t = std::clamp((progress - start) / 0.45f, 0.0f, 1.0f);
        const float scale = 1.0f - t * t * (3.0f - 2.0f * t);
        for (std::size_t axis = 0; axis < 3u; ++axis) {
            const float centre = (vertices[first].position[axis] +
                vertices[first+1u].position[axis] + vertices[first+2u].position[axis]) / 3.0f;
            for (std::size_t corner = 0; corner < 3u; ++corner) {
                float& position = vertices[first+corner].position[axis];
                position = centre + (position - centre) * scale;
            }
        }
    }
    return true;
}

} // namespace horde::scene
