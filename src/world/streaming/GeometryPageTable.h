#pragma once
#include "vulkan/raytracing/RtSceneAbi.generated.h"
#include <array>
#include <cstdint>
#include <limits>
#include <span>

namespace horde::world::streaming {
// Candidate ABI for independent static owners. Page0 is the retained shared
// domain; pages1-3 are leased detail domains. Prefix layout/capacities unchanged.
// This helper does not upload/adopt/free resources or prove GPU readiness.
inline constexpr std::uint32_t kGeometryPages=4;
inline constexpr std::uint32_t kGeometryPageShift=8;
struct AddressRange { std::uint64_t address=0, bytes=0; };
struct GeometryPageAddresses {
    AddressRange vertices, indices, primitives;
    std::uint32_t primitiveCount=0;
};
struct alignas(16) GeometryPageTable {
    std::array<horde::vulkan::raytracing::RtInstanceMetadata,
               horde::vulkan::raytracing::kRtInstanceMetadataCapacity> instances{};
    // Two uvec4 records/page: vertex low/high, index low/high; then primitive
    // low/high, zero/zero. Never encode C++ pointers or borrowed host storage.
    std::array<std::array<std::uint32_t,4>,kGeometryPages*2> addresses{};
};
static_assert(sizeof(GeometryPageTable)==sizeof(GeometryPageTable::instances)+128);
inline bool ValidRange(AddressRange range,std::uint64_t alignment,std::uint64_t minimum) noexcept {
    return range.address && !(range.address%alignment) && range.bytes>=minimum &&
           range.bytes<=std::numeric_limits<std::uint64_t>::max()-range.address;
}
inline bool BuildGeometryPageTable(
    std::span<const horde::vulkan::raytracing::RtInstanceMetadata> metadata,
    const std::array<GeometryPageAddresses,kGeometryPages>& pages,
    GeometryPageTable& output) noexcept {
    using namespace horde::vulkan::raytracing;
    if(metadata.size()!=kRtInstanceMetadataCapacity)return false;
    if(!pages[0].primitiveCount)return false;
    GeometryPageTable candidate{};
    for(std::size_t p=0;p<pages.size();++p) {
        const auto& page=pages[p];
        if(!page.primitiveCount) {
            if(page.vertices.address || page.indices.address || page.primitives.address ||
               page.vertices.bytes || page.indices.bytes || page.primitives.bytes)return false;
            continue;
        }
        if(page.primitiveCount>kRtPrimitiveMetadataCapacity ||
           !ValidRange(page.vertices,16,sizeof(StaticRtVertex)) ||
           !ValidRange(page.indices,4,3*sizeof(std::uint32_t)) ||
           !ValidRange(page.primitives,16,page.primitiveCount*sizeof(RtPrimitiveMetadata)))return false;
        const auto words=[](std::uint64_t a){return std::array<std::uint32_t,2>{
            static_cast<std::uint32_t>(a),static_cast<std::uint32_t>(a>>32)};};
        const auto v=words(page.vertices.address),i=words(page.indices.address),r=words(page.primitives.address);
        candidate.addresses[p*2]={v[0],v[1],i[0],i[1]};
        candidate.addresses[p*2+1]={r[0],r[1],0,0};
    }
    constexpr std::uint32_t pageMask=(kGeometryPages-1)<<kGeometryPageShift;
    constexpr std::uint32_t allowed=static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr) |
        static_cast<std::uint32_t>(RtInstanceFlag::Emissive) |
        static_cast<std::uint32_t>(RtInstanceFlag::Transmissive) |
        static_cast<std::uint32_t>(RtInstanceFlag::BodyRemainderOnlyPrimary) | pageMask;
    for(std::size_t i=0;i<metadata.size();++i) {
        const auto& m=metadata[i];
        // The shader intentionally does NOT mask invalid high flag/page bits.
        // Reject them before any metadata can reach a command/reference owner.
        if(m.flags&~allowed)return false;
        const auto page=m.flags>>kGeometryPageShift;
        if(page>=kGeometryPages || (page && (!(m.flags&static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr)) ||
           (m.flags&static_cast<std::uint32_t>(RtInstanceFlag::Transmissive)) ||
           m.geometryRole!=static_cast<std::uint32_t>(RtGeometryRole::Static))))return false;
        if(m.flags&static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr)) {
            if(!m.primitiveCount || !pages[page].primitiveCount || m.primitiveBase>=pages[page].primitiveCount ||
               m.primitiveCount>pages[page].primitiveCount-m.primitiveBase)return false;
        } else if(m.primitiveCount)return false;
        candidate.instances[i]=m;
    }
    output=candidate;return true;
}
}
