#include <android/log.h>

#include <cerrno>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>

#include <jni.h>

#include "ui/DiagnosticOverlay.h"
#include "vulkan/RtCapabilityReport.h"
#include "vulkan/VulkanContext.h"

namespace
{

constexpr const char* kTag = "HordeRtProbeBridge";
constexpr const char* kReportDirectory = "reports";
constexpr const char* kTextReportFilename = "vulkan_capability_report.txt";
constexpr const char* kJsonReportFilename = "vulkan_capability_report.json";

bool EnsureDirectoryExists(const std::string& path)
{
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
}

bool WriteTextFile(const std::string& path, const std::string& data)
{
    std::ofstream stream(path, std::ios::binary);
    if (!stream.good())
    {
        return false;
    }

    stream << data;
    return stream.good();
}

horde::vulkan::DeviceCapabilities RunProbe()
{
    horde::vulkan::VulkanContext context;
    context.InitialiseForCapabilityProbe();
    return context.QueryDeviceCapabilities();
}

std::string BuildReportDirectory(const std::string& baseDirectory)
{
    return baseDirectory + '/' + kReportDirectory;
}

std::string BuildDisplayText(const horde::vulkan::DeviceCapabilities& capabilities)
{
    if (capabilities.rtMode == horde::vulkan::RtMode::Unsupported)
    {
        return horde::ui::BuildUnsupportedDeviceText(capabilities);
    }
    return horde::ui::BuildDiagnosticOverlayText(capabilities);
}

} // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getTextReport(JNIEnv* env, jclass)
{
    const horde::vulkan::DeviceCapabilities capabilities = RunProbe();
    const std::string reportText = BuildDisplayText(capabilities);
    return env->NewStringUTF(reportText.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getJsonReport(JNIEnv* env, jclass)
{
    const horde::vulkan::DeviceCapabilities capabilities = RunProbe();
    const std::string reportJson = horde::vulkan::BuildCapabilityJsonReport(capabilities);
    return env->NewStringUTF(reportJson.c_str());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_writeReports(JNIEnv* env, jclass, jstring baseDirectory)
{
    const char* baseDirectoryUtf = env->GetStringUTFChars(baseDirectory, nullptr);
    if (!baseDirectoryUtf)
    {
        return JNI_FALSE;
    }

    const std::string baseDirectoryValue(baseDirectoryUtf);
    env->ReleaseStringUTFChars(baseDirectory, baseDirectoryUtf);

    const horde::vulkan::DeviceCapabilities capabilities = RunProbe();
    const std::string textReport = BuildDisplayText(capabilities);
    const std::string jsonReport = horde::vulkan::BuildCapabilityJsonReport(capabilities);

    const std::string reportDirectory = BuildReportDirectory(baseDirectoryValue);
    if (!EnsureDirectoryExists(reportDirectory))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to create report directory: %s", reportDirectory.c_str());
        return JNI_FALSE;
    }

    if (!WriteTextFile(reportDirectory + '/' + kTextReportFilename, textReport))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to write text report file");
        return JNI_FALSE;
    }

    if (!WriteTextFile(reportDirectory + '/' + kJsonReportFilename, jsonReport))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to write JSON report file");
        return JNI_FALSE;
    }

    return JNI_TRUE;
}
