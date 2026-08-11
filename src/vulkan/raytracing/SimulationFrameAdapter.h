#pragma once

#include "gameplay/simulation/SimulationSnapshot.h"
#include "vulkan/raytracing/PresentableTinyRtScene.h"

namespace horde::vulkan::raytracing
{

// Preserve the established renderer and shader boundary while making the
// shared simulation the sole gameplay authority on every platform.
RtSceneFrameInputs BuildRtSceneFrameInputs(
    const horde::gameplay::simulation::SimulationSnapshot& simulation,
    float outputExposure);

} // namespace horde::vulkan::raytracing
