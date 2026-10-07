#pragma once

#include <cmath>
#include <cstdint>
#include <ostream>

#include "gameplay/simulation/SimulationSnapshot.h"
#include "telemetry/RtPerformanceEvidence.h"

namespace horde::telemetry
{
// Bounded diagnostic for continuous controls and later idle observations. It
// has its own quota so movement cannot consume the combat-edge trace quota.
// Call only for explicit Debug inspection; queue acceptance is not scan-out.
class InputPresentationTrace
{
public:
    static constexpr std::uint32_t kMaximumRows = 256u;
    static constexpr std::uint64_t kIdleIntervalNs = 250'000'000u;

    bool HasReportable(const horde::gameplay::simulation::SimulationSnapshot& state,
                       const std::uint64_t nowNs) const noexcept
    {
        return rows_ < kMaximumRows && state.inputPublicationSequence != 0u &&
            (rows_ == 0u || state.inputPublicationSequence != lastPublication_ ||
             (nowNs >= lastPresentNs_ && nowNs - lastPresentNs_ >= kIdleIntervalNs));
    }

    bool WriteAcceptedPresent(std::ostream& output,
        const horde::gameplay::simulation::SimulationSnapshot& state,
        const RtSubmittedFrameIdentity& submitted,
        const std::uint64_t presentAcceptedNs, const bool accepted)
    {
        if (!accepted || presentAcceptedNs == 0u || submitted.submissionSerial == 0u ||
            submitted.frame.recordSerial == 0u ||
            submitted.frame.simulationTick != state.tickIndex ||
            !HasReportable(state, presentAcceptedNs) ||
            (rows_ != 0u && (presentAcceptedNs < lastPresentNs_ ||
                submitted.frame.sceneEpoch < lastSceneEpoch_ ||
                (submitted.frame.sceneEpoch == lastSceneEpoch_ &&
                 submitted.submissionSerial <= lastSubmission_)))) return false;
        if (!std::isfinite(state.inputMoveForward) || !std::isfinite(state.inputMoveStrafe) ||
            !std::isfinite(state.playerX) || !std::isfinite(state.playerZ) ||
            !std::isfinite(state.playerYawRadians) || !std::isfinite(state.playerPitchRadians))
            return false; // Optional diagnostics never prevent valid rendering.

        output << "{\"type\":\"input-presentation\",\"inputPublication\":" << state.inputPublicationSequence
            << ",\"poseTick\":" << state.tickIndex
            << ",\"moveForward\":" << state.inputMoveForward
            << ",\"moveStrafe\":" << state.inputMoveStrafe
            << ",\"playerX\":" << state.playerX << ",\"playerZ\":" << state.playerZ
            << ",\"yaw\":" << state.playerYawRadians << ",\"pitch\":" << state.playerPitchRadians
            << ",\"paused\":" << (state.paused ? "true" : "false")
            << ",\"consumedAttack\":" << state.lastConsumedAttackSequence
            << ",\"consumedParry\":" << state.lastConsumedParrySequence
            << ",\"consumedDodge\":" << state.lastConsumedDodgeSequence
            << ",\"playerAction\":" << static_cast<unsigned>(state.playerCombat.action)
            << ",\"dodgeActive\":" << (state.dodgeActive ? "true" : "false")
            << ",\"sceneEpoch\":" << submitted.frame.sceneEpoch
            << ",\"record\":" << submitted.frame.recordSerial
            << ",\"submission\":" << submitted.submissionSerial
            << ",\"presentCallAcceptedNs\":" << presentAcceptedNs
            << ",\"displayTimeMeasured\":false}\n";
        ++rows_;
        lastPublication_ = state.inputPublicationSequence;
        lastPresentNs_ = presentAcceptedNs;
        lastSceneEpoch_ = submitted.frame.sceneEpoch;
        lastSubmission_ = submitted.submissionSerial;
        return true;
    }

private:
    std::uint32_t rows_ = 0u;
    std::uint64_t lastPublication_ = 0u;
    std::uint64_t lastPresentNs_ = 0u;
    std::uint64_t lastSceneEpoch_ = 0u;
    std::uint64_t lastSubmission_ = 0u;
};
} // namespace horde::telemetry
