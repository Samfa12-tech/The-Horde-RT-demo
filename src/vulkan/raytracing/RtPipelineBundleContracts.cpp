#include "vulkan/raytracing/RtPipelineBundleContracts.h"

#include "vulkan/raytracing/RtSceneAbi.generated.h"

#include <algorithm>
#include <array>
#include <bit>
#include <iomanip>
#include <limits>
#include <sstream>

namespace horde::vulkan::raytracing {
namespace {

constexpr std::uint32_t kSpirvMagic = 0x07230203u;
constexpr std::uint16_t kOpEntryPoint = 15u;
constexpr std::uint16_t kOpDecorate = 71u;
constexpr std::uint32_t kExecutionModelRayGenerationKhr = 5313u;
constexpr std::uint32_t kDecorationBinding = 33u;

constexpr std::array<RtDescriptorResourceKind, 23u> kDescriptorKinds{
    RtDescriptorResourceKind::AccelerationStructure,
    RtDescriptorResourceKind::StorageImage,
    RtDescriptorResourceKind::StorageBuffer,
    RtDescriptorResourceKind::CombinedImageSampler,
    RtDescriptorResourceKind::CombinedImageSampler,
    RtDescriptorResourceKind::CombinedImageSampler,
    RtDescriptorResourceKind::StorageBuffer,
    RtDescriptorResourceKind::StorageBuffer,
    RtDescriptorResourceKind::CombinedImageSampler,
    RtDescriptorResourceKind::CombinedImageSampler,
    RtDescriptorResourceKind::StorageBuffer,
    RtDescriptorResourceKind::StorageBuffer,
    RtDescriptorResourceKind::StorageBuffer,
    RtDescriptorResourceKind::StorageBuffer,
    RtDescriptorResourceKind::StorageBuffer,
    RtDescriptorResourceKind::StorageBuffer,
    RtDescriptorResourceKind::CombinedImageSampler,
    RtDescriptorResourceKind::CombinedImageSampler,
    RtDescriptorResourceKind::CombinedImageSampler,
    RtDescriptorResourceKind::CombinedImageSampler,
    RtDescriptorResourceKind::StorageBuffer,
    RtDescriptorResourceKind::StorageBuffer,
    RtDescriptorResourceKind::StorageBuffer,
};

constexpr std::uint32_t RotateRight(std::uint32_t value, std::uint32_t bits) noexcept
{
    return (value >> bits) | (value << (32u - bits));
}

std::string Sha256Words(std::span<const std::uint32_t> words)
{
    constexpr std::array<std::uint32_t, 64u> constants{
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
        0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
        0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
        0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
        0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
        0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
        0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
        0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
    };
    std::array<std::uint32_t, 8u> state{
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u,
    };
    const std::uint64_t byteCount = static_cast<std::uint64_t>(words.size()) * 4u;
    const std::uint64_t paddedBytes = ((byteCount + 9u + 63u) / 64u) * 64u;
    std::array<std::uint8_t, 64u> block{};
    std::array<std::uint32_t, 64u> schedule{};
    for (std::uint64_t blockOffset = 0u; blockOffset < paddedBytes; blockOffset += 64u) {
        block.fill(0u);
        for (std::uint64_t i = 0u; i < 64u; ++i) {
            const std::uint64_t byteOffset = blockOffset + i;
            if (byteOffset < byteCount) {
                const std::uint32_t word = words[static_cast<std::size_t>(byteOffset / 4u)];
                block[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(
                    word >> ((byteOffset % 4u) * 8u));
            } else if (byteOffset == byteCount) {
                block[static_cast<std::size_t>(i)] = 0x80u;
            }
        }
        if (blockOffset + 64u == paddedBytes) {
            const std::uint64_t bitCount = byteCount * 8u;
            for (std::uint32_t i = 0u; i < 8u; ++i) {
                block[56u + i] = static_cast<std::uint8_t>(bitCount >> ((7u - i) * 8u));
            }
        }
        for (std::uint32_t i = 0u; i < 16u; ++i) {
            schedule[i] = (static_cast<std::uint32_t>(block[i * 4u]) << 24u) |
                          (static_cast<std::uint32_t>(block[i * 4u + 1u]) << 16u) |
                          (static_cast<std::uint32_t>(block[i * 4u + 2u]) << 8u) |
                          static_cast<std::uint32_t>(block[i * 4u + 3u]);
        }
        for (std::uint32_t i = 16u; i < 64u; ++i) {
            const std::uint32_t s0 = RotateRight(schedule[i - 15u], 7u) ^
                                     RotateRight(schedule[i - 15u], 18u) ^
                                     (schedule[i - 15u] >> 3u);
            const std::uint32_t s1 = RotateRight(schedule[i - 2u], 17u) ^
                                     RotateRight(schedule[i - 2u], 19u) ^
                                     (schedule[i - 2u] >> 10u);
            schedule[i] = schedule[i - 16u] + s0 + schedule[i - 7u] + s1;
        }
        std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        std::uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
        for (std::uint32_t i = 0u; i < 64u; ++i) {
            const std::uint32_t sum1 = RotateRight(e, 6u) ^ RotateRight(e, 11u) ^ RotateRight(e, 25u);
            const std::uint32_t choice = (e & f) ^ ((~e) & g);
            const std::uint32_t temp1 = h + sum1 + choice + constants[i] + schedule[i];
            const std::uint32_t sum0 = RotateRight(a, 2u) ^ RotateRight(a, 13u) ^ RotateRight(a, 22u);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = sum0 + majority;
            h = g; g = f; f = e; e = d + temp1; d = c; c = b; b = a; a = temp1 + temp2;
        }
        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        state[4] += e; state[5] += f; state[6] += g; state[7] += h;
    }
    std::ostringstream digest;
    digest << std::hex << std::setfill('0');
    for (const std::uint32_t word : state) { digest << std::setw(8) << word; }
    return digest.str();
}

bool IsLowerHexSha256(std::string_view value) noexcept
{
    return value.size() == 64u && std::all_of(value.begin(), value.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}

struct ReflectedModuleContract {
    std::size_t raygenEntryPoints = 0u;
    std::size_t diagnosticsBindings = 0u;
    std::size_t atomicInstructions = 0u;
};

bool IsAtomicOpcode(std::uint16_t opcode) noexcept
{
    return (opcode >= 227u && opcode <= 242u) || opcode == 318u || opcode == 319u;
}

bool ReflectModule(std::span<const std::uint32_t> words,
                   ReflectedModuleContract& reflected) noexcept
{
    reflected = {};
    if (words.size() < 5u || words[0] != kSpirvMagic || words[4] != 0u) { return false; }
    for (std::size_t offset = 5u; offset < words.size();) {
        const std::uint32_t instruction = words[offset];
        const std::uint16_t wordCount = static_cast<std::uint16_t>(instruction >> 16u);
        const std::uint16_t opcode = static_cast<std::uint16_t>(instruction & 0xffffu);
        if (wordCount == 0u || wordCount > words.size() - offset) { return false; }
        if (opcode == kOpEntryPoint && wordCount >= 4u &&
            words[offset + 1u] == kExecutionModelRayGenerationKhr) {
            ++reflected.raygenEntryPoints;
        }
        if (opcode == kOpDecorate && wordCount >= 4u &&
            words[offset + 2u] == kDecorationBinding &&
            words[offset + 3u] == kRtBindingDielectricDiagnostics) {
            ++reflected.diagnosticsBindings;
        }
        if (IsAtomicOpcode(opcode)) { ++reflected.atomicInstructions; }
        offset += wordCount;
    }
    return reflected.raygenEntryPoints == 1u;
}

bool ValidateRecord(const RtPipelineVariantArtifact& record,
                    RtPipelineVariantKey expected,
                    const RtDescriptorIoContract& descriptorIo,
                    std::string_view expectedKey)
{
    if (record.key != expected || record.canonicalKey != expectedKey ||
        record.artifactPath != std::string("src/vulkan/raytracing/variants/") +
                                   std::string(expectedKey) + ".inc" ||
        !IsLowerHexSha256(record.spirvSha256) ||
        !IsLowerHexSha256(record.includeSha256) ||
        record.words.empty() || record.expectedWordCount != record.words.size() ||
        record.words.size_bytes() % sizeof(std::uint32_t) != 0u ||
        reinterpret_cast<std::uintptr_t>(record.words.data()) % alignof(std::uint32_t) != 0u) {
        return false;
    }
    ReflectedModuleContract reflected{};
    if (!ReflectModule(record.words, reflected) ||
        reflected.atomicInstructions != record.atomicInstructions ||
        (reflected.diagnosticsBindings != 0u) != record.hasDiagnosticsBinding ||
        record.hasDiagnosticsBinding != descriptorIo.diagnosticIo.descriptorWrite ||
        (descriptorIo.instrumentation == RtInstrumentation::Shipping &&
         reflected.atomicInstructions != 0u) ||
        (descriptorIo.instrumentation == RtInstrumentation::Diagnostic &&
         (reflected.atomicInstructions == 0u || reflected.diagnosticsBindings != 1u))) {
        return false;
    }
    return Sha256Words(record.words) == record.spirvSha256;
}

} // namespace

std::string_view ToString(RtDiagnosticAvailability availability) noexcept
{
    switch (availability) {
    case RtDiagnosticAvailability::Unavailable: return "Unavailable";
    case RtDiagnosticAvailability::CompiledOut: return "CompiledOut";
    case RtDiagnosticAvailability::Available: return "Available";
    }
    return "Unavailable";
}

std::optional<RtDescriptorIoContract> TryMakeRtDescriptorIoContract(
    RtInstrumentation instrumentation) noexcept
{
    if (instrumentation != RtInstrumentation::Shipping &&
        instrumentation != RtInstrumentation::Diagnostic) {
        return std::nullopt;
    }
    const bool diagnostic = instrumentation == RtInstrumentation::Diagnostic;
    RtDescriptorIoContract contract{};
    contract.instrumentation = instrumentation;
    contract.bindingCount = diagnostic ? 23u : 22u;
    contract.descriptorWriteCount = contract.bindingCount;
    contract.diagnosticAvailability = diagnostic
        ? RtDiagnosticAvailability::Available : RtDiagnosticAvailability::CompiledOut;
    contract.diagnosticIo = diagnostic
        ? RtDiagnosticIoContract{true, true, true, true, true, true}
        : RtDiagnosticIoContract{};
    for (std::uint32_t binding = 0u; binding < contract.bindingCount; ++binding) {
        contract.bindings[binding] = {binding, kDescriptorKinds[binding]};
        if (kDescriptorKinds[binding] == RtDescriptorResourceKind::StorageBuffer) {
            ++contract.storageBufferDescriptorCount;
        }
    }
    return contract;
}

bool ValidateRtPipelineBundlePreflight(
    RtPipelineBundleRequest request,
    std::span<const RtPipelineVariantArtifact> candidateRecords,
    RtPipelineBundlePreflight& preflight,
    std::string& failureKey)
{
    preflight = {};
    failureKey.clear();
    if (!TryMakeRtPipelineBundleRequest(request.instrumentation, request.quality)) {
        failureKey = "invalid_rt_pipeline_bundle_request";
        return false;
    }
    const auto descriptorIo = TryMakeRtDescriptorIoContract(request.instrumentation);
    if (!descriptorIo) {
        failureKey = "invalid_rt_pipeline_bundle_request";
        return false;
    }
    constexpr std::array<RtMaterialStrategy, 2u> strategies{
        RtMaterialStrategy::OpaqueFast, RtMaterialStrategy::GenericDielectric};
    std::array<RtPipelineVariantArtifact, 2u> selected{};
    for (std::size_t strategyIndex = 0u; strategyIndex < strategies.size(); ++strategyIndex) {
        const RtPipelineVariantKey expected{request.instrumentation, request.quality,
                                            strategies[strategyIndex]};
        const std::string_view expectedKey = FormatRtPipelineVariantKey(expected);
        std::size_t matches = 0u;
        for (const auto& candidate : candidateRecords) {
            if (candidate.key == expected) {
                selected[strategyIndex] = candidate;
                ++matches;
            }
        }
        if (matches != 1u ||
            !ValidateRecord(selected[strategyIndex], expected, *descriptorIo, expectedKey)) {
            failureKey = std::string(expectedKey);
            return false;
        }
    }
    if (candidateRecords.size() != selected.size()) {
        failureKey = std::string(candidateRecords.size() > selected.size()
            ? candidateRecords[selected.size()].canonicalKey
            : FormatRtPipelineVariantKey(selected.back().key));
        return false;
    }
    preflight.request = request;
    preflight.descriptorIo = *descriptorIo;
    preflight.strategies = selected;
    return true;
}

bool ResolveCompiledRtPipelineBundlePreflight(
    RtPipelineBundlePreflight& preflight,
    std::string& failureKey)
{
    preflight = {};
    failureKey.clear();
    const auto& provider = RtPipelineVariantProvider::Compiled();
    const RtPipelineBundleRequest request = provider.request();
    const RtPipelineVariantKey opaqueKey{
        request.instrumentation, request.quality, RtMaterialStrategy::OpaqueFast};
    const RtPipelineVariantKey genericKey{
        request.instrumentation, request.quality, RtMaterialStrategy::GenericDielectric};
    const auto opaque = provider.ResolveExact(opaqueKey, &failureKey);
    if (!opaque) { return false; }
    const auto generic = provider.ResolveExact(genericKey, &failureKey);
    if (!generic) { return false; }
    const std::array<RtPipelineVariantArtifact, 2u> records{*opaque, *generic};
    return ValidateRtPipelineBundlePreflight(request, records, preflight, failureKey);
}

static_assert(sizeof(RtDielectricDiagnostics) == 176u);
static_assert(alignof(RtDielectricDiagnostics) == 16u);
static_assert(offsetof(RtDielectricDiagnostics, primaryRewardBodyPixelCount) +
                  sizeof(std::uint32_t) ==
              kRtDielectricDiagnosticsSchema1FieldCount * sizeof(std::uint32_t));

} // namespace horde::vulkan::raytracing
