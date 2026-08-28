#pragma once

#include "gameplay/items/HeldItemState.h"

#include <array>

namespace horde::gameplay::interactions
{

inline constexpr float kLanternPendulumSoftLimitRadians = 0.78539816339f;
inline constexpr float kLanternPendulumHardLimitRadians = 0.95993108860f;
inline constexpr float kLanternPendulumTorsionSoftLimitRadians = 0.26179938780f;
inline constexpr float kLanternPendulumTorsionHardLimitRadians = 0.34906585040f;
inline constexpr float kLanternPendulumMaximumTorsionVelocity = 6.0f;

horde::gameplay::items::HeldItemTransform ComposeLanternPendulumBodyTransform(
    const horde::gameplay::items::HeldItemTransform& worldFromHinge,
    float forwardAngleRadians,
    float strafeAngleRadians,
    float torsionAngleRadians = 0.0f,
    float presentationYawRadians = 0.0f);

struct LanternPendulumSnapshot
{
    float forwardAngleRadians = 0.0f;
    float strafeAngleRadians = 0.0f;
    float torsionAngleRadians = 0.0f;
    float forwardAngularVelocity = 0.0f;
    float strafeAngularVelocity = 0.0f;
    float torsionAngularVelocity = 0.0f;
    std::array<float, 3u> previousPivotPosition{};
    std::array<float, 3u> previousPivotVelocity{};
    std::array<float, 3u> previousHandForward{{0.0f, 0.0f, 1.0f}};
    float centreOfMassLengthMetres = 0.54f;
    horde::gameplay::items::HeldItemTransform worldFromBody{};
    bool initialized = false;

    bool operator==(const LanternPendulumSnapshot&) const = default;
};

class LanternPendulum
{
public:
    void Reset(const horde::gameplay::items::HeldItemTransform& worldFromHinge,
               float presentationYawRadians = 0.0f);
    void Import(const LanternPendulumSnapshot& snapshot);
    const LanternPendulumSnapshot& StepFixed(
        const horde::gameplay::items::HeldItemTransform& worldFromHinge,
        float fixedDeltaSeconds,
        float presentationYawRadians = 0.0f);
    const LanternPendulumSnapshot& Snapshot() const { return snapshot_; }

private:
    LanternPendulumSnapshot snapshot_{};
};

} // namespace horde::gameplay::interactions
