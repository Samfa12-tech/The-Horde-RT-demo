#pragma once
#include "scene/assets/StaticMeshAsset.h"
#include "gameplay/simulation/TriangleSupportSurface.h"
#include <filesystem>
#include <vector>
namespace horde::scene {
struct ForestDeadwoodPlacement {std::array<float,3> origin{},minimum{},maximum{};unsigned source=0;};
struct ForestDeadwoodReport {
    std::size_t triangles=0,nonWalkableTriangles=0,stepTriangles=0;
    std::vector<ForestDeadwoodPlacement> placements; // The ten unchanged imported off-trail pieces.
    ForestDeadwoodPlacement trailRoot; // Original authored geometry, source identity 4.
    std::size_t rootVertexOffset=0,rootIndexOffset=0,rootVertices=0,rootGeometryBytes=0;
    float maximumRootRise=0,approachMinimumY=0,approachMaximumY=0;
};
bool AppendPreparedForestDeadwood(const std::filesystem::path& root,assets::StaticMeshAsset& owner,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>& support,
    ForestDeadwoodReport& report,std::string& diagnostic);
}
