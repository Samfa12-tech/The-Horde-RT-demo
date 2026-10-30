#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>

namespace horde::platform::windows {
// Development-only process lease. This is test-state isolation, not a security
// boundary against another process running as the same Windows user.
class WindowsAcceptanceProfile final {
public:
    ~WindowsAcceptanceProfile() { if (lease_ != INVALID_HANDLE_VALUE) CloseHandle(lease_); }
    WindowsAcceptanceProfile() = default;
    WindowsAcceptanceProfile(const WindowsAcceptanceProfile&) = delete;
    WindowsAcceptanceProfile& operator=(const WindowsAcceptanceProfile&) = delete;
    bool Configure(std::span<const std::wstring_view> args, bool developmentEnabled,
        const std::filesystem::path& reports, const std::filesystem::path& executableDirectory);
    bool Active() const { return !root_.empty(); }
    std::filesystem::path CampaignDirectory() const { return Active() ? root_ / L"campaign-v1" : std::filesystem::path{}; }
    std::filesystem::path SettingsPath() const { return Active() ? root_ / L"settings.ini" : std::filesystem::path{}; }
    const std::string& Error() const { return error_; }
private:
    HANDLE lease_ = INVALID_HANDLE_VALUE;
    std::filesystem::path root_;
    std::string error_;
};
}
