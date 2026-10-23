#include "audio/MusicPcmAssetBank.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <new>
#include <stdexcept>
#include <type_traits>

namespace
{

using namespace horde::audio;
bool passed = true;

void Check(const bool condition, const char* message)
{
    if (!condition)
    {
        passed = false;
        std::cerr << "Music PCM bank test failed: " << message << '\n';
    }
}

enum class Fault { None, Missing, Truncated, Oversized, WrongFormat, ThrowRead, ThrowAllocation };
struct Reader
{
    std::filesystem::path root{HORDE_RT_MUSIC_ASSET_DIR};
    std::string_view failurePath = kMusicPcmAssets[7].tailPath;
    Fault fault = Fault::None;
    std::size_t calls = 0u;
    std::size_t largestReadBytes = 0u;
    std::size_t largestInputCapacity = 0u;
};

bool ReadAsset(void* context, const std::string_view path, const std::size_t maximumBytes,
               std::vector<std::uint8_t>& bytes)
{
    auto& reader = *static_cast<Reader*>(context);
    ++reader.calls;
    bytes.clear();
    if (path == reader.failurePath)
    {
        if (reader.fault == Fault::Missing) return false;
        if (reader.fault == Fault::ThrowRead) throw std::runtime_error("injected reader failure");
        if (reader.fault == Fault::ThrowAllocation) throw std::bad_alloc{};
    }
    std::ifstream file(reader.root / path, std::ios::binary | std::ios::ate);
    if (!file) return false;
    const auto length = file.tellg();
    if (length < 0 || static_cast<std::uint64_t>(length) > maximumBytes) return false;
    bytes.resize(static_cast<std::size_t>(length));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!file) return false;
    reader.largestReadBytes = std::max(reader.largestReadBytes, bytes.size());
    reader.largestInputCapacity = std::max(reader.largestInputCapacity, bytes.capacity());
    if (path == reader.failurePath)
    {
        if (reader.fault == Fault::Truncated) bytes.pop_back();
        // Deliberately violate the reader's declared bound; bank must reject too.
        if (reader.fault == Fault::Oversized) bytes.push_back(0u);
        if (reader.fault == Fault::WrongFormat) bytes[22] = 1u; // mono instead of stereo
    }
    return true;
}

void CheckEmpty(const MusicPcmAssetBank& bank)
{
    Check(!bank.IsReady() && bank.PcmBytes() == 0u, "failed bank retains no ready state or partial PCM");
    for (const auto& clip : bank.Clips())
    {
        Check(clip.body.empty() && clip.tail.empty(), "failed bank exports no borrowed spans");
    }
}

void TestAdmissionAndBorrowedLifetime()
{
    MusicPcmAssetBank bank;
    CheckEmpty(bank);
    Check(bank.Load(nullptr, nullptr).status == MusicPcmBankStatus::InvalidReader,
          "null asset reader fails explicitly");
    CheckEmpty(bank);
    Reader reader;
    const auto loadStart = std::chrono::steady_clock::now();
    const auto result = bank.Load(ReadAsset, &reader);
    const auto loadMilliseconds = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - loadStart).count();
    Check(result.status == MusicPcmBankStatus::Loaded && result.assetPath.empty(),
          "actual seventeen admitted WAVs decode into ready bank");
    Check(reader.calls == 17u, "exactly seventeen bounded reads, no source/manifest/quiet playback reads");
    Check(bank.PcmBytes() == 40'640'000u, "exact mixed-bank immutable PCM payload budget");
    const auto clips = bank.Clips();
    Check(clips[0].body.empty() && clips[0].tail.empty(), "None owns no PCM");
    for (std::size_t index = 1u; index < clips.size(); ++index)
    {
        Check(clips[index].body.size() == kMusicPcmAssets[index].bodyFrames * 2u &&
              clips[index].tail.size() == kMusicPcmAssets[index].tailFrames * 2u,
              "every published span has exact admitted interleaved frame count");
    }
    std::size_t legacyPcmBytes = 0u;
    for (std::size_t index = 1u; index <= static_cast<std::size_t>(MusicCue::H); ++index)
        legacyPcmBytes += (clips[index].body.size() + clips[index].tail.size()) * sizeof(std::int16_t);
    Check(legacyPcmBytes == 20'160'000u, "all sixteen legacy body/tail payloads retain their independent budget");
    const auto& outdoor = clips[static_cast<std::size_t>(MusicCue::Outdoor)];
    Check(outdoor.body.size() == 10'240'000u && outdoor.tail.empty(),
          "outdoor owns the full approved main and no duplicated quiet mix or extra tail");
    Check(reader.largestReadBytes == 20'480'044u &&
          reader.largestInputCapacity >= reader.largestReadBytes,
          "actual full WAV uses its explicit encoded bound and recorded input capacity");
    std::cout << "Music bank load: " << loadMilliseconds << " ms; PCM bytes=" << bank.PcmBytes()
              << "; largest encoded read=" << reader.largestReadBytes
              << "; observed input capacity=" << reader.largestInputCapacity
              << "; logical/capacity payload, not process RSS or phone timing\n";
    MusicPcmStream stream(clips);
    Check(stream.IsValid(), "Core stream borrows completed bank spans");
    Check(bank.Load(nullptr, nullptr).status == MusicPcmBankStatus::AlreadyLoaded,
          "even an invalid reload cannot evict a live bank");
    Check(bank.Load(ReadAsset, &reader).status == MusicPcmBankStatus::AlreadyLoaded && reader.calls == 17u,
          "successful bank cannot reload or perform further file I/O");
    Check(bank.Clips()[1].body.data() == clips[1].body.data(), "borrowed PCM addresses stay stable");
    Check(stream.SetSelection({.cue = MusicCue::A, .looping = true, .revision = 1u}) == MusicPcmStatus::Ok,
          "admitted bank feeds actual Core playback cursor");
    std::array<float, 2048u> output{};
    float peak = 0.0f;
    for (std::size_t chunk = 0u; chunk < 48u; ++chunk)
    {
        Check(stream.Render(output) == MusicPcmStatus::Ok, "actual PCM renders through Core");
        for (const float sample : output)
        {
            Check(std::isfinite(sample), "native bank output samples are finite");
            peak = std::max(peak, std::abs(sample));
        }
    }
    Check(peak > 0.0f && peak <= 1.0f, "actual first second produces nonzero, bounded PCM");
    Check(reader.calls == 17u && bank.PcmBytes() == 40'640'000u,
          "rendering neither reads nor evicts PCM assets");

    // Exercise the actual admitted waveform through a whole long period, not
    // a generated marker clip. Every returned sample must match its source.
    MusicPcmStream actualOutdoor(clips);
    Check(actualOutdoor.SetSelection({.cue = MusicCue::Outdoor, .looping = true,
          .revision = 1u}) == MusicPcmStatus::Ok, "actual outdoor selection is admitted");
    const std::array<std::size_t, 5> chunks{37u, 1'024u, 4'093u, 257u, 2'011u};
    std::array<float, 8'192u> longOutput{};
    constexpr std::uint64_t outdoorFrames = 5'120'000u;
    constexpr std::uint64_t countFrames = outdoorFrames + 31u;
    std::uint64_t rendered = 0u;
    std::size_t chunk = 0u;
    bool exactSamples = true, validOutput = true;
    while (rendered < countFrames) {
        const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(
            chunks[chunk++ % chunks.size()], countFrames - rendered));
        const auto view = std::span<float>(longOutput).first(count * 2u);
        validOutput = validOutput && actualOutdoor.Render(view) == MusicPcmStatus::Ok;
        for (std::size_t frame = 0u; frame < count; ++frame) {
            const auto source = static_cast<std::size_t>((rendered + frame) % outdoorFrames) * 2u;
            exactSamples = exactSamples &&
                view[frame * 2u] == static_cast<float>(outdoor.body[source]) / 32768.0f &&
                view[frame * 2u + 1u] == static_cast<float>(outdoor.body[source + 1u]) / 32768.0f;
        }
        rendered += count;
    }
    Check(validOutput && exactSamples, "actual outdoor plays every frame and wraps sample-exactly after 5120000 frames");
    MusicPcmStream seek(clips);
    Check(seek.SetSelection({.cue = MusicCue::Outdoor, .looping = true,
          .positionSeconds = static_cast<double>(outdoorFrames - 1u) / 48'000.0,
          .revision = 1u}) == MusicPcmStatus::Ok, "actual outdoor can seek to its final frame");
    std::array<float, 4u> seam{};
    Check(seek.Render(seam) == MusicPcmStatus::Ok &&
          seam[0] == static_cast<float>(outdoor.body[outdoor.body.size() - 2u]) / 32768.0f &&
          seam[1] == static_cast<float>(outdoor.body.back()) / 32768.0f &&
          seam[2] == static_cast<float>(outdoor.body[0]) / 32768.0f &&
          seam[3] == static_cast<float>(outdoor.body[1]) / 32768.0f,
          "actual final stereo frame joins exact first frame without truncation, padding or a duplicated tail");
    Check(reader.calls == 17u && bank.PcmBytes() == 40'640'000u,
          "long render and seek do not perform disk reads or change PCM ownership");
}

void TestTransactionalFailuresAndRetry()
{
    for (const Fault fault : {Fault::Missing, Fault::Truncated, Fault::Oversized,
                             Fault::WrongFormat, Fault::ThrowRead, Fault::ThrowAllocation})
    {
        MusicPcmAssetBank bank;
        Reader reader;
        reader.fault = fault;
        const auto result = bank.Load(ReadAsset, &reader);
        const auto expected = fault == Fault::Truncated || fault == Fault::Oversized
            ? MusicPcmBankStatus::WrongFileSize : fault == Fault::WrongFormat
            ? MusicPcmBankStatus::DecodeFailed : fault == Fault::ThrowAllocation
            ? MusicPcmBankStatus::AllocationFailure : MusicPcmBankStatus::ReadFailed;
        Check(result.status == expected && result.assetPath == reader.failurePath,
              "late failure retains precise cause/static path, not partial admission");
        if (fault == Fault::WrongFormat)
        {
            Check(result.decoderStatus == MusicPcmWaveStatus::WrongChannelCount,
                  "Core decoder failure detail survives bank admission");
        }
        Check(reader.calls == 14u, "late failure follows thirteen successful clips");
        CheckEmpty(bank);
        MusicPcmStream rejected(bank.Clips());
        Check(!rejected.IsValid(), "partial bank cannot become an accepted Core stream");
        reader.fault = Fault::None;
        reader.calls = 0u;
        Check(bank.Load(ReadAsset, &reader).status == MusicPcmBankStatus::Loaded && reader.calls == 17u,
              "failed bank can retry cleanly before playback");
        Check(bank.IsReady() && bank.PcmBytes() == 40'640'000u, "retry admits the exact complete mixed bank");
    }
}

void TestLateOutdoorFailureAndRetry()
{
    for (const Fault fault : {Fault::Missing, Fault::WrongFormat}) {
        MusicPcmAssetBank bank;
        Reader reader;
        reader.failurePath = kMusicPcmAssets[static_cast<std::size_t>(MusicCue::Outdoor)].bodyPath;
        reader.fault = fault;
        const auto result = bank.Load(ReadAsset, &reader);
        Check(result.status == (fault == Fault::Missing ? MusicPcmBankStatus::ReadFailed
                                                      : MusicPcmBankStatus::DecodeFailed) &&
              result.assetPath == reader.failurePath && reader.calls == 17u,
              "outdoor read/decode failure follows all sixteen unchanged legacy clips and retains exact cause");
        CheckEmpty(bank);
        if (fault == Fault::WrongFormat)
            Check(result.decoderStatus == MusicPcmWaveStatus::WrongChannelCount,
                  "outdoor explicit-limits decode retains format rejection");
        reader.fault = Fault::None; reader.calls = 0u;
        Check(bank.Load(ReadAsset, &reader).status == MusicPcmBankStatus::Loaded &&
              reader.calls == 17u && bank.PcmBytes() == 40'640'000u,
              "late outdoor failure can retry atomically before playback without a partial bank");
    }
}

static_assert(!std::is_copy_constructible_v<MusicPcmAssetBank>);
static_assert(!std::is_move_constructible_v<MusicPcmAssetBank>);

} // namespace

int main()
{
    TestAdmissionAndBorrowedLifetime();
    TestTransactionalFailuresAndRetry();
    TestLateOutdoorFailureAndRetry();
    return passed ? 0 : 1;
}
