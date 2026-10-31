#include "platform/windows/DiagnosticWindow.h"
#include "platform/windows/WindowsAcceptanceProfile.h"
#ifndef HORDE_RT_WINDOWS_ACCEPTANCE
#define HORDE_RT_WINDOWS_ACCEPTANCE 0
#endif
#ifndef HORDE_RT_CHAPTER_MEASUREMENT
#define HORDE_RT_CHAPTER_MEASUREMENT 0
#endif
#include "platform/windows/WindowsSkyAttributionLaunch.h"
#include "platform/windows/WindowsHiddenCaptureLaunch.h"
#include "platform/windows/WindowsEnvironmentSurveyLaunch.h"
#if defined(_DEBUG)
#include "gameplay/validation/PlayerDodgePoseFixture.h"
#include "gameplay/effects/KeeperCastPresentation.h"
#include "../../../tests/KitTurnContactData.h"
#endif
#include "vulkan/raytracing/EnvironmentSurveyFrame.h"
#include "platform/windows/WindowsMusicPlayback.h"
#include "platform/windows/WindowsMusicFocus.h"
#include "platform/windows/WindowsCampaignMenu.h"
#include "platform/windows/WindowsLookoutObjectivePanel.h"
#include "gameplay/campaign/LookoutObjective.h"
#include "gameplay/campaign/CampaignPlaySession.h"
#include "gameplay/campaign/CampaignLoadBarrier.h"
#include "platform/windows/GraphicsMenuNavigation.h"
#include "platform/windows/WindowsPlaytestReport.h"
#include "platform/windows/WindowsBenchmarkReviewPayload.h"
#include "platform/windows/WindowsStatisticsSubmission.h"
#include "platform/windows/WindowsCompatibilityReport.h"
#include "reporting/BuildIdentity.h"
#include "platform/windows/WindowsRemotePlaytestReport.h"
#include "audio/SfxVolume.h"
#include "graphics/GraphicsSettings.h"
#include "graphics/RtPresentationTransform.h"
#include "graphics/ForegroundPauseRenderCadence.h"
#include "platform/windows/WindowsGraphicsPersistence.h"
#include "graphics/GraphicsPreviewSession.h"
#include "graphics/EntryMenuScene.h"
#include "graphics/GraphicsPreviewPerformance.h"
#if !defined(NDEBUG)
#include "telemetry/CombatTimingTrace.h"
#endif

#include <algorithm>
#include <array>
#include <atomic>
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
#include <locale>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
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
#include <shlobj.h>
#include <commctrl.h>
#include <dwmapi.h>
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
#include "gameplay/EquipmentFeedback.h"
#include "gameplay/ShowcaseBenchmark.h"
#include "telemetry/RtBenchmarkEvidenceRun.h"
#include "telemetry/BenchmarkSummary.h"
#include "reporting/BenchmarkSummaryReport.h"
#include "telemetry/RtEvidencePublication.h"
#include "gameplay/ShowcaseCheckpoints.h"
#include "gameplay/LanternBenchmarkScenario.h"
#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/SpatialAudio.h"
#include "gameplay/dialogue/ChapterDialogue.h"
#include "gameplay/dialogue/SubtitleLayout.h"
#include "platform/windows/WindowsSubtitleBitmap.h"
#include "gameplay/SwordCombat.h"
#include "gameplay/simulation/GameSimulation.h"
#include "platform/windows/DesktopControllerInput.h"
#include "platform/windows/WindowsCaptureContracts.h"
#include "platform/windows/WindowsGraphicsPreviewCapture.h"
#include "platform/windows/WindowsPreviewMeshPairCapture.h"
#include "platform/windows/WindowsForestRouteProofLaunch.h"
#include "platform/windows/WindowsForestContentCandidate.h"
#if defined(_DEBUG)
#include "scene/ForestDressing.h"
#include "scene/CorridorStaticDetail.h"
#include "scene/CorridorRegionalWaterClosure.h"
#endif
#include "platform/windows/WindowsOutputResizeLaunch.h"
#include "platform/windows/WindowsMotionEvidenceLaunch.h"
#include "platform/windows/WindowsMotionEvidenceCapture.h"
#include "platform/windows/WindowsCombatPracticeLaunch.h"
#include "platform/windows/WindowsCombatTeachingPrompt.h"
#include "platform/windows/WindowsPrivateLichRevivalAudio.h"
#if defined(_DEBUG) || HORDE_RT_CHAPTER_MEASUREMENT
#include "gameplay/validation/MotionEvidenceScenario.h"
#include "gameplay/validation/ChapterRoundTripProbe.h"
#include "gameplay/validation/RecordedReturnCheckpoint.h"
#include "gameplay/validation/ChapterRouteMeasurementClock.h"
#include "platform/windows/WindowsPresentInterval.h"
#include "telemetry/MotionEvidenceLedger.h"
#endif
#include "platform/windows/WindowsBenchmarkLaunch.h"
#include "platform/windows/WindowsBenchmarkProfile.h"
#include "platform/windows/WindowsBenchmarkMenuOwner.h"
#include "platform/windows/WindowsChapterShippingLaunch.h"
#include "gameplay/validation/ChapterRouteHostPhases.h"
#include "platform/windows/WindowsAssetRoot.h"
#include "platform/windows/WindowsMistCaptureLaunch.h"
#include "platform/windows/WindowsInteractionPrompt.h"
#include "platform/windows/WindowsControlPresentation.h"
#include "platform/windows/WindowsGameplayInput.h"
#include "platform/windows/WindowsChapterDialogue.h"
#include "platform/windows/WindowsVitalityHudLayout.h"
#include "platform/windows/WindowsGitHubReleaseUpdate.h"
#include "platform/windows/WindowsRtLabState.h"
#include "vulkan/GpuFrameTimer.h"
#include "vulkan/RtCapabilityReport.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/PresentCompletion.h"
#include "vulkan/RetirementOwner.h"
#include "vulkan/raytracing/PresentableTinyRtScene.h"
#include "vulkan/raytracing/DevelopmentStaticAssetPolicy.h"
#include "vulkan/raytracing/RtFrameEvidenceCoordinator.h"
#include "vulkan/raytracing/RtDeviceEnablePlan.h"
#include "vulkan/raytracing/SimulationFrameAdapter.h"
#include "vulkan/raytracing/DevelopmentWorldSceneAdapter.h"
#if HORDE_RT_STAGED_PRIMARY_TIMING
#include "vulkan/raytracing/experimental/StagedPrimaryProfile.h"
#endif

#ifndef HORDE_RT_BUILD_ID
#define HORDE_RT_BUILD_ID "development"
#endif
#ifndef HORDE_RT_DISPLAY_VERSION
#define HORDE_RT_DISPLAY_VERSION "development"
#endif
namespace
{

constexpr char kWindowClassName[] = "HordeRtDiagnosticWindowClass";
constexpr int kCampaignSaveButtonId=4300,kCampaignLoadButtonId=4301;
constexpr int kLookoutObjectiveButtonId=4302;
using LookoutPanel=horde::platform::windows::WindowsLookoutObjectivePanel;
// Debug validation owns only an unshown HWND, isolated save directory and RT
// targets. It must never acquire foreground input or touch the owner's INI.
bool gCampaignHiddenValidation=false;
bool gOpeningBenchmarkMenuValidation=false;
#if defined(_DEBUG)
bool gOpeningBenchmarkDetachedCancelValidation=false;
bool gOpeningBenchmarkDetachedCloseValidation=false;
bool gOpeningBenchmarkDetachedCancelInjected=false;
#endif
bool gLookoutHiddenValidation=false;
bool gRouteHiddenValidation=false;
bool gRouteKitTurnStartValidation=false;
bool gRouteNaturalWalkValidation=false;
bool gRouteReturnRecovery=false;
bool gLookoutInspectKit=false;
bool gLookoutInspectKitMotion=false;
bool gLookoutInspectKitTiming=false;
bool gLookoutInspectKitTurns=false;
bool gLookoutInspectKitPartials=false;
bool gLookoutInspectKitPartialsDense=false;
bool gLookoutInspectKitController=false;
bool gShowcaseCaptureHidden=false;
bool gCampaignValidationHoldPreparation=false;
std::filesystem::path gCampaignValidationDirectory;
horde::platform::windows::WindowsAcceptanceProfile gAcceptanceProfile;
int GameMessageBoxA(HWND owner,const char* text,const char* title,UINT flags) {
    if(gCampaignHiddenValidation || gOpeningBenchmarkMenuValidation || gShowcaseCaptureHidden){std::cerr<<"Hidden validation suppressed dialog: "<<text<<'\n';return IDCANCEL;}
    return ::MessageBoxA(owner,text,title,flags);
}
void InstallControllerFocusOutline(HWND control);
void FocusGameControl(HWND control) {
    if(!gCampaignHiddenValidation && !gOpeningBenchmarkMenuValidation && !gShowcaseCaptureHidden && control) ::SetFocus(control);
}
constexpr char kWindowTitle[] = "Horde Lantern RT - Showcase Alpha " HORDE_RT_DISPLAY_VERSION
#if HORDE_RT_WINDOWS_ACCEPTANCE
    " [isolated acceptance]"
#endif
    ;
constexpr char kHudStartingText[] = "ALPHA " HORDE_RT_DISPLAY_VERSION "  |  VULKAN RT STARTING...  |  F1 CONTROLS  |  ESC MENU";
constexpr char kHudActiveText[] = "RT ACTIVE | DIAGNOSTICS";
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
constexpr int kSfxVolumeLabelId = 165;
constexpr int kSfxVolumeSliderId = 166;
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
constexpr int kBenchmarkReviewStatsButtonId = 205;
constexpr int kVitalityHudControlId = 124;
constexpr int kEndingBodyId = 125;
constexpr int kWaterQualityButtonId = 126;
constexpr int kGraphicsOpenButtonId = 190;
constexpr int kGraphicsPresetButtonId = 191;
constexpr int kGraphicsFireButtonId = 192;
constexpr int kGraphicsApplyButtonId = 193;
constexpr int kGraphicsConfirmButtonId = 194;
constexpr int kGraphicsRevertButtonId = 195;
constexpr int kGraphicsResetButtonId = 196;
constexpr int kGraphicsInfoId = 197;
constexpr int kGraphicsPreviewPauseId = 198;
constexpr int kGraphicsPreviewCameraId = 199;
constexpr int kGraphicsPreviewMotionId = 200;
constexpr int kGraphicsPreviewResetId = 201;
constexpr int kGraphicsPreviewTelemetryId = 202;
constexpr int kGraphicsPreviewGraphId = 203;
constexpr int kGraphicsGlassButtonId = 204;
constexpr int kGraphicsShadowButtonId = 206;
constexpr int kGraphicsMistButtonId = 207;
constexpr int kEntryMoreButtonId = 208;
constexpr int kEntryBackButtonId = 209;
constexpr int kEntryLoadingIndicatorId = 210;
constexpr int kGraphicsDustButtonId = 211;
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
constexpr int kChestPromptControlId = 161;
constexpr int kCombatTeachingPromptId = 212;
constexpr int kMusicVolumeLabelId = 162;
constexpr int kMusicVolumeSliderId = 163;
constexpr int kSettingsMoreId = 213;
constexpr int kVoiceVolumeLabelId = 214;
constexpr int kVoiceVolumeSliderId = 215;
constexpr int kSubtitlesButtonId = 216;
constexpr int kSubtitleSizeLabelId = 217;
constexpr int kSubtitleSizeSliderId = 218;
constexpr int kSubtitlePositionButtonId = 219;
constexpr int kChapterSubtitleId = 221;
constexpr int kReportProblemButtonId = 164;
constexpr int kMenuPauseId = 2001;
constexpr int kMenuRestartId = 2002;
constexpr int kMenuExitId = 2003;
constexpr int kMenuSensitivityLowId = 2011;
constexpr int kMenuSensitivityNormalId = 2012;
constexpr int kMenuSensitivityHighId = 2013;
constexpr int kMenuFullscreenId = 2014;
constexpr int kMenuControlsId = 2020;
constexpr int kMenuDiagnosticsId = 2021;
constexpr int kMenuAboutId = 2022;
constexpr int kMenuCreditsId = 2023;
constexpr int kMenuDeveloperOverlayId = 2024;
constexpr int kMenuCheckUpdatesId = 2025;
constexpr int kMenuReportProblemId = 2026;
constexpr int kMenuCompatibilityReportId = 2027;
constexpr wchar_t kCompatibilityProperty[]=L"HordeCompatibilityOwnerV1";
struct CompatibilityWindowOwner
{
    horde::reporting::CompatibilityReportSnapshot snapshot;
    horde::reporting::CompatibilityReportSnapshot probeSnapshot;
    horde::reporting::CompatibilityStage stage=horde::reporting::CompatibilityStage::WindowCreation;
    bool reviewOpen=false, failureCleanup=false;
};
CompatibilityWindowOwner* CompatibilityFor(HWND window)
{return reinterpret_cast<CompatibilityWindowOwner*>(GetPropW(window,kCompatibilityProperty));}
void SetCompatibilityAttemptStage(HWND window,horde::reporting::CompatibilityStage stage)
{if(auto* owner=CompatibilityFor(window))owner->stage=stage;}
void CompatibilityFailure(CompatibilityWindowOwner& owner)
{
    using namespace horde::reporting;
    owner.snapshot.lastStage=owner.stage;
    owner.snapshot.outcome=owner.snapshot.rtPresented?CompatibilityOutcome::RuntimeFailure:CompatibilityOutcome::StartupFailure;
}
void CompatibilityPresentation(HWND window,horde::vulkan::RtExecutionBackend backend)
{
    using namespace horde::reporting;
    if(backend!=horde::vulkan::RtExecutionBackend::RayTracingPipeline && backend!=horde::vulkan::RtExecutionBackend::RayQueryCompute)return;
    if(auto* owner=CompatibilityFor(window)) {
        owner->snapshot.backend=backend==horde::vulkan::RtExecutionBackend::RayTracingPipeline?
            CompatibilityBackend::RayTracingPipeline:CompatibilityBackend::RayQueryCompute;
        owner->snapshot.rtPresented=true;owner->snapshot.outcome=CompatibilityOutcome::Presented;
        owner->snapshot.lastStage=CompatibilityStage::Presentation;
    }
}
CompatibilityWindowOwner CaptureWindowsCompatibilityProbe(const horde::vulkan::VulkanContext& probe,
    const bool initialised,const horde::vulkan::DeviceCapabilities& capabilities)
{
    using namespace horde::reporting;
    CompatibilityWindowOwner owner;auto& s=owner.snapshot;
    s.version=HORDE_RT_PACKAGE_VERSION;s.build=CurrentBuildIdentity().ReportLabel();
    s.platform=CompatibilityPlatform::Windows;s.lastStage=CompatibilityStage::CapabilityProbe;
    s.probe=probe.HasConfirmedNoPhysicalDevices()?CompatibilityProbe::NoDevice:CompatibilityProbe::Unavailable;
    s.outcome=probe.HasConfirmedNoPhysicalDevices()?CompatibilityOutcome::Unsupported:CompatibilityOutcome::ProbeUnavailable;
    // The API-reported OS value can depend on the executable manifest or user
    // compatibility mode. Preserve that caveat; never infer RT support from it.
#pragma warning(push)
#pragma warning(disable:4996)
    OSVERSIONINFOW os{sizeof(OSVERSIONINFOW)};
    if(GetVersionExW(&os))s.os="Windows API major "+std::to_string(os.dwMajorVersion)+" minor "+
        std::to_string(os.dwMinorVersion)+" build "+std::to_string(os.dwBuildNumber)+" (manifest/compatibility dependent)";
#pragma warning(pop)
    const auto physical=probe.GetSelectedPhysicalDevice();
    if(initialised && physical!=VK_NULL_HANDLE) {
        s.probe=CompatibilityProbe::DeviceChecked;s.outcome=CompatibilityOutcome::CapabilityOnly;
        s.gpu=capabilities.identity.gpuName;s.rawDriverVersion=capabilities.identity.driverVersion;s.vulkanApiVersion=capabilities.identity.vulkanApiVersion;
        std::uint32_t count=0;bool checked=vkEnumerateDeviceExtensionProperties(physical,nullptr,&count,nullptr)==VK_SUCCESS;
        std::vector<VkExtensionProperties> extensions;
        if(checked && count<=4096u) {extensions.resize(count);const auto capacity=count;
            if(count)checked=vkEnumerateDeviceExtensionProperties(physical,nullptr,&count,extensions.data())==VK_SUCCESS && count<=capacity;
            if(checked)extensions.resize(count);
        } else checked=false;
        const auto extension=[&](bool observed,const char* name){
            if(!checked)return observed?CompatibilityCheck::Present:CompatibilityCheck::Unknown;
            return std::any_of(extensions.begin(),extensions.end(),[&](const auto& e){return std::strncmp(e.extensionName,name,VK_MAX_EXTENSION_NAME_SIZE)==0;})?
                CompatibilityCheck::Present:CompatibilityCheck::Absent;};
        const auto& e=capabilities.extensions;
        s.extensions={extension(e.accelerationStructure,VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME),extension(e.rayTracingPipeline,VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME),
            extension(e.rayQuery,VK_KHR_RAY_QUERY_EXTENSION_NAME),extension(e.bufferDeviceAddress,VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME),extension(e.deferredHostOperations,VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)};
        const auto feature=[](bool b){return b?CompatibilityCheck::Present:CompatibilityCheck::Unknown;};const auto& f=capabilities.features;
        s.features={feature(f.accelerationStructure),feature(f.rayTracingPipeline),feature(f.rayQuery),feature(f.bufferDeviceAddress)};
        const auto& x=s.extensions;
        if((capabilities.identity.vulkanApiVersion!=0 && capabilities.identity.vulkanApiVersion<VK_API_VERSION_1_2) ||
            x.accelerationStructure==CompatibilityCheck::Absent || x.bufferDeviceAddress==CompatibilityCheck::Absent || x.deferredHostOperations==CompatibilityCheck::Absent ||
            (x.rayTracingPipeline==CompatibilityCheck::Absent && x.rayQuery==CompatibilityCheck::Absent))s.outcome=CompatibilityOutcome::Unsupported;
    }
    owner.probeSnapshot=s;return owner;
}
constexpr int kMenuTeachingEnabledId = 2030;
constexpr int kMenuTeachingSlowdownId = 2031;
constexpr int kMenuTeachingSkipId = 2032;
constexpr int kMenuTeachingReplayId = 2033;
constexpr int kBenchmarkStandardChoiceId = 230;
constexpr int kBenchmarkCustomChoiceId = 231;
constexpr int kBenchmarkChoiceRunId = 232;
constexpr int kBenchmarkChoiceBackId = 233;
constexpr int kBenchmarkChoiceInfoId = 234;
constexpr int kAppIconId = 1;
constexpr UINT kDefaultDpi = 96u;
constexpr char kUiFontProperty[] = "HordeLanternRtUiFont";
constexpr char kEntryTitleFontProperty[] = "HordeLanternRtEntryTitleFont";
constexpr char kEntryPlaqueFontProperty[] = "HordeLanternRtEntryPlaqueFont";
constexpr char kCombatTeachingPromptFontProperty[] = "HordeLanternRtCombatTeachingPromptFont";
constexpr char kSubtitleFontProperty[] = "HordeLanternRtSubtitleFont";
constexpr char kGraphicsInfoFontProperty[] = "HordeLanternRtGraphicsInfoFont";
constexpr char kMonoFontProperty[] = "HordeLanternRtMonoFont";
constexpr char kDeveloperFontProperty[] = "HordeLanternRtDeveloperFont";
constexpr char kCaptureModeProperty[] = "HordeLanternRtCaptureMode";
constexpr char kOpeningBenchmarkTransferProperty[] = "HordeLanternRtOpeningBenchmarkTransfer";
// One frame in flight keeps the dynamically refit held-torch TLAS safely synchronized with its host-written instance buffer.
constexpr UINT kMaxFramesInFlight = 1u;

struct CaptureLaunchOptions
{
    horde::platform::windows::SkyAttributionMode skyAttribution =
        horde::platform::windows::SkyAttributionMode::Production;
    horde::platform::windows::WindowsBenchmarkLaunch benchmark;
    bool requested = false;
    bool captureHidden = false;
    horde::platform::windows::WindowsEnvironmentSurveyLaunch environmentSurvey;
    bool graphicsPreview = false;
    horde::platform::windows::PreviewMeshPairCaptureMode previewMeshPair =
        horde::platform::windows::PreviewMeshPairCaptureMode::None;
    horde::platform::windows::ForestRouteCaptureMode forestRouteProof=
        horde::platform::windows::ForestRouteCaptureMode::Disabled;
    bool mistOff = false;
    std::optional<horde::graphics::DustQuality> captureDustQuality;
    bool outputResizeValidation = false;
    std::string nativeMotionScenario;
    horde::vulkan::raytracing::RtWorkloadPreset nativeMotionRtWorkloadPreset =
        horde::vulkan::raytracing::RtWorkloadPreset::Authored;
    bool requireRayQueryCompute = false;
    bool portrait = false;
    bool anatomicalPlayerMount = false;
    bool validateStaticZoneCutover = false;
    bool validateStaticTriangleCutover = false;
    std::vector<std::uint32_t> staticTriangleOmissions;
    std::string inspectStaticWorldMaterial;
    std::string forestContentCandidate;
    bool validateForestContentCandidate = false;
    bool validateChapterRoundTrip = false;
    bool chapterLandingOnly = false;
    bool captureGpuStages = false;
    bool measureChapterShippingRoute = false;
    bool measureChapterForeground = false;
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
    // Record the rendered simulation state, not an unclamped authored request.
    std::array<float, 4u> camera{};
    std::string torchFailurePhase;
    std::string selectedEnemy;
    std::string lichPhase;
    float finaleSkylightOpenProgress = 0.0f;
    std::string filename;
    std::string pngSha256;
    std::string completedFrameEvidenceJson;
    std::string viewmodelGeometryFile;
    std::string viewmodelGeometrySha256;
    horde::platform::windows::CapturedPlayerGeometryEvidence viewmodelGeometryEvidence{};
    std::string playerWorldBodyGeometryFile;
    std::string playerWorldBodyGeometrySha256;
    horde::platform::windows::CapturedPlayerGeometryEvidence worldBodyGeometryEvidence{};
    horde::graphics::DustQuality actualUploadedDustQuality = horde::graphics::DustQuality::Off;
    horde::scene::atmosphere::DustWork dustWork{};
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    bool redBlueSwapNormalised = false;
    std::array<std::uint8_t,
               horde::vulkan::raytracing::PresentableTinyRtScene::kTlasInstanceCount>
        instanceMasks{};
    bool playerPrimaryVisible = false;
    bool primaryArmsMayBeOutsideFrame = false;
    bool primaryArmsCeilingRetracted = false;
    float playerSupportWorldY = 0.0f;
    horde::gameplay::traversal::RescueTraversalSnapshot rescue{};
    bool rescueBodyStowWitness = false;
    std::array<float,3u> renderedEyePosition{};
    horde::gameplay::items::HeldItemTransform publishedLanternHinge{};
    horde::gameplay::items::HeldItemTransform publishedLanternBody{};
    horde::gameplay::dialogue::CompanionSnapshot chapterCompanion{};
    float keeperDissolve = 0.0f;
    float playerHeightDelta = 0.0f;
    std::uint8_t playerSupportId = 0u;
    std::uint8_t heldLightKind = 0u;
    float swordStowBlend = 0.0f;
    float swordHandGripBlend = 0.0f;
    float torchOverheadLoweringMetres = 0.0f;
    float torchOverheadRetractionMetres = 0.0f;
    std::uint32_t playerWorldBodyInstanceFlags = 0u;
    std::uint32_t primaryTorchPixels = 0u;
    std::uint32_t primarySwordPixels = 0u;
    std::uint32_t primaryPlayerPixels = 0u;
    std::uint32_t primaryRewardRingPixels = 0u;
    std::uint32_t primaryRewardBodyPixels = 0u;
    float rewardGripPositionErrorMetres = 0.0f;
    float rewardGripOrientationErrorRadians = 0.0f;
    float rewardAuthorityPositionErrorMetres = 0.0f;
    float rewardAuthorityOrientationErrorRadians = 0.0f;
    std::array<float, 3u> rewardFinalGripPosition{};
    std::array<float, 3u> rewardRingGripPosition{};
    std::array<float, 3u> rewardBodyPosition{};
    std::vector<double> frameTimesMs;
};

struct WindowsBenchmarkSummaryArm
{
    horde::telemetry::BenchmarkSummaryConfiguration configuration;
    horde::graphics::GraphicsSettings applied;
    horde::vulkan::raytracing::RtSceneProfile profile = horde::vulkan::raytracing::RtSceneProfile::Showcase;
};

using OpeningBenchmarkChoice = horde::platform::windows::OpeningBenchmarkChoice;
using OpeningBenchmarkOwnerPhase = horde::platform::windows::OpeningBenchmarkOwnerPhase;
struct OpeningBenchmarkTransfer
{
    horde::platform::windows::OpeningBenchmarkOwnerTransfer owner;
    horde::telemetry::RtLifecycleSeeds seeds{};
    horde::graphics::GraphicsSettings personalGraphics{}, savedGraphics{};
    horde::gameplay::BenchmarkProfileConfiguration entryActual{};
    WINDOWPLACEMENT windowedPlacement{sizeof(WINDOWPLACEMENT)};
    bool hasPersonal = false, fullscreen = false, closeRequested = false, cancelPending = false;
    std::filesystem::path reports;
    std::string status, statusTitle;
    // Completed CPU values only; no renderer, campaign or mutable UI ownership
    // survives the benchmark-to-Entry retirement boundary.
    std::string completedReport;
    std::optional<horde::telemetry::FrozenBenchmarkSummary> completedSummary;
};

struct VulkanSurfaceContext
{
    VulkanSurfaceContext() = default;

    HWND windowHandle = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
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
    // Present waits belong to swapchain images, not graphics fence slots.
    std::vector<VkSemaphore> renderFinishedSemaphores;
    horde::vulkan::PresentSurfaceSupport presentSurfaceSupport{};
    horde::vulkan::PresentCompletionMode presentCompletionMode = horde::vulkan::PresentCompletionMode::Unextended;
    horde::vulkan::PresentCompletionFences presentCompletionFences;
    bool imageAcquirePending = false;
    std::vector<VkFence> inFlightFences;
    horde::vulkan::raytracing::PresentableTinyRtScene rtScene;
    horde::vulkan::GpuFrameTimer gpuFrameTimer;
#if HORDE_RT_FRAME_STAGE_PROBE
    bool captureGpuStages=false;
    std::array<horde::vulkan::GpuFrameTimer,3> frameStageTimers;
    std::array<horde::vulkan::raytracing::RtSceneGpuStageCompletedObservation,kMaxFramesInFlight> pendingStageOwners{};
    horde::vulkan::raytracing::RtSceneGpuStageCompletedObservation completedStageObservation{};
#endif
#if HORDE_RT_STAGED_PRIMARY_TIMING
    horde::vulkan::raytracing::experimental::StagedPrimaryTiming stagedPassTimer;
    horde::vulkan::raytracing::experimental::StagedPrimaryProfile stagedPassProfile;
#endif
    horde::vulkan::raytracing::RtFrameEvidenceCoordinator rtFrameEvidence;
    horde::vulkan::GpuRtTimingSnapshot gpuRtTiming;
    double gpuFrameTimingTotalMs = 0.0;
    std::uint64_t gpuFrameTimingSampleCount = 0u;
    bool rtFrameEvidenceInitialised = false;
    horde::telemetry::RtPresentationOutcome lastFramePresentation =
        horde::telemetry::RtPresentationOutcome::NotAttempted;
    // Borrowed from RunDiagnosticSwapchainWindow's live owner; UI captures a
    // copied report context on this same application thread.
    const horde::vulkan::DeviceCapabilities* capabilitySnapshot = nullptr;
    bool useRtPath = false;
#if defined(_DEBUG)
    std::optional<horde::vulkan::raytracing::EnvironmentSurveyPose> lookoutKitInspectionPose;
    horde::gameplay::dialogue::CompanionActingPose lookoutKitInspectionActing{};
    std::optional<horde::gameplay::dialogue::CompanionSnapshot> lookoutKitInspectionMotion;
    std::optional<horde::vulkan::raytracing::ChapterActorInspectionPose> lookoutKitInspectionTurn;
#endif
    horde::vulkan::RtExecutionBackend executionBackend = horde::vulkan::RtExecutionBackend::Unsupported;
    std::string developmentCheckpoint;
    std::string lastRtFrameError;
    bool controlsEnabled = false;
    bool combatTeachingEnabled = true;
    bool combatTeachingSlowdown = false;
    bool reducedMotionEnabled = false;
    std::string combatTeachingFadeText;
    bool combatAttackCueVisible = false;
    bool combatPracticeCueVisible = false;
    unsigned char combatTeachingFadeOpacity = 0u;
    std::uint64_t combatTeachingFadeStartedAtMs = 0u;
    std::uint64_t combatTeachingSkipSequence = 0u;
    std::uint64_t combatTeachingReplaySequence = 0u;
    bool simulationPaused = true;
    bool pauseMenuVisible = true;
    bool settingsVisible = false;
    bool graphicsVisible = false;
    bool graphicsCloseAfterRevert = false;
    // Debug automation uses an isolated in-memory graphics transaction only.
    // It never loads or writes the user's INI, advances gameplay, or starts audio.
    bool entryMenuVisible = false;
    bool entryMoreVisible = false;
    bool entryPlayQueued = false;
    bool entryPlayHandoffPending = false;
    bool entryLoadingVisible = false;
    bool lookoutAppActive=true,lookoutMovementNeedsFreshKeyDown=false;
    std::unique_ptr<LookoutPanel> lookoutObjective;
    horde::graphics::EntryMenuSession entryMenu;
    horde::audio::MenuCreakDelivery menuCreakDelivery;
    horde::vulkan::raytracing::RtSceneProfile graphicsReturnProfile =
        horde::vulkan::raytracing::RtSceneProfile::Showcase;
    bool graphicsPreviewCapture = false;
#if defined(_DEBUG)
    horde::platform::windows::PreviewMeshPairCaptureMode previewMeshPair =
        horde::platform::windows::PreviewMeshPairCaptureMode::None;
    std::shared_ptr<const horde::scene::assets::StaticMeshAsset> previewMeshPairMaster;
    std::optional<horde::graphics::PreviewCameraPose> previewMeshPairCamera;
    horde::platform::windows::ForestRouteCaptureMode forestRouteProof=
        horde::platform::windows::ForestRouteCaptureMode::Disabled;
#endif
    bool outputResizeValidation = false;
    bool nativeMotionValidation = false;
    bool chapterRoundTripCaptureActive = false;
#if defined(_DEBUG) || HORDE_RT_CHAPTER_MEASUREMENT
    std::uint64_t lastSuccessfulPresentReturnSteadyNs = 0;
#endif
    horde::platform::windows::SkyAttributionMode skyAttribution =
        horde::platform::windows::SkyAttributionMode::Production;
#if defined(_DEBUG) || HORDE_RT_CHAPTER_MEASUREMENT
    bool staticReplacementFrameRequested = false;
    std::uint64_t staticReplacementExpectedGeneration = 0u;
    std::uint64_t staticReplacementProofGeneration = 0u;
    horde::gameplay::validation::MotionEvidenceScenario motionScenario;
    horde::gameplay::validation::MotionScenario motionRequestedScenario{};
    horde::vulkan::raytracing::RtWorkloadPreset motionRequestedRtPreset =
        horde::vulkan::raytracing::RtWorkloadPreset::Authored;
    horde::telemetry::MotionEvidenceLedger motionLedger;
    bool motionArmed = false;
    std::uint32_t motionRetryGeneration = 0;
    bool motionRetryPending = false;
    std::uint64_t motionSurfaceGeneration = 1;
    bool resizePresentTimestampArmed = false;
    std::uint64_t resizeFirstPresentNanoseconds = 0u;
    double lastOutputResizeIdleMilliseconds = 0.0;
#endif
    std::optional<horde::graphics::GraphicsEditSession> graphicsEdit;
    std::optional<horde::graphics::GraphicsCommand> graphicsCommand;
    horde::graphics::GraphicsSettings savedGraphics =
        horde::graphics::PlatformDefaultGraphicsSettings(horde::graphics::GraphicsPlatform::Windows);
    horde::graphics::GraphicsSettings graphicsBeforeApply = savedGraphics;
    horde::graphics::FireDetail fireDetail = horde::graphics::FireDetail::High;
    horde::graphics::ShadowQuality shadowQuality = horde::graphics::ShadowQuality::Current;
    // Requested intent is distinct from the ready scene's applied geometry policy.
    bool requestedGlassEnabled = true;
    bool requestedMistEnabled = true;
    horde::graphics::DustQuality requestedDustQuality = savedGraphics.dustQuality;
    bool glassGeometryDirty = false;
    std::uint64_t graphicsSerialFloor = 0u;
    ULONGLONG graphicsConfirmationTick = 0u;
    std::string graphicsStatus;
    horde::vulkan::raytracing::RtSceneProfile sceneProfile = horde::vulkan::raytracing::RtSceneProfile::Showcase;
    bool sceneProfileDirty = false;
    horde::graphics::GraphicsPreviewSession graphicsPreview;
    horde::graphics::GraphicsPreviewPerformance graphicsPreviewPerformance;
    horde::graphics::GraphicsPreviewCamera graphicsPreviewCamera = horde::graphics::GraphicsPreviewCamera::Overview;
    bool graphicsPreviewPaused = false;
    bool graphicsPreviewMotion = false;
    bool graphicsSceneRestoring = false;
    int graphicsPreviewFrameCap = 30;
    std::uint64_t graphicsPreviewEpoch = 0u;
    double graphicsPreviewDelta = 0.0;
    std::chrono::steady_clock::time_point graphicsPreviewLastFrame{};
    std::chrono::steady_clock::time_point graphicsPreviewLastSample{};
    std::uint64_t graphicsPreviewLastGpuSample = 0u;
    ULONGLONG graphicsPreviewLastUiTick = 0u;
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
    int sfxVolumePercent = 100;
    int musicVolumePercent = 70;
    int voiceVolumePercent = 70;
    int subtitleSizePercent = 100;
    int appliedSubtitleFontPixels = 0;
    horde::gameplay::dialogue::SubtitlePosition subtitlePosition =
        horde::gameplay::dialogue::SubtitlePosition::Auto;
    bool subtitlesEnabled = true;
    bool settingsDialoguePage = false;
    std::uint64_t dialogueSkipSequence = 0u;
    std::uint32_t dialogueCompletionLine = 0u;
    std::uint64_t dialogueCompletionGeneration = 0u;
    horde::gameplay::dialogue::Line presentedDialogueLine = horde::gameplay::dialogue::Line::None;
    std::uint64_t presentedDialogueGeneration = 0u;
    horde::platform::windows::SubtitlePlacementLatch subtitlePlacementLatch{};
    horde::platform::windows::SubtitlePaintKey subtitlePaintKey{};
    bool subtitleBitmapCurrent = false;
    std::uint32_t playingDialogueLine = 0u;
    std::uint64_t playingDialogueGeneration = 0u;
    std::uint64_t loggedDialogueAttemptGeneration = 0u;
    bool loggedDialogueAttemptAccepted = false;
    std::uint64_t lastChapterPresentationAckTick = 0u;
    std::uint64_t lastChapterPresentationAckGeneration = 0u;
    std::unique_ptr<horde::platform::windows::WindowsMusicPlayback> musicPlayback;
    std::uint64_t musicEventLifetimeToken = 1u;
    std::uint64_t musicResetToken = 0u;
    horde::platform::windows::PrivateLichRevivalAudioState privateRevivalAudio;
    int musicLastLoggedGate = -1;
    bool fullscreen = false;
    bool forwardHeld = false;
    bool backwardHeld = false;
    bool leftHeld = false;
    bool rightHeld = false;
    bool runHeld = false;
    bool controllerRunHeld = false;
    bool controllerRunBlockedUntilRelease = false;
    bool crouchHeld = false;
    bool crouchKeyBlockedUntilRelease = false;
    bool controllerCrouchHeld = false;
    bool controllerCrouchBlockedUntilRelease = true;
    bool crouchTraversalOwned = false;
    bool runToggleKeyDown = false;
    bool mouseLookActive = false;
    bool chestInteractionPromptPresented = false;
    bool rescueInteractionPromptPresented = false;
    bool flaskUpgradeInteractionPromptPresented = false;
    bool mouseCursorHidden = false;
    bool environmentSurveyActive=false,environmentSurveyWaterDisabled=false,environmentSurveyDebrisDisabled=false;
    std::string environmentSurveyProfile;
#if defined(_DEBUG)
    std::optional<horde::gameplay::simulation::SimulationSnapshot> playerDodgeInspectionSnapshot;
    std::optional<horde::vulkan::raytracing::SkeletonPoseBlendRequests> inspectionSkeletonBlends;
    bool playerDodgeInspectionDisabled=false;
#endif
    float environmentSurveyTime=0;
    horde::platform::windows::EnvironmentSurveyCamera environmentSurveyCamera{};
    POINT mouseRestorePosition{};
    POINT lastMousePosition{};
    float controllerForward = 0.0f;
    float controllerStrafe = 0.0f;
    float controllerLookHorizontal = 0.0f;
    float controllerLookVertical = 0.0f;
    bool teachingControllerGlyphs = false;
    bool keyboardGuardHeld = false;
    bool guardHeld = false;
    horde::platform::windows::GuardHoldLatch controllerGuardHold;
    WORD previousControllerButtons = 0u;
    WORD previousXInputUiButtons = 0u;
    horde::platform::windows::ControllerFocusLatch controllerFocusLatch;
    DWORD previousLegacyControllerButtons = 0u;
    DWORD previousLegacyUiButtons = 0u;
    DWORD previousLegacyPov = JOY_POVCENTERED;
    horde::platform::windows::ControllerTriggerLatch controllerTriggerLatch;
    std::optional<DWORD> xInputUserIndex;
    std::optional<UINT> legacyJoystickId;
    horde::platform::windows::LegacyRightStickAxes legacyRightStickAxes;
    std::uint64_t lastControlSteadyNs = 0u;
#if !defined(NDEBUG)
    horde::telemetry::CombatTimingTrace combatTimingTrace;
#endif
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
    float appliedRenderScale = 1.0f;
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
    std::uint64_t drinkFlaskSequence=0,restSequence=0;
    std::uint64_t toggleHeldLightPoseSequence = 0u;
    std::uint64_t runToggleSequence = 0u;
    std::uint64_t clearRunIntentSequence = 0u;
    std::uint64_t crouchToggleSequence = 0u;
    std::uint64_t clearCrouchIntentSequence = 0u;
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
    bool developmentVerticalProof = false;
    bool developmentWorldRoute = false;
    bool stagedWorldPreparation = false;
    bool developmentRescueJourney = false;
    bool developmentCombatPractice = false;
    bool developmentKeeperPractice = false;
    horde::gameplay::ShowcaseBenchmarkRun benchmark;
    horde::vulkan::raytracing::RtWorkloadPreset benchmarkRequestedRtPreset =
        horde::vulkan::raytracing::RtWorkloadPreset::Authored;
    horde::vulkan::raytracing::RtWorkloadPreset benchmarkEffectiveRtPresetAtStart =
        horde::vulkan::raytracing::RtWorkloadPreset::Authored;
    std::optional<horde::vulkan::raytracing::RtQualityControlsGpu> benchmarkQualityControlsAtStart;
    std::string benchmarkCompiledQualityAtStart;
    horde::telemetry::RtBenchmarkEvidenceRun benchmarkEvidence;
    std::optional<WindowsBenchmarkSummaryArm> benchmarkSummaryArm;
    std::optional<horde::telemetry::FrozenBenchmarkSummary> benchmarkSummary;
    std::optional<std::size_t> expectedBenchmarkFrame;
    std::string benchmarkReport;
    std::string benchmarkJsonReport;
    bool benchmarkCompletionHandled = false;
    bool benchmarkReportsSaved = false;
    bool unattendedBenchmark = false;
    OpeningBenchmarkTransfer* openingBenchmark = nullptr;
    bool openingBenchmarkOwner = false, openingBenchmarkReturnPending = false;
    bool benchmarkChooserVisible = false;
    OpeningBenchmarkChoice benchmarkChoice = OpeningBenchmarkChoice::None;
    bool benchmarkProfileCustom = false;
    bool benchmarkProfileRequested = false, benchmarkProfileActive = false;
    bool benchmarkProfileInternalResize = false, benchmarkProfileCancelRequested = false;
    bool benchmarkProfileCloseRequested = false;
    bool benchmarkProfileStaleOwnerRejected = false;
    bool benchmarkProfileValidationPassed = false;
    unsigned benchmarkProfileValidationFrames = 0;
    std::unique_ptr<horde::platform::windows::WindowsBenchmarkProfileSession> benchmarkProfile;
    WINDOWPLACEMENT benchmarkProfileOriginalPlacement{sizeof(WINDOWPLACEMENT)};
    WINDOWPLACEMENT benchmarkProfileOriginalWindowedPlacement{sizeof(WINDOWPLACEMENT)};
    LONG_PTR benchmarkProfileOriginalStyle = 0, benchmarkProfileOriginalExStyle = 0;
    LONG_PTR benchmarkProfileAppliedStyle = 0, benchmarkProfileAppliedExStyle = 0;
    HMENU benchmarkProfileOriginalMenu = nullptr;
    bool benchmarkProfileOriginalFullscreen = false, benchmarkProfileWindowStateRestored = false;
    bool benchmarkProfileTemporaryMenuDetached = false;
    RECT benchmarkProfileOriginalRect{}, benchmarkProfileOriginalClient{};
    RECT benchmarkProfileMonitorRect{}, benchmarkProfileTargetRect{}, benchmarkProfileObservedClient{}, benchmarkProfileAppliedRect{};
    horde::graphics::GraphicsSettings benchmarkProfileOriginalIntent{};
    horde::graphics::GraphicsSettings benchmarkProfileOriginalSaved{};
    horde::telemetry::RtLifecyclePublishedState benchmarkProfileBefore{}, benchmarkProfileApplied{}, benchmarkProfileAfter{};
    std::string benchmarkProfileError;
    std::filesystem::path benchmarkProfileJsonPath;
    std::unique_ptr<horde::gameplay::campaign::PlaySession> campaign;
    std::unique_ptr<horde::platform::windows::WindowsCampaignMenu> campaignMenu;
    std::optional<horde::platform::windows::WindowsCampaignMenu::Command> campaignCommand;
    horde::gameplay::campaign::CampaignLoadBarrier campaignBarrier;
    std::optional<horde::gameplay::campaign::LoadPreparation> campaignPreparation;
    std::unique_ptr<horde::vulkan::raytracing::PresentableTinyRtScene> campaignPreparedScene;
    horde::platform::windows::WindowsCampaignMenu::Phase campaignPhase=
        horde::platform::windows::WindowsCampaignMenu::Phase::Unavailable;
    std::string campaignMessage,campaignStorageWarning;
    std::uint64_t campaignSerial=0,campaignSurfaceGeneration=1,campaignFrameTick=0,campaignFrameWorld=0;
    std::uint64_t campaignUncertaintyRecheck=0,campaignVerifiedUncertainty=0;
    std::uint64_t campaignRetryOperationRevision=0;
    std::chrono::steady_clock::time_point campaignStarted{};
    bool campaignPause=false,campaignIsolated=false,campaignRetirementUnproved=false,campaignNewGame=false,campaignConfirmedRetry=false;
    uint32_t currentFrame = 0u;
};

bool WriteReportFile(const std::filesystem::path& path, const std::string& data);
#if defined(_DEBUG)
bool MaybeAdmitForestContentCandidate(VulkanSurfaceContext& context, std::string& error);
#endif
bool SaveGraphicsRecord(const VulkanSurfaceContext& context,
                        const horde::graphics::GraphicsPersistenceRecord& record);
bool ObserveWindowsBenchmarkProfileMeasurement(VulkanSurfaceContext& context, bool requireCompleted);
void ClearDesktopInput(VulkanSurfaceContext& context);
void DiscardDesktopPendingCommands(VulkanSurfaceContext& context);
void UpdateVitalityHud(VulkanSurfaceContext& context);
void UpdateChestPrompt(VulkanSurfaceContext& context);
int ScaleForDpi(HWND window, int logicalPixels);
void LayoutOverlayControls(HWND window, int width, int height);
void ApplyDpiScaledFonts(HWND window);
horde::platform::windows::OpeningBenchmarkEligibility OpeningBenchmarkEligibilityFor(const VulkanSurfaceContext&);
void RefreshOpeningBenchmarkChooser(VulkanSurfaceContext&);
void InstallControllerFocusOutline(HWND control);
void SetEntryLoadingVisible(VulkanSurfaceContext&,bool);
void OpenCampaignMenu(VulkanSurfaceContext&,horde::platform::windows::WindowsCampaignMenu::Page);
void RefreshCampaignMenu(VulkanSurfaceContext&);
void PumpCampaign(VulkanSurfaceContext&);
void ObserveCampaignPresentation(VulkanSurfaceContext&,bool);
std::string WindowSafeText(const std::string& value);
void RefreshGpuTimingTelemetry(
    VulkanSurfaceContext& context,
    const horde::vulkan::GpuFrameTimingCollection* completed = nullptr);
horde::telemetry::RtSampleStatus CurrentInitialGpuEvidenceStatus(
    VulkanSurfaceContext& context);
bool CompleteRtEvidenceAfterDeviceIdle(VulkanSurfaceContext& ctx, VkResult idleResult);

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
    std::vector<std::wstring_view> argumentViews;
    for (int index = 1; index < argumentCount; ++index) argumentViews.emplace_back(arguments[index]);
    const auto forestProof=horde::platform::windows::ParseForestRouteProofLaunch(argumentViews,
#if defined(_DEBUG)
        true
#else
        false
#endif
    );
    if(!forestProof.error.empty()){options.error=forestProof.error;LocalFree(arguments);return options;}
    options.forestRouteProof=forestProof.mode;
    options.chapterLandingOnly=forestProof.landingOnly;
    options.captureGpuStages=forestProof.gpuStages;
#if !HORDE_RT_FRAME_STAGE_PROBE
    if(options.captureGpuStages) {
        options.error="GPU stage capture requires the explicit HORDE_RT_FRAME_STAGE_PROBE build.";
        LocalFree(arguments);return options;
    }
#endif
    const auto previewPair=horde::platform::windows::ParsePreviewMeshPairCaptureLaunch(argumentViews,
#if defined(_DEBUG)
        true
#else
        false
#endif
    );
    if(!previewPair.error.empty()){options.error=previewPair.error;LocalFree(arguments);return options;}
    options.previewMeshPair=previewPair.mode;
    if(previewPair.mode!=horde::platform::windows::PreviewMeshPairCaptureMode::None) {
        options.requested=true;options.graphicsPreview=true;options.outputDirectory=previewPair.outputDirectory;
    }
    const auto hiddenCapture=horde::platform::windows::ParseWindowsHiddenCaptureLaunch(argumentViews,
#if defined(_DEBUG)
        true
#else
        false
#endif
    );
    if(!hiddenCapture.error.empty()) {options.error=hiddenCapture.error;LocalFree(arguments);return options;}
    options.captureHidden=hiddenCapture.hidden || previewPair.mode!=horde::platform::windows::PreviewMeshPairCaptureMode::None;
    options.environmentSurvey=horde::platform::windows::ParseWindowsEnvironmentSurveyLaunch(argumentViews,
#if defined(_DEBUG)
        true
#else
        false
#endif
    );
    if(!options.environmentSurvey.error.empty()) {options.error=options.environmentSurvey.error;LocalFree(arguments);return options;}
    const auto skyLaunch=horde::platform::windows::ParseSkyAttributionLaunch(argumentViews,
#if defined(_DEBUG)
        true
#else
        false
#endif
    );
    if(!skyLaunch.error.empty()) {options.error=skyLaunch.error;LocalFree(arguments);return options;}
    options.skyAttribution=skyLaunch.mode;
    const auto combatPractice = horde::platform::windows::ParseWindowsCombatPracticeLaunch(argumentViews,
#if defined(_DEBUG)
        true
#else
        false
#endif
    );
    if (!combatPractice.error.empty())
    { options.error = combatPractice.error; LocalFree(arguments); return options; }
    const auto mistLaunch = horde::platform::windows::ParseWindowsMistCaptureLaunch(argumentViews,
#if defined(_DEBUG)
        true
#else
        false
#endif
    );
    if (!mistLaunch.error.empty())
    { options.error = mistLaunch.error; LocalFree(arguments); return options; }
    options.mistOff = mistLaunch.off;
    options.captureDustQuality = mistLaunch.dustQuality;
    const auto motionLaunch = horde::platform::windows::ParseWindowsMotionEvidenceLaunch(argumentViews,
#if defined(_DEBUG)
        true
#else
        false
#endif
    );
    if (!motionLaunch.error.empty())
    { options.error = motionLaunch.error; LocalFree(arguments); return options; }
    if (motionLaunch.requested)
    {
#if defined(_DEBUG)
        options.requested = true; options.nativeMotionScenario = motionLaunch.scenario;
        options.nativeMotionRtWorkloadPreset = motionLaunch.rtWorkloadPreset;
        options.outputDirectory = std::filesystem::path(motionLaunch.outputDirectory);
#else
        options.error = "--validate-native-motion and --motion-scenario are Debug-only validation controls.";
        LocalFree(arguments); return options;
#endif
    }
    const auto resizeValidation = horde::platform::windows::ParseOutputResizeValidationLaunch(argumentViews);
    if (!resizeValidation.error.empty())
    { options.error = resizeValidation.error; LocalFree(arguments); return options; }
    if (resizeValidation.requested)
    {
        options.requested = true; options.outputResizeValidation = true;
        options.outputDirectory = std::filesystem::path(resizeValidation.outputDirectory);
    }
    const auto previewCapture = horde::platform::windows::ParseGraphicsPreviewCaptureLaunch(argumentViews);
    if (!previewCapture.error.empty())
    { options.error = previewCapture.error; LocalFree(arguments); return options; }
    if (previewCapture.requested)
    {
        options.requested = true; options.graphicsPreview = true;
        options.outputDirectory = std::filesystem::path(previewCapture.outputDirectory);
    }
    options.benchmark = horde::platform::windows::ParseWindowsBenchmarkLaunch(argumentViews);
    if (!options.benchmark.error.empty())
    {
        options.error = options.benchmark.error;
        LocalFree(arguments);
        return options;
    }
    const auto chapterShipping=horde::platform::windows::ParseWindowsChapterShippingLaunch(argumentViews,
#if defined(_DEBUG)
        true
#else
        false
#endif
        ,HORDE_RT_CHAPTER_MEASUREMENT != 0
    );
    if(!chapterShipping.error.empty()) {
        options.error=chapterShipping.error;LocalFree(arguments);return options;
    }
    options.measureChapterShippingRoute=chapterShipping.requested;
    options.measureChapterForeground=chapterShipping.foregroundRequested;
    for (int index = 1; index < argumentCount; ++index)
    {
        const std::wstring_view argument(arguments[index]);
        if (argument == L"--validate-native-motion" || argument == L"--motion-scenario" ||
            argument == L"--motion-rt-workload") { ++index; continue; }
        if (argument == L"--validate-output-resize") { ++index; continue; }
        if (argument == L"--capture-graphics-preview") { ++index; continue; }
        if (argument == L"--capture-hidden-preview-pair") { index+=2; continue; }
        if (argument == L"--forest-route-proof") { ++index; continue; }
        if (argument == L"--development-vertical-proof")
        {
#if !defined(_DEBUG)
            options.error = "--development-vertical-proof is Debug-only.";
#endif
            continue;
        }
        if (argument == L"--validate-static-zone-cutover" || argument == L"--validate-static-triangle-cutover")
        {
#if defined(_DEBUG)
            options.validateStaticZoneCutover = true;
            options.validateStaticTriangleCutover = argument == L"--validate-static-triangle-cutover";
#else
            options.error = "--validate-static-zone-cutover is Debug-only.";
#endif
            continue;
        }
        if(argument==L"--validate-static-sparse-cutover") {
#if defined(_DEBUG)
            if(index+1>=argumentCount) {options.error="Sparse cutover requires one quoted list of triangle IDs.";break;}
            std::wistringstream ids(arguments[++index]);std::uint64_t id=0u;
            while(ids>>id) {
                if(id>UINT32_MAX){options.error="Sparse triangle ID exceeds uint32.";break;}
                options.staticTriangleOmissions.push_back(static_cast<std::uint32_t>(id));
            }
            if(!options.error.empty())break;
            if(!ids.eof() || options.staticTriangleOmissions.empty()) {
                options.error="Sparse cutover requires whitespace-separated unsigned triangle IDs.";break;
            }
            options.validateStaticZoneCutover=true;
#else
            options.error="--validate-static-sparse-cutover is Debug-only.";
#endif
            continue;
        }
        if(argument==L"--forest-content-candidate") {
#if defined(_DEBUG)
            if(!options.forestContentCandidate.empty() || index+1>=argumentCount) {
                options.error="Forest content candidate requires one frozen name, once.";break;
            }
            const auto name=std::filesystem::path(arguments[++index]).u8string();
            options.forestContentCandidate.assign(name.begin(),name.end());
            if(!horde::platform::windows::IsKnownForestContentCandidate(options.forestContentCandidate)) {
                options.error="Unknown frozen forest content candidate.";break;
            }
#else
            options.error="Forest content candidates are Debug-only.";break;
#endif
            continue;
        }
        if(argument==L"--validate-forest-content-candidate") {
#if defined(_DEBUG)
            if(options.validateForestContentCandidate) {options.error="Duplicate forest content validation.";break;}
            options.validateForestContentCandidate=true;
#else
            options.error="Forest content validation is Debug-only.";break;
#endif
            continue;
        }
        if(argument==L"--inspect-static-world-source") {
#if defined(_DEBUG)
            if(!options.inspectStaticWorldMaterial.empty() || index+1>=argumentCount ||
               arguments[index+1][0]==L'-' || arguments[index+1][0]==L'\0') {
                options.error="--inspect-static-world-source requires one material name, once.";break;
            }
            const auto name=std::filesystem::path(arguments[++index]).u8string();
            options.inspectStaticWorldMaterial.assign(name.begin(),name.end());
            if(options.inspectStaticWorldMaterial.size()>128u ||
               std::any_of(options.inspectStaticWorldMaterial.begin(),options.inspectStaticWorldMaterial.end(),
                   [](unsigned char c){return c<0x20u || c==0x7fu;})) {
                options.error="Static world inspection material name is invalid or too long.";break;
            }
#else
            options.error="--inspect-static-world-source is Debug-only.";break;
#endif
            continue;
        }
        if(argument==L"--capture-chapter-landing") {++index;continue;} // Validated by the bounded launch parser above.
        if(argument==L"--capture-chapter-route-stages")continue; // Same full-route gate, optional observation only.
        if(argument==L"--validate-chapter-roundtrip") {
#if defined(_DEBUG)
            options.validateChapterRoundTrip=true;
#else
            options.error="--validate-chapter-roundtrip is Debug-only.";
#endif
            continue;
        }
        if (argument == L"--anatomical-player-mount")
        {
#if defined(_DEBUG)
            options.anatomicalPlayerMount = true;
#else
            options.error = "--anatomical-player-mount is an unaccepted Debug-only candidate.";
#endif
            continue;
        }
        if (argument == L"--require-rayquery-compute")
        {
            options.requireRayQueryCompute = true;
            continue;
        }
        if (argument == L"--capture-portrait")
        {
            if (options.portrait)
            {
                options.error = "--capture-portrait may only be specified once.";
                break;
            }
            options.portrait = true;
            continue;
        }
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
    if (options.error.empty() && options.portrait && !options.requested)
        options.error = "--capture-portrait requires a Debug capture mode.";
    if (options.error.empty() && options.anatomicalPlayerMount &&
        (!options.requested || !options.developmentCheckpoint.starts_with("player-viewmodel-")))
        options.error = "--anatomical-player-mount requires a modelled-viewmodel development capture.";
    if (options.error.empty() && !options.developmentCheckpoint.empty() &&
        horde::gameplay::FindDevelopmentCheckpoint(options.developmentCheckpoint) == nullptr)
        options.error = "Unknown development checkpoint: " + options.developmentCheckpoint;
    if(options.error.empty() && !options.inspectStaticWorldMaterial.empty()) {
        const auto* checkpoint=horde::gameplay::FindDevelopmentCheckpoint(options.developmentCheckpoint);
        if(!options.requested || !options.captureHidden || !checkpoint ||
           !horde::gameplay::DevelopmentCheckpointRequiresAuthoredChapter(*checkpoint) ||
           options.validateChapterRoundTrip)
            options.error="--inspect-static-world-source requires one hidden Debug authored-chapter showcase capture.";
    }
    if(options.error.empty() && (!options.forestContentCandidate.empty() || options.validateForestContentCandidate)) {
        const auto* checkpoint=horde::gameplay::FindDevelopmentCheckpoint(options.developmentCheckpoint);
        if(options.forestContentCandidate.empty() || !options.requested || !options.captureHidden || !checkpoint ||
           !horde::gameplay::DevelopmentCheckpointRequiresAuthoredChapter(*checkpoint) ||
           options.forestRouteProof!=horde::platform::windows::ForestRouteCaptureMode::Disabled ||
           options.environmentSurvey.requested || options.graphicsPreview || options.outputResizeValidation ||
           !options.nativeMotionScenario.empty() || options.benchmark.requested || options.validateStaticZoneCutover ||
           !options.inspectStaticWorldMaterial.empty() || options.chapterLandingOnly ||
           (options.validateForestContentCandidate && (options.developmentCheckpoint!="chapter-forest-retention-west" ||
               options.validateChapterRoundTrip)))
            options.error="Frozen forest content requires an isolated hidden authored-chapter capture; preflight starts at retention-west.";
    }
    if(options.error.empty() && options.forestRouteProof!=horde::platform::windows::ForestRouteCaptureMode::Disabled) {
        const auto* checkpoint=horde::gameplay::FindDevelopmentCheckpoint(options.developmentCheckpoint);
        const bool normalRoute=options.forestRouteProof==horde::platform::windows::ForestRouteCaptureMode::NormalShared64 &&
            options.developmentCheckpoint=="chapter-kit-reunion";
        if(!options.requested || !options.captureHidden || !checkpoint ||
           !horde::gameplay::DevelopmentCheckpointRequiresAuthoredChapter(*checkpoint) || options.environmentSurvey.requested ||
           options.validateStaticZoneCutover || (options.validateChapterRoundTrip && !normalRoute) || !options.inspectStaticWorldMaterial.empty())
            options.error="Forest route proof requires an unfiltered hidden authored-chapter capture.";
    }
    if(options.error.empty() && options.validateStaticTriangleCutover && !options.staticTriangleOmissions.empty())
        options.error="Sparse cutover cannot mix with whole-region filtering.";
    if (options.error.empty() && options.validateStaticZoneCutover &&
        (!options.requested || options.developmentCheckpoint.empty()))
        options.error = "--validate-static-zone-cutover requires one frozen development capture.";
    if(options.error.empty() && options.validateChapterRoundTrip &&
       (!options.requested || options.developmentCheckpoint!="chapter-kit-reunion" ||
        options.validateStaticZoneCutover || options.graphicsPreview || options.outputResizeValidation ||
        !options.nativeMotionScenario.empty()))
        options.error="--validate-chapter-roundtrip requires only the admitted chapter-kit-reunion capture.";
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

bool ValidateCapturedObjGeometry(const std::filesystem::path& path,
                                 horde::platform::windows::CapturedPlayerGeometryEvidence& evidence,
                                 std::string& diagnostic)
{
    evidence.allVertexPositionsFinite = false;
    evidence.vertexCount = 0u;
    evidence.faceCount = 0u;
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        diagnostic = "Captured player geometry could not be reopened.";
        return false;
    }

    std::string line;
    while (std::getline(input, line))
    {
        if (line.starts_with("v "))
        {
            std::istringstream values(line.substr(2u));
            values.imbue(std::locale::classic());
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            if (!(values >> x >> y >> z) || !std::isfinite(x) ||
                !std::isfinite(y) || !std::isfinite(z))
            {
                diagnostic = "Captured player geometry has a malformed or non-finite vertex.";
                return false;
            }
            ++evidence.vertexCount;
        }
        else if (line.starts_with("f "))
        {
            std::istringstream indices(line.substr(2u));
            std::string index;
            std::size_t corners = 0u;
            while (indices >> index) ++corners;
            if (corners < 3u)
            {
                diagnostic = "Captured player geometry has a malformed face.";
                return false;
            }
            ++evidence.faceCount;
        }
    }
    if (input.bad() || evidence.vertexCount < 3u || evidence.faceCount == 0u)
    {
        diagnostic = "Captured player geometry is empty or incomplete.";
        return false;
    }
    evidence.allVertexPositionsFinite = true;
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
#if defined(HORDE_RT_SOURCE_DIR)
    const std::filesystem::path source=HORDE_RT_SOURCE_DIR;
#else
    const std::filesystem::path source;
#endif
    // Only the separately compiled, exact development measurement may use the
    // checkout in optimized builds. Ordinary Release still proves its package.
    return horde::platform::windows::ResolveWindowsAssetRoot(packaged,std::filesystem::exists(packaged),source,
#if defined(_DEBUG)
        true,
#else
        false,
#endif
        HORDE_RT_CHAPTER_MEASUREMENT && ParseCaptureLaunchOptions().measureChapterShippingRoute);
}

std::filesystem::path SettingsPath()
{
    if(gAcceptanceProfile.Active())return gAcceptanceProfile.SettingsPath();
    return ExecutableDirectory() / "HordeLanternRT.settings.ini";
}

void LoadSettings(VulkanSurfaceContext& context)
{
    const std::string path = SettingsPath().string();
    // Progress is deliberately loaded independently from ordinary display/audio settings.
    context.rtLabUnlocked = GetPrivateProfileIntA("progress", "rtLabUnlocked", 0, path.c_str()) != 0;
    const bool legacySfxEnabled = GetPrivateProfileIntA("audio", "sfx", 1, path.c_str()) != 0;
    const int configuredSfxVolume = static_cast<int>(
        GetPrivateProfileIntA("audio", "sfxVolume",
                              horde::audio::kSfxVolumeSettingMissing, path.c_str()));
    context.sfxVolumePercent = horde::audio::ResolveSfxVolumePercent(
        configuredSfxVolume, legacySfxEnabled);
    context.musicVolumePercent = std::clamp(
        static_cast<int>(GetPrivateProfileIntA("audio", "musicVolume", 70, path.c_str())), 0, 100);
    context.voiceVolumePercent = std::clamp(
        static_cast<int>(GetPrivateProfileIntA("audio", "voiceVolume", 70, path.c_str())), 0, 100);
    context.subtitlesEnabled = GetPrivateProfileIntA("subtitles", "enabled", 1, path.c_str()) != 0;
    context.subtitleSizePercent = std::clamp(
        static_cast<int>(GetPrivateProfileIntA("subtitles", "size", 100, path.c_str())), 80, 160);
    context.subtitlePosition = static_cast<horde::gameplay::dialogue::SubtitlePosition>(
        std::clamp(static_cast<int>(GetPrivateProfileIntA("subtitles", "position", 0, path.c_str())), 0, 2));
    const int sensitivity = std::clamp(static_cast<int>(GetPrivateProfileIntA("controls", "lookSensitivity", 100, path.c_str())), 60, 150);
    context.mouseSensitivity = static_cast<float>(sensitivity) / 100.0f;
    context.combatTeachingEnabled = GetPrivateProfileIntA("controls", "combatTeaching", 1, path.c_str()) != 0;
    context.combatTeachingSlowdown = GetPrivateProfileIntA("controls", "combatTeachingSlowdown", 0, path.c_str()) != 0;
    const int renderScale = horde::graphics::ClampGraphicsRenderScalePercent(
        static_cast<int>(GetPrivateProfileIntA("display", "renderScale", 100, path.c_str())));
    context.renderScale = static_cast<float>(renderScale) / 100.0f;
    const int waterQuality = std::clamp(static_cast<int>(GetPrivateProfileIntA("display", "waterQuality", 2, path.c_str())), 0, 2);
    context.waterQuality = static_cast<horde::vulkan::raytracing::WaterQuality>(waterQuality);
    context.savedGraphics = horde::graphics::MigrateLegacyGraphicsSettings(
        {renderScale, waterQuality}, horde::graphics::GraphicsPlatform::Windows);
    if (const auto record = horde::platform::windows::LoadGraphicsPersistenceRecord(path, context.savedGraphics))
    {
        const auto recovered = horde::graphics::RecoverGraphicsSettings(*record, horde::graphics::GraphicsPlatform::Windows);
        context.savedGraphics = recovered.startup;
        if (recovered.reasons != horde::graphics::GraphicsReason::None)
            context.graphicsStatus = "Recovered last confirmed graphics settings after an interrupted or invalid selection.";
    }
    context.renderScale = context.savedGraphics.renderScalePercent / 100.0f;
    context.waterQuality = static_cast<horde::vulkan::raytracing::WaterQuality>(context.savedGraphics.waterQuality);
    context.fireDetail = context.savedGraphics.fireDetail;
    context.shadowQuality = context.savedGraphics.shadowQuality;
    context.graphicsPreviewFrameCap = context.savedGraphics.previewFrameCap;
    context.requestedGlassEnabled = context.savedGraphics.glassEnabled;
    context.requestedMistEnabled = context.savedGraphics.mistEnabled;
    context.requestedDustQuality = context.savedGraphics.dustQuality;
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
    if (context.benchmarkProfileRequested) return;
    if (context.graphicsPreviewCapture || context.outputResizeValidation || context.nativeMotionValidation || context.environmentSurveyActive) return;
    const std::string path = SettingsPath().string();
    WritePrivateProfileStringA("progress", "rtLabUnlocked",
                               context.rtLabUnlocked ? "1" : "0", path.c_str());
}

void SaveSettings(const VulkanSurfaceContext& context)
{
    if (context.benchmarkProfileRequested) return;
    if(gCampaignHiddenValidation)return;
    if (context.graphicsPreviewCapture || context.outputResizeValidation || context.nativeMotionValidation) return;
    const std::string path = SettingsPath().string();
    const std::string sfxVolume = std::to_string(
        horde::audio::ClampSfxVolumePercent(context.sfxVolumePercent));
    WritePrivateProfileStringA("audio", "sfxVolume", sfxVolume.c_str(), path.c_str());
    WritePrivateProfileStringA("audio", "sfx",
                               context.sfxVolumePercent > 0 ? "1" : "0", path.c_str());
    const std::string musicVolume = std::to_string(context.musicVolumePercent);
    WritePrivateProfileStringA("audio", "musicVolume", musicVolume.c_str(), path.c_str());
    const std::string voiceVolume = std::to_string(std::clamp(context.voiceVolumePercent, 0, 100));
    WritePrivateProfileStringA("audio", "voiceVolume", voiceVolume.c_str(), path.c_str());
    WritePrivateProfileStringA("subtitles", "enabled", context.subtitlesEnabled ? "1" : "0", path.c_str());
    const std::string subtitleSize = std::to_string(std::clamp(context.subtitleSizePercent, 80, 160));
    WritePrivateProfileStringA("subtitles", "size", subtitleSize.c_str(), path.c_str());
    const std::string subtitlePosition = std::to_string(static_cast<int>(context.subtitlePosition));
    WritePrivateProfileStringA("subtitles", "position", subtitlePosition.c_str(), path.c_str());
    const std::string sensitivity = std::to_string(static_cast<int>(std::round(context.mouseSensitivity * 100.0f)));
    WritePrivateProfileStringA("controls", "lookSensitivity", sensitivity.c_str(), path.c_str());
    WritePrivateProfileStringA("controls", "combatTeaching",
                               context.combatTeachingEnabled ? "1" : "0", path.c_str());
    WritePrivateProfileStringA("controls", "combatTeachingSlowdown",
                               context.combatTeachingSlowdown ? "1" : "0", path.c_str());
    // Ordinary Audio/Controls saves must never persist an unconfirmed Graphics candidate.
    const std::string renderScale = std::to_string(context.savedGraphics.renderScalePercent);
    WritePrivateProfileStringA("display", "renderScale", renderScale.c_str(), path.c_str());
    const std::string waterQuality = std::to_string(static_cast<int>(context.savedGraphics.waterQuality));
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

bool PlayXAudioFile(const std::filesystem::path& path,
                    float leftGain,
                    float rightGain,
                    int sfxVolumePercent);
void SetControlVisible(HWND window, int id, bool visible);
bool MeasurementPausedByUi(const VulkanSurfaceContext& context);
bool NativeUiUsesHighContrast();
void ReplaceFontProperty(HWND window, const char* propertyName, HFONT font);

void PlaySoundEffect(const VulkanSurfaceContext& context, const char* filename)
{
    if (gCampaignHiddenValidation || context.sfxVolumePercent <= 0)
    {
        return;
    }
    const std::filesystem::path path = ResolveAssetRoot() / "audio/filmcow" / filename;
    if (std::filesystem::exists(path))
    {
        (void)PlayXAudioFile(path, 1.0f, 1.0f, context.sfxVolumePercent);
    }
    else
    {
        LogWindowsAudio("missing centred SFX asset: " + path.string());
    }
}

void PlayAmbientSoundEffect(const VulkanSurfaceContext& context,
                            const char* filename,
                            const float cueGain = 1.0f)
{
    if (gCampaignHiddenValidation || context.sfxVolumePercent <= 0)
    {
        return;
    }
    const std::filesystem::path path = ResolveAssetRoot() / "audio/filmcow" / filename;
    if (std::filesystem::exists(path))
    {
        (void)PlayXAudioFile(path, cueGain, cueGain, context.sfxVolumePercent);
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
        const HRESULT unityResult = masteringVoice_->SetVolume(1.0f);
        if (FAILED(unityResult))
            LogWindowsAudio("mastering voice unity gain could not be confirmed, HRESULT=" +
                            std::to_string(static_cast<long>(unityResult)));
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
                if (it->dialogueLine != 0u)
                {
                    completedDialogueLine_ = it->dialogueLine;
                    completedDialogueGeneration_ = it->dialogueGeneration;
                }
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

    std::pair<std::uint32_t, std::uint64_t> TakeDialogueCompletion()
    {
        const auto completion = std::pair{completedDialogueLine_, completedDialogueGeneration_};
        completedDialogueLine_ = 0u;
        completedDialogueGeneration_ = 0u;
        return completion;
    }

    bool StartOrUpdateLoop(const std::string_view key,
                           const std::filesystem::path& path,
                           float leftGain,
                           float rightGain,
                           const float sourceVolume = 1.0f)
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
                const bool matrixOk = SetVoiceMatrix(active.voice, leftGain, rightGain, path);
                const HRESULT volumeResult = active.voice->SetVolume(std::clamp(sourceVolume, 0.0f, 1.0f));
                return matrixOk && SUCCEEDED(volumeResult);
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
        const HRESULT volumeResult = voice->SetVolume(std::clamp(sourceVolume, 0.0f, 1.0f));
        if (FAILED(volumeResult))
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
        activeVoices_.push_back({voice, wave, path.filename().string(), std::string(key), 0u, 0u});
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

    void PauseDialogue(const bool paused)
    {
        for (auto& active : activeVoices_)
            if (active.dialogueLine != 0u)
            {
                if (paused) (void)active.voice->Stop();
                else (void)active.voice->Start();
            }
    }

    void UpdateDialogueSpatial(const std::uint32_t line, const std::uint64_t generation,
                               const float leftGain, const float rightGain)
    {
        if (line == 0u || generation == 0u) return;
        for (auto& active : activeVoices_)
            if (active.loopKey == "chapter_dialogue" && active.dialogueLine == line &&
                active.dialogueGeneration == generation)
            {
                // Follow the immutable active-line anchors without replacing the
                // queued PCM, cursor, completion identity or independent speech volume.
                (void)SetVoiceMatrix(active.voice, leftGain, rightGain, active.filename);
                return;
            }
    }

    void UpdateOwnedGain(const std::string_view key, const float gain)
    {
        for (auto &active : activeVoices_)
            if (active.loopKey == key) SetVoiceMatrix(active.voice, gain, gain, active.filename);
    }

    bool PreloadMonoPcm48k16(const std::filesystem::path& path) {
        const auto wave=Load(path);
        return wave && wave->format.nChannels==1u && wave->format.nSamplesPerSec==48000u && wave->format.wBitsPerSample==16u;
    }
    void SetOwnedVolume(const std::string_view key,float gain) {
        for(auto& voice:activeVoices_)if(voice.loopKey==key)
            (void)voice.voice->SetVolume(std::isfinite(gain)?std::clamp(gain,0.f,1.f):0.f);
    }

    void SetDialogueVolume(const int percent)
    {
        const float gain = horde::platform::windows::WindowsDialogueSourceGain(percent);
        for (auto& active : activeVoices_)
            if (active.dialogueLine != 0u) (void)active.voice->SetVolume(gain);
    }

    void SetSfxVolume(const int percent)
    {
        const float gain = horde::audio::SfxVolumeLinearGain(percent);
        for (auto& active : activeVoices_)
            if (active.dialogueLine == 0u) (void)active.voice->SetVolume(gain);
    }

    bool Play(const std::filesystem::path& path, float leftGain, float rightGain,
              const std::string_view ownedKey = {},
              const std::uint32_t dialogueLine = 0u,
              const std::uint64_t dialogueGeneration = 0u,
              const float sourceVolume = 1.0f)
    {
        if (engine_ == nullptr || masteringVoice_ == nullptr)
        {
            LogFailureOnce("XAudio2 unavailable; SFX was not played: " + path.string());
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
        const HRESULT volumeResult = voice->SetVolume(std::clamp(sourceVolume, 0.0f, 1.0f));
        if (FAILED(volumeResult))
        {
            LogFailureOnce("source voice SetVolume failed, HRESULT=" +
                           std::to_string(static_cast<long>(volumeResult)) + ": " + path.string());
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
        activeVoices_.push_back({voice, wave, filename, std::string(ownedKey),
                                 dialogueLine, dialogueGeneration});
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
        std::uint32_t dialogueLine = 0u;
        std::uint64_t dialogueGeneration = 0u;
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
    std::uint32_t completedDialogueLine_ = 0u;
    std::uint64_t completedDialogueGeneration_ = 0u;
};

PositionalAudioEngine& SpatialAudioEngine()
{
    static PositionalAudioEngine engine;
    return engine;
}

bool PlayXAudioFile(const std::filesystem::path& path,
                    float leftGain,
                    float rightGain,
                    const int sfxVolumePercent)
{
    PositionalAudioEngine& engine = SpatialAudioEngine();
    return engine.Play(path, std::clamp(leftGain, 0.0f, 1.0f),
                       std::clamp(rightGain, 0.0f, 1.0f), {}, 0u, 0u,
                       horde::platform::windows::WindowsSfxSourceGain(1.0f, sfxVolumePercent));
}

#include "windows_private_lich_revival_audio.inl"

void UpdateChapterDialogueAudio(VulkanSurfaceContext& context)
{
    if(gCampaignHiddenValidation)return;
    UpdatePrivateLichRevivalAudio(context);
    if((!horde::gameplay::simulation::ForestBirdAmbience::Eligible(context.simulation.Snapshot()) &&
        !horde::gameplay::simulation::ForestBirdAmbience::PondEligible(context.simulation.Snapshot()) &&
        !horde::gameplay::simulation::ChapterLocalAmbience::Eligible(context.simulation.Snapshot())) ||
       !context.controlsEnabled || MeasurementPausedByUi(context) || context.sfxVolumePercent<=0 ||
       GetForegroundWindow()!=context.windowHandle)SpatialAudioEngine().StopLoop("chapter_ambience");
    using horde::gameplay::dialogue::Line;
    const auto& simulation = context.simulation.Snapshot();
    const auto& dialogue = simulation.chapterDialogue;
    const auto line = dialogue.line;
    const auto lineId = static_cast<std::uint32_t>(line);
    if (line == Line::None)
    {
        if (context.playingDialogueLine != 0u)
            SpatialAudioEngine().StopLoop("chapter_dialogue");
        context.playingDialogueLine = 0u;
        context.playingDialogueGeneration = 0u;
        return;
    }
    const auto gains = horde::gameplay::CalculateSpatialAudio(
        {dialogue.source.x, dialogue.source.z, .36f, 1.0f, 14.0f, dialogue.source.y},
        {dialogue.listener.x, dialogue.listener.z, dialogue.listenerYaw, dialogue.listener.y},
        horde::gameplay::ResolveSpatialAudioDomain(
            simulation.developmentRescueJourney, simulation.worldRoute.current));
    const bool sameVoice = context.playingDialogueLine == lineId &&
        context.playingDialogueGeneration == dialogue.generation;
    if (sameVoice)
        SpatialAudioEngine().UpdateDialogueSpatial(lineId, dialogue.generation, gains.left, gains.right);
    const bool paused = dialogue.paused || !context.controlsEnabled ||
        GetForegroundWindow() != context.windowHandle;
    SpatialAudioEngine().PauseDialogue(paused);
    if (paused) return;
    if (sameVoice) return; // A missing/completed voice must not restart this line.
    SpatialAudioEngine().StopLoop("chapter_dialogue");
    context.playingDialogueLine = 0u;
    context.playingDialogueGeneration = 0u;
    const auto& spec = horde::gameplay::dialogue::Spec(line);
    if (spec.audio == nullptr || spec.audio[0] == '\0') return;
    const auto path = horde::platform::windows::WindowsDialogueAudioPath(
        ResolveAssetRoot(), spec.audio);
    if (!std::filesystem::exists(path))
    {
        LogWindowsAudio("chapter line audio unavailable; subtitle remains active: " + path.string());
        return;
    }
    if (gains.left <= 0.0f && gains.right <= 0.0f) return;
    const float voiceGain = horde::platform::windows::WindowsDialogueSourceGain(context.voiceVolumePercent);
    const bool accepted = SpatialAudioEngine().Play(path, gains.left, gains.right,
            "chapter_dialogue", lineId, dialogue.generation,
            voiceGain);
    if (context.loggedDialogueAttemptGeneration != dialogue.generation ||
        context.loggedDialogueAttemptAccepted != accepted)
    {
        // One failed attempt plus its eventual admission per generation, not
        // one log row per retry frame. Admission is not proof of audibility.
        LogWindowsAudio("chapter voice admission: line=" + std::to_string(lineId) +
            " generation=" + std::to_string(dialogue.generation) + " accepted=" + (accepted ? "1" : "0") +
            " left=" + std::to_string(gains.left) + " right=" + std::to_string(gains.right) +
            " voiceGain=" + std::to_string(voiceGain));
        context.loggedDialogueAttemptGeneration = dialogue.generation;
        context.loggedDialogueAttemptAccepted = accepted;
    }
    if (accepted)
    {
        context.playingDialogueLine = lineId;
        context.playingDialogueGeneration = dialogue.generation;
    }
}

void RefreshChapterSubtitle(VulkanSurfaceContext& context)
{
    HWND subtitle = GetDlgItem(context.windowHandle, kChapterSubtitleId);
    if (subtitle == nullptr) return;
    const auto& dialogue = context.simulation.Snapshot().chapterDialogue;
    if (!context.subtitlesEnabled || dialogue.line == horde::gameplay::dialogue::Line::None ||
        dialogue.paused || !context.controlsEnabled || MeasurementPausedByUi(context))
    {
        SetControlVisible(context.windowHandle, kChapterSubtitleId, false);
        return;
    }
    const auto& spec = horde::gameplay::dialogue::Spec(dialogue.line);
    if (context.presentedDialogueLine != dialogue.line ||
        context.presentedDialogueGeneration != dialogue.generation)
    {
        context.presentedDialogueLine = dialogue.line;
        context.presentedDialogueGeneration = dialogue.generation;
        context.subtitlePlacementLatch.active = false;
    }
    RECT client{};
    GetClientRect(context.windowHandle, &client);
    const int width = std::max(0, static_cast<int>(client.right - client.left));
    const int height = std::max(0, static_cast<int>(client.bottom - client.top));
    const int fontPixels = ScaleForDpi(context.windowHandle,
        std::max(16, 22 * context.subtitleSizePercent / 100));
    if (fontPixels != context.appliedSubtitleFontPixels)
    {
        HFONT font = CreateFontA(-fontPixels, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_ROMAN, "Georgia");
        if (font)
        {
            SendMessageA(subtitle, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            ReplaceFontProperty(context.windowHandle, kSubtitleFontProperty, font);
            context.appliedSubtitleFontPixels = fontPixels;
        }
    }
    std::string text = horde::platform::windows::WindowsSubtitleCaption(spec.speaker, spec.text);
    const int subtitleWidth = std::max(0, std::min(900,
        width - 2 * ScaleForDpi(context.windowHandle, 24)));
    HDC measureDc = GetDC(context.windowHandle);
    RECT measured{0, 0, std::max(0, subtitleWidth - 24), 0};
    const HFONT measureFont = reinterpret_cast<HFONT>(SendMessageA(subtitle, WM_GETFONT, 0, 0));
    const HGDIOBJ oldMeasureFont = measureDc && measureFont ? SelectObject(measureDc, measureFont) : nullptr;
    if (measureDc) DrawTextA(measureDc, text.c_str(), -1, &measured,
        DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
    if (measureDc && oldMeasureFont && oldMeasureFont != HGDI_ERROR)
        SelectObject(measureDc, oldMeasureFont);
    if (measureDc) ReleaseDC(context.windowHandle, measureDc);
    const int measuredLineHeight = std::max(1, fontPixels + ScaleForDpi(context.windowHandle, 6));
    const int wrappedLines = std::max(1, static_cast<int>((measured.bottom + measuredLineHeight - 1) / measuredLineHeight));
    const int topReserved = ScaleForDpi(context.windowHandle, 164);
    const HWND interaction = GetDlgItem(context.windowHandle, kChestPromptControlId);
    const int bottomReserved = ScaleForDpi(context.windowHandle,
        interaction && IsWindowVisible(interaction) ? 100 : 20);
    horde::gameplay::dialogue::SubtitleRequest request;
    request.width = width;
    request.height = height;
    request.margin = ScaleForDpi(context.windowHandle, 12);
    request.topReserved = topReserved;
    request.bottomReserved = bottomReserved;
    request.fontPixels = fontPixels;
    request.lines = wrappedLines;
    request.position = context.subtitlePosition;
    request.touch = false;
    if ((!context.subtitlePlacementLatch.active ||
         context.subtitlePlacementLatch.preference != context.subtitlePosition) && request.position ==
        horde::gameplay::dialogue::SubtitlePosition::Auto)
    {
        auto topRequest = request;
        topRequest.position = horde::gameplay::dialogue::SubtitlePosition::Top;
        auto bottomRequest = request;
        bottomRequest.position = horde::gameplay::dialogue::SubtitlePosition::Bottom;
        const auto top = horde::gameplay::dialogue::LayoutSubtitle(topRequest);
        const auto bottom = horde::gameplay::dialogue::LayoutSubtitle(bottomRequest);
        const auto collisionCount = [&](const horde::gameplay::dialogue::SubtitleLayout& candidate)
        {
            const RECT subtitleRect{candidate.x, candidate.y,
                candidate.x + candidate.width, candidate.y + candidate.height};
            int collisions = 0;
            for (const int controlId : {kHudControlId, kVitalityHudControlId,
                                        kCombatTeachingPromptId, kChestPromptControlId})
            {
                HWND control = GetDlgItem(context.windowHandle, controlId);
                if (control == nullptr || !IsWindowVisible(control)) continue;
                RECT bounds{};
                GetWindowRect(control, &bounds);
                MapWindowPoints(HWND_DESKTOP, context.windowHandle,
                    reinterpret_cast<POINT*>(&bounds), 2);
                RECT overlap{};
                if (IntersectRect(&overlap, &subtitleRect, &bounds)) ++collisions;
            }
            return collisions;
        };
        if ((!bottom.fits && top.fits) ||
            (collisionCount(bottom) > collisionCount(top)))
            request.position = horde::gameplay::dialogue::SubtitlePosition::Top;
        else
            request.position = horde::gameplay::dialogue::SubtitlePosition::Bottom;
    }
    auto layout = horde::gameplay::dialogue::LayoutSubtitle(request);
    layout = horde::platform::windows::ResolveWindowsSubtitleLayout(
        request, dialogue.generation, context.subtitlePlacementLatch, context.subtitlePosition);
    if (!layout.fits)
    {
        // Keep the requested accessible size; report insufficient room rather
        // than swapping large/small fonts on each publication.
        text = "Enlarge window for subtitles";
        HDC dc = GetDC(context.windowHandle);
        RECT bounds{0, 0, std::max(0, subtitleWidth - 24), 0};
        const HFONT font = reinterpret_cast<HFONT>(SendMessageA(subtitle, WM_GETFONT, 0, 0));
        const HGDIOBJ old = dc && font ? SelectObject(dc, font) : nullptr;
        if (dc) DrawTextA(dc, text.c_str(), -1, &bounds, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
        if (dc && old && old != HGDI_ERROR) SelectObject(dc, old);
        if (dc) ReleaseDC(context.windowHandle, dc);
        request.lines = std::max(1, static_cast<int>((bounds.bottom + measuredLineHeight - 1) / measuredLineHeight));
        layout = horde::platform::windows::ResolveWindowsSubtitleLayout(
            request, dialogue.generation, context.subtitlePlacementLatch, context.subtitlePosition);
        if (!layout.fits)
        {
            SetControlVisible(context.windowHandle, kChapterSubtitleId, false);
            return;
        }
    }
    const horde::platform::windows::SubtitlePaintKey paint{
        text, layout.width, layout.height, context.appliedSubtitleFontPixels, NativeUiUsesHighContrast()};
    RECT previous{};
    GetWindowRect(subtitle, &previous);
    MapWindowPoints(HWND_DESKTOP, context.windowHandle, reinterpret_cast<POINT*>(&previous), 2);
    if (previous.left != layout.x || previous.top != layout.y ||
        previous.right - previous.left != layout.width || previous.bottom - previous.top != layout.height)
        SetWindowPos(subtitle, HWND_TOP, layout.x, layout.y, layout.width, layout.height, SWP_NOACTIVATE);
    // Do not erase/repaint identical labels at render cadence: that flickers over
    // Vulkan's independently presented scene. The native alpha sprite owns paint.
    if (!context.subtitleBitmapCurrent || !(paint == context.subtitlePaintKey))
    {
        if (text != context.subtitlePaintKey.text) SetWindowTextA(subtitle, text.c_str());
        const HFONT font = reinterpret_cast<HFONT>(SendMessageA(subtitle, WM_GETFONT, 0, 0));
        context.subtitleBitmapCurrent = horde::platform::windows::PresentSubtitleBitmap(subtitle, paint, font);
        if (context.subtitleBitmapCurrent) context.subtitlePaintKey = paint;
    }
    SetControlVisible(context.windowHandle, kChapterSubtitleId, context.subtitleBitmapCurrent);
}

void UpdateWaterfallAmbience(const VulkanSurfaceContext& context)
{
    constexpr std::string_view loopKey = "waterfall";
    PositionalAudioEngine& engine = SpatialAudioEngine();
    if (context.sfxVolumePercent <= 0 || context.simulationPaused)
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
    const float sfxGain = horde::platform::windows::WindowsSfxSourceGain(horde::audio::DialogueBedGain(simulation), context.sfxVolumePercent);
    engine.StartOrUpdateLoop(loopKey, path, gains.left, gains.right, sfxGain);
}

void StopMenuAmbience(VulkanSurfaceContext& context)
{
    if(gCampaignHiddenValidation)return;
    auto &engine = SpatialAudioEngine();
    for (const auto key : {"menu_room", "menu_chain"}) engine.StopLoop(key);
    context.menuCreakDelivery.Suspend();
}

void UpdateMenuAmbience(VulkanSurfaceContext& context)
{
    if(gCampaignHiddenValidation)return;
    const auto menu = context.entryMenu.Snapshot();
    const bool audible = context.useRtPath && context.rtScene.IsReady() &&
        context.rtScene.Profile() == horde::vulkan::raytracing::RtSceneProfile::EntryMenu &&
        !context.graphicsPreviewCapture && !context.outputResizeValidation && !context.nativeMotionValidation &&
        GetForegroundWindow() == context.windowHandle && !IsIconic(context.windowHandle) &&
        context.sfxVolumePercent > 0 && menu.fade < 1.0f;
    if (!audible) { StopMenuAmbience(context); return; }
    auto &engine = SpatialAudioEngine();
    const float envelope = std::clamp(1.0f - menu.fade, 0.0f, 1.0f);
    const float sfxGain = horde::platform::windows::WindowsSfxSourceGain(1.0f, context.sfxVolumePercent);
    const auto root = ResolveAssetRoot() / "audio/menu";
    engine.StartOrUpdateLoop("menu_room", root / "menu_room.wav",
        horde::audio::kMenuRoomGain * envelope, horde::audio::kMenuRoomGain * envelope, sfxGain);
    // Existing keyed native voice ownership handles the one-shot too: mute,
    // focus loss and Play can cancel it independently of gameplay SFX.
    if (context.menuCreakDelivery.Observe(menu.tick, menu.chainCreakSerial, true))
    {
        engine.StopLoop("menu_chain");
        engine.Play(root / "menu_chain.wav", horde::audio::kMenuChainGain * envelope,
                    horde::audio::kMenuChainGain * envelope, "menu_chain", 0u, 0u, sfxGain);
    }
    else
        engine.UpdateOwnedGain("menu_chain", horde::audio::kMenuChainGain * envelope);
}

void PlayPositionalSoundEffect(const VulkanSurfaceContext& context,
                               const char* filename,
                               float mixGain,
                               const horde::gameplay::simulation::GameplayEvent& event,
                               const char* collection = "filmcow")
{
    if (gCampaignHiddenValidation || context.sfxVolumePercent <= 0)
    {
        return;
    }
    const horde::gameplay::SpatialAudioGains gains = horde::gameplay::CalculateSpatialAudio(
        {event.worldX, event.worldZ, mixGain, 1.0f, 14.0f, event.worldY},
        {event.listenerX, event.listenerZ, event.listenerYawRadians, event.listenerY});
    if (gains.left <= 0.0f && gains.right <= 0.0f)
    {
        return;
    }
    const std::filesystem::path path = ResolveAssetRoot() / "audio" / collection / filename;
    if (std::filesystem::exists(path))
    {
        (void)PlayXAudioFile(path, gains.left, gains.right, context.sfxVolumePercent);
    }
    else
    {
        LogWindowsAudio("missing positional SFX asset: " + path.string());
    }
}

void PublishMusicPlayback(VulkanSurfaceContext& context, const bool focusLossNotification = false)
{
    if(gCampaignHiddenValidation)return;
    if (context.musicPlayback)
    {
        // Copy before the existing SFX drain. No worker accesses GameSimulation.
        const auto& snapshot = context.simulation.Snapshot();
        const bool gameForeground = GetForegroundWindow() == context.windowHandle;
        const bool suspended = horde::platform::windows::WindowsMusicShouldSuspend(
            gameForeground, context.controlsEnabled, context.simulationPaused, focusLossNotification);
        const bool accepted = context.musicPlayback->Publish(snapshot,
            context.simulation.Events().Events(), context.musicEventLifetimeToken, context.musicResetToken,
            suspended,
            context.musicVolumePercent);
        // Bounded transition observations distinguish an explicit loss from a
        // settled foreground handover. No foreign-window identity or frame log.
        const bool active = gameForeground && !focusLossNotification;
        const int gate = (active ? 1 : 0) |
                         (context.controlsEnabled ? 2 : 0) |
                         (context.simulationPaused ? 4 : 0) |
                         (focusLossNotification ? 8 : 0) |
                         (gameForeground ? 16 : 0);
        if (gate != context.musicLastLoggedGate)
        {
            context.musicLastLoggedGate = gate;
            LogWindowsAudio("Windows music gate: active=" + std::to_string(active) +
                " foreground=" + std::to_string(gameForeground) +
                " focusLoss=" + std::to_string(focusLossNotification) +
                " controls=" + std::to_string(context.controlsEnabled) +
                " menuPaused=" + std::to_string(context.simulationPaused) +
                " snapshotPaused=" + std::to_string(snapshot.paused) +
                " accepted=" + std::to_string(accepted) +
                " volume=" + std::to_string(context.musicVolumePercent) +
                " tick=" + std::to_string(snapshot.tickIndex) +
                " wallMs=" + std::to_string(GetTickCount64()));
        }
    }
}

void DrainGameplayEvents(VulkanSurfaceContext& context)
{
    if(gCampaignHiddenValidation){context.simulation.ClearEvents();return;}
    using horde::gameplay::simulation::EntityId;
    using horde::gameplay::simulation::GameplayEvent;
    using horde::gameplay::simulation::GameplayEventType;

    context.delayedFeedback.DrainDue(GetTickCount64(), [&context](const GameplayEvent& event)
    {
        PlayPositionalSoundEffect(context, "skeleton_falling_bones.wav", 0.36f, event, "pixabay");
    });

    std::uint64_t latestAmbienceSignal=0;
    for(const auto& event:context.simulation.Events().Events())
        if(horde::gameplay::simulation::IsChapterAmbienceEvent(event.type))latestAmbienceSignal=event.sequence;
    for (const GameplayEvent& event : context.simulation.Events().Events())
    {
        switch (event.type)
        {
        case GameplayEventType::ForestBird:
        case GameplayEventType::ForestCrickets:
        case GameplayEventType::PondFrogs:
        case GameplayEventType::TombDrip:
        case GameplayEventType::RopeContact:
        case GameplayEventType::ForestFoliage:
            if(event.sequence!=latestAmbienceSignal)break;
            if(event.type==GameplayEventType::ForestFoliage &&
               !horde::gameplay::simulation::ForestBirdAmbience::FoliageEligible(context.simulation.Snapshot())) {
                SpatialAudioEngine().StopLoop("chapter_ambience");break;
            }
            if(context.controlsEnabled && !context.simulationPaused && context.sfxVolumePercent>0 &&
               GetForegroundWindow()==context.windowHandle &&
               (event.type==GameplayEventType::ForestFoliage?
                   horde::gameplay::simulation::ForestBirdAmbience::FoliageEligible(context.simulation.Snapshot()):
                event.type==GameplayEventType::TombDrip?
                   horde::gameplay::simulation::ChapterLocalAmbience::DripEligible(context.simulation.Snapshot()):
                event.type==GameplayEventType::RopeContact?
                   horde::gameplay::simulation::ChapterLocalAmbience::RopeEligible(context.simulation.Snapshot()):
                event.type==GameplayEventType::PondFrogs?
                   horde::gameplay::simulation::ForestBirdAmbience::PondEligible(context.simulation.Snapshot()):
                   horde::gameplay::simulation::ForestBirdAmbience::Eligible(context.simulation.Snapshot()))) {
                auto& audio=SpatialAudioEngine();audio.StopLoop("chapter_ambience");
                const auto gains=horde::gameplay::CalculateSpatialAudio(
                    horde::gameplay::simulation::ChapterAmbienceEmitter(event),
                    {event.listenerX,event.listenerZ,event.listenerYawRadians,event.listenerY});
                const auto clip=event.type==GameplayEventType::ForestFoliage?
                    "audio/forest-foliage/runtime/leaf_rustle.wav":event.type==GameplayEventType::TombDrip?
                    "audio/chapter-local-ambience/runtime/tomb_drip.wav":event.type==GameplayEventType::RopeContact?
                    "audio/chapter-local-ambience/runtime/rope_contact.wav":event.type==GameplayEventType::PondFrogs?
                    "audio/night-ambience/runtime/pond_frogs_crickets.wav":event.type==GameplayEventType::ForestCrickets?
                    "audio/night-ambience/runtime/evening_cricket.wav":"audio/night-ambience/runtime/tawny_owl.wav";
                (void)audio.Play(ResolveAssetRoot()/clip,gains.left,gains.right,
                    "chapter_ambience",0,0,horde::platform::windows::WindowsSfxSourceGain(1.f,context.sfxVolumePercent));
            }
            break;
        case GameplayEventType::ForestBirdStopped:
        case GameplayEventType::ChapterLocalAmbienceStopped:
            if(event.sequence!=latestAmbienceSignal)break;
            SpatialAudioEngine().StopLoop("chapter_ambience");break;
        case GameplayEventType::PlayerWetFootstep:
        {
            const unsigned variant = static_cast<unsigned>(context.playerFootstepVariant++) & 3u;
            const char* clip = variant == 0u ? "water_wet_step_1.wav" :
                               variant == 1u ? "water_wet_step_2.wav" :
                               variant == 2u ? "water_wet_step_3.wav" : "water_wet_step_4.wav";
            PlayPositionalSoundEffect(context, clip,
                event.intensity * horde::audio::kWetFootstepCueGain, event, "pixabay");
            break;
        }
        case GameplayEventType::WaterfallContact:
            PlayPositionalSoundEffect(context, "water_stream_contact.wav",
                event.intensity * 0.45f, event, "pixabay");
            break;
        case GameplayEventType::PlayerFootstep:
        {
            const char* clip = horde::gameplay::DryFootstepClip(
                horde::gameplay::DryFootstepCueForEvent(event));
            if (clip) PlayAmbientSoundEffect(context, clip, horde::audio::kPlayerFootstepCueGain);
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
        case GameplayEventType::ParryPrepareCue:
            PlayPositionalSoundEffect(context, "sword_hit_1.wav", 0.22f, event);
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
        case GameplayEventType::LichRevivalCry:
            PlayPrivateLichRevivalAudio(context,event);
            break;
        case GameplayEventType::LichChargeStarted:
            PlayPositionalSoundEffect(context, "lich_charge.wav", 0.42f, event);
            break;
        case GameplayEventType::LichImpact:
            PlayPositionalSoundEffect(context, "lich_impact.wav", 0.55f, event);
            break;
        case GameplayEventType::LichDischargeWarning:
            PlayPositionalSoundEffect(context, "skeleton_attack.wav", 0.20f, event);
            break;
        case GameplayEventType::LichDefeated:
            PlayPositionalSoundEffect(context, "keeper_death.wav", 0.36f, event, "pixabay");
            break;
        case GameplayEventType::KeeperRevealStarted:
        case GameplayEventType::KeeperWarning:
        case GameplayEventType::KeeperCombatReady:
            break;
        case GameplayEventType::SkeletonIncidental:
            PlayPositionalSoundEffect(context, "skeleton_idle_rattle.wav", 0.30f, event, "pixabay");
            break;
        case GameplayEventType::SkeletonEncounterWarning:
            if (!context.controlsEnabled || context.simulationPaused ||
                GetForegroundWindow() != context.windowHandle) break;
            PlayPositionalSoundEffect(context, "skeleton_idle_rattle.wav", 0.48f, event, "pixabay");
            break;
        case GameplayEventType::PlayerSwordAttachmentChanged:
            if (!context.controlsEnabled || context.simulationPaused ||
                GetForegroundWindow() != context.windowHandle) break;
            switch (horde::gameplay::EquipmentCueForEvent(event))
            {
            case horde::gameplay::EquipmentAudioCue::SwordDraw:
                PlayAmbientSoundEffect(context, "equipment/sword_draw.wav", 0.42f);
                break;
            case horde::gameplay::EquipmentAudioCue::SwordSheath:
                PlayAmbientSoundEffect(context, "equipment/sword_sheath.wav", 0.38f);
                break;
            default:
                break;
            }
            break;
        case GameplayEventType::ChestUnlocked:
            PlayPositionalSoundEffect(context, "chest_unlock.wav", 0.82f, event,
                                      "pixabay");
            break;
        case GameplayEventType::ChestOpened:
            PlayPositionalSoundEffect(context, "chest_open.wav", 1.0f, event,
                                      "pixabay");
            break;
        case GameplayEventType::TorchExtinguished:
            PlayPositionalSoundEffect(context, "torch_extinguish.wav", 0.78f,
                                      event, "pixabay");
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
        horde::platform::windows::SetNativeVisibilityIfChanged(control,visible);
    }
}

const char* GraphicsFireName(const horde::graphics::FireDetail detail)
{
    switch (detail)
    {
    case horde::graphics::FireDetail::Mobile: return "MOBILE";
    case horde::graphics::FireDetail::High: return "HIGH";
    case horde::graphics::FireDetail::Low: return "LOW";
    }
    return "UNAVAILABLE";
}

const char* GraphicsShadowName(const horde::graphics::ShadowQuality quality)
{
    switch (quality)
    {
    case horde::graphics::ShadowQuality::Current: return "CURRENT";
    case horde::graphics::ShadowQuality::Higher: return "HIGHER";
    }
    return "UNAVAILABLE";
}

horde::graphics::FireDetail UploadedGraphicsFireDetail(const VulkanSurfaceContext& context)
{
    if (context.rtScene.IsReady() && context.rtScene.HasUploadedQualityControls())
    {
        switch (context.rtScene.UploadedFireQuality())
        {
        case horde::vulkan::raytracing::FireEmitterQuality::Mobile: return horde::graphics::FireDetail::Mobile;
        case horde::vulkan::raytracing::FireEmitterQuality::High: return horde::graphics::FireDetail::High;
        case horde::vulkan::raytracing::FireEmitterQuality::Low: return horde::graphics::FireDetail::Low;
        }
    }
    return static_cast<horde::graphics::FireDetail>(255u);
}

horde::graphics::ShadowQuality UploadedGraphicsShadowQuality(const VulkanSurfaceContext& context)
{
    if (context.rtScene.IsReady() && context.rtScene.HasUploadedQualityControls() &&
        context.rtScene.QualityControls().controls[0] <= 2u)
        return static_cast<horde::graphics::ShadowQuality>(context.rtScene.QualityControls().controls[0]);
    return static_cast<horde::graphics::ShadowQuality>(255u);
}

const char* GraphicsDustName(const horde::graphics::DustQuality quality)
{
    switch (quality)
    {
    case horde::graphics::DustQuality::Off: return "OFF";
    case horde::graphics::DustQuality::Low: return "LOW";
    case horde::graphics::DustQuality::Standard: return "STANDARD";
    }
    return "UNAVAILABLE";
}

void UpdateSettingsLabels(VulkanSurfaceContext& context)
{
    const auto graphicsDraft = context.graphicsEdit ? context.graphicsEdit->Draft() : context.savedGraphics;
    if (HWND title = GetDlgItem(context.windowHandle, kSettingsTitleId))
        SetWindowTextA(title, context.settingsDialoguePage
            ? "SETTINGS  |  AUDIO & SUBTITLES" : "SETTINGS  |  SAVED BESIDE THE DEMO");
    if (HWND label = GetDlgItem(context.windowHandle, kSfxVolumeLabelId))
    {
        const std::string text = "SFX VOLUME: " + std::to_string(context.sfxVolumePercent) + "%";
        SetWindowTextA(label, text.c_str());
    }
    if (HWND slider = GetDlgItem(context.windowHandle, kSfxVolumeSliderId))
    {
        SendMessageA(slider, TBM_SETPOS, TRUE, static_cast<LPARAM>(context.sfxVolumePercent));
    }
    if (HWND sensitivity = GetDlgItem(context.windowHandle, kSensitivityButtonId))
    {
        const char* value = context.mouseSensitivity < 0.8f ? "LOW" : (context.mouseSensitivity > 1.2f ? "HIGH" : "NORMAL");
        const std::string label = std::string("LOOK SENSITIVITY: ") + value;
        SetWindowTextA(sensitivity, label.c_str());
    }
    if (HWND water = GetDlgItem(context.windowHandle, kWaterQualityButtonId))
    {
        const char* value = graphicsDraft.waterQuality == horde::graphics::WaterQuality::High ? "HIGH" :
                            (graphicsDraft.waterQuality == horde::graphics::WaterQuality::Mobile ? "MOBILE" : "OFF");
        const std::string label = std::string("RT WATER: ") + value;
        SetWindowTextA(water, label.c_str());
    }
    if (HWND fullscreen = GetDlgItem(context.windowHandle, kFullscreenButtonId))
    {
        SetWindowTextA(fullscreen, context.fullscreen ? "DISPLAY: FULLSCREEN" : "DISPLAY: WINDOWED");
    }
    if (HWND label = GetDlgItem(context.windowHandle, kRenderScaleLabelId))
    {
        const std::string text = "Resolution: " + std::to_string(graphicsDraft.renderScalePercent) + "%" +
            (horde::graphics::ExperimentalGraphicsRenderScalePercent(graphicsDraft.renderScalePercent) ? " (Experimental)" : "");
        SetWindowTextA(label, text.c_str());
    }
    if (HWND slider = GetDlgItem(context.windowHandle, kRenderScaleSliderId))
    {
        SendMessageA(slider, TBM_SETPOS, TRUE, static_cast<LPARAM>(
            horde::graphics::GraphicsRenderScaleSliderPositionFromPercent(graphicsDraft.renderScalePercent)));
    }
    if (HWND label = GetDlgItem(context.windowHandle, kMusicVolumeLabelId))
    {
        const std::string text = "MUSIC VOLUME: " +
                                 std::to_string(context.musicVolumePercent) + "%";
        SetWindowTextA(label, text.c_str());
    }
    if (HWND slider = GetDlgItem(context.windowHandle, kMusicVolumeSliderId))
    {
        SendMessageA(slider, TBM_SETPOS, TRUE, static_cast<LPARAM>(context.musicVolumePercent));
    }
    if (HWND label = GetDlgItem(context.windowHandle, kVoiceVolumeLabelId))
        SetWindowTextA(label, ("DIALOGUE / SPEECH: " + std::to_string(context.voiceVolumePercent) + "%").c_str());
    if (HWND slider = GetDlgItem(context.windowHandle, kVoiceVolumeSliderId))
        SendMessageA(slider, TBM_SETPOS, TRUE, context.voiceVolumePercent);
    if (HWND button = GetDlgItem(context.windowHandle, kSubtitlesButtonId))
        SetWindowTextA(button, context.subtitlesEnabled ? "SUBTITLES: ON" : "SUBTITLES: OFF");
    if (HWND label = GetDlgItem(context.windowHandle, kSubtitleSizeLabelId))
        SetWindowTextA(label, ("SUBTITLE TEXT SIZE: " + std::to_string(context.subtitleSizePercent) + "%").c_str());
    if (HWND slider = GetDlgItem(context.windowHandle, kSubtitleSizeSliderId))
        SendMessageA(slider, TBM_SETPOS, TRUE, context.subtitleSizePercent);
    if (HWND button = GetDlgItem(context.windowHandle, kSubtitlePositionButtonId))
    {
        const char* position = context.subtitlePosition == horde::gameplay::dialogue::SubtitlePosition::Top ? "TOP" :
            context.subtitlePosition == horde::gameplay::dialogue::SubtitlePosition::Bottom ? "BOTTOM" : "AUTO";
        SetWindowTextA(button, (std::string("SUBTITLE POSITION: ") + position).c_str());
    }
    if (HWND preset = GetDlgItem(context.windowHandle, kGraphicsPresetButtonId))
    {
        const auto name = horde::graphics::GraphicsPresetName(horde::graphics::MatchGraphicsPreset(
            graphicsDraft, horde::graphics::GraphicsPlatform::Windows));
        SetWindowTextA(preset, ("PRESET: " + std::string(name)).c_str());
    }
    if (HWND fire = GetDlgItem(context.windowHandle, kGraphicsFireButtonId))
        SetWindowTextA(fire, (std::string("FIRE: ") + GraphicsFireName(graphicsDraft.fireDetail)).c_str());
    if (HWND shadow = GetDlgItem(context.windowHandle, kGraphicsShadowButtonId))
        SetWindowTextA(shadow, (std::string("SHADOW: ") + GraphicsShadowName(graphicsDraft.shadowQuality)).c_str());
    if (HWND glass = GetDlgItem(context.windowHandle, kGraphicsGlassButtonId))
        SetWindowTextA(glass, graphicsDraft.glassEnabled ? "GLASS: ON" : "GLASS: OFF");
    if (HWND mist = GetDlgItem(context.windowHandle, kGraphicsMistButtonId))
        SetWindowTextA(mist, graphicsDraft.mistEnabled ? "MIST: ON" : "MIST: OFF");
    if (HWND dust = GetDlgItem(context.windowHandle, kGraphicsDustButtonId))
        SetWindowTextA(dust, (std::string("INDOOR DUST: ") + GraphicsDustName(graphicsDraft.dustQuality)).c_str());
    if (context.graphicsVisible)
    {
        const auto state = context.graphicsEdit->State();
        const bool editable = !context.graphicsSceneRestoring && (state == horde::graphics::GraphicsEditState::Editing ||
            state == horde::graphics::GraphicsEditState::Committed || state == horde::graphics::GraphicsEditState::Failed);
        for (const int id : {kGraphicsPresetButtonId, kGraphicsFireButtonId, kGraphicsShadowButtonId, kGraphicsGlassButtonId, kGraphicsMistButtonId, kGraphicsDustButtonId, kWaterQualityButtonId,
                             kRenderScaleSliderId, kGraphicsApplyButtonId, kGraphicsResetButtonId})
            EnableWindow(GetDlgItem(context.windowHandle, id), editable);
        EnableWindow(GetDlgItem(context.windowHandle, kGraphicsConfirmButtonId),
                     !context.graphicsSceneRestoring && state == horde::graphics::GraphicsEditState::AwaitingConfirmation);
        EnableWindow(GetDlgItem(context.windowHandle, kGraphicsRevertButtonId), !context.graphicsSceneRestoring);
        EnableWindow(GetDlgItem(context.windowHandle, kSettingsBackButtonId), !context.graphicsSceneRestoring);
        if (HWND title = GetDlgItem(context.windowHandle, kSettingsTitleId))
            SetWindowTextA(title, "GRAPHICS  |  APPLY, KEEP OR REVERT");
        std::ostringstream info;
        const auto extent = context.rtScene.DispatchExtent();
        const bool mobileOptics = context.rtScene.SelectedDielectricQualityName() == "Mobile";
        info << "Effective: " << std::lround(context.appliedRenderScale * 100.0f) << "%  |  internal "
             << extent.width << 'x' << extent.height << "  |  output " << context.swapchainExtent.width << 'x' << context.swapchainExtent.height
            << "  |  glass " << (context.rtScene.IsReady() && context.rtScene.GlassEnabled() ? "On" : "Off")
            << "\r\nUploaded fire: " << GraphicsFireName(UploadedGraphicsFireDetail(context))
            << "  |  shadows: " << GraphicsShadowName(UploadedGraphicsShadowQuality(context))
            << "  |  mist: " << (context.rtScene.UploadedMistEnabled().has_value() ? (*context.rtScene.UploadedMistEnabled() ? "On" : "Off") : "unavailable")
            << "  |  indoor dust: " << (context.rtScene.UploadedDustQuality().has_value() ? GraphicsDustName(*context.rtScene.UploadedDustQuality()) : "unavailable")
            << "\r\n" << (mobileOptics ?
                 "Mobile optical build remains fixed; Glass Off removes all pane geometry." :
                 "High optical build: physical panes retained; profile is fixed by this build.")
             << "\r\nWater: Off omits water, Mobile refracts, High adds scene reflections."
             << "\r\nFire: Low/Mobile/High use2/4/10 steps; light strength unchanged."
             << "\r\nShadows: Current area1; Higher area2/4. Cost unmeasured."
             << "\r\nIndoor dust: Off/Low/Standard controls bounded visible motes; cost not yet measured."
             << "\r\nApply needs an RT frame. Keep confirms within 15 foreground seconds."
             << "\r\n" << context.graphicsStatus;
        SetWindowTextA(GetDlgItem(context.windowHandle, kGraphicsInfoId), info.str().c_str());
    }
    else if (HWND title = GetDlgItem(context.windowHandle, kSettingsTitleId))
        SetWindowTextA(title, context.entryMenuVisible ? "SETTINGS" :
            "SETTINGS  |  SAVED BESIDE THE DEMO");

    HMENU menu = GetMenu(context.windowHandle);
    if (menu)
    {
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

void LayoutVitalityHud(HWND window, int clientWidth, int maximum)
{
    if (HWND hud = GetDlgItem(window, kVitalityHudControlId))
    {
        const UINT dpi = GetDpiForWindow(window);
        const auto layout = horde::platform::windows::LayoutWindowsVitalityHud(
            clientWidth, maximum, static_cast<int>(dpi ? dpi : kDefaultDpi));
        MoveWindow(hud, ScaleForDpi(window, 14), ScaleForDpi(window, 52),
                   layout.width, layout.height, TRUE);
    }
}

void UpdateVitalityHud(VulkanSurfaceContext& context)
{
    const horde::gameplay::PlayerVitalsSnapshot& vitals = context.simulation.Snapshot().playerVitals;
    // Native accessible name is retained; the owner-drawn HUD only paints hearts.
    const std::string current = vitals.halfHeart
        ? std::to_string(vitals.vitality - 1) + ".5" : std::to_string(vitals.vitality);
    const std::string text = "Vitality " + current + " of " + std::to_string(vitals.maxVitality);
    if (HWND hud = GetDlgItem(context.windowHandle, kVitalityHudControlId))
    {
        if(!horde::platform::windows::SetNativeTextIfChanged(hud,text))return;
        RECT client{};
        GetClientRect(context.windowHandle, &client);
        LayoutVitalityHud(context.windowHandle, client.right, vitals.maxVitality);
        InvalidateRect(hud, nullptr, FALSE);
    }
}

void RecordWindowsTextLayout(const VulkanSurfaceContext& context)
{
#if HORDE_RT_WINDOWS_ACCEPTANCE
    RECT client{},outer{};
    GetClientRect(context.windowHandle,&client);GetWindowRect(context.windowHandle,&outer);
    std::ostringstream report;
    report<<"DPI: "<<GetDpiForWindow(context.windowHandle)
        <<"\nClient: "<<client.right<<'x'<<client.bottom
        <<"\nWindow: "<<outer.right-outer.left<<'x'<<outer.bottom-outer.top
        <<"\nSubtitle scale percent: "<<context.subtitleSizePercent
        <<"\nConfigured render scale: "<<context.renderScale<<'\n';
    for(int id:{kPauseTitleId,kChestPromptControlId,kCombatTeachingPromptId}) {
        HWND control=GetDlgItem(context.windowHandle,id);if(!control)continue;
        RECT bounds{};GetClientRect(control,&bounds);char text[1024]{};GetWindowTextA(control,text,sizeof(text));
        HFONT font=reinterpret_cast<HFONT>(SendMessageA(control,WM_GETFONT,0,0));LOGFONTA face{};
        if(font)GetObjectA(font,sizeof(face),&face);
        HDC dc=GetDC(control);SIZE measured{};HGDIOBJ old=dc&&font?SelectObject(dc,font):nullptr;
        if(dc)GetTextExtentPoint32A(dc,text,static_cast<int>(std::strlen(text)),&measured);
        if(dc&&old&&old!=HGDI_ERROR)SelectObject(dc,old);if(dc)ReleaseDC(control,dc);
        report<<"Control "<<id<<": "<<bounds.right<<'x'<<bounds.bottom
            <<" font="<<face.lfFaceName<<" height="<<face.lfHeight
            <<" textWidth="<<measured.cx<<" text="<<text<<'\n';
    }
    (void)WriteReportFile(ExecutableDirectory()/kReportDirectory/"windows_text_layout.txt",report.str());
#else
    (void)context;
#endif
}

void UpdateChestPrompt(VulkanSurfaceContext& context)
{
    context.chestInteractionPromptPresented = false;
    context.rescueInteractionPromptPresented = false;
    context.flaskUpgradeInteractionPromptPresented = false;
    HWND promptControl = GetDlgItem(context.windowHandle, kChestPromptControlId);
    if (promptControl == nullptr)
    {
        return;
    }
    const auto& snapshot = context.simulation.Snapshot();
    const horde::platform::windows::WindowsChestPromptVisibility visibility{
         .simulationPaused = context.simulationPaused,
         .pauseMenuVisible = context.pauseMenuVisible,
         .settingsVisible = context.settingsVisible,
         .diagnosticsVisible = context.diagnosticsVisible,
         .benchmarkReportVisible = context.benchmarkReportVisible,
         .rtLabVisible = context.rtLabVisible,
         .deathOverlayVisible = context.deathOverlayVisible,
         .endingOverlayVisible = context.endingOverlayVisible,
         .benchmarkRunning = context.benchmark.IsRunning(),
         .captureMode = GetPropA(context.windowHandle, kCaptureModeProperty) != nullptr};
    const bool rescueJourney = snapshot.developmentRescueJourney &&
        snapshot.rescuePrompt != horde::gameplay::traversal::RescuePrompt::None;
    const bool flaskUpgrade = !rescueJourney && snapshot.flaskUpgrade.interactEligible;
    const bool raiseLantern = snapshot.NeedsRaiseLanternPrompt();
    const bool visible = (rescueJourney || flaskUpgrade || raiseLantern)
        ? !visibility.simulationPaused && !visibility.pauseMenuVisible && !visibility.settingsVisible &&
          !visibility.diagnosticsVisible && !visibility.benchmarkReportVisible && !visibility.rtLabVisible &&
          !visibility.deathOverlayVisible && !visibility.endingOverlayVisible && !visibility.benchmarkRunning &&
          !visibility.captureMode
        : horde::platform::windows::ShouldShowWindowsChestPrompt(snapshot.chestPrompt, visibility);
    std::string_view text;
    if (rescueJourney)
    {
        using horde::gameplay::traversal::RescuePrompt;
        switch (snapshot.rescuePrompt)
        {
        case RescuePrompt::Climb: text = "RESCUE ROPE | LEFT-CLICK / A TO CLIMB"; break;
        case RescuePrompt::Descend: text = "RESCUE ROPE | LEFT-CLICK / A TO DESCEND"; break;
        case RescuePrompt::Preparing: text = "RESCUE ROUTE PREPARING..."; break;
        case RescuePrompt::Traversing: text = "TRAVERSING RESCUE ROPE..."; break;
        default: break;
        }
        context.rescueInteractionPromptPresented = visible &&
            (snapshot.rescuePrompt == RescuePrompt::Climb || snapshot.rescuePrompt == RescuePrompt::Descend);
    }
    else if(flaskUpgrade) {
        text = "FLASK UPGRADE | LEFT-CLICK / A TO TAKE";
        context.flaskUpgradeInteractionPromptPresented=visible;
    }
    else
    {
        text = horde::platform::windows::WindowsChestPromptText(snapshot.chestPrompt);
        context.chestInteractionPromptPresented = visible &&
            (snapshot.chestPrompt == horde::gameplay::interactions::ChestRewardPrompt::OpenChest ||
             snapshot.chestPrompt == horde::gameplay::interactions::ChestRewardPrompt::ClaimLantern);
    }
    std::string contextualText(text);
    if (raiseLantern) {
        contextualText=horde::platform::windows::WindowsRaiseLanternPromptText(context.teachingControllerGlyphs,
            rescueJourney && snapshot.rescuePrompt==horde::gameplay::traversal::RescuePrompt::Descend);
    }
    if (visible)
    {
        if(horde::platform::windows::SetNativeTextIfChanged(promptControl,contextualText)) {
            InvalidateRect(promptControl, nullptr, FALSE);
            RecordWindowsTextLayout(context);
        }
    }
    horde::platform::windows::SetNativeVisibilityIfChanged(promptControl,visible);
}

bool IsDesktopGameplayAvailable(const VulkanSurfaceContext& context)
{
    return context.controlsEnabled && !context.graphicsVisible &&
        context.simulation.Snapshot().playerVitals.phase == horde::gameplay::PlayerLifePhase::Alive &&
        horde::platform::windows::ShouldShowWindowsChestPrompt(
            horde::gameplay::interactions::ChestRewardPrompt::OpenChest,
            {.simulationPaused = context.simulationPaused,
             .pauseMenuVisible = context.pauseMenuVisible,
             .settingsVisible = context.settingsVisible,
             .diagnosticsVisible = context.diagnosticsVisible,
             .benchmarkReportVisible = context.benchmarkReportVisible,
             .rtLabVisible = context.rtLabVisible,
             .deathOverlayVisible = context.deathOverlayVisible,
             .endingOverlayVisible = context.endingOverlayVisible,
             .benchmarkRunning = context.benchmark.IsRunning(),
             .captureMode = GetPropA(context.windowHandle, kCaptureModeProperty) != nullptr});
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
    // Restore/load changes health without a damage event. Keep the accessible
    // native caption synchronized with the same snapshot that paints hearts.
    UpdateVitalityHud(context);
}

bool MeasurementPausedByUi(const VulkanSurfaceContext& context)
{
    const bool pauseVisible = context.pauseMenuVisible && !context.settingsVisible &&
                              !context.diagnosticsVisible &&
                              !context.benchmarkReportVisible && !context.rtLabVisible;
    return (context.lookoutObjective && context.lookoutObjective->Visible()) ||
           context.campaignPause || (context.campaignMenu && context.campaignMenu->Visible()) ||
           pauseVisible || context.settingsVisible || context.rtLabVisible ||
           context.diagnosticsVisible || context.benchmarkReportVisible;
}

void RefreshCombatTeachingPrompt(VulkanSurfaceContext& context)
{
    HWND prompt = GetDlgItem(context.windowHandle, kCombatTeachingPromptId);
    if (!prompt) return;
    const auto& snapshot = context.simulation.Snapshot();
    const auto& teaching = snapshot.combatTeaching;
    const bool revivalClearance = snapshot.skeletonRevival.enabled &&
        snapshot.skeletonRevival.phase == horde::gameplay::simulation::SkeletonRevivalPhase::Rearming &&
        snapshot.skeletonRevival.waitingForClearance && snapshot.skeletonRevivalPresentationReady;
    const bool attackCue = horde::platform::windows::HasAttackReadabilityCue(teaching);
    const bool practiceCue = teaching.safePractice && !teaching.lessonSuppressed;
    if ((context.combatAttackCueVisible && !attackCue) || (context.combatPracticeCueVisible && !practiceCue))
    {
        context.combatTeachingFadeText.clear();
        context.combatTeachingFadeOpacity = 0u;
        context.combatTeachingFadeStartedAtMs = 0u;
    }
    context.combatAttackCueVisible = attackCue;
    context.combatPracticeCueVisible = practiceCue;
    const std::string text = horde::platform::windows::CombatTeachingPromptText(teaching, context.teachingControllerGlyphs, revivalClearance);
    const bool eligible = context.controlsEnabled && !MeasurementPausedByUi(context) &&
        !context.benchmarkReportVisible && !context.benchmark.IsRunning() &&
        !context.deathOverlayVisible && !context.endingOverlayVisible && !context.rtLabVisible;
    if (!eligible)
    {
        context.combatTeachingFadeText.clear();
        context.combatTeachingFadeOpacity = 0u;
        context.combatTeachingFadeStartedAtMs = 0u;
        horde::platform::windows::SetNativeTextIfChanged(prompt, "");
        SetLayeredWindowAttributes(prompt, 0u, 0u, LWA_ALPHA);
        SetControlVisible(context.windowHandle, kCombatTeachingPromptId, false);
        return;
    }

    if (!text.empty())
    {
        context.combatTeachingFadeText = text;
        context.combatTeachingFadeOpacity = attackCue || teaching.safePractice || revivalClearance ? static_cast<unsigned char>(255u) : horde::platform::windows::CombatTeachingPromptAlpha(
            teaching.promptOpacity, context.reducedMotionEnabled);
        context.combatTeachingFadeStartedAtMs = 0u;
        horde::platform::windows::SetNativeTextIfChanged(prompt, text);
        SetLayeredWindowAttributes(prompt, 0u, context.combatTeachingFadeOpacity, LWA_ALPHA);
        SetControlVisible(context.windowHandle, kCombatTeachingPromptId, true);
        return;
    }

    if (context.combatTeachingFadeText.empty() || context.reducedMotionEnabled)
    {
        context.combatTeachingFadeText.clear();
        context.combatTeachingFadeOpacity = 0u;
        context.combatTeachingFadeStartedAtMs = 0u;
        horde::platform::windows::SetNativeTextIfChanged(prompt, "");
        SetLayeredWindowAttributes(prompt, 0u, 0u, LWA_ALPHA);
        SetControlVisible(context.windowHandle, kCombatTeachingPromptId, false);
        return;
    }

    const std::uint64_t now = GetTickCount64();
    if (context.combatTeachingFadeStartedAtMs == 0u)
        context.combatTeachingFadeStartedAtMs = now;
    const unsigned char alpha = horde::platform::windows::CombatTeachingPromptFadeAlpha(
        context.combatTeachingFadeOpacity, now - context.combatTeachingFadeStartedAtMs,
        context.reducedMotionEnabled);
    if (alpha == 0u)
    {
        context.combatTeachingFadeText.clear();
        context.combatTeachingFadeOpacity = 0u;
        context.combatTeachingFadeStartedAtMs = 0u;
        horde::platform::windows::SetNativeTextIfChanged(prompt, "");
        SetLayeredWindowAttributes(prompt, 0u, 0u, LWA_ALPHA);
        SetControlVisible(context.windowHandle, kCombatTeachingPromptId, false);
        return;
    }
    horde::platform::windows::SetNativeTextIfChanged(prompt, context.combatTeachingFadeText);
    SetLayeredWindowAttributes(prompt, 0u, alpha, LWA_ALPHA);
    SetControlVisible(context.windowHandle, kCombatTeachingPromptId, true);
}

LookoutPanel::View CurrentLookoutObjectiveView(const VulkanSurfaceContext& context)
{
    const auto& snapshot=context.simulation.Snapshot();
    const bool available=context.controlsEnabled && context.pauseMenuVisible && !context.entryMenuVisible &&
        !context.settingsVisible && !context.diagnosticsVisible && !context.benchmarkReportVisible &&
        !context.graphicsVisible && !context.rtLabVisible && !context.deathOverlayVisible && !context.endingOverlayVisible &&
        !context.campaignPause && !(context.campaignMenu && context.campaignMenu->Visible()) &&
        !context.benchmark.IsRunning() && context.rtScene.Profile()==horde::vulkan::raytracing::RtSceneProfile::Showcase &&
        horde::gameplay::campaign::LookoutObjectiveAvailable(snapshot,context.simulation.LatestCampaignBoundary());
    return {available,context.lookoutAppActive,snapshot.worldRoute.generation,context.campaignSurfaceGeneration};
}

void ApplyOverlayState(VulkanSurfaceContext& context)
{
    if(context.lookoutObjective)context.lookoutObjective->Refresh(CurrentLookoutObjectiveView(context));
    const bool lookoutVisible=context.lookoutObjective && context.lookoutObjective->Visible();
    const bool pauseVisible = context.pauseMenuVisible && !context.settingsVisible &&
                              !context.diagnosticsVisible && !context.benchmarkReportVisible &&
                              !context.rtLabVisible && !lookoutVisible;
    for (const int id : {kPauseTitleId, kResumeButtonId, kRestartButtonId, kExitButtonId})
    {
        SetControlVisible(context.windowHandle, id, pauseVisible);
    }
    const bool entryFront = context.entryMenuVisible && pauseVisible && !context.entryMoreVisible;
    if (context.entryMenuVisible && pauseVisible)
    {
        for (const int id :
             {kRestartButtonId, kControlsButtonId, kDiagnosticsButtonId, kRunBenchmarkButtonId,
              kReportProblemButtonId, kMoreBySamfa12ButtonId, kExitButtonId})
            SetControlVisible(context.windowHandle, id,
                              context.entryMoreVisible && id != kRestartButtonId);
    }
    SetControlVisible(context.windowHandle, kEntryMoreButtonId, entryFront);
    SetControlVisible(context.windowHandle, kEntryBackButtonId,
                      context.entryMenuVisible && context.entryMoreVisible && pauseVisible);
    SetControlVisible(context.windowHandle, kResumeButtonId,
                      pauseVisible && (!context.entryMenuVisible || entryFront));
    SetControlVisible(context.windowHandle, kEndingBodyId, pauseVisible && context.endingOverlayVisible);
    const bool fullPauseMenuVisible = pauseVisible && !context.deathOverlayVisible && !context.endingOverlayVisible;
    for (const int id : {kControlsButtonId, kSettingsButtonId, kDiagnosticsButtonId,
                         kRunBenchmarkButtonId, kMoreBySamfa12ButtonId, kReportProblemButtonId})
    {
        SetControlVisible(context.windowHandle, id, fullPauseMenuVisible);
    }
    if (context.entryMenuVisible && pauseVisible)
    {
        for (const int id :
             {kRestartButtonId, kControlsButtonId, kDiagnosticsButtonId, kRunBenchmarkButtonId,
              kReportProblemButtonId, kMoreBySamfa12ButtonId, kExitButtonId})
            SetControlVisible(context.windowHandle, id,
                              context.entryMoreVisible && id != kRestartButtonId);
        SetControlVisible(context.windowHandle, kSettingsButtonId, entryFront);
    }
    SetControlVisible(context.windowHandle, kRunBenchmarkButtonId,
        context.entryMenuVisible && context.entryMoreVisible && pauseVisible &&
        OpeningBenchmarkEligibilityFor(context).Eligible());
    for (int id : {kBenchmarkStandardChoiceId,kBenchmarkCustomChoiceId,kBenchmarkChoiceRunId,
                   kBenchmarkChoiceBackId,kBenchmarkChoiceInfoId})
        SetControlVisible(context.windowHandle,id,context.benchmarkChooserVisible);
    const bool rtLabAccess = context.rtLabUnlocked || context.rtLabDebugInjection;
    SetControlVisible(context.windowHandle, kRtLabButtonId,
                      pauseVisible && rtLabAccess && !context.deathOverlayVisible);
    for (const int id : {kSettingsTitleId, kSfxVolumeLabelId, kSfxVolumeSliderId,
                          kSensitivityButtonId, kMusicVolumeLabelId, kMusicVolumeSliderId,
                          kFullscreenButtonId, kGraphicsOpenButtonId, kVoiceVolumeLabelId,
                          kVoiceVolumeSliderId, kSubtitlesButtonId, kSubtitleSizeLabelId,
                          kSubtitleSizeSliderId, kSubtitlePositionButtonId, kSettingsMoreId})
    {
        const bool dialogueControl = id == kVoiceVolumeLabelId || id == kVoiceVolumeSliderId ||
            id == kSubtitlesButtonId || id == kSubtitleSizeLabelId || id == kSubtitleSizeSliderId ||
            id == kSubtitlePositionButtonId;
        SetControlVisible(context.windowHandle, id, context.settingsVisible && !context.graphicsVisible &&
            (dialogueControl == context.settingsDialoguePage));
    }
    SetControlVisible(context.windowHandle, kSettingsTitleId, context.settingsVisible);
    SetControlVisible(context.windowHandle, kSettingsBackButtonId, context.settingsVisible);
    if (HWND page = GetDlgItem(context.windowHandle, kSettingsMoreId))
        SetWindowTextA(page, context.settingsDialoguePage ? "BACK TO GENERAL SETTINGS" : "AUDIO & SUBTITLES...");
    for (const int id : {kWaterQualityButtonId, kRenderScaleLabelId, kRenderScaleSliderId,
                         kGraphicsPresetButtonId, kGraphicsFireButtonId, kGraphicsShadowButtonId, kGraphicsGlassButtonId, kGraphicsMistButtonId, kGraphicsApplyButtonId,
                         kGraphicsDustButtonId,
                         kGraphicsConfirmButtonId, kGraphicsRevertButtonId, kGraphicsResetButtonId, kGraphicsInfoId,
                         kGraphicsPreviewPauseId, kGraphicsPreviewCameraId, kGraphicsPreviewMotionId,
                         kGraphicsPreviewResetId, kGraphicsPreviewTelemetryId, kGraphicsPreviewGraphId})
        SetControlVisible(context.windowHandle, id, context.graphicsVisible);
    SetControlVisible(context.windowHandle, kEditControlId,
                      context.diagnosticsVisible || context.benchmarkReportVisible);
    for (const int id : {kBenchmarkTitleId, kBenchmarkCopyButtonId,
                         kBenchmarkSaveButtonId, kBenchmarkReviewStatsButtonId, kBenchmarkBackButtonId})
    {
        SetControlVisible(context.windowHandle, id, context.benchmarkReportVisible);
    }
    EnableWindow(GetDlgItem(context.windowHandle, kBenchmarkReviewStatsButtonId),
        context.benchmarkSummary && context.benchmarkSummary->IsReady() && !context.benchmark.IsRunning());
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
                      !context.entryMenuVisible && !context.diagnosticsVisible &&
                          !context.benchmarkReportVisible && !context.rtLabVisible);
    SetControlVisible(context.windowHandle, kVitalityHudControlId,
                      !pauseVisible && !context.settingsVisible && !context.diagnosticsVisible &&
                          !context.benchmarkReportVisible && !context.benchmark.IsRunning() &&
                          !context.rtLabVisible);
    RefreshCombatTeachingPrompt(context);
    if (HMENU menu = GetMenu(context.windowHandle))
    {
        if (HMENU demo = GetSubMenu(menu, 0))
            if (HMENU teaching = GetSubMenu(demo, 2))
            {
                CheckMenuItem(teaching, kMenuTeachingEnabledId,
                              MF_BYCOMMAND | (context.combatTeachingEnabled ? MF_CHECKED : MF_UNCHECKED));
                CheckMenuItem(teaching, kMenuTeachingSlowdownId,
                              MF_BYCOMMAND | (context.combatTeachingSlowdown ? MF_CHECKED : MF_UNCHECKED));
                ModifyMenuA(teaching,kMenuTeachingSkipId,MF_BYCOMMAND | MF_STRING,kMenuTeachingSkipId,
                    horde::platform::windows::CombatPracticeActionLabel(context.simulation.Snapshot().combatTeaching));
            }
    }
#if defined(_DEBUG)
    SetControlVisible(context.windowHandle, kDeveloperOverlayId,
                       context.developerOverlayVisible && !pauseVisible && !context.benchmarkReportVisible &&
                           !context.settingsVisible && !context.diagnosticsVisible &&
                           !context.benchmark.IsRunning() && !context.rtLabVisible);
#endif
    const bool wasSimulationPaused = context.simulationPaused;
    context.simulationPaused = MeasurementPausedByUi(context);
    context.simulationInput.paused = context.simulationPaused;
    PublishMusicPlayback(context); // Menu suspension need not wait for a GPU frame.
    if (context.rtFrameEvidenceInitialised)
    {
        (void)context.rtFrameEvidence.SetPaused(context.simulationPaused);
    }
    if (context.simulationPaused != wasSimulationPaused)
    {
        context.simulation.ResetTiming();
    }
    if (context.simulationPaused)
    {
        context.benchmarkEvidence.Cancel();
        ClearDesktopInput(context);
        DiscardDesktopPendingCommands(context);
    }
    // Overlay transitions are synchronous. Re-evaluate here so a chest prompt
    // cannot survive for one rendered frame beneath pause, death, finale, lab,
    // diagnostics, benchmark, settings, or capture UI.
    UpdateChestPrompt(context);
    if (HMENU menu = GetMenu(context.windowHandle))
    {
        ModifyMenuA(menu, kMenuPauseId, MF_BYCOMMAND | MF_STRING, kMenuPauseId,
                    context.simulationPaused ? "&Resume\tEsc" : "&Pause\tEsc");
        DrawMenuBar(context.windowHandle);
    }
    if (HWND title = GetDlgItem(context.windowHandle, kPauseTitleId))
    {
        SetWindowTextA(
            title,
            context.deathOverlayVisible
                ? "YOU FELL  |  THE RUIN CLAIMS ANOTHER LIGHT."
                : (context.endingOverlayVisible
                       ? (context.rtLabJustUnlocked
                              ? "RT LAB UNLOCKED"
                              : "DAWN RETURNS  |  THE LAST LANTERN HAS DONE ITS WORK")
                       : (context.entryMenuVisible ?
                            (context.entryMoreVisible && context.openingBenchmark && !context.openingBenchmark->statusTitle.empty()
                                ? context.openingBenchmark->statusTitle.c_str() : "THE HORDE")
                            : "PAUSED")));
    }
    if (HWND resume = GetDlgItem(context.windowHandle, kResumeButtonId))
    {
        SetWindowTextA(
            resume, context.deathOverlayVisible
                        ? "RETRY ENCOUNTER"
                        : (context.endingOverlayVisible
                               ? "CONTINUE"
                               : (context.entryMenuVisible ? "PLAY" : "ENTER THE RUIN / RESUME")));
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
    const bool campaignVisible=context.campaignMenu && context.campaignMenu->Visible();
    SetControlVisible(context.windowHandle,kCampaignSaveButtonId,pauseVisible && !campaignVisible &&
        !context.entryMenuVisible && !context.deathOverlayVisible && !context.endingOverlayVisible);
    SetControlVisible(context.windowHandle,kCampaignLoadButtonId,pauseVisible && !campaignVisible &&
        (!context.entryMenuVisible || context.entryMoreVisible) && !context.endingOverlayVisible);
    const auto lookoutView=CurrentLookoutObjectiveView(context);
    SetControlVisible(context.windowHandle,kLookoutObjectiveButtonId,pauseVisible && lookoutView.available && lookoutView.active);
    if(campaignVisible) {
        EnumChildWindows(context.windowHandle,[](HWND child,LPARAM)->BOOL {
            const int id=GetDlgCtrlID(child);
            if(id<4100 || id>4123)ShowWindow(child,SW_HIDE);
            return TRUE;
        },0);
        RefreshCampaignMenu(context);
    }
    if(lookoutVisible) {
        EnumChildWindows(context.windowHandle,[](HWND child,LPARAM)->BOOL {
            const int id=GetDlgCtrlID(child);
            if(id<LookoutPanel::TitleId || id>LookoutPanel::BackId)ShowWindow(child,SW_HIDE);
            return TRUE;
        },0);
    }
    if (context.benchmarkChooserVisible || context.openingBenchmarkReturnPending) {
        EnumChildWindows(context.windowHandle,[](HWND child,LPARAM value)->BOOL {
            const auto& c=*reinterpret_cast<const VulkanSurfaceContext*>(value);
            const int id=GetDlgCtrlID(child);
            ShowWindow(child, !c.openingBenchmarkReturnPending && id>=kBenchmarkStandardChoiceId &&
                id<=kBenchmarkChoiceInfoId ? SW_SHOW : SW_HIDE);
            return TRUE;
        },reinterpret_cast<LPARAM>(&context));
        if (context.benchmarkChooserVisible) RefreshOpeningBenchmarkChooser(context);
    }
}

horde::graphics::GraphicsSettings CurrentGraphicsSettings(const VulkanSurfaceContext& context)
{
    const auto uploadedDust = context.rtScene.UploadedDustQuality();
    return {static_cast<int>(std::lround(context.appliedRenderScale * 100.0f)),
        static_cast<horde::graphics::WaterQuality>(context.waterQuality), UploadedGraphicsFireDetail(context),
        context.graphicsPreviewFrameCap, context.rtScene.IsReady() && context.rtScene.GlassEnabled(),
        UploadedGraphicsShadowQuality(context), horde::telemetry::CurrentCompletedMistEnabled(
            context.rtFrameEvidence.PublishedStateByValue()).value_or(true),
        uploadedDust.value_or(static_cast<horde::graphics::DustQuality>(255u))};
}

horde::graphics::GraphicsAppliedSnapshot GraphicsSnapshot(
    const VulkanSurfaceContext& context, const horde::graphics::GraphicsCommand& command)
{
    horde::graphics::GraphicsAppliedSnapshot snapshot;
    snapshot.serial = command.serial; snapshot.lifecycleGeneration = command.lifecycleGeneration;
    snapshot.requested = command.requested; snapshot.effective = CurrentGraphicsSettings(context);
    snapshot.backend = context.executionBackend == horde::vulkan::RtExecutionBackend::RayQueryCompute ?
        horde::graphics::GraphicsBackend::RayQueryCompute : horde::graphics::GraphicsBackend::RayTracingPipeline;
    snapshot.opticalProfile = context.rtScene.SelectedDielectricQualityName() == "Mobile" ?
        horde::graphics::OpticalProfile::Mobile : horde::graphics::OpticalProfile::High;
    const auto internal = context.rtScene.DispatchExtent();
    snapshot.internalExtent = {internal.width, internal.height};
    snapshot.outputExtent = {context.swapchainExtent.width, context.swapchainExtent.height};
    snapshot.scene =
        context.rtScene.Profile() == horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview
            ? horde::graphics::GraphicsScene::Preview
        : context.rtScene.Profile() == horde::vulkan::raytracing::RtSceneProfile::EntryMenu
            ? horde::graphics::GraphicsScene::EntryMenu
            : horde::graphics::GraphicsScene::Showcase;
    return snapshot;
}

void CloseGraphicsPage(VulkanSurfaceContext& context)
{
    if (context.rtScene.Profile() != context.graphicsReturnProfile)
    {
        context.graphicsSceneRestoring = true;
        context.sceneProfile = context.graphicsReturnProfile;
        context.sceneProfileDirty = true;
        context.graphicsStatus = "Restoring the previous scene; Back closes after its next successful RT frame.";
        UpdateSettingsLabels(context);
        return;
    }
    context.graphicsSceneRestoring = false;
    context.graphicsVisible = false; context.graphicsCloseAfterRevert = false;
    context.graphicsEdit.reset(); context.graphicsCommand.reset();
    EnableWindow(GetDlgItem(context.windowHandle, kSettingsBackButtonId), TRUE);
    context.sceneProfile = context.graphicsReturnProfile;
    context.sceneProfileDirty = context.rtScene.Profile() != context.sceneProfile;
    ApplyOverlayState(context);
    RECT client{}; GetClientRect(context.windowHandle, &client);
    LayoutOverlayControls(context.windowHandle, client.right, client.bottom);
    FocusGameControl(GetDlgItem(context.windowHandle, kGraphicsOpenButtonId));
}

bool QueueGraphicsCommand(VulkanSurfaceContext& context,
                          const std::optional<horde::graphics::GraphicsCommand>& command)
{
    if (!command || !context.graphicsEdit || command->lifecycleGeneration != 1u ||
        command->serial <= context.graphicsSerialFloor) return false;
    context.graphicsSerialFloor = command->serial;
    const bool persisted = SaveGraphicsRecord(context, context.graphicsEdit->Persistence());
    if (!persisted && command->kind == horde::graphics::GraphicsCommandKind::Apply)
    {
        context.graphicsEdit->Acknowledge(GraphicsSnapshot(context, *command), false);
        context.graphicsStatus = "Graphics settings file could not be written. The live configuration was preserved.";
        UpdateSettingsLabels(context);
        return false;
    }
    context.graphicsBeforeApply = CurrentGraphicsSettings(context);
    context.graphicsCommand = command;
    context.renderScale = command->requested.renderScalePercent / 100.0f;
    context.waterQuality = static_cast<horde::vulkan::raytracing::WaterQuality>(command->requested.waterQuality);
    context.fireDetail = command->requested.fireDetail;
    context.shadowQuality = command->requested.shadowQuality;
    context.graphicsPreviewFrameCap = command->requested.previewFrameCap;
    context.requestedGlassEnabled = command->requested.glassEnabled;
    context.requestedMistEnabled = command->requested.mistEnabled;
    context.requestedDustQuality = command->requested.dustQuality;
    context.glassGeometryDirty = !context.rtScene.IsReady() ||
        context.requestedGlassEnabled != context.rtScene.GlassEnabled();
    context.renderScaleDirty = std::abs(context.renderScale - context.appliedRenderScale) > 0.001f;
    if (context.rtScene.Profile() != horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview)
        context.graphicsReturnProfile = context.rtScene.Profile();
    context.graphicsPreview.Reset();
    context.graphicsPreviewCamera = horde::graphics::GraphicsPreviewCamera::Overview;
    context.graphicsPreview.SelectCamera(context.graphicsPreviewCamera);
    SetWindowTextA(GetDlgItem(context.windowHandle, kGraphicsPreviewCameraId), "VIEW: OVERVIEW");
    context.graphicsPreviewPerformance.BeginScope(++context.graphicsPreviewEpoch);
    context.graphicsPreviewLastSample = {};
    context.graphicsPreviewLastGpuSample = context.gpuRtTiming.sampleCount;
    context.graphicsStatus = command->kind == horde::graphics::GraphicsCommandKind::Apply ?
        "Applying requested graphics; saved settings remain unchanged." : "Restoring last confirmed graphics...";
    if (!persisted) context.graphicsStatus += " Storage is unavailable; the prior recovery record remains intact.";
    UpdateSettingsLabels(context);
    return true;
}

void FinishGraphicsFrame(VulkanSurfaceContext& context, const bool rtPresented)
{
    if (!context.graphicsEdit) return;
    const bool currentResourcesPresented = rtPresented && context.lastFramePresentation ==
        horde::telemetry::RtPresentationOutcome::Presented;
    if (context.graphicsSceneRestoring && currentResourcesPresented &&
        context.rtScene.Profile() == context.graphicsReturnProfile)
    { CloseGraphicsPage(context); return; }
    const auto completedMist = horde::telemetry::CurrentCompletedMistEnabled(context.rtFrameEvidence.PublishedStateByValue());
    const auto completedDust = horde::telemetry::CurrentCompletedDustQuality(context.rtFrameEvidence.PublishedStateByValue());
    if (context.graphicsCommand && currentResourcesPresented && context.rtScene.IsReady() &&
        completedMist && completedMist == context.rtScene.UploadedMistEnabled() &&
        *completedMist == context.requestedMistEnabled &&
        completedDust && completedDust == context.rtScene.UploadedDustQuality() &&
        *completedDust == context.requestedDustQuality &&
        context.rtScene.HasUploadedQualityControls() && context.rtScene.UploadedMistEnabled().has_value() &&
        !context.renderScaleDirty && !context.sceneProfileDirty && !context.glassGeometryDirty)
    {
        auto snapshot = GraphicsSnapshot(context, *context.graphicsCommand);
        snapshot.rtPresented = true;
        if (context.graphicsEdit->Acknowledge(snapshot, true))
        {
            const bool reverted = context.graphicsCommand->kind == horde::graphics::GraphicsCommandKind::Revert;
            context.graphicsCommand.reset();
            context.graphicsConfirmationTick = GetTickCount64();
            context.graphicsStatus = reverted ? "Last confirmed settings restored." : "RT frame presented. KEEP confirms; REVERT restores your saved choice.";
            if (reverted)
            {
                (void)SaveGraphicsRecord(context, context.graphicsEdit->Persistence());
                if (context.graphicsCloseAfterRevert) { CloseGraphicsPage(context); return; }
            }
            UpdateSettingsLabels(context);
        }
    }
    const auto now = GetTickCount64();
    if (context.graphicsConfirmationTick == 0u) context.graphicsConfirmationTick = now;
    const double seconds = static_cast<double>(now - context.graphicsConfirmationTick) / 1000.0;
    context.graphicsConfirmationTick = now;
    const bool foreground = GetForegroundWindow() == context.windowHandle && !IsIconic(context.windowHandle);
    if (auto restore = context.graphicsEdit->AdvanceConfirmation(seconds, foreground, 1u))
        (void)QueueGraphicsCommand(context, restore);
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
    FocusGameControl(GetDlgItem(context.windowHandle, kRtLabWaterfallSliderId));
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
    FocusGameControl(GetDlgItem(context.windowHandle, kRtLabButtonId));
}

void ShowPauseMenu(VulkanSurfaceContext& context, const bool visible)
{
    if (context.entryMenuVisible && !visible)
        return;
    if (context.graphicsVisible && context.graphicsEdit)
    {
        context.graphicsCloseAfterRevert = true;
        (void)QueueGraphicsCommand(context, context.graphicsEdit->RequestRevert(1u));
        return;
    }
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
        FocusGameControl(GetDlgItem(context.windowHandle, kResumeButtonId));
    }
    else
    {
        PlaySoundEffect(context, "ui_back.wav");
        FocusGameControl(context.windowHandle);
    }
}

horde::platform::windows::WindowsPlaytestReportContext CapturePlaytestReportContext(
    const VulkanSurfaceContext& context)
{
    horde::platform::windows::WindowsPlaytestReportContext report;
    auto& values = report.values;
    values.product = "Horde Lantern RT";
    values.version = HORDE_RT_DISPLAY_VERSION;
    values.build = HORDE_RT_BUILD_ID;
    values.platform = "Windows";
    values.rawModel = "Windows desktop";
    values.renderScale = context.renderScale;
    values.rtPresented = context.capabilitySnapshot != nullptr && context.capabilitySnapshot->rtScene.presented;
    values.quality = context.rtScene.SelectedDielectricQualityName();

    if (context.physicalDevice == VK_NULL_HANDLE) return report;
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(context.physicalDevice, &properties);
    const auto extent = context.rtScene.DispatchExtent();
    if (extent.width == 0u || extent.height == 0u || !std::isfinite(context.renderScale) ||
        context.renderScale <= 0.0f || properties.deviceName[0] == '\0')
    {
        return report;
    }
    values.gpu = properties.deviceName;
    values.backend = horde::vulkan::ToString(context.rtScene.ExecutionBackend());
    values.internalWidth = extent.width;
    values.internalHeight = extent.height;
    report.available = !values.backend.empty();
    return report;
}

void OpenLookoutObjective(VulkanSurfaceContext& context)
{
    const auto view=CurrentLookoutObjectiveView(context);
    if(!view.available || !view.active)return;
    if(!context.lookoutObjective)context.lookoutObjective=std::make_unique<LookoutPanel>();
    const auto font=reinterpret_cast<HFONT>(SendMessageA(GetDlgItem(context.windowHandle,kResumeButtonId),WM_GETFONT,0,0));
    if(!context.lookoutObjective->Open(context.windowHandle,view,font))return;
    for(auto control:context.lookoutObjective->ActionControls())InstallControllerFocusOutline(control);
    ApplyOverlayState(context);
    RECT client{};GetClientRect(context.windowHandle,&client);
    LayoutOverlayControls(context.windowHandle,client.right,client.bottom);
    FocusGameControl(GetDlgItem(context.windowHandle,LookoutPanel::ContinueId));
}

void FinishLookoutObjective(VulkanSurfaceContext& context,LookoutPanel::Action action)
{
    if(context.lookoutObjective)context.lookoutObjective->Close();
    ClearDesktopInput(context);DiscardDesktopPendingCommands(context);
    const auto& snapshot=context.simulation.Snapshot();
    context.cameraYaw=snapshot.playerYawRadians;context.cameraPitch=snapshot.playerPitchRadians;
    context.simulationInput.yawRadians=context.cameraYaw;context.simulationInput.pitchRadians=context.cameraPitch;
    context.simulationInput.hasAuthoritativePlayerPose=false;context.simulationInput.combatEdgeHistory={};
    context.simulationInput.dialogueCompletionLine=0;context.simulationInput.dialogueCompletionGeneration=0;
    context.lookoutMovementNeedsFreshKeyDown=true;context.lastControlSteadyNs=0;
    if(action==LookoutPanel::Action::ContinueExploring) {ShowPauseMenu(context,false);return;}
    // Reuse the branded RT entry profile. Its existing retirement path retains
    // the paused logical simulation and latest boundary; no reset/save/write.
    context.entryMenu.Reset();context.entryMenu.SetReducedMotion(context.reducedMotionEnabled);
    context.entryMenuVisible=true;context.entryMoreVisible=false;
    context.entryPlayQueued=context.entryPlayHandoffPending=false;
    SetEntryLoadingVisible(context,false);
    context.pauseMenuVisible=true;context.settingsVisible=context.diagnosticsVisible=context.benchmarkReportVisible=false;
    context.sceneProfile=horde::vulkan::raytracing::RtSceneProfile::EntryMenu;
    context.sceneProfileDirty=context.rtScene.Profile()!=context.sceneProfile;
    ApplyOverlayState(context);ApplyDpiScaledFonts(context.windowHandle);
    RECT client{};GetClientRect(context.windowHandle,&client);
    LayoutOverlayControls(context.windowHandle,client.right,client.bottom);
    FocusGameControl(GetDlgItem(context.windowHandle,kResumeButtonId));
}

void CancelLookoutObjective(VulkanSurfaceContext& context)
{
    if(!context.lookoutObjective || !context.lookoutObjective->Visible())return;
    // Dispatch the native command directly. Keyboard/controller cancellation
    // must not synthesize mouse/focus behavior, including in an unshown owner.
    SendMessageA(context.windowHandle,WM_COMMAND,MAKEWPARAM(LookoutPanel::BackId,BN_CLICKED),
        reinterpret_cast<LPARAM>(GetDlgItem(context.windowHandle,LookoutPanel::BackId)));
}

void RevokeLookoutObjective(VulkanSurfaceContext& context)
{
    if(!context.lookoutObjective || !context.lookoutObjective->Visible())return;
    context.lookoutObjective->Close();context.pauseMenuVisible=true;
    ApplyOverlayState(context);
    RECT client{};GetClientRect(context.windowHandle,&client);
    LayoutOverlayControls(context.windowHandle,client.right,client.bottom);
}

void OpenPlaytestReport(VulkanSurfaceContext& context)
{
    // The form is only reachable from paused/menu UI. A Help-menu request made
    // during play pauses first; closing the form leaves the pause in place.
    if (!context.simulationPaused || !context.pauseMenuVisible || context.settingsVisible ||
        context.diagnosticsVisible || context.benchmarkReportVisible || context.rtLabVisible)
    {
        ShowPauseMenu(context, true);
    }
    PlaySoundEffect(context, "ui_select.wav");
    horde::platform::windows::WindowsRemotePlaytestServices services;
    services.captureGameFrame = [&context](HWND,
        horde::platform::windows::WindowsPlaytestReportContext& capturedContext,
        horde::reporting::PlaytestScreenshotPixels& thumbnail, std::wstring& error) {
        // Called synchronously only after native consent/preflight. The paused
        // application/render owner owns both the RT image and this context.
        thumbnail = {};
        capturedContext = {};
        try
        {
            const auto presentation = context.lastFramePresentation;
            if (GetCurrentThreadId() != GetWindowThreadProcessId(context.windowHandle, nullptr) ||
                !context.simulationPaused || !context.useRtPath || !context.rtScene.IsReady() ||
                (presentation != horde::telemetry::RtPresentationOutcome::Presented &&
                 presentation != horde::telemetry::RtPresentationOutcome::PresentedNeedsRecreate))
            {
                error = L"No paused, successfully presented game RT frame is available. Uncheck the image option or try again after playing.";
                return false;
            }
            const auto extent = context.rtScene.DispatchExtent();
            if (extent.width == 0u || extent.height == 0u ||
                std::uint64_t{extent.width} * extent.height > horde::reporting::kPlaytestScreenshotMaxSourcePixels)
            {
                error = L"The game RT target exceeds the bounded report readback size. No image was captured.";
                return false;
            }
            horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
            std::string diagnostic;
            if (!context.rtScene.CaptureStorageImage(image, diagnostic) ||
                image.width != extent.width || image.height != extent.height ||
                !horde::reporting::ResizePlaytestScreenshotRgba(image.width, image.height, image.rgba, thumbnail))
            {
                error = L"The game-only image readback failed. No report was sent; the image was not silently omitted.";
                return false;
            }
            capturedContext = CapturePlaytestReportContext(context);
            return true;
        }
        catch (...)
        {
            thumbnail = {};
            capturedContext = {};
            // Error text itself may allocate. Return an empty last-resort error
            // rather than throw out of the application/render-owner callback.
            try { error = L"The bounded game-image allocation failed. No report was sent."; }
            catch (...) { error.clear(); }
            return false;
        }
    };
    horde::platform::windows::ShowWindowsRemotePlaytestReport(
        context.windowHandle, CapturePlaytestReportContext(context), std::move(services));
}

void ResetRoute(VulkanSurfaceContext& context, const bool preserveBenchmark = false)
{
    if(context.lookoutObjective)context.lookoutObjective->Close();
    if (!preserveBenchmark)
    {
        context.benchmarkEvidence.Cancel();
        if (context.benchmark.IsRunning()) context.benchmark.Cancel();
    }
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
    StopPrivateLichRevivalAudio(context);
    context.delayedFeedback.Clear();
    context.simulation.ResetRoute();
    if (context.rtFrameEvidenceInitialised)
    {
        (void)context.rtFrameEvidence.ApplyEvent(
            horde::telemetry::RtLifecycleEvent::RouteReset);
    }
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
    if(gCampaignHiddenValidation)return;
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
    if(context.lookoutObjective)context.lookoutObjective->Close();
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
    FocusGameControl(GetDlgItem(context.windowHandle, kResumeButtonId));
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
    FocusGameControl(GetDlgItem(context.windowHandle, kResumeButtonId));
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
    if(context.campaignConfirmedRetry && context.campaignPause)return false;
    const auto& beforeRetry=context.simulation.Snapshot();
    const bool confirmedRetry=beforeRetry.healingFlaskEnabled &&
        beforeRetry.developmentRescueJourney && beforeRetry.rescue.claimCount!=0;
    StopPrivateLichRevivalAudio(context);
    context.delayedFeedback.Clear();
    context.simulation.RetryEncounter();
    if(confirmedRetry) {
        // The campaign owner activates the confirmed-record barrier next pump.
        // Alive alone must not close the death pause or install a legacy camera.
        ClearDesktopInput(context);
        context.campaignPause=true;context.pauseMenuVisible=true;
        context.simulationInput.paused=true;ApplyOverlayState(context);
        return true;
    }
    context.benchmarkEvidence.Cancel();
    if (context.rtFrameEvidenceInitialised)
    {
        (void)context.rtFrameEvidence.ApplyEvent(
            horde::telemetry::RtLifecycleEvent::Retry);
    }
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
    FocusGameControl(context.windowHandle);
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
    metadata.legacyFrameTimingScope = "windows-render-plus-rtlab-telemetry";
    metadata.timestampUtc = UtcTimestamp("%Y-%m-%dT%H:%M:%SZ");
    metadata.buildIdentity = horde::reporting::CurrentBuildIdentity().ReportLabel();
    metadata.shaderIdentity = context.rtScene.SelectedPipelineBundleIdentity();
    metadata.gpuName = capabilities.identity.gpuName;
    metadata.vulkanApi = std::to_string(VK_API_VERSION_MAJOR(capabilities.identity.vulkanApiVersion)) + "." +
                         std::to_string(VK_API_VERSION_MINOR(capabilities.identity.vulkanApiVersion)) + "." +
                         std::to_string(VK_API_VERSION_PATCH(capabilities.identity.vulkanApiVersion));
    metadata.rtMode = horde::vulkan::ToString(capabilities.rtMode);
    metadata.executionBackend = horde::vulkan::ToString(context.rtScene.ExecutionBackend());
    metadata.presentMode = PresentModeName(context.swapchainPresentMode);
    metadata.materialEncoding = context.rtScene.MaterialEncoding();
    metadata.renderScalePercent = static_cast<std::uint32_t>(std::lround(context.renderScale * 100.0f));
    metadata.internalWidth = context.rtScene.DispatchExtent().width;
    metadata.internalHeight = context.rtScene.DispatchExtent().height;
    metadata.presentationWidth = context.swapchainExtent.width;
    metadata.presentationHeight = context.swapchainExtent.height;
    return metadata;
}

#include "platform/windows/WindowsBenchmarkSummaryReview.inl"

void UpdateBenchmarkHud(VulkanSurfaceContext& context)
{
    if (HWND hud = GetDlgItem(context.windowHandle, kHudControlId))
    {
        const std::string text = context.benchmark.ProgressText() + "  |  ESC CANCELS";
        SetWindowTextA(hud, text.c_str());
    }
}

void StartBenchmark(VulkanSurfaceContext& context,
                    const horde::gameplay::BenchmarkWorkload workload =
                        horde::gameplay::BenchmarkWorkload::ShowcaseRoute,
                    const bool showLiveFps = true,
                    const horde::vulkan::raytracing::RtWorkloadPreset rtWorkloadPreset =
                        horde::vulkan::raytracing::RtWorkloadPreset::Authored)
{
    context.benchmarkSummaryArm.reset();
    context.benchmarkSummary.reset();
    ResetRoute(context);
    // Route resets restore authored tuning; apply only the explicit existing
    // preset afterwards, including the unattended Release path.
    context.benchmarkRequestedRtPreset = rtWorkloadPreset;
    context.rtSceneTuning.workloadPreset = rtWorkloadPreset;
    context.benchmarkEffectiveRtPresetAtStart = context.rtSceneTuning.workloadPreset;
    context.benchmarkQualityControlsAtStart.reset();
    context.benchmarkCompiledQualityAtStart = context.rtScene.SelectedDielectricQualityName();
    context.benchmark.Start(horde::gameplay::ShowcaseBenchmarkRun::kDefaultLaps, workload, showLiveFps);
    (void)context.benchmarkEvidence.Start(
        horde::gameplay::ShowcaseBenchmarkRun::kMaximumFramesPerLap);
#if HORDE_RT_STAGED_PRIMARY_TIMING
    (void)context.stagedPassProfile.Start(horde::gameplay::ShowcaseBenchmarkRun::kMaximumFramesPerLap);
#endif
    context.expectedBenchmarkFrame.reset();
    if (context.rtFrameEvidenceInitialised)
    {
        (void)context.rtFrameEvidence.ApplyEvent(
            horde::telemetry::RtLifecycleEvent::BenchmarkStart);
    }
    context.rtLabRouteTainted = true;
    context.benchmarkCompletionHandled = false;
    context.benchmarkReportsSaved = false;
    context.benchmarkReport.clear();
    context.benchmarkJsonReport.clear();
    context.pauseMenuVisible = false;
    context.settingsVisible = false;
    context.diagnosticsVisible = false;
    context.benchmarkReportVisible = false;
    ApplyOverlayState(context);
    RECT benchmarkClient{};
    GetClientRect(context.windowHandle, &benchmarkClient);
    LayoutOverlayControls(context.windowHandle, benchmarkClient.right, benchmarkClient.bottom);
    UpdateBenchmarkHud(context);
    PlaySoundEffect(context, "ui_select.wav");
    FocusGameControl(context.windowHandle);
}

void LogBenchmarkCancellation(const VulkanSurfaceContext& context, const char* trigger)
{
    // One observation at cancellation, not per-frame instrumentation. Keep
    // lifecycle failures distinct from a completed hardware timing result.
    if (context.benchmark.IsRunning())
    {
        std::cerr << "Windows benchmark cancelled: trigger=" << trigger
                  << " measuredFrames=" << context.benchmark.Frames().size()
                  << " tick=" << context.simulation.Snapshot().tickIndex
                  << " foreground=" << (GetForegroundWindow() == context.windowHandle) << '\n';
    }
}

void CancelBenchmark(VulkanSurfaceContext& context, const bool showMenu, const char* trigger)
{
    if (context.benchmarkProfileActive || context.openingBenchmarkOwner) context.benchmarkProfileCancelRequested = true;
    if (!context.benchmark.IsRunning())
    {
        return;
    }
    LogBenchmarkCancellation(context, trigger);
    context.benchmarkSummaryArm.reset();
    context.benchmarkSummary.reset();
    context.benchmark.Cancel();
    context.benchmarkEvidence.Cancel();
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
    // The last measured present has no following frame fence. Drain its owning
    // submission before report construction or the route reset below. The caller
    // has already sampled the legacy outer-loop interval before this wait.
    if (context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Measuring)
    {
        const VkResult idleResult = vkDeviceWaitIdle(context.device);
        const bool drained = CompleteRtEvidenceAfterDeviceIdle(context, idleResult);
        (void)context.benchmarkEvidence.RecordOwnerDrainResult(drained);
        (void)context.benchmarkEvidence.Finalize();
    }
    if (context.benchmarkProfileActive && !ObserveWindowsBenchmarkProfileMeasurement(context, true))
        context.benchmark.Cancel();
    // Optional local review copies completed owner evidence before ResetRoute.
    // It never changes the legacy outcome, automatic files or unattended runs.
    FreezeWindowsBenchmarkSummary(context, capabilities);
    const horde::gameplay::ShowcaseBenchmarkMetadata metadata =
        BuildBenchmarkMetadata(context, capabilities);
    context.benchmarkReport = context.benchmark.BuildTextReport(metadata, &context.benchmarkEvidence);
    context.benchmarkJsonReport = context.benchmark.BuildJsonReport(metadata, &context.benchmarkEvidence);
    const auto tuningJson = horde::platform::windows::BuildWindowsBenchmarkTuningJson(
        context.benchmarkRequestedRtPreset, context.benchmarkEffectiveRtPresetAtStart,
        context.rtSceneTuning.workloadPreset, context.benchmarkCompiledQualityAtStart,
        context.rtScene.SelectedDielectricQualityName(), context.benchmarkQualityControlsAtStart,
        context.rtScene.IsReady() && context.rtScene.HasUploadedQualityControls() ?
            std::optional{context.rtScene.QualityControls()} : std::nullopt, true);
    context.benchmarkReport += "\nWindows RT workload policy: " + tuningJson + "\n";
    if (const auto closingBrace = context.benchmarkJsonReport.rfind('}'); closingBrace != std::string::npos)
        context.benchmarkJsonReport.insert(closingBrace, ",\n  \"windowsBenchmarkTuning\": " + tuningJson + "\n");
#if HORDE_RT_STAGED_PRIMARY_TIMING
    context.benchmarkJsonReport = horde::vulkan::raytracing::experimental::AttachStagedPrimaryProfile(
        std::move(context.benchmarkJsonReport),
        context.stagedPassProfile.Json(context.benchmarkEvidence.ExpectedCount(), context.stagedPassTimer,
                                      context.rtScene.ExecutionOrganisationJson()));
#endif
    const std::string stamp = UtcTimestamp("%Y%m%d-%H%M%S");
    const std::filesystem::path textPath = reportDirectory / ("HordeLanternRT-benchmark-" + stamp + ".txt");
    const std::filesystem::path jsonPath = reportDirectory / ("HordeLanternRT-benchmark-" + stamp + ".json");
    if (context.benchmarkProfileRequested) context.benchmarkProfileJsonPath = jsonPath;
    const bool jsonSaved = WriteReportFile(jsonPath, context.benchmarkJsonReport);
    context.benchmarkReport += "\nSaved text report: " + textPath.string() +
        "\nSaved JSON report: " + (jsonSaved ? jsonPath.string() : std::string("FAILED")) +
        "\nUse the buttons below or Ctrl+A, Ctrl+C to copy.\n";
    const bool textSaved = WriteReportFile(textPath, context.benchmarkReport);
    context.benchmarkReportsSaved = textSaved && jsonSaved;
    if (!textSaved)
    {
        context.benchmarkReport += "WARNING: automatic text report save failed; COPY REPORT and SAVE AS remain available.\n";
    }
    ResetRoute(context);
    // The explicit profile is unattended: its saved report is final, while the
    // original HWND/UI state still belongs to the pending RT restoration.
    if (context.benchmarkProfileRequested) return;
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
    FocusGameControl(GetDlgItem(context.windowHandle, kEditControlId));
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

std::string BuildDisplayText(const horde::vulkan::DeviceCapabilities& capabilities,
    const horde::telemetry::RtLifecyclePublishedState* evidence = nullptr)
{
    if (capabilities.rtMode == horde::vulkan::RtMode::Unsupported)
    {
        return horde::ui::BuildUnsupportedDeviceText(capabilities);
    }
    return horde::vulkan::BuildCapabilityTextReport(capabilities, evidence);
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
    snapshot.shaderIdentity = context.rtScene.SelectedPipelineBundleDisplayIdentity();
    snapshot.gpuName = capabilities.identity.gpuName;
    snapshot.vulkanApi = PackedVulkanVersion(capabilities.identity.vulkanApiVersion);
    snapshot.rtMode = horde::vulkan::ToString(context.rtScene.ExecutionBackend());
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
    snapshot.playerSafePractice = simulation.combatTeaching.safePractice;
    snapshot.combatObservation = horde::telemetry::ObserveCombatTarget(simulation);
    snapshot.playerDamageEnabled = IsPlayerDamageEnabled(context) && !snapshot.playerSafePractice;
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

bool CreateInstance(VkInstance& instance, horde::vulkan::PresentSurfaceSupport& presentSurfaceSupport)
{
    std::vector<const char*> extensions{VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    presentSurfaceSupport = horde::vulkan::AppendOptionalPresentInstanceExtensions(extensions);
    const VkApplicationInfo appInfo{
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "HordeLanternRTDiagnostic",
        VK_MAKE_VERSION(1, 0, 0),
        "horde_rt",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_2};

    const VkInstanceCreateInfo createInfo{
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr,
        0,
        &appInfo,
        0,
        nullptr,
        static_cast<uint32_t>(extensions.size()),
        extensions.data()};

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

void ClearDesktopInput(VulkanSurfaceContext& context)
{
    StopPrivateLichRevivalAudio(context);
    if(!gCampaignHiddenValidation)SpatialAudioEngine().StopLoop("chapter_ambience");
    if (context.crouchHeld) context.crouchKeyBlockedUntilRelease = true;
    context.crouchHeld = false;
    context.controllerCrouchHeld = false;
    context.controllerCrouchBlockedUntilRelease = true;
    if (context.clearCrouchIntentSequence != UINT64_MAX) ++context.clearCrouchIntentSequence;
    const auto& runSnapshot = context.simulation.Snapshot();
    if (context.controllerRunHeld)
        context.controllerRunBlockedUntilRelease = true;
    if (context.runHeld || context.controllerRunHeld || runSnapshot.runToggleActive ||
        runSnapshot.runActive)
    {
        if (context.clearRunIntentSequence != UINT64_MAX)
            ++context.clearRunIntentSequence;
    }
    context.runHeld = false;
    context.controllerRunHeld = false;
    context.runToggleKeyDown = false;
    context.keyboardGuardHeld = context.guardHeld = false;
    context.controllerGuardHold.Cancel();
    context.simulationInput.guardHeld = false;
    context.forwardHeld = false;
    context.backwardHeld = false;
    context.leftHeld = false;
    context.rightHeld = false;
    context.mouseLookActive = false;
    context.controllerForward = 0.0f;
    context.controllerStrafe = 0.0f;
    context.simulationInput.analogueMovement = false;
    context.controllerLookHorizontal = 0.0f;
    context.controllerLookVertical = 0.0f;
    context.previousControllerButtons = 0u;
    context.previousLegacyControllerButtons = 0u;
    context.controllerFocusLatch.LoseFocus();
    context.controllerTriggerLatch = {};
    context.xInputUserIndex.reset();
    context.legacyJoystickId.reset();
    context.legacyRightStickAxes = {};
    if(!gCampaignHiddenValidation && GetCapture()==context.windowHandle)ClipCursor(nullptr);
    if (context.mouseCursorHidden && !gCampaignHiddenValidation)
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

bool AcquireDesktopMouseLook(VulkanSurfaceContext& context)
{
    const HWND window = context.windowHandle;
    if (!IsDesktopGameplayAvailable(context) || GetForegroundWindow() != window ||
        gCampaignHiddenValidation || gOpeningBenchmarkMenuValidation || gShowcaseCaptureHidden)
        return false;
    FocusGameControl(window);
    SetCapture(window);
    if (GetCapture() != window || GetFocus() != window)
    {
        ClearDesktopInput(context);
        return false;
    }
    if (!context.mouseCursorHidden)
    {
        GetCursorPos(&context.mouseRestorePosition);
        ShowCursor(FALSE);
        context.mouseCursorHidden = true;
    }
    context.mouseLookActive = true;
    RECT clientRect{};
    GetClientRect(window, &clientRect);
    POINT centre{(clientRect.right-clientRect.left)/2,(clientRect.bottom-clientRect.top)/2};
    context.lastMousePosition = centre; // The centering warp is not a look delta.
    ClientToScreen(window, &centre);
    RECT screenRect{clientRect};
    ClientToScreen(window, reinterpret_cast<POINT*>(&screenRect.left));
    ClientToScreen(window, reinterpret_cast<POINT*>(&screenRect.right));
    ClipCursor(&screenRect);
    SetCursorPos(centre.x, centre.y);
    if (GetCapture() != window || GetFocus() != window || GetForegroundWindow() != window)
    {
        ClearDesktopInput(context);
        return false;
    }
    return true;
}

void DiscardDesktopPendingCommands(VulkanSurfaceContext& context)
{
    auto input = context.simulationInput;
    input.commands = {context.attackSequence, context.parrySequence, context.dodgeSequence,
        context.routeResetSequence, context.retrySequence, context.interactSequence,
        context.toggleHeldLightPoseSequence, context.runToggleSequence, context.clearRunIntentSequence,
        context.combatTeachingSkipSequence, context.combatTeachingReplaySequence,
        context.dialogueSkipSequence};
    input.runHeld = false;
    input.analogueMovement = false;
    input.crouchHeld = false;
    input.guardHeld = false;
    input.commands.crouchToggle = context.crouchToggleSequence;
    input.commands.clearCrouchIntent = context.clearCrouchIntentSequence;
    input.commands.drinkFlask=context.drinkFlaskSequence;input.commands.rest=context.restSequence;
    input.moveForward = input.moveStrafe = 0.0f;
    context.simulation.SynchronizePausedInput(input, 0u,
        horde::gameplay::simulation::PausedInputPolicy::PreserveWorldCommands);
}

std::vector<HWND> VisibleControllerMenuControls(const VulkanSurfaceContext& context)
{
    if(context.lookoutObjective && context.lookoutObjective->Visible()) {
        std::vector<HWND> controls;
        for(auto control:context.lookoutObjective->ActionControls())
            if(control && IsWindowVisible(control) && IsWindowEnabled(control))controls.push_back(control);
        return controls;
    }
    if(context.campaignMenu && context.campaignMenu->Visible()) {
        std::vector<HWND> controls;
        for(int id:{4110,4111,4112,4113,4114,4115,4120,4121,4122,4123}) {
            HWND control=GetDlgItem(context.windowHandle,id);
            if(control && IsWindowVisible(control) && IsWindowEnabled(control))controls.push_back(control);
        }
        return controls;
    }
    constexpr auto controlIds = std::to_array<int>({
        kBenchmarkStandardChoiceId,kBenchmarkCustomChoiceId,kBenchmarkChoiceRunId,kBenchmarkChoiceBackId,
        kResumeButtonId,
        kEntryMoreButtonId,
        kEntryBackButtonId,
        kRestartButtonId,
        kCampaignSaveButtonId,kCampaignLoadButtonId,kLookoutObjectiveButtonId,
        kControlsButtonId,
        kSettingsButtonId,
        kReportProblemButtonId,
        kRtLabButtonId,
        kDiagnosticsButtonId,
        kRunBenchmarkButtonId,
        kMoreBySamfa12ButtonId,
        kExitButtonId,
        kSensitivityButtonId,
        kGraphicsOpenButtonId,
        kWaterQualityButtonId,
        kRenderScaleSliderId,
        kSfxVolumeSliderId,
        kMusicVolumeSliderId,
        kVoiceVolumeSliderId,
        kSubtitleSizeSliderId,
        kSubtitlesButtonId,
        kSubtitlePositionButtonId,
        kSettingsMoreId,
        kFullscreenButtonId,
        kSettingsBackButtonId,
        kGraphicsPresetButtonId,
        kGraphicsFireButtonId,
        kGraphicsShadowButtonId,
        kGraphicsGlassButtonId,
        kGraphicsMistButtonId,
        kGraphicsDustButtonId,
        kGraphicsApplyButtonId,
        kGraphicsConfirmButtonId,
        kGraphicsRevertButtonId,
        kGraphicsResetButtonId,
        kGraphicsPreviewPauseId,
        kGraphicsPreviewCameraId,
        kGraphicsPreviewMotionId,
        kGraphicsPreviewResetId,
        kBenchmarkCopyButtonId,
        kBenchmarkSaveButtonId,
        kBenchmarkReviewStatsButtonId,
        kBenchmarkBackButtonId,
        kRtLabWaterfallSliderId,
        kRtLabRoofSliderId,
        kRtLabDawnSliderId,
        kRtLabFogSliderId,
        kRtLabFireStrengthSliderId,
        kRtLabFireTurbulenceSliderId,
        kRtLabFireSmokeSliderId,
        kRtLabGlassVisibilitySliderId,
        kRtLabGlassTransmissionSliderId,
        kRtLabGlassIorSliderId,
        kRtLabGlassRoughnessSliderId,
        kRtLabLightGroupButtonId,
        kRtLabHueSliderId,
        kRtLabIntensitySliderId,
        kRtLabWorkloadButtonId,
        kRtLabRestoreButtonId,
        kRtLabBackButtonId,
    });
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
    std::stable_sort(controls.begin(), controls.end(), [](HWND a, HWND b) {
        RECT first{}, second{};
        GetWindowRect(a, &first);
        GetWindowRect(b, &second);
        return horde::platform::windows::MenuControlRectPrecedes(
            {first.left, first.top, first.right, first.bottom},
            {second.left, second.top, second.right, second.bottom});
    });
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
    FocusGameControl(controls[index]);
    if (horde::platform::windows::ShouldPlayControllerMenuSound(context.rtLabVisible))
        PlaySoundEffect(context, "ui_select.wav");
}

void NavigateGraphicsMenu(VulkanSurfaceContext& context,
                          const horde::platform::windows::GraphicsMenuDirection direction)
{
    if (!context.graphicsVisible)
    {
        const bool backwards = direction == horde::platform::windows::GraphicsMenuDirection::Up ||
                               direction == horde::platform::windows::GraphicsMenuDirection::Left;
        NavigateControllerMenu(context, backwards ? -1 : 1);
        return;
    }

    const std::vector<HWND> controls = VisibleControllerMenuControls(context);
    if (controls.empty()) return;

    std::vector<horde::platform::windows::GraphicsMenuRect> rectangles;
    rectangles.reserve(controls.size());
    for (const HWND control : controls)
    {
        RECT bounds{};
        GetWindowRect(control, &bounds);
        rectangles.push_back({bounds.left, bounds.top, bounds.right, bounds.bottom});
    }

    const HWND focused = GetFocus();
    const auto found = std::find(controls.begin(), controls.end(), focused);
    std::size_t currentIndex = 0u;
    if (found != controls.end())
    {
        currentIndex = static_cast<std::size_t>(std::distance(controls.begin(), found));
    }
    else
    {
        // When the panel first receives navigation, start at its top-left
        // control instead of inheriting focus from an obscured settings item.
        for (std::size_t index = 1u; index < rectangles.size(); ++index)
        {
            if (rectangles[index].top < rectangles[currentIndex].top ||
                (rectangles[index].top == rectangles[currentIndex].top &&
                 rectangles[index].left < rectangles[currentIndex].left))
                currentIndex = index;
        }
    }

    const auto neighbor = horde::platform::windows::FindGraphicsMenuNeighbor(
        rectangles, currentIndex, direction);
    if (!neighbor) return;
    FocusGameControl(controls[*neighbor]);
    if (horde::platform::windows::ShouldPlayControllerMenuSound(context.rtLabVisible))
        PlaySoundEffect(context, "ui_select.wav");
}

void CancelControllerMenu(VulkanSurfaceContext& context)
{
    if(context.benchmarkChooserVisible) {
        PostMessageA(context.windowHandle,WM_COMMAND,kBenchmarkChoiceBackId,0);return;
    }
    if(context.lookoutObjective && context.lookoutObjective->Visible()) {CancelLookoutObjective(context);return;}
    if(context.campaignMenu && context.campaignMenu->Visible()) {
        context.campaignMenu->HandleCommand(4121);return;
    }
    int command = context.entryMenuVisible ? kEntryBackButtonId : kResumeButtonId;
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
    if (id == kSubtitlesButtonId)
    {
        context.subtitlesEnabled = !context.subtitlesEnabled;
        UpdateSettingsLabels(context);
        SaveSettings(context);
        return true;
    }
    if (id == kSubtitlePositionButtonId)
    {
        const int next = (static_cast<int>(context.subtitlePosition) + (increase ? 1 : 2)) % 3;
        context.subtitlePosition = static_cast<horde::gameplay::dialogue::SubtitlePosition>(next);
        UpdateSettingsLabels(context);
        SaveSettings(context);
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
    if (!rtLabSlider && id != kRenderScaleSliderId && id != kSfxVolumeSliderId &&
        id != kMusicVolumeSliderId && id != kVoiceVolumeSliderId &&
        id != kSubtitleSizeSliderId) return false;
    const int current = static_cast<int>(SendMessageA(focused, TBM_GETPOS, 0, 0));
    const int next = rtLabSlider
        ? horde::platform::windows::StepRtLabControl(current, increase, range)
        : (id == kRenderScaleSliderId
               ? horde::graphics::GraphicsRenderScaleSliderPositionFromPercent(
                     horde::graphics::StepGraphicsRenderScalePercent(
                         horde::graphics::GraphicsRenderScalePercentFromSliderPosition(current), increase))
               : id == kSubtitleSizeSliderId
                   ? std::clamp(current + (increase ? 5 : -5), 80, 160)
                   : horde::platform::windows::StepControllerAudioVolume(current, increase));
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
        if(context.lookoutObjective && context.lookoutObjective->Visible()) {CancelLookoutObjective(context);return;}
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
            if (context.graphicsVisible)
            {
                NavigateGraphicsMenu(context, edges.increase
                    ? horde::platform::windows::GraphicsMenuDirection::Right
                    : horde::platform::windows::GraphicsMenuDirection::Left);
                return;
            }
            NavigateControllerMenu(context, edges.increase ? 1 : -1);
        }
    }
    else if (edges.previous)
    {
        if (context.graphicsVisible)
            NavigateGraphicsMenu(context, horde::platform::windows::GraphicsMenuDirection::Up);
        else
            NavigateControllerMenu(context, -1);
    }
    else if (edges.next)
    {
        if (context.graphicsVisible)
            NavigateGraphicsMenu(context, horde::platform::windows::GraphicsMenuDirection::Down);
        else
            NavigateControllerMenu(context, 1);
    }
    else if (edges.confirm)
    {
        HWND focused = GetFocus();
        const std::vector<HWND> controls = VisibleControllerMenuControls(context);
        if (std::find(controls.begin(), controls.end(), focused) == controls.end())
        {
            if (!controls.empty()) FocusGameControl(controls.front());
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

void PublishDesktopCombatEdge(VulkanSurfaceContext& context,
    const horde::gameplay::simulation::CombatInputEdgeKind kind)
{
    using namespace horde::gameplay::simulation;
    std::uint64_t* sequence = kind == CombatInputEdgeKind::Attack ? &context.attackSequence :
        kind == CombatInputEdgeKind::Parry ? &context.parrySequence : &context.dodgeSequence;
    if (*sequence == UINT64_MAX) return;
    ++*sequence;
    CommandSequenceFor(context.simulationInput, kind) = *sequence;
    const auto movement=horde::platform::windows::ResolveDesktopMovementInput(
        context.forwardHeld,context.backwardHeld,context.leftHeld,context.rightHeld,
        context.controllerForward,context.controllerStrafe);
    context.simulationInput.moveForward=movement.forward;
    context.simulationInput.moveStrafe=movement.strafe;
    context.simulationInput.analogueMovement=movement.analogue;
    RecordCombatInputEdge(context.simulationInput, kind,
        horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr));
}

void PublishDesktopGuardHold(VulkanSurfaceContext& context)
{
    const bool allowed = IsDesktopGameplayAvailable(context) &&
        GetForegroundWindow() == context.windowHandle && GetFocus() == context.windowHandle;
    const bool next = allowed && (context.keyboardGuardHeld || context.controllerGuardHold.held);
    const bool fresh = next && !context.guardHeld;
    context.guardHeld = next;
    context.simulationInput.guardHeld = next;
    if (fresh) PublishDesktopCombatEdge(context, horde::gameplay::simulation::CombatInputEdgeKind::Parry);
}

void PublishDesktopHealingFlaskAction(VulkanSurfaceContext& context,bool rest)
{
    const auto& state=context.simulation.Snapshot();
    if(!IsDesktopGameplayAvailable(context) || GetForegroundWindow()!=context.windowHandle ||
       GetFocus()!=context.windowHandle || !state.healingFlaskEnabled ||
       state.healingFlask.action!=horde::gameplay::healing::FlaskAction::Idle ||
       !(rest?state.flaskRestAnchor!=0:state.flaskDrinkEligible))return;
    auto& sequence=rest?context.restSequence:context.drinkFlaskSequence;
    if(sequence!=UINT64_MAX)++sequence; // Named command; simulation owns time and eligibility.
}

void PollDesktopController(VulkanSurfaceContext& context)
{
    const horde::platform::windows::ControllerPollDisposition pollDisposition =
        context.controllerFocusLatch.Observe(GetForegroundWindow() == context.windowHandle);
    if (pollDisposition == horde::platform::windows::ControllerPollDisposition::Suppress)
    {
        context.controllerCrouchHeld = false;
        context.controllerCrouchBlockedUntilRelease = true;
        context.controllerRunHeld = false;
        context.controllerGuardHold.Cancel();
        PublishDesktopGuardHold(context);
        context.controllerForward = 0.0f;
        context.controllerStrafe = 0.0f;
        context.controllerLookHorizontal = 0.0f;
        context.controllerLookVertical = 0.0f;
        return;
    }

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
        context.controllerCrouchHeld = false; // No guessed WinMM shoulder mapping.
        context.controllerCrouchBlockedUntilRelease = true;
        context.controllerRunHeld = false;
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
            context.controllerGuardHold.Cancel();
            PublishDesktopGuardHold(context);
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
            context.controllerGuardHold.Cancel();
            PublishDesktopGuardHold(context);
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
        if (pollDisposition == horde::platform::windows::ControllerPollDisposition::Reseed)
        {
            context.legacyRightStickAxes =
                horde::platform::windows::SelectLegacyRightStickAxes(axisSample, identity);
            context.legacyJoystickId = joystick;
            context.previousLegacyControllerButtons = legacy.dwButtons;
            context.previousLegacyUiButtons = legacy.dwButtons;
            context.previousLegacyPov = legacy.dwPOV;
            context.controllerGuardHold.Reseed(horde::platform::windows::LegacyGuardHeld(legacy.dwButtons, identity));
            PublishDesktopGuardHold(context);
            context.controllerStrafe = 0.0f;
            context.controllerForward = 0.0f;
            context.controllerLookHorizontal = 0.0f;
            context.controllerLookVertical = 0.0f;
            context.controllerTriggerLatch = {};
            context.controllerFocusLatch.CompleteReseed();
            return;
        }
        if (context.legacyJoystickId != joystick ||
            context.legacyRightStickAxes.horizontal == horde::platform::windows::LegacyAxis::None)
        {
            context.legacyRightStickAxes =
                horde::platform::windows::SelectLegacyRightStickAxes(axisSample, identity);
            context.previousControllerButtons = 0u;
            // Acquiring a device is a baseline sample, not a fresh button
            // press. A stick held during connection must not toggle running.
            if (context.legacyJoystickId != joystick)
                context.previousLegacyControllerButtons = legacy.dwButtons;
            else
                context.previousLegacyControllerButtons = 0u;
            context.previousLegacyUiButtons = legacy.dwButtons;
            context.previousLegacyPov = legacy.dwPOV;
            context.controllerTriggerLatch = {};
            context.controllerGuardHold.Reseed(horde::platform::windows::LegacyGuardHeld(legacy.dwButtons, identity));
            PublishDesktopGuardHold(context);
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
        if (legacy.dwButtons != context.previousLegacyControllerButtons ||
            std::abs(context.controllerForward) > 0.0f || std::abs(context.controllerStrafe) > 0.0f ||
            std::abs(context.controllerLookHorizontal) > 0.0f || std::abs(context.controllerLookVertical) > 0.0f)
            context.teachingControllerGlyphs = true;
        context.controllerGuardHold.Observe(horde::platform::windows::LegacyGuardHeld(legacy.dwButtons, identity),
            !context.simulationPaused);
        PublishDesktopGuardHold(context);
        if (!context.simulationPaused)
        {
            if (edges.attackPressed) PublishDesktopCombatEdge(context, horde::gameplay::simulation::CombatInputEdgeKind::Attack);
            if (edges.dodgePressed) PublishDesktopCombatEdge(context, horde::gameplay::simulation::CombatInputEdgeKind::Dodge);
            if (edges.interactPressed) ++context.interactSequence;
            if (edges.toggleHeldLightPosePressed) ++context.toggleHeldLightPoseSequence;
        }
        const horde::platform::windows::ControllerMenuEdges menuEdges =
            horde::platform::windows::MapLegacyControllerMenuEdges(
                legacy.dwButtons, context.previousLegacyUiButtons,
                legacy.dwPOV, context.previousLegacyPov, identity);
        if(!context.simulationPaused) {
            const auto flask=horde::platform::windows::ResolveControllerFlaskEdges(menuEdges.previous,menuEdges.next,
                context.simulation.Snapshot().healingFlaskEnabled);
            if(flask.drink)PublishDesktopHealingFlaskAction(context,false);
            if(flask.rest)PublishDesktopHealingFlaskAction(context,true);
        }
        context.previousLegacyControllerButtons = legacy.dwButtons;
        context.previousLegacyUiButtons = legacy.dwButtons;
        context.previousLegacyPov = legacy.dwPOV;
        HandleControllerMenuEdges(context, menuEdges);
        return;
    }
    context.controllerRunHeld = false;
    context.controllerCrouchHeld = false;
    if (context.xInputUserIndex != xinputUser)
    {
        context.previousControllerButtons = state.Gamepad.wButtons;
        context.previousXInputUiButtons = state.Gamepad.wButtons;
        context.previousLegacyControllerButtons = 0u;
        horde::platform::windows::SeedXInputTriggerLatch(
            state.Gamepad.bLeftTrigger, state.Gamepad.bRightTrigger,
            context.controllerTriggerLatch);
        context.controllerGuardHold.Reseed(context.controllerTriggerLatch.leftHeld);
        PublishDesktopGuardHold(context);
    }
    context.xInputUserIndex = xinputUser;
    context.legacyJoystickId.reset();
    context.legacyRightStickAxes = {};
    context.previousLegacyUiButtons = 0u;
    context.previousLegacyPov = JOY_POVCENTERED;
    if (pollDisposition == horde::platform::windows::ControllerPollDisposition::Reseed)
    {
        context.previousControllerButtons = state.Gamepad.wButtons;
        context.previousXInputUiButtons = state.Gamepad.wButtons;
        context.previousLegacyControllerButtons = 0u;
        horde::platform::windows::SeedXInputTriggerLatch(
            state.Gamepad.bLeftTrigger, state.Gamepad.bRightTrigger,
            context.controllerTriggerLatch);
        context.controllerGuardHold.Reseed(context.controllerTriggerLatch.leftHeld);
        PublishDesktopGuardHold(context);
        context.controllerStrafe = 0.0f;
        context.controllerForward = 0.0f;
        context.controllerLookHorizontal = 0.0f;
        context.controllerLookVertical = 0.0f;
        context.controllerFocusLatch.CompleteReseed();
        return;
    }
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
    if (pressed != 0u || triggerEdges.attackPressed || triggerEdges.parryPressed ||
        std::abs(context.controllerForward) > 0.0f || std::abs(context.controllerStrafe) > 0.0f ||
        std::abs(context.controllerLookHorizontal) > 0.0f || std::abs(context.controllerLookVertical) > 0.0f)
        context.teachingControllerGlyphs = true;
    context.controllerGuardHold.Observe(context.controllerTriggerLatch.leftHeld, !context.simulationPaused);
    PublishDesktopGuardHold(context);
    if (!context.simulationPaused)
    {
        if (triggerEdges.attackPressed) PublishDesktopCombatEdge(context, horde::gameplay::simulation::CombatInputEdgeKind::Attack);
        if ((pressed & XINPUT_GAMEPAD_B) != 0u) PublishDesktopCombatEdge(context, horde::gameplay::simulation::CombatInputEdgeKind::Dodge);
        if ((pressed & XINPUT_GAMEPAD_A) != 0u) ++context.interactSequence;
        if ((pressed & XINPUT_GAMEPAD_X) != 0u &&
            context.simulation.Snapshot().chapterDialogue.line != horde::gameplay::dialogue::Line::None &&
            context.dialogueSkipSequence != UINT64_MAX)
            ++context.dialogueSkipSequence;
        if ((pressed & XINPUT_GAMEPAD_Y) != 0u) ++context.toggleHeldLightPoseSequence;
        const auto flask=horde::platform::windows::ResolveControllerFlaskEdges(
            (pressed & XINPUT_GAMEPAD_DPAD_UP)!=0u,(pressed & XINPUT_GAMEPAD_DPAD_DOWN)!=0u,
            context.simulation.Snapshot().healingFlaskEnabled);
        if(flask.drink)PublishDesktopHealingFlaskAction(context,false);
        if(flask.rest)PublishDesktopHealingFlaskAction(context,true);
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

void SynchronizeSimulationViewportAspect(VulkanSurfaceContext& context)
{
    // swapchainExtent is the logical output geometry, independent of the
    // render-scaled RT dispatch extent. Showcase capture and live frames share
    // this owner-thread input before the simulation snapshot is consumed.
    context.simulation.SetPresentationAspect(
        horde::graphics::RtViewAspectFromImageExtent(
            context.swapchainExtent.width, context.swapchainExtent.height,
            horde::graphics::RtPresentationTransform::Identity));
}

void UpdateDesktopSceneControls(
    VulkanSurfaceContext& context,
    horde::vulkan::raytracing::RtSceneRecordObservation* observation)
{
    const int previousVitality = context.simulation.Snapshot().playerVitals.vitality;
    const bool previousHalfHeart = context.simulation.Snapshot().playerVitals.halfHeart;
    const horde::gameplay::PlayerLifePhase previousLifePhase =
        context.simulation.Snapshot().playerVitals.phase;
    const std::uint64_t ownerAdvanceSteadyNs = horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr);
    double rawDeltaSeconds = 1.0 / 60.0;
    if (context.lastControlSteadyNs != 0u && ownerAdvanceSteadyNs >= context.lastControlSteadyNs)
    {
        rawDeltaSeconds = static_cast<double>(ownerAdvanceSteadyNs - context.lastControlSteadyNs) * 1.0e-9;
    }
    context.lastControlSteadyNs = ownerAdvanceSteadyNs;
    const float deltaSeconds = static_cast<float>(std::clamp(rawDeltaSeconds, 0.0, 0.1));
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
    const bool crouchTraversalOwned = context.simulation.Snapshot().rescuePrompt ==
        horde::gameplay::traversal::RescuePrompt::Traversing;
    if (crouchTraversalOwned && !context.crouchTraversalOwned)
    {
        if (context.crouchHeld) context.crouchKeyBlockedUntilRelease = true;
        context.crouchHeld = context.controllerCrouchHeld = false;
        context.controllerCrouchBlockedUntilRelease = true;
        if (context.clearCrouchIntentSequence != UINT64_MAX) ++context.clearCrouchIntentSequence;
    }
    context.crouchTraversalOwned = crouchTraversalOwned;
    input.yawRadians = context.cameraYaw;
    input.pitchRadians = context.cameraPitch;
    input.torchLightStrength = context.torchLightStrength;
    input.paused = context.simulationPaused;
    input.damageEnabled = IsPlayerDamageEnabled(context);
    input.commands.attack = context.attackSequence;
    input.commands.parry = context.parrySequence;
    input.commands.drinkFlask=context.drinkFlaskSequence;input.commands.rest=context.restSequence;
    input.commands.dodge = context.dodgeSequence;
    input.commands.routeReset = context.routeResetSequence;
    input.commands.retry = context.retrySequence;
    input.commands.interact = context.interactSequence;
    input.commands.toggleHeldLightPose = context.toggleHeldLightPoseSequence;
    input.commands.runToggle = context.runToggleSequence;
    input.commands.clearRunIntent = context.clearRunIntentSequence;
    input.commands.crouchToggle = context.crouchToggleSequence;
    input.commands.clearCrouchIntent = context.clearCrouchIntentSequence;
    input.tutorialEnabled = context.combatTeachingEnabled;
    input.tutorialSlowdownEnabled = context.combatTeachingSlowdown;
    input.commands.tutorialSkip = context.combatTeachingSkipSequence;
    input.commands.tutorialReplay = context.combatTeachingReplaySequence;
    input.commands.dialogueSkip = context.dialogueSkipSequence;
    input.dialogueCompletionLine = context.dialogueCompletionLine;
    input.dialogueCompletionGeneration = context.dialogueCompletionGeneration;
    input.runHeld = context.runHeld;
    input.crouchHeld = false;
    input.guardHeld = context.guardHeld && !context.simulationPaused;
    input.waterfallWidthScale = horde::vulkan::raytracing::ClampRtSceneTuning(
        context.rtSceneTuning).waterfallWidthScale;
    input.hasAuthoritativePlayerPose = false;
    const auto movement=horde::platform::windows::ResolveDesktopMovementInput(
        context.forwardHeld,context.backwardHeld,context.leftHeld,context.rightHeld,
        context.controllerForward,context.controllerStrafe);
    input.moveForward=movement.forward;input.moveStrafe=movement.strafe;
    input.analogueMovement=movement.analogue;

    if (context.benchmark.IsRunning())
    {
        if (context.rtSceneTuning.workloadPreset != context.benchmarkRequestedRtPreset ||
            context.rtScene.SelectedDielectricQualityName() != context.benchmarkCompiledQualityAtStart)
        {
            LogBenchmarkCancellation(context, "rt-workload-policy-changed");
            context.benchmark.Cancel();
            context.benchmarkEvidence.Cancel();
            return;
        }
        context.frameDeltaSeconds = 1.0f / 60.0f;
        ClearDesktopInput(context);
        const horde::gameplay::ShowcaseBenchmarkAdvance advance = context.benchmark.Advance();
        if (advance.lapStarted)
        {
            ResetRoute(context, true);
            context.rtSceneTuning.workloadPreset = context.benchmarkRequestedRtPreset;
            if (context.benchmark.CurrentLap() == context.benchmark.TotalLaps() &&
                context.rtFrameEvidenceInitialised)
            {
                (void)context.rtFrameEvidence.ApplyEvent(
                    horde::telemetry::RtLifecycleEvent::WarmupToMeasure);
            }
        }
        const auto workload = context.benchmark.Workload();
        if (horde::gameplay::IsLanternBenchmark(workload) && advance.frameInLap == 1u &&
            !horde::gameplay::StageLanternBenchmark(context.simulation, workload))
        {
            LogBenchmarkCancellation(context, "lantern-staging-failed");
            context.benchmark.Cancel();
            context.benchmarkEvidence.Cancel();
            return;
        }
        if (context.benchmark.CurrentLap() == context.benchmark.TotalLaps())
        {
            if (context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Allocated)
            {
                // Single-lap mode has no lapStarted transition.
                if (context.benchmark.TotalLaps() == 1u && context.rtFrameEvidenceInitialised)
                    (void)context.rtFrameEvidence.ApplyEvent(horde::telemetry::RtLifecycleEvent::WarmupToMeasure);
                const auto state = context.rtFrameEvidence.PublishedStateByValue();
                (void)context.benchmarkEvidence.ArmMeasurement(state.sceneEpoch, state.measurementGeneration);
                ArmWindowsBenchmarkSummary(context);
            }
            if (context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Measuring)
                context.expectedBenchmarkFrame = context.benchmarkEvidence.ExpectFrame(
                    {static_cast<std::uint32_t>(advance.replay.zone), context.benchmark.CurrentLap()});
        }
        input = context.simulationInput;
        input.paused = false;
        input.damageEnabled = false;
        input.hasAuthoritativePlayerPose = true;
        input.analogueMovement = false;
        input.authoritativePlayerX = advance.replay.x;
        input.authoritativePlayerZ = advance.replay.z;
        input.yawRadians = advance.replay.yaw;
        input.pitchRadians = horde::gameplay::IsLanternBenchmark(workload)
            ? horde::gameplay::kLanternBenchmarkPitch : -0.04f;
        if (horde::gameplay::IsFrozenBenchmark(workload)) context.frameDeltaSeconds = 0.0f;
        input.torchLightStrength = context.torchLightStrength;
        input.commands.attack = context.attackSequence;
        input.commands.parry = context.parrySequence;
        input.commands.drinkFlask=context.drinkFlaskSequence;input.commands.rest=context.restSequence;
        input.commands.dodge = context.dodgeSequence;
        input.commands.routeReset = context.routeResetSequence;
        input.commands.retry = context.retrySequence;
        input.commands.interact = context.interactSequence;
        input.commands.toggleHeldLightPose = context.toggleHeldLightPoseSequence;
        input.commands.runToggle = context.runToggleSequence;
        input.commands.clearRunIntent = context.clearRunIntentSequence;
        input.tutorialEnabled = context.combatTeachingEnabled;
        input.tutorialSlowdownEnabled = context.combatTeachingSlowdown;
        input.commands.tutorialSkip = context.combatTeachingSkipSequence;
        input.commands.tutorialReplay = context.combatTeachingReplaySequence;
        input.commands.dialogueSkip = context.dialogueSkipSequence;
        input.dialogueCompletionLine = context.dialogueCompletionLine;
        input.dialogueCompletionGeneration = context.dialogueCompletionGeneration;
        input.runHeld = false;
        input.analogueMovement = false;
        input.crouchHeld = false;
        input.commands.crouchToggle = context.crouchToggleSequence;
        input.commands.clearCrouchIntent = context.clearCrouchIntentSequence;
        if (advance.replay.waypointReached || advance.lapStarted || advance.finished)
        {
            UpdateBenchmarkHud(context);
        }
    }

#if defined(_DEBUG)
    if (context.nativeMotionValidation)
    {
        const auto publication = context.rtFrameEvidence.PublishedStateByValue();
        const bool ready = context.motionLedger.HasCurrentPresentedFrame(context.motionSurfaceGeneration,
            publication.sceneEpoch, publication.measurementGeneration);
        input = context.motionScenario.BuildInput(context.simulation.Snapshot(), input,
            horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr), ready);
    }
#endif
    context.simulationInput = input;
    horde::vulkan::raytracing::RtSceneStageScope simulationScope(
        observation, horde::telemetry::RtStage::SimulationStep);
    bool timestampedPlayerInput = !input.hasAuthoritativePlayerPose && !context.benchmark.IsRunning();
#if defined(_DEBUG)
    timestampedPlayerInput = timestampedPlayerInput && !context.nativeMotionValidation;
#endif
    context.simulation.AdvanceFrame(input,
                                    timestampedPlayerInput ? rawDeltaSeconds : context.frameDeltaSeconds,
                                    ++context.inputPublicationSequence,
                                    timestampedPlayerInput ? ownerAdvanceSteadyNs : 0u);
    if (context.dialogueCompletionGeneration != 0u &&
        context.simulation.Snapshot().chapterDialogue.generation != context.dialogueCompletionGeneration)
    {
        context.dialogueCompletionLine = 0u;
        context.dialogueCompletionGeneration = 0u;
    }
    RefreshCombatTeachingPrompt(context);
    simulationScope.Complete(1u);
#if defined(_DEBUG)
    if (context.nativeMotionValidation)
    {
        const auto& snapshot = context.simulation.Snapshot();
        const auto events = context.simulation.Events().Events();
        context.motionScenario.ObserveAdvance(snapshot, events);
        if (!context.motionLedger.AppendState(horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr), snapshot, input,
                context.motionScenario, events)) context.motionScenario.Fail(context.motionLedger.Failure());
        if (snapshot.retryGeneration != context.motionRetryGeneration)
        {
            context.motionRetryGeneration = snapshot.retryGeneration;
            context.motionRetryPending = true;
        }
    }
#endif
    // Camera yaw/pitch are continuous platform input targets. A render frame
    // may not produce a 60 Hz simulation tick, so copying the previous fixed
    // snapshot back here would erase right-stick and mouse look accumulated
    // between ticks. Reset/checkpoint paths still request a full view mirror.
    MirrorSimulationSnapshot(context, false);
    if (context.simulation.Snapshot().playerVitals.vitality != previousVitality ||
        context.simulation.Snapshot().playerVitals.halfHeart != previousHalfHeart ||
        context.simulation.Snapshot().playerVitals.phase != previousLifePhase)
    {
        UpdateVitalityHud(context);
    }
    if (horde::gameplay::IsLanternBenchmark(context.benchmark.Workload()) &&
        !context.benchmarkCompletionHandled &&
        (context.benchmark.FrameInLap() == 1u || context.benchmark.FrameInLap() % 60u == 0u))
        UpdateBenchmarkHud(context);
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
    input.analogueMovement = false;
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
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:
        context.runHeld = held;
        return true;
    default:
        return false;
    }
}

bool CreateLogicalDevice(VkPhysicalDevice physicalDevice,
                         VkInstance instance,
                         uint32_t graphicsQueueFamilyIndex,
                         horde::vulkan::RtExecutionBackend executionBackend,
                         VkDevice& device,
                         VkQueue& graphicsQueue,
                         horde::vulkan::PresentSurfaceSupport presentSurfaceSupport,
                         horde::vulkan::PresentCompletionMode& presentCompletionMode)
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
                std::cerr << "Selected hardware RT backend is missing required extension: " << extension << ".\n";
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
    ctx.renderFinishedSemaphores.resize(ctx.swapchainImages.size(), VK_NULL_HANDLE);
    ctx.inFlightFences.resize(kMaxFramesInFlight);
    VkSemaphoreCreateInfo semaphoreCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
    for (UINT i = 0u; i < kMaxFramesInFlight; ++i)
    {
        if (vkCreateSemaphore(ctx.device, &semaphoreCreateInfo, nullptr, &ctx.imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(ctx.device, &fenceCreateInfo, nullptr, &ctx.inFlightFences[i]) != VK_SUCCESS)
        {
            return false;
        }
    }

    for (VkSemaphore& semaphore : ctx.renderFinishedSemaphores)
    {
        if (vkCreateSemaphore(ctx.device, &semaphoreCreateInfo, nullptr, &semaphore) != VK_SUCCESS)
        {
            return false;
        }
    }
    if (!ctx.presentCompletionFences.Create(ctx.device, ctx.swapchainImages.size(),
        ctx.presentCompletionMode != horde::vulkan::PresentCompletionMode::Unextended)) return false;

    return true;
}

void AcceptBenchmarkCompletion(
    VulkanSurfaceContext& ctx,
    const horde::vulkan::raytracing::RtFrameEvidenceCompletionResult& result,
    const horde::telemetry::RtPerformanceEvidenceSnapshot& snapshot)
{
    if (result.completedEvidence &&
        ctx.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Measuring &&
        snapshot.identity.submitted.frame.sceneEpoch == ctx.benchmarkEvidence.SceneEpoch() &&
        snapshot.identity.submitted.frame.measurementGeneration == ctx.benchmarkEvidence.MeasurementGeneration())
    {
        (void)ctx.benchmarkEvidence.Complete(snapshot);
    }
}

#if HORDE_RT_STAGED_PRIMARY_TIMING
void CollectStagedPassTiming(VulkanSurfaceContext& ctx, std::uint32_t frameSlot,
                            const horde::vulkan::raytracing::RtFrameEvidenceCompletionResult& result,
                            const horde::telemetry::RtPerformanceEvidenceSnapshot& snapshot)
{
    // Called only after this slot's fence or a successful device idle. Even a
    // discarded benchmark identity must consume its query slot before reuse.
    if (!result.ownedGraphicsSubmission) return;
    const auto timing = ctx.stagedPassTimer.CollectCompleted(frameSlot);
    if (result.completedEvidence &&
        ctx.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Measuring &&
        snapshot.identity.submitted.frame.sceneEpoch == ctx.benchmarkEvidence.SceneEpoch() &&
        snapshot.identity.submitted.frame.measurementGeneration == ctx.benchmarkEvidence.MeasurementGeneration())
        (void)ctx.stagedPassProfile.Append(timing, snapshot);
}
#endif

#include "WindowsGpuStageObservation.inl"

bool CompleteRtEvidenceAfterDeviceIdle(
    VulkanSurfaceContext& ctx,
    const VkResult idleResult)
{
    if (!ctx.rtFrameEvidenceInitialised)
    {
        return idleResult == VK_SUCCESS;
    }
    if (idleResult != VK_SUCCESS)
    {
        ctx.rtFrameEvidence.NoteFailedDeviceIdle();
        ctx.lastRtFrameError = "vkDeviceWaitIdle failed before RT evidence completion.";
        return false;
    }

    const horde::vulkan::raytracing::RtGpuFrameTimerIo gpuIo =
        horde::vulkan::raytracing::MakeRtGpuFrameTimerIo(ctx.gpuFrameTimer);
    const horde::vulkan::raytracing::RtDiagnosticFrameIo diagnosticIo =
        horde::vulkan::raytracing::MakeRtDiagnosticFrameIo(ctx.rtScene);
    bool completed = true;
    for (std::uint32_t frameSlot = 0u; frameSlot < kMaxFramesInFlight; ++frameSlot)
    {
        horde::telemetry::RtPerformanceEvidenceSnapshot snapshot{};
        const horde::vulkan::raytracing::RtFrameEvidenceCompletionResult result =
            ctx.rtFrameEvidence.CompleteFinalIdle(frameSlot, gpuIo, diagnosticIo, &snapshot);
#if HORDE_RT_FRAME_STAGE_PROBE
        CollectWindowsGpuStages(ctx,frameSlot,result,snapshot);
#endif
#if HORDE_RT_STAGED_PRIMARY_TIMING
        CollectStagedPassTiming(ctx, frameSlot, result, snapshot);
#endif
        AcceptBenchmarkCompletion(ctx, result, snapshot);
#if defined(_DEBUG)
        if (ctx.nativeMotionValidation &&
            !ctx.motionLedger.AppendCompletedFrame(ctx.motionSurfaceGeneration, ctx.rtFrameEvidence.PublishedStateByValue()))
            ctx.motionScenario.Fail(ctx.motionLedger.Failure());
#endif
        if (result.gpuCollectionAttempted)
        {
            RefreshGpuTimingTelemetry(ctx, &result.gpuCollection);
        }
        if (result.fatalDiagnosticIoFailure)
        {
            ctx.lastRtFrameError =
                "Failed to read the completed RT Diagnostic buffer after device idle.";
            completed = false;
        }
    }
    return completed;
}

bool ConsumePendingImageAcquire(VulkanSurfaceContext& ctx)
{
    if (!ctx.imageAcquirePending) return true;
    // A successful acquire can outlive an aborted command recording. Consume
    // its signal before the teardown idle so the acquire semaphore is no longer
    // owned by WSI. This submission produces no frame or presentation evidence.
    const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.waitSemaphoreCount = 1u;
    submitInfo.pWaitSemaphores = &ctx.imageAvailableSemaphores[ctx.currentFrame];
    submitInfo.pWaitDstStageMask = &waitStage;
    if (vkQueueSubmit(ctx.graphicsQueue, 1u, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
        return false;
    ctx.imageAcquirePending = false;
    return true;
}

bool SaveGraphicsRecord(const VulkanSurfaceContext& context,
                        const horde::graphics::GraphicsPersistenceRecord& record)
{
    if (context.benchmarkProfileRequested) return true;
    if (context.graphicsPreviewCapture || context.outputResizeValidation || context.nativeMotionValidation)
        return true; // Ephemeral validation transaction.
    return horde::platform::windows::SaveGraphicsPersistenceRecord(SettingsPath(), record);
}

bool ReleaseSwapchainResources(VulkanSurfaceContext& ctx)
{
    if (ctx.device == VK_NULL_HANDLE)
    {
        ctx.benchmarkEvidence.Cancel();
        return true;
    }

    if (!ConsumePendingImageAcquire(ctx)) return false;
    // Unextended Vulkan has no present-completion fence at retirement. Retain
    // the portable idle drain; per-image reuse below never relies on this idle.
    const VkResult idleResult = vkDeviceWaitIdle(ctx.device);
    const bool evidenceCompleted = CompleteRtEvidenceAfterDeviceIdle(ctx, idleResult);
    if (idleResult != VK_SUCCESS) return false;
    if (ctx.presentCompletionFences.Drain() != VK_SUCCESS) return false;
    if (!ctx.presentCompletionFences.DestroyCompleted()) return false;
    ctx.benchmarkEvidence.Cancel();
    const bool evidenceRecreated = !ctx.rtFrameEvidenceInitialised ||
        ctx.rtFrameEvidence.Recreate(
            horde::telemetry::RtResourceResetReason::SwapchainRecreate,
            CurrentInitialGpuEvidenceStatus(ctx));
    ctx.gpuFrameTimer.ResetAfterDeviceIdle();
#if HORDE_RT_FRAME_STAGE_PROBE
    ResetWindowsGpuStagesAfterIdle(ctx);
#endif
#if HORDE_RT_STAGED_PRIMARY_TIMING
    if (idleResult == VK_SUCCESS) ctx.stagedPassTimer.ResetAfterDeviceIdle();
#endif
    ctx.gpuFrameTimingTotalMs = 0.0;
    ctx.gpuFrameTimingSampleCount = 0u;
    ctx.campaignBarrier.SurfaceRetired(ctx.campaignSurfaceGeneration++);
    if(ctx.campaign && !ctx.campaignBarrier.State().loaded){
        ctx.campaign->Cancel();ctx.campaignPreparation.reset();
        if(ctx.campaignPhase==horde::platform::windows::WindowsCampaignMenu::Phase::Reading || ctx.campaignPhase==horde::platform::windows::WindowsCampaignMenu::Phase::Preparing){
            ctx.campaignPhase=horde::platform::windows::WindowsCampaignMenu::Phase::Failed;
            ctx.campaignMessage="Load cancelled after output recreation. Current game retained.";
        }
    }
    ctx.campaignPreparedScene.reset();
    ctx.simulation.InvalidateWorldZoneReadiness();
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
    return evidenceCompleted && evidenceRecreated;
}

VkExtent2D ScaledRenderExtent(VkExtent2D presentationExtent, float renderScale)
{
    const int percent = horde::graphics::ClampGraphicsRenderScalePercent(static_cast<int>(std::lround(
        std::clamp(renderScale, 0.0f, 1.0f) * 100.0f)));
    const auto extent = horde::graphics::ScaledGraphicsExtent(
        {presentationExtent.width, presentationExtent.height}, percent);
    return {extent.width, extent.height};
}

void RefreshGpuTimingTelemetry(
    VulkanSurfaceContext& ctx,
    const horde::vulkan::GpuFrameTimingCollection* completed)
{
    if (completed != nullptr && completed->hasSample)
    {
        ctx.gpuFrameTimingTotalMs += completed->sample.milliseconds;
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

horde::telemetry::RtSampleStatus CurrentInitialGpuEvidenceStatus(
    VulkanSurfaceContext& ctx)
{
    const horde::vulkan::raytracing::RtGpuFrameTimerIo gpuIo =
        horde::vulkan::raytracing::MakeRtGpuFrameTimerIo(ctx.gpuFrameTimer);
    const horde::vulkan::raytracing::RtGpuTimerStateSnapshot timer =
        gpuIo.snapshot != nullptr ? gpuIo.snapshot(gpuIo.user)
                                  : horde::vulkan::raytracing::RtGpuTimerStateSnapshot{};
    return horde::vulkan::raytracing::InitialRtGpuEvidenceStatus(true, timer);
}

bool InitialiseRtSceneForSwapchain(VulkanSurfaceContext& ctx, const bool startupFailureDialog = true)
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
    const bool rescueJourney = ctx.simulation.Snapshot().developmentRescueJourney;
#if defined(_DEBUG)
    if(!ctx.developmentCheckpoint.empty())
        std::cerr << "HORDE_CHAPTER_INITIALISE begin checkpoint=" << ctx.developmentCheckpoint << std::endl;
#endif
    ctx.rtScene.SetDevelopmentWorldRoute(ctx.developmentWorldRoute || rescueJourney,ctx.stagedWorldPreparation);
    ctx.rtScene.SetDevelopmentRescueJourney(rescueJourney);
    ctx.rtScene.SetDevelopmentSupportFixture(ctx.developmentVerticalProof &&
        ctx.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::Showcase);
#if defined(_DEBUG)
    if(ctx.forestRouteProof!=horde::platform::windows::ForestRouteCaptureMode::Disabled) {
        using Mode=horde::vulkan::raytracing::PresentableTinyRtScene::ForestRouteProofMode;
        const bool normalShared=ctx.forestRouteProof==horde::platform::windows::ForestRouteCaptureMode::NormalShared64;
        if(normalShared && !ctx.rtScene.SetSharedForestGeometryEnabled(true)) {
            ctx.lastRtFrameError="Normal shared forest must be selected before scene creation.";return false;
        }
        const auto mode=normalShared?Mode::Disabled:
            ctx.forestRouteProof==horde::platform::windows::ForestRouteCaptureMode::SharedExisting64?
            Mode::SharedExisting64:
            ctx.forestRouteProof==horde::platform::windows::ForestRouteCaptureMode::SharedRetention64?
            Mode::SharedRetention64:
            ctx.forestRouteProof==horde::platform::windows::ForestRouteCaptureMode::SharedF01Grouped76?
            Mode::SharedF01Grouped76:
            ctx.forestRouteProof==horde::platform::windows::ForestRouteCaptureMode::SharedF01Sector76?
            Mode::SharedF01Sector76:Mode::BakedResident;
        if(!ctx.rtScene.SetForestRouteProofMode(mode,diagnostic)){ctx.lastRtFrameError=diagnostic;return false;}
    }
    if(ctx.previewMeshPair!=horde::platform::windows::PreviewMeshPairCaptureMode::None) {
        if(!ctx.previewMeshPairMaster) {
            std::shared_ptr<const horde::scene::PreparedForestDressing> prepared;
            if(!horde::scene::PrepareForestDressing(assetRoot,prepared,diagnostic)) {
                ctx.lastRtFrameError=diagnostic;return false;
            }
            ctx.previewMeshPairMaster=std::shared_ptr<const horde::scene::assets::StaticMeshAsset>(prepared,&prepared->masters[0]);
        }
        std::array<horde::gameplay::items::HeldItemTransform,2> transforms;
        constexpr std::array<float,2> yaw{{.42f,-.63f}};
        for(unsigned i=0;i<2;++i) {
            auto& m=transforms[i];m=horde::gameplay::items::IdentityHeldItemTransform();
            m[0]=m[10]=.25f*std::cos(yaw[i]);m[2]=-.25f*std::sin(yaw[i]);m[8]=-m[2];m[5]=.25f;
            m[12]=-1.35f;m[14]=i==0?-14.8f:-16.1f;
            m[13]=horde::gameplay::kRouteFloorWorldY-.25f*ctx.previewMeshPairMaster->bounds.minimum[1]-.01f;
        }
        if(!ctx.rtScene.SetDiagnosticPreviewMeshPair(ctx.previewMeshPairMaster,transforms,
            ctx.previewMeshPair==horde::platform::windows::PreviewMeshPairCaptureMode::Shared,diagnostic)) {
            ctx.lastRtFrameError=diagnostic;return false;
        }
    }
#endif
    std::string selectedSkeletonAssetPath =
        (assetRoot / "models/enemies/meshy/skeleton_biped_merged_animations_v01.glb").string();
    std::vector<horde::scene::SkinnedClipBinding> selectedSkeletonBindings;
#if defined(_DEBUG)
    if (const char* standPath = std::getenv("HORDE_SKELETON_STAND_CAPTURE_ASSET"))
    {
        const char* capture = std::getenv("HORDE_SKELETON_REVIVE_CAPTURE");
        std::string candidateHash;
        if (!gShowcaseCaptureHidden || ctx.sceneProfile != horde::vulkan::raytracing::RtSceneProfile::Showcase ||
            !capture || std::string_view(capture) != "1" || ctx.developmentCheckpoint.empty() ||
            std::getenv("HORDE_SKELETON_REVIVE_BLEND_CAPTURE") || !*standPath ||
            !Sha256File(standPath, candidateHash, diagnostic) ||
            candidateHash != "800632b9b1dbe53bd4f6508a6d167e52e940c6bdb4e902af752429e96d7fe259")
        {
            ctx.lastRtFrameError = "Authored stand asset requires its exact isolated hidden revival capture.";
            return false;
        }
        selectedSkeletonAssetPath = standPath;
        const auto& ordinary = horde::scene::SkeletonCombatClipSet().clips;
        selectedSkeletonBindings.assign(ordinary.begin(), ordinary.end());
        selectedSkeletonBindings.push_back({"SkeletonProof|StandDead0ToIdle", false, true});
    }
#endif
    if (!ctx.rtScene.Initialise(ctx.instance,
                                ctx.physicalDevice,
                                ctx.device,
                                ctx.graphicsQueue,
                                ctx.commandPool,
                                renderExtent,
                                ctx.swapchainFormat,
                                selectedSkeletonAssetPath,
                                (assetRoot / "models/enemies/meshy/lich_placeholder_merged_animations_v01.glb").string(),
                                (assetRoot / "textures/polyhaven/mobile_1k").string(),
                                (assetRoot / "textures/meshy/lich_placeholder_v01").string(),
                                diagnostic,
                                developmentStaticAssetDirectory,
                                assetRoot.string(),
                                ctx.executionBackend, ctx.sceneProfile, ctx.requestedGlassEnabled,
                                ctx.pipelineCache, nullptr, selectedSkeletonBindings))
    {
        std::cerr << "Failed to initialise presentable RT scene: " << diagnostic << '\n';
        ctx.lastRtFrameError = diagnostic;
        if (startupFailureDialog && !ctx.unattendedBenchmark) GameMessageBoxA(ctx.windowHandle,
                    ("The native RT scene could not start.\n\n" + diagnostic +
                     "\n\nKeep the packaged assets folder beside HordeLanternRT.exe. No fallback renderer will be used.").c_str(),
                    "Horde Lantern RT - startup error",
                    MB_OK | MB_ICONERROR);
        return false;
    }
#if defined(_DEBUG)
    if(!MaybeAdmitForestContentCandidate(ctx,diagnostic)) {
        ctx.lastRtFrameError=diagnostic;
        return false;
    }
    if(!ctx.developmentCheckpoint.empty())
        std::cerr << "HORDE_CHAPTER_INITIALISE scene-ready checkpoint=" << ctx.developmentCheckpoint << std::endl;
#endif
    if (ctx.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview &&
        !ctx.rtScene.ConfigurePreviewFireSockets(ctx.graphicsPreview, diagnostic))
    {
        ctx.lastRtFrameError = diagnostic;
        return false;
    }
    if (ctx.sceneProfile == horde::vulkan::raytracing::RtSceneProfile::EntryMenu &&
        !ctx.rtScene.ConfigureEntryMenu(ctx.entryMenu, diagnostic))
    {
        ctx.lastRtFrameError = diagnostic;
        return false;
    }
    if (ctx.gpuFrameTimer.Telemetry().status == horde::vulkan::GpuFrameTimerStatus::Uninitialised)
    {
        ctx.gpuFrameTimer.Initialise(
            ctx.physicalDevice, ctx.device, ctx.graphicsQueueFamilyIndex, kMaxFramesInFlight);
    }
#if HORDE_RT_FRAME_STAGE_PROBE
    if(ctx.captureGpuStages)for(auto& timer:ctx.frameStageTimers)
        if(timer.Telemetry().status==horde::vulkan::GpuFrameTimerStatus::Uninitialised)
            timer.Initialise(ctx.physicalDevice,ctx.device,ctx.graphicsQueueFamilyIndex,kMaxFramesInFlight);
#endif
    ctx.appliedRenderScale = ctx.renderScale;
#if HORDE_RT_STAGED_PRIMARY_TIMING
    if (ctx.stagedPassTimer.Status() == horde::vulkan::raytracing::experimental::StagedPrimaryTimingInitStatus::Uninitialised)
        ctx.stagedPassTimer.Initialise(ctx.physicalDevice, ctx.device, ctx.graphicsQueueFamilyIndex, kMaxFramesInFlight);
#endif
    RefreshGpuTimingTelemetry(ctx);
    if (!ctx.rtFrameEvidenceInitialised)
    {
        horde::telemetry::RtLifecycleSeeds seeds{};
        if (ctx.openingBenchmark) seeds = ctx.openingBenchmark->seeds;
        seeds.sceneEpoch = std::max<std::uint64_t>(1u, seeds.sceneEpoch);
        seeds.measurementGeneration = std::max<std::uint64_t>(1u, seeds.measurementGeneration);
        const horde::telemetry::RtInstrumentationMode instrumentation =
            ctx.rtScene.DiagnosticsAvailability() ==
                    horde::vulkan::raytracing::RtDiagnosticAvailability::Available
                ? horde::telemetry::RtInstrumentationMode::Diagnostic
                : horde::telemetry::RtInstrumentationMode::Shipping;
        if (!ctx.rtFrameEvidence.Initialise(
                seeds, kMaxFramesInFlight, instrumentation,
                CurrentInitialGpuEvidenceStatus(ctx),
                MeasurementPausedByUi(ctx)))
        {
            ctx.lastRtFrameError = "Invalid RT evidence ownership configuration.";
            return false;
        }
        ctx.rtFrameEvidenceInitialised = true;
    }
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
    if (!ReleaseSwapchainResources(ctx))
    {
        return false;
    }
    if (ctx.windowHandle == nullptr)
    {
        return false;
    }

    return CreateSwapchain(ctx, ctx.windowHandle) &&
           InitialiseRtSceneForSwapchain(ctx);
}

void DetachRenderContextHost(VulkanSurfaceContext& context) noexcept
{
    StopMenuAmbience(context); // The shared HWND survives an opening-menu benchmark owner hop.
    // Stop every host callback before either native cleanup or fatal retention.
    const HWND window = context.windowHandle;
    if (window != nullptr && IsWindow(window) &&
        GetWindowLongPtrA(window, GWLP_USERDATA) == reinterpret_cast<LONG_PTR>(&context))
        SetWindowLongPtrA(window, GWLP_USERDATA, 0);
#if defined(_DEBUG)
    // External polling cannot reserve this interval: a cross-thread SendMessage
    // may be serviced only after the successor attaches. Exercise the real HWND
    // handler synchronously here, only in the explicitly isolated Debug fixture.
    if (gOpeningBenchmarkMenuValidation &&
        (gOpeningBenchmarkDetachedCancelValidation || gOpeningBenchmarkDetachedCloseValidation) &&
        !gOpeningBenchmarkDetachedCancelInjected && context.openingBenchmark &&
        context.openingBenchmark->owner.Phase()==OpeningBenchmarkOwnerPhase::Queued &&
        GetWindowLongPtrA(window,GWLP_USERDATA)==0 &&
        GetPropA(window,kOpeningBenchmarkTransferProperty)==context.openingBenchmark) {
        gOpeningBenchmarkDetachedCancelInjected=true;
        std::cerr<<"HORDE_OPENING_MENU_STAGE detached-inject context-null=1 phase=queued pending-before="
                 <<context.openingBenchmark->cancelPending<<'\n';
        SendMessageA(window,gOpeningBenchmarkDetachedCloseValidation?WM_CLOSE:WM_KEYDOWN,
                     gOpeningBenchmarkDetachedCloseValidation?0:VK_ESCAPE,0);
        std::cerr<<"HORDE_OPENING_MENU_STAGE detached-inject-result pending-after="
                 <<context.openingBenchmark->cancelPending<<'\n';
    }
#endif
    if (context.musicPlayback) context.musicPlayback->Stop(); // Joins; noexcept.
    context.windowHandle = nullptr;
    context.capabilitySnapshot = nullptr; // Borrowed from the returning caller.
}

bool DestroyRenderContext(VulkanSurfaceContext& ctx)
{
    if (ctx.device == VK_NULL_HANDLE)
    {
        if (ctx.rtFrameEvidenceInitialised)
        {
            (void)ctx.rtFrameEvidence.Destroy();
            ctx.rtFrameEvidenceInitialised = false;
        }
        if (ctx.surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
        }
        if (ctx.instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(ctx.instance, nullptr);
        }
        return true;
    }

    if (!ConsumePendingImageAcquire(ctx))
    {
        std::cerr << "Failed to consume an acquired image semaphore; renderer resources retained.\n";
        return false;
    }
    const VkResult idleResult = vkDeviceWaitIdle(ctx.device);
    (void)CompleteRtEvidenceAfterDeviceIdle(ctx, idleResult);
    if (idleResult != VK_SUCCESS || ctx.presentCompletionFences.Drain() != VK_SUCCESS ||
        !ctx.presentCompletionFences.DestroyCompleted())
    {
        std::cerr << "Presentation retirement could not be proved; renderer resources retained.\n";
        return false;
    }
    if (ctx.rtFrameEvidenceInitialised)
    {
        (void)ctx.rtFrameEvidence.Destroy();
        ctx.rtFrameEvidenceInitialised = false;
    }
    ctx.campaignBarrier.SurfaceRetired(ctx.campaignSurfaceGeneration++);
    if(ctx.campaign && !ctx.campaignBarrier.State().loaded){
        ctx.campaign->Cancel();ctx.campaignPreparation.reset();
        if(ctx.campaignPhase==horde::platform::windows::WindowsCampaignMenu::Phase::Reading || ctx.campaignPhase==horde::platform::windows::WindowsCampaignMenu::Phase::Preparing){
            ctx.campaignPhase=horde::platform::windows::WindowsCampaignMenu::Phase::Failed;
            ctx.campaignMessage="Load cancelled after output recreation. Current game retained.";
        }
    }
    ctx.campaignPreparedScene.reset();
    ctx.simulation.InvalidateWorldZoneReadiness();
    ctx.rtScene.Destroy();
    if (ctx.pipelineCache != VK_NULL_HANDLE)
    {
        vkDestroyPipelineCache(ctx.device, ctx.pipelineCache, nullptr);
        ctx.pipelineCache = VK_NULL_HANDLE;
    }
    ctx.gpuFrameTimer.Destroy();
#if HORDE_RT_FRAME_STAGE_PROBE
    for(auto& timer:ctx.frameStageTimers)timer.Destroy();
    ctx.pendingStageOwners={};ctx.completedStageObservation={};
#endif
#if HORDE_RT_STAGED_PRIMARY_TIMING
    ctx.stagedPassTimer.Destroy();
#endif

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
    return true;
}

#include "windows_campaign_owner.inl"

bool ApplyPendingSceneReplacement(VulkanSurfaceContext& context,
                                  horde::vulkan::DeviceCapabilities& capabilities,
                                  std::vector<double>& timingSamples)
{
    if (!context.sceneProfileDirty && !context.glassGeometryDirty) return true;
    if (!context.useRtPath || !context.rtScene.IsReady()) return false;

    const auto previousProfile = context.rtScene.Profile();
    const auto previousBackend = context.rtScene.ExecutionBackend();
    auto previousSettings = context.graphicsCommand ? context.graphicsBeforeApply : CurrentGraphicsSettings(context);
    previousSettings.glassEnabled = context.rtScene.GlassEnabled();
    const bool restoringShowcase = context.graphicsSceneRestoring;
    const auto retireSceneReferences = [&]() -> bool
    {
        if (!ConsumePendingImageAcquire(context)) return false;
        const VkResult idleResult = vkDeviceWaitIdle(context.device);
        if (idleResult != VK_SUCCESS || !CompleteRtEvidenceAfterDeviceIdle(context, idleResult)) return false;
        // Drop every recorded reference before destroying the single active scene.
        for (const auto commandBuffer : context.commandBuffers)
            if (vkResetCommandBuffer(commandBuffer, 0u) != VK_SUCCESS) return false;
        context.gpuFrameTimer.ResetAfterDeviceIdle();
#if HORDE_RT_FRAME_STAGE_PROBE
        ResetWindowsGpuStagesAfterIdle(context);
#endif
#if HORDE_RT_STAGED_PRIMARY_TIMING
        context.stagedPassTimer.ResetAfterDeviceIdle();
#endif
        context.gpuFrameTimingTotalMs = 0.0;
        context.gpuFrameTimingSampleCount = 0u;
        RefreshGpuTimingTelemetry(context);
        return !context.rtFrameEvidenceInitialised || context.rtFrameEvidence.Recreate(
            horde::telemetry::RtResourceResetReason::DiagnosticResourceReplacement,
            CurrentInitialGpuEvidenceStatus(context));
    };
    context.benchmarkEvidence.Cancel();
    if (!retireSceneReferences()) return false;
    context.simulation.InvalidateWorldZoneReadiness();
    context.rtScene.Destroy(); // Exactly one scene; shared simulation remains paused and CPU-owned.
    if (!InitialiseRtSceneForSwapchain(context, false))
    {
        const std::string requestedFailure = context.lastRtFrameError;
        // Partial initialisation may have submitted work. A fresh proof is required
        // before its cleanup and before recreating the prior complete configuration.
        if (!retireSceneReferences()) return false;
        context.simulation.InvalidateWorldZoneReadiness();
        context.rtScene.Destroy();
        context.sceneProfile = previousProfile;
        context.executionBackend = previousBackend;
        context.renderScale = previousSettings.renderScalePercent / 100.0f;
        context.waterQuality = static_cast<horde::vulkan::raytracing::WaterQuality>(previousSettings.waterQuality);
        context.fireDetail = previousSettings.fireDetail;
        context.shadowQuality = previousSettings.shadowQuality;
        context.graphicsPreviewFrameCap = previousSettings.previewFrameCap;
        context.requestedGlassEnabled = previousSettings.glassEnabled;
        context.requestedMistEnabled = previousSettings.mistEnabled;
        context.requestedDustQuality = previousSettings.dustQuality;
        if (!InitialiseRtSceneForSwapchain(context, false)) return false;
        if (context.graphicsCommand && context.graphicsEdit)
        {
            auto failure = GraphicsSnapshot(context, *context.graphicsCommand);
            failure.reasons = horde::graphics::GraphicsReason::ResourceFailure;
            // The old tuple is restored but has not presented its own frame yet.
            failure.rtPresented = false;
            context.graphicsEdit->Acknowledge(failure, false);
            context.graphicsCommand.reset();
        }
        if (restoringShowcase)
        {
            context.graphicsSceneRestoring = false;
            context.graphicsCloseAfterRevert = false;
        }
        context.graphicsStatus = "Requested scene could not load. Previous graphics and scene restored; saved settings and pending recovery remain unchanged.";
        context.lastRtFrameError = requestedFailure;
        std::cerr << "RT scene replacement rolled back: " << requestedFailure << '\n';
    }
    else context.lastRtFrameError.clear();

    // A combined scale/profile/glass request has already rebuilt the target output.
    context.renderScaleDirty = false;
    context.sceneProfileDirty = false;
    context.glassGeometryDirty = false;
    context.lastFramePresentation = horde::telemetry::RtPresentationOutcome::NotAttempted;
    context.graphicsPreview.Reset();
    context.graphicsPreviewLastFrame = {}; context.graphicsPreviewLastSample = {};
    context.graphicsPreviewPerformance.BeginScope(++context.graphicsPreviewEpoch);
    context.graphicsPreviewLastGpuSample = 0u;
    capabilities.rtScene.presented = false;
    capabilities.rtScene.dispatchWidth = 0u; capabilities.rtScene.dispatchHeight = 0u;
    capabilities.performance.internalRenderWidth = 0u; capabilities.performance.internalRenderHeight = 0u;
    capabilities.performance.frameTimeMs = 0.0f; capabilities.performance.fps = 0.0f;
    timingSamples.clear();
    const bool previewReady = context.rtScene.Profile() == horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview;
    for (const int id : {kGraphicsPreviewPauseId, kGraphicsPreviewCameraId, kGraphicsPreviewMotionId, kGraphicsPreviewResetId})
        EnableWindow(GetDlgItem(context.windowHandle, id), previewReady);
    SetControlVisible(context.windowHandle, kGraphicsPreviewGraphId, context.graphicsVisible && previewReady);
    UpdateSettingsLabels(context);
    return true;
}

void SetEntryLoadingVisible(VulkanSurfaceContext& context, const bool visible)
{
    context.entryLoadingVisible = visible;
    if (HWND spinner = GetDlgItem(context.windowHandle, kEntryLoadingIndicatorId))
    {
        ShowWindow(spinner, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
        KillTimer(context.windowHandle, kEntryLoadingIndicatorId);
        if (visible)
        {
            BOOL animations = TRUE;
            if (SystemParametersInfoA(SPI_GETCLIENTAREAANIMATION, 0, &animations, 0) && animations)
                SetTimer(context.windowHandle, kEntryLoadingIndicatorId, 100u, nullptr);
            InvalidateRect(spinner, nullptr, FALSE);
            // Paint once before synchronous owner-thread scene construction.
            // The spinner remains visible during that real loading state.
            UpdateWindow(spinner);
        }
    }
}

void FinishEntryLoadingAfterPresentation(VulkanSurfaceContext& context, const bool presented)
{
    if (context.entryLoadingVisible && presented &&
        context.rtScene.Profile() == horde::vulkan::raytracing::RtSceneProfile::Showcase)
        SetEntryLoadingVisible(context, false);
}

void QueuePresentedEntryPlay(VulkanSurfaceContext& context, const bool presented)
{
    if (!context.entryPlayQueued || !context.entryMenu.ReadyToPlay() || !presented)
        return;
    context.entryPlayQueued = false;
    context.entryPlayHandoffPending = true;
    context.entryMenuVisible = false;
    context.entryMoreVisible = false;
    context.sceneProfile = horde::vulkan::raytracing::RtSceneProfile::Showcase;
    context.sceneProfileDirty = true;
    SetEntryLoadingVisible(context, true);
    // Keep gameplay paused while the presented black frame covers loading.
}

void FinishPendingEntryPlay(VulkanSurfaceContext& context)
{
    if (!context.entryPlayHandoffPending)
        return;
    context.entryPlayHandoffPending = false;
    const bool loaded = context.rtScene.IsReady() &&
        context.rtScene.Profile() == horde::vulkan::raytracing::RtSceneProfile::Showcase;
    if (!loaded)
    {
        SetEntryLoadingVisible(context, false);
        context.entryMenuVisible = true;
        context.entryMenu.Reset();
    }
    for (const int id :
         {kPauseTitleId, kResumeButtonId, kSettingsButtonId, kEntryMoreButtonId})
        EnableWindow(GetDlgItem(context.windowHandle, id), TRUE);
    ApplyDpiScaledFonts(context.windowHandle);
    ShowPauseMenu(context, !loaded);
}

bool ApplyPendingOutputResize(VulkanSurfaceContext& context,
                              horde::vulkan::DeviceCapabilities& capabilities,
                              std::vector<double>& timingSamples)
{
    const HWND hWnd = context.windowHandle;
        if (context.renderScaleDirty && context.useRtPath)
        {
            const auto resizeStart = std::chrono::steady_clock::now();
            const float requestedRenderScale = context.renderScale;
            context.benchmarkEvidence.Cancel();
            context.renderScaleDirty = false;
            timingSamples.clear();
            const VkResult idleResult = vkDeviceWaitIdle(context.device);
            const bool evidenceCompleted =
                CompleteRtEvidenceAfterDeviceIdle(context, idleResult);
            if (idleResult != VK_SUCCESS || !evidenceCompleted)
            {
                return false;
            }
            const bool evidenceRecreated = !context.rtFrameEvidenceInitialised ||
                context.rtFrameEvidence.Recreate(
                    horde::telemetry::RtResourceResetReason::RenderScaleChange,
                    CurrentInitialGpuEvidenceStatus(context));
            context.gpuFrameTimer.ResetAfterDeviceIdle();
#if HORDE_RT_FRAME_STAGE_PROBE
            ResetWindowsGpuStagesAfterIdle(context);
#endif
#if HORDE_RT_STAGED_PRIMARY_TIMING
            if (idleResult == VK_SUCCESS) context.stagedPassTimer.ResetAfterDeviceIdle();
#endif
            context.gpuFrameTimingTotalMs = 0.0;
            context.gpuFrameTimingSampleCount = 0u;
            RefreshGpuTimingTelemetry(context);
            if (HWND hud = GetDlgItem(hWnd, kHudControlId))
            {
                SetWindowTextA(hud, kHudApplyingScaleText);
            }
            if (!evidenceRecreated)
            {
                return false;
            }
            const VkExtent2D requestedExtent = ScaledRenderExtent(
                context.swapchainExtent, requestedRenderScale);
            std::string resizeDiagnostic;
            if (!context.rtScene.ResizeOutputAfterDeviceIdle(requestedExtent, resizeDiagnostic))
            {
                // Allocation/preflight failure retains the old image and descriptor.
                // Restore the saved selection so relaunch does not repeat an unusable choice.
                context.renderScale = context.appliedRenderScale;
                if (context.graphicsCommand && context.graphicsEdit)
                {
                    context.waterQuality = static_cast<horde::vulkan::raytracing::WaterQuality>(context.graphicsBeforeApply.waterQuality);
                    context.fireDetail = context.graphicsBeforeApply.fireDetail;
                    context.shadowQuality = context.graphicsBeforeApply.shadowQuality;
                    context.graphicsPreviewFrameCap = context.graphicsBeforeApply.previewFrameCap;
                    context.requestedGlassEnabled = context.graphicsBeforeApply.glassEnabled;
                    context.requestedMistEnabled = context.graphicsBeforeApply.mistEnabled;
                    context.requestedDustQuality = context.graphicsBeforeApply.dustQuality;
                    auto failure = GraphicsSnapshot(context, *context.graphicsCommand);
                    failure.reasons = horde::graphics::GraphicsReason::ResourceFailure;
                    context.graphicsEdit->Acknowledge(failure, false);
                    context.graphicsCommand.reset();
                    context.graphicsStatus = "Requested output allocation failed; previous effective graphics were retained. Your saved choice is unchanged.";
                }
                UpdateSettingsLabels(context);
                std::cerr << "RT render scale was restored after resize failure: "
                          << resizeDiagnostic << '\n';
                if (!context.unattendedBenchmark && !context.outputResizeValidation) GameMessageBoxA(hWnd,
                    ("The requested render resolution could not be applied. Your previous setting was restored.\n\n" +
                     resizeDiagnostic).c_str(), "Horde Lantern RT - graphics", MB_OK | MB_ICONWARNING);
            }
            else
            {
                context.appliedRenderScale = requestedRenderScale;
                capabilities.rtScene.presented = false;
                capabilities.rtScene.dispatchWidth = 0u;
                capabilities.rtScene.dispatchHeight = 0u;
                capabilities.performance.internalRenderWidth = 0u;
                capabilities.performance.internalRenderHeight = 0u;
                capabilities.performance.frameTimeMs = 0.0f;
                capabilities.performance.fps = 0.0f;
            }
            // RenderFrame resets/re-records each acquired command buffer before
            // submission; no commands referencing the retired image are reused.
            const double resizeMilliseconds = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - resizeStart).count();
#if defined(_DEBUG)
            context.lastOutputResizeIdleMilliseconds = resizeMilliseconds;
#endif
            const auto effectiveExtent = context.rtScene.DispatchExtent();
            std::cout << "HORDE_RT_SCALE_RESIZE scale=" << std::round(context.appliedRenderScale * 100.0f)
                      << " extent=" << effectiveExtent.width << 'x' << effectiveExtent.height
                      << " output_only=1 applied=" << resizeDiagnostic.empty()
                      << " idle_and_resize_ms=" << resizeMilliseconds << '\n';
        }
    return true;
}

bool RenderFrame(VulkanSurfaceContext& ctx, const VkClearColorValue& clearColor, bool& rtFramePresented)
{
#if defined(_DEBUG) || HORDE_RT_CHAPTER_MEASUREMENT
    ctx.lastSuccessfulPresentReturnSteadyNs = 0;
#endif
    const std::uint64_t frameStartNanoseconds =
        horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr);
    ctx.lastFramePresentation = horde::telemetry::RtPresentationOutcome::NotAttempted;
    if (ctx.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Measuring)
    {
        const auto state = ctx.rtFrameEvidence.PublishedStateByValue();
        if (!state.running || state.paused ||
            state.sceneEpoch != ctx.benchmarkEvidence.SceneEpoch() ||
            state.measurementGeneration != ctx.benchmarkEvidence.MeasurementGeneration())
            ctx.benchmarkEvidence.Cancel();
    }
    rtFramePresented = false;
    if (ctx.commandBuffers.empty())
    {
        return false;
    }

#if defined(_DEBUG)
    if (ctx.staticReplacementFrameRequested)
    {
        // Drain/reset all prior references BEFORE beginning this frame, its
        // evidence token or command buffer. The recorder callback only verifies
        // this owner-thread proof; it must not reset an already-begun command.
        const auto generation = ctx.rtScene.StaticGeometryGeneration();
        if (generation != ctx.staticReplacementExpectedGeneration ||
            !ctx.rtScene.StaticGeometryReplacementReady() || !ConsumePendingImageAcquire(ctx))
        { ctx.lastRtFrameError="Moving-frame static replacement is stale or incomplete."; return false; }
        const VkResult idle=vkDeviceWaitIdle(ctx.device);
        if (!CompleteRtEvidenceAfterDeviceIdle(ctx,idle))
        { ctx.lastRtFrameError="Moving-frame prior GPU completion is unproved."; return false; }
        for (const auto command : ctx.commandBuffers)
            if (vkResetCommandBuffer(command,0u)!=VK_SUCCESS)
            { ctx.lastRtFrameError="Moving-frame prior command references could not be reset."; return false; }
        ctx.gpuFrameTimer.ResetAfterDeviceIdle();
#if HORDE_RT_FRAME_STAGE_PROBE
        ResetWindowsGpuStagesAfterIdle(ctx);
#endif
#if HORDE_RT_STAGED_PRIMARY_TIMING
        ctx.stagedPassTimer.ResetAfterDeviceIdle();
#endif
        ctx.gpuFrameTimingTotalMs=0;ctx.gpuFrameTimingSampleCount=0;
        RefreshGpuTimingTelemetry(ctx);ctx.benchmarkEvidence.Cancel();
        if (ctx.rtFrameEvidenceInitialised && !ctx.rtFrameEvidence.Recreate(
                horde::telemetry::RtResourceResetReason::DiagnosticResourceReplacement,
                CurrentInitialGpuEvidenceStatus(ctx)))
        { ctx.lastRtFrameError="Moving-frame evidence retirement failed."; return false; }
        ctx.staticReplacementProofGeneration=generation;
    }
#endif
    const bool useRtFrame = ctx.useRtPath && ctx.rtScene.IsReady();
    if (!ctx.environmentSurveyActive && useRtFrame && ctx.rtScene.Profile() ==
            horde::vulkan::raytracing::RtSceneProfile::Showcase)
    {
        SynchronizeSimulationViewportAspect(ctx);
    }
    horde::vulkan::raytracing::RtSceneRecordObservation observation{};
    const bool evidenceFrame = useRtFrame && ctx.rtFrameEvidenceInitialised &&
        ctx.rtFrameEvidence.BeginFrame(ctx.currentFrame, observation);
    horde::vulkan::raytracing::RtSceneStageScope wholeFrameScope(
        evidenceFrame ? &observation : nullptr,
        horde::telemetry::RtStage::WholeFrameCycle,
        frameStartNanoseconds);

    horde::vulkan::raytracing::RtSceneStageScope fenceScope(
        evidenceFrame ? &observation : nullptr,
        horde::telemetry::RtStage::FrameFenceWait);
    const VkResult waitResult = vkWaitForFences(
        ctx.device, 1u, &ctx.inFlightFences[ctx.currentFrame], VK_TRUE,
        (gCampaignHiddenValidation || ctx.environmentSurveyActive || ctx.graphicsPreviewCapture || ctx.outputResizeValidation || ctx.nativeMotionValidation) ? 2'000'000'000ull : UINT64_MAX);
    fenceScope.Complete(1u, 0u, 1u);
    if (waitResult != VK_SUCCESS)
    {
        if (evidenceFrame)
        {
            ctx.rtFrameEvidence.AbortFrame();
        }
        return false;
    }

    if (evidenceFrame)
    {
        horde::telemetry::RtPerformanceEvidenceSnapshot snapshot{};
        const horde::vulkan::raytracing::RtFrameEvidenceCompletionResult completion =
            ctx.rtFrameEvidence.CompleteFence(
                ctx.currentFrame,
                horde::vulkan::raytracing::MakeRtGpuFrameTimerIo(ctx.gpuFrameTimer),
                horde::vulkan::raytracing::MakeRtDiagnosticFrameIo(ctx.rtScene), &snapshot);
#if HORDE_RT_FRAME_STAGE_PROBE
        CollectWindowsGpuStages(ctx,ctx.currentFrame,completion,snapshot);
#endif
#if HORDE_RT_STAGED_PRIMARY_TIMING
        CollectStagedPassTiming(ctx, ctx.currentFrame, completion, snapshot);
#endif
        AcceptBenchmarkCompletion(ctx, completion, snapshot);
#if defined(_DEBUG)
        if (ctx.nativeMotionValidation &&
            !ctx.motionLedger.AppendCompletedFrame(ctx.motionSurfaceGeneration, ctx.rtFrameEvidence.PublishedStateByValue()))
            ctx.motionScenario.Fail(ctx.motionLedger.Failure());
#endif
        RefreshGpuTimingTelemetry(
            ctx, completion.gpuCollectionAttempted ? &completion.gpuCollection : nullptr);
        if (completion.fatalDiagnosticIoFailure)
        {
            ctx.lastRtFrameError =
                "Failed to read the completed RT Diagnostic buffer after its owning fence.";
            ctx.rtFrameEvidence.AbortFrame();
            return false;
        }
    }

    uint32_t imageIndex = 0u;
    horde::vulkan::raytracing::RtSceneStageScope acquireScope(
        evidenceFrame ? &observation : nullptr,
        horde::telemetry::RtStage::ImageAcquire);
    const VkResult acquireResult = vkAcquireNextImageKHR(
        ctx.device,
        ctx.swapchain,
        (gCampaignHiddenValidation || ctx.environmentSurveyActive || ctx.graphicsPreviewCapture || ctx.outputResizeValidation || ctx.nativeMotionValidation) ? 2'000'000'000ull : UINT64_MAX,
        ctx.imageAvailableSemaphores[ctx.currentFrame],
        VK_NULL_HANDLE,
        &imageIndex);
    acquireScope.Complete(1u, 0u, 1u);

    ctx.imageAcquirePending = acquireResult == VK_SUCCESS || acquireResult == VK_SUBOPTIMAL_KHR;

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        if (evidenceFrame)
        {
            ctx.rtFrameEvidence.AbortFrame();
        }
        return !ctx.environmentSurveyActive && RecreateSwapchain(ctx);
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
    {
        if (evidenceFrame)
        {
            ctx.rtFrameEvidence.AbortFrame();
        }
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
        if (evidenceFrame)
        {
            ctx.rtFrameEvidence.AbortFrame();
        }
        return false;
    }

    bool gpuTimingRecording = false;
#if HORDE_RT_FRAME_STAGE_PROBE
    horde::vulkan::raytracing::RtSceneGpuStageProbe frameStageProbe(
        {{ctx.captureGpuStages?&ctx.frameStageTimers[0]:nullptr,
          ctx.captureGpuStages?&ctx.frameStageTimers[1]:nullptr,
          ctx.captureGpuStages?&ctx.frameStageTimers[2]:nullptr}},ctx.currentFrame);
#endif
#if HORDE_RT_STAGED_PRIMARY_TIMING
    horde::vulkan::raytracing::experimental::StagedPrimaryRecordingGuard stagedRecordingGuard(ctx.stagedPassTimer, ctx.currentFrame);
#endif
    if (useRtFrame)
    {
        if (!gCampaignHiddenValidation && !ctx.environmentSurveyActive && !ctx.graphicsPreviewCapture && !ctx.outputResizeValidation && !ctx.nativeMotionValidation)
        {
            SpatialAudioEngine().Update();
            const auto completion = SpatialAudioEngine().TakeDialogueCompletion();
            const auto& activeDialogue = ctx.simulation.Snapshot().chapterDialogue;
            if (completion.first != 0u && completion.first == static_cast<std::uint32_t>(activeDialogue.line) &&
                completion.second == activeDialogue.generation)
            {
                ctx.dialogueCompletionLine = completion.first;
                ctx.dialogueCompletionGeneration = completion.second;
            }
        }
        const bool frozenDevelopmentCheckpoint = ctx.environmentSurveyActive ||
            (ctx.simulationPaused && ctx.frameDeltaSeconds == 0.0f &&
            !ctx.developmentCheckpoint.empty());
        const bool entryFrame =
            ctx.rtScene.Profile() == horde::vulkan::raytracing::RtSceneProfile::EntryMenu;
        const bool previewFrame =
            ctx.rtScene.Profile() != horde::vulkan::raytracing::RtSceneProfile::Showcase;
        if (!frozenDevelopmentCheckpoint && !previewFrame && !ctx.outputResizeValidation && !ctx.chapterRoundTripCaptureActive)
        {
            UpdateDesktopSceneControls(ctx, evidenceFrame ? &observation : nullptr);
        }
        if (!gCampaignHiddenValidation && !ctx.environmentSurveyActive && !previewFrame && !ctx.outputResizeValidation && !ctx.nativeMotionValidation) UpdateWaterfallAmbience(ctx);
        const horde::gameplay::simulation::SimulationSnapshot& simulation =
#if defined(_DEBUG)
            ctx.playerDodgeInspectionSnapshot ? *ctx.playerDodgeInspectionSnapshot :
#endif
            ctx.simulation.Snapshot();
        if(!ctx.environmentSurveyActive)UpdateChapterDialogueAudio(ctx);
        if(!ctx.environmentSurveyActive) {
            RefreshChapterSubtitle(ctx);
            UpdateChestPrompt(ctx);
        }
        if (!ctx.environmentSurveyActive && !previewFrame && !ctx.outputResizeValidation && !ctx.nativeMotionValidation && simulation.playerVitals.phase == horde::gameplay::PlayerLifePhase::Dead)
        {
            ShowDeathMenu(ctx);
        }
        if (!ctx.environmentSurveyActive && !previewFrame && !ctx.outputResizeValidation && !ctx.nativeMotionValidation && !ctx.graphicsVisible && simulation.finaleComplete &&
            (!ctx.benchmark.HasStarted() || ctx.benchmarkCompletionHandled))
        {
            TryGrantRtLabUnlock(ctx, true);
            ShowEndingMenu(ctx);
        }
        if (!ctx.environmentSurveyActive && !ctx.graphicsPreviewCapture && !ctx.outputResizeValidation && !ctx.nativeMotionValidation) PublishMusicPlayback(ctx);
        if (!ctx.environmentSurveyActive && !previewFrame && !ctx.outputResizeValidation) DrainGameplayEvents(ctx);
        if(!ctx.environmentSurveyActive)
            horde::vulkan::raytracing::PublishDevelopmentWorldReadiness(ctx.simulation,ctx.rtScene);
        ctx.campaignFrameTick=simulation.tickIndex;ctx.campaignFrameWorld=simulation.worldRoute.generation;
        horde::vulkan::raytracing::RtSceneFrameInputs frameInputs =
            horde::vulkan::raytracing::BuildRtSceneFrameInputs(
                simulation, ctx.outputExposure, ctx.waterQuality, ctx.rtSceneTuning,
                horde::vulkan::raytracing::ResolveFireEmitterQuality(ctx.fireDetail));
#if defined(_DEBUG)
        if (ctx.inspectionSkeletonBlends) {
            if (!ctx.playerDodgeInspectionSnapshot || !frozenDevelopmentCheckpoint ||
                !gShowcaseCaptureHidden || IsWindowVisible(ctx.windowHandle) ||
                GetForegroundWindow() == ctx.windowHandle || GetFocus() == ctx.windowHandle) {
                ctx.lastRtFrameError = "Skeleton clip blend requires the immutable hidden pose fixture.";
                if (evidenceFrame) ctx.rtFrameEvidence.FailRecord(observation);
                return false;
            }
            frameInputs.skeletonPoseBlends = *ctx.inspectionSkeletonBlends;
        }
#endif
        if (entryFrame)
        {
            ctx.entryMenu.Advance(ctx.graphicsPreviewDelta);
            frameInputs = horde::vulkan::raytracing::BuildEntryMenuFrameInputs(
                ctx.entryMenu, ctx.outputExposure,
                horde::vulkan::raytracing::ResolveFireEmitterQuality(ctx.fireDetail),
                ctx.shadowQuality);
        }
        else if (previewFrame)
        {
            ctx.graphicsPreview.Advance(ctx.graphicsPreviewDelta);
            frameInputs = horde::vulkan::raytracing::BuildGraphicsPreviewFrameInputs(
                ctx.graphicsPreview, ctx.outputExposure, ctx.waterQuality,
                horde::vulkan::raytracing::ResolveFireEmitterQuality(ctx.fireDetail));
#if defined(_DEBUG)
            if(ctx.previewMeshPairCamera) {
                frameInputs.cameraX=ctx.previewMeshPairCamera->x;frameInputs.cameraZ=ctx.previewMeshPairCamera->z;
                frameInputs.cameraYaw=ctx.previewMeshPairCamera->yaw;frameInputs.cameraPitch=ctx.previewMeshPairCamera->pitch;
            }
#endif
        }
        if(!ctx.environmentSurveyActive)UpdateMenuAmbience(ctx);
        frameInputs.shadowQuality = ctx.shadowQuality;
#if defined(_DEBUG)
        if(frozenDevelopmentCheckpoint && simulation.developmentRescueJourney) {
            frameInputs.diagnosticSuppressFinaleApertureLighting = ctx.skyAttribution ==
                horde::platform::windows::SkyAttributionMode::DirectionalOnly;
            if(ctx.skyAttribution==horde::platform::windows::SkyAttributionMode::Disabled)
                frameInputs.tuning.lights[static_cast<std::size_t>(
                    horde::vulkan::raytracing::RtLightGroup::Skylight)].intensityScale=0.0f;
        }
#endif
        ctx.rtScene.SetMistEnabled(ctx.requestedMistEnabled);
        ctx.rtScene.SetDustQuality(ctx.requestedDustQuality);
#if defined(_DEBUG)
        // The separately named Max motion diagnostic preserves its explicit
        // legacy whole-workload policy. Ordinary Graphics/RT Lab do not opt in.
        if (ctx.nativeMotionValidation && ctx.motionRequestedRtPreset == horde::vulkan::raytracing::RtWorkloadPreset::Max)
            frameInputs.shadowQuality = std::nullopt;
#endif
        // Only explicit diagnostic comparisons opt out of the accepted
        // modelled production presentation; gameplay and glass share one route.
        const horde::gameplay::DevelopmentCheckpoint* development =
            horde::gameplay::FindDevelopmentCheckpoint(ctx.developmentCheckpoint);
        const bool usesGlassFixture =
            development != nullptr && development->usesGlassFixture;
        const bool usesProductionRewardProps =
            development != nullptr && development->usesProductionRewardProps;
        frameInputs.playerRenderRoute = horde::vulkan::raytracing::PlayerRenderRouteForCheckpoint(
            ctx.developmentCheckpoint);
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
#if defined(_DEBUG)
        if(ctx.lookoutKitInspectionPose) {
            // Explicit hidden pose inspection only. Reuse the coherent camera/
            // attachment adapter without the exterior survey's fog override.
            if(!gCampaignHiddenValidation || !gLookoutHiddenValidation || !gLookoutInspectKit ||
               !horde::vulkan::raytracing::RebaseEnvironmentSurveyFrame(frameInputs,*ctx.lookoutKitInspectionPose,diagnostic)) {
                ctx.lastRtFrameError="Hidden Kit inspection rebase rejected: "+diagnostic;
                if(evidenceFrame)ctx.rtFrameEvidence.FailRecord(observation);
                return false;
            }
            if(ctx.lookoutKitInspectionMotion)frameInputs.chapterCompanion=*ctx.lookoutKitInspectionMotion;
            frameInputs.chapterCompanion.acting=ctx.lookoutKitInspectionActing;
            if(frameInputs.chapterCompanion.motion.turnStartEnabled && ctx.lookoutKitInspectionTurn) {
                ctx.lastRtFrameError="Shared Kit motion cannot mix with manual clip inspection.";
                if(evidenceFrame)ctx.rtFrameEvidence.FailRecord(observation);
                return false;
            }
            if(ctx.lookoutKitInspectionTurn) {
                if(!gLookoutInspectKitTurns || !ctx.simulationPaused) {ctx.lastRtFrameError="Hidden turn playback requires the isolated paused fixture.";return false;}
                frameInputs.diagnosticChapterActor=ctx.lookoutKitInspectionTurn;
            }
        }
        if(ctx.environmentSurveyActive && ctx.environmentSurveyProfile!="player-dodge")
        {
            const auto& camera=ctx.environmentSurveyCamera;
            if(!horde::vulkan::raytracing::RebaseEnvironmentSurveyFrame(frameInputs,
                {camera.x,camera.z,camera.supportY,camera.yaw,camera.pitch,ctx.environmentSurveyTime},diagnostic)) {
                ctx.lastRtFrameError=diagnostic;
                if(evidenceFrame)ctx.rtFrameEvidence.FailRecord(observation);
                return false;
            }
            // Render-only exterior survey: no simulated pose/progress import.
            if(ctx.environmentSurveyProfile=="river-bend")frameInputs.tuning.fogDensityScale=.004f;
        }
#endif
        if (evidenceFrame)
        {
            (void)ctx.rtFrameEvidence.BeginRecord(frameInputs.tickIndex);
        }
        // This owner waited its sole graphics fence above. No scene descriptor
        // has been bound in the current recording; reset every older recording
        // before a binding change can invalidate its references.
        struct ActorRetirementOwner { VulkanSurfaceContext* context; std::uint32_t image; };
        ActorRetirementOwner actorOwner{&ctx, imageIndex};
        const auto retireResourceReferences=[](void* user, std::string& error) {
                    auto& owner = *static_cast<ActorRetirementOwner*>(user);
                    auto& context = *owner.context;
                    if (context.inFlightFences.size() != 1u) {
                        error = "Character retirement requires the single completed graphics owner.";
                        return false;
                    }
                    for (std::size_t i = 0; i < context.commandBuffers.size(); ++i)
                        if (i != owner.image && vkResetCommandBuffer(context.commandBuffers[i], 0u) != VK_SUCCESS) {
                            error = "Prior character command references could not be reset.";
                            return false;
                        }
                    return true;
                };
        if (!ctx.rtScene.PrepareThirdActorResourceLifetime(frameInputs,
                retireResourceReferences, &actorOwner, diagnostic, evidenceFrame ? &observation : nullptr)) {
            ctx.lastRtFrameError = diagnostic;
            if (evidenceFrame) ctx.rtFrameEvidence.FailRecord(observation);
            return false;
        }
        using DetailFrame=horde::vulkan::raytracing::PresentableTinyRtScene::StaticDetailFrame;
        DetailFrame detail=DetailFrame::Render;
        if(!ctx.environmentSurveyActive && !ctx.rtScene.AdvanceOrdinaryStaticDetail(ctx.simulation,frameInputs,
            !previewFrame && !frozenDevelopmentCheckpoint && !ctx.nativeMotionValidation && !ctx.outputResizeValidation &&
            !ctx.graphicsPreviewCapture && !ctx.benchmark.IsRunning(),detail,diagnostic)) {
            ctx.lastRtFrameError=diagnostic;
            if(evidenceFrame)ctx.rtFrameEvidence.FailRecord(observation);
            return false;
        }
        if(detail==DetailFrame::WaitForRestore) {
            if(evidenceFrame)ctx.rtFrameEvidence.AbortFrame();
            if(vkResetCommandBuffer(ctx.commandBuffers[imageIndex],0u)!=VK_SUCCESS)return false;
            return ConsumePendingImageAcquire(ctx);
        }
        gpuTimingRecording = evidenceFrame && ctx.gpuFrameTimer.RecordBegin(
            ctx.commandBuffers[imageIndex], ctx.currentFrame);
        bool recorded=false;
        if(detail==DetailFrame::Cutover) {
            recorded=ctx.rtScene.RecordTraceAndCopyWithStaticReplacement(ctx.rtScene.StaticGeometryGeneration(),
                retireResourceReferences,&actorOwner,ctx.commandBuffers[imageIndex],ctx.swapchainImages[imageIndex],
                ctx.swapchainImageLayouts[imageIndex],ctx.swapchainExtent,frameInputs,diagnostic,
                evidenceFrame?&observation:nullptr
#if HORDE_RT_STAGED_PRIMARY_TIMING
                ,gpuTimingRecording?&ctx.stagedPassTimer:nullptr,ctx.currentFrame
#endif
#if HORDE_RT_FRAME_STAGE_PROBE
                ,gpuTimingRecording && ctx.captureGpuStages?&frameStageProbe:nullptr
#endif
            );
            if(recorded && !ctx.environmentSurveyActive)ctx.rtScene.FinishOrdinaryStaticDetail(ctx.simulation);
        }else {
#if defined(_DEBUG)
        recorded = ctx.staticReplacementFrameRequested
            ? ctx.rtScene.RecordTraceAndCopyWithStaticReplacement(
                ctx.staticReplacementExpectedGeneration,
                [](void* user,std::string& error) {
                    const auto& owner=*static_cast<VulkanSurfaceContext*>(user);
                    if (!owner.staticReplacementFrameRequested ||
                        owner.staticReplacementProofGeneration!=owner.rtScene.StaticGeometryGeneration())
                    { error="Moving-frame retirement proof became stale before record.";return false; }
                    return true;
                }, &ctx, ctx.commandBuffers[imageIndex],ctx.swapchainImages[imageIndex],
                ctx.swapchainImageLayouts[imageIndex],ctx.swapchainExtent,frameInputs,diagnostic,
                evidenceFrame ? &observation : nullptr
#if HORDE_RT_STAGED_PRIMARY_TIMING
                , gpuTimingRecording ? &ctx.stagedPassTimer : nullptr,ctx.currentFrame
#endif
#if HORDE_RT_FRAME_STAGE_PROBE
                ,gpuTimingRecording && ctx.captureGpuStages?&frameStageProbe:nullptr
#endif
              )
            : ctx.rtScene.RecordTraceAndCopy(ctx.commandBuffers[imageIndex],ctx.swapchainImages[imageIndex],
                ctx.swapchainImageLayouts[imageIndex],ctx.swapchainExtent,frameInputs,diagnostic,
                evidenceFrame ? &observation : nullptr
#if HORDE_RT_STAGED_PRIMARY_TIMING
                , gpuTimingRecording ? &ctx.stagedPassTimer : nullptr,ctx.currentFrame
#endif
#if HORDE_RT_FRAME_STAGE_PROBE
                ,gpuTimingRecording && ctx.captureGpuStages?&frameStageProbe:nullptr
#endif
              );
        if (recorded && ctx.staticReplacementFrameRequested)
        { ctx.staticReplacementFrameRequested=false;ctx.staticReplacementProofGeneration=0; }
#else
        recorded = ctx.rtScene.RecordTraceAndCopy(ctx.commandBuffers[imageIndex],
                                            ctx.swapchainImages[imageIndex],
                                            ctx.swapchainImageLayouts[imageIndex],
                                            ctx.swapchainExtent,
                                            frameInputs,
                                            diagnostic,
                                           evidenceFrame ? &observation : nullptr
#if HORDE_RT_STAGED_PRIMARY_TIMING
                                           , gpuTimingRecording ? &ctx.stagedPassTimer : nullptr, ctx.currentFrame
#endif
#if HORDE_RT_FRAME_STAGE_PROBE
                ,gpuTimingRecording && ctx.captureGpuStages?&frameStageProbe:nullptr
#endif
                                           );
#endif
        }
        if (!recorded)
        {
            if (evidenceFrame)
            {
                ctx.rtFrameEvidence.FailRecord(observation);
            }
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
        if (evidenceFrame)
        {
            (void)ctx.rtFrameEvidence.FinishRecord(observation);
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
        if (evidenceFrame)
        {
            ctx.rtFrameEvidence.FailGraphicsSubmit(
                gpuTimingRecording,
                horde::vulkan::raytracing::MakeRtGpuFrameTimerIo(ctx.gpuFrameTimer));
        }
        else if (gpuTimingRecording)
        {
            ctx.gpuFrameTimer.CancelRecording(ctx.currentFrame);
        }
        return false;
    }

    const horde::vulkan::raytracing::RtEvidenceSubmitTransaction submitTransaction =
        evidenceFrame
        ? ctx.rtFrameEvidence.PrevalidateSubmit()
        : horde::vulkan::raytracing::RtEvidenceSubmitTransaction{};
    if (vkResetFences(ctx.device, 1u, &ctx.inFlightFences[ctx.currentFrame]) != VK_SUCCESS)
    {
        if (evidenceFrame)
        {
            ctx.rtFrameEvidence.FailGraphicsSubmit(
                gpuTimingRecording,
                horde::vulkan::raytracing::MakeRtGpuFrameTimerIo(ctx.gpuFrameTimer));
        }
        else if (gpuTimingRecording)
        {
            ctx.gpuFrameTimer.CancelRecording(ctx.currentFrame);
        }
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
        // Waiting on acquisition orders reuse after this image's previous
        // presentation consumed its wait semaphore (Khronos Vulkan Guide).
        &ctx.renderFinishedSemaphores[imageIndex]};

    horde::vulkan::raytracing::RtSceneStageScope submitScope(
        evidenceFrame ? &observation : nullptr,
        horde::telemetry::RtStage::QueueSubmit);
    const VkResult submitResult = vkQueueSubmit(
        ctx.graphicsQueue, 1u, &submitInfo, ctx.inFlightFences[ctx.currentFrame]);
    submitScope.Complete(1u, 0u, 1u);
    if (submitResult != VK_SUCCESS)
    {
        if (evidenceFrame)
        {
            ctx.rtFrameEvidence.FailGraphicsSubmit(
                gpuTimingRecording,
                horde::vulkan::raytracing::MakeRtGpuFrameTimerIo(ctx.gpuFrameTimer));
        }
        else if (gpuTimingRecording)
        {
            ctx.gpuFrameTimer.CancelRecording(ctx.currentFrame);
        }
        return false;
    }
    ctx.imageAcquirePending = false;
    if (useRtFrame) ctx.rtScene.NotifyFrameSubmitted();
    if (evidenceFrame)
    {
        ctx.rtFrameEvidence.CommitGraphicsSubmit(
            submitTransaction,
            true,
            gpuTimingRecording,
            horde::vulkan::raytracing::MakeRtGpuFrameTimerIo(ctx.gpuFrameTimer));
#if HORDE_RT_FRAME_STAGE_PROBE
        if(gpuTimingRecording && ctx.captureGpuStages) {
            auto& pending=ctx.pendingStageOwners[ctx.currentFrame];pending={};
            if(ctx.rtFrameEvidence.TryGetCommittedIdentity(ctx.currentFrame,pending.submitted))
                for(unsigned i=0;i<ctx.frameStageTimers.size();++i)
                    if(frameStageProbe.recorded[i])
                        (void)ctx.frameStageTimers[i].MarkSubmitted(ctx.currentFrame,pending.submitted.submissionSerial);
        }
#endif
#if defined(_DEBUG)
        if (ctx.nativeMotionValidation)
        {
            horde::telemetry::RtSubmittedFrameIdentity committed{};
            if (!ctx.rtFrameEvidence.TryGetCommittedIdentity(ctx.currentFrame, committed) ||
                !ctx.motionLedger.BindSubmittedFrame(ctx.motionSurfaceGeneration, committed))
                ctx.motionScenario.Fail("Motion frame could not bind its actual committed graphics owner.");
        }
#endif
#if HORDE_RT_STAGED_PRIMARY_TIMING
        horde::telemetry::RtSubmittedFrameIdentity stagedOwner{};
        if (ctx.rtFrameEvidence.TryGetCommittedIdentity(ctx.currentFrame, stagedOwner))
            (void)ctx.stagedPassTimer.MarkSubmitted(ctx.currentFrame, stagedOwner.submissionSerial);
#endif
    }
    if (ctx.expectedBenchmarkFrame &&
        ctx.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Measuring)
    {
        horde::telemetry::RtSubmittedFrameIdentity committed{};
        if (ctx.rtFrameEvidence.TryGetCommittedIdentity(ctx.currentFrame, committed))
            (void)ctx.benchmarkEvidence.BindSubmitted(*ctx.expectedBenchmarkFrame, committed);
        else
            (void)ctx.benchmarkEvidence.RejectExpected(*ctx.expectedBenchmarkFrame,
                horde::telemetry::RtBenchmarkFailureReason::TokenlessCompletion);
    }

    if (ctx.presentCompletionFences.Prepare(imageIndex) != VK_SUCCESS) return false;
    const VkFence presentFence = ctx.presentCompletionFences.Fence(imageIndex);
    VkSwapchainPresentFenceInfoEXT presentFenceInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT};
    presentFenceInfo.swapchainCount = 1u;
    presentFenceInfo.pFences = &presentFence;
    VkPresentInfoKHR presentInfo{
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        presentFence != VK_NULL_HANDLE ? &presentFenceInfo : nullptr,
        1u,
        &ctx.renderFinishedSemaphores[imageIndex],
        1u,
        &ctx.swapchain,
        &imageIndex,
        nullptr};
    horde::vulkan::raytracing::RtSceneStageScope presentScope(
        evidenceFrame ? &observation : nullptr,
        horde::telemetry::RtStage::PresentCall);
    const VkResult presentResult = vkQueuePresentKHR(ctx.graphicsQueue, &presentInfo);
#if defined(_DEBUG) || HORDE_RT_CHAPTER_MEASUREMENT
    if (useRtFrame && presentResult == VK_SUCCESS && acquireResult == VK_SUCCESS)
        ctx.lastSuccessfulPresentReturnSteadyNs = horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr);
#endif
    if (!ctx.environmentSurveyActive && useRtFrame && presentResult == VK_SUCCESS && acquireResult == VK_SUCCESS &&
        ctx.rtScene.Profile() == horde::vulkan::raytracing::RtSceneProfile::Showcase)
    {
        horde::telemetry::RtSubmittedFrameIdentity committed{};
        const auto& current = ctx.simulation.Snapshot();
        if (ctx.rtFrameEvidence.TryGetCommittedIdentity(ctx.currentFrame, committed) &&
            committed.frame.simulationTick == current.tickIndex &&
            (ctx.lastChapterPresentationAckTick != current.tickIndex ||
             ctx.lastChapterPresentationAckGeneration != current.chapterDialogue.generation) &&
            ctx.simulation.AcknowledgeChapterPresentation(
                current.tickIndex, current.chapterDialogue.generation))
        {
            ctx.lastChapterPresentationAckTick = current.tickIndex;
            ctx.lastChapterPresentationAckGeneration = current.chapterDialogue.generation;
        }
    }
#if !defined(NDEBUG)
    if (evidenceFrame && useRtFrame && presentResult == VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR &&
        ctx.rtScene.Profile() == horde::vulkan::raytracing::RtSceneProfile::Showcase &&
        ctx.combatTimingTrace.HasReportable(ctx.simulation.Snapshot()))
    {
        horde::telemetry::RtSubmittedFrameIdentity committed{};
        if (ctx.rtFrameEvidence.TryGetCommittedIdentity(ctx.currentFrame, committed))
            ctx.combatTimingTrace.WriteAcceptedPresent(std::cout, ctx.simulation.Snapshot(), committed,
                horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr), true,
                observation.recordedScene ? &observation.recordedScene->player : nullptr);
    }
#endif
#if defined(_DEBUG)
    if (ctx.outputResizeValidation && ctx.resizePresentTimestampArmed &&
        useRtFrame && presentResult == VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
    {
        ctx.resizeFirstPresentNanoseconds = horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr);
        ctx.resizePresentTimestampArmed = false;
    }
#endif
    ctx.presentCompletionFences.Presented(imageIndex, presentResult);
    presentScope.Complete(1u, 0u, 1u);
    wholeFrameScope.Complete(1u, 0u, 1u);

    ctx.lastFramePresentation =
            presentResult == VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR
            ? horde::telemetry::RtPresentationOutcome::Presented
            : (presentResult == VK_SUBOPTIMAL_KHR ||
               (presentResult == VK_SUCCESS && acquireResult == VK_SUBOPTIMAL_KHR)
                ? horde::telemetry::RtPresentationOutcome::PresentedNeedsRecreate
                : presentResult == VK_ERROR_OUT_OF_DATE_KHR
                ? horde::telemetry::RtPresentationOutcome::NotPresentedNeedsRecreate
                : horde::telemetry::RtPresentationOutcome::Failed);
    if (evidenceFrame)
    {
        (void)ctx.rtFrameEvidence.AttachPresentation(ctx.lastFramePresentation);
        ctx.rtFrameEvidence.FinalizeSubmittedFrame(observation);
    }
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR ||
        (presentResult == VK_SUCCESS && acquireResult == VK_SUBOPTIMAL_KHR))
    {
        rtFramePresented = useRtFrame && (presentResult == VK_SUCCESS || presentResult == VK_SUBOPTIMAL_KHR);
        return !ctx.environmentSurveyActive && RecreateSwapchain(ctx);
    }
    if (presentResult != VK_SUCCESS)
    {
        return false;
    }

    rtFramePresented = useRtFrame;
    if (rtFramePresented && ctx.benchmark.IsRunning() && !ctx.benchmarkQualityControlsAtStart &&
        ctx.rtScene.HasUploadedQualityControls())
        ctx.benchmarkQualityControlsAtStart = ctx.rtScene.QualityControls();
    ctx.currentFrame = (ctx.currentFrame + 1u) % kMaxFramesInFlight;
    return true;
}

#include "platform/windows/WindowsBenchmarkProfile.inl"
#include "platform/windows/WindowsOpeningBenchmarkMenu.inl"

#if defined(_DEBUG) || HORDE_RT_CHAPTER_MEASUREMENT
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
    SynchronizeSimulationViewportAspect(context);
    context.simulation.ApplyShowcaseCheckpoint(checkpoint.id);
    ++context.musicResetToken; // Import is an explicit audio discontinuity even at the same tick.
    context.benchmarkEvidence.Cancel();
    if (context.rtFrameEvidenceInitialised)
    {
        (void)context.rtFrameEvidence.ApplyEvent(
            horde::telemetry::RtLifecycleEvent::CheckpointChange);
    }
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
             << "  \"playerMountProfile\": \""
             << (context.simulation.Snapshot().playerMountProfile ==
                     horde::gameplay::items::PlayerMountProfile::AnatomicalBody
                 ? "AnatomicalBody" : "LegacyViewRelative") << "\",\n"
             << "  \"executionBackend\": \""
             << JsonEscape(horde::vulkan::ToString(context.rtScene.ExecutionBackend())) << "\",\n"
             << "  \"selectedRtPipelineBundle\": {\"opaqueFast\": {\"key\": \""
             << JsonEscape(std::string(context.rtScene.SelectedOpaqueFastKey()))
             << "\", \"sha256\": \""
             << JsonEscape(std::string(context.rtScene.SelectedOpaqueFastSha256()))
             << "\"}, \"genericDielectric\": {\"key\": \""
             << JsonEscape(std::string(context.rtScene.SelectedGenericDielectricKey()))
             << "\", \"sha256\": \""
             << JsonEscape(std::string(context.rtScene.SelectedGenericDielectricSha256()))
             << "\"}},\n"
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
             << "  \"executionOrganisation\": " << context.rtScene.ExecutionOrganisationJson() << ",\n"
#endif
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
#if defined(_DEBUG)
             << "  \"forestRouteProof\": " << context.rtScene.ForestRouteProofJson() << ",\n"
#endif
             << "  \"dielectricDiagnostics\": {\"availability\": \""
             << horde::vulkan::raytracing::ToString(
                    context.rtScene.DiagnosticsAvailability())
             << "\", \"available\": "
             << (context.rtScene.DiagnosticsAvailability() ==
                         horde::vulkan::raytracing::RtDiagnosticAvailability::Available
                     ? "true" : "false")
             << ", \"transportOverflowCount\": "
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
             << "  \"dielectricReasonDiagnostics\": {\"availability\": \""
             << horde::vulkan::raytracing::ToString(
                    context.rtScene.DiagnosticsAvailability())
             << "\", \"available\": "
             << (context.rtScene.DiagnosticsAvailability() ==
                         horde::vulkan::raytracing::RtDiagnosticAvailability::Available
                     ? "true" : "false")
             << ", \"secondaryNearSelfHitCount\": "
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
             << "  \"lightAttribution\": \"" << horde::platform::windows::SkyAttributionName(context.skyAttribution) << "\",\n"
             << "  \"error\": " << (error.empty() ? "null" : "\"" + JsonEscape(error) + "\"") << ",\n"
             << "  \"captures\": [\n";
    for (std::size_t index = 0; index < captures.size(); ++index)
    {
        const ShowcaseCaptureRecord& capture = captures[index];
        const auto& checkpoint = *capture.checkpoint;
        std::ostringstream traversalEvidence;
        const auto& rescue=capture.rescue;
        const auto vectorJson=[&](const horde::gameplay::traversal::Vec3& v) {
            traversalEvidence << '[' << v.x << ',' << v.y << ',' << v.z << ']';
        };
        traversalEvidence << "{\"phase\":" << static_cast<unsigned>(rescue.phase)
            << ",\"generation\":" << rescue.generation
            << ",\"claimCount\":" << rescue.claimCount
            << ",\"deploymentCount\":" << rescue.deploymentCount
            << ",\"equipmentStowed\":" << (rescue.equipmentStowed?"true":"false")
            << ",\"ropeHandsActive\":" << (rescue.ropeHandsActive?"true":"false")
            << ",\"bodyStowWitness\":" << (capture.rescueBodyStowWitness?"true":"false")
            << ",\"plantedPelvisDropMetres\":" << rescue.plantedPelvisDropMetres
            << ",\"bodyYawRadians\":" << rescue.bodyYawRadians
            << ",\"root\":";
        vectorJson(rescue.playerPosition);
        traversalEvidence << ",\"renderedEye\":[" << capture.renderedEyePosition[0] << ','
            << capture.renderedEyePosition[1] << ',' << capture.renderedEyePosition[2]
            << "],\"gripNodeIndices\":[" << rescue.grippingRopeNodeIndices[0] << ','
            << rescue.grippingRopeNodeIndices[1] << "],\"gripTargets\":[";
        vectorJson(rescue.grippingHandTargets[0]); traversalEvidence << ',';
        vectorJson(rescue.grippingHandTargets[1]); traversalEvidence << "],\"ropeNodes\":[";
        for(std::size_t i=0;i<rescue.ropeNodes.size();++i) {
            if(i)traversalEvidence << ',';
            vectorJson(rescue.ropeNodes[i]);
        }
        traversalEvidence << "],\"publishedLanternHinge\":[";
        for(std::size_t i=0;i<capture.publishedLanternHinge.size();++i) {
            if(i)traversalEvidence << ',';
            traversalEvidence << capture.publishedLanternHinge[i];
        }
        traversalEvidence << "],\"publishedLanternBody\":[";
        for(std::size_t i=0;i<capture.publishedLanternBody.size();++i) {
            if(i)traversalEvidence << ',';
            traversalEvidence << capture.publishedLanternBody[i];
        }
        traversalEvidence << "]}";
        manifest << "    {\n"
                 << "      \"id\": " << checkpoint.id << ",\n"
                 << "      \"checkpoint\": \"" << JsonEscape(checkpoint.name) << "\",\n"
                 << "      \"preset\": \"" << CapturePresetName(checkpoint.preset) << "\",\n"
                 << "      \"zone\": \"" << horde::gameplay::ShowcaseZoneName(checkpoint.expectedZone) << "\",\n"
                 << "      \"camera\": {\"x\": " << capture.camera[0] << ", \"z\": " << capture.camera[1]
                 << ", \"yaw\": " << capture.camera[2] << ", \"pitch\": " << capture.camera[3] << "},\n"
                 << "      \"requestedCamera\": {\"x\": " << checkpoint.x << ", \"z\": " << checkpoint.z
                 << ", \"yaw\": " << checkpoint.yaw << ", \"pitch\": " << checkpoint.pitch << "},\n"
                 << "      \"state\": {\"torchFailurePhase\": \"" << capture.torchFailurePhase
                 << "\", \"selectedEnemy\": \"" << capture.selectedEnemy
                 << "\", \"lichPhase\": \"" << capture.lichPhase
                 << "\", \"finaleSkylightOpenProgress\": " << capture.finaleSkylightOpenProgress << "},\n"
                 << "      \"width\": " << capture.width << ",\n"
                 << "      \"height\": " << capture.height << ",\n"
                 << "      \"rescueTraversal\": " << traversalEvidence.str() << ",\n"
                  << "      \"honestlyPresentedRtFrame\": true,\n"
                  << "      \"completedFrame\": " << capture.completedFrameEvidenceJson << ",\n"
                  << "      \"indoorDust\": {\"requestedQuality\": "
                  << static_cast<unsigned>(context.requestedDustQuality)
                  << ", \"actualUploadedQuality\": "
                  << static_cast<unsigned>(capture.actualUploadedDustQuality)
                  << ", \"cpuWork\": {\"admittedZones\": " << capture.dustWork.admittedZones
                  << ", \"generatedMotes\": " << capture.dustWork.generatedMotes
                  << ", \"projectedMotes\": " << capture.dustWork.projectedMotes
                  << ", \"tileReferences\": " << capture.dustWork.tileReferences
                  << ", \"overflowReferences\": " << capture.dustWork.overflowReferences << "}},\n"
                  << "      \"visibility\": {\"playerPrimaryVisible\": "
                 << (capture.playerPrimaryVisible ? "true" : "false")
                 << ", \"primaryArmsMayBeOutsideFrame\": "
                 << (capture.primaryArmsMayBeOutsideFrame ? "true" : "false")
                 << ", \"primaryArmsCeilingRetracted\": "
                 << (capture.primaryArmsCeilingRetracted ? "true" : "false")
                 << ", \"verticalProofPoseWitness\": {\"supportWorldY\": "
                 << capture.playerSupportWorldY << ", \"heightDelta\": "
                 << capture.playerHeightDelta << ", \"supportId\": "
                 << static_cast<unsigned>(capture.playerSupportId)
                 << ", \"heldLightKind\": " << static_cast<unsigned>(capture.heldLightKind)
                 << ", \"swordStowBlend\": " << capture.swordStowBlend
                 << ", \"swordHandGripBlend\": " << capture.swordHandGripBlend
                 << ", \"torchOverheadLoweringMetres\": "
                 << capture.torchOverheadLoweringMetres
                 << ", \"torchOverheadRetractionMetres\": "
                 << capture.torchOverheadRetractionMetres << "}"
                 << ", \"chapterCompanion\": {\"grounded\": " << (capture.chapterCompanion.grounded ? "true" : "false")
                 << ", \"walking\": " << (capture.chapterCompanion.walking ? "true" : "false")
                 << ", \"position\": [" << capture.chapterCompanion.x << ", " << capture.chapterCompanion.y << ", " << capture.chapterCompanion.z << "]"
                 << ", \"yaw\": " << capture.chapterCompanion.yaw << ", \"animationSeconds\": " << capture.chapterCompanion.animationSeconds
                 << ", \"generation\": " << capture.chapterCompanion.generation << ", \"keeperDissolve\": " << capture.keeperDissolve << "}"
                 << ", \"playerWorldBodyInstanceFlags\": " << capture.playerWorldBodyInstanceFlags
                 << ", \"instanceMasks\": [";
        for (std::size_t mask = 0u; mask < capture.instanceMasks.size(); ++mask)
            manifest << (mask == 0u ? "" : ", ")
                     << static_cast<std::uint32_t>(capture.instanceMasks[mask]);
        manifest << "], \"diagnostics\": {\"availability\": \""
                 << horde::vulkan::raytracing::ToString(
                        context.rtScene.DiagnosticsAvailability())
                 << "\", \"available\": "
                 << (context.rtScene.DiagnosticsAvailability() ==
                             horde::vulkan::raytracing::RtDiagnosticAvailability::Available
                         ? "true" : "false")
                 << "}, \"primaryPixels\": {\"torch\": "
                 << capture.primaryTorchPixels << ", \"sword\": "
                 << capture.primarySwordPixels << ", \"player\": "
                 << capture.primaryPlayerPixels << ", \"rewardRing\": "
                 << capture.primaryRewardRingPixels << ", \"rewardBody\": "
                 << capture.primaryRewardBodyPixels << "}, \"rewardGrip\": {\"positionErrorMetres\": "
                 << capture.rewardGripPositionErrorMetres
                 << ", \"orientationErrorRadians\": "
                 << capture.rewardGripOrientationErrorRadians
                 << ", \"authorityPositionErrorMetres\": "
                 << capture.rewardAuthorityPositionErrorMetres
                 << ", \"authorityOrientationErrorRadians\": "
                 << capture.rewardAuthorityOrientationErrorRadians
                 << ", \"finalGripPosition\": ["
                 << capture.rewardFinalGripPosition[0] << ", "
                 << capture.rewardFinalGripPosition[1] << ", "
                 << capture.rewardFinalGripPosition[2]
                 << "], \"ringGripPosition\": ["
                 << capture.rewardRingGripPosition[0] << ", "
                 << capture.rewardRingGripPosition[1] << ", "
                 << capture.rewardRingGripPosition[2]
                 << "], \"bodyPosition\": ["
                 << capture.rewardBodyPosition[0] << ", "
                 << capture.rewardBodyPosition[1] << ", "
                 << capture.rewardBodyPosition[2] << "]}},\n"
                 << "      \"timing\": {\"sampleCount\": " << capture.frameTimesMs.size()
                 << ", \"medianMs\": " << CaptureMedianMs(capture.frameTimesMs)
                 << ", \"meanMs\": " << CaptureMeanMs(capture.frameTimesMs) << "},\n"
                 << "      \"outputRedBlueSwapAppliedAndNormalised\": "
                 << (capture.redBlueSwapNormalised ? "true" : "false") << ",\n"
                 << "      \"pixelFormat\": \"RGBA8\",\n"
                 << "      \"file\": \"" << JsonEscape(capture.filename) << "\",\n"
                 << "      \"viewmodelGeometry\": {\"available\": "
                 << (capture.viewmodelGeometryFile.empty() ? "false" : "true")
                 << ", \"space\": \"model\", \"source\": \"cpu-upload\", \"file\": \""
                 << JsonEscape(capture.viewmodelGeometryFile) << "\", \"sha256\": \""
                 << capture.viewmodelGeometrySha256 << "\", \"finiteVerticesValidated\": "
                 << (capture.viewmodelGeometryEvidence.allVertexPositionsFinite ? "true" : "false")
                 << ", \"finiteVertexCount\": "
                 << capture.viewmodelGeometryEvidence.vertexCount << ", \"faceCount\": "
                 << capture.viewmodelGeometryEvidence.faceCount << "},\n"
                 << "      \"playerWorldBodyGeometry\": {\"available\": "
                 << (capture.playerWorldBodyGeometryFile.empty() ? "false" : "true")
                 << ", \"space\": \"model\", \"source\": \"cpu-upload\", \"file\": \""
                 << JsonEscape(capture.playerWorldBodyGeometryFile) << "\", \"sha256\": \""
                 << capture.playerWorldBodyGeometrySha256 << "\", \"finiteVerticesValidated\": "
                 << (capture.worldBodyGeometryEvidence.allVertexPositionsFinite ? "true" : "false")
                 << ", \"finiteVertexCount\": "
                 << capture.worldBodyGeometryEvidence.vertexCount << ", \"faceCount\": "
                 << capture.worldBodyGeometryEvidence.faceCount << "},\n"
                 << "      \"pngSha256\": \"" << capture.pngSha256 << "\"\n"
                 << "    }" << (index + 1u == captures.size() ? "\n" : ",\n");
    }
    manifest << "  ]\n}\n";
    return WriteReportFile(outputDirectory / "capture-manifest.json", manifest.str());
}

#if defined(_DEBUG)
#include "platform/windows/WindowsStaticGeometrySourceInspection.inl"
#include "platform/windows/WindowsStaticGeometryComparison.inl"
bool MaybeAdmitForestContentCandidate(VulkanSurfaceContext& context,std::string& error)
{
    const auto options=ParseCaptureLaunchOptions();
    if(options.forestContentCandidate.empty())return true;
    StaticWorldForestMeasurement measured;
    if(!MeasureStaticWorldForestSource(context,measured,error))return false;
    std::array<std::uint64_t,6> bits{};
    for(unsigned i=0;i<3;++i) {
        static_assert(sizeof(bits[i])==sizeof(measured.bounds.min[i]));
        std::memcpy(&bits[i],&measured.bounds.min[i],sizeof(bits[i]));
        std::memcpy(&bits[3+i],&measured.bounds.max[i],sizeof(bits[i]));
    }
    if(!horde::platform::windows::MatchesFrozenForestContentCandidate(options.forestContentCandidate,
           measured.inputFingerprint,measured.count,bits,measured.structurallyValid)) {
        error="Forest candidate does not match independently frozen source identity/count/bounds: "+measured.inputFingerprint;
        return false;
    }
    if(!context.rtScene.DebugAdmitOrdinaryForestCandidate(measured.generation,measured.source,
           measured.ids,measured.count,measured.bounds,error))return false;
    std::cerr<<"HORDE_FOREST_CONTENT_ADMITTED name="<<options.forestContentCandidate
             <<" fingerprint="<<measured.inputFingerprint<<" triangles="<<measured.count<<'\n';
    return true;
}
#include "platform/windows/WindowsForestContentCapture.inl"
bool RunStaticGeometryCutoverCapture(VulkanSurfaceContext& context,
    const std::filesystem::path& directory, const VkClearColorValue& clear, std::string& error,
    bool triangleSubset = false, std::span<const std::uint32_t> omittedTriangles = {})
{
    using Scene = horde::vulkan::raytracing::PresentableTinyRtScene;
    const auto readback = [&](const char* filename) {
        const VkResult idle = vkDeviceWaitIdle(context.device);
        if (!CompleteRtEvidenceAfterDeviceIdle(context, idle)) { error="Cutover capture completion failed."; return false; }
        Scene::StorageImageCapture image;
        return context.rtScene.CaptureStorageImage(image,error) && WriteRgbaPng(directory/filename,image,error);
    };
    const bool measure=!omittedTriangles.empty();
    if (measure && !MeasureStaticGeometryPhase(context,directory,clear,"before",error)) return false;
    if (!readback("cutover-00-before.png")) return false;
    std::ostringstream receipt;
    receipt << "{\"schema\":1,\"scope\":\"diagnostic GPU owner replacement; shared textures/materials/unchanged BLAS retained\",\"backend\":"
            << static_cast<int>(context.rtScene.ExecutionBackend()) << ",\"hops\":[";
    // This is a feasibility capture, not ordinary-play eviction eligibility.
    // Burial ornaments now share these material families across both zones.
    // Retain the full owner until spatial contributor eligibility is proved;
    // omitting an entire ornament family would also discard exterior pieces.
    for (unsigned hop=0;hop<2;++hop) {
        // Explicit diagnostic only. A matched view does not prove global ray
        // irrelevance or authorize normal-play omission of indoor dressing.
        const auto regions=static_cast<std::uint8_t>(triangleSubset && hop==0 ? 2u :
            horde::scene::assets::kAllStaticTriangleRegions);
        const auto generation=context.rtScene.StaticGeometryGeneration();
        std::shared_ptr<const horde::vulkan::raytracing::RtStaticTriangleOmissions> omissions;
        if(hop==0 && !omittedTriangles.empty()) {
            omissions=context.rtScene.CreateStaticGeometryTriangleOmissions(generation,omittedTriangles,error);
            if(!omissions)return false;
        }
        const auto before=context.rtScene.ResourceInventory();
        const std::array<std::uint32_t,1> invalid{{UINT32_MAX}};
        if (context.rtScene.PrepareStaticGeometryReplacement(generation,invalid,error) ||
            context.rtScene.StaticGeometryGeneration()!=generation ||
            context.rtScene.ResourceInventory().deviceLocalBytes != before.deviceLocalBytes) {
            error="Invalid family preparation changed the admitted GPU owner."; return false;
        }
        const std::span<const std::uint32_t> retained{};
        // Prove both CPU-source and actual GPU allocation cancellation before
        // spreading reconstruction/upload across RT frames.
        if (!context.rtScene.BeginStaticGeometryReplacement(generation,retained,error,regions,omissions) ||
            !context.rtScene.AdvanceStaticGeometryReplacement(generation,64u*1024u,error) ||
            !context.rtScene.CancelStaticGeometryReplacement(error) ||
            context.rtScene.ResourceInventory().deviceLocalBytes != before.deviceLocalBytes ||
            context.rtScene.ResourceInventory().hostVisibleBytes != before.hostVisibleBytes) {
            error="Cancelled real candidate did not preserve the admitted allocations."; return false;
        }
        if (!context.rtScene.BeginStaticGeometryReplacement(generation,retained,error,regions,omissions)) return false;
        unsigned cancelledPreparationFrames=0;
        while (context.rtScene.StaticGeometryReplacementEvidence().uploadedBytes==0u) {
            if (++cancelledPreparationFrames>1024u ||
                !context.rtScene.AdvanceStaticGeometryReplacement(generation,256u*1024u,error)) return false;
            bool presented=false;
            if (!RenderFrame(context,clear,presented) || !presented) {
                error="Admitted owner failed during cancellation proof: "+context.lastRtFrameError; return false;
            }
        }
        if (!context.rtScene.CancelStaticGeometryReplacement(error) ||
            context.rtScene.ResourceInventory().deviceLocalBytes != before.deviceLocalBytes ||
            context.rtScene.ResourceInventory().hostVisibleBytes != before.hostVisibleBytes) {
            error="Cancelled GPU upload did not preserve admitted allocations."; return false;
        }
        if (!context.rtScene.BeginStaticGeometryReplacement(generation,retained,error,regions,omissions)) return false;
        unsigned preparationFrames=0;
        while (!context.rtScene.StaticGeometryReplacementReady()) {
            if (++preparationFrames>1024u ||
                !context.rtScene.AdvanceStaticGeometryReplacement(generation,256u*1024u,error)) return false;
            // The frozen diagnostic remains renderable during AS fence polls.
            if (!context.rtScene.StaticGeometryReplacementReady()) {
                bool presented=false;
                if (!RenderFrame(context,clear,presented) || !presented) {
                    error="Admitted owner failed while candidate uploaded: "+context.lastRtFrameError; return false;
                }
            }
        }
        // Exercise the final TLAS-only refresh on both hardware backends;
        // immutable reconstruction, uploaded bytes and collapse ownership stay
        // unchanged. Moving ordinary-play scheduling is a separate gate.
        const auto uploaded=context.rtScene.StaticGeometryReplacementEvidence().uploadedBytes;
        if(!context.rtScene.RefreshStaticGeometryReplacement(generation,error))return false;
        while(!context.rtScene.StaticGeometryReplacementReady()) {
            if(++preparationFrames>1024u ||
               !context.rtScene.AdvanceStaticGeometryReplacement(generation,256u*1024u,error))return false;
            if(!context.rtScene.StaticGeometryReplacementReady()) {
                bool presented=false;
                if(!RenderFrame(context,clear,presented)||!presented)return false;
            }
        }
        if(context.rtScene.StaticGeometryReplacementEvidence().uploadedBytes!=uploaded) {
            error="TLAS refresh restarted immutable reconstruction/upload.";return false;
        }
        const auto overlap=context.rtScene.ResourceInventory();
        const auto reject = [](void*,std::string& diagnostic) { diagnostic="Injected failed retirement proof"; return false; };
        // A failed completion proof keeps the candidate available for a safe
        // retry; a stale generation must not even invoke the platform proof.
        if (context.rtScene.CommitStaticGeometryReplacement(generation,reject,nullptr,error) ||
            context.rtScene.CommitStaticGeometryReplacement(generation+1,reject,nullptr,error) ||
            context.rtScene.StaticGeometryGeneration()!=generation) {
            error="Failed/stale cutover did not preserve the active owner."; return false;
        }
        // An exact zero-delta authority publication changes the camera/held
        // transforms after preparation. The candidate must rebuild THIS frame's
        // dynamic TLAS before trace, while retiring the previous static owner.
        const auto original=context.simulation.Snapshot();
        horde::gameplay::simulation::InputSnapshot pose;
        pose.damageEnabled=false;pose.tutorialEnabled=false;
        pose.hasAuthoritativePlayerPose=true;
        pose.authoritativePlayerX=original.playerX;pose.authoritativePlayerZ=original.playerZ;
        pose.yawRadians=original.playerYawRadians+.025f;pose.pitchRadians=original.playerPitchRadians;
        context.simulation.StepFixed(pose,0,original.inputPublicationSequence+1);
        context.staticReplacementExpectedGeneration=generation;
        context.staticReplacementFrameRequested=true;
        bool presented=false;
        if (!RenderFrame(context,clear,presented) || !presented ||
            context.rtScene.StaticGeometryGeneration()!=generation+1 || context.staticReplacementFrameRequested)
        { error="Moving-frame static owner adoption did not produce a submitted RT frame: "+context.lastRtFrameError;return false; }
        pose.yawRadians=original.playerYawRadians;
        context.simulation.StepFixed(pose,0,context.simulation.Snapshot().inputPublicationSequence+1);
        const auto& m=context.rtScene.StaticGeometryReplacementEvidence();
        const auto after=context.rtScene.ResourceInventory();
        receipt << (hop?",":"") << "{\"sourceGeneration\":" << generation
            << ",\"admittedGeneration\":" << context.rtScene.StaticGeometryGeneration()
            << ",\"movingFirstFrame\":true,\"poseYawDeltaRadians\":0.025"
            << ",\"selection\":\"" << (omissions ? "explicit-triangle-omissions" :
                regions==2u ? "shared+graveyard-triangles" : "complete-with-graveyard")
            << "\",\"omittedTriangleCount\":" << (omissions ? omissions->Triangles().size() : 0u)
            << ",\"completeContributors\":" << (context.rtScene.StaticGeometryHasCompleteContributors() ? "true" : "false")
            << ",\"prepareMilliseconds\":" << m.prepareMilliseconds << ",\"cutoverMilliseconds\":" << m.cutoverMilliseconds
            << ",\"packMilliseconds\":" << m.packMilliseconds
            << ",\"beginMilliseconds\":" << m.beginMilliseconds
            << ",\"maximumCpuCopyAdvanceMilliseconds\":" << m.maximumCpuCopyAdvanceMilliseconds
            << ",\"maximumAdvanceMilliseconds\":" << m.maximumAdvanceMilliseconds
            << ",\"uploadAdvanceCount\":" << m.uploadAdvanceCount
            << ",\"uploadedBytes\":" << m.uploadedBytes
            << ",\"cpuCopyAdvanceCount\":" << m.cpuCopyAdvanceCount
            << ",\"cpuRegionScanAdvanceCount\":" << m.cpuRegionScanAdvanceCount
            << ",\"scannedCpuPayloadBytes\":" << m.scannedCpuPayloadBytes
            << ",\"asSubmissionCount\":" << m.asSubmissionCount
            << ",\"asPollCount\":" << m.asPollCount
            << ",\"asPendingPollCount\":" << m.asPendingPollCount
            << ",\"copiedCpuPayloadBytes\":" << m.copiedCpuPayloadBytes
            << ",\"cachedCpuSourcePayloadBytes\":" << context.rtScene.CachedStaticGeometryPayloadBytes()
            << ",\"preparationFrames\":" << preparationFrames
            << ",\"oldOwnerAllocationBytes\":" << m.oldOwnerAllocationBytes
            << ",\"candidateOwnerAllocationBytes\":" << m.candidateOwnerAllocationBytes
            << ",\"overlapOwnerAllocationBytes\":" << m.overlapOwnerAllocationBytes
            << ",\"oldPrimitiveCount\":" << m.oldPrimitiveCount << ",\"newPrimitiveCount\":" << m.newPrimitiveCount
            << ",\"sceneDeviceLocalOverlap\":" << overlap.deviceLocalBytes
            << ",\"sceneHostVisibleOverlap\":" << overlap.hostVisibleBytes
            << ",\"topLevelOwnersBefore\":" << before.topLevelAccelerationStructureCount
            << ",\"topLevelOwnersOverlap\":" << overlap.topLevelAccelerationStructureCount
            << ",\"topLevelOwnersAfter\":" << after.topLevelAccelerationStructureCount
            << ",\"sceneDeviceLocalBefore\":" << before.deviceLocalBytes << ",\"sceneDeviceLocalAfter\":" << after.deviceLocalBytes
            << ",\"sceneHostVisibleBefore\":" << before.hostVisibleBytes << ",\"sceneHostVisibleAfter\":" << after.hostVisibleBytes << "}";
        for (int frame=0;frame<(measure ? 0 : kCaptureSettlingFrames);++frame) {
            bool presented=false;
            if (!RenderFrame(context,clear,presented) || !presented) {
                error="Replacement owner did not produce a real RT-presented frame: "+context.lastRtFrameError; return false;
            }
        }
        if (measure && !MeasureStaticGeometryPhase(context,directory,clear,hop?"restored":"candidate",error)) return false;
        if (!readback(hop?"cutover-02-restored.png":"cutover-01-selected.png")) return false;
    }
    receipt << "]}\n";
    return WriteReportFile(directory/"static-zone-cutover.json",receipt.str());
}
#endif

#include "platform/windows/WindowsChapterRoundTripCapture.inl"
#if defined(_DEBUG) || HORDE_RT_CHAPTER_MEASUREMENT
#include "platform/windows/WindowsChapterShippingMeasurement.inl"
#endif

int RunShowcaseCapture(VulkanSurfaceContext& context,
                       horde::vulkan::DeviceCapabilities& capabilities,
                       const std::filesystem::path& outputDirectory)
{
    std::cerr << "HORDE_SHOWCASE_CAPTURE enter checkpoint=" << context.developmentCheckpoint << std::endl;
    // The finite validation flag overrides saved Mist only for this process.
    // Ordinary captures retain historical Mist On; each frame uploads the choice.
    context.requestedMistEnabled = !ParseCaptureLaunchOptions().mistOff;
    std::cerr << "HORDE_SHOWCASE_CAPTURE options-ready" << std::endl;
    std::error_code directoryError;
    std::filesystem::create_directories(outputDirectory, directoryError);
    std::cerr << "HORDE_SHOWCASE_CAPTURE directory-result=" << directoryError.value() << std::endl;
    if (directoryError)
    {
        std::cerr << "Failed to create showcase capture directory: " << directoryError.message() << '\n';
        return 1;
    }

    std::vector<ShowcaseCaptureRecord> captures;
    std::vector<const horde::gameplay::ShowcaseCheckpoint*> checkpoints;
    horde::gameplay::ShowcaseCheckpoint developmentShowcase{};
    auto ordinaryShowcase = horde::gameplay::kShowcaseCheckpoints;
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
        // This branch uses the normal production route; historical player
        // comparisons own the explicit development-checkpoint branch above.
        for (auto& checkpoint : ordinaryShowcase)
        {
            checkpoint = horde::gameplay::ShowcaseCheckpointForEncounter(checkpoint,
                horde::gameplay::simulation::ProductionGameSimulationConfig().waterfallSkeletonEncounter);
            checkpoints.push_back(&checkpoint);
        }
    }
#if defined(_DEBUG)
    // Moving routes use identical ordinary staging for both representations.
    // Frozen captures retain the separate same-owner recreation gate.
    const bool forestProof=context.forestRouteProof!=horde::platform::windows::ForestRouteCaptureMode::Disabled &&
        !ParseCaptureLaunchOptions().chapterLandingOnly && !ParseCaptureLaunchOptions().validateChapterRoundTrip;
    bool forestRecreated=false;
    std::uint64_t forestEpochBefore=0;
    std::uint64_t forestTickBefore=0;
    horde::telemetry::RtResourceInventory forestInventoryBefore{};
    std::string forestBeforeJson;
    if(forestProof) {
        if(checkpoints.size()!=1u){std::cerr<<"Forest proof requires exactly one frozen checkpoint.\n";return 1;}
        checkpoints.push_back(checkpoints.front()); // One explicit same-owner recreate, using the unchanged staging/capture body.
    }
#endif
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
#if defined(_DEBUG)
        if(forestProof && captures.size()==1u) {
            std::string proofError;
            if(!context.rtScene.ValidateForestRouteProof(proofError))return fail(proofError);
            forestEpochBefore=context.rtFrameEvidence.PublishedStateByValue().sceneEpoch;
            forestTickBefore=context.simulation.Snapshot().tickIndex;
            forestInventoryBefore=context.rtScene.ResourceInventory();
            forestBeforeJson=context.rtScene.ForestRouteProofJson();
            if(!RecreateSwapchain(context))return fail("Forest proof same-owner recreation failed.");
            forestRecreated=true;
        }
#endif
        const horde::gameplay::ShowcaseCheckpoint& checkpoint = *checkpointPointer;
#if defined(_DEBUG)
        if(forestProof && forestRecreated)
        {
            // Reuse the frozen pose/time while normal readiness rebinds.
            // Re-staging the route would advance its tick/lantern phase.
            if(context.simulation.Snapshot().tickIndex!=forestTickBefore)
                return fail("Forest recreation advanced the frozen simulation.");
            MirrorSimulationSnapshot(context);
        }
        else
#endif
        if (checkpoint.id >= 100)
        {
            if(ParseCaptureLaunchOptions().measureChapterShippingRoute)
                std::cerr<<"HORDE_TIMING_BOOTSTRAP checkpoint-stage-begin"<<std::endl;
            horde::vulkan::raytracing::PublishDevelopmentWorldReadiness(context.simulation,context.rtScene);
            const auto* development =
                horde::gameplay::FindDevelopmentCheckpoint(context.developmentCheckpoint);
            horde::gameplay::DevelopmentCheckpointStageEvidence stageEvidence;
            if (development == nullptr ||
                !horde::gameplay::StageDevelopmentCheckpointSimulation(
                    context.simulation, *development, &stageEvidence))
            {
                const auto& state=context.simulation.Snapshot();
                std::ostringstream trace;
                trace << "Development checkpoint '" << checkpoint.name
                    << "' could not stage its authoritative state; reason="
                    << (development == nullptr ? "unknown-checkpoint" :
                        horde::gameplay::DevelopmentCheckpointStageFailureName(stageEvidence.failure))
                    << ", admissionWorldGeneration=" << stageEvidence.worldGeneration
                    << ", admissionTerrainGeneration=" << stageEvidence.terrainGeneration << "; player=("
                    << state.playerX << ',' << state.playerZ << "), rescuePhase="
                    << static_cast<unsigned>(state.rescue.phase) << ", stowed="
                    << state.rescue.equipmentStowed << ", ropeReady=" << state.rescue.ropeReady
                    << ", exterior=" << state.rescue.exteriorSide << ", worldGeneration="
                    << state.worldRoute.generation << ", terrain=" << bool(context.rtScene.ChapterTerrainSurface());
                for(const auto ready:state.worldRoute.readiness)trace << ',' << static_cast<unsigned>(ready);
                return fail(trace.str());
            }
            context.simulation.ResetTiming();
            context.simulation.ClearEvents();
            context.frameDeltaSeconds = 0.0f;
            context.simulationPaused = true;
            context.simulationInput.paused = true;
            context.simulationInput.damageEnabled = false;
            context.simulationInput.hasAuthoritativePlayerPose = false;
            MirrorSimulationSnapshot(context);
            if(ParseCaptureLaunchOptions().measureChapterShippingRoute)
                std::cerr<<"HORDE_TIMING_BOOTSTRAP checkpoint-stage-complete"<<std::endl;
        }
        else
        {
            ApplyCaptureCheckpoint(context, checkpoint);
        }
#if defined(_DEBUG)
        if(forestProof && !forestRecreated)
        {
            // Match the existing lifecycle pause policy before the first view,
            // rather than compare pre-pause sway with its intentional reset.
            context.simulation.SynchronizePausedInput(context.simulationInput,
                context.simulation.Snapshot().inputPublicationSequence);
            MirrorSimulationSnapshot(context);
        }
#endif
#if defined(_DEBUG)
        if (const char* reviveSequence = std::getenv("HORDE_SKELETON_REVIVE_CAPTURE"))
        {
            if (std::string_view(reviveSequence) != "1" ||
                std::getenv("HORDE_GUARD_TRANSITION_CAPTURE") || std::getenv("HORDE_KEEPER_CAST_CAPTURE"))
                return fail("Skeleton revival pose preview requires value 1 and a separate capture process.");
#include "platform/windows/WindowsSkeletonReviveCapture.inl"
        }
        if (const char* guardSequence = std::getenv("HORDE_GUARD_TRANSITION_CAPTURE"))
        {
            if (std::getenv("HORDE_KEEPER_CAST_CAPTURE"))
                return fail("Guard and Keeper fixtures must run in separate processes.");
            if (std::string_view(guardSequence) != "1")
                return fail("Guard transition fixture accepts only explicit value 1.");
#include "platform/windows/WindowsGuardTransitionCapture.inl"
        }
        if (const char* keeperSequence = std::getenv("HORDE_KEEPER_CAST_CAPTURE"))
        {
            if (std::string_view(keeperSequence) != "1")
                return fail("Keeper cast fixture accepts only explicit value 1.");
#include "platform/windows/WindowsKeeperCastCapture.inl"
        }
#endif
        std::vector<double> frameTimesMs;
        frameTimesMs.reserve(kCaptureSettlingFrames);
        unsigned recreateAttempts = 0u;
        for (int frame = 0; frame < kCaptureSettlingFrames; ++frame)
        {
            bool rtFramePresented = false;
            const auto frameStart = std::chrono::steady_clock::now();
            if(frame==0 && ParseCaptureLaunchOptions().measureChapterShippingRoute)
                std::cerr<<"HORDE_TIMING_BOOTSTRAP first-render-begin"<<std::endl;
            if (!RenderFrame(context, clearColor, rtFramePresented))
            {
                return fail(std::string("Checkpoint '") + checkpoint.name +
                            "' did not reach a successful RT swapchain presentation" +
                            (context.lastRtFrameError.empty()
                                ? "."
                                : ": " + context.lastRtFrameError));
            }
            if (context.lastFramePresentation == horde::telemetry::RtPresentationOutcome::PresentedNeedsRecreate ||
                context.lastFramePresentation == horde::telemetry::RtPresentationOutcome::NotPresentedNeedsRecreate)
            {
                if (++recreateAttempts > 8u) return fail(std::string("Checkpoint '") + checkpoint.name +
                    "' repeatedly recreated its output before a stable rendered capture.");
                frame = -1;
                frameTimesMs.clear();
                continue; // Newly allocated output requires a fresh complete settling run.
            }
            if (!rtFramePresented || context.lastFramePresentation != horde::telemetry::RtPresentationOutcome::Presented)
                return fail(std::string("Checkpoint '") + checkpoint.name + "' has no stable RT-presented output for readback.");
            const auto frameEnd = std::chrono::steady_clock::now();
            if(frame==0 && ParseCaptureLaunchOptions().measureChapterShippingRoute)
                std::cerr<<"HORDE_TIMING_BOOTSTRAP first-render-complete"<<std::endl;
            frameTimesMs.push_back(std::chrono::duration<double, std::milli>(frameEnd - frameStart).count());
            capabilities.performance.gpuRt = context.gpuRtTiming;
            capabilities.rtScene.presented = true;
            capabilities.rtScene.executionBackend = context.rtScene.ExecutionBackend();
        }

#if defined(_DEBUG)
        if (!ParseCaptureLaunchOptions().inspectStaticWorldMaterial.empty()) {
            std::string inspectionError;
            if (!InspectStaticWorldSource(context,outputDirectory,
                    ParseCaptureLaunchOptions().inspectStaticWorldMaterial,inspectionError))
                return fail("Static world source inspection: "+inspectionError);
        }
        if (ParseCaptureLaunchOptions().validateStaticZoneCutover) {
            std::string cutoverError;
            if (!RunStaticGeometryCutoverCapture(context,outputDirectory,clearColor,cutoverError,
                    ParseCaptureLaunchOptions().validateStaticTriangleCutover,
                    ParseCaptureLaunchOptions().staticTriangleOmissions))
                return fail("Static GPU owner cutover: "+cutoverError);
        }
#endif
        // Complete the actual last presented submission before reading its
        // image. Capture evidence must not borrow an earlier checkpoint's fire
        // upload or manufacture completion from the current CPU simulation.
        const VkResult captureIdle = vkDeviceWaitIdle(context.device);
        if (!CompleteRtEvidenceAfterDeviceIdle(context, captureIdle))
            return fail(std::string("Checkpoint '") + checkpoint.name +
                        "' could not complete its owning submitted frames.");
        const auto capturePublication = context.rtFrameEvidence.PublishedStateByValue();
        const auto& completedFrame = capturePublication.completedEvidence;
        const auto& completedIdentity = completedFrame.identity.submitted;
        if (!capturePublication.presented || !capturePublication.hasCompletedEvidence ||
            completedIdentity.frame.sceneEpoch != capturePublication.sceneEpoch ||
            completedIdentity.frame.measurementGeneration != capturePublication.measurementGeneration ||
            completedIdentity.frame.simulationTick != context.simulation.Snapshot().tickIndex ||
            completedFrame.presentation.outcome != horde::telemetry::RtPresentationOutcome::Presented ||
            completedIdentity.submissionSerial !=
                completedFrame.presentation.lastSuccessfulPresentSubmissionSerial ||
            !completedFrame.scene.fireLighting.has_value() ||
            completedFrame.scene.actualUploadedMistEnabled != std::optional<bool>{context.requestedMistEnabled} ||
            completedFrame.scene.actualUploadedDustQuality !=
                std::optional<horde::graphics::DustQuality>{context.requestedDustQuality} ||
            context.rtScene.UploadedDustQuality() !=
                std::optional<horde::graphics::DustQuality>{context.requestedDustQuality})
            return fail(std::string("Checkpoint '") + checkpoint.name +
                        "' lacks current completed presentation with matching uploaded Mist and Dust state.");
        std::string completedFrameJson;
        horde::telemetry::RtEvidenceValidationError captureEvidenceError{};
        if (!horde::telemetry::SerializeRtPerformanceEvidenceJson(
                completedFrame, completedFrameJson, captureEvidenceError))
            return fail(std::string("Checkpoint '") + checkpoint.name +
                        "' completed-frame evidence failed canonical validation.");
        const bool completedRtDispatch = context.useRtPath &&
            completedFrame.scene.dispatch.sceneReady &&
            completedFrame.scene.dispatch.rtDispatchRecorded &&
            completedFrame.scene.dispatch.swapchainCopyRecorded;
        const bool completedRtPresentation = capturePublication.presented &&
            completedFrame.presentation.outcome == horde::telemetry::RtPresentationOutcome::Presented &&
            completedIdentity.submissionSerial != 0u &&
            completedIdentity.submissionSerial ==
                completedFrame.presentation.lastSuccessfulPresentSubmissionSerial;

        horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
        std::string diagnostic;
        if (!context.rtScene.CaptureStorageImage(image, diagnostic))
        {
            return fail(std::string("Checkpoint '") + checkpoint.name + "' readback failed: " + diagnostic);
        }
        const std::uint64_t expectedStorageImageBytes =
            static_cast<std::uint64_t>(image.width) * image.height * 4u;
        const bool rtStorageImageCopied = image.width != 0u && image.height != 0u &&
            expectedStorageImageBytes == image.rgba.size();

        std::ostringstream filename;
        filename << std::setw(2) << std::setfill('0') << checkpoint.id << '-' << checkpoint.name;
#if defined(_DEBUG)
        if(forestProof && forestRecreated)filename<<"-recreated";
#endif
        filename<<".png";
        const std::filesystem::path pngPath = outputDirectory / filename.str();
        if (!WriteRgbaPng(pngPath, image, diagnostic))
        {
            return fail(std::string("Checkpoint '") + checkpoint.name + "' PNG write failed: " + diagnostic);
        }

        ShowcaseCaptureRecord record;
        record.completedFrameEvidenceJson = std::move(completedFrameJson);
        record.checkpoint = &checkpoint;
        record.actualUploadedDustQuality = *completedFrame.scene.actualUploadedDustQuality;
        record.dustWork = context.rtScene.DustWork();
        const horde::gameplay::simulation::SimulationSnapshot& simulation = context.simulation.Snapshot();
        record.camera = {simulation.playerX, simulation.playerZ,
                         simulation.playerYawRadians, simulation.playerPitchRadians};
        record.torchOverheadLoweringMetres = simulation.heldItemKinematics.torchOverheadLowering;
        record.torchOverheadRetractionMetres = simulation.heldItemKinematics.torchOverheadRetraction;
        record.playerSupportWorldY = simulation.playerSupportWorldY;
        record.rescue = simulation.rescue;
        record.rescueBodyStowWitness =
            horde::platform::windows::HasRescueBodyStowCaptureWitness(simulation);
        const auto torsoOffset = horde::gameplay::traversal::RescueTorsoOffset(simulation.rescue);
        record.renderedEyePosition = {simulation.playerX+torsoOffset.x,
            horde::gameplay::simulation::PlayerEyeWorldY(simulation.playerSupportWorldY)+torsoOffset.y,
            simulation.playerZ+torsoOffset.z};
        record.publishedLanternHinge = simulation.rewardLanternWorldFromHinge;
        record.publishedLanternBody = simulation.lanternPendulum.worldFromBody;
        record.chapterCompanion = simulation.chapterCompanion;
        record.keeperDissolve = simulation.lich.deathDissolveProgress;
        record.playerHeightDelta = simulation.playerHeightDelta;
        record.playerSupportId = static_cast<std::uint8_t>(simulation.playerSupportId);
        record.heldLightKind = static_cast<std::uint8_t>(simulation.interaction.heldLightKind);
        record.swordStowBlend = simulation.heldItemKinematics.swordStowBlend;
        record.swordHandGripBlend = simulation.heldItemKinematics.swordHandGripBlend;
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
        record.playerWorldBodyInstanceFlags = context.rtScene.LastPlayerWorldBodyInstanceFlags();
        record.primaryTorchPixels = context.rtScene.PrimaryTorchPixelCount();
        record.primarySwordPixels = context.rtScene.PrimarySwordPixelCount();
        record.primaryPlayerPixels = context.rtScene.PrimaryPlayerPixelCount();
        record.primaryRewardRingPixels = context.rtScene.PrimaryRewardRingPixelCount();
        record.primaryRewardBodyPixels = context.rtScene.PrimaryRewardBodyPixelCount();
        record.rewardGripPositionErrorMetres =
            context.rtScene.RewardLanternGripAgreement().positionErrorMetres;
        record.rewardGripOrientationErrorRadians =
            context.rtScene.RewardLanternGripAgreement().orientationErrorRadians;
        record.rewardAuthorityPositionErrorMetres =
            context.rtScene.RewardLanternAuthorityAgreement().positionErrorMetres;
        record.rewardAuthorityOrientationErrorRadians =
            context.rtScene.RewardLanternAuthorityAgreement().orientationErrorRadians;
        record.rewardFinalGripPosition = context.rtScene.RewardLanternFinalGripPosition();
        record.rewardRingGripPosition = context.rtScene.RewardLanternRingGripPosition();
        record.rewardBodyPosition = context.rtScene.RewardLanternBodyPosition();
        record.frameTimesMs = std::move(frameTimesMs);
        const auto* development = horde::gameplay::FindDevelopmentCheckpoint(
            context.developmentCheckpoint);
        record.primaryArmsMayBeOutsideFrame = development != nullptr &&
            development->primaryArmsMayBeOutsideFrame;
        const bool simulationRewardClaimed =
            simulation.interaction.heldLightKind ==
                horde::gameplay::interactions::HeldLightKind::RewardLantern;
        const bool claimedRewardCapture = simulationRewardClaimed ||
            (development != nullptr &&
             development->rewardPose !=
                 horde::gameplay::DevelopmentRewardPose::None);
        const auto claimedRewardPixelPolicy =
            horde::platform::windows::ClaimedRewardCapturePolicy(checkpoint.name,
                horde::platform::windows::IsCaptureSwordFullyStowed(simulation.heldItems[1]), &simulation);
        const bool claimedTorchOwnershipValid =
            horde::platform::windows::ClaimedTorchPrimaryOwnershipValid(
                record.primaryTorchPixels, record.instanceMasks[1],
                record.instanceMasks[horde::vulkan::raytracing::PresentableTinyRtScene::kPlayerTorchInstanceIndex],
                record.instanceMasks[horde::vulkan::raytracing::PresentableTinyRtScene::kKeeperTorchFirstTlasInstance],
                record.instanceMasks[horde::vulkan::raytracing::PresentableTinyRtScene::kKeeperTorchFirstTlasInstance+1u]);
        const bool diagnosticPixelCountersAvailable =
            context.rtScene.DiagnosticsAvailability() ==
            horde::vulkan::raytracing::RtDiagnosticAvailability::Available;
        const bool viewmodelCapture = horde::vulkan::raytracing::PlayerRenderRouteForCheckpoint(
            context.developmentCheckpoint) == horde::vulkan::raytracing::kProductionPlayerRenderRoute;
        const bool inspectionCapture = development != nullptr && development->usesProductionRewardProps &&
            simulation.chestReward.phase == horde::gameplay::interactions::ChestRewardPhase::Locked;
        const bool dedicatedPlayerOwnership = horde::vulkan::raytracing::HasDedicatedPlayerPrimaryOwnership(
            record.instanceMasks, record.playerWorldBodyInstanceFlags);
        const bool verticalProofRaisedCandidate = development != nullptr && development->id == 171u &&
            development->name == "vertical-proof-raised" &&
            !development->primaryArmsMayBeOutsideFrame;
        const auto capturePlayerGeometry = [&]() -> bool
        {
            record.viewmodelGeometryFile = std::filesystem::path(record.filename).replace_extension(".obj").string();
            const auto geometryPath = outputDirectory / record.viewmodelGeometryFile;
            if (!context.rtScene.CaptureViewmodelMesh(geometryPath.string(), diagnostic))
            {
                fail(std::string("Checkpoint '") + checkpoint.name + "' geometry capture failed: " + diagnostic);
                return false;
            }
            record.viewmodelGeometryEvidence.currentUploadCaptured = true;
            if ((verticalProofRaisedCandidate &&
                 !ValidateCapturedObjGeometry(geometryPath, record.viewmodelGeometryEvidence, diagnostic)) ||
                !Sha256File(geometryPath, record.viewmodelGeometrySha256, diagnostic))
            {
                fail(std::string("Checkpoint '") + checkpoint.name + "' geometry capture failed: " + diagnostic);
                return false;
            }

            record.playerWorldBodyGeometryFile =
                std::filesystem::path(record.filename).replace_extension(".player-world-body.obj").string();
            const auto worldBodyGeometryPath = outputDirectory / record.playerWorldBodyGeometryFile;
            if (!context.rtScene.CapturePlayerWorldBodyMesh(worldBodyGeometryPath.string(), diagnostic))
            {
                fail(std::string("Checkpoint '") + checkpoint.name +
                     "' world-body geometry capture failed: " + diagnostic);
                return false;
            }
            record.worldBodyGeometryEvidence.currentUploadCaptured = true;
            if ((verticalProofRaisedCandidate &&
                 !ValidateCapturedObjGeometry(worldBodyGeometryPath,
                                              record.worldBodyGeometryEvidence, diagnostic)) ||
                !Sha256File(worldBodyGeometryPath,
                            record.playerWorldBodyGeometrySha256, diagnostic))
            {
                fail(std::string("Checkpoint '") + checkpoint.name +
                     "' world-body geometry capture failed: " + diagnostic);
                return false;
            }
            return true;
        };
        if (viewmodelCapture && verticalProofRaisedCandidate && !capturePlayerGeometry())
            return 1;
        const horde::platform::windows::VerticalProofRaisedCaptureEvidence ceilingRetractionEvidence{
            .checkpointId = development == nullptr ? 0u : development->id,
            .checkpointName = development == nullptr ? std::string_view{} : development->name,
            .checkpointAllowsCroppedArms = development != nullptr &&
                development->primaryArmsMayBeOutsideFrame,
            .simulation = &simulation,
            .completedRtDispatch = completedRtDispatch,
            .completedRtPresentation = completedRtPresentation,
            .rtStorageImageCopied = rtStorageImageCopied,
            .dedicatedPlayerOwnership = dedicatedPlayerOwnership,
            .primaryPlayerVisible = record.playerPrimaryVisible,
            .primaryPixelCounterAvailable = diagnosticPixelCountersAvailable,
            .primaryArmPixels = record.primaryPlayerPixels,
            .viewmodelGeometry = record.viewmodelGeometryEvidence,
            .worldBodyGeometry = record.worldBodyGeometryEvidence};
        record.primaryArmsCeilingRetracted =
            horde::platform::windows::AdmitsVerticalProofRaisedCeilingRetraction(
                ceilingRetractionEvidence);
        if (viewmodelCapture && !inspectionCapture &&
            !horde::platform::windows::HasExpectedPlayerCaptureVisibility(
                dedicatedPlayerOwnership, record.playerPrimaryVisible,
                diagnosticPixelCountersAvailable, record.primaryPlayerPixels,
                record.primaryArmsMayBeOutsideFrame,
                &ceilingRetractionEvidence))
        {
            // Preserve the actual uploaded meshes before rejecting the frame.
            // Eligibility is not a primary-hit witness; these diagnostics help
            // distinguish a solved pose outside the frustum from occlusion.
            std::cerr<<"HORDE_CAPTURE_OWNERSHIP_FAILURE dedicated="<<dedicatedPlayerOwnership
                <<" visible="<<record.playerPrimaryVisible<<" flags="<<record.playerWorldBodyInstanceFlags
                <<" world_mask="<<static_cast<unsigned>(record.instanceMasks[horde::vulkan::raytracing::kPlayerWorldBodyInstanceIndex])
                <<" viewmodel_mask="<<static_cast<unsigned>(record.instanceMasks[horde::vulkan::raytracing::kPlayerViewmodelInstanceIndex])
                <<" diagnostic="<<diagnosticPixelCountersAvailable<<std::endl;
            if (!record.viewmodelGeometryEvidence.currentUploadCaptured && !capturePlayerGeometry()) return false;
            const auto& k = simulation.heldItemKinematics;
            std::ostringstream pose;
            pose << "{\"checkpoint\":\"" << checkpoint.name << "\",\"camera\":["
                 << simulation.playerX << ',' << simulation.playerSupportWorldY << ','
                 << simulation.playerZ << ',' << simulation.playerYawRadians << ','
                 << simulation.playerPitchRadians << "],\"torchLowering\":"
                 << k.torchOverheadLowering << ",\"torchRetraction\":" << k.torchOverheadRetraction
                 << ",\"leftHandLocal\":[" << k.leftHandLocal[0] << ',' << k.leftHandLocal[1] << ','
                 << k.leftHandLocal[2] << "],\"rightHandLocal\":[" << k.rightHandLocal[0] << ','
                 << k.rightHandLocal[1] << ',' << k.rightHandLocal[2] << "]}\n";
            if (!WriteReportFile(outputDirectory / "failed-primary-pose.json", pose.str()))
                return fail("Failed to retain rejected primary-pose evidence.");
            return fail("Dedicated viewmodel capture lacks modelled primary arms or contains legacy/full-body primary ownership: dedicated=" +
                        std::to_string(dedicatedPlayerOwnership) + ", visible=" +
                        std::to_string(record.playerPrimaryVisible) + ", armPixels=" +
                        std::to_string(record.primaryPlayerPixels) + ".");
        }
        if (context.developmentCheckpoint.empty() &&
            !simulationRewardClaimed &&
            (record.instanceMasks[1] != 0x02u ||
             record.instanceMasks[3] != 0x02u ||
             !dedicatedPlayerOwnership ||
             !record.playerPrimaryVisible ||
             (diagnosticPixelCountersAvailable &&
              record.primaryPlayerPixels == 0u)))
        {
            return fail(std::string("Checkpoint '") + checkpoint.name +
                        "' masked the ordinary torch/sword or lost dedicated modelled player ownership.");
        }
        if (diagnosticPixelCountersAvailable && checkpoint.name == "opening" &&
            (record.primaryTorchPixels == 0u ||
             record.primarySwordPixels == 0u ||
             record.primaryPlayerPixels == 0u))
        {
            return fail("Opening capture lacks primary-visible modelled arms, torch, or sword (floating-prop regression).");
        }
        if (claimedRewardCapture &&
            ((viewmodelCapture ? !dedicatedPlayerOwnership : record.instanceMasks[4] != 0x10u) ||
             (!viewmodelCapture && (record.instanceMasks[10] != 0x04u ||
             record.instanceMasks[11] != 0x04u ||
             record.instanceMasks[12] != 0x04u ||
             record.instanceMasks[13] != 0x04u)) ||
             !record.playerPrimaryVisible ||
             !claimedTorchOwnershipValid ||
             record.instanceMasks[7] != 0x01u ||
             record.instanceMasks[8] != 0x01u ||
             (diagnosticPixelCountersAvailable &&
              ((claimedRewardPixelPolicy.requirePlayerPixels &&
                record.primaryPlayerPixels == 0u) ||
               (claimedRewardPixelPolicy.requireRewardRingPixels &&
                record.primaryRewardRingPixels == 0u) ||
               (claimedRewardPixelPolicy.requireRewardBodyPixels &&
                record.primaryRewardBodyPixels == 0u) ||
               (claimedRewardPixelPolicy.requireSwordPixels &&
                record.primarySwordPixels == 0u))) ||
             record.rewardGripPositionErrorMetres >
                 horde::vulkan::raytracing::kPlayerGripSocketToleranceMetres ||
             record.rewardGripOrientationErrorRadians >
                 horde::vulkan::raytracing::kPlayerGripOrientationToleranceRadians ||
             record.rewardAuthorityPositionErrorMetres >
                 horde::vulkan::raytracing::kPlayerGripSocketToleranceMetres ||
             record.rewardAuthorityOrientationErrorRadians >
                 horde::vulkan::raytracing::kPlayerGripOrientationToleranceRadians))
        {
            std::ostringstream reason;
            reason << "Checkpoint '" << checkpoint.name
                   << "' failed claimed reward player/lantern primary visibility or final GripRing contact: pixels="
                   << record.primaryPlayerPixels << "/" << record.primaryRewardRingPixels
                   << "/" << record.primaryRewardBodyPixels << " sword/torch="
                   << record.primarySwordPixels << "/"
                   << record.primaryTorchPixels << " wall-retraction="
                   << claimedRewardPixelPolicy.permitsCompleteWallRetraction
                   << " grip="
                   << record.rewardGripPositionErrorMetres << "/"
                   << record.rewardGripOrientationErrorRadians << " authority="
                   << record.rewardAuthorityPositionErrorMetres << "/"
                   << record.rewardAuthorityOrientationErrorRadians << " finalGrip="
                   << record.rewardFinalGripPosition[0] << ","
                   << record.rewardFinalGripPosition[1] << ","
                   << record.rewardFinalGripPosition[2] << " body="
                   << record.rewardBodyPosition[0] << ","
                   << record.rewardBodyPosition[1] << ","
                   << record.rewardBodyPosition[2] << ".";
            return fail(reason.str());
        }
        if (!Sha256File(pngPath, record.pngSha256, diagnostic))
        {
            return fail(std::string("Checkpoint '") + checkpoint.name + "' hash failed: " + diagnostic);
        }
        if (viewmodelCapture && record.viewmodelGeometryFile.empty() && !capturePlayerGeometry())
            return 1;
#if defined(_DEBUG)
        if(forestProof && !context.rtScene.ValidateForestRouteProof(diagnostic))return fail(diagnostic);
#endif
        captures.push_back(std::move(record));
        std::cout << "Captured " << checkpoint.name << " -> " << pngPath << '\n';
    }

#if defined(_DEBUG)
    if(forestProof) {
        const auto after=context.rtScene.ResourceInventory();
        const auto epoch=context.rtFrameEvidence.PublishedStateByValue().sceneEpoch;
        const bool resourcesMatch=after.bottomLevelAccelerationStructureCount==forestInventoryBefore.bottomLevelAccelerationStructureCount &&
            after.topLevelAccelerationStructureCount==forestInventoryBefore.topLevelAccelerationStructureCount &&
            after.tlasInstanceCount==forestInventoryBefore.tlasInstanceCount && after.bufferCount==forestInventoryBefore.bufferCount &&
            after.memoryAllocationCount==forestInventoryBefore.memoryAllocationCount && after.deviceLocalBytes==forestInventoryBefore.deviceLocalBytes &&
            after.hostVisibleBytes==forestInventoryBefore.hostVisibleBytes;
        const bool sameImage=captures.size()==2u && captures[0].pngSha256==captures[1].pngSha256;
        const bool sameTick=context.simulation.Snapshot().tickIndex==forestTickBefore;
        const bool complete=forestRecreated && epoch>forestEpochBefore && resourcesMatch && sameImage && sameTick;
        std::ostringstream proof;
        proof<<"{\"complete\":"<<(complete?"true":"false")<<",\"sameRendererObject\":true,\"initialPausedInputBarrier\":true,\"sceneEpochBefore\":"<<forestEpochBefore
             <<",\"sceneEpochAfter\":"<<epoch<<",\"resourcesMatch\":"<<(resourcesMatch?"true":"false")
             <<",\"simulationTickBefore\":"<<forestTickBefore<<",\"simulationTickAfter\":"<<context.simulation.Snapshot().tickIndex
             <<",\"storageImageSha256Matches\":"<<(sameImage?"true":"false")<<",\"before\":"<<forestBeforeJson
             <<",\"after\":"<<context.rtScene.ForestRouteProofJson()<<"}\n";
        if(!WriteReportFile(outputDirectory/"forest-route-recreate.json",proof.str()) || !complete)
            return fail("Forest proof recreation changed its frozen pixels/resources or lacked a fresh scene epoch; evidence retained.");
    }
    if((context.forestRouteProof==horde::platform::windows::ForestRouteCaptureMode::SharedRetention64 ||
        context.forestRouteProof==horde::platform::windows::ForestRouteCaptureMode::NormalShared64) &&
       context.developmentCheckpoint.starts_with("chapter-forest-retention-")) {
        // Bounded coherent restaging, not natural traversal. The existing
        // checkpoint path resolves camera, hands and lights together; an unsafe
        // source-pocket exit must retain every tree before that valid frame.
        const auto* sourceView=horde::gameplay::FindDevelopmentCheckpoint(context.developmentCheckpoint);
        if(!sourceView)return fail("Retention transition lacks its source checkpoint.");
        std::string error;
        if(!context.rtScene.ValidateForestRouteProof(error,true))return fail(error);
        std::ostringstream proof;proof<<"{\"schema\":1,\"fault\":\"source-pocket-exit-restaging\",\"phases\":[";
        std::uint64_t precedingSerial=context.rtFrameEvidence.PublishedStateByValue().completedEvidence.identity.submitted.submissionSerial;
        for(unsigned phase=0;phase<2u;++phase) {
            auto view=*sourceView;
            if(phase==0u)view.cameraX=-28.f;
            horde::vulkan::raytracing::PublishDevelopmentWorldReadiness(context.simulation,context.rtScene);
            if(!horde::gameplay::StageDevelopmentCheckpointSimulation(context.simulation,view))
                return fail("Retention transition could not coherently stage its diagnostic view.");
            context.simulation.ResetTiming();context.simulation.ClearEvents();
            context.simulation.SynchronizePausedInput(context.simulationInput,
                context.simulation.Snapshot().inputPublicationSequence);
            MirrorSimulationSnapshot(context);
            const auto expectedTick=context.simulation.Snapshot().tickIndex;
            bool presented=false;
            const bool rendered=RenderFrame(context,clearColor,presented);
            if(!rendered || !presented || context.lastFramePresentation!=horde::telemetry::RtPresentationOutcome::Presented ||
               !CompleteRtEvidenceAfterDeviceIdle(context,vkDeviceWaitIdle(context.device)))
                return fail("Retention guard transition lacks its completed current RT presentation.");
            const auto publication=context.rtFrameEvidence.PublishedStateByValue();
            const auto& completed=publication.completedEvidence;
            const auto& submitted=completed.identity.submitted;
            if(!publication.hasCompletedEvidence || submitted.submissionSerial<=precedingSerial ||
               submitted.frame.sceneEpoch!=publication.sceneEpoch || submitted.frame.simulationTick!=expectedTick ||
               !completed.scene.dispatch.rtDispatchRecorded || !completed.scene.dispatch.swapchainCopyRecorded ||
               completed.presentation.lastSuccessfulPresentSubmissionSerial!=submitted.submissionSerial ||
               !context.rtScene.ValidateForestRouteProof(error,phase!=0u))
                return fail("Retention guard transition failed its exact submitted mask/identity decision: "+error);
            precedingSerial=submitted.submissionSerial;
            std::string canonical;horde::telemetry::RtEvidenceValidationError evidenceError{};
            if(!horde::telemetry::SerializeRtPerformanceEvidenceJson(completed,canonical,evidenceError))
                return fail("Retention transition evidence failed canonical serialization.");
            if(phase)proof<<',';
            proof<<"{\"phase\":\""<<(phase?"restored-safe":"unsafe-retained")<<"\",\"frame\":"<<canonical
                 <<",\"forest\":"<<context.rtScene.ForestRouteProofJson()<<'}';
        }
        horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture restored;
        if(!context.rtScene.CaptureStorageImage(restored,error))return fail("Retention restored image readback failed: "+error);
        const auto restoredPath=outputDirectory/"forest-retention-restored.png";std::string restoredHash;
        if(!WriteRgbaPng(restoredPath,restored,error) || !Sha256File(restoredPath,restoredHash,error))return fail(error);
        const bool sameImage=!captures.empty() && restoredHash==captures.back().pngSha256;
        proof<<"],\"restoredPixelsMatch\":"<<(sameImage?"true":"false")<<",\"complete\":"<<(sameImage?"true":"false")<<"}\n";
        if(!WriteReportFile(outputDirectory/"forest-retention-transition.json",proof.str()) || !sameImage)
            return fail("Retention restore changed frozen output; transition evidence retained.");
    }
#endif
    if (!WriteCaptureManifest(outputDirectory, context, capabilities, captures, true, {}))
    {
        std::cerr << "Failed to write capture manifest.\n";
        return 1;
    }
    std::cout << "Captured all " << captures.size() << " showcase checkpoints to " << outputDirectory << '\n';
    return 0;
}
bool CaptureEntryNativeControls(
    HWND window, const std::filesystem::path &path, std::string &error,
    const horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture *rtBackground = nullptr)
{
    RECT client{};
    if (!GetClientRect(window, &client) || client.right <= 0 || client.bottom <= 0)
        return false;
    HDC source = GetDC(window);
    if (!source)
        return false;
    HDC target = CreateCompatibleDC(source);
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = client.right;
    info.bmiHeader.biHeight = -client.bottom;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void *pixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(source, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (!target || !bitmap || !pixels)
    {
        if (bitmap)
            DeleteObject(bitmap);
        if (target)
            DeleteDC(target);
        ReleaseDC(window, source);
        return false;
    }
    const HGDIOBJ previous = SelectObject(target, bitmap);
    UpdateWindow(window);
    GdiFlush();
    const bool copied =
        BitBlt(target, 0, 0, client.right, client.bottom, source, 0, 0, SRCCOPY) != FALSE;
    horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture capture;
    capture.width = static_cast<std::uint32_t>(client.right);
    capture.height = static_cast<std::uint32_t>(client.bottom);
    if (copied)
    {
        const auto bytes = static_cast<const std::uint8_t *>(pixels);
        capture.rgba.assign(bytes,
                            bytes + static_cast<std::size_t>(capture.width) * capture.height * 4);
        for (std::size_t index = 0; index < capture.rgba.size(); index += 4)
        {
            std::swap(capture.rgba[index], capture.rgba[index + 2]);
            capture.rgba[index + 3] = 255;
        }
    }
    SelectObject(target, previous);
    DeleteObject(bitmap);
    DeleteDC(target);
    ReleaseDC(window, source);
    if (copied && rtBackground != nullptr)
    {
        if (rtBackground->width != capture.width || rtBackground->height != capture.height)
        {
            error = "Native-control review composition requires native-resolution RT readback.";
            return false;
        }
        // GDI readback omits the Vulkan surface. Preserve the actual presented
        // RT pixels and copy only actual visible native child rectangles. This
        // review composition is explicitly identified in the receipt; it does
        // not certify physical display timing or replace the real controls.
        struct ChildRectangles { HWND parent; std::vector<RECT> rectangles; } children{window, {}};
        EnumChildWindows(window, [](HWND child, LPARAM parameter) -> BOOL
        {
            auto &list = *reinterpret_cast<ChildRectangles *>(parameter);
            if (IsWindowVisible(child))
            {
                RECT rectangle{};
                if (GetWindowRect(child, &rectangle))
                {
                    MapWindowPoints(HWND_DESKTOP, list.parent, reinterpret_cast<POINT *>(&rectangle), 2);
                    list.rectangles.push_back(rectangle);
                }
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&children));
        auto composed = *rtBackground;
        for (const auto &rectangle : children.rectangles)
            for (int y = std::max(0L, rectangle.top);
                 y < std::min(static_cast<LONG>(capture.height), rectangle.bottom); ++y)
                for (int x = std::max(0L, rectangle.left);
                     x < std::min(static_cast<LONG>(capture.width), rectangle.right); ++x)
                {
                    const auto offset = (static_cast<std::size_t>(y) * capture.width + x) * 4;
                    std::copy_n(capture.rgba.data() + offset, 4, composed.rgba.data() + offset);
                }
        capture = std::move(composed);
    }
    return copied && WriteRgbaPng(path, capture, error);
}

int RunEntryMenuCapture(VulkanSurfaceContext &context,
                        horde::vulkan::DeviceCapabilities &capabilities,
                        const std::filesystem::path &directory)
{
    std::filesystem::create_directories(directory);
    std::ostringstream receipt;
    std::string error, executableHash;
    std::array<wchar_t, 32768> executable{};
    GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
    if (!Sha256File(std::filesystem::path(executable.data()), executableHash, error))
        return 1;
    receipt << "{\n\"buildId\":\"" << JsonEscape(HORDE_RT_BUILD_ID) << "\",\"executableSha256\":\""
            << executableHash
            << "\",\"scene\":\"EntryMenu\",\"physicalDisplayTimingMeasured\":false,"
            << "\"reviewComposition\":\"Presented RT readback plus actual visible native child rectangles\","
            << "\"renderScalePercent\":" << CurrentGraphicsSettings(context).renderScalePercent
            << ",\"glassEnabled\":" << (context.requestedGlassEnabled ? "true" : "false")
            << ",\"mistEnabled\":" << (context.requestedMistEnabled ? "true" : "false")
            << ",\"captures\":[";
    const auto fail = [&](const int code, const char *reason)
    {
        std::cerr << "Entry capture failed: " << reason << ' ' << error << '\n';
        WriteReportFile(directory / "entry-menu-capture.json", receipt.str() +
            "],\"complete\":false,\"failure\":\"" + JsonEscape(reason) + "\"}\n");
        return code;
    };
    const VkClearColorValue clear{{0, 0, 0, 1}};
    context.pauseMenuVisible = true;
    ApplyOverlayState(context);
    RECT client{};
    GetClientRect(context.windowHandle, &client);
    LayoutOverlayControls(context.windowHandle, client.right, client.bottom);
    const auto savedBeforePreview = context.savedGraphics;
    RECT originalWindow{};
    GetWindowRect(context.windowHandle, &originalWindow);
    // Exercise the real native no-Use Back path. The return destination must
    // be captured on opening Graphics, before its preview replaces Entry.
    SendMessageA(context.windowHandle, WM_COMMAND,
                 MAKEWPARAM(kSettingsButtonId, BN_CLICKED), 0);
    SendMessageA(context.windowHandle, WM_COMMAND,
                 MAKEWPARAM(kGraphicsOpenButtonId, BN_CLICKED), 0);
    std::vector<double> previewTimings;
    if (!ApplyPendingSceneReplacement(context, capabilities, previewTimings) ||
        !ApplyPendingOutputResize(context, capabilities, previewTimings) ||
        context.rtScene.Profile() != horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview)
        return fail(18, "Native Graphics did not enter its separate preview");
    SendMessageA(context.windowHandle, WM_COMMAND,
                 MAKEWPARAM(kSettingsBackButtonId, BN_CLICKED), 0);
    for (unsigned frame = 0; frame < 12 && context.graphicsVisible; ++frame)
    {
        if (!ApplyPendingSceneReplacement(context, capabilities, previewTimings) ||
            !ApplyPendingOutputResize(context, capabilities, previewTimings))
            return fail(19, "Native Graphics Back replacement failed");
        context.graphicsPreviewDelta = 1.0 / 30;
        bool presented = false;
        if (!RenderFrame(context, clear, presented))
            return fail(20, "Native Graphics Back lacks a presented RT frame");
        if (!presented)
            continue; // Surface recreation is a retry, never an acknowledgement.
        const auto idle = vkDeviceWaitIdle(context.device);
        if (idle != VK_SUCCESS || !CompleteRtEvidenceAfterDeviceIdle(context, idle))
            return fail(21, "Native Graphics Back frame did not complete");
        FinishGraphicsFrame(context, presented);
    }
    if (context.graphicsVisible || context.graphicsCommand || context.graphicsSceneRestoring ||
        context.rtScene.Profile() != horde::vulkan::raytracing::RtSceneProfile::EntryMenu ||
        context.savedGraphics != savedBeforePreview || !context.simulationPaused)
        return fail(22, "Native Graphics Back did not restore Entry and saved graphics");
    SetWindowPos(context.windowHandle, nullptr, originalWindow.left, originalWindow.top,
                 originalWindow.right - originalWindow.left,
                 originalWindow.bottom - originalWindow.top, SWP_NOZORDER | SWP_NOACTIVATE);
    if (!ApplyPendingOutputResize(context, capabilities, previewTimings))
        return fail(23, "Original Entry orientation did not recover after Graphics");
    SendMessageA(context.windowHandle, WM_COMMAND,
                 MAKEWPARAM(kSettingsBackButtonId, BN_CLICKED), 0);
    for (unsigned pose = 0; pose < 6; ++pose)
    {
        if (pose == 1)
            SendMessageA(context.windowHandle, WM_COMMAND,
                         MAKEWPARAM(kSettingsButtonId, BN_CLICKED), 0);
        if (pose == 2)
            SendMessageA(context.windowHandle, WM_COMMAND,
                         MAKEWPARAM(kSettingsBackButtonId, BN_CLICKED), 0);
        if (pose == 3)
            SendMessageA(context.windowHandle, WM_COMMAND,
                         MAKEWPARAM(kEntryMoreButtonId, BN_CLICKED), 0);
        if (pose == 4)
            SendMessageA(context.windowHandle, WM_COMMAND,
                         MAKEWPARAM(kEntryBackButtonId, BN_CLICKED), 0);
        if (pose == 5)
            SendMessageA(context.windowHandle, WM_COMMAND, MAKEWPARAM(kResumeButtonId, BN_CLICKED),
                         0);
        const auto start = std::chrono::steady_clock::now();
        unsigned renderCalls = 0, presentedFrames = 0;
        while (presentedFrames < 30 && renderCalls < 60)
        {
            ++renderCalls;
            context.graphicsPreviewDelta = 1.0 / 30;
            bool presented = false;
            if (!RenderFrame(context, clear, presented))
                return fail(2, "No presented RT frame");
            if (!presented)
                continue;
            ++presentedFrames;
            capabilities.rtScene.presented = true;
            capabilities.performance.gpuRt = context.gpuRtTiming;
        }
        if (presentedFrames != 30)
            return fail(24, "Entry surface did not recover to thirty presented frames");
        const auto elapsed =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
                .count();
        const VkResult idle = vkDeviceWaitIdle(context.device);
        if (idle != VK_SUCCESS || !CompleteRtEvidenceAfterDeviceIdle(context, idle))
            return fail(3, "Presented submission did not complete");
        const auto publication = context.rtFrameEvidence.PublishedStateByValue();
        const auto &completed = publication.completedEvidence;
        const auto &identity = completed.identity.submitted;
        std::string completedJson;
        horde::telemetry::RtEvidenceValidationError evidenceError{};
        if (!publication.presented || !publication.hasCompletedEvidence ||
            identity.frame.sceneEpoch != publication.sceneEpoch ||
            identity.frame.simulationTick != context.entryMenu.Snapshot().tick ||
            completed.presentation.outcome != horde::telemetry::RtPresentationOutcome::Presented ||
            identity.submissionSerial != completed.presentation.lastSuccessfulPresentSubmissionSerial ||
            !completed.scene.fireLighting.has_value() ||
            !horde::telemetry::SerializeRtPerformanceEvidenceJson(completed, completedJson, evidenceError))
            return fail(11, "Current completed Entry RT identity is invalid");
        horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
        if (!context.rtScene.CaptureStorageImage(image, error))
            return fail(4, "RT readback failed");
        const std::string filename = std::to_string(pose) + "-entry-menu.png";
        std::string hash;
        if (!WriteRgbaPng(directory / filename, image, error) ||
            !Sha256File(directory / filename, hash, error))
            return fail(5, "RT image or hash failed");
        const std::string nativeFilename = std::to_string(pose) + "-entry-native-controls.png";
        std::string nativeHash;
        if (!CaptureEntryNativeControls(context.windowHandle, directory / nativeFilename, error) ||
            !Sha256File(directory / nativeFilename, nativeHash, error))
            return fail(10, "Native control readback failed");
        const std::string reviewFilename = std::to_string(pose) + "-entry-review-composition.png";
        std::string reviewHash;
        if (!CaptureEntryNativeControls(context.windowHandle, directory / reviewFilename, error, &image) ||
            !Sha256File(directory / reviewFilename, reviewHash, error))
            return fail(12, "Explicit review composition failed");
        const auto resources = context.rtScene.ResourceInventory();
        if (resources.tlasInstanceCount != 3 || resources.topLevelAccelerationStructureCount != 1 ||
            resources.bottomLevelAccelerationStructureCount != 3)
            return fail(6, "Entry resource admission differs from three instances");
        const auto snapshot = context.entryMenu.Snapshot();
        if ((pose == 1 || pose == 3) && snapshot.camera.x >= -.59f)
            return fail(7, "Native side page did not pan left");
        if (pose == 5 && (!context.entryMenu.ReadyToPlay() || snapshot.fade != 1))
            return fail(8, "Native Play did not complete black fade");
        if (pose)
            receipt << ',';
        receipt << "{\"file\":\"" << filename << "\",\"sha256\":\"" << hash
                << "\",\"tick\":" << snapshot.tick << ",\"nativeControlsFile\":\"" << nativeFilename
                << "\",\"nativeControlsSha256\":\"" << nativeHash
                << "\",\"reviewCompositionFile\":\"" << reviewFilename
                << "\",\"reviewCompositionSha256\":\"" << reviewHash
                << "\",\"cameraX\":" << snapshot.camera.x << ",\"fade\":" << snapshot.fade
                << ",\"renderCalls\":" << renderCalls << ",\"presentedFrames\":" << presentedFrames
                << ",\"renderCallsTotalMs\":" << elapsed
                << ",\"tlasInstances\":" << resources.tlasInstanceCount
                << ",\"blas\":" << resources.bottomLevelAccelerationStructureCount
                << ",\"buffers\":" << resources.bufferCount
                << ",\"allocations\":" << resources.memoryAllocationCount
                << ",\"deviceLocalBytes\":" << resources.deviceLocalBytes
                << ",\"hostVisibleBytes\":" << resources.hostVisibleBytes
                << ",\"completedRtEvidence\":" << completedJson << '}';
    }
    QueuePresentedEntryPlay(context, true);
    if (!context.entryPlayHandoffPending || !context.simulationPaused)
        return fail(13, "Gameplay advanced before Entry replacement");
    if (!context.entryLoadingVisible ||
        !IsWindowVisible(GetDlgItem(context.windowHandle, kEntryLoadingIndicatorId)))
        return fail(18, "Real Entry replacement has no visible native loading indicator");
    std::vector<double> timings;
    if (!ApplyPendingSceneReplacement(context, capabilities, timings))
        return fail(14, "Play scene replacement failed");
    FinishPendingEntryPlay(context);
    bool gameplayPresented = false;
    if (context.entryMenuVisible || context.simulationPaused ||
        context.rtScene.Profile() != horde::vulkan::raytracing::RtSceneProfile::Showcase ||
        !RenderFrame(context, clear, gameplayPresented) || !gameplayPresented)
        return fail(15, "Play did not return to functioning Showcase");
    FinishEntryLoadingAfterPresentation(context, gameplayPresented);
    if (context.entryLoadingVisible ||
        IsWindowVisible(GetDlgItem(context.windowHandle, kEntryLoadingIndicatorId)))
        return fail(19, "Loading indicator outlived the first functioning Showcase present");
    const VkResult handoffIdle = vkDeviceWaitIdle(context.device);
    if (handoffIdle != VK_SUCCESS || !CompleteRtEvidenceAfterDeviceIdle(context, handoffIdle))
        return fail(16, "Gameplay handoff frame did not complete");
    const auto handoff = context.rtFrameEvidence.PublishedStateByValue();
    if (!handoff.presented || !handoff.hasCompletedEvidence)
        return fail(17, "Gameplay handoff lacks completed presentation");
    capabilities.rtScene.presented = true;
    capabilities.performance.gpuRt = context.gpuRtTiming;
    receipt << "],\"graphicsNoUseBackReturnedToEntry\":true,\"savedGraphicsPreserved\":true,"
            << "\"playHandoffPresented\":true,\"playHandoffSceneEpoch\":"
            << handoff.sceneEpoch << ",\"complete\":true}\n";
    return WriteReportFile(directory / "entry-menu-capture.json", receipt.str()) ? 0 : 9;
}

#include "platform/windows/WindowsGraphicsPreviewCapture.inl"
#if defined(_DEBUG)
#include "platform/windows/WindowsPreviewMeshPairCapture.inl"
#include "platform/windows/WindowsOutputResizeValidation.inl"
#include "platform/windows/WindowsMotionEvidenceValidation.inl"
#include "platform/windows/WindowsPlayerDodgeCapture.inl"
#include "platform/windows/WindowsEnvironmentSurveyCapture.inl"
#endif
#endif

#include "windows_campaign_validation.inl"

int RunDiagnosticSwapchainWindow(HWND hWnd,
                                 horde::vulkan::DeviceCapabilities& capabilities,
                                 const std::filesystem::path& textReportPath,
                                 const std::filesystem::path& jsonReportPath,
                                 const std::filesystem::path* captureDirectory,
                                 const std::string* developmentCheckpoint,
                                 const bool requireRayQueryCompute,
                                 const bool unattendedBenchmark,
                                 const horde::gameplay::BenchmarkWorkload benchmarkWorkload,
                                 const horde::vulkan::raytracing::RtWorkloadPreset benchmarkRtWorkloadPreset,
                                 const bool anatomicalPlayerMount,
                                 const bool graphicsPreviewCapture,
                                 const bool outputResizeValidation,
                                 const std::string& nativeMotionScenario,
                                 const horde::vulkan::raytracing::RtWorkloadPreset nativeMotionRtWorkloadPreset,
                                 const std::optional<horde::graphics::DustQuality> captureDustQuality,
                                 OpeningBenchmarkTransfer* openingBenchmark = nullptr)
{
    horde::vulkan::RetirementOwner<VulkanSurfaceContext> renderOwner(
        std::make_unique<VulkanSurfaceContext>(), DestroyRenderContext, DetachRenderContextHost);
    auto& context = *renderOwner.Get();
    if(auto* compatibility=CompatibilityFor(hWnd))compatibility->snapshot=compatibility->probeSnapshot;
    context.openingBenchmark = openingBenchmark;
    context.openingBenchmarkOwner = openingBenchmark && openingBenchmark->owner.Phase()==OpeningBenchmarkOwnerPhase::Benchmark;
    context.openingBenchmarkReturnPending = openingBenchmark && openingBenchmark->owner.Phase()==OpeningBenchmarkOwnerPhase::ReturningEntry;
    const bool automatedBenchmark = unattendedBenchmark || context.openingBenchmarkOwner;
    // Ordinary interactive launches enter the accepted lantern menu. Explicit
    // captures/checkpoints keep their requested scene; the slice flag below
    // remains available for dedicated menu inspection.
    context.entryMenuVisible = captureDirectory == nullptr && developmentCheckpoint == nullptr &&
        !automatedBenchmark && !graphicsPreviewCapture && !outputResizeValidation &&
        nativeMotionScenario.empty();
    int entryArgumentCount = 0;
    LPWSTR *entryArguments = CommandLineToArgvW(GetCommandLineW(), &entryArgumentCount);
    if (entryArguments)
    {
        for (int argument = 1; argument < entryArgumentCount; ++argument)
        {
#if defined(_DEBUG) || HORDE_RT_CHAPTER_MEASUREMENT
            if (std::wstring_view(entryArguments[argument]) == L"--development-rescue-journey")
            { context.developmentRescueJourney = true; context.entryMenuVisible = false; }
            if (std::wstring_view(entryArguments[argument]) == L"--development-rescue-journey-staged")
            { context.developmentRescueJourney = true; context.stagedWorldPreparation = true; context.entryMenuVisible = false; }
            if (std::wstring_view(entryArguments[argument]) == L"--development-world-route")
                context.developmentWorldRoute = true;
            if (std::wstring_view(entryArguments[argument]) == L"--development-world-route-staged")
            { context.developmentWorldRoute = true; context.stagedWorldPreparation = true; }
            if (std::wstring_view(entryArguments[argument]) == L"--development-vertical-proof")
                context.developmentVerticalProof = true;
            if (std::wstring_view(entryArguments[argument]) == L"--development-combat-practice")
            { context.developmentCombatPractice = true; context.entryMenuVisible = false; }
            if (std::wstring_view(entryArguments[argument]) == L"--development-keeper-practice")
            { context.developmentKeeperPractice = true; context.entryMenuVisible = false; }
#endif
            if (std::wstring_view(entryArguments[argument]) == L"--entry-menu-slice")
                context.entryMenuVisible = !automatedBenchmark;
        }
        LocalFree(entryArguments);
    }
    if (context.developmentVerticalProof || context.developmentWorldRoute || context.developmentRescueJourney) context.entryMenuVisible = false;
    BOOL animations = TRUE;
    if (SystemParametersInfoA(SPI_GETCLIENTAREAANIMATION, 0, &animations, 0))
        context.reducedMotionEnabled = !animations;
    if (context.entryMenuVisible)
    {
        context.sceneProfile = horde::vulkan::raytracing::RtSceneProfile::EntryMenu;
        context.entryMenu.SetReducedMotion(context.reducedMotionEnabled);
    }
    context.graphicsPreviewCapture = graphicsPreviewCapture;
#if defined(_DEBUG)
    context.previewMeshPair=ParseCaptureLaunchOptions().previewMeshPair;
    context.forestRouteProof=ParseCaptureLaunchOptions().forestRouteProof;
#if HORDE_RT_FRAME_STAGE_PROBE
    context.captureGpuStages=ParseCaptureLaunchOptions().captureGpuStages;
#endif
#endif
    context.outputResizeValidation = outputResizeValidation;
    context.nativeMotionValidation = !nativeMotionScenario.empty();
    context.environmentSurveyActive=ParseCaptureLaunchOptions().environmentSurvey.requested;
    context.environmentSurveyWaterDisabled=ParseCaptureLaunchOptions().environmentSurvey.waterDisabled;
    context.environmentSurveyDebrisDisabled=ParseCaptureLaunchOptions().environmentSurvey.debrisDisabled;
    context.environmentSurveyProfile=ParseCaptureLaunchOptions().environmentSurvey.profile;
#if defined(_DEBUG)
    context.playerDodgeInspectionDisabled=ParseCaptureLaunchOptions().environmentSurvey.dodgeDisabled;
#endif
#if defined(_DEBUG)
    if(captureDirectory != nullptr) context.skyAttribution=ParseCaptureLaunchOptions().skyAttribution;
    if (context.nativeMotionValidation) context.motionRequestedRtPreset = nativeMotionRtWorkloadPreset;
#endif
    if (graphicsPreviewCapture)
        context.sceneProfile = horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview;
    const bool explicitComparison = developmentCheckpoint != nullptr &&
        horde::vulkan::raytracing::PlayerRenderRouteForCheckpoint(*developmentCheckpoint) !=
            horde::vulkan::raytracing::kProductionPlayerRenderRoute;
    if (!graphicsPreviewCapture) {
        auto simulationConfig=explicitComparison?horde::gameplay::simulation::GameSimulationConfig{}:
            horde::gameplay::simulation::ProductionGameSimulationConfig();
#if defined(_DEBUG)
        // Only the strictly isolated existing route validator may opt in.
        // Normal application configuration, settings and save schema stay OFF.
        if(gRouteHiddenValidation && gRouteKitTurnStartValidation)simulationConfig.companionTurnStart=true;
#endif
        context.simulation=horde::gameplay::simulation::GameSimulation(simulationConfig);
    }
    if (context.developmentRescueJourney)
    {
        context.simulation.SetDevelopmentRescueJourney(true,true);
        context.developmentWorldRoute = true; // Scene route only; simulation route owns no teleport.
    }
    if (context.developmentVerticalProof)
        context.simulation.SetDevelopmentSupportFixture(true, 1u);
    if(context.developmentWorldRoute && !context.developmentRescueJourney)
    {
        context.simulation.SetDevelopmentWorldRoute(true,context.stagedWorldPreparation);
        context.cameraX=context.simulation.Snapshot().playerX;
        context.cameraZ=context.simulation.Snapshot().playerZ;
        context.cameraYaw=context.simulation.Snapshot().playerYawRadians;
        context.cameraPitch=context.simulation.Snapshot().playerPitchRadians;
    }
    // The former opt-in argument remains compatible with recorded capture
    // commands; every normal application now uses this accepted profile.
    (void)anatomicalPlayerMount;
    context.windowHandle = hWnd;
    context.capabilitySnapshot = &capabilities;
    context.unattendedBenchmark = automatedBenchmark || ParseCaptureLaunchOptions().measureChapterShippingRoute;
    const auto benchmarkProfileLaunch = ParseCaptureLaunchOptions().benchmark;
    context.benchmarkProfileCustom = context.openingBenchmarkOwner &&
        openingBenchmark->owner.Choice()==OpeningBenchmarkChoice::Custom;
    context.benchmarkProfileRequested = context.openingBenchmarkOwner || (unattendedBenchmark && benchmarkProfileLaunch.profile ==
        horde::gameplay::BenchmarkProfileId::WindowsProvisionalV1);
    if (context.benchmarkProfileRequested)
        context.benchmarkProfile = std::make_unique<horde::platform::windows::WindowsBenchmarkProfileSession>();
    if (developmentCheckpoint != nullptr)
    {
        context.developmentCheckpoint = *developmentCheckpoint;
        const auto* proof = horde::gameplay::FindDevelopmentCheckpoint(*developmentCheckpoint);
        context.developmentVerticalProof = proof != nullptr && proof->developmentSupportFixture;
        context.developmentWorldRoute = proof != nullptr && proof->developmentWorldRoute;
        context.stagedWorldPreparation = proof != nullptr && proof->stagedWorldPreparation;
    }
    if (!gCampaignHiddenValidation && !context.environmentSurveyActive && !graphicsPreviewCapture && !outputResizeValidation && !context.nativeMotionValidation) LoadSettings(context);
#if defined(_DEBUG)
    // Explicit owner-playtest setup; ordinary launches keep their saved choice.
    if (GetCommandLineW() != nullptr) {
        int teachingArgc = 0;
        auto teachingArgv = CommandLineToArgvW(GetCommandLineW(), &teachingArgc);
        if (teachingArgv) {
            for (int i = 1; i < teachingArgc; ++i)
                if (std::wstring_view(teachingArgv[i]) == L"--development-teaching-slowdown") {
                    context.combatTeachingEnabled = true;
                    context.combatTeachingSlowdown = true;
                }
            LocalFree(teachingArgv);
        }
    }
#endif

    if (openingBenchmark && openingBenchmark->hasPersonal) {
        SetBenchmarkProfileIntent(context,openingBenchmark->personalGraphics);
        context.savedGraphics=openingBenchmark->savedGraphics;
        context.fullscreen=openingBenchmark->fullscreen;
        context.windowedPlacement=openingBenchmark->windowedPlacement;
    }
    if (context.developmentCombatPractice)
        context.simulation.BeginCombatPractice(horde::gameplay::EnemyKind::Skeleton);
    else if (context.developmentKeeperPractice)
        context.simulation.BeginCombatPractice(horde::gameplay::EnemyKind::Lich);
    if (captureDirectory != nullptr && captureDustQuality.has_value())
        context.requestedDustQuality = *captureDustQuality;
#if defined(_DEBUG)
    const RtLabDebugLaunchOptions rtLabDebug = ParseRtLabDebugLaunchOptions();
    if (rtLabDebug.requested && !graphicsPreviewCapture && !outputResizeValidation && !context.nativeMotionValidation)
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
        context.sfxVolumePercent = 0;
        context.simulationPaused = true;
        context.pauseMenuVisible = false;
        context.graphicsPreviewDelta = 0.0;
    }
#if defined(_DEBUG)
    if (context.nativeMotionValidation)
    {
        const auto now = horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr);
        const auto runId = std::string("native-motion-") + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(now);
        if (!horde::gameplay::validation::ParseMotionScenario(nativeMotionScenario, context.motionRequestedScenario) ||
            !context.motionLedger.Begin(runId, context.motionRequestedScenario)) return 2;
        // The sole checkpoint seed and scenario clock start only after actual
        // user focus arms the run. Native evidence starts no audio at any point.
    }
#endif
    SetCompatibilityAttemptStage(hWnd,horde::reporting::CompatibilityStage::VulkanInstance);
    if (!CreateInstance(context.instance, context.presentSurfaceSupport))
    {
        return 1;
    }

    SetCompatibilityAttemptStage(hWnd,horde::reporting::CompatibilityStage::Surface);
    if (!CreateSurface(context.instance, hWnd, context.surface))
    {
        (void)renderOwner.Retire();
        return 1;
    }

    SetCompatibilityAttemptStage(hWnd,horde::reporting::CompatibilityStage::BackendSelection);
    uint32_t physicalDeviceCount = 0u;
    if (vkEnumeratePhysicalDevices(context.instance, &physicalDeviceCount, nullptr) != VK_SUCCESS || physicalDeviceCount == 0u)
    {
        std::cerr << "No physical devices found for diagnostic swapchain.\n";
        (void)renderOwner.Retire();
        return 1;
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(context.instance, &physicalDeviceCount, physicalDevices.data());

    for (const VkPhysicalDevice candidate : physicalDevices)
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);
        const horde::vulkan::DeviceIdentity candidateIdentity{
            properties.deviceName, properties.vendorID, properties.deviceID,
            properties.driverVersion, properties.apiVersion};
        if (horde::vulkan::raytracing::SameRtDeviceIdentity(capabilities.identity, candidateIdentity))
        {
            context.physicalDevice = candidate;
            break;
        }
    }

    if (context.physicalDevice == VK_NULL_HANDLE)
    {
        std::cerr << "No physical device matches the probed GPU, driver and API identity.\n";
        (void)renderOwner.Retire();
        return 1;
    }

    if (!FindGraphicsAndPresentQueueFamily(context.physicalDevice, context.surface, context.graphicsQueueFamilyIndex))
    {
        (void)renderOwner.Retire();
        return 1;
    }

    context.executionBackend = horde::vulkan::raytracing::SelectRtExecutionBackend(
        capabilities, true, requireRayQueryCompute);
    horde::vulkan::BeginRtBackendSelection(capabilities.rtScene, context.executionBackend);
    if(auto* owner=CompatibilityFor(hWnd)) {
        using namespace horde::reporting;
        owner->snapshot.rtPresented=false;
        owner->snapshot.backend=context.executionBackend==horde::vulkan::RtExecutionBackend::RayTracingPipeline?CompatibilityBackend::RayTracingPipeline:
            context.executionBackend==horde::vulkan::RtExecutionBackend::RayQueryCompute?CompatibilityBackend::RayQueryCompute:CompatibilityBackend::NotSelected;
        owner->snapshot.lastStage=CompatibilityStage::BackendSelection;
        if(owner->snapshot.backend==CompatibilityBackend::NotSelected && owner->snapshot.outcome!=CompatibilityOutcome::Unsupported)
            owner->snapshot.outcome=CompatibilityOutcome::StartupFailure;
        else if(owner->snapshot.backend!=CompatibilityBackend::NotSelected)owner->snapshot.outcome=CompatibilityOutcome::CapabilityOnly;
    }
    const bool selectedTextWritten = WriteReportFile(
        textReportPath, horde::vulkan::BuildCapabilityTextReport(capabilities));
    const bool selectedJsonWritten = WriteReportFile(
        jsonReportPath, horde::vulkan::BuildCapabilityJsonReport(capabilities));
    if (!selectedTextWritten || !selectedJsonWritten)
    {
        std::cerr << "Failed to persist selected RT backend diagnostics.\n";
    }
    if (requireRayQueryCompute && context.executionBackend != horde::vulkan::RtExecutionBackend::RayQueryCompute)
    {
        std::cerr << "The required hardware RayQuery compute backend is unavailable; no other backend will be selected.\n";
        (void)renderOwner.Retire();
        return 2;
    }
    context.useRtPath = context.executionBackend != horde::vulkan::RtExecutionBackend::Unsupported;
    SetCompatibilityAttemptStage(hWnd,horde::reporting::CompatibilityStage::LogicalDevice);
    if (!CreateLogicalDevice(context.physicalDevice, context.instance, context.graphicsQueueFamilyIndex, context.executionBackend,
        context.device, context.graphicsQueue, context.presentSurfaceSupport, context.presentCompletionMode))
    {
        (void)renderOwner.Retire();
        return 1;
    }
    const VkPipelineCacheCreateInfo pipelineCacheInfo{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    const VkResult pipelineCacheResult = context.useRtPath
        ? vkCreatePipelineCache(context.device, &pipelineCacheInfo, nullptr, &context.pipelineCache)
        : VK_NOT_READY;
    if (pipelineCacheResult != VK_SUCCESS) context.pipelineCache = VK_NULL_HANDLE;
    std::cerr << "HORDE_PIPELINE_CACHE_CREATE attempted=" << (context.useRtPath ? 1 : 0)
              << " result=" << pipelineCacheResult
              << " available=" << (context.pipelineCache != VK_NULL_HANDLE ? 1 : 0) << '\n';
    capabilities.diagnostics.push_back(horde::vulkan::PresentCompletionDiagnostic(context.presentCompletionMode));

    SetCompatibilityAttemptStage(hWnd,horde::reporting::CompatibilityStage::Swapchain);
    if (!CreateSwapchain(context, hWnd))
    {
        (void)renderOwner.Retire();
        return 1;
    }
    #if defined(_DEBUG)
    if(context.environmentSurveyActive && (!context.rtScene.SetFloorDebrisDiagnosticEnabled(
            !context.environmentSurveyDebrisDisabled) || !context.rtScene.SetRegionalWaterDiagnosticEnabled(
            !context.environmentSurveyWaterDisabled)))
    {std::cerr<<"Environment survey geometry selection was not admitted before scene initialization.\n";(void)renderOwner.Retire();return 1;}
    #endif
    SetCompatibilityAttemptStage(hWnd,horde::reporting::CompatibilityStage::RtScene);
    if (!InitialiseRtSceneForSwapchain(context))
    {
        (void)renderOwner.Retire();
        return 1;
    }

    context.controlsEnabled = context.useRtPath && context.rtScene.IsReady();
#if defined(_DEBUG)
    if(captureDirectory != nullptr)
        std::cerr << "HORDE_CAPTURE_HOST scene-admitted" << std::endl;
#endif
    if (context.controlsEnabled)
    {
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&context));
#if defined(_DEBUG)
        if(captureDirectory != nullptr)
            std::cerr << "HORDE_CAPTURE_HOST callbacks-attached" << std::endl;
#endif
        TraceOpeningMenuStage("callbacks-attached");
        if (context.entryMenuVisible) {
            TraceOpeningMenuStage("fonts-begin");ApplyDpiScaledFonts(hWnd);TraceOpeningMenuStage("fonts-end");
        }
        if(captureDirectory==nullptr && !automatedBenchmark && !graphicsPreviewCapture && !outputResizeValidation && !context.nativeMotionValidation) {
            const auto directory=gCampaignHiddenValidation?gCampaignValidationDirectory:WindowsCampaignDirectory();
            std::error_code campaignDirectoryError;
            if(!directory.empty())std::filesystem::create_directories(directory,campaignDirectoryError);
            if(directory.empty() || campaignDirectoryError)context.campaignStorageWarning="Campaign storage unavailable. New games can start, but saves cannot be guaranteed. Recheck records before writing.";
            {
                context.campaignIsolated=gCampaignHiddenValidation || gAcceptanceProfile.Active();
                horde::gameplay::campaign::storage::FaultInjector* campaignFaults=nullptr;
#if defined(_DEBUG)
                if(gCampaignHiddenValidation)campaignFaults=&gHiddenCampaignFaults;
#endif
                context.campaign=std::make_unique<horde::gameplay::campaign::PlaySession>(context.simulation,directory,campaignFaults);
                context.campaignMenu=std::make_unique<horde::platform::windows::WindowsCampaignMenu>([&context](auto command){
                    QueueCampaignCommand(context,command);
                });
                TraceOpeningMenuStage("campaign-controls-begin");
                if(!context.campaignMenu->Create(hWnd)){(void)renderOwner.Retire();return 1;}
                // These controls are created after the initial menu children.
                // Reuse their keyboard forwarding and controller focus outline.
                for(int id:{4110,4111,4112,4113,4114,4115,4120,4121,4122,4123})
                    InstallControllerFocusOutline(GetDlgItem(hWnd,id));
                TraceOpeningMenuStage("campaign-controls-end");
                context.campaignPhase=horde::platform::windows::WindowsCampaignMenu::Phase::Ready;
            }
        }
        if (captureDirectory == nullptr && !gCampaignHiddenValidation)
        {
            // Normal gameplay and benchmark runs include music. Frozen image
            // capture mode deliberately has no playback, as with existing SFX.
            TraceOpeningMenuStage("music-begin");
            try
            {
                context.musicPlayback = std::make_unique<horde::platform::windows::WindowsMusicPlayback>(
                    ResolveAssetRoot(), &LogWindowsAudio);
            }
            catch (const std::exception& error)
            {
                LogWindowsAudio(std::string("Music could not initialize; RT/SFX unchanged: ") + error.what());
            }
            TraceOpeningMenuStage("music-end-overlay-begin");
            ApplyOverlayState(context);
            TraceOpeningMenuStage("overlay-end");
            {
                RECT entryClient{};
                GetClientRect(hWnd, &entryClient);
                LayoutOverlayControls(hWnd, entryClient.right, entryClient.bottom);
            }
            if (!automatedBenchmark && !gOpeningBenchmarkMenuValidation)
                horde::platform::windows::BeginGitHubReleaseUpdateCheck(
                    hWnd, HORDE_RT_DISPLAY_VERSION, false);
#if defined(_DEBUG)
            if (context.rtLabDebugInjection) OpenRtLab(context);
            else
#endif
                FocusGameControl(GetDlgItem(hWnd, kResumeButtonId));
        }
    }

#if defined(_DEBUG) || HORDE_RT_CHAPTER_MEASUREMENT
    if (captureDirectory != nullptr)
    {
        int captureResult =
#if defined(_DEBUG)
            context.environmentSurveyActive ? RunEnvironmentSurveyCapture(context, capabilities, *captureDirectory)
            : context.entryMenuVisible ? RunEntryMenuCapture(context, capabilities, *captureDirectory)
            : context.nativeMotionValidation
                ? RunNativeMotionEvidence(context, capabilities, *captureDirectory)
            : outputResizeValidation
                ? RunOutputResizeValidation(context, capabilities, *captureDirectory)
            : graphicsPreviewCapture
                ? (context.previewMeshPair!=horde::platform::windows::PreviewMeshPairCaptureMode::None
                    ? RunPreviewMeshPairCapture(context,capabilities,*captureDirectory)
                    : RunGraphicsPreviewCapture(context, capabilities, *captureDirectory))
                :
#endif
                RunShowcaseCapture(context, capabilities, *captureDirectory);
#if defined(_DEBUG)
        if(captureResult==0 && ParseCaptureLaunchOptions().validateForestContentCandidate) {
            std::string forestError;
            if(!RunForestContentPreflight(context,*captureDirectory,forestError)) {
                std::cerr<<"Forest content preflight failed: "<<forestError<<'\n';captureResult=1;
            }
        }
#endif
        if(captureResult==0 && (ParseCaptureLaunchOptions().validateChapterRoundTrip || ParseCaptureLaunchOptions().chapterLandingOnly)) {
            std::string journeyError;
            if(!RunChapterRoundTripCapture(context,*captureDirectory,journeyError,ParseCaptureLaunchOptions().chapterLandingOnly)) {
                std::cerr<<"Chapter two-way capture failed: "<<journeyError<<'\n';captureResult=1;
            }
        }
        if(captureResult==0 && ParseCaptureLaunchOptions().measureChapterShippingRoute) {
            std::string measurementError;
            if(!RunChapterShippingRouteMeasurement(context,*captureDirectory,measurementError)) {
                std::cerr<<"Chapter Shipping measurement failed: "<<measurementError<<'\n';captureResult=1;
            }
        }
        if (captureResult == 0 && capabilities.rtScene.presented)
        {
            capabilities.rtScene.executionBackend = context.rtScene.ExecutionBackend();
            capabilities.rtScene.status = "Presented via swapchain";
            capabilities.rtScene.geometry = context.environmentSurveyActive
                ? "Debug environment-only terrain shore survey; no gameplay or equipment acceptance"
                : graphicsPreviewCapture
                ? "Compact authored graphics preview with actual Idle skeleton, lantern, torch, water, mirror and material surfaces"
                : "Complete Horde showcase route with sequential animated skeleton and staff-lit lich";
            capabilities.rtScene.dispatchWidth = context.rtScene.DispatchExtent().width;
            capabilities.rtScene.dispatchHeight = context.rtScene.DispatchExtent().height;
            capabilities.performance.internalRenderWidth = capabilities.rtScene.dispatchWidth;
            capabilities.performance.internalRenderHeight = capabilities.rtScene.dispatchHeight;
            auto& presentationDiagnostics = capabilities.diagnostics;
            presentationDiagnostics.erase(
                std::remove(presentationDiagnostics.begin(), presentationDiagnostics.end(),
                            "Internal render resolution: not measured yet."),
                presentationDiagnostics.end());
            const auto publication = context.rtFrameEvidence.PublishedStateByValue();
            const auto* evidence = context.rtFrameEvidenceInitialised && context.rtFrameEvidence.ObserverAvailable()
                ? &publication : nullptr;
            const bool textReportWritten = WriteReportFile(
                textReportPath, horde::vulkan::BuildCapabilityTextReport(capabilities, evidence));
            const bool jsonReportWritten = WriteReportFile(
                jsonReportPath, horde::vulkan::BuildCapabilityJsonReport(capabilities, evidence));
            if (!textReportWritten || !jsonReportWritten)
            {
                std::cerr << "Failed to refresh capability reports after successful showcase capture.\n";
                captureResult = 1;
            }
        }
        if (!renderOwner.Retire()) captureResult = 1;
        return captureResult;
    }
#else
    (void)captureDirectory;
#endif

#if defined(_DEBUG)
    if(gCampaignHiddenValidation){
        context.sfxVolumePercent=context.musicVolumePercent=context.voiceVolumePercent=0;
        const int result=gRouteHiddenValidation?RunHiddenRouteValidation(context):
            gLookoutHiddenValidation?RunHiddenLookoutValidation(context,capabilities):RunHiddenCampaignValidation(context);
        return renderOwner.Retire()?result:1;
    }
#endif
    const VkClearColorValue clearColor = ClearColorForMode(capabilities.rtMode);
    if (context.openingBenchmarkReturnPending) {
        horde::gameplay::BenchmarkProfileConfiguration actual;
        const bool presented=CaptureBenchmarkProfileCompletion(context,clearColor,false,false) &&
            ReadBenchmarkProfileActual(context,actual,false) && actual==openingBenchmark->entryActual;
        const bool admitted=openingBenchmark->owner.EntryPresented(presented);
        if(!admitted) {(void)renderOwner.Retire();return 1;}
        context.openingBenchmarkReturnPending=false;
        openingBenchmark->cancelPending=false;
        std::cerr<<"HORDE_OPENING_BENCHMARK entry-presented campaignAttached="<<(context.campaign?1:0)<<'\n';
        context.entryMoreVisible=true;
        // Publish the frozen result only after the successor actually presents.
        // The old renderer has already retired; cancellation/failure has no result.
        if(!openingBenchmark->completedReport.empty()) {
            context.benchmarkReport=std::move(openingBenchmark->completedReport);
            context.benchmarkSummary=std::move(openingBenchmark->completedSummary);
            openingBenchmark->completedReport.clear();
            openingBenchmark->completedSummary.reset();
            context.pauseMenuVisible=false;
            context.benchmarkReportVisible=true;
            SetWindowTextA(GetDlgItem(hWnd,kEditControlId),WindowSafeText(context.benchmarkReport).c_str());
        }
        ApplyOverlayState(context);
        RECT client{};GetClientRect(hWnd,&client);LayoutOverlayControls(hWnd,client.right,client.bottom);
        FocusGameControl(GetDlgItem(hWnd,context.benchmarkReportVisible?kEditControlId:kRunBenchmarkButtonId));
        if(openingBenchmark->closeRequested) PostMessageA(hWnd,WM_CLOSE,0,0);
    }
    if(context.openingBenchmarkOwner) {
        openingBenchmark->completedReport.clear();
        openingBenchmark->completedSummary.reset();
        context.benchmarkProfileCancelRequested=context.benchmarkProfileCancelRequested || openingBenchmark->cancelPending;
        context.benchmarkProfileCloseRequested=context.benchmarkProfileCloseRequested || openingBenchmark->closeRequested;
    }
    if (automatedBenchmark)
    {
        if (!context.controlsEnabled)
        {
            (void)renderOwner.Retire();
            return 1;
        }
        if (context.benchmarkProfileRequested)
        {
            const auto validation = context.openingBenchmarkOwner ? std::string{} : benchmarkProfileLaunch.profileValidation;
            const bool applied = ApplyWindowsBenchmarkProfile(context, clearColor, validation);
            if (!applied || !validation.empty())
            {
                bool framesOkay = true;
                if (applied && !validation.empty())
                {
                    for (unsigned frame = 0; frame < 3 && framesOkay; ++frame)
                    {
                        framesOkay = CaptureBenchmarkProfileCompletion(context, clearColor, false, true);
                        horde::gameplay::BenchmarkProfileConfiguration actual;
                        framesOkay = framesOkay && ReadBenchmarkProfileActual(context, actual, true) &&
                            context.benchmarkProfile->ObserveMeasuring(actual);
                        if (framesOkay) ++context.benchmarkProfileValidationFrames;
                    }
                    framesOkay = framesOkay && CaptureBenchmarkProfileImage(context,
                        textReportPath.parent_path() / "benchmark-profile-applied.png");
                    if (!framesOkay) context.benchmarkProfileError += " Bounded applied RT frames/readback failed.";
                }
                if (applied && validation == "cancel-measure") SendMessageA(hWnd, WM_KEYDOWN, VK_ESCAPE, 0);
                if (applied && validation == "restart-measure") SendMessageA(hWnd, WM_COMMAND, kMenuRestartId, 0);
                if (applied && validation == "close-measure") {
                    SendMessageA(hWnd, WM_SYSKEYDOWN, VK_F4, 0);
                    framesOkay = framesOkay && context.benchmarkProfileCloseRequested && context.benchmarkProfileCancelRequested;
                    SendMessageA(hWnd, WM_CLOSE, 0, 0);
                }
                const bool restored = RestoreWindowsBenchmarkProfile(context, clearColor, false);
                const bool expectedCancellation = validation != "apply-restore" && validation != "stale-owner" && !validation.empty();
                const bool validated = !validation.empty() && restored && context.benchmarkProfile->Restored() &&
                    (applied || validation == "cancel-apply") &&
                    context.benchmarkProfileCancelRequested == expectedCancellation &&
                    (validation != "stale-owner" || context.benchmarkProfileStaleOwnerRejected) && framesOkay &&
                    context.benchmarkProfileError.empty();
                const bool imageSaved = !context.benchmarkProfile->Restored() || CaptureBenchmarkProfileImage(context,
                    textReportPath.parent_path() / "benchmark-profile-restored.png");
                if (!imageSaved) context.benchmarkProfileError += " Restored RT readback failed.";
                context.benchmarkProfileValidationPassed = validated && imageSaved;
                const bool saved = WriteWindowsBenchmarkProfileReceipt(context, textReportPath.parent_path(), validation);
                const bool validationPassed = context.benchmarkProfileValidationPassed;
                if(context.openingBenchmarkOwner) {
                    openingBenchmark->status=context.benchmarkProfileCancelRequested ?
                        "Benchmark cancelled. Personal graphics restored." : "Benchmark could not start. No result admitted.";
                    openingBenchmark->statusTitle=context.benchmarkProfileCancelRequested ? "CANCELLED" : "NO RESULT";
                    openingBenchmark->closeRequested=openingBenchmark->closeRequested || context.benchmarkProfileCloseRequested;
                    const bool seedsKept=CarryOpeningBenchmarkSeeds(*openingBenchmark,context);
                    const bool retired=renderOwner.Retire();
                    return openingBenchmark->owner.BenchmarkRetired(restored && seedsKept,retired)?0:1;
                }
                const bool retired = renderOwner.Retire();
                return validationPassed && saved && retired ? 0 : 1;
            }
        }
        if(context.openingBenchmarkOwner)std::cerr<<"HORDE_OPENING_BENCHMARK applied fixed="
            <<(!context.benchmarkProfileCustom)<<" campaignAttached="<<(context.campaign?1:0)<<'\n';
        StartBenchmark(context, benchmarkWorkload, false, benchmarkRtWorkloadPreset); // No FPS observer.
    }
    TraceOpeningMenuStage("ui-ready");
    MSG message{};
    bool running = true;
    bool renderFailed = false;
    horde::graphics::ForegroundPauseRenderCadence foregroundPauseCadence;
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
        if (context.benchmarkProfileActive && context.benchmarkProfileCancelRequested)
        {
            CancelBenchmark(context, false, "profile-lifecycle-interrupted");
            break;
        }
        if (automatedBenchmark && !context.benchmark.IsRunning())
        {
            CompleteBenchmark(context, capabilities, textReportPath.parent_path());
            break;
        }

        if (openingBenchmark && openingBenchmark->owner.Phase()==OpeningBenchmarkOwnerPhase::Queued) {
            auto eligible=OpeningBenchmarkEligibilityFor(context);
            if(!eligible.Eligible()) {
                (void)openingBenchmark->owner.CancelQueued();
            } else {
                horde::gameplay::BenchmarkProfileConfiguration actual;
                if(!CaptureBenchmarkProfileCompletion(context,clearColor,false,false) ||
                   !ReadBenchmarkProfileActual(context,actual,false)) {
                    (void)openingBenchmark->owner.CancelQueued();
                    openingBenchmark->status="Current Entry RT state could not be acknowledged. No benchmark started.";
                    if(!IsWindow(hWnd) || context.benchmarkProfileCloseRequested) {
                        running=false;break; // The completion pump may have consumed WM_QUIT.
                    }
                } else {
                    eligible=OpeningBenchmarkEligibilityFor(context); // completion pumps messages
                    if(eligible.Eligible() && openingBenchmark->owner.Phase()==OpeningBenchmarkOwnerPhase::Queued) {
                        openingBenchmark->personalGraphics=BenchmarkProfileIntent(context);
                        openingBenchmark->savedGraphics=context.savedGraphics;
                        openingBenchmark->entryActual=actual;
                        openingBenchmark->fullscreen=context.fullscreen;
                        openingBenchmark->windowedPlacement=context.windowedPlacement;
                        openingBenchmark->hasPersonal=true;
                        const bool seedsKept=CarryOpeningBenchmarkSeeds(*openingBenchmark,context);
                        const bool retired=renderOwner.Retire();
                        const bool admitted=openingBenchmark->owner.RecheckAndRetireEntry(eligible,retired && seedsKept);
                        std::cerr<<"HORDE_OPENING_BENCHMARK entry-retired="<<retired<<" successorAdmitted="<<admitted<<'\n';
                        return admitted?0:1;
                    }
                    (void)openingBenchmark->owner.CancelQueued();
                }
            }
        }
        PumpCampaign(context);
        if(context.entryMenuVisible && context.entryMoreVisible && !context.benchmarkChooserVisible)
            EnableWindow(GetDlgItem(hWnd,kRunBenchmarkButtonId),OpeningBenchmarkEligibilityFor(context).Eligible());
        if(context.campaignRetirementUnproved){renderFailed=true;break;}
        const auto ownerFrameStart = std::chrono::steady_clock::now();
        const bool graphicsTransition = context.sceneProfileDirty || context.glassGeometryDirty ||
            context.renderScaleDirty || context.graphicsCommand.has_value();
        if (!ApplyPendingSceneReplacement(context, capabilities, timingSamples))
        { renderFailed = true; break; }
        FinishPendingEntryPlay(context);
        const auto loopProfile = context.rtScene.Profile();
        const bool isLiveAnimatedProfile = loopProfile != horde::vulkan::raytracing::RtSceneProfile::Showcase;
        if (isLiveAnimatedProfile && (IsIconic(hWnd) || GetForegroundWindow() != hWnd))
        {
            StopMenuAmbience(context);
            context.graphicsPreviewLastFrame = {}; context.graphicsPreviewLastSample = {};
            context.graphicsConfirmationTick = GetTickCount64();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue; // Background Entry/Preview preserves the existing no-render behavior.
        }

        if (!ApplyPendingOutputResize(context, capabilities, timingSamples))
        { renderFailed = true; break; }

        if(context.lookoutObjective && context.lookoutObjective->Visible() &&
           !context.lookoutObjective->Refresh(CurrentLookoutObjectiveView(context))) {
            ApplyOverlayState(context);
            RECT client{};GetClientRect(hWnd,&client);LayoutOverlayControls(hWnd,client.right,client.bottom);
        }
        if (!context.graphicsPreviewCapture && !context.outputResizeValidation && !context.nativeMotionValidation && !context.benchmarkProfileActive)
            PollDesktopController(context);

        const auto profile = context.rtScene.Profile();
        const auto foregroundNow = std::chrono::steady_clock::now();
        if (profile != horde::vulkan::raytracing::RtSceneProfile::Showcase &&
            context.graphicsPreviewLastFrame.time_since_epoch().count() != 0 &&
            !graphicsTransition && !context.entryPlayQueued && !context.entryPlayHandoffPending &&
            !context.rtLabVisible && !context.graphicsPreviewCapture && !context.outputResizeValidation &&
            !context.nativeMotionValidation && !context.benchmark.IsRunning() &&
            !context.expectedBenchmarkFrame.has_value() && GetPropA(hWnd, kCaptureModeProperty) == nullptr)
        {
            const auto liveScene = profile == horde::vulkan::raytracing::RtSceneProfile::EntryMenu
                ? horde::graphics::ForegroundPauseScene::EntryMenu
                : horde::graphics::ForegroundPauseScene::GraphicsPreview;
            const int liveProfileCapHz = horde::graphics::ForegroundPauseRenderCadence::ResolveLiveProfileCapHz(
                liveScene, context.graphicsPreviewFrameCap);
            const auto interval = std::chrono::duration<double>(1.0 / liveProfileCapHz);
            const auto due = context.graphicsPreviewLastFrame +
                std::chrono::duration_cast<std::chrono::steady_clock::duration>(interval);
            if (foregroundNow < due)
            {
                const auto remaining = std::chrono::ceil<std::chrono::milliseconds>(due - foregroundNow);
                const DWORD waitMilliseconds = static_cast<DWORD>(std::clamp<std::int64_t>(remaining.count(), 1,
                    horde::graphics::ForegroundPauseRenderCadence::kMaximumPollInterval.count()));
                (void)MsgWaitForMultipleObjectsEx(0u, nullptr, waitMilliseconds, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
                continue;
            }
        }
        if (profile != horde::vulkan::raytracing::RtSceneProfile::Showcase)
        {
            context.graphicsPreviewDelta = context.graphicsPreviewLastFrame.time_since_epoch().count() == 0 ? 0.0 :
                std::chrono::duration<double>(foregroundNow - context.graphicsPreviewLastFrame).count();
            context.graphicsPreviewLastFrame = foregroundNow;
        }
        horde::graphics::ForegroundPauseRenderInput pauseCadenceInput{};
        pauseCadenceInput.foreground = !IsIconic(hWnd) && GetForegroundWindow() == hWnd;
        pauseCadenceInput.paused = context.simulationPaused;
        pauseCadenceInput.scene = profile == horde::vulkan::raytracing::RtSceneProfile::EntryMenu
            ? horde::graphics::ForegroundPauseScene::EntryMenu
            : profile == horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview
                ? horde::graphics::ForegroundPauseScene::GraphicsPreview
                : horde::graphics::ForegroundPauseScene::Showcase;
        pauseCadenceInput.currentRtOutputValid = context.useRtPath && context.rtScene.IsReady() &&
            capabilities.rtScene.presented;
        pauseCadenceInput.wakeRequested = context.campaignBarrier.HasPendingCommittedLoad() || graphicsTransition || context.sceneProfileDirty ||
            context.glassGeometryDirty || context.renderScaleDirty || context.graphicsCommand.has_value() ||
            context.entryPlayQueued || context.entryPlayHandoffPending || context.rtLabVisible ||
            context.graphicsPreviewCapture || context.outputResizeValidation || context.nativeMotionValidation ||
            context.benchmark.IsRunning() || context.expectedBenchmarkFrame.has_value() ||
            GetPropA(hWnd, kCaptureModeProperty) != nullptr;
        const bool wasPausedFallback = foregroundPauseCadence.UsingPausedFallback();
        const auto cadenceDecision = foregroundPauseCadence.Evaluate(foregroundNow, pauseCadenceInput);
        horde::graphics::ForegroundPauseRenderAggregate cadenceAggregate{};
        if (foregroundPauseCadence.TakeAggregate(foregroundNow, cadenceAggregate))
        {
            std::ostringstream line;
            line << "HORDE_PAUSE_RENDER_CADENCE wall_ns=" << cadenceAggregate.wallNanoseconds
                 << " render_attempts=" << cadenceAggregate.renderAttempts
                 << " skipped_iterations=" << cadenceAggregate.skippedIterations
                 << " entered=" << cadenceAggregate.enteredFallback
                 << " exited=" << cadenceAggregate.exitedFallback << "\n";
            OutputDebugStringA(line.str().c_str());
        }
        if (wasPausedFallback && !cadenceDecision.usingPausedFallback)
            context.lastControlSteadyNs = horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr);
        if (!cadenceDecision.renderNow)
        {
            context.lastControlSteadyNs = horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr);
            const DWORD waitMilliseconds = static_cast<DWORD>(std::clamp<std::int64_t>(
                cadenceDecision.wait.count(), 1,
                horde::graphics::ForegroundPauseRenderCadence::kMaximumPollInterval.count()));
            (void)MsgWaitForMultipleObjectsEx(0u, nullptr, waitMilliseconds, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            continue;
        }

        const bool benchmarkFrame = context.benchmark.IsRunning();
        const auto frameStart = std::chrono::steady_clock::now();
        bool rtFramePresented = false;
        context.expectedBenchmarkFrame.reset();
        SetCompatibilityAttemptStage(hWnd,horde::reporting::CompatibilityStage::RenderLoop);
        const bool frameRendered = RenderFrame(context, clearColor, rtFramePresented);
        if(rtFramePresented && (context.lastFramePresentation==horde::telemetry::RtPresentationOutcome::Presented ||
            context.lastFramePresentation==horde::telemetry::RtPresentationOutcome::PresentedNeedsRecreate))
            CompatibilityPresentation(hWnd,context.rtScene.ExecutionBackend());
        if (benchmarkFrame && context.benchmarkProfileActive &&
            !ObserveWindowsBenchmarkProfileMeasurement(context, false))
            CancelBenchmark(context, false, "profile-observed-state-changed");
        FinishGraphicsFrame(context, frameRendered && rtFramePresented);
        ObserveCampaignPresentation(context,frameRendered && rtFramePresented);
        QueuePresentedEntryPlay(context, rtFramePresented);
        FinishEntryLoadingAfterPresentation(context, rtFramePresented);
        if (context.expectedBenchmarkFrame &&
            context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Measuring)
        {
            horde::telemetry::RtExpectedFrameRecord row{};
            if (context.benchmarkEvidence.TryGetExpectedFrame(*context.expectedBenchmarkFrame, row) &&
                row.disposition == horde::telemetry::RtExpectedFrameDisposition::AwaitingSubmission)
                (void)context.benchmarkEvidence.RejectExpected(*context.expectedBenchmarkFrame,
                    horde::telemetry::RtBenchmarkFailureReason::SubmissionFailed);
        }
        if (!frameRendered)
        {
            if (benchmarkFrame)
            {
                LogBenchmarkCancellation(context, "render-frame-failed");
                context.benchmark.Cancel();
                CompleteBenchmark(context, capabilities, textReportPath.parent_path());
            }
            renderFailed = true;
            // The dialog directs the owner to reports. Persist the actual
            // record failure before waiting for dismissal; stderr alone is
            // unavailable in an ordinary GUI launch.
            const auto& failedSnapshot = context.simulation.Snapshot();
            std::ostringstream failureReport;
            failureReport << "RT render loop stopped: "
                << (context.lastRtFrameError.empty() ? "No detailed frame error available." : context.lastRtFrameError)
                << "\nSimulation tick: " << failedSnapshot.tickIndex
                << "\nPlayer support world Y: " << failedSnapshot.playerSupportWorldY
                << "\nRescue phase: " << static_cast<unsigned>(failedSnapshot.rescue.phase)
                << "\nRescue equipment stowed: " << failedSnapshot.rescue.equipmentStowed << '\n';
            (void)WriteReportFile(textReportPath.parent_path() / "windows_render_failure.txt",
                                  failureReport.str());
            if (!automatedBenchmark) GameMessageBoxA(hWnd,
                        "The native RT render loop stopped unexpectedly. Check the reports folder for diagnostics.",
                        "Horde Lantern RT - renderer stopped",
                        MB_OK | MB_ICONERROR);
            break;
        }
        capabilities.performance.gpuRt = context.gpuRtTiming;
        UpdateRtLabTelemetry(context);
        const auto frameEnd = std::chrono::steady_clock::now();
        const double frameTimeMs = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();
        if (cadenceDecision.usingPausedFallback)
        {
            context.lastControlSteadyNs = horde::vulkan::raytracing::ReadRtSceneSteadyClock(nullptr);
            foregroundPauseCadence.RecordRenderAttempt(frameEnd);
            horde::graphics::ForegroundPauseRenderAggregate aggregate{};
            if (foregroundPauseCadence.TakeAggregate(frameEnd, aggregate))
            {
                std::ostringstream line;
                line << "HORDE_PAUSE_RENDER_CADENCE wall_ns=" << aggregate.wallNanoseconds
                     << " render_attempts=" << aggregate.renderAttempts
                     << " skipped_iterations=" << aggregate.skippedIterations
                     << " entered=" << aggregate.enteredFallback
                     << " exited=" << aggregate.exitedFallback << "\n";
                OutputDebugStringA(line.str().c_str());
            }
        }
        if (context.rtScene.Profile() == horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview)
        {
            const double loopSeconds = std::chrono::duration<double>(frameEnd -
                (context.graphicsPreviewLastSample.time_since_epoch().count() == 0 ? ownerFrameStart : context.graphicsPreviewLastSample)).count();
            context.graphicsPreviewLastSample = frameEnd;
            std::optional<double> gpu;
            const bool previewTransition = graphicsTransition || context.lastFramePresentation ==
                horde::telemetry::RtPresentationOutcome::PresentedNeedsRecreate || context.lastFramePresentation ==
                horde::telemetry::RtPresentationOutcome::NotPresentedNeedsRecreate;
            if (!previewTransition && context.gpuRtTiming.valid && context.gpuRtTiming.sampleCount > context.graphicsPreviewLastGpuSample)
                gpu = context.gpuRtTiming.latestMs;
            context.graphicsPreviewLastGpuSample = context.gpuRtTiming.sampleCount;
            const auto resources = context.rtScene.ResourceInventory();
            context.graphicsPreviewPerformance.SetTrackedAllocations(resources.deviceLocalBytes, resources.hostVisibleBytes);
            context.graphicsPreviewPerformance.RecordFrame(loopSeconds, frameTimeMs, gpu, rtFramePresented, previewTransition);
            if (context.graphicsVisible && GetTickCount64() - context.graphicsPreviewLastUiTick >= 250u)
            {
                context.graphicsPreviewLastUiTick = GetTickCount64();
                const auto stats = context.graphicsPreviewPerformance.Snapshot();
                std::ostringstream text;
                text << "Preview scene performance | cap " << context.graphicsPreviewFrameCap << " Hz\r\n"
                     << std::fixed << std::setprecision(1) << "Successful RT presents/s " << stats.successfulPresentsPerSecond
                     << " | loop " << stats.meanLoopMilliseconds << " ms | CPU render " << stats.meanCpuRenderMilliseconds << " ms | GPU ";
                if (stats.meanGpuMilliseconds) text << *stats.meanGpuMilliseconds << " ms"; else text << "unavailable";
                text << "\r\nTracked scene allocations: device-local " << resources.deviceLocalBytes / 1048576u
                     << " MiB, host-visible " << resources.hostVisibleBytes / 1048576u << " MiB (may overlap). Budget/residency unavailable."
                     << "\r\nSwapchain success rate, not scanout FPS. Preview does not predict full-game sustained performance.";
                SetWindowTextA(GetDlgItem(hWnd, kGraphicsPreviewTelemetryId), text.str().c_str());
                InvalidateRect(GetDlgItem(hWnd, kGraphicsPreviewGraphId), nullptr, FALSE);
            }
        }
        if (benchmarkFrame)
        {
            timingSamples.clear();
            if (context.lastFramePresentation ==
                    horde::telemetry::RtPresentationOutcome::PresentedNeedsRecreate ||
                context.lastFramePresentation ==
                    horde::telemetry::RtPresentationOutcome::NotPresentedNeedsRecreate)
            {
                // A successful SUBOPTIMAL present is still honest presentation,
                // but recreation interrupted this measurement. Cancel before
                // RecordFrame so the legacy collector cannot admit its timing.
                LogBenchmarkCancellation(context, "presentation-needs-recreate");
                context.benchmark.Cancel();
            }
            context.benchmark.RecordFrame(frameTimeMs, rtFramePresented);
            if (context.benchmark.ConsumeLiveProgressUpdate()) UpdateBenchmarkHud(context);
            if (!context.benchmark.IsRunning())
            {
                CompleteBenchmark(context, capabilities, textReportPath.parent_path());
            }
        }
        else if (context.rtScene.Profile() == horde::vulkan::raytracing::RtSceneProfile::Showcase)
        {
            timingSamples.push_back(frameTimeMs);
        }
        else timingSamples.clear();
        const auto publication = context.rtFrameEvidence.PublishedStateByValue();
        const auto* evidencePublication = context.rtFrameEvidenceInitialised && context.rtFrameEvidence.ObserverAvailable()
            ? &publication : nullptr;
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
            WriteReportFile(textReportPath, horde::vulkan::BuildCapabilityTextReport(capabilities, evidencePublication));
            WriteReportFile(jsonReportPath, horde::vulkan::BuildCapabilityJsonReport(capabilities, evidencePublication));
            if (context.diagnosticsVisible)
            {
                if (HWND edit = GetDlgItem(hWnd, kEditControlId))
                {
                    const std::string updatedText = WindowSafeText(BuildDisplayText(capabilities, evidencePublication));
                    SetWindowTextA(edit, updatedText.c_str());
                }
            }
            timingSamples.clear();
        }
        if (rtFramePresented && context.lastFramePresentation == horde::telemetry::RtPresentationOutcome::Presented &&
            (!capabilities.rtScene.presented || capabilities.rtScene.dispatchWidth != context.rtScene.DispatchExtent().width ||
             capabilities.rtScene.dispatchHeight != context.rtScene.DispatchExtent().height))
        {
            capabilities.rtScene.presented = true;
            capabilities.rtScene.executionBackend = context.rtScene.ExecutionBackend();
            capabilities.rtScene.status = "Presented via swapchain";
            capabilities.rtScene.geometry = context.rtScene.Profile() == horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview ?
                "Compact authored graphics preview with actual Idle skeleton, lantern, torch, water, mirror and material surfaces" :
                "Complete Horde showcase route with sequential animated skeleton and staff-lit lich";
            capabilities.rtScene.dispatchWidth = context.rtScene.DispatchExtent().width;
            capabilities.rtScene.dispatchHeight = context.rtScene.DispatchExtent().height;
            capabilities.performance.internalRenderWidth = capabilities.rtScene.dispatchWidth;
            capabilities.performance.internalRenderHeight = capabilities.rtScene.dispatchHeight;
            auto& presentationDiagnostics = capabilities.diagnostics;
            presentationDiagnostics.erase(std::remove(presentationDiagnostics.begin(), presentationDiagnostics.end(),
                                                       "Internal render resolution: not measured yet."),
                                          presentationDiagnostics.end());
            WriteReportFile(textReportPath, horde::vulkan::BuildCapabilityTextReport(capabilities, evidencePublication));
            WriteReportFile(jsonReportPath, horde::vulkan::BuildCapabilityJsonReport(capabilities, evidencePublication));
            if (HWND hud = GetDlgItem(hWnd, kHudControlId))
            {
                SetWindowTextA(hud, kHudActiveText);
                RECT hudClient{};
                GetClientRect(hWnd, &hudClient);
                LayoutOverlayControls(hWnd, hudClient.right, hudClient.bottom);
            }
            if (HWND edit = context.benchmarkReportVisible ? nullptr : GetDlgItem(hWnd, kEditControlId))
            {
                const std::string updatedText = WindowSafeText(BuildDisplayText(capabilities, evidencePublication));
                SetWindowTextA(edit, updatedText.c_str());
            }
        }
#if defined(_DEBUG)
        RefreshDeveloperOverlay(context, capabilities);
#endif
    }

    const bool benchmarkSucceeded = context.benchmark.Passed() && context.benchmarkReportsSaved &&
        context.benchmarkEvidence.Status() == horde::telemetry::RtBenchmarkRunStatus::Complete &&
        context.benchmarkEvidence.ExpectedCount() == context.benchmark.Frames().size();
    bool restorationPassed=true;
    if (context.benchmarkProfileRequested)
    {
        restorationPassed=RestoreWindowsBenchmarkProfile(context, clearColor, !renderFailed && benchmarkSucceeded);
        if (!WriteWindowsBenchmarkProfileReceipt(context, textReportPath.parent_path(), {})) renderFailed = true;
        if (!restorationPassed || (!context.openingBenchmarkOwner && !context.benchmarkProfile->Succeeded())) renderFailed = true;
    }
    if(context.openingBenchmarkOwner) {
        openingBenchmark->closeRequested=openingBenchmark->closeRequested || context.benchmarkProfileCloseRequested;
        openingBenchmark->statusTitle=benchmarkSucceeded && !renderFailed ? "RUN COMPLETE" :
            context.benchmarkProfileCancelRequested ? "CANCELLED" : "NO RESULT";
        openingBenchmark->status=benchmarkSucceeded && !renderFailed ? "Benchmark complete. Local reports saved." :
            context.benchmarkProfileCancelRequested ? "Benchmark cancelled. Personal graphics restored." :
            "Benchmark did not complete. Personal graphics restored; no result admitted.";
        const bool seedsKept=CarryOpeningBenchmarkSeeds(*openingBenchmark,context);
        auto completedReport=benchmarkSucceeded && !renderFailed ? std::move(context.benchmarkReport) : std::string{};
        auto completedSummary=benchmarkSucceeded && !renderFailed ? std::move(context.benchmarkSummary) :
            std::optional<horde::telemetry::FrozenBenchmarkSummary>{};
        const bool retired=renderOwner.Retire();
        const bool admitted=openingBenchmark->owner.BenchmarkRetired(restorationPassed && seedsKept,retired);
        if(admitted) {
            openingBenchmark->completedReport=std::move(completedReport);
            openingBenchmark->completedSummary=std::move(completedSummary);
        }
        std::cerr<<"HORDE_OPENING_BENCHMARK benchmark-retired="<<retired<<" restored="<<restorationPassed
                 <<" successorAdmitted="<<admitted<<'\n';
        return admitted?0:1;
    }
    const bool finalRetired=renderOwner.Retire();
    if (!finalRetired) renderFailed = true;
    if(gOpeningBenchmarkMenuValidation && openingBenchmark && openingBenchmark->closeRequested)
        std::cerr<<"HORDE_OPENING_BENCHMARK final-entry-retired="<<finalRetired
                 <<" window-alive="<<IsWindow(hWnd)<<'\n';
    if (automatedBenchmark) return !renderFailed && benchmarkSucceeded ? 0 : 1;
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

bool IsGraphicsMenuButton(const int controlId)
{
    switch (controlId)
    {
    case kGraphicsPresetButtonId:
    case kGraphicsFireButtonId:
    case kGraphicsShadowButtonId:
    case kGraphicsGlassButtonId:
    case kGraphicsMistButtonId:
    case kGraphicsDustButtonId:
    case kGraphicsApplyButtonId:
    case kGraphicsConfirmButtonId:
    case kGraphicsRevertButtonId:
    case kGraphicsResetButtonId:
    case kGraphicsPreviewPauseId:
    case kGraphicsPreviewCameraId:
    case kGraphicsPreviewMotionId:
    case kGraphicsPreviewResetId:
    case kWaterQualityButtonId:
    case kSettingsBackButtonId:
        return true;
    default:
        return false;
    }
}

HFONT CreateGraphicsButtonFitFont(HWND window, HDC dc, HFONT currentFont,
                                  const char* text, const int availableWidth)
{
    if (currentFont == nullptr || text == nullptr || text[0] == '\0' || availableWidth <= 0)
        return nullptr;

    SIZE measured{};
    const int textLength = lstrlenA(text);
    if (textLength <= 0 || !GetTextExtentPoint32A(dc, text, textLength, &measured) ||
        measured.cx <= availableWidth)
        return nullptr;

    LOGFONTA fontDescription{};
    if (GetObjectA(currentFont, sizeof(fontDescription), &fontDescription) !=
        static_cast<int>(sizeof(fontDescription)))
        return nullptr;

    const int originalHeight = std::max(1,
        static_cast<int>(std::abs(fontDescription.lfHeight)));
    const int minimumHeight = std::max(1, ScaleForDpi(window, 9));
    int candidateHeight = std::max(minimumHeight,
        MulDiv(originalHeight, availableWidth, std::max(1, static_cast<int>(measured.cx))));
    while (candidateHeight >= minimumHeight)
    {
        LOGFONTA fittedDescription = fontDescription;
        fittedDescription.lfHeight = fontDescription.lfHeight < 0
            ? -candidateHeight : candidateHeight;
        HFONT fittedFont = CreateFontIndirectA(&fittedDescription);
        if (fittedFont == nullptr)
            return nullptr;

        const HGDIOBJ previousFont = SelectObject(dc, fittedFont);
        if (previousFont == nullptr || previousFont == HGDI_ERROR)
        {
            DeleteObject(fittedFont);
            return nullptr;
        }
        SIZE fittedExtent{};
        const bool measuredFit = GetTextExtentPoint32A(
            dc, text, textLength, &fittedExtent) && fittedExtent.cx <= availableWidth;
        SelectObject(dc, previousFont);
        if (measuredFit)
            return fittedFont;
        DeleteObject(fittedFont);
        if (candidateHeight == minimumHeight)
            break;
        --candidateHeight;
    }
    return nullptr;
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

    auto *fontContext = reinterpret_cast<VulkanSurfaceContext *>(
        GetWindowLongPtrA(window, GWLP_USERDATA));
    const bool entryStyle = fontContext != nullptr && fontContext->entryMenuVisible;
    HFONT uiFont = CreateFontA(
        ScaleForDpi(window, 18), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    if (!uiFont)
    {
        uiFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    }

    HFONT teachingPromptFont = CreateFontA(
        ScaleForDpi(window, 19), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_ROMAN, "Georgia");
    if (teachingPromptFont)
    {
        if (HWND prompt = GetDlgItem(window, kCombatTeachingPromptId))
            SendMessageA(prompt, WM_SETFONT, reinterpret_cast<WPARAM>(teachingPromptFont), TRUE);
        ReplaceFontProperty(window, kCombatTeachingPromptFontProperty, teachingPromptFont);
    }
    HFONT subtitleFont = CreateFontA(-ScaleForDpi(window, 22), 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_ROMAN, "Georgia");
    if (subtitleFont)
    {
        if (HWND subtitle = GetDlgItem(window, kChapterSubtitleId))
            SendMessageA(subtitle, WM_SETFONT, reinterpret_cast<WPARAM>(subtitleFont), TRUE);
        ReplaceFontProperty(window, kSubtitleFontProperty, subtitleFont);
        if (fontContext)
        {
            fontContext->appliedSubtitleFontPixels = 0;
            fontContext->subtitleBitmapCurrent = false;
        }
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
    for (const int id : {kHudControlId,
                         kVitalityHudControlId,
                         kChestPromptControlId,
                         kCombatTeachingPromptId,
                         kPauseTitleId,
                         kEndingBodyId,
                         kResumeButtonId,
                         kLookoutObjectiveButtonId,
                         LookoutPanel::TitleId,LookoutPanel::BodyId,LookoutPanel::ContinueId,LookoutPanel::MenuId,LookoutPanel::BackId,
                         kEntryMoreButtonId,
                         kEntryBackButtonId,
                         kRestartButtonId,
                         kControlsButtonId,
                         kSettingsButtonId,
                         kReportProblemButtonId,
                         kDiagnosticsButtonId,
                         kRunBenchmarkButtonId,
                         kMoreBySamfa12ButtonId,
                         kExitButtonId,
                         kBenchmarkTitleId,
                         kBenchmarkCopyButtonId,
                         kBenchmarkSaveButtonId,
                         kBenchmarkReviewStatsButtonId,
                         kBenchmarkBackButtonId,
                         kSettingsTitleId,
                         kSettingsMoreId,
                         kVoiceVolumeLabelId,
                         kVoiceVolumeSliderId,
                         kSubtitlesButtonId,
                         kSubtitleSizeLabelId,
                         kSubtitleSizeSliderId,
                         kSubtitlePositionButtonId,
                         kSfxVolumeLabelId,
                         kSfxVolumeSliderId,
                         kSensitivityButtonId,
                         kWaterQualityButtonId,
                         kRenderScaleLabelId,
                         kRenderScaleSliderId,
                         kMusicVolumeLabelId,
                         kMusicVolumeSliderId,
                         kFullscreenButtonId,
                         kSettingsBackButtonId,
                         kGraphicsOpenButtonId,
                         kGraphicsPresetButtonId,
                         kGraphicsFireButtonId,
                         kGraphicsShadowButtonId,
                         kGraphicsGlassButtonId,
                         kGraphicsMistButtonId,
                         kGraphicsDustButtonId,
                         kGraphicsApplyButtonId,
                         kGraphicsConfirmButtonId,
                         kGraphicsRevertButtonId,
                         kGraphicsResetButtonId,
                         kGraphicsInfoId,
                         kGraphicsPreviewPauseId,
                         kGraphicsPreviewCameraId,
                         kGraphicsPreviewMotionId,
                         kGraphicsPreviewResetId,
                         kGraphicsPreviewTelemetryId,
                         kRtLabTitleId,
                         kRtLabTelemetryId,
                         kRtLabWaterfallLabelId,
                         kRtLabWaterfallSliderId,
                         kRtLabRoofLabelId,
                         kRtLabRoofSliderId,
                         kRtLabDawnLabelId,
                         kRtLabDawnSliderId,
                         kRtLabFogLabelId,
                         kRtLabFogSliderId,
                         kRtLabLightGroupButtonId,
                         kRtLabFireStrengthLabelId,
                         kRtLabFireStrengthSliderId,
                         kRtLabFireTurbulenceLabelId,
                         kRtLabFireTurbulenceSliderId,
                         kRtLabFireSmokeLabelId,
                         kRtLabFireSmokeSliderId,
                         kRtLabGlassVisibilityLabelId,
                         kRtLabGlassVisibilitySliderId,
                         kRtLabGlassTransmissionLabelId,
                         kRtLabGlassTransmissionSliderId,
                         kRtLabGlassIorLabelId,
                         kRtLabGlassIorSliderId,
                         kRtLabGlassRoughnessLabelId,
                         kRtLabGlassRoughnessSliderId,
                         kRtLabHueLabelId,
                         kRtLabHueSliderId,
                         kRtLabIntensityLabelId,
                         kRtLabIntensitySliderId,
                         kRtLabWorkloadButtonId,
                         kRtLabRestoreButtonId,
                         kRtLabBackButtonId})
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
    HFONT graphicsInfoFont = CreateFontA(ScaleForDpi(window, 14), 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    if (graphicsInfoFont)
    {
        for (const int id : {kGraphicsInfoId, kGraphicsPreviewTelemetryId, kGraphicsPreviewGraphId})
            SendMessageA(GetDlgItem(window, id), WM_SETFONT, reinterpret_cast<WPARAM>(graphicsInfoFont), TRUE);
        ReplaceFontProperty(window, kGraphicsInfoFontProperty, graphicsInfoFont);
    }
    if (uiFont != GetStockObject(DEFAULT_GUI_FONT))
    {
        ReplaceFontProperty(window, kUiFontProperty, uiFont);
    }
    if (entryStyle)
    {
        HFONT plaque = CreateFontA(ScaleForDpi(window, 24), 0, 0, 0, FW_BOLD,
            FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_ROMAN, "Georgia");
        if (plaque)
        {
            for (const int id : {kResumeButtonId, kSettingsButtonId, kEntryMoreButtonId,
                 kControlsButtonId, kReportProblemButtonId, kDiagnosticsButtonId,
                 kMoreBySamfa12ButtonId, kExitButtonId, kEntryBackButtonId,
                 kSettingsBackButtonId, kSensitivityButtonId, kGraphicsOpenButtonId,
                 kFullscreenButtonId})
                SendMessageA(GetDlgItem(window, id), WM_SETFONT,
                             reinterpret_cast<WPARAM>(plaque), TRUE);
            ReplaceFontProperty(window, kEntryPlaqueFontProperty, plaque);
        }
        HFONT title = CreateFontA(ScaleForDpi(window, 36), 0, 0, 0, FW_BOLD,
            FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_ROMAN, "Georgia");
        if (title)
        {
            SendMessageA(GetDlgItem(window, kPauseTitleId), WM_SETFONT,
                         reinterpret_cast<WPARAM>(title), TRUE);
            ReplaceFontProperty(window, kEntryTitleFontProperty, title);
        }
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
    for (const char* propertyName : {kUiFontProperty, kMonoFontProperty, kDeveloperFontProperty,
                                   kGraphicsInfoFontProperty, kEntryTitleFontProperty,
                                   kCombatTeachingPromptFontProperty,
                                   kSubtitleFontProperty,
                                   kEntryPlaqueFontProperty})
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
    if (HWND spinner = GetDlgItem(window, kEntryLoadingIndicatorId))
    {
        const int size = ScaleForDpi(window, 32);
        MoveWindow(spinner, (width - size) / 2, (height - size) / 2, size, size, TRUE);
    }
    if (HWND edit = GetDlgItem(window, kEditControlId))
    {
        MoveWindow(edit, inset, inset,
                   std::max(ScaleForDpi(window, 100), width - inset * 2),
                   std::max(ScaleForDpi(window, 100), height - inset * 2), TRUE);
    }
    auto* sceneContext = reinterpret_cast<VulkanSurfaceContext*>(GetWindowLongPtrA(window, GWLP_USERDATA));
    if(sceneContext && sceneContext->campaignMenu && sceneContext->campaignMenu->Visible()) {
        RECT bounds{inset,inset,width-inset,height-inset};
        sceneContext->campaignMenu->Layout(bounds);return;
    }
    if(sceneContext && sceneContext->lookoutObjective && sceneContext->lookoutObjective->Visible()) {
        sceneContext->lookoutObjective->Layout({inset,inset,width-inset,height-inset},ScaleForDpi(window,96));return;
    }
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
        const int availableWidth = width - inset * 2 - reportGap * 3;
        const int reportButtonWidth = availableWidth / 4;
        int reportX = inset;
        for (const int id : {kBenchmarkCopyButtonId, kBenchmarkSaveButtonId, kBenchmarkReviewStatsButtonId, kBenchmarkBackButtonId})
        {
            if (HWND control = GetDlgItem(window, id))
            {
                MoveWindow(control, reportX, reportButtonY, reportButtonWidth, reportButtonHeight, TRUE);
            }
            reportX += reportButtonWidth + reportGap;
        }
    }
    if(sceneContext && sceneContext->benchmarkChooserVisible) LayoutOpeningBenchmarkChooser(window,width,height);
    if (HWND hud = GetDlgItem(window, kHudControlId))
    {
        const int hudInset = ScaleForDpi(window, 14);
        const bool expanded = sceneContext != nullptr && sceneContext->benchmark.ShowsLiveFps();
        char hudText[96]{};
        GetWindowTextA(hud, hudText, static_cast<int>(sizeof(hudText)));
        const bool compact = !expanded && std::string_view(hudText) == kHudActiveText;
        const LONG_PTR style = GetWindowLongPtrA(hud, GWL_STYLE);
        SetWindowLongPtrA(hud, GWL_STYLE, expanded ? style & ~SS_CENTERIMAGE : style | SS_CENTERIMAGE);
        const int hudWidth = compact ? ScaleForDpi(window, 240) :
            std::min(ScaleForDpi(window, 650), std::max(ScaleForDpi(window, 260), width - hudInset * 2));
        MoveWindow(hud, compact ? width - hudWidth - hudInset : hudInset, hudInset, hudWidth,
                   ScaleForDpi(window, expanded ? 80 : 30), TRUE);
    }
    LayoutVitalityHud(window, width, sceneContext ?
        sceneContext->simulation.Snapshot().playerVitals.maxVitality : horde::gameplay::PlayerVitals::kMaxVitality);
    if (HWND prompt = GetDlgItem(window, kChestPromptControlId))
    {
        const int promptWidth = std::min(ScaleForDpi(window, 650),
                                         std::max(ScaleForDpi(window, 250),
                                                  width - ScaleForDpi(window, 48)));
        MoveWindow(prompt,
                   (width - promptWidth) / 2,
                   height - ScaleForDpi(window, 94),
                   promptWidth,
                   ScaleForDpi(window, 34),
                   TRUE);
    }
    if (HWND prompt = GetDlgItem(window, kCombatTeachingPromptId))
    {
        const int promptWidth = std::min(ScaleForDpi(window, 700),
                                         std::max(ScaleForDpi(window, 320),
                                                  width - ScaleForDpi(window, 48)));
        MoveWindow(prompt, (width - promptWidth) / 2, ScaleForDpi(window, 72),
                   promptWidth, ScaleForDpi(window, 76), TRUE);
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
    const bool lookoutEntryShown=sceneContext && IsWindowVisible(GetDlgItem(window,kLookoutObjectiveButtonId));
    const int pauseButtonCount=12+(lookoutEntryShown?1:0);
    const int buttonHeight = std::min(ScaleForDpi(window,38),std::max(ScaleForDpi(window,24),
        (height-ScaleForDpi(window,24+54+(pauseButtonCount-1)*8))/pauseButtonCount));
    const int gap = ScaleForDpi(window, 8);
    const int titleHeight = ScaleForDpi(window, 48);
    const int titleAdvance = ScaleForDpi(window, 54);
    const auto* layoutContext = reinterpret_cast<const VulkanSurfaceContext*>(GetWindowLongPtrA(window, GWLP_USERDATA));
    const bool endingLayout = layoutContext != nullptr && layoutContext->endingOverlayVisible;
    const bool compactOverlayLayout = layoutContext != nullptr &&
                                      (layoutContext->deathOverlayVisible || endingLayout);
    const int endingBodyHeight = endingLayout ? ScaleForDpi(window, 132) : 0;
    const int compactButtonCount = 4;
    const int pauseTotal = compactOverlayLayout
        ? titleHeight + endingBodyHeight + (endingLayout ? gap : 0) + compactButtonCount * buttonHeight + compactButtonCount * gap
        : titleHeight + pauseButtonCount * buttonHeight + (pauseButtonCount-1) * gap;
    const int pauseX = (width - buttonWidth) / 2;
    int y = std::max(ScaleForDpi(window, 12), (height - pauseTotal) / 2);
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
    if (layoutContext && layoutContext->entryMenuVisible && !layoutContext->entryMoreVisible)
    {
        const int plaqueWidth =
            std::min(ScaleForDpi(window, 240), std::max(ScaleForDpi(window, 120), width / 3));
        const int plaqueHeight = std::max(ScaleForDpi(window, 54), buttonHeight);
        MoveWindow(GetDlgItem(window, kPauseTitleId), ScaleForDpi(window, 24),
                   ScaleForDpi(window, 30), std::min(width - 48, ScaleForDpi(window, 340)),
                   titleHeight, TRUE);
        MoveWindow(GetDlgItem(window, kResumeButtonId),
                   (width - plaqueWidth) / 2, height * 3 / 5, plaqueWidth,
                   plaqueHeight, TRUE);
        const int bottom = height - plaqueHeight - ScaleForDpi(window, 26);
        MoveWindow(GetDlgItem(window, kSettingsButtonId),
                   width - plaqueWidth - ScaleForDpi(window, 26), bottom,
                   plaqueWidth, plaqueHeight, TRUE);
        MoveWindow(GetDlgItem(window, kEntryMoreButtonId), ScaleForDpi(window, 26),
                   bottom, plaqueWidth, plaqueHeight, TRUE);
    }
    else if (layoutContext && layoutContext->entryMenuVisible && layoutContext->entryMoreVisible)
    {
        const int moreHeight = ScaleForDpi(window, 48);
        for (const int id : {kCampaignLoadButtonId,kRunBenchmarkButtonId,kControlsButtonId, kReportProblemButtonId, kDiagnosticsButtonId,
                             kMoreBySamfa12ButtonId, kExitButtonId, kEntryBackButtonId})
        {
            MoveWindow(GetDlgItem(window, id), pauseX, y, buttonWidth, moreHeight, TRUE);
            y += moreHeight + gap;
        }
    }
    else if (compactOverlayLayout)
    {
        for (const int id : {kRtLabButtonId, kResumeButtonId, kCampaignLoadButtonId,kRestartButtonId, kExitButtonId})
        {
            if ((!endingLayout && id == kRtLabButtonId) || (endingLayout && id==kCampaignLoadButtonId)) continue;
            if (HWND control = GetDlgItem(window, id)) MoveWindow(control, pauseX, y, buttonWidth, buttonHeight, TRUE);
            y += buttonHeight + gap;
        }
    }
    else
    {
        for (const int id : {kResumeButtonId,kLookoutObjectiveButtonId,kCampaignSaveButtonId,kCampaignLoadButtonId,kRestartButtonId, kControlsButtonId, kSettingsButtonId,
                             kReportProblemButtonId, kRtLabButtonId, kDiagnosticsButtonId,
                             kRunBenchmarkButtonId, kMoreBySamfa12ButtonId, kExitButtonId})
        {
            if(id==kLookoutObjectiveButtonId && !lookoutEntryShown)continue;
            if (HWND control = GetDlgItem(window, id)) MoveWindow(control, pauseX, y, buttonWidth, buttonHeight, TRUE);
            y += buttonHeight + gap;
        }
    }

    const int labelHeight = ScaleForDpi(window, 26);
    const int sliderHeight = ScaleForDpi(window, 38);
    const bool entrySettings = layoutContext && layoutContext->entryMenuVisible &&
                               !layoutContext->graphicsVisible;
    const int settingsButtonHeight = entrySettings ? ScaleForDpi(window, 48) : buttonHeight;
    if (layoutContext && layoutContext->settingsVisible && layoutContext->settingsDialoguePage &&
        !layoutContext->graphicsVisible)
    {
        const int pageTotal = titleHeight + 2 * (labelHeight + sliderHeight + gap) +
            4 * (settingsButtonHeight + gap) + ScaleForDpi(window, 18);
        y = std::max(ScaleForDpi(window, 12), (height - pageTotal) / 2);
        MoveWindow(GetDlgItem(window, kSettingsTitleId), pauseX, y, buttonWidth, titleHeight, TRUE);
        y += titleAdvance;
        MoveWindow(GetDlgItem(window, kVoiceVolumeLabelId), pauseX, y, buttonWidth, labelHeight, TRUE);
        y += labelHeight;
        MoveWindow(GetDlgItem(window, kVoiceVolumeSliderId), pauseX, y, buttonWidth, sliderHeight, TRUE);
        y += sliderHeight + gap;
        MoveWindow(GetDlgItem(window, kSubtitlesButtonId), pauseX, y, buttonWidth, settingsButtonHeight, TRUE);
        y += settingsButtonHeight + gap;
        MoveWindow(GetDlgItem(window, kSubtitleSizeLabelId), pauseX, y, buttonWidth, labelHeight, TRUE);
        y += labelHeight;
        MoveWindow(GetDlgItem(window, kSubtitleSizeSliderId), pauseX, y, buttonWidth, sliderHeight, TRUE);
        y += sliderHeight + gap;
        MoveWindow(GetDlgItem(window, kSubtitlePositionButtonId), pauseX, y, buttonWidth, settingsButtonHeight, TRUE);
        y += settingsButtonHeight + gap;
        MoveWindow(GetDlgItem(window, kSettingsMoreId), pauseX, y, buttonWidth, settingsButtonHeight, TRUE);
        y += settingsButtonHeight + gap;
        MoveWindow(GetDlgItem(window, kSettingsBackButtonId), pauseX, y, buttonWidth, settingsButtonHeight, TRUE);
    }
    else
    {
    const int sliderRows = entrySettings ? 2 : 3;
    const int settingsTotal = titleHeight + 5 * settingsButtonHeight +
        sliderRows * labelHeight + sliderRows * sliderHeight + 8 * gap;
    y = std::max(ScaleForDpi(window, entrySettings ? 12 : 54), (height - settingsTotal) / 2);
    if (HWND title = GetDlgItem(window, kSettingsTitleId)) MoveWindow(title, pauseX, y, buttonWidth, titleHeight, TRUE);
    y += titleAdvance;
    for (const int id : {kSensitivityButtonId, kWaterQualityButtonId})
    {
        if (HWND control = GetDlgItem(window, id)) MoveWindow(control, pauseX, y, buttonWidth, settingsButtonHeight, TRUE);
        y += settingsButtonHeight + gap;
    }
    if (HWND graphics = GetDlgItem(window, kGraphicsOpenButtonId))
        MoveWindow(graphics, pauseX, y - settingsButtonHeight - gap, buttonWidth, settingsButtonHeight, TRUE);
    if (!entrySettings)
    {
        if (HWND label = GetDlgItem(window, kRenderScaleLabelId)) MoveWindow(label, pauseX, y, buttonWidth, labelHeight, TRUE);
        y += labelHeight;
        if (HWND slider = GetDlgItem(window, kRenderScaleSliderId)) MoveWindow(slider, pauseX, y, buttonWidth, sliderHeight, TRUE);
        y += sliderHeight + gap;
    }
    if (HWND label = GetDlgItem(window, kSfxVolumeLabelId)) MoveWindow(label, pauseX, y, buttonWidth, labelHeight, TRUE);
    y += labelHeight;
    if (HWND slider = GetDlgItem(window, kSfxVolumeSliderId)) MoveWindow(slider, pauseX, y, buttonWidth, sliderHeight, TRUE);
    y += sliderHeight + gap;
    if (HWND label = GetDlgItem(window, kMusicVolumeLabelId)) MoveWindow(label, pauseX, y, buttonWidth, labelHeight, TRUE);
    y += labelHeight;
    if (HWND slider = GetDlgItem(window, kMusicVolumeSliderId)) MoveWindow(slider, pauseX, y, buttonWidth, sliderHeight, TRUE);
    y += sliderHeight + gap;
    for (const int id : {kFullscreenButtonId, kSettingsBackButtonId})
    {
        if (HWND control = GetDlgItem(window, id)) MoveWindow(control, pauseX, y, buttonWidth, settingsButtonHeight, TRUE);
        y += settingsButtonHeight + gap;
    }
    if (HWND page = GetDlgItem(window, kSettingsMoreId))
    {
        MoveWindow(page, pauseX, y, buttonWidth, settingsButtonHeight, TRUE);
        y += settingsButtonHeight + gap;
    }
    }

    if (layoutContext != nullptr && layoutContext->graphicsVisible)
    {
        const int graphicsWidth = std::min(ScaleForDpi(window, 600), std::max(ScaleForDpi(window, 420), width / 2 - inset * 2));
        const int graphicsX = inset;
        const int compactHeight = ScaleForDpi(window, 36);
        const int infoHeight = ScaleForDpi(window, 148);
        // The preset receives a full row so its full caption remains readable
        // even when the graphics panel is at its 420-DIP minimum width.
        y = std::max(ScaleForDpi(window, 12), (height - ScaleForDpi(window, 506)) / 2);
        MoveWindow(GetDlgItem(window, kSettingsTitleId), graphicsX, y, graphicsWidth, titleHeight, TRUE);
        y += titleHeight + gap;
        MoveWindow(GetDlgItem(window, kGraphicsInfoId), graphicsX, y, graphicsWidth, infoHeight, TRUE);
        y += infoHeight + gap;
        MoveWindow(GetDlgItem(window, kGraphicsPresetButtonId), graphicsX, y, graphicsWidth, compactHeight, TRUE);
        y += compactHeight + gap;
        const int glassWidth = ScaleForDpi(window, 104);
        const int dustWidth = ScaleForDpi(window, 144);
        const int toggleRowWidth = glassWidth * 2 + dustWidth + gap * 2;
        const int toggleX = graphicsX + graphicsWidth - toggleRowWidth;
        MoveWindow(GetDlgItem(window, kGraphicsGlassButtonId), toggleX, y, glassWidth, compactHeight, TRUE);
        MoveWindow(GetDlgItem(window, kGraphicsMistButtonId), toggleX + glassWidth + gap, y, glassWidth, compactHeight, TRUE);
        MoveWindow(GetDlgItem(window, kGraphicsDustButtonId), toggleX + (glassWidth + gap) * 2, y, dustWidth, compactHeight, TRUE);
        y += compactHeight + gap;
        MoveWindow(GetDlgItem(window, kRenderScaleLabelId), graphicsX, y, graphicsWidth, labelHeight, TRUE);
        y += labelHeight;
        MoveWindow(GetDlgItem(window, kRenderScaleSliderId), graphicsX, y, graphicsWidth, sliderHeight, TRUE);
        y += sliderHeight + gap;
        const int half = (graphicsWidth - gap) / 2;
        const int third = (graphicsWidth - gap * 2) / 3;
        MoveWindow(GetDlgItem(window, kWaterQualityButtonId), graphicsX, y, third, compactHeight, TRUE);
        MoveWindow(GetDlgItem(window, kGraphicsFireButtonId), graphicsX + third + gap, y, third, compactHeight, TRUE);
        MoveWindow(GetDlgItem(window, kGraphicsShadowButtonId), graphicsX + (third + gap) * 2, y, third, compactHeight, TRUE);
        y += compactHeight + gap;
        int column = 0;
        for (const int id : {kGraphicsApplyButtonId, kGraphicsConfirmButtonId, kGraphicsRevertButtonId})
            MoveWindow(GetDlgItem(window, id), graphicsX + column++ * (third + gap), y, third, compactHeight, TRUE);
        y += compactHeight + gap;
        MoveWindow(GetDlgItem(window, kGraphicsResetButtonId), graphicsX, y, half, compactHeight, TRUE);
        MoveWindow(GetDlgItem(window, kSettingsBackButtonId), graphicsX + half + gap, y, half, compactHeight, TRUE);
        const int previewX = graphicsX + graphicsWidth + gap * 2;
        const int previewWidth = std::max(ScaleForDpi(window, 260), width - previewX - inset);
        MoveWindow(GetDlgItem(window, kGraphicsPreviewTelemetryId), previewX, ScaleForDpi(window, 66), previewWidth, ScaleForDpi(window, 120), TRUE);
        MoveWindow(GetDlgItem(window, kGraphicsPreviewGraphId), previewX, ScaleForDpi(window, 192), previewWidth, ScaleForDpi(window, 76), TRUE);
        const int previewHalf = (previewWidth - gap) / 2;
        const int bottom = height - inset - compactHeight * 3 - gap * 2;
        MoveWindow(GetDlgItem(window, kGraphicsPreviewPauseId), previewX, bottom, previewHalf, compactHeight, TRUE);
        MoveWindow(GetDlgItem(window, kGraphicsPreviewCameraId), previewX + previewHalf + gap, bottom, previewHalf, compactHeight, TRUE);
        MoveWindow(GetDlgItem(window, kGraphicsPreviewMotionId), previewX, bottom + compactHeight + gap, previewWidth, compactHeight, TRUE);
        MoveWindow(GetDlgItem(window, kGraphicsPreviewResetId), previewX, bottom + (compactHeight + gap) * 2, previewWidth, compactHeight, TRUE);
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
    GameMessageBoxA(window,
                "WASD  Move and strafe\n"
                "Shift  Hold to run    Caps Lock  Toggle run\n"
                "Crouch  Automatic under low openings; stand when clear\n"
                "Click once to capture mouse; then free look\n"
                "Left mouse  Swing / indicated chest or rope action\n"
                "Rope  Follow CLIMB / DESCEND; equipment stows automatically\n"
                "Q  Parry skeleton strike\n"
                "Space + movement direction  Dodge (neutral: forward)\n"
                "E  Raise / lower claimed lantern    Right mouse  Reserved\n"
                "Controller left stick  Deflect gently to walk, farther to run\n"
                "Controller right stick  Camera look\n"
                "RT  Attack    LT  Parry    B / Circle  Dodge\n"
                "A  Interact    Y  Raise / lower claimed lantern\n"
                "Tab  Skip current chapter line    X  Skip line\n"
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
    GameMessageBoxA(window,
                "Environment materials: Poly Haven (CC0).\n"
                "Sound effects: FilmCow Royalty Free Sound Effects Library.\n"
                "Menu lantern creak: Irhouen via Pixabay (Pixabay Content License).\n"
                "Water Dripping by DRAGON-STUDIO via Pixabay (Pixabay Content License).\n"
                "Skeleton derivative: original by Hotstrike Studio; texture, rig, and animation processing created with Meshy (CC BY 4.0).\n"
                "Placeholder lich character created and animated with Meshy (CC0).\n"
                "Production Gothic arming sword created with Meshy; runtime processing by Samfa12/Codex (CC BY 4.0).\n"
                "Production medieval hand torch created with Meshy; runtime processing by Samfa12/Codex (CC BY 4.0).\n"
                "Historical-Gothic traveller/fighter and viewmodel gauntlets created with Meshy; runtime processing and animation integration by Samfa12/Codex (CC BY 4.0).\n"
                "Production Gothic reward chest created with Meshy; runtime processing by Samfa12/Codex (CC BY 4.0).\n"
                "Production Gothic reward lantern created with Meshy; runtime processing by Samfa12/Codex (CC BY 4.0).\n"
                "Application icon created for this project with OpenAI image generation.\n\n"
                "glTF loading: cgltf by Johannes Kuhlmann (MIT).\n"
                "Full notice: THIRD_PARTY_NOTICES/cgltf-LICENSE.txt beside the demo.\n\n"
                "See ASSET_LICENSES.md beside the demo for source links and full licence details.",
                "Horde Lantern RT - credits and licences",
                MB_OK | MB_ICONINFORMATION);
}

void OpenSamfa12Website(HWND window)
{
    const HINSTANCE result = ShellExecuteA(window, "open", "https://samfa12.com/", nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32)
    {
        GameMessageBoxA(window,
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
        GameMessageBoxA(window, "The benchmark report could not be saved to that location.",
                    "Horde Lantern RT", MB_OK | MB_ICONERROR);
    }
}

void ToggleDiagnostics(VulkanSurfaceContext& context)
{
    context.diagnosticsVisible = !context.diagnosticsVisible;
    context.settingsVisible = false;
    context.benchmarkReportVisible = false;
    context.pauseMenuVisible = context.diagnosticsVisible;
    if (context.diagnosticsVisible && context.capabilitySnapshot)
    {
        const auto publication = context.rtFrameEvidence.PublishedStateByValue();
        const auto* evidence = context.rtFrameEvidenceInitialised && context.rtFrameEvidence.ObserverAvailable()
            ? &publication : nullptr;
        SetWindowTextA(GetDlgItem(context.windowHandle, kEditControlId),
            WindowSafeText(BuildDisplayText(*context.capabilitySnapshot, evidence)).c_str());
    }
    ApplyOverlayState(context);
    PlaySoundEffect(context, context.diagnosticsVisible ? "ui_select.wav" : "ui_back.wav");
    FocusGameControl(context.diagnosticsVisible ? GetDlgItem(context.windowHandle, kEditControlId) : context.windowHandle);
}

void OpenSettings(VulkanSurfaceContext& context)
{
    if (context.graphicsVisible) return;
    if (!context.simulationPaused) ShowPauseMenu(context, true);
    context.pauseMenuVisible = true;
    context.settingsVisible = true;
    context.settingsDialoguePage = false;
    context.graphicsVisible = false;
    context.diagnosticsVisible = false;
    context.benchmarkReportVisible = false;
    ApplyOverlayState(context);
    UpdateSettingsLabels(context);
    PlaySoundEffect(context, "ui_select.wav");
    FocusGameControl(GetDlgItem(context.windowHandle, kSfxVolumeSliderId));
}

bool NativeUiUsesHighContrast()
{
    HIGHCONTRASTA highContrast{sizeof(HIGHCONTRASTA)};
    return SystemParametersInfoA(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, 0) &&
           (highContrast.dwFlags & HCF_HIGHCONTRASTON) != 0u;
}

LRESULT CALLBACK DiagnosticWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* sceneContext = reinterpret_cast<VulkanSurfaceContext*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
    if(message==WM_COMMAND && LOWORD(wParam)==kMenuCompatibilityReportId) {
        auto* owner=CompatibilityFor(hWnd);
        if(!owner || owner->reviewOpen || (sceneContext && sceneContext->benchmark.IsRunning()))return 0;
        if(sceneContext) {ShowPauseMenu(*sceneContext,true);ClearDesktopInput(*sceneContext);DiscardDesktopPendingCommands(*sceneContext);}
        owner->reviewOpen=true;
        horde::platform::windows::ShowWindowsCompatibilityReport(hWnd,owner->snapshot);
        owner->reviewOpen=false;
        if(IsWindow(hWnd) && reinterpret_cast<VulkanSurfaceContext*>(GetWindowLongPtrA(hWnd,GWLP_USERDATA))==sceneContext && sceneContext)
            ClearDesktopInput(*sceneContext);
        return 0;
    }
#if defined(_DEBUG)
    if(gOpeningBenchmarkMenuValidation && ((message==WM_KEYDOWN && wParam==VK_ESCAPE) || message==WM_CLOSE)) {
        const auto* transfer=sceneContext ? sceneContext->openingBenchmark :
            reinterpret_cast<OpeningBenchmarkTransfer*>(GetPropA(hWnd,kOpeningBenchmarkTransferProperty));
        std::cerr<<"HORDE_OPENING_MENU_STAGE "<<(message==WM_CLOSE?"close-delivery":"escape-delivery")
                 <<" context-attached="<<(sceneContext!=nullptr)
                 <<" phase="<<(transfer?static_cast<int>(transfer->owner.Phase()):-1)<<'\n';
    }
#endif
    if(!sceneContext) {
        auto* transfer=reinterpret_cast<OpeningBenchmarkTransfer*>(GetPropA(hWnd,kOpeningBenchmarkTransferProperty));
        if(transfer && (transfer->owner.Phase()==OpeningBenchmarkOwnerPhase::Queued ||
           transfer->owner.Phase()==OpeningBenchmarkOwnerPhase::Benchmark ||
           transfer->owner.Phase()==OpeningBenchmarkOwnerPhase::ReturningEntry)) {
            // During detach/creation there is deliberately no render callback.
            // Keep lifecycle intent with the HWND shell until its next owner.
            if(message==WM_CLOSE || (message==WM_SYSKEYDOWN && wParam==VK_F4) ||
               (message==WM_COMMAND && (LOWORD(wParam)==kExitButtonId || LOWORD(wParam)==kMenuExitId))) {
                transfer->closeRequested=transfer->cancelPending=true;TraceOpeningMenuStage("detached-close");return 0;
            }
            if(message==WM_KEYDOWN && wParam==VK_ESCAPE) {
                transfer->cancelPending=true;TraceOpeningMenuStage("detached-cancel");return 0;
            }
            if(message==WM_COMMAND || message==WM_KEYDOWN || message==WM_SYSKEYDOWN ||
               message==WM_KEYUP || message==WM_SYSKEYUP || message==WM_LBUTTONDOWN ||
               message==WM_RBUTTONDOWN || message==WM_HSCROLL)return 0;
        }
    }
#if defined(_DEBUG)
    if (sceneContext && sceneContext->nativeMotionValidation)
    {
        switch (message)
        {
        case WM_COMMAND: case WM_HSCROLL: case WM_VSCROLL: case WM_MOUSEWHEEL:
        case WM_KEYDOWN: case WM_SYSKEYDOWN: case WM_KEYUP: case WM_SYSKEYUP:
        case WM_LBUTTONDOWN: case WM_RBUTTONDOWN:
            if (sceneContext->motionArmed)
                sceneContext->motionScenario.Fail("Native motion interrupted by external input/menu control."); return 0;
        case WM_ACTIVATEAPP:
            if (sceneContext->motionArmed && !wParam) sceneContext->motionScenario.Fail("Native motion lost application focus."); return 0;
        case WM_ACTIVATE:
            if (sceneContext->motionArmed && LOWORD(wParam) == WA_INACTIVE) sceneContext->motionScenario.Fail("Native motion lost window focus."); return 0;
        case WM_MOUSEMOVE: case WM_CAPTURECHANGED: case WM_KILLFOCUS:
            return 0; // Producer axes are exclusively the declared schedule; outer focus guard remains authoritative.
        default: break;
        }
    }
#endif
    if (sceneContext && (sceneContext->graphicsPreviewCapture || sceneContext->outputResizeValidation))
    {
        // Keep externally delivered input/menu/focus messages from mutating the
        // frozen gameplay snapshot or entering a preferences-writing UI path.
        switch (message)
        {
        case WM_COMMAND: case WM_HSCROLL: case WM_VSCROLL: case WM_MOUSEWHEEL:
        case WM_KEYDOWN: case WM_SYSKEYDOWN: case WM_KEYUP: case WM_SYSKEYUP:
        case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MOUSEMOVE:
        case WM_CAPTURECHANGED: case WM_KILLFOCUS: case WM_ACTIVATE: case WM_ACTIVATEAPP:
            return 0;
        default: break;
        }
    }
    if (horde::platform::windows::HandleGitHubReleaseUpdateMessage(hWnd, message, wParam))
    {
        return 0;
    }
    if(sceneContext && sceneContext->openingBenchmark &&
       sceneContext->openingBenchmark->owner.Phase()==OpeningBenchmarkOwnerPhase::Queued &&
       message==WM_KEYDOWN && wParam==VK_ESCAPE) {
        (void)sceneContext->openingBenchmark->owner.CancelQueued();
        sceneContext->benchmarkChooserVisible=true;
        ApplyOverlayState(*sceneContext);
        FocusGameControl(GetDlgItem(hWnd,kBenchmarkChoiceBackId));return 0;
    }
    if(sceneContext && sceneContext->openingBenchmarkReturnPending) {
        if(message==WM_CLOSE && sceneContext->openingBenchmark) {
            sceneContext->openingBenchmark->closeRequested=true;return 0;
        }
        if(message==WM_COMMAND || message==WM_KEYDOWN || message==WM_SYSKEYDOWN ||
           message==WM_LBUTTONDOWN || message==WM_RBUTTONDOWN || message==WM_HSCROLL)return 0;
    }
    if (sceneContext && (sceneContext->benchmarkProfileActive || sceneContext->openingBenchmarkOwner))
    {
        // The explicit launch owns this HWND until original RT restoration is
        // acknowledged. Closing/restarting requests cancellation, never destroys
        // the surface or starts another run across an apply/restore boundary.
        if (message == WM_CLOSE || (message == WM_COMMAND &&
            (LOWORD(wParam) == kExitButtonId || LOWORD(wParam) == kMenuExitId)))
        {
            sceneContext->benchmarkProfileCloseRequested = true;
            CancelBenchmark(*sceneContext, false, "profile-close"); return 0;
        }
        if (message == WM_COMMAND)
        {
            if (LOWORD(wParam) == kMenuPauseId || LOWORD(wParam) == kMenuRestartId || LOWORD(wParam) == kRestartButtonId)
                CancelBenchmark(*sceneContext, false, "profile-pause-or-restart");
            return 0;
        }
        if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP)
        {
            if (message == WM_SYSKEYDOWN && wParam == VK_F4) {
                sceneContext->benchmarkProfileCloseRequested = true;
                CancelBenchmark(*sceneContext, false, "profile-alt-f4"); return 0;
            }
            if (message == WM_KEYDOWN && wParam == VK_ESCAPE) CancelBenchmark(*sceneContext, false, "profile-escape");
            return 0;
        }
        if (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MOUSEMOVE ||
            message == WM_HSCROLL || message == WM_VSCROLL || message == WM_MOUSEWHEEL) return 0;
        if (((message == WM_SIZE || message == WM_DPICHANGED) && !sceneContext->benchmarkProfileInternalResize) ||
            (message == WM_ACTIVATEAPP && wParam == FALSE))
            CancelBenchmark(*sceneContext, false, "profile-window-interrupted");
    }
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
        if (sceneContext && reinterpret_cast<HWND>(lParam) == GetDlgItem(hWnd, kSfxVolumeSliderId))
        {
            sceneContext->sfxVolumePercent = horde::audio::ClampSfxVolumePercent(
                static_cast<int>(SendMessageA(reinterpret_cast<HWND>(lParam), TBM_GETPOS, 0, 0)));
            SpatialAudioEngine().SetSfxVolume(sceneContext->sfxVolumePercent);
            UpdateSettingsLabels(*sceneContext);
            if (LOWORD(wParam) != TB_THUMBTRACK)
            {
                SaveSettings(*sceneContext);
            }
            return 0;
        }
        if (sceneContext && reinterpret_cast<HWND>(lParam) == GetDlgItem(hWnd, kVoiceVolumeSliderId))
        {
            sceneContext->voiceVolumePercent = std::clamp(
                static_cast<int>(SendMessageA(reinterpret_cast<HWND>(lParam), TBM_GETPOS, 0, 0)), 0, 100);
            SpatialAudioEngine().SetDialogueVolume(sceneContext->voiceVolumePercent);
            UpdateSettingsLabels(*sceneContext);
            if (LOWORD(wParam) != TB_THUMBTRACK) SaveSettings(*sceneContext);
            return 0;
        }
        if (sceneContext && reinterpret_cast<HWND>(lParam) == GetDlgItem(hWnd, kSubtitleSizeSliderId))
        {
            sceneContext->subtitleSizePercent = std::clamp(
                static_cast<int>(SendMessageA(reinterpret_cast<HWND>(lParam), TBM_GETPOS, 0, 0)), 80, 160);
            UpdateSettingsLabels(*sceneContext);
            if (LOWORD(wParam) != TB_THUMBTRACK) SaveSettings(*sceneContext);
            return 0;
        }
        if (sceneContext && reinterpret_cast<HWND>(lParam) == GetDlgItem(hWnd, kMusicVolumeSliderId))
        {
            sceneContext->musicVolumePercent = std::clamp(
                static_cast<int>(SendMessageA(reinterpret_cast<HWND>(lParam), TBM_GETPOS, 0, 0)), 0, 100);
            UpdateSettingsLabels(*sceneContext);
            PublishMusicPlayback(*sceneContext);
            if (LOWORD(wParam) != TB_THUMBTRACK)
            {
                SaveSettings(*sceneContext);
            }
            return 0;
        }
        if (sceneContext && reinterpret_cast<HWND>(lParam) == GetDlgItem(hWnd, kRenderScaleSliderId))
        {
            const int percentage = horde::graphics::GraphicsRenderScalePercentFromSliderPosition(
                static_cast<int>(SendMessageA(reinterpret_cast<HWND>(lParam), TBM_GETPOS, 0, 0)));
            if (sceneContext->graphicsEdit)
            {
                auto draft = sceneContext->graphicsEdit->Draft();
                draft.renderScalePercent = percentage;
                sceneContext->graphicsEdit->Stage(draft);
            }
            UpdateSettingsLabels(*sceneContext);
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
                    CancelBenchmark(*sceneContext, true, "user-pause");
                }
                return 0;
            }
            const int commandId = LOWORD(wParam);
            if(LookoutPanel::Handles(commandId)) {
                if(sceneContext->lookoutObjective) {
                    const bool visible=sceneContext->lookoutObjective->Visible();
                    const auto action=sceneContext->lookoutObjective->Consume(commandId,reinterpret_cast<HWND>(lParam),
                        CurrentLookoutObjectiveView(*sceneContext));
                    if(action)FinishLookoutObjective(*sceneContext,*action);
                    else if(visible && !sceneContext->lookoutObjective->Visible())ApplyOverlayState(*sceneContext);
                }
                return 0;
            }
            if(sceneContext->lookoutObjective && sceneContext->lookoutObjective->Visible()) {
                if(commandId==kMenuPauseId)CancelLookoutObjective(*sceneContext);
                if(commandId!=kExitButtonId && commandId!=kMenuExitId)return 0;
            }
            if(sceneContext->campaignMenu && sceneContext->campaignMenu->HandleCommand(commandId))return 0;
            if(sceneContext->campaignMenu && sceneContext->campaignMenu->Visible() &&
               commandId!=kExitButtonId && commandId!=kMenuExitId)return 0;
            if (!sceneContext->rtLabVisible &&
                (sceneContext->deathOverlayVisible || sceneContext->endingOverlayVisible) &&
                commandId != kResumeButtonId && commandId != kRestartButtonId &&
                commandId != kRtLabButtonId && commandId != kRtLabBackButtonId &&
                commandId != kMenuRestartId && commandId != kExitButtonId &&
                commandId != kMenuExitId && commandId != kMenuTeachingEnabledId &&
                commandId != kMenuTeachingSlowdownId && commandId != kMenuTeachingSkipId &&
                commandId != kMenuTeachingReplayId && commandId != kCampaignLoadButtonId)
            {
                return 0;
            }
            switch (commandId)
            {
            case kLookoutObjectiveButtonId:
                if(HIWORD(wParam)==BN_CLICKED && reinterpret_cast<HWND>(lParam)==GetDlgItem(hWnd,kLookoutObjectiveButtonId))
                    OpenLookoutObjective(*sceneContext);
                return 0;
            case kCampaignSaveButtonId:
                OpenCampaignMenu(*sceneContext,horde::platform::windows::WindowsCampaignMenu::Page::Save);return 0;
            case kCampaignLoadButtonId:
                OpenCampaignMenu(*sceneContext,horde::platform::windows::WindowsCampaignMenu::Page::Load);return 0;
            case kMenuTeachingEnabledId:
                sceneContext->combatTeachingEnabled = !sceneContext->combatTeachingEnabled;
                SaveSettings(*sceneContext);
                ApplyOverlayState(*sceneContext);
                PlaySoundEffect(*sceneContext, "ui_select.wav");
                return 0;
            case kMenuTeachingSlowdownId:
                sceneContext->combatTeachingSlowdown = !sceneContext->combatTeachingSlowdown;
                SaveSettings(*sceneContext);
                ApplyOverlayState(*sceneContext);
                PlaySoundEffect(*sceneContext, "ui_select.wav");
                return 0;
            case kMenuTeachingSkipId:
                if (sceneContext->combatTeachingSkipSequence != UINT64_MAX)
                    ++sceneContext->combatTeachingSkipSequence;
                PlaySoundEffect(*sceneContext, "ui_select.wav");
                return 0;
            case kMenuTeachingReplayId:
                sceneContext->combatTeachingEnabled = true;
                SaveSettings(*sceneContext);
                if (sceneContext->combatTeachingReplaySequence != UINT64_MAX)
                    ++sceneContext->combatTeachingReplaySequence;
                ApplyOverlayState(*sceneContext);
                PlaySoundEffect(*sceneContext, "ui_select.wav");
                return 0;
            case kEntryMoreButtonId:
                TraceOpeningMenuStage("more-command");
                sceneContext->entryMoreVisible = true;
                sceneContext->entryMenu.ShowSidePage(true);
                ApplyOverlayState(*sceneContext);
                {
                    RECT client{};
                    GetClientRect(hWnd, &client);
                    LayoutOverlayControls(hWnd, client.right, client.bottom);
                }
                FocusGameControl(GetDlgItem(hWnd, kControlsButtonId));
                return 0;
            case kEntryBackButtonId:
                sceneContext->entryMoreVisible = false;
                sceneContext->entryMenu.ShowSidePage(false);
                ApplyOverlayState(*sceneContext);
                {
                    RECT client{};
                    GetClientRect(hWnd, &client);
                    LayoutOverlayControls(hWnd, client.right, client.bottom);
                }
                FocusGameControl(GetDlgItem(hWnd, kEntryMoreButtonId));
                return 0;
            case kResumeButtonId:
                if (sceneContext->entryMenuVisible)
                {
                    OpenCampaignMenu(*sceneContext,horde::platform::windows::WindowsCampaignMenu::Page::NewGame);
                    return 0;
                }
                if (sceneContext->deathOverlayVisible)
                {
                    if (!ApplyPlayerRetryCheckpoint(*sceneContext, sceneContext->playerRetryCheckpoint))
                    {
                        GameMessageBoxA(hWnd, "The encounter checkpoint could not be restored.", "Horde Lantern RT", MB_OK | MB_ICONERROR);
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
                CancelBenchmark(*sceneContext, false, "user-restart");
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
                if (sceneContext->entryMenuVisible)
                    sceneContext->entryMenu.ShowSidePage(true);
                OpenSettings(*sceneContext);
                return 0;
            case kReportProblemButtonId:
            case kMenuReportProblemId:
                OpenPlaytestReport(*sceneContext);
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
            case kHudControlId:
            case kMenuDiagnosticsId:
                ToggleDiagnostics(*sceneContext);
                return 0;
            case kRunBenchmarkButtonId:
                if(OpeningBenchmarkEligibilityFor(*sceneContext).Eligible()) {
                    sceneContext->benchmarkChooserVisible=true;
                    sceneContext->benchmarkChoice=OpeningBenchmarkChoice::None;
                    ApplyOverlayState(*sceneContext);
                    RECT r{};GetClientRect(hWnd,&r);LayoutOverlayControls(hWnd,r.right,r.bottom);
                    FocusGameControl(GetDlgItem(hWnd,kBenchmarkStandardChoiceId));
                }
                return 0;
            case kBenchmarkStandardChoiceId:
            case kBenchmarkCustomChoiceId:
                if(sceneContext->benchmarkChooserVisible) {
                    sceneContext->benchmarkChoice=LOWORD(wParam)==kBenchmarkStandardChoiceId ?
                        OpeningBenchmarkChoice::WindowsProvisional : OpeningBenchmarkChoice::Custom;
                    RefreshOpeningBenchmarkChooser(*sceneContext);
                }
                return 0;
            case kBenchmarkChoiceBackId:
                sceneContext->benchmarkChooserVisible=false;
                ApplyOverlayState(*sceneContext);
                FocusGameControl(GetDlgItem(hWnd,kRunBenchmarkButtonId));return 0;
            case kBenchmarkChoiceRunId:
                if(sceneContext->benchmarkChooserVisible && sceneContext->openingBenchmark &&
                    !(sceneContext->fullscreen && sceneContext->benchmarkChoice==OpeningBenchmarkChoice::WindowsProvisional) &&
                    sceneContext->openingBenchmark->owner.Queue(sceneContext->benchmarkChoice,
                        OpeningBenchmarkEligibilityFor(*sceneContext))) {
                    sceneContext->openingBenchmark->cancelPending=sceneContext->openingBenchmark->closeRequested=false;
                    sceneContext->benchmarkChooserVisible=false;
                    ApplyOverlayState(*sceneContext);
                }
                return 0;
#if defined(_DEBUG)
            case kMenuDeveloperOverlayId:
                ToggleDeveloperOverlay(*sceneContext);
                return 0;
#endif
            case kSensitivityButtonId:
                sceneContext->mouseSensitivity = sceneContext->mouseSensitivity < 0.8f ? 1.0f :
                                                 (sceneContext->mouseSensitivity < 1.2f ? 1.35f : 0.70f);
                SaveSettings(*sceneContext);
                UpdateSettingsLabels(*sceneContext);
                PlaySoundEffect(*sceneContext, "ui_select.wav");
                return 0;
            case kWaterQualityButtonId:
                if (sceneContext->graphicsEdit)
                {
                    auto draft = sceneContext->graphicsEdit->Draft();
                    draft.waterQuality = static_cast<horde::graphics::WaterQuality>((static_cast<unsigned>(draft.waterQuality) + 1u) % 3u);
                    sceneContext->graphicsEdit->Stage(draft);
                }
                UpdateSettingsLabels(*sceneContext);
                PlaySoundEffect(*sceneContext, "ui_select.wav");
                return 0;
            case kGraphicsOpenButtonId:
                if (sceneContext->rtScene.Profile() !=
                    horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview)
                    sceneContext->graphicsReturnProfile = sceneContext->rtScene.Profile();
                sceneContext->graphicsSceneRestoring = false;
                SpatialAudioEngine().StopLoop("waterfall");
                sceneContext->graphicsVisible = true;
                sceneContext->graphicsCloseAfterRevert = false;
                sceneContext->graphicsEdit.emplace(sceneContext->savedGraphics, sceneContext->graphicsSerialFloor);
                sceneContext->graphicsConfirmationTick = GetTickCount64();
                sceneContext->sceneProfile = horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview;
                sceneContext->sceneProfileDirty = sceneContext->rtScene.Profile() != sceneContext->sceneProfile;
                ClearDesktopInput(*sceneContext);
                ApplyOverlayState(*sceneContext);
                {
                    RECT bounds{};
                    MONITORINFO monitor{sizeof(MONITORINFO)};
                    if (GetWindowRect(hWnd, &bounds) && GetMonitorInfoA(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &monitor))
                    {
                        const int requiredWidth = std::min(ScaleForDpi(hWnd, 800), static_cast<int>(monitor.rcWork.right - monitor.rcWork.left));
                        if (bounds.right - bounds.left < requiredWidth)
                            SetWindowPos(hWnd, nullptr, std::clamp(bounds.left, monitor.rcWork.left, monitor.rcWork.right - requiredWidth),
                                bounds.top, requiredWidth, bounds.bottom - bounds.top, SWP_NOZORDER | SWP_NOACTIVATE);
                    }
                    RECT client{}; GetClientRect(hWnd, &client);
                    LayoutOverlayControls(hWnd, client.right, client.bottom);
                }
                FocusGameControl(GetDlgItem(hWnd, kGraphicsPresetButtonId));
                return 0;
            case kGraphicsPresetButtonId:
                if (sceneContext->graphicsEdit)
                {
                    const auto current = horde::graphics::MatchGraphicsPreset(sceneContext->graphicsEdit->Draft(), horde::graphics::GraphicsPlatform::Windows);
                    sceneContext->graphicsEdit->Stage(current == horde::graphics::GraphicsPreset::AcceptedBaseline ?
                        horde::graphics::ReducedEffectsGraphicsSettings(horde::graphics::GraphicsPlatform::Windows) :
                        horde::graphics::BaselineGraphicsSettings(horde::graphics::GraphicsPlatform::Windows));
                    UpdateSettingsLabels(*sceneContext);
                }
                return 0;
            case kGraphicsFireButtonId:
                if (sceneContext->graphicsEdit)
                {
                    auto draft = sceneContext->graphicsEdit->Draft();
                    draft.fireDetail = static_cast<horde::graphics::FireDetail>(
                        (static_cast<unsigned>(draft.fireDetail) + 1u) % 3u);
                    sceneContext->graphicsEdit->Stage(draft);
                    UpdateSettingsLabels(*sceneContext);
                }
                return 0;
            case kGraphicsShadowButtonId:
                if (sceneContext->graphicsEdit)
                {
                    auto draft = sceneContext->graphicsEdit->Draft();
                    draft.shadowQuality = horde::graphics::NextShadowQuality(draft.shadowQuality);
                    sceneContext->graphicsEdit->Stage(draft);
                    UpdateSettingsLabels(*sceneContext);
                }
                return 0;
            case kGraphicsGlassButtonId:
                if (sceneContext->graphicsEdit)
                {
                    auto draft = sceneContext->graphicsEdit->Draft();
                    draft.glassEnabled = !draft.glassEnabled;
                    sceneContext->graphicsEdit->Stage(draft);
                    UpdateSettingsLabels(*sceneContext);
                }
                return 0;
            case kGraphicsMistButtonId:
                if (sceneContext->graphicsEdit)
                {
                    auto draft = sceneContext->graphicsEdit->Draft();
                    draft.mistEnabled = !draft.mistEnabled;
                    sceneContext->graphicsEdit->Stage(draft);
                    UpdateSettingsLabels(*sceneContext);
                }
                return 0;
            case kGraphicsDustButtonId:
                if (sceneContext->graphicsEdit)
                {
                    auto draft = sceneContext->graphicsEdit->Draft();
                    draft.dustQuality = static_cast<horde::graphics::DustQuality>(
                        (static_cast<unsigned>(draft.dustQuality) + 1u) % 3u);
                    sceneContext->graphicsEdit->Stage(draft);
                    UpdateSettingsLabels(*sceneContext);
                }
                return 0;
            case kGraphicsApplyButtonId:
                if (sceneContext->graphicsEdit)
                    (void)QueueGraphicsCommand(*sceneContext, sceneContext->graphicsEdit->RequestApply(1u));
                return 0;
            case kGraphicsConfirmButtonId:
                if (sceneContext->graphicsEdit && sceneContext->graphicsEdit->State() == horde::graphics::GraphicsEditState::AwaitingConfirmation)
                {
                    auto record = sceneContext->graphicsEdit->Persistence();
                    record.confirmed = sceneContext->graphicsEdit->Draft(); record.pending.reset();
                    if (SaveGraphicsRecord(*sceneContext, record) && sceneContext->graphicsEdit->Confirm())
                    {
                        sceneContext->savedGraphics = sceneContext->graphicsEdit->Committed();
                        SaveSettings(*sceneContext);
                        sceneContext->graphicsStatus = "Graphics settings confirmed and saved beside the demo.";
                    }
                    else sceneContext->graphicsStatus = "Could not save graphics. Revert remains available.";
                    UpdateSettingsLabels(*sceneContext);
                }
                return 0;
            case kGraphicsRevertButtonId:
                if (sceneContext->graphicsEdit)
                    (void)QueueGraphicsCommand(*sceneContext, sceneContext->graphicsEdit->RequestRevert(1u));
                return 0;
            case kGraphicsResetButtonId:
                if (sceneContext->graphicsEdit)
                {
                    sceneContext->graphicsEdit->ResetDraft(horde::graphics::GraphicsPlatform::Windows);
                    sceneContext->graphicsStatus = "Platform defaults staged. Use and Keep to save; saved settings stay unchanged.";
                    UpdateSettingsLabels(*sceneContext);
                }
                return 0;
            case kGraphicsPreviewPauseId:
                sceneContext->graphicsPreviewPaused = !sceneContext->graphicsPreviewPaused;
                sceneContext->graphicsPreview.Pause(sceneContext->graphicsPreviewPaused);
                SetWindowTextA(GetDlgItem(hWnd, kGraphicsPreviewPauseId), sceneContext->graphicsPreviewPaused ? "RESUME PREVIEW" : "PAUSE PREVIEW");
                return 0;
            case kGraphicsPreviewCameraId:
                sceneContext->graphicsPreviewCamera = static_cast<horde::graphics::GraphicsPreviewCamera>(
                    (static_cast<unsigned>(sceneContext->graphicsPreviewCamera) + 1u) % 6u);
                sceneContext->graphicsPreview.SelectCamera(sceneContext->graphicsPreviewCamera);
                {
                    const char* names[]{"Overview", "Materials", "Glass", "Water", "Skeleton", "Mirror"};
                    SetWindowTextA(GetDlgItem(hWnd, kGraphicsPreviewCameraId),
                        (std::string("VIEW: ") + names[static_cast<unsigned>(sceneContext->graphicsPreviewCamera)]).c_str());
                }
                sceneContext->graphicsPreviewPerformance.BeginScope(++sceneContext->graphicsPreviewEpoch);
                return 0;
            case kGraphicsPreviewMotionId:
                sceneContext->graphicsPreviewMotion = !sceneContext->graphicsPreviewMotion;
                sceneContext->graphicsPreview.SetMotion(sceneContext->graphicsPreviewMotion);
                SetWindowTextA(GetDlgItem(hWnd, kGraphicsPreviewMotionId), sceneContext->graphicsPreviewMotion ? "MOTION TEST: ON" : "MOTION TEST: OFF");
                return 0;
            case kGraphicsPreviewResetId:
                sceneContext->graphicsPreview.Reset();
                sceneContext->graphicsPreviewPerformance.BeginScope(++sceneContext->graphicsPreviewEpoch);
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
                CancelBenchmark(*sceneContext, true, "fullscreen-change");
                ToggleFullscreen(*sceneContext);
                PlaySoundEffect(*sceneContext, "ui_select.wav");
                return 0;
            case kSettingsBackButtonId:
                if (sceneContext->graphicsVisible)
                {
                    sceneContext->graphicsCloseAfterRevert = true;
                    QueueGraphicsCommand(*sceneContext, sceneContext->graphicsEdit->RequestRevert(1u));
                    return 0;
                }
                sceneContext->settingsVisible = false;
                if (sceneContext->entryMenuVisible)
                    sceneContext->entryMenu.ShowSidePage(false);
                sceneContext->pauseMenuVisible = true;
                ApplyOverlayState(*sceneContext);
                PlaySoundEffect(*sceneContext, "ui_back.wav");
                FocusGameControl(GetDlgItem(hWnd, kResumeButtonId));
                return 0;
            case kSettingsMoreId:
                sceneContext->settingsDialoguePage = !sceneContext->settingsDialoguePage;
                ApplyOverlayState(*sceneContext);
                UpdateSettingsLabels(*sceneContext);
                {
                    RECT client{};
                    GetClientRect(hWnd, &client);
                    LayoutOverlayControls(hWnd, client.right, client.bottom);
                }
                FocusGameControl(GetDlgItem(hWnd, sceneContext->settingsDialoguePage
                    ? kVoiceVolumeSliderId : kSfxVolumeSliderId));
                return 0;
            case kSubtitlesButtonId:
                sceneContext->subtitlesEnabled = !sceneContext->subtitlesEnabled;
                UpdateSettingsLabels(*sceneContext);
                SaveSettings(*sceneContext);
                return 0;
            case kSubtitlePositionButtonId:
                sceneContext->subtitlePosition = static_cast<horde::gameplay::dialogue::SubtitlePosition>(
                    (static_cast<int>(sceneContext->subtitlePosition) + 1) % 3);
                UpdateSettingsLabels(*sceneContext);
                SaveSettings(*sceneContext);
                return 0;
            case kMenuAboutId:
                GameMessageBoxA(hWnd,
                            kAboutText,
                            "About Horde Lantern RT",
                            MB_OK | MB_ICONINFORMATION);
                return 0;
            case kMenuCreditsId:
                ShowCredits(hWnd);
                return 0;
            case kMenuCheckUpdatesId:
                horde::platform::windows::BeginGitHubReleaseUpdateCheck(
                    hWnd, HORDE_RT_DISPLAY_VERSION, true);
                return 0;
            case kMoreBySamfa12ButtonId:
                PlaySoundEffect(*sceneContext, "ui_select.wav");
                OpenSamfa12Website(hWnd);
                return 0;
            case kBenchmarkReviewStatsButtonId:
                if (!sceneContext->benchmark.IsRunning() && sceneContext->benchmarkSummary &&
                    sceneContext->benchmarkSummary->IsReady())
                    ShowWindowsBenchmarkSummaryReview(hWnd, *sceneContext->benchmarkSummary);
                return 0;
            case kBenchmarkCopyButtonId:
                if (!CopyTextToClipboard(hWnd, sceneContext->benchmarkReport))
                {
                    GameMessageBoxA(hWnd, "The benchmark report could not be copied to the clipboard.",
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
                FocusGameControl(GetDlgItem(hWnd, kRunBenchmarkButtonId));
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
            sceneContext->teachingControllerGlyphs = false;
            if(wParam==VK_ESCAPE && sceneContext->lookoutObjective && sceneContext->lookoutObjective->Visible()) {
                if((lParam & (1ll<<30))==0)CancelLookoutObjective(*sceneContext);
                return 0;
            }
            if(wParam==VK_ESCAPE && sceneContext->campaignMenu && sceneContext->campaignMenu->Visible()){sceneContext->campaignMenu->HandleCommand(4121);return 0;}
            if (sceneContext->simulationPaused && wParam == VK_TAB && (lParam & (1ll << 30)) == 0)
            {
                NavigateControllerMenu(*sceneContext,
                    (GetKeyState(VK_SHIFT) & 0x8000) != 0 ? -1 : 1);
                return 0;
            }
            if (sceneContext->simulationPaused && (wParam == VK_UP || wParam == VK_DOWN) &&
                (lParam & (1ll << 30)) == 0)
            {
                if (sceneContext->graphicsVisible)
                    NavigateGraphicsMenu(*sceneContext, wParam == VK_UP
                        ? horde::platform::windows::GraphicsMenuDirection::Up
                        : horde::platform::windows::GraphicsMenuDirection::Down);
                else
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
                {
                    if (sceneContext->graphicsVisible)
                        NavigateGraphicsMenu(*sceneContext, wParam == VK_RIGHT
                            ? horde::platform::windows::GraphicsMenuDirection::Right
                            : horde::platform::windows::GraphicsMenuDirection::Left);
                    else
                        NavigateControllerMenu(*sceneContext, wParam == VK_RIGHT ? 1 : -1);
                }
                return 0;
            }
            if (sceneContext->simulationPaused && (wParam == VK_RETURN || wParam == VK_SPACE) &&
                (GetKeyState(VK_MENU) & 0x8000) == 0 && (lParam & (1ll << 30)) == 0)
            {
                if (HWND focused = GetFocus()) SendMessageA(focused, BM_CLICK, 0, 0);
                return 0;
            }
            if (!sceneContext->simulationPaused && wParam == VK_TAB &&
                (GetKeyState(VK_MENU) & 0x8000) == 0 && (lParam & (1ll << 30)) == 0 &&
                sceneContext->simulation.Snapshot().chapterDialogue.line != horde::gameplay::dialogue::Line::None)
            {
                if (sceneContext->dialogueSkipSequence != UINT64_MAX)
                    ++sceneContext->dialogueSkipSequence;
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
                if(sceneContext->benchmarkChooserVisible) {SendMessageA(hWnd,WM_COMMAND,kBenchmarkChoiceBackId,0);return 0;}
                if(sceneContext->campaignMenu && sceneContext->campaignMenu->Visible()){sceneContext->campaignMenu->HandleCommand(4121);return 0;}
                if ((lParam & (1ll << 30)) != 0) return 0;
                if (sceneContext->graphicsVisible && sceneContext->graphicsEdit)
                {
                    sceneContext->graphicsCloseAfterRevert = true;
                    (void)QueueGraphicsCommand(*sceneContext, sceneContext->graphicsEdit->RequestRevert(1u));
                    return 0;
                }
                if (sceneContext->rtLabVisible)
                {
                    CloseRtLab(*sceneContext);
                    return 0;
                }
                if (sceneContext->benchmark.IsRunning())
                {
                    CancelBenchmark(*sceneContext, true, "escape");
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
                ++sceneContext->musicResetToken;
                if (sceneContext->rtFrameEvidenceInitialised)
                {
                    (void)sceneContext->rtFrameEvidence.ApplyEvent(
                        horde::telemetry::RtLifecycleEvent::CheckpointChange);
                }
                sceneContext->simulation.ClearEvents();
                MirrorSimulationSnapshot(*sceneContext);
                return 0;
            }
            if (wParam == VK_F7)
            {
                sceneContext->debugEnemyOverride = horde::gameplay::EnemyKind::None;
                sceneContext->simulation.ApplyShowcaseCheckpoint(3);
                ++sceneContext->musicResetToken;
                if (sceneContext->rtFrameEvidenceInitialised)
                {
                    (void)sceneContext->rtFrameEvidence.ApplyEvent(
                        horde::telemetry::RtLifecycleEvent::CheckpointChange);
                }
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
                PlayAmbientSoundEffect(*sceneContext, clip, horde::audio::kPlayerFootstepCueGain);
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
                    input.analogueMovement = false;
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
            if (wParam == 'R' && !sceneContext->simulationPaused &&
                (!sceneContext->lookoutMovementNeedsFreshKeyDown || (lParam & (1ll<<30))==0))
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
            using horde::platform::windows::DesktopKeyAction;
            const bool keyboardGameplayHasFocus = IsDesktopGameplayAvailable(*sceneContext) &&
                GetFocus() == hWnd && GetForegroundWindow() == hWnd;
            if (horde::platform::windows::ShouldAcquireDesktopMouseLookForKey(
                    static_cast<unsigned>(wParam), keyboardGameplayHasFocus,
                    sceneContext->mouseLookActive && GetCapture() == hWnd,
                    (lParam & (1ll << 30)) != 0,
                    (GetKeyState(VK_MENU) & 0x8000) != 0 || (GetKeyState(VK_CONTROL) & 0x8000) != 0,
                    sceneContext->simulation.Snapshot().chestReward.phase ==
                        horde::gameplay::interactions::ChestRewardPhase::LanternClaimed,
                    sceneContext->simulation.Snapshot().healingFlaskEnabled))
            {
                // Losing capture/focus during handoff consumes this key. Do
                // not republish a movement/defense hold after cancellation.
                if (!AcquireDesktopMouseLook(*sceneContext)) return 0;
            }
            const DesktopKeyAction keyAction = horde::platform::windows::ResolveDesktopGameplayKey(
                static_cast<unsigned>(wParam),
                IsDesktopGameplayAvailable(*sceneContext) && GetFocus() == hWnd && GetForegroundWindow() == hWnd &&
                    (GetKeyState(VK_MENU) & 0x8000) == 0,
                (lParam & (1ll << 30)) != 0,
                sceneContext->simulation.Snapshot().chestReward.phase ==
                    horde::gameplay::interactions::ChestRewardPhase::LanternClaimed,
                sceneContext->simulation.Snapshot().healingFlaskEnabled);
            if(keyAction==DesktopKeyAction::DrinkFlask || keyAction==DesktopKeyAction::Rest) {
                PublishDesktopHealingFlaskAction(*sceneContext,keyAction==DesktopKeyAction::Rest);return 0;
            }
            if (keyAction == DesktopKeyAction::Dodge || keyAction == DesktopKeyAction::Parry)
            {
                if (keyAction == DesktopKeyAction::Parry)
                {
                    sceneContext->keyboardGuardHeld = true;
                    PublishDesktopGuardHold(*sceneContext);
                }
                else PublishDesktopCombatEdge(*sceneContext, horde::gameplay::simulation::CombatInputEdgeKind::Dodge);
                return 0;
            }
            if (keyAction == DesktopKeyAction::ToggleLantern)
            {
                ++sceneContext->toggleHeldLightPoseSequence;
                return 0;
            }
            if (IsDesktopGameplayAvailable(*sceneContext) && wParam == VK_CAPITAL &&
                (lParam & (1ll << 30)) == 0)
            {
                sceneContext->runToggleKeyDown = true;
                if (sceneContext->runToggleSequence != UINT64_MAX)
                    ++sceneContext->runToggleSequence;
                return 0;
            }
            if (IsDesktopGameplayAvailable(*sceneContext) &&
                (!sceneContext->lookoutMovementNeedsFreshKeyDown || (lParam & (1ll<<30))==0) &&
                SetDesktopMovementKey(*sceneContext, wParam, true))
            {
                return 0;
            }
        }
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (sceneContext && wParam == 'Q')
        {
            sceneContext->keyboardGuardHeld = false;
            PublishDesktopGuardHold(*sceneContext);
        }
        if (sceneContext && wParam == VK_CAPITAL)
            sceneContext->runToggleKeyDown = false;
        if (sceneContext && sceneContext->controlsEnabled && SetDesktopMovementKey(*sceneContext, wParam, false))
        {
            return 0;
        }
        break;
    case WM_LBUTTONDOWN:
        if (sceneContext) sceneContext->teachingControllerGlyphs = false;
        if (sceneContext)
        {
            using horde::platform::windows::DesktopClickAction;
            const auto& snapshot = sceneContext->simulation.Snapshot();
            const bool capturedAndFocused = sceneContext->mouseLookActive && GetCapture() == hWnd &&
                GetFocus() == hWnd && GetForegroundWindow() == hWnd;
            DesktopClickAction clickAction;
            if (snapshot.developmentRescueJourney &&
                (snapshot.rescuePrompt != horde::gameplay::traversal::RescuePrompt::None ||
                 sceneContext->rescueInteractionPromptPresented || snapshot.flaskUpgrade.interactEligible ||
                 sceneContext->flaskUpgradeInteractionPromptPresented))
            {
                // Rescue mode owns every click. A stale/disabled rescue prompt
                // is consumed without falling through to the attack action.
                clickAction = !IsDesktopGameplayAvailable(*sceneContext) ? DesktopClickAction::Ignore :
                    !capturedAndFocused ? DesktopClickAction::AcquireCapture :
                    ((sceneContext->rescueInteractionPromptPresented &&
                      (snapshot.rescuePrompt == horde::gameplay::traversal::RescuePrompt::Climb ||
                       snapshot.rescuePrompt == horde::gameplay::traversal::RescuePrompt::Descend)) ||
                     (sceneContext->flaskUpgradeInteractionPromptPresented && snapshot.flaskUpgrade.interactEligible))
                        ? DesktopClickAction::Interact : DesktopClickAction::Ignore;
            }
            else
            {
                clickAction = horde::platform::windows::ResolveDesktopLeftClick(
                    IsDesktopGameplayAvailable(*sceneContext), capturedAndFocused,
                    snapshot.chestPrompt, sceneContext->chestInteractionPromptPresented);
            }
            if (clickAction == DesktopClickAction::AcquireCapture)
            {
                // The capture/focus click is consumed even if acquisition fails.
                (void)AcquireDesktopMouseLook(*sceneContext);
            }
            else if (clickAction == DesktopClickAction::Interact)
            {
                ++sceneContext->interactSequence;
            }
            else if (clickAction == DesktopClickAction::Attack)
            {
                PublishDesktopCombatEdge(*sceneContext, horde::gameplay::simulation::CombatInputEdgeKind::Attack);
            }
            return 0;
        }
        break;
    case WM_RBUTTONDOWN:
        // Reserved: no capture, interaction or attack.
        return 0;
    case WM_MOUSEMOVE:
        if (sceneContext && IsDesktopGameplayAvailable(*sceneContext) && sceneContext->mouseLookActive &&
            GetCapture() == hWnd && GetFocus() == hWnd && GetForegroundWindow() == hWnd)
        {
            const POINT currentMousePosition{
                static_cast<LONG>(static_cast<short>(LOWORD(lParam))),
                static_cast<LONG>(static_cast<short>(HIWORD(lParam)))};
            const LONG deltaX = currentMousePosition.x - sceneContext->lastMousePosition.x;
            const LONG deltaY = currentMousePosition.y - sceneContext->lastMousePosition.y;
            if (deltaX != 0 || deltaY != 0) sceneContext->teachingControllerGlyphs = false;
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
            ClearDesktopInput(*sceneContext); DiscardDesktopPendingCommands(*sceneContext);
            return 0;
        }
        break;
    case WM_KILLFOCUS:
        if (sceneContext && sceneContext->controlsEnabled)
        {
            ClearDesktopInput(*sceneContext); DiscardDesktopPendingCommands(*sceneContext);
        }
        break;
    case WM_ACTIVATEAPP:
        if (sceneContext)
        {
            sceneContext->lookoutAppActive=wParam!=FALSE;
            if(wParam==FALSE && sceneContext->campaignMenu)sceneContext->campaignMenu->HoldLoadedGame();
            if(wParam==FALSE)RevokeLookoutObjective(*sceneContext);
            if (wParam == FALSE) { ClearDesktopInput(*sceneContext); DiscardDesktopPendingCommands(*sceneContext); sceneContext->controllerFocusLatch.LoseFocus(); StopMenuAmbience(*sceneContext); }
            PublishMusicPlayback(*sceneContext, wParam == FALSE);
        }
        if (sceneContext && wParam == FALSE && sceneContext->benchmark.IsRunning())
        {
            CancelBenchmark(*sceneContext, true, "app-deactivated");
        }
        break;
    case WM_ACTIVATE:
        if (sceneContext)
        {
            sceneContext->lookoutAppActive=LOWORD(wParam)!=WA_INACTIVE;
            if(LOWORD(wParam)==WA_INACTIVE)RevokeLookoutObjective(*sceneContext);
            if (LOWORD(wParam) == WA_INACTIVE) { ClearDesktopInput(*sceneContext); DiscardDesktopPendingCommands(*sceneContext); sceneContext->controllerFocusLatch.LoseFocus(); StopMenuAmbience(*sceneContext); }
            PublishMusicPlayback(*sceneContext, LOWORD(wParam) == WA_INACTIVE);
        }
        break;
    case WM_SIZE:
        if(sceneContext && wParam==SIZE_MINIMIZED)RevokeLookoutObjective(*sceneContext);
        if (sceneContext && sceneContext->benchmark.IsRunning() && wParam != SIZE_MINIMIZED)
        {
            CancelBenchmark(*sceneContext, true, "window-size");
        }
        LayoutOverlayControls(hWnd, LOWORD(lParam), HIWORD(lParam));
        // Moved owner-drawn children must repaint their complete client areas;
        // preserving old window bits can otherwise leave stale button labels
        // until focus happens to invalidate each individual control.
        if (sceneContext && wParam != SIZE_MINIMIZED)
            RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        return 0;
    case WM_TIMER:
        if (wParam == kEntryLoadingIndicatorId && sceneContext && sceneContext->entryLoadingVisible)
        {
            InvalidateRect(GetDlgItem(hWnd, kEntryLoadingIndicatorId), nullptr, FALSE);
            return 0;
        }
        break;
    case WM_GETMINMAXINFO:
    {
        if (GetPropA(hWnd, kCaptureModeProperty) != nullptr)
        {
            return 0;
        }
        auto* minMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
        minMaxInfo->ptMinTrackSize.x = ScaleForDpi(hWnd, sceneContext && sceneContext->graphicsVisible ? 780 : 520);
        minMaxInfo->ptMinTrackSize.y = ScaleForDpi(hWnd, 560);
        return 0;
    }
    case WM_DPICHANGED:
    {
        if (sceneContext && sceneContext->benchmark.IsRunning())
        {
            CancelBenchmark(*sceneContext, true, "dpi-change");
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
        RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        return 0;
    }
    case WM_DRAWITEM:
    {
        const auto* item = reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
        if (item && item->CtlID == kChapterSubtitleId)
        {
            // Per-pixel layered child is updated only on actual label/layout changes.
            return TRUE;
        }
        if (item && item->CtlID == kCombatTeachingPromptId)
        {
            const bool highContrast = NativeUiUsesHighContrast();
            const auto classification = sceneContext
                ? sceneContext->simulation.Snapshot().combatTeaching.attackReadability
                : horde::gameplay::simulation::AttackReadability::Neutral;
            const std::uint32_t cueRgb = horde::platform::windows::AttackReadabilityColourRgb(classification);
            const COLORREF text = highContrast ? GetSysColor(COLOR_WINDOWTEXT)
                : RGB((cueRgb >> 16u) & 0xffu, (cueRgb >> 8u) & 0xffu, cueRgb & 0xffu);
            static HBRUSH slatePanel = CreateSolidBrush(RGB(24, 25, 27));
            static HBRUSH brassFrame = CreateSolidBrush(RGB(157, 124, 72));
            static HBRUSH ironInset = CreateSolidBrush(RGB(77, 68, 53));
            HBRUSH panel = highContrast ? GetSysColorBrush(COLOR_WINDOW) : slatePanel;
            HBRUSH frame = highContrast ? GetSysColorBrush(COLOR_WINDOWTEXT) : brassFrame;
            HBRUSH innerFrame = highContrast ? GetSysColorBrush(COLOR_WINDOWTEXT) : ironInset;
            FillRect(item->hDC, &item->rcItem, panel);
            FrameRect(item->hDC, &item->rcItem, frame);
            RECT inner = item->rcItem;
            InflateRect(&inner, -ScaleForDpi(hWnd, 4), -ScaleForDpi(hWnd, 4));
            FrameRect(item->hDC, &inner, innerFrame);
            RECT rule = inner;
            rule.left += ScaleForDpi(hWnd, 12);
            rule.right -= ScaleForDpi(hWnd, 12);
            rule.top += ScaleForDpi(hWnd, 3);
            rule.bottom = rule.top + ScaleForDpi(hWnd, 1);
            FillRect(item->hDC, &rule, frame);
            rule.top = inner.bottom - ScaleForDpi(hWnd, 4);
            rule.bottom = rule.top + ScaleForDpi(hWnd, 1);
            FillRect(item->hDC, &rule, frame);

            const HFONT font = reinterpret_cast<HFONT>(
                SendMessageA(item->hwndItem, WM_GETFONT, 0, 0));
            const HGDIOBJ previousFont = font ? SelectObject(item->hDC, font) : nullptr;
            SetTextColor(item->hDC, text);
            SetBkMode(item->hDC, TRANSPARENT);
            RECT label = item->rcItem;
            InflateRect(&label, -ScaleForDpi(hWnd, 18), -ScaleForDpi(hWnd, 9));
            char caption[256]{};
            GetWindowTextA(item->hwndItem, caption, static_cast<int>(sizeof(caption)));
            DrawTextA(item->hDC, caption, -1, &label,
                      DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_NOPREFIX);
            if (previousFont && previousFont != HGDI_ERROR) SelectObject(item->hDC, previousFont);
            return TRUE;
        }
        if (item && item->CtlID == kEntryLoadingIndicatorId)
        {
            // A small native loading indicator, with an accessible STATIC label.
            // Motion stays still while the owner is compiling the RT scene.
            const bool highContrast = NativeUiUsesHighContrast();
            FillRect(item->hDC, &item->rcItem,
                     highContrast ? GetSysColorBrush(COLOR_WINDOW) :
                         static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            BOOL animations = TRUE;
            SystemParametersInfoA(SPI_GETCLIENTAREAANIMATION, 0, &animations, 0);
            const int phase = animations ? static_cast<int>((GetTickCount64() / 100u) % 8u) : 0;
            const int cx = (item->rcItem.left + item->rcItem.right) / 2;
            const int cy = (item->rcItem.top + item->rcItem.bottom) / 2;
            const int radius = ScaleForDpi(hWnd, 11), dot = ScaleForDpi(hWnd, 2);
            const auto oldBrush = SelectObject(item->hDC, GetStockObject(DC_BRUSH));
            const auto oldPen = SelectObject(item->hDC, GetStockObject(NULL_PEN));
            for (int index = 0; index < 8; ++index)
            {
                const float angle = index * 0.7853981634f;
                const int x = cx + static_cast<int>(std::lround(std::cos(angle) * radius));
                const int y = cy + static_cast<int>(std::lround(std::sin(angle) * radius));
                SetDCBrushColor(item->hDC, highContrast ? GetSysColor(COLOR_WINDOWTEXT) :
                    (index == phase ? RGB(240, 214, 162) : RGB(112, 93, 60)));
                Ellipse(item->hDC, x - dot, y - dot, x + dot + 1, y + dot + 1);
            }
            SelectObject(item->hDC, oldPen);
            SelectObject(item->hDC, oldBrush);
            return TRUE;
        }
        if (item && item->CtlID == kVitalityHudControlId && sceneContext)
        {
            const bool highContrast = NativeUiUsesHighContrast();
            static HBRUSH slate = CreateSolidBrush(RGB(36, 39, 42));
            const auto& vitals = sceneContext->simulation.Snapshot().playerVitals;
            FillRect(item->hDC, &item->rcItem, highContrast ? GetSysColorBrush(COLOR_WINDOW) : slate);
            const int inset = ScaleForDpi(hWnd, 6);
            const int heartWidth = ScaleForDpi(hWnd, 18);
            const int heartHeight = ScaleForDpi(hWnd, 22);
            const HGDIOBJ previousBrush = SelectObject(item->hDC, GetStockObject(DC_BRUSH));
            const HPEN outline = CreatePen(PS_SOLID, ScaleForDpi(hWnd, 1),
                highContrast ? GetSysColor(COLOR_WINDOWTEXT) : RGB(242, 233, 216));
            const HGDIOBJ previousPen = SelectObject(item->hDC, outline);
            // Original vector heart path matches the native Android icon artwork.
            constexpr std::array<POINT, 13u> heart{{
                {12,21},{9,18},{2,12},{2,7},{2,1},{9,0},{12,5},
                {15,0},{22,1},{22,7},{22,12},{15,18},{12,21}}};
            const int gap = ScaleForDpi(hWnd, 4);
            const int columns = horde::platform::windows::VitalityHudColumns(
                item->rcItem.right - item->rcItem.left, heartWidth, gap, inset);
            for (int index = 0; index < std::max(0, vitals.maxVitality); ++index)
            {
                const int x = item->rcItem.left + inset + (index % columns) * (heartWidth + gap);
                const int y = item->rcItem.top + ScaleForDpi(hWnd, 4) +
                              (index / columns) * (heartHeight + gap);
                std::array<POINT, 13u> points{};
                for (std::size_t point = 0u; point < heart.size(); ++point)
                    points[point] = {x + heart[point].x * heartWidth / 24,
                                     y + heart[point].y * heartHeight / 24};
                SetDCBrushColor(item->hDC, highContrast ? GetSysColor(COLOR_HIGHLIGHT) : RGB(214, 71, 71));
                BeginPath(item->hDC);
                MoveToEx(item->hDC, points[0].x, points[0].y, nullptr);
                PolyBezierTo(item->hDC, points.data() + 1u, 12u);
                CloseFigure(item->hDC); EndPath(item->hDC);
                const bool half = vitals.halfHeart && index == vitals.vitality - 1;
                if (half)
                {
                    const int saved = SaveDC(item->hDC);
                    IntersectClipRect(item->hDC, x, y, x + heartWidth / 2, y + heartHeight);
                    StrokeAndFillPath(item->hDC);
                    RestoreDC(item->hDC, saved);
                    BeginPath(item->hDC);
                    MoveToEx(item->hDC, points[0].x, points[0].y, nullptr);
                    PolyBezierTo(item->hDC, points.data() + 1u, 12u);
                    CloseFigure(item->hDC); EndPath(item->hDC);
                    StrokePath(item->hDC);
                }
                else if (index < vitals.vitality) StrokeAndFillPath(item->hDC);
                else StrokePath(item->hDC);
            }
            if (previousPen && previousPen != HGDI_ERROR) SelectObject(item->hDC, previousPen);
            if (previousBrush && previousBrush != HGDI_ERROR) SelectObject(item->hDC, previousBrush);
            DeleteObject(outline);
            return TRUE;
        }
        if (item && item->CtlType == ODT_BUTTON)
        {
            const bool highContrast = NativeUiUsesHighContrast();
            const HFONT controlFont = reinterpret_cast<HFONT>(
                SendMessageA(item->hwndItem, WM_GETFONT, 0, 0));
            const HGDIOBJ previousFont = controlFont != nullptr
                ? SelectObject(item->hDC, controlFont) : nullptr;
            const bool disabled = (item->itemState & ODS_DISABLED) != 0u;
            const bool pressed = (item->itemState & ODS_SELECTED) != 0u;
            const bool focused = (item->itemState & ODS_FOCUS) != 0u;
            static HBRUSH slate = CreateSolidBrush(RGB(36, 39, 42));
            static HBRUSH inset = CreateSolidBrush(RGB(21, 23, 25));
            static HBRUSH brass = CreateSolidBrush(RGB(207, 169, 106));
            static HBRUSH iron = CreateSolidBrush(RGB(119, 126, 132));
            FillRect(item->hDC, &item->rcItem, highContrast ?
                GetSysColorBrush(pressed ? COLOR_HIGHLIGHT : COLOR_BTNFACE) : (pressed ? inset : slate));
            FrameRect(item->hDC, &item->rcItem, highContrast ? GetSysColorBrush(COLOR_BTNTEXT) :
                (focused || pressed ? brass : iron));
            if (!highContrast)
            {
                // Raised native plaque: bevel, recessed inset and four small
                // iron rivets retain real labels, hit targets and focus state.
                const int height = std::max(1L, item->rcItem.bottom - item->rcItem.top);
                for (int band = 0; band < 32; ++band)
                {
                    RECT grain = item->rcItem;
                    grain.top += band * height / 32;
                    grain.bottom = item->rcItem.top + (band + 1) * height / 32;
                    const int texture = ((band * 17 + static_cast<int>(item->CtlID)) % 9) - 4;
                    const int shade = (pressed ? 31 : 48) + texture - band / 4;
                    SetDCBrushColor(item->hDC, RGB(shade + 9, shade + 3, shade - 7));
                    FillRect(item->hDC, &grain, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
                }
                RECT inner = item->rcItem;
                InflateRect(&inner, -ScaleForDpi(hWnd, 4), -ScaleForDpi(hWnd, 4));
                FrameRect(item->hDC, &inner, pressed ? iron : brass);
                RECT edge = item->rcItem;
                edge.bottom = edge.top + ScaleForDpi(hWnd, 2);
                FillRect(item->hDC, &edge, pressed ? inset : iron);
                edge = item->rcItem;
                edge.top = edge.bottom - ScaleForDpi(hWnd, 3);
                FillRect(item->hDC, &edge, inset);
                const int rivet = ScaleForDpi(hWnd, 3), margin = ScaleForDpi(hWnd, 10);
                for (int x : {item->rcItem.left + margin, item->rcItem.right - margin})
                    for (int y : {item->rcItem.top + margin, item->rcItem.bottom - margin})
                    {
                        RECT bolt{x - rivet, y - rivet, x + rivet, y + rivet};
                        const auto previousBrush = SelectObject(item->hDC, iron);
                        const auto previousPen = SelectObject(item->hDC, GetStockObject(BLACK_PEN));
                        Ellipse(item->hDC, bolt.left, bolt.top, bolt.right, bolt.bottom);
                        MoveToEx(item->hDC, x - rivet + 1, y, nullptr);
                        LineTo(item->hDC, x + rivet - 1, y);
                        SelectObject(item->hDC, previousPen);
                        SelectObject(item->hDC, previousBrush);
                    }
            }
            RECT label = item->rcItem;
            const bool graphicsMenuButton = IsGraphicsMenuButton(static_cast<int>(item->CtlID));
            const int horizontalLabelInset = ScaleForDpi(hWnd, graphicsMenuButton ? 15 : 6);
            InflateRect(&label, -horizontalLabelInset, -ScaleForDpi(hWnd, 3));
            if (pressed) OffsetRect(&label, 1, 1);
            SetTextColor(item->hDC, highContrast ?
                GetSysColor(disabled ? COLOR_GRAYTEXT : (pressed ? COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT)) :
                (disabled ? RGB(174, 176, 172) : RGB(242, 233, 216)));
            SetBkMode(item->hDC, TRANSPARENT);
            char text[192]{};
            GetWindowTextA(item->hwndItem, text, static_cast<int>(sizeof(text)));
            HFONT fittedFont = graphicsMenuButton
                ? CreateGraphicsButtonFitFont(hWnd, item->hDC, controlFont, text,
                                              label.right - label.left)
                : nullptr;
            if (fittedFont != nullptr)
                SelectObject(item->hDC, fittedFont);
            DrawTextA(item->hDC, text, -1, &label, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            if (fittedFont != nullptr)
            {
                if (controlFont != nullptr) SelectObject(item->hDC, controlFont);
                DeleteObject(fittedFont);
            }
            if (focused && (item->itemState & ODS_NOFOCUSRECT) == 0u)
            {
                RECT focus = item->rcItem;
                InflateRect(&focus, -3, -3);
                DrawFocusRect(item->hDC, &focus);
            }
            if (previousFont && previousFont != HGDI_ERROR) SelectObject(item->hDC, previousFont);
            return TRUE;
        }
        if (!sceneContext || !item || item->CtlID != kGraphicsPreviewGraphId) break;
        static HBRUSH background = CreateSolidBrush(RGB(36, 39, 42));
        FillRect(item->hDC, &item->rcItem, background);
        SetTextColor(item->hDC, RGB(242, 233, 216));
        SetBkMode(item->hDC, TRANSPARENT);
        RECT label = item->rcItem;
        DrawTextA(item->hDC, "Loop interval history | amber = transition | top = 100 ms+", -1,
                  &label, DT_TOP | DT_LEFT | DT_SINGLELINE);
        const auto stats = sceneContext->graphicsPreviewPerformance.Snapshot();
        const int top = item->rcItem.top + ScaleForDpi(hWnd, 22);
        const int bottom = item->rcItem.bottom - 2;
        const int width = item->rcItem.right - item->rcItem.left;
        const int height = std::max(1, bottom - top);
        static HBRUSH stable = CreateSolidBrush(RGB(150, 168, 173));
        static HBRUSH transition = CreateSolidBrush(RGB(224, 170, 76));
        for (std::size_t index = 0; index < stats.sampleCount; ++index)
        {
            const auto& sample = stats.samples[index];
            const int barHeight = std::max(1, static_cast<int>(height * std::min(sample.loopMilliseconds, 100.0) / 100.0));
            const int x = item->rcItem.left + static_cast<int>(index * width / stats.samples.size());
            const int next = item->rcItem.left + static_cast<int>((index + 1) * width / stats.samples.size());
            RECT bar{x, bottom - barHeight, std::max(x + 1, next - 1), bottom};
            FillRect(item->hDC, &bar, sample.transition ? transition : stable);
        }
        return TRUE;
    }
    case WM_CTLCOLORSTATIC:
    {
        HDC dc = reinterpret_cast<HDC>(wParam);
        if (NativeUiUsesHighContrast())
        {
            SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
            SetBkColor(dc, GetSysColor(COLOR_WINDOW));
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
        }
        COLORREF textColor = RGB(242, 233, 216);
        const int controlId = GetDlgCtrlID(reinterpret_cast<HWND>(lParam));
        if (controlId == kGraphicsInfoId || controlId == kGraphicsPreviewTelemetryId)
        {
            SetTextColor(dc, RGB(242, 233, 216));
            SetBkColor(dc, RGB(36, 39, 42));
            static HBRUSH slate = CreateSolidBrush(RGB(36, 39, 42));
            return reinterpret_cast<LRESULT>(slate);
        }
        SetTextColor(dc, textColor);
        SetBkColor(dc, RGB(21, 23, 25));
        static HBRUSH brush = CreateSolidBrush(RGB(21, 23, 25));
        return reinterpret_cast<LRESULT>(brush);
    }
    case WM_DESTROY:
        if (sceneContext) {StopPrivateLichRevivalAudio(*sceneContext);StopMenuAmbience(*sceneContext);}
        horde::platform::windows::CancelGitHubReleaseUpdateCheck(hWnd);
        if (sceneContext && sceneContext->controlsEnabled)
        {
            ClearDesktopInput(*sceneContext);
        }
        ReleaseDpiScaledFonts(hWnd);
        RemovePropA(hWnd, kCaptureModeProperty);
        if(!CompatibilityFor(hWnd) || !CompatibilityFor(hWnd)->failureCleanup)PostQuitMessage(0);
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
    HMENU teaching = CreatePopupMenu();
    AppendMenuA(teaching, MF_STRING | MF_CHECKED, kMenuTeachingEnabledId, "Optional combat lesson");
    AppendMenuA(teaching, MF_STRING, kMenuTeachingSlowdownId, "Ease time during lesson");
    AppendMenuA(teaching, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(teaching, MF_STRING, kMenuTeachingSkipId, "Skip practice");
    AppendMenuA(teaching, MF_STRING, kMenuTeachingReplayId, "Replay opening practice");
    AppendMenuA(demo, MF_POPUP, reinterpret_cast<UINT_PTR>(teaching), "Combat &practice");
    AppendMenuA(demo, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(demo, MF_STRING, kMenuExitId, "E&xit");
    AppendMenuA(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(demo), "&Demo");

    HMENU settings = CreatePopupMenu();
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
    AppendMenuA(help, MF_STRING, kMenuCheckUpdatesId, "Check for &updates...");
    AppendMenuA(help, MF_STRING, kMenuReportProblemId, "&Report a problem...");
    AppendMenuA(help, MF_STRING, kMenuCompatibilityReportId, "Report &compatibility to developer...");
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
            const bool highContrast = NativeUiUsesHighContrast();
            const HBRUSH focusBorder = highContrast ? GetSysColorBrush(COLOR_HIGHLIGHT) : gold;
            const HBRUSH focusInner = highContrast ? GetSysColorBrush(COLOR_WINDOWTEXT) : brightGold;
            FrameRect(dc, &border, focusBorder);
            InflateRect(&border, -1, -1);
            FrameRect(dc, &border, focusBorder);
            InflateRect(&border, -1, -1);
            FrameRect(dc, &border, focusInner);
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
                        CompatibilityWindowOwner& compatibility,
                        const std::filesystem::path& textReportPath,
                        const std::filesystem::path& jsonReportPath,
                        const std::filesystem::path* captureDirectory,
                        const std::string* developmentCheckpoint,
                        const bool portraitCapture,
                        const bool requireRayQueryCompute,
                        const bool unattendedBenchmark,
                        const horde::gameplay::BenchmarkWorkload benchmarkWorkload,
                        const horde::vulkan::raytracing::RtWorkloadPreset benchmarkRtWorkloadPreset,
                        const bool anatomicalPlayerMount,
                        const bool graphicsPreviewCapture,
                        const bool outputResizeValidation,
                        const std::string& nativeMotionScenario,
                        const horde::vulkan::raytracing::RtWorkloadPreset nativeMotionRtWorkloadPreset,
                        const std::optional<horde::graphics::DustQuality> captureDustQuality)
{
    // Only the Debug capture surface changes aspect; camera, gameplay pose,
    // renderer quality and normal interactive-window sizing are untouched.
    const auto captureWidth = portraitCapture ? kCaptureHeight : kCaptureWidth;
    const auto captureHeight = portraitCapture ? kCaptureWidth : kCaptureHeight;
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
        RECT captureRect{0, 0, static_cast<LONG>(captureWidth), static_cast<LONG>(captureHeight)};
        AdjustWindowRectEx(&captureRect, windowStyle, TRUE, 0u);
        windowWidth = captureRect.right - captureRect.left;
        windowHeight = captureRect.bottom - captureRect.top;
    }
    HWND hWnd = CreateWindowExA(
        (gLookoutHiddenValidation || gRouteHiddenValidation) ? WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW :
            (gShowcaseCaptureHidden ? WS_EX_NOACTIVATE : 0),
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
    if(!SetPropW(hWnd,kCompatibilityProperty,&compatibility)){DestroyWindow(hWnd);return 1;}
    struct CompatibilityBinding {
        HWND window;CompatibilityWindowOwner* owner;
        ~CompatibilityBinding(){if(IsWindow(window) && CompatibilityFor(window)==owner)RemovePropW(window,kCompatibilityProperty);}
    } compatibilityBinding{hWnd,&compatibility};
#if defined(_DEBUG)
    if((gLookoutHiddenValidation || gRouteHiddenValidation) && !ObserveHiddenLookoutForeground(hWnd,"owner-created")) {
        DestroyWindow(hWnd);return 1;
    }
#endif

    if (captureDirectory != nullptr)
    {
        SetPropA(hWnd, kCaptureModeProperty, reinterpret_cast<HANDLE>(1));
        RECT windowRect{};
        RECT actualClientRect{};
        GetWindowRect(hWnd, &windowRect);
        GetClientRect(hWnd, &actualClientRect);
        const int adjustedWidth = (windowRect.right - windowRect.left) +
                                  static_cast<int>(captureWidth) - (actualClientRect.right - actualClientRect.left);
        const int adjustedHeight = (windowRect.bottom - windowRect.top) +
                                   static_cast<int>(captureHeight) - (actualClientRect.bottom - actualClientRect.top);
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
        compatibility.failureCleanup=true;DestroyWindow(hWnd);return 1;
    }

    auto createStatic = [&](const int id, const char* text, const DWORD style = SS_CENTER) {
        HWND control = CreateWindowExA(0, "STATIC", text, WS_CHILD | WS_VISIBLE | style,
                                       0, 0, 100, 30, hWnd,
                                       reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
        return control;
    };
    auto createButton = [&](const int id, const char* text) {
        HWND control = CreateWindowExA(0, "BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                       0, 0, 100, 38, hWnd,
                                       reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
        InstallControllerFocusOutline(control);
        return control;
    };

    createStatic(kHudControlId, kHudStartingText, SS_LEFT | SS_CENTERIMAGE | SS_NOTIFY);
    createStatic(kVitalityHudControlId, "Vitality 3 of 3", SS_OWNERDRAW);
    if (HWND prompt = createStatic(kCombatTeachingPromptId, "", SS_OWNERDRAW))
    {
        SetWindowLongPtrA(prompt, GWL_EXSTYLE,
            GetWindowLongPtrA(prompt, GWL_EXSTYLE) | WS_EX_LAYERED);
        SetLayeredWindowAttributes(prompt, 0u, 255u, LWA_ALPHA);
        ShowWindow(prompt, SW_HIDE);
    }
    if (HWND prompt = createStatic(kChestPromptControlId, "", SS_CENTER | SS_CENTERIMAGE))
    {
        ShowWindow(prompt, SW_HIDE);
    }
#if defined(_DEBUG)
    if (HWND developerOverlay = createStatic(kDeveloperOverlayId, "DEV OVERLAY STARTING...", SS_LEFT | SS_NOPREFIX))
    {
        EnableWindow(developerOverlay, FALSE);
        ShowWindow(developerOverlay, SW_HIDE);
    }
#endif
    createStatic(kPauseTitleId, "HORDE LANTERN RT  |  SHOWCASE ALPHA", SS_CENTER | SS_CENTERIMAGE);
    if (HWND spinner = createStatic(kEntryLoadingIndicatorId, "Loading", SS_OWNERDRAW))
        ShowWindow(spinner, SW_HIDE);
    createStatic(kEndingBodyId,
                 "The old guard bound the lich beneath this ruin and left one lantern to guide whoever came after.\r\n\r\n"
                 "Its flame died when the final seal opened. Now the staff is silent, the roof gives way, and stolen morning returns to the halls.",
                 SS_CENTER | SS_NOPREFIX);
    createButton(kEntryMoreButtonId, "MORE");
    createButton(kEntryBackButtonId, "BACK");
    createButton(kResumeButtonId, "ENTER THE RUIN / RESUME");
    createButton(kLookoutObjectiveButtonId,"LOOKOUT OBJECTIVE");
    createButton(kRestartButtonId, "RESTART ROUTE");
    createButton(kCampaignSaveButtonId,"SAVE CHECKPOINT...");
    createButton(kCampaignLoadButtonId,"LOAD...");
    createButton(kControlsButtonId, "CONTROLS");
    createButton(kSettingsButtonId, "SETTINGS");
    createButton(kReportProblemButtonId, "REPORT A PROBLEM...");
    createButton(kRtLabButtonId, "RT LAB");
    createButton(kDiagnosticsButtonId, "RT DIAGNOSTICS");
    createButton(kRunBenchmarkButtonId, "BENCHMARK...");
    createButton(kBenchmarkStandardChoiceId,"WINDOWS STANDARD - PROVISIONAL");
    createButton(kBenchmarkCustomChoiceId,"CUSTOM - UNRANKED");
    createButton(kBenchmarkChoiceRunId,"RUN SELECTED BENCHMARK");
    createButton(kBenchmarkChoiceBackId,"BACK");
    createStatic(kBenchmarkChoiceInfoId,"",SS_LEFT);
    createButton(kMoreBySamfa12ButtonId, "MORE BY SAMFA12");
    createButton(kExitButtonId, "QUIT DEMO");
    createStatic(kBenchmarkTitleId, "BENCHMARK REPORT  |  SELECTABLE TEXT", SS_CENTER | SS_CENTERIMAGE);
    createButton(kBenchmarkCopyButtonId, "COPY REPORT");
    createButton(kBenchmarkSaveButtonId, "SAVE AS...");
    createButton(kBenchmarkReviewStatsButtonId, "REVIEW STATS...");
    createButton(kBenchmarkBackButtonId, "BACK TO MENU");
    createStatic(kSettingsTitleId, "SETTINGS  |  SAVED BESIDE THE DEMO", SS_CENTER | SS_CENTERIMAGE);
    createButton(kGraphicsOpenButtonId, "GRAPHICS...");
    createButton(kGraphicsPresetButtonId, "PRESET: ACCEPTED BASELINE");
    createButton(kGraphicsFireButtonId, "FIRE DETAIL: HIGH");
    createButton(kGraphicsShadowButtonId, "SHADOW: CURRENT");
    createButton(kGraphicsGlassButtonId, "GLASS: ON");
    createButton(kGraphicsMistButtonId, "MIST: ON");
    createButton(kGraphicsDustButtonId, "INDOOR DUST: OFF");
    createButton(kGraphicsApplyButtonId, "APPLY");
    createButton(kGraphicsConfirmButtonId, "KEEP (15 SECONDS)");
    createButton(kGraphicsRevertButtonId, "REVERT");
    createButton(kGraphicsResetButtonId, "DEFAULTS (DRAFT)");
    createStatic(kGraphicsInfoId, "Cost: not yet measured for this candidate.", SS_LEFT);
    createButton(kGraphicsPreviewPauseId, "PAUSE PREVIEW");
    createButton(kGraphicsPreviewCameraId, "VIEW: OVERVIEW");
    createButton(kGraphicsPreviewMotionId, "MOTION TEST: OFF");
    createButton(kGraphicsPreviewResetId, "RESET PREVIEW TIMELINE");
    createStatic(kGraphicsPreviewTelemetryId, "Preview scene performance | loading production RT scene...", SS_LEFT);
    createStatic(kGraphicsPreviewGraphId, "Loop interval history", SS_OWNERDRAW);
    createButton(kSensitivityButtonId, "LOOK SENSITIVITY: NORMAL");
    createButton(kWaterQualityButtonId, "RT WATER: HIGH");
    createStatic(kRenderScaleLabelId, "Resolution: 100%", SS_CENTER | SS_CENTERIMAGE);
    HWND renderScaleSlider = CreateWindowExA(0, TRACKBAR_CLASSA, "",
                                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_AUTOTICKS,
                                              0, 0, 100, 38, hWnd,
                                              reinterpret_cast<HMENU>(static_cast<INT_PTR>(kRenderScaleSliderId)), instance, nullptr);
    InstallControllerFocusOutline(renderScaleSlider);
    SendMessageA(renderScaleSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 52));
    SendMessageA(renderScaleSlider, TBM_SETTICFREQ, 10, 0);
    SendMessageA(renderScaleSlider, TBM_SETPOS, TRUE, 52);
    createStatic(kSfxVolumeLabelId, "SFX VOLUME: 100%", SS_CENTER | SS_CENTERIMAGE);
    HWND sfxVolumeSlider = CreateWindowExA(0, TRACKBAR_CLASSA, "",
                                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_AUTOTICKS,
                                            0, 0, 100, 38, hWnd,
                                            reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSfxVolumeSliderId)), instance, nullptr);
    InstallControllerFocusOutline(sfxVolumeSlider);
    SendMessageA(sfxVolumeSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendMessageA(sfxVolumeSlider, TBM_SETTICFREQ, 10, 0);
    SendMessageA(sfxVolumeSlider, TBM_SETPOS, TRUE, 100);
    createStatic(kMusicVolumeLabelId, "MUSIC VOLUME: 70%", SS_CENTER | SS_CENTERIMAGE);
    HWND musicVolumeSlider = CreateWindowExA(0, TRACKBAR_CLASSA, "",
                                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_AUTOTICKS,
                                              0, 0, 100, 38, hWnd,
                                              reinterpret_cast<HMENU>(static_cast<INT_PTR>(kMusicVolumeSliderId)), instance, nullptr);
    InstallControllerFocusOutline(musicVolumeSlider);
    SendMessageA(musicVolumeSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendMessageA(musicVolumeSlider, TBM_SETTICFREQ, 10, 0);
    SendMessageA(musicVolumeSlider, TBM_SETPOS, TRUE, 70);
    createButton(kSettingsMoreId, "AUDIO & SUBTITLES...");
    createStatic(kVoiceVolumeLabelId, "DIALOGUE / SPEECH: 70%", SS_CENTER | SS_CENTERIMAGE);
    HWND voiceVolumeSlider = CreateWindowExA(0, TRACKBAR_CLASSA, "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_AUTOTICKS, 0, 0, 100, 38, hWnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kVoiceVolumeSliderId)), instance, nullptr);
    InstallControllerFocusOutline(voiceVolumeSlider);
    SendMessageA(voiceVolumeSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendMessageA(voiceVolumeSlider, TBM_SETTICFREQ, 10, 0);
    SendMessageA(voiceVolumeSlider, TBM_SETPOS, TRUE, 70);
    createButton(kSubtitlesButtonId, "SUBTITLES: ON");
    createStatic(kSubtitleSizeLabelId, "SUBTITLE TEXT SIZE: 100%", SS_CENTER | SS_CENTERIMAGE);
    HWND subtitleSizeSlider = CreateWindowExA(0, TRACKBAR_CLASSA, "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_AUTOTICKS, 0, 0, 100, 38, hWnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSubtitleSizeSliderId)), instance, nullptr);
    InstallControllerFocusOutline(subtitleSizeSlider);
    SendMessageA(subtitleSizeSlider, TBM_SETRANGE, TRUE, MAKELPARAM(80, 160));
    SendMessageA(subtitleSizeSlider, TBM_SETTICFREQ, 10, 0);
    SendMessageA(subtitleSizeSlider, TBM_SETPOS, TRUE, 100);
    createButton(kSubtitlePositionButtonId, "SUBTITLE POSITION: AUTO");
    if (HWND subtitle = createStatic(kChapterSubtitleId, "", SS_OWNERDRAW | SS_NOTIFY))
        SetWindowLongPtrA(subtitle, GWL_EXSTYLE,
            GetWindowLongPtrA(subtitle, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetControlVisible(hWnd, kChapterSubtitleId, false);
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
    const bool sceneMode = horde::vulkan::raytracing::SelectRtExecutionBackend(capabilities, true) !=
        horde::vulkan::RtExecutionBackend::Unsupported;
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
    int showMode=gShowcaseCaptureHidden ? SW_HIDE :
        (captureDirectory != nullptr ? SW_SHOWNOACTIVATE : SW_SHOW);
    if(gOpeningBenchmarkMenuValidation)showMode=SW_SHOWNOACTIVATE;
#if defined(_DEBUG)
    if (!gShowcaseCaptureHidden && !nativeMotionScenario.empty()) showMode=SW_SHOWNORMAL;
#endif
    if(!gCampaignHiddenValidation)ShowWindow(hWnd,showMode);
    // Only the explicitly owner-authorized foreground comparison may raise the
    // window. Background measurements fail closed if another app covers it.
    if(!gCampaignHiddenValidation && !gShowcaseCaptureHidden && ParseCaptureLaunchOptions().measureChapterForeground)
        SetWindowPos(hWnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    if(!gCampaignHiddenValidation && !gShowcaseCaptureHidden && ParseCaptureLaunchOptions().measureChapterForeground) {
        SetForegroundWindow(hWnd);
        FocusGameControl(hWnd);
    }
    UpdateWindow(hWnd);
    if (captureDirectory != nullptr)
    {
        RECT windowRect{};
        RECT actualClientRect{};
        GetWindowRect(hWnd, &windowRect);
        GetClientRect(hWnd, &actualClientRect);
        SetWindowPos(hWnd, nullptr, 0, 0,
                     (windowRect.right - windowRect.left) + static_cast<int>(captureWidth) -
                         (actualClientRect.right - actualClientRect.left),
                     (windowRect.bottom - windowRect.top) + static_cast<int>(captureHeight) -
                         (actualClientRect.bottom - actualClientRect.top),
                     SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    }
    if (captureDirectory == nullptr && !gCampaignHiddenValidation && !gOpeningBenchmarkMenuValidation && !gShowcaseCaptureHidden)
    {
        SetForegroundWindow(hWnd);
        FocusGameControl(sceneMode ? hWnd : edit);
    }
    if(gOpeningBenchmarkMenuValidation)TraceOpeningMenuStage("isolated-window no-activate");

    OpeningBenchmarkTransfer openingBenchmark;
    openingBenchmark.reports=textReportPath.parent_path();
    const bool openingEligible=captureDirectory==nullptr && developmentCheckpoint==nullptr &&
        !unattendedBenchmark && !graphicsPreviewCapture && !outputResizeValidation && nativeMotionScenario.empty() && !gCampaignHiddenValidation;
    if(openingEligible && !SetPropA(hWnd,kOpeningBenchmarkTransferProperty,&openingBenchmark)) {
        DestroyWindow(hWnd);return 1;
    }
    int result=0;
    do {
        const bool menuRun=openingEligible && openingBenchmark.owner.Phase()==OpeningBenchmarkOwnerPhase::Benchmark;
        result = RunDiagnosticSwapchainWindow(
            hWnd, capabilities, textReportPath, jsonReportPath, captureDirectory,
            developmentCheckpoint, requireRayQueryCompute, unattendedBenchmark,
            menuRun?horde::gameplay::BenchmarkWorkload::ShowcaseRoute:benchmarkWorkload,
            menuRun?horde::vulkan::raytracing::RtWorkloadPreset::Authored:benchmarkRtWorkloadPreset,
            anatomicalPlayerMount, graphicsPreviewCapture, outputResizeValidation, nativeMotionScenario, nativeMotionRtWorkloadPreset,
            captureDustQuality,openingEligible?&openingBenchmark:nullptr);
        if(!openingEligible || result!=0 || !IsWindow(hWnd))break;
    } while(openingBenchmark.owner.Phase()==OpeningBenchmarkOwnerPhase::Benchmark ||
            openingBenchmark.owner.Phase()==OpeningBenchmarkOwnerPhase::ReturningEntry);
    if(openingEligible && IsWindow(hWnd))RemovePropA(hWnd,kOpeningBenchmarkTransferProperty);
    if(result!=0 && IsWindow(hWnd)){compatibility.failureCleanup=true;DestroyWindow(hWnd);}
    if ((gCampaignHiddenValidation || captureDirectory != nullptr || unattendedBenchmark) && IsWindow(hWnd))
    {
#if defined(_DEBUG)
        if((gLookoutHiddenValidation || gRouteHiddenValidation) && !ObserveHiddenLookoutForeground(hWnd,"before-owner-destroy"))result=1;
#endif
        DestroyWindow(hWnd);
    }
#if defined(_DEBUG)
    if(gLookoutHiddenValidation)result=FinishHiddenLookoutValidation(hWnd,result);
    if(gRouteHiddenValidation)result=FinishHiddenRouteValidation(hWnd,result);
#endif
    return result;
}

} // namespace

namespace horde::platform::windows
{

int RunDiagnosticWindow(const int showCommand)
{
    (void)showCommand;
    SetProcessDPIAware();

    // This runs before any settings read, known-folder lookup or Vulkan startup.
    int acceptanceArgc=0;
    auto acceptanceArgv=CommandLineToArgvW(GetCommandLineW(),&acceptanceArgc);
    if(!acceptanceArgv)return 2;
    std::vector<std::wstring_view> acceptanceArgs;
    for(int i=1;i<acceptanceArgc;++i)acceptanceArgs.emplace_back(acceptanceArgv[i]);
    std::filesystem::path acceptanceReports;
#if HORDE_RT_WINDOWS_ACCEPTANCE
    acceptanceReports=std::filesystem::path(HORDE_RT_SOURCE_DIR)/"reports";
#endif
    const bool acceptanceReady=gAcceptanceProfile.Configure(acceptanceArgs,HORDE_RT_WINDOWS_ACCEPTANCE!=0,
        acceptanceReports,ExecutableDirectory());
    LocalFree(acceptanceArgv);
    if(!acceptanceReady){std::cerr<<gAcceptanceProfile.Error()<<'\n';return 2;}

    if(!ConfigureHiddenCampaignValidation())return 2;
    const CaptureLaunchOptions launchOptions = ParseCaptureLaunchOptions();
    if(gOpeningBenchmarkMenuValidation && (launchOptions.requested || launchOptions.benchmark.requested ||
        launchOptions.graphicsPreview || launchOptions.outputResizeValidation || !launchOptions.nativeMotionScenario.empty())) {
        std::cerr<<"Opening-menu validation cannot combine capture or standalone benchmark modes.\n";return 2;
    }
    if(gCampaignHiddenValidation && (launchOptions.requested || launchOptions.benchmark.requested)){std::cerr<<"Hidden campaign validation cannot combine with capture or benchmark modes.\n";return 2;}
    if (!launchOptions.error.empty())
    {
        std::cerr << launchOptions.error << '\n';
        return 2;
    }
    gShowcaseCaptureHidden=launchOptions.captureHidden;
#if !defined(_DEBUG)
    if (launchOptions.requested && !(HORDE_RT_CHAPTER_MEASUREMENT && launchOptions.measureChapterShippingRoute))
    {
        std::cerr << "Scene capture modes are Debug-only automation; Release builds reject them.\n";
        return 2;
    }
#endif

    if (launchOptions.environmentSurvey.requested || launchOptions.graphicsPreview || launchOptions.outputResizeValidation || !launchOptions.nativeMotionScenario.empty())
    {
        std::error_code capturePathError;
        const bool exists = std::filesystem::exists(launchOptions.outputDirectory, capturePathError);
        if (capturePathError || (exists &&
            (!std::filesystem::is_directory(launchOptions.outputDirectory, capturePathError) ||
             !std::filesystem::is_empty(launchOptions.outputDirectory, capturePathError))) || capturePathError)
        {
            std::cerr << "Isolated RT validation requires a new or empty absolute output directory.\n";
            return 2;
        }
    }

    horde::vulkan::VulkanContext context;
    const bool initialised = context.InitialiseForCapabilityProbe();
    horde::vulkan::DeviceCapabilities capabilities = context.QueryDeviceCapabilities();
    auto compatibility=CaptureWindowsCompatibilityProbe(context,initialised,capabilities);
    const bool reportInteractive=!launchOptions.requested && !launchOptions.benchmark.requested && !launchOptions.graphicsPreview &&
        !launchOptions.outputResizeValidation && launchOptions.nativeMotionScenario.empty() && !launchOptions.environmentSurvey.requested &&
        !gCampaignHiddenValidation && !gOpeningBenchmarkMenuValidation && !gShowcaseCaptureHidden;
    const auto failure=[&](horde::reporting::CompatibilityStage stage){
        compatibility.stage=stage;CompatibilityFailure(compatibility);
        if(reportInteractive)ShowWindowsCompatibilityReport(nullptr,compatibility.snapshot,{},true);
        return 1;
    };

    const std::string diagnosticText = BuildDisplayText(capabilities);
    const std::string textReport = horde::vulkan::BuildCapabilityTextReport(capabilities);
    const std::string jsonReport = horde::vulkan::BuildCapabilityJsonReport(capabilities);

    std::cout << "=== Horde RT Diagnostic Window ===\n";
    std::cout << "Probe initialisation: " << (initialised ? "OK" : "Fallback") << "\n\n";
    std::cout << diagnosticText << "\n\n";

    std::error_code error;
    const std::filesystem::path reportDirectory = gCampaignHiddenValidation?gCampaignValidationDirectory:(launchOptions.environmentSurvey.requested || launchOptions.graphicsPreview || launchOptions.outputResizeValidation || !launchOptions.nativeMotionScenario.empty())
        ? launchOptions.outputDirectory
        : launchOptions.benchmark.requested
        ? std::filesystem::absolute(std::filesystem::path(launchOptions.benchmark.outputDirectory))
        : ExecutableDirectory() / kReportDirectory;
    std::filesystem::create_directories(reportDirectory, error);
    if (error)
    {
        std::cerr << "Failed to create report directory '" << kReportDirectory << "': " << error.message() << '\n';
        return failure(horde::reporting::CompatibilityStage::DiagnosticStorage);
    }

    const std::filesystem::path textReportPath = reportDirectory / kTextReportFilename;
    const std::filesystem::path jsonReportPath = reportDirectory / kJsonReportFilename;

    if (!WriteReportFile(textReportPath, textReport))
    {
        std::cerr << "Failed to write text report to " << textReportPath << '\n';
        return failure(horde::reporting::CompatibilityStage::DiagnosticStorage);
    }

    if (!WriteReportFile(jsonReportPath, jsonReport))
    {
        std::cerr << "Failed to write JSON report to " << jsonReportPath << '\n';
        return failure(horde::reporting::CompatibilityStage::DiagnosticStorage);
    }

    std::cout << "Stored report (text): " << textReportPath << '\n';
    std::cout << "Stored report (json): " << jsonReportPath << '\n';

    const std::filesystem::path* captureDirectory = launchOptions.requested
        ? &launchOptions.outputDirectory
        : nullptr;
    const std::string* developmentCheckpoint = launchOptions.developmentCheckpoint.empty()
        ? nullptr
        : &launchOptions.developmentCheckpoint;
    const auto result=CreateAndShowWindow(diagnosticText, capabilities, compatibility, textReportPath, jsonReportPath,
                               captureDirectory, developmentCheckpoint, launchOptions.portrait,
                               launchOptions.requireRayQueryCompute,
                               launchOptions.benchmark.requested, launchOptions.benchmark.workload,
                               launchOptions.benchmark.rtWorkloadPreset,
                               launchOptions.anatomicalPlayerMount, launchOptions.graphicsPreview, launchOptions.outputResizeValidation,
                               launchOptions.nativeMotionScenario, launchOptions.nativeMotionRtWorkloadPreset,
                               launchOptions.captureDustQuality);
    if(result!=0 && reportInteractive){CompatibilityFailure(compatibility);ShowWindowsCompatibilityReport(nullptr,compatibility.snapshot,{},true);}
    return result;
}

} // namespace horde::platform::windows
