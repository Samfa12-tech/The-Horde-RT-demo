#include "platform/windows/WindowsPlaytestVerification.h"

#include <iostream>
#include <string>
#include <string_view>

using namespace horde::platform::windows;

namespace
{
int failures = 0;
void Check(bool condition, std::string_view message)
{
    if (!condition) { ++failures; std::cerr << "Windows Turnstile verification: " << message << '\n'; }
}
}

int main(int argc, char** argv)
{
    // Explicit developer/owner smoke only. Normal CTest never opens a browser
    // or contacts the hosted page. Never log a real token or submit a report.
    if (argc == 2 && std::string_view(argv[1]) == "--interactive-verification-smoke")
    {
        auto result = ShowWindowsPlaytestVerification(nullptr);
        if (!result.token.empty()) SecureZeroMemory(result.token.data(), result.token.size());
        result.token.clear();
        std::cout << "Real verification status enum (0=verified, 1=failed, 2=cancelled, 3=unavailable, 4=deadline): "
                  << static_cast<int>(result.status) << ", last local stage: "
                  << WindowsPlaytestVerificationStageName(result.stage)
                  << ". No report or email submitted.\n";
        return result.status == WindowsPlaytestVerificationStatus::Verified ? 0 : 2;
    }
    if (argc != 1) return 2;
    for (const auto stage : {
        WindowsPlaytestVerificationStage::NotStarted,
        WindowsPlaytestVerificationStage::ModalReady,
        WindowsPlaytestVerificationStage::EnvironmentReady,
        WindowsPlaytestVerificationStage::InPrivateControllerReady,
        WindowsPlaytestVerificationStage::PageNavigationStarted,
        WindowsPlaytestVerificationStage::PageLoaded,
        WindowsPlaytestVerificationStage::HandshakePosted,
        WindowsPlaytestVerificationStage::ReplyReceived})
        Check(!WindowsPlaytestVerificationStageName(stage).empty(), "stage diagnostic label is fixed and non-empty");
    Check(IsAllowedWindowsPlaytestVerificationPageUrl(
        L"https://briarhold-signal.samfa12.com/horde-report/verify"), "fixed verification page is allowed");
    for (const auto bad : {
        L"http://briarhold-signal.samfa12.com/horde-report/verify",
        L"https://briarhold-signal.samfa12.com/horde-report/verify/",
        L"https://briarhold-signal.samfa12.com/horde-report/verify?token=x",
        L"https://briarhold-signal.samfa12.com.evil.test/horde-report/verify",
        L"https://user@briarhold-signal.samfa12.com/horde-report/verify",
        L"https://briarhold-signal.samfa12.com:443/horde-report/verify"})
        Check(!IsAllowedWindowsPlaytestVerificationPageUrl(bad), "non-exact top-level URL is rejected");

    Check(IsAllowedWindowsPlaytestVerificationChallengeUrl(L"https://challenges.cloudflare.com/turnstile/v0/api.js"),
        "Cloudflare challenge HTTPS origin is allowed");
    Check(IsAllowedWindowsPlaytestVerificationChallengeUrl(L"https://CHALLENGES.CLOUDFLARE.COM:443/cdn-cgi/challenge-platform/"),
        "challenge host comparison is case-insensitive and explicit port 443 is allowed");
    for (const auto local : {L"about:blank", L"about:srcdoc"})
    {
        Check(IsAllowedWindowsPlaytestVerificationFrameUrl(local), "exact local challenge child document is allowed");
        Check(!IsAllowedWindowsPlaytestVerificationPageUrl(local), "local child document cannot replace the top-level page");
        Check(!IsAllowedWindowsPlaytestVerificationChallengeUrl(local), "local child document does not widen outbound hosts");
    }
    Check(IsAllowedWindowsPlaytestVerificationFrameUrl(L"https://challenges.cloudflare.com/turnstile/v0/api.js"),
        "challenge origin remains allowed for child navigation");
    for (const auto bad : {L"about:blank?x=1", L"about:blank#fragment", L"about:srcdoc#fragment",
        L"about:config", L"data:text/html,hello", L"file:///private.txt", L"https://evil.test/"})
        Check(!IsAllowedWindowsPlaytestVerificationFrameUrl(bad), "other local or remote child documents are rejected");
    for (const auto bad : {
        L"http://challenges.cloudflare.com/script.js",
        L"https://challenges.cloudflare.com.evil.test/script.js",
        L"https://user@challenges.cloudflare.com/script.js",
        L"https://challenges.cloudflare.com:444/script.js",
        L"https://challenges.cloudflare.com/script.js#fragment",
        L"https://challenges.cloudflare.com@evil.test/script.js"})
        Check(!IsAllowedWindowsPlaytestVerificationChallengeUrl(bad), "non-allowed challenge URL is rejected");

    constexpr std::string_view nonce = "A7dQ_2bC9xL3mP5rT8vW0yZa";
    const auto valid = ParseWindowsPlaytestVerificationMessage(
        R"({"type":"horde-report-verification","nonce":"A7dQ_2bC9xL3mP5rT8vW0yZa","status":"verified","token":"transient-token"})",
        nonce);
    Check(valid.recognized && valid.verified && valid.token == "transient-token", "valid matching token parses");
    const auto failed = ParseWindowsPlaytestVerificationMessage(
        R"({"type":"horde-report-verification","nonce":"A7dQ_2bC9xL3mP5rT8vW0yZa","status":"failed"})",
        nonce);
    Check(failed.recognized && !failed.verified && failed.token.empty(), "valid failure parses without token");
    Check(!ParseWindowsPlaytestVerificationMessage(
        R"({"type":"horde-report-verification","nonce":"wrong_nonce_123456","status":"verified","token":"x"})",
        nonce).recognized, "mismatched nonce is ignored");
    Check(!ParseWindowsPlaytestVerificationMessage(
        R"({"type":"horde-report-verification","nonce":"A7dQ_2bC9xL3mP5rT8vW0yZa","status":"verified","token":"x","extra":"y"})",
        nonce).recognized, "unknown field is rejected");
    Check(!ParseWindowsPlaytestVerificationMessage(
        R"({"type":"horde-report-verification","nonce":"A7dQ_2bC9xL3mP5rT8vW0yZa","nonce":"A7dQ_2bC9xL3mP5rT8vW0yZa","status":"verified","token":"x"})",
        nonce).recognized, "duplicate key is rejected");
    Check(!ParseWindowsPlaytestVerificationMessage(
        R"({"type":"horde-report-verification","nonce":"A7dQ_2bC9xL3mP5rT8vW0yZa","status":"verified","token":"x","})",
        nonce).recognized, "malformed JSON is rejected");
    Check(!ParseWindowsPlaytestVerificationMessage(
        R"({"type":"horde-report-verification","nonce":"A7dQ_2bC9xL3mP5rT8vW0yZa","status":"verified","token":"line\nfeed"})",
        nonce).recognized, "control characters in token are rejected");
    Check(!ParseWindowsPlaytestVerificationMessage(
        R"({"type":"horde-report-verification","nonce":"A7dQ_2bC9xL3mP5rT8vW0yZa","status":"verified","token":"x"})" +
            std::string(kWindowsPlaytestVerificationMaxMessageBytes, ' '), nonce).recognized,
        "message above 4 KiB is rejected");
    const std::string longToken(kWindowsPlaytestVerificationMaxTokenChars + 1, 'x');
    Check(!ParseWindowsPlaytestVerificationMessage(
        "{\"type\":\"horde-report-verification\",\"nonce\":\"" + std::string(nonce) +
        "\",\"status\":\"verified\",\"token\":\"" + longToken + "\"}", nonce).recognized,
        "token above 2048 ASCII bytes is rejected");

    WindowsPlaytestVerificationOneShot gate;
    WindowsPlaytestVerificationResult accepted;
    Check(gate.TryComplete(valid, accepted) && accepted.status == WindowsPlaytestVerificationStatus::Verified &&
        accepted.token == "transient-token", "first valid result completes with token");
    Check(!gate.TryComplete(valid, accepted) && accepted.token == "transient-token",
        "duplicate completion is ignored after one-shot success");
    WindowsPlaytestVerificationOneShot failedGate;
    Check(failedGate.TryComplete(failed, accepted) && accepted.status == WindowsPlaytestVerificationStatus::Failed,
        "failure response is terminal");
    Check(!failedGate.TryComplete(valid, accepted) && accepted.status == WindowsPlaytestVerificationStatus::Failed,
        "late success cannot replace terminal failure");

    if (failures == 0) std::cout << "Windows Turnstile verification contracts passed.\n";
    return failures == 0 ? 0 : 1;
}
