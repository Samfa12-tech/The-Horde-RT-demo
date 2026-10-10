#pragma once

#include "scene/assets/StaticMeshAsset.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>

namespace horde::scene
{

enum class TombDressingWallAxis : unsigned char
{
    X,
};

struct TombDressingNicheOpening
{
    const char* id;
    TombDressingWallAxis wallAxis;
    float wallCoordinate;
    float minimumY;
    float maximumY;
    float minimumZ;
    float maximumZ;
    const char* backing;
};

// The two tomb-side holes cut only the visible wall at x=-1.85. A matching
// backing/cut is required behind that plane so the recessed module's rear wall
// is the ray-visible niche back rather than a paper-thin room shell. The third
// opening is in the outer route's x=6 wall, never a connector to another room.
inline constexpr std::array<TombDressingNicheOpening, 3u> kTombDressingNicheOpenings{{
    {"entry-left-rect", TombDressingWallAxis::X, -1.85f, -0.65f, 0.95f,
     -1.34f, -0.06f, "recess-back at x=-1.85; cut matching hidden shell x=-1.92"},
    {"entry-left-arched", TombDressingWallAxis::X, -1.85f, -0.65f, 0.95f,
     -3.24f, -1.96f, "recess-back at x=-1.85; cut matching hidden shell x=-1.92"},
    {"outer-passage-east-rect", TombDressingWallAxis::X, 6.00f, -0.65f, 0.95f,
     -13.04f, -11.76f, "self-contained back wall; no room portal"},
}};

struct TombDressingBounds
{
    std::array<float, 3u> minimum{};
    std::array<float, 3u> maximum{};
};

struct TombDressingPlacementBounds
{
    const char* asset = nullptr;
    TombDressingBounds worldBounds{};
};

struct TombDressingBuildReport
{
    std::size_t addedVertices = 0u;
    std::size_t addedIndices = 0u;
    std::size_t addedTriangles = 0u;
    std::size_t addedPrimitives = 0u;
    std::size_t addedMaterials = 0u;
    std::size_t placedInstances = 0u;
    TombDressingBounds addedBounds{};
    std::array<TombDressingPlacementBounds, 13u> placementBounds{};
};

// Loads selected, prepared T02/T03/niche GLBs through StaticMeshAsset::Load,
// then bakes sparse scene transforms into the caller's already-owned static
// asset. It adds no BLAS/asset owner and commits atomically on success.
bool AppendPreparedTombDressing(const std::filesystem::path& assetRoot,
                                assets::StaticMeshAsset& existingOwner,
                                TombDressingBuildReport& report,
                                std::string& diagnostic);

} // namespace horde::scene
