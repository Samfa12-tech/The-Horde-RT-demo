#include "vulkan/raytracing/CharacterRenderSlot.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

namespace
{

bool Require(const bool condition, const char* message)
{
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}

bool Near(const float actual, const float expected, const float tolerance = 0.00001f)
{
    return std::abs(actual - expected) <= tolerance;
}

std::filesystem::path FindRepoRoot()
{
    std::filesystem::path candidate = std::filesystem::current_path();
    for (int depth = 0; depth < 6; ++depth)
    {
        if (std::filesystem::exists(candidate / "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb"))
        {
            return candidate;
        }
        if (!candidate.has_parent_path()) break;
        candidate = candidate.parent_path();
    }
    return {};
}

} // namespace

int main()
{
    using namespace horde::gameplay;
    using namespace horde::vulkan::raytracing;

    static_assert(CharacterRenderSlot::kTlasInstanceIndex == 2u);
    static_assert(CharacterRenderSlot::kMaximumActiveCharacters == 1u);

    bool ok = true;
    CombatSnapshot combat;
    combat.enemyX = 1.25f;
    combat.enemyZ = -6.5f;
    combat.enemyFacingRadians = 0.0f;
    combat.enemyAnimation = EnemyAnimation::Walking;
    combat.enemyAnimationTime = 2.0f;
    EnemyRosterSnapshot roster;
    roster.selectedEnemy = EnemyKind::Skeleton;
    roster.renderedEnemyCapacity = 1u;
    roster.renderedEnemyCount = 1u;
    roster.renderedEnemies[0] = EnemyKind::Skeleton;
    LichSnapshot lich;

    CharacterRenderPlan skeletonPlan = EvaluateCharacterRenderPlan(combat, roster, lich);
    ok &= Require(!skeletonPlan.selectedLich, "skeleton selection changed");
    ok &= Require(skeletonPlan.skeletonClip == horde::scene::SkeletonClip::Walking,
                  "walking clip mapping changed");
    ok &= Require(Near(skeletonPlan.skeletonTime, 1.8f), "walking time multiplier changed");
    ok &= Require(Near(skeletonPlan.transform.matrix[0][3], 1.25f) &&
                  Near(skeletonPlan.transform.matrix[1][3], -0.95f) &&
                  Near(skeletonPlan.transform.matrix[2][3], -6.5f),
                  "skeleton transform changed");

    combat.enemyAnimation = EnemyAnimation::Attack;
    combat.enemyAnimationTime = 0.4f;
    skeletonPlan = EvaluateCharacterRenderPlan(combat, roster, lich);
    ok &= Require(skeletonPlan.skeletonClip == horde::scene::SkeletonClip::Attack &&
                  Near(skeletonPlan.skeletonTime, 0.4f),
                  "attack clip mapping changed");
    combat.enemyAnimation = EnemyAnimation::Dead;
    ok &= Require(EvaluateCharacterRenderPlan(combat, roster, lich).skeletonClip == horde::scene::SkeletonClip::Dead,
                  "dead clip mapping changed");

    roster.selectedEnemy = EnemyKind::Lich;
    lich.phase = LichPhase::Charging;
    lich.x = -32.2f;
    lich.y = -0.77f;
    lich.z = -13.1f;
    lich.facingRadians = 0.0f;
    lich.hitRecoil = 0.0f;
    lich.animationTime = 3.25f;
    const CharacterRenderPlan lichPlan = EvaluateCharacterRenderPlan(combat, roster, lich);
    ok &= Require(lichPlan.selectedLich && lichPlan.lichClip == horde::scene::SkinnedClip::Idle,
                  "living lich selection or clip changed");
    ok &= Require(Near(lichPlan.lichTime, 3.25f), "lich animation time changed");
    ok &= Require(Near(lichPlan.transform.matrix[0][3], -32.2f) &&
                  Near(lichPlan.transform.matrix[1][3], -0.77f) &&
                  Near(lichPlan.transform.matrix[2][3], -13.1f),
                  "lich transform changed");
    lich.phase = LichPhase::Dead;
    ok &= Require(EvaluateCharacterRenderPlan(combat, roster, lich).lichClip == horde::scene::SkinnedClip::Dead,
                  "lich death clip mapping changed");

    constexpr float interval = 1.0f / 30.0f;
    ok &= Require(CharacterPoseNeedsRefresh(1, 1.0f, -1, -1.0f), "first pose must refresh");
    ok &= Require(!CharacterPoseNeedsRefresh(1, 1.0f + interval * 0.5f, 1, 1.0f),
                  "pose refreshed before the 30 Hz boundary");
    ok &= Require(CharacterPoseNeedsRefresh(1, 1.0f + interval * 1.01f, 1, 1.0f),
                  "pose did not refresh at the 30 Hz boundary");
    ok &= Require(CharacterPoseNeedsRefresh(2, 1.0f, 1, 1.0f), "clip change did not refresh");
    ok &= Require(CharacterPoseNeedsRefresh(1, 0.5f, 1, 1.0f), "time rewind did not refresh");

    {
        CharacterRenderSlot slot;
        RtGpuResources unboundResources;
        std::string diagnostic;
        EnemyRosterSnapshot invalidRoster = roster;
        invalidRoster.renderedEnemyCapacity = 8u;
        invalidRoster.renderedEnemyCount = 2u;
        invalidRoster.renderedEnemies[0] = EnemyKind::Skeleton;
        invalidRoster.renderedEnemies[1] = EnemyKind::Lich;
        ok &= Require(!slot.PrepareFrame(combat, invalidRoster, lich, unboundResources, diagnostic),
                      "character slot accepted a two-character roster");
        ok &= Require(diagnostic.find("exceeded") != std::string::npos,
                      "character slot did not explain the one-active-character limit");

        invalidRoster = roster;
        invalidRoster.renderedEnemyCount = 1u;
        invalidRoster.renderedEnemies[0] = EnemyKind::Skeleton;
        ok &= Require(!slot.PrepareFrame(combat, invalidRoster, lich, unboundResources, diagnostic),
                      "character slot accepted a selected/active roster mismatch");
        ok &= Require(diagnostic.find("does not match") != std::string::npos,
                      "character slot did not explain the roster mismatch");
    }

    const std::filesystem::path root = FindRepoRoot();
    ok &= Require(!root.empty(), "repo assets were not found");
    if (!root.empty())
    {
        CharacterRenderSlot slot;
        std::string diagnostic;
        ok &= Require(slot.LoadAssets(
                          (root / "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb").string(),
                          (root / "assets/models/enemies/meshy/lich_placeholder_merged_animations_v01.glb").string(),
                          diagnostic),
                      diagnostic.c_str());
        ok &= Require(slot.PrepareInitialGeometry(diagnostic), diagnostic.c_str());
        ok &= Require(slot.SkeletonVertices().size() == 28206u, "skeleton slot vertex count changed");
        ok &= Require(slot.LichVertices().size() == 27564u, "lich slot vertex count changed");
        EnemyRosterSnapshot spareCapacityRoster;
        spareCapacityRoster.renderedEnemyCapacity = 8u;
        spareCapacityRoster.renderedEnemyCount = 1u;
        spareCapacityRoster.selectedEnemy = EnemyKind::Skeleton;
        spareCapacityRoster.renderedEnemies[0] = EnemyKind::Skeleton;
        RtGpuResources unboundResources;
        diagnostic.clear();
        ok &= Require(!slot.PrepareFrame(combat, spareCapacityRoster, lich, unboundResources, diagnostic) &&
                      diagnostic.find("Invalid animated skeleton vertex upload") != std::string::npos,
                      "spare roster capacity was mistaken for more than one active character");
        const auto& staff = slot.LichStaffLocalSample();
        ok &= Require(staff[0] > 0.90f && staff[1] > 0.70f,
                      "audited lich staff sample moved into the robe or eye cluster");
    }

    return ok ? 0 : 1;
}
