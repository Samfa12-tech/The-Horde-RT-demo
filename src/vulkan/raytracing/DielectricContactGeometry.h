#pragma once

#include "vulkan/raytracing/DielectricMath.h"

#include <bit>
#include <limits>
#include <optional>

namespace horde::vulkan::raytracing
{

// Qualification of an already hardware-intersected pair, NOT ray traversal.
// Unsupported geometry is deliberately uncertified. Plane coincidence alone
// does not establish candidate identity, footprint, visibility or a valid exit.
struct AxisContactPlane
{
    std::uint32_t axis = 0u;
    float coordinate = 0.0f;
    float outwardSign = 1.0f;
};

namespace contact_detail
{

inline float Component(const Vec3& value, std::uint32_t axis)
{
    return axis == 0u ? value.x : (axis == 1u ? value.y : value.z);
}

inline bool EligibleOperand(float value)
{
    // Arithmetic domain, not spatial tolerance. Interpret binary32 bits, so
    // subnormal flushing or floating-point reassociation cannot erase a gap.
    const auto magnitude = std::bit_cast<std::uint32_t>(value) & 0x7fffffffu;
    return magnitude == 0u || (magnitude >= 0x35800000u && magnitude <= 0x49800000u);
}

inline int AxisTriangleWinding(const std::array<Vec3, 3u>& triangle, std::uint32_t axis)
{
    const auto u = (axis + 1u) % 3u;
    const auto v = (axis + 2u) % 3u;
    const auto ordering = [](float a, float b) { return (a > b) - (a < b); };
    // A source edge aligned to either in-plane axis makes the determinant sign
    // provable by coordinate ordering alone. Other triangles are uncertified:
    // no rounded determinant is used as an exact geometry certificate.
    for (std::uint32_t edge = 0u; edge < 3u; ++edge)
    {
        const Vec3& a = triangle[edge];
        const Vec3& b = triangle[(edge + 1u) % 3u];
        const Vec3& c = triangle[(edge + 2u) % 3u];
        if (Component(b, u) == Component(a, u))
            return -ordering(Component(b, v), Component(a, v)) *
                     ordering(Component(c, u), Component(a, u));
        if (Component(b, v) == Component(a, v))
            return ordering(Component(b, u), Component(a, u)) *
                   ordering(Component(c, v), Component(a, v));
    }
    return 0;
}

inline bool ExactAffineCoordinate(float scale, float local, float translation,
                                  float plane)
{
    if (scale == 0.0f || !EligibleOperand(scale) || !EligibleOperand(local) ||
        !EligibleOperand(translation) || !EligibleOperand(plane)) return false;
    static_assert(sizeof(float) == sizeof(std::uint32_t) &&
                  std::numeric_limits<float>::is_iec559 &&
                  std::numeric_limits<float>::digits == 24);
    const auto scaleBits = std::bit_cast<std::uint32_t>(scale);
    const auto localBits = std::bit_cast<std::uint32_t>(local);
    const auto planeBits = std::bit_cast<std::uint32_t>(plane);
    const auto translationBits = std::bit_cast<std::uint32_t>(translation);
    if ((localBits & 0x7fffffffu) == 0u) return translation == plane;

    // Deliberately bounded admission: a power-of-two operand makes the product
    // exact by exponent adjustment alone. No GLSL Fma/rounding-mode guarantee,
    // shaderFloat64/int64 feature, or empirically chosen ray-distance epsilon.
    // Other products remain uncertified, even if their rounded values agree.
    constexpr std::uint32_t fractionMask = 0x007fffffu;
    if ((scaleBits & fractionMask) != 0u && (localBits & fractionMask) != 0u)
        return false;
    const int productExponent = static_cast<int>((scaleBits >> 23u) & 255u) +
        static_cast<int>((localBits >> 23u) & 255u) - 127;
    if (productExponent < 107 || productExponent > 147) return false;
    const auto productSign = (scaleBits ^ localBits) & 0x80000000u;
    const auto productMantissa = 0x00800000u |
        ((scaleBits | localBits) & fractionMask);
    if ((planeBits & 0x7fffffffu) == 0u) return false;
    if ((planeBits & 0x80000000u) != productSign) return false;
    const int planeExponent = static_cast<int>((planeBits >> 23u) & 255u);
    const int commonExponent = std::min(planeExponent, productExponent);
    if (std::abs(planeExponent - productExponent) > 2) return false;

    // Align two 24-bit significands using shifts <=2: each fits in 26 bits.
    // Their unsigned difference is exactly plane-product. Normalise it only
    // if every discarded bit is zero; otherwise no binary32 translation can
    // equal the exact difference. All arithmetic below is uint32 and exact.
    const auto planeMantissa = 0x00800000u | (planeBits & fractionMask);
    const auto planeAligned = planeMantissa << (planeExponent - commonExponent);
    const auto productAligned = productMantissa << (productExponent - commonExponent);
    const bool negativeDifference = planeAligned < productAligned;
    auto difference = negativeDifference ? productAligned - planeAligned :
        planeAligned - productAligned;
    if (difference == 0u) return (translationBits & 0x7fffffffu) == 0u;
    const int highestBit = static_cast<int>(std::bit_width(difference)) - 1;
    if (highestBit > 23)
    {
        const auto shift = static_cast<std::uint32_t>(highestBit - 23);
        if ((difference & ((1u << shift) - 1u)) != 0u) return false;
        difference >>= shift;
    }
    else difference <<= (23 - highestBit);
    const int differenceExponent = commonExponent - 23 + highestBit;
    if (differenceExponent <= 0 || differenceExponent >= 255) return false;
    const auto differenceBits = (productSign ^ (negativeDifference ? 0x80000000u : 0u)) |
        (static_cast<std::uint32_t>(differenceExponent) << 23u) |
        (difference & fractionMask);
    return translationBits == differenceBits;
}

} // namespace contact_detail

inline std::optional<AxisContactPlane> CertifyAxisContactPlane(
    const std::array<Vec3, 3u>& triangle, const Vec3& authoredOutwardNormal)
{
    std::uint32_t axis = 3u;
    float sign = 0.0f;
    for (std::uint32_t component = 0u; component < 3u; ++component)
    {
        const float value = contact_detail::Component(authoredOutwardNormal, component);
        if (value == 0.0f) continue;
        if (axis != 3u || (value != 1.0f && value != -1.0f)) return std::nullopt;
        axis = component;
        sign = value;
    }
    if (axis == 3u) return std::nullopt;
    const float plane = contact_detail::Component(triangle[0], axis);
    for (const Vec3& vertex : triangle)
        for (std::uint32_t component = 0u; component < 3u; ++component)
            if (!contact_detail::EligibleOperand(contact_detail::Component(vertex, component)) ||
                contact_detail::Component(vertex, axis) != plane) return std::nullopt;
    if (contact_detail::AxisTriangleWinding(triangle, axis) != static_cast<int>(sign))
        return std::nullopt;
    return AxisContactPlane{axis, plane, sign};
}

inline bool MatchesAxisContactPlane(const AxisContactPlane& receiver,
    const std::array<Vec3, 3u>& exitTriangle,
    const std::array<float, 4u>& exitTransformRow)
{
    if (receiver.axis >= 3u ||
        (receiver.outwardSign != 1.0f && receiver.outwardSign != -1.0f)) return false;
    std::uint32_t localAxis = 3u;
    for (std::uint32_t component = 0u; component < 3u; ++component)
    {
        if (exitTransformRow[component] == 0.0f) continue;
        if (localAxis != 3u || !contact_detail::EligibleOperand(exitTransformRow[component]))
            return false;
        localAxis = component;
    }
    if (localAxis == 3u) return false;
    const float localPlane = contact_detail::Component(exitTriangle[0], localAxis);
    for (const Vec3& vertex : exitTriangle)
        for (std::uint32_t component = 0u; component < 3u; ++component)
            if (!contact_detail::EligibleOperand(contact_detail::Component(vertex, component)) ||
                contact_detail::Component(vertex, localAxis) != localPlane) return false;
    if (contact_detail::AxisTriangleWinding(exitTriangle, localAxis) == 0) return false;
    return contact_detail::ExactAffineCoordinate(exitTransformRow[localAxis],
        localPlane, exitTransformRow[3], receiver.coordinate);
}

} // namespace horde::vulkan::raytracing
