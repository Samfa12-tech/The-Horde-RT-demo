#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>

#include "audio/MusicPcmAssetBank.h"
#include "audio/MusicPlaybackSession.h"

namespace horde::platform::windows
{

using WindowsMusicLogger = void (*)(const std::string& message);

struct WindowsMusicPlaybackStatus
{
    bool workerRunning = false;
    bool assetsReady = false;
    bool backendReady = false;
    bool backendPlaying = false;
    bool disabled = false;
    std::uint32_t queuedBuffers = 0u;
    std::uint64_t restartEpoch = 0u;
    std::uint64_t generatedFrames = 0u;
    std::uint64_t submittedFrames = 0u;
    std::uint64_t samplesPlayed = 0u;
    std::uint64_t stalePublications = 0u;
    std::uint64_t eventOverflowCount = 0u;
    std::int32_t lastBackendError = 0;
    audio::MusicPcmBankStatus bankStatus = audio::MusicPcmBankStatus::InvalidReader;
    audio::MusicPcmStatus streamStatus = audio::MusicPcmStatus::InvalidClips;
};

// Separate native XAudio2 music path. One worker owns the asset bank, Core
// session, engine and voices. Publish is called from the simulation owner
// before its existing SFX event drain; callbacks only return fixed-buffer slots
// and wake that worker. Music errors never alter gameplay or existing SFX.
class WindowsMusicPlayback
{
public:
    WindowsMusicPlayback(std::filesystem::path assetRoot,
                         WindowsMusicLogger logger = nullptr);
    ~WindowsMusicPlayback();

    WindowsMusicPlayback(const WindowsMusicPlayback&) = delete;
    WindowsMusicPlayback& operator=(const WindowsMusicPlayback&) = delete;
    WindowsMusicPlayback(WindowsMusicPlayback&&) = delete;
    WindowsMusicPlayback& operator=(WindowsMusicPlayback&&) = delete;

    [[nodiscard]] bool Publish(
        const gameplay::simulation::SimulationSnapshot& snapshot,
        std::span<const gameplay::simulation::GameplayEvent> events,
        std::uint64_t lifetimeToken,
        std::uint64_t resetToken,
        bool externallySuspended,
        int musicVolumePercent);

    [[nodiscard]] WindowsMusicPlaybackStatus GetStatus() const noexcept;
    void Stop() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace horde::platform::windows
