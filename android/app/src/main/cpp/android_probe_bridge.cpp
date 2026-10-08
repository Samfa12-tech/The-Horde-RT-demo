#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <chrono>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <vector>
#include <sys/stat.h>

#include <jni.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>

#include "ui/DiagnosticOverlay.h"
#include "graphics/GraphicsSettings.h"
#include "graphics/RtPresentationTransform.h"
#include "graphics/EntryMenuHandoff.h"
#include "graphics/ForegroundPauseRenderCadence.h"
#include "telemetry/InputPresentationTrace.h"
#include "graphics/GraphicsPreviewPerformance.h"
#if (defined(HORDE_RT_DEBUG_CHECKPOINTS) && !defined(NDEBUG)) || HORDE_RT_ANDROID_MOTION_VALIDATION
#include "platform/android/AndroidMotionEvidencePolicy.h"
#include "telemetry/MotionEvidenceLedger.h"
#endif
#if !defined(NDEBUG)
#include "telemetry/CombatTimingTrace.h"
#endif
#include "reporting/PlaytestReport.h"
#include "reporting/PlaytestSubmission.h"
#include "reporting/BenchmarkSummaryReport.h"
#include "gameplay/CorridorCollision.h"
#include "gameplay/DevelopmentCheckpoints.h"
#include "gameplay/DevelopmentCheckpointSimulation.h"
#include "gameplay/ShowcaseBenchmark.h"
#include "gameplay/LanternBenchmarkScenario.h"
#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/ShowcaseCheckpoints.h"
#include "gameplay/ShowcaseReplay.h"
#include "gameplay/SpatialAudio.h"
#include "gameplay/SwordCombat.h"
#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/simulation/BoundedTransportQueue.h"
#include "gameplay/simulation/InputMailbox.h"
#include "platform/android/AndroidRtLabState.h"
#include "platform/android/AndroidMusicPlayback.h"
#include "platform/android/SurfaceSessionMailbox.h"
#include "platform/android/SurfacePresentationPolicy.h"
#include "platform/android/GameplayEventMetadata.h"
#include "update/GitHubReleaseUpdater.h"
#include "vulkan/GpuFrameTimer.h"
#include "vulkan/PipelineCacheSeed.h"
#include "vulkan/RtCapabilityReport.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/PresentCompletion.h"
#if HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION
#include "vulkan/PresentTimingEvidence.h"
#endif
#include "vulkan/raytracing/PresentableTinyRtScene.h"
#include "vulkan/raytracing/RtFrameEvidenceCoordinator.h"
#include "vulkan/raytracing/RtDeviceEnablePlan.h"
#include "telemetry/RtBenchmarkEvidenceRun.h"
#include "telemetry/RtEvidencePublication.h"
#include "vulkan/raytracing/SimulationFrameAdapter.h"
#if HORDE_RT_STAGED_PRIMARY_TIMING
#include "vulkan/raytracing/experimental/StagedPrimaryProfile.h"
#endif

#if (defined(HORDE_RT_DEBUG_CHECKPOINTS) && !defined(NDEBUG)) || HORDE_RT_ANDROID_MOTION_VALIDATION
#define HORDE_RT_ANDROID_MOTION_EVIDENCE 1
#else
#define HORDE_RT_ANDROID_MOTION_EVIDENCE 0
#endif

#ifndef HORDE_RT_BUILD_ID
#define HORDE_RT_BUILD_ID "development"
#endif
namespace
{

constexpr const char* kTag = "HordeRtProbeBridge";
constexpr const char* kReportDirectory = "reports";
constexpr const char* kTextReportFilename = "vulkan_capability_report.txt";
constexpr const char* kJsonReportFilename = "vulkan_capability_report.json";
constexpr const char* kShowcaseDebugStateFilename = "showcase_debug_state.json";
constexpr float kDefaultAndroidRtRenderScale = 0.50f;
// One frame in flight keeps the dynamically refit held-torch TLAS safely synchronized with its host-written instance buffer.
constexpr uint32_t kMaxFramesInFlight = 1u;

std::string JsonUtf8String(const std::string_view value)
{
    static constexpr char kHex[] = "0123456789abcdef";
    std::string escaped;
    escaped.reserve(value.size() + 2u);
    escaped.push_back('"');
    for (const unsigned char byte : value)
    {
        switch (byte)
        {
        case '"': escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\b': escaped += "\\b"; break;
        case '\f': escaped += "\\f"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default:
            if (byte < 0x20u)
            {
                escaped += "\\u00";
                escaped.push_back(kHex[(byte >> 4u) & 0x0fu]);
                escaped.push_back(kHex[byte & 0x0fu]);
            }
            else
            {
                escaped.push_back(static_cast<char>(byte));
            }
            break;
        }
    }
    escaped.push_back('"');
    return escaped;
}

jbyteArray NewUtf8ByteArray(JNIEnv* env, const std::string& value)
{
    if (value.size() > static_cast<std::size_t>(std::numeric_limits<jsize>::max())) return nullptr;
    jbyteArray bytes = env->NewByteArray(static_cast<jsize>(value.size()));
    if (bytes != nullptr && !value.empty())
    {
        env->SetByteArrayRegion(bytes, 0, static_cast<jsize>(value.size()),
                                reinterpret_cast<const jbyte*>(value.data()));
    }
    return bytes;
}

const char* DebugPlayerCombatActionName(const horde::gameplay::PlayerCombatAction action)
{
    using horde::gameplay::PlayerCombatAction;
    switch (action)
    {
    case PlayerCombatAction::SwingWindup: return "swing-windup";
    case PlayerCombatAction::SwingActive: return "swing-active";
    case PlayerCombatAction::SwingRecovery: return "swing-recovery";
    case PlayerCombatAction::UpwardSliceWindup: return "upward-windup";
    case PlayerCombatAction::UpwardSliceActive: return "upward-active";
    case PlayerCombatAction::UpwardSliceRecovery: return "upward-recovery";
    case PlayerCombatAction::ParryStartup: return "parry-startup";
    case PlayerCombatAction::ParryActive: return "parry-active";
    case PlayerCombatAction::ParryRecovery: return "parry-recovery";
    case PlayerCombatAction::Idle: return "idle";
    }
    return "unknown";
}

constexpr auto kDefaultPlayerPresentationRoute =
    horde::vulkan::raytracing::kProductionPlayerRenderRoute;

struct GraphicsCommandLatency
{
    std::uint64_t serial = 0u, receivedNs = 0u, publishedNs = 0u, observedNs = 0u;
    std::uint64_t firstRenderNs = 0u, firstPresentedNs = 0u;
    double idleMilliseconds = 0.0, destroyMilliseconds = 0.0, initialiseMilliseconds = 0.0;
    bool active = false, glassChanged = false;
};

std::uint64_t GraphicsSteadyNs()
{
    return horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr);
}

double GraphicsElapsedMs(std::uint64_t start, std::uint64_t end)
{
    return start != 0u && end >= start ? static_cast<double>(end - start) * 1.0e-6 : -1.0;
}

using AndroidCompiledPipelineCache =
    horde::vulkan::raytracing::RtBundleCompiledPipelineCache;
using AndroidCompiledPipelineObjects =
    horde::vulkan::raytracing::RtBundleCompiledPipelineObjects;

struct AndroidCompiledPipelineCacheOwner;

void DestroyAndroidCompiledPipelineObjects(
    void* user, AndroidCompiledPipelineObjects& objects) noexcept;

struct AndroidCompiledPipelineCacheOwner
{
    explicit AndroidCompiledPipelineCacheOwner(VkDevice selectedDevice) noexcept
        : device(selectedDevice),
          deviceIdentity(static_cast<std::uint64_t>(
              reinterpret_cast<std::uintptr_t>(selectedDevice))),
          cache(deviceIdentity, this, DestroyAndroidCompiledPipelineObjects)
    {
    }

    VkDevice device = VK_NULL_HANDLE;
    std::uint64_t deviceIdentity = 0u;
    AndroidCompiledPipelineCache cache;
};

void DestroyAndroidCompiledPipelineObjects(
    void* user, AndroidCompiledPipelineObjects& objects) noexcept
{
    const auto* owner = static_cast<AndroidCompiledPipelineCacheOwner*>(user);
    if (owner == nullptr || owner->device == VK_NULL_HANDLE) return;
    for (VkPipeline& pipeline : objects.pipelines)
    {
        if (pipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(owner->device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }
    if (objects.pipelineLayout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(owner->device, objects.pipelineLayout, nullptr);
    objects.pipelineLayout = VK_NULL_HANDLE;
    if (objects.descriptorSetLayout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(owner->device, objects.descriptorSetLayout, nullptr);
    objects.descriptorSetLayout = VK_NULL_HANDLE;
}

#if HORDE_RT_ANDROID_MOTION_EVIDENCE
struct AndroidMotionRun
{
    horde::gameplay::validation::MotionEvidenceScenario scenario;
    horde::gameplay::validation::MotionScenario selected{};
    horde::telemetry::MotionEvidenceLedger ledger;
    horde::platform::android::AndroidMotionEvidenceScope scope;
    horde::graphics::GraphicsSettings settings;
    horde::gameplay::simulation::SimulationCommandSequences commands{}, externalCommands{};
    horde::gameplay::simulation::CombatInputEdgeHistory edges;
    std::string id, path, captures = "[";
    std::uint64_t requestedNs = 0u;
    double lastCaptureSeconds = -2.0;
    horde::gameplay::validation::MotionStage lastCaptureStage{};
    horde::gameplay::PlayerCombatAction lastPresentedAction = horde::gameplay::PlayerCombatAction::Idle;
    unsigned capturedDrawThresholds = 0u;
    unsigned capturedTorchThresholds = 0u;
    unsigned captureCount = 0u;
    float externalYaw = 0.0f, externalPitch = 0.0f, externalTorch = 0.0f;
    bool armed = false, finished = false, retryPending = false;
    bool equipmentSeedActive = false;
    bool terminalTimingDrainPending = false;
    std::uint64_t terminalTimingDrainStartedNs = 0u;
#if HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION
    std::size_t timingRowBaseline = 0u;
    horde::vulkan::PresentTimingEvidence::Counters timingCountersBaseline{};
#endif
};
std::mutex gMotionRequestMutex;
std::string gMotionRequestedId;
horde::gameplay::validation::MotionScenario gMotionRequestedScenario{};
std::atomic<int> gMotionStatus{0}; // 0 none, 1 requested, 2 active, 3 complete, 4 failed
std::atomic<bool> gMotionReleaseRequested{false};
std::atomic<bool> gMotionValidationRequested{false};
#endif

struct SwapchainContext
{
    std::uint64_t surfaceGeneration = 0u;
    ANativeWindow* window = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
    // Heap-stable because scene cache leases retain a pointer while this
    // context is moved from its setup scope to the render-owner global.
    std::unique_ptr<AndroidCompiledPipelineCacheOwner> compiledPipelineCache;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamilyIndex = 0u;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkColorSpaceKHR swapchainColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D swapchainExtent{};
    VkExtent2D nativeWindowExtent{};
    VkExtent2D surfaceCurrentExtent{};
    VkSurfaceTransformFlagsKHR surfaceSupportedTransforms = 0u;
    VkSurfaceTransformFlagBitsKHR surfaceCurrentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    VkSurfaceTransformFlagBitsKHR swapchainPreTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    bool rtPreRotationRequired = false;
    horde::graphics::RtPresentationTransform rtPresentationTransform =
        horde::graphics::RtPresentationTransform::Identity;
    float renderScale = kDefaultAndroidRtRenderScale;
    horde::graphics::GraphicsSettings graphicsSettings = horde::graphics::PlatformDefaultGraphicsSettings(horde::graphics::GraphicsPlatform::Android);
    horde::graphics::GraphicsSettings graphicsRequested{};
    std::uint64_t graphicsSerial = 0u;
    GraphicsCommandLatency graphicsLatency;
    horde::vulkan::raytracing::RtSceneProfile sceneProfile = horde::vulkan::raytracing::RtSceneProfile::Showcase;
    bool previewTransitionFailed = false;
    horde::graphics::GraphicsPreviewSession previewSession;
    horde::graphics::EntryMenuSession entrySession;
    horde::graphics::EntryMenuHandoff entryHandoff;
    std::uint32_t entryWarmFrames = kMaxFramesInFlight;
    horde::graphics::GraphicsPreviewPerformance previewPerformance;
    std::chrono::steady_clock::time_point previewLastPublish{};
    std::uint64_t previewEpoch = 0u, previewResetSerial = 0u, previewGpuSamples = 0u;
    std::uint32_t previewWarmFrames = 0u;
    horde::graphics::GraphicsReason graphicsReason = horde::graphics::GraphicsReason::None;
    float frameDeltaSeconds = 1.0f / 60.0f;
    std::uint64_t lastInputOwnerSteadyNs = 0u;
#if HORDE_RT_ANDROID_MOTION_EVIDENCE
    std::unique_ptr<AndroidMotionRun> motion;
#endif
    bool motionValidationRun = false;
#if !defined(NDEBUG)
    horde::telemetry::CombatTimingTrace combatTimingTrace;
    horde::telemetry::InputPresentationTrace inputPresentationTrace;
#endif
    uint32_t timingFrameCount = 0u;
    double timingFenceMs = 0.0;
    double timingRecordMs = 0.0;
    double timingPresentMs = 0.0;
    double timingTotalMs = 0.0;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageLayout> swapchainImageLayouts;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> swapchainFramebuffers;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    VkSemaphore imageAvailableSemaphores[kMaxFramesInFlight] = {};
    // Present waits belong to swapchain images, not graphics fence slots.
    std::vector<VkSemaphore> renderFinishedSemaphores;
    horde::vulkan::PresentSurfaceSupport presentSurfaceSupport{};
    horde::vulkan::PresentCompletionMode presentCompletionMode = horde::vulkan::PresentCompletionMode::Unextended;
    horde::vulkan::PresentCompletionFences presentCompletionFences;
    bool presentTimingExtensionEnabled = false;
#if HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION
    std::unique_ptr<horde::vulkan::PresentTimingEvidence> presentTiming;
    std::uint64_t presentTimingPollCpuNs = 0u;
#endif
    bool imageAcquirePending = false;
    VkFence inFlightFences[kMaxFramesInFlight] = {};
    VkClearColorValue clearColor = {{0.12f, 0.04f, 0.18f, 1.0f}};
    horde::vulkan::DeviceCapabilities capabilities;
    horde::vulkan::raytracing::PresentableTinyRtScene rtScene;
    horde::vulkan::GpuFrameTimer gpuFrameTimer;
#if HORDE_RT_STAGED_PRIMARY_TIMING
    horde::vulkan::raytracing::experimental::StagedPrimaryTiming stagedPassTimer;
    horde::vulkan::raytracing::experimental::StagedPrimaryProfile stagedPassProfile;
#endif
    horde::vulkan::raytracing::RtFrameEvidenceCoordinator rtFrameEvidence;
    bool gpuFrameTimingEnabled = true;
    double gpuFrameTimingTotalMs = 0.0;
    std::uint64_t gpuFrameTimingSampleCount = 0u;
    bool rtFrameEvidenceInitialised = false;
    float outputExposure = 0.92f;
    std::int32_t activeBenchmarkCheckpoint = -1;
    std::string activeBenchmarkName;
    horde::vulkan::raytracing::PlayerRenderRoute playerRenderRoute =
        kDefaultPlayerPresentationRoute;
    bool glassFixtureRequested = false;
    bool productionRewardPropsRequested = false;
    bool productionLanternGlassOnly = false;
    float glassDepthScale = 1.0f;
    std::array<float, 3u> glassAttenuationColor{{0.72f, 0.90f, 1.0f}};
    float glassAttenuationDistance = 2.4f;
    std::uint32_t benchmarkGeneration = 0u;
    std::uint32_t benchmarkWarmupFrames = 0u;
    std::uint32_t benchmarkSampleFrames = 0u;
    std::uint32_t benchmarkWindow = 0u;
    double benchmarkFenceMs = 0.0;
    double benchmarkRecordMs = 0.0;
    double benchmarkPresentMs = 0.0;
    double benchmarkTotalMs = 0.0;
    bool benchmarkSampling = false;
    bool captureActive = false;
    std::uint32_t capturePresentedFrames = 0u;
    horde::gameplay::ShowcaseRouteReplay routeReplay;
    bool routeReplayActive = false;
    horde::gameplay::ShowcaseBenchmarkRun inAppBenchmark;
    std::string benchmarkRunId;
    std::string benchmarkSummaryRunId;
    std::string benchmarkSummaryRawModel;
    std::uint64_t benchmarkSummaryRevision = 0u;
    std::optional<horde::telemetry::BenchmarkSummaryConfiguration> benchmarkSummaryStart;
    horde::telemetry::RtBenchmarkEvidenceRun benchmarkEvidence;
    std::optional<std::size_t> benchmarkExpectedFrame;
    std::string reportDirectory;
    bool useRtPath = false;
    horde::vulkan::RtExecutionBackend executionBackend = horde::vulkan::RtExecutionBackend::Unsupported;
    uint32_t currentFrame = 0u;
};

SwapchainContext gSwapchainContext{};
#if HORDE_RT_ANDROID_MOTION_VALIDATION
void FailAndroidMotion(SwapchainContext& context, std::string_view reason);
#endif
// CPU bytes only. NativeSurfaceOwner joins the retiring render owner before
// starting the next generation, so cache read/replacement is serialized.
// No Vulkan object or background rendering survives normal surface teardown.
horde::vulkan::PipelineCacheSeed gPipelineCacheSeed;
// Failed retirement retains exactly one context; a later lifecycle action may
// retry its proof, but can never overwrite it with another device/swapchain.
std::atomic<bool> gSurfaceRetirementBlocked{false};
std::atomic<bool> gSwapchainRunning{false};
std::thread gSwapchainThread;
struct NativeWindowRelease
{
    void operator()(ANativeWindow* window) const { if (window) ANativeWindow_release(window); }
};
struct SurfaceStartRequest
{
    std::unique_ptr<ANativeWindow, NativeWindowRelease> window;
    std::string reportDirectory;
};
horde::platform::android::SurfaceSessionMailbox<SurfaceStartRequest> gSurfaceSessions;
std::mutex gReportMutex;
std::string gLatestTextReport;
std::string gLatestJsonReport;
std::uint64_t gLatestReportSurfaceGeneration = 0u; // guarded by gReportMutex; zero is probe-only.
horde::reporting::OwnedPlaytestReportContext gLatestPlaytestContext;
// One consented foreground capture; only the render owner accesses rtScene.
// JNI transfers owned pixels/context, never a live renderer pointer. Cancellation
// invalidates the generation; it cannot undo a GPU readback already started.
struct PlaytestCaptureRequest
{
    std::uint64_t token = 0u;
    enum class State { Empty, Pending, Ready, Transferred, Failed } state = State::Empty;
    horde::reporting::PlaytestScreenshotPixels pixels;
    horde::reporting::OwnedPlaytestReportContext context;
    std::string capturedAtUtc;
    std::uint32_t width = 0u, height = 0u;
};
PlaytestCaptureRequest gPlaytestCapture;
std::uint64_t gPlaytestCaptureNextToken = 0u; // guarded by gReportMutex
std::string gLatestDeveloperOverlayText;
std::string gLatestBenchmarkReport;
std::string gLatestBenchmarkProgress;
// Request identity shares the report mutex; it is not a frame/submission counter.
std::string gRequestedBenchmarkRunId;
std::string gRequestedBenchmarkSummaryRunId;
std::string gRequestedBenchmarkSummaryRawModel;
horde::telemetry::FrozenBenchmarkSummary gLatestBenchmarkSummary;
std::uint64_t gBenchmarkSummaryRevision = 0u; // guarded by gReportMutex
void InvalidateBenchmarkSummaryLocked()
{
    gLatestBenchmarkSummary = {};
    gRequestedBenchmarkSummaryRunId.clear();
    gRequestedBenchmarkSummaryRawModel.clear();
    if (gBenchmarkSummaryRevision != std::numeric_limits<std::uint64_t>::max())
        ++gBenchmarkSummaryRevision;
}
horde::gameplay::BenchmarkWorkload gRequestedBenchmarkWorkload =
    horde::gameplay::BenchmarkWorkload::ShowcaseRoute;
horde::gameplay::simulation::GameSimulation gGameSimulation(
    horde::gameplay::simulation::ProductionGameSimulationConfig());
horde::gameplay::simulation::InputMailbox gInputMailbox;
std::mutex gInputPublisherMutex;
horde::gameplay::simulation::InputSnapshot gInputPublisherState = []
{
    horde::gameplay::simulation::InputSnapshot input;
    input.torchLightStrength = 1.8f;
    input.paused = true;
    return input;
}();
// JNI publishes pause requests, but only the render/gameplay owner may
// acknowledge their command counters. An unpause remains deferred until the
// latest coherent paused publication has been synchronized by that owner.
horde::gameplay::simulation::InputSnapshot gLifecyclePausedInput =
    gInputPublisherState;
std::uint64_t gLifecyclePausedPublicationSequence = 0u;
std::uint64_t gLifecyclePauseGeneration = 0u;
std::uint64_t gLifecyclePauseAcknowledgedGeneration = 0u;
horde::gameplay::simulation::PausedInputPolicy gLifecyclePausedPolicy =
    horde::gameplay::simulation::PausedInputPolicy::DiscardAllCommands;
// Keep a real stop/start's discard floor even if a later menu request replaces
// the latest pause publication before the render owner observes either one.
horde::gameplay::simulation::InputSnapshot gLifecycleDiscardInput = gInputPublisherState;
std::uint64_t gLifecycleDiscardPublicationSequence = 0u;
std::uint64_t gLifecycleDiscardGeneration = 0u;
std::uint64_t gLifecycleDiscardAcknowledgedGeneration = 0u;
bool gLifecycleUnpausePending = false;
bool gLifecycleMeasurementPaused = true;
horde::telemetry::RtLifecycleSeeds gPreservedRtEvidenceSeeds = [] {
    horde::telemetry::RtLifecycleSeeds seeds{};
    seeds.sceneEpoch = 1u;
    seeds.measurementGeneration = 1u;
    return seeds;
}();
std::mutex gRtEvidenceSeedMutex;
std::mutex gGraphicsMutex;
horde::graphics::GraphicsCommand gRequestedGraphics{0u, 0u, horde::graphics::GraphicsCommandKind::Apply,
    horde::graphics::PlatformDefaultGraphicsSettings(horde::graphics::GraphicsPlatform::Android)};
horde::graphics::GraphicsAppliedSnapshot gAppliedGraphics{};
std::optional<horde::graphics::GraphicsEditSession> gGraphicsEdit;
std::uint64_t gGraphicsSerial = 0u;
GraphicsCommandLatency gPublishedGraphicsLatency; // guarded by gGraphicsMutex.
struct PreviewControls {
    bool enabled = false, paused = false, motion = false;
    int camera = 0;
    std::uint64_t resetSerial = 0u, generation = 0u;
};
PreviewControls gPreviewControls{};
horde::graphics::EntryMenuControls gEntryControls{};
std::array<jlong, 6u> gMenuAmbienceState{}; // Generation/reset/tick/creak/fade/presented.
std::array<jlong, 5u> gEntryState{}; // Coherent generation/profile/phase/fade/presentation.
horde::graphics::GraphicsPreviewPerformanceSnapshot gPreviewPerformance{};
std::uint64_t gPreviewPerformanceGeneration = 0u; // guarded by gGraphicsMutex.
PreviewControls ReadPreviewControls() { std::lock_guard lock(gGraphicsMutex); return gPreviewControls; }
horde::graphics::EntryMenuControls ReadEntryControls()
{
    std::lock_guard lock(gGraphicsMutex);
    return gEntryControls;
}

horde::graphics::GraphicsScene GraphicsSceneForProfile(
    horde::vulkan::raytracing::RtSceneProfile profile)
{
    using horde::vulkan::raytracing::RtSceneProfile;
    return profile == RtSceneProfile::GraphicsPreview ? horde::graphics::GraphicsScene::Preview :
        profile == RtSceneProfile::EntryMenu ? horde::graphics::GraphicsScene::EntryMenu :
        horde::graphics::GraphicsScene::Showcase;
}

horde::vulkan::raytracing::RtSceneProfile DesiredProfile(
    const SwapchainContext& context, const PreviewControls& preview)
{
    const bool currentPreview = (preview.generation == 0u ||
        preview.generation == context.surfaceGeneration) && preview.enabled;
    switch (context.entryHandoff.DesiredScene(currentPreview))
    {
    case horde::graphics::GraphicsScene::Preview:
        return horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview;
    case horde::graphics::GraphicsScene::EntryMenu:
        return horde::vulkan::raytracing::RtSceneProfile::EntryMenu;
    default: return horde::vulkan::raytracing::RtSceneProfile::Showcase;
    }
}

void PublishEntryState(const SwapchainContext& context, bool presented)
{
    if (!gSurfaceSessions.IsCurrent(context.surfaceGeneration)) return;
    std::lock_guard lock(gGraphicsMutex);
    if (!gSurfaceSessions.IsCurrent(context.surfaceGeneration)) return;
    const bool currentRequest = horde::graphics::CurrentEntryMenuControls(
        gEntryControls, context.surfaceGeneration) &&
        gEntryControls.resetSerial == context.entryHandoff.ResetSerial();
    const int phase = context.entryHandoff.Phase();
    const float fade = context.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::EntryMenu ?
        context.entrySession.Snapshot().fade : phase != 0 ? 1.0f : 0.0f;
    const std::array<jlong, 5u> nextState{{static_cast<jlong>(context.surfaceGeneration),
        static_cast<jlong>(GraphicsSceneForProfile(context.sceneProfile)), phase,
        static_cast<jlong>(std::lround(std::clamp(fade, 0.0f, 1.0f) * 1000.0f)),
        presented && currentRequest && context.sceneProfile == DesiredProfile(context, gPreviewControls) ? 1 : 0}};
    if (gEntryState[0] != nextState[0] || gEntryState[1] != nextState[1] ||
        gEntryState[2] != nextState[2] || gEntryState[4] != nextState[4])
        __android_log_print(ANDROID_LOG_INFO, kTag,
            "HORDE_ENTRY_STATE generation=%lld profile=%lld phase=%lld fade_permille=%lld presented=%lld",
            static_cast<long long>(nextState[0]), static_cast<long long>(nextState[1]),
            static_cast<long long>(nextState[2]), static_cast<long long>(nextState[3]),
            static_cast<long long>(nextState[4]));
    gEntryState = nextState;
    const auto snapshot = context.entrySession.Snapshot();
    gMenuAmbienceState = {{nextState[0], static_cast<jlong>(context.entryHandoff.ResetSerial()),
        static_cast<jlong>(snapshot.tick), static_cast<jlong>(snapshot.chainCreakSerial), nextState[3],
        currentRequest && gEntryControls.enabled && nextState[1] == 2 && nextState[4] == 1 && phase != 4 ? 1 : 0}};
}

horde::graphics::GraphicsCommand ReadRequestedGraphics()
{
    std::lock_guard lock(gGraphicsMutex);
    return gRequestedGraphics;
}

std::pair<horde::graphics::GraphicsCommand, GraphicsCommandLatency> ReadGraphicsCommandObservation()
{
    std::lock_guard lock(gGraphicsMutex);
    return {gRequestedGraphics, gPublishedGraphicsLatency};
}

void LogGraphicsCommandLatency(SwapchainContext& context, bool success, bool presented, bool exactAck)
{
    auto& timing = context.graphicsLatency;
    if (!timing.active || timing.serial != context.graphicsSerial) return;
    const auto completed = GraphicsSteadyNs();
    __android_log_print(ANDROID_LOG_INFO, kTag,
        "HORDE_GRAPHICS_LATENCY serial=%llu generation=%llu profile=%d backend=%d glass_changed=%d "
        "success=%d presented=%d exact_ack=%d receipt_ns=%llu publication_ns=%llu owner_ns=%llu "
        "first_render_ns=%llu first_presented_ns=%llu completion_ns=%llu jni_to_publish_ms=%.3f owner_wait_ms=%.3f "
        "idle_ms=%.3f destroy_ms=%.3f init_ms=%.3f render_to_completion_ms=%.3f total_cpu_wall_ms=%.3f",
        static_cast<unsigned long long>(timing.serial), static_cast<unsigned long long>(context.surfaceGeneration),
        static_cast<int>(context.sceneProfile), static_cast<int>(context.executionBackend), timing.glassChanged ? 1 : 0,
        success ? 1 : 0, presented ? 1 : 0, exactAck ? 1 : 0,
        static_cast<unsigned long long>(timing.receivedNs), static_cast<unsigned long long>(timing.publishedNs),
        static_cast<unsigned long long>(timing.observedNs), static_cast<unsigned long long>(timing.firstRenderNs),
        static_cast<unsigned long long>(timing.firstPresentedNs),
        static_cast<unsigned long long>(completed), GraphicsElapsedMs(timing.receivedNs, timing.publishedNs),
        GraphicsElapsedMs(timing.publishedNs, timing.observedNs), timing.idleMilliseconds, timing.destroyMilliseconds,
        timing.initialiseMilliseconds, GraphicsElapsedMs(timing.firstRenderNs, completed),
        GraphicsElapsedMs(timing.receivedNs, completed));
    timing.active = false; // One outcome per owner-observed serial, never one line per frame.
}

void PublishGraphicsApplied(SwapchainContext& context, const bool success, const bool presented)
{
    if (!gSurfaceSessions.IsCurrent(context.surfaceGeneration)) return;
    horde::graphics::GraphicsAppliedSnapshot snapshot;
    snapshot.serial = context.graphicsSerial;
    snapshot.lifecycleGeneration = context.surfaceGeneration;
    snapshot.requested = context.graphicsRequested;
    snapshot.effective = context.graphicsSettings;
    snapshot.effective.glassEnabled = context.rtScene.GlassEnabled();
    const auto completedMist = horde::telemetry::CurrentCompletedMistEnabled(context.rtFrameEvidence.PublishedStateByValue());
    const auto completedDust = horde::telemetry::CurrentCompletedDustQuality(context.rtFrameEvidence.PublishedStateByValue());
    const auto uploadedMist = context.rtScene.UploadedMistEnabled();
    const auto uploadedDust = context.rtScene.UploadedDustQuality();
    snapshot.effective.mistEnabled = completedMist.value_or(true);
    snapshot.effective.dustQuality = uploadedDust.value_or(static_cast<horde::graphics::DustQuality>(255u));
    const bool uploadedQuality = context.rtScene.HasUploadedQualityControls();
    if (uploadedQuality)
    {
        snapshot.effective.shadowQuality = static_cast<horde::graphics::ShadowQuality>(
            context.rtScene.QualityControls().controls[0]);
        switch (context.rtScene.UploadedFireQuality())
        {
        case horde::vulkan::raytracing::FireEmitterQuality::Mobile: snapshot.effective.fireDetail = horde::graphics::FireDetail::Mobile; break;
        case horde::vulkan::raytracing::FireEmitterQuality::High: snapshot.effective.fireDetail = horde::graphics::FireDetail::High; break;
        case horde::vulkan::raytracing::FireEmitterQuality::Low: snapshot.effective.fireDetail = horde::graphics::FireDetail::Low; break;
        }
    }
    snapshot.opticalProfile = context.rtScene.SelectedDielectricQualityName() == "High" ?
        horde::graphics::OpticalProfile::High : horde::graphics::OpticalProfile::Mobile;
    snapshot.backend = context.rtScene.ExecutionBackend() == horde::vulkan::RtExecutionBackend::RayTracingPipeline ?
        horde::graphics::GraphicsBackend::RayTracingPipeline :
        context.rtScene.ExecutionBackend() == horde::vulkan::RtExecutionBackend::RayQueryCompute ?
        horde::graphics::GraphicsBackend::RayQueryCompute : horde::graphics::GraphicsBackend::Unsupported;
    const auto extent = context.rtScene.DispatchExtent();
    snapshot.internalExtent = {extent.width, extent.height};
    snapshot.outputExtent = {context.swapchainExtent.width, context.swapchainExtent.height};
    snapshot.scene = GraphicsSceneForProfile(context.sceneProfile);
    snapshot.reasons = context.graphicsReason | (context.previewTransitionFailed ?
        horde::graphics::GraphicsReason::ResourceFailure : horde::graphics::GraphicsReason::None);
    snapshot.rtPresented = presented && uploadedQuality && completedMist && uploadedMist &&
        completedMist == uploadedMist && *completedMist == context.graphicsSettings.mistEnabled &&
        completedDust && uploadedDust && completedDust == uploadedDust &&
        *completedDust == context.graphicsSettings.dustQuality;
    std::lock_guard lock(gGraphicsMutex);
    if (!gSurfaceSessions.IsCurrent(context.surfaceGeneration)) return;
    if ((gPreviewControls.generation == 0u || gPreviewControls.generation == snapshot.lifecycleGeneration) &&
        gPreviewControls.enabled != (snapshot.scene == horde::graphics::GraphicsScene::Preview))
        snapshot.rtPresented = false; // A previous profile cannot acknowledge a pending scene switch.
    if (context.sceneProfile != DesiredProfile(context, gPreviewControls) ||
        (horde::graphics::CurrentEntryMenuControls(gEntryControls, context.surfaceGeneration) &&
         gEntryControls.resetSerial != context.entryHandoff.ResetSerial()))
        snapshot.rtPresented = false;
    gAppliedGraphics = snapshot;
    bool acknowledged = false;
    if (gGraphicsEdit && (gGraphicsEdit->State() == horde::graphics::GraphicsEditState::Applying ||
                         gGraphicsEdit->State() == horde::graphics::GraphicsEditState::Reverting))
        acknowledged = gGraphicsEdit->Acknowledge(snapshot, success);
    if (!success || snapshot.rtPresented)
        LogGraphicsCommandLatency(context, success, snapshot.rtPresented, success && acknowledged);
}
std::atomic<bool> gRequestedGpuFrameTimingEnabled{true};
std::atomic<bool> gRequiredRayQueryCompute{false};
// The sole Android-owned RT tuning state. The render thread takes one coherent
// copy per frame while JNI setters publish complete, clamped updates.
horde::platform::android::AndroidRtLabState gRtLabState;
std::atomic<bool> gRtLabDebugAutomationSession{false};
std::atomic<bool> gRtLabBenchmarkRoute{false};
std::atomic<bool> gRtLabUnlockEligible{false};
std::atomic<float> gRtLabGpuFrameTimeMs{0.0f};
std::atomic<std::uint64_t> gRtLabGpuSampleCount{0u};
std::atomic<std::int32_t> gBenchmarkCheckpointRequested{-1};
std::atomic<std::int32_t> gCaptureCheckpointRequested{-1};
std::atomic<bool> gRouteReplayRequested{false};
std::atomic<bool> gInAppBenchmarkRequested{false};
std::atomic<bool> gInAppBenchmarkCancelRequested{false};
std::atomic<int> gInAppBenchmarkStatus{0}; // 0 idle, 1 running, 2 complete, 3 failed/cancelled.
std::atomic<std::uint64_t> gPlayerVitalityState{
    (static_cast<std::uint64_t>(horde::gameplay::PlayerVitals::kMaxVitality) << 32u) |
    static_cast<std::uint32_t>(horde::gameplay::PlayerVitals::kMaxVitality)};
std::atomic<int> gPlayerLifePhase{static_cast<int>(horde::gameplay::PlayerLifePhase::Alive)};
std::atomic<std::int32_t> gPlayerRetryCheckpoint{0};
std::atomic<float> gKeeperTitleOpacity{0.0f};
std::atomic<int> gFinaleEndingPhase{static_cast<int>(horde::gameplay::FinaleEndingPhase::Inactive)};
// Bit 0: INTERACT, bit 1: RAISE, bit 2: LOWER. Bits 3..5 carry the shared
// ChestRewardPrompt value. The render thread owns gameplay and publishes this
// compact contextual UI view to the Java thread; Java only chooses labels.
std::atomic<int> gContextualControlState{0};
std::atomic<std::uint64_t> gWaterfallStereoGains{0u};

struct PlatformGameplayEvent
{
    std::uint64_t metadata = 0u;
    std::uint64_t stereoGains = 0u;
};

constexpr std::size_t kPlatformGameplayEventCapacity = 128u;
horde::gameplay::simulation::BoundedTransportQueue<
    PlatformGameplayEvent, kPlatformGameplayEventCapacity> gPlatformGameplayEvents;
std::mutex gPlatformGameplayEventMutex;

uint64_t PackStereoGains(float left, float right)
{
    return static_cast<uint64_t>(std::bit_cast<uint32_t>(left)) |
           (static_cast<uint64_t>(std::bit_cast<uint32_t>(right)) << 32u);
}

std::uint64_t PublishInputLocked()
{
    return gInputMailbox.Publish(gInputPublisherState);
}

void RequestLifecyclePauseSynchronizationLocked(const bool resumeAfterAcknowledgement,
    const horde::gameplay::simulation::PausedInputPolicy policy =
        horde::gameplay::simulation::PausedInputPolicy::DiscardAllCommands)
{
    gInputPublisherState.paused = true;
    gLifecyclePausedPublicationSequence = PublishInputLocked();
    gLifecyclePausedInput = gInputPublisherState;
    ++gLifecyclePauseGeneration;
    gLifecyclePausedPolicy = policy;
    if (policy == horde::gameplay::simulation::PausedInputPolicy::DiscardAllCommands)
    {
        gLifecycleDiscardInput = gLifecyclePausedInput;
        gLifecycleDiscardPublicationSequence = gLifecyclePausedPublicationSequence;
        gLifecycleDiscardGeneration = gLifecyclePauseGeneration;
    }
    gLifecycleUnpausePending = resumeAfterAcknowledgement;
    gLifecycleMeasurementPaused = true;
}

// Caller holds the publication mutex. Synchronization never advances gameplay
// time; this lets final world-command admission linearize against accepted Stop.
bool SynchronizeLifecyclePauseOnOwnerThreadLocked()
{
    if (gLifecyclePauseAcknowledgedGeneration >= gLifecyclePauseGeneration)
        return gLifecycleMeasurementPaused;
    const auto generation = gLifecyclePauseGeneration;
    const auto discardGeneration = gLifecycleDiscardGeneration;

    // GameSimulation is owned by this render thread. No JNI callback mutates
    // it directly, including during lifecycle teardown/recreation.
    if (gLifecycleDiscardAcknowledgedGeneration < discardGeneration && discardGeneration < generation)
        gGameSimulation.SynchronizePausedInput(gLifecycleDiscardInput, gLifecycleDiscardPublicationSequence);
    gGameSimulation.SynchronizePausedInput(gLifecyclePausedInput, gLifecyclePausedPublicationSequence,
        gLifecyclePausedPolicy);

    gLifecyclePauseAcknowledgedGeneration = generation;
    gLifecycleDiscardAcknowledgedGeneration = discardGeneration;
    if (gLifecycleUnpausePending)
    {
        gInputPublisherState.paused = false;
        PublishInputLocked();
        gLifecycleUnpausePending = false;
        gLifecycleMeasurementPaused = false;
    }
    return gLifecycleMeasurementPaused;
}

bool SynchronizeLifecyclePauseOnOwnerThread()
{
    std::lock_guard<std::mutex> lock(gInputPublisherMutex);
    return SynchronizeLifecyclePauseOnOwnerThreadLocked();
}

horde::telemetry::RtLifecycleSeeds LoadPreservedRtEvidenceSeeds()
{
    std::lock_guard<std::mutex> lock(gRtEvidenceSeedMutex);
    return gPreservedRtEvidenceSeeds;
}

void PreserveRtEvidenceSeeds(const horde::telemetry::RtLifecycleSeeds& seeds)
{
    std::lock_guard<std::mutex> lock(gRtEvidenceSeedMutex);
    gPreservedRtEvidenceSeeds.sceneEpoch = std::max(
        gPreservedRtEvidenceSeeds.sceneEpoch, seeds.sceneEpoch);
    gPreservedRtEvidenceSeeds.measurementGeneration = std::max(
        gPreservedRtEvidenceSeeds.measurementGeneration, seeds.measurementGeneration);
    gPreservedRtEvidenceSeeds.recordAttemptSerial = std::max(
        gPreservedRtEvidenceSeeds.recordAttemptSerial, seeds.recordAttemptSerial);
    gPreservedRtEvidenceSeeds.successfulRecordSerial = std::max(
        gPreservedRtEvidenceSeeds.successfulRecordSerial, seeds.successfulRecordSerial);
    gPreservedRtEvidenceSeeds.submissionSerial = std::max(
        gPreservedRtEvidenceSeeds.submissionSerial, seeds.submissionSerial);
    gPreservedRtEvidenceSeeds.completionSerial = std::max(
        gPreservedRtEvidenceSeeds.completionSerial, seeds.completionSerial);
}

void ClearPlatformGameplayEvents()
{
    std::lock_guard<std::mutex> lock(gPlatformGameplayEventMutex);
    gPlatformGameplayEvents.Clear();
}

std::uint64_t PlatformGameplayEventOverflowCount()
{
    std::lock_guard<std::mutex> lock(gPlatformGameplayEventMutex);
    return gPlatformGameplayEvents.OverflowCount();
}

void EnqueuePlatformGameplayEvent(const horde::gameplay::simulation::GameplayEvent& event)
{
    const horde::gameplay::SpatialAudioGains gains = horde::gameplay::CalculateSpatialAudio(
        {event.worldX, event.worldZ, std::max(0.0f, event.intensity), 1.0f, 14.0f},
        {event.listenerX, event.listenerZ, event.listenerYawRadians});
    const std::uint64_t metadata = horde::platform::android::PackGameplayEventMetadata(event);

    std::lock_guard<std::mutex> lock(gPlatformGameplayEventMutex);
    const bool enqueued = gPlatformGameplayEvents.Push(
        {metadata, PackStereoGains(gains.left, gains.right)});
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
    if (event.type == horde::gameplay::simulation::GameplayEventType::PlayerSwing)
    {
        __android_log_print(
            ANDROID_LOG_INFO, kTag,
            "HORDE_PLAYER_SWING_TRANSPORT phase=enqueued sequence=%llu cut=%d accepted=%d",
            static_cast<unsigned long long>(event.sequence), event.payload,
            enqueued ? 1 : 0);
    }
#endif
}

void DrainSimulationEventsToPlatform()
{
    for (const horde::gameplay::simulation::GameplayEvent& event : gGameSimulation.Events().Events())
    {
        EnqueuePlatformGameplayEvent(event);
    }
    gGameSimulation.ClearEvents();
}


void PublishSimulationUiState()
{
    const horde::gameplay::simulation::SimulationSnapshot& simulation = gGameSimulation.Snapshot();
    const horde::gameplay::PlayerVitalsSnapshot& vitals = simulation.playerVitals;
    gKeeperTitleOpacity.store(std::clamp(simulation.lich.titleOpacity, 0.0f, 1.0f), std::memory_order_release);
    gPlayerVitalityState.store(
        (static_cast<std::uint64_t>(static_cast<std::uint32_t>(vitals.maxVitality)) << 32u) |
        static_cast<std::uint32_t>(vitals.vitality), std::memory_order_release);
    gPlayerLifePhase.store(static_cast<int>(vitals.phase), std::memory_order_release);
    gPlayerRetryCheckpoint.store(simulation.retryCheckpoint, std::memory_order_release);
    gFinaleEndingPhase.store(
        static_cast<int>(simulation.finale.endingPhase),
        std::memory_order_release);
    int contextualControls = 0;
    using horde::gameplay::interactions::ChestRewardPrompt;
    using horde::gameplay::interactions::HeldLightKind;
    using horde::gameplay::interactions::HeldLightPose;
    contextualControls |= static_cast<int>(simulation.chestPrompt) << 3;
    if (simulation.chestPrompt == ChestRewardPrompt::OpenChest ||
        simulation.chestPrompt == ChestRewardPrompt::ClaimLantern)
    {
        contextualControls |= 1;
    }
    if (simulation.interaction.heldLightKind == HeldLightKind::RewardLantern)
    {
        if (simulation.interaction.heldLightPose == HeldLightPose::Low)
        {
            contextualControls |= 2;
        }
        else if (simulation.interaction.heldLightPose == HeldLightPose::High)
        {
            contextualControls |= 4;
        }
    }
    gContextualControlState.store(contextualControls, std::memory_order_release);
    const horde::gameplay::SpatialAudioGains waterfallGains =
        horde::gameplay::CalculateSpatialAudio(
            {-2.32f, -15.26f, 0.52f, 0.65f, 10.0f},
            {simulation.playerX, simulation.playerZ, simulation.playerYawRadians});
    gWaterfallStereoGains.store(
        PackStereoGains(waterfallGains.left, waterfallGains.right),
        std::memory_order_release);
}

std::string BuildDisplayText(const horde::vulkan::DeviceCapabilities& capabilities,
    const horde::telemetry::RtLifecyclePublishedState* evidence = nullptr, bool observerAvailable = true)
{
    if (capabilities.rtMode == horde::vulkan::RtMode::Unsupported)
    {
        return horde::ui::BuildUnsupportedDeviceText(capabilities);
    }
    return horde::vulkan::BuildCapabilityTextReport(capabilities, evidence, observerAvailable);
}

#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
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

horde::ui::DeveloperOverlaySnapshot BuildDeveloperOverlaySnapshot(const SwapchainContext& context)
{
    const horde::gameplay::simulation::SimulationSnapshot& simulation = gGameSimulation.Snapshot();
    const horde::gameplay::EnemyRosterSnapshot& roster = simulation.enemyRoster;
    const horde::gameplay::LichSnapshot& lich = simulation.lich;
    const horde::gameplay::EnemyEncounterSnapshot* encounter = SelectedEncounter(roster);
    horde::ui::DeveloperOverlaySnapshot snapshot;
    snapshot.buildIdentity = std::string(HORDE_RT_BUILD_ID) + " DEBUG";
    snapshot.shaderIdentity = context.rtScene.SelectedPipelineBundleDisplayIdentity();
    const horde::gameplay::PlayerVitalsSnapshot& playerVitals = simulation.playerVitals;
    snapshot.playerLifePhase = horde::gameplay::PlayerLifePhaseName(playerVitals.phase);
    snapshot.playerVitality = playerVitals.vitality;
    snapshot.playerMaxVitality = playerVitals.maxVitality;
    snapshot.playerDamageEnabled =
        playerVitals.phase == horde::gameplay::PlayerLifePhase::Alive &&
        !simulation.paused &&
        !context.inAppBenchmark.IsRunning() &&
        !context.routeReplayActive && !context.benchmarkSampling && !context.captureActive;

    snapshot.gpuName = context.capabilities.identity.gpuName;
    snapshot.vulkanApi = PackedVulkanVersion(context.capabilities.identity.vulkanApiVersion);
    snapshot.rtMode = horde::vulkan::ToString(context.rtScene.ExecutionBackend());
    snapshot.routeZone = horde::gameplay::ShowcaseZoneName(
        simulation.zone);
    snapshot.materialEncoding = context.rtScene.MaterialEncoding();
    snapshot.torchFailurePhase = horde::gameplay::TorchFailurePhaseName(simulation.torchFailure.phase);
    snapshot.selectedEnemy = horde::gameplay::EnemyKindName(roster.selectedEnemy);
    snapshot.encounterPhase = encounter ? EncounterStatusName(encounter->status) : "inactive";
    if (roster.selectedEnemy == horde::gameplay::EnemyKind::Lich)
    {
        snapshot.encounterPhase = horde::gameplay::LichPhaseName(lich.phase);
        snapshot.enemyHealth = lich.health;
    }
    snapshot.internalWidth = context.capabilities.performance.internalRenderWidth;
    snapshot.internalHeight = context.capabilities.performance.internalRenderHeight;
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
    snapshot.fps = context.capabilities.performance.fps;
    snapshot.frameTimeMs = context.capabilities.performance.frameTimeMs;
    snapshot.gpuRtTimingValid = context.capabilities.performance.gpuRt.valid;
    snapshot.gpuRtLatestMs = context.capabilities.performance.gpuRt.latestMs;
    snapshot.gpuRtAverageMs = context.capabilities.performance.gpuRt.averageMs;
    snapshot.gpuRtSampleCount = context.capabilities.performance.gpuRt.sampleCount;
    snapshot.gpuRtTimingStatus = context.capabilities.performance.gpuRt.status;
    snapshot.presented = context.capabilities.rtScene.presented;
    snapshot.simulationTicksThisFrame = simulation.simulationTicksThisFrame;
    snapshot.fixedStepAccumulatorSeconds = simulation.fixedStepAccumulatorSeconds;
    snapshot.catchUpOverrunCount = simulation.catchUpOverrunCount;
    snapshot.queuedEventCount = simulation.queuedEventCount;
    snapshot.eventQueueHighWaterMark = simulation.eventQueueHighWaterMark;
    snapshot.eventQueueOverflowCount =
        simulation.eventQueueOverflowCount + PlatformGameplayEventOverflowCount();
    snapshot.inputPublicationSequence = simulation.inputPublicationSequence;
    snapshot.consumedAttackSequence = simulation.lastConsumedAttackSequence;
    snapshot.consumedParrySequence = simulation.lastConsumedParrySequence;
    snapshot.consumedRouteResetSequence = simulation.lastConsumedRouteResetSequence;
    snapshot.consumedRetrySequence = simulation.lastConsumedRetrySequence;
    return snapshot;
}

void PublishDeveloperOverlaySnapshot(const SwapchainContext& context)
{
    const std::string text = horde::ui::BuildDeveloperOverlayText(BuildDeveloperOverlaySnapshot(context));
    std::lock_guard<std::mutex> lock(gReportMutex);
    gLatestDeveloperOverlayText = text;
}
#endif

std::string LatestDeveloperOverlayText()
{
    std::lock_guard<std::mutex> lock(gReportMutex);
    return gLatestDeveloperOverlayText;
}

void PublishReportSnapshot(const horde::vulkan::DeviceCapabilities& capabilities)
{
    const std::string text = BuildDisplayText(capabilities);
    const std::string json = horde::vulkan::BuildCapabilityJsonReport(capabilities);
    std::lock_guard<std::mutex> lock(gReportMutex);
    gLatestTextReport = text;
    gLatestJsonReport = json;
    gLatestReportSurfaceGeneration = 0u;
    gLatestPlaytestContext = {}; // A probe is not a live renderer/context snapshot.
}

std::string LatestTextReport()
{
    std::lock_guard<std::mutex> lock(gReportMutex);
    if (gLatestReportSurfaceGeneration != 0u && !gSurfaceSessions.IsCurrent(gLatestReportSurfaceGeneration))
        return "RT surface stopped or starting. No current surface report is available.";
    return gLatestTextReport;
}

std::string LatestJsonReport()
{
    std::lock_guard<std::mutex> lock(gReportMutex);
    if (gLatestReportSurfaceGeneration != 0u && !gSurfaceSessions.IsCurrent(gLatestReportSurfaceGeneration))
        return R"({"rtScene":{"presented":false,"status":"Surface stopped or starting"}})";
    return gLatestJsonReport;
}

std::string LatestBenchmarkReport()
{
    std::lock_guard<std::mutex> lock(gReportMutex);
    return gLatestBenchmarkReport;
}

std::string LatestBenchmarkProgress()
{
    std::lock_guard<std::mutex> lock(gReportMutex);
    return gLatestBenchmarkProgress;
}

std::string BuildReportDirectory(const std::string& baseDirectory)
{
    return baseDirectory + '/' + kReportDirectory;
}

bool EnsureDirectoryExists(const std::string& path)
{
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
}

bool WriteTextFile(const std::string& path, const std::string& data)
{
    std::ofstream stream(path, std::ios::binary);
    if (!stream.good())
    {
        return false;
    }

    stream << data;
    return stream.good();
}

horde::reporting::OwnedPlaytestReportContext PlaytestContextOnRenderOwner(const SwapchainContext& context)
{
    return {"Horde Lantern RT", HORDE_RT_PACKAGE_VERSION,
        HORDE_RT_BUILD_ID, "Android", "", context.capabilities.identity.gpuName,
        horde::vulkan::ToString(context.capabilities.rtScene.executionBackend),
        std::string(context.rtScene.SelectedDielectricQualityName()),
        context.renderScale, context.capabilities.performance.internalRenderWidth,
        context.capabilities.performance.internalRenderHeight, context.capabilities.rtScene.presented};
}

#if HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION
void PollPresentTimingOnOwner(SwapchainContext& context)
{
    if (!context.presentTiming || !context.presentTiming->Bound()) return;
    const auto started = std::chrono::steady_clock::now();
    context.presentTiming->Poll();
    context.presentTimingPollCpuNs += static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - started).count());
}
std::string PresentTimingJson(const SwapchainContext& context)
{
    std::ostringstream stream;
    stream << "{\"extensionEnabled\":" << (context.presentTimingExtensionEnabled ? "true" : "false")
           << ",\"queryCpuWallNanoseconds\":" << context.presentTimingPollCpuNs << ",\"capture\":";
    if (context.presentTiming) context.presentTiming->WriteJson(stream);
    else stream << "{\"enabled\":false,\"status\":\"unsupported-or-allocation-failed\"}";
    stream << '}';
    return stream.str();
}
#endif

void PublishRuntimeReports(const SwapchainContext& context,
    const horde::telemetry::RtLifecyclePublishedState* finalPublication = nullptr)
{
    if (!gSurfaceSessions.IsCurrent(context.surfaceGeneration)) return;
    const auto publication = finalPublication ? *finalPublication
        : context.rtFrameEvidence.PublishedStateByValue();
    const auto* evidence = finalPublication ||
        (context.rtFrameEvidenceInitialised && context.rtFrameEvidence.ObserverAvailable())
        ? &publication : nullptr;
    const std::string text = BuildDisplayText(context.capabilities, evidence, finalPublication == nullptr);
    const std::string json = horde::vulkan::BuildCapabilityJsonReport(context.capabilities, evidence, finalPublication == nullptr);
    {
        std::lock_guard<std::mutex> lock(gReportMutex);
        if (!gSurfaceSessions.IsCurrent(context.surfaceGeneration)) return;
        gLatestTextReport = text;
        gLatestJsonReport = json;
        gLatestReportSurfaceGeneration = context.surfaceGeneration;
        // Publish the narrow allowlist from this render-thread-owned snapshot;
        // report preparation never reads the live scene or private diagnostic JSON.
        gLatestPlaytestContext = PlaytestContextOnRenderOwner(context);
    }
    WriteTextFile(context.reportDirectory + '/' + kTextReportFilename, text);
    WriteTextFile(context.reportDirectory + '/' + kJsonReportFilename, json);
}

void ResetShowcaseSimulation()
{
    const bool replacedQueue = gGameSimulation.Snapshot().playerMountProfile !=
        horde::gameplay::items::PlayerMountProfile::AnatomicalBody;
    if (replacedQueue)
        gGameSimulation = horde::gameplay::simulation::GameSimulation(
            horde::gameplay::simulation::ProductionGameSimulationConfig());
    else
        gGameSimulation.ResetRoute();
    horde::platform::android::ResetMusicSession(replacedQueue);
    gGameSimulation.ClearEvents();
    ClearPlatformGameplayEvents();
    PublishSimulationUiState();
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

std::string UtcTimestamp()
{
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
    gmtime_r(&now, &utc);
    char text[32]{};
    std::strftime(text, sizeof(text), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return text;
}

void CaptureConsentedPlaytestFrameOnRenderOwner(SwapchainContext& context,
    const bool framePresented, const bool framePaused)
{
    std::uint64_t token = 0u;
    {
        std::lock_guard<std::mutex> lock(gReportMutex);
        if (gPlaytestCapture.state != PlaytestCaptureRequest::State::Pending) return;
        token = gPlaytestCapture.token;
    }
    // This exact frame must have reached presentation; a recorded/ready target
    // alone is not RT presentation evidence. Do not capture at all when unpaused.
    bool pauseAcknowledged = false;
    {
        std::lock_guard<std::mutex> lock(gInputPublisherMutex);
        pauseAcknowledged = gLifecycleMeasurementPaused && gInputPublisherState.paused &&
            gLifecyclePauseAcknowledgedGeneration >= gLifecyclePauseGeneration;
    }
    if (!framePresented || !context.capabilities.rtScene.presented ||
        !framePaused || !pauseAcknowledged) return;
    const auto extent = context.rtScene.DispatchExtent();
    const auto sourcePixels = static_cast<std::uint64_t>(extent.width) * extent.height;
    horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture raw;
    horde::reporting::PlaytestScreenshotPixels pixels;
    std::string diagnostic; // Never added to the public report/logged with user content.
    bool success = false;
    try
    {
        success = sourcePixels != 0u && sourcePixels <= horde::reporting::kPlaytestScreenshotMaxSourcePixels &&
            context.rtScene.CaptureStorageImage(raw, diagnostic) &&
            horde::reporting::ResizePlaytestScreenshotRgba(raw.width, raw.height, raw.rgba, pixels);
    }
    catch (...) { success = false; }
    const auto ownedContext = PlaytestContextOnRenderOwner(context);
    const auto capturedAtUtc = UtcTimestamp();
    std::lock_guard<std::mutex> lock(gReportMutex);
    if (gPlaytestCapture.token != token || gPlaytestCapture.state != PlaytestCaptureRequest::State::Pending) return;
    if (!success) { gPlaytestCapture.state = PlaytestCaptureRequest::State::Failed; return; }
    gPlaytestCapture.width = pixels.width; gPlaytestCapture.height = pixels.height;
    gPlaytestCapture.pixels = std::move(pixels);
    gPlaytestCapture.context = ownedContext;
    gPlaytestCapture.capturedAtUtc = capturedAtUtc;
    gPlaytestCapture.state = PlaytestCaptureRequest::State::Ready;
}

horde::gameplay::ShowcaseBenchmarkMetadata BuildBenchmarkMetadata(const SwapchainContext& context)
{
    horde::gameplay::ShowcaseBenchmarkMetadata metadata;
    metadata.runId = context.benchmarkRunId;
    metadata.timestampUtc = UtcTimestamp();
    metadata.buildIdentity = HORDE_RT_BUILD_ID;
    metadata.shaderIdentity = context.rtScene.SelectedPipelineBundleIdentity();
    metadata.gpuName = context.capabilities.identity.gpuName;
    metadata.vulkanApi = std::to_string(VK_API_VERSION_MAJOR(context.capabilities.identity.vulkanApiVersion)) + "." +
                         std::to_string(VK_API_VERSION_MINOR(context.capabilities.identity.vulkanApiVersion)) + "." +
                         std::to_string(VK_API_VERSION_PATCH(context.capabilities.identity.vulkanApiVersion));
    metadata.rtMode = horde::vulkan::ToString(context.capabilities.rtMode);
    metadata.executionBackend = horde::vulkan::ToString(context.rtScene.ExecutionBackend());
    metadata.presentMode = PresentModeName(context.swapchainPresentMode);
    metadata.legacyFrameTimingScope = "android-render-entry-through-present";
    metadata.materialEncoding = context.rtScene.MaterialEncoding();
    metadata.renderScalePercent = static_cast<std::uint32_t>(std::lround(context.renderScale * 100.0f));
    metadata.internalWidth = context.rtScene.DispatchExtent().width;
    metadata.internalHeight = context.rtScene.DispatchExtent().height;
    metadata.presentationWidth = context.swapchainExtent.width;
    metadata.presentationHeight = context.swapchainExtent.height;
    return metadata;
}

std::optional<horde::telemetry::BenchmarkSummaryConfiguration>
ReadBenchmarkSummaryConfigurationOnOwner(const SwapchainContext& context) noexcept
{
    if (context.benchmarkSummaryRunId.empty() || !context.rtFrameEvidenceInitialised ||
        context.sceneProfile != horde::vulkan::raytracing::RtSceneProfile::Showcase) return std::nullopt;
    try
    {
        horde::telemetry::BenchmarkSummaryConfiguration configuration;
        configuration.platform = horde::telemetry::BenchmarkSummaryPlatform::Android;
        configuration.metadata = BuildBenchmarkMetadata(context);
        configuration.water = static_cast<horde::telemetry::RtWaterQuality>(context.graphicsSettings.waterQuality);
        if (!context.rtScene.HasUploadedQualityControls()) return std::nullopt;
        const auto& quality = context.rtScene.QualityControls().controls;
        configuration.shadowQuality = horde::telemetry::RtShadowQualityEvidence{
            static_cast<horde::telemetry::RtShadowMode>(quality[0]), quality[1], quality[2], 0u};
        configuration.actualUploadedMistEnabled = context.rtScene.UploadedMistEnabled();
        if (!configuration.actualUploadedMistEnabled) return {};
        configuration.actualUploadedDustQuality = context.rtScene.UploadedDustQuality();
        if (!configuration.actualUploadedDustQuality) return {};
        const auto fireQuality = context.rtScene.UploadedFireQuality();
        horde::telemetry::RtFireQuality fireTier = horde::telemetry::RtFireQuality::Mobile;
        switch (fireQuality)
        {
        case horde::vulkan::raytracing::FireEmitterQuality::Mobile:
            configuration.fire = horde::telemetry::BenchmarkSummaryFireQuality::Mobile; break;
        case horde::vulkan::raytracing::FireEmitterQuality::High:
            configuration.fire = horde::telemetry::BenchmarkSummaryFireQuality::High;
            fireTier = horde::telemetry::RtFireQuality::High; break;
        case horde::vulkan::raytracing::FireEmitterQuality::Low:
            configuration.fire = horde::telemetry::BenchmarkSummaryFireQuality::Low;
            fireTier = horde::telemetry::RtFireQuality::Low; break;
        }
        const auto fireBudget = horde::vulkan::raytracing::ResolveFireEmitterQualityBudget(fireQuality);
        configuration.uploadedFireQuality = horde::telemetry::RtFireQualityEvidence{
            fireTier, fireBudget.volumeSteps, fireBudget.reflectionSamples};
        const auto optics = context.rtScene.SelectedDielectricQualityName();
        if (optics != "Mobile" && optics != "High") return std::nullopt;
        configuration.dielectric = optics == "High" ? horde::telemetry::RtDielectricQuality::High :
            horde::telemetry::RtDielectricQuality::Mobile;
        configuration.glassEnabled = context.rtScene.GlassEnabled();
        // Real current owner scope at arming/completion, never a fabricated or
        // stale ledger scope after another lifecycle event.
        const auto seeds = context.rtFrameEvidence.SeedsByValue();
        configuration.sceneEpoch = seeds.sceneEpoch;
        configuration.measurementGeneration = seeds.measurementGeneration;
        return configuration;
    }
    catch (...) { return std::nullopt; }
}

void CaptureBenchmarkSummaryStartOnOwner(SwapchainContext& context) noexcept
{
    // Called only after the real evidence owner accepted measurement arming.
    context.benchmarkSummaryStart = ReadBenchmarkSummaryConfigurationOnOwner(context);
}

void FreezeCompletedBenchmarkSummaryOnOwner(SwapchainContext& context) noexcept
{
    if (!context.benchmarkSummaryStart) return;
    try
    {
        // The same typed owner getter seam supplies completion; no latest file,
        // JNI live scene read or fabricated scope participates in this record.
        const auto completion = ReadBenchmarkSummaryConfigurationOnOwner(context);
        if (!completion) return;
        auto summary = horde::telemetry::CaptureBenchmarkSummary(context.inAppBenchmark,
            context.benchmarkEvidence, *context.benchmarkSummaryStart, *completion,
            context.benchmarkSummaryRunId, context.benchmarkSummaryRawModel);
        if (!summary.IsReady()) return;
        std::lock_guard<std::mutex> lock(gReportMutex);
        if (context.benchmarkSummaryRevision == gBenchmarkSummaryRevision &&
            gBenchmarkSummaryRevision != std::numeric_limits<std::uint64_t>::max())
            gLatestBenchmarkSummary = std::move(summary);
    }
    catch (...) { /* Optional summary failure never changes benchmark/render success. */ }
}

void CancelActiveInAppBenchmark(SwapchainContext& context)
{
    bool cancelled = false;
#if HORDE_RT_ANDROID_MOTION_VALIDATION
    if (context.motionValidationRun)
    {
        gMotionValidationRequested.store(false, std::memory_order_release);
        if (context.motion)
            FailAndroidMotion(context, "Motion validation was cancelled before completion.");
        else
            gMotionStatus.store(4, std::memory_order_release);
        cancelled = true;
    }
#endif
    if (context.inAppBenchmark.IsRunning())
    {
        context.inAppBenchmark.Cancel();
        cancelled = true;
    }
    if (context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Allocated ||
        context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Measuring)
    {
        context.benchmarkEvidence.Cancel();
        cancelled = true;
    }
    context.benchmarkExpectedFrame.reset();
    if (cancelled)
    {
        // All active-context callers run on the owning render thread, including
        // surface teardown. Home can stop the loop before the queued cancel
        // command runs; restore the benchmark's temporary gameplay state here.
        ResetShowcaseSimulation();
        gInAppBenchmarkStatus.store(3, std::memory_order_release);
    }
}

void CancelBenchmarkEvidenceOnly(SwapchainContext& context)
{
    if (context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Allocated ||
        context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Measuring)
    {
        context.benchmarkEvidence.Cancel();
    }
    context.benchmarkExpectedFrame.reset();
}

void DeliverCompletedBenchmarkEvidence(
    SwapchainContext& context,
    const horde::vulkan::raytracing::RtFrameEvidenceCompletionResult& completion,
    const horde::telemetry::RtPerformanceEvidenceSnapshot& snapshot)
{
    if (!completion.completedEvidence ||
        context.benchmarkEvidence.Status() != horde::telemetry::RtBenchmarkRunStatus::Measuring)
    {
        return;
    }
    if (snapshot.identity.submitted.frame.sceneEpoch != context.benchmarkEvidence.SceneEpoch() ||
        snapshot.identity.submitted.frame.measurementGeneration !=
            context.benchmarkEvidence.MeasurementGeneration())
    {
        return;
    }
    if (!context.benchmarkEvidence.Complete(snapshot))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag,
                            "Android benchmark rejected completed RT evidence.");
    }
}

#if HORDE_RT_STAGED_PRIMARY_TIMING
void CollectStagedPassTiming(SwapchainContext& context, std::uint32_t frameSlot,
                            const horde::vulkan::raytracing::RtFrameEvidenceCompletionResult& completion,
                            const horde::telemetry::RtPerformanceEvidenceSnapshot& snapshot)
{
    if (!completion.ownedGraphicsSubmission) return; // Own fence/device idle already succeeded.
    const auto timing = context.stagedPassTimer.CollectCompleted(frameSlot);
    if (completion.completedEvidence &&
        context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Measuring &&
        snapshot.identity.submitted.frame.sceneEpoch == context.benchmarkEvidence.SceneEpoch() &&
        snapshot.identity.submitted.frame.measurementGeneration == context.benchmarkEvidence.MeasurementGeneration())
        (void)context.stagedPassProfile.Append(timing, snapshot);
}
#endif

void RejectPendingBenchmarkExpectation(
    SwapchainContext& context,
    const horde::telemetry::RtBenchmarkFailureReason reason)
{
    if (!context.benchmarkExpectedFrame.has_value())
    {
        return;
    }
    if (context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Measuring)
    {
        (void)context.benchmarkEvidence.RejectExpected(*context.benchmarkExpectedFrame, reason);
    }
    context.benchmarkExpectedFrame.reset();
}

void BindCommittedBenchmarkExpectation(SwapchainContext& context)
{
    if (!context.benchmarkExpectedFrame.has_value())
    {
        return;
    }
    horde::telemetry::RtSubmittedFrameIdentity committedIdentity{};
    if (context.rtFrameEvidence.TryGetCommittedIdentity(
            context.currentFrame, committedIdentity))
    {
        if (!context.benchmarkEvidence.BindSubmitted(
                *context.benchmarkExpectedFrame, committedIdentity))
        {
            __android_log_print(ANDROID_LOG_ERROR, kTag,
                                "Android benchmark failed to bind committed submission.");
        }
    }
    else
    {
        (void)context.benchmarkEvidence.RejectExpected(
            *context.benchmarkExpectedFrame,
            horde::telemetry::RtBenchmarkFailureReason::TokenlessCompletion);
    }
    context.benchmarkExpectedFrame.reset();
}

void PublishBenchmarkProgress(const SwapchainContext& context)
{
    std::lock_guard<std::mutex> lock(gReportMutex);
    gLatestBenchmarkProgress = context.inAppBenchmark.ProgressText();
}

void StartInAppBenchmark(SwapchainContext& context)
{
    const bool motionValidation =
#if HORDE_RT_ANDROID_MOTION_VALIDATION
        gMotionValidationRequested.exchange(false, std::memory_order_acq_rel);
#else
        false;
#endif
    gBenchmarkCheckpointRequested.store(-1, std::memory_order_release);
    gCaptureCheckpointRequested.store(-1, std::memory_order_release);
    gRouteReplayRequested.store(false, std::memory_order_release);
    ResetShowcaseSimulation();
    if (context.rtFrameEvidenceInitialised)
    {
        (void)context.rtFrameEvidence.ApplyEvent(
            horde::telemetry::RtLifecycleEvent::RouteReset);
        (void)context.rtFrameEvidence.ApplyEvent(
            horde::telemetry::RtLifecycleEvent::BenchmarkStart);
    }
    context.activeBenchmarkCheckpoint = -1;
    context.benchmarkSampling = false;
    context.routeReplayActive = false;
    context.captureActive = false;
    context.capturePresentedFrames = 0u;
    context.benchmarkExpectedFrame.reset();
    horde::gameplay::BenchmarkWorkload requestedWorkload =
        horde::gameplay::BenchmarkWorkload::ShowcaseRoute;
    {
        std::lock_guard<std::mutex> lock(gReportMutex);
        gLatestBenchmarkReport.clear();
        context.benchmarkRunId = std::move(gRequestedBenchmarkRunId);
        context.benchmarkSummaryRunId = std::move(gRequestedBenchmarkSummaryRunId);
        context.benchmarkSummaryRawModel = std::move(gRequestedBenchmarkSummaryRawModel);
        context.benchmarkSummaryRevision = gBenchmarkSummaryRevision;
        context.benchmarkSummaryStart.reset();
        requestedWorkload = gRequestedBenchmarkWorkload;
    }
    context.motionValidationRun = motionValidation;
    if (motionValidation)
    {
        std::lock_guard<std::mutex> lock(gReportMutex);
        gLatestBenchmarkProgress = "MOTION VALIDATION STARTING";
        gInAppBenchmarkStatus.store(1, std::memory_order_release);
        return; // The motion scenario runs through the ordinary wall-delta simulation branch below.
    }
    if (!context.benchmarkEvidence.Start(
            horde::gameplay::ShowcaseBenchmarkRun::kMaximumFramesPerLap))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag,
                            "Android benchmark evidence allocation failed; route will remain invalid.");
    }
#if HORDE_RT_STAGED_PRIMARY_TIMING
    (void)context.stagedPassProfile.Start(horde::gameplay::ShowcaseBenchmarkRun::kMaximumFramesPerLap);
#endif
    context.inAppBenchmark.Start(
        horde::gameplay::ShowcaseBenchmarkRun::kDefaultLaps, requestedWorkload,
        context.benchmarkRunId.empty()); // Automated evidence has no FPS observer.
    {
        std::lock_guard<std::mutex> lock(gReportMutex);
        gLatestBenchmarkProgress = context.inAppBenchmark.ProgressText();
    }
    gInAppBenchmarkStatus.store(1, std::memory_order_release);
}

void FinishInAppBenchmark(SwapchainContext& context)
{
    FreezeCompletedBenchmarkSummaryOnOwner(context); // Finalized owners, before route/evidence reset below.
    const horde::gameplay::ShowcaseBenchmarkMetadata metadata = BuildBenchmarkMetadata(context);
    const bool evidenceComplete = context.benchmarkEvidence.Status() ==
        horde::telemetry::RtBenchmarkRunStatus::Complete &&
        context.benchmarkEvidence.ExpectedCount() == context.inAppBenchmark.Frames().size();
    std::string text = context.inAppBenchmark.BuildTextReport(metadata, &context.benchmarkEvidence);
    std::string json = context.inAppBenchmark.BuildJsonReport(metadata, &context.benchmarkEvidence);
#if HORDE_RT_STAGED_PRIMARY_TIMING
    json = horde::vulkan::raytracing::experimental::AttachStagedPrimaryProfile(std::move(json),
        context.stagedPassProfile.Json(context.benchmarkEvidence.ExpectedCount(), context.stagedPassTimer,
                                      context.rtScene.ExecutionOrganisationJson()));
#endif
#if HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION
    PollPresentTimingOnOwner(context); // Asynchronous; unresolved tail stays explicit.
    const auto closingBrace = json.find_last_of('}');
    if (closingBrace != std::string::npos)
        json.insert(closingBrace, ",\n  \"imagePresentationTiming\": " + PresentTimingJson(context) + "\n");
#endif
    const std::string textPath = context.reportDirectory + "/HordeLanternRT-benchmark-latest.txt";
    const std::string jsonPath = context.reportDirectory + "/HordeLanternRT-benchmark-latest.json";
    const bool jsonSaved = WriteTextFile(jsonPath, json);
    text += "\nPrivate JSON copy: " + std::string(jsonSaved ? "saved" : "save failed") +
            "\nUse COPY REPORT or SAVE REPORT to export this result.\n";
    if (!WriteTextFile(textPath, text))
    {
        text += "WARNING: private text copy failed; COPY REPORT and SAVE REPORT remain available.\n";
    }
    {
        std::lock_guard<std::mutex> lock(gReportMutex);
        gLatestBenchmarkReport = text;
        gLatestBenchmarkProgress = context.inAppBenchmark.Passed() && evidenceComplete
            ? "BENCHMARK COMPLETE" : "BENCHMARK INVALID";
    }
    gInAppBenchmarkStatus.store(context.inAppBenchmark.Passed() && evidenceComplete ? 2 : 3,
                                std::memory_order_release);
    {
        std::lock_guard<std::mutex> inputLock(gInputPublisherMutex);
        gInputPublisherState.paused = true;
        PublishInputLocked();
    }
    ResetShowcaseSimulation();
    context.benchmarkExpectedFrame.reset();
    if (context.rtFrameEvidenceInitialised)
    {
        (void)context.rtFrameEvidence.ApplyEvent(
            horde::telemetry::RtLifecycleEvent::RouteReset);
    }
}

void ResetBenchmarkTiming(SwapchainContext& context)
{
    context.timingFrameCount = 0u;
    context.timingFenceMs = 0.0;
    context.timingRecordMs = 0.0;
    context.timingPresentMs = 0.0;
    context.timingTotalMs = 0.0;
    context.benchmarkWarmupFrames = 0u;
    context.benchmarkSampleFrames = 0u;
    context.benchmarkWindow = 0u;
    context.benchmarkFenceMs = 0.0;
    context.benchmarkRecordMs = 0.0;
    context.benchmarkPresentMs = 0.0;
    context.benchmarkTotalMs = 0.0;
    context.capabilities.performance.frameTimeMs = 0.0f;
    context.capabilities.performance.fps = 0.0f;
}

struct DebugCheckpointSelection
{
    horde::gameplay::ShowcaseCheckpoint checkpoint{};
    std::int32_t simulationCheckpointId = -1;
    horde::vulkan::raytracing::PlayerRenderRoute playerRoute =
        horde::vulkan::raytracing::PlayerRenderRoute::Procedural;
    const horde::gameplay::DevelopmentCheckpoint* development = nullptr;
};

bool ResolveDebugCheckpoint(const std::int32_t id, DebugCheckpointSelection& selection)
{
    if (const auto* showcase = horde::gameplay::FindShowcaseCheckpoint(id))
    {
        selection = {horde::gameplay::ShowcaseCheckpointForEncounter(*showcase,
                         horde::gameplay::simulation::ProductionGameSimulationConfig().waterfallSkeletonEncounter),
                     showcase->id, kDefaultPlayerPresentationRoute};
        return true;
    }
    const auto* development = horde::gameplay::FindDevelopmentCheckpoint(id);
    const auto* base = development == nullptr ? nullptr :
        horde::gameplay::FindShowcaseCheckpoint(development->baseShowcaseCheckpointId);
    if (development == nullptr || base == nullptr)
        return false;
    selection.checkpoint = {development->id, development->name.data(),
                            development->cameraX, development->cameraZ,
                            development->yaw, development->pitch,
                            base->expectedZone, base->preset};
    selection.simulationCheckpointId = base->id;
    selection.playerRoute = horde::vulkan::raytracing::PlayerRenderRouteForCheckpoint(
        development->name);
    selection.development = development;
    return true;
}

void ApplyDebugCheckpointSimulation(
    const DebugCheckpointSelection& selection,
    horde::vulkan::raytracing::RtSceneRecordObservation* observation)
{
    // Owning-thread, explicit full checkpoint import; not JNI or a fallback.
    const auto config = selection.playerRoute == kDefaultPlayerPresentationRoute
        ? horde::gameplay::simulation::ProductionGameSimulationConfig()
        : horde::gameplay::simulation::GameSimulationConfig{};
    const bool replacedQueue = gGameSimulation.Snapshot().playerMountProfile != config.playerMountProfile;
    if (replacedQueue)
        gGameSimulation = horde::gameplay::simulation::GameSimulation(config);
    horde::platform::android::ResetMusicSession(replacedQueue);
    if (selection.development != nullptr)
    {
        horde::gameplay::DevelopmentCheckpointStageEvidence evidence{};
        struct StepTimingState
        {
            horde::vulkan::raytracing::RtSceneRecordObservation* observation = nullptr;
            std::optional<horde::vulkan::raytracing::RtSceneStageScope> activeScope;
        } stepTiming{observation};
        const horde::gameplay::DevelopmentCheckpointStepFixedObservation
            stepObservation{
                &stepTiming,
                [](void* user) noexcept
                {
                    auto& timing = *static_cast<StepTimingState*>(user);
                    timing.activeScope.emplace(
                        timing.observation,
                        horde::telemetry::RtStage::SimulationStep);
                },
                [](void* user) noexcept
                {
                    auto& timing = *static_cast<StepTimingState*>(user);
                    if (timing.activeScope.has_value())
                    {
                        timing.activeScope->Complete(1u);
                        timing.activeScope.reset();
                    }
                }};
        const bool staged = horde::gameplay::StageDevelopmentCheckpointSimulation(
            gGameSimulation, *selection.development, &evidence,
            observation != nullptr ? &stepObservation : nullptr);
        if (selection.development->combatPose ==
            horde::gameplay::DevelopmentCombatPose::ParryActive)
        {
            __android_log_print(
                ANDROID_LOG_INFO, kTag,
                "HORDE_PARRY_STAGE checkpoint=%s staged=%d consumed_parry_edges=%u "
                "parry_success_events=%u player_damaged_events=%u player_killed_events=%u "
                "enemy_hit_events=%u action=%s action_time=%.4f events_cleared=1",
                selection.development->name.data(), staged ? 1 : 0,
                evidence.consumedParryEdges,
                evidence.playerParrySucceededEvents, evidence.playerDamagedEvents,
                evidence.playerKilledEvents, evidence.enemyHitEvents,
                DebugPlayerCombatActionName(evidence.action), evidence.actionTime);
        }
        else if (selection.development->combatPose !=
                 horde::gameplay::DevelopmentCombatPose::Rest)
        {
            __android_log_print(
                ANDROID_LOG_INFO, kTag,
                "HORDE_COMBO_STAGE checkpoint=%s staged=%d consumed_attack_edges=%u "
                "player_swing_events=%u enemy_hit_events=%u action=%s action_time=%.4f events_cleared=1",
                selection.development->name.data(), staged ? 1 : 0,
                evidence.consumedAttackEdges, evidence.playerSwingEvents,
                evidence.enemyHitEvents, DebugPlayerCombatActionName(evidence.action),
                evidence.actionTime);
        }
        return;
    }
    gGameSimulation.ApplyShowcaseCheckpoint(selection.simulationCheckpointId);
    if (selection.checkpoint.id == selection.simulationCheckpointId)
        return;
    horde::gameplay::simulation::InputSnapshot input;
    input.paused = false;
    input.damageEnabled = false;
    input.hasAuthoritativePlayerPose = true;
    input.authoritativePlayerX = selection.checkpoint.x;
    input.authoritativePlayerZ = selection.checkpoint.z;
    input.yawRadians = selection.checkpoint.yaw;
    input.pitchRadians = selection.checkpoint.pitch;
    input.torchLightStrength = 1.8f;
    horde::vulkan::raytracing::RtSceneStageScope simulationScope(
        observation, horde::telemetry::RtStage::SimulationStep);
    gGameSimulation.StepFixed(input, 0.0f,
                              gGameSimulation.Snapshot().inputPublicationSequence + 1u);
    simulationScope.Complete(1u);
}

void WriteShowcaseDebugState(const SwapchainContext& context, const char* status)
{
    const auto publication = context.rtFrameEvidence.PublishedStateByValue();
    std::string frameEvidenceJson;
    std::string frameEvidenceText;
    std::string frameEvidenceError;
    (void)horde::telemetry::SerializeRtEvidencePublication(
        publication, context.rtFrameEvidenceInitialised && context.rtFrameEvidence.ObserverAvailable(),
        frameEvidenceJson, frameEvidenceText, frameEvidenceError);
    const horde::gameplay::simulation::SimulationSnapshot& simulation = gGameSimulation.Snapshot();
    const horde::gameplay::ShowcaseZone zone = simulation.zone;
    const horde::gameplay::LichSnapshot& lich = simulation.lich;
    const horde::gameplay::EnemyRosterSnapshot& roster = simulation.enemyRoster;
    const horde::gameplay::ShowcaseReplaySnapshot& replay = context.routeReplay.Snapshot();
    const horde::gameplay::PlayerVitalsSnapshot& playerVitals = simulation.playerVitals;
    const horde::vulkan::raytracing::RtSceneTuning rtLab = gRtLabState.Snapshot();
    const bool playerDamageEnabled =
        playerVitals.phase == horde::gameplay::PlayerLifePhase::Alive &&
        !simulation.paused &&
        !context.inAppBenchmark.IsRunning() &&
        !context.routeReplayActive && !context.benchmarkSampling && !context.captureActive;
    std::ostringstream json;
    json.setf(std::ios::fixed);
    json.precision(4);
    json << "{\n"
         << "  \"schema\": 1,\n"
         << "  \"status\": \"" << status << "\",\n"
         << "  \"generation\": " << context.benchmarkGeneration << ",\n"
         << "  \"checkpoint\": \"" << (!context.activeBenchmarkName.empty() ? context.activeBenchmarkName : (context.routeReplayActive ? "route-replay" : "none")) << "\",\n"
         << "  \"player\": {\"x\": " << simulation.playerX << ", \"z\": " << simulation.playerZ
         << ", \"yaw\": " << simulation.playerYawRadians << ", \"pitch\": " << simulation.playerPitchRadians
         << ", \"vitality\": " << playerVitals.vitality
         << ", \"maxVitality\": " << playerVitals.maxVitality
         << ", \"lifePhase\": \"" << horde::gameplay::PlayerLifePhaseName(playerVitals.phase) << "\""
         << ", \"damageEnabled\": " << (playerDamageEnabled ? "true" : "false") << "},\n"
         << "  \"playerCombat\": {\"action\": \""
         << DebugPlayerCombatActionName(simulation.playerCombat.action)
         << "\", \"actionTime\": " << simulation.playerCombat.actionTime
         << ", \"comboQueued\": " << (simulation.playerCombat.comboQueued ? "true" : "false")
         << ", \"swordRadians\": " << simulation.swordCombat.swordSwingRadians
         << ", \"lastConsumedAttackSequence\": " << simulation.lastConsumedAttackSequence
         << ", \"lastConsumedParrySequence\": " << simulation.lastConsumedParrySequence
         << "},\n"
         << "  \"zone\": \"" << horde::gameplay::ShowcaseZoneName(zone) << "\",\n"
         << "  \"renderScale\": " << context.renderScale << ",\n"
         << "  \"waterQuality\": " << static_cast<int>(context.graphicsSettings.waterQuality) << ",\n"
         << "  \"rtLab\": {\"waterfallWidthScale\": " << rtLab.waterfallWidthScale
         << ", \"roofOverrideEnabled\": " << (rtLab.finaleRoofOpenOverride.has_value() ? "true" : "false")
         << ", \"roofOpen\": " << rtLab.finaleRoofOpenOverride.value_or(-1.0f)
         << ", \"dawnOverrideEnabled\": " << (rtLab.finaleDawnRevealOverride.has_value() ? "true" : "false")
         << ", \"dawnReveal\": " << rtLab.finaleDawnRevealOverride.value_or(-1.0f)
         << ", \"fogDensityScale\": " << rtLab.fogDensityScale
         << ", \"fireStrengthScale\": " << rtLab.fireStrengthScale
         << ", \"fireTurbulenceScale\": " << rtLab.fireTurbulenceScale
         << ", \"fireSmokeScale\": " << rtLab.fireSmokeScale
         << ", \"glassVisible\": " << (rtLab.glassFixtureVisible ? "true" : "false")
         << ", \"productionRewardPropsVisible\": "
         << (context.productionRewardPropsRequested ? "true" : "false")
         << ", \"productionLanternGlassOnly\": "
         << (context.productionLanternGlassOnly ? "true" : "false")
         << ", \"glassTransmission\": " << rtLab.glassTransmission
         << ", \"glassIor\": " << rtLab.glassIor
         << ", \"glassRoughness\": " << rtLab.glassRoughness
         << ", \"workloadPreset\": " << static_cast<std::int32_t>(rtLab.workloadPreset) << "},\n"
         << "  \"internalExtent\": {\"width\": " << context.capabilities.performance.internalRenderWidth
         << ", \"height\": " << context.capabilities.performance.internalRenderHeight << "},\n"
         << "  \"presented\": " << (context.capabilities.rtScene.presented ? "true" : "false") << ",\n"
         << "  \"executionBackend\": \""
         << horde::vulkan::ToString(context.rtScene.ExecutionBackend()) << "\",\n"
         << "  \"playerRenderRoute\": \""
         << (context.playerRenderRoute == horde::vulkan::raytracing::PlayerRenderRoute::ModelledViewmodel
                 ? "modelled-viewmodel"
                 : context.playerRenderRoute == horde::vulkan::raytracing::PlayerRenderRoute::Skinned
                 ? "skinned"
                 : (context.playerRenderRoute ==
                        horde::vulkan::raytracing::PlayerRenderRoute::HybridBlockPrimary
                        ? "hybrid-block-primary" : "procedural")) << "\",\n"
         << "  \"playerSkinCadenceHz\": " << context.rtScene.PlayerSkinCadenceHz() << ",\n"
         << "  \"playerMountProfile\": \""
         << (simulation.playerMountProfile == horde::gameplay::items::PlayerMountProfile::AnatomicalBody
                 ? "AnatomicalBody" : "LegacyViewRelative") << "\",\n"
         << "  \"dedicatedPlayerPrimaryOwnership\": "
         << (horde::vulkan::raytracing::HasDedicatedPlayerPrimaryOwnership(
                 context.rtScene.LastInstanceMasks(), context.rtScene.LastPlayerWorldBodyInstanceFlags())
                 ? "true" : "false") << ",\n"
         << "  \"playerSkinUpdates\": " << context.rtScene.PlayerSkinUpdateCount() << ",\n"
         << "  \"playerSkinCpuAverageMs\": " << context.rtScene.PlayerSkinAverageMilliseconds() << ",\n"
         << "  \"playerMaxSocketErrorM\": " << context.rtScene.PlayerMaxSocketErrorMetres() << ",\n"
         << "  \"selectedRtPipelineBundle\": {\"opaqueFast\": {\"key\": \""
         << context.rtScene.SelectedOpaqueFastKey() << "\", \"sha256\": \""
         << context.rtScene.SelectedOpaqueFastSha256()
         << "\"}, \"genericDielectric\": {\"key\": \""
         << context.rtScene.SelectedGenericDielectricKey() << "\", \"sha256\": \""
         << context.rtScene.SelectedGenericDielectricSha256() << "\"}},\n"
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
         << "  \"executionOrganisation\": " << context.rtScene.ExecutionOrganisationJson() << ",\n"
#endif
         << "  \"diagnosticsAvailability\": \""
         << horde::vulkan::raytracing::ToString(
                context.rtScene.DiagnosticsAvailability()) << "\",\n"
         << "  \"diagnosticsAvailable\": "
         << (context.rtScene.DiagnosticsAvailability() ==
                     horde::vulkan::raytracing::RtDiagnosticAvailability::Available
                 ? "true" : "false") << ",\n"
         << "  \"dielectricTransportOverflowCount\": "
         << context.rtScene.DielectricTransportOverflowCount() << ",\n"
         << "  \"dielectricShadowOverflowCount\": "
         << context.rtScene.DielectricShadowOverflowCount() << ",\n"
         << "  \"dielectricSecondaryRejectCount\": "
         << context.rtScene.DielectricSecondaryRejectCount() << ",\n"
         << "  \"dielectricUnclosedVolumeCount\": "
         << context.rtScene.DielectricUnclosedVolumeCount() << ",\n"
         << "  \"dielectricPrimaryUnclosedVolumeCount\": "
         << context.rtScene.DielectricPrimaryUnclosedVolumeCount() << ",\n"
         << "  \"dielectricShadowUnclosedVolumeCount\": "
         << context.rtScene.DielectricShadowUnclosedVolumeCount() << ",\n"
         << "  \"productionPaneStackFailureCount\": "
         << context.rtScene.ProductionPaneStackFailureCount() << ",\n"
         << "  \"productionPaneSecondaryOriginCount\": "
         << context.rtScene.ProductionPaneSecondaryOriginCount() << ",\n"
         << "  \"productionPaneSecondaryTerminalCount\": "
         << context.rtScene.ProductionPaneSecondaryTerminalCount() << ",\n"
         << "  \"productionPaneSecondarySameMediumCount\": "
         << context.rtScene.ProductionPaneSecondarySameMediumCount() << ",\n"
         << "  \"productionPaneSecondaryDifferentMediumCount\": "
         << context.rtScene.ProductionPaneSecondaryDifferentMediumCount() << ",\n"
         << "  \"secondaryNearSelfHitCount\": " << context.rtScene.SecondaryNearSelfHitCount() << ",\n"
         << "  \"primaryOpenMissCount\": " << context.rtScene.PrimaryOpenMissCount() << ",\n"
         << "  \"primaryOpenOpaqueCount\": " << context.rtScene.PrimaryOpenOpaqueCount() << ",\n"
         << "  \"primaryMismatchedExitCount\": " << context.rtScene.PrimaryMismatchedExitCount() << ",\n"
         << "  \"primaryInterfaceBudgetCount\": " << context.rtScene.PrimaryInterfaceBudgetCount() << ",\n"
         << "  \"primaryVolumeBudgetCount\": " << context.rtScene.PrimaryVolumeBudgetCount() << ",\n"
         << "  \"shadowOpenMissCount\": " << context.rtScene.ShadowOpenMissCount() << ",\n"
         << "  \"shadowMismatchedExitCount\": " << context.rtScene.ShadowMismatchedExitCount() << ",\n"
         << "  \"primaryTirCount\": " << context.rtScene.PrimaryTirCount() << ",\n"
         << "  \"primaryInterfaceBudgetOpenVolumeCount\": "
         << context.rtScene.PrimaryInterfaceBudgetOpenVolumeCount() << ",\n"
         << "  \"primaryInterfaceBudgetClosedVolumeCount\": "
         << context.rtScene.PrimaryInterfaceBudgetClosedVolumeCount() << ",\n"
         << "  \"shadowMismatchEmptyCount\": " << context.rtScene.ShadowMismatchEmptyCount() << ",\n"
         << "  \"shadowImplicitOriginExitCount\": "
         << context.rtScene.ShadowImplicitOriginExitCount() << ",\n"
         << "  \"secondaryDielectricTerminalCount\": "
         << context.rtScene.SecondaryDielectricTerminalCount() << ",\n"
         << "  \"primaryTirTerminationCount\": "
         << context.rtScene.PrimaryTirTerminationCount() << ",\n"
         << "  \"shadowFiniteEndpointVolumeCount\": "
         << context.rtScene.ShadowFiniteEndpointVolumeCount() << ",\n"
         << "  \"primaryOpenOpaqueSameInstanceDifferentMaterialCount\": "
         << context.rtScene.PrimaryOpenOpaqueSameInstanceDifferentMaterialCount() << ",\n"
         << "  \"primaryOpenOpaqueAfterTirCount\": "
         << context.rtScene.PrimaryOpenOpaqueAfterTirCount() << ",\n"
         << "  \"primaryOpenOpaqueTerminalInstanceMask\": "
         << context.rtScene.PrimaryOpenOpaqueTerminalInstanceMask() << ",\n"
         << "  \"primaryOpenOpaqueVolumeInstanceMask\": "
         << context.rtScene.PrimaryOpenOpaqueVolumeInstanceMask() << ",\n"
         << "  \"primaryOpenOpaqueTerminalMaterialMask\": "
         << context.rtScene.PrimaryOpenOpaqueTerminalMaterialMask() << ",\n"
         << "  \"primaryClosedVolumeAbsorptionCount\": "
         << context.rtScene.PrimaryClosedVolumeAbsorptionCount() << ",\n"
         << "  \"primaryCertifiedClosedVolumeRecoveryCount\": "
         << context.rtScene.PrimaryCertifiedClosedVolumeRecoveryCount() << ",\n"
         << "  \"shadowCertifiedClosedVolumeRecoveryCount\": "
         << context.rtScene.ShadowCertifiedClosedVolumeRecoveryCount() << ",\n"
         << "  \"certifiedClosedVolumeRecoveryReasonMask\": "
         << context.rtScene.CertifiedClosedVolumeRecoveryReasonMask() << ",\n"
         << "  \"tlasInstanceCount\": " << context.rtScene.TlasInstanceCount() << ",\n"
         << "  \"buildIdentity\": \"" << HORDE_RT_BUILD_ID << " DEBUG\",\n"
         << "  \"shaderIdentity\": \""
         << context.rtScene.SelectedPipelineBundleDisplayIdentity()
         << "\",\n"
         << "  \"gpu\": \"" << context.capabilities.identity.gpuName << "\",\n"
         << "  \"swapchainExtent\": {\"width\": " << context.swapchainExtent.width
         << ", \"height\": " << context.swapchainExtent.height << "},\n"
         << "  \"outputRedBlueSwap\": "
         << ((context.swapchainFormat == VK_FORMAT_B8G8R8A8_UNORM ||
              context.swapchainFormat == VK_FORMAT_B8G8R8A8_SRGB) &&
             context.rtScene.DispatchExtent().width == context.swapchainExtent.width &&
             context.rtScene.DispatchExtent().height == context.swapchainExtent.height ? "true" : "false") << ",\n"
         << "  \"animationTime\": " << simulation.walkTime << ",\n"
         << "  \"captureStableFrames\": " << context.capturePresentedFrames << ",\n"
         << "  \"torchFailurePhase\": \"" << horde::gameplay::TorchFailurePhaseName(simulation.torchFailure.phase) << "\",\n"
         << "  \"selectedEnemy\": \"" << horde::gameplay::EnemyKindName(roster.selectedEnemy) << "\",\n"
         << "  \"activeSkinnedEnemies\": "
         << (simulation.activeEnemyKind == horde::gameplay::EnemyKind::Skeleton
                 ? simulation.activeSkeletonCount
                 : roster.renderedEnemyCount) << ",\n"
         << "  \"activeEnemyEntities\": "
         << (simulation.activeEnemyKind == horde::gameplay::EnemyKind::Skeleton
                 ? simulation.activeSkeletonCount
                 : roster.renderedEnemyCount) << ",\n"
         << "  \"attackerEntityId\": " << static_cast<std::uint32_t>(simulation.skeletonAttackerId) << ",\n"
         << "  \"skeletonPoseBuckets\": " << context.rtScene.SkeletonPoseBucketCount() << ",\n"
         << "  \"gpuTimingMode\": \"" << (context.gpuFrameTimingEnabled ? "enabled" : "disabled") << "\",\n"
         << "  \"lich\": {\"phase\": \"" << horde::gameplay::LichPhaseName(lich.phase)
         << "\", \"health\": " << lich.health << ", \"roofProgress\": " << lich.finaleSkylightOpenProgress
         << ", \"ending\": \"" << horde::gameplay::FinaleEndingPhaseName(lich.finaleEndingPhase)
         << "\", \"dawnProgress\": " << lich.finaleDawnRevealProgress << "},\n"
         << "  \"benchmarkWindowsCompleted\": " << context.benchmarkWindow << ",\n"
         << "  \"replayWaypointsReached\": " << replay.reachedWaypoints << ",\n"
         << "  \"replayComplete\": " << (replay.complete ? "true" : "false") << ",\n"
         << "  \"replayFailed\": " << (replay.failed ? "true" : "false") << ",\n"
         << "  \"rtFrameEvidence\": " << frameEvidenceJson
         << "}\n";
    WriteTextFile(context.reportDirectory + '/' + kShowcaseDebugStateFilename, json.str());
}

void ApplyBenchmarkCheckpoint(
    SwapchainContext& context,
    const DebugCheckpointSelection& selection,
    horde::vulkan::raytracing::RtSceneRecordObservation* observation)
{
    const auto& checkpoint = selection.checkpoint;
    ApplyDebugCheckpointSimulation(selection, observation);
    if (context.rtFrameEvidenceInitialised)
    {
        (void)context.rtFrameEvidence.ApplyEvent(
            horde::telemetry::RtLifecycleEvent::CheckpointChange);
        (void)context.rtFrameEvidence.ApplyEvent(
            horde::telemetry::RtLifecycleEvent::BenchmarkStart);
    }
    gGameSimulation.ResetTiming();
    gGameSimulation.ClearEvents();
    PublishSimulationUiState();
    context.activeBenchmarkCheckpoint = checkpoint.id;
    context.activeBenchmarkName = checkpoint.name;
    context.playerRenderRoute = selection.playerRoute;
    context.glassFixtureRequested =
        selection.development != nullptr && selection.development->usesGlassFixture;
    context.productionRewardPropsRequested = selection.development != nullptr &&
        selection.development->usesProductionRewardProps;
    context.productionLanternGlassOnly = context.productionRewardPropsRequested &&
        selection.development->productionLanternGlassOnly;
    if (context.glassFixtureRequested)
    {
        context.glassDepthScale = selection.development->glassDepthScale;
        context.glassAttenuationColor = selection.development->glassAttenuationColor;
        context.glassAttenuationDistance = selection.development->glassAttenuationDistance;
    }
    context.routeReplayActive = false;
    context.captureActive = false;
    context.capturePresentedFrames = 0u;
    context.benchmarkSampling = true;
    ++context.benchmarkGeneration;
    ResetBenchmarkTiming(context);
    __android_log_print(ANDROID_LOG_INFO,
                        kTag,
                        "HORDE_BENCH begin generation=%u checkpoint=%s scale=%.0f warmup_frames=120 sample_frames=120 windows=3 zone=%s",
                        context.benchmarkGeneration,
                        checkpoint.name,
                        context.renderScale * 100.0f,
                        horde::gameplay::ShowcaseZoneName(checkpoint.expectedZone));
    WriteShowcaseDebugState(context, "warming");
}

void ApplyCaptureCheckpoint(
    SwapchainContext& context,
    const DebugCheckpointSelection& selection,
    horde::vulkan::raytracing::RtSceneRecordObservation* observation)
{
    const auto& checkpoint = selection.checkpoint;
    ApplyDebugCheckpointSimulation(selection, observation);
    if (context.rtFrameEvidenceInitialised)
    {
        (void)context.rtFrameEvidence.ApplyEvent(
            horde::telemetry::RtLifecycleEvent::CheckpointChange);
    }
    gGameSimulation.ResetTiming();
    gGameSimulation.ClearEvents();
    PublishSimulationUiState();
    context.activeBenchmarkCheckpoint = checkpoint.id;
    context.activeBenchmarkName = checkpoint.name;
    context.playerRenderRoute = selection.playerRoute;
    context.glassFixtureRequested =
        selection.development != nullptr && selection.development->usesGlassFixture;
    context.productionRewardPropsRequested = selection.development != nullptr &&
        selection.development->usesProductionRewardProps;
    context.productionLanternGlassOnly = context.productionRewardPropsRequested &&
        selection.development->productionLanternGlassOnly;
    if (context.glassFixtureRequested)
    {
        context.glassDepthScale = selection.development->glassDepthScale;
        context.glassAttenuationColor = selection.development->glassAttenuationColor;
        context.glassAttenuationDistance = selection.development->glassAttenuationDistance;
    }
    context.routeReplayActive = false;
    context.benchmarkSampling = false;
    context.captureActive = true;
    context.capturePresentedFrames = 0u;
    ++context.benchmarkGeneration;
    ResetBenchmarkTiming(context);
    __android_log_print(ANDROID_LOG_INFO,
                        kTag,
                        "HORDE_CAPTURE begin generation=%u checkpoint=%s scale=%.0f stable_frames=12 zone=%s",
                        context.benchmarkGeneration,
                        checkpoint.name,
                        context.renderScale * 100.0f,
                        horde::gameplay::ShowcaseZoneName(checkpoint.expectedZone));
    WriteShowcaseDebugState(context, "capture-warming");
}

void ApplyRouteReplay(SwapchainContext& context)
{
    ResetShowcaseSimulation();
    if (context.rtFrameEvidenceInitialised)
    {
        (void)context.rtFrameEvidence.ApplyEvent(
            horde::telemetry::RtLifecycleEvent::RouteReset);
    }
    context.activeBenchmarkCheckpoint = -1;
    context.activeBenchmarkName.clear();
    context.playerRenderRoute = kDefaultPlayerPresentationRoute;
    context.glassFixtureRequested = false;
    context.productionRewardPropsRequested = false;
    context.productionLanternGlassOnly = false;
    context.glassDepthScale = 1.0f;
    context.glassAttenuationColor = {{0.72f, 0.90f, 1.0f}};
    context.glassAttenuationDistance = 2.4f;
    context.benchmarkSampling = false;
    context.captureActive = false;
    context.capturePresentedFrames = 0u;
    context.routeReplay.Reset();
    context.routeReplayActive = true;
    ++context.benchmarkGeneration;
    ResetBenchmarkTiming(context);
    __android_log_print(ANDROID_LOG_INFO,
                        kTag,
                        "HORDE_REPLAY begin generation=%u waypoints=%zu distance_per_frame=0.032",
                        context.benchmarkGeneration,
                        horde::gameplay::kShowcaseReplayPath.size());
    WriteShowcaseDebugState(context, "replaying");
}

void RecordBenchmarkFrame(SwapchainContext& context,
                          double fenceMs,
                          double recordMs,
                          double presentMs,
                          double totalMs)
{
    if (!context.benchmarkSampling || context.activeBenchmarkCheckpoint < 0)
    {
        return;
    }
    if (context.benchmarkWarmupFrames < 120u)
    {
        ++context.benchmarkWarmupFrames;
        if (context.benchmarkWarmupFrames == 120u)
        {
            if (context.rtFrameEvidenceInitialised)
            {
                (void)context.rtFrameEvidence.ApplyEvent(
                    horde::telemetry::RtLifecycleEvent::WarmupToMeasure);
            }
            WriteShowcaseDebugState(context, "sampling");
        }
        return;
    }

    context.benchmarkFenceMs += fenceMs;
    context.benchmarkRecordMs += recordMs;
    context.benchmarkPresentMs += presentMs;
    context.benchmarkTotalMs += totalMs;
    if (++context.benchmarkSampleFrames < 120u)
    {
        return;
    }

    ++context.benchmarkWindow;
    const double count = static_cast<double>(context.benchmarkSampleFrames);
    const horde::gameplay::ShowcaseZone zone =
        gGameSimulation.Snapshot().zone;
    __android_log_print(ANDROID_LOG_INFO,
                        kTag,
                        "HORDE_BENCH sample generation=%u checkpoint=%s scale=%.0f window=%u frames=120 total_ms=%.3f fence_ms=%.3f record_ms=%.3f present_ms=%.3f zone=%s presented=%d",
                        context.benchmarkGeneration,
                        context.activeBenchmarkName.c_str(),
                        context.renderScale * 100.0f,
                        context.benchmarkWindow,
                        context.benchmarkTotalMs / count,
                        context.benchmarkFenceMs / count,
                        context.benchmarkRecordMs / count,
                        context.benchmarkPresentMs / count,
                        horde::gameplay::ShowcaseZoneName(zone),
                        context.capabilities.rtScene.presented ? 1 : 0);
    context.benchmarkSampleFrames = 0u;
    context.benchmarkFenceMs = 0.0;
    context.benchmarkRecordMs = 0.0;
    context.benchmarkPresentMs = 0.0;
    context.benchmarkTotalMs = 0.0;

    if (context.benchmarkWindow >= 3u)
    {
        context.benchmarkSampling = false;
        __android_log_print(ANDROID_LOG_INFO,
                            kTag,
                            "HORDE_BENCH complete generation=%u checkpoint=%s scale=%.0f windows=3",
                            context.benchmarkGeneration,
                            context.activeBenchmarkName.c_str(),
                            context.renderScale * 100.0f);
        WriteShowcaseDebugState(context, "complete");
    }
    else
    {
        WriteShowcaseDebugState(context, "sampling");
    }
}

horde::vulkan::DeviceCapabilities RunProbe()
{
    horde::vulkan::VulkanContext context;
    context.InitialiseForCapabilityProbe();
    return context.QueryDeviceCapabilities();
}

VkClearColorValue ClearColorForMode(const horde::vulkan::RtMode mode)
{
    switch (mode)
    {
    case horde::vulkan::RtMode::RayTracingPipeline:
        return {{0.04f, 0.34f, 0.08f, 1.0f}};
    case horde::vulkan::RtMode::RayQuery:
        return {{0.14f, 0.07f, 0.42f, 1.0f}};
    default:
        return {{0.33f, 0.04f, 0.04f, 1.0f}};
    }
}

bool CreateInstance(VkInstance& instance, horde::vulkan::PresentSurfaceSupport& presentSurfaceSupport)
{
    std::vector<const char*> extensions{VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};
    presentSurfaceSupport = horde::vulkan::AppendOptionalPresentInstanceExtensions(extensions);
    const VkApplicationInfo appInfo{
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "HordeLanternRTDiagnostic",
        VK_MAKE_VERSION(1, 0, 0),
        "horde_rt",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_2};

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    const VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to create Vulkan instance for Android diagnostic: VkResult(%d)", result);
        return false;
    }

    return true;
}

bool CreateSurface(VkInstance instance, ANativeWindow* window, VkSurfaceKHR& surface)
{
    VkAndroidSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.window = window;

    const VkResult result = vkCreateAndroidSurfaceKHR(instance, &createInfo, nullptr, &surface);
    if (result != VK_SUCCESS)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to create Android Vulkan surface: VkResult(%d)", result);
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
        constexpr VkQueueFlags requiredFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
        if ((queueFamilies[index].queueFlags & requiredFlags) != requiredFlags)
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

bool CreateLogicalDevice(VkPhysicalDevice physicalDevice,
                         VkInstance instance,
                         uint32_t graphicsQueueFamilyIndex,
                         horde::vulkan::RtExecutionBackend executionBackend,
                         VkDevice& device,
                         VkQueue& graphicsQueue,
                         horde::vulkan::PresentSurfaceSupport presentSurfaceSupport,
                         horde::vulkan::PresentCompletionMode& presentCompletionMode,
                         bool& presentTimingExtensionEnabled)
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
    presentCompletionMode = horde::vulkan::SelectPresentCompletion(presentSurfaceSupport,
        horde::vulkan::QueryPresentDeviceSupport(physicalDevice, instance));
    if (const auto* completionExtension = horde::vulkan::PresentCompletionExtension(presentCompletionMode))
        extensions.push_back(completionExtension);
    presentTimingExtensionEnabled = false;
#if HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION
    // Investigation-only capability. Never a rendering/backend requirement.
    presentTimingExtensionEnabled = HasDeviceExtension(physicalDevice, VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME);
    if (presentTimingExtensionEnabled) extensions.push_back(VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME);
#endif
    const auto rtPlan = horde::vulkan::raytracing::MakeRtDeviceEnablePlan(executionBackend);
    const bool enableRayTracing = rtPlan.has_value();
    const horde::vulkan::FeatureSupport requestedFeatures = rtPlan ? rtPlan->features : horde::vulkan::FeatureSupport{};
    if (enableRayTracing)
    {
        for (std::uint32_t index = 0u; index < rtPlan->extensionCount; ++index)
        {
            const char* extension = rtPlan->extensions[index];
            if (!HasDeviceExtension(physicalDevice, extension))
            {
                __android_log_print(ANDROID_LOG_ERROR, kTag, "Selected hardware RT backend is missing required extension: %s", extension);
                return false;
            }
            extensions.push_back(extension);
        }
    }

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    accelerationStructureFeatures.accelerationStructure = requestedFeatures.accelerationStructure ? VK_TRUE : VK_FALSE;
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    rayTracingPipelineFeatures.rayTracingPipeline = requestedFeatures.rayTracingPipeline ? VK_TRUE : VK_FALSE;
    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
    rayQueryFeatures.rayQuery = requestedFeatures.rayQuery ? VK_TRUE : VK_FALSE;
    VkPhysicalDeviceBufferDeviceAddressFeaturesKHR bufferDeviceAddressFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR};
    bufferDeviceAddressFeatures.bufferDeviceAddress = requestedFeatures.bufferDeviceAddress ? VK_TRUE : VK_FALSE;
    VkPhysicalDeviceFeatures2 features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    VkPhysicalDeviceFeatures supportedCoreFeatures{};
    vkGetPhysicalDeviceFeatures(physicalDevice, &supportedCoreFeatures);
    features2.features.textureCompressionASTC_LDR = supportedCoreFeatures.textureCompressionASTC_LDR;
    features2.pNext = &accelerationStructureFeatures;
    accelerationStructureFeatures.pNext = &rayQueryFeatures;
    if (requestedFeatures.rayTracingPipeline)
        accelerationStructureFeatures.pNext = &rayTracingPipelineFeatures;
    rayTracingPipelineFeatures.pNext = &rayQueryFeatures;
    rayQueryFeatures.pNext = &bufferDeviceAddressFeatures;
    VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT presentFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT};
    if (presentCompletionMode != horde::vulkan::PresentCompletionMode::Unextended)
    {
        presentFeatures.swapchainMaintenance1 = VK_TRUE;
        presentFeatures.pNext = enableRayTracing ? features2.pNext : nullptr;
        features2.pNext = &presentFeatures;
    }

    const VkDeviceCreateInfo createInfo{
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        enableRayTracing || presentCompletionMode != horde::vulkan::PresentCompletionMode::Unextended ? &features2 : nullptr,
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
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to create Vulkan device for diagnostic surface: VkResult(%d)", createResult);
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

    return {
        std::clamp(desiredWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(desiredHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
}

VkExtent2D ScaledRenderExtent(VkExtent2D presentationExtent, float renderScale)
{
    const int percent = horde::graphics::ClampGraphicsRenderScalePercent(static_cast<int>(std::lround(
        std::clamp(renderScale, 0.0f, 1.0f) * 100.0f)));
    const auto extent = horde::graphics::ScaledGraphicsExtent(
        {presentationExtent.width, presentationExtent.height}, percent);
    return {extent.width, extent.height};
}

bool CreateSwapchain(SwapchainContext& context)
{
    VkSurfaceCapabilitiesKHR capabilities{};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physicalDevice, context.surface, &capabilities) != VK_SUCCESS)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to query Android surface capabilities.");
        return false;
    }

    uint32_t formatCount = 0u;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0u)
    {
        return false;
    }

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface, &formatCount, formats.data()) != VK_SUCCESS)
    {
        return false;
    }

    uint32_t presentModeCount = 0u;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(context.physicalDevice, context.surface, &presentModeCount, nullptr) != VK_SUCCESS ||
        presentModeCount == 0u)
    {
        return false;
    }

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(context.physicalDevice, context.surface, &presentModeCount, presentModes.data()) != VK_SUCCESS)
    {
        return false;
    }

    VkSurfaceFormatKHR chosenFormat = formats[0];
    for (const VkSurfaceFormatKHR& candidate : formats)
    {
        if (candidate.format == VK_FORMAT_B8G8R8A8_UNORM && candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
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

    ANativeWindow* window = context.window;
    const uint32_t width = static_cast<uint32_t>(ANativeWindow_getWidth(window));
    const uint32_t height = static_cast<uint32_t>(ANativeWindow_getHeight(window));
    context.nativeWindowExtent = {width, height};
    context.surfaceCurrentExtent = capabilities.currentExtent;
    context.surfaceSupportedTransforms = capabilities.supportedTransforms;
    context.surfaceCurrentTransform = capabilities.currentTransform;
    const auto presentationPolicy = horde::platform::android::ChooseSurfacePresentationPolicy(
        static_cast<std::uint32_t>(capabilities.supportedTransforms),
        static_cast<std::uint32_t>(capabilities.currentTransform));
    if (!presentationPolicy.transformSupported)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag,
            "Surface transform 0x%08x has no supported RT pre-rotation mapping.",
            static_cast<unsigned int>(capabilities.currentTransform));
        return false;
    }
    context.swapchainPreTransform = static_cast<VkSurfaceTransformFlagBitsKHR>(
        presentationPolicy.chosenPreTransform);
    context.rtPreRotationRequired = presentationPolicy.rtPreRotationRequired;
    context.rtPresentationTransform = presentationPolicy.rtTransform;
    context.swapchainExtent = ClampExtent(capabilities, std::max(1u, width), std::max(1u, height));
    // Swapchain/storage images use the surface's natural orientation. Primary
    // rays use the oriented view aspect; native UI remains in window coordinates.
    if (horde::graphics::RtPresentationTransformSwapsAxes(context.rtPresentationTransform))
        std::swap(context.swapchainExtent.width, context.swapchainExtent.height);
    context.swapchainFormat = chosenFormat.format;
    context.swapchainColorSpace = chosenFormat.colorSpace;
    context.swapchainPresentMode = chosenPresentMode;

    uint32_t imageCount = std::max(2u, capabilities.minImageCount);
    if (capabilities.maxImageCount > 0u && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        nullptr,
        0,
        context.surface,
        imageCount,
        context.swapchainFormat,
        context.swapchainColorSpace,
        context.swapchainExtent,
        1u,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        context.swapchainPreTransform,
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        chosenPresentMode,
        VK_TRUE,
        VK_NULL_HANDLE};

    if (vkCreateSwapchainKHR(context.device, &createInfo, nullptr, &context.swapchain) != VK_SUCCESS)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to create swapchain.");
        return false;
    }

    uint32_t imageArraySize = 0u;
    if (vkGetSwapchainImagesKHR(context.device, context.swapchain, &imageArraySize, nullptr) != VK_SUCCESS || imageArraySize == 0u)
    {
        return false;
    }

    context.swapchainImages.resize(imageArraySize);
    if (vkGetSwapchainImagesKHR(context.device, context.swapchain, &imageArraySize, context.swapchainImages.data()) != VK_SUCCESS)
    {
        return false;
    }
    context.swapchainImageLayouts.assign(context.swapchainImages.size(), VK_IMAGE_LAYOUT_UNDEFINED);

    context.swapchainImageViews.resize(context.swapchainImages.size());
    for (size_t index = 0u; index < context.swapchainImages.size(); ++index)
    {
        const VkImageViewCreateInfo viewCreateInfo{
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            nullptr,
            0,
            context.swapchainImages[index],
            VK_IMAGE_VIEW_TYPE_2D,
            context.swapchainFormat,
            {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
             VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u}};
        if (vkCreateImageView(context.device, &viewCreateInfo, nullptr, &context.swapchainImageViews[index]) != VK_SUCCESS)
        {
            return false;
        }
    }

    const VkAttachmentDescription colorAttachment{
        0,
        context.swapchainFormat,
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

    const VkAttachmentDescription attachmentArray[] = {colorAttachment};
    const VkSubpassDescription subpassArray[] = {subpass};
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0u;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.srcAccessMask = 0u;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    const VkRenderPassCreateInfo renderPassCreateInfo{
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        nullptr,
        0,
        static_cast<uint32_t>(std::size(attachmentArray)),
        attachmentArray,
        1u,
        subpassArray,
        1u,
        &dependency};

    if (vkCreateRenderPass(context.device, &renderPassCreateInfo, nullptr, &context.renderPass) != VK_SUCCESS)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to create render pass.");
        return false;
    }

    context.swapchainFramebuffers.resize(context.swapchainImageViews.size());
    for (size_t index = 0u; index < context.swapchainImageViews.size(); ++index)
    {
        VkImageView attachments[] = {context.swapchainImageViews[index]};
        const VkFramebufferCreateInfo framebufferCreateInfo{
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            nullptr,
            0,
            context.renderPass,
            1u,
            attachments,
            context.swapchainExtent.width,
            context.swapchainExtent.height,
            1u};
        if (vkCreateFramebuffer(context.device, &framebufferCreateInfo, nullptr, &context.swapchainFramebuffers[index]) != VK_SUCCESS)
        {
            return false;
        }
    }

    const VkCommandPoolCreateInfo commandPoolCreateInfo{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        context.graphicsQueueFamilyIndex};

    if (vkCreateCommandPool(context.device, &commandPoolCreateInfo, nullptr, &context.commandPool) != VK_SUCCESS)
    {
        return false;
    }

    VkCommandBufferAllocateInfo commandBufferAllocateInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        context.commandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        static_cast<uint32_t>(context.swapchainImageViews.size())};
    context.commandBuffers.resize(context.swapchainImageViews.size());
    if (vkAllocateCommandBuffers(context.device, &commandBufferAllocateInfo, context.commandBuffers.data()) != VK_SUCCESS)
    {
        return false;
    }

    context.renderFinishedSemaphores.resize(context.swapchainImages.size(), VK_NULL_HANDLE);
    VkSemaphoreCreateInfo semaphoreCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
    for (uint32_t i = 0u; i < kMaxFramesInFlight; ++i)
    {
        if (vkCreateSemaphore(context.device, &semaphoreCreateInfo, nullptr, &context.imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(context.device, &fenceCreateInfo, nullptr, &context.inFlightFences[i]) != VK_SUCCESS)
        {
            return false;
        }
    }

    for (VkSemaphore& semaphore : context.renderFinishedSemaphores)
    {
        if (vkCreateSemaphore(context.device, &semaphoreCreateInfo, nullptr, &semaphore) != VK_SUCCESS)
        {
            return false;
        }
    }
    if (!context.presentCompletionFences.Create(context.device, context.swapchainImages.size(),
        context.presentCompletionMode != horde::vulkan::PresentCompletionMode::Unextended)) return false;

#if HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION
    if (context.presentTiming) (void)context.presentTiming->BindSwapchain(context.device, context.swapchain, context.surfaceGeneration);
#endif
    return true;
}

VkPhysicalDevice FindMatchingPhysicalDevice(
    const VkInstance instance,
    const horde::vulkan::DeviceCapabilities& capabilities,
    const VkSurfaceKHR surface)
{
    uint32_t physicalDeviceCount = 0u;
    if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr) != VK_SUCCESS || physicalDeviceCount == 0u)
    {
        return VK_NULL_HANDLE;
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

    for (const VkPhysicalDevice candidate : physicalDevices)
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);
        const horde::vulkan::DeviceIdentity candidateIdentity{
            properties.deviceName, properties.vendorID, properties.deviceID,
            properties.driverVersion, properties.apiVersion};
        if (horde::vulkan::raytracing::SameRtDeviceIdentity(capabilities.identity, candidateIdentity))
        {
            uint32_t queueFamilyIndex = 0u;
            if (FindGraphicsAndPresentQueueFamily(candidate, surface, queueFamilyIndex))
            {
                return candidate;
            }
        }
    }

    return VK_NULL_HANDLE;
}

void RefreshGpuTimingTelemetry(
    SwapchainContext& context,
    const horde::vulkan::GpuFrameTimingCollection* completed = nullptr);
bool CompleteRtEvidenceAfterDeviceIdle(SwapchainContext& context, VkResult idleResult);
horde::telemetry::RtSampleStatus CurrentInitialGpuEvidenceStatus(
    SwapchainContext& context);

bool ConsumePendingImageAcquire(SwapchainContext& context)
{
    if (!context.imageAcquirePending) return true;
    // Recording or a surface-generation change can abort after acquire. Consume
    // its signal before the teardown idle so WSI no longer owns this semaphore.
    // No command buffers or presentation evidence belong to this drain.
    const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.waitSemaphoreCount = 1u;
    submitInfo.pWaitSemaphores = &context.imageAvailableSemaphores[context.currentFrame];
    submitInfo.pWaitDstStageMask = &waitStage;
    if (vkQueueSubmit(context.graphicsQueue, 1u, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
        return false;
    context.imageAcquirePending = false;
    return true;
}

bool ReleaseSwapchainResources(SwapchainContext& context)
{
    if (context.device == VK_NULL_HANDLE)
    {
        return true;
    }

    if (!ConsumePendingImageAcquire(context)) return false;
    // Unextended Vulkan has no present-completion fence at retirement. Retain
    // the portable idle drain; per-image reuse below never relies on this idle.
    const VkResult idleResult = vkDeviceWaitIdle(context.device);
    const bool evidenceCompleted = CompleteRtEvidenceAfterDeviceIdle(context, idleResult);
    if (idleResult != VK_SUCCESS) return false;
    if (context.presentCompletionFences.Drain() != VK_SUCCESS ||
        !context.presentCompletionFences.DestroyCompleted()) return false;
    CancelActiveInAppBenchmark(context);
    const bool evidenceRecreated = !context.rtFrameEvidenceInitialised ||
        context.rtFrameEvidence.Recreate(
            horde::telemetry::RtResourceResetReason::SwapchainRecreate,
            CurrentInitialGpuEvidenceStatus(context));
    context.gpuFrameTimer.ResetAfterDeviceIdle();
#if HORDE_RT_STAGED_PRIMARY_TIMING
    if (idleResult == VK_SUCCESS) context.stagedPassTimer.ResetAfterDeviceIdle();
#endif
    context.gpuFrameTimingTotalMs = 0.0;
    context.gpuFrameTimingSampleCount = 0u;
    context.rtScene.Destroy();

    if (context.commandPool != VK_NULL_HANDLE)
    {
        if (!context.commandBuffers.empty())
        {
            vkFreeCommandBuffers(context.device,
                                 context.commandPool,
                                 static_cast<uint32_t>(context.commandBuffers.size()),
                                 context.commandBuffers.data());
            context.commandBuffers.clear();
        }
        vkDestroyCommandPool(context.device, context.commandPool, nullptr);
        context.commandPool = VK_NULL_HANDLE;
    }

    for (VkFramebuffer framebuffer : context.swapchainFramebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(context.device, framebuffer, nullptr);
        }
    }
    context.swapchainFramebuffers.clear();

    for (VkImageView imageView : context.swapchainImageViews)
    {
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(context.device, imageView, nullptr);
        }
    }
    context.swapchainImageViews.clear();

    if (context.renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(context.device, context.renderPass, nullptr);
        context.renderPass = VK_NULL_HANDLE;
    }

    if (context.swapchain != VK_NULL_HANDLE)
    {
#if HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION
        PollPresentTimingOnOwner(context);
        if (context.presentTiming) context.presentTiming->UnbindSwapchain();
        (void)WriteTextFile(context.reportDirectory + "/HordeLanternRT-present-timing-latest.json", PresentTimingJson(context));
#endif
        vkDestroySwapchainKHR(context.device, context.swapchain, nullptr);
        context.swapchain = VK_NULL_HANDLE;
    }

    for (VkFence& fence : context.inFlightFences)
    {
        if (fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(context.device, fence, nullptr);
            fence = VK_NULL_HANDLE;
        }
    }
    for (VkSemaphore& semaphore : context.imageAvailableSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(context.device, semaphore, nullptr);
            semaphore = VK_NULL_HANDLE;
        }
    }
    for (VkSemaphore& semaphore : context.renderFinishedSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(context.device, semaphore, nullptr);
            semaphore = VK_NULL_HANDLE;
        }
    }

    context.renderFinishedSemaphores.clear();

    context.swapchainImageLayouts.clear();
    context.swapchainImages.clear();
    context.currentFrame = 0u;
    return evidenceCompleted && evidenceRecreated;
}

void RefreshGpuTimingTelemetry(
    SwapchainContext& context,
    const horde::vulkan::GpuFrameTimingCollection* completed)
{
    if (completed != nullptr && completed->hasSample)
    {
        context.gpuFrameTimingTotalMs += completed->sample.milliseconds;
        ++context.gpuFrameTimingSampleCount;
    }
    if (!context.gpuFrameTimingEnabled)
    {
        context.capabilities.performance.gpuRt = {};
        context.capabilities.performance.gpuRt.status =
            "Disabled for matched benchmark A/B; RT rendering is unchanged.";
        gRtLabGpuFrameTimeMs.store(0.0f, std::memory_order_release);
        gRtLabGpuSampleCount.store(0u, std::memory_order_release);
        return;
    }
    const horde::vulkan::GpuFrameTimerTelemetry& timer = context.gpuFrameTimer.Telemetry();
    auto& output = context.capabilities.performance.gpuRt;
    output.status = timer.diagnostic;
    output.supported = context.gpuFrameTimer.Supported();
    output.valid = context.gpuFrameTimingSampleCount > 0u &&
        horde::vulkan::GpuFrameTimerHasCurrentSample(timer.status);
    output.latestMs = output.valid ? static_cast<float>(timer.latestMilliseconds) : 0.0f;
    output.averageMs = output.valid
        ? static_cast<float>(context.gpuFrameTimingTotalMs / static_cast<double>(context.gpuFrameTimingSampleCount))
        : 0.0f;
    output.timestampPeriodNanoseconds = timer.timestampPeriodNanoseconds;
    output.timestampValidBits = timer.timestampValidBits;
    output.sampleCount = context.gpuFrameTimingSampleCount;
    output.unavailableCount = timer.unavailableResultCount;
    output.errorCount = timer.errorCount;
    gRtLabGpuFrameTimeMs.store(output.valid ? output.latestMs : 0.0f, std::memory_order_release);
    gRtLabGpuSampleCount.store(output.sampleCount, std::memory_order_release);
}

#if HORDE_RT_ANDROID_MOTION_EVIDENCE
#include "android_motion_evidence.inl"
#endif

bool CompleteRtEvidenceAfterDeviceIdle(
    SwapchainContext& context,
    const VkResult idleResult)
{
    if (!context.rtFrameEvidenceInitialised)
    {
        return idleResult == VK_SUCCESS;
    }
    if (idleResult != VK_SUCCESS)
    {
        context.rtFrameEvidence.NoteFailedDeviceIdle();
        __android_log_print(
            ANDROID_LOG_ERROR, kTag,
            "vkDeviceWaitIdle failed before RT evidence completion: %d",
            static_cast<int>(idleResult));
        return false;
    }

    const horde::vulkan::raytracing::RtGpuFrameTimerIo gpuIo =
        horde::vulkan::raytracing::MakeRtGpuFrameTimerIo(context.gpuFrameTimer);
    const horde::vulkan::raytracing::RtDiagnosticFrameIo diagnosticIo =
        horde::vulkan::raytracing::MakeRtDiagnosticFrameIo(context.rtScene);
    bool completed = true;
    for (std::uint32_t frameSlot = 0u; frameSlot < kMaxFramesInFlight; ++frameSlot)
    {
        horde::telemetry::RtPerformanceEvidenceSnapshot completedSnapshot{};
        const horde::vulkan::raytracing::RtFrameEvidenceCompletionResult result =
            context.rtFrameEvidence.CompleteFinalIdle(
                frameSlot, gpuIo, diagnosticIo, &completedSnapshot);
#if HORDE_RT_STAGED_PRIMARY_TIMING
        CollectStagedPassTiming(context, frameSlot, result, completedSnapshot);
#endif
        if (result.gpuCollectionAttempted)
        {
            RefreshGpuTimingTelemetry(context, &result.gpuCollection);
        }
        DeliverCompletedBenchmarkEvidence(context, result, completedSnapshot);
#if HORDE_RT_ANDROID_MOTION_EVIDENCE
        ObserveAndroidMotionCompletion(context, result.completedEvidence);
#endif
        if (result.fatalDiagnosticIoFailure)
        {
            __android_log_print(
                ANDROID_LOG_ERROR, kTag,
                "Failed to read the completed RT Diagnostic buffer after device idle.");
            completed = false;
        }
    }
    return completed;
}

bool FinalizeCompletedInAppBenchmark(SwapchainContext& context)
{
    const VkResult idleResult = vkDeviceWaitIdle(context.device);
    const bool drained = CompleteRtEvidenceAfterDeviceIdle(context, idleResult);
    if (context.benchmarkEvidence.Status() != horde::telemetry::RtBenchmarkRunStatus::Measuring)
    {
        return drained;
    }
    const bool drainRecorded = context.benchmarkEvidence.RecordOwnerDrainResult(
        idleResult == VK_SUCCESS && drained);
    const bool finalized = context.benchmarkEvidence.Finalize();
    return drained && drainRecorded && finalized;
}

void FailInAppBenchmarkAfterRenderFailure(SwapchainContext& context)
{
    const bool active = context.inAppBenchmark.IsRunning() ||
        context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Allocated ||
        context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Measuring;
    if (!active)
    {
        return;
    }

    const VkResult idleResult = vkDeviceWaitIdle(context.device);
    const bool drained = CompleteRtEvidenceAfterDeviceIdle(context, idleResult);
    if (context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Measuring)
    {
        (void)context.benchmarkEvidence.RecordOwnerDrainResult(
            idleResult == VK_SUCCESS && drained);
        (void)context.benchmarkEvidence.Finalize();
    }
    CancelActiveInAppBenchmark(context);
    FinishInAppBenchmark(context);
}

horde::telemetry::RtSampleStatus CurrentInitialGpuEvidenceStatus(
    SwapchainContext& context)
{
    const horde::vulkan::raytracing::RtGpuFrameTimerIo gpuIo =
        horde::vulkan::raytracing::MakeRtGpuFrameTimerIo(context.gpuFrameTimer);
    const horde::vulkan::raytracing::RtGpuTimerStateSnapshot timer =
        gpuIo.snapshot != nullptr ? gpuIo.snapshot(gpuIo.user)
                                  : horde::vulkan::raytracing::RtGpuTimerStateSnapshot{};
    return horde::vulkan::raytracing::InitialRtGpuEvidenceStatus(
        context.gpuFrameTimingEnabled, timer);
}

bool InitialiseRtEvidenceOnOwnerThread(
    SwapchainContext& context,
    const bool initiallyPaused)
{
    if (!context.useRtPath || context.rtFrameEvidenceInitialised)
    {
        return true;
    }
    const horde::telemetry::RtInstrumentationMode instrumentation =
        context.rtScene.DiagnosticsAvailability() ==
                horde::vulkan::raytracing::RtDiagnosticAvailability::Available
            ? horde::telemetry::RtInstrumentationMode::Diagnostic
            : horde::telemetry::RtInstrumentationMode::Shipping;
    if (!context.rtFrameEvidence.Initialise(
            LoadPreservedRtEvidenceSeeds(),
            kMaxFramesInFlight,
            instrumentation,
            CurrentInitialGpuEvidenceStatus(context),
            initiallyPaused))
    {
        return false;
    }
    context.rtFrameEvidenceInitialised = true;
    return true;
}

void DestroyRtEvidenceOnOwnerThread(SwapchainContext& context)
{
    if (!context.rtFrameEvidenceInitialised)
    {
        return;
    }
    const bool destroyed = context.rtFrameEvidence.Destroy();
    context.capabilities.rtScene.presented = false;
    context.capabilities.rtScene.status = "RT surface stopped";
    context.capabilities.rtScene.dispatchWidth = 0u;
    context.capabilities.rtScene.dispatchHeight = 0u;
    context.capabilities.performance = {};
    const auto stoppedPublication = context.rtFrameEvidence.PublishedStateByValue();
    // A successful Destroy yields one final accepted not-running publication.
    PublishRuntimeReports(context, destroyed ? &stoppedPublication : nullptr);
    PreserveRtEvidenceSeeds(context.rtFrameEvidence.SeedsByValue());
    context.rtFrameEvidenceInitialised = false;
}

bool InitialiseRtSceneForSwapchain(SwapchainContext& context)
{
    if (!context.useRtPath)
    {
        return true;
    }

    const VkExtent2D renderExtent = ScaledRenderExtent(context.swapchainExtent, context.renderScale);
    std::string diagnostic;
    const bool initialised = context.rtScene.Initialise(context.instance,
                                    context.physicalDevice,
                                    context.device,
                                    context.graphicsQueue,
                                    context.commandPool,
                                    renderExtent,
                                    context.swapchainFormat,
                                    context.reportDirectory + "/../skeleton_biped_merged_animations_v01.glb",
                                    context.reportDirectory + "/../lich_placeholder_merged_animations_v01.glb",
                                    context.reportDirectory + "/..",
                                    context.reportDirectory + "/..",
                                    diagnostic,
                                    {},
                                    context.reportDirectory + "/..",
                                    context.executionBackend, context.sceneProfile, context.graphicsSettings.glassEnabled,
                                    context.pipelineCache,
                                    context.compiledPipelineCache != nullptr
                                        ? &context.compiledPipelineCache->cache : nullptr);
    // Bounded per-attempt CPU evidence; never emit frame-by-frame timings.
    // Log each attempt here so a subsequent rollback cannot overwrite it.
    const auto& measurements = context.rtScene.InitialiseMeasurements();
    const bool compiledPairReused = context.rtScene.ReusedCompiledPipelines();
    const std::string compiledPair = context.rtScene.SelectedPipelineBundleIdentity();
    __android_log_print(ANDROID_LOG_INFO, kTag,
        "HORDE_RT_INIT serial=%llu generation=%llu profile=%d backend=%d glass=%d success=%d compiled_pair_reused=%d compiled_pair=%s compiled_cache_entries=%zu total_cpu_wall_ms=%.3f",
        static_cast<unsigned long long>(context.graphicsSerial),
        static_cast<unsigned long long>(context.surfaceGeneration), static_cast<int>(context.sceneProfile),
        static_cast<int>(context.executionBackend), context.graphicsSettings.glassEnabled ? 1 : 0,
        initialised ? 1 : 0, compiledPairReused ? 1 : 0,
        compiledPair.empty() ? "none" : compiledPair.c_str(),
        context.compiledPipelineCache != nullptr
            ? context.compiledPipelineCache->cache.Size() : 0u,
        static_cast<double>(measurements.totalCpuNanoseconds) * 1.0e-6);
    for (const auto& stage : measurements.stages)
        __android_log_print(ANDROID_LOG_INFO, kTag,
            "HORDE_RT_INIT_STAGE serial=%llu generation=%llu name=%.*s attempted=%d success=%d cpu_wall_ms=%.3f",
            static_cast<unsigned long long>(context.graphicsSerial),
            static_cast<unsigned long long>(context.surfaceGeneration),
            static_cast<int>(stage.name.size()), stage.name.data(), stage.attempted ? 1 : 0,
            stage.succeeded ? 1 : 0, static_cast<double>(stage.cpuNanoseconds) * 1.0e-6);
    for (const auto& pipeline : measurements.pipelines)
        __android_log_print(ANDROID_LOG_INFO, kTag,
            "HORDE_RT_INIT_PIPELINE serial=%llu generation=%llu strategy=%.*s backend=%d result=%d attempted=%d cache_null=%d cpu_wall_ms=%.3f",
            static_cast<unsigned long long>(context.graphicsSerial),
            static_cast<unsigned long long>(context.surfaceGeneration),
            static_cast<int>(pipeline.strategy.size()), pipeline.strategy.data(), static_cast<int>(pipeline.backend),
            static_cast<int>(pipeline.result), pipeline.attempted ? 1 : 0,
            pipeline.pipelineCacheWasNull ? 1 : 0, static_cast<double>(pipeline.cpuNanoseconds) * 1.0e-6);
    for (const auto& table : measurements.shaderBindingTables)
        __android_log_print(ANDROID_LOG_INFO, kTag,
            "HORDE_RT_INIT_SBT serial=%llu generation=%llu strategy=%.*s attempted=%d success=%d cpu_wall_ms=%.3f",
            static_cast<unsigned long long>(context.graphicsSerial),
            static_cast<unsigned long long>(context.surfaceGeneration),
            static_cast<int>(table.strategy.size()), table.strategy.data(), table.attempted ? 1 : 0,
            table.succeeded ? 1 : 0, static_cast<double>(table.cpuNanoseconds) * 1.0e-6);
    if (!initialised)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to initialise presentable RT scene: %s", diagnostic.c_str());
        return false;
    }
    if (context.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview &&
        !context.rtScene.ConfigurePreviewFireSockets(context.previewSession, diagnostic))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Preview production sockets unavailable: %s", diagnostic.c_str());
        return false;
    }
    if (context.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::EntryMenu &&
        !context.rtScene.ConfigureEntryMenu(context.entrySession, diagnostic))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Entry lantern sockets unavailable: %s", diagnostic.c_str());
        return false;
    }
    if (context.gpuFrameTimingEnabled &&
        context.gpuFrameTimer.Telemetry().status == horde::vulkan::GpuFrameTimerStatus::Uninitialised)
    {
        context.gpuFrameTimer.Initialise(
            context.physicalDevice, context.device, context.graphicsQueueFamilyIndex, kMaxFramesInFlight);
    }
#if HORDE_RT_STAGED_PRIMARY_TIMING
    if (context.stagedPassTimer.Status() == horde::vulkan::raytracing::experimental::StagedPrimaryTimingInitStatus::Uninitialised)
        context.stagedPassTimer.Initialise(context.physicalDevice, context.device, context.graphicsQueueFamilyIndex, kMaxFramesInFlight);
#endif
    RefreshGpuTimingTelemetry(context);
    __android_log_print(ANDROID_LOG_INFO, kTag, "PBR material encoding: %s", context.rtScene.MaterialEncoding().c_str());
    __android_log_print(ANDROID_LOG_INFO,
                        kTag,
                        "RT render scale %.0f%%: %ux%u -> %ux%u",
                        static_cast<double>(context.renderScale * 100.0f),
                        renderExtent.width,
                        renderExtent.height,
                        context.swapchainExtent.width,
                        context.swapchainExtent.height);

    const VkExtent2D dispatchExtent = context.rtScene.DispatchExtent();
    __android_log_print(ANDROID_LOG_INFO, kTag,
        "HORDE_SURFACE_PRESENTATION generation=%llu window=%ux%u current_extent=%ux%u current_transform=0x%08x supported_transforms=0x%08x pre_transform=0x%08x swapchain=%ux%u rt_dispatch=%ux%u rt_pre_rotation_required=%d",
        static_cast<unsigned long long>(context.surfaceGeneration),
        context.nativeWindowExtent.width, context.nativeWindowExtent.height,
        context.surfaceCurrentExtent.width, context.surfaceCurrentExtent.height,
        static_cast<unsigned int>(context.surfaceCurrentTransform),
        static_cast<unsigned int>(context.surfaceSupportedTransforms),
        static_cast<unsigned int>(context.swapchainPreTransform),
        context.swapchainExtent.width, context.swapchainExtent.height,
        dispatchExtent.width, dispatchExtent.height,
        context.rtPreRotationRequired ? 1 : 0);

    return true;
}

bool RecreateSwapchain(SwapchainContext& context)
{
    context.entryHandoff.BeginLoad();
    PublishEntryState(context, false);
    if (!ReleaseSwapchainResources(context))
    {
        context.entryHandoff.FailLoad();
        PublishEntryState(context, false);
        return false;
    }
    const bool restored = CreateSwapchain(context) && InitialiseRtSceneForSwapchain(context);
    if (restored) context.entryHandoff.EndLoad();
    else context.entryHandoff.FailLoad();
    context.entryWarmFrames = kMaxFramesInFlight;
    PublishEntryState(context, false);
    return restored;
}

horde::vulkan::PipelineCacheSeed::DeviceIdentity PipelineCacheDeviceIdentity(
    const VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    horde::vulkan::PipelineCacheSeed::DeviceIdentity identity;
    identity.vendorId = properties.vendorID;
    identity.deviceId = properties.deviceID;
    std::copy_n(properties.pipelineCacheUUID, identity.pipelineCacheUuid.size(),
                identity.pipelineCacheUuid.begin());
    return identity;
}

void RetainPipelineCacheSeedAfterIdle(const SwapchainContext& context)
{
    // Called only after successful device idle, presentation retirement and
    // compiled-pipeline lease retirement. Vulkan returns the exact opaque
    // bytes accepted by the next compatible vkCreatePipelineCache.
    // https://docs.vulkan.org/refpages/latest/refpages/source/vkGetPipelineCacheData.html
    if (context.pipelineCache == VK_NULL_HANDLE) return;
    const auto started = std::chrono::steady_clock::now();
    std::size_t requestedBytes = 0u;
    const VkResult queryResult = vkGetPipelineCacheData(
        context.device, context.pipelineCache, &requestedBytes, nullptr);
    VkResult readResult = VK_NOT_READY;
    std::size_t returnedBytes = 0u;
    bool admitted = false;
    if (queryResult == VK_SUCCESS &&
        requestedBytes >= horde::vulkan::PipelineCacheSeed::kHeaderSize &&
        requestedBytes <= horde::vulkan::PipelineCacheSeed::kMaximumSize)
    {
        try
        {
            std::vector<std::byte> data(requestedBytes);
            returnedBytes = requestedBytes;
            readResult = vkGetPipelineCacheData(
                context.device, context.pipelineCache, &returnedBytes, data.data());
            if (readResult == VK_SUCCESS && returnedBytes <= requestedBytes)
            {
                data.resize(returnedBytes);
                admitted = gPipelineCacheSeed.ReplaceFromDriverData(
                    PipelineCacheDeviceIdentity(context.physicalDevice), data, true, false);
            }
        }
        catch (...)
        {
            // Optional cache allocation/query failure never blocks retirement.
            readResult = VK_ERROR_OUT_OF_HOST_MEMORY;
        }
    }
    const auto elapsed = std::chrono::steady_clock::now() - started;
    __android_log_print(ANDROID_LOG_INFO, kTag,
        "HORDE_PIPELINE_CACHE_SEED_STORE generation=%llu query_result=%d read_result=%d "
        "requested_bytes=%zu returned_bytes=%zu admitted=%d retained_bytes=%zu cpu_wall_ms=%.3f",
        static_cast<unsigned long long>(context.surfaceGeneration), static_cast<int>(queryResult),
        static_cast<int>(readResult), requestedBytes, returnedBytes, admitted ? 1 : 0,
        gPipelineCacheSeed.Size(), std::chrono::duration<double, std::milli>(elapsed).count());
}

bool DestroySwapchainContext(SwapchainContext& context)
{
    if (context.device == VK_NULL_HANDLE)
    {
        if (context.compiledPipelineCache != nullptr &&
            !context.compiledPipelineCache->cache.Empty())
        {
            __android_log_print(ANDROID_LOG_ERROR, kTag,
                "Compiled pipeline cache still owns Vulkan objects without a device; resources retained.");
            return false;
        }
        context.compiledPipelineCache.reset();
        DestroyRtEvidenceOnOwnerThread(context);
        if (context.surface != VK_NULL_HANDLE && context.instance != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
        }
        if (context.instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(context.instance, nullptr);
        }
        if (context.window != nullptr)
        {
            ANativeWindow_release(context.window);
            context.window = nullptr;
        }
        context = {};
        return true;
    }

    if (!ConsumePendingImageAcquire(context))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag,
                            "Acquired image drain failed; renderer resources retained.");
        return false;
    }
    const VkResult idleResult = vkDeviceWaitIdle(context.device);
    (void)CompleteRtEvidenceAfterDeviceIdle(context, idleResult);
    if (idleResult != VK_SUCCESS || context.presentCompletionFences.Drain() != VK_SUCCESS ||
        !context.presentCompletionFences.DestroyCompleted())
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag,
            "Presentation retirement could not be proved; renderer resources retained.");
        return false;
    }
    CancelActiveInAppBenchmark(context);
    DestroyRtEvidenceOnOwnerThread(context);
    context.rtScene.Destroy();
    if (context.compiledPipelineCache != nullptr)
    {
        if (!context.compiledPipelineCache->cache.DestroyAfterDeviceIdle(true))
        {
            __android_log_print(ANDROID_LOG_ERROR, kTag,
                "Compiled pipeline cache retirement failed after scene lease release; device resources retained.");
            return false;
        }
        context.compiledPipelineCache.reset();
    }
    if (context.pipelineCache != VK_NULL_HANDLE)
    {
        RetainPipelineCacheSeedAfterIdle(context);
        vkDestroyPipelineCache(context.device, context.pipelineCache, nullptr);
        context.pipelineCache = VK_NULL_HANDLE;
    }
    context.gpuFrameTimer.Destroy();
#if HORDE_RT_STAGED_PRIMARY_TIMING
    context.stagedPassTimer.Destroy();
#endif
    for (VkSemaphore semaphore : context.imageAvailableSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(context.device, semaphore, nullptr);
        }
    }
    for (VkSemaphore semaphore : context.renderFinishedSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(context.device, semaphore, nullptr);
        }
    }

    for (VkFence fence : context.inFlightFences)
    {
        if (fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(context.device, fence, nullptr);
        }
    }

    if (context.commandPool != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(context.device, context.commandPool, static_cast<uint32_t>(context.commandBuffers.size()), context.commandBuffers.data());
        vkDestroyCommandPool(context.device, context.commandPool, nullptr);
    }
    for (VkFramebuffer framebuffer : context.swapchainFramebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(context.device, framebuffer, nullptr);
        }
    }
    for (VkImageView imageView : context.swapchainImageViews)
    {
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(context.device, imageView, nullptr);
        }
    }
    if (context.renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(context.device, context.renderPass, nullptr);
    }
    if (context.swapchain != VK_NULL_HANDLE)
    {
#if HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION
        PollPresentTimingOnOwner(context);
        if (context.presentTiming) context.presentTiming->UnbindSwapchain();
        (void)WriteTextFile(context.reportDirectory + "/HordeLanternRT-present-timing-latest.json", PresentTimingJson(context));
#endif
        vkDestroySwapchainKHR(context.device, context.swapchain, nullptr);
    }
    vkDestroyDevice(context.device, nullptr);
    if (context.surface != VK_NULL_HANDLE && context.instance != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
    }
    if (context.instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(context.instance, nullptr);
    }
    if (context.window != nullptr)
    {
        ANativeWindow_release(context.window);
        context.window = nullptr;
    }

    context = {};
    return true;
}

bool RenderFrame(SwapchainContext& context, bool& rtFramePresented, bool& resourceRecreated)
{
    const auto frameStart = std::chrono::steady_clock::now();
    const auto frameStartCount = std::chrono::duration_cast<std::chrono::nanoseconds>(
        frameStart.time_since_epoch()).count();
    const std::uint64_t frameStartNanoseconds = frameStartCount < 0
        ? 0u
        : static_cast<std::uint64_t>(frameStartCount);
    rtFramePresented = false;
    resourceRecreated = false;
    if (!gSurfaceSessions.IsCurrent(context.surfaceGeneration)) return false;
    if (context.commandBuffers.empty())
    {
        return false;
    }

    const bool useRtFrame = context.useRtPath && context.rtScene.IsReady();
    horde::vulkan::raytracing::RtSceneRecordObservation observation{};
    const bool evidenceFrame = useRtFrame && context.rtFrameEvidenceInitialised &&
        context.rtFrameEvidence.BeginFrame(context.currentFrame, observation);
    horde::vulkan::raytracing::RtSceneStageScope wholeFrameScope(
        evidenceFrame ? &observation : nullptr,
        horde::telemetry::RtStage::WholeFrameCycle,
        frameStartNanoseconds);
    horde::vulkan::raytracing::RtSceneStageScope fenceScope(
        evidenceFrame ? &observation : nullptr,
        horde::telemetry::RtStage::FrameFenceWait);
    const VkResult waitResult = vkWaitForFences(
        context.device, 1u, &context.inFlightFences[context.currentFrame],
        VK_TRUE, UINT64_MAX);
    fenceScope.Complete(1u, 0u, 1u);
    const auto fenceDone = std::chrono::steady_clock::now();
    if (!gSurfaceSessions.IsCurrent(context.surfaceGeneration))
    {
        if (evidenceFrame) context.rtFrameEvidence.AbortFrame();
        return false;
    }
    if (waitResult != VK_SUCCESS)
    {
        if (evidenceFrame)
        {
            context.rtFrameEvidence.AbortFrame();
        }
        return false;
    }

    if (evidenceFrame)
    {
        horde::telemetry::RtPerformanceEvidenceSnapshot completedSnapshot{};
        const horde::vulkan::raytracing::RtFrameEvidenceCompletionResult completion =
            context.rtFrameEvidence.CompleteFence(
                context.currentFrame,
                horde::vulkan::raytracing::MakeRtGpuFrameTimerIo(
                    context.gpuFrameTimer),
                horde::vulkan::raytracing::MakeRtDiagnosticFrameIo(context.rtScene),
                &completedSnapshot);
#if HORDE_RT_STAGED_PRIMARY_TIMING
        CollectStagedPassTiming(context, context.currentFrame, completion, completedSnapshot);
#endif
        RefreshGpuTimingTelemetry(
            context,
            completion.gpuCollectionAttempted ? &completion.gpuCollection : nullptr);
        DeliverCompletedBenchmarkEvidence(context, completion, completedSnapshot);
#if HORDE_RT_ANDROID_MOTION_EVIDENCE
        ObserveAndroidMotionCompletion(context, completion.completedEvidence);
#endif
        if (completion.fatalDiagnosticIoFailure)
        {
            __android_log_print(
                ANDROID_LOG_ERROR, kTag,
                "Failed to read the completed RT Diagnostic buffer after its owning fence.");
            context.rtFrameEvidence.AbortFrame();
            return false;
        }
    }

    uint32_t imageIndex = 0u;
    horde::vulkan::raytracing::RtSceneStageScope acquireScope(
        evidenceFrame ? &observation : nullptr,
        horde::telemetry::RtStage::ImageAcquire);
    const VkResult acquireResult = vkAcquireNextImageKHR(
        context.device,
        context.swapchain,
        UINT64_MAX,
        context.imageAvailableSemaphores[context.currentFrame],
        VK_NULL_HANDLE,
        &imageIndex);
    acquireScope.Complete(1u, 0u, 1u);
    context.imageAcquirePending = acquireResult == VK_SUCCESS || acquireResult == VK_SUBOPTIMAL_KHR;
    if (!gSurfaceSessions.IsCurrent(context.surfaceGeneration))
    {
        if (evidenceFrame) context.rtFrameEvidence.AbortFrame();
        return false;
    }

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        if (evidenceFrame)
        {
            context.rtFrameEvidence.AbortFrame();
        }
        resourceRecreated = true;
        return RecreateSwapchain(context);
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
    {
        if (evidenceFrame)
        {
            context.rtFrameEvidence.AbortFrame();
        }
        return false;
    }

    VkCommandBufferResetFlags resetFlags = 0;
    if (vkResetCommandBuffer(context.commandBuffers[imageIndex], resetFlags) != VK_SUCCESS)
    {
        if (evidenceFrame)
        {
            context.rtFrameEvidence.AbortFrame();
        }
        return false;
    }

    const VkCommandBufferBeginInfo beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr};

    if (vkBeginCommandBuffer(context.commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
    {
        if (evidenceFrame)
        {
            context.rtFrameEvidence.AbortFrame();
        }
        return false;
    }

    bool gpuTimingRecording = false;
#if HORDE_RT_STAGED_PRIMARY_TIMING
    horde::vulkan::raytracing::experimental::StagedPrimaryRecordingGuard stagedRecordingGuard(context.stagedPassTimer, context.currentFrame);
#endif
    bool inAppBenchmarkFrame = false;
    const auto recordStart = std::chrono::steady_clock::now();
    if (useRtFrame)
    {
        if (context.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::Showcase)
        {
            if (gInAppBenchmarkRequested.exchange(false, std::memory_order_acq_rel))
            {
                StartInAppBenchmark(context);
            }
            if (gInAppBenchmarkCancelRequested.exchange(false, std::memory_order_acq_rel) &&
                (context.inAppBenchmark.IsRunning() ||
                 context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Allocated ||
                 context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Measuring
#if HORDE_RT_ANDROID_MOTION_VALIDATION
                 || context.motionValidationRun
#endif
                 ))
            {
                CancelActiveInAppBenchmark(context);
                if (context.rtFrameEvidenceInitialised)
                {
                    (void)context.rtFrameEvidence.ApplyEvent(
                        horde::telemetry::RtLifecycleEvent::RouteReset);
                }
                {
                    std::lock_guard<std::mutex> lock(gReportMutex);
                    gLatestBenchmarkProgress = "BENCHMARK CANCELLED";
                }
                gInAppBenchmarkStatus.store(3, std::memory_order_release);
            }

            const std::int32_t requestedCheckpoint =
                gBenchmarkCheckpointRequested.exchange(-1, std::memory_order_acq_rel);
            const std::int32_t requestedCaptureCheckpoint =
                gCaptureCheckpointRequested.exchange(-1, std::memory_order_acq_rel);
            if (!context.inAppBenchmark.IsRunning())
            {
                DebugCheckpointSelection selection;
                if (ResolveDebugCheckpoint(requestedCaptureCheckpoint, selection))
                {
                    ApplyCaptureCheckpoint(
                        context, selection, evidenceFrame ? &observation : nullptr);
                }
                else if (ResolveDebugCheckpoint(requestedCheckpoint, selection))
                {
                    ApplyBenchmarkCheckpoint(
                        context, selection, evidenceFrame ? &observation : nullptr);
                }
                if (gRouteReplayRequested.exchange(false, std::memory_order_acq_rel))
                {
                    ApplyRouteReplay(context);
                }
            }
            else
            {
                gRouteReplayRequested.store(false, std::memory_order_release);
                gCaptureCheckpointRequested.store(-1, std::memory_order_release);
            }

            // Accepted Stop and world commands have one publication/admission
            // order. Do not hold this mutex through normal ticks or GPU work.
            std::unique_lock<std::mutex> worldCommandAdmission(gInputPublisherMutex);
            (void)SynchronizeLifecyclePauseOnOwnerThreadLocked();
            if (!gSurfaceSessions.IsCurrent(context.surfaceGeneration))
            {
                if (evidenceFrame) context.rtFrameEvidence.AbortFrame();
                return false;
            }
            const horde::gameplay::simulation::PublishedInput publishedInput =
                gInputMailbox.ConsumeLatest();
            // Same native steady-clock domain as JNI receipt edges. Take this
            // cutoff while publisher admission is locked; later edges belong
            // to a later coherent publication, never an earlier catch-up tick.
            const std::uint64_t inputOwnerSteadyNs =
                horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr);
            horde::gameplay::simulation::InputSnapshot simulationInput = publishedInput.snapshot;
            gGameSimulation.SetPresentationAspect(
                horde::graphics::RtViewAspectFromImageExtent(
                    context.swapchainExtent.width, context.swapchainExtent.height,
                    context.rtPresentationTransform));
#if HORDE_RT_ANDROID_MOTION_EVIDENCE
            BuildAndroidMotionInput(context, simulationInput, inputOwnerSteadyNs);
#endif
            const horde::gameplay::simulation::SimulationSnapshot& beforeCommands = gGameSimulation.Snapshot();
            const bool routeResetPending =
                simulationInput.commands.routeReset > beforeCommands.lastConsumedRouteResetSequence;
            const bool retryPending =
                simulationInput.commands.retry > beforeCommands.lastConsumedRetrySequence;
            if (routeResetPending)
            {
                if (context.inAppBenchmark.IsRunning() ||
                    context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Allocated ||
                    context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Measuring)
                {
                    CancelActiveInAppBenchmark(context);
                }
                context.activeBenchmarkCheckpoint = -1;
                context.benchmarkSampling = false;
                context.routeReplayActive = false;
                context.captureActive = false;
                context.capturePresentedFrames = 0u;
            }
            if (context.rtFrameEvidenceInitialised && routeResetPending)
            {
                (void)context.rtFrameEvidence.ApplyEvent(
                    horde::telemetry::RtLifecycleEvent::RouteReset);
            }
            if (context.rtFrameEvidenceInitialised && retryPending &&
                context.inAppBenchmark.IsRunning())
            {
                CancelBenchmarkEvidenceOnly(context);
            }
            else if (context.rtFrameEvidenceInitialised && retryPending)
            {
                (void)context.rtFrameEvidence.ApplyEvent(
                    horde::telemetry::RtLifecycleEvent::Retry);
            }

            // World commands remain responsive while menus/death pause fixed time.
            // Reset/retry wins this zero-delta loop; the shared paused AdvanceFrame
            // path consumes every unavailable action edge without buffering it.
            for (std::size_t command = 0u; command < 128u; ++command)
            {
                const horde::gameplay::simulation::SimulationSnapshot& snapshot = gGameSimulation.Snapshot();
                if (snapshot.lastConsumedRouteResetSequence >= simulationInput.commands.routeReset &&
                    snapshot.lastConsumedRetrySequence >= simulationInput.commands.retry)
                {
                    break;
                }
                horde::vulkan::raytracing::RtSceneStageScope simulationScope(
                    evidenceFrame ? &observation : nullptr,
                    horde::telemetry::RtStage::SimulationStep);
                gGameSimulation.StepFixed(simulationInput, 0.0f, publishedInput.publicationSequence);
                simulationScope.Complete(1u);
            }
            const auto& admittedCommands = gGameSimulation.Snapshot();
            const bool worldCommandsDeferred =
                admittedCommands.lastConsumedRouteResetSequence < simulationInput.commands.routeReset ||
                admittedCommands.lastConsumedRetrySequence < simulationInput.commands.retry;
            worldCommandAdmission.unlock();
#if HORDE_RT_ANDROID_MOTION_EVIDENCE
            UpdateAndroidMotionScope(context, simulationInput);
#endif

            const bool simulationPaused = context.captureActive || simulationInput.paused;
            const bool scriptedMotion =
#if HORDE_RT_ANDROID_MOTION_EVIDENCE
                context.motion && context.motion->armed && !context.motion->finished;
#else
                false;
#endif
            if (!scriptedMotion) simulationInput.damageEnabled = !simulationPaused && !context.inAppBenchmark.IsRunning() &&
                !context.routeReplayActive && !context.benchmarkSampling && !context.captureActive;
            inAppBenchmarkFrame = context.inAppBenchmark.IsRunning() && !worldCommandsDeferred;
            if (inAppBenchmarkFrame)
            {
                context.frameDeltaSeconds = 1.0f / 60.0f;
                const horde::gameplay::ShowcaseBenchmarkAdvance advance = context.inAppBenchmark.Advance();
                if (advance.lapStarted)
                {
                    ResetShowcaseSimulation();
                    if (context.rtFrameEvidenceInitialised)
                    {
                        const bool routeResetApplied = context.rtFrameEvidence.ApplyEvent(
                            horde::telemetry::RtLifecycleEvent::RouteReset);
                        const bool finalLap = context.inAppBenchmark.CurrentLap() ==
                            context.inAppBenchmark.TotalLaps();
                        const bool warmupApplied = !finalLap || context.rtFrameEvidence.ApplyEvent(
                            horde::telemetry::RtLifecycleEvent::WarmupToMeasure);
                        if (finalLap && routeResetApplied && warmupApplied &&
                            context.benchmarkEvidence.Status() ==
                                horde::telemetry::RtBenchmarkRunStatus::Allocated)
                        {
                            const horde::telemetry::RtLifecycleSeeds seeds =
                                context.rtFrameEvidence.SeedsByValue();
                            if (context.benchmarkEvidence.ArmMeasurement(
                                    seeds.sceneEpoch, seeds.measurementGeneration))
                                CaptureBenchmarkSummaryStartOnOwner(context);
                        }
                    }
                }
                if (context.rtFrameEvidenceInitialised &&
                    context.benchmarkEvidence.Status() ==
                        horde::telemetry::RtBenchmarkRunStatus::Allocated &&
                    context.inAppBenchmark.TotalLaps() == 1u &&
                    !advance.lapStarted &&
                    context.inAppBenchmark.CurrentLap() == context.inAppBenchmark.TotalLaps())
                {
                    const bool warmupApplied = context.rtFrameEvidence.ApplyEvent(
                        horde::telemetry::RtLifecycleEvent::WarmupToMeasure);
                    if (warmupApplied)
                    {
                        const horde::telemetry::RtLifecycleSeeds seeds =
                            context.rtFrameEvidence.SeedsByValue();
                        if (context.benchmarkEvidence.ArmMeasurement(
                                seeds.sceneEpoch, seeds.measurementGeneration))
                            CaptureBenchmarkSummaryStartOnOwner(context);
                    }
                    else
                    {
                        CancelBenchmarkEvidenceOnly(context);
                    }
                }
                const bool lanternBenchmark =
                    horde::gameplay::IsLanternBenchmark(context.inAppBenchmark.Workload());
                if (lanternBenchmark && advance.frameInLap == 1u &&
                    !horde::gameplay::StageLanternBenchmark(
                        gGameSimulation, context.inAppBenchmark.Workload()))
                {
                    CancelActiveInAppBenchmark(context);
                }
                simulationInput.paused = false;
                simulationInput.damageEnabled = false;
                simulationInput.hasAuthoritativePlayerPose = true;
                simulationInput.authoritativePlayerX = advance.replay.x;
                simulationInput.authoritativePlayerZ = advance.replay.z;
                simulationInput.yawRadians = advance.replay.yaw;
                simulationInput.pitchRadians = lanternBenchmark
                    ? horde::gameplay::kLanternBenchmarkPitch : -0.04f;
                horde::vulkan::raytracing::RtSceneStageScope simulationScope(
                    evidenceFrame ? &observation : nullptr,
                    horde::telemetry::RtStage::SimulationStep);
                if (lanternBenchmark &&
                    horde::gameplay::IsFrozenBenchmark(context.inAppBenchmark.Workload()))
                {
                    gGameSimulation.AdvanceFrame(
                        simulationInput, 0.0, publishedInput.publicationSequence);
                }
                else if (lanternBenchmark)
                {
                    gGameSimulation.AdvanceFrame(
                        simulationInput,
                        horde::gameplay::simulation::FixedStepRunner::kFixedDeltaSeconds,
                        publishedInput.publicationSequence);
                }
                else
                {
                    gGameSimulation.StepFixed(
                        simulationInput,
                        static_cast<float>(horde::gameplay::simulation::FixedStepRunner::kFixedDeltaSeconds),
                        publishedInput.publicationSequence);
                }
                simulationScope.Complete(1u);
                if (context.benchmarkEvidence.Status() ==
                        horde::telemetry::RtBenchmarkRunStatus::Measuring)
                {
                    context.benchmarkExpectedFrame = context.benchmarkEvidence.ExpectFrame({
                        static_cast<std::uint32_t>(advance.replay.zone),
                        context.inAppBenchmark.CurrentLap()});
                }
                if (advance.replay.waypointReached || advance.lapStarted || advance.finished ||
                    (lanternBenchmark && (advance.frameInLap == 1u || advance.frameInLap % 60u == 0u)))
                {
                    PublishBenchmarkProgress(context);
                }
            }
            else if (!worldCommandsDeferred && context.routeReplayActive)
            {
                // Debug route replay owns an authoritative pose and fixed step. It
                // must keep advancing even if a menu/death overlay published a
                // paused input snapshot immediately before the automation intent.
                const horde::gameplay::ShowcaseReplaySnapshot& replay = context.routeReplay.Update();
                simulationInput.paused = false;
                simulationInput.damageEnabled = false;
                simulationInput.hasAuthoritativePlayerPose = true;
                simulationInput.authoritativePlayerX = replay.x;
                simulationInput.authoritativePlayerZ = replay.z;
                simulationInput.yawRadians = replay.yaw;
                simulationInput.pitchRadians = -0.04f;
                horde::vulkan::raytracing::RtSceneStageScope simulationScope(
                    evidenceFrame ? &observation : nullptr,
                    horde::telemetry::RtStage::SimulationStep);
                gGameSimulation.StepFixed(
                    simulationInput,
                    static_cast<float>(horde::gameplay::simulation::FixedStepRunner::kFixedDeltaSeconds),
                    publishedInput.publicationSequence);
                simulationScope.Complete(1u);
                if (replay.waypointReached)
                {
                    __android_log_print(ANDROID_LOG_INFO,
                                        kTag,
                                        "HORDE_REPLAY waypoint generation=%u index=%zu zone=%s x=%.3f z=%.3f",
                                        context.benchmarkGeneration,
                                        replay.reachedWaypoints,
                                        horde::gameplay::ShowcaseZoneName(replay.zone),
                                        replay.x,
                                        replay.z);
                    WriteShowcaseDebugState(context, replay.complete ? "complete" : "replaying");
                }
                if (replay.complete || replay.failed)
                {
                    __android_log_print(replay.complete ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR,
                                        kTag,
                                        "HORDE_REPLAY %s generation=%u reached=%zu expected=%zu zone=%s",
                                        replay.complete ? "complete" : "failed",
                                        context.benchmarkGeneration,
                                        replay.reachedWaypoints,
                                        horde::gameplay::kShowcaseReplayPath.size(),
                                        horde::gameplay::ShowcaseZoneName(replay.zone));
                    WriteShowcaseDebugState(context, replay.complete ? "complete" : "failed");
                    context.routeReplayActive = false;
                }
            }
            else if (!worldCommandsDeferred && (context.captureActive || context.benchmarkSampling))
            {
                // Debug checkpoint measurements and captures are frozen snapshots.
                simulationInput.paused = true;
                simulationInput.damageEnabled = false;
                simulationInput.hasAuthoritativePlayerPose = false;
                horde::vulkan::raytracing::RtSceneStageScope simulationScope(
                    evidenceFrame ? &observation : nullptr,
                    horde::telemetry::RtStage::SimulationStep);
                gGameSimulation.AdvanceFrame(
                    simulationInput,
                    0.0,
                    publishedInput.publicationSequence);
                simulationScope.Complete(1u);
            }
            else if (!worldCommandsDeferred && !inAppBenchmarkFrame && !context.routeReplayActive)
            {
                simulationInput.hasAuthoritativePlayerPose = false;
                const double rawInputDeltaSeconds = context.lastInputOwnerSteadyNs != 0u &&
                    inputOwnerSteadyNs >= context.lastInputOwnerSteadyNs
                    ? static_cast<double>(inputOwnerSteadyNs - context.lastInputOwnerSteadyNs) * 1.0e-9
                    : static_cast<double>(context.frameDeltaSeconds);
                context.lastInputOwnerSteadyNs = inputOwnerSteadyNs;
                context.frameDeltaSeconds = static_cast<float>(std::clamp(rawInputDeltaSeconds, 0.0, 0.1));
                horde::vulkan::raytracing::RtSceneStageScope simulationScope(
                    evidenceFrame ? &observation : nullptr,
                    horde::telemetry::RtStage::SimulationStep);
                gGameSimulation.AdvanceFrame(
                    simulationInput,
                    rawInputDeltaSeconds,
                    publishedInput.publicationSequence,
                    inputOwnerSteadyNs);
                simulationScope.Complete(1u);
            }

            // Immutable copy before the established SFX transport is drained. Music
#if HORDE_RT_ANDROID_MOTION_EVIDENCE
            ObserveAndroidMotionAdvance(context, simulationInput, inputOwnerSteadyNs);
#endif
            // has no authority over simulation or renderer and no second SFX drain.
            horde::platform::android::PublishMusicSnapshot(
                gGameSimulation.Snapshot(), gGameSimulation.Events().Events(), context.captureActive
#if HORDE_RT_ANDROID_MOTION_EVIDENCE
                || (context.motion && !context.motion->finished)
#endif
                );
            DrainSimulationEventsToPlatform();
            PublishSimulationUiState();
            const horde::gameplay::simulation::SimulationSnapshot& renderedSimulation =
                gGameSimulation.Snapshot();
            const bool benchmarkActive = context.benchmarkSampling ||
                context.inAppBenchmark.IsRunning() ||
                gInAppBenchmarkStatus.load(std::memory_order_acquire) == 1;
            gRtLabUnlockEligible.store(
                horde::platform::android::ShouldPersistRtLabUnlock({
                    renderedSimulation.finaleComplete,
                    gRtLabDebugAutomationSession.load(std::memory_order_acquire) ||
                        gRtLabBenchmarkRoute.load(std::memory_order_acquire),
                    context.captureActive,
                    context.routeReplayActive,
                    benchmarkActive}),
                std::memory_order_release);
        }
        const horde::vulkan::raytracing::RtSceneTuning rtLabTuning = gRtLabState.Snapshot();
        horde::vulkan::raytracing::RtSceneFrameInputs frameInputs;
        if (context.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview)
        {
            const auto controls = ReadPreviewControls();
            context.previewSession.Pause(controls.paused);
            context.previewSession.SetMotion(controls.motion);
            context.previewSession.SelectCamera(static_cast<horde::graphics::GraphicsPreviewCamera>(controls.camera));
            if (context.previewResetSerial != controls.resetSerial)
            {
                context.previewResetSerial = controls.resetSerial;
                context.previewSession.Reset();
                context.previewPerformance.BeginScope(++context.previewEpoch);
                context.previewWarmFrames = kMaxFramesInFlight;
                context.previewGpuSamples = context.gpuFrameTimingSampleCount;
            }
            // Reset/loading costs must not advance the newly reset A/B pose.
            context.previewSession.Advance(context.previewWarmFrames == kMaxFramesInFlight ?
                0.0 : context.frameDeltaSeconds);
            frameInputs = horde::vulkan::raytracing::BuildGraphicsPreviewFrameInputs(context.previewSession,
                context.outputExposure, static_cast<horde::vulkan::raytracing::WaterQuality>(context.graphicsSettings.waterQuality),
                horde::vulkan::raytracing::ResolveFireEmitterQuality(context.graphicsSettings.fireDetail),
                rtLabTuning, context.graphicsSettings.shadowQuality);
        }
        else if (context.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::EntryMenu)
        {
            const auto controls = ReadEntryControls();
            if (horde::graphics::CurrentEntryMenuControls(controls, context.surfaceGeneration))
            {
                context.entrySession.SetReducedMotion(controls.reducedMotion);
                context.entrySession.ShowSidePage(controls.sidePage);
                if (context.entryHandoff.PlayRequested()) context.entrySession.Play();
            }
            // Loading and surface-recovery time cannot fast-forward menu motion.
            context.entrySession.Advance(context.entryWarmFrames > 0u ? 0.0 : context.frameDeltaSeconds);
            frameInputs = horde::vulkan::raytracing::BuildEntryMenuFrameInputs(context.entrySession,
                context.outputExposure,
                horde::vulkan::raytracing::ResolveFireEmitterQuality(context.graphicsSettings.fireDetail),
                context.graphicsSettings.shadowQuality);
        }
        else
        {
            frameInputs =
                horde::vulkan::raytracing::BuildRtSceneFrameInputs(
                    gGameSimulation.Snapshot(),
                    context.outputExposure,
                    static_cast<horde::vulkan::raytracing::WaterQuality>(
                        static_cast<int>(context.graphicsSettings.waterQuality)),
                    rtLabTuning);
            frameInputs.fireDetail = horde::vulkan::raytracing::ResolveFireEmitterQuality(context.graphicsSettings.fireDetail);
            frameInputs.shadowQuality = context.graphicsSettings.shadowQuality;
            frameInputs.playerRenderRoute = context.playerRenderRoute;
            if (context.glassFixtureRequested)
            {
                frameInputs.tuning.glassFixtureVisible = true;
                frameInputs.tuning.glassDepthScale = context.glassDepthScale;
                frameInputs.tuning.glassAttenuationColor = context.glassAttenuationColor;
                frameInputs.tuning.glassAttenuationDistance = context.glassAttenuationDistance;
            }
            frameInputs.tuning.productionRewardPropsVisible =
                context.productionRewardPropsRequested;
            frameInputs.tuning.productionLanternGlassOnly =
                context.productionLanternGlassOnly;
        }
        frameInputs.presentationTransform = context.rtPresentationTransform;
        std::string diagnostic;
        if (evidenceFrame)
        {
            (void)context.rtFrameEvidence.BeginRecord(frameInputs.tickIndex);
        }
        gpuTimingRecording = evidenceFrame && context.gpuFrameTimingEnabled &&
            context.gpuFrameTimer.RecordBegin(context.commandBuffers[imageIndex], context.currentFrame);
        context.rtScene.SetMistEnabled(context.graphicsSettings.mistEnabled);
        context.rtScene.SetDustQuality(context.graphicsSettings.dustQuality);
        if (!context.rtScene.RecordTraceAndCopy(context.commandBuffers[imageIndex],
                                                context.swapchainImages[imageIndex],
                                                context.swapchainImageLayouts[imageIndex],
                                                context.swapchainExtent,
                                                frameInputs,
                                                diagnostic,
                                                evidenceFrame ? &observation : nullptr
#if HORDE_RT_STAGED_PRIMARY_TIMING
                                                , gpuTimingRecording ? &context.stagedPassTimer : nullptr, context.currentFrame
#endif
                                                ))
        {
            if (evidenceFrame)
            {
                context.rtFrameEvidence.FailRecord(observation);
            }
            if (gpuTimingRecording)
            {
                context.gpuFrameTimer.CancelRecording(context.currentFrame);
            }
            RejectPendingBenchmarkExpectation(
                context, horde::telemetry::RtBenchmarkFailureReason::SubmissionFailed);
            __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to record RT frame: %s", diagnostic.c_str());
            return false;
        }
        if (gpuTimingRecording &&
            !context.gpuFrameTimer.RecordEnd(context.commandBuffers[imageIndex], context.currentFrame))
        {
            context.gpuFrameTimer.CancelRecording(context.currentFrame);
            gpuTimingRecording = false;
            RefreshGpuTimingTelemetry(context);
        }
        if (evidenceFrame)
        {
            (void)context.rtFrameEvidence.FinishRecord(observation);
        }
    }
    else
    {
        const VkClearValue clearValue = {context.clearColor};
        const VkRenderPassBeginInfo renderPassBegin{
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            context.renderPass,
            context.swapchainFramebuffers[imageIndex],
            {{0, 0}, context.swapchainExtent},
            1u,
            &clearValue};
        vkCmdBeginRenderPass(context.commandBuffers[imageIndex], &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(context.commandBuffers[imageIndex]);
        context.swapchainImageLayouts[imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    if (vkEndCommandBuffer(context.commandBuffers[imageIndex]) != VK_SUCCESS)
    {
        if (evidenceFrame)
        {
            context.rtFrameEvidence.FailGraphicsSubmit(
                gpuTimingRecording,
                horde::vulkan::raytracing::MakeRtGpuFrameTimerIo(
                    context.gpuFrameTimer));
        }
        else if (gpuTimingRecording)
        {
            context.gpuFrameTimer.CancelRecording(context.currentFrame);
        }
        RejectPendingBenchmarkExpectation(
            context, horde::telemetry::RtBenchmarkFailureReason::SubmissionFailed);
        return false;
    }
    const auto recordDone = std::chrono::steady_clock::now();

    const horde::vulkan::raytracing::RtEvidenceSubmitTransaction submitTransaction =
        evidenceFrame
        ? context.rtFrameEvidence.PrevalidateSubmit()
        : horde::vulkan::raytracing::RtEvidenceSubmitTransaction{};
    if (vkResetFences(context.device, 1u, &context.inFlightFences[context.currentFrame]) != VK_SUCCESS)
    {
        if (evidenceFrame)
        {
            context.rtFrameEvidence.FailGraphicsSubmit(
                gpuTimingRecording,
                horde::vulkan::raytracing::MakeRtGpuFrameTimerIo(
                    context.gpuFrameTimer));
        }
        else if (gpuTimingRecording)
        {
            context.gpuFrameTimer.CancelRecording(context.currentFrame);
        }
        RejectPendingBenchmarkExpectation(
            context, horde::telemetry::RtBenchmarkFailureReason::SubmissionFailed);
        return false;
    }

    VkPipelineStageFlags waitStages = useRtFrame ? VK_PIPELINE_STAGE_TRANSFER_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkSubmitInfo submitInfo{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        1u,
        &context.imageAvailableSemaphores[context.currentFrame],
        &waitStages,
        1u,
        &context.commandBuffers[imageIndex],
        1u,
        // Acquisition orders this image's previous present wait before reuse.
        &context.renderFinishedSemaphores[imageIndex]};

    horde::vulkan::raytracing::RtSceneStageScope submitScope(
        evidenceFrame ? &observation : nullptr,
        horde::telemetry::RtStage::QueueSubmit);
    const VkResult submitResult = vkQueueSubmit(
        context.graphicsQueue, 1u, &submitInfo,
        context.inFlightFences[context.currentFrame]);
    submitScope.Complete(1u, 0u, 1u);
    if (submitResult != VK_SUCCESS)
    {
        if (evidenceFrame)
        {
            context.rtFrameEvidence.FailGraphicsSubmit(
                gpuTimingRecording,
                horde::vulkan::raytracing::MakeRtGpuFrameTimerIo(
                    context.gpuFrameTimer));
        }
        else if (gpuTimingRecording)
        {
            context.gpuFrameTimer.CancelRecording(context.currentFrame);
        }
        RejectPendingBenchmarkExpectation(
            context, horde::telemetry::RtBenchmarkFailureReason::SubmissionFailed);
        return false;
    }
    context.imageAcquirePending = false;
    if (useRtFrame) context.rtScene.NotifyFrameSubmitted();
    if (evidenceFrame)
    {
        context.rtFrameEvidence.CommitGraphicsSubmit(
            submitTransaction,
            context.gpuFrameTimingEnabled,
            gpuTimingRecording,
            horde::vulkan::raytracing::MakeRtGpuFrameTimerIo(
                context.gpuFrameTimer));
#if HORDE_RT_ANDROID_MOTION_EVIDENCE
        if (context.motion && context.motion->armed && !context.motion->finished)
        {
            horde::telemetry::RtSubmittedFrameIdentity owner{};
            if (!context.rtFrameEvidence.TryGetCommittedIdentity(context.currentFrame, owner) ||
                !context.motion->ledger.BindSubmittedFrame(context.surfaceGeneration, owner))
                FailAndroidMotion(context, "Motion frame could not bind its actual submitted simulation owner.");
        }
#endif
#if HORDE_RT_STAGED_PRIMARY_TIMING
        horde::telemetry::RtSubmittedFrameIdentity stagedOwner{};
        if (context.rtFrameEvidence.TryGetCommittedIdentity(context.currentFrame, stagedOwner))
            (void)context.stagedPassTimer.MarkSubmitted(context.currentFrame, stagedOwner.submissionSerial);
#endif
    }
    if (context.benchmarkExpectedFrame.has_value())
    {
        if (evidenceFrame)
        {
            BindCommittedBenchmarkExpectation(context);
        }
        else
        {
            RejectPendingBenchmarkExpectation(
                context, horde::telemetry::RtBenchmarkFailureReason::TokenlessCompletion);
        }
    }

    if (context.presentCompletionFences.Prepare(imageIndex) != VK_SUCCESS) return false;
    const VkFence presentFence = context.presentCompletionFences.Fence(imageIndex);
    VkSwapchainPresentFenceInfoEXT presentFenceInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT};
    presentFenceInfo.swapchainCount = 1u;
    presentFenceInfo.pFences = &presentFence;
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = presentFence != VK_NULL_HANDLE ? &presentFenceInfo : nullptr;
    presentInfo.waitSemaphoreCount = 1u;
    presentInfo.pWaitSemaphores = &context.renderFinishedSemaphores[imageIndex];
    presentInfo.swapchainCount = 1u;
    presentInfo.pSwapchains = &context.swapchain;
    presentInfo.pImageIndices = &imageIndex;
#if HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION
    VkPresentTimeGOOGLE measuredPresentTime{};
    VkPresentTimesInfoGOOGLE measuredPresentInfo{VK_STRUCTURE_TYPE_PRESENT_TIMES_INFO_GOOGLE};
    horde::telemetry::RtSubmittedFrameIdentity measuredFrame{};
    const bool measuredPresent = evidenceFrame && useRtFrame &&
        context.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::Showcase &&
        context.rtFrameEvidence.TryGetCommittedIdentity(context.currentFrame, measuredFrame) &&
        context.presentTiming && context.presentTiming->PrepareNext(measuredPresentTime);
    if (measuredPresent)
    {
        measuredPresentInfo.pNext = presentInfo.pNext; // Retain maintenance1 retirement fences.
        measuredPresentInfo.swapchainCount = 1u;
        measuredPresentInfo.pTimes = &measuredPresentTime; // desiredPresentTime remains zero.
        presentInfo.pNext = &measuredPresentInfo;
    }
    const auto measuredQueuedNs = GraphicsSteadyNs();
#endif
    horde::vulkan::raytracing::RtSceneStageScope presentScope(
        evidenceFrame ? &observation : nullptr,
        horde::telemetry::RtStage::PresentCall);
    const VkResult presentResult = vkQueuePresentKHR(context.graphicsQueue, &presentInfo);

    if (context.graphicsLatency.active && context.graphicsLatency.firstPresentedNs == 0u &&
        useRtFrame && presentResult == VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
        context.graphicsLatency.firstPresentedNs = GraphicsSteadyNs();
#if !defined(NDEBUG)
    const std::uint64_t inputPresentNs = horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr);
    const bool inspectInput = gRtLabDebugAutomationSession.load(std::memory_order_acquire) &&
        context.inputPresentationTrace.HasReportable(gGameSimulation.Snapshot(), inputPresentNs);
    if (evidenceFrame && useRtFrame && presentResult == VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR &&
        context.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::Showcase &&
        (context.combatTimingTrace.HasReportable(gGameSimulation.Snapshot()) || inspectInput))
    {
        horde::telemetry::RtSubmittedFrameIdentity committed{};
        if (context.rtFrameEvidence.TryGetCommittedIdentity(context.currentFrame, committed))
        {
            std::ostringstream trace;
            context.combatTimingTrace.WriteAcceptedPresent(trace, gGameSimulation.Snapshot(), committed,
                inputPresentNs, true,
                observation.recordedScene ? &observation.recordedScene->player : nullptr);
            if (inspectInput)
                context.inputPresentationTrace.WriteAcceptedPresent(trace, gGameSimulation.Snapshot(),
                    committed, inputPresentNs, true);
            const std::string rows = trace.str();
            // Logcat has a per-message size bound. Emit each admitted row as a
            // separate message rather than truncate a multi-edge frame.
            std::size_t start = 0u;
            while (start < rows.size())
            {
                const auto end = rows.find('\n', start);
                const auto length = (end == std::string::npos ? rows.size() : end) - start;
                __android_log_print(ANDROID_LOG_INFO, kTag, "%.*s", static_cast<int>(length), rows.data() + start);
                if (end == std::string::npos) break;
                start = end + 1u;
            }
        }
    }
#endif
    context.presentCompletionFences.Presented(imageIndex, presentResult);
    presentScope.Complete(1u, 0u, 1u);
    wholeFrameScope.Complete(1u, 0u, 1u);
    const auto presentDone = std::chrono::steady_clock::now();
#if HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION
    if (measuredPresent)
    {
        horde::vulkan::PresentTimingFrameMetadata metadata;
        metadata.sceneEpoch = measuredFrame.frame.sceneEpoch;
        metadata.measurementGeneration = measuredFrame.frame.measurementGeneration;
        metadata.recordSerial = measuredFrame.frame.recordSerial;
        metadata.submissionSerial = measuredFrame.submissionSerial;
        metadata.simulationTick = measuredFrame.frame.simulationTick;
        metadata.queuedSteadyNs = measuredQueuedNs;
        metadata.scalePercent = static_cast<std::uint32_t>(std::lround(context.renderScale * 100.0f));
        metadata.width = context.swapchainExtent.width;
        metadata.height = context.swapchainExtent.height;
        metadata.backend = context.executionBackend == horde::vulkan::RtExecutionBackend::RayQueryCompute
            ? horde::vulkan::PresentTimingBackend::RayQueryCompute : horde::vulkan::PresentTimingBackend::RayTracingPipeline;
        (void)context.presentTiming->RegisterPresent(measuredPresentTime.presentID,
            acquireResult == VK_SUBOPTIMAL_KHR ? VK_SUBOPTIMAL_KHR : presentResult, metadata);
    }
    PollPresentTimingOnOwner(context);
#endif

    if (evidenceFrame)
    {
        const horde::telemetry::RtPresentationOutcome outcome =
            presentResult == VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR
            ? horde::telemetry::RtPresentationOutcome::Presented
            : (presentResult == VK_SUBOPTIMAL_KHR ||
               (presentResult == VK_SUCCESS && acquireResult == VK_SUBOPTIMAL_KHR)
                ? horde::telemetry::RtPresentationOutcome::PresentedNeedsRecreate
                : presentResult == VK_ERROR_OUT_OF_DATE_KHR
                ? horde::telemetry::RtPresentationOutcome::NotPresentedNeedsRecreate
                : horde::telemetry::RtPresentationOutcome::Failed);
        (void)context.rtFrameEvidence.AttachPresentation(outcome);
        context.rtFrameEvidence.FinalizeSubmittedFrame(observation);
    }
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR ||
        (presentResult == VK_SUCCESS && acquireResult == VK_SUBOPTIMAL_KHR))
    {
        __android_log_print(ANDROID_LOG_INFO, kTag,
            "HORDE_SURFACE_RECREATE generation=%llu acquire_result=%d present_result=%d active_pre_transform=0x%08x active_extent=%ux%u",
            static_cast<unsigned long long>(context.surfaceGeneration),
            static_cast<int>(acquireResult), static_cast<int>(presentResult),
            static_cast<unsigned int>(context.swapchainPreTransform),
            context.swapchainExtent.width, context.swapchainExtent.height);
        rtFramePresented = useRtFrame && (presentResult == VK_SUCCESS || presentResult == VK_SUBOPTIMAL_KHR);
        if (inAppBenchmarkFrame)
        {
            const double interruptedFrameMs =
                std::chrono::duration<double, std::milli>(presentDone - frameStart).count();
            context.inAppBenchmark.RecordFrame(interruptedFrameMs, false);
            PublishBenchmarkProgress(context);
        }
        resourceRecreated = true;
        const bool recreated = RecreateSwapchain(context);
        return recreated;
    }
    if (presentResult != VK_SUCCESS)
    {
        return false;
    }

    rtFramePresented = useRtFrame;
#if HORDE_RT_ANDROID_MOTION_EVIDENCE
    AfterAndroidMotionPresent(context);
#endif
    context.currentFrame = (context.currentFrame + 1u) % kMaxFramesInFlight;
    const auto milliseconds = [](auto duration) { return std::chrono::duration<double, std::milli>(duration).count(); };
    const double frameFenceMs = milliseconds(fenceDone - frameStart);
    const double frameRecordMs = milliseconds(recordDone - recordStart);
    const double framePresentMs = milliseconds(presentDone - recordDone);
    const double frameTotalMs = milliseconds(presentDone - frameStart);
    context.timingFenceMs += frameFenceMs;
    context.timingRecordMs += frameRecordMs;
    context.timingPresentMs += framePresentMs;
    context.timingTotalMs += frameTotalMs;
    // Publish the first live diagnostic quickly so opening the panel does not
    // sit on "N/A" for several seconds; subsequent samples use the steadier
    // two-second window.
    const uint32_t timingSampleFrames = context.capabilities.performance.frameTimeMs > 0.0f ? 120u : 30u;
    if (++context.timingFrameCount >= timingSampleFrames)
    {
        const double count = static_cast<double>(context.timingFrameCount);
        const double averageFrameMs = context.timingTotalMs / count;
        context.capabilities.performance.frameTimeMs = static_cast<float>(averageFrameMs);
        context.capabilities.performance.fps = averageFrameMs > 0.0
            ? static_cast<float>(1000.0 / averageFrameMs)
            : 0.0f;
        auto& timingDiagnostics = context.capabilities.diagnostics;
        timingDiagnostics.erase(std::remove(timingDiagnostics.begin(), timingDiagnostics.end(),
                                            "FPS / frame time: not measured yet."),
                                timingDiagnostics.end());
        PublishRuntimeReports(context);
        __android_log_print(ANDROID_LOG_INFO,
                            kTag,
                            "RT frame timing avg ms: total=%.3f fence=%.3f record=%.3f submit+present=%.3f",
                            context.timingTotalMs / count,
                            context.timingFenceMs / count,
                            context.timingRecordMs / count,
                            context.timingPresentMs / count);
        const auto& gpuTiming = context.capabilities.performance.gpuRt;
        __android_log_print(ANDROID_LOG_INFO,
                            kTag,
                            "HORDE_GPU mode=%s status=%s valid=%d latest_ms=%.3f average_ms=%.3f samples=%llu valid_bits=%u period_ns=%.6f unavailable=%llu errors=%llu",
                            context.gpuFrameTimingEnabled ? "enabled" : "disabled",
                            context.gpuFrameTimingEnabled
                                ? horde::vulkan::GpuFrameTimerStatusName(context.gpuFrameTimer.Telemetry().status)
                                : "disabled",
                            gpuTiming.valid ? 1 : 0,
                            static_cast<double>(gpuTiming.latestMs),
                            static_cast<double>(gpuTiming.averageMs),
                            static_cast<unsigned long long>(gpuTiming.sampleCount),
                            gpuTiming.timestampValidBits,
                            static_cast<double>(gpuTiming.timestampPeriodNanoseconds),
                            static_cast<unsigned long long>(gpuTiming.unavailableCount),
                            static_cast<unsigned long long>(gpuTiming.errorCount));
        context.timingFrameCount = 0u;
        context.timingFenceMs = context.timingRecordMs = context.timingPresentMs = context.timingTotalMs = 0.0;
    }
    RecordBenchmarkFrame(context, frameFenceMs, frameRecordMs, framePresentMs, frameTotalMs);
    if (inAppBenchmarkFrame)
    {
        context.inAppBenchmark.RecordFrame(frameTotalMs, rtFramePresented);
        if (context.inAppBenchmark.ConsumeLiveProgressUpdate()) PublishBenchmarkProgress(context);
        if (!context.inAppBenchmark.IsRunning())
        {
            if (context.inAppBenchmark.Status() ==
                    horde::gameplay::ShowcaseBenchmarkStatus::Complete)
            {
                if (!FinalizeCompletedInAppBenchmark(context))
                {
                    __android_log_print(ANDROID_LOG_ERROR, kTag,
                                        "Android benchmark final evidence drain was incomplete.");
                }
            }
            else
            {
                CancelActiveInAppBenchmark(context);
            }
            FinishInAppBenchmark(context);
        }
    }
    if (context.captureActive && rtFramePresented && context.capturePresentedFrames < 12u)
    {
        ++context.capturePresentedFrames;
        if (context.capturePresentedFrames == 12u)
        {
            WriteShowcaseDebugState(context, "capture-ready");
            __android_log_print(ANDROID_LOG_INFO,
                                kTag,
                                "HORDE_CAPTURE_READY generation=%u checkpoint=%s scale=%.0f stable_frames=12 presented=%d",
                                context.benchmarkGeneration,
                                context.activeBenchmarkName.c_str(),
                                context.renderScale * 100.0f,
                                context.capabilities.rtScene.presented ? 1 : 0);
        }
    }
    return true;
}

enum class GlassGeometryApplyResult { Applied, RolledBack, Fatal };

GlassGeometryApplyResult ApplyGlassGeometryOnOwner(
    SwapchainContext& context, const horde::graphics::GraphicsSettings requested)
{
    const auto previous = context.graphicsSettings;
    const float previousScale = context.renderScale;
    const auto drainAndReset = [&]() {
        const auto idleStart = GraphicsSteadyNs();
        const VkResult idle = vkDeviceWaitIdle(context.device);
        const bool drained = CompleteRtEvidenceAfterDeviceIdle(context, idle) &&
            vkResetCommandPool(context.device, context.commandPool, 0) == VK_SUCCESS;
        context.graphicsLatency.idleMilliseconds += GraphicsElapsedMs(idleStart, GraphicsSteadyNs());
        return drained;
    };
    if (!drainAndReset()) return GlassGeometryApplyResult::Fatal;
    CancelActiveInAppBenchmark(context);
    gSurfaceSessions.Publish(context.surfaceGeneration, 0);
    context.capabilities.rtScene.presented = false;
    PublishGraphicsApplied(context, true, false);
    context.gpuFrameTimer.ResetAfterDeviceIdle();
#if HORDE_RT_STAGED_PRIMARY_TIMING
    context.stagedPassTimer.ResetAfterDeviceIdle();
#endif
    if (context.rtFrameEvidenceInitialised && !context.rtFrameEvidence.Recreate(
            horde::telemetry::RtResourceResetReason::DiagnosticResourceReplacement,
            CurrentInitialGpuEvidenceStatus(context)))
    {
        (void)context.rtFrameEvidence.Destroy();
        context.rtFrameEvidenceInitialised = false;
    }
    // One scene, same backend/provider/compiled optics. The fifth setting owns
    // actual admitted geometry; a combined scale uses this one new extent.
    const auto destroyStart = GraphicsSteadyNs();
    context.rtScene.Destroy();
    context.graphicsLatency.destroyMilliseconds += GraphicsElapsedMs(destroyStart, GraphicsSteadyNs());
    context.graphicsSettings = requested;
    context.renderScale = requested.renderScalePercent / 100.0f;
    const auto initialiseStart = GraphicsSteadyNs();
    const bool applied = InitialiseRtSceneForSwapchain(context) &&
        context.rtScene.GlassEnabled() == requested.glassEnabled;
    context.graphicsLatency.initialiseMilliseconds += GraphicsElapsedMs(initialiseStart, GraphicsSteadyNs());
    if (!applied)
    {
        // Initialization may have uploaded GPU resources before failing. Prove
        // their real retirement too; never destroy/fallback after failed idle.
        if (!drainAndReset()) return GlassGeometryApplyResult::Fatal;
        const auto rollbackDestroyStart = GraphicsSteadyNs();
        context.rtScene.Destroy();
        context.graphicsLatency.destroyMilliseconds += GraphicsElapsedMs(rollbackDestroyStart, GraphicsSteadyNs());
        context.graphicsSettings = previous;
        context.renderScale = previousScale;
        const auto rollbackInitialiseStart = GraphicsSteadyNs();
        const bool restored = InitialiseRtSceneForSwapchain(context) && context.rtScene.GlassEnabled() == previous.glassEnabled;
        context.graphicsLatency.initialiseMilliseconds += GraphicsElapsedMs(rollbackInitialiseStart, GraphicsSteadyNs());
        if (!restored)
            return GlassGeometryApplyResult::Fatal;
        context.graphicsReason = horde::graphics::GraphicsReason::ResourceFailure;
    }
    if (!gSurfaceSessions.IsCurrent(context.surfaceGeneration)) return GlassGeometryApplyResult::Fatal;
    context.capturePresentedFrames = 0u;
    context.timingFrameCount = 0u;
    context.timingFenceMs = context.timingRecordMs = context.timingPresentMs = context.timingTotalMs = 0.0;
    context.gpuFrameTimingTotalMs = 0.0; context.gpuFrameTimingSampleCount = 0u;
    context.previewGpuSamples = 0u;
    RefreshGpuTimingTelemetry(context);
    context.previewPerformance.BeginScope(++context.previewEpoch);
    context.previewWarmFrames = kMaxFramesInFlight;
    if (!applied)
    {
        PublishGraphicsApplied(context, false, false);
        std::lock_guard lock(gGraphicsMutex);
        if (gRequestedGraphics.serial == context.graphicsSerial)
            gRequestedGraphics.requested = context.graphicsSettings;
    }
    __android_log_print(applied ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR, kTag,
        "HORDE_RT_GLASS_REBUILD requested=%d effective=%d scale=%.0f restored=%d presented=0",
        requested.glassEnabled ? 1 : 0, context.rtScene.GlassEnabled() ? 1 : 0,
        context.renderScale * 100.0f, applied ? 0 : 1);
    return applied ? GlassGeometryApplyResult::Applied : GlassGeometryApplyResult::RolledBack;
}

bool ApplySceneProfileOnOwner(SwapchainContext& context, const PreviewControls controls)
{
    const auto desired = DesiredProfile(context, controls);
    if (context.sceneProfile == desired) return true;
    const auto previous = context.sceneProfile;
    context.previewTransitionFailed = false;
    context.entryHandoff.BeginLoad();
    PublishEntryState(context, false);
    gSurfaceSessions.Publish(context.surfaceGeneration, 0);
    const VkResult idle = vkDeviceWaitIdle(context.device);
    if (!CompleteRtEvidenceAfterDeviceIdle(context, idle)) return false;
    CancelActiveInAppBenchmark(context);
    if (vkResetCommandPool(context.device, context.commandPool, 0) != VK_SUCCESS) return false;
    context.gpuFrameTimer.ResetAfterDeviceIdle();
#if HORDE_RT_STAGED_PRIMARY_TIMING
    context.stagedPassTimer.ResetAfterDeviceIdle();
#endif
    if (context.rtFrameEvidenceInitialised && !context.rtFrameEvidence.Recreate(
            horde::telemetry::RtResourceResetReason::DiagnosticResourceReplacement,
            CurrentInitialGpuEvidenceStatus(context)))
    {
        (void)context.rtFrameEvidence.Destroy();
        context.rtFrameEvidenceInitialised = false;
    }
    context.rtScene.Destroy(); // One active GPU scene; no simulation/reset constructor.
    context.sceneProfile = desired;
    if (!InitialiseRtSceneForSwapchain(context))
    {
        // A failed replacement can have submitted uploads. Prove retirement
        // again before reusing the command pool and rebuilding the old scene.
        const VkResult rollbackIdle = vkDeviceWaitIdle(context.device);
        if (!CompleteRtEvidenceAfterDeviceIdle(context, rollbackIdle) ||
            vkResetCommandPool(context.device, context.commandPool, 0) != VK_SUCCESS)
            return false;
        context.rtScene.Destroy();
        context.sceneProfile = previous;
        if (!InitialiseRtSceneForSwapchain(context)) return false;
        {
            std::lock_guard lock(gGraphicsMutex);
            gPreviewControls.enabled = previous == horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview;
        }
        if (previous == horde::vulkan::raytracing::RtSceneProfile::EntryMenu &&
            desired == horde::vulkan::raytracing::RtSceneProfile::Showcase)
        {
            context.entrySession.Reset();
            context.entryHandoff.FailLoad();
        }
        else if (desired == horde::vulkan::raytracing::RtSceneProfile::EntryMenu &&
                 previous == horde::vulkan::raytracing::RtSceneProfile::Showcase)
        {
            context.entryHandoff.FailLoad(GraphicsSceneForProfile(previous));
        }
        else context.entryHandoff.EndLoad();
        context.previewTransitionFailed = true;
        context.graphicsReason = horde::graphics::GraphicsReason::ResourceFailure;
    }
    else context.entryHandoff.EndLoad();
    context.capabilities.rtScene.presented = false;
    context.capturePresentedFrames = 0u;
    context.gpuFrameTimingTotalMs = 0.0; context.gpuFrameTimingSampleCount = 0u;
    RefreshGpuTimingTelemetry(context);
    context.previewGpuSamples = 0u;
    context.previewPerformance.BeginScope(++context.previewEpoch);
    context.previewWarmFrames = kMaxFramesInFlight;
    context.entryWarmFrames = kMaxFramesInFlight;
    PublishEntryState(context, false);
    gSurfaceSessions.Publish(context.surfaceGeneration, 0);
    return true;
}

void SwapchainRenderLoop()
{
    auto previousFrameStart = std::chrono::steady_clock::now();
    horde::graphics::ForegroundPauseRenderCadence pausedRenderCadence;
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
    std::chrono::steady_clock::time_point lastDeveloperOverlayPublish{};
#endif
    const bool initiallyPaused = SynchronizeLifecyclePauseOnOwnerThread();
    if (!InitialiseRtEvidenceOnOwnerThread(gSwapchainContext, initiallyPaused))
    {
        gSurfaceSessions.Publish(gSwapchainContext.surfaceGeneration, 3);
        gSwapchainRunning.store(false, std::memory_order_release);
        __android_log_print(
            ANDROID_LOG_ERROR, kTag,
            "Invalid RT evidence ownership configuration on render owner thread.");
    }
    while (gSwapchainRunning.load(std::memory_order_acquire) &&
           gSurfaceSessions.IsCurrent(gSwapchainContext.surfaceGeneration))
    {
        const auto loopStart = std::chrono::steady_clock::now();
        if (gSurfaceSessions.IsSuspended(gSwapchainContext.surfaceGeneration))
        {
            // Lifecycle suspension is stronger than ordinary foreground pause:
            // synchronize/discard input, drain already-owned GPU work, then wait
            // without simulation, skinning, uploads, scene changes or rendering.
            (void)SynchronizeLifecyclePauseOnOwnerThread();
            if (!ConsumePendingImageAcquire(gSwapchainContext))
            {
                gSurfaceSessions.Publish(gSwapchainContext.surfaceGeneration, 3);
                break; // Retirement keeps ownership if the acquire drain failed.
            }
            const VkResult idleResult = vkDeviceWaitIdle(gSwapchainContext.device);
            (void)CompleteRtEvidenceAfterDeviceIdle(gSwapchainContext, idleResult);
            __android_log_print(idleResult == VK_SUCCESS ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR, kTag,
                "HORDE_SURFACE_SUSPENDED generation=%llu idle_result=%d",
                static_cast<unsigned long long>(gSwapchainContext.surfaceGeneration),
                static_cast<int>(idleResult));
            if (idleResult != VK_SUCCESS)
            {
                gSurfaceSessions.Publish(gSwapchainContext.surfaceGeneration, 3);
                break; // Normal proved-retirement path still owns all resources.
            }
            CancelActiveInAppBenchmark(gSwapchainContext);
            ClearPlatformGameplayEvents();
#if HORDE_RT_ANDROID_MOTION_EVIDENCE
            FailAndroidMotion(gSwapchainContext, "Motion interrupted by lifecycle suspension.");
#endif
            gSurfaceSessions.WaitWhileSuspended(gSwapchainContext.surfaceGeneration);
            // Suspended wall time is not a gameplay hitch/catch-up interval.
            previousFrameStart = std::chrono::steady_clock::now();
            gSwapchainContext.lastInputOwnerSteadyNs = GraphicsSteadyNs();
            __android_log_print(ANDROID_LOG_INFO, kTag,
                "HORDE_SURFACE_SUSPEND_WAIT_END generation=%llu current=%d",
                static_cast<unsigned long long>(gSwapchainContext.surfaceGeneration),
                gSurfaceSessions.IsCurrent(gSwapchainContext.surfaceGeneration) ? 1 : 0);
            continue;
        }
#if HORDE_RT_ANDROID_MOTION_EVIDENCE
        ResetAndroidMotionIfRequested(gSwapchainContext);
#endif
        const bool measurementPaused = SynchronizeLifecyclePauseOnOwnerThread();
        const auto previewControls = ReadPreviewControls();
        const auto entryControls = ReadEntryControls();
        if (gSwapchainContext.entryHandoff.Observe(entryControls, gSwapchainContext.surfaceGeneration))
            gSwapchainContext.entrySession.Reset();
        bool sceneTransition = DesiredProfile(gSwapchainContext, previewControls) != gSwapchainContext.sceneProfile ||
            previewControls.resetSerial != gSwapchainContext.previewResetSerial;
#if HORDE_RT_ANDROID_MOTION_EVIDENCE
        if (sceneTransition || measurementPaused)
            FailAndroidMotion(gSwapchainContext, "Motion interrupted by foreground pause or scene transition.");
#endif
        if (!ApplySceneProfileOnOwner(gSwapchainContext, previewControls))
        {
            gSwapchainContext.entryHandoff.FailLoad();
            PublishEntryState(gSwapchainContext, false);
            gSurfaceSessions.Publish(gSwapchainContext.surfaceGeneration, 3);
            break;
        }
        if (gSwapchainContext.rtFrameEvidenceInitialised)
        {
            (void)gSwapchainContext.rtFrameEvidence.SetPaused(measurementPaused);
        }
        if (measurementPaused &&
            (gSwapchainContext.benchmarkEvidence.Status() ==
                 horde::telemetry::RtBenchmarkRunStatus::Allocated ||
             gSwapchainContext.benchmarkEvidence.Status() ==
                 horde::telemetry::RtBenchmarkRunStatus::Measuring))
        {
            CancelActiveInAppBenchmark(gSwapchainContext);
        }
        const auto graphicsObservation = ReadGraphicsCommandObservation();
        const auto graphicsCommand = graphicsObservation.first;
        if (graphicsCommand.serial != gSwapchainContext.graphicsSerial &&
            (graphicsCommand.lifecycleGeneration == 0u ||
             graphicsCommand.lifecycleGeneration == gSwapchainContext.surfaceGeneration))
        {
            auto& context = gSwapchainContext;
            sceneTransition = true;
            if (context.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview)
            {
                context.previewSession.Reset();
                context.previewPerformance.BeginScope(++context.previewEpoch);
                context.previewWarmFrames = kMaxFramesInFlight;
                context.previewGpuSamples = context.gpuFrameTimingSampleCount;
            }
            context.graphicsSerial = graphicsCommand.serial;
            context.graphicsLatency = graphicsObservation.second;
            context.graphicsLatency.active = context.graphicsLatency.serial == context.graphicsSerial;
            context.graphicsLatency.observedNs = GraphicsSteadyNs();
            context.graphicsRequested = graphicsCommand.requested;
            context.graphicsReason = horde::graphics::GraphicsReason::None;
            const float requestedRenderScale = graphicsCommand.requested.renderScalePercent / 100.0f;
            bool applied = true;
            const bool glassChanged = graphicsCommand.requested.glassEnabled != context.rtScene.GlassEnabled();
            context.graphicsLatency.glassChanged = glassChanged;
            if (context.useRtPath && glassChanged)
            {
                const auto result = ApplyGlassGeometryOnOwner(context, graphicsCommand.requested);
                if (result == GlassGeometryApplyResult::Fatal)
                {
                    gSurfaceSessions.Publish(context.surfaceGeneration, 3);
                    __android_log_print(ANDROID_LOG_ERROR, kTag, "Glass Apply could not prove/rebuild its actual GPU ownership.");
                    break;
                }
                applied = result == GlassGeometryApplyResult::Applied;
            }
            else if (context.useRtPath && std::abs(requestedRenderScale - context.renderScale) > 0.001f)
            {
                const auto resizeStart = std::chrono::steady_clock::now();
                const VkResult idleResult = vkDeviceWaitIdle(context.device);
                if (!CompleteRtEvidenceAfterDeviceIdle(context, idleResult))
                {
                    gSurfaceSessions.Publish(context.surfaceGeneration, 3);
                    __android_log_print(ANDROID_LOG_ERROR, kTag, "Cannot resize before owned GPU work completes.");
                    break; // Device/ownership failure remains a renderer failure.
                }
                CancelActiveInAppBenchmark(context);
                std::string diagnostic;
                const VkExtent2D extent = ScaledRenderExtent(context.swapchainExtent, requestedRenderScale);
                applied = context.rtScene.ResizeOutputAfterDeviceIdle(extent, diagnostic);
                if (applied)
                {
                    if (context.rtFrameEvidenceInitialised && !context.rtFrameEvidence.Recreate(
                        horde::telemetry::RtResourceResetReason::RenderScaleChange,
                        CurrentInitialGpuEvidenceStatus(context)))
                    {
                        (void)context.rtFrameEvidence.Destroy();
                        context.rtFrameEvidenceInitialised = false;
                    }
                    context.gpuFrameTimer.ResetAfterDeviceIdle();
#if HORDE_RT_STAGED_PRIMARY_TIMING
                    context.stagedPassTimer.ResetAfterDeviceIdle();
#endif
                    context.gpuFrameTimingTotalMs = 0.0;
                    context.gpuFrameTimingSampleCount = 0u;
                    context.previewGpuSamples = 0u;
                    RefreshGpuTimingTelemetry(context);
                    context.renderScale = requestedRenderScale;
                    context.capabilities.rtScene.presented = false;
                    context.capturePresentedFrames = 0u;
                    context.timingFrameCount = 0u;
                    context.timingFenceMs = context.timingRecordMs = 0.0;
                    context.timingPresentMs = context.timingTotalMs = 0.0;
                    gSurfaceSessions.Publish(context.surfaceGeneration, 0);
                    const double milliseconds = std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - resizeStart).count();
                    __android_log_print(ANDROID_LOG_INFO, kTag,
                        "HORDE_RT_SCALE_RESIZE scale=%.0f extent=%ux%u output_only=1 idle_and_resize_ms=%.3f",
                        requestedRenderScale * 100.0f, extent.width, extent.height, milliseconds);
                }
                else
                {
                    // Shared output resize is transactional. Keep old resources,
                    // dimensions and all settings, and consume the failed command.
                    context.graphicsReason = horde::graphics::GraphicsReason::ResourceFailure;
                    PublishGraphicsApplied(context, false, context.capabilities.rtScene.presented);
                    __android_log_print(ANDROID_LOG_ERROR, kTag,
                        "Graphics Apply failed; previous output retained: %s", diagnostic.c_str());
                    std::lock_guard lock(gGraphicsMutex);
                    if (gRequestedGraphics.serial == graphicsCommand.serial)
                        gRequestedGraphics.requested = context.graphicsSettings;
                }
            }
            if (applied) context.graphicsSettings = graphicsCommand.requested;
        }
#if HORDE_RT_ANDROID_MOTION_VALIDATION
        if (gSwapchainContext.motion && gSwapchainContext.motion->terminalTimingDrainPending)
        {
            if (gInAppBenchmarkCancelRequested.exchange(false, std::memory_order_acq_rel))
                CancelActiveInAppBenchmark(gSwapchainContext);
            PollAndroidMotionTerminalTimingDrain(gSwapchainContext);
            const auto pollFinished = std::chrono::steady_clock::now();
            previousFrameStart = pollFinished;
            gSwapchainContext.lastInputOwnerSteadyNs = GraphicsSteadyNs();
            // The render owner stays responsive to lifecycle and cancellation
            // above, but submits no new frames while the final timing IDs drain.
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
#endif
        horde::graphics::ForegroundPauseRenderInput pauseCadenceInput{};
        pauseCadenceInput.foreground = true; // A live swapchain owner exists only for the foreground surface.
        pauseCadenceInput.paused = measurementPaused;
        pauseCadenceInput.scene = gSwapchainContext.sceneProfile ==
            horde::vulkan::raytracing::RtSceneProfile::EntryMenu
                ? horde::graphics::ForegroundPauseScene::EntryMenu
                : gSwapchainContext.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview
                    ? horde::graphics::ForegroundPauseScene::GraphicsPreview
                    : horde::graphics::ForegroundPauseScene::Showcase;
        pauseCadenceInput.currentRtOutputValid = gSwapchainContext.useRtPath &&
            gSwapchainContext.rtScene.IsReady() && gSwapchainContext.capabilities.rtScene.presented;
        pauseCadenceInput.wakeRequested = sceneTransition || gSwapchainContext.graphicsLatency.active ||
            gSwapchainContext.inAppBenchmark.IsRunning() || gSwapchainContext.captureActive ||
            gSwapchainContext.routeReplayActive;
        const auto cadenceDecision = pausedRenderCadence.Evaluate(
            std::chrono::steady_clock::now(), pauseCadenceInput);
        horde::graphics::ForegroundPauseRenderAggregate cadenceAggregate{};
        if (pausedRenderCadence.TakeAggregate(std::chrono::steady_clock::now(), cadenceAggregate))
        {
            __android_log_print(ANDROID_LOG_INFO, kTag,
                "HORDE_PAUSE_RENDER_CADENCE wall_ns=%llu render_attempts=%llu skipped_iterations=%llu entered=%llu exited=%llu",
                static_cast<unsigned long long>(cadenceAggregate.wallNanoseconds),
                static_cast<unsigned long long>(cadenceAggregate.renderAttempts),
                static_cast<unsigned long long>(cadenceAggregate.skippedIterations),
                static_cast<unsigned long long>(cadenceAggregate.enteredFallback),
                static_cast<unsigned long long>(cadenceAggregate.exitedFallback));
        }
        if (!cadenceDecision.renderNow)
        {
            const auto skippedAt = std::chrono::steady_clock::now();
            previousFrameStart = skippedAt;
            gSwapchainContext.lastInputOwnerSteadyNs = GraphicsSteadyNs();
            std::this_thread::sleep_for(cadenceDecision.wait);
            continue;
        }
        const auto frameStart = std::chrono::steady_clock::now();
        gSwapchainContext.frameDeltaSeconds = std::clamp(std::chrono::duration<float>(frameStart - previousFrameStart).count(), 1.0f / 240.0f, 0.1f);
        previousFrameStart = frameStart;
        bool rtFramePresented = false;
        bool resourceRecreated = false;
        if (gSwapchainContext.graphicsLatency.active && gSwapchainContext.graphicsLatency.firstRenderNs == 0u)
            gSwapchainContext.graphicsLatency.firstRenderNs = GraphicsSteadyNs();
        if (!RenderFrame(gSwapchainContext, rtFramePresented, resourceRecreated))
        {
            if (!gSurfaceSessions.IsCurrent(gSwapchainContext.surfaceGeneration)) break;
            FailInAppBenchmarkAfterRenderFailure(gSwapchainContext);
            gSwapchainContext.entryHandoff.FailLoad();
            PublishEntryState(gSwapchainContext, false);
            gSurfaceSessions.Publish(gSwapchainContext.surfaceGeneration, 3);
            __android_log_print(ANDROID_LOG_ERROR, kTag, "Diagnostic surface render loop ended unexpectedly.");
            break;
        }
        const auto renderAttemptFinished = std::chrono::steady_clock::now();
        if (cadenceDecision.usingPausedFallback)
        {
            pausedRenderCadence.RecordRenderAttempt(renderAttemptFinished);
            horde::graphics::ForegroundPauseRenderAggregate aggregate{};
            if (pausedRenderCadence.TakeAggregate(renderAttemptFinished, aggregate))
            {
                __android_log_print(ANDROID_LOG_INFO, kTag,
                    "HORDE_PAUSE_RENDER_CADENCE wall_ns=%llu render_attempts=%llu skipped_iterations=%llu entered=%llu exited=%llu",
                    static_cast<unsigned long long>(aggregate.wallNanoseconds),
                    static_cast<unsigned long long>(aggregate.renderAttempts),
                    static_cast<unsigned long long>(aggregate.skippedIterations),
                    static_cast<unsigned long long>(aggregate.enteredFallback),
                    static_cast<unsigned long long>(aggregate.exitedFallback));
            }
        }
        const double cpuRenderMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - frameStart).count();
        if (!gSurfaceSessions.IsCurrent(gSwapchainContext.surfaceGeneration)) break;
        // A SUBOPTIMAL old frame counts as a successful presentation, but the
        // replacement output is not acknowledged until it presents its own frame.
        const bool currentOutputPresented = rtFramePresented && !resourceRecreated;
        sceneTransition = sceneTransition || resourceRecreated;
        if (resourceRecreated) {
            gSwapchainContext.previewPerformance.BeginScope(++gSwapchainContext.previewEpoch);
            gSwapchainContext.previewWarmFrames = kMaxFramesInFlight;
            gSwapchainContext.previewGpuSamples = 0u;
        }
        if (resourceRecreated) PublishGraphicsApplied(gSwapchainContext, true, false);
        if (currentOutputPresented) PublishGraphicsApplied(gSwapchainContext, true, true);
        if (currentOutputPresented)
            gSwapchainContext.entryHandoff.Presented(GraphicsSceneForProfile(gSwapchainContext.sceneProfile),
                gSwapchainContext.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::EntryMenu &&
                    gSwapchainContext.entrySession.ReadyToPlay());
        PublishEntryState(gSwapchainContext, currentOutputPresented);
        if (currentOutputPresented && gSwapchainContext.entryWarmFrames > 0u)
            --gSwapchainContext.entryWarmFrames;
        if (currentOutputPresented && !gSwapchainContext.capabilities.rtScene.presented)
        {
            gSwapchainContext.capabilities.rtScene.presented = true;
            gSwapchainContext.capabilities.rtScene.executionBackend =
                gSwapchainContext.rtScene.ExecutionBackend();
            gSwapchainContext.capabilities.rtScene.status = "Presented via swapchain";
            gSwapchainContext.capabilities.rtScene.geometry = gSwapchainContext.sceneProfile ==
                horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview ? "Compact authored graphics preview with actual idle skeleton" :
                gSwapchainContext.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::EntryMenu ?
                "Compact hanging-lantern entry scene with physical fire and native controls" :
                "Complete Horde showcase route with sequential animated skeleton and staff-lit lich";
            gSwapchainContext.capabilities.rtScene.dispatchWidth = gSwapchainContext.rtScene.DispatchExtent().width;
            gSwapchainContext.capabilities.rtScene.dispatchHeight = gSwapchainContext.rtScene.DispatchExtent().height;
            gSwapchainContext.capabilities.performance.internalRenderWidth = gSwapchainContext.capabilities.rtScene.dispatchWidth;
            gSwapchainContext.capabilities.performance.internalRenderHeight = gSwapchainContext.capabilities.rtScene.dispatchHeight;
            auto& presentationDiagnostics = gSwapchainContext.capabilities.diagnostics;
            presentationDiagnostics.erase(std::remove(presentationDiagnostics.begin(), presentationDiagnostics.end(),
                                                       "Internal render resolution: not measured yet."),
                                          presentationDiagnostics.end());
            if (!gSurfaceSessions.Publish(gSwapchainContext.surfaceGeneration, 1)) break;

            PublishRuntimeReports(gSwapchainContext);
            __android_log_print(ANDROID_LOG_INFO, kTag, "RT frame reached Android swapchain presentation.");
            __android_log_print(ANDROID_LOG_INFO, kTag, "HORDE_SURFACE_PRESENTED generation=%llu",
                static_cast<unsigned long long>(gSwapchainContext.surfaceGeneration));
        }
        if (gSwapchainContext.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::Showcase)
            CaptureConsentedPlaytestFrameOnRenderOwner(gSwapchainContext, currentOutputPresented, measurementPaused);
        if (measurementPaused && gSwapchainContext.sceneProfile !=
                horde::vulkan::raytracing::RtSceneProfile::Showcase &&
            !gSwapchainContext.inAppBenchmark.IsRunning() &&
            !gSwapchainContext.captureActive && !gSwapchainContext.routeReplayActive)
        {
            const auto liveScene = gSwapchainContext.sceneProfile ==
                horde::vulkan::raytracing::RtSceneProfile::EntryMenu
                    ? horde::graphics::ForegroundPauseScene::EntryMenu
                    : horde::graphics::ForegroundPauseScene::GraphicsPreview;
            const int liveProfileCapHz = horde::graphics::ForegroundPauseRenderCadence::ResolveLiveProfileCapHz(
                liveScene, gSwapchainContext.graphicsSettings.previewFrameCap);
            const auto deadline = frameStart + std::chrono::microseconds(1000000 / liveProfileCapHz);
            while (gSwapchainRunning.load(std::memory_order_acquire) &&
                   std::chrono::steady_clock::now() < deadline)
            {
                const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                    deadline - std::chrono::steady_clock::now());
                std::this_thread::sleep_for(std::clamp(remaining, std::chrono::milliseconds{1},
                    horde::graphics::ForegroundPauseRenderCadence::kMaximumPollInterval));
            }
        }
        if (gSwapchainContext.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview)
        {
            const bool warming = gSwapchainContext.previewWarmFrames > 0u;
            if (warming) --gSwapchainContext.previewWarmFrames;
            std::optional<double> gpu;
            if (gSwapchainContext.gpuFrameTimingSampleCount > gSwapchainContext.previewGpuSamples)
            {
                gSwapchainContext.previewGpuSamples = gSwapchainContext.gpuFrameTimingSampleCount;
                if (!warming && !sceneTransition)
                    gpu = gSwapchainContext.gpuFrameTimer.Telemetry().latestMilliseconds;
            }
            (void)gSwapchainContext.previewPerformance.RecordFrame(
                std::chrono::duration<double>(std::chrono::steady_clock::now() - loopStart).count(),
                cpuRenderMs, gpu, rtFramePresented, sceneTransition || warming);
            const auto publishTime = std::chrono::steady_clock::now();
            if (publishTime - gSwapchainContext.previewLastPublish >= std::chrono::milliseconds(250))
            {
                const auto inventory = gSwapchainContext.rtScene.ResourceInventory();
                gSwapchainContext.previewPerformance.SetTrackedAllocations(inventory.deviceLocalBytes, inventory.hostVisibleBytes);
                gSwapchainContext.previewLastPublish = publishTime;
                std::lock_guard lock(gGraphicsMutex);
                if (gSurfaceSessions.IsCurrent(gSwapchainContext.surfaceGeneration))
                {
                    gPreviewPerformance = gSwapchainContext.previewPerformance.Snapshot();
                    gPreviewPerformanceGeneration = gSwapchainContext.surfaceGeneration;
                }
            }
        }
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
        if (gSwapchainContext.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::Showcase &&
            !gSwapchainContext.inAppBenchmark.IsRunning() &&
            frameStart - lastDeveloperOverlayPublish >= std::chrono::milliseconds(250))
        {
            PublishDeveloperOverlaySnapshot(gSwapchainContext);
            lastDeveloperOverlayPublish = frameStart;
        }
#endif
    }

    gSwapchainRunning.store(false, std::memory_order_release);

    {
        std::lock_guard<std::mutex> lock(gReportMutex);
        if (gPlaytestCapture.state != PlaytestCaptureRequest::State::Empty)
        {
            gPlaytestCapture.pixels = {};
            gPlaytestCapture.context = {};
            gPlaytestCapture.state = PlaytestCaptureRequest::State::Failed;
        }
    }

    const auto retiredGeneration = gSwapchainContext.surfaceGeneration;
#if HORDE_RT_ANDROID_MOTION_EVIDENCE
    FailAndroidMotion(gSwapchainContext, "Motion interrupted by surface/lifecycle retirement.");
    RestoreAndroidMotionEquipmentSeed(gSwapchainContext);
#endif
    const bool retired = DestroySwapchainContext(gSwapchainContext);
    gSurfaceRetirementBlocked.store(!retired, std::memory_order_release);
    if (!retired) gSurfaceSessions.Publish(retiredGeneration, 3);
}

bool StartSurfaceInternal(ANativeWindow* window,
                          horde::vulkan::DeviceCapabilities capabilities,
                          const std::string& reportDirectory,
                          const std::uint64_t generation)
{
    if (window == nullptr)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "startDiagnosticSurface called with null window.");
        return false;
    }

    if (gSwapchainRunning.load(std::memory_order_acquire) ||
        gSurfaceRetirementBlocked.load(std::memory_order_acquire))
    {
        __android_log_print(ANDROID_LOG_WARN, kTag, "Surface start refused while a renderer is active or retirement is unproved.");
        ANativeWindow_release(window);
        return false;
    }

    SwapchainContext context;
    context.window = window;
    context.surfaceGeneration = generation;
    struct Cleanup
    {
        SwapchainContext& context;
        bool transferred = false;
        ~Cleanup()
        {
            if (!transferred && context.window && !DestroySwapchainContext(context))
            {
                gSwapchainContext = std::move(context);
                gSurfaceRetirementBlocked.store(true, std::memory_order_release);
            }
        }
    } cleanup{context};
    context.capabilities = capabilities;
    context.reportDirectory = reportDirectory;
    const auto startupGraphics = ReadRequestedGraphics();
    context.graphicsSettings = startupGraphics.requested;
    context.graphicsRequested = startupGraphics.requested;
    context.graphicsSerial = startupGraphics.serial;
    context.renderScale = context.graphicsSettings.renderScalePercent / 100.0f;
    context.gpuFrameTimingEnabled = gRequestedGpuFrameTimingEnabled.load(std::memory_order_acquire);
    const bool requireRayQueryCompute = gRequiredRayQueryCompute.load(std::memory_order_acquire);
    context.clearColor = ClearColorForMode(capabilities.rtMode);
    __android_log_print(ANDROID_LOG_INFO,
                        kTag,
                        "HORDE_GPU_TIMING mode=%s rt_rendering=unchanged",
                        context.gpuFrameTimingEnabled ? "enabled" : "disabled");
    if (!gSurfaceSessions.IsCurrent(generation)) return false;
    // Refresh the probe snapshot off the UI thread even on later startup failure;
    // this does not certify an RT-presented frame.
    PublishRuntimeReports(context);
    ClearPlatformGameplayEvents();
    {
        std::lock_guard<std::mutex> inputLock(gInputPublisherMutex);
        PublishInputLocked();
    }

    if (!CreateInstance(context.instance, context.presentSurfaceSupport) || !gSurfaceSessions.IsCurrent(generation))
    {
        DestroySwapchainContext(context);
        return false;
    }

    if (!CreateSurface(context.instance, context.window, context.surface) || !gSurfaceSessions.IsCurrent(generation))
    {
        DestroySwapchainContext(context);
        return false;
    }

    context.physicalDevice = FindMatchingPhysicalDevice(context.instance, capabilities, context.surface);
    if (context.physicalDevice == VK_NULL_HANDLE || !gSurfaceSessions.IsCurrent(generation))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "No matching physical device found for Android diagnostic surface.");
        DestroySwapchainContext(context);
        return false;
    }

    if (!FindGraphicsAndPresentQueueFamily(context.physicalDevice, context.surface, context.graphicsQueueFamilyIndex) ||
        !gSurfaceSessions.IsCurrent(generation))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Could not find graphics+compute+present queue family on Android.");
        DestroySwapchainContext(context);
        return false;
    }

    // The selector's queue prerequisite is now proved for this exact device/surface.
    context.executionBackend = horde::vulkan::raytracing::SelectRtExecutionBackend(
        capabilities, true, requireRayQueryCompute);
    horde::vulkan::BeginRtBackendSelection(context.capabilities.rtScene, context.executionBackend);
    PublishRuntimeReports(context);
    if (requireRayQueryCompute &&
        context.executionBackend != horde::vulkan::RtExecutionBackend::RayQueryCompute)
    {
        __android_log_print(
            ANDROID_LOG_ERROR, kTag,
            "Required hardware RayQuery compute backend is unavailable; no alternate backend will be selected.");
        gSurfaceSessions.Publish(generation, 2);
        DestroySwapchainContext(context);
        return false;
    }
    context.useRtPath = context.executionBackend != horde::vulkan::RtExecutionBackend::Unsupported;
    gSurfaceSessions.Publish(generation, context.useRtPath ? 0 : 2);

    if (!CreateLogicalDevice(context.physicalDevice, context.instance, context.graphicsQueueFamilyIndex, context.executionBackend,
        context.device, context.graphicsQueue, context.presentSurfaceSupport, context.presentCompletionMode,
        context.presentTimingExtensionEnabled) ||
        !gSurfaceSessions.IsCurrent(generation))
    {
        DestroySwapchainContext(context);
        return false;
    }
#if HORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION
    if (context.presentTimingExtensionEnabled)
    {
        const auto timingQuery = reinterpret_cast<PFN_vkGetPastPresentationTimingGOOGLE>(
            vkGetDeviceProcAddr(context.device, "vkGetPastPresentationTimingGOOGLE"));
        if (timingQuery != nullptr)
        {
            try { context.presentTiming = std::make_unique<horde::vulkan::PresentTimingEvidence>(); }
            catch (...) { /* Optional evidence allocation failure leaves rendering available. */ }
            if (context.presentTiming && !context.presentTiming->Initialize(timingQuery)) context.presentTiming.reset();
        }
    }
    __android_log_print(ANDROID_LOG_INFO, kTag,
        "HORDE_PRESENT_TIMING extension_enabled=%d collector_enabled=%d desired_present_time_ns=0",
        context.presentTimingExtensionEnabled ? 1 : 0, context.presentTiming && context.presentTiming->Enabled() ? 1 : 0);
#endif
    const auto cacheSeed = context.useRtPath
        ? gPipelineCacheSeed.ForDevice(PipelineCacheDeviceIdentity(context.physicalDevice))
        : std::span<const std::byte>{};
    VkPipelineCacheCreateInfo pipelineCacheInfo{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    pipelineCacheInfo.initialDataSize = cacheSeed.size();
    pipelineCacheInfo.pInitialData = cacheSeed.empty() ? nullptr : cacheSeed.data();
    VkResult pipelineCacheResult = context.useRtPath
        ? vkCreatePipelineCache(context.device, &pipelineCacheInfo, nullptr, &context.pipelineCache)
        : VK_NOT_READY;
    const VkResult seedAttemptResult = pipelineCacheResult;
    const bool fallbackToEmpty = pipelineCacheResult != VK_SUCCESS && !cacheSeed.empty();
    if (fallbackToEmpty)
    {
        context.pipelineCache = VK_NULL_HANDLE;
        pipelineCacheInfo.initialDataSize = 0u;
        pipelineCacheInfo.pInitialData = nullptr;
        pipelineCacheResult = vkCreatePipelineCache(
            context.device, &pipelineCacheInfo, nullptr, &context.pipelineCache);
    }
    if (pipelineCacheResult != VK_SUCCESS) context.pipelineCache = VK_NULL_HANDLE;
    __android_log_print(pipelineCacheResult == VK_SUCCESS || !context.useRtPath
            ? ANDROID_LOG_INFO : ANDROID_LOG_WARN, kTag,
        "HORDE_PIPELINE_CACHE_CREATE attempted=%d result=%d available=%d generation=%llu "
        "seed_bytes=%zu seed_attempt_result=%d fallback_empty=%d seed_limit_bytes=%zu",
        context.useRtPath ? 1 : 0, static_cast<int>(pipelineCacheResult), context.pipelineCache != VK_NULL_HANDLE ? 1 : 0,
        static_cast<unsigned long long>(generation), cacheSeed.size(), static_cast<int>(seedAttemptResult),
        fallbackToEmpty ? 1 : 0, horde::vulkan::PipelineCacheSeed::kMaximumSize);
    if (context.executionBackend == horde::vulkan::RtExecutionBackend::RayTracingPipeline)
    {
        auto* cacheOwner = new (std::nothrow) AndroidCompiledPipelineCacheOwner(context.device);
        if (cacheOwner != nullptr)
            context.compiledPipelineCache.reset(cacheOwner);
        __android_log_print(cacheOwner != nullptr ? ANDROID_LOG_INFO : ANDROID_LOG_WARN, kTag,
            "HORDE_COMPILED_PIPELINE_CACHE_CREATE generation=%llu available=%d capacity=%zu",
            static_cast<unsigned long long>(generation), cacheOwner != nullptr ? 1 : 0,
            AndroidCompiledPipelineCache::kCapacity);
    }
    context.capabilities.diagnostics.push_back(horde::vulkan::PresentCompletionDiagnostic(context.presentCompletionMode));

    if (!CreateSwapchain(context) || !gSurfaceSessions.IsCurrent(generation))
    {
        DestroySwapchainContext(context);
        return false;
    }
    context.entryHandoff.Observe(ReadEntryControls(), generation);
    context.sceneProfile = DesiredProfile(context, ReadPreviewControls());
    context.entryHandoff.BeginLoad();
    PublishEntryState(context, false);
    if (!InitialiseRtSceneForSwapchain(context) || !gSurfaceSessions.IsCurrent(generation))
    {
        context.entryHandoff.FailLoad();
        PublishEntryState(context, false);
        DestroySwapchainContext(context);
        return false;
    }
    context.entryHandoff.EndLoad();
    PublishEntryState(context, false);

    gSwapchainContext = std::move(context);
    cleanup.transferred = true;
    gSwapchainRunning.store(true, std::memory_order_release);
    try { gSwapchainThread = std::thread(SwapchainRenderLoop); }
    catch (...)
    {
        gSwapchainRunning.store(false, std::memory_order_release);
        gSurfaceRetirementBlocked.store(!DestroySwapchainContext(gSwapchainContext), std::memory_order_release);
        throw;
    }

    return true;
}

void StopSurfaceInternal()
{
    if (gInAppBenchmarkStatus.load(std::memory_order_acquire) == 1)
    {
        gInAppBenchmarkStatus.store(3, std::memory_order_release);
        std::lock_guard<std::mutex> reportLock(gReportMutex);
        gLatestBenchmarkProgress = "BENCHMARK INTERRUPTED";
    }
    gInAppBenchmarkRequested.store(false, std::memory_order_release);
    gInAppBenchmarkCancelRequested.store(false, std::memory_order_release);
    gSwapchainRunning.store(false, std::memory_order_release);
    if (gSwapchainThread.joinable())
    {
        gSwapchainThread.join();
    }
    if (gSurfaceRetirementBlocked.load(std::memory_order_acquire))
        gSurfaceRetirementBlocked.store(!DestroySwapchainContext(gSwapchainContext), std::memory_order_release);
    // Home/surface teardown is an ownership boundary. Never replay an immediate
    // gameplay cue (including ChestUnlocked) into the next Activity surface.
    ClearPlatformGameplayEvents();
    PublishSimulationUiState();
}

// Process-lifetime serial lifecycle owner. It joins the previous render owner
// before touching Vulkan for another generation; gameplay remains on that
// existing render owner. Activity callbacks never wait for driver work or joins.
class NativeSurfaceOwner
{
public:
    NativeSurfaceOwner() : thread_([this] { Run(); }) {}
    ~NativeSurfaceOwner()
    {
        gSurfaceSessions.Close();
        thread_.join();
    }
private:
    void Run()
    {
        while (auto action = gSurfaceSessions.Take())
        {
            StopSurfaceInternal();
            if (gSurfaceRetirementBlocked.load(std::memory_order_acquire))
            {
                gSurfaceSessions.Publish(action->generation, 3);
                __android_log_print(ANDROID_LOG_ERROR, kTag,
                    "Surface lifecycle blocked until retained presentation resources can be retired.");
                continue;
            }
            if (!action->request || !gSurfaceSessions.IsCurrent(action->generation)) continue;
            // A pending cold start may be paused before driver work begins.
            gSurfaceSessions.WaitWhileSuspended(action->generation);
            if (!gSurfaceSessions.IsCurrent(action->generation)) continue;
            auto& request = *action->request;
            try
            {
                const auto capabilities = RunProbe();
                if (!gSurfaceSessions.IsCurrent(action->generation)) continue;
                if (!EnsureDirectoryExists(request.reportDirectory))
                {
                    gSurfaceSessions.Publish(action->generation, 3);
                    continue;
                }
                if (!StartSurfaceInternal(request.window.release(), capabilities,
                                          request.reportDirectory, action->generation))
                {
                    if (gSurfaceSessions.State(action->generation) == 0)
                        gSurfaceSessions.Publish(action->generation, 3);
                    continue;
                }
                __android_log_print(ANDROID_LOG_INFO, kTag,
                    "HORDE_SURFACE_STARTED generation=%llu", static_cast<unsigned long long>(action->generation));
            }
            catch (const std::exception& error)
            {
                StopSurfaceInternal();
                gSurfaceSessions.Publish(action->generation, 3);
                __android_log_print(ANDROID_LOG_ERROR, kTag, "Native surface startup failed: %s", error.what());
            }
        }
        StopSurfaceInternal();
    }
    std::thread thread_;
};

NativeSurfaceOwner& SurfaceOwner()
{
    // Constructed after the globals and destroyed before them. This join is
    // library/process teardown only, never Activity.onPause/onDestroy.
    static NativeSurfaceOwner owner;
    return owner;
}

} // namespace

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getGitHubReleaseRequestContract(
    JNIEnv* env,
    jclass)
{
    const horde::update::GitHubHttpRequest request = horde::update::BuildHordeGitHubReleaseRequest();
    std::ostringstream json;
    json << "{\"url\":" << JsonUtf8String(request.url)
         << ",\"accept\":" << JsonUtf8String(request.accept)
         << ",\"apiVersion\":" << JsonUtf8String(request.apiVersion)
         << ",\"userAgent\":" << JsonUtf8String(request.userAgent)
         << ",\"maximumResponseBytes\":" << request.maximumResponseBytes << '}';
    return NewUtf8ByteArray(env, json.str());
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_evaluateGitHubReleaseUpdate(
    JNIEnv* env,
    jclass,
    jstring installedVersion,
    jint httpStatus,
    jbyteArray responseBodyUtf8)
{
    if (installedVersion == nullptr || responseBodyUtf8 == nullptr)
    {
        return NewUtf8ByteArray(
            env, "{\"status\":\"error\",\"diagnostic\":\"Missing update-check input.\"}");
    }
    const char* installedUtf = env->GetStringUTFChars(installedVersion, nullptr);
    if (installedUtf == nullptr) return nullptr;
    const horde::update::GitHubHttpRequest request = horde::update::BuildHordeGitHubReleaseRequest();
    const jsize responseSize = env->GetArrayLength(responseBodyUtf8);
    std::string body;
    if (responseSize < 0)
    {
        env->ReleaseStringUTFChars(installedVersion, installedUtf);
        return nullptr;
    }
    const std::size_t boundedSize = std::min<std::size_t>(
        static_cast<std::size_t>(responseSize), request.maximumResponseBytes + 1u);
    body.resize(boundedSize);
    if (boundedSize != 0u)
    {
        env->GetByteArrayRegion(responseBodyUtf8, 0, static_cast<jsize>(boundedSize),
                                reinterpret_cast<jbyte*>(body.data()));
        if (env->ExceptionCheck())
        {
            env->ReleaseStringUTFChars(installedVersion, installedUtf);
            return nullptr;
        }
    }
    const std::string installed(installedUtf);
    env->ReleaseStringUTFChars(installedVersion, installedUtf);

    const horde::update::UpdateCheckResult result = horde::update::CheckForGitHubReleaseUpdate(
        installed,
        horde::update::ReleaseChannel::IncludePrerelease,
        [httpStatus, &body](const horde::update::GitHubHttpRequest&) {
            return horde::update::GitHubHttpResponse{static_cast<int>(httpStatus), body};
        });

    using horde::update::UpdateCheckStatus;
    const char* status = "error";
    if (result.status == UpdateCheckStatus::UpdateAvailable) status = "update-available";
    else if (result.status == UpdateCheckStatus::UpToDate ||
             result.status == UpdateCheckStatus::NoPublishedRelease)
    {
        status = "up-to-date";
    }

    std::ostringstream json;
    json << "{\"status\":" << JsonUtf8String(status)
         << ",\"installedVersion\":" << JsonUtf8String(result.installedVersion)
         << ",\"diagnostic\":" << JsonUtf8String(result.diagnostic);
    if (result.update)
    {
        const auto& update = *result.update;
        json << ",\"update\":{\"version\":" << JsonUtf8String(update.version)
             << ",\"tag\":" << JsonUtf8String(update.tag)
             << ",\"title\":" << JsonUtf8String(update.title)
             << ",\"notes\":" << JsonUtf8String(update.notes)
             << ",\"releasePageUrl\":" << JsonUtf8String(update.releasePageUrl)
             << ",\"prerelease\":" << (update.prerelease ? "true" : "false") << '}';
    }
    json << '}';
    return NewUtf8ByteArray(env, json.str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getTextReport(JNIEnv* env, jclass)
{
    std::string reportText = LatestTextReport();
    if (reportText.empty())
    {
        const horde::vulkan::DeviceCapabilities capabilities = RunProbe();
        PublishReportSnapshot(capabilities);
        reportText = LatestTextReport();
    }
    return env->NewStringUTF(reportText.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getJsonReport(JNIEnv* env, jclass)
{
    std::string reportJson = LatestJsonReport();
    if (reportJson.empty())
    {
        const horde::vulkan::DeviceCapabilities capabilities = RunProbe();
        PublishReportSnapshot(capabilities);
        reportJson = LatestJsonReport();
    }
    return env->NewStringUTF(reportJson.c_str());
}

static jbyteArray PrepareAndroidPlaytestReport(JNIEnv* env,
    jstring reportId, jstring capturedAtUtc, jint category, jint impact, jstring note,
    jboolean exportConsent, jboolean includeBasicContext, jstring rawModel,
    const bool submission, const bool includeScreenshot, const jlong captureToken, jbyteArray screenshotPng)
{
    using namespace horde::reporting;
    const auto reply = [env](const PlaytestReportStatus status, const std::string_view text) {
        std::string envelope(1u, static_cast<char>(status));
        envelope.append(text);
        return NewUtf8ByteArray(env, envelope);
    };
    try
    {
        if (exportConsent != JNI_TRUE)
            return reply(PlaytestReportStatus::ConsentRequired,
                PlaytestReportStatusName(PlaytestReportStatus::ConsentRequired));
        if (category < 0 || category > 5 || impact < 0 || impact > 3)
            return reply(PlaytestReportStatus::InvalidCategory, "Choose a valid category and impact.");
        const auto read = [env](jstring value, const std::size_t maximum, std::string& out) {
            if (!value) return PlaytestReportStatus::InvalidUtf8;
            const auto length = env->GetStringLength(value);
            if (static_cast<std::size_t>(length) > maximum) return PlaytestReportStatus::NoteTooLarge;
            std::vector<jchar> units(static_cast<std::size_t>(length));
            if (length) env->GetStringRegion(value, 0, length, units.data());
            if (env->ExceptionCheck()) return PlaytestReportStatus::InvalidUtf8;
            std::u16string text;
            text.reserve(units.size());
            for (const jchar unit : units) text.push_back(static_cast<char16_t>(unit));
            return EncodePlaytestReportUtf16(text, maximum, out);
        };
        std::string id, utc, text;
        for (const auto [value, maximum, out] : {
            std::tuple{reportId, std::size_t{96u}, &id},
            std::tuple{capturedAtUtc, std::size_t{30u}, &utc},
            std::tuple{note, kPlaytestReportMaxNoteBytes, &text}})
        {
            const auto status = read(value, maximum, *out);
            if (env->ExceptionCheck()) return nullptr;
            if (status != PlaytestReportStatus::Ready) return reply(status, PlaytestReportStatusName(status));
        }
        OwnedPlaytestReportContext context;
        std::uint32_t screenshotWidth = 0u, screenshotHeight = 0u;
        std::vector<std::uint8_t> png;
        if (includeScreenshot)
        {
            if (!submission || !screenshotPng || captureToken <= 0)
                return reply(PlaytestReportStatus::InvalidPreparedReport, "The game screenshot is unavailable. Nothing was submitted.");
            const auto pngLength = env->GetArrayLength(screenshotPng);
            if (pngLength <= 0 || static_cast<std::size_t>(pngLength) > kPlaytestScreenshotMaxBytes)
                return reply(PlaytestReportStatus::InvalidPreparedReport, "The screenshot exceeds the attachment limit. Nothing was submitted.");
            {
                std::lock_guard<std::mutex> lock(gReportMutex);
                if (gPlaytestCapture.token != static_cast<std::uint64_t>(captureToken) ||
                    gPlaytestCapture.state != PlaytestCaptureRequest::State::Transferred)
                    return reply(PlaytestReportStatus::InvalidPreparedReport, "The game capture was cancelled or expired. Nothing was submitted.");
                context = gPlaytestCapture.context;
                utc = gPlaytestCapture.capturedAtUtc;
                screenshotWidth = gPlaytestCapture.width; screenshotHeight = gPlaytestCapture.height;
            }
            png.resize(static_cast<std::size_t>(pngLength));
            env->GetByteArrayRegion(screenshotPng, 0, pngLength, reinterpret_cast<jbyte*>(png.data()));
            if (env->ExceptionCheck()) return nullptr;
        }
        else if (screenshotPng || captureToken != 0)
            return reply(PlaytestReportStatus::ConsentRequired, "Screenshot consent is required before capture.");
        if (includeBasicContext == JNI_TRUE)
        {
            if (!includeScreenshot) {
                std::lock_guard<std::mutex> lock(gReportMutex);
                if (gLatestReportSurfaceGeneration != 0u &&
                    gSurfaceSessions.IsCurrent(gLatestReportSurfaceGeneration))
                    context = gLatestPlaytestContext;
            }
            if (read(rawModel, kPlaytestReportMaxContextStringBytes, context.rawModel) != PlaytestReportStatus::Ready ||
                context.internalWidth == 0u || context.internalHeight == 0u)
            {
                if (env->ExceptionCheck()) return nullptr;
                return reply(PlaytestReportStatus::InvalidContext,
                    "Renderer context is unavailable. Uncheck basic context to export your note alone.");
            }
        }
        const PlaytestReportInput input{id, utc,
            static_cast<PlaytestReportCategory>(category), static_cast<PlaytestReportImpact>(impact),
            text, true, includeBasicContext == JNI_TRUE, context.View()};
        if (submission)
        {
            const auto prepared = PreparePlaytestSubmission(input, includeScreenshot,
                {png, screenshotWidth, screenshotHeight});
            if (!prepared.IsReady())
                return reply(prepared.reportStatus != PlaytestReportStatus::Ready ? prepared.reportStatus :
                    PlaytestReportStatus::InvalidPreparedReport, "Report or screenshot validation failed. Nothing was submitted.");
            if (includeScreenshot)
            {
                std::lock_guard<std::mutex> lock(gReportMutex);
                if (gPlaytestCapture.token == static_cast<std::uint64_t>(captureToken)) gPlaytestCapture = {};
            }
            return reply(PlaytestReportStatus::Ready, prepared.json);
        }
        const auto prepared = PreparePlaytestReport(input);
        return reply(prepared.status, prepared.IsReady() ? std::string_view(prepared.json)
            : PlaytestReportStatusName(prepared.status));
    }
    catch (...)
    {
        return nullptr; // No exception or allocation retry escapes the JNI boundary.
    }
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_preparePlaytestReport(JNIEnv* env, jclass,
    jstring reportId, jstring capturedAtUtc, jint category, jint impact, jstring note,
    jboolean exportConsent, jboolean includeBasicContext, jstring rawModel)
{
    return PrepareAndroidPlaytestReport(env, reportId, capturedAtUtc, category, impact, note,
        exportConsent, includeBasicContext, rawModel, false, false, 0, nullptr);
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_preparePlaytestSubmission(JNIEnv* env, jclass,
    jstring reportId, jstring capturedAtUtc, jint category, jint impact, jstring note,
    jboolean submissionConsent, jboolean includeBasicContext, jstring rawModel,
    jboolean includeScreenshot, jlong captureToken, jbyteArray screenshotPng)
{
    return PrepareAndroidPlaytestReport(env, reportId, capturedAtUtc, category, impact, note,
        submissionConsent, includeBasicContext, rawModel, true, includeScreenshot == JNI_TRUE,
        captureToken, screenshotPng);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestPlaytestCapture(JNIEnv*, jclass, jboolean screenshotConsent)
{
    if (screenshotConsent != JNI_TRUE || !gSwapchainRunning.load(std::memory_order_acquire) ||
        gSurfaceSessions.State() != 1) return 0;
    std::lock_guard<std::mutex> lock(gReportMutex);
    if (gPlaytestCapture.state != PlaytestCaptureRequest::State::Empty ||
        gPlaytestCaptureNextToken == static_cast<std::uint64_t>(std::numeric_limits<jlong>::max())) return 0;
    gPlaytestCapture.token = ++gPlaytestCaptureNextToken;
    gPlaytestCapture.state = PlaytestCaptureRequest::State::Pending;
    return static_cast<jlong>(gPlaytestCapture.token);
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_takePlaytestCapture(JNIEnv* env, jclass, jlong token)
{
    // status0 + little-endian width/height + owned RGBA;1=pending,2=unavailable.
    std::lock_guard<std::mutex> lock(gReportMutex);
    if (token <= 0 || gPlaytestCapture.token != static_cast<std::uint64_t>(token) ||
        gPlaytestCapture.state != PlaytestCaptureRequest::State::Ready)
    {
        const jbyte status = token > 0 && gPlaytestCapture.token == static_cast<std::uint64_t>(token) &&
            gPlaytestCapture.state == PlaytestCaptureRequest::State::Pending ? 1 : 2;
        auto result = env->NewByteArray(1);
        if (result) env->SetByteArrayRegion(result, 0, 1, &status);
        return result;
    }
    const auto& pixels = gPlaytestCapture.pixels;
    const auto length = static_cast<jsize>(pixels.rgba.size() + 9u);
    auto result = env->NewByteArray(length);
    if (!result) return nullptr;
    std::array<jbyte, 9u> header{};
    for (unsigned byte = 0u; byte < 4u; ++byte)
    {
        header[1u + byte] = static_cast<jbyte>(pixels.width >> (byte * 8u));
        header[5u + byte] = static_cast<jbyte>(pixels.height >> (byte * 8u));
    }
    env->SetByteArrayRegion(result, 0, 9, header.data());
    env->SetByteArrayRegion(result, 9, length - 9, reinterpret_cast<const jbyte*>(pixels.rgba.data()));
    if (env->ExceptionCheck()) return nullptr;
    gPlaytestCapture.pixels = {};
    gPlaytestCapture.state = PlaytestCaptureRequest::State::Transferred;
    return result;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_cancelPlaytestCapture(JNIEnv*, jclass, jlong token)
{
    std::lock_guard<std::mutex> lock(gReportMutex);
    if (token > 0 && gPlaytestCapture.token == static_cast<std::uint64_t>(token)) gPlaytestCapture = {};
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getDeveloperOverlayText(JNIEnv* env, jclass)
{
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
    const std::string text = LatestDeveloperOverlayText();
    return env->NewStringUTF(text.c_str());
#else
    return env->NewStringUTF("");
#endif
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_writeReports(JNIEnv* env, jclass, jstring baseDirectory)
{
    const char* baseDirectoryUtf = env->GetStringUTFChars(baseDirectory, nullptr);
    if (!baseDirectoryUtf)
    {
        return JNI_FALSE;
    }

    const std::string baseDirectoryValue(baseDirectoryUtf);
    env->ReleaseStringUTFChars(baseDirectory, baseDirectoryUtf);

    const horde::vulkan::DeviceCapabilities capabilities = RunProbe();
    const std::string textReport = BuildDisplayText(capabilities);
    const std::string jsonReport = horde::vulkan::BuildCapabilityJsonReport(capabilities);
    PublishReportSnapshot(capabilities);

    const std::string reportDirectory = BuildReportDirectory(baseDirectoryValue);
    if (!EnsureDirectoryExists(reportDirectory))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to create report directory: %s", reportDirectory.c_str());
        return JNI_FALSE;
    }

    if (!WriteTextFile(reportDirectory + '/' + kTextReportFilename, textReport))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to write text report file.");
        return JNI_FALSE;
    }

    if (!WriteTextFile(reportDirectory + '/' + kJsonReportFilename, jsonReport))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to write JSON report file.");
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_startDiagnosticSurface(JNIEnv* env, jclass, jobject surface, jstring baseDirectory)
{
    if (surface == nullptr)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "startDiagnosticSurface called with null Java Surface.");
        return JNI_FALSE;
    }

    if (baseDirectory == nullptr)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "startDiagnosticSurface called with null base directory.");
        return JNI_FALSE;
    }

    const char* baseDirectoryUtf = env->GetStringUTFChars(baseDirectory, nullptr);
    if (!baseDirectoryUtf)
    {
        return JNI_FALSE;
    }

    const std::string baseDirectoryValue(baseDirectoryUtf);
    env->ReleaseStringUTFChars(baseDirectory, baseDirectoryUtf);

    const std::string reportDirectory = BuildReportDirectory(baseDirectoryValue);
    // Acquire the native reference before enqueueing; never retain a Java Surface
    // or JNIEnv across threads. Pending replacements release their own reference.
    std::unique_ptr<ANativeWindow, NativeWindowRelease> window(ANativeWindow_fromSurface(env, surface));
    if (!window)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to resolve ANativeWindow.");
        return JNI_FALSE;
    }

    try
    {
        (void)SurfaceOwner();
        std::uint64_t generation = 0u;
        {
            std::lock_guard<std::mutex> lock(gInputPublisherMutex);
            RequestLifecyclePauseSynchronizationLocked(false);
            generation = gSurfaceSessions.Start({std::move(window), reportDirectory});
        }
        {
            std::lock_guard lock(gGraphicsMutex);
            gPreviewPerformance = {};
            gPreviewPerformanceGeneration = 0u;
        }
        __android_log_print(ANDROID_LOG_INFO, kTag, "HORDE_SURFACE_REQUEST generation=%llu",
                            static_cast<unsigned long long>(generation));
        return static_cast<jlong>(generation); // Accepted request, NOT presented/ready.
    }
    catch (const std::exception& error)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Could not request native surface: %s", error.what());
        return 0;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_stopDiagnosticSurface(JNIEnv*, jclass, jlong generation)
{
    std::lock_guard<std::mutex> lock(gInputPublisherMutex);
    if (generation > 0 && gSurfaceSessions.Stop(static_cast<std::uint64_t>(generation)))
    {
        RequestLifecyclePauseSynchronizationLocked(false);
        __android_log_print(ANDROID_LOG_INFO, kTag, "HORDE_SURFACE_CANCEL generation=%llu",
                            static_cast<unsigned long long>(generation));
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setDiagnosticSurfaceSuspended(
    JNIEnv*, jclass, jlong generation, jboolean suspended)
{
    std::lock_guard<std::mutex> lock(gInputPublisherMutex);
    const bool accepted = generation > 0 && gSurfaceSessions.SetSuspended(
        static_cast<std::uint64_t>(generation), suspended == JNI_TRUE);
    if (accepted && suspended == JNI_TRUE)
        RequestLifecyclePauseSynchronizationLocked(false); // Discard lifecycle-stale commands.
    __android_log_print(ANDROID_LOG_INFO, kTag,
        "HORDE_SURFACE_SUSPEND_REQUEST generation=%lld suspended=%d accepted=%d",
        static_cast<long long>(generation), suspended == JNI_TRUE ? 1 : 0, accepted ? 1 : 0);
    return accepted ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getSurfaceRuntimeState(JNIEnv*, jclass, jlong generation)
{
    return static_cast<jint>(gSurfaceSessions.State(static_cast<std::uint64_t>(generation)));
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setViewControls(JNIEnv*, jclass, jfloat yaw, jfloat pitch, jfloat torchLightStrength, jfloat moveStrafe, jfloat moveForward)
{
    std::lock_guard<std::mutex> lock(gInputPublisherMutex);
    gInputPublisherState.yawRadians = static_cast<float>(yaw);
    gInputPublisherState.pitchRadians = std::clamp(static_cast<float>(pitch), -0.32f, 0.28f);
    gInputPublisherState.torchLightStrength =
        std::clamp(static_cast<float>(torchLightStrength), 0.65f, 2.4f);
    gInputPublisherState.moveStrafe = std::clamp(static_cast<float>(moveStrafe), -1.0f, 1.0f);
    gInputPublisherState.moveForward = std::clamp(static_cast<float>(moveForward), -1.0f, 1.0f);
    PublishInputLocked();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestAttack(JNIEnv*, jclass)
{
    std::lock_guard<std::mutex> lock(gInputPublisherMutex);
    if (gInputPublisherState.commands.attack != UINT64_MAX)
    {
        ++gInputPublisherState.commands.attack;
        horde::gameplay::simulation::RecordCombatInputEdge(gInputPublisherState,
            horde::gameplay::simulation::CombatInputEdgeKind::Attack,
            horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr));
    }
    PublishInputLocked();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestParry(JNIEnv*, jclass)
{
    std::lock_guard<std::mutex> lock(gInputPublisherMutex);
    if (gInputPublisherState.commands.parry != UINT64_MAX)
    {
        ++gInputPublisherState.commands.parry;
        horde::gameplay::simulation::RecordCombatInputEdge(gInputPublisherState,
            horde::gameplay::simulation::CombatInputEdgeKind::Parry,
            horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr));
    }
    PublishInputLocked();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestDodge(JNIEnv*, jclass)
{
    std::lock_guard<std::mutex> lock(gInputPublisherMutex);
    if (gInputPublisherState.commands.dodge != UINT64_MAX)
    {
        ++gInputPublisherState.commands.dodge;
        horde::gameplay::simulation::RecordCombatInputEdge(gInputPublisherState,
            horde::gameplay::simulation::CombatInputEdgeKind::Dodge,
            horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr));
    }
    PublishInputLocked();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestInteract(JNIEnv*, jclass)
{
    std::lock_guard<std::mutex> lock(gInputPublisherMutex);
    if (gInputPublisherState.commands.interact != UINT64_MAX)
    {
        ++gInputPublisherState.commands.interact;
    }
    PublishInputLocked();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestToggleHeldLightPose(JNIEnv*, jclass)
{
    std::lock_guard<std::mutex> lock(gInputPublisherMutex);
    if (gInputPublisherState.commands.toggleHeldLightPose != UINT64_MAX)
    {
        ++gInputPublisherState.commands.toggleHeldLightPose;
    }
    PublishInputLocked();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestRouteReset(JNIEnv*, jclass)
{
    gRtLabState.Reset();
    gRtLabUnlockEligible.store(false, std::memory_order_release);
    gRtLabBenchmarkRoute.store(false, std::memory_order_release);
    std::lock_guard<std::mutex> lock(gInputPublisherMutex);
    if (gInputPublisherState.commands.routeReset != UINT64_MAX)
    {
        ++gInputPublisherState.commands.routeReset;
    }
    PublishInputLocked();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getPlayerVitality(JNIEnv*, jclass)
{
    return static_cast<jint>(static_cast<std::uint32_t>(gPlayerVitalityState.load(std::memory_order_acquire)));
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getPlayerVitalityState(JNIEnv*, jclass)
{
    return static_cast<jlong>(gPlayerVitalityState.load(std::memory_order_acquire));
}

extern "C" JNIEXPORT jint JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getPlayerLifePhase(JNIEnv*, jclass)
{
    return static_cast<jint>(gPlayerLifePhase.load(std::memory_order_acquire));
}

extern "C" JNIEXPORT jint JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getFinaleEndingPhase(JNIEnv*, jclass)
{
    return static_cast<jint>(gFinaleEndingPhase.load(std::memory_order_acquire));
}

extern "C" JNIEXPORT jint JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getContextualControlState(JNIEnv*, jclass)
{
    return static_cast<jint>(gContextualControlState.load(std::memory_order_acquire));
}
extern "C" JNIEXPORT jint JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_retryEncounter(JNIEnv*, jclass)
{
    if (gSurfaceSessions.State() != 1 ||
        gPlayerLifePhase.load(std::memory_order_acquire) !=
            static_cast<int>(horde::gameplay::PlayerLifePhase::Dead))
    {
        return -1;
    }
    const std::int32_t checkpoint = gPlayerRetryCheckpoint.load(std::memory_order_acquire);
    if (checkpoint != 0 && checkpoint != 9)
    {
        return -1;
    }
    {
        std::lock_guard<std::mutex> lock(gInputPublisherMutex);
        if (gInputPublisherState.commands.retry != UINT64_MAX)
        {
            ++gInputPublisherState.commands.retry;
        }
        PublishInputLocked();
    }
    return static_cast<jint>(checkpoint);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setSimulationPaused(JNIEnv*, jclass, jboolean paused)
{
    std::lock_guard<std::mutex> lock(gInputPublisherMutex);
    RequestLifecyclePauseSynchronizationLocked(paused != JNI_TRUE,
        horde::gameplay::simulation::PausedInputPolicy::PreserveWorldCommands);
}

namespace
{
horde::graphics::GraphicsSettings GraphicsTuple(const jint scale, const jint water, const jint fire, const jint cap, const jboolean glass, const jint shadow, const jboolean mist, const jint dust)
{
    if (water < 0 || water > 2 || fire < 0 || fire > 2 || shadow < 0 || shadow > 2 || dust < 0 || dust > 2)
        return {0}; // Reject before narrowing enum storage.
    return {scale, static_cast<horde::graphics::WaterQuality>(water),
        static_cast<horde::graphics::FireDetail>(fire), cap, glass == JNI_TRUE,
        static_cast<horde::graphics::ShadowQuality>(shadow), mist == JNI_TRUE,
        static_cast<horde::graphics::DustQuality>(dust)};
}
void PublishGraphicsCommandLocked(const horde::graphics::GraphicsCommand& command,
                                 const std::uint64_t receivedNs)
{
    gRequestedGraphics = command;
    gGraphicsSerial = command.serial;
    gPublishedGraphicsLatency = {};
    gPublishedGraphicsLatency.serial = command.serial;
    gPublishedGraphicsLatency.receivedNs = receivedNs;
    gPublishedGraphicsLatency.publishedNs = GraphicsSteadyNs();
}
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setEntryMenu(
    JNIEnv*, jclass, jboolean enabled, jboolean sidePage, jboolean reducedMotion,
    jboolean play, jlong generation)
{
    if (generation < 0 || (generation != 0 &&
        !gSurfaceSessions.IsCurrent(static_cast<std::uint64_t>(generation)))) return;
    std::lock_guard lock(gGraphicsMutex);
    if (generation != 0 && !gSurfaceSessions.IsCurrent(static_cast<std::uint64_t>(generation))) return;
    // Zero is only a pre-surface launch/recovery request. A live generation
    // must use its accepted identity, including cancellation of a Play fade.
    if (generation == 0 && gEntryState[0] > 0 &&
        gSurfaceSessions.IsCurrent(static_cast<std::uint64_t>(gEntryState[0]))) return;
    const bool nextEnabled = enabled == JNI_TRUE;
    const bool nextPlay = nextEnabled && play == JNI_TRUE;
    const bool retryFailedEntry = generation != 0 && nextPlay && !gEntryControls.play &&
        gEntryState[0] == generation && gEntryState[2] == 4;
    if (nextEnabled != gEntryControls.enabled || (gEntryControls.play && !nextPlay) || retryFailedEntry)
    {
        ++gEntryControls.resetSerial;
        gEntryState[4] = 0;
        gAppliedGraphics.rtPresented = false;
    }
    gEntryControls.enabled = nextEnabled;
    gEntryControls.sidePage = sidePage == JNI_TRUE;
    gEntryControls.reducedMotion = reducedMotion == JNI_TRUE;
    gEntryControls.play = nextPlay;
    gEntryControls.generation = static_cast<std::uint64_t>(generation);
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getEntryMenuState(JNIEnv* env, jclass)
{
    std::lock_guard lock(gGraphicsMutex);
    if (gEntryState[0] <= 0 || !gSurfaceSessions.IsCurrent(static_cast<std::uint64_t>(gEntryState[0])))
        return env->NewLongArray(0);
    auto result = env->NewLongArray(static_cast<jsize>(gEntryState.size()));
    if (result) env->SetLongArrayRegion(result, 0, static_cast<jsize>(gEntryState.size()), gEntryState.data());
    return result;
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getMenuAmbienceState(JNIEnv* env, jclass)
{
    std::lock_guard lock(gGraphicsMutex);
    if (gMenuAmbienceState[0] <= 0 || !gSurfaceSessions.IsCurrent(static_cast<std::uint64_t>(gMenuAmbienceState[0])))
        return env->NewLongArray(0);
    auto result = env->NewLongArray(static_cast<jsize>(gMenuAmbienceState.size()));
    if (result) env->SetLongArrayRegion(result, 0, static_cast<jsize>(gMenuAmbienceState.size()), gMenuAmbienceState.data());
    return result;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setGraphicsPreview(
    JNIEnv*, jclass, jboolean enabled, jboolean paused, jboolean motion, jint camera, jboolean reset, jlong generation)
{
    if ((generation == 0 && enabled == JNI_TRUE) ||
        (generation != 0 && !gSurfaceSessions.IsCurrent(static_cast<std::uint64_t>(generation)))) return;
    std::lock_guard lock(gGraphicsMutex);
    gPreviewControls.enabled = enabled == JNI_TRUE;
    gPreviewControls.paused = paused == JNI_TRUE;
    gPreviewControls.motion = motion == JNI_TRUE;
    gPreviewControls.camera = std::clamp(static_cast<int>(camera), 0, 5);
    gPreviewControls.generation = static_cast<std::uint64_t>(generation);
    if (reset == JNI_TRUE) ++gPreviewControls.resetSerial;
    if (gPreviewControls.enabled != (gAppliedGraphics.scene == horde::graphics::GraphicsScene::Preview))
    {
        gAppliedGraphics.rtPresented = false;
        gEntryState[4] = 0;
    }
}

extern "C" JNIEXPORT jdoubleArray JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getGraphicsPreviewPerformance(JNIEnv* env, jclass)
{
    std::lock_guard lock(gGraphicsMutex);
    if (gPreviewPerformanceGeneration == 0u ||
        gPreviewPerformanceGeneration != gAppliedGraphics.lifecycleGeneration ||
        gPreviewPerformanceGeneration != gPreviewControls.generation ||
        !gSurfaceSessions.IsCurrent(gPreviewPerformanceGeneration)) return env->NewDoubleArray(0);
    const auto& a = gPreviewPerformance;
    std::array<jdouble, 9u + 256u> values{};
    values[0] = static_cast<double>(a.scopeEpoch); values[1] = a.successfulPresentsPerSecond;
    values[2] = a.meanLoopMilliseconds; values[3] = a.meanCpuRenderMilliseconds;
    values[4] = a.meanGpuMilliseconds.value_or(-1.0);
    values[5] = a.trackedDeviceLocalBytes ? static_cast<double>(*a.trackedDeviceLocalBytes) : -1.0;
    values[6] = a.trackedHostVisibleBytes ? static_cast<double>(*a.trackedHostVisibleBytes) : -1.0;
    values[7] = static_cast<double>(a.transitionCount); values[8] = static_cast<double>(a.sampleCount);
    for (std::size_t i = 0u; i < a.sampleCount; ++i)
    { values[9u + i * 2u] = a.samples[i].loopMilliseconds; values[10u + i * 2u] = a.samples[i].transition ? 1.0 : 0.0; }
    const auto length = static_cast<jsize>(9u + a.sampleCount * 2u);
    auto result = env->NewDoubleArray(length);
    if (result) env->SetDoubleArrayRegion(result, 0, length, values.data());
    return result;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getKeeperRevealTitleOpacity(JNIEnv*, jclass)
{
    return gKeeperTitleOpacity.load(std::memory_order_acquire);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setGraphicsSettings(JNIEnv*, jclass, jint scale, jint water, jint fire, jint cap, jboolean glass, jint shadow, jboolean mist, jint dust)
{
    const auto receivedNs = GraphicsSteadyNs();
    const auto settings = GraphicsTuple(scale, water, fire, cap, glass, shadow, mist, dust);
    if (!horde::graphics::ValidGraphicsSettings(settings)) return;
    std::lock_guard lock(gGraphicsMutex);
    PublishGraphicsCommandLocked({gGraphicsSerial + 1u, 0u,
        horde::graphics::GraphicsCommandKind::Revert, settings}, receivedNs);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_beginGraphicsEdit(JNIEnv*, jclass, jint scale, jint water, jint fire, jint cap, jboolean glass, jint shadow, jboolean mist, jint dust)
{
    const auto settings = GraphicsTuple(scale, water, fire, cap, glass, shadow, mist, dust);
    if (!horde::graphics::ValidGraphicsSettings(settings)) return;
    std::lock_guard lock(gGraphicsMutex);
    gGraphicsEdit.emplace(settings, gGraphicsSerial);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_applyGraphicsSettings(
    JNIEnv*, jclass, jint scale, jint water, jint fire, jint cap, jboolean glass, jint shadow, jboolean mist, jint dust, jlong generation)
{
    const auto receivedNs = GraphicsSteadyNs();
    if (!gSurfaceSessions.IsCurrent(static_cast<std::uint64_t>(generation))) return 0;
    std::lock_guard lock(gGraphicsMutex);
    if (!gGraphicsEdit || !gGraphicsEdit->Stage(GraphicsTuple(scale, water, fire, cap, glass, shadow, mist, dust))) return 0;
    const auto command = gGraphicsEdit->RequestApply(static_cast<std::uint64_t>(generation));
    if (!command) return 0;
    PublishGraphicsCommandLocked(*command, receivedNs);
    return static_cast<jlong>(command->serial);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_compareGraphicsPreview(
    JNIEnv*, jclass, jint scale, jint water, jint fire, jint cap, jboolean glass, jint shadow, jboolean mist, jint dust, jlong generation)
{
    const auto receivedNs = GraphicsSteadyNs();
    const auto settings = GraphicsTuple(scale, water, fire, cap, glass, shadow, mist, dust);
    if (!horde::graphics::ValidGraphicsSettings(settings) ||
        !gSurfaceSessions.IsCurrent(static_cast<std::uint64_t>(generation))) return 0;
    std::lock_guard lock(gGraphicsMutex);
    if (!gGraphicsEdit || !gPreviewControls.enabled ||
        gGraphicsEdit->State() == horde::graphics::GraphicsEditState::Applying ||
        gGraphicsEdit->State() == horde::graphics::GraphicsEditState::AwaitingConfirmation ||
        gGraphicsEdit->State() == horde::graphics::GraphicsEditState::Reverting) return 0;
    const auto confirmed = gGraphicsEdit->Committed();
    PublishGraphicsCommandLocked({gGraphicsSerial + 1u, static_cast<std::uint64_t>(generation),
        horde::graphics::GraphicsCommandKind::Apply, settings}, receivedNs);
    // A/B is a transient preview request. Explicit Apply/Confirm is required to
    // persist it. Restore serials must remain newer than every comparison.
    gGraphicsEdit.emplace(confirmed, gGraphicsSerial);
    return static_cast<jlong>(gGraphicsSerial);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_revertGraphicsSettings(JNIEnv*, jclass, jlong generation)
{
    const auto receivedNs = GraphicsSteadyNs();
    if (!gSurfaceSessions.IsCurrent(static_cast<std::uint64_t>(generation))) return 0;
    std::lock_guard lock(gGraphicsMutex);
    if (!gGraphicsEdit) return 0;
    const auto command = gGraphicsEdit->RequestRevert(static_cast<std::uint64_t>(generation));
    if (!command) return 0;
    PublishGraphicsCommandLocked(*command, receivedNs);
    return static_cast<jlong>(command->serial);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_confirmGraphicsSettings(JNIEnv*, jclass, jlong serial, jlong generation)
{
    std::lock_guard lock(gGraphicsMutex);
    if (gSurfaceSessions.State(static_cast<std::uint64_t>(generation)) != 1) return JNI_FALSE;
    if (!gGraphicsEdit || !gAppliedGraphics.rtPresented || gAppliedGraphics.serial != static_cast<std::uint64_t>(serial) ||
        gAppliedGraphics.lifecycleGeneration != static_cast<std::uint64_t>(generation)) return JNI_FALSE;
    return gGraphicsEdit->Confirm() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_advanceGraphicsConfirmation(
    JNIEnv*, jclass, jdouble seconds, jboolean foreground, jlong generation)
{
    const auto receivedNs = GraphicsSteadyNs();
    if (!gSurfaceSessions.IsCurrent(static_cast<std::uint64_t>(generation))) return 0;
    std::lock_guard lock(gGraphicsMutex);
    if (!gGraphicsEdit) return 0;
    const auto command = gGraphicsEdit->AdvanceConfirmation(seconds, foreground == JNI_TRUE,
        static_cast<std::uint64_t>(generation));
    if (!command) return 0;
    PublishGraphicsCommandLocked(*command, receivedNs);
    return static_cast<jlong>(command->serial);
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getGraphicsSnapshot(JNIEnv* env, jclass)
{
    std::lock_guard lock(gGraphicsMutex);
    const auto& a = gAppliedGraphics;
    const jlong values[] = {static_cast<jlong>(a.serial), static_cast<jlong>(a.lifecycleGeneration),
        gGraphicsEdit ? static_cast<jlong>(gGraphicsEdit->State()) : 0,
        a.effective.renderScalePercent, static_cast<jlong>(a.effective.waterQuality),
        static_cast<jlong>(a.effective.fireDetail), a.effective.previewFrameCap,
        a.internalExtent.width, a.internalExtent.height, a.outputExtent.width, a.outputExtent.height,
        static_cast<jlong>(a.opticalProfile), static_cast<jlong>(a.backend), a.rtPresented ? 1 : 0,
        static_cast<jlong>(a.reasons), a.requested.renderScalePercent, static_cast<jlong>(a.requested.waterQuality),
        static_cast<jlong>(a.requested.fireDetail), a.requested.previewFrameCap, static_cast<jlong>(a.scene),
        a.effective.glassEnabled ? 1 : 0, a.requested.glassEnabled ? 1 : 0,
        static_cast<jlong>(a.effective.shadowQuality), static_cast<jlong>(a.requested.shadowQuality),
        a.effective.mistEnabled ? 1 : 0, a.requested.mistEnabled ? 1 : 0,
        static_cast<jlong>(a.effective.dustQuality), static_cast<jlong>(a.requested.dustQuality)};
    auto result = env->NewLongArray(static_cast<jsize>(std::size(values)));
    if (result) env->SetLongArrayRegion(result, 0, static_cast<jsize>(std::size(values)), values);
    return result;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setRenderScale(JNIEnv*, jclass, jfloat scale)
{
    if (!std::isfinite(scale)) return;
    std::lock_guard lock(gGraphicsMutex);
    gRequestedGraphics.requested.renderScalePercent = horde::graphics::ClampGraphicsRenderScalePercent(
        static_cast<int>(std::lround(std::clamp(scale, 0.0f, 1.0f) * 100.0f)));
    gRequestedGraphics.serial = ++gGraphicsSerial;
    gRequestedGraphics.lifecycleGeneration = 0u;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setWaterQuality(JNIEnv*, jclass, jint quality)
{
    std::lock_guard lock(gGraphicsMutex);
    gRequestedGraphics.requested.waterQuality = static_cast<horde::graphics::WaterQuality>(std::clamp(static_cast<int>(quality), 0, 2));
    gRequestedGraphics.requested.fireDetail = quality == 2 ? horde::graphics::FireDetail::High : horde::graphics::FireDetail::Mobile;
    gRequestedGraphics.serial = ++gGraphicsSerial;
    gRequestedGraphics.lifecycleGeneration = 0u;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setRtSceneTuning(
    JNIEnv*,
    jclass,
    jfloat waterfallWidthScale,
    jboolean roofOverrideEnabled,
    jfloat roofOpen,
    jboolean dawnOverrideEnabled,
    jfloat dawnReveal,
    jfloat fogDensityScale)
{
    gRtLabState.SetScene(
        static_cast<float>(waterfallWidthScale),
        roofOverrideEnabled == JNI_TRUE,
        static_cast<float>(roofOpen),
        dawnOverrideEnabled == JNI_TRUE,
        static_cast<float>(dawnReveal),
        static_cast<float>(fogDensityScale));
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setRtLightTuning(
    JNIEnv*, jclass, jint group, jfloat hueDegrees, jfloat intensityScale)
{
    if (group < 0) return;
    gRtLabState.SetLight(
        static_cast<std::uint32_t>(group),
        static_cast<float>(hueDegrees),
        static_cast<float>(intensityScale));
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setRtFireTuning(
    JNIEnv*, jclass, jfloat strengthScale, jfloat turbulenceScale, jfloat smokeScale)
{
    gRtLabState.SetFire(
        static_cast<float>(strengthScale),
        static_cast<float>(turbulenceScale),
        static_cast<float>(smokeScale));
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setRtGlassTuning(
    JNIEnv*, jclass, jboolean visible, jfloat transmission, jfloat ior, jfloat roughness)
{
    gRtLabState.SetGlass(
        visible == JNI_TRUE,
        static_cast<float>(transmission),
        static_cast<float>(ior),
        static_cast<float>(roughness));
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setRtWorkloadPreset(JNIEnv*, jclass, jint preset)
{
    gRtLabState.SetWorkload(static_cast<std::int32_t>(preset));
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_resetRtSceneTuning(JNIEnv*, jclass)
{
    gRtLabState.Reset();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_markRtLabDebugAutomation(JNIEnv*, jclass)
{
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
    gRtLabDebugAutomationSession.store(true, std::memory_order_release);
#endif
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_isRtLabUnlockEligible(JNIEnv*, jclass)
{
    const bool eligible = gRtLabUnlockEligible.load(std::memory_order_acquire) &&
        !gRtLabDebugAutomationSession.load(std::memory_order_acquire) &&
        !gRtLabBenchmarkRoute.load(std::memory_order_acquire);
    return eligible ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getRtGpuFrameTimeMilliseconds(JNIEnv*, jclass)
{
    return static_cast<jfloat>(gRtLabGpuFrameTimeMs.load(std::memory_order_acquire));
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getRtGpuSampleCount(JNIEnv*, jclass)
{
    return static_cast<jlong>(gRtLabGpuSampleCount.load(std::memory_order_acquire));
}

extern "C" JNIEXPORT jint JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getCurrentRenderScalePercent(JNIEnv*, jclass)
{
    std::lock_guard lock(gGraphicsMutex);
    return gAppliedGraphics.rtPresented ? gAppliedGraphics.effective.renderScalePercent : 0;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getCurrentWaterQuality(JNIEnv*, jclass)
{
    std::lock_guard lock(gGraphicsMutex);
    return gAppliedGraphics.rtPresented ? static_cast<jint>(gAppliedGraphics.effective.waterQuality) : -1;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setGpuTimingEnabled(JNIEnv*, jclass, jboolean enabled)
{
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
    gRequestedGpuFrameTimingEnabled.store(enabled == JNI_TRUE, std::memory_order_release);
#else
    (void)enabled;
    gRequestedGpuFrameTimingEnabled.store(true, std::memory_order_release);
#endif
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setRequiredRayQueryCompute(
    JNIEnv*, jclass, jboolean required)
{
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
    gRequiredRayQueryCompute.store(required == JNI_TRUE, std::memory_order_release);
#else
    (void)required;
    gRequiredRayQueryCompute.store(false, std::memory_order_release);
#endif
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_finishDebugMotionEvidence(JNIEnv*, jclass)
{
#if defined(HORDE_RT_DEBUG_CHECKPOINTS) && !defined(NDEBUG)
    const int status = gMotionStatus.load(std::memory_order_acquire);
    if (status != 3 && status != 4) return JNI_FALSE;
    gMotionReleaseRequested.store(true, std::memory_order_release);
    return JNI_TRUE;
#else
    return JNI_FALSE;
#endif
}

extern "C" JNIEXPORT jint JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getDebugMotionEvidenceStatus(JNIEnv*, jclass)
{
#if defined(HORDE_RT_DEBUG_CHECKPOINTS) && !defined(NDEBUG)
    return gMotionStatus.load(std::memory_order_acquire);
#else
    return 0;
#endif
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestDebugMotionEvidence(
    JNIEnv* env, jclass, jstring scenarioName, jstring runId)
{
#if defined(HORDE_RT_DEBUG_CHECKPOINTS) && !defined(NDEBUG)
    if (!scenarioName || !runId || env->GetStringLength(scenarioName) > 64 || env->GetStringLength(runId) > 64 ||
        gSurfaceSessions.State() != 1 || gInAppBenchmarkStatus.load(std::memory_order_acquire) == 1)
        return JNI_FALSE;
    const char* scenarioText = env->GetStringUTFChars(scenarioName, nullptr);
    if (!scenarioText) return JNI_FALSE;
    horde::gameplay::validation::MotionScenario scenario{};
    const bool admitted = horde::gameplay::validation::ParseMotionScenario(scenarioText, scenario);
    env->ReleaseStringUTFChars(scenarioName, scenarioText);
    const char* idText = env->GetStringUTFChars(runId, nullptr);
    if (!idText) return JNI_FALSE;
    const std::string id(idText);
    env->ReleaseStringUTFChars(runId, idText);
    if (!admitted || !horde::platform::android::AndroidMotionRunIdValid(id)) return JNI_FALSE;
    std::lock_guard<std::mutex> lock(gMotionRequestMutex);
    if (gMotionStatus.load(std::memory_order_acquire) != 0) return JNI_FALSE;
    gMotionRequestedScenario = scenario; gMotionRequestedId = id;
    gMotionStatus.store(1, std::memory_order_release);
    return JNI_TRUE;
#else
    (void)env; (void)scenarioName; (void)runId;
    return JNI_FALSE;
#endif
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestDebugCheckpoint(JNIEnv*, jclass, jint checkpointId)
{
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
    DebugCheckpointSelection selection;
    if (!ResolveDebugCheckpoint(static_cast<std::int32_t>(checkpointId), selection))
    {
        return JNI_FALSE;
    }
    gBenchmarkCheckpointRequested.store(static_cast<std::int32_t>(checkpointId), std::memory_order_release);
    return JNI_TRUE;
#else
    (void)checkpointId;
    return JNI_FALSE;
#endif
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestDebugCaptureCheckpoint(JNIEnv*, jclass, jint checkpointId)
{
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
    DebugCheckpointSelection selection;
    if (gSurfaceSessions.State() != 1 ||
        gInAppBenchmarkStatus.load(std::memory_order_acquire) == 1 ||
        !ResolveDebugCheckpoint(static_cast<std::int32_t>(checkpointId), selection))
    {
        return JNI_FALSE;
    }
    gCaptureCheckpointRequested.store(static_cast<std::int32_t>(checkpointId), std::memory_order_release);
    return JNI_TRUE;
#else
    (void)checkpointId;
    return JNI_FALSE;
#endif
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestDebugRouteReplay(JNIEnv*, jclass)
{
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
    gRouteReplayRequested.store(true, std::memory_order_release);
    return JNI_TRUE;
#else
    return JNI_FALSE;
#endif
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestBenchmark(JNIEnv*, jclass)
{
    std::lock_guard<std::mutex> lock(gReportMutex);
    if (gSurfaceSessions.State() != 1 ||
        gInAppBenchmarkStatus.load(std::memory_order_acquire) == 1)
    {
        return JNI_FALSE;
    }
    InvalidateBenchmarkSummaryLocked();
    gRequestedBenchmarkRunId.clear();
    gRequestedBenchmarkWorkload = horde::gameplay::BenchmarkWorkload::ShowcaseRoute;
    gInAppBenchmarkCancelRequested.store(false, std::memory_order_release);
    gRtLabBenchmarkRoute.store(true, std::memory_order_release);
    gInAppBenchmarkStatus.store(1, std::memory_order_release);
    gInAppBenchmarkRequested.store(true, std::memory_order_release);
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestBenchmarkWithId(
    JNIEnv* env, jclass, jstring runId)
{
    if (runId == nullptr || env->GetStringUTFLength(runId) < 1 ||
        env->GetStringUTFLength(runId) > 64) return JNI_FALSE;
    const char* text = env->GetStringUTFChars(runId, nullptr);
    if (text == nullptr) return JNI_FALSE;
    const std::string id(text);
    env->ReleaseStringUTFChars(runId, text);
    if (!std::all_of(id.begin(), id.end(), [](const char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '-' || c == '_';
        })) return JNI_FALSE;
    std::lock_guard<std::mutex> lock(gReportMutex);
    if (gSurfaceSessions.State() != 1 ||
        gInAppBenchmarkStatus.load(std::memory_order_acquire) == 1) return JNI_FALSE;
    InvalidateBenchmarkSummaryLocked();
    gRequestedBenchmarkRunId = id;
    gRequestedBenchmarkWorkload = horde::gameplay::BenchmarkWorkload::ShowcaseRoute;
    gInAppBenchmarkCancelRequested.store(false, std::memory_order_release);
    gRtLabBenchmarkRoute.store(true, std::memory_order_release);
    gInAppBenchmarkStatus.store(1, std::memory_order_release);
    gInAppBenchmarkRequested.store(true, std::memory_order_release);
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestBenchmarkWithIdAndWorkload(
    JNIEnv* env, jclass, jstring runId, jstring workloadName)
{
    if (runId == nullptr || workloadName == nullptr ||
        env->GetStringUTFLength(runId) < 1 || env->GetStringUTFLength(runId) > 64 ||
        env->GetStringUTFLength(workloadName) < 1 || env->GetStringUTFLength(workloadName) > 64) {
        return JNI_FALSE;
    }
    const char* runText = env->GetStringUTFChars(runId, nullptr);
    const char* workloadText = env->GetStringUTFChars(workloadName, nullptr);
    if (runText == nullptr || workloadText == nullptr) {
        if (runText != nullptr) env->ReleaseStringUTFChars(runId, runText);
        if (workloadText != nullptr) env->ReleaseStringUTFChars(workloadName, workloadText);
        return JNI_FALSE;
    }
    const std::string id(runText);
    const std::string workloadNameUtf8(workloadText);
    env->ReleaseStringUTFChars(runId, runText);
    env->ReleaseStringUTFChars(workloadName, workloadText);
    if (!std::all_of(id.begin(), id.end(), [](const char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '-' || c == '_';
        })) return JNI_FALSE;
    horde::gameplay::BenchmarkWorkload workload;
    if (!horde::gameplay::ParseBenchmarkWorkload(workloadNameUtf8, workload)) {
        return JNI_FALSE;
    }
    std::lock_guard<std::mutex> lock(gReportMutex);
    if (gSurfaceSessions.State() != 1 ||
        gInAppBenchmarkStatus.load(std::memory_order_acquire) == 1) return JNI_FALSE;
    InvalidateBenchmarkSummaryLocked();
    gRequestedBenchmarkRunId = id;
    gRequestedBenchmarkWorkload = workload;
    gInAppBenchmarkCancelRequested.store(false, std::memory_order_release);
    gRtLabBenchmarkRoute.store(true, std::memory_order_release);
    gInAppBenchmarkStatus.store(1, std::memory_order_release);
    gInAppBenchmarkRequested.store(true, std::memory_order_release);
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestBenchmarkMotionValidation(
    JNIEnv* env, jclass, jstring runId, jstring scenarioName)
{
#if HORDE_RT_ANDROID_MOTION_VALIDATION
    if (runId == nullptr || scenarioName == nullptr || env->GetStringLength(runId) < 1 ||
        env->GetStringLength(runId) > 64 || env->GetStringLength(scenarioName) > 64 ||
        gSurfaceSessions.State() != 1 || gMotionStatus.load(std::memory_order_acquire) != 0 ||
        gInAppBenchmarkStatus.load(std::memory_order_acquire) == 1)
        return JNI_FALSE;
    const char* runText = env->GetStringUTFChars(runId, nullptr);
    const char* scenarioText = env->GetStringUTFChars(scenarioName, nullptr);
    if (runText == nullptr || scenarioText == nullptr)
    {
        if (runText != nullptr) env->ReleaseStringUTFChars(runId, runText);
        if (scenarioText != nullptr) env->ReleaseStringUTFChars(scenarioName, scenarioText);
        return JNI_FALSE;
    }
    const std::string id(runText);
    const std::string scenarioUtf8(scenarioText);
    env->ReleaseStringUTFChars(runId, runText);
    env->ReleaseStringUTFChars(scenarioName, scenarioText);
    if (!std::all_of(id.begin(), id.end(), [](const char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '-' || c == '_';
        })) return JNI_FALSE;
    horde::gameplay::validation::MotionScenario scenario{};
    if (!horde::gameplay::validation::ParseMotionScenario(scenarioUtf8, scenario)) return JNI_FALSE;
    {
        std::lock_guard<std::mutex> reportLock(gReportMutex);
        if (gSurfaceSessions.State() != 1 || gInAppBenchmarkStatus.load(std::memory_order_acquire) == 1)
            return JNI_FALSE;
        InvalidateBenchmarkSummaryLocked();
        gRequestedBenchmarkRunId = id;
        gInAppBenchmarkCancelRequested.store(false, std::memory_order_release);
        gRtLabBenchmarkRoute.store(true, std::memory_order_release);
        gInAppBenchmarkStatus.store(1, std::memory_order_release);
    }
    {
        std::lock_guard<std::mutex> motionLock(gMotionRequestMutex);
        if (gMotionStatus.load(std::memory_order_acquire) != 0)
        {
            gInAppBenchmarkStatus.store(3, std::memory_order_release);
            return JNI_FALSE;
        }
        gMotionRequestedId = id;
        gMotionRequestedScenario = scenario;
        gMotionStatus.store(1, std::memory_order_release);
    }
    gMotionValidationRequested.store(true, std::memory_order_release);
    gInAppBenchmarkRequested.store(true, std::memory_order_release);
    return JNI_TRUE;
#else
    (void)env; (void)runId; (void)scenarioName;
    return JNI_FALSE;
#endif
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_finishBenchmarkMotionValidation(JNIEnv*, jclass)
{
#if HORDE_RT_ANDROID_MOTION_VALIDATION
    const int status = gMotionStatus.load(std::memory_order_acquire);
    if (status == 3 || status == 4) gMotionReleaseRequested.store(true, std::memory_order_release);
#endif
}

namespace
{
bool ReadBenchmarkSummaryText(JNIEnv* env, jstring value, const std::size_t maximum, std::string& out)
{
    if (!value) return false;
    const auto length = env->GetStringLength(value);
    if (static_cast<std::size_t>(length) > maximum) return false;
    std::vector<jchar> units(static_cast<std::size_t>(length));
    if (length) env->GetStringRegion(value, 0, length, units.data());
    if (env->ExceptionCheck()) return false;
    std::u16string text;
    text.reserve(units.size());
    for (const jchar unit : units) text.push_back(static_cast<char16_t>(unit));
    return horde::reporting::EncodePlaytestReportUtf16(text, maximum, out) ==
        horde::reporting::PlaytestReportStatus::Ready;
}
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestBenchmarkWithSummaryId(
    JNIEnv* env, jclass, jstring summaryRunUuid, jstring rawModel)
{
    try
    {
        std::string uuid, model;
        if (!ReadBenchmarkSummaryText(env, summaryRunUuid, 36u, uuid) ||
            !horde::telemetry::IsBenchmarkSummaryUuid(uuid) ||
            !ReadBenchmarkSummaryText(env, rawModel, 128u, model)) return JNI_FALSE;
        std::lock_guard<std::mutex> lock(gReportMutex);
        if (gSurfaceSessions.State() != 1 || gInAppBenchmarkStatus.load(std::memory_order_acquire) == 1)
            return JNI_FALSE;
        InvalidateBenchmarkSummaryLocked();
        gRequestedBenchmarkSummaryRunId = std::move(uuid);
        gRequestedBenchmarkSummaryRawModel = std::move(model);
        gRequestedBenchmarkRunId.clear(); // Ordinary run keeps its accepted FPS observer contract.
        gRequestedBenchmarkWorkload = horde::gameplay::BenchmarkWorkload::ShowcaseRoute;
        gInAppBenchmarkCancelRequested.store(false, std::memory_order_release);
        gRtLabBenchmarkRoute.store(true, std::memory_order_release);
        gInAppBenchmarkStatus.store(1, std::memory_order_release);
        gInAppBenchmarkRequested.store(true, std::memory_order_release);
        return JNI_TRUE;
    }
    catch (...) { return JNI_FALSE; }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getReadyBenchmarkSummaryRunId(JNIEnv* env, jclass)
{
    try
    {
        std::lock_guard<std::mutex> lock(gReportMutex);
        return env->NewStringUTF(gLatestBenchmarkSummary.IsReady() ?
            gLatestBenchmarkSummary.Data().runUuid.c_str() : "");
    }
    catch (...) { return nullptr; }
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_prepareBenchmarkSummaryReport(
    JNIEnv* env, jclass, jstring expectedRunUuid, jstring reportUuid, jstring capturedAtUtc,
    jboolean consentToPrepare, jboolean includeBasicHardware, jint declaredCooling)
{
    using namespace horde::reporting;
    const auto reply = [env](const BenchmarkSummaryReportStatus status, const std::string_view json = {}) {
        std::string envelope(1u, static_cast<char>(status)); envelope.append(json);
        return NewUtf8ByteArray(env, envelope);
    };
    try
    {
        if (consentToPrepare != JNI_TRUE) return reply(BenchmarkSummaryReportStatus::ConsentRequired);
        if (declaredCooling < 0 || declaredCooling > 2) return reply(BenchmarkSummaryReportStatus::InvalidCooling);
        std::string expected, id, utc;
        if (!ReadBenchmarkSummaryText(env, expectedRunUuid, 36u, expected) ||
            !ReadBenchmarkSummaryText(env, reportUuid, 36u, id))
            return env->ExceptionCheck() ? nullptr : reply(BenchmarkSummaryReportStatus::InvalidIdentity);
        if (!ReadBenchmarkSummaryText(env, capturedAtUtc, 30u, utc))
            return env->ExceptionCheck() ? nullptr : reply(BenchmarkSummaryReportStatus::InvalidTimestamp);
        horde::telemetry::FrozenBenchmarkSummary frozen;
        std::uint64_t revision = 0u;
        {
            std::lock_guard<std::mutex> lock(gReportMutex);
            if (!gLatestBenchmarkSummary.IsReady() || gLatestBenchmarkSummary.Data().runUuid != expected)
                return reply(BenchmarkSummaryReportStatus::InvalidSummary);
            frozen = gLatestBenchmarkSummary;
            revision = gBenchmarkSummaryRevision;
        }
        const auto prepared = PrepareBenchmarkSummaryReport(frozen,
            {true, includeBasicHardware == JNI_TRUE, id, utc,
                static_cast<horde::telemetry::BenchmarkSummaryCooling>(declaredCooling)});
        if (!prepared.IsReady()) return reply(prepared.Status());
        // Never publish a copied old result after a newer accepted run invalidates it.
        std::lock_guard<std::mutex> lock(gReportMutex);
        if (revision != gBenchmarkSummaryRevision || !gLatestBenchmarkSummary.IsReady() ||
            gLatestBenchmarkSummary.Data().runUuid != expected)
            return reply(BenchmarkSummaryReportStatus::InvalidSummary);
        return reply(BenchmarkSummaryReportStatus::Ready, prepared.Json());
    }
    catch (...) { return nullptr; }
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_cancelBenchmark(JNIEnv*, jclass)
{
    gInAppBenchmarkCancelRequested.store(true, std::memory_order_release);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getBenchmarkStatus(JNIEnv*, jclass)
{
    return static_cast<jint>(gInAppBenchmarkStatus.load(std::memory_order_acquire));
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getBenchmarkProgress(JNIEnv* env, jclass)
{
    const std::string progress = LatestBenchmarkProgress();
    return env->NewStringUTF(progress.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getBenchmarkReport(JNIEnv* env, jclass)
{
    const std::string report = LatestBenchmarkReport();
    return env->NewStringUTF(report.c_str());
}

extern "C" JNIEXPORT jint JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getRuntimeState(JNIEnv*, jclass)
{
    return static_cast<jint>(gSurfaceSessions.State());
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getWaterfallStereoGains(JNIEnv*, jclass)
{
    return static_cast<jlong>(gWaterfallStereoGains.load(std::memory_order_acquire));
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_drainPlatformEvents(JNIEnv* env, jclass)
{
    std::vector<jlong> packed;
    {
        std::lock_guard<std::mutex> lock(gPlatformGameplayEventMutex);
        packed.reserve(gPlatformGameplayEvents.Size() * 2u);
        for (const PlatformGameplayEvent& event : gPlatformGameplayEvents.Values())
        {
            packed.push_back(static_cast<jlong>(event.metadata));
            packed.push_back(static_cast<jlong>(event.stereoGains));
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
            if ((event.metadata & 0xffu) == static_cast<std::uint64_t>(
                    horde::gameplay::simulation::GameplayEventType::PlayerSwing))
            {
                __android_log_print(
                    ANDROID_LOG_INFO, kTag,
                    "HORDE_PLAYER_SWING_TRANSPORT phase=drained sequence=%llu",
                    static_cast<unsigned long long>((event.metadata >> 32u) & 0xffffffffu));
            }
#endif
        }
        gPlatformGameplayEvents.Clear();
    }

    jlongArray result = env->NewLongArray(static_cast<jsize>(packed.size()));
    if (result != nullptr && !packed.empty())
    {
        env->SetLongArrayRegion(result, 0, static_cast<jsize>(packed.size()), packed.data());
    }
    return result;
}
