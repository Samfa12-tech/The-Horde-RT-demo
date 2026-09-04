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

#if HORDE_RT_SELECTED_INSTRUMENTATION == 1 && HORDE_RT_SELECTED_DIELECTRIC_QUALITY == 1
constexpr std::string_view kExpectedFullPairIdentity =
    "opaqueFast:diagnostic_high_opaque_fast@d7b0722e572e63bd96e62bae85cc6a5b968ae71bf157526b513b50b56e515c3b|"
    "genericDielectric:diagnostic_high_generic_dielectric@2b61176f676227adfd13a880cd415478536e1403b46317ea3321dc463bebecd0";
constexpr std::string_view kExpectedShortPairIdentity = "d7b0722e+2b61176f";
#elif HORDE_RT_SELECTED_INSTRUMENTATION == 0 && HORDE_RT_SELECTED_DIELECTRIC_QUALITY == 1
constexpr std::string_view kExpectedFullPairIdentity =
    "opaqueFast:shipping_high_opaque_fast@7ebdb794794a854b6cb5c44c75dd9a9decd42f4999c44a9cfa78f13b12a13f21|"
    "genericDielectric:shipping_high_generic_dielectric@1be2b430ba190ae82250693ae079adf3f0712b361914b1355f386fb1f57fc5aa";
constexpr std::string_view kExpectedShortPairIdentity = "7ebdb794+1be2b430";
#else
#error "Lifetime pair-identity fixture expects the Windows High policy."
#endif

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

    bool Begin(RtPipelineBundleBuildStep step, std::string_view name)
    {
        created.emplace_back(name);
        return failure != step;
    }
};

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
    api.allocateDescriptorSet = [](void* user, VkDescriptorPool, VkDescriptorSetLayout,
                                   VkDescriptorSet& out, std::string&) {
        auto& state = *static_cast<Ledger*>(user);
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
        return succeeds;
    };
    api.createSharedShaderModules = [](void* user, VkShaderModule& miss,
                                       VkShaderModule& hit, std::string&) {
        auto& state = *static_cast<Ledger*>(user);
        const bool succeeds = state.Begin(
            RtPipelineBundleBuildStep::SharedShaderModules, "shared-modules");
        miss = FakeHandle<VkShaderModule>(state.nextHandle++);
        hit = FakeHandle<VkShaderModule>(state.nextHandle++);
        return succeeds;
    };
    api.createRaygenShaderModule = [](void* user, const RtPipelineVariantArtifact& artifact,
                                      VkShaderModule& out, std::string&) {
        auto& state = *static_cast<Ledger*>(user);
        const auto step = artifact.key.material == RtMaterialStrategy::OpaqueFast
            ? RtPipelineBundleBuildStep::OpaqueFastShaderModule
            : RtPipelineBundleBuildStep::GenericDielectricShaderModule;
        const bool succeeds = state.Begin(
            step, std::string(StrategyName(artifact.key.material)) + "-module");
        out = FakeHandle<VkShaderModule>(state.nextHandle++);
        return succeeds;
    };
    api.createStrategyPipeline = [](void* user, RtMaterialStrategy strategy,
                                    VkShaderModule, VkShaderModule, VkShaderModule,
                                    VkPipelineLayout, VkPipeline& out, std::string&) {
        auto& state = *static_cast<Ledger*>(user);
        const auto step = strategy == RtMaterialStrategy::OpaqueFast
            ? RtPipelineBundleBuildStep::OpaqueFastPipeline
            : RtPipelineBundleBuildStep::GenericDielectricPipeline;
        const bool succeeds = state.Begin(
            step, std::string(StrategyName(strategy)) + "-pipeline");
        out = FakeHandle<VkPipeline>(state.nextHandle++);
        return succeeds;
    };
    api.destroyShaderModule = [](void* user, VkShaderModule& module) noexcept {
        if (module == VK_NULL_HANDLE) { return; }
        static_cast<Ledger*>(user)->destroyed.emplace_back("temporary-module");
        module = VK_NULL_HANDLE;
    };
    api.createStrategySbt = [](void* user, RtMaterialStrategy strategy, VkPipeline,
                               RtGpuBuffer& out,
                               std::array<VkStridedDeviceAddressRegionKHR, 4u>& regions,
                               std::string&) {
        auto& state = *static_cast<Ledger*>(user);
        const auto step = strategy == RtMaterialStrategy::OpaqueFast
            ? RtPipelineBundleBuildStep::OpaqueFastSbt
            : RtPipelineBundleBuildStep::GenericDielectricSbt;
        const bool succeeds = state.Begin(
            step, std::string(StrategyName(strategy)) + "-sbt");
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
    return api;
}

RtPipelineBundlePreflight MakePreflight()
{
    RtPipelineBundlePreflight preflight{};
    std::string error;
    if (!ResolveCompiledRtPipelineBundlePreflight(preflight, error)) {
        throw std::runtime_error(error);
    }
    return preflight;
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
                          RtDiagnosticAvailability::Unavailable,
                      "partial construction failure must leave no selected/live bundle");
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
    ok &= Require(bundle.AdoptPreflight(MakePreflight(),
                                        MakeDestroyApi(ledger), error) &&
                      BuildRtPipelineBundleResources(bundle, MakeBuildApi(ledger), error),
                  "complete compiled-policy bundle construction must succeed");
    ok &= Require(bundle.DiagnosticAvailability() ==
                          (diagnosticPolicy ? RtDiagnosticAvailability::Available
                                            : RtDiagnosticAvailability::CompiledOut) &&
                      bundle.Strategy(RtMaterialStrategy::OpaqueFast).pipeline != VK_NULL_HANDLE &&
                      bundle.Strategy(RtMaterialStrategy::GenericDielectric).pipeline != VK_NULL_HANDLE,
                  "the complete bundle must own both material strategy records");
    ok &= Require(bundle.FullPairIdentity() == kExpectedFullPairIdentity,
                  "the full selected-pair identity must contain both canonical keys and hashes");
    ok &= Require(bundle.ShortPairIdentity() == kExpectedShortPairIdentity,
                  "the display identity must be derived from both selected module hashes");
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

    return ok ? 0 : 1;
}
