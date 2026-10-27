#pragma once

#include "reporting/BenchmarkSummaryReport.h"
#include "reporting/CompatibilityReport.h"

namespace horde::reporting
{
inline constexpr std::size_t kDeveloperReportSubmissionMaxBytes = 20u * 1024u;
enum class DeveloperReportKind : std::uint8_t { Benchmark, Compatibility };
enum class DeveloperReportSubmissionStatus : std::uint8_t { Ready, ConsentRequired, ReportRejected, TooLarge };
class PreparedDeveloperReportSubmission;
[[nodiscard]] PreparedDeveloperReportSubmission PreparePrivateDeveloperReport(
    const PreparedBenchmarkSummaryReport&, bool explicitPrivateSendConsent = false);
[[nodiscard]] PreparedDeveloperReportSubmission PreparePrivateDeveloperReport(
    const PreparedCompatibilityReport&, bool explicitPrivateSendConsent = false);

// Only typed local reports can enter. This API cannot authorize public posting.
// Deployment admission remains a separate platform transport gate.
class PreparedDeveloperReportSubmission
{
public:
    [[nodiscard]] bool IsReady() const noexcept
    { return status_ == DeveloperReportSubmissionStatus::Ready && !localJson_.empty() && !json_.empty(); }
    [[nodiscard]] DeveloperReportSubmissionStatus Status() const noexcept { return status_; }
    [[nodiscard]] DeveloperReportKind Kind() const noexcept { return kind_; }
    [[nodiscard]] std::string_view ReportId() const noexcept { return id_; }
    [[nodiscard]] std::string_view LocalJson() const noexcept { return localJson_; }
    [[nodiscard]] std::string_view Json() const noexcept { return json_; }
private:
    friend PreparedDeveloperReportSubmission PreparePrivateDeveloperReport(const PreparedBenchmarkSummaryReport&, bool);
    friend PreparedDeveloperReportSubmission PreparePrivateDeveloperReport(const PreparedCompatibilityReport&, bool);
    static PreparedDeveloperReportSubmission Prepare(bool ready, std::string_view id,
        std::string_view local, DeveloperReportKind kind, bool consent);
    DeveloperReportSubmissionStatus status_ = DeveloperReportSubmissionStatus::ReportRejected;
    DeveloperReportKind kind_ = DeveloperReportKind::Compatibility;
    std::string id_, localJson_, json_;
};
// Token exists only in an attempt, never in local export or frozen approval.
[[nodiscard]] std::string BuildDeveloperReportRequest(
    const PreparedDeveloperReportSubmission&, std::string_view freshVerificationToken);
} // namespace horde::reporting
