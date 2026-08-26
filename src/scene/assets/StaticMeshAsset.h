#pragma once

#include "scene/assets/AssetManifest.h"
#include "vulkan/raytracing/RtSceneAbi.generated.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace horde::scene::assets
{

using StaticRtVertex = horde::vulkan::raytracing::StaticRtVertex;

struct StaticPrimitiveRecord
{
    std::uint32_t vertexOffset = 0u;
    std::uint32_t indexOffset = 0u;
    std::uint32_t indexCount = 0u;
    std::uint32_t materialIndex = 0u;
    std::uint32_t nodeTransformIndex = 0u;
};

struct StaticNodeTransform
{
    std::string name;
    std::array<float, 16u> world{};
};

struct StaticSocket
{
    std::string name;
    std::uint32_t nodeTransformIndex = 0u;
    std::array<float, 16u> world{};
};

struct StaticBounds
{
    std::array<float, 3u> minimum{};
    std::array<float, 3u> maximum{};
};

struct StaticMaterial
{
    std::string name;
    std::array<float, 4u> baseColorFactor{{1.0f, 1.0f, 1.0f, 1.0f}};
    std::array<float, 3u> emissiveFactor{};
    std::array<float, 3u> attenuationColor{{1.0f, 1.0f, 1.0f}};
    float emissiveStrength = 1.0f;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    float occlusionStrength = 1.0f;
    float transmissionFactor = 0.0f;
    float ior = 1.5f;
    float thicknessFactor = 0.0f;
    float attenuationDistance = 0.0f;
    std::int32_t baseColorTexture = -1;
    std::int32_t normalTexture = -1;
    std::int32_t ormTexture = -1;
    std::int32_t emissiveTexture = -1;
    std::uint32_t flags = 0u;
};

class StaticMeshAsset
{
public:
    static bool Load(const std::filesystem::path& runtimeGlb,
                     const AssetManifest& manifest,
                     StaticMeshAsset& asset,
                     std::string& diagnostic);

    std::vector<StaticRtVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<StaticPrimitiveRecord> primitives;
    std::vector<StaticNodeTransform> nodeTransforms;
    std::vector<StaticMaterial> materials;
    std::vector<StaticSocket> sockets;
    StaticBounds bounds{};
};

} // namespace horde::scene::assets
