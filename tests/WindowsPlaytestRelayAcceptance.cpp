// Explicitly invoked owner-approved synthetic acceptance utility. Keep out of
// CTest, shipping targets, and the game UI; never retries automatically.
#include "platform/windows/WindowsPlaytestSubmission.h"
#include "platform/windows/WindowsPlaytestVerification.h"
#include "reporting/PlaytestSubmission.h"

#include <bcrypt.h>

#include <array>
#include <chrono>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
using namespace horde::platform::windows;
using namespace horde::reporting;

constexpr wchar_t kApprovedFixturePath[] =
    L"C:\\Dev\\tmp\\horde-report-submission-20261002\\windows-rtx-report-thumbnail.png";
constexpr std::size_t kApprovedFixtureBytes = 463310u;
constexpr std::string_view kApprovedFixtureSha256 =
    "bc02f8fe4c6974831b3b1eb2188760bf"
    "e764992f357ae472dde5a529b4abe6bc";
constexpr std::string_view kReportId = "horde_acceptance_windows_20261002_01";
constexpr std::string_view kCapturedAtUtc = "2026-10-02T00:00:00.000Z";
constexpr std::string_view kSyntheticNote =
    "SYNTHETIC acceptance fixture only; no player issue or real-user report.";

void Wipe(std::string& value) noexcept
{
    if (!value.empty()) SecureZeroMemory(value.data(), value.size());
    value.clear();
}

void Wipe(std::vector<std::uint8_t>& value) noexcept
{
    if (!value.empty()) SecureZeroMemory(value.data(), value.size());
    value.clear();
}

struct SecureString final
{
    std::string value;
    ~SecureString() { Wipe(value); }
};

struct SecureBytes final
{
    std::vector<std::uint8_t> value;
    ~SecureBytes() { Wipe(value); }
};

struct Prepared final
{
    PreparedPlaytestSubmission value;
    ~Prepared() { Wipe(value.json); Wipe(value.reportId); }
};

struct HttpResponse final
{
    WindowsPlaytestHttpResponse value;
    ~HttpResponse() { Wipe(value.body); }
};

struct Verification final
{
    WindowsPlaytestVerificationResult value;
    ~Verification() { Wipe(value.token); }
};

struct Algorithm final
{
    BCRYPT_ALG_HANDLE value = nullptr;
    ~Algorithm() { if (value) BCryptCloseAlgorithmProvider(value, 0); }
};

struct Hash final
{
    BCRYPT_HASH_HANDLE value = nullptr;
    ~Hash() { if (value) BCryptDestroyHash(value); }
};

struct Event final
{
    HANDLE value = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    ~Event() { if (value) CloseHandle(value); }
};

bool HasApprovedHash(const std::vector<std::uint8_t>& bytes)
{
    Algorithm algorithm;
    if (BCryptOpenAlgorithmProvider(&algorithm.value, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
        return false;
    DWORD objectBytes = 0, hashBytes = 0, resultBytes = 0;
    if (BCryptGetProperty(algorithm.value, BCRYPT_OBJECT_LENGTH,
            reinterpret_cast<PUCHAR>(&objectBytes), sizeof(objectBytes), &resultBytes, 0) < 0 ||
        resultBytes != sizeof(objectBytes) || objectBytes == 0 ||
        BCryptGetProperty(algorithm.value, BCRYPT_HASH_LENGTH,
            reinterpret_cast<PUCHAR>(&hashBytes), sizeof(hashBytes), &resultBytes, 0) < 0 ||
        resultBytes != sizeof(hashBytes) || hashBytes != 32u)
        return false;
    std::vector<UCHAR> object(objectBytes);
    std::array<UCHAR, 32> digest{};
    Hash hash;
    const bool hashed = BCryptCreateHash(algorithm.value, &hash.value, object.data(), objectBytes,
            nullptr, 0, 0) >= 0 &&
        BCryptHashData(hash.value, const_cast<PUCHAR>(bytes.data()), static_cast<ULONG>(bytes.size()), 0) >= 0 &&
        BCryptFinishHash(hash.value, digest.data(), static_cast<ULONG>(digest.size()), 0) >= 0;
    bool matches = hashed;
    constexpr char hex[] = "0123456789abcdef";
    for (std::size_t i = 0; matches && i < digest.size(); ++i)
        matches = hex[digest[i] >> 4u] == kApprovedFixtureSha256[i * 2u] &&
            hex[digest[i] & 0x0fu] == kApprovedFixtureSha256[i * 2u + 1u];
    SecureZeroMemory(digest.data(), digest.size());
    SecureZeroMemory(object.data(), object.size());
    return matches;
}

bool ReadApprovedFixture(const std::filesystem::path& supplied, std::vector<std::uint8_t>& bytes)
{
    if (!supplied.is_absolute() ||
        _wcsicmp(supplied.lexically_normal().c_str(),
            std::filesystem::path(kApprovedFixturePath).lexically_normal().c_str()) != 0)
        return false;
    std::error_code error;
    const auto status = std::filesystem::symlink_status(supplied, error);
    if (error || !std::filesystem::is_regular_file(status)) return false;
    const auto size = std::filesystem::file_size(supplied, error);
    if (error || size != kApprovedFixtureBytes || size > kPlaytestScreenshotMaxBytes) return false;

    std::ifstream input(supplied, std::ios::binary);
    if (!input) return false;
    bytes.resize(kApprovedFixtureBytes);
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (input.gcount() != static_cast<std::streamsize>(bytes.size()) || input.peek() != std::char_traits<char>::eof())
    {
        Wipe(bytes);
        return false;
    }
    return HasApprovedHash(bytes);
}

std::string_view ResultName(const WindowsPlaytestSubmissionResult result) noexcept
{
    switch (result)
    {
    case WindowsPlaytestSubmissionResult::Queued: return "queued";
    case WindowsPlaytestSubmissionResult::Sent: return "sent";
    case WindowsPlaytestSubmissionResult::VerificationExpired: return "verification-expired";
    case WindowsPlaytestSubmissionResult::RateLimited: return "rate-limited";
    case WindowsPlaytestSubmissionResult::Conflict: return "conflict";
    case WindowsPlaytestSubmissionResult::Rejected: return "rejected";
    case WindowsPlaytestSubmissionResult::Uncertain: return "uncertain";
    case WindowsPlaytestSubmissionResult::None: return "none";
    }
    return "unknown";
}

int Run(const std::filesystem::path& fixture)
{
    SecureBytes png;
    if (!ReadApprovedFixture(fixture, png.value))
    {
        std::cerr << "Acceptance fixture rejected before verification or network.\n";
        return 2;
    }

    constexpr PlaytestReportContext context{
        "Horde Lantern RT", "Synthetic acceptance fixture", "Synthetic acceptance fixture",
        "Windows", "Synthetic fixture host", "Synthetic fixture GPU", "Synthetic RT fixture",
        "Fixture", 1.0, 768u, 432u, true};
    Prepared prepared;
    prepared.value = PreparePlaytestSubmission({kReportId, kCapturedAtUtc,
        PlaytestReportCategory::Visuals, PlaytestReportImpact::Polish, kSyntheticNote,
        true, true, context}, true, {png.value, 768u, 432u});
    if (!prepared.value.IsReady())
    {
        std::cerr << "Synthetic acceptance report failed shared preparation.\n";
        return 3;
    }

    Verification verification;
    verification.value = ShowWindowsPlaytestVerification(nullptr);
    if (verification.value.status != WindowsPlaytestVerificationStatus::Verified)
    {
        std::cout << "Acceptance report not sent; verification status="
                  << static_cast<unsigned>(verification.value.status) << ", reportId=" << kReportId << ".\n";
        return 4;
    }

    SecureString request;
    try
    {
        request.value = BuildPlaytestSubmissionRequest(prepared.value, verification.value.token);
    }
    catch (...)
    {
        Wipe(verification.value.token);
        std::cerr << "Acceptance request construction failed; nothing was sent.\n";
        return 5;
    }
    Wipe(verification.value.token);
    if (request.value.empty() || request.value.size() > kPlaytestSubmissionMaxBytes)
    {
        std::cerr << "Acceptance request exceeded the shared byte limit; nothing was sent.\n";
        return 6;
    }

    Event cancellation;
    if (!cancellation.value)
    {
        std::cerr << "Acceptance transport could not create its cancellation event.\n";
        return 7;
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    HttpResponse response;
    response.value = ExchangeWindowsPlaytestReport(request.value, deadline, cancellation.value);
    const auto outcome = ClassifyWindowsPlaytestResponse(response.value.statusCode,
        response.value.body, kReportId);
    std::cout << "Synthetic acceptance result=" << ResultName(outcome)
              << ", reportId=" << kReportId << ".\n";
    // Queued is deliberately reported as queued, never as sent.
    return outcome == WindowsPlaytestSubmissionResult::Queued ||
        outcome == WindowsPlaytestSubmissionResult::Sent ? 0 : 8;
}
}

int wmain(const int argc, wchar_t** argv)
{
    if (argc != 3 || std::wstring_view(argv[1]) != L"--approved-single-fixture")
    {
        std::wcerr << L"Usage: WindowsPlaytestRelayAcceptance.exe --approved-single-fixture <approved PNG path>\n";
        return 64;
    }
    try { return Run(std::filesystem::path(argv[2])); }
    catch (...)
    {
        std::cerr << "Synthetic acceptance utility failed safely; provider details were suppressed.\n";
        return 9;
    }
}
