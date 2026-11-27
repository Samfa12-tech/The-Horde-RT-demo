#pragma once

#include <cstdint>

namespace horde::gameplay::bellwether {

enum class PrivyRagState : std::uint8_t { Available, Carried, Delivered };
enum class PrivyRagAction : std::uint8_t { None, Take, Pass };

} // namespace horde::gameplay::bellwether
