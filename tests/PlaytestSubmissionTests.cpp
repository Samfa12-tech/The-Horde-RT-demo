#include "reporting/PlaytestSubmission.h"
#include "reporting/BenchmarkSummarySubmission.h"

#include <iostream>
#include <vector>

namespace
{
using namespace horde::reporting;
bool passed = true;
void Check(const bool value, const char* message)
{
    if (!value) { passed = false; std::cerr << "Playtest submission: " << message << '\n'; }
}
void BigEndian(std::vector<std::uint8_t>& bytes, const std::uint32_t word)
{
    for (unsigned shift = 32u; shift != 0u; shift -= 8u)
        bytes.push_back(static_cast<std::uint8_t>(word >> (shift - 8u)));
}
void Chunk(std::vector<std::uint8_t>& png, const std::string_view type,
    const std::vector<std::uint8_t>& data)
{
    BigEndian(png, static_cast<std::uint32_t>(data.size()));
    const auto start = png.size();
    png.insert(png.end(), type.begin(), type.end());
    png.insert(png.end(), data.begin(), data.end());
    std::uint32_t crc = 0xffffffffu;
    for (std::size_t at = start; at < png.size(); ++at)
    {
        crc ^= png[at];
        for (unsigned bit = 0u; bit < 8u; ++bit)
            crc = (crc >> 1u) ^ ((crc & 1u) ? 0xedb88320u : 0u);
    }
    BigEndian(png, crc ^ 0xffffffffu);
}
std::vector<std::uint8_t> Png(const std::uint32_t width = 1u, const std::uint32_t height = 1u,
    const std::string_view metadata = {})
{
    std::vector<std::uint8_t> png{137u,80u,78u,71u,13u,10u,26u,10u};
    std::vector<std::uint8_t> header;
    BigEndian(header, width); BigEndian(header, height);
    header.insert(header.end(), {8u,6u,0u,0u,0u});
    Chunk(png, "IHDR", header);
    // Valid one-pixel RGBA zlib stream, stored block, opaque warm fixture.
    std::vector<std::uint8_t> data{0x78u,0x01u,0x01u,0x05u,0x00u,0xfau,0xffu,0u,127u,63u,31u,255u};
    std::uint32_t a = 1u, b = 0u;
    for (const std::uint8_t value : std::vector<std::uint8_t>{0u,127u,63u,31u,255u})
    { a = (a + value) % 65521u; b = (b + a) % 65521u; }
    BigEndian(data, (b << 16u) | a);
    Chunk(png, "IDAT", data);
    if (!metadata.empty()) Chunk(png, metadata, {1u});
    Chunk(png, "IEND", {});
    return png;
}
PlaytestReportInput Input()
{
    PlaytestReportInput input;
    input.reportId = "random_report_8f21";
    input.capturedAtUtc = "2026-10-02T00:01:02.003Z";
    input.note = "Walk forward.\nThen parry.\tThe hand clips the doorway.";
    input.category = PlaytestReportCategory::Visuals;
    input.impact = PlaytestReportImpact::MinorFriction;
    input.consentToSubmit = true;
    return input;
}

void BenchmarkSummarySubmissionChecks()
{
    using namespace horde::telemetry;
    using namespace horde::gameplay;
    // Real bounded route/evidence owners supply the typed builder. No arbitrary
    // ready-status JSON fixture bypasses its privacy/population admission.
    ShowcaseBenchmarkRun benchmark;
    RtBenchmarkEvidenceRun evidence;
    benchmark.Start(2u, BenchmarkWorkload::LanternHeldHigh);
    Check(evidence.Start(kLanternBenchmarkFramesPerLap), "summary evidence owner allocated");
    std::uint64_t serial = 0u;
    bool armed = false;
    while (benchmark.IsRunning())
    {
        const auto advance = benchmark.Advance();
        if (benchmark.CurrentLap() == 2u)
        {
            if (!armed) { Check(evidence.ArmMeasurement(3u,8u), "summary measured scope armed"); armed = true; }
            ++serial;
            RtPerformanceEvidenceSnapshot s;
            s.identity.submitted.frame = {3u,8u,serial,serial,1000u+serial,0u};
            s.identity.submitted.submissionSerial = serial; s.identity.completionSerial = serial;
            auto& p = s.scene.pipeline;
            p.executionMode = RtExecutionMode::RayTracingPipeline;
            p.instrumentation = RtInstrumentationMode::Shipping;
            p.dielectricQuality = RtDielectricQuality::Mobile; p.waterQuality = RtWaterQuality::Mobile;
            p.activeStrategy = RtMaterialStrategy::OpaqueFast;
            Check(AssignRtFixedText(p.bundleKey,"shipping_mobile_pair"), "summary fixture bundle key");
            Check(AssignRtFixedText(p.opaqueFast.key,"shipping_mobile_opaque"), "summary fixture opaque key");
            Check(AssignRtFixedText(p.opaqueFast.sha256,std::string(64u,'a')), "summary fixture opaque hash");
            Check(AssignRtFixedText(p.genericDielectric.key,"shipping_mobile_generic"), "summary fixture generic key");
            Check(AssignRtFixedText(p.genericDielectric.sha256,std::string(64u,'b')), "summary fixture generic hash");
            p.active = p.opaqueFast;
            s.scene.stages.status = RtSampleStatus::Valid;
            for (auto& stage : s.scene.stages.values) { stage.durationNanoseconds = 1000u; stage.operationCount = 1u; }
            s.scene.stages.values[RtStageIndex(RtStage::Skin)].durationNanoseconds = 2000u;
            s.scene.stages.values[RtStageIndex(RtStage::Skin)].operationCount = 2u;
            s.scene.stages.values[RtStageIndex(RtStage::WholeFrameCycle)].durationNanoseconds = 10'000'000u;
            s.scene.dispatch.sceneReady = s.scene.dispatch.rtDispatchRecorded = s.scene.dispatch.swapchainCopyRecorded = true;
            s.dielectric.status = RtSampleStatus::CompiledOut; s.gpu.status = RtSampleStatus::Disabled;
            s.presentation.outcome = RtPresentationOutcome::Presented;
            s.presentation.lastSuccessfulPresentSubmissionSerial = serial;
            s.cpuBenchmarkEligible = s.benchmarkEligible = true;
            const auto index = evidence.ExpectFrame({static_cast<std::uint32_t>(advance.replay.zone),2u});
            Check(index.has_value(), "summary exact lap/zone expectation retained");
            if (index)
            {
                Check(evidence.BindSubmitted(*index,s.identity.submitted), "summary exact submitted owner bound");
                Check(evidence.Complete(s), "summary completed owner accepted");
            }
        }
        benchmark.RecordFrame(16.0,true);
    }
    Check(benchmark.Passed() && evidence.RecordOwnerDrainResult(true) && evidence.Finalize(),
        "summary only prepared after real route/final owning drain");
    BenchmarkSummaryConfiguration c;
    c.sceneEpoch = 3u; c.measurementGeneration = 8u;
    c.metadata.buildIdentity = "1.6.2-offline-fixture";
    c.metadata.shaderIdentity = "opaqueFast:shipping_mobile_opaque@" + std::string(64u,'a') +
        "|genericDielectric:shipping_mobile_generic@" + std::string(64u,'b');
    c.metadata.executionBackend = "RayTracingPipeline"; c.metadata.presentMode = "FIFO";
    c.metadata.materialEncoding = "RGBA8 raw fallback";
    c.metadata.legacyFrameTimingScope = "windows-render-plus-rtlab-telemetry";
    c.metadata.internalWidth = c.metadata.presentationWidth = 960u;
    c.metadata.internalHeight = c.metadata.presentationHeight = 540u;
    constexpr std::string_view run = "11111111-1111-4111-8111-111111111111";
    constexpr std::string_view report = "22222222-2222-4222-9222-222222222222";
    const auto summary = CaptureBenchmarkSummary(benchmark,evidence,c,c,run);
    const auto local = PrepareBenchmarkSummaryReport(summary,{true,false,report,"2026-10-04T12:00:00Z"});
    Check(local.IsReady(), "actual native typed local summary ready");
    const std::string literal(local.Json());
    const auto noConsent = PrepareBenchmarkSummarySubmission(local);
    Check(noConsent.Status() == BenchmarkSummarySubmissionStatus::ConsentRequired &&
        noConsent.Json().empty() && noConsent.LocalJson().empty() && noConsent.ReportId().empty(),
        "local preparation approval never implies remote consent");
    Check(!PrepareBenchmarkSummarySubmission(PreparedBenchmarkSummaryReport{},true).IsReady(),
        "unprepared summary cannot enter typed remote wrapper");
    const auto prepared = PrepareBenchmarkSummarySubmission(local,true);
    const std::string expected = literal.substr(0u,literal.size()-1u) + ",\"consentToSubmit\":true}";
    Check(prepared.IsReady() && prepared.ReportId() == local.ReportId() && prepared.LocalJson() == literal &&
        prepared.Json() == expected && local.Json() == literal &&
        prepared.Json().find("turnstileToken") == std::string_view::npos &&
        literal.find("consentToSubmit") == std::string::npos,
        "disclosed outer remote consent alone added without reserializing local bytes or ID");
    const auto request = BuildBenchmarkSummarySubmissionRequest(prepared,"one\\\"token");
    const auto retryRequest = BuildBenchmarkSummarySubmissionRequest(prepared,"fresh-token");
    Check(request == expected.substr(0u,expected.size()-1u) + ",\"turnstileToken\":\"one\\\\\\\"token\"}" &&
        request != retryRequest && prepared.Json() == expected && prepared.LocalJson() == literal,
        "escaped one-use verification changes request only, not approved literal/wrapper");
    for (const auto token : {std::string{},std::string(2049u,'x'),std::string("bad\n"),std::string("\xff")})
        Check(BuildBenchmarkSummarySubmissionRequest(prepared,token).empty(), "summary token malformed/overflow rejected");
    Check(BuildBenchmarkSummarySubmissionRequest(noConsent,"token").empty(), "unconsented request construction rejected");
    Check(BuildBenchmarkSummarySubmissionRequest(prepared,std::string(2048u,'"')).size() <=
        kBenchmarkSummarySubmissionMaxBytes &&
        !BuildBenchmarkSummarySubmissionRequest(prepared,std::string(2048u,'"')).empty(),
        "maximum escaped token for typed fixture remains within20KiB");
    PlaytestReportDelivery owner;
    PlaytestReportAttempt attempt;
    Check(owner.BeginBenchmarkSummarySubmission(prepared,attempt) && attempt.json == expected &&
        attempt.reportId == report, "shared local attempt owner admits typed approved wrapper");
    Check(!owner.BeginBenchmarkSummarySubmission(prepared,attempt), "duplicate typed summary attempt rejected");
    const auto firstToken = attempt.token;
    Check(owner.Complete(firstToken,PlaytestReportDeliveryResult::RetryableFailure) && owner.Retry(attempt) &&
        attempt.json == expected && attempt.reportId == report &&
        !owner.Complete(firstToken,PlaytestReportDeliveryResult::Accepted),
        "explicit summary retry retains exact frozen ID/body and invalidates stale completion");
    owner.Cancel();
    Check(!owner.Complete(attempt.token,PlaytestReportDeliveryResult::Accepted) && !owner.Retry(attempt),
        "cancelled typed summary owner rejects late acceptance and automatic retry");
    Check(prepared.LocalJson() == literal && local.Json() == literal, "local Copy/Save bytes survive attempt cancellation");
    c.metadata.buildIdentity = "password=private-value";
    const auto sensitive = CaptureBenchmarkSummary(benchmark,evidence,c,c,run);
    const auto rejected = PrepareBenchmarkSummaryReport(sensitive,{true,false,report,"2026-10-04T12:00:00Z"});
    Check(!rejected.IsReady() && !PrepareBenchmarkSummarySubmission(rejected,true).IsReady(),
        "existing native text/privacy rejection preserved by typed wrapper");
}
} // namespace

int main(const int argc, char** argv)
{
    const auto png = Png();
    if (argc == 2 && std::string_view(argv[1]) == "--wire-fixture")
    {
        const auto prepared = PreparePlaytestSubmission(Input(), true, {png,1u,1u});
        std::cout << BuildPlaytestSubmissionRequest(prepared, "mock-verification-only") << '\n';
        return prepared.IsReady() ? 0 : 1;
    }
    PlaytestScreenshotPixels pixels;
    const std::vector<std::uint8_t> warm{127u,63u,31u,255u};
    Check(ResizePlaytestScreenshotRgba(1u,1u,warm,pixels) && pixels.rgba == warm &&
        pixels.width == 1u && pixels.height == 1u, "capture RGB order and small pixels preserved");
    Check(!ResizePlaytestScreenshotRgba(0u,1u,warm,pixels) && pixels.rgba.empty(), "invalid capture clears owned pixels");
    Check(!ResizePlaytestScreenshotRgba(1u,2u,warm,pixels), "capture exact byte length enforced");
    Check(!ResizePlaytestScreenshotRgba(0xffffffffu,0xffffffffu,warm,pixels), "source pixel cap before multiplication/allocation");
    std::vector<std::uint8_t> portrait(1080u * 2235u * 4u,77u);
    Check(ResizePlaytestScreenshotRgba(1080u,2235u,portrait,pixels) &&
        pixels.width == 371u && pixels.height == 768u &&
        pixels.rgba == std::vector<std::uint8_t>(371u * 768u * 4u,77u), "bounded proportional portrait and interpolation");
    std::vector<std::uint8_t> landscape(1920u * 1080u * 4u,123u);
    Check(ResizePlaytestScreenshotRgba(1920u,1080u,landscape,pixels) &&
        pixels.width == 768u && pixels.height == 432u && pixels.rgba.front() == 123u,
        "landscape both edge bounds preserved");
    std::vector<std::uint8_t> square(721u * 721u * 4u,31u);
    Check(ResizePlaytestScreenshotRgba(721u,721u,square,pixels) &&
        pixels.width == 432u && pixels.height == 432u, "square short edge independently bounded");
    std::vector<std::uint8_t> gradient(864u * 864u * 4u);
    for (std::uint32_t y = 0u; y < 864u; ++y)
        for (std::uint32_t x = 0u; x < 864u; ++x)
        {
            const auto at = (static_cast<std::size_t>(y) * 864u + x) * 4u;
            gradient[at] = static_cast<std::uint8_t>(x);
            gradient[at + 1u] = static_cast<std::uint8_t>(y);
            gradient[at + 2u] = 31u; gradient[at + 3u] = 255u;
        }
    Check(ResizePlaytestScreenshotRgba(864u,864u,gradient,pixels) &&
        pixels.width == 432u && pixels.height == 432u, "gradient both axes downsampled");
    bool gradientCorrect = pixels.rgba.size() == 432u * 432u * 4u;
    for (std::uint32_t y = 0u; gradientCorrect && y < 432u; ++y)
        for (std::uint32_t x = 0u; x < 432u; ++x)
        {
            const auto at = (static_cast<std::size_t>(y) * 432u + x) * 4u;
            gradientCorrect = gradientCorrect && pixels.rgba[at] == static_cast<std::uint8_t>(2u * x + 1u) &&
                pixels.rgba[at + 1u] == static_cast<std::uint8_t>(2u * y + 1u) &&
                pixels.rgba[at + 2u] == 31u && pixels.rgba[at + 3u] == 255u;
        }
    Check(gradientCorrect, "fixed-point bilinear pixel-centre RGB and rounding golden values");
    auto input = Input();
    input.consentToSubmit = false;
    auto prepared = PreparePlaytestSubmission(input, true, {png,1u,1u});
    Check(!prepared.IsReady() && prepared.reportStatus == PlaytestReportStatus::ConsentRequired &&
        prepared.json.empty(), "no submission bytes without explicit consent");
    input = Input();
    prepared = PreparePlaytestSubmission(input);
    const auto local = PreparePlaytestReport(input);
    Check(prepared.IsReady() && prepared.json.find(local.json) != std::string::npos &&
        prepared.json.find("\"includeScreenshot\":false") != std::string::npos &&
        prepared.json.find("\"includeDiagnostics\":false") != std::string::npos &&
        prepared.json.find("screenshot\"") == std::string::npos &&
        prepared.json.find("turnstileToken") == std::string::npos,
        "unchanged native schema wrapped with separate consent, no token or implicit attachment");
    Check(PreparePlaytestSubmission(input, false, {png,1u,1u}).status ==
        PlaytestSubmissionStatus::ScreenshotNotConsented,
        "unconsented attachment is rejected, never silently gathered or uploaded");
    Check(!PreparePlaytestSubmission(input, true).IsReady(), "requested unavailable screenshot is not fabricated");
    Check(ValidatePlaytestScreenshot({png,1u,1u}), "valid metadata-free RGBA8 PNG accepted");
    Check(!ValidatePlaytestScreenshot({png,2u,1u}), "declared dimensions must match PNG");
    auto broken = png;
    broken[broken.size() - 1u] ^= 1u;
    Check(!ValidatePlaytestScreenshot({broken,1u,1u}), "corrupt chunk CRC rejected");
    broken = png; broken.push_back(0u);
    Check(!ValidatePlaytestScreenshot({broken,1u,1u}), "trailing content rejected");
    for (const auto chunk : {"tEXt", "eXIf", "iCCP", "sRGB", "IHDR"})
    {
        const auto withMetadata = Png(1u,1u,chunk);
        Check(!ValidatePlaytestScreenshot({withMetadata,1u,1u}), "extra metadata or repeated IHDR rejected");
    }
    for (const auto size : {0u, kPlaytestScreenshotMaxLongEdge + 1u})
    {
        const auto wrongSize = Png(size,1u);
        Check(!ValidatePlaytestScreenshot({wrongSize,size,1u}), "zero/overlong image dimensions rejected");
    }
    const auto squarePng = Png(721u,721u);
    Check(!ValidatePlaytestScreenshot({squarePng,721u,721u}), "short-edge cap enforced independently");
    auto tooLarge = png; tooLarge.resize(kPlaytestScreenshotMaxBytes + 1u);
    Check(!ValidatePlaytestScreenshot({tooLarge,1u,1u}), "encoded byte cap enforced before parsing");
    for (std::size_t size = 0u; size < png.size(); ++size)
        Check(!ValidatePlaytestScreenshot({std::span(png).first(size),1u,1u}), "every truncation rejected without out-of-range access");
    prepared = PreparePlaytestSubmission(input, true, {png,1u,1u});
    Check(prepared.IsReady() && prepared.json.find("data:image/png;base64,iVBORw0KGgo") != std::string::npos,
        "bounded consented attachment encoded without filesystem data");
    const auto frozen = prepared.json;
    PlaytestReportDelivery delivery;
    PlaytestReportAttempt attempt;
    Check(delivery.BeginSubmission(prepared, attempt) && attempt.json == frozen,
        "existing foreground owner accepts frozen remote envelope");
    Check(!delivery.BeginSubmission(prepared, attempt), "no duplicate in-flight submission");
    const auto originalAttempt = attempt.token;
    Check(delivery.Complete(originalAttempt, PlaytestReportDeliveryResult::RetryableFailure) &&
        delivery.Retry(attempt) && attempt.json == frozen && attempt.reportId == input.reportId &&
        !delivery.Complete(originalAttempt, PlaytestReportDeliveryResult::Accepted),
        "explicit retry keeps bytes and invalidates late callback");
    Check(delivery.Complete(attempt.token, PlaytestReportDeliveryResult::Accepted) &&
        !delivery.Retry(attempt) && !delivery.BeginSubmission(prepared, attempt),
        "accepted remote payload cannot be submitted twice by same owner");
    PlaytestReportDelivery cancelled;
    Check(cancelled.BeginSubmission(prepared, attempt), "cancellation fixture starts");
    cancelled.Cancel();
    Check(!cancelled.Complete(attempt.token, PlaytestReportDeliveryResult::Accepted) &&
        !cancelled.Retry(attempt), "closed form drops late network acceptance");
    PreparedPlaytestReport oversizedLocal{PlaytestReportStatus::Ready,prepared.reportId,
        std::string(kPlaytestReportMaxJsonBytes + 1u,'x')};
    PlaytestReportDelivery localOwner;
    Check(!localOwner.Begin(oversizedLocal,attempt), "local JSON limit never expanded by attachment support");
    const auto first = BuildPlaytestSubmissionRequest(prepared, "token-one");
    const auto retry = BuildPlaytestSubmissionRequest(prepared, "token-two");
    Check(first != retry && prepared.json == frozen && prepared.reportId == input.reportId &&
        first.find("\"turnstileToken\":\"token-one\"") != std::string::npos,
        "refreshing one-time anti-spam token never changes frozen payload or ID");
    for (const auto token : {std::string{}, std::string(2049u,'x'), std::string("bad\n"), std::string("\xff")})
        Check(BuildPlaytestSubmissionRequest(prepared, token).empty(), "invalid/unbounded token produces no request");
    Check(!BuildPlaytestSubmissionRequest(prepared, std::string(2048u,'"')).empty(), "escaped maximum token remains bounded");
    input.note = "token\n=private-value";
    Check(!PreparePlaytestSubmission(input).IsReady(), "shared privacy contract not weakened for upload");
    input = Input(); input.includeBasicContext = true;
    Check(!PreparePlaytestSubmission(input).IsReady(), "typed unavailable context cannot be invented");
    input.context = {"Horde Lantern RT","1.6.1","build-abc","Android","SM-S948B",
        "Adreno 840","RayTracingPipeline","Mobile",0.75,1080u,2235u,false};
    prepared = PreparePlaytestSubmission(input);
    Check(prepared.IsReady() && prepared.json.find("\"includeDiagnostics\":true") != std::string::npos &&
        prepared.json.find("\"rtPresented\":false") != std::string::npos,
        "opted-in owned context retains honest false presentation evidence");
    BenchmarkSummarySubmissionChecks();
    return passed ? 0 : 1;
}
