#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ostream>

#include "gameplay/simulation/SimulationSnapshot.h"
#include "telemetry/RtPerformanceEvidence.h"

namespace horde::telemetry
{
// Diagnostic only: at most 128 rows per renderer session, emitted for real
// timestamped edges and their later semantic feedback. Queue-present acceptance
// is observable here; physical scan-out and input-device latency are not.
class CombatTimingTrace
{
public:
    static constexpr std::uint32_t kMaximumRows = 128u;

    bool HasReportable(const horde::gameplay::simulation::SimulationSnapshot& state) const
    {
        if (rows_ == kMaximumRows) return false;
        for (std::uint32_t offset = 0u; offset < state.combatInputTiming.traceCount; ++offset)
        {
            const auto& trace = At(state.combatInputTiming, offset);
            const auto kind = static_cast<std::size_t>(trace.kind);
            if (kind < reportedCommands_.size() && trace.edgeSteadyTimeNanoseconds != 0u &&
                trace.actualTick != 0u && (trace.commandSequence > reportedCommands_[kind] ||
                trace.semanticEventSequence > reportedSemanticEvents_[kind])) return true;
        }
        return false;
    }

    std::uint32_t WriteAcceptedPresent(std::ostream& output,
        const horde::gameplay::simulation::SimulationSnapshot& state,
        const RtSubmittedFrameIdentity& submitted, std::uint64_t presentAcceptedNs,
        bool accepted, const RtPlayerDiagnostics* recordedPlayer = nullptr)
    {
        if (!accepted || submitted.submissionSerial == 0u || presentAcceptedNs == 0u ||
            submitted.frame.simulationTick != state.tickIndex || !HasReportable(state)) return 0u;
        const auto previousRows = rows_;
        for (std::uint32_t offset = 0u; offset < state.combatInputTiming.traceCount && rows_ < kMaximumRows; ++offset)
        {
            const auto& trace = At(state.combatInputTiming, offset);
            const auto kind = static_cast<std::size_t>(trace.kind);
            if (kind >= reportedCommands_.size() || trace.edgeSteadyTimeNanoseconds == 0u ||
                trace.actualTick == 0u || (trace.commandSequence <= reportedCommands_[kind] &&
                trace.semanticEventSequence <= reportedSemanticEvents_[kind])) continue;
            output << "{\"type\":\"combat-timing\",\"edgeKind\":" << kind
                << ",\"command\":" << trace.commandSequence << ",\"edgeNativeReceiptNs\":" << trace.edgeSteadyTimeNanoseconds
                << ",\"inputPublication\":" << trace.publicationSequence << ",\"disposition\":" << static_cast<unsigned>(trace.disposition)
                << ",\"targetTick\":" << trace.targetTick << ",\"consumedTick\":" << trace.actualTick
                << ",\"commandCount\":" << trace.commandCount << ",\"consumedCount\":" << trace.consumedCount
                << ",\"semanticEvent\":" << trace.semanticEventSequence << ",\"semanticTick\":" << trace.semanticEventTick
                << ",\"poseTick\":" << state.tickIndex << ",\"playerAction\":" << static_cast<unsigned>(state.playerCombat.action)
                << ",\"actionTime\":" << state.playerCombat.actionTime
                << ",\"parryFeedbackActive\":" << (state.combatPresentation.parrySuccessActive ? "true" : "false")
                << ",\"sharedSwordMatrix\":[";
            for (std::size_t component = 0u; component < state.heldItems[1].worldFromItem.size(); ++component)
            {
                if (component) output << ',';
                output << state.heldItems[1].worldFromItem[component];
            }
            output << "],\"recordedMaxGripErrorMicrometres\":";
            if (recordedPlayer) output << recordedPlayer->maximumSocketErrorMicrometres; else output << "null";
            output << ",\"sceneEpoch\":" << submitted.frame.sceneEpoch << ",\"record\":" << submitted.frame.recordSerial
                << ",\"submission\":" << submitted.submissionSerial << ",\"presentCallAcceptedNs\":" << presentAcceptedNs
                << ",\"displayTimeMeasured\":false}\n";
            reportedCommands_[kind] = std::max(reportedCommands_[kind], trace.commandSequence);
            reportedSemanticEvents_[kind] = std::max(reportedSemanticEvents_[kind], trace.semanticEventSequence);
            ++rows_;
        }
        return rows_ - previousRows;
    }

private:
    static const horde::gameplay::simulation::CombatInputTimingTrace& At(
        const horde::gameplay::simulation::CombatInputTimingSnapshot& timing, std::uint32_t offset)
    {
        constexpr auto capacity = static_cast<std::uint32_t>(horde::gameplay::simulation::kCombatInputTimingTraceCapacity);
        return timing.traces[(timing.nextTraceIndex + capacity - timing.traceCount + offset) % capacity];
    }
    std::array<std::uint64_t, 3u> reportedCommands_{};
    std::array<std::uint64_t, 3u> reportedSemanticEvents_{};
    std::uint32_t rows_ = 0u;
};
} // namespace horde::telemetry
