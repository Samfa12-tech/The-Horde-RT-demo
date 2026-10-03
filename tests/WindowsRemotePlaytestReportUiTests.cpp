#include "platform/windows/WindowsRemotePlaytestReport.h"

#include <atomic>
#include <chrono>
#include <cwchar>
#include <iostream>
#include <iterator>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace
{
using namespace horde::platform::windows;
using namespace horde::reporting;
constexpr wchar_t kDialogClass[] = L"HordeLanternRemotePlaytestReportDialog";
std::atomic_bool passed{true};
HWND FindChildControl(HWND parent, int controlId);
HWND FindBodyViewport(HWND parent);
HWND FindBodyContent(HWND parent);

void Check(const bool condition, const char* message)
{
    if (!condition) { passed.store(false); std::cerr << "remote playtest report UI: " << message << '\n'; }
}

// Simpler bounded lookup: the target dialog has a unique class and only one
// instance is opened by each test phase.
HWND WaitForForm(const DWORD thread)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    do
    {
        HWND found = nullptr;
        EnumThreadWindows(thread, [](HWND window, LPARAM result) -> BOOL {
            wchar_t name[96]{};
            GetClassNameW(window, name, static_cast<int>(std::size(name)));
            if (wcscmp(name, kDialogClass) == 0)
            {
                *reinterpret_cast<HWND*>(result) = window;
                return FALSE;
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&found));
        GUITHREADINFO gui{}; gui.cbSize = sizeof(gui);
        if (found && IsWindowVisible(found) && FindChildControl(found, remote_playtest_control::Send) &&
            GetGUIThreadInfo(thread, &gui) != FALSE &&
            gui.hwndFocus == FindChildControl(found, remote_playtest_control::Category)) return found;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (std::chrono::steady_clock::now() < deadline);
    return nullptr;
}

HWND WaitForPrompt(const DWORD thread)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    do
    {
        HWND found = nullptr;
        EnumThreadWindows(thread, [](HWND window, LPARAM result) -> BOOL {
            wchar_t name[96]{};
            GetClassNameW(window, name, static_cast<int>(std::size(name)));
            if (wcscmp(name, L"#32770") == 0)
            {
                *reinterpret_cast<HWND*>(result) = window;
                return FALSE;
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&found));
        if (found && IsWindowVisible(found)) return found;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (std::chrono::steady_clock::now() < deadline);
    return nullptr;
}

LRESULT Send(HWND window, const UINT message, const WPARAM value = 0, const LPARAM argument = 0)
{
    DWORD_PTR result = 0;
    if (window == nullptr || SendMessageTimeoutW(window, message, value, argument,
        SMTO_ABORTIFHUNG, 2000u, &result) == 0)
    {
        Check(false, "native control message timed out");
        return 0;
    }
    return static_cast<LRESULT>(result);
}

HWND FindChildControl(HWND parent, const int controlId)
{
    if (HWND direct = ::GetDlgItem(parent, controlId)) return direct;
    struct Search { int id; HWND found; } search{controlId, nullptr};
    EnumChildWindows(parent, [](HWND child, LPARAM data) -> BOOL {
        auto& search = *reinterpret_cast<Search*>(data);
        if (GetDlgCtrlID(child) == search.id)
        {
            search.found = child;
            return FALSE;
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&search));
    return search.found;
}

HWND FindBodyViewport(HWND parent)
{
    HWND content = FindBodyContent(parent);
    return content ? GetParent(content) : nullptr;
}

HWND FindBodyContent(HWND parent)
{
    HWND content = nullptr;
    EnumChildWindows(parent, [](HWND child, LPARAM output) -> BOOL {
        wchar_t className[96]{};
        GetClassNameW(child, className, static_cast<int>(std::size(className)));
        if (wcscmp(className, L"HordeLanternRemotePlaytestBodyContent") == 0)
        {
            *reinterpret_cast<HWND*>(output) = child;
            return FALSE;
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&content));
    return content;
}

void FillDraft(HWND form, const bool consent, const bool screenshot = false)
{
    Send(FindChildControl(form, remote_playtest_control::Category), CB_SETCURSEL, 1u);
    Send(FindChildControl(form, remote_playtest_control::Impact), CB_SETCURSEL, 2u);
    Send(FindChildControl(form, remote_playtest_control::Note), WM_SETTEXT, 0,
        reinterpret_cast<LPARAM>(L"The torch clips the doorway. Walk, then parry."));
    if (consent) Send(FindChildControl(form, remote_playtest_control::SubmitConsent), BM_SETCHECK, BST_CHECKED);
    if (screenshot) Send(FindChildControl(form, remote_playtest_control::IncludeScreenshot), BM_SETCHECK, BST_CHECKED);
}

template <typename Predicate>
bool WaitFor(Predicate predicate, const std::chrono::milliseconds timeout = std::chrono::seconds(4))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (predicate()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return predicate();
}

std::string StripFreshToken(const std::string& request)
{
    constexpr std::string_view suffix = ",\"turnstileToken\":\"";
    const auto token = request.rfind(suffix);
    if (token == std::string::npos || request.empty() || request.back() != '}') return {};
    const auto end = request.find('"', token + suffix.size());
    if (end == std::string::npos || end + 1u != request.size() - 1u) return {};
    return request.substr(0u, token) + "}";
}

std::string ReportIdFromRequest(const std::string_view request)
{
    constexpr std::string_view key = "\"reportId\":\"";
    const auto begin = request.find(key);
    if (begin == std::string_view::npos) return {};
    const auto value = begin + key.size();
    const auto end = request.find('"', value);
    return end == std::string_view::npos ? std::string{} : std::string(request.substr(value, end - value));
}

WindowsPlaytestReportContext TestContext()
{
    WindowsPlaytestReportContext context;
    context.available = true;
    context.values = {"Horde Lantern RT", "1.6.1", "fixture", "Windows", "Windows desktop",
        "Fixture GPU", "RayTracingPipeline", "High", 0.75, 960u, 540u, true};
    return context;
}
} // namespace

int main()
{
    const RECT previewBox{0, 0, 300, 120};
    const RECT landscape = FitWindowsPlaytestPreviewRect(previewBox, 2u, 1u);
    const RECT portrait = FitWindowsPlaytestPreviewRect(previewBox, 1u, 2u);
    Check(landscape.left == 30 && landscape.top == 0 && landscape.right == 270 && landscape.bottom == 120 &&
        portrait.left == 120 && portrait.top == 0 && portrait.right == 180 && portrait.bottom == 120,
        "preview fits landscape and portrait game frames without stretching aspect ratio");
    SetProcessDPIAware();
    const HWND owner = CreateWindowExW(0, L"STATIC", L"Remote report UI test owner", WS_OVERLAPPEDWINDOW,
        0, 0, 900, 900, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!owner) return 1;
    const DWORD thread = GetCurrentThreadId();

    // Consent-off blocks all network/verification/capture work. The explicitly
    // selected offline JSON export remains local and does not require network consent.
    std::atomic_uint verifyCalls{0u}, exchangeCalls{0u}, captureCalls{0u}, offlineCalls{0u};
    std::mutex savedMutex;
    std::string savedOffline;
    WindowsRemotePlaytestServices offlineServices;
    offlineServices.verifyForeground = [&](HWND) {
        ++verifyCalls;
        return WindowsPlaytestVerificationResult{WindowsPlaytestVerificationStatus::Verified, WindowsPlaytestVerificationStage::ReplyReceived, "never-used"};
    };
    offlineServices.exchange = [&](const std::string_view, const auto, const HANDLE) {
        ++exchangeCalls;
        return WindowsPlaytestHttpResponse{};
    };
    offlineServices.exportOfflineJson = [&](HWND, const std::string& json, std::wstring& message) {
        { std::lock_guard lock(savedMutex); savedOffline = json; }
        ++offlineCalls;
        message = L"Synthetic local save";
        return true;
    };
    std::thread offlineActions([&] {
        HWND form = WaitForForm(thread);
        Check(form != nullptr, "offline fallback form appears");
        if (!form) return;
        Check(Send(FindChildControl(form, remote_playtest_control::SubmitConsent), BM_GETCHECK) == BST_UNCHECKED &&
            Send(FindChildControl(form, remote_playtest_control::IncludeDiagnostics), BM_GETCHECK) == BST_UNCHECKED &&
            Send(FindChildControl(form, remote_playtest_control::IncludeScreenshot), BM_GETCHECK) == BST_UNCHECKED,
            "remote consent and optional data default off");
        Check(!IsWindowEnabled(FindChildControl(form, remote_playtest_control::IncludeScreenshot)),
            "missing game-frame capture service disables screenshot opt-in");
        std::this_thread::sleep_for(std::chrono::milliseconds(180));
        Check(!IsWindowEnabled(FindChildControl(form, remote_playtest_control::IncludeScreenshot)),
            "idle completion polling does not re-enable an unavailable screenshot service");
        SetWindowPos(form, nullptr, 0, 0, 420, 370, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        RECT suggested{}; GetWindowRect(form, &suggested);
        Send(form, WM_DPICHANGED, MAKELONG(144, 144), reinterpret_cast<LPARAM>(&suggested));
        SCROLLINFO scroll{}; scroll.cbSize = sizeof(scroll); scroll.fMask = SIF_PAGE | SIF_RANGE;
        GetScrollInfo(form, SB_VERT, &scroll);
        Check(scroll.nMax + 1 > static_cast<int>(scroll.nPage),
            "small native window retains a scrollable report body");
        HWND viewport = FindBodyViewport(form);
        HWND body = FindBodyContent(form);
        const HWND category = FindChildControl(form, remote_playtest_control::Category);
        const HWND impact = FindChildControl(form, remote_playtest_control::Impact);
        const HWND note = FindChildControl(form, remote_playtest_control::Note);
        const HWND consentControl = FindChildControl(form, remote_playtest_control::SubmitConsent);
        const HWND nextCategory = GetNextDlgTabItem(body, category, FALSE);
        const HWND nextImpact = GetNextDlgTabItem(body, impact, FALSE);
        const HWND nextNote = GetNextDlgTabItem(body, note, FALSE);
        Check(nextCategory == impact && nextImpact == note && nextNote == consentControl,
            "dialog tab order traverses nested report controls");
        Send(form, WM_VSCROLL, SB_TOP);
        RECT previewBefore{}, viewportBefore{};
        GetWindowRect(FindChildControl(form, remote_playtest_control::Preview), &previewBefore);
        GetWindowRect(FindBodyViewport(form), &viewportBefore);
        Check(previewBefore.bottom > viewportBefore.bottom,
            "preview starts clipped in the compact high-DPI window");
        RECT consent{}, viewportRect{}, close{};
        GetWindowRect(consentControl, &consent);
        if (viewport) GetWindowRect(viewport, &viewportRect);
        Check(consent.top < viewportRect.top || consent.bottom > viewportRect.bottom,
            "consent begins clipped before keyboard focus navigation");
        Send(form, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(note), TRUE);
        Check(WaitFor([&] {
            GUITHREADINFO gui{}; gui.cbSize = sizeof(gui);
            return GetGUIThreadInfo(thread, &gui) != FALSE && gui.hwndFocus == note;
        }), "focus can enter nested note editor");
        PostMessageW(note, WM_KEYDOWN, VK_TAB, 0);
        const bool reachedConsent = WaitFor([&] {
            GUITHREADINFO gui{}; gui.cbSize = sizeof(gui);
            return GetGUIThreadInfo(thread, &gui) != FALSE && gui.hwndFocus == consentControl;
        });
        Check(reachedConsent, "nested keyboard tab navigation reaches remote consent");
        GetWindowRect(consentControl, &consent);
        if (viewport) GetWindowRect(viewport, &viewportRect);
        GetWindowRect(FindChildControl(form, remote_playtest_control::Close), &close);
        RECT formRect{}; GetWindowRect(form, &formRect);
        Check(viewport && consent.top >= viewportRect.top && consent.bottom <= viewportRect.bottom,
            "keyboard focus scrolls the full remote-consent control into the viewport");
        const HWND sendButton = FindChildControl(form, remote_playtest_control::Send);
        const HWND offlineButton = FindChildControl(form, remote_playtest_control::OfflineExport);
        const HWND diagnosticsControl = FindChildControl(form, remote_playtest_control::IncludeDiagnostics);
        Send(form, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(diagnosticsControl), TRUE);
        PostMessageW(diagnosticsControl, WM_KEYDOWN, VK_TAB, 0);
        Check(WaitFor([&] {
            GUITHREADINFO gui{}; gui.cbSize = sizeof(gui);
            return GetGUIThreadInfo(thread, &gui) != FALSE && gui.hwndFocus == sendButton;
        }), "Tab crosses from last enabled body control to sticky Prepare action");
        PostMessageW(sendButton, WM_KEYDOWN, VK_TAB, 0);
        Check(WaitFor([&] {
            GUITHREADINFO gui{}; gui.cbSize = sizeof(gui);
            return GetGUIThreadInfo(thread, &gui) != FALSE && gui.hwndFocus == offlineButton;
        }), "forward whole-form tab order skips disabled Retry action");
        Send(form, WM_NEXTDLGCTL, TRUE, 0);
        Check(WaitFor([&] {
            GUITHREADINFO gui{}; gui.cbSize = sizeof(gui);
            return GetGUIThreadInfo(thread, &gui) != FALSE && gui.hwndFocus == sendButton;
        }), "reverse whole-form traversal returns from Offline to Prepare and skips disabled Retry");
        Check(close.left >= formRect.left && close.right <= formRect.right &&
            close.top >= formRect.top && close.bottom <= formRect.bottom,
            "sticky close action remains inside the resized window");
        FillDraft(form, false);
        Send(form, WM_COMMAND, MAKEWPARAM(remote_playtest_control::Send, BN_CLICKED));
        Check(verifyCalls.load() == 0u && exchangeCalls.load() == 0u && captureCalls.load() == 0u,
            "unchecked consent prevents verification, capture and network exchange");
        PostMessageW(form, WM_COMMAND, MAKEWPARAM(remote_playtest_control::OfflineExport, BN_CLICKED), 0);
        HWND prompt = WaitForPrompt(thread);
        Check(prompt != nullptr, "offline save shows explicit local-only warning");
        if (prompt) Send(prompt, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED));
        Check(WaitFor([&] { return offlineCalls.load() == 1u; }), "injected offline export is invoked after local confirmation");
        Send(form, WM_COMMAND, MAKEWPARAM(remote_playtest_control::NewReport, BN_CLICKED));
        Check(!IsWindowEnabled(FindChildControl(form, remote_playtest_control::IncludeScreenshot)),
            "new-draft reset preserves disabled screenshot capability when service is absent");
        PostMessageW(form, WM_COMMAND, MAKEWPARAM(remote_playtest_control::Close, BN_CLICKED), 0);
    });
    ShowWindowsRemotePlaytestReport(owner, TestContext(), std::move(offlineServices));
    offlineActions.join();
    {
        std::lock_guard lock(savedMutex);
        Check(savedOffline.find("\"reportId\"") != std::string::npos &&
            savedOffline.find("\"product\":\"horde-lantern-rt\"") == std::string::npos &&
            savedOffline.find("\"turnstileToken\"") == std::string::npos,
            "offline file is the local report schema, without remote envelope or verification token");
    }
    Check(verifyCalls.load() == 0u && exchangeCalls.load() == 0u && captureCalls.load() == 0u,
        "offline fallback never verifies or sends");

    // Bad game-frame readback is rejected after opt-in and validation, before
    // verification or exchange; an image is never silently omitted.
    std::atomic_uint imageVerifications{0u}, imageExchanges{0u};
    std::thread imageActions([&] {
        HWND form = WaitForForm(thread);
        Check(form != nullptr, "screenshot validation form appears");
        if (!form) return;
        FillDraft(form, true, true);
        Send(form, WM_COMMAND, MAKEWPARAM(remote_playtest_control::Send, BN_CLICKED));
        Check(captureCalls.load() == 1u && imageVerifications.load() == 0u && imageExchanges.load() == 0u,
            "opted-in RT capture occurs before verification and failed capture never reaches relay");
        PostMessageW(form, WM_COMMAND, MAKEWPARAM(remote_playtest_control::Close, BN_CLICKED), 0);
    });
    WindowsRemotePlaytestServices imageServices;
    imageServices.captureGameFrame = [&](HWND, WindowsPlaytestReportContext& captureContext,
        PlaytestScreenshotPixels& pixels, std::wstring&) {
        ++captureCalls;
        captureContext = TestContext();
        pixels.width = 0u; // Deliberately invalid synthetic frame.
        pixels.height = 0u;
        return true;
    };
    imageServices.verifyForeground = [&](HWND) {
        ++imageVerifications;
        return WindowsPlaytestVerificationResult{WindowsPlaytestVerificationStatus::Verified, WindowsPlaytestVerificationStage::ReplyReceived, "not-used"};
    };
    imageServices.exchange = [&](const std::string_view, const auto, const HANDLE) {
        ++imageExchanges;
        return WindowsPlaytestHttpResponse{};
    };
    ShowWindowsRemotePlaytestReport(owner, TestContext(), std::move(imageServices));
    imageActions.join();

    // A foreground attempt freezes the payload before exchange. Editing is
    // disabled in flight; a 403 requires an explicit fresh-token same-ID retry.
    std::atomic_uint verifySequence{0u}, exchangeSequence{0u};
    HANDLE firstExchangeStarted = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HANDLE releaseFirstExchange = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    std::mutex requestsMutex;
    std::vector<std::string> requests;
    WindowsRemotePlaytestServices retryServices;
    retryServices.verifyForeground = [&](HWND) {
        const unsigned call = ++verifySequence;
        if (call == 1u)
            return WindowsPlaytestVerificationResult{WindowsPlaytestVerificationStatus::Failed,
                WindowsPlaytestVerificationStage::ReplyReceived, {}};
        return WindowsPlaytestVerificationResult{WindowsPlaytestVerificationStatus::Verified,
            WindowsPlaytestVerificationStage::ReplyReceived,
            call == 2u ? "fresh-verification-one" : "fresh-verification-two"};
    };
    retryServices.exchange = [&](const std::string_view request, const auto, const HANDLE) {
        const unsigned call = ++exchangeSequence;
        { std::lock_guard lock(requestsMutex); requests.emplace_back(request); }
        if (call == 1u)
        {
            SetEvent(firstExchangeStarted);
            (void)WaitForSingleObject(releaseFirstExchange, 3000u);
            return WindowsPlaytestHttpResponse{403, "{}"};
        }
        const std::string id = ReportIdFromRequest(request);
        return WindowsPlaytestHttpResponse{202,
            "{\"ok\":true,\"id\":\"" + id + "\",\"status\":\"accepted\"}"};
    };
    retryServices.captureGameFrame = [](HWND, WindowsPlaytestReportContext& captureContext,
        PlaytestScreenshotPixels& pixels, std::wstring&) {
        captureContext = TestContext();
        pixels.width = 2u;
        pixels.height = 2u;
        pixels.rgba = {12u, 20u, 32u, 255u, 24u, 36u, 48u, 255u,
            36u, 48u, 60u, 255u, 48u, 60u, 72u, 255u};
        return true;
    };
    std::thread retryActions([&] {
        HWND form = WaitForForm(thread);
        Check(form != nullptr, "retry form appears");
        if (!form) return;
        SetWindowPos(form, nullptr, 0, 0, 420, 370, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        RECT suggested{}; GetWindowRect(form, &suggested);
        Send(form, WM_DPICHANGED, MAKELONG(144, 144), reinterpret_cast<LPARAM>(&suggested));
        Send(form, WM_VSCROLL, SB_BOTTOM);
        FillDraft(form, true, true);
        Send(form, WM_COMMAND, MAKEWPARAM(remote_playtest_control::Send, BN_CLICKED));
        wchar_t preparedStatus[240]{};
        GetWindowTextW(FindChildControl(form, remote_playtest_control::Status), preparedStatus, 240);
        RECT preview{}, viewport{};
        GetWindowRect(FindChildControl(form, remote_playtest_control::Preview), &preview);
        GetWindowRect(FindBodyViewport(form), &viewport);
        Check(wcsstr(preparedStatus, L"preview are ready") != nullptr &&
            preview.top >= viewport.top && preview.bottom <= viewport.bottom &&
            verifySequence.load() == 0u && exchangeSequence.load() == 0u,
            "first action scrolls the frozen game-frame preview fully into view without verification or exchange");
        Send(form, WM_COMMAND, MAKEWPARAM(remote_playtest_control::Send, BN_CLICKED));
        Check(WaitFor([&] { return IsWindowEnabled(FindChildControl(form, remote_playtest_control::Send)) != FALSE; }) &&
            exchangeSequence.load() == 0u,
            "failed foreground verification returns to an explicit Verify action without sending");
        Send(form, WM_COMMAND, MAKEWPARAM(remote_playtest_control::Send, BN_CLICKED));
        Check(WaitForSingleObject(firstExchangeStarted, 2000u) == WAIT_OBJECT_0,
            "second explicit action starts exchange after foreground verification");
        Check(!IsWindowEnabled(FindChildControl(form, remote_playtest_control::Note)) &&
            !IsWindowEnabled(FindChildControl(form, remote_playtest_control::Send)),
            "note and new-send actions are disabled while request is in flight");
        Send(FindChildControl(form, remote_playtest_control::Note), WM_SETTEXT, 0,
            reinterpret_cast<LPARAM>(L"post-freeze mutation"));
        SetEvent(releaseFirstExchange);
        Check(WaitFor([&] { return IsWindowEnabled(FindChildControl(form, remote_playtest_control::Retry)) != FALSE; }),
            "retry appears only after the first attempt resolves as retryable");
        Send(form, WM_COMMAND, MAKEWPARAM(remote_playtest_control::Retry, BN_CLICKED));
        Check(WaitFor([&] { return exchangeSequence.load() == 2u; }), "explicit retry starts a second exchange");
        Check(WaitFor([&] {
            wchar_t text[160]{};
            GetWindowTextW(FindChildControl(form, remote_playtest_control::Status), text, 160);
            return wcsstr(text, L"durably queued") != nullptr;
        }), "202 is described as durable queue acceptance, not delivery");
        PostMessageW(form, WM_COMMAND, MAKEWPARAM(remote_playtest_control::Close, BN_CLICKED), 0);
    });
    ShowWindowsRemotePlaytestReport(owner, TestContext(), std::move(retryServices));
    retryActions.join();
    Check(verifySequence.load() == 3u && exchangeSequence.load() == 2u,
        "each actual explicit attempt gets fresh verification and only verified attempts exchange");
    {
        std::lock_guard lock(requestsMutex);
        Check(requests.size() == 2u && StripFreshToken(requests[0]) == StripFreshToken(requests[1]) &&
            StripFreshToken(requests[0]).find("The torch clips the doorway") != std::string::npos &&
            StripFreshToken(requests[0]).find("post-freeze mutation") == std::string::npos,
            "same report ID and exact original payload survive disabled-field mutation on explicit retry");
        Check(requests.size() == 2u && requests[0].find("fresh-verification-one") != std::string::npos &&
            requests[1].find("fresh-verification-two") != std::string::npos,
            "verification tokens are fresh per explicit attempt");
    }
    if (firstExchangeStarted) CloseHandle(firstExchangeStarted);
    if (releaseFirstExchange) CloseHandle(releaseFirstExchange);

    // Closing an in-flight form explicitly cancels its owner; late acceptance
    // cannot call back into the destroyed window or trigger a second attempt.
    HANDLE cancelStarted = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    std::atomic_bool returnedLateAcceptance{false};
    WindowsRemotePlaytestServices cancelServices;
    cancelServices.verifyForeground = [](HWND) {
        return WindowsPlaytestVerificationResult{WindowsPlaytestVerificationStatus::Verified,
            WindowsPlaytestVerificationStage::ReplyReceived, "cancel-test-token"};
    };
    cancelServices.exchange = [&](const std::string_view request, const auto, const HANDLE cancellation) {
        SetEvent(cancelStarted);
        (void)WaitForSingleObject(cancellation, 3000u);
        returnedLateAcceptance.store(true);
        const std::string id = ReportIdFromRequest(request);
        return WindowsPlaytestHttpResponse{202,
            "{\"ok\":true,\"id\":\"" + id + "\",\"status\":\"accepted\"}"};
    };
    std::thread cancelActions([&] {
        HWND form = WaitForForm(thread);
        Check(form != nullptr, "cancel form appears");
        if (!form) return;
        FillDraft(form, true);
        Send(form, WM_COMMAND, MAKEWPARAM(remote_playtest_control::Send, BN_CLICKED));
        Send(form, WM_COMMAND, MAKEWPARAM(remote_playtest_control::Send, BN_CLICKED));
        Check(WaitForSingleObject(cancelStarted, 2000u) == WAIT_OBJECT_0,
            "cancel fixture exchange starts");
        PostMessageW(form, WM_COMMAND, MAKEWPARAM(remote_playtest_control::Close, BN_CLICKED), 0);
        HWND prompt = WaitForPrompt(thread);
        Check(prompt != nullptr, "closing in-flight form warns that delivery cannot be recalled");
        if (prompt) Send(prompt, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED));
    });
    ShowWindowsRemotePlaytestReport(owner, TestContext(), std::move(cancelServices));
    cancelActions.join();
    Check(WaitFor([&] { return returnedLateAcceptance.load(); }),
        "late fake acceptance drains after UI cancellation without a stale window callback");
    Check(IsWindowEnabled(owner) == TRUE, "closing remote form restores its owner");
    if (cancelStarted) CloseHandle(cancelStarted);
    DestroyWindow(owner);
    return passed.load() ? 0 : 1;
}
