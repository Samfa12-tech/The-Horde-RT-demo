#pragma once
#include "platform/windows/WindowsGraphicsPreviewCapture.h"
#include "platform/windows/WindowsBenchmarkLaunch.h"
#include <span>
#include <string>
#include <string_view>

namespace horde::platform::windows
{
struct WindowsMotionEvidenceLaunch
{
    bool requested = false;
    std::wstring outputDirectory;
    std::string scenario;
    std::string error;
    horde::vulkan::raytracing::RtWorkloadPreset rtWorkloadPreset =
        horde::vulkan::raytracing::RtWorkloadPreset::Authored;
};
inline WindowsMotionEvidenceLaunch ParseWindowsMotionEvidenceLaunch(
    std::span<const std::wstring_view> arguments, const bool debugValidationSupported = true)
{
    WindowsMotionEvidenceLaunch result;
    bool scenarioSeen = false;
    bool conflict = false;
    bool unknown = false;
    bool computeSeen = false;
    bool motionArgument = false;
    bool presetSeen = false;
    for (const auto argument : arguments)
        motionArgument |= argument == L"--validate-native-motion" || argument == L"--motion-scenario" ||
                          argument == L"--motion-rt-workload";
    if (motionArgument && arguments.size() > 16)
    { result.error = "Native motion accepts at most sixteen command-line entries."; return result; }
    for (std::size_t i = 0; i < arguments.size(); ++i)
    {
        const auto argument = arguments[i];
        if (argument == L"--validate-native-motion")
        {
            if (result.requested || i+1 == arguments.size() || arguments[i+1].starts_with(L"--"))
            { result.error = "--validate-native-motion requires one unique absolute fresh output directory."; return result; }
            result.requested = true; result.outputDirectory = arguments[++i];
            if (!AbsoluteWindowsCapturePath(result.outputDirectory) || result.outputDirectory.size() > 32700 ||
                result.outputDirectory.starts_with(L"\\\\?\\") || result.outputDirectory.starts_with(L"\\\\.\\"))
            { result.error = "Native motion requires an ordinary absolute Windows output directory."; return result; }
        }
        else if (argument == L"--motion-scenario")
        {
            if (scenarioSeen || i+1 == arguments.size())
            { result.error = "--motion-scenario requires one unique literal scenario."; return result; }
            scenarioSeen = true;
            const auto scenario = arguments[++i];
            for (const auto literal : {"torch-low-opening", "shaft-up", "keeper-first-entry", "keeper-retry-reward", "waterfall-equipment", "torch-drench"})
                if (scenario == std::wstring(literal, literal + std::char_traits<char>::length(literal))) result.scenario = literal;
            if (result.scenario.empty())
            { result.error = "Unknown native motion scenario."; return result; }
        }
        else if (argument == L"--motion-rt-workload")
        {
            if (presetSeen || i+1 == arguments.size())
            { result.error = "--motion-rt-workload requires one unique authored or max value."; return result; }
            presetSeen = true;
            const auto preset = arguments[++i];
            if (preset == L"authored") result.rtWorkloadPreset = horde::vulkan::raytracing::RtWorkloadPreset::Authored;
            else if (preset == L"max") result.rtWorkloadPreset = horde::vulkan::raytracing::RtWorkloadPreset::Max;
            else { result.error = "Unknown motion RT workload; use authored or max."; return result; }
        }
        else if (argument == L"--capture-dust")
        {
            if (i + 1u == arguments.size() ||
                (arguments[i + 1u] != L"off" && arguments[i + 1u] != L"low" && arguments[i + 1u] != L"standard"))
            { result.error = "--capture-dust requires low, standard, or off."; return result; }
            ++i; // This Debug-only still-capture override is validated by the shared capture policy.
        }
        else if (argument == L"--require-rayquery-compute") { unknown |= computeSeen; computeSeen = true; }
        else
        {
            unknown = true;
            if (argument.starts_with(L"--capture") || argument.starts_with(L"--benchmark") ||
                argument.starts_with(L"--rt-lab") || argument == L"--debug-rt-lab" ||
                argument == L"--development-checkpoint" || argument == L"--validate-output-resize" ||
                argument == L"--anatomical-player-mount" || argument.starts_with(L"--report")) conflict = true;
        }
    }
    if (!result.requested && presetSeen) result.error = "--motion-rt-workload requires --validate-native-motion.";
    else if (!result.requested && scenarioSeen) result.error = "--motion-scenario requires --validate-native-motion.";
    else if (result.requested && !scenarioSeen) result.error = "Native motion requires --motion-scenario.";
    else if (result.requested && (conflict || unknown))
        result.error = "Native motion cannot share capture, benchmark, checkpoint, RT Lab, report or unknown options.";
    if (result.error.empty() && result.requested && !debugValidationSupported)
        result.error = "Native motion and its RT workload selector are Debug-only validation controls.";
    return result;
}

// The native owner supplies actual selected-module facts, never a desired build
// profile. Mobile Authored remains valid; Max motion specifically requires High.
inline bool WindowsMotionRtPolicyAdmitted(
    const horde::vulkan::raytracing::RtWorkloadPreset requested,
    const horde::vulkan::raytracing::RtWorkloadPreset effective,
    const std::string_view compiledQuality, const bool selectedArtifactsValid)
{
    using horde::vulkan::raytracing::RtWorkloadPreset;
    if (!selectedArtifactsValid || requested != effective ||
        (compiledQuality != "High" && compiledQuality != "Mobile")) return false;
    if (requested == RtWorkloadPreset::Authored) return true;
    return requested == RtWorkloadPreset::Max && compiledQuality == "High" &&
        horde::vulkan::raytracing::ResolvePrimaryAreaShadowSamples(effective, true) == 4u;
}

inline bool WindowsMotionArtifactIdentityValid(
    const std::string_view key, const std::string_view spirvSha256,
    const std::string_view includeSha256, const std::size_t wordCount,
    const std::string_view quality, const bool compute, const bool opaque)
{
    if (quality != "High" && quality != "Mobile") return false;
    const auto hashValid = [](const std::string_view hash) {
        if (hash.size() != 64u) return false;
        for (const auto value : hash)
            if (!((value >= '0' && value <= '9') || (value >= 'a' && value <= 'f'))) return false;
        return true;
    };
    const std::string prefix = compute ? "rayquery_compute_" : "";
    const std::string suffix = std::string(quality == "High" ? "_high_" : "_mobile_") +
        (opaque ? "opaque_fast" : "generic_dielectric");
    return wordCount > 0u && hashValid(spirvSha256) && hashValid(includeSha256) &&
        (key == prefix + "diagnostic" + suffix || key == prefix + "shipping" + suffix);
}

inline std::string BuildWindowsMotionTuningJson(
    const horde::vulkan::raytracing::RtWorkloadPreset requested,
    const horde::vulkan::raytracing::RtWorkloadPreset start,
    const horde::vulkan::raytracing::RtWorkloadPreset end,
    const std::string_view startQuality, const std::string_view endQuality,
    const std::optional<horde::vulkan::raytracing::RtQualityControlsGpu> uploadedAtStart = std::nullopt,
    const std::optional<horde::vulkan::raytracing::RtQualityControlsGpu> uploadedAtEnd = std::nullopt,
    const bool requireUploadedPolicy = false)
{
    auto json = BuildWindowsBenchmarkTuningJson(requested, start, end, startQuality, endQuality,
        uploadedAtStart, uploadedAtEnd, requireUploadedPolicy);
    json.pop_back();
    const auto mistSamples = end == horde::vulkan::raytracing::RtWorkloadPreset::Max ? 8u :
        end == horde::vulkan::raytracing::RtWorkloadPreset::Authored ? 6u : 0u;
    return json + ",\"lichMistSamplesPerIntersectingRay\":" + std::to_string(mistSamples) +
        ",\"mistSampleMeaning\":\"compiled-preset-budget-not-observed-dynamic-samples\"}";
}
}
