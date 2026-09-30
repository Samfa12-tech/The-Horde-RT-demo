#include "audio/MusicPcmWave.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace
{

using horde::audio::DecodeMusicPcmWave;
using horde::audio::MusicPcmWaveStatus;

bool passed = true;

void Check(const bool condition, const char* message)
{
    if (!condition)
    {
        passed = false;
        std::cerr << "Music PCM WAV test failed: " << message << '\n';
    }
}

void AppendFourCc(std::vector<std::uint8_t>& bytes, const char* value)
{
    for (std::size_t index = 0u; index < 4u; ++index)
    {
        bytes.push_back(static_cast<std::uint8_t>(value[index]));
    }
}

void AppendU16(std::vector<std::uint8_t>& bytes, const std::uint16_t value)
{
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
}

void AppendU32(std::vector<std::uint8_t>& bytes, const std::uint32_t value)
{
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16u) & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24u) & 0xffu));
}

void StoreU32(std::vector<std::uint8_t>& bytes, const std::size_t offset,
              const std::uint32_t value)
{
    bytes[offset] = static_cast<std::uint8_t>(value & 0xffu);
    bytes[offset + 1u] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
    bytes[offset + 2u] = static_cast<std::uint8_t>((value >> 16u) & 0xffu);
    bytes[offset + 3u] = static_cast<std::uint8_t>((value >> 24u) & 0xffu);
}

struct WaveFormat
{
    std::uint16_t tag = 1u;
    std::uint16_t channels = 2u;
    std::uint32_t sampleRate = 48'000u;
    std::uint32_t byteRate = 192'000u;
    std::uint16_t blockAlignment = 4u;
    std::uint16_t bitsPerSample = 16u;
};

std::vector<std::uint8_t> FormatPayload(const WaveFormat format = {})
{
    std::vector<std::uint8_t> payload;
    AppendU16(payload, format.tag);
    AppendU16(payload, format.channels);
    AppendU32(payload, format.sampleRate);
    AppendU32(payload, format.byteRate);
    AppendU16(payload, format.blockAlignment);
    AppendU16(payload, format.bitsPerSample);
    return payload;
}

void AppendChunk(std::vector<std::uint8_t>& bytes, const char* fourCc,
                 const std::span<const std::uint8_t> payload,
                 const bool includeOddPadding = true)
{
    AppendFourCc(bytes, fourCc);
    AppendU32(bytes, static_cast<std::uint32_t>(payload.size()));
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    if ((payload.size() & 1u) != 0u && includeOddPadding)
    {
        bytes.push_back(0u);
    }
}

std::vector<std::uint8_t> MakeWave(
    const std::vector<std::pair<std::string, std::vector<std::uint8_t>>>& chunks)
{
    std::vector<std::uint8_t> bytes;
    AppendFourCc(bytes, "RIFF");
    AppendU32(bytes, 0u);
    AppendFourCc(bytes, "WAVE");
    for (const auto& [fourCc, payload] : chunks)
    {
        AppendChunk(bytes, fourCc.c_str(), payload);
    }
    StoreU32(bytes, 4u, static_cast<std::uint32_t>(bytes.size() - 8u));
    return bytes;
}

std::vector<std::uint8_t> SmallWave(
    const std::vector<std::uint8_t>& samples,
    const WaveFormat format = {})
{
    return MakeWave({{"fmt ", FormatPayload(format)}, {"data", samples}});
}

void ExpectRejected(const MusicPcmWaveStatus expectedStatus,
                    const std::span<const std::uint8_t> bytes,
                    const std::uint32_t expectedFrames,
                    const char* message)
{
    std::vector<std::int16_t> output{12, -34};
    const MusicPcmWaveStatus status = DecodeMusicPcmWave(bytes, expectedFrames, output);
    Check(status == expectedStatus, message);
    Check(output.empty(), "failed decode leaves the caller output empty");
}

void TestValidUnknownChunksAndUnalignedLittleEndianSamples()
{
    const std::vector<std::uint8_t> oddMetadata{0xa5u, 0x5au, 0x01u};
    const std::vector<std::uint8_t> sampleBytes{
        0x00u, 0x80u, 0xffu, 0x7fu, 0x34u, 0x12u, 0xccu, 0xedu};
    auto wave = MakeWave({{"JUNK", oddMetadata}, {"fmt ", FormatPayload()},
                          {"LIST", {0x10u, 0x20u}}, {"data", sampleBytes}});
    std::vector<std::uint8_t> unaligned;
    unaligned.reserve(wave.size() + 1u);
    unaligned.push_back(0xeeu);
    unaligned.insert(unaligned.end(), wave.begin(), wave.end());

    std::vector<std::int16_t> decoded;
    const auto status = DecodeMusicPcmWave(
        std::span<const std::uint8_t>(unaligned.data() + 1u, wave.size()), 2u, decoded);
    Check(status == MusicPcmWaveStatus::Ok, "valid WAV with padded unknown chunks decodes");
    Check(decoded == std::vector<std::int16_t>{-32'768, 32'767, 0x1234, -0x1234},
          "unaligned bytes decode as explicit signed little-endian interleaved PCM16");
}

void TestChunkOrderAndExtendedPcmFormat()
{
    const std::vector<std::uint8_t> sampleBytes{0x01u, 0x00u, 0xfeu, 0xffu};
    const auto format = FormatPayload();

    std::vector<std::int16_t> decoded;
    const auto dataBeforeFormat = MakeWave({{"data", sampleBytes}, {"fmt ", format}});
    Check(DecodeMusicPcmWave(dataBeforeFormat, 1u, decoded) == MusicPcmWaveStatus::Ok,
          "unique data chunk may precede fmt chunk");
    Check(decoded == std::vector<std::int16_t>{1, -2},
          "data-before-format samples decode unchanged");

    auto extendedFormat = format;
    AppendU16(extendedFormat, 2u); // WAVEFORMATEX cbSize
    AppendU16(extendedFormat, 0xbeefu); // harmless codec-specific extension bytes
    const auto extendedWave = MakeWave({{"fmt ", extendedFormat}, {"data", sampleBytes}});
    Check(DecodeMusicPcmWave(extendedWave, 1u, decoded) == MusicPcmWaveStatus::Ok,
          "PCM fmt chunk with harmless declared extension bytes is accepted");
    Check(decoded == std::vector<std::int16_t>{1, -2},
          "extended fmt payload does not change decoded sample bytes");
}

void TestHeaderAndChunkBoundaries()
{
    const auto valid = SmallWave({0x01u, 0x00u, 0x02u, 0x00u});
    ExpectRejected(MusicPcmWaveStatus::InvalidRiffHeader,
                   std::span<const std::uint8_t>(valid.data(), 11u), 1u,
                   "truncated RIFF/WAVE header is rejected");

    auto wrongRiffTag = valid;
    wrongRiffTag[0] = static_cast<std::uint8_t>('X');
    ExpectRejected(MusicPcmWaveStatus::InvalidRiffHeader, wrongRiffTag, 1u,
                   "wrong RIFF FourCC is rejected");

    auto wrongWaveTag = valid;
    wrongWaveTag[8] = static_cast<std::uint8_t>('Z');
    ExpectRejected(MusicPcmWaveStatus::InvalidRiffHeader, wrongWaveTag, 1u,
                   "wrong WAVE FourCC is rejected");

    auto riffLengthBelowFormType = valid;
    StoreU32(riffLengthBelowFormType, 4u, 3u);
    ExpectRejected(MusicPcmWaveStatus::InvalidRiffLength, riffLengthBelowFormType, 1u,
                   "RIFF length smaller than its WAVE form type is rejected");

    auto badRiffLength = valid;
    StoreU32(badRiffLength, 4u,
             static_cast<std::uint32_t>(badRiffLength.size() + 12u));
    ExpectRejected(MusicPcmWaveStatus::InvalidRiffLength, badRiffLength, 1u,
                   "declared RIFF length beyond available bytes is rejected");

    auto trailing = valid;
    trailing.push_back(0u);
    ExpectRejected(MusicPcmWaveStatus::InvalidRiffLength, trailing, 1u,
                   "bytes outside declared RIFF extent are rejected");

    auto truncatedChunk = valid;
    truncatedChunk.push_back(0x42u);
    StoreU32(truncatedChunk, 4u,
             static_cast<std::uint32_t>(truncatedChunk.size() - 8u));
    ExpectRejected(MusicPcmWaveStatus::InvalidChunk, truncatedChunk, 1u,
                   "trailing partial chunk header is rejected");

    auto oversizedChunk = valid;
    StoreU32(oversizedChunk, 40u, 0xffff'ffffu);
    ExpectRejected(MusicPcmWaveStatus::InvalidChunk, oversizedChunk, 1u,
                   "chunk payload past RIFF end is rejected");

    std::vector<std::uint8_t> noOddPad;
    AppendFourCc(noOddPad, "RIFF");
    AppendU32(noOddPad, 0u);
    AppendFourCc(noOddPad, "WAVE");
    AppendChunk(noOddPad, "fmt ", FormatPayload());
    const std::vector<std::uint8_t> samples{1u, 0u, 2u, 0u};
    AppendChunk(noOddPad, "data", samples);
    const std::vector<std::uint8_t> oddByte{0x7fu};
    AppendChunk(noOddPad, "JUNK", oddByte, false);
    StoreU32(noOddPad, 4u, static_cast<std::uint32_t>(noOddPad.size() - 8u));
    ExpectRejected(MusicPcmWaveStatus::InvalidChunkPadding, noOddPad, 1u,
                   "odd-sized chunk without its pad byte is rejected");
}

void TestChunkUniquenessAndPresence()
{
    const auto format = FormatPayload();
    const std::vector<std::uint8_t> samples{1u, 0u, 2u, 0u};
    ExpectRejected(MusicPcmWaveStatus::DuplicateFormatChunk,
                   MakeWave({{"fmt ", format}, {"fmt ", format}, {"data", samples}}), 1u,
                   "duplicate fmt chunks are rejected");
    ExpectRejected(MusicPcmWaveStatus::DuplicateDataChunk,
                   MakeWave({{"fmt ", format}, {"data", samples}, {"data", samples}}), 1u,
                   "duplicate data chunks are rejected");
    ExpectRejected(MusicPcmWaveStatus::MissingFormatChunk,
                   MakeWave({{"data", samples}}), 1u,
                   "missing fmt chunk is rejected");
    ExpectRejected(MusicPcmWaveStatus::MissingDataChunk,
                   MakeWave({{"fmt ", format}}), 1u,
                   "missing data chunk is rejected");
    ExpectRejected(MusicPcmWaveStatus::InvalidFormatChunk,
                   MakeWave({{"fmt ", {1u, 0u}}, {"data", samples}}), 1u,
                   "short fmt chunk is rejected");
}

void TestFormatAndFrameValidation()
{
    const std::vector<std::uint8_t> samples{1u, 0u, 2u, 0u};
    WaveFormat format;
    format.tag = 3u;
    ExpectRejected(MusicPcmWaveStatus::UnsupportedEncoding,
                   SmallWave(samples, format), 1u, "non-PCM encoding is rejected");

    format = {};
    format.channels = 1u;
    format.blockAlignment = 2u;
    format.byteRate = 96'000u;
    ExpectRejected(MusicPcmWaveStatus::WrongChannelCount,
                   SmallWave(samples, format), 1u, "mono format is rejected");

    format = {};
    format.sampleRate = 44'100u;
    format.byteRate = 176'400u;
    ExpectRejected(MusicPcmWaveStatus::WrongSampleRate,
                   SmallWave(samples, format), 1u, "wrong sample rate is rejected");

    format = {};
    format.bitsPerSample = 8u;
    format.blockAlignment = 2u;
    format.byteRate = 96'000u;
    ExpectRejected(MusicPcmWaveStatus::WrongBitDepth,
                   SmallWave(samples, format), 1u, "non-16-bit format is rejected");

    format = {};
    format.blockAlignment = 2u;
    ExpectRejected(MusicPcmWaveStatus::WrongBlockAlignment,
                   SmallWave(samples, format), 1u, "incorrect block alignment is rejected");

    format = {};
    format.byteRate = 1u;
    ExpectRejected(MusicPcmWaveStatus::WrongByteRate,
                   SmallWave(samples, format), 1u, "incorrect byte rate is rejected");

    ExpectRejected(MusicPcmWaveStatus::OddDataLength,
                   SmallWave({1u, 0u, 2u}), 1u, "odd data byte count is rejected");
    ExpectRejected(MusicPcmWaveStatus::InvalidDataAlignment,
                   SmallWave({1u, 0u, 2u, 0u, 3u, 0u}), 1u,
                   "data not aligned to a stereo frame is rejected");
    ExpectRejected(MusicPcmWaveStatus::WrongFrameCount,
                   SmallWave(samples), 2u, "unexpected exact frame count is rejected");
    ExpectRejected(MusicPcmWaveStatus::InvalidExpectedFrameCount,
                   SmallWave(samples), 0u, "zero expected frame count is rejected");
    ExpectRejected(MusicPcmWaveStatus::InvalidExpectedFrameCount,
                   SmallWave(samples), 576'001u, "oversized expected frame count is rejected");

    const std::vector<std::uint8_t> excessiveMetadata(65'536u, 0x5au);
    ExpectRejected(MusicPcmWaveStatus::FileTooLarge,
                   MakeWave({{"fmt ", FormatPayload()}, {"data", samples},
                             {"JUNK", excessiveMetadata}}), 1u,
                   "container overhead beyond the explicit parser bound is rejected");
}

void TestActualMusicPrototypes()
{
    constexpr std::array<char, 8u> cues{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
    const std::filesystem::path prototypeDirectory{HORDE_RT_MUSIC_PROTOTYPE_DIR};
    for (const char cue : cues)
    {
        const std::uint32_t bodyFrames = cue == 'C' ? 144'000u
            : cue == 'G' ? 288'000u : 576'000u;
        for (const char* part : {"loop", "tail"})
        {
            const std::uint32_t expectedFrames = std::string(part) == "tail"
                ? 144'000u : bodyFrames;
            const std::filesystem::path path = prototypeDirectory /
                (std::string(1u, cue) + "-" + part + ".wav");
            std::ifstream input(path, std::ios::binary | std::ios::ate);
            Check(static_cast<bool>(input), "actual rendered prototype WAV is readable");
            if (!input)
            {
                continue;
            }
            const std::streamsize fileSize = input.tellg();
            Check(fileSize > 0, "prototype WAV has nonzero bytes");
            if (fileSize <= 0)
            {
                continue;
            }
            std::vector<char> rawBytes(static_cast<std::size_t>(fileSize));
            input.seekg(0, std::ios::beg);
            input.read(rawBytes.data(), fileSize);
            Check(static_cast<bool>(input), "prototype WAV bytes are fully read");
            if (!input)
            {
                continue;
            }
            std::vector<std::uint8_t> fileBytes(rawBytes.size());
            std::transform(rawBytes.begin(), rawBytes.end(), fileBytes.begin(),
                [](const char value)
                {
                    return static_cast<std::uint8_t>(static_cast<unsigned char>(value));
                });

            std::vector<std::int16_t> samples;
            const MusicPcmWaveStatus status = DecodeMusicPcmWave(
                fileBytes, expectedFrames, samples);
            Check(status == MusicPcmWaveStatus::Ok,
                  "actual prototype body/tail satisfies strict decoder format");
            Check(samples.size() == static_cast<std::size_t>(expectedFrames) * 2u,
                  "actual prototype decodes to exact expected interleaved frame count");
            Check(std::any_of(samples.begin(), samples.end(), [](const std::int16_t value)
                  { return value != 0; }), "actual prototype audio is not all zero");
        }
    }
}

} // namespace

int main()
{
    TestValidUnknownChunksAndUnalignedLittleEndianSamples();
    TestChunkOrderAndExtendedPcmFormat();
    TestHeaderAndChunkBoundaries();
    TestChunkUniquenessAndPresence();
    TestFormatAndFrameValidation();
    TestActualMusicPrototypes();
    return passed ? 0 : 1;
}
