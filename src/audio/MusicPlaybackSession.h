#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <span>

#include "audio/MusicPcmStream.h"

namespace horde::audio
{

// Immutable copy taken by the audio worker, never a simulation-owned span.
struct MusicPlaybackInput
{
    gameplay::simulation::SimulationSnapshot snapshot;
    std::array<gameplay::simulation::GameplayEvent, 128u> events{};
    std::size_t eventCount = 0u;
    std::uint64_t lifetimeToken = 0u;
    std::uint64_t restartEpoch = 0u;
    std::uint64_t overflowCount = 0u;
    bool available = false;
    bool externallySuspended = true;
};

// Transfer only, not an audio callback API. Producer copies before the original
// SFX queue is drained. Latest snapshots may coalesce; ordered event edges may
// not. Take runs on the worker outside Core Render. Overflow is explicit, drops
// only the newest copied event, and must disable/report degraded music playback.
class MusicPlaybackInbox
{
public:
    bool Publish(const gameplay::simulation::SimulationSnapshot& snapshot,
                 std::span<const gameplay::simulation::GameplayEvent> events,
                 std::uint64_t lifetimeToken, std::uint64_t resetToken,
                 bool externallySuspended);
    [[nodiscard]] MusicPlaybackInput Take();

private:
    std::mutex mutex_;
    MusicPlaybackInput pending_;
    std::uint64_t resetToken_ = 0u;
    std::uint64_t lastEventSequence_ = 0u;
};

// One audio-worker owner for Horde cue decisions + Core PCM. GeneratedFrames is
// the content timeline of rendered buffers, NOT a device-played clock. A backend
// must separately track accepted/submitted and device-consumed frames, bound its
// queue, and stop on a failed/partial submission rather than skip content.
// Pause device + retain queued PCM on ordinary suspension; do not Render silence
// to advance the musical clock. Retry/import/lifetime reset requires quiescing
// and discarding old device buffers before using the new restartEpoch.
class MusicPlaybackSession
{
public:
    explicit MusicPlaybackSession(std::span<const MusicPcmClip> clips) noexcept;
    MusicPlaybackSession(const MusicPlaybackSession&) = delete;
    MusicPlaybackSession& operator=(const MusicPlaybackSession&) = delete;
    MusicPlaybackSession(MusicPlaybackSession&&) = delete;
    MusicPlaybackSession& operator=(MusicPlaybackSession&&) = delete;
    [[nodiscard]] MusicPcmStatus Observe(const MusicPlaybackInput& input) noexcept;
    [[nodiscard]] MusicPcmStatus Render(const MusicPlaybackInput& input,
                                       std::span<float> stereoOutput) noexcept;
    [[nodiscard]] std::uint64_t GeneratedFrames() const noexcept { return generatedFrames_; }
    [[nodiscard]] const MusicSelection& Selection() const noexcept { return selection_; }
    [[nodiscard]] bool IsValid() const noexcept { return stream_.IsValid(); }

private:
    MusicDirector director_;
    MusicPcmStream stream_;
    MusicSelection selection_;
    std::uint64_t generatedFrames_ = 0u;
    std::uint64_t restartEpoch_ = 0u;
    bool epochInitialized_ = false;
};

} // namespace horde::audio
