#include <android/log.h>
#include <android/native_activity.h>
#include <jni.h>

#include <cerrno>
#include <fstream>
#include <string>
#include <sys/stat.h>

#include "vulkan/RtCapabilityReport.h"
#include "vulkan/VulkanContext.h"

namespace
{

constexpr const char* kLogTag = "HordeRTCapabilityProbe";
constexpr const char* kReportDirectory = "reports";
constexpr const char* kTextReportFilename = "vulkan_capability_report.txt";
constexpr const char* kJsonReportFilename = "vulkan_capability_report.json";

std::string BuildAppReportPath(const char* internalDataPath)
{
    const std::string reportRoot = std::string(internalDataPath ? internalDataPath : ".");
    return reportRoot + "/" + kReportDirectory;
}

bool EnsureReportDirectory(const std::string& reportDirectory)
{
    return mkdir(reportDirectory.c_str(), 0755) == 0 || errno == EEXIST;
}

bool WriteReport(const std::string& path, const std::string& data)
{
    std::ofstream stream(path, std::ios::binary);
    if (!stream.good())
    {
        return false;
    }

    stream << data;
    return stream.good();
}

void ShowToastSummary(ANativeActivity* activity, const std::string& text)
{
    if (!activity || !activity->vm)
    {
        return;
    }

    const std::string summary = text.size() > 1100 ? text.substr(0, 1100) + "..." : text;

    JavaVM* javaVm = activity->vm;
    JNIEnv* env = nullptr;
    bool attached = false;

    if (javaVm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_EDETACHED)
    {
        if (javaVm->AttachCurrentThread(&env, nullptr) != JNI_OK || env == nullptr)
        {
            return;
        }
        attached = true;
    }

    jclass toastClass = env->FindClass("android/widget/Toast");
    if (!toastClass)
    {
        if (attached)
        {
            javaVm->DetachCurrentThread();
        }
        return;
    }

    const jmethodID makeTextMethod = env->GetStaticMethodID(
        toastClass,
        "makeText",
        "(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;"
    );
    const jmethodID showMethod = env->GetMethodID(toastClass, "show", "()V");
    if (makeTextMethod == nullptr || showMethod == nullptr)
    {
        if (attached)
        {
            javaVm->DetachCurrentThread();
        }
        return;
    }

    jstring message = env->NewStringUTF(summary.c_str());
    jobject toastObj = env->CallStaticObjectMethod(toastClass, makeTextMethod, activity->clazz, message, 1);
    if (toastObj)
    {
        env->CallVoidMethod(toastObj, showMethod);
    }

    env->DeleteLocalRef(message);
    if (attached)
    {
        javaVm->DetachCurrentThread();
    }
}

void RunCapabilityProbe(ANativeActivity* activity)
{
    if (!activity)
    {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "ANativeActivity is null.");
        return;
    }

    if (!activity->internalDataPath)
    {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "internalDataPath is null.");
        return;
    }

    horde::vulkan::VulkanContext context;
    const bool initialised = context.InitialiseForCapabilityProbe();
    const horde::vulkan::DeviceCapabilities capabilities = context.QueryDeviceCapabilities();

    const std::string textReport = horde::vulkan::BuildCapabilityTextReport(capabilities);
    const std::string jsonReport = horde::vulkan::BuildCapabilityJsonReport(capabilities);
    const std::string reportDirectory = BuildAppReportPath(activity->internalDataPath);
    const std::string textReportPath = reportDirectory + "/" + kTextReportFilename;
    const std::string jsonReportPath = reportDirectory + "/" + kJsonReportFilename;

    if (!EnsureReportDirectory(reportDirectory))
    {
        const std::string errnoMessage = "Failed to create report directory: " + reportDirectory;
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "%s", errnoMessage.c_str());
        return;
    }

    if (!WriteReport(textReportPath, textReport))
    {
        const std::string errnoMessage = "Failed to write " + textReportPath;
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "%s", errnoMessage.c_str());
        return;
    }

    if (!WriteReport(jsonReportPath, jsonReport))
    {
        const std::string errnoMessage = "Failed to write " + jsonReportPath;
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "%s", errnoMessage.c_str());
        return;
    }

    const std::string screenText = std::string("RT mode: ")
        + horde::vulkan::ToString(capabilities.rtMode)
        + "\n" + capabilities.identity.gpuName
        + "\n" + (initialised ? "Initialised" : "Initialised with fallback");

    ShowToastSummary(activity, screenText);

    __android_log_print(ANDROID_LOG_INFO, kLogTag, "Probe complete; wrote reports to %s and %s", textReportPath.c_str(), jsonReportPath.c_str());
    __android_log_print(ANDROID_LOG_INFO, kLogTag, "Initialised: %s", initialised ? "yes" : "no");
}

} // namespace

extern "C" void ANativeActivity_onCreate(ANativeActivity* activity, void* savedState, std::size_t savedStateSize)
{
    (void)savedState;
    (void)savedStateSize;

    RunCapabilityProbe(activity);
}
