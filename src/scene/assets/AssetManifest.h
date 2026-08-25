#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace horde::scene::assets
{

struct AssetBudgets
{
    std::uint32_t maxVertices = 0u;
    std::uint32_t maxIndices = 0u;
    std::uint32_t maxPrimitives = 0u;
    std::uint32_t maxMaterials = 0u;
    std::uint32_t maxTextureLayersPerKind = 0u;
};

struct AssetLodBudget
{
    std::string name;
    std::uint32_t maxTriangles = 0u;
};

struct RuntimeTextureProfile
{
    std::string androidEncoding;
    std::string windowsEncoding;
    bool mipmapped = false;
};

struct MaterialOverride
{
    std::string material;
    float emissiveStrength = 1.0f;
};

struct AssetManifest
{
    std::uint32_t schema = 0u;
    std::string assetName;
    float metresPerUnit = 0.0f;
    std::string upAxis;
    std::string forwardAxis;
    AssetBudgets budgets;
    std::vector<AssetLodBudget> lods;
    std::vector<std::string> requiredSockets;
    RuntimeTextureProfile textureProfile;
    std::vector<MaterialOverride> materialOverrides;

    static bool Load(const std::filesystem::path& path,
                     AssetManifest& manifest,
                     std::string& diagnostic);
};

} // namespace horde::scene::assets
