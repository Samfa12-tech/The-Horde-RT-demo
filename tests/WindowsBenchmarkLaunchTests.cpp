#include <array>
#include <iostream>
#include <string_view>
#include <vector>

#include "platform/windows/WindowsBenchmarkLaunch.h"
#include "platform/windows/WindowsMistCaptureLaunch.h"
#include "platform/windows/WindowsChapterShippingLaunch.h"
#include "platform/windows/WindowsAssetRoot.h"
#include "platform/windows/WindowsPresentInterval.h"

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
    horde::platform::windows::WindowsPresentIntervals intervals(100);
    check(!intervals.Consume(0) && !intervals.Consume(99) && !intervals.Consume(100),
        "missing and pre-route successful-present stamps fail before interval publication");
    const auto first=intervals.Consume(120);
    check(first && !first->available && first->nanoseconds==0,
        "first successful present has no preceding interval and cannot become an FPS sample");
    check(!intervals.Consume(120) && !intervals.Consume(110),
        "duplicate and reversed successful-present stamps fail closed");
    const auto next=intervals.Consume(150);
    check(next && next->available && next->nanoseconds==30,
        "invalid stamps preserve the previous owner; interval joins actual successive present returns");
    using horde::platform::windows::ResolveWindowsAssetRoot;
    const std::filesystem::path packaged="C:/build/Release/assets",source="C:/source";
    check(ResolveWindowsAssetRoot(packaged,false,source,false,true)==source/"assets",
        "opt-in optimized route resolves existing checkout assets before Vulkan startup");
    check(ResolveWindowsAssetRoot(packaged,true,source,false,true)==packaged &&
        ResolveWindowsAssetRoot(packaged,false,source,false,false)==packaged &&
        ResolveWindowsAssetRoot(packaged,false,{},false,true)==packaged,
        "packaged assets win and ordinary Release never masks a missing package");
    check(ResolveWindowsAssetRoot(packaged,false,source,true,false)==source/"assets",
        "Debug asset fallback remains unchanged");
    using horde::platform::windows::ParseWindowsChapterShippingLaunch;
    const std::array<std::wstring_view,5> shippingArgs{L"--measure-chapter-shipping-route",
        L"--capture-showcase",L"C:\\reports\\shipping-route",L"--development-checkpoint",L"chapter-kit-reunion"};
    check(ParseWindowsChapterShippingLaunch(shippingArgs,true).requested &&
        ParseWindowsChapterShippingLaunch(shippingArgs,true).error.empty(),
        "bounded Shipping route must select exactly the admitted Debug capture");
    check(!ParseWindowsChapterShippingLaunch(shippingArgs,false).error.empty(),
        "Shipping measurement controls cannot enter ordinary Release play");
    check(ParseWindowsChapterShippingLaunch(shippingArgs,false,true).requested &&
        ParseWindowsChapterShippingLaunch(shippingArgs,false,true).error.empty(),
        "separately compiled optimized measurement build admits the exact route without Debug CPU policy");
    check(!ParseWindowsChapterShippingLaunch(ordinary,false,true).requested &&
        ParseWindowsChapterShippingLaunch(ordinary,false,true).error.empty(),
        "optimized measurement support alone cannot acquire inputs or change ordinary launch");
    auto foregroundArgs=std::vector<std::wstring_view>(shippingArgs.begin(),shippingArgs.end());
    foregroundArgs.push_back(L"--measure-chapter-foreground");
    check(ParseWindowsChapterShippingLaunch(foregroundArgs,false,true).foregroundRequested &&
        ParseWindowsChapterShippingLaunch(foregroundArgs,false,true).error.empty(),
        "foreground acquisition requires the explicit bounded measurement option");
    check(!ParseWindowsChapterShippingLaunch(shippingArgs,false,true).foregroundRequested,
        "ordinary measurement cannot acquire foreground implicitly");
    const std::array<std::wstring_view,1> foregroundAlone{L"--measure-chapter-foreground"};
    check(!ParseWindowsChapterShippingLaunch(foregroundAlone,false,true).error.empty(),
        "foreground flag without the closed measurement fails before window creation");
    foregroundArgs.push_back(L"--measure-chapter-foreground");
    check(!ParseWindowsChapterShippingLaunch(foregroundArgs,false,true).error.empty(),
        "duplicate foreground ownership fails closed");
    check(!ParseWindowsChapterShippingLaunch(ordinary,true).requested &&
        ParseWindowsChapterShippingLaunch(ordinary,true).error.empty(),
        "ordinary launches must not gain a measurement owner");
    for(const auto conflict:{L"--validate-chapter-roundtrip",L"--validate-static-zone-cutover",
        L"--validate-static-triangle-cutover",L"--capture-graphics-preview",L"--benchmark-showcase",
        L"--validate-output-resize",L"--validate-native-motion",L"--capture-sky-attribution",L"--mist-off",
        L"--capture-dust",L"--measure-chapter-shipping-route"}) {
        auto args=std::vector<std::wstring_view>(shippingArgs.begin(),shippingArgs.end());args.push_back(conflict);
        check(!ParseWindowsChapterShippingLaunch(args,true).error.empty(),
            "route measurement rejects conflicting validation/tuning and duplicate flags");
        check(!ParseWindowsChapterShippingLaunch(args,false,true).error.empty(),
            "optimized measurement rejects the same conflicting tuning and validation");
    }
    for(const auto badArgs:{
        std::vector<std::wstring_view>{L"--measure-chapter-shipping-route"},
        std::vector<std::wstring_view>{L"--measure-chapter-shipping-route",L"--capture-showcase",L"C:\\reports",L"--development-checkpoint",L"chapter-lookout"},
        std::vector<std::wstring_view>{L"--measure-chapter-shipping-route",L"--capture-showcase",L"C:\\reports",L"--capture-showcase",L"C:\\again",L"--development-checkpoint",L"chapter-kit-reunion"},
        std::vector<std::wstring_view>{L"--measure-chapter-shipping-route",L"--capture-showcase",L"C:\\reports",L"--development-checkpoint",L"chapter-kit-reunion",L"--development-checkpoint",L"chapter-kit-reunion"}})
        check(!ParseWindowsChapterShippingLaunch(badArgs,true).error.empty(),
            "missing, wrong or duplicate route sources fail closed");
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
