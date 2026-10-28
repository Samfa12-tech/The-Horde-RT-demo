#include "reporting/DeveloperReportSubmission.h"
#include "reporting/PlaytestSubmission.h"

namespace horde::reporting
{
PreparedDeveloperReportSubmission PreparedDeveloperReportSubmission::Prepare(const bool ready,
    const std::string_view id, const std::string_view local, const DeveloperReportKind kind, const bool consent)
{
    PreparedDeveloperReportSubmission result;
    if (!consent) { result.status_ = DeveloperReportSubmissionStatus::ConsentRequired; return result; }
    if (!ready || local.size() < 2u || local.front() != '{' || local.back() != '}') return result;
    constexpr std::string_view suffix =
        ",\"consentToSubmit\":true,\"delivery\":{\"destination\":\"developer-private\",\"publishToBoard\":false}}";
    if (local.size() - 1u + suffix.size() > kDeveloperReportSubmissionMaxBytes)
    { result.status_ = DeveloperReportSubmissionStatus::TooLarge; return result; }
    result.id_ = id; result.localJson_ = local; result.kind_ = kind;
    result.json_.assign(local.substr(0u, local.size() - 1u)); result.json_.append(suffix);
    result.status_ = DeveloperReportSubmissionStatus::Ready;
    return result;
}
PreparedDeveloperReportSubmission PreparePrivateDeveloperReport(
    const PreparedBenchmarkSummaryReport& report, const bool consent)
{ return PreparedDeveloperReportSubmission::Prepare(report.IsReady(), report.ReportId(), report.Json(), DeveloperReportKind::Benchmark, consent); }
PreparedDeveloperReportSubmission PreparePrivateDeveloperReport(
    const PreparedCompatibilityReport& report, const bool consent)
{ return PreparedDeveloperReportSubmission::Prepare(report.IsReady(), report.ReportId(), report.Json(), DeveloperReportKind::Compatibility, consent); }

std::string BuildDeveloperReportRequest(const PreparedDeveloperReportSubmission& report, const std::string_view token)
{
    if (!report.IsReady()) return {};
    // Reuse the existing strict printable token escaping, without accepting
    // arbitrary JSON through the public typed API.
    PreparedPlaytestSubmission bridge;
    bridge.status = PlaytestSubmissionStatus::Ready;
    bridge.reportId = report.ReportId(); bridge.json = report.Json();
    auto request = BuildPlaytestSubmissionRequest(bridge, token);
    if (request.size() > kDeveloperReportSubmissionMaxBytes) return {};
    return request;
}

PreparedPublicBenchmarkSubmission PreparePublicBenchmarkSubmission(
    const PreparedPublicBenchmarkReport& local, const bool consent)
{
    PreparedPublicBenchmarkSubmission result;
    if (!consent) { result.status_ = DeveloperReportSubmissionStatus::ConsentRequired; return result; }
    if (!local.IsReady()) return result;
    constexpr std::string_view suffix = ",\"consentToPublish\":true}";
    if (local.Json().size() - 1u + suffix.size() > kDeveloperReportSubmissionMaxBytes)
    { result.status_ = DeveloperReportSubmissionStatus::TooLarge; return result; }
    result.id_ = local.ResultId(); result.localJson_ = local.Json();
    result.json_.assign(local.Json().substr(0u, local.Json().size() - 1u)); result.json_.append(suffix);
    result.status_ = DeveloperReportSubmissionStatus::Ready;
    return result;
}

std::string BuildPublicBenchmarkRequest(const PreparedPublicBenchmarkSubmission& report, const std::string_view token)
{
    if (!report.IsReady()) return {};
    PreparedPlaytestSubmission bridge;
    bridge.status = PlaytestSubmissionStatus::Ready;
    bridge.reportId = report.ResultId(); bridge.json = report.Json();
    auto request = BuildPlaytestSubmissionRequest(bridge, token);
    if (request.size() > kDeveloperReportSubmissionMaxBytes) return {};
    return request;
}
} // namespace horde::reporting
