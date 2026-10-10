#include "gameplay/simulation/DevelopmentWorldRoute.h"
#include "gameplay/simulation/ForestTreePlacementContract.h"
#include "scene/ForestDressing.h"
#include "scene/assets/AssetManifest.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace
{

using namespace horde::scene;
using namespace horde::scene::assets;
namespace gameplay = horde::gameplay;
int failures = 0;
const std::filesystem::path kRoot{HORDE_RT_SOURCE_DIR};

void Check(bool condition, std::string_view message)
{
    if (!condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

StaticMeshAsset LoadCollapse(std::string& diagnostic)
{
    AssetManifest manifest;
    StaticMeshAsset owner;
    const auto directory = kRoot / "assets/models/world/runtime/collapsed-entry";
    Check(AssetManifest::Load(directory / "asset.manifest.json", manifest, diagnostic),
          "existing collapsed owner manifest loads");
    Check(StaticMeshAsset::Load(directory / "collapsed-entry-lod0.runtime.glb", manifest,
                                owner, diagnostic),
          "existing collapsed owner imports through the production static loader");
    return owner;
}

bool Near(float left, float right, float tolerance = 0.001f)
{
    return std::abs(left - right) <= tolerance;
}

bool FiniteBounds(const ForestDressingBounds& bounds)
{
    for (std::size_t axis = 0u; axis < 3u; ++axis)
        if (!std::isfinite(bounds.minimum[axis]) || !std::isfinite(bounds.maximum[axis]) ||
            bounds.minimum[axis] > bounds.maximum[axis]) return false;
    return true;
}

bool OverlapXZ(const ForestDressingBounds& left, const ForestDressingBounds& right)
{
    return left.minimum[0] < right.maximum[0] && right.minimum[0] < left.maximum[0] &&
           left.minimum[2] < right.maximum[2] && right.minimum[2] < left.maximum[2];
}

void TestNativeTreePairAndAppend()
{
    std::string diagnostic;
    StaticMeshAsset owner = LoadCollapse(diagnostic);
    if (owner.vertices.empty()) return;
    const auto oldVertices = owner.vertices;
    const auto oldIndices = owner.indices;
    const auto oldMaterials = owner.materials.size();
    const auto oldPrimitives = owner.primitives.size();

    ForestDressingBuildReport report;
    Check(AppendPreparedForestDressing(kRoot / "assets", owner, report, diagnostic),
          diagnostic.empty() ? "prepared tree pair imports and appends to the existing static owner"
                             : diagnostic.c_str());
    Check(report.addedTriangles == 50510u && report.addedIndices == 50510u * 3u,
          "ten authored-scale instances contribute five LOD1 pine and five LOD1 alder triangle costs");
    Check(report.placedInstances == 10u && report.addedPrimitives == 4u && report.addedMaterials == 4u,
          "ten tree placements share exactly four material-family groups on the existing owner");
    Check(owner.primitives.size() == oldPrimitives + 4u && owner.materials.size() == oldMaterials + 4u,
          "forest adds no asset owner and stays within four appended materials and primitives");
    Check(owner.primitives.size() <= 32u && owner.materials.size() <= 32u,
          "existing shared 32 primitive/material limits remain intact");
    Check(owner.vertices.size() == oldVertices.size() + report.addedVertices &&
              owner.indices.size() == oldIndices.size() + report.addedIndices,
          "append report accounts for each forest vertex and index");
    Check(std::memcmp(owner.vertices.data(), oldVertices.data(), oldVertices.size() * sizeof(oldVertices[0])) == 0 &&
              std::equal(oldIndices.begin(), oldIndices.end(), owner.indices.begin()),
          "existing collapsed-owner geometry and index prefix remain byte-identical");
    Check(FiniteBounds(report.addedBounds), "combined forest world bounds are finite and ordered");

    constexpr std::array<std::string_view, 4u> families{{
        "ForestBark", "ForestPine", "ForestMoss", "ForestAlder"}};
    constexpr std::array<std::int32_t, 4u> groups{{0, 1, 2, 3}};
    for (std::size_t i = 0u; i < families.size(); ++i)
    {
        const auto found = std::find_if(owner.materials.begin() + static_cast<std::ptrdiff_t>(oldMaterials),
            owner.materials.end(), [family = families[i]](const StaticMaterial& material) {
                return material.name == family;
            });
        Check(found != owner.materials.end(), "canonical forest material family is present");
        if (found == owner.materials.end()) continue;
        Check(found->deferTextureAllocation && found->textureGroup == groups[i] &&
                  found->baseColorTexture >= 0 && found->emissiveTexture < 0,
              "forest family uses its stable deferred atlas group without emissive sampling");
        Check((i == 0u) == (found->normalTexture >= 0) &&
                  (i == 0u) == (found->ormTexture >= 0),
              "only source-mapped bark carries original normal and roughness/ORM maps");
        Check(found->metallicFactor == 0.0f && found->transmissionFactor == 0.0f &&
                  found->emissiveFactor == std::array<float, 3u>{},
              "tree material stays opaque, dielectric, and non-emissive");
        const auto& primitive = owner.primitives[oldPrimitives + i];
        Check(primitive.materialIndex < owner.materials.size() &&
                  owner.materials[primitive.materialIndex].name == families[i] &&
                  primitive.indexCount > 0u && primitive.indexCount % 3u == 0u,
              "one bounded imported primitive is grouped for each tree material family");
    }

    std::size_t pineCount = 0u;
    std::size_t alderCount = 0u;
    for (std::size_t i = 0u; i < report.placements.size(); ++i)
    {
        const auto& placement = report.placements[i];
        const auto& contract = gameplay::simulation::kForestTreePlacementContract[i];
        Check(placement.asset != nullptr && FiniteBounds(placement.worldBounds) &&
                  FiniteBounds(placement.trunkBounds),
              "each tree has finite world and bark/trunk bounds");
        if (placement.asset == nullptr) continue;
        const auto expectedSpecies = contract.species == gameplay::simulation::ForestTreeSpecies::Pine
            ? std::string_view("pine") : std::string_view("alder");
        if (std::string_view(placement.asset) == "pine") ++pineCount;
        if (std::string_view(placement.asset) == "alder") ++alderCount;
        const auto support = gameplay::simulation::ResolveWorldRouteSupport(contract.x, contract.z);
        Check(support.grounded && Near(placement.supportY, support.worldY) &&
                  std::string_view(placement.asset) == expectedSpecies &&
                  Near(placement.origin[0], contract.x) && Near(placement.origin[2], contract.z) &&
                  Near(placement.yawRadians, contract.yawRadians) &&
                  Near(placement.worldBounds.minimum[1], support.worldY),
              "rendered tree uses the shared authored species, position, yaw, and resolved ground support");
        const float height = placement.worldBounds.maximum[1] - placement.worldBounds.minimum[1];
        const float expectedHeight = std::string_view(placement.asset) == "pine" ? 8.690f : 8.478f;
        Check(Near(height, expectedHeight, 0.025f),
              "tree retains its authored 8.69m pine or 8.48m alder scale");
        Check(Near(placement.trunkBounds.minimum[0], contract.trunkMinX, 0.002f) &&
                  Near(placement.trunkBounds.minimum[2], contract.trunkMinZ, 0.002f) &&
                  Near(placement.trunkBounds.maximum[0], contract.trunkMaxX, 0.002f) &&
                  Near(placement.trunkBounds.maximum[2], contract.trunkMaxZ, 0.002f),
              "native imported bark bounds match the shared runtime/simulation collision contract");
        for (std::size_t j = 0u; j < i; ++j)
            Check(!OverlapXZ(placement.trunkBounds, report.placements[j].trunkBounds),
                  "measured bark/trunk AABBs remain separated between adjacent placements");
    }
    Check(pineCount == 5u && alderCount == 5u,
          "the ten-point placement set alternates the original pine and alder silhouettes evenly");
    const auto& collisionTrunks = gameplay::simulation::WorldRouteForestTrunks();
    Check(collisionTrunks.size() == gameplay::simulation::kForestTreePlacementContract.size(),
          "simulation exposes one shared collision trunk for each rendered tree");
    for (std::size_t i = 0u; i < report.placements.size() && i < collisionTrunks.size(); ++i)
    {
        const auto& placement = report.placements[i];
        const auto& collision = collisionTrunks[i];
        Check(Near(collision.minimum[0], placement.trunkBounds.minimum[0], 0.002f) &&
                  Near(collision.minimum[2], placement.trunkBounds.minimum[2], 0.002f) &&
                  Near(collision.maximum[0], placement.trunkBounds.maximum[0], 0.002f) &&
                  Near(collision.maximum[2], placement.trunkBounds.maximum[2], 0.002f) &&
                  Near(collision.minimum[1], placement.supportY) &&
                  Near(collision.maximum[1], placement.supportY + 2.0f),
              "simulation blocker uses the same measured imported bark bounds and vertical interval");

        const float trunkZ = (placement.trunkBounds.minimum[2] + placement.trunkBounds.maximum[2]) * 0.5f;
        const float supportY = placement.supportY + 0.1f;
        const bool blocked = !gameplay::simulation::WorldRouteBlockoutMovementClear(
            placement.trunkBounds.minimum[0] - 0.5f, trunkZ,
            placement.trunkBounds.maximum[0] + 0.5f, trunkZ,
            supportY, 0.30f, true);
        Check(blocked, "a swept player capsule cannot pass through the native tree trunk");
        const bool canopyPasses = gameplay::simulation::WorldRouteBlockoutMovementClear(
            placement.trunkBounds.minimum[0] - 0.5f, trunkZ,
            placement.trunkBounds.maximum[0] + 0.5f, trunkZ,
            placement.supportY + 2.1f, 0.30f, true);
        Check(canopyPasses, "tree foliage and upper branches above the two-metre trunk remain visual contributors");
    }
    for (std::size_t i = 2u; i + 1u < gameplay::simulation::kWorldRoutePoints.size(); ++i)
    {
        const auto& from = gameplay::simulation::kWorldRoutePoints[i];
        const auto& to = gameplay::simulation::kWorldRoutePoints[i + 1u];
        Check(gameplay::simulation::WorldRouteBlockoutMovementClear(
                  from.x, from.z, to.x, to.z, from.y, 0.30f, true),
              "the authored route capsule sweep remains clear between forest blockers");
    }

    ForestDressingBuildReport duplicate;
    const auto countBeforeDuplicate = owner.vertices.size();
    Check(!AppendPreparedForestDressing(kRoot / "assets", owner, duplicate, diagnostic) &&
              owner.vertices.size() == countBeforeDuplicate,
          "an accidental repeated append is rejected without mutating the existing owner");
}

void TestCleanFailures()
{
    std::string diagnostic;
    StaticMeshAsset owner = LoadCollapse(diagnostic);
    if (owner.vertices.empty()) return;
    const auto oldVertices = owner.vertices;
    const auto oldIndices = owner.indices;
    ForestDressingBuildReport report;
    Check(!AppendPreparedForestDressing(kRoot / "missing-forest-root", owner, report, diagnostic) &&
              std::memcmp(owner.vertices.data(), oldVertices.data(), oldVertices.size() * sizeof(oldVertices[0])) == 0 &&
              owner.indices == oldIndices,
          "missing runtime payload fails cleanly without a partial append");

    StaticMeshAsset crowded = LoadCollapse(diagnostic);
    if (crowded.vertices.empty()) return;
    crowded.materials.resize(29u, crowded.materials.front());
    const auto crowdedVertices = crowded.vertices;
    Check(!AppendPreparedForestDressing(kRoot / "assets", crowded, report, diagnostic) &&
              crowded.vertices.size() == crowdedVertices.size() &&
              std::memcmp(crowded.vertices.data(), crowdedVertices.data(),
                          crowdedVertices.size() * sizeof(crowdedVertices[0])) == 0,
          "four-group admission fails before geometry mutation when the shared 32-material cap would overflow");
}

} // namespace

int main()
{
    TestNativeTreePairAndAppend();
    TestCleanFailures();
    if (failures == 0)
    {
        std::cout << "Native forest tree-pair import, grounding, grouping, and clean-failure checks passed.\n";
        return 0;
    }
    std::cerr << failures << " forest dressing check(s) failed.\n";
    return 1;
}
