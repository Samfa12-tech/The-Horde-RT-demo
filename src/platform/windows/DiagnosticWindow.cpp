#include "platform/windows/DiagnosticWindow.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <windows.h>

#include "ui/DiagnosticOverlay.h"
#include "vulkan/RtCapabilityReport.h"
#include "vulkan/VulkanContext.h"

namespace
{

constexpr char kWindowClassName[] = "HordeRtDiagnosticWindowClass";
constexpr char kWindowTitle[] = "Horde Lantern RT Diagnostic";
constexpr char kReportDirectory[] = "reports";
constexpr char kTextReportFilename[] = "vulkan_capability_report.txt";
constexpr char kJsonReportFilename[] = "vulkan_capability_report.json";
constexpr int kEditControlId = 101;

std::string BuildDisplayText(const horde::vulkan::DeviceCapabilities& capabilities)
{
    if (capabilities.rtMode == horde::vulkan::RtMode::Unsupported)
    {
        return horde::ui::BuildUnsupportedDeviceText(capabilities);
    }
    return horde::ui::BuildDiagnosticOverlayText(capabilities);
}

std::string WindowSafeText(const std::string& value)
{
    std::string replaced = value;
    std::string out;
    out.reserve(replaced.size());
    for (const char c : replaced)
    {
        if (c == '\n')
        {
            out += "\r\n";
        }
        else
        {
            out += c;
        }
    }
    return out;
}

bool WriteReportFile(const std::filesystem::path& path, const std::string& data)
{
    std::ofstream stream(path, std::ios::binary);
    if (!stream.good())
    {
        return false;
    }

    stream << data;
    return stream.good();
}

LRESULT CALLBACK DiagnosticWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
    {
        const int width = LOWORD(lParam);
        const int height = HIWORD(lParam);
        HWND child = GetDlgItem(hWnd, kEditControlId);
        if (child)
        {
            MoveWindow(child, 12, 12, width - 24, height - 24, TRUE);
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

int CreateAndShowWindow(const std::string& diagnosticText)
{
    const HINSTANCE instance = GetModuleHandleA(nullptr);
    WNDCLASSA windowClass{};
    windowClass.lpfnWndProc = DiagnosticWindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kWindowClassName;
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassA(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        std::cerr << "Failed to register diagnostic window class." << std::endl;
        return 1;
    }

    HWND hWnd = CreateWindowExA(
        0,
        kWindowClassName,
        kWindowTitle,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1000,
        700,
        nullptr,
        nullptr,
        instance,
        nullptr);
    if (!hWnd)
    {
        std::cerr << "Failed to create diagnostic window." << std::endl;
        return 1;
    }

    HFONT monoFont = CreateFontA(
        18,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        FF_MODERN,
        "Consolas");
    if (!monoFont)
    {
        monoFont = GetStockObject(ANSI_FIXED_FONT);
    }

    RECT clientRect{};
    GetClientRect(hWnd, &clientRect);
    HWND edit = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        "EDIT",
        "",
        WS_CHILD | WS_VISIBLE | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
        12,
        12,
        clientRect.right - clientRect.left - 24,
        clientRect.bottom - clientRect.top - 24,
        hWnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kEditControlId)),
        instance,
        nullptr);
    if (!edit)
    {
        std::cerr << "Failed to create diagnostic text area." << std::endl;
        return 1;
    }

    SendMessageA(edit, WM_SETFONT, reinterpret_cast<WPARAM>(monoFont), TRUE);
    const std::string windowText = WindowSafeText(diagnosticText);
    SetWindowTextA(edit, windowText.c_str());

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    SetForegroundWindow(hWnd);
    SetFocus(edit);

    MSG message{};
    while (GetMessageA(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return static_cast<int>(message.wParam);
}

} // namespace

namespace horde::platform::windows
{

int RunDiagnosticWindow(const int showCommand)
{
    (void)showCommand;

    horde::vulkan::VulkanContext context;
    const bool initialised = context.InitialiseForCapabilityProbe();
    const horde::vulkan::DeviceCapabilities capabilities = context.QueryDeviceCapabilities();

    const std::string diagnosticText = BuildDisplayText(capabilities);
    const std::string textReport = horde::vulkan::BuildCapabilityTextReport(capabilities);
    const std::string jsonReport = horde::vulkan::BuildCapabilityJsonReport(capabilities);

    std::cout << "=== Horde RT Diagnostic Window ===\n";
    std::cout << "Probe initialisation: " << (initialised ? "OK" : "Fallback") << "\n\n";
    std::cout << diagnosticText << "\n\n";

    std::error_code error;
    const std::filesystem::path reportDirectory = kReportDirectory;
    std::filesystem::create_directories(reportDirectory, error);
    if (error)
    {
        std::cerr << "Failed to create report directory '" << kReportDirectory << "': " << error.message() << '\n';
        return 1;
    }

    const std::filesystem::path textReportPath = reportDirectory / kTextReportFilename;
    const std::filesystem::path jsonReportPath = reportDirectory / kJsonReportFilename;

    if (!WriteReportFile(textReportPath, textReport))
    {
        std::cerr << "Failed to write text report to " << textReportPath << '\n';
        return 1;
    }

    if (!WriteReportFile(jsonReportPath, jsonReport))
    {
        std::cerr << "Failed to write JSON report to " << jsonReportPath << '\n';
        return 1;
    }

    std::cout << "Stored report (text): " << textReportPath << '\n';
    std::cout << "Stored report (json): " << jsonReportPath << '\n';

    return CreateAndShowWindow(diagnosticText);
}

} // namespace horde::platform::windows
