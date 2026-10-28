#pragma once
#include "platform/windows/WindowsPlaytestReport.h"
#include "reporting/CompatibilityReport.h"
#include "platform/windows/WindowsStatisticsSubmission.h"

namespace horde::platform::windows
{
inline constexpr int kCompatibilityConsent=6101, kCompatibilityPrepare=6102,
    kCompatibilityPreview=6103, kCompatibilityCopy=6104, kCompatibilitySave=6105,
    kCompatibilityClose=6106, kCompatibilityStatus=6107,
    kCompatibilitySendConsent=6109, kCompatibilitySendPreview=6110, kCompatibilitySend=6111, kCompatibilityCancelSend=6112;
using WindowsCompatibilityCopy = bool (*)(HWND,const std::string&,std::wstring&);
struct WindowsCompatibilityServices
{
    WindowsPlaytestReportExporter exporter=nullptr;
    WindowsCompatibilityCopy copy=nullptr;
    WindowsStatisticsSendServices send;
};
// Creation is hidden and does not activate, pump, probe, write or send. The
// ordinary Show owner opts into presentation only after a user action/failure.
[[nodiscard]] HWND CreateWindowsCompatibilityReportView(HWND,
    reporting::CompatibilityReportSnapshot, WindowsCompatibilityServices={}, bool failure=false);
void ShowWindowsCompatibilityReport(HWND,reporting::CompatibilityReportSnapshot,
    WindowsCompatibilityServices={},bool failure=false);
} // namespace horde::platform::windows
