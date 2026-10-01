#include "platform/windows/WindowsPlaytestVerification.h"

#include <objbase.h> // COM interface declarations before the generated SDK header.
#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>
#include <wrl.h>
#include <bcrypt.h>
#include <shellscalingapi.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cwchar>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace horde::platform::windows
{
namespace
{
using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

constexpr wchar_t kWindowClass[] = L"HordePlaytestVerificationWindow";
constexpr UINT_PTR kDeadlineTimer = 0x484f;
constexpr int kCancelButton = 0x4841;
constexpr int kStatusLabel = 0x4842;
constexpr UINT kFinishVerificationMessage = WM_APP + 0x5a;
constexpr wchar_t kEphemeralProfileName[] = L"HordeReportVerification";

bool IsAsciiToken(std::string_view value) noexcept
{
    if (value.empty() || value.size() > kWindowsPlaytestVerificationMaxTokenChars) return false;
    for (const unsigned char ch : value)
        if (ch < 0x20 || ch > 0x7e) return false;
    return true;
}

bool IsNonce(std::string_view value) noexcept
{
    if (value.size() < 16 || value.size() > 96) return false;
    for (const unsigned char ch : value)
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
              (ch >= '0' && ch <= '9') || ch == '-' || ch == '_')) return false;
    return true;
}

class JsonFlatObjectParser final
{
public:
    explicit JsonFlatObjectParser(std::string_view source) : source_(source) {}

    bool Parse(std::string& type, std::string& nonce, std::string& status, std::string& token,
        unsigned& fieldCount)
    {
        SkipSpace();
        if (!Take('{')) return false;
        SkipSpace();
        if (Take('}')) return AtEnd();
        for (;;) {
            std::string key, value;
            if (!ParseString(key)) return false;
            SkipSpace();
            if (!Take(':')) return false;
            SkipSpace();
            if (!ParseString(value)) return false;
            ++fieldCount;
            if (key == "type") { if (seenType_) return false; seenType_ = true; type = std::move(value); }
            else if (key == "nonce") { if (seenNonce_) return false; seenNonce_ = true; nonce = std::move(value); }
            else if (key == "status") { if (seenStatus_) return false; seenStatus_ = true; status = std::move(value); }
            else if (key == "token") { if (seenToken_) return false; seenToken_ = true; token = std::move(value); }
            else return false;
            SkipSpace();
            if (Take('}')) return AtEnd();
            if (!Take(',')) return false;
            SkipSpace();
        }
    }

private:
    bool AtEnd() noexcept { SkipSpace(); return position_ == source_.size(); }
    void SkipSpace() noexcept
    {
        while (position_ < source_.size() && (source_[position_] == ' ' || source_[position_] == '\t' ||
            source_[position_] == '\r' || source_[position_] == '\n')) ++position_;
    }
    bool Take(char ch) noexcept
    {
        if (position_ >= source_.size() || source_[position_] != ch) return false;
        ++position_;
        return true;
    }
    static int Hex(char ch) noexcept
    {
        if (ch >= '0' && ch <= '9') return ch - '0';
        if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
        if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
        return -1;
    }
    bool ParseString(std::string& result)
    {
        if (!Take('"')) return false;
        while (position_ < source_.size()) {
            const unsigned char ch = static_cast<unsigned char>(source_[position_++]);
            if (ch == '"') return true;
            if (ch < 0x20 || ch >= 0x80) return false;
            if (ch != '\\') { result.push_back(static_cast<char>(ch)); continue; }
            if (position_ >= source_.size()) return false;
            const char escaped = source_[position_++];
            switch (escaped) {
            case '"': result.push_back('"'); break;
            case '\\': result.push_back('\\'); break;
            case '/': result.push_back('/'); break;
            case 'b': result.push_back('\b'); break;
            case 'f': result.push_back('\f'); break;
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case 't': result.push_back('\t'); break;
            case 'u': {
                if (source_.size() - position_ < 4) return false;
                unsigned code = 0;
                for (int i = 0; i < 4; ++i) {
                    const int digit = Hex(source_[position_++]);
                    if (digit < 0) return false;
                    code = code * 16u + static_cast<unsigned>(digit);
                }
                // The protocol is deliberately ASCII only. Escaped controls are
                // permitted by JSON but rejected by every field contract below.
                if (code > 0x7f) return false;
                result.push_back(static_cast<char>(code));
                break;
            }
            default: return false;
            }
        }
        return false;
    }

    std::string_view source_;
    size_t position_ = 0;
    bool seenType_ = false, seenNonce_ = false, seenStatus_ = false, seenToken_ = false;
};

std::wstring Wide(const wchar_t* value)
{
    return value == nullptr ? std::wstring{} : std::wstring(value);
}

std::wstring NewNonce()
{
    std::array<unsigned char, 24> bytes{};
    if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
        return {};
    constexpr wchar_t alphabet[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::wstring result;
    result.reserve(32);
    uint32_t accumulator = 0;
    unsigned bits = 0;
    for (const unsigned char byte : bytes) {
        accumulator = (accumulator << 8u) | byte;
        bits += 8;
        while (bits >= 6) {
            bits -= 6;
            result.push_back(alphabet[(accumulator >> bits) & 63u]);
        }
    }
    if (bits != 0) result.push_back(alphabet[(accumulator << (6u - bits)) & 63u]);
    SecureZeroMemory(bytes.data(), bytes.size());
    return result;
}

std::string NarrowAscii(std::wstring_view value)
{
    std::string result;
    result.reserve(value.size());
    for (wchar_t ch : value) {
        if (ch < 0 || ch > 0x7f) return {};
        result.push_back(static_cast<char>(ch));
    }
    return result;
}

std::wstring GetWebViewUserDataPath()
{
    const DWORD required = GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0);
    if (required < 4 || required > 32767) return {};
    std::vector<wchar_t> buffer(required);
    const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer.data(), required);
    if (length == 0 || length >= required) return {};
    std::wstring root(buffer.data(), length);
    auto ensureDirectory = [](const std::wstring& path) noexcept {
        DWORD attributes = GetFileAttributesW(path.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES) {
            if (!CreateDirectoryW(path.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
            attributes = GetFileAttributesW(path.c_str());
        }
        return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
            (attributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0;
    };
    if (!ensureDirectory(root)) return {};
    for (const wchar_t* component : {L"Samfa12", L"HordeLanternRT", L"ReportVerification"}) {
        if (!root.empty() && root.back() != L'\\') root.push_back(L'\\');
        root += component;
        if (!ensureDirectory(root)) return {};
    }
    return root;
}

struct Session;
LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

struct Session final : std::enable_shared_from_this<Session>
{
    HWND owner = nullptr;
    HWND window = nullptr;
    HWND status = nullptr;
    HWND cancel = nullptr;
    ComPtr<ICoreWebView2Environment> environment;
    ComPtr<ICoreWebView2Controller> controller;
    ComPtr<ICoreWebView2> webview;
    std::wstring nonceWide;
    std::string nonce;
    std::wstring userDataPath;
    WindowsPlaytestVerificationResult result;
    WindowsPlaytestVerificationOneShot oneShot;
    bool terminal = false;
    bool finishPosted = false;
    bool comInitialized = false;
    bool priorOwnerEnabled = false;
    bool pageNavigationStarted = false;
    uint64_t generation = 0;
    uint64_t startedAt = 0;
    UINT dpi = 96;

    [[nodiscard]] bool IsTerminalOrQueued() const noexcept { return terminal || finishPosted; }

    [[nodiscard]] bool DeadlineExpired() const noexcept
    {
        return GetTickCount64() - startedAt >= kWindowsPlaytestVerificationDeadlineMs;
    }

    void CloseOnUiThread() noexcept
    {
        if (window && IsWindow(window)) KillTimer(window, kDeadlineTimer);
        if (webview) { webview->Stop(); webview.Reset(); }
        if (controller) { controller->Close(); controller.Reset(); }
        environment.Reset();
        if (window && IsWindow(window)) DestroyWindow(window);
        window = nullptr;
    }

    void Finish(WindowsPlaytestVerificationStatus outcome, std::string token = {}) noexcept
    {
        if (terminal) return;
        if (finishPosted) { CompletePostedFinish(); return; }
        if (DeadlineExpired()) {
            if (!token.empty()) SecureZeroMemory(token.data(), token.size());
            token.clear();
            outcome = WindowsPlaytestVerificationStatus::Deadline;
        }
        terminal = true;
        result.status = outcome;
        result.token = std::move(token);
        CloseOnUiThread();
    }

    void QueueFinish(WindowsPlaytestVerificationStatus outcome, std::string token = {}) noexcept
    {
        if (IsTerminalOrQueued()) {
            if (!token.empty()) SecureZeroMemory(token.data(), token.size());
            return;
        }
        if (DeadlineExpired()) {
            if (!token.empty()) SecureZeroMemory(token.data(), token.size());
            token.clear();
            outcome = WindowsPlaytestVerificationStatus::Deadline;
        }
        finishPosted = true;
        result.status = outcome;
        result.token = std::move(token);
        if (window && IsWindow(window)) {
            PostMessageW(window, kFinishVerificationMessage, static_cast<WPARAM>(generation), 0);
        }
    }

    void CompletePostedFinish() noexcept
    {
        if (!finishPosted || terminal) return;
        finishPosted = false;
        if (DeadlineExpired()) {
            if (!result.token.empty()) SecureZeroMemory(result.token.data(), result.token.size());
            result.token.clear();
            result.status = WindowsPlaytestVerificationStatus::Deadline;
        }
        terminal = true;
        CloseOnUiThread();
    }

    void SafeUnavailable() noexcept { QueueFinish(WindowsPlaytestVerificationStatus::Unavailable); }

    HRESULT StartEnvironment()
    {
        userDataPath = GetWebViewUserDataPath();
        if (userDataPath.empty()) return E_FAIL;
        auto weak = weak_from_this();
        return CreateCoreWebView2EnvironmentWithOptions(nullptr, userDataPath.c_str(), nullptr,
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [weak, expectedGeneration = generation](HRESULT error, ICoreWebView2Environment* created) -> HRESULT {
                    auto self = weak.lock();
                    if (!self || self->generation != expectedGeneration || self->IsTerminalOrQueued() || !self->window || !IsWindow(self->window))
                        return S_OK;
                    if (FAILED(error) || created == nullptr) { self->SafeUnavailable(); return S_OK; }
                    self->environment = created;
                    self->result.stage = WindowsPlaytestVerificationStage::EnvironmentReady;
                    ComPtr<ICoreWebView2Environment10> privateEnvironment;
                    ComPtr<ICoreWebView2ControllerOptions> options;
                    if (FAILED(self->environment.As(&privateEnvironment)) ||
                        FAILED(privateEnvironment->CreateCoreWebView2ControllerOptions(&options)) ||
                        FAILED(options->put_IsInPrivateModeEnabled(TRUE)) ||
                        FAILED(options->put_ProfileName(kEphemeralProfileName))) {
                        self->SafeUnavailable(); return S_OK;
                    }
                    const HRESULT controllerStart = privateEnvironment->CreateCoreWebView2ControllerWithOptions(self->window, options.Get(),
                        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                            [weak, expectedGeneration](HRESULT controllerError, ICoreWebView2Controller* createdController) -> HRESULT {
                                auto current = weak.lock();
                                if (!current || current->generation != expectedGeneration || current->IsTerminalOrQueued() ||
                                    !current->window || !IsWindow(current->window)) return S_OK;
                                if (FAILED(controllerError) || !createdController) { current->SafeUnavailable(); return S_OK; }
                                current->OnController(createdController);
                                return S_OK;
                            }).Get());
                    if (FAILED(controllerStart)) self->SafeUnavailable();
                    return S_OK;
                }).Get());
    }

    void OnController(ICoreWebView2Controller* createdController)
    {
        controller = createdController;
        if (FAILED(controller->get_CoreWebView2(&webview)) || !webview) { SafeUnavailable(); return; }
        ComPtr<ICoreWebView2_13> webview13;
        ComPtr<ICoreWebView2Profile> profile;
        BOOL inPrivate = FALSE;
        if (FAILED(webview.As(&webview13)) || FAILED(webview13->get_Profile(&profile)) || !profile ||
            FAILED(profile->get_IsInPrivateModeEnabled(&inPrivate)) || !inPrivate) {
            SafeUnavailable(); return;
        }
        RECT bounds{};
        GetClientRect(window, &bounds);
        bounds.top = MulDiv(44, static_cast<int>(dpi), 96);
        bounds.bottom -= MulDiv(54, static_cast<int>(dpi), 96);
        if (FAILED(controller->put_Bounds(bounds)) || FAILED(controller->put_IsVisible(TRUE))) { SafeUnavailable(); return; }
        auto weak = weak_from_this();
        if (FAILED(controller->add_AcceleratorKeyPressed(
            Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
                [weak](ICoreWebView2Controller*, ICoreWebView2AcceleratorKeyPressedEventArgs* args) -> HRESULT {
                    UINT key = 0;
                    if (FAILED(args->get_VirtualKey(&key))) return S_OK;
                    if (key == VK_ESCAPE) {
                        args->put_Handled(TRUE);
                        if (auto self = weak.lock(); self && !self->IsTerminalOrQueued())
                            self->QueueFinish(WindowsPlaytestVerificationStatus::Cancelled);
                    }
                    return S_OK;
                }).Get(), &acceleratorToken))) { SafeUnavailable(); return; }
        if (FAILED(ConfigureWebView())) { SafeUnavailable(); return; }
        result.stage = WindowsPlaytestVerificationStage::InPrivateControllerReady;
        static constexpr wchar_t filePickerBlocker[] =
            L"(()=>{const p=HTMLInputElement.prototype,c=p.click,s=p.showPicker;"
            L"Object.defineProperty(p,'click',{value:function(){if(String(this.type).toLowerCase()==='file')return;return c.apply(this,arguments)},configurable:false,writable:false});"
            L"Object.defineProperty(p,'showPicker',{value:function(){if(String(this.type).toLowerCase()==='file')return;if(s)return s.apply(this,arguments)},configurable:false,writable:false});"
            L"document.addEventListener('click',e=>{const t=e.target;if(t instanceof HTMLInputElement&&String(t.type).toLowerCase()==='file'){e.preventDefault();e.stopImmediatePropagation()}},true)})();";
        const HRESULT script = webview->AddScriptToExecuteOnDocumentCreated(filePickerBlocker,
            Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                [weak, expectedGeneration = generation](HRESULT error, LPCWSTR) -> HRESULT {
                    auto self = weak.lock();
                    if (!self || self->generation != expectedGeneration || self->IsTerminalOrQueued() || !self->webview) return S_OK;
                    if (FAILED(error) || FAILED(self->webview->Navigate(kWindowsPlaytestVerificationPageUrl)))
                        self->QueueFinish(WindowsPlaytestVerificationStatus::Unavailable);
                    return S_OK;
                }).Get());
        if (FAILED(script)) SafeUnavailable();
    }

    HRESULT ConfigureWebView()
    {
        ComPtr<ICoreWebView2Settings> settings;
        if (FAILED(webview->get_Settings(&settings)) || !settings) return E_FAIL;
        if (FAILED(settings->put_AreDevToolsEnabled(FALSE)) ||
            FAILED(settings->put_AreDefaultContextMenusEnabled(FALSE)) ||
            FAILED(settings->put_AreDefaultScriptDialogsEnabled(FALSE)) ||
            FAILED(settings->put_AreHostObjectsAllowed(FALSE)) ||
            FAILED(settings->put_IsStatusBarEnabled(FALSE)) ||
            FAILED(settings->put_IsWebMessageEnabled(TRUE)) ||
            FAILED(settings->put_IsScriptEnabled(TRUE))) return E_FAIL;
        ComPtr<ICoreWebView2Settings4> settings4;
        if (FAILED(settings.As(&settings4)) ||
            FAILED(settings4->put_IsPasswordAutosaveEnabled(FALSE)) ||
            FAILED(settings4->put_IsGeneralAutofillEnabled(FALSE))) return E_FAIL;

        auto weak = weak_from_this();
        if (FAILED(webview->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
            [weak](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                LPWSTR uri = nullptr;
                if (FAILED(args->get_Uri(&uri))) { args->put_Cancel(TRUE); return S_OK; }
                const bool allowed = IsAllowedWindowsPlaytestVerificationPageUrl(Wide(uri));
                CoTaskMemFree(uri);
                if (!allowed) args->put_Cancel(TRUE);
                else if (auto self = weak.lock()) {
                    self->pageNavigationStarted = true;
                    self->result.stage = WindowsPlaytestVerificationStage::PageNavigationStarted;
                }
                return S_OK;
            }).Get(), &navigationToken))) return E_FAIL;
        if (FAILED(webview->add_FrameNavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
            [](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                LPWSTR uri = nullptr;
                if (FAILED(args->get_Uri(&uri))) { args->put_Cancel(TRUE); return S_OK; }
                const bool allowed = IsAllowedWindowsPlaytestVerificationChallengeUrl(Wide(uri));
                CoTaskMemFree(uri);
                if (!allowed) args->put_Cancel(TRUE);
                return S_OK;
            }).Get(), &frameNavigationToken))) return E_FAIL;
        if (FAILED(webview->add_NavigationCompleted(Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [weak](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                auto self = weak.lock();
                if (!self || self->IsTerminalOrQueued() || !self->pageNavigationStarted) return S_OK;
                BOOL success = FALSE;
                LPWSTR source = nullptr;
                if (FAILED(args->get_IsSuccess(&success)) || !success || FAILED(sender->get_Source(&source)) ||
                    !IsAllowedWindowsPlaytestVerificationPageUrl(Wide(source))) {
                    CoTaskMemFree(source);
                    self->QueueFinish(WindowsPlaytestVerificationStatus::Failed);
                    return S_OK;
                }
                CoTaskMemFree(source);
                // A plain HTTP error page has no bridge script. Do not mistake
                // successful navigation to a 503 body for an active challenge.
                ComPtr<ICoreWebView2NavigationCompletedEventArgs2> http;
                INT httpStatus = 0;
                if (FAILED(args->QueryInterface(IID_PPV_ARGS(&http))) ||
                    FAILED(http->get_HttpStatusCode(&httpStatus)) || httpStatus != 200)
                {
                    self->QueueFinish(WindowsPlaytestVerificationStatus::Unavailable);
                    return S_OK;
                }
                self->result.stage = WindowsPlaytestVerificationStage::PageLoaded;
                if (self->status && IsWindow(self->status))
                    SetWindowTextW(self->status, L"Complete the anti-spam check below. Your report stays in the game.");
                self->PostHandshake();
                return S_OK;
            }).Get(), &navigationCompletedToken))) return E_FAIL;
        if (FAILED(webview->add_SourceChanged(Callback<ICoreWebView2SourceChangedEventHandler>(
            [weak](ICoreWebView2* sender, ICoreWebView2SourceChangedEventArgs*) -> HRESULT {
                auto self = weak.lock();
                if (!self || self->IsTerminalOrQueued() || !self->pageNavigationStarted) return S_OK;
                LPWSTR source = nullptr;
                const bool allowed = SUCCEEDED(sender->get_Source(&source)) &&
                    IsAllowedWindowsPlaytestVerificationPageUrl(Wide(source));
                CoTaskMemFree(source);
                if (!allowed) self->QueueFinish(WindowsPlaytestVerificationStatus::Failed);
                return S_OK;
            }).Get(), &sourceChangedToken))) return E_FAIL;
        if (FAILED(webview->add_ProcessFailed(Callback<ICoreWebView2ProcessFailedEventHandler>(
            [weak](ICoreWebView2*, ICoreWebView2ProcessFailedEventArgs*) -> HRESULT {
                if (auto self = weak.lock(); self && !self->IsTerminalOrQueued())
                    self->QueueFinish(WindowsPlaytestVerificationStatus::Failed);
                return S_OK;
            }).Get(), &processFailedToken))) return E_FAIL;
        if (FAILED(webview->add_NewWindowRequested(Callback<ICoreWebView2NewWindowRequestedEventHandler>(
            [](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
                args->put_Handled(TRUE); return S_OK;
            }).Get(), &newWindowToken))) return E_FAIL;
        if (FAILED(webview->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [weak](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                auto self = weak.lock();
                if (!self || self->IsTerminalOrQueued()) return S_OK;
                LPWSTR eventSource = nullptr, currentSource = nullptr, raw = nullptr;
                const HRESULT sourceResult = args->get_Source(&eventSource);
                const HRESULT currentResult = sender->get_Source(&currentSource);
                const HRESULT messageResult = args->get_WebMessageAsJson(&raw);
                if (FAILED(sourceResult) || FAILED(currentResult) || FAILED(messageResult) ||
                    !IsAllowedWindowsPlaytestVerificationPageUrl(Wide(eventSource)) ||
                    !IsAllowedWindowsPlaytestVerificationPageUrl(Wide(currentSource))) {
                    CoTaskMemFree(eventSource); CoTaskMemFree(currentSource);
                    if (raw) { SecureZeroMemory(raw, wcslen(raw) * sizeof(wchar_t)); CoTaskMemFree(raw); }
                    return S_OK;
                }
                const std::wstring_view rawView(raw ? raw : L"");
                if (rawView.size() > kWindowsPlaytestVerificationMaxMessageBytes) {
                    CoTaskMemFree(eventSource); CoTaskMemFree(currentSource);
                    if (raw) { SecureZeroMemory(raw, wcslen(raw) * sizeof(wchar_t)); CoTaskMemFree(raw); }
                    return S_OK;
                }
                std::string json = NarrowAscii(rawView);
                CoTaskMemFree(eventSource); CoTaskMemFree(currentSource);
                if (raw) { SecureZeroMemory(raw, wcslen(raw) * sizeof(wchar_t)); CoTaskMemFree(raw); }
                auto reply = ParseWindowsPlaytestVerificationMessage(json, self->nonce);
                SecureZeroMemory(json.data(), json.size());
                WindowsPlaytestVerificationResult accepted;
                const bool completed = self->oneShot.TryComplete(reply, accepted);
                if (!reply.token.empty()) SecureZeroMemory(reply.token.data(), reply.token.size());
                if (completed) {
                    self->result.stage = WindowsPlaytestVerificationStage::ReplyReceived;
                    self->QueueFinish(accepted.status, std::move(accepted.token));
                }
                return S_OK;
            }).Get(), &webMessageToken))) return E_FAIL;

        ComPtr<ICoreWebView2_4> webviewEvents;
        if (FAILED(webview.As(&webviewEvents)) || FAILED(webviewEvents->add_DownloadStarting(
            Callback<ICoreWebView2DownloadStartingEventHandler>(
                [](ICoreWebView2*, ICoreWebView2DownloadStartingEventArgs* args) -> HRESULT {
                    args->put_Cancel(TRUE); return S_OK;
                }).Get(), &downloadToken))) return E_FAIL;

        ComPtr<ICoreWebView2_11> webview11;
        if (FAILED(webview.As(&webview11)) || FAILED(webview11->add_PermissionRequested(
            Callback<ICoreWebView2PermissionRequestedEventHandler>(
                [](ICoreWebView2*, ICoreWebView2PermissionRequestedEventArgs* args) -> HRESULT {
                    args->put_State(COREWEBVIEW2_PERMISSION_STATE_DENY); return S_OK;
                }).Get(), &permissionToken))) return E_FAIL;

        ComPtr<ICoreWebView2_14> webview14;
        if (FAILED(webview.As(&webview14)) || FAILED(webview14->add_ServerCertificateErrorDetected(
            Callback<ICoreWebView2ServerCertificateErrorDetectedEventHandler>(
                [weak](ICoreWebView2*, ICoreWebView2ServerCertificateErrorDetectedEventArgs* args) -> HRESULT {
                    args->put_Action(COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_CANCEL);
                    if (auto self = weak.lock(); self && !self->IsTerminalOrQueued()) self->QueueFinish(WindowsPlaytestVerificationStatus::Failed);
                    return S_OK;
                }).Get(), &certificateToken))) return E_FAIL;
        ComPtr<ICoreWebView2_10> webview10;
        if (FAILED(webview.As(&webview10)) || FAILED(webview10->add_BasicAuthenticationRequested(
            Callback<ICoreWebView2BasicAuthenticationRequestedEventHandler>(
                [](ICoreWebView2*, ICoreWebView2BasicAuthenticationRequestedEventArgs* args) -> HRESULT {
                    args->put_Cancel(TRUE); return S_OK;
                }).Get(), &authToken))) return E_FAIL;
        ComPtr<ICoreWebView2_5> webview5;
        if (FAILED(webview.As(&webview5)) || FAILED(webview5->add_ClientCertificateRequested(
            Callback<ICoreWebView2ClientCertificateRequestedEventHandler>(
                [](ICoreWebView2*, ICoreWebView2ClientCertificateRequestedEventArgs* args) -> HRESULT {
                    args->put_Cancel(TRUE); return S_OK;
                }).Get(), &clientCertificateToken))) return E_FAIL;

        // Catch every network resource so only the exact page and Cloudflare
        // challenge origin can leave the WebView. The page itself has no fetch
        // endpoint for report data and receives only the nonce handshake.
        if (FAILED(webview->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL)) ||
            FAILED(webview->add_WebResourceRequested(Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                [weak](ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT {
                    ComPtr<ICoreWebView2WebResourceRequest> request;
                    LPWSTR uri = nullptr;
                    bool allowed = false;
                    if (SUCCEEDED(args->get_Request(&request)) && request && SUCCEEDED(request->get_Uri(&uri))) {
                        const std::wstring value = Wide(uri);
                        allowed = IsAllowedWindowsPlaytestVerificationPageUrl(value) ||
                            IsAllowedWindowsPlaytestVerificationChallengeUrl(value);
                    }
                    CoTaskMemFree(uri);
                    if (allowed) return S_OK;
                    auto self = weak.lock();
                    if (!self || self->IsTerminalOrQueued() || !self->environment) return S_OK;
                    ComPtr<ICoreWebView2WebResourceResponse> blocked;
                    if (FAILED(self->environment->CreateWebResourceResponse(nullptr, 403, L"Blocked",
                            L"Content-Type: text/plain\r\n", &blocked)) || !blocked ||
                        FAILED(args->put_Response(blocked.Get())))
                        self->QueueFinish(WindowsPlaytestVerificationStatus::Failed);
                    return S_OK;
                }).Get(), &resourceToken))) return E_FAIL;
        return S_OK;
    }

    void PostHandshake()
    {
        if (IsTerminalOrQueued() || !webview || handshakeSent) return;
        LPWSTR source = nullptr;
        if (FAILED(webview->get_Source(&source)) || !IsAllowedWindowsPlaytestVerificationPageUrl(Wide(source))) {
            CoTaskMemFree(source); QueueFinish(WindowsPlaytestVerificationStatus::Failed); return;
        }
        CoTaskMemFree(source);
        const std::wstring message = L"{\"type\":\"horde-report-init\",\"nonce\":\"" + nonceWide + L"\"}";
        const HRESULT posted = webview->PostWebMessageAsJson(message.c_str());
        if (FAILED(posted)) QueueFinish(WindowsPlaytestVerificationStatus::Failed);
        else {
            handshakeSent = true;
            result.stage = WindowsPlaytestVerificationStage::HandshakePosted;
        }
    }

    void Resize()
    {
        if (!window || !controller) return;
        RECT bounds{};
        GetClientRect(window, &bounds);
        const LONG margin = MulDiv(16, static_cast<int>(dpi), 96);
        const LONG top = MulDiv(52, static_cast<int>(dpi), 96);
        const LONG bottom = MulDiv(58, static_cast<int>(dpi), 96);
        bounds.top = top;
        bounds.bottom = (std::max)(top, bounds.bottom - bottom);
        controller->put_Bounds(bounds);
        if (status) MoveWindow(status, margin, MulDiv(10, static_cast<int>(dpi), 96),
            (std::max)(0L, bounds.right - margin * 2), top - MulDiv(12, static_cast<int>(dpi), 96), TRUE);
        if (cancel) MoveWindow(cancel, (std::max)(margin, bounds.right - MulDiv(100, static_cast<int>(dpi), 96)),
            bounds.bottom + MulDiv(12, static_cast<int>(dpi), 96), MulDiv(84, static_cast<int>(dpi), 96),
            MulDiv(30, static_cast<int>(dpi), 96), TRUE);
    }

    void CreateWindowAndStart()
    {
        nonceWide = NewNonce();
        nonce = NarrowAscii(nonceWide);
        if (nonce.empty()) { result.status = WindowsPlaytestVerificationStatus::Unavailable; return; }
        static std::atomic<uint64_t> nextGeneration{1};
        generation = nextGeneration.fetch_add(1, std::memory_order_relaxed);
        startedAt = GetTickCount64();
        WNDCLASSEXW klass{sizeof(klass)};
        klass.lpfnWndProc = WindowProc;
        klass.hInstance = GetModuleHandleW(nullptr);
        klass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        klass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        klass.lpszClassName = kWindowClass;
        RegisterClassExW(&klass);
        const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
        RECT rect{0, 0, MulDiv(620, 96, 96), MulDiv(610, 96, 96)};
        AdjustWindowRectEx(&rect, style, FALSE, WS_EX_DLGMODALFRAME);
        window = CreateWindowExW(WS_EX_DLGMODALFRAME, kWindowClass, L"Verify report submission", style,
            CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
            owner, nullptr, GetModuleHandleW(nullptr), this);
        if (!window) { result.status = WindowsPlaytestVerificationStatus::Unavailable; return; }
        dpi = GetDpiForWindow(window);
        if (dpi == 0) dpi = 96;
        SetWindowPos(window, nullptr, 0, 0, MulDiv(620, static_cast<int>(dpi), 96),
            MulDiv(610, static_cast<int>(dpi), 96), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        status = CreateWindowExW(0, L"STATIC",
            L"Opening private verification. Your report and optional screenshot stay in the game.",
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX, 18, 12, 570, 42, window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(kStatusLabel)), GetModuleHandleW(nullptr), nullptr);
        cancel = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            500, 560, 90, 30, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCancelButton)),
            GetModuleHandleW(nullptr), nullptr);
        if (!status || !cancel) { Finish(WindowsPlaytestVerificationStatus::Unavailable); return; }
        if (owner && IsWindow(owner)) {
            priorOwnerEnabled = IsWindowEnabled(owner) != FALSE;
            EnableWindow(owner, FALSE);
        }
        ShowWindow(window, SW_SHOW);
        UpdateWindow(window);
        SetFocus(cancel);
        const ULONGLONG elapsed = GetTickCount64() - startedAt;
        const UINT remaining = elapsed >= kWindowsPlaytestVerificationDeadlineMs ? 1u :
            static_cast<UINT>(kWindowsPlaytestVerificationDeadlineMs - elapsed);
        if (SetTimer(window, kDeadlineTimer, remaining, nullptr) == 0) {
            Finish(WindowsPlaytestVerificationStatus::Unavailable);
            return;
        }
        result.stage = WindowsPlaytestVerificationStage::ModalReady;
        if (FAILED(StartEnvironment())) Finish(WindowsPlaytestVerificationStatus::Unavailable);
    }

    void OnDestroyed() noexcept
    {
        if (window && IsWindow(window)) KillTimer(window, kDeadlineTimer);
        window = nullptr;
        if (!terminal) {
            if (!finishPosted) result.status = WindowsPlaytestVerificationStatus::Cancelled;
            terminal = true;
            finishPosted = false;
        }
        if (webview) { webview->Stop(); webview.Reset(); }
        if (controller) { controller->Close(); controller.Reset(); }
        environment.Reset();
    }

    EventRegistrationToken navigationToken{}, frameNavigationToken{}, frameCreatedToken{},
        navigationCompletedToken{}, sourceChangedToken{}, processFailedToken{}, webMessageToken{}, downloadToken{}, permissionToken{},
        certificateToken{}, authToken{}, clientCertificateToken{}, resourceToken{}, newWindowToken{}, acceleratorToken{};
    bool handshakeSent = false;
};

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* self = reinterpret_cast<Session*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        self = static_cast<Session*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->window = window;
    }
    if (self == nullptr) return DefWindowProcW(window, message, wParam, lParam);
    switch (message) {
    case WM_COMMAND:
        if (LOWORD(wParam) == kCancelButton) { self->Finish(WindowsPlaytestVerificationStatus::Cancelled); return 0; }
        break;
    case WM_CLOSE:
        self->Finish(WindowsPlaytestVerificationStatus::Cancelled); return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) { self->Finish(WindowsPlaytestVerificationStatus::Cancelled); return 0; }
        break;
    case WM_TIMER:
        if (wParam == kDeadlineTimer) { self->Finish(WindowsPlaytestVerificationStatus::Deadline); return 0; }
        break;
    case kFinishVerificationMessage:
        if (wParam == static_cast<WPARAM>(self->generation)) self->CompletePostedFinish();
        return 0;
    case WM_SIZE: self->Resize(); return 0;
    case WM_DPICHANGED: {
        self->dpi = HIWORD(wParam);
        const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
        SetWindowPos(window, nullptr, suggested->left, suggested->top,
            suggested->right - suggested->left, suggested->bottom - suggested->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        self->Resize(); return 0;
    }
    case WM_NCDESTROY:
        SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        self->OnDestroyed();
        return DefWindowProcW(window, message, wParam, lParam);
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

} // namespace

bool IsAllowedWindowsPlaytestVerificationPageUrl(std::wstring_view url) noexcept
{
    return url == kWindowsPlaytestVerificationPageUrl;
}

bool IsAllowedWindowsPlaytestVerificationChallengeUrl(std::wstring_view url) noexcept
{
    constexpr std::wstring_view prefix = L"https://";
    if (url.size() < prefix.size() + 1 || _wcsnicmp(url.data(), prefix.data(), prefix.size()) != 0) return false;
    const size_t authorityStart = prefix.size();
    size_t authorityEnd = url.find_first_of(L"/?#", authorityStart);
    if (authorityEnd == std::wstring_view::npos) authorityEnd = url.size();
    const auto authority = url.substr(authorityStart, authorityEnd - authorityStart);
    constexpr std::wstring_view host = kWindowsPlaytestVerificationChallengeHost;
    if (authority.size() < host.size() || _wcsnicmp(authority.data(), host.data(), host.size()) != 0) return false;
    if (authority.size() == host.size()) return url.find(L'#', authorityEnd) == std::wstring_view::npos;
    if (authority[host.size()] != L':') return false;
    if (authority.substr(host.size() + 1) != L"443") return false;
    return url.find(L'#', authorityEnd) == std::wstring_view::npos;
}

std::string_view WindowsPlaytestVerificationStageName(const WindowsPlaytestVerificationStage stage) noexcept
{
    switch (stage) {
    case WindowsPlaytestVerificationStage::NotStarted: return "not-started";
    case WindowsPlaytestVerificationStage::ModalReady: return "modal-ready";
    case WindowsPlaytestVerificationStage::EnvironmentReady: return "environment-ready";
    case WindowsPlaytestVerificationStage::InPrivateControllerReady: return "inprivate-controller-ready";
    case WindowsPlaytestVerificationStage::PageNavigationStarted: return "page-navigation-started";
    case WindowsPlaytestVerificationStage::PageLoaded: return "page-loaded";
    case WindowsPlaytestVerificationStage::HandshakePosted: return "handshake-posted";
    case WindowsPlaytestVerificationStage::ReplyReceived: return "reply-received";
    }
    return "unknown";
}

WindowsPlaytestVerificationReply ParseWindowsPlaytestVerificationMessage(
    std::string_view json, std::string_view expectedNonce) noexcept
{
    WindowsPlaytestVerificationReply result;
    if (json.size() > kWindowsPlaytestVerificationMaxMessageBytes || !IsNonce(expectedNonce)) return result;
    try {
        std::string type, nonce, status, token;
        unsigned fields = 0;
        JsonFlatObjectParser parser(json);
        if (!parser.Parse(type, nonce, status, token, fields) ||
            type != "horde-report-verification" || nonce != expectedNonce) return result;
        if (status == "verified" && fields == 4 && IsAsciiToken(token)) {
            result.recognized = true;
            result.verified = true;
            result.token = std::move(token);
        } else if (status == "failed" && fields == 3 && token.empty()) {
            result.recognized = true;
        }
    } catch (...) { return {}; }
    return result;
}

bool WindowsPlaytestVerificationOneShot::TryComplete(const WindowsPlaytestVerificationReply& reply,
    WindowsPlaytestVerificationResult& result) noexcept
{
    if (terminal_ || !reply.recognized) return false;
    try {
        result.token = reply.verified ? reply.token : std::string{};
        result.status = reply.verified ? WindowsPlaytestVerificationStatus::Verified :
            WindowsPlaytestVerificationStatus::Failed;
    } catch (...) {
        result.token.clear();
        result.status = WindowsPlaytestVerificationStatus::Unavailable;
    }
    terminal_ = true;
    return true;
}

WindowsPlaytestVerificationResult ShowWindowsPlaytestVerification(HWND owner)
{
    WindowsPlaytestVerificationResult unavailable;
    if (owner && (!IsWindow(owner) || GetCurrentThreadId() != GetWindowThreadProcessId(owner, nullptr))) return unavailable;
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com)) return unavailable;
    auto session = std::make_shared<Session>();
    session->owner = owner;
    session->comInitialized = SUCCEEDED(com);
    session->CreateWindowAndStart();
    while (session->window && IsWindow(session->window) && !session->terminal) {
        MSG message{};
        const BOOL got = GetMessageW(&message, nullptr, 0, 0);
        if (got <= 0) {
            const bool quit = got == 0;
            const int quitCode = static_cast<int>(message.wParam);
            session->Finish(WindowsPlaytestVerificationStatus::Cancelled);
            if (quit) PostQuitMessage(quitCode);
            break;
        }
        if (session->window && IsDialogMessageW(session->window, &message)) continue;
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    if (session->window && IsWindow(session->window)) session->Finish(WindowsPlaytestVerificationStatus::Cancelled);
    if (owner && IsWindow(owner) && session->priorOwnerEnabled) { EnableWindow(owner, TRUE); SetActiveWindow(owner); }
    auto result = std::move(session->result);
    if (session->comInitialized) CoUninitialize();
    return result;
}
} // namespace horde::platform::windows
