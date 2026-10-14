#include "world/streaming/GeometryPartition.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>

namespace horde::world::streaming {
bool PartitionGeometry(const GeometryPartitionSource& s,std::uint32_t instance,
    std::span<const std::uint32_t> selection,std::uint64_t byteLimit,
    GeometryPartition& output,std::string& error) {
    using namespace horde::vulkan::raytracing;
    const auto reject=[&](const char* why){error=why;return false;};
    if(!byteLimit || instance>=s.instances.size() || selection.empty() || s.vertices.empty() ||
       s.primitives.empty() || s.primitives.size()>kRtPrimitiveMetadataCapacity ||
       s.vertexCounts.size()!=s.primitives.size() || s.geometryTransforms.size()!=s.primitives.size() ||
       s.materials.empty() || s.materials.size()>kRtMaterialCapacity ||
       s.vertices.size()>UINT32_MAX || s.indices.size()>UINT32_MAX || s.indices.size()%3)
        return reject("Invalid partition domains, selection or explicit output allowance.");
    if(s.indices.size()*sizeof(std::uint32_t)+s.primitives.size()*sizeof(RtPrimitiveMetadata)>byteLimit)
        return reject("Output allowance cannot even retain source indices/primitive identities.");
    const auto& owner=s.instances[instance];
    if(owner.geometryRole!=static_cast<std::uint32_t>(RtGeometryRole::Static) ||
       !(owner.flags&static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr)) ||
       (owner.flags&static_cast<std::uint32_t>(RtInstanceFlag::Transmissive)) ||
       !owner.primitiveCount || owner.primitiveBase>=s.primitives.size() ||
       owner.primitiveCount>s.primitives.size()-owner.primitiveBase)
        return reject("Region partition needs an admitted opaque static owner.");
    const auto end=owner.primitiveBase+owner.primitiveCount;
    std::vector<std::uint32_t> roles(s.primitives.size(),UINT32_MAX);
    for(std::size_t i=0;i<s.instances.size();++i) {
        const auto& m=s.instances[i];if(!m.primitiveCount)continue;
        if(m.primitiveBase>=s.primitives.size() || m.primitiveCount>s.primitives.size()-m.primitiveBase ||
           m.geometryRole>static_cast<std::uint32_t>(RtGeometryRole::PlayerViewmodel))
            return reject("Invalid source instance primitive/role span.");
        if(i!=instance && m.primitiveBase<end && owner.primitiveBase<m.primitiveBase+m.primitiveCount)
            return reject("Detail selection cannot modify another instance's geometry.");
        for(auto p=m.primitiveBase;p<m.primitiveBase+m.primitiveCount;++p) {
            if(roles[p]!=UINT32_MAX && roles[p]!=m.geometryRole)
                return reject("Source primitive aliases conflicting vertex domains.");
            roles[p]=m.geometryRole;
        }
    }
    std::uint64_t indexEnd=0;
    std::vector<std::pair<std::uint32_t,std::uint32_t>> staticSpans;
    for(std::size_t p=0;p<s.primitives.size();++p) {
        const auto& r=s.primitives[p];
        if(roles[p]==UINT32_MAX || r.indexOffset!=indexEnd || !r.indexCount || r.indexCount%3 ||
           r.indexCount>s.indices.size()-std::min<std::uint64_t>(indexEnd,s.indices.size()) || !s.vertexCounts[p] ||
           r.materialIndex>=s.materials.size())return reject("Invalid or unowned source primitive boundary.");
        indexEnd+=r.indexCount;
        if(roles[p]==static_cast<std::uint32_t>(RtGeometryRole::Static)) {
            if(r.vertexOffset>=s.vertices.size() || s.vertexCounts[p]>s.vertices.size()-r.vertexOffset)
                return reject("Static primitive vertex span is outside its buffer.");
            staticSpans.emplace_back(r.vertexOffset,r.vertexOffset+s.vertexCounts[p]);
        }
        for(std::size_t i=r.indexOffset;i<indexEnd;++i)
            if(s.indices[i]>=s.vertexCounts[p])return reject("Out-of-domain triangle index.");
        if(p>=owner.primitiveBase && p<end) {
            constexpr std::array<float,12> identity{1,0,0,0,0,1,0,0,0,0,1,0};
            if(std::memcmp(s.geometryTransforms[p].data(),identity.data(),sizeof(identity)))
                return reject("First region pages require verified world-baked identity geometry.");
            const auto& m=s.materials[r.materialIndex];
            if((m.materialFlags[0]&(static_cast<std::uint32_t>(RtMaterialFlag::Alpha)|
                 static_cast<std::uint32_t>(RtMaterialFlag::Transmission))) ||
               !std::isfinite(m.metallicRoughnessOcclusionTransmission[3]) ||
               m.metallicRoughnessOcclusionTransmission[3]!=0.f)
                return reject("Alpha/transmissive or uncertain geometry stays in the shared base.");
        }
    }
    if(indexEnd!=s.indices.size())return reject("Index buffer has an unowned suffix.");
    std::sort(staticSpans.begin(),staticSpans.end());std::uint32_t vertexEnd=0;
    for(const auto& span:staticSpans) {
        if(span.first!=vertexEnd)return reject("Static vertex ownership must be complete and exclusive.");
        vertexEnd=span.second;
    }
    if(vertexEnd!=s.vertices.size())return reject("Static buffer has an unowned suffix.");
    const auto firstTriangle=s.primitives[owner.primitiveBase].indexOffset/3u;
    const auto lastTriangle=(s.primitives[end-1].indexOffset+s.primitives[end-1].indexCount)/3u;
    for(std::size_t i=0;i<selection.size();++i)
        if(selection[i]<firstTriangle || selection[i]>=lastTriangle || (i && selection[i]<=selection[i-1]))
            return reject("Selection must be sorted, unique and inside the exclusive owner.");
    try {
        // Count touched vertices before allocating result capacity. Both counting
        // and final remaps fit the declared scratch allowance; sparse large spans
        // cannot allocate a giant map behind a small output allowance.
        std::uint64_t mapBytes=0;
        for(auto p=owner.primitiveBase;p<end;++p)
            mapBytes=std::max<std::uint64_t>(mapBytes,2ull*s.vertexCounts[p]*sizeof(std::uint32_t));
        const auto planningBytes=roles.capacity()*sizeof(std::uint32_t)+
            staticSpans.capacity()*sizeof(staticSpans.front());
        if(planningBytes>byteLimit || mapBytes>byteLimit-planningBytes)
            return reject("Remap scratch exceeds the explicit preparation allowance.");
        if(mapBytes>byteLimit)return reject("Remap scratch exceeds the explicit preparation allowance.");
        std::array<std::uint32_t,kRtPrimitiveMetadataCapacity> baseCounts{},detailCounts{};
        std::uint64_t baseVertexTotal=0,detailVertexTotal=0;
        std::size_t selected=0,detailRows=0;
        for(std::size_t p=0;p<s.primitives.size();++p) {
            const auto& row=s.primitives[p];
            if(p<owner.primitiveBase || p>=end) {
                baseCounts[p]=s.vertexCounts[p];
                if(roles[p]==static_cast<std::uint32_t>(RtGeometryRole::Static))baseVertexTotal+=s.vertexCounts[p];
                continue;
            }
            std::vector<std::uint8_t> baseTouched(s.vertexCounts[p]),detailTouched(s.vertexCounts[p]);
            for(std::size_t i=row.indexOffset;i<row.indexOffset+row.indexCount;i+=3) {
                const bool take=selected<selection.size() && selection[selected]==i/3;
                if(take)++selected;
                auto& touched=take?detailTouched:baseTouched;
                auto& count=take?detailCounts[p]:baseCounts[p];
                for(unsigned k=0;k<3;++k)if(!touched[s.indices[i+k]]) {touched[s.indices[i+k]]=1;++count;}
            }
            if(!baseCounts[p])return reject("First page admission must retain every base primitive identity.");
            baseVertexTotal+=baseCounts[p];detailVertexTotal+=detailCounts[p];detailRows+=detailCounts[p]!=0;
        }
        const auto baseIndices=s.indices.size()-selection.size()*3,detailIndices=selection.size()*3;
        const auto plannedBytes=planningBytes+sizeof(GeometryPartition)+(baseVertexTotal+detailVertexTotal)*sizeof(StaticRtVertex)+
            s.indices.size()*sizeof(std::uint32_t)+s.indices.size()/3*sizeof(std::uint32_t)+
            (s.primitives.size()+detailRows)*(sizeof(RtPrimitiveMetadata)+sizeof(std::uint32_t));
        if(plannedBytes>byteLimit || mapBytes>byteLimit-plannedBytes)
            return reject("Result plus provenance/remap scratch exceeds the explicit preparation allowance.");
        GeometryPartition candidate;
        candidate.base.vertices.reserve(static_cast<std::size_t>(baseVertexTotal));
        candidate.detail.vertices.reserve(static_cast<std::size_t>(detailVertexTotal));
        candidate.base.indices.reserve(baseIndices);candidate.detail.indices.reserve(detailIndices);
        candidate.base.sourceTriangles.reserve(baseIndices/3);candidate.detail.sourceTriangles.reserve(detailIndices/3);
        candidate.base.primitives.reserve(s.primitives.size());candidate.detail.primitives.reserve(detailRows);
        candidate.base.vertexCounts.reserve(s.primitives.size());candidate.detail.vertexCounts.reserve(detailRows);
        candidate.base.bounds.min=candidate.detail.bounds.min={INFINITY,INFINITY,INFINITY};
        candidate.base.bounds.max=candidate.detail.bounds.max={-INFINITY,-INFINITY,-INFINITY};
        selected=0;
        const auto withinAllowance=[&]{return candidate.base.StorageBytes()+candidate.detail.StorageBytes()+mapBytes+planningBytes<=byteLimit;};
        if(!withinAllowance())return reject("Allocator capacity exceeds the declared preparation allowance.");
        const auto appendVertex=[&](GeometryPayload& target,const StaticRtVertex& v) {
            for(unsigned a=0;a<3;++a) {
                if(!std::isfinite(v.position[a]))return false;
                target.bounds.min[a]=std::min(target.bounds.min[a],v.position[a]);
                target.bounds.max[a]=std::max(target.bounds.max[a],v.position[a]);
            }
            target.vertices.push_back(v);return withinAllowance();
        };
        for(std::size_t p=0;p<s.primitives.size();++p) {
            const auto& source=s.primitives[p];
            auto base=source;base.indexOffset=static_cast<std::uint32_t>(candidate.base.indices.size());
            const bool world=p>=owner.primitiveBase && p<end;
            if(!world) {
                if(roles[p]==static_cast<std::uint32_t>(RtGeometryRole::Static)) {
                    base.vertexOffset=static_cast<std::uint32_t>(candidate.base.vertices.size());
                    // Retain complete pose/grip layouts, including unindexed vertices.
                    for(unsigned v=0;v<s.vertexCounts[p];++v)
                        if(!appendVertex(candidate.base,s.vertices[source.vertexOffset+v]))return reject("Partition exceeds allowance or has nonfinite positions.");
                }
                candidate.base.indices.insert(candidate.base.indices.end(),s.indices.begin()+source.indexOffset,s.indices.begin()+source.indexOffset+source.indexCount);
                for(auto i=source.indexOffset/3;i<(source.indexOffset+source.indexCount)/3;++i)candidate.base.sourceTriangles.push_back(i);
                candidate.base.vertexCounts.push_back(s.vertexCounts[p]);candidate.base.primitives.push_back(base);
                if(!withinAllowance())return reject("Partition exceeds its explicit output allowance.");
                continue;
            }
            base.vertexOffset=static_cast<std::uint32_t>(candidate.base.vertices.size());base.indexCount=0;
            auto detail=source;detail.vertexOffset=static_cast<std::uint32_t>(candidate.detail.vertices.size());
            detail.indexOffset=static_cast<std::uint32_t>(candidate.detail.indices.size());detail.indexCount=0;
            std::vector<std::uint32_t> baseMap(s.vertexCounts[p],UINT32_MAX),detailMap(s.vertexCounts[p],UINT32_MAX);
            unsigned baseVertices=0,detailVertices=0;
            for(std::size_t i=source.indexOffset;i<source.indexOffset+source.indexCount;i+=3) {
                const auto triangle=static_cast<std::uint32_t>(i/3);
                const bool take=selected<selection.size() && selection[selected]==triangle;
                if(take)++selected;
                auto& target=take?candidate.detail:candidate.base;
                auto& map=take?detailMap:baseMap;
                auto& count=take?detailVertices:baseVertices;
                auto& row=take?detail:base;
                for(unsigned k=0;k<3;++k) {
                    const auto vertex=s.indices[i+k];
                    if(map[vertex]==UINT32_MAX) {
                        map[vertex]=count++;
                        if(!appendVertex(target,s.vertices[source.vertexOffset+vertex]))return reject("Partition exceeds allowance or has nonfinite positions.");
                    }
                    target.indices.push_back(map[vertex]);++row.indexCount;
                }
                target.sourceTriangles.push_back(triangle);
                if(!withinAllowance())return reject("Partition exceeds its explicit output allowance.");
            }
            if(!base.indexCount)return reject("First page admission must retain every base primitive identity.");
            candidate.base.vertexCounts.push_back(baseVertices);candidate.base.primitives.push_back(base);
            if(detail.indexCount) {candidate.detail.vertexCounts.push_back(detailVertices);candidate.detail.primitives.push_back(detail);}
            if(!withinAllowance())return reject("Partition exceeds its explicit output allowance.");
        }
        if(selected!=selection.size() || candidate.detail.indices.empty() || !candidate.detail.bounds.Valid())
            return reject("Incomplete partition selection or bounds.");
        output=std::move(candidate);error.clear();return true;
    } catch(const std::bad_alloc&) {return reject("Region geometry preparation allocation failed.");}
}
}
