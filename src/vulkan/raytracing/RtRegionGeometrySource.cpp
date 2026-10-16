#include "vulkan/raytracing/RtRegionGeometrySource.h"
#include <algorithm>
#include <new>

namespace horde::vulkan::raytracing {
bool RtRegionGeometrySource::Prepare(const RtStaticMeshSlot& s,std::uint32_t instance,
    std::span<const std::uint32_t> triangles,std::uint64_t limit,
    RtRegionGeometrySourceResult& out,std::string& error) {
    using namespace horde::world::streaming;
    if(!s.admitted_ || s.TriangleRegions().size()!=s.Indices().size()/3) {
        error="Region source requires an admitted complete native geometry/provenance owner.";return false;
    }
    const auto extras=sizeof(RtStaticMeshSlot)+sizeof(RtRegionGeometrySourceResult)+
        s.materials_.size()*sizeof(RtMaterialGpu)+
        (s.Vertices(RtGeometryRole::PlayerWorldBody).size()+s.Vertices(RtGeometryRole::PlayerViewmodel).size())*sizeof(StaticRtVertex)+
        s.geometryTransforms_.size()*sizeof(s.geometryTransforms_.front())+
        s.triangleRegions_.size()*sizeof(s.triangleRegions_.front());
    if(extras>=limit){error="Region source shared CPU records exceed its preparation allowance.";return false;}
    try {
        // Native quality profiles may retain unused backing indices (for
        // example an open aperture) while removing their primitive references.
        // Build a canonical view of referenced ranges; never relax partition
        // ownership, restore excluded geometry, or change source-global IDs.
        std::uint64_t end=0,active=0;bool sparse=false;
        if(s.Indices().size()>UINT32_MAX || s.PrimitiveMetadata().size()>kRtPrimitiveMetadataCapacity) {
            error="Region source domains exceed addressable capacity.";return false;
        }
        for(const auto& r:s.PrimitiveMetadata()) {
            if(!r.indexCount || r.indexCount%3 || r.indexOffset%3 || r.indexOffset<end ||
               r.indexOffset>s.Indices().size() || r.indexCount>s.Indices().size()-r.indexOffset) {
                error="Region source referenced index ranges are invalid or overlapping.";return false;
            }
            sparse|=r.indexOffset!=end;end=std::uint64_t(r.indexOffset)+r.indexCount;active+=r.indexCount;
        }
        sparse|=end!=s.Indices().size();
        std::vector<std::uint32_t> indices,sourceTriangles,selection;
        std::vector<RtPrimitiveMetadata> primitives;
        std::uint64_t normalizationBytes=0;
        if(sparse) {
            if(triangles.empty() || triangles.size()>s.Indices().size()/3 ||
               !std::is_sorted(triangles.begin(),triangles.end()) ||
               std::adjacent_find(triangles.begin(),triangles.end())!=triangles.end()) {
                error="Sparse source selection must be bounded, sorted and unique.";return false;
            }
            const auto planned=sizeof(indices)+sizeof(sourceTriangles)+sizeof(selection)+sizeof(primitives)+
                (active+active/3+triangles.size())*sizeof(std::uint32_t)+
                s.PrimitiveMetadata().size()*sizeof(RtPrimitiveMetadata);
            if(planned>=limit-extras){error="Sparse source normalization exceeds its preparation allowance.";return false;}
            indices.reserve(static_cast<std::size_t>(active));sourceTriangles.reserve(static_cast<std::size_t>(active/3));
            selection.reserve(triangles.size());primitives.reserve(s.PrimitiveMetadata().size());
            normalizationBytes=sizeof(indices)+sizeof(sourceTriangles)+sizeof(selection)+sizeof(primitives)+
                (indices.capacity()+sourceTriangles.capacity()+selection.capacity())*sizeof(std::uint32_t)+
                primitives.capacity()*sizeof(RtPrimitiveMetadata);
            if(normalizationBytes>=limit-extras){error="Sparse source allocation exceeds its preparation allowance.";return false;}
            std::size_t selected=0;
            for(const auto& r:s.PrimitiveMetadata()) {
                auto row=r;row.indexOffset=static_cast<std::uint32_t>(indices.size());primitives.push_back(row);
                indices.insert(indices.end(),s.Indices().begin()+r.indexOffset,s.Indices().begin()+r.indexOffset+r.indexCount);
                for(std::uint32_t id=r.indexOffset/3;id<(r.indexOffset+r.indexCount)/3;++id) {
                    if(selected<triangles.size() && triangles[selected]<id) {
                        error="Detail selection names unreferenced source storage.";return false;
                    }
                    if(selected<triangles.size() && triangles[selected]==id) {
                        selection.push_back(static_cast<std::uint32_t>(sourceTriangles.size()));++selected;
                    }
                    sourceTriangles.push_back(id);
                }
            }
            if(selected!=triangles.size()){error="Detail selection names unreferenced source storage.";return false;}
        }
        GeometryPartition p;
        if(!PartitionGeometry({s.Vertices(),sparse?std::span<const std::uint32_t>(indices):s.Indices(),
            s.PrimitiveVertexCounts(),sparse?std::span<const RtPrimitiveMetadata>(primitives):s.PrimitiveMetadata(),
            s.InstanceMetadata(),s.Materials(),s.GeometryTransforms()},instance,
            sparse?std::span<const std::uint32_t>(selection):triangles,limit-extras-normalizationBytes,p,error))return false;
        if(sparse)for(auto* payload:{&p.base,&p.detail})for(auto& id:payload->sourceTriangles) {
            if(id>=sourceTriangles.size()){error="Partition returned an invalid source triangle identity.";return false;}
            id=sourceTriangles[id];
        }
        auto base=std::make_unique<RtStaticMeshSlot>();
        base->instanceMetadata_=s.instanceMetadata_;base->materials_=s.materials_;
        base->worldBodyVertices_=s.Vertices(RtGeometryRole::PlayerWorldBody);
        base->viewmodelVertices_=s.Vertices(RtGeometryRole::PlayerViewmodel);
        base->geometryTransforms_=s.geometryTransforms_;base->textureArrayCounts_=s.textureArrayCounts_;
        base->triangleRegions_.reserve(p.base.sourceTriangles.size());
        for(auto id:p.base.sourceTriangles)base->triangleRegions_.push_back(s.triangleRegions_[id]);
        const auto actualExtras=sizeof(*base)+sizeof(RtRegionGeometrySourceResult)+
            base->materials_.capacity()*sizeof(RtMaterialGpu)+
            (base->worldBodyVertices_.capacity()+base->viewmodelVertices_.capacity())*sizeof(StaticRtVertex)+
            base->geometryTransforms_.capacity()*sizeof(base->geometryTransforms_.front())+
            base->triangleRegions_.capacity()*sizeof(base->triangleRegions_.front());
        // Account allocator capacities before publication. The partitioner has
        // already separately enforced its result/provenance/remap allowance.
        if(actualExtras>extras){error="Region source allocator exceeded its declared shared-record allowance.";return false;}
        base->vertices_=std::move(p.base.vertices);base->indices_=std::move(p.base.indices);
        base->primitiveMetadata_=std::move(p.base.primitives);base->primitiveVertexCounts_=std::move(p.base.vertexCounts);
        base->measurements_=s.measurements_;
        base->measurements_.vertexBytes=(base->vertices_.size()+base->worldBodyVertices_.size()+base->viewmodelVertices_.size())*sizeof(StaticRtVertex);
        base->measurements_.indexBytes=base->indices_.size()*sizeof(std::uint32_t);
        base->admitted_=true;
        auto detail=std::make_shared<const GeometryPayload>(std::move(p.detail));
        out.base=std::move(base);out.detail=std::move(detail);error.clear();return true;
    }catch(const std::bad_alloc&){error="Region source preparation allocation failed; previous owner retained.";return false;}
}
}
