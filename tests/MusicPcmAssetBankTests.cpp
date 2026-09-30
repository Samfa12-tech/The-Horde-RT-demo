#include "audio/MusicPcmAssetBank.h"

#include <algorithm>
#include <array>
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
    const auto result = bank.Load(ReadAsset, &reader);
    Check(result.status == MusicPcmBankStatus::Loaded && result.assetPath.empty(),
          "actual sixteen admitted WAVs decode into ready bank");
    Check(reader.calls == 16u, "exactly sixteen bounded reads, no source/manifest playback reads");
    Check(bank.PcmBytes() == 20'160'000u, "exact immutable PCM payload budget");
    const auto clips = bank.Clips();
    Check(clips[0].body.empty() && clips[0].tail.empty(), "None owns no PCM");
    for (std::size_t index = 1u; index < clips.size(); ++index)
    {
        Check(clips[index].body.size() == kMusicPcmAssets[index].bodyFrames * 2u &&
              clips[index].tail.size() == kMusicPcmTailFrames * 2u,
              "every published span has exact admitted interleaved frame count");
    }
    MusicPcmStream stream(clips);
    Check(stream.IsValid(), "Core stream borrows completed bank spans");
    Check(bank.Load(nullptr, nullptr).status == MusicPcmBankStatus::AlreadyLoaded,
          "even an invalid reload cannot evict a live bank");
    Check(bank.Load(ReadAsset, &reader).status == MusicPcmBankStatus::AlreadyLoaded && reader.calls == 16u,
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
    Check(reader.calls == 16u && bank.PcmBytes() == 20'160'000u,
          "rendering neither reads nor evicts PCM assets");
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
        Check(bank.Load(ReadAsset, &reader).status == MusicPcmBankStatus::Loaded && reader.calls == 16u,
              "failed bank can retry cleanly before playback");
        Check(bank.IsReady() && bank.PcmBytes() == 20'160'000u, "retry admits the exact complete bank");
    }
}

static_assert(!std::is_copy_constructible_v<MusicPcmAssetBank>);
static_assert(!std::is_move_constructible_v<MusicPcmAssetBank>);

} // namespace

int main()
{
    TestAdmissionAndBorrowedLifetime();
    TestTransactionalFailuresAndRetry();
    return passed ? 0 : 1;
}
