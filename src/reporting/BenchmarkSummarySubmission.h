#pragma once

#include "reporting/BenchmarkSummaryReport.h"

namespace horde::reporting
{
// Proposed bounded wire contract only. Preparation does not establish deployed
// endpoint admission or authorize transport; live benchmark Send is unavailable.
inline constexpr std::size_t kBenchmarkSummarySubmissionMaxBytes = 20u * 1024u;
inline constexpr std::size_t kBenchmarkSummarySubmissionMaxTokenBytes = 2048u;

enum class BenchmarkSummarySubmissionStatus : std::uint8_t
{
    Ready, ConsentRequired, ReportRejected, TooLarge,
};

class PreparedBenchmarkSummarySubmission;
[[nodiscard]] PreparedBenchmarkSummarySubmission PrepareBenchmarkSummarySubmission(
    const PreparedBenchmarkSummaryReport&, bool explicitRemoteConsent = false);

// A separately approved, immutable memory-only wrapper. No arbitrary JSON,
// screenshot, diagnostic dump or identity mutation can enter this API.
class PreparedBenchmarkSummarySubmission
{
public:
    [[nodiscard]] bool IsReady() const noexcept
    { return status_ == BenchmarkSummarySubmissionStatus::Ready && !localJson_.empty() && !json_.empty(); }
    [[nodiscard]] BenchmarkSummarySubmissionStatus Status() const noexcept { return status_; }
    [[nodiscard]] std::string_view ReportId() const noexcept { return reportId_; }
    [[nodiscard]] std::string_view LocalJson() const noexcept { return localJson_; }
    // Exact approved local bytes, with only a disclosed outer consentToSubmit
    // member added. Token-free; keep independent local Copy/Save bytes unchanged.
    [[nodiscard]] std::string_view Json() const noexcept { return json_; }
private:
    friend PreparedBenchmarkSummarySubmission PrepareBenchmarkSummarySubmission(
        const PreparedBenchmarkSummaryReport&, bool);
    BenchmarkSummarySubmissionStatus status_ = BenchmarkSummarySubmissionStatus::ReportRejected;
    std::string reportId_, localJson_, json_;
};

// Offline request construction for a reviewed future client. Each attempt needs
// fresh verification; neither token nor request replaces the frozen local report.
// Callers must not dispatch until exact compatible deployment is established.
[[nodiscard]] std::string BuildBenchmarkSummarySubmissionRequest(
    const PreparedBenchmarkSummarySubmission&, std::string_view turnstileToken);
} // namespace horde::reporting
