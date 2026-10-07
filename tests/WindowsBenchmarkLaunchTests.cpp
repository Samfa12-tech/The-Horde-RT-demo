#include <array>
#include <iostream>
#include <string_view>

#include "platform/windows/WindowsBenchmarkLaunch.h"
#include "platform/windows/WindowsMistCaptureLaunch.h"

int main()
{
    using horde::platform::windows::ParseWindowsBenchmarkLaunch;
    bool passed = true;
    const auto check = [&passed](bool condition, const char* message) {
        if (!condition)
        {
            std::cerr << message << '\n';
            passed = false;
        }
    };
    const std::array<std::wstring_view, 0> ordinary{};
    const auto absent = ParseWindowsBenchmarkLaunch(ordinary);
    check(!absent.requested && absent.error.empty(), "ordinary launch must remain interactive");
    using horde::vulkan::raytracing::RtWorkloadPreset;
    check(absent.rtWorkloadPreset == RtWorkloadPreset::Authored,
          "ordinary launches keep the authored preset");
    const std::array<std::wstring_view, 3> benchmark{
        L"--benchmark-showcase", L"C:\\reports\\Horde α", L"--require-rayquery-compute"};
    const auto selected = ParseWindowsBenchmarkLaunch(benchmark);
    check(selected.requested && selected.error.empty() &&
              selected.outputDirectory == L"C:\\reports\\Horde α",
          "benchmark launch must preserve its exact Unicode output directory");
    const std::array<std::wstring_view, 1> missing{L"--benchmark-showcase"};
    check(!ParseWindowsBenchmarkLaunch(missing).error.empty(), "missing output directory must fail");
    const std::array<std::wstring_view, 2> empty{L"--benchmark-showcase", L""};
    check(!ParseWindowsBenchmarkLaunch(empty).error.empty(), "empty output directory must fail");
    const std::array<std::wstring_view, 2> option{L"--benchmark-showcase", L"--other-option"};
    check(!ParseWindowsBenchmarkLaunch(option).error.empty(), "another option is not an output directory");
    const std::array<std::wstring_view, 4> duplicate{
        L"--benchmark-showcase", L"first", L"--benchmark-showcase", L"second"};
    check(!ParseWindowsBenchmarkLaunch(duplicate).error.empty(), "ambiguous duplicate launch must fail");
    for (const auto conflicting : {L"--capture-showcase", L"--development-checkpoint"})
    {
        const std::array<std::wstring_view, 4> before{
            conflicting, L"value", L"--benchmark-showcase", L"reports"};
        const std::array<std::wstring_view, 4> after{
            L"--benchmark-showcase", L"reports", conflicting, L"value"};
        check(!ParseWindowsBenchmarkLaunch(before).error.empty() &&
                  !ParseWindowsBenchmarkLaunch(after).error.empty(),
              "benchmark cannot be combined with checkpoint/capture mutation in either order");
    }
    const std::array<std::wstring_view, 2> capture{L"--capture-showcase", L"reports"};
    check(!ParseWindowsBenchmarkLaunch(capture).requested &&
              ParseWindowsBenchmarkLaunch(capture).error.empty(),
          "existing capture-only launch remains the capture parser's responsibility");
    for (const auto workload : horde::gameplay::kBenchmarkWorkloads)
    {
        const auto name = horde::gameplay::BenchmarkWorkloadName(workload);
        const std::wstring wideName(name.begin(), name.end());
        const std::array<std::wstring_view, 4> args{
            L"--benchmark-showcase", L"reports", L"--benchmark-workload", wideName};
        const auto launch = ParseWindowsBenchmarkLaunch(args);
        check(launch.requested && launch.error.empty() && launch.workload == workload,
              "each exact allowlisted workload must select its shared simulation case");
    }
    const std::array<std::wstring_view, 2> orphan{L"--benchmark-workload", L"lantern-held-high-v1"};
    check(!ParseWindowsBenchmarkLaunch(orphan).error.empty(), "workload must not modify an ordinary launch");
    const std::array<std::wstring_view, 3> missingWorkload{L"--benchmark-showcase", L"reports", L"--benchmark-workload"};
    check(!ParseWindowsBenchmarkLaunch(missingWorkload).error.empty(), "missing workload must fail");
    const std::array<std::wstring_view, 4> unknownWorkload{L"--benchmark-showcase", L"reports", L"--benchmark-workload", L"glass-fast"};
    check(!ParseWindowsBenchmarkLaunch(unknownWorkload).error.empty(), "unknown workload must not silently run the route");
    const std::array<std::wstring_view, 6> duplicateWorkload{L"--benchmark-showcase", L"reports", L"--benchmark-workload", L"lantern-held-high-v1", L"--benchmark-workload", L"lantern-held-low-v1"};
    check(!ParseWindowsBenchmarkLaunch(duplicateWorkload).error.empty(), "duplicate workloads must fail");
    for (const auto value : {L"authored", L"max"})
    {
        const std::array<std::wstring_view, 4> args{
            L"--benchmark-rt-workload", value, L"--benchmark-showcase", L"reports"};
        const auto launch = ParseWindowsBenchmarkLaunch(args);
        check(launch.requested && launch.error.empty() &&
                  launch.rtWorkloadPreset == (std::wstring_view(value) == L"max" ? RtWorkloadPreset::Max : RtWorkloadPreset::Authored) &&
                  launch.workload == horde::gameplay::BenchmarkWorkload::ShowcaseRoute,
              "explicit existing RT preset works before the launch flag without changing the gameplay workload");
    }
    const std::array<std::wstring_view, 6> selectedLantern{
        L"--benchmark-showcase", L"reports", L"--benchmark-workload", L"lantern-held-high-v1", L"--benchmark-rt-workload", L"max"};
    check(ParseWindowsBenchmarkLaunch(selectedLantern).error.empty() &&
              ParseWindowsBenchmarkLaunch(selectedLantern).rtWorkloadPreset == RtWorkloadPreset::Max &&
              ParseWindowsBenchmarkLaunch(selectedLantern).workload == horde::gameplay::BenchmarkWorkload::LanternHeldHigh,
          "RT preset and versioned gameplay workload are independent selectors");
    const std::array<std::wstring_view, 2> orphanRt{L"--benchmark-rt-workload", L"max"};
    check(!ParseWindowsBenchmarkLaunch(orphanRt).error.empty(), "RT selector cannot silently alter an ordinary launch");
    const std::array<std::wstring_view, 3> missingRt{L"--benchmark-showcase", L"reports", L"--benchmark-rt-workload"};
    check(!ParseWindowsBenchmarkLaunch(missingRt).error.empty(), "missing RT selector value fails");
    for (const auto value : {L"", L"lean", L"MAX", L"high", L"--require-rayquery-compute"})
    {
        const std::array<std::wstring_view, 4> args{L"--benchmark-showcase", L"reports", L"--benchmark-rt-workload", value};
        check(!ParseWindowsBenchmarkLaunch(args).error.empty(), "unknown RT selector never silently runs authored");
    }
    const std::array<std::wstring_view, 6> duplicateRt{
        L"--benchmark-showcase", L"reports", L"--benchmark-rt-workload", L"max", L"--benchmark-rt-workload", L"max"};
    check(!ParseWindowsBenchmarkLaunch(duplicateRt).error.empty(), "even repeated identical RT selector values are ambiguous");
    for (const auto conflicting : {L"--debug-rt-lab", L"--rt-lab-workload", L"--capture-graphics-preview", L"--validate-output-resize"})
    {
        const std::array<std::wstring_view, 4> args{L"--benchmark-showcase", L"reports", conflicting, L"max"};
        check(!ParseWindowsBenchmarkLaunch(args).error.empty(), "normal benchmark rejects diagnostic/preview/resize injection");
    }
    const auto highMax = horde::platform::windows::BuildWindowsBenchmarkTuningJson(
        RtWorkloadPreset::Max, RtWorkloadPreset::Max, RtWorkloadPreset::Max, "High", "High");
    check(highMax.find("\"policyStable\":true") != std::string::npos &&
              highMax.find("\"primaryAreaShadowSamplesPerContributingReceiver\":4") != std::string::npos &&
              highMax.find("contributing-primary-local-and-fire-area-lights") != std::string::npos &&
              highMax.find("\"primarySkyVisibilitySamples\":2") != std::string::npos &&
              highMax.find("whole-rt-workload-preset-not-isolated-shadow-cost") != std::string::npos &&
              highMax.find("compiled-physical-policy-not-dynamic-query-counts") != std::string::npos,
          "High Max evidence states physical four-sample policy and honest whole-preset cost");
    const auto mobileMax = horde::platform::windows::BuildWindowsBenchmarkTuningJson(
        RtWorkloadPreset::Max, RtWorkloadPreset::Max, RtWorkloadPreset::Max, "Mobile", "Mobile");
    check(mobileMax.find("\"primaryAreaShadowSamplesPerContributingReceiver\":2") != std::string::npos,
          "Mobile Max remains its separate two-sample policy");
    const auto authored = horde::platform::windows::BuildWindowsBenchmarkTuningJson(
        RtWorkloadPreset::Authored, RtWorkloadPreset::Authored, RtWorkloadPreset::Authored, "High", "High");
    check(authored.find("\"primaryAreaShadowSamplesPerContributingReceiver\":1") != std::string::npos &&
              authored.find("\"policyStable\":true") != std::string::npos,
          "default Authored reports its unchanged one-sample primary policy");
    const auto changedPreset = horde::platform::windows::BuildWindowsBenchmarkTuningJson(
        RtWorkloadPreset::Max, RtWorkloadPreset::Max, RtWorkloadPreset::Authored, "High", "High");
    const auto changedQuality = horde::platform::windows::BuildWindowsBenchmarkTuningJson(
        RtWorkloadPreset::Max, RtWorkloadPreset::Max, RtWorkloadPreset::Max, "High", "Mobile");
    const auto unavailable = horde::platform::windows::BuildWindowsBenchmarkTuningJson(
        RtWorkloadPreset::Max, RtWorkloadPreset::Max, RtWorkloadPreset::Max, "", "");
    check(changedPreset.find("\"policyStable\":false") != std::string::npos &&
              changedQuality.find("\"policyStable\":false") != std::string::npos &&
              unavailable.find("\"policyStable\":false") != std::string::npos &&
              unavailable.find("\"primaryAreaShadowSamplesPerContributingReceiver\":0") != std::string::npos,
          "changed or unavailable actual shader policy never appears stable");
    const horde::vulkan::raytracing::RtQualityControlsGpu current{{1u, 1u, 1u, 0u}};
    const horde::vulkan::raytracing::RtQualityControlsGpu lower{{0u, 1u, 1u, 0u}};
    const horde::vulkan::raytracing::RtQualityControlsGpu higher{{2u, 4u, 2u, 0u}};
    const auto independentCurrentMax = horde::platform::windows::BuildWindowsBenchmarkTuningJson(
        RtWorkloadPreset::Max, RtWorkloadPreset::Max, RtWorkloadPreset::Max, "High", "High", current, current, true);
    check(independentCurrentMax.find("\"policyStable\":true") != std::string::npos &&
        independentCurrentMax.find("\"shadowPolicySource\":\"uploaded\"") != std::string::npos &&
        independentCurrentMax.find("\"shadowMode\":\"Current\"") != std::string::npos &&
        independentCurrentMax.find("\"primaryAreaShadowSamplesPerContributingReceiver\":1") != std::string::npos &&
        independentCurrentMax.find("\"primarySkyVisibilitySamples\":1") != std::string::npos,
        "production Current on whole Max reports actual uploaded1/1 rather than legacy4/2");
    const auto independentHigherAuthored = horde::platform::windows::BuildWindowsBenchmarkTuningJson(
        RtWorkloadPreset::Authored, RtWorkloadPreset::Authored, RtWorkloadPreset::Authored, "High", "High", higher, higher, true);
    check(independentHigherAuthored.find("\"shadowMode\":\"Higher\"") != std::string::npos &&
        independentHigherAuthored.find("\"primaryAreaShadowSamplesPerContributingReceiver\":4") != std::string::npos &&
        independentHigherAuthored.find("\"primarySkyVisibilitySamples\":2") != std::string::npos,
        "uploaded Higher shadow budget does not require or invent a Max workload");
    check(horde::platform::windows::BuildWindowsBenchmarkTuningJson(
        RtWorkloadPreset::Authored, RtWorkloadPreset::Authored, RtWorkloadPreset::Authored, "High", "High", current, lower, true)
        .find("\"policyStable\":false") != std::string::npos,
        "changing spatial sample mode invalidates policy stability even when query budgets both equal1");
    const auto notUploaded = horde::platform::windows::BuildWindowsBenchmarkTuningJson(
        RtWorkloadPreset::Max, RtWorkloadPreset::Max, RtWorkloadPreset::Max, "High", "High", std::nullopt, std::nullopt, true);
    check(notUploaded.find("\"policyStable\":false") != std::string::npos &&
        notUploaded.find("\"shadowPolicySource\":\"unavailable\"") != std::string::npos &&
        notUploaded.find("\"primaryAreaShadowSamplesPerContributingReceiver\":0") != std::string::npos,
        "ordinary reporting without an actual upload never fabricates Max shadow policy");
    auto invalidUpload = higher; invalidUpload.controls[3] = 2u;
    check(horde::platform::windows::BuildWindowsBenchmarkTuningJson(
        RtWorkloadPreset::Authored, RtWorkloadPreset::Authored, RtWorkloadPreset::Authored, "High", "High", higher, invalidUpload, true)
        .find("\"shadowPolicySource\":\"unavailable\"") != std::string::npos,
        "unknown quality flags are not admitted as actual physical policy");
    check(horde::platform::windows::BuildWindowsBenchmarkTuningJson(
        RtWorkloadPreset::Authored, RtWorkloadPreset::Authored, RtWorkloadPreset::Authored, "Mobile", "Mobile", higher, higher, true)
        .find("\"policyStable\":false") != std::string::npos,
        "High four-sample record cannot certify a Mobile compiled two-sample policy");
    check(highMax.find("explicit-diagnostic-legacy-workload") != std::string::npos,
        "historical explicit diagnostic caller keeps separately named legacy workload semantics");
    auto mistOff = current; mistOff.controls[3] = 1u;
    const auto offJson = horde::platform::windows::BuildWindowsBenchmarkTuningJson(
        RtWorkloadPreset::Authored, RtWorkloadPreset::Authored, RtWorkloadPreset::Authored, "High", "High", mistOff, mistOff, true);
    check(offJson.find("\"policyStable\":true") != std::string::npos &&
          offJson.find("\"actualUploadedMistEnabled\":false") != std::string::npos &&
          offJson.find("\"shadowMode\":\"Current\"") != std::string::npos,
          "actual MistOff flag is stable independently of unchanged shadow policy");
    check(horde::platform::windows::BuildWindowsBenchmarkTuningJson(
        RtWorkloadPreset::Authored, RtWorkloadPreset::Authored, RtWorkloadPreset::Authored, "High", "High", current, mistOff, true)
        .find("\"policyStable\":false") != std::string::npos, "changing onlyMist cannot certify a stable benchmark policy");
    using horde::platform::windows::ParseWindowsMistCaptureLaunch;
    const std::array<std::wstring_view, 3> offCapture{L"--capture-showcase", L"reports", L"--mist-off"};
    check(ParseWindowsMistCaptureLaunch(offCapture, true).off && ParseWindowsMistCaptureLaunch(offCapture, true).error.empty(),
          "bounded Debug Showcase accepts actual MistOff control");
    check(!ParseWindowsMistCaptureLaunch(offCapture, false).error.empty(), "Release rejects Debug-only Mist capture control before Vulkan");
    check(!ParseWindowsMistCaptureLaunch(std::array<std::wstring_view, 1>{L"--mist-off"}, true).error.empty(), "orphan Mist capture flag rejects");
    check(!ParseWindowsMistCaptureLaunch(std::array<std::wstring_view, 4>{L"--capture-showcase", L"reports", L"--mist-off", L"--mist-off"}, true).error.empty(),
          "duplicate Mist capture flag rejects");
    for (const auto conflicting : {L"--capture-graphics-preview", L"--benchmark-showcase", L"--validate-native-motion", L"--validate-output-resize"})
        check(!ParseWindowsMistCaptureLaunch(std::array<std::wstring_view, 4>{L"--capture-showcase", L"reports", L"--mist-off", conflicting}, true).error.empty(),
              "mixed finite modes never admit a MistOff Showcase capture");
    check(!ParseWindowsMistCaptureLaunch(capture, true).off && ParseWindowsMistCaptureLaunch(capture, true).error.empty(),
          "ordinary historical Showcase capture preserves MistOn without new flag");
    const std::array<std::wstring_view, 4> dustLow{L"--capture-showcase", L"reports", L"--capture-dust", L"low"};
    const std::array<std::wstring_view, 4> dustStandard{L"--capture-showcase", L"reports", L"--capture-dust", L"standard"};
    const std::array<std::wstring_view, 4> dustOff{L"--capture-showcase", L"reports", L"--capture-dust", L"off"};
    check(ParseWindowsMistCaptureLaunch(dustLow, true).dustQuality == horde::graphics::DustQuality::Low &&
          ParseWindowsMistCaptureLaunch(dustStandard, true).dustQuality == horde::graphics::DustQuality::Standard &&
          ParseWindowsMistCaptureLaunch(dustOff, true).dustQuality == horde::graphics::DustQuality::Off,
          "Debug Showcase accepts explicit Off/Low/Standard dust capture choices");
    check(!ParseWindowsMistCaptureLaunch(dustLow, false).error.empty() &&
          !ParseWindowsMistCaptureLaunch(std::array<std::wstring_view, 2>{L"--capture-dust", L"low"}, true).error.empty() &&
          !ParseWindowsMistCaptureLaunch(std::array<std::wstring_view, 4>{L"--capture-showcase", L"reports", L"--capture-dust", L"high"}, true).error.empty(),
          "dust capture rejects Release, orphaned, and unknown values before renderer startup");
    check(ParseWindowsMistCaptureLaunch(std::array<std::wstring_view, 4>{L"--validate-native-motion", L"--motion-scenario", L"skeleton", L"--capture-dust"}, true).error.size() > 0,
          "native-motion dust option still requires an explicit value");
    check(ParseWindowsMistCaptureLaunch(std::array<std::wstring_view, 6>{L"--validate-native-motion", L"--motion-scenario", L"skeleton", L"--capture-dust", L"low", L"reports"}, true).dustQuality == horde::graphics::DustQuality::Low,
          "Debug native-motion capture admits explicit dust choice");
    check(!ParseWindowsMistCaptureLaunch(std::array<std::wstring_view, 6>{L"--capture-showcase", L"reports", L"--capture-dust", L"low", L"--capture-dust", L"off"}, true).error.empty(),
          "duplicate dust capture option rejects");
    return passed ? 0 : 1;
}
