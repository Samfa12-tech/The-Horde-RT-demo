#include "reporting/PlaytestSubmission.h"

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
    const auto square = Png(721u,721u);
    Check(!ValidatePlaytestScreenshot({square,721u,721u}), "short-edge cap enforced independently");
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
    return passed ? 0 : 1;
}
