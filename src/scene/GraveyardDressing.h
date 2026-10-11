#pragma once

#include "scene/assets/StaticMeshAsset.h"
#include "gameplay/simulation/TriangleSupportSurface.h"
#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace horde::scene {
struct GraveyardPlacement {
    const char* asset = nullptr;
    std::array<float, 3> origin{}, minimum{}, maximum{};
    float yaw = 0, burial = 0;
};
struct GraveyardDressingReport {
    std::vector<GraveyardPlacement> placements;
    std::size_t triangles = 0, contactTriangles = 0;
};
// Adds only original/verified burial pieces to existing admitted material families.
// Imported world triangles also supply immutable nonwalkable contact geometry.
// Neither owner nor support is changed on a failed intake.
bool AppendPreparedGraveyard(const std::filesystem::path& assetRoot,
    assets::StaticMeshAsset& owner,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>& support,
    GraveyardDressingReport& report, std::string& diagnostic);
}
