#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/SwordCombat.h"

namespace
{
bool ContactPartitionsStayOrdered()
{
    using namespace horde::gameplay;
    const std::array<std::array<float, 3>, 6> partitions{{
        {{1.0f / 120.0f, 1.0f / 120.0f, 1.0f / 120.0f}},
        {{1.0f / 60.0f, 1.0f / 60.0f, 1.0f / 60.0f}},
        {{1.0f / 30.0f, 1.0f / 30.0f, 1.0f / 30.0f}},
        {{0.05f, 0.05f, 0.05f}},
        {{0.01f, 0.01f, 0.01f}},
        {{0.003f, 0.05f, 0.017f}},
    }};
    for (const auto& partition : partitions)
    {
        SwordCombat combat;
        if (combat.RequestAttack() != PlayerAttackCut::DownwardCut) return false;
        int downwardPulses = 0, upwardPulses = 0;
        bool comboQueued = false;
        for (std::size_t tick = 0; tick < 300u; ++tick)
        {
            if (!comboQueued && combat.Snapshot().player.action == PlayerCombatAction::SwingActive &&
                combat.Snapshot().player.actionTime >= 0.05f)
                comboQueued = combat.RequestAttack() == PlayerAttackCut::UpwardSlice;
            const float step = partition[tick % partition.size()];
            const auto snapshot = combat.Update(step, 0.0f, 1.85f, 0.0f);
            if (snapshot.playerAttackPulse)
            {
                const bool downward = snapshot.playerAttackCut == PlayerAttackCut::DownwardCut;
                const float contact = downward ? SwordCombat::kDownwardContactTime : SwordCombat::kUpwardContactTime;
                if (snapshot.player.action != (downward ? PlayerCombatAction::SwingActive : PlayerCombatAction::UpwardSliceActive) ||
                    snapshot.player.actionTime < contact || snapshot.player.actionTime > contact + step + 0.000001f)
                    return false;
                if (downward) ++downwardPulses;
                else
                {
                    if (downwardPulses != 1) return false;
                    ++upwardPulses;
                }
            }
            const auto zero = combat.Update(0.0f, 0.0f, 1.85f, 0.0f);
            if (zero.playerAttackPulse || zero.player.action != snapshot.player.action ||
                zero.player.actionTime != snapshot.player.actionTime) return false;
        }
        if (!comboQueued || downwardPulses != 1 || upwardPulses != 1 ||
            combat.Snapshot().player.action != PlayerCombatAction::Idle) return false;
    }
    return true;
}

bool HeldGuardActionsAreBounded()
{
    using namespace horde::gameplay;
    constexpr float step = 1.0f / 60.0f;
    bool passed = true;
    const auto expect = [&](bool condition, const char* message) {
        if (!condition) { passed = false; std::cerr << "Guard smoke: " << message << '\n'; }
    };
    // Disable enemy admission rather than manufacture damage or overwrite poses;
    // this fixture isolates the player's real action/recovery clock.
    const auto tick = [&](SwordCombat& combat, bool held, bool parryEdge = false) {
        combat.SetGuardHeld(held);
        if (parryEdge) combat.RequestParry();
        return combat.Update(step, 0.0f, 20.0f, 0.0f, true, false);
    };
    SwordCombat held;
    auto state = tick(held, true, true);
    expect(state.player.action == PlayerCombatAction::ParryStartup,
           "fresh held press starts the original startup, not immediate guard");
    bool sawActive = false, reachedGuard = false;
    for (int frame = 0; frame < 60; ++frame)
    {
        state = tick(held, true);
        sawActive |= state.player.action == PlayerCombatAction::ParryActive;
        reachedGuard |= state.player.action == PlayerCombatAction::Guard;
    }
    expect(sawActive && reachedGuard && state.player.action == PlayerCombatAction::Guard,
           "the existing active window finishes into held guard");
    for (int frame = 0; frame < 120; ++frame)
    {
        state = tick(held, true, true); // Even malformed repeated edges cannot refresh a hold.
        expect(state.player.action == PlayerCombatAction::Guard,
               "continued hold and edge spam cannot reopen parry");
        expect(std::isfinite(state.player.guardPoseWeight) &&
               state.player.guardPoseWeight >= 0.0f && state.player.guardPoseWeight <= 1.0f,
               "held presentation weight stays finite and bounded");
    }

    expect(held.RequestAttack() == PlayerAttackCut::DownwardCut,
           "a deliberate attack is accepted from guard");
    state = tick(held, true);
    expect(state.player.action == PlayerCombatAction::SwingWindup &&
           state.player.guardPoseWeight > 0.0f,
           "attack starts immediately while blending from guard instead of dropping to rest");
    float attackSeconds = step;
    bool returnedGuard = false, sawAttackRecovery = false, sawRepeatedParry = false;
    for (int frame = 0; frame < 120; ++frame)
    {
        state = tick(held, true, true);
        attackSeconds += step;
        sawAttackRecovery |= state.player.action == PlayerCombatAction::SwingRecovery;
        sawRepeatedParry |= state.player.action == PlayerCombatAction::ParryStartup ||
                            state.player.action == PlayerCombatAction::ParryActive;
        if (state.player.action == PlayerCombatAction::Guard) { returnedGuard = true; break; }
    }
    expect(returnedGuard && sawAttackRecovery && !sawRepeatedParry &&
           attackSeconds + 0.00001f >= SwordCombat::kSwordDuration - step,
           "held protection returns only after complete attack recovery and never grants a parry");

    // A release or re-press during the busy action cannot cancel its recovery.
    expect(held.RequestAttack() == PlayerAttackCut::DownwardCut,
           "the recovered guard accepts another ordinary attack");
    state = tick(held, true);
    float busySeconds = step;
    for (int frame = 0; frame < 8; ++frame) { state = tick(held, false); busySeconds += step; }
    bool repressedWindow = false;
    state = tick(held, true, true); busySeconds += step;
    for (int frame = 0; frame < 120; ++frame)
    {
        repressedWindow |= state.player.action == PlayerCombatAction::ParryStartup ||
                           state.player.action == PlayerCombatAction::ParryActive;
        if (state.player.action == PlayerCombatAction::Guard) break;
        state = tick(held, true); busySeconds += step;
    }
    expect(state.player.action == PlayerCombatAction::Guard && !repressedWindow &&
           busySeconds + 0.00001f >= SwordCombat::kSwordDuration - step,
           "rapid release/re-press during a swing cannot cancel recovery or buffer a parry");

    expect(held.RequestAttack() == PlayerAttackCut::DownwardCut,
           "release-during-swing fixture starts an actual attack");
    tick(held, true);
    for (int frame = 0; frame < 90; ++frame) state = tick(held, false);
    expect(state.player.action == PlayerCombatAction::Idle && state.player.guardPoseWeight == 0.0f,
           "release during swing leaves no recovered guard or stuck pose");
    state = tick(held, true, true);
    expect(state.player.action == PlayerCombatAction::ParryStartup,
           "a fresh physical press after release can open one new parry");
    for (int frame = 0; frame < 50; ++frame) state = tick(held, true);
    state = tick(held, false);
    expect(state.player.action != PlayerCombatAction::Guard,
           "release ends logical guard on its first update");
    for (int frame = 0; frame < 30; ++frame) state = tick(held, false);
    expect(state.player.action == PlayerCombatAction::Idle && state.player.guardPoseWeight == 0.0f,
           "released guard settles naturally without a stuck action");
    held.CancelPlayerActions();
    expect(held.Snapshot().player.action == PlayerCombatAction::Idle &&
           held.Snapshot().player.guardPoseWeight == 0.0f,
           "explicit action cancellation clears held presentation");
    return passed;
}
}

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
        !actionTimeline.Snapshot().playerAttackPulse;
    bool noEarlyDownwardContact = true;
    int downwardContactPulses = 0;
    float downwardContactTime = -1.0f;
    // Ten 10ms float additions may sit just below 100ms. Admit the next
    // update, while checking the measured action time and once-only pulse.
    for (int tick = 0; tick < 11; ++tick)
    {
        actionTimeline.Update(0.01f, 0.0f, 1.85f, 0.0f);
        if (actionTimeline.Snapshot().playerAttackPulse)
        {
            ++downwardContactPulses;
            downwardContactTime = actionTimeline.Snapshot().player.actionTime;
            noEarlyDownwardContact &= downwardContactTime >=
                horde::gameplay::SwordCombat::kDownwardContactTime;
        }
    }
    const bool downwardContactDuringStroke = noEarlyDownwardContact &&
        downwardContactPulses == 1 && downwardContactTime <=
            horde::gameplay::SwordCombat::kDownwardContactTime + 0.01001f;
    // The accepted combo window holds the completed downward impact through
    // 420 ms so a human second press can chain the upward slice. With no second
    // press, the legacy single-cut path enters recovery at that exact boundary.
    for (int tick = 0; tick < 32; ++tick)
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
    bool contactsDuringBothStrokes = true;
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
            contactsDuringBothStrokes &= snapshot.player.actionTime >=
                (snapshot.playerAttackCut == horde::gameplay::PlayerAttackCut::DownwardCut ?
                    horde::gameplay::SwordCombat::kDownwardContactTime : horde::gameplay::SwordCombat::kUpwardContactTime);
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
        horde::gameplay::PlayerCombatAction::SwingWindup && !riposte.playerAttackPulse;
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
    const bool contactPartitions = ContactPartitionsStayOrdered();
    const bool heldGuardActions = HeldGuardActionsAreBounded();
    if (!heldGuardActions || !contactPartitions || !exactSpawns || !exactHistoricalSpawn || !strictNearestSelection ||
        !sawSwing || !sawDeath || !deadPersisted || !singleTargetHit ||
        !clearedPair || !pairPersistsDead || !resetPair || !maintainedSeparation ||
        !oneAttackToken || !sawDamage ||
        playerHitPulses < horde::gameplay::PlayerVitals::kMaxVitality ||
        acceptedPlayerHits != horde::gameplay::PlayerVitals::kMaxVitality ||
        attackedPlayer.Snapshot().phase != horde::gameplay::PlayerLifePhase::Dead ||
        repeatedPlayerHitPulse || !stayedInsideArena || !idledBeyondDoor || idleWalkTransitions > 2 ||
        !swingStillWindup || !swingEnteredActive || !downwardContactDuringStroke ||
        !contactsDuringBothStrokes || !swingEnteredRecovery || !swingFinished ||
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
                  << " strokeContact=" << downwardContactDuringStroke << contactsDuringBothStrokes
                  << " partitions=" << contactPartitions
                  << " heldGuard=" << heldGuardActions
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
