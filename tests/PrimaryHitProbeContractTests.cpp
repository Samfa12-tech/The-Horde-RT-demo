#include "vulkan/raytracing/experimental/PrimaryHitProbeContracts.h"
#include <algorithm>
#include <iostream>
#include <vector>

using namespace horde::vulkan::raytracing::experimental;
int main()
{
    const auto fail = [](const char* label) { std::cerr << label << '\n'; return 1; };
    std::array<std::uint32_t, 32u> cpu{};
    cpu[0] = 0x80000000u; cpu[31] = 0x3f800000u;
    std::vector<std::uint32_t> words(kPrimaryProbeWordCount);
    const auto populate = [&](bool staged) {
        std::fill(words.begin(), words.end(), 0u);
        for (std::uint32_t phase = 0u; phase < (staged ? 2u : 1u); ++phase)
            for (std::uint32_t slot = 0u; slot < kPrimaryProbeRecordCount; ++slot) {
                auto* r = words.data() + (phase * kPrimaryProbeRecordCount + slot) * kPrimaryProbeRecordWords;
                const auto centre = kPrimaryProbeCentres[slot / 25u];
                r[0] = kPrimaryProbeMagic; r[1] = phase;
                r[2] = centre[0] - 2u + slot % 25u % 5u;
                r[3] = centre[1] - 2u + slot % 25u / 5u;
                r[4] = 1080u; r[5] = 2235u;
                std::copy(cpu.begin(), cpu.end(), r + 48u);
            }
    };
    std::string error;
    if (ValidatePrimaryHitProbe(words, false, cpu, error) || error.empty()) return fail("absent sample admitted");
    for (const bool staged : {false, true}) {
        populate(staged);
        if (!ValidatePrimaryHitProbe(words, staged, cpu, error)) return fail("valid phases rejected");
        for (const auto index : {0u, 1u, 2u, 3u, 4u, 5u, 7u, 11u, 15u, 48u, 79u, 84u, 95u}) {
            const auto saved = words[index]; words[index] = ~saved;
            if (ValidatePrimaryHitProbe(words, staged, cpu, error)) return fail("corrupt identity/reserved word admitted");
            words[index] = saved;
        }
        if (ValidatePrimaryHitProbe(std::span(words).first(words.size() - 1u), staged, cpu, error))
            return fail("truncated record admitted");
        const auto json = PrimaryHitProbeJson(words, staged, cpu);
        if (json.find("\"valid\":true") == std::string::npos ||
            json.find("\"bufferBytes\":57776") == std::string::npos)
            return fail("valid serialization identity");
    }
    populate(false);
    if (ValidatePrimaryHitProbe(words, true, cpu, error)) return fail("missing staged reload admitted");
    words[kPrimaryProbeRecordCount * kPrimaryProbeRecordWords] = kPrimaryProbeMagic;
    if (ValidatePrimaryHitProbe(words, false, cpu, error)) return fail("unexpected normal reload admitted");
    if (PrimaryHitProbeJson(words, false, cpu).find("\"valid\":false") == std::string::npos)
        return fail("invalid evidence concealed");
    std::cout << "PASS bounded primary-hit probe identity/phase/extent/push/serialization contracts; not GPU acceptance\n";
    return 0;
}
