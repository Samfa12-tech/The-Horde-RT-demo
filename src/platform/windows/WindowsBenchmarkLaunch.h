#pragma once

#include <span>
#include <optional>
#include <string>
#include <string_view>
#include "gameplay/BenchmarkWorkload.h"
#include "vulkan/raytracing/RtSceneTuning.h"

namespace horde::platform::windows
{

struct WindowsBenchmarkLaunch
{
    bool requested = false;
    std::wstring outputDirectory;
    std::string error;
    horde::gameplay::BenchmarkWorkload workload = horde::gameplay::BenchmarkWorkload::ShowcaseRoute;
    horde::vulkan::raytracing::RtWorkloadPreset rtWorkloadPreset =
        horde::vulkan::raytracing::RtWorkloadPreset::Authored;
};

// Arguments exclude argv[0]. This only selects the existing player benchmark;
// the optional RT selector uses existing shared tuning, with no checkpoint or
// diagnostic injection. Max cost is the whole preset, not isolated shadow cost.
inline WindowsBenchmarkLaunch ParseWindowsBenchmarkLaunch(
    const std::span<const std::wstring_view> arguments)
{
    WindowsBenchmarkLaunch result;
    bool captureOrCheckpoint = false;
    bool workloadSpecified = false;
    bool rtWorkloadSpecified = false;
    for (std::size_t index = 0u; index < arguments.size(); ++index)
    {
        const auto argument = arguments[index];
        if (argument == L"--benchmark-rt-workload")
        {
            if (rtWorkloadSpecified || ++index == arguments.size())
            {
                result.error = "--benchmark-rt-workload requires one unique authored or max value.";
                return result;
            }
            rtWorkloadSpecified = true;
            if (arguments[index] == L"authored")
                result.rtWorkloadPreset = horde::vulkan::raytracing::RtWorkloadPreset::Authored;
            else if (arguments[index] == L"max")
                result.rtWorkloadPreset = horde::vulkan::raytracing::RtWorkloadPreset::Max;
            else
            {
                result.error = "Unknown benchmark RT workload; use authored or max.";
                return result;
            }
            continue;
        }
        if (argument == L"--benchmark-workload")
        {
            if (workloadSpecified || ++index == arguments.size())
            {
                result.error = "--benchmark-workload requires one unique workload name.";
                return result;
            }
            workloadSpecified = true;
            bool found = false;
            for (const auto workload : horde::gameplay::kBenchmarkWorkloads)
            {
                const auto name = horde::gameplay::BenchmarkWorkloadName(workload);
                if (arguments[index] == std::wstring(name.begin(), name.end()))
                {
                    result.workload = workload;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                result.error = "Unknown versioned benchmark workload.";
                return result;
            }
            continue;
        }
        if (argument == L"--capture-showcase" || argument == L"--development-checkpoint" ||
            argument == L"--capture-graphics-preview" || argument == L"--validate-output-resize" ||
            argument == L"--debug-rt-lab" || argument.starts_with(L"--rt-lab-"))
        {
            captureOrCheckpoint = true;
        }
        if (argument != L"--benchmark-showcase")
        {
            continue;
        }
        if (result.requested)
        {
            result.error = "--benchmark-showcase may only be specified once.";
            return result;
        }
        result.requested = true;
        if (++index == arguments.size() || arguments[index].empty() ||
            arguments[index].front() == L'-')
        {
            result.error = "--benchmark-showcase requires an output directory.";
            return result;
        }
        result.outputDirectory = arguments[index];
    }
    if (result.requested && captureOrCheckpoint)
    {
        result.error = "--benchmark-showcase cannot be combined with capture, checkpoint, preview, resize or Debug RT Lab mutation.";
    }
    if (workloadSpecified && !result.requested)
        result.error = "--benchmark-workload requires --benchmark-showcase.";
    if (rtWorkloadSpecified && !result.requested)
        result.error = "--benchmark-rt-workload requires --benchmark-showcase.";
    return result;
}

inline std::string_view WindowsBenchmarkRtWorkloadName(
    const horde::vulkan::raytracing::RtWorkloadPreset preset)
{
    using horde::vulkan::raytracing::RtWorkloadPreset;
    switch (preset)
    {
    case RtWorkloadPreset::Lean: return "lean";
    case RtWorkloadPreset::Authored: return "authored";
    case RtWorkloadPreset::Max: return "max";
    default: return "unknown";
    }
}

// Windows-only evidence attachment. Samples describe the actual compiled shared
// shader policy per contributing receiver, not observed dynamic query counters.
inline std::string BuildWindowsBenchmarkTuningJson(
    const horde::vulkan::raytracing::RtWorkloadPreset requested,
    const horde::vulkan::raytracing::RtWorkloadPreset effectiveAtStart,
    const horde::vulkan::raytracing::RtWorkloadPreset effectiveAtEnd,
    const std::string_view compiledQualityAtStart,
    const std::string_view compiledQualityAtEnd,
    const std::optional<horde::vulkan::raytracing::RtQualityControlsGpu> uploadedAtStart = std::nullopt,
    const std::optional<horde::vulkan::raytracing::RtQualityControlsGpu> uploadedAtEnd = std::nullopt,
    const bool requireUploadedPolicy = false)
{
    const bool knownQuality = compiledQualityAtEnd == "High" || compiledQualityAtEnd == "Mobile";
    const auto validUpload = [](
        const std::optional<horde::vulkan::raytracing::RtQualityControlsGpu>& upload,
        const horde::vulkan::raytracing::RtWorkloadPreset preset, const std::string_view quality) {
        if (!upload || (quality != "High" && quality != "Mobile") || upload->controls[0] > 3u || upload->controls[3] > 1u) return false;
        std::optional<horde::graphics::ShadowQuality> shadow;
        if (upload->controls[0] < 3u) shadow = static_cast<horde::graphics::ShadowQuality>(upload->controls[0]);
        const auto expected = horde::vulkan::raytracing::ResolveRtQualityControls(shadow, preset, quality == "High", upload->controls[3] == 0u);
        return expected && expected->controls == upload->controls;
    };
    const bool startUploadValid = validUpload(uploadedAtStart, effectiveAtStart, compiledQualityAtStart);
    const bool endUploadValid = validUpload(uploadedAtEnd, effectiveAtEnd, compiledQualityAtEnd);
    // Only historical/non-Graphics fixtures explicitly omit both records.
    // Ordinary owners require uploaded policy and never guess it from Max.
    const bool legacy = !requireUploadedPolicy && !uploadedAtStart && !uploadedAtEnd;
    const bool stable = requested == effectiveAtStart && effectiveAtStart == effectiveAtEnd &&
        knownQuality && compiledQualityAtStart == compiledQualityAtEnd &&
        (legacy || (startUploadValid && endUploadValid && uploadedAtStart->controls == uploadedAtEnd->controls));
    const auto samples = endUploadValid ? uploadedAtEnd->controls[1] : legacy && knownQuality
        ? horde::vulkan::raytracing::ResolvePrimaryAreaShadowSamples(effectiveAtEnd, compiledQualityAtEnd == "High") : 0u;
    const auto skySamples = endUploadValid ? uploadedAtEnd->controls[2] : legacy && knownQuality
        ? (effectiveAtEnd == horde::vulkan::raytracing::RtWorkloadPreset::Max ? 2u : 1u) : 0u;
    const char* shadowMode = endUploadValid ? (uploadedAtEnd->controls[0] == 0u ? "Lower" :
        uploadedAtEnd->controls[0] == 1u ? "Current" : uploadedAtEnd->controls[0] == 2u ? "Higher" : "DiagnosticLegacy") :
        legacy && knownQuality ? "DiagnosticLegacy" : "unavailable";
    const auto qualityName = [](const std::string_view name) -> std::string_view {
        return name == "High" || name == "Mobile" ? name : "unavailable";
    };
    return "{\"schema\":1,\"requestedPreset\":\"" + std::string(WindowsBenchmarkRtWorkloadName(requested)) +
        "\",\"effectivePresetAtStart\":\"" + std::string(WindowsBenchmarkRtWorkloadName(effectiveAtStart)) +
        "\",\"effectivePresetAtEnd\":\"" + std::string(WindowsBenchmarkRtWorkloadName(effectiveAtEnd)) +
        "\",\"compiledQualityAtStart\":\"" + std::string(qualityName(compiledQualityAtStart)) +
        "\",\"compiledQualityAtEnd\":\"" + std::string(qualityName(compiledQualityAtEnd)) +
        "\",\"policyStable\":" + (stable ? "true" : "false") +
        ",\"shadowPolicySource\":\"" + (endUploadValid ? "uploaded" : legacy ? "explicit-diagnostic-legacy-workload" : "unavailable") +
        "\",\"shadowMode\":\"" + shadowMode + "\"" +
        ",\"primaryAreaShadowSamplesPerContributingReceiver\":" + std::to_string(samples) +
        ",\"sampleDomain\":\"contributing-primary-local-and-fire-area-lights\",\"primarySkyVisibilitySamples\":" +
        std::to_string(skySamples) +
        (endUploadValid ? ",\"actualUploadedMistEnabled\":" + std::string(uploadedAtEnd->controls[3] == 0u ? "true" : "false") : "") +
        ",\"secondaryAreaShadowSamples\":1,\"sampleCountMeaning\":\"compiled-physical-policy-not-dynamic-query-counts\","
        "\"costMeaning\":\"whole-rt-workload-preset-not-isolated-shadow-cost\"}";
}

} // namespace horde::platform::windows
