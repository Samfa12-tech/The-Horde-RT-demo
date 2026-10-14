#include "world/streaming/GeometryPartition.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
using namespace horde::world::streaming;
using namespace horde::vulkan::raytracing;
int main() {
    int failures=0;const auto check=[&](bool v,const char* what){if(!v){++failures;std::cerr<<what<<'\n';}};
    std::vector<StaticRtVertex> vertices(16);
    for(unsigned i=0;i<vertices.size();++i) {
        vertices[i].position={float(i),float(i%3),float(i%2),1};
        vertices[i].normal={float(i),2,3,4};vertices[i].tangent={5,6,7,-1};vertices[i].uv0={float(i)/16,0.5,0,0};
    }
    std::vector<std::uint32_t> indices{0,1,2, 2,3,4, 0,1,2, 2,3,4, 0,1,2, 0,1,2};
    std::vector<std::uint32_t> counts{6,6,4,3};
    std::vector<RtPrimitiveMetadata> primitives{{0,0,6,0},{6,6,6,1},{12,12,3,0},{0,15,3,0}};
    std::array<RtInstanceMetadata,3> instances{};
    instances[0]={0,2,101,1,0,0,0,0};instances[1]={2,1,102,1,0,1,0,0};instances[2]={3,1,103,1,0,2,1,0};
    std::array<RtMaterialGpu,2> materials{};
    std::array<std::array<float,12>,4> transforms{};
    for(auto& t:transforms)t={1,0,0,0,0,1,0,0,0,0,1,0};
    const std::array<std::uint32_t,2> selected{0,3};
    auto source=[&]{return GeometryPartitionSource{vertices,indices,counts,primitives,instances,materials,transforms};};
    GeometryPartition result;std::string error;
    if(!PartitionGeometry(source(),0,selected,3000,result,error)){std::cerr<<error<<'\n';return 1;}
    check(result.base.primitives.size()==4 && result.detail.primitives.size()==2,"base identities and private detail rows retained");
    check(result.detail.primitives[0].materialIndex==0 && result.detail.primitives[1].materialIndex==1,"shared global material IDs survive partition");
    check(result.base.primitives[3].vertexOffset==0 && result.base.vertexCounts[3]==3,"player domain/layout stays unchanged");
    check(result.base.vertexCounts[2]==4 && !std::memcmp(&result.base.vertices[result.base.primitives[2].vertexOffset],&vertices[12],4*sizeof(StaticRtVertex)),"complete dynamic/held layout preserves unindexed vertices");
    std::vector<unsigned> seen(indices.size()/3);
    const auto inspect=[&](const GeometryPayload& payload,bool detail) {
        check(payload.sourceTriangles.size()==payload.indices.size()/3,"one source ID per complete triangle");
        for(const auto& p:payload.primitives)for(std::size_t t=p.indexOffset;t<p.indexOffset+p.indexCount;t+=3) {
            const auto original=payload.sourceTriangles[t/3];++seen[original];
            auto src=std::find_if(primitives.begin(),primitives.end(),[&](const auto& r){return original*3>=r.indexOffset && original*3<r.indexOffset+r.indexCount;});
            check(src!=primitives.end() && p.materialIndex==src->materialIndex,"per-triangle material attribution");
            for(unsigned k=0;k<3;++k) {
                if(original==5) {check(!detail && payload.indices[t+k]==indices[original*3+k],"player indices stay exact");continue;}
                const auto& actual=payload.vertices[p.vertexOffset+payload.indices[t+k]];
                const auto& expected=vertices[src->vertexOffset+indices[original*3+k]];
                check(!std::memcmp(&actual,&expected,sizeof(actual)),"every attribute and winding preserved on both sides");
            }
        }
    };
    inspect(result.base,false);inspect(result.detail,true);
    for(auto count:seen)check(count==1,"every source triangle appears exactly once");
    check(result.detail.bounds.Valid() && result.detail.bounds.min[0]==0 && result.detail.bounds.max[0]==10,"actual selected bounds");
    const auto savedBytes=result.detail.GeometryBytes();
    check(!PartitionGeometry(source(),0,selected,result.base.GeometryBytes()+result.detail.GeometryBytes(),result,error),
          "GPU-only payload count cannot understate CPU provenance and scratch allowance");
    check(!PartitionGeometry(source(),0,selected,100,result,error) && result.detail.GeometryBytes()==savedBytes,"allowance failure preserves previous payload");
    for(const std::vector<std::uint32_t> bad: {std::vector<std::uint32_t>{3,0}, {0,0}, {6}, {0,1}})
        check(!PartitionGeometry(source(),0,bad,3000,result,error),"unordered/duplicate/outside/empty-base-row rejected");
    materials[0].materialFlags[0]=static_cast<unsigned>(RtMaterialFlag::Alpha);
    check(!PartitionGeometry(source(),0,selected,3000,result,error),"alpha cannot become opaque page");
    materials[0].materialFlags[0]=static_cast<unsigned>(RtMaterialFlag::Transmission);
    check(!PartitionGeometry(source(),0,selected,3000,result,error),"transmission flag rejected even at zero strength");
    materials[0].materialFlags[0]=0;instances[1].primitiveBase=1;
    check(!PartitionGeometry(source(),0,selected,3000,result,error),"shared selection ownership rejected");
    instances[1].primitiveBase=2;indices[0]=6;
    check(!PartitionGeometry(source(),0,selected,3000,result,error),"out-of-domain index rejected");
    indices[0]=0;transforms[0][3]=2;
    check(!PartitionGeometry(source(),0,selected,3000,result,error),"unbaked transform cannot supply misleading page bounds");
    transforms[0][3]=0;vertices[0].position[0]=INFINITY;
    check(!PartitionGeometry(source(),0,selected,3000,result,error),"nonfinite geometry rejected");
    {
        std::vector<StaticRtVertex> sparseVertices(1024);
        const std::array<std::uint32_t,6> sparseIndices{0,1,2,0,2,3};
        const std::array<std::uint32_t,1> sparseCounts{1024},oneTriangle{0};
        const std::array<RtPrimitiveMetadata,1> sparsePrimitives{{{0,0,6,0}}};
        const std::array<RtInstanceMetadata,1> sparseInstances{{{0,1,201,1,0,0,0,0}}};
        const std::array<std::array<float,12>,1> identity{{{1,0,0,0,0,1,0,0,0,0,1,0}}};
        GeometryPartitionSource sparse{sparseVertices,sparseIndices,sparseCounts,sparsePrimitives,sparseInstances,materials,identity};
        check(!PartitionGeometry(sparse,0,oneTriangle,3000,result,error) && error.find("scratch")!=std::string::npos,
              "large sparse remaps are rejected before allocation despite a tiny final vertex payload");
    }
    if(!failures)std::cout<<"Compact base/detail attributes, ownership, bounds and negatives passed; no GPU saving claim.\n";
    return failures?1:0;
}
