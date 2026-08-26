#pragma once

#include <array>

#include "gameplay/items/HeldItemState.h"

namespace horde::gameplay::animation
{

using PlayerIkVector = std::array<float, 3u>;

struct PlayerArmIkTarget
{
    PlayerIkVector shoulder{};
    PlayerIkVector target{};
    PlayerIkVector pole{{0.0f, 0.0f, 1.0f}};
    // Gameplay-authored Grip basis in view-local coordinates. The renderer
    // transforms these vectors into player space and uses a stable rest
    // bone-to-Grip basis to orient the actual hand joint.
    PlayerIkVector gripX{{1.0f, 0.0f, 0.0f}};
    PlayerIkVector gripY{{0.0f, 1.0f, 0.0f}};
    PlayerIkVector gripZ{{0.0f, 0.0f, -1.0f}};
    float upperArmLength = 0.42f;
    float lowerArmLength = 0.40f;

    bool operator==(const PlayerArmIkTarget&) const = default;
};

struct TwoBoneIkSolution
{
    PlayerIkVector shoulder{};
    PlayerIkVector elbow{};
    PlayerIkVector hand{};
    float requestedDistance = 0.0f;
    float solvedDistance = 0.0f;
    bool reachable = false;

    bool operator==(const TwoBoneIkSolution&) const = default;
};

TwoBoneIkSolution SolveTwoBoneIk(const PlayerIkVector& shoulder,
                                 const PlayerIkVector& target,
                                 const PlayerIkVector& pole,
                                 float upperArmLength,
                                 float lowerArmLength);

horde::gameplay::items::HeldItemTransform BuildHandBoneSocketTransform(
    const PlayerIkVector& position,
    const PlayerIkVector& forward,
    const PlayerIkVector& up);

float MeasureHandSocketError(
    const horde::gameplay::items::HeldItemTransform& socket,
    const PlayerIkVector& intendedTarget);

} // namespace horde::gameplay::animation
