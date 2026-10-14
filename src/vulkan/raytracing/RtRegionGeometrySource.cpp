#include "vulkan/raytracing/RtRegionGeometrySource.h"
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
        GeometryPartition p;
        if(!PartitionGeometry({s.Vertices(),s.Indices(),s.PrimitiveVertexCounts(),s.PrimitiveMetadata(),
            s.InstanceMetadata(),s.Materials(),s.GeometryTransforms()},instance,triangles,limit-extras,p,error))return false;
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
