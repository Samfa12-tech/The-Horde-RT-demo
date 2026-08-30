#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "gameplay/simulation/BoundedTransportQueue.h"
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
    first.listenerX = -1.0f;
    first.listenerZ = 2.0f;
    first.listenerYawRadians = 0.25f;
    GameplayEvent second = first;
    second.target = EntityId::Lich;
    second.worldX = 2.0f;
    second.listenerX = -2.0f;
    second.listenerYawRadians = -0.50f;
    check(identityQueue.Push(first) && identityQueue.Push(second),
          "two same-type events must fit the bounded queue");
    check(identityQueue[0].sequence != identityQueue[1].sequence &&
          identityQueue[0].target == EntityId::Skeleton &&
          identityQueue[1].target == EntityId::Lich &&
          identityQueue[0].worldX != identityQueue[1].worldX &&
          identityQueue[0].listenerX != identityQueue[1].listenerX &&
          identityQueue[0].listenerYawRadians != identityQueue[1].listenerYawRadians,
          "same-type events must retain distinct sequences, entities, positions, and listener state");
    for (std::size_t i = identityQueue.Size(); i < BoundedGameplayEventQueue::kCapacity; ++i)
    {
        identityQueue.Push({});
    }
    check(!identityQueue.Push({}) && identityQueue.OverflowCount() == 1u &&
          identityQueue.Size() == BoundedGameplayEventQueue::kCapacity &&
          identityQueue.HighWaterMark() == BoundedGameplayEventQueue::kCapacity &&
          identityQueue.NextSequence() == BoundedGameplayEventQueue::kCapacity + 1u &&
          identityQueue[0].source == EntityId::Player &&
          identityQueue[0].target == EntityId::Skeleton &&
          NearlyEqual(identityQueue[0].listenerX, -1.0f),
          "overflow must be explicit and must retain the ordered event identity without advancing sequence");

    BoundedTransportQueue<GameplayEvent, 2u> platformQueue;
    check(platformQueue.Push(first) && platformQueue.Push(second) &&
          platformQueue.Size() == 2u && platformQueue.HighWaterMark() == 2u,
          "platform transport queue must retain bounded publications in order");
    check(!platformQueue.Push({}) && platformQueue.OverflowCount() == 1u &&
          platformQueue.Size() == 2u && platformQueue[0].target == EntityId::Skeleton &&
          platformQueue[1].target == EntityId::Lich &&
          NearlyEqual(platformQueue[0].listenerX, -1.0f) &&
          NearlyEqual(platformQueue[1].listenerX, -2.0f),
          "platform transport overflow must be visible and must not overwrite queued events");
    platformQueue.Clear();
    check(platformQueue.Size() == 0u && platformQueue.OverflowCount() == 1u &&
          platformQueue.HighWaterMark() == 2u,
          "platform transport drain must clear entries without hiding overflow diagnostics");

    GameSimulationConfig listenerConfig;
    listenerConfig.movementSpeedMetresPerSecond = 30.0f;
    GameSimulation movingListeners(listenerConfig);
    InputSnapshot movingListenerInput;
    movingListenerInput.moveForward = 1.0f;
    movingListenerInput.yawRadians = 0.15f;
    movingListenerInput.damageEnabled = false;
    const std::uint32_t listenerTicks = movingListeners.AdvanceFrame(
        movingListenerInput,
        0.100,
        101u);
    std::size_t movingFootstepCount = 0u;
    std::uint64_t previousFootstepSequence = 0u;
    bool movingFootstepsAreOrdered = true;
    bool movingFootstepsCaptureCurrentListener = true;
    bool atLeastOneListenerDiffersFromFrameEnd = false;
    for (const GameplayEvent& event : movingListeners.Events().Events())
    {
        if (event.type != GameplayEventType::PlayerFootstep)
        {
            continue;
        }
        ++movingFootstepCount;
        movingFootstepsAreOrdered = movingFootstepsAreOrdered &&
            event.sequence > previousFootstepSequence;
        movingFootstepsCaptureCurrentListener = movingFootstepsCaptureCurrentListener &&
            event.source == EntityId::Player &&
            event.target == EntityId::Invalid &&
            NearlyEqual(event.listenerX, event.worldX) &&
            NearlyEqual(event.listenerZ, event.worldZ) &&
            NearlyEqual(event.listenerYawRadians, movingListenerInput.yawRadians);
        atLeastOneListenerDiffersFromFrameEnd = atLeastOneListenerDiffersFromFrameEnd ||
            !NearlyEqual(event.listenerX, movingListeners.Snapshot().playerX) ||
            !NearlyEqual(event.listenerZ, movingListeners.Snapshot().playerZ);
        previousFootstepSequence = event.sequence;
    }
    check(listenerTicks >= 3u && movingFootstepCount >= 2u && movingFootstepsAreOrdered &&
          movingFootstepsCaptureCurrentListener && atLeastOneListenerDiffersFromFrameEnd,
          "events from several fixed ticks in one moving frame must retain each contact's listener state");

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
          "a natural second press during downward wind-up must buffer one upward slice");

    attackInput.commands.attack = 3u;
    for (int frame = 0; frame < 5; ++frame)
    {
        attacks.AdvanceFrame(attackInput, 1.0 / 60.0, 3u);
    }
    check(attacks.Snapshot().lastConsumedAttackSequence == 3u &&
          CountEvents(attacks.Events(), GameplayEventType::PlayerSwing) == 2u,
          "a third edge during the committed upward action must be consumed without replay");

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
          "buffered downward/upward swings must publish two ordered semantic events");
    check(std::all_of(swingEvents.begin(), swingEvents.end(), [](const GameplayEvent& event)
          {
              return event.type != GameplayEventType::PlayerSwing || event.target == EntityId::Invalid;
          }),
          "an out-of-range player swing must not falsely claim a skeleton target");

    GameSimulation chainedAttack;
    InputSnapshot chainedInput;
    chainedInput.hasAuthoritativePlayerPose = true;
    chainedInput.authoritativePlayerX = 0.0f;
    chainedInput.authoritativePlayerZ = -3.20f;
    chainedInput.damageEnabled = false;
    chainedInput.commands.attack = 1u;
    chainedAttack.StepFixed(chainedInput);
    while (chainedAttack.Snapshot().playerCombat.action != PlayerCombatAction::SwingActive)
    {
        chainedAttack.StepFixed(chainedInput);
    }
    chainedInput.commands.attack = 2u;
    chainedAttack.StepFixed(chainedInput);
    for (int tick = 0; tick < 60; ++tick)
    {
        chainedAttack.StepFixed(chainedInput);
    }
    std::size_t chainedSwingCount = 0u;
    std::size_t chainedHitCount = 0u;
    std::array<std::int32_t, 2u> chainedSwingPayloads{};
    for (const GameplayEvent& event : chainedAttack.Events().Events())
    {
        if (event.type == GameplayEventType::PlayerSwing && chainedSwingCount < 2u)
        {
            chainedSwingPayloads[chainedSwingCount++] = event.payload;
        }
        if (event.type == GameplayEventType::EnemyHit) ++chainedHitCount;
    }
    check(chainedAttack.Snapshot().lastConsumedAttackSequence == 2u &&
          chainedSwingCount == 2u && chainedSwingPayloads[0] == 1 &&
          chainedSwingPayloads[1] == 2 && chainedHitCount == 2u &&
          chainedAttack.Snapshot().openingEncounterComplete,
          "downward then upward cut must consume two edges and publish one identified swing/hit per cut");

    GameSimulation ownerTimedChainedAttack;
    InputSnapshot ownerTimedChainedInput;
    ownerTimedChainedInput.damageEnabled = false;
    ownerTimedChainedInput.commands.attack = 1u;
    // Reproduce the real input trace: the owner's second click arrives about
    // 400 ms after the first, after the 160 ms downstroke has visibly landed.
    for (int tick = 0; tick < 24; ++tick)
    {
        ownerTimedChainedAttack.StepFixed(ownerTimedChainedInput);
    }
    ownerTimedChainedInput.commands.attack = 2u;
    ownerTimedChainedAttack.StepFixed(ownerTimedChainedInput);
    for (int tick = 0; tick < 70; ++tick)
    {
        ownerTimedChainedAttack.StepFixed(ownerTimedChainedInput);
    }
    check(ownerTimedChainedAttack.Snapshot().lastConsumedAttackSequence == 2u &&
          CountEvents(ownerTimedChainedAttack.Events(), GameplayEventType::PlayerSwing) == 2u,
          "a natural 400 ms second press after the downstroke lands must produce the upward slice");

    GameSimulation coalescedAttack;
    InputSnapshot coalescedInput;
    coalescedInput.hasAuthoritativePlayerPose = true;
    coalescedInput.authoritativePlayerX = 0.0f;
    coalescedInput.authoritativePlayerZ = -3.20f;
    coalescedInput.damageEnabled = false;
    coalescedInput.commands.attack = 2u;
    for (int tick = 0; tick < 70; ++tick)
    {
        coalescedAttack.StepFixed(coalescedInput);
    }
    check(coalescedAttack.Snapshot().lastConsumedAttackSequence == 2u &&
          CountEvents(coalescedAttack.Events(), GameplayEventType::PlayerSwing) == 2u &&
          CountEvents(coalescedAttack.Events(), GameplayEventType::EnemyHit) == 2u,
          "a coalesced 0-to-2 publication must preserve both downward and upward edges");

    GameSimulation pausedChain;
    InputSnapshot pausedChainInput;
    pausedChainInput.damageEnabled = false;
    pausedChainInput.commands.attack = 1u;
    pausedChain.StepFixed(pausedChainInput);
    while (pausedChain.Snapshot().playerCombat.action != PlayerCombatAction::SwingActive)
    {
        pausedChain.StepFixed(pausedChainInput);
    }
    pausedChainInput.paused = true;
    pausedChainInput.commands.attack = 2u;
    pausedChain.StepFixed(pausedChainInput);
    pausedChainInput.paused = false;
    pausedChain.StepFixed(pausedChainInput);
    check(pausedChain.Snapshot().lastConsumedAttackSequence == 2u &&
          CountEvents(pausedChain.Events(), GameplayEventType::PlayerSwing) == 1u,
          "a chained edge consumed while paused must not replay or duplicate after resume");

    const auto deliverChainedAttack = [](const int renderRate)
    {
        GameSimulation delivered;
        InputSnapshot input;
        input.hasAuthoritativePlayerPose = true;
        input.authoritativePlayerX = 0.0f;
        input.authoritativePlayerZ = -3.20f;
        input.damageEnabled = false;
        input.commands.attack = 1u;
        bool secondEdgePublished = false;
        for (int frame = 0; frame < renderRate * 2; ++frame)
        {
            if (!secondEdgePublished && frame == renderRate * 2 / 5)
            {
                input.commands.attack = 2u;
                secondEdgePublished = true;
            }
            delivered.AdvanceFrame(input, 1.0 / static_cast<double>(renderRate));
        }
        return std::array<std::uint64_t, 5u>{
            delivered.Snapshot().tickIndex,
            delivered.Snapshot().lastConsumedAttackSequence,
            static_cast<std::uint64_t>(delivered.Snapshot().playerCombat.action),
            CountEvents(delivered.Events(), GameplayEventType::PlayerSwing),
            CountEvents(delivered.Events(), GameplayEventType::EnemyHit)};
    };
    const auto chainedAt30 = deliverChainedAttack(30);
    const auto chainedAt60 = deliverChainedAttack(60);
    const auto chainedAt120 = deliverChainedAttack(120);
    check(chainedAt30 == chainedAt60 && chainedAt60 == chainedAt120 &&
          chainedAt60[0] == 120u && chainedAt60[1] == 2u &&
          chainedAt60[2] == static_cast<std::uint64_t>(PlayerCombatAction::Idle) &&
          chainedAt60[3] == 2u && chainedAt60[4] == 2u,
          "30/60/120 render delivery must preserve the same owner-timed 400 ms two-cut commands, hits, and final phase");

    GameSimulation skeletonPair;
    InputSnapshot pairInput;
    pairInput.hasAuthoritativePlayerPose = true;
    pairInput.authoritativePlayerX = 0.0f;
    pairInput.authoritativePlayerZ = -2.85f;
    pairInput.yawRadians = 0.0f;
    pairInput.damageEnabled = false;
    for (int frame = 0; frame < 100; ++frame)
    {
        skeletonPair.AdvanceFrame(pairInput, 1.0 / 60.0);
    }
    check(skeletonPair.Snapshot().skeletonEnemyCount == 2u &&
          skeletonPair.Snapshot().activeSkeletonCount == 2u &&
          skeletonPair.Snapshot().skeletonEnemies[0].id == EntityId::SkeletonA &&
          skeletonPair.Snapshot().skeletonEnemies[1].id == EntityId::SkeletonB &&
          skeletonPair.Snapshot().skeletonAttackerId == EntityId::SkeletonA,
          "the bounded pair must expose stable A/B IDs and choose A on an equal-distance tie");
    check(std::hypot(skeletonPair.Snapshot().skeletonEnemies[1].x -
                     skeletonPair.Snapshot().skeletonEnemies[0].x,
                     skeletonPair.Snapshot().skeletonEnemies[1].z -
                     skeletonPair.Snapshot().skeletonEnemies[0].z) >= 0.699f,
          "the live skeleton pair must retain the deterministic 0.70 m separation");

    const float distanceToA = std::hypot(skeletonPair.Snapshot().skeletonEnemies[0].x -
                                         pairInput.authoritativePlayerX,
                                         skeletonPair.Snapshot().skeletonEnemies[0].z -
                                         pairInput.authoritativePlayerZ);
    const float distanceToB = std::hypot(skeletonPair.Snapshot().skeletonEnemies[1].x -
                                         pairInput.authoritativePlayerX,
                                         skeletonPair.Snapshot().skeletonEnemies[1].z -
                                         pairInput.authoritativePlayerZ);
    const std::size_t expectedFirstTarget = distanceToA <= distanceToB ? 0u : 1u;
    const std::size_t expectedSecondTarget = 1u - expectedFirstTarget;

    pairInput.commands.attack = 1u;
    for (int frame = 0; frame < 40 && skeletonPair.Snapshot().activeSkeletonCount == 2u; ++frame)
    {
        skeletonPair.AdvanceFrame(pairInput, 1.0 / 60.0);
    }
    check(skeletonPair.Snapshot().skeletonEnemies[expectedFirstTarget].dead &&
          !skeletonPair.Snapshot().skeletonEnemies[expectedSecondTarget].dead &&
          skeletonPair.Snapshot().activeSkeletonCount == 1u,
          "one sword action must kill only the nearest valid skeleton");
    const float defeatedX = skeletonPair.Snapshot().skeletonEnemies[expectedFirstTarget].x;
    const float defeatedZ = skeletonPair.Snapshot().skeletonEnemies[expectedFirstTarget].z;
    bool postDeathSeparationHeld = true;
    for (int frame = 0; frame < 120; ++frame)
    {
        skeletonPair.AdvanceFrame(pairInput, 1.0 / 60.0);
        postDeathSeparationHeld = postDeathSeparationHeld &&
            std::hypot(skeletonPair.Snapshot().skeletonEnemies[1].x -
                       skeletonPair.Snapshot().skeletonEnemies[0].x,
                       skeletonPair.Snapshot().skeletonEnemies[1].z -
                       skeletonPair.Snapshot().skeletonEnemies[0].z) >= 0.699f &&
            NearlyEqual(skeletonPair.Snapshot().skeletonEnemies[expectedFirstTarget].x, defeatedX) &&
            NearlyEqual(skeletonPair.Snapshot().skeletonEnemies[expectedFirstTarget].z, defeatedZ);
    }
    check(skeletonPair.Snapshot().skeletonEnemies[expectedFirstTarget].dead &&
          !skeletonPair.Snapshot().skeletonEnemies[expectedSecondTarget].dead &&
          postDeathSeparationHeld &&
          skeletonPair.Snapshot().skeletonAttackerId ==
              skeletonPair.Snapshot().skeletonEnemies[expectedSecondTarget].id,
          "a defeated skeleton must remain fixed while the separated survivor owns the attack token");

    pairInput.commands.attack = 2u;
    for (int frame = 0; frame < 60 && !skeletonPair.Snapshot().openingEncounterComplete; ++frame)
    {
        skeletonPair.AdvanceFrame(pairInput, 1.0 / 60.0);
    }
    check(skeletonPair.Snapshot().openingEncounterComplete &&
          skeletonPair.Snapshot().activeSkeletonCount == 0u &&
          skeletonPair.Snapshot().enemyRoster.encounters[0].status == EncounterStatus::Dead,
          "defeating both stable entities must complete the opening encounter");

    std::array<EntityId, 2> defeatedTargets{};
    std::size_t defeatedTargetCount = 0u;
    std::uint64_t previousDefeatSequence = 0u;
    bool orderedDistinctDefeats = true;
    bool hitImmediatelyPrecedesDefeat = true;
    GameplayEvent previousEvent{};
    for (const GameplayEvent& event : skeletonPair.Events().Events())
    {
        if (event.type == GameplayEventType::EnemyDefeated)
        {
            orderedDistinctDefeats = orderedDistinctDefeats && event.sequence > previousDefeatSequence;
            hitImmediatelyPrecedesDefeat = hitImmediatelyPrecedesDefeat &&
                previousEvent.type == GameplayEventType::EnemyHit &&
                previousEvent.target == event.target && previousEvent.sequence + 1u == event.sequence;
            previousDefeatSequence = event.sequence;
            if (defeatedTargetCount < defeatedTargets.size())
            {
                defeatedTargets[defeatedTargetCount++] = event.target;
            }
        }
        previousEvent = event;
    }
    check(defeatedTargetCount == 2u && orderedDistinctDefeats && hitImmediatelyPrecedesDefeat &&
          defeatedTargets[0] == skeletonPair.Snapshot().skeletonEnemies[expectedFirstTarget].id &&
          defeatedTargets[1] == skeletonPair.Snapshot().skeletonEnemies[expectedSecondTarget].id,
          "each ordered A/B defeat must immediately follow its entity-aware hit event");
    std::size_t pairSwingCount = 0u;
    bool pairSwingsAreTargetless = true;
    for (const GameplayEvent& event : skeletonPair.Events().Events())
    {
        if (event.type == GameplayEventType::PlayerSwing)
        {
            ++pairSwingCount;
            pairSwingsAreTargetless = pairSwingsAreTargetless && event.target == EntityId::Invalid;
        }
    }
    check(pairSwingCount == 2u && pairSwingsAreTargetless,
          "swing-intent events must stay targetless until an actual entity-aware hit resolves");

    GameSimulation parrySimulation;
    InputSnapshot parryInput;
    parryInput.hasAuthoritativePlayerPose = true;
    parryInput.authoritativePlayerX = -0.75f;
    parryInput.authoritativePlayerZ = -3.20f;
    parryInput.yawRadians = 0.0f;
    parryInput.damageEnabled = true;
    bool parryIssued = false;
    for (int frame = 0; frame < 360 &&
         CountEvents(parrySimulation.Events(), GameplayEventType::PlayerParrySucceeded) == 0u;
         ++frame)
    {
        const auto& attacker = parrySimulation.Snapshot().skeletonEnemies[0];
        if (!parryIssued && attacker.action == EnemyCombatAction::AttackWindup &&
            attacker.actionTime >= 0.98f)
        {
            parryInput.commands.parry = 1u;
            parryIssued = true;
        }
        parrySimulation.AdvanceFrame(parryInput, 1.0 / 60.0, frame + 1u);
    }
    const auto parryEvents = parrySimulation.Events().Events();
    const auto parryEvent = std::find_if(parryEvents.begin(), parryEvents.end(), [](const GameplayEvent& event)
    {
        return event.type == GameplayEventType::PlayerParrySucceeded;
    });
    check(parrySimulation.Snapshot().lastConsumedParrySequence == 1u &&
          parryEvent != parryEvents.end() &&
          parryEvent->source == EntityId::Player &&
          parryEvent->target == EntityId::SkeletonA &&
          parrySimulation.Snapshot().playerVitals.vitality == PlayerVitals::kMaxVitality,
          "a successful parry must consume its independent sequence, suppress damage, and emit one entity-aware event");
    for (int frame = 0; frame < 10; ++frame)
    {
        parrySimulation.AdvanceFrame(parryInput, 1.0 / 60.0);
    }
    check(CountEvents(parrySimulation.Events(), GameplayEventType::PlayerParrySucceeded) == 1u,
          "re-reading one parry sequence must not repeat its semantic event");

    InputSnapshot spamParry = parryInput;
    spamParry.commands.parry = 3u;
    parrySimulation.AdvanceFrame(spamParry, 1.0 / 60.0);
    check(parrySimulation.Snapshot().lastConsumedParrySequence == 3u &&
          CountEvents(parrySimulation.Events(), GameplayEventType::PlayerParrySucceeded) == 1u,
          "unavailable parry commands must be consumed without delayed buffering");

    pairInput.commands.retry = 1u;
    skeletonPair.AdvanceFrame(pairInput, 1.0 / 60.0);
    check(skeletonPair.Snapshot().activeSkeletonCount == 2u &&
          !skeletonPair.Snapshot().openingEncounterComplete &&
          NearlyEqual(skeletonPair.Snapshot().skeletonEnemies[0].x, -0.75f) &&
          NearlyEqual(skeletonPair.Snapshot().skeletonEnemies[1].x, 0.75f),
          "encounter retry must restore both skeletons at their authored spawns");

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
    bool playerDamageEventsIdentifyTheirAttacker = true;
    for (const GameplayEvent& event : damageEvents.Events().Events())
    {
        if (event.type == GameplayEventType::PlayerDamaged ||
            event.type == GameplayEventType::PlayerKilled)
        {
            playerDamageEventsIdentifyTheirAttacker = playerDamageEventsIdentifyTheirAttacker &&
                (event.source == EntityId::SkeletonA || event.source == EntityId::SkeletonB) &&
                event.target == EntityId::Player;
        }
    }
    check(damageEvents.Snapshot().playerVitals.phase == PlayerLifePhase::Dying &&
          CountEvents(damageEvents.Events(), GameplayEventType::PlayerDamaged) == 2u &&
          CountEvents(damageEvents.Events(), GameplayEventType::PlayerKilled) == 1u &&
          playerDamageEventsIdentifyTheirAttacker,
          "two entity-aware nonfatal hits must emit PlayerDamaged while the lethal hit emits only PlayerKilled");

    GameSimulation skeletonFeedback;
    InputSnapshot skeletonFeedbackInput;
    skeletonFeedbackInput.damageEnabled = false;
    bool sawEnemyFootstep = false;
    bool sawEnemyAttackStarted = false;
    bool skeletonFeedbackIdentityValid = true;
    std::uint64_t previousSkeletonFeedbackSequence = 0u;
    for (int frame = 0; frame < 900 && (!sawEnemyFootstep || !sawEnemyAttackStarted); ++frame)
    {
        skeletonFeedback.AdvanceFrame(
            skeletonFeedbackInput, 1.0 / 60.0, static_cast<std::uint64_t>(frame + 1));
        for (const GameplayEvent& event : skeletonFeedback.Events().Events())
        {
            skeletonFeedbackIdentityValid = skeletonFeedbackIdentityValid &&
                event.sequence > previousSkeletonFeedbackSequence;
            previousSkeletonFeedbackSequence = event.sequence;
            if (event.type == GameplayEventType::EnemyFootstep)
            {
                sawEnemyFootstep = true;
                skeletonFeedbackIdentityValid = skeletonFeedbackIdentityValid &&
                    (event.source == EntityId::SkeletonA || event.source == EntityId::SkeletonB) &&
                    event.target == EntityId::Invalid;
            }
            if (event.type == GameplayEventType::EnemyAttackStarted)
            {
                sawEnemyAttackStarted = true;
                skeletonFeedbackIdentityValid = skeletonFeedbackIdentityValid &&
                    (event.source == EntityId::SkeletonA || event.source == EntityId::SkeletonB) &&
                    event.target == EntityId::Player;
            }
        }
        skeletonFeedback.ClearEvents();
    }
    check(sawEnemyFootstep && sawEnemyAttackStarted && skeletonFeedbackIdentityValid,
          "walking and attacking skeletons must emit ordered entity-aware footstep and attack events");

    GameSimulation lichFeedback;
    check(lichFeedback.ApplyShowcaseCheckpoint(9),
          "mirror checkpoint import must initialise lich feedback coverage");
    InputSnapshot lichFeedbackInput;
    lichFeedbackInput.damageEnabled = false;
    bool sawLichCharge = false;
    bool sawLichImpact = false;
    bool lichFeedbackIdentityValid = true;
    std::uint64_t chargeSequence = 0u;
    std::uint64_t impactSequence = 0u;
    for (int frame = 0; frame < 900 && (!sawLichCharge || !sawLichImpact); ++frame)
    {
        lichFeedback.AdvanceFrame(
            lichFeedbackInput, 1.0 / 60.0, static_cast<std::uint64_t>(frame + 1));
        for (const GameplayEvent& event : lichFeedback.Events().Events())
        {
            if (event.type == GameplayEventType::LichChargeStarted)
            {
                sawLichCharge = true;
                chargeSequence = event.sequence;
                lichFeedbackIdentityValid = lichFeedbackIdentityValid &&
                    event.source == EntityId::Lich && event.target == EntityId::Player &&
                    event.intensity > 0.0f;
            }
            if (event.type == GameplayEventType::LichImpact)
            {
                sawLichImpact = true;
                impactSequence = event.sequence;
                lichFeedbackIdentityValid = lichFeedbackIdentityValid &&
                    event.source == EntityId::Lich && event.target == EntityId::Player;
            }
        }
        lichFeedback.ClearEvents();
    }
    check(sawLichCharge && sawLichImpact && chargeSequence < impactSequence &&
          lichFeedbackIdentityValid,
          "the lich must emit an ordered entity-aware charge then impact sequence");

    GameSimulation lichDefeatFeedback;
    check(lichDefeatFeedback.ApplyShowcaseCheckpoint(10),
          "lich checkpoint import must initialise defeat-event coverage");
    InputSnapshot lichDefeatInput;
    lichDefeatInput.damageEnabled = false;
    std::uint64_t lichAttackCommand = 0u;
    std::size_t lichHitEventCount = 0u;
    std::size_t lichDefeatedEventCount = 0u;
    std::size_t chestUnlockedEventCount = 0u;
    std::uint64_t lichDefeatedSequence = 0u;
    std::uint64_t chestUnlockedSequence = 0u;
    bool lichDefeatIdentityAndOrderValid = true;
    for (int frame = 0; frame < 1200 && lichDefeatFeedback.Snapshot().lich.health > 0; ++frame)
    {
        const auto& before = lichDefeatFeedback.Snapshot();
        lichDefeatInput.hasAuthoritativePlayerPose = true;
        lichDefeatInput.authoritativePlayerX = before.lich.x;
        lichDefeatInput.authoritativePlayerZ = before.lich.z;
        if (before.playerCombat.action == PlayerCombatAction::Idle &&
            before.lich.hitCooldownRemaining <= 0.00001f)
        {
            lichDefeatInput.commands.attack = ++lichAttackCommand;
        }
        lichDefeatFeedback.AdvanceFrame(
            lichDefeatInput, 1.0 / 60.0, static_cast<std::uint64_t>(frame + 1));
        GameplayEventType previousType = GameplayEventType::PlayerFootstep;
        bool hasPrevious = false;
        for (const GameplayEvent& event : lichDefeatFeedback.Events().Events())
        {
            if (event.type == GameplayEventType::EnemyHit && event.target == EntityId::Lich)
            {
                ++lichHitEventCount;
                lichDefeatIdentityAndOrderValid = lichDefeatIdentityAndOrderValid &&
                    event.source == EntityId::Player;
            }
            if (event.type == GameplayEventType::LichDefeated)
            {
                ++lichDefeatedEventCount;
                lichDefeatedSequence = event.sequence;
                lichDefeatIdentityAndOrderValid = lichDefeatIdentityAndOrderValid &&
                    hasPrevious && previousType == GameplayEventType::EnemyHit &&
                    event.source == EntityId::Player && event.target == EntityId::Lich;
            }
            if (event.type == GameplayEventType::ChestUnlocked)
            {
                ++chestUnlockedEventCount;
                chestUnlockedSequence = event.sequence;
                lichDefeatIdentityAndOrderValid = lichDefeatIdentityAndOrderValid &&
                    event.source == EntityId::Lich && event.target == EntityId::RewardChest;
            }
            previousType = event.type;
            hasPrevious = true;
        }
        lichDefeatFeedback.ClearEvents();
    }
    check(lichDefeatFeedback.Snapshot().chestReward.phase ==
              horde::gameplay::interactions::ChestRewardPhase::Locked &&
          lichDefeatFeedback.Snapshot().chestReward.unlockPending &&
          chestUnlockedEventCount == 0u,
          "the lethal tick must arm, but not emit, the two-second chest unlock");
    for (int frame = 0; frame < 118; ++frame)
    {
        lichDefeatFeedback.AdvanceFrame(lichDefeatInput, 1.0 / 60.0);
        for (const GameplayEvent& event : lichDefeatFeedback.Events().Events())
        {
            if (event.type == GameplayEventType::LichDefeated) ++lichDefeatedEventCount;
            if (event.type == GameplayEventType::ChestUnlocked) ++chestUnlockedEventCount;
        }
        lichDefeatFeedback.ClearEvents();
    }
    check(lichDefeatFeedback.Snapshot().chestReward.phase ==
              horde::gameplay::interactions::ChestRewardPhase::Locked &&
          lichDefeatFeedback.Snapshot().chestReward.unlockPending &&
          chestUnlockedEventCount == 0u,
          "the chest must remain locked through 1.983 fixed seconds after the lethal tick begins");
    lichDefeatFeedback.AdvanceFrame(lichDefeatInput, 1.0 / 60.0);
    for (const GameplayEvent& event : lichDefeatFeedback.Events().Events())
    {
        if (event.type == GameplayEventType::LichDefeated) ++lichDefeatedEventCount;
        if (event.type == GameplayEventType::ChestUnlocked)
        {
            ++chestUnlockedEventCount;
            chestUnlockedSequence = event.sequence;
            lichDefeatIdentityAndOrderValid = lichDefeatIdentityAndOrderValid &&
                event.source == EntityId::Lich &&
                event.target == EntityId::RewardChest;
        }
    }
    lichDefeatFeedback.ClearEvents();
    std::cout << "lich/chest delayed finale events hits/defeat/unlock="
              << lichHitEventCount << '/' << lichDefeatedEventCount << '/'
              << chestUnlockedEventCount << " phase="
              << static_cast<int>(lichDefeatFeedback.Snapshot().chestReward.phase)
              << " pending="
              << lichDefeatFeedback.Snapshot().chestReward.unlockPending
              << " time="
              << lichDefeatFeedback.Snapshot().chestReward.phaseTime
              << " sequence=" << lichDefeatedSequence << '/'
              << chestUnlockedSequence << '\n';
    check(lichDefeatFeedback.Snapshot().lich.health == 0 && lichHitEventCount == 3u &&
          lichDefeatedEventCount == 1u && chestUnlockedEventCount == 1u &&
          lichDefeatFeedback.Snapshot().chestReward.phase ==
              horde::gameplay::interactions::ChestRewardPhase::ClosedUnlocked &&
          lichDefeatIdentityAndOrderValid &&
          chestUnlockedSequence > lichDefeatedSequence,
          "three accepted hits must emit one lich defeat followed exactly two fixed seconds later by one chest unlock");

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
    check(retry.Snapshot().lich.health == 3,
          "the lich must not take damage on the swing input edge");
    for (int frame = 0; frame < 20 && retry.Snapshot().lich.health == 3; ++frame)
    {
        retry.AdvanceFrame(lichHitInput, 1.0 / 60.0);
    }
    check(retry.Snapshot().lich.health == 2,
          "the live lich must take exactly one hit when the sword enters its active window");

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
          retry.Snapshot().torchFailure.phase == TorchFailurePhase::Settled &&
          retry.Snapshot().lich.phase != LichPhase::Dormant &&
          retry.Snapshot().playerVitals.vitality == PlayerVitals::kMaxVitality,
          "retry must restore the authored mirror player, torch failure, lich, and vitality state exactly once");
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

    GameSimulation deadRejectsParry;
    InputSnapshot lethalInput;
    lethalInput.hasAuthoritativePlayerPose = true;
    lethalInput.authoritativePlayerX = -0.75f;
    lethalInput.authoritativePlayerZ = -3.40f;
    lethalInput.damageEnabled = true;
    for (int frame = 0; frame < 700 && deadRejectsParry.Snapshot().playerAlive; ++frame)
    {
        deadRejectsParry.AdvanceFrame(lethalInput, 1.0 / 60.0);
    }
    lethalInput.commands.parry = 1u;
    deadRejectsParry.AdvanceFrame(lethalInput, 1.0 / 60.0);
    check(deadRejectsParry.Snapshot().lastConsumedParrySequence == 1u &&
          deadRejectsParry.Snapshot().playerCombat.action == PlayerCombatAction::Idle,
          "death-state parry input must be consumed without starting or buffering an action");

    GameSimulation pausedRejectsParry;
    InputSnapshot pausedParryInput;
    pausedParryInput.paused = true;
    pausedParryInput.commands.parry = 1u;
    pausedRejectsParry.StepFixed(pausedParryInput);
    pausedParryInput.paused = false;
    pausedRejectsParry.StepFixed(pausedParryInput);
    check(pausedRejectsParry.Snapshot().lastConsumedParrySequence == 1u &&
          pausedRejectsParry.Snapshot().playerCombat.action == PlayerCombatAction::Idle,
          "paused parry input must be consumed without starting after resume");

    GameSimulation directionalDodge;
    InputSnapshot dodgeInput;
    dodgeInput.damageEnabled = false;
    dodgeInput.moveForward = 1.0f;
    dodgeInput.commands.dodge = 1u;
    directionalDodge.StepFixed(dodgeInput);
    dodgeInput.moveForward = 0.0f;
    for (int tick = 1; tick < 12; ++tick)
    {
        directionalDodge.StepFixed(dodgeInput);
    }
    const float forwardDodgeDistance = std::hypot(
        directionalDodge.Snapshot().playerX - kPlayerSpawn.x,
        directionalDodge.Snapshot().playerZ - kPlayerSpawn.z);
    check(directionalDodge.Snapshot().lastConsumedDodgeSequence == 1u &&
          forwardDodgeDistance > 0.82f && forwardDodgeDistance < 0.98f &&
          directionalDodge.Snapshot().playerZ < kPlayerSpawn.z,
          "one dodge command must latch the left-stick direction and travel a bounded distance");
    const float settledDodgeZ = directionalDodge.Snapshot().playerZ;
    for (int tick = 0; tick < 12; ++tick)
    {
        directionalDodge.StepFixed(dodgeInput);
    }
    check(NearlyEqual(directionalDodge.Snapshot().playerZ, settledDodgeZ) &&
          directionalDodge.Snapshot().lastConsumedDodgeSequence == 1u,
          "re-reading one dodge sequence must not repeat movement");

    GameSimulation diagonalDodge;
    InputSnapshot diagonalDodgeInput;
    diagonalDodgeInput.damageEnabled = false;
    diagonalDodgeInput.moveForward = 1.0f;
    diagonalDodgeInput.moveStrafe = 1.0f;
    diagonalDodgeInput.commands.dodge = 1u;
    diagonalDodge.StepFixed(diagonalDodgeInput);
    diagonalDodgeInput.moveForward = 0.0f;
    diagonalDodgeInput.moveStrafe = 0.0f;
    for (int tick = 1; tick < 12; ++tick)
    {
        diagonalDodge.StepFixed(diagonalDodgeInput);
    }
    const float diagonalDistance = std::hypot(
        diagonalDodge.Snapshot().playerX - kPlayerSpawn.x,
        diagonalDodge.Snapshot().playerZ - kPlayerSpawn.z);
    check(diagonalDistance > 0.82f && diagonalDistance < 0.98f,
          "diagonal left-stick dodge direction must be normalized");

    GameSimulation neutralDodge;
    InputSnapshot neutralDodgeInput;
    neutralDodgeInput.damageEnabled = false;
    neutralDodgeInput.yawRadians = 1.57079632679f;
    neutralDodgeInput.commands.dodge = 1u;
    for (int tick = 0; tick < 12; ++tick)
    {
        neutralDodge.StepFixed(neutralDodgeInput);
    }
    check(neutralDodge.Snapshot().playerX > kPlayerSpawn.x + 0.82f &&
          std::abs(neutralDodge.Snapshot().playerZ - kPlayerSpawn.z) < 0.04f,
          "neutral-stick dodge must fall back to current facing");

    GameSimulation collisionDodge(GameSimulationConfig{.playerStartX = 1.65f,
                                                        .playerStartZ = 1.85f,
                                                        .playerStartYawRadians = 0.0f});
    InputSnapshot collisionDodgeInput;
    collisionDodgeInput.damageEnabled = false;
    collisionDodgeInput.moveStrafe = 1.0f;
    collisionDodgeInput.commands.dodge = 1u;
    for (int tick = 0; tick < 12; ++tick)
    {
        collisionDodge.StepFixed(collisionDodgeInput);
        collisionDodgeInput.moveStrafe = 0.0f;
    }
    check(IsShowcasePlayerPositionWalkable(collisionDodge.Snapshot().playerX,
                                           collisionDodge.Snapshot().playerZ) &&
          collisionDodge.Snapshot().playerX < 1.85f,
          "dodge displacement must remain inside the shared corridor collision route");

    GameSimulation pausedRejectsDodge;
    InputSnapshot pausedDodgeInput;
    pausedDodgeInput.paused = true;
    pausedDodgeInput.commands.dodge = 1u;
    pausedRejectsDodge.StepFixed(pausedDodgeInput);
    pausedDodgeInput.paused = false;
    pausedRejectsDodge.StepFixed(pausedDodgeInput);
    check(pausedRejectsDodge.Snapshot().lastConsumedDodgeSequence == 1u &&
          !pausedRejectsDodge.Snapshot().dodgeActive &&
          NearlyEqual(pausedRejectsDodge.Snapshot().playerX, kPlayerSpawn.x) &&
          NearlyEqual(pausedRejectsDodge.Snapshot().playerZ, kPlayerSpawn.z),
          "paused dodge input must be consumed without buffering movement after resume");

    GameSimulation torchDrenchFeedback;
    InputSnapshot drenchedTorchInput;
    drenchedTorchInput.hasAuthoritativePlayerPose = true;
    drenchedTorchInput.authoritativePlayerX = -2.10f;
    drenchedTorchInput.authoritativePlayerZ = -15.20f;
    drenchedTorchInput.damageEnabled = false;
    torchDrenchFeedback.StepFixed(drenchedTorchInput);
    check(CountEvents(torchDrenchFeedback.Events(),
                      GameplayEventType::TorchExtinguished) == 1u &&
              torchDrenchFeedback.Snapshot().torchFailure.triggered,
          "entering roof water must emit one shared positional torch-extinguish cue");
    bool extinguishUsesTriggerPosition = false;
    for (const GameplayEvent& event : torchDrenchFeedback.Events().Events())
    {
        if (event.type == GameplayEventType::TorchExtinguished)
        {
            extinguishUsesTriggerPosition =
                NearlyEqual(event.worldX, -2.10f) &&
                NearlyEqual(event.worldZ, -15.20f) &&
                NearlyEqual(event.listenerX, -2.10f) &&
                NearlyEqual(event.listenerZ, -15.20f);
        }
    }
    check(extinguishUsesTriggerPosition,
          "torch-extinguish audio must retain exact event-time source/listener state");
    torchDrenchFeedback.ClearEvents();
    for (int tick = 0; tick < 120; ++tick)
    {
        torchDrenchFeedback.StepFixed(drenchedTorchInput);
    }
    check(CountEvents(torchDrenchFeedback.Events(),
                      GameplayEventType::TorchExtinguished) == 0u,
          "guttering, drop, settle, and repeated polling must not duplicate the extinguish cue");
    drenchedTorchInput.paused = true;
    torchDrenchFeedback.StepFixed(drenchedTorchInput);
    drenchedTorchInput.paused = false;
    torchDrenchFeedback.StepFixed(drenchedTorchInput);
    check(CountEvents(torchDrenchFeedback.Events(),
                      GameplayEventType::TorchExtinguished) == 0u,
          "pause/resume must not replay the already-consumed extinguish cue");

    GameSimulation resetParity;
    check(resetParity.ApplyShowcaseCheckpoint(0) &&
          NearlyEqual(resetParity.Snapshot().playerPitchRadians, -0.05f) &&
          resetParity.Snapshot().skeletonEnemyCount == 1u &&
          resetParity.Snapshot().activeSkeletonCount == 1u &&
          NearlyEqual(resetParity.Snapshot().skeletonEnemies[0].x, 0.0f) &&
          NearlyEqual(resetParity.Snapshot().skeletonEnemies[0].z, -4.65f) &&
          resetParity.Snapshot().swordCombat.enemyAnimation == EnemyAnimation::Walking &&
          NearlyEqual(resetParity.Snapshot().swordCombat.enemyAnimationTime, 0.0f) &&
          resetParity.Snapshot().tickIndex == 0u &&
          resetParity.Events().Empty(),
          "exact checkpoint 0 import must retain capture pitch and zero-time walking renderer state without a tick or event");
    bool historicalCheckpointsRemainSingle = true;
    for (std::int32_t checkpointId = 0; checkpointId < 12; ++checkpointId)
    {
        GameSimulation historicalCapture;
        historicalCheckpointsRemainSingle = historicalCheckpointsRemainSingle &&
            historicalCapture.ApplyShowcaseCheckpoint(checkpointId) &&
            historicalCapture.Snapshot().skeletonEnemyCount == 1u &&
            historicalCapture.Snapshot().activeSkeletonCount == 1u &&
            NearlyEqual(historicalCapture.Snapshot().skeletonEnemies[0].x, 0.0f) &&
            NearlyEqual(historicalCapture.Snapshot().skeletonEnemies[0].z, -4.65f);
    }
    check(historicalCheckpointsRemainSingle,
          "all twelve historical authored checkpoints must retain the original one-skeleton capture state");
    GameSimulation twoEnemyCapture;
    check(twoEnemyCapture.ApplyShowcaseCheckpoint(12) &&
          twoEnemyCapture.Snapshot().activeEnemyKind == EnemyKind::Skeleton &&
          twoEnemyCapture.Snapshot().skeletonEnemyCount == 2u &&
          twoEnemyCapture.Snapshot().activeSkeletonCount == 2u &&
          NearlyEqual(twoEnemyCapture.Snapshot().skeletonEnemies[0].x, -0.75f) &&
          NearlyEqual(twoEnemyCapture.Snapshot().skeletonEnemies[1].x, 0.75f) &&
          twoEnemyCapture.Snapshot().tickIndex == 0u &&
          twoEnemyCapture.Events().Empty(),
          "two-enemy-combat checkpoint import must retain the exact fresh bounded pair without a tick or event");
    resetParity.ResetRoute();
    check(resetParity.Snapshot().skeletonEnemyCount == 2u &&
          resetParity.Snapshot().activeSkeletonCount == 2u &&
          NearlyEqual(resetParity.Snapshot().skeletonEnemies[0].x, -0.75f) &&
          NearlyEqual(resetParity.Snapshot().skeletonEnemies[1].x, 0.75f) &&
          NearlyEqual(resetParity.Snapshot().playerYawRadians, 0.0f) &&
          NearlyEqual(resetParity.Snapshot().playerPitchRadians, 0.0f),
          "live ResetRoute must restore the pair and override checkpoint pose with configured yaw and pitch");

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
          pausedRetry.Snapshot().lastConsumedAttackSequence == 1u &&
          pausedRetry.Snapshot().retryGeneration == 1u &&
          pausedRetry.Events().Empty() &&
          NearlyEqual(pausedRetry.Snapshot().playerX, -33.70f) &&
          NearlyEqual(pausedRetry.Snapshot().playerZ, -15.20f),
          "paused retry must consume the competing attack exactly once, discard stale events, and clear catch-up time");

    if (!passed)
    {
        return 1;
    }
    std::cout << "Shared simulation command, event, seam-persistence, and retry tests passed.\n";
    return 0;
}
