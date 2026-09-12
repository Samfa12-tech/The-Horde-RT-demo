#include "vulkan/raytracing/RtPipelineBundleContracts.h"
#include "vulkan/raytracing/RtPipelineVariantProvider.h"
#include "vulkan/raytracing/RtSceneAbi.generated.h"

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

std::array<RtPipelineVariantArtifact, 2u> ResolveCompiledPair(
    const horde::vulkan::RtExecutionBackend executionBackend =
        horde::vulkan::RtExecutionBackend::RayTracingPipeline)
{
    const auto& provider = RtPipelineVariantProvider::Compiled(executionBackend);
    const auto opaque = provider.ResolveExact(
        {provider.request().instrumentation, provider.request().quality,
         RtMaterialStrategy::OpaqueFast, executionBackend});
    const auto generic = provider.ResolveExact(
        {provider.request().instrumentation, provider.request().quality,
         RtMaterialStrategy::GenericDielectric, executionBackend});
    if (!opaque || !generic) { throw std::runtime_error("compiled fixture pair is incomplete"); }
    return {*opaque, *generic};
}

} // namespace

int main()
{
    bool ok = true;

    const auto shipping = TryMakeRtDescriptorIoContract(RtInstrumentation::Shipping);
    const auto diagnostic = TryMakeRtDescriptorIoContract(RtInstrumentation::Diagnostic);
    ok &= Require(shipping.has_value() && diagnostic.has_value(),
                  "both instrumentation plans must exist");
    if (shipping && diagnostic) {
        ok &= Require(shipping->bindingCount == 22u &&
                          shipping->storageBufferDescriptorCount == 11u &&
                          shipping->descriptorWriteCount == 22u,
                      "Shipping must use the exact 0-21/11-storage/22-write plan");
        ok &= Require(shipping->diagnosticAvailability ==
                              RtDiagnosticAvailability::CompiledOut &&
                          !shipping->diagnosticIo.allocateBuffer &&
                          !shipping->diagnosticIo.descriptorInfo &&
                          !shipping->diagnosticIo.descriptorWrite &&
                          !shipping->diagnosticIo.priorFrameRead &&
                          !shipping->diagnosticIo.zeroReset &&
                          !shipping->diagnosticIo.shaderWriteBarrier,
                      "Shipping must compile every diagnostic resource and IO action out");
        ok &= Require(diagnostic->bindingCount == 23u &&
                          diagnostic->storageBufferDescriptorCount == 12u &&
                          diagnostic->descriptorWriteCount == 23u,
                      "Diagnostic must use the exact 0-22/12-storage/23-write plan");
        ok &= Require(diagnostic->diagnosticAvailability ==
                              RtDiagnosticAvailability::Available &&
                          diagnostic->diagnosticIo.allocateBuffer &&
                          diagnostic->diagnosticIo.descriptorInfo &&
                          diagnostic->diagnosticIo.descriptorWrite &&
                          diagnostic->diagnosticIo.priorFrameRead &&
                          diagnostic->diagnosticIo.zeroReset &&
                          diagnostic->diagnosticIo.shaderWriteBarrier,
                      "Diagnostic must retain the exact counter IO route");
        for (std::uint32_t binding = 0u; binding < shipping->bindingCount; ++binding) {
            ok &= Require(shipping->bindings[binding].binding == binding,
                          "Shipping bindings must be the contiguous 0-21 interface");
        }
        for (std::uint32_t binding = 0u; binding < diagnostic->bindingCount; ++binding) {
            ok &= Require(diagnostic->bindings[binding].binding == binding,
                          "Diagnostic bindings must be the contiguous 0-22 interface");
        }
        ok &= Require(diagnostic->bindings[22u].kind ==
                          RtDescriptorResourceKind::StorageBuffer,
                      "binding 22 must retain the generated diagnostics storage-buffer ABI");
    }
    ok &= Require(!TryMakeRtDescriptorIoContract(static_cast<RtInstrumentation>(99)).has_value(),
                  "invalid instrumentation must not inherit a nearby descriptor plan");
    ok &= Require(sizeof(RtDielectricDiagnostics) == 176u &&
                      alignof(RtDielectricDiagnostics) == 16u &&
                      kRtDielectricDiagnosticsSchema1FieldCount == 41u &&
                      offsetof(RtDielectricDiagnostics, primaryRewardBodyPixelCount) +
                              sizeof(std::uint32_t) ==
                          kRtDielectricDiagnosticsSchema1FieldCount * sizeof(std::uint32_t),
                  "schema-1 diagnostic ABI must remain 176-byte/16-byte/41-field");
    const RtDielectricDiagnostics allZeroShippingFixture{};
    ok &= Require(allZeroShippingFixture.transportOverflowCount == 0u && shipping &&
                      shipping->diagnosticAvailability == RtDiagnosticAvailability::CompiledOut,
                  "all-zero Shipping scalars must remain unavailable, not valid measurements");
    ok &= Require(ToString(RtDiagnosticAvailability::CompiledOut) == "CompiledOut" &&
                      ToString(RtDiagnosticAvailability::Available) == "Available" &&
                      ToString(RtDiagnosticAvailability::Unavailable) == "Unavailable",
                  "diagnostic availability must have explicit stable report values");

    const auto& provider = RtPipelineVariantProvider::Compiled();
    auto pair = ResolveCompiledPair();
    RtPipelineBundlePreflight preflight{};
    std::string failureKey;
    ok &= Require(ValidateRtPipelineBundlePreflight(provider.request(), pair,
                                                    preflight, failureKey),
                  "the exact compiled selected pair must pass full preflight");
    ok &= Require(failureKey.empty() && preflight.request == provider.request() &&
                      preflight.strategies[0].key.material == RtMaterialStrategy::OpaqueFast &&
                      preflight.strategies[1].key.material ==
                          RtMaterialStrategy::GenericDielectric,
                  "preflight must retain one ownership-matched ordered material pair");
    if (shipping) {
        ok &= Require(preflight.descriptorIo.instrumentation == shipping->instrumentation,
                      "preflight must carry the matching descriptor/IO authority");
    }
    for (const auto& record : pair) {
        ok &= Require(!record.artifactPath.empty() && record.expectedWordCount == record.words.size() &&
                          record.spirvSha256.size() == 64u &&
                          record.includeSha256.size() == 64u,
                      "provider records must carry complete frozen catalog metadata");
    }

    RtPipelineBundlePreflight compiledPreflight{};
    ok &= Require(ResolveCompiledRtPipelineBundlePreflight(compiledPreflight, failureKey) &&
                      failureKey.empty() &&
                      compiledPreflight.request.executionBackend ==
                          horde::vulkan::RtExecutionBackend::RayTracingPipeline &&
                      compiledPreflight.strategies[0].key.executionBackend ==
                          horde::vulkan::RtExecutionBackend::RayTracingPipeline &&
                      compiledPreflight.strategies[1].key.executionBackend ==
                          horde::vulkan::RtExecutionBackend::RayTracingPipeline,
                  "default production preflight must independently resolve the exact RTP pair");

    const auto computePair = ResolveCompiledPair(
        horde::vulkan::RtExecutionBackend::RayQueryCompute);
    RtPipelineBundlePreflight computePreflight{};
    ok &= Require(ResolveCompiledRtPipelineBundlePreflight(
                      computePreflight, failureKey,
                      horde::vulkan::RtExecutionBackend::RayQueryCompute) &&
                      failureKey.empty() &&
                      computePreflight.request.executionBackend ==
                          horde::vulkan::RtExecutionBackend::RayQueryCompute &&
                      computePreflight.strategies[0].key.executionBackend ==
                          horde::vulkan::RtExecutionBackend::RayQueryCompute &&
                      computePreflight.strategies[1].key.executionBackend ==
                          horde::vulkan::RtExecutionBackend::RayQueryCompute &&
                      computePreflight.descriptorIo.instrumentation ==
                          compiledPreflight.descriptorIo.instrumentation &&
                      computePreflight.descriptorIo.bindingCount ==
                          compiledPreflight.descriptorIo.bindingCount &&
                      computePreflight.descriptorIo.diagnosticAvailability ==
                          compiledPreflight.descriptorIo.diagnosticAvailability &&
                      computePreflight.strategies[0].canonicalKey ==
                          "rayquery_compute_shipping_mobile_opaque_fast" &&
                      computePreflight.strategies[1].canonicalKey ==
                          "rayquery_compute_shipping_mobile_generic_dielectric",
                  "compute production preflight must resolve its exact two-entry strategy pair");

    const auto computeRequest = computePreflight.request;
    ok &= Require(!ValidateRtPipelineBundlePreflight(computeRequest, pair, preflight, failureKey),
                  "compute request must reject a raygen-only pair");
    ok &= Require(!ValidateRtPipelineBundlePreflight(
                      provider.request(), computePair, preflight, failureKey) &&
                      failureKey == "shipping_mobile_opaque_fast",
                  "RTP request must reject a compute-only pair without cross-stage fallback");
    auto invalidBackendRequest = provider.request();
    invalidBackendRequest.executionBackend = static_cast<horde::vulkan::RtExecutionBackend>(99);
    ok &= Require(!ValidateRtPipelineBundlePreflight(invalidBackendRequest, pair, preflight, failureKey),
                  "invalid backend request must fail closed");

    auto disguisedRaygen = pair;
    for (auto& record : disguisedRaygen) {
        record.key.executionBackend = horde::vulkan::RtExecutionBackend::RayQueryCompute;
    }
    disguisedRaygen[0].canonicalKey = "rayquery_compute_shipping_mobile_opaque_fast";
    disguisedRaygen[0].artifactPath = "src/vulkan/raytracing/variants/rayquery_compute_shipping_mobile_opaque_fast.inc";
    disguisedRaygen[1].canonicalKey = "rayquery_compute_shipping_mobile_generic_dielectric";
    disguisedRaygen[1].artifactPath = "src/vulkan/raytracing/variants/rayquery_compute_shipping_mobile_generic_dielectric.inc";
    ok &= Require(!ValidateRtPipelineBundlePreflight(computeRequest, disguisedRaygen, preflight, failureKey),
                  "compute metadata must not disguise raygen execution-model bytes");

    auto disguisedCompute = computePair;
    for (auto& record : disguisedCompute) {
        record.key.executionBackend = horde::vulkan::RtExecutionBackend::RayTracingPipeline;
    }
    disguisedCompute[0].canonicalKey = "shipping_mobile_opaque_fast";
    disguisedCompute[0].artifactPath =
        "src/vulkan/raytracing/variants/shipping_mobile_opaque_fast.inc";
    disguisedCompute[1].canonicalKey = "shipping_mobile_generic_dielectric";
    disguisedCompute[1].artifactPath =
        "src/vulkan/raytracing/variants/shipping_mobile_generic_dielectric.inc";
    ok &= Require(!ValidateRtPipelineBundlePreflight(
                      provider.request(), disguisedCompute, preflight, failureKey),
                  "RTP metadata must not disguise compute execution-model bytes");

    const std::array<RtPipelineVariantArtifact, 1u> missingGeneric{pair[0]};
    ok &= Require(!ValidateRtPipelineBundlePreflight(provider.request(), missingGeneric,
                                                     preflight, failureKey) &&
                      failureKey == "shipping_mobile_generic_dielectric",
                  "missing second strategy must fail with its exact canonical key");
    const std::array<RtPipelineVariantArtifact, 2u> duplicateOpaque{pair[0], pair[0]};
    ok &= Require(!ValidateRtPipelineBundlePreflight(provider.request(), duplicateOpaque,
                                                     preflight, failureKey) &&
                      failureKey == "shipping_mobile_opaque_fast",
                  "duplicate strategy must fail with the duplicated exact canonical key");

    auto wrongCanonical = pair;
    wrongCanonical[0].canonicalKey = "shipping_mobile_generic_dielectric";
    ok &= Require(!ValidateRtPipelineBundlePreflight(provider.request(), wrongCanonical,
                                                     preflight, failureKey) &&
                      failureKey == "shipping_mobile_opaque_fast",
                  "wrong canonical metadata must fail with the requested key");
    auto staleHash = pair;
    staleHash[0].spirvSha256 =
        "0000000000000000000000000000000000000000000000000000000000000000";
    ok &= Require(!ValidateRtPipelineBundlePreflight(provider.request(), staleHash,
                                                     preflight, failureKey) &&
                      failureKey == "shipping_mobile_opaque_fast",
                  "stale raw hash metadata must fail before ownership");
    auto emptyWords = pair;
    emptyWords[0].words = {};
    ok &= Require(!ValidateRtPipelineBundlePreflight(provider.request(), emptyWords,
                                                     preflight, failureKey) &&
                      failureKey == "shipping_mobile_opaque_fast",
                  "empty provider words must fail before ownership");
    auto wrongWordCount = pair;
    --wrongWordCount[0].expectedWordCount;
    ok &= Require(!ValidateRtPipelineBundlePreflight(provider.request(), wrongWordCount,
                                                     preflight, failureKey) &&
                      failureKey == "shipping_mobile_opaque_fast",
                  "stale word-count metadata must fail before ownership");
    auto crossPolicy = pair;
    crossPolicy[0].key.quality = DielectricQuality::High;
    ok &= Require(!ValidateRtPipelineBundlePreflight(provider.request(), crossPolicy,
                                                     preflight, failureKey) &&
                      failureKey == "shipping_mobile_opaque_fast",
                  "cross-policy input must fail against the exact requested key");
    auto crossInstrumentation = pair;
    crossInstrumentation[0].hasDiagnosticsBinding = true;
    ok &= Require(!ValidateRtPipelineBundlePreflight(provider.request(), crossInstrumentation,
                                                     preflight, failureKey) &&
                      failureKey == "shipping_mobile_opaque_fast",
                  "cross-instrumentation reflection metadata must fail closed");

    std::vector<std::byte> misalignedStorage(pair[0].words.size_bytes() + 1u);
    auto unalignedWords = pair;
    unalignedWords[0].words = std::span<const std::uint32_t>(
        reinterpret_cast<const std::uint32_t*>(misalignedStorage.data() + 1u),
        pair[0].words.size());
    ok &= Require(!ValidateRtPipelineBundlePreflight(provider.request(), unalignedWords,
                                                     preflight, failureKey) &&
                      failureKey == "shipping_mobile_opaque_fast",
                  "unaligned word storage must fail before any SPIR-V read");

    return ok ? 0 : 1;
}
