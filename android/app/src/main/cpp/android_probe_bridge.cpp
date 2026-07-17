#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <sys/stat.h>

#include <jni.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>

#include "ui/DiagnosticOverlay.h"
#include "gameplay/CorridorCollision.h"
#include "gameplay/ShowcaseBenchmark.h"
#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/ShowcaseCheckpoints.h"
#include "gameplay/ShowcaseReplay.h"
#include "gameplay/SpatialAudio.h"
#include "gameplay/SwordCombat.h"
#include "vulkan/RtCapabilityReport.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/raytracing/PresentableTinyRtScene.h"

#ifndef HORDE_RT_BUILD_ID
#define HORDE_RT_BUILD_ID "development"
#endif
#ifndef HORDE_RT_RAYGEN_SHA256
#define HORDE_RT_RAYGEN_SHA256 "unknown"
#endif

namespace
{

constexpr const char* kTag = "HordeRtProbeBridge";
constexpr const char* kReportDirectory = "reports";
constexpr const char* kTextReportFilename = "vulkan_capability_report.txt";
constexpr const char* kJsonReportFilename = "vulkan_capability_report.json";
constexpr const char* kShowcaseDebugStateFilename = "showcase_debug_state.json";
// One frame in flight keeps the dynamically refit held-torch TLAS safely synchronized with its host-written instance buffer.
constexpr uint32_t kMaxFramesInFlight = 1u;
struct SwapchainContext
{
    ANativeWindow* window = nullptr;
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
    float renderScale = 1.0f;
    float frameDeltaSeconds = 1.0f / 60.0f;
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
    VkSemaphore renderFinishedSemaphores[kMaxFramesInFlight] = {};
    VkFence inFlightFences[kMaxFramesInFlight] = {};
    VkClearColorValue clearColor = {{0.12f, 0.04f, 0.18f, 1.0f}};
    horde::vulkan::DeviceCapabilities capabilities;
    horde::vulkan::raytracing::PresentableTinyRtScene rtScene;
    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    float lanternStrength = 1.8f;
    float walkTime = 0.0f;
    float cameraX = 0.0f;
    float cameraZ = 1.85f;
    float moveStrafe = 0.0f;
    float moveForward = 0.0f;
    horde::gameplay::TravelFootstepCadence playerFootsteps;
    horde::gameplay::PlayerFootstepCadence enemyFootsteps;
    float outputExposure = 0.92f;
    horde::gameplay::SwordCombat combat;
    horde::gameplay::CombatSnapshot combatSnapshot;
    horde::gameplay::LanternSequence lanternSequence;
    horde::gameplay::LanternSnapshot lanternSnapshot;
    horde::gameplay::EnemyDirector enemyDirector;
    horde::gameplay::LichEncounter lichEncounter;
    horde::gameplay::EnemyKind activeEnemyKind = horde::gameplay::EnemyKind::Skeleton;
    float lichDamageFlash = 0.0f;
    std::int32_t activeBenchmarkCheckpoint = -1;
    std::uint32_t benchmarkGeneration = 0u;
    std::uint32_t benchmarkWarmupFrames = 0u;
    std::uint32_t benchmarkSampleFrames = 0u;
    std::uint32_t benchmarkWindow = 0u;
    double benchmarkFenceMs = 0.0;
    double benchmarkRecordMs = 0.0;
    double benchmarkPresentMs = 0.0;
    double benchmarkTotalMs = 0.0;
    bool benchmarkSampling = false;
    horde::gameplay::ShowcaseRouteReplay routeReplay;
    bool routeReplayActive = false;
    horde::gameplay::ShowcaseBenchmarkRun inAppBenchmark;
    std::string reportDirectory;
    bool useRtPath = false;
    uint32_t currentFrame = 0u;
};

SwapchainContext gSwapchainContext{};
std::atomic<bool> gSwapchainRunning{false};
std::thread gSwapchainThread;
std::mutex gSwapchainMutex;
std::mutex gReportMutex;
std::string gLatestTextReport;
std::string gLatestJsonReport;
std::string gLatestDeveloperOverlayText;
std::string gLatestBenchmarkReport;
std::string gLatestBenchmarkProgress;
std::atomic<bool> gAttackRequested{false};
std::atomic<bool> gResetRequested{false};
std::atomic<bool> gSimulationPaused{true};
std::atomic<int> gRuntimeState{0}; // 0 starting/stopped, 1 honestly presented RT, 2 unsupported, 3 render error.
std::atomic<uint32_t> gAudioEvents{0u};
std::atomic<uint64_t> gEnemyAudioStereoGains{0u};
std::atomic<float> gRequestedRenderScale{1.0f};
std::atomic<std::int32_t> gBenchmarkCheckpointRequested{-1};
std::atomic<bool> gRouteReplayRequested{false};
std::atomic<bool> gInAppBenchmarkRequested{false};
std::atomic<bool> gInAppBenchmarkCancelRequested{false};
std::atomic<int> gInAppBenchmarkStatus{0}; // 0 idle, 1 running, 2 complete, 3 failed/cancelled.

constexpr uint32_t kAudioEventEnemyDefeated = 1u << 0u;
constexpr uint32_t kAudioEventPlayerFootstep = 1u << 1u;
constexpr uint32_t kAudioEventEnemyFootstep = 1u << 2u;
constexpr uint32_t kAudioEventEnemyAttack = 1u << 3u;
constexpr uint32_t kAudioEventLichCharge = 1u << 4u;
constexpr uint32_t kAudioEventLichImpact = 1u << 5u;
constexpr uint32_t kAudioEventLichDefeated = 1u << 6u;
constexpr uint32_t kAudioEventLichHit = 1u << 7u;

uint64_t PackStereoGains(float left, float right)
{
    return static_cast<uint64_t>(std::bit_cast<uint32_t>(left)) |
           (static_cast<uint64_t>(std::bit_cast<uint32_t>(right)) << 32u);
}

void PublishEnemyAudioGains(const SwapchainContext& context)
{
    const float emitterX = context.activeEnemyKind == horde::gameplay::EnemyKind::Lich
        ? context.lichEncounter.Snapshot().x : context.combatSnapshot.enemyX;
    const float emitterZ = context.activeEnemyKind == horde::gameplay::EnemyKind::Lich
        ? context.lichEncounter.Snapshot().z : context.combatSnapshot.enemyZ;
    const horde::gameplay::SpatialAudioGains gains = horde::gameplay::CalculateSpatialAudio(
        {emitterX, emitterZ, 1.0f, 1.0f, 14.0f},
        {context.cameraX, context.cameraZ, context.cameraYaw});
    gEnemyAudioStereoGains.store(PackStereoGains(gains.left, gains.right), std::memory_order_release);
}

std::string BuildDisplayText(const horde::vulkan::DeviceCapabilities& capabilities)
{
    if (capabilities.rtMode == horde::vulkan::RtMode::Unsupported)
    {
        return horde::ui::BuildUnsupportedDeviceText(capabilities);
    }
    return horde::ui::BuildDiagnosticOverlayText(capabilities);
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
    const horde::gameplay::EnemyRosterSnapshot& roster = context.enemyDirector.Snapshot();
    const horde::gameplay::LichSnapshot& lich = context.lichEncounter.Snapshot();
    const horde::gameplay::EnemyEncounterSnapshot* encounter = SelectedEncounter(roster);
    horde::ui::DeveloperOverlaySnapshot snapshot;
    snapshot.buildIdentity = std::string(HORDE_RT_BUILD_ID) + " DEBUG";
    snapshot.shaderIdentity = std::string(HORDE_RT_RAYGEN_SHA256).substr(0u, 12u);
    snapshot.gpuName = context.capabilities.identity.gpuName;
    snapshot.vulkanApi = PackedVulkanVersion(context.capabilities.identity.vulkanApiVersion);
    snapshot.rtMode = horde::vulkan::ToString(context.capabilities.rtMode);
    snapshot.routeZone = horde::gameplay::ShowcaseZoneName(
        horde::gameplay::QueryShowcaseZone(context.cameraX, context.cameraZ));
    snapshot.materialEncoding = context.rtScene.MaterialEncoding();
    snapshot.lanternPhase = horde::gameplay::LanternPhaseName(context.lanternSnapshot.phase);
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
    snapshot.activeSkinnedEnemies = static_cast<std::uint32_t>(roster.renderedEnemyCount);
    snapshot.renderScale = context.renderScale;
    snapshot.fps = context.capabilities.performance.fps;
    snapshot.frameTimeMs = context.capabilities.performance.frameTimeMs;
    snapshot.presented = context.capabilities.rtScene.presented;
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
}

std::string LatestTextReport()
{
    std::lock_guard<std::mutex> lock(gReportMutex);
    return gLatestTextReport;
}

std::string LatestJsonReport()
{
    std::lock_guard<std::mutex> lock(gReportMutex);
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

void ResetShowcaseSimulation(SwapchainContext& context)
{
    context.cameraYaw = 0.0f;
    context.cameraPitch = 0.0f;
    context.lanternStrength = 1.8f;
    context.walkTime = 0.0f;
    context.cameraX = horde::gameplay::kPlayerSpawn.x;
    context.cameraZ = horde::gameplay::kPlayerSpawn.z;
    context.moveStrafe = 0.0f;
    context.moveForward = 0.0f;
    context.playerFootsteps.Reset();
    context.enemyFootsteps.Reset();
    context.combat = {};
    context.combatSnapshot = {};
    context.lanternSequence.Reset();
    context.lanternSnapshot = {};
    context.enemyDirector.Reset();
    context.lichEncounter.Reset();
    context.activeEnemyKind = horde::gameplay::EnemyKind::Skeleton;
    context.lichDamageFlash = 0.0f;
    gAttackRequested.store(false, std::memory_order_release);
    gAudioEvents.store(0u, std::memory_order_release);
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

horde::gameplay::ShowcaseBenchmarkMetadata BuildBenchmarkMetadata(const SwapchainContext& context)
{
    horde::gameplay::ShowcaseBenchmarkMetadata metadata;
    metadata.timestampUtc = UtcTimestamp();
    metadata.buildIdentity = HORDE_RT_BUILD_ID;
    metadata.shaderIdentity = std::string(HORDE_RT_RAYGEN_SHA256).substr(0u, 12u);
    metadata.gpuName = context.capabilities.identity.gpuName;
    metadata.vulkanApi = std::to_string(VK_API_VERSION_MAJOR(context.capabilities.identity.vulkanApiVersion)) + "." +
                         std::to_string(VK_API_VERSION_MINOR(context.capabilities.identity.vulkanApiVersion)) + "." +
                         std::to_string(VK_API_VERSION_PATCH(context.capabilities.identity.vulkanApiVersion));
    metadata.rtMode = horde::vulkan::ToString(context.capabilities.rtMode);
    metadata.presentMode = PresentModeName(context.swapchainPresentMode);
    metadata.materialEncoding = context.rtScene.MaterialEncoding();
    metadata.renderScalePercent = static_cast<std::uint32_t>(std::lround(context.renderScale * 100.0f));
    metadata.internalWidth = context.rtScene.DispatchExtent().width;
    metadata.internalHeight = context.rtScene.DispatchExtent().height;
    metadata.presentationWidth = context.swapchainExtent.width;
    metadata.presentationHeight = context.swapchainExtent.height;
    return metadata;
}

void PublishBenchmarkProgress(const SwapchainContext& context)
{
    std::lock_guard<std::mutex> lock(gReportMutex);
    gLatestBenchmarkProgress = context.inAppBenchmark.ProgressText();
}

void StartInAppBenchmark(SwapchainContext& context)
{
    gBenchmarkCheckpointRequested.store(-1, std::memory_order_release);
    gRouteReplayRequested.store(false, std::memory_order_release);
    ResetShowcaseSimulation(context);
    context.activeBenchmarkCheckpoint = -1;
    context.benchmarkSampling = false;
    context.routeReplayActive = false;
    context.inAppBenchmark.Start();
    {
        std::lock_guard<std::mutex> lock(gReportMutex);
        gLatestBenchmarkReport.clear();
        gLatestBenchmarkProgress = context.inAppBenchmark.ProgressText();
    }
    gInAppBenchmarkStatus.store(1, std::memory_order_release);
}

void FinishInAppBenchmark(SwapchainContext& context)
{
    const horde::gameplay::ShowcaseBenchmarkMetadata metadata = BuildBenchmarkMetadata(context);
    std::string text = context.inAppBenchmark.BuildTextReport(metadata);
    const std::string json = context.inAppBenchmark.BuildJsonReport(metadata);
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
        gLatestBenchmarkProgress = context.inAppBenchmark.Passed() ? "BENCHMARK COMPLETE" : "BENCHMARK INVALID";
    }
    gInAppBenchmarkStatus.store(context.inAppBenchmark.Passed() ? 2 : 3, std::memory_order_release);
    gSimulationPaused.store(true, std::memory_order_release);
    ResetShowcaseSimulation(context);
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

void WriteShowcaseDebugState(const SwapchainContext& context, const char* status)
{
    const horde::gameplay::ShowcaseCheckpoint* checkpoint =
        horde::gameplay::FindShowcaseCheckpoint(context.activeBenchmarkCheckpoint);
    const horde::gameplay::ShowcaseZone zone =
        horde::gameplay::QueryShowcaseZone(context.cameraX, context.cameraZ);
    const horde::gameplay::LichSnapshot& lich = context.lichEncounter.Snapshot();
    const horde::gameplay::EnemyRosterSnapshot& roster = context.enemyDirector.Snapshot();
    const horde::gameplay::ShowcaseReplaySnapshot& replay = context.routeReplay.Snapshot();
    std::ostringstream json;
    json.setf(std::ios::fixed);
    json.precision(4);
    json << "{\n"
         << "  \"schema\": 1,\n"
         << "  \"status\": \"" << status << "\",\n"
         << "  \"generation\": " << context.benchmarkGeneration << ",\n"
         << "  \"checkpoint\": \"" << (checkpoint ? checkpoint->name : (context.routeReplayActive ? "route-replay" : "none")) << "\",\n"
         << "  \"player\": {\"x\": " << context.cameraX << ", \"z\": " << context.cameraZ
         << ", \"yaw\": " << context.cameraYaw << ", \"pitch\": " << context.cameraPitch << "},\n"
         << "  \"zone\": \"" << horde::gameplay::ShowcaseZoneName(zone) << "\",\n"
         << "  \"renderScale\": " << context.renderScale << ",\n"
         << "  \"internalExtent\": {\"width\": " << context.capabilities.performance.internalRenderWidth
         << ", \"height\": " << context.capabilities.performance.internalRenderHeight << "},\n"
         << "  \"presented\": " << (context.capabilities.rtScene.presented ? "true" : "false") << ",\n"
         << "  \"lanternPhase\": \"" << horde::gameplay::LanternPhaseName(context.lanternSnapshot.phase) << "\",\n"
         << "  \"selectedEnemy\": \"" << horde::gameplay::EnemyKindName(roster.selectedEnemy) << "\",\n"
         << "  \"activeSkinnedEnemies\": " << roster.renderedEnemyCount << ",\n"
         << "  \"lich\": {\"phase\": \"" << horde::gameplay::LichPhaseName(lich.phase)
         << "\", \"health\": " << lich.health << ", \"roofProgress\": " << lich.finaleSkylightOpenProgress << "},\n"
         << "  \"benchmarkWindowsCompleted\": " << context.benchmarkWindow << ",\n"
         << "  \"replayWaypointsReached\": " << replay.reachedWaypoints << ",\n"
         << "  \"replayComplete\": " << (replay.complete ? "true" : "false") << ",\n"
         << "  \"replayFailed\": " << (replay.failed ? "true" : "false") << "\n"
         << "}\n";
    WriteTextFile(context.reportDirectory + '/' + kShowcaseDebugStateFilename, json.str());
}

void ApplyBenchmarkCheckpoint(SwapchainContext& context, const horde::gameplay::ShowcaseCheckpoint& checkpoint)
{
    ResetShowcaseSimulation(context);
    const horde::gameplay::ShowcaseCheckpointState state =
        horde::gameplay::BuildShowcaseCheckpointState(checkpoint);
    context.cameraX = checkpoint.x;
    context.cameraZ = checkpoint.z;
    context.cameraYaw = checkpoint.yaw;
    context.cameraPitch = checkpoint.pitch;
    context.lanternSequence = state.lantern;
    context.lanternSnapshot = state.lanternSnapshot;
    context.enemyDirector = state.enemyDirector;
    context.lichEncounter = state.lichEncounter;
    context.activeEnemyKind = state.activeEnemyKind;
    context.activeBenchmarkCheckpoint = checkpoint.id;
    context.routeReplayActive = false;
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

void ApplyRouteReplay(SwapchainContext& context)
{
    ResetShowcaseSimulation(context);
    context.activeBenchmarkCheckpoint = -1;
    context.benchmarkSampling = false;
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
    const horde::gameplay::ShowcaseCheckpoint* checkpoint =
        horde::gameplay::FindShowcaseCheckpoint(context.activeBenchmarkCheckpoint);
    const horde::gameplay::ShowcaseZone zone =
        horde::gameplay::QueryShowcaseZone(context.cameraX, context.cameraZ);
    __android_log_print(ANDROID_LOG_INFO,
                        kTag,
                        "HORDE_BENCH sample generation=%u checkpoint=%s scale=%.0f window=%u frames=120 total_ms=%.3f fence_ms=%.3f record_ms=%.3f present_ms=%.3f zone=%s presented=%d",
                        context.benchmarkGeneration,
                        checkpoint ? checkpoint->name : "unknown",
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
                            checkpoint ? checkpoint->name : "unknown",
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

bool CreateInstance(VkInstance& instance)
{
    const char* extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};
    const VkApplicationInfo appInfo{
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "HordeLanternRTDiagnostic",
        VK_MAKE_VERSION(1, 0, 0),
        "horde_rt",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_1};

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(std::size(extensions));
    createInfo.ppEnabledExtensionNames = extensions;

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
                __android_log_print(ANDROID_LOG_ERROR, kTag, "Selected RayTracingPipeline device is missing required extension: %s", extension);
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
    const float scale = std::clamp(renderScale, 0.50f, 1.0f);
    return {
        std::max(1u, static_cast<uint32_t>(std::lround(static_cast<double>(presentationExtent.width) * scale))),
        std::max(1u, static_cast<uint32_t>(std::lround(static_cast<double>(presentationExtent.height) * scale)))};
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
    context.swapchainExtent = ClampExtent(capabilities, std::max(1u, width), std::max(1u, height));
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
        capabilities.currentTransform,
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

    VkSemaphoreCreateInfo semaphoreCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
    for (uint32_t i = 0u; i < kMaxFramesInFlight; ++i)
    {
        if (vkCreateSemaphore(context.device, &semaphoreCreateInfo, nullptr, &context.imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(context.device, &semaphoreCreateInfo, nullptr, &context.renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(context.device, &fenceCreateInfo, nullptr, &context.inFlightFences[i]) != VK_SUCCESS)
        {
            return false;
        }
    }

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
        if (properties.vendorID == capabilities.identity.vendorId &&
            properties.deviceID == capabilities.identity.deviceId &&
            capabilities.identity.gpuName == properties.deviceName)
        {
            uint32_t queueFamilyIndex = 0u;
            if (FindGraphicsAndPresentQueueFamily(candidate, surface, queueFamilyIndex))
            {
                return candidate;
            }
        }
    }

    for (const VkPhysicalDevice candidate : physicalDevices)
    {
        uint32_t queueFamilyIndex = 0u;
        if (FindGraphicsAndPresentQueueFamily(candidate, surface, queueFamilyIndex))
        {
            return candidate;
        }
    }

    return VK_NULL_HANDLE;
}

void ReleaseSwapchainResources(SwapchainContext& context)
{
    if (context.device == VK_NULL_HANDLE)
    {
        return;
    }

    vkDeviceWaitIdle(context.device);
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

    context.swapchainImageLayouts.clear();
    context.swapchainImages.clear();
    context.currentFrame = 0u;
}

bool InitialiseRtSceneForSwapchain(SwapchainContext& context)
{
    if (!context.useRtPath)
    {
        return true;
    }

    const VkExtent2D renderExtent = ScaledRenderExtent(context.swapchainExtent, context.renderScale);
    std::string diagnostic;
    if (!context.rtScene.Initialise(context.instance,
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
                                    diagnostic))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to initialise presentable RT scene: %s", diagnostic.c_str());
        return false;
    }
    __android_log_print(ANDROID_LOG_INFO, kTag, "PBR material encoding: %s", context.rtScene.MaterialEncoding().c_str());
    __android_log_print(ANDROID_LOG_INFO,
                        kTag,
                        "RT render scale %.0f%%: %ux%u -> %ux%u",
                        static_cast<double>(context.renderScale * 100.0f),
                        renderExtent.width,
                        renderExtent.height,
                        context.swapchainExtent.width,
                        context.swapchainExtent.height);

    return true;
}

bool RecreateSwapchain(SwapchainContext& context)
{
    ReleaseSwapchainResources(context);
    return CreateSwapchain(context) && InitialiseRtSceneForSwapchain(context);
}

void DestroySwapchainContext(SwapchainContext& context)
{
    if (context.device == VK_NULL_HANDLE)
    {
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
        return;
    }

    vkDeviceWaitIdle(context.device);
    context.rtScene.Destroy();
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
}

bool RenderFrame(SwapchainContext& context, bool& rtFramePresented)
{
    const auto frameStart = std::chrono::steady_clock::now();
    rtFramePresented = false;
    if (context.commandBuffers.empty())
    {
        return false;
    }

    const VkResult waitResult = vkWaitForFences(context.device, 1u, &context.inFlightFences[context.currentFrame], VK_TRUE, UINT64_MAX);
    const auto fenceDone = std::chrono::steady_clock::now();
    if (waitResult != VK_SUCCESS && waitResult != VK_TIMEOUT)
    {
        return false;
    }

    uint32_t imageIndex = 0u;
    const VkResult acquireResult = vkAcquireNextImageKHR(
        context.device,
        context.swapchain,
        UINT64_MAX,
        context.imageAvailableSemaphores[context.currentFrame],
        VK_NULL_HANDLE,
        &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_SUBOPTIMAL_KHR)
    {
        return RecreateSwapchain(context);
    }
    if (acquireResult != VK_SUCCESS)
    {
        return false;
    }

    VkCommandBufferResetFlags resetFlags = 0;
    if (vkResetCommandBuffer(context.commandBuffers[imageIndex], resetFlags) != VK_SUCCESS)
    {
        return false;
    }

    const VkCommandBufferBeginInfo beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr};

    if (vkBeginCommandBuffer(context.commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
    {
        return false;
    }

    const bool useRtFrame = context.useRtPath && context.rtScene.IsReady();
    bool inAppBenchmarkFrame = false;
    const auto recordStart = std::chrono::steady_clock::now();
    if (useRtFrame)
    {
        if (gResetRequested.exchange(false))
        {
            if (context.inAppBenchmark.IsRunning())
            {
                context.inAppBenchmark.Cancel();
                gInAppBenchmarkStatus.store(3, std::memory_order_release);
            }
            ResetShowcaseSimulation(context);
            context.activeBenchmarkCheckpoint = -1;
            context.benchmarkSampling = false;
            context.routeReplayActive = false;
        }

        if (gInAppBenchmarkRequested.exchange(false, std::memory_order_acq_rel))
        {
            StartInAppBenchmark(context);
        }
        if (gInAppBenchmarkCancelRequested.exchange(false, std::memory_order_acq_rel) &&
            context.inAppBenchmark.IsRunning())
        {
            context.inAppBenchmark.Cancel();
            ResetShowcaseSimulation(context);
            {
                std::lock_guard<std::mutex> lock(gReportMutex);
                gLatestBenchmarkProgress = "BENCHMARK CANCELLED";
            }
            gInAppBenchmarkStatus.store(3, std::memory_order_release);
        }

        const std::int32_t requestedCheckpoint =
            gBenchmarkCheckpointRequested.exchange(-1, std::memory_order_acq_rel);
        if (!context.inAppBenchmark.IsRunning())
        {
            if (const horde::gameplay::ShowcaseCheckpoint* checkpoint =
                    horde::gameplay::FindShowcaseCheckpoint(requestedCheckpoint))
            {
                ApplyBenchmarkCheckpoint(context, *checkpoint);
            }
            if (gRouteReplayRequested.exchange(false, std::memory_order_acq_rel))
            {
                ApplyRouteReplay(context);
            }
        }
        else
        {
            gRouteReplayRequested.store(false, std::memory_order_release);
        }

        const bool simulationPaused = gSimulationPaused.load(std::memory_order_acquire);
        if (!simulationPaused)
        {
            context.walkTime += context.frameDeltaSeconds;
        }
        const bool playerAttackRequested = gAttackRequested.exchange(false) && !simulationPaused;
        float moveAmount = simulationPaused
            ? 0.0f
            : std::clamp(std::abs(context.moveForward) + std::abs(context.moveStrafe), 0.0f, 1.0f);
        float travelledThisFrame = 0.0f;
        inAppBenchmarkFrame = context.inAppBenchmark.IsRunning() && !simulationPaused;
        if (inAppBenchmarkFrame)
        {
            context.frameDeltaSeconds = 1.0f / 60.0f;
            const float previousCameraX = context.cameraX;
            const float previousCameraZ = context.cameraZ;
            const horde::gameplay::ShowcaseBenchmarkAdvance advance = context.inAppBenchmark.Advance();
            if (advance.lapStarted)
            {
                ResetShowcaseSimulation(context);
            }
            context.cameraX = advance.replay.x;
            context.cameraZ = advance.replay.z;
            context.cameraYaw = advance.replay.yaw;
            context.cameraPitch = -0.04f;
            moveAmount = advance.finished ? 0.0f : 1.0f;
            travelledThisFrame = std::hypot(context.cameraX - previousCameraX,
                                             context.cameraZ - previousCameraZ);
            if (advance.replay.waypointReached || advance.lapStarted || advance.finished)
            {
                PublishBenchmarkProgress(context);
            }
        }
        else if (context.routeReplayActive && !simulationPaused)
        {
            const float previousCameraX = context.cameraX;
            const float previousCameraZ = context.cameraZ;
            const horde::gameplay::ShowcaseReplaySnapshot& replay = context.routeReplay.Update();
            context.cameraX = replay.x;
            context.cameraZ = replay.z;
            context.cameraYaw = replay.yaw;
            context.cameraPitch = -0.04f;
            moveAmount = replay.complete || replay.failed ? 0.0f : 1.0f;
            travelledThisFrame = std::hypot(context.cameraX - previousCameraX,
                                             context.cameraZ - previousCameraZ);
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
        else if (moveAmount > 0.02f)
        {
            const float forwardX = std::sin(context.cameraYaw);
            const float forwardZ = -std::cos(context.cameraYaw);
            const float rightX = std::cos(context.cameraYaw);
            const float rightZ = std::sin(context.cameraYaw);
            const float speed = 0.032f;
            const float previousCameraX = context.cameraX;
            const float previousCameraZ = context.cameraZ;
            context.cameraX += (forwardX * context.moveForward + rightX * context.moveStrafe) * speed;
            context.cameraZ += (forwardZ * context.moveForward + rightZ * context.moveStrafe) * speed;
            horde::gameplay::ResolveCorridorPlayerCollision(previousCameraX, previousCameraZ, context.cameraX, context.cameraZ);
            travelledThisFrame = std::hypot(context.cameraX - previousCameraX,
                                             context.cameraZ - previousCameraZ);
        }
        if (context.playerFootsteps.Update(travelledThisFrame, moveAmount > 0.02f))
        {
            gAudioEvents.fetch_or(kAudioEventPlayerFootstep, std::memory_order_release);
        }
        context.enemyDirector.Update(context.cameraX, context.cameraZ);
        const horde::gameplay::EnemyRosterSnapshot& roster = context.enemyDirector.Snapshot();
        if (roster.selectedEnemy != context.activeEnemyKind)
        {
            context.activeEnemyKind = roster.selectedEnemy;
            if (context.activeEnemyKind == horde::gameplay::EnemyKind::Skeleton)
            {
                context.combat = {};
                context.combatSnapshot = {};
            }
            else if (context.activeEnemyKind == horde::gameplay::EnemyKind::Lich)
            {
                context.lichEncounter.Reset();
            }
        }
        bool lichHitRequested = false;
        if (playerAttackRequested)
        {
            // Keep the player sword animation independent of the selected enemy.
            context.combat.RequestAttack();
            lichHitRequested = context.activeEnemyKind == horde::gameplay::EnemyKind::Lich;
        }
        if (!simulationPaused)
        {
            context.lanternSnapshot = context.lanternSequence.Update(
                context.frameDeltaSeconds,
                context.cameraX,
                context.cameraZ,
                context.cameraYaw,
                context.cameraPitch);
            const horde::gameplay::EnemyAnimation previousAnimation = context.combatSnapshot.enemyAnimation;
            context.combatSnapshot = context.combat.Update(context.frameDeltaSeconds, context.cameraX, context.cameraZ, context.cameraYaw);
            if (context.activeEnemyKind == horde::gameplay::EnemyKind::Skeleton &&
                previousAnimation != horde::gameplay::EnemyAnimation::Dead &&
                context.combatSnapshot.enemyAnimation == horde::gameplay::EnemyAnimation::Dead)
            {
                gAudioEvents.fetch_or(kAudioEventEnemyDefeated, std::memory_order_release);
            }
            if (context.activeEnemyKind == horde::gameplay::EnemyKind::Skeleton &&
                previousAnimation != horde::gameplay::EnemyAnimation::Attack &&
                context.combatSnapshot.enemyAnimation == horde::gameplay::EnemyAnimation::Attack)
            {
                gAudioEvents.fetch_or(kAudioEventEnemyAttack, std::memory_order_release);
            }
            const bool skeletonWalking = context.activeEnemyKind == horde::gameplay::EnemyKind::Skeleton &&
                                         context.combatSnapshot.enemyAnimation == horde::gameplay::EnemyAnimation::Walking;
            if (context.enemyFootsteps.Update(context.frameDeltaSeconds, skeletonWalking))
            {
                gAudioEvents.fetch_or(kAudioEventEnemyFootstep, std::memory_order_release);
            }
        }
        const bool finaleActive = horde::gameplay::QueryShowcaseZone(context.cameraX, context.cameraZ) ==
                                  horde::gameplay::ShowcaseZone::Finale;
        const horde::gameplay::LichPhase previousLichPhase = context.lichEncounter.Snapshot().phase;
        const horde::gameplay::LichSnapshot& lich = context.lichEncounter.Update(
            !simulationPaused && context.activeEnemyKind == horde::gameplay::EnemyKind::Lich ? context.frameDeltaSeconds : 0.0f,
            context.cameraX,
            context.cameraZ,
            !horde::gameplay::IsRouteAudioObstructed(context.cameraX, context.cameraZ,
                                                      context.lichEncounter.Snapshot().x,
                                                      context.lichEncounter.Snapshot().z),
            context.activeEnemyKind == horde::gameplay::EnemyKind::Lich && finaleActive);
        if (lichHitRequested && context.lichEncounter.TryAcceptPlayerHit(context.cameraX, context.cameraZ))
        {
            gAudioEvents.fetch_or(kAudioEventLichHit, std::memory_order_release);
            if (lich.phase == horde::gameplay::LichPhase::Dead)
            {
                gAudioEvents.fetch_or(kAudioEventLichDefeated, std::memory_order_release);
            }
        }
        if (context.activeEnemyKind == horde::gameplay::EnemyKind::Lich &&
            previousLichPhase != horde::gameplay::LichPhase::Charging &&
            lich.phase == horde::gameplay::LichPhase::Charging)
        {
            gAudioEvents.fetch_or(kAudioEventLichCharge, std::memory_order_release);
        }
        if (context.activeEnemyKind == horde::gameplay::EnemyKind::Lich &&
            lich.deathAnimationComplete)
        {
            context.enemyDirector.MarkSelectedDead();
        }
        context.lichDamageFlash = std::max(0.0f, context.lichDamageFlash - context.frameDeltaSeconds * 2.8f);
        if (lich.damagePulse)
        {
            context.lichDamageFlash = 1.0f;
            gAudioEvents.fetch_or(kAudioEventLichImpact, std::memory_order_release);
        }
        horde::gameplay::CombatSnapshot renderCombat = context.combatSnapshot;
        renderCombat.damageFlash = std::max(renderCombat.damageFlash, context.lichDamageFlash);
        PublishEnemyAudioGains(context);
        std::string diagnostic;
        if (!context.rtScene.RecordTraceAndCopy(context.commandBuffers[imageIndex],
                                                context.swapchainImages[imageIndex],
                                                context.swapchainImageLayouts[imageIndex],
                                                context.swapchainExtent,
                                                context.cameraYaw,
                                                context.cameraPitch,
                                                context.lanternStrength * context.lanternSnapshot.flameStrength,
                                                context.walkTime,
                                                context.cameraX,
                                                context.cameraZ,
                                                moveAmount,
                                                context.outputExposure,
                                                renderCombat,
                                                context.lanternSnapshot,
                                                roster,
                                                lich,
                                                diagnostic))
        {
            __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to record RT frame: %s", diagnostic.c_str());
            return false;
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
        return false;
    }
    const auto recordDone = std::chrono::steady_clock::now();

    if (vkResetFences(context.device, 1u, &context.inFlightFences[context.currentFrame]) != VK_SUCCESS)
    {
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
        &context.renderFinishedSemaphores[context.currentFrame]};

    if (vkQueueSubmit(context.graphicsQueue, 1u, &submitInfo, context.inFlightFences[context.currentFrame]) != VK_SUCCESS)
    {
        return false;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1u;
    presentInfo.pWaitSemaphores = &context.renderFinishedSemaphores[context.currentFrame];
    presentInfo.swapchainCount = 1u;
    presentInfo.pSwapchains = &context.swapchain;
    presentInfo.pImageIndices = &imageIndex;
    const VkResult presentResult = vkQueuePresentKHR(context.graphicsQueue, &presentInfo);
    const auto presentDone = std::chrono::steady_clock::now();
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
    {
        if (inAppBenchmarkFrame)
        {
            const double interruptedFrameMs =
                std::chrono::duration<double, std::milli>(presentDone - frameStart).count();
            context.inAppBenchmark.RecordFrame(interruptedFrameMs, false);
            PublishBenchmarkProgress(context);
        }
        return RecreateSwapchain(context);
    }
    if (presentResult != VK_SUCCESS)
    {
        return false;
    }

    rtFramePresented = useRtFrame;
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
        PublishReportSnapshot(context.capabilities);
        WriteTextFile(context.reportDirectory + '/' + kTextReportFilename, BuildDisplayText(context.capabilities));
        WriteTextFile(context.reportDirectory + '/' + kJsonReportFilename,
                      horde::vulkan::BuildCapabilityJsonReport(context.capabilities));
        __android_log_print(ANDROID_LOG_INFO,
                            kTag,
                            "RT frame timing avg ms: total=%.3f fence=%.3f record=%.3f submit+present=%.3f",
                            context.timingTotalMs / count,
                            context.timingFenceMs / count,
                            context.timingRecordMs / count,
                            context.timingPresentMs / count);
        context.timingFrameCount = 0u;
        context.timingFenceMs = context.timingRecordMs = context.timingPresentMs = context.timingTotalMs = 0.0;
    }
    RecordBenchmarkFrame(context, frameFenceMs, frameRecordMs, framePresentMs, frameTotalMs);
    if (inAppBenchmarkFrame)
    {
        context.inAppBenchmark.RecordFrame(frameTotalMs, rtFramePresented);
        if (!context.inAppBenchmark.IsRunning())
        {
            FinishInAppBenchmark(context);
        }
    }
    return true;
}

void SwapchainRenderLoop()
{
    auto previousFrameStart = std::chrono::steady_clock::now();
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
    std::chrono::steady_clock::time_point lastDeveloperOverlayPublish{};
#endif
    while (gSwapchainRunning.load(std::memory_order_acquire))
    {
        const float requestedRenderScale = std::clamp(gRequestedRenderScale.load(std::memory_order_acquire), 0.50f, 1.0f);
        if (gSwapchainContext.useRtPath && std::abs(requestedRenderScale - gSwapchainContext.renderScale) > 0.001f)
        {
            vkDeviceWaitIdle(gSwapchainContext.device);
            gSwapchainContext.rtScene.Destroy();
            gSwapchainContext.renderScale = requestedRenderScale;
            gSwapchainContext.capabilities.rtScene.presented = false;
            gSwapchainContext.capabilities.rtScene.dispatchWidth = 0u;
            gSwapchainContext.capabilities.rtScene.dispatchHeight = 0u;
            gSwapchainContext.capabilities.performance.internalRenderWidth = 0u;
            gSwapchainContext.capabilities.performance.internalRenderHeight = 0u;
            gSwapchainContext.capabilities.performance.frameTimeMs = 0.0f;
            gSwapchainContext.capabilities.performance.fps = 0.0f;
            gSwapchainContext.timingFrameCount = 0u;
            gSwapchainContext.timingFenceMs = gSwapchainContext.timingRecordMs = 0.0;
            gSwapchainContext.timingPresentMs = gSwapchainContext.timingTotalMs = 0.0;
            gRuntimeState.store(0, std::memory_order_release);
            if (!InitialiseRtSceneForSwapchain(gSwapchainContext))
            {
                if (gSwapchainContext.inAppBenchmark.IsRunning())
                {
                    gSwapchainContext.inAppBenchmark.Cancel();
                    gInAppBenchmarkStatus.store(3, std::memory_order_release);
                }
                gRuntimeState.store(3, std::memory_order_release);
                __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to apply requested RT render scale.");
                break;
            }
        }
        const auto frameStart = std::chrono::steady_clock::now();
        gSwapchainContext.frameDeltaSeconds = std::clamp(std::chrono::duration<float>(frameStart - previousFrameStart).count(), 1.0f / 240.0f, 0.1f);
        previousFrameStart = frameStart;
        bool rtFramePresented = false;
        if (!RenderFrame(gSwapchainContext, rtFramePresented))
        {
            if (gSwapchainContext.inAppBenchmark.IsRunning())
            {
                gSwapchainContext.inAppBenchmark.Cancel();
                gInAppBenchmarkStatus.store(3, std::memory_order_release);
            }
            gRuntimeState.store(3, std::memory_order_release);
            __android_log_print(ANDROID_LOG_ERROR, kTag, "Diagnostic surface render loop ended unexpectedly.");
            break;
        }
        if (rtFramePresented && !gSwapchainContext.capabilities.rtScene.presented)
        {
            gSwapchainContext.capabilities.rtScene.presented = true;
            gSwapchainContext.capabilities.rtScene.status = "Presented via swapchain";
            gSwapchainContext.capabilities.rtScene.geometry = "Complete Horde showcase route with sequential animated skeleton and staff-lit lich";
            gSwapchainContext.capabilities.rtScene.dispatchWidth = gSwapchainContext.rtScene.DispatchExtent().width;
            gSwapchainContext.capabilities.rtScene.dispatchHeight = gSwapchainContext.rtScene.DispatchExtent().height;
            gSwapchainContext.capabilities.performance.internalRenderWidth = gSwapchainContext.capabilities.rtScene.dispatchWidth;
            gSwapchainContext.capabilities.performance.internalRenderHeight = gSwapchainContext.capabilities.rtScene.dispatchHeight;
            auto& presentationDiagnostics = gSwapchainContext.capabilities.diagnostics;
            presentationDiagnostics.erase(std::remove(presentationDiagnostics.begin(), presentationDiagnostics.end(),
                                                       "Internal render resolution: not measured yet."),
                                          presentationDiagnostics.end());
            gRuntimeState.store(1, std::memory_order_release);

            PublishReportSnapshot(gSwapchainContext.capabilities);
            WriteTextFile(gSwapchainContext.reportDirectory + '/' + kTextReportFilename, BuildDisplayText(gSwapchainContext.capabilities));
            WriteTextFile(gSwapchainContext.reportDirectory + '/' + kJsonReportFilename, horde::vulkan::BuildCapabilityJsonReport(gSwapchainContext.capabilities));
            __android_log_print(ANDROID_LOG_INFO, kTag, "RT frame reached Android swapchain presentation.");
        }
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
        if (!gSwapchainContext.inAppBenchmark.IsRunning() &&
            frameStart - lastDeveloperOverlayPublish >= std::chrono::milliseconds(250))
        {
            PublishDeveloperOverlaySnapshot(gSwapchainContext);
            lastDeveloperOverlayPublish = frameStart;
        }
#endif
    }

    gSwapchainRunning.store(false, std::memory_order_release);

    SwapchainContext cleanup = std::move(gSwapchainContext);
    gSwapchainContext = {};
    DestroySwapchainContext(cleanup);
}

bool StartSurfaceInternal(ANativeWindow* window,
                          horde::vulkan::DeviceCapabilities capabilities,
                          const std::string& reportDirectory)
{
    if (window == nullptr)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "startDiagnosticSurface called with null window.");
        return false;
    }

    if (gSwapchainRunning.load(std::memory_order_acquire))
    {
        __android_log_print(ANDROID_LOG_WARN, kTag, "Diagnostic surface already running.");
        ANativeWindow_release(window);
        return false;
    }

    SwapchainContext context;
    context.window = window;
    context.capabilities = capabilities;
    context.reportDirectory = reportDirectory;
    context.renderScale = std::clamp(gRequestedRenderScale.load(std::memory_order_acquire), 0.50f, 1.0f);
    context.useRtPath = capabilities.rtMode == horde::vulkan::RtMode::RayTracingPipeline;
    context.clearColor = ClearColorForMode(capabilities.rtMode);
    gRuntimeState.store(context.useRtPath ? 0 : 2, std::memory_order_release);
    gAudioEvents.store(0u, std::memory_order_release);
    gEnemyAudioStereoGains.store(0u, std::memory_order_release);

    if (!CreateInstance(context.instance))
    {
        DestroySwapchainContext(context);
        return false;
    }

    if (!CreateSurface(context.instance, context.window, context.surface))
    {
        DestroySwapchainContext(context);
        return false;
    }

    context.physicalDevice = FindMatchingPhysicalDevice(context.instance, capabilities, context.surface);
    if (context.physicalDevice == VK_NULL_HANDLE)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "No matching physical device found for Android diagnostic surface.");
        DestroySwapchainContext(context);
        return false;
    }

    if (!FindGraphicsAndPresentQueueFamily(context.physicalDevice, context.surface, context.graphicsQueueFamilyIndex))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Could not find graphics+present queue family on Android.");
        DestroySwapchainContext(context);
        return false;
    }

    if (!CreateLogicalDevice(context.physicalDevice, context.graphicsQueueFamilyIndex, capabilities, context.device, context.graphicsQueue))
    {
        DestroySwapchainContext(context);
        return false;
    }

    if (!CreateSwapchain(context))
    {
        DestroySwapchainContext(context);
        return false;
    }
    if (!InitialiseRtSceneForSwapchain(context))
    {
        DestroySwapchainContext(context);
        return false;
    }

    gSwapchainContext = std::move(context);
    gSwapchainRunning.store(true, std::memory_order_release);
    gSwapchainThread = std::thread(SwapchainRenderLoop);

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
    const bool wasRunning = gSwapchainRunning.exchange(false, std::memory_order_acq_rel);
    if (!wasRunning)
    {
        if (gSwapchainThread.joinable())
        {
            gSwapchainThread.join();
        }
        return;
    }

    if (gSwapchainThread.joinable())
    {
        gSwapchainThread.join();
    }
}

} // namespace

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

extern "C" JNIEXPORT jboolean JNICALL
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
    if (!EnsureDirectoryExists(reportDirectory))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to create report directory: %s", reportDirectory.c_str());
        return JNI_FALSE;
    }

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to resolve ANativeWindow.");
        return JNI_FALSE;
    }

    std::lock_guard<std::mutex> lock(gSwapchainMutex);
    const horde::vulkan::DeviceCapabilities capabilities = RunProbe();

    // Write updated report from shared probe source.
    const std::string textReport = BuildDisplayText(capabilities);
    const std::string jsonReport = horde::vulkan::BuildCapabilityJsonReport(capabilities);
    PublishReportSnapshot(capabilities);
    if (!WriteTextFile(reportDirectory + '/' + kTextReportFilename, textReport) ||
        !WriteTextFile(reportDirectory + '/' + kJsonReportFilename, jsonReport))
    {
        ANativeWindow_release(window);
        return JNI_FALSE;
    }

    if (!StartSurfaceInternal(window, capabilities, reportDirectory))
    {
        return JNI_FALSE;
    }

    __android_log_print(ANDROID_LOG_INFO, kTag, "Started Android diagnostic surface rendering loop.");
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_stopDiagnosticSurface(JNIEnv*, jclass)
{
    std::lock_guard<std::mutex> lock(gSwapchainMutex);
    StopSurfaceInternal();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setViewControls(JNIEnv*, jclass, jfloat yaw, jfloat pitch, jfloat lanternStrength, jfloat moveStrafe, jfloat moveForward)
{
    std::lock_guard<std::mutex> lock(gSwapchainMutex);
    gSwapchainContext.cameraYaw = static_cast<float>(yaw);
    gSwapchainContext.cameraPitch = std::clamp(static_cast<float>(pitch), -0.32f, 0.28f);
    gSwapchainContext.lanternStrength = std::clamp(static_cast<float>(lanternStrength), 0.65f, 2.4f);
    gSwapchainContext.moveStrafe = std::clamp(static_cast<float>(moveStrafe), -1.0f, 1.0f);
    gSwapchainContext.moveForward = std::clamp(static_cast<float>(moveForward), -1.0f, 1.0f);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestAttack(JNIEnv*, jclass)
{
    gAttackRequested.store(true);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestRouteReset(JNIEnv*, jclass)
{
    gResetRequested.store(true, std::memory_order_release);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setSimulationPaused(JNIEnv*, jclass, jboolean paused)
{
    gSimulationPaused.store(paused == JNI_TRUE, std::memory_order_release);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setRenderScale(JNIEnv*, jclass, jfloat scale)
{
    gRequestedRenderScale.store(std::clamp(static_cast<float>(scale), 0.50f, 1.0f), std::memory_order_release);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_requestDebugCheckpoint(JNIEnv*, jclass, jint checkpointId)
{
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
    if (horde::gameplay::FindShowcaseCheckpoint(static_cast<std::int32_t>(checkpointId)) == nullptr)
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
    if (gRuntimeState.load(std::memory_order_acquire) != 1 ||
        gInAppBenchmarkStatus.load(std::memory_order_acquire) == 1)
    {
        return JNI_FALSE;
    }
    gInAppBenchmarkCancelRequested.store(false, std::memory_order_release);
    gInAppBenchmarkStatus.store(1, std::memory_order_release);
    gInAppBenchmarkRequested.store(true, std::memory_order_release);
    return JNI_TRUE;
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
    return static_cast<jint>(gRuntimeState.load(std::memory_order_acquire));
}

extern "C" JNIEXPORT jint JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_consumeAudioEvents(JNIEnv*, jclass)
{
    return static_cast<jint>(gAudioEvents.exchange(0u, std::memory_order_acq_rel));
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getEnemyAudioStereoGains(JNIEnv*, jclass)
{
    return static_cast<jlong>(gEnemyAudioStereoGains.load(std::memory_order_acquire));
}
