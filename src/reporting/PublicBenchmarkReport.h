#pragma once

#include "reporting/BenchmarkSummaryReport.h"

namespace horde::reporting
{
inline constexpr std::size_t kPublicBenchmarkReportMaxBytes = 16u * 1024u;
struct PublicBenchmarkReportApproval
{
    // Separate LOCAL public preview consent, never inherited from private sharing.
    // This does not authorize sending or publication. A future send owner must
    // request explicit publication consent after these exact bytes are reviewed.
    bool consentToPreparePublicPreview = false;
    bool includeBasicHardware = false;
    // Fresh CSPRNG UUIDv4 for the public result, not a private report/run/device ID.
    std::string_view resultUuid, capturedAtUtc;
    std::optional<horde::telemetry::BenchmarkSummaryCooling> declaredCooling;
};
class PreparedPublicBenchmarkReport;
[[nodiscard]] PreparedPublicBenchmarkReport PreparePublicBenchmarkReport(
    const horde::telemetry::FrozenBenchmarkSummary&, const PublicBenchmarkReportApproval&);

// Projection from typed frozen evidence only. No arbitrary/private JSON input,
// private submission conversion, transport, or remote consent is provided.
class PreparedPublicBenchmarkReport
{
public:
    [[nodiscard]] bool IsReady() const noexcept
    { return status_ == BenchmarkSummaryReportStatus::Ready && resultId_.size() == 36u && !json_.empty(); }
    [[nodiscard]] BenchmarkSummaryReportStatus Status() const noexcept { return status_; }
    [[nodiscard]] std::string_view ResultId() const noexcept { return resultId_; }
    [[nodiscard]] std::string_view Json() const noexcept { return json_; }
private:
    friend PreparedPublicBenchmarkReport PreparePublicBenchmarkReport(
        const horde::telemetry::FrozenBenchmarkSummary&, const PublicBenchmarkReportApproval&);
    BenchmarkSummaryReportStatus status_ = BenchmarkSummaryReportStatus::InvalidSummary;
    std::string resultId_, json_;
};
} // namespace horde::reporting
