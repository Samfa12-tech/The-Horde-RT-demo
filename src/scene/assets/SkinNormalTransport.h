#pragma once

#include <array>
#include <cmath>

namespace horde::scene::detail
{

// CPU-only inverse-transpose factors. Keep cofactors unscaled: applying the
// reciprocal after the dot product preserves the admitted arithmetic order.
struct SkinNormalTransport
{
    std::array<float, 3u> cofactor0{}, cofactor1{}, cofactor2{};
    float inverseDeterminant = 0.0f;
    bool invertible = false;
};

inline SkinNormalTransport MakeSkinNormalTransport(const std::array<float, 16u>& m)
{
    const std::array<float, 3u> c0{{m[0], m[1], m[2]}};
    const std::array<float, 3u> c1{{m[4], m[5], m[6]}};
    const std::array<float, 3u> c2{{m[8], m[9], m[10]}};
    SkinNormalTransport result{};
    result.cofactor0 = {{c1[1]*c2[2]-c1[2]*c2[1], c1[2]*c2[0]-c1[0]*c2[2], c1[0]*c2[1]-c1[1]*c2[0]}};
    result.cofactor1 = {{c2[1]*c0[2]-c2[2]*c0[1], c2[2]*c0[0]-c2[0]*c0[2], c2[0]*c0[1]-c2[1]*c0[0]}};
    result.cofactor2 = {{c0[1]*c1[2]-c0[2]*c1[1], c0[2]*c1[0]-c0[0]*c1[2], c0[0]*c1[1]-c0[1]*c1[0]}};
    const float determinant = c0[0]*result.cofactor0[0] +
        c0[1]*result.cofactor0[1] + c0[2]*result.cofactor0[2];
    result.invertible = std::isfinite(determinant) && std::abs(determinant) > 1.0e-20f;
    if (result.invertible) result.inverseDeterminant = 1.0f / determinant;
    return result;
}

inline bool ApplySkinNormalTransport(const SkinNormalTransport& transform,
    const float x, const float y, const float z, float& outX, float& outY, float& outZ)
{
    if (!transform.invertible) return false;
    outX = (transform.cofactor0[0]*x + transform.cofactor1[0]*y + transform.cofactor2[0]*z)*transform.inverseDeterminant;
    outY = (transform.cofactor0[1]*x + transform.cofactor1[1]*y + transform.cofactor2[1]*z)*transform.inverseDeterminant;
    outZ = (transform.cofactor0[2]*x + transform.cofactor1[2]*y + transform.cofactor2[2]*z)*transform.inverseDeterminant;
    return std::isfinite(outX) && std::isfinite(outY) && std::isfinite(outZ);
}

} // namespace horde::scene::detail
