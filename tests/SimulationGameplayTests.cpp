#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "gameplay/simulation/GameSimulation.h"

namespace
{

using namespace horde::gameplay;
using namespace horde::gameplay::simulation;

std::size_t CountEvents(const BoundedGameplayEventQueue& queue, GameplayEventType type)
{
    return static_cast<std::size_t>(std::count_if(
        queue.Events().begin(),
        queue.Events().end(),
        [type](const GameplayEvent& event) { return event.type == type; }));
}

bool NearlyEqual(float left, float right, float epsilon = 0.0001f)
{
    return std::abs(left - right) <= epsilon;
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
            std::cerr << "Simulation gameplay test failed: " << message << '\n';
        }
    };

    BoundedGameplayEventQueue identityQueue;
    GameplayEvent first;
    first.type = GameplayEventType::EnemyHit;
    first.source = EntityId::Player;
    first.target = EntityId::Skeleton;
    first.worldX = 1.0f;
    GameplayEvent second = first;
    second.target = EntityId::Lich;
    second.worldX = 2.0f;
    check(identityQueue.Push(first) && identityQueue.Push(second),
          "two same-type events must fit the bounded queue");
    check(identityQueue[0].sequence != identityQueue[1].sequence &&
          identityQueue[0].target == EntityId::Skeleton &&
          identityQueue[1].target == EntityId::Lich &&
          identityQueue[0].worldX != identityQueue[1].worldX,
          "same-type events must retain distinct sequences, entities, and positions");
    for (std::size_t i = identityQueue.Size(); i < BoundedGameplayEventQueue::kCapacity; ++i)
    {
        identityQueue.Push({});
    }
    check(!identityQueue.Push({}) && identityQueue.OverflowCount() == 1u &&
          identityQueue.Size() == BoundedGameplayEventQueue::kCapacity,
          "overflow must be explicit and must not overwrite an unrelated event");

    GameSimulation attacks;
    InputSnapshot attackInput;
    attackInput.damageEnabled = false;
    attackInput.commands.attack = 1u;
    attacks.AdvanceFrame(attackInput, 1.0 / 60.0, 1u);
    check(attacks.Snapshot().lastConsumedAttackSequence == 1u &&
          CountEvents(attacks.Events(), GameplayEventType::PlayerSwing) == 1u,
          "attack sequence N must be consumed exactly once");
    for (int frame = 0; frame < 5; ++frame)
    {
        attacks.AdvanceFrame(attackInput, 1.0 / 60.0, 1u);
    }
    check(CountEvents(attacks.Events(), GameplayEventType::PlayerSwing) == 1u,
          "re-reading sequence N must not repeat the attack");
    attackInput.commands.attack = 2u;
    for (int frame = 0; frame < 40; ++frame)
    {
        attacks.AdvanceFrame(attackInput, 1.0 / 60.0, 2u);
    }
    check(attacks.Snapshot().lastConsumedAttackSequence == 2u &&
          CountEvents(attacks.Events(), GameplayEventType::PlayerSwing) == 2u,
          "N and N+1 must both survive publication and serialize through the sword action");

    const auto swingEvents = attacks.Events().Events();
    std::uint64_t firstSwingSequence = 0u;
    std::uint64_t secondSwingSequence = 0u;
    for (const GameplayEvent& event : swingEvents)
    {
        if (event.type == GameplayEventType::PlayerSwing)
        {
            if (firstSwingSequence == 0u)
            {
                firstSwingSequence = event.sequence;
            }
            else
            {
                secondSwingSequence = event.sequence;
                break;
            }
        }
    }
    check(firstSwingSequence != 0u && secondSwingSequence > firstSwingSequence,
          "serialized swings must remain separate semantic events");

    GameSimulation damageEvents;
    InputSnapshot damageInput;
    damageInput.hasAuthoritativePlayerPose = true;
    damageInput.authoritativePlayerX = 0.0f;
    damageInput.authoritativePlayerZ = -3.40f;
    damageInput.damageEnabled = true;
    for (int frame = 0;
         frame < 600 && damageEvents.Snapshot().playerVitals.phase == PlayerLifePhase::Alive;
         ++frame)
    {
        damageEvents.AdvanceFrame(damageInput, 1.0 / 60.0, static_cast<std::uint64_t>(frame + 1));
    }
    check(damageEvents.Snapshot().playerVitals.phase == PlayerLifePhase::Dying &&
          CountEvents(damageEvents.Events(), GameplayEventType::PlayerDamaged) == 2u &&
          CountEvents(damageEvents.Events(), GameplayEventType::PlayerKilled) == 1u,
          "two nonfatal hits must emit PlayerDamaged while the lethal hit emits only PlayerKilled");

    GameSimulation retry;
    InputSnapshot finaleInput;
    finaleInput.hasAuthoritativePlayerPose = true;
    finaleInput.authoritativePlayerX = -33.70f;
    finaleInput.authoritativePlayerZ = -15.20f;
    finaleInput.yawRadians = -1.57079632679f;
    finaleInput.damageEnabled = false;
    retry.AdvanceFrame(finaleInput, 1.0 / 60.0);
    check(retry.Snapshot().activeEnemyKind == EnemyKind::Lich &&
          retry.Snapshot().retryCheckpoint == 9,
          "entering the finale must select the persistent lich encounter and mirror retry");
    const std::uint32_t lichGeneration = retry.Snapshot().enemyRoster.encounters[1].resetGeneration;

    InputSnapshot lichHitInput = finaleInput;
    lichHitInput.authoritativePlayerX = retry.Snapshot().lich.x;
    lichHitInput.authoritativePlayerZ = retry.Snapshot().lich.z;
    lichHitInput.commands.attack = 1u;
    retry.AdvanceFrame(lichHitInput, 1.0 / 60.0);
    check(retry.Snapshot().lich.health == 2,
          "seam-persistence setup must damage the live lich exactly once");

    InputSnapshot outsideInput = lichHitInput;
    outsideInput.authoritativePlayerX = 50.0f;
    outsideInput.authoritativePlayerZ = 50.0f;
    retry.AdvanceFrame(outsideInput, 1.0 / 60.0);
    InputSnapshot returnInput = finaleInput;
    returnInput.commands.attack = 1u;
    retry.AdvanceFrame(returnInput, 1.0 / 60.0);
    check(retry.Snapshot().activeEnemyKind == EnemyKind::Lich &&
          retry.Snapshot().enemyRoster.encounters[1].resetGeneration == lichGeneration &&
          retry.Snapshot().lich.health == 2 &&
          retry.Snapshot().playerVitals.vitality == PlayerVitals::kMaxVitality,
          "leaving and re-entering an outside seam must not heal or reset a live selected encounter");

    finaleInput.commands.attack = 1u;
    finaleInput.commands.retry = 1u;
    retry.AdvanceFrame(finaleInput, 1.0 / 60.0);
    check(retry.Snapshot().lastConsumedRetrySequence == 1u &&
          retry.Snapshot().retryGeneration == 1u &&
          retry.Snapshot().activeEnemyKind == EnemyKind::Lich &&
          NearlyEqual(retry.Snapshot().playerX, -33.70f) &&
          NearlyEqual(retry.Snapshot().playerZ, -15.20f) &&
          retry.Snapshot().lantern.phase == LanternPhase::Settled &&
          retry.Snapshot().lich.phase != LichPhase::Dormant &&
          retry.Snapshot().playerVitals.vitality == PlayerVitals::kMaxVitality,
          "retry must restore the authored mirror player, lantern, lich, and vitality state exactly once");
    retry.AdvanceFrame(finaleInput, 1.0 / 60.0);
    check(retry.Snapshot().retryGeneration == 1u,
          "re-reading the same retry sequence must not apply a second retry");

    InputSnapshot resetInput = finaleInput;
    resetInput.paused = true;
    resetInput.commands.routeReset = 1u;
    retry.AdvanceFrame(resetInput, 1.0);
    check(retry.Snapshot().lastConsumedRouteResetSequence == 1u &&
          retry.Snapshot().activeEnemyKind == EnemyKind::Skeleton &&
          retry.Snapshot().retryCheckpoint == 0 &&
          NearlyEqual(retry.Snapshot().playerX, kPlayerSpawn.x) &&
          NearlyEqual(retry.Snapshot().playerZ, kPlayerSpawn.z) &&
          NearlyEqual(retry.Snapshot().playerPitchRadians, 0.0f),
          "a paused route reset must restore the live opening pose with zero pitch once");

    GameSimulation resetParity;
    check(resetParity.ApplyShowcaseCheckpoint(0) &&
          NearlyEqual(resetParity.Snapshot().playerPitchRadians, -0.05f) &&
          resetParity.Snapshot().swordCombat.enemyAnimation == EnemyAnimation::Walking &&
          NearlyEqual(resetParity.Snapshot().swordCombat.enemyAnimationTime, 0.0f) &&
          resetParity.Snapshot().tickIndex == 0u &&
          resetParity.Events().Empty(),
          "exact checkpoint 0 import must retain capture pitch and zero-time walking renderer state without a tick or event");
    resetParity.ResetRoute();
    check(NearlyEqual(resetParity.Snapshot().playerYawRadians, 0.0f) &&
          NearlyEqual(resetParity.Snapshot().playerPitchRadians, 0.0f),
          "live ResetRoute must override checkpoint pose with the configured zero yaw and pitch");

    GameSimulation mirrorCapture;
    check(mirrorCapture.ApplyShowcaseCheckpoint(9),
          "mirror checkpoint import must succeed");
    const float mirrorFacing = std::atan2(mirrorCapture.Snapshot().playerX - mirrorCapture.Snapshot().lich.x,
                                          mirrorCapture.Snapshot().playerZ - mirrorCapture.Snapshot().lich.z);
    check(mirrorCapture.Snapshot().activeEnemyKind == EnemyKind::Lich &&
          NearlyEqual(mirrorCapture.Snapshot().lich.facingRadians, mirrorFacing) &&
          mirrorCapture.Snapshot().tickIndex == 0u &&
          mirrorCapture.Events().Empty(),
          "mirror import must finalize zero-delta lich facing without a tick or event");

    GameSimulation finaleRoofCapture;
    check(finaleRoofCapture.ApplyShowcaseCheckpoint(11) &&
          finaleRoofCapture.Snapshot().lich.phase == LichPhase::Dead &&
          finaleRoofCapture.Snapshot().lich.deathAnimationComplete &&
          NearlyEqual(finaleRoofCapture.Snapshot().lich.finaleSkylightOpenProgress, 1.0f, 0.003f) &&
          finaleRoofCapture.Snapshot().tickIndex == 0u &&
          finaleRoofCapture.Events().Empty(),
          "finale-roof zero-delta finalization must preserve the authored dead/open-roof state without a tick or event");

    GameSimulation pausedRetry;
    InputSnapshot pausedFinale = finaleInput;
    pausedFinale.commands = {};
    pausedRetry.AdvanceFrame(pausedFinale, 1.0 / 60.0);
    InputSnapshot queuedSwing = pausedFinale;
    queuedSwing.commands.attack = 1u;
    pausedRetry.AdvanceFrame(queuedSwing, 1.0 / 60.0);
    check(!pausedRetry.Events().Empty(), "pre-retry semantic events must exist for stale-event coverage");
    InputSnapshot pausedRetryInput = pausedFinale;
    pausedRetryInput.paused = true;
    pausedRetryInput.commands.attack = 1u;
    pausedRetryInput.commands.retry = 1u;
    pausedRetry.AdvanceFrame(pausedRetryInput, 1.0);
    check(pausedRetry.Snapshot().lastConsumedRetrySequence == 1u &&
          pausedRetry.Snapshot().retryGeneration == 1u &&
          pausedRetry.Events().Empty() &&
          NearlyEqual(pausedRetry.Snapshot().playerX, -33.70f) &&
          NearlyEqual(pausedRetry.Snapshot().playerZ, -15.20f),
          "paused retry must apply immediately, discard stale events, and clear catch-up time");

    if (!passed)
    {
        return 1;
    }
    std::cout << "Shared simulation command, event, seam-persistence, and retry tests passed.\n";
    return 0;
}
