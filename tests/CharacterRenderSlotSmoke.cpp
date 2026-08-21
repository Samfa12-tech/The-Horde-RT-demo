#include "vulkan/raytracing/CharacterRenderSlot.h"
#include "vulkan/raytracing/SimulationFrameAdapter.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
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

bool HasInvertibleLinearTransform(const VkTransformMatrixKHR& transform)
{
    const float determinant =
        transform.matrix[0][0] *
            (transform.matrix[1][1] * transform.matrix[2][2] -
             transform.matrix[1][2] * transform.matrix[2][1]) -
        transform.matrix[0][1] *
            (transform.matrix[1][0] * transform.matrix[2][2] -
             transform.matrix[1][2] * transform.matrix[2][0]) +
        transform.matrix[0][2] *
            (transform.matrix[1][0] * transform.matrix[2][1] -
             transform.matrix[1][1] * transform.matrix[2][0]);
    return std::abs(determinant) > 0.00001f;
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

std::string ReadTextFile(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    std::ostringstream text;
    text << input.rdbuf();
    return text.str();
}

} // namespace

int main()
{
    using namespace horde::gameplay;
    using namespace horde::vulkan::raytracing;

    static_assert(CharacterRenderSlot::kTlasInstanceIndex == 2u);
    static_assert(CharacterRenderSlot::kSecondSkeletonTlasInstanceIndex == 18u);
    static_assert(CharacterRenderSlot::kMaximumSkeletonPoseBuckets == 2u);
    static_assert(PresentableTinyRtScene::kBlasCount == 9u);
    static_assert(PresentableTinyRtScene::kTlasInstanceCount == 19u);

    bool ok = true;
    EnemyRosterSnapshot roster;
    roster.selectedEnemy = EnemyKind::Skeleton;
    roster.renderedEnemyCapacity = 1u;
    roster.renderedEnemyCount = 1u;
    roster.renderedEnemies[0] = EnemyKind::Skeleton;
    LichSnapshot lich;
    std::array<simulation::SkeletonEnemySnapshot, simulation::kSkeletonEnemyCapacity> skeletons{};
    skeletons[0].id = simulation::EntityId::SkeletonA;
    skeletons[0].x = -0.75f;
    skeletons[0].z = -4.65f;
    skeletons[0].animation = EnemyAnimation::Walking;
    skeletons[0].animationTime = 2.0f;
    skeletons[0].action = EnemyCombatAction::Locomotion;
    skeletons[1] = skeletons[0];
    skeletons[1].id = simulation::EntityId::SkeletonB;
    skeletons[1].x = 0.75f;

    simulation::SimulationSnapshot simulationSnapshot;
    simulationSnapshot.skeletonEnemies = skeletons;
    simulationSnapshot.skeletonEnemyCount = skeletons.size();
    const RtSceneFrameInputs adaptedFrame = BuildRtSceneFrameInputs(simulationSnapshot, 0.75f);
    ok &= Require(adaptedFrame.skeletonEnemyCount == 2u &&
                  adaptedFrame.skeletonEnemies[0].id == simulation::EntityId::SkeletonA &&
                  adaptedFrame.skeletonEnemies[1].id == simulation::EntityId::SkeletonB,
                  "simulation adapter dropped the bounded skeleton entity snapshots");

    PlayerCombatSnapshot playerCombat;
    const PlayerWeaponRenderPose idleWeapon = EvaluatePlayerWeaponRenderPose(playerCombat, 0.0f, 1.05f);
    ok &= Require(Near(idleWeapon.parryBlend, 0.0f) && Near(idleWeapon.swordRadians, 0.0f) &&
                  Near(idleWeapon.rightHandLocal[0], 0.34f),
                  "idle weapon pose changed while adding parry composition");
    playerCombat.action = PlayerCombatAction::ParryActive;
    const PlayerWeaponRenderPose activeParry = EvaluatePlayerWeaponRenderPose(playerCombat, 0.0f, 1.05f);
    ok &= Require(Near(activeParry.parryBlend, 1.0f) && Near(activeParry.swordRadians, -0.82f) &&
                  Near(activeParry.rightHandLocal[0], -0.20f),
                  "active parry did not move the sword and right hand across the view");
    playerCombat.reaction = CombatReaction::Parried;
    playerCombat.reactionTime = 0.12f;
    const PlayerWeaponRenderPose successfulParry = EvaluatePlayerWeaponRenderPose(playerCombat, 0.0f, 1.05f);
    ok &= Require(Near(successfulParry.successJolt, 1.0f) &&
                  successfulParry.swordRadians > activeParry.swordRadians &&
                  successfulParry.rightHandLocal[0] > activeParry.rightHandLocal[0],
                  "successful parry did not add the bounded procedural weapon jolt");

    CharacterFramePlan sharedPlan = EvaluateCharacterFramePlan(skeletons, skeletons.size(), roster, lich, 2.967f);
    ok &= Require(sharedPlan.skeletonCount == 2u && sharedPlan.skeletonPoseBucketCount == 1u,
                  "matching skeleton poses did not share one bucket");
    ok &= Require(sharedPlan.skeletons[0].poseBucket == 0u && sharedPlan.skeletons[1].poseBucket == 0u,
                  "matching skeleton poses did not select bucket zero");
    ok &= Require(Near(sharedPlan.skeletons[0].transform.matrix[0][3], -0.75f) &&
                  Near(sharedPlan.skeletons[1].transform.matrix[0][3], 0.75f),
                  "two-skeleton transforms were not independent");

    skeletons[1].action = EnemyCombatAction::AttackActive;
    skeletons[1].actionTime = 0.08f;
    const CharacterFramePlan splitPlan = EvaluateCharacterFramePlan(skeletons, skeletons.size(), roster, lich, 2.967f);
    ok &= Require(splitPlan.skeletonPoseBucketCount == 2u && splitPlan.skeletons[1].poseBucket == 1u,
                  "divergent skeleton poses did not allocate the bounded second bucket");
    ok &= Require(splitPlan.skeletons[1].clip == horde::scene::SkeletonClip::Attack &&
                  Near(splitPlan.skeletons[1].time, 1.20f),
                  "active phase did not map continuously through the authored Attack clip");
    skeletons[1].action = EnemyCombatAction::Staggered;
    skeletons[1].actionTime = 0.0f;
    const CharacterFramePlan staggerStartPlan = EvaluateCharacterFramePlan(skeletons, skeletons.size(), roster, lich, 2.967f);
    skeletons[1].actionTime = 0.14f;
    const CharacterFramePlan staggerImpactPlan = EvaluateCharacterFramePlan(skeletons, skeletons.size(), roster, lich, 2.967f);
    skeletons[1].actionTime = 0.80f;
    const CharacterFramePlan staggerRecoveryPlan = EvaluateCharacterFramePlan(skeletons, skeletons.size(), roster, lich, 2.967f);
    ok &= Require(Near(staggerStartPlan.skeletons[1].time, 1.20f) &&
                  Near(staggerImpactPlan.skeletons[1].time, 1.48f) &&
                  Near(staggerRecoveryPlan.skeletons[1].time, 2.80f),
                  "stagger did not traverse the bounded authored Attack recovery from contact to rest");
    ok &= Require(!Near(staggerImpactPlan.skeletons[1].transform.matrix[1][1], 1.0f) &&
                  !Near(staggerImpactPlan.skeletons[1].transform.matrix[1][3], -0.95f) &&
                  Near(staggerRecoveryPlan.skeletons[1].transform.matrix[1][1], 1.0f) &&
                  Near(staggerRecoveryPlan.skeletons[1].transform.matrix[1][3], -0.95f),
                  "stagger did not produce a bounded recoil and settle over its 800 ms action");
    skeletons[0] = skeletons[1];
    skeletons[0].id = simulation::EntityId::SkeletonA;
    skeletons[0].x = -0.75f;
    skeletons[1].x = 0.75f;
    const CharacterFramePlan sharedStaggerPlan =
        EvaluateCharacterFramePlan(skeletons, skeletons.size(), roster, lich, 2.967f);
    ok &= Require(sharedStaggerPlan.skeletonPoseBucketCount == 1u &&
                  sharedStaggerPlan.skeletons[0].poseBucket == 0u &&
                  sharedStaggerPlan.skeletons[1].poseBucket == 0u,
                  "matching stagger skeletons did not reuse the existing pose zero bucket");

    roster.selectedEnemy = EnemyKind::Lich;
    lich.phase = LichPhase::Charging;
    lich.x = -32.2f;
    lich.y = -0.77f;
    lich.z = -13.1f;
    lich.facingRadians = 0.0f;
    lich.hitRecoil = 0.0f;
    lich.animationTime = 3.25f;
    const CharacterFramePlan lichPlan = EvaluateCharacterFramePlan(skeletons, 0u, roster, lich, 2.967f);
    ok &= Require(lichPlan.selectedLich && lichPlan.lichClip == horde::scene::SkinnedClip::Idle,
                  "living lich selection or clip changed");
    ok &= Require(Near(lichPlan.lichTime, 3.25f), "lich animation time changed");
    ok &= Require(Near(lichPlan.lichTransform.matrix[0][3], -32.2f) &&
                  Near(lichPlan.lichTransform.matrix[1][3], -0.77f) &&
                  Near(lichPlan.lichTransform.matrix[2][3], -13.1f),
                  "lich transform changed");
    lich.phase = LichPhase::Dead;
    ok &= Require(EvaluateCharacterFramePlan(skeletons, 0u, roster, lich, 2.967f).lichClip == horde::scene::SkinnedClip::Dead,
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
        ok &= Require(!slot.PrepareFrame(skeletons, 3u, invalidRoster, lich, unboundResources, diagnostic),
                      "character slot accepted more than two skeletons");
        ok &= Require(diagnostic.find("at most two") != std::string::npos,
                      "character slot did not explain the two-skeleton limit");

        slot.SkeletonGpu(0u).accelerationStructure.address = 101u;
        slot.SkeletonGpu(1u).accelerationStructure.address = 202u;
        slot.LichGpu().accelerationStructure.address = 303u;
        EnemyRosterSnapshot skeletonRoster = roster;
        skeletonRoster.selectedEnemy = EnemyKind::Skeleton;
        skeletons[1] = skeletons[0];
        skeletons[1].id = simulation::EntityId::SkeletonB;
        skeletons[1].x = 0.75f;
        ok &= Require(slot.CacheFramePlan(skeletons, skeletons.size(), skeletonRoster, lich, diagnostic), diagnostic.c_str());
        const auto sharedInstances = slot.BuildActiveInstances();
        ok &= Require(sharedInstances[0].mask == 0x01u && sharedInstances[1].mask == 0x01u,
                      "two active skeleton TLAS instances were not visible");
        ok &= Require(sharedInstances[0].instanceCustomIndex == 2u &&
                      sharedInstances[1].instanceCustomIndex == 2u &&
                      sharedInstances[0].accelerationStructureReference == 101u &&
                      sharedInstances[1].accelerationStructureReference == 101u,
                      "matching skeleton instances did not share pose zero BLAS and shader route");
        skeletons[1].action = EnemyCombatAction::AttackWindup;
        ok &= Require(slot.CacheFramePlan(skeletons, skeletons.size(), skeletonRoster, lich, diagnostic), diagnostic.c_str());
        const auto splitInstances = slot.BuildActiveInstances();
        ok &= Require(splitInstances[1].instanceCustomIndex == 18u &&
                      splitInstances[1].accelerationStructureReference == 202u,
                      "divergent second skeleton did not select pose one BLAS and shader route");
        invalidRoster.selectedEnemy = EnemyKind::Lich;
        ok &= Require(slot.CacheFramePlan(skeletons, 0u, invalidRoster, lich, diagnostic), diagnostic.c_str());
        const auto lichInstances = slot.BuildActiveInstances();
        ok &= Require(lichInstances[0].accelerationStructureReference == 303u &&
                      lichInstances[0].mask == 0x01u && lichInstances[1].mask == 0u,
                      "singular lich route populated the second character slot");
        ok &= Require(HasInvertibleLinearTransform(lichInstances[0].transform) &&
                      HasInvertibleLinearTransform(lichInstances[1].transform),
                      "singular lich route emitted a non-invertible masked TLAS transform");
        skeletonRoster.selectedEnemy = EnemyKind::Skeleton;
        ok &= Require(slot.CacheFramePlan(skeletons, 1u, skeletonRoster, lich, diagnostic), diagnostic.c_str());
        const auto oneSkeletonInstances = slot.BuildActiveInstances();
        ok &= Require(oneSkeletonInstances[1].mask == 0u &&
                      HasInvertibleLinearTransform(oneSkeletonInstances[1].transform),
                      "unused second skeleton slot emitted a non-invertible TLAS transform");
        ok &= Require(slot.CacheFramePlan(skeletons, 0u, skeletonRoster, lich, diagnostic), diagnostic.c_str());
        const auto noSkeletonInstances = slot.BuildActiveInstances();
        ok &= Require(noSkeletonInstances[0].mask == 0u && noSkeletonInstances[1].mask == 0u &&
                      noSkeletonInstances[0].instanceCustomIndex == 2u &&
                      noSkeletonInstances[1].instanceCustomIndex == 18u &&
                      HasInvertibleLinearTransform(noSkeletonInstances[0].transform) &&
                      HasInvertibleLinearTransform(noSkeletonInstances[1].transform),
                      "empty skeleton route did not preserve masked TLAS transforms and custom indices");
    }

    const std::filesystem::path root = FindRepoRoot();
    ok &= Require(!root.empty(), "repo assets were not found");
    if (!root.empty())
    {
        const std::string raygenSource = ReadTextFile(root / "shaders/raytracing/minimal.rgen");
        const std::string sceneSource =
            ReadTextFile(root / "src/vulkan/raytracing/PresentableTinyRtScene.cpp");
        ok &= Require(raygenSource.find(
                          "layout(std430, set = 0, binding = 10) readonly buffer SecondSkeletonVertices") !=
                          std::string::npos,
                      "raygen no longer declares the bounded second-skeleton pose buffer at binding 10");
        ok &= Require(raygenSource.find("bool secondSkeletonPose = h.instance == 18;") !=
                          std::string::npos &&
                      raygenSource.find("secondSkeleton.vertices[firstVertex].normal") !=
                          std::string::npos &&
                      raygenSource.find(
                          "secondSkeleton.vertices[firstVertex + 1].position.xyz - secondSkeleton.vertices[firstVertex].position.xyz") !=
                          std::string::npos,
                      "custom index 18 no longer selects second-pose shading and geometric-normal vertices");
        ok &= Require(sceneSource.find("secondSkeletonWrite.dstBinding = 10u;") !=
                          std::string::npos &&
                      sceneSource.find("secondSkeletonWrite.pBufferInfo = &secondSkeletonBufferInfo;") !=
                          std::string::npos,
                      "CPU descriptor writes no longer bind the second skeleton GPU vertex buffer at binding 10");

        CharacterRenderSlot slot;
        std::string diagnostic;
        ok &= Require(slot.LoadAssets(
                          (root / "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb").string(),
                          (root / "assets/models/enemies/meshy/lich_placeholder_merged_animations_v01.glb").string(),
                          diagnostic),
                      diagnostic.c_str());
        ok &= Require(slot.PrepareInitialGeometry(diagnostic), diagnostic.c_str());
        EnemyRosterSnapshot spareCapacityRoster;
        spareCapacityRoster.renderedEnemyCapacity = 8u;
        spareCapacityRoster.renderedEnemyCount = 1u;
        spareCapacityRoster.selectedEnemy = EnemyKind::Skeleton;
        spareCapacityRoster.renderedEnemies[0] = EnemyKind::Skeleton;
        const float deadDuration = slot.SkeletonDeadClipDuration();
        ok &= Require(deadDuration > 0.0f, "skeleton Dead clip duration was not captured from the model");
        skeletons[0].animation = EnemyAnimation::Dead;
        skeletons[0].action = EnemyCombatAction::Dead;
        skeletons[0].animationTime = deadDuration + 1.0f;
        skeletons[1] = skeletons[0];
        skeletons[1].id = simulation::EntityId::SkeletonB;
        skeletons[1].animationTime = deadDuration + 4.0f;
        const CharacterFramePlan finalDeadPlan =
            EvaluateCharacterFramePlan(skeletons, skeletons.size(), spareCapacityRoster, lich, deadDuration);
        ok &= Require(finalDeadPlan.skeletonPoseBucketCount == 1u &&
                      Near(finalDeadPlan.skeletons[0].time, deadDuration) &&
                      Near(finalDeadPlan.skeletons[1].time, deadDuration),
                      "final persistent skeleton corpses did not converge to one clamped pose bucket");
        ok &= Require(!CharacterPoseNeedsRefresh(
                          static_cast<int>(horde::scene::SkeletonClip::Dead),
                          finalDeadPlan.skeletons[0].time,
                          static_cast<int>(horde::scene::SkeletonClip::Dead),
                          deadDuration),
                      "clamped final skeleton corpse pose requested a redundant BLAS refit");
        ok &= Require(slot.SkeletonVertices(0u).size() == 28206u, "skeleton pose zero vertex count changed");
        ok &= Require(slot.SkeletonVertices(1u).size() == 28206u, "skeleton pose one vertex count changed");
        ok &= Require(slot.LichVertices().size() == 27564u, "lich slot vertex count changed");
        RtGpuResources unboundResources;
        diagnostic.clear();
        skeletons[1] = skeletons[0];
        skeletons[1].id = simulation::EntityId::SkeletonB;
        ok &= Require(!slot.PrepareFrame(skeletons, skeletons.size(), spareCapacityRoster, lich,
                                         unboundResources, diagnostic) &&
                      diagnostic.find("Invalid animated skeleton pose 0 vertex upload") != std::string::npos,
                      "spare roster capacity was mistaken for more than one active character");
        const auto& staff = slot.LichStaffLocalSample();
        ok &= Require(staff[0] > 0.90f && staff[1] > 0.70f,
                      "audited lich staff sample moved into the robe or eye cluster");
    }

    return ok ? 0 : 1;
}
