#include "pocket_audio/PcmLoopStream.h"
#include "pocket_audio/PcmWave.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

#if defined(_MSC_VER)
#include <crtdbg.h>
#endif

namespace
{

void Check(const bool condition, const char* expression, const int line)
{
    if (!condition)
    {
        std::cerr << "Check failed at line " << line << ": " << expression << '\n';
        throw std::runtime_error("native PCM contract failed");
    }
}

#define CHECK(expression) Check((expression), #expression, __LINE__)

template <std::size_t SampleCount>
std::span<float> OutputFrames(std::array<float, SampleCount>& output,
                              const std::size_t frameCount)
{
    CHECK(frameCount <= output.size() / 2u);
    return std::span<float>(output).first(frameCount * 2u);
}

std::vector<std::int16_t> StereoFrames(const std::initializer_list<std::int16_t> left)
{
    std::vector<std::int16_t> samples;
    samples.reserve(left.size() * 2u);
    for (const std::int16_t sample : left)
    {
        samples.push_back(sample);
        samples.push_back(sample);
    }
    return samples;
}

pocket_audio::PcmSelection Selection(const std::uint8_t index,
                                     const bool looping,
                                     const std::uint64_t revision,
                                     const double positionSeconds = 0.0)
{
    return {
        .clipIndex = index,
        .looping = looping,
        .suspended = false,
        .discontinuity = false,
        .clockValid = true,
        .positionSeconds = positionSeconds,
        .revision = revision,
        .naturalTailHandoff = false,
    };
}

void CheckStereoConstant(const std::span<const float> output,
                         const std::size_t firstFrame,
                         const std::size_t frameCount,
                         const float expected)
{
    for (std::size_t frame = firstFrame; frame < firstFrame + frameCount; ++frame)
    {
        CHECK(std::abs(output[frame * 2u] - expected) < 0.000001f);
        CHECK(std::abs(output[frame * 2u + 1u] - expected) < 0.000001f);
    }
}

void TestClipAndSelectionValidation()
{
    using namespace pocket_audio;
    const std::vector<std::int16_t> body = StereoFrames({1000, 1100, 1200, 1300});
    const std::vector<std::int16_t> tail = StereoFrames({100, 200});
    const std::array<PcmClip, 2> clips{{{}, {body, tail, true}}};
    PcmLoopStream stream(clips, 2u);
    CHECK(stream.IsValid());

    std::array<float, 4> output{};
    CHECK(stream.Render(output) == PcmStatus::InvalidSelection);
    CHECK(std::all_of(output.begin(), output.end(), [](float v) { return v == 0.0f; }));
    CHECK(stream.SetSelection(Selection(1u, true, 1u)) == PcmStatus::Ok);

    auto bad = Selection(2u, true, 2u);
    CHECK(stream.SetSelection(bad) == PcmStatus::InvalidSelection);
    CHECK(stream.Render(output) == PcmStatus::InvalidSelection);
    CHECK(std::all_of(output.begin(), output.end(), [](float v) { return v == 0.0f; }));

    const std::array<PcmClip, 2> invalidSlotClips{{{body, {}, false}, {body, {}, true}}};
    PcmLoopStream badSlot(invalidSlotClips);
    CHECK(!badSlot.IsValid());
    const std::vector<std::int16_t> tooLongTail = StereoFrames({1, 2, 3, 4, 5});
    const std::array<PcmClip, 2> invalidTailClips{{{}, {body, tooLongTail, true}}};
    PcmLoopStream badTail(invalidTailClips);
    CHECK(!badTail.IsValid());
    const std::span<const std::int16_t> oddBodySpan(body.data(), 7u);
    const std::array<PcmClip, 2> invalidBodyClips{{{}, {oddBodySpan, {}, true}}};
    PcmLoopStream oddBody(invalidBodyClips);
    CHECK(!oddBody.IsValid());
    const std::array<PcmClip, 33> tooManyClips{};
    PcmLoopStream tooMany(tooManyClips);
    CHECK(!tooMany.IsValid());
    PcmLoopStream tooLongFade(clips, kPcmMaximumCrossfadeFrames + 1u);
    CHECK(!tooLongFade.IsValid());

    PcmLoopStream selectionChecks(clips, 0u);
    auto wrongLoop = Selection(1u, false, 1u);
    CHECK(selectionChecks.SetSelection(wrongLoop) == PcmStatus::InvalidSelection);
    auto negative = Selection(1u, true, 1u, -0.01);
    CHECK(selectionChecks.SetSelection(negative) == PcmStatus::InvalidSelection);
    auto nanPosition = Selection(1u, true, 1u, std::numeric_limits<double>::quiet_NaN());
    CHECK(selectionChecks.SetSelection(nanPosition) == PcmStatus::InvalidSelection);
    auto outOfRange = Selection(1u, true, 1u,
        static_cast<double>(body.size() / 2u) / kPcmSampleRate);
    CHECK(selectionChecks.SetSelection(outOfRange) == PcmStatus::InvalidSelection);
}

void TestLoopTailAndTwentyPeriodsWithIrregularChunks()
{
    using namespace pocket_audio;
    const std::vector<std::int16_t> body = StereoFrames({1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000});
    const std::vector<std::int16_t> tail = StereoFrames({100, 200, 300});
    const std::array<PcmClip, 2> clips{{{}, {body, tail, true}}};
    PcmLoopStream stream(clips, 0u);
    CHECK(stream.SetSelection(Selection(1u, true, 1u)) == PcmStatus::Ok);

    constexpr std::size_t periodFrames = 8u;
    constexpr std::size_t cycles = 20u;
    constexpr std::array<std::size_t, 5> chunkPattern{1u, 3u, 2u, 5u, 4u};
    std::size_t framePosition = 0u;
    std::size_t chunk = 0u;
    std::array<float, 10> output{};
    while (framePosition < periodFrames * cycles)
    {
        const std::size_t frames = std::min(chunkPattern[chunk++ % chunkPattern.size()],
            periodFrames * cycles - framePosition);
        const std::span<float> target = OutputFrames(output, frames);
        CHECK(stream.Render(target) == PcmStatus::Ok);
        for (std::size_t local = 0u; local < frames; ++local)
        {
            const std::size_t absolute = framePosition + local;
            const std::size_t cycle = absolute / periodFrames;
            const std::size_t phase = absolute % periodFrames;
            float expected = static_cast<float>(body[phase * 2u]) / 32768.0f;
            if (cycle > 0u && phase < tail.size() / 2u)
            {
                expected += static_cast<float>(tail[phase * 2u]) / 32768.0f;
            }
            CHECK(std::abs(target[local * 2u] - expected) < 0.000001f);
            CHECK(std::abs(target[local * 2u + 1u] - expected) < 0.000001f);
        }
        framePosition += frames;
    }
    CHECK(framePosition == periodFrames * cycles);
}

void TestNaturalTailRequiresCompletedBodyAndUsesRemainingTail()
{
    using namespace pocket_audio;
    const std::vector<std::int16_t> oldBody = StereoFrames({1000, 1000, 1000, 1000});
    const std::vector<std::int16_t> oldTail = StereoFrames({100, 200, 300, 400});
    const std::vector<std::int16_t> newBody = StereoFrames({2000, 2000, 2000, 2000, 2000, 2000});
    const std::array<PcmClip, 3> clips{{{}, {oldBody, oldTail, false}, {newBody, {}, false}}};
    PcmLoopStream stream(clips, 3u);
    CHECK(stream.SetSelection(Selection(1u, false, 1u)) == PcmStatus::Ok);

    std::array<float, 24> output{};
    // Interrupting before the one-shot body ends must use the bounded fade;
    // the old tail must not be jumped to or mixed additively.
    CHECK(stream.Render(std::span<float>(output.data(), 2u)) == PcmStatus::Ok);
    auto early = Selection(2u, false, 2u);
    early.naturalTailHandoff = true;
    CHECK(stream.SetSelection(early) == PcmStatus::Ok);
    CHECK(stream.Render(std::span<float>(output.data(), 2u)) == PcmStatus::Ok);
    CHECK(std::abs(output[0] - 1000.0f / 32768.0f) < 0.000001f);

    // A real body-end handoff retains the authored tail exactly, additive and
    // unnormalised, beginning at the first tail sample.
    CHECK(stream.SetSelection(Selection(1u, false, 3u)) == PcmStatus::Ok);
    CHECK(stream.Render(std::span<float>(output.data(), 8u)) == PcmStatus::Ok);
    auto atEnd = Selection(2u, false, 4u);
    atEnd.naturalTailHandoff = true;
    CHECK(stream.SetSelection(atEnd) == PcmStatus::Ok);
    CHECK(stream.Render(std::span<float>(output.data(), 2u)) == PcmStatus::Ok);
    const float bodyPlusTail0 = 2100.0f / 32768.0f;
    CHECK(std::abs(output[0] - bodyPlusTail0) < 0.000001f);

    // Late handoff resumes at the current tail cursor and then retires rather
    // than replaying or stacking the already-consumed tail.
    CHECK(stream.SetSelection(Selection(1u, false, 5u)) == PcmStatus::Ok);
    CHECK(stream.Render(OutputFrames(output, 6u)) == PcmStatus::Ok);
    CHECK(stream.SetSelection(Selection(1u, false, 6u)) == PcmStatus::Ok);
    CHECK(stream.Render(OutputFrames(output, 6u)) == PcmStatus::Ok);
    auto late = Selection(2u, false, 7u);
    late.naturalTailHandoff = true;
    CHECK(stream.SetSelection(late) == PcmStatus::Ok);
    CHECK(stream.Render(std::span<float>(output.data(), 8u)) == PcmStatus::Ok);
    CHECK(std::abs(output[0] - 2300.0f / 32768.0f) < 0.000001f);
    CHECK(std::abs(output[2] - 2400.0f / 32768.0f) < 0.000001f);
    CheckStereoConstant(output, 2u, 2u, 2000.0f / 32768.0f);
}

void TestRevisionDiscontinuitySuspensionVolumeAndReset()
{
    using namespace pocket_audio;
    const std::vector<std::int16_t> body = StereoFrames({1000, 2000, 3000, 4000, 5000, 6000});
    const std::array<PcmClip, 2> clips{{{}, {body, {}, true}}};
    PcmLoopStream stream(clips, 0u);
    CHECK(stream.SetVolumePercent(50.0f));
    CHECK(stream.SetSelection(Selection(1u, true, 5u)) == PcmStatus::Ok);
    std::array<float, 4> output{};
    CHECK(stream.Render(std::span<float>(output.data(), 2u)) == PcmStatus::Ok);
    CHECK(std::abs(output[0] - (1000.0f / 32768.0f * 0.5f)) < 0.000001f);

    // Same revision with moving clock position does not rewind the cursor.
    auto same = Selection(1u, true, 5u, 0.0);
    CHECK(stream.SetSelection(same) == PcmStatus::Ok);
    CHECK(stream.Render(std::span<float>(output.data(), 2u)) == PcmStatus::Ok);
    CHECK(std::abs(output[0] - (2000.0f / 32768.0f * 0.5f)) < 0.000001f);

    auto suspended = same;
    suspended.suspended = true;
    CHECK(stream.SetSelection(suspended) == PcmStatus::Suspended);
    CHECK(stream.Render(output) == PcmStatus::Suspended);
    CHECK(std::all_of(output.begin(), output.end(), [](float v) { return v == 0.0f; }));
    same.suspended = false;
    CHECK(stream.SetSelection(same) == PcmStatus::Ok);
    CHECK(stream.Render(std::span<float>(output.data(), 2u)) == PcmStatus::Ok);
    CHECK(std::abs(output[0] - (3000.0f / 32768.0f * 0.5f)) < 0.000001f);

    auto badClock = same;
    badClock.clockValid = false;
    CHECK(stream.SetSelection(badClock) == PcmStatus::ClockInvalid);
    CHECK(stream.Render(output) == PcmStatus::ClockInvalid);
    CHECK(std::all_of(output.begin(), output.end(), [](float v) { return v == 0.0f; }));
    same.clockValid = true;
    CHECK(stream.SetSelection(same) == PcmStatus::Ok);
    CHECK(stream.Render(std::span<float>(output.data(), 2u)) == PcmStatus::Ok);
    CHECK(std::abs(output[0] - (4000.0f / 32768.0f * 0.5f)) < 0.000001f);

    auto edge = same;
    edge.discontinuity = true;
    edge.positionSeconds = 1.0 / static_cast<double>(kPcmSampleRate);
    CHECK(stream.SetSelection(edge) == PcmStatus::Ok);
    CHECK(stream.Render(std::span<float>(output.data(), 2u)) == PcmStatus::Ok);
    CHECK(std::abs(output[0] - (2000.0f / 32768.0f * 0.5f)) < 0.000001f);
    edge.positionSeconds = 4.0 / static_cast<double>(kPcmSampleRate);
    CHECK(stream.SetSelection(edge) == PcmStatus::Ok);
    CHECK(stream.Render(std::span<float>(output.data(), 2u)) == PcmStatus::Ok);
    CHECK(std::abs(output[0] - (3000.0f / 32768.0f * 0.5f)) < 0.000001f);
    edge.discontinuity = false;
    CHECK(stream.SetSelection(edge) == PcmStatus::Ok);
    edge.discontinuity = true;
    edge.positionSeconds = 2.0 / static_cast<double>(kPcmSampleRate);
    CHECK(stream.SetSelection(edge) == PcmStatus::Ok);
    CHECK(stream.Render(std::span<float>(output.data(), 2u)) == PcmStatus::Ok);
    CHECK(std::abs(output[0] - (3000.0f / 32768.0f * 0.5f)) < 0.000001f);

    CHECK(!stream.SetVolumePercent(std::numeric_limits<float>::infinity()));
    CHECK(stream.SetVolumePercent(150.0f));
    CHECK(stream.SetSelection(Selection(1u, true, 6u)) == PcmStatus::Ok);
    CHECK(stream.Render(std::span<float>(output.data(), 2u)) == PcmStatus::Ok);
    CHECK(std::abs(output[0] - 1000.0f / 32768.0f) < 0.000001f);
    stream.Reset();
    CHECK(stream.SetSelection(Selection(1u, true, 1u)) == PcmStatus::Ok);
    CHECK(stream.Render(std::span<float>(output.data(), 2u)) == PcmStatus::Ok);
    CHECK(std::abs(output[0] - 1000.0f / 32768.0f) < 0.000001f);

    output.fill(1.0f);
    CHECK(stream.Render(std::span<float>(output.data(), 3u)) == PcmStatus::InvalidOutput);
    CHECK(output[0] == 0.0f && output[1] == 0.0f && output[2] == 0.0f);
    CHECK(stream.SetSelection(Selection(1u, true, 0u)) == PcmStatus::InvalidSelection);
    CHECK(stream.SetSelection(Selection(1u, true, 9u)) == PcmStatus::Ok);
    CHECK(stream.SetSelection(Selection(1u, true, 8u)) == PcmStatus::InvalidSelection);
}

void TestRapidChangesReplaceOutgoingAndNoneClears()
{
    using namespace pocket_audio;
    const std::vector<std::int16_t> first = StereoFrames({1000, 1000, 1000, 1000, 1000, 1000});
    const std::vector<std::int16_t> second = StereoFrames({2000, 2000, 2000, 2000, 2000, 2000});
    const std::vector<std::int16_t> third = StereoFrames({3000, 3000, 3000, 3000, 3000, 3000});
    const std::array<PcmClip, 4> clips{{{}, {first, {}, true}, {second, {}, true}, {third, {}, true}}};
    PcmLoopStream stream(clips, 4u);
    std::array<float, 2> output{};
    CHECK(stream.SetSelection(Selection(1u, true, 1u)) == PcmStatus::Ok);
    CHECK(stream.SetSelection(Selection(2u, true, 2u)) == PcmStatus::Ok);
    CHECK(stream.Render(output) == PcmStatus::Ok);
    CHECK(std::abs(output[0] - 1000.0f / 32768.0f) < 0.000001f);
    CHECK(stream.SetSelection(Selection(3u, true, 3u)) == PcmStatus::Ok);
    CHECK(stream.Render(output) == PcmStatus::Ok);
    CHECK(std::abs(output[0] - 2000.0f / 32768.0f) < 0.000001f);
    CHECK(stream.SetSelection(Selection(0u, false, 4u)) == PcmStatus::Ok);
    CHECK(stream.Render(output) == PcmStatus::Ok);
    CHECK(output[0] == 0.0f && output[1] == 0.0f);

    PcmLoopStream immediateCut(clips, 0u);
    CHECK(immediateCut.SetSelection(Selection(1u, true, 1u)) == PcmStatus::Ok);
    CHECK(immediateCut.SetSelection(Selection(2u, true, 2u)) == PcmStatus::Ok);
    CHECK(immediateCut.Render(output) == PcmStatus::Ok);
    CHECK(std::abs(output[0] - 2000.0f / 32768.0f) < 0.000001f);
}

void AppendU16(std::vector<std::uint8_t>& bytes, const std::uint16_t value)
{
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
}

void AppendU32(std::vector<std::uint8_t>& bytes, const std::uint32_t value)
{
    for (unsigned int shift = 0u; shift < 32u; shift += 8u)
    {
        bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffu));
    }
}

void AppendTag(std::vector<std::uint8_t>& bytes, const std::string_view tag)
{
    for (const char value : tag)
    {
        bytes.push_back(static_cast<std::uint8_t>(value));
    }
}

void AppendChunk(std::vector<std::uint8_t>& bytes,
                 const std::string_view tag,
                 const std::span<const std::uint8_t> payload)
{
    AppendTag(bytes, tag);
    AppendU32(bytes, static_cast<std::uint32_t>(payload.size()));
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    if ((payload.size() & 1u) != 0u)
    {
        bytes.push_back(0u);
    }
}

void FinalizeRiffLength(std::vector<std::uint8_t>& bytes)
{
    const std::uint32_t riffBytes = static_cast<std::uint32_t>(bytes.size() - 8u);
    for (unsigned int offset = 0u; offset < 4u; ++offset)
    {
        bytes[4u + offset] = static_cast<std::uint8_t>((riffBytes >> (offset * 8u)) & 0xffu);
    }
}

std::vector<std::uint8_t> MakeFmt(const std::uint16_t format = 1u,
                                  const std::uint16_t channels = 2u,
                                  const std::uint32_t sampleRate = 48'000u,
                                  const std::uint16_t bits = 16u,
                                  const std::uint16_t alignment = 4u,
                                  const std::uint32_t byteRate = 192'000u,
                                  const bool extension = false)
{
    std::vector<std::uint8_t> fmt;
    AppendU16(fmt, format);
    AppendU16(fmt, channels);
    AppendU32(fmt, sampleRate);
    AppendU32(fmt, byteRate);
    AppendU16(fmt, alignment);
    AppendU16(fmt, bits);
    if (extension)
    {
        AppendU16(fmt, 2u);
        fmt.push_back(0xabu);
        fmt.push_back(0xcdu);
    }
    return fmt;
}

std::vector<std::uint8_t> MakeWave(const std::vector<std::uint8_t>& fmt,
                                  const std::vector<std::uint8_t>& data,
                                  const bool dataFirst = false,
                                  const bool oddUnknown = false)
{
    std::vector<std::uint8_t> bytes;
    AppendTag(bytes, "RIFF");
    AppendU32(bytes, 0u);
    AppendTag(bytes, "WAVE");
    if (oddUnknown)
    {
        const std::array<std::uint8_t, 3> unknown{0x11u, 0x22u, 0x33u};
        AppendChunk(bytes, "JUNK", unknown);
    }
    if (dataFirst)
    {
        AppendChunk(bytes, "data", data);
        AppendChunk(bytes, "fmt ", fmt);
    }
    else
    {
        AppendChunk(bytes, "fmt ", fmt);
        AppendChunk(bytes, "data", data);
    }
    FinalizeRiffLength(bytes);
    return bytes;
}

std::vector<std::uint8_t> EncodeSamples(const std::initializer_list<std::int16_t> samples)
{
    std::vector<std::uint8_t> bytes;
    bytes.reserve(samples.size() * 2u);
    for (const std::int16_t sample : samples)
    {
        const std::uint16_t encoded = static_cast<std::uint16_t>(sample);
        bytes.push_back(static_cast<std::uint8_t>(encoded & 0xffu));
        bytes.push_back(static_cast<std::uint8_t>(encoded >> 8u));
    }
    return bytes;
}

void ExpectDecodeFailure(std::vector<std::uint8_t> bytes,
                         const std::uint32_t frames,
                         const pocket_audio::PcmWaveStatus expected)
{
    std::vector<std::int16_t> decoded{55, 66};
    CHECK(pocket_audio::DecodePcmWave(bytes, frames, decoded) == expected);
    CHECK(decoded.empty());
}

void TestWaveDecoderValidOrderPaddingEndianAndAlignment()
{
    using namespace pocket_audio;
    const auto data = EncodeSamples({0, 32767, -32768, -2});
    const auto fmt = MakeFmt(1u, 2u, 48'000u, 16u, 4u, 192'000u, true);
    auto bytes = MakeWave(fmt, data, true, true);
    std::vector<std::uint8_t> unaligned(bytes.size() + 1u, 0xffu);
    std::copy(bytes.begin(), bytes.end(), unaligned.begin() + 1);
    std::vector<std::int16_t> decoded;
    const auto input = std::span<const std::uint8_t>(unaligned.data() + 1u, bytes.size());
    CHECK(DecodePcmWave(input, 2u, decoded) == PcmWaveStatus::Ok);
    CHECK(decoded == std::vector<std::int16_t>({0, 32767, -32768, -2}));
}

void TestWaveDecoderRejectsMalformedInputsTransactionally()
{
    using namespace pocket_audio;
    const auto fmt = MakeFmt();
    const auto data = EncodeSamples({1, -1, 2, -2});
    const auto good = MakeWave(fmt, data);

    auto wrongRiff = good;
    wrongRiff[0] = static_cast<std::uint8_t>('X');
    ExpectDecodeFailure(wrongRiff, 2u, PcmWaveStatus::InvalidRiffHeader);
    auto wrongWave = good;
    wrongWave[8] = static_cast<std::uint8_t>('X');
    ExpectDecodeFailure(wrongWave, 2u, PcmWaveStatus::InvalidRiffHeader);
    auto shortRiff = good;
    shortRiff[4] = 3u;
    shortRiff[5] = shortRiff[6] = shortRiff[7] = 0u;
    ExpectDecodeFailure(shortRiff, 2u, PcmWaveStatus::InvalidRiffLength);
    auto wrongLength = good;
    wrongLength[4] -= 1u;
    ExpectDecodeFailure(wrongLength, 2u, PcmWaveStatus::InvalidRiffLength);
    auto truncated = std::vector<std::uint8_t>(good.begin(), good.begin() + 20);
    FinalizeRiffLength(truncated);
    ExpectDecodeFailure(truncated, 2u, PcmWaveStatus::InvalidChunk);

    auto duplicateFmt = good;
    const auto duplicate = MakeWave(fmt, data);
    const std::size_t fmtChunkBytes = 8u + fmt.size();
    duplicateFmt.insert(duplicateFmt.end() - static_cast<std::ptrdiff_t>(8u + data.size()),
                        duplicate.begin() + 12, duplicate.begin() + 12 + static_cast<std::ptrdiff_t>(fmtChunkBytes));
    const std::uint32_t duplicateSize = static_cast<std::uint32_t>(duplicateFmt.size() - 8u);
    for (unsigned int shift = 0u; shift < 32u; shift += 8u)
    {
        duplicateFmt[4u + shift / 8u] = static_cast<std::uint8_t>((duplicateSize >> shift) & 0xffu);
    }
    ExpectDecodeFailure(duplicateFmt, 2u, PcmWaveStatus::DuplicateFormatChunk);

    std::vector<std::uint8_t> duplicateData;
    AppendTag(duplicateData, "RIFF");
    AppendU32(duplicateData, 0u);
    AppendTag(duplicateData, "WAVE");
    AppendChunk(duplicateData, "fmt ", fmt);
    AppendChunk(duplicateData, "data", data);
    AppendChunk(duplicateData, "data", data);
    FinalizeRiffLength(duplicateData);
    ExpectDecodeFailure(duplicateData, 2u, PcmWaveStatus::DuplicateDataChunk);

    ExpectDecodeFailure(MakeWave(MakeFmt(3u), data), 2u, PcmWaveStatus::UnsupportedEncoding);
    ExpectDecodeFailure(MakeWave(MakeFmt(1u, 1u), data), 2u, PcmWaveStatus::WrongChannelCount);
    ExpectDecodeFailure(MakeWave(MakeFmt(1u, 2u, 44'100u), data), 2u, PcmWaveStatus::WrongSampleRate);
    ExpectDecodeFailure(MakeWave(MakeFmt(1u, 2u, 48'000u, 24u), data), 2u, PcmWaveStatus::WrongBitDepth);
    ExpectDecodeFailure(MakeWave(MakeFmt(1u, 2u, 48'000u, 16u, 2u), data), 2u, PcmWaveStatus::WrongBlockAlignment);
    ExpectDecodeFailure(MakeWave(MakeFmt(1u, 2u, 48'000u, 16u, 4u, 1u), data), 2u, PcmWaveStatus::WrongByteRate);
    ExpectDecodeFailure(MakeWave(fmt, std::vector<std::uint8_t>{1u, 2u, 3u}), 2u,
                        PcmWaveStatus::OddDataLength);
    ExpectDecodeFailure(MakeWave(fmt, data), 3u, PcmWaveStatus::WrongFrameCount);
    ExpectDecodeFailure(good, 0u, PcmWaveStatus::InvalidExpectedFrameCount);
    ExpectDecodeFailure(good, kPcmWaveMaximumFrameCount + 1u, PcmWaveStatus::InvalidExpectedFrameCount);

    std::vector<std::uint8_t> noFmt;
    AppendTag(noFmt, "RIFF");
    AppendU32(noFmt, 0u);
    AppendTag(noFmt, "WAVE");
    AppendChunk(noFmt, "data", data);
    FinalizeRiffLength(noFmt);
    ExpectDecodeFailure(noFmt, 2u, PcmWaveStatus::MissingFormatChunk);

    std::vector<std::uint8_t> noData;
    AppendTag(noData, "RIFF");
    AppendU32(noData, 0u);
    AppendTag(noData, "WAVE");
    AppendChunk(noData, "fmt ", fmt);
    FinalizeRiffLength(noData);
    ExpectDecodeFailure(noData, 2u, PcmWaveStatus::MissingDataChunk);
}

} // namespace

int main()
{
#if defined(_MSC_VER)
    (void)_set_error_mode(_OUT_TO_STDERR);
    (void)_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    (void)_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    (void)_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    (void)_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif
    try
    {
        std::cerr << "clip validation\n";
        TestClipAndSelectionValidation();
        std::cerr << "loop periods\n";
        TestLoopTailAndTwentyPeriodsWithIrregularChunks();
        std::cerr << "natural tail\n";
        TestNaturalTailRequiresCompletedBodyAndUsesRemainingTail();
        std::cerr << "selection controls\n";
        TestRevisionDiscontinuitySuspensionVolumeAndReset();
        std::cerr << "rapid changes\n";
        TestRapidChangesReplaceOutgoingAndNoneClears();
        std::cerr << "wave valid\n";
        TestWaveDecoderValidOrderPaddingEndianAndAlignment();
        std::cerr << "wave invalid\n";
        TestWaveDecoderRejectsMalformedInputsTransactionally();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "Pocket Audio Core native PCM tests passed\n";
    return 0;
}
