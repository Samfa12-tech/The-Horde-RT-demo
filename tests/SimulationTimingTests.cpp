#include <cmath>
#include <cstdint>
#include <iostream>

#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/simulation/InputMailbox.h"

namespace
{

using horde::gameplay::simulation::GameSimulation;
using horde::gameplay::simulation::InputMailbox;
using horde::gameplay::simulation::InputSnapshot;
using horde::gameplay::simulation::PublishedInput;
using horde::gameplay::simulation::SimulationSnapshot;

bool NearlyEqual(float left, float right, float epsilon = 0.00002f)
{
    return std::abs(left - right) <= epsilon;
}

SimulationSnapshot RunCadence(int frameRate, double seconds, float forward, float strafe)
{
    GameSimulation simulation;
    InputSnapshot input;
    input.moveForward = forward;
    input.moveStrafe = strafe;
    input.damageEnabled = false;
    const int frames = static_cast<int>(frameRate * seconds);
    for (int frame = 0; frame < frames; ++frame)
    {
        simulation.AdvanceFrame(input, 1.0 / static_cast<double>(frameRate), frame + 1u);
    }
    return simulation.Snapshot();
}

SimulationSnapshot RunCombatCadence(int frameRate, bool insertHitch)
{
    GameSimulation simulation;
    InputSnapshot input;
    input.hasAuthoritativePlayerPose = true;
    input.authoritativePlayerX = -0.75f;
    input.authoritativePlayerZ = -3.20f;
    input.yawRadians = 0.0f;
    input.damageEnabled = false;
    input.commands.attack = 1u;
    const int ordinaryFrames = frameRate - (insertHitch ? 6 : 0);
    for (int frame = 0; frame < ordinaryFrames; ++frame)
    {
        simulation.AdvanceFrame(input, 1.0 / static_cast<double>(frameRate), frame + 1u);
        if (insertHitch && frame == frameRate / 4)
        {
            simulation.AdvanceFrame(input, 0.100, frameRate + 1u);
        }
    }
    return simulation.Snapshot();
}

} // namespace

int main()
{
    bool passed = true;
    const auto check = [&passed](bool condition, const char* message)
    {
        if (!condition)
        {
            passed = false;
            std::cerr << "Simulation timing test failed: " << message << '\n';
        }
    };

    const SimulationSnapshot at30 = RunCadence(30, 2.0, 1.0f, 0.0f);
    const SimulationSnapshot at60 = RunCadence(60, 2.0, 1.0f, 0.0f);
    const SimulationSnapshot at120 = RunCadence(120, 2.0, 1.0f, 0.0f);
    const auto sameEnemy = [](const auto& left, const auto& right)
    {
        return left.id == right.id && NearlyEqual(left.x, right.x) &&
               NearlyEqual(left.z, right.z) && NearlyEqual(left.facingRadians, right.facingRadians) &&
               NearlyEqual(left.animationTime, right.animationTime) &&
               NearlyEqual(left.damageFlash, right.damageFlash) && left.health == right.health &&
               left.animation == right.animation && left.dead == right.dead &&
               left.playerHitPulse == right.playerHitPulse;
    };
    check(at30.tickIndex == 120u && at60.tickIndex == 120u && at120.tickIndex == 120u,
          "30/60/120 Hz delivery must execute the same 120 fixed ticks");
    check(NearlyEqual(at30.playerX, at60.playerX) && NearlyEqual(at60.playerX, at120.playerX) &&
          NearlyEqual(at30.playerZ, at60.playerZ) && NearlyEqual(at60.playerZ, at120.playerZ),
          "render cadence must not change final player position");
    check(at30.activeEnemyKind == at60.activeEnemyKind && at60.activeEnemyKind == at120.activeEnemyKind &&
          at30.playerVitals.vitality == at60.playerVitals.vitality &&
          at60.playerVitals.vitality == at120.playerVitals.vitality &&
          at30.activeSkeletonCount == at60.activeSkeletonCount &&
          at60.activeSkeletonCount == at120.activeSkeletonCount &&
          at30.skeletonEnemyCount == at60.skeletonEnemyCount &&
          at60.skeletonEnemyCount == at120.skeletonEnemyCount &&
          at30.skeletonAttackerId == at60.skeletonAttackerId &&
          at60.skeletonAttackerId == at120.skeletonAttackerId &&
          at30.openingEncounterComplete == at60.openingEncounterComplete &&
          at60.openingEncounterComplete == at120.openingEncounterComplete &&
          sameEnemy(at30.skeletonEnemies[0], at60.skeletonEnemies[0]) &&
          sameEnemy(at60.skeletonEnemies[0], at120.skeletonEnemies[0]) &&
          sameEnemy(at30.skeletonEnemies[1], at60.skeletonEnemies[1]) &&
          sameEnemy(at60.skeletonEnemies[1], at120.skeletonEnemies[1]),
          "render cadence must preserve encounter, vitality, and action state");

    GameSimulation baseline;
    GameSimulation hitch;
    InputSnapshot forward;
    forward.moveForward = 1.0f;
    forward.damageEnabled = false;
    for (int i = 0; i < 120; ++i)
    {
        baseline.AdvanceFrame(forward, 1.0 / 60.0);
    }
    for (int i = 0; i < 30; ++i)
    {
        hitch.AdvanceFrame(forward, 1.0 / 60.0);
    }
    hitch.AdvanceFrame(forward, 0.100);
    for (int i = 0; i < 84; ++i)
    {
        hitch.AdvanceFrame(forward, 1.0 / 60.0);
    }
    check(hitch.Snapshot().tickIndex == baseline.Snapshot().tickIndex &&
          NearlyEqual(hitch.Snapshot().playerX, baseline.Snapshot().playerX) &&
          NearlyEqual(hitch.Snapshot().playerZ, baseline.Snapshot().playerZ),
          "a bounded 100 ms hitch must retain deterministic time and motion parity");
    check(hitch.Snapshot().catchUpOverrunCount == 0u,
          "an exactly 100 ms hitch must fit the supported catch-up bound");

    const SimulationSnapshot combat30 = RunCombatCadence(30, false);
    const SimulationSnapshot combat60 = RunCombatCadence(60, false);
    const SimulationSnapshot combat120 = RunCombatCadence(120, false);
    const SimulationSnapshot combatHitch = RunCombatCadence(60, true);
    check(combat30.tickIndex == combat60.tickIndex && combat60.tickIndex == combat120.tickIndex &&
          combatHitch.tickIndex == combat60.tickIndex &&
          combat30.activeSkeletonCount == 1u &&
          combat30.activeSkeletonCount == combat60.activeSkeletonCount &&
          combat60.activeSkeletonCount == combat120.activeSkeletonCount &&
          combat120.activeSkeletonCount == combatHitch.activeSkeletonCount &&
          combat30.playerCombat.action == combat60.playerCombat.action &&
          combat60.playerCombat.action == combat120.playerCombat.action &&
          combat120.playerCombat.action == combatHitch.playerCombat.action,
          "swing contact and action completion must retain 30/60/120 Hz and bounded-hitch parity");

    const SimulationSnapshot axial = RunCadence(60, 0.75, 1.0f, 0.0f);
    const SimulationSnapshot diagonal = RunCadence(60, 0.75, 1.0f, 1.0f);
    const float axialDistance = std::hypot(axial.playerX, axial.playerZ - 1.85f);
    const float diagonalDistance = std::hypot(diagonal.playerX, diagonal.playerZ - 1.85f);
    check(NearlyEqual(axialDistance, diagonalDistance, 0.0001f),
          "full diagonal input must have no speed advantage over one axis");

    GameSimulation paused;
    paused.AdvanceFrame(forward, 1.0 / 120.0);
    const float beforePauseX = paused.Snapshot().playerX;
    const float beforePauseZ = paused.Snapshot().playerZ;
    const std::uint64_t beforePauseTick = paused.Snapshot().tickIndex;
    InputSnapshot pauseInput = forward;
    pauseInput.paused = true;
    paused.AdvanceFrame(pauseInput, 1.0);
    check(paused.Snapshot().tickIndex == beforePauseTick &&
          NearlyEqual(paused.Snapshot().playerX, beforePauseX) &&
          NearlyEqual(paused.Snapshot().playerZ, beforePauseZ),
          "pause must not advance movement or combat");
    paused.AdvanceFrame(forward, 1.0 / 120.0);
    paused.AdvanceFrame(forward, 1.0 / 120.0);
    check(paused.Snapshot().tickIndex == beforePauseTick + 1u,
          "resume must discard the stale partial accumulator and advance only fresh time");

    GameSimulation oversized;
    const std::uint32_t oversizedTicks = oversized.AdvanceFrame(forward, 0.5);
    check(oversizedTicks == 6u && oversizedTicks <= 8u &&
          oversized.Snapshot().catchUpOverrunCount == 1u,
          "an oversized stall must clamp to 100 ms, stay within eight ticks, and report one overrun");

    GameSimulation windowsStyle;
    GameSimulation androidMailboxStyle;
    InputMailbox mailbox;
    InputSnapshot equivalentInput;
    equivalentInput.moveForward = 0.72f;
    equivalentInput.moveStrafe = -0.31f;
    equivalentInput.yawRadians = 0.38f;
    equivalentInput.pitchRadians = -0.07f;
    equivalentInput.torchLightStrength = 1.8f;
    equivalentInput.damageEnabled = false;
    for (std::uint64_t frame = 1u; frame <= 90u; ++frame)
    {
        if (frame == 12u || frame == 48u)
        {
            ++equivalentInput.commands.attack;
        }
        if (frame == 24u || frame == 72u)
        {
            ++equivalentInput.commands.dodge;
        }
        windowsStyle.AdvanceFrame(equivalentInput, 1.0 / 60.0, frame);
        mailbox.Publish(equivalentInput);
        const PublishedInput published = mailbox.ConsumeLatest();
        androidMailboxStyle.AdvanceFrame(
            published.snapshot, 1.0 / 60.0, published.publicationSequence);
    }
    const SimulationSnapshot& windowsSnapshot = windowsStyle.Snapshot();
    const SimulationSnapshot& androidSnapshot = androidMailboxStyle.Snapshot();
    check(windowsSnapshot.tickIndex == androidSnapshot.tickIndex &&
          NearlyEqual(windowsSnapshot.playerX, androidSnapshot.playerX) &&
          NearlyEqual(windowsSnapshot.playerZ, androidSnapshot.playerZ) &&
          windowsSnapshot.activeEnemyKind == androidSnapshot.activeEnemyKind &&
          windowsSnapshot.playerVitals.vitality == androidSnapshot.playerVitals.vitality &&
          windowsSnapshot.lastConsumedAttackSequence == androidSnapshot.lastConsumedAttackSequence &&
          windowsSnapshot.lastConsumedDodgeSequence == androidSnapshot.lastConsumedDodgeSequence,
          "equivalent direct Windows and coherent Android-mailbox input must produce the same simulation snapshot");

    if (!passed)
    {
        return 1;
    }
    std::cout << "Shared simulation cadence, hitch, diagonal, pause, and overrun tests passed.\n";
    return 0;
}
