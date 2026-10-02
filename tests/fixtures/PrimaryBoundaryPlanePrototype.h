#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

// Investigation-only reference. This is NOT a candidate-rejection policy:
// even a certified outside origin can produce a valid edge/corner backface.
// Keep it out of renderer includes unless a separate boundary proof is made.
namespace horde::vulkan::raytracing::investigation
{

// Bounds of immutable, actually uploaded binary32 positions, not authored
// accessor bounds. Closed/outward material certification is a separate gate.
struct RtPrimaryBoundaryBounds
{
    std::array<float, 3u> minimum{};
    std::array<float, 3u> maximum{};
    bool valid = false;
};

struct RtPrimaryBoundaryPlane
{
    float coordinate = 0.0f;
    std::uint32_t axisPlusOne = 0u;
    bool negativeHalfSpace = false;
};

inline constexpr float kPrimaryBoundaryMaxQueryMinimum = 0.002f;
inline constexpr float kPrimaryBoundaryDirectionAxisLimit = 2.0f;

namespace primary_boundary_detail
{
struct Interval { double low; double high; };
inline double Down(double value)
{
    return std::nextafter(value, -std::numeric_limits<double>::infinity());
}
inline double Up(double value)
{
    return std::nextafter(value, std::numeric_limits<double>::infinity());
}
inline Interval Add(Interval a, Interval b)
{
    return {Down(a.low + b.low), Up(a.high + b.high)};
}
inline Interval Subtract(Interval a, Interval b)
{
    return {Down(a.low - b.high), Up(a.high - b.low)};
}
inline Interval Multiply(Interval a, Interval b)
{
    const std::array<double, 4u> products{{
        a.low * b.low, a.low * b.high, a.high * b.low, a.high * b.high}};
    const auto [low, high] = std::minmax_element(products.begin(), products.end());
    return {Down(*low), Up(*high)};
}
inline float OutwardFloat(double value, bool lower)
{
    float result = static_cast<float>(value);
    if ((lower && double(result) > value) || (!lower && double(result) < value))
        result = std::nextafter(result, lower ? -std::numeric_limits<float>::infinity()
                                             : std::numeric_limits<float>::infinity());
    return result;
}
} // namespace primary_boundary_detail

// Positive determinant must be proved, not guessed with a determinant epsilon.
// Unknown/singular/reflected transforms fall back to the unmodified native path.
// Vulkan's row-major 3x4 instance transform consumes exactly these float inputs.
inline RtPrimaryBoundaryPlane BuildPrimaryBoundaryPlane(
    const RtPrimaryBoundaryBounds& bounds, const std::array<float, 12u>& transform,
    const std::array<float, 3u>& nominalCamera)
{
    using namespace primary_boundary_detail;
    static_assert(std::numeric_limits<float>::is_iec559 &&
                  std::numeric_limits<double>::is_iec559);
    if (!bounds.valid) return {};
    for (std::size_t axis = 0u; axis < 3u; ++axis)
        if (!std::isfinite(bounds.minimum[axis]) || !std::isfinite(bounds.maximum[axis]) ||
            bounds.minimum[axis] > bounds.maximum[axis] || !std::isfinite(nominalCamera[axis]))
            return {};
    for (float value : transform)
        if (!std::isfinite(value)) return {};
    const auto coefficient = [&](std::size_t index) -> Interval {
        return {double(transform[index]), double(transform[index])};
    };
    const Interval determinant = Add(Subtract(
        Multiply(coefficient(0u), Subtract(Multiply(coefficient(5u), coefficient(10u)),
                                          Multiply(coefficient(6u), coefficient(9u)))),
        Multiply(coefficient(1u), Subtract(Multiply(coefficient(4u), coefficient(10u)),
                                          Multiply(coefficient(6u), coefficient(8u))))),
        Multiply(coefficient(2u), Subtract(Multiply(coefficient(4u), coefficient(9u)),
                                          Multiply(coefficient(5u), coefficient(8u)))));
    if (!(determinant.low > 0.0) || !std::isfinite(determinant.high)) return {};

    std::array<Interval, 3u> world{};
    for (std::size_t axis = 0u; axis < 3u; ++axis)
    {
        world[axis] = coefficient(axis * 4u + 3u);
        for (std::size_t column = 0u; column < 3u; ++column)
            world[axis] = Add(world[axis], Multiply(coefficient(axis * 4u + column),
                {double(bounds.minimum[column]), double(bounds.maximum[column])}));
    }
    // If |direction[axis]| <=2 and 0<=tmin<=the unchanged primary minimum,
    // the whole excluded segment varies on this axis by at most this amount.
    // This expands a qualification bound, not any surface, ray or optical value.
    const double margin = double(kPrimaryBoundaryDirectionAxisLimit) *
                          double(kPrimaryBoundaryMaxQueryMinimum);
    std::size_t selectedAxis = 0u;
    bool selectedNegative = false;
    double selectedCoordinate = 0.0;
    double largestNominalGap = -std::numeric_limits<double>::infinity();
    for (std::size_t axis = 0u; axis < 3u; ++axis)
    {
        const double lower = Down(world[axis].low - margin);
        const double upper = Up(world[axis].high + margin);
        const double negativeGap = lower - double(nominalCamera[axis]);
        const double positiveGap = double(nominalCamera[axis]) - upper;
        const bool negative = negativeGap > positiveGap;
        const double gap = negative ? negativeGap : positiveGap;
        if (gap > largestNominalGap)
        {
            largestNominalGap = gap;
            selectedAxis = axis;
            selectedNegative = negative;
            selectedCoordinate = negative ? lower : upper;
        }
    }
    // Check before narrowing: an out-of-range double-to-float conversion is
    // not a portable certificate, even on hosts that usually return infinity.
    if (!std::isfinite(selectedCoordinate) ||
        selectedCoordinate < -double(std::numeric_limits<float>::max()) ||
        selectedCoordinate > double(std::numeric_limits<float>::max())) return {};
    const float coordinate = OutwardFloat(selectedCoordinate, selectedNegative);
    if (!std::isfinite(coordinate)) return {};
    // Camera is only an efficiency hint for axis choice. No assertion about
    // its actual bobbed location is serialized: the shader must check origin.
    return {coordinate, static_cast<std::uint32_t>(selectedAxis + 1u), selectedNegative};
}

// CPU reference for near-segment exclusion only. It does not certify that a
// native backface is spurious, even with closed/outward material certification.
inline bool PrimaryBoundaryNearSegmentOutside(
    const RtPrimaryBoundaryPlane& plane, const std::array<float, 3u>& origin,
    const std::array<float, 3u>& direction, float minimumDistance)
{
    if (plane.axisPlusOne == 0u || plane.axisPlusOne > 3u || !std::isfinite(plane.coordinate) ||
        !std::isfinite(minimumDistance) || minimumDistance < 0.0f ||
        minimumDistance > kPrimaryBoundaryMaxQueryMinimum) return false;
    for (std::size_t axis = 0u; axis < 3u; ++axis)
        if (!std::isfinite(origin[axis]) || !std::isfinite(direction[axis])) return false;
    const std::size_t axis = plane.axisPlusOne - 1u;
    if (std::abs(direction[axis]) > kPrimaryBoundaryDirectionAxisLimit) return false;
    return plane.negativeHalfSpace ? origin[axis] < plane.coordinate
                                   : origin[axis] > plane.coordinate;
}

} // namespace horde::vulkan::raytracing::investigation
