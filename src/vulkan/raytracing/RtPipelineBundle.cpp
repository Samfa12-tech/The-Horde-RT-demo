#include "vulkan/raytracing/RtPipelineBundle.h"

#include <utility>

namespace horde::vulkan::raytracing {
namespace {

constexpr std::size_t StrategyIndex(RtMaterialStrategy strategy) noexcept
{
    return strategy == RtMaterialStrategy::GenericDielectric ? 1u : 0u;
}

RtPipelineOwnedBuffer OwnedSbtKind(RtMaterialStrategy strategy) noexcept
{
    return strategy == RtMaterialStrategy::GenericDielectric
        ? RtPipelineOwnedBuffer::GenericDielectricSbt
        : RtPipelineOwnedBuffer::OpaqueFastSbt;
}

bool SameDescriptorIoContract(const RtDescriptorIoContract& candidate,
                              const RtDescriptorIoContract& authoritative) noexcept
{
    if (candidate.instrumentation != authoritative.instrumentation ||
        candidate.bindingCount != authoritative.bindingCount ||
        candidate.storageBufferDescriptorCount !=
            authoritative.storageBufferDescriptorCount ||
        candidate.descriptorWriteCount != authoritative.descriptorWriteCount ||
        candidate.diagnosticAvailability != authoritative.diagnosticAvailability ||
        candidate.diagnosticIo.allocateBuffer != authoritative.diagnosticIo.allocateBuffer ||
        candidate.diagnosticIo.descriptorInfo != authoritative.diagnosticIo.descriptorInfo ||
        candidate.diagnosticIo.descriptorWrite != authoritative.diagnosticIo.descriptorWrite ||
        candidate.diagnosticIo.priorFrameRead != authoritative.diagnosticIo.priorFrameRead ||
        candidate.diagnosticIo.zeroReset != authoritative.diagnosticIo.zeroReset ||
        candidate.diagnosticIo.shaderWriteBarrier !=
            authoritative.diagnosticIo.shaderWriteBarrier) {
        return false;
    }
    for (std::size_t index = 0u; index < candidate.bindings.size(); ++index) {
        if (candidate.bindings[index].binding != authoritative.bindings[index].binding ||
            candidate.bindings[index].kind != authoritative.bindings[index].kind) {
            return false;
        }
    }
    return true;
}

bool SameAuthoritativeArtifact(const RtPipelineVariantArtifact& candidate,
                               const RtPipelineVariantArtifact& authoritative) noexcept
{
    return candidate.key == authoritative.key &&
           candidate.words.data() == authoritative.words.data() &&
           candidate.words.size() == authoritative.words.size() &&
           candidate.canonicalKey == authoritative.canonicalKey &&
           candidate.artifactPath == authoritative.artifactPath &&
           candidate.spirvSha256 == authoritative.spirvSha256 &&
           candidate.includeSha256 == authoritative.includeSha256 &&
           candidate.expectedWordCount == authoritative.expectedWordCount &&
           candidate.atomicInstructions == authoritative.atomicInstructions &&
           candidate.hasDiagnosticsBinding == authoritative.hasDiagnosticsBinding;
}

bool SameCompiledPreflight(const RtPipelineBundlePreflight& candidate,
                           const RtPipelineBundlePreflight& authoritative) noexcept
{
    return candidate.request == authoritative.request &&
           SameDescriptorIoContract(candidate.descriptorIo, authoritative.descriptorIo) &&
           SameAuthoritativeArtifact(candidate.strategies[0], authoritative.strategies[0]) &&
           SameAuthoritativeArtifact(candidate.strategies[1], authoritative.strategies[1]);
}

} // namespace

bool RtPipelineBundleDestroyApi::Complete() const noexcept
{
    return destroyBuffer && destroyPipeline && destroyPipelineLayout &&
           destroyDescriptorPool && destroyDescriptorSetLayout;
}

bool RtPipelineBundleBuildApi::Complete() const noexcept
{
    return createDescriptorSetLayout && createDescriptorPool && allocateDescriptorSet &&
           createDiagnosticBuffer && writeDescriptors && createPipelineLayout &&
           createSharedShaderModules && createRaygenShaderModule &&
           createStrategyPipeline && destroyShaderModule && createStrategySbt;
}

RtPipelineBundle::~RtPipelineBundle()
{
    Reset();
}

RtPipelineBundle::RtPipelineBundle(RtPipelineBundle&& other) noexcept
{
    MoveFrom(std::move(other));
}

RtPipelineBundle& RtPipelineBundle::operator=(RtPipelineBundle&& other) noexcept
{
    if (this != &other) {
        Reset();
        MoveFrom(std::move(other));
    }
    return *this;
}

bool RtPipelineBundle::AdoptPreflight(RtPipelineBundlePreflight preflight,
                                      RtPipelineBundleDestroyApi destroyApi,
                                      std::string& diagnostic)
{
    RtPipelineBundlePreflight authoritative{};
    std::string authorityFailure;
    if (selected_ || HasLiveResources() || !destroyApi.Complete() ||
        !ResolveCompiledRtPipelineBundlePreflight(authoritative, authorityFailure) ||
        !SameCompiledPreflight(preflight, authoritative)) {
        diagnostic = "Invalid selected RT pipeline bundle preflight.";
        return false;
    }
    preflight_ = std::move(preflight);
    strategies_[0].artifact = preflight_.strategies[0];
    strategies_[1].artifact = preflight_.strategies[1];
    destroyApi_ = destroyApi;
    diagnosticAvailability_ = preflight_.request.instrumentation == RtInstrumentation::Shipping
        ? RtDiagnosticAvailability::CompiledOut
        : RtDiagnosticAvailability::Unavailable;
    selected_ = true;
    diagnostic.clear();
    return true;
}

void RtPipelineBundle::RebindDestroyContext(void* user,
                                            RtGpuResources* gpuResources) noexcept
{
    destroyApi_.user = user;
    destroyApi_.gpuResources = gpuResources;
}

void RtPipelineBundle::Reset() noexcept
{
    if (destroyApi_.Complete()) {
        for (std::size_t index = strategies_.size(); index-- > 0u;) {
            auto& strategy = strategies_[index];
            strategy.sbtRegions = {};
            destroyApi_.destroyBuffer(destroyApi_.user, destroyApi_.gpuResources,
                                      strategy.shaderBindingTable,
                                      OwnedSbtKind(strategy.artifact.key.material));
            destroyApi_.destroyPipeline(destroyApi_.user, strategy.pipeline,
                                        strategy.artifact.key.material);
        }
        destroyApi_.destroyPipelineLayout(destroyApi_.user, pipelineLayout);
        descriptorSet = VK_NULL_HANDLE;
        destroyApi_.destroyDescriptorPool(destroyApi_.user, descriptorPool);
        destroyApi_.destroyDescriptorSetLayout(destroyApi_.user, descriptorSetLayout);
        destroyApi_.destroyBuffer(destroyApi_.user, destroyApi_.gpuResources,
                                  diagnosticBuffer, RtPipelineOwnedBuffer::Diagnostics);
    } else {
        for (auto& strategy : strategies_) {
            strategy.sbtRegions = {};
            strategy.shaderBindingTable = {};
            strategy.pipeline = VK_NULL_HANDLE;
        }
        pipelineLayout = VK_NULL_HANDLE;
        descriptorSet = VK_NULL_HANDLE;
        descriptorPool = VK_NULL_HANDLE;
        descriptorSetLayout = VK_NULL_HANDLE;
        diagnosticBuffer = {};
    }
    preflight_ = {};
    strategies_ = {};
    destroyApi_ = {};
    diagnosticAvailability_ = RtDiagnosticAvailability::Unavailable;
    selected_ = false;
}

bool RtPipelineBundle::HasLiveResources() const noexcept
{
    if (descriptorSetLayout != VK_NULL_HANDLE || descriptorPool != VK_NULL_HANDLE ||
        descriptorSet != VK_NULL_HANDLE || pipelineLayout != VK_NULL_HANDLE ||
        diagnosticBuffer.buffer != VK_NULL_HANDLE || diagnosticBuffer.memory != VK_NULL_HANDLE) {
        return true;
    }
    for (const auto& strategy : strategies_) {
        if (strategy.pipeline != VK_NULL_HANDLE ||
            strategy.shaderBindingTable.buffer != VK_NULL_HANDLE ||
            strategy.shaderBindingTable.memory != VK_NULL_HANDLE) {
            return true;
        }
    }
    return false;
}

std::string_view RtPipelineBundle::OpaqueFastKey() const noexcept
{
    return selected_ ? strategies_[0].artifact.canonicalKey : std::string_view{};
}

std::string_view RtPipelineBundle::GenericDielectricKey() const noexcept
{
    return selected_ ? strategies_[1].artifact.canonicalKey : std::string_view{};
}

std::string_view RtPipelineBundle::OpaqueFastSha256() const noexcept
{
    return selected_ ? strategies_[0].artifact.spirvSha256 : std::string_view{};
}

std::string_view RtPipelineBundle::GenericDielectricSha256() const noexcept
{
    return selected_ ? strategies_[1].artifact.spirvSha256 : std::string_view{};
}

std::string RtPipelineBundle::FullPairIdentity() const
{
    if (!selected_) {
        return {};
    }
    std::string identity;
    identity.reserve(OpaqueFastKey().size() + OpaqueFastSha256().size() +
                     GenericDielectricKey().size() + GenericDielectricSha256().size() + 35u);
    identity.append("opaqueFast:");
    identity.append(OpaqueFastKey());
    identity.push_back('@');
    identity.append(OpaqueFastSha256());
    identity.append("|genericDielectric:");
    identity.append(GenericDielectricKey());
    identity.push_back('@');
    identity.append(GenericDielectricSha256());
    return identity;
}

std::string RtPipelineBundle::ShortPairIdentity() const
{
    if (!selected_) {
        return {};
    }
    constexpr std::size_t kShortHashLength = 8u;
    std::string identity;
    identity.reserve(kShortHashLength * 2u + 1u);
    identity.append(OpaqueFastSha256().substr(0u, kShortHashLength));
    identity.push_back('+');
    identity.append(GenericDielectricSha256().substr(0u, kShortHashLength));
    return identity;
}

RtStrategyPipelineResources& RtPipelineBundle::Strategy(
    RtMaterialStrategy strategy) noexcept
{
    return strategies_[StrategyIndex(strategy)];
}

const RtStrategyPipelineResources& RtPipelineBundle::Strategy(
    RtMaterialStrategy strategy) const noexcept
{
    return strategies_[StrategyIndex(strategy)];
}

void RtPipelineBundle::MoveFrom(RtPipelineBundle&& other) noexcept
{
    preflight_ = std::exchange(other.preflight_, RtPipelineBundlePreflight{});
    strategies_ = std::exchange(other.strategies_, {});
    destroyApi_ = std::exchange(other.destroyApi_, {});
    diagnosticAvailability_ = std::exchange(
        other.diagnosticAvailability_, RtDiagnosticAvailability::Unavailable);
    selected_ = std::exchange(other.selected_, false);
    descriptorSetLayout = std::exchange(other.descriptorSetLayout, VK_NULL_HANDLE);
    descriptorPool = std::exchange(other.descriptorPool, VK_NULL_HANDLE);
    descriptorSet = std::exchange(other.descriptorSet, VK_NULL_HANDLE);
    pipelineLayout = std::exchange(other.pipelineLayout, VK_NULL_HANDLE);
    diagnosticBuffer = std::exchange(other.diagnosticBuffer, RtGpuBuffer{});
}

bool BuildRtPipelineBundleResources(RtPipelineBundle& bundle,
                                    const RtPipelineBundleBuildApi& api,
                                    std::string& diagnostic)
{
    if (!bundle.selected_ || bundle.HasLiveResources() || !api.Complete()) {
        diagnostic = "Invalid RT pipeline bundle construction state.";
        return false;
    }
    VkShaderModule missModule = VK_NULL_HANDLE;
    VkShaderModule hitModule = VK_NULL_HANDLE;
    VkShaderModule raygenModule = VK_NULL_HANDLE;
    const auto fail = [&]() {
        api.destroyShaderModule(api.user, raygenModule);
        api.destroyShaderModule(api.user, hitModule);
        api.destroyShaderModule(api.user, missModule);
        bundle.Reset();
        if (diagnostic.empty()) { diagnostic = "Failed to create selected RT pipeline bundle."; }
        return false;
    };

    if (!api.createDescriptorSetLayout(api.user, bundle.preflight_.descriptorIo,
                                       bundle.descriptorSetLayout, diagnostic) ||
        !api.createDescriptorPool(api.user, bundle.preflight_.descriptorIo,
                                  bundle.descriptorPool, diagnostic) ||
        !api.allocateDescriptorSet(api.user, bundle.descriptorPool,
                                   bundle.descriptorSetLayout, bundle.descriptorSet,
                                   diagnostic)) {
        return fail();
    }
    if (bundle.preflight_.descriptorIo.diagnosticIo.allocateBuffer) {
        if (!api.createDiagnosticBuffer(api.user, bundle.diagnosticBuffer, diagnostic)) {
            return fail();
        }
        bundle.diagnosticAvailability_ = RtDiagnosticAvailability::Available;
    }
    if (!api.writeDescriptors(api.user, bundle, diagnostic) ||
        !api.createPipelineLayout(api.user, bundle.descriptorSetLayout,
                                  bundle.pipelineLayout, diagnostic) ||
        !api.createSharedShaderModules(api.user, missModule, hitModule, diagnostic)) {
        return fail();
    }

    for (const RtMaterialStrategy material :
         {RtMaterialStrategy::OpaqueFast, RtMaterialStrategy::GenericDielectric}) {
        auto& strategy = bundle.Strategy(material);
        if (!api.createRaygenShaderModule(api.user, strategy.artifact,
                                          raygenModule, diagnostic) ||
            !api.createStrategyPipeline(api.user, material, raygenModule,
                                        missModule, hitModule, bundle.pipelineLayout,
                                        strategy.pipeline, diagnostic)) {
            return fail();
        }
        api.destroyShaderModule(api.user, raygenModule);
    }
    api.destroyShaderModule(api.user, hitModule);
    api.destroyShaderModule(api.user, missModule);

    for (const RtMaterialStrategy material :
         {RtMaterialStrategy::OpaqueFast, RtMaterialStrategy::GenericDielectric}) {
        auto& strategy = bundle.Strategy(material);
        if (!api.createStrategySbt(api.user, material, strategy.pipeline,
                                   strategy.shaderBindingTable, strategy.sbtRegions,
                                   diagnostic)) {
            return fail();
        }
    }
    diagnostic.clear();
    return true;
}

} // namespace horde::vulkan::raytracing
