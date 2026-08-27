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

bool Near(float left, float right, float tolerance = 0.00001f)
{
    return std::abs(left - right) <= tolerance;
}

bool IsSevenMillimetrePaneThickness(float thickness)
{
    return std::abs(thickness - 0.007f) <= 0.0005f;
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

struct PaneTopology
{
    std::size_t components = 0u;
    bool manifoldOutward = false;
    bool sevenMillimetres = false;
};

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
}

} // namespace

int main()
{
    const LoadedAsset chestBase = Load("gothic-chest-base", "gothic-chest-base-lod0.runtime.glb");
    const LoadedAsset chestLid = Load("gothic-chest-lid", "gothic-chest-lid-lod0.runtime.glb");
    const LoadedAsset lanternRing = Load("reward-lantern-ring", "reward-lantern-ring-lod0.runtime.glb");
    const LoadedAsset lanternBody = Load("reward-lantern-body", "reward-lantern-body-lod0.runtime.glb");

    Check(HasNode(chestBase.asset, "ChestBase") && HasNode(chestBase.asset, "Latch") &&
              HasNode(chestBase.asset, "LanternSocket"),
          "chest base exposes exact rigid nodes and lantern socket");
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
    const auto* ringHinge = Socket(lanternRing.asset, "Hinge");
    const auto* flameSocket = Socket(lanternBody.asset, "Flame");
    const auto* lightSocket = Socket(lanternBody.asset, "Light");
    const auto* lidSocket = Socket(chestLid.asset, "ChestLid");
    Check(chestSocket != nullptr && ringHinge != nullptr && flameSocket != nullptr &&
              lightSocket != nullptr && lidSocket != nullptr &&
              Near(chestSocket->world[12], 0.0f) && Near(chestSocket->world[13], 0.18f) &&
              Near(chestSocket->world[14], 0.0f) && Near(ringHinge->world[13], -0.012f) &&
              Near(flameSocket->world[13], -0.545f) && Near(lightSocket->world[13], -0.515f) &&
              Near(lidSocket->world[0], 1.0f) && Near(lidSocket->world[5], 1.0f) &&
              Near(lidSocket->world[10], 1.0f),
          "authored chest/lantern socket matrices retain exact rear-hinge, flame, and light offsets");

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
    Check(!IsSevenMillimetrePaneThickness(0.004f) && !IsSevenMillimetrePaneThickness(0.010f),
          "wrong-thickness pane negative fixtures cannot satisfy the production 7 mm tolerance");
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

    if (failures != 0)
    {
        std::cerr << failures << " production prop asset assertion(s) failed\n";
        return 1;
    }
    std::cout << "Production Gothic chest and reward lantern asset contracts passed\n";
    return 0;
}
