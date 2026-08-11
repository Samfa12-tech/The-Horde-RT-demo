#include "vulkan/raytracing/CharacterRenderSlot.h"

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

horde::scene::SkeletonClip SkeletonClipForAnimation(horde::gameplay::EnemyAnimation animation)
{
    return animation == horde::gameplay::EnemyAnimation::Walking
        ? horde::scene::SkeletonClip::Walking
        : (animation == horde::gameplay::EnemyAnimation::Attack
               ? horde::scene::SkeletonClip::Attack
               : (animation == horde::gameplay::EnemyAnimation::Dead
                      ? horde::scene::SkeletonClip::Dead
                      : horde::scene::SkeletonClip::Idle));
}

float SkeletonTimeForAnimation(horde::gameplay::EnemyAnimation animation,
                               float animationTime,
                               float deadClipDuration)
{
    if (animation == horde::gameplay::EnemyAnimation::Walking)
    {
        return animationTime * 0.90f;
    }
    if (animation == horde::gameplay::EnemyAnimation::Dead && deadClipDuration > 0.0f)
    {
        return std::min(animationTime, deadClipDuration);
    }
    return animationTime;
}

VkTransformMatrixKHR SkeletonInstanceTransform(float x, float z, float facingRadians)
{
    const float enemyCos = std::cos(facingRadians);
    const float enemySin = std::sin(facingRadians);
    return {{
        enemyCos, 0.0f, enemySin, x,
        0.0f, 1.0f, 0.0f, -0.95f,
        -enemySin, 0.0f, enemyCos, z}};
}

VkTransformMatrixKHR LichInstanceTransform(const horde::gameplay::LichSnapshot& lich)
{
    const float hitRecoil = std::clamp(lich.hitRecoil, 0.0f, 1.0f);
    const float activeX = lich.x - std::sin(lich.facingRadians) * hitRecoil * 0.18f;
    const float activeY = lich.y + hitRecoil * 0.035f;
    const float activeZ = lich.z - std::cos(lich.facingRadians) * hitRecoil * 0.18f;
    const float yawCos = std::cos(lich.facingRadians);
    const float yawSin = std::sin(lich.facingRadians);
    const float lean = -0.17f * hitRecoil;
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

} // namespace

CharacterRenderPlan EvaluateCharacterRenderPlan(
    const horde::gameplay::CombatSnapshot& combat,
    const horde::gameplay::EnemyRosterSnapshot& roster,
    const horde::gameplay::LichSnapshot& lich)
{
    CharacterRenderPlan plan;
    plan.selectedLich = roster.selectedEnemy == horde::gameplay::EnemyKind::Lich;
    plan.skeletonClip = SkeletonClipForAnimation(combat.enemyAnimation);
    plan.skeletonTime = SkeletonTimeForAnimation(combat.enemyAnimation, combat.enemyAnimationTime, 0.0f);
    plan.lichClip = lich.phase == horde::gameplay::LichPhase::Dead
        ? horde::scene::SkinnedClip::Dead
        : horde::scene::SkinnedClip::Idle;
    plan.lichTime = lich.animationTime;

    if (plan.selectedLich)
    {
        plan.transform = LichInstanceTransform(lich);
    }
    else
    {
        plan.transform = SkeletonInstanceTransform(combat.enemyX, combat.enemyZ, combat.enemyFacingRadians);
    }
    return plan;
}

CharacterFramePlan EvaluateCharacterFramePlan(
    const std::array<horde::gameplay::simulation::SkeletonEnemySnapshot,
                     horde::gameplay::simulation::kSkeletonEnemyCapacity>& skeletons,
    const std::size_t skeletonCount,
    const horde::gameplay::EnemyRosterSnapshot& roster,
    const float skeletonDeadClipDuration)
{
    CharacterFramePlan plan;
    plan.selectedLich = roster.selectedEnemy == horde::gameplay::EnemyKind::Lich;
    if (plan.selectedLich)
    {
        return plan;
    }

    plan.skeletonCount = std::min<std::size_t>(skeletonCount, plan.skeletons.size());
    for (std::size_t skeletonIndex = 0u; skeletonIndex < plan.skeletonCount; ++skeletonIndex)
    {
        const auto& source = skeletons[skeletonIndex];
        auto& destination = plan.skeletons[skeletonIndex];
        destination.clip = SkeletonClipForAnimation(source.animation);
        destination.time = SkeletonTimeForAnimation(
            source.animation, source.animationTime, skeletonDeadClipDuration);
        destination.transform = SkeletonInstanceTransform(source.x, source.z, source.facingRadians);
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
    return plan;
}

bool CharacterPoseNeedsRefresh(const int requestedClip,
                               const float requestedTime,
                               const int lastClip,
                               const float lastTime,
                               const float updateInterval)
{
    return requestedClip != lastClip || lastTime < 0.0f || requestedTime < lastTime ||
           (requestedTime - lastTime) >= updateInterval;
}

bool CharacterRenderSlot::LoadAssets(const std::string& skeletonAssetPath,
                                     const std::string& lichAssetPath,
                                     std::string& diagnostic)
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

bool CharacterRenderSlot::PrepareFrame(
    const std::array<horde::gameplay::simulation::SkeletonEnemySnapshot,
                     horde::gameplay::simulation::kSkeletonEnemyCapacity>& skeletons,
    const std::size_t skeletonCount,
    const horde::gameplay::EnemyRosterSnapshot& roster,
    const horde::gameplay::LichSnapshot& lich,
    const RtGpuResources& resources,
    std::string& diagnostic)
{
    pendingRefit_ = CharacterBlasRefit::None;
    activeSkeletonCount_ = 0u;
    skeletonPoseBucketCount_ = 0u;
    if (skeletonCount > kMaximumActiveSkeletons)
    {
        diagnostic = "CharacterRenderSlot supports at most two active skeletons; the frame exceeded that limit.";
        return false;
    }
    const CharacterFramePlan framePlan = EvaluateCharacterFramePlan(
        skeletons, skeletonCount, roster, skeletonDeadClipDuration_);
    activeSkeletonCount_ = framePlan.skeletonCount;
    skeletonPoseBucketCount_ = framePlan.skeletonPoseBucketCount;
    if (!framePlan.selectedLich)
    {
        for (std::size_t bucket = 0u; bucket < framePlan.skeletonPoseBucketCount; ++bucket)
        {
            const auto representative = std::find_if(
                framePlan.skeletons.begin(),
                framePlan.skeletons.begin() + framePlan.skeletonCount,
                [bucket](const SkeletonRenderPlan& skeleton) { return skeleton.poseBucket == bucket; });
            if (representative == framePlan.skeletons.begin() + framePlan.skeletonCount)
            {
                diagnostic = "CharacterRenderSlot produced an empty skeleton pose bucket.";
                return false;
            }
            const int clipIndex = static_cast<int>(representative->clip);
            if (!CharacterPoseNeedsRefresh(clipIndex,
                                           representative->time,
                                           lastSkeletonClips_[bucket],
                                           lastSkeletonUpdateTimes_[bucket]))
            {
                continue;
            }
            auto& vertices = skeletonSkinnedVertices_[bucket];
            if (!skeletonModel_.Skin(representative->clip, representative->time, vertices, diagnostic))
            {
                return false;
            }
            const VkDeviceSize byteSize = sizeof(horde::scene::SkinnedRtVertex) * vertices.size();
            if (!resources.WriteBuffer(skeletonGpus_[bucket].vertices,
                                       vertices.data(),
                                       byteSize,
                                       bucket == 0u ? "animated skeleton pose 0 vertex" : "animated skeleton pose 1 vertex",
                                       diagnostic))
            {
                return false;
            }
            lastSkeletonUpdateTimes_[bucket] = representative->time;
            lastSkeletonClips_[bucket] = clipIndex;
            pendingRefit_ = pendingRefit_ |
                (bucket == 0u ? CharacterBlasRefit::SkeletonPose0 : CharacterBlasRefit::SkeletonPose1);
        }
    }
    else
    {
        const CharacterRenderPlan lichPlan = EvaluateCharacterRenderPlan({}, roster, lich);
        const int clipIndex = static_cast<int>(lichPlan.lichClip);
        if (CharacterPoseNeedsRefresh(clipIndex, lichPlan.lichTime, lastLichClip_, lastLichUpdateTime_))
        {
            if (!lichModel_.SkinTextured(lichPlan.lichClip, lichPlan.lichTime, lichSkinnedVertices_, diagnostic))
            {
                return false;
            }
            const VkDeviceSize byteSize = sizeof(horde::scene::TexturedSkinnedRtVertex) * lichSkinnedVertices_.size();
            if (!resources.WriteBuffer(lichGpu_.vertices, lichSkinnedVertices_.data(), byteSize,
                                       "animated lich vertex", diagnostic) ||
                !UpdateLichStaffSample(diagnostic))
            {
                return false;
            }
            lastLichUpdateTime_ = lichPlan.lichTime;
            lastLichClip_ = clipIndex;
            pendingRefit_ = CharacterBlasRefit::Lich;
        }
    }
    diagnostic.clear();
    return true;
}

std::array<VkAccelerationStructureInstanceKHR, CharacterRenderSlot::kMaximumActiveSkeletons>
CharacterRenderSlot::BuildActiveInstances(
    const std::array<horde::gameplay::simulation::SkeletonEnemySnapshot,
                     horde::gameplay::simulation::kSkeletonEnemyCapacity>& skeletons,
    const std::size_t skeletonCount,
    const horde::gameplay::EnemyRosterSnapshot& roster,
    const horde::gameplay::LichSnapshot& lich) const
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
    const CharacterFramePlan framePlan = EvaluateCharacterFramePlan(
        skeletons, skeletonCount, roster, skeletonDeadClipDuration_);
    if (framePlan.selectedLich)
    {
        instances[0].transform = LichInstanceTransform(lich);
        instances[0].instanceCustomIndex = kTlasInstanceIndex;
        instances[0].mask = 0x01u;
        instances[0].accelerationStructureReference = lichGpu_.accelerationStructure.address;
        return instances;
    }

    for (std::size_t index = 0u; index < framePlan.skeletonCount; ++index)
    {
        const auto& plan = framePlan.skeletons[index];
        auto& instance = instances[index];
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
    lastLichClip_ = -1;
    pendingRefit_ = CharacterBlasRefit::None;
    activeSkeletonCount_ = 0u;
    skeletonPoseBucketCount_ = 0u;
    skeletonDeadClipDuration_ = 0.0f;
    lichStaffLocalSample_ = {{0.94f, 0.79f, 0.64f}};
}

} // namespace horde::vulkan::raytracing
