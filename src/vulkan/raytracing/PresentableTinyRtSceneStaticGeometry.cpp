#include "vulkan/raytracing/PresentableTinyRtScene.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <limits>
#include <utility>

namespace horde::vulkan::raytracing {

bool PresentableTinyRtScene::PackLoadedStaticAssets(RtStaticMeshSlot& out,
    std::span<const std::uint32_t> retainedWorldFamilies, std::string& diagnostic)
{
    std::vector<StaticRtAssetRegistration> registrations{
        {3u, 0x53574f52u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),
         0u, &developmentStaticAsset_},
        {1u, 0x544f5243u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),
         1u, &productionTorchAsset_},
        {kPlayerWorldBodyInstanceIndex, 0x504c4159u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),
         0u, &productionPlayerAsset_, nullptr, RtGeometryRole::PlayerWorldBody},
        {9u, 0x474c4153u,
         static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr) |
             static_cast<std::uint32_t>(RtInstanceFlag::Transmissive),
         0u, &productionDielectricFixtureAsset_},
        {5u, 0x43484241u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),
         0u, &gothicChestBaseAsset_},
        {6u, 0x43484c44u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),
         0u, &gothicChestLidAsset_, developmentRescueJourney_ ? &gothicChestBaseAsset_ : nullptr},
        {7u, 0x4c4e5247u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),
         0u, &rewardLanternRingAsset_},
        {8u, 0x4c4e4244u,
         static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr) |
             static_cast<std::uint32_t>(RtInstanceFlag::Transmissive),
         0u, &rewardLanternBodyAsset_, developmentRescueJourney_ ? &rewardLanternRingAsset_ : nullptr},
    };
    if (viewmodelAvailable_)
        registrations.push_back({kPlayerViewmodelInstanceIndex, 0x56494557u,
            static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr), 0u,
            &viewmodelAsset_, &productionPlayerAsset_, RtGeometryRole::PlayerViewmodel});
    registrations.push_back({kCollapseInstanceIndex, 0x434f4c4cu,
        static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr), 0u,
        &collapseStaticAsset_});
    // Player Rag torch gets its own metadata/material route. The original
    // production torch remains layer1 for permanent Keeper/world instances.
    registrations.push_back({kPlayerTorchInstanceIndex, 0x544f5243u,
        static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr), 0u,
        &playerTorchAsset_});
    registrations.push_back({kPlayerSwordScabbardMetadataIndex, 0x53434142u,
        static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr), 0u,
        &playerSwordScabbardAsset_});
    if (sceneProfile_ == RtSceneProfile::Showcase)
        registrations.push_back({kWaterDropletMetadataIndex, 0x57415452u,
            static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr) |
                static_cast<std::uint32_t>(RtInstanceFlag::Transmissive), 0u,
            &waterDropletAsset_});
    if (sceneProfile_ == RtSceneProfile::Showcase && developmentRescueJourney_) {
        std::erase_if(registrations, [](const auto& row) { return row.instanceCustomIndex == 9u; });
        if (!chapterActorReady_) { diagnostic = "Loaded chapter actor is required for candidate packing."; return false; }
        registrations.push_back({CharacterRenderSlot::kKeeperMetadataIndex, 0x4b495431u,
            static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr), 0u, &chapterActor_.Asset()});
    }
    for (auto& row : registrations)
        if (row.instanceCustomIndex == kCollapseInstanceIndex) row.primitiveSelection = retainedWorldFamilies;
    // Packing consumes borrowed selections synchronously into owned vectors.
    if (!(developmentRescueJourney_
            ? out.InitializeSelectedGeometry(registrations, diagnostic)
            : out.Initialize(registrations, diagnostic))) return false;
    if (developmentRescueJourney_)
    {
        // Only these immutable opaque chest families opt in. The separately
        // mutable dielectric fixture keeps its own row, as do all other assets.
        std::array<RtImmutableMaterialAlias, 2> aliases{};
        const auto materialRow = [&](const auto& asset, std::uint32_t instance,
                                     const char* name, std::uint32_t& row) {
            const auto& metadata = out.InstanceMetadata()[instance];
            for (std::size_t i = 0; i < asset.primitives.size(); ++i)
                if (asset.materials[asset.primitives[i].materialIndex].name == name)
                {
                    row = out.PrimitiveMetadata()[metadata.primitiveBase + i].materialIndex;
                    return true;
                }
            return false;
        };
        constexpr std::array<const char*, 2> families{{"ChestWood", "BlackIron"}};
        for (std::size_t i = 0; i < families.size(); ++i)
            if (!materialRow(gothicChestBaseAsset_, 5u, families[i], aliases[i].provider) ||
                !materialRow(gothicChestLidAsset_, 6u, families[i], aliases[i].consumer))
            {
                diagnostic = "Chapter immutable chest material roster changed.";
                return false;
            }
        if (!out.ShareImmutableMaterialRows(aliases, diagnostic)) return false;
    }
    return true;
}


namespace {
template<class T> bool SameBytes(const std::vector<T>& a, const std::vector<T>& b) {
    return a.size() == b.size() && (a.empty() || std::memcmp(a.data(), b.data(), a.size()*sizeof(T)) == 0);
}
}

void PresentableTinyRtScene::DestroyPreparedStaticGeometry(PreparedStaticGeometry& owner) noexcept
{
    DestroyAccelerationStructure(owner.tlas);
    DestroyAccelerationStructure(owner.collapse);
    for (Buffer* buffer : {&owner.vertices, &owner.indices, &owner.transforms,
         &owner.instances, &owner.primitives, &owner.tlasInstances,
         &owner.collapseScratch, &owner.tlasScratch}) DestroyBuffer(*buffer);
    owner.ready = false;
}

bool PresentableTinyRtScene::PrepareStaticGeometryReplacement(
    const std::uint64_t expectedGeneration, std::span<const std::uint32_t> retainedWorldFamilies,
    std::string& diagnostic)
{
    if (!ready_ || !developmentRescueJourney_ || sceneProfile_ != RtSceneProfile::Showcase ||
        preparedStaticGeometry_ || staticGeometryCompletionUnproved_ || expectedGeneration == 0 ||
        expectedGeneration != staticGeometryGeneration_ || expectedGeneration == UINT64_MAX ||
        !tlasInstanceDefinitionsValid_ || tlasPendingDefinitionsValid_) {
        diagnostic = "Static geometry preparation requires the current completed chapter owner and no pending candidate.";
        return false;
    }
    const auto start = std::chrono::steady_clock::now();
    auto candidate = std::make_unique<PreparedStaticGeometry>();
    candidate->sourceGeneration = expectedGeneration;
    if (!PackLoadedStaticAssets(candidate->slot, retainedWorldFamilies, diagnostic)) return false;
    const auto& packed = candidate->slot;
    // Every material/texture identity stays shared, including rows unused by a
    // selected zone. There is no hidden texture expansion or layer relocation.
    const auto& oldCounts = staticMeshSlot_.TextureArrayCounts();
    const auto& newCounts = packed.TextureArrayCounts();
    if (!SameBytes(packed.Materials(), staticMeshSlot_.Materials()) ||
        std::memcmp(&oldCounts, &newCounts, sizeof(oldCounts)) != 0 ||
        !SameBytes(packed.Vertices(RtGeometryRole::PlayerWorldBody), staticMeshSlot_.Vertices(RtGeometryRole::PlayerWorldBody)) ||
        !SameBytes(packed.Vertices(RtGeometryRole::PlayerViewmodel), staticMeshSlot_.Vertices(RtGeometryRole::PlayerViewmodel))) {
        diagnostic = "Candidate changes shared material, texture or dedicated player-buffer identity."; return false;
    }
    // Unchanged self-contained BLAS may be reused only with identical local
    // geometry numbering, indices, transforms and source vertices. Kit is an
    // explicitly dynamic owner: its current posed vertices are reuploaded/refit.
    for (std::size_t instance = 0; instance < kRtInstanceMetadataCapacity; ++instance) {
        if (instance == kCollapseInstanceIndex) continue;
        const auto& a = staticMeshSlot_.InstanceMetadata()[instance];
        const auto& b = packed.InstanceMetadata()[instance];
        if (a.primitiveCount != b.primitiveCount || a.geometryRole != b.geometryRole ||
            a.stableObjectId != b.stableObjectId || a.flags != b.flags || a.emitterIndex != b.emitterIndex ||
            a.assetIndex != b.assetIndex || a.reserved1 != b.reserved1) {
            diagnostic = "Candidate changes an unchanged BLAS owner's hit ABI."; return false;
        }
        for (std::uint32_t local = 0; local < a.primitiveCount; ++local) {
            const auto ai = a.primitiveBase + local, bi = b.primitiveBase + local;
            const auto& ap = staticMeshSlot_.PrimitiveMetadata()[ai];
            const auto& bp = packed.PrimitiveMetadata()[bi];
            const auto ac = staticMeshSlot_.PrimitiveVertexCounts()[ai], bc = packed.PrimitiveVertexCounts()[bi];
            const auto& av = staticMeshSlot_.Vertices(static_cast<RtGeometryRole>(a.geometryRole));
            const auto& bv = packed.Vertices(static_cast<RtGeometryRole>(b.geometryRole));
            if (ac != bc || ap.indexCount != bp.indexCount || ap.materialIndex != bp.materialIndex ||
                std::memcmp(staticMeshSlot_.Indices().data()+ap.indexOffset, packed.Indices().data()+bp.indexOffset,
                    ap.indexCount*sizeof(std::uint32_t)) != 0 ||
                staticMeshSlot_.GeometryTransforms()[ai] != packed.GeometryTransforms()[bi] ||
                (instance != CharacterRenderSlot::kKeeperMetadataIndex &&
                 std::memcmp(av.data()+ap.vertexOffset, bv.data()+bp.vertexOffset, ac*sizeof(av.front())) != 0)) {
                diagnostic = "Candidate changes retained BLAS geometry instead of rebuilding its owner."; return false;
            }
        }
    }
    candidate->definitions = tlasBuiltInstances_;
    std::array<RtGpuBufferUpload, 6> uploads{};
    std::array<Buffer, 6> buffers{};
    const VkBufferUsageFlags storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    const VkBufferUsageFlags geometry = storage | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    uploads = {{
        {packed.Vertices().data(), packed.Vertices().size()*sizeof(packed.Vertices().front()), geometry, true, false, "candidate static vertices", VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
        {packed.Indices().data(), packed.Indices().size()*sizeof(std::uint32_t), geometry, true, false, "candidate static indices", VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
        {packed.GeometryTransforms().data(), packed.GeometryTransforms().size()*sizeof(packed.GeometryTransforms().front()), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, true, false, "candidate static transforms", VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
        {packed.InstanceMetadata().data(), sizeof(packed.InstanceMetadata()), storage, false, true, "candidate instance metadata", 0},
        {packed.PrimitiveMetadata().data(), packed.PrimitiveMetadata().size()*sizeof(RtPrimitiveMetadata), storage, false, false, "candidate primitive metadata", VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
        {candidate->definitions.data(), sizeof(candidate->definitions), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, true, true, "candidate TLAS instances", 0}
    }};
    if (!gpuResources_.PrepareBufferUploads(uploads, buffers, diagnostic)) return false;
    candidate->vertices = std::exchange(buffers[0], {});
    candidate->indices = std::exchange(buffers[1], {});
    candidate->transforms = std::exchange(buffers[2], {});
    candidate->instances = std::exchange(buffers[3], {});
    candidate->primitives = std::exchange(buffers[4], {});
    candidate->tlasInstances = std::exchange(buffers[5], {});
    const auto failBuild = [&]() {
        if (staticGeometryCompletionUnproved_) {
            // Do not clean resources referenced by a successfully submitted but
            // unproved command. Platform fatal retirement retains the whole owner.
            preparedStaticGeometry_ = std::move(candidate);
            ready_ = false;
        } else DestroyPreparedStaticGeometry(*candidate);
        return false;
    };
    const auto world = packed.InstanceMetadata()[kCollapseInstanceIndex];
    std::vector<VkAccelerationStructureGeometryKHR> geometries;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges;
    for (std::uint32_t local = 0; local < world.primitiveCount; ++local) {
        const auto index = world.primitiveBase + local;
        const auto& primitive = packed.PrimitiveMetadata()[index];
        VkAccelerationStructureGeometryKHR g{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        g.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        g.flags = (packed.Materials()[primitive.materialIndex].materialFlags[0] & static_cast<std::uint32_t>(RtMaterialFlag::Transmission))
            ? VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR : VK_GEOMETRY_OPAQUE_BIT_KHR;
        auto& t = g.geometry.triangles;
        t.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        t.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        t.vertexData.deviceAddress = candidate->vertices.address + VkDeviceSize(primitive.vertexOffset)*sizeof(horde::scene::assets::StaticRtVertex);
        t.vertexStride = sizeof(horde::scene::assets::StaticRtVertex);
        t.maxVertex = packed.PrimitiveVertexCounts()[index]-1;
        t.indexType = VK_INDEX_TYPE_UINT32;
        t.indexData.deviceAddress = candidate->indices.address + VkDeviceSize(primitive.indexOffset)*sizeof(std::uint32_t);
        t.transformData.deviceAddress = candidate->transforms.address + VkDeviceSize(index)*sizeof(VkTransformMatrixKHR);
        geometries.push_back(g);
        ranges.push_back({primitive.indexCount/3u, 0u, 0u, 0u});
    }
    if (!BuildProfileAccelerationStructure(geometries, ranges, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, candidate->collapse,
        &candidate->collapseScratch, diagnostic, &staticGeometryCompletionUnproved_)) return failBuild();
    const std::uint64_t collapseBuildOwnerBytes = candidate->vertices.allocationSize + candidate->indices.allocationSize +
        candidate->transforms.allocationSize + candidate->instances.allocationSize + candidate->primitives.allocationSize +
        candidate->tlasInstances.allocationSize + candidate->collapse.backing.allocationSize + candidate->collapseScratch.allocationSize;
    DestroyBuffer(candidate->collapseScratch); // Built and completed; not a retained UPDATE owner.
    candidate->definitions[kCollapseInstanceIndex].accelerationStructureReference = candidate->collapse.address;
    if (!WriteBuffer(candidate->tlasInstances, candidate->definitions.data(), sizeof(candidate->definitions),
        "prepared TLAS instances", diagnostic)) return failBuild();
    VkAccelerationStructureGeometryKHR tlasGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    tlasGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    tlasGeometry.geometry.instances.data.deviceAddress = candidate->tlasInstances.address;
    const VkAccelerationStructureBuildRangeInfoKHR tlasRange{tlasInstanceCount_, 0u, 0u, 0u};
    if (!BuildProfileAccelerationStructure(std::span(&tlasGeometry, 1), std::span(&tlasRange, 1),
        VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR,
        candidate->tlas, &candidate->tlasScratch, diagnostic, &staticGeometryCompletionUnproved_)) return failBuild();
    const auto bytes = [](std::initializer_list<const Buffer*> list) {
        std::uint64_t sum=0; for (const auto* buffer : list) sum += buffer->allocationSize; return sum;
    };
    auto& evidence = staticGeometryReplacementMeasurements_;
    evidence = {};
    evidence.oldOwnerAllocationBytes = bytes({&staticVertexBuffer_, &staticIndexBuffer_, &staticGeometryTransformBuffer_,
        &instanceMetadataBuffer_, &primitiveMetadataBuffer_, &instanceBuffer_, &collapseBlas_.backing, &tlas_.backing, &tlasUpdateScratch_});
    evidence.candidateOwnerAllocationBytes = bytes({&candidate->vertices, &candidate->indices, &candidate->transforms,
        &candidate->instances, &candidate->primitives, &candidate->tlasInstances, &candidate->collapse.backing,
        &candidate->tlas.backing, &candidate->tlasScratch});
    evidence.overlapOwnerAllocationBytes = evidence.oldOwnerAllocationBytes + std::max(evidence.candidateOwnerAllocationBytes, collapseBuildOwnerBytes);
    evidence.oldPrimitiveCount = static_cast<std::uint32_t>(staticMeshSlot_.PrimitiveMetadata().size());
    evidence.newPrimitiveCount = static_cast<std::uint32_t>(packed.PrimitiveMetadata().size());
    evidence.prepareMilliseconds = std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    candidate->ready = true;
    preparedStaticGeometry_ = std::move(candidate);
    diagnostic.clear(); return true;
}

bool PresentableTinyRtScene::CommitStaticGeometryWithApi(const std::uint64_t expectedGeneration,
    const StaticCutoverApi& api, std::string& diagnostic)
{
    const auto eligible = [&] {
        return ready_ && !staticGeometryCompletionUnproved_ && expectedGeneration != 0 &&
            expectedGeneration != UINT64_MAX && expectedGeneration == staticGeometryGeneration_ &&
            preparedStaticGeometry_ && preparedStaticGeometry_->ready &&
            preparedStaticGeometry_->sourceGeneration == expectedGeneration && !tlasPendingDefinitionsValid_;
    };
    if (!eligible() || !api.retireReferences || !api.publishDescriptors || !api.destroyRetired) {
        diagnostic = "Static geometry cutover rejected an incomplete or stale candidate/proof."; return false;
    }
    const auto start = std::chrono::steady_clock::now();
    if (!api.retireReferences(api.user, diagnostic)) return false;
    // The platform proof is an external call boundary. Revalidate ownership
    // before changing a descriptor, even if a lifecycle generation changed.
    if (!eligible()) { diagnostic = "Static geometry candidate became stale during retirement proof."; return false; }
    auto retired = std::move(preparedStaticGeometry_);
    const auto exchangeOwners = [&] {
        std::swap(staticMeshSlot_, retired->slot);
        std::swap(staticVertexBuffer_, retired->vertices);
        std::swap(staticIndexBuffer_, retired->indices);
        std::swap(staticGeometryTransformBuffer_, retired->transforms);
        std::swap(instanceMetadataBuffer_, retired->instances);
        std::swap(primitiveMetadataBuffer_, retired->primitives);
        std::swap(instanceBuffer_, retired->tlasInstances);
        std::swap(collapseBlas_, retired->collapse);
        std::swap(tlas_, retired->tlas);
        std::swap(tlasUpdateScratch_, retired->tlasScratch);
    };
    exchangeOwners();
    // The writer rejects its contract BEFORE vkUpdateDescriptorSets. A false
    // result/exception must leave descriptors untouched, allowing both owners
    // and every cache/generation to roll back without retiring either resource.
    bool published = false;
    try { published = api.publishDescriptors(api.user, *this, diagnostic); }
    catch (...) { diagnostic = "Static geometry descriptor preparation threw before publication."; }
    if (!published) {
        exchangeOwners();
        preparedStaticGeometry_ = std::move(retired);
        return false;
    }
    tlasBuiltInstances_ = retired->definitions;
    tlasInstanceDefinitionsValid_ = true;
    tlasPendingInstances_ = {}; tlasPendingDefinitionsValid_ = false;
    for (std::size_t i=0; i<lastInstanceMasks_.size(); ++i) lastInstanceMasks_[i] = static_cast<std::uint8_t>(tlasBuiltInstances_[i].mask);
    chapterActorPoseGeneration_ = UINT64_MAX; chapterActorPoseSeconds_ = -1.0f;
    chapterActorNeedsRefit_ = thirdActorIsCompanion_;
    waterDropletGeometryVisible_ = true; // Forces one write/refit, also for a now-empty pool.
    ++staticGeometryGeneration_;
    api.destroyRetired(api.user, *this, *retired);
    staticGeometryReplacementMeasurements_.cutoverMilliseconds =
        std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    diagnostic.clear(); return true;
}

bool PresentableTinyRtScene::CommitStaticGeometryReplacement(const std::uint64_t expectedGeneration,
    bool (*retireReferences)(void*, std::string&), void* user, std::string& diagnostic)
{
    const StaticCutoverApi api{user, retireReferences,
        [](void*, PresentableTinyRtScene& scene, std::string& error) {
            return scene.WriteBundleDescriptors(scene.pipelineBundle_, error);
        },
        [](void*, PresentableTinyRtScene& scene, PreparedStaticGeometry& retired) noexcept {
            scene.DestroyPreparedStaticGeometry(retired);
        }};
    return CommitStaticGeometryWithApi(expectedGeneration, api, diagnostic);
}

} // namespace horde::vulkan::raytracing
