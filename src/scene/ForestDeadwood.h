#pragma once
#include "scene/assets/StaticMeshAsset.h"
#include "gameplay/simulation/TriangleSupportSurface.h"
#include <filesystem>
#include <vector>
namespace horde::scene {
struct ForestDeadwoodPlacement {std::array<float,3> origin{},minimum{},maximum{};unsigned source=0;};
struct ForestDeadwoodReport {std::size_t triangles=0;std::vector<ForestDeadwoodPlacement> placements;};
bool AppendPreparedForestDeadwood(const std::filesystem::path& root,assets::StaticMeshAsset& owner,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>& support,
    ForestDeadwoodReport& report,std::string& diagnostic);
}
