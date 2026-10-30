#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace horde::platform::windows {
// Frozen after independent native inspection; never derived/accepted in the
// same run. Debug capture-only source candidate; normal84219 admission remains.
inline constexpr std::string_view kForestContentCandidateName="pine-branch-r4";
inline constexpr std::string_view kForestContentCandidateFingerprint="96247d7f072d460d0b80559ab82cfcf38333f9ae95e3b23f5765fabc336be903";
inline constexpr std::uint64_t kForestContentCandidateCount=83553u;
inline constexpr std::array<std::uint64_t,6> kForestContentCandidateBounds{{0xc0372c38a0000000ull, 0xbff299c060000000ull, 0x4038000c40000000ull, 0x4033f8ed60000000ull, 0x4025491720000000ull, 0x404615c880000000ull}};
inline bool IsKnownForestContentCandidate(std::string_view name) {
    return name==kForestContentCandidateName;
}
inline bool MatchesFrozenForestContentCandidate(std::string_view name,
    std::string_view fingerprint,std::uint64_t count,std::span<const std::uint64_t,6> bounds,bool structurallyValid) {
    if(!structurallyValid || !IsKnownForestContentCandidate(name) ||
       fingerprint!=kForestContentCandidateFingerprint || count!=kForestContentCandidateCount)return false;
    for(std::size_t i=0;i<bounds.size();++i)if(bounds[i]!=kForestContentCandidateBounds[i])return false;
    return true;
}
}
