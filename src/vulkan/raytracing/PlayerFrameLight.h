#pragma once

#include "gameplay/simulation/SimulationSnapshot.h"
#include "vulkan/raytracing/RtSceneAbi.generated.h"

#include <array>

namespace horde::vulkan::raytracing
{

// The exact binding-20 payload consumed by both real primary-ray backends.
inline RtHeldLightGpu BuildPlayerFrameLight(const std::array<float, 4>& physicalLight,
                                            const float supportWorldY,
                                            const float viewHeightOffset = 0.0f)
{
    return {physicalLight,
            {horde::gameplay::simulation::PlayerHeightDelta(supportWorldY)+viewHeightOffset, 0, 0, 0}};
}

} // namespace horde::vulkan::raytracing
