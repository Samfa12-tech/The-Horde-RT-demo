#pragma once
#include "scene/assets/StaticMeshAsset.h"
#include "gameplay/simulation/TriangleSupportSurface.h"
#include "gameplay/healing/FlaskUpgradeSite.h"
#include <filesystem>
#include <vector>

namespace horde::scene {
struct ForestRoadCluePlacement {
    std::array<float,3> origin{},minimum{},maximum{};
    std::vector<std::array<float,3>> bottomContacts;
    unsigned source=0;
};
struct ForestRoadClueReport {
    std::size_t triangles=0,vertexBytes=0,indexBytes=0;
    float minimumTrailClearance=0,minimumBottomGap=0,maximumBottomGap=0;
    std::vector<ForestRoadCluePlacement> placements;
};
// Original closed static meshes, existing masonry family/owner, and the same
// immutable nonwalkable contact. No new material, instance or walkable support.
bool AppendForestRoadClue(const std::filesystem::path& assetRoot,assets::StaticMeshAsset& owner,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>& support,
    ForestRoadClueReport& report,std::string& diagnostic);

struct ForestFlaskUpgradePlacement : gameplay::healing::FlaskUpgradeSite {
    float minimumBottomGap=0,maximumBottomGap=0,minimumTrailClearance=0;
};
// Source candidate only: derives a rigid placement from the actual waystone,
// native flask and final immutable ground/contact. No mesh append, collision
// change, resource readiness, entitlement or automatic gameplay admission.
bool PrepareForestFlaskUpgrade(const ForestRoadClueReport& road,
    const assets::StaticMeshAsset& flask,const gameplay::simulation::TriangleSupportSurface& terrain,
    ForestFlaskUpgradePlacement& placement,std::string& diagnostic);
}
