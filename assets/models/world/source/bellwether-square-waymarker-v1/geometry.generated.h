#pragma once
#include <array>
#include <cstddef>

namespace horde::gameplay::bellwether {
inline constexpr std::size_t kSquareWaymarkerSolidTriangles=1972;
inline constexpr std::size_t kSquareWaymarkerSignTriangles=752;
inline constexpr std::size_t kSquareWaymarkerCaskTriangles=1100;
inline constexpr std::size_t kSquareWaymarkerLampTriangles=120;
inline constexpr std::array<float,3> kSquareWaymarkerPost{{80.200000f, -11.708496f, 129.200000f}};
inline constexpr std::array<float,2> kSquareWaymarkerView{{77.400000f, 131.500000f}};
inline constexpr std::array<float,3> kSquareWaymarkerFlame{{79.643631f, -8.828496f, 129.657013f}};
inline constexpr std::array<float,3> kSquareWaymarkerLight{{79.643631f, -8.688496f, 129.657013f}};
inline constexpr std::array<float,3> kSquareWaymarkerTavern{{80.200000f, -9.228496f, 129.200000f}};
inline constexpr std::array<float,3> kSquareWaymarkerDealer{{80.200000f, -9.488496f, 129.200000f}};
inline constexpr std::array<float,3> kSquareWaymarkerMill{{80.200000f, -8.768496f, 129.200000f}};
inline constexpr std::array<float,3> kSquareWaymarkerSmith{{80.200000f, -9.768496f, 129.200000f}};
} // namespace horde::gameplay::bellwether
