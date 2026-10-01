#include "platform/windows/WindowsPlaytestReport.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace
{
std::atomic<bool> passed{true};
std::atomic<int> exports{0};
std::atomic<const char*> currentPhase{"startup"};
std::vector<std::string> approved;
bool acceptExport = false; // Accessed only by the UI owner thread.

void SetPhase(const char* phase) { currentPhase.store(phase); }

void Check(const bool condition, const char* description)
{
    if (!condition) { passed.store(false); std::cerr << description << '\n'; }
}

bool FakeDestination(HWND, const std::string& bytes, std::wstring& message)
{
    approved.push_back(bytes);
    ++exports;
    message = acceptExport ? L"Test destination accepted" : L"Test destination cancelled";
    return acceptExport;
}

HWND WaitForForm(const DWORD thread)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    do
    {
        HWND found = nullptr;
        EnumThreadWindows(thread, [](HWND window, LPARAM result) -> BOOL {
            wchar_t name[80]{};
            GetClassNameW(window, name, 80);
            if (std::wstring(name) == L"HordeLanternPlaytestReportDialog")
            {
                *reinterpret_cast<HWND*>(result) = window;
                return FALSE;
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&found));
        if (found && GetDlgItem(found, 111))
        {
            // Child creation happens in WM_CREATE, before the caller has shown
            // the dialog and assigned its initial focus. Do not start driving
            // controls until the UI thread has completed that initialization.
            GUITHREADINFO info{};
            info.cbSize = sizeof(info);
            if (IsWindowVisible(found) && GetGUIThreadInfo(thread, &info) != FALSE &&
                info.hwndFocus == GetDlgItem(found, 102)) return found;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (std::chrono::steady_clock::now() < deadline);
    std::cerr << "report form did not become visible with category focus; phase="
              << currentPhase.load() << '\n';
    return nullptr;
}

LRESULT Send(HWND window, const UINT message, const WPARAM value = 0, const LPARAM argument = 0)
{
    DWORD_PTR result = 0;
    if (SendMessageTimeoutW(window, message, value, argument, SMTO_ABORTIFHUNG, 2000, &result) == 0)
    {
        const DWORD error = GetLastError();
        std::cerr << "native control message timed out; phase=" << currentPhase.load()
                  << " hwnd=" << window << " controlId=" << GetDlgCtrlID(window)
                  << " message=0x" << std::hex << message << std::dec
                  << " error=" << error << '\n';
        passed.store(false);
    }
    return static_cast<LRESULT>(result);
}

void ClickExport(HWND form) { Send(form, WM_COMMAND, MAKEWPARAM(111, BN_CLICKED)); }
void SetNote(HWND form, const wchar_t* text)
{
    Send(GetDlgItem(form, 106), WM_SETTEXT, 0, reinterpret_cast<LPARAM>(text));
}

void Defaults(HWND form, const bool hasContext)
{
    Check(form != nullptr, "native form did not open");
    if (!form) return;
    Check(Send(GetDlgItem(form, 107), BM_GETCHECK) == BST_UNCHECKED, "export consent default is not off");
    Check(Send(GetDlgItem(form, 108), BM_GETCHECK) == BST_UNCHECKED, "context consent default is not off");
    Check(IsWindowEnabled(GetDlgItem(form, 108)) == (hasContext ? TRUE : FALSE),
        "unavailable context opt-in must be disabled, never guessed");
    Check(GetDlgItem(form, 102) != GetDlgItem(form, IDCANCEL), "category and Close IDs collide");
}
} // namespace

int main()
{
    SetProcessDPIAware();
    const HWND owner = CreateWindowExW(0, L"STATIC", L"Horde report UI test owner", WS_OVERLAPPEDWINDOW,
        0, 0, 800, 800, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!owner) return 1;
    const DWORD thread = GetCurrentThreadId();
    std::thread actions([thread] {
        SetPhase("note-only default and private-content checks");
        const HWND form = WaitForForm(thread);
        Defaults(form, false);
        if (!form) return;
        SetNote(form, L"Steps to reproduce");
        ClickExport(form);
        Check(exports.load() == 0, "category/impact or consent omission reached destination");
        Send(GetDlgItem(form, 102), CB_SETCURSEL, 1);
        Send(GetDlgItem(form, 104), CB_SETCURSEL, 2);
        ClickExport(form);
        Check(exports.load() == 0, "unchecked export consent reached destination");
        Send(GetDlgItem(form, 107), BM_SETCHECK, BST_CHECKED);
        SetNote(form, L"test@example.invalid");
        ClickExport(form);
        Check(exports.load() == 0, "private-content rejection reached destination");
        SetNote(form, L"Walk.\r\nThen parry. \u9f8d \U0001f525");
        SetPhase("note-only approved export and retry");
        ClickExport(form);
        Check(exports.load() == 1, "approved note-only export did not reach injected destination once");
        Check(!IsWindowEnabled(GetDlgItem(form, 106)), "approved note did not freeze for retry");
        // Even programmatically changing a disabled edit cannot change frozen JSON.
        SetNote(form, L"later mutation");
        ClickExport(form);
        Check(exports.load() == 2, "explicit retry did not reach destination");
        PostMessageW(form, WM_CLOSE, 0, 0);
    });
    horde::platform::windows::ShowWindowsPlaytestReport(owner, {}, FakeDestination);
    actions.join();
    Check(IsWindowEnabled(owner) == TRUE, "closing report did not restore owner window");
    Check(approved.size() == 2 && approved[0] == approved[1], "retry changed report ID or JSON bytes");
    if (approved.size() == 2)
    {
        Check(approved[0].find("\"context\"") == std::string::npos, "note-only export leaked context");
        Check(approved[0].find("\\r\\n") != std::string::npos &&
            approved[0].find("\xe9\xbe\x8d \xf0\x9f\x94\xa5") != std::string::npos,
            "native Unicode/reproduction steps were changed");
    }
    approved.clear(); exports.store(0); acceptExport = true;
    SetPhase("explicit-context form startup");
    horde::platform::windows::WindowsPlaytestReportContext context;
    context.available = true;
    context.values = {"Horde Lantern RT", "1.6.1", "fixture-build", "Windows", "Windows desktop",
        "Fixture GPU", "RayTracingPipeline", "High", 0.75, 960, 540, false};
    std::thread contextActions([thread] {
        SetPhase("explicit-context consent and duplicate-export checks");
        const HWND form = WaitForForm(thread);
        Defaults(form, true);
        if (!form) return;
        Send(GetDlgItem(form, 102), CB_SETCURSEL, 1);
        Send(GetDlgItem(form, 104), CB_SETCURSEL, 2);
        SetNote(form, L"Explicit context test");
        Send(GetDlgItem(form, 107), BM_SETCHECK, BST_CHECKED);
        Send(GetDlgItem(form, 108), BM_SETCHECK, BST_CHECKED);
        ClickExport(form);
        ClickExport(form);
        Check(exports.load() == 1, "accepted export was duplicated");
        PostMessageW(form, WM_CLOSE, 0, 0);
    });
    horde::platform::windows::ShowWindowsPlaytestReport(owner, context, FakeDestination);
    contextActions.join();
    Check(approved.size() == 1 && approved[0].find("\"rtPresented\":false") != std::string::npos &&
        approved[0].find("\"context\"") != std::string::npos,
        "explicit allowlist context or honest false presentation state missing");
    DestroyWindow(owner);
    return passed.load() ? 0 : 1;
}
