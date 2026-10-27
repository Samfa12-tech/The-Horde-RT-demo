#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace horde::reporting
{
inline constexpr std::size_t kCompatibilityReportMaxBytes = 8u * 1024u;
inline constexpr std::size_t kCompatibilityReportMaxLabelBytes = 512u;
inline constexpr std::string_view kCompatibilityReportKind = "compatibility";

enum class CompatibilityPlatform : std::uint8_t { Windows, Android };
enum class CompatibilityCheck : std::uint8_t { Unknown, Absent, Present };
enum class CompatibilityProbe : std::uint8_t { NotAttempted, Unavailable, NoDevice, DeviceChecked };
enum class CompatibilityBackend : std::uint8_t { NotSelected, RayTracingPipeline, RayQueryCompute };
enum class CompatibilityStage : std::uint8_t
{
    None, CapabilityProbe, AssetPreparation, VulkanInstance, Surface,
    BackendSelection, LogicalDevice, Swapchain, RtScene, Presentation, RenderLoop, DiagnosticStorage, WindowCreation,
};
enum class CompatibilityOutcome : std::uint8_t
{
    NotAttempted, ProbeUnavailable, CapabilityOnly, Unsupported, StartupFailure, Presented, RuntimeFailure,
};

struct CompatibilityExtensions
{
    CompatibilityCheck accelerationStructure = CompatibilityCheck::Unknown;
    CompatibilityCheck rayTracingPipeline = CompatibilityCheck::Unknown;
    CompatibilityCheck rayQuery = CompatibilityCheck::Unknown;
    CompatibilityCheck bufferDeviceAddress = CompatibilityCheck::Unknown;
    CompatibilityCheck deferredHostOperations = CompatibilityCheck::Unknown;
};
struct CompatibilityFeatures
{
    CompatibilityCheck accelerationStructure = CompatibilityCheck::Unknown;
    CompatibilityCheck rayTracingPipeline = CompatibilityCheck::Unknown;
    CompatibilityCheck rayQuery = CompatibilityCheck::Unknown;
    CompatibilityCheck bufferDeviceAddress = CompatibilityCheck::Unknown;
};

// Copied by the platform startup owner, including failures before any renderer
// exists. No raw log, arbitrary diagnostic, device ID or borrowed scene pointer.
struct CompatibilityReportSnapshot
{
    std::string app = "Horde Lantern RT";
    std::string version, build;
    CompatibilityPlatform platform = CompatibilityPlatform::Windows;
    // Empty optional labels mean unavailable, not guessed hardware/OS evidence.
    std::string model, os, gpu;
    std::optional<std::uint32_t> rawDriverVersion, vulkanApiVersion;
    CompatibilityProbe probe = CompatibilityProbe::NotAttempted;
    CompatibilityExtensions extensions{};
    CompatibilityFeatures features{};
    CompatibilityStage lastStage = CompatibilityStage::None;
    CompatibilityOutcome outcome = CompatibilityOutcome::NotAttempted;
    CompatibilityBackend backend = CompatibilityBackend::NotSelected;
    bool rtPresented = false;
};

struct CompatibilityReportApproval
{
    // Local review/export only. No remote or public-board consent is implied.
    bool consentToPrepare = false;
    // Caller supplies a fresh CSPRNG UUIDv4 for this report, never a device ID.
    std::string_view reportUuid, capturedAtUtc;
};
enum class CompatibilityReportStatus : std::uint8_t
{
    Ready, ConsentRequired, InvalidIdentity, InvalidTimestamp, InvalidLabel,
    InvalidEvidence, TooLarge,
};

class PreparedCompatibilityReport;
[[nodiscard]] PreparedCompatibilityReport PrepareCompatibilityReport(
    const CompatibilityReportSnapshot&, const CompatibilityReportApproval&);

class PreparedCompatibilityReport
{
public:
    [[nodiscard]] bool IsReady() const noexcept
    { return status_ == CompatibilityReportStatus::Ready && reportId_.size() == 36u && !json_.empty(); }
    [[nodiscard]] CompatibilityReportStatus Status() const noexcept { return status_; }
    [[nodiscard]] std::string_view ReportId() const noexcept { return reportId_; }
    // Exact bounded, token-free approved bytes. Retry/export must retain these.
    [[nodiscard]] std::string_view Json() const noexcept { return json_; }
private:
    friend PreparedCompatibilityReport PrepareCompatibilityReport(
        const CompatibilityReportSnapshot&, const CompatibilityReportApproval&);
    CompatibilityReportStatus status_ = CompatibilityReportStatus::InvalidEvidence;
    std::string reportId_, json_;
};
} // namespace horde::reporting
