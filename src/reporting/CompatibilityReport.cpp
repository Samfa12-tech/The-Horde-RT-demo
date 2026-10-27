#include "reporting/CompatibilityReport.h"
#include "reporting/PlaytestReport.h"
#include "reporting/BuildIdentity.h"
#include "reporting/ReportIdentity.h"

#include <algorithm>
#include <array>
#include <utility>

namespace horde::reporting
{
namespace
{
using Check = CompatibilityCheck;

bool SafeLabel(const std::string_view value)
{
    // The same public strict UTF-8/privacy policy used by benchmark labels.
    // Its temporary validation report is never exported or submitted.
    const PlaytestReportInput validation{"11111111-1111-4111-8111-111111111111", "2026-10-03T00:00:00Z",
        PlaytestReportCategory::Other, PlaytestReportImpact::Polish, value, true};
    return PreparePlaytestReport(validation).IsReady() &&
        std::none_of(value.begin(), value.end(), [](char c) { return c=='\n' || c=='\r' || c=='\t'; });
}

auto AllChecks(const CompatibilityReportSnapshot& s)
{
    return std::array{s.extensions.accelerationStructure,s.extensions.rayTracingPipeline,
        s.extensions.rayQuery,s.extensions.bufferDeviceAddress,s.extensions.deferredHostOperations,
        s.features.accelerationStructure,s.features.rayTracingPipeline,s.features.rayQuery,s.features.bufferDeviceAddress};
}

auto RequiredChecks(const CompatibilityReportSnapshot& s, bool pipeline)
{
    return std::array{s.extensions.accelerationStructure,
        pipeline?s.extensions.rayTracingPipeline:s.extensions.rayQuery,
        s.extensions.bufferDeviceAddress,s.extensions.deferredHostOperations,
        s.features.accelerationStructure,pipeline?s.features.rayTracingPipeline:s.features.rayQuery,
        s.features.bufferDeviceAddress};
}

bool ValidApi(std::uint32_t api)
{
    return (api>>29u)==0u && ((api>>22u)&0x7fu)>=1u;
}
bool RequiredApi(std::uint32_t api)
{
    const auto major=(api>>22u)&0x7fu, minor=(api>>12u)&0x3ffu;
    return ValidApi(api) && (major>1u || (major==1u && minor>=2u));
}
bool RouteSupported(const CompatibilityReportSnapshot& s, bool pipeline)
{
    const auto required=RequiredChecks(s,pipeline);
    return s.vulkanApiVersion && RequiredApi(*s.vulkanApiVersion) &&
        std::all_of(required.begin(),required.end(),[](Check c){return c==Check::Present;});
}
bool RouteBlocked(const CompatibilityReportSnapshot& s, bool pipeline)
{
    const auto required=RequiredChecks(s,pipeline);
    return (s.vulkanApiVersion && !RequiredApi(*s.vulkanApiVersion)) ||
        std::any_of(required.begin(),required.end(),[](Check c){return c==Check::Absent;});
}

bool ValidEvidence(const CompatibilityReportSnapshot& s)
{
    if (s.platform>CompatibilityPlatform::Android || s.probe>CompatibilityProbe::DeviceChecked ||
        s.backend>CompatibilityBackend::RayQueryCompute || s.lastStage>CompatibilityStage::WindowCreation ||
        s.outcome>CompatibilityOutcome::RuntimeFailure) return false;
    const auto all=AllChecks(s);
    if (std::any_of(all.begin(),all.end(),[](Check c){return c>Check::Present;})) return false;
    if (s.vulkanApiVersion && !ValidApi(*s.vulkanApiVersion)) return false;
    if (s.probe==CompatibilityProbe::DeviceChecked) {
        if (s.gpu.empty() || !s.rawDriverVersion || !s.vulkanApiVersion) return false;
    } else {
        // A missing/failed probe cannot turn default false checks into observed
        // absence, or carry hardware copied from an earlier startup attempt.
        if (!s.gpu.empty() || s.rawDriverVersion || s.vulkanApiVersion ||
            std::any_of(all.begin(),all.end(),[](Check c){return c!=Check::Unknown;}) ||
            s.backend!=CompatibilityBackend::NotSelected) return false;
    }
    if (s.backend!=CompatibilityBackend::NotSelected &&
        !RouteSupported(s,s.backend==CompatibilityBackend::RayTracingPipeline)) return false;
    if (s.rtPresented!=(s.outcome==CompatibilityOutcome::Presented || s.outcome==CompatibilityOutcome::RuntimeFailure)) return false;
    switch (s.outcome)
    {
    case CompatibilityOutcome::NotAttempted:
        return s.probe==CompatibilityProbe::NotAttempted && s.lastStage==CompatibilityStage::None;
    case CompatibilityOutcome::ProbeUnavailable:
        return s.probe==CompatibilityProbe::Unavailable && s.lastStage==CompatibilityStage::CapabilityProbe;
    case CompatibilityOutcome::CapabilityOnly:
        return s.probe==CompatibilityProbe::DeviceChecked &&
            (s.lastStage==CompatibilityStage::CapabilityProbe || s.lastStage==CompatibilityStage::BackendSelection) &&
            !(RouteBlocked(s,true) && RouteBlocked(s,false));
    case CompatibilityOutcome::Unsupported:
        return s.backend==CompatibilityBackend::NotSelected &&
            (s.lastStage==CompatibilityStage::CapabilityProbe || s.lastStage==CompatibilityStage::BackendSelection) &&
            (s.probe==CompatibilityProbe::NoDevice ||
             (s.probe==CompatibilityProbe::DeviceChecked && RouteBlocked(s,true) && RouteBlocked(s,false)));
    case CompatibilityOutcome::StartupFailure:
        // A directly observed failed attempt does not certify probe support.
        // Unknown capability fields remain valid failure evidence.
        return s.lastStage>=CompatibilityStage::AssetPreparation;
    case CompatibilityOutcome::Presented:
        return s.probe==CompatibilityProbe::DeviceChecked && s.lastStage==CompatibilityStage::Presentation &&
            s.backend!=CompatibilityBackend::NotSelected;
    case CompatibilityOutcome::RuntimeFailure:
        return s.probe==CompatibilityProbe::DeviceChecked && s.backend!=CompatibilityBackend::NotSelected &&
            (s.lastStage==CompatibilityStage::RtScene || s.lastStage==CompatibilityStage::Presentation ||
             s.lastStage==CompatibilityStage::RenderLoop);
    }
    return false;
}

std::string_view Name(CompatibilityPlatform v)
{ return v==CompatibilityPlatform::Windows?"Windows":"Android"; }
std::string_view Name(Check v)
{ return v==Check::Unknown?"unknown":v==Check::Absent?"absent":"present"; }
std::string_view Name(CompatibilityProbe v)
{
    constexpr std::array<std::string_view,4> names{"not-attempted","unavailable","no-device","device-checked"};
    return names[static_cast<std::size_t>(v)];
}
std::string_view Name(CompatibilityBackend v)
{
    constexpr std::array<std::string_view,3> names{"not-selected","RayTracingPipeline","RayQueryCompute"};
    return names[static_cast<std::size_t>(v)];
}
std::string_view Name(CompatibilityStage v)
{
    constexpr std::array<std::string_view,13> names{"not-started","capability-probe","asset-preparation",
        "vulkan-instance","surface","backend-selection","logical-device","swapchain","rt-scene","presentation","render-loop","diagnostic-storage","window-creation"};
    return names[static_cast<std::size_t>(v)];
}
std::string_view Name(CompatibilityOutcome v)
{
    constexpr std::array<std::string_view,7> names{"not-attempted","probe-unavailable","capability-only",
        "unsupported","startup-failure","rt-presented","runtime-failure"};
    return names[static_cast<std::size_t>(v)];
}

void Text(std::string& out, std::string_view value)
{
    out+='"';
    for (char c:value) { if (c=='"' || c=='\\') out+='\\'; out+=c; }
    out+='"';
}
void Member(std::string& out, std::string_view key, std::string_view value)
{
    out+=','; Text(out,key); out+=':'; Text(out,value);
}
void OptionalLabel(std::string& out, std::string_view key, std::string_view value)
{
    out+=','; Text(out,key); out+=':';
    if (value.empty()) out+="null"; else Text(out,value);
}
void Number(std::string& out, std::string_view key, std::optional<std::uint32_t> value)
{
    out+=','; Text(out,key); out+=':'; out+=value?std::to_string(*value):"null";
}
} // namespace

PreparedCompatibilityReport PrepareCompatibilityReport(const CompatibilityReportSnapshot& s,
    const CompatibilityReportApproval& approval)
{
    PreparedCompatibilityReport result;
    if (!approval.consentToPrepare) { result.status_=CompatibilityReportStatus::ConsentRequired; return result; }
    if (!IsReportUuidV4(approval.reportUuid)) {
        result.status_=CompatibilityReportStatus::InvalidIdentity; return result;
    }
    // Reuse the established timestamp validator too, without weakening its
    // strict calendar/UTF-8/privacy rules or changing its public API.
    const PlaytestReportInput validation{approval.reportUuid,approval.capturedAtUtc,
        PlaytestReportCategory::Other,PlaytestReportImpact::Polish,"Compatibility report",true};
    if (!PreparePlaytestReport(validation).IsReady()) {
        result.status_=CompatibilityReportStatus::InvalidTimestamp; return result;
    }
    const std::array<const std::string*,6> labels{&s.app,&s.version,&s.build,&s.model,&s.os,&s.gpu};
    for (std::size_t i=0;i<labels.size();++i) {
        const auto& value=*labels[i];
        if (value.size()>kCompatibilityReportMaxLabelBytes) { result.status_=CompatibilityReportStatus::TooLarge; return result; }
        if ((value.empty() && i<3u) || (!value.empty() && !(i==2u && IsBuildIdentityReportLabel(value)) && !SafeLabel(value))) {
            result.status_=CompatibilityReportStatus::InvalidLabel; return result;
        }
    }
    if (!ValidEvidence(s)) return result;

    std::string out="{\"schemaVersion\":2,\"product\":\"horde-lantern-rt\",\"reportKind\":\"compatibility\",\"consentToPrepare\":true,\"report\":{\"schemaVersion\":1,\"reportId\":";
    Text(out,approval.reportUuid); Member(out,"capturedAtUtc",approval.capturedAtUtc);
    Member(out,"app",s.app); Member(out,"version",s.version); Member(out,"build",s.build);
    Member(out,"platform",Name(s.platform)); OptionalLabel(out,"model",s.model);
    OptionalLabel(out,"os",s.os); OptionalLabel(out,"gpu",s.gpu);
    Number(out,"rawDriverVersion",s.rawDriverVersion); Number(out,"vulkanApiVersion",s.vulkanApiVersion);
    Member(out,"probe",Name(s.probe)); Member(out,"lastStage",Name(s.lastStage));
    Member(out,"outcome",Name(s.outcome)); Member(out,"backend",Name(s.backend));
    out+=s.rtPresented?",\"rtPresented\":true":",\"rtPresented\":false";
    out+=",\"extensions\":{\"VK_KHR_acceleration_structure\":"; Text(out,Name(s.extensions.accelerationStructure));
    Member(out,"VK_KHR_ray_tracing_pipeline",Name(s.extensions.rayTracingPipeline));
    Member(out,"VK_KHR_ray_query",Name(s.extensions.rayQuery));
    Member(out,"VK_KHR_buffer_device_address",Name(s.extensions.bufferDeviceAddress));
    Member(out,"VK_KHR_deferred_host_operations",Name(s.extensions.deferredHostOperations));
    out+="},\"features\":{\"accelerationStructure\":"; Text(out,Name(s.features.accelerationStructure));
    Member(out,"rayTracingPipeline",Name(s.features.rayTracingPipeline));
    Member(out,"rayQuery",Name(s.features.rayQuery));
    Member(out,"bufferDeviceAddress",Name(s.features.bufferDeviceAddress));
    out+="}}}";
    if (out.size()>kCompatibilityReportMaxBytes) { result.status_=CompatibilityReportStatus::TooLarge; return result; }
    result.reportId_=approval.reportUuid; result.json_=std::move(out);
    result.status_=CompatibilityReportStatus::Ready;
    return result;
}
} // namespace horde::reporting
