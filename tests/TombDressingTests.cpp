#include "scene/TombDressing.h"
#include "scene/assets/AssetManifest.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
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
    Check(kTombDressingNicheOpenings[0].minimumZ == -1.048f &&
              kTombDressingNicheOpenings[0].maximumZ == -0.352f &&
              kTombDressingNicheOpenings[1].minimumZ == -2.957f &&
              kTombDressingNicheOpenings[1].maximumZ == -2.243f &&
              kTombDressingNicheOpenings[2].minimumZ == -12.729f &&
              kTombDressingNicheOpenings[2].maximumZ == -12.071f,
          "all three backing cuts match the final per-instance niche widths");
    Check(kTombDressingNicheOpenings[0].minimumY == -0.54f &&
              kTombDressingNicheOpenings[0].maximumY == 0.285f &&
              kTombDressingNicheOpenings[1].minimumY == -0.55f &&
              kTombDressingNicheOpenings[1].maximumY == 0.35f &&
              kTombDressingNicheOpenings[2].minimumY == -0.50f &&
              kTombDressingNicheOpenings[2].maximumY == 0.275f,
          "each varied cut follows its placed niche height");
}

float NearestMasonryHit(const horde::scene::assets::StaticMeshAsset& asset,
                        const std::array<float, 3u>& origin,
                        const std::array<float, 3u>& direction,
                        const std::array<float, 3u>& translation = {},
                        float yaw = 0.0f,
                        std::size_t firstPrimitive = 0u,
                        std::size_t primitiveEnd = std::numeric_limits<std::size_t>::max())
{
    float nearest = std::numeric_limits<float>::max();
    const float cosine = std::cos(yaw), sine = std::sin(yaw);
    const auto transformed = [&](const std::array<float, 3u>& source) {
        return std::array<float, 3u>{{cosine * source[0] + sine * source[2] + translation[0],
                                      source[1] + translation[1],
                                      -sine * source[0] + cosine * source[2] + translation[2]}};
    };
    primitiveEnd = std::min(primitiveEnd, asset.primitives.size());
    for (std::size_t primitiveIndex = firstPrimitive; primitiveIndex < primitiveEnd; ++primitiveIndex)
    {
        const auto& primitive = asset.primitives[primitiveIndex];
        if (primitive.materialIndex >= asset.materials.size() ||
            asset.materials[primitive.materialIndex].name != "MedievalWall02") continue;
        for (std::size_t i = primitive.indexOffset; i + 2u < primitive.indexOffset + primitive.indexCount; i += 3u)
        {
            const auto as3 = [](const std::array<float, 4u>& p) {
                return std::array<float, 3u>{{p[0], p[1], p[2]}};
            };
            const auto a = transformed(as3(asset.vertices[primitive.vertexOffset + asset.indices[i]].position));
            const auto b = transformed(as3(asset.vertices[primitive.vertexOffset + asset.indices[i + 1u]].position));
            const auto c = transformed(as3(asset.vertices[primitive.vertexOffset + asset.indices[i + 2u]].position));
            const std::array<float, 3u> e1{b[0]-a[0], b[1]-a[1], b[2]-a[2]};
            const std::array<float, 3u> e2{c[0]-a[0], c[1]-a[1], c[2]-a[2]};
            const std::array<float, 3u> p{
                direction[1]*e2[2]-direction[2]*e2[1],
                direction[2]*e2[0]-direction[0]*e2[2],
                direction[0]*e2[1]-direction[1]*e2[0]};
            const float determinant = e1[0]*p[0] + e1[1]*p[1] + e1[2]*p[2];
            if (std::abs(determinant) < 1.0e-7f) continue;
            const float inverse = 1.0f / determinant;
            const std::array<float, 3u> tvec{origin[0]-a[0], origin[1]-a[1], origin[2]-a[2]};
            const float u = (tvec[0]*p[0] + tvec[1]*p[1] + tvec[2]*p[2]) * inverse;
            if (u < 0.0f || u > 1.0f) continue;
            const std::array<float, 3u> q{
                tvec[1]*e1[2]-tvec[2]*e1[1],
                tvec[2]*e1[0]-tvec[0]*e1[2],
                tvec[0]*e1[1]-tvec[1]*e1[0]};
            const float v = (direction[0]*q[0] + direction[1]*q[1] + direction[2]*q[2]) * inverse;
            if (v < 0.0f || u + v > 1.0f) continue;
            const float distance = (e2[0]*q[0] + e2[1]*q[1] + e2[2]*q[2]) * inverse;
            if (distance > 0.0f) nearest = std::min(nearest, distance);
        }
    }
    return nearest;
}

void TestRayHelperFixture()
{
    using namespace horde::scene::assets;
    StaticMeshAsset fixture;
    fixture.vertices.resize(3u);
    fixture.vertices[0].position = {{-1.9f, -0.5f, -1.0f, 1.0f}};
    fixture.vertices[1].position = {{-1.9f, 0.5f, -1.0f, 1.0f}};
    fixture.vertices[2].position = {{-1.9f, 0.0f, 0.0f, 1.0f}};
    fixture.indices = {0u, 1u, 2u};
    fixture.materials.push_back({});
    fixture.materials[0].name = "MedievalWall02";
    fixture.primitives.push_back({0u, 0u, 3u, 0u, 0u});
    const float hit = NearestMasonryHit(fixture, {{-1.8f, 0.0f, -0.667f}}, {{-1.0f, 0.0f, 0.0f}});
    Check(Near(hit, 0.1f, 0.001f), "CPU triangle-ray helper hits a synthetic wall face");
}

void TestOriginalNicheBaseline(const std::filesystem::path& assetRoot,
                               const std::filesystem::path& baselineRoot)
{
    using namespace horde::scene;
    struct BaselineCase
    {
        const char* id;
        const char* baselineFile;
        std::array<float, 3u> position;
        float yaw;
        std::array<float, 3u> origin;
        std::array<float, 3u> direction;
        float wall;
    };
    const std::array<BaselineCase, 3u> cases{{
        {"tomb-niche-rect", "original-rect.glb", {-2.0475f, -0.65f, -0.70f}, 1.57079632679f,
         {-1.80f, 0.02f, -0.70f}, {-1.0f, 0.0f, 0.0f}, -1.92f},
        {"tomb-niche-arched", "original-arched.glb", {-2.0475f, -0.65f, -2.60f}, 1.57079632679f,
         {-1.80f, 0.02f, -2.60f}, {-1.0f, 0.0f, 0.0f}, -1.92f},
        {"tomb-niche-rect", "original-rect.glb", {6.1975f, -0.65f, -12.40f}, -1.57079632679f,
         {5.95f, 0.02f, -12.40f}, {1.0f, 0.0f, 0.0f}, 6.0f},
    }};
    for (const auto& test : cases)
    {
        const auto manifestPath = assetRoot / "models/world/runtime/tomb-dressing-v01" /
                                  test.id / "asset.manifest.json";
        assets::AssetManifest manifest;
        assets::StaticMeshAsset imported;
        std::string diagnostic;
        Check(assets::AssetManifest::Load(manifestPath, manifest, diagnostic),
              "pinned niche manifest loads for exact original-GLB ray comparison");
        Check(assets::StaticMeshAsset::Load(baselineRoot / test.baselineFile, manifest,
                                            imported, diagnostic),
              "original pinned niche GLB imports through StaticMeshAsset for ray comparison");
        if (imported.vertices.empty()) continue;
        const float hit = NearestMasonryHit(imported, test.origin, test.direction,
                                            test.position, test.yaw);
        const float shellDistance = std::abs(test.wall - test.origin[0]);
        Check(!std::isfinite(hit) || hit >= 1.0e30f || hit <= shellDistance + 0.01f,
              "exact original pinned GLB reproduces an unsealed hidden-shell aperture ray");
        std::cout << "Original " << test.id << " aperture-ray t=" << hit
                  << " hidden-shell t=" << shellDistance << '\n';
    }
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

    Check(firstReport.addedTriangles == 17680u && firstReport.addedIndices == 17680u * 3u,
          "sparse niche, funerary, and lintel dressing triangle/index cost is bounded");
    Check(firstReport.placedInstances == 16u,
          "placement inventory contains three niches and thirteen selected source-prop instances");
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
            return vertex.position[0] <= -0.35f || vertex.position[0] >= 5.35f ||
                   vertex.position[1] > 1.08f;
        });
    Check(laneClear,
          "low dressing stays outside the route lane; only upper lintel dressing may cross it");
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
              firstReport.addedBounds.maximum[1] < 1.25f,
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
        Check(Near(niche.minimum[1], kTombDressingNicheOpenings[i].minimumY) &&
                  Near(niche.maximum[1], kTombDressingNicheOpenings[i].maximumY),
              "niche module bounds fit the reduced varied opening");
        Check(Near(niche.minimum[2], kTombDressingNicheOpenings[i].minimumZ, 0.001f) &&
                  Near(niche.maximum[2], kTombDressingNicheOpenings[i].maximumZ, 0.001f),
              "transformed niche geometry fits its exact wall cut rectangle");
    }
    const std::array<std::array<float, 3u>, 3u> rayOrigins{{
        {{-1.92f, 0.02f, -0.70f}}, {{-1.92f, 0.02f, -2.60f}}, {{6.05f, 0.02f, -12.40f}}}};
    const std::array<std::array<float, 3u>, 3u> rayDirections{{
        {{-1.0f, 0.0f, 0.0f}}, {{-1.0f, 0.0f, 0.0f}}, {{1.0f, 0.0f, 0.0f}}}};
    const std::array<float, 3u> minimumBackDistances{{0.13f, 0.13f, 0.06f}};
    for (std::size_t i = 0; i < rayOrigins.size(); ++i)
    {
        const float hit = NearestMasonryHit(first, rayOrigins[i], rayDirections[i], {}, 0.0f,
                                            oldPrimitives, oldPrimitives + firstReport.addedPrimitives);
        Check(std::isfinite(hit) && hit < 1.0e30f && hit > minimumBackDistances[i],
              "native placed module seals the real opening with an actual ray-visible back surface");
        if (!std::isfinite(hit) || hit >= 1.0e30f || hit <= minimumBackDistances[i])
            std::cout << "Niche ray " << i << " nearest masonry t=" << hit << '\n';
        else
            std::cout << "Refined niche ray " << i << " back-surface t=" << hit << '\n';
    }
    Check(firstReport.placementBounds[0].worldBounds.maximum[0] <= -1.799f &&
              firstReport.placementBounds[1].worldBounds.maximum[0] <= -1.799f &&
              firstReport.placementBounds[0].worldBounds.minimum[0] < -1.85f &&
              firstReport.placementBounds[1].worldBounds.minimum[0] < -1.85f,
          "entry niches stay behind the -1.61 m capsule boundary with only 5 cm of frame reveal");
    Check(firstReport.placementBounds[2].worldBounds.minimum[0] >= 5.949f &&
              firstReport.placementBounds[2].worldBounds.maximum[0] > 6.0f,
          "outer niche stays behind the 5.76 m capsule boundary with only 5 cm of frame reveal");
    const std::array<float, 6u> shelfTops{{kEntryRectNicheShelfTop,
        kEntryArchNicheShelfTop, kEntryArchNicheShelfTop, kEntryArchNicheShelfTop,
        kOuterNicheShelfTop, kOuterNicheShelfTop}};
    for (std::size_t i = 0u; i < shelfTops.size(); ++i)
        Check(Near(firstReport.placementBounds[i + 3u].worldBounds.minimum[1], shelfTops[i]),
              "skull, bones and cold candles rest on their varied real niche shelves");
    const auto& lid = firstReport.placementBounds[9u].worldBounds;
    Check(lid.minimum[0] >= -1.55f && lid.maximum[0] <= -0.72f &&
              lid.minimum[2] >= 0.05f && lid.maximum[2] <= 2.35f &&
              Near(lid.minimum[1], -0.58f),
          "displaced lid fits and rests on the existing collidable stone bier");
    for (std::size_t i = 10u; i <= 12u; ++i)
        Check(Near(firstReport.placementBounds[i].worldBounds.minimum[1], -0.95f, 0.001f),
              "offering bowl and urn fragments rest on the Keeper-corner floor");
    const auto& lintelShelf = firstReport.entryLintelShelfBounds;
    Check(Near(lintelShelf.minimum[0], -0.34f) && Near(lintelShelf.maximum[0], 0.34f) &&
              Near(lintelShelf.minimum[1], 1.09f) && Near(lintelShelf.maximum[1], 1.14f) &&
              Near(lintelShelf.minimum[2], -6.40f) && Near(lintelShelf.maximum[2], -6.20f),
          "entry lintel receives a shallow masonry shelf on its room-facing edge below the ceiling");
    Check(firstReport.placementBounds[13].asset != nullptr &&
              firstReport.placementBounds[14].asset != nullptr &&
              firstReport.placementBounds[15].asset != nullptr &&
              Near(firstReport.placementBounds[13].worldBounds.minimum[1], 1.14f) &&
              Near(firstReport.placementBounds[14].worldBounds.minimum[1], 1.14f) &&
              Near(firstReport.placementBounds[15].worldBounds.minimum[1], 1.14f) &&
              firstReport.placementBounds[13].worldBounds.maximum[1] < 1.25f &&
              firstReport.placementBounds[14].worldBounds.maximum[1] < 1.25f &&
              firstReport.placementBounds[15].worldBounds.maximum[1] < 1.25f &&
              firstReport.placementBounds[13].worldBounds.maximum[0] <
                  firstReport.placementBounds[14].worldBounds.minimum[0] &&
              firstReport.placementBounds[14].worldBounds.maximum[0] <
                  firstReport.placementBounds[15].worldBounds.minimum[0] &&
              firstReport.placementBounds[13].worldBounds.minimum[2] >= lintelShelf.minimum[2] &&
              firstReport.placementBounds[13].worldBounds.maximum[2] <= lintelShelf.maximum[2] &&
              firstReport.placementBounds[14].worldBounds.minimum[2] >= lintelShelf.minimum[2] &&
              firstReport.placementBounds[14].worldBounds.maximum[2] <= lintelShelf.maximum[2] &&
              firstReport.placementBounds[15].worldBounds.minimum[2] >= lintelShelf.minimum[2] &&
              firstReport.placementBounds[15].worldBounds.maximum[2] <= lintelShelf.maximum[2],
          "two small unlit candles and one offering bowl rest below the 1.35 m ceiling plane");
    const float lintelTopHit = NearestMasonryHit(first, {{0.29f, 1.30f, -6.30f}},
                                                 {{0.0f, -1.0f, 0.0f}}, {}, 0.0f,
                                                 oldPrimitives, oldPrimitives + 1u);
    Check(std::isfinite(lintelTopHit) && Near(lintelTopHit, 0.16f, 0.002f),
          "lintel lip is real native masonry triangles with a ray-visible grounded top");
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
    TestRayHelperFixture();
    const char* assetRootOverride = std::getenv("HORDE_RT_NICHE_ASSET_ROOT");
    const auto assetRoot = assetRootOverride != nullptr
        ? std::filesystem::path(assetRootOverride)
        : std::filesystem::path(HORDE_RT_SOURCE_DIR) / "assets";
    TestPreparedDressingAppend(assetRoot);
    if (const char* baseline = std::getenv("HORDE_RT_NICHE_BASELINE_DIR"); baseline != nullptr)
        TestOriginalNicheBaseline(assetRoot, baseline);
    if (failures == 0)
    {
        std::cout << "Prepared tomb dressing import and merge checks passed.\n";
        return 0;
    }
    std::cerr << failures << " tomb dressing check(s) failed.\n";
    return 1;
}
