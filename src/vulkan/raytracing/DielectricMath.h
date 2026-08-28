#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

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

inline Vec3 Multiply(const Vec3& a, const Vec3& b)
{
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}

inline Vec3 Lerp(const Vec3& a, const Vec3& b, float amount)
{
    return Add(Scale(a, 1.0f - amount), Scale(b, amount));
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

inline float EffectiveDielectricFresnel(float incidentCosine,
                                         float incidentIor,
                                         float transmittedIor,
                                         float roughness)
{
    const float smoothFresnel = SchlickFresnel(
        incidentCosine, incidentIor, transmittedIor);
    const float normalFresnel = SchlickFresnel(
        1.0f, incidentIor, transmittedIor);
    roughness = std::clamp(
        dielectric_detail::FiniteOr(roughness, 0.0f), 0.0f, 1.0f);
    const float roughnessBlend = roughness * roughness * 0.25f;
    return std::clamp(
        smoothFresnel * (1.0f - roughnessBlend) +
            normalFresnel * roughnessBlend,
        0.0f, 1.0f);
}

struct DielectricEnergyPartition
{
    float reflection = 0.0f;
    float transmission = 0.0f;
};

inline DielectricEnergyPartition PartitionDielectricEnergy(
    float effectiveFresnel, float transmissionFactor)
{
    const float reflection = std::clamp(
        dielectric_detail::FiniteOr(effectiveFresnel, 0.0f), 0.0f, 1.0f);
    const float materialTransmission = std::clamp(
        dielectric_detail::FiniteOr(transmissionFactor, 0.0f), 0.0f, 1.0f);
    return {reflection, materialTransmission * (1.0f - reflection)};
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
    if (!std::isfinite(discriminant) || discriminant < 0.0f) return false;
    transmittedDirection = dielectric_detail::Normalize(
        dielectric_detail::Add(
            dielectric_detail::Scale(incident, eta),
            dielectric_detail::Scale(normal,
                eta * incidentCosine - std::sqrt(discriminant))),
        Vec3{});
    return dielectric_detail::Dot(transmittedDirection, transmittedDirection) > 0.0f;
}

inline float DielectricRayEpsilon(const Vec3& position, float interfaceDistance)
{
    const float maximumCoordinate = std::max({
        std::abs(dielectric_detail::FiniteOr(position.x, 0.0f)),
        std::abs(dielectric_detail::FiniteOr(position.y, 0.0f)),
        std::abs(dielectric_detail::FiniteOr(position.z, 0.0f)),
        1.0f});
    const float finiteDistance = std::abs(
        dielectric_detail::FiniteOr(interfaceDistance, 0.0f));
    return std::clamp(
        std::max({2.0e-5f, maximumCoordinate * 2.0e-6f,
                  finiteDistance * 1.0e-7f}),
        2.0e-5f, 2.5e-4f);
}

inline Vec3 AdvanceDielectricRayOrigin(const Vec3& position,
                                       const Vec3& geometricNormal,
                                       const Vec3& direction,
                                       float epsilon)
{
    const Vec3 rayDirection = dielectric_detail::Normalize(
        direction, Vec3{0.0f, 0.0f, 1.0f});
    const Vec3 outward = dielectric_detail::Normalize(
        geometricNormal, Vec3{0.0f, 1.0f, 0.0f});
    epsilon = std::clamp(
        dielectric_detail::FiniteOr(epsilon, 2.0e-5f), 2.0e-5f, 2.5e-4f);
    const float side = dielectric_detail::Dot(outward, rayDirection) >= 0.0f
        ? 1.0f : -1.0f;
    return dielectric_detail::Add(position,
        dielectric_detail::Add(
            dielectric_detail::Scale(outward, side * epsilon),
            dielectric_detail::Scale(rayDirection, epsilon)));
}

inline Vec3 OffsetShadowRayOrigin(const Vec3& position,
                                  const Vec3& geometricNormal,
                                  const Vec3& direction,
                                  float interfaceDistance,
                                  bool genericTransmissionActive)
{
    if (genericTransmissionActive)
    {
        return AdvanceDielectricRayOrigin(
            position, geometricNormal, direction,
            DielectricRayEpsilon(position, interfaceDistance));
    }
    const Vec3 outward = dielectric_detail::Normalize(
        geometricNormal, Vec3{0.0f, 1.0f, 0.0f});
    const Vec3 rayDirection = dielectric_detail::Normalize(
        direction, Vec3{0.0f, 0.0f, 1.0f});
    const float side = dielectric_detail::Dot(outward, rayDirection) >= 0.0f
        ? 1.0f : -1.0f;
    return dielectric_detail::Add(
        position, dielectric_detail::Scale(outward, side * 0.004f));
}

inline Vec3 ConstrainDirectionToIdealHemisphere(const Vec3& idealDirection,
                                                const Vec3& candidateDirection,
                                                const Vec3& surfaceNormal)
{
    const Vec3 ideal = dielectric_detail::Normalize(
        idealDirection, Vec3{0.0f, 0.0f, 1.0f});
    Vec3 candidate = dielectric_detail::Normalize(candidateDirection, ideal);
    const Vec3 normal = dielectric_detail::Normalize(
        surfaceNormal, Vec3{0.0f, 1.0f, 0.0f});
    const float side = dielectric_detail::Dot(ideal, normal) >= 0.0f
        ? 1.0f : -1.0f;
    const float signedAlignment = dielectric_detail::Dot(candidate, normal) * side;
    constexpr float kMinimumHemisphereAlignment = 1.0e-4f;
    if (signedAlignment < kMinimumHemisphereAlignment)
    {
        candidate = dielectric_detail::Normalize(
            dielectric_detail::Add(
                candidate,
                dielectric_detail::Scale(
                    normal, side * (kMinimumHemisphereAlignment - signedAlignment))),
            ideal);
    }
    return candidate;
}

inline Vec3 ConstrainClosedVolumeTransmission(const Vec3& idealDirection,
                                              const Vec3& roughDirection,
                                              bool thinWall,
                                              bool entering)
{
    // Rough closed-volume entry must reach the authored paired boundary before
    // applying its directional lobe. Thin walls and physical exits have no
    // unresolved interior segment, so their rough direction remains active.
    return dielectric_detail::Normalize(
        (!thinWall && entering) ? idealDirection : roughDirection,
        dielectric_detail::Normalize(idealDirection, Vec3{0.0f, 0.0f, 1.0f}));
}

inline bool IsDielectricNearSelfHit(float localDistance, float rayEpsilon)
{
    localDistance = std::max(
        0.0f, dielectric_detail::FiniteOr(localDistance, 0.0f));
    rayEpsilon = std::clamp(
        dielectric_detail::FiniteOr(rayEpsilon, 2.0e-5f), 2.0e-5f, 2.5e-4f);
    return localDistance <= rayEpsilon * 8.0f;
}

enum class DielectricBudgetResolution
{
    Overflow,
    AbsorbTrappedTir
};

inline DielectricBudgetResolution ResolveDielectricInterfaceBudget(
    std::size_t volumeDepth, std::size_t tirSinceLastTransition)
{
    // Energy left in a volume after repeated TIR has not escaped or overflowed
    // a stack. At the fixed path budget it is conservatively absorbed; only a
    // non-TIR open path is an actual transport overflow.
    return volumeDepth > 0u && tirSinceLastTransition > 0u
        ? DielectricBudgetResolution::AbsorbTrappedTir
        : DielectricBudgetResolution::Overflow;
}

enum class DielectricTerminalKind
{
    ContinueGeneric,
    Water,
    Opaque,
    Miss
};

enum class DielectricTerminalResolution
{
    ContinueGeneric,
    ShadeTerminal,
    ConservativeCertifiedAbsorption,
    FailUnclosedVolume
};

inline DielectricTerminalKind ClassifyDielectricTerminal(
    bool hit, bool genericDielectric, bool water)
{
    if (!hit) return DielectricTerminalKind::Miss;
    if (water) return DielectricTerminalKind::Water;
    if (genericDielectric) return DielectricTerminalKind::ContinueGeneric;
    return DielectricTerminalKind::Opaque;
}

inline DielectricTerminalResolution ResolveDielectricTerminal(
    std::size_t closedVolumeDepth, DielectricTerminalKind terminal,
    bool everyOpenVolumeCertified = false)
{
    if (terminal == DielectricTerminalKind::ContinueGeneric)
        return DielectricTerminalResolution::ContinueGeneric;
    // A loader-issued certificate proves every thick component is an outward,
    // closed manifold. If a finite-precision grazing ray nevertheless reaches
    // an ordinary terminal with only certified media open, absorb the remaining
    // energy. This never shades through the cage and is unavailable to open,
    // inward, thin-wall, or otherwise uncertified geometry.
    if (closedVolumeDepth > 0u && everyOpenVolumeCertified)
        return DielectricTerminalResolution::ConservativeCertifiedAbsorption;
    return closedVolumeDepth > 0u
        ? DielectricTerminalResolution::FailUnclosedVolume
        : DielectricTerminalResolution::ShadeTerminal;
}

struct ShadowInterfaceSample
{
    float distance = 0.0f;
    std::uint32_t stableId = 0u;
    std::uint32_t materialId = 0u;
    bool entering = true;
    bool thinWall = false;
    float transmission = 0.0f;
    float metallic = 0.0f;
    Vec3 attenuationColor{1.0f, 1.0f, 1.0f};
    float attenuationDistance = 0.0f;
    std::uint32_t instanceId = 0u;
};

struct BoundedShadowResult
{
    Vec3 transmittance{1.0f, 1.0f, 1.0f};
    std::size_t interfaceCount = 0u;
    std::size_t implicitOriginExitCount = 0u;
    bool blocked = false;
    bool overflow = false;
    bool unclosedVolume = false;
};

inline Vec3 BeerLambert(const Vec3& attenuationColor,
                        float pathLength,
                        float attenuationDistance);

template<std::size_t MaxInterfaces, std::size_t MaxVolumes>
BoundedShadowResult EvaluateBoundedShadow(
    std::span<const ShadowInterfaceSample> candidates, float terminalDistance)
{
    static_assert(MaxInterfaces > 0u && MaxVolumes > 0u);
    constexpr std::size_t kCandidateCapacity = 32u;
    BoundedShadowResult result;
    if (candidates.size() > kCandidateCapacity)
    {
        result.overflow = true;
        result.transmittance = {0.08f, 0.08f, 0.08f};
        return result;
    }

    terminalDistance = std::max(
        0.0f, dielectric_detail::FiniteOr(terminalDistance, 0.0f));
    std::array<bool, kCandidateCapacity> consumed{};
    std::array<std::uint32_t, MaxVolumes> volumeInstances{};
    std::array<std::uint32_t, MaxVolumes> volumeMaterials{};
    std::array<float, MaxVolumes> volumeEntryDistances{};
    std::array<Vec3, MaxVolumes> volumeAttenuationColors{};
    std::array<float, MaxVolumes> volumeAttenuationDistances{};
    std::size_t volumeDepth = 0u;
    bool observedClosedVolumeEntry = false;

    for (std::size_t traversal = 0u; traversal < candidates.size(); ++traversal)
    {
        std::size_t nearest = candidates.size();
        for (std::size_t index = 0u; index < candidates.size(); ++index)
        {
            if (consumed[index]) continue;
            const ShadowInterfaceSample& candidate = candidates[index];
            if (!std::isfinite(candidate.distance) || candidate.distance < 0.0f ||
                candidate.distance > terminalDistance)
            {
                consumed[index] = true;
                continue;
            }
            if (nearest == candidates.size() ||
                candidate.distance < candidates[nearest].distance ||
                (candidate.distance == candidates[nearest].distance &&
                 candidate.stableId < candidates[nearest].stableId))
            {
                nearest = index;
            }
        }
        if (nearest == candidates.size()) break;
        consumed[nearest] = true;
        const ShadowInterfaceSample& sample = candidates[nearest];
        const float transmission = std::clamp(
            dielectric_detail::FiniteOr(sample.transmission, 0.0f), 0.0f, 1.0f);
        const float metallic = std::clamp(
            dielectric_detail::FiniteOr(sample.metallic, 0.0f), 0.0f, 1.0f);
        if (transmission <= 0.001f || metallic > 0.5f)
        {
            result.blocked = true;
            result.transmittance = {};
            return result;
        }
        if (result.interfaceCount >= MaxInterfaces)
        {
            result.overflow = true;
            result.transmittance = dielectric_detail::Scale(result.transmittance, 0.08f);
            return result;
        }
        ++result.interfaceCount;
        if (sample.thinWall)
        {
            const Vec3 tint = dielectric_detail::Lerp(
                Vec3{1.0f, 1.0f, 1.0f},
                Vec3{
                    std::clamp(dielectric_detail::FiniteOr(sample.attenuationColor.x, 1.0f), 0.0f, 1.0f),
                    std::clamp(dielectric_detail::FiniteOr(sample.attenuationColor.y, 1.0f), 0.0f, 1.0f),
                    std::clamp(dielectric_detail::FiniteOr(sample.attenuationColor.z, 1.0f), 0.0f, 1.0f)},
                0.12f);
            result.transmittance = dielectric_detail::Multiply(
                result.transmittance, dielectric_detail::Scale(tint, transmission));
            continue;
        }
        if (sample.entering)
        {
            if (volumeDepth >= MaxVolumes)
            {
                result.overflow = true;
                result.transmittance = dielectric_detail::Scale(result.transmittance, 0.08f);
                return result;
            }
            observedClosedVolumeEntry = true;
            volumeInstances[volumeDepth] = sample.instanceId;
            volumeMaterials[volumeDepth] = sample.materialId;
            volumeEntryDistances[volumeDepth] = sample.distance;
            volumeAttenuationColors[volumeDepth] = sample.attenuationColor;
            volumeAttenuationDistances[volumeDepth] = sample.attenuationDistance;
            ++volumeDepth;
            result.transmittance = dielectric_detail::Scale(result.transmittance, transmission);
        }
        else
        {
            if (volumeDepth == 0u && !observedClosedVolumeEntry)
            {
                // A finite shadow ray can be born inside one or more closed
                // media. Consecutive first exits are therefore complete
                // origin-to-boundary segments, not corrupt stack pops.
                const Vec3 absorption = BeerLambert(
                    sample.attenuationColor, sample.distance,
                    sample.attenuationDistance);
                result.transmittance = dielectric_detail::Multiply(
                    result.transmittance,
                    dielectric_detail::Scale(absorption, transmission));
                ++result.implicitOriginExitCount;
                continue;
            }
            if (volumeDepth == 0u ||
                volumeInstances[volumeDepth - 1u] != sample.instanceId ||
                volumeMaterials[volumeDepth - 1u] != sample.materialId)
            {
                result.unclosedVolume = true;
                result.transmittance = dielectric_detail::Scale(result.transmittance, 0.08f);
                return result;
            }
            --volumeDepth;
            const Vec3 absorption = BeerLambert(
                volumeAttenuationColors[volumeDepth],
                std::max(sample.distance - volumeEntryDistances[volumeDepth], 0.0f),
                volumeAttenuationDistances[volumeDepth]);
            result.transmittance = dielectric_detail::Multiply(
                result.transmittance, absorption);
        }
    }
    if (volumeDepth != 0u)
    {
        // terminalDistance is a light-segment endpoint, not an unbounded ray.
        // Reaching it with open media means the light is inside those media;
        // apply their partial physical path lengths rather than inventing an
        // absent exit interface or treating the segment as malformed.
        for (std::size_t index = 0u; index < volumeDepth; ++index)
        {
            result.transmittance = dielectric_detail::Multiply(
                result.transmittance,
                BeerLambert(volumeAttenuationColors[index],
                    std::max(terminalDistance - volumeEntryDistances[index], 0.0f),
                    volumeAttenuationDistances[index]));
        }
    }
    return result;
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
        return Enter(0u, materialId, materialIor);
    }

    InterfaceTransition Enter(std::uint32_t instanceId,
                              std::uint32_t materialId,
                              float materialIor)
    {
        const float incident = CurrentIor();
        const float transmitted = dielectric_detail::SanitizeIor(materialIor);
        if (depth_ >= MaxVolumes)
            return {incident, transmitted, false, true};
        entries_[depth_++] = {instanceId, materialId, transmitted};
        return {incident, transmitted, true, false};
    }

    InterfaceTransition Exit(std::uint32_t materialId)
    {
        return Exit(0u, materialId);
    }

    InterfaceTransition Exit(std::uint32_t instanceId,
                             std::uint32_t materialId)
    {
        const float incident = CurrentIor();
        if (depth_ == 0u ||
            entries_[depth_ - 1u].instanceId != instanceId ||
            entries_[depth_ - 1u].materialId != materialId)
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
        std::uint32_t instanceId = 0u;
        std::uint32_t materialId = 0u;
        float ior = 1.0f;
    };
    std::array<Entry, MaxVolumes> entries_{};
    std::size_t depth_ = 0u;
};

} // namespace horde::vulkan::raytracing
