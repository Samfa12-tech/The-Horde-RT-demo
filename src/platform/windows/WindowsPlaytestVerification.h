#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <string>
#include <string_view>

namespace horde::platform::windows
{
inline constexpr wchar_t kWindowsPlaytestVerificationPageUrl[] =
    L"https://briarhold-signal.samfa12.com/horde-report/verify";
inline constexpr wchar_t kWindowsPlaytestVerificationPageSource[] =
    L"https://briarhold-signal.samfa12.com/horde-report/verify";
inline constexpr wchar_t kWindowsPlaytestVerificationChallengeHost[] =
    L"challenges.cloudflare.com";
inline constexpr unsigned kWindowsPlaytestVerificationMaxMessageBytes = 4096;
inline constexpr unsigned kWindowsPlaytestVerificationMaxTokenChars = 2048;
inline constexpr unsigned kWindowsPlaytestVerificationDeadlineMs = 20'000;

enum class WindowsPlaytestVerificationStatus : unsigned char
{
    Verified,
    Failed,
    Cancelled,
    Unavailable,
    Deadline,
};

struct WindowsPlaytestVerificationResult
{
    WindowsPlaytestVerificationStatus status = WindowsPlaytestVerificationStatus::Unavailable;
    // A successful result contains the sole transient token. Callers must pass
    // it directly to their explicit send action and then discard it.
    std::string token;
};

struct WindowsPlaytestVerificationReply
{
    bool recognized = false;
    bool verified = false;
    std::string token;
};

class WindowsPlaytestVerificationOneShot final
{
public:
    [[nodiscard]] bool TryComplete(const WindowsPlaytestVerificationReply& reply,
        WindowsPlaytestVerificationResult& result) noexcept;
    [[nodiscard]] bool IsTerminal() const noexcept { return terminal_; }
private:
    bool terminal_ = false;
};

[[nodiscard]] bool IsAllowedWindowsPlaytestVerificationPageUrl(std::wstring_view url) noexcept;
[[nodiscard]] bool IsAllowedWindowsPlaytestVerificationChallengeUrl(std::wstring_view url) noexcept;
[[nodiscard]] WindowsPlaytestVerificationReply ParseWindowsPlaytestVerificationMessage(
    std::string_view json, std::string_view expectedNonce) noexcept;

// Called on the application UI thread. Shows an owner-modal native window and
// pumps that thread until verified, cancelled, failed, or the 20 second total
// foreground deadline expires. It never receives report bytes or a screenshot.
[[nodiscard]] WindowsPlaytestVerificationResult ShowWindowsPlaytestVerification(HWND owner);
} // namespace horde::platform::windows
