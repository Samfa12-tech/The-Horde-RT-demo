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
    plan.skeletonClip = combat.enemyAnimation == horde::gameplay::EnemyAnimation::Walking
        ? horde::scene::SkeletonClip::Walking
        : (combat.enemyAnimation == horde::gameplay::EnemyAnimation::Attack
               ? horde::scene::SkeletonClip::Attack
               : (combat.enemyAnimation == horde::gameplay::EnemyAnimation::Dead
                      ? horde::scene::SkeletonClip::Dead
                      : horde::scene::SkeletonClip::Idle));
    plan.skeletonTime = combat.enemyAnimation == horde::gameplay::EnemyAnimation::Walking
        ? combat.enemyAnimationTime * 0.90f
        : combat.enemyAnimationTime;
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
        const float enemyCos = std::cos(combat.enemyFacingRadians);
        const float enemySin = std::sin(combat.enemyFacingRadians);
        plan.transform = {{
            enemyCos, 0.0f, enemySin, combat.enemyX,
            0.0f, 1.0f, 0.0f, -0.95f,
            -enemySin, 0.0f, enemyCos, combat.enemyZ}};
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
    return skeletonModel_.LoadCombatClips(skeletonAssetPath, diagnostic) &&
           lichModel_.LoadClips(lichAssetPath, horde::scene::LichPlaceholderClipSet(), diagnostic);
}

bool CharacterRenderSlot::PrepareInitialGeometry(std::string& diagnostic)
{
    if (!skeletonModel_.Skin(horde::scene::SkeletonClip::Idle, 0.0f, skeletonSkinnedVertices_, diagnostic) ||
        skeletonSkinnedVertices_.empty())
    {
        if (diagnostic.empty()) diagnostic = "Skeleton produced no skinned vertices.";
        return false;
    }
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

bool CharacterRenderSlot::PrepareFrame(const horde::gameplay::CombatSnapshot& combat,
                                       const horde::gameplay::EnemyRosterSnapshot& roster,
                                       const horde::gameplay::LichSnapshot& lich,
                                       const RtGpuResources& resources,
                                       std::string& diagnostic)
{
    pendingRefit_ = CharacterBlasRefit::None;
    if (roster.renderedEnemyCount > kMaximumActiveCharacters)
    {
        diagnostic = "CharacterRenderSlot supports exactly one active character; the roster exceeded that limit.";
        return false;
    }
    if (roster.renderedEnemyCount == 1u && roster.renderedEnemies[0] != roster.selectedEnemy)
    {
        diagnostic = "CharacterRenderSlot roster selection does not match its active character.";
        return false;
    }
    const CharacterRenderPlan plan = EvaluateCharacterRenderPlan(combat, roster, lich);
    if (!plan.selectedLich)
    {
        const int clipIndex = static_cast<int>(plan.skeletonClip);
        if (CharacterPoseNeedsRefresh(clipIndex, plan.skeletonTime, lastSkeletonClip_, lastSkeletonUpdateTime_))
        {
            if (!skeletonModel_.Skin(plan.skeletonClip, plan.skeletonTime, skeletonSkinnedVertices_, diagnostic))
            {
                return false;
            }
            const VkDeviceSize byteSize = sizeof(horde::scene::SkinnedRtVertex) * skeletonSkinnedVertices_.size();
            if (!resources.WriteBuffer(skeletonGpu_.vertices, skeletonSkinnedVertices_.data(), byteSize,
                                       "animated skeleton vertex", diagnostic))
            {
                return false;
            }
            lastSkeletonUpdateTime_ = plan.skeletonTime;
            lastSkeletonClip_ = clipIndex;
            pendingRefit_ = CharacterBlasRefit::Skeleton;
        }
    }
    else
    {
        const int clipIndex = static_cast<int>(plan.lichClip);
        if (CharacterPoseNeedsRefresh(clipIndex, plan.lichTime, lastLichClip_, lastLichUpdateTime_))
        {
            if (!lichModel_.SkinTextured(plan.lichClip, plan.lichTime, lichSkinnedVertices_, diagnostic))
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
            lastLichUpdateTime_ = plan.lichTime;
            lastLichClip_ = clipIndex;
            pendingRefit_ = CharacterBlasRefit::Lich;
        }
    }
    diagnostic.clear();
    return true;
}

VkAccelerationStructureInstanceKHR CharacterRenderSlot::BuildActiveInstance(
    const horde::gameplay::CombatSnapshot& combat,
    const horde::gameplay::EnemyRosterSnapshot& roster,
    const horde::gameplay::LichSnapshot& lich) const
{
    const CharacterRenderPlan plan = EvaluateCharacterRenderPlan(combat, roster, lich);
    VkAccelerationStructureInstanceKHR instance{};
    instance.transform = plan.transform;
    instance.instanceCustomIndex = kTlasInstanceIndex;
    instance.mask = 0x01u;
    instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instance.accelerationStructureReference = plan.selectedLich
        ? lichGpu_.accelerationStructure.address
        : skeletonGpu_.accelerationStructure.address;
    return instance;
}

std::array<float, 3u> CharacterRenderSlot::LichStaffWorldPosition(
    const horde::gameplay::LichSnapshot& lich) const
{
    return TransformPoint(LichInstanceTransform(lich), lichStaffLocalSample_);
}

void CharacterRenderSlot::DestroyGpuResources(const RtGpuResources& resources)
{
    resources.DestroyAccelerationStructure(skeletonGpu_.accelerationStructure);
    resources.DestroyBuffer(skeletonGpu_.updateScratch);
    resources.DestroyBuffer(skeletonGpu_.vertices);
    resources.DestroyAccelerationStructure(lichGpu_.accelerationStructure);
    resources.DestroyBuffer(lichGpu_.updateScratch);
    resources.DestroyBuffer(lichGpu_.vertices);
    skeletonGpu_ = {};
    lichGpu_ = {};
    lastSkeletonUpdateTime_ = -1.0f;
    lastSkeletonClip_ = -1;
    lastLichUpdateTime_ = -1.0f;
    lastLichClip_ = -1;
    pendingRefit_ = CharacterBlasRefit::None;
    lichStaffLocalSample_ = {{0.94f, 0.79f, 0.64f}};
}

} // namespace horde::vulkan::raytracing
