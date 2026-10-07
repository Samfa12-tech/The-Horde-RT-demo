#pragma once
#include <cstdint>
namespace horde::graphics {
// Optional prototype: existing and fresh settings start at Off.
enum class DustQuality : std::uint8_t { Off = 0, Low = 1, Standard = 2 };
inline constexpr bool ValidDustQuality(DustQuality q) noexcept {
    return static_cast<unsigned>(q) <= 2u;
}
}
