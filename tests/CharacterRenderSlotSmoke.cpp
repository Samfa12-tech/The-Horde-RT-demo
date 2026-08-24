#include "vulkan/raytracing/CharacterRenderSlot.h"
#include "vulkan/raytracing/RtSceneTuning.h"
#include "vulkan/raytracing/SimulationFrameAdapter.h"
#include "platform/android/AndroidRtLabState.h"

#include <cmath>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

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
    static_assert(static_cast<std::uint32_t>(WaterQuality::Off) == 0u);
    static_assert(static_cast<std::uint32_t>(WaterQuality::Mobile) == 1u);
    static_assert(static_cast<std::uint32_t>(WaterQuality::High) == 2u);
    static_assert(static_cast<std::uint32_t>(RtWorkloadPreset::Lean) == 0u);
    static_assert(static_cast<std::uint32_t>(RtWorkloadPreset::Authored) == 1u);
    static_assert(static_cast<std::uint32_t>(RtWorkloadPreset::Max) == 2u);

    using horde::platform::android::AndroidRtLabState;
    using horde::platform::android::RtLabUnlockDecisionInputs;
    using horde::platform::android::ShouldPersistRtLabUnlock;

    AndroidRtLabState androidTuning;
    RtSceneTuning androidUnclamped;
    androidUnclamped.waterfallWidthScale = 3.0f;
    androidUnclamped.finaleRoofOpenOverride = -1.0f;
    androidUnclamped.finaleDawnRevealOverride = 2.0f;
    androidUnclamped.fogDensityScale = -1.0f;
    androidUnclamped.lights[static_cast<std::size_t>(RtLightGroup::Staff)] = {999.0f, 3.0f};
    androidUnclamped.workloadPreset = RtWorkloadPreset::Max;
    androidTuning.Replace(androidUnclamped);
    const RtSceneTuning androidClamped = androidTuning.Snapshot();
    bool ok = true;
    ok &= Require(Near(androidClamped.waterfallWidthScale, 2.0f) &&
                      Near(*androidClamped.finaleRoofOpenOverride, 0.0f) &&
                      Near(*androidClamped.finaleDawnRevealOverride, 1.0f) &&
                      Near(androidClamped.fogDensityScale, 0.0f) &&
                      Near(androidClamped.lights[static_cast<std::size_t>(RtLightGroup::Staff)].hueDegrees, 180.0f) &&
                      Near(androidClamped.lights[static_cast<std::size_t>(RtLightGroup::Staff)].intensityScale, 2.0f) &&
                      androidClamped.workloadPreset == RtWorkloadPreset::Max,
                  "Android RT Lab publication did not atomically clamp a complete tuning snapshot");
    androidTuning.Reset();
    const RtSceneTuning androidReset = androidTuning.Snapshot();
    ok &= Require(Near(androidReset.waterfallWidthScale, 1.0f) &&
                      !androidReset.finaleRoofOpenOverride.has_value() &&
                      !androidReset.finaleDawnRevealOverride.has_value() &&
                      Near(androidReset.fogDensityScale, 1.0f) &&
                      androidReset.workloadPreset == RtWorkloadPreset::Authored,
                  "Android RT Lab route reset did not restore authored tuning");

    RtSceneTuning coherentA;
    coherentA.waterfallWidthScale = 0.25f;
    coherentA.fogDensityScale = 0.0f;
    coherentA.lights[static_cast<std::size_t>(RtLightGroup::Torch)] = {-180.0f, 0.0f};
    coherentA.workloadPreset = RtWorkloadPreset::Lean;
    RtSceneTuning coherentB;
    coherentB.waterfallWidthScale = 2.0f;
    coherentB.fogDensityScale = 2.0f;
    coherentB.lights[static_cast<std::size_t>(RtLightGroup::Torch)] = {180.0f, 2.0f};
    coherentB.workloadPreset = RtWorkloadPreset::Max;
    androidTuning.Replace(coherentA);
    std::atomic<bool> coherentPublication{true};
    std::thread tuningWriter([&]
    {
        for (int iteration = 0; iteration < 2000; ++iteration)
        {
            androidTuning.Replace((iteration & 1) == 0 ? coherentB : coherentA);
        }
    });
    for (int iteration = 0; iteration < 2000; ++iteration)
    {
        const RtSceneTuning sample = androidTuning.Snapshot();
        const bool isA = Near(sample.waterfallWidthScale, 0.25f) &&
            Near(sample.fogDensityScale, 0.0f) &&
            Near(sample.lights[0].hueDegrees, -180.0f) &&
            sample.workloadPreset == RtWorkloadPreset::Lean;
        const bool isB = Near(sample.waterfallWidthScale, 2.0f) &&
            Near(sample.fogDensityScale, 2.0f) &&
            Near(sample.lights[0].hueDegrees, 180.0f) &&
            sample.workloadPreset == RtWorkloadPreset::Max;
        if (!isA && !isB) coherentPublication.store(false, std::memory_order_release);
    }
    tuningWriter.join();
    ok &= Require(coherentPublication.load(std::memory_order_acquire),
                  "Android renderer observed a torn RT Lab tuning publication");

    ok &= Require(ShouldPersistRtLabUnlock({true, false, false, false, false}) &&
                      !ShouldPersistRtLabUnlock({false, false, false, false, false}) &&
                      !ShouldPersistRtLabUnlock({true, true, false, false, false}) &&
                      !ShouldPersistRtLabUnlock({true, false, true, false, false}) &&
                      !ShouldPersistRtLabUnlock({true, false, false, true, false}) &&
                      !ShouldPersistRtLabUnlock({true, false, false, false, true}),
                  "Android RT Lab unlock was not restricted to genuine live finale completion");

    ok &= Require(PresentableTinyRtScene::kBlasCount == 10u &&
                      PresentableTinyRtScene::kTlasInstanceCount == 20u,
                  "RT lab waterfall must report its dedicated tenth BLAS and twentieth TLAS instance");
    const RtSceneTuning authoredTuning;
    ok &= Require(Near(authoredTuning.waterfallWidthScale, 1.0f) &&
                      !authoredTuning.finaleRoofOpenOverride.has_value() &&
                      !authoredTuning.finaleDawnRevealOverride.has_value() &&
                      Near(authoredTuning.fogDensityScale, 1.0f) &&
                      authoredTuning.workloadPreset == RtWorkloadPreset::Authored,
                  "authored RT tuning defaults changed the released scene baseline");
    for (const RtLightTuning& light : authoredTuning.lights)
    {
        ok &= Require(Near(light.hueDegrees, 0.0f) && Near(light.intensityScale, 1.0f),
                      "authored RT light tuning defaults changed the released scene baseline");
    }

    RtSceneTuning unclampedLow = authoredTuning;
    unclampedLow.waterfallWidthScale = 0.0f;
    unclampedLow.finaleRoofOpenOverride = -1.0f;
    unclampedLow.finaleDawnRevealOverride = -1.0f;
    unclampedLow.fogDensityScale = -1.0f;
    for (RtLightTuning& light : unclampedLow.lights)
    {
        light.hueDegrees = -999.0f;
        light.intensityScale = -1.0f;
    }
    const RtSceneTuning clampedLow = ClampRtSceneTuning(unclampedLow);
    ok &= Require(Near(clampedLow.waterfallWidthScale, 0.25f) &&
                      Near(*clampedLow.finaleRoofOpenOverride, 0.0f) &&
                      Near(*clampedLow.finaleDawnRevealOverride, 0.0f) &&
                      Near(clampedLow.fogDensityScale, 0.0f),
                  "RT tuning lower bounds did not preserve the approved minimums");
    for (const RtLightTuning& light : clampedLow.lights)
    {
        ok &= Require(Near(light.hueDegrees, -180.0f) && Near(light.intensityScale, 0.0f),
                      "RT light tuning lower bounds did not preserve the approved minimums");
    }

    RtSceneTuning unclampedHigh = authoredTuning;
    unclampedHigh.waterfallWidthScale = 3.0f;
    unclampedHigh.finaleRoofOpenOverride = 2.0f;
    unclampedHigh.finaleDawnRevealOverride = 2.0f;
    unclampedHigh.fogDensityScale = 3.0f;
    for (RtLightTuning& light : unclampedHigh.lights)
    {
        light.hueDegrees = 999.0f;
        light.intensityScale = 3.0f;
    }
    const RtSceneTuning clampedHigh = ClampRtSceneTuning(unclampedHigh);
    ok &= Require(Near(clampedHigh.waterfallWidthScale, 2.0f) &&
                      Near(*clampedHigh.finaleRoofOpenOverride, 1.0f) &&
                      Near(*clampedHigh.finaleDawnRevealOverride, 1.0f) &&
                      Near(clampedHigh.fogDensityScale, 2.0f),
                  "RT tuning upper bounds did not preserve the approved maximums");
    for (const RtLightTuning& light : clampedHigh.lights)
    {
        ok &= Require(Near(light.hueDegrees, 180.0f) && Near(light.intensityScale, 2.0f),
                      "RT light tuning upper bounds did not preserve the approved maximums");
    }

    RtSceneTuning independentLights = authoredTuning;
    independentLights.lights[static_cast<std::size_t>(RtLightGroup::Torch)] = {45.0f, 0.75f};
    independentLights.lights[static_cast<std::size_t>(RtLightGroup::Staff)] = {-90.0f, 1.50f};
    const RtSceneTuning resolvedLights = ClampRtSceneTuning(independentLights);
    ok &= Require(Near(resolvedLights.lights[static_cast<std::size_t>(RtLightGroup::Torch)].hueDegrees, 45.0f) &&
                      Near(resolvedLights.lights[static_cast<std::size_t>(RtLightGroup::Torch)].intensityScale, 0.75f) &&
                      Near(resolvedLights.lights[static_cast<std::size_t>(RtLightGroup::Skylight)].hueDegrees, 0.0f) &&
                      Near(resolvedLights.lights[static_cast<std::size_t>(RtLightGroup::Skylight)].intensityScale, 1.0f) &&
                      Near(resolvedLights.lights[static_cast<std::size_t>(RtLightGroup::Passage)].hueDegrees, 0.0f) &&
                      Near(resolvedLights.lights[static_cast<std::size_t>(RtLightGroup::Passage)].intensityScale, 1.0f) &&
                      Near(resolvedLights.lights[static_cast<std::size_t>(RtLightGroup::Staff)].hueDegrees, -90.0f) &&
                      Near(resolvedLights.lights[static_cast<std::size_t>(RtLightGroup::Staff)].intensityScale, 1.50f),
                  "RT light groups did not remain independently tunable");

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
    simulationSnapshot.lich.finaleSkylightOpenProgress = 0.35f;
    simulationSnapshot.lich.finaleDawnRevealProgress = 0.65f;
    const RtSceneFrameInputs adaptedFrame = BuildRtSceneFrameInputs(simulationSnapshot, 0.75f);
    ok &= Require(adaptedFrame.skeletonEnemyCount == 2u &&
                  adaptedFrame.skeletonEnemies[0].id == simulation::EntityId::SkeletonA &&
                  adaptedFrame.skeletonEnemies[1].id == simulation::EntityId::SkeletonB,
                  "simulation adapter dropped the bounded skeleton entity snapshots");
    ok &= Require(adaptedFrame.waterQuality == WaterQuality::High,
                  "simulation adapter default water quality must remain High on Windows/capture callers");
    ok &= Require(BuildRtSceneFrameInputs(simulationSnapshot, 0.75f, WaterQuality::Off).waterQuality ==
                      WaterQuality::Off &&
                  BuildRtSceneFrameInputs(simulationSnapshot, 0.75f, WaterQuality::Mobile).waterQuality ==
                      WaterQuality::Mobile &&
                  BuildRtSceneFrameInputs(simulationSnapshot, 0.75f, WaterQuality::High).waterQuality ==
                      WaterQuality::High,
                  "simulation adapter did not preserve all three explicit RT water quality modes");
    RtSceneTuning finaleOverrides;
    finaleOverrides.finaleRoofOpenOverride = 0.9f;
    finaleOverrides.finaleDawnRevealOverride = 0.1f;
    const RtSceneFrameInputs overriddenFrame = BuildRtSceneFrameInputs(simulationSnapshot, 0.75f, finaleOverrides);
    ok &= Require(Near(overriddenFrame.lich.finaleSkylightOpenProgress, 0.9f) &&
                      Near(overriddenFrame.lich.finaleDawnRevealProgress, 0.1f) &&
                      Near(simulationSnapshot.lich.finaleSkylightOpenProgress, 0.35f) &&
                      Near(simulationSnapshot.lich.finaleDawnRevealProgress, 0.65f),
                  "RT finale overrides did not resolve into a renderer copy without mutating the simulation snapshot");
    const RtSceneFrameInputs authoredFinaleFrame = BuildRtSceneFrameInputs(simulationSnapshot, 0.75f, authoredTuning);
    ok &= Require(Near(authoredFinaleFrame.lich.finaleSkylightOpenProgress, 0.35f) &&
                      Near(authoredFinaleFrame.lich.finaleDawnRevealProgress, 0.65f),
                  "absent RT finale overrides did not preserve the authored simulation finale state");

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
        const std::string windowsSource =
            ReadTextFile(root / "src/platform/windows/DiagnosticWindow.cpp");
        const std::string androidSource = ReadTextFile(
            root / "android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java");
        const std::string androidBridgeSource =
            ReadTextFile(root / "android/app/src/main/cpp/android_probe_bridge.cpp");
        const std::string androidJavaBridgeSource = ReadTextFile(
            root / "android/app/src/main/java/com/samfa12/hordelanternrt/ProbeBridge.java");
        const std::string androidValidationSource =
            ReadTextFile(root / "tools/run-android-showcase-validation.ps1");
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
        constexpr std::array<const char*, 11u> cpuMaterialAbi{{
            "SurfaceDryStone = 0u", "SurfaceWetCobble = 1u", "SurfaceMossyStone = 2u",
            "SurfaceDampGround = 3u", "SurfaceAgedMetal = 4u", "SurfaceFlame = 5u",
            "SurfaceDarkFigure = 6u", "SurfaceHiddenShell = 7u", "SurfaceMirror = 8u",
            "SurfaceClearGlass = 9u", "SurfaceWater = 10u",
        }};
        constexpr std::array<const char*, 11u> shaderMaterialAbi{{
            "kMaterialDryStone = 0;", "kMaterialWetCobble = 1;", "kMaterialMossyStone = 2;",
            "kMaterialDampGround = 3;", "kMaterialAgedMetal = 4;", "kMaterialFlame = 5;",
            "kMaterialDarkFigure = 6;", "kMaterialHiddenShell = 7;", "kMaterialMirror = 8;",
            "kMaterialClearGlass = 9;", "kMaterialWater = 10;",
        }};
        for (std::size_t material = 0u; material < cpuMaterialAbi.size(); ++material)
        {
            ok &= Require(sceneSource.find(cpuMaterialAbi[material]) != std::string::npos &&
                          raygenSource.find(shaderMaterialAbi[material]) != std::string::npos,
                          "CPU/shader world material ABI changed while appending RT water");
        }
        ok &= Require(sceneSource.find("SurfaceWater = 10u") != std::string::npos &&
                      sceneSource.find("SurfaceWater, SurfaceUp") != std::string::npos &&
                      sceneSource.find("addWaterfallQuad") != std::string::npos &&
                      sceneSource.find("worldSurfaceCodes.size() != sceneIndexCount / 3u") != std::string::npos &&
                      sceneSource.find("waterStreams.size() == 3u") != std::string::npos &&
                      sceneSource.find("falling water streams must retain real ten-centimetre air gaps") != std::string::npos,
                      "world water must use separated metadata-backed geometry and retain the triangle-code contract");
        ok &= Require(sceneSource.find("roundedCatchmentRim") != std::string::npos &&
                      sceneSource.find("addWorldTriangle(waterImpactCentre") != std::string::npos &&
                      sceneSource.find("runoffDrainLipX = -5.30f") != std::string::npos &&
                      sceneSource.find("-5.16f, -1.035f") == std::string::npos,
                      "floor water must use a rounded catchment and remain visible through the drain lip");
        ok &= Require(raygenSource.find("const int kMaterialWater = 10;") != std::string::npos &&
                      raygenSource.find("bool isThinWater") != std::string::npos &&
                      raygenSource.find("material == kMaterialClearGlass || material == kMaterialWater") != std::string::npos &&
                      raygenSource.find("vec4 lichGroundMist") != std::string::npos,
                      "raygen must retain the RT water ABI, transparent visibility route, and bounded lich mist path");
        ok &= Require(sceneSource.find("sizeof(ScenePushConstants) == 120u") != std::string::npos &&
                      sceneSource.find("offsetof(ScenePushConstants, waterQuality) == 72u") != std::string::npos &&
                      sceneSource.find("offsetof(ScenePushConstants, waterfallWidthScale) == 76u") != std::string::npos &&
                      raygenSource.find("float waterQuality;") != std::string::npos &&
                      raygenSource.find("float waterfallWidthScale;") != std::string::npos &&
                      raygenSource.find("float workloadPreset;") != std::string::npos,
                      "RT lab tuning did not append its eleven floats after the released water-quality ABI");
        ok &= Require(sceneSource.find("waterfallBlas_") != std::string::npos &&
                      sceneSource.find("instances[19].instanceCustomIndex = 19u;") != std::string::npos &&
                      sceneSource.find("instances[19].accelerationStructureReference = waterfallBlas_.address;") != std::string::npos &&
                      sceneSource.find("const float waterfallWidthScale = ClampRtSceneTuning(frame.tuning).waterfallWidthScale;") != std::string::npos &&
                      sceneSource.find("waterfallWidthScale, 0.0f, 0.0f, -2.32f") != std::string::npos,
                      "falling streams must use a dedicated local-space BLAS and a non-degenerate tuned TLAS transform");
        ok &= Require(raygenSource.find("const int kWaterfallInstance = 19;") != std::string::npos &&
                      raygenSource.find("candidateInstance == kWaterfallInstance") != std::string::npos &&
                      raygenSource.find("instance == kWaterfallInstance") != std::string::npos &&
                      raygenSource.find("radiusX *= controls.waterfallWidthScale;") != std::string::npos &&
                      raygenSource.find("bool isThinWater = h.material == kMaterialWater;") != std::string::npos,
                      "dedicated waterfall hits must remain transparent water with width-matched shader profiles");
        ok &= Require(raygenSource.find("vec3 tunedLightColor(vec3 authoredColor, int group)") != std::string::npos &&
                      raygenSource.find("if (maximum <= 0.0 || intensityScale <= 0.0)") != std::string::npos &&
                      raygenSource.find("controls.fogDensityScale") != std::string::npos &&
                      raygenSource.find("kLightTorch") != std::string::npos &&
                      raygenSource.find("kLightSkylight") != std::string::npos &&
                      raygenSource.find("kLightPassage") != std::string::npos &&
                      raygenSource.find("kLightStaff") != std::string::npos,
                      "RT light groups need centralized zero-preserving hue/intensity tuning and fog scaling");
        ok &= Require(raygenSource.find(
                          "vec3 authoredStaffEmission = mix(albedo * 0.72, vec3(0.72, 0.12, 1.0), glowMask);") !=
                          std::string::npos &&
                      raygenSource.find(
                          "h.base = glowMask > 0.0 ? tunedLightColor(authoredStaffEmission, kLightStaff)") !=
                          std::string::npos &&
                      raygenSource.find(
                          "vec3 violetEmission = tunedLightColor(vec3(0.72, 0.12, 1.0), kLightStaff);") ==
                          std::string::npos,
                      "zero staff intensity must zero the complete partial-glow lich emissive radiance");
        ok &= Require(raygenSource.find("bool leanWorkload = controls.workloadPreset < 0.5;") != std::string::npos &&
                      raygenSource.find("bool maxWorkload = controls.workloadPreset >= 1.5;") != std::string::npos &&
                      raygenSource.find("if (!leanWorkload)") != std::string::npos &&
                      raygenSource.find("if (maxWorkload)") != std::string::npos &&
                      raygenSource.find("const float sampleCount = 2.0;") != std::string::npos &&
                      raygenSource.find("const float sampleCount = 6.0;") != std::string::npos &&
                      raygenSource.find("const float sampleCount = 8.0;") != std::string::npos &&
                      raygenSource.find("stepLength * 7.5") != std::string::npos,
                      "Lean/Authored/Max must deterministically select no-bounce 2, authored 6, and dual-visibility 8 work");
        ok &= Require(raygenSource.find("waterSheetCoverage") == std::string::npos &&
                      raygenSource.find("bool waterStreamExit") != std::string::npos &&
                      raygenSource.find("controls.waterQuality < 0.5") != std::string::npos &&
                      raygenSource.find("controls.waterQuality >= 1.5") != std::string::npos &&
                      raygenSource.find("transmissionDirection, 12.0, 0x23u, true") != std::string::npos &&
                      raygenSource.find("reflectionDirection, 12.0, 0x37u, false") != std::string::npos,
                      "water quality must retain real ellipse refraction and bounded High/Mobile/Off query routes");
        const std::size_t waterSecondaryBegin = raygenSource.find("vec3 waterSecondarySample");
        const std::size_t waterSecondaryEnd = raygenSource.find("vec3 shadeThinWater", waterSecondaryBegin);
        const std::string waterSecondary =
            waterSecondaryBegin != std::string::npos && waterSecondaryEnd != std::string::npos
                ? raygenSource.substr(waterSecondaryBegin, waterSecondaryEnd - waterSecondaryBegin)
                : std::string{};
        ok &= Require(!waterSecondary.empty() &&
                      waterSecondary.find("traceScene(") == std::string::npos &&
                      waterSecondary.find("visibility(") == std::string::npos &&
                      waterSecondary.find("rayQuery") == std::string::npos &&
                      waterSecondary.find("shadePrimary(") == std::string::npos,
                      "water secondary shading must remain query-free and non-recursive");
        ok &= Require(raygenSource.find("rayBoxInterval(rayOrigin, rayDirection") != std::string::npos &&
                      raygenSource.find("const float sampleCount = 6.0") != std::string::npos &&
                      raygenSource.find("color = color * lichMist.a + lichMist.rgb") != std::string::npos &&
                      raygenSource.find("vec3(-36.65, -0.95, -18.15)") != std::string::npos,
                      "lich mist must retain bounded fixed-step front-to-back integration");
        ok &= Require(windowsSource.find("WaterQuality::High") != std::string::npos &&
                      windowsSource.find("context.waterQuality = horde::vulkan::raytracing::WaterQuality::High") !=
                          std::string::npos &&
                      androidSource.find("WATER_QUALITY_MOBILE = 1") != std::string::npos &&
                      androidSource.find("preferences.getInt(\"water_quality\", WATER_QUALITY_MOBILE)") !=
                          std::string::npos,
                      "platform water-quality defaults changed (Windows/capture High, Android Mobile)");
        ok &= Require(androidBridgeSource.find("else if (context.routeReplayActive)") !=
                          std::string::npos &&
                      androidBridgeSource.find("context.routeReplayActive && !simulationPaused") ==
                          std::string::npos,
                      "authoritative Android route replay must not freeze behind menu/death pause state");
        ok &= Require(androidBridgeSource.find("AndroidRtLabState gRtLabState;") != std::string::npos &&
                      androidBridgeSource.find("const horde::vulkan::raytracing::RtSceneTuning rtLabTuning = gRtLabState.Snapshot();") != std::string::npos &&
                      androidBridgeSource.find("rtLabTuning);") != std::string::npos &&
                      androidBridgeSource.find("gRtLabState.Reset();") != std::string::npos,
                      "Android renderer must consume exactly one mutex-protected RT tuning snapshot and reset it with the route");
        ok &= Require(androidSource.find("PREF_RT_LAB_UNLOCKED = \"rt_lab_unlocked\"") != std::string::npos &&
                      androidSource.find(".putBoolean(PREF_RT_LAB_UNLOCKED, rtLabUnlocked)") != std::string::npos &&
                      androidSource.find("ProbeBridge.isRtLabUnlockEligible()") != std::string::npos &&
                      androidSource.find("handler.postDelayed(this, 250L)") != std::string::npos &&
                      androidSource.find("slider.setMinimumHeight(dp(48))") != std::string::npos &&
                      androidSource.find("button.setSingleLine(true)") != std::string::npos &&
                      androidSource.find("button.setAutoSizeTextTypeUniformWithConfiguration(") !=
                          std::string::npos &&
                      androidSource.find("row.setOrientation(LinearLayout.VERTICAL)") !=
                          std::string::npos &&
                      androidSource.find("LinearLayout.LayoutParams.MATCH_PARENT, dp(48)") !=
                          std::string::npos &&
                      androidSource.find("private void showRtLab()") != std::string::npos &&
                      androidSource.find("ProbeBridge.setSimulationPaused(true)") != std::string::npos,
                      "Android RT Lab must preserve progress through settings reset and expose a live paused 48dp panel");
        ok &= Require(androidJavaBridgeSource.find("setRtSceneTuning") != std::string::npos &&
                      androidJavaBridgeSource.find("setRtLightTuning") != std::string::npos &&
                      androidJavaBridgeSource.find("setRtWorkloadPreset") != std::string::npos &&
                      androidJavaBridgeSource.find("resetRtSceneTuning") != std::string::npos &&
                      androidJavaBridgeSource.find("getRtGpuFrameTimeMilliseconds") != std::string::npos &&
                      androidJavaBridgeSource.find("getRtGpuSampleCount") != std::string::npos &&
                      androidJavaBridgeSource.find("getCurrentRenderScalePercent") != std::string::npos &&
                      androidJavaBridgeSource.find("getCurrentWaterQuality") != std::string::npos,
                      "ProbeBridge does not expose the typed RT Lab tuning and telemetry API");
        ok &= Require(androidValidationSource.find("[switch]$RtLabWorkloadComparison") != std::string::npos &&
                      androidValidationSource.find("@('lantern-drop', 'skylight', 'finale-roof')") !=
                          std::string::npos &&
                      androidValidationSource.find("horde.debug.rt_workload") != std::string::npos &&
                      androidValidationSource.find("@{ name = 'lean'; workload = 0 }") !=
                          std::string::npos &&
                      androidValidationSource.find("rt_lab_profile") != std::string::npos &&
                      androidValidationSource.find(
                          "if ($RtWorkload -ge 0) { Invoke-AdbText @(\"logcat\", \"-c\")") !=
                          std::string::npos &&
                      androidBridgeSource.find("\\\"waterQuality\\\":") != std::string::npos &&
                      androidBridgeSource.find("\\\"rtLab\\\":") != std::string::npos,
                      "Android validation must retain matched Authored/Max RT Lab evidence with explicit renderer state");

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
