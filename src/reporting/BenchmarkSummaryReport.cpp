#include "reporting/BenchmarkSummaryReport.h"
#include "reporting/PlaytestReport.h"

#include <algorithm>
#include <iomanip>
#include <locale>
#include <sstream>
#include <utility>

namespace horde::reporting
{
namespace
{
using namespace horde::telemetry;
bool SafeLabel(const std::string_view value, const std::size_t cap = 128u)
{
    if (value.empty() || value.size() > cap) return false;
    // Reuse the existing strict text/privacy policy, not a second secret filter.
    const PlaytestReportInput validation{"11111111-1111-4111-8111-111111111111", "2026-10-03T00:00:00Z",
        PlaytestReportCategory::Performance, PlaytestReportImpact::Polish, value, true};
    return PreparePlaytestReport(validation).IsReady() &&
        std::none_of(value.begin(), value.end(), [](const char c) { return c == '\n' || c == '\r' || c == '\t'; });
}

bool ValidShaderPart(const std::string_view part)
{
    const auto at = part.find('@');
    if (at == std::string_view::npos || at == 0u || at >= 96u || part.size() - at - 1u != 64u) return false;
    const auto key = part.substr(0u, at);
    if (!SafeLabel(key, 95u) || !std::all_of(key.begin(), key.end(), [](const char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
    })) return false;
    return std::all_of(part.begin() + static_cast<std::ptrdiff_t>(at + 1u), part.end(), [](const char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}

bool ValidShaderPair(std::string_view value)
{
    // Admit the exact production selected-pair syntax; @ cannot admit emails.
    constexpr std::string_view first = "opaqueFast:", second = "|genericDielectric:";
    if (!value.starts_with(first)) return false;
    value.remove_prefix(first.size());
    const auto divider = value.find(second);
    return divider != std::string_view::npos && ValidShaderPart(value.substr(0u, divider)) &&
        ValidShaderPart(value.substr(divider + second.size()));
}

void Text(std::ostream& out, const std::string_view value)
{
    out << '"';
    for (const char c : value)
    {
        if (c == '"' || c == '\\') out << '\\';
        out << c;
    }
    out << '"';
}

void Statistics(std::ostream& out, const RtStageStatistics& s, const bool frameRate)
{
    if (!s.valid) { out << "null"; return; }
    out << "{\"sampleCount\":" << s.sampleCount << ",\"meanMs\":" << s.meanMilliseconds <<
        ",\"medianMs\":" << s.medianMilliseconds << ",\"p90Ms\":" << s.p90Milliseconds <<
        ",\"p95Ms\":" << s.p95Milliseconds << ",\"slowestOnePercentMeanMs\":" << s.slowestOnePercentMeanMilliseconds;
    // GPU duration and sub-stage cost do not claim a frame rate.
    if (frameRate) out << ",\"onePercentLowFps\":" << s.onePercentLowFps;
    out << '}';
}

void Population(std::ostream& out, const BenchmarkSummaryPopulation& p, const bool allCpuStages)
{
    out << "{\"measuredFrames\":" << p.frames << ",\"legacyPlatformInterval\":";
    if (p.frames == 0u) out << "null";
    else out << "{\"sampleCount\":" << p.frames << ",\"meanMs\":" << p.legacy.averageMs <<
        ",\"medianMs\":" << p.legacy.medianMs << ",\"p95Ms\":" << p.legacy.p95Ms <<
        ",\"onePercentLowFps\":" << p.legacy.onePercentLowFps << '}';
    out << ",\"cpuStages\":{";
    const std::size_t first = allCpuStages ? 0u : RtStageIndex(RtStage::WholeFrameCycle);
    for (std::size_t i = first; i < kRtStageCount; ++i)
    {
        if (i != first) out << ',';
        Text(out, RtStageMetricName(static_cast<RtStage>(i))); out << ':';
        Statistics(out, p.cpu[i], i == RtStageIndex(RtStage::WholeFrameCycle));
    }
    out << "},\"gpuRtDurationMs\":"; Statistics(out, p.gpu, false);
    out << ",\"gpuStatusCounts\":{\"denominator\":" << p.frames <<
        ",\"valid\":" << p.gpuStatuses.valid << ",\"disabled\":" << p.gpuStatuses.disabled <<
        ",\"unsupported\":" << p.gpuStatuses.unsupported << ",\"compiledOut\":" << p.gpuStatuses.compiledOut << "}}";
}
} // namespace

PreparedBenchmarkSummaryReport PrepareBenchmarkSummaryReport(const FrozenBenchmarkSummary& summary,
    const BenchmarkSummaryReportApproval& approval)
{
    PreparedBenchmarkSummaryReport result;
    if (!approval.consentToPrepare) { result.status_ = BenchmarkSummaryReportStatus::ConsentRequired; return result; }
    if (!summary.IsReady()) return result;
    if (!IsBenchmarkSummaryUuid(approval.reportUuid)) { result.status_ = BenchmarkSummaryReportStatus::InvalidIdentity; return result; }
    const PlaytestReportInput validation{approval.reportUuid, approval.capturedAtUtc,
        PlaytestReportCategory::Performance, PlaytestReportImpact::Polish, "Benchmark summary", true};
    if (!PreparePlaytestReport(validation).IsReady())
    { result.status_ = BenchmarkSummaryReportStatus::InvalidTimestamp; return result; }
    const auto& data = summary.Data();
    const auto cooling = approval.declaredCooling.value_or(data.cooling);
    if (cooling > BenchmarkSummaryCooling::ExternalDeclared)
    { result.status_ = BenchmarkSummaryReportStatus::InvalidCooling; return result; }
    const auto& c = data.configuration;
    const auto& m = c.metadata;
    if (!SafeLabel(m.buildIdentity) || !ValidShaderPair(m.shaderIdentity) || !SafeLabel(m.materialEncoding) ||
        (approval.includeBasicHardware && (!SafeLabel(data.rawModel) || !SafeLabel(m.gpuName) || !SafeLabel(m.vulkanApi))))
    { result.status_ = BenchmarkSummaryReportStatus::InvalidLabel; return result; }
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(8) << "{\"schemaVersion\":2,\"product\":\"horde-lantern-rt\",\"reportKind\":\"benchmark-summary\","
        "\"consentToPrepare\":true,\"includeBasicHardware\":" << (approval.includeBasicHardware ? "true" : "false") <<
        ",\"report\":{\"schemaVersion\":1,\"reportId\":";
    Text(out, approval.reportUuid); out << ",\"capturedAtUtc\":"; Text(out, approval.capturedAtUtc);
    out << ",\"runId\":"; Text(out, data.runUuid);
    out << ",\"result\":\"complete\",\"workload\":"; Text(out, horde::gameplay::BenchmarkWorkloadName(data.workload));
    out << ",\"simulationPolicy\":";
    Text(out, horde::gameplay::IsFrozenBenchmark(data.workload) ? "frozen-authored-snapshot" : "fixed-step-60hz");
    out << ",\"warmupLaps\":1,\"measuredLaps\":1,\"completedLaps\":" << data.laps <<
        ",\"rtPresentedEveryMeasuredFrame\":true,\"scanoutFpsMeasured\":false,\"counts\":{\"expected\":" << data.overall.frames <<
        ",\"completed\":" << data.overall.frames << ",\"cpuAccepted\":" << data.overall.frames <<
        ",\"rejected\":0,\"cancelled\":0,\"cpuRejected\":0,\"outstanding\":0},\"configuration\":{\"platform\":";
    Text(out, c.platform == BenchmarkSummaryPlatform::Windows ? "Windows" : "Android");
    out << ",\"build\":"; Text(out, m.buildIdentity); out << ",\"shaderPair\":"; Text(out, m.shaderIdentity);
    out << ",\"executionBackend\":"; Text(out, m.executionBackend);
    out << ",\"presentMode\":"; Text(out, m.presentMode);
    out << ",\"materialEncoding\":"; Text(out, m.materialEncoding);
    out << ",\"renderScalePercent\":" << m.renderScalePercent << ",\"internalExtent\":[" << m.internalWidth << ',' << m.internalHeight <<
        "],\"presentationExtent\":[" << m.presentationWidth << ',' << m.presentationHeight << "],\"waterQuality\":";
    Text(out, RtWaterQualityName(c.water)); out << ",\"dielectricQuality\":"; Text(out, RtDielectricQualityName(c.dielectric));
    out << ",\"fireQuality\":";
    Text(out, RtFireQualityName(static_cast<RtFireQuality>(c.fire)));
    if (c.shadowQuality)
    {
        const auto& quality = *c.shadowQuality;
        out << ",\"shadowQuality\":{\"mode\":"; Text(out, RtShadowModeName(quality.mode));
        out << ",\"localPrimarySamples\":" << quality.localPrimarySamples
            << ",\"skyPrimarySamples\":" << quality.skyPrimarySamples << ",\"reserved\":0}";
    }
    if (c.uploadedFireQuality)
    {
        const auto& quality = *c.uploadedFireQuality;
        out << ",\"uploadedFireQuality\":{\"volumeSteps\":" << quality.volumeSteps
            << ",\"reflectionSamples\":" << quality.reflectionSamples
            << ",\"reflectedVolumeSteps\":" << std::min(quality.volumeSteps, quality.reflectionSamples * 4u) << '}';
    }
    out << ",\"glassEnabled\":" << (c.glassEnabled ? "true" : "false") << "},\"legacyFrameTimingScope\":";
    Text(out, m.legacyFrameTimingScope);
    out << ",\"cpuTimingScope\":\"completed-owning-render-entry-through-present\",\"gpuTimingScope\":\"completed-owning-rt-duration\","
        "\"thermal\":{\"availability\":\"not-collected\",\"temperatureC\":null,\"status\":\"unknown\"},\"declaredCooling\":";
    Text(out, cooling == BenchmarkSummaryCooling::Unknown ? "unknown" :
        cooling == BenchmarkSummaryCooling::NoneDeclared ? "none-declared" : "external-declared");
    out << ",\"overall\":"; Population(out, data.overall, true);
    out << ",\"zones\":[";
    for (std::size_t i = 0u; i < data.zones.size(); ++i)
    {
        if (i != 0u) out << ',';
        out << "{\"name\":"; Text(out, horde::gameplay::ShowcaseZoneName(data.zones[i].zone));
        out << ",\"statistics\":"; Population(out, data.zones[i].population, false); out << '}';
    }
    out << ']';
    if (approval.includeBasicHardware)
    {
        out << ",\"basicHardware\":{\"rawModel\":"; Text(out, data.rawModel);
        out << ",\"gpu\":"; Text(out, m.gpuName); out << ",\"vulkanApi\":"; Text(out, m.vulkanApi); out << '}';
    }
    out << "}}";
    std::string json = out.str();
    if (json.size() > kBenchmarkSummaryReportMaxBytes)
    { result.status_ = BenchmarkSummaryReportStatus::TooLarge; return result; }
    result.reportUuid_ = approval.reportUuid;
    result.json_ = std::move(json);
    result.status_ = BenchmarkSummaryReportStatus::Ready;
    return result;
}

BenchmarkSummaryContentMatch CompareBenchmarkSummaryContent(const PreparedBenchmarkSummaryReport& a,
    const PreparedBenchmarkSummaryReport& b) noexcept
{
    if (!a.IsReady() || !b.IsReady()) return BenchmarkSummaryContentMatch::Invalid;
    if (a.ReportId() != b.ReportId()) return BenchmarkSummaryContentMatch::DifferentId;
    return a.Json() == b.Json() ? BenchmarkSummaryContentMatch::SameContent : BenchmarkSummaryContentMatch::Conflict;
}
} // namespace horde::reporting
