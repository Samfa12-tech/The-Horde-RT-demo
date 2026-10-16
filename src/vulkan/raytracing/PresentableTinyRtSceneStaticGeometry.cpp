#include "vulkan/raytracing/PresentableTinyRtScene.h"
#include "vulkan/raytracing/TlasInstanceRefresh.h"
#include "scene/assets/StaticTriangleSelection.h"
#include "scene/CorridorStaticDetail.h"
#include "gameplay/simulation/GameSimulation.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <limits>
#include <utility>

namespace horde::vulkan::raytracing {

void PresentableTinyRtScene::AdmitOrdinaryStaticDetail()
{
    ordinaryStaticOmissions_.reset();
    if(!fullStaticGeometrySource_ || !developmentRescueJourney_ || sceneProfile_!=RtSceneProfile::Showcase)return;
    const auto& source=*fullStaticGeometrySource_;
    constexpr std::array<std::uint32_t,2> transmissionOwners{{8u,kWaterDropletMetadataIndex}};
    if(!source.TransmissionOwnersWithin(transmissionOwners))return;
    const auto& owner=source.InstanceMetadata()[kCollapseInstanceIndex];
    if(owner.primitiveCount!=collapseStaticAsset_.primitives.size())return;
    std::vector<std::uint32_t> ids;
    horde::scene::DetailBox bounds{{INFINITY,INFINITY,INFINITY},{-INFINITY,-INFINITY,-INFINITY}};
    for(unsigned p=0;p<owner.primitiveCount;++p) {
        const auto& local=collapseStaticAsset_.primitives[p];
        const auto& material=collapseStaticAsset_.materials[local.materialIndex];
        if(material.name!="ForestBark" && material.name!="ForestPine" && material.name!="ForestMoss" && material.name!="ForestAlder")continue;
        const auto& primitive=source.PrimitiveMetadata()[owner.primitiveBase+p];
        for(std::size_t i=primitive.indexOffset;i<primitive.indexOffset+primitive.indexCount;i+=3) {
            bool selected=true;
            for(unsigned k=0;k<3;++k) {
                const auto& v=source.Vertices()[primitive.vertexOffset+source.Indices()[i+k]].position;
                for(unsigned axis=0;axis<3;++axis)if(!std::isfinite(v[axis]))return;
                selected&=v[2]>24.f;
            }
            if(!selected)continue;
            if(source.TriangleRegions()[i/3]!=horde::scene::assets::StaticTriangleRegion::ExteriorDressing)return;
            ids.push_back(static_cast<std::uint32_t>(i/3));
            for(unsigned k=0;k<3;++k)for(unsigned axis=0;axis<3;++axis) {
                const double value=source.Vertices()[primitive.vertexOffset+source.Indices()[i+k]].position[axis];
                bounds.min[axis]=std::min(bounds.min[axis],value);bounds.max[axis]=std::max(bounds.max[axis],value);
            }
        }
    }
    if(ids.size()!=84219 || !horde::scene::DetailForestReceiverEnvelopeBlocked(bounds))return;
    // Enclose the actual admitted panes around their body origin. All other
    // transmission owners stay loaded. Refuse a changed lantern geometry or
    // attachment rather than assuming an old master still bounds its queries.
    const auto* grip=horde::gameplay::items::FindHeldItemSocket(rewardLanternRingAsset_.sockets,"GripRing");
    const auto* hinge=horde::gameplay::items::FindHeldItemSocket(rewardLanternRingAsset_.sockets,"Hinge");
    if(!grip || !hinge)return;
    const auto* light=horde::gameplay::items::FindHeldItemSocket(rewardLanternBodyAsset_.sockets,"Light");
    if(!light)return;
    double lightRadius=0;
    for(unsigned axis=12;axis<15;++axis)lightRadius+=double(light->world[axis])*light->world[axis];
    if(!std::isfinite(lightRadius) || lightRadius>.515001*.515001)return;
    double hingeRadius=0;
    for(unsigned axis=12;axis<15;++axis)hingeRadius+=std::pow(double(grip->world[axis])-hinge->world[axis],2);
    if(!std::isfinite(hingeRadius) || hingeRadius>.097001*.097001)return;
    for(const auto& p:rewardLanternBodyAsset_.primitives) {
        if(!(rewardLanternBodyAsset_.materials[p.materialIndex].flags&static_cast<unsigned>(RtMaterialFlag::Transmission)))continue;
        for(std::size_t i=p.indexOffset;i<p.indexOffset+p.indexCount;++i) {
            const auto& v=rewardLanternBodyAsset_.vertices[p.vertexOffset+rewardLanternBodyAsset_.indices[i]].position;
            const double radius=double(v[0])*v[0]+double(v[1])*v[1]+double(v[2])*v[2];
            if(!std::isfinite(radius) || radius>.736324*.736324)return;
        }
    }
    std::string error;
    ordinaryStaticOmissions_=RtStaticTriangleOmissions::Create(fullStaticGeometrySource_,kCollapseInstanceIndex,ids,error);
}

namespace {
bool CorridorFrameSourcesInside(const RtSceneFrameInputs& f) {
    using namespace horde::gameplay;
    if(!f.developmentRescueJourney || f.rescue.exteriorSide || f.rescue.equipmentStowed ||
       f.playerSupportWorldY!=kRouteFloorWorldY || f.playerMountProfile!=items::PlayerMountProfile::AnatomicalBody ||
       f.playerRenderRoute!=kProductionPlayerRenderRoute ||
       f.interaction.heldLightKind!=interactions::HeldLightKind::RewardLantern ||
       f.chestReward.phase!=interactions::ChestRewardPhase::LanternClaimed ||
       f.lich.deathDissolveProgress!=1.f || f.lich.staffLightStrength!=0.f ||
       f.tuning.glassFixtureVisible || f.tuning.productionRewardPropsVisible || f.previewMotion)return false;
    // Moving transmissive drops are not part of this first static certificate.
    // Keep the full owner until they expire; never delete or mute the effect.
    if(std::any_of(f.waterContact.droplets.begin(),f.waterContact.droplets.end(),[](const auto& d){return d.active;}))return false;
    // 0.404 = native .736324*.44 + .097001*.44 + two validated
    // 15 mm attachment tolerances + 6 mm dielectric spawn + outward reserve.
    // It encloses every pane continuation and all lantern light area samples.
    // Reverse light->candidate segments cross the same retained faces, so
    // point-light shadows are preserved for arbitrary receiver distance. The
    // closest area-light endpoint is >34 mm from a face (>20 mm end trim).
    const auto& hand=f.rewardLanternWorldFromHinge;
    if(!horde::scene::DetailSourceSphereInside({{hand[12],hand[13],hand[14]}},.404))return false;
    // Enclose both 35 mm lateral and 17.5 mm centred vertical camera bob.
    if(!horde::scene::DetailSourceSphereInside({{f.cameraX,.7175f,f.cameraZ}},.040))return false;
    for(std::size_t i=0;i<f.fireEmitterCount;++i) {
        if(i>=f.fireEmitters.size())return false;
        const auto& e=f.fireEmitters[i];
        if(e.strength>.001f && e.fuel>.001f && e.parentObject!=effects::FireEmitterParentObject::RewardLantern)return false;
    }
    // The three active passage lamp endpoints, including all four area offsets,
    // are inside the source prism. The fourth bay is outside this lease.
    return f.cameraX>=-27.5f && f.cameraX<=-16.6f && f.cameraZ>=-16.55f && f.cameraZ<=-13.85f;
}
}

bool PresentableTinyRtScene::RecoverOrdinaryStaticPreparation(bool safe,StaticDetailFrame& decision,std::string& diagnostic)
{
    ++ordinaryStaticDetailEvidence_.preparationFailures;
    if(staticGeometryCompletionUnproved_ || preparedStaticGeometry_)return false;
    ordinaryStaticCandidate_=false;
    if(!ordinaryStaticReduced_) {
        ordinaryStaticRearm_=false; // Full owner remains valid; no per-frame retry storm.
        decision=StaticDetailFrame::Render;diagnostic.clear();return true;
    }
    // One fresh bounded restore attempt. If allocation/build fails again, keep
    // the safe-side lease and old owner and report the real failure explicitly.
    if(++ordinaryStaticRestoreFailures_>1u)return false;
    decision=safe?StaticDetailFrame::Render:StaticDetailFrame::WaitForRestore;
    diagnostic.clear();return true;
}

bool PresentableTinyRtScene::AdvanceOrdinaryStaticDetail(
    horde::gameplay::simulation::GameSimulation& simulation,const RtSceneFrameInputs& frame,
    bool allowEviction,StaticDetailFrame& decision,std::string& diagnostic)
{
    decision=StaticDetailFrame::Render;
    if(!ordinaryStaticReduced_ && !ordinaryStaticCandidate_ && StaticGeometryHasCompleteContributors() &&
       simulation.Snapshot().staticDetailLease.resourceGeneration)
        simulation.ReleaseStaticDetailLease(simulation.Snapshot().staticDetailLease.resourceGeneration);
    const auto snapshot=simulation.Snapshot();
    constexpr float identity[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};
    const bool safe=ordinaryStaticFacesVerified_ && ordinaryStaticOmissions_ &&
        staticMeshSlot_.TransmissionOwnersWithin(std::array<std::uint32_t,2>{{8u,kWaterDropletMetadataIndex}}) &&
        tlasInstanceDefinitionsValid_ && tlasBuiltInstances_[0].accelerationStructureReference==blas_.address &&
        (tlasBuiltInstances_[0].mask&0x01u)!=0 &&
        std::memcmp(tlasBuiltInstances_[0].transform.matrix,identity,sizeof(identity))==0 &&
        playerCpuSkinCadence_==PlayerCpuSkinCadence::Hz60 && !snapshot.developmentWorldRoute &&
        !snapshot.developmentSupportFixture && CorridorFrameSourcesInside(frame);
    const bool inner=safe && frame.cameraX>=-24.0f && frame.cameraX<=-20.0f;
    if(frame.cameraX< -25.f || frame.cameraX> -19.f)ordinaryStaticRearm_=true;
    const bool restore=ordinaryStaticReduced_ && (!allowEviction || !safe ||
        frame.cameraX< -22.5f || frame.cameraX> -21.5f || snapshot.staticDetailLease.restoreRequested);
#if HORDE_RT_REGION_PAGE_PROTOTYPE
    if(regionPageSource_)return AdvanceRegionPage(simulation,frame,safe,inner,allowEviction,decision,diagnostic);
#endif
    if(!ordinaryStaticCandidate_ && preparedStaticGeometry_) {
        // An explicit diagnostic owns its transaction, but does not make an
        // already reduced live owner safe for a changed source/fixture state.
        if(ordinaryStaticReduced_ && !safe) {
            decision=StaticDetailFrame::WaitForRestore;++ordinaryStaticDetailEvidence_.waitFrames;
        }
        return true;
    }
    if(!ordinaryStaticCandidate_ && ((allowEviction && inner && ordinaryStaticRearm_ && !ordinaryStaticReduced_) || restore)) {
        ordinaryStaticCandidateReduced_=!ordinaryStaticReduced_;
        if(!BeginStaticGeometryReplacement(staticGeometryGeneration_,{},diagnostic,
            horde::scene::assets::kAllStaticTriangleRegions,ordinaryStaticCandidateReduced_?ordinaryStaticOmissions_:nullptr))
            return RecoverOrdinaryStaticPreparation(safe,decision,diagnostic);
        ordinaryStaticCandidate_=true;
    }
    if(ordinaryStaticCandidate_ && ordinaryStaticCandidateReduced_ && (!allowEviction || !safe)) {
        if(!CancelStaticGeometryReplacement(diagnostic)) {
            if(staticGeometryCompletionUnproved_)return false;
            diagnostic.clear();return true; // candidate AS fence is still pending
        }
        ordinaryStaticCandidate_=false;++ordinaryStaticDetailEvidence_.cancelled;
    }
    if(ordinaryStaticCandidate_) {
        if(!StaticGeometryReplacementReady() && !AdvanceStaticGeometryReplacement(staticGeometryGeneration_,256u*1024u,diagnostic))
            return RecoverOrdinaryStaticPreparation(safe,decision,diagnostic);
        if(StaticGeometryReplacementReady()) {
            if(ordinaryStaticCandidateReduced_) {
                const horde::gameplay::simulation::StaticDetailLease lease{staticGeometryGeneration_,snapshot.worldRoute.generation,
                    -27.5f,-16.6f,-16.55f,-13.85f,false};
                if(!safe || !simulation.HoldStaticDetailLease(lease)) {
                    if(!CancelStaticGeometryReplacement(diagnostic))return false;
                    ordinaryStaticCandidate_=false;++ordinaryStaticDetailEvidence_.cancelled;return true;
                }
            }
            decision=StaticDetailFrame::Cutover;return true;
        }
    }
    // Unknown/discontinuous states get no trace with missing contributors. The
    // platform consumes any acquisition without presenting and continues only
    // bounded restoration. The prior successfully presented RT image remains.
    if(ordinaryStaticReduced_ && !safe) {
        decision=StaticDetailFrame::WaitForRestore;++ordinaryStaticDetailEvidence_.waitFrames;
    }
    diagnostic.clear();return true;
}

void PresentableTinyRtScene::FinishOrdinaryStaticDetail(horde::gameplay::simulation::GameSimulation& simulation)
{
#if HORDE_RT_REGION_PAGE_PROTOTYPE
    if(regionPageSource_) {
        if(regionPageVisible_ && StaticGeometryHasCompleteContributors())
            simulation.ReleaseStaticDetailLease(simulation.Snapshot().staticDetailLease.resourceGeneration);
        return;
    }
#endif
    if(!ordinaryStaticCandidate_ || preparedStaticGeometry_)return;
    ordinaryStaticCandidate_=false;
    ordinaryStaticRestoreFailures_=0;
    ordinaryStaticReduced_=!StaticGeometryHasCompleteContributors();
    auto& e=ordinaryStaticDetailEvidence_;
    e.peakOverlapOwnerBytes=std::max(e.peakOverlapOwnerBytes,staticGeometryReplacementMeasurements_.overlapOwnerAllocationBytes);
    if(ordinaryStaticReduced_) {
        ++e.unloads;e.omittedTriangles=ordinaryStaticOmissions_->Triangles().size();
        const auto& r=staticGeometryReplacementMeasurements_;
        e.savedOwnerBytes=r.oldOwnerAllocationBytes>r.candidateOwnerAllocationBytes?r.oldOwnerAllocationBytes-r.candidateOwnerAllocationBytes:0;
        e.peakSavedOwnerBytes=std::max(e.peakSavedOwnerBytes,e.savedOwnerBytes);
    }else {
        ordinaryStaticRearm_=false;
        ++e.restores;e.savedOwnerBytes=0;e.omittedTriangles=0;
        simulation.ReleaseStaticDetailLease(simulation.Snapshot().staticDetailLease.resourceGeneration);
    }
}

bool PresentableTinyRtScene::PackLoadedStaticAssets(RtStaticMeshSlot& out,
    std::span<const std::uint32_t> retainedWorldFamilies, std::string& diagnostic,
    const std::uint8_t retainedTriangleRegions)
{
    horde::scene::assets::StaticMeshAsset regionSelectedWorld;
    if(retainedTriangleRegions!=horde::scene::assets::kAllStaticTriangleRegions &&
       !horde::scene::assets::SelectStaticTriangleRegions(collapseStaticAsset_,retainedTriangleRegions,
            regionSelectedWorld,diagnostic))return false;
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
        if (row.instanceCustomIndex == kCollapseInstanceIndex) {
            row.primitiveSelection = retainedWorldFamilies;
            if(retainedTriangleRegions!=horde::scene::assets::kAllStaticTriangleRegions)row.asset=&regionSelectedWorld;
        }
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

std::shared_ptr<const RtStaticTriangleOmissions> PresentableTinyRtScene::CreateStaticGeometryTriangleOmissions(
    std::uint64_t expectedGeneration, std::span<const std::uint32_t> triangles,
    std::string& diagnostic) const
{
    if(!developmentRescueJourney_ || sceneProfile_!=RtSceneProfile::Showcase || !fullStaticGeometrySource_ ||
       staticGeometryCompletionUnproved_ || !expectedGeneration || expectedGeneration==UINT64_MAX ||
       expectedGeneration!=staticGeometryGeneration_) {
        diagnostic="Triangle omission admission requires the current chapter's complete immutable source.";return {};
    }
    return RtStaticTriangleOmissions::Create(fullStaticGeometrySource_,kCollapseInstanceIndex,triangles,diagnostic);
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
    std::string& diagnostic, const std::uint8_t retainedTriangleRegions,
    std::shared_ptr<const RtStaticTriangleOmissions> omissions)
{
#if HORDE_RT_REGION_PAGE_PROTOTYPE
    if(regionPageSource_ || !regionPageOwner_.Empty()) {
        diagnostic="Combined static preparation cannot replace an independent region-page owner.";return false;
    }
#endif
    if (!ready_ || !developmentRescueJourney_ || sceneProfile_ != RtSceneProfile::Showcase ||
        preparedStaticGeometry_ || staticGeometryCompletionUnproved_ || expectedGeneration == 0 ||
        expectedGeneration != staticGeometryGeneration_ || expectedGeneration == UINT64_MAX ||
        !tlasInstanceDefinitionsValid_ || tlasPendingDefinitionsValid_ ||
        retainedTriangleRegions>horde::scene::assets::kAllStaticTriangleRegions ||
        (retainedTriangleRegions!=horde::scene::assets::kAllStaticTriangleRegions && !retainedWorldFamilies.empty()) ||
        (omissions && (!fullStaticGeometrySource_ || !retainedWorldFamilies.empty() ||
            retainedTriangleRegions!=horde::scene::assets::kAllStaticTriangleRegions))) {
        diagnostic = "Static geometry preparation requires the current completed chapter owner and no pending candidate.";
        return false;
    }
    const auto start = std::chrono::steady_clock::now();
    auto candidate = std::make_unique<PreparedStaticGeometry>();
    candidate->prepareStart = start;
    candidate->sourceGeneration = expectedGeneration;
    candidate->retainedTriangleRegions = retainedTriangleRegions;
    candidate->retainedAllFamilies = retainedWorldFamilies.empty();
    candidate->retainedAllTriangles = !omissions || omissions->Triangles().empty();
    staticGeometryReplacementMeasurements_ = {};
    if (retainedWorldFamilies.empty() && fullStaticGeometrySource_) {
        candidate->copy = std::make_unique<RtStaticMeshCopy>();
        if (!candidate->copy->Begin(fullStaticGeometrySource_, diagnostic,
                kCollapseInstanceIndex,retainedTriangleRegions,std::move(omissions),true)) return false;
        candidate->phase = PreparedStaticGeometry::Phase::Copy;
        preparedStaticGeometry_ = std::move(candidate);
        staticGeometryReplacementMeasurements_.beginMilliseconds =
            std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        staticGeometryReplacementMeasurements_.packMilliseconds = staticGeometryReplacementMeasurements_.beginMilliseconds;
        diagnostic.clear(); return true;
    }
    if (!PackLoadedStaticAssets(candidate->slot, retainedWorldFamilies, diagnostic,retainedTriangleRegions)) return false;
    if (!BeginPreparedStaticUploads(*candidate, diagnostic)) return false;
    staticGeometryReplacementMeasurements_ = {};
    staticGeometryReplacementMeasurements_.packMilliseconds =
        std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    staticGeometryReplacementMeasurements_.beginMilliseconds = staticGeometryReplacementMeasurements_.packMilliseconds;
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
    // The cached path preserves every vertex offset and value. Reuse only an
    // exactly equal address domain; compact-family candidates still upload.
    // Dynamic Kit writes stay in their existing, disjoint vertex subrange.
    if (packed.SharesVertexPayloadWith(staticMeshSlot_) ||
        SameBytes(packed.Vertices(), staticMeshSlot_.Vertices()))
        candidate.sharedVertexAddress = staticVertexBuffer_.address;
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
    if (!candidate.uploads->Begin(std::span(uploads).subspan(candidate.sharedVertexAddress ? 1u : 0u), diagnostic)) return false;
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
        if(candidate->copy->LastScannedBytes())++evidence.cpuRegionScanAdvanceCount;
        evidence.scannedCpuPayloadBytes+=candidate->copy->LastScannedBytes();
        if (candidate->copy->Ready()) {
            auto copied = candidate->copy->Take();
            candidate->slot = std::move(*copied);
            candidate->copy.reset();
            if (!BeginPreparedStaticUploads(*candidate, diagnostic)) return failBuild();
        }
        const auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        evidence.packMilliseconds += elapsed;
        evidence.maximumCpuCopyAdvanceMilliseconds=std::max(evidence.maximumCpuCopyAdvanceMilliseconds,elapsed);
        return completedStep();
    }
    if (candidate->phase == PreparedStaticGeometry::Phase::Upload) {
        if (!candidate->uploads->Advance(maxCopyBytes, diagnostic)) return failBuild();
        ++evidence.uploadAdvanceCount;
        evidence.uploadedBytes += candidate->uploads->LastAdvanceCopiedBytes();
        if (candidate->uploads->Status() != RtGpuBufferPreparation::State::Ready) return completedStep();
        std::array<Buffer, 6> buffers{};
        if (!candidate->uploads->Take(std::span(buffers).subspan(candidate->sharedVertexAddress ? 1u : 0u), diagnostic)) return failBuild();
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
            t.vertexData.deviceAddress = (candidate->sharedVertexAddress ? candidate->sharedVertexAddress : candidate->vertices.address) +
                VkDeviceSize(primitive.vertexOffset)*sizeof(horde::scene::assets::StaticRtVertex);
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
    // Candidate total includes its shared resident vertex allocation; overlap
    // counts that allocation once, and retirement never frees the borrow.
    if (candidate->sharedVertexAddress) evidence.candidateOwnerAllocationBytes += staticVertexBuffer_.allocationSize;
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
#if HORDE_RT_REGION_PAGE_PROTOTYPE
    if(regionPageSource_ || !regionPageOwner_.Empty()) {
        diagnostic="Combined static cutover cannot replace an independent region-page owner.";return false;
    }
#endif
    const auto eligible = [&] {
        const bool owned = ready_ && !staticGeometryCompletionUnproved_ && expectedGeneration != 0 &&
            expectedGeneration != UINT64_MAX && expectedGeneration == staticGeometryGeneration_ &&
            preparedStaticGeometry_ && preparedStaticGeometry_->ready &&
            !preparedStaticGeometry_->buildWork.submitted &&
            preparedStaticGeometry_->sourceGeneration == expectedGeneration &&
            tlasInstanceDefinitionsValid_ && !tlasPendingDefinitionsValid_ &&
            (!preparedStaticGeometry_->sharedVertexAddress ||
                (preparedStaticGeometry_->sharedVertexAddress == staticVertexBuffer_.address &&
                 !preparedStaticGeometry_->vertices.buffer));
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
        if (!retired->sharedVertexAddress) std::swap(staticVertexBuffer_, retired->vertices);
        std::swap(staticIndexBuffer_, retired->indices);
        std::swap(staticGeometryTransformBuffer_, retired->transforms);
        std::swap(instanceMetadataBuffer_, retired->instances);
        std::swap(primitiveMetadataBuffer_, retired->primitives);
        std::swap(instanceBuffer_, retired->tlasInstances);
        std::swap(collapseBlas_, retired->collapse);
        std::swap(tlas_, retired->tlas);
        std::swap(tlasUpdateScratch_, retired->tlasScratch);
        std::swap(staticGeometryRetainedTriangleRegions_, retired->retainedTriangleRegions);
        std::swap(staticGeometryRetainedAllFamilies_, retired->retainedAllFamilies);
        std::swap(staticGeometryRetainedAllTriangles_, retired->retainedAllTriangles);
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
#if HORDE_RT_FRAME_STAGE_PROBE
    , RtSceneGpuStageProbe* gpuProbe
#endif
    )
{
    if (!retireReferences || commandBuffer == VK_NULL_HANDLE || swapchainImage == VK_NULL_HANDLE ||
        swapchainExtent.width == 0 || swapchainExtent.height == 0) {
        diagnostic = "Static replacement requires an actual first-frame command, output and retirement proof.";
        return false;
    }
#if HORDE_RT_REGION_PAGE_PROTOTYPE
    if(regionPageSource_) {
        const auto action=regionCutover_;const auto token=regionCutoverToken_;const bool wasVisible=regionPageVisible_;
        const auto eligible=[&] {
            return expectedGeneration==staticGeometryGeneration_ && ready_ && regionPageSource_ &&
                action!=RegionCutover::None && regionCutover_==action && regionCutoverToken_==token &&
                !tlasPendingDefinitionsValid_ && std::any_of(regionStreaming_.States().begin(),regionStreaming_.States().end(),
                    [&](const auto& state){return state.token==token && !state.cancelRequested &&
                        state.phase==(action==RegionCutover::Adopt?horde::world::streaming::Phase::Ready:horde::world::streaming::Phase::Retiring);});
        };
        if(!eligible() || !retireReferences(user,diagnostic) || !eligible()) {
            if(diagnostic.empty())diagnostic="Region cutover has stale source/readiness/reference identity.";return false;
        }
        if(action==RegionCutover::Adopt) {
            if(regionPageOwner_.Empty() && !regionPagePreparation_->Take(token,regionPageOwner_,diagnostic))return false;
            if(regionPageOwner_.token!=token)return false;
            regionPageVisible_=true;
        }else {
            if(!regionPageVisible_ || regionPageOwner_.Empty() || regionPageOwner_.token!=token)return false;
            regionPageVisible_=false;
        }
        const bool recorded=RecordTraceAndCopy(commandBuffer,swapchainImage,swapchainImageLayout,swapchainExtent,frame,diagnostic,observation
#if HORDE_RT_STAGED_PRIMARY_TIMING
            ,stagedTiming,timingFrameSlot
#endif
#if HORDE_RT_FRAME_STAGE_PROBE
            ,gpuProbe
#endif
        );
        if(!recorded){
            regionPageVisible_=wasVisible;
            if(action==RegionCutover::Disable)regionStreaming_.MarkRetirementCancelled(token);
            return false;
        }
        if(action==RegionCutover::Adopt) {
            if(!regionStreaming_.MarkResident(token)){diagnostic="Region adoption scheduler identity became stale.";return false;}
            regionPreparationRecovery_.ResetAfterAdoption();
            staticGeometryRetainedAllTriangles_=true;ordinaryStaticReduced_=false;
            ++ordinaryStaticDetailEvidence_.restores;ordinaryStaticDetailEvidence_.savedOwnerBytes=0;
            ordinaryStaticDetailEvidence_.omittedTriangles=0;
        }else {
            // Keep V/I/primitive/BLAS alive through this TLAS update's graphics
            // completion. The next proven frame retires them, never this one.
            regionRetirementAwaitingFrame_=true;staticGeometryRetainedAllTriangles_=false;ordinaryStaticReduced_=true;
        }
        regionCutover_=RegionCutover::None;diagnostic.clear();return true;
    }
#endif
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
#if HORDE_RT_FRAME_STAGE_PROBE
        RtSceneGpuStageProbe* gpuProbe;
#endif
    } first{retireReferences,user,commandBuffer,swapchainImage,&swapchainImageLayout,
        swapchainExtent,&frame,observation
#if HORDE_RT_STAGED_PRIMARY_TIMING
        ,stagedTiming,timingFrameSlot
#endif
#if HORDE_RT_FRAME_STAGE_PROBE
        ,gpuProbe
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
#if HORDE_RT_FRAME_STAGE_PROBE
                ,f.gpuProbe
#endif
            );
        }};
    return CommitStaticGeometryWithApi(expectedGeneration,api,diagnostic);
}

} // namespace horde::vulkan::raytracing
