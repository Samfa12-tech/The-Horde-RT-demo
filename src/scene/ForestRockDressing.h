#pragma once
#include "scene/assets/StaticMeshAsset.h"
#include "gameplay/simulation/TriangleSupportSurface.h"
#include <filesystem>
#include <vector>
namespace horde::scene {
struct ForestRockPlacement {
    std::array<float,3> origin{},minimum{},maximum{};
    unsigned source=0;
};
struct ForestRockReport {std::size_t triangles=0;std::vector<ForestRockPlacement> placements;};
bool AppendPreparedForestRocks(const std::filesystem::path& assetRoot,assets::StaticMeshAsset& owner,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>& support,
    ForestRockReport& report,std::string& diagnostic);
}
