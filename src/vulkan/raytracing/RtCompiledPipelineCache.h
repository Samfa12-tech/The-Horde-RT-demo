#pragma once

#include "vulkan/RtExecutionBackend.h"
#include "vulkan/raytracing/RtPipelineVariants.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace horde::vulkan::raytracing
{
struct RtCachedArtifactIdentity
{
    std::string key;
    std::string spirvSha256;
    std::string includeSha256;
    std::uint64_t wordCount = 0u;
    bool operator==(const RtCachedArtifactIdentity&) const = default;
};

struct RtCachedSpecializationEntry
{
    std::uint32_t constantId = 0u;
    std::uint32_t offset = 0u;
    std::size_t size = 0u;
    constexpr bool operator==(const RtCachedSpecializationEntry&) const = default;
};

struct RtCachedShaderStage
{
    std::uint32_t stage = 0u;
    std::uint32_t flags = 0u;
    std::string moduleIdentity;
    std::string entryPoint;
    std::vector<RtCachedSpecializationEntry> specializationEntries;
    std::vector<std::byte> specializationData;
    std::string extensionStateIdentity;
    bool operator==(const RtCachedShaderStage&) const = default;
};

struct RtCachedShaderGroup
{
    std::uint32_t type = 0u;
    std::int32_t generalShader = -1;
    std::int32_t closestHitShader = -1;
    std::int32_t anyHitShader = -1;
    std::int32_t intersectionShader = -1;
    std::vector<std::byte> captureReplayHandle;
    std::string extensionStateIdentity;
    bool operator==(const RtCachedShaderGroup&) const = default;
};

struct RtCachedDescriptorBinding
{
    std::uint32_t binding = 0u;
    std::uint32_t descriptorType = 0u;
    std::uint32_t descriptorCount = 0u;
    std::uint32_t stageFlags = 0u;
    std::uint32_t bindingFlags = 0u;
    std::vector<std::uint64_t> immutableSamplerIdentities;
    bool operator==(const RtCachedDescriptorBinding&) const = default;
};

struct RtCachedPushConstantRange
{
    std::uint32_t stageFlags = 0u;
    std::uint32_t offset = 0u;
    std::uint32_t size = 0u;
    constexpr bool operator==(const RtCachedPushConstantRange&) const = default;
};

// The cache is scoped to one logical device. deviceIdentity is still explicit
// in the key so accidental cross-device lookup is rejected rather than relying
// on callers to remember that rule.
struct RtCompiledPipelineKey
{
    std::uint64_t deviceIdentity = 0u;
    RtExecutionBackend backend = RtExecutionBackend::Unsupported;
    RtInstrumentation instrumentation = RtInstrumentation::Shipping;
    DielectricQuality quality = DielectricQuality::Mobile;
    std::array<RtCachedArtifactIdentity, 2u> strategyArtifacts{};
    std::vector<RtCachedArtifactIdentity> sharedShaderModules;
    std::array<std::vector<RtCachedShaderStage>, 2u> strategyStages{};
    std::vector<RtCachedShaderGroup> shaderGroups;
    std::uint32_t pipelineCreateFlags = 0u;
    std::uint32_t maximumRecursionDepth = 0u;
    std::string basePipelineIdentity;
    std::int32_t basePipelineIndex = 0;
    std::string pipelineCreateExtensionStateIdentity;
    std::uint32_t descriptorSetLayoutCreateFlags = 0u;
    std::vector<RtCachedDescriptorBinding> descriptorBindings;
    std::uint32_t pipelineLayoutCreateFlags = 0u;
    std::vector<RtCachedPushConstantRange> pushConstantRanges;
    std::string pipelineLayoutExtensionStateIdentity;
    bool operator==(const RtCompiledPipelineKey&) const = default;
};

enum class RtPipelineCacheInsertResult
{
    Adopted,
    AlreadyPresent,
    BuildFailed,
    InvalidKey,
    CapacityFull,
    RetirementNotProven,
};

// Owner-thread only; callers keep this device-local cache alive longer than all
// leases and only retire/evict entries after their queue/device-idle proof.
// Pipeline and original layout objects stay together. The descriptor pool,
// descriptor set and SBT remain scene-owned and are rebuilt per scene.
template <typename Pipeline, typename PipelineLayout, typename DescriptorSetLayout>
struct RtCompiledPipelineObjects
{
    std::array<Pipeline, 2u> pipelines{};
    PipelineLayout pipelineLayout{};
    DescriptorSetLayout descriptorSetLayout{};
};

template <typename Pipeline, typename PipelineLayout, typename DescriptorSetLayout>
class RtCompiledPipelineCache
{
  public:
    using Objects = RtCompiledPipelineObjects<Pipeline, PipelineLayout, DescriptorSetLayout>;
    using DestroyObjects = void (*)(void*, Objects&) noexcept;
    static constexpr std::size_t kCapacity = 2u;

    class Lease
    {
      public:
        Lease() = default;
        Lease(const Lease&) = delete;
        Lease& operator=(const Lease&) = delete;
        Lease(Lease&& other) noexcept { MoveFrom(std::move(other)); }
        Lease& operator=(Lease&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                MoveFrom(std::move(other));
            }
            return *this;
        }
        ~Lease() { Reset(); }

        explicit operator bool() const noexcept { return owner_ != nullptr; }
        bool IsBorrowed() const noexcept { return owner_ != nullptr; }
        const Objects* Get() const noexcept
        {
            return owner_ == nullptr ? nullptr : owner_->GetLeasedObjects(slot_, generation_);
        }

      private:
        friend class RtCompiledPipelineCache;
        Lease(RtCompiledPipelineCache* owner, std::size_t slot, std::uint64_t generation) noexcept
            : owner_(owner), slot_(slot), generation_(generation) {}
        void Reset() noexcept
        {
            if (owner_ != nullptr) owner_->Release(slot_, generation_);
            owner_ = nullptr;
        }
        void MoveFrom(Lease&& other) noexcept
        {
            owner_ = std::exchange(other.owner_, nullptr);
            slot_ = other.slot_;
            generation_ = other.generation_;
        }
        RtCompiledPipelineCache* owner_ = nullptr;
        std::size_t slot_ = 0u;
        std::uint64_t generation_ = 0u;
    };

    RtCompiledPipelineCache(std::uint64_t deviceIdentity, void* destroyUser,
                            DestroyObjects destroyObjects) noexcept
        : deviceIdentity_(deviceIdentity), destroyUser_(destroyUser), destroyObjects_(destroyObjects)
    {
    }
    RtCompiledPipelineCache(const RtCompiledPipelineCache&) = delete;
    RtCompiledPipelineCache& operator=(const RtCompiledPipelineCache&) = delete;
    ~RtCompiledPipelineCache() { assert(Empty() && "destroy pipeline cache after owner-thread device idle"); }

    Lease Acquire(const RtCompiledPipelineKey& key) noexcept
    {
        const auto slot = Find(key);
        if (!slot) return {};
        auto& entry = entries_[*slot];
        ++entry.leaseCount;
        entry.lastUse = ++useClock_;
        return Lease(this, *slot, entry.generation);
    }

    RtPipelineCacheInsertResult Adopt(const RtCompiledPipelineKey& key,
                                      Objects& candidate,
                                      const bool creationSucceeded,
                                      const bool deviceIdleProven)
    {
        if (!creationSucceeded) return RtPipelineCacheInsertResult::BuildFailed;
        if (!ValidKey(key) || key.deviceIdentity != deviceIdentity_ || destroyObjects_ == nullptr)
            return RtPipelineCacheInsertResult::InvalidKey;
        if (Find(key)) return RtPipelineCacheInsertResult::AlreadyPresent;
        if (!CompleteObjects(candidate)) return RtPipelineCacheInsertResult::InvalidKey;

        std::size_t target = kCapacity;
        for (std::size_t index = 0u; index < entries_.size(); ++index)
        {
            if (!entries_[index].valid)
            {
                target = index;
                break;
            }
        }
        if (target == kCapacity)
        {
            target = kCapacity;
            for (std::size_t index = 0u; index < entries_.size(); ++index)
            {
                if (entries_[index].leaseCount == 0u &&
                    (target == kCapacity || entries_[index].lastUse < entries_[target].lastUse))
                    target = index;
            }
            if (target == kCapacity) return RtPipelineCacheInsertResult::CapacityFull;
            if (!deviceIdleProven) return RtPipelineCacheInsertResult::RetirementNotProven;
            DestroyEntry(entries_[target]);
        }

        Entry& entry = entries_[target];
        entry.key = key;
        entry.objects = std::exchange(candidate, Objects{});
        entry.leaseCount = 0u;
        entry.lastUse = ++useClock_;
        entry.generation = ++generationClock_;
        entry.valid = true;
        return RtPipelineCacheInsertResult::Adopted;
    }

    // Must be called on the device owner after a successful idle/retirement
    // proof. False means the caller must keep the device alive and retry later.
    bool DestroyAfterDeviceIdle(const bool deviceIdleProven) noexcept
    {
        if (!deviceIdleProven) return false;
        for (const auto& entry : entries_)
            if (entry.valid && entry.leaseCount != 0u) return false;
        for (auto& entry : entries_)
            if (entry.valid) DestroyEntry(entry);
        return true;
    }

    bool Empty() const noexcept
    {
        for (const auto& entry : entries_)
            if (entry.valid) return false;
        return true;
    }

    std::size_t Size() const noexcept
    {
        std::size_t size = 0u;
        for (const auto& entry : entries_) size += entry.valid ? 1u : 0u;
        return size;
    }

  private:
    struct Entry
    {
        RtCompiledPipelineKey key{};
        Objects objects{};
        std::uint64_t generation = 0u;
        std::uint64_t lastUse = 0u;
        std::size_t leaseCount = 0u;
        bool valid = false;
    };

    static bool CompleteObjects(const Objects& objects) noexcept
    {
        return objects.pipelines[0] != Pipeline{} && objects.pipelines[1] != Pipeline{} &&
               objects.pipelineLayout != PipelineLayout{} &&
               objects.descriptorSetLayout != DescriptorSetLayout{};
    }

    bool ValidKey(const RtCompiledPipelineKey& key) const noexcept
    {
        if (key.deviceIdentity == 0u || key.backend == RtExecutionBackend::Unsupported ||
            key.strategyArtifacts[0].key.empty() || key.strategyArtifacts[1].key.empty() ||
            key.strategyArtifacts[0].spirvSha256.empty() || key.strategyArtifacts[1].spirvSha256.empty() ||
            key.strategyStages[0].empty() || key.strategyStages[1].empty() || key.shaderGroups.empty())
            return false;
        for (const auto& binding : key.descriptorBindings)
            if (binding.descriptorCount == 0u) return false;
        for (const auto& range : key.pushConstantRanges)
            if (range.size == 0u) return false;
        return true;
    }

    std::optional<std::size_t> Find(const RtCompiledPipelineKey& key) const noexcept
    {
        if (key.deviceIdentity != deviceIdentity_) return std::nullopt;
        for (std::size_t index = 0u; index < entries_.size(); ++index)
            if (entries_[index].valid && entries_[index].key == key) return index;
        return std::nullopt;
    }

    const Objects* GetLeasedObjects(const std::size_t slot,
                                    const std::uint64_t generation) const noexcept
    {
        if (slot >= entries_.size()) return nullptr;
        const auto& entry = entries_[slot];
        return entry.valid && entry.generation == generation && entry.leaseCount != 0u
            ? &entry.objects : nullptr;
    }

    void Release(const std::size_t slot, const std::uint64_t generation) noexcept
    {
        if (slot >= entries_.size()) return;
        auto& entry = entries_[slot];
        if (entry.valid && entry.generation == generation && entry.leaseCount != 0u)
            --entry.leaseCount;
    }

    void DestroyEntry(Entry& entry) noexcept
    {
        if (entry.valid && destroyObjects_ != nullptr)
            destroyObjects_(destroyUser_, entry.objects);
        entry = {};
    }

    std::uint64_t deviceIdentity_ = 0u;
    void* destroyUser_ = nullptr;
    DestroyObjects destroyObjects_ = nullptr;
    std::array<Entry, kCapacity> entries_{};
    std::uint64_t useClock_ = 0u;
    std::uint64_t generationClock_ = 0u;
};
} // namespace horde::vulkan::raytracing
