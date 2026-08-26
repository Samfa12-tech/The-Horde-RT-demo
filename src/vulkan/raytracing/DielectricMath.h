#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace horde::vulkan::raytracing
{

struct Vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

namespace dielectric_detail
{

inline float FiniteOr(float value, float fallback)
{
    return std::isfinite(value) ? value : fallback;
}

inline float SanitizeIor(float ior)
{
    ior = FiniteOr(ior, 1.5f);
    return ior >= 1.0f && ior <= 4.0f ? ior : 1.5f;
}

inline float Dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 Scale(const Vec3& value, float scale)
{
    return {value.x * scale, value.y * scale, value.z * scale};
}

inline Vec3 Add(const Vec3& a, const Vec3& b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vec3 Normalize(const Vec3& value, const Vec3& fallback)
{
    const Vec3 finite{
        FiniteOr(value.x, 0.0f),
        FiniteOr(value.y, 0.0f),
        FiniteOr(value.z, 0.0f)};
    const float lengthSquared = Dot(finite, finite);
    if (!std::isfinite(lengthSquared) || lengthSquared <= 1.0e-12f) return fallback;
    return Scale(finite, 1.0f / std::sqrt(lengthSquared));
}

} // namespace dielectric_detail

inline float SchlickFresnel(float incidentCosine, float incidentIor, float transmittedIor)
{
    const float etaI = dielectric_detail::SanitizeIor(incidentIor);
    const float etaT = dielectric_detail::SanitizeIor(transmittedIor);
    const float denominator = std::max(etaI + etaT, 1.0e-6f);
    const float ratio = (etaI - etaT) / denominator;
    const float r0 = ratio * ratio;
    const float cosine = std::clamp(
        dielectric_detail::FiniteOr(incidentCosine, 0.0f), 0.0f, 1.0f);
    const float oneMinus = 1.0f - cosine;
    return std::clamp(r0 + (1.0f - r0) * oneMinus * oneMinus * oneMinus *
                         oneMinus * oneMinus,
                      0.0f, 1.0f);
}

struct OrientedInterface
{
    Vec3 normal{};
    bool entering = true;
};

inline OrientedInterface OrientInterface(const Vec3& incidentDirection,
                                          const Vec3& outwardNormal)
{
    const Vec3 incident = dielectric_detail::Normalize(
        incidentDirection, Vec3{0.0f, -1.0f, 0.0f});
    const Vec3 outward = dielectric_detail::Normalize(
        outwardNormal, Vec3{0.0f, 1.0f, 0.0f});
    const bool entering = dielectric_detail::Dot(incident, outward) < 0.0f;
    return {entering ? outward : dielectric_detail::Scale(outward, -1.0f), entering};
}

inline bool RefractDirection(const Vec3& incidentDirection,
                             const Vec3& orientedNormal,
                             float incidentIor,
                             float transmittedIor,
                             Vec3& transmittedDirection)
{
    transmittedDirection = {};
    const Vec3 incident = dielectric_detail::Normalize(
        incidentDirection, Vec3{0.0f, -1.0f, 0.0f});
    const Vec3 normal = dielectric_detail::Normalize(
        orientedNormal, Vec3{0.0f, 1.0f, 0.0f});
    const float eta = dielectric_detail::SanitizeIor(incidentIor) /
                      dielectric_detail::SanitizeIor(transmittedIor);
    const float incidentCosine = std::clamp(
        -dielectric_detail::Dot(incident, normal), 0.0f, 1.0f);
    const float discriminant = 1.0f - eta * eta *
        std::max(0.0f, 1.0f - incidentCosine * incidentCosine);
    if (!std::isfinite(discriminant) || discriminant <= 0.0f) return false;
    transmittedDirection = dielectric_detail::Normalize(
        dielectric_detail::Add(
            dielectric_detail::Scale(incident, eta),
            dielectric_detail::Scale(normal,
                eta * incidentCosine - std::sqrt(discriminant))),
        Vec3{});
    return dielectric_detail::Dot(transmittedDirection, transmittedDirection) > 0.0f;
}

inline Vec3 BeerLambert(const Vec3& attenuationColor,
                        float pathLength,
                        float attenuationDistance)
{
    pathLength = std::max(0.0f, dielectric_detail::FiniteOr(pathLength, 0.0f));
    if (!std::isfinite(attenuationDistance)) return {1.0f, 1.0f, 1.0f};
    attenuationDistance = dielectric_detail::FiniteOr(attenuationDistance, 0.0f);
    if (attenuationDistance <= 1.0e-6f || pathLength <= 0.0f)
        return {1.0f, 1.0f, 1.0f};
    const float exponent = std::min(pathLength / attenuationDistance, 1.0e6f);
    const auto channel = [exponent](float value) {
        value = std::clamp(dielectric_detail::FiniteOr(value, 1.0f), 0.0f, 1.0f);
        return std::isfinite(value) ? std::pow(value, exponent) : 1.0f;
    };
    return {channel(attenuationColor.x), channel(attenuationColor.y),
            channel(attenuationColor.z)};
}

struct InterfaceTransition
{
    float incidentIor = 1.0f;
    float transmittedIor = 1.0f;
    bool accepted = false;
    bool overflow = false;
};

struct ThinWallTransition
{
    float outsideIor = 1.0f;
    float wallIor = 1.5f;
};

template<std::size_t MaxVolumes>
class DielectricStack
{
public:
    static_assert(MaxVolumes > 0u);

    std::size_t Depth() const { return depth_; }

    float CurrentIor() const
    {
        return depth_ == 0u ? 1.0f : entries_[depth_ - 1u].ior;
    }

    InterfaceTransition Enter(std::uint32_t materialId, float materialIor)
    {
        const float incident = CurrentIor();
        const float transmitted = dielectric_detail::SanitizeIor(materialIor);
        if (depth_ >= MaxVolumes)
            return {incident, transmitted, false, true};
        entries_[depth_++] = {materialId, transmitted};
        return {incident, transmitted, true, false};
    }

    InterfaceTransition Exit(std::uint32_t materialId)
    {
        const float incident = CurrentIor();
        if (depth_ == 0u || entries_[depth_ - 1u].materialId != materialId)
            return {incident, incident, false, false};
        --depth_;
        return {incident, CurrentIor(), true, false};
    }

    ThinWallTransition ThinWall(float wallIor) const
    {
        return {CurrentIor(), dielectric_detail::SanitizeIor(wallIor)};
    }

private:
    struct Entry
    {
        std::uint32_t materialId = 0u;
        float ior = 1.0f;
    };
    std::array<Entry, MaxVolumes> entries_{};
    std::size_t depth_ = 0u;
};

} // namespace horde::vulkan::raytracing
