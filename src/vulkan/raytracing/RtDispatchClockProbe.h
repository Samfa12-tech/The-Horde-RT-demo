#pragma once
#include "world/streaming/GeometryPageTable.h"
#include <array>
#include <cstdint>
#include <limits>
#include <span>

namespace horde::vulkan::raytracing {
// Private observer tail of the existing metadata allocation; no new descriptor,
// geometry page, actor, shader budget or resource-lifetime owner is introduced.
inline constexpr std::uint32_t kDispatchClockCapacity=8192;
inline constexpr std::size_t kDispatchClockControlOffset=sizeof(horde::world::streaming::GeometryPageTable);
inline constexpr std::size_t kDispatchClockSamplesOffset=kDispatchClockControlOffset+16;
struct alignas(16) RtDispatchClockSample {
    std::array<std::uint32_t,4> tag{}; // recording ticket, x, y, primary instance+1 (0=miss)
    std::array<std::array<std::uint32_t,2>,8> ticks{};
};
static_assert(sizeof(RtDispatchClockSample)==80);
static_assert(kDispatchClockControlOffset%16==0);
inline constexpr std::size_t kDispatchClockAllocationBytes=
    kDispatchClockSamplesOffset+kDispatchClockCapacity*sizeof(RtDispatchClockSample);
inline std::uint32_t DispatchClockAxisSamples(std::uint32_t extent) noexcept {
    return extent>16 ? (extent-17)/32+1 : 0;
}
inline bool SummarizeDispatchClock(std::span<const RtDispatchClockSample> samples,
    std::uint32_t ticket,std::uint32_t width,std::uint32_t height,
    std::array<std::uint64_t,8>& totals) noexcept {
    const auto columns=DispatchClockAxisSamples(width),rows=DispatchClockAxisSamples(height);
    const std::uint64_t count=std::uint64_t(columns)*rows;
    if(!ticket || !count || count>kDispatchClockCapacity || count!=samples.size())return false;
    std::array<std::uint64_t,8> result{};
    for(std::size_t i=0;i<samples.size();++i) {
        const auto& sample=samples[i];
        if(sample.tag[0]!=ticket || sample.tag[1]!=(i%columns)*32+16 ||
           sample.tag[2]!=(i/columns)*32+16)return false;
        std::uint64_t invocation=0;
        for(std::size_t c=0;c<result.size();++c) {
            const auto value=std::uint64_t(sample.ticks[c][0])|(std::uint64_t(sample.ticks[c][1])<<32);
            if(value>std::numeric_limits<std::uint64_t>::max()-result[c] ||
               value>std::numeric_limits<std::uint64_t>::max()-invocation)return false;
            result[c]+=value;invocation+=value;
        }
        if(!invocation)return false; // clock unavailable/stale must not become zero cost
    }
    totals=result;return true;
}
}
