// Offline candidate-PCM check through the production Horde adapter and pinned
// Pocket Audio Core. No OS output, renderer, synthesis, or asset writes.
#include "audio/MusicPcmStream.h"
#include "audio/MusicPcmWave.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using namespace horde::audio;
constexpr std::uint64_t bodyFrames = kMusicPcmLoopFrames;
constexpr std::uint64_t tailFrames = kMusicPcmTailFrames;
constexpr std::uint64_t fadeFrames = kMusicPcmCrossfadeFrames;
constexpr double errorTolerance = 4.0 * std::numeric_limits<float>::epsilon();
constexpr std::array<std::size_t, 5> chunks{1u, 257u, 480u, 1023u, 2048u};

void Require(const bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

char CueName(const MusicCue cue)
{
    return static_cast<char>('A' + static_cast<std::size_t>(cue) - 1u);
}

struct Bank
{
    std::array<std::vector<std::int16_t>, kMusicPcmCueCount> bodies, tails;
    std::array<MusicPcmClip, kMusicPcmCueCount> clips{};
    std::uint64_t bytes = 0u;

    void Load(const std::filesystem::path& repo, const std::filesystem::path& auditions,
              const std::string& variant)
    {
        for (std::size_t index = 1u; index < clips.size(); ++index)
        {
            const auto& asset = kMusicPcmAssets[index];
            auto body = repo / "assets" / asset.bodyPath;
            auto tail = repo / "assets" / asset.tailPath;
            if (variant != "reference" && variant != "bank" &&
                (asset.cue == MusicCue::A || asset.cue == MusicCue::E))
            {
                const std::string cue = asset.cue == MusicCue::A ? "A" : "E";
                body = auditions / "audio" / variant / (cue + "-loop.wav");
                tail = auditions / "audio" / variant / (cue + "-tail.wav");
            }
            Read(body, asset.bodyFrames, bodies[index]);
            Read(tail, static_cast<std::uint32_t>(tailFrames), tails[index]);
            clips[index] = {bodies[index], tails[index]};
            bytes += (bodies[index].size() + tails[index].size()) * sizeof(std::int16_t);
        }
        Require(bytes == 20'160'000u, "candidate bank changes the decoded PCM payload");
    }

    static void Read(const std::filesystem::path& path, const std::uint32_t frames,
                     std::vector<std::int16_t>& samples)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        Require(static_cast<bool>(file), "cannot read " + path.string());
        const auto length = file.tellg();
        Require(length == static_cast<std::streamoff>(44u + frames * 4u), "wrong exact WAV size");
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        Require(static_cast<bool>(file), "short WAV read");
        Require(DecodeMusicPcmWave(bytes, frames, samples) == MusicPcmWaveStatus::Ok,
                "production Core WAV decoder rejected candidate");
    }
};

// PCM16's native full-scale denominator is 32768, unlike the earlier preview
// arithmetic's conservative 32767. Compare to the native contract, not a WAV
// rerender or a relaxed signal tolerance.
double LoopSample(const Bank& bank, const MusicCue cue, const std::uint64_t absoluteFrame,
                  const std::size_t channel)
{
    const auto index = static_cast<std::size_t>(cue);
    if (!kMusicPcmAssets[index].looping)
    {
        const auto length = bank.bodies[index].size() / 2u;
        if (absoluteFrame < length)
            return bank.bodies[index][absoluteFrame * 2u + channel] / 32768.0;
        const auto tail = absoluteFrame - length;
        return tail < tailFrames ? bank.tails[index][tail * 2u + channel] / 32768.0 : 0.0;
    }
    const auto frame = absoluteFrame % bodyFrames;
    auto integer = static_cast<std::int32_t>(bank.bodies[index][frame * 2u + channel]);
    if (absoluteFrame >= bodyFrames && frame < tailFrames)
        integer += bank.tails[index][frame * 2u + channel];
    return static_cast<double>(integer) / 32768.0;
}

struct Metrics
{
    double peak = 0.0, maximumError = 0.0, boundaryDelta = 0.0;
    std::array<double, 2> previous{};

    void Check(const float actual, const double expected, const std::uint64_t frame,
               const std::size_t channel)
    {
        Require(std::isfinite(actual), "non-finite Core output");
        peak = std::max(peak, std::abs(static_cast<double>(actual)));
        maximumError = std::max(maximumError, std::abs(actual - expected));
        Require(std::abs(actual) < 1.0f, "Core output clips; no limiter allowed");
        Require(std::abs(actual - expected) <= errorTolerance, "Core output differs from sample oracle");
        if (frame != 0u && frame % bodyFrames == 0u)
            boundaryDelta = std::max(boundaryDelta, std::abs(actual - previous[channel]));
        previous[channel] = actual;
    }
};

template<class CheckSample>
void RenderFrames(MusicPcmStream& stream, const std::uint64_t frames, CheckSample check)
{
    std::array<float, 4096> buffer{};
    std::uint64_t completed = 0u;
    std::size_t chunk = 0u;
    while (completed < frames)
    {
        const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(
            chunks[chunk++ % chunks.size()], frames - completed));
        auto output = std::span(buffer).first(count * 2u);
        Require(stream.Render(output) == MusicPcmStatus::Ok, "Core render failed");
        for (std::size_t frame = 0u; frame < count; ++frame)
            for (std::size_t channel = 0u; channel < 2u; ++channel)
                check(output[frame * 2u + channel], completed + frame, channel);
        completed += count;
    }
}

Metrics CheckLoops(const Bank& bank, const MusicCue cue)
{
    MusicPcmStream stream(bank.clips);
    Require(stream.IsValid(), "candidate bank/adapter invalid");
    MusicSelection selection{.cue = cue, .looping = true, .revision = 1u};
    Require(stream.SetSelection(selection) == MusicPcmStatus::Ok, "loop selection failed");
    Metrics metrics;
    RenderFrames(stream, 20u * bodyFrames, [&](const float sample, const auto frame, const auto channel)
    { metrics.Check(sample, LoopSample(bank, cue, frame, channel), frame, channel); });

    // A real Core suspension must emit zero without moving the sample cursor.
    selection.suspended = true;
    Require(stream.SetSelection(selection) == MusicPcmStatus::Suspended, "suspend failed");
    std::array<float, 960> silence{};
    silence.fill(1.0f);
    Require(stream.Render(silence) == MusicPcmStatus::Suspended, "suspended render status");
    Require(std::all_of(silence.begin(), silence.end(), [](const float value) { return value == 0.0f; }),
            "suspended output is not silent");
    selection.suspended = false;
    Require(stream.SetSelection(selection) == MusicPcmStatus::Ok, "resume failed");
    Require(stream.SetVolumePercent(70.0f), "default music gain rejected");
    Metrics resumed;
    RenderFrames(stream, 480u, [&](const float sample, const auto frame, const auto channel)
    { resumed.Check(sample, LoopSample(bank, cue, 20u * bodyFrames + frame, channel) * 0.70,
                    frame, channel); });
    metrics.maximumError = std::max(metrics.maximumError, resumed.maximumError);
    return metrics;
}

Metrics CheckTransition(const Bank& bank, const MusicCue from, const MusicCue to,
                        const std::uint64_t exitFrame)
{
    MusicPcmStream stream(bank.clips);
    Require(stream.SetSelection({.cue = from,
        .looping = kMusicPcmAssets[static_cast<std::size_t>(from)].looping,
        .revision = 1u}) == MusicPcmStatus::Ok,
            "transition source selection failed");
    RenderFrames(stream, exitFrame, [](const float, const auto, const auto) {});
    Require(stream.SetSelection({.cue = to,
        .looping = kMusicPcmAssets[static_cast<std::size_t>(to)].looping,
        .revision = 2u}) == MusicPcmStatus::Ok,
            "transition target selection failed");
    Metrics metrics;
    // Includes one sample beyond the complete fade, not just its initial frame.
    RenderFrames(stream, fadeFrames + 1u, [&](const float sample, const auto frame, const auto channel)
    {
        const double incomingGain = static_cast<double>(std::min(frame, fadeFrames)) / fadeFrames;
        const double expected = LoopSample(bank, from, exitFrame + frame, channel) * (1.0 - incomingGain) +
                                LoopSample(bank, to, frame, channel) * incomingGain;
        metrics.Check(sample, expected, frame, channel);
    });
    return metrics;
}

Metrics CheckOneShot(const Bank& bank, const MusicCue cue)
{
    MusicPcmStream stream(bank.clips);
    Require(stream.SetSelection({.cue = cue, .looping = false, .revision = 1u}) == MusicPcmStatus::Ok,
            "one-shot selection failed");
    Metrics metrics;
    const auto length = kMusicPcmAssets[static_cast<std::size_t>(cue)].bodyFrames;
    RenderFrames(stream, length + tailFrames + 480u,
        [&](const float sample, const auto frame, const auto channel)
        { metrics.Check(sample, LoopSample(bank, cue, frame, channel), frame, channel); });
    return metrics;
}

Metrics CheckNaturalHandoff(const Bank& bank, const MusicCue from, const MusicCue to,
                           const std::uint64_t tailOffset)
{
    MusicPcmStream stream(bank.clips);
    Require(stream.SetSelection({.cue = from, .looping = false, .revision = 1u}) == MusicPcmStatus::Ok,
            "natural handoff source failed");
    const auto index = static_cast<std::size_t>(from);
    RenderFrames(stream, kMusicPcmAssets[index].bodyFrames + tailOffset,
                 [](const float, const auto, const auto) {});
    Require(stream.SetSelection({.cue = to, .looping = true, .revision = 2u}) == MusicPcmStatus::Ok,
            "natural handoff target failed");
    Metrics metrics;
    RenderFrames(stream, tailFrames + 1u, [&](const float sample, const auto frame, const auto channel)
    {
        const auto outgoingFrame = frame + tailOffset;
        const double tail = outgoingFrame < tailFrames
            ? bank.tails[index][outgoingFrame * 2u + channel] / 32768.0 : 0.0;
        metrics.Check(sample, LoopSample(bank, to, frame, channel) + tail, frame, channel);
    });
    return metrics;
}

void CheckRemainingBank(const Bank& bank)
{
    // A/E already have exact candidate native checks. Check only newly rendered
    // cues here; do not restart the completed auditions or listen via an OS sink.
    std::cout << "{\"variant\":\"bank\",\"pcmBytes\":" << bank.bytes << ",\"loops\":[";
    bool first = true;
    for (const auto cue : {MusicCue::B, MusicCue::D, MusicCue::F, MusicCue::H})
    {
        const auto metrics = CheckLoops(bank, cue);
        if (!first) std::cout << ',';
        first = false;
        std::cout << "{\"cue\":\"" << CueName(cue)
                  << "\",\"periods\":20,\"frames\":" << 20u * bodyFrames
                  << ",\"peak\":" << metrics.peak << ",\"boundaryDelta\":" << metrics.boundaryDelta
                  << ",\"maximumError\":" << metrics.maximumError << '}';
    }
    std::cout << "],\"oneShots\":[";
    first = true;
    for (const auto cue : {MusicCue::C, MusicCue::G})
    {
        const auto metrics = CheckOneShot(bank, cue);
        if (!first) std::cout << ',';
        first = false;
        std::cout << "{\"cue\":\"" << CueName(cue)
                  << "\",\"peak\":" << metrics.peak << ",\"maximumError\":" << metrics.maximumError << '}';
    }
    std::cout << "],\"handoffs\":[";
    first = true;
    for (const auto from : {MusicCue::C, MusicCue::G})
        for (const auto offset : {0u, 24'000u, 144'000u})
        {
            const auto metrics = CheckNaturalHandoff(bank, from,
                from == MusicCue::C ? MusicCue::D : MusicCue::H, offset);
            if (!first) std::cout << ',';
            first = false;
            std::cout << "{\"from\":\"" << CueName(from)
                      << "\",\"tailOffset\":" << offset << ",\"checkedFrames\":" << tailFrames + 1u
                      << ",\"peak\":" << metrics.peak << ",\"maximumError\":" << metrics.maximumError << '}';
        }
    std::cout << "],\"earlyCrossfades\":[";
    first = true;
    for (const auto from : {MusicCue::B, MusicCue::F, MusicCue::C, MusicCue::G})
    {
        const auto to = from == MusicCue::C ? MusicCue::D : from == MusicCue::G ? MusicCue::H : MusicCue::A;
        const auto metrics = CheckTransition(bank, from, to, 48'000u);
        if (!first) std::cout << ',';
        first = false;
        std::cout << "{\"from\":\"" << CueName(from)
                  << "\",\"checkedFrames\":" << fadeFrames + 1u
                  << ",\"peak\":" << metrics.peak << ",\"maximumError\":" << metrics.maximumError << '}';
    }
    std::cout << "],\"status\":\"PASS-native-offline; listening-and-device-acceptance-open\"}\n";
}
} // namespace

int main(const int argc, char** argv)
{
    try
    {
        Require(argc == 4, "usage: check <repository> <audition-evidence-directory> <variant>");
        const std::string variant = argv[3];
        Require(variant == "reference" || variant == "whistle" || variant == "reed" || variant == "owner-combo" || variant == "bank",
                "unknown variant");
        Bank bank;
        bank.Load(argv[1], argv[2], variant);
        if (variant == "bank")
        {
            std::cout << std::setprecision(9);
            CheckRemainingBank(bank);
            return 0;
        }
        std::cout << std::setprecision(9) << "{\"variant\":\"" << variant
                  << "\",\"pcmBytes\":" << bank.bytes << ",\"loops\":[";
        bool first = true;
        for (const auto cue : {MusicCue::A, MusicCue::E})
        {
            const auto metrics = CheckLoops(bank, cue);
            if (!first) std::cout << ',';
            first = false;
            std::cout << "{\"cue\":\"" << (cue == MusicCue::A ? 'A' : 'E')
                      << "\",\"periods\":20,\"frames\":" << 20u * bodyFrames
                      << ",\"peak\":" << metrics.peak << ",\"boundaryDelta\":" << metrics.boundaryDelta
                      << ",\"maximumError\":" << metrics.maximumError << ",\"pauseResumeAnd70Percent\":true}";
        }
        std::cout << "],\"transitions\":[";
        first = true;
        for (const auto from : {MusicCue::A, MusicCue::E})
            for (const std::uint64_t cycle : {0u, 1u})
                for (const std::uint64_t phase : {48'000u, 288'000u, 575'999u})
                {
                    const auto exitFrame = cycle * bodyFrames + phase;
                    const auto metrics = CheckTransition(bank, from, from == MusicCue::A ? MusicCue::E : MusicCue::A, exitFrame);
                    if (!first) std::cout << ',';
                    first = false;
                    std::cout << "{\"from\":\"" << (from == MusicCue::A ? 'A' : 'E')
                              << "\",\"exitFrame\":" << exitFrame << ",\"checkedFrames\":" << fadeFrames + 1u
                              << ",\"peak\":" << metrics.peak << ",\"maximumError\":" << metrics.maximumError << '}';
                }
        std::cout << "],\"status\":\"PASS-native-offline; listening-and-device-acceptance-open\"}\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Candidate Core check failed: " << error.what() << '\n';
        return 1;
    }
}
