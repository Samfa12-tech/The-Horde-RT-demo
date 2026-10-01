#pragma once

#include <span>
#include "gameplay/simulation/SimulationSnapshot.h"

namespace horde::platform::android
{
// Render/gameplay-owner calls only. Copies before the existing SFX drain.
void PublishMusicSnapshot(const gameplay::simulation::SimulationSnapshot& snapshot,
                          std::span<const gameplay::simulation::GameplayEvent> events,
                          bool externallySuspended);
void ResetMusicSession(bool newEventQueue);
} // namespace horde::platform::android
