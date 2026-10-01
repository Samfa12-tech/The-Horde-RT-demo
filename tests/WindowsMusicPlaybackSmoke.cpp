#include "platform/windows/WindowsMusicPlayback.h"

#include <chrono>
#include <iostream>
#include <span>
#include <string>
#include <thread>

namespace
{

using horde::gameplay::simulation::SimulationSnapshot;
using horde::platform::windows::WindowsMusicPlayback;
using horde::platform::windows::WindowsMusicPlaybackStatus;

void LogMessage(const std::string& message)
{
    std::cerr << message << '\n';
}

bool WaitFor(const WindowsMusicPlayback& playback,
             const auto predicate,
             const std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (predicate(playback.GetStatus())) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return predicate(playback.GetStatus());
}

bool Check(const bool pass, const char* message)
{
    if (!pass) std::cerr << "Windows music playback smoke failed: " << message << '\n';
    return pass;
}

} // namespace

int wmain(const int argc, wchar_t** argv)
{
    if (argc != 2 && argc != 3)
    {
        std::cerr << "usage: WindowsMusicPlaybackSmoke <assets-root> [1..20 loop periods at silent gain]\n";
        return 2;
    }
    int loopPeriods = 0;
    if (argc == 3)
    {
        try
        {
            std::size_t end = 0u;
            const std::wstring value(argv[2]);
            loopPeriods = std::stoi(value, &end);
            if (end != value.size() || loopPeriods < 1 || loopPeriods > 20) return 2;
        }
        catch (...) { return 2; }
    }
    WindowsMusicPlayback playback(std::filesystem::path(argv[1]), &LogMessage);
    SimulationSnapshot snapshot{};
    snapshot.tickIndex = 1u;
    if (!playback.Publish(snapshot, {}, 1u, 0u, false, 0)) return 3;
    if (!WaitFor(playback, [](const auto& status)
        { return status.assetsReady && status.backendReady && status.samplesPlayed > 0u; },
        std::chrono::seconds(5)))
    {
        const auto status = playback.GetStatus();
        std::cerr << "Windows music playback did not advance silently; disabled="
                  << status.disabled << ", generated=" << status.generatedFrames
                  << ", submitted=" << status.submittedFrames
                  << ", played=" << status.samplesPlayed << '\n';
        return 4;
    }

    if (loopPeriods != 0)
    {
        const std::uint64_t target = static_cast<std::uint64_t>(loopPeriods) * 576000u;
        if (!WaitFor(playback, [target](const auto& status)
            { return status.samplesPlayed >= target; }, std::chrono::seconds(loopPeriods * 12 + 20))) return 14;
        const auto status = playback.GetStatus();
        if (!Check(!status.disabled && status.workerRunning && status.queuedBuffers <= 3u &&
                   status.samplesPlayed <= status.submittedFrames && status.generatedFrames == status.submittedFrames,
                   "long output-clock run violated bounded queue/admission")) return 15;
        std::cout << "Consumed " << loopPeriods << " actual12s PCM body periods: generated=" << status.generatedFrames
                  << " submitted=" << status.submittedFrames << " played=" << status.samplesPlayed
                  << " queuedBuffers=" << status.queuedBuffers << '\n';
    }
    const std::uint64_t beforePause = playback.GetStatus().samplesPlayed;
    std::cout << "Native48k output advanced at gain0: generated=" << playback.GetStatus().generatedFrames
              << " submitted=" << playback.GetStatus().submittedFrames << " played=" << beforePause << '\n';
    snapshot.tickIndex = 2u;
    snapshot.paused = true;
    if (!playback.Publish(snapshot, {}, 1u, 0u, false, 0)) return 5;
    if (!WaitFor(playback, [](const auto& status) { return !status.backendPlaying; }, std::chrono::seconds(2))) return 6;
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    const std::uint64_t pausedAt = playback.GetStatus().samplesPlayed;
    if (!Check(pausedAt >= beforePause, "played-frame counter moved backwards on pause") ||
        !Check(playback.GetStatus().samplesPlayed == pausedAt, "paused source voice consumed queued music")) return 7;
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    const std::uint64_t pausedAgain = playback.GetStatus().samplesPlayed;
    if (!Check(pausedAgain == pausedAt, "paused source voice resumed without a publication")) return 13;
    std::cout << "Pause retained queue; device clock frozen through two150ms intervals at=" << pausedAgain << '\n';

    snapshot.tickIndex = 3u;
    snapshot.paused = false;
    if (!playback.Publish(snapshot, {}, 1u, 0u, false, 0)) return 8;
    if (!WaitFor(playback, [pausedAt](const auto& status)
        { return status.backendPlaying && status.samplesPlayed > pausedAt; }, std::chrono::seconds(2))) return 9;

    snapshot.tickIndex = 4u;
    ++snapshot.retryGeneration;
    if (!playback.Publish(snapshot, {}, 1u, 1u, false, 0)) return 10;
    if (!WaitFor(playback, [](const auto& status)
        { return status.restartEpoch >= 2u && status.backendPlaying && status.samplesPlayed > 0u; },
        std::chrono::seconds(3))) return 11;
    const auto restarted = playback.GetStatus();
    std::cout << "Resume then retry: epoch=" << restarted.restartEpoch << " generated=" << restarted.generatedFrames
              << " submitted=" << restarted.submittedFrames << " played=" << restarted.samplesPlayed << '\n';

    playback.Stop();
    if (!Check(!playback.GetStatus().workerRunning, "Stop joined the worker")) return 12;
    std::cout << "PASS: output clock/pause/resume/retry/join; silent smoke is not owner listening\n";
    return 0;
}
