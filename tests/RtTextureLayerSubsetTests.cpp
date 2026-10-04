#include "vulkan/raytracing/RtTextureLayerSubset.h"

#include <array>
#include <iostream>
#include <limits>

int main()
{
    using horde::vulkan::raytracing::AppendRtTextureLayerSubset;
    bool passed = true;
    const auto check = [&passed](bool condition, const char* label) {
        if (!condition) { std::cerr << label << "\n"; passed = false; }
    };
    const std::array<std::uint8_t, 12> source{10,11,12,13,20,21,22,23,30,31,32,33};
    const std::array<std::uint32_t, 3> reorder{2,0,2};
    std::vector<std::uint8_t> destination{99};
    check(AppendRtTextureLayerSubset(source, 3, 4, reorder, destination), "valid compressed layer selection");
    check(destination == std::vector<std::uint8_t>({99,30,31,32,33,10,11,12,13,30,31,32,33}), "byte exact reordered/repeated layers");
    const std::array<std::uint8_t, 3> nextMip{1,2,3};
    check(AppendRtTextureLayerSubset(nextMip, 3, 1, reorder, destination), "append subsequent mip");
    check(destination[destination.size()-3] == 3 && destination[destination.size()-2] == 1 && destination.back() == 3, "mip layer order retained");
    const auto saved = destination;
    const std::array<std::uint32_t, 2> badSelection{0,3};
    check(!AppendRtTextureLayerSubset(source, 3, 4, badSelection, destination), "reject invalid later layer before append");
    check(destination == saved, "invalid layer leaves destination unchanged");
    check(!AppendRtTextureLayerSubset(std::span<const std::uint8_t>(source).first(11), 3, 4, reorder, destination), "reject truncated full source");
    check(!AppendRtTextureLayerSubset(source, 3, 3, reorder, destination), "reject oversized full source");
    check(!AppendRtTextureLayerSubset(source, 0, 4, reorder, destination), "reject zero source layers");
    check(!AppendRtTextureLayerSubset(source, 3, 0, reorder, destination), "reject zero layer size");
    check(!AppendRtTextureLayerSubset(source, 3, 4, {}, destination), "reject empty admission");
    check(!AppendRtTextureLayerSubset(source, 3, std::numeric_limits<std::uint64_t>::max(), reorder, destination), "reject host size multiplication overflow");
    check(destination == saved, "all malformed payloads retain destination");
    return passed ? 0 : 1;
}
