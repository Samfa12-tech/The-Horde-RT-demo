#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstdint>
#include <string>

#include "reporting/PlaytestReport.h"

namespace horde::platform::windows
{

// All strings are owned snapshots captured by the Windows application thread.
// An empty/unavailable context remains a valid note-only report.
struct WindowsPlaytestReportContext
{
    horde::reporting::OwnedPlaytestReportContext values;
    bool available = false;
};

// Shows a modal native report form. It does not resume gameplay or send data
// anywhere; export is an explicit user-selected local JSON save.
// Injectable synchronous foreground destination boundary for native UI tests.
// Null uses the real user-selected picker; no remote or automatic transport.
using WindowsPlaytestReportExporter = bool (*)(HWND, const std::string&, std::wstring&);
// Shared foreground local fallback: explicit picker, bounded prepared JSON,
// checked write/flush/close. No verification or remote transport.
[[nodiscard]] bool ExportWindowsPlaytestReportJson(HWND owner,
    const std::string& json, std::wstring& message);
void ShowWindowsPlaytestReport(HWND owner, WindowsPlaytestReportContext context,
    WindowsPlaytestReportExporter exporter = nullptr);

} // namespace horde::platform::windows
