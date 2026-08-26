#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

namespace horde::scene::assets::detail
{

inline constexpr double kDielectricMinimumWeldToleranceMetres = 1.0e-7;
inline constexpr double kDielectricMaximumWeldToleranceMetres = 1.0e-5;
inline constexpr double kExclusiveWeldCellCoordinateLimit = 0x1p63;

inline bool DielectricWeldCellCoordinate(double scaled,
                                         std::int64_t& coordinate)
{
    if (!std::isfinite(scaled) ||
        !(scaled > -kExclusiveWeldCellCoordinateLimit &&
          scaled < kExclusiveWeldCellCoordinateLimit))
    {
        return false;
    }
    coordinate = static_cast<std::int64_t>(std::floor(scaled));
    return true;
}

inline bool BakedCoordinateInDielectricWeldDomain(float coordinate)
{
    const double scaled = static_cast<double>(coordinate) /
                          kDielectricMinimumWeldToleranceMetres;
    std::int64_t ignored = 0;
    return DielectricWeldCellCoordinate(scaled, ignored);
}

} // namespace horde::scene::assets::detail
