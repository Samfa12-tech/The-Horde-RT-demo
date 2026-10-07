#include "platform/windows/WindowsMotionEvidenceLaunch.h"
#include "platform/windows/WindowsMotionEvidenceCapture.h"
#include <array>
#include <iostream>
#include <vector>
int main()
{
    using horde::platform::windows::ParseWindowsMotionEvidenceLaunch;
    using horde::vulkan::raytracing::RtWorkloadPreset;
    int failures=0;
    const auto check=[&](bool ok){ if(!ok) ++failures; };
    for(const auto scenario:{L"torch-low-opening",L"shaft-up",L"keeper-first-entry",L"keeper-retry-reward",L"waterfall-equipment",L"torch-drench"})
    {
        const std::array<std::wstring_view,4> args{L"--validate-native-motion",L"C:\\fresh motion",L"--motion-scenario",scenario};
        const auto parsed=ParseWindowsMotionEvidenceLaunch(args);
        check(parsed.requested && parsed.error.empty() && !parsed.scenario.empty() &&
              parsed.rtWorkloadPreset==RtWorkloadPreset::Authored);
        check(!ParseWindowsMotionEvidenceLaunch(args,false).error.empty()); // Release never admits the mode.
    }
    for(const auto conflict:{L"--capture-showcase",L"--capture-graphics-preview",L"--benchmark-showcase",
        L"--development-checkpoint",L"--debug-rt-lab",L"--rt-lab-hue",L"--validate-output-resize",L"--report-preview",L"--unknown"})
    {
        const std::array<std::wstring_view,5> args{L"--validate-native-motion",L"C:\\fresh",L"--motion-scenario",L"shaft-up",conflict};
        check(!ParseWindowsMotionEvidenceLaunch(args).error.empty());
    }
    for(const auto path:{L"relative",L"C:relative",L"\\root",L"\\\\?\\C:\\device",L"\\\\.\\PhysicalDrive0"})
    {
        const std::array<std::wstring_view,4> args{L"--validate-native-motion",path,L"--motion-scenario",L"shaft-up"};
        check(!ParseWindowsMotionEvidenceLaunch(args).error.empty());
    }
    for(const auto args:{std::vector<std::wstring_view>{L"--motion-scenario",L"shaft-up"},
        {L"--validate-native-motion",L"C:\\fresh"}, {L"--validate-native-motion"},
        {L"--validate-native-motion",L"C:\\fresh",L"--motion-scenario",L"bogus"},
        {L"--validate-native-motion",L"C:\\fresh",L"--motion-scenario",L"torch-drench "},
        {L"--validate-native-motion",L"C:\\fresh",L"--motion-scenario",L"torch-drench/extra"},
        {L"--validate-native-motion",L"C:\\fresh",L"--motion-scenario",L"shaft-up",L"--motion-scenario",L"shaft-up"},
        {L"--validate-native-motion",L"C:\\fresh",L"--motion-scenario",L"shaft-up",L"--validate-native-motion",L"C:\\other"},
        {L"--motion-rt-workload",L"max"},
        {L"--validate-native-motion",L"C:\\fresh",L"--motion-scenario",L"shaft-up",L"--motion-rt-workload"},
        {L"--validate-native-motion",L"C:\\fresh",L"--motion-scenario",L"shaft-up",L"--motion-rt-workload",L"lean"},
        {L"--validate-native-motion",L"C:\\fresh",L"--motion-scenario",L"shaft-up",L"--motion-rt-workload",L"MAX"},
        {L"--validate-native-motion",L"C:\\fresh",L"--motion-scenario",L"shaft-up",L"--motion-rt-workload",L"max",L"--motion-rt-workload",L"authored"}})
        check(!ParseWindowsMotionEvidenceLaunch(args).error.empty());
    std::vector<std::wstring_view> tooMany(17,L"x"); tooMany[0]=L"--validate-native-motion";
    check(!ParseWindowsMotionEvidenceLaunch(tooMany).error.empty());
    check(ParseWindowsMotionEvidenceLaunch(std::vector<std::wstring_view>(17,L"unrelated")).error.empty());
    const std::array<std::wstring_view,5> compute{L"--motion-scenario",L"shaft-up",L"--require-rayquery-compute",L"--validate-native-motion",L"C:\\fresh"};
    check(ParseWindowsMotionEvidenceLaunch(compute).error.empty());
    for(const auto literal:{L"authored",L"max"})
    {
        const std::array<std::wstring_view,7> args{L"--motion-rt-workload",literal,L"--motion-scenario",
            L"torch-low-opening",L"--require-rayquery-compute",L"--validate-native-motion",L"C:\\fresh"};
        const auto parsed=ParseWindowsMotionEvidenceLaunch(args);
        check(parsed.error.empty() && parsed.rtWorkloadPreset==
            (literal==std::wstring_view(L"max") ? RtWorkloadPreset::Max : RtWorkloadPreset::Authored));
        check(!ParseWindowsMotionEvidenceLaunch(args,false).error.empty());
    }
    using horde::platform::windows::WindowsMotionRtPolicyAdmitted;
    check(WindowsMotionRtPolicyAdmitted(RtWorkloadPreset::Authored,RtWorkloadPreset::Authored,"High",true));
    check(WindowsMotionRtPolicyAdmitted(RtWorkloadPreset::Authored,RtWorkloadPreset::Authored,"Mobile",true));
    check(WindowsMotionRtPolicyAdmitted(RtWorkloadPreset::Max,RtWorkloadPreset::Max,"High",true));
    check(!WindowsMotionRtPolicyAdmitted(RtWorkloadPreset::Max,RtWorkloadPreset::Max,"Mobile",true));
    check(!WindowsMotionRtPolicyAdmitted(RtWorkloadPreset::Max,RtWorkloadPreset::Authored,"High",true));
    check(!WindowsMotionRtPolicyAdmitted(RtWorkloadPreset::Max,RtWorkloadPreset::Max,"High",false));
    check(!WindowsMotionRtPolicyAdmitted(RtWorkloadPreset::Authored,RtWorkloadPreset::Authored,"",true));
    check(!WindowsMotionRtPolicyAdmitted(RtWorkloadPreset::Lean,RtWorkloadPreset::Lean,"High",true));
    using horde::platform::windows::WindowsMotionArtifactIdentityValid;
    const std::string hash(64,'a');
    for(const bool computeBackend:{false,true})
    {
        const std::string prefix=computeBackend ? "rayquery_compute_" : "";
        check(WindowsMotionArtifactIdentityValid(prefix+"diagnostic_high_opaque_fast",hash,hash,123,"High",computeBackend,true));
        check(WindowsMotionArtifactIdentityValid(prefix+"shipping_high_generic_dielectric",hash,hash,123,"High",computeBackend,false));
        check(!WindowsMotionArtifactIdentityValid(prefix+"diagnostic_mobile_opaque_fast",hash,hash,123,"High",computeBackend,true));
        check(!WindowsMotionArtifactIdentityValid(prefix+"diagnostic_high_opaque_fast",hash,hash,0,"High",computeBackend,true));
        check(!WindowsMotionArtifactIdentityValid(prefix+"diagnostic_high_opaque_fast","unavailable",hash,123,"High",computeBackend,true));
        check(!WindowsMotionArtifactIdentityValid(prefix+"diagnostic_high_opaque_fast",hash,std::string(64,'Z'),123,"High",computeBackend,true));
        check(!WindowsMotionArtifactIdentityValid(prefix+"diagnostic_high_opaque_fast",hash,hash,123,"High",!computeBackend,true));
    }
    using horde::platform::windows::BuildWindowsMotionTuningJson;
    const auto max=BuildWindowsMotionTuningJson(RtWorkloadPreset::Max,RtWorkloadPreset::Max,RtWorkloadPreset::Max,"High","High");
    check(max.find("\"policyStable\":true")!=std::string::npos &&
          max.find("\"primaryAreaShadowSamplesPerContributingReceiver\":4")!=std::string::npos &&
          max.find("\"primarySkyVisibilitySamples\":2")!=std::string::npos &&
          max.find("\"secondaryAreaShadowSamples\":1")!=std::string::npos &&
          max.find("\"lichMistSamplesPerIntersectingRay\":8")!=std::string::npos &&
          max.find("whole-rt-workload-preset-not-isolated-shadow-cost")!=std::string::npos);
    const auto authored=BuildWindowsMotionTuningJson(RtWorkloadPreset::Authored,RtWorkloadPreset::Authored,RtWorkloadPreset::Authored,"High","High");
    check(authored.find("\"primaryAreaShadowSamplesPerContributingReceiver\":1")!=std::string::npos &&
          authored.find("\"lichMistSamplesPerIntersectingRay\":6")!=std::string::npos);
    check(BuildWindowsMotionTuningJson(RtWorkloadPreset::Max,RtWorkloadPreset::Max,RtWorkloadPreset::Authored,"High","High").find("\"policyStable\":false")!=std::string::npos);
    const horde::vulkan::raytracing::RtQualityControlsGpu current{{1u,1u,1u,0u}};
    const horde::vulkan::raytracing::RtQualityControlsGpu explicitLegacy{{3u,4u,2u,0u}};
    const auto uploadedMax=BuildWindowsMotionTuningJson(RtWorkloadPreset::Max,RtWorkloadPreset::Max,
        RtWorkloadPreset::Max,"High","High",explicitLegacy,explicitLegacy,true);
    check(uploadedMax.find("\"shadowPolicySource\":\"uploaded\"")!=std::string::npos &&
        uploadedMax.find("\"shadowMode\":\"DiagnosticLegacy\"")!=std::string::npos &&
        uploadedMax.find("\"primaryAreaShadowSamplesPerContributingReceiver\":4")!=std::string::npos &&
        uploadedMax.find("\"lichMistSamplesPerIntersectingRay\":8")!=std::string::npos);
    const auto independentMax=BuildWindowsMotionTuningJson(RtWorkloadPreset::Max,RtWorkloadPreset::Max,
        RtWorkloadPreset::Max,"High","High",current,current,true);
    check(independentMax.find("\"shadowMode\":\"Current\"")!=std::string::npos &&
        independentMax.find("\"primaryAreaShadowSamplesPerContributingReceiver\":1")!=std::string::npos &&
        independentMax.find("\"primarySkyVisibilitySamples\":1")!=std::string::npos &&
        independentMax.find("\"lichMistSamplesPerIntersectingRay\":8")!=std::string::npos);
    check(BuildWindowsMotionTuningJson(RtWorkloadPreset::Max,RtWorkloadPreset::Max,RtWorkloadPreset::Max,
        "High","High",std::nullopt,std::nullopt,true).find("\"policyStable\":false")!=std::string::npos);
    using horde::platform::windows::NativeEquipmentCaptureMilestones;
    using horde::platform::windows::NativeMotionNeedsCapture;
    using horde::gameplay::items::HeldItemTransitionKind;
    using horde::gameplay::PlayerCombatAction;
    // CCD's stage-change captures observed Idle at ticks 302 and 352. Actual
    // attack/parry poses followed within the same stage and two-second interval.
    // Every subsequently observed pose must therefore be independently eligible.
    unsigned capturedEquipment = 0u;
    check(!NativeMotionNeedsCapture(false, 0.05, 0u, 0u, 0u, capturedEquipment));
    for (const auto action : {PlayerCombatAction::SwingWindup, PlayerCombatAction::SwingActive,
                             PlayerCombatAction::UpwardSliceWindup, PlayerCombatAction::UpwardSliceActive,
                             PlayerCombatAction::ParryActive})
    {
        const unsigned observed = NativeEquipmentCaptureMilestones(HeldItemTransitionKind::None, false, 1.0f, action);
        check(NativeMotionNeedsCapture(false, 0.05, 0u, 0u, observed, capturedEquipment));
        capturedEquipment |= observed;
        check(!NativeMotionNeedsCapture(false, 0.05, 0u, 0u, observed, capturedEquipment));
    }
    capturedEquipment = 0u;
    for (const float progress : {0.29f, 0.50f, 0.83f})
    {
        const unsigned observed = NativeEquipmentCaptureMilestones(HeldItemTransitionKind::Draw, true, progress,
                                                                   PlayerCombatAction::Idle);
        check(NativeMotionNeedsCapture(false, 0.05, 0u, 0u, observed, capturedEquipment));
        capturedEquipment |= observed;
        check(!NativeMotionNeedsCapture(false, 0.05, 0u, 0u, observed, capturedEquipment));
    }
    check(NativeEquipmentCaptureMilestones(HeldItemTransitionKind::Draw, false, 1.0f, PlayerCombatAction::Idle) == 0u);
    check(NativeEquipmentCaptureMilestones(HeldItemTransitionKind::Sheath, true, 0.75f, PlayerCombatAction::Idle) == 0u);
    check(NativeEquipmentCaptureMilestones(HeldItemTransitionKind::Draw, true, 0.20f, PlayerCombatAction::Idle) == 0u);
    check(NativeMotionNeedsCapture(true, 0.05, 0u, 0u, 0u, 0u));
    check(NativeMotionNeedsCapture(false, 2.0, 0u, 0u, 0u, 0u));
    check(NativeMotionNeedsCapture(false, 0.05, 2u, 0u, 0u, 0u));
    check(!NativeMotionNeedsCapture(false, 0.05, 2u, 2u, 0u, 0u));
    std::cout<<"Native motion launch/capture failures="<<failures<<'\n';
    return failures ? 1 : 0;
}
