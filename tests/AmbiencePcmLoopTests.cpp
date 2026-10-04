#include "audio/AmbiencePcmLoop.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace
{
bool passed = true;
void Check(bool condition, const char* message)
{
    if (!condition) { passed = false; std::cerr << "Ambience PCM: " << message << '\n'; }
}
std::filesystem::path FindRoot()
{
    auto root = std::filesystem::current_path();
    for (int i = 0; i < 8; ++i)
    {
        if (std::filesystem::exists(root / "assets" / horde::audio::kWaterfallCoreAsset)) return root;
        if (!root.has_parent_path()) break;
        root = root.parent_path();
    }
    return {};
}
}
int main()
{
    using namespace horde::audio;
    using pocket_audio::PcmStatus;
    const auto root = FindRoot();
    if (root.empty()) { std::cerr << "Core waterfall candidate missing\n"; return 1; }
    std::ifstream file(root / "assets" / kWaterfallCoreAsset, std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)), {});
    std::vector<std::int16_t> reference;
    Check(pocket_audio::DecodePcmWave(bytes, kWaterfallCoreFrames, reference) == pocket_audio::PcmWaveStatus::Ok,
          "candidate strictly admits through unchanged canonical decoder");
    if (!passed) return 1;
    AmbiencePcmLoop loop;
    Check(loop.Load(bytes) == pocket_audio::PcmWaveStatus::Ok && loop.IsValid() &&
          loop.DecodedBytes() == 2'208'000u, "single bounded decoded body, no second permanent PCM copy");
    std::array<float, kAmbienceChunkFrames * 2u> output{};
    Check(loop.Render(output) == PcmStatus::Suspended && loop.GeneratedFrames() == 0u &&
          std::all_of(output.begin(), output.end(), [](float x) { return x == 0.0f; }),
          "startup suspended: no generated samples or false content clock");
    Check(loop.SetSuspended(false) == PcmStatus::Ok, "active source starts Core cursor");
    constexpr std::array<std::size_t, 5> chunks{{479u, 1u, 317u, 480u, 233u}};
    const std::uint64_t total = static_cast<std::uint64_t>(kWaterfallCoreFrames) * 10u + 19u;
    std::uint64_t generated = 0u;
    bool exact = true;
    for (std::size_t call = 0; generated < total; ++call)
    {
        const auto frames = static_cast<std::size_t>(std::min<std::uint64_t>(chunks[call % chunks.size()], total - generated));
        const auto block = std::span(output).first(frames * 2u);
        if (generated > 12345u && generated < 12345u + kAmbienceChunkFrames)
        {
            Check(loop.SetSuspended(true) == PcmStatus::Suspended && loop.Render(block) == PcmStatus::Suspended &&
                  loop.GeneratedFrames() == generated && std::all_of(block.begin(), block.end(), [](float x) { return x == 0; }),
                  "pause contributes no frames and cannot skip/restart pending loop content");
            Check(loop.SetSuspended(false) == PcmStatus::Ok, "resume restores the same Core cursor");
        }
        Check(loop.Render(block) == PcmStatus::Ok, "bounded irregular Core block renders");
        for (std::size_t frame = 0; frame < frames; ++frame)
        {
            const auto sourceFrame = (generated + frame) % kWaterfallCoreFrames;
            for (std::size_t channel = 0; channel < 2u; ++channel)
                exact = exact && block[frame * 2u + channel] == reference[sourceFrame * 2u + channel] / 32768.0f;
        }
        generated += frames;
    }
    Check(exact && loop.GeneratedFrames() == total,
          "ten complete actual Core wraps match decoded file sample-for-sample through every seam with no padding/skip/restart");
    Check(loop.Reset(false) == PcmStatus::Ok && loop.GeneratedFrames() == 0u &&
          loop.Render(std::span(output).first(2u)) == PcmStatus::Ok && output[0] == reference[0] / 32768.0f,
          "new lifecycle epoch starts exact frame zero and discards old cursor state");
    const auto before = loop.GeneratedFrames();
    Check(loop.Render(std::span(output).first(3u)) == PcmStatus::InvalidOutput && loop.GeneratedFrames() == before,
          "invalid output never advances content ownership");
    bytes[0] = 0;
    Check(loop.Load(bytes) == pocket_audio::PcmWaveStatus::InvalidRiffHeader && !loop.IsValid() && loop.DecodedBytes() == 0u,
          "malformed replacement cannot retain a prior admitted source");
    return passed ? 0 : 1;
}
