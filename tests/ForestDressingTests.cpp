#include "gameplay/simulation/DevelopmentWorldRoute.h"
#include "gameplay/DevelopmentCheckpointSimulation.h"
#include "gameplay/simulation/ForestTreePlacementContract.h"
#include "scene/ForestDressing.h"
#include "scene/CorridorRegionalWaterClosure.h"
#include "scene/ForestUnderstory.h"
#include "scene/ForestRockDressing.h"
#include "scene/ForestDeadwood.h"
#include "scene/ForestRoadClue.h"
#include "scene/BellwetherShell.h"
#include "scene/GraveyardDressing.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/ChapterTerrainAdmission.h"
#include "scene/TombDressing.h"
#include "scene/assets/StaticPrimitiveGrouping.h"
#include "scene/assets/StaticTriangleSelection.h"
#include "vulkan/raytracing/RtStaticMeshSlot.h"
#include "gameplay/traversal/ChapterTerrainMovement.h"
#include "scene/assets/AssetManifest.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <chrono>
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

// Stable semantic receipt for matched same-toolchain control/candidate runs.
// No pointer values, std::string storage or struct padding enters this digest.
std::uint64_t ForestRepresentationDigest(const StaticMeshAsset& mesh,const ForestDressingBuildReport& report) {
    std::uint64_t hash=14695981039346656037ull;
    const auto bytes=[&](const auto* data,std::size_t count) {
        const auto* p=reinterpret_cast<const unsigned char*>(data);
        for(std::size_t i=0;i<count*sizeof(*data);++i){hash^=p[i];hash*=1099511628211ull;}
    };
    const auto scalar=[&](const auto& v){bytes(&v,1);};
    const auto text=[&](std::string_view v){scalar(std::uint64_t(v.size()));bytes(v.data(),v.size());};
    scalar(std::uint64_t(mesh.vertices.size()));
    for(const auto& v:mesh.vertices){bytes(v.position.data(),4);bytes(v.normal.data(),4);bytes(v.tangent.data(),4);bytes(v.uv0.data(),4);}
    scalar(std::uint64_t(mesh.indices.size()));bytes(mesh.indices.data(),mesh.indices.size());
    scalar(std::uint64_t(mesh.triangleRegions.size()));bytes(mesh.triangleRegions.data(),mesh.triangleRegions.size());
    for(const auto& v:mesh.primitives){scalar(v.vertexOffset);scalar(v.indexOffset);scalar(v.indexCount);scalar(v.materialIndex);scalar(v.nodeTransformIndex);}
    for(const auto& v:mesh.nodeTransforms){text(v.name);bytes(v.world.data(),16);}
    for(const auto& v:mesh.materials) {
        text(v.name);bytes(v.baseColorFactor.data(),4);bytes(v.emissiveFactor.data(),3);bytes(v.attenuationColor.data(),3);
        scalar(v.emissiveStrength);scalar(v.metallicFactor);scalar(v.roughnessFactor);scalar(v.occlusionStrength);scalar(v.normalScale);
        bytes(v.textureScale.data(),2);scalar(v.transmissionFactor);scalar(v.ior);scalar(v.thicknessFactor);scalar(v.attenuationDistance);
        scalar(v.numericalSpawnMinimumWidth);scalar(v.numericalSpawnGeometryError);scalar(v.baseColorTexture);scalar(v.normalTexture);
        scalar(v.ormTexture);scalar(v.emissiveTexture);scalar(v.textureGroup);scalar(v.deferTextureAllocation);scalar(v.flags);
    }
    for(const auto& v:mesh.sockets){text(v.name);scalar(v.nodeTransformIndex);bytes(v.world.data(),16);}
    bytes(mesh.bounds.minimum.data(),3);bytes(mesh.bounds.maximum.data(),3);
    scalar(std::uint64_t(report.addedVertices));scalar(std::uint64_t(report.addedIndices));scalar(std::uint64_t(report.addedTriangles));
    scalar(std::uint64_t(report.addedPrimitives));scalar(std::uint64_t(report.addedMaterials));scalar(std::uint64_t(report.placedInstances));
    bytes(report.addedBounds.minimum.data(),3);bytes(report.addedBounds.maximum.data(),3);
    for(const auto& v:report.placements){text(v.asset?v.asset:"");bytes(v.origin.data(),3);scalar(v.yawRadians);scalar(v.supportY);
        bytes(v.worldBounds.minimum.data(),3);bytes(v.worldBounds.maximum.data(),3);bytes(v.trunkBounds.minimum.data(),3);bytes(v.trunkBounds.maximum.data(),3);}
    return hash;
}
void PrintForestDigest(const char* label,const StaticMeshAsset& mesh,const ForestDressingBuildReport& report) {
    std::cout<<"FOREST_REPRESENTATION "<<label<<" fnv1a64="<<ForestRepresentationDigest(mesh,report)
        <<" vertices="<<mesh.vertices.size()<<" indices="<<mesh.indices.size()<<" placements="<<report.placedInstances<<'\n';
}

void TestNativeTreePairAndAppend()
{
    Check(gameplay::simulation::kForestTreePlacementContract.size() >= 64u,
          "chapter woodland requires an authored near/mid stand rather than ten isolated demonstration trees");
    std::string diagnostic;
    StaticMeshAsset owner = LoadCollapse(diagnostic);
    if (owner.vertices.empty()) return;
    const auto oldVertices = owner.vertices;
    const auto oldIndices = owner.indices;
    const auto oldMaterials = owner.materials.size();
    const auto oldPrimitives = owner.primitives.size();

    ForestDressingBuildReport report;
    const bool appended = AppendPreparedForestDressing(kRoot / "assets", owner, report, diagnostic);
    Check(appended, diagnostic.empty() ? "prepared tree pair imports and appends to the existing static owner"
                                      : diagnostic.c_str());
    if (!appended) {
        for (const auto& tree : gameplay::simulation::kForestTreePlacementContract)
            if (!gameplay::simulation::ResolveWorldRouteSupport(tree.x,tree.z).grounded)
                std::cerr << "Unbacked placement: " << tree.x << ", " << tree.z << '\n';
        return;
    }
    PrintForestDigest("ribbon",owner,report);
    Check(report.addedTriangles == 323264u && report.addedIndices == 323264u * 3u,
          "64 authored-scale instances contribute 32 LOD1 pine and 32 LOD1 alder triangle costs");
    Check(report.placedInstances == 64u && report.addedPrimitives == 4u && report.addedMaterials == 4u,
          "64 tree placements share exactly four material-family groups on the existing owner");
    Check(owner.primitives.size() == oldPrimitives + 4u && owner.materials.size() == oldMaterials + 4u,
          "forest adds no asset owner and stays within four appended materials and primitives");
    Check(owner.primitives.size() <= 32u && owner.materials.size() <= 32u,
          "existing shared 32 primitive/material limits remain intact");
    Check(owner.vertices.size() == oldVertices.size() + report.addedVertices &&
              owner.indices.size() == oldIndices.size() + report.addedIndices,
          "append report accounts for each forest vertex and index");
    Check(owner.triangleRegions.size() == owner.indices.size()/3u &&
              std::count(owner.triangleRegions.begin(), owner.triangleRegions.end(),
                  StaticTriangleRegion::ExteriorDressing) == static_cast<std::ptrdiff_t>(report.addedTriangles),
          "every prepared tree triangle has exterior ownership while the existing prefix stays shared");
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
                  Near(placement.worldBounds.minimum[1]+contract.rootBurialMetres, support.worldY),
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
    Check(pineCount == 32u && alderCount == 32u,
          "the woodland balances the original pine and alder silhouettes");
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

void TestPreparedMasterOwnership()
{
    std::string diagnostic;
    std::shared_ptr<const PreparedForestDressing> prepared;
    Check(PrepareForestDressing(kRoot/"assets",prepared,diagnostic),
          "canonical preparation admits the existing masters and rigid placements");
    if(!prepared)return;
    std::size_t triangles=0,pines=0,alders=0;
    for(const auto& master:prepared->masters)triangles+=master.indices.size()/3u;
    for(const auto& placement:prepared->placements) {
        Check(placement.masterIndex<prepared->masters.size(),"each rigid placement references an owned canonical master");
        if(placement.masterIndex==0)++pines;else if(placement.masterIndex==1)++alders;
    }
    Check(triangles==10102 && pines==32 && alders==32,
          "preparation owns only two local payloads while retaining all 64 placement records");
    const auto retained=prepared;
    Check(!PrepareForestDressing(kRoot/"missing-forest-root",prepared,diagnostic) && prepared==retained,
          "failed preparation preserves the prior immutable owned handle");
    prepared.reset();
    auto explicitOwner=LoadCollapse(diagnostic),wrappedOwner=explicitOwner;
    ForestDressingBuildReport explicitReport,wrappedReport;
    Check(AppendPreparedForestDressing(*retained,explicitOwner,explicitReport,diagnostic) &&
          AppendPreparedForestDressing(kRoot/"assets",wrappedOwner,wrappedReport,diagnostic) &&
          ForestRepresentationDigest(explicitOwner,explicitReport)==ForestRepresentationDigest(wrappedOwner,wrappedReport),
          "retained canonical handle outlives its original reference and reproduces the complete compatibility bake");
    for(unsigned mutation=0;mutation<7;++mutation) {
        auto invalid=*retained;
        if(mutation==0)invalid.placements[0].masterIndex=invalid.masters.size();
        if(mutation==1)invalid.placements[0].origin[1]+=.1f;
        if(mutation==2)invalid.primitiveFamilies[0][0]=255;
        if(mutation==3)invalid.masters[0].indices[0]=UINT32_MAX;
        if(mutation==4)invalid.masters[0].primitives[0].indexCount=0;
        if(mutation==5)invalid.masters[0].primitives[0].indexCount=1;
        if(mutation==6)invalid.masters[0].primitives[0].indexCount=2;
        auto owner=LoadCollapse(diagnostic);ForestDressingBuildReport report;
        const auto indices=owner.indices;
        const auto vertices=owner.vertices;
        Check(!AppendPreparedForestDressing(invalid,owner,report,diagnostic) && owner.indices==indices &&
              owner.vertices.size()==vertices.size() &&
              std::memcmp(owner.vertices.data(),vertices.data(),vertices.size()*sizeof(vertices.front()))==0 &&
              report.addedVertices==0 && report.placedInstances==0,
              "invalid canonical master/transform/mapping fails without mutating the existing owner or publishing a report");
    }
}

void TestF01Supplement(ForestDressingSharedLayout layout,
                       const gameplay::simulation::TriangleSupportSurface& terrain)
{
    // Copy only the shared ownership handle/ranges; never duplicate either master.
    const auto control=layout;
    const auto original64=layout.prepared->placements;
    const std::array vertexPointers{layout.prepared->masters[0].vertices.data(),layout.prepared->masters[1].vertices.data()};
    const std::array indexPointers{layout.prepared->masters[0].indices.data(),layout.prepared->masters[1].indices.data()};
    std::string diagnostic;
    Check(layout.supplementalPlacements.empty(),"existing64 shared control has no implicit background supplement");
    Check(PrepareF01BackgroundForestDressing(layout,terrain,diagnostic),
          diagnostic.empty()?"the exact twelve-root F01 authoring candidate admits against native terrain":diagnostic);
    if(layout.supplementalPlacements.size()!=12) {Check(false,"F01 publishes exactly twelve complete receipts");return;}
    struct Expected {std::size_t master;float x,z,yaw;};
    const std::array<Expected,12> expected{{
        {0,-39,-37,.2f},{1,-30,-36,1.1f},{0,-47,-33,2.f},{0,-54,-26,.8f},
        {1,-58,-17,2.5f},{0,-57,-8,1.7f},{0,-42,-45,1.4f},{0,-29,-44,2.9f},
        {1,-55,-39,.4f},{0,-64,-30,2.4f},{0,-67,-17,1.f},{1,-66,-5,3.1f}}};
    std::size_t samples=0,pines=0,alders=0,exposed=0;float maximumBurial=0,worstGap=-1000;
    for(std::size_t i=0;i<expected.size();++i) {
        const auto& receipt=layout.supplementalPlacements[i];const auto& p=receipt.placement;const auto& e=expected[i];
        Check(p.masterIndex==e.master && p.origin[0]==e.x && p.origin[2]==e.z && p.yawRadians==e.yaw,
              "F01 authoring roots retain exact approved positions/yaws/master references");
        if(p.masterIndex>=2)continue;
        const auto& source=layout.prepared->masters[p.masterIndex];const auto centre=terrain.Query(p.origin[0],p.origin[2]);
        Check(centre.found && centre.worldY==p.supportY && std::isfinite(receipt.rootBurialMetres) &&
              receipt.rootBurialMetres>=0 && receipt.rootBurialMetres<=.65f && receipt.rootTipSamples>=18 &&
              receipt.rootBurialMetres==p.supportY-source.bounds.minimum[1]-p.origin[1],
              "new authoring burial is actually measured, bounded and separate from the original collision receipts");
        Check(FiniteBounds(receipt.worldBounds) && FiniteBounds(receipt.trunkBounds) &&
              receipt.trunkBounds.minimum[1]>=p.supportY-.0001f && receipt.trunkBounds.maximum[1]<=p.supportY+2.f+.0001f &&
              Near(receipt.worldBounds.maximum[1]-receipt.worldBounds.minimum[1],source.bounds.maximum[1]-source.bounds.minimum[1]),
              "measured world/trunk bounds preserve rigid tree height and the existing two-metre clipped bark slab");
        const float c=std::cos(p.yawRadians),s=std::sin(p.yawRadians);std::size_t rootSamples=0;
        for(const auto& vertex:source.vertices) {
            const auto& v=vertex.position;if(v[1]>source.bounds.minimum[1]+.02f)continue;
            const auto ground=terrain.Query(p.origin[0]+c*v[0]+s*v[2],p.origin[2]-s*v[0]+c*v[2]);
            Check(ground.found,"every supplemental imported root tip has native terrain support");
            if(!ground.found)continue;
            const float gap=p.origin[1]+v[1]-ground.worldY;
            worstGap=std::max(worstGap,gap);if(gap>-.019f)++exposed;
            ++rootSamples;++samples;
        }
        Check(rootSamples==receipt.rootTipSamples,"new root-tip count certifies actual imported vertices, not a desired count");
        maximumBurial=std::max(maximumBurial,receipt.rootBurialMetres);
        if(p.masterIndex==0)++pines;else ++alders;
    }
    bool originalsExact=layout.prepared==control.prepared && layout.commonPrimitiveCount==control.commonPrimitiveCount;
    for(std::size_t i=0;i<2;++i) originalsExact=originalsExact &&
        layout.masters[i].primitiveOffset==control.masters[i].primitiveOffset &&
        layout.masters[i].primitiveCount==control.masters[i].primitiveCount &&
        layout.masters[i].localBounds.minimum==control.masters[i].localBounds.minimum &&
        layout.masters[i].localBounds.maximum==control.masters[i].localBounds.maximum &&
        layout.prepared->masters[i].vertices.data()==vertexPointers[i] && layout.prepared->masters[i].indices.data()==indexPointers[i];
    for(std::size_t i=0;i<original64.size();++i) {
        const auto& a=original64[i];const auto& b=layout.prepared->placements[i];
        originalsExact=originalsExact && a.masterIndex==b.masterIndex && a.origin==b.origin &&
            std::bit_cast<std::uint32_t>(a.yawRadians)==std::bit_cast<std::uint32_t>(b.yawRadians) &&
            std::bit_cast<std::uint32_t>(a.supportY)==std::bit_cast<std::uint32_t>(b.supportY);
    }
    Check(originalsExact && samples==936 && exposed==0 && pines==8 && alders==4 &&
          control.supplementalPlacements.empty() && gameplay::simulation::kForestTreePlacementContract.size()==64,
          "the diagnostic76 population reuses two unchanged masters and original64, without publishing replacement collision");
    const auto receipts=layout.supplementalPlacements;
    Check(!PrepareF01BackgroundForestDressing(layout,terrain,diagnostic) && layout.supplementalPlacements.size()==receipts.size(),
          "repeated supplement admission rejects rather than multiplying background references");
    for(std::size_t i=0;i<receipts.size();++i) {
        const auto& a=receipts[i];const auto& b=layout.supplementalPlacements[i];
        Check(a.placement.origin==b.placement.origin && a.placement.yawRadians==b.placement.yawRadians &&
              a.placement.masterIndex==b.placement.masterIndex && a.placement.supportY==b.placement.supportY &&
              a.worldBounds.minimum==b.worldBounds.minimum && a.worldBounds.maximum==b.worldBounds.maximum &&
              a.trunkBounds.minimum==b.trunkBounds.minimum && a.trunkBounds.maximum==b.trunkBounds.maximum &&
              a.rootBurialMetres==b.rootBurialMetres && a.rootTipSamples==b.rootTipSamples,
              "rejected duplicate keeps every prior diagnostic receipt field intact");
    }
    using Surface=gameplay::simulation::TriangleSupportSurface;
    const auto patch=[&](float x,float z,float radius,float slope) {
        const Surface::Point a{x-radius,-slope*radius,z-radius},b{x-radius,-slope*radius,z+radius},
            c{x+radius,slope*radius,z-radius},d{x+radius,slope*radius,z+radius};
        const std::array<Surface::Triangle,2> triangles{{{{a,b,c}},{{c,b,d}}}};
        return Surface::Build(triangles,diagnostic);
    };
    for(unsigned scenario=0;scenario<3;++scenario) {
        const auto surface=scenario==0?patch(-39,-37,4,0):scenario==1?patch(-39,-37,.01f,0):patch(0,0,100,2);
        Check(bool(surface),"supplement negative fixture has a valid immutable surface");if(!surface)continue;
        auto candidate=control;
        Check(!PrepareF01BackgroundForestDressing(candidate,*surface,diagnostic) && candidate.supplementalPlacements.empty() &&
              candidate.prepared==control.prepared && candidate.masters[0].primitiveOffset==control.masters[0].primitiveOffset,
              "partial/missing-footprint/excessive-burial failures publish no supplement or master replacement");
        if(scenario==0)Check(diagnostic.find("centre support at 1")!=std::string::npos,
              "the staged first root cannot leak when a later root has no support");
    }
    const auto flat=patch(0,0,100,0);
    if(flat) {
        auto authoring=control;
        Check(PrepareF01BackgroundForestDressing(authoring,*flat,diagnostic) && authoring.supplementalPlacements.size()==12,
              "new roots use their measured authoring burial instead of an invented prior collision receipt");
        auto retained=control.prepared;
        Check(!PrepareForestDressing(kRoot/"assets",retained,diagnostic,flat.get()) && retained==control.prepared &&
              diagnostic.find("collision receipt must match within 2mm")!=std::string::npos,
              "original64 still rejects changed support even when the root fit alone would satisfy the maximum burial guard");
    }
    auto missing=control;missing.prepared.reset();
    Check(!PrepareF01BackgroundForestDressing(missing,terrain,diagnostic) && missing.supplementalPlacements.empty(),
          "supplement cannot manufacture absent immutable master ownership");
    std::cout<<"F01_SUPPLEMENT_CPU roots="<<layout.supplementalPlacements.size()<<" pine="<<pines<<" alder="<<alders
        <<" rootTipSamples="<<samples<<" exposed="<<exposed<<" maximumBurial="<<maximumBurial<<" maximumGap="<<worstGap
        <<"; diagnostic composition only, collision/adoption unadmitted\n";
}

void TestSharedMasterPacking()
{
    std::string diagnostic;
    auto owner=LoadCollapse(diagnostic);
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface> terrain;
    TombDressingBuildReport tomb;GraveyardDressingReport graveyard;
    std::shared_ptr<const PreparedForestDressing> prepared;
    const bool base=AppendPreparedTombDressing(kRoot/"assets",owner,tomb,diagnostic) &&
        AppendChapterTerrain(kRoot/"assets",owner,terrain,diagnostic) &&
        AppendPreparedGraveyard(kRoot/"assets",owner,terrain,graveyard,diagnostic) &&
        AppendBellwetherShell(kRoot/"assets",owner,diagnostic) &&
        PrepareForestDressing(kRoot/"assets",prepared,diagnostic,terrain.get());
    Check(base,diagnostic.empty()?"shared master proof prepares the same real chapter and root-fitted snapshot":diagnostic);
    if(!base)return;
    const auto beforeSeed=owner;
    Check(SeedPreparedForestMaterialFamilies(*prepared,owner,diagnostic),"shared forest seeds only the canonical four material rows");
    Check(owner.materials.size()==beforeSeed.materials.size()+4 && owner.vertices.size()==beforeSeed.vertices.size() &&
          std::memcmp(owner.vertices.data(),beforeSeed.vertices.data(),owner.vertices.size()*sizeof(StaticRtVertex))==0 &&
          owner.indices==beforeSeed.indices && owner.triangleRegions==beforeSeed.triangleRegions &&
          owner.primitives.size()==beforeSeed.primitives.size() && owner.nodeTransforms.size()==beforeSeed.nodeTransforms.size() &&
          owner.bounds.minimum==beforeSeed.bounds.minimum && owner.bounds.maximum==beforeSeed.bounds.maximum,
          "material-only seeding adds no baked tree geometry, primitive, node or world-bounds growth");
    const auto seededDigest=ForestRepresentationDigest(owner,{});
    Check(!SeedPreparedForestMaterialFamilies(*prepared,owner,diagnostic) && ForestRepresentationDigest(owner,{})==seededDigest,
          "duplicate seed fails without changing the existing owner");
    auto crowded=beforeSeed;
    while(crowded.materials.size()<29) {auto material=crowded.materials.front();material.name="seed-cap-padding-"+std::to_string(crowded.materials.size());crowded.materials.push_back(material);}
    const auto crowdedDigest=ForestRepresentationDigest(crowded,{});
    Check(!SeedPreparedForestMaterialFamilies(*prepared,crowded,diagnostic) && ForestRepresentationDigest(crowded,{})==crowdedDigest,
          "material-only seed rejects insufficient pre-alias headroom without partial row or geometry publication");
    ForestUnderstoryReport understory;ForestRockReport rocks;ForestDeadwoodReport deadwood;ForestRoadClueReport road;
    const bool commonReady=AppendPreparedForestUnderstory(kRoot/"assets",owner,*terrain,understory,diagnostic) &&
        AppendPreparedForestRocks(kRoot/"assets",owner,terrain,rocks,diagnostic) &&
        AppendPreparedForestDeadwood(kRoot/"assets",owner,terrain,deadwood,diagnostic) &&
        AppendForestRoadClue(kRoot/"assets",owner,terrain,road,diagnostic) &&
        ConsolidateWorldBakedMaterialFamilies(owner,diagnostic);
    Check(commonReady,diagnostic.empty()?"shared proof retains the unchanged non-tree dressing builders":diagnostic);
    if(!commonReady)return;
    const auto common=owner;
    Check(common.primitives.size()==9,"common world keeps nine material groups before adding local masters");
    const auto familyTriangles=[&](std::string_view family) {
        std::size_t count=0;
        for(const auto& p:common.primitives)if(common.materials[p.materialIndex].name==family)count+=p.indexCount/3u;
        return count;
    };
    Check(familyTriangles("ForestBark")==deadwood.triangles && familyTriangles("ForestAlder")==understory.triangles &&
          familyTriangles("ForestPine")==0 && familyTriangles("ForestMoss")==0,
          "common Bark/deadwood and Alder/understory survive without any world-baked tree triangles");
    ForestDressingSharedLayout layout;
    Check(AppendPreparedForestMasters(prepared,owner,layout,diagnostic),
          diagnostic.empty()?"two local masters append once after common consolidation":diagnostic);
    if(!layout.prepared)return;
    Check(layout.commonPrimitiveCount==9 && layout.masters[0].primitiveOffset==9 && layout.masters[0].primitiveCount==3 &&
          layout.masters[1].primitiveOffset==12 && layout.masters[1].primitiveCount==3 && owner.primitives.size()==15,
          "common/pine/alder ranges are disjoint, contiguous and cover all fifteen primitives");
    Check(owner.materials.size()==common.materials.size() && owner.sockets.size()==common.sockets.size() &&
          owner.bounds.minimum==common.bounds.minimum && owner.bounds.maximum==common.bounds.maximum &&
          std::memcmp(owner.vertices.data(),common.vertices.data(),common.vertices.size()*sizeof(StaticRtVertex))==0 &&
          std::equal(common.indices.begin(),common.indices.end(),owner.indices.begin()) &&
          std::equal(common.triangleRegions.begin(),common.triangleRegions.end(),owner.triangleRegions.begin()),
          "local masters preserve common world bounds and exact common geometry/provenance prefix without materials");
    auto prefix=owner;
    prefix.vertices.resize(common.vertices.size());prefix.indices.resize(common.indices.size());
    prefix.triangleRegions.resize(common.triangleRegions.size());prefix.primitives.resize(common.primitives.size());
    prefix.nodeTransforms.resize(common.nodeTransforms.size());
    Check(ForestRepresentationDigest(prefix,{})==ForestRepresentationDigest(common,{}),
          "all common material, node, socket, primitive and bounds records remain unchanged");
    std::size_t vertices=common.vertices.size(),indices=common.indices.size();
    for(std::size_t tree=0;tree<2;++tree) {
        const auto& source=prepared->masters[tree];const auto& range=layout.masters[tree];
        Check(range.localBounds.minimum==source.bounds.minimum && range.localBounds.maximum==source.bounds.maximum &&
              std::memcmp(owner.vertices.data()+vertices,source.vertices.data(),source.vertices.size()*sizeof(StaticRtVertex))==0 &&
              std::equal(source.indices.begin(),source.indices.end(),owner.indices.begin()+indices),
              "each appended normalized local master retains exact positions/normals/tangents/UVs/indices and local bounds");
        for(std::size_t i=0;i<3;++i) {
            const auto& p=owner.primitives[range.primitiveOffset+i];const auto& original=source.primitives[i];
            Check(p.vertexOffset==vertices+original.vertexOffset && p.indexOffset==indices+original.indexOffset &&
                  p.indexCount==original.indexCount &&
                  owner.materials[p.materialIndex].name==prepared->materialFamilies[prepared->primitiveFamilies[tree][i]].name &&
                  owner.nodeTransforms[p.nodeTransformIndex].world==std::array<float,16>{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
                  "master primitive offsets/materials remap to existing rows with exactly one later rigid instance transform");
        }
        vertices+=source.vertices.size();indices+=source.indices.size();
    }
    Check(owner.vertices.size()==vertices && owner.indices.size()==indices &&
          owner.indices.size()-common.indices.size()==10102u*3u &&
          owner.triangleRegions.size()==owner.indices.size()/3u &&
          std::all_of(owner.triangleRegions.begin()+common.indices.size()/3u,owner.triangleRegions.end(),
              [](auto region){return region==StaticTriangleRegion::ExteriorDressing;}),
          "shared ownership adds exactly 10102 local triangles once, labelled exterior without granting an omission certificate");
    {
        const auto pbr=static_cast<std::uint32_t>(horde::vulkan::raytracing::RtInstanceFlag::StaticPbr);
        using namespace horde::vulkan::raytracing;
        const std::array<StaticRtAssetRegistration,3> ranges{{
            {0,100,pbr,0,&owner,nullptr,RtGeometryRole::Static,0,static_cast<std::uint32_t>(layout.commonPrimitiveCount)},
            {1,101,pbr,0,&owner,nullptr,RtGeometryRole::Static,9,3},
            {2,102,pbr,0,&owner,nullptr,RtGeometryRole::Static,12,3}}};
        RtStaticMeshSlot packed,commonPacked;
        const bool packedReady=packed.InitializeSelectedGeometry(ranges,diagnostic) &&
            commonPacked.Initialize(std::array{StaticRtAssetRegistration{0,100,pbr,0,&common}},diagnostic);
        Check(packedReady,diagnostic.empty()?"disjoint ranges pack the same static owner once":diagnostic);
        if(packedReady) {
            Check(packed.Vertices().size()==owner.vertices.size() && packed.Indices()==owner.indices &&
                  packed.PrimitiveMetadata().size()==15 && packed.Measurements().vertexBytes==owner.vertices.size()*sizeof(StaticRtVertex) &&
                  packed.Materials().size()==commonPacked.Materials().size() &&
                  std::memcmp(packed.Materials().data(),commonPacked.Materials().data(),packed.Measurements().materialBytes)==0 &&
                  packed.TextureArrayCounts().baseColor==commonPacked.TextureArrayCounts().baseColor &&
                  packed.TextureArrayCounts().normal==commonPacked.TextureArrayCounts().normal &&
                  packed.TextureArrayCounts().orm==commonPacked.TextureArrayCounts().orm,
                  "three registered ranges duplicate no vertex/index/material/atlas payload");
            for(std::size_t i=0;i<ranges.size();++i) {
                const auto& row=packed.InstanceMetadata()[i];
                Check(row.primitiveBase==ranges[i].primitiveOffset && row.primitiveCount==ranges[i].primitiveCount,
                      "each BLAS-local index resolves only its common or local-master range");
            }
        }
    }
    const auto immutable=prepared;prepared.reset();
    Check(layout.prepared==immutable && layout.prepared->placements.size()==64,
          "published layout retains the exact const root/terrain placement owner after caller reference retirement");
    auto fullMaterials=common;
    while(fullMaterials.materials.size()<32) {auto m=fullMaterials.materials.front();m.name="capacity-padding-"+std::to_string(fullMaterials.materials.size());fullMaterials.materials.push_back(m);}
    ForestDressingSharedLayout fullLayout;
    Check(AppendPreparedForestMasters(immutable,fullMaterials,fullLayout,diagnostic) && fullMaterials.materials.size()==32,
          "local masters remain appendable at the full pre-alias 32 material rows without six temporary source materials");
    const auto rejects=[&](std::shared_ptr<const PreparedForestDressing> input,StaticMeshAsset invalid,std::string_view label) {
        const auto digest=ForestRepresentationDigest(invalid,{});auto published=layout;
        const bool accepted=AppendPreparedForestMasters(std::move(input),invalid,published,diagnostic);
        bool sameRanges=published.commonPrimitiveCount==layout.commonPrimitiveCount;
        for(std::size_t i=0;i<2;++i)sameRanges=sameRanges &&
            published.masters[i].primitiveOffset==layout.masters[i].primitiveOffset &&
            published.masters[i].primitiveCount==layout.masters[i].primitiveCount &&
            published.masters[i].localBounds.minimum==layout.masters[i].localBounds.minimum &&
            published.masters[i].localBounds.maximum==layout.masters[i].localBounds.maximum;
        Check(!accepted && ForestRepresentationDigest(invalid,{})==digest &&
              published.prepared==layout.prepared && sameRanges,label);
    };
    rejects(nullptr,common,"null immutable source fails without partial owner/layout publication");
    rejects(immutable,owner,"a second shared append fails without growing or replacing the existing owner/layout");
    rejects(immutable,beforeSeed,"missing canonical rows cannot silently allocate replacement master materials");
    auto bad=common;bad.primitives.push_back(bad.primitives.front());
    rejects(immutable,std::move(bad),"unconsolidated common families are rejected before master append");
    bad=common;bad.triangleRegions.push_back(StaticTriangleRegion::Shared);
    rejects(immutable,std::move(bad),"corrupt common provenance rejects without relabelling earlier geometry");
    bad=common;for(auto& m:bad.materials)if(m.name=="ForestBark")m.normalScale+=.1f;
    rejects(immutable,std::move(bad),"changed canonical material factors cannot inherit the prepared master mapping");
    for(unsigned mutation=0;mutation<8;++mutation) {
        auto invalid=std::make_shared<PreparedForestDressing>(*immutable);
        if(mutation==0)invalid->placements[0].origin[1]+=.1f;
        if(mutation==1)invalid->placements[0].masterIndex=2;
        if(mutation==2)invalid->primitiveFamilies[0][0]=255;
        if(mutation==3)invalid->masters[0].indices[0]=UINT32_MAX;
        if(mutation==4)invalid->masters[0].primitives[0].indexCount=1;
        if(mutation==5)invalid->masters[0].bounds.maximum[0]+=.1f;
        if(mutation==6)invalid->masters[0].vertices[0].tangent[0]=NAN;
        if(mutation==7)invalid->masters[0].nodeTransforms[0].world[0]=NAN;
        rejects(std::move(invalid),common,"invalid local source/placement fails atomically while preserving prior immutable publication");
    }
    TestF01Supplement(layout,*terrain);
    std::cout<<"SHARED_FOREST_CPU commonPrimitives="<<layout.commonPrimitiveCount<<" localMasterPrimitives=6 placements="
        <<layout.prepared->placements.size()<<" localTriangles=10102 materials="<<owner.materials.size()
        <<" vertexBytes="<<owner.vertices.size()*sizeof(StaticRtVertex)<<" indexBytes="<<owner.indices.size()*sizeof(std::uint32_t)
        <<"; CPU ownership only, normal omission certificate not inherited\n";
}

template<class Append>
void CheckCorruptForestProvenance(const StaticMeshAsset& owner, Append append)
{
    for (bool excessive : {false, true}) {
        auto invalid = owner;
        if (excessive) invalid.triangleRegions.resize(owner.indices.size()/3u+1u, StaticTriangleRegion::Shared);
        else invalid.triangleRegions.assign(1u, static_cast<StaticTriangleRegion>(9u));
        const auto regions = invalid.triangleRegions;
        Check(!append(invalid) && invalid.indices == owner.indices && invalid.triangleRegions == regions &&
                  invalid.vertices.size() == owner.vertices.size() &&
                  std::memcmp(invalid.vertices.data(), owner.vertices.data(),
                      owner.vertices.size()*sizeof(owner.vertices.front())) == 0 &&
                  invalid.primitives.size() == owner.primitives.size() &&
                  invalid.materials.size() == owner.materials.size() &&
                  invalid.nodeTransforms.size() == owner.nodeTransforms.size(),
              "forest append rejects corrupt ownership before publishing geometry or labels");
    }
}

void TestUnsafeRootFootprintsFailAtomically()
{
    using Surface=gameplay::simulation::TriangleSupportSurface;
    std::string diagnostic;
    const auto& first=gameplay::simulation::kForestTreePlacementContract.front();
    for(bool missing:{false,true}) {
        const float radius=missing ? .01f : 100.f;
        const float slope=missing ? 0.f : 2.f;
        const float x=first.x,z=first.z;
        const Surface::Point a{x-radius,-slope*radius,z-radius};
        const Surface::Point b{x-radius,-slope*radius,z+radius};
        const Surface::Point c{x+radius,slope*radius,z-radius};
        const Surface::Point d{x+radius,slope*radius,z+radius};
        const std::array<Surface::Triangle,2> triangles{{{{a,b,c}},{{c,b,d}}}};
        const auto terrain=Surface::Build(triangles,diagnostic);
        Check(terrain!=nullptr,"root negative fixture has valid native support triangles");
        if(!terrain)continue;
        auto owner=LoadCollapse(diagnostic);
        const auto oldVertices=owner.vertices;
        const auto oldIndices=owner.indices;
        ForestDressingBuildReport report;
        Check(!AppendPreparedForestDressing(kRoot/"assets",owner,report,diagnostic,terrain.get()),
            "missing footprint or excessive burial must reject the placement");
        Check(diagnostic.find(missing ? "missing terrain support" : "needs reauthoring")!=std::string::npos,
            "root failure must identify the actual support/admission problem");
        Check(owner.vertices.size()==oldVertices.size() && owner.indices==oldIndices &&
              std::memcmp(owner.vertices.data(),oldVertices.data(),oldVertices.size()*sizeof(oldVertices[0]))==0,
              "failed root fitting must preserve the original static owner byte-for-byte");
    }
}

void TestNativeTerrainAndTrailClearance()
{
    std::string diagnostic;
    AssetManifest manifest;
    StaticMeshAsset mesh;
    const auto directory = kRoot / "assets/models/world/runtime/chapter-terrain-v1";
    if (!AssetManifest::Load(directory / "asset.manifest.json", manifest, diagnostic) ||
        !StaticMeshAsset::Load(directory / "chapter-terrain-lod0.runtime.glb", manifest, mesh, diagnostic))
    {
        Check(false, diagnostic);
        return;
    }
    const auto surface = PrepareChapterTerrainSurface(mesh, diagnostic);
    Check(surface != nullptr, "native woodland terrain imports for actual grounding");
    if (!surface) return;
    auto owner = LoadCollapse(diagnostic);
    ForestDressingBuildReport report;
    const bool appended = AppendPreparedForestDressing(kRoot / "assets", owner, report, diagnostic, surface.get());
    Check(appended, diagnostic.empty() ? "woodland appends on actual terrain" : diagnostic);
    if (!appended) return;
    PrintForestDigest("native-terrain",owner,report);
    std::array<StaticMeshAsset,2> rootSources;
    for (std::size_t i=0;i<rootSources.size();++i) {
        const auto sourceDirectory=kRoot/"assets/models/world/runtime/forest-tree-pair-v1";
        const auto name=i==0 ? "horde-irregular-pine-v1-lod1.glb" : "horde-upright-alder-v1-lod1.glb";
        Check(AssetManifest::Load(sourceDirectory/"asset.manifest.json",manifest,diagnostic) &&
            StaticMeshAsset::Load(sourceDirectory/name,manifest,rootSources[i],diagnostic),
            "actual native tree source required for footprint regression");
    }
    float worstRootGap=-1000.f;
    std::size_t exposedRoots=0,rootSamples=0;
    for (const auto& placement : report.placements)
    {
        const auto& source=rootSources[std::string_view(placement.asset)=="pine" ? 0 : 1];
        const float c=std::cos(placement.yawRadians),sine=std::sin(placement.yawRadians);
        for (const auto& vertex:source.vertices) {
            const auto& p=vertex.position;
            if(p[1]>source.bounds.minimum[1]+.02f)continue;
            const float x=placement.origin[0]+c*p[0]+sine*p[2];
            const float z=placement.origin[2]-sine*p[0]+c*p[2];
            const auto under=surface->Query(x,z);
            Check(under.found,"root tip must have real terrain underneath");
            if(!under.found)continue;
            const float gap=placement.origin[1]+p[1]-under.worldY;
            worstRootGap=std::max(worstRootGap,gap);++rootSamples;
            if(gap>-.019f)++exposedRoots;
        }
        const auto ground = surface->Query(placement.origin[0], placement.origin[2]);
        Check(ground.found && placement.worldBounds.minimum[1]<=ground.worldY &&
                  ground.worldY-placement.worldBounds.minimum[1]<=.6501f,
              "root burial is bounded against actual native terrain, never the old ribbon");
        Check(!gameplay::traversal::ChapterExteriorMovementClear(*surface,
                  placement.origin[0], placement.origin[2], placement.origin[0], placement.origin[2],
                  ground.worldY, .24f), "native chapter collision rejects entering a rendered trunk");
    }
    std::cout<<"Native root footprint samples="<<rootSamples<<" exposed="<<exposedRoots
             <<" maximum_gap_m="<<worstRootGap<<'\n';
    Check(rootSamples>=64u*18u && exposedRoots==0,
          "all imported root-tip base vertices must embed at least 2cm into the exact terrain");
    // Sweep the full 2.4 m authored clear trail, including player radius, against
    // measured bark boxes. Side foliage can overhang; solids cannot narrow it.
    const auto& route = gameplay::simulation::kWorldRoutePoints;
    for (std::size_t segment = 2; segment + 1 < route.size(); ++segment)
    {
        const auto a = route[segment], b = route[segment + 1];
        for (const auto& tree : gameplay::simulation::kForestTreePlacementContract)
        {
            float enter = 0, leave = 1;
            const auto axis = [&](float start, float end, float low, float high) {
                const float delta = end - start;
                if (std::abs(delta) < 1e-7f) return start >= low && start <= high;
                float t0 = (low - start) / delta, t1 = (high - start) / delta;
                if (t0 > t1) std::swap(t0, t1);
                enter = std::max(enter, t0); leave = std::min(leave, t1);
                return enter <= leave;
            };
            Check(!(axis(a.x,b.x,tree.trunkMinX-1.44f,tree.trunkMaxX+1.44f) &&
                    axis(a.z,b.z,tree.trunkMinZ-1.44f,tree.trunkMaxZ+1.44f)),
                  "new woodland must preserve the 2.4 m trail plus capsule clearance");
        }
    }
}

void TestNativeChapterPrimitiveOwnership()
{
    using namespace horde::vulkan::raytracing;
    std::string diagnostic;
    AssetManifest manifest;
    StaticMeshAsset owner;
    const auto directory = kRoot / "assets/models/world/runtime/chapter-entry-return-v1";
    const bool imported = AssetManifest::Load(directory / "asset.manifest.json", manifest, diagnostic) &&
        StaticMeshAsset::Load(directory / "collapsed-entry-lod0.runtime.glb", manifest, owner, diagnostic);
    Check(imported, "current chapter entry imports for real primitive ownership proof");
    if (!imported) return;
    auto malformedOwner=owner;
    malformedOwner.triangleRegions.resize(owner.indices.size()/3u+1u,StaticTriangleRegion::Shared);
    TombDressingBuildReport malformedTomb;
    Check(!AppendPreparedTombDressing(kRoot/"assets",malformedOwner,malformedTomb,diagnostic) &&
          malformedOwner.triangleRegions.size()==owner.indices.size()/3u+1u,
          "tomb append rejects excessive provenance without truncating existing source data");
    TombDressingBuildReport tomb;
    ForestDressingBuildReport forest;
    GraveyardDressingReport graveyard;
    ForestUnderstoryReport understory;
    ForestDeadwoodReport deadwood;
    ForestRockReport rocks;
    ForestRoadClueReport roadClue;
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface> terrain;
    const bool prepared = AppendPreparedTombDressing(kRoot / "assets", owner, tomb, diagnostic) &&
        AppendChapterTerrain(kRoot / "assets", owner, terrain, diagnostic) &&
        [&]{auto malformed=owner;malformed.triangleRegions.resize(owner.indices.size()/3u+1u,StaticTriangleRegion::Shared);
            auto retainedTerrain=terrain;GraveyardDressingReport rejected;
            Check(!AppendPreparedGraveyard(kRoot/"assets",malformed,retainedTerrain,rejected,diagnostic) && retainedTerrain==terrain &&
                  malformed.triangleRegions.size()==owner.indices.size()/3u+1u,
                  "graveyard append rejects excessive provenance and retains prior geometry/contact");
            return AppendPreparedGraveyard(kRoot/"assets",owner,terrain,graveyard,diagnostic);}() &&
        AppendBellwetherShell(kRoot / "assets", owner, diagnostic) &&
        [&]{CheckCorruptForestProvenance(owner,[&](auto& invalid) {
                ForestDressingBuildReport rejected;
                return AppendPreparedForestDressing(kRoot/"assets",invalid,rejected,diagnostic,terrain.get());
            });
            return AppendPreparedForestDressing(kRoot/"assets",owner,forest,diagnostic,terrain.get());}() &&
        [&]{CheckCorruptForestProvenance(owner,[&](auto& invalid) {
                ForestUnderstoryReport rejected;
                return AppendPreparedForestUnderstory(kRoot/"assets",invalid,*terrain,rejected,diagnostic);
            });
            return AppendPreparedForestUnderstory(kRoot/"assets",owner,*terrain,understory,diagnostic);}() &&
        [&]{CheckCorruptForestProvenance(owner,[&](auto& invalid) {
                auto retained=terrain;ForestRockReport rejected;
                const bool accepted=AppendPreparedForestRocks(kRoot/"assets",invalid,retained,rejected,diagnostic);
                Check(retained==terrain,"rejected rock ownership must retain authoritative collision support");
                return accepted;
            });
            return AppendPreparedForestRocks(kRoot/"assets",owner,terrain,rocks,diagnostic);}() &&
        [&]{CheckCorruptForestProvenance(owner,[&](auto& invalid) {
                auto retained=terrain;ForestDeadwoodReport rejected;
                const bool accepted=AppendPreparedForestDeadwood(kRoot/"assets",invalid,retained,rejected,diagnostic);
                Check(retained==terrain,"rejected deadwood ownership must retain authoritative collision support");
                return accepted;
            });
            return AppendPreparedForestDeadwood(kRoot/"assets",owner,terrain,deadwood,diagnostic);}() &&
        [&]{CheckCorruptForestProvenance(owner,[&](auto& invalid) {
                auto retained=terrain;ForestRoadClueReport rejected;
                const bool accepted=AppendForestRoadClue(kRoot/"assets",invalid,retained,rejected,diagnostic);
                Check(retained==terrain,"rejected road-clue ownership retains authoritative contact");
                return accepted;
            });
            const auto prior=terrain;
            const auto materialCount=owner.materials.size();
            if(!AppendForestRoadClue(kRoot/"assets",owner,terrain,roadClue,diagnostic))return false;
            Check(roadClue.triangles==144 && roadClue.placements.size()==9 &&
                  owner.materials.size()==materialCount && terrain->TriangleCount()==prior->TriangleCount() &&
                  terrain->StepTriangleCount()==prior->StepTriangleCount() &&
                  terrain->NonWalkableTriangleCount()==prior->NonWalkableTriangleCount()+144,
                  "bounded road cue shares masonry and adds only its same 144 nonwalkable render/contact triangles");
            Check(roadClue.minimumTrailClearance>=1.44f && roadClue.maximumBottomGap<=-.029f,
                  "actual road geometry preserves trail/capsule margin and embeds its bottom perimeter");
            for(const auto& placed:roadClue.placements) {
                Check(!placed.bottomContacts.empty(),"every road piece retains actual bottom-footprint witnesses");
                for(const auto& point:placed.bottomContacts) {
                    const auto under=prior->Query(point[0],point[2]);
                    if(under.found && point[1]<under.worldY-.30f)
                        std::cerr<<"Buried road footprint: source="<<placed.source<<" origin="<<placed.origin[0]<<','
                            <<placed.origin[1]<<','<<placed.origin[2]<<" bottom="<<point[0]<<','<<point[1]<<','<<point[2]
                            <<" groundY="<<under.worldY<<" gap="<<(point[1]-under.worldY)<<'\n';
                    Check(under.found && point[1]<=under.worldY-.029f && point[1]>=under.worldY-.30f,
                          "road base vertices are grounded and burial remains bounded on actual immutable terrain");
                }
                const auto ground=terrain->Query(placed.origin[0],placed.origin[2]);
                Check(ground.found && !terrain->NonWalkableVolumeClear(placed.origin[0],placed.origin[2],
                    placed.origin[0],placed.origin[2],ground.worldY+.01f,ground.worldY+1.6f,.24f),
                    "rendered road solids participate in the existing physical capsule contact");
            }
            for(std::size_t segment=5;segment<=6;++segment) {
                const auto a=gameplay::simulation::kWorldRoutePoints[segment];
                const auto b=gameplay::simulation::kWorldRoutePoints[segment+1];
                const float dx=b.x-a.x,dz=b.z-a.z,len=std::hypot(dx,dz);
                for(unsigned station=0;station<=40;++station)for(float side:{-1.2f,0.f,1.2f}) {
                    const float x=a.x+dx*station/40.f-dz/len*side,z=a.z+dz*station/40.f+dx/len*side;
                    const auto before=prior->Query(x,z),after=terrain->Query(x,z);
                    Check(before.found==after.found && before.worldY==after.worldY &&
                          before.triangle==after.triangle && before.normal==after.normal && before.surface==after.surface,
                          "F03 trail and capsule-side samples retain exact pre-cue ground/support identity");
                    if(before.found)Check(prior->NonWalkableVolumeClear(x,z,x,z,before.worldY+.02f,before.worldY+1.6f,.24f)==
                        terrain->NonWalkableVolumeClear(x,z,x,z,before.worldY+.02f,before.worldY+1.6f,.24f),
                        "road cue cannot add a blocker to the existing trail and capsule margin");
                }
            }
            auto duplicate=owner;auto retained=terrain;ForestRoadClueReport rejected;
            Check(!AppendForestRoadClue(kRoot/"assets",duplicate,retained,rejected,diagnostic) &&
                  retained==terrain && duplicate.vertices.size()==owner.vertices.size() && rejected.placements.empty(),
                  "duplicate road admission rejects atomically without new contact or geometry");
            std::cout<<"F03 road clue: triangles="<<roadClue.triangles<<" verticesBytes="<<roadClue.vertexBytes
                <<" indexBytes="<<roadClue.indexBytes<<" minimumTrailClearance="<<roadClue.minimumTrailClearance
                <<" minimumBottomGap="<<roadClue.minimumBottomGap<<" maximumBottomGap="<<roadClue.maximumBottomGap
                <<" stepTriangles="<<terrain->StepTriangleCount()<<'\n';
            return true;}() &&
        ConsolidateWorldBakedMaterialFamilies(owner, diagnostic);
    Check(prepared, diagnostic.empty() ? "real tomb/terrain/forest preparation succeeds" : diagnostic);
    if (!prepared) return;
    Check(understory.placements.size()==18 && understory.triangles==17712,
          "real exported terrain supports all bounded understory clumps before native owner packing");
    Check(deadwood.placements.size()==10 && deadwood.triangles==11904 && deadwood.nonWalkableTriangles==11884 && deadwood.stepTriangles==20,
          "real exported terrain supports all four prepared deadwood assets with exact contact");
    {
        namespace sim=gameplay::simulation;
        namespace tr=gameplay::traversal;
        const auto a=sim::kWorldRoutePoints[3],b=sim::kWorldRoutePoints[4];
        const float length=std::hypot(b.x-a.x,b.z-a.z),fx=(b.x-a.x)/length,fz=(b.z-a.z)/length;
        const auto root=deadwood.trailRoot.origin;
        const float fromX=root[0]-fx*.6f,fromZ=root[2]-fz*.6f;
        const auto from=tr::ChapterExteriorStepSupport(*terrain,fromX,fromZ,gameplay::kPlayerCollisionRadius);
        const auto onto=tr::ResolveChapterStepMovement(*terrain,fromX,fromZ,root[0],root[2],
            from.worldY,gameplay::kPlayerCollisionRadius);
        Check(terrain->StepTriangleCount()==20u && from.grounded &&
              !tr::ChapterExteriorMovementClear(*terrain,fromX,fromZ,root[0],root[2],
                  from.worldY,gameplay::kPlayerCollisionRadius) && onto.accepted &&
              onto.support.surface==sim::SupportSurface::Wood && onto.support.worldY>root[1],
              "same actual prepared root blocks legacy collision but admits bounded STEP support");

        // Bare terrain companion fixtures miss the appended trail root. Walk
        // Kit against the actual production dressing/collision owner, keeping
        // the observed player nearby so the escort distance gate stays valid.
        gameplay::dialogue::CompanionRouteFixture kit;
        bool crossedRoot=false,bounded=true,raisedOnWood=false;
        for(unsigned tick=0;tick<6000 && kit.State().waypoint<sim::kWorldRoutePoints.size();++tick) {
            const auto previous=kit.State();
            kit.Step(1.f/60.f,previous.x,previous.z,true,true,false,terrain.get(),true);
            const auto& current=kit.State();
            const auto bare=terrain->Query(current.x,current.z);
            const auto support=tr::ChapterExteriorStepSupport(*terrain,current.x,current.z,.24f);
            bounded&=current.grounded && std::hypot(current.x-previous.x,current.z-previous.z)<=.0301f &&
                std::abs(current.y-previous.y)<=sim::kWorldRouteMaximumStep+.0001f;
            raisedOnWood|=bare.found && support.surface==sim::SupportSurface::Wood && current.y>bare.worldY+.01f;
            crossedRoot|=(current.x-root[0])*fx+(current.z-root[2])*fz>.6f;
        }
        Check(bounded && crossedRoot && raisedOnWood,"Kit crosses the actual prepared root with grounded bounded shared STEP movement");
        Check(kit.State().waypoint==sim::kWorldRoutePoints.size() && kit.State().grounded,
              "Kit reaches the actual dressed lookout without bypassing collision or escort distance");

        struct RouteObservation {
            const sim::GameSimulation* game;
            const sim::TriangleSupportSurface* terrain;
            std::array<float,3> root;
            float fx,fz,lastX=0,lastZ=0,maximumTravel=0;
            bool previousExterior=false,ascent=false,pullUp=false,before=false,after=false,wood=false;
            static void Begin(void*) noexcept {}
            static void Complete(void* user) noexcept {
                auto& o=*static_cast<RouteObservation*>(user);
                const auto& s=o.game->Snapshot();
                o.ascent|=s.rescue.phase==tr::Phase::Ascent;
                o.pullUp|=s.rescue.phase==tr::Phase::PullUp;
                const bool exterior=s.rescue.exteriorSide&&!s.rescue.equipmentStowed;
                if(exterior) {
                    if(o.previousExterior)o.maximumTravel=std::max(o.maximumTravel,
                        std::hypot(s.playerX-o.lastX,s.playerZ-o.lastZ));
                    const float along=(s.playerX-o.root[0])*o.fx+(s.playerZ-o.root[2])*o.fz;
                    o.before|=along<-.5f;o.after|=along>.5f;
                    const auto ground=o.terrain->Query(s.playerX,s.playerZ);
                    o.wood|=std::abs(along)<.4f && s.playerGrounded && ground.found &&
                        s.playerSupportSurface==sim::SupportSurface::Wood &&
                        s.playerSupportWorldY>ground.worldY+.001f;
                }
                o.lastX=s.playerX;o.lastZ=s.playerZ;o.previousExterior=exterior;
            }
        };
        for(const auto name:{"chapter-forest-first-bend","chapter-forest-second-bend","chapter-forest-lookout",
                            "chapter-forest-clue-forward","chapter-forest-clue-reverse"}) {
            const auto* checkpoint=gameplay::FindDevelopmentCheckpoint(name);
            Check(checkpoint && checkpoint->keeperDeathFixedTicks==240u,
                  "canonical exterior route capture explicitly seeds accepted Keeper death/current rules");
            if(!checkpoint)continue;
            sim::GameSimulation capture(sim::ProductionGameSimulationConfig());
            RouteObservation observed{&capture,terrain.get(),root,fx,fz};
            const gameplay::DevelopmentCheckpointStepFixedObservation observer{
                &observed,RouteObservation::Begin,RouteObservation::Complete};
            gameplay::DevelopmentCheckpointStageEvidence evidence;
            const bool staged=gameplay::StageDevelopmentCheckpointForScene(capture,*checkpoint,[&] {
                for(std::size_t zone=1;zone<sim::kWorldZones.size();++zone)
                    Check(capture.PublishChapterTerrain({capture.Snapshot().worldRoute.generation,
                        static_cast<sim::WorldZoneId>(zone)},terrain),"canonical route capture publishes actual prepared terrain");
            },false,&evidence,&observer);
            Check(staged,std::string(name)+" stages through real ascent and prepared root traversal");
            if(!staged) {
                std::cerr<<name<<": "<<gameplay::DevelopmentCheckpointStageFailureName(evidence.failure)
                         <<" at ("<<capture.Snapshot().playerX<<','<<capture.Snapshot().playerZ<<")\n";
                continue;
            }
            const auto& s=capture.Snapshot();
            const auto support=tr::ChapterExteriorStepSupport(*terrain,s.playerX,s.playerZ,gameplay::kPlayerCollisionRadius);
            Check(evidence.failure==gameplay::DevelopmentCheckpointStageFailure::None &&
                  evidence.worldGeneration!=0 && evidence.worldGeneration==evidence.terrainGeneration &&
                  s.modernCombatRules && s.lich.phase==gameplay::LichPhase::Dead && s.lich.health==0 &&
                  s.lich.deathAnimationComplete && s.lich.deathDissolveProgress==1.f && s.lich.roomMistDensityScale==0.f,
                  "canonical exterior capture retains current rules and fully retired diagnostic Keeper");
            Check(observed.ascent && observed.pullUp && observed.before && observed.after && observed.wood &&
                  observed.maximumTravel<=.056f && s.rescue.firstAscentCompleted && s.rescue.exteriorSide &&
                  !s.rescue.equipmentStowed && s.rescue.claimCount==1u && s.rescue.deploymentCount==1u,
                  "route capture crosses root on Wood through bounded fixed-step movement after one physical ascent");
            const bool cameraAndSupport=std::hypot(s.playerX-checkpoint->cameraX,s.playerZ-checkpoint->cameraZ)<.01f &&
                  Near(s.playerYawRadians,checkpoint->yaw) && Near(s.playerPitchRadians,checkpoint->pitch) &&
                  s.playerGrounded && support.grounded && s.playerSupportId==support.id &&
                  Near(s.playerSupportWorldY,support.worldY,.0001f) && s.playerSupportSurface==support.surface &&
                  Near(s.playerHeightDelta,sim::PlayerHeightDelta(support.worldY),.0001f);
            if(!cameraAndSupport)std::cerr<<name<<": pose ("<<s.playerX<<','<<s.playerZ<<','
                <<s.playerYawRadians<<','<<s.playerPitchRadians<<") expected ("<<checkpoint->cameraX<<','
                <<checkpoint->cameraZ<<','<<checkpoint->yaw<<','<<checkpoint->pitch<<") grounded="
                <<s.playerGrounded<<'/'<<support.grounded<<" supportId="
                <<static_cast<int>(s.playerSupportId)<<'/'<<static_cast<int>(support.id)
                <<" supportY="<<s.playerSupportWorldY<<'/'<<support.worldY<<" surface="
                <<static_cast<int>(s.playerSupportSurface)<<'/'<<static_cast<int>(support.surface)
                <<" heightDelta="<<s.playerHeightDelta<<'/'<<sim::PlayerHeightDelta(support.worldY)<<'\n';
            Check(cameraAndSupport,
                  "canonical route camera and player height retain the exact current native terrain support");
        }
    }
    constexpr std::array<std::string_view,11> families{{
        "Boulder01Rock", "MedievalWall02", "TombBoneWax", "TombEarthenware", "TombCharredWick",
        "ChapterTrailDampSand", "ChapterForestGround", "ForestBark", "ForestPine", "ForestMoss", "ForestAlder"}};
    Check(owner.primitives.size() == families.size(),
          "current coalesced chapter retains eleven existing primitive rows");
    if (owner.primitives.size() != families.size()) return;
    Check(owner.triangleRegions.size() == owner.indices.size()/3u,
          "native material consolidation must retain explicit per-triangle region provenance");
    const auto taggedTomb=std::count(owner.triangleRegions.begin(),owner.triangleRegions.end(),
        StaticTriangleRegion::TombDressing);
    const auto taggedExterior=std::count(owner.triangleRegions.begin(),owner.triangleRegions.end(),
        StaticTriangleRegion::ExteriorDressing);
    Check(taggedTomb>0 && taggedExterior>0,
          "mixed candle/urn material families must distinguish tomb and actual graveyard geometry");
    Check(taggedExterior==static_cast<std::ptrdiff_t>(graveyard.triangles + forest.addedTriangles +
              understory.triangles + rocks.triangles + deadwood.triangles + roadClue.triangles),
          "all graveyard, tree, understory, rock, deadwood and road triangles retain exterior ownership");
    for(const auto& primitive:owner.primitives)for(std::size_t n=0;n<primitive.indexCount;n+=3u) {
        if(owner.triangleRegions[(primitive.indexOffset+n)/3u]!=StaticTriangleRegion::ExteriorDressing)continue;
        const auto inside=[&](const auto& minimum,const auto& maximum) {
            for(unsigned corner=0;corner<3;++corner) {
                const auto& point=owner.vertices[primitive.vertexOffset+owner.indices[primitive.indexOffset+n+corner]].position;
                for(unsigned axis=0;axis<3;++axis)
                    if(point[axis]<minimum[axis]-.0001f || point[axis]>maximum[axis]+.0001f)return false;
            }
            return true;
        };
        const bool authored=std::any_of(graveyard.placements.begin(),graveyard.placements.end(),
                [&](const auto& p){return inside(p.minimum,p.maximum);}) ||
            std::any_of(forest.placements.begin(),forest.placements.end(),
                [&](const auto& p){return inside(p.worldBounds.minimum,p.worldBounds.maximum);}) ||
            std::any_of(understory.placements.begin(),understory.placements.end(),
                [&](const auto& p){return inside(p.minimum,p.maximum);}) ||
            std::any_of(rocks.placements.begin(),rocks.placements.end(),
                [&](const auto& p){return inside(p.minimum,p.maximum);}) ||
            std::any_of(deadwood.placements.begin(),deadwood.placements.end(),
                [&](const auto& p){return inside(p.minimum,p.maximum);}) ||
            inside(deadwood.trailRoot.minimum,deadwood.trailRoot.maximum) ||
            std::any_of(roadClue.placements.begin(),roadClue.placements.end(),
                [&](const auto& p){return inside(p.minimum,p.maximum);});
        Check(authored,"exterior provenance must still identify actual authored placements after reordering");
    }
    StaticMeshAsset selected, restored;
    const bool selectionReady=SelectStaticTriangleRegions(owner,2u,selected,diagnostic);
    Check(selectionReady,"triangle-region candidate must retain shared and exterior geometry");
    if(selectionReady) {
        Check(selected.indices.size()+3u*taggedTomb==owner.indices.size() &&
              selected.vertices.size()<owner.vertices.size() && selected.primitives.size()==owner.primitives.size(),
              "real tomb-only triangle omission shrinks buffers while keeping all eleven material families");
        Check(std::count(selected.triangleRegions.begin(),selected.triangleRegions.end(),StaticTriangleRegion::ExteriorDressing)==taggedExterior,
              "graveyard offering and candle triangles cannot disappear with indoor material peers");
        std::size_t target=0;
        for(std::size_t p=0;p<owner.primitives.size();++p) {
            const auto& a=owner.primitives[p];const auto& b=selected.primitives[p];
            Check(a.materialIndex==b.materialIndex,"retained material identity is exact");
            for(std::size_t n=0;n<a.indexCount;n+=3u) {
                const auto tri=(a.indexOffset+n)/3u;
                if(owner.triangleRegions[tri]==StaticTriangleRegion::TombDressing)continue;
                for(unsigned corner=0;corner<3;++corner) {
                    const auto& original=owner.vertices[a.vertexOffset+owner.indices[a.indexOffset+n+corner]];
                    const auto& kept=selected.vertices[b.vertexOffset+selected.indices[target++]];
                    Check(std::memcmp(&original,&kept,sizeof(original))==0,"retained triangle position, normal, UV and tangent bytes are exact");
                }
            }
        }
        Check(target==selected.indices.size(),"every retained native triangle participates exactly once");
        RtStaticMeshSlot selectedSlot;
        Check(selectedSlot.Initialize(std::array{StaticRtAssetRegistration{21,100,
            static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),0,&selected}},diagnostic),
            "triangle-selected native candidate packs within the unchanged one-owner ABI");
        std::cout<<"Triangle-region candidate: retainedExterior="<<taggedExterior<<" omittedTomb="<<taggedTomb
                 <<" vertexBytes="<<selectedSlot.Measurements().vertexBytes<<" indexBytes="<<selectedSlot.Measurements().indexBytes
                 <<"; CPU candidate only, spatial eviction permission not established\n";
    }
    Check(SelectStaticTriangleRegions(owner,kAllStaticTriangleRegions,restored,diagnostic) &&
          restored.indices==owner.indices && restored.vertices.size()==owner.vertices.size() &&
          std::memcmp(restored.vertices.data(),owner.vertices.data(),owner.vertices.size()*sizeof(StaticRtVertex))==0 &&
          restored.primitives.size()==owner.primitives.size() &&
          std::memcmp(restored.primitives.data(),owner.primitives.data(),owner.primitives.size()*sizeof(StaticPrimitiveRecord))==0 &&
          restored.bounds.minimum==owner.bounds.minimum && restored.bounds.maximum==owner.bounds.maximum &&
          restored.nodeTransforms.size()==owner.nodeTransforms.size() && restored.materials.size()==owner.materials.size() &&
          restored.sockets.size()==owner.sockets.size(),
          "complete reverse selection preserves full geometry byte-for-byte");
    for (std::size_t i = 0; i < families.size(); ++i)
        Check(owner.materials.at(owner.primitives[i].materialIndex).name == families[i],
              "actual material families identify preserved shared, tomb and forest boundaries");
    const auto pbr = static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr);
    RtStaticMeshSlot complete, partitioned;
    const bool completeReady = complete.Initialize(
        std::array{StaticRtAssetRegistration{21,100,pbr,0,&owner}}, diagnostic);
    Check(completeReady,
          "real combined chapter static route initializes");
    if (!completeReady) return;
    {
        // Mirror normal admission's selection against the ACTUAL native packed
        // forest owner. No rounded report envelope or asset-name bounds certify
        // the omitted triangle list. This remains host geometry, not GPU proof.
        std::size_t omittedCount=0;bool exterior=true,bitExact=true;
        DetailBox omitted{{INFINITY,INFINITY,INFINITY},{-INFINITY,-INFINITY,-INFINITY}};
        const auto& registryOwner=complete.InstanceMetadata()[21];
        for(std::size_t p=0;p<registryOwner.primitiveCount;++p) {
            const auto& original=owner.primitives[p];
            const auto& name=owner.materials[original.materialIndex].name;
            if(name!="ForestBark" && name!="ForestPine" && name!="ForestMoss" && name!="ForestAlder")continue;
            const auto& primitive=complete.PrimitiveMetadata()[registryOwner.primitiveBase+p];
            for(std::size_t n=0;n<primitive.indexCount;n+=3) {
                bool selected=true;
                for(std::size_t k=0;k<3;++k)
                    selected=selected&&complete.Vertices()[primitive.vertexOffset+complete.Indices()[primitive.indexOffset+n+k]].position[2]>24.f;
                if(!selected)continue;
                ++omittedCount;
                exterior=exterior&&complete.TriangleRegions()[(primitive.indexOffset+n)/3]==StaticTriangleRegion::ExteriorDressing;
                for(std::size_t k=0;k<3;++k) {
                    const auto& v=complete.Vertices()[primitive.vertexOffset+complete.Indices()[primitive.indexOffset+n+k]].position;
                    const auto& source=owner.vertices[original.vertexOffset+owner.indices[original.indexOffset+n+k]].position;
                    for(std::size_t axis=0;axis<3;++axis) {
                        bitExact=bitExact&&std::bit_cast<std::uint32_t>(v[axis])==std::bit_cast<std::uint32_t>(source[axis]);
                        omitted.min[axis]=std::min(omitted.min[axis],double(v[axis]));
                        omitted.max[axis]=std::max(omitted.max[axis],double(v[axis]));
                    }
                }
            }
        }
        Check(omittedCount==84219 && exterior && bitExact,
              "actual ordinary omission roster retains all 84219 exterior-only registry triangles and float-bit source identities");
        Check(DetailRegionalWaterQueriesSeparated(omitted,ChapterRegionalWaterTriangles()),
              "actual packed omission bounds preserve old opaque/source-prism proof plus regional water bounded-query closure");
    }
    {
        auto unclassified = owner;
        unclassified.triangleRegions.clear();
        RtStaticMeshSlot baseline;
        const bool ready = baseline.Initialize(
            std::array{StaticRtAssetRegistration{21,100,pbr,0,&unclassified}}, diagnostic);
        const auto sameBytes=[](const auto& a,const auto& b) {
            return a.size()==b.size() && (a.empty() ||
                std::memcmp(a.data(),b.data(),a.size()*sizeof(a.front()))==0);
        };
        Check(ready && sameBytes(complete.Vertices(),baseline.Vertices()) &&
                  sameBytes(complete.Indices(),baseline.Indices()) &&
                  sameBytes(complete.Materials(),baseline.Materials()) &&
                  sameBytes(complete.PrimitiveMetadata(),baseline.PrimitiveMetadata()) &&
                  sameBytes(complete.PrimitiveVertexCounts(),baseline.PrimitiveVertexCounts()) &&
                  sameBytes(complete.GeometryTransforms(),baseline.GeometryTransforms()) &&
                  sameBytes(complete.InstanceMetadata(),baseline.InstanceMetadata()) &&
                  std::memcmp(&complete.TextureArrayCounts(),&baseline.TextureArrayCounts(),
                      sizeof(complete.TextureArrayCounts()))==0,
              "CPU region provenance must leave every native GPU payload and texture identity byte-identical");
    }
    {
        auto immutable=std::make_shared<const RtStaticMeshSlot>(complete);
        RtStaticMeshCopy copy;
        Check(copy.Begin(immutable,diagnostic,21u,2u),"actual chapter subset begins from the admitted immutable packed owner");
        unsigned turns=0;double maximumMs=0.;
        while(!copy.Ready() && turns++<1024u) {
            const auto start=std::chrono::steady_clock::now();
            if(!copy.Advance(256u*1024u,diagnostic)){Check(false,"actual chapter cached selection advances");break;}
            maximumMs=std::max(maximumMs,std::chrono::duration<double,std::milli>(
                std::chrono::steady_clock::now()-start).count());
            Check(copy.LastCopiedBytes()<=256u*1024u && copy.LastScannedBytes()<=256u*1024u,
                "real chapter copying and selection scans stay within each unchanged byte allowance");
        }
        auto candidate=copy.Take();
        Check(candidate && candidate->Indices().size()+3u*taggedTomb==complete.Indices().size() &&
            candidate->Vertices().size()==complete.Vertices().size() &&
            std::memcmp(candidate->Vertices().data(),complete.Vertices().data(),complete.Measurements().vertexBytes)==0 &&
            candidate->PrimitiveMetadata().size()==complete.PrimitiveMetadata().size() &&
            std::count(candidate->TriangleRegions().begin(),candidate->TriangleRegions().end(),
                StaticTriangleRegion::ExteriorDressing)==taggedExterior,
            "bounded native candidate preserves all vertex bytes, eleven rows and every graveyard triangle");
        if(candidate)for(std::size_t p=0;p<complete.PrimitiveMetadata().size();++p) {
            const auto& a=complete.PrimitiveMetadata()[p];const auto& b=candidate->PrimitiveMetadata()[p];
            std::size_t target=b.indexOffset;
            for(std::size_t n=a.indexOffset;n<a.indexOffset+a.indexCount;n+=3u) {
                if(complete.TriangleRegions()[n/3u]==StaticTriangleRegion::TombDressing)continue;
                for(unsigned i=0;i<3u;++i)Check(complete.Indices()[n+i]==candidate->Indices()[target++],
                    "actual retained local indices and triangle order remain byte-exact");
            }
            Check(target==b.indexOffset+b.indexCount && a.vertexOffset==b.vertexOffset && a.materialIndex==b.materialIndex,
                "native cached selection only changes selected index spans, never material or vertex identity");
        }
        std::cout<<"Cached native selection: turns="<<turns<<" maximumCpuAdvanceMs="<<maximumMs
                 <<" scannedBytes="<<copy.ScannedBytes()<<"; CPU candidate only, normal eviction remains prohibited\n";
    }
    const std::array<StaticRtAssetRegistration,3> ranges{{
        {14,101,pbr,0,&owner,nullptr,RtGeometryRole::Static,0,2},
        {15,102,pbr,0,&owner,nullptr,RtGeometryRole::Static,2,3},
        {16,103,pbr,0,&owner,nullptr,RtGeometryRole::Static,5,6}}};
    const bool partitionReady = partitioned.Initialize(ranges, diagnostic);
    Check(partitionReady,
          "three actual chapter ranges share one packed native asset");
    if (!partitionReady) return;
    Check(complete.Indices() == partitioned.Indices() &&
          complete.Vertices().size() == partitioned.Vertices().size() &&
          std::memcmp(complete.Vertices().data(), partitioned.Vertices().data(),
                      complete.Measurements().vertexBytes) == 0 &&
          complete.Measurements().primitiveMetadataBytes == partitioned.Measurements().primitiveMetadataBytes &&
          complete.Measurements().materialBytes == partitioned.Measurements().materialBytes &&
          complete.TextureArrayCounts().baseColor == partitioned.TextureArrayCounts().baseColor,
          "native owner partition preserves every triangle, vertex byte, material row and atlas allocation");
    std::uint32_t visited = 0;
    for (const auto& range : ranges) {
        const auto& row = partitioned.InstanceMetadata()[range.instanceCustomIndex];
        Check(row.primitiveBase == range.primitiveOffset && row.primitiveCount == range.primitiveCount,
              "actual BLAS-local geometry indices resolve to the intended packed chapter range");
        for (std::uint32_t local = 0; local < row.primitiveCount; ++local) {
            const auto& metadata = partitioned.PrimitiveMetadata()[row.primitiveBase + local];
            const auto& source = owner.primitives[range.primitiveOffset + local];
            Check(metadata.vertexOffset == source.vertexOffset && metadata.indexOffset == source.indexOffset &&
                  metadata.indexCount == source.indexCount && metadata.materialIndex == source.materialIndex,
                  "native per-owner hit decode keeps source positions/indices and exact material identity");
            ++visited;
        }
    }
    Check(visited == owner.primitives.size(),
          "native ownership ranges cover all chapter primitives exactly once");
    const auto verifySelection = [&](std::span<const StaticRtAssetRegistration> selectedRanges,
                                     const char* label) {
        RtStaticMeshSlot candidate;
        const bool ready = candidate.InitializeSelectedGeometry(selectedRanges, diagnostic);
        Check(ready, std::string(label) + " selected native geometry initializes: " + diagnostic);
        if (!ready) return;
        std::uint64_t expectedVertices = 0, expectedIndices = 0;
        std::uint32_t expectedPrimitives = 0;
        for (const auto& range : selectedRanges) {
            const auto& metadata = candidate.InstanceMetadata()[range.instanceCustomIndex];
            const auto selectedCount = range.primitiveSelection.empty()
                ? range.primitiveCount : range.primitiveSelection.size();
            Check(metadata.primitiveCount == selectedCount &&
                  metadata.stableObjectId == range.stableObjectId && metadata.flags == range.flags,
                  "selected native owner retains its logical identity and full geometry count");
            for (std::uint32_t local = 0; local < selectedCount; ++local) {
                const auto sourceIndex = range.primitiveSelection.empty()
                    ? range.primitiveOffset + local : range.primitiveSelection[local];
                const auto& source = owner.primitives[sourceIndex];
                const auto& packed = candidate.PrimitiveMetadata()[metadata.primitiveBase + local];
                const std::size_t sourceEnd = sourceIndex + 1 < owner.primitives.size()
                    ? owner.primitives[sourceIndex + 1].vertexOffset : owner.vertices.size();
                const auto count = sourceEnd - source.vertexOffset;
                const bool inBounds = packed.vertexOffset <= candidate.Vertices().size() &&
                    count <= candidate.Vertices().size() - packed.vertexOffset &&
                    packed.indexOffset <= candidate.Indices().size() &&
                    source.indexCount <= candidate.Indices().size() - packed.indexOffset;
                Check(inBounds, "selected native hit decode stays inside its packed upload payload");
                if (!inBounds) continue;
                Check(packed.indexCount == source.indexCount &&
                      packed.materialIndex == source.materialIndex &&
                      candidate.GeometryTransforms()[metadata.primitiveBase + local] ==
                          complete.GeometryTransforms()[sourceIndex] &&
                      candidate.PrimitiveVertexCounts()[metadata.primitiveBase + local] == count &&
                      std::memcmp(candidate.Vertices().data() + packed.vertexOffset,
                          owner.vertices.data() + source.vertexOffset, count * sizeof(StaticRtVertex)) == 0 &&
                      std::equal(owner.indices.begin() + source.indexOffset,
                          owner.indices.begin() + source.indexOffset + source.indexCount,
                          candidate.Indices().begin() + packed.indexOffset),
                      "every retained native triangle/normal/tangent/UV/local index/material is exact");
                expectedVertices += count; expectedIndices += source.indexCount; ++expectedPrimitives;
            }
        }
        Check(candidate.Vertices().size() == expectedVertices &&
              candidate.Indices().size() == expectedIndices &&
              candidate.PrimitiveMetadata().size() == expectedPrimitives &&
              candidate.Measurements().vertexBytes == expectedVertices * sizeof(StaticRtVertex) &&
              candidate.Measurements().indexBytes == expectedIndices * sizeof(std::uint32_t) &&
              candidate.Measurements().vertexBytes < complete.Measurements().vertexBytes &&
              candidate.Measurements().indexBytes < complete.Measurements().indexBytes,
              "absent native zone geometry costs no candidate vertex/index bytes or hidden padding");
        Check(candidate.Materials().size() == complete.Materials().size() &&
              std::memcmp(candidate.Materials().data(), complete.Materials().data(),
                  complete.Measurements().materialBytes) == 0 &&
              candidate.TextureArrayCounts().baseColor == complete.TextureArrayCounts().baseColor &&
              candidate.TextureArrayCounts().normal == complete.TextureArrayCounts().normal &&
              candidate.TextureArrayCounts().orm == complete.TextureArrayCounts().orm,
              "native candidate keeps shared texture layers/material identities unchanged");
        std::cout << "Native selected geometry " << label << ": primitives=" << expectedPrimitives
                  << " vertexBytes=" << candidate.Measurements().vertexBytes
                  << " indexBytes=" << candidate.Measurements().indexBytes
                  << " removedVertexBytes=" << complete.Measurements().vertexBytes - candidate.Measurements().vertexBytes
                  << " removedIndexBytes=" << complete.Measurements().indexBytes - candidate.Measurements().indexBytes
                  << "; CPU preparation only; common masonry still includes tomb surfaces\n";
    };
    const std::array tombRanges{ranges[0], ranges[1]};
    const std::array forestRanges{ranges[0], ranges[2]};
    verifySelection(tombRanges, "common+tomb");
    verifySelection(forestRanges, "common+forest");
    // Production has no spare primitive or instance row. Disconnected source
    // families must therefore pack into the existing collapse owner, preserving
    // the same BLAS-local decode rather than inventing extra world instances.
    const std::array<std::uint32_t,8> forestFamilies{{0,1,5,6,7,8,9,10}};
    auto forestOwner = StaticRtAssetRegistration{21,100,pbr,0,&owner};
    forestOwner.primitiveSelection = forestFamilies;
    verifySelection(std::array{forestOwner}, "one-owner common+forest");
    auto tombOwner = StaticRtAssetRegistration{21,100,pbr,0,&owner};
    tombOwner.primitiveCount = 5;
    verifySelection(std::array{tombOwner}, "one-owner common+tomb");
    std::cout << "Native chapter ownership: common 2 (includes mixed tomb masonry), tomb 3, forest 6; "
              << "primitiveRows=" << visited << " vertexBytes=" << complete.Measurements().vertexBytes
              << " indexBytes=" << complete.Measurements().indexBytes << "; CPU admission only\n";
}

void TestTriangleRegionFailures()
{
    StaticMeshAsset original;
    original.vertices.resize(9);original.indices={0,1,2,0,1,2,0,1,2};
    original.materials.resize(2);original.materials[0].name="shared-family";original.materials[1].name="indoor-family";
    original.nodeTransforms.resize(1);
    for(unsigned i=0;i<9;++i){original.vertices[i].position[0]=static_cast<float>(i);original.vertices[i].normal[1]=1;original.vertices[i].tangent[0]=1;}
    original.primitives={{0,0,3,0,0},{3,3,3,1,0},{6,6,3,0,0}};
    original.triangleRegions={StaticTriangleRegion::Shared,StaticTriangleRegion::TombDressing,StaticTriangleRegion::ExteriorDressing};
    std::string diagnostic;
    auto reordered=original;
    auto adjacent=original;adjacent.primitives[1].materialIndex=0;
    Check(CoalesceAdjacentWorldBakedPrimitives(adjacent,diagnostic) && adjacent.triangleRegions==original.triangleRegions,
          "adjacent-row coalescing preserves triangle-order provenance");
    Check(ConsolidateWorldBakedMaterialFamilies(reordered,diagnostic) &&
          reordered.triangleRegions==std::vector{StaticTriangleRegion::Shared,StaticTriangleRegion::ExteriorDressing,StaticTriangleRegion::TombDressing},
          "disconnected family consolidation reorders region provenance with its actual triangles");
    StaticMeshAsset selected;
    Check(SelectStaticTriangleRegions(reordered,2,selected,diagnostic) && selected.indices.size()==6 &&
          selected.vertices[3].position[0]==6 && selected.materials.size()==2,
          "disconnected exterior triangles survive exact rebasing beside shared geometry");
    auto unclassified=original;unclassified.triangleRegions.clear();
    Check(SelectStaticTriangleRegions(unclassified,0,selected,diagnostic) && selected.indices==original.indices,
          "missing provenance conservatively retains all unclassified geometry");
    const auto rejects=[&](StaticMeshAsset source,std::uint8_t mask,const char* label) {
        StaticMeshAsset sentinel;sentinel.indices={17};sentinel.vertices.resize(1);sentinel.vertices[0].position[0]=123;
        Check(!SelectStaticTriangleRegions(source,mask,sentinel,diagnostic) && sentinel.indices==std::vector<std::uint32_t>{17} &&
              sentinel.vertices.size()==1 && sentinel.vertices[0].position[0]==123,label);
    };
    rejects(original,4,"unknown retained-region bit fails without overwriting the previous candidate");
    auto allIndoor=original;allIndoor.triangleRegions.assign(3,StaticTriangleRegion::TombDressing);
    rejects(allIndoor,2,"empty retained candidate fails without destroying prior output");
    auto invalid=original;invalid.triangleRegions.push_back(StaticTriangleRegion::Shared);
    rejects(invalid,2,"excess region entries fail atomically");
    invalid=original;invalid.triangleRegions[1]=static_cast<StaticTriangleRegion>(5);
    rejects(invalid,2,"unknown source region fails atomically");
    invalid=original;invalid.indices[3]=UINT32_MAX;
    rejects(invalid,2,"invalid index in even an omitted triangle fails atomically");
    invalid=original;invalid.primitives[1].indexOffset=1;
    rejects(invalid,2,"unaligned overlapping triangle spans fail atomically");
    invalid=original;invalid.primitives[1].vertexOffset=UINT32_MAX;
    rejects(invalid,2,"unaddressable vertex span fails atomically");
    invalid=original;invalid.indices.push_back(0);
    rejects(invalid,2,"unowned trailing index fails atomically");
    Check(!SelectStaticTriangleRegions(original,2,original,diagnostic) && original.indices.size()==9,
          "selection never overwrites its source master");
}

} // namespace

int main()
{
    TestNativeTreePairAndAppend();
    TestCleanFailures();
    TestPreparedMasterOwnership();
    TestSharedMasterPacking();
    TestNativeTerrainAndTrailClearance();
    TestUnsafeRootFootprintsFailAtomically();
    TestNativeChapterPrimitiveOwnership();
    TestTriangleRegionFailures();
    if (failures == 0)
    {
        std::cout << "Native forest tree-pair import, grounding, grouping, and clean-failure checks passed.\n";
        return 0;
    }
    std::cerr << failures << " forest dressing check(s) failed.\n";
    return 1;
}
