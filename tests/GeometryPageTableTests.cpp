#include "world/streaming/GeometryPageTable.h"
#include <cstring>
#include <iostream>
using namespace horde::world::streaming;
using namespace horde::vulkan::raytracing;
int main() {
    int failures=0;const auto check=[&](bool v,const char* what){if(!v){++failures;std::cerr<<what<<'\n';}};
    std::array<RtInstanceMetadata,kRtInstanceMetadataCapacity> metadata{};
    std::array<GeometryPageAddresses,kGeometryPages> pages{};
    pages[0]={{0x123456781000,128},{0xABCDE0002000,24},{0x345678003000,32},2};
    for(unsigned p=1;p<kGeometryPages;++p)pages[p]={{0x10000+0x100*p,128},{0x20000+0x100*p,24},{0x30000+0x100*p,16},1};
    for(unsigned p=0;p<kGeometryPages;++p) {
        metadata[p].flags=static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr)|(p<<kGeometryPageShift);
        metadata[p].primitiveCount=1;
    }
    GeometryPageTable table;check(BuildGeometryPageTable(metadata,pages,table),"valid four-page immutable table");
    check(!std::memcmp(metadata.data(),table.instances.data(),sizeof(metadata)),"legacy instance prefix is byte-identical");
    check(table.addresses[0]==std::array<std::uint32_t,4>{0x56781000,0x1234,0xE0002000,0xABCD},"full 64-bit addresses preserve high words");
    for(unsigned p=0;p<kGeometryPages;++p) {
        const auto v=metadata[p].flags>>7u;
        check(v==p*2 && v+1<table.addresses.size(),"shader indexing matches all admitted identities");
    }
    const auto preserved=table;
    for(auto flag:{16u,128u,1024u,0x80000000u}) {
        auto bad=metadata;bad[1].flags|=flag;
        check(!BuildGeometryPageTable(bad,pages,table),"unknown flag/overflow cannot alias a valid page");
        check(!std::memcmp(&table,&preserved,sizeof(table)),"failed admission leaves previous table intact");
    }
    auto bad=metadata;bad[1].geometryRole=static_cast<std::uint32_t>(RtGeometryRole::PlayerViewmodel);
    check(!BuildGeometryPageTable(bad,pages,table),"detail page cannot hijack player vertex ownership");
    bad=metadata;bad[1].flags|=static_cast<std::uint32_t>(RtInstanceFlag::Transmissive);
    check(!BuildGeometryPageTable(bad,pages,table),"opaque page owner cannot be paired with transmissive instance semantics");
    bad=metadata;bad[1].primitiveBase=1;check(!BuildGeometryPageTable(bad,pages,table),"primitive base overflow rejected");
    bad=metadata;bad[1].primitiveCount=2;check(!BuildGeometryPageTable(bad,pages,table),"primitive count overflow rejected");
    auto ranges=pages;ranges[1].vertices.address+=1;check(!BuildGeometryPageTable(metadata,ranges,table),"unaligned vertex address rejected");
    ranges=pages;ranges[1].primitives.bytes=15;check(!BuildGeometryPageTable(metadata,ranges,table),"truncated primitive buffer rejected");
    ranges=pages;ranges[1].indices.address=UINT64_MAX-3;check(!BuildGeometryPageTable(metadata,ranges,table),"address plus extent overflow rejected");
    ranges=pages;ranges[1]={};check(!BuildGeometryPageTable(metadata,ranges,table),"active page cannot reference absent owner");
    bad=metadata;bad[1]={};check(BuildGeometryPageTable(bad,ranges,table),"absent owner permitted only with no referencing instance");
    check(!BuildGeometryPageTable(std::span(metadata).first(25),pages,table),"truncated metadata prefix rejected");
    std::array<GeometryPageAddresses,kGeometryPages> emptyPages{};
    std::array<RtInstanceMetadata,kRtInstanceMetadataCapacity> emptyMetadata{};
    check(!BuildGeometryPageTable(emptyMetadata,emptyPages,table),"retained base domain must have a real owner");
    if(!failures)std::cout<<"Page table address/layout/overflow admission passed; no GPU ownership claim.\n";
    return failures?1:0;
}
