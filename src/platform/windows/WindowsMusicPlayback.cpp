#include "platform/windows/WindowsMusicPlayback.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <xaudio2.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <exception>
#include <fstream>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

namespace horde::platform::windows
{
namespace
{

constexpr std::size_t kChunkFrames = 480u; // 10 ms at the Core 48 kHz rate.
constexpr std::size_t kChunkSamples = kChunkFrames * 2u;
constexpr std::size_t kChunkCount = 3u; // At most 30 ms queued by this adapter.

std::string HResultMessage(const char* operation, const HRESULT result)
{
    return std::string("Windows music ") + operation + " failed (HRESULT=" +
           std::to_string(static_cast<long>(result)) + ").";
}

} // namespace

class WindowsMusicPlayback::Impl
{
public:
    struct Chunk
    {
        std::array<float, kChunkSamples> samples{};
        std::atomic<bool> available{true};
    };

    class VoiceCallback final : public IXAudio2VoiceCallback
    {
    public:
        explicit VoiceCallback(Impl& owner) : owner_(owner) {}

        void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
        void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
        void STDMETHODCALLTYPE OnStreamEnd() override {}
        void STDMETHODCALLTYPE OnBufferStart(void*) override {}
        void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
        void STDMETHODCALLTYPE OnVoiceError(void*, const HRESULT error) override
        {
            owner_.lastBackendError_.store(static_cast<std::int32_t>(error), std::memory_order_release);
            if (owner_.completionEvent_ != nullptr) SetEvent(owner_.completionEvent_);
        }
        void STDMETHODCALLTYPE OnBufferEnd(void* context) override
        {
            auto* const chunk = static_cast<Chunk*>(context);
            if (chunk != nullptr)
            {
                chunk->available.store(true, std::memory_order_release);
            }
            if (owner_.completionEvent_ != nullptr) SetEvent(owner_.completionEvent_);
        }

    private:
        Impl& owner_;
    };

    Impl(std::filesystem::path assetRoot, const WindowsMusicLogger logger)
        : assetRoot_(std::move(assetRoot)), logger_(logger), callback_(*this)
    {
        stopEvent_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        controlEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        completionEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (stopEvent_ == nullptr || controlEvent_ == nullptr || completionEvent_ == nullptr)
        {
            const DWORD error = GetLastError();
            disabled_.store(true, std::memory_order_release);
            Log("Windows music could not create worker events (Win32 error=" +
                std::to_string(error) + "); music is disabled.");
            PublishStatus();
            return;
        }

        workerRunning_.store(true, std::memory_order_release);
        publishedStatus_.workerRunning = true;
        try
        {
            worker_ = std::thread([this] { WorkerMain(); });
        }
        catch (const std::exception& error)
        {
            workerRunning_.store(false, std::memory_order_release);
            disabled_.store(true, std::memory_order_release);
            Log(std::string("Windows music worker could not start: ") + error.what());
            PublishStatus();
        }
        catch (...)
        {
            workerRunning_.store(false, std::memory_order_release);
            disabled_.store(true, std::memory_order_release);
            Log("Windows music worker could not start; music is disabled.");
            PublishStatus();
        }
    }

    ~Impl()
    {
        Stop();
        if (completionEvent_ != nullptr) CloseHandle(completionEvent_);
        if (controlEvent_ != nullptr) CloseHandle(controlEvent_);
        if (stopEvent_ != nullptr) CloseHandle(stopEvent_);
    }

    bool Publish(const gameplay::simulation::SimulationSnapshot& snapshot,
                 const std::span<const gameplay::simulation::GameplayEvent> events,
                 const std::uint64_t lifetimeToken,
                 const std::uint64_t resetToken,
                 const bool externallySuspended,
                 const int musicVolumePercent)
    {
        if (disabled_.load(std::memory_order_acquire) || stopRequested_.load(std::memory_order_acquire))
        {
            return false;
        }
        const int clampedVolume = std::clamp(musicVolumePercent, 0, 100);

        bool accepted = false;
        try
        {
            accepted = inbox_.Publish(snapshot, events, lifetimeToken, resetToken, externallySuspended);
        }
        catch (...)
        {
            DisableFromProducer("Windows music input publication failed; music is disabled.");
            return false;
        }
        if (!accepted)
        {
            stalePublications_.fetch_add(1u, std::memory_order_relaxed);
            staleLogPending_.store(true, std::memory_order_release);
            if (controlEvent_ != nullptr) SetEvent(controlEvent_);
            return false;
        }
        if (clampedVolume != musicVolumePercent)
        {
            volumeClampLogPending_.store(true, std::memory_order_release);
        }
        requestedVolumePercent_.store(clampedVolume, std::memory_order_release);
        if (controlEvent_ == nullptr || SetEvent(controlEvent_) == FALSE)
        {
            DisableFromProducer("Windows music could not signal its worker; music is disabled.");
            return false;
        }
        return true;
    }

    WindowsMusicPlaybackStatus GetStatus() const noexcept
    {
        const std::lock_guard lock(statusMutex_);
        return publishedStatus_;
    }

private:
    friend class WindowsMusicPlayback;
    void PublishStatus() noexcept
    {
        const WindowsMusicPlaybackStatus status{
            .workerRunning = workerRunning_.load(std::memory_order_acquire),
            .assetsReady = assetsReady_.load(std::memory_order_acquire),
            .backendReady = backendReady_.load(std::memory_order_acquire),
            .backendPlaying = backendPlaying_.load(std::memory_order_acquire),
            .disabled = disabled_.load(std::memory_order_acquire),
            .queuedBuffers = queuedBuffers_,
            .restartEpoch = restartEpoch_.load(std::memory_order_acquire),
            .generatedFrames = generatedFrames_.load(std::memory_order_acquire),
            .submittedFrames = submittedFrames_.load(std::memory_order_acquire),
            .samplesPlayed = samplesPlayed_.load(std::memory_order_acquire),
            .stalePublications = stalePublications_.load(std::memory_order_acquire),
            .eventOverflowCount = eventOverflowCount_.load(std::memory_order_acquire),
            .lastBackendError = lastBackendError_.load(std::memory_order_acquire),
            .bankStatus = bankStatus_.load(std::memory_order_acquire),
            .streamStatus = streamStatus_.load(std::memory_order_acquire),
        };
        const std::lock_guard lock(statusMutex_);
        publishedStatus_ = status;
    }

    void Stop() noexcept
    {
        if (stopRequested_.exchange(true, std::memory_order_acq_rel))
        {
            if (worker_.joinable()) worker_.join();
            return;
        }
        if (stopEvent_ != nullptr) SetEvent(stopEvent_);
        if (worker_.joinable()) worker_.join();
    }

    static bool ReadAsset(void* const context,
                         const std::string_view assetPath,
                         const std::size_t maximumBytes,
                         std::vector<std::uint8_t>& output)
    {
        auto& self = *static_cast<Impl*>(context);
        output.clear();
        const std::filesystem::path path = self.assetRoot_ / std::filesystem::path(std::string(assetPath));
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) return false;
        const std::streamoff length = file.tellg();
        if (length < 0 || static_cast<std::uint64_t>(length) > maximumBytes) return false;
        const std::size_t size = static_cast<std::size_t>(length);
        output.resize(size);
        file.seekg(0, std::ios::beg);
        if (size != 0u && !file.read(reinterpret_cast<char*>(output.data()), static_cast<std::streamsize>(size)))
        {
            output.clear();
            return false;
        }
        return true;
    }

    void Log(const std::string& message) const noexcept
    {
        if (logger_ == nullptr) return;
        try { logger_(message); } catch (...) {}
    }

    void DisableFromProducer(const std::string& message) noexcept
    {
        disabled_.store(true, std::memory_order_release);
        Log(message);
        if (stopEvent_ != nullptr) SetEvent(stopEvent_);
    }

    bool InitializeAudio()
    {
        const HRESULT engineResult = XAudio2Create(&engine_, 0u, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(engineResult) || engine_ == nullptr)
        {
            Disable(HResultMessage("XAudio2Create", engineResult));
            return false;
        }
        const HRESULT masteringResult = engine_->CreateMasteringVoice(&masteringVoice_);
        if (FAILED(masteringResult) || masteringVoice_ == nullptr)
        {
            Disable(HResultMessage("CreateMasteringVoice", masteringResult));
            return false;
        }
        return true;
    }

    bool CreateSourceVoice()
    {
        if (engine_ == nullptr) return false;
        WAVEFORMATEX format{};
        format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        format.nChannels = 2u;
        format.nSamplesPerSec = static_cast<DWORD>(audio::kMusicPcmSampleRate);
        format.wBitsPerSample = 32u;
        format.nBlockAlign = static_cast<WORD>(format.nChannels * (format.wBitsPerSample / 8u));
        format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
        const HRESULT result = engine_->CreateSourceVoice(&sourceVoice_, &format, 0u,
            XAUDIO2_DEFAULT_FREQ_RATIO, &callback_);
        if (FAILED(result) || sourceVoice_ == nullptr)
        {
            Disable(HResultMessage("CreateSourceVoice", result));
            return false;
        }
        const float volume = static_cast<float>(requestedVolumePercent_.load(std::memory_order_acquire)) / 100.0f;
        const HRESULT volumeResult = sourceVoice_->SetVolume(volume);
        if (FAILED(volumeResult))
        {
            Disable(HResultMessage("SetVolume", volumeResult));
            return false;
        }
        backendReady_.store(true, std::memory_order_release);
        return true;
    }

    void DestroySourceVoice() noexcept
    {
        if (sourceVoice_ != nullptr)
        {
            sourceVoice_->Stop(0u, XAUDIO2_COMMIT_NOW);
            sourceVoice_->DestroyVoice(); // Quiesces callbacks before slot reuse.
            sourceVoice_ = nullptr;
        }
        for (Chunk& chunk : chunks_) chunk.available.store(true, std::memory_order_release);
        queuedBuffers_ = 0u;
        backendPlaying_.store(false, std::memory_order_release);
        backendReady_.store(false, std::memory_order_release);
    }

    void DestroyAudio() noexcept
    {
        DestroySourceVoice();
        if (masteringVoice_ != nullptr)
        {
            masteringVoice_->DestroyVoice();
            masteringVoice_ = nullptr;
        }
        if (engine_ != nullptr)
        {
            engine_->Release();
            engine_ = nullptr;
        }
    }

    void Disable(const std::string& message) noexcept
    {
        disabled_.store(true, std::memory_order_release);
        Log(message);
        DestroySourceVoice();
    }

    bool ApplyVolume()
    {
        if (sourceVoice_ == nullptr) return true;
        const int percent = requestedVolumePercent_.load(std::memory_order_acquire);
        if (percent == appliedVolumePercent_) return true;
        const HRESULT result = sourceVoice_->SetVolume(static_cast<float>(percent) / 100.0f);
        if (FAILED(result))
        {
            Disable(HResultMessage("SetVolume", result));
            return false;
        }
        appliedVolumePercent_ = percent;
        return true;
    }

    bool SubmitOne(Chunk& chunk, const audio::MusicPlaybackInput& input)
    {
        bool expected = true;
        if (!chunk.available.compare_exchange_strong(expected, false, std::memory_order_acq_rel)) return true;
        const audio::MusicPcmStatus renderStatus = session_->Render(input, chunk.samples);
        streamStatus_.store(renderStatus, std::memory_order_release);
        generatedFrames_.store(session_->GeneratedFrames(), std::memory_order_release);
        if (renderStatus != audio::MusicPcmStatus::Ok)
        {
            chunk.available.store(true, std::memory_order_release);
            Disable("Windows music Core render failed (status=" +
                    std::to_string(static_cast<unsigned>(renderStatus)) + "); music is disabled.");
            return false;
        }

        XAUDIO2_BUFFER buffer{};
        buffer.AudioBytes = static_cast<UINT32>(sizeof(chunk.samples));
        buffer.pAudioData = reinterpret_cast<const BYTE*>(chunk.samples.data());
        buffer.pContext = &chunk;
        // Reserve the accepted-frame slot before SubmitSourceBuffer can make
        // the buffer visible to the audio thread and its completion callback.
        ++queuedBuffers_;
        const HRESULT submitResult = sourceVoice_->SubmitSourceBuffer(&buffer);
        if (FAILED(submitResult))
        {
            --queuedBuffers_;
            chunk.available.store(true, std::memory_order_release);
            Disable(HResultMessage("SubmitSourceBuffer", submitResult));
            return false;
        }
        submittedFrames_.fetch_add(kChunkFrames, std::memory_order_acq_rel);
        return true;
    }

    bool FillQueue(const audio::MusicPlaybackInput& input)
    {
        if (sourceVoice_ == nullptr || !ApplyVolume()) return false;
        for (Chunk& chunk : chunks_)
        {
            if (disabled_.load(std::memory_order_acquire)) return false;
            if (!SubmitOne(chunk, input)) return false;
        }
        if (!backendPlaying_.load(std::memory_order_acquire) &&
            queuedBuffers_ != 0u)
        {
            const HRESULT result = sourceVoice_->Start(0u, XAUDIO2_COMMIT_NOW);
            if (FAILED(result))
            {
                Disable(HResultMessage("Start", result));
                return false;
            }
            backendPlaying_.store(true, std::memory_order_release);
        }
        return true;
    }

    bool SetSuspended(const bool suspended)
    {
        if (sourceVoice_ == nullptr) return false;
        const bool playing = backendPlaying_.load(std::memory_order_acquire);
        if (suspended == playing)
        {
            const HRESULT result = suspended
                ? sourceVoice_->Stop(0u, XAUDIO2_COMMIT_NOW)
                : sourceVoice_->Start(0u, XAUDIO2_COMMIT_NOW);
            if (FAILED(result))
            {
                Disable(HResultMessage(suspended ? "Stop" : "Start", result));
                return false;
            }
            backendPlaying_.store(!suspended, std::memory_order_release);
        }
        return true;
    }

    void UpdateCounters() noexcept
    {
        if (sourceVoice_ == nullptr) return;
        XAUDIO2_VOICE_STATE state{};
        sourceVoice_->GetState(&state, 0u);
        queuedBuffers_ = state.BuffersQueued;
        const std::uint64_t previous = samplesPlayed_.load(std::memory_order_acquire);
        const std::uint64_t accepted = submittedFrames_.load(std::memory_order_acquire);
        if (state.SamplesPlayed < previous || state.SamplesPlayed > accepted)
        {
            Disable("Windows music device frame counter was inconsistent; music is disabled.");
            return;
        }
        samplesPlayed_.store(state.SamplesPlayed, std::memory_order_release);
        // Callback slots are authoritative because XAudio2 may reorder callbacks
        // after a queue flush; this adapter never flushes, but does not infer
        // ownership from BuffersQueued either.
    }

    void WorkerMain() noexcept
    {
        // COM apartments are per-thread, not inherited from the game's owner.
        // Keep it alive until every voice/engine has been released, including
        // early initialization failures.
        const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(comResult))
        {
            Disable(HResultMessage("CoInitializeEx", comResult));
            workerRunning_.store(false, std::memory_order_release);
            PublishStatus();
            return;
        }
        struct ComLifetime { ~ComLifetime() { CoUninitialize(); } } comLifetime;
        try
        {
            const audio::MusicPcmBankResult bankResult = bank_.Load(&ReadAsset, this);
            bankStatus_.store(bankResult.status, std::memory_order_release);
            if (bankResult.status != audio::MusicPcmBankStatus::Loaded)
            {
                Disable(bankResult.assetPath.empty()
                    ? "Windows music assets failed admission; music is disabled."
                    : "Windows music asset failed admission: " + std::string(bankResult.assetPath));
                DestroyAudio();
                workerRunning_.store(false, std::memory_order_release);
                PublishStatus();
                return;
            }
            assetsReady_.store(true, std::memory_order_release);
            session_.emplace(bank_.Clips());
            if (!session_->IsValid() || !InitializeAudio())
            {
                if (!disabled_.load(std::memory_order_acquire)) Disable("Windows music Core stream is invalid; music is disabled.");
                DestroyAudio();
                workerRunning_.store(false, std::memory_order_release);
                PublishStatus();
                return;
            }

            Log("Windows music admitted PCM bank and created XAudio2 master; wallMs=" +
                std::to_string(GetTickCount64()));
            bool firstOutputLogged = false;
            bool epochInitialized = false;
            std::uint64_t activeEpoch = 0u;
            while (!stopRequested_.load(std::memory_order_acquire) &&
                   !disabled_.load(std::memory_order_acquire))
            {
                const HANDLE waits[] = {stopEvent_, controlEvent_, completionEvent_};
                const DWORD waitResult = WaitForMultipleObjects(3u, waits, FALSE, 25u);
                if (waitResult == WAIT_FAILED)
                {
                    Disable("Windows music worker wait failed (Win32 error=" +
                            std::to_string(GetLastError()) + "); music is disabled.");
                    break;
                }
                PublishStatus();
                if (staleLogPending_.exchange(false, std::memory_order_acq_rel))
                {
                    Log("Windows music rejected one or more stale simulation publications.");
                }
                if (volumeClampLogPending_.exchange(false, std::memory_order_acq_rel))
                {
                    Log("Windows music volume outside 0..100 was clamped.");
                }
                if (waitResult == WAIT_OBJECT_0) break;
                const std::int32_t callbackError = lastBackendError_.load(std::memory_order_acquire);
                if (callbackError != 0)
                {
                    Disable(HResultMessage("voice processing", static_cast<HRESULT>(callbackError)));
                    break;
                }

                audio::MusicPlaybackInput input = inbox_.Take();
                if (!input.available) continue;
                eventOverflowCount_.store(std::max(input.overflowCount,
                    input.snapshot.eventQueueOverflowCount), std::memory_order_release);
                if (input.overflowCount != 0u || input.snapshot.eventQueueOverflowCount != 0u)
                {
                    Disable("Windows music input overflowed; one or more semantic events may be missing, music is disabled.");
                    break;
                }
                if (!epochInitialized || input.restartEpoch != activeEpoch)
                {
                    if (epochInitialized) DestroySourceVoice();
                    const audio::MusicPcmStatus observeStatus = session_->Observe(input);
                    streamStatus_.store(observeStatus, std::memory_order_release);
                    if (observeStatus != audio::MusicPcmStatus::Ok &&
                        observeStatus != audio::MusicPcmStatus::Suspended)
                    {
                        Disable("Windows music session rejected a restart publication; music is disabled.");
                        break;
                    }
                    if (!CreateSourceVoice()) break;
                    epochInitialized = true;
                    activeEpoch = input.restartEpoch;
                    restartEpoch_.store(activeEpoch, std::memory_order_release);
                    generatedFrames_.store(session_->GeneratedFrames(), std::memory_order_release);
                    submittedFrames_.store(0u, std::memory_order_release);
                    samplesPlayed_.store(0u, std::memory_order_release);
                    appliedVolumePercent_ = -1;
                }
                else
                {
                    const audio::MusicPcmStatus observeStatus = session_->Observe(input);
                    streamStatus_.store(observeStatus, std::memory_order_release);
                    if (observeStatus != audio::MusicPcmStatus::Ok &&
                        observeStatus != audio::MusicPcmStatus::Suspended)
                    {
                        Disable("Windows music session rejected a simulation publication; music is disabled.");
                        break;
                    }
                }

                const bool suspended = input.externallySuspended || input.snapshot.paused;
                if (suspended)
                {
                    if (!SetSuspended(true)) break;
                }
                else
                {
                    if (!ApplyVolume() || !FillQueue(input)) break;
                }
                UpdateCounters();
                if (!firstOutputLogged && samplesPlayed_.load(std::memory_order_acquire) != 0u)
                {
                    firstOutputLogged = true;
                    Log("Windows music first device output: played=" +
                        std::to_string(samplesPlayed_.load(std::memory_order_acquire)) +
                        " generated=" + std::to_string(generatedFrames_.load(std::memory_order_acquire)) +
                        " submitted=" + std::to_string(submittedFrames_.load(std::memory_order_acquire)) +
                        " volume=" + std::to_string(appliedVolumePercent_) +
                        " wallMs=" + std::to_string(GetTickCount64()));
                }
                PublishStatus();
            }
        }
        catch (const std::exception& error)
        {
            Disable(std::string("Windows music worker exception: ") + error.what());
        }
        catch (...)
        {
            Disable("Windows music worker encountered an unknown exception.");
        }
        DestroyAudio();
        workerRunning_.store(false, std::memory_order_release);
        PublishStatus();
    }

    std::filesystem::path assetRoot_;
    WindowsMusicLogger logger_ = nullptr;
    audio::MusicPlaybackInbox inbox_;
    audio::MusicPcmAssetBank bank_;
    std::optional<audio::MusicPlaybackSession> session_;
    std::array<Chunk, kChunkCount> chunks_{};
    VoiceCallback callback_;
    IXAudio2* engine_ = nullptr;
    IXAudio2MasteringVoice* masteringVoice_ = nullptr;
    IXAudio2SourceVoice* sourceVoice_ = nullptr;
    HANDLE stopEvent_ = nullptr;
    HANDLE controlEvent_ = nullptr;
    HANDLE completionEvent_ = nullptr;
    std::thread worker_;
    std::atomic<int> requestedVolumePercent_{70};
    int appliedVolumePercent_ = -1;
    std::atomic<bool> workerRunning_{false};
    std::atomic<bool> assetsReady_{false};
    std::atomic<bool> backendReady_{false};
    std::atomic<bool> backendPlaying_{false};
    std::atomic<bool> disabled_{false};
    std::atomic<bool> stopRequested_{false};
    std::atomic<bool> staleLogPending_{false};
    std::atomic<bool> volumeClampLogPending_{false};
    std::uint32_t queuedBuffers_ = 0u; // Worker-owned; callbacks only release slots/signal.
    std::atomic<std::uint64_t> restartEpoch_{0u};
    std::atomic<std::uint64_t> generatedFrames_{0u};
    std::atomic<std::uint64_t> submittedFrames_{0u};
    std::atomic<std::uint64_t> samplesPlayed_{0u};
    std::atomic<std::uint64_t> stalePublications_{0u};
    std::atomic<std::uint64_t> eventOverflowCount_{0u};
    std::atomic<std::int32_t> lastBackendError_{0};
    std::atomic<audio::MusicPcmBankStatus> bankStatus_{audio::MusicPcmBankStatus::InvalidReader};
    std::atomic<audio::MusicPcmStatus> streamStatus_{audio::MusicPcmStatus::InvalidClips};
    mutable std::mutex statusMutex_;
    WindowsMusicPlaybackStatus publishedStatus_{};
};

WindowsMusicPlayback::WindowsMusicPlayback(std::filesystem::path assetRoot,
                                           const WindowsMusicLogger logger)
    : impl_(std::make_unique<Impl>(std::move(assetRoot), logger))
{
}

WindowsMusicPlayback::~WindowsMusicPlayback() = default;

bool WindowsMusicPlayback::Publish(
    const gameplay::simulation::SimulationSnapshot& snapshot,
    const std::span<const gameplay::simulation::GameplayEvent> events,
    const std::uint64_t lifetimeToken,
    const std::uint64_t resetToken,
    const bool externallySuspended,
    const int musicVolumePercent)
{
    return impl_ && impl_->Publish(snapshot, events, lifetimeToken, resetToken,
                                   externallySuspended, musicVolumePercent);
}

WindowsMusicPlaybackStatus WindowsMusicPlayback::GetStatus() const noexcept
{
    return impl_ ? impl_->GetStatus() : WindowsMusicPlaybackStatus{.disabled = true};
}

void WindowsMusicPlayback::Stop() noexcept
{
    if (impl_) impl_->Stop();
}

} // namespace horde::platform::windows
