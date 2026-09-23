#include "telemetry/RtEvidencePublication.h"

#include <locale>
#include <sstream>
#include <string_view>

namespace horde::telemetry
{
namespace
{

struct CompletedFrameProjection
{
    std::string_view status;
    std::string_view reason;
    const std::string* json = nullptr;
    const std::string* text = nullptr;
};

void WriteOptionalIdentity(std::ostream& output, const std::uint64_t value)
{
    if (value == 0u)
    {
        output << "null";
    }
    else
    {
        output << value;
    }
}

void WriteOptionalReason(std::ostream& output, const std::string_view reason)
{
    if (reason.empty())
    {
        output << "null";
    }
    else
    {
        output << '"' << reason << '"';
    }
}

void BuildAvailableObserverProjection(
    const RtLifecyclePublishedState& publication,
    const std::string_view diagnosticStatus,
    const std::string_view gpuStatus,
    const CompletedFrameProjection& completed,
    std::string& jsonOutput,
    std::string& textOutput,
    const RtEvidencePublicationSource source = RtEvidencePublicationSource::ActiveObserver)
{
    const bool observerAvailable = source == RtEvidencePublicationSource::ActiveObserver;
    std::ostringstream json;
    json.imbue(std::locale::classic());
    json << "{\"version\":" << kRtEvidencePublicationVersion
         << ",\"observerAvailable\":" << (observerAvailable ? "true" : "false")
         << ",\"sceneEpoch\":";
    WriteOptionalIdentity(json, publication.sceneEpoch);
    json << ",\"measurementGeneration\":";
    WriteOptionalIdentity(json, publication.measurementGeneration);
    json << ",\"running\":" << (publication.running ? "true" : "false")
         << ",\"paused\":" << (publication.paused ? "true" : "false")
         << ",\"presented\":" << (publication.presented ? "true" : "false")
         << ",\"diagnosticStatus\":\"" << diagnosticStatus
         << "\",\"gpuStatus\":\"" << gpuStatus
         << "\",\"completedFrameStatus\":\"" << completed.status
         << "\",\"completedFrameReason\":";
    WriteOptionalReason(json, completed.reason);
    json << ",\"completedFrame\":";
    if (completed.json == nullptr)
    {
        json << "null}\n";
    }
    else
    {
        // Preserve the complete canonical JSON document, including its trailing
        // newline, as the exact bytes of the nested value.
        json << *completed.json << "}\n";
    }
    jsonOutput = json.str();

    std::ostringstream text;
    text.imbue(std::locale::classic());
    text << "RT EVIDENCE PUBLICATION version=" << kRtEvidencePublicationVersion << '\n'
         << (observerAvailable ? "Observer: available\n" :
             source == RtEvidencePublicationSource::StoppedObserver ? "Observer: stopped (terminal publication)\n" :
                 "Observer: unavailable (invalid publication)\n")
         << "Lifecycle: epoch=";
    if (publication.sceneEpoch == 0u)
    {
        text << "N/A";
    }
    else
    {
        text << publication.sceneEpoch;
    }
    text << " generation=";
    if (publication.measurementGeneration == 0u)
    {
        text << "N/A";
    }
    else
    {
        text << publication.measurementGeneration;
    }
    text << " running=" << (publication.running ? "yes" : "no")
         << " paused=" << (publication.paused ? "yes" : "no")
         << " presented=" << (publication.presented ? "yes" : "no") << '\n'
         << "Diagnostic status: " << diagnosticStatus << '\n'
         << "GPU status: " << gpuStatus << '\n'
         << "Completed frame: ";
    if (completed.text == nullptr)
    {
        text << "N/A";
        if (!completed.reason.empty())
        {
            text << " (" << completed.reason << ')';
        }
        text << '\n';
    }
    else
    {
        text << "available\n" << *completed.text;
    }
    textOutput = text.str();
}

void BuildUnavailableObserverProjection(std::string& jsonOutput,
                                        std::string& textOutput)
{
    jsonOutput =
        "{\"version\":1,\"observerAvailable\":false,\"sceneEpoch\":null,"
        "\"measurementGeneration\":null,\"running\":null,\"paused\":null,"
        "\"presented\":null,\"diagnosticStatus\":\"unavailable\","
        "\"gpuStatus\":\"unavailable\",\"completedFrameStatus\":\"unavailable\","
        "\"completedFrameReason\":\"observer-unavailable\","
        "\"completedFrame\":null}\n";
    textOutput =
        "RT EVIDENCE PUBLICATION version=1\n"
        "Observer: unavailable\n"
        "Lifecycle: N/A\n"
        "Diagnostic status: unavailable\n"
        "GPU status: unavailable\n"
        "Completed frame: N/A (observer-unavailable)\n";
}

bool FailPublication(const RtLifecyclePublishedState& publication,
                     const std::string_view reason,
                     std::string& jsonOutput,
                     std::string& textOutput,
                     std::string& validationReason)
{
    validationReason.assign(reason);
    BuildAvailableObserverProjection(
        publication,
        "error",
        "error",
        {"error", "invalid-publication", nullptr, nullptr},
        jsonOutput,
        textOutput, RtEvidencePublicationSource::Unavailable);
    return false;
}

} // namespace

bool SerializeRtEvidencePublication(RtLifecyclePublishedState publication,
                                    const RtEvidencePublicationSource source,
                                    std::string& jsonOutput,
                                    std::string& textOutput,
                                    std::string& validationReason)
{
    validationReason.clear();
    if (source == RtEvidencePublicationSource::Unavailable)
    {
        BuildUnavailableObserverProjection(jsonOutput, textOutput);
        return true;
    }
    if (source != RtEvidencePublicationSource::ActiveObserver &&
        source != RtEvidencePublicationSource::StoppedObserver)
        return FailPublication(publication, "unknown publication source", jsonOutput, textOutput, validationReason);
    if (publication.sceneEpoch == 0u)
    {
        return FailPublication(
            publication,
            "observer publication scene epoch must be positive",
            jsonOutput,
            textOutput,
            validationReason);
    }
    if (publication.measurementGeneration == 0u)
    {
        return FailPublication(
            publication,
            "observer publication measurement generation must be positive",
            jsonOutput,
            textOutput,
            validationReason);
    }
    const char* diagnosticStatus = RtSampleStatusName(publication.diagnosticStatus);
    if (diagnosticStatus == nullptr)
    {
        return FailPublication(
            publication,
            "observer publication has unknown diagnostic status",
            jsonOutput,
            textOutput,
            validationReason);
    }
    const char* gpuStatus = RtSampleStatusName(publication.gpuStatus);
    if (gpuStatus == nullptr)
    {
        return FailPublication(
            publication,
            "observer publication has unknown GPU status",
            jsonOutput,
            textOutput,
            validationReason);
    }
    if (source == RtEvidencePublicationSource::StoppedObserver)
    {
        if (publication.running || publication.paused || publication.presented || publication.hasCompletedEvidence ||
            publication.diagnosticStatus != RtSampleStatus::NotReady || publication.gpuStatus != RtSampleStatus::NotReady)
            return FailPublication(publication, "terminal publication retains live state", jsonOutput, textOutput, validationReason);
        BuildAvailableObserverProjection(publication, diagnosticStatus, gpuStatus,
            {"unavailable", "lifecycle-stopped", nullptr, nullptr}, jsonOutput, textOutput,
            RtEvidencePublicationSource::StoppedObserver);
        return true;
    }
    if (!publication.running)
        return FailPublication(publication, "active observer publication is not running", jsonOutput, textOutput, validationReason);
    if (!publication.hasCompletedEvidence)
    {
        BuildAvailableObserverProjection(
            publication,
            diagnosticStatus,
            gpuStatus,
            {"pending", "no-accepted-completed-frame", nullptr, nullptr},
            jsonOutput,
            textOutput);
        return true;
    }

    RtEvidenceValidationError canonicalError = RtEvidenceValidationError::None;
    if (!ValidateRtPerformanceEvidence(publication.completedEvidence, canonicalError))
    {
        return FailPublication(
            publication,
            "completed frame failed canonical validation",
            jsonOutput,
            textOutput,
            validationReason);
    }

    const RtFrameToken& frame = publication.completedEvidence.identity.submitted.frame;
    if (frame.sceneEpoch != publication.sceneEpoch)
    {
        return FailPublication(
            publication,
            "completed frame scene epoch does not match observer publication",
            jsonOutput,
            textOutput,
            validationReason);
    }
    if (frame.measurementGeneration > publication.measurementGeneration)
    {
        return FailPublication(
            publication,
            "completed frame has a future measurement generation",
            jsonOutput,
            textOutput,
            validationReason);
    }
    if (frame.measurementGeneration < publication.measurementGeneration)
    {
        BuildAvailableObserverProjection(
            publication,
            diagnosticStatus,
            gpuStatus,
            {"pending", "previous-measurement-generation", nullptr, nullptr},
            jsonOutput,
            textOutput);
        return true;
    }

    std::string completedJson;
    if (!SerializeRtPerformanceEvidenceJson(
            publication.completedEvidence, completedJson, canonicalError))
    {
        return FailPublication(
            publication,
            "completed frame failed canonical JSON validation",
            jsonOutput,
            textOutput,
            validationReason);
    }
    std::string completedText;
    if (!SerializeRtPerformanceEvidenceText(
            publication.completedEvidence, completedText, canonicalError))
    {
        return FailPublication(
            publication,
            "completed frame failed canonical text validation",
            jsonOutput,
            textOutput,
            validationReason);
    }
    BuildAvailableObserverProjection(
        publication,
        diagnosticStatus,
        gpuStatus,
        {"available", {}, &completedJson, &completedText},
        jsonOutput,
        textOutput);
    return true;
}

bool SerializeRtEvidencePublication(RtLifecyclePublishedState publication,
                                    const bool observerAvailable,
                                    std::string& jsonOutput,
                                    std::string& textOutput,
                                    std::string& validationReason)
{
    return SerializeRtEvidencePublication(publication,
        observerAvailable ? RtEvidencePublicationSource::ActiveObserver : RtEvidencePublicationSource::Unavailable,
        jsonOutput, textOutput, validationReason);
}

} // namespace horde::telemetry
