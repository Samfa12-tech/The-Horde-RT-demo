#include "reporting/CompatibilityReport.h"
#include "reporting/DeveloperReportSubmission.h"

#include <array>
#include <iostream>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
using namespace horde::reporting;
bool passed=true;
void Check(bool value,const char* message)
{
    if (!value) { passed=false; std::cerr<<"Compatibility report: "<<message<<'\n'; }
}
constexpr std::string_view kId="11111111-1111-4111-8111-111111111111";
constexpr std::string_view kUtc="2026-10-27T12:34:56.789Z";
CompatibilityReportApproval Approval() { return {true,kId,kUtc}; }
CompatibilityReportSnapshot Initial()
{
    CompatibilityReportSnapshot s;
    s.version="1.7-development"; s.build="local-test-build";
    s.platform=CompatibilityPlatform::Android; s.model="Example model"; s.os="Android 16";
    return s;
}
CompatibilityReportSnapshot Device()
{
    auto s=Initial(); s.probe=CompatibilityProbe::DeviceChecked;
    s.gpu="Example GPU"; s.rawDriverVersion=std::numeric_limits<std::uint32_t>::max();
    s.vulkanApiVersion=(1u<<22u)|(3u<<12u);
    s.lastStage=CompatibilityStage::CapabilityProbe; s.outcome=CompatibilityOutcome::CapabilityOnly;
    return s;
}
CompatibilityReportSnapshot Presented(bool pipeline=true)
{
    auto s=Device(); s.extensions.accelerationStructure=CompatibilityCheck::Present;
    s.extensions.bufferDeviceAddress=s.extensions.deferredHostOperations=CompatibilityCheck::Present;
    s.features.accelerationStructure=s.features.bufferDeviceAddress=CompatibilityCheck::Present;
    if (pipeline) {
        s.extensions.rayTracingPipeline=s.features.rayTracingPipeline=CompatibilityCheck::Present;
        s.backend=CompatibilityBackend::RayTracingPipeline;
    } else {
        s.extensions.rayQuery=s.features.rayQuery=CompatibilityCheck::Present;
        s.backend=CompatibilityBackend::RayQueryCompute;
    }
    s.rtPresented=true; s.lastStage=CompatibilityStage::Presentation; s.outcome=CompatibilityOutcome::Presented;
    return s;
}
bool Has(const PreparedCompatibilityReport& report,std::string_view value)
{ return report.Json().find(value)!=std::string_view::npos; }
void CheckRejected(const CompatibilityReportSnapshot& s,const char* message)
{
    const auto result=PrepareCompatibilityReport(s,Approval());
    Check(!result.IsReady() && result.Status()==CompatibilityReportStatus::InvalidEvidence &&
        result.Json().empty() && result.ReportId().empty(),message);
}

void MissingProbeAndUnsupported()
{
    auto s=Initial();
    const auto initial=PrepareCompatibilityReport(s,Approval());
    Check(initial.IsReady() && Has(initial,"\"probe\":\"not-attempted\"") &&
        Has(initial,"\"gpu\":null") && Has(initial,"\"rawDriverVersion\":null") &&
        Has(initial,"\"VK_KHR_acceleration_structure\":\"unknown\"") &&
        Has(initial,"\"rtPresented\":false"),"app/model/OS data survives before any renderer or Vulkan probe exists");
    s.probe=CompatibilityProbe::Unavailable; s.outcome=CompatibilityOutcome::ProbeUnavailable;
    s.lastStage=CompatibilityStage::CapabilityProbe;
    Check(PrepareCompatibilityReport(s,Approval()).IsReady(),"failed probe is reportable with explicitly unknown checks");
    s.outcome=CompatibilityOutcome::Unsupported;
    CheckRejected(s,"unavailable probe cannot turn unknown support into a false unsupported claim");
    s.probe=CompatibilityProbe::NoDevice;
    const auto noDevice=PrepareCompatibilityReport(s,Approval());
    Check(noDevice.IsReady() && Has(noDevice,"\"probe\":\"no-device\"") &&
        Has(noDevice,"\"rayQuery\":\"unknown\""),"observed absence of devices is distinct from unmeasured per-device feature checks");
    s.extensions.accelerationStructure=CompatibilityCheck::Absent;
    CheckRejected(s,"no-device evidence cannot manufacture an observed absent device extension");
    s=Device();
    Check(PrepareCompatibilityReport(s,Approval()).IsReady(),"selected physical properties can be reported while feature checks remain unknown");
    s.outcome=CompatibilityOutcome::Unsupported;
    CheckRejected(s,"all-unknown device checks do not establish unsupported hardware");
    s.extensions.rayTracingPipeline=CompatibilityCheck::Absent;
    CheckRejected(s,"one blocked backend does not silently declare an unchecked alternate backend unsupported");
    s.extensions.rayQuery=CompatibilityCheck::Absent;
    const auto unsupported=PrepareCompatibilityReport(s,Approval());
    Check(unsupported.IsReady() && Has(unsupported,"\"outcome\":\"unsupported\"") &&
        Has(unsupported,"\"rawDriverVersion\":4294967295") &&
        Has(unsupported,"\"accelerationStructure\":\"unknown\""),
        "two observed blocked routes establish unsupported while preserving raw driver numeric data and remaining unknowns");
    s=Device(); s.outcome=CompatibilityOutcome::Unsupported;
    s.vulkanApiVersion=(1u<<22u)|(1u<<12u);
    Check(PrepareCompatibilityReport(s,Approval()).IsReady(),"observed API below the engine's Vulkan1.2 requirement independently blocks both routes");
    s=Initial(); s.outcome=CompatibilityOutcome::StartupFailure; s.lastStage=CompatibilityStage::AssetPreparation;
    Check(PrepareCompatibilityReport(s,Approval()).IsReady(),"early asset failure remains reportable without fake renderer context or zero-sized extents");
    s.lastStage=CompatibilityStage::RtScene;
    Check(PrepareCompatibilityReport(s,Approval()).IsReady(),
        "an observed failed attempt never certifies missing physical-device checks");
    for (auto stage:{CompatibilityStage::VulkanInstance,CompatibilityStage::Surface}) {
        auto unavailable=Initial(); unavailable.probe=CompatibilityProbe::Unavailable;
        unavailable.outcome=CompatibilityOutcome::StartupFailure; unavailable.lastStage=stage;
        const auto report=PrepareCompatibilityReport(unavailable,Approval());
        Check(report.IsReady() && Has(report,"\"probe\":\"unavailable\"") &&
            Has(report,"\"outcome\":\"startup-failure\"") && Has(report,"\"rtPresented\":false") &&
            Has(report,"\"VK_KHR_acceleration_structure\":\"unknown\""),
            "failed probe followed by a real instance or surface failure preserves unknown capability evidence");
        unavailable.backend=CompatibilityBackend::RayTracingPipeline;
        CheckRejected(unavailable,"missing-probe failure cannot manufacture a selected RT backend without checked prerequisites");
    }
}

void PresentationAndStartupEvidence()
{
    for (bool pipeline:{true,false}) {
        const auto s=Presented(pipeline);
        const auto report=PrepareCompatibilityReport(s,Approval());
        Check(report.IsReady() && Has(report,"\"outcome\":\"rt-presented\"") &&
            Has(report,"\"rtPresented\":true") && Has(report,pipeline?"RayTracingPipeline":"RayQueryCompute"),
            "either real backend needs its actual checked prerequisites and presentation outcome");
    }
    for (int fault=0;fault<16;++fault) {
        auto s=Presented();
        if (fault==0)s.rtPresented=false;
        if (fault==1)s.outcome=CompatibilityOutcome::StartupFailure;
        if (fault==2)s.lastStage=CompatibilityStage::RtScene;
        if (fault==3)s.backend=CompatibilityBackend::NotSelected;
        if (fault==4)s.extensions.accelerationStructure=CompatibilityCheck::Unknown;
        if (fault==5)s.extensions.rayTracingPipeline=CompatibilityCheck::Absent;
        if (fault==6)s.extensions.bufferDeviceAddress=CompatibilityCheck::Unknown;
        if (fault==7)s.extensions.deferredHostOperations=CompatibilityCheck::Unknown;
        if (fault==8)s.features.accelerationStructure=CompatibilityCheck::Unknown;
        if (fault==9)s.features.rayTracingPipeline=CompatibilityCheck::Absent;
        if (fault==10)s.features.bufferDeviceAddress=CompatibilityCheck::Unknown;
        if (fault==11)s.gpu.clear();
        if (fault==12)s.rawDriverVersion.reset();
        if (fault==13)s.vulkanApiVersion.reset();
        if (fault==14)s.vulkanApiVersion=(1u<<22u)|(1u<<12u);
        if (fault==15)s.extensions.rayQuery=static_cast<CompatibilityCheck>(255);
        CheckRejected(s,"inconsistent presentation or malformed checked support cannot escape into a prepared report");
    }
    auto query=Presented(false); query.features.rayQuery=CompatibilityCheck::Unknown;
    CheckRejected(query,"hardware ray-query presentation requires checked ray-query feature evidence");
    auto startup=Presented(); startup.outcome=CompatibilityOutcome::StartupFailure; startup.rtPresented=false;
    startup.lastStage=CompatibilityStage::RtScene;
    const auto failed=PrepareCompatibilityReport(startup,Approval());
    Check(failed.IsReady() && Has(failed,"\"lastStage\":\"rt-scene\"") &&
        Has(failed,"\"rtPresented\":false"),"supported device and selected backend do not turn a startup failure into presented RT");
    startup.lastStage=static_cast<CompatibilityStage>(255);
    CheckRejected(startup,"invalid stage enum rejects before indexing a serialized label");
    startup=Presented(); startup.probe=CompatibilityProbe::Unavailable;
    CheckRejected(startup,"failed probe cannot retain a previous attempt's GPU checks or backend");
    startup=Device(); startup.vulkanApiVersion=0u;
    CheckRejected(startup,"zero API is unavailable data rather than a valid observed API version");
}

void ConsentPrivacyBoundsAndOwnership()
{
    const auto snapshot=Initial();
    auto approval=Approval(); approval.consentToPrepare=false;
    const auto noConsent=PrepareCompatibilityReport(snapshot,approval);
    Check(noConsent.Status()==CompatibilityReportStatus::ConsentRequired && noConsent.Json().empty(),
        "default-off local preparation consent produces no bytes and implies no remote/public consent");
    approval=Approval(); approval.reportUuid="device-serial-12345678";
    Check(PrepareCompatibilityReport(snapshot,approval).Status()==CompatibilityReportStatus::InvalidIdentity,
        "a device-looking ID cannot replace the required report-scoped UUIDv4 syntax");
    approval.reportUuid="11111111-1111-1111-8111-111111111111";
    Check(PrepareCompatibilityReport(snapshot,approval).Status()==CompatibilityReportStatus::InvalidIdentity,
        "other UUID versions do not establish the required random report identity form");
    for (std::string_view timestamp:{"2026-02-29T00:00:00Z","2026-10-27T12:34:56+00:00","not-a-date"}) {
        approval=Approval(); approval.capturedAtUtc=timestamp;
        Check(PrepareCompatibilityReport(snapshot,approval).Status()==CompatibilityReportStatus::InvalidTimestamp,
            "existing strict UTC/calendar validation is reused without a compatibility exception");
    }
    for (const std::string& label:std::array<std::string,8>{"player@example.test","C:\\Users\\name\\private.txt",
            "token=private-value","https://example.test/private","GPU\nlog line",std::string("GPU\x01",4),
            std::string("\xc0\xaf",2),"/storage/emulated/0/private"}) {
        auto invalid=snapshot; invalid.model=label;
        const auto result=PrepareCompatibilityReport(invalid,Approval());
        Check(result.Status()==CompatibilityReportStatus::InvalidLabel && result.Json().empty(),
            "existing UTF-8/privacy checks reject addresses paths secrets logs and malformed labels");
    }
    auto huge=snapshot; huge.model=std::string(kCompatibilityReportMaxLabelBytes+1u,'x');
    Check(PrepareCompatibilityReport(huge,Approval()).Status()==CompatibilityReportStatus::TooLarge,
        "oversized input rejects instead of silently truncating an approved label");
    huge=snapshot;
    for (auto* label:{&huge.app,&huge.version,&huge.build,&huge.model,&huge.os})
        *label=std::string(kCompatibilityReportMaxLabelBytes,'"');
    const auto escaped=PrepareCompatibilityReport(huge,Approval());
    Check(escaped.IsReady() && escaped.Json().size()<=kCompatibilityReportMaxBytes && Has(escaped,"\\\"\\\""),
        "maximum admitted labels stay within the8KiB exact-byte bound after JSON escaping");
    auto owned=Presented(); std::string id(kId),utc(kUtc);
    const CompatibilityReportApproval borrowed{true,id,utc};
    const auto prepared=PrepareCompatibilityReport(owned,borrowed);
    const std::string frozen(prepared.Json());
    owned.gpu="Changed later GPU"; owned.extensions.rayTracingPipeline=CompatibilityCheck::Absent;
    id.assign(36,'x'); utc.clear();
    Check(prepared.IsReady() && prepared.ReportId()==kId && prepared.Json()==frozen &&
        !Has(prepared,"Changed later GPU"),"prepared report owns immutable reviewed identity and bytes independently of later input mutations");
    Check(Has(prepared,"\"reportKind\":\"compatibility\"") && Has(prepared,"\"consentToPrepare\":true") &&
        !Has(prepared,"consentToSubmit") && !Has(prepared,"public") && !Has(prepared,"driverInfo") &&
        !Has(prepared,"deviceId") && !Has(prepared,"vendorId") && !Has(prepared,"diagnostics") &&
        !Has(prepared,"fingerprint") && !Has(prepared,"screenshot") && !Has(prepared,"fps"),
        "local compatibility schema excludes remote/public consent raw dumps IDs screenshots and invented performance");
    const auto repeated=PrepareCompatibilityReport(Presented(),Approval());
    Check(repeated.ReportId()==prepared.ReportId() && repeated.Json()==prepared.Json(),
        "same reviewed snapshot and report identity reproduce exact local retry/export bytes");
}
} // namespace

int main()
{
    static_assert(std::is_same_v<decltype(std::declval<PreparedCompatibilityReport>().Json()),std::string_view>);
    static_assert(!std::is_constructible_v<PreparedCompatibilityReport,std::string>);
    MissingProbeAndUnsupported(); PresentationAndStartupEvidence(); ConsentPrivacyBoundsAndOwnership();
    const auto local=PrepareCompatibilityReport(Initial(),Approval());
    const auto privateReport=PreparePrivateDeveloperReport(local,true);
    Check(!PreparePrivateDeveloperReport(local).IsReady() && privateReport.IsReady() &&
        privateReport.LocalJson()==local.Json() && privateReport.Kind()==DeveloperReportKind::Compatibility &&
        privateReport.Json().find("\"publishToBoard\":false")!=std::string_view::npos,
        "private consent is separate and preserves local compatibility bytes without public authority");
    Check(BuildDeveloperReportRequest(privateReport,"").empty() &&
        BuildDeveloperReportRequest(privateReport,"bad\ntoken").empty() &&
        !BuildDeveloperReportRequest(privateReport,"fresh-local-verification").empty() &&
        privateReport.Json().find("turnstileToken")==std::string_view::npos,
        "only an attempt owns a fresh bounded verification token");
    return passed?0:1;
}
