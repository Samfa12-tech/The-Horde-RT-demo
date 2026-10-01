#include "platform/windows/WindowsPlaytestReport.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <bcrypt.h>

#include "reporting/PlaytestReport.h"

#include <algorithm>
#include <array>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <utility>

namespace horde::platform::windows
{
namespace
{

constexpr wchar_t kWindowClass[] = L"HordeLanternPlaytestReportDialog";
constexpr int kIntro = 100;
constexpr int kCategoryLabel = 101;
constexpr int kCategoryCombo = 102;
constexpr int kImpactLabel = 103;
constexpr int kImpactCombo = 104;
constexpr int kNoteLabel = 105;
constexpr int kNoteEdit = 106;
constexpr int kConsent = 107;
constexpr int kIncludeContext = 108;
constexpr int kDisclosure = 109;
constexpr int kStatus = 110;
constexpr int kExport = 111;
constexpr int kCancel = IDCANCEL;
constexpr wchar_t kDialogTitle[] = L"Report a problem";
constexpr wchar_t kFontProperty[] = L"HordeLanternPlaytestReportFont";

struct Choice
{
    const wchar_t* label;
    horde::reporting::PlaytestReportCategory category;
};

constexpr std::array<Choice, 6> kCategories{{
    {L"Gameplay", horde::reporting::PlaytestReportCategory::Gameplay},
    {L"Visuals", horde::reporting::PlaytestReportCategory::Visuals},
    {L"Performance", horde::reporting::PlaytestReportCategory::Performance},
    {L"Controls", horde::reporting::PlaytestReportCategory::Controls},
    {L"Audio", horde::reporting::PlaytestReportCategory::Audio},
    {L"Other", horde::reporting::PlaytestReportCategory::Other},
}};

struct ImpactChoice
{
    const wchar_t* label;
    horde::reporting::PlaytestReportImpact impact;
};

constexpr std::array<ImpactChoice, 4> kImpacts{{
    {L"Blocks progress", horde::reporting::PlaytestReportImpact::BlocksProgress},
    {L"Major friction", horde::reporting::PlaytestReportImpact::MajorFriction},
    {L"Minor friction", horde::reporting::PlaytestReportImpact::MinorFriction},
    {L"Polish", horde::reporting::PlaytestReportImpact::Polish},
}};

struct DialogState
{
    WindowsPlaytestReportContext context;
    horde::reporting::PreparedPlaytestReport prepared;
    bool saved = false;
    WindowsPlaytestReportExporter exporter = nullptr;
};

int Scale(HWND window, const int logical)
{
    const UINT dpi = GetDpiForWindow(window);
    return MulDiv(logical, static_cast<int>(dpi == 0u ? 96u : dpi), 96);
}

HWND AddControl(HWND parent, const wchar_t* className, const wchar_t* text,
                const DWORD style, const int id, const DWORD extendedStyle = 0)
{
    HWND control = CreateWindowExW(extendedStyle, className, text, WS_CHILD | WS_VISIBLE | style,
                                   0, 0, 10, 10, parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                   GetModuleHandleW(nullptr), nullptr);
    if (control != nullptr)
    {
        SendMessageW(control, WM_SETFONT,
                     reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
    }
    return control;
}

void Layout(HWND window)
{
    RECT client{};
    GetClientRect(window, &client);
    const int width = client.right;
    const int height = client.bottom;
    const int margin = Scale(window, 18);
    const int gap = Scale(window, 7);
    const int labelHeight = Scale(window, 22);
    const int comboHeight = Scale(window, 30);
    int y = margin;
    if (HWND intro = GetDlgItem(window, kIntro))
        MoveWindow(intro, margin, y, width - margin * 2, Scale(window, 44), TRUE);
    y += Scale(window, 44) + gap;
    if (HWND label = GetDlgItem(window, kCategoryLabel)) MoveWindow(label, margin, y, width - margin * 2, labelHeight, TRUE);
    y += labelHeight;
    if (HWND combo = GetDlgItem(window, kCategoryCombo)) MoveWindow(combo, margin, y, width - margin * 2, comboHeight, TRUE);
    y += comboHeight + gap;
    if (HWND label = GetDlgItem(window, kImpactLabel)) MoveWindow(label, margin, y, width - margin * 2, labelHeight, TRUE);
    y += labelHeight;
    if (HWND combo = GetDlgItem(window, kImpactCombo)) MoveWindow(combo, margin, y, width - margin * 2, comboHeight, TRUE);
    y += comboHeight + gap;
    if (HWND label = GetDlgItem(window, kNoteLabel)) MoveWindow(label, margin, y, width - margin * 2, labelHeight, TRUE);
    y += labelHeight;
    const int footerHeight = Scale(window, 280);
    const int noteHeight = std::max(Scale(window, 100), height - y - footerHeight - margin);
    if (HWND edit = GetDlgItem(window, kNoteEdit)) MoveWindow(edit, margin, y, width - margin * 2, noteHeight, TRUE);
    y += noteHeight + gap;
    if (HWND check = GetDlgItem(window, kConsent)) MoveWindow(check, margin, y, width - margin * 2, Scale(window, 25), TRUE);
    y += Scale(window, 27);
    if (HWND check = GetDlgItem(window, kIncludeContext)) MoveWindow(check, margin, y, width - margin * 2, Scale(window, 25), TRUE);
    y += Scale(window, 28);
    const int disclosureHeight = Scale(window, 96);
    if (HWND disclosure = GetDlgItem(window, kDisclosure))
        MoveWindow(disclosure, margin, y, width - margin * 2, disclosureHeight, TRUE);
    y += disclosureHeight + gap;
    const int buttonHeight = Scale(window, 34);
    const int buttonWidth = Scale(window, 172);
    const int buttonY = height - margin - buttonHeight;
    if (HWND status = GetDlgItem(window, kStatus))
        MoveWindow(status, margin, std::min(y, buttonY - Scale(window, 42)), width - margin * 2,
                   std::max(Scale(window, 24), buttonY - std::min(y, buttonY - Scale(window, 42)) - gap), TRUE);
    if (HWND exportButton = GetDlgItem(window, kExport)) MoveWindow(exportButton, width - margin - buttonWidth * 2 - gap, buttonY, buttonWidth, buttonHeight, TRUE);
    if (HWND cancel = GetDlgItem(window, kCancel)) MoveWindow(cancel, width - margin - buttonWidth, buttonY, buttonWidth, buttonHeight, TRUE);
}

void ApplyFont(HWND window)
{
    if (HFONT old = reinterpret_cast<HFONT>(RemovePropW(window, kFontProperty))) DeleteObject(old);
    HFONT font = CreateFontW(Scale(window, 17), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    if (font == nullptr) font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    else SetPropW(window, kFontProperty, font);
    EnumChildWindows(window, [](HWND child, LPARAM parameter) -> BOOL {
        SendMessageW(child, WM_SETFONT, static_cast<WPARAM>(parameter), TRUE);
        return TRUE;
    }, reinterpret_cast<LPARAM>(font));
}

void SetStatus(HWND window, const std::wstring& value)
{
    if (HWND status = GetDlgItem(window, kStatus)) SetWindowTextW(status, value.c_str());
}

bool ReadUtf8(HWND edit, std::string& result, horde::reporting::PlaytestReportStatus& status)
{
    const int count = GetWindowTextLengthW(edit);
    status = horde::reporting::PlaytestReportStatus::InvalidUtf8;
    if (count < 0) return false;
    if (count > 16384)
    {
        status = horde::reporting::PlaytestReportStatus::NoteTooLarge;
        return false;
    }
    std::wstring wide(static_cast<std::size_t>(count) + 1u, L'\0');
    const int copied = GetWindowTextW(edit, wide.data(), count + 1);
    if (copied != count) return false;
    wide.resize(static_cast<std::size_t>(count));
    static_assert(sizeof(wchar_t) == sizeof(char16_t));
    std::u16string units;
    units.reserve(wide.size());
    for (const wchar_t unit : wide) units.push_back(static_cast<char16_t>(unit));
    status = horde::reporting::EncodePlaytestReportUtf16(
        std::u16string_view(units),
        horde::reporting::kPlaytestReportMaxNoteBytes, result);
    return status == horde::reporting::PlaytestReportStatus::Ready;
}

bool MakeIdentity(std::string& id, std::string& timestamp)
{
    std::array<unsigned char, 16> random{};
    if (BCryptGenRandom(nullptr, random.data(), static_cast<ULONG>(random.size()),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) return false;
    constexpr char hex[] = "0123456789abcdef";
    id.clear();
    id.reserve(random.size() * 2u);
    for (const unsigned char byte : random)
    {
        id.push_back(hex[byte >> 4u]);
        id.push_back(hex[byte & 0x0fu]);
    }
    SYSTEMTIME now{};
    GetSystemTime(&now);
    wchar_t formatted[32]{};
    const int count = swprintf_s(formatted, L"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
                                 now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute,
                                 now.wSecond, now.wMilliseconds);
    if (count <= 0) return false;
    timestamp.clear();
    timestamp.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) timestamp.push_back(static_cast<char>(formatted[i]));
    return true;
}

bool SaveJson(HWND owner, const std::string& json, std::wstring& message)
{
    std::array<wchar_t, 32768> path{};
    wcscpy_s(path.data(), path.size(), L"HordeLanternRT-playtest-report.json");
    constexpr wchar_t filter[] = L"JSON report (*.json)\0*.json\0All files (*.*)\0*.*\0\0";
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = path.data();
    dialog.nMaxFile = static_cast<DWORD>(path.size());
    dialog.lpstrDefExt = L"json";
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&dialog))
    {
        const DWORD error = CommDlgExtendedError();
        if (error == 0u) message = L"Export cancelled. Your prepared report remains ready for another explicit export.";
        else message = L"The file picker failed (Windows dialog error " + std::to_wstring(error) + L").";
        return false;
    }
    std::ofstream stream(std::filesystem::path(path.data()), std::ios::binary | std::ios::trunc);
    if (!stream.good())
    {
        message = L"Could not open the selected file. Choose Export to retry the same report.";
        return false;
    }
    stream.write(json.data(), static_cast<std::streamsize>(json.size()));
    stream.flush();
    if (!stream.good())
    {
        stream.close();
        message = L"Write or flush failed; the selected file may contain a partial report. Choose Export to retry the same report.";
        return false;
    }
    stream.close();
    if (stream.fail())
    {
        message = L"The file close failed; the selected file may contain a partial report. Choose Export to retry the same report.";
        return false;
    }
    message = L"Report saved as UTF-8 JSON (no BOM). The report ID and contents are now fixed.";
    return true;
}

void FreezeInput(HWND window, const bool freeze)
{
    for (const int id : {kCategoryCombo, kImpactCombo, kNoteEdit, kConsent, kIncludeContext})
    {
        if (HWND control = GetDlgItem(window, id)) EnableWindow(control, !freeze);
    }
}

bool Prepare(HWND window, DialogState& state)
{
    std::string note;
    horde::reporting::PlaytestReportStatus encodeStatus{};
    if (!ReadUtf8(GetDlgItem(window, kNoteEdit), note, encodeStatus))
    {
        const std::string_view status = horde::reporting::PlaytestReportStatusName(encodeStatus);
        std::wstring message = L"Note encoding failed: ";
        for (const char ch : status) message.push_back(static_cast<wchar_t>(ch));
        SetStatus(window, message);
        return false;
    }
    std::string id;
    std::string timestamp;
    if (!MakeIdentity(id, timestamp))
    {
        SetStatus(window, L"Windows could not create a secure report ID or UTC timestamp. Nothing was exported.");
        return false;
    }
    const LRESULT categoryIndex = SendMessageW(GetDlgItem(window, kCategoryCombo), CB_GETCURSEL, 0, 0);
    const LRESULT impactIndex = SendMessageW(GetDlgItem(window, kImpactCombo), CB_GETCURSEL, 0, 0);
    if (categoryIndex < 0 || impactIndex < 0 ||
        static_cast<std::size_t>(categoryIndex) >= kCategories.size() ||
        static_cast<std::size_t>(impactIndex) >= kImpacts.size())
    {
        SetStatus(window, L"Choose a category and impact before saving.");
        return false;
    }
    const bool includeContext = SendMessageW(GetDlgItem(window, kIncludeContext), BM_GETCHECK, 0, 0) == BST_CHECKED;
    const bool consent = SendMessageW(GetDlgItem(window, kConsent), BM_GETCHECK, 0, 0) == BST_CHECKED;
    state.prepared = horde::reporting::PreparePlaytestReport({
        id, timestamp, kCategories[static_cast<std::size_t>(categoryIndex)].category,
        kImpacts[static_cast<std::size_t>(impactIndex)].impact, note, consent,
        includeContext, state.context.values.View()});
    if (!state.prepared.IsReady())
    {
        const std::string_view status = horde::reporting::PlaytestReportStatusName(state.prepared.status);
        std::wstring message = L"Report not ready: ";
        for (const char ch : status) message.push_back(static_cast<wchar_t>(ch));
        SetStatus(window, message);
        return false;
    }
    FreezeInput(window, true);
    return true;
}

void Export(HWND window, DialogState& state)
{
    if (state.saved)
    {
        SetStatus(window, L"This report has already been saved. Close the form when you are done.");
        return;
    }
    if (!state.prepared.IsReady() && !Prepare(window, state)) return;
    std::wstring message;
    state.saved = (state.exporter ? state.exporter : SaveJson)(window, state.prepared.json, message);
    message += L" Report ID: ";
    for (const char ch : state.prepared.reportId) message.push_back(static_cast<wchar_t>(ch));
    SetStatus(window, message);
}

void CreateControls(HWND window, DialogState& state)
{
    AddControl(window, L"STATIC", L"This report stays on your PC until you choose Export local report. Nothing is sent.",
               SS_LEFT | SS_NOPREFIX, kIntro);
    AddControl(window, L"STATIC", L"Category", SS_LEFT, kCategoryLabel);
    HWND categories = AddControl(window, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL,
                                 kCategoryCombo);
    for (const Choice& choice : kCategories) SendMessageW(categories, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(choice.label));
    SendMessageW(categories, CB_SETCURSEL, static_cast<WPARAM>(-1), 0);
    SendMessageW(categories, CB_SETMINVISIBLE, static_cast<WPARAM>(kCategories.size()), 0);
    AddControl(window, L"STATIC", L"Impact", SS_LEFT, kImpactLabel);
    HWND impacts = AddControl(window, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL,
                              kImpactCombo);
    for (const ImpactChoice& choice : kImpacts) SendMessageW(impacts, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(choice.label));
    SendMessageW(impacts, CB_SETCURSEL, static_cast<WPARAM>(-1), 0);
    SendMessageW(impacts, CB_SETMINVISIBLE, static_cast<WPARAM>(kImpacts.size()), 0);
    AddControl(window, L"STATIC", L"What happened? Add a short note", SS_LEFT, kNoteLabel);
    AddControl(window, L"EDIT", L"", ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL | WS_TABSTOP,
               kNoteEdit, WS_EX_CLIENTEDGE);
    AddControl(window, L"BUTTON", L"I consent to export this report as a local JSON file", BS_AUTOCHECKBOX | WS_TABSTOP,
               kConsent);
    AddControl(window, L"BUTTON", L"Include basic device and RT context (optional)", BS_AUTOCHECKBOX | WS_TABSTOP,
               kIncludeContext);
    HWND disclosure = AddControl(window, L"STATIC", L"Both choices start off each time. Optional context contains only the generic Windows platform, Vulkan GPU model/backend, selected quality and scale, current internal extent, and whether an RT frame was presented. No logs, saves, screenshots, account or device identifiers are included.",
                                 SS_LEFT | SS_NOPREFIX, kDisclosure);
    if (!state.context.available)
    {
        SetWindowTextW(disclosure,
            L"Basic context is unavailable in this session. You can still export a note-only report. No extent or RT state will be guessed. No logs, saves, screenshots, account or device identifiers are included.");
        EnableWindow(GetDlgItem(window, kIncludeContext), FALSE);
    }
    AddControl(window, L"STATIC", L"Review the fields, then select Export local report to choose a file.", SS_LEFT | SS_NOPREFIX, kStatus);
    AddControl(window, L"BUTTON", L"Export local report...", BS_DEFPUSHBUTTON | WS_TABSTOP, kExport);
    AddControl(window, L"BUTTON", L"Close", BS_PUSHBUTTON | WS_TABSTOP, kCancel);
    ApplyFont(window);
    Layout(window);
}

LRESULT CALLBACK ReportWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* state = reinterpret_cast<DialogState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return DefWindowProcW(window, message, wParam, lParam);
    }
    if (message == WM_CREATE && state != nullptr)
    {
        CreateControls(window, *state);
        return 0;
    }
    switch (message)
    {
    case WM_SIZE:
        Layout(window);
        return 0;
    case WM_GETMINMAXINFO:
    {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = Scale(window, 540);
        info->ptMinTrackSize.y = Scale(window, 640);
        return 0;
    }
    case WM_DPICHANGED:
    {
        const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
        SetWindowPos(window, nullptr, suggested->left, suggested->top,
                     suggested->right - suggested->left, suggested->bottom - suggested->top,
                     SWP_NOACTIVATE | SWP_NOZORDER);
        ApplyFont(window);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == kExport)
        {
            if (state != nullptr) Export(window, *state);
            return 0;
        }
        if (LOWORD(wParam) == kCancel)
        {
            DestroyWindow(window);
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(window);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_NCDESTROY:
        if (HFONT font = reinterpret_cast<HFONT>(RemovePropW(window, kFontProperty))) DeleteObject(font);
        break;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

bool RegisterWindowClass()
{
    static bool registered = false;
    if (registered) return true;
    WNDCLASSEXW cls{};
    cls.cbSize = sizeof(cls);
    cls.style = CS_HREDRAW | CS_VREDRAW;
    cls.lpfnWndProc = ReportWindowProc;
    cls.hInstance = GetModuleHandleW(nullptr);
    cls.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    cls.lpszClassName = kWindowClass;
    if (RegisterClassExW(&cls) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
    registered = true;
    return true;
}

} // namespace

bool ExportWindowsPlaytestReportJson(HWND owner, const std::string& json, std::wstring& message)
{
    if (json.empty() || json.size() > horde::reporting::kPlaytestReportMaxJsonBytes)
    {
        message = L"The prepared local report exceeds its bounded JSON contract. Nothing was written.";
        return false;
    }
    try { return SaveJson(owner, json, message); }
    catch (...)
    {
        message = L"Local export failed; a selected file may be partial. Retry explicitly with the same report.";
        return false;
    }
}

void ShowWindowsPlaytestReport(HWND owner, WindowsPlaytestReportContext context,
    const WindowsPlaytestReportExporter exporter)
{
    if (!RegisterWindowClass()) return;
    DialogState state{std::move(context)};
    state.exporter = exporter;
    const UINT ownerDpi = GetDpiForWindow(owner);
    const int dpi = static_cast<int>(ownerDpi == 0u ? 96u : ownerDpi);
    RECT rect{0, 0, MulDiv(650, dpi, 96), MulDiv(690, dpi, 96)};
    AdjustWindowRectEx(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN,
                       FALSE, WS_EX_DLGMODALFRAME);
    RECT ownerRect{};
    GetWindowRect(owner, &ownerRect);
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    const int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - width) / 2;
    const int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - height) / 2;
    HWND window = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT,
                                  kWindowClass, kDialogTitle,
                                  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN,
                                  x, y, width, height, owner, nullptr, GetModuleHandleW(nullptr), &state);
    if (window == nullptr) return;
    EnableWindow(owner, FALSE);
    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);
    SetFocus(GetDlgItem(window, kCategoryCombo));
    MSG message{};
    BOOL keepRunning = TRUE;
    while (IsWindow(window) && keepRunning)
    {
        const BOOL result = GetMessageW(&message, nullptr, 0, 0);
        if (result <= 0)
        {
            keepRunning = FALSE;
            if (result == 0) PostQuitMessage(static_cast<int>(message.wParam));
            break;
        }
        if (message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE)
        {
            const HWND focused = GetFocus();
            const int focusedId = focused != nullptr ? GetDlgCtrlID(focused) : 0;
            const int comboId = focusedId == kCategoryCombo ? kCategoryCombo :
                                focusedId == kImpactCombo ? kImpactCombo : 0;
            if (comboId != 0 && SendMessageW(GetDlgItem(window, comboId), CB_GETDROPPEDSTATE, 0, 0) != 0)
            {
                SendMessageW(GetDlgItem(window, comboId), CB_SHOWDROPDOWN, FALSE, 0);
            }
            else
            {
                SendMessageW(window, WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), 0);
            }
            continue;
        }
        if (!IsDialogMessageW(window, &message))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    if (IsWindow(window)) DestroyWindow(window);
    EnableWindow(owner, TRUE);
    SetForegroundWindow(owner);
}

} // namespace horde::platform::windows
