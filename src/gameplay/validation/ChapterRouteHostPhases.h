#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>

namespace horde::gameplay::validation {
// Harness-only partition of a successful-present interval. Not a GPU timer or
// attribution to an OS function. Previous evidence is a subset of previous tail.
struct ChapterRouteHostPhases {
    std::array<std::uint64_t,6> nanoseconds{};
    std::uint64_t previousEvidenceNanoseconds=0;
    bool available=false;
};
inline constexpr std::array<const char*,6> kChapterHostPhaseNames{{
    "host_previous_tail_ns","host_system_load_ns","host_messages_ns",
    "host_visibility_ns","host_clock_simulation_ns","host_render_to_present_ns"}};
inline std::optional<ChapterRouteHostPhases> MeasureChapterRouteHostPhases(
    const std::array<std::uint64_t,7>& marks,std::uint64_t previousEvidence,bool available) noexcept {
    if(!available)return ChapterRouteHostPhases{};
    if(!std::is_sorted(marks.begin(),marks.end()) || previousEvidence>marks[1]-marks[0])return std::nullopt;
    ChapterRouteHostPhases result;result.available=true;result.previousEvidenceNanoseconds=previousEvidence;
    for(std::size_t i=0;i<result.nanoseconds.size();++i)result.nanoseconds[i]=marks[i+1]-marks[i];
    return result;
}
}
