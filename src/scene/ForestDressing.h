#pragma once

#include "scene/assets/StaticMeshAsset.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>

namespace horde::scene
{

struct ForestDressingBounds
{
    std::array<float, 3u> minimum{};
    std::array<float, 3u> maximum{};
};

struct ForestDressingPlacement
{
    const char* asset = nullptr;
    std::array<float, 3u> origin{};
    float yawRadians = 0.0f;
    float supportY = 0.0f;
    ForestDressingBounds worldBounds{};
    ForestDressingBounds trunkBounds{};
};

struct ForestDressingBuildReport
{
    std::size_t addedVertices = 0u;
    std::size_t addedIndices = 0u;
    std::size_t addedTriangles = 0u;
    std::size_t addedPrimitives = 0u;
    std::size_t addedMaterials = 0u;
    std::size_t placedInstances = 0u;
    ForestDressingBounds addedBounds{};
    std::array<ForestDressingPlacement, 10u> placements{};
};

// Imports the admitted LOD1 pine and alder, bakes ten grounded world
// placements into the existing static owner, and groups them into four
// shared PBR material families. The source models add no new asset/BLAS owner.
bool AppendPreparedForestDressing(const std::filesystem::path& assetRoot,
                                 assets::StaticMeshAsset& existingOwner,
                                 ForestDressingBuildReport& report,
                                 std::string& diagnostic);

} // namespace horde::scene
