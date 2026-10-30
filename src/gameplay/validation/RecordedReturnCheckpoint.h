#pragma once
#include <cstddef>
#include <span>
#include <string_view>

namespace horde::gameplay::validation {
// Admission-only identity of the real R3 lower-return record. These bytes are
// never loaded or used to grant state. SHA-256: 031ced0a83a136861833ab4e9930ae6ea28c7c6e86d47c61c80ba24e8778472b.
inline bool IsRecordedR3LowerReturn(std::span<const std::byte> encoded) {
    constexpr std::string_view hex=
        "48524443504b5400010000008e00000065aca03301000000020000000100000000000000cdcc06c2333373bf333373c1db0fc9bf00000000020000000300000001010101010100016666e6bf333373c1db0fc9bf0ad7a3bd000080c0333373c100000000000080c0333373c1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001010100010001010101";
    static_assert(hex.size()==324);
    if(encoded.size()!=hex.size()/2)return false;
    const auto digit=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};
    for(std::size_t i=0;i<encoded.size();++i)
        if(std::to_integer<unsigned>(encoded[i])!=(digit(hex[2*i])*16+digit(hex[2*i+1])))return false;
    return true;
}
}
