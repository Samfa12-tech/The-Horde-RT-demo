#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>

namespace horde::gameplay
{

using SwordContactPoint = std::array<float, 3u>;

// Experimental, asset-independent diagnostic shape. It is not enabled by the
// live simulation. Admission would require measured authored Grip/blade and
// target-pose data plus the caller's wall/occlusion and attack-identity rules.
struct SwordContactCapsule
{
    SwordContactPoint start{};
    SwordContactPoint end{};
    float radiusMetres = 0.0f;
};

struct SwordContactTargetPair
{
    std::int32_t targetId = -1;
    SwordContactCapsule previous{};
    SwordContactCapsule current{};
    bool eligible = false;
};

enum class SwordContactSample : std::uint8_t
{
    None,
    Previous,
    Current,
};

struct SwordContactProxyResult
{
    std::int32_t targetId = -1;
    float minimumSeparationMetres = std::numeric_limits<float>::infinity();
    SwordContactSample sample = SwordContactSample::None;
    bool touching = false;
};

namespace detail
{

inline float SwordContactDot(const SwordContactPoint& left,
                             const SwordContactPoint& right) noexcept
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

inline SwordContactPoint SwordContactSubtract(const SwordContactPoint& left,
                                              const SwordContactPoint& right) noexcept
{
    return {{left[0] - right[0], left[1] - right[1], left[2] - right[2]}};
}

inline SwordContactPoint SwordContactAddScaled(const SwordContactPoint& point,
                                               const SwordContactPoint& axis,
                                               const float amount) noexcept
{
    return {{point[0] + axis[0] * amount,
             point[1] + axis[1] * amount,
             point[2] + axis[2] * amount}};
}

inline float SwordContactSegmentDistanceSquared(const SwordContactPoint& p1,
                                                const SwordContactPoint& q1,
                                                const SwordContactPoint& p2,
                                                const SwordContactPoint& q2) noexcept
{
    const auto d1 = SwordContactSubtract(q1, p1);
    const auto d2 = SwordContactSubtract(q2, p2);
    const auto r = SwordContactSubtract(p1, p2);
    const float a = SwordContactDot(d1, d1);
    const float e = SwordContactDot(d2, d2);
    const float f = SwordContactDot(d2, r);
    float s = 0.0f;
    float t = 0.0f;
    if (a <= 1.0e-12f && e <= 1.0e-12f)
        return SwordContactDot(r, r);
    if (a <= 1.0e-12f)
        t = std::clamp(f / e, 0.0f, 1.0f);
    else
    {
        const float c = SwordContactDot(d1, r);
        if (e <= 1.0e-12f)
            s = std::clamp(-c / a, 0.0f, 1.0f);
        else
        {
            const float b = SwordContactDot(d1, d2);
            const float denominator = a * e - b * b;
            if (denominator > 1.0e-12f)
                s = std::clamp((b * f - c * e) / denominator, 0.0f, 1.0f);
            t = (b * s + f) / e;
            if (t < 0.0f)
            {
                t = 0.0f;
                s = std::clamp(-c / a, 0.0f, 1.0f);
            }
            else if (t > 1.0f)
            {
                t = 1.0f;
                s = std::clamp((b - c) / a, 0.0f, 1.0f);
            }
        }
    }
    const auto delta = SwordContactSubtract(SwordContactAddScaled(p1, d1, s),
                                            SwordContactAddScaled(p2, d2, t));
    return SwordContactDot(delta, delta);
}

inline float SwordContactCapsuleSeparation(const SwordContactCapsule& left,
                                           const SwordContactCapsule& right) noexcept
{
    const float centerlineSquared = SwordContactSegmentDistanceSquared(
        left.start, left.end, right.start, right.end);
    const float radii = left.radiusMetres + right.radiusMetres;
    if (!std::isfinite(centerlineSquared) || centerlineSquared < 0.0f ||
        !std::isfinite(radii))
        return std::numeric_limits<float>::infinity();
    const float separation = std::sqrt(centerlineSquared) - radii;
    return std::isfinite(separation)
        ? separation
        : std::numeric_limits<float>::infinity();
}

inline bool SwordContactCapsuleIsFinite(const SwordContactCapsule& capsule) noexcept
{
    if (!std::isfinite(capsule.radiusMetres) || capsule.radiusMetres < 0.0f)
        return false;
    for (std::size_t axis = 0u; axis < capsule.start.size(); ++axis)
        if (!std::isfinite(capsule.start[axis]) || !std::isfinite(capsule.end[axis]))
            return false;
    return true;
}

} // namespace detail

// Compare corresponding fixed-step endpoint poses only. This reports discrete
// contact samples, not a continuous sweep between the poses; wall/occlusion
// authority remains with the simulation caller.
inline SwordContactProxyResult EvaluateSwordContactProxy(
    const SwordContactCapsule& previousBlade,
    const SwordContactCapsule& currentBlade,
    const std::span<const SwordContactTargetPair> targets) noexcept
{
    SwordContactProxyResult result;
    for (const SwordContactTargetPair& target : targets)
    {
        if (!target.eligible || target.targetId < 0)
            continue;
        const auto considerSample = [&](const SwordContactCapsule& blade,
                                        const SwordContactCapsule& targetCapsule,
                                        const SwordContactSample sample) {
            if (!detail::SwordContactCapsuleIsFinite(blade) ||
                !detail::SwordContactCapsuleIsFinite(targetCapsule))
                return;
            const float separation =
                detail::SwordContactCapsuleSeparation(blade, targetCapsule);
            if (!std::isfinite(separation))
                return;
            if (separation < result.minimumSeparationMetres ||
                (separation == result.minimumSeparationMetres &&
                 target.targetId < result.targetId))
            {
                result.minimumSeparationMetres = separation;
                result.targetId = target.targetId;
                result.sample = sample;
            }
            result.touching |= separation <= 0.0f;
        };
        considerSample(previousBlade, target.previous, SwordContactSample::Previous);
        considerSample(currentBlade, target.current, SwordContactSample::Current);
    }
    return result;
}

} // namespace horde::gameplay
