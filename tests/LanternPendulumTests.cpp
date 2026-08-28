#include "gameplay/interactions/LanternPendulum.h"
#include "gameplay/simulation/GameSimulation.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace
{

using namespace horde::gameplay::interactions;
using horde::gameplay::items::HeldItemTransform;

bool passed = true;

void Check(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << "Lantern pendulum test failed: " << message << '\n';
        passed = false;
    }
}

HeldItemTransform Hinge(const float x, const float y, const float z,
                        const float yaw = 0.0f)
{
    const float c = std::cos(yaw);
    const float s = std::sin(yaw);
    return {{c, 0.0f, -s, 0.0f,
             0.0f, 1.0f, 0.0f, 0.0f,
             s, 0.0f, c, 0.0f,
             x, y, z, 1.0f}};
}

bool Finite(const LanternPendulumSnapshot& snapshot)
{
    if (!std::isfinite(snapshot.forwardAngleRadians) ||
        !std::isfinite(snapshot.strafeAngleRadians) ||
        !std::isfinite(snapshot.forwardAngularVelocity) ||
        !std::isfinite(snapshot.strafeAngularVelocity)) return false;
    for (const float value : snapshot.worldFromBody)
    {
        if (!std::isfinite(value)) return false;
    }
    return true;
}

LanternPendulumSnapshot RunStartAndStop()
{
    LanternPendulum pendulum;
    pendulum.Reset(Hinge(0.0f, 1.0f, 0.0f));
    float z = 0.0f;
    for (int tick = 0; tick < 30; ++tick)
    {
        z -= 1.8f / 60.0f;
        pendulum.StepFixed(Hinge(0.0f, 1.0f, z), 1.0f / 60.0f);
    }
    const float movingAngle = pendulum.Snapshot().forwardAngleRadians;
    for (int tick = 0; tick < 45; ++tick)
    {
        pendulum.StepFixed(Hinge(0.0f, 1.0f, z), 1.0f / 60.0f);
    }
    Check(std::abs(movingAngle) > 0.015f,
          "forward acceleration must create visible body lag");
    Check(pendulum.Snapshot().forwardAngleRadians * movingAngle < 0.0f ||
              std::abs(pendulum.Snapshot().forwardAngularVelocity) > 0.03f,
          "stopping must create a physical overshoot response");
    return pendulum.Snapshot();
}

} // namespace

int main()
{
    constexpr float dt = 1.0f / 60.0f;
    LanternPendulum rest;
    rest.Reset(Hinge(0.0f, 1.0f, 0.0f));
    LanternPendulumSnapshot displaced = rest.Snapshot();
    displaced.forwardAngleRadians = 0.40f;
    displaced.strafeAngleRadians = -0.30f;
    rest.Import(displaced);
    for (int tick = 0; tick < 360; ++tick)
    {
        rest.StepFixed(Hinge(0.0f, 1.0f, 0.0f), dt);
    }
    Check(std::abs(rest.Snapshot().forwardAngleRadians) < 0.01f &&
              std::abs(rest.Snapshot().strafeAngleRadians) < 0.01f,
          "gravity and damping must converge a displaced lantern to rest");

    Check(Finite(RunStartAndStop()), "forward start/stop must remain finite");

    LanternPendulum lateral;
    lateral.Reset(Hinge(0.0f, 1.0f, 0.0f));
    lateral.StepFixed(Hinge(0.04f, 1.0f, 0.0f), dt);
    Check(std::abs(lateral.Snapshot().strafeAngleRadians) > 0.01f,
          "strafe acceleration must excite the independent lateral component");
    lateral.StepFixed(Hinge(0.04f, 1.0f, 0.0f, 0.30f), dt);
    Check(std::abs(lateral.Snapshot().forwardAngularVelocity) > 0.001f ||
              std::abs(lateral.Snapshot().strafeAngularVelocity) > 0.001f,
          "turning the authoritative hinge must retain dynamic angular response");

    for (int tick = 0; tick < 18; ++tick)
    {
        lateral.StepFixed(Hinge(0.04f + 0.90f * (tick + 1) / 18.0f,
                                1.0f, 0.0f, 0.30f), dt);
    }
    Check(std::hypot(lateral.Snapshot().forwardAngleRadians,
                     lateral.Snapshot().strafeAngleRadians) <=
              kLanternPendulumHardLimitRadians + 0.0001f,
          "dodge-scale hinge acceleration must remain inside the hard 55 degree stop");

    LanternPendulum clamp;
    clamp.Reset(Hinge(0.0f, 1.0f, 0.0f));
    LanternPendulumSnapshot extreme = clamp.Snapshot();
    extreme.forwardAngleRadians = 4.0f;
    extreme.strafeAngleRadians = -3.0f;
    extreme.forwardAngularVelocity = 80.0f;
    extreme.strafeAngularVelocity = -60.0f;
    clamp.Import(extreme);
    clamp.StepFixed(Hinge(0.0f, 1.0f, 0.0f), dt);
    Check(std::hypot(clamp.Snapshot().forwardAngleRadians,
                     clamp.Snapshot().strafeAngleRadians) <=
              kLanternPendulumHardLimitRadians + 0.0001f,
          "import and integration must enforce soft 45/hard 55 degree limits");

    clamp.StepFixed(Hinge(20.0f, 4.0f, -18.0f), dt);
    Check(std::abs(clamp.Snapshot().forwardAngleRadians) < 0.0001f &&
              std::abs(clamp.Snapshot().strafeAngleRadians) < 0.0001f &&
              clamp.Snapshot().worldFromBody[12] == 20.0f &&
              clamp.Snapshot().worldFromBody[13] == 4.0f &&
              clamp.Snapshot().worldFromBody[14] == -18.0f,
          "teleports must reset motion at the exact new hinge without velocity injection");

    LanternPendulum nanGuard;
    nanGuard.Reset(Hinge(0.0f, 1.0f, 0.0f));
    HeldItemTransform invalid = Hinge(0.0f, 1.0f, 0.0f);
    invalid[12] = std::numeric_limits<float>::quiet_NaN();
    nanGuard.StepFixed(invalid, dt);
    Check(Finite(nanGuard.Snapshot()), "invalid hinge input must never publish NaNs");

    LanternPendulum frozen;
    frozen.Reset(Hinge(-3.0f, 2.0f, 7.0f));
    LanternPendulumSnapshot authored = frozen.Snapshot();
    authored.forwardAngleRadians = 0.22f;
    authored.strafeAngleRadians = -0.18f;
    authored.forwardAngularVelocity = 0.70f;
    authored.strafeAngularVelocity = -0.45f;
    frozen.Import(authored);
    Check(frozen.Snapshot().forwardAngleRadians == authored.forwardAngleRadians &&
              frozen.Snapshot().strafeAngularVelocity == authored.strafeAngularVelocity,
          "checkpoint import must preserve exact finite pendulum angles and velocities");

    LanternPendulum cadence30;
    LanternPendulum cadence60;
    LanternPendulum cadence120;
    cadence30.Reset(Hinge(0.0f, 1.0f, 0.0f));
    cadence60.Reset(Hinge(0.0f, 1.0f, 0.0f));
    cadence120.Reset(Hinge(0.0f, 1.0f, 0.0f));
    for (int tick = 1; tick <= 120; ++tick)
    {
        const float t = tick * dt;
        const HeldItemTransform hinge = Hinge(0.35f * std::sin(t * 2.1f),
                                              1.0f,
                                              -0.80f * t,
                                              0.4f * std::sin(t));
        cadence30.StepFixed(hinge, dt);
        cadence60.StepFixed(hinge, dt);
        cadence120.StepFixed(hinge, dt);
    }
    Check(cadence30.Snapshot() == cadence60.Snapshot() &&
              cadence60.Snapshot() == cadence120.Snapshot(),
          "30/60/120 render delivery must not change shared 60 Hz pendulum state");

    horde::gameplay::simulation::GameSimulation simulation30;
    horde::gameplay::simulation::GameSimulation simulation60;
    horde::gameplay::simulation::GameSimulation simulation120;
    ChestRewardSnapshot claimedChest;
    claimedChest.phase = ChestRewardPhase::LanternClaimed;
    claimedChest.lidOpenProgress = 1.0f;
    InteractionState heldReward;
    heldReward.heldLightKind = HeldLightKind::RewardLantern;
    heldReward.heldLightPose = HeldLightPose::High;
    FinaleSequenceSnapshot inactiveFinale;
    simulation30.ImportRewardCheckpoint(claimedChest, heldReward, inactiveFinale);
    simulation60.ImportRewardCheckpoint(claimedChest, heldReward, inactiveFinale);
    simulation120.ImportRewardCheckpoint(claimedChest, heldReward, inactiveFinale);
    horde::gameplay::simulation::InputSnapshot moving;
    moving.damageEnabled = false;
    moving.moveForward = 1.0f;
    for (int frame = 0; frame < 60; ++frame) simulation30.AdvanceFrame(moving, 1.0 / 30.0);
    for (int frame = 0; frame < 120; ++frame) simulation60.AdvanceFrame(moving, 1.0 / 60.0);
    for (int frame = 0; frame < 240; ++frame) simulation120.AdvanceFrame(moving, 1.0 / 120.0);
    const auto& shared30 = simulation30.Snapshot().lanternPendulum;
    const auto& shared60 = simulation60.Snapshot().lanternPendulum;
    const auto& shared120 = simulation120.Snapshot().lanternPendulum;
    Check(simulation30.Snapshot().tickIndex == simulation60.Snapshot().tickIndex &&
              simulation60.Snapshot().tickIndex == simulation120.Snapshot().tickIndex &&
              std::abs(shared30.forwardAngleRadians - shared60.forwardAngleRadians) < 0.00001f &&
              std::abs(shared60.forwardAngleRadians - shared120.forwardAngleRadians) < 0.00001f &&
              std::abs(shared30.strafeAngleRadians - shared120.strafeAngleRadians) < 0.00001f,
          "GameSimulation must own identical pendulum state under 30/60/120 Hz frame delivery");

    if (!passed) return EXIT_FAILURE;
    std::cout << "Lantern pendulum deterministic physics tests passed\n";
    return EXIT_SUCCESS;
}
