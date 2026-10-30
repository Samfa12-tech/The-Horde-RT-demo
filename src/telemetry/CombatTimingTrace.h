#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>
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
        for (std::uint32_t offset = 0; offset < state.combatContactTrace.count; ++offset)
        {
            const auto& sample = ContactAt(state.combatContactTrace, offset);
            if (sample.sequence > reportedContactSequence_) return true;
        }
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
        for (std::uint32_t offset = 0; offset < state.combatContactTrace.count && rows_ < kMaximumRows; ++offset)
        {
            const auto& sample = ContactAt(state.combatContactTrace, offset);
            if (sample.sequence <= reportedContactSequence_) continue;
            output << "{\"type\":\"combat-contact\",\"sample\":" << sample.sequence
                << ",\"generation\":" << state.combatContactTrace.generation
                << ",\"overwrittenSamples\":" << state.combatContactTrace.overwritten
                << ",\"command\":" << sample.commandSequence
                << ",\"consumedTick\":" << sample.consumedTick
                << ",\"attackId\":" << sample.attackId << ",\"cut\":" << static_cast<unsigned>(sample.cut)
                << ",\"contactSampleTick\":" << sample.tick
                << ",\"phase\":" << static_cast<unsigned>(sample.phase)
                << ",\"phaseSeconds\":" << sample.phaseSeconds
                << ",\"target\":" << static_cast<unsigned>(sample.target)
                << ",\"outcome\":" << static_cast<unsigned>(sample.outcome)
                << ",\"semanticEvent\":" << sample.semanticEventSequence
                << ",\"supportWorldY\":" << sample.supportWorldY
                << ",\"dodgeElapsedSeconds\":" << sample.dodgeElapsedSeconds
                << ",\"separationMetres\":";
            if (sample.hasSeparation && std::isfinite(sample.separationMetres))
                output << sample.separationMetres; else output << "null";
            output << ",\"bladeStart\":";
            WritePoint(output, sample.bladeStart, sample.hasBlade);
            output << ",\"bladeEnd\":";
            WritePoint(output, sample.bladeEnd, sample.hasBlade);
            output << ",\"fixedStepSwordMatrix\":";
            if (sample.hasSwordTransform && std::all_of(sample.worldFromSword.begin(),
                sample.worldFromSword.end(), [](float value){ return std::isfinite(value); }))
            {
                output << '[';
                for (std::size_t component = 0; component < sample.worldFromSword.size(); ++component)
                {
                    if (component) output << ',';
                    output << sample.worldFromSword[component];
                }
                output << ']';
            }
            else output << "null";
            output << ",\"targetXZ\":[" << sample.targetX << ',' << sample.targetZ << ']'
                << ",\"poseTick\":" << state.tickIndex
                << ",\"sampleTickWasPresented\":" << (sample.tick == state.tickIndex ? "true" : "false")
                // A hit can replace the target pose with stagger/death on this
                // same tick. Joining the RT frame is not proof that the
                // pre-resolution contact geometry was displayed unchanged.
                << ",\"targetReactionMayReplaceContactPose\":"
                << (sample.outcome == gameplay::simulation::CombatContactOutcome::AcceptedSwordContact ? "true" : "false")
                << ",\"sceneEpoch\":" << submitted.frame.sceneEpoch
                << ",\"record\":" << submitted.frame.recordSerial
                << ",\"submission\":" << submitted.submissionSerial
                << ",\"presentCallAcceptedNs\":" << presentAcceptedNs
                << ",\"displayTimeMeasured\":false}\n";
            reportedContactSequence_ = sample.sequence;
            ++rows_;
        }
        return rows_ - previousRows;
    }

private:
    static void WritePoint(std::ostream& output, const std::array<float,3>& point, bool valid)
    {
        if (!valid || !std::all_of(point.begin(),point.end(),[](float value){return std::isfinite(value);}))
        { output << "null"; return; }
        output << '[' << point[0] << ',' << point[1] << ',' << point[2] << ']';
    }
    static const horde::gameplay::simulation::CombatContactSample& ContactAt(
        const horde::gameplay::simulation::CombatContactTraceSnapshot& trace, std::uint32_t offset)
    {
        return trace.samples[(trace.nextIndex + trace.kCapacity - trace.count + offset) % trace.kCapacity];
    }
    static const horde::gameplay::simulation::CombatInputTimingTrace& At(
        const horde::gameplay::simulation::CombatInputTimingSnapshot& timing, std::uint32_t offset)
    {
        constexpr auto capacity = static_cast<std::uint32_t>(horde::gameplay::simulation::kCombatInputTimingTraceCapacity);
        return timing.traces[(timing.nextTraceIndex + capacity - timing.traceCount + offset) % capacity];
    }
    std::array<std::uint64_t, 3u> reportedCommands_{};
    std::array<std::uint64_t, 3u> reportedSemanticEvents_{};
    std::uint32_t rows_ = 0u;
    std::uint64_t reportedContactSequence_ = 0;
};
} // namespace horde::telemetry
