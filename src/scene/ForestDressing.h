#pragma once

#include "scene/assets/StaticMeshAsset.h"
#include "gameplay/simulation/TriangleSupportSurface.h"
#include "gameplay/simulation/ForestTreePlacementContract.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

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
    std::vector<ForestDressingPlacement> placements; // bounded original64 or explicit prototype68 report
};

struct ForestDressingRigidPlacement
{
    std::size_t masterIndex = 0u;
    std::array<float, 3u> origin{};
    float yawRadians = 0.0f;
    float supportY = 0.0f;
};

// Own each admitted asset-local metre-space payload once (source node transforms
// are already baked by the shared GLB loader). Preparation publishes a const
// handle only after material/root admission succeeds. This is a snapshot of
// the supplied support surface: reprepare when its resource generation changes.
// It does not itself create a Vulkan instance or certify region retirement.
struct PreparedForestDressing
{
    std::array<assets::StaticMeshAsset, 2u> masters;
    std::array<assets::StaticMaterial, 4u> materialFamilies;
    std::array<std::array<std::uint8_t, 3u>, 2u> primitiveFamilies{};
    std::array<ForestDressingRigidPlacement, gameplay::simulation::kForestTreePlacementContract.size()> placements{};
};

struct ForestDressingMasterRange
{
    std::size_t primitiveOffset = 0u;
    std::size_t primitiveCount = 0u;
    ForestDressingBounds localBounds{};
};

// Diagnostic authoring receipt, not an admitted collision contract. Bounds and
// burial come from the same native ground/root measurement as the original64;
// reachable trunks still need separately matched production collision.
struct ForestDressingSupplementalPlacement
{
    ForestDressingRigidPlacement placement{};
    ForestDressingBounds worldBounds{}, trunkBounds{};
    float rootBurialMetres = 0.0f;
    std::size_t rootTipSamples = 0u;
};

// One explicit baked 68-tree candidate. Original64 and both older 12-root
// trial rosters stay unchanged; this object cannot be combined with them.
class F01NorthGrovePrototype {
public:
    const PreparedForestDressing& Prepared() const { return *prepared_; }
    const auto& Placements() const { return placements_; }
    auto Terrain() const { return terrain_; }
    std::size_t CollisionTriangles() const { return collisionTriangles_; }
private:
    F01NorthGrovePrototype()=default;
    std::shared_ptr<const PreparedForestDressing> prepared_;
    std::array<ForestDressingSupplementalPlacement,4> placements_{};
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface> terrain_;
    std::size_t collisionTriangles_=0;
    friend bool PrepareF01NorthGrovePrototype(std::shared_ptr<const PreparedForestDressing>,
        std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>,
        std::shared_ptr<const F01NorthGrovePrototype>&,std::string&);
};
bool PrepareF01NorthGrovePrototype(std::shared_ptr<const PreparedForestDressing> prepared,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface> terrain,
    std::shared_ptr<const F01NorthGrovePrototype>& result,std::string& diagnostic);
bool AppendPreparedF01NorthGrove(const F01NorthGrovePrototype& prototype,
    assets::StaticMeshAsset& owner,ForestDressingBuildReport& report,std::string& diagnostic);

// One mixed-space CPU owner: the common prefix remains world-space; each
// appended master range stays in its normalized local metre space. Keep these
// ranges separate at BLAS registration, and never consolidate them afterward.
// The const handle owns the exact terrain/root-fitted placement snapshot.
struct ForestDressingSharedLayout
{
    std::size_t commonPrimitiveCount = 0u;
    std::array<ForestDressingMasterRange, 2u> masters{};
    std::shared_ptr<const PreparedForestDressing> prepared;
    std::vector<ForestDressingSupplementalPlacement> supplementalPlacements;
};

bool PrepareForestDressing(const std::filesystem::path& assetRoot,
                          std::shared_ptr<const PreparedForestDressing>& prepared,
                          std::string& diagnostic,
                          const gameplay::simulation::TriangleSupportSurface* terrain = nullptr);

// Conservative whole-instance world AABBs for the original immutable 64.
// Encloses the local master AABB under the renderer's float rigid matrix,
// including an outward arithmetic reserve. Does not change geometry/root
// receipts; failure preserves the caller's previous complete bounds array.
bool MeasurePreparedForestInstanceBounds(const PreparedForestDressing& prepared,
    std::array<ForestDressingBounds, gameplay::simulation::kForestTreePlacementContract.size()>& bounds,
    std::string& diagnostic);

// Seed exactly the same four canonical material rows for the unchanged
// understory/deadwood builders, without creating any tree geometry.
bool SeedPreparedForestMaterialFamilies(const PreparedForestDressing& prepared,
                                       assets::StaticMeshAsset& existingOwner,
                                       std::string& diagnostic);

// Call only AFTER common world-family consolidation. Append each local master
// once, reusing the existing canonical material rows; common bounds stay world
// bounds, and the returned disjoint ranges carry their own local bounds.
// Failure leaves the owner and any prior published layout untouched.
bool AppendPreparedForestMasters(std::shared_ptr<const PreparedForestDressing> prepared,
                                 assets::StaticMeshAsset& existingOwner,
                                 ForestDressingSharedLayout& layout,
                                 std::string& diagnostic);

// One fixed, separately selected F01 composition candidate. Reuses the owned
// masters/ranges, stages all twelve receipts, and publishes only the supplement.
// The original64/root collision receipts, geometry, materials and ranges remain
// unchanged; failure preserves any earlier publication.
enum class F01BackgroundLayout { OriginalArc, GroupedR4 };
bool PrepareF01BackgroundForestDressing(ForestDressingSharedLayout& layout,
                                       const gameplay::simulation::TriangleSupportSurface& terrain,
                                        std::string& diagnostic,
                                        F01BackgroundLayout composition=F01BackgroundLayout::OriginalArc);

// Compatibility bake from the same canonical masters/transforms. Production
// remains world-baked until a separate RT ownership/capacity proof is admitted.
bool AppendPreparedForestDressing(const PreparedForestDressing& prepared,
                                 assets::StaticMeshAsset& existingOwner,
                                 ForestDressingBuildReport& report,
                                 std::string& diagnostic);

// Imports the admitted LOD1 pine and alder, bakes the authored grounded world
// placements into the existing static owner, and groups them into four
// shared PBR material families. The source models add no new asset/BLAS owner.
bool AppendPreparedForestDressing(const std::filesystem::path& assetRoot,
                                 assets::StaticMeshAsset& existingOwner,
                                 ForestDressingBuildReport& report,
                                 std::string& diagnostic,
                                 const gameplay::simulation::TriangleSupportSurface* terrain = nullptr);

} // namespace horde::scene
