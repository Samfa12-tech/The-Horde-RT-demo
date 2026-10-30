#include "vulkan/raytracing/RtStaticMeshSlot.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <limits>
#include <map>
#include <unordered_map>
#include <type_traits>
#include <utility>

namespace horde::vulkan::raytracing
{

namespace
{

RtMaterialGpu ConvertMaterial(const horde::scene::assets::StaticMaterial& source,
                              const std::array<std::uint32_t, 4u>& textureLayers)
{
    RtMaterialGpu result{};
    result.baseColorFactor = source.baseColorFactor;
    result.emissiveFactorStrength = {{
        source.emissiveFactor[0], source.emissiveFactor[1],
        source.emissiveFactor[2], source.emissiveStrength}};
    result.metallicRoughnessOcclusionTransmission = {{
        source.metallicFactor, source.roughnessFactor,
        source.occlusionStrength, source.transmissionFactor}};
    result.iorThicknessAttenuationDistance = {{
        source.ior, source.thicknessFactor, source.attenuationDistance,
        source.numericalSpawnMinimumWidth}};
    result.attenuationColor = {{
        source.attenuationColor[0], source.attenuationColor[1],
        source.attenuationColor[2], source.numericalSpawnGeometryError}};
    result.textureLayers = textureLayers;
    result.normalScaleUvScaleBlend = {{
        source.normalScale, source.textureScale[0], source.textureScale[1], 1.0f}};
    result.materialFlags[0] = source.flags;
    if (source.baseColorTexture >= 0)
        result.materialFlags[0] |= static_cast<std::uint32_t>(RtMaterialFlag::BaseColorTexture);
    if (source.normalTexture >= 0)
        result.materialFlags[0] |= static_cast<std::uint32_t>(RtMaterialFlag::NormalTexture);
    if (source.ormTexture >= 0)
        result.materialFlags[0] |= static_cast<std::uint32_t>(RtMaterialFlag::OrmTexture);
    if (source.emissiveTexture >= 0)
        result.materialFlags[0] |= static_cast<std::uint32_t>(RtMaterialFlag::EmissiveTexture);
    return result;
}

struct OwnedTriangleSpan
{
    std::uint32_t primitiveBegin = 0u, primitiveEnd = 0u, indexBegin = 0u, indexEnd = 0u;
};

bool ValidateSelectedOwner(const RtStaticMeshSlot& source, std::uint32_t instance,
    OwnedTriangleSpan& span, std::string& diagnostic)
{
    if(instance>=source.InstanceMetadata().size()) {
        diagnostic="Cached selection needs an existing immutable instance.";return false;
    }
    const auto& owner=source.InstanceMetadata()[instance];
    const auto& primitives=source.PrimitiveMetadata();
    if(!owner.primitiveCount || owner.geometryRole!=static_cast<std::uint32_t>(RtGeometryRole::Static) ||
       (owner.flags&static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr))==0u ||
       owner.primitiveBase>=primitives.size() || owner.primitiveCount>primitives.size()-owner.primitiveBase) {
        diagnostic="Cached selection requires a complete static primitive owner.";return false;
    }
    span.primitiveBegin=owner.primitiveBase;span.primitiveEnd=owner.primitiveBase+owner.primitiveCount;
    span.indexBegin=primitives[span.primitiveBegin].indexOffset;span.indexEnd=span.indexBegin;
    for(std::size_t i=span.primitiveBegin;i<span.primitiveEnd;++i) {
        const auto& p=primitives[i];
        if(p.indexOffset!=span.indexEnd || !p.indexCount || p.indexCount%3u || p.indexOffset%3u ||
           p.indexOffset>source.Indices().size() || p.indexCount>source.Indices().size()-p.indexOffset ||
           p.indexCount>UINT32_MAX-span.indexEnd) {
            diagnostic="Cached selection needs contiguous owned triangle spans.";return false;
        }
        span.indexEnd+=p.indexCount;
    }
    // Selection cannot change geometry another instance/primitive still owns.
    for(std::size_t i=0;i<source.InstanceMetadata().size();++i) if(i!=instance) {
        const auto& other=source.InstanceMetadata()[i];
        if(other.primitiveCount && other.primitiveBase<span.primitiveEnd &&
           span.primitiveBegin<static_cast<std::uint64_t>(other.primitiveBase)+other.primitiveCount) {
            diagnostic="Cached selection cannot omit another instance's shared geometry.";return false;
        }
    }
    for(std::size_t i=0;i<primitives.size();++i) {
        const auto& p=primitives[i];
        if(p.indexOffset%3u || p.indexCount%3u || !p.indexCount ||
           p.indexOffset>source.Indices().size() || p.indexCount>source.Indices().size()-p.indexOffset ||
           p.indexCount>UINT32_MAX-p.indexOffset) {
            diagnostic="Cached selection rejected an invalid admitted index boundary.";return false;
        }
        if((i<span.primitiveBegin || i>=span.primitiveEnd) &&
           p.indexOffset<span.indexEnd && span.indexBegin<p.indexOffset+p.indexCount) {
            diagnostic="Cached selection cannot omit another primitive's overlapping indices.";return false;
        }
    }
    return true;
}

} // namespace

std::shared_ptr<const RtStaticTriangleOmissions> RtStaticTriangleOmissions::Create(
    std::shared_ptr<const RtStaticMeshSlot> source, std::uint32_t selectedInstance,
    std::span<const std::uint32_t> triangles, std::string& diagnostic)
{
    OwnedTriangleSpan span;
    if(!source || !source->admitted_ || source->TriangleRegions().size()!=source->Indices().size()/3u) {
        diagnostic="Triangle omissions require an admitted immutable source.";return {};
    }
    if(!ValidateSelectedOwner(*source,selectedInstance,span,diagnostic))return {};
    std::size_t primitive=span.primitiveBegin, removed=0u;
    for(std::size_t i=0;i<triangles.size();++i) {
        const auto triangle=triangles[i];
        if((i && triangles[i-1]>=triangle) || triangle<span.indexBegin/3u || triangle>=span.indexEnd/3u) {
            diagnostic="Triangle omissions must be sorted, unique and inside their exclusive owner.";return {};
        }
        const auto region=static_cast<std::uint8_t>(source->TriangleRegions()[triangle]);
        if(region!=1u && region!=2u) {
            diagnostic="Triangle omissions cannot remove Shared or corrupt provenance.";return {};
        }
        while(triangle>=(source->PrimitiveMetadata()[primitive].indexOffset+
                         source->PrimitiveMetadata()[primitive].indexCount)/3u) {
            ++primitive;removed=0u;
        }
        if(++removed>=source->PrimitiveMetadata()[primitive].indexCount/3u) {
            diagnostic="Triangle omissions would empty a required primitive row.";return {};
        }
    }
    try {
        auto plan=std::shared_ptr<RtStaticTriangleOmissions>(new RtStaticTriangleOmissions);
        plan->source_=std::move(source);plan->selectedInstance_=selectedInstance;
        plan->triangles_.assign(triangles.begin(),triangles.end());
        diagnostic.clear();return plan;
    } catch(const std::exception&) {
        diagnostic="Failed to allocate immutable triangle omissions.";return {};
    }
}

bool RtStaticMeshSlot::TransmissionOwnersWithin(std::span<const std::uint32_t> allowed) const noexcept
{
    if(!admitted_)return false;
    for(std::size_t instance=0;instance<instanceMetadata_.size();++instance) {
        const auto& owner=instanceMetadata_[instance];
        if(owner.primitiveBase>primitiveMetadata_.size() || owner.primitiveCount>primitiveMetadata_.size()-owner.primitiveBase)return false;
        for(std::size_t p=owner.primitiveBase;p<owner.primitiveBase+owner.primitiveCount;++p) {
            const auto row=primitiveMetadata_[p].materialIndex;
            if(row>=materials_.size())return false;
            if((materials_[row].materialFlags[0]&static_cast<std::uint32_t>(RtMaterialFlag::Transmission)) &&
               std::find(allowed.begin(),allowed.end(),static_cast<std::uint32_t>(instance))==allowed.end())return false;
        }
    }
    return true;
}

std::uint64_t RtStaticMeshSlot::PayloadBytes() const noexcept
{
    // Logical payload. Shared vertex storage is counted once by its owner in
    // allocation inventories, but remains part of this slot's address domain.
    return Vertices().size()*sizeof(horde::scene::assets::StaticRtVertex) +
        Vertices(RtGeometryRole::PlayerWorldBody).size()*sizeof(horde::scene::assets::StaticRtVertex) +
        Vertices(RtGeometryRole::PlayerViewmodel).size()*sizeof(horde::scene::assets::StaticRtVertex) + indices_.size()*sizeof(indices_.front()) +
        materials_.size()*sizeof(materials_.front()) + primitiveMetadata_.size()*sizeof(primitiveMetadata_.front()) +
        primitiveVertexCounts_.size()*sizeof(primitiveVertexCounts_.front()) +
        geometryTransforms_.size()*sizeof(geometryTransforms_.front()) + triangleRegions_.size() + sizeof(instanceMetadata_) +
        sizeof(textureArrayCounts_) + sizeof(measurements_);
}
bool RtStaticMeshCopy::Begin(std::shared_ptr<const RtStaticMeshSlot> source, std::string& diagnostic,
    std::uint32_t selectedInstance, std::uint8_t retainedRegions,
    std::shared_ptr<const RtStaticTriangleOmissions> omissions, bool shareImmutableVertices)
{
    if(output_ || !source || !source->admitted_ || source->PrimitiveMetadata().empty() || source->Materials().empty() ||
        source->Indices().empty() || retainedRegions>horde::scene::assets::kAllStaticTriangleRegions ||
        source->triangleRegions_.size()!=source->indices_.size()/3u) {
        diagnostic="Immutable copy needs an admitted source and no existing candidate."; return false;
    }
    if(omissions && (omissions->source_.get()!=source.get() || omissions->selectedInstance_!=selectedInstance ||
        retainedRegions!=horde::scene::assets::kAllStaticTriangleRegions)) {
        diagnostic="Triangle omissions require their exact source/owner and cannot mix with region filtering.";return false;
    }
    try {
        std::vector<Boundary> boundaries;
        std::uint32_t selectedBegin=0u,selectedEnd=0u;
        if(omissions || retainedRegions!=horde::scene::assets::kAllStaticTriangleRegions) {
            OwnedTriangleSpan span;
            if(!ValidateSelectedOwner(*source,selectedInstance,span,diagnostic))return false;
            selectedBegin=span.indexBegin;selectedEnd=span.indexEnd;
            for(std::size_t i=0;i<source->primitiveMetadata_.size();++i) {
                const auto& p=source->primitiveMetadata_[i];
                boundaries.push_back({p.indexOffset,static_cast<std::uint32_t>(i),false});
                boundaries.push_back({p.indexOffset+p.indexCount,static_cast<std::uint32_t>(i),true});
            }
            std::sort(boundaries.begin(),boundaries.end(),[](const auto& a,const auto& b){return a.index<b.index;});
        }
        auto output=std::make_unique<RtStaticMeshSlot>();
        output->instanceMetadata_=source->instanceMetadata_;
        output->textureArrayCounts_=source->textureArrayCounts_;
        output->measurements_=source->measurements_;
        Cancel(); source_=std::move(source); output_=std::move(output);
        if (shareImmutableVertices) {
            // Flatten aliases so copies never build an ownership chain. The
            // immutable root outlives cancellation, source release and cutover.
            output_->vertexSource_ = source_->vertexSource_ ? source_->vertexSource_ : source_;
            row_ = 3u;
        }
        boundaries_=std::move(boundaries);selectedBegin_=selectedBegin;selectedEnd_=selectedEnd;
        retainedRegions_=retainedRegions;
        omissions_=std::move(omissions);
    } catch(const std::exception&) {
        diagnostic="Failed to allocate an immutable CPU copy owner."; return false;
    }
    diagnostic.clear(); return true;
}
bool RtStaticMeshCopy::Advance(const std::size_t maxBytes, std::string& diagnostic)
{
    lastBytes_=lastScannedBytes_=0u;
    if(!output_ || Ready() || maxBytes==0u) {
        diagnostic="Immutable copy advance needs an active owner and positive byte budget."; return false;
    }
    const auto copy=[&](const auto& from,auto& to) {
        using T=typename std::remove_cvref_t<decltype(from)>::value_type;
        static_assert(std::is_trivially_copyable_v<T>);
        if(!from.empty() && maxBytes<sizeof(T)) {
            diagnostic="Immutable copy budget cannot hold one complete record."; return false;
        }
        if(offset_==0u) to.reserve(from.size());
        const auto count=std::min(maxBytes/sizeof(T),from.size()-offset_);
        to.insert(to.end(),from.begin()+offset_,from.begin()+offset_+count);
        offset_+=count; lastBytes_=count*sizeof(T); copiedBytes_+=lastBytes_;
        if(offset_==from.size()) { ++row_;offset_=0u; }
        diagnostic.clear(); return true;
    };
    try {
        switch(row_) {
        case 0: return copy(source_->Vertices(),output_->vertices_);
        case 1: return copy(source_->Vertices(RtGeometryRole::PlayerWorldBody),output_->worldBodyVertices_);
        case 2: return copy(source_->Vertices(RtGeometryRole::PlayerViewmodel),output_->viewmodelVertices_);
        case 3: return copy(source_->indices_,output_->indices_);
        case 4: return copy(source_->materials_,output_->materials_);
        case 5: return copy(source_->primitiveMetadata_,output_->primitiveMetadata_);
        case 6: return copy(source_->primitiveVertexCounts_,output_->primitiveVertexCounts_);
        case 7: return copy(source_->geometryTransforms_,output_->geometryTransforms_);
        case 8: return copy(source_->triangleRegions_,output_->triangleRegions_);
        case 9: return FilterIndices(maxBytes,diagnostic);
        default: diagnostic="Invalid immutable copy cursor."; return false;
        }
    } catch(const std::exception&) {
        Cancel(); diagnostic="Immutable CPU candidate allocation/copy failed."; return false;
    }
}
bool RtStaticMeshCopy::FilterIndices(std::size_t maxBytes,std::string& diagnostic)
{
    constexpr auto triangleBytes=3u*sizeof(std::uint32_t)+sizeof(horde::scene::assets::StaticTriangleRegion);
    // Charge the extra plan lookup conservatively even when the next omitted
    // ID is reused. The same existing per-turn budget now scans fewer triangles.
    const auto scanBytes=triangleBytes+(omissions_ ? sizeof(std::uint32_t) : 0u);
    if(maxBytes<scanBytes) {diagnostic="Cached selection budget cannot scan one triangle.";return false;}
    const auto boundaries=[&] {
        while(boundary_<boundaries_.size() && boundaries_[boundary_].index==readIndex_) {
            const auto event=boundaries_[boundary_++];auto& p=output_->primitiveMetadata_[event.primitive];
            if(event.end)p.indexCount=static_cast<std::uint32_t>(writeIndex_)-p.indexOffset;
            else p.indexOffset=static_cast<std::uint32_t>(writeIndex_);
        }
    };
    const auto count=std::min(maxBytes/scanBytes,(source_->indices_.size()-readIndex_)/3u);
    const auto finish=readIndex_+count*3u;
    while(readIndex_<finish) {
        boundaries();
        const auto region=source_->triangleRegions_[readIndex_/3u];
        const auto flag=static_cast<std::uint8_t>(region);
        if(flag>2u) {Cancel();diagnostic="Cached selection rejected corrupt triangle provenance.";return false;}
        const bool targeted=readIndex_>=selectedBegin_ && readIndex_<selectedEnd_;
        const bool omit=omissions_ && omission_<omissions_->triangles_.size() &&
            omissions_->triangles_[omission_]==readIndex_/3u;
        if(omit)++omission_;
        if(!omit && (!targeted || flag==0u || (retainedRegions_&flag)!=0u)) {
            for(unsigned i=0;i<3u;++i)output_->indices_[writeIndex_+i]=source_->indices_[readIndex_+i];
            output_->triangleRegions_[writeIndex_/3u]=region;
            writeIndex_+=3u;lastBytes_+=triangleBytes;copiedBytes_+=triangleBytes;
        }
        readIndex_+=3u;lastScannedBytes_+=scanBytes;scannedBytes_+=scanBytes;
    }
    boundaries();
    if(readIndex_==source_->indices_.size()) {
        if(boundary_!=boundaries_.size() || (omissions_ && omission_!=omissions_->triangles_.size()) ||
            std::any_of(output_->primitiveMetadata_.begin(),
            output_->primitiveMetadata_.end(),[](const auto& p){return !p.indexCount;})) {
            Cancel();diagnostic="Cached selection would empty a required primitive row.";return false;
        }
        output_->indices_.resize(writeIndex_);output_->triangleRegions_.resize(writeIndex_/3u);
        output_->measurements_.indexBytes=writeIndex_*sizeof(std::uint32_t);
        filtered_=true;
    }
    diagnostic.clear();return true;
}
std::unique_ptr<RtStaticMeshSlot> RtStaticMeshCopy::Take() noexcept
{
    if(!Ready()) return {};
    output_->admitted_=true;
    source_.reset(); omissions_.reset();omission_=0u;row_=offset_=lastBytes_=lastScannedBytes_=0u;
    boundaries_.clear();boundary_=readIndex_=writeIndex_=0u;filtered_=false;
    retainedRegions_=horde::scene::assets::kAllStaticTriangleRegions;
    return std::move(output_);
}
void RtStaticMeshCopy::Cancel() noexcept
{ output_.reset(); source_.reset(); omissions_.reset();omission_=0u;row_=offset_=lastBytes_=lastScannedBytes_=0u; copiedBytes_=scannedBytes_=0u;
  boundaries_.clear();boundary_=readIndex_=writeIndex_=0u;selectedBegin_=selectedEnd_=0u;filtered_=false;
  retainedRegions_=horde::scene::assets::kAllStaticTriangleRegions; }

bool RtStaticMeshSlot::InitializeSelectedGeometry(
    std::span<const StaticRtAssetRegistration> registrations, std::string& diagnostic)
{
    using Asset = horde::scene::assets::StaticMeshAsset;
    struct Selection {
        const Asset* source = nullptr;
        Asset payload;
        std::vector<bool> retained;
        std::vector<std::uint32_t> remap;
        bool complete = false;
    };
    std::vector<Selection> selections;
    selections.reserve(kRtStaticAssetCapacity);
    const auto find = [&](const Asset* asset) {
        return std::find_if(selections.begin(), selections.end(),
            [asset](const Selection& item) { return item.source == asset; });
    };
    const auto reject = [&](const char* reason) {
        diagnostic = std::string("RtStaticMeshSlot selected geometry: ") + reason;
        return false;
    };
    if (registrations.empty()) return reject("candidate has no registrations.");
    for (const auto& registration : registrations) {
        if (!registration.asset) return reject("registration has no asset.");
        const auto& asset = *registration.asset;
        if (asset.vertices.size() > UINT32_MAX || asset.indices.size() > UINT32_MAX ||
            asset.primitives.empty() || asset.primitives.size() > UINT32_MAX)
            return reject("empty primitives or geometry exceeds addressable offsets.");
        auto selection = find(&asset);
        if (selection == selections.end()) {
            if (selections.size() == kRtStaticAssetCapacity)
                return reject("source assets exceed the unchanged capacity.");
            selections.push_back({&asset, {}, std::vector<bool>(asset.primitives.size()), {}});
            selection = std::prev(selections.end());
        }
        const bool ranged = registration.primitiveOffset != 0 || registration.primitiveCount != 0;
        const bool listed = !registration.primitiveSelection.empty();
        if (listed && ranged) return reject("explicit selection conflicts with a primitive range.");
        if ((ranged || listed) && (registration.geometryRole != RtGeometryRole::Static ||
            !(registration.flags & static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr))))
            return reject("primitive selections require immutable StaticPbr geometry.");
        if (listed) {
            if (registration.primitiveSelection.size() > asset.primitives.size())
                return reject("explicit selection exceeds its source primitive count.");
            std::uint32_t previous = 0;
            bool first = true;
            for (const auto index : registration.primitiveSelection) {
                if (index >= asset.primitives.size() || (!first && index <= previous))
                    return reject("explicit selection must be strictly increasing and inside its source.");
                selection->retained[index] = true;
                first = false; previous = index;
            }
            continue;
        }
        const std::size_t start = ranged ? registration.primitiveOffset : 0;
        const std::size_t count = ranged ? registration.primitiveCount : asset.primitives.size();
        if (!count || start >= asset.primitives.size() || count > asset.primitives.size() - start)
            return reject("primitive range is empty or outside its source.");
        std::fill(selection->retained.begin() + start, selection->retained.begin() + start + count, true);
    }
    // Source-local ordering is deterministic even when owner registrations are
    // reordered or overlap. Retain whole indexed vertex spans; no decimation,
    // altered attributes, index widening, or shader/material changes occur.
    for (auto& selection : selections) {
        const auto& source = *selection.source;
        if(!source.HasValidTriangleProvenance())return reject("invalid source triangle provenance.");
        selection.remap.assign(source.primitives.size(), UINT32_MAX);
        selection.complete = std::all_of(selection.retained.begin(), selection.retained.end(),
            [](bool retained) { return retained; });
        if (!selection.complete) {
            // Do not duplicate the retired zone's geometry in a temporary CPU
            // asset just to clear it again. Shared source metadata stays exact.
            selection.payload.materials = source.materials;
            selection.payload.nodeTransforms = source.nodeTransforms;
            selection.payload.sockets = source.sockets;
            selection.payload.bounds = source.bounds;
        }
        for (std::size_t i = 0; i < source.primitives.size(); ++i) {
            if (!selection.retained[i]) continue;
            const auto& primitive = source.primitives[i];
            const std::size_t end = i + 1 < source.primitives.size()
                ? source.primitives[i + 1].vertexOffset : source.vertices.size();
            if (primitive.vertexOffset >= source.vertices.size() || end > source.vertices.size() ||
                end <= primitive.vertexOffset || (i > 0 &&
                    primitive.vertexOffset <= source.primitives[i - 1].vertexOffset))
                return reject("primitive has an invalid source-local vertex span.");
            if (!primitive.indexCount || primitive.indexCount % 3 != 0 || primitive.indexOffset%3u ||
                primitive.indexOffset > source.indices.size() ||
                primitive.indexCount > source.indices.size() - primitive.indexOffset)
                return reject("primitive has an invalid triangle index range.");
            if (primitive.materialIndex >= source.materials.size() ||
                primitive.nodeTransformIndex >= source.nodeTransforms.size())
                return reject("primitive references an absent material or node transform.");
            const auto indexBegin = source.indices.begin() + primitive.indexOffset;
            const auto indexEnd = indexBegin + primitive.indexCount;
            if (std::any_of(indexBegin, indexEnd, [&](std::uint32_t index) {
                return index >= end - primitive.vertexOffset;
            })) return reject("triangle index leaves its source-local vertex span.");
            if (selection.complete) { selection.remap[i] = static_cast<std::uint32_t>(i); continue; }
            auto compacted = primitive;
            compacted.vertexOffset = static_cast<std::uint32_t>(selection.payload.vertices.size());
            compacted.indexOffset = static_cast<std::uint32_t>(selection.payload.indices.size());
            selection.remap[i] = static_cast<std::uint32_t>(selection.payload.primitives.size());
            selection.payload.vertices.insert(selection.payload.vertices.end(),
                source.vertices.begin() + primitive.vertexOffset, source.vertices.begin() + end);
            selection.payload.indices.insert(selection.payload.indices.end(), indexBegin, indexEnd);
            for(std::size_t t=primitive.indexOffset/3u;t<(primitive.indexOffset+primitive.indexCount)/3u;++t)
                selection.payload.triangleRegions.push_back(t<source.triangleRegions.size()?source.triangleRegions[t]:
                    horde::scene::assets::StaticTriangleRegion::Shared);
            selection.payload.primitives.push_back(compacted);
        }
    }
    std::vector<StaticRtAssetRegistration> packed(registrations.begin(), registrations.end());
    for (auto& registration : packed) {
        const auto selection = find(registration.asset);
        const bool listed = !registration.primitiveSelection.empty();
        const auto oldOffset = listed ? registration.primitiveSelection.front() : registration.primitiveOffset;
        if (listed) {
            // The existing hit ABI needs one contiguous packed span per owner.
            // Source indices may have holes, but another owner's interleaved
            // selection must never silently put unrelated geometry in this BLAS.
            for (std::size_t i = 0; i < registration.primitiveSelection.size(); ++i)
                if (selection->remap[registration.primitiveSelection[i]] !=
                    selection->remap[oldOffset] + i)
                    return reject("overlapping selections leave an owner noncontiguous in the packed hit ABI.");
            registration.primitiveCount = static_cast<std::uint32_t>(registration.primitiveSelection.size());
            registration.primitiveSelection = {};
        }
        registration.asset = selection->complete ? selection->source : &selection->payload;
        registration.primitiveOffset = selection->remap[oldOffset];
        if (registration.textureSource) {
            const auto provider = find(registration.textureSource);
            if (provider == selections.end()) return reject("texture provider is not registered.");
            registration.textureSource = provider->complete ? provider->source : &provider->payload;
        }
    }
    RtStaticMeshSlot candidate;
    if (!candidate.Initialize(packed, diagnostic)) return false;
    *this = std::move(candidate);
    return true;
}

bool RtStaticMeshSlot::Initialize(std::span<const StaticRtAssetRegistration> registrations,
                                  std::string& diagnostic)
{
    if (std::any_of(registrations.begin(), registrations.end(), [](const auto& registration) {
        return !registration.primitiveSelection.empty();
    })) {
        diagnostic = "RtStaticMeshSlot explicit primitive selection requires selected-geometry preparation.";
        return false;
    }
    admitted_ = false;
    instanceMetadata_ = {};
    primitiveMetadata_.clear();
    primitiveVertexCounts_.clear();
    materials_.clear();
    vertices_.clear();
    vertexSource_.reset();
    worldBodyVertices_.clear();
    viewmodelVertices_.clear();
    indices_.clear();
    triangleRegions_.clear();
    geometryTransforms_.clear();
    textureArrayCounts_ = {};
    measurements_ = {};

    std::array<bool, kRtInstanceMetadataCapacity> occupied{};
    std::vector<const horde::scene::assets::StaticMeshAsset*> uniqueAssets;
    std::unordered_map<const horde::scene::assets::StaticMeshAsset*, std::size_t>
        firstRegistrationIndex;
    std::unordered_map<const horde::scene::assets::StaticMeshAsset*,
                       const horde::scene::assets::StaticMeshAsset*> textureSources;
    std::unordered_map<const horde::scene::assets::StaticMeshAsset*, RtGeometryRole>
        geometryRoles;
    const horde::scene::assets::StaticMeshAsset* worldBodyAsset = nullptr;
    const horde::scene::assets::StaticMeshAsset* viewmodelAsset = nullptr;
    for (std::size_t registrationIndex = 0u;
         registrationIndex < registrations.size(); ++registrationIndex)
    {
        const StaticRtAssetRegistration& registration = registrations[registrationIndex];
        if (registration.instanceCustomIndex >= kRtInstanceMetadataCapacity)
        {
            diagnostic = "RtStaticMeshSlot capacity overflow: instanceCustomIndex exceeds RtInstanceMetadata[" +
                         std::to_string(kRtInstanceMetadataCapacity) + "].";
            return false;
        }
        if (occupied[registration.instanceCustomIndex])
        {
            diagnostic = "RtStaticMeshSlot registration duplicates instanceCustomIndex " +
                         std::to_string(registration.instanceCustomIndex) + ".";
            return false;
        }
        occupied[registration.instanceCustomIndex] = true;
        if (registration.asset == nullptr)
        {
            diagnostic = "RtStaticMeshSlot registration has no static asset.";
            return false;
        }
        const bool hasPrimitiveRange = registration.primitiveOffset != 0u ||
                                       registration.primitiveCount != 0u;
        if (hasPrimitiveRange)
        {
            if (registration.geometryRole != RtGeometryRole::Static ||
                (registration.flags & static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr)) == 0u)
            {
                diagnostic = "RtStaticMeshSlot primitive range requires immutable StaticPbr geometry.";
                return false;
            }
            const auto available = registration.asset->primitives.size();
            // Subtract only after checking the start. Do not let a wrapped
            // offset+count turn an invalid registration into a valid owner.
            if (registration.primitiveCount == 0u ||
                registration.primitiveOffset >= available ||
                registration.primitiveCount > available - registration.primitiveOffset)
            {
                diagnostic = "RtStaticMeshSlot primitive range is empty or outside its asset.";
                return false;
            }
        }
        switch (registration.geometryRole)
        {
        case RtGeometryRole::Static:
            break;
        case RtGeometryRole::PlayerWorldBody:
            if ((registration.flags & static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr)) == 0u)
            {
                diagnostic = "RtStaticMeshSlot player world-body geometry requires the StaticPbr flag.";
                return false;
            }
            if (worldBodyAsset != nullptr && worldBodyAsset != registration.asset)
            {
                diagnostic = "RtStaticMeshSlot permits only one unique PlayerWorldBody asset.";
                return false;
            }
            worldBodyAsset = registration.asset;
            break;
        case RtGeometryRole::PlayerViewmodel:
            if ((registration.flags & static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr)) == 0u)
            {
                diagnostic = "RtStaticMeshSlot player viewmodel geometry requires the StaticPbr flag.";
                return false;
            }
            if (viewmodelAsset != nullptr && viewmodelAsset != registration.asset)
            {
                diagnostic = "RtStaticMeshSlot permits only one unique PlayerViewmodel asset.";
                return false;
            }
            viewmodelAsset = registration.asset;
            break;
        default:
            diagnostic = "RtStaticMeshSlot registration has an invalid geometry role.";
            return false;
        }
        const auto [roleAsset, insertedRole] = geometryRoles.emplace(
            registration.asset, registration.geometryRole);
        if (!insertedRole && roleAsset->second != registration.geometryRole)
        {
            diagnostic = "RtStaticMeshSlot repeated registration of one asset must keep a consistent geometry role.";
            return false;
        }
        firstRegistrationIndex.emplace(registration.asset, registrationIndex);
        const auto [sourceAsset, insertedSource] = textureSources.emplace(
            registration.asset, registration.textureSource);
        if (!insertedSource && sourceAsset->second != registration.textureSource)
        {
            diagnostic = "RtStaticMeshSlot repeated registration of one asset must keep a consistent texture source.";
            return false;
        }
        if (registration.textureSource != nullptr)
        {
            const auto sourceFirst = firstRegistrationIndex.find(registration.textureSource);
            if (sourceFirst == firstRegistrationIndex.end())
            {
                const bool appearsLater = std::any_of(
                    registrations.begin() + registrationIndex + 1u,
                    registrations.end(),
                    [&registration](const StaticRtAssetRegistration& candidate) {
                        return candidate.asset == registration.textureSource;
                    });
                diagnostic = appearsLater
                    ? "RtStaticMeshSlot texture source must be registered earlier than its consumer."
                    : "RtStaticMeshSlot texture source is not registered by this slot.";
                return false;
            }
            if (sourceFirst->second >= registrationIndex)
            {
                diagnostic = "RtStaticMeshSlot texture source must be registered earlier than its consumer.";
                return false;
            }
            const auto sourceAlias = textureSources.find(registration.textureSource);
            if (sourceAlias != textureSources.end() && sourceAlias->second != nullptr)
            {
                diagnostic = "RtStaticMeshSlot texture source alias chains are not allowed; provider must self-own its groups.";
                return false;
            }
        }
        if (std::find(uniqueAssets.begin(), uniqueAssets.end(), registration.asset) == uniqueAssets.end())
            uniqueAssets.push_back(registration.asset);
    }
    if (uniqueAssets.size() > kRtStaticAssetCapacity)
    {
        diagnostic = "RtStaticMeshSlot capacity overflow: static assets exceed " +
                     std::to_string(kRtStaticAssetCapacity) + ".";
        return false;
    }

    struct AssetRoute
    {
        RtGeometryRole geometryRole;
        std::uint32_t assetIndex;
        std::uint32_t primitiveBase;
        std::uint32_t primitiveCount;
        std::map<std::int32_t, std::array<bool, 4u>> groupPresence;
        std::map<std::int32_t, std::array<std::uint32_t, 4u>> groupLayers;
    };
    std::unordered_map<const horde::scene::assets::StaticMeshAsset*, AssetRoute> routes;
    std::array<std::uint32_t, 4u> nextTextureLayers{};
    struct DeferredGroup {
        std::array<bool,4u> present{};
        std::vector<std::size_t> materialIndices;
    };
    std::map<std::pair<std::size_t,std::int32_t>,DeferredGroup> deferredGroups;
    for (std::size_t assetIndex = 0u; assetIndex < uniqueAssets.size(); ++assetIndex)
    {
        const auto& asset = *uniqueAssets[assetIndex];
        if(!asset.HasValidTriangleProvenance()) {
            diagnostic="RtStaticMeshSlot rejected invalid triangle provenance.";return false;
        }
        const RtGeometryRole geometryRole = geometryRoles.at(uniqueAssets[assetIndex]);
        std::vector<horde::scene::assets::StaticRtVertex>* roleVertices = &vertices_;
        switch (geometryRole)
        {
        case RtGeometryRole::Static: roleVertices = &vertices_; break;
        case RtGeometryRole::PlayerWorldBody: roleVertices = &worldBodyVertices_; break;
        case RtGeometryRole::PlayerViewmodel: roleVertices = &viewmodelVertices_; break;
        default:
            diagnostic = "RtStaticMeshSlot registration has an invalid geometry role.";
            return false;
        }
        if (primitiveMetadata_.size() + asset.primitives.size() > kRtPrimitiveMetadataCapacity)
        {
            diagnostic = "RtStaticMeshSlot capacity overflow: primitives exceed " +
                         std::to_string(kRtPrimitiveMetadataCapacity) + ".";
            return false;
        }
        if (materials_.size() + asset.materials.size() > kRtMaterialCapacity)
        {
            diagnostic = "RtStaticMeshSlot capacity overflow: materials exceed " +
                         std::to_string(kRtMaterialCapacity) + ".";
            return false;
        }
        if (roleVertices->size() + asset.vertices.size() > std::numeric_limits<std::uint32_t>::max() ||
            indices_.size() + asset.indices.size() > std::numeric_limits<std::uint32_t>::max())
        {
            diagnostic = "RtStaticMeshSlot geometry exceeds 32-bit addressable offsets.";
            return false;
        }
        const std::uint32_t vertexBase = static_cast<std::uint32_t>(roleVertices->size());
        const std::uint32_t indexBase = static_cast<std::uint32_t>(indices_.size());
        const std::uint32_t materialBase = static_cast<std::uint32_t>(materials_.size());
        const std::uint32_t primitiveBase = static_cast<std::uint32_t>(primitiveMetadata_.size());
        roleVertices->insert(roleVertices->end(), asset.vertices.begin(), asset.vertices.end());
        indices_.insert(indices_.end(), asset.indices.begin(), asset.indices.end());
        triangleRegions_.insert(triangleRegions_.end(),asset.triangleRegions.begin(),asset.triangleRegions.end());
        triangleRegions_.resize(indices_.size()/3u,horde::scene::assets::StaticTriangleRegion::Shared);
        std::array<std::unordered_map<std::int32_t, std::uint32_t>, 4u> textureRoutes;
        std::map<std::int32_t, std::array<bool, 4u>> groupPresence;
        std::map<std::int32_t, std::array<std::uint32_t, 4u>> groupLayers;
        const auto textureSource = textureSources.at(uniqueAssets[assetIndex]);
        if (textureSource != nullptr)
        {
            const auto sourceRoute = routes.find(textureSource);
            if (sourceRoute == routes.end())
            {
                diagnostic = "RtStaticMeshSlot texture source route was not built before its consumer.";
                return false;
            }
            groupPresence = sourceRoute->second.groupPresence;
            groupLayers = sourceRoute->second.groupLayers;
        }
        else
        {
            std::map<std::int32_t,bool> groupDeferred;
            // Explicit asset-local groups are allocated canonically before the
            // ordinary per-texture routes. Visibility-only material duplication
            // cannot exchange body/gauntlet atlas layers by changing material order.
            for (const auto& material : asset.materials)
            {
                if(material.deferTextureAllocation && material.textureGroup<0) {
                    diagnostic="RtStaticMeshSlot deferred texture material requires an explicit group.";
                    return false;
                }
                if (material.textureGroup < 0) continue;
                const auto [policy,policyInserted]=groupDeferred.try_emplace(
                    material.textureGroup,material.deferTextureAllocation);
                if(!policyInserted && policy->second!=material.deferTextureAllocation) {
                    diagnostic="RtStaticMeshSlot texture group has conflicting allocation policy.";
                    return false;
                }
                const std::array<bool, 4u> present{{material.baseColorTexture >= 0,
                    material.normalTexture >= 0, material.ormTexture >= 0, material.emissiveTexture >= 0}};
                const auto [entry, inserted] = groupPresence.try_emplace(material.textureGroup, present);
                if (!inserted && entry->second != present)
                {
                    diagnostic = "RtStaticMeshSlot texture group has conflicting texture presence.";
                    return false;
                }
            }
            constexpr std::array<const char*, 4u> categoryNames{{"baseColor", "normal", "ORM", "emissive"}};
            for (const auto& [group, present] : groupPresence)
            {
                auto& layers = groupLayers[group];
                if(groupDeferred.at(group)) {
                    deferredGroups[{assetIndex,group}].present=present;
                    continue;
                }
                for (std::size_t category = 0; category < layers.size(); ++category)
                {
                    if (!present[category]) continue;
                    if (nextTextureLayers[category] >= kRtTextureLayerCapacity)
                    {
                        diagnostic = std::string("RtStaticMeshSlot capacity overflow: ") +
                            categoryNames[category] + " texture layers exceed 18.";
                        return false;
                    }
                    layers[category] = nextTextureLayers[category]++;
                }
            }
        }
        const auto routeTexture = [&textureRoutes, &nextTextureLayers, &diagnostic](
            std::size_t category,
            std::int32_t sourceTexture,
            const char* categoryName,
            std::uint32_t& layer) {
            if (sourceTexture < 0)
            {
                layer = 0u;
                return true;
            }
            auto [entry, inserted] = textureRoutes[category].try_emplace(
                sourceTexture, nextTextureLayers[category]);
            if (inserted)
            {
                if (nextTextureLayers[category] >= kRtTextureLayerCapacity)
                {
                    diagnostic = std::string("RtStaticMeshSlot capacity overflow: ") +
                                 categoryName + " texture layers exceed 18.";
                    return false;
                }
                ++nextTextureLayers[category];
            }
            layer = entry->second;
            return true;
        };
        for (const auto& sourceMaterial : asset.materials)
        {
            std::array<std::uint32_t, 4u> layers{};
            if (textureSource != nullptr)
            {
                const auto provider=std::find(uniqueAssets.begin(),uniqueAssets.end(),textureSource);
                if(sourceMaterial.deferTextureAllocation ||
                    deferredGroups.contains({static_cast<std::size_t>(provider-uniqueAssets.begin()),sourceMaterial.textureGroup})) {
                    diagnostic="RtStaticMeshSlot deferred texture groups cannot be aliased.";
                    return false;
                }
                const std::array<bool, 4u> present{{sourceMaterial.baseColorTexture >= 0,
                    sourceMaterial.normalTexture >= 0, sourceMaterial.ormTexture >= 0,
                    sourceMaterial.emissiveTexture >= 0}};
                if (sourceMaterial.textureGroup < 0)
                {
                    if (std::any_of(present.begin(), present.end(), [](bool value) { return value; }))
                    {
                        diagnostic = "RtStaticMeshSlot aliased material with textures must declare an explicit texture group.";
                        return false;
                    }
                }
                else
                {
                    const auto presence = groupPresence.find(sourceMaterial.textureGroup);
                    const auto route = groupLayers.find(sourceMaterial.textureGroup);
                    if (presence == groupPresence.end() || route == groupLayers.end())
                    {
                        diagnostic = "RtStaticMeshSlot aliased material references a texture group absent from its provider.";
                        return false;
                    }
                    if (presence->second != present)
                    {
                        diagnostic = "RtStaticMeshSlot aliased texture group has conflicting four-category texture presence.";
                        return false;
                    }
                    layers = route->second;
                }
            }
            else if (sourceMaterial.textureGroup >= 0)
            {
                layers = groupLayers.at(sourceMaterial.textureGroup);
            }
            else
            {
                if (!routeTexture(0u, sourceMaterial.baseColorTexture, "baseColor", layers[0]) ||
                    !routeTexture(1u, sourceMaterial.normalTexture, "normal", layers[1]) ||
                    !routeTexture(2u, sourceMaterial.ormTexture, "ORM", layers[2]) ||
                    !routeTexture(3u, sourceMaterial.emissiveTexture, "emissive", layers[3]))
                    return false;
            }
            if (!std::isfinite(sourceMaterial.normalScale) ||
                std::any_of(sourceMaterial.textureScale.begin(), sourceMaterial.textureScale.end(),
                    [](float value) {
                        return !std::isfinite(value) || value < 1.0f / 1024.0f || value > 1024.0f;
                    }))
            {
                diagnostic = "RtStaticMeshSlot material normal/texture scale is outside the finite authoring contract.";
                return false;
            }
            if(sourceMaterial.deferTextureAllocation)
                deferredGroups.at({assetIndex,sourceMaterial.textureGroup}).materialIndices.push_back(materials_.size());
            materials_.push_back(ConvertMaterial(sourceMaterial, layers));
        }
        for (std::size_t primitiveIndex = 0u;
             primitiveIndex < asset.primitives.size(); ++primitiveIndex)
        {
            const auto& primitive = asset.primitives[primitiveIndex];
            if (primitive.materialIndex >= asset.materials.size())
            {
                diagnostic = "RtStaticMeshSlot primitive references an out-of-range material.";
                return false;
            }
            const std::uint32_t vertexEnd = primitiveIndex + 1u < asset.primitives.size()
                ? asset.primitives[primitiveIndex + 1u].vertexOffset
                : static_cast<std::uint32_t>(asset.vertices.size());
            if (primitive.vertexOffset >= asset.vertices.size() ||
                vertexEnd > asset.vertices.size() || vertexEnd <= primitive.vertexOffset)
            {
                diagnostic = "RtStaticMeshSlot primitive has an empty or out-of-range asset-local vertex span.";
                return false;
            }
            if (primitive.nodeTransformIndex >= asset.nodeTransforms.size())
            {
                diagnostic = "RtStaticMeshSlot primitive references an out-of-range node transform.";
                return false;
            }
            if(!primitive.indexCount || primitive.indexCount%3u || primitive.indexOffset%3u ||
               primitive.indexOffset>asset.indices.size() ||
               primitive.indexCount>asset.indices.size()-primitive.indexOffset ||
               std::any_of(asset.indices.begin()+primitive.indexOffset,
                   asset.indices.begin()+primitive.indexOffset+primitive.indexCount,
                   [&](auto index){return index>=vertexEnd-primitive.vertexOffset;})) {
                diagnostic="RtStaticMeshSlot primitive has invalid local triangle indices.";return false;
            }
            primitiveMetadata_.push_back({
                vertexBase + primitive.vertexOffset,
                indexBase + primitive.indexOffset,
                primitive.indexCount,
                materialBase + primitive.materialIndex});
            primitiveVertexCounts_.push_back(vertexEnd - primitive.vertexOffset);
            geometryTransforms_.push_back({{
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f}});
        }
        routes.emplace(&asset, AssetRoute{
            geometryRole, static_cast<std::uint32_t>(assetIndex), primitiveBase,
            static_cast<std::uint32_t>(asset.primitives.size()),
            std::move(groupPresence), std::move(groupLayers)});
    }

    // Canonical asset/group order, after every unchanged ordinary route. Only
    // CPU allocation is deferred; material/primitive/vertex ordering is retained.
    for(const auto& [key,group]:deferredGroups) {
        std::array<std::uint32_t,4u> layers{};
        for(std::size_t category=0;category<4;++category) if(group.present[category]) {
            if(nextTextureLayers[category]>=kRtTextureLayerCapacity) {
                diagnostic="RtStaticMeshSlot deferred texture layers exceed capacity.";
                return false;
            }
            layers[category]=nextTextureLayers[category]++;
        }
        for(const auto index:group.materialIndices) materials_[index].textureLayers=layers;
    }

    for (const StaticRtAssetRegistration& registration : registrations)
    {
        const AssetRoute route = routes.at(registration.asset);
        instanceMetadata_[registration.instanceCustomIndex] = {
            route.primitiveBase + registration.primitiveOffset,
            registration.primitiveCount == 0u ? route.primitiveCount : registration.primitiveCount,
            registration.stableObjectId, registration.flags,
            registration.emitterIndex, route.assetIndex,
            static_cast<std::uint32_t>(route.geometryRole), 0u};
    }
    textureArrayCounts_ = {
        nextTextureLayers[0], nextTextureLayers[1],
        nextTextureLayers[2], nextTextureLayers[3]};
    measurements_.vertexBytes =
        (vertices_.size() + worldBodyVertices_.size() + viewmodelVertices_.size()) *
        sizeof(horde::scene::assets::StaticRtVertex);
    measurements_.indexBytes = indices_.size() * sizeof(std::uint32_t);
    measurements_.materialBytes = materials_.size() * sizeof(RtMaterialGpu);
    measurements_.instanceMetadataBytes = instanceMetadata_.size() * sizeof(RtInstanceMetadata);
    measurements_.primitiveMetadataBytes = primitiveMetadata_.size() * sizeof(RtPrimitiveMetadata);
    measurements_.descriptorCount = 9u;
    admitted_ = true;
    diagnostic.clear();
    return true;
}

bool RtStaticMeshSlot::ApplyFrameMaterialAppearance(std::span<RtMaterialGpu> frameMaterials,
    const std::uint32_t metadataIndex, const RtMaterialAppearance& appearance,
    std::string& diagnostic) const
{
    const auto reject=[&](const char* message){diagnostic=message;return false;};
    if(metadataIndex>=instanceMetadata_.size() || frameMaterials.size()<materials_.size() ||
       frameMaterials.size()>kRtMaterialCapacity)
        return reject("Frame material appearance has unavailable owner/material storage.");
    const auto& owner=instanceMetadata_[metadataIndex];
    if(owner.primitiveCount!=1u || owner.primitiveBase>=primitiveMetadata_.size() ||
       !(owner.flags&static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr)) ||
       owner.geometryRole!=static_cast<std::uint32_t>(RtGeometryRole::Static))
        return reject("Frame material appearance requires one static-PBR primitive.");
    const auto row=primitiveMetadata_[owner.primitiveBase].materialIndex;
    if(row>=materials_.size() || row>=frameMaterials.size() ||
       (frameMaterials[row].materialFlags[0]&
        (static_cast<std::uint32_t>(RtMaterialFlag::Alpha)|static_cast<std::uint32_t>(RtMaterialFlag::Transmission))))
        return reject("Frame material appearance requires an available opaque row.");
    for(std::size_t i=0;i<instanceMetadata_.size();++i) {
        if(i==metadataIndex)continue;
        const auto& other=instanceMetadata_[i];
        if(other.primitiveCount && owner.primitiveBase>=other.primitiveBase &&
           owner.primitiveBase-other.primitiveBase<other.primitiveCount)
            return reject("Frame material appearance cannot mutate shared geometry ownership.");
    }
    for(std::size_t i=0;i<primitiveMetadata_.size();++i)
        if(i!=owner.primitiveBase && primitiveMetadata_[i].materialIndex==row)
            return reject("Frame material appearance cannot mutate a shared material row.");
    for(unsigned i=0;i<4u;++i)
        if(!std::isfinite(appearance.baseColour[i]) || appearance.baseColour[i]<0.f ||
           appearance.baseColour[i]>1.f || !std::isfinite(appearance.emission[i]) || appearance.emission[i]<0.f)
            return reject("Frame material appearance factors must be finite and nonnegative.");
    if(appearance.baseColour[3]!=1.f)
        return reject("Frame material appearance cannot change opaque coverage.");
    for(unsigned i=0;i<3u;++i)
        if(double(appearance.emission[i])*appearance.emission[3]>std::numeric_limits<float>::max())
            return reject("Frame material appearance emission product is nonfinite.");
    frameMaterials[row].baseColorFactor=appearance.baseColour;
    frameMaterials[row].emissiveFactorStrength=appearance.emission;
    diagnostic.clear();return true;
}

bool RtStaticMeshSlot::ShareImmutableMaterialRows(
    std::span<const RtImmutableMaterialAlias> aliases, std::string& diagnostic)
{
    std::array<std::uint32_t, kRtMaterialCapacity> provider{};
    std::array<bool, kRtMaterialCapacity> removed{};
    for (std::uint32_t i = 0; i < provider.size(); ++i) provider[i] = i;
    for (const auto alias : aliases)
    {
        if (alias.consumer >= materials_.size() || alias.provider >= alias.consumer ||
            removed[alias.consumer])
        {
            diagnostic = "Immutable material aliases require unique consumers and earlier valid providers.";
            return false;
        }
        if (std::memcmp(&materials_[alias.consumer], &materials_[alias.provider],
                        sizeof(RtMaterialGpu)) != 0)
        {
            diagnostic = "Immutable material aliases must match every converted GPU field and texture route.";
            return false;
        }
        removed[alias.consumer] = true;
        provider[alias.consumer] = alias.provider;
    }
    for (const auto alias : aliases)
        if (removed[alias.provider])
        {
            diagnostic = "Immutable material alias chains are not permitted.";
            return false;
        }
    for (const auto& primitive : primitiveMetadata_)
        if (primitive.materialIndex >= materials_.size())
        {
            diagnostic = "Immutable material alias has an invalid primitive owner.";
            return false;
        }
    // All possible rejection precedes publication. Vertex/index/transform,
    // instance identities and texture ownership remain entirely unchanged.
    std::array<std::uint32_t, kRtMaterialCapacity> remap{};
    std::vector<RtMaterialGpu> retained;
    retained.reserve(materials_.size() - aliases.size());
    for (std::uint32_t i = 0; i < materials_.size(); ++i)
        if (!removed[i])
        {
            remap[i] = static_cast<std::uint32_t>(retained.size());
            retained.push_back(materials_[i]);
        }
    for (auto& primitive : primitiveMetadata_)
        primitive.materialIndex = remap[provider[primitive.materialIndex]];
    materials_ = std::move(retained);
    measurements_.materialBytes = materials_.size() * sizeof(RtMaterialGpu);
    diagnostic.clear();
    return true;
}

} // namespace horde::vulkan::raytracing
