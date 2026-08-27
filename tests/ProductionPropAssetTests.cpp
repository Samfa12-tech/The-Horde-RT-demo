#include "scene/assets/AssetManifest.h"
#include "scene/assets/StaticMeshAsset.h"

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

bool InLanternInspectionFrustum(const Matrix& matrix, float cameraX,
                                float cameraZ, float yaw, float pitch)
{
    const auto point = Origin(matrix);
    const std::array<float, 3u> delta{{point[0] - cameraX,
                                       point[1] - 0.58f,
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
              HasNode(chestBase.asset, "LanternSocket") && HasNode(chestBase.asset, "ChestLidHinge"),
          "chest base exposes exact rigid nodes, lantern socket, and rear lid hinge anchor");
    Check(HasNode(chestLid.asset, "ChestLid"),
          "chest lid exposes its exact rigid node");
    Check(HasNode(lanternRing.asset, "GripRing") && HasNode(lanternRing.asset, "Hinge"),
          "lantern ring exposes an independent grip and hinge");
    Check(HasNode(lanternBody.asset, "LanternBody") &&
              HasNode(lanternBody.asset, "LanternGlass") &&
              HasNode(lanternBody.asset, "Flame") && HasNode(lanternBody.asset, "Light") &&
              HasNode(lanternBody.asset, "FlameCore"),
          "lantern body exposes exact metal, closed glass, emitter sockets and local core");
    const auto* chestSocket = Socket(chestBase.asset, "LanternSocket");
    const auto* chestLidHinge = Socket(chestBase.asset, "ChestLidHinge");
    const auto* ringHinge = Socket(lanternRing.asset, "Hinge");
    const auto* flameSocket = Socket(lanternBody.asset, "Flame");
    const auto* lightSocket = Socket(lanternBody.asset, "Light");
    const auto* lidNode = Node(chestLid.asset, "ChestLid");
    Check(chestSocket != nullptr && chestLidHinge != nullptr && ringHinge != nullptr && flameSocket != nullptr &&
              lightSocket != nullptr && lidNode != nullptr &&
              MatrixNear(chestSocket->world, Translation(0.0f, 0.18f, 0.0f)) &&
              MatrixNear(chestLidHinge->world, Translation(0.0f, 0.34f, -0.286f)) &&
              MatrixNear(ringHinge->world, Translation(0.0f, -0.012f, 0.0f)) &&
              MatrixNear(flameSocket->world, Translation(0.0f, -0.545f, 0.0f)) &&
              MatrixNear(lightSocket->world, Translation(0.0f, -0.515f, 0.0f)) &&
              MatrixNear(lidNode->world, Translation(0.0f, 0.0f, 0.0f)),
          "all sixteen authored matrix elements retain the exact chest hinge, lantern hinge, flame, light, and lid contracts");
    if (chestSocket != nullptr && ringHinge != nullptr && flameSocket != nullptr &&
        lightSocket != nullptr && chestLidHinge != nullptr && lidNode != nullptr)
    {
        const Matrix stage{{0.50f, 0.0f, -0.8660254f, 0.0f,
                            0.0f, 1.0f, 0.0f, 0.0f,
                            0.8660254f, 0.0f, 0.50f, 0.0f,
                            -12.50f, 0.37f, -15.42f, 1.0f}};
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
        Check(Near(Origin(hinge)[1], 0.55f) && Near(Origin(lid)[1], 0.71f) &&
                  InLanternInspectionFrustum(
                      lid, -10.65f, -15.20f, -1.57079632679f, -0.22f),
              "authored rear hinge and -70 degree local angle place the lid in checkpoint 114's exact raygen frustum");
        constexpr std::array<float, 2u> scales{{0.90f, 1.05f}};
        constexpr std::array<std::array<float, 4u>, 2u> cameras{{
            {{-10.65f, -15.20f, -1.57079632679f, -0.22f}},
            {{-10.95f, -15.20f, -1.57079632679f, -0.16f}}}};
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
            const auto& camera = cameras[checkpoint];
            const auto flameOrigin = Origin(flame);
            const auto lightOrigin = Origin(light);
            Check(InLanternInspectionFrustum(ring, camera[0], camera[1], camera[2], camera[3]) &&
                      InLanternInspectionFrustum(body, camera[0], camera[1], camera[2], camera[3]) &&
                      InLanternInspectionFrustum(glass, camera[0], camera[1], camera[2], camera[3]) &&
                      InLanternInspectionFrustum(flame, camera[0], camera[1], camera[2], camera[3]) &&
                      InLanternInspectionFrustum(light, camera[0], camera[1], camera[2], camera[3]),
                  checkpoint == 0u
                      ? "ring, body, glass, flame, and light origins lie inside checkpoint 114's exact raygen frustum"
                      : "ring, body, glass, flame, and light origins lie inside checkpoint 115's exact raygen frustum");
            Check(Near(flameOrigin[0], -12.50f) && Near(flameOrigin[2], -15.42f) &&
                      Near(lightOrigin[0], -12.50f) && Near(lightOrigin[2], -15.42f) &&
                      Near(flameOrigin[1], 0.55f - 0.545f * value) &&
                      Near(lightOrigin[1], 0.55f - 0.515f * value) &&
                      Near(lightOrigin[1] - flameOrigin[1], 0.030f * value),
                  checkpoint == 0u
                      ? "checkpoint 114 flame and light origins exactly compose at 0.90 scale"
                      : "checkpoint 115 flame and light origins exactly compose at 1.05 scale");
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

    if (failures != 0)
    {
        std::cerr << failures << " production prop asset assertion(s) failed\n";
        return 1;
    }
    std::cout << "Production Gothic chest and reward lantern asset contracts passed\n";
    return 0;
}
