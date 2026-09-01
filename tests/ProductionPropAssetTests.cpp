#include "scene/assets/AssetManifest.h"
#include "scene/assets/StaticMeshAsset.h"
#include "gameplay/DevelopmentCheckpoints.h"
#include "gameplay/ShowcaseRoute.h"
#include "gameplay/items/HeldItemKinematics.h"
#include "gameplay/items/LanternPendulum.h"
#include "vulkan/raytracing/RtSceneRouteConstants.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <array>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace
{

using horde::scene::assets::AssetManifest;
using horde::scene::assets::StaticMeshAsset;

int failures = 0;
const std::filesystem::path kRoot{HORDE_RT_PRODUCTION_PROP_ROOT};

void Check(bool condition, std::string_view message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

struct LoadedAsset
{
    AssetManifest manifest;
    StaticMeshAsset asset;
};

LoadedAsset Load(std::string_view directory, std::string_view file)
{
    LoadedAsset result;
    std::string diagnostic;
    const auto assetDirectory = kRoot / directory;
    Check(AssetManifest::Load(assetDirectory / "asset.manifest.json",
                              result.manifest, diagnostic),
          std::string(directory) + " manifest loads: " + diagnostic);
    if (result.manifest.schema != 0u)
    {
        diagnostic.clear();
        const bool loaded = StaticMeshAsset::Load(assetDirectory / file, result.manifest,
                                                  result.asset, diagnostic);
        Check(loaded,
              std::string(directory) + " runtime GLB loads: " + diagnostic);
    }
    return result;
}

std::uint64_t Triangles(const StaticMeshAsset& asset)
{
    std::uint64_t count = 0u;
    for (const auto& primitive : asset.primitives) count += primitive.indexCount / 3u;
    return count;
}

bool HasNode(const StaticMeshAsset& asset, std::string_view name)
{
    return std::any_of(asset.nodeTransforms.begin(), asset.nodeTransforms.end(),
        [name](const auto& node) { return node.name == name; });
}

bool HasMaterial(const StaticMeshAsset& asset, std::string_view name)
{
    return std::any_of(asset.materials.begin(), asset.materials.end(),
        [name](const auto& material) { return material.name == name; });
}

const horde::scene::assets::StaticSocket* Socket(const StaticMeshAsset& asset,
                                                 std::string_view name)
{
    const auto socket = std::find_if(asset.sockets.begin(), asset.sockets.end(),
        [name](const auto& candidate) { return candidate.name == name; });
    return socket == asset.sockets.end() ? nullptr : &*socket;
}

const horde::scene::assets::StaticNodeTransform* Node(const StaticMeshAsset& asset,
                                                       std::string_view name)
{
    const auto node = std::find_if(asset.nodeTransforms.begin(), asset.nodeTransforms.end(),
        [name](const auto& candidate) { return candidate.name == name; });
    return node == asset.nodeTransforms.end() ? nullptr : &*node;
}

bool Near(float left, float right, float tolerance = 0.00001f)
{
    return std::abs(left - right) <= tolerance;
}

bool IsSevenMillimetrePaneThickness(float thickness)
{
    return std::abs(thickness - 0.007f) <= 0.0005f;
}

StaticMeshAsset MakeClosedPaneFixture(float thickness, bool openFace, bool malformed)
{
    StaticMeshAsset asset;
    asset.materials.push_back({.name = "LanternGlass"});
    constexpr std::array<std::array<float, 3u>, 8u> unitVertices{{
        {{-0.10f, -0.08f, 0.0f}}, {{0.10f, -0.08f, 0.0f}},
        {{0.10f, 0.08f, 0.0f}}, {{-0.10f, 0.08f, 0.0f}},
        {{-0.10f, -0.08f, 1.0f}}, {{0.10f, -0.08f, 1.0f}},
        {{0.10f, 0.08f, 1.0f}}, {{-0.10f, 0.08f, 1.0f}}}};
    for (const auto& source : unitVertices)
    {
        horde::scene::assets::StaticRtVertex vertex{};
        vertex.position = {{source[0], source[1], source[2] * thickness, 1.0f}};
        asset.vertices.push_back(vertex);
    }
    // Outward-wound production-like cuboid. Each face is two triangles.
    constexpr std::array<std::uint32_t, 36u> indices{{
        0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7,
        0, 1, 5, 0, 5, 4, 1, 2, 6, 1, 6, 5,
        2, 3, 7, 2, 7, 6, 3, 0, 4, 3, 4, 7}};
    asset.indices.assign(indices.begin(), indices.end());
    if (openFace) asset.indices.resize(asset.indices.size() - 6u);
    if (malformed) asset.indices.insert(asset.indices.end(), {0u, 2u, 1u});
    asset.primitives.push_back({0u, 0u, static_cast<std::uint32_t>(asset.indices.size()), 0u, 0u});
    return asset;
}

StaticMeshAsset ReversePaneWinding(StaticMeshAsset asset)
{
    for (std::size_t offset = 0u; offset + 2u < asset.indices.size(); offset += 3u)
        std::swap(asset.indices[offset + 1u], asset.indices[offset + 2u]);
    return asset;
}

StaticMeshAsset FlipFirstPaneTriangle(StaticMeshAsset asset)
{
    if (asset.indices.size() >= 3u)
        std::swap(asset.indices[1u], asset.indices[2u]);
    return asset;
}

std::uint64_t TrianglesForMaterial(const StaticMeshAsset& asset, std::string_view name)
{
    const auto material = std::find_if(asset.materials.begin(), asset.materials.end(),
        [name](const auto& candidate) { return candidate.name == name; });
    if (material == asset.materials.end()) return 0u;
    const std::size_t materialIndex = static_cast<std::size_t>(material - asset.materials.begin());
    std::uint64_t count = 0u;
    for (const auto& primitive : asset.primitives)
    {
        if (primitive.materialIndex == materialIndex) count += primitive.indexCount / 3u;
    }
    return count;
}

float MinimumCageGlassRadialClearance(const StaticMeshAsset& asset)
{
    const auto glassMaterial = std::find_if(asset.materials.begin(), asset.materials.end(),
        [](const auto& material) { return material.name == "LanternGlass"; });
    const auto cageMaterial = std::find_if(asset.materials.begin(), asset.materials.end(),
        [](const auto& material) { return material.name == "BlackIron"; });
    if (glassMaterial == asset.materials.end() || cageMaterial == asset.materials.end())
        return -INFINITY;
    const std::size_t glassIndex = static_cast<std::size_t>(glassMaterial - asset.materials.begin());
    const std::size_t cageIndex = static_cast<std::size_t>(cageMaterial - asset.materials.begin());
    std::vector<std::array<float, 3u>> glassVertices;
    std::vector<std::array<float, 3u>> cageVertices;
    for (const auto& primitive : asset.primitives)
    {
        auto* destination = primitive.materialIndex == glassIndex ? &glassVertices :
            (primitive.materialIndex == cageIndex ? &cageVertices : nullptr);
        if (destination == nullptr) continue;
        for (std::uint32_t offset = 0u; offset < primitive.indexCount; ++offset)
        {
            const auto& vertex = asset.vertices[
                asset.indices[primitive.indexOffset + offset] + primitive.vertexOffset];
            destination->push_back({{vertex.position[0], vertex.position[1], vertex.position[2]}});
        }
    }
    if (glassVertices.empty() || cageVertices.empty()) return -INFINITY;
    float glassMinimumY = INFINITY;
    float glassMaximumY = -INFINITY;
    for (const auto& vertex : glassVertices)
    {
        glassMinimumY = std::min(glassMinimumY, vertex[1]);
        glassMaximumY = std::max(glassMaximumY, vertex[1]);
    }
    float minimumClearance = INFINITY;
    constexpr float kPi = 3.14159265358979323846f;
    for (int face = 0; face < 6; ++face)
    {
        const float angle = static_cast<float>(face) * kPi / 3.0f;
        const float normalX = std::cos(angle);
        const float normalZ = std::sin(angle);
        const float tangentX = -normalZ;
        const float tangentZ = normalX;
        float glassOuter = -INFINITY;
        for (const auto& vertex : glassVertices)
            glassOuter = std::max(glassOuter, vertex[0] * normalX + vertex[2] * normalZ);
        float cageInner = INFINITY;
        for (const auto& vertex : cageVertices)
        {
            if (vertex[1] < glassMinimumY || vertex[1] > glassMaximumY) continue;
            const float radial = vertex[0] * normalX + vertex[2] * normalZ;
            const float tangent = vertex[0] * tangentX + vertex[2] * tangentZ;
            if (radial > 0.15f && std::abs(tangent) <= 0.10f)
                cageInner = std::min(cageInner, radial);
        }
        minimumClearance = std::min(minimumClearance, cageInner - glassOuter);
    }
    return minimumClearance;
}

float MaximumGlassPaneTangentialHalfWidth(const StaticMeshAsset& asset)
{
    const auto glassMaterial = std::find_if(asset.materials.begin(), asset.materials.end(),
        [](const auto& material) { return material.name == "LanternGlass"; });
    if (glassMaterial == asset.materials.end()) return INFINITY;
    const std::size_t glassIndex = static_cast<std::size_t>(
        glassMaterial - asset.materials.begin());
    float maximumHalfWidth = 0.0f;
    constexpr float kPi = 3.14159265358979323846f;
    for (const auto& primitive : asset.primitives)
    {
        if (primitive.materialIndex != glassIndex) continue;
        for (std::uint32_t offset = 0u; offset < primitive.indexCount; ++offset)
        {
            const auto& vertex = asset.vertices[
                asset.indices[primitive.indexOffset + offset] + primitive.vertexOffset];
            float bestRadial = -INFINITY;
            float tangentAtBestRadial = 0.0f;
            for (int face = 0; face < 6; ++face)
            {
                const float angle = static_cast<float>(face) * kPi / 3.0f;
                const float normalX = std::cos(angle);
                const float normalZ = std::sin(angle);
                const float radial = vertex.position[0] * normalX +
                    vertex.position[2] * normalZ;
                if (radial > bestRadial)
                {
                    bestRadial = radial;
                    tangentAtBestRadial = vertex.position[0] * -normalZ +
                        vertex.position[2] * normalX;
                }
            }
            maximumHalfWidth = std::max(maximumHalfWidth,
                                        std::abs(tangentAtBestRadial));
        }
    }
    return maximumHalfWidth;
}

struct PaneTopology
{
    std::size_t components = 0u;
    bool manifoldOutward = false;
    bool sevenMillimetres = false;
};

using Matrix = std::array<float, 16u>;

struct TransformedBounds
{
    std::array<float, 3u> minimum{{INFINITY, INFINITY, INFINITY}};
    std::array<float, 3u> maximum{{-INFINITY, -INFINITY, -INFINITY}};
};

Matrix Translation(float x, float y, float z)
{
    return {{1.0f, 0.0f, 0.0f, 0.0f,
             0.0f, 1.0f, 0.0f, 0.0f,
             0.0f, 0.0f, 1.0f, 0.0f,
             x, y, z, 1.0f}};
}

bool MatrixNear(const Matrix& actual, const Matrix& expected,
                float tolerance = 0.00001f)
{
    for (std::size_t index = 0u; index < actual.size(); ++index)
        if (!Near(actual[index], expected[index], tolerance)) return false;
    return true;
}

Matrix Multiply(const Matrix& left, const Matrix& right)
{
    Matrix result{};
    for (std::size_t column = 0u; column < 4u; ++column)
        for (std::size_t row = 0u; row < 4u; ++row)
            for (std::size_t inner = 0u; inner < 4u; ++inner)
                result[column * 4u + row] += left[inner * 4u + row] * right[column * 4u + inner];
    return result;
}

std::array<float, 3u> Origin(const Matrix& matrix)
{
    return {{matrix[12], matrix[13], matrix[14]}};
}

TransformedBounds TransformBounds(const StaticMeshAsset& asset, const Matrix& matrix)
{
    TransformedBounds result;
    for (std::uint32_t corner = 0u; corner < 8u; ++corner)
    {
        const float x = (corner & 1u) != 0u
            ? asset.bounds.maximum[0] : asset.bounds.minimum[0];
        const float y = (corner & 2u) != 0u
            ? asset.bounds.maximum[1] : asset.bounds.minimum[1];
        const float z = (corner & 4u) != 0u
            ? asset.bounds.maximum[2] : asset.bounds.minimum[2];
        const std::array<float, 3u> point{{
            matrix[0] * x + matrix[4] * y + matrix[8] * z + matrix[12],
            matrix[1] * x + matrix[5] * y + matrix[9] * z + matrix[13],
            matrix[2] * x + matrix[6] * y + matrix[10] * z + matrix[14]}};
        for (std::size_t axis = 0u; axis < 3u; ++axis)
        {
            result.minimum[axis] = std::min(result.minimum[axis], point[axis]);
            result.maximum[axis] = std::max(result.maximum[axis], point[axis]);
        }
    }
    return result;
}

std::array<float, 3u> TransformPosition(
    const Matrix& matrix, const std::array<float, 4u>& position)
{
    return {{
        matrix[0] * position[0] + matrix[4] * position[1] +
            matrix[8] * position[2] + matrix[12],
        matrix[1] * position[0] + matrix[5] * position[1] +
            matrix[9] * position[2] + matrix[13],
        matrix[2] * position[0] + matrix[6] * position[1] +
            matrix[10] * position[2] + matrix[14]}};
}

float PointTriangleDistance(
    const std::array<float, 3u>& point,
    const std::array<float, 3u>& a,
    const std::array<float, 3u>& b,
    const std::array<float, 3u>& c)
{
    const auto subtract = [](const auto& left, const auto& right) {
        return std::array<float, 3u>{{left[0] - right[0],
                                      left[1] - right[1],
                                      left[2] - right[2]}};
    };
    const auto addScaled = [](const auto& origin, const auto& direction,
                              const float scale) {
        return std::array<float, 3u>{{origin[0] + direction[0] * scale,
                                      origin[1] + direction[1] * scale,
                                      origin[2] + direction[2] * scale}};
    };
    const auto dot = [](const auto& left, const auto& right) {
        return left[0] * right[0] + left[1] * right[1] +
            left[2] * right[2];
    };
    const auto distance = [&](const auto& candidate) {
        const auto delta = subtract(point, candidate);
        return std::sqrt(dot(delta, delta));
    };
    const auto ab = subtract(b, a);
    const auto ac = subtract(c, a);
    const auto ap = subtract(point, a);
    const float d1 = dot(ab, ap);
    const float d2 = dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return distance(a);
    const auto bp = subtract(point, b);
    const float d3 = dot(ab, bp);
    const float d4 = dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return distance(b);
    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
        return distance(addScaled(a, ab, d1 / (d1 - d3)));
    const auto cp = subtract(point, c);
    const float d5 = dot(ab, cp);
    const float d6 = dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return distance(c);
    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
        return distance(addScaled(a, ac, d2 / (d2 - d6)));
    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && d4 - d3 >= 0.0f && d5 - d6 >= 0.0f)
    {
        const auto bc = subtract(c, b);
        return distance(addScaled(b, bc, (d4 - d3) /
            ((d4 - d3) + (d5 - d6))));
    }
    const float denominator = 1.0f / (va + vb + vc);
    const float v = vb * denominator;
    const float w = vc * denominator;
    return distance(addScaled(addScaled(a, ab, v), ac, w));
}

float MinimumTriangleDistance(
    const StaticMeshAsset& asset, const Matrix& matrix,
    const std::array<float, 3u>& point,
    const std::string_view materialName = {})
{
    float result = INFINITY;
    for (const auto& primitive : asset.primitives)
    {
        if (!materialName.empty() &&
            (primitive.materialIndex >= asset.materials.size() ||
             asset.materials[primitive.materialIndex].name != materialName))
        {
            continue;
        }
        for (std::uint32_t offset = 0u;
             offset + 2u < primitive.indexCount; offset += 3u)
        {
            std::array<std::array<float, 3u>, 3u> triangle{};
            for (std::uint32_t corner = 0u; corner < 3u; ++corner)
            {
                const std::uint32_t vertexIndex =
                    asset.indices[primitive.indexOffset + offset + corner] +
                    primitive.vertexOffset;
                triangle[corner] = TransformPosition(
                    matrix, asset.vertices[vertexIndex].position);
            }
            result = std::min(result, PointTriangleDistance(
                point, triangle[0], triangle[1], triangle[2]));
        }
    }
    return result;
}

bool BoundsSeparatedBy(const TransformedBounds& left,
                       const TransformedBounds& right,
                       float clearance)
{
    for (std::size_t axis = 0u; axis < 3u; ++axis)
    {
        if (left.maximum[axis] + clearance <= right.minimum[axis] ||
            right.maximum[axis] + clearance <= left.minimum[axis])
            return true;
    }
    return false;
}

bool InLanternInspectionFrustum(const Matrix& matrix, float cameraX,
                                float cameraZ, float yaw, float pitch)
{
    const auto point = Origin(matrix);
    const std::array<float, 3u> delta{{point[0] - cameraX,
                                       point[1] - horde::gameplay::kShowcaseEyeWorldY,
                                       point[2] - cameraZ}};
    std::array<float, 3u> forward{{std::sin(yaw), -0.05f + pitch,
                                   -std::cos(yaw)}};
    const float forwardLength = std::sqrt(
        forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
    for (float& value : forward) value /= forwardLength;
    std::array<float, 3u> right{{-forward[2], 0.0f, forward[0]}};
    const float rightLength = std::sqrt(right[0] * right[0] + right[2] * right[2]);
    for (float& value : right) value /= rightLength;
    const std::array<float, 3u> up{{right[1] * forward[2] - right[2] * forward[1],
                                   right[2] * forward[0] - right[0] * forward[2],
                                   right[0] * forward[1] - right[1] * forward[0]}};
    const auto dot = [&delta](const std::array<float, 3u>& axis) {
        return delta[0] * axis[0] + delta[1] * axis[1] + delta[2] * axis[2];
    };
    const float depth = dot(forward);
    if (depth <= 0.0f) return false;
    // This is the exact 960x540 raygen projection: forward*1.22,
    // screen.x in [-16/9,+16/9], and screen.y in [-0.74,+0.74].
    const float projectedX = 1.22f * dot(right) / depth;
    const float projectedY = 1.22f * dot(up) / depth;
    return std::abs(projectedX) <= 16.0f / 9.0f &&
           std::abs(projectedY) <= 0.74f;
}

bool BoundsInLanternInspectionFrustum(const StaticMeshAsset& asset,
                                      const Matrix& matrix,
                                      float cameraX, float cameraZ,
                                      float yaw, float pitch)
{
    for (std::uint32_t corner = 0u; corner < 8u; ++corner)
    {
        Matrix point = matrix;
        const float x = (corner & 1u) != 0u
            ? asset.bounds.maximum[0] : asset.bounds.minimum[0];
        const float y = (corner & 2u) != 0u
            ? asset.bounds.maximum[1] : asset.bounds.minimum[1];
        const float z = (corner & 4u) != 0u
            ? asset.bounds.maximum[2] : asset.bounds.minimum[2];
        point[12] = matrix[0] * x + matrix[4] * y + matrix[8] * z + matrix[12];
        point[13] = matrix[1] * x + matrix[5] * y + matrix[9] * z + matrix[13];
        point[14] = matrix[2] * x + matrix[6] * y + matrix[10] * z + matrix[14];
        if (!InLanternInspectionFrustum(point, cameraX, cameraZ, yaw, pitch))
            return false;
    }
    return true;
}

struct ProjectedSafeFrame
{
    float minimumX = INFINITY;
    float maximumX = -INFINITY;
    float minimumY = INFINITY;
    float maximumY = -INFINITY;
    float horizontalCoverage = 0.0f;
    float verticalCoverage = 0.0f;
    bool finite = true;
};

ProjectedSafeFrame ProjectAuthoredBoundsToSafeFrame(
    const StaticMeshAsset& asset,
    const Matrix& worldFromAsset,
    const float aspect,
    const float cameraX = 0.0f,
    const float cameraZ = 0.0f,
    const float yaw = 0.0f,
    const float pitch = -0.05f)
{
    ProjectedSafeFrame result;
    const std::array<float, 3u> eye{{
        cameraX, horde::gameplay::kShowcaseEyeWorldY, cameraZ}};
    std::array<float, 3u> forward{{std::sin(yaw), -0.05f + pitch,
                                   -std::cos(yaw)}};
    const float forwardLength = std::sqrt(
        forward[0] * forward[0] + forward[1] * forward[1] +
        forward[2] * forward[2]);
    for (float& value : forward) value /= forwardLength;
    std::array<float, 3u> right{{-forward[2], 0.0f, forward[0]}};
    const float rightLength = std::hypot(right[0], right[2]);
    for (float& value : right) value /= rightLength;
    const std::array<float, 3u> up{{
        right[1] * forward[2] - right[2] * forward[1],
        right[2] * forward[0] - right[0] * forward[2],
        right[0] * forward[1] - right[1] * forward[0]}};
    const auto dot = [](const std::array<float, 3u>& left,
                        const std::array<float, 3u>& axis) {
        return left[0] * axis[0] + left[1] * axis[1] + left[2] * axis[2];
    };
    for (std::uint32_t corner = 0u; corner < 8u; ++corner)
    {
        const float x = (corner & 1u) != 0u
            ? asset.bounds.maximum[0] : asset.bounds.minimum[0];
        const float y = (corner & 2u) != 0u
            ? asset.bounds.maximum[1] : asset.bounds.minimum[1];
        const float z = (corner & 4u) != 0u
            ? asset.bounds.maximum[2] : asset.bounds.minimum[2];
        const std::array<float, 3u> world{{
            worldFromAsset[0] * x + worldFromAsset[4] * y +
                worldFromAsset[8] * z + worldFromAsset[12],
            worldFromAsset[1] * x + worldFromAsset[5] * y +
                worldFromAsset[9] * z + worldFromAsset[13],
            worldFromAsset[2] * x + worldFromAsset[6] * y +
                worldFromAsset[10] * z + worldFromAsset[14]}};
        const std::array<float, 3u> delta{{world[0] - eye[0],
                                           world[1] - eye[1],
                                           world[2] - eye[2]}};
        const float depth = dot(delta, forward);
        if (!std::isfinite(depth) || depth <= 0.02f)
        {
            result.finite = false;
            continue;
        }
        const float ndcX = 1.22f * dot(delta, right) / (depth * aspect);
        const float ndcY = 1.22f * dot(delta, up) / (depth * 0.74f);
        result.minimumX = std::min(result.minimumX, ndcX);
        result.maximumX = std::max(result.maximumX, ndcX);
        result.minimumY = std::min(result.minimumY, ndcY);
        result.maximumY = std::max(result.maximumY, ndcY);
    }
    constexpr float safeMinimum = -0.90f;
    constexpr float safeMaximum = 0.90f;
    const auto coverage = [safeMinimum, safeMaximum](const float minimum,
                                                      const float maximum) {
        const float span = maximum - minimum;
        if (!std::isfinite(span) || span <= 0.000001f) return 0.0f;
        const float visible = std::max(
            0.0f, std::min(maximum, safeMaximum) -
                      std::max(minimum, safeMinimum));
        return std::clamp(visible / span, 0.0f, 1.0f);
    };
    result.horizontalCoverage = coverage(result.minimumX, result.maximumX);
    result.verticalCoverage = coverage(result.minimumY, result.maximumY);
    return result;
}

bool BoundsNear(const StaticMeshAsset& asset,
                const std::array<float, 3u>& minimum,
                const std::array<float, 3u>& maximum,
                float tolerance = 0.002f)
{
    for (std::size_t axis = 0u; axis < 3u; ++axis)
        if (!Near(asset.bounds.minimum[axis], minimum[axis], tolerance) ||
            !Near(asset.bounds.maximum[axis], maximum[axis], tolerance)) return false;
    return true;
}

PaneTopology InspectClosedPaneTopology(const StaticMeshAsset& asset,
                                       std::string_view materialName)
{
    const auto material = std::find_if(asset.materials.begin(), asset.materials.end(),
        [materialName](const auto& candidate) { return candidate.name == materialName; });
    if (material == asset.materials.end()) return {};
    PaneTopology result;
    const std::size_t materialIndex = static_cast<std::size_t>(material - asset.materials.begin());
    using Position = std::array<float, 3u>;
    struct Triangle { std::array<std::size_t, 3u> vertices{}; };
    std::vector<Position> representatives;
    std::vector<Triangle> triangles;
    constexpr float weldTolerance = 1.0e-5f;
    for (const auto& primitive : asset.primitives)
    {
        if (primitive.materialIndex != materialIndex) continue;
        for (std::uint32_t offset = 0u; offset < primitive.indexCount; offset += 3u)
        {
            Triangle triangle;
            for (std::size_t corner = 0u; corner < 3u; ++corner)
            {
                const auto& vertex = asset.vertices[asset.indices[primitive.indexOffset + offset + corner] +
                                                    primitive.vertexOffset];
                const Position position{{vertex.position[0], vertex.position[1], vertex.position[2]}};
                std::size_t representative = representatives.size();
                for (std::size_t candidate = 0u; candidate < representatives.size(); ++candidate)
                {
                    const Position& value = representatives[candidate];
                    const float dx = position[0] - value[0];
                    const float dy = position[1] - value[1];
                    const float dz = position[2] - value[2];
                    if (dx * dx + dy * dy + dz * dz <= weldTolerance * weldTolerance)
                    {
                        representative = candidate;
                        break;
                    }
                }
                if (representative == representatives.size()) representatives.push_back(position);
                triangle.vertices[corner] = representative;
            }
            triangles.push_back(triangle);
        }
    }
    std::map<std::pair<std::size_t, std::size_t>, std::vector<std::pair<std::size_t, bool>>> edges;
    for (std::size_t triangleIndex = 0u; triangleIndex < triangles.size(); ++triangleIndex)
    {
        for (std::size_t corner = 0u; corner < 3u; ++corner)
        {
            const std::size_t a = triangles[triangleIndex].vertices[corner];
            const std::size_t b = triangles[triangleIndex].vertices[(corner + 1u) % 3u];
            edges[{std::min(a, b), std::max(a, b)}].push_back({triangleIndex, a < b});
        }
    }
    std::vector<std::set<std::size_t>> adjacency(triangles.size());
    for (const auto& [edge, uses] : edges)
    {
        (void)edge;
        for (std::size_t left = 0u; left < uses.size(); ++left)
            for (std::size_t right = left + 1u; right < uses.size(); ++right)
            {
                adjacency[uses[left].first].insert(uses[right].first);
                adjacency[uses[right].first].insert(uses[left].first);
            }
    }
    std::vector<bool> visited(triangles.size(), false);
    bool allManifoldOutward = !triangles.empty();
    bool allSevenMillimetres = !triangles.empty();
    for (std::size_t first = 0u; first < triangles.size(); ++first)
    {
        if (visited[first]) continue;
        ++result.components;
        std::vector<std::size_t> pending{first};
        std::set<std::size_t> componentTriangles;
        std::set<std::size_t> componentVertices;
        visited[first] = true;
        while (!pending.empty())
        {
            const std::size_t triangle = pending.back();
            pending.pop_back();
            componentTriangles.insert(triangle);
            componentVertices.insert(triangles[triangle].vertices.begin(), triangles[triangle].vertices.end());
            for (const std::size_t neighbor : adjacency[triangle])
                if (!visited[neighbor]) { visited[neighbor] = true; pending.push_back(neighbor); }
        }
        std::array<float, 3u> minimum{{INFINITY, INFINITY, INFINITY}};
        std::array<float, 3u> maximum{{-INFINITY, -INFINITY, -INFINITY}};
        for (const std::size_t vertex : componentVertices)
            for (std::size_t axis = 0u; axis < 3u; ++axis)
            {
                minimum[axis] = std::min(minimum[axis], representatives[vertex][axis]);
                maximum[axis] = std::max(maximum[axis], representatives[vertex][axis]);
            }
        float thickness = INFINITY;
        for (const std::size_t left : componentVertices)
            for (const std::size_t right : componentVertices)
            {
                if (left >= right) continue;
                const float dx = representatives[left][0] - representatives[right][0];
                const float dy = representatives[left][1] - representatives[right][1];
                const float dz = representatives[left][2] - representatives[right][2];
                thickness = std::min(thickness, std::sqrt(dx * dx + dy * dy + dz * dz));
            }
        allSevenMillimetres = allSevenMillimetres && IsSevenMillimetrePaneThickness(thickness);
        double signedSixTimesVolume = 0.0;
        for (const std::size_t triangle : componentTriangles)
        {
            const auto& a = representatives[triangles[triangle].vertices[0]];
            const auto& b = representatives[triangles[triangle].vertices[1]];
            const auto& c = representatives[triangles[triangle].vertices[2]];
            signedSixTimesVolume += a[0] * (b[1] * c[2] - b[2] * c[1]) +
                a[1] * (b[2] * c[0] - b[0] * c[2]) +
                a[2] * (b[0] * c[1] - b[1] * c[0]);
        }
        allManifoldOutward = allManifoldOutward && signedSixTimesVolume > 1.0e-12;
        for (const auto& [edge, uses] : edges)
        {
            (void)edge;
            if (!componentTriangles.contains(uses.front().first)) continue;
            allManifoldOutward = allManifoldOutward && uses.size() == 2u &&
                uses[0].second != uses[1].second;
        }
    }
    return {result.components, allManifoldOutward, allSevenMillimetres};
}

std::string Lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

void CheckNoContamination(const StaticMeshAsset& asset, std::string_view label)
{
    constexpr std::string_view prohibited[] = {
        "archway", "surroundingarch", "floor", "stand", "hand", "beam", "background"};
    for (const auto& node : asset.nodeTransforms)
    {
        const std::string lower = Lower(node.name);
        for (const auto term : prohibited)
            Check(lower.find(term) == std::string::npos,
                  std::string(label) + " contains no prohibited node " + std::string(term));
    }
    const auto& extent = asset.bounds;
    const float width = extent.maximum[0] - extent.minimum[0];
    const float height = extent.maximum[1] - extent.minimum[1];
    const float depth = extent.maximum[2] - extent.minimum[2];
    Check(width > 0.01f && height > 0.01f && depth > 0.01f &&
              width < 1.20f && height < 1.20f && depth < 1.20f,
          std::string(label) + " bounds remain a single prop-scale silhouette without scene contamination");
}

} // namespace

int main()
{
    const LoadedAsset chestBase = Load("gothic-chest-base", "gothic-chest-base-lod0.runtime.glb");
    const LoadedAsset chestLid = Load("gothic-chest-lid", "gothic-chest-lid-lod0.runtime.glb");
    const LoadedAsset lanternRing = Load("reward-lantern-ring", "reward-lantern-ring-lod0.runtime.glb");
    const LoadedAsset lanternBody = Load("reward-lantern-body", "reward-lantern-body-lod0.runtime.glb");

    Check(HasNode(chestBase.asset, "ChestBase") && HasNode(chestBase.asset, "Latch") &&
              HasNode(chestBase.asset, "RewardLanternHingeSocket") &&
              !HasNode(chestBase.asset, "LanternSocket") &&
              HasNode(chestBase.asset, "ChestLidHinge"),
          "chest base exposes an unambiguous reward hinge target and rear lid hinge anchor");
    Check(HasNode(chestLid.asset, "ChestLid"),
          "chest lid exposes its exact rigid node");
    Check(HasNode(lanternRing.asset, "GripRing") && HasNode(lanternRing.asset, "Hinge"),
          "lantern ring exposes an independent grip and hinge");
    Check(HasNode(lanternBody.asset, "LanternBody") &&
              HasNode(lanternBody.asset, "LanternGlass") &&
              HasNode(lanternBody.asset, "Flame") && HasNode(lanternBody.asset, "Light") &&
              HasNode(lanternBody.asset, "FlameCore"),
          "lantern body exposes exact metal, closed glass, emitter sockets and local core");
    const auto* chestSocket = Socket(chestBase.asset, "RewardLanternHingeSocket");
    const auto* chestLidHinge = Socket(chestBase.asset, "ChestLidHinge");
    const auto* ringGrip = Socket(lanternRing.asset, "GripRing");
    const auto* ringHinge = Socket(lanternRing.asset, "Hinge");
    const auto* flameSocket = Socket(lanternBody.asset, "Flame");
    const auto* lightSocket = Socket(lanternBody.asset, "Light");
    const auto* lidNode = Node(chestLid.asset, "ChestLid");
    Check(chestSocket != nullptr && chestLidHinge != nullptr && ringGrip != nullptr &&
              ringHinge != nullptr && flameSocket != nullptr &&
              lightSocket != nullptr && lidNode != nullptr &&
              MatrixNear(chestSocket->world, Translation(0.0f, 1.28f, 0.30f)) &&
              MatrixNear(chestLidHinge->world, Translation(0.0f, 0.34f, -0.286f)) &&
              MatrixNear(ringGrip->world, Translation(0.0f, 0.085f, 0.0f)) &&
              MatrixNear(ringHinge->world, Translation(0.0f, -0.012f, 0.0f)) &&
              MatrixNear(flameSocket->world, Translation(0.0f, -0.545f, 0.0f)) &&
              MatrixNear(lightSocket->world, Translation(0.0f, -0.515f, 0.0f)) &&
              MatrixNear(lidNode->world, Translation(0.0f, 0.0f, 0.0f)),
          "all sixteen authored matrix elements retain identity-basis GripRing, chest/lantern hinges, flame, light, and lid contracts");

    if (ringGrip != nullptr && ringHinge != nullptr)
    {
        using horde::gameplay::interactions::HeldLightKind;
        using horde::gameplay::interactions::HeldLightPose;
        using horde::gameplay::interactions::kLanternPendulumHardLimitRadians;
        using horde::gameplay::interactions::kLanternPendulumSoftLimitRadians;
        using horde::gameplay::interactions::kLanternPendulumTorsionHardLimitRadians;
        using horde::gameplay::items::HeldItemFixedStepInput;
        using horde::gameplay::items::HeldItemFixedStepState;

        struct Motion
        {
            const char* name;
            float forward;
            float strafe;
            float torsion;
        };
        constexpr float diagonalHard = 0.678773f;
        const std::array<Motion, 9u> motions{{
            {"rest", 0.0f, 0.0f, 0.0f},
            {"soft-forward", kLanternPendulumSoftLimitRadians, 0.0f,
             kLanternPendulumTorsionHardLimitRadians},
            {"soft-backward", -kLanternPendulumSoftLimitRadians, 0.0f,
             -kLanternPendulumTorsionHardLimitRadians},
            {"soft-left", 0.0f, kLanternPendulumSoftLimitRadians,
             -kLanternPendulumTorsionHardLimitRadians},
            {"soft-right", 0.0f, -kLanternPendulumSoftLimitRadians,
             kLanternPendulumTorsionHardLimitRadians},
            {"hard-forward", kLanternPendulumHardLimitRadians, 0.0f,
             -kLanternPendulumTorsionHardLimitRadians},
            {"hard-backward", -kLanternPendulumHardLimitRadians, 0.0f,
             kLanternPendulumTorsionHardLimitRadians},
            {"hard-diagonal", diagonalHard, diagonalHard,
             kLanternPendulumTorsionHardLimitRadians},
            {"hard-opposite", -diagonalHard, -diagonalHard,
             -kLanternPendulumTorsionHardLimitRadians}}};
        constexpr std::array<float, 3u> aspects{{
            16.0f / 9.0f, 9.0f / 16.0f, 1440.0f / 3120.0f}};
        constexpr float claimedScale =
            horde::gameplay::items::kClaimedRewardLanternScale;
        constexpr Matrix lanternScale{{
            claimedScale, 0.0f, 0.0f, 0.0f,
            0.0f, claimedScale, 0.0f, 0.0f,
            0.0f, 0.0f, claimedScale, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f}};
        Matrix inverseRingGrip = ringGrip->world;
        inverseRingGrip[12] = -inverseRingGrip[12];
        inverseRingGrip[13] = -inverseRingGrip[13];
        inverseRingGrip[14] = -inverseRingGrip[14];

        std::array<float, 2u> handHeight{};
        for (std::size_t poseIndex = 0u; poseIndex < 2u; ++poseIndex)
        {
            HeldItemFixedStepInput input;
            input.playerX = 0.0f;
            input.playerZ = 0.0f;
            input.playerPitchRadians = -0.05f;
            input.interaction.heldLightKind = HeldLightKind::RewardLantern;
            input.interaction.heldLightPose = poseIndex == 0u
                ? HeldLightPose::High : HeldLightPose::Low;
            input.interaction.heldLightPoseProgress = 1.0f;
            auto items = horde::gameplay::items::MakeDefaultHeldItemStates();
            HeldItemFixedStepState state;
            std::string diagnostic;
            Check(horde::gameplay::items::ResolveHeldItemsFixedStep(
                      items, input, 1u, state, diagnostic),
                  std::string("reward safe-frame shared hand target resolves: ") +
                      diagnostic);
            handHeight[poseIndex] = state.kinematics.leftHandLocal[1];
            const Matrix ring = Multiply(
                Multiply(state.worldFromLeftHand, lanternScale),
                inverseRingGrip);
            const Matrix hinge = Multiply(ring, ringHinge->world);
            float worstHorizontalCoverage = 1.0f;
            float worstVerticalCoverage = 1.0f;
            float largestRingAbsX = 0.0f;
            float largestRingAbsY = 0.0f;
            for (const float aspect : aspects)
            {
                const ProjectedSafeFrame projectedRing =
                    ProjectAuthoredBoundsToSafeFrame(
                        lanternRing.asset, ring, aspect);
                largestRingAbsX = std::max(
                    largestRingAbsX,
                    std::max(std::abs(projectedRing.minimumX),
                             std::abs(projectedRing.maximumX)));
                largestRingAbsY = std::max(
                    largestRingAbsY,
                    std::max(std::abs(projectedRing.minimumY),
                             std::abs(projectedRing.maximumY)));
                for (const Motion& motion : motions)
                {
                    const Matrix bodyRotation =
                        horde::gameplay::interactions::
                            ComposeLanternPendulumBodyTransform(
                                hinge, motion.forward, motion.strafe,
                                motion.torsion,
                                state.kinematics.
                                    rewardLanternPresentationYawRadians);
                    const Matrix body = Multiply(bodyRotation, lanternScale);
                    const ProjectedSafeFrame projectedBody =
                        ProjectAuthoredBoundsToSafeFrame(
                            lanternBody.asset, body, aspect);
                    if (projectedBody.horizontalCoverage < 0.90f ||
                        projectedBody.verticalCoverage < 0.90f)
                    {
                        std::cout << (poseIndex == 0u ? "high " : "low ")
                                  << motion.name << " aspect=" << aspect
                                  << " frame x=[" << projectedBody.minimumX
                                  << ',' << projectedBody.maximumX << "] y=["
                                  << projectedBody.minimumY << ','
                                  << projectedBody.maximumY << "] coverage="
                                  << projectedBody.horizontalCoverage << '/'
                                  << projectedBody.verticalCoverage << '\n';
                    }
                    worstHorizontalCoverage = std::min(
                        worstHorizontalCoverage,
                        projectedBody.horizontalCoverage);
                    worstVerticalCoverage = std::min(
                        worstVerticalCoverage,
                        projectedBody.verticalCoverage);
                    Check(projectedBody.finite,
                          std::string(poseIndex == 0u ? "high " : "low ") +
                              motion.name +
                              " authored body AABB remains in front of the camera");
                    if (motion.forward == 0.0f && motion.strafe == 0.0f)
                    {
                        Check(projectedBody.minimumX >= -0.90f &&
                                  projectedBody.maximumX <= 0.90f &&
                                  projectedBody.minimumY >= -0.90f &&
                                  projectedBody.maximumY <= 0.90f,
                              std::string(poseIndex == 0u ? "high" : "low") +
                                  " rest full authored lantern body AABB fits the safe frame");
                    }
                }
            }
            std::cout << (poseIndex == 0u ? "high" : "low")
                      << " reward safe-frame worst coverage x/y="
                      << worstHorizontalCoverage << '/' << worstVerticalCoverage
                      << " ring abs x/y=" << largestRingAbsX << '/'
                      << largestRingAbsY << '\n';
            Check(largestRingAbsX <= 0.86f && largestRingAbsY <= 0.86f,
                  std::string(poseIndex == 0u ? "high" : "low") +
                      " complete authored GripRing AABB retains a 14% safe-frame margin");
            Check(worstHorizontalCoverage >= 0.90f &&
                      worstVerticalCoverage >= 0.90f,
                  std::string(poseIndex == 0u ? "high" : "low") +
                      " full authored body AABB keeps at least 90% horizontal and vertical coverage through 45/55-degree swing and torsion extremes on 16:9, 9:16, and 1440:3120");
        }
        Check(handHeight[0] - handHeight[1] >= 0.18f,
              "shared high and low hand targets remain visibly distinct after safe-frame composition");

        // This is the real fixed-step reward-carry stop produced by the shared
        // z=-10 wall clearance response, not an inspection-only camera offset.
        constexpr float wallCameraZ = -9.70f;
        constexpr float wallPlaneZ = -10.0f;
        constexpr float wallSafetyMargin = 0.015f;
        constexpr std::array<float, 3u> wallCameraOrigin{{
            0.0f, horde::gameplay::kShowcaseEyeWorldY, wallCameraZ}};
        constexpr std::array<Motion, 2u> wallMotions{{
            {"wall-rest", 0.0f, 0.0f, 0.0f},
            {"wall-swing", 0.0f, 0.5235988f,
             kLanternPendulumTorsionHardLimitRadians}}};
        const std::array<const horde::gameplay::DevelopmentCheckpoint*, 2u>
            wallCheckpoints{{
                horde::gameplay::FindDevelopmentCheckpoint("lantern-wall-high"),
                horde::gameplay::FindDevelopmentCheckpoint("lantern-wall-low")}};
        for (std::size_t poseIndex = 0u; poseIndex < 2u; ++poseIndex)
        {
            const horde::gameplay::DevelopmentCheckpoint* wallCheckpoint =
                wallCheckpoints[poseIndex];
            Check(wallCheckpoint != nullptr &&
                      wallCheckpoint->cameraX == wallCameraOrigin[0] &&
                      wallCheckpoint->cameraZ == wallCameraZ &&
                      wallCheckpoint->rewardForwardAngleRadians ==
                          wallMotions[poseIndex].forward &&
                      wallCheckpoint->rewardStrafeAngleRadians ==
                          wallMotions[poseIndex].strafe &&
                      wallCheckpoint->rewardTorsionAngleRadians ==
                          wallMotions[poseIndex].torsion,
                  std::string(poseIndex == 0u ? "high" : "low") +
                      " rendered wall checkpoint shares the exact authored-AABB fixture origin and motion");
            HeldItemFixedStepInput input;
            input.playerX = wallCheckpoint == nullptr ? 0.0f : wallCheckpoint->cameraX;
            input.playerZ = wallCheckpoint == nullptr ? wallCameraZ : wallCheckpoint->cameraZ;
            input.playerYawRadians = wallCheckpoint == nullptr ? 0.0f : wallCheckpoint->yaw;
            input.playerPitchRadians = wallCheckpoint == nullptr
                ? -0.05f : wallCheckpoint->pitch;
            input.interaction.heldLightKind = HeldLightKind::RewardLantern;
            input.interaction.heldLightPose = poseIndex == 0u
                ? HeldLightPose::High : HeldLightPose::Low;
            input.interaction.heldLightPoseProgress = 1.0f;
            auto items = horde::gameplay::items::MakeDefaultHeldItemStates();
            HeldItemFixedStepState state;
            std::string diagnostic;
            Check(horde::gameplay::items::ResolveHeldItemsFixedStep(
                      items, input, 1u, state, diagnostic),
                  std::string("wall reward shared hand target resolves: ") + diagnostic);
            const Matrix ring = Multiply(
                Multiply(state.worldFromLeftHand, lanternScale),
                inverseRingGrip);
            const Matrix hinge = Multiply(ring, ringHinge->world);
            const TransformedBounds ringBounds = TransformBounds(
                lanternRing.asset, ring);
            const float ringCameraClearance = MinimumTriangleDistance(
                lanternRing.asset, ring, wallCameraOrigin);
            Check(ringBounds.minimum[2] >= wallPlaneZ + wallSafetyMargin,
                  std::string(poseIndex == 0u ? "high" : "low") +
                      " wall-retracted authored GripRing stays camera-side of the wall plane");
            Check(hinge[14] <= wallCameraZ - wallSafetyMargin,
                  std::string(poseIndex == 0u ? "high" : "low") +
                      " wall-retracted GripRing keeps its shared hinge in front of the camera without inversion");
            Check(ringCameraClearance >= wallSafetyMargin,
                  std::string(poseIndex == 0u ? "high" : "low") +
                      " wall-retracted GripRing keeps the camera origin and 15 mm near sphere outside authored triangles");
            for (const Motion& motion : wallMotions)
            {
                const Matrix bodyRotation =
                    horde::gameplay::interactions::
                        ComposeLanternPendulumBodyTransform(
                            hinge, motion.forward, motion.strafe,
                            motion.torsion,
                            state.kinematics.
                                rewardLanternPresentationYawRadians);
                const Matrix body = Multiply(bodyRotation, lanternScale);
                const TransformedBounds bodyBounds = TransformBounds(
                    lanternBody.asset, body);
                const float bodyCameraClearance = MinimumTriangleDistance(
                    lanternBody.asset, body, wallCameraOrigin);
                const float glassCameraClearance = MinimumTriangleDistance(
                    lanternBody.asset, body, wallCameraOrigin,
                    "LanternGlass");
                std::cout << (poseIndex == 0u ? "high " : "low ")
                          << motion.name << " wall z=["
                          << std::min(ringBounds.minimum[2], bodyBounds.minimum[2])
                          << ','
                          << std::max(ringBounds.maximum[2], bodyBounds.maximum[2])
                          << "] hinge=" << hinge[14]
                          << " presentationYaw="
                          << state.kinematics.rewardLanternPresentationYawRadians
                          << " body x=[" << bodyBounds.minimum[0] << ','
                          << bodyBounds.maximum[0] << "]"
                          << " camera clearance body/glass/ring="
                          << bodyCameraClearance << '/'
                          << glassCameraClearance << '/'
                          << ringCameraClearance
                          << '\n';
                Check(bodyBounds.minimum[2] >=
                          wallPlaneZ + wallSafetyMargin,
                      std::string(poseIndex == 0u ? "high " : "low ") +
                          motion.name +
                          " full transformed body AABB stays camera-side of the actual wall plane");
                Check(hinge[14] <= wallCameraZ - wallSafetyMargin &&
                          bodyBounds.minimum[2] <=
                              wallCameraZ - wallSafetyMargin,
                      std::string(poseIndex == 0u ? "high " : "low ") +
                          motion.name +
                          " wall-retracted body keeps geometry in front of its non-inverted shared hinge");
                Check(bodyCameraClearance >= wallSafetyMargin &&
                          glassCameraClearance >= wallSafetyMargin,
                      std::string(poseIndex == 0u ? "high " : "low ") +
                          motion.name +
                          " camera origin and 15 mm near sphere stay outside body triangles and every closed LanternGlass pane");
            }
        }

        std::vector<float> fullWallSweepZ;
        for (float z = -7.50f; z >= -9.7001f; z -= 0.05f)
            fullWallSweepZ.push_back(z);
        fullWallSweepZ.push_back(-8.53f);
        fullWallSweepZ.push_back(-9.735f);
        std::sort(fullWallSweepZ.begin(), fullWallSweepZ.end(),
                  std::greater<float>());
        for (std::size_t poseIndex = 0u; poseIndex < 2u; ++poseIndex)
        {
            float worstRingClearance = INFINITY;
            float worstBodyClearance = INFINITY;
            float worstGlassClearance = INFINITY;
            float worstWallMargin = INFINITY;
            float worstZ = 0.0f;
            for (const float cameraZ : fullWallSweepZ)
            {
                HeldItemFixedStepInput input;
                input.playerX = 0.0f;
                input.playerZ = cameraZ;
                input.playerYawRadians = 0.0f;
                input.playerPitchRadians = -0.08f;
                input.interaction.heldLightKind = HeldLightKind::RewardLantern;
                input.interaction.heldLightPose = poseIndex == 0u
                    ? HeldLightPose::High : HeldLightPose::Low;
                input.interaction.heldLightPoseProgress = 1.0f;
                auto items = horde::gameplay::items::MakeDefaultHeldItemStates();
                HeldItemFixedStepState state;
                std::string diagnostic;
                Check(horde::gameplay::items::ResolveHeldItemsFixedStep(
                          items, input, 1u, state, diagnostic),
                      std::string("wall interval shared hand target resolves: ") +
                          diagnostic);
                const Matrix ring = Multiply(
                    Multiply(state.worldFromLeftHand, lanternScale),
                    inverseRingGrip);
                const Matrix hinge = Multiply(ring, ringHinge->world);
                const std::array<float, 3u> cameraOrigin{{
                    0.0f, horde::gameplay::kShowcaseEyeWorldY, cameraZ}};
                const TransformedBounds ringBounds = TransformBounds(
                    lanternRing.asset, ring);
                const float ringClearance = MinimumTriangleDistance(
                    lanternRing.asset, ring, cameraOrigin);
                for (const Motion& motion : wallMotions)
                {
                    const Matrix bodyRotation =
                        horde::gameplay::interactions::
                            ComposeLanternPendulumBodyTransform(
                                hinge, motion.forward, motion.strafe,
                                motion.torsion,
                                state.kinematics.
                                    rewardLanternPresentationYawRadians);
                    const Matrix body = Multiply(bodyRotation, lanternScale);
                    const TransformedBounds bodyBounds = TransformBounds(
                        lanternBody.asset, body);
                    const float bodyClearance = MinimumTriangleDistance(
                        lanternBody.asset, body, cameraOrigin);
                    const float glassClearance = MinimumTriangleDistance(
                        lanternBody.asset, body, cameraOrigin,
                        "LanternGlass");
                    const float wallMargin = std::min(
                        ringBounds.minimum[2], bodyBounds.minimum[2]) -
                        wallPlaneZ;
                    worstRingClearance = std::min(
                        worstRingClearance, ringClearance);
                    worstBodyClearance = std::min(
                        worstBodyClearance, bodyClearance);
                    worstGlassClearance = std::min(
                        worstGlassClearance, glassClearance);
                    if (wallMargin < worstWallMargin)
                    {
                        worstWallMargin = wallMargin;
                        worstZ = cameraZ;
                    }
                    Check(std::isfinite(ringClearance) &&
                              std::isfinite(bodyClearance) &&
                              std::isfinite(glassClearance) &&
                              ringBounds.minimum[2] >=
                                  wallPlaneZ + wallSafetyMargin &&
                              bodyBounds.minimum[2] >=
                                  wallPlaneZ + wallSafetyMargin &&
                              ringClearance >= wallSafetyMargin &&
                              bodyClearance >= wallSafetyMargin &&
                              glassClearance >= wallSafetyMargin &&
                              hinge[14] <= cameraZ - wallSafetyMargin,
                          std::string(poseIndex == 0u ? "high " : "low ") +
                              motion.name +
                              " full 5 cm wall interval keeps ring, complete body AABB, every glass pane, and hinge finite/camera-side/wall-safe");
                }
            }
            std::cout << (poseIndex == 0u ? "high" : "low")
                      << " full prop wall sweep samples="
                      << fullWallSweepZ.size()
                      << " clearance ring/body/glass="
                      << worstRingClearance << '/' << worstBodyClearance << '/'
                      << worstGlassClearance << " wall-margin@z="
                      << worstWallMargin << '@' << worstZ << '\n';
        }
    }

    if (chestSocket != nullptr && ringHinge != nullptr && flameSocket != nullptr &&
        lightSocket != nullptr && chestLidHinge != nullptr && lidNode != nullptr)
    {
        const Matrix stage =
            horde::vulkan::raytracing::kProductionRewardChestStageWorldFromBase;
        const Matrix openAngle{{1.0f, 0.0f, 0.0f, 0.0f,
                                0.0f, 0.3420201f, -0.9396926f, 0.0f,
                                0.0f, 0.9396926f, 0.3420201f, 0.0f,
                                0.0f, 0.0f, 0.0f, 1.0f}};
        const Matrix hinge = Multiply(stage, chestSocket->world);
        const Matrix lid = Multiply(Multiply(Multiply(stage, chestLidHinge->world), openAngle),
                                    lidNode->world);
        // Hinge is a translation-only authored socket; inverse is exact and
        // proves the ring grip meets the same world hinge as the body.
        Matrix inverseRingHinge = ringHinge->world;
        inverseRingHinge[12] = -inverseRingHinge[12];
        inverseRingHinge[13] = -inverseRingHinge[13];
        inverseRingHinge[14] = -inverseRingHinge[14];
        const Matrix chestLocalLid = Multiply(
            Multiply(chestLidHinge->world, openAngle), lidNode->world);
        const TransformedBounds chestWorldBounds = TransformBounds(chestBase.asset, stage);
        const TransformedBounds lidLocalBounds = TransformBounds(chestLid.asset, chestLocalLid);
        Check(Near(chestWorldBounds.minimum[1], horde::gameplay::kRouteFloorWorldY) &&
                  Near(chestWorldBounds.maximum[1], horde::gameplay::kRouteFloorWorldY + 0.38f) &&
                  Near(Origin(hinge)[1], horde::gameplay::kRouteFloorWorldY + 1.28f) &&
                  Near(Origin(lid)[1], horde::gameplay::kRouteFloorWorldY + 0.34f),
              "checkpoint 114 places the complete chest on the authoritative corridor floor and composes both authored hinges exactly");
        constexpr std::array<float, 2u> scales{{0.90f, 0.90f}};
        constexpr std::array<std::array<float, 4u>, 2u> cameras{{
            {{horde::gameplay::kRewardChestRoutePosition.x + 1.85f,
              horde::gameplay::kRewardChestRoutePosition.z, -1.57079632679f, -0.35f}},
            {{horde::gameplay::kRewardChestRoutePosition.x + 1.85f,
              horde::gameplay::kRewardChestRoutePosition.z, -1.57079632679f, -0.35f}}}};
        for (std::size_t checkpoint = 0u; checkpoint < scales.size(); ++checkpoint)
        {
            const float value = scales[checkpoint];
            const Matrix scale{{value, 0.0f, 0.0f, 0.0f,
                                0.0f, value, 0.0f, 0.0f,
                                0.0f, 0.0f, value, 0.0f,
                                0.0f, 0.0f, 0.0f, 1.0f}};
            const Matrix ring = Multiply(Multiply(hinge, inverseRingHinge), scale);
            const Matrix body = Multiply(hinge, scale);
            const Matrix glass = Multiply(body, Translation(0.0f, -0.25f, 0.0f));
            const Matrix flame = Multiply(body, flameSocket->world);
            const Matrix light = Multiply(body, lightSocket->world);
            const Matrix bodyLocal = Multiply(chestSocket->world, scale);
            const Matrix ringLocal = Multiply(
                Multiply(chestSocket->world, inverseRingHinge), scale);
            const TransformedBounds bodyLocalBounds =
                TransformBounds(lanternBody.asset, bodyLocal);
            const TransformedBounds ringLocalBounds =
                TransformBounds(lanternRing.asset, ringLocal);
            const TransformedBounds bodyWorldBounds =
                TransformBounds(lanternBody.asset, body);
            const TransformedBounds ringWorldBounds =
                TransformBounds(lanternRing.asset, ring);
            const TransformedBounds chestLocalBounds =
                TransformBounds(chestBase.asset, Translation(0.0f, 0.0f, 0.0f));
            const auto& camera = cameras[checkpoint];
            const auto flameOrigin = Origin(flame);
            const auto lightOrigin = Origin(light);
            std::cout << "checkpoint " << (114u + checkpoint)
                      << " flame origin=" << flameOrigin[0] << ',' << flameOrigin[1]
                      << ',' << flameOrigin[2] << " light origin=" << lightOrigin[0]
                      << ',' << lightOrigin[1] << ',' << lightOrigin[2] << '\n';
            Check(BoundsInLanternInspectionFrustum(
                      lanternRing.asset, ring, camera[0], camera[1], camera[2], camera[3]) &&
                      BoundsInLanternInspectionFrustum(
                          lanternBody.asset, body, camera[0], camera[1], camera[2], camera[3]) &&
                      InLanternInspectionFrustum(glass, camera[0], camera[1], camera[2], camera[3]) &&
                      InLanternInspectionFrustum(flame, camera[0], camera[1], camera[2], camera[3]) &&
                      InLanternInspectionFrustum(light, camera[0], camera[1], camera[2], camera[3]) &&
                      (checkpoint != 0u ||
                       (BoundsInLanternInspectionFrustum(
                            chestBase.asset, stage, camera[0], camera[1], camera[2], camera[3]) &&
                        BoundsInLanternInspectionFrustum(
                            chestLid.asset, lid, camera[0], camera[1], camera[2], camera[3]))),
                  checkpoint == 0u
                      ? "full chest, lid, ring, and body bounds fit checkpoint 114's exact raygen frustum"
                      : "full ring and body bounds fit checkpoint 115's exact raygen frustum");
            Check(Near(flameOrigin[0], horde::gameplay::kRewardChestRoutePosition.x + 0.2598076f) &&
                      Near(flameOrigin[2], horde::gameplay::kRewardChestRoutePosition.z + 0.15f) &&
                      Near(lightOrigin[0], horde::gameplay::kRewardChestRoutePosition.x + 0.2598076f) &&
                      Near(lightOrigin[2], horde::gameplay::kRewardChestRoutePosition.z + 0.15f) &&
                      Near(flameOrigin[1], horde::gameplay::kRouteFloorWorldY + 1.28f - 0.545f * value) &&
                      Near(lightOrigin[1], horde::gameplay::kRouteFloorWorldY + 1.28f - 0.515f * value) &&
                      Near(lightOrigin[1] - flameOrigin[1], 0.030f * value),
                  checkpoint == 0u
                      ? "checkpoint 114 flame and light origins exactly compose at 0.90 scale"
                      : "checkpoint 115 flame and light origins exactly compose at 0.90 scale");
            Check(BoundsSeparatedBy(bodyLocalBounds, chestLocalBounds, 0.002f) &&
                      BoundsSeparatedBy(ringLocalBounds, chestLocalBounds, 0.002f) &&
                      BoundsSeparatedBy(bodyLocalBounds, lidLocalBounds, 0.025f) &&
                      BoundsSeparatedBy(ringLocalBounds, lidLocalBounds, 0.025f) &&
                      BoundsSeparatedBy(bodyWorldBounds, chestWorldBounds, 0.002f) &&
                      BoundsSeparatedBy(ringWorldBounds, chestWorldBounds, 0.002f) &&
                      Near(bodyWorldBounds.minimum[1] - chestWorldBounds.maximum[1], 0.0225f) &&
                      bodyLocalBounds.minimum[1] >= chestLocalBounds.maximum[1] + 0.002f &&
                      bodyLocalBounds.minimum[1] <= chestLocalBounds.maximum[1] + 0.035f &&
                      bodyLocalBounds.minimum[2] - lidLocalBounds.maximum[2] > 0.177f,
                  checkpoint == 0u
                      ? "checkpoint 114 full transformed bounds reveal the lantern immediately above the floor-contact chest with open-lid clearance"
                      : "checkpoint 115 reuses the exact nonintersecting Task 8 reward reveal transform");
        }
    }

    const std::uint64_t chestTriangles = Triangles(chestBase.asset) + Triangles(chestLid.asset);
    const std::uint64_t lanternTriangles = Triangles(lanternRing.asset) + Triangles(lanternBody.asset);
    Check(chestTriangles >= 4000u && chestTriangles <= 8000u,
          "production chest remains within the 4k-8k triangle budget");
    Check(lanternTriangles >= 6000u && lanternTriangles <= 10000u,
          "production lantern metal plus glass remains within the 6k-10k triangle budget");
    Check(chestBase.asset.materials.size() == 2u && chestLid.asset.materials.size() == 2u &&
              HasMaterial(chestBase.asset, "ChestWood") && HasMaterial(chestBase.asset, "BlackIron"),
          "chest uses only the shared neutral wood and blackened-iron material pair");
    Check(lanternRing.asset.materials.size() == 1u && lanternBody.asset.materials.size() == 3u &&
              HasMaterial(lanternRing.asset, "BlackIron") &&
              HasMaterial(lanternBody.asset, "BlackIron") &&
              HasMaterial(lanternBody.asset, "LanternGlass") &&
              HasMaterial(lanternBody.asset, "FlameCore"),
          "lantern uses the bounded metal, glass and engine-core material set");

    const auto glass = std::find_if(lanternBody.asset.materials.begin(),
        lanternBody.asset.materials.end(), [](const auto& material) {
            return material.name == "LanternGlass";
        });
    Check(glass != lanternBody.asset.materials.end() && glass->transmissionFactor >= 0.90f &&
              glass->ior > 1.50f && glass->ior < 1.53f && glass->thicknessFactor > 0.0f &&
              (glass->flags & 4u) != 0u && (glass->flags & 512u) == 0u &&
              glass->baseColorTexture < 0,
          "lantern glass is a separate unpainted thick dielectric with physical IOR");
    // The deterministic production asset uses six isolated closed cuboids: 12
    // triangles each. This ensures the runtime cannot silently regress to a
    // single thin/fused pane while still retaining the LanternGlass material.
    Check(TrianglesForMaterial(lanternBody.asset, "LanternGlass") == 72u,
          "lantern glass retains six separate closed 7 mm panes");
    const PaneTopology panes = InspectClosedPaneTopology(lanternBody.asset, "LanternGlass");
    if (!(panes.components == 6u && panes.manifoldOutward && panes.sevenMillimetres))
        std::cerr << "pane topology components=" << panes.components << " manifold="
                  << panes.manifoldOutward << " thickness=" << panes.sevenMillimetres << '\n';
    Check(panes.components == 6u && panes.manifoldOutward && panes.sevenMillimetres,
          "lantern glass metre-space weld has exactly six outward two-use-edge closed components at 7 mm thickness");
    const float cageGlassClearance = MinimumCageGlassRadialClearance(lanternBody.asset);
    const float paneTangentialHalfWidth = MaximumGlassPaneTangentialHalfWidth(lanternBody.asset);
    std::cout << "production pane cage radial clearance=" << cageGlassClearance << '\n';
    std::cout << "production pane tangential half-width=" << paneTangentialHalfWidth << '\n';
    Check(std::isfinite(cageGlassClearance) && cageGlassClearance >= 0.0015f,
          "production pane volumes retain at least 1.5 mm radial clearance from cage geometry across all six faces");
    Check(std::isfinite(paneTangentialHalfWidth) &&
              paneTangentialHalfWidth >= 0.092f && paneTangentialHalfWidth <= 0.093f,
          "production pane side boundaries preserve the authored 185 mm clear aperture");
    Check(lanternBody.asset.bounds.minimum[1] <= -0.97f,
          "lantern body keeps the pointed lower finial after pane aperture insetting");
    const PaneTopology paneFixture = InspectClosedPaneTopology(
        MakeClosedPaneFixture(0.007f, false, false), "LanternGlass");
    const PaneTopology openPaneFixture = InspectClosedPaneTopology(
        MakeClosedPaneFixture(0.007f, true, false), "LanternGlass");
    const PaneTopology malformedPaneFixture = InspectClosedPaneTopology(
        MakeClosedPaneFixture(0.007f, false, true), "LanternGlass");
    const PaneTopology wrongThicknessPaneFixture = InspectClosedPaneTopology(
        MakeClosedPaneFixture(0.004f, false, false), "LanternGlass");
    const PaneTopology inwardPaneFixture = InspectClosedPaneTopology(
        ReversePaneWinding(MakeClosedPaneFixture(0.007f, false, false)),
        "LanternGlass");
    const PaneTopology flippedTrianglePaneFixture = InspectClosedPaneTopology(
        FlipFirstPaneTriangle(MakeClosedPaneFixture(0.007f, false, false)),
        "LanternGlass");
    Check(paneFixture.components == 1u && paneFixture.manifoldOutward && paneFixture.sevenMillimetres &&
              (!openPaneFixture.manifoldOutward || !openPaneFixture.sevenMillimetres) &&
              !malformedPaneFixture.manifoldOutward && !wrongThicknessPaneFixture.sevenMillimetres &&
              !inwardPaneFixture.manifoldOutward &&
              !flippedTrianglePaneFixture.manifoldOutward,
          "complete metre-space pane inspector rejects open, duplicated, 4 mm, wholly inward, and one-triangle-flipped cuboid fixtures");
    Check(lanternBody.asset.bounds.maximum[1] <= 0.05f &&
              lanternBody.asset.bounds.minimum[1] < -0.55f,
          "lantern body is authored below its local hinge pivot");
    Check(chestLid.asset.bounds.minimum[2] >= -0.05f &&
              chestLid.asset.bounds.maximum[2] > 0.30f,
          "chest lid geometry extends forward from its rear local hinge pivot");

    CheckNoContamination(chestBase.asset, "chest base");
    CheckNoContamination(chestLid.asset, "chest lid");
    CheckNoContamination(lanternRing.asset, "lantern ring");
    CheckNoContamination(lanternBody.asset, "lantern body");
    Check(BoundsNear(chestBase.asset,
                     {{-0.510f, 0.000f, -0.299f}},
                     {{0.510f, 0.380f, 0.355f}}) &&
              BoundsNear(chestLid.asset,
                         {{-0.505f, 0.025f, -0.025f}},
                         {{0.505f, 0.285f, 0.590f}}) &&
              BoundsNear(lanternRing.asset,
                         {{-0.072f, -0.028f, -0.020f}},
                         {{0.072f, 0.157f, 0.020f}}) &&
              BoundsNear(lanternBody.asset,
                         {{-0.224f, -0.975f, -0.256f}},
                         {{0.224f, -0.015f, 0.256f}}),
          "exact per-component metre-space bounds reject imported arch, floor, room, and scenery contamination even when node names look benign");
    const TransformedBounds stagedChestBaseBounds = TransformBounds(
        chestBase.asset,
        horde::vulkan::raytracing::kProductionRewardChestStageWorldFromBase);
    Check(horde::gameplay::kRewardChestCollisionRect.minX <=
                  stagedChestBaseBounds.minimum[0] &&
              horde::gameplay::kRewardChestCollisionRect.maxX >=
                  stagedChestBaseBounds.maximum[0] &&
              horde::gameplay::kRewardChestCollisionRect.minZ <=
                  stagedChestBaseBounds.minimum[2] &&
              horde::gameplay::kRewardChestCollisionRect.maxZ >=
                  stagedChestBaseBounds.maximum[2],
          "shared reward chest collision conservatively contains the complete rotated production base footprint");

    if (failures != 0)
    {
        std::cerr << failures << " production prop asset assertion(s) failed\n";
        return 1;
    }
    std::cout << "Production Gothic chest and reward lantern asset contracts passed\n";
    return 0;
}
