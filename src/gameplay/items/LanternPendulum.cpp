#include "gameplay/items/LanternPendulum.h"

#include <algorithm>
#include <cmath>

namespace horde::gameplay::interactions
{
namespace
{

using horde::gameplay::items::HeldItemTransform;
using Vec3 = std::array<float, 3u>;

constexpr float kGravity = 9.81f;
constexpr float kDefaultCentreOfMassLength = 0.54f;
constexpr float kAngularDamping = 2.45f;
constexpr float kSoftLimitSpring = 52.0f;
constexpr float kTorsionRestoring = 16.0f;
constexpr float kTorsionDamping = 4.2f;
constexpr float kTorsionTurnResponse = 1.3f;
constexpr float kTorsionSoftLimitSpring = 44.0f;
constexpr float kTeleportDistanceMetres = 0.70f;
constexpr float kHandBasisTeleportRadians = 0.75f;
constexpr float kMaximumHingeAcceleration = 70.0f;
constexpr float kMaximumAngularVelocity = 9.0f;

bool FiniteTransform(const HeldItemTransform& transform)
{
    for (const float value : transform)
    {
        if (!std::isfinite(value)) return false;
    }
    return true;
}

float Dot(const Vec3& left, const Vec3& right)
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

Vec3 Column(const HeldItemTransform& transform, const std::size_t column)
{
    return {{transform[column * 4u], transform[column * 4u + 1u],
             transform[column * 4u + 2u]}};
}

HeldItemTransform Multiply(const HeldItemTransform& left,
                           const HeldItemTransform& right)
{
    HeldItemTransform result{};
    for (std::size_t column = 0u; column < 4u; ++column)
    {
        for (std::size_t row = 0u; row < 4u; ++row)
        {
            for (std::size_t inner = 0u; inner < 4u; ++inner)
            {
                result[column * 4u + row] +=
                    left[inner * 4u + row] * right[column * 4u + inner];
            }
        }
    }
    return result;
}

Vec3 HorizontalForward(const HeldItemTransform& transform)
{
    Vec3 forward = Column(transform, 2u);
    forward[1] = 0.0f;
    const float lengthSquared = Dot(forward, forward);
    if (!std::isfinite(lengthSquared) || lengthSquared <= 0.000001f)
        return {{0.0f, 0.0f, 1.0f}};
    const float inverseLength = 1.0f / std::sqrt(lengthSquared);
    return {{forward[0] * inverseLength, 0.0f, forward[2] * inverseLength}};
}

float SignedYawDelta(const Vec3& previous, const Vec3& current)
{
    const float crossY = previous[2] * current[0] - previous[0] * current[2];
    const float cosine = previous[0] * current[0] + previous[2] * current[2];
    return std::atan2(crossY, cosine);
}

HeldItemTransform Rotation(const float forward,
                           const float strafe,
                           const float torsion)
{
    const float cx = std::cos(forward);
    const float sx = std::sin(forward);
    const float cz = std::cos(strafe);
    const float sz = std::sin(strafe);
    const float cy = std::cos(torsion);
    const float sy = std::sin(torsion);
    const HeldItemTransform rotateX{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, cx, sx, 0.0f,
        0.0f, -sx, cx, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f}};
    const HeldItemTransform rotateZ{{
        cz, sz, 0.0f, 0.0f,
        -sz, cz, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f}};
    const HeldItemTransform rotateY{{
        cy, 0.0f, -sy, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        sy, 0.0f, cy, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f}};
    // Swing establishes the hanging axis; torsion then rotates the body about
    // its local Y axis without altering the ring/hand rigid attachment.
    return Multiply(Multiply(rotateZ, rotateX), rotateY);
}

void ClampMotion(LanternPendulumSnapshot& snapshot)
{
    if (!std::isfinite(snapshot.forwardAngleRadians)) snapshot.forwardAngleRadians = 0.0f;
    if (!std::isfinite(snapshot.strafeAngleRadians)) snapshot.strafeAngleRadians = 0.0f;
    if (!std::isfinite(snapshot.torsionAngleRadians)) snapshot.torsionAngleRadians = 0.0f;
    if (!std::isfinite(snapshot.forwardAngularVelocity)) snapshot.forwardAngularVelocity = 0.0f;
    if (!std::isfinite(snapshot.strafeAngularVelocity)) snapshot.strafeAngularVelocity = 0.0f;
    if (!std::isfinite(snapshot.torsionAngularVelocity)) snapshot.torsionAngularVelocity = 0.0f;
    snapshot.forwardAngularVelocity = std::clamp(
        snapshot.forwardAngularVelocity, -kMaximumAngularVelocity, kMaximumAngularVelocity);
    snapshot.strafeAngularVelocity = std::clamp(
        snapshot.strafeAngularVelocity, -kMaximumAngularVelocity, kMaximumAngularVelocity);
    snapshot.torsionAngularVelocity = std::clamp(
        snapshot.torsionAngularVelocity,
        -kLanternPendulumMaximumTorsionVelocity,
        kLanternPendulumMaximumTorsionVelocity);
    snapshot.torsionAngleRadians = std::clamp(
        snapshot.torsionAngleRadians,
        -kLanternPendulumTorsionHardLimitRadians,
        kLanternPendulumTorsionHardLimitRadians);
    if (std::abs(snapshot.torsionAngleRadians) >=
            kLanternPendulumTorsionHardLimitRadians &&
        snapshot.torsionAngularVelocity * snapshot.torsionAngleRadians > 0.0f)
    {
        snapshot.torsionAngularVelocity = 0.0f;
    }
    const float magnitude = std::hypot(snapshot.forwardAngleRadians,
                                       snapshot.strafeAngleRadians);
    if (magnitude <= kLanternPendulumHardLimitRadians || magnitude <= 0.000001f)
    {
        return;
    }
    const float scale = kLanternPendulumHardLimitRadians / magnitude;
    snapshot.forwardAngleRadians *= scale;
    snapshot.strafeAngleRadians *= scale;
    const float radialVelocity =
        (snapshot.forwardAngularVelocity * snapshot.forwardAngleRadians +
         snapshot.strafeAngularVelocity * snapshot.strafeAngleRadians) /
        kLanternPendulumHardLimitRadians;
    if (radialVelocity > 0.0f)
    {
        const float inverseRadius = 1.0f / kLanternPendulumHardLimitRadians;
        snapshot.forwardAngularVelocity -=
            1.20f * radialVelocity * snapshot.forwardAngleRadians * inverseRadius;
        snapshot.strafeAngularVelocity -=
            1.20f * radialVelocity * snapshot.strafeAngleRadians * inverseRadius;
    }
}

} // namespace

HeldItemTransform ComposeLanternPendulumBodyTransform(
    const HeldItemTransform& worldFromHinge,
    const float forwardAngleRadians,
    const float strafeAngleRadians,
    const float torsionAngleRadians)
{
    if (!FiniteTransform(worldFromHinge) || !std::isfinite(forwardAngleRadians) ||
        !std::isfinite(strafeAngleRadians) || !std::isfinite(torsionAngleRadians))
    {
        return horde::gameplay::items::IdentityHeldItemTransform();
    }
    return Multiply(worldFromHinge,
                    Rotation(forwardAngleRadians, strafeAngleRadians,
                             torsionAngleRadians));
}

void LanternPendulum::Reset(const HeldItemTransform& worldFromHinge)
{
    if (!FiniteTransform(worldFromHinge))
    {
        snapshot_ = {};
        snapshot_.worldFromBody = horde::gameplay::items::IdentityHeldItemTransform();
        return;
    }
    snapshot_ = {};
    snapshot_.previousPivotPosition = {{worldFromHinge[12], worldFromHinge[13],
                                        worldFromHinge[14]}};
    snapshot_.previousHandForward = HorizontalForward(worldFromHinge);
    snapshot_.worldFromBody = worldFromHinge;
    snapshot_.initialized = true;
}

void LanternPendulum::Import(const LanternPendulumSnapshot& snapshot)
{
    snapshot_ = snapshot;
    snapshot_.centreOfMassLengthMetres =
        std::isfinite(snapshot_.centreOfMassLengthMetres)
        ? std::clamp(snapshot_.centreOfMassLengthMetres, 0.12f, 1.20f)
        : kDefaultCentreOfMassLength;
    ClampMotion(snapshot_);
    for (float& value : snapshot_.previousPivotPosition)
    {
        if (!std::isfinite(value)) value = 0.0f;
    }
    for (float& value : snapshot_.previousPivotVelocity)
    {
        if (!std::isfinite(value)) value = 0.0f;
    }
    bool finiteHandForward = true;
    for (const float value : snapshot_.previousHandForward)
        finiteHandForward = finiteHandForward && std::isfinite(value);
    const float handForwardLengthSquared = Dot(snapshot_.previousHandForward,
                                               snapshot_.previousHandForward);
    if (!finiteHandForward || handForwardLengthSquared < 0.81f ||
        handForwardLengthSquared > 1.21f)
    {
        snapshot_.previousHandForward = {{0.0f, 0.0f, 1.0f}};
    }
    if (!FiniteTransform(snapshot_.worldFromBody))
    {
        snapshot_.worldFromBody = horde::gameplay::items::IdentityHeldItemTransform();
        snapshot_.initialized = false;
    }
}

const LanternPendulumSnapshot& LanternPendulum::StepFixed(
    const HeldItemTransform& worldFromHinge,
    const float fixedDeltaSeconds)
{
    if (!FiniteTransform(worldFromHinge) || !std::isfinite(fixedDeltaSeconds) ||
        fixedDeltaSeconds <= 0.0f || fixedDeltaSeconds > 0.05f)
    {
        return snapshot_;
    }
    const Vec3 pivot{{worldFromHinge[12], worldFromHinge[13], worldFromHinge[14]}};
    if (!snapshot_.initialized)
    {
        Reset(worldFromHinge);
        return snapshot_;
    }
    const Vec3 displacement{{pivot[0] - snapshot_.previousPivotPosition[0],
                             pivot[1] - snapshot_.previousPivotPosition[1],
                             pivot[2] - snapshot_.previousPivotPosition[2]}};
    if (std::sqrt(Dot(displacement, displacement)) > kTeleportDistanceMetres)
    {
        Reset(worldFromHinge);
        return snapshot_;
    }
    const Vec3 handForward = HorizontalForward(worldFromHinge);
    const float handYawDelta = SignedYawDelta(snapshot_.previousHandForward,
                                              handForward);
    if (!std::isfinite(handYawDelta) ||
        std::abs(handYawDelta) > kHandBasisTeleportRadians)
    {
        Reset(worldFromHinge);
        return snapshot_;
    }

    const Vec3 velocity{{displacement[0] / fixedDeltaSeconds,
                         displacement[1] / fixedDeltaSeconds,
                         displacement[2] / fixedDeltaSeconds}};
    Vec3 acceleration{{
        (velocity[0] - snapshot_.previousPivotVelocity[0]) / fixedDeltaSeconds,
        (velocity[1] - snapshot_.previousPivotVelocity[1]) / fixedDeltaSeconds,
        (velocity[2] - snapshot_.previousPivotVelocity[2]) / fixedDeltaSeconds}};
    for (float& component : acceleration)
    {
        component = std::clamp(component,
                               -kMaximumHingeAcceleration,
                               kMaximumHingeAcceleration);
    }
    const float forwardAcceleration = Dot(acceleration, Column(worldFromHinge, 2u));
    const float strafeAcceleration = Dot(acceleration, Column(worldFromHinge, 0u));
    const float centreOfMassLength = snapshot_.centreOfMassLengthMetres;
    float forwardAngularAcceleration =
        -(kGravity / centreOfMassLength) * std::sin(snapshot_.forwardAngleRadians) +
        forwardAcceleration / centreOfMassLength -
        kAngularDamping * snapshot_.forwardAngularVelocity;
    float strafeAngularAcceleration =
        -(kGravity / centreOfMassLength) * std::sin(snapshot_.strafeAngleRadians) -
        strafeAcceleration / centreOfMassLength -
        kAngularDamping * snapshot_.strafeAngularVelocity;
    float torsionAngularAcceleration =
        -kTorsionRestoring * std::sin(snapshot_.torsionAngleRadians) -
        kTorsionDamping * snapshot_.torsionAngularVelocity -
        kTorsionTurnResponse * (handYawDelta / fixedDeltaSeconds);

    const float angleMagnitude = std::hypot(snapshot_.forwardAngleRadians,
                                            snapshot_.strafeAngleRadians);
    if (angleMagnitude > kLanternPendulumSoftLimitRadians)
    {
        const float excess = angleMagnitude - kLanternPendulumSoftLimitRadians;
        const float scale = -kSoftLimitSpring * excess / angleMagnitude;
        forwardAngularAcceleration += scale * snapshot_.forwardAngleRadians;
        strafeAngularAcceleration += scale * snapshot_.strafeAngleRadians;
    }
    if (std::abs(snapshot_.torsionAngleRadians) >
        kLanternPendulumTorsionSoftLimitRadians)
    {
        const float excess = std::abs(snapshot_.torsionAngleRadians) -
            kLanternPendulumTorsionSoftLimitRadians;
        torsionAngularAcceleration -= kTorsionSoftLimitSpring * excess *
            std::copysign(1.0f, snapshot_.torsionAngleRadians);
    }

    snapshot_.forwardAngularVelocity += forwardAngularAcceleration * fixedDeltaSeconds;
    snapshot_.strafeAngularVelocity += strafeAngularAcceleration * fixedDeltaSeconds;
    snapshot_.torsionAngularVelocity += torsionAngularAcceleration * fixedDeltaSeconds;
    snapshot_.forwardAngleRadians += snapshot_.forwardAngularVelocity * fixedDeltaSeconds;
    snapshot_.strafeAngleRadians += snapshot_.strafeAngularVelocity * fixedDeltaSeconds;
    snapshot_.torsionAngleRadians += snapshot_.torsionAngularVelocity * fixedDeltaSeconds;
    snapshot_.previousPivotPosition = pivot;
    snapshot_.previousPivotVelocity = velocity;
    snapshot_.previousHandForward = handForward;
    ClampMotion(snapshot_);
    snapshot_.worldFromBody = ComposeLanternPendulumBodyTransform(
        worldFromHinge, snapshot_.forwardAngleRadians, snapshot_.strafeAngleRadians,
        snapshot_.torsionAngleRadians);
    return snapshot_;
}

} // namespace horde::gameplay::interactions
