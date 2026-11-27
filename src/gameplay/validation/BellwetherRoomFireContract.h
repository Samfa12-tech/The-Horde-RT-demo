#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

#include "gameplay/effects/BellwetherHearth.h"
#include "gameplay/effects/FireEmitterState.h"
#include "telemetry/RtPerformanceEvidence.h"

namespace horde::gameplay::validation {

// The stationary room position sees the three original flames and the south porch.
inline bool HasBellwetherRoomFireAdmission(const telemetry::RtFireLightingEvidence& evidence) noexcept {
    constexpr std::array expected{effects::kBellwetherHearthEmitterId, effects::kBellwetherCandleEmitterId,
                                  effects::kRewardLanternFireEmitterId, effects::kBellwetherPorchLampEmitterId};
    if (evidence.count != expected.size()) return false;
    return std::all_of(expected.begin(), expected.end(), [&](auto id) {
        return std::any_of(evidence.emitters.begin(), evidence.emitters.begin()+evidence.count,
                           [&](const auto& emitter) { return emitter.stableId == id; });
    });
}

} // namespace horde::gameplay::validation
