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

// These compact fitted openings cut only the visible wall at x=-1.85. Their
// native modules retain thick returns and a sealed rear wall beyond the hidden
// shell; the third opening is in the outer route's x=6 wall, never a portal.
inline constexpr std::array<TombDressingNicheOpening, 3u> kTombDressingNicheOpenings{{
    {"entry-left-rect", TombDressingWallAxis::X, -1.85f, -0.54f, 0.285f,
     -1.048f, -0.352f, "sealed module back about 0.29 m behind the visible wall"},
    {"entry-left-arched", TombDressingWallAxis::X, -1.85f, -0.55f, 0.35f,
     -2.957f, -2.243f, "sealed module back about 0.29 m behind the visible wall"},
    {"outer-passage-east-rect", TombDressingWallAxis::X, 6.00f, -0.50f, 0.275f,
     -12.729f, -12.071f, "self-contained back wall; no room portal"},
}};
inline constexpr float kEntryRectNicheShelfTop = -0.394336f;
inline constexpr float kEntryArchNicheShelfTop = -0.391094f;
inline constexpr float kOuterNicheShelfTop = -0.363164f;

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
    std::array<TombDressingPlacementBounds, 16u> placementBounds{};
    TombDressingBounds entryLintelShelfBounds{};
};

// Loads selected, prepared T02/T03/niche GLBs through StaticMeshAsset::Load,
// then bakes sparse scene transforms into the caller's already-owned static
// asset. It adds no BLAS/asset owner and commits atomically on success.
bool AppendPreparedTombDressing(const std::filesystem::path& assetRoot,
                                assets::StaticMeshAsset& existingOwner,
                                TombDressingBuildReport& report,
                                std::string& diagnostic);

} // namespace horde::scene
