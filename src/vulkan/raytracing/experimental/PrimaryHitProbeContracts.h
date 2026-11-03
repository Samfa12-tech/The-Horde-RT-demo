#pragma once

// Bounded evidence for one failed execution-organisation experiment. No Vulkan
// ownership, normal renderer ABI, telemetry schema or performance policy here.
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <sstream>
#include <string>

namespace horde::vulkan::raytracing::experimental {
inline constexpr std::uint32_t kPrimaryProbePrefixBytes = 176u;
inline constexpr std::uint32_t kPrimaryProbeRecordWords = 96u;
inline constexpr std::uint32_t kPrimaryProbeRecordCount = 75u;
inline constexpr std::uint32_t kPrimaryProbePhases = 2u;
inline constexpr std::uint32_t kPrimaryProbeWordCount =
    kPrimaryProbeRecordWords * kPrimaryProbeRecordCount * kPrimaryProbePhases;
inline constexpr std::uint32_t kPrimaryProbeBufferBytes =
    kPrimaryProbePrefixBytes + 4u * kPrimaryProbeWordCount;
inline constexpr std::uint32_t kPrimaryProbeMagic = 0x48505231u;
inline constexpr std::array<std::array<std::uint32_t, 2u>, 3u> kPrimaryProbeCentres{{
    {875u, 1857u}, {216u, 1955u}, {194u, 1085u}}};
static_assert(kPrimaryProbeBufferBytes == 57776u);

inline bool ValidatePrimaryHitProbe(
    std::span<const std::uint32_t> words, bool staged,
    std::span<const std::uint32_t, 32u> cpuPushWords, std::string& error)
{
    if (words.size() != kPrimaryProbeWordCount) {
        error = "wrong_payload_size"; return false;
    }
    for (std::uint32_t phase = 0u; phase < kPrimaryProbePhases; ++phase) {
        for (std::uint32_t slot = 0u; slot < kPrimaryProbeRecordCount; ++slot) {
            const auto r = words.subspan((phase * kPrimaryProbeRecordCount + slot) *
                                        kPrimaryProbeRecordWords, kPrimaryProbeRecordWords);
            if (phase == 1u && !staged) {
                for (auto value : r) if (value != 0u) {
                    error = "unexpected_normal_second_phase"; return false;
                }
                continue;
            }
            const auto centre = kPrimaryProbeCentres[slot / 25u];
            const auto local = slot % 25u;
            if (r[0] != kPrimaryProbeMagic || r[1] != phase ||
                r[2] != centre[0] - 2u + local % 5u ||
                r[3] != centre[1] - 2u + local / 5u ||
                r[4] != 1080u || r[5] != 2235u || r[7] > 1u) {
                error = "missing_or_wrong_pixel_phase_extent"; return false;
            }
            for (std::uint32_t i = 0u; i < 32u; ++i) if (r[48u + i] != cpuPushWords[i]) {
                error = "gpu_cpu_push_identity_mismatch"; return false;
            }
            if (r[11] != 0u || r[15] != 0u) {
                error = "reserved_word_not_zero"; return false;
            }
            for (std::uint32_t i = 84u; i < kPrimaryProbeRecordWords; ++i) if (r[i] != 0u) {
                error = "reserved_word_not_zero"; return false;
            }
        }
    }
    error.clear(); return true;
}

inline std::string PrimaryHitProbeJson(
    std::span<const std::uint32_t> words, bool staged,
    std::span<const std::uint32_t, 32u> cpuPushWords,
    std::span<const std::uint32_t> cpuSceneWords = {})
{
    std::string error;
    const bool valid = ValidatePrimaryHitProbe(words, staged, cpuPushWords, error);
    std::ostringstream json;
    json << "{\"schema\":1,\"classification\":\"diagnostic-only-primary-hit-isolation\","
         << "\"valid\":" << (valid ? "true" : "false")
         << ",\"error\":\"" << error << "\",\"staged\":" << (staged ? "true" : "false")
         << ",\"prefixBytes\":" << kPrimaryProbePrefixBytes
         << ",\"bufferBytes\":" << kPrimaryProbeBufferBytes
         << ",\"recordWords\":" << kPrimaryProbeRecordWords
         << ",\"recordCount\":" << kPrimaryProbeRecordCount
         << ",\"cpuPushWords\":[";
    for (std::size_t i = 0u; i < cpuPushWords.size(); ++i) {
        if (i != 0u) json << ',';
        json << cpuPushWords[i];
    }
    json << "],\"cpuSceneWords\":[";
    for (std::size_t i = 0u; i < cpuSceneWords.size(); ++i) {
        if (i != 0u) json << ',';
        json << cpuSceneWords[i];
    }
    json << "],\"words\":[";
    // Preserve invalid evidence too; never turn a missing sample into a pass.
    for (std::size_t i = 0u; i < words.size(); ++i) {
        if (i != 0u) json << ',';
        json << words[i];
    }
    json << "]}"; return json.str();
}
} // namespace horde::vulkan::raytracing::experimental
