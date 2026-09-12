#pragma once

#include <cstdint>
#include <string>

#include "telemetry/RtPerformanceEvidence.h"

namespace horde::telemetry
{

inline constexpr std::uint32_t kRtEvidencePublicationVersion = 1u;

enum class RtEvidencePublicationSource : std::uint8_t
{
    Unavailable,
    ActiveObserver,
    StoppedObserver,
};

[[nodiscard]] bool SerializeRtEvidencePublication(
    RtLifecyclePublishedState publication,
    RtEvidencePublicationSource source,
    std::string& jsonOutput,
    std::string& textOutput,
    std::string& validationReason);

[[nodiscard]] bool SerializeRtEvidencePublication(
    RtLifecyclePublishedState publication,
    bool observerAvailable,
    std::string& jsonOutput,
    std::string& textOutput,
    std::string& validationReason);

} // namespace horde::telemetry
