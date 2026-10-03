#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <vector>

#include "audio/MusicPcmStream.h"

namespace
{

using namespace horde::audio;

bool passed = true;

void Check(const bool condition, const char* message)
{
    if (!condition)
    {
        passed = false;
        std::cerr << "Music PCM stream test failed: " << message << '\n';
    }
}

bool Near(const float left, const float right, const float epsilon = 0.00001f)
{
    return std::abs(left - right) <= epsilon;
}

struct ClipSet
{
    std::array<std::vector<std::int16_t>, kMusicPcmCueCount> bodies;
    std::array<std::vector<std::int16_t>, kMusicPcmCueCount> tails;
    std::array<MusicPcmClip, kMusicPcmCueCount> clips{};

    ClipSet()
    {
        for (std::size_t index = 1u; index < kMusicPcmCueCount; ++index)
        {
            const auto cue = static_cast<MusicCue>(index);
            const std::uint64_t frames = cue == MusicCue::C ? 144'000u
                : cue == MusicCue::G ? 288'000u : kMusicPcmLoopFrames;
            bodies[index].resize(static_cast<std::size_t>(frames * 2u));
            tails[index].resize(static_cast<std::size_t>(kMusicPcmTailFrames * 2u));
            const auto bodyValue = static_cast<std::int16_t>(1'000 + index * 1'000);
            std::fill(bodies[index].begin(), bodies[index].end(), bodyValue);
            for (std::size_t frame = 0u; frame < tails[index].size() / 2u; ++frame)
            {
                const auto value = static_cast<std::int16_t>(300 + frame % 1'000u);
                tails[index][frame * 2u] = value;
                tails[index][frame * 2u + 1u] = static_cast<std::int16_t>(-value);
            }
            // A marker at frame zero makes exact loop-period placement visible.
            bodies[index][0] = 15'000;
            bodies[index][1] = -15'000;
            clips[index] = {bodies[index], tails[index]};
        }
    }

    void Rebind(const std::size_t index)
    {
        clips[index] = {bodies[index], tails[index]};
    }
};

MusicSelection Selection(const MusicCue cue, const std::uint64_t revision,
                         const double position = 0.0)
{
    const bool looping = cue == MusicCue::A || cue == MusicCue::B || cue == MusicCue::D ||
                         cue == MusicCue::E || cue == MusicCue::F || cue == MusicCue::H;
    return {.cue = cue, .looping = looping, .positionSeconds = position, .revision = revision};
}

void RenderFrames(MusicPcmStream& stream, const std::uint64_t frames,
                  std::vector<float>& buffer, const std::array<std::size_t, 5>& chunkPattern)
{
    std::uint64_t rendered = 0u;
    std::size_t chunkIndex = 0u;
    while (rendered < frames)
    {
        const std::size_t count = static_cast<std::size_t>(std::min<std::uint64_t>(
            chunkPattern[chunkIndex++ % chunkPattern.size()], frames - rendered));
        const auto status = stream.Render(std::span<float>(buffer.data(), count * 2u));
        Check(status == MusicPcmStatus::Ok, "irregular render chunk is accepted");
        rendered += count;
    }
}

void TestClipAndSelectionValidation()
{
    ClipSet clips;
    MusicPcmStream stream(clips.clips);
    Check(stream.IsValid(), "nine exact cue rows are accepted");

    auto badCount = std::span<const MusicPcmClip>(clips.clips.data(), clips.clips.size() - 1u);
    MusicPcmStream missingRow(badCount);
    Check(!missingRow.IsValid(), "missing cue row is rejected");

    auto wrongBody = clips.clips;
    wrongBody[static_cast<std::size_t>(MusicCue::C)].body =
        std::span<const std::int16_t>(clips.bodies[3].data(), clips.bodies[3].size() - 2u);
    MusicPcmStream malformedClip(wrongBody);
    Check(!malformedClip.IsValid(), "incorrect body frame count is rejected");

    auto wrongTail = clips.clips;
    wrongTail[static_cast<std::size_t>(MusicCue::G)].tail =
        std::span<const std::int16_t>(clips.tails[7].data(), clips.tails[7].size() - 2u);
    MusicPcmStream malformedTail(wrongTail);
    Check(!malformedTail.IsValid(), "incorrect tail frame count is rejected");

    auto nonemptyNone = clips.clips;
    nonemptyNone[0] = clips.clips[1];
    MusicPcmStream malformedNone(nonemptyNone);
    Check(!malformedNone.IsValid(), "None must map to an empty clip row");

    Check(stream.SetSelection(Selection(MusicCue::A, 1u)) == MusicPcmStatus::Ok,
          "valid loop selection is accepted");
    Check(stream.SetSelection(Selection(MusicCue::C, 1u)) == MusicPcmStatus::InvalidSelection,
          "cue changes without a new revision fail closed");
    std::array<float, 4> silent{};
    Check(stream.Render(silent) == MusicPcmStatus::InvalidSelection &&
          std::all_of(silent.begin(), silent.end(), [](const float value) { return value == 0.0f; }),
          "rejected selection silences stale playback");

    Check(stream.SetSelection(Selection(MusicCue::B, 2u, 12.0)) ==
              MusicPcmStatus::InvalidSelection,
          "loop seek at the exclusive body end is rejected");
    auto wrongLoopFlag = Selection(MusicCue::G, 3u);
    wrongLoopFlag.looping = true;
    Check(stream.SetSelection(wrongLoopFlag) == MusicPcmStatus::InvalidSelection,
          "one-shot cannot be marked looping");
    auto nanPosition = Selection(MusicCue::G, 4u);
    nanPosition.positionSeconds = std::numeric_limits<double>::quiet_NaN();
    Check(stream.SetSelection(nanPosition) == MusicPcmStatus::InvalidSelection,
          "non-finite seek is rejected");

    Check(stream.SetSelection(Selection(MusicCue::A, 10u)) == MusicPcmStatus::Ok,
          "valid selection can recover after rejected input");
    auto decreasingRevision = Selection(MusicCue::A, 9u);
    Check(stream.SetSelection(decreasingRevision) == MusicPcmStatus::InvalidSelection,
          "decreasing selection revision is rejected");
    std::array<float, 4> rejectedSilence{1.0f, 1.0f, 1.0f, 1.0f};
    Check(stream.Render(rejectedSilence) == MusicPcmStatus::InvalidSelection &&
          std::all_of(rejectedSilence.begin(), rejectedSilence.end(),
                      [](const float value) { return value == 0.0f; }),
          "decreasing revision fails closed without stale audio");

    auto unknownCue = Selection(static_cast<MusicCue>(255u), 1u);
    Check(stream.SetSelection(unknownCue) == MusicPcmStatus::InvalidSelection,
          "unknown cue value is rejected");
    auto negativePosition = Selection(MusicCue::G, 1u);
    negativePosition.positionSeconds = -0.001;
    Check(stream.SetSelection(negativePosition) == MusicPcmStatus::InvalidSelection,
          "negative source position is rejected");

    std::array<float, 3> oddOutput{7.0f, 8.0f, 9.0f};
    Check(stream.Render(oddOutput) == MusicPcmStatus::InvalidOutput &&
          std::all_of(oddOutput.begin(), oddOutput.end(), [](const float value) { return value == 0.0f; }),
          "odd interleaved output shape is rejected and silenced");
    Check(!stream.SetVolumePercent(std::numeric_limits<float>::quiet_NaN()),
          "non-finite volume is rejected");
}

void TestLoopTailAndTwentyExactPeriods()
{
    ClipSet clips;
    constexpr std::size_t a = static_cast<std::size_t>(MusicCue::A);
    clips.bodies[a][2] = 2'000;
    clips.bodies[a][4] = 3'000;
    clips.bodies[a][0] = 15'000;
    clips.tails[a][0] = 3'000;
    clips.tails[a][1] = -3'000;
    clips.Rebind(a);
    MusicPcmStream stream(clips.clips);
    Check(stream.SetSelection(Selection(MusicCue::A, 1u)) == MusicPcmStatus::Ok,
          "loop starts at the selected frame");

    std::vector<float> buffer(8'192u);
    const std::array<std::size_t, 5> chunks{37u, 1'024u, 4'093u, 257u, 2'011u};
    constexpr std::uint64_t duration = 20u * kMusicPcmLoopFrames + 1u;
    std::uint64_t rendered = 0u;
    std::size_t chunkIndex = 0u;
    std::uint32_t observedMarkers = 0u;
    while (rendered < duration)
    {
        const std::size_t count = static_cast<std::size_t>(std::min<std::uint64_t>(
            chunks[chunkIndex++ % chunks.size()], duration - rendered));
        auto output = std::span<float>(buffer.data(), count * 2u);
        Check(stream.Render(output) == MusicPcmStatus::Ok,
              "loop accepts every irregular callback size");
        for (std::size_t frame = 0u; frame < count; ++frame)
        {
            const std::uint64_t absoluteFrame = rendered + frame;
            const bool expectedStart = absoluteFrame % kMusicPcmLoopFrames == 0u;
            const float left = output[frame * 2u];
            const float right = output[frame * 2u + 1u];
            if (!std::isfinite(left) || !std::isfinite(right))
            {
                Check(false, "loop samples remain finite");
            }
            if (expectedStart)
            {
                ++observedMarkers;
                const std::int16_t expected = absoluteFrame == 0u ? 15'000 : 18'000;
                Check(Near(left, static_cast<float>(expected) / 32768.0f) &&
                      Near(right, static_cast<float>(-expected) / 32768.0f),
                      "body begins exactly every 576000 frames with prior-tail sum after wrap");
            }
        }
        rendered += count;
    }
    Check(observedMarkers == 21u,
          "twenty complete 12-second periods retain exact sample-frame starts");

    auto positionOnly = Selection(MusicCue::A, 1u, 5.0 / 48'000.0);
    Check(stream.SetSelection(positionOnly) == MusicPcmStatus::Ok,
          "same-revision position refresh is accepted");
    std::array<float, 2> nextFrame{};
    Check(stream.Render(nextFrame) == MusicPcmStatus::Ok &&
          Near(nextFrame[0], 2'301.0f / 32768.0f),
          "same revision does not seek/reset the current cursor");
}

void TestOneShotTailsAndAuthoredGSeek()
{
    ClipSet clips;
    constexpr std::size_t g = static_cast<std::size_t>(MusicCue::G);
    constexpr std::size_t fsharpFrame = 4u * 48'000u + 24'000u;
    clips.bodies[g][fsharpFrame * 2u] = 12'345;
    clips.bodies[g][fsharpFrame * 2u + 1u] = -12'345;
    clips.Rebind(g);
    MusicPcmStream stream(clips.clips);
    Check(stream.SetSelection(Selection(MusicCue::G, 1u, 4.5)) == MusicPcmStatus::Ok,
          "G seek retains its authored 4.50-second boundary");
    std::array<float, 2> sample{};
    Check(stream.Render(sample) == MusicPcmStatus::Ok &&
          Near(sample[0], 12'345.0f / 32768.0f) &&
          Near(sample[1], -12'345.0f / 32768.0f),
          "G begins on the exact 4.50-second/F-sharp marker frame");

    Check(stream.SetSelection(Selection(MusicCue::C, 2u)) == MusicPcmStatus::Ok,
          "C one-shot starts");
    std::vector<float> buffer(8'192u);
    const std::array<std::size_t, 5> chunks{31u, 2'047u, 89u, 4'096u, 311u};
    RenderFrames(stream, 144'000u - 1u, buffer, chunks);
    Check(stream.Render(sample) == MusicPcmStatus::Ok &&
          Near(sample[0], 4'000.0f / 32768.0f),
          "C's last body frame is played before the separate tail");
    Check(stream.Render(sample) == MusicPcmStatus::Ok &&
          Near(sample[0], 300.0f / 32768.0f),
          "one-shot appends its full tail immediately after its body");
    RenderFrames(stream, kMusicPcmTailFrames - 1u, buffer, chunks);
    sample.fill(1.0f);
    Check(stream.Render(sample) == MusicPcmStatus::Ok && sample[0] == 0.0f && sample[1] == 0.0f,
          "completed one-shot tail ends in silence without repeating");
}

void TestNaturalTailTransitions()
{
    ClipSet clips;
    MusicPcmStream stream(clips.clips);
    std::vector<float> buffer(8'192u);
    const std::array<std::size_t, 5> chunks{511u, 2'009u, 17u, 4'000u, 333u};
    std::array<float, 2> sample{};

    Check(stream.SetSelection(Selection(MusicCue::C, 1u)) == MusicPcmStatus::Ok,
          "C begins for natural C-to-D transition");
    RenderFrames(stream, 144'010u, buffer, chunks);
    Check(stream.SetSelection(Selection(MusicCue::D, 2u)) == MusicPcmStatus::Ok,
          "C-to-D starts the new bed immediately");
    Check(stream.Render(sample) == MusicPcmStatus::Ok &&
          Near(sample[0], (15'000.0f + 310.0f) / 32768.0f) &&
          Near(sample[1], (-15'000.0f - 310.0f) / 32768.0f),
          "C tail resumes at its exact rendered offset and overlays D without fading/cropping");
    RenderFrames(stream, kMusicPcmTailFrames - 11u, buffer, chunks);
    Check(stream.Render(sample) == MusicPcmStatus::Ok &&
          Near(sample[0], 5'000.0f / 32768.0f),
          "D remains alone after the preserved C tail completes");

    MusicPcmStream atBoundary(clips.clips);
    Check(atBoundary.SetSelection(Selection(MusicCue::C, 1u)) == MusicPcmStatus::Ok,
          "C starts for exact body-boundary transition");
    RenderFrames(atBoundary, 144'000u, buffer, chunks);
    Check(atBoundary.SetSelection(Selection(MusicCue::D, 2u)) == MusicPcmStatus::Ok,
          "C-to-D changes at the exact authored body boundary");
    Check(atBoundary.Render(sample) == MusicPcmStatus::Ok &&
          Near(sample[0], (15'000.0f + 300.0f) / 32768.0f) &&
          Near(sample[1], (-15'000.0f - 300.0f) / 32768.0f),
          "exact body-end transition preserves tail frame zero without a crossfade");

    MusicPcmStream afterTail(clips.clips);
    Check(afterTail.SetSelection(Selection(MusicCue::C, 1u)) == MusicPcmStatus::Ok,
          "C starts for late transition after full tail");
    RenderFrames(afterTail, 144'000u + kMusicPcmTailFrames, buffer, chunks);
    Check(afterTail.SetSelection(Selection(MusicCue::D, 2u)) == MusicPcmStatus::Ok,
          "C-to-D arrives after its full tail has already played");
    Check(afterTail.Render(sample) == MusicPcmStatus::Ok &&
          Near(sample[0], 15'000.0f / 32768.0f) &&
          Near(sample[1], -15'000.0f / 32768.0f),
          "completed C tail is not replayed or attached stale to D");

    Check(stream.SetSelection(Selection(MusicCue::G, 3u)) == MusicPcmStatus::Ok,
          "G begins for natural G-to-H transition");
    RenderFrames(stream, 288'007u, buffer, chunks);
    Check(stream.SetSelection(Selection(MusicCue::H, 4u)) == MusicPcmStatus::Ok,
          "G-to-H starts the new bed immediately");
    Check(stream.Render(sample) == MusicPcmStatus::Ok &&
          Near(sample[0], (15'000.0f + 307.0f) / 32768.0f) &&
          Near(sample[1], (-15'000.0f - 307.0f) / 32768.0f),
          "G tail resumes at its exact rendered offset and overlays H at unity gain");
}

void TestEarlyOneShotChangesCrossfadeInsteadOfSkippingBody()
{
    ClipSet clips;
    MusicPcmStream stream(clips.clips);
    std::array<float, 2> sample{};
    std::array<float, 128> fade{};

    Check(stream.SetSelection(Selection(MusicCue::C, 1u)) == MusicPcmStatus::Ok,
          "early C transition begins");
    Check(stream.Render(std::span<float>(fade.data(), 20u)) == MusicPcmStatus::Ok,
          "C advances only a short distance into its authored body");
    Check(stream.SetSelection(Selection(MusicCue::D, 2u)) == MusicPcmStatus::Ok,
          "early C-to-D revision is accepted");
    Check(stream.Render(sample) == MusicPcmStatus::Ok &&
          Near(sample[0], 4'000.0f / 32768.0f),
          "early C-to-D crossfades the current C body instead of jumping to its tail");

    Check(stream.SetSelection(Selection(MusicCue::G, 3u)) == MusicPcmStatus::Ok,
          "early G transition begins");
    Check(stream.Render(std::span<float>(fade.data(), 20u)) == MusicPcmStatus::Ok,
          "G advances only a short distance into its authored body");
    Check(stream.SetSelection(Selection(MusicCue::H, 4u)) == MusicPcmStatus::Ok,
          "early G-to-H revision is accepted");
    Check(stream.Render(sample) == MusicPcmStatus::Ok &&
          Near(sample[0], 8'000.0f / 32768.0f),
          "early G-to-H crossfades the current G body instead of jumping to its tail");
}

void TestDiscontinuityEdgeSeeksExactlyOnce()
{
    ClipSet clips;
    constexpr std::size_t b = static_cast<std::size_t>(MusicCue::B);
    clips.bodies[b][20] = 10'000;
    clips.bodies[b][22] = 11'000;
    clips.bodies[b][24] = 12'000;
    clips.bodies[b][60] = 30'000;
    clips.Rebind(b);

    MusicPcmStream stream(clips.clips);
    std::array<float, 2> sample{};
    Check(stream.SetSelection(Selection(MusicCue::A, 1u)) == MusicPcmStatus::Ok,
          "A begins before a discontinuous cue change");
    Check(stream.SetSelection(Selection(MusicCue::B, 2u)) == MusicPcmStatus::Ok,
          "normal change establishes an outgoing A stream");

    auto discontinuity = Selection(MusicCue::B, 2u, 10.0 / 48'000.0);
    discontinuity.discontinuity = true;
    Check(stream.SetSelection(discontinuity) == MusicPcmStatus::Ok,
          "rising discontinuity seeks without requiring a revision increment");
    Check(stream.Render(sample) == MusicPcmStatus::Ok &&
          Near(sample[0], 10'000.0f / 32768.0f),
          "discontinuity seek clears the outgoing stream and lands on its exact frame");

    discontinuity.positionSeconds = 30.0 / 48'000.0;
    Check(stream.SetSelection(discontinuity) == MusicPcmStatus::Ok,
          "repeated true discontinuity selection is accepted");
    Check(stream.Render(sample) == MusicPcmStatus::Ok &&
          Near(sample[0], 11'000.0f / 32768.0f),
          "repeated true does not rewind or re-seek the active cursor");

    discontinuity.discontinuity = false;
    discontinuity.positionSeconds = 0.0;
    Check(stream.SetSelection(discontinuity) == MusicPcmStatus::Ok,
          "discontinuity can return low without changing revision");
    Check(stream.Render(sample) == MusicPcmStatus::Ok &&
          Near(sample[0], 12'000.0f / 32768.0f),
          "false discontinuity update retains forward cursor progress");

    discontinuity.discontinuity = true;
    discontinuity.positionSeconds = 30.0 / 48'000.0;
    Check(stream.SetSelection(discontinuity) == MusicPcmStatus::Ok,
          "new false-to-true edge seeks again at the same revision");
    Check(stream.Render(sample) == MusicPcmStatus::Ok &&
          Near(sample[0], 30'000.0f / 32768.0f),
          "second rising edge applies its new validated seek position");
}

void TestVolumeSuspensionSeekAndRapidChanges()
{
    ClipSet clips;
    constexpr std::size_t a = static_cast<std::size_t>(MusicCue::A);
    clips.bodies[a][2] = 2'000;
    clips.bodies[a][4] = 3'000;
    clips.bodies[a][6] = 4'000;
    clips.bodies[a][10] = 6'000;
    clips.Rebind(a);
    MusicPcmStream stream(clips.clips);
    std::array<float, 8> output{};
    Check(stream.SetSelection(Selection(MusicCue::A, 1u)) == MusicPcmStatus::Ok,
          "A starts for volume and lifecycle checks");
    Check(stream.SetVolumePercent(50.0f), "music gain accepts a separate percent value");
    Check(stream.Render(std::span<float>(output.data(), 2u)) == MusicPcmStatus::Ok &&
          Near(output[0], 15'000.0f / 65'536.0f),
          "music volume scales samples independently");

    auto paused = Selection(MusicCue::A, 1u, 8.0 / 48'000.0);
    paused.suspended = true;
    Check(stream.SetSelection(paused) == MusicPcmStatus::Suspended,
          "suspension is accepted without a selection revision");
    output.fill(1.0f);
    Check(stream.Render(std::span<float>(output.data(), 2u)) == MusicPcmStatus::Suspended &&
          output[0] == 0.0f && output[1] == 0.0f,
          "suspended render is silent and does not advance");
    auto resumed = Selection(MusicCue::A, 1u, 0.0);
    Check(stream.SetSelection(resumed) == MusicPcmStatus::Ok,
          "resume keeps same-revision cursor rather than seeking");
    Check(stream.Render(std::span<float>(output.data(), 2u)) == MusicPcmStatus::Ok &&
          Near(output[0], 2'000.0f / 65'536.0f),
          "first resumed sample is the next unfrozen source frame");

    auto invalidClock = Selection(MusicCue::A, 1u);
    invalidClock.clockValid = false;
    Check(stream.SetSelection(invalidClock) == MusicPcmStatus::ClockInvalid,
          "invalid clock fails closed");
    output.fill(1.0f);
    Check(stream.Render(std::span<float>(output.data(), 2u)) == MusicPcmStatus::ClockInvalid &&
          output[0] == 0.0f && output[1] == 0.0f,
          "invalid clock produces silence and freezes cursor");
    Check(stream.SetSelection(Selection(MusicCue::A, 1u)) == MusicPcmStatus::Ok,
          "valid clock resumes the same stream");
    Check(stream.Render(std::span<float>(output.data(), 2u)) == MusicPcmStatus::Ok &&
          Near(output[0], 3'000.0f / 65'536.0f),
          "invalid clock interval did not advance source position");

    Check(stream.SetVolumePercent(125.0f), "volume above range clamps");
    Check(stream.SetSelection(Selection(MusicCue::A, 2u, 5.0 / 48'000.0)) ==
              MusicPcmStatus::Ok,
          "new revision seeks to its validated source position");
    Check(stream.Render(std::span<float>(output.data(), 2u)) == MusicPcmStatus::Ok &&
          Near(output[0], 6'000.0f / 32768.0f),
          "seek and clamped full music volume are applied");

    Check(stream.SetSelection(Selection(MusicCue::B, 3u)) == MusicPcmStatus::Ok,
          "normal cue change starts bounded crossfade");
    std::array<float, 100> firstFade{};
    Check(stream.Render(firstFade) == MusicPcmStatus::Ok,
          "first crossfade segment renders");
    Check(stream.SetSelection(Selection(MusicCue::B, 5u, 10.0 / 48'000.0)) ==
              MusicPcmStatus::Ok,
          "same-cue discontinuity starts a validated seek");
    Check(stream.Render(std::span<float>(output.data(), 2u)) == MusicPcmStatus::Ok &&
          Near(output[0], 3'000.0f / 32768.0f),
          "same-cue discontinuity cancels stale outgoing audio and seeks exactly");
    Check(stream.SetSelection(Selection(MusicCue::B, 6u)) == MusicPcmStatus::Ok,
          "a new cue revision can follow the seek");
    Check(stream.Render(firstFade) == MusicPcmStatus::Ok,
          "post-seek cue transition is still bounded");
    Check(stream.SetSelection(Selection(MusicCue::E, 7u)) == MusicPcmStatus::Ok,
          "rapid change replaces the single outgoing stream");
    std::vector<float> fade(kMusicPcmCrossfadeFrames * 2u);
    Check(stream.Render(fade) == MusicPcmStatus::Ok,
          "replacement crossfade remains bounded to two streams");
    Check(Near(fade[fade.size() - 2u], 6'000.0f / 32768.0f) &&
          Near(fade[fade.size() - 1u], 6'000.0f / 32768.0f),
          "rapid replacement leaves only the latest cue with no stale outgoing tail");

    Check(stream.SetVolumePercent(0.0f), "zero music volume is accepted");
    Check(stream.Render(std::span<float>(output.data(), 2u)) == MusicPcmStatus::Ok &&
          output[0] == 0.0f && output[1] == 0.0f,
          "music mute does not affect any event-time SFX system");
    stream.Reset();
    output.fill(1.0f);
    Check(stream.Render(std::span<float>(output.data(), 2u)) == MusicPcmStatus::InvalidSelection &&
          output[0] == 0.0f && output[1] == 0.0f,
          "Reset cancels current/outgoing audio and prevents stale tails");
    Check(stream.SetSelection(Selection(MusicCue::A, 1u)) == MusicPcmStatus::Ok &&
          stream.Render(std::span<float>(output.data(), 2u)) == MusicPcmStatus::Ok &&
          output[0] == 0.0f && output[1] == 0.0f,
          "Reset preserves the separately configured music volume");
}

} // namespace

int main()
{
    TestClipAndSelectionValidation();
    TestLoopTailAndTwentyExactPeriods();
    TestOneShotTailsAndAuthoredGSeek();
    TestNaturalTailTransitions();
    TestEarlyOneShotChangesCrossfadeInsteadOfSkippingBody();
    TestDiscontinuityEdgeSeeksExactlyOnce();
    TestVolumeSuspensionSeekAndRapidChanges();
    if (!passed)
    {
        return 1;
    }
    std::cout << "Shared PCM music stream tests passed.\n";
}
