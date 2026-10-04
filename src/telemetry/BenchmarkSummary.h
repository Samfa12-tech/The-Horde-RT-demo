#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

#include "gameplay/ShowcaseBenchmark.h"
#include "telemetry/RtBenchmarkEvidenceRun.h"

namespace horde::telemetry
{
enum class BenchmarkSummaryPlatform : std::uint8_t { Windows, Android };
enum class BenchmarkSummaryCooling : std::uint8_t { Unknown, NoneDeclared, ExternalDeclared };
enum class BenchmarkSummaryFireQuality : std::uint8_t { Mobile, High, Low };
enum class BenchmarkSummaryStatus : std::uint8_t
{
    Ready, InvalidIdentity, InvalidConfiguration, Incomplete, MismatchedPopulation, InvalidTiming,
};

// Copied on the render/application owner at measurement arming and completion.
// Both copies must describe the same measured configuration/scope. Hardware labels
// are deliberately separate and are admitted only by the reporting consent layer.
struct BenchmarkSummaryConfiguration
{
    BenchmarkSummaryPlatform platform = BenchmarkSummaryPlatform::Windows;
    horde::gameplay::ShowcaseBenchmarkMetadata metadata{};
    RtWaterQuality water = RtWaterQuality::Mobile;
    RtDielectricQuality dielectric = RtDielectricQuality::Mobile;
    BenchmarkSummaryFireQuality fire = BenchmarkSummaryFireQuality::Mobile;
    bool glassEnabled = true;
    std::uint64_t sceneEpoch = 0u;
    std::uint64_t measurementGeneration = 0u;
    std::optional<RtShadowQualityEvidence> shadowQuality{};
    std::optional<RtFireQualityEvidence> uploadedFireQuality{};
};

struct BenchmarkSummaryPopulation
{
    std::size_t frames = 0u;
    horde::gameplay::ShowcaseBenchmarkStatistics legacy{};
    std::array<RtStageStatistics, kRtStageCount> cpu{};
    RtStageStatistics gpu{};
    RtGpuSampleStatusCounts gpuStatuses{};
};

struct BenchmarkSummaryZone
{
    horde::gameplay::ShowcaseZone zone = horde::gameplay::ShowcaseZone::Opening;
    BenchmarkSummaryPopulation population{};
};

inline constexpr std::size_t kBenchmarkSummaryZoneCount = 10u;
struct BenchmarkSummaryData
{
    std::string runUuid;
    BenchmarkSummaryConfiguration configuration{};
    std::string rawModel;
    BenchmarkSummaryCooling cooling = BenchmarkSummaryCooling::Unknown;
    horde::gameplay::BenchmarkWorkload workload = horde::gameplay::BenchmarkWorkload::ShowcaseRoute;
    std::uint32_t laps = 0u;
    BenchmarkSummaryPopulation overall{};
    std::array<BenchmarkSummaryZone, kBenchmarkSummaryZoneCount> zones{};
};

class FrozenBenchmarkSummary;
[[nodiscard]] FrozenBenchmarkSummary CaptureBenchmarkSummary(
    const horde::gameplay::ShowcaseBenchmarkRun& benchmark, const RtBenchmarkEvidenceRun& evidence,
    const BenchmarkSummaryConfiguration& measurementStart, const BenchmarkSummaryConfiguration& completion,
    std::string_view runUuid, std::string_view rawModel = {},
    BenchmarkSummaryCooling cooling = BenchmarkSummaryCooling::Unknown);

// Immutable copied evidence. Capture BEFORE route reset; never read live owner
// pointers or mutable latest-report files on a GUI/JNI/report worker.
class FrozenBenchmarkSummary
{
public:
    [[nodiscard]] bool IsReady() const noexcept
    { return status_ == BenchmarkSummaryStatus::Ready && data_.runUuid.size() == 36u && data_.overall.frames != 0u; }
    [[nodiscard]] BenchmarkSummaryStatus Status() const noexcept { return status_; }
    [[nodiscard]] const BenchmarkSummaryData& Data() const noexcept { return data_; }
private:
    friend FrozenBenchmarkSummary CaptureBenchmarkSummary(
        const horde::gameplay::ShowcaseBenchmarkRun&, const RtBenchmarkEvidenceRun&,
        const BenchmarkSummaryConfiguration&, const BenchmarkSummaryConfiguration&,
        std::string_view, std::string_view, BenchmarkSummaryCooling);
    BenchmarkSummaryStatus status_ = BenchmarkSummaryStatus::Incomplete;
    BenchmarkSummaryData data_{};
};

// Syntax only: platform must supply CSPRNG UUIDv4. No device/user/timestamp IDs.
[[nodiscard]] bool IsBenchmarkSummaryUuid(std::string_view uuid) noexcept;
} // namespace horde::telemetry
