#include <cmath>
#include <iostream>

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

    horde::gameplay::SwordCombat enemyAttack;
    bool sawDamage = false;
    for (int frame = 0; frame < 1000; ++frame)
    {
        const auto& snapshot = enemyAttack.Update(dt, 0.0f, -0.8f, 0.0f);
        sawDamage = sawDamage || snapshot.damageFlash > 0.5f;
    }

    if (!sawSwing || !sawDeath || !sawRespawn || !sawDamage)
    {
        std::cerr << "Combat smoke failed: swing=" << sawSwing << " death=" << sawDeath
                  << " respawn=" << sawRespawn << " damage=" << sawDamage << '\n';
        return 1;
    }
    std::cout << "Combat smoke passed: swing, hit/death, respawn, and enemy damage pulse.\n";
    return 0;
}
