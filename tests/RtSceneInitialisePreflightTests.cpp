#include "vulkan/raytracing/PresentableTinyRtScene.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace horde::vulkan::raytracing {

struct PresentableTinyRtScenePreflightTestAccess {
    struct Ledger {
        std::size_t resolverCalls = 0u;
        std::size_t ownershipCalls = 0u;
    };

    struct ResolverFixture {
        RtPipelineBundleRequest request{};
        std::span<const RtPipelineVariantArtifact> candidates{};
        Ledger* ledger = nullptr;
    };

    static bool Initialise(PresentableTinyRtScene& scene,
                           RtPipelineBundleRequest request,
                           std::span<const RtPipelineVariantArtifact> candidates,
                           Ledger& ledger,
                           std::string& diagnostic)
    {
        ResolverFixture fixture{request, candidates, &ledger};
        PresentableTinyRtScene::InitialiseOrchestrationApi api{};
        api.user = &fixture;
        api.resolvePreflight = [](void* user, RtPipelineBundlePreflight& preflight,
                                  std::string& failureKey) {
            auto& state = *static_cast<ResolverFixture*>(user);
            ++state.ledger->resolverCalls;
            return ValidateRtPipelineBundlePreflight(
                state.request, state.candidates, preflight, failureKey);
        };
        api.continueAfterPreflight = [](
            void* user, PresentableTinyRtScene&, VkFormat, const std::string&,
            const std::string&, const std::string&, const std::string&,
            const std::string&, const std::string&, std::string&) {
            auto& state = *static_cast<ResolverFixture*>(user);
            ++state.ledger->ownershipCalls;
            return false;
        };
        const auto poison = [](std::uintptr_t value) {
            return reinterpret_cast<void*>(value);
        };
        return scene.InitialiseWithOrchestration(
            reinterpret_cast<VkInstance>(poison(1u)),
            reinterpret_cast<VkPhysicalDevice>(poison(2u)),
            reinterpret_cast<VkDevice>(poison(3u)),
            reinterpret_cast<VkQueue>(poison(4u)),
            reinterpret_cast<VkCommandPool>(poison(5u)),
            VkExtent2D{1u, 1u}, VK_FORMAT_B8G8R8A8_UNORM,
            {}, {}, {}, {}, diagnostic, {}, {}, api);
    }
};

} // namespace horde::vulkan::raytracing

namespace {

using namespace horde::vulkan::raytracing;

bool Require(bool condition, std::string_view message)
{
    if (!condition) { std::cerr << message << '\n'; }
    return condition;
}

bool RejectsBeforeOwnership(
    RtPipelineBundleRequest request,
    std::span<const RtPipelineVariantArtifact> candidates,
    std::string_view expectedFailureKey)
{
    PresentableTinyRtScene scene;
    PresentableTinyRtScenePreflightTestAccess::Ledger ledger{};
    std::string diagnostic;
    const bool initialised = PresentableTinyRtScenePreflightTestAccess::Initialise(
        scene, request, candidates, ledger, diagnostic);
    return !initialised && diagnostic == expectedFailureKey && !scene.IsReady() &&
           scene.SelectedOpaqueFastKey().empty() &&
           scene.SelectedGenericDielectricKey().empty() &&
           ledger.resolverCalls == 1u && ledger.ownershipCalls == 0u;
}

} // namespace

int main()
{
    bool ok = true;
    const auto& provider = RtPipelineVariantProvider::Compiled();
    const auto opaque = provider.ResolveExact(
        {provider.request().instrumentation, provider.request().quality,
         RtMaterialStrategy::OpaqueFast});
    const auto generic = provider.ResolveExact(
        {provider.request().instrumentation, provider.request().quality,
         RtMaterialStrategy::GenericDielectric});
    ok &= Require(opaque.has_value() && generic.has_value(),
                  "scene preflight tests require the genuine compiled provider pair");
    if (!opaque || !generic) { return 1; }

    const std::array genuinePair{*opaque, *generic};
    const std::string opaqueKey(opaque->canonicalKey);
    const std::string genericKey(generic->canonicalKey);

    const std::array<RtPipelineVariantArtifact, 1u> missingGeneric{*opaque};
    ok &= Require(RejectsBeforeOwnership(provider.request(), missingGeneric, genericKey),
                  "production initialise must reject a missing strategy before ownership");

    const std::array duplicateOpaque{*opaque, *opaque};
    ok &= Require(RejectsBeforeOwnership(provider.request(), duplicateOpaque, opaqueKey),
                  "production initialise must reject a duplicate strategy before ownership");

    auto mismatchedMetadata = genuinePair;
    mismatchedMetadata[0].canonicalKey = generic->canonicalKey;
    ok &= Require(RejectsBeforeOwnership(
                      provider.request(), mismatchedMetadata, opaqueKey),
                  "production initialise must reject mismatched canonical metadata before ownership");

    auto staleHash = genuinePair;
    staleHash[0].spirvSha256 =
        "0000000000000000000000000000000000000000000000000000000000000000";
    ok &= Require(RejectsBeforeOwnership(provider.request(), staleHash, opaqueKey),
                  "production initialise must reject stale provider metadata before ownership");

    auto crossPolicy = genuinePair;
    crossPolicy[0].key.quality = provider.request().quality == DielectricQuality::Mobile
        ? DielectricQuality::High : DielectricQuality::Mobile;
    ok &= Require(RejectsBeforeOwnership(provider.request(), crossPolicy, opaqueKey),
                  "production initialise must reject cross-policy provider input before ownership");

    auto emptyWords = genuinePair;
    emptyWords[0].words = {};
    ok &= Require(RejectsBeforeOwnership(provider.request(), emptyWords, opaqueKey),
                  "production initialise must reject an empty module before ownership");

    std::vector<std::byte> unalignedStorage(opaque->words.size_bytes() + 1u);
    auto unalignedWords = genuinePair;
    unalignedWords[0].words = std::span<const std::uint32_t>(
        reinterpret_cast<const std::uint32_t*>(unalignedStorage.data() + 1u),
        opaque->words.size());
    ok &= Require(RejectsBeforeOwnership(provider.request(), unalignedWords, opaqueKey),
                  "production initialise must reject an unaligned module before ownership");

    return ok ? 0 : 1;
}
