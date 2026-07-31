#include <cmath>
#include <iostream>

#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/SwordCombat.h"

int main()
{
    constexpr float dt = 1.0f / 120.0f;
    horde::gameplay::SwordCombat combat;
    bool sawSwing = false;
    bool sawDeath = false;
    bool sawRespawn = false;
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
        else if (sawDeath)
        {
            sawRespawn = true;
        }
    }

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
        stayedInsideArena = stayedInsideArena &&
                            horde::gameplay::IsSkeletonEnemyPositionWalkable(snapshot.enemyX, snapshot.enemyZ);
    }
    const auto beforeLeash = leashedEnemy.Snapshot();
    for (int frame = 0; frame < 240; ++frame)
    {
        const auto& snapshot = leashedEnemy.Update(dt, 0.0f, -7.0f, 0.0f);
        idledBeyondDoor = snapshot.enemyAnimation == horde::gameplay::EnemyAnimation::Idle &&
                          std::abs(snapshot.enemyX - beforeLeash.enemyX) < 0.001f &&
                          std::abs(snapshot.enemyZ - beforeLeash.enemyZ) < 0.001f;
    }

    if (!sawSwing || !sawDeath || !sawRespawn || !sawDamage ||
        playerHitPulses < horde::gameplay::PlayerVitals::kMaxVitality ||
        acceptedPlayerHits != horde::gameplay::PlayerVitals::kMaxVitality ||
        attackedPlayer.Snapshot().phase != horde::gameplay::PlayerLifePhase::Dead ||
        repeatedPlayerHitPulse || !stayedInsideArena || !idledBeyondDoor || idleWalkTransitions > 2)
    {
        std::cerr << "Combat smoke failed: swing=" << sawSwing << " death=" << sawDeath
                  << " respawn=" << sawRespawn << " damage=" << sawDamage
                  << " enemyCollision=" << stayedInsideArena << " leash=" << idledBeyondDoor
                  << " hitPulses=" << playerHitPulses << " repeatedPulse=" << repeatedPlayerHitPulse
                  << " acceptedPlayerHits=" << acceptedPlayerHits
                  << " idleWalkTransitions=" << idleWalkTransitions << '\n';
        return 1;
    }
    std::cout << "Combat smoke passed: swing, hit/death, respawn, single-frame damage pulses, "
                 "three-vitality routing, collision, and route leash.\n";
    return 0;
}
