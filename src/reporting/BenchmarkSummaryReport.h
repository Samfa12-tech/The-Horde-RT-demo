#pragma once

#include "telemetry/BenchmarkSummary.h"

namespace horde::reporting
{
inline constexpr std::size_t kBenchmarkSummaryReportMaxBytes = 16u * 1024u;
inline constexpr std::string_view kBenchmarkSummaryReportKind = "benchmark-summary";
enum class BenchmarkSummaryReportStatus : std::uint8_t
{
    Ready, ConsentRequired, InvalidSummary, InvalidIdentity, InvalidTimestamp, InvalidLabel, TooLarge,
};
enum class BenchmarkSummaryContentMatch : std::uint8_t { Invalid, DifferentId, SameContent, Conflict };
struct BenchmarkSummaryReportApproval
{
    // LOCAL export/review consent. Does not authorize a remote send or imply that
    // the existing schema1 endpoint/Java parser admits this new local schema2 kind.
    bool consentToPrepare = false;
    bool includeBasicHardware = false;
    std::string_view reportUuid;
    std::string_view capturedAtUtc;
};
class PreparedBenchmarkSummaryReport;
[[nodiscard]] PreparedBenchmarkSummaryReport PrepareBenchmarkSummaryReport(
    const horde::telemetry::FrozenBenchmarkSummary&, const BenchmarkSummaryReportApproval&);

class PreparedBenchmarkSummaryReport
{
public:
    [[nodiscard]] bool IsReady() const noexcept
    { return status_ == BenchmarkSummaryReportStatus::Ready && reportUuid_.size() == 36u && !json_.empty(); }
    [[nodiscard]] BenchmarkSummaryReportStatus Status() const noexcept { return status_; }
    [[nodiscard]] std::string_view ReportId() const noexcept { return reportUuid_; }
    // Exact reviewed, bounded token-free bytes, shared by local preview/export/retry.
    [[nodiscard]] std::string_view Json() const noexcept { return json_; }
private:
    friend PreparedBenchmarkSummaryReport PrepareBenchmarkSummaryReport(
        const horde::telemetry::FrozenBenchmarkSummary&, const BenchmarkSummaryReportApproval&);
    BenchmarkSummaryReportStatus status_ = BenchmarkSummaryReportStatus::InvalidSummary;
    std::string reportUuid_;
    std::string json_;
};

// Local exact-byte identity check, not a server storage/delivery guarantee.
[[nodiscard]] BenchmarkSummaryContentMatch CompareBenchmarkSummaryContent(
    const PreparedBenchmarkSummaryReport&, const PreparedBenchmarkSummaryReport&) noexcept;
} // namespace horde::reporting
