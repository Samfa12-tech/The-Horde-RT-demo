#include "scene/assets/AssetManifest.h"
#include "scene/assets/StaticMeshAsset.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
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
