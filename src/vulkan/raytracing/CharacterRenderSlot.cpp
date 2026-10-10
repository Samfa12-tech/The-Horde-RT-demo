#include "vulkan/raytracing/CharacterRenderSlot.h"
#include "vulkan/raytracing/RtSceneRecordObservation.h"
#include "scene/SkeletonRenderPose.h"

#include <algorithm>
#include <cmath>

namespace horde::vulkan::raytracing
{

namespace
{

constexpr std::array<std::uint32_t, 40u> kLichStaffEmissiveVertices{{
    15436u, 16369u, 16370u, 16373u, 17107u, 17124u, 17125u, 17339u,
    17685u, 17686u, 17687u, 17988u, 17989u, 17990u, 17994u, 17995u,
    17996u, 18822u, 18823u, 18826u, 18829u, 18835u, 19152u, 19153u,
    19154u, 19174u, 19792u, 20010u, 20011u, 20012u, 20385u, 20387u,
    20388u, 20389u, 20390u, 20625u, 20845u, 20846u, 21255u, 25309u}};

VkTransformMatrixKHR LichInstanceTransform(const horde::gameplay::LichSnapshot& lich)
{
    const float hitRecoil = std::clamp(lich.hitRecoil, 0.0f, 1.0f);
    const float activeX = lich.x - std::sin(lich.facingRadians) * hitRecoil * 0.18f;
    const float activeY = lich.y + hitRecoil * 0.035f;
    const float activeZ = lich.z - std::cos(lich.facingRadians) * hitRecoil * 0.18f;
    const float yawCos = std::cos(lich.facingRadians);
    const float yawSin = std::sin(lich.facingRadians);
    const float lean = lich.presentationTiltRadians - 0.17f * hitRecoil;
    const float leanCos = std::cos(lean);
    const float leanSin = std::sin(lean);
    return {{
        yawCos, yawSin * leanSin, yawSin * leanCos, activeX,
        0.0f, leanCos, -leanSin, activeY,
        -yawSin, yawCos * leanSin, yawCos * leanCos, activeZ}};
}

std::array<float, 3u> TransformPoint(const VkTransformMatrixKHR& transform,
                                    const std::array<float, 3u>& point)
{
    return {{
        transform.matrix[0][0] * point[0] + transform.matrix[0][1] * point[1] +
            transform.matrix[0][2] * point[2] + transform.matrix[0][3],
        transform.matrix[1][0] * point[0] + transform.matrix[1][1] * point[1] +
            transform.matrix[1][2] * point[2] + transform.matrix[1][3],
        transform.matrix[2][0] * point[0] + transform.matrix[2][1] * point[1] +
            transform.matrix[2][2] * point[2] + transform.matrix[2][3]}};
}

std::array<float, 4u> RotationAroundX(const float radians)
{
    const float halfAngle = radians * 0.5f;
    return {{std::sin(halfAngle), 0.0f, 0.0f, std::cos(halfAngle)}};
}

float SmoothStep01(const float value)
{
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return clamped * clamped * (3.0f - 2.0f * clamped);
}

} // namespace

CharacterFramePlan EvaluateCharacterFramePlan(
    const std::array<horde::gameplay::simulation::SkeletonEnemySnapshot,
                     horde::gameplay::simulation::kSkeletonEnemyCapacity>& skeletons,
    const std::size_t skeletonCount,
    const horde::gameplay::EnemyRosterSnapshot& roster,
    const horde::gameplay::LichSnapshot& lich,
    const float skeletonDeadClipDuration, const bool retainedWorkloadSkeleton)
{
    CharacterFramePlan plan;
    plan.retainedWorkloadSkeleton=retainedWorkloadSkeleton;
    plan.selectedLich = roster.selectedEnemy == horde::gameplay::EnemyKind::Lich;
    plan.lichClip = lich.phase == horde::gameplay::LichPhase::Dead
        ? horde::scene::SkinnedClip::Dead
        : horde::scene::SkinnedClip::Idle;
    plan.lichTime = lich.animationTime;
    if (lich.readableCastPresentation && lich.phase == horde::gameplay::LichPhase::Charging)
    {
        const float chargeFraction = std::clamp(
            lich.phaseTime / horde::gameplay::LichEncounter::kChargeDuration, 0.0f, 1.0f);
        // Reach the held anticipation pose before discharge and keep it through
        // the final fifth of the authoritative 1.20 second charge.
        plan.lichStaffLiftRadians = 0.58f * SmoothStep01(chargeFraction / 0.80f);
    }
    else if (lich.readableCastPresentation && lich.phase == horde::gameplay::LichPhase::Recovering)
    {
        const float castFade = 1.0f - SmoothStep01(
            lich.phaseTime / horde::gameplay::LichEncounter::kDischargeVisibleBurstDuration);
        plan.lichStaffLiftRadians = 0.58f * castFade;
        plan.lichStaffCastRadians = 0.46f * castFade;
    }
    plan.lichTransform = LichInstanceTransform(lich);
    if (plan.selectedLich && !retainedWorkloadSkeleton)
    {
        return plan;
    }

    plan.skeletonCount = std::min<std::size_t>(skeletonCount, plan.skeletons.size());
    for (std::size_t skeletonIndex = 0u; skeletonIndex < plan.skeletonCount; ++skeletonIndex)
    {
        const auto& source = skeletons[skeletonIndex];
        auto& destination = plan.skeletons[skeletonIndex];
        const horde::scene::SkeletonRenderPose pose =
            horde::scene::EvaluateSkeletonRenderPose(source, skeletonDeadClipDuration);
        destination.clip = pose.clip;
        destination.time = pose.time;
        destination.forceCurrentCombatPose = source.forceCurrentCombatPose;
        destination.transform = {{
            {pose.transform[0], pose.transform[1], pose.transform[2], pose.transform[3]},
            {pose.transform[4], pose.transform[5], pose.transform[6], pose.transform[7]},
            {pose.transform[8], pose.transform[9], pose.transform[10], pose.transform[11]}}};
        destination.poseBucket = static_cast<std::uint32_t>(plan.skeletonPoseBucketCount);
        for (std::size_t previousIndex = 0u; previousIndex < skeletonIndex; ++previousIndex)
        {
            const auto& previous = plan.skeletons[previousIndex];
            if (destination.clip == previous.clip && std::abs(destination.time - previous.time) <= 0.0001f)
            {
                destination.poseBucket = previous.poseBucket;
                break;
            }
        }
        if (destination.poseBucket == plan.skeletonPoseBucketCount)
        {
            ++plan.skeletonPoseBucketCount;
        }
    }
    if(retainedWorkloadSkeleton && plan.selectedLich && plan.skeletonCount) {
        plan.skeletonCount=1;plan.skeletons[0].poseBucket=1;plan.skeletonPoseBucketCount=2;
    }
    return plan;
}

bool CharacterPoseNeedsRefresh(const int requestedClip,
                               const float requestedTime,
                               const int lastClip,
                               const float lastTime,
                               const float updateInterval,
                               const bool forceCurrentCombatPose)
{
    return requestedClip != lastClip || lastTime < 0.0f || requestedTime < lastTime ||
           (forceCurrentCombatPose && requestedTime != lastTime) ||
           (requestedTime - lastTime) >= updateInterval;
}

bool CharacterRenderSlot::LoadAssets(const std::string& skeletonAssetPath,
                                     const std::string& lichAssetPath,
                                     std::string& diagnostic,
                                     const bool skeletonOnly)
{
    if (!skeletonModel_.LoadCombatClips(skeletonAssetPath, diagnostic))
    {
        return false;
    }
    skeletonDeadClipDuration_ = skeletonModel_.ClipDuration(horde::scene::SkeletonClip::Dead);
    if (skeletonDeadClipDuration_ <= 0.0f)
    {
        diagnostic = "The skeleton Dead clip has no usable duration.";
        return false;
    }
    skeletonOnly_ = skeletonOnly;
    if (skeletonOnly_)
    {
        lichModel_ = {};
        std::vector<horde::scene::TexturedSkinnedRtVertex>{}.swap(lichSkinnedVertices_);
        std::vector<horde::scene::SkinnedRtVertex>{}.swap(skeletonSkinnedVertices_[1]);
        diagnostic.clear();
        return true;
    }
    return lichModel_.LoadClips(
        lichAssetPath, horde::scene::LichPlaceholderClipSet(), diagnostic);
}

bool CharacterRenderSlot::PrepareInitialGeometry(std::string& diagnostic)
{
    if (!skeletonModel_.Skin(horde::scene::SkeletonClip::Idle, 0.0f, skeletonSkinnedVertices_[0], diagnostic) ||
        skeletonSkinnedVertices_[0].empty())
    {
        if (diagnostic.empty()) diagnostic = "Skeleton produced no skinned vertices.";
        return false;
    }
    if (skeletonOnly_)
    {
        diagnostic.clear();
        return true;
    }
    skeletonSkinnedVertices_[1] = skeletonSkinnedVertices_[0];
    if (!lichModel_.SkinTextured(horde::scene::SkinnedClip::Idle, 0.0f, lichSkinnedVertices_, diagnostic) ||
        lichSkinnedVertices_.empty())
    {
        if (diagnostic.empty()) diagnostic = "Lich produced no textured skinned vertices.";
        return false;
    }
    return UpdateLichStaffSample(diagnostic);
}

bool CharacterRenderSlot::UpdateLichStaffSample(std::string& diagnostic)
{
    lichStaffLocalSample_ = {{0.0f, 0.0f, 0.0f}};
    for (std::uint32_t vertexIndex : kLichStaffEmissiveVertices)
    {
        if (vertexIndex >= lichSkinnedVertices_.size())
        {
            diagnostic = "The audited lich staff emissive vertex set no longer matches the placeholder mesh.";
            return false;
        }
        for (std::size_t axis = 0u; axis < 3u; ++axis)
        {
            lichStaffLocalSample_[axis] += lichSkinnedVertices_[vertexIndex].position[axis];
        }
    }
    for (float& component : lichStaffLocalSample_)
    {
        component /= static_cast<float>(kLichStaffEmissiveVertices.size());
    }
    diagnostic.clear();
    return true;
}

bool CharacterRenderSlot::CacheFramePlan(
    const std::array<horde::gameplay::simulation::SkeletonEnemySnapshot,
                     horde::gameplay::simulation::kSkeletonEnemyCapacity>& skeletons,
    const std::size_t skeletonCount,
    const horde::gameplay::EnemyRosterSnapshot& roster,
    const horde::gameplay::LichSnapshot& lich,
    std::string& diagnostic, const bool retainedWorkloadSkeleton)
{
    if (skeletonOnly_ && (skeletonCount > 1u ||
        roster.selectedEnemy != horde::gameplay::EnemyKind::Skeleton))
    {
        diagnostic = "Skeleton-only CharacterRenderSlot admits one skeleton and no lich.";
        return false;
    }
    if (skeletonCount > kMaximumActiveSkeletons)
    {
        diagnostic = "CharacterRenderSlot supports at most two active skeletons; the frame exceeded that limit.";
        return false;
    }
    cachedFramePlan_ = EvaluateCharacterFramePlan(
        skeletons, skeletonCount, roster, lich, skeletonDeadClipDuration_,retainedWorkloadSkeleton);
    skeletonPoseBucketCount_ = cachedFramePlan_.skeletonPoseBucketCount;
    diagnostic.clear();
    return true;
}

bool CharacterRenderSlot::PrepareFrame(
    const std::array<horde::gameplay::simulation::SkeletonEnemySnapshot,
                     horde::gameplay::simulation::kSkeletonEnemyCapacity>& skeletons,
    const std::size_t skeletonCount,
    const horde::gameplay::EnemyRosterSnapshot& roster,
    const horde::gameplay::LichSnapshot& lich,
    const RtGpuResources& resources,
    std::string& diagnostic,
    RtSceneRecordObservation* observation, const bool retainedWorkloadSkeleton)
{
    pendingRefit_ = CharacterBlasRefit::None;
    if (!CacheFramePlan(skeletons, skeletonCount, roster, lich, diagnostic,retainedWorkloadSkeleton))
    {
        return false;
    }
    const CharacterFramePlan& framePlan = cachedFramePlan_;
    if (!framePlan.selectedLich || framePlan.retainedWorkloadSkeleton)
    {
        for (std::size_t bucket = 0u; bucket < framePlan.skeletonPoseBucketCount; ++bucket)
        {
            const auto representative = std::find_if(
                framePlan.skeletons.begin(),
                framePlan.skeletons.begin() + framePlan.skeletonCount,
                [bucket](const SkeletonRenderPlan& skeleton) { return skeleton.poseBucket == bucket; });
            if (representative == framePlan.skeletons.begin() + framePlan.skeletonCount)
            {
                if(framePlan.retainedWorkloadSkeleton && bucket==0) continue;
                diagnostic = "CharacterRenderSlot produced an empty skeleton pose bucket.";
                return false;
            }
            const int clipIndex = static_cast<int>(representative->clip);
            const bool forceCurrentCombatPose = std::any_of(
                framePlan.skeletons.begin(), framePlan.skeletons.begin() + framePlan.skeletonCount,
                [bucket](const SkeletonRenderPlan& skeleton) {
                    return skeleton.poseBucket == bucket && skeleton.forceCurrentCombatPose;
                });
            if (!CharacterPoseNeedsRefresh(clipIndex,
                                           representative->time,
                                           lastSkeletonClips_[bucket],
                                           lastSkeletonUpdateTimes_[bucket], 1.0f / 30.0f,
                                           forceCurrentCombatPose))
            {
                continue;
            }
            auto& vertices = skeletonSkinnedVertices_[bucket];
            RtSceneStageScope skinScope(
                observation, horde::telemetry::RtStage::CharacterSkin);
            if (!skeletonModel_.Skin(representative->clip, representative->time, vertices, diagnostic))
            {
                skinScope.Cancel();
                return false;
            }
            skinScope.Complete(1u);
            const VkDeviceSize byteSize = sizeof(horde::scene::SkinnedRtVertex) * vertices.size();
            if (!resources.WriteBuffer(skeletonGpus_[bucket].vertices,
                                       vertices.data(),
                                       byteSize,
                                       bucket == 0u ? "animated skeleton pose 0 vertex" : "animated skeleton pose 1 vertex",
                                       diagnostic,
                                       observation))
            {
                return false;
            }
            lastSkeletonUpdateTimes_[bucket] = representative->time;
            lastSkeletonClips_[bucket] = clipIndex;
            pendingRefit_ = pendingRefit_ |
                (bucket == 0u ? CharacterBlasRefit::SkeletonPose0 : CharacterBlasRefit::SkeletonPose1);
        }
    }
    if(framePlan.selectedLich)
    {
        const int clipIndex = static_cast<int>(framePlan.lichClip);
        const bool castPoseChanged =
            std::abs(framePlan.lichStaffLiftRadians - lastLichStaffLiftRadians_) > 0.0001f ||
            std::abs(framePlan.lichStaffCastRadians - lastLichStaffCastRadians_) > 0.0001f;
        if (castPoseChanged || CharacterPoseNeedsRefresh(
                clipIndex, framePlan.lichTime, lastLichClip_, lastLichUpdateTime_))
        {
            RtSceneStageScope skinScope(
                observation, horde::telemetry::RtStage::CharacterSkin);
            bool skinned = false;
            if (framePlan.lichStaffLiftRadians == 0.0f && framePlan.lichStaffCastRadians == 0.0f)
            {
                // Keep dormant, dead, and legacy capture skins byte-identical
                // to the authored clip path.
                skinned = lichModel_.SkinTextured(
                    framePlan.lichClip, framePlan.lichTime, lichSkinnedVertices_, diagnostic);
            }
            else
            {
                const std::array<horde::scene::SkinnedNodeRotation, 2u> castPose{{
                    {"LeftArm", RotationAroundX(-framePlan.lichStaffLiftRadians -
                                                  framePlan.lichStaffCastRadians)},
                    {"LeftForeArm", RotationAroundX(-0.72f * framePlan.lichStaffLiftRadians)}}};
                skinned = lichModel_.SkinTextured(framePlan.lichClip, framePlan.lichTime,
                                                  lichSkinnedVertices_, diagnostic, castPose);
            }
            if (!skinned)
            {
                skinScope.Cancel();
                return false;
            }
            skinScope.Complete(1u);
            const VkDeviceSize byteSize = sizeof(horde::scene::TexturedSkinnedRtVertex) * lichSkinnedVertices_.size();
            if (!resources.WriteBuffer(lichGpu_.vertices, lichSkinnedVertices_.data(), byteSize,
                                       "animated lich vertex", diagnostic, observation) ||
                !UpdateLichStaffSample(diagnostic))
            {
                return false;
            }
            lastLichUpdateTime_ = framePlan.lichTime;
            lastLichClip_ = clipIndex;
            lastLichStaffLiftRadians_ = framePlan.lichStaffLiftRadians;
            lastLichStaffCastRadians_ = framePlan.lichStaffCastRadians;
            pendingRefit_ = pendingRefit_ | CharacterBlasRefit::Lich;
        }
    }
    diagnostic.clear();
    return true;
}

void CharacterRenderSlot::AccumulateResourceInventory(
    horde::telemetry::RtResourceInventory& inventory) const noexcept
{
    const auto accumulate = [&inventory](const RtUpdatableTriangleBlas& gpu) {
        AccumulateRtGpuBuffer(inventory, gpu.vertices);
        AccumulateRtGpuBuffer(inventory, gpu.accelerationStructure.backing);
        AccumulateRtGpuBuffer(inventory, gpu.updateScratch);
        if (gpu.accelerationStructure.handle != VK_NULL_HANDLE)
        {
            AccumulateRtResourceCount(inventory.bottomLevelAccelerationStructureCount);
        }
    };
    for (const auto& gpu : skeletonGpus_)
    {
        accumulate(gpu);
    }
    accumulate(lichGpu_);
}

std::array<VkAccelerationStructureInstanceKHR, CharacterRenderSlot::kMaximumActiveSkeletons>
CharacterRenderSlot::BuildActiveInstances() const
{
    std::array<VkAccelerationStructureInstanceKHR, kMaximumActiveSkeletons> instances{};
    for (std::size_t index = 0u; index < instances.size(); ++index)
    {
        auto& instance = instances[index];
        instance.transform = {{
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f}};
        instance.instanceCustomIndex = index == 0u
            ? kTlasInstanceIndex
            : kSecondSkeletonTlasInstanceIndex;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.accelerationStructureReference = skeletonGpus_[0].accelerationStructure.address;
    }
    const CharacterFramePlan& framePlan = cachedFramePlan_;
    if (framePlan.selectedLich)
    {
        instances[0].transform = framePlan.lichTransform;
        instances[0].instanceCustomIndex = kTlasInstanceIndex;
        instances[0].mask = 0x01u;
        instances[0].accelerationStructureReference = lichGpu_.accelerationStructure.address;
        if(!framePlan.retainedWorkloadSkeleton) return instances;
    }

    for (std::size_t index = 0u; index < framePlan.skeletonCount; ++index)
    {
        const auto& plan = framePlan.skeletons[index];
        auto& instance = instances[framePlan.selectedLich?index+1:index];
        instance.transform = plan.transform;
        instance.instanceCustomIndex = plan.poseBucket == 0u
            ? kTlasInstanceIndex
            : kSecondSkeletonTlasInstanceIndex;
        instance.mask = 0x01u;
        instance.accelerationStructureReference = skeletonGpus_[plan.poseBucket].accelerationStructure.address;
    }
    return instances;
}

std::array<float, 3u> CharacterRenderSlot::LichStaffWorldPosition(
    const horde::gameplay::LichSnapshot& lich) const
{
    return TransformPoint(LichInstanceTransform(lich), lichStaffLocalSample_);
}

void CharacterRenderSlot::DestroyGpuResources(const RtGpuResources& resources)
{
    for (auto& skeletonGpu : skeletonGpus_)
    {
        resources.DestroyAccelerationStructure(skeletonGpu.accelerationStructure);
        resources.DestroyBuffer(skeletonGpu.updateScratch);
        resources.DestroyBuffer(skeletonGpu.vertices);
        skeletonGpu = {};
    }
    resources.DestroyAccelerationStructure(lichGpu_.accelerationStructure);
    resources.DestroyBuffer(lichGpu_.updateScratch);
    resources.DestroyBuffer(lichGpu_.vertices);
    lichGpu_ = {};
    lastSkeletonUpdateTimes_.fill(-1.0f);
    lastSkeletonClips_.fill(-1);
    lastLichUpdateTime_ = -1.0f;
    lastLichStaffLiftRadians_ = -1.0f;
    lastLichStaffCastRadians_ = -1.0f;
    lastLichClip_ = -1;
    pendingRefit_ = CharacterBlasRefit::None;
    skeletonPoseBucketCount_ = 0u;
    skeletonDeadClipDuration_ = 0.0f;
    cachedFramePlan_ = {};
    lichStaffLocalSample_ = {{0.94f, 0.79f, 0.64f}};
}

} // namespace horde::vulkan::raytracing
