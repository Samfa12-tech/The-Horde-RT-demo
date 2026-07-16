#include "platform/windows/DiagnosticWindow.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <xaudio2.h>
#ifdef DeviceCapabilities
#undef DeviceCapabilities
#endif
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include "ui/DiagnosticOverlay.h"
#include "gameplay/CorridorCollision.h"
#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/SpatialAudio.h"
#include "gameplay/SwordCombat.h"
#include "vulkan/RtCapabilityReport.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/raytracing/PresentableTinyRtScene.h"

namespace
{

constexpr char kWindowClassName[] = "HordeRtDiagnosticWindowClass";
constexpr char kWindowTitle[] = "Horde Lantern RT - Initial Showing Alpha";
constexpr char kReportDirectory[] = "reports";
constexpr char kTextReportFilename[] = "vulkan_capability_report.txt";
constexpr char kJsonReportFilename[] = "vulkan_capability_report.json";
constexpr int kEditControlId = 101;
constexpr int kHudControlId = 102;
constexpr int kPauseTitleId = 103;
constexpr int kResumeButtonId = 104;
constexpr int kRestartButtonId = 105;
constexpr int kControlsButtonId = 106;
constexpr int kSettingsButtonId = 107;
constexpr int kDiagnosticsButtonId = 108;
constexpr int kExitButtonId = 109;
constexpr int kSettingsTitleId = 110;
constexpr int kSfxButtonId = 111;
constexpr int kSensitivityButtonId = 112;
constexpr int kFullscreenButtonId = 113;
constexpr int kSettingsBackButtonId = 114;
constexpr int kRenderScaleLabelId = 115;
constexpr int kRenderScaleSliderId = 116;
constexpr int kMenuPauseId = 2001;
constexpr int kMenuRestartId = 2002;
constexpr int kMenuExitId = 2003;
constexpr int kMenuSfxId = 2010;
constexpr int kMenuSensitivityLowId = 2011;
constexpr int kMenuSensitivityNormalId = 2012;
constexpr int kMenuSensitivityHighId = 2013;
constexpr int kMenuFullscreenId = 2014;
constexpr int kMenuControlsId = 2020;
constexpr int kMenuDiagnosticsId = 2021;
constexpr int kMenuAboutId = 2022;
constexpr int kMenuCreditsId = 2023;
constexpr int kAppIconId = 1;
constexpr UINT kDefaultDpi = 96u;
constexpr char kUiFontProperty[] = "HordeLanternRtUiFont";
constexpr char kMonoFontProperty[] = "HordeLanternRtMonoFont";
// One frame in flight keeps the dynamically refit held-torch TLAS safely synchronized with its host-written instance buffer.
constexpr UINT kMaxFramesInFlight = 1u;
struct VulkanSurfaceContext
{
    HWND windowHandle = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamilyIndex = 0u;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkColorSpaceKHR swapchainColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkExtent2D swapchainExtent{};
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageLayout> swapchainImageLayouts;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> swapchainFramebuffers;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    horde::vulkan::raytracing::PresentableTinyRtScene rtScene;
    bool useRtPath = false;
    bool controlsEnabled = false;
    bool simulationPaused = true;
    bool pauseMenuVisible = true;
    bool settingsVisible = false;
    bool diagnosticsVisible = false;
    bool sfxEnabled = true;
    bool fullscreen = false;
    bool forwardHeld = false;
    bool backwardHeld = false;
    bool leftHeld = false;
    bool rightHeld = false;
    bool mouseLookActive = false;
    bool playerAttackRequested = false;
    POINT lastMousePosition{};
    ULONGLONG lastControlTick = 0u;
    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    float lanternStrength = 1.8f;
    float walkTime = 0.0f;
    float cameraX = 0.0f;
    float cameraZ = 1.85f;
    float walkAmount = 0.0f;
    float playerTravelledThisFrame = 0.0f;
    float frameDeltaSeconds = 1.0f / 60.0f;
    horde::gameplay::TravelFootstepCadence playerFootsteps;
    horde::gameplay::PlayerFootstepCadence enemyFootsteps;
    int playerFootstepVariant = 0;
    int enemyFootstepVariant = 0;
    bool lichDeathCuePlayed = false;
    float outputExposure = 0.62f;
    float mouseSensitivity = 1.0f;
    float renderScale = 1.0f;
    bool renderScaleDirty = false;
    WINDOWPLACEMENT windowedPlacement{sizeof(WINDOWPLACEMENT)};
    horde::gameplay::SwordCombat combat;
    horde::gameplay::CombatSnapshot combatSnapshot;
    horde::gameplay::LanternSequence lanternSequence;
    horde::gameplay::LanternSnapshot lanternSnapshot;
    horde::gameplay::EnemyDirector enemyDirector;
    horde::gameplay::LichEncounter lichEncounter;
    horde::gameplay::EnemyKind activeEnemyKind = horde::gameplay::EnemyKind::Skeleton;
    horde::gameplay::EnemyKind debugEnemyOverride = horde::gameplay::EnemyKind::None;
    uint32_t debugValidationPoint = 0u;
    float lichDamageFlash = 0.0f;
    uint32_t currentFrame = 0u;
};

bool WriteReportFile(const std::filesystem::path& path, const std::string& data);
void ClearDesktopInput(VulkanSurfaceContext& context);

std::filesystem::path ExecutableDirectory()
{
    std::vector<char> path(MAX_PATH);
    for (;;)
    {
        const DWORD length = GetModuleFileNameA(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (length == 0u)
        {
            return std::filesystem::current_path();
        }
        if (length < path.size() - 1u)
        {
            return std::filesystem::path(path.data()).parent_path();
        }
        path.resize(path.size() * 2u);
    }
}

std::filesystem::path ResolveAssetRoot()
{
    const std::filesystem::path packaged = ExecutableDirectory() / "assets";
    if (std::filesystem::exists(packaged))
    {
        return packaged;
    }
    return std::filesystem::path(HORDE_RT_SOURCE_DIR) / "assets";
}

std::filesystem::path SettingsPath()
{
    return ExecutableDirectory() / "HordeLanternRT.settings.ini";
}

void LoadSettings(VulkanSurfaceContext& context)
{
    const std::string path = SettingsPath().string();
    context.sfxEnabled = GetPrivateProfileIntA("audio", "sfx", 1, path.c_str()) != 0;
    const int sensitivity = std::clamp(static_cast<int>(GetPrivateProfileIntA("controls", "lookSensitivity", 100, path.c_str())), 60, 150);
    context.mouseSensitivity = static_cast<float>(sensitivity) / 100.0f;
    const int renderScale = std::clamp(static_cast<int>(GetPrivateProfileIntA("display", "renderScale", 100, path.c_str())), 50, 100);
    context.renderScale = static_cast<float>(renderScale) / 100.0f;
}

void SaveSettings(const VulkanSurfaceContext& context)
{
    const std::string path = SettingsPath().string();
    WritePrivateProfileStringA("audio", "sfx", context.sfxEnabled ? "1" : "0", path.c_str());
    const std::string sensitivity = std::to_string(static_cast<int>(std::round(context.mouseSensitivity * 100.0f)));
    WritePrivateProfileStringA("controls", "lookSensitivity", sensitivity.c_str(), path.c_str());
    const std::string renderScale = std::to_string(static_cast<int>(std::round(context.renderScale * 100.0f)));
    WritePrivateProfileStringA("display", "renderScale", renderScale.c_str(), path.c_str());
}

void LogWindowsAudio(const std::string& message)
{
    static std::mutex logMutex;
    const std::lock_guard<std::mutex> lock(logMutex);
    const std::string debugMessage = "Horde audio: " + message + "\n";
    OutputDebugStringA(debugMessage.c_str());
    const std::filesystem::path reportDirectory = ExecutableDirectory() / kReportDirectory;
    std::error_code error;
    std::filesystem::create_directories(reportDirectory, error);
    std::ofstream log(reportDirectory / "windows_audio.log", std::ios::app);
    if (log)
    {
        log << message << '\n';
    }
}

bool PlayXAudioFile(const std::filesystem::path& path, float leftGain, float rightGain);

void PlaySoundEffect(const VulkanSurfaceContext& context, const char* filename)
{
    if (!context.sfxEnabled)
    {
        return;
    }
    const std::filesystem::path path = ResolveAssetRoot() / "audio/filmcow" / filename;
    if (std::filesystem::exists(path))
    {
        if (!PlayXAudioFile(path, 1.0f, 1.0f))
        {
            PlaySoundA(path.string().c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
        }
    }
    else
    {
        LogWindowsAudio("missing centred SFX asset: " + path.string());
    }
}

void PlayAmbientSoundEffect(const VulkanSurfaceContext& context, const char* filename)
{
    if (!context.sfxEnabled)
    {
        return;
    }
    const std::filesystem::path path = ResolveAssetRoot() / "audio/filmcow" / filename;
    if (std::filesystem::exists(path))
    {
        if (!PlayXAudioFile(path, 1.0f, 1.0f))
        {
            PlaySoundA(path.string().c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT | SND_NOSTOP);
        }
    }
    else
    {
        LogWindowsAudio("missing ambient SFX asset: " + path.string());
    }
}

class PositionalAudioEngine
{
public:
    PositionalAudioEngine()
    {
        const HRESULT engineResult = XAudio2Create(&engine_, 0u, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(engineResult) || engine_ == nullptr)
        {
            LogWindowsAudio("XAudio2Create failed, HRESULT=" + std::to_string(static_cast<long>(engineResult)));
            return;
        }
        const HRESULT masteringResult = engine_->CreateMasteringVoice(&masteringVoice_);
        if (FAILED(masteringResult) || masteringVoice_ == nullptr)
        {
            LogWindowsAudio("CreateMasteringVoice failed, HRESULT=" + std::to_string(static_cast<long>(masteringResult)));
            engine_->Release();
            engine_ = nullptr;
            return;
        }
        XAUDIO2_VOICE_DETAILS details{};
        masteringVoice_->GetVoiceDetails(&details);
        outputChannels_ = std::max(1u, details.InputChannels);
        LogWindowsAudio("XAudio2 ready; output channels=" + std::to_string(outputChannels_) +
                        ", asset root=" + ResolveAssetRoot().string());
    }

    ~PositionalAudioEngine()
    {
        for (ActiveVoice& active : activeVoices_)
        {
            active.voice->DestroyVoice();
        }
        if (masteringVoice_ != nullptr)
        {
            masteringVoice_->DestroyVoice();
        }
        if (engine_ != nullptr)
        {
            engine_->Release();
        }
    }

    PositionalAudioEngine(const PositionalAudioEngine&) = delete;
    PositionalAudioEngine& operator=(const PositionalAudioEngine&) = delete;

    void Update()
    {
        for (auto it = activeVoices_.begin(); it != activeVoices_.end();)
        {
            XAUDIO2_VOICE_STATE state{};
            it->voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
            if (state.BuffersQueued == 0u)
            {
                if (!completedVoiceLogged_)
                {
                    completedVoiceLogged_ = true;
                    LogWindowsAudio("first voice completed: " + it->filename);
                }
                it->voice->DestroyVoice();
                it = activeVoices_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    bool Play(const std::filesystem::path& path, float leftGain, float rightGain)
    {
        if (engine_ == nullptr || masteringVoice_ == nullptr)
        {
            LogFailureOnce("XAudio2 unavailable; falling back for " + path.string());
            return false;
        }
        const std::shared_ptr<const LoadedWave> wave = Load(path);
        if (!wave || wave->format.nChannels != 1u)
        {
            LogFailureOnce("unsupported or unreadable mono WAV: " + path.string());
            return false;
        }

        if (wave->samples.size() > UINT32_MAX)
        {
            LogFailureOnce("WAV exceeds XAudio2 buffer size: " + path.string());
            return false;
        }

        IXAudio2SourceVoice* voice = nullptr;
        const HRESULT sourceResult = engine_->CreateSourceVoice(&voice, &wave->format);
        if (FAILED(sourceResult) || voice == nullptr)
        {
            LogFailureOnce("CreateSourceVoice failed, HRESULT=" +
                           std::to_string(static_cast<long>(sourceResult)) + ": " + path.string());
            return false;
        }

        std::vector<float> matrix(outputChannels_, 0.0f);
        if (outputChannels_ == 1u)
        {
            matrix[0] = std::max(leftGain, rightGain);
        }
        else
        {
            matrix[0] = leftGain;
            matrix[1] = rightGain;
        }
        const HRESULT matrixResult = voice->SetOutputMatrix(masteringVoice_, 1u, outputChannels_, matrix.data());
        if (FAILED(matrixResult))
        {
            LogFailureOnce("SetOutputMatrix failed, HRESULT=" +
                           std::to_string(static_cast<long>(matrixResult)) + ": " + path.string());
            voice->DestroyVoice();
            return false;
        }

        const XAUDIO2_BUFFER buffer{
            0u,
            static_cast<UINT32>(wave->samples.size()),
            wave->samples.data(),
            0u,
            0u,
            0u,
            0u,
            0u,
            nullptr};
        const HRESULT submitResult = voice->SubmitSourceBuffer(&buffer);
        const HRESULT startResult = SUCCEEDED(submitResult) ? voice->Start() : E_FAIL;
        if (FAILED(submitResult) || FAILED(startResult))
        {
            LogFailureOnce("voice submit/start failed, HRESULT=" +
                           std::to_string(static_cast<long>(FAILED(submitResult) ? submitResult : startResult)) +
                           ": " + path.string());
            voice->DestroyVoice();
            return false;
        }
        const std::string filename = path.filename().string();
        activeVoices_.push_back({voice, wave, filename});
        if (!successfulVoiceLogged_)
        {
            successfulVoiceLogged_ = true;
            LogWindowsAudio("first voice started: " + path.filename().string() +
                            ", format=" + std::to_string(wave->format.nSamplesPerSec) + " Hz/" +
                            std::to_string(wave->format.wBitsPerSample) + " bit mono");
        }
        return true;
    }

private:
    struct LoadedWave
    {
        WAVEFORMATEX format{};
        std::vector<BYTE> samples;
    };

    struct ActiveVoice
    {
        IXAudio2SourceVoice* voice = nullptr;
        std::shared_ptr<const LoadedWave> wave;
        std::string filename;
    };

    std::shared_ptr<const LoadedWave> Load(const std::filesystem::path& path)
    {
        const std::string key = path.string();
        if (const auto found = waves_.find(key); found != waves_.end())
        {
            return found->second;
        }

        std::ifstream stream(path, std::ios::binary | std::ios::ate);
        if (!stream)
        {
            return {};
        }
        const std::streamsize size = stream.tellg();
        if (size < 12)
        {
            return {};
        }
        stream.seekg(0, std::ios::beg);
        std::vector<BYTE> fileBytes(static_cast<std::size_t>(size));
        if (!stream.read(reinterpret_cast<char*>(fileBytes.data()), size))
        {
            return {};
        }

        const auto fourCc = [&fileBytes](std::size_t offset, const char* value)
        {
            return offset + 4u <= fileBytes.size() &&
                   std::memcmp(fileBytes.data() + offset, value, 4u) == 0;
        };
        const auto readU32 = [&fileBytes](std::size_t offset)
        {
            uint32_t value = 0u;
            if (offset + sizeof(value) <= fileBytes.size())
            {
                std::memcpy(&value, fileBytes.data() + offset, sizeof(value));
            }
            return value;
        };
        if (!fourCc(0u, "RIFF") || !fourCc(8u, "WAVE"))
        {
            return {};
        }

        auto wave = std::make_shared<LoadedWave>();
        bool hasFormat = false;
        bool hasSamples = false;
        for (std::size_t offset = 12u; offset + 8u <= fileBytes.size();)
        {
            const uint32_t chunkSize = readU32(offset + 4u);
            const std::size_t dataOffset = offset + 8u;
            if (dataOffset + chunkSize > fileBytes.size())
            {
                return {};
            }
            if (fourCc(offset, "fmt ") && chunkSize >= 16u)
            {
                const std::size_t formatBytes = std::min<std::size_t>(chunkSize, sizeof(WAVEFORMATEX));
                std::memcpy(&wave->format, fileBytes.data() + dataOffset, formatBytes);
                if (chunkSize == 16u)
                {
                    wave->format.cbSize = 0u;
                }
                hasFormat = true;
            }
            else if (fourCc(offset, "data"))
            {
                wave->samples.assign(fileBytes.begin() + static_cast<std::ptrdiff_t>(dataOffset),
                                     fileBytes.begin() + static_cast<std::ptrdiff_t>(dataOffset + chunkSize));
                hasSamples = !wave->samples.empty();
            }
            offset = dataOffset + chunkSize + (chunkSize & 1u);
        }

        if (!hasFormat || !hasSamples || wave->format.wFormatTag != WAVE_FORMAT_PCM ||
            wave->format.nChannels == 0u || wave->format.nSamplesPerSec == 0u ||
            wave->format.nBlockAlign == 0u || wave->format.wBitsPerSample == 0u)
        {
            return {};
        }
        waves_.emplace(key, wave);
        return wave;
    }

    void LogFailureOnce(const std::string& message)
    {
        if (message != lastFailure_)
        {
            lastFailure_ = message;
            LogWindowsAudio(message);
        }
    }

    IXAudio2* engine_ = nullptr;
    IXAudio2MasteringVoice* masteringVoice_ = nullptr;
    UINT32 outputChannels_ = 2u;
    std::unordered_map<std::string, std::shared_ptr<const LoadedWave>> waves_;
    std::vector<ActiveVoice> activeVoices_;
    std::string lastFailure_;
    bool successfulVoiceLogged_ = false;
    bool completedVoiceLogged_ = false;
};

PositionalAudioEngine& SpatialAudioEngine()
{
    static PositionalAudioEngine engine;
    return engine;
}

bool PlayXAudioFile(const std::filesystem::path& path, float leftGain, float rightGain)
{
    return SpatialAudioEngine().Play(path,
                                     std::clamp(leftGain, 0.0f, 1.0f),
                                     std::clamp(rightGain, 0.0f, 1.0f));
}

void PlayEnemySoundEffect(const VulkanSurfaceContext& context, const char* filename, float mixGain)
{
    if (!context.sfxEnabled)
    {
        return;
    }
    const float emitterX = context.activeEnemyKind == horde::gameplay::EnemyKind::Lich
        ? context.lichEncounter.Snapshot().x : context.combatSnapshot.enemyX;
    const float emitterZ = context.activeEnemyKind == horde::gameplay::EnemyKind::Lich
        ? context.lichEncounter.Snapshot().z : context.combatSnapshot.enemyZ;
    const horde::gameplay::SpatialAudioGains gains = horde::gameplay::CalculateSpatialAudio(
        {emitterX, emitterZ, mixGain, 1.0f, 14.0f},
        {context.cameraX, context.cameraZ, context.cameraYaw});
    if (gains.left <= 0.0f && gains.right <= 0.0f)
    {
        return;
    }
    const std::filesystem::path path = ResolveAssetRoot() / "audio/filmcow" / filename;
    if (std::filesystem::exists(path))
    {
        if (!PlayXAudioFile(path, gains.left, gains.right))
        {
            PlaySoundA(path.string().c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT | SND_NOSTOP);
        }
    }
    else
    {
        LogWindowsAudio("missing positional SFX asset: " + path.string());
    }
}

void SetControlVisible(HWND window, const int id, const bool visible)
{
    if (HWND control = GetDlgItem(window, id))
    {
        ShowWindow(control, visible ? SW_SHOW : SW_HIDE);
    }
}

void UpdateSettingsLabels(VulkanSurfaceContext& context)
{
    if (HWND sfx = GetDlgItem(context.windowHandle, kSfxButtonId))
    {
        SetWindowTextA(sfx, context.sfxEnabled ? "SOUND EFFECTS: ON" : "SOUND EFFECTS: OFF");
    }
    if (HWND sensitivity = GetDlgItem(context.windowHandle, kSensitivityButtonId))
    {
        const char* value = context.mouseSensitivity < 0.8f ? "LOW" : (context.mouseSensitivity > 1.2f ? "HIGH" : "NORMAL");
        const std::string label = std::string("LOOK SENSITIVITY: ") + value;
        SetWindowTextA(sensitivity, label.c_str());
    }
    if (HWND fullscreen = GetDlgItem(context.windowHandle, kFullscreenButtonId))
    {
        SetWindowTextA(fullscreen, context.fullscreen ? "DISPLAY: FULLSCREEN" : "DISPLAY: WINDOWED");
    }
    if (HWND label = GetDlgItem(context.windowHandle, kRenderScaleLabelId))
    {
        const std::string text = "RENDER RESOLUTION: " + std::to_string(static_cast<int>(std::round(context.renderScale * 100.0f))) + "%";
        SetWindowTextA(label, text.c_str());
    }
    if (HWND slider = GetDlgItem(context.windowHandle, kRenderScaleSliderId))
    {
        SendMessageA(slider, TBM_SETPOS, TRUE, static_cast<LPARAM>(std::round(context.renderScale * 100.0f)));
    }

    HMENU menu = GetMenu(context.windowHandle);
    if (menu)
    {
        CheckMenuItem(menu, kMenuSfxId, MF_BYCOMMAND | (context.sfxEnabled ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(menu, kMenuFullscreenId, MF_BYCOMMAND | (context.fullscreen ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuRadioItem(menu, kMenuSensitivityLowId, kMenuSensitivityHighId,
                           context.mouseSensitivity < 0.8f ? kMenuSensitivityLowId :
                           (context.mouseSensitivity > 1.2f ? kMenuSensitivityHighId : kMenuSensitivityNormalId), MF_BYCOMMAND);
    }
}

void ApplyOverlayState(VulkanSurfaceContext& context)
{
    const bool pauseVisible = context.pauseMenuVisible && !context.settingsVisible && !context.diagnosticsVisible;
    for (const int id : {kPauseTitleId, kResumeButtonId, kRestartButtonId, kControlsButtonId,
                         kSettingsButtonId, kDiagnosticsButtonId, kExitButtonId})
    {
        SetControlVisible(context.windowHandle, id, pauseVisible);
    }
    for (const int id : {kSettingsTitleId, kSfxButtonId, kSensitivityButtonId, kRenderScaleLabelId,
                         kRenderScaleSliderId, kFullscreenButtonId, kSettingsBackButtonId})
    {
        SetControlVisible(context.windowHandle, id, context.settingsVisible);
    }
    SetControlVisible(context.windowHandle, kEditControlId, context.diagnosticsVisible);
    SetControlVisible(context.windowHandle, kHudControlId, !context.diagnosticsVisible);
    context.simulationPaused = pauseVisible || context.settingsVisible || context.diagnosticsVisible;
    if (context.simulationPaused)
    {
        ClearDesktopInput(context);
    }
    if (HMENU menu = GetMenu(context.windowHandle))
    {
        ModifyMenuA(menu, kMenuPauseId, MF_BYCOMMAND | MF_STRING, kMenuPauseId,
                    context.simulationPaused ? "&Resume\tEsc" : "&Pause\tEsc");
        DrawMenuBar(context.windowHandle);
    }
    UpdateSettingsLabels(context);
}

void ShowPauseMenu(VulkanSurfaceContext& context, const bool visible)
{
    context.pauseMenuVisible = visible;
    context.settingsVisible = false;
    context.diagnosticsVisible = false;
    ApplyOverlayState(context);
    if (visible)
    {
        PlaySoundEffect(context, "menu_toggle.wav");
        SetFocus(GetDlgItem(context.windowHandle, kResumeButtonId));
    }
    else
    {
        PlaySoundEffect(context, "ui_back.wav");
        SetFocus(context.windowHandle);
    }
}

void ResetRoute(VulkanSurfaceContext& context)
{
    context.cameraYaw = 0.0f;
    context.cameraPitch = 0.0f;
    context.lanternStrength = 1.8f;
    context.walkTime = 0.0f;
    context.cameraX = 0.0f;
    context.cameraZ = 1.85f;
    context.walkAmount = 0.0f;
    context.playerTravelledThisFrame = 0.0f;
    context.playerFootsteps.Reset();
    context.enemyFootsteps.Reset();
    context.lichDeathCuePlayed = false;
    context.combat = {};
    context.combatSnapshot = {};
    context.lanternSequence.Reset();
    context.lanternSnapshot = {};
    context.enemyDirector.Reset();
    context.lichEncounter.Reset();
    context.activeEnemyKind = horde::gameplay::EnemyKind::Skeleton;
    context.debugEnemyOverride = horde::gameplay::EnemyKind::None;
    context.debugValidationPoint = 0u;
    context.lichDamageFlash = 0.0f;
    context.playerAttackRequested = false;
    ClearDesktopInput(context);
}

void ToggleFullscreen(VulkanSurfaceContext& context)
{
    HWND window = context.windowHandle;
    if (!context.fullscreen)
    {
        context.windowedPlacement.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(window, &context.windowedPlacement);
        MONITORINFO monitor{sizeof(MONITORINFO)};
        if (GetMonitorInfoA(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor))
        {
            SetWindowLongA(window, GWL_STYLE, WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN);
            SetWindowPos(window, HWND_TOP, monitor.rcMonitor.left, monitor.rcMonitor.top,
                         monitor.rcMonitor.right - monitor.rcMonitor.left,
                         monitor.rcMonitor.bottom - monitor.rcMonitor.top,
                         SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
            context.fullscreen = true;
        }
    }
    else
    {
        SetWindowLongA(window, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN);
        SetWindowPlacement(window, &context.windowedPlacement);
        SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
        context.fullscreen = false;
    }
    UpdateSettingsLabels(context);
}

std::string BuildDisplayText(const horde::vulkan::DeviceCapabilities& capabilities)
{
    if (capabilities.rtMode == horde::vulkan::RtMode::Unsupported)
    {
        return horde::ui::BuildUnsupportedDeviceText(capabilities);
    }
    return horde::ui::BuildDiagnosticOverlayText(capabilities);
}

std::string WindowSafeText(const std::string& value)
{
    std::string out;
    out.reserve(value.size());
    for (const char c : value)
    {
        if (c == '\n')
        {
            out += "\r\n";
        }
        else
        {
            out += c;
        }
    }
    return out;
}

std::string MakeWindowTitle(const std::string& diagnosticText)
{
    std::string title = diagnosticText;
    if (title.empty())
    {
        return kWindowTitle;
    }

    const size_t firstNewLine = title.find('\n');
    if (firstNewLine != std::string::npos)
    {
        title = title.substr(0, firstNewLine);
    }

    if (title.size() > 80u)
    {
        title = title.substr(0, 77u) + "...";
    }

    if (title.empty())
    {
        return kWindowTitle;
    }

    return std::string("Horde RT Diagnostic - ") + title;
}

VkClearColorValue ClearColorForMode(const horde::vulkan::RtMode mode)
{
    switch (mode)
    {
    case horde::vulkan::RtMode::RayTracingPipeline:
        return { {0.04f, 0.36f, 0.06f, 1.0f} };
    case horde::vulkan::RtMode::RayQuery:
        return { {0.14f, 0.08f, 0.40f, 1.0f} };
    default:
        return { {0.28f, 0.04f, 0.04f, 1.0f} };
    }
}

bool CreateInstance(VkInstance& instance)
{
    const char* extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    const VkApplicationInfo appInfo{
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "HordeLanternRTDiagnostic",
        VK_MAKE_VERSION(1, 0, 0),
        "horde_rt",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_1};

    const VkInstanceCreateInfo createInfo{
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr,
        0,
        &appInfo,
        0,
        nullptr,
        static_cast<uint32_t>(std::size(extensions)),
        extensions};

    const VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan instance for diagnostic window: VkResult(" << result << ").\n";
        return false;
    }

    return true;
}

bool CreateSurface(VkInstance instance, HWND hwnd, VkSurfaceKHR& surface)
{
    const VkWin32SurfaceCreateInfoKHR createInfo{
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        nullptr,
        0,
        GetModuleHandleA(nullptr),
        hwnd};

    const VkResult result = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan Win32 surface: VkResult(" << result << ").\n";
        return false;
    }

    return true;
}

bool FindGraphicsAndPresentQueueFamily(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t& queueFamilyIndex)
{
    queueFamilyIndex = 0u;
    uint32_t queueFamilyCount = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    if (queueFamilyCount == 0u)
    {
        return false;
    }

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t index = 0u; index < queueFamilyCount; ++index)
    {
        if ((queueFamilies[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0u)
        {
            continue;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, surface, &presentSupport);
        if (presentSupport == VK_TRUE)
        {
            queueFamilyIndex = index;
            return true;
        }
    }

    return false;
}

bool HasDeviceExtension(VkPhysicalDevice physicalDevice, const char* extensionName)
{
    uint32_t extensionCount = 0u;
    if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr) != VK_SUCCESS)
    {
        return false;
    }

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());
    for (const VkExtensionProperties& extension : extensions)
    {
        if (std::string(extension.extensionName) == extensionName)
        {
            return true;
        }
    }
    return false;
}

void ClearDesktopInput(VulkanSurfaceContext& context)
{
    context.forwardHeld = false;
    context.backwardHeld = false;
    context.leftHeld = false;
    context.rightHeld = false;
    context.mouseLookActive = false;
    if (GetCapture() == context.windowHandle)
    {
        ReleaseCapture();
    }
}

void UpdateDesktopSceneControls(VulkanSurfaceContext& context)
{
    const ULONGLONG now = GetTickCount64();
    float deltaSeconds = 1.0f / 60.0f;
    if (context.lastControlTick != 0u)
    {
        deltaSeconds = std::clamp(static_cast<float>(now - context.lastControlTick) / 1000.0f, 0.0f, 0.05f);
    }
    context.lastControlTick = now;
    context.frameDeltaSeconds = deltaSeconds;
    context.playerTravelledThisFrame = 0.0f;
    if (context.simulationPaused)
    {
        context.frameDeltaSeconds = 0.0f;
        context.walkAmount = 0.0f;
        return;
    }
    context.walkTime += deltaSeconds * 2.7f;

    const float forwardAmount = (context.forwardHeld ? 1.0f : 0.0f) - (context.backwardHeld ? 1.0f : 0.0f);
    const float strafeAmount = (context.rightHeld ? 1.0f : 0.0f) - (context.leftHeld ? 1.0f : 0.0f);
    context.walkAmount = std::clamp(std::abs(forwardAmount) + std::abs(strafeAmount), 0.0f, 1.0f);
    if (context.walkAmount <= 0.01f)
    {
        return;
    }

    const float forwardX = std::sin(context.cameraYaw);
    const float forwardZ = -std::cos(context.cameraYaw);
    const float rightX = std::cos(context.cameraYaw);
    const float rightZ = std::sin(context.cameraYaw);
    constexpr float kMoveSpeed = 1.9f;
    const float previousCameraX = context.cameraX;
    const float previousCameraZ = context.cameraZ;
    context.cameraX += (forwardX * forwardAmount + rightX * strafeAmount) * kMoveSpeed * deltaSeconds;
    context.cameraZ += (forwardZ * forwardAmount + rightZ * strafeAmount) * kMoveSpeed * deltaSeconds;
    horde::gameplay::ResolveCorridorPlayerCollision(previousCameraX, previousCameraZ, context.cameraX, context.cameraZ);
    const float travelledX = context.cameraX - previousCameraX;
    const float travelledZ = context.cameraZ - previousCameraZ;
    context.playerTravelledThisFrame = std::sqrt(travelledX * travelledX + travelledZ * travelledZ);
}

bool SetDesktopMovementKey(VulkanSurfaceContext& context, const WPARAM key, const bool held)
{
    switch (key)
    {
    case 'W':
        context.forwardHeld = held;
        return true;
    case 'S':
        context.backwardHeld = held;
        return true;
    case 'A':
        context.leftHeld = held;
        return true;
    case 'D':
        context.rightHeld = held;
        return true;
    default:
        return false;
    }
}

bool CreateLogicalDevice(VkPhysicalDevice physicalDevice,
                         uint32_t graphicsQueueFamilyIndex,
                         const horde::vulkan::DeviceCapabilities& capabilities,
                         VkDevice& device,
                         VkQueue& graphicsQueue)
{
    const float queuePriority = 1.0f;
    const VkDeviceQueueCreateInfo queueCreateInfo{
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        nullptr,
        0,
        graphicsQueueFamilyIndex,
        1u,
        &queuePriority};
    std::vector<const char*> extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const bool enableRayTracing = capabilities.rtMode == horde::vulkan::RtMode::RayTracingPipeline;
    if (enableRayTracing)
    {
        const char* rtExtensions[] = {
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
            VK_KHR_RAY_QUERY_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_KHR_SPIRV_1_4_EXTENSION_NAME,
            VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME};
        for (const char* extension : rtExtensions)
        {
            if (!HasDeviceExtension(physicalDevice, extension))
            {
                std::cerr << "Selected RayTracingPipeline device is missing required extension: " << extension << ".\n";
                return false;
            }
            extensions.push_back(extension);
        }
    }

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    accelerationStructureFeatures.accelerationStructure = enableRayTracing ? VK_TRUE : VK_FALSE;
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    rayTracingPipelineFeatures.rayTracingPipeline = enableRayTracing ? VK_TRUE : VK_FALSE;
    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
    rayQueryFeatures.rayQuery = enableRayTracing ? VK_TRUE : VK_FALSE;
    VkPhysicalDeviceBufferDeviceAddressFeaturesKHR bufferDeviceAddressFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR};
    bufferDeviceAddressFeatures.bufferDeviceAddress = enableRayTracing ? VK_TRUE : VK_FALSE;
    VkPhysicalDeviceFeatures2 features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    VkPhysicalDeviceFeatures supportedCoreFeatures{};
    vkGetPhysicalDeviceFeatures(physicalDevice, &supportedCoreFeatures);
    features2.features.textureCompressionASTC_LDR = supportedCoreFeatures.textureCompressionASTC_LDR;
    features2.pNext = &accelerationStructureFeatures;
    accelerationStructureFeatures.pNext = &rayTracingPipelineFeatures;
    rayTracingPipelineFeatures.pNext = &rayQueryFeatures;
    rayQueryFeatures.pNext = &bufferDeviceAddressFeatures;

    const VkDeviceCreateInfo createInfo{
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        enableRayTracing ? &features2 : nullptr,
        0,
        1u,
        &queueCreateInfo,
        0,
        nullptr,
        static_cast<uint32_t>(extensions.size()),
        extensions.data(),
        nullptr};

    const VkResult createResult = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
    if (createResult != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan device for diagnostic swapchain: VkResult(" << createResult << ").\n";
        return false;
    }

    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0u, &graphicsQueue);
    return graphicsQueue != VK_NULL_HANDLE;
}

VkExtent2D ClampExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t desiredWidth, uint32_t desiredHeight)
{
    if (capabilities.currentExtent.width != UINT32_MAX && capabilities.currentExtent.height != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }

    const VkExtent2D minExtent = capabilities.minImageExtent;
    const VkExtent2D maxExtent = capabilities.maxImageExtent;
    return {
        std::clamp(desiredWidth, minExtent.width, maxExtent.width),
        std::clamp(desiredHeight, minExtent.height, maxExtent.height)};
}

bool CreateSwapchain(VulkanSurfaceContext& ctx, HWND hwnd)
{
    VkSurfaceCapabilitiesKHR capabilities{};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice, ctx.surface, &capabilities) != VK_SUCCESS)
    {
        std::cerr << "Failed to query Vulkan surface capabilities.\n";
        return false;
    }

    uint32_t formatCount = 0u;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physicalDevice, ctx.surface, &formatCount, nullptr) != VK_SUCCESS ||
        formatCount == 0u)
    {
        return false;
    }

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physicalDevice, ctx.surface, &formatCount, formats.data()) != VK_SUCCESS)
    {
        return false;
    }

    uint32_t presentModeCount = 0u;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.physicalDevice, ctx.surface, &presentModeCount, nullptr) != VK_SUCCESS ||
        presentModeCount == 0u)
    {
        return false;
    }

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.physicalDevice, ctx.surface, &presentModeCount, presentModes.data()) != VK_SUCCESS)
    {
        return false;
    }

    VkSurfaceFormatKHR chosenFormat = formats[0];
    for (const VkSurfaceFormatKHR& candidate : formats)
    {
        if (candidate.format == VK_FORMAT_B8G8R8A8_UNORM &&
            candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            chosenFormat = candidate;
            break;
        }
    }

    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const VkPresentModeKHR candidate : presentModes)
    {
        if (candidate == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            chosenPresentMode = candidate;
            break;
        }
    }

    RECT clientRect{};
    GetClientRect(hwnd, &clientRect);
    const uint32_t requestedWidth = static_cast<uint32_t>(std::max<long>(1, clientRect.right - clientRect.left));
    const uint32_t requestedHeight = static_cast<uint32_t>(std::max<long>(1, clientRect.bottom - clientRect.top));
    ctx.swapchainExtent = ClampExtent(capabilities, requestedWidth, requestedHeight);
    ctx.swapchainFormat = chosenFormat.format;
    ctx.swapchainColorSpace = chosenFormat.colorSpace;

    uint32_t imageCount = std::max(2u, capabilities.minImageCount);
    if (capabilities.maxImageCount > 0u && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        nullptr,
        0,
        ctx.surface,
        imageCount,
        ctx.swapchainFormat,
        ctx.swapchainColorSpace,
        ctx.swapchainExtent,
        1u,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        capabilities.currentTransform,
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        chosenPresentMode,
        VK_TRUE,
        VK_NULL_HANDLE};

    if (vkCreateSwapchainKHR(ctx.device, &createInfo, nullptr, &ctx.swapchain) != VK_SUCCESS)
    {
        std::cerr << "Failed to create swapchain.\n";
        return false;
    }

    uint32_t imageArraySize = 0u;
    if (vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &imageArraySize, nullptr) != VK_SUCCESS || imageArraySize == 0u)
    {
        return false;
    }

    ctx.swapchainImages.resize(imageArraySize);
    if (vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &imageArraySize, ctx.swapchainImages.data()) != VK_SUCCESS)
    {
        return false;
    }
    ctx.swapchainImageLayouts.assign(ctx.swapchainImages.size(), VK_IMAGE_LAYOUT_UNDEFINED);

    ctx.swapchainImageViews.resize(ctx.swapchainImages.size());
    for (size_t index = 0u; index < ctx.swapchainImages.size(); ++index)
    {
        const VkImageViewCreateInfo viewCreateInfo{
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            nullptr,
            0,
            ctx.swapchainImages[index],
            VK_IMAGE_VIEW_TYPE_2D,
            ctx.swapchainFormat,
            {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY
            },
            {
                VK_IMAGE_ASPECT_COLOR_BIT,
                0u,
                1u,
                0u,
                1u
            }};
        if (vkCreateImageView(ctx.device, &viewCreateInfo, nullptr, &ctx.swapchainImageViews[index]) != VK_SUCCESS)
        {
            return false;
        }
    }

    const VkAttachmentDescription colorAttachment{
        0,
        ctx.swapchainFormat,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

    const VkAttachmentReference colorAttachmentReference{0u, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    const VkSubpassDescription subpass{
        0,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        0u,
        nullptr,
        1u,
        &colorAttachmentReference,
        nullptr,
        nullptr,
        0u,
        nullptr};

    const VkAttachmentDescription colorAttachmentArray[] = {colorAttachment};
    const VkSubpassDescription subpassArray[] = {subpass};
    const VkSubpassDependency dependency{
        VK_SUBPASS_EXTERNAL,
        0u,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0u,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_DEPENDENCY_BY_REGION_BIT};

    const VkRenderPassCreateInfo renderPassCreateInfo{
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        nullptr,
        0,
        static_cast<uint32_t>(std::size(colorAttachmentArray)),
        colorAttachmentArray,
        1u,
        subpassArray,
        1u,
        &dependency};

    if (vkCreateRenderPass(ctx.device, &renderPassCreateInfo, nullptr, &ctx.renderPass) != VK_SUCCESS)
    {
        std::cerr << "Failed to create render pass.\n";
        return false;
    }

    ctx.swapchainFramebuffers.resize(ctx.swapchainImageViews.size());
    for (size_t index = 0u; index < ctx.swapchainImageViews.size(); ++index)
    {
        VkImageView attachments[] = {ctx.swapchainImageViews[index]};
        const VkFramebufferCreateInfo framebufferCreateInfo{
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            nullptr,
            0,
            ctx.renderPass,
            1u,
            attachments,
            ctx.swapchainExtent.width,
            ctx.swapchainExtent.height,
            1u};
        if (vkCreateFramebuffer(ctx.device, &framebufferCreateInfo, nullptr, &ctx.swapchainFramebuffers[index]) != VK_SUCCESS)
        {
            return false;
        }
    }

    const VkCommandPoolCreateInfo commandPoolCreateInfo{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        ctx.graphicsQueueFamilyIndex};

    if (vkCreateCommandPool(ctx.device, &commandPoolCreateInfo, nullptr, &ctx.commandPool) != VK_SUCCESS)
    {
        return false;
    }

    VkCommandBufferAllocateInfo commandBufferAllocateInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        ctx.commandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        static_cast<uint32_t>(ctx.swapchainImageViews.size())};
    ctx.commandBuffers.resize(ctx.swapchainImageViews.size());
    if (vkAllocateCommandBuffers(ctx.device, &commandBufferAllocateInfo, ctx.commandBuffers.data()) != VK_SUCCESS)
    {
        return false;
    }

    ctx.imageAvailableSemaphores.resize(kMaxFramesInFlight);
    ctx.renderFinishedSemaphores.resize(kMaxFramesInFlight);
    ctx.inFlightFences.resize(kMaxFramesInFlight);
    VkSemaphoreCreateInfo semaphoreCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
    for (UINT i = 0u; i < kMaxFramesInFlight; ++i)
    {
        if (vkCreateSemaphore(ctx.device, &semaphoreCreateInfo, nullptr, &ctx.imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(ctx.device, &semaphoreCreateInfo, nullptr, &ctx.renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(ctx.device, &fenceCreateInfo, nullptr, &ctx.inFlightFences[i]) != VK_SUCCESS)
        {
            return false;
        }
    }

    return true;
}

void ReleaseSwapchainResources(VulkanSurfaceContext& ctx)
{
    if (ctx.device == VK_NULL_HANDLE)
    {
        return;
    }

    vkDeviceWaitIdle(ctx.device);
    ctx.rtScene.Destroy();

    if (ctx.commandPool != VK_NULL_HANDLE)
    {
        if (!ctx.commandBuffers.empty())
        {
            vkFreeCommandBuffers(ctx.device,
                                 ctx.commandPool,
                                 static_cast<uint32_t>(ctx.commandBuffers.size()),
                                 ctx.commandBuffers.data());
            ctx.commandBuffers.clear();
        }
        vkDestroyCommandPool(ctx.device, ctx.commandPool, nullptr);
        ctx.commandPool = VK_NULL_HANDLE;
    }

    for (VkFramebuffer framebuffer : ctx.swapchainFramebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(ctx.device, framebuffer, nullptr);
        }
    }
    ctx.swapchainFramebuffers.clear();

    for (VkImageView imageView : ctx.swapchainImageViews)
    {
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(ctx.device, imageView, nullptr);
        }
    }
    ctx.swapchainImageViews.clear();

    if (ctx.renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(ctx.device, ctx.renderPass, nullptr);
        ctx.renderPass = VK_NULL_HANDLE;
    }

    if (ctx.swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(ctx.device, ctx.swapchain, nullptr);
        ctx.swapchain = VK_NULL_HANDLE;
    }

    for (VkSemaphore semaphore : ctx.imageAvailableSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(ctx.device, semaphore, nullptr);
        }
    }
    for (VkSemaphore semaphore : ctx.renderFinishedSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(ctx.device, semaphore, nullptr);
        }
    }
    for (VkFence fence : ctx.inFlightFences)
    {
        if (fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(ctx.device, fence, nullptr);
        }
    }

    ctx.imageAvailableSemaphores.clear();
    ctx.renderFinishedSemaphores.clear();
    ctx.inFlightFences.clear();
    ctx.swapchainImageLayouts.clear();
    ctx.swapchainImages.clear();
    ctx.currentFrame = 0u;
}

VkExtent2D ScaledRenderExtent(VkExtent2D presentationExtent, float renderScale)
{
    const float scale = std::clamp(renderScale, 0.50f, 1.0f);
    return {
        std::max(1u, static_cast<uint32_t>(std::lround(static_cast<double>(presentationExtent.width) * scale))),
        std::max(1u, static_cast<uint32_t>(std::lround(static_cast<double>(presentationExtent.height) * scale)))};
}

bool InitialiseRtSceneForSwapchain(VulkanSurfaceContext& ctx)
{
    if (!ctx.useRtPath)
    {
        return true;
    }

    const std::filesystem::path assetRoot = ResolveAssetRoot();
    const VkExtent2D renderExtent = ScaledRenderExtent(ctx.swapchainExtent, ctx.renderScale);
    std::string diagnostic;
    if (!ctx.rtScene.Initialise(ctx.instance,
                                ctx.physicalDevice,
                                ctx.device,
                                ctx.graphicsQueue,
                                ctx.commandPool,
                                renderExtent,
                                ctx.swapchainFormat,
                                (assetRoot / "models/enemies/meshy/skeleton_biped_merged_animations_v01.glb").string(),
                                (assetRoot / "models/enemies/meshy/lich_placeholder_merged_animations_v01.glb").string(),
                                (assetRoot / "textures/polyhaven/mobile_1k").string(),
                                (assetRoot / "textures/meshy/lich_placeholder_v01").string(),
                                diagnostic))
    {
        std::cerr << "Failed to initialise presentable RT scene: " << diagnostic << '\n';
        MessageBoxA(ctx.windowHandle,
                    ("The native RT scene could not start.\n\n" + diagnostic +
                     "\n\nKeep the packaged assets folder beside HordeLanternRT.exe. No fallback renderer will be used.").c_str(),
                    "Horde Lantern RT - startup error",
                    MB_OK | MB_ICONERROR);
        return false;
    }
    std::cout << "PBR material encoding: " << ctx.rtScene.MaterialEncoding() << '\n'
              << "RT render scale " << std::round(ctx.renderScale * 100.0f) << "%: "
              << renderExtent.width << 'x' << renderExtent.height << " -> "
              << ctx.swapchainExtent.width << 'x' << ctx.swapchainExtent.height << '\n' << std::flush;

    return true;
}

bool RecreateSwapchain(VulkanSurfaceContext& ctx)
{
    ReleaseSwapchainResources(ctx);
    if (ctx.windowHandle == nullptr)
    {
        return false;
    }

    return CreateSwapchain(ctx, ctx.windowHandle) && InitialiseRtSceneForSwapchain(ctx);
}

void DestroyRenderContext(VulkanSurfaceContext& ctx)
{
    if (ctx.device == VK_NULL_HANDLE)
    {
        if (ctx.surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
        }
        if (ctx.instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(ctx.instance, nullptr);
        }
        return;
    }

    vkDeviceWaitIdle(ctx.device);
    ctx.rtScene.Destroy();

    for (VkFence fence : ctx.inFlightFences)
    {
        if (fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(ctx.device, fence, nullptr);
        }
    }
    for (VkSemaphore semaphore : ctx.imageAvailableSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(ctx.device, semaphore, nullptr);
        }
    }
    for (VkSemaphore semaphore : ctx.renderFinishedSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(ctx.device, semaphore, nullptr);
        }
    }
    for (VkFramebuffer framebuffer : ctx.swapchainFramebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(ctx.device, framebuffer, nullptr);
        }
    }
    for (VkImageView imageView : ctx.swapchainImageViews)
    {
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(ctx.device, imageView, nullptr);
        }
    }
    if (ctx.commandPool != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(ctx.device, ctx.commandPool,
                             static_cast<uint32_t>(ctx.commandBuffers.size()), ctx.commandBuffers.data());
        vkDestroyCommandPool(ctx.device, ctx.commandPool, nullptr);
    }
    if (ctx.renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(ctx.device, ctx.renderPass, nullptr);
    }
    if (ctx.swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(ctx.device, ctx.swapchain, nullptr);
    }
    vkDestroyDevice(ctx.device, nullptr);
    if (ctx.surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
    }
    if (ctx.instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(ctx.instance, nullptr);
    }

    ctx = {};
}

bool RenderFrame(VulkanSurfaceContext& ctx, const VkClearColorValue& clearColor, bool& rtFramePresented)
{
    rtFramePresented = false;
    if (ctx.commandBuffers.empty())
    {
        return false;
    }

    const VkResult waitResult = vkWaitForFences(ctx.device, 1u, &ctx.inFlightFences[ctx.currentFrame], VK_TRUE, UINT64_MAX);
    if (waitResult != VK_SUCCESS && waitResult != VK_TIMEOUT)
    {
        return false;
    }

    uint32_t imageIndex = 0u;
    const VkResult acquireResult = vkAcquireNextImageKHR(
        ctx.device,
        ctx.swapchain,
        UINT64_MAX,
        ctx.imageAvailableSemaphores[ctx.currentFrame],
        VK_NULL_HANDLE,
        &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_SUBOPTIMAL_KHR)
    {
        return RecreateSwapchain(ctx);
    }
    if (acquireResult != VK_SUCCESS)
    {
        return false;
    }

    VkCommandBufferBeginInfo beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr};

    if (vkResetCommandBuffer(ctx.commandBuffers[imageIndex], 0u) != VK_SUCCESS ||
        vkBeginCommandBuffer(ctx.commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
    {
        return false;
    }

    const bool useRtFrame = ctx.useRtPath && ctx.rtScene.IsReady();
    if (useRtFrame)
    {
        SpatialAudioEngine().Update();
        UpdateDesktopSceneControls(ctx);
        ctx.lanternSnapshot = ctx.lanternSequence.Update(
            ctx.simulationPaused ? 0.0f : ctx.frameDeltaSeconds,
            ctx.cameraX,
            ctx.cameraZ,
            ctx.cameraYaw,
            ctx.cameraPitch);
        if (ctx.debugEnemyOverride == horde::gameplay::EnemyKind::None)
        {
            ctx.enemyDirector.Update(ctx.cameraX, ctx.cameraZ);
        }
        else
        {
            ctx.enemyDirector.ForceSelectForDebug(ctx.debugEnemyOverride);
        }
        const horde::gameplay::EnemyRosterSnapshot& roster = ctx.enemyDirector.Snapshot();
        if (roster.selectedEnemy != ctx.activeEnemyKind)
        {
            ctx.activeEnemyKind = roster.selectedEnemy;
            if (ctx.activeEnemyKind == horde::gameplay::EnemyKind::Skeleton)
            {
                ctx.combat = {};
                ctx.combatSnapshot = {};
            }
            else if (ctx.activeEnemyKind == horde::gameplay::EnemyKind::Lich)
            {
                ctx.lichEncounter.Reset();
                ctx.lichDeathCuePlayed = false;
            }
        }
        bool lichHitRequested = false;
        if (ctx.playerAttackRequested)
        {
            ctx.playerAttackRequested = false;
            // The sword is a player action, not a skeleton-only animation. Keep
            // its swing advancing after the lantern drop and through the finale.
            ctx.combat.RequestAttack();
            lichHitRequested = ctx.activeEnemyKind == horde::gameplay::EnemyKind::Lich;
        }
        if (ctx.playerFootsteps.Update(ctx.playerTravelledThisFrame, ctx.walkAmount > 0.01f))
        {
            const char* clip = (ctx.playerFootstepVariant++ & 1) == 0 ? "player_step_1.wav" : "player_step_2.wav";
            PlayAmbientSoundEffect(ctx, clip);
        }
        const horde::gameplay::EnemyAnimation previousAnimation = ctx.combatSnapshot.enemyAnimation;
        ctx.combatSnapshot = ctx.combat.Update(ctx.frameDeltaSeconds, ctx.cameraX, ctx.cameraZ, ctx.cameraYaw);
        const bool finaleActive = horde::gameplay::QueryShowcaseZone(ctx.cameraX, ctx.cameraZ) == horde::gameplay::ShowcaseZone::Finale;
        const horde::gameplay::LichPhase previousLichPhase = ctx.lichEncounter.Snapshot().phase;
        const horde::gameplay::LichSnapshot& lich = ctx.lichEncounter.Update(
            ctx.activeEnemyKind == horde::gameplay::EnemyKind::Lich ? ctx.frameDeltaSeconds : 0.0f,
            ctx.cameraX,
            ctx.cameraZ,
            !horde::gameplay::IsRouteAudioObstructed(ctx.cameraX, ctx.cameraZ,
                                                      ctx.lichEncounter.Snapshot().x, ctx.lichEncounter.Snapshot().z),
            ctx.activeEnemyKind == horde::gameplay::EnemyKind::Lich && finaleActive);
        // Resolve melee after the encounter update. A debug warp or first entry
        // can select and awaken the lich on this same frame; resolving earlier
        // incorrectly discarded that visibly connected opening strike as a hit
        // against the dormant state.
        if (lichHitRequested)
        {
            if (ctx.lichEncounter.TryAcceptPlayerHit(ctx.cameraX, ctx.cameraZ))
            {
                // The hurt recording already contains its own body impact.
                // Do not mask its brief vocal with a second centred fencing hit.
                PlayEnemySoundEffect(ctx, "lich_hurt.wav", 0.95f);
            }
        }
        if (ctx.activeEnemyKind == horde::gameplay::EnemyKind::Lich &&
            previousLichPhase != horde::gameplay::LichPhase::Charging &&
            lich.phase == horde::gameplay::LichPhase::Charging)
        {
            PlayEnemySoundEffect(ctx, "lich_charge.wav", 0.42f);
        }
        ctx.lichDamageFlash = std::max(0.0f, ctx.lichDamageFlash - ctx.frameDeltaSeconds * 2.8f);
        if (lich.damagePulse)
        {
            ctx.lichDamageFlash = 1.0f;
            PlayEnemySoundEffect(ctx, "lich_impact.wav", 0.55f);
        }
        if (ctx.activeEnemyKind == horde::gameplay::EnemyKind::Lich &&
            lich.phase == horde::gameplay::LichPhase::Dead &&
            !ctx.lichDeathCuePlayed)
        {
            ctx.lichDeathCuePlayed = true;
            PlayEnemySoundEffect(ctx, "lich_fall.wav", 0.36f);
        }
        if (ctx.activeEnemyKind == horde::gameplay::EnemyKind::Lich &&
            lich.deathAnimationComplete)
        {
            ctx.enemyDirector.MarkSelectedDead();
        }
        horde::gameplay::CombatSnapshot renderCombat = ctx.combatSnapshot;
        renderCombat.damageFlash = std::max(renderCombat.damageFlash, ctx.lichDamageFlash);
        if (ctx.activeEnemyKind == horde::gameplay::EnemyKind::Skeleton &&
            previousAnimation != horde::gameplay::EnemyAnimation::Dead &&
            ctx.combatSnapshot.enemyAnimation == horde::gameplay::EnemyAnimation::Dead)
        {
            PlaySoundEffect(ctx, "sword_hit_1.wav");
        }
        if (ctx.activeEnemyKind == horde::gameplay::EnemyKind::Skeleton &&
            previousAnimation != horde::gameplay::EnemyAnimation::Attack &&
            ctx.combatSnapshot.enemyAnimation == horde::gameplay::EnemyAnimation::Attack)
        {
            PlayEnemySoundEffect(ctx, "skeleton_attack.wav", 0.85f);
        }
        const bool skeletonWalking = !ctx.simulationPaused &&
                                     ctx.activeEnemyKind == horde::gameplay::EnemyKind::Skeleton &&
                                     ctx.combatSnapshot.enemyAnimation == horde::gameplay::EnemyAnimation::Walking;
        if (ctx.enemyFootsteps.Update(ctx.frameDeltaSeconds, skeletonWalking))
        {
            const char* clip = (ctx.enemyFootstepVariant++ & 1) == 0 ? "skeleton_step_1.wav" : "skeleton_step_2.wav";
            PlayEnemySoundEffect(ctx, clip, 1.0f);
        }
        std::string diagnostic;
        if (!ctx.rtScene.RecordTraceAndCopy(ctx.commandBuffers[imageIndex],
                                            ctx.swapchainImages[imageIndex],
                                            ctx.swapchainImageLayouts[imageIndex],
                                            ctx.swapchainExtent,
                                            ctx.cameraYaw,
                                            ctx.cameraPitch,
                                            ctx.lanternStrength * ctx.lanternSnapshot.flameStrength,
                                            ctx.walkTime,
                                            ctx.cameraX,
                                            ctx.cameraZ,
                                            ctx.walkAmount,
                                            ctx.outputExposure,
                                            renderCombat,
                                            ctx.lanternSnapshot,
                                            roster,
                                            lich,
                                            diagnostic))
        {
            std::cerr << "Failed to record RT frame: " << diagnostic << '\n';
            return false;
        }
    }
    else
    {
        VkClearValue clearValue{};
        clearValue.color.float32[0] = clearColor.float32[0];
        clearValue.color.float32[1] = clearColor.float32[1];
        clearValue.color.float32[2] = clearColor.float32[2];
        clearValue.color.float32[3] = clearColor.float32[3];

        const VkRenderPassBeginInfo renderPassBegin{
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            ctx.renderPass,
            ctx.swapchainFramebuffers[imageIndex],
            {{0, 0}, ctx.swapchainExtent},
            1u,
            &clearValue};
        vkCmdBeginRenderPass(ctx.commandBuffers[imageIndex], &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(ctx.commandBuffers[imageIndex]);
        ctx.swapchainImageLayouts[imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    if (vkEndCommandBuffer(ctx.commandBuffers[imageIndex]) != VK_SUCCESS)
    {
        return false;
    }

    if (vkResetFences(ctx.device, 1u, &ctx.inFlightFences[ctx.currentFrame]) != VK_SUCCESS)
    {
        return false;
    }

    VkPipelineStageFlags waitStages = useRtFrame ? VK_PIPELINE_STAGE_TRANSFER_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        1u,
        &ctx.imageAvailableSemaphores[ctx.currentFrame],
        &waitStages,
        1u,
        &ctx.commandBuffers[imageIndex],
        1u,
        &ctx.renderFinishedSemaphores[ctx.currentFrame]};

    if (vkQueueSubmit(ctx.graphicsQueue, 1u, &submitInfo, ctx.inFlightFences[ctx.currentFrame]) != VK_SUCCESS)
    {
        return false;
    }

    VkPresentInfoKHR presentInfo{
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        nullptr,
        1u,
        &ctx.renderFinishedSemaphores[ctx.currentFrame],
        1u,
        &ctx.swapchain,
        &imageIndex,
        nullptr};
    const VkResult presentResult = vkQueuePresentKHR(ctx.graphicsQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
    {
        return RecreateSwapchain(ctx);
    }
    if (presentResult != VK_SUCCESS)
    {
        return false;
    }

    rtFramePresented = useRtFrame;
    ctx.currentFrame = (ctx.currentFrame + 1u) % kMaxFramesInFlight;
    return true;
}

int RunDiagnosticSwapchainWindow(HWND hWnd,
                                 horde::vulkan::DeviceCapabilities& capabilities,
                                 const std::filesystem::path& textReportPath,
                                 const std::filesystem::path& jsonReportPath)
{
    VulkanSurfaceContext context;
    context.windowHandle = hWnd;
    LoadSettings(context);
    if (!CreateInstance(context.instance))
    {
        return 1;
    }

    if (!CreateSurface(context.instance, hWnd, context.surface))
    {
        DestroyRenderContext(context);
        return 1;
    }

    const uint32_t desiredVendorId = capabilities.identity.vendorId;
    const uint32_t desiredDeviceId = capabilities.identity.deviceId;
    const std::string& desiredDeviceName = capabilities.identity.gpuName;

    uint32_t physicalDeviceCount = 0u;
    if (vkEnumeratePhysicalDevices(context.instance, &physicalDeviceCount, nullptr) != VK_SUCCESS || physicalDeviceCount == 0u)
    {
        std::cerr << "No physical devices found for diagnostic swapchain.\n";
        DestroyRenderContext(context);
        return 1;
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(context.instance, &physicalDeviceCount, physicalDevices.data());

    for (const VkPhysicalDevice candidate : physicalDevices)
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);
        if (properties.vendorID == desiredVendorId &&
            properties.deviceID == desiredDeviceId &&
            desiredDeviceName == properties.deviceName)
        {
            context.physicalDevice = candidate;
            break;
        }
    }

    if (context.physicalDevice == VK_NULL_HANDLE)
    {
        for (const VkPhysicalDevice candidate : physicalDevices)
        {
            uint32_t queueFamilyIndex = 0u;
            if (FindGraphicsAndPresentQueueFamily(candidate, context.surface, queueFamilyIndex))
            {
                context.physicalDevice = candidate;
                break;
            }
        }
    }

    if (context.physicalDevice == VK_NULL_HANDLE)
    {
        DestroyRenderContext(context);
        return 1;
    }

    if (!FindGraphicsAndPresentQueueFamily(context.physicalDevice, context.surface, context.graphicsQueueFamilyIndex))
    {
        DestroyRenderContext(context);
        return 1;
    }

    context.useRtPath = capabilities.rtMode == horde::vulkan::RtMode::RayTracingPipeline;
    if (!CreateLogicalDevice(context.physicalDevice, context.graphicsQueueFamilyIndex, capabilities, context.device, context.graphicsQueue))
    {
        DestroyRenderContext(context);
        return 1;
    }

    if (!CreateSwapchain(context, hWnd))
    {
        DestroyRenderContext(context);
        return 1;
    }
    if (!InitialiseRtSceneForSwapchain(context))
    {
        DestroyRenderContext(context);
        return 1;
    }

    context.controlsEnabled = context.useRtPath && context.rtScene.IsReady();
    if (context.controlsEnabled)
    {
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&context));
        ApplyOverlayState(context);
        SetFocus(GetDlgItem(hWnd, kResumeButtonId));
    }

    const VkClearColorValue clearColor = ClearColorForMode(capabilities.rtMode);
    MSG message{};
    bool running = true;
    bool renderFailed = false;
    std::vector<double> timingSamples;
    timingSamples.reserve(120u);
    std::uint64_t timingWindowIndex = 0u;
    const std::filesystem::path timingEvidencePath = textReportPath.parent_path() / "windows_showcase_timing.csv";
    if (!std::filesystem::exists(timingEvidencePath))
    {
        std::ofstream header(timingEvidencePath, std::ios::binary);
        header << "window,render_scale_percent,zone,median_ms,p95_ms,average_ms,fps_from_median,cap_bound_165\n";
    }
    while (running)
    {
        while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE) != 0)
        {
            if (message.message == WM_QUIT)
            {
                running = false;
                break;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        if (!running)
        {
            break;
        }

        if (context.renderScaleDirty && context.useRtPath)
        {
            context.renderScaleDirty = false;
            timingSamples.clear();
            vkDeviceWaitIdle(context.device);
            context.rtScene.Destroy();
            capabilities.rtScene.presented = false;
            capabilities.rtScene.dispatchWidth = 0u;
            capabilities.rtScene.dispatchHeight = 0u;
            capabilities.performance.internalRenderWidth = 0u;
            capabilities.performance.internalRenderHeight = 0u;
            capabilities.performance.frameTimeMs = 0.0f;
            capabilities.performance.fps = 0.0f;
            if (HWND hud = GetDlgItem(hWnd, kHudControlId))
            {
                SetWindowTextA(hud, "ALPHA 0.1.0  |  APPLYING RT RENDER SCALE...");
            }
            if (!InitialiseRtSceneForSwapchain(context))
            {
                renderFailed = true;
                break;
            }
        }

        const auto frameStart = std::chrono::steady_clock::now();
        bool rtFramePresented = false;
        if (!RenderFrame(context, clearColor, rtFramePresented))
        {
            renderFailed = true;
            MessageBoxA(hWnd,
                        "The native RT render loop stopped unexpectedly. Check the reports folder for diagnostics.",
                        "Horde Lantern RT - renderer stopped",
                        MB_OK | MB_ICONERROR);
            break;
        }
        const auto frameEnd = std::chrono::steady_clock::now();
        timingSamples.push_back(std::chrono::duration<double, std::milli>(frameEnd - frameStart).count());
        if (timingSamples.size() >= 120u)
        {
            std::vector<double> sortedSamples = timingSamples;
            std::sort(sortedSamples.begin(), sortedSamples.end());
            const double medianFrameMs = (sortedSamples[59] + sortedSamples[60]) * 0.5;
            const double p95FrameMs = sortedSamples[113];
            double totalFrameMs = 0.0;
            for (double sample : timingSamples) totalFrameMs += sample;
            const double averageFrameMs = totalFrameMs / static_cast<double>(timingSamples.size());
            capabilities.performance.frameTimeMs = static_cast<float>(medianFrameMs);
            capabilities.performance.fps = medianFrameMs > 0.0
                ? static_cast<float>(1000.0 / medianFrameMs)
                : 0.0f;
            {
                std::ofstream evidence(timingEvidencePath, std::ios::binary | std::ios::app);
                const horde::gameplay::ShowcaseZone zone = horde::gameplay::QueryShowcaseZone(context.cameraX, context.cameraZ);
                const double fps = medianFrameMs > 0.0 ? 1000.0 / medianFrameMs : 0.0;
                evidence << ++timingWindowIndex << ','
                         << static_cast<int>(std::lround(context.renderScale * 100.0f)) << ','
                         << horde::gameplay::ShowcaseZoneName(zone) << ','
                         << medianFrameMs << ',' << p95FrameMs << ',' << averageFrameMs << ',' << fps << ','
                         << (fps >= 160.0 ? "yes" : "no") << '\n';
            }
            auto& timingDiagnostics = capabilities.diagnostics;
            timingDiagnostics.erase(std::remove(timingDiagnostics.begin(), timingDiagnostics.end(),
                                                "FPS / frame time: not measured yet."),
                                    timingDiagnostics.end());
            WriteReportFile(textReportPath, horde::vulkan::BuildCapabilityTextReport(capabilities));
            WriteReportFile(jsonReportPath, horde::vulkan::BuildCapabilityJsonReport(capabilities));
            if (context.diagnosticsVisible)
            {
                if (HWND edit = GetDlgItem(hWnd, kEditControlId))
                {
                    const std::string updatedText = WindowSafeText(BuildDisplayText(capabilities));
                    SetWindowTextA(edit, updatedText.c_str());
                }
            }
            timingSamples.clear();
        }
        if (rtFramePresented && !capabilities.rtScene.presented)
        {
            capabilities.rtScene.presented = true;
            capabilities.rtScene.status = "Presented via swapchain";
            capabilities.rtScene.geometry = "Complete Horde showcase route with sequential animated skeleton and staff-lit lich";
            capabilities.rtScene.dispatchWidth = context.rtScene.DispatchExtent().width;
            capabilities.rtScene.dispatchHeight = context.rtScene.DispatchExtent().height;
            capabilities.performance.internalRenderWidth = capabilities.rtScene.dispatchWidth;
            capabilities.performance.internalRenderHeight = capabilities.rtScene.dispatchHeight;
            auto& presentationDiagnostics = capabilities.diagnostics;
            presentationDiagnostics.erase(std::remove(presentationDiagnostics.begin(), presentationDiagnostics.end(),
                                                       "Internal render resolution: not measured yet."),
                                          presentationDiagnostics.end());
            WriteReportFile(textReportPath, horde::vulkan::BuildCapabilityTextReport(capabilities));
            WriteReportFile(jsonReportPath, horde::vulkan::BuildCapabilityJsonReport(capabilities));
            if (HWND hud = GetDlgItem(hWnd, kHudControlId))
            {
                SetWindowTextA(hud, "ALPHA 0.1.0  |  NATIVE VULKAN HARDWARE RT ACTIVE  |  F1 CONTROLS  |  ESC MENU");
            }
            if (HWND edit = GetDlgItem(hWnd, kEditControlId))
            {
                const std::string updatedText = WindowSafeText(BuildDisplayText(capabilities));
                SetWindowTextA(edit, updatedText.c_str());
            }
        }
    }

    if (IsWindow(hWnd))
    {
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, 0);
    }
    DestroyRenderContext(context);
    return renderFailed ? 1 : (running ? 0 : static_cast<int>(message.wParam));
}

bool WriteReportFile(const std::filesystem::path& path, const std::string& data)
{
    std::ofstream stream(path, std::ios::binary);
    if (!stream.good())
    {
        return false;
    }

    stream << data;
    return stream.good();
}

int ScaleForDpi(HWND window, const int logicalPixels)
{
    const UINT dpi = GetDpiForWindow(window);
    return MulDiv(logicalPixels, static_cast<int>(dpi == 0u ? kDefaultDpi : dpi), static_cast<int>(kDefaultDpi));
}

void ReplaceFontProperty(HWND window, const char* propertyName, HFONT font)
{
    if (HFONT oldFont = reinterpret_cast<HFONT>(GetPropA(window, propertyName)))
    {
        DeleteObject(oldFont);
    }
    SetPropA(window, propertyName, font);
}

void ApplyDpiScaledFonts(HWND window)
{
    HFONT monoFont = CreateFontA(
        ScaleForDpi(window, 18), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FF_MODERN, "Consolas");
    if (!monoFont)
    {
        monoFont = static_cast<HFONT>(GetStockObject(ANSI_FIXED_FONT));
    }

    HFONT uiFont = CreateFontA(
        ScaleForDpi(window, 18), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    if (!uiFont)
    {
        uiFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    }

    if (HWND edit = GetDlgItem(window, kEditControlId))
    {
        SendMessageA(edit, WM_SETFONT, reinterpret_cast<WPARAM>(monoFont), TRUE);
    }
    for (const int id : {kHudControlId, kPauseTitleId, kResumeButtonId, kRestartButtonId,
                         kControlsButtonId, kSettingsButtonId, kDiagnosticsButtonId, kExitButtonId,
                         kSettingsTitleId, kSfxButtonId, kSensitivityButtonId, kRenderScaleLabelId,
                         kRenderScaleSliderId, kFullscreenButtonId, kSettingsBackButtonId})
    {
        if (HWND control = GetDlgItem(window, id))
        {
            SendMessageA(control, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        }
    }

    if (monoFont != GetStockObject(ANSI_FIXED_FONT))
    {
        ReplaceFontProperty(window, kMonoFontProperty, monoFont);
    }
    if (uiFont != GetStockObject(DEFAULT_GUI_FONT))
    {
        ReplaceFontProperty(window, kUiFontProperty, uiFont);
    }
}

void ReleaseDpiScaledFonts(HWND window)
{
    for (const char* propertyName : {kUiFontProperty, kMonoFontProperty})
    {
        if (HFONT font = reinterpret_cast<HFONT>(RemovePropA(window, propertyName)))
        {
            DeleteObject(font);
        }
    }
}

void LayoutOverlayControls(HWND window, const int width, const int height)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    const int inset = ScaleForDpi(window, 16);
    if (HWND edit = GetDlgItem(window, kEditControlId))
    {
        MoveWindow(edit, inset, inset,
                   std::max(ScaleForDpi(window, 100), width - inset * 2),
                   std::max(ScaleForDpi(window, 100), height - inset * 2), TRUE);
    }
    if (HWND hud = GetDlgItem(window, kHudControlId))
    {
        const int hudInset = ScaleForDpi(window, 14);
        MoveWindow(hud, hudInset, hudInset,
                   std::min(ScaleForDpi(window, 650), std::max(ScaleForDpi(window, 260), width - hudInset * 2)),
                   ScaleForDpi(window, 30), TRUE);
    }

    const int buttonWidth = std::min(ScaleForDpi(window, 420),
                                     std::max(ScaleForDpi(window, 220), width - ScaleForDpi(window, 64)));
    const int buttonHeight = ScaleForDpi(window, 38);
    const int gap = ScaleForDpi(window, 8);
    const int titleHeight = ScaleForDpi(window, 48);
    const int titleAdvance = ScaleForDpi(window, 54);
    const int pauseTotal = titleHeight + 6 * buttonHeight + 5 * gap;
    const int pauseX = (width - buttonWidth) / 2;
    int y = std::max(ScaleForDpi(window, 54), (height - pauseTotal) / 2);
    if (HWND title = GetDlgItem(window, kPauseTitleId)) MoveWindow(title, pauseX, y, buttonWidth, titleHeight, TRUE);
    y += titleAdvance;
    for (const int id : {kResumeButtonId, kRestartButtonId, kControlsButtonId, kSettingsButtonId, kDiagnosticsButtonId, kExitButtonId})
    {
        if (HWND control = GetDlgItem(window, id)) MoveWindow(control, pauseX, y, buttonWidth, buttonHeight, TRUE);
        y += buttonHeight + gap;
    }

    const int labelHeight = ScaleForDpi(window, 26);
    const int sliderHeight = ScaleForDpi(window, 38);
    const int settingsTotal = titleHeight + 4 * buttonHeight + labelHeight + sliderHeight + 5 * gap;
    y = std::max(ScaleForDpi(window, 54), (height - settingsTotal) / 2);
    if (HWND title = GetDlgItem(window, kSettingsTitleId)) MoveWindow(title, pauseX, y, buttonWidth, titleHeight, TRUE);
    y += titleAdvance;
    for (const int id : {kSfxButtonId, kSensitivityButtonId})
    {
        if (HWND control = GetDlgItem(window, id)) MoveWindow(control, pauseX, y, buttonWidth, buttonHeight, TRUE);
        y += buttonHeight + gap;
    }
    if (HWND label = GetDlgItem(window, kRenderScaleLabelId)) MoveWindow(label, pauseX, y, buttonWidth, labelHeight, TRUE);
    y += labelHeight;
    if (HWND slider = GetDlgItem(window, kRenderScaleSliderId)) MoveWindow(slider, pauseX, y, buttonWidth, sliderHeight, TRUE);
    y += sliderHeight + gap;
    for (const int id : {kFullscreenButtonId, kSettingsBackButtonId})
    {
        if (HWND control = GetDlgItem(window, id)) MoveWindow(control, pauseX, y, buttonWidth, buttonHeight, TRUE);
        y += buttonHeight + gap;
    }
}

void ShowControlsHelp(HWND window)
{
    MessageBoxA(window,
                "WASD  Move and strafe\n"
                "Left mouse drag  360 camera look\n"
                "Right mouse or Space  Swing sword\n"
                "Esc  Pause / resume\n"
                "R  Restart route\n"
                "F1  Controls\n"
                "F2  RT diagnostics\n"
                "Alt+Enter  Fullscreen",
                "Horde Lantern RT - controls",
                MB_OK | MB_ICONINFORMATION);
}

void ShowCredits(HWND window)
{
    MessageBoxA(window,
                "Environment materials: Poly Haven (CC0).\n"
                "Sound effects: FilmCow Royalty Free Sound Effects Library.\n"
                "Skeleton derivative: original by Hotstrike Studio; texture, rig, and animation processing created with Meshy (CC BY 4.0).\n"
                "Application icon created for this project with OpenAI image generation.\n\n"
                "See ASSET_LICENSES.md beside the demo for source links and full licence details.",
                "Horde Lantern RT - credits and licences",
                MB_OK | MB_ICONINFORMATION);
}

void ToggleDiagnostics(VulkanSurfaceContext& context)
{
    context.diagnosticsVisible = !context.diagnosticsVisible;
    context.settingsVisible = false;
    context.pauseMenuVisible = context.diagnosticsVisible;
    ApplyOverlayState(context);
    PlaySoundEffect(context, context.diagnosticsVisible ? "ui_select.wav" : "ui_back.wav");
    SetFocus(context.diagnosticsVisible ? GetDlgItem(context.windowHandle, kEditControlId) : context.windowHandle);
}

void OpenSettings(VulkanSurfaceContext& context)
{
    context.pauseMenuVisible = true;
    context.settingsVisible = true;
    context.diagnosticsVisible = false;
    ApplyOverlayState(context);
    PlaySoundEffect(context, "ui_select.wav");
    SetFocus(GetDlgItem(context.windowHandle, kSfxButtonId));
}

LRESULT CALLBACK DiagnosticWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* sceneContext = reinterpret_cast<VulkanSurfaceContext*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
    switch (message)
    {
    case WM_HSCROLL:
        if (sceneContext && reinterpret_cast<HWND>(lParam) == GetDlgItem(hWnd, kRenderScaleSliderId))
        {
            const int percentage = std::clamp(static_cast<int>(SendMessageA(reinterpret_cast<HWND>(lParam), TBM_GETPOS, 0, 0)), 50, 100);
            sceneContext->renderScale = static_cast<float>(percentage) / 100.0f;
            UpdateSettingsLabels(*sceneContext);
            if (LOWORD(wParam) != TB_THUMBTRACK)
            {
                sceneContext->renderScaleDirty = true;
                SaveSettings(*sceneContext);
            }
            return 0;
        }
        break;
    case WM_COMMAND:
        if (sceneContext)
        {
            switch (LOWORD(wParam))
            {
            case kResumeButtonId:
            case kMenuPauseId:
                ShowPauseMenu(*sceneContext, !sceneContext->simulationPaused);
                return 0;
            case kRestartButtonId:
            case kMenuRestartId:
                ResetRoute(*sceneContext);
                PlaySoundEffect(*sceneContext, "ui_select.wav");
                ShowPauseMenu(*sceneContext, false);
                return 0;
            case kControlsButtonId:
            case kMenuControlsId:
                PlaySoundEffect(*sceneContext, "ui_select.wav");
                ShowControlsHelp(hWnd);
                return 0;
            case kSettingsButtonId:
                OpenSettings(*sceneContext);
                return 0;
            case kDiagnosticsButtonId:
            case kMenuDiagnosticsId:
                ToggleDiagnostics(*sceneContext);
                return 0;
            case kSfxButtonId:
            case kMenuSfxId:
                sceneContext->sfxEnabled = !sceneContext->sfxEnabled;
                SaveSettings(*sceneContext);
                UpdateSettingsLabels(*sceneContext);
                PlaySoundEffect(*sceneContext, "ui_select.wav");
                return 0;
            case kSensitivityButtonId:
                sceneContext->mouseSensitivity = sceneContext->mouseSensitivity < 0.8f ? 1.0f :
                                                 (sceneContext->mouseSensitivity < 1.2f ? 1.35f : 0.70f);
                SaveSettings(*sceneContext);
                UpdateSettingsLabels(*sceneContext);
                PlaySoundEffect(*sceneContext, "ui_select.wav");
                return 0;
            case kMenuSensitivityLowId:
            case kMenuSensitivityNormalId:
            case kMenuSensitivityHighId:
                sceneContext->mouseSensitivity = LOWORD(wParam) == kMenuSensitivityLowId ? 0.70f :
                                                 (LOWORD(wParam) == kMenuSensitivityHighId ? 1.35f : 1.0f);
                SaveSettings(*sceneContext);
                UpdateSettingsLabels(*sceneContext);
                return 0;
            case kFullscreenButtonId:
            case kMenuFullscreenId:
                ToggleFullscreen(*sceneContext);
                PlaySoundEffect(*sceneContext, "ui_select.wav");
                return 0;
            case kSettingsBackButtonId:
                sceneContext->settingsVisible = false;
                sceneContext->pauseMenuVisible = true;
                ApplyOverlayState(*sceneContext);
                PlaySoundEffect(*sceneContext, "ui_back.wav");
                SetFocus(GetDlgItem(hWnd, kResumeButtonId));
                return 0;
            case kMenuAboutId:
                MessageBoxA(hWnd,
                            "Horde Lantern RT\nInitial Showing Alpha 0.1.0\n\nNative Vulkan hardware ray tracing. RT or nothing.\nA Samfa12 technology demo.",
                            "About Horde Lantern RT",
                            MB_OK | MB_ICONINFORMATION);
                return 0;
            case kMenuCreditsId:
                ShowCredits(hWnd);
                return 0;
            case kExitButtonId:
            case kMenuExitId:
                DestroyWindow(hWnd);
                return 0;
            default:
                break;
            }
        }
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (sceneContext && sceneContext->controlsEnabled)
        {
            if (wParam == VK_ESCAPE)
            {
                ShowPauseMenu(*sceneContext, !sceneContext->simulationPaused);
                return 0;
            }
            if (wParam == VK_F1)
            {
                ShowControlsHelp(hWnd);
                return 0;
            }
            if (wParam == VK_F2)
            {
                ToggleDiagnostics(*sceneContext);
                return 0;
            }
#if defined(_DEBUG)
            if (wParam == VK_F5)
            {
                sceneContext->debugEnemyOverride = sceneContext->debugEnemyOverride == horde::gameplay::EnemyKind::Lich
                    ? horde::gameplay::EnemyKind::Skeleton
                    : horde::gameplay::EnemyKind::Lich;
                return 0;
            }
            if (wParam == VK_F6)
            {
                sceneContext->debugEnemyOverride = horde::gameplay::EnemyKind::None;
                // Place the validation camera inside the real 2 m sword range.
                sceneContext->cameraX = -33.25f;
                sceneContext->cameraZ = -14.25f;
                sceneContext->cameraYaw = 2.52f;
                sceneContext->cameraPitch = 0.0f;
                return 0;
            }
            if (wParam == VK_F7)
            {
                sceneContext->debugEnemyOverride = horde::gameplay::EnemyKind::None;
                sceneContext->lanternSequence.Reset();
                sceneContext->lanternSnapshot = {};
                sceneContext->cameraX = -1.8f;
                sceneContext->cameraZ = -15.2f;
                sceneContext->cameraYaw = -1.57079632679f;
                sceneContext->cameraPitch = -0.08f;
                return 0;
            }
            if (wParam == VK_F8)
            {
                struct ValidationPoint
                {
                    float x;
                    float z;
                    float yaw;
                    float pitch;
                };
                static constexpr ValidationPoint kValidationPoints[] = {
                    {0.0f, 1.85f, 0.0f, -0.05f},
                    {4.2f, -10.0f, 0.0f, -0.04f},
                    {-5.5f, -15.2f, 0.0f, 0.22f},
                    {-27.5f, -15.2f, -1.57079632679f, -0.02f},
                    {-33.7f, -15.2f, 2.52f, 0.0f},
                };
                const ValidationPoint& point = kValidationPoints[sceneContext->debugValidationPoint];
                sceneContext->cameraX = point.x;
                sceneContext->cameraZ = point.z;
                sceneContext->cameraYaw = point.yaw;
                sceneContext->cameraPitch = point.pitch;
                sceneContext->debugValidationPoint =
                    (sceneContext->debugValidationPoint + 1u) %
                    static_cast<uint32_t>(sizeof(kValidationPoints) / sizeof(kValidationPoints[0]));
                return 0;
            }
            if (wParam == VK_F9)
            {
                // Inspect the settled prop from inside the same corridor leg
                // without resetting the already-triggered lantern sequence.
                sceneContext->cameraX = 1.20f;
                sceneContext->cameraZ = -15.20f;
                sceneContext->cameraYaw = -1.57079632679f;
                sceneContext->cameraPitch = -0.32f;
                return 0;
            }
            if (wParam == VK_F10 && (lParam & (1ll << 30)) == 0)
            {
                const char* clip = (sceneContext->playerFootstepVariant++ & 1) == 0
                    ? "player_step_1.wav" : "player_step_2.wav";
                PlayAmbientSoundEffect(*sceneContext, clip);
                return 0;
            }
#endif
            if (wParam == 'R' && !sceneContext->simulationPaused)
            {
                ResetRoute(*sceneContext);
                PlaySoundEffect(*sceneContext, "ui_select.wav");
                return 0;
            }
            if (wParam == VK_RETURN && (GetKeyState(VK_MENU) & 0x8000) != 0)
            {
                ToggleFullscreen(*sceneContext);
                return 0;
            }
            if (!sceneContext->simulationPaused && wParam == VK_SPACE && (lParam & (1ll << 30)) == 0)
            {
                sceneContext->playerAttackRequested = true;
                PlaySoundEffect(*sceneContext, "sword_swing_1.wav");
                return 0;
            }
            if (!sceneContext->simulationPaused && SetDesktopMovementKey(*sceneContext, wParam, true))
            {
                return 0;
            }
        }
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (sceneContext && sceneContext->controlsEnabled && SetDesktopMovementKey(*sceneContext, wParam, false))
        {
            return 0;
        }
        break;
    case WM_LBUTTONDOWN:
        if (sceneContext && sceneContext->controlsEnabled && !sceneContext->simulationPaused)
        {
            SetFocus(hWnd);
            SetCapture(hWnd);
            sceneContext->mouseLookActive = true;
            sceneContext->lastMousePosition = {
                static_cast<LONG>(static_cast<short>(LOWORD(lParam))),
                static_cast<LONG>(static_cast<short>(HIWORD(lParam)))};
            return 0;
        }
        break;
    case WM_RBUTTONDOWN:
        if (sceneContext && sceneContext->controlsEnabled && !sceneContext->simulationPaused)
        {
            sceneContext->playerAttackRequested = true;
            PlaySoundEffect(*sceneContext, "sword_swing_2.wav");
            return 0;
        }
        break;
    case WM_MOUSEMOVE:
        if (sceneContext && sceneContext->controlsEnabled && !sceneContext->simulationPaused && sceneContext->mouseLookActive)
        {
            const POINT currentMousePosition{
                static_cast<LONG>(static_cast<short>(LOWORD(lParam))),
                static_cast<LONG>(static_cast<short>(HIWORD(lParam)))};
            const LONG deltaX = currentMousePosition.x - sceneContext->lastMousePosition.x;
            const LONG deltaY = currentMousePosition.y - sceneContext->lastMousePosition.y;
            sceneContext->lastMousePosition = currentMousePosition;
            sceneContext->cameraYaw += static_cast<float>(deltaX) * 0.0036f * sceneContext->mouseSensitivity;
            sceneContext->cameraPitch = std::clamp(sceneContext->cameraPitch - static_cast<float>(deltaY) * 0.0028f * sceneContext->mouseSensitivity, -0.32f, 0.28f);
            return 0;
        }
        break;
    case WM_LBUTTONUP:
    case WM_CAPTURECHANGED:
        if (sceneContext && sceneContext->controlsEnabled)
        {
            sceneContext->mouseLookActive = false;
            if (GetCapture() == hWnd)
            {
                ReleaseCapture();
            }
            return 0;
        }
        break;
    case WM_KILLFOCUS:
        if (sceneContext && sceneContext->controlsEnabled)
        {
            ClearDesktopInput(*sceneContext);
        }
        break;
    case WM_SIZE:
        LayoutOverlayControls(hWnd, LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_GETMINMAXINFO:
    {
        auto* minMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
        minMaxInfo->ptMinTrackSize.x = ScaleForDpi(hWnd, 520);
        minMaxInfo->ptMinTrackSize.y = ScaleForDpi(hWnd, 560);
        return 0;
    }
    case WM_DPICHANGED:
    {
        const auto* suggested = reinterpret_cast<const RECT*>(lParam);
        SetWindowPos(hWnd, nullptr,
                     suggested->left, suggested->top,
                     suggested->right - suggested->left,
                     suggested->bottom - suggested->top,
                     SWP_NOACTIVATE | SWP_NOZORDER);
        ApplyDpiScaledFonts(hWnd);
        RECT clientRect{};
        GetClientRect(hWnd, &clientRect);
        LayoutOverlayControls(hWnd, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, RGB(255, 208, 122));
        SetBkColor(dc, RGB(13, 11, 9));
        static HBRUSH brush = CreateSolidBrush(RGB(13, 11, 9));
        return reinterpret_cast<LRESULT>(brush);
    }
    case WM_DESTROY:
        if (sceneContext && sceneContext->controlsEnabled)
        {
            ClearDesktopInput(*sceneContext);
        }
        ReleaseDpiScaledFonts(hWnd);
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

HMENU CreateApplicationMenu()
{
    HMENU bar = CreateMenu();
    HMENU demo = CreatePopupMenu();
    AppendMenuA(demo, MF_STRING, kMenuPauseId, "&Pause\tEsc");
    AppendMenuA(demo, MF_STRING, kMenuRestartId, "&Restart route\tR");
    AppendMenuA(demo, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(demo, MF_STRING, kMenuExitId, "E&xit");
    AppendMenuA(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(demo), "&Demo");

    HMENU settings = CreatePopupMenu();
    AppendMenuA(settings, MF_STRING | MF_CHECKED, kMenuSfxId, "Sound &effects");
    HMENU sensitivity = CreatePopupMenu();
    AppendMenuA(sensitivity, MF_STRING, kMenuSensitivityLowId, "Low");
    AppendMenuA(sensitivity, MF_STRING | MF_CHECKED, kMenuSensitivityNormalId, "Normal");
    AppendMenuA(sensitivity, MF_STRING, kMenuSensitivityHighId, "High");
    AppendMenuA(settings, MF_POPUP, reinterpret_cast<UINT_PTR>(sensitivity), "Look &sensitivity");
    AppendMenuA(settings, MF_STRING, kMenuFullscreenId, "&Fullscreen\tAlt+Enter");
    AppendMenuA(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(settings), "&Settings");

    HMENU help = CreatePopupMenu();
    AppendMenuA(help, MF_STRING, kMenuControlsId, "&Controls\tF1");
    AppendMenuA(help, MF_STRING, kMenuDiagnosticsId, "RT &diagnostics\tF2");
    AppendMenuA(help, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(help, MF_STRING, kMenuCreditsId, "&Credits && licences");
    AppendMenuA(help, MF_STRING, kMenuAboutId, "&About");
    AppendMenuA(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(help), "&Help");
    return bar;
}

int CreateAndShowWindow(const std::string& diagnosticText,
                        horde::vulkan::DeviceCapabilities& capabilities,
                        const std::filesystem::path& textReportPath,
                        const std::filesystem::path& jsonReportPath)
{
    const HINSTANCE instance = GetModuleHandleA(nullptr);
    INITCOMMONCONTROLSEX commonControls{sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES};
    InitCommonControlsEx(&commonControls);
    WNDCLASSA windowClass{};
    windowClass.lpfnWndProc = DiagnosticWindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kWindowClassName;
    windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hIcon = LoadIconA(instance, MAKEINTRESOURCEA(kAppIconId));

    if (!RegisterClassA(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        std::cerr << "Failed to register diagnostic window class." << std::endl;
        return 1;
    }

    const UINT systemDpi = GetDpiForSystem();
    HWND hWnd = CreateWindowExA(
        0,
        kWindowClassName,
        kWindowTitle,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        MulDiv(1000, static_cast<int>(systemDpi == 0u ? kDefaultDpi : systemDpi), static_cast<int>(kDefaultDpi)),
        MulDiv(700, static_cast<int>(systemDpi == 0u ? kDefaultDpi : systemDpi), static_cast<int>(kDefaultDpi)),
        nullptr,
        CreateApplicationMenu(),
        instance,
        nullptr);
    if (!hWnd)
    {
        std::cerr << "Failed to create diagnostic window." << std::endl;
        return 1;
    }

    RECT clientRect{};
    GetClientRect(hWnd, &clientRect);
    HWND edit = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        "EDIT",
        "",
        WS_CHILD | WS_VISIBLE | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
        12,
        12,
        clientRect.right - clientRect.left - 24,
        clientRect.bottom - clientRect.top - 24,
        hWnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kEditControlId)),
        instance,
        nullptr);
    if (!edit)
    {
        std::cerr << "Failed to create diagnostic text area." << std::endl;
        return 1;
    }

    auto createStatic = [&](const int id, const char* text, const DWORD style = SS_CENTER) {
        HWND control = CreateWindowExA(0, "STATIC", text, WS_CHILD | WS_VISIBLE | style,
                                       0, 0, 100, 30, hWnd,
                                       reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
        return control;
    };
    auto createButton = [&](const int id, const char* text) {
        HWND control = CreateWindowExA(0, "BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                       0, 0, 100, 38, hWnd,
                                       reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
        return control;
    };

    createStatic(kHudControlId, "ALPHA 0.1.0  |  VULKAN RT STARTING...  |  F1 CONTROLS  |  ESC MENU", SS_LEFT | SS_CENTERIMAGE);
    createStatic(kPauseTitleId, "HORDE LANTERN RT  |  INITIAL SHOWING ALPHA", SS_CENTER | SS_CENTERIMAGE);
    createButton(kResumeButtonId, "ENTER THE RUIN / RESUME");
    createButton(kRestartButtonId, "RESTART ROUTE");
    createButton(kControlsButtonId, "CONTROLS");
    createButton(kSettingsButtonId, "SETTINGS");
    createButton(kDiagnosticsButtonId, "RT DIAGNOSTICS");
    createButton(kExitButtonId, "QUIT DEMO");
    createStatic(kSettingsTitleId, "SETTINGS  |  SAVED BESIDE THE DEMO", SS_CENTER | SS_CENTERIMAGE);
    createButton(kSfxButtonId, "SOUND EFFECTS: ON");
    createButton(kSensitivityButtonId, "LOOK SENSITIVITY: NORMAL");
    createStatic(kRenderScaleLabelId, "RENDER RESOLUTION: 100%", SS_CENTER | SS_CENTERIMAGE);
    HWND renderScaleSlider = CreateWindowExA(0, TRACKBAR_CLASSA, "",
                                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_AUTOTICKS,
                                              0, 0, 100, 38, hWnd,
                                              reinterpret_cast<HMENU>(static_cast<INT_PTR>(kRenderScaleSliderId)), instance, nullptr);
    SendMessageA(renderScaleSlider, TBM_SETRANGE, TRUE, MAKELPARAM(50, 100));
    SendMessageA(renderScaleSlider, TBM_SETTICFREQ, 10, 0);
    SendMessageA(renderScaleSlider, TBM_SETPOS, TRUE, 100);
    createButton(kFullscreenButtonId, "DISPLAY: WINDOWED");
    createButton(kSettingsBackButtonId, "BACK");

    ApplyDpiScaledFonts(hWnd);

    const std::string windowText = WindowSafeText(diagnosticText);
    const bool sceneMode = capabilities.rtMode == horde::vulkan::RtMode::RayTracingPipeline;
    const std::string windowTitle = sceneMode
        ? "Horde Lantern RT - Initial Showing Alpha 0.1.0"
        : MakeWindowTitle(diagnosticText);
    SetWindowTextA(edit, windowText.c_str());
    SetWindowTextA(hWnd, windowTitle.c_str());
    if (sceneMode)
    {
        ShowWindow(edit, SW_HIDE);
    }

    LayoutOverlayControls(hWnd, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    SetForegroundWindow(hWnd);
    SetFocus(sceneMode ? hWnd : edit);

    return RunDiagnosticSwapchainWindow(hWnd, capabilities, textReportPath, jsonReportPath);
}

} // namespace

namespace horde::platform::windows
{

int RunDiagnosticWindow(const int showCommand)
{
    (void)showCommand;
    SetProcessDPIAware();

    horde::vulkan::VulkanContext context;
    const bool initialised = context.InitialiseForCapabilityProbe();
    horde::vulkan::DeviceCapabilities capabilities = context.QueryDeviceCapabilities();

    const std::string diagnosticText = BuildDisplayText(capabilities);
    const std::string textReport = horde::vulkan::BuildCapabilityTextReport(capabilities);
    const std::string jsonReport = horde::vulkan::BuildCapabilityJsonReport(capabilities);

    std::cout << "=== Horde RT Diagnostic Window ===\n";
    std::cout << "Probe initialisation: " << (initialised ? "OK" : "Fallback") << "\n\n";
    std::cout << diagnosticText << "\n\n";

    std::error_code error;
    const std::filesystem::path reportDirectory = ExecutableDirectory() / kReportDirectory;
    std::filesystem::create_directories(reportDirectory, error);
    if (error)
    {
        std::cerr << "Failed to create report directory '" << kReportDirectory << "': " << error.message() << '\n';
        return 1;
    }

    const std::filesystem::path textReportPath = reportDirectory / kTextReportFilename;
    const std::filesystem::path jsonReportPath = reportDirectory / kJsonReportFilename;

    if (!WriteReportFile(textReportPath, textReport))
    {
        std::cerr << "Failed to write text report to " << textReportPath << '\n';
        return 1;
    }

    if (!WriteReportFile(jsonReportPath, jsonReport))
    {
        std::cerr << "Failed to write JSON report to " << jsonReportPath << '\n';
        return 1;
    }

    std::cout << "Stored report (text): " << textReportPath << '\n';
    std::cout << "Stored report (json): " << jsonReportPath << '\n';

    return CreateAndShowWindow(diagnosticText, capabilities, textReportPath, jsonReportPath);
}

} // namespace horde::platform::windows
