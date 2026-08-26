#include "gameplay/animation/PlayerIkTargets.h"

#include <algorithm>
#include <cmath>

namespace horde::gameplay::animation
{
namespace
{

PlayerIkVector Add(const PlayerIkVector& left, const PlayerIkVector& right)
{
    return {{left[0] + right[0], left[1] + right[1], left[2] + right[2]}};
}

PlayerIkVector Subtract(const PlayerIkVector& left, const PlayerIkVector& right)
{
    return {{left[0] - right[0], left[1] - right[1], left[2] - right[2]}};
}

PlayerIkVector Scale(const PlayerIkVector& value, const float scale)
{
    return {{value[0] * scale, value[1] * scale, value[2] * scale}};
}

float Dot(const PlayerIkVector& left, const PlayerIkVector& right)
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

PlayerIkVector Cross(const PlayerIkVector& left, const PlayerIkVector& right)
{
    return {{left[1] * right[2] - left[2] * right[1],
             left[2] * right[0] - left[0] * right[2],
             left[0] * right[1] - left[1] * right[0]}};
}

float Length(const PlayerIkVector& value)
{
    return std::sqrt(std::max(0.0f, Dot(value, value)));
}

PlayerIkVector Normalize(const PlayerIkVector& value, const PlayerIkVector& fallback)
{
    const float length = Length(value);
    return length > 0.000001f ? Scale(value, 1.0f / length) : fallback;
}

} // namespace

TwoBoneIkSolution SolveTwoBoneIk(const PlayerIkVector& shoulder,
                                 const PlayerIkVector& target,
                                 const PlayerIkVector& pole,
                                 float upperArmLength,
                                 float lowerArmLength)
{
    upperArmLength = std::max(0.0001f, std::abs(upperArmLength));
    lowerArmLength = std::max(0.0001f, std::abs(lowerArmLength));
    const PlayerIkVector requested = Subtract(target, shoulder);
    const float requestedDistance = Length(requested);
    const PlayerIkVector direction = Normalize(requested, {{0.0f, 0.0f, 1.0f}});
    const float minimumReach = std::abs(upperArmLength - lowerArmLength) + 0.00001f;
    const float maximumReach = upperArmLength + lowerArmLength;
    const float solvedDistance = std::clamp(requestedDistance, minimumReach, maximumReach);
    const PlayerIkVector hand = Add(shoulder, Scale(direction, solvedDistance));

    PlayerIkVector bend = Subtract(pole, Scale(direction, Dot(pole, direction)));
    if (Length(bend) <= 0.000001f)
    {
        const PlayerIkVector fallback = std::abs(direction[1]) < 0.9f
            ? PlayerIkVector{{0.0f, 1.0f, 0.0f}}
            : PlayerIkVector{{1.0f, 0.0f, 0.0f}};
        bend = Cross(direction, fallback);
    }
    bend = Normalize(bend, {{0.0f, 0.0f, 1.0f}});
    const float adjacent = std::clamp(
        (upperArmLength * upperArmLength + solvedDistance * solvedDistance -
         lowerArmLength * lowerArmLength) / (2.0f * solvedDistance),
        0.0f,
        upperArmLength);
    const float height = std::sqrt(std::max(
        0.0f, upperArmLength * upperArmLength - adjacent * adjacent));

    TwoBoneIkSolution result;
    result.shoulder = shoulder;
    result.elbow = Add(Add(shoulder, Scale(direction, adjacent)), Scale(bend, height));
    result.hand = hand;
    result.requestedDistance = requestedDistance;
    result.solvedDistance = solvedDistance;
    result.reachable = requestedDistance >= minimumReach - 0.00001f &&
                       requestedDistance <= maximumReach + 0.00001f;
    return result;
}

horde::gameplay::items::HeldItemTransform BuildHandBoneSocketTransform(
    const PlayerIkVector& position,
    const PlayerIkVector& forward,
    const PlayerIkVector& up)
{
    const PlayerIkVector z = Normalize(forward, {{0.0f, 0.0f, 1.0f}});
    PlayerIkVector x = Normalize(Cross(up, z), {{1.0f, 0.0f, 0.0f}});
    const PlayerIkVector y = Normalize(Cross(z, x), {{0.0f, 1.0f, 0.0f}});
    x = Normalize(Cross(y, z), x);
    return {{x[0], x[1], x[2], 0.0f,
             y[0], y[1], y[2], 0.0f,
             z[0], z[1], z[2], 0.0f,
             position[0], position[1], position[2], 1.0f}};
}

float MeasureHandSocketError(
    const horde::gameplay::items::HeldItemTransform& socket,
    const PlayerIkVector& intendedTarget)
{
    return Length({{socket[12] - intendedTarget[0],
                    socket[13] - intendedTarget[1],
                    socket[14] - intendedTarget[2]}});
}

} // namespace horde::gameplay::animation
