#include "reporting/BenchmarkSummarySubmission.h"

namespace horde::reporting
{
PreparedBenchmarkSummarySubmission PrepareBenchmarkSummarySubmission(
    const PreparedBenchmarkSummaryReport& report, const bool explicitRemoteConsent)
{
    PreparedBenchmarkSummarySubmission result;
    if (!explicitRemoteConsent)
    { result.status_ = BenchmarkSummarySubmissionStatus::ConsentRequired; return result; }
    if (!report.IsReady()) return result;
    const auto local = report.Json();
    if (local.size() > kBenchmarkSummaryReportMaxBytes)
    { result.status_ = BenchmarkSummarySubmissionStatus::TooLarge; return result; }
    if (local.size() < 2u || local.front() != '{' || local.back() != '}') return result;
    constexpr std::string_view consent = ",\"consentToSubmit\":true}";
    if (local.size() - 1u + consent.size() > kBenchmarkSummarySubmissionMaxBytes)
    { result.status_ = BenchmarkSummarySubmissionStatus::TooLarge; return result; }
    result.reportId_ = report.ReportId();
    result.localJson_ = local;
    result.json_.assign(local.substr(0u, local.size() - 1u));
    result.json_.append(consent);
    result.status_ = BenchmarkSummarySubmissionStatus::Ready;
    return result;
}

std::string BuildBenchmarkSummarySubmissionRequest(
    const PreparedBenchmarkSummarySubmission& submission, const std::string_view turnstileToken)
{
    if (!submission.IsReady() || turnstileToken.empty() ||
        turnstileToken.size() > kBenchmarkSummarySubmissionMaxTokenBytes) return {};
    std::string escaped;
    escaped.reserve(turnstileToken.size());
    for (const unsigned char byte : turnstileToken)
    {
        if (byte < 0x20u || byte > 0x7eu) return {};
        if (byte == '"' || byte == '\\') escaped += '\\';
        escaped += static_cast<char>(byte);
    }
    constexpr std::string_view prefix = ",\"turnstileToken\":\"";
    const auto frozen = submission.Json();
    if (frozen.size() - 1u + prefix.size() + escaped.size() + 2u >
        kBenchmarkSummarySubmissionMaxBytes) return {};
    std::string request(frozen.substr(0u, frozen.size() - 1u));
    request.append(prefix); request.append(escaped); request.append("\"}");
    return request;
}
} // namespace horde::reporting
