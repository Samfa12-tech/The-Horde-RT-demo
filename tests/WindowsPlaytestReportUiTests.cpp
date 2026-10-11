#include "platform/windows/WindowsPlaytestReport.h"

#include <atomic>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <wct.h>
#include <dbghelp.h>

namespace
{
std::atomic<bool> passed{true};
std::atomic<int> exports{0};
std::atomic<const char*> currentPhase{"startup"};
std::vector<std::string> approved;
bool acceptExport = false; // Accessed only by the UI owner thread.
std::atomic<ULONGLONG> noteEditStarted{0};
std::atomic<bool> noteEditExecuting{false};
std::atomic<ULONGLONG> noteEditElapsed{0};
bool exerciseTimeoutDiagnostic = false;

LRESULT CALLBACK TraceNoteEntry(int code, WPARAM value, LPARAM argument)
{
    if (code >= 0) {
        const auto& message = *reinterpret_cast<CWPSTRUCT*>(argument);
        if (message.message == WM_SETTEXT && GetDlgCtrlID(message.hwnd) == 106) {
            noteEditStarted.store(GetTickCount64());
            noteEditElapsed.store(0);
            noteEditExecuting.store(true);
            if (exerciseTimeoutDiagnostic &&
                std::strcmp(currentPhase.load(), "note-only approved Unicode note edit") == 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(2200));
        }
    }
    return CallNextHookEx(nullptr, code, value, argument);
}

LRESULT CALLBACK TraceNoteReturn(int code, WPARAM value, LPARAM argument)
{
    if (code >= 0) {
        const auto& message = *reinterpret_cast<CWPRETSTRUCT*>(argument);
        if (message.message == WM_SETTEXT && GetDlgCtrlID(message.hwnd) == 106) {
            const auto elapsed = GetTickCount64() - noteEditStarted.load();
            noteEditExecuting.store(false);
            noteEditElapsed.store(elapsed);
        }
    }
    return CallNextHookEx(nullptr, code, value, argument);
}

#if defined(_M_X64)
std::vector<unsigned char> ownerStack;
DWORD64 ownerStackBase = 0;
BOOL CALLBACK ReadCapturedStack(HANDLE process, DWORD64 address, PVOID output, DWORD bytes, LPDWORD read)
{
    if (address >= ownerStackBase && address - ownerStackBase <= ownerStack.size() &&
        bytes <= ownerStack.size() - static_cast<std::size_t>(address - ownerStackBase)) {
        std::memcpy(output, ownerStack.data() + (address - ownerStackBase), bytes);
        *read = bytes; return TRUE;
    }
    SIZE_T count = 0;
    const BOOL result = ReadProcessMemory(process, reinterpret_cast<void*>(address), output, bytes, &count);
    *read = static_cast<DWORD>(count); return result;
}
void TraceOwnerStack(DWORD thread)
{
    HANDLE owner = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, thread);
    if (!owner) return;
    CONTEXT context{}; context.ContextFlags = CONTEXT_FULL;
    bool captured = false;
    // Snapshot only the synthetic fixture's UI stack. Resume before symbol
    // lookup/output so a loader or stream lock can never deadlock its owner.
    ownerStack.resize(64u * 1024u);
    if (SuspendThread(owner) != DWORD(-1)) {
        if (GetThreadContext(owner, &context)) {
            MEMORY_BASIC_INFORMATION region{};
            if (VirtualQuery(reinterpret_cast<void*>(context.Rsp), &region, sizeof(region))) {
                const auto end = reinterpret_cast<ULONG_PTR>(region.BaseAddress) + region.RegionSize;
                const auto size = std::min<std::size_t>(ownerStack.size(), end - context.Rsp);
                SIZE_T read = 0;
                captured = ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(context.Rsp),
                                              ownerStack.data(), size, &read) != FALSE;
                ownerStack.resize(read); ownerStackBase = context.Rsp;
            }
        }
        ResumeThread(owner);
    }
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_NO_PROMPTS);
    if (captured && SymInitialize(GetCurrentProcess(), "", TRUE)) {
        STACKFRAME64 frame{};
        frame.AddrPC = {context.Rip, 0, AddrModeFlat};
        frame.AddrStack = {context.Rsp, 0, AddrModeFlat};
        frame.AddrFrame = {context.Rbp, 0, AddrModeFlat};
        for (int depth = 0; depth < 24; ++depth) {
            alignas(SYMBOL_INFO) unsigned char storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME]{};
            auto* symbol = reinterpret_cast<SYMBOL_INFO*>(storage);
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO); symbol->MaxNameLen = MAX_SYM_NAME;
            DWORD64 displacement = 0;
            const bool named = SymFromAddr(GetCurrentProcess(), frame.AddrPC.Offset, &displacement, symbol) != FALSE;
            IMAGEHLP_MODULE64 module{}; module.SizeOfStruct = sizeof(module);
            const bool hasModule = SymGetModuleInfo64(GetCurrentProcess(), frame.AddrPC.Offset, &module) != FALSE;
            std::cerr << "UI stack " << depth << ": " << (hasModule ? module.ModuleName : "unknown-module")
                      << '!' << (named ? symbol->Name : "unresolved") << "+0x" << std::hex << displacement
                      << " address=0x" << frame.AddrPC.Offset << std::dec << '\n';
            if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, GetCurrentProcess(), owner, &frame, &context,
                             ReadCapturedStack, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) break;
        }
        SymCleanup(GetCurrentProcess());
    }
    CloseHandle(owner); ownerStack.clear();
}
#else
void TraceOwnerStack(DWORD) {}
#endif

void TraceOwnerWait(DWORD thread)
{
    std::cerr << "note EDIT executing=" << noteEditExecuting.load() << '\n';
    const HWCT session = OpenThreadWaitChainSession(0, nullptr);
    if (!session) { std::cerr << "wait-chain session error=" << GetLastError() << '\n'; return; }
    WAITCHAIN_NODE_INFO nodes[WCT_MAX_NODE_COUNT]{};
    DWORD count = WCT_MAX_NODE_COUNT;
    BOOL cycle = FALSE;
    if (GetThreadWaitChain(session, 0, WCTP_GETINFO_ALL_FLAGS, thread, &count, nodes, &cycle)) {
        std::cerr << "UI wait-chain nodes=" << count << " cycle=" << cycle << '\n';
        for (DWORD i = 0; i < count; ++i)
            std::cerr << "  node=" << i << " type=" << nodes[i].ObjectType
                      << " status=" << nodes[i].ObjectStatus << '\n';
    } else std::cerr << "wait-chain query error=" << GetLastError() << '\n';
    // Deliberately omit object names, note text and private process metadata.
    CloseThreadWaitChainSession(session);
}

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
    const DWORD senderThread = GetCurrentThreadId();
    const DWORD ownerThread = GetWindowThreadProcessId(window, nullptr);
    const auto started = std::chrono::steady_clock::now();
    DWORD_PTR result = 0;
    if (SendMessageTimeoutW(window, message, value, argument, SMTO_ABORTIFHUNG, 2000, &result) == 0)
    {
        const DWORD error = GetLastError();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count();
        // Query state only: never read note text, send another control message,
        // retry the timed-out operation or change the UI owner's focus.
        GUITHREADINFO info{};
        info.cbSize = sizeof(info);
        const bool hasGuiInfo = ownerThread != 0 && GetGUIThreadInfo(ownerThread, &info) != FALSE;
        const DWORD guiInfoError = ownerThread == 0 ? ERROR_INVALID_THREAD_ID :
            hasGuiInfo ? ERROR_SUCCESS : GetLastError();
        std::cerr << "native control message timed out; phase=" << currentPhase.load()
                  << " hwnd=" << window << " controlId=" << GetDlgCtrlID(window)
                  << " message=0x" << std::hex << message << std::dec
                  << " error=" << error << " senderThread=" << senderThread
                  << " ownerThread=" << ownerThread << " elapsedMs=" << elapsed
                  << " enabled=" << (IsWindowEnabled(window) != FALSE)
                  << " visible=" << (IsWindowVisible(window) != FALSE)
                  << " guiInfoAvailable=" << hasGuiInfo << " guiInfoError=" << guiInfoError;
        if (hasGuiInfo)
        {
            std::cerr << " guiFlags=0x" << std::hex << info.flags << std::dec
                      << " active=" << info.hwndActive << " focus=" << info.hwndFocus
                      << " capture=" << info.hwndCapture;
        }
        std::cerr << '\n';
        TraceOwnerStack(ownerThread);
        TraceOwnerWait(ownerThread);
        passed.store(false);
    }
    if (message == WM_SETTEXT && GetDlgCtrlID(window) == 106)
        std::cerr << "note EDIT trace; phase=" << currentPhase.load()
                  << " returned=" << !noteEditExecuting.load()
                  << " elapsedMs=" << noteEditElapsed.load() << '\n';
    return static_cast<LRESULT>(result);
}

void ClickExport(HWND form) { Send(form, WM_COMMAND, MAKEWPARAM(111, BN_CLICKED)); }
void SetNote(HWND form, const wchar_t* text, const char* phase)
{
    SetPhase(phase);
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

int main(int argc, char** argv)
{
    if (argc > 1) {
        if (argc != 2 || std::strcmp(argv[1], "--exercise-timeout-diagnostic") != 0) return 2;
        exerciseTimeoutDiagnostic = true; // Explicit failing diagnostic, never enabled by normal CTest.
    }
    SetProcessDPIAware();
    const HWND owner = CreateWindowExW(0, L"STATIC", L"Horde report UI test owner", WS_OVERLAPPEDWINDOW,
        0, 0, 800, 800, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!owner) return 1;
    const DWORD thread = GetCurrentThreadId();
    const HHOOK entryTrace = SetWindowsHookExW(WH_CALLWNDPROC, TraceNoteEntry, nullptr, thread);
    const HHOOK returnTrace = SetWindowsHookExW(WH_CALLWNDPROCRET, TraceNoteReturn, nullptr, thread);
    Check(entryTrace != nullptr && returnTrace != nullptr, "note UI timing trace hooks unavailable");
    std::thread actions([thread] {
        SetPhase("note-only default and private-content checks");
        const HWND form = WaitForForm(thread);
        Defaults(form, false);
        if (!form) return;
        SetNote(form, L"Steps to reproduce", "note-only initial note edit");
        ClickExport(form);
        Check(exports.load() == 0, "category/impact or consent omission reached destination");
        Send(GetDlgItem(form, 102), CB_SETCURSEL, 1);
        Send(GetDlgItem(form, 104), CB_SETCURSEL, 2);
        ClickExport(form);
        Check(exports.load() == 0, "unchecked export consent reached destination");
        Send(GetDlgItem(form, 107), BM_SETCHECK, BST_CHECKED);
        SetNote(form, L"test@example.invalid", "note-only rejected-input note edit");
        ClickExport(form);
        Check(exports.load() == 0, "private-content rejection reached destination");
        SetNote(form, L"Walk.\r\nThen parry. \u9f8d \U0001f525", "note-only approved Unicode note edit");
        SetPhase("note-only approved export and retry");
        ClickExport(form);
        Check(exports.load() == 1, "approved note-only export did not reach injected destination once");
        Check(!IsWindowEnabled(GetDlgItem(form, 106)), "approved note did not freeze for retry");
        // Even programmatically changing a disabled edit cannot change frozen JSON.
        SetNote(form, L"later mutation", "note-only frozen retry edit");
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
        SetNote(form, L"Explicit context test", "explicit-context note edit");
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
    if (entryTrace) UnhookWindowsHookEx(entryTrace);
    if (returnTrace) UnhookWindowsHookEx(returnTrace);
    return passed.load() ? 0 : 1;
}
