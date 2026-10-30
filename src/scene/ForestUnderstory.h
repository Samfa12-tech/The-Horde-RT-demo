#pragma once
#include "scene/assets/StaticMeshAsset.h"
#include "gameplay/simulation/TriangleSupportSurface.h"
#include <vector>

namespace horde::scene {
struct ForestUnderstoryPlacement {
    std::array<float,3> origin{};
    std::array<float,3> surfaceNormal{};
    float supportY=0, scale=1;
    std::array<float,3> minimum{}, maximum{};
};
struct ForestUnderstoryReport {
    std::size_t triangles=0;
    std::vector<ForestUnderstoryPlacement> placements;
};
// Soft closed foliage reuses ForestAlder. It creates no collision, material,
// texture or AS owner and publishes only after every placement validates.
bool AppendPreparedForestUnderstory(const std::filesystem::path& root,
    assets::StaticMeshAsset& owner, const gameplay::simulation::TriangleSupportSurface& support,
    ForestUnderstoryReport& report, std::string& diagnostic);
}
