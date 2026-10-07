#include "vulkan/raytracing/RtCompiledPipelineCache.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace horde::vulkan::raytracing;
using horde::vulkan::RtExecutionBackend;

namespace
{
using Cache = RtCompiledPipelineCache<std::uint32_t, std::uint32_t, std::uint32_t>;
using Objects = Cache::Objects;

struct DestroyLog
{
    std::vector<std::uint32_t> destroyed;
};

void DestroyFakeObjects(void* user, Objects& objects) noexcept
{
    auto& log = *static_cast<DestroyLog*>(user);
    // Mirror the Vulkan lifetime dependency: pipelines precede their creation
    // pipeline layout; the descriptor-set layout is retained with both.
    for (const auto pipeline : objects.pipelines)
        if (pipeline != 0u) log.destroyed.push_back(pipeline);
    if (objects.pipelineLayout != 0u) log.destroyed.push_back(objects.pipelineLayout);
    if (objects.descriptorSetLayout != 0u) log.destroyed.push_back(objects.descriptorSetLayout);
    objects = {};
}

RtCompiledPipelineKey MakeKey(const std::uint64_t device, const std::string& suffix)
{
    RtCompiledPipelineKey key{};
    key.deviceIdentity = device;
    key.backend = RtExecutionBackend::RayTracingPipeline;
    key.instrumentation = RtInstrumentation::Shipping;
    key.quality = DielectricQuality::Mobile;
    key.strategyArtifacts[0] = {"opaque-" + suffix, "spirv-opaque-" + suffix,
                                "includes-opaque-" + suffix, 41u};
    key.strategyArtifacts[1] = {"glass-" + suffix, "spirv-glass-" + suffix,
                                "includes-glass-" + suffix, 67u};
    key.sharedShaderModules.push_back({"shared-raygen", "shared-spirv", "shared-includes", 19u});
    for (std::size_t strategy = 0; strategy < key.strategyStages.size(); ++strategy)
    {
        key.strategyStages[strategy] = {
            {1u, 0u, "raygen-" + std::to_string(strategy) + "-" + suffix, "main", {}, {}, {}},
            {2u, 0u, "miss-shared", "main", {}, {}, {}},
            {4u, 0u, "hit-" + std::to_string(strategy) + "-" + suffix, "main", {}, {}, {}},
        };
    }
    key.strategyStages[1][0].specializationEntries.push_back({3u, 0u, sizeof(std::uint32_t)});
    key.strategyStages[1][0].specializationData = {std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
    key.shaderGroups = {
        {0u, 0, -1, -1, -1, {}, {}},
        {1u, -1, 2, -1, -1, {}, {}},
    };
    key.pipelineCreateFlags = 0x10u;
    key.maximumRecursionDepth = 2u;
    key.pipelineCreateExtensionStateIdentity = "pipeline-pnext-v1";
    key.descriptorSetLayoutCreateFlags = 0x20u;
    key.descriptorBindings = {{0u, 6u, 1u, 0x3u, 0x1u, {0x101u}}};
    key.pipelineLayoutCreateFlags = 0x40u;
    key.pushConstantRanges = {{0x7u, 0u, 128u}};
    key.pipelineLayoutExtensionStateIdentity = "layout-pnext-v1";
    return key;
}

Objects MakeObjects(const std::uint32_t base)
{
    return {{{base, base + 1u}}, base + 2u, base + 3u};
}
} // namespace

int main()
{
    bool ok = true;
    const auto require = [&](const bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ok = false; }
    };

    const auto base = MakeKey(1u, "a");
    const std::vector<void (*)(RtCompiledPipelineKey&)> keyMutations = {
        [](auto& k) { ++k.deviceIdentity; },
        [](auto& k) { k.backend = RtExecutionBackend::RayQueryCompute; },
        [](auto& k) { k.instrumentation = RtInstrumentation::Diagnostic; },
        [](auto& k) { k.quality = DielectricQuality::High; },
        [](auto& k) { k.strategyArtifacts[0].spirvSha256 += "-changed"; },
        [](auto& k) { k.strategyArtifacts[1].includeSha256 += "-changed"; },
        [](auto& k) { ++k.strategyArtifacts[0].wordCount; },
        [](auto& k) { k.sharedShaderModules[0].key += "-changed"; },
        [](auto& k) { k.strategyStages[0][0].moduleIdentity += "-changed"; },
        [](auto& k) { k.strategyStages[1][0].entryPoint = "alternate"; },
        [](auto& k) { ++k.strategyStages[1][0].flags; },
        [](auto& k) { ++k.strategyStages[1][0].specializationEntries[0].constantId; },
        [](auto& k) { k.strategyStages[1][0].specializationData[0] = std::byte{9}; },
        [](auto& k) { k.strategyStages[0][0].extensionStateIdentity += "-changed"; },
        [](auto& k) { ++k.shaderGroups[0].type; },
        [](auto& k) { ++k.shaderGroups[1].closestHitShader; },
        [](auto& k) { k.shaderGroups[0].captureReplayHandle.push_back(std::byte{7}); },
        [](auto& k) { k.shaderGroups[0].extensionStateIdentity += "-changed"; },
        [](auto& k) { ++k.pipelineCreateFlags; },
        [](auto& k) { ++k.maximumRecursionDepth; },
        [](auto& k) { k.basePipelineIdentity += "-changed"; },
        [](auto& k) { ++k.basePipelineIndex; },
        [](auto& k) { k.pipelineCreateExtensionStateIdentity += "-changed"; },
        [](auto& k) { ++k.descriptorSetLayoutCreateFlags; },
        [](auto& k) { ++k.descriptorBindings[0].binding; },
        [](auto& k) { ++k.descriptorBindings[0].descriptorType; },
        [](auto& k) { ++k.descriptorBindings[0].descriptorCount; },
        [](auto& k) { ++k.descriptorBindings[0].stageFlags; },
        [](auto& k) { ++k.descriptorBindings[0].bindingFlags; },
        [](auto& k) { ++k.descriptorBindings[0].immutableSamplerIdentities[0]; },
        [](auto& k) { ++k.pipelineLayoutCreateFlags; },
        [](auto& k) { ++k.pushConstantRanges[0].stageFlags; },
        [](auto& k) { ++k.pushConstantRanges[0].offset; },
        [](auto& k) { ++k.pushConstantRanges[0].size; },
        [](auto& k) { k.pipelineLayoutExtensionStateIdentity += "-changed"; },
    };
    for (const auto mutate : keyMutations)
    {
        auto changed = base;
        mutate(changed);
        require(!(changed == base), "every Vulkan creation identity dimension participates in exact key equality");
    }

    DestroyLog log{};
    Cache cache(1u, &log, &DestroyFakeObjects);
    auto invalidKey = base;
    invalidKey.deviceIdentity = 2u;
    auto untouched = MakeObjects(10u);
    require(cache.Adopt(invalidKey, untouched, true, false) == RtPipelineCacheInsertResult::InvalidKey &&
            untouched.pipelines[0] == 10u && cache.Empty(),
            "a different logical device cannot publish into this owner cache");
    auto partialObjects = MakeObjects(20u);
    partialObjects.pipelines[0] = 0u;
    require(cache.Adopt(base, partialObjects, true, false) == RtPipelineCacheInsertResult::InvalidKey &&
            partialObjects.pipelines[1] == 21u && cache.Empty(),
            "a partially constructed object bundle remains caller-owned and cannot be cached");

    auto a = MakeObjects(100u);
    require(cache.Adopt(base, a, true, false) == RtPipelineCacheInsertResult::Adopted &&
            a.pipelines[0] == 0u && cache.Size() == 1u,
            "successful creation transfers the complete object set into the cache");
    auto duplicate = MakeObjects(150u);
    require(cache.Adopt(base, duplicate, true, false) == RtPipelineCacheInsertResult::AlreadyPresent &&
            duplicate.pipelines[0] == 150u && cache.Size() == 1u,
            "an already-cached exact key leaves the duplicate build caller-owned");
    auto borrowed = cache.Acquire(base);
    require(borrowed && borrowed.IsBorrowed() && borrowed.Get() != nullptr &&
            borrowed.Get()->pipelines[0] == 100u && borrowed.Get()->pipelineLayout == 102u &&
            borrowed.Get()->descriptorSetLayout == 103u,
            "exact request key borrows both pipelines and their original layout objects");
    require(!cache.Acquire(invalidKey), "device identity mismatch cannot hit a cached entry");
    borrowed = {};

    auto failed = MakeObjects(200u);
    const auto failedCopy = failed;
    require(cache.Adopt(MakeKey(1u, "failed"), failed, false, false) == RtPipelineCacheInsertResult::BuildFailed &&
            failed.pipelines == failedCopy.pipelines && failed.pipelineLayout == failedCopy.pipelineLayout &&
            cache.Size() == 1u,
            "failed Vulkan creation is never published and leaves cleanup with the caller");

    auto bKey = MakeKey(1u, "b");
    auto b = MakeObjects(300u);
    require(cache.Adopt(bKey, b, true, false) == RtPipelineCacheInsertResult::Adopted,
            "second request pair fits the fixed two-entry capacity");
    auto leaseA = cache.Acquire(base);
    auto leaseB = cache.Acquire(bKey);
    auto cKey = MakeKey(1u, "c");
    auto c = MakeObjects(500u);
    require(cache.Adopt(cKey, c, true, true) == RtPipelineCacheInsertResult::CapacityFull &&
            c.pipelines[0] == 500u,
            "capacity exhaustion with all entries leased safely falls back to scene ownership");
    leaseB = {};
    require(cache.Adopt(cKey, c, true, false) == RtPipelineCacheInsertResult::RetirementNotProven &&
            c.pipelines[0] == 500u,
            "an idle slot is not enough to evict Vulkan objects without device-idle proof");
    leaseA = {};
    auto bAgain = cache.Acquire(bKey);
    require(cache.Adopt(cKey, c, true, true) == RtPipelineCacheInsertResult::Adopted &&
            cache.Size() == Cache::kCapacity && c.pipelines[0] == 0u,
            "device-idle proof permits least-recently-used eviction after all leases release");
    require(bAgain && bAgain.Get()->pipelines[0] == 300u,
            "recently reused pair remains cached while the colder pair is evicted");

    require(!cache.DestroyAfterDeviceIdle(false), "cache teardown refuses absent device-idle proof");
    require(!cache.DestroyAfterDeviceIdle(true), "cache teardown refuses live leases even after device idle");
    bAgain = {};
    require(cache.DestroyAfterDeviceIdle(true) && cache.Empty(),
            "explicit post-idle teardown destroys all cached objects");
    require(cache.DestroyAfterDeviceIdle(true), "repeated post-idle teardown is harmless");

    const std::vector<std::uint32_t> expectedDestruction = {
        100u, 101u, 102u, 103u, // LRU eviction of A.
        500u, 501u, 502u, 503u, // C occupies A's former slot and is destroyed there.
        300u, 301u, 302u, 303u, // Final destroy of B.
    };
    require(log.destroyed == expectedDestruction,
            "each cached Vulkan object is destroyed once and pipelines precede retained layouts");
    if (log.destroyed != expectedDestruction)
    {
        std::cerr << "destroyed:";
        for (const auto value : log.destroyed) std::cerr << ' ' << value;
        std::cerr << '\n';
    }
    return ok ? 0 : 1;
}
