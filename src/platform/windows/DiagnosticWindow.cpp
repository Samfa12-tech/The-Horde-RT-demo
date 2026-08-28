#include "platform/windows/DiagnosticWindow.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <xinput.h>
#include <bcrypt.h>
#include <commdlg.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <xaudio2.h>
#ifdef DeviceCapabilities
#undef DeviceCapabilities
#endif
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include "ui/DiagnosticOverlay.h"
#include "gameplay/CorridorCollision.h"
#include "gameplay/DevelopmentCheckpoints.h"
#include "gameplay/DevelopmentCheckpointSimulation.h"
#include "gameplay/FeedbackTiming.h"
#include "gameplay/ShowcaseBenchmark.h"
#include "gameplay/ShowcaseCheckpoints.h"
#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/SpatialAudio.h"
#include "gameplay/SwordCombat.h"
#include "gameplay/simulation/GameSimulation.h"
#include "platform/windows/DesktopControllerInput.h"
#include "platform/windows/WindowsRtLabState.h"
#include "vulkan/GpuFrameTimer.h"
#include "vulkan/RtCapabilityReport.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/raytracing/PresentableTinyRtScene.h"
#include "vulkan/raytracing/DevelopmentStaticAssetPolicy.h"
#include "vulkan/raytracing/SimulationFrameAdapter.h"

#ifndef HORDE_RT_BUILD_ID
#define HORDE_RT_BUILD_ID "development"
#endif
#ifndef HORDE_RT_DISPLAY_VERSION
#define HORDE_RT_DISPLAY_VERSION "development"
#endif
#ifndef HORDE_RT_RAYGEN_SHA256
#define HORDE_RT_RAYGEN_SHA256 "unknown"
#endif

namespace
{

constexpr char kWindowClassName[] = "HordeRtDiagnosticWindowClass";
constexpr char kWindowTitle[] = "Horde Lantern RT - Showcase Alpha " HORDE_RT_DISPLAY_VERSION;
constexpr char kHudStartingText[] = "ALPHA " HORDE_RT_DISPLAY_VERSION "  |  VULKAN RT STARTING...  |  F1 CONTROLS  |  ESC MENU";
constexpr char kHudActiveText[] = "ALPHA " HORDE_RT_DISPLAY_VERSION "  |  NATIVE VULKAN HARDWARE RT ACTIVE  |  F1 CONTROLS  |  ESC MENU";
constexpr char kHudApplyingScaleText[] = "ALPHA " HORDE_RT_DISPLAY_VERSION "  |  APPLYING RT RENDER SCALE...";
constexpr char kAboutText[] = "Horde Lantern RT\nShowcase Alpha " HORDE_RT_DISPLAY_VERSION "\n\nNative Vulkan hardware ray tracing. RT or nothing.\nA Samfa12 technology demo.";
constexpr char kReportDirectory[] = "reports";
constexpr char kTextReportFilename[] = "vulkan_capability_report.txt";
constexpr char kJsonReportFilename[] = "vulkan_capability_report.json";
constexpr std::uint32_t kCaptureWidth = 960u;
constexpr std::uint32_t kCaptureHeight = 540u;
constexpr int kCaptureSettlingFrames = 12;
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
constexpr int kDeveloperOverlayId = 117;
constexpr int kMoreBySamfa12ButtonId = 118;
constexpr int kRunBenchmarkButtonId = 119;
constexpr int kBenchmarkTitleId = 120;
constexpr int kBenchmarkCopyButtonId = 121;
constexpr int kBenchmarkSaveButtonId = 122;
constexpr int kBenchmarkBackButtonId = 123;
constexpr int kVitalityHudControlId = 124;
constexpr int kEndingBodyId = 125;
constexpr int kWaterQualityButtonId = 126;
constexpr int kRtLabButtonId = 127;
constexpr int kRtLabPanelId = 128;
constexpr int kRtLabTitleId = 129;
constexpr int kRtLabTelemetryId = 130;
constexpr int kRtLabWaterfallLabelId = 131;
constexpr int kRtLabWaterfallSliderId = 132;
constexpr int kRtLabRoofLabelId = 133;
constexpr int kRtLabRoofSliderId = 134;
constexpr int kRtLabDawnLabelId = 135;
constexpr int kRtLabDawnSliderId = 136;
constexpr int kRtLabFogLabelId = 137;
constexpr int kRtLabFogSliderId = 138;
constexpr int kRtLabLightGroupButtonId = 139;
constexpr int kRtLabHueLabelId = 140;
constexpr int kRtLabHueSliderId = 141;
constexpr int kRtLabIntensityLabelId = 142;
constexpr int kRtLabIntensitySliderId = 143;
constexpr int kRtLabWorkloadButtonId = 144;
constexpr int kRtLabRestoreButtonId = 145;
constexpr int kRtLabBackButtonId = 146;
constexpr int kRtLabFireStrengthLabelId = 147;
constexpr int kRtLabFireStrengthSliderId = 148;
constexpr int kRtLabFireTurbulenceLabelId = 149;
constexpr int kRtLabFireTurbulenceSliderId = 150;
constexpr int kRtLabFireSmokeLabelId = 151;
constexpr int kRtLabFireSmokeSliderId = 152;
constexpr int kRtLabGlassVisibilityLabelId = 153;
constexpr int kRtLabGlassVisibilitySliderId = 154;
constexpr int kRtLabGlassTransmissionLabelId = 155;
constexpr int kRtLabGlassTransmissionSliderId = 156;
constexpr int kRtLabGlassIorLabelId = 157;
constexpr int kRtLabGlassIorSliderId = 158;
constexpr int kRtLabGlassRoughnessLabelId = 159;
constexpr int kRtLabGlassRoughnessSliderId = 160;
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
constexpr int kMenuDeveloperOverlayId = 2024;
constexpr int kAppIconId = 1;
constexpr UINT kDefaultDpi = 96u;
constexpr char kUiFontProperty[] = "HordeLanternRtUiFont";
constexpr char kMonoFontProperty[] = "HordeLanternRtMonoFont";
constexpr char kDeveloperFontProperty[] = "HordeLanternRtDeveloperFont";
constexpr char kCaptureModeProperty[] = "HordeLanternRtCaptureMode";
// One frame in flight keeps the dynamically refit held-torch TLAS safely synchronized with its host-written instance buffer.
constexpr UINT kMaxFramesInFlight = 1u;

struct CaptureLaunchOptions
{
    bool requested = false;
    std::filesystem::path outputDirectory;
    std::string developmentCheckpoint;
    std::string error;
};

#if defined(_DEBUG)
struct RtLabDebugLaunchOptions
{
    bool requested = false;
    horde::vulkan::raytracing::RtSceneTuning tuning{};
    horde::vulkan::raytracing::RtLightGroup lightGroup =
        horde::vulkan::raytracing::RtLightGroup::Torch;
};
#endif

struct ShowcaseCaptureRecord
{
    const horde::gameplay::ShowcaseCheckpoint* checkpoint = nullptr;
    std::string torchFailurePhase;
    std::string selectedEnemy;
    std::string lichPhase;
    float finaleSkylightOpenProgress = 0.0f;
    std::string filename;
    std::string pngSha256;
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    bool redBlueSwapNormalised = false;
    std::array<std::uint8_t,
               horde::vulkan::raytracing::PresentableTinyRtScene::kTlasInstanceCount>
        instanceMasks{};
    bool playerPrimaryVisible = false;
    std::uint32_t primaryTorchPixels = 0u;
    std::uint32_t primarySwordPixels = 0u;
    std::uint32_t primaryPlayerPixels = 0u;
    std::uint32_t primaryRewardRingPixels = 0u;
    std::uint32_t primaryRewardBodyPixels = 0u;
    float rewardGripPositionErrorMetres = 0.0f;
    float rewardGripOrientationErrorRadians = 0.0f;
    std::vector<double> frameTimesMs;
};

struct VulkanSurfaceContext
{
    VulkanSurfaceContext() = default;

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
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
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
    horde::vulkan::GpuFrameTimer gpuFrameTimer;
    horde::vulkan::GpuRtTimingSnapshot gpuRtTiming;
    double gpuFrameTimingTotalMs = 0.0;
    std::uint64_t gpuFrameTimingSampleCount = 0u;
    std::uint64_t gpuFrameSubmissionSequence = 0u;
    bool useRtPath = false;
    std::string developmentCheckpoint;
    std::string lastRtFrameError;
    bool controlsEnabled = false;
    bool simulationPaused = true;
    bool pauseMenuVisible = true;
    bool settingsVisible = false;
    bool diagnosticsVisible = false;
    bool benchmarkReportVisible = false;
    bool rtLabVisible = false;
    bool rtLabUnlocked = false;
    bool rtLabJustUnlocked = false;
    bool rtLabOpenedFromEnding = false;
    bool rtLabRouteTainted = false;
    bool rtLabDebugInjection = false;
    int rtLabScrollOffset = 0;
    horde::vulkan::raytracing::RtLightGroup rtLabLightGroup =
        horde::vulkan::raytracing::RtLightGroup::Torch;
    horde::vulkan::raytracing::RtSceneTuning rtSceneTuning{};
    ULONGLONG lastRtLabTelemetryTick = 0u;
#if defined(_DEBUG)
    bool developerOverlayVisible = false;
    ULONGLONG lastDeveloperOverlayTick = 0u;
#endif
    bool sfxEnabled = true;
    bool fullscreen = false;
    bool forwardHeld = false;
    bool backwardHeld = false;
    bool leftHeld = false;
    bool rightHeld = false;
    bool mouseLookActive = false;
    bool mouseCursorHidden = false;
    POINT mouseRestorePosition{};
    POINT lastMousePosition{};
    float controllerForward = 0.0f;
    float controllerStrafe = 0.0f;
    float controllerLookHorizontal = 0.0f;
    float controllerLookVertical = 0.0f;
    WORD previousControllerButtons = 0u;
    WORD previousXInputUiButtons = 0u;
    DWORD previousLegacyControllerButtons = 0u;
    DWORD previousLegacyUiButtons = 0u;
    DWORD previousLegacyPov = JOY_POVCENTERED;
    horde::platform::windows::ControllerTriggerLatch controllerTriggerLatch;
    std::optional<DWORD> xInputUserIndex;
    std::optional<UINT> legacyJoystickId;
    horde::platform::windows::LegacyRightStickAxes legacyRightStickAxes;
    ULONGLONG lastControlTick = 0u;
    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    float torchLightStrength = 1.8f;
    float walkTime = 0.0f;
    float walkVisualAmount = 0.0f;
    float cameraX = 0.0f;
    float cameraZ = 1.85f;
    float walkAmount = 0.0f;
    float playerTravelledThisFrame = 0.0f;
    float frameDeltaSeconds = 1.0f / 60.0f;
    int playerFootstepVariant = 0;
    int enemyFootstepVariant = 0;
    float outputExposure = 0.62f;
    float mouseSensitivity = 1.0f;
    float renderScale = 1.0f;
    horde::vulkan::raytracing::WaterQuality waterQuality =
        horde::vulkan::raytracing::WaterQuality::High;
    bool renderScaleDirty = false;
    WINDOWPLACEMENT windowedPlacement{sizeof(WINDOWPLACEMENT)};
    horde::gameplay::simulation::GameSimulation simulation;
    horde::gameplay::simulation::InputSnapshot simulationInput;
    std::uint64_t inputPublicationSequence = 0u;
    std::uint64_t attackSequence = 0u;
    std::uint64_t parrySequence = 0u;
    std::uint64_t dodgeSequence = 0u;
    std::uint64_t routeResetSequence = 0u;
    std::uint64_t retrySequence = 0u;
    std::uint64_t interactSequence = 0u;
    std::uint64_t toggleHeldLightPoseSequence = 0u;
    int playerSwingVariant = 0;
    horde::gameplay::DelayedGameplayFeedbackQueue delayedFeedback;
    // Legacy mirrors retained only for Win32 overlays, capture manifests, and
    // existing debug authoring controls. GameSimulation is gameplay authority.
    horde::gameplay::CombatSnapshot combatSnapshot;
    bool deathOverlayVisible = false;
    bool endingOverlayVisible = false;
    bool endingOverlayDismissed = false;
    std::int32_t playerRetryCheckpoint = 0;
    horde::gameplay::TorchFailureSnapshot torchFailureSnapshot;
    horde::gameplay::EnemyKind activeEnemyKind = horde::gameplay::EnemyKind::Skeleton;
    horde::gameplay::EnemyKind debugEnemyOverride = horde::gameplay::EnemyKind::None;
    uint32_t debugValidationPoint = 0u;
    horde::gameplay::ShowcaseBenchmarkRun benchmark;
    std::string benchmarkReport;
    std::string benchmarkJsonReport;
    bool benchmarkCompletionHandled = false;
    uint32_t currentFrame = 0u;
};

bool WriteReportFile(const std::filesystem::path& path, const std::string& data);
void ClearDesktopInput(VulkanSurfaceContext& context);
void UpdateVitalityHud(VulkanSurfaceContext& context);
int ScaleForDpi(HWND window, int logicalPixels);
void LayoutOverlayControls(HWND window, int width, int height);
std::string WindowSafeText(const std::string& value);

CaptureLaunchOptions ParseCaptureLaunchOptions()
{
    CaptureLaunchOptions options;
    int argumentCount = 0;
    LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
    if (arguments == nullptr)
    {
        options.error = "Failed to parse the process command line.";
        return options;
    }
    for (int index = 1; index < argumentCount; ++index)
    {
        const std::wstring_view argument(arguments[index]);
        if (argument == L"--development-checkpoint")
        {
            if (!options.developmentCheckpoint.empty())
            {
                options.error = "--development-checkpoint may only be specified once.";
                break;
            }
            if (index + 1 >= argumentCount || arguments[index + 1][0] == L'-')
            {
                options.error = "--development-checkpoint requires a checkpoint name.";
                break;
            }
            options.developmentCheckpoint = std::filesystem::path(arguments[++index]).string();
            continue;
        }
        if (argument != L"--capture-showcase")
        {
            continue;
        }
        if (options.requested)
        {
            options.error = "--capture-showcase may only be specified once.";
            break;
        }
        if (index + 1 >= argumentCount || arguments[index + 1][0] == L'-')
        {
            options.error = "--capture-showcase requires an output directory.";
            break;
        }
        options.requested = true;
        options.outputDirectory = std::filesystem::absolute(std::filesystem::path(arguments[++index]));
    }
    if (options.error.empty() && !options.developmentCheckpoint.empty() && !options.requested)
        options.error = "--development-checkpoint requires --capture-showcase.";
    if (options.error.empty() && !options.developmentCheckpoint.empty() &&
        horde::gameplay::FindDevelopmentCheckpoint(options.developmentCheckpoint) == nullptr)
        options.error = "Unknown development checkpoint: " + options.developmentCheckpoint;
    LocalFree(arguments);
    return options;
}

std::string JsonEscape(const std::string& value)
{
    std::ostringstream escaped;
    for (const unsigned char character : value)
    {
        switch (character)
        {
        case '\\': escaped << "\\\\"; break;
        case '"': escaped << "\\\""; break;
        case '\b': escaped << "\\b"; break;
        case '\f': escaped << "\\f"; break;
        case '\n': escaped << "\\n"; break;
        case '\r': escaped << "\\r"; break;
        case '\t': escaped << "\\t"; break;
        default:
            if (character < 0x20u)
            {
                escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<unsigned int>(character) << std::dec;
            }
            else
            {
                escaped << static_cast<char>(character);
            }
            break;
        }
    }
    return escaped.str();
}

bool WriteRgbaPng(const std::filesystem::path& path,
                  const horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture& capture,
                  std::string& diagnostic)
{
    const HRESULT initialiseResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool uninitialiseCom = SUCCEEDED(initialiseResult);
    if (FAILED(initialiseResult) && initialiseResult != RPC_E_CHANGED_MODE)
    {
        diagnostic = "Failed to initialise COM for WIC PNG encoding.";
        return false;
    }

    using Microsoft::WRL::ComPtr;
    ComPtr<IWICImagingFactory> factory;
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapEncoder> encoder;
    ComPtr<IWICBitmapFrameEncode> frame;
    ComPtr<IPropertyBag2> properties;
    HRESULT result = CoCreateInstance(CLSID_WICImagingFactory,
                                      nullptr,
                                      CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&factory));
    if (SUCCEEDED(result)) result = factory->CreateStream(&stream);
    if (SUCCEEDED(result)) result = stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE);
    if (SUCCEEDED(result)) result = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
    if (SUCCEEDED(result)) result = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
    if (SUCCEEDED(result)) result = encoder->CreateNewFrame(&frame, &properties);
    if (SUCCEEDED(result)) result = frame->Initialize(properties.Get());
    if (SUCCEEDED(result)) result = frame->SetSize(capture.width, capture.height);
    std::vector<std::uint8_t> encoderPixels = capture.rgba;
    for (std::size_t offset = 0; offset < encoderPixels.size(); offset += 4u)
    {
        std::swap(encoderPixels[offset], encoderPixels[offset + 2u]);
    }
    WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat32bppBGRA;
    if (SUCCEEDED(result)) result = frame->SetPixelFormat(&pixelFormat);
    if (SUCCEEDED(result) && !IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppBGRA)) result = WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
    if (SUCCEEDED(result))
    {
        result = frame->WritePixels(capture.height,
                                    capture.width * 4u,
                                    static_cast<UINT>(encoderPixels.size()),
                                    encoderPixels.data());
    }
    if (SUCCEEDED(result)) result = frame->Commit();
    if (SUCCEEDED(result)) result = encoder->Commit();

    properties.Reset();
    frame.Reset();
    encoder.Reset();
    stream.Reset();
    factory.Reset();
    if (uninitialiseCom)
    {
        CoUninitialize();
    }
    if (FAILED(result))
    {
        std::ostringstream failure;
        failure << "WIC PNG encoding failed with HRESULT 0x" << std::hex
                << static_cast<unsigned long>(result) << '.';
        diagnostic = failure.str();
        return false;
    }
    diagnostic.clear();
    return true;
}

bool Sha256File(const std::filesystem::path& path, std::string& hexDigest, std::string& diagnostic)
{
    std::ifstream input(path, std::ios::binary);
    if (!input.good())
    {
        diagnostic = "Failed to reopen capture for SHA-256: " + path.string();
        return false;
    }
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD objectSize = 0u;
    DWORD digestSize = 0u;
    DWORD resultSize = 0u;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0u);
    if (status >= 0) status = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
                                               reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &resultSize, 0u);
    if (status >= 0) status = BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH,
                                               reinterpret_cast<PUCHAR>(&digestSize), sizeof(digestSize), &resultSize, 0u);
    std::vector<std::uint8_t> object(objectSize);
    std::vector<std::uint8_t> digest(digestSize);
    if (status >= 0) status = BCryptCreateHash(algorithm, &hash, object.data(), objectSize, nullptr, 0u, 0u);
    if (status >= 0 && !bytes.empty())
    {
        status = BCryptHashData(hash, bytes.data(), static_cast<ULONG>(bytes.size()), 0u);
    }
    if (status >= 0) status = BCryptFinishHash(hash, digest.data(), digestSize, 0u);
    if (hash != nullptr) BCryptDestroyHash(hash);
    if (algorithm != nullptr) BCryptCloseAlgorithmProvider(algorithm, 0u);
    if (status < 0 || digestSize != 32u)
    {
        diagnostic = "Failed to calculate capture SHA-256.";
        return false;
    }

    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (const std::uint8_t byte : digest) output << std::setw(2) << static_cast<unsigned int>(byte);
    hexDigest = output.str();
    diagnostic.clear();
    return true;
}

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
#if defined(_DEBUG) && defined(HORDE_RT_SOURCE_DIR)
    return std::filesystem::path(HORDE_RT_SOURCE_DIR) / "assets";
#else
    // Release must prove that the executable-relative package is complete.
    // Falling back into a developer checkout can otherwise hide a broken ZIP.
    return packaged;
#endif
}

std::filesystem::path SettingsPath()
{
    return ExecutableDirectory() / "HordeLanternRT.settings.ini";
}

void LoadSettings(VulkanSurfaceContext& context)
{
    const std::string path = SettingsPath().string();
    // Progress is deliberately loaded independently from ordinary display/audio settings.
    context.rtLabUnlocked = GetPrivateProfileIntA("progress", "rtLabUnlocked", 0, path.c_str()) != 0;
    context.sfxEnabled = GetPrivateProfileIntA("audio", "sfx", 1, path.c_str()) != 0;
    const int sensitivity = std::clamp(static_cast<int>(GetPrivateProfileIntA("controls", "lookSensitivity", 100, path.c_str())), 60, 150);
    context.mouseSensitivity = static_cast<float>(sensitivity) / 100.0f;
    const int renderScale = std::clamp(static_cast<int>(GetPrivateProfileIntA("display", "renderScale", 100, path.c_str())), 50, 100);
    context.renderScale = static_cast<float>(renderScale) / 100.0f;
    const int waterQuality = std::clamp(static_cast<int>(GetPrivateProfileIntA("display", "waterQuality", 2, path.c_str())), 0, 2);
    context.waterQuality = static_cast<horde::vulkan::raytracing::WaterQuality>(waterQuality);
}

#if defined(_DEBUG)
RtLabDebugLaunchOptions ParseRtLabDebugLaunchOptions()
{
    RtLabDebugLaunchOptions options;
    int count = 0;
    LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &count);
    if (arguments == nullptr) return options;
    const auto readFloat = [&](const int index, float& output)
    {
        if (index + 1 >= count) return false;
        wchar_t* end = nullptr;
        const float value = std::wcstof(arguments[index + 1], &end);
        if (end == arguments[index + 1] || *end != L'\0') return false;
        output = value;
        return true;
    };
    for (int index = 1; index < count; ++index)
    {
        const std::wstring_view argument(arguments[index]);
        if (argument == L"--debug-rt-lab")
        {
            options.requested = true;
        }
        else if (argument == L"--rt-lab-waterfall")
        {
            float value = 100.0f;
            if (readFloat(index, value)) options.tuning.waterfallWidthScale = value / 100.0f, ++index;
        }
        else if (argument == L"--rt-lab-roof")
        {
            float value = 100.0f;
            if (readFloat(index, value)) options.tuning.finaleRoofOpenOverride = value / 100.0f, ++index;
        }
        else if (argument == L"--rt-lab-dawn")
        {
            float value = 100.0f;
            if (readFloat(index, value)) options.tuning.finaleDawnRevealOverride = value / 100.0f, ++index;
        }
        else if (argument == L"--rt-lab-fog")
        {
            float value = 100.0f;
            if (readFloat(index, value)) options.tuning.fogDensityScale = value / 100.0f, ++index;
        }
        else if (argument == L"--rt-lab-light")
        {
            if (index + 1 < count)
            {
                const std::wstring_view value(arguments[++index]);
                options.lightGroup = value == L"skylight" ? horde::vulkan::raytracing::RtLightGroup::Skylight :
                    (value == L"passage" ? horde::vulkan::raytracing::RtLightGroup::Passage :
                     (value == L"staff" ? horde::vulkan::raytracing::RtLightGroup::Staff :
                                          horde::vulkan::raytracing::RtLightGroup::Torch));
            }
        }
        else if (argument == L"--rt-lab-hue" || argument == L"--rt-lab-intensity")
        {
            float value = argument == L"--rt-lab-hue" ? 0.0f : 100.0f;
            if (readFloat(index, value))
            {
                auto& light = options.tuning.lights[static_cast<std::size_t>(options.lightGroup)];
                if (argument == L"--rt-lab-hue") light.hueDegrees = value;
                else light.intensityScale = value / 100.0f;
                ++index;
            }
        }
        else if (argument == L"--rt-lab-workload" && index + 1 < count)
        {
            const std::wstring_view value(arguments[++index]);
            options.tuning.workloadPreset = value == L"lean" ? horde::vulkan::raytracing::RtWorkloadPreset::Lean :
                (value == L"max" ? horde::vulkan::raytracing::RtWorkloadPreset::Max :
                                   horde::vulkan::raytracing::RtWorkloadPreset::Authored);
        }
    }
    LocalFree(arguments);
    options.tuning = horde::vulkan::raytracing::ClampRtSceneTuning(options.tuning);
    return options;
}
#endif

void SaveRtLabProgress(const VulkanSurfaceContext& context)
{
    const std::string path = SettingsPath().string();
    WritePrivateProfileStringA("progress", "rtLabUnlocked",
                               context.rtLabUnlocked ? "1" : "0", path.c_str());
}

void SaveSettings(const VulkanSurfaceContext& context)
{
    const std::string path = SettingsPath().string();
    WritePrivateProfileStringA("audio", "sfx", context.sfxEnabled ? "1" : "0", path.c_str());
    const std::string sensitivity = std::to_string(static_cast<int>(std::round(context.mouseSensitivity * 100.0f)));
    WritePrivateProfileStringA("controls", "lookSensitivity", sensitivity.c_str(), path.c_str());
    const std::string renderScale = std::to_string(static_cast<int>(std::round(context.renderScale * 100.0f)));
    WritePrivateProfileStringA("display", "renderScale", renderScale.c_str(), path.c_str());
    const std::string waterQuality = std::to_string(static_cast<int>(context.waterQuality));
    WritePrivateProfileStringA("display", "waterQuality", waterQuality.c_str(), path.c_str());
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

    bool StartOrUpdateLoop(const std::string_view key,
                           const std::filesystem::path& path,
                           float leftGain,
                           float rightGain)
    {
        if (engine_ == nullptr || masteringVoice_ == nullptr)
        {
            LogFailureOnce("XAudio2 unavailable for loop " + path.string());
            return false;
        }
        for (ActiveVoice& active : activeVoices_)
        {
            if (active.loopKey == key)
            {
                return SetVoiceMatrix(active.voice, leftGain, rightGain, path);
            }
        }

        const std::shared_ptr<const LoadedWave> wave = Load(path);
        if (!wave || wave->format.nChannels != 1u || wave->samples.size() > UINT32_MAX)
        {
            LogFailureOnce("unsupported or unreadable mono loop WAV: " + path.string());
            return false;
        }
        IXAudio2SourceVoice* voice = nullptr;
        const HRESULT sourceResult = engine_->CreateSourceVoice(&voice, &wave->format);
        if (FAILED(sourceResult) || voice == nullptr)
        {
            LogFailureOnce("CreateSourceVoice failed for loop, HRESULT=" +
                           std::to_string(static_cast<long>(sourceResult)) + ": " + path.string());
            return false;
        }
        if (!SetVoiceMatrix(voice, leftGain, rightGain, path))
        {
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
            XAUDIO2_LOOP_INFINITE,
            nullptr};
        const HRESULT submitResult = voice->SubmitSourceBuffer(&buffer);
        const HRESULT startResult = SUCCEEDED(submitResult) ? voice->Start() : E_FAIL;
        if (FAILED(submitResult) || FAILED(startResult))
        {
            LogFailureOnce("loop voice submit/start failed, HRESULT=" +
                           std::to_string(static_cast<long>(FAILED(submitResult) ? submitResult : startResult)) +
                           ": " + path.string());
            voice->DestroyVoice();
            return false;
        }
        activeVoices_.push_back({voice, wave, path.filename().string(), std::string(key)});
        LogWindowsAudio("positional loop started: " + path.filename().string());
        return true;
    }

    void StopLoop(const std::string_view key)
    {
        for (auto it = activeVoices_.begin(); it != activeVoices_.end();)
        {
            if (it->loopKey == key)
            {
                it->voice->Stop(0u);
                it->voice->FlushSourceBuffers();
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
        activeVoices_.push_back({voice, wave, filename, {}});
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
        std::string loopKey;
    };

    bool SetVoiceMatrix(IXAudio2SourceVoice* voice,
                        float leftGain,
                        float rightGain,
                        const std::filesystem::path& path)
    {
        std::vector<float> matrix(outputChannels_, 0.0f);
        if (outputChannels_ == 1u)
        {
            matrix[0] = std::max(leftGain, rightGain);
        }
        else
        {
            matrix[0] = std::clamp(leftGain, 0.0f, 1.0f);
            matrix[1] = std::clamp(rightGain, 0.0f, 1.0f);
        }
        const HRESULT result = voice->SetOutputMatrix(
            masteringVoice_, 1u, outputChannels_, matrix.data());
        if (FAILED(result))
        {
            LogFailureOnce("SetOutputMatrix failed, HRESULT=" +
                           std::to_string(static_cast<long>(result)) + ": " + path.string());
            return false;
        }
        return true;
    }

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

void UpdateWaterfallAmbience(const VulkanSurfaceContext& context)
{
    constexpr std::string_view loopKey = "waterfall";
    PositionalAudioEngine& engine = SpatialAudioEngine();
    if (!context.sfxEnabled || context.simulationPaused)
    {
        engine.StopLoop(loopKey);
        return;
    }
    const horde::gameplay::simulation::SimulationSnapshot& simulation =
        context.simulation.Snapshot();
    const horde::gameplay::SpatialAudioGains gains = horde::gameplay::CalculateSpatialAudio(
        {-2.32f, -15.26f, 0.52f, 0.65f, 10.0f},
        {simulation.playerX, simulation.playerZ, simulation.playerYawRadians});
    const std::filesystem::path path =
        ResolveAssetRoot() / "audio/pixabay/waterfall_loop.wav";
    if (gains.left <= 0.0f && gains.right <= 0.0f)
    {
        engine.StopLoop(loopKey);
        return;
    }
    engine.StartOrUpdateLoop(loopKey, path, gains.left, gains.right);
}

void PlayPositionalSoundEffect(const VulkanSurfaceContext& context,
                               const char* filename,
                               float mixGain,
                               const horde::gameplay::simulation::GameplayEvent& event)
{
    if (!context.sfxEnabled)
    {
        return;
    }
    const horde::gameplay::SpatialAudioGains gains = horde::gameplay::CalculateSpatialAudio(
        {event.worldX, event.worldZ, mixGain, 1.0f, 14.0f},
        {event.listenerX, event.listenerZ, event.listenerYawRadians});
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

void DrainGameplayEvents(VulkanSurfaceContext& context)
{
    using horde::gameplay::simulation::EntityId;
    using horde::gameplay::simulation::GameplayEvent;
    using horde::gameplay::simulation::GameplayEventType;

    context.delayedFeedback.DrainDue(GetTickCount64(), [&context](const GameplayEvent& event)
    {
        PlayPositionalSoundEffect(context, "enemy_fall.wav", 0.36f, event);
    });

    for (const GameplayEvent& event : context.simulation.Events().Events())
    {
        switch (event.type)
        {
        case GameplayEventType::PlayerFootstep:
        {
            const char* clip = (context.playerFootstepVariant++ & 1) == 0
                ? "player_step_1.wav" : "player_step_2.wav";
            PlayAmbientSoundEffect(context, clip);
            break;
        }
        case GameplayEventType::PlayerSwing:
            PlaySoundEffect(context,
                            (context.playerSwingVariant++ & 1) == 0
                                ? "sword_swing_1.wav" : "sword_swing_2.wav");
            break;
        case GameplayEventType::PlayerParrySucceeded:
            PlayPositionalSoundEffect(context, "sword_hit_2.wav", 1.0f, event);
            break;
        case GameplayEventType::EnemyFootstep:
        {
            const char* clip = (context.enemyFootstepVariant++ & 1) == 0
                ? "skeleton_step_1.wav" : "skeleton_step_2.wav";
            PlayPositionalSoundEffect(context, clip, 1.0f, event);
            break;
        }
        case GameplayEventType::EnemyAttackStarted:
            PlayPositionalSoundEffect(context, "skeleton_attack.wav", 0.85f, event);
            break;
        case GameplayEventType::EnemyHit:
            if (event.target == EntityId::Lich)
            {
                PlayPositionalSoundEffect(context, "lich_hurt.wav", 0.95f, event);
            }
            else
            {
                PlayPositionalSoundEffect(context, "sword_hit_1.wav", 1.0f, event);
            }
            break;
        case GameplayEventType::EnemyDefeated:
            if (!context.delayedFeedback.Enqueue(
                    event,
                    GetTickCount64() + horde::gameplay::kEnemyImpactFallDelayMilliseconds))
            {
                LogWindowsAudio("delayed enemy-fall feedback queue overflowed; newest cue was dropped");
            }
            break;
        case GameplayEventType::LichChargeStarted:
            PlayPositionalSoundEffect(context, "lich_charge.wav", 0.42f, event);
            break;
        case GameplayEventType::LichImpact:
            PlayPositionalSoundEffect(context, "lich_impact.wav", 0.55f, event);
            break;
        case GameplayEventType::LichDefeated:
            PlayPositionalSoundEffect(context, "lich_fall.wav", 0.36f, event);
            break;
        case GameplayEventType::PlayerDamaged:
            UpdateVitalityHud(context);
            break;
        case GameplayEventType::PlayerKilled:
            UpdateVitalityHud(context);
            ClearDesktopInput(context);
            break;
        default:
            break;
        }
    }
    context.simulation.ClearEvents();
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
    if (HWND water = GetDlgItem(context.windowHandle, kWaterQualityButtonId))
    {
        const char* value = context.waterQuality == horde::vulkan::raytracing::WaterQuality::High ? "HIGH" :
                            (context.waterQuality == horde::vulkan::raytracing::WaterQuality::Mobile ? "MOBILE" : "OFF");
        const std::string label = std::string("RT WATER: ") + value;
        SetWindowTextA(water, label.c_str());
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

const char* RtLightGroupName(const horde::vulkan::raytracing::RtLightGroup group)
{
    using horde::vulkan::raytracing::RtLightGroup;
    switch (group)
    {
    case RtLightGroup::Skylight: return "SKYLIGHT";
    case RtLightGroup::Passage: return "PASSAGE";
    case RtLightGroup::Staff: return "STAFF";
    default: return "TORCH";
    }
}

const char* RtWorkloadName(const horde::vulkan::raytracing::RtWorkloadPreset preset)
{
    using horde::vulkan::raytracing::RtWorkloadPreset;
    switch (preset)
    {
    case RtWorkloadPreset::Lean: return "LEAN";
    case RtWorkloadPreset::Max: return "MAX";
    default: return "AUTHORED";
    }
}

void UpdateRtLabTelemetry(VulkanSurfaceContext& context, const bool force = false)
{
    if (!context.rtLabVisible) return;
    const ULONGLONG now = GetTickCount64();
    if (!force && now - context.lastRtLabTelemetryTick < 250u) return;
    context.lastRtLabTelemetryTick = now;
    std::ostringstream text;
    text << std::fixed << std::setprecision(2) << "GPU RT: ";
    if (context.gpuRtTiming.valid)
    {
        text << context.gpuRtTiming.latestMs << " ms  |  "
             << context.gpuRtTiming.sampleCount << " samples";
    }
    else
    {
        text << "warming up  |  " << context.gpuRtTiming.sampleCount << " samples";
    }
    text << "  |  SCALE " << static_cast<int>(std::lround(context.renderScale * 100.0f)) << "%  |  WATER ";
    text << (context.waterQuality == horde::vulkan::raytracing::WaterQuality::High ? "HIGH" :
             (context.waterQuality == horde::vulkan::raytracing::WaterQuality::Mobile ? "MOBILE" : "OFF"));
    if (HWND control = GetDlgItem(context.windowHandle, kRtLabTelemetryId))
    {
        SetWindowTextA(control, text.str().c_str());
    }
}

void UpdateRtLabLabels(VulkanSurfaceContext& context)
{
    const auto setText = [&](const int id, const std::string& text)
    {
        if (HWND control = GetDlgItem(context.windowHandle, id)) SetWindowTextA(control, text.c_str());
    };
    const auto setSlider = [&](const int id, const int value)
    {
        if (HWND control = GetDlgItem(context.windowHandle, id)) SendMessageA(control, TBM_SETPOS, TRUE, value);
    };
    const auto& tuning = context.rtSceneTuning;
    const int waterfall = static_cast<int>(std::lround(tuning.waterfallWidthScale * 100.0f));
    setText(kRtLabWaterfallLabelId, "WATERFALL WIDTH: " + std::to_string(waterfall) + "%");
    setSlider(kRtLabWaterfallSliderId, waterfall);
    const int roof = static_cast<int>(std::lround(tuning.finaleRoofOpenOverride.value_or(1.0f) * 100.0f));
    setText(kRtLabRoofLabelId, tuning.finaleRoofOpenOverride.has_value()
        ? "FINALE ROOF OPEN: " + std::to_string(roof) + "%"
        : "FINALE ROOF: AUTHORED (ADJUST TO OVERRIDE)");
    setSlider(kRtLabRoofSliderId, roof);
    const int dawn = static_cast<int>(std::lround(tuning.finaleDawnRevealOverride.value_or(1.0f) * 100.0f));
    setText(kRtLabDawnLabelId, tuning.finaleDawnRevealOverride.has_value()
        ? "FINALE DAWN: " + std::to_string(dawn) + "%"
        : "FINALE DAWN: AUTHORED (ADJUST TO OVERRIDE)");
    setSlider(kRtLabDawnSliderId, dawn);
    const int fog = static_cast<int>(std::lround(tuning.fogDensityScale * 100.0f));
    setText(kRtLabFogLabelId, "FOG DENSITY: " + std::to_string(fog) + "%");
    setSlider(kRtLabFogSliderId, fog);
    const int fireStrength = static_cast<int>(std::lround(tuning.fireStrengthScale * 100.0f));
    setText(kRtLabFireStrengthLabelId, "FLAME STRENGTH: " + std::to_string(fireStrength) + "%");
    setSlider(kRtLabFireStrengthSliderId, fireStrength);
    const int fireTurbulence = static_cast<int>(std::lround(tuning.fireTurbulenceScale * 100.0f));
    setText(kRtLabFireTurbulenceLabelId, "FLAME TURBULENCE: " + std::to_string(fireTurbulence) + "%");
    setSlider(kRtLabFireTurbulenceSliderId, fireTurbulence);
    const int fireSmoke = static_cast<int>(std::lround(tuning.fireSmokeScale * 100.0f));
    setText(kRtLabFireSmokeLabelId, "FLAME SMOKE: " + std::to_string(fireSmoke) + "%");
    setSlider(kRtLabFireSmokeSliderId, fireSmoke);
    const int glassVisibility = tuning.glassFixtureVisible ? 100 : 0;
    setText(kRtLabGlassVisibilityLabelId,
            std::string("GLASS FIXTURE: ") + (glassVisibility != 0 ? "VISIBLE" : "HIDDEN"));
    setSlider(kRtLabGlassVisibilitySliderId, glassVisibility);
    const int glassTransmission = static_cast<int>(std::lround(tuning.glassTransmission * 100.0f));
    setText(kRtLabGlassTransmissionLabelId,
            "GLASS TRANSMISSION: " + std::to_string(glassTransmission) + "%");
    setSlider(kRtLabGlassTransmissionSliderId, glassTransmission);
    const int glassIor = static_cast<int>(std::lround(tuning.glassIor * 100.0f));
    setText(kRtLabGlassIorLabelId,
            "GLASS IOR: " + std::to_string(glassIor / 100) + "." +
            (glassIor % 100 < 10 ? "0" : "") + std::to_string(glassIor % 100));
    setSlider(kRtLabGlassIorSliderId, glassIor);
    const int glassRoughness = static_cast<int>(std::lround(tuning.glassRoughness * 100.0f));
    setText(kRtLabGlassRoughnessLabelId,
            "GLASS ROUGHNESS: " + std::to_string(glassRoughness) + "%");
    setSlider(kRtLabGlassRoughnessSliderId, glassRoughness);

    const auto& light = tuning.lights[static_cast<std::size_t>(context.rtLabLightGroup)];
    setText(kRtLabLightGroupButtonId, std::string("LIGHT GROUP: ") + RtLightGroupName(context.rtLabLightGroup));
    const int hue = static_cast<int>(std::lround(light.hueDegrees));
    setText(kRtLabHueLabelId, "LIGHT HUE SHIFT: " + std::to_string(hue) + " DEG");
    setSlider(kRtLabHueSliderId, hue);
    const int intensity = static_cast<int>(std::lround(light.intensityScale * 100.0f));
    setText(kRtLabIntensityLabelId, "LIGHT INTENSITY: " + std::to_string(intensity) + "%");
    setSlider(kRtLabIntensitySliderId, intensity);
    setText(kRtLabWorkloadButtonId, std::string("RT WORKLOAD: ") + RtWorkloadName(tuning.workloadPreset));
    UpdateRtLabTelemetry(context, true);
}

void UpdateVitalityHud(VulkanSurfaceContext& context)
{
    const horde::gameplay::PlayerVitalsSnapshot& vitals = context.simulation.Snapshot().playerVitals;
    const std::string text = "VITALITY  " + std::to_string(vitals.vitality) + " / " +
                             std::to_string(vitals.maxVitality);
    if (HWND hud = GetDlgItem(context.windowHandle, kVitalityHudControlId))
    {
        SetWindowTextA(hud, text.c_str());
        InvalidateRect(hud, nullptr, TRUE);
    }
}

bool IsPlayerDamageEnabled(const VulkanSurfaceContext& context)
{
    return !context.simulationPaused &&
           context.simulation.Snapshot().playerVitals.phase == horde::gameplay::PlayerLifePhase::Alive &&
           !context.benchmark.IsRunning() &&
           GetPropA(context.windowHandle, kCaptureModeProperty) == nullptr;
}

void MirrorSimulationSnapshot(VulkanSurfaceContext& context, const bool mirrorView = true)
{
    const horde::gameplay::simulation::SimulationSnapshot& snapshot = context.simulation.Snapshot();
    if (mirrorView)
    {
        context.cameraYaw = snapshot.playerYawRadians;
        context.cameraPitch = snapshot.playerPitchRadians;
    }
    context.walkTime = snapshot.walkTime;
    context.walkVisualAmount = snapshot.walkAmount;
    context.cameraX = snapshot.playerX;
    context.cameraZ = snapshot.playerZ;
    context.walkAmount = snapshot.walkAmount;
    context.playerTravelledThisFrame = snapshot.playerTravelledThisTick;
    context.combatSnapshot = snapshot.swordCombat;
    context.torchFailureSnapshot = snapshot.torchFailure;
    context.activeEnemyKind = snapshot.activeEnemyKind;
    context.playerRetryCheckpoint = snapshot.retryCheckpoint;
}

void ApplyOverlayState(VulkanSurfaceContext& context)
{
    const bool pauseVisible = context.pauseMenuVisible && !context.settingsVisible &&
                              !context.diagnosticsVisible && !context.benchmarkReportVisible &&
                              !context.rtLabVisible;
    for (const int id : {kPauseTitleId, kResumeButtonId, kRestartButtonId, kExitButtonId})
    {
        SetControlVisible(context.windowHandle, id, pauseVisible);
    }
    SetControlVisible(context.windowHandle, kEndingBodyId, pauseVisible && context.endingOverlayVisible);
    const bool fullPauseMenuVisible = pauseVisible && !context.deathOverlayVisible && !context.endingOverlayVisible;
    for (const int id : {kControlsButtonId, kSettingsButtonId, kDiagnosticsButtonId,
                         kRunBenchmarkButtonId, kMoreBySamfa12ButtonId})
    {
        SetControlVisible(context.windowHandle, id, fullPauseMenuVisible);
    }
    const bool rtLabAccess = context.rtLabUnlocked || context.rtLabDebugInjection;
    SetControlVisible(context.windowHandle, kRtLabButtonId,
                      pauseVisible && rtLabAccess && !context.deathOverlayVisible);
    for (const int id : {kSettingsTitleId, kSfxButtonId, kSensitivityButtonId, kWaterQualityButtonId, kRenderScaleLabelId,
                          kRenderScaleSliderId, kFullscreenButtonId, kSettingsBackButtonId})
    {
        SetControlVisible(context.windowHandle, id, context.settingsVisible);
    }
    SetControlVisible(context.windowHandle, kEditControlId,
                      context.diagnosticsVisible || context.benchmarkReportVisible);
    for (const int id : {kBenchmarkTitleId, kBenchmarkCopyButtonId,
                         kBenchmarkSaveButtonId, kBenchmarkBackButtonId})
    {
        SetControlVisible(context.windowHandle, id, context.benchmarkReportVisible);
    }
    for (const int id : {kRtLabPanelId, kRtLabTitleId, kRtLabTelemetryId,
                         kRtLabWaterfallLabelId, kRtLabWaterfallSliderId,
                         kRtLabRoofLabelId, kRtLabRoofSliderId,
                         kRtLabDawnLabelId, kRtLabDawnSliderId,
                         kRtLabFogLabelId, kRtLabFogSliderId,
                         kRtLabFireStrengthLabelId, kRtLabFireStrengthSliderId,
                         kRtLabFireTurbulenceLabelId, kRtLabFireTurbulenceSliderId,
                         kRtLabFireSmokeLabelId, kRtLabFireSmokeSliderId,
                         kRtLabGlassVisibilityLabelId, kRtLabGlassVisibilitySliderId,
                         kRtLabGlassTransmissionLabelId, kRtLabGlassTransmissionSliderId,
                         kRtLabGlassIorLabelId, kRtLabGlassIorSliderId,
                         kRtLabGlassRoughnessLabelId, kRtLabGlassRoughnessSliderId,
                         kRtLabLightGroupButtonId, kRtLabHueLabelId, kRtLabHueSliderId,
                         kRtLabIntensityLabelId, kRtLabIntensitySliderId,
                         kRtLabWorkloadButtonId, kRtLabRestoreButtonId, kRtLabBackButtonId})
    {
        SetControlVisible(context.windowHandle, id, context.rtLabVisible);
    }
    SetControlVisible(context.windowHandle, kHudControlId,
                      !context.diagnosticsVisible && !context.benchmarkReportVisible && !context.rtLabVisible);
    SetControlVisible(context.windowHandle, kVitalityHudControlId,
                      !pauseVisible && !context.settingsVisible && !context.diagnosticsVisible &&
                          !context.benchmarkReportVisible && !context.benchmark.IsRunning() &&
                          !context.rtLabVisible);
#if defined(_DEBUG)
    SetControlVisible(context.windowHandle, kDeveloperOverlayId,
                       context.developerOverlayVisible && !pauseVisible && !context.benchmarkReportVisible &&
                           !context.settingsVisible && !context.diagnosticsVisible &&
                           !context.benchmark.IsRunning() && !context.rtLabVisible);
#endif
    const bool wasSimulationPaused = context.simulationPaused;
    context.simulationPaused = pauseVisible || context.settingsVisible || context.rtLabVisible ||
                               context.diagnosticsVisible || context.benchmarkReportVisible;
    context.simulationInput.paused = context.simulationPaused;
    if (context.simulationPaused != wasSimulationPaused)
    {
        context.simulation.ResetTiming();
    }
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
    if (HWND title = GetDlgItem(context.windowHandle, kPauseTitleId))
    {
        SetWindowTextA(title, context.deathOverlayVisible
            ? "YOU FELL  |  THE RUIN CLAIMS ANOTHER LIGHT."
            : (context.endingOverlayVisible
                ? (context.rtLabJustUnlocked ? "RT LAB UNLOCKED" : "DAWN RETURNS  |  THE LAST LANTERN HAS DONE ITS WORK")
                : "HORDE LANTERN RT  |  SHOWCASE ALPHA"));
    }
    if (HWND resume = GetDlgItem(context.windowHandle, kResumeButtonId))
    {
        SetWindowTextA(resume, context.deathOverlayVisible
            ? "RETRY ENCOUNTER"
            : (context.endingOverlayVisible ? "CONTINUE" : "ENTER THE RUIN / RESUME"));
    }
    if (HWND restart = GetDlgItem(context.windowHandle, kRestartButtonId))
    {
        SetWindowTextA(restart, context.endingOverlayVisible ? "BEGIN AGAIN" : "RESTART ROUTE");
    }
    if (HWND lab = GetDlgItem(context.windowHandle, kRtLabButtonId))
    {
        SetWindowTextA(lab, context.endingOverlayVisible ? "OPEN RT LAB" : "RT LAB");
    }
    if (HWND body = GetDlgItem(context.windowHandle, kEndingBodyId))
    {
        SetWindowTextA(body, context.rtLabJustUnlocked
            ? "The lich is defeated. The renderer controls used to shape this ruin are now yours.\r\n\r\n"
              "Tune true RT geometry, light, fog, and workload while the scene continues to render."
            : "The old guard bound the lich beneath this ruin and left one lantern to guide whoever came after.\r\n\r\n"
              "Its flame died when the final seal opened. Now the staff is silent, the roof gives way, and stolen morning returns to the halls.");
    }
    UpdateSettingsLabels(context);
    if (context.rtLabVisible) UpdateRtLabLabels(context);
}

void OpenRtLab(VulkanSurfaceContext& context)
{
    if (!context.rtLabUnlocked && !context.rtLabDebugInjection) return;
    context.rtLabOpenedFromEnding = context.endingOverlayVisible;
    context.rtLabVisible = true;
    context.pauseMenuVisible = false;
    context.settingsVisible = false;
    context.diagnosticsVisible = false;
    context.benchmarkReportVisible = false;
    context.rtLabScrollOffset = 0;
    ApplyOverlayState(context);
    RECT client{};
    GetClientRect(context.windowHandle, &client);
    LayoutOverlayControls(context.windowHandle, client.right, client.bottom);
    SetFocus(GetDlgItem(context.windowHandle, kRtLabWaterfallSliderId));
}

void CloseRtLab(VulkanSurfaceContext& context)
{
    context.rtLabVisible = false;
    context.pauseMenuVisible = true;
    if (!context.rtLabOpenedFromEnding)
    {
        context.endingOverlayVisible = false;
    }
    ApplyOverlayState(context);
    RECT client{};
    GetClientRect(context.windowHandle, &client);
    LayoutOverlayControls(context.windowHandle, client.right, client.bottom);
    SetFocus(GetDlgItem(context.windowHandle, kRtLabButtonId));
}

void ShowPauseMenu(VulkanSurfaceContext& context, const bool visible)
{
    if (context.deathOverlayVisible && !visible)
    {
        return;
    }
    if (context.endingOverlayVisible && !visible)
    {
        context.endingOverlayVisible = false;
        context.endingOverlayDismissed = true;
    }
    context.pauseMenuVisible = visible;
    context.settingsVisible = false;
    context.diagnosticsVisible = false;
    context.benchmarkReportVisible = false;
    ApplyOverlayState(context);
    RECT clientRect{};
    GetClientRect(context.windowHandle, &clientRect);
    LayoutOverlayControls(context.windowHandle,
                          clientRect.right - clientRect.left,
                          clientRect.bottom - clientRect.top);
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
    context.torchLightStrength = 1.8f;
    context.deathOverlayVisible = false;
    context.endingOverlayVisible = false;
    context.endingOverlayDismissed = false;
    context.debugEnemyOverride = horde::gameplay::EnemyKind::None;
    context.debugValidationPoint = 0u;
    context.rtSceneTuning = {};
    context.rtLabLightGroup = horde::vulkan::raytracing::RtLightGroup::Torch;
    context.rtLabVisible = false;
    context.rtLabOpenedFromEnding = false;
    context.rtLabJustUnlocked = false;
    context.rtLabRouteTainted = false;
    context.delayedFeedback.Clear();
    context.simulation.ResetRoute();
    context.simulation.ClearEvents();
    context.simulationInput.moveForward = 0.0f;
    context.simulationInput.moveStrafe = 0.0f;
    context.simulationInput.torchLightStrength = context.torchLightStrength;
    context.simulationInput.hasAuthoritativePlayerPose = false;
    MirrorSimulationSnapshot(context);
    ClearDesktopInput(context);
    UpdateVitalityHud(context);
}

void TryGrantRtLabUnlock(VulkanSurfaceContext& context, const bool finaleComplete)
{
    const horde::platform::windows::RtLabUnlockContext decision{
        .finaleComplete = finaleComplete,
        .capture = GetPropA(context.windowHandle, kCaptureModeProperty) != nullptr,
        .checkpoint = context.rtLabRouteTainted,
        .replay = false,
        .benchmark = context.benchmark.IsRunning(),
        .debugInjection = context.rtLabDebugInjection,
    };
    if (!context.rtLabUnlocked && horde::platform::windows::CanPersistRtLabUnlock(decision))
    {
        context.rtLabUnlocked = true;
        context.rtLabJustUnlocked = true;
        SaveRtLabProgress(context);
    }
}

void ShowDeathMenu(VulkanSurfaceContext& context)
{
    if (context.deathOverlayVisible)
    {
        return;
    }
    context.deathOverlayVisible = true;
    context.pauseMenuVisible = true;
    context.settingsVisible = false;
    context.diagnosticsVisible = false;
    context.benchmarkReportVisible = false;
    ApplyOverlayState(context);
    RECT clientRect{};
    GetClientRect(context.windowHandle, &clientRect);
    LayoutOverlayControls(context.windowHandle,
                          clientRect.right - clientRect.left,
                          clientRect.bottom - clientRect.top);
    SetFocus(GetDlgItem(context.windowHandle, kResumeButtonId));
}

void ShowEndingMenu(VulkanSurfaceContext& context)
{
    if (context.endingOverlayVisible || context.endingOverlayDismissed ||
        context.deathOverlayVisible || context.rtLabVisible || context.benchmark.IsRunning() ||
        GetPropA(context.windowHandle, kCaptureModeProperty) != nullptr)
    {
        return;
    }
    context.endingOverlayVisible = true;
    context.pauseMenuVisible = true;
    context.settingsVisible = false;
    context.diagnosticsVisible = false;
    context.benchmarkReportVisible = false;
    ApplyOverlayState(context);
    RECT clientRect{};
    GetClientRect(context.windowHandle, &clientRect);
    LayoutOverlayControls(context.windowHandle,
                          clientRect.right - clientRect.left,
                          clientRect.bottom - clientRect.top);
    SetFocus(GetDlgItem(context.windowHandle, kResumeButtonId));
}
bool ApplyPlayerRetryCheckpoint(VulkanSurfaceContext& context, const std::int32_t checkpointId)
{
    if (checkpointId != 0 && checkpointId != 9)
    {
        return false;
    }
    const horde::gameplay::ShowcaseCheckpoint* checkpoint =
        horde::gameplay::FindShowcaseCheckpoint(checkpointId);
    if (checkpoint == nullptr)
    {
        return false;
    }
    context.delayedFeedback.Clear();
    context.simulation.RetryEncounter();
    context.simulation.ClearEvents();
    context.simulationInput.hasAuthoritativePlayerPose = false;
    context.simulationInput.paused = false;
    MirrorSimulationSnapshot(context);
    context.debugEnemyOverride = horde::gameplay::EnemyKind::None;
    context.pauseMenuVisible = false;
    context.settingsVisible = false;
    context.diagnosticsVisible = false;
    context.benchmarkReportVisible = false;
    ApplyOverlayState(context);
    UpdateVitalityHud(context);
    SetFocus(context.windowHandle);
    return true;
}

const char* PresentModeName(const VkPresentModeKHR mode)
{
    switch (mode)
    {
    case VK_PRESENT_MODE_MAILBOX_KHR: return "MAILBOX";
    case VK_PRESENT_MODE_IMMEDIATE_KHR: return "IMMEDIATE";
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return "FIFO_RELAXED";
    default: return "FIFO";
    }
}

std::string UtcTimestamp(const char* format)
{
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
    gmtime_s(&utc, &now);
    char text[64]{};
    std::strftime(text, sizeof(text), format, &utc);
    return text;
}

horde::gameplay::ShowcaseBenchmarkMetadata BuildBenchmarkMetadata(
    const VulkanSurfaceContext& context,
    const horde::vulkan::DeviceCapabilities& capabilities)
{
    horde::gameplay::ShowcaseBenchmarkMetadata metadata;
    metadata.timestampUtc = UtcTimestamp("%Y-%m-%dT%H:%M:%SZ");
    metadata.buildIdentity = HORDE_RT_BUILD_ID;
    metadata.shaderIdentity = std::string(HORDE_RT_RAYGEN_SHA256).substr(0u, 12u);
    metadata.gpuName = capabilities.identity.gpuName;
    metadata.vulkanApi = std::to_string(VK_API_VERSION_MAJOR(capabilities.identity.vulkanApiVersion)) + "." +
                         std::to_string(VK_API_VERSION_MINOR(capabilities.identity.vulkanApiVersion)) + "." +
                         std::to_string(VK_API_VERSION_PATCH(capabilities.identity.vulkanApiVersion));
    metadata.rtMode = horde::vulkan::ToString(capabilities.rtMode);
    metadata.presentMode = PresentModeName(context.swapchainPresentMode);
    metadata.materialEncoding = context.rtScene.MaterialEncoding();
    metadata.renderScalePercent = static_cast<std::uint32_t>(std::lround(context.renderScale * 100.0f));
    metadata.internalWidth = context.rtScene.DispatchExtent().width;
    metadata.internalHeight = context.rtScene.DispatchExtent().height;
    metadata.presentationWidth = context.swapchainExtent.width;
    metadata.presentationHeight = context.swapchainExtent.height;
    return metadata;
}

void UpdateBenchmarkHud(VulkanSurfaceContext& context)
{
    if (HWND hud = GetDlgItem(context.windowHandle, kHudControlId))
    {
        const std::string text = context.benchmark.ProgressText() + "  |  ESC CANCELS";
        SetWindowTextA(hud, text.c_str());
    }
}

void StartBenchmark(VulkanSurfaceContext& context)
{
    ResetRoute(context);
    context.benchmark.Start();
    context.rtLabRouteTainted = true;
    context.benchmarkCompletionHandled = false;
    context.benchmarkReport.clear();
    context.benchmarkJsonReport.clear();
    context.pauseMenuVisible = false;
    context.settingsVisible = false;
    context.diagnosticsVisible = false;
    context.benchmarkReportVisible = false;
    ApplyOverlayState(context);
    UpdateBenchmarkHud(context);
    PlaySoundEffect(context, "ui_select.wav");
    SetFocus(context.windowHandle);
}

void CancelBenchmark(VulkanSurfaceContext& context, const bool showMenu)
{
    if (!context.benchmark.IsRunning())
    {
        return;
    }
    context.benchmark.Cancel();
    ResetRoute(context);
    if (HWND hud = GetDlgItem(context.windowHandle, kHudControlId))
    {
        SetWindowTextA(hud, kHudActiveText);
    }
    if (showMenu)
    {
        ShowPauseMenu(context, true);
    }
}

void CompleteBenchmark(VulkanSurfaceContext& context,
                       const horde::vulkan::DeviceCapabilities& capabilities,
                       const std::filesystem::path& reportDirectory)
{
    if (context.benchmarkCompletionHandled)
    {
        return;
    }
    context.benchmarkCompletionHandled = true;
    const horde::gameplay::ShowcaseBenchmarkMetadata metadata =
        BuildBenchmarkMetadata(context, capabilities);
    context.benchmarkReport = context.benchmark.BuildTextReport(metadata);
    context.benchmarkJsonReport = context.benchmark.BuildJsonReport(metadata);
    const std::string stamp = UtcTimestamp("%Y%m%d-%H%M%S");
    const std::filesystem::path textPath = reportDirectory / ("HordeLanternRT-benchmark-" + stamp + ".txt");
    const std::filesystem::path jsonPath = reportDirectory / ("HordeLanternRT-benchmark-" + stamp + ".json");
    const bool jsonSaved = WriteReportFile(jsonPath, context.benchmarkJsonReport);
    context.benchmarkReport += "\nSaved text report: " + textPath.string() +
        "\nSaved JSON report: " + (jsonSaved ? jsonPath.string() : std::string("FAILED")) +
        "\nUse the buttons below or Ctrl+A, Ctrl+C to copy.\n";
    if (!WriteReportFile(textPath, context.benchmarkReport))
    {
        context.benchmarkReport += "WARNING: automatic text report save failed; COPY REPORT and SAVE AS remain available.\n";
    }
    ResetRoute(context);
    context.pauseMenuVisible = false;
    context.settingsVisible = false;
    context.diagnosticsVisible = false;
    context.benchmarkReportVisible = true;
    if (HWND edit = GetDlgItem(context.windowHandle, kEditControlId))
    {
        SetWindowTextA(edit, WindowSafeText(context.benchmarkReport).c_str());
    }
    ApplyOverlayState(context);
    RECT client{};
    GetClientRect(context.windowHandle, &client);
    LayoutOverlayControls(context.windowHandle, client.right - client.left, client.bottom - client.top);
    SetFocus(GetDlgItem(context.windowHandle, kEditControlId));
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

#if defined(_DEBUG)
std::string PackedVulkanVersion(const std::uint32_t version)
{
    return std::to_string(VK_API_VERSION_MAJOR(version)) + "." +
           std::to_string(VK_API_VERSION_MINOR(version)) + "." +
           std::to_string(VK_API_VERSION_PATCH(version));
}

const char* EncounterStatusName(const horde::gameplay::EncounterStatus status)
{
    switch (status)
    {
    case horde::gameplay::EncounterStatus::Active: return "active";
    case horde::gameplay::EncounterStatus::Dead: return "dead";
    default: return "inactive";
    }
}

const horde::gameplay::EnemyEncounterSnapshot* SelectedEncounter(
    const horde::gameplay::EnemyRosterSnapshot& roster)
{
    for (const horde::gameplay::EnemyEncounterSnapshot& encounter : roster.encounters)
    {
        if (encounter.kind == roster.selectedEnemy)
        {
            return &encounter;
        }
    }
    return nullptr;
}

horde::ui::DeveloperOverlaySnapshot BuildDeveloperOverlaySnapshot(
    const VulkanSurfaceContext& context,
    const horde::vulkan::DeviceCapabilities& capabilities)
{
    const horde::gameplay::simulation::SimulationSnapshot& simulation = context.simulation.Snapshot();
    const horde::gameplay::EnemyRosterSnapshot& roster = simulation.enemyRoster;
    const horde::gameplay::LichSnapshot& lich = simulation.lich;
    const horde::gameplay::EnemyEncounterSnapshot* encounter = SelectedEncounter(roster);
    horde::ui::DeveloperOverlaySnapshot snapshot;
    snapshot.buildIdentity = std::string(HORDE_RT_BUILD_ID) + " DEBUG";
    snapshot.shaderIdentity = std::string(HORDE_RT_RAYGEN_SHA256).substr(0u, 12u);
    snapshot.gpuName = capabilities.identity.gpuName;
    snapshot.vulkanApi = PackedVulkanVersion(capabilities.identity.vulkanApiVersion);
    snapshot.rtMode = horde::vulkan::ToString(capabilities.rtMode);
    snapshot.routeZone = horde::gameplay::ShowcaseZoneName(
        horde::gameplay::QueryShowcaseZone(context.cameraX, context.cameraZ));
    snapshot.materialEncoding = context.rtScene.MaterialEncoding();
    snapshot.torchFailurePhase = horde::gameplay::TorchFailurePhaseName(context.torchFailureSnapshot.phase);
    snapshot.selectedEnemy = horde::gameplay::EnemyKindName(roster.selectedEnemy);
    snapshot.encounterPhase = encounter ? EncounterStatusName(encounter->status) : "inactive";
    if (roster.selectedEnemy == horde::gameplay::EnemyKind::Lich)
    {
        snapshot.encounterPhase = horde::gameplay::LichPhaseName(lich.phase);
        snapshot.enemyHealth = lich.health;
    }
    const horde::gameplay::PlayerVitalsSnapshot& player = simulation.playerVitals;
    snapshot.playerLifePhase = horde::gameplay::PlayerLifePhaseName(player.phase);
    snapshot.playerVitality = player.vitality;
    snapshot.playerMaxVitality = player.maxVitality;
    snapshot.playerDamageEnabled = IsPlayerDamageEnabled(context);
    snapshot.simulationTicksThisFrame = simulation.simulationTicksThisFrame;
    snapshot.fixedStepAccumulatorSeconds = simulation.fixedStepAccumulatorSeconds;
    snapshot.catchUpOverrunCount = simulation.catchUpOverrunCount;
    snapshot.queuedEventCount = simulation.queuedEventCount;
    snapshot.eventQueueHighWaterMark = simulation.eventQueueHighWaterMark;
    snapshot.eventQueueOverflowCount =
        simulation.eventQueueOverflowCount + context.delayedFeedback.OverflowCount();
    snapshot.inputPublicationSequence = simulation.inputPublicationSequence;
    snapshot.consumedAttackSequence = simulation.lastConsumedAttackSequence;
    snapshot.consumedParrySequence = simulation.lastConsumedParrySequence;
    snapshot.consumedRouteResetSequence = simulation.lastConsumedRouteResetSequence;
    snapshot.consumedRetrySequence = simulation.lastConsumedRetrySequence;
    snapshot.internalWidth = capabilities.performance.internalRenderWidth;
    snapshot.internalHeight = capabilities.performance.internalRenderHeight;
    snapshot.presentationWidth = context.swapchainExtent.width;
    snapshot.presentationHeight = context.swapchainExtent.height;
    snapshot.blasCount = context.rtScene.BlasCount();
    snapshot.tlasCount = context.rtScene.TlasCount();
    snapshot.tlasInstanceCount = context.rtScene.TlasInstanceCount();
    snapshot.activeSkinnedEnemies = simulation.activeEnemyKind == horde::gameplay::EnemyKind::Skeleton
        ? static_cast<std::uint32_t>(simulation.activeSkeletonCount)
        : static_cast<std::uint32_t>(roster.renderedEnemyCount);
    snapshot.activeEnemyEntityCount = simulation.activeEnemyKind == horde::gameplay::EnemyKind::Skeleton
        ? static_cast<std::uint32_t>(simulation.activeSkeletonCount)
        : static_cast<std::uint32_t>(roster.renderedEnemyCount);
    snapshot.attackerEntityId = simulation.skeletonAttackerId == horde::gameplay::simulation::EntityId::Invalid
        ? -1
        : static_cast<std::int32_t>(simulation.skeletonAttackerId);
    snapshot.playerCombatAction = simulation.playerCombat.action;
    for (std::size_t index = 0u; index < simulation.skeletonEnemyCount; ++index)
    {
        if (simulation.skeletonEnemies[index].id == simulation.skeletonAttackerId)
        {
            snapshot.attackerCombatAction = simulation.skeletonEnemies[index].action;
            snapshot.hasAttackerCombatAction = true;
            break;
        }
    }
    snapshot.skeletonPoseBucketCount = static_cast<std::uint32_t>(context.rtScene.SkeletonPoseBucketCount());
    snapshot.renderScale = context.renderScale;
    snapshot.fps = capabilities.performance.fps;
    snapshot.frameTimeMs = capabilities.performance.frameTimeMs;
    snapshot.gpuRtTimingValid = context.gpuRtTiming.valid;
    snapshot.gpuRtLatestMs = context.gpuRtTiming.latestMs;
    snapshot.gpuRtAverageMs = context.gpuRtTiming.averageMs;
    snapshot.gpuRtSampleCount = context.gpuRtTiming.sampleCount;
    snapshot.gpuRtTimingStatus = context.gpuRtTiming.status;
    snapshot.presented = capabilities.rtScene.presented;
    return snapshot;
}

void RefreshDeveloperOverlay(VulkanSurfaceContext& context,
                             const horde::vulkan::DeviceCapabilities& capabilities,
                             const bool force = false)
{
    if (!context.developerOverlayVisible || context.benchmark.IsRunning())
    {
        return;
    }
    const ULONGLONG now = GetTickCount64();
    if (!force && now - context.lastDeveloperOverlayTick < 250u)
    {
        return;
    }
    context.lastDeveloperOverlayTick = now;
    if (HWND overlay = GetDlgItem(context.windowHandle, kDeveloperOverlayId))
    {
        const std::string text = WindowSafeText(
            horde::ui::BuildDeveloperOverlayText(BuildDeveloperOverlaySnapshot(context, capabilities)));
        SetWindowTextA(overlay, text.c_str());
    }
}

void ToggleDeveloperOverlay(VulkanSurfaceContext& context)
{
    context.developerOverlayVisible = !context.developerOverlayVisible;
    context.lastDeveloperOverlayTick = 0u;
    ApplyOverlayState(context);
}
#endif

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
    context.controllerForward = 0.0f;
    context.controllerStrafe = 0.0f;
    context.controllerLookHorizontal = 0.0f;
    context.controllerLookVertical = 0.0f;
    context.previousControllerButtons = 0u;
    context.previousLegacyControllerButtons = 0u;
    context.controllerTriggerLatch = {};
    context.xInputUserIndex.reset();
    context.legacyJoystickId.reset();
    context.legacyRightStickAxes = {};
    ClipCursor(nullptr);
    if (context.mouseCursorHidden)
    {
        ShowCursor(TRUE);
        SetCursorPos(context.mouseRestorePosition.x, context.mouseRestorePosition.y);
        context.mouseCursorHidden = false;
    }
    if (GetCapture() == context.windowHandle)
    {
        ReleaseCapture();
    }
}

std::vector<HWND> VisibleControllerMenuControls(const VulkanSurfaceContext& context)
{
    constexpr std::array<int, 35u> controlIds{{
        kResumeButtonId, kRestartButtonId, kControlsButtonId, kSettingsButtonId,
        kRtLabButtonId, kDiagnosticsButtonId, kRunBenchmarkButtonId, kMoreBySamfa12ButtonId,
        kExitButtonId, kSfxButtonId, kSensitivityButtonId, kWaterQualityButtonId,
        kRenderScaleSliderId, kFullscreenButtonId, kSettingsBackButtonId,
        kBenchmarkCopyButtonId, kBenchmarkSaveButtonId, kBenchmarkBackButtonId,
        kRtLabWaterfallSliderId, kRtLabRoofSliderId, kRtLabDawnSliderId,
        kRtLabFogSliderId, kRtLabFireStrengthSliderId, kRtLabFireTurbulenceSliderId,
        kRtLabFireSmokeSliderId, kRtLabGlassVisibilitySliderId,
        kRtLabGlassTransmissionSliderId, kRtLabGlassIorSliderId,
        kRtLabGlassRoughnessSliderId, kRtLabLightGroupButtonId, kRtLabHueSliderId,
        kRtLabIntensitySliderId, kRtLabWorkloadButtonId, kRtLabRestoreButtonId,
        kRtLabBackButtonId,
    }};
    std::vector<HWND> controls;
    controls.reserve(controlIds.size());
    for (const int id : controlIds)
    {
        HWND control = GetDlgItem(context.windowHandle, id);
        const bool labControl = id == kRtLabWaterfallSliderId || id == kRtLabRoofSliderId ||
            id == kRtLabDawnSliderId || id == kRtLabFogSliderId ||
            id == kRtLabFireStrengthSliderId || id == kRtLabFireTurbulenceSliderId ||
            id == kRtLabFireSmokeSliderId || id == kRtLabGlassVisibilitySliderId ||
            id == kRtLabGlassTransmissionSliderId || id == kRtLabGlassIorSliderId ||
            id == kRtLabGlassRoughnessSliderId ||
            id == kRtLabLightGroupButtonId || id == kRtLabHueSliderId ||
            id == kRtLabIntensitySliderId || id == kRtLabWorkloadButtonId ||
            id == kRtLabRestoreButtonId || id == kRtLabBackButtonId;
        if (control != nullptr && IsWindowEnabled(control) &&
            (IsWindowVisible(control) || (context.rtLabVisible && labControl)))
        {
            controls.push_back(control);
        }
    }
    return controls;
}

void NavigateControllerMenu(VulkanSurfaceContext& context, const int direction)
{
    const std::vector<HWND> controls = VisibleControllerMenuControls(context);
    if (controls.empty())
    {
        return;
    }
    HWND focused = GetFocus();
    auto found = std::find(controls.begin(), controls.end(), focused);
    std::size_t index = found == controls.end()
        ? (direction < 0 ? controls.size() - 1u : 0u)
        : static_cast<std::size_t>(std::distance(controls.begin(), found));
    if (found != controls.end())
    {
        index = horde::platform::windows::WrapRtLabFocus(index, direction, controls.size());
    }
    if (context.rtLabVisible)
    {
        RECT focusedRect{};
        RECT panelRect{};
        GetWindowRect(controls[index], &focusedRect);
        GetWindowRect(GetDlgItem(context.windowHandle, kRtLabPanelId), &panelRect);
        MapWindowPoints(HWND_DESKTOP, context.windowHandle,
                        reinterpret_cast<POINT*>(&focusedRect), 2);
        MapWindowPoints(HWND_DESKTOP, context.windowHandle,
                        reinterpret_cast<POINT*>(&panelRect), 2);
        if (focusedRect.top < panelRect.top + ScaleForDpi(context.windowHandle, 8))
            context.rtLabScrollOffset = std::max(0, context.rtLabScrollOffset -
                static_cast<int>(panelRect.top + ScaleForDpi(context.windowHandle, 8) - focusedRect.top));
        else if (focusedRect.bottom > panelRect.bottom - ScaleForDpi(context.windowHandle, 8))
            context.rtLabScrollOffset += static_cast<int>(focusedRect.bottom -
                (panelRect.bottom - ScaleForDpi(context.windowHandle, 8)));
        RECT client{};
        GetClientRect(context.windowHandle, &client);
        LayoutOverlayControls(context.windowHandle, client.right, client.bottom);
    }
    SetFocus(controls[index]);
    if (horde::platform::windows::ShouldPlayControllerMenuSound(context.rtLabVisible))
        PlaySoundEffect(context, "ui_select.wav");
}

void CancelControllerMenu(VulkanSurfaceContext& context)
{
    int command = kResumeButtonId;
    if (context.rtLabVisible)
    {
        command = kRtLabBackButtonId;
    }
    else if (context.settingsVisible)
    {
        command = kSettingsBackButtonId;
    }
    else if (context.diagnosticsVisible)
    {
        command = kMenuDiagnosticsId;
    }
    else if (context.benchmarkReportVisible)
    {
        command = kBenchmarkBackButtonId;
    }
    PostMessageA(context.windowHandle, WM_COMMAND, MAKEWPARAM(command, BN_CLICKED), 0);
}

bool AdjustFocusedControllerSlider(VulkanSurfaceContext& context, const bool increase)
{
    HWND focused = GetFocus();
    if (focused == nullptr)
    {
        return false;
    }
    const int id = GetDlgCtrlID(focused);
    if (id == kRtLabLightGroupButtonId)
    {
        const std::uint32_t count = static_cast<std::uint32_t>(horde::vulkan::raytracing::kRtLightGroupCount);
        const std::uint32_t current = static_cast<std::uint32_t>(context.rtLabLightGroup);
        context.rtLabLightGroup = static_cast<horde::vulkan::raytracing::RtLightGroup>(
            increase ? (current + 1u) % count : (current + count - 1u) % count);
        UpdateRtLabLabels(context);
        return true;
    }
    if (id == kRtLabWorkloadButtonId)
    {
        const int current = static_cast<int>(context.rtSceneTuning.workloadPreset);
        context.rtSceneTuning.workloadPreset = static_cast<horde::vulkan::raytracing::RtWorkloadPreset>(
            std::clamp(current + (increase ? 1 : -1), 0, 2));
        UpdateRtLabLabels(context);
        return true;
    }
    horde::platform::windows::RtLabControlRange range =
        horde::platform::windows::RtLabControlRange::DoublePercent;
    bool rtLabSlider = true;
    switch (id)
    {
    case kRtLabWaterfallSliderId: range = horde::platform::windows::RtLabControlRange::WaterfallPercent; break;
    case kRtLabRoofSliderId:
    case kRtLabDawnSliderId:
    case kRtLabGlassVisibilitySliderId:
    case kRtLabGlassTransmissionSliderId:
    case kRtLabGlassRoughnessSliderId:
        range = horde::platform::windows::RtLabControlRange::UnitPercent; break;
    case kRtLabGlassIorSliderId:
        range = horde::platform::windows::RtLabControlRange::IorHundredths; break;
    case kRtLabFogSliderId:
    case kRtLabFireStrengthSliderId:
    case kRtLabFireTurbulenceSliderId:
    case kRtLabFireSmokeSliderId:
    case kRtLabIntensitySliderId: range = horde::platform::windows::RtLabControlRange::DoublePercent; break;
    case kRtLabHueSliderId: range = horde::platform::windows::RtLabControlRange::HueDegrees; break;
    default: rtLabSlider = false; break;
    }
    if (!rtLabSlider && id != kRenderScaleSliderId) return false;
    const int current = static_cast<int>(SendMessageA(focused, TBM_GETPOS, 0, 0));
    const int next = rtLabSlider
        ? horde::platform::windows::StepRtLabControl(current, increase, range)
        : horde::platform::windows::StepControllerSlider(current, increase);
    if (next != current)
    {
        SendMessageA(focused, TBM_SETPOS, TRUE, next);
        SendMessageA(context.windowHandle, WM_HSCROLL,
                     MAKEWPARAM(TB_ENDTRACK, next),
                     reinterpret_cast<LPARAM>(focused));
        if (horde::platform::windows::ShouldPlayControllerMenuSound(context.rtLabVisible))
            PlaySoundEffect(context, "ui_select.wav");
    }
    return true;
}

void HandleControllerMenuEdges(
    VulkanSurfaceContext& context,
    const horde::platform::windows::ControllerMenuEdges& edges)
{
    if (edges.togglePause)
    {
        if (context.rtLabVisible)
        {
            CloseRtLab(context);
            return;
        }
        PostMessageA(context.windowHandle, WM_COMMAND,
                     MAKEWPARAM(kMenuPauseId, BN_CLICKED), 0);
        return;
    }
    if (!context.simulationPaused)
    {
        return;
    }
    if (edges.decrease || edges.increase)
    {
        if (!AdjustFocusedControllerSlider(context, edges.increase))
        {
            NavigateControllerMenu(context, edges.increase ? 1 : -1);
        }
    }
    else if (edges.previous)
    {
        NavigateControllerMenu(context, -1);
    }
    else if (edges.next)
    {
        NavigateControllerMenu(context, 1);
    }
    else if (edges.confirm)
    {
        HWND focused = GetFocus();
        const std::vector<HWND> controls = VisibleControllerMenuControls(context);
        if (std::find(controls.begin(), controls.end(), focused) == controls.end())
        {
            if (!controls.empty()) SetFocus(controls.front());
        }
        else
        {
            SendMessageA(focused, BM_CLICK, 0, 0);
        }
    }
    else if (edges.cancel)
    {
        CancelControllerMenu(context);
    }
}

using XInputGetStateProc = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);

void PollDesktopController(VulkanSurfaceContext& context)
{
    static XInputGetStateProc getState = []() -> XInputGetStateProc
    {
        for (const wchar_t* library : {L"xinput1_4.dll", L"xinput9_1_0.dll", L"xinput1_3.dll"})
        {
            if (HMODULE module = LoadLibraryW(library))
            {
                if (auto proc = reinterpret_cast<XInputGetStateProc>(GetProcAddress(module, "XInputGetState"))) return proc;
            }
        }
        return nullptr;
    }();
    XINPUT_STATE state{};
    std::optional<DWORD> xinputUser;
    if (getState != nullptr)
    {
        if (context.xInputUserIndex.has_value() &&
            getState(*context.xInputUserIndex, &state) == ERROR_SUCCESS)
        {
            xinputUser = context.xInputUserIndex;
        }
        else
        {
            for (DWORD user = 0u; user < XUSER_MAX_COUNT; ++user)
            {
                if (getState(user, &state) == ERROR_SUCCESS)
                {
                    xinputUser = user;
                    break;
                }
            }
        }
    }
    const bool xinputConnected = xinputUser.has_value();
    if (!xinputConnected)
    {
        context.xInputUserIndex.reset();
        JOYINFOEX legacy{};
        const auto pollLegacy = [&legacy](const UINT joystick)
        {
            legacy = {};
            legacy.dwSize = sizeof(legacy);
            legacy.dwFlags = JOY_RETURNALL;
            return joyGetPosEx(joystick, &legacy) == JOYERR_NOERROR;
        };
        std::optional<UINT> joystick;
        if (context.legacyJoystickId.has_value() && pollLegacy(*context.legacyJoystickId))
        {
            joystick = context.legacyJoystickId;
        }
        else
        {
            for (UINT candidate = 0u; candidate < joyGetNumDevs(); ++candidate)
            {
                if (pollLegacy(candidate))
                {
                    joystick = candidate;
                    break;
                }
            }
        }
        if (!joystick.has_value())
        {
            context.controllerStrafe = 0.0f;
            context.controllerForward = 0.0f;
            context.controllerLookHorizontal = 0.0f;
            context.controllerLookVertical = 0.0f;
            context.previousControllerButtons = 0u;
            context.previousLegacyUiButtons = 0u;
            context.previousLegacyPov = JOY_POVCENTERED;
            context.legacyJoystickId.reset();
            context.legacyRightStickAxes = {};
            return;
        }
        JOYCAPSA caps{};
        if (joyGetDevCapsA(*joystick, &caps, sizeof(caps)) != JOYERR_NOERROR)
        {
            return;
        }
        const auto normaliseRaw = [](const DWORD value, const UINT minimum, const UINT maximum)
        {
            const float centered = (float(value) - (float(minimum) + float(maximum)) * 0.5f) / std::max(1.0f, (float(maximum) - float(minimum)) * 0.5f);
            return std::clamp(centered, -1.0f, 1.0f);
        };
        const auto applyDeadzone = [](const float value)
        {
            return std::abs(value) > 0.16f ? value : 0.0f;
        };
        context.controllerStrafe = applyDeadzone(normaliseRaw(legacy.dwXpos, caps.wXmin, caps.wXmax));
        context.controllerForward = -applyDeadzone(normaliseRaw(legacy.dwYpos, caps.wYmin, caps.wYmax));

        const horde::platform::windows::LegacyAxisSample axisSample{
            .z = normaliseRaw(legacy.dwZpos, caps.wZmin, caps.wZmax),
            .r = normaliseRaw(legacy.dwRpos, caps.wRmin, caps.wRmax),
            .u = normaliseRaw(legacy.dwUpos, caps.wUmin, caps.wUmax),
            .v = normaliseRaw(legacy.dwVpos, caps.wVmin, caps.wVmax),
            .hasZ = (caps.wCaps & JOYCAPS_HASZ) != 0u && caps.wZmax > caps.wZmin,
            .hasR = (caps.wCaps & JOYCAPS_HASR) != 0u && caps.wRmax > caps.wRmin,
            .hasU = (caps.wCaps & JOYCAPS_HASU) != 0u && caps.wUmax > caps.wUmin,
            .hasV = (caps.wCaps & JOYCAPS_HASV) != 0u && caps.wVmax > caps.wVmin,
        };
        const horde::platform::windows::LegacyControllerIdentity identity{
            .vendorId = caps.wMid,
            .productId = caps.wPid,
            .productName = caps.szPname,
        };
        if (context.legacyJoystickId != joystick ||
            context.legacyRightStickAxes.horizontal == horde::platform::windows::LegacyAxis::None)
        {
            context.legacyRightStickAxes =
                horde::platform::windows::SelectLegacyRightStickAxes(axisSample, identity);
            context.previousControllerButtons = 0u;
            context.previousLegacyControllerButtons = 0u;
            context.previousLegacyUiButtons = legacy.dwButtons;
            context.previousLegacyPov = legacy.dwPOV;
            context.controllerTriggerLatch = {};
            std::ostringstream controllerDiagnostic;
            controllerDiagnostic << "WinMM controller connected: id=" << *joystick
                                 << ", vendor=0x" << std::hex << caps.wMid
                                 << ", product=0x" << caps.wPid << std::dec
                                 << ", name=" << caps.szPname
                                 << ", axes=" << caps.wNumAxes
                                 << ", buttons=" << caps.wNumButtons
                                 << ", look=" << static_cast<int>(context.legacyRightStickAxes.horizontal)
                                 << '/' << static_cast<int>(context.legacyRightStickAxes.vertical);
            LogWindowsAudio(controllerDiagnostic.str());
        }
        context.legacyJoystickId = joystick;
        context.controllerLookHorizontal = applyDeadzone(horde::platform::windows::LegacyAxisValue(
            axisSample, context.legacyRightStickAxes.horizontal));
        context.controllerLookVertical = applyDeadzone(horde::platform::windows::LegacyAxisValue(
            axisSample, context.legacyRightStickAxes.vertical));
        const horde::platform::windows::ControllerActionEdges edges =
            horde::platform::windows::MapLegacyControllerEdges(
                legacy.dwButtons, context.previousLegacyControllerButtons, identity);
        if (!context.simulationPaused)
        {
            if (edges.attackPressed) ++context.attackSequence;
            if (edges.parryPressed) ++context.parrySequence;
            if (edges.dodgePressed) ++context.dodgeSequence;
            if (edges.interactPressed) ++context.interactSequence;
            if (edges.toggleHeldLightPosePressed) ++context.toggleHeldLightPoseSequence;
        }
        const horde::platform::windows::ControllerMenuEdges menuEdges =
            horde::platform::windows::MapLegacyControllerMenuEdges(
                legacy.dwButtons, context.previousLegacyUiButtons,
                legacy.dwPOV, context.previousLegacyPov, identity);
        context.previousLegacyControllerButtons = legacy.dwButtons;
        context.previousLegacyUiButtons = legacy.dwButtons;
        context.previousLegacyPov = legacy.dwPOV;
        HandleControllerMenuEdges(context, menuEdges);
        return;
    }
    if (context.xInputUserIndex != xinputUser)
    {
        context.previousControllerButtons = 0u;
        context.previousXInputUiButtons = state.Gamepad.wButtons;
        context.previousLegacyControllerButtons = 0u;
        context.controllerTriggerLatch = {};
    }
    context.xInputUserIndex = xinputUser;
    context.legacyJoystickId.reset();
    context.legacyRightStickAxes = {};
    context.previousLegacyUiButtons = 0u;
    context.previousLegacyPov = JOY_POVCENTERED;
    const auto axis = [](SHORT value, SHORT deadzone)
    {
        const float magnitude = static_cast<float>(value) / 32767.0f;
        return std::abs(magnitude) > static_cast<float>(deadzone) / 32767.0f ? magnitude : 0.0f;
    };
    context.controllerStrafe = axis(state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    context.controllerForward = axis(state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    context.controllerLookHorizontal = axis(state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    context.controllerLookVertical = -axis(state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    const WORD pressed = state.Gamepad.wButtons & ~context.previousControllerButtons;
    const horde::platform::windows::ControllerActionEdges triggerEdges =
        horde::platform::windows::UpdateXInputTriggerEdges(
            state.Gamepad.bLeftTrigger,
            state.Gamepad.bRightTrigger,
            context.controllerTriggerLatch);
    if (!context.simulationPaused)
    {
        if (triggerEdges.attackPressed) ++context.attackSequence;
        if (triggerEdges.parryPressed) ++context.parrySequence;
        if ((pressed & XINPUT_GAMEPAD_B) != 0u) ++context.dodgeSequence;
        if ((pressed & XINPUT_GAMEPAD_A) != 0u) ++context.interactSequence;
        if ((pressed & XINPUT_GAMEPAD_Y) != 0u) ++context.toggleHeldLightPoseSequence;
    }
    const WORD uiPressed = state.Gamepad.wButtons & ~context.previousXInputUiButtons;
    const horde::platform::windows::ControllerMenuEdges menuEdges{
        .previous = (uiPressed & XINPUT_GAMEPAD_DPAD_UP) != 0u,
        .next = (uiPressed & XINPUT_GAMEPAD_DPAD_DOWN) != 0u,
        .decrease = (uiPressed & XINPUT_GAMEPAD_DPAD_LEFT) != 0u,
        .increase = (uiPressed & XINPUT_GAMEPAD_DPAD_RIGHT) != 0u,
        .confirm = (uiPressed & XINPUT_GAMEPAD_A) != 0u,
        .cancel = (uiPressed & XINPUT_GAMEPAD_B) != 0u,
        .togglePause = (uiPressed & XINPUT_GAMEPAD_START) != 0u,
    };
    context.previousControllerButtons = state.Gamepad.wButtons;
    context.previousXInputUiButtons = state.Gamepad.wButtons;
    HandleControllerMenuEdges(context, menuEdges);
}

void UpdateDesktopSceneControls(VulkanSurfaceContext& context)
{
    const int previousVitality = context.simulation.Snapshot().playerVitals.vitality;
    const horde::gameplay::PlayerLifePhase previousLifePhase =
        context.simulation.Snapshot().playerVitals.phase;
    const ULONGLONG now = GetTickCount64();
    float deltaSeconds = 1.0f / 60.0f;
    if (context.lastControlTick != 0u)
    {
        deltaSeconds = std::clamp(static_cast<float>(now - context.lastControlTick) / 1000.0f, 0.0f, 0.1f);
    }
    context.lastControlTick = now;
    context.frameDeltaSeconds = deltaSeconds;

    if (!context.simulationPaused)
    {
        const horde::platform::windows::ControllerView view =
            horde::platform::windows::ApplyControllerLook(
                context.cameraYaw,
                context.cameraPitch,
                context.controllerLookHorizontal,
                context.controllerLookVertical,
                deltaSeconds);
        context.cameraYaw = view.yawRadians;
        context.cameraPitch = view.pitchRadians;
    }

    horde::gameplay::simulation::InputSnapshot input = context.simulationInput;
    input.yawRadians = context.cameraYaw;
    input.pitchRadians = context.cameraPitch;
    input.torchLightStrength = context.torchLightStrength;
    input.paused = context.simulationPaused;
    input.damageEnabled = IsPlayerDamageEnabled(context);
    input.commands.attack = context.attackSequence;
    input.commands.parry = context.parrySequence;
    input.commands.dodge = context.dodgeSequence;
    input.commands.routeReset = context.routeResetSequence;
    input.commands.retry = context.retrySequence;
    input.commands.interact = context.interactSequence;
    input.commands.toggleHeldLightPose = context.toggleHeldLightPoseSequence;
    input.hasAuthoritativePlayerPose = false;
    input.moveForward = (context.forwardHeld ? 1.0f : 0.0f) -
                        (context.backwardHeld ? 1.0f : 0.0f) + context.controllerForward;
    input.moveStrafe = (context.rightHeld ? 1.0f : 0.0f) -
                       (context.leftHeld ? 1.0f : 0.0f) + context.controllerStrafe;

    if (context.benchmark.IsRunning())
    {
        context.frameDeltaSeconds = 1.0f / 60.0f;
        ClearDesktopInput(context);
        const horde::gameplay::ShowcaseBenchmarkAdvance advance = context.benchmark.Advance();
        if (advance.lapStarted)
        {
            ResetRoute(context);
        }
        input = context.simulationInput;
        input.paused = false;
        input.damageEnabled = false;
        input.hasAuthoritativePlayerPose = true;
        input.authoritativePlayerX = advance.replay.x;
        input.authoritativePlayerZ = advance.replay.z;
        input.yawRadians = advance.replay.yaw;
        input.pitchRadians = -0.04f;
        input.torchLightStrength = context.torchLightStrength;
        input.commands.attack = context.attackSequence;
        input.commands.parry = context.parrySequence;
        input.commands.dodge = context.dodgeSequence;
        input.commands.routeReset = context.routeResetSequence;
        input.commands.retry = context.retrySequence;
        input.commands.interact = context.interactSequence;
        input.commands.toggleHeldLightPose = context.toggleHeldLightPoseSequence;
        if (advance.replay.waypointReached || advance.lapStarted || advance.finished)
        {
            UpdateBenchmarkHud(context);
        }
    }

    context.simulationInput = input;
    context.simulation.AdvanceFrame(input,
                                    context.frameDeltaSeconds,
                                    ++context.inputPublicationSequence);
    // Camera yaw/pitch are continuous platform input targets. A render frame
    // may not produce a 60 Hz simulation tick, so copying the previous fixed
    // snapshot back here would erase right-stick and mouse look accumulated
    // between ticks. Reset/checkpoint paths still request a full view mirror.
    MirrorSimulationSnapshot(context, false);
    if (context.simulation.Snapshot().playerVitals.vitality != previousVitality ||
        context.simulation.Snapshot().playerVitals.phase != previousLifePhase)
    {
        UpdateVitalityHud(context);
    }
}

#if defined(_DEBUG)
void DebugWarpSimulation(VulkanSurfaceContext& context,
                         float x,
                         float z,
                         float yaw,
                         float pitch)
{
    horde::gameplay::simulation::InputSnapshot input = context.simulationInput;
    input.paused = false;
    input.damageEnabled = false;
    input.hasAuthoritativePlayerPose = true;
    input.authoritativePlayerX = x;
    input.authoritativePlayerZ = z;
    input.yawRadians = yaw;
    input.pitchRadians = pitch;
    input.torchLightStrength = context.torchLightStrength;
    context.simulation.StepFixed(input, 0.0f, ++context.inputPublicationSequence);
    context.simulation.ResetTiming();
    context.simulation.ClearEvents();
    context.simulationInput = input;
    context.simulationInput.hasAuthoritativePlayerPose = false;
    context.simulationInput.paused = context.simulationPaused;
    MirrorSimulationSnapshot(context);
}
#endif

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
    ctx.swapchainPresentMode = chosenPresentMode;

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
    ctx.gpuFrameTimer.ResetAfterDeviceIdle();
    ctx.gpuFrameTimingTotalMs = 0.0;
    ctx.gpuFrameTimingSampleCount = 0u;
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

void RefreshGpuTimingTelemetry(
    VulkanSurfaceContext& ctx,
    const std::optional<horde::vulkan::GpuFrameTimingSample>& completedSample = std::nullopt)
{
    if (completedSample.has_value())
    {
        ctx.gpuFrameTimingTotalMs += completedSample->milliseconds;
        ++ctx.gpuFrameTimingSampleCount;
    }
    const horde::vulkan::GpuFrameTimerTelemetry& timer = ctx.gpuFrameTimer.Telemetry();
    auto& output = ctx.gpuRtTiming;
    output.status = timer.diagnostic;
    output.supported = ctx.gpuFrameTimer.Supported();
    output.valid = ctx.gpuFrameTimingSampleCount > 0u &&
        horde::vulkan::GpuFrameTimerHasCurrentSample(timer.status);
    output.latestMs = output.valid ? static_cast<float>(timer.latestMilliseconds) : 0.0f;
    output.averageMs = output.valid
        ? static_cast<float>(ctx.gpuFrameTimingTotalMs / static_cast<double>(ctx.gpuFrameTimingSampleCount))
        : 0.0f;
    output.timestampPeriodNanoseconds = timer.timestampPeriodNanoseconds;
    output.timestampValidBits = timer.timestampValidBits;
    output.sampleCount = ctx.gpuFrameTimingSampleCount;
    output.unavailableCount = timer.unavailableResultCount;
    output.errorCount = timer.errorCount;
}

bool InitialiseRtSceneForSwapchain(VulkanSurfaceContext& ctx)
{
    if (!ctx.useRtPath)
    {
        return true;
    }

    const std::filesystem::path assetRoot = ResolveAssetRoot();
    std::string developmentStaticAssetDirectory;
#if defined(_DEBUG) && defined(HORDE_RT_SOURCE_DIR)
    developmentStaticAssetDirectory =
        horde::vulkan::raytracing::ResolveDevelopmentStaticAssetDirectory(
            true,
            horde::vulkan::raytracing::UseGenericStaticAssetForCheckpoint(
                ctx.developmentCheckpoint),
            HORDE_RT_SOURCE_DIR).string();
#endif
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
                                diagnostic,
                                developmentStaticAssetDirectory,
                                assetRoot.string()))
    {
        std::cerr << "Failed to initialise presentable RT scene: " << diagnostic << '\n';
        MessageBoxA(ctx.windowHandle,
                    ("The native RT scene could not start.\n\n" + diagnostic +
                     "\n\nKeep the packaged assets folder beside HordeLanternRT.exe. No fallback renderer will be used.").c_str(),
                    "Horde Lantern RT - startup error",
                    MB_OK | MB_ICONERROR);
        return false;
    }
    if (ctx.gpuFrameTimer.Telemetry().status == horde::vulkan::GpuFrameTimerStatus::Uninitialised)
    {
        ctx.gpuFrameTimer.Initialise(
            ctx.physicalDevice, ctx.device, ctx.graphicsQueueFamilyIndex, kMaxFramesInFlight);
    }
    RefreshGpuTimingTelemetry(ctx);
    std::cout << "PBR material encoding: " << ctx.rtScene.MaterialEncoding() << '\n'
              << "RT render scale " << std::round(ctx.renderScale * 100.0f) << "%: "
              << renderExtent.width << 'x' << renderExtent.height << " -> "
              << ctx.swapchainExtent.width << 'x' << ctx.swapchainExtent.height << '\n' << std::flush;
    if (ctx.rtScene.GenericStaticAssetEnabled())
    {
        const auto& measurements = ctx.rtScene.StaticMeshMeasurements();
        std::cout << "Development static RT asset: vertexBytes=" << measurements.vertexBytes
                  << ", indexBytes=" << measurements.indexBytes
                  << ", materialBytes=" << measurements.materialBytes
                  << ", instanceMetadataBytes=" << measurements.instanceMetadataBytes
                  << ", primitiveMetadataBytes=" << measurements.primitiveMetadataBytes
                  << ", descriptors=" << measurements.descriptorCount
                  << ", textureBytes=" << ctx.rtScene.StaticTextureBytes()
                  << ", blasBytes=" << ctx.rtScene.StaticMeshBlasBytes()
                  << ", blasBuildMs=" << ctx.rtScene.StaticMeshBlasBuildMilliseconds()
                  << ", swordBlasBytes=" << ctx.rtScene.StaticMeshSwordBlasBytes()
                  << ", swordBlasBuildMs=" << ctx.rtScene.StaticMeshSwordBlasBuildMilliseconds()
                  << ", torchBlasBytes=" << ctx.rtScene.StaticMeshTorchBlasBytes()
                  << ", torchBlasBuildMs=" << ctx.rtScene.StaticMeshTorchBlasBuildMilliseconds()
                  << ", productionPropBlasBytes=" << ctx.rtScene.ProductionPropBlasBytes()
                  << ", productionPropBlasBuildMs=" << ctx.rtScene.ProductionPropBlasBuildMilliseconds()
                  << '\n' << std::flush;
    }

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
    ctx.gpuFrameTimer.Destroy();

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
    if (waitResult != VK_SUCCESS)
    {
        return false;
    }
    RefreshGpuTimingTelemetry(ctx, ctx.gpuFrameTimer.CollectCompleted(ctx.currentFrame));

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
    bool gpuTimingRecording = false;
    if (useRtFrame)
    {
        SpatialAudioEngine().Update();
        PollDesktopController(ctx);
        const bool frozenDevelopmentCheckpoint =
            ctx.simulationPaused && ctx.frameDeltaSeconds == 0.0f &&
            !ctx.developmentCheckpoint.empty();
        if (!frozenDevelopmentCheckpoint)
        {
            UpdateDesktopSceneControls(ctx);
        }
        UpdateWaterfallAmbience(ctx);
        const horde::gameplay::simulation::SimulationSnapshot& simulation =
            ctx.simulation.Snapshot();
        if (simulation.playerVitals.phase == horde::gameplay::PlayerLifePhase::Dead)
        {
            ShowDeathMenu(ctx);
        }
        if (simulation.finaleComplete)
        {
            TryGrantRtLabUnlock(ctx, true);
            ShowEndingMenu(ctx);
        }
        DrainGameplayEvents(ctx);
        horde::vulkan::raytracing::RtSceneFrameInputs frameInputs =
            horde::vulkan::raytracing::BuildRtSceneFrameInputs(
                simulation, ctx.outputExposure, ctx.waterQuality, ctx.rtSceneTuning);
        // Development-only player/glass proofs share the skinned route because
        // the generic fixture owns procedural slot 5. Authored captures retain
        // the established procedural phone-safe fallback.
        const horde::gameplay::DevelopmentCheckpoint* development =
            horde::gameplay::FindDevelopmentCheckpoint(ctx.developmentCheckpoint);
        const bool usesGlassFixture =
            development != nullptr && development->usesGlassFixture;
        const bool usesProductionRewardProps =
            development != nullptr && development->usesProductionRewardProps;
        frameInputs.playerRenderRoute =
            (ctx.developmentCheckpoint.starts_with("player-body-") || usesGlassFixture ||
             usesProductionRewardProps)
            ? horde::vulkan::raytracing::PlayerRenderRoute::Skinned
            : horde::vulkan::raytracing::PlayerRenderRoute::Procedural;
        if (usesGlassFixture)
        {
            frameInputs.tuning.glassFixtureVisible = true;
            frameInputs.tuning.glassDepthScale = development->glassDepthScale;
            frameInputs.tuning.glassAttenuationColor =
                development->glassAttenuationColor;
            frameInputs.tuning.glassAttenuationDistance =
                development->glassAttenuationDistance;
        }
        if (usesProductionRewardProps)
        {
            frameInputs.tuning.productionRewardPropsVisible = true;
            frameInputs.tuning.productionLanternGlassOnly =
                development->productionLanternGlassOnly;
        }
        if (ctx.debugEnemyOverride != horde::gameplay::EnemyKind::None)
        {
            // Debug-only renderer inspection remains non-authoritative gameplay.
            frameInputs.roster.selectedEnemy = ctx.debugEnemyOverride;
            frameInputs.roster.renderedEnemyCount = 1u;
            frameInputs.roster.renderedEnemies.fill(horde::gameplay::EnemyKind::None);
            frameInputs.roster.renderedEnemies[0] = ctx.debugEnemyOverride;
            if (ctx.debugEnemyOverride == horde::gameplay::EnemyKind::Lich)
            {
                frameInputs.skeletonEnemyCount = 0u;
            }
        }
        std::string diagnostic;
        gpuTimingRecording = ctx.gpuFrameTimer.RecordBegin(
            ctx.commandBuffers[imageIndex], ctx.currentFrame);
        if (!ctx.rtScene.RecordTraceAndCopy(ctx.commandBuffers[imageIndex],
                                            ctx.swapchainImages[imageIndex],
                                            ctx.swapchainImageLayouts[imageIndex],
                                            ctx.swapchainExtent,
                                            frameInputs,
                                            diagnostic))
        {
            if (gpuTimingRecording)
            {
                ctx.gpuFrameTimer.CancelRecording(ctx.currentFrame);
            }
            std::cerr << "Failed to record RT frame: " << diagnostic << '\n';
            ctx.lastRtFrameError = diagnostic;
            return false;
        }
        if (gpuTimingRecording &&
            !ctx.gpuFrameTimer.RecordEnd(ctx.commandBuffers[imageIndex], ctx.currentFrame))
        {
            ctx.gpuFrameTimer.CancelRecording(ctx.currentFrame);
            gpuTimingRecording = false;
            RefreshGpuTimingTelemetry(ctx);
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
        if (gpuTimingRecording) ctx.gpuFrameTimer.CancelRecording(ctx.currentFrame);
        return false;
    }

    if (vkResetFences(ctx.device, 1u, &ctx.inFlightFences[ctx.currentFrame]) != VK_SUCCESS)
    {
        if (gpuTimingRecording) ctx.gpuFrameTimer.CancelRecording(ctx.currentFrame);
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
        if (gpuTimingRecording) ctx.gpuFrameTimer.CancelRecording(ctx.currentFrame);
        return false;
    }
    if (gpuTimingRecording &&
        !ctx.gpuFrameTimer.MarkSubmitted(ctx.currentFrame, ++ctx.gpuFrameSubmissionSequence))
    {
        ctx.gpuFrameTimer.CancelRecording(ctx.currentFrame);
        RefreshGpuTimingTelemetry(ctx);
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

#if defined(_DEBUG)
const char* CapturePresetName(const horde::gameplay::ShowcaseCheckpointPreset preset)
{
    switch (preset)
    {
    case horde::gameplay::ShowcaseCheckpointPreset::Fresh: return "fresh";
    case horde::gameplay::ShowcaseCheckpointPreset::TorchFailureTrigger: return "torch-failure-trigger";
    case horde::gameplay::ShowcaseCheckpointPreset::TorchFailureSettled: return "torch-failure-settled";
    case horde::gameplay::ShowcaseCheckpointPreset::LichActive: return "lich-active";
    case horde::gameplay::ShowcaseCheckpointPreset::FinaleRoofOpen: return "finale-roof-open";
    default: return "unknown";
    }
}

void ApplyCaptureCheckpoint(VulkanSurfaceContext& context,
                            const horde::gameplay::ShowcaseCheckpoint& checkpoint)
{
    context.simulation.ApplyShowcaseCheckpoint(checkpoint.id);
    context.simulation.ClearEvents();
    context.simulation.ResetTiming();
    context.frameDeltaSeconds = 0.0f;
    context.simulationPaused = true;
    context.simulationInput.paused = true;
    context.simulationInput.damageEnabled = false;
    context.simulationInput.hasAuthoritativePlayerPose = false;
    context.simulationInput.torchLightStrength = context.torchLightStrength;
    context.simulation.AdvanceFrame(context.simulationInput,
                                    0.0,
                                    ++context.inputPublicationSequence);
    MirrorSimulationSnapshot(context);
    context.debugEnemyOverride = horde::gameplay::EnemyKind::None;
}

double CaptureMeanMs(const std::vector<double>& samples)
{
    return samples.empty()
        ? 0.0
        : std::accumulate(samples.begin(), samples.end(), 0.0) / static_cast<double>(samples.size());
}

double CaptureMedianMs(const std::vector<double>& samples)
{
    if (samples.empty())
    {
        return 0.0;
    }
    std::vector<double> sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    const std::size_t middle = sorted.size() / 2u;
    return (sorted.size() & 1u) != 0u
        ? sorted[middle]
        : (sorted[middle - 1u] + sorted[middle]) * 0.5;
}

bool WriteCaptureManifest(const std::filesystem::path& outputDirectory,
                          const VulkanSurfaceContext& context,
                          const horde::vulkan::DeviceCapabilities& capabilities,
                          const std::vector<ShowcaseCaptureRecord>& captures,
                          bool complete,
                          const std::string& error)
{
    std::vector<double> allFrameTimes;
    allFrameTimes.reserve(captures.size() * static_cast<std::size_t>(kCaptureSettlingFrames));
    for (const ShowcaseCaptureRecord& capture : captures)
    {
        allFrameTimes.insert(allFrameTimes.end(), capture.frameTimesMs.begin(), capture.frameTimesMs.end());
    }
    std::ostringstream manifest;
    manifest << std::fixed << std::setprecision(6)
             << "{\n"
             << "  \"schemaVersion\": 1,\n"
             << "  \"complete\": " << (complete ? "true" : "false") << ",\n"
             << "  \"source\": \"rt-storage-image\",\n"
             << "  \"sceneOnly\": true,\n"
             << "  \"overlaysIncluded\": false,\n"
             << "  \"settlingFrames\": " << kCaptureSettlingFrames << ",\n"
             << "  \"fixedAnimationTimeSeconds\": 0.000000,\n"
             << "  \"buildId\": \"" << JsonEscape(HORDE_RT_BUILD_ID) << "\",\n"
             << "  \"raygenSha256\": \"" << JsonEscape(HORDE_RT_RAYGEN_SHA256) << "\",\n"
             << "  \"device\": {\n"
             << "    \"gpuName\": \"" << JsonEscape(capabilities.identity.gpuName) << "\",\n"
             << "    \"vendorId\": " << capabilities.identity.vendorId << ",\n"
             << "    \"deviceId\": " << capabilities.identity.deviceId << "\n"
             << "  },\n"
             << "  \"presentation\": {\n"
             << "    \"renderScale\": " << context.renderScale << ",\n"
             << "    \"dispatchWidth\": " << context.rtScene.DispatchExtent().width << ",\n"
             << "    \"dispatchHeight\": " << context.rtScene.DispatchExtent().height << ",\n"
             << "    \"swapchainWidth\": " << context.swapchainExtent.width << ",\n"
             << "    \"swapchainHeight\": " << context.swapchainExtent.height << ",\n"
             << "    \"swapchainFormat\": " << static_cast<std::uint32_t>(context.swapchainFormat) << "\n"
             << "  },\n"
             << "  \"timing\": {\n"
             << "    \"sampleCount\": " << allFrameTimes.size() << ",\n"
             << "    \"overallMedianMs\": " << CaptureMedianMs(allFrameTimes) << ",\n"
             << "    \"overallMeanMs\": " << CaptureMeanMs(allFrameTimes) << ",\n"
             << "    \"gpuRtCommandBuffer\": {\"valid\": "
             << (context.gpuRtTiming.valid ? "true" : "false")
             << ", \"latestMs\": " << context.gpuRtTiming.latestMs
             << ", \"averageMs\": " << context.gpuRtTiming.averageMs
             << ", \"sampleCount\": " << context.gpuRtTiming.sampleCount
             << ", \"timestampValidBits\": " << context.gpuRtTiming.timestampValidBits
             << ", \"timestampPeriodNanoseconds\": "
             << context.gpuRtTiming.timestampPeriodNanoseconds << "}\n"
             << "  },\n"
             << "  \"staticRtAsset\": {\"enabled\": "
             << (context.rtScene.GenericStaticAssetEnabled() ? "true" : "false")
             << ", \"vertexBytes\": " << context.rtScene.StaticMeshMeasurements().vertexBytes
             << ", \"indexBytes\": " << context.rtScene.StaticMeshMeasurements().indexBytes
             << ", \"materialBytes\": " << context.rtScene.StaticMeshMeasurements().materialBytes
             << ", \"instanceMetadataBytes\": " << context.rtScene.StaticMeshMeasurements().instanceMetadataBytes
             << ", \"primitiveMetadataBytes\": " << context.rtScene.StaticMeshMeasurements().primitiveMetadataBytes
             << ", \"textureBytes\": " << context.rtScene.StaticTextureBytes()
             << ", \"descriptorCount\": " << context.rtScene.StaticMeshMeasurements().descriptorCount
             << ", \"blasBytes\": " << context.rtScene.StaticMeshBlasBytes()
             << ", \"blasBuildMilliseconds\": " << context.rtScene.StaticMeshBlasBuildMilliseconds()
             << ", \"swordBlasBytes\": " << context.rtScene.StaticMeshSwordBlasBytes()
             << ", \"swordBlasBuildMilliseconds\": "
             << context.rtScene.StaticMeshSwordBlasBuildMilliseconds()
             << ", \"torchBlasBytes\": " << context.rtScene.StaticMeshTorchBlasBytes()
             << ", \"torchBlasBuildMilliseconds\": "
             << context.rtScene.StaticMeshTorchBlasBuildMilliseconds()
             << ", \"productionPropBlasBytes\": "
             << context.rtScene.ProductionPropBlasBytes()
             << ", \"productionPropBlasBuildMilliseconds\": "
             << context.rtScene.ProductionPropBlasBuildMilliseconds()
             << "},\n"
             << "  \"dielectricDiagnostics\": {\"transportOverflowCount\": "
             << context.rtScene.DielectricTransportOverflowCount()
             << ", \"shadowOverflowCount\": "
             << context.rtScene.DielectricShadowOverflowCount()
             << ", \"secondaryDielectricRejectCount\": "
             << context.rtScene.DielectricSecondaryRejectCount()
             << ", \"unclosedVolumeCount\": "
             << context.rtScene.DielectricUnclosedVolumeCount()
             << ", \"primaryUnclosedVolumeCount\": "
             << context.rtScene.DielectricPrimaryUnclosedVolumeCount()
             << ", \"shadowUnclosedVolumeCount\": "
             << context.rtScene.DielectricShadowUnclosedVolumeCount()
             << ", \"productionPaneStackFailureCount\": "
             << context.rtScene.ProductionPaneStackFailureCount()
             << ", \"productionPaneSecondaryOriginCount\": "
             << context.rtScene.ProductionPaneSecondaryOriginCount()
             << ", \"productionPaneSecondaryTerminalCount\": "
             << context.rtScene.ProductionPaneSecondaryTerminalCount()
             << ", \"productionPaneSecondarySameMediumCount\": "
             << context.rtScene.ProductionPaneSecondarySameMediumCount()
             << ", \"productionPaneSecondaryDifferentMediumCount\": "
             << context.rtScene.ProductionPaneSecondaryDifferentMediumCount() << "},\n"
             << "  \"dielectricReasonDiagnostics\": {\"secondaryNearSelfHitCount\": "
             << context.rtScene.SecondaryNearSelfHitCount()
             << ", \"primaryOpenMissCount\": " << context.rtScene.PrimaryOpenMissCount()
             << ", \"primaryOpenOpaqueCount\": " << context.rtScene.PrimaryOpenOpaqueCount()
             << ", \"primaryMismatchedExitCount\": " << context.rtScene.PrimaryMismatchedExitCount()
             << ", \"primaryInterfaceBudgetCount\": " << context.rtScene.PrimaryInterfaceBudgetCount()
             << ", \"primaryVolumeBudgetCount\": " << context.rtScene.PrimaryVolumeBudgetCount()
             << ", \"shadowOpenMissCount\": " << context.rtScene.ShadowOpenMissCount()
             << ", \"shadowMismatchedExitCount\": " << context.rtScene.ShadowMismatchedExitCount()
             << ", \"primaryTirCount\": " << context.rtScene.PrimaryTirCount()
             << ", \"primaryInterfaceBudgetOpenVolumeCount\": "
             << context.rtScene.PrimaryInterfaceBudgetOpenVolumeCount()
             << ", \"primaryInterfaceBudgetClosedVolumeCount\": "
             << context.rtScene.PrimaryInterfaceBudgetClosedVolumeCount()
             << ", \"shadowMismatchEmptyCount\": " << context.rtScene.ShadowMismatchEmptyCount()
             << ", \"shadowImplicitOriginExitCount\": "
             << context.rtScene.ShadowImplicitOriginExitCount()
             << ", \"secondaryDielectricTerminalCount\": "
             << context.rtScene.SecondaryDielectricTerminalCount()
             << ", \"primaryTirTerminationCount\": "
             << context.rtScene.PrimaryTirTerminationCount()
             << ", \"shadowFiniteEndpointVolumeCount\": "
             << context.rtScene.ShadowFiniteEndpointVolumeCount()
             << ", \"primaryOpenOpaqueSameInstanceDifferentMaterialCount\": "
             << context.rtScene.PrimaryOpenOpaqueSameInstanceDifferentMaterialCount()
             << ", \"primaryOpenOpaqueAfterTirCount\": "
             << context.rtScene.PrimaryOpenOpaqueAfterTirCount()
             << ", \"primaryOpenOpaqueTerminalInstanceMask\": "
             << context.rtScene.PrimaryOpenOpaqueTerminalInstanceMask()
             << ", \"primaryOpenOpaqueVolumeInstanceMask\": "
             << context.rtScene.PrimaryOpenOpaqueVolumeInstanceMask()
             << ", \"primaryOpenOpaqueTerminalMaterialMask\": "
             << context.rtScene.PrimaryOpenOpaqueTerminalMaterialMask()
             << ", \"primaryClosedVolumeAbsorptionCount\": "
             << context.rtScene.PrimaryClosedVolumeAbsorptionCount()
             << ", \"primaryCertifiedClosedVolumeRecoveryCount\": "
             << context.rtScene.PrimaryCertifiedClosedVolumeRecoveryCount()
             << ", \"shadowCertifiedClosedVolumeRecoveryCount\": "
             << context.rtScene.ShadowCertifiedClosedVolumeRecoveryCount()
             << ", \"certifiedClosedVolumeRecoveryReasonMask\": "
             << context.rtScene.CertifiedClosedVolumeRecoveryReasonMask()
             << ", \"primaryTorchPixelCount\": "
             << context.rtScene.PrimaryTorchPixelCount()
             << ", \"primarySwordPixelCount\": "
             << context.rtScene.PrimarySwordPixelCount()
             << ", \"primaryPlayerPixelCount\": "
             << context.rtScene.PrimaryPlayerPixelCount()
             << ", \"primaryRewardRingPixelCount\": "
             << context.rtScene.PrimaryRewardRingPixelCount()
             << ", \"primaryRewardBodyPixelCount\": "
             << context.rtScene.PrimaryRewardBodyPixelCount()
             << "},\n"
             << "  \"error\": " << (error.empty() ? "null" : "\"" + JsonEscape(error) + "\"") << ",\n"
             << "  \"captures\": [\n";
    for (std::size_t index = 0; index < captures.size(); ++index)
    {
        const ShowcaseCaptureRecord& capture = captures[index];
        const auto& checkpoint = *capture.checkpoint;
        manifest << "    {\n"
                 << "      \"id\": " << checkpoint.id << ",\n"
                 << "      \"checkpoint\": \"" << JsonEscape(checkpoint.name) << "\",\n"
                 << "      \"preset\": \"" << CapturePresetName(checkpoint.preset) << "\",\n"
                 << "      \"zone\": \"" << horde::gameplay::ShowcaseZoneName(checkpoint.expectedZone) << "\",\n"
                 << "      \"camera\": {\"x\": " << checkpoint.x << ", \"z\": " << checkpoint.z
                 << ", \"yaw\": " << checkpoint.yaw << ", \"pitch\": " << checkpoint.pitch << "},\n"
                 << "      \"state\": {\"torchFailurePhase\": \"" << capture.torchFailurePhase
                 << "\", \"selectedEnemy\": \"" << capture.selectedEnemy
                 << "\", \"lichPhase\": \"" << capture.lichPhase
                 << "\", \"finaleSkylightOpenProgress\": " << capture.finaleSkylightOpenProgress << "},\n"
                 << "      \"width\": " << capture.width << ",\n"
                 << "      \"height\": " << capture.height << ",\n"
                 << "      \"honestlyPresentedRtFrame\": true,\n"
                 << "      \"visibility\": {\"playerPrimaryVisible\": "
                 << (capture.playerPrimaryVisible ? "true" : "false")
                 << ", \"instanceMasks\": [";
        for (std::size_t mask = 0u; mask < capture.instanceMasks.size(); ++mask)
            manifest << (mask == 0u ? "" : ", ")
                     << static_cast<std::uint32_t>(capture.instanceMasks[mask]);
        manifest << "], \"primaryPixels\": {\"torch\": "
                 << capture.primaryTorchPixels << ", \"sword\": "
                 << capture.primarySwordPixels << ", \"player\": "
                 << capture.primaryPlayerPixels << ", \"rewardRing\": "
                 << capture.primaryRewardRingPixels << ", \"rewardBody\": "
                 << capture.primaryRewardBodyPixels << "}, \"rewardGrip\": {\"positionErrorMetres\": "
                 << capture.rewardGripPositionErrorMetres
                 << ", \"orientationErrorRadians\": "
                 << capture.rewardGripOrientationErrorRadians << "}},\n"
                 << "      \"timing\": {\"sampleCount\": " << capture.frameTimesMs.size()
                 << ", \"medianMs\": " << CaptureMedianMs(capture.frameTimesMs)
                 << ", \"meanMs\": " << CaptureMeanMs(capture.frameTimesMs) << "},\n"
                 << "      \"outputRedBlueSwapAppliedAndNormalised\": "
                 << (capture.redBlueSwapNormalised ? "true" : "false") << ",\n"
                 << "      \"pixelFormat\": \"RGBA8\",\n"
                 << "      \"file\": \"" << JsonEscape(capture.filename) << "\",\n"
                 << "      \"pngSha256\": \"" << capture.pngSha256 << "\"\n"
                 << "    }" << (index + 1u == captures.size() ? "\n" : ",\n");
    }
    manifest << "  ]\n}\n";
    return WriteReportFile(outputDirectory / "capture-manifest.json", manifest.str());
}

int RunShowcaseCapture(VulkanSurfaceContext& context,
                       horde::vulkan::DeviceCapabilities& capabilities,
                       const std::filesystem::path& outputDirectory)
{
    std::error_code directoryError;
    std::filesystem::create_directories(outputDirectory, directoryError);
    if (directoryError)
    {
        std::cerr << "Failed to create showcase capture directory: " << directoryError.message() << '\n';
        return 1;
    }

    std::vector<ShowcaseCaptureRecord> captures;
    std::vector<const horde::gameplay::ShowcaseCheckpoint*> checkpoints;
    horde::gameplay::ShowcaseCheckpoint developmentShowcase{};
    if (!context.developmentCheckpoint.empty())
    {
        const auto* development =
            horde::gameplay::FindDevelopmentCheckpoint(context.developmentCheckpoint);
        const auto* base = development == nullptr
            ? nullptr
            : horde::gameplay::FindShowcaseCheckpoint(development->baseShowcaseCheckpointId);
        if (development == nullptr || base == nullptr)
        {
            std::cerr << "Development checkpoint contract could not resolve its base checkpoint.\n";
            return 1;
        }
        developmentShowcase = {
            development->id,
            development->name.data(),
            development->cameraX,
            development->cameraZ,
            development->yaw,
            development->pitch,
            base->expectedZone,
            base->preset};
        checkpoints.push_back(&developmentShowcase);
    }
    else
    {
        for (const auto& checkpoint : horde::gameplay::kShowcaseCheckpoints)
            checkpoints.push_back(&checkpoint);
    }
    captures.reserve(checkpoints.size());
    auto fail = [&](const std::string& diagnostic) {
        WriteCaptureManifest(outputDirectory, context, capabilities, captures, false, diagnostic);
        std::cerr << "Showcase capture failed: " << diagnostic << '\n';
        return 1;
    };
    if (!context.useRtPath || !context.rtScene.IsReady())
    {
        return fail("A ready RayTracingPipeline scene is required; no fallback capture is allowed.");
    }

    const VkClearColorValue clearColor = ClearColorForMode(capabilities.rtMode);
    for (const horde::gameplay::ShowcaseCheckpoint* checkpointPointer : checkpoints)
    {
        const horde::gameplay::ShowcaseCheckpoint& checkpoint = *checkpointPointer;
        if (checkpoint.id >= 100)
        {
            const auto* development =
                horde::gameplay::FindDevelopmentCheckpoint(context.developmentCheckpoint);
            if (development == nullptr ||
                !horde::gameplay::StageDevelopmentCheckpointSimulation(
                    context.simulation, *development))
            {
                return fail(std::string("Development checkpoint '") + checkpoint.name +
                            "' could not stage its authoritative combat pose.");
            }
            context.simulation.ResetTiming();
            context.simulation.ClearEvents();
            context.frameDeltaSeconds = 0.0f;
            context.simulationPaused = true;
            context.simulationInput.paused = true;
            context.simulationInput.damageEnabled = false;
            context.simulationInput.hasAuthoritativePlayerPose = false;
            MirrorSimulationSnapshot(context);
        }
        else
        {
            ApplyCaptureCheckpoint(context, checkpoint);
        }
        std::vector<double> frameTimesMs;
        frameTimesMs.reserve(kCaptureSettlingFrames);
        for (int frame = 0; frame < kCaptureSettlingFrames; ++frame)
        {
            bool rtFramePresented = false;
            const auto frameStart = std::chrono::steady_clock::now();
            if (!RenderFrame(context, clearColor, rtFramePresented) || !rtFramePresented)
            {
                return fail(std::string("Checkpoint '") + checkpoint.name +
                            "' did not reach a successful RT swapchain presentation" +
                            (context.lastRtFrameError.empty()
                                ? "."
                                : ": " + context.lastRtFrameError));
            }
            const auto frameEnd = std::chrono::steady_clock::now();
            frameTimesMs.push_back(std::chrono::duration<double, std::milli>(frameEnd - frameStart).count());
            capabilities.performance.gpuRt = context.gpuRtTiming;
            capabilities.rtScene.presented = true;
        }

        horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
        std::string diagnostic;
        if (!context.rtScene.CaptureStorageImage(image, diagnostic))
        {
            return fail(std::string("Checkpoint '") + checkpoint.name + "' readback failed: " + diagnostic);
        }

        std::ostringstream filename;
        filename << std::setw(2) << std::setfill('0') << checkpoint.id << '-' << checkpoint.name << ".png";
        const std::filesystem::path pngPath = outputDirectory / filename.str();
        if (!WriteRgbaPng(pngPath, image, diagnostic))
        {
            return fail(std::string("Checkpoint '") + checkpoint.name + "' PNG write failed: " + diagnostic);
        }

        ShowcaseCaptureRecord record;
        record.checkpoint = &checkpoint;
        const horde::gameplay::simulation::SimulationSnapshot& simulation = context.simulation.Snapshot();
        record.torchFailurePhase = horde::gameplay::TorchFailurePhaseName(simulation.torchFailure.phase);
        record.selectedEnemy = horde::gameplay::EnemyKindName(simulation.enemyRoster.selectedEnemy);
        record.lichPhase = horde::gameplay::LichPhaseName(simulation.lich.phase);
        record.finaleSkylightOpenProgress = simulation.lich.finaleSkylightOpenProgress;
        record.filename = filename.str();
        record.width = image.width;
        record.height = image.height;
        record.redBlueSwapNormalised = image.redBlueSwapNormalised;
        record.instanceMasks = context.rtScene.LastInstanceMasks();
        record.playerPrimaryVisible = context.rtScene.LastPlayerPrimaryVisible();
        record.primaryTorchPixels = context.rtScene.PrimaryTorchPixelCount();
        record.primarySwordPixels = context.rtScene.PrimarySwordPixelCount();
        record.primaryPlayerPixels = context.rtScene.PrimaryPlayerPixelCount();
        record.primaryRewardRingPixels = context.rtScene.PrimaryRewardRingPixelCount();
        record.primaryRewardBodyPixels = context.rtScene.PrimaryRewardBodyPixelCount();
        record.rewardGripPositionErrorMetres =
            context.rtScene.RewardLanternGripAgreement().positionErrorMetres;
        record.rewardGripOrientationErrorRadians =
            context.rtScene.RewardLanternGripAgreement().orientationErrorRadians;
        record.frameTimesMs = std::move(frameTimesMs);
        const auto* development = horde::gameplay::FindDevelopmentCheckpoint(
            context.developmentCheckpoint);
        const bool claimedRewardCapture = development != nullptr &&
            development->id >= 116 && development->id <= 119;
        if (context.developmentCheckpoint.empty() &&
            (record.instanceMasks[1] != 0x02u ||
             record.instanceMasks[3] != 0x02u))
        {
            return fail(std::string("Checkpoint '") + checkpoint.name +
                        "' masked the ordinary torch/sword production instances.");
        }
        if (checkpoint.name == "opening" && record.primaryTorchPixels == 0u)
        {
            return fail("Opening capture has no primary-visible torch pixels (floating-flame regression).");
        }
        if (claimedRewardCapture &&
            (record.instanceMasks[4] != 0x14u ||
             !record.playerPrimaryVisible ||
             record.instanceMasks[7] != 0x01u ||
             record.instanceMasks[8] != 0x01u ||
             record.primaryPlayerPixels == 0u ||
             record.primaryRewardRingPixels == 0u ||
             record.primaryRewardBodyPixels == 0u ||
             record.rewardGripPositionErrorMetres >
                 horde::vulkan::raytracing::kPlayerGripSocketToleranceMetres ||
             record.rewardGripOrientationErrorRadians >
                 horde::vulkan::raytracing::kPlayerGripOrientationToleranceRadians))
        {
            return fail(std::string("Checkpoint '") + checkpoint.name +
                        "' failed claimed reward player/lantern primary visibility or final GripRing contact.");
        }
        if (!Sha256File(pngPath, record.pngSha256, diagnostic))
        {
            return fail(std::string("Checkpoint '") + checkpoint.name + "' hash failed: " + diagnostic);
        }
        captures.push_back(std::move(record));
        std::cout << "Captured " << checkpoint.name << " -> " << pngPath << '\n';
    }

    if (!WriteCaptureManifest(outputDirectory, context, capabilities, captures, true, {}))
    {
        std::cerr << "Failed to write capture manifest.\n";
        return 1;
    }
    std::cout << "Captured all " << captures.size() << " showcase checkpoints to " << outputDirectory << '\n';
    return 0;
}
#endif

int RunDiagnosticSwapchainWindow(HWND hWnd,
                                 horde::vulkan::DeviceCapabilities& capabilities,
                                 const std::filesystem::path& textReportPath,
                                 const std::filesystem::path& jsonReportPath,
                                 const std::filesystem::path* captureDirectory,
                                 const std::string* developmentCheckpoint)
{
    VulkanSurfaceContext context;
    context.windowHandle = hWnd;
    if (developmentCheckpoint != nullptr) context.developmentCheckpoint = *developmentCheckpoint;
    LoadSettings(context);
#if defined(_DEBUG)
    const RtLabDebugLaunchOptions rtLabDebug = ParseRtLabDebugLaunchOptions();
    if (rtLabDebug.requested)
    {
        context.rtLabDebugInjection = true;
        context.rtLabRouteTainted = true;
        context.rtSceneTuning = rtLabDebug.tuning;
        context.rtLabLightGroup = rtLabDebug.lightGroup;
    }
#endif
    if (captureDirectory != nullptr)
    {
        context.renderScale = 1.0f;
        context.outputExposure = 0.62f;
        context.waterQuality = horde::vulkan::raytracing::WaterQuality::High;
        context.sfxEnabled = false;
        context.simulationPaused = true;
        context.pauseMenuVisible = false;
    }
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
        if (captureDirectory == nullptr)
        {
            ApplyOverlayState(context);
#if defined(_DEBUG)
            if (context.rtLabDebugInjection) OpenRtLab(context);
            else
#endif
                SetFocus(GetDlgItem(hWnd, kResumeButtonId));
        }
    }

#if defined(_DEBUG)
    if (captureDirectory != nullptr)
    {
        const int captureResult = RunShowcaseCapture(context, capabilities, *captureDirectory);
        if (IsWindow(hWnd))
        {
            SetWindowLongPtrA(hWnd, GWLP_USERDATA, 0);
        }
        DestroyRenderContext(context);
        return captureResult;
    }
#else
    (void)captureDirectory;
#endif

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
            context.gpuFrameTimer.ResetAfterDeviceIdle();
            context.gpuFrameTimingTotalMs = 0.0;
            context.gpuFrameTimingSampleCount = 0u;
            RefreshGpuTimingTelemetry(context);
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
                SetWindowTextA(hud, kHudApplyingScaleText);
            }
            if (!InitialiseRtSceneForSwapchain(context))
            {
                renderFailed = true;
                break;
            }
        }

        const bool benchmarkFrame = context.benchmark.IsRunning();
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
        capabilities.performance.gpuRt = context.gpuRtTiming;
        UpdateRtLabTelemetry(context);
        const auto frameEnd = std::chrono::steady_clock::now();
        const double frameTimeMs = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();
        if (benchmarkFrame)
        {
            timingSamples.clear();
            context.benchmark.RecordFrame(frameTimeMs, rtFramePresented);
            if (!context.benchmark.IsRunning())
            {
                CompleteBenchmark(context, capabilities, textReportPath.parent_path());
            }
        }
        else
        {
            timingSamples.push_back(frameTimeMs);
        }
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
                SetWindowTextA(hud, kHudActiveText);
            }
            if (HWND edit = GetDlgItem(hWnd, kEditControlId))
            {
                const std::string updatedText = WindowSafeText(BuildDisplayText(capabilities));
                SetWindowTextA(edit, updatedText.c_str());
            }
        }
#if defined(_DEBUG)
        RefreshDeveloperOverlay(context, capabilities);
#endif
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

#if defined(_DEBUG)
    HFONT developerFont = CreateFontA(
        ScaleForDpi(window, 11), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        FF_MODERN, "Consolas");
    if (!developerFont)
    {
        developerFont = static_cast<HFONT>(GetStockObject(ANSI_FIXED_FONT));
    }
#endif

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
#if defined(_DEBUG)
    if (HWND developerOverlay = GetDlgItem(window, kDeveloperOverlayId))
    {
        SendMessageA(developerOverlay, WM_SETFONT, reinterpret_cast<WPARAM>(developerFont), TRUE);
    }
#endif
    for (const int id : {kHudControlId, kVitalityHudControlId, kPauseTitleId, kEndingBodyId, kResumeButtonId, kRestartButtonId,
                         kControlsButtonId, kSettingsButtonId, kDiagnosticsButtonId, kRunBenchmarkButtonId,
                         kMoreBySamfa12ButtonId, kExitButtonId, kBenchmarkTitleId,
                         kBenchmarkCopyButtonId, kBenchmarkSaveButtonId, kBenchmarkBackButtonId,
                         kSettingsTitleId, kSfxButtonId, kSensitivityButtonId, kWaterQualityButtonId, kRenderScaleLabelId,
                         kRenderScaleSliderId, kFullscreenButtonId, kSettingsBackButtonId,
                         kRtLabTitleId, kRtLabTelemetryId, kRtLabWaterfallLabelId, kRtLabWaterfallSliderId,
                         kRtLabRoofLabelId, kRtLabRoofSliderId, kRtLabDawnLabelId, kRtLabDawnSliderId,
                         kRtLabFogLabelId, kRtLabFogSliderId, kRtLabLightGroupButtonId,
                         kRtLabFireStrengthLabelId, kRtLabFireStrengthSliderId,
                         kRtLabFireTurbulenceLabelId, kRtLabFireTurbulenceSliderId,
                         kRtLabFireSmokeLabelId, kRtLabFireSmokeSliderId,
                         kRtLabGlassVisibilityLabelId, kRtLabGlassVisibilitySliderId,
                         kRtLabGlassTransmissionLabelId, kRtLabGlassTransmissionSliderId,
                         kRtLabGlassIorLabelId, kRtLabGlassIorSliderId,
                         kRtLabGlassRoughnessLabelId, kRtLabGlassRoughnessSliderId,
                         kRtLabHueLabelId, kRtLabHueSliderId, kRtLabIntensityLabelId,
                         kRtLabIntensitySliderId, kRtLabWorkloadButtonId,
                         kRtLabRestoreButtonId, kRtLabBackButtonId})
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
#if defined(_DEBUG)
    if (developerFont != GetStockObject(ANSI_FIXED_FONT))
    {
        ReplaceFontProperty(window, kDeveloperFontProperty, developerFont);
    }
#endif
}

void ReleaseDpiScaledFonts(HWND window)
{
    for (const char* propertyName : {kUiFontProperty, kMonoFontProperty, kDeveloperFontProperty})
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
    auto* sceneContext = reinterpret_cast<VulkanSurfaceContext*>(GetWindowLongPtrA(window, GWLP_USERDATA));
    if (sceneContext && sceneContext->benchmarkReportVisible)
    {
        const int reportTitleHeight = ScaleForDpi(window, 42);
        const int reportButtonHeight = ScaleForDpi(window, 40);
        const int reportGap = ScaleForDpi(window, 8);
        const int reportButtonY = height - inset - reportButtonHeight;
        if (HWND title = GetDlgItem(window, kBenchmarkTitleId))
        {
            MoveWindow(title, inset, inset, width - inset * 2, reportTitleHeight, TRUE);
        }
        if (HWND edit = GetDlgItem(window, kEditControlId))
        {
            const int editY = inset + reportTitleHeight + reportGap;
            MoveWindow(edit, inset, editY, width - inset * 2,
                       std::max(ScaleForDpi(window, 100), reportButtonY - reportGap - editY), TRUE);
        }
        const int availableWidth = width - inset * 2 - reportGap * 2;
        const int reportButtonWidth = availableWidth / 3;
        int reportX = inset;
        for (const int id : {kBenchmarkCopyButtonId, kBenchmarkSaveButtonId, kBenchmarkBackButtonId})
        {
            if (HWND control = GetDlgItem(window, id))
            {
                MoveWindow(control, reportX, reportButtonY, reportButtonWidth, reportButtonHeight, TRUE);
            }
            reportX += reportButtonWidth + reportGap;
        }
    }
    if (HWND hud = GetDlgItem(window, kHudControlId))
    {
        const int hudInset = ScaleForDpi(window, 14);
        MoveWindow(hud, hudInset, hudInset,
                   std::min(ScaleForDpi(window, 650), std::max(ScaleForDpi(window, 260), width - hudInset * 2)),
                   ScaleForDpi(window, 30), TRUE);
    }
    if (HWND vitality = GetDlgItem(window, kVitalityHudControlId))
    {
        const int vitalityWidth = ScaleForDpi(window, 190);
        MoveWindow(vitality, (width - vitalityWidth) / 2, ScaleForDpi(window, 52),
                   vitalityWidth, ScaleForDpi(window, 28), TRUE);
    }
#if defined(_DEBUG)
    if (HWND developerOverlay = GetDlgItem(window, kDeveloperOverlayId))
    {
        const int overlayInset = ScaleForDpi(window, 14);
        const int overlayWidth = std::min(ScaleForDpi(window, 520),
                                          std::max(ScaleForDpi(window, 300), width - overlayInset * 2));
        MoveWindow(developerOverlay,
                   std::max(overlayInset, width - overlayWidth - overlayInset),
                   ScaleForDpi(window, 86),
                   overlayWidth,
                   ScaleForDpi(window, 116), TRUE);
    }
#endif

    const int buttonWidth = std::min(ScaleForDpi(window, 420),
                                     std::max(ScaleForDpi(window, 220), width - ScaleForDpi(window, 64)));
    const int buttonHeight = ScaleForDpi(window, 38);
    const int gap = ScaleForDpi(window, 8);
    const int titleHeight = ScaleForDpi(window, 48);
    const int titleAdvance = ScaleForDpi(window, 54);
    const auto* layoutContext = reinterpret_cast<const VulkanSurfaceContext*>(GetWindowLongPtrA(window, GWLP_USERDATA));
    const bool endingLayout = layoutContext != nullptr && layoutContext->endingOverlayVisible;
    const bool compactOverlayLayout = layoutContext != nullptr &&
                                      (layoutContext->deathOverlayVisible || endingLayout);
    const int endingBodyHeight = endingLayout ? ScaleForDpi(window, 132) : 0;
    const int compactButtonCount = endingLayout ? 4 : 3;
    const int pauseTotal = compactOverlayLayout
        ? titleHeight + endingBodyHeight + (endingLayout ? gap : 0) + compactButtonCount * buttonHeight + compactButtonCount * gap
        : titleHeight + 9 * buttonHeight + 8 * gap;
    const int pauseX = (width - buttonWidth) / 2;
    int y = std::max(ScaleForDpi(window, 54), (height - pauseTotal) / 2);
    if (HWND title = GetDlgItem(window, kPauseTitleId)) MoveWindow(title, pauseX, y, buttonWidth, titleHeight, TRUE);
    y += titleAdvance;
    if (endingLayout)
    {
        if (HWND body = GetDlgItem(window, kEndingBodyId))
        {
            MoveWindow(body, pauseX, y, buttonWidth, endingBodyHeight, TRUE);
        }
        y += endingBodyHeight + gap;
    }
    if (compactOverlayLayout)
    {
        for (const int id : {kRtLabButtonId, kResumeButtonId, kRestartButtonId, kExitButtonId})
        {
            if (!endingLayout && id == kRtLabButtonId) continue;
            if (HWND control = GetDlgItem(window, id)) MoveWindow(control, pauseX, y, buttonWidth, buttonHeight, TRUE);
            y += buttonHeight + gap;
        }
    }
    else
    {
        for (const int id : {kResumeButtonId, kRestartButtonId, kControlsButtonId, kSettingsButtonId, kRtLabButtonId,
                             kDiagnosticsButtonId, kRunBenchmarkButtonId, kMoreBySamfa12ButtonId, kExitButtonId})
        {
            if (HWND control = GetDlgItem(window, id)) MoveWindow(control, pauseX, y, buttonWidth, buttonHeight, TRUE);
            y += buttonHeight + gap;
        }
    }

    const int labelHeight = ScaleForDpi(window, 26);
    const int sliderHeight = ScaleForDpi(window, 38);
    const int settingsTotal = titleHeight + 5 * buttonHeight + labelHeight + sliderHeight + 6 * gap;
    y = std::max(ScaleForDpi(window, 54), (height - settingsTotal) / 2);
    if (HWND title = GetDlgItem(window, kSettingsTitleId)) MoveWindow(title, pauseX, y, buttonWidth, titleHeight, TRUE);
    y += titleAdvance;
    for (const int id : {kSfxButtonId, kSensitivityButtonId, kWaterQualityButtonId})
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

    if (layoutContext != nullptr && layoutContext->rtLabVisible)
    {
        auto* mutableContext = const_cast<VulkanSurfaceContext*>(layoutContext);
        const int panelWidth = std::min(ScaleForDpi(window, 680), std::max(ScaleForDpi(window, 300), width - inset * 2));
        const int panelHeight = std::min(ScaleForDpi(window, 650), std::max(ScaleForDpi(window, 260), height - inset * 2));
        const int panelX = (width - panelWidth) / 2;
        const int panelY = (height - panelHeight) / 2;
        if (HWND panel = GetDlgItem(window, kRtLabPanelId))
        {
            MoveWindow(panel, panelX, panelY, panelWidth, panelHeight, TRUE);
            SetWindowPos(panel, HWND_BOTTOM, panelX, panelY, panelWidth, panelHeight, SWP_NOACTIVATE);
        }
        const int contentHeight = ScaleForDpi(window, 1344);
        const int maxScroll = std::max(0, contentHeight - panelHeight + ScaleForDpi(window, 28));
        mutableContext->rtLabScrollOffset = std::clamp(mutableContext->rtLabScrollOffset, 0, maxScroll);
        SCROLLINFO scroll{sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE | SIF_POS};
        scroll.nMin = 0;
        scroll.nMax = contentHeight;
        scroll.nPage = static_cast<UINT>(panelHeight);
        scroll.nPos = mutableContext->rtLabScrollOffset;
        SetScrollInfo(window, SB_VERT, &scroll, TRUE);
        ShowScrollBar(window, SB_VERT, maxScroll > 0);

        const int contentX = panelX + ScaleForDpi(window, 28);
        const int contentWidth = panelWidth - ScaleForDpi(window, 56);
        const int contentTop = panelY + ScaleForDpi(window, 18) - mutableContext->rtLabScrollOffset;
        const int smallGap = ScaleForDpi(window, 4);
        const auto place = [&](const int id, const int offset, const int controlHeight)
        {
            if (HWND control = GetDlgItem(window, id))
            {
                const int controlY = contentTop + ScaleForDpi(window, offset);
                MoveWindow(control, contentX, controlY, contentWidth, controlHeight, TRUE);
                const bool inside = controlY >= panelY + ScaleForDpi(window, 8) &&
                    controlY + controlHeight <= panelY + panelHeight - ScaleForDpi(window, 8);
                ShowWindow(control, inside ? SW_SHOW : SW_HIDE);
                if (inside)
                {
                    // Trackbars that were hidden while scrolling must repaint
                    // immediately when they re-enter the clipped lab panel.
                    RedrawWindow(control, nullptr, nullptr,
                                 RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_FRAME);
                }
            }
        };
        place(kRtLabTitleId, 0, titleHeight);
        place(kRtLabTelemetryId, 50, labelHeight);
        int labY = 84;
        for (const auto [labelId, sliderId] : std::array<std::pair<int, int>, 11>{{
                 {kRtLabWaterfallLabelId, kRtLabWaterfallSliderId},
                 {kRtLabRoofLabelId, kRtLabRoofSliderId},
                 {kRtLabDawnLabelId, kRtLabDawnSliderId},
                 {kRtLabFogLabelId, kRtLabFogSliderId},
                 {kRtLabFireStrengthLabelId, kRtLabFireStrengthSliderId},
                 {kRtLabFireTurbulenceLabelId, kRtLabFireTurbulenceSliderId},
                 {kRtLabFireSmokeLabelId, kRtLabFireSmokeSliderId},
                 {kRtLabGlassVisibilityLabelId, kRtLabGlassVisibilitySliderId},
                 {kRtLabGlassTransmissionLabelId, kRtLabGlassTransmissionSliderId},
                 {kRtLabGlassIorLabelId, kRtLabGlassIorSliderId},
                 {kRtLabGlassRoughnessLabelId, kRtLabGlassRoughnessSliderId}}})
        {
            place(labelId, labY, labelHeight);
            labY += 26;
            place(sliderId, labY, sliderHeight);
            labY += 46;
        }
        place(kRtLabLightGroupButtonId, labY, buttonHeight);
        labY += 48;
        place(kRtLabHueLabelId, labY, labelHeight);
        labY += 26;
        place(kRtLabHueSliderId, labY, sliderHeight);
        labY += 46;
        place(kRtLabIntensityLabelId, labY, labelHeight);
        labY += 26;
        place(kRtLabIntensitySliderId, labY, sliderHeight);
        labY += 48;
        place(kRtLabWorkloadButtonId, labY, buttonHeight);
        labY += 46;
        place(kRtLabRestoreButtonId, labY, buttonHeight);
        labY += 46;
        place(kRtLabBackButtonId, labY, buttonHeight);
        (void)smallGap;
    }
    else
    {
        ShowScrollBar(window, SB_VERT, FALSE);
    }
}

void ShowControlsHelp(HWND window)
{
    MessageBoxA(window,
                "WASD  Move and strafe\n"
                "Left mouse drag  360 camera look\n"
                "Right mouse or Space  Swing sword\n"
                "Q  Parry skeleton strike\n"
                "E  Interact    F  Raise / lower claimed lantern\n"
                "Controller left stick  Move and strafe\n"
                "Controller right stick  Camera look\n"
                "RT  Attack    LT  Parry    B / Circle  Dodge\n"
                "A  Interact    Y  Raise / lower claimed lantern\n"
                "D-pad  Navigate menus    A  Select    B / Circle  Back\n"
                "Menu / Start  Pause / resume\n"
                "Esc  Pause / resume\n"
                 "R  Restart route\n"
                 "F1  Controls\n"
                 "F2  RT diagnostics\n"
#if defined(_DEBUG)
                 "F3  Live developer overlay\n"
#endif
                 "Alt+Enter  Fullscreen",
                "Horde Lantern RT - controls",
                MB_OK | MB_ICONINFORMATION);
}

void ShowCredits(HWND window)
{
    MessageBoxA(window,
                "Environment materials: Poly Haven (CC0).\n"
                "Sound effects: FilmCow Royalty Free Sound Effects Library.\n"
                "Water Dripping by DRAGON-STUDIO via Pixabay (Pixabay Content License).\n"
                "Skeleton derivative: original by Hotstrike Studio; texture, rig, and animation processing created with Meshy (CC BY 4.0).\n"
                "Placeholder lich character created and animated with Meshy (CC0).\n"
                "Production Gothic arming sword created with Meshy; runtime processing by Samfa12/Codex (CC BY 4.0).\n"
                "Production medieval hand torch created with Meshy; runtime processing by Samfa12/Codex (CC BY 4.0).\n"
                "Historical-Gothic traveller/fighter created with Meshy; runtime processing and animation integration by Samfa12/Codex (CC BY 4.0).\n"
                "Production Gothic reward chest created with Meshy; runtime processing by Samfa12/Codex (CC BY 4.0).\n"
                "Production Gothic reward lantern created with Meshy; runtime processing by Samfa12/Codex (CC BY 4.0).\n"
                "Application icon created for this project with OpenAI image generation.\n\n"
                "See ASSET_LICENSES.md beside the demo for source links and full licence details.",
                "Horde Lantern RT - credits and licences",
                MB_OK | MB_ICONINFORMATION);
}

void OpenSamfa12Website(HWND window)
{
    const HINSTANCE result = ShellExecuteA(window, "open", "https://samfa12.com/", nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32)
    {
        MessageBoxA(window,
                    "Samfa12.com could not be opened in your default browser.",
                    "Horde Lantern RT",
                    MB_OK | MB_ICONERROR);
    }
}

bool CopyTextToClipboard(HWND window, const std::string& text)
{
    if (!OpenClipboard(window))
    {
        return false;
    }
    EmptyClipboard();
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1u);
    if (!memory)
    {
        CloseClipboard();
        return false;
    }
    void* destination = GlobalLock(memory);
    std::memcpy(destination, text.c_str(), text.size() + 1u);
    GlobalUnlock(memory);
    if (!SetClipboardData(CF_TEXT, memory))
    {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }
    CloseClipboard();
    return true;
}

void SaveBenchmarkReportAs(HWND window, const std::string& report)
{
    char path[MAX_PATH] = "HordeLanternRT-benchmark.txt";
    OPENFILENAMEA dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = window;
    dialog.lpstrFilter = "Text report (*.txt)\0*.txt\0All files (*.*)\0*.*\0\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrDefExt = "txt";
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (GetSaveFileNameA(&dialog) && !WriteReportFile(path, report))
    {
        MessageBoxA(window, "The benchmark report could not be saved to that location.",
                    "Horde Lantern RT", MB_OK | MB_ICONERROR);
    }
}

void ToggleDiagnostics(VulkanSurfaceContext& context)
{
    context.diagnosticsVisible = !context.diagnosticsVisible;
    context.settingsVisible = false;
    context.benchmarkReportVisible = false;
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
    context.benchmarkReportVisible = false;
    ApplyOverlayState(context);
    PlaySoundEffect(context, "ui_select.wav");
    SetFocus(GetDlgItem(context.windowHandle, kSfxButtonId));
}

LRESULT CALLBACK DiagnosticWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* sceneContext = reinterpret_cast<VulkanSurfaceContext*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
    switch (message)
    {
    case WM_MOUSEWHEEL:
        if (sceneContext && sceneContext->rtLabVisible)
        {
            UINT configuredLines = 3u;
            SystemParametersInfoA(SPI_GETWHEELSCROLLLINES, 0u, &configuredLines, 0u);
            const int wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            const int wheelSteps = std::max(1, std::abs(wheelDelta) / WHEEL_DELTA);
            if (configuredLines == WHEEL_PAGESCROLL)
            {
                for (int step = 0; step < wheelSteps; ++step)
                    SendMessageA(hWnd, WM_VSCROLL, wheelDelta > 0 ? SB_PAGEUP : SB_PAGEDOWN, 0);
            }
            else
            {
                const int lineCount = std::clamp(static_cast<int>(configuredLines), 1, 12);
                for (int line = 0; line < lineCount * wheelSteps; ++line)
                    SendMessageA(hWnd, WM_VSCROLL, wheelDelta > 0 ? SB_LINEUP : SB_LINEDOWN, 0);
            }
            return 0;
        }
        break;
    case WM_VSCROLL:
        if (sceneContext && sceneContext->rtLabVisible)
        {
            SCROLLINFO scroll{sizeof(SCROLLINFO), SIF_ALL};
            GetScrollInfo(hWnd, SB_VERT, &scroll);
            using horde::platform::windows::RtLabScrollAction;
            RtLabScrollAction action = RtLabScrollAction::Thumb;
            bool handled = true;
            switch (LOWORD(wParam))
            {
            case SB_TOP: action = RtLabScrollAction::Top; break;
            case SB_BOTTOM: action = RtLabScrollAction::Bottom; break;
            case SB_LINEUP: action = RtLabScrollAction::LineUp; break;
            case SB_LINEDOWN: action = RtLabScrollAction::LineDown; break;
            case SB_PAGEUP: action = RtLabScrollAction::PageUp; break;
            case SB_PAGEDOWN: action = RtLabScrollAction::PageDown; break;
            case SB_THUMBPOSITION:
            case SB_THUMBTRACK: action = RtLabScrollAction::Thumb; break;
            default: handled = false; break;
            }
            const int maximum = std::max(0, scroll.nMax - static_cast<int>(scroll.nPage) + 1);
            if (handled)
            {
                sceneContext->rtLabScrollOffset = horde::platform::windows::StepRtLabScroll(
                    sceneContext->rtLabScrollOffset,
                    maximum,
                    ScaleForDpi(hWnd, 36),
                    static_cast<int>(scroll.nPage),
                    action,
                    scroll.nTrackPos);
            }
            RECT client{};
            GetClientRect(hWnd, &client);
            LayoutOverlayControls(hWnd, client.right, client.bottom);
            return 0;
        }
        break;
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
        if (sceneContext && sceneContext->rtLabVisible && lParam != 0)
        {
            HWND slider = reinterpret_cast<HWND>(lParam);
            const int id = GetDlgCtrlID(slider);
            const int value = static_cast<int>(SendMessageA(slider, TBM_GETPOS, 0, 0));
            auto& tuning = sceneContext->rtSceneTuning;
            switch (id)
            {
            case kRtLabWaterfallSliderId: tuning.waterfallWidthScale = static_cast<float>(value) / 100.0f; break;
            case kRtLabRoofSliderId: tuning.finaleRoofOpenOverride = static_cast<float>(value) / 100.0f; break;
            case kRtLabDawnSliderId: tuning.finaleDawnRevealOverride = static_cast<float>(value) / 100.0f; break;
            case kRtLabFogSliderId: tuning.fogDensityScale = static_cast<float>(value) / 100.0f; break;
            case kRtLabFireStrengthSliderId: tuning.fireStrengthScale = static_cast<float>(value) / 100.0f; break;
            case kRtLabFireTurbulenceSliderId: tuning.fireTurbulenceScale = static_cast<float>(value) / 100.0f; break;
            case kRtLabFireSmokeSliderId: tuning.fireSmokeScale = static_cast<float>(value) / 100.0f; break;
            case kRtLabGlassVisibilitySliderId: tuning.glassFixtureVisible = value > 0; break;
            case kRtLabGlassTransmissionSliderId: tuning.glassTransmission = static_cast<float>(value) / 100.0f; break;
            case kRtLabGlassIorSliderId: tuning.glassIor = static_cast<float>(value) / 100.0f; break;
            case kRtLabGlassRoughnessSliderId: tuning.glassRoughness = static_cast<float>(value) / 100.0f; break;
            case kRtLabHueSliderId:
                tuning.lights[static_cast<std::size_t>(sceneContext->rtLabLightGroup)].hueDegrees =
                    static_cast<float>(value);
                break;
            case kRtLabIntensitySliderId:
                tuning.lights[static_cast<std::size_t>(sceneContext->rtLabLightGroup)].intensityScale =
                    static_cast<float>(value) / 100.0f;
                break;
            default: break;
            }
            tuning = horde::vulkan::raytracing::ClampRtSceneTuning(tuning);
            UpdateRtLabLabels(*sceneContext);
            return 0;
        }
        break;
    case WM_COMMAND:
        if (sceneContext)
        {
            if (sceneContext->benchmark.IsRunning())
            {
                if (LOWORD(wParam) == kExitButtonId || LOWORD(wParam) == kMenuExitId)
                {
                    DestroyWindow(hWnd);
                }
                else if (LOWORD(wParam) == kMenuPauseId)
                {
                    CancelBenchmark(*sceneContext, true);
                }
                return 0;
            }
            const int commandId = LOWORD(wParam);
            if (!sceneContext->rtLabVisible &&
                (sceneContext->deathOverlayVisible || sceneContext->endingOverlayVisible) &&
                commandId != kResumeButtonId && commandId != kRestartButtonId &&
                commandId != kRtLabButtonId && commandId != kRtLabBackButtonId &&
                commandId != kMenuRestartId && commandId != kExitButtonId &&
                commandId != kMenuExitId)
            {
                return 0;
            }
            switch (commandId)
            {
            case kResumeButtonId:
                if (sceneContext->deathOverlayVisible)
                {
                    if (!ApplyPlayerRetryCheckpoint(*sceneContext, sceneContext->playerRetryCheckpoint))
                    {
                        MessageBoxA(hWnd, "The encounter checkpoint could not be restored.", "Horde Lantern RT", MB_OK | MB_ICONERROR);
                    }
                    else
                    {
                        PlaySoundEffect(*sceneContext, "ui_select.wav");
                    }
                }
                else if (sceneContext->endingOverlayVisible)
                {
                    PlaySoundEffect(*sceneContext, "ui_select.wav");
                    ShowPauseMenu(*sceneContext, false);
                }
                else
                {
                    ShowPauseMenu(*sceneContext, !sceneContext->simulationPaused);
                }
                return 0;
            case kMenuPauseId:
                ShowPauseMenu(*sceneContext, !sceneContext->simulationPaused);
                return 0;
            case kRestartButtonId:
            case kMenuRestartId:
                CancelBenchmark(*sceneContext, false);
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
            case kRtLabButtonId:
                OpenRtLab(*sceneContext);
                return 0;
            case kRtLabLightGroupButtonId:
                sceneContext->rtLabLightGroup = static_cast<horde::vulkan::raytracing::RtLightGroup>(
                    (static_cast<std::uint32_t>(sceneContext->rtLabLightGroup) + 1u) %
                    static_cast<std::uint32_t>(horde::vulkan::raytracing::kRtLightGroupCount));
                UpdateRtLabLabels(*sceneContext);
                return 0;
            case kRtLabWorkloadButtonId:
            {
                using horde::vulkan::raytracing::RtWorkloadPreset;
                sceneContext->rtSceneTuning.workloadPreset =
                    sceneContext->rtSceneTuning.workloadPreset == RtWorkloadPreset::Lean
                        ? RtWorkloadPreset::Authored
                        : (sceneContext->rtSceneTuning.workloadPreset == RtWorkloadPreset::Authored
                               ? RtWorkloadPreset::Max : RtWorkloadPreset::Lean);
                UpdateRtLabLabels(*sceneContext);
                return 0;
            }
            case kRtLabRestoreButtonId:
                sceneContext->rtSceneTuning = {};
                sceneContext->rtLabLightGroup = horde::vulkan::raytracing::RtLightGroup::Torch;
                UpdateRtLabLabels(*sceneContext);
                return 0;
            case kRtLabBackButtonId:
                CloseRtLab(*sceneContext);
                return 0;
            case kDiagnosticsButtonId:
            case kMenuDiagnosticsId:
                ToggleDiagnostics(*sceneContext);
                return 0;
            case kRunBenchmarkButtonId:
                StartBenchmark(*sceneContext);
                return 0;
#if defined(_DEBUG)
            case kMenuDeveloperOverlayId:
                ToggleDeveloperOverlay(*sceneContext);
                return 0;
#endif
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
            case kWaterQualityButtonId:
                sceneContext->waterQuality =
                    sceneContext->waterQuality == horde::vulkan::raytracing::WaterQuality::High
                        ? horde::vulkan::raytracing::WaterQuality::Mobile
                        : (sceneContext->waterQuality == horde::vulkan::raytracing::WaterQuality::Mobile
                               ? horde::vulkan::raytracing::WaterQuality::Off
                               : horde::vulkan::raytracing::WaterQuality::High);
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
                CancelBenchmark(*sceneContext, true);
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
                            kAboutText,
                            "About Horde Lantern RT",
                            MB_OK | MB_ICONINFORMATION);
                return 0;
            case kMenuCreditsId:
                ShowCredits(hWnd);
                return 0;
            case kMoreBySamfa12ButtonId:
                PlaySoundEffect(*sceneContext, "ui_select.wav");
                OpenSamfa12Website(hWnd);
                return 0;
            case kBenchmarkCopyButtonId:
                if (!CopyTextToClipboard(hWnd, sceneContext->benchmarkReport))
                {
                    MessageBoxA(hWnd, "The benchmark report could not be copied to the clipboard.",
                                "Horde Lantern RT", MB_OK | MB_ICONERROR);
                }
                return 0;
            case kBenchmarkSaveButtonId:
                SaveBenchmarkReportAs(hWnd, sceneContext->benchmarkReport);
                return 0;
            case kBenchmarkBackButtonId:
                sceneContext->benchmarkReportVisible = false;
                sceneContext->pauseMenuVisible = true;
                ApplyOverlayState(*sceneContext);
                {
                    RECT client{};
                    GetClientRect(hWnd, &client);
                    LayoutOverlayControls(hWnd, client.right - client.left, client.bottom - client.top);
                }
                SetFocus(GetDlgItem(hWnd, kRunBenchmarkButtonId));
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
            if (sceneContext->simulationPaused && wParam == VK_TAB && (lParam & (1ll << 30)) == 0)
            {
                NavigateControllerMenu(*sceneContext,
                    (GetKeyState(VK_SHIFT) & 0x8000) != 0 ? -1 : 1);
                return 0;
            }
            if (sceneContext->simulationPaused && (wParam == VK_UP || wParam == VK_DOWN) &&
                (lParam & (1ll << 30)) == 0)
            {
                NavigateControllerMenu(*sceneContext, wParam == VK_UP ? -1 : 1);
                return 0;
            }
            if (sceneContext->rtLabVisible &&
                (wParam == VK_PRIOR || wParam == VK_NEXT || wParam == VK_HOME || wParam == VK_END) &&
                (lParam & (1ll << 30)) == 0)
            {
                const WPARAM scrollCommand = wParam == VK_PRIOR ? SB_PAGEUP :
                    (wParam == VK_NEXT ? SB_PAGEDOWN : (wParam == VK_HOME ? SB_TOP : SB_BOTTOM));
                SendMessageA(hWnd, WM_VSCROLL, scrollCommand, 0);
                return 0;
            }
            if (sceneContext->simulationPaused && (wParam == VK_LEFT || wParam == VK_RIGHT) &&
                (lParam & (1ll << 30)) == 0)
            {
                if (!AdjustFocusedControllerSlider(*sceneContext, wParam == VK_RIGHT))
                    NavigateControllerMenu(*sceneContext, wParam == VK_RIGHT ? 1 : -1);
                return 0;
            }
            if (sceneContext->simulationPaused && wParam == VK_RETURN &&
                (GetKeyState(VK_MENU) & 0x8000) == 0 && (lParam & (1ll << 30)) == 0)
            {
                if (HWND focused = GetFocus()) SendMessageA(focused, BM_CLICK, 0, 0);
                return 0;
            }
            if (sceneContext->simulation.Snapshot().playerVitals.phase != horde::gameplay::PlayerLifePhase::Alive)
            {
                if (wParam == VK_F4 && (GetKeyState(VK_MENU) & 0x8000) != 0)
                {
                    break;
                }
                return 0;
            }
            if (wParam == VK_ESCAPE)
            {
                if (sceneContext->rtLabVisible)
                {
                    CloseRtLab(*sceneContext);
                    return 0;
                }
                if (sceneContext->benchmark.IsRunning())
                {
                    CancelBenchmark(*sceneContext, true);
                    return 0;
                }
                if (sceneContext->benchmarkReportVisible)
                {
                    sceneContext->benchmarkReportVisible = false;
                    sceneContext->pauseMenuVisible = true;
                    ApplyOverlayState(*sceneContext);
                    return 0;
                }
                ShowPauseMenu(*sceneContext, !sceneContext->simulationPaused);
                return 0;
            }
            if (sceneContext->benchmark.IsRunning())
            {
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
            if ((wParam >= VK_F5 && wParam <= VK_F9) || wParam == VK_F11)
            {
                sceneContext->rtLabRouteTainted = true;
            }
            if (wParam == VK_F3 && (lParam & (1ll << 30)) == 0)
            {
                ToggleDeveloperOverlay(*sceneContext);
                return 0;
            }
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
                sceneContext->simulation.ApplyShowcaseCheckpoint(10);
                sceneContext->simulation.ClearEvents();
                MirrorSimulationSnapshot(*sceneContext);
                return 0;
            }
            if (wParam == VK_F7)
            {
                sceneContext->debugEnemyOverride = horde::gameplay::EnemyKind::None;
                sceneContext->simulation.ApplyShowcaseCheckpoint(3);
                sceneContext->simulation.ClearEvents();
                MirrorSimulationSnapshot(*sceneContext);
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
                DebugWarpSimulation(*sceneContext, point.x, point.z, point.yaw, point.pitch);
                sceneContext->debugValidationPoint =
                    (sceneContext->debugValidationPoint + 1u) %
                    static_cast<uint32_t>(sizeof(kValidationPoints) / sizeof(kValidationPoints[0]));
                return 0;
            }
            if (wParam == VK_F9)
            {
                // Inspect the settled prop from inside the same corridor leg
                // without resetting the already-triggered torch-failure sequence.
                DebugWarpSimulation(*sceneContext, 1.20f, -15.20f,
                                    -1.57079632679f, -0.32f);
                return 0;
            }
            if (wParam == VK_F10 && (lParam & (1ll << 30)) == 0)
            {
                const char* clip = (sceneContext->playerFootstepVariant++ & 1) == 0
                    ? "player_step_1.wav" : "player_step_2.wav";
                PlayAmbientSoundEffect(*sceneContext, clip);
                return 0;
            }
            if (wParam == VK_F11 && (lParam & (1ll << 30)) == 0)
            {
                // Debug-only authoring preview: reuse the deterministic open-roof
                // checkpoint, then advance only the dawn hold to exercise the
                // real completed-finale UI without changing the 12 capture gates.
                if (const horde::gameplay::ShowcaseCheckpoint* finale =
                        horde::gameplay::FindShowcaseCheckpoint(11))
                {
                    ApplyCaptureCheckpoint(*sceneContext, *finale);
                    const int dawnFrames = static_cast<int>(
                        horde::gameplay::LichEncounter::kFinaleDawnRevealDuration / 0.05f) + 2;
                    horde::gameplay::simulation::InputSnapshot input = sceneContext->simulationInput;
                    input.paused = false;
                    input.damageEnabled = false;
                    input.hasAuthoritativePlayerPose = false;
                    for (int frame = 0; frame < dawnFrames; ++frame)
                    {
                        sceneContext->simulation.StepFixed(
                            input, 0.05f, ++sceneContext->inputPublicationSequence);
                    }
                    sceneContext->simulation.ClearEvents();
                    sceneContext->simulation.ResetTiming();
                    MirrorSimulationSnapshot(*sceneContext);
                    ShowEndingMenu(*sceneContext);
                }
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
                ++sceneContext->attackSequence;
                return 0;
            }
            if (!sceneContext->simulationPaused && wParam == 'Q' && (lParam & (1ll << 30)) == 0)
            {
                ++sceneContext->parrySequence;
                return 0;
            }
            if (!sceneContext->simulationPaused && wParam == 'E' &&
                (lParam & (1ll << 30)) == 0)
            {
                ++sceneContext->interactSequence;
                return 0;
            }
            if (!sceneContext->simulationPaused && wParam == 'F' &&
                (lParam & (1ll << 30)) == 0)
            {
                ++sceneContext->toggleHeldLightPoseSequence;
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
        if (sceneContext && sceneContext->controlsEnabled && !sceneContext->simulationPaused &&
            sceneContext->simulation.Snapshot().playerVitals.phase == horde::gameplay::PlayerLifePhase::Alive &&
            !sceneContext->benchmark.IsRunning())
        {
            SetFocus(hWnd);
            SetCapture(hWnd);
            GetCursorPos(&sceneContext->mouseRestorePosition);
            ShowCursor(FALSE);
            sceneContext->mouseCursorHidden = true;
            sceneContext->mouseLookActive = true;
            RECT clientRect{};
            GetClientRect(hWnd, &clientRect);
            POINT centre{(clientRect.right - clientRect.left) / 2, (clientRect.bottom - clientRect.top) / 2};
            sceneContext->lastMousePosition = centre;
            POINT screenCentre = centre;
            ClientToScreen(hWnd, &screenCentre);
            RECT screenRect{clientRect};
            ClientToScreen(hWnd, reinterpret_cast<POINT*>(&screenRect.left));
            ClientToScreen(hWnd, reinterpret_cast<POINT*>(&screenRect.right));
            ClipCursor(&screenRect);
            SetCursorPos(screenCentre.x, screenCentre.y);
            return 0;
        }
        break;
    case WM_RBUTTONDOWN:
        if (sceneContext && sceneContext->controlsEnabled && !sceneContext->simulationPaused &&
            sceneContext->simulation.Snapshot().playerVitals.phase == horde::gameplay::PlayerLifePhase::Alive &&
            !sceneContext->benchmark.IsRunning())
        {
            ++sceneContext->attackSequence;
            return 0;
        }
        break;
    case WM_MOUSEMOVE:
        if (sceneContext && sceneContext->controlsEnabled && !sceneContext->simulationPaused &&
            sceneContext->simulation.Snapshot().playerVitals.phase == horde::gameplay::PlayerLifePhase::Alive &&
            !sceneContext->benchmark.IsRunning() && sceneContext->mouseLookActive)
        {
            const POINT currentMousePosition{
                static_cast<LONG>(static_cast<short>(LOWORD(lParam))),
                static_cast<LONG>(static_cast<short>(HIWORD(lParam)))};
            const LONG deltaX = currentMousePosition.x - sceneContext->lastMousePosition.x;
            const LONG deltaY = currentMousePosition.y - sceneContext->lastMousePosition.y;
            sceneContext->lastMousePosition = currentMousePosition;
            sceneContext->cameraYaw += static_cast<float>(deltaX) * 0.0036f * sceneContext->mouseSensitivity;
            sceneContext->cameraPitch = std::clamp(sceneContext->cameraPitch - static_cast<float>(deltaY) * 0.0028f * sceneContext->mouseSensitivity, -0.32f, 0.28f);
            RECT clientRect{};
            GetClientRect(hWnd, &clientRect);
            const POINT centre{(clientRect.right - clientRect.left) / 2, (clientRect.bottom - clientRect.top) / 2};
            sceneContext->lastMousePosition = centre;
            POINT screenCentre = centre;
            ClientToScreen(hWnd, &screenCentre);
            SetCursorPos(screenCentre.x, screenCentre.y);
            return 0;
        }
        break;
    case WM_CAPTURECHANGED:
        if (sceneContext && sceneContext->controlsEnabled)
        {
            ClearDesktopInput(*sceneContext);
            return 0;
        }
        break;
    case WM_KILLFOCUS:
        if (sceneContext && sceneContext->controlsEnabled)
        {
            ClearDesktopInput(*sceneContext);
        }
        break;
    case WM_ACTIVATEAPP:
        if (sceneContext && wParam == FALSE && sceneContext->benchmark.IsRunning())
        {
            CancelBenchmark(*sceneContext, true);
        }
        break;
    case WM_SIZE:
        if (sceneContext && sceneContext->benchmark.IsRunning() && wParam != SIZE_MINIMIZED)
        {
            CancelBenchmark(*sceneContext, true);
        }
        LayoutOverlayControls(hWnd, LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_GETMINMAXINFO:
    {
        if (GetPropA(hWnd, kCaptureModeProperty) != nullptr)
        {
            return 0;
        }
        auto* minMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
        minMaxInfo->ptMinTrackSize.x = ScaleForDpi(hWnd, 520);
        minMaxInfo->ptMinTrackSize.y = ScaleForDpi(hWnd, 560);
        return 0;
    }
    case WM_DPICHANGED:
    {
        if (sceneContext && sceneContext->benchmark.IsRunning())
        {
            CancelBenchmark(*sceneContext, true);
        }
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
        COLORREF textColor = RGB(255, 208, 122);
        if (sceneContext && GetDlgCtrlID(reinterpret_cast<HWND>(lParam)) == kVitalityHudControlId)
        {
            const int vitality = sceneContext->simulation.Snapshot().playerVitals.vitality;
            textColor = vitality >= 3 ? RGB(255, 208, 122) :
                        (vitality == 2 ? RGB(255, 154, 67) : RGB(255, 83, 72));
        }
        SetTextColor(dc, textColor);
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
        RemovePropA(hWnd, kCaptureModeProperty);
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
#if defined(_DEBUG)
    AppendMenuA(help, MF_STRING, kMenuDeveloperOverlayId, "Live developer &overlay\tF3");
#endif
    AppendMenuA(help, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(help, MF_STRING, kMenuCreditsId, "&Credits && licences");
    AppendMenuA(help, MF_STRING, kMenuAboutId, "&About");
    AppendMenuA(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(help), "&Help");
    return bar;
}

LRESULT CALLBACK ControllerFocusOutlineSubclass(
    HWND control,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR subclassId,
    DWORD_PTR referenceData)
{
    (void)referenceData;
    if (message == WM_NCDESTROY)
    {
        RemoveWindowSubclass(control, ControllerFocusOutlineSubclass, subclassId);
        return DefSubclassProc(control, message, wParam, lParam);
    }
    if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) &&
        (wParam == VK_TAB || wParam == VK_UP || wParam == VK_DOWN ||
         wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_RETURN ||
         wParam == VK_ESCAPE || wParam == VK_PRIOR || wParam == VK_NEXT ||
         wParam == VK_HOME || wParam == VK_END))
    {
        return SendMessageA(GetParent(control), message, wParam, lParam);
    }
    if (message == WM_MOUSEWHEEL)
    {
        HWND parent = GetParent(control);
        auto* context = reinterpret_cast<VulkanSurfaceContext*>(GetWindowLongPtrA(parent, GWLP_USERDATA));
        if (context != nullptr && context->rtLabVisible)
        {
            return SendMessageA(parent, message, wParam, lParam);
        }
    }

    const LRESULT result = DefSubclassProc(control, message, wParam, lParam);
    if (message == WM_SETFOCUS || message == WM_KILLFOCUS)
    {
        InvalidateRect(control, nullptr, TRUE);
        UpdateWindow(control);
    }
    if (message == WM_PAINT && GetFocus() == control)
    {
        HDC dc = GetDC(control);
        if (dc != nullptr)
        {
            RECT border{};
            GetClientRect(control, &border);
            static HBRUSH gold = CreateSolidBrush(RGB(255, 177, 55));
            static HBRUSH brightGold = CreateSolidBrush(RGB(255, 221, 137));
            FrameRect(dc, &border, gold);
            InflateRect(&border, -1, -1);
            FrameRect(dc, &border, gold);
            InflateRect(&border, -1, -1);
            FrameRect(dc, &border, brightGold);
            ReleaseDC(control, dc);
        }
    }
    return result;
}

void InstallControllerFocusOutline(HWND control)
{
    if (control != nullptr)
    {
        constexpr UINT_PTR kControllerFocusOutlineSubclassId = 1u;
        SetWindowSubclass(control,
                          ControllerFocusOutlineSubclass,
                          kControllerFocusOutlineSubclassId,
                          0u);
    }
}

int CreateAndShowWindow(const std::string& diagnosticText,
                        horde::vulkan::DeviceCapabilities& capabilities,
                        const std::filesystem::path& textReportPath,
                        const std::filesystem::path& jsonReportPath,
                        const std::filesystem::path* captureDirectory,
                        const std::string* developmentCheckpoint)
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
    constexpr DWORD windowStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VSCROLL;
    int windowWidth = MulDiv(1000, static_cast<int>(systemDpi == 0u ? kDefaultDpi : systemDpi), static_cast<int>(kDefaultDpi));
    int windowHeight = MulDiv(700, static_cast<int>(systemDpi == 0u ? kDefaultDpi : systemDpi), static_cast<int>(kDefaultDpi));
    if (captureDirectory != nullptr)
    {
        RECT captureRect{0, 0, static_cast<LONG>(kCaptureWidth), static_cast<LONG>(kCaptureHeight)};
        AdjustWindowRectEx(&captureRect, windowStyle, TRUE, 0u);
        windowWidth = captureRect.right - captureRect.left;
        windowHeight = captureRect.bottom - captureRect.top;
    }
    HWND hWnd = CreateWindowExA(
        0,
        kWindowClassName,
        kWindowTitle,
        windowStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowWidth,
        windowHeight,
        nullptr,
        CreateApplicationMenu(),
        instance,
        nullptr);
    if (!hWnd)
    {
        std::cerr << "Failed to create diagnostic window." << std::endl;
        return 1;
    }

    if (captureDirectory != nullptr)
    {
        SetPropA(hWnd, kCaptureModeProperty, reinterpret_cast<HANDLE>(1));
        RECT windowRect{};
        RECT actualClientRect{};
        GetWindowRect(hWnd, &windowRect);
        GetClientRect(hWnd, &actualClientRect);
        const int adjustedWidth = (windowRect.right - windowRect.left) +
                                  static_cast<int>(kCaptureWidth) - (actualClientRect.right - actualClientRect.left);
        const int adjustedHeight = (windowRect.bottom - windowRect.top) +
                                   static_cast<int>(kCaptureHeight) - (actualClientRect.bottom - actualClientRect.top);
        SetWindowPos(hWnd, nullptr, 0, 0, adjustedWidth, adjustedHeight,
                     SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
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
        InstallControllerFocusOutline(control);
        return control;
    };

    createStatic(kHudControlId, kHudStartingText, SS_LEFT | SS_CENTERIMAGE);
    createStatic(kVitalityHudControlId, "VITALITY  3 / 3", SS_CENTER | SS_CENTERIMAGE);
#if defined(_DEBUG)
    if (HWND developerOverlay = createStatic(kDeveloperOverlayId, "DEV OVERLAY STARTING...", SS_LEFT | SS_NOPREFIX))
    {
        EnableWindow(developerOverlay, FALSE);
        ShowWindow(developerOverlay, SW_HIDE);
    }
#endif
    createStatic(kPauseTitleId, "HORDE LANTERN RT  |  SHOWCASE ALPHA", SS_CENTER | SS_CENTERIMAGE);
    createStatic(kEndingBodyId,
                 "The old guard bound the lich beneath this ruin and left one lantern to guide whoever came after.\r\n\r\n"
                 "Its flame died when the final seal opened. Now the staff is silent, the roof gives way, and stolen morning returns to the halls.",
                 SS_CENTER | SS_NOPREFIX);
    createButton(kResumeButtonId, "ENTER THE RUIN / RESUME");
    createButton(kRestartButtonId, "RESTART ROUTE");
    createButton(kControlsButtonId, "CONTROLS");
    createButton(kSettingsButtonId, "SETTINGS");
    createButton(kRtLabButtonId, "RT LAB");
    createButton(kDiagnosticsButtonId, "RT DIAGNOSTICS");
    createButton(kRunBenchmarkButtonId, "RUN BENCHMARK");
    createButton(kMoreBySamfa12ButtonId, "MORE BY SAMFA12");
    createButton(kExitButtonId, "QUIT DEMO");
    createStatic(kBenchmarkTitleId, "BENCHMARK REPORT  |  SELECTABLE TEXT", SS_CENTER | SS_CENTERIMAGE);
    createButton(kBenchmarkCopyButtonId, "COPY REPORT");
    createButton(kBenchmarkSaveButtonId, "SAVE AS...");
    createButton(kBenchmarkBackButtonId, "BACK TO MENU");
    createStatic(kSettingsTitleId, "SETTINGS  |  SAVED BESIDE THE DEMO", SS_CENTER | SS_CENTERIMAGE);
    createButton(kSfxButtonId, "SOUND EFFECTS: ON");
    createButton(kSensitivityButtonId, "LOOK SENSITIVITY: NORMAL");
    createButton(kWaterQualityButtonId, "RT WATER: HIGH");
    createStatic(kRenderScaleLabelId, "RENDER RESOLUTION: 100%", SS_CENTER | SS_CENTERIMAGE);
    HWND renderScaleSlider = CreateWindowExA(0, TRACKBAR_CLASSA, "",
                                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_AUTOTICKS,
                                              0, 0, 100, 38, hWnd,
                                              reinterpret_cast<HMENU>(static_cast<INT_PTR>(kRenderScaleSliderId)), instance, nullptr);
    InstallControllerFocusOutline(renderScaleSlider);
    SendMessageA(renderScaleSlider, TBM_SETRANGE, TRUE, MAKELPARAM(50, 100));
    SendMessageA(renderScaleSlider, TBM_SETTICFREQ, 10, 0);
    SendMessageA(renderScaleSlider, TBM_SETPOS, TRUE, 100);
    createButton(kFullscreenButtonId, "DISPLAY: WINDOWED");
    createButton(kSettingsBackButtonId, "BACK");

    HWND rtLabPanel = CreateWindowExA(WS_EX_LAYERED, "STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_BLACKRECT,
        0, 0, 100, 100, hWnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kRtLabPanelId)), instance, nullptr);
    if (rtLabPanel != nullptr) SetLayeredWindowAttributes(rtLabPanel, 0, 218u, LWA_ALPHA);
    createStatic(kRtLabTitleId, "RT LAB  |  LIVE VULKAN RAY TRACING", SS_CENTER | SS_CENTERIMAGE);
    createStatic(kRtLabTelemetryId, "GPU RT: WARMING UP", SS_CENTER | SS_CENTERIMAGE);
    createStatic(kRtLabWaterfallLabelId, "WATERFALL WIDTH: 100%", SS_LEFT | SS_CENTERIMAGE);
    createStatic(kRtLabRoofLabelId, "FINALE ROOF: AUTHORED (ADJUST TO OVERRIDE)", SS_LEFT | SS_CENTERIMAGE);
    createStatic(kRtLabDawnLabelId, "FINALE DAWN: AUTHORED (ADJUST TO OVERRIDE)", SS_LEFT | SS_CENTERIMAGE);
    createStatic(kRtLabFogLabelId, "FOG DENSITY: 100%", SS_LEFT | SS_CENTERIMAGE);
    createStatic(kRtLabFireStrengthLabelId, "FLAME STRENGTH: 100%", SS_LEFT | SS_CENTERIMAGE);
    createStatic(kRtLabFireTurbulenceLabelId, "FLAME TURBULENCE: 100%", SS_LEFT | SS_CENTERIMAGE);
    createStatic(kRtLabFireSmokeLabelId, "FLAME SMOKE: 100%", SS_LEFT | SS_CENTERIMAGE);
    createStatic(kRtLabGlassVisibilityLabelId, "GLASS FIXTURE: HIDDEN", SS_LEFT | SS_CENTERIMAGE);
    createStatic(kRtLabGlassTransmissionLabelId, "GLASS TRANSMISSION: 94%", SS_LEFT | SS_CENTERIMAGE);
    createStatic(kRtLabGlassIorLabelId, "GLASS IOR: 1.52", SS_LEFT | SS_CENTERIMAGE);
    createStatic(kRtLabGlassRoughnessLabelId, "GLASS ROUGHNESS: 12%", SS_LEFT | SS_CENTERIMAGE);
    createButton(kRtLabLightGroupButtonId, "LIGHT GROUP: TORCH");
    createStatic(kRtLabHueLabelId, "LIGHT HUE SHIFT: 0 DEG", SS_LEFT | SS_CENTERIMAGE);
    createStatic(kRtLabIntensityLabelId, "LIGHT INTENSITY: 100%", SS_LEFT | SS_CENTERIMAGE);
    createButton(kRtLabWorkloadButtonId, "RT WORKLOAD: AUTHORED");
    createButton(kRtLabRestoreButtonId, "RESTORE AUTHORED");
    createButton(kRtLabBackButtonId, "BACK");
    const auto createRtLabSlider = [&](const int id, const int minimum, const int maximum, const int position)
    {
        HWND slider = CreateWindowExA(0, TRACKBAR_CLASSA, "",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_AUTOTICKS,
            0, 0, 100, 38, hWnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
        InstallControllerFocusOutline(slider);
        SendMessageA(slider, TBM_SETRANGE, TRUE, MAKELPARAM(minimum, maximum));
        SendMessageA(slider, TBM_SETTICFREQ, 25, 0);
        SendMessageA(slider, TBM_SETPOS, TRUE, position);
        return slider;
    };
    createRtLabSlider(kRtLabWaterfallSliderId, 25, 200, 100);
    createRtLabSlider(kRtLabRoofSliderId, 0, 100, 100);
    createRtLabSlider(kRtLabDawnSliderId, 0, 100, 100);
    createRtLabSlider(kRtLabFogSliderId, 0, 200, 100);
    createRtLabSlider(kRtLabFireStrengthSliderId, 0, 200, 100);
    createRtLabSlider(kRtLabFireTurbulenceSliderId, 0, 200, 100);
    createRtLabSlider(kRtLabFireSmokeSliderId, 0, 200, 100);
    createRtLabSlider(kRtLabGlassVisibilitySliderId, 0, 100, 0);
    createRtLabSlider(kRtLabGlassTransmissionSliderId, 0, 100, 94);
    createRtLabSlider(kRtLabGlassIorSliderId, 100, 250, 152);
    createRtLabSlider(kRtLabGlassRoughnessSliderId, 0, 100, 12);
    createRtLabSlider(kRtLabHueSliderId, -180, 180, 0);
    createRtLabSlider(kRtLabIntensitySliderId, 0, 200, 100);
    for (const int id : {kRtLabPanelId, kRtLabTitleId, kRtLabTelemetryId,
                         kRtLabWaterfallLabelId, kRtLabWaterfallSliderId,
                         kRtLabRoofLabelId, kRtLabRoofSliderId,
                         kRtLabDawnLabelId, kRtLabDawnSliderId,
                         kRtLabFogLabelId, kRtLabFogSliderId,
                         kRtLabFireStrengthLabelId, kRtLabFireStrengthSliderId,
                         kRtLabFireTurbulenceLabelId, kRtLabFireTurbulenceSliderId,
                         kRtLabFireSmokeLabelId, kRtLabFireSmokeSliderId,
                         kRtLabGlassVisibilityLabelId, kRtLabGlassVisibilitySliderId,
                         kRtLabGlassTransmissionLabelId, kRtLabGlassTransmissionSliderId,
                         kRtLabGlassIorLabelId, kRtLabGlassIorSliderId,
                         kRtLabGlassRoughnessLabelId, kRtLabGlassRoughnessSliderId,
                         kRtLabLightGroupButtonId, kRtLabHueLabelId, kRtLabHueSliderId,
                         kRtLabIntensityLabelId, kRtLabIntensitySliderId,
                         kRtLabWorkloadButtonId, kRtLabRestoreButtonId, kRtLabBackButtonId})
    {
        if (HWND control = GetDlgItem(hWnd, id)) ShowWindow(control, SW_HIDE);
    }

    ApplyDpiScaledFonts(hWnd);

    const std::string windowText = WindowSafeText(diagnosticText);
    const bool sceneMode = capabilities.rtMode == horde::vulkan::RtMode::RayTracingPipeline;
    const std::string windowTitle = sceneMode
        ? kWindowTitle
        : MakeWindowTitle(diagnosticText);
    SetWindowTextA(edit, windowText.c_str());
    SetWindowTextA(hWnd, windowTitle.c_str());
    if (sceneMode)
    {
        ShowWindow(edit, SW_HIDE);
    }

    LayoutOverlayControls(hWnd, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

    if (captureDirectory != nullptr)
    {
        EnumChildWindows(hWnd, [](HWND child, LPARAM) -> BOOL {
            ShowWindow(child, SW_HIDE);
            return TRUE;
        }, 0);
    }
    ShowWindow(hWnd, captureDirectory != nullptr ? SW_SHOWNOACTIVATE : SW_SHOW);
    UpdateWindow(hWnd);
    if (captureDirectory != nullptr)
    {
        RECT windowRect{};
        RECT actualClientRect{};
        GetWindowRect(hWnd, &windowRect);
        GetClientRect(hWnd, &actualClientRect);
        SetWindowPos(hWnd, nullptr, 0, 0,
                     (windowRect.right - windowRect.left) + static_cast<int>(kCaptureWidth) -
                         (actualClientRect.right - actualClientRect.left),
                     (windowRect.bottom - windowRect.top) + static_cast<int>(kCaptureHeight) -
                         (actualClientRect.bottom - actualClientRect.top),
                     SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    }
    if (captureDirectory == nullptr)
    {
        SetForegroundWindow(hWnd);
        SetFocus(sceneMode ? hWnd : edit);
    }

    const int result = RunDiagnosticSwapchainWindow(
        hWnd, capabilities, textReportPath, jsonReportPath, captureDirectory,
        developmentCheckpoint);
    if (captureDirectory != nullptr && IsWindow(hWnd))
    {
        DestroyWindow(hWnd);
    }
    return result;
}

} // namespace

namespace horde::platform::windows
{

int RunDiagnosticWindow(const int showCommand)
{
    (void)showCommand;
    SetProcessDPIAware();

    const CaptureLaunchOptions launchOptions = ParseCaptureLaunchOptions();
    if (!launchOptions.error.empty())
    {
        std::cerr << launchOptions.error << '\n';
        return 2;
    }
#if !defined(_DEBUG)
    if (launchOptions.requested)
    {
        std::cerr << "--capture-showcase is a Debug-only automation mode; Release builds reject it.\n";
        return 2;
    }
#endif

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

    const std::filesystem::path* captureDirectory = launchOptions.requested
        ? &launchOptions.outputDirectory
        : nullptr;
    const std::string* developmentCheckpoint = launchOptions.developmentCheckpoint.empty()
        ? nullptr
        : &launchOptions.developmentCheckpoint;
    return CreateAndShowWindow(diagnosticText, capabilities, textReportPath, jsonReportPath,
                               captureDirectory, developmentCheckpoint);
}

} // namespace horde::platform::windows
