#include "scene/TombDressing.h"

#include "scene/assets/AssetManifest.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#ifndef HORDE_RT_SOURCE_DIR
#error HORDE_RT_SOURCE_DIR must identify the source checkout for prepared tomb assets.
#endif

namespace
{

int failures = 0;

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

bool LoadCollapsedOwner(const std::filesystem::path& assetRoot,
                        horde::scene::assets::StaticMeshAsset& owner,
                        std::string& diagnostic)
{
    horde::scene::assets::AssetManifest manifest;
    const auto directory = assetRoot / "models/world/runtime/collapsed-entry";
    return horde::scene::assets::AssetManifest::Load(
               directory / "asset.manifest.json", manifest, diagnostic) &&
           horde::scene::assets::StaticMeshAsset::Load(
               directory / "collapsed-entry-lod0.runtime.glb", manifest, owner, diagnostic);
}

bool Finite(const horde::scene::assets::StaticRtVertex& vertex)
{
    return std::all_of(vertex.position.begin(), vertex.position.end(),
                       [](float value) { return std::isfinite(value); }) &&
           std::all_of(vertex.normal.begin(), vertex.normal.end(),
                       [](float value) { return std::isfinite(value); }) &&
           std::all_of(vertex.tangent.begin(), vertex.tangent.end(),
                       [](float value) { return std::isfinite(value); }) &&
           std::all_of(vertex.uv0.begin(), vertex.uv0.end(),
                       [](float value) { return std::isfinite(value); });
}

bool Near(float value, float expected, float tolerance = 0.0002f)
{
    return std::abs(value - expected) <= tolerance;
}

void TestNicheOpeningContract()
{
    using horde::scene::kTombDressingNicheOpenings;
    Check(kTombDressingNicheOpenings.size() == 3u,
          "only two entry-left niches and one outer-passage niche are admitted");
    Check(kTombDressingNicheOpenings[0].wallCoordinate == -1.85f &&
              kTombDressingNicheOpenings[1].wallCoordinate == -1.85f &&
              kTombDressingNicheOpenings[2].wallCoordinate == 6.00f,
          "cutouts target two entry left-wall positions and the outer-route east wall");
    Check(kTombDressingNicheOpenings[0].minimumZ == -1.34f &&
              kTombDressingNicheOpenings[0].maximumZ == -0.06f &&
              kTombDressingNicheOpenings[1].minimumZ == -3.24f &&
              kTombDressingNicheOpenings[1].maximumZ == -1.96f &&
              kTombDressingNicheOpenings[2].minimumZ == -13.04f &&
              kTombDressingNicheOpenings[2].maximumZ == -11.76f,
          "rectangular backing cuts match each 1.28 m niche envelope exactly");
    Check(kTombDressingNicheOpenings[0].minimumY == -0.65f &&
              kTombDressingNicheOpenings[0].maximumY == 0.95f,
          "recess openings retain 0.30 m floor clearance and 0.40 m ceiling clearance");
}

void TestPreparedDressingAppend(const std::filesystem::path& assetRoot)
{
    using namespace horde::scene;
    assets::StaticMeshAsset first, second;
    std::string diagnostic;
    Check(LoadCollapsedOwner(assetRoot, first, diagnostic),
          "existing collapsed static asset loads for owner reuse");
    if (first.vertices.empty()) return;
    Check(LoadCollapsedOwner(assetRoot, second, diagnostic),
          "second collapsed owner loads for deterministic append comparison");
    if (second.vertices.empty()) return;

    const auto oldVertices = first.vertices;
    const auto oldIndices = first.indices;
    const std::size_t oldMaterials = first.materials.size();
    const std::size_t oldPrimitives = first.primitives.size();
    TombDressingBuildReport firstReport, secondReport;
    Check(AppendPreparedTombDressing(assetRoot, first, firstReport, diagnostic),
          diagnostic.empty() ? "prepared source assets append through native static import" : diagnostic.c_str());
    Check(AppendPreparedTombDressing(assetRoot, second, secondReport, diagnostic),
          diagnostic.empty() ? "second prepared source append succeeds" : diagnostic.c_str());

    Check(firstReport.addedTriangles == 15380u && firstReport.addedIndices == 15380u * 3u,
          "sparse niche and funerary placement triangle/index cost matches prepared sources");
    Check(firstReport.placedInstances == 13u,
          "placement inventory contains three niches and ten selected source-prop instances");
    Check(firstReport.addedPrimitives == 4u && firstReport.addedMaterials == 3u,
          "geometry is grouped into bounded masonry, bone/wax, earthenware and wick factors");
    Check(first.primitives.size() == oldPrimitives + 4u && first.materials.size() == oldMaterials + 3u,
          "all dressing is appended to the existing static asset owner");
    Check(first.vertices.size() == oldVertices.size() + firstReport.addedVertices &&
              first.indices.size() == oldIndices.size() + firstReport.addedIndices,
          "append report accounts for every added vertex and index");
    Check(std::memcmp(first.vertices.data(), oldVertices.data(), oldVertices.size() * sizeof(oldVertices[0])) == 0 &&
              std::equal(oldIndices.begin(), oldIndices.end(), first.indices.begin()),
          "existing collapsed asset geometry and index prefix remain byte-for-byte unchanged");
    Check(firstReport.addedVertices == secondReport.addedVertices &&
              firstReport.addedIndices == secondReport.addedIndices &&
              std::memcmp(first.vertices.data() + oldVertices.size(),
                          second.vertices.data() + oldVertices.size(),
                          firstReport.addedVertices * sizeof(first.vertices[0])) == 0 &&
              std::equal(first.indices.begin() + static_cast<std::ptrdiff_t>(oldIndices.size()),
                         first.indices.end(),
                         second.indices.begin() + static_cast<std::ptrdiff_t>(oldIndices.size())),
          "prepared asset load, transforms and static grouping are deterministic");
    Check(first.primitives.size() <= 32u && first.materials.size() <= 32u,
          "combined owner remains within the shared 32 primitive/material capacities");

    const std::array<std::string_view, 4u> appendedMaterialNames{{
        "MedievalWall02", "TombBoneWax", "TombEarthenware", "TombCharredWick"}};
    for (std::size_t i = 0u; i < firstReport.addedPrimitives; ++i)
    {
        const auto& primitive = first.primitives[oldPrimitives + i];
        Check(primitive.materialIndex < first.materials.size(),
              "appended primitive material slot is in range");
        if (primitive.materialIndex >= first.materials.size()) continue;
        Check(std::find(appendedMaterialNames.begin(), appendedMaterialNames.end(),
                        first.materials[primitive.materialIndex].name) != appendedMaterialNames.end(),
              "appended primitive uses a declared shared static material family");
        const std::size_t vertexEnd = oldVertices.size() + firstReport.addedVertices;
        const std::size_t nextVertexOffset = i + 1u < firstReport.addedPrimitives
            ? first.primitives[oldPrimitives + i + 1u].vertexOffset
            : vertexEnd;
        Check(primitive.vertexOffset < nextVertexOffset &&
                  primitive.indexOffset + primitive.indexCount <= first.indices.size() &&
                  primitive.nodeTransformIndex < first.nodeTransforms.size(),
              "appended material group has valid spans and an identity transform record");
        for (std::size_t index = primitive.indexOffset;
             index < primitive.indexOffset + primitive.indexCount; ++index)
            Check(first.indices[index] < nextVertexOffset - primitive.vertexOffset,
                  "merged indices stay inside their grouped vertex span");
    }
    Check(std::all_of(first.vertices.begin() + static_cast<std::ptrdiff_t>(oldVertices.size()),
                      first.vertices.end(), Finite),
          "appended transformed positions, normals, tangents and UVs are finite");
    const auto laneClear = std::all_of(
        first.vertices.begin() + static_cast<std::ptrdiff_t>(oldVertices.size()), first.vertices.end(),
        [](const assets::StaticRtVertex& vertex)
        {
            return vertex.position[0] <= -0.35f || vertex.position[0] >= 5.35f;
        });
    Check(laneClear,
          "all dressing geometry remains outside the central route lane from x=-0.35 to x=5.35");
    for (const auto& name : appendedMaterialNames)
    {
        const auto found = std::find_if(first.materials.begin(), first.materials.end(),
            [name](const assets::StaticMaterial& material) { return material.name == name; });
        if (found == first.materials.end()) continue;
        Check(found->metallicFactor == 0.0f && found->transmissionFactor == 0.0f &&
                  found->emissiveFactor == std::array<float, 3u>{} &&
                  found->emissiveTexture < 0,
              "dressing material groups remain opaque, static, and nonemissive");
    }
    Check(firstReport.addedBounds.minimum[0] < -1.8f &&
              firstReport.addedBounds.maximum[0] > 6.0f &&
              firstReport.addedBounds.minimum[1] >= -0.96f &&
              firstReport.addedBounds.maximum[1] < 1.0f,
          "dressing bounds stay low and at route-wall edges rather than filling the centre lane");
    for (const auto& placement : firstReport.placementBounds)
    {
        Check(placement.asset != nullptr, "each sparse placement produces a named world-bounds record");
        if (placement.asset == nullptr) continue;
        for (std::size_t axis = 0u; axis < 3u; ++axis)
            Check(std::isfinite(placement.worldBounds.minimum[axis]) &&
                      std::isfinite(placement.worldBounds.maximum[axis]) &&
                      placement.worldBounds.minimum[axis] <= placement.worldBounds.maximum[axis],
                  "per-placement transformed bounds are finite and ordered");
    }
    for (std::size_t i = 0u; i < 3u; ++i)
    {
        const auto& niche = firstReport.placementBounds[i].worldBounds;
        Check(Near(niche.minimum[1], -0.65f) && Near(niche.maximum[1], 0.95f),
              "niche module bounds fit the 1.60 m opening below the route ceiling");
        Check(Near(niche.minimum[2], kTombDressingNicheOpenings[i].minimumZ) &&
                  Near(niche.maximum[2], kTombDressingNicheOpenings[i].maximumZ),
              "transformed niche geometry fits its exact wall cut rectangle");
    }
    Check(firstReport.placementBounds[0].worldBounds.maximum[0] <= -1.799f &&
              firstReport.placementBounds[1].worldBounds.maximum[0] <= -1.799f &&
              firstReport.placementBounds[0].worldBounds.minimum[0] < -1.85f &&
              firstReport.placementBounds[1].worldBounds.minimum[0] < -1.85f,
          "entry niches stay behind the -1.61 m capsule boundary with only 5 cm of frame reveal");
    Check(firstReport.placementBounds[2].worldBounds.minimum[0] >= 5.949f &&
              firstReport.placementBounds[2].worldBounds.maximum[0] > 6.0f,
          "outer niche stays behind the 5.76 m capsule boundary with only 5 cm of frame reveal");
    const float shelfTop = -0.3675f;
    for (std::size_t i = 3u; i <= 8u; ++i)
        Check(Near(firstReport.placementBounds[i].worldBounds.minimum[1], shelfTop),
              "skull, bones and cold candles rest on the real niche shelf surface");
    const auto& lid = firstReport.placementBounds[9u].worldBounds;
    Check(lid.minimum[0] >= -1.55f && lid.maximum[0] <= -0.72f &&
              lid.minimum[2] >= 0.05f && lid.maximum[2] <= 2.35f &&
              Near(lid.minimum[1], -0.58f),
          "displaced lid fits and rests on the existing collidable stone bier");
    for (std::size_t i = 10u; i <= 12u; ++i)
        Check(Near(firstReport.placementBounds[i].worldBounds.minimum[1], -0.95f, 0.001f),
              "offering bowl and urn fragments rest on the Keeper-corner floor");
    std::cout << "Dressing metrics: vertices=" << firstReport.addedVertices
              << " indices=" << firstReport.addedIndices
              << " triangles=" << firstReport.addedTriangles
              << " primitives=" << firstReport.addedPrimitives
              << " materials=" << firstReport.addedMaterials
              << " instances=" << firstReport.placedInstances
              << " bounds=[" << firstReport.addedBounds.minimum[0] << ','
              << firstReport.addedBounds.minimum[1] << ',' << firstReport.addedBounds.minimum[2]
              << "]..[" << firstReport.addedBounds.maximum[0] << ','
              << firstReport.addedBounds.maximum[1] << ',' << firstReport.addedBounds.maximum[2]
              << "]\n";
    for (std::size_t i = 0u; i < firstReport.placementBounds.size(); ++i)
    {
        const auto& placement = firstReport.placementBounds[i];
        if (placement.asset == nullptr) continue;
        std::cout << "Placement " << i << ' ' << placement.asset << " bounds=["
                  << placement.worldBounds.minimum[0] << ',' << placement.worldBounds.minimum[1] << ','
                  << placement.worldBounds.minimum[2] << "]..["
                  << placement.worldBounds.maximum[0] << ',' << placement.worldBounds.maximum[1] << ','
                  << placement.worldBounds.maximum[2] << "]\n";
    }

    TombDressingBuildReport repeatReport;
    Check(!AppendPreparedTombDressing(assetRoot, first, repeatReport, diagnostic),
          "the module refuses an accidental second append into the same static owner");
}

} // namespace

int main()
{
    TestNicheOpeningContract();
    TestPreparedDressingAppend(std::filesystem::path(HORDE_RT_SOURCE_DIR) / "assets");
    if (failures == 0)
    {
        std::cout << "Prepared tomb dressing import and merge checks passed.\n";
        return 0;
    }
    std::cerr << failures << " tomb dressing check(s) failed.\n";
    return 1;
}
