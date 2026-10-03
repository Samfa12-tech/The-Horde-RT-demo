#pragma once

#include <string>

#include "telemetry/RtBenchmarkEvidenceRun.h"

namespace horde::telemetry
{

// Report-time projections of the retained benchmark evidence owner. These
// functions do not collect, synthesize or mutate frame identity.
[[nodiscard]] std::string BuildRtBenchmarkEvidenceJson(
    const RtBenchmarkEvidenceRun& run);
[[nodiscard]] std::string BuildRtBenchmarkEvidenceText(
    const RtBenchmarkEvidenceRun& run);

} // namespace horde::telemetry
