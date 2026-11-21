#pragma once

#include <array>
#include <cmath>
#include <optional>

namespace horde::vulkan::raytracing
{

inline constexpr std::array<float, 3u> kBellwetherWaterwheelSurveyCentre{{
    137.2f, -21.10f, 109.35f}};

// VkTransformMatrix3x4 uses row-major 3x4 values. The source GLB stays in
// pivot-local space; this instance transform places its local origin at the
// surveyed axle and applies a rigid X-axis rotation there.
inline std::optional<std::array<float, 12u>> BellwetherWaterwheelWorldFromModel(
    const float angleRadians) noexcept
{
    if (!std::isfinite(angleRadians)) return std::nullopt;
    constexpr double twoPi = 6.283185307179586476925286766559;
    const double angle = std::remainder(static_cast<double>(angleRadians), twoPi);
    const float cosine = static_cast<float>(std::cos(angle));
    const float sine = static_cast<float>(std::sin(angle));
    return std::array<float, 12u>{{
        1.f, 0.f, 0.f, kBellwetherWaterwheelSurveyCentre[0],
        0.f, cosine, -sine, kBellwetherWaterwheelSurveyCentre[1],
        0.f, sine, cosine, kBellwetherWaterwheelSurveyCentre[2]}};
}

} // namespace horde::vulkan::raytracing
