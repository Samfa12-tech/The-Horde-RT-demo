#include "scene/TombDressing.h"
#include "scene/TombFloorDebris.h"
#include "scene/TombStoneBevels.h"
#include "scene/TombWallRecess.h"
#include "scene/visibility/OpaqueStaticQuadAdmission.h"
#include "scene/assets/AssetManifest.h"
#include "gameplay/traversal/RescueTraversal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
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

void TestNativeNicheWallSeams(const horde::scene::assets::StaticMeshAsset& native)
{
    using namespace horde::scene;
    // Actual production panel corners, native world-baked triangles, and exact
    // failing endpoints. A bounds box must not stand in for a stone surface.
    assets::StaticMeshAsset wall;
    wall.materials.push_back({});
    wall.materials[0].name="MedievalWall02";
    const auto append=[&](float x,float logicalX,float lowZ,float highZ,float lowY,float highY) {
        for(const auto& panel:TombWallPanels(logicalX,lowZ,highZ,lowY,highY,true)) {
            const auto points=TombWallPanelVertices(panel,x);
            const auto base=static_cast<std::uint32_t>(wall.vertices.size());
            for(const auto& point:points) {
                assets::StaticRtVertex vertex{};
                vertex.position={{point[0],point[1],point[2],1.f}};
                wall.vertices.push_back(vertex);
            }
            wall.indices.insert(wall.indices.end(),{base,base+1,base+2,base,base+2,base+3});
        }
    };
    append(-1.85f,-1.85f,-6.4f,3.4f,-.95f,1.35f);
    append(-1.92f,-1.85f,-6.47f,3.4f,-1.02f,1.42f);
    append(6.f,6.f,-15.2f,-10.f,-.95f,1.35f);
    append(6.07f,6.f,-15.2f,-10.f,-1.02f,1.42f);
    wall.primitives.push_back({0u,0u,static_cast<std::uint32_t>(wall.indices.size()),0u,0u});
    struct Segment { const char* name;std::array<float,3u> start,end; };
    const std::array<Segment,4u> reproduced{{
        {"rect low-Z seam",{{-1.84f,0,-1.0479f}},{{-2.17f,0,-1.0479f}}},
        {"rect high-Z seam",{{-1.84f,0,-.3521f}},{{-2.17f,0,-.3521f}}},
        {"arch upward seam",{{-1.7f,.19f,-2.6f}},{{-2.25f,.5475f,-2.6f}}},
        {"east edge control",{{5.99f,0,-12.72895f}},{{6.32f,0,-12.72895f}}},
    }};
    for(const auto& ray:reproduced) {
        std::array<float,3u> direction{};
        for(unsigned axis=0;axis<3;++axis)direction[axis]=ray.end[axis]-ray.start[axis];
        const float hit=std::min(NearestMasonryHit(native,ray.start,direction),
                                 NearestMasonryHit(wall,ray.start,direction));
        Check(std::isfinite(hit) && hit>0 && hit<1,ray.name);
        std::cout<<"Native niche seam "<<ray.name<<" first hit fraction="<<hit<<'\n';
    }
    // Join regressions also use the real imported frame, independently of the
    // procedural wall. A ray from either wall plane toward the room must hit
    // the frame on every cut edge; an enclosing bounds box would miss the
    // arch crown and shelf/jamb defects reproduced above.
    std::size_t frameChecks=0;
    for(std::size_t niche=0;niche<kTombWallCuts.size();++niche) {
        const auto& cut=kTombWallCuts[niche];
        std::vector<std::array<float,2u>> boundary;
        boundary.push_back({cut.minimumZ,cut.minimumY});
        boundary.push_back({cut.maximumZ,cut.minimumY});
        for(std::size_t i=cut.topPointCount;i>0;--i)boundary.push_back(cut.topZY[i-1]);
        const float visibleX=kTombDressingNicheOpenings[niche].wallCoordinate;
        const float facing=niche==2 ? -1.f : 1.f;
        for(const float plane:{visibleX,visibleX-facing*.07f}) {
            const auto checkFrame=[&](float z,float y) {
                const float hit=NearestMasonryHit(native,{{plane,y,z}},{{facing,0,0}});
                Check(std::isfinite(hit) && hit>0 && hit<.15f,
                      "native module covers the wall-cut perimeter on both wall planes");
                ++frameChecks;
            };
            for(std::size_t edge=0;edge<boundary.size();++edge) {
                const auto& a=boundary[edge];const auto& b=boundary[(edge+1)%boundary.size()];
                for(const float fraction:{0.f,.25f,.5f,.75f})
                    checkFrame(a[0]+fraction*(b[0]-a[0]),a[1]+fraction*(b[1]-a[1]));
            }
            if(niche==1) {
                checkFrame(-2.798f,cut.minimumY);
                checkFrame(-2.402f,cut.minimumY);
            }
        }
    }
    std::cout<<"Native fitted-frame perimeter ray checks: "<<frameChecks<<'\n';
}

void TestImportedNicheFaceNormals(const std::filesystem::path& assetRoot)
{
    using namespace horde::scene::assets;
    const auto directory = assetRoot / "models/world/runtime/tomb-dressing-v01/tomb-niche-arched";
    AssetManifest manifest;
    StaticMeshAsset imported;
    std::string diagnostic;
    Check(AssetManifest::Load(directory / "asset.manifest.json", manifest, diagnostic),
          "arched niche manifest loads for native winding inspection");
    Check(StaticMeshAsset::Load(directory / "tomb-niche-arched-lod0.runtime.glb", manifest,
                                imported, diagnostic),
          "arched niche GLB imports through the production native static-mesh loader");
    if (imported.vertices.empty()) return;

    std::size_t visibleArchTriangles = 0u;
    std::size_t opposedNormals = 0u;
    std::size_t backFacingArchTriangles = 0u;
    float minimumAgreement = 1.0f;
    const auto normalize = [](std::array<float, 3u>& value) {
        const float length = std::sqrt(value[0] * value[0] + value[1] * value[1] + value[2] * value[2]);
        if (!std::isfinite(length) || length <= 1.0e-8f) return false;
        for (float& component : value) component /= length;
        return true;
    };
    for (const auto& primitive : imported.primitives)
    {
        if (primitive.materialIndex >= imported.materials.size() ||
            imported.materials[primitive.materialIndex].name != "MedievalWall02" ||
            primitive.nodeTransformIndex >= imported.nodeTransforms.size() ||
            imported.nodeTransforms[primitive.nodeTransformIndex].name.find("arch_voussoir_") != 0u) continue;
        float maxZ = std::numeric_limits<float>::lowest();
        std::size_t vertexEnd = imported.vertices.size();
        for (const auto& next : imported.primitives)
            if (next.vertexOffset > primitive.vertexOffset)
                vertexEnd = std::min(vertexEnd, static_cast<std::size_t>(next.vertexOffset));
        for (std::size_t v = primitive.vertexOffset; v < vertexEnd; ++v)
        {
            const auto& point = imported.vertices[v].position;
            maxZ = std::max(maxZ, point[2]);
        }
        for (std::size_t index = primitive.indexOffset;
             index + 2u < primitive.indexOffset + primitive.indexCount; index += 3u)
        {
            const auto& a = imported.vertices[primitive.vertexOffset + imported.indices[index]];
            const auto& b = imported.vertices[primitive.vertexOffset + imported.indices[index + 1u]];
            const auto& c = imported.vertices[primitive.vertexOffset + imported.indices[index + 2u]];
            const float centerX = (a.position[0] + b.position[0] + c.position[0]) / 3.0f;
            // Test the actual front plane of each imported voussoir. A
            // geometric-normal filter would hide precisely the reversed faces
            // this regression guards against.
            if (std::abs(centerX) > 0.49f ||
                std::abs(a.position[2] - maxZ) > 1.0e-4f ||
                std::abs(b.position[2] - maxZ) > 1.0e-4f ||
                std::abs(c.position[2] - maxZ) > 1.0e-4f) continue;
            std::array<float, 3u> edge1{{b.position[0] - a.position[0],
                                         b.position[1] - a.position[1],
                                         b.position[2] - a.position[2]}};
            std::array<float, 3u> edge2{{c.position[0] - a.position[0],
                                         c.position[1] - a.position[1],
                                         c.position[2] - a.position[2]}};
            auto geometricNormal = std::array<float, 3u>{{
                edge1[1] * edge2[2] - edge1[2] * edge2[1],
                edge1[2] * edge2[0] - edge1[0] * edge2[2],
                edge1[0] * edge2[1] - edge1[1] * edge2[0]}};
            auto shadingNormal = std::array<float, 3u>{{
                a.normal[0] + b.normal[0] + c.normal[0],
                a.normal[1] + b.normal[1] + c.normal[1],
                a.normal[2] + b.normal[2] + c.normal[2]}};
            if (!normalize(geometricNormal) || !normalize(shadingNormal)) continue;
            if (std::abs(geometricNormal[2]) < 0.90f) continue;
            const float agreement = geometricNormal[0] * shadingNormal[0] +
                                    geometricNormal[1] * shadingNormal[1] +
                                    geometricNormal[2] * shadingNormal[2];
            ++visibleArchTriangles;
            minimumAgreement = std::min(minimumAgreement, agreement);
            if (geometricNormal[2] < 0.90f) ++backFacingArchTriangles;
            if (agreement <= 0.0f) ++opposedNormals;
        }
    }
    Check(visibleArchTriangles >= 14u,
          "native-imported arched niche exposes a useful set of room-facing voussoir triangles");
    Check(backFacingArchTriangles == 0u && opposedNormals == 0u,
          "imported room-facing arch faces point into the room and agree with their shading normals");
    std::cout << "Imported arch front faces: triangles=" << visibleArchTriangles
              << " back-facing=" << backFacingArchTriangles << " opposed-normal=" << opposedNormals
              << " minimum-normal-dot=" << minimumAgreement << '\n';
    if (opposedNormals != 0u)
        std::cout << "Arched niche normal diagnostics: frontTriangles=" << visibleArchTriangles
                  << " backFacing=" << backFacingArchTriangles << " opposed=" << opposedNormals
                  << " minimumDot=" << minimumAgreement << '\n';
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
    // Preserve the original imported dressing fixture independently of the new
    // architectural replacements; its low-prop/grounding checks remain strict.
    Check(AppendPreparedTombDressing(assetRoot, first, firstReport, diagnostic, false),
          diagnostic.empty() ? "prepared source assets append through native static import" : diagnostic.c_str());
    Check(AppendPreparedTombDressing(assetRoot, second, secondReport, diagnostic, false),
          diagnostic.empty() ? "second prepared source append succeeds" : diagnostic.c_str());

    TestNativeNicheWallSeams(first);
    const std::array<float,12> identity{{1,0,0,0,0,1,0,0,0,0,1,0}};
    std::size_t realQuads=0,boneWitnesses=0;
    for(std::size_t p=oldPrimitives;p<first.primitives.size();++p) {
        const auto& primitive=first.primitives[p];
        for(std::size_t i=0;i+6<=primitive.indexCount;i+=6) {
            const auto witness=horde::scene::visibility::AdmitRetainedStaticQuad(first,p,i,identity,0x01,1,1,true);
            if(!witness) continue;
            ++realQuads;
            if(first.materials[primitive.materialIndex].name=="TombBoneWax") ++boneWitnesses;
        }
    }
    Check(realQuads>0,"native imported and world-baked retained masonry must supply actual opaque quad witnesses");
    Check(boneWitnesses==0,"candidate TombDressing triangles must never serve as their own retained witness");
    std::cout<<"Native retained dressing quad witnesses: "<<realQuads<<"; BoneWax witnesses: "<<boneWitnesses<<'\n';

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
    const auto& firstRectOffering = firstReport.placementBounds[3u].worldBounds;
    Check(Near((firstRectOffering.minimum[2] + firstRectOffering.maximum[2]) * 0.5f,
               (kTombDressingNicheOpenings[0].minimumZ +
                kTombDressingNicheOpenings[0].maximumZ) * 0.5f, 0.01f),
          "the first rectangular niche candle is centered on the real shelf");
    const auto& outerCandleA = firstReport.placementBounds[7u].worldBounds;
    const auto& outerCandleB = firstReport.placementBounds[8u].worldBounds;
    Check(outerCandleA.maximum[2] < outerCandleB.minimum[2] &&
              outerCandleB.minimum[2] - outerCandleA.maximum[2] < 0.11f &&
              Near((std::min(outerCandleA.minimum[2], outerCandleB.minimum[2]) +
                    std::max(outerCandleA.maximum[2], outerCandleB.maximum[2])) * 0.5f,
                   (kTombDressingNicheOpenings[2].minimumZ +
                    kTombDressingNicheOpenings[2].maximumZ) * 0.5f, 0.01f) &&
              Near(outerCandleA.minimum[1], kOuterNicheShelfTop) &&
              Near(outerCandleB.minimum[1], kOuterNicheShelfTop),
          "outer rectangular niche candles form a close, centered cluster grounded on its shelf");
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

void TestStoneArchitecture(const std::filesystem::path& assetRoot)
{
    using namespace horde::scene;
    assets::StaticMeshAsset asset; std::string diagnostic; TombDressingBuildReport report;
    Check(LoadCollapsedOwner(assetRoot, asset, diagnostic), "load existing masonry owner for stone finish");
    const auto firstPrimitive=asset.primitives.size();
    const auto oldMaterialCount=asset.materials.size();
    Check(AppendPreparedTombDressing(assetRoot,asset,report,diagnostic,true,false), diagnostic.c_str());
    Check(report.bevelTriangles==656 && report.addedTriangles==17680+656,
          "twelve bevelled solids and sixteen corner joins add exactly 656 triangles");
    Check(asset.primitives.size()==firstPrimitive+4 && asset.materials.size()==oldMaterialCount+3,
          "bevels share admitted masonry without another primitive/material/texture owner");
    Check(Near(NearestMasonryHit(asset,{{-0.915f,0,-6}},{{0,0,-1}}, {},0,firstPrimitive,firstPrimitive+1),0.295f),
          "entry jamb has a true cut 30 mm bevel");
    Check(Near(NearestMasonryHit(asset,{{1.835f,0,-6}},{{0,0,-1}}, {},0,firstPrimitive,firstPrimitive+1),0.385f),
          "meeting entry walls have a real diagonal stone corner");
    Check(NearestMasonryHit(asset,{{0,0,-6}},{{0,0,-1}}, {},0,firstPrimitive,firstPrimitive+1)>2.0f,
          "bevel architecture leaves the entrance walking lane open");
    Check(Near(NearestMasonryHit(asset,{{1.14f,0,-7}},{{0,0,1}}, {},0,firstPrimitive,firstPrimitive+1),0.53f),
          "backtracking sees a real closed return behind the former black paper-wall edge");

    for(const auto& box:TombBevelBoxes())
    {
        std::vector<assets::StaticRtVertex> vertices; std::vector<std::uint32_t> indices;
        AppendChamferedStoneBox(vertices,indices,box);
        Check(indices.size()==132,"closed single-segment bevel box has 44 triangles");
        // Face vertices are split for correct flat normals. Weld only in this
        // topology check, to prove each geometric edge has two opposite uses.
        using Key=std::array<long long,3>; using Edge=std::pair<Key,Key>;
        std::map<Edge,std::pair<int,int>> edges;
        const auto key=[](const auto& v) { return Key{std::llround(v.position[0]*100000),
            std::llround(v.position[1]*100000),std::llround(v.position[2]*100000)}; };
        for(std::size_t i=0;i<indices.size();i+=3)
        {
            const auto& a=vertices[indices[i]];const auto& b=vertices[indices[i+1]];const auto& c=vertices[indices[i+2]];
            using namespace stone_bevel;
            const Point pa{a.position[0],a.position[1],a.position[2]},pb{b.position[0],b.position[1],b.position[2]},pc{c.position[0],c.position[1],c.position[2]};
            const auto face=Cross(Sub(pb,pa),Sub(pc,pa));
            Check(Dot(face,face)>1e-10f && Dot(Unit(face),{a.normal[0],a.normal[1],a.normal[2]})>0.9999f,
                  "every chamfer has nondegenerate winding and matching geometric/shading normals");
            for(unsigned k=0;k<3;++k)
            { auto from=key(vertices[indices[i+k]]),to=key(vertices[indices[i+(k+1)%3]]);const int direction=from<to?1:-1;
              if(to<from)std::swap(from,to);auto& edge=edges[{from,to}];++edge.first;edge.second+=direction; }
        }
        for(const auto& entry:edges) Check(entry.second.first==2 && entry.second.second==0,
            "stone bevel is watertight with no missing faces or reversed shared edges");
        for(const auto& v:vertices)
        {
            Check(Finite(v),"bevel PBR vertex is finite");
            for(unsigned axis=0;axis<3;++axis) Check(v.position[axis]>=box.minimum[axis]-0.00001f &&
                v.position[axis]<=box.maximum[axis]+0.00001f,"cut bevel never reduces existing passage/capsule clearance");
            Check(std::abs(v.normal[0]*v.tangent[0]+v.normal[1]*v.tangent[1]+v.normal[2]*v.tangent[2])<0.0001f,
                "bevel normal-map tangent remains orthogonal");
        }
    }
    std::vector<assets::StaticRtVertex> badVertices;std::vector<std::uint32_t> badIndices;
    for(const float radius:{0.0f,-0.1f,2.0f,std::numeric_limits<float>::quiet_NaN()})
    {
        bool rejected=false;
        try { AppendChamferedStoneBox(badVertices,badIndices,TombBevelBoxes()[0],radius); }
        catch(const std::invalid_argument&) { rejected=true; }
        Check(rejected && badVertices.empty() && badIndices.empty(),"invalid bevel radius rejects before any partial append");
    }
}

template<class T> bool SameBytes(const std::vector<T>& a,const std::vector<T>& b)
{
    return a.size()==b.size() && (a.empty() || std::memcmp(a.data(),b.data(),a.size()*sizeof(T))==0);
}

bool SameMaterial(const horde::scene::assets::StaticMaterial& a,
                  const horde::scene::assets::StaticMaterial& b)
{
    const auto fields=[](const auto& m) { return std::tie(m.name,m.baseColorFactor,m.emissiveFactor,
        m.attenuationColor,m.emissiveStrength,m.metallicFactor,m.roughnessFactor,m.occlusionStrength,
        m.normalScale,m.textureScale,m.transmissionFactor,m.ior,m.thicknessFactor,m.attenuationDistance,
        m.numericalSpawnMinimumWidth,m.numericalSpawnGeometryError,m.baseColorTexture,m.normalTexture,
        m.ormTexture,m.emissiveTexture,m.textureGroup,m.deferTextureAllocation,m.flags); };
    return fields(a)==fields(b);
}

bool SameOwner(const horde::scene::assets::StaticMeshAsset& a,const horde::scene::assets::StaticMeshAsset& b)
{
    if(!SameBytes(a.vertices,b.vertices) || a.indices!=b.indices || a.triangleRegions!=b.triangleRegions ||
       a.bounds.minimum!=b.bounds.minimum || a.bounds.maximum!=b.bounds.maximum ||
       a.primitives.size()!=b.primitives.size() || a.materials.size()!=b.materials.size() ||
       a.nodeTransforms.size()!=b.nodeTransforms.size() || a.sockets.size()!=b.sockets.size()) return false;
    for(std::size_t i=0;i<a.primitives.size();++i) {
        const auto& x=a.primitives[i];const auto& y=b.primitives[i];
        if(std::tie(x.vertexOffset,x.indexOffset,x.indexCount,x.materialIndex,x.nodeTransformIndex)!=
           std::tie(y.vertexOffset,y.indexOffset,y.indexCount,y.materialIndex,y.nodeTransformIndex))return false;
    }
    for(std::size_t i=0;i<a.materials.size();++i)if(!SameMaterial(a.materials[i],b.materials[i]))return false;
    for(std::size_t i=0;i<a.nodeTransforms.size();++i)
        if(a.nodeTransforms[i].name!=b.nodeTransforms[i].name || a.nodeTransforms[i].world!=b.nodeTransforms[i].world)return false;
    for(std::size_t i=0;i<a.sockets.size();++i)
        if(a.sockets[i].name!=b.sockets[i].name || a.sockets[i].nodeTransformIndex!=b.sockets[i].nodeTransformIndex ||
           a.sockets[i].world!=b.sockets[i].world)return false;
    return true;
}

void TestFloorFragmentGeometry()
{
    using namespace horde::scene;
    using namespace stone_bevel;
    Check(kTombFloorFragments.size()==6,"debris is a fixed six-fragment pocket");
    std::vector<assets::StaticRtVertex> all,again;std::vector<std::uint32_t> allIndices,againIndices;
    AppendTombFloorDebris(all,allIndices);AppendTombFloorDebris(again,againIndices);
    Check(allIndices.size()==72u*3u && all.size()==72u*3u && SameBytes(all,again) && allIndices==againIndices,
          "six closed chips have exactly 72 deterministic triangles and split PBR vertices");
    for(const auto& stone:kTombFloorFragments) {
        std::vector<assets::StaticRtVertex> vertices;std::vector<std::uint32_t> indices;
        AppendStoneFragment(vertices,indices,stone);
        Check(indices.size()==36 && vertices.size()==36,"one fractured chip has twelve triangles");
        using Key=std::array<long long,3>;using Edge=std::pair<Key,Key>;
        std::map<Edge,std::pair<int,int>> edges;
        const auto key=[](const auto& v){return Key{std::llround(v.position[0]*100000),
            std::llround(v.position[1]*100000),std::llround(v.position[2]*100000)};};
        const Point inside{stone.x,horde::gameplay::kRouteFloorWorldY-.001f+stone.height*.3f,stone.z};
        float minimumY=INFINITY,maximumY=-INFINITY;
        for(std::size_t i=0;i<indices.size();i+=3) {
            const auto& a=vertices.at(indices[i]);const auto& b=vertices.at(indices[i+1]);const auto& c=vertices.at(indices[i+2]);
            const Point pa{a.position[0],a.position[1],a.position[2]},pb{b.position[0],b.position[1],b.position[2]},
                pc{c.position[0],c.position[1],c.position[2]};
            const auto face=Cross(Sub(pb,pa),Sub(pc,pa));
            Check(Dot(face,face)>1e-12f && Dot(face,Sub(pa,inside))>0 &&
                  Dot(Unit(face),{a.normal[0],a.normal[1],a.normal[2]})>.9999f,
                  "chip triangles are nondegenerate, outward and agree with their shading normals");
            for(unsigned k=0;k<3;++k) {
                auto from=key(vertices[indices[i+k]]),to=key(vertices[indices[i+(k+1)%3]]);
                const int direction=from<to?1:-1;if(to<from)std::swap(from,to);
                auto& edge=edges[{from,to}];++edge.first;edge.second+=direction;
            }
        }
        for(const auto& [edge,uses]:edges)Check(uses.first==2 && uses.second==0,
            "welded fragment edges have exactly two opposite uses, including the closed bottom");
        for(const auto& v:vertices) {
            const Point n{v.normal[0],v.normal[1],v.normal[2]},t{v.tangent[0],v.tangent[1],v.tangent[2]};
            const float dx=v.position[0]-stone.x,dz=v.position[2]-stone.z;
            const float localX=std::cos(stone.yaw)*dx-std::sin(stone.yaw)*dz,
                localZ=std::sin(stone.yaw)*dx+std::cos(stone.yaw)*dz;
            Check(Finite(v) && Near(Dot(n,n),1,1e-4f) && Near(Dot(t,t),1,1e-4f) &&
                  std::abs(Dot(n,t))<1e-4f && std::abs(v.tangent[3])==1,
                  "each chip has finite UVs and normalized orthogonal tangent frames");
            Check(std::abs(localX)<=stone.width*.5f+1e-5f && std::abs(localZ)<=stone.length*.5f+1e-5f,
                  "yawed chip stays inside its declared dimensions");
            minimumY=std::min(minimumY,v.position[1]);maximumY=std::max(maximumY,v.position[1]);
        }
        Check(Near(minimumY,horde::gameplay::kRouteFloorWorldY-.001f,1e-5f) &&
              Near(maximumY-minimumY,stone.height,1e-5f) && stone.height<=.10f,
              "chip bottom is grounded with one millimetre overlap and bounded relief");
    }
    const auto valid=kTombFloorFragments.front();
    std::vector<StoneFragmentSpec> invalid;
    for(unsigned component=0;component<6;++component)for(float bad:{INFINITY,std::numeric_limits<float>::quiet_NaN()}) {
        auto s=valid;std::array<float*,6> fields{&s.x,&s.z,&s.width,&s.length,&s.height,&s.yaw};
        *fields[component]=bad;invalid.push_back(s);
    }
    for(unsigned dimension=0;dimension<3;++dimension)for(float bad:{0.f,-.01f,dimension==2?.101f:.251f}) {
        auto s=valid;std::array<float*,3> fields{&s.width,&s.length,&s.height};
        *fields[dimension]=bad;invalid.push_back(s);
    }
    for(const auto& bad:invalid) {
        auto vertices=all;auto indices=allIndices;bool rejected=false;
        try{AppendStoneFragment(vertices,indices,bad);}catch(const std::invalid_argument&){rejected=true;}
        Check(rejected && SameBytes(vertices,all) && indices==allIndices,
              "nonfinite, nonpositive or oversized fragment rejects without changing an existing prefix");
    }
}

void TestFloorDebrisOwner(const std::filesystem::path& assetRoot)
{
    using namespace horde::scene;
    assets::StaticMeshAsset original;std::string diagnostic;
    if(!LoadCollapsedOwner(assetRoot,original,diagnostic)){Check(false,"load owner for debris admission");return;}
    auto off=original,on=original,repeat=original,legacyOff=original,legacyOn=original;
    TombDressingBuildReport offReport,onReport,repeatReport,legacyOffReport,legacyOnReport;
    const bool prepared=AppendPreparedTombDressing(assetRoot,off,offReport,diagnostic,true,false) &&
        AppendPreparedTombDressing(assetRoot,on,onReport,diagnostic,true,true) &&
        AppendPreparedTombDressing(assetRoot,repeat,repeatReport,diagnostic,true,true) &&
        AppendPreparedTombDressing(assetRoot,legacyOff,legacyOffReport,diagnostic,false,false) &&
        AppendPreparedTombDressing(assetRoot,legacyOn,legacyOnReport,diagnostic,false,true);
    Check(prepared,diagnostic.c_str());if(!prepared)return;
    Check(SameOwner(on,repeat),"complete debris-enabled owner admission is deterministic");
    Check(SameOwner(legacyOff,legacyOn) && legacyOnReport.floorDebrisTriangles==0 &&
          legacyOnReport.addedTriangles==17680,"legacy architecture=false fixture omits debris independently of its detail flag");
    Check(offReport.floorDebrisTriangles==0 && offReport.addedTriangles==17680+656 &&
          onReport.bevelTriangles==656 && onReport.floorDebrisTriangles==72 &&
          onReport.addedTriangles==offReport.addedTriangles+72 && on.indices.size()==off.indices.size()+216 &&
          on.vertices.size()==off.vertices.size()+216,"detail switch preserves old architecture cost and adds exactly 72 triangles");
    Check(on.primitives.size()==off.primitives.size() && on.materials.size()==off.materials.size() &&
          on.nodeTransforms.size()==off.nodeTransforms.size() && onReport.placedInstances==offReport.placedInstances &&
          onReport.addedPrimitives==offReport.addedPrimitives && onReport.addedMaterials==offReport.addedMaterials,
          "chips reuse the admitted masonry primitive/material/transform without adding prop owners");
    if(on.primitives.size()!=off.primitives.size() || on.materials.size()!=off.materials.size())return;
    for(std::size_t i=0;i<off.materials.size();++i)Check(SameMaterial(on.materials[i],off.materials[i]),
        "all PBR factors, texture references and material families remain exact with detail on");
    Check(std::memcmp(on.vertices.data(),original.vertices.data(),original.vertices.size()*sizeof(original.vertices[0]))==0 &&
          std::equal(original.indices.begin(),original.indices.end(),on.indices.begin()),
          "original collapsed owner vertex/index prefix survives debris admission byte-exact");
    std::vector<assets::StaticRtVertex> expected;std::vector<std::uint32_t> expectedIndices;
    AppendTombFloorDebris(expected,expectedIndices);
    const auto masonry=original.primitives.size();
    for(std::size_t p=0;p<off.primitives.size();++p) {
        const auto& a=off.primitives[p];const auto& b=on.primitives[p];
        const auto end=p+1<off.primitives.size()?off.primitives[p+1].vertexOffset:off.vertices.size();
        const auto count=end-a.vertexOffset;
        Check(a.materialIndex==b.materialIndex && a.nodeTransformIndex==b.nodeTransformIndex &&
              b.indexCount==a.indexCount+(p==masonry?216u:0u) &&
              std::memcmp(off.vertices.data()+a.vertexOffset,on.vertices.data()+b.vertexOffset,count*sizeof(off.vertices[0]))==0 &&
              std::equal(off.indices.begin()+a.indexOffset,off.indices.begin()+a.indexOffset+a.indexCount,on.indices.begin()+b.indexOffset),
              "every old material group retains exact vertex/index content despite suffix offset shifts");
        for(std::size_t t=0;t<a.indexCount/3;++t)Check(off.triangleRegions[a.indexOffset/3+t]==on.triangleRegions[b.indexOffset/3+t],
            "every pre-existing triangle retains its region provenance");
        if(p!=masonry)continue;
        Check(on.materials[b.materialIndex].name=="MedievalWall02" &&
              std::memcmp(on.vertices.data()+b.vertexOffset+count,expected.data(),expected.size()*sizeof(expected[0]))==0,
              "new fragment geometry lives in the existing masonry group");
        for(std::size_t i=0;i<expectedIndices.size();++i)Check(on.indices[b.indexOffset+a.indexCount+i]==count+expectedIndices[i],
            "masonry fragment suffix rebases all local indices exactly once");
        for(std::size_t t=0;t<72;++t)Check(on.triangleRegions[(b.indexOffset+a.indexCount)/3+t]==assets::StaticTriangleRegion::TombDressing,
            "decorative chips remain TombDressing even when sharing structural masonry material");
    }
    TombDressingBounds actual;actual.minimum.fill(INFINITY);actual.maximum.fill(-INFINITY);
    for(const auto& v:expected)for(unsigned axis=0;axis<3;++axis) {
        actual.minimum[axis]=std::min(actual.minimum[axis],v.position[axis]);actual.maximum[axis]=std::max(actual.maximum[axis],v.position[axis]);
    }
    Check(actual.minimum==onReport.floorDebrisBounds.minimum && actual.maximum==onReport.floorDebrisBounds.maximum,
          "debris report bounds exactly cover the generated world-space vertices");
    std::cout<<"Floor debris metrics: triangles="<<onReport.floorDebrisTriangles
        <<" vertices="<<on.vertices.size()-off.vertices.size()<<" indices="<<on.indices.size()-off.indices.size()
        <<" vertexBytes="<<(on.vertices.size()-off.vertices.size())*sizeof(assets::StaticRtVertex)
        <<" indexBytes="<<(on.indices.size()-off.indices.size())*sizeof(std::uint32_t)
        <<" primitivesDelta="<<on.primitives.size()-off.primitives.size()<<" materialsDelta="<<on.materials.size()-off.materials.size()
        <<" bounds=["<<actual.minimum[0]<<','<<actual.minimum[1]<<','<<actual.minimum[2]<<"]-["
        <<actual.maximum[0]<<','<<actual.maximum[1]<<','<<actual.maximum[2]<<"]\n";
    Check(actual.minimum[0]>1.60f && actual.maximum[0]<1.85f && actual.minimum[2]>3.10f && actual.maximum[2]<3.39f,
          "bounded pocket stays in front of the actual collapse wall below its right fractured shoulder");
    // Cosmetic chips are not promised to avoid every possible perimeter foot.
    // Prove the accepted spawn/central route, chest approach and durable lower
    // checkpoint paths instead, using conservative swept-capsule AABBs.
    using horde::gameplay::RoutePosition;
    const auto lower=horde::gameplay::traversal::kLowerLanding;
    const auto upper=horde::gameplay::traversal::kExteriorLanding;
    const std::array<RoutePosition,18> route{{
        {0,2.9f},horde::gameplay::kPlayerSpawn,{0,-1},{0,-4.8f},{0,-9.4f},{4.2f,-9.4f},
        {4.2f,-14.6f},{-2,-14.6f},horde::gameplay::kSkylightChamberCenter,
        horde::gameplay::kTorchBayCenters[0],horde::gameplay::kTorchBayCenters[1],
        horde::gameplay::kTorchBayCenters[2],horde::gameplay::kTorchBayCenters[3],
        horde::gameplay::kTransmissionThresholdCenter,horde::gameplay::kFinaleCenter,
        {horde::gameplay::kRewardChestRoutePosition.x+1.30f,horde::gameplay::kRewardChestRoutePosition.z},
        {lower.x,lower.z},{upper.x,upper.z}
    }};
    for(std::size_t i=1;i<route.size();++i) {
        const float lowX=std::min(route[i-1].x,route[i].x),highX=std::max(route[i-1].x,route[i].x),
            lowZ=std::min(route[i-1].z,route[i].z),highZ=std::max(route[i-1].z,route[i].z);
        const float gapX=std::max({lowX-actual.maximum[0],actual.minimum[0]-highX,0.f}),
            gapZ=std::max({lowZ-actual.maximum[2],actual.minimum[2]-highZ,0.f});
        Check(std::hypot(gapX,gapZ)>horde::gameplay::kPlayerCollisionRadius,
              "rendered pocket clears the accepted central route, grate/bier passage, combat and chest/checkpoint approaches");
    }
    for(unsigned mode=0;mode<6;++mode) {
        auto bad=original;
        if(mode==0)bad.materials.resize(32);if(mode==1)bad.primitives.resize(32);
        if(mode==2)bad.materials.resize(30);if(mode==3)bad.primitives.resize(30);
        if(mode==4)bad.triangleRegions.assign(bad.indices.size()/3+1,assets::StaticTriangleRegion::Shared);
        if(mode==5)bad.vertices.clear();
        const auto before=bad;TombDressingBuildReport report;report.floorDebrisTriangles=999;
        Check(!AppendPreparedTombDressing(assetRoot,bad,report,diagnostic,true,true) && SameOwner(before,bad) &&
              report.floorDebrisTriangles==0 && !diagnostic.empty(),
              "capacity, malformed provenance and empty owners reject atomically with cleared debris report");
    }
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
    TestImportedNicheFaceNormals(assetRoot);
    TestPreparedDressingAppend(assetRoot);
    TestStoneArchitecture(assetRoot);
    TestFloorFragmentGeometry();
    TestFloorDebrisOwner(assetRoot);
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
