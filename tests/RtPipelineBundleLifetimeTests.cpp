#include "vulkan/raytracing/RtPipelineBundle.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace horde::vulkan::raytracing;

bool Require(bool condition, std::string_view message)
{
    if (!condition) { std::cerr << message << '\n'; }
    return condition;
}

template <typename Handle>
Handle FakeHandle(std::uintptr_t value)
{
    return reinterpret_cast<Handle>(value);
}

struct Ledger {
    RtPipelineBundleBuildStep failure = RtPipelineBundleBuildStep::None;
    std::uintptr_t nextHandle = 0x100u;
    std::vector<std::string> created;
    std::vector<std::string> destroyed;
    horde::vulkan::RtExecutionBackend expectedBackend =
        horde::vulkan::RtExecutionBackend::RayTracingPipeline;
    std::size_t sbtDestroyCalls = 0u;
    std::size_t liveTemporaryModules = 0u;
    bool pipelineModuleContractHonored = true;
    bool injectedFailureReached = false;
    RtBundleCompiledPipelineCache* compiledCache = nullptr;
    bool allowCachePublish = true;
    std::size_t cacheBorrowCalls = 0u;
    std::size_t cachePublishCalls = 0u;
    std::vector<VkDescriptorSetLayout> descriptorLayoutsUsed;
    std::vector<VkPipeline> sbtPipelines;
    std::vector<std::string> cacheDestroyed;
    std::array<VkPipeline, 2u> createdPipelines{};
    VkPipelineLayout createdPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout createdDescriptorSetLayout = VK_NULL_HANDLE;
    bool publishSawBothSbtStages = false;

    bool Begin(RtPipelineBundleBuildStep step, std::string_view name)
    {
        created.emplace_back(name);
        if (failure == step) {
            injectedFailureReached = true;
            return false;
        }
        return true;
    }
};

RtCompiledPipelineKey MakeCompiledCacheKey(const RtPipelineBundlePreflight& preflight)
{
    RtCompiledPipelineKey key{};
    key.deviceIdentity = 1u;
    key.backend = preflight.request.executionBackend;
    key.instrumentation = preflight.request.instrumentation;
    key.quality = preflight.request.quality;
    for (std::size_t index = 0u; index < key.strategyArtifacts.size(); ++index) {
        const auto& artifact = preflight.strategies[index];
        key.strategyArtifacts[index] = {
            std::string(artifact.canonicalKey), std::string(artifact.spirvSha256),
            std::string(artifact.includeSha256), artifact.expectedWordCount};
        key.strategyStages[index] = {
            {1u, 0u, std::string(artifact.spirvSha256), "main", {}, {}, {}},
            {2u, 0u, "shared-miss", "main", {}, {}, {}},
            {4u, 0u, "shared-hit", "main", {}, {}, {}},
        };
    }
    key.sharedShaderModules = {
        {"shared-miss", "shared-miss-spirv", "shared-miss-includes", 16u},
        {"shared-hit", "shared-hit-spirv", "shared-hit-includes", 24u},
    };
    key.shaderGroups = {
        {0u, 0, -1, -1, -1, {}, {}},
        {0u, 1, -1, -1, -1, {}, {}},
        {1u, -1, 2, -1, -1, {}, {}},
    };
    for (std::size_t index = 0u; index < preflight.descriptorIo.bindingCount; ++index) {
        const auto& binding = preflight.descriptorIo.bindings[index];
        key.descriptorBindings.push_back({binding.binding,
            static_cast<std::uint32_t>(binding.kind), 1u, 0x7u, 0u, {}});
    }
    key.maximumRecursionDepth = preflight.request.executionBackend ==
            horde::vulkan::RtExecutionBackend::RayTracingPipeline ? 1u : 0u;
    key.pushConstantRanges = {{0x7u, 0u, 128u}};
    return key;
}

void DestroyCachedObjects(void* user, RtBundleCompiledPipelineObjects& objects) noexcept
{
    auto& ledger = *static_cast<Ledger*>(user);
    if (objects.pipelines[0] != VK_NULL_HANDLE) ledger.cacheDestroyed.emplace_back("opaque-pipeline");
    if (objects.pipelines[1] != VK_NULL_HANDLE) ledger.cacheDestroyed.emplace_back("generic-pipeline");
    if (objects.pipelineLayout != VK_NULL_HANDLE) ledger.cacheDestroyed.emplace_back("pipeline-layout");
    if (objects.descriptorSetLayout != VK_NULL_HANDLE) ledger.cacheDestroyed.emplace_back("descriptor-layout");
    objects = {};
}

bool BorrowCachedObjects(void* user, const RtPipelineBundlePreflight& preflight,
                         RtBundleCompiledPipelineLease& lease,
                         RtBundleCompiledPipelineObjects& objects)
{
    auto& ledger = *static_cast<Ledger*>(user);
    ++ledger.cacheBorrowCalls;
    if (ledger.compiledCache == nullptr) return false;
    auto acquired = ledger.compiledCache->Acquire(MakeCompiledCacheKey(preflight));
    if (!acquired) return false;
    objects = *acquired.Get();
    lease = std::move(acquired);
    return true;
}

bool PublishCachedObjects(void* user, const RtPipelineBundlePreflight& preflight,
                          RtBundleCompiledPipelineObjects& candidate,
                          RtBundleCompiledPipelineLease& lease)
{
    auto& ledger = *static_cast<Ledger*>(user);
    ++ledger.cachePublishCalls;
    ledger.publishSawBothSbtStages =
        std::find(ledger.created.begin(), ledger.created.end(), "opaque-sbt") != ledger.created.end() &&
        std::find(ledger.created.begin(), ledger.created.end(), "generic-sbt") != ledger.created.end();
    if (ledger.compiledCache == nullptr || !ledger.allowCachePublish) return false;
    if (ledger.compiledCache->Adopt(MakeCompiledCacheKey(preflight), candidate, true, false) !=
        RtPipelineCacheInsertResult::Adopted) {
        return false;
    }
    lease = ledger.compiledCache->Acquire(MakeCompiledCacheKey(preflight));
    return static_cast<bool>(lease);
}

const char* StrategyName(RtMaterialStrategy strategy)
{
    return strategy == RtMaterialStrategy::OpaqueFast ? "opaque" : "generic";
}

RtPipelineBundleDestroyApi MakeDestroyApi(Ledger& ledger)
{
    RtPipelineBundleDestroyApi api{};
    api.user = &ledger;
    api.destroyBuffer = [](void* user, RtGpuResources*, RtGpuBuffer& buffer,
                           RtPipelineOwnedBuffer kind) noexcept {
        auto& state = *static_cast<Ledger*>(user);
        if (kind == RtPipelineOwnedBuffer::OpaqueFastSbt ||
            kind == RtPipelineOwnedBuffer::GenericDielectricSbt) {
            ++state.sbtDestroyCalls;
        }
        if (buffer.buffer == VK_NULL_HANDLE && buffer.memory == VK_NULL_HANDLE) { return; }
        switch (kind) {
        case RtPipelineOwnedBuffer::OpaqueFastSbt: state.destroyed.emplace_back("opaque-sbt"); break;
        case RtPipelineOwnedBuffer::GenericDielectricSbt: state.destroyed.emplace_back("generic-sbt"); break;
        case RtPipelineOwnedBuffer::Diagnostics: state.destroyed.emplace_back("diagnostics-buffer"); break;
        }
        buffer = {};
    };
    api.destroyPipeline = [](void* user, VkPipeline& pipeline,
                             RtMaterialStrategy strategy) noexcept {
        if (pipeline == VK_NULL_HANDLE) { return; }
        static_cast<Ledger*>(user)->destroyed.emplace_back(
            std::string(StrategyName(strategy)) + "-pipeline");
        pipeline = VK_NULL_HANDLE;
    };
    api.destroyPipelineLayout = [](void* user, VkPipelineLayout& layout) noexcept {
        if (layout == VK_NULL_HANDLE) { return; }
        static_cast<Ledger*>(user)->destroyed.emplace_back("pipeline-layout");
        layout = VK_NULL_HANDLE;
    };
    api.destroyDescriptorPool = [](void* user, VkDescriptorPool& pool) noexcept {
        if (pool == VK_NULL_HANDLE) { return; }
        static_cast<Ledger*>(user)->destroyed.emplace_back("descriptor-pool");
        pool = VK_NULL_HANDLE;
    };
    api.destroyDescriptorSetLayout = [](void* user, VkDescriptorSetLayout& layout) noexcept {
        if (layout == VK_NULL_HANDLE) { return; }
        static_cast<Ledger*>(user)->destroyed.emplace_back("descriptor-layout");
        layout = VK_NULL_HANDLE;
    };
    return api;
}

RtPipelineBundleBuildApi MakeBuildApi(Ledger& ledger)
{
    RtPipelineBundleBuildApi api{};
    api.user = &ledger;
    api.createDescriptorSetLayout = [](void* user, const RtDescriptorIoContract&,
                                       VkDescriptorSetLayout& out, std::string&) {
        auto& state = *static_cast<Ledger*>(user);
        const bool succeeds = state.Begin(
            RtPipelineBundleBuildStep::DescriptorSetLayout, "descriptor-layout");
        out = FakeHandle<VkDescriptorSetLayout>(state.nextHandle++);
        state.createdDescriptorSetLayout = out;
        return succeeds;
    };
    api.createDescriptorPool = [](void* user, const RtDescriptorIoContract&,
                                  VkDescriptorPool& out, std::string&) {
        auto& state = *static_cast<Ledger*>(user);
        const bool succeeds = state.Begin(
            RtPipelineBundleBuildStep::DescriptorPool, "descriptor-pool");
        out = FakeHandle<VkDescriptorPool>(state.nextHandle++);
        return succeeds;
    };
    api.allocateDescriptorSet = [](void* user, VkDescriptorPool, VkDescriptorSetLayout layout,
                                   VkDescriptorSet& out, std::string&) {
        auto& state = *static_cast<Ledger*>(user);
        state.descriptorLayoutsUsed.push_back(layout);
        const bool succeeds = state.Begin(
            RtPipelineBundleBuildStep::DescriptorSet, "descriptor-set");
        out = FakeHandle<VkDescriptorSet>(state.nextHandle++);
        return succeeds;
    };
    api.createDiagnosticBuffer = [](void* user, RtGpuBuffer& out, std::string&) {
        auto& state = *static_cast<Ledger*>(user);
        const bool succeeds = state.Begin(
            RtPipelineBundleBuildStep::DiagnosticBuffer, "diagnostics-buffer");
        out.buffer = FakeHandle<VkBuffer>(state.nextHandle++);
        out.memory = FakeHandle<VkDeviceMemory>(state.nextHandle++);
        out.size = 176u;
        return succeeds;
    };
    api.writeDescriptors = [](void* user, RtPipelineBundle&, std::string&) {
        return static_cast<Ledger*>(user)->Begin(
            RtPipelineBundleBuildStep::DescriptorWrites, "descriptor-writes");
    };
    api.createPipelineLayout = [](void* user, VkDescriptorSetLayout,
                                  VkPipelineLayout& out, std::string&) {
        auto& state = *static_cast<Ledger*>(user);
        const bool succeeds = state.Begin(
            RtPipelineBundleBuildStep::PipelineLayout, "pipeline-layout");
        out = FakeHandle<VkPipelineLayout>(state.nextHandle++);
        state.createdPipelineLayout = out;
        return succeeds;
    };
    if (ledger.expectedBackend == horde::vulkan::RtExecutionBackend::RayTracingPipeline) {
        api.createSharedShaderModules = [](void* user, VkShaderModule& miss,
                                           VkShaderModule& hit, std::string&) {
            auto& state = *static_cast<Ledger*>(user);
            const bool succeeds = state.Begin(
                RtPipelineBundleBuildStep::SharedShaderModules, "shared-modules");
            miss = FakeHandle<VkShaderModule>(state.nextHandle++);
            hit = FakeHandle<VkShaderModule>(state.nextHandle++);
            state.liveTemporaryModules += 2u;
            return succeeds;
        };
    }
    api.createEntryShaderModule = [](void* user, const RtPipelineVariantArtifact& artifact,
                                     VkShaderModule& out, std::string&) {
        auto& state = *static_cast<Ledger*>(user);
        const auto step = artifact.key.material == RtMaterialStrategy::OpaqueFast
            ? RtPipelineBundleBuildStep::OpaqueFastShaderModule
            : RtPipelineBundleBuildStep::GenericDielectricShaderModule;
        const bool succeeds = state.Begin(
            step, std::string(StrategyName(artifact.key.material)) + "-module");
        out = FakeHandle<VkShaderModule>(state.nextHandle++);
        ++state.liveTemporaryModules;
        return succeeds;
    };
    api.createStrategyPipeline = [](void* user, RtMaterialStrategy strategy,
                                    VkShaderModule entry, VkShaderModule miss,
                                    VkShaderModule hit,
                                    VkPipelineLayout, VkPipeline& out, std::string&) {
        auto& state = *static_cast<Ledger*>(user);
        const bool compute = state.expectedBackend ==
            horde::vulkan::RtExecutionBackend::RayQueryCompute;
        state.pipelineModuleContractHonored =
            state.pipelineModuleContractHonored && entry != VK_NULL_HANDLE &&
            (compute
                 ? miss == VK_NULL_HANDLE && hit == VK_NULL_HANDLE
                 : miss != VK_NULL_HANDLE && hit != VK_NULL_HANDLE);
        const auto step = strategy == RtMaterialStrategy::OpaqueFast
            ? RtPipelineBundleBuildStep::OpaqueFastPipeline
            : RtPipelineBundleBuildStep::GenericDielectricPipeline;
        const bool succeeds = state.Begin(
            step, std::string(StrategyName(strategy)) + "-pipeline");
        out = FakeHandle<VkPipeline>(state.nextHandle++);
        state.createdPipelines[strategy == RtMaterialStrategy::GenericDielectric ? 1u : 0u] = out;
        return succeeds;
    };
    api.destroyShaderModule = [](void* user, VkShaderModule& module) noexcept {
        if (module == VK_NULL_HANDLE) { return; }
        auto& state = *static_cast<Ledger*>(user);
        state.destroyed.emplace_back("temporary-module");
        if (state.liveTemporaryModules != 0u) {
            --state.liveTemporaryModules;
        }
        module = VK_NULL_HANDLE;
    };
    if (ledger.expectedBackend == horde::vulkan::RtExecutionBackend::RayTracingPipeline) {
        api.createStrategySbt = [](void* user, RtMaterialStrategy strategy, VkPipeline pipeline,
                                   RtGpuBuffer& out,
                                   std::array<VkStridedDeviceAddressRegionKHR, 4u>& regions,
                                   std::string&) {
            auto& state = *static_cast<Ledger*>(user);
            const auto step = strategy == RtMaterialStrategy::OpaqueFast
                ? RtPipelineBundleBuildStep::OpaqueFastSbt
                : RtPipelineBundleBuildStep::GenericDielectricSbt;
            const bool succeeds = state.Begin(
                step, std::string(StrategyName(strategy)) + "-sbt");
            state.sbtPipelines.push_back(pipeline);
            out.buffer = FakeHandle<VkBuffer>(state.nextHandle++);
            out.memory = FakeHandle<VkDeviceMemory>(state.nextHandle++);
            out.address = state.nextHandle++;
            out.size = 192u;
            for (std::size_t index = 0u; index < 3u; ++index) {
                regions[index].deviceAddress = out.address + index * 64u;
                regions[index].stride = 32u;
                regions[index].size = 32u;
            }
            return succeeds;
        };
    }
    if (ledger.compiledCache != nullptr) {
        api.borrowCompiledObjects = &BorrowCachedObjects;
        api.publishCompiledObjects = &PublishCachedObjects;
    }
    return api;
}

RtPipelineBundlePreflight MakePreflight(
    const horde::vulkan::RtExecutionBackend executionBackend =
        horde::vulkan::RtExecutionBackend::RayTracingPipeline)
{
    RtPipelineBundlePreflight preflight{};
    std::string error;
    if (!ResolveCompiledRtPipelineBundlePreflight(
            preflight, error, executionBackend)) {
        throw std::runtime_error(error);
    }
    return preflight;
}

bool MatchesFullPairIdentity(
    std::string_view identity,
    const RtPipelineBundlePreflight& preflight)
{
    if (preflight.strategies[0].key.material != RtMaterialStrategy::OpaqueFast ||
        preflight.strategies[1].key.material != RtMaterialStrategy::GenericDielectric) {
        return false;
    }
    const auto separator = identity.find('|');
    if (separator == std::string_view::npos ||
        identity.find('|', separator + 1u) != std::string_view::npos) {
        return false;
    }
    const auto matchesRecord = [](
        std::string_view record,
        std::string_view label,
        const RtPipelineVariantArtifact& artifact) {
        if (!record.starts_with(label)) { return false; }
        const auto payload = record.substr(label.size());
        const auto keyHashSeparator = payload.find('@');
        return keyHashSeparator != std::string_view::npos &&
               payload.find('@', keyHashSeparator + 1u) == std::string_view::npos &&
               payload.substr(0u, keyHashSeparator) == artifact.canonicalKey &&
               payload.substr(keyHashSeparator + 1u) == artifact.spirvSha256;
    };
    return matchesRecord(identity.substr(0u, separator), "opaqueFast:",
                         preflight.strategies[0]) &&
           matchesRecord(identity.substr(separator + 1u), "genericDielectric:",
                         preflight.strategies[1]);
}

bool MatchesShortPairIdentity(
    std::string_view identity,
    const RtPipelineBundlePreflight& preflight)
{
    constexpr std::size_t kShortHashLength = 8u;
    return identity.size() == kShortHashLength * 2u + 1u &&
           identity[kShortHashLength] == '+' &&
           identity.find('+', kShortHashLength + 1u) == std::string_view::npos &&
           identity.substr(0u, kShortHashLength) ==
               preflight.strategies[0].spirvSha256.substr(0u, kShortHashLength) &&
           identity.substr(kShortHashLength + 1u) ==
               preflight.strategies[1].spirvSha256.substr(0u, kShortHashLength);
}

bool IsZero(const VkStridedDeviceAddressRegionKHR& region)
{
    return region.deviceAddress == 0u && region.stride == 0u && region.size == 0u;
}

bool UsesContractTeardownOrder(const std::vector<std::string>& destroyed)
{
    const std::array<std::string_view, 8u> order{
        "generic-sbt", "generic-pipeline", "opaque-sbt", "opaque-pipeline",
        "pipeline-layout", "descriptor-pool", "descriptor-layout",
        "diagnostics-buffer"};
    std::size_t previous = 0u;
    bool observed = false;
    for (const std::string& resource : destroyed) {
        const auto position = std::find(order.begin(), order.end(), resource);
        if (position == order.end()) { continue; }
        const std::size_t rank = static_cast<std::size_t>(position - order.begin());
        if (observed && rank <= previous) { return false; }
        previous = rank;
        observed = true;
    }
    return true;
}

bool RejectsForgedAdoption(RtPipelineBundlePreflight preflight)
{
    Ledger ledger{};
    RtPipelineBundle bundle;
    std::string error;
    return !bundle.AdoptPreflight(std::move(preflight), MakeDestroyApi(ledger), error) &&
           error == "Invalid selected RT pipeline bundle preflight." &&
           !bundle.HasSelection() && !bundle.HasLiveResources() &&
           ledger.created.empty() && ledger.destroyed.empty();
}

} // namespace

int main()
{
    bool ok = true;
    std::string error;

    const auto& provider = RtPipelineVariantProvider::Compiled();
    const bool diagnosticPolicy =
        provider.request().instrumentation == RtInstrumentation::Diagnostic;
    auto forgedDescriptor = MakePreflight();
    ++forgedDescriptor.descriptorIo.descriptorWriteCount;
    ok &= Require(RejectsForgedAdoption(std::move(forgedDescriptor)),
                  "adoption must reject a forged descriptor/IO plan before ownership");
    auto forgedBindingRoster = MakePreflight();
    // Diagnostic already has binding 22 here; assigning 22 was a no-op.
    // Increment the actual roster value so both compiled policies are forged.
    ++forgedBindingRoster.descriptorIo.bindings[22u].binding;
    ok &= Require(RejectsForgedAdoption(std::move(forgedBindingRoster)),
                  "adoption must reject a forged non-contiguous descriptor binding roster");
    auto forgedPath = MakePreflight();
    forgedPath.strategies[0].artifactPath = "forged-selected-module.inc";
    ok &= Require(RejectsForgedAdoption(std::move(forgedPath)),
                  "adoption must reject forged selected-record catalog metadata");
    auto forgedHash = MakePreflight();
    forgedHash.strategies[1].spirvSha256 =
        "0000000000000000000000000000000000000000000000000000000000000000";
    ok &= Require(RejectsForgedAdoption(std::move(forgedHash)),
                  "adoption must reject forged selected-module hashes");
    auto forgedWords = MakePreflight();
    forgedWords.strategies[0].words = forgedWords.strategies[1].words;
    ok &= Require(RejectsForgedAdoption(std::move(forgedWords)),
                  "adoption must reject a non-authoritative selected module payload");

    std::vector<RtPipelineBundleBuildStep> constructionFaults{
        RtPipelineBundleBuildStep::DescriptorSetLayout,
        RtPipelineBundleBuildStep::DescriptorPool,
        RtPipelineBundleBuildStep::DescriptorSet,
        RtPipelineBundleBuildStep::DescriptorWrites,
        RtPipelineBundleBuildStep::PipelineLayout,
        RtPipelineBundleBuildStep::SharedShaderModules,
        RtPipelineBundleBuildStep::OpaqueFastShaderModule,
        RtPipelineBundleBuildStep::OpaqueFastPipeline,
        RtPipelineBundleBuildStep::GenericDielectricShaderModule,
        RtPipelineBundleBuildStep::GenericDielectricPipeline,
        RtPipelineBundleBuildStep::OpaqueFastSbt,
        RtPipelineBundleBuildStep::GenericDielectricSbt,
    };
    if (diagnosticPolicy) {
        constructionFaults.insert(constructionFaults.begin() + 3,
                                  RtPipelineBundleBuildStep::DiagnosticBuffer);
    }
    for (const auto failure : constructionFaults) {
        Ledger ledger{failure};
        RtPipelineBundle bundle;
        ok &= Require(bundle.AdoptPreflight(MakePreflight(),
                                            MakeDestroyApi(ledger), error),
                      "compiled-policy preflight adoption failed");
        ok &= Require(!BuildRtPipelineBundleResources(bundle, MakeBuildApi(ledger), error),
                      "every injected compiled-policy construction fault must fail");
        ok &= Require(!bundle.HasSelection() && bundle.DiagnosticAvailability() ==
                          RtDiagnosticAvailability::Unavailable &&
                          ledger.liveTemporaryModules == 0u,
                      "partial construction failure must leave no selected/live bundle");
        ok &= Require(ledger.injectedFailureReached,
                      "compiled-policy failure injection must reach its requested build step");
        ok &= Require(UsesContractTeardownOrder(ledger.destroyed),
                      "every partial Shipping failure must use contract teardown order");
        if (!diagnosticPolicy) {
            ok &= Require(std::find(ledger.created.begin(), ledger.created.end(),
                                    "diagnostics-buffer") == ledger.created.end() &&
                              std::find(ledger.destroyed.begin(), ledger.destroyed.end(),
                                    "diagnostics-buffer") == ledger.destroyed.end(),
                          "Shipping must never call or own the diagnostic-buffer seam");
        }
        const std::size_t destroyed = ledger.destroyed.size();
        bundle.Reset();
        ok &= Require(ledger.destroyed.size() == destroyed,
                      "failure cleanup and explicit reset must be idempotent");
    }

    Ledger ledger{};
    RtPipelineBundle bundle;
    const auto selectedPreflight = MakePreflight();
    ok &= Require(bundle.AdoptPreflight(selectedPreflight,
                                        MakeDestroyApi(ledger), error) &&
                      BuildRtPipelineBundleResources(bundle, MakeBuildApi(ledger), error),
                  "complete compiled-policy bundle construction must succeed");
    ok &= Require(bundle.DiagnosticAvailability() ==
                          (diagnosticPolicy ? RtDiagnosticAvailability::Available
                                            : RtDiagnosticAvailability::CompiledOut) &&
                      bundle.Strategy(RtMaterialStrategy::OpaqueFast).pipeline != VK_NULL_HANDLE &&
                      bundle.Strategy(RtMaterialStrategy::GenericDielectric).pipeline != VK_NULL_HANDLE &&
                      ledger.liveTemporaryModules == 0u,
                  "the complete bundle must own both material strategy records");
    ok &= Require(MatchesFullPairIdentity(bundle.FullPairIdentity(), selectedPreflight),
                  "the full selected-pair identity must preserve exact labels, order, keys, and hashes");
    ok &= Require(MatchesShortPairIdentity(bundle.ShortPairIdentity(), selectedPreflight),
                  "the display identity must preserve exact format and both selected hash prefixes");
    RtPipelineBundle moved(std::move(bundle));
    ok &= Require(!bundle.HasSelection() && moved.HasSelection(),
                  "move construction must transfer ownership and clear the source");
    moved.Reset();
    std::vector<std::string> expectedTail{
        "generic-sbt", "generic-pipeline", "opaque-sbt", "opaque-pipeline",
        "pipeline-layout", "descriptor-pool", "descriptor-layout"};
    if (diagnosticPolicy) expectedTail.emplace_back("diagnostics-buffer");
    ok &= Require(ledger.destroyed.size() >= expectedTail.size() &&
                      std::equal(expectedTail.begin(), expectedTail.end(),
                                 ledger.destroyed.end() - expectedTail.size()),
                  "reset must destroy the owned graph in exact inverse order");
    for (const auto strategy : {RtMaterialStrategy::OpaqueFast,
                                RtMaterialStrategy::GenericDielectric}) {
        const auto& record = moved.Strategy(strategy);
        ok &= Require(std::all_of(record.sbtRegions.begin(), record.sbtRegions.end(), IsZero),
                      "reset must zero all four SBT regions");
    }
    const std::size_t afterFirstReset = ledger.destroyed.size();
    moved.Reset();
    ok &= Require(ledger.destroyed.size() == afterFirstReset,
                  "normal reset must be idempotent");

    Ledger assignmentLedger{};
    RtPipelineBundle destination;
    RtPipelineBundle source;
    ok &= Require(destination.AdoptPreflight(MakePreflight(),
                                             MakeDestroyApi(assignmentLedger), error) &&
                      BuildRtPipelineBundleResources(destination, MakeBuildApi(assignmentLedger), error) &&
                      source.AdoptPreflight(MakePreflight(),
                                            MakeDestroyApi(assignmentLedger), error) &&
                      BuildRtPipelineBundleResources(source, MakeBuildApi(assignmentLedger), error),
                  "move-assignment fixtures must construct");
    destination = std::move(source);
    ok &= Require(destination.HasSelection() && !source.HasSelection() &&
                      destination.DiagnosticAvailability() ==
                          (diagnosticPolicy ? RtDiagnosticAvailability::Available
                                            : RtDiagnosticAvailability::CompiledOut),
                  "move assignment must reset the destination then transfer one live bundle");
    destination.Reset();

    Ledger movedFromOwner{};
    Ledger movedToOwner{};
    RtPipelineBundle ownerBoundSource;
    ok &= Require(ownerBoundSource.AdoptPreflight(
                      MakePreflight(),
                      MakeDestroyApi(movedFromOwner), error) &&
                      BuildRtPipelineBundleResources(ownerBoundSource,
                                                     MakeBuildApi(movedFromOwner), error),
                  "owner-bound move fixture must construct");
    RtPipelineBundle ownerBoundDestination(std::move(ownerBoundSource));
    ownerBoundDestination.RebindDestroyContext(&movedToOwner, nullptr);
    ownerBoundDestination.Reset();
    ok &= Require(movedFromOwner.destroyed.size() == 4u &&
                      !movedToOwner.destroyed.empty(),
                  "scene-style move must rebind the enclosing owner before later Reset");

    {
        Ledger cacheOwner{};
        RtBundleCompiledPipelineCache cache(1u, &cacheOwner, &DestroyCachedObjects);
        cacheOwner.compiledCache = &cache;
        const auto cachedPreflight = MakePreflight();
        auto cachedObjects = RtBundleCompiledPipelineObjects{
            {FakeHandle<VkPipeline>(0x901u), FakeHandle<VkPipeline>(0x902u)},
            FakeHandle<VkPipelineLayout>(0x903u),
            FakeHandle<VkDescriptorSetLayout>(0x904u)};
        ok &= Require(cache.Adopt(MakeCompiledCacheKey(cachedPreflight), cachedObjects,
                                  true, false) == RtPipelineCacheInsertResult::Adopted,
                      "cache-hit bundle fixture must seed one complete cached pair");

        Ledger sceneOwner{};
        sceneOwner.compiledCache = &cache;
        RtPipelineBundle cachedBundle;
        ok &= Require(cachedBundle.AdoptPreflight(cachedPreflight,
                                                   MakeDestroyApi(sceneOwner), error) &&
                          BuildRtPipelineBundleResources(cachedBundle,
                                                         MakeBuildApi(sceneOwner), error),
                      "cache hit must build scene-owned descriptors and fresh SBTs");
        const auto hasCreated = [&](std::string_view name) {
            return std::find(sceneOwner.created.begin(), sceneOwner.created.end(), name) !=
                   sceneOwner.created.end();
        };
        ok &= Require(cachedBundle.UsesCachedPipelines() && cachedBundle.ReusedCompiledPipelines() && cache.Size() == 1u &&
                          cachedBundle.Strategy(RtMaterialStrategy::OpaqueFast).pipeline ==
                              FakeHandle<VkPipeline>(0x901u) &&
                          cachedBundle.Strategy(RtMaterialStrategy::GenericDielectric).pipeline ==
                              FakeHandle<VkPipeline>(0x902u) &&
                          cachedBundle.pipelineLayout == FakeHandle<VkPipelineLayout>(0x903u) &&
                          cachedBundle.descriptorSetLayout == FakeHandle<VkDescriptorSetLayout>(0x904u),
                      "hit must attach the borrowed pair and its original layouts to the scene bundle");
        ok &= Require(!hasCreated("descriptor-layout") && !hasCreated("pipeline-layout") &&
                          !hasCreated("shared-modules") &&
                          std::none_of(sceneOwner.created.begin(), sceneOwner.created.end(),
                                       [](const std::string& name) {
                                           return name.ends_with("-pipeline") || name.ends_with("-module");
                                       }),
                      "hit must skip only compiled layout/module/pipeline creation");
        ok &= Require(sceneOwner.descriptorLayoutsUsed.size() == 1u &&
                          sceneOwner.descriptorLayoutsUsed[0] ==
                              FakeHandle<VkDescriptorSetLayout>(0x904u) &&
                          sceneOwner.sbtPipelines == std::vector<VkPipeline>{
                              FakeHandle<VkPipeline>(0x901u), FakeHandle<VkPipeline>(0x902u)},
                      "hit must allocate a fresh descriptor set against cached layout and rebuild SBTs from cached pipelines");
        ok &= Require(sceneOwner.cacheBorrowCalls == 1u && sceneOwner.cachePublishCalls == 0u,
                      "hit performs one lookup and never republishes its borrowed objects");

        RtPipelineBundle movedCachedBundle(std::move(cachedBundle));
        ok &= Require(!cachedBundle.HasLiveResources() && !cachedBundle.ReusedCompiledPipelines() &&
                          movedCachedBundle.UsesCachedPipelines() && movedCachedBundle.ReusedCompiledPipelines(),
                      "move transfers the typed cache lease without leaving source ownership");
        Ledger reboundSceneOwner{};
        movedCachedBundle.RebindDestroyContext(&reboundSceneOwner, nullptr);
        movedCachedBundle.Reset();
        ok &= Require(!reboundSceneOwner.destroyed.empty() &&
                          std::find(reboundSceneOwner.destroyed.begin(), reboundSceneOwner.destroyed.end(),
                                    "opaque-pipeline") == reboundSceneOwner.destroyed.end() &&
                          std::find(reboundSceneOwner.destroyed.begin(), reboundSceneOwner.destroyed.end(),
                                    "generic-pipeline") == reboundSceneOwner.destroyed.end() &&
                          std::find(reboundSceneOwner.destroyed.begin(), reboundSceneOwner.destroyed.end(),
                                    "pipeline-layout") == reboundSceneOwner.destroyed.end() &&
                          std::find(reboundSceneOwner.destroyed.begin(), reboundSceneOwner.destroyed.end(),
                                    "descriptor-layout") == reboundSceneOwner.destroyed.end() &&
                          cacheOwner.cacheDestroyed.empty(),
                      "moved cached bundle reset destroys only scene resources and releases its lease");
        ok &= Require(cache.DestroyAfterDeviceIdle(true) &&
                          cacheOwner.cacheDestroyed == std::vector<std::string>{
                              "opaque-pipeline", "generic-pipeline", "pipeline-layout", "descriptor-layout"},
                      "cache-owned objects remain alive until owner device-idle teardown");
    }

    {
        Ledger cacheOwner{};
        RtBundleCompiledPipelineCache cache(1u, &cacheOwner, &DestroyCachedObjects);
        cacheOwner.compiledCache = &cache;
        const auto preflight = MakePreflight();
        auto cachedObjects = RtBundleCompiledPipelineObjects{
            {FakeHandle<VkPipeline>(0x911u), FakeHandle<VkPipeline>(0x912u)},
            FakeHandle<VkPipelineLayout>(0x913u),
            FakeHandle<VkDescriptorSetLayout>(0x914u)};
        cache.Adopt(MakeCompiledCacheKey(preflight), cachedObjects, true, false);
        Ledger sceneOwner{RtPipelineBundleBuildStep::GenericDielectricSbt};
        sceneOwner.compiledCache = &cache;
        RtPipelineBundle failedCachedBundle;
        ok &= Require(failedCachedBundle.AdoptPreflight(preflight,
                                                         MakeDestroyApi(sceneOwner), error) &&
                          !BuildRtPipelineBundleResources(failedCachedBundle,
                                                          MakeBuildApi(sceneOwner), error),
                      "SBT failure after borrow must fail scene build");
        ok &= Require(!failedCachedBundle.UsesCachedPipelines() && cache.Size() == 1u &&
                          std::find(sceneOwner.destroyed.begin(), sceneOwner.destroyed.end(),
                                    "opaque-pipeline") == sceneOwner.destroyed.end() &&
                          std::find(sceneOwner.destroyed.begin(), sceneOwner.destroyed.end(),
                                    "generic-pipeline") == sceneOwner.destroyed.end() &&
                          std::find(sceneOwner.destroyed.begin(), sceneOwner.destroyed.end(),
                                    "pipeline-layout") == sceneOwner.destroyed.end() &&
                          std::find(sceneOwner.destroyed.begin(), sceneOwner.destroyed.end(),
                                    "descriptor-layout") == sceneOwner.destroyed.end() &&
                          cacheOwner.cacheDestroyed.empty(),
                      "failed SBT releases the lease without destroying cached pipeline objects");
        ok &= Require(cache.DestroyAfterDeviceIdle(true) && cacheOwner.cacheDestroyed.size() == 4u,
                      "failed scene build leaves cached objects for later owner teardown");
    }

    {
        Ledger ledger{};
        RtBundleCompiledPipelineCache cache(1u, &ledger, &DestroyCachedObjects);
        ledger.compiledCache = &cache;
        RtPipelineBundle publishedBundle;
        const auto preflight = MakePreflight();
        ok &= Require(publishedBundle.AdoptPreflight(preflight, MakeDestroyApi(ledger), error) &&
                          BuildRtPipelineBundleResources(publishedBundle, MakeBuildApi(ledger), error),
                      "cache miss must build through the legacy path then publish after SBT creation");
        ok &= Require(ledger.cacheBorrowCalls == 1u && ledger.cachePublishCalls == 1u &&
                          ledger.publishSawBothSbtStages && cache.Size() == 1u &&
                          publishedBundle.UsesCachedPipelines() && !publishedBundle.ReusedCompiledPipelines(),
                      "cache miss publishes only after both SBTs succeed and stores a lease");
        ok &= Require(ledger.sbtPipelines == std::vector<VkPipeline>{
                          ledger.createdPipelines[0], ledger.createdPipelines[1]},
                      "published miss constructs each SBT from its newly created strategy pipeline");
        auto published = cache.Acquire(MakeCompiledCacheKey(preflight));
        ok &= Require(published && published.Get()->pipelines[0] ==
                          publishedBundle.Strategy(RtMaterialStrategy::OpaqueFast).pipeline &&
                          published.Get()->pipelines[1] ==
                          publishedBundle.Strategy(RtMaterialStrategy::GenericDielectric).pipeline &&
                          published.Get()->pipelineLayout == publishedBundle.pipelineLayout &&
                          published.Get()->descriptorSetLayout == publishedBundle.descriptorSetLayout,
                      "bundle handles stay usable while cache owns the same compiled object set");
        published = {};
        publishedBundle.Reset();
        ok &= Require(std::find(ledger.destroyed.begin(), ledger.destroyed.end(),
                                "opaque-pipeline") == ledger.destroyed.end() &&
                          std::find(ledger.destroyed.begin(), ledger.destroyed.end(),
                                    "generic-pipeline") == ledger.destroyed.end() &&
                          std::find(ledger.destroyed.begin(), ledger.destroyed.end(),
                                    "pipeline-layout") == ledger.destroyed.end() &&
                          std::find(ledger.destroyed.begin(), ledger.destroyed.end(),
                                    "descriptor-layout") == ledger.destroyed.end() &&
                          ledger.cacheDestroyed.empty(),
                      "reset after publication destroys scene resources but leaves compiled objects cached");
        ok &= Require(cache.DestroyAfterDeviceIdle(true) && ledger.cacheDestroyed.size() == 4u,
                      "published compiled objects are destroyed once by cache owner after idle");
    }

    {
        Ledger ledger{};
        RtBundleCompiledPipelineCache cache(1u, &ledger, &DestroyCachedObjects);
        ledger.compiledCache = &cache;
        ledger.allowCachePublish = false;
        RtPipelineBundle fallbackBundle;
        const auto preflight = MakePreflight();
        ok &= Require(fallbackBundle.AdoptPreflight(preflight, MakeDestroyApi(ledger), error) &&
                          BuildRtPipelineBundleResources(fallbackBundle, MakeBuildApi(ledger), error),
                      "cache publication refusal must preserve a successful normal scene build");
        ok &= Require(!fallbackBundle.UsesCachedPipelines() && cache.Empty(),
                      "failed cache publication keeps ownership on the completed scene bundle");
        fallbackBundle.Reset();
        ok &= Require(std::find(ledger.destroyed.begin(), ledger.destroyed.end(),
                                "opaque-pipeline") != ledger.destroyed.end() &&
                          std::find(ledger.destroyed.begin(), ledger.destroyed.end(),
                                    "generic-pipeline") != ledger.destroyed.end() &&
                          std::find(ledger.destroyed.begin(), ledger.destroyed.end(),
                                    "pipeline-layout") != ledger.destroyed.end() &&
                          std::find(ledger.destroyed.begin(), ledger.destroyed.end(),
                                    "descriptor-layout") != ledger.destroyed.end() &&
                          ledger.cacheDestroyed.empty(),
                      "scene fallback retains and destroys its own pipelines and layouts");
    }

    constexpr auto computeBackend = horde::vulkan::RtExecutionBackend::RayQueryCompute;
    auto computePreflight = MakePreflight(computeBackend);
    auto forgedComputeRequest = computePreflight;
    forgedComputeRequest.request.executionBackend =
        horde::vulkan::RtExecutionBackend::RayTracingPipeline;
    ok &= Require(RejectsForgedAdoption(std::move(forgedComputeRequest)),
                  "adoption must reject a compute artifact pair relabelled as RTP");

    std::vector<RtPipelineBundleBuildStep> computeConstructionFaults{
        RtPipelineBundleBuildStep::DescriptorSetLayout,
        RtPipelineBundleBuildStep::DescriptorPool,
        RtPipelineBundleBuildStep::DescriptorSet,
        RtPipelineBundleBuildStep::DescriptorWrites,
        RtPipelineBundleBuildStep::PipelineLayout,
        RtPipelineBundleBuildStep::OpaqueFastShaderModule,
        RtPipelineBundleBuildStep::OpaqueFastPipeline,
        RtPipelineBundleBuildStep::GenericDielectricShaderModule,
        RtPipelineBundleBuildStep::GenericDielectricPipeline,
    };
    if (diagnosticPolicy) {
        computeConstructionFaults.insert(computeConstructionFaults.begin() + 3,
                                         RtPipelineBundleBuildStep::DiagnosticBuffer);
    }
    for (const auto failure : computeConstructionFaults) {
        Ledger computeFailureLedger{failure};
        computeFailureLedger.expectedBackend = computeBackend;
        RtPipelineBundle computeFailureBundle;
        ok &= Require(computeFailureBundle.AdoptPreflight(
                          MakePreflight(computeBackend),
                          MakeDestroyApi(computeFailureLedger), error),
                      "compute preflight adoption must use compute authority");
        const auto computeFailureApi = MakeBuildApi(computeFailureLedger);
        ok &= Require(computeFailureApi.Complete(computeBackend) &&
                          !computeFailureApi.Complete(
                              horde::vulkan::RtExecutionBackend::RayTracingPipeline),
                      "compute build API must not require shared-module or SBT callbacks");
        ok &= Require(!BuildRtPipelineBundleResources(
                          computeFailureBundle, computeFailureApi, error),
                      "every injected compute construction fault must fail");
        ok &= Require(computeFailureLedger.injectedFailureReached &&
                          !computeFailureBundle.HasSelection() &&
                          !computeFailureBundle.HasLiveResources() &&
                          computeFailureLedger.liveTemporaryModules == 0u,
                      "compute partial failure must reach its fault and release every live handle");
        ok &= Require(computeFailureLedger.pipelineModuleContractHonored,
                      "compute pipeline creation must receive one entry module and null miss/hit modules");
        ok &= Require(std::find(computeFailureLedger.created.begin(),
                                computeFailureLedger.created.end(),
                                "shared-modules") == computeFailureLedger.created.end() &&
                          std::none_of(computeFailureLedger.created.begin(),
                                       computeFailureLedger.created.end(),
                                       [](const std::string& name) {
                                           return name.ends_with("-sbt");
                                       }) &&
                          computeFailureLedger.sbtDestroyCalls == 0u,
                      "compute partial failure must perform zero shared-module or SBT operations");
        const std::size_t destroyed = computeFailureLedger.destroyed.size();
        computeFailureBundle.Reset();
        ok &= Require(computeFailureLedger.destroyed.size() == destroyed,
                      "compute partial-failure cleanup must be idempotent");
    }

    Ledger computeLedger{};
    computeLedger.expectedBackend = computeBackend;
    RtPipelineBundle computeBundle;
    const auto computeApi = MakeBuildApi(computeLedger);
    ok &= Require(computeBundle.AdoptPreflight(
                      computePreflight, MakeDestroyApi(computeLedger), error) &&
                      BuildRtPipelineBundleResources(computeBundle, computeApi, error),
                  "complete compute bundle construction must succeed without shared modules or SBTs");
    ok &= Require(computeBundle.Request().executionBackend == computeBackend &&
                      computeBundle.DiagnosticAvailability() ==
                          (diagnosticPolicy ? RtDiagnosticAvailability::Available
                                            : RtDiagnosticAvailability::CompiledOut) &&
                      computeBundle.pipelineLayout != VK_NULL_HANDLE &&
                      computeBundle.descriptorSet != VK_NULL_HANDLE &&
                      computeBundle.Strategy(RtMaterialStrategy::OpaqueFast).pipeline !=
                          VK_NULL_HANDLE &&
                      computeBundle.Strategy(RtMaterialStrategy::GenericDielectric).pipeline !=
                          VK_NULL_HANDLE &&
                      computeLedger.liveTemporaryModules == 0u &&
                      computeLedger.pipelineModuleContractHonored &&
                      std::find(computeLedger.created.begin(), computeLedger.created.end(),
                                "shared-modules") == computeLedger.created.end() &&
                      std::none_of(computeLedger.created.begin(), computeLedger.created.end(),
                                   [](const std::string& name) {
                                       return name.ends_with("-sbt");
                                   }),
                  "compute bundle must own two strategy pipelines and no shared-module/SBT work");
    horde::telemetry::RtResourceInventory computeInventory{};
    computeBundle.AccumulateResourceInventory(computeInventory);
    ok &= Require(computeInventory.pipelineCount == 2u &&
                      computeInventory.shaderBindingTableCount == 0u &&
                      computeInventory.descriptorSetCount == 1u,
                  "compute inventory must count two pipelines, one descriptor set, and zero SBTs");
    for (const auto strategy : {RtMaterialStrategy::OpaqueFast,
                                RtMaterialStrategy::GenericDielectric}) {
        const auto& resources = computeBundle.Strategy(strategy);
        ok &= Require(resources.shaderBindingTable.buffer == VK_NULL_HANDLE &&
                          resources.shaderBindingTable.memory == VK_NULL_HANDLE &&
                          std::all_of(resources.sbtRegions.begin(), resources.sbtRegions.end(),
                                      IsZero),
                      "compute strategy resources must contain no SBT handles or regions");
    }

    RtPipelineBundle movedCompute(std::move(computeBundle));
    ok &= Require(!computeBundle.HasSelection() && !computeBundle.HasLiveResources() &&
                      movedCompute.HasSelection() &&
                      movedCompute.Request().executionBackend == computeBackend,
                  "compute move construction must transfer its exact backend ownership");
    movedCompute.Reset();
    std::vector<std::string> expectedComputeTail{
        "generic-pipeline", "opaque-pipeline", "pipeline-layout",
        "descriptor-pool", "descriptor-layout"};
    if (diagnosticPolicy) expectedComputeTail.emplace_back("diagnostics-buffer");
    ok &= Require(computeLedger.destroyed.size() >= expectedComputeTail.size() &&
                      std::equal(expectedComputeTail.begin(), expectedComputeTail.end(),
                                 computeLedger.destroyed.end() - expectedComputeTail.size()) &&
                      computeLedger.sbtDestroyCalls == 0u,
                  "compute reset must destroy its graph in reverse order without SBT operations");
    const std::size_t computeDestroyed = computeLedger.destroyed.size();
    movedCompute.Reset();
    ok &= Require(computeLedger.destroyed.size() == computeDestroyed,
                  "compute move/reset cleanup must remain idempotent");

    return ok ? 0 : 1;
}
