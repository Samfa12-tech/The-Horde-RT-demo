#include "platform/windows/WindowsGitHubReleaseUpdate.h"

#include <atomic>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <commctrl.h>
#include <shellapi.h>
#include <winhttp.h>

#include "update/GitHubReleaseUpdater.h"

namespace horde::platform::windows
{
namespace
{
constexpr UINT kUpdateCheckCompletedMessage = WM_APP + 47u;
constexpr int kUpdateNowButtonId = 4701;
constexpr DWORD kConnectTimeoutMilliseconds = 3500u;
constexpr DWORD kRequestTimeoutMilliseconds = 5000u;

struct UpdateCheckPayload
{
    horde::update::UpdateCheckResult result;
    bool manualRequest = false;
};

std::atomic_bool gUpdateCheckInFlight = false;
std::atomic_uintptr_t gNextPayloadToken = 1u;
std::mutex gPayloadMutex;
std::map<std::uintptr_t, std::unique_ptr<UpdateCheckPayload>> gCompletedChecks;
HWND gUpdateWindow = nullptr;
std::uint64_t gUpdateWindowGeneration = 0u;

class WinHttpHandle
{
public:
    WinHttpHandle() = default;
    explicit WinHttpHandle(HINTERNET handle) : handle_(handle) {}
    ~WinHttpHandle() { if (handle_ != nullptr) WinHttpCloseHandle(handle_); }
    WinHttpHandle(const WinHttpHandle&) = delete;
    WinHttpHandle& operator=(const WinHttpHandle&) = delete;
    HINTERNET Get() const { return handle_; }
    explicit operator bool() const { return handle_ != nullptr; }

private:
    HINTERNET handle_ = nullptr;
};

std::wstring Utf8ToWide(const std::string& text)
{
    if (text.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                           text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (count <= 0) return L"Update details could not be displayed.";
    std::wstring wide(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                        static_cast<int>(text.size()), wide.data(), count);
    return wide;
}

horde::update::GitHubHttpResponse FetchGitHubReleaseList(
    const horde::update::GitHubHttpRequest& request)
{
    horde::update::GitHubHttpResponse response;
    const std::wstring url = Utf8ToWide(request.url);
    if (url.empty()) return response;

    URL_COMPONENTSW components{};
    components.dwStructSize = sizeof(components);
    components.dwSchemeLength = static_cast<DWORD>(-1);
    components.dwHostNameLength = static_cast<DWORD>(-1);
    components.dwUrlPathLength = static_cast<DWORD>(-1);
    components.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0u, &components) ||
        components.nScheme != INTERNET_SCHEME_HTTPS)
    {
        return response;
    }

    const std::wstring host(components.lpszHostName, components.dwHostNameLength);
    std::wstring path(components.lpszUrlPath, components.dwUrlPathLength);
    if (components.dwExtraInfoLength != 0u)
    {
        path.append(components.lpszExtraInfo, components.dwExtraInfoLength);
    }
    const std::wstring userAgent = Utf8ToWide(request.userAgent);
    WinHttpHandle session(WinHttpOpen(userAgent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                      WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0u));
    if (!session) return response;
    WinHttpSetTimeouts(session.Get(), kConnectTimeoutMilliseconds, kConnectTimeoutMilliseconds,
                       kRequestTimeoutMilliseconds, kRequestTimeoutMilliseconds);

    WinHttpHandle connection(WinHttpConnect(session.Get(), host.c_str(), components.nPort, 0u));
    if (!connection) return response;
    WinHttpHandle httpRequest(WinHttpOpenRequest(connection.Get(), L"GET", path.c_str(), nullptr,
                                                 WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                 WINHTTP_FLAG_SECURE));
    if (!httpRequest) return response;

    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    if (!WinHttpSetOption(httpRequest.Get(), WINHTTP_OPTION_REDIRECT_POLICY,
                          &redirectPolicy, sizeof(redirectPolicy)))
    {
        return response;
    }
    const std::wstring headers = L"Accept: " + Utf8ToWide(request.accept) +
                                 L"\r\nX-GitHub-Api-Version: " + Utf8ToWide(request.apiVersion) + L"\r\n";
    if (!WinHttpSendRequest(httpRequest.Get(), headers.c_str(), static_cast<DWORD>(headers.size()),
                            WINHTTP_NO_REQUEST_DATA, 0u, 0u, 0u) ||
        !WinHttpReceiveResponse(httpRequest.Get(), nullptr))
    {
        return response;
    }

    DWORD status = 0u;
    DWORD statusSize = sizeof(status);
    if (!WinHttpQueryHeaders(httpRequest.Get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize,
                             WINHTTP_NO_HEADER_INDEX))
    {
        return response;
    }
    response.statusCode = static_cast<int>(status);

    std::vector<char> chunk(8192u);
    while (true)
    {
        DWORD bytesRead = 0u;
        if (!WinHttpReadData(httpRequest.Get(), chunk.data(), static_cast<DWORD>(chunk.size()), &bytesRead))
        {
            response.statusCode = 0;
            response.body.clear();
            return response;
        }
        if (bytesRead == 0u) break;
        const std::size_t remaining = request.maximumResponseBytes + 1u -
                                      std::min(response.body.size(), request.maximumResponseBytes + 1u);
        response.body.append(chunk.data(), std::min<std::size_t>(bytesRead, remaining));
        if (response.body.size() > request.maximumResponseBytes) break;
    }
    return response;
}

void OpenVerifiedReleasePage(HWND window, const std::string& releasePageUrl)
{
    const HINSTANCE opened = ShellExecuteA(window, "open", releasePageUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(opened) <= 32)
    {
        MessageBoxA(window, "The GitHub release page could not be opened in your default browser.",
                    "Horde Lantern RT - update", MB_OK | MB_ICONERROR);
    }
}

void ShowAvailableUpdate(HWND window, const horde::update::UpdateMetadata& update)
{
    std::string body = "Horde Lantern RT " + update.version + " is available.";
    if (!update.title.empty() && update.title != update.tag) body += "\n\n" + update.title;
    if (!update.notes.empty()) body += "\n\n" + update.notes;
    const std::wstring wideBody = Utf8ToWide(body);
    const TASKDIALOG_BUTTON buttons[] = {
        {kUpdateNowButtonId, L"Update now"},
        {IDCANCEL, L"Later"},
    };
    TASKDIALOGCONFIG dialog{};
    dialog.cbSize = sizeof(dialog);
    dialog.hwndParent = window;
    dialog.dwFlags = TDF_SIZE_TO_CONTENT | TDF_ALLOW_DIALOG_CANCELLATION;
    dialog.pszWindowTitle = L"Horde Lantern RT - update available";
    dialog.pszMainIcon = TD_INFORMATION_ICON;
    dialog.pszMainInstruction = L"A new version is available";
    dialog.pszContent = wideBody.c_str();
    dialog.cButtons = static_cast<UINT>(std::size(buttons));
    dialog.pButtons = buttons;
    dialog.nDefaultButton = kUpdateNowButtonId;
    int pressed = IDCANCEL;
    if (SUCCEEDED(TaskDialogIndirect(&dialog, &pressed, nullptr, nullptr)) && pressed == kUpdateNowButtonId)
    {
        OpenVerifiedReleasePage(window, update.releasePageUrl);
    }
}
}

void BeginGitHubReleaseUpdateCheck(HWND window, const char* installedVersion, bool manualRequest)
{
    bool expected = false;
    if (!gUpdateCheckInFlight.compare_exchange_strong(expected, true))
    {
        if (manualRequest)
        {
            MessageBoxA(window, "An update check is already in progress.",
                        "Horde Lantern RT - updates", MB_OK | MB_ICONINFORMATION);
        }
        return;
    }

    const std::string version = installedVersion == nullptr ? std::string{} : installedVersion;
    std::uint64_t windowGeneration = 0u;
    {
        std::lock_guard<std::mutex> lock(gPayloadMutex);
        gUpdateWindow = window;
        windowGeneration = ++gUpdateWindowGeneration;
    }
    std::thread([window, windowGeneration, version, manualRequest]() {
        auto payload = std::make_unique<UpdateCheckPayload>();
        payload->manualRequest = manualRequest;
        payload->result = horde::update::CheckForGitHubReleaseUpdate(
            version, horde::update::ReleaseChannel::IncludePrerelease, FetchGitHubReleaseList);
        const std::uintptr_t token = gNextPayloadToken.fetch_add(1u);
        {
            std::lock_guard<std::mutex> lock(gPayloadMutex);
            if (gUpdateWindow != window || gUpdateWindowGeneration != windowGeneration)
            {
                return;
            }
            gCompletedChecks.emplace(token, std::move(payload));
        }
        if (!PostMessageA(window, kUpdateCheckCompletedMessage, static_cast<WPARAM>(token), 0u))
        {
            std::lock_guard<std::mutex> lock(gPayloadMutex);
            gCompletedChecks.erase(token);
            if (gUpdateWindow == window && gUpdateWindowGeneration == windowGeneration)
            {
                gUpdateCheckInFlight.store(false);
            }
        }
    }).detach();
}

bool HandleGitHubReleaseUpdateMessage(HWND window, UINT message, WPARAM rawToken)
{
    if (message != kUpdateCheckCompletedMessage) return false;
    std::unique_ptr<UpdateCheckPayload> payload;
    {
        std::lock_guard<std::mutex> lock(gPayloadMutex);
        const auto found = gCompletedChecks.find(static_cast<std::uintptr_t>(rawToken));
        if (gUpdateWindow == window && found != gCompletedChecks.end())
        {
            payload = std::move(found->second);
            gCompletedChecks.erase(found);
            gUpdateCheckInFlight.store(false);
        }
    }
    if (!payload) return true;

    using horde::update::UpdateCheckStatus;
    if (payload->result.status == UpdateCheckStatus::UpdateAvailable && payload->result.update)
    {
        ShowAvailableUpdate(window, *payload->result.update);
    }
    else if (payload->manualRequest &&
             (payload->result.status == UpdateCheckStatus::UpToDate ||
              payload->result.status == UpdateCheckStatus::NoPublishedRelease))
    {
        MessageBoxA(window, "You already have the newest published version.",
                    "Horde Lantern RT - updates", MB_OK | MB_ICONINFORMATION);
    }
    else if (payload->manualRequest)
    {
        MessageBoxA(window,
                    "Horde Lantern RT could not check GitHub Releases. Check your connection and try again.",
                    "Horde Lantern RT - updates", MB_OK | MB_ICONWARNING);
    }
    return true;
}

void CancelGitHubReleaseUpdateCheck(HWND window)
{
    std::lock_guard<std::mutex> lock(gPayloadMutex);
    if (gUpdateWindow != window) return;
    gUpdateWindow = nullptr;
    ++gUpdateWindowGeneration;
    gCompletedChecks.clear();
    gUpdateCheckInFlight.store(false);
}
}
