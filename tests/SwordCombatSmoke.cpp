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

    // Player actions own their exact phase boundaries and resolve contact once.
    horde::gameplay::SwordCombat actionTimeline;
    actionTimeline.RequestAttack();
    for (int tick = 0; tick < 17; ++tick)
    {
        actionTimeline.Update(0.01f, 0.0f, 1.85f, 0.0f);
    }
    const bool swingStillWindup = actionTimeline.Snapshot().player.action ==
        horde::gameplay::PlayerCombatAction::SwingWindup;
    actionTimeline.Update(0.01f, 0.0f, 1.85f, 0.0f);
    const bool swingEnteredActive = actionTimeline.Snapshot().player.action ==
        horde::gameplay::PlayerCombatAction::SwingActive &&
        actionTimeline.Snapshot().playerAttackPulse;
    for (int tick = 0; tick < 16; ++tick)
    {
        actionTimeline.Update(0.01f, 0.0f, 1.85f, 0.0f);
    }
    const bool swingEnteredRecovery = actionTimeline.Snapshot().player.action ==
        horde::gameplay::PlayerCombatAction::SwingRecovery;
    for (int tick = 0; tick < 22; ++tick)
    {
        actionTimeline.Update(0.01f, 0.0f, 1.85f, 0.0f);
    }
    const bool swingFinished = actionTimeline.Snapshot().player.action ==
        horde::gameplay::PlayerCombatAction::Idle;

    // A second attack edge accepted during the first active cut owns a distinct
    // upward slice. Its transition is continuous and each cut publishes one
    // and only one authoritative contact pulse.
    horde::gameplay::SwordCombat comboTimeline;
    const auto firstCut = comboTimeline.RequestAttack();
    int downwardPulses = 0;
    int upwardPulses = 0;
    bool queuedDuringActive = false;
    bool continuousComboTransition = true;
    float previousSwordRadians = comboTimeline.Snapshot().swordSwingRadians;
    horde::gameplay::PlayerCombatAction previousComboAction =
        comboTimeline.Snapshot().player.action;
    for (int tick = 0; tick < 140; ++tick)
    {
        const auto& before = comboTimeline.Snapshot();
        if (!queuedDuringActive &&
            before.player.action == horde::gameplay::PlayerCombatAction::SwingActive &&
            before.player.actionTime >= 0.05f)
        {
            queuedDuringActive = comboTimeline.CanAcceptAttack() &&
                comboTimeline.RequestAttack() == horde::gameplay::PlayerAttackCut::UpwardSlice;
        }
        const auto& snapshot = comboTimeline.Update(0.01f, 0.0f, 1.85f, 0.0f);
        if (snapshot.playerAttackPulse)
        {
            downwardPulses += snapshot.playerAttackCut ==
                horde::gameplay::PlayerAttackCut::DownwardCut ? 1 : 0;
            upwardPulses += snapshot.playerAttackCut ==
                horde::gameplay::PlayerAttackCut::UpwardSlice ? 1 : 0;
        }
        if (previousComboAction == horde::gameplay::PlayerCombatAction::SwingActive &&
            snapshot.player.action == horde::gameplay::PlayerCombatAction::UpwardSliceWindup)
        {
            continuousComboTransition = continuousComboTransition &&
                std::abs(snapshot.swordSwingRadians - previousSwordRadians) < 0.12f;
        }
        previousComboAction = snapshot.player.action;
        previousSwordRadians = snapshot.swordSwingRadians;
    }
    const bool comboFinished = firstCut == horde::gameplay::PlayerAttackCut::DownwardCut &&
                               queuedDuringActive && downwardPulses == 1 && upwardPulses == 1 &&
                               continuousComboTransition &&
                               comboTimeline.Snapshot().player.action ==
                                   horde::gameplay::PlayerCombatAction::Idle;
    comboTimeline.Reset();
    const bool comboReset = comboTimeline.Snapshot().player.action ==
                                horde::gameplay::PlayerCombatAction::Idle &&
                            !comboTimeline.Snapshot().player.comboQueued;

    horde::gameplay::SwordCombat lateCombo;
    lateCombo.RequestAttack();
    while (lateCombo.Snapshot().player.action !=
           horde::gameplay::PlayerCombatAction::SwingRecovery)
    {
        lateCombo.Update(0.01f, 0.0f, 1.85f, 0.0f);
    }
    const bool lateSecondCutRejected = !lateCombo.CanAcceptAttack();

    horde::gameplay::SwordCombat rearMiss;
    rearMiss.Reset(1u);
    rearMiss.RequestAttack();
    for (int tick = 0; tick < 80; ++tick)
    {
        rearMiss.Update(0.01f, 0.0f, -3.20f, 3.14159265f);
    }
    const bool rearSwingMissed = rearMiss.Snapshot().combatants[0].health == 1;

    const auto runParry = [](float playerX, float playerYaw, bool requestEarly)
    {
        horde::gameplay::SwordCombat parry;
        const float playerZ = -3.20f;
        bool requested = false;
        bool succeeded = false;
        bool damagePulse = false;
        int staggeredIndex = -1;
        for (int tick = 0; tick < 600 && !succeeded && !damagePulse; ++tick)
        {
            const auto& before = parry.Snapshot();
            if (!requested &&
                (requestEarly || (before.attackerIndex >= 0 &&
                 before.combatants[static_cast<std::size_t>(before.attackerIndex)].action ==
                     horde::gameplay::EnemyCombatAction::AttackWindup &&
                 before.combatants[static_cast<std::size_t>(before.attackerIndex)].actionTime >= 0.98f)))
            {
                parry.RequestParry();
                requested = true;
            }
            const auto& snapshot = parry.Update(dt, playerX, playerZ, playerYaw);
            damagePulse = damagePulse || snapshot.playerHitPulse;
            if (snapshot.parriedAttackerIndex >= 0)
            {
                succeeded = true;
                staggeredIndex = snapshot.parriedAttackerIndex;
            }
        }
        return std::array<int, 3>{succeeded ? 1 : 0, damagePulse ? 1 : 0, staggeredIndex};
    };
    const auto parryA = runParry(-0.75f, 0.0f, false);
    const auto parryB = runParry(0.75f, 0.0f, false);
    const auto earlyParry = runParry(-0.75f, 0.0f, true);
    const auto rearParry = runParry(-0.75f, 3.14159265f, false);
    const bool bothIdsParry = parryA[0] == 1 && parryA[2] == 0 &&
                              parryB[0] == 1 && parryB[2] == 1;
    const bool failedParriesDamage = earlyParry[0] == 0 && earlyParry[1] == 1 &&
                                     rearParry[0] == 0 && rearParry[1] == 1;

    horde::gameplay::SwordCombat stagger;
    bool staggerSucceeded = false;
    int heldIndex = -1;
    for (int tick = 0; tick < 600 && !staggerSucceeded; ++tick)
    {
        const auto& before = stagger.Snapshot();
        if (before.attackerIndex >= 0 &&
            before.combatants[static_cast<std::size_t>(before.attackerIndex)].action ==
                horde::gameplay::EnemyCombatAction::AttackWindup &&
            before.combatants[static_cast<std::size_t>(before.attackerIndex)].actionTime >= 0.98f)
        {
            stagger.RequestParry();
        }
        const auto& snapshot = stagger.Update(dt, -0.75f, -3.20f, 0.0f);
        if (snapshot.parriedAttackerIndex >= 0)
        {
            staggerSucceeded = true;
            heldIndex = snapshot.parriedAttackerIndex;
        }
    }
    bool tokenHeldThroughStagger = staggerSucceeded;
    stagger.RequestAttack();
    const auto& riposte = stagger.Update(dt, -0.75f, -3.20f, 0.0f);
    const bool immediateRiposte = riposte.player.action ==
        horde::gameplay::PlayerCombatAction::SwingWindup;
    for (int tick = 0; tick < 94; ++tick)
    {
        const auto& snapshot = stagger.Update(dt, -0.75f, -3.20f, 0.0f);
        tokenHeldThroughStagger = tokenHeldThroughStagger &&
            snapshot.attackerIndex == heldIndex &&
            snapshot.combatants[static_cast<std::size_t>(heldIndex)].action ==
                horde::gameplay::EnemyCombatAction::Staggered;
    }
    const bool staggerNearlyComplete = stagger.Snapshot().combatants[static_cast<std::size_t>(heldIndex)].action ==
        horde::gameplay::EnemyCombatAction::Staggered;
    int staggerBoundaryTicks = 95;
    while (staggerBoundaryTicks < 98 &&
           stagger.Snapshot().combatants[static_cast<std::size_t>(heldIndex)].action ==
               horde::gameplay::EnemyCombatAction::Staggered)
    {
        stagger.Update(dt, -0.75f, -3.20f, 0.0f);
        ++staggerBoundaryTicks;
    }
    const bool staggerCompletedAtEightTenths = stagger.Snapshot().combatants[static_cast<std::size_t>(heldIndex)].action !=
        horde::gameplay::EnemyCombatAction::Staggered && staggerBoundaryTicks <= 97;

    horde::gameplay::SwordCombat lateParry;
    bool lateSawDamage = false;
    bool lateRequested = false;
    bool lateSucceeded = false;
    for (int tick = 0; tick < 500; ++tick)
    {
        if (lateSawDamage && !lateRequested)
        {
            lateParry.RequestParry();
            lateRequested = true;
        }
        const auto& snapshot = lateParry.Update(dt, -0.75f, -3.20f, 0.0f);
        lateSawDamage = lateSawDamage || snapshot.playerHitPulse;
        lateSucceeded = lateSucceeded || snapshot.parriedAttackerIndex >= 0;
    }
    const bool lateParryFailed = lateSawDamage && lateRequested && !lateSucceeded;
    if (!exactSpawns || !exactHistoricalSpawn || !strictNearestSelection ||
        !sawSwing || !sawDeath || !deadPersisted || !singleTargetHit ||
        !clearedPair || !pairPersistsDead || !resetPair || !maintainedSeparation ||
        !oneAttackToken || !sawDamage ||
        playerHitPulses < horde::gameplay::PlayerVitals::kMaxVitality ||
        acceptedPlayerHits != horde::gameplay::PlayerVitals::kMaxVitality ||
        attackedPlayer.Snapshot().phase != horde::gameplay::PlayerLifePhase::Dead ||
        repeatedPlayerHitPulse || !stayedInsideArena || !idledBeyondDoor || idleWalkTransitions > 2 ||
        !swingStillWindup || !swingEnteredActive || !swingEnteredRecovery || !swingFinished ||
        !rearSwingMissed || !bothIdsParry || !failedParriesDamage ||
        !tokenHeldThroughStagger || !immediateRiposte || !staggerNearlyComplete ||
        !staggerCompletedAtEightTenths || !lateParryFailed || !comboFinished ||
        !comboReset || !lateSecondCutRejected)
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
                  << " idleWalkTransitions=" << idleWalkTransitions
                  << " phases=" << swingStillWindup << swingEnteredActive << swingEnteredRecovery << swingFinished
                  << " rearMiss=" << rearSwingMissed << " bothParry=" << bothIdsParry
                  << " failedParries=" << failedParriesDamage << " tokenHeld=" << tokenHeldThroughStagger
                  << " riposte=" << immediateRiposte << " staggerEdge=" << staggerNearlyComplete
                  << staggerCompletedAtEightTenths << " late=" << lateParryFailed
                  << " combo=" << comboFinished << comboReset << lateSecondCutRejected << '\n';
        return 1;
    }
    std::cout << "Combat smoke passed: two stable spawns, separation, one attacker, nearest-only hits, "
                 "persistent independent deaths, reset, damage pulses, collision, and route leash.\n";
    return 0;
}
