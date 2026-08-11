#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/SwordCombat.h"
#include "scene/SkeletonBipedModel.h"
#include "vulkan/raytracing/RtGpuResources.h"

namespace horde::vulkan::raytracing
{

enum class CharacterBlasRefit
{
    None,
    Skeleton,
    Lich,
};

struct CharacterRenderPlan
{
    bool selectedLich = false;
    horde::scene::SkeletonClip skeletonClip = horde::scene::SkeletonClip::Idle;
    float skeletonTime = 0.0f;
    horde::scene::SkinnedClip lichClip = horde::scene::SkinnedClip::Idle;
    float lichTime = 0.0f;
    VkTransformMatrixKHR transform{};
};

CharacterRenderPlan EvaluateCharacterRenderPlan(
    const horde::gameplay::CombatSnapshot& combat,
    const horde::gameplay::EnemyRosterSnapshot& roster,
    const horde::gameplay::LichSnapshot& lich);

bool CharacterPoseNeedsRefresh(int requestedClip,
                               float requestedTime,
                               int lastClip,
                               float lastTime,
                               float updateInterval = 1.0f / 30.0f);

class CharacterRenderSlot
{
public:
    static constexpr std::uint32_t kTlasInstanceIndex = 2u;
    static constexpr std::uint32_t kMaximumActiveCharacters = 1u;

    bool LoadAssets(const std::string& skeletonAssetPath,
                    const std::string& lichAssetPath,
                    std::string& diagnostic);
    bool PrepareInitialGeometry(std::string& diagnostic);
    bool PrepareFrame(const horde::gameplay::CombatSnapshot& combat,
                      const horde::gameplay::EnemyRosterSnapshot& roster,
                      const horde::gameplay::LichSnapshot& lich,
                      const RtGpuResources& resources,
                      std::string& diagnostic);

    VkAccelerationStructureInstanceKHR BuildActiveInstance(
        const horde::gameplay::CombatSnapshot& combat,
        const horde::gameplay::EnemyRosterSnapshot& roster,
        const horde::gameplay::LichSnapshot& lich) const;
    std::array<float, 3u> LichStaffWorldPosition(const horde::gameplay::LichSnapshot& lich) const;

    CharacterBlasRefit PendingRefit() const { return pendingRefit_; }
    void ClearPendingRefit() { pendingRefit_ = CharacterBlasRefit::None; }

    RtUpdatableTriangleBlas& SkeletonGpu() { return skeletonGpu_; }
    const RtUpdatableTriangleBlas& SkeletonGpu() const { return skeletonGpu_; }
    RtUpdatableTriangleBlas& LichGpu() { return lichGpu_; }
    const RtUpdatableTriangleBlas& LichGpu() const { return lichGpu_; }
    const std::vector<horde::scene::SkinnedRtVertex>& SkeletonVertices() const { return skeletonSkinnedVertices_; }
    const std::vector<horde::scene::TexturedSkinnedRtVertex>& LichVertices() const { return lichSkinnedVertices_; }
    const std::array<float, 3u>& LichStaffLocalSample() const { return lichStaffLocalSample_; }

    void DestroyGpuResources(const RtGpuResources& resources);

private:
    bool UpdateLichStaffSample(std::string& diagnostic);

    horde::scene::SkeletonBipedModel skeletonModel_;
    horde::scene::SkinnedCharacterModel lichModel_;
    std::vector<horde::scene::SkinnedRtVertex> skeletonSkinnedVertices_;
    std::vector<horde::scene::TexturedSkinnedRtVertex> lichSkinnedVertices_;
    RtUpdatableTriangleBlas skeletonGpu_;
    RtUpdatableTriangleBlas lichGpu_;
    std::array<float, 3u> lichStaffLocalSample_{{0.94f, 0.79f, 0.64f}};
    float lastSkeletonUpdateTime_ = -1.0f;
    int lastSkeletonClip_ = -1;
    float lastLichUpdateTime_ = -1.0f;
    int lastLichClip_ = -1;
    CharacterBlasRefit pendingRefit_ = CharacterBlasRefit::None;
};

} // namespace horde::vulkan::raytracing
