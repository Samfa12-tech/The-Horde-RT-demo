#include <cmath>
#include <cstdint>
#include <iostream>

#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/SwordCombat.h"

int main()
{
    constexpr float dt = 1.0f / 120.0f;
    horde::gameplay::SwordCombat combat;
    const auto initial = combat.Snapshot();
    horde::gameplay::SwordCombat historicalCaptureCombat;
    historicalCaptureCombat.Reset(1u);
    const auto historical = historicalCaptureCombat.Snapshot();
    horde::gameplay::SwordCombat strictNearestCombat;
    const std::int32_t strictNearestTarget = strictNearestCombat.PlayerSwingTargetIndex(
        0.000001f, -3.20f, 0.0f);
    const auto strictAttacker = strictNearestCombat.Update(0.0f, 0.000001f, -3.20f, 0.0f);
    bool sawSwing = false;
    bool sawDeath = false;
    bool deadPersisted = true;
    bool maintainedSeparation = true;
    bool oneAttackToken = true;
    bool requestedCloseAttack = false;
    for (int frame = 0; frame < 1600; ++frame)
    {
        const auto& snapshot = combat.Update(dt, 0.0f, -0.8f, 0.0f);
        if (!requestedCloseAttack && std::abs(snapshot.enemyZ - (-0.8f)) <= 1.58f)
        {
            combat.RequestAttack();
            requestedCloseAttack = true;
        }
        sawSwing = sawSwing || std::abs(snapshot.swordSwingRadians) > 0.1f;
        if (snapshot.enemyAnimation == horde::gameplay::EnemyAnimation::Dead)
        {
            sawDeath = true;
        }
        if (sawDeath)
        {
            deadPersisted = deadPersisted && snapshot.combatants[0].health == 0;
        }
        if (snapshot.combatants[0].health > 0 && snapshot.combatants[1].health > 0)
        {
            maintainedSeparation = maintainedSeparation &&
                std::hypot(snapshot.combatants[1].x - snapshot.combatants[0].x,
                           snapshot.combatants[1].z - snapshot.combatants[0].z) >= 0.699f;
        }
        const int attacking = static_cast<int>(snapshot.combatants[0].animation ==
                                               horde::gameplay::EnemyAnimation::Attack) +
                              static_cast<int>(snapshot.combatants[1].animation ==
                                               horde::gameplay::EnemyAnimation::Attack);
        oneAttackToken = oneAttackToken && attacking <= 1;
    }
    const bool singleTargetHit = combat.Snapshot().combatants[0].health == 0 &&
                                 combat.Snapshot().combatants[1].health == 1 &&
                                 combat.Snapshot().aliveCount == 1u;

    combat.RequestAttack();
    for (int frame = 0; frame < 240 && !combat.Snapshot().encounterComplete; ++frame)
    {
        combat.Update(dt, 0.0f, -0.8f, 0.0f);
    }
    const bool clearedPair = combat.Snapshot().encounterComplete && combat.Snapshot().aliveCount == 0u;
    for (int frame = 0; frame < 600; ++frame)
    {
        combat.Update(dt, 0.0f, -0.8f, 0.0f);
    }
    const bool pairPersistsDead = combat.Snapshot().encounterComplete;
    combat.Reset();
    const bool resetPair = combat.Snapshot().aliveCount == 2u &&
                           !combat.Snapshot().encounterComplete;

    int playerHitPulses = 0;
    bool previousPlayerHitPulse = false;
    bool repeatedPlayerHitPulse = false;
    int acceptedPlayerHits = 0;
    horde::gameplay::PlayerVitals attackedPlayer;
    horde::gameplay::SwordCombat enemyAttack;
    bool sawDamage = false;
    for (int frame = 0; frame < 2000; ++frame)
    {
        attackedPlayer.Update(dt);
        const auto& snapshot = enemyAttack.Update(dt, 0.0f, -0.8f, 0.0f);
        sawDamage = sawDamage || snapshot.damageFlash > 0.5f;
        if (snapshot.playerHitPulse)
        {
            ++playerHitPulses;
            repeatedPlayerHitPulse = repeatedPlayerHitPulse || previousPlayerHitPulse;
            if (attackedPlayer.TryApplyDamage() != horde::gameplay::PlayerDamageResult::Ignored)
            {
                ++acceptedPlayerHits;
            }
        }
        previousPlayerHitPulse = snapshot.playerHitPulse;
    }

    horde::gameplay::SwordCombat leashedEnemy;
    bool stayedInsideArena = true;
    bool idledBeyondDoor = false;
    int idleWalkTransitions = 0;
    auto previousLeashAnimation = leashedEnemy.Snapshot().enemyAnimation;
    for (int frame = 0; frame < 1000; ++frame)
    {
        const auto& snapshot = leashedEnemy.Update(dt, -1.55f, 1.85f, 0.0f);
        const bool wasLocomotion = previousLeashAnimation == horde::gameplay::EnemyAnimation::Idle ||
                                   previousLeashAnimation == horde::gameplay::EnemyAnimation::Walking;
        const bool isLocomotion = snapshot.enemyAnimation == horde::gameplay::EnemyAnimation::Idle ||
                                  snapshot.enemyAnimation == horde::gameplay::EnemyAnimation::Walking;
        if (wasLocomotion && isLocomotion && snapshot.enemyAnimation != previousLeashAnimation)
        {
            ++idleWalkTransitions;
        }
        previousLeashAnimation = snapshot.enemyAnimation;
        for (const auto& skeleton : snapshot.combatants)
        {
            stayedInsideArena = stayedInsideArena &&
                                horde::gameplay::IsSkeletonEnemyPositionWalkable(skeleton.x, skeleton.z);
        }
    }
    const auto beforeLeash = leashedEnemy.Snapshot();
    for (int frame = 0; frame < 240; ++frame)
    {
        const auto& snapshot = leashedEnemy.Update(dt, 0.0f, -7.0f, 0.0f);
        idledBeyondDoor = snapshot.enemyAnimation == horde::gameplay::EnemyAnimation::Idle &&
                          std::abs(snapshot.enemyX - beforeLeash.enemyX) < 0.001f &&
                          std::abs(snapshot.enemyZ - beforeLeash.enemyZ) < 0.001f;
    }

    const bool exactSpawns = std::abs(initial.combatants[0].x - (-0.75f)) < 0.0001f &&
                             std::abs(initial.combatants[0].z - (-4.65f)) < 0.0001f &&
                             std::abs(initial.combatants[1].x - 0.75f) < 0.0001f &&
                             std::abs(initial.combatants[1].z - (-4.65f)) < 0.0001f;
    const bool exactHistoricalSpawn = historical.combatantCount == 1u &&
                                      historical.aliveCount == 1u &&
                                      std::abs(historical.combatants[0].x) < 0.0001f &&
                                      std::abs(historical.combatants[0].z - (-4.65f)) < 0.0001f &&
                                      historical.combatants[1].health == 0;
    const bool strictNearestSelection = strictNearestTarget == 1 && strictAttacker.attackerIndex == 1;
    if (!exactSpawns || !exactHistoricalSpawn || !strictNearestSelection ||
        !sawSwing || !sawDeath || !deadPersisted || !singleTargetHit ||
        !clearedPair || !pairPersistsDead || !resetPair || !maintainedSeparation ||
        !oneAttackToken || !sawDamage ||
        playerHitPulses < horde::gameplay::PlayerVitals::kMaxVitality ||
        acceptedPlayerHits != horde::gameplay::PlayerVitals::kMaxVitality ||
        attackedPlayer.Snapshot().phase != horde::gameplay::PlayerLifePhase::Dead ||
        repeatedPlayerHitPulse || !stayedInsideArena || !idledBeyondDoor || idleWalkTransitions > 2)
    {
        std::cerr << "Combat smoke failed: spawns=" << exactSpawns
                  << " historicalSpawn=" << exactHistoricalSpawn
                  << " strictNearest=" << strictNearestSelection << " swing=" << sawSwing
                  << " death=" << sawDeath << " persistent=" << deadPersisted
                  << " singleTarget=" << singleTargetHit << " cleared=" << clearedPair
                  << " reset=" << resetPair << " separated=" << maintainedSeparation
                  << " oneAttacker=" << oneAttackToken << " damage=" << sawDamage
                  << " enemyCollision=" << stayedInsideArena << " leash=" << idledBeyondDoor
                  << " hitPulses=" << playerHitPulses << " repeatedPulse=" << repeatedPlayerHitPulse
                  << " acceptedPlayerHits=" << acceptedPlayerHits
                  << " idleWalkTransitions=" << idleWalkTransitions << '\n';
        return 1;
    }
    std::cout << "Combat smoke passed: two stable spawns, separation, one attacker, nearest-only hits, "
                 "persistent independent deaths, reset, damage pulses, collision, and route leash.\n";
    return 0;
}
