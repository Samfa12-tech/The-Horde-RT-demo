#include "reporting/PlaytestReport.h"

#include <array>
#include <iostream>
#include <string>

namespace
{
using namespace horde::reporting;
bool passed = true;

void Check(const bool condition, const char* message)
{
    if (!condition) { passed = false; std::cerr << "Playtest report contract: " << message << '\n'; }
}

PlaytestReportInput ValidInput()
{
    PlaytestReportInput input;
    input.reportId = "random_report_8f21";
    input.capturedAtUtc = "2026-10-01T12:34:56.789Z";
    input.category = PlaytestReportCategory::Gameplay;
    input.impact = PlaytestReportImpact::MajorFriction;
    input.note = "The lantern clips through the doorway near the west stair.";
    input.consentToSubmit = true;
    return input;
}

PlaytestReportContext ValidContext()
{
    return {
        .product = "Horde Lantern RT",
        .version = "1.6.1",
        .build = "build-abc123",
        .platform = "Android",
        .rawModel = "SM-S948B",
        .gpu = "Adreno 840",
        .backend = "Vulkan ray tracing",
        .quality = "Mobile",
        .renderScale = 0.75,
        .internalWidth = 1440u,
        .internalHeight = 1080u,
        .rtPresented = true,
    };
}

void TestConsentSchemaAndContext()
{
    auto input = ValidInput();
    input.consentToSubmit = false;
    auto result = PreparePlaytestReport(input);
    Check(result.status == PlaytestReportStatus::ConsentRequired && result.json.empty(),
          "default-off explicit submission/export consent produces no bytes");

    input = ValidInput();
    result = PreparePlaytestReport(input);
    Check(result.IsReady() && result.reportId == input.reportId &&
          result.json.find("\"schemaVersion\":1") != std::string::npos &&
          result.json.find("\"note\":\"The lantern clips") != std::string::npos &&
          result.json.find("\"context\"") == std::string::npos,
          "schema v1 prepares only player-authored note by default");
    Check(result.json.find("screenshot") == std::string::npos &&
          result.json.find("save") == std::string::npos &&
          result.json.find("logs") == std::string::npos,
          "no attachments, save payload or arbitrary diagnostics are serialized");

    input.includeBasicContext = true;
    input.context = ValidContext();
    result = PreparePlaytestReport(input);
    Check(result.IsReady() && result.json.find("\"rawModel\":\"SM-S948B\"") != std::string::npos &&
          result.json.find("\"internalExtent\":[1440,1080]") != std::string::npos &&
          result.json.find("\"rtPresented\":true") != std::string::npos,
          "opted-in context is limited to the typed allowlist and honest RT presentation flag");

    input.context.build = "tag with \"quotes\" and \\ slashes";
    result = PreparePlaytestReport(input);
    Check(result.IsReady() && result.json.find("tag with \\\"quotes\\\" and \\\\ slashes") != std::string::npos,
          "JSON string fields are escaped safely");
}

void TestRejectedIdentifiersTimesEnumsAndText()
{
    auto input = ValidInput();
    input.reportId = "sam@example.com";
    Check(PreparePlaytestReport(input).status == PlaytestReportStatus::InvalidReportId,
          "ID syntax rejects contact identifiers");
    input = ValidInput(); input.capturedAtUtc = "2026-02-30T12:34:56Z";
    Check(PreparePlaytestReport(input).status == PlaytestReportStatus::InvalidTimestamp,
          "invalid calendar dates rejected");
    input = ValidInput(); input.capturedAtUtc = "2026-10-01T12:34:56+10:00";
    Check(PreparePlaytestReport(input).status == PlaytestReportStatus::InvalidTimestamp,
          "non-UTC local offsets rejected");
    input = ValidInput(); input.category = static_cast<PlaytestReportCategory>(255u);
    Check(PreparePlaytestReport(input).status == PlaytestReportStatus::InvalidCategory,
          "unknown category enum rejected");
    input = ValidInput(); input.impact = static_cast<PlaytestReportImpact>(255u);
    Check(PreparePlaytestReport(input).status == PlaytestReportStatus::InvalidImpact,
          "unknown impact enum rejected");
    input = ValidInput(); input.note = "";
    Check(PreparePlaytestReport(input).status == PlaytestReportStatus::EmptyNote, "empty note rejected");
    input = ValidInput();
    const std::string oversizedNote(kPlaytestReportMaxNoteBytes + 1u, 'x');
    input.note = oversizedNote;
    Check(PreparePlaytestReport(input).status == PlaytestReportStatus::NoteTooLarge,
          "note overflow is rejected, not silently truncated");
    input = ValidInput(); input.note = "bad\x01line";
    Check(PreparePlaytestReport(input).status == PlaytestReportStatus::ControlCharacter,
          "control characters rejected");
    input = ValidInput();
    const std::string invalidUtf8("bad\xc0\xaf", 5u);
    input.note = invalidUtf8;
    Check(PreparePlaytestReport(input).status == PlaytestReportStatus::InvalidUtf8,
          "invalid UTF-8 rejected");
    input = ValidInput();
    const std::string quoteHeavyNote(kPlaytestReportMaxNoteBytes, '"');
    input.note = quoteHeavyNote;
    input.includeBasicContext = true;
    const std::string padded(kPlaytestReportMaxContextStringBytes, '"');
    input.context = {.product=padded, .version=padded, .build=padded, .platform=padded,
        .rawModel=padded, .gpu=padded, .backend=padded, .quality=padded,
        .renderScale=1.0, .internalWidth=1u, .internalHeight=1u, .rtPresented=false};
    Check(PreparePlaytestReport(input).status == PlaytestReportStatus::JsonTooLarge,
          "final encoded JSON byte cap is independently enforced");

    input = ValidInput();
    input.note = "The torch looks good in the 龍 cave 🔥.";
    Check(PreparePlaytestReport(input).IsReady(), "valid multibyte UTF-8 remains accepted");
    input.note = " \t \r\n ";
    Check(PreparePlaytestReport(input).status == PlaytestReportStatus::EmptyNote,
          "whitespace-only note does not count as a player issue");
}

void TestSensitiveContentRejected()
{
    const std::array<std::string, 7u> unsafeNotes{
        "token=abc123", "Bearer abc123", "See https://example.invalid/path", "email me at sam@example.invalid",
        "call +1 (555) 123-4567", "file C:\\Users\\sam\\save.json", "saved under /home/sam/private/run",
    };
    for (const auto& note : unsafeNotes)
    {
        auto input = ValidInput();
        input.note = note;
        const auto result = PreparePlaytestReport(input);
        Check(result.status == PlaytestReportStatus::SensitiveContent && result.json.empty(),
              "likely credential/contact/URL/private path is explicitly rejected");
    }
    auto contextInput = ValidInput();
    contextInput.includeBasicContext = true;
    contextInput.context = ValidContext();
    contextInput.context.gpu = "gpu token:secret-value";
    const auto contextResult = PreparePlaytestReport(contextInput);
    Check(contextResult.status == PlaytestReportStatus::InvalidContext && contextResult.json.empty(),
          "sensitive opted-in context is rejected before output");
}

void TestBearerCredentialBoundaries()
{
    constexpr std::string_view safeNotes[] = {
        "The torchbearer is silent", "The TORCHBEARER is silent", "The standardbearer: silent",
        "The torchbearer=quiet", "The bearer.", "The bearer, beside the stairs", "The bearer!",
        "Bearer", "Bearer  ", "Bearer .", "Bearer ,", "bearerhood is unfamiliar",
        "torchbearer Bearer.", "bearer_figure is silent", "2bearer is silent",
        "bearer\xc3\xa9 is silent",
    };
    for (const auto note : safeNotes)
    {
        auto input = ValidInput();
        input.note = note;
        Check(PreparePlaytestReport(input).IsReady(),
              "ordinary bearer words and punctuation are accepted");
    }
    constexpr std::string_view unsafeNotes[] = {
        "Bearer abc123", "bEaReR abc123", "(Bearer abc123)", "Bearer\tabc123",
        "Bearer\r\nabc123", "Bearer:abc123", "Bearer=abc123", "Bearer : abc123",
        "Bearer a.b_c-12+/=", "torchbearer is silent; Bearer abc123", "torch-bearer abc123",
        "\xe3\x80\x90" "Bearer abc123", // Unicode opening bracket retains credential protection.
        "token=abc123", "secret:abc123", "password=abc123", "authorization:abc123",
        "cookie=abc123", "api_key=abc123", "apikey:abc123", "credential=abc123",
    };
    for (const auto note : unsafeNotes)
    {
        auto input = ValidInput();
        input.note = note;
        const auto result = PreparePlaytestReport(input);
        Check(result.status == PlaytestReportStatus::SensitiveContent && result.json.empty(),
              "separate bearer credentials and other existing secret patterns remain rejected");
    }
    auto input = ValidInput();
    input.includeBasicContext = true;
    input.context = ValidContext();
    input.context.build = "torchbearer build";
    Check(PreparePlaytestReport(input).IsReady(), "context shares corrected bearer word boundaries");
    input.context.build = "build (Bearer abc123)";
    const auto result = PreparePlaytestReport(input);
    Check(result.status == PlaytestReportStatus::InvalidContext && result.json.empty(),
          "context retains bearer credential protection");
}

void TestRetryIdentityAndCancellation()
{
    const auto prepared = PreparePlaytestReport(ValidInput());
    PlaytestReportDelivery delivery;
    PlaytestReportAttempt first;
    auto forged = prepared;
    forged.reportId = "not valid!";
    Check(!delivery.Begin(forged, first), "delivery seam rejects forged invalid ID");
    forged = prepared;
    forged.json.assign(kPlaytestReportMaxJsonBytes + 1u, 'x');
    Check(!delivery.Begin(forged, first), "delivery seam rejects forged oversized bytes");
    forged = prepared;
    forged.json.clear();
    Check(!delivery.Begin(forged, first), "delivery seam rejects forged empty bytes");
    Check(delivery.Begin(prepared, first) && delivery.State() == PlaytestReportDeliveryState::InFlight,
          "explicit valid prepared report begins one foreground attempt");
    Check(first.reportId == prepared.reportId && first.json == prepared.json,
          "attempt contains the exact prepared ID and bytes");
    Check(!delivery.Begin(prepared, first), "active attempt cannot be duplicated");
    Check(delivery.Complete(first.token, PlaytestReportDeliveryResult::RetryableFailure) &&
          delivery.State() == PlaytestReportDeliveryState::RetryableFailure,
          "transport failure enables only explicit retry");
    PlaytestReportAttempt retry;
    Check(delivery.Retry(retry) && retry.token != first.token && retry.reportId == first.reportId &&
          retry.json == first.json, "retry preserves byte-identical report ID and JSON");
    delivery.Cancel();
    Check(delivery.State() == PlaytestReportDeliveryState::Cancelled &&
          !delivery.Complete(retry.token, PlaytestReportDeliveryResult::Accepted),
          "cancellation invalidates late completion");
    Check(!delivery.Retry(retry), "cancelled report is not automatically retried");

    PlaytestReportDelivery accepted;
    Check(accepted.Begin(prepared, first), "new owner may begin after cancellation");
    Check(accepted.Complete(first.token, PlaytestReportDeliveryResult::Accepted) &&
          accepted.State() == PlaytestReportDeliveryState::Accepted && !accepted.Retry(retry),
          "accepted delivery is terminal and is not resubmitted");
}

} // namespace

int main()
{
    {
        std::string output = "stale";
        Check(EncodePlaytestReportUtf16(u"\u9f8d \U0001f525", 8u, output) == PlaytestReportStatus::Ready &&
            output == "\xe9\xbe\x8d \xf0\x9f\x94\xa5", "UTF-16 CJK and paired emoji encode to exact UTF-8 bytes");
        Check(EncodePlaytestReportUtf16(u"\u9f8d \U0001f525", 7u, output) == PlaytestReportStatus::NoteTooLarge &&
            output.empty(), "UTF-8 byte cap clears output instead of truncating");
        const std::u16string high(1u, static_cast<char16_t>(0xd800u));
        const std::u16string low(1u, static_cast<char16_t>(0xdc00u));
        Check(EncodePlaytestReportUtf16(high, 10u, output) == PlaytestReportStatus::InvalidUtf8 &&
            output.empty(), "unpaired high surrogate rejected");
        Check(EncodePlaytestReportUtf16(low, 10u, output) == PlaytestReportStatus::InvalidUtf8 &&
            output.empty(), "unpaired low surrogate rejected");
        Check(EncodePlaytestReportUtf16(high + u"a", 10u, output) == PlaytestReportStatus::InvalidUtf8,
            "high surrogate followed by ordinary character rejected");
        const std::u16string nul(1u, u'\0');
        Check(EncodePlaytestReportUtf16(nul, 1u, output) == PlaytestReportStatus::Ready &&
            output.size() == 1u && output[0] == '\0', "NUL remains for builder policy, not JNI truncation");
        Check(EncodePlaytestReportUtf16(u"", 0u, output) == PlaytestReportStatus::Ready && output.empty(),
            "empty conversion remains builder's required-note decision");
    }
    TestConsentSchemaAndContext();
    {
        auto input = ValidInput();
        input.note = "Walk forward.\r\nThen parry.\tExpected: no clipping.";
        const auto prepared = PreparePlaytestReport(input);
        Check(prepared.IsReady() && prepared.json.find("\\r\\n") != std::string::npos &&
            prepared.json.find("\\t") != std::string::npos,
            "authored multiline steps survive as escaped JSON, not flattened/truncated");
        input.includeBasicContext = true;
        input.context = ValidContext();
        input.context.gpu = "GPU\nInjected";
        Check(PreparePlaytestReport(input).status == PlaytestReportStatus::InvalidContext,
            "note layout exception never permits control characters in context");
        input = ValidInput(); input.note = "token\n=secret-value";
        Check(PreparePlaytestReport(input).status == PlaytestReportStatus::SensitiveContent,
            "credential assignment cannot bypass detection through an authored line break");
    }
    TestRejectedIdentifiersTimesEnumsAndText();
    TestSensitiveContentRejected();
    TestBearerCredentialBoundaries();
    TestRetryIdentityAndCancellation();
    return passed ? 0 : 1;
}
