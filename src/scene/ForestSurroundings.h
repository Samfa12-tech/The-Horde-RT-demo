#pragma once
#include "scene/ForestDressing.h"

namespace horde::scene {
// Immutable native preparation of the normal private chapter woodland.
// Geometry is world-baked into the existing owner/material families. No new
// texture/instance/BLAS role or ordinary omission certificate.
struct ForestSurroundingsReport {
    std::size_t extraTrees=0, addedTreeTriangles=0, fenceRuns=0, naturalBankRuns=0, arrivalPortalRuns=0, terminalFencePosts=0, openWoodlandRuns=0;
    std::size_t rockRuns=0, rockTriangles=0;
    std::size_t treeContactTriangles=0, understoryClumps=0, understoryTriangles=0;
    std::size_t fenceTriangles=0, boundaryCollisionTriangles=0, boundaryProbes=0, addedVertices=0, addedIndices=0;
    float maximumRootBurial=0;
    // All sides and the temporary demo endpoint require rendered contact proof.
    bool boundarySectionsContained=false;
    bool boundaryContained=false;
};
struct PreparedForestSurroundings {
    ForestSurroundingsReport report;
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface> terrain;
};
// Transactional: failure preserves owner, terrain and prior result. The caller
// supplies the fully prepared original scene, including its existing rocks and
// nonwalkable lanes; no original placement/collider is removed or replaced.
bool AppendPreparedForestSurroundings(const std::filesystem::path& assetRoot,
    const PreparedForestDressing& original, assets::StaticMeshAsset& owner,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>& terrain,
    std::shared_ptr<const PreparedForestSurroundings>& result, std::string& diagnostic);
}
