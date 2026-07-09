#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "ui/DiagnosticOverlay.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/RtCapabilityReport.h"

namespace
{

constexpr const char* kReportDirectory = "reports";
constexpr const char* kTextReportFilename = "vulkan_capability_report.txt";
constexpr const char* kJsonReportFilename = "vulkan_capability_report.json";

bool WriteReport(const std::filesystem::path& path, const std::string& data)
{
    std::ofstream stream(path, std::ios::binary);
    if (!stream.good())
    {
        return false;
    }

    stream << data;
    return stream.good();
}

void PrintDiagnosticLines(const std::vector<std::string>& lines)
{
    for (const std::string& line : lines)
    {
        std::cout << line << '\n';
    }
}

void PrintRunSummary(const std::string& textReport, const std::string& jsonReportPath,
                    const std::string& txtReportPath, const bool initialised)
{
    std::cout << "Capability probe complete.\n";
    std::cout << "Stored report (text): " << txtReportPath << '\n';
    std::cout << "Stored report (json): " << jsonReportPath << '\n';
    std::cout << "Initialisation status: " << (initialised ? "OK" : "Failed, but report still emitted from fallback data.") << '\n';
    std::cout << '\n';
    std::cout << textReport << '\n';
}

} // namespace

int main()
{
    using namespace horde::vulkan;

    VulkanContext context;
    const bool initialised = context.InitialiseForCapabilityProbe();
    const DeviceCapabilities capabilities = context.QueryDeviceCapabilities();

    std::cout << "=== Horde RT Capability Probe ===\n\n";
    PrintDiagnosticLines(context.GetDiagnosticLog());
    std::cout << '\n';

    const std::string textReport = horde::ui::BuildDiagnosticOverlayText(capabilities);
    const std::string jsonReport = BuildCapabilityJsonReport(capabilities);

    const std::filesystem::path reportDirectory = kReportDirectory;
    std::error_code error;
    std::filesystem::create_directories(reportDirectory, error);
    if (error)
    {
        std::cerr << "Failed to create report directory '" << kReportDirectory << "': " << error.message() << '\n';
        return 1;
    }

    const std::filesystem::path textReportPath = reportDirectory / kTextReportFilename;
    const std::filesystem::path jsonReportPath = reportDirectory / kJsonReportFilename;

    if (!WriteReport(textReportPath, textReport))
    {
        std::cerr << "Failed to write text report to " << textReportPath << '\n';
        return 1;
    }

    if (!WriteReport(jsonReportPath, jsonReport))
    {
        std::cerr << "Failed to write JSON report to " << jsonReportPath << '\n';
        return 1;
    }

    std::cout << '\n';
    PrintRunSummary(textReport, jsonReportPath.string(), textReportPath.string(), initialised);

    return initialised ? 0 : 1;
}
