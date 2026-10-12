#include "vulkan/raytracing/PresentableTinyRtScene.h"
#include "vulkan/raytracing/TlasInstanceRefresh.h"
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
    // Caller has proved completed work (cancel/advance) or platform device idle
    // (scene destruction). Never call this while an ordinary build is pending.
    RetireRtGpuWorkAfterIdle({device_,queue_,commandPool_},owner.buildWork);
    owner.copy.reset();
    owner.uploads.reset();
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
    if (!BeginStaticGeometryReplacement(expectedGeneration, retainedWorldFamilies, diagnostic)) return false;
    while (!StaticGeometryReplacementReady())
        if (!AdvanceStaticGeometryReplacement(expectedGeneration,
                std::numeric_limits<VkDeviceSize>::max(), diagnostic)) return false;
    return true;
}

bool PresentableTinyRtScene::StaticGeometryReplacementReady() const noexcept
{
    return preparedStaticGeometry_ && preparedStaticGeometry_->ready;
}
bool PresentableTinyRtScene::StaticGeometryReplacementHasGpuWork() const noexcept
{
    return preparedStaticGeometry_ && preparedStaticGeometry_->buildWork.submitted;
}
bool PresentableTinyRtScene::RefreshStaticGeometryReplacement(std::uint64_t expectedGeneration,
    std::string& diagnostic)
{
    if(!ready_ || staticGeometryCompletionUnproved_ || !preparedStaticGeometry_ ||
       !preparedStaticGeometry_->ready || preparedStaticGeometry_->buildWork.submitted ||
       !tlasInstanceDefinitionsValid_ || tlasPendingDefinitionsValid_ || expectedGeneration==0 ||
       expectedGeneration!=staticGeometryGeneration_ ||
       preparedStaticGeometry_->sourceGeneration!=expectedGeneration) {
        diagnostic="Candidate pose refresh requires completed current owners and no submitted candidate work.";
        return false;
    }
    // Explicit final-boundary refresh: preserve CPU/uploads/BLAS and queue a
    // single TLAS refit (or BUILD on discrete changes) from the latest submit.
    preparedStaticGeometry_->ready=false;
    preparedStaticGeometry_->phase=PreparedStaticGeometry::Phase::Tlas;
    diagnostic.clear();return true;
}

bool PresentableTinyRtScene::CancelStaticGeometryReplacement(std::string& diagnostic)
{
    if (staticGeometryCompletionUnproved_) {
        diagnostic = "Cannot cancel resources with unproved submitted GPU work."; return false;
    }
    if (preparedStaticGeometry_ && preparedStaticGeometry_->buildWork.submitted) {
        const auto status=PollRtGpuWork({device_,queue_,commandPool_},preparedStaticGeometry_->buildWork,diagnostic);
        if(status==RtGpuWorkStatus::Pending) {
            diagnostic="Cannot cancel a candidate while its AS build is in flight.";return false;
        }
        if(status==RtGpuWorkStatus::Unproved) {
            staticGeometryCompletionUnproved_=true;ready_=false;return false;
        }
    }
    if (preparedStaticGeometry_) DestroyPreparedStaticGeometry(*preparedStaticGeometry_);
    preparedStaticGeometry_.reset(); diagnostic.clear(); return true;
}

bool PresentableTinyRtScene::BeginStaticGeometryReplacement(
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
    candidate->prepareStart = start;
    candidate->sourceGeneration = expectedGeneration;
    staticGeometryReplacementMeasurements_ = {};
    if (retainedWorldFamilies.empty() && fullStaticGeometrySource_) {
        candidate->copy = std::make_unique<RtStaticMeshCopy>();
        if (!candidate->copy->Begin(fullStaticGeometrySource_, diagnostic)) return false;
        candidate->phase = PreparedStaticGeometry::Phase::Copy;
        preparedStaticGeometry_ = std::move(candidate);
        diagnostic.clear(); return true;
    }
    if (!PackLoadedStaticAssets(candidate->slot, retainedWorldFamilies, diagnostic)) return false;
    if (!BeginPreparedStaticUploads(*candidate, diagnostic)) return false;
    staticGeometryReplacementMeasurements_ = {};
    staticGeometryReplacementMeasurements_.packMilliseconds =
        std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    preparedStaticGeometry_ = std::move(candidate);
    diagnostic.clear(); return true;
}

bool PresentableTinyRtScene::BeginPreparedStaticUploads(
    PreparedStaticGeometry& candidate, std::string& diagnostic)
{
    const auto& packed = candidate.slot;
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
    candidate.definitions = tlasBuiltInstances_;
    std::array<RtGpuBufferUpload, 6> uploads{};
    const VkBufferUsageFlags storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    const VkBufferUsageFlags geometry = storage | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    uploads = {{
        {packed.Vertices().data(), packed.Vertices().size()*sizeof(packed.Vertices().front()), geometry, true, false, "candidate static vertices", VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
        {packed.Indices().data(), packed.Indices().size()*sizeof(std::uint32_t), geometry, true, false, "candidate static indices", VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
        {packed.GeometryTransforms().data(), packed.GeometryTransforms().size()*sizeof(packed.GeometryTransforms().front()), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, true, false, "candidate static transforms", VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
        {packed.InstanceMetadata().data(), sizeof(packed.InstanceMetadata()), storage, false, true, "candidate instance metadata", 0},
        {packed.PrimitiveMetadata().data(), packed.PrimitiveMetadata().size()*sizeof(RtPrimitiveMetadata), storage, false, false, "candidate primitive metadata", VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
        {candidate.definitions.data(), sizeof(candidate.definitions), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, true, true, "candidate TLAS instances", 0}
    }};
    candidate.uploads = std::make_unique<RtGpuBufferPreparation>(gpuResources_);
    if (!candidate.uploads->Begin(uploads, diagnostic)) return false;
    candidate.phase = PreparedStaticGeometry::Phase::Upload;
    return true;
}

bool PresentableTinyRtScene::AdvanceStaticGeometryReplacement(
    const std::uint64_t expectedGeneration, const VkDeviceSize maxCopyBytes, std::string& diagnostic)
{
    if (!ready_ || staticGeometryCompletionUnproved_ || !preparedStaticGeometry_ ||
        expectedGeneration == 0u || expectedGeneration != staticGeometryGeneration_ ||
        preparedStaticGeometry_->sourceGeneration != expectedGeneration ||
        preparedStaticGeometry_->ready || !tlasInstanceDefinitionsValid_ ||
        tlasPendingDefinitionsValid_ || maxCopyBytes == 0u) {
        diagnostic = "Static geometry advance rejected stale, pending or invalid preparation."; return false;
    }
    auto* candidate = preparedStaticGeometry_.get();
    const auto& packed = candidate->slot;
    const auto start = std::chrono::steady_clock::now();
    auto& evidence = staticGeometryReplacementMeasurements_;
    const auto completedStep = [&] {
        evidence.maximumAdvanceMilliseconds = std::max(evidence.maximumAdvanceMilliseconds,
            std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
        diagnostic.clear(); return true;
    };
    const auto failBuild = [&]() {
        if(candidate->buildWork.completionUnproved) staticGeometryCompletionUnproved_=true;
        if (staticGeometryCompletionUnproved_) {
            // Retain every owner referenced by unproved submitted GPU work.
            ready_ = false;
        } else {
            DestroyPreparedStaticGeometry(*candidate);
            preparedStaticGeometry_.reset();
        }
        return false;
    };
    if (candidate->phase == PreparedStaticGeometry::Phase::Copy) {
        if (!candidate->copy->Advance(static_cast<std::size_t>(std::min<VkDeviceSize>(
                maxCopyBytes, std::numeric_limits<std::size_t>::max())), diagnostic)) return failBuild();
        ++evidence.cpuCopyAdvanceCount;
        evidence.copiedCpuPayloadBytes += candidate->copy->LastCopiedBytes();
        if (candidate->copy->Ready()) {
            auto copied = candidate->copy->Take();
            candidate->slot = std::move(*copied);
            candidate->copy.reset();
            if (!BeginPreparedStaticUploads(*candidate, diagnostic)) return failBuild();
        }
        evidence.packMilliseconds += std::chrono::duration<double,std::milli>(
            std::chrono::steady_clock::now()-start).count();
        return completedStep();
    }
    if (candidate->phase == PreparedStaticGeometry::Phase::Upload) {
        if (!candidate->uploads->Advance(maxCopyBytes, diagnostic)) return failBuild();
        ++evidence.uploadAdvanceCount;
        evidence.uploadedBytes += candidate->uploads->LastAdvanceCopiedBytes();
        if (candidate->uploads->Status() != RtGpuBufferPreparation::State::Ready) return completedStep();
        std::array<Buffer, 6> buffers{};
        if (!candidate->uploads->Take(buffers, diagnostic)) return failBuild();
        candidate->vertices = std::exchange(buffers[0], {});
        candidate->indices = std::exchange(buffers[1], {});
        candidate->transforms = std::exchange(buffers[2], {});
        candidate->instances = std::exchange(buffers[3], {});
        candidate->primitives = std::exchange(buffers[4], {});
        candidate->tlasInstances = std::exchange(buffers[5], {});
        candidate->uploads.reset();
        candidate->phase = PreparedStaticGeometry::Phase::Collapse;
        return completedStep();
    }
    if (candidate->phase == PreparedStaticGeometry::Phase::Collapse) {
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
            &candidate->collapseScratch, diagnostic, &staticGeometryCompletionUnproved_, &candidate->buildWork)) return failBuild();
        ++evidence.asSubmissionCount;
        candidate->collapseBuildOwnerBytes = candidate->vertices.allocationSize + candidate->indices.allocationSize +
            candidate->transforms.allocationSize + candidate->instances.allocationSize + candidate->primitives.allocationSize +
            candidate->tlasInstances.allocationSize + candidate->collapse.backing.allocationSize + candidate->collapseScratch.allocationSize;
        candidate->phase = PreparedStaticGeometry::Phase::CollapseWait;
        return completedStep();
    }
    if (candidate->phase == PreparedStaticGeometry::Phase::CollapseWait ||
        candidate->phase == PreparedStaticGeometry::Phase::TlasWait) {
        const auto status=PollRtGpuWork({device_,queue_,commandPool_},candidate->buildWork,diagnostic);
        ++evidence.asPollCount;
        if(status==RtGpuWorkStatus::Pending) {++evidence.asPendingPollCount;return completedStep();}
        if(status!=RtGpuWorkStatus::Complete) {
            staticGeometryCompletionUnproved_=true;return failBuild();
        }
        if(candidate->phase==PreparedStaticGeometry::Phase::TlasWait) {
            candidate->phase=PreparedStaticGeometry::Phase::Ready;
        }else {
            DestroyBuffer(candidate->collapseScratch); // Fence-proved completed build; not an UPDATE owner.
            candidate->phase = PreparedStaticGeometry::Phase::Tlas;
            return completedStep();
        }
    }
    if(candidate->phase==PreparedStaticGeometry::Phase::Tlas) {
    // Gameplay may have advanced during uploads. Build against the latest
    // submitted definitions; never restore preparation-time actor/held poses.
    auto latest = tlasBuiltInstances_;
    latest[kCollapseInstanceIndex].accelerationStructureReference = candidate->collapse.address;
    const bool rebuild=!candidate->tlas.handle || RequiresTlasInstanceRebuild(candidate->definitions,latest);
    candidate->definitions = latest;
    if (!WriteBuffer(candidate->tlasInstances, candidate->definitions.data(), sizeof(candidate->definitions),
        "prepared TLAS instances", diagnostic)) return failBuild();
    VkAccelerationStructureGeometryKHR tlasGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    tlasGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    tlasGeometry.geometry.instances.data.deviceAddress = candidate->tlasInstances.address;
    const VkAccelerationStructureBuildRangeInfoKHR tlasRange{tlasInstanceCount_, 0u, 0u, 0u};
    if (!candidate->tlas.handle) {
    if (!BuildProfileAccelerationStructure(std::span(&tlasGeometry, 1), std::span(&tlasRange, 1),
        VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR,
        candidate->tlas, &candidate->tlasScratch, diagnostic, &staticGeometryCompletionUnproved_, &candidate->buildWork)) return failBuild();
    }else {
        // A completed candidate may refresh only its TLAS, behind its own
        // fence; do not reallocate its AS, restart uploads or retire geometry.
        VkAccelerationStructureBuildGeometryInfoKHR update{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        update.type=VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        update.flags=VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR|VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        update.mode=rebuild?VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR:VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
        update.srcAccelerationStructure=rebuild?VK_NULL_HANDLE:candidate->tlas.handle;
        update.dstAccelerationStructure=candidate->tlas.handle;
        update.geometryCount=1;update.pGeometries=&tlasGeometry;
        update.scratchData.deviceAddress=candidate->tlasScratch.AlignedAddress();
        const VkAccelerationStructureBuildRangeInfoKHR* range=&tlasRange;
        struct Refresh {PresentableTinyRtScene* scene;VkAccelerationStructureBuildGeometryInfoKHR* update;
            const VkAccelerationStructureBuildRangeInfoKHR** range;} data{this,&update,&range};
        if(!SubmitRtGpuWork({device_,queue_,commandPool_},candidate->buildWork,
            [](VkCommandBuffer command,void* user){
                const auto& data=*static_cast<Refresh*>(user);
                data.scene->vkCmdBuildAccelerationStructuresKHR_(command,1,data.update,data.range);
                VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
                barrier.srcAccessMask=VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
                barrier.dstAccessMask=VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
                vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                    data.scene->executionPolicy_.shaderPipelineStage,0,1,&barrier,0,nullptr,0,nullptr);
            },&data,diagnostic))return failBuild();
    }
    ++evidence.asSubmissionCount;
    candidate->phase=PreparedStaticGeometry::Phase::TlasWait;
    return completedStep();
    }
    const auto bytes = [](std::initializer_list<const Buffer*> list) {
        std::uint64_t sum=0; for (const auto* buffer : list) sum += buffer->allocationSize; return sum;
    };
    evidence.oldOwnerAllocationBytes = bytes({&staticVertexBuffer_, &staticIndexBuffer_, &staticGeometryTransformBuffer_,
        &instanceMetadataBuffer_, &primitiveMetadataBuffer_, &instanceBuffer_, &collapseBlas_.backing, &tlas_.backing, &tlasUpdateScratch_});
    evidence.candidateOwnerAllocationBytes = bytes({&candidate->vertices, &candidate->indices, &candidate->transforms,
        &candidate->instances, &candidate->primitives, &candidate->tlasInstances, &candidate->collapse.backing,
        &candidate->tlas.backing, &candidate->tlasScratch});
    evidence.overlapOwnerAllocationBytes = evidence.oldOwnerAllocationBytes + std::max(evidence.candidateOwnerAllocationBytes, candidate->collapseBuildOwnerBytes);
    evidence.oldPrimitiveCount = static_cast<std::uint32_t>(staticMeshSlot_.PrimitiveMetadata().size());
    evidence.newPrimitiveCount = static_cast<std::uint32_t>(packed.PrimitiveMetadata().size());
    evidence.prepareMilliseconds = std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-candidate->prepareStart).count();
    candidate->ready = true;
    candidate->phase = PreparedStaticGeometry::Phase::Ready;
    return completedStep();
}

bool PresentableTinyRtScene::CommitStaticGeometryWithApi(const std::uint64_t expectedGeneration,
    const StaticCutoverApi& api, std::string& diagnostic)
{
    const auto eligible = [&] {
        const bool owned = ready_ && !staticGeometryCompletionUnproved_ && expectedGeneration != 0 &&
            expectedGeneration != UINT64_MAX && expectedGeneration == staticGeometryGeneration_ &&
            preparedStaticGeometry_ && preparedStaticGeometry_->ready &&
            !preparedStaticGeometry_->buildWork.submitted &&
            preparedStaticGeometry_->sourceGeneration == expectedGeneration &&
            tlasInstanceDefinitionsValid_ && !tlasPendingDefinitionsValid_;
        if (!owned) return false;
        // A moving first frame cannot use a preparation-time pose as its
        // displayed TLAS. The separate recorder path must stage that frame's
        // complete dynamic definitions before tracing or retiring old owners.
        // Snapshot-only cutovers retain their exact submitted-pose guard.
        if (api.recordCurrentFrame) return true;
        auto latest = tlasBuiltInstances_;
        latest[kCollapseInstanceIndex].accelerationStructureReference = preparedStaticGeometry_->collapse.address;
        return std::memcmp(latest.data(), preparedStaticGeometry_->definitions.data(), sizeof(latest)) == 0;
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
    if (api.recordCurrentFrame) {
        bool recorded = false;
        try { recorded = api.recordCurrentFrame(api.user, *this, diagnostic); }
        catch (...) { diagnostic = "Static geometry first-frame recording threw."; }
        const bool currentWorld = tlasPendingDefinitionsValid_ &&
            tlasPendingInstances_[kCollapseInstanceIndex].accelerationStructureReference == collapseBlas_.address;
        if (!recorded || !currentWorld || staticGeometryGeneration_ != expectedGeneration) {
            // Commands may already reference the new resources. Do not roll
            // descriptors back underneath that recording or free either owner.
            // The platform must retire/reset recorded work before teardown.
            if (diagnostic.empty()) diagnostic = "Static geometry first frame lacks current recorded-world definitions.";
            ready_ = false;
            staticGeometryCompletionUnproved_ = true;
            tlasPendingDefinitionsValid_ = false;
            retired->ready = false;
            preparedStaticGeometry_ = std::move(retired);
            return false;
        }
    }
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

bool PresentableTinyRtScene::RecordTraceAndCopyWithStaticReplacement(
    const std::uint64_t expectedGeneration,
    bool (*retireReferences)(void*, std::string&), void* user,
    VkCommandBuffer commandBuffer, VkImage swapchainImage,
    VkImageLayout& swapchainImageLayout, VkExtent2D swapchainExtent,
    const RtSceneFrameInputs& frame, std::string& diagnostic,
    RtSceneRecordObservation* observation
#if HORDE_RT_STAGED_PRIMARY_TIMING
    , experimental::StagedPrimaryTiming* stagedTiming, std::uint32_t timingFrameSlot
#endif
    )
{
    if (!retireReferences || commandBuffer == VK_NULL_HANDLE || swapchainImage == VK_NULL_HANDLE ||
        swapchainExtent.width == 0 || swapchainExtent.height == 0) {
        diagnostic = "Static replacement requires an actual first-frame command, output and retirement proof.";
        return false;
    }
    struct FirstFrame {
        bool (*proof)(void*, std::string&);
        void* user;
        VkCommandBuffer command;
        VkImage image;
        VkImageLayout* layout;
        VkExtent2D extent;
        const RtSceneFrameInputs* frame;
        RtSceneRecordObservation* observation;
#if HORDE_RT_STAGED_PRIMARY_TIMING
        experimental::StagedPrimaryTiming* stagedTiming;
        std::uint32_t timingFrameSlot;
#endif
    } first{retireReferences,user,commandBuffer,swapchainImage,&swapchainImageLayout,
        swapchainExtent,&frame,observation
#if HORDE_RT_STAGED_PRIMARY_TIMING
        ,stagedTiming,timingFrameSlot
#endif
    };
    const StaticCutoverApi api{&first,
        [](void* u,std::string& error) {
            const auto& f=*static_cast<FirstFrame*>(u);return f.proof(f.user,error);
        },
        [](void*,PresentableTinyRtScene& scene,std::string& error) {
            return scene.WriteBundleDescriptors(scene.pipelineBundle_,error);
        },
        [](void*,PresentableTinyRtScene& scene,PreparedStaticGeometry& retired) noexcept {
            scene.DestroyPreparedStaticGeometry(retired);
        },
        [](void* u,PresentableTinyRtScene& scene,std::string& error) {
            const auto& f=*static_cast<FirstFrame*>(u);
            return scene.RecordTraceAndCopy(f.command,f.image,*f.layout,f.extent,*f.frame,error,f.observation
#if HORDE_RT_STAGED_PRIMARY_TIMING
                ,f.stagedTiming,f.timingFrameSlot
#endif
            );
        }};
    return CommitStaticGeometryWithApi(expectedGeneration,api,diagnostic);
}

} // namespace horde::vulkan::raytracing
