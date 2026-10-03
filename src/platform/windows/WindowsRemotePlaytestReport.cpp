#include "platform/windows/WindowsRemotePlaytestReport.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commctrl.h>
#include <bcrypt.h>
#include <objbase.h>

#include "platform/windows/WindowsPlaytestScreenshot.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <utility>
#include <vector>

namespace horde::platform::windows
{
namespace
{
constexpr wchar_t kWindowClass[] = L"HordeLanternRemotePlaytestReportDialog";
constexpr wchar_t kBodyContentClass[] = L"HordeLanternRemotePlaytestBodyContent";
constexpr wchar_t kDialogTitle[] = L"Send a playtest report";
constexpr int kIntro = 201;
constexpr int kCategoryLabel = 203;
constexpr int kImpactLabel = 205;
constexpr int kNoteLabel = 219;
constexpr int kDisclosure = 217;
constexpr int kFontProperty = 218;
constexpr int kDpiProperty = 220;
constexpr UINT_PTR kPollTimer = 0x484e;
constexpr UINT_PTR kFocusSubclassId = 0x484f;
constexpr UINT kEnsureControlVisibleMessage = WM_APP + 0x4a1;
constexpr auto kPollIntervalMs = 75u;
constexpr auto kReportSizeMaxUtf16Units = 16384;
namespace id = remote_playtest_control;
constexpr std::array<int, 11> kTabControlIds{{
    id::Category, id::Impact, id::Note, id::SubmitConsent, id::IncludeDiagnostics,
    id::IncludeScreenshot, id::Send, id::Retry, id::OfflineExport, id::NewReport, id::Close,
}};

struct CategoryChoice
{
    const wchar_t* label;
    reporting::PlaytestReportCategory value;
};
constexpr std::array<CategoryChoice, 6> kCategories{{
    {L"Gameplay", reporting::PlaytestReportCategory::Gameplay},
    {L"Visuals", reporting::PlaytestReportCategory::Visuals},
    {L"Performance", reporting::PlaytestReportCategory::Performance},
    {L"Controls", reporting::PlaytestReportCategory::Controls},
    {L"Audio", reporting::PlaytestReportCategory::Audio},
    {L"Other", reporting::PlaytestReportCategory::Other},
}};
struct ImpactChoice
{
    const wchar_t* label;
    reporting::PlaytestReportImpact value;
};
constexpr std::array<ImpactChoice, 4> kImpacts{{
    {L"Blocks progress", reporting::PlaytestReportImpact::BlocksProgress},
    {L"Major friction", reporting::PlaytestReportImpact::MajorFriction},
    {L"Minor friction", reporting::PlaytestReportImpact::MinorFriction},
    {L"Polish", reporting::PlaytestReportImpact::Polish},
}};

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
void WipeContext(reporting::OwnedPlaytestReportContext& context) noexcept
{
    Wipe(context.product); Wipe(context.version); Wipe(context.build); Wipe(context.platform);
    Wipe(context.rawModel); Wipe(context.gpu); Wipe(context.backend); Wipe(context.quality);
    context.renderScale = 0.0; context.internalWidth = 0u; context.internalHeight = 0u;
    context.rtPresented = false;
}

bool MakeIdentity(std::string& idValue, std::string& timestamp)
{
    std::array<unsigned char, 16> random{};
    if (BCryptGenRandom(nullptr, random.data(), static_cast<ULONG>(random.size()),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) return false;
    constexpr char hex[] = "0123456789abcdef";
    idValue = "win_report_";
    idValue.reserve(idValue.size() + random.size() * 2u);
    for (const unsigned char byte : random)
    {
        idValue.push_back(hex[byte >> 4u]);
        idValue.push_back(hex[byte & 0x0fu]);
    }
    SYSTEMTIME now{};
    GetSystemTime(&now);
    wchar_t formatted[32]{};
    const int count = swprintf_s(formatted, L"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
        now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);
    if (count <= 0) return false;
    timestamp.clear();
    timestamp.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) timestamp.push_back(static_cast<char>(formatted[i]));
    return true;
}

int Scale(HWND window, const int value)
{
    const auto property = reinterpret_cast<LPCWSTR>(static_cast<ULONG_PTR>(kDpiProperty));
    const UINT dpi = static_cast<UINT>(reinterpret_cast<ULONG_PTR>(GetPropW(window, property)));
    const UINT effectiveDpi = dpi == 0u ? GetDpiForWindow(window) : dpi;
    return MulDiv(value, static_cast<int>(effectiveDpi == 0u ? 96u : effectiveDpi), 96);
}

HWND AddControl(HWND parent, const wchar_t* className, const wchar_t* text,
    const DWORD style, const int controlId, const DWORD extendedStyle = 0u)
{
    HWND control = CreateWindowExW(extendedStyle, className, text, WS_CHILD | WS_VISIBLE | style,
        0, 0, 1, 1, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(controlId)),
        GetModuleHandleW(nullptr), nullptr);
    if (control != nullptr)
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
    return control;
}

struct DialogState
{
    DialogState(WindowsPlaytestReportContext ownedContext, WindowsRemotePlaytestServices injected)
        : context(std::move(ownedContext)), services(std::move(injected)), submission(services.exchange) {}
    ~DialogState()
    {
        submission.Reset();
        Wipe(remote.json); Wipe(offline.json); Wipe(reportId); Wipe(timestamp); Wipe(note);
        Wipe(png); Wipe(pixels.rgba); Wipe(previewBgra); WipeContext(context.values);
    }

    WindowsPlaytestReportContext context;
    WindowsRemotePlaytestServices services;
    WindowsPlaytestSubmission submission;
    reporting::PreparedPlaytestSubmission remote;
    reporting::PreparedPlaytestReport offline;
    reporting::PlaytestScreenshotPixels pixels;
    std::vector<std::uint8_t> png;
    std::vector<std::uint8_t> previewBgra;
    std::string reportId, timestamp, note;
    WindowsPlaytestSubmissionResult lastShown = WindowsPlaytestSubmissionResult::None;
    bool remoteFrozen = false;
    bool attemptStarted = false;
    bool wasBusy = false;
    bool pollingAvailable = false;
    int scrollY = 0;
    int bodyHeight = 0;
    int bodyViewportHeight = 0;
    HWND bodyViewport = nullptr;
    HWND bodyContent = nullptr;
};

void SetStatus(HWND window, const wchar_t* text)
{
    if (HWND status = GetDlgItem(window, id::Status)) SetWindowTextW(status, text);
}
void SetStatus(HWND window, const std::wstring& text)
{
    if (HWND status = GetDlgItem(window, id::Status)) SetWindowTextW(status, text.c_str());
}

HWND Control(HWND window, const int controlId)
{
    if (HWND direct = GetDlgItem(window, controlId)) return direct;
    const auto* state = reinterpret_cast<const DialogState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    return state != nullptr && state->bodyContent != nullptr
        ? GetDlgItem(state->bodyContent, controlId) : nullptr;
}

bool ReadNote(HWND window, std::string& utf8, std::wstring& error)
{
    const HWND edit = Control(window, id::Note);
    const int count = GetWindowTextLengthW(edit);
    if (count < 0 || count > kReportSizeMaxUtf16Units)
    {
        error = L"The note is too large to validate safely.";
        return false;
    }
    std::wstring wide(static_cast<std::size_t>(count) + 1u, L'\0');
    const int copied = GetWindowTextW(edit, wide.data(), count + 1);
    if (copied != count)
    {
        error = L"Windows could not read the note. Nothing was captured or sent.";
        return false;
    }
    wide.resize(static_cast<std::size_t>(count));
    static_assert(sizeof(wchar_t) == sizeof(char16_t));
    std::u16string units;
    units.reserve(wide.size());
    for (const wchar_t unit : wide) units.push_back(static_cast<char16_t>(unit));
    const auto status = reporting::EncodePlaytestReportUtf16(units,
        reporting::kPlaytestReportMaxNoteBytes, utf8);
    if (status == reporting::PlaytestReportStatus::Ready) return true;
    const auto message = reporting::PlaytestReportStatusName(status);
    error = L"Note validation failed: ";
    for (const char ch : message) error.push_back(static_cast<wchar_t>(ch));
    return false;
}

bool ReadFields(HWND window, DialogState& state, const bool requireRemoteConsent,
    reporting::PlaytestReportInput& input, std::wstring& error)
{
    if (!ReadNote(window, state.note, error)) return false;
    const LRESULT category = SendMessageW(Control(window, id::Category), CB_GETCURSEL, 0, 0);
    const LRESULT impact = SendMessageW(Control(window, id::Impact), CB_GETCURSEL, 0, 0);
    if (category < 0 || impact < 0 || static_cast<std::size_t>(category) >= kCategories.size() ||
        static_cast<std::size_t>(impact) >= kImpacts.size())
    {
        error = L"Choose a category and impact first.";
        return false;
    }
    const bool consent = SendMessageW(Control(window, id::SubmitConsent), BM_GETCHECK, 0, 0) == BST_CHECKED;
    if (requireRemoteConsent && !consent)
    {
        error = L"Remote submission is off. Check the consent box before sending.";
        return false;
    }
    if (state.reportId.empty() && !MakeIdentity(state.reportId, state.timestamp))
    {
        error = L"Windows could not create a secure report ID or UTC timestamp.";
        return false;
    }
    const bool includeContext = SendMessageW(Control(window, id::IncludeDiagnostics), BM_GETCHECK, 0, 0) == BST_CHECKED;
    if (includeContext && !state.context.available)
    {
        error = L"Optional diagnostics are unavailable in this session.";
        return false;
    }
    input = {state.reportId, state.timestamp,
        kCategories[static_cast<std::size_t>(category)].value,
        kImpacts[static_cast<std::size_t>(impact)].value, state.note,
        true, includeContext, includeContext ? state.context.values.View() : reporting::PlaytestReportContext{}};
    return true;
}

void EnableEditableControls(HWND window, DialogState& state, const bool enable)
{
    if (HWND item = Control(window, id::Category)) EnableWindow(item, enable);
    if (HWND item = Control(window, id::Impact)) EnableWindow(item, enable);
    if (HWND item = Control(window, id::Note)) EnableWindow(item, enable);
    if (HWND item = Control(window, id::SubmitConsent)) EnableWindow(item, enable);
    if (HWND item = Control(window, id::IncludeDiagnostics))
        EnableWindow(item, enable && state.context.available);
    if (HWND item = Control(window, id::IncludeScreenshot))
        EnableWindow(item, enable && static_cast<bool>(state.services.captureGameFrame));
}

void DrawPreview(const DRAWITEMSTRUCT& draw, const DialogState& state)
{
    FillRect(draw.hDC, &draw.rcItem, GetSysColorBrush(COLOR_WINDOW));
    if (state.pixels.width == 0u || state.pixels.height == 0u || state.pixels.rgba.empty())
    {
        const wchar_t* text = L"No game RT frame selected. This preview never captures the desktop or report form.";
        SetBkMode(draw.hDC, TRANSPARENT);
        SetTextColor(draw.hDC, GetSysColor(COLOR_GRAYTEXT));
        DrawTextW(draw.hDC, text, -1, const_cast<RECT*>(&draw.rcItem), DT_CENTER | DT_VCENTER | DT_WORDBREAK);
        return;
    }
    if (state.previewBgra.size() != state.pixels.rgba.size()) return;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = static_cast<LONG>(state.pixels.width);
    info.bmiHeader.biHeight = -static_cast<LONG>(state.pixels.height);
    info.bmiHeader.biPlanes = 1u;
    info.bmiHeader.biBitCount = 32u;
    info.bmiHeader.biCompression = BI_RGB;
    const RECT target = FitWindowsPlaytestPreviewRect(draw.rcItem,
        state.pixels.width, state.pixels.height);
    SetStretchBltMode(draw.hDC, HALFTONE);
    (void)StretchDIBits(draw.hDC, target.left, target.top,
        target.right - target.left, target.bottom - target.top,
        0, 0, static_cast<int>(state.pixels.width), static_cast<int>(state.pixels.height),
        state.previewBgra.data(), &info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK BodyContentProc(HWND window, const UINT message, const WPARAM wParam, const LPARAM lParam)
{
    auto* state = reinterpret_cast<DialogState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        state = static_cast<DialogState*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }
    if (message == WM_DRAWITEM && state != nullptr && wParam == id::Preview)
    {
        DrawPreview(*reinterpret_cast<const DRAWITEMSTRUCT*>(lParam), *state);
        return TRUE;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

int WrappedHeight(HWND window, const int controlId, const int width, const int minimum)
{
    HWND control = Control(window, controlId);
    if (!control) return minimum;
    const int length = GetWindowTextLengthW(control);
    std::wstring text(static_cast<std::size_t>(std::max(0, length)) + 1u, L'\0');
    GetWindowTextW(control, text.data(), length + 1);
    RECT bounds{0, 0, std::max(1, width), 0};
    HDC dc = GetDC(control);
    if (!dc) return minimum;
    const HFONT font = reinterpret_cast<HFONT>(SendMessageW(control, WM_GETFONT, 0, 0));
    const HGDIOBJ oldFont = dc && font ? SelectObject(dc, font) : nullptr;
    DrawTextW(dc, text.c_str(), length, &bounds, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
    if (dc && oldFont) SelectObject(dc, oldFont);
    if (dc) ReleaseDC(control, dc);
    return std::max(minimum, static_cast<int>(bounds.bottom) + Scale(window, 8));
}

void Layout(HWND window, DialogState& state)
{
    RECT client{};
    GetClientRect(window, &client);
    const int width = client.right;
    const int height = client.bottom;
    const int margin = Scale(window, 16);
    const int gap = Scale(window, 4);
    const int label = Scale(window, 19);
    const int combo = Scale(window, 27);
    const int contentWidth = std::max(1, width - 2 * margin - Scale(window, GetSystemMetrics(SM_CXVSCROLL)));
    const int footerHeight = Scale(window, 82);
    state.bodyViewportHeight = std::max(Scale(window, 72), height - 2 * margin - footerHeight);
    if (state.bodyViewport) MoveWindow(state.bodyViewport, margin, margin, contentWidth, state.bodyViewportHeight, TRUE);

    int y = margin;
    if (HWND control = Control(window, kIntro)) MoveWindow(control, margin, y, contentWidth - 2 * margin, Scale(window, 44), TRUE);
    y += Scale(window, 44) + gap;
    if (HWND control = Control(window, kCategoryLabel)) MoveWindow(control, margin, y, contentWidth - 2 * margin, label, TRUE);
    y += label;
    if (HWND control = Control(window, id::Category)) MoveWindow(control, margin, y, contentWidth - 2 * margin, combo, TRUE);
    y += combo + gap;
    if (HWND control = Control(window, kImpactLabel)) MoveWindow(control, margin, y, contentWidth - 2 * margin, label, TRUE);
    y += label;
    if (HWND control = Control(window, id::Impact)) MoveWindow(control, margin, y, contentWidth - 2 * margin, combo, TRUE);
    y += combo + gap;
    if (HWND control = Control(window, kNoteLabel)) MoveWindow(control, margin, y, contentWidth - 2 * margin, label, TRUE);
    y += label;
    const int noteHeight = Scale(window, 96);
    if (HWND control = Control(window, id::Note)) MoveWindow(control, margin, y, contentWidth - 2 * margin, noteHeight, TRUE);
    y += noteHeight + gap;
    for (const int controlId : {id::SubmitConsent, id::IncludeDiagnostics, id::IncludeScreenshot})
    {
        const int itemHeight = WrappedHeight(window, controlId, contentWidth - 2 * margin, Scale(window, 30));
        if (HWND item = Control(window, controlId)) MoveWindow(item, margin, y, contentWidth - 2 * margin, itemHeight, TRUE);
        y += itemHeight;
    }
    y += gap;
    const int disclosureHeight = WrappedHeight(window, kDisclosure, contentWidth - 2 * margin, Scale(window, 55));
    if (HWND control = Control(window, kDisclosure)) MoveWindow(control, margin, y, contentWidth - 2 * margin, disclosureHeight, TRUE);
    y += disclosureHeight + gap;
    if (HWND control = Control(window, id::Preview)) MoveWindow(control, margin, y, contentWidth - 2 * margin, Scale(window, 120), TRUE);
    state.bodyHeight = y + Scale(window, 120) + margin;
    state.scrollY = std::clamp(state.scrollY, 0, std::max(0, state.bodyHeight - state.bodyViewportHeight));
    if (state.bodyContent) MoveWindow(state.bodyContent, 0, -state.scrollY, contentWidth, state.bodyHeight, TRUE);
    SCROLLINFO scroll{};
    scroll.cbSize = sizeof(scroll); scroll.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    scroll.nMin = 0; scroll.nMax = std::max(0, state.bodyHeight - 1);
    scroll.nPage = static_cast<UINT>(std::max(0, state.bodyViewportHeight)); scroll.nPos = state.scrollY;
    SetScrollInfo(window, SB_VERT, &scroll, TRUE);

    const int buttonH = Scale(window, 32), buttonGap = Scale(window, 4);
    const int buttonsY = height - margin - buttonH;
    const int buttonWidth = std::max(1, (width - 2 * margin - 4 * buttonGap) / 5);
    const int buttonIds[]{id::Send, id::Retry, id::OfflineExport, id::NewReport, id::Close};
    const wchar_t* captions[]{state.remoteFrozen ? L"Verify" : L"Prepare", L"Retry", L"Offline", L"New", L"Close"};
    int x = margin;
    for (std::size_t index = 0; index < std::size(buttonIds); ++index)
    {
        if (HWND button = GetDlgItem(window, buttonIds[index]))
        {
            SetWindowTextW(button, captions[index]);
            MoveWindow(button, x, buttonsY, buttonWidth, buttonH, TRUE);
        }
        x += buttonWidth + buttonGap;
    }
    const int statusTop = buttonsY - Scale(window, 42);
    if (HWND control = GetDlgItem(window, id::Status))
        MoveWindow(control, margin, statusTop, width - 2 * margin, Scale(window, 38), TRUE);
}

void EnsureControlVisible(HWND window, DialogState& state, HWND control)
{
    if (control == nullptr || state.bodyViewport == nullptr || state.bodyContent == nullptr ||
        !IsChild(state.bodyContent, control)) return;
    RECT item{}, viewport{};
    if (!GetWindowRect(control, &item) || !GetWindowRect(state.bodyViewport, &viewport)) return;
    int delta = 0;
    if (item.top < viewport.top) delta = item.top - viewport.top;
    else if (item.bottom > viewport.bottom) delta = item.bottom - viewport.bottom;
    if (delta == 0) return;
    state.scrollY = std::clamp(state.scrollY + delta, 0,
        std::max(0, state.bodyHeight - state.bodyViewportHeight));
    Layout(window, state);
}

HWND NextTabControl(HWND window, const HWND current, const bool reverse)
{
    const int currentId = current == nullptr ? 0 : GetDlgCtrlID(current);
    const auto currentPosition = std::find(kTabControlIds.begin(), kTabControlIds.end(), currentId);
    std::size_t index = currentPosition == kTabControlIds.end()
        ? (reverse ? 0u : kTabControlIds.size() - 1u)
        : static_cast<std::size_t>(std::distance(kTabControlIds.begin(), currentPosition));
    for (std::size_t tries = 0; tries < kTabControlIds.size(); ++tries)
    {
        index = reverse
            ? (index + kTabControlIds.size() - 1u) % kTabControlIds.size()
            : (index + 1u) % kTabControlIds.size();
        const HWND candidate = Control(window, kTabControlIds[index]);
        if (candidate != nullptr && IsWindowEnabled(candidate) && IsWindowVisible(candidate)) return candidate;
    }
    return nullptr;
}

void FocusTabControl(HWND window, DialogState& state, const HWND target)
{
    if (target == nullptr) return;
    SetFocus(target);
    EnsureControlVisible(window, state, target);
}

LRESULT CALLBACK FocusScrollSubclassProc(HWND control, const UINT message,
    const WPARAM wParam, const LPARAM lParam, const UINT_PTR subclassId, const DWORD_PTR reference)
{
    if (message == WM_SETFOCUS)
    {
        const HWND owner = reinterpret_cast<HWND>(reference);
        if (owner != nullptr && IsWindow(owner))
            PostMessageW(owner, kEnsureControlVisibleMessage, reinterpret_cast<WPARAM>(control), 0);
    }
    if (message == WM_NCDESTROY) RemoveWindowSubclass(control, FocusScrollSubclassProc, subclassId);
    return DefSubclassProc(control, message, wParam, lParam);
}

void ApplyFont(HWND window)
{
    const auto fontProperty = reinterpret_cast<LPCWSTR>(static_cast<ULONG_PTR>(kFontProperty));
    if (HFONT old = reinterpret_cast<HFONT>(RemovePropW(window, fontProperty))) DeleteObject(old);
    HFONT font = CreateFontW(Scale(window, 16), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    if (font == nullptr) font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    else SetPropW(window, fontProperty, font);
    EnumChildWindows(window, [](HWND child, LPARAM fontHandle) -> BOOL {
        SendMessageW(child, WM_SETFONT, static_cast<WPARAM>(fontHandle), TRUE);
        return TRUE;
    }, reinterpret_cast<LPARAM>(font));
}

void CreateControls(HWND window, DialogState& state)
{
    state.bodyViewport = CreateWindowExW(WS_EX_CONTROLPARENT, L"STATIC", L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        0, 0, 1, 1, window, nullptr, GetModuleHandleW(nullptr), nullptr);
    state.bodyContent = CreateWindowExW(WS_EX_CONTROLPARENT, kBodyContentClass, L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        0, 0, 1, 1, state.bodyViewport, nullptr, GetModuleHandleW(nullptr), &state);
    AddControl(state.bodyContent, L"STATIC", L"Describe the issue. Nothing is sent unless you opt in and choose Prepare report, then Verify and send.", SS_LEFT | SS_NOPREFIX, kIntro);
    AddControl(state.bodyContent, L"STATIC", L"Category", SS_LEFT, kCategoryLabel);
    HWND categories = AddControl(state.bodyContent, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL, id::Category);
    for (const auto& choice : kCategories) SendMessageW(categories, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(choice.label));
    SendMessageW(categories, CB_SETCURSEL, static_cast<WPARAM>(-1), 0);
    AddControl(state.bodyContent, L"STATIC", L"Impact", SS_LEFT, kImpactLabel);
    HWND impacts = AddControl(state.bodyContent, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL, id::Impact);
    for (const auto& choice : kImpacts) SendMessageW(impacts, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(choice.label));
    SendMessageW(impacts, CB_SETCURSEL, static_cast<WPARAM>(-1), 0);
    AddControl(state.bodyContent, L"STATIC", L"What happened? Include steps to reproduce.", SS_LEFT, kNoteLabel);
    AddControl(state.bodyContent, L"EDIT", L"", ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN |
        WS_VSCROLL | WS_TABSTOP, id::Note, WS_EX_CLIENTEDGE);
    AddControl(state.bodyContent, L"BUTTON", L"I consent to submit this report to the Horde report relay", BS_AUTOCHECKBOX | BS_MULTILINE | WS_TABSTOP, id::SubmitConsent);
    AddControl(state.bodyContent, L"BUTTON", L"Include basic diagnostics (optional)", BS_AUTOCHECKBOX | BS_MULTILINE | WS_TABSTOP, id::IncludeDiagnostics);
    AddControl(state.bodyContent, L"BUTTON", L"Include a game RT frame (optional)", BS_AUTOCHECKBOX | BS_MULTILINE | WS_TABSTOP, id::IncludeScreenshot);
    const wchar_t* details = state.context.available
        ? L"Diagnostics are only the allowlisted build/platform/GPU/backend/quality/RT context. The optional image is a downsampled game RT frame only; no desktop, window, UI, logs, saves, accounts, or identifiers. Both options start off."
        : L"Diagnostics are unavailable in this session. The optional image is a downsampled game RT frame only; no desktop, window, UI, logs, saves, accounts, or identifiers. Both options start off.";
    AddControl(state.bodyContent, L"STATIC", details, SS_LEFT | SS_NOPREFIX, kDisclosure);
    AddControl(state.bodyContent, L"STATIC", L"", SS_OWNERDRAW, id::Preview, WS_EX_CLIENTEDGE);
    AddControl(window, L"STATIC", L"Review the report before sending. Verification receives no note or screenshot.", SS_LEFT | SS_NOPREFIX, id::Status);
    AddControl(window, L"BUTTON", L"Prepare", BS_DEFPUSHBUTTON | WS_TABSTOP, id::Send);
    AddControl(window, L"BUTTON", L"Retry", BS_PUSHBUTTON | WS_TABSTOP, id::Retry);
    AddControl(window, L"BUTTON", L"Offline", BS_PUSHBUTTON | WS_TABSTOP, id::OfflineExport);
    AddControl(window, L"BUTTON", L"New", BS_PUSHBUTTON | WS_TABSTOP, id::NewReport);
    AddControl(window, L"BUTTON", L"Close", BS_PUSHBUTTON | WS_TABSTOP, id::Close);
    if (!state.context.available) EnableWindow(Control(window, id::IncludeDiagnostics), FALSE);
    if (!state.services.captureGameFrame) EnableWindow(Control(window, id::IncludeScreenshot), FALSE);
    EnableWindow(GetDlgItem(window, id::Retry), FALSE);
    for (const int controlId : {id::Category, id::Impact, id::Note, id::SubmitConsent,
        id::IncludeDiagnostics, id::IncludeScreenshot})
        if (HWND control = Control(window, controlId))
            SetWindowSubclass(control, FocusScrollSubclassProc, kFocusSubclassId,
                reinterpret_cast<DWORD_PTR>(window));
    ApplyFont(window);
    Layout(window, state);
}

void FreezeControls(HWND window, DialogState& state)
{
    EnableEditableControls(window, state, FALSE);
    state.remoteFrozen = true;
    EnsureControlVisible(window, state, Control(window, id::Preview));
    if (HWND preview = Control(window, id::Preview)) InvalidateRect(preview, nullptr, TRUE);
    EnableWindow(GetDlgItem(window, id::Send), FALSE);
    Layout(window, state);
}

bool EncodeFrame(DialogState& state, std::wstring& error)
{
    const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool uninitialize = SUCCEEDED(init);
    const bool encoded = EncodeWindowsPlaytestScreenshot(state.pixels, state.png);
    if (uninitialize) CoUninitialize();
    if (!encoded)
    {
        error = L"The consented game RT frame could not be encoded within the screenshot limits. Nothing was verified or sent.";
        return false;
    }
    return true;
}

bool PrepareRemote(HWND window, DialogState& state)
{
    reporting::PlaytestReportInput input;
    std::wstring error;
    if (!ReadFields(window, state, true, input, error))
    {
        SetStatus(window, error);
        return false;
    }
    // Validate consent, note, timestamp, identity and selected context before
    // asking the render owner to read back an optional frame.
    auto preflight = reporting::PreparePlaytestSubmission(input, false);
    if (!preflight.IsReady())
    {
        const auto reason = reporting::PlaytestReportStatusName(preflight.reportStatus);
        error = L"Report validation failed: ";
        for (const char ch : reason) error.push_back(static_cast<wchar_t>(ch));
        SetStatus(window, error);
        return false;
    }
    const bool includeScreenshot = SendMessageW(Control(window, id::IncludeScreenshot), BM_GETCHECK, 0, 0) == BST_CHECKED;
    Wipe(state.png);
    Wipe(state.previewBgra);
    Wipe(state.pixels.rgba);
    state.pixels = {};
    WindowsPlaytestReportContext capturedContext = state.context;
    if (includeScreenshot)
    {
        if (!state.services.captureGameFrame ||
            !state.services.captureGameFrame(window, capturedContext, state.pixels, error))
        {
            Wipe(state.pixels.rgba);
            state.pixels = {};
            if (error.empty()) error = L"The game RT frame is unavailable. Nothing was verified or sent.";
            SetStatus(window, error);
            return false;
        }
        if (!EncodeFrame(state, error))
        {
            Wipe(state.png);
            Wipe(state.pixels.rgba);
            state.pixels = {};
            SetStatus(window, error);
            return false;
        }
        try
        {
            state.previewBgra.resize(state.pixels.rgba.size());
            for (std::size_t pixel = 0; pixel < state.previewBgra.size() / 4u; ++pixel)
            {
                state.previewBgra[pixel * 4u] = state.pixels.rgba[pixel * 4u + 2u];
                state.previewBgra[pixel * 4u + 1u] = state.pixels.rgba[pixel * 4u + 1u];
                state.previewBgra[pixel * 4u + 2u] = state.pixels.rgba[pixel * 4u];
                state.previewBgra[pixel * 4u + 3u] = 255u;
            }
        }
        catch (...)
        {
            Wipe(state.png); Wipe(state.pixels.rgba); Wipe(state.previewBgra); state.pixels = {};
            SetStatus(window, L"The bounded game-frame preview could not be allocated. Nothing was verified or sent.");
            return false;
        }
    }
    reporting::PreparedPlaytestSubmission prepared;
    if (includeScreenshot)
    {
        if (input.includeBasicContext)
        {
            if (!capturedContext.available)
            {
                Wipe(state.png); Wipe(state.pixels.rgba); state.pixels = {};
                SetStatus(window, L"The consented diagnostics snapshot was unavailable with the RT frame. Nothing was verified or sent.");
                return false;
            }
            input.context = capturedContext.values.View();
        }
        prepared = reporting::PreparePlaytestSubmission(input, true,
            {state.png, state.pixels.width, state.pixels.height});
    }
    else prepared = std::move(preflight);
    if (!prepared.IsReady())
    {
        Wipe(prepared.json);
        Wipe(state.png); Wipe(state.previewBgra);
        Wipe(state.pixels.rgba);
        state.pixels = {};
        SetStatus(window, L"The report or RT screenshot did not pass the shared validation limits. Nothing was sent.");
        return false;
    }
    Wipe(state.remote.json);
    state.remote = std::move(prepared);
    FreezeControls(window, state);
    SetStatus(window, includeScreenshot
        ? L"Frozen report and consented game-frame preview are ready. Choose Verify and send to continue."
        : L"Frozen report is ready. Choose Verify and send to continue.");
    return true;
}

WindowsPlaytestVerificationResult Verify(HWND window, DialogState& state)
{
    if (state.services.verifyForeground) return state.services.verifyForeground(window);
    return ShowWindowsPlaytestVerification(window);
}

void StartRemoteAttempt(HWND window, DialogState& state)
{
    if (state.submission.IsBusy()) return;
    if (!state.remoteFrozen)
    {
        if (!PrepareRemote(window, state)) return;
        return; // Preparation is a distinct review step before verification.
    }
    if (!state.pollingAvailable)
    {
        SetStatus(window, L"The form could not start its bounded completion monitor. Nothing was sent.");
        EnableWindow(GetDlgItem(window, id::Send), !state.attemptStarted);
        EnableWindow(GetDlgItem(window, id::Retry), state.attemptStarted && state.submission.CanRetry());
        return;
    }
    SetStatus(window, L"Opening foreground anti-spam verification. It receives no report text or image.");
    EnableWindow(GetDlgItem(window, id::Send), FALSE);
    EnableWindow(GetDlgItem(window, id::Retry), FALSE);
    auto verified = Verify(window, state);
    if (verified.status != WindowsPlaytestVerificationStatus::Verified || verified.token.empty())
    {
        Wipe(verified.token);
        switch (verified.status)
        {
        case WindowsPlaytestVerificationStatus::Cancelled: SetStatus(window, L"Verification cancelled. No report was sent; the frozen report can be verified again."); break;
        case WindowsPlaytestVerificationStatus::Deadline: SetStatus(window, L"Verification timed out. No report was sent; choose Verify and send to try again."); break;
        case WindowsPlaytestVerificationStatus::Failed: SetStatus(window, L"Verification was not accepted. No report was sent; choose Verify and send to try again."); break;
        default: SetStatus(window, L"Verification is unavailable. No report was sent."); break;
        }
        EnableWindow(GetDlgItem(window, id::Send), !state.attemptStarted);
        EnableWindow(GetDlgItem(window, id::Retry), state.attemptStarted && state.submission.CanRetry());
        return;
    }
    bool started = false;
    if (state.attemptStarted)
    {
        if (state.submission.CanRetry()) started = state.submission.Retry(window, verified.token);
    }
    else started = state.submission.Begin(window, state.remote, verified.token);
    Wipe(verified.token);
    if (!started)
    {
        SetStatus(window, L"The verified attempt could not start. Your frozen report remains unchanged; try again explicitly.");
        EnableWindow(GetDlgItem(window, id::Send), !state.attemptStarted);
        EnableWindow(GetDlgItem(window, id::Retry), state.attemptStarted && state.submission.CanRetry());
        return;
    }
    state.attemptStarted = true;
    state.wasBusy = true;
    EnableWindow(GetDlgItem(window, id::Send), FALSE);
    EnableWindow(GetDlgItem(window, id::Retry), FALSE);
    SetStatus(window, L"Sending this foreground attempt. Cancel cannot recall a report already accepted by the relay.");
}

void ShowResult(HWND window, DialogState& state, const WindowsPlaytestSubmissionResult result)
{
    state.lastShown = result;
    switch (result)
    {
    case WindowsPlaytestSubmissionResult::Queued:
        SetStatus(window, L"Relay durably queued the report. This confirms queue acceptance, not delivery."); break;
    case WindowsPlaytestSubmissionResult::Sent:
        SetStatus(window, L"Relay confirmed the report was sent."); break;
    case WindowsPlaytestSubmissionResult::VerificationExpired:
        SetStatus(window, L"Verification expired. Choose Retry same report for fresh verification; frozen ID and bytes stay the same."); break;
    case WindowsPlaytestSubmissionResult::RateLimited:
        SetStatus(window, L"The relay rate-limited this attempt. Retry explicitly later with fresh verification; the report identity is unchanged."); break;
    case WindowsPlaytestSubmissionResult::Conflict:
        SetStatus(window, L"The relay has different content under this report ID. Do not edit and retry this report; start a new report only after reviewing the warning."); break;
    case WindowsPlaytestSubmissionResult::Rejected:
        SetStatus(window, L"The relay rejected this report. No automatic retry will occur."); break;
    case WindowsPlaytestSubmissionResult::Uncertain:
        SetStatus(window, L"Delivery is uncertain; the report may already have been received. Retry is explicit and reuses the same ID and bytes."); break;
    default: break;
    }
}

void Poll(HWND window, DialogState& state)
{
    const bool busy = state.submission.IsBusy();
    if (busy)
    {
        state.wasBusy = true;
        EnableEditableControls(window, state, FALSE);
        EnableWindow(GetDlgItem(window, id::Send), FALSE);
        EnableWindow(GetDlgItem(window, id::Retry), FALSE);
        if (state.submission.LastResult() == WindowsPlaytestSubmissionResult::Uncertain)
            SetStatus(window, L"Cancellation requested. Waiting for the local transport to retire; delivery may already have occurred.");
        return;
    }
    if (state.remoteFrozen) EnableEditableControls(window, state, FALSE);
    else EnableEditableControls(window, state, TRUE);
    const auto result = state.submission.LastResult();
    if (state.wasBusy || result != state.lastShown) ShowResult(window, state, result);
    state.wasBusy = false;
    const bool retry = state.remoteFrozen && state.attemptStarted && state.submission.CanRetry();
    EnableWindow(GetDlgItem(window, id::Send), !state.attemptStarted);
    EnableWindow(GetDlgItem(window, id::Retry), retry);
    EnableWindow(GetDlgItem(window, id::OfflineExport), TRUE);
    EnableWindow(GetDlgItem(window, id::NewReport), TRUE);
}

void ExportOffline(HWND window, DialogState& state)
{
    if (state.submission.IsBusy()) return;
    const bool screenshotChecked = SendMessageW(Control(window, id::IncludeScreenshot), BM_GETCHECK, 0, 0) == BST_CHECKED;
    const int warning = screenshotChecked
        ? MessageBoxW(window, L"Offline JSON contains the report and opted-in diagnostics, but not the RT frame. It remains on this PC unless you choose a file. Continue?",
            kDialogTitle, MB_OKCANCEL | MB_ICONINFORMATION)
        : MessageBoxW(window, L"This action writes report JSON to a file you choose. Nothing will be sent. Continue?",
            kDialogTitle, MB_OKCANCEL | MB_ICONINFORMATION);
    if (warning != IDOK) return;
    reporting::PlaytestReportInput input;
    std::wstring error;
    if (!ReadFields(window, state, false, input, error)) { SetStatus(window, error); return; }
    state.offline = reporting::PreparePlaytestReport(input);
    if (!state.offline.IsReady())
    {
        const auto reason = reporting::PlaytestReportStatusName(state.offline.status);
        error = L"Offline report validation failed: ";
        for (const char ch : reason) error.push_back(static_cast<wchar_t>(ch));
        SetStatus(window, error);
        return;
    }
    std::wstring message;
    const bool saved = state.services.exportOfflineJson
        ? state.services.exportOfflineJson(window, state.offline.json, message)
        : ExportWindowsPlaytestReportJson(window, state.offline.json, message);
    SetStatus(window, message.empty() ? (saved ? L"Offline JSON saved locally." : L"Offline export did not complete.") : message);
}

void ClearForNew(HWND window, DialogState& state)
{
    const auto result = state.submission.LastResult();
    const bool uncertainOrAccepted = state.attemptStarted || result == WindowsPlaytestSubmissionResult::Queued ||
        result == WindowsPlaytestSubmissionResult::Sent || result == WindowsPlaytestSubmissionResult::Uncertain;
    if (uncertainOrAccepted)
    {
        const wchar_t* warning = result == WindowsPlaytestSubmissionResult::Queued || result == WindowsPlaytestSubmissionResult::Sent
            ? L"The report may already be queued or sent and cannot be recalled. Starting a new report discards this frozen ID and local retry bytes. Continue?"
            : L"The previous attempt may already have reached the relay. Starting a new report discards its ID and retry bytes; the old report cannot be recalled. Continue?";
        if (MessageBoxW(window, warning, kDialogTitle, MB_OKCANCEL | MB_ICONWARNING) != IDOK) return;
    }
    if (state.submission.IsBusy()) state.submission.CancelAttempt();
    state.submission.Reset();
    Wipe(state.remote.json); Wipe(state.offline.json); Wipe(state.reportId); Wipe(state.timestamp); Wipe(state.note);
    Wipe(state.png); Wipe(state.pixels.rgba); Wipe(state.previewBgra); state.pixels = {};
    state.remote = {}; state.offline = {};
    state.remoteFrozen = false; state.attemptStarted = false; state.lastShown = WindowsPlaytestSubmissionResult::None;
    state.scrollY = 0;
    EnableEditableControls(window, state, true);
    for (const int control : {id::SubmitConsent, id::IncludeDiagnostics, id::IncludeScreenshot})
        SendMessageW(Control(window, control), BM_SETCHECK, BST_UNCHECKED, 0);
    SendMessageW(Control(window, id::Category), CB_SETCURSEL, static_cast<WPARAM>(-1), 0);
    SendMessageW(Control(window, id::Impact), CB_SETCURSEL, static_cast<WPARAM>(-1), 0);
    SetWindowTextW(Control(window, id::Note), L"");
    EnableWindow(GetDlgItem(window, id::Send), !state.submission.IsBusy());
    EnableWindow(GetDlgItem(window, id::Retry), FALSE);
    InvalidateRect(Control(window, id::Preview), nullptr, TRUE);
    Layout(window, state);
    SetStatus(window, L"New report draft. Consent and optional diagnostics/image remain off.");
}

bool ConfirmClose(HWND window, DialogState& state)
{
    const bool busy = state.submission.IsBusy();
    const auto result = state.submission.LastResult();
    if (!busy && result != WindowsPlaytestSubmissionResult::Uncertain && !state.submission.CanRetry()) return true;
    const wchar_t* message = busy
        ? L"A report attempt is still in flight. Cancel signals the transport but cannot recall a report already accepted. Close and discard the local retry copy?"
        : L"Delivery may be uncertain or retryable. Closing discards this local same-ID retry copy; a report already received cannot be recalled. Close?";
    if (MessageBoxW(window, message, kDialogTitle, MB_OKCANCEL | MB_ICONWARNING) != IDOK) return false;
    if (busy) state.submission.CancelAttempt();
    return true;
}

LRESULT CALLBACK ReportWindowProc(HWND window, const UINT message, const WPARAM wParam, const LPARAM lParam)
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
        state->pollingAvailable = SetTimer(window, kPollTimer, kPollIntervalMs, nullptr) != 0;
        if (!state->pollingAvailable)
        {
            EnableWindow(GetDlgItem(window, id::Send), FALSE);
            EnableWindow(GetDlgItem(window, id::Retry), FALSE);
            SetStatus(window, L"The bounded completion monitor is unavailable. Remote submission is disabled.");
        }
        return 0;
    }
    switch (message)
    {
    case WM_SIZE: if (state) Layout(window, *state); return 0;
    case kEnsureControlVisibleMessage:
        if (state != nullptr)
            EnsureControlVisible(window, *state, reinterpret_cast<HWND>(wParam));
        return 0;
    case WM_VSCROLL:
        if (state != nullptr)
        {
            SCROLLINFO info{}; info.cbSize = sizeof(info); info.fMask = SIF_ALL;
            GetScrollInfo(window, SB_VERT, &info);
            int next = state->scrollY;
            switch (LOWORD(wParam))
            {
            case SB_LINEUP: next -= Scale(window, 36); break;
            case SB_LINEDOWN: next += Scale(window, 36); break;
            case SB_PAGEUP: next -= static_cast<int>(info.nPage); break;
            case SB_PAGEDOWN: next += static_cast<int>(info.nPage); break;
            case SB_THUMBTRACK: next = info.nTrackPos; break;
            case SB_TOP: next = 0; break;
            case SB_BOTTOM: next = state->bodyHeight; break;
            default: return 0;
            }
            state->scrollY = std::clamp(next, 0, std::max(0, state->bodyHeight - state->bodyViewportHeight));
            Layout(window, *state);
        }
        return 0;
    case WM_MOUSEWHEEL:
        if (state != nullptr)
        {
            state->scrollY -= GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA * Scale(window, 48);
            Layout(window, *state);
            return 0;
        }
        break;
    case WM_GETMINMAXINFO:
    {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = Scale(window, 400);
        info->ptMinTrackSize.y = Scale(window, 350);
        return 0;
    }
    case WM_DPICHANGED:
    {
        const auto* rect = reinterpret_cast<const RECT*>(lParam);
        SetPropW(window, reinterpret_cast<LPCWSTR>(static_cast<ULONG_PTR>(kDpiProperty)),
            reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(HIWORD(wParam))));
        SetWindowPos(window, nullptr, rect->left, rect->top, rect->right - rect->left,
            rect->bottom - rect->top, SWP_NOACTIVATE | SWP_NOZORDER);
        ApplyFont(window); if (state) Layout(window, *state); return 0;
    }
    case WM_TIMER:
        if (state != nullptr && wParam == kPollTimer) { Poll(window, *state); return 0; }
        break;
    case kWindowsPlaytestSubmissionCompletedMessage:
        if (state != nullptr)
        {
            WindowsPlaytestSubmissionResult result = WindowsPlaytestSubmissionResult::None;
            (void)state->submission.HandleMessage(message, wParam, result);
            Poll(window, *state);
            return 0;
        }
        break;
    case WM_NEXTDLGCTL:
        if (state != nullptr)
        {
            HWND target = lParam != 0
                ? reinterpret_cast<HWND>(wParam)
                : NextTabControl(window, GetFocus(), wParam != 0);
            if (target != nullptr && IsWindowEnabled(target)) FocusTabControl(window, *state, target);
            return 0;
        }
        break;
    case WM_COMMAND:
        if (state == nullptr) break;
        if (LOWORD(wParam) == id::Send) { StartRemoteAttempt(window, *state); return 0; }
        if (LOWORD(wParam) == id::Retry)
        {
            if (state->attemptStarted && !state->submission.CanRetry()) return 0;
            StartRemoteAttempt(window, *state);
            return 0;
        }
        if (LOWORD(wParam) == id::OfflineExport) { ExportOffline(window, *state); return 0; }
        if (LOWORD(wParam) == id::NewReport) { ClearForNew(window, *state); return 0; }
        if (LOWORD(wParam) == id::Close) { if (ConfirmClose(window, *state)) DestroyWindow(window); return 0; }
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE && state != nullptr)
        {
            if (ConfirmClose(window, *state)) DestroyWindow(window);
            return 0;
        }
        break;
    case WM_CLOSE:
        if (state == nullptr || ConfirmClose(window, *state)) DestroyWindow(window);
        return 0;
    case WM_NCDESTROY:
        KillTimer(window, kPollTimer);
        const auto fontProperty = reinterpret_cast<LPCWSTR>(static_cast<ULONG_PTR>(kFontProperty));
        RemovePropW(window, reinterpret_cast<LPCWSTR>(static_cast<ULONG_PTR>(kDpiProperty)));
        if (HFONT font = reinterpret_cast<HFONT>(RemovePropW(window, fontProperty))) DeleteObject(font);
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
    WNDCLASSEXW body{};
    body.cbSize = sizeof(body);
    body.style = CS_HREDRAW | CS_VREDRAW;
    body.lpfnWndProc = BodyContentProc;
    body.hInstance = GetModuleHandleW(nullptr);
    body.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    body.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    body.lpszClassName = kBodyContentClass;
    if (RegisterClassExW(&body) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
    registered = true;
    return true;
}
} // namespace

RECT FitWindowsPlaytestPreviewRect(const RECT& bounds, const std::uint32_t imageWidth,
    const std::uint32_t imageHeight) noexcept
{
    const int boxWidth = std::max(0, static_cast<int>(bounds.right - bounds.left));
    const int boxHeight = std::max(0, static_cast<int>(bounds.bottom - bounds.top));
    if (boxWidth == 0 || boxHeight == 0 || imageWidth == 0u || imageHeight == 0u)
        return {bounds.left, bounds.top, bounds.left, bounds.top};
    int width = 0, height = 0;
    if (static_cast<std::uint64_t>(boxWidth) * imageHeight <=
        static_cast<std::uint64_t>(boxHeight) * imageWidth)
    {
        width = boxWidth;
        height = std::max(1, static_cast<int>(static_cast<std::uint64_t>(imageHeight) *
            static_cast<unsigned>(boxWidth) / imageWidth));
    }
    else
    {
        height = boxHeight;
        width = std::max(1, static_cast<int>(static_cast<std::uint64_t>(imageWidth) *
            static_cast<unsigned>(boxHeight) / imageHeight));
    }
    const int left = bounds.left + (boxWidth - width) / 2;
    const int top = bounds.top + (boxHeight - height) / 2;
    return {left, top, left + width, top + height};
}

void ShowWindowsRemotePlaytestReport(HWND owner, WindowsPlaytestReportContext context,
    WindowsRemotePlaytestServices services)
{
    if (!RegisterWindowClass()) return;
    DialogState state(std::move(context), std::move(services));
    const UINT dpi = GetDpiForWindow(owner);
    const int scale = static_cast<int>(dpi == 0u ? 96u : dpi);
    RECT rect{0, 0, MulDiv(760, scale, 96), MulDiv(820, scale, 96)};
    constexpr DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN | WS_VSCROLL;
    AdjustWindowRectEx(&rect, style, FALSE, WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT);
    RECT ownerRect{};
    GetWindowRect(owner, &ownerRect);
    const HMONITOR monitor = MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{}; monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfoW(monitor, &monitorInfo);
    const int availableWidth = std::max(1, static_cast<int>(monitorInfo.rcWork.right - monitorInfo.rcWork.left) - Scale(owner, 24));
    const int availableHeight = std::max(1, static_cast<int>(monitorInfo.rcWork.bottom - monitorInfo.rcWork.top) - Scale(owner, 24));
    const int width = std::min(static_cast<int>(rect.right - rect.left), availableWidth);
    const int height = std::min(static_cast<int>(rect.bottom - rect.top), availableHeight);
    const int x = std::clamp(ownerRect.left + ((ownerRect.right - ownerRect.left) - width) / 2,
        monitorInfo.rcWork.left + Scale(owner, 12), monitorInfo.rcWork.right - width - Scale(owner, 12));
    const int y = std::clamp(ownerRect.top + ((ownerRect.bottom - ownerRect.top) - height) / 2,
        monitorInfo.rcWork.top + Scale(owner, 12), monitorInfo.rcWork.bottom - height - Scale(owner, 12));
    HWND window = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT,
        kWindowClass, kDialogTitle, style, x, y, width, height, owner, nullptr,
        GetModuleHandleW(nullptr), &state);
    if (window == nullptr) return;
    EnableWindow(owner, FALSE);
    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);
    SetFocus(Control(window, id::Category));
    MSG message{};
    while (IsWindow(window))
    {
        const BOOL result = GetMessageW(&message, nullptr, 0, 0);
        if (result <= 0)
        {
            if (result == 0) PostQuitMessage(static_cast<int>(message.wParam));
            break;
        }
        if (message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE)
        {
            SendMessageW(window, WM_KEYDOWN, VK_ESCAPE, 0);
            continue;
        }
        if (message.message == WM_KEYDOWN && message.wParam == VK_TAB && message.hwnd != nullptr &&
            IsChild(window, message.hwnd))
        {
            const bool reverse = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            FocusTabControl(window, state, NextTabControl(window, GetFocus(), reverse));
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
