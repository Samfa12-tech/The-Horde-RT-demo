#include "audio/MusicPlaybackSession.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <iostream>
#include <thread>
#include <vector>

namespace
{
using namespace horde::audio;
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
bool passed = true;
void Check(bool condition, const char* message)
{
    if (!condition) { passed = false; std::cerr << "Music playback session: " << message << '\n'; }
}

struct Clips
{
    std::array<std::vector<std::int16_t>, kMusicPcmCueCount> bodies, tails;
    std::array<MusicPcmClip, kMusicPcmCueCount> clips{};
    Clips()
    {
        for (std::size_t cue = 1u; cue < clips.size(); ++cue)
        {
            bodies[cue].resize(kMusicPcmAssets[cue].bodyFrames * 2u);
            tails[cue].resize(kMusicPcmTailFrames * 2u);
            for (std::size_t i = 0; i < bodies[cue].size(); ++i)
                bodies[cue][i] = static_cast<std::int16_t>(static_cast<int>((i * 17u + cue * 97u) % 2048u) - 1024);
            for (std::size_t i = 0; i < tails[cue].size(); ++i)
                tails[cue][i] = static_cast<std::int16_t>(static_cast<int>((i * 7u + cue * 83u) % 1024u) - 512);
            clips[cue] = {bodies[cue], tails[cue]};
        }
    }
};

MusicPlaybackInput Input()
{
    MusicPlaybackInput input;
    input.available = true;
    input.externallySuspended = false;
    input.lifetimeToken = 42u;
    input.restartEpoch = 1u;
    return input;
}

void TestInboxCopiesOrderingAndRestart()
{
    MusicPlaybackInbox inbox;
    Check(!inbox.Take().available, "empty inbox does not invent gameplay");
    BoundedGameplayEventQueue source;
    SimulationSnapshot snapshot;
    snapshot.tickIndex = 10u;
    source.Push({.type = GameplayEventType::TorchExtinguished});
    Check(inbox.Publish(snapshot, source.Events(), 3u, 0u, false), "current publication accepted");
    Check(source.Size() == 1u && source.Events()[0].sequence == 1u, "music leaves SFX queue untouched");
    source.Clear();
    source.Push({.type = GameplayEventType::EnemyAttackStarted, .source = EntityId::SkeletonB});
    snapshot.tickIndex = 11u;
    Check(inbox.Publish(snapshot, source.Events(), 3u, 0u, true), "latest state coalesces without losing edge");
    inbox.Publish(snapshot, source.Events(), 3u, 0u, true); // duplicate not accumulated
    auto packet = inbox.Take();
    Check(packet.eventCount == 2u && packet.events[0].sequence == 1u && packet.events[1].sequence == 2u,
          "copied edges survive source clear and retain unique order");
    Check(packet.externallySuspended && packet.snapshot.tickIndex == 11u, "coherent newest state/control");
    Check(inbox.Take().eventCount == 0u, "take drains only copied events");
    snapshot.tickIndex = 9u;
    Check(!inbox.Publish(snapshot, {}, 3u, 0u, false), "stale same-session snapshot rejected");
    Check(inbox.Take().snapshot.tickIndex == 11u, "old phase cannot roll back current state");
    source.Clear();
    source.Push({.type = GameplayEventType::PlayerSwing});
    snapshot.tickIndex = 12u;
    inbox.Publish(snapshot, source.Events(), 3u, 0u, false);
    snapshot.retryGeneration = 1u;
    snapshot.tickIndex = 0u;
    inbox.Publish(snapshot, {}, 3u, 0u, false);
    packet = inbox.Take();
    Check(packet.restartEpoch == 2u && packet.eventCount == 0u, "retry drops stale copied cues despite lower tick");
    inbox.Publish(snapshot, source.Events(), 3u, 0u, false);
    Check(inbox.Take().eventCount == 0u, "retry retains same queue sequence high-water mark");
    GameplayEvent newLifetime{.sequence=1u, .type=GameplayEventType::TorchExtinguished};
    inbox.Publish(snapshot, std::span(&newLifetime, 1u), 4u, 0u, false);
    Check(inbox.Take().eventCount == 1u, "new queue lifetime admits restarted sequences");
    inbox.Publish(snapshot, {}, 4u, 7u, false);
    Check(inbox.Take().restartEpoch == 4u, "out-of-band import advances explicit epoch even at same tick");
}

void TestInboxOverflowAndCoherence()
{
    MusicPlaybackInbox inbox;
    SimulationSnapshot snapshot;
    std::array<GameplayEvent, 150u> events{};
    for (std::size_t i = 0; i < events.size(); ++i) events[i].sequence = i + 1u;
    inbox.Publish(snapshot, events, 1u, 0u, false);
    auto packet = inbox.Take();
    Check(packet.eventCount == 128u && packet.overflowCount == 22u && packet.events[127].sequence == 128u,
          "overflow drops newest copied events and is explicitly counted");

    MusicPlaybackInbox threaded;
    std::atomic<bool> complete{false};
    std::thread producer([&] {
        SimulationSnapshot current;
        for (std::uint64_t tick = 1; tick <= 2500u; ++tick)
        {
            current.tickIndex = tick;
            current.playerX = static_cast<float>(tick);
            current.playerZ = -static_cast<float>(tick);
            threaded.Publish(current, {}, 1u, 0u, (tick & 1u) != 0u);
        }
        complete.store(true, std::memory_order_release);
    });
    std::uint64_t lastTick = 0u;
    do
    {
        packet = threaded.Take();
        if (!packet.available) continue;
        Check(packet.snapshot.tickIndex >= lastTick && packet.snapshot.playerX == packet.snapshot.tickIndex &&
              packet.snapshot.playerZ == -packet.snapshot.playerX &&
              packet.externallySuspended == ((packet.snapshot.tickIndex & 1u) != 0u),
              "concurrent publication is coherent and monotonic");
        lastTick = packet.snapshot.tickIndex;
    } while (!complete.load(std::memory_order_acquire));
    producer.join();
    Check(threaded.Take().snapshot.tickIndex == 2500u, "final input not lost");
}

void RenderFrames(MusicPlaybackSession& session, const MusicPlaybackInput& input, std::size_t frames)
{
    std::array<float, 8192u> output{};
    for (std::size_t iteration = 0u; frames != 0u; ++iteration)
    {
        constexpr std::array<std::size_t, 4u> chunks{317u, 4093u, 997u, 601u};
        const auto count = std::min(frames, chunks[iteration % chunks.size()]);
        Check(session.Render(input, std::span(output).first(count * 2u)) == MusicPcmStatus::Ok, "irregular chunk renders");
        frames -= count;
    }
}

void TestPreciseOneShots(const Clips& clips)
{
    auto input = Input();
    input.snapshot.torchFailure.phase = TorchFailurePhase::Falling;
    input.snapshot.torchFailure.heldByPlayer = false;
    input.events[0] = {.sequence=1u, .type=GameplayEventType::TorchExtinguished};
    input.eventCount = 1u;
    MusicPlaybackSession torch(clips.clips);
    RenderFrames(torch, input, 143999u);
    Check(torch.Selection().cue == MusicCue::C, "C is not ended one frame early");
    std::array<float, 4u> seam{};
    Check(torch.Render(input, seam) == MusicPcmStatus::Ok, "one buffer straddles exact C-to-D boundary");
    Check(torch.Selection().cue == MusicCue::D && torch.GeneratedFrames() == 144001u,
          "C hands off exactly at144000 without any new render/gameplay publication");
    Check(seam[0] == clips.bodies[3][143999u*2u] / 32768.0f &&
          seam[2] == (clips.bodies[4][0] + clips.tails[3][0]) / 32768.0f,
          "last C body sample then first D plus natural C tail, no gap/early tail");

    input = Input();
    input.snapshot.finale.phase = interactions::FinaleSequencePhase::SkylightOpening;
    MusicPlaybackSession ending(clips.clips);
    RenderFrames(ending, input, 287999u);
    Check(ending.Selection().cue == MusicCue::G, "G is not ended one frame early");
    Check(ending.Render(input, seam) == MusicPcmStatus::Ok, "one buffer straddles exact G-to-H boundary");
    Check(ending.Selection().cue == MusicCue::H && ending.GeneratedFrames() == 288001u,
          "G ends exactly at288000 on audio timeline despite stale gameplay phase");
    Check(seam[0] == clips.bodies[7][287999u*2u] / 32768.0f &&
          seam[2] == (clips.bodies[8][0] + clips.tails[7][0]) / 32768.0f,
          "last G body sample then H plus G tail");
    RenderFrames(ending, input, 10000u);
    Check(ending.Selection().cue == MusicCue::H, "stale SkylightOpening does not replay G");
}

void TestLoopsPauseAndReset(const Clips& clips)
{
    auto input = Input();
    MusicPlaybackSession session(clips.clips);
    MusicPcmStream reference(clips.clips);
    Check(reference.SetSelection({.cue=MusicCue::A, .looping=true, .revision=1u}) == MusicPcmStatus::Ok,
          "reference Core stream selected");
    std::array<float, 8192u> actual{}, expected{};
    std::size_t remaining = 20u * 576000u;
    bool equal = true;
    for (std::size_t iteration = 0u; remaining != 0u; ++iteration)
    {
        const auto count = std::min<std::size_t>(remaining, iteration % 2u == 0u ? 317u : 4093u);
        auto output = std::span(actual).first(count * 2u);
        auto control = std::span(expected).first(count * 2u);
        Check(session.Render(input, output) == MusicPcmStatus::Ok && reference.Render(control) == MusicPcmStatus::Ok,
              "twenty actual PCM periods render");
        equal = equal && std::equal(output.begin(), output.end(), control.begin());
        remaining -= count;
    }
    Check(equal && session.GeneratedFrames() == 20u * 576000u,
          "session adds no PCM changes/drift across twenty irregular Core loop periods");
    input.externallySuspended = true;
    Check(session.Render(input, actual) == MusicPcmStatus::Suspended &&
          std::all_of(actual.begin(), actual.end(), [](float v) { return v == 0.0f; }) &&
          session.GeneratedFrames() == 20u * 576000u, "suspend emits no content and does not advance clock");
    input.externallySuspended = false;
    Check(session.Render(input, actual) == MusicPcmStatus::Ok && reference.Render(expected) == MusicPcmStatus::Ok &&
          actual == expected, "ordinary pause retains Core cursor without redecoding or wall-clock jump");
    ++input.restartEpoch;
    Check(session.Render(input, actual) == MusicPcmStatus::Ok && session.GeneratedFrames() == actual.size()/2u,
          "new audio epoch cancels prior cursor/tails and starts fresh timeline");
    input.overflowCount = 1u;
    const auto frames = session.GeneratedFrames();
    Check(session.Render(input, actual) == MusicPcmStatus::InvalidSelection && session.GeneratedFrames() == frames &&
          std::all_of(actual.begin(), actual.end(), [](float v) { return v == 0.0f; }), "degraded input fails explicitly with silence");
    input.overflowCount = 0u;
    input.snapshot.eventQueueOverflowCount = 1u;
    Check(session.Render(input, actual) == MusicPcmStatus::InvalidSelection && session.GeneratedFrames() == frames &&
          std::all_of(actual.begin(), actual.end(), [](float v) { return v == 0.0f; }),
          "original gameplay event overflow also fails explicitly without advancing");
    input.snapshot.eventQueueOverflowCount = 0u;
    Check(session.Render(input, std::span(actual).first(3u)) == MusicPcmStatus::InvalidOutput,
          "odd output span is rejected before advancing");
}
} // namespace

int main()
{
    Clips clips;
    TestInboxCopiesOrderingAndRestart();
    TestInboxOverflowAndCoherence();
    TestPreciseOneShots(clips);
    TestLoopsPauseAndReset(clips);
    return passed ? 0 : 1;
}
