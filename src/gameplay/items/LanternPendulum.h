#pragma once

#include "gameplay/items/HeldItemState.h"

#include <array>

namespace horde::gameplay::interactions
{

inline constexpr float kLanternPendulumSoftLimitRadians = 0.78539816339f;
inline constexpr float kLanternPendulumHardLimitRadians = 0.95993108860f;

horde::gameplay::items::HeldItemTransform ComposeLanternPendulumBodyTransform(
    const horde::gameplay::items::HeldItemTransform& worldFromHinge,
    float forwardAngleRadians,
    float strafeAngleRadians);

struct LanternPendulumSnapshot
{
    float forwardAngleRadians = 0.0f;
    float strafeAngleRadians = 0.0f;
    float forwardAngularVelocity = 0.0f;
    float strafeAngularVelocity = 0.0f;
    std::array<float, 3u> previousPivotPosition{};
    std::array<float, 3u> previousPivotVelocity{};
    float centreOfMassLengthMetres = 0.54f;
    horde::gameplay::items::HeldItemTransform worldFromBody{};
    bool initialized = false;

    bool operator==(const LanternPendulumSnapshot&) const = default;
};

class LanternPendulum
{
public:
    void Reset(const horde::gameplay::items::HeldItemTransform& worldFromHinge);
    void Import(const LanternPendulumSnapshot& snapshot);
    const LanternPendulumSnapshot& StepFixed(
        const horde::gameplay::items::HeldItemTransform& worldFromHinge,
        float fixedDeltaSeconds);
    const LanternPendulumSnapshot& Snapshot() const { return snapshot_; }

private:
    LanternPendulumSnapshot snapshot_{};
};

} // namespace horde::gameplay::interactions
