#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/SwordCombat.h"
#include "gameplay/simulation/SimulationSnapshot.h"
#include "scene/SkeletonBipedModel.h"
#include "vulkan/raytracing/RtGpuResources.h"

namespace horde::vulkan::raytracing
{

enum class CharacterBlasRefit
{
    None = 0u,
    SkeletonPose0 = 1u << 0u,
    SkeletonPose1 = 1u << 1u,
    Lich = 1u << 2u,
};

constexpr CharacterBlasRefit operator|(CharacterBlasRefit left, CharacterBlasRefit right)
{
    return static_cast<CharacterBlasRefit>(static_cast<unsigned>(left) | static_cast<unsigned>(right));
}

constexpr bool HasCharacterBlasRefit(CharacterBlasRefit value, CharacterBlasRefit flag)
{
    return (static_cast<unsigned>(value) & static_cast<unsigned>(flag)) != 0u;
}

struct SkeletonRenderPlan
{
    horde::scene::SkeletonClip clip = horde::scene::SkeletonClip::Idle;
    float time = 0.0f;
    VkTransformMatrixKHR transform{};
    std::uint32_t poseBucket = 0u;
};

struct CharacterFramePlan
{
    bool selectedLich = false;
    horde::scene::SkinnedClip lichClip = horde::scene::SkinnedClip::Idle;
    float lichTime = 0.0f;
    VkTransformMatrixKHR lichTransform{};
    std::array<SkeletonRenderPlan, horde::gameplay::simulation::kSkeletonEnemyCapacity> skeletons{};
    std::size_t skeletonCount = 0u;
    std::size_t skeletonPoseBucketCount = 0u;
};

bool CharacterPoseNeedsRefresh(int requestedClip,
                               float requestedTime,
                               int lastClip,
                               float lastTime,
                               float updateInterval = 1.0f / 30.0f);

CharacterFramePlan EvaluateCharacterFramePlan(
    const std::array<horde::gameplay::simulation::SkeletonEnemySnapshot,
                     horde::gameplay::simulation::kSkeletonEnemyCapacity>& skeletons,
    std::size_t skeletonCount,
    const horde::gameplay::EnemyRosterSnapshot& roster,
    const horde::gameplay::LichSnapshot& lich,
    float skeletonDeadClipDuration);

class CharacterRenderSlot
{
public:
    static constexpr std::uint32_t kTlasInstanceIndex = 2u;
    static constexpr std::uint32_t kSecondSkeletonTlasInstanceIndex = 18u;
    static constexpr std::uint32_t kMaximumActiveSkeletons = 2u;
    static constexpr std::uint32_t kMaximumSkeletonPoseBuckets = 2u;

    bool LoadAssets(const std::string& skeletonAssetPath,
                    const std::string& lichAssetPath,
                    std::string& diagnostic);
    bool PrepareInitialGeometry(std::string& diagnostic);
    bool CacheFramePlan(const std::array<horde::gameplay::simulation::SkeletonEnemySnapshot,
                                         horde::gameplay::simulation::kSkeletonEnemyCapacity>& skeletons,
                        std::size_t skeletonCount,
                        const horde::gameplay::EnemyRosterSnapshot& roster,
                        const horde::gameplay::LichSnapshot& lich,
                        std::string& diagnostic);
    bool PrepareFrame(const std::array<horde::gameplay::simulation::SkeletonEnemySnapshot,
                                       horde::gameplay::simulation::kSkeletonEnemyCapacity>& skeletons,
                      std::size_t skeletonCount,
                      const horde::gameplay::EnemyRosterSnapshot& roster,
                      const horde::gameplay::LichSnapshot& lich,
                      const RtGpuResources& resources,
                      std::string& diagnostic);

    std::array<VkAccelerationStructureInstanceKHR, kMaximumActiveSkeletons> BuildActiveInstances() const;
    std::array<float, 3u> LichStaffWorldPosition(const horde::gameplay::LichSnapshot& lich) const;

    CharacterBlasRefit PendingRefit() const { return pendingRefit_; }

    RtUpdatableTriangleBlas& SkeletonGpu(std::size_t bucket = 0u) { return skeletonGpus_.at(bucket); }
    const RtUpdatableTriangleBlas& SkeletonGpu(std::size_t bucket = 0u) const { return skeletonGpus_.at(bucket); }
    RtUpdatableTriangleBlas& LichGpu() { return lichGpu_; }
    const RtUpdatableTriangleBlas& LichGpu() const { return lichGpu_; }
    const std::vector<horde::scene::SkinnedRtVertex>& SkeletonVertices(std::size_t bucket = 0u) const
    {
        return skeletonSkinnedVertices_.at(bucket);
    }
    const std::vector<horde::scene::TexturedSkinnedRtVertex>& LichVertices() const { return lichSkinnedVertices_; }
    const std::array<float, 3u>& LichStaffLocalSample() const { return lichStaffLocalSample_; }
    float SkeletonDeadClipDuration() const { return skeletonDeadClipDuration_; }
    std::size_t SkeletonPoseBucketCount() const { return skeletonPoseBucketCount_; }

    void DestroyGpuResources(const RtGpuResources& resources);

private:
    bool UpdateLichStaffSample(std::string& diagnostic);

    horde::scene::SkeletonBipedModel skeletonModel_;
    horde::scene::SkinnedCharacterModel lichModel_;
    std::array<std::vector<horde::scene::SkinnedRtVertex>, kMaximumSkeletonPoseBuckets> skeletonSkinnedVertices_;
    std::vector<horde::scene::TexturedSkinnedRtVertex> lichSkinnedVertices_;
    std::array<RtUpdatableTriangleBlas, kMaximumSkeletonPoseBuckets> skeletonGpus_{};
    RtUpdatableTriangleBlas lichGpu_;
    std::array<float, 3u> lichStaffLocalSample_{{0.94f, 0.79f, 0.64f}};
    std::array<float, kMaximumSkeletonPoseBuckets> lastSkeletonUpdateTimes_{{-1.0f, -1.0f}};
    std::array<int, kMaximumSkeletonPoseBuckets> lastSkeletonClips_{{-1, -1}};
    float lastLichUpdateTime_ = -1.0f;
    int lastLichClip_ = -1;
    CharacterBlasRefit pendingRefit_ = CharacterBlasRefit::None;
    std::size_t skeletonPoseBucketCount_ = 0u;
    float skeletonDeadClipDuration_ = 0.0f;
    CharacterFramePlan cachedFramePlan_{};
};

} // namespace horde::vulkan::raytracing
