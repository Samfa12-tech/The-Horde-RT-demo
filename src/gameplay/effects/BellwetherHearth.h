#pragma once

#include "gameplay/effects/FireEmitterState.h"

namespace horde::gameplay::effects
{
inline constexpr std::uint32_t kBellwetherHearthEmitterId=0x42303148u;
// The B01 west-wall recess spans x=79.72 to 80.85 and opens east.
// Both anchors sit inside the masonry mouth, over the imported tavern floor.
inline constexpr std::array<float,3> kBellwetherHearthFlame{{80.62f,-8.53f,109.90f}};
inline constexpr std::array<float,3> kBellwetherHearthLight{{80.77f,-8.17f,109.90f}};

inline FireEmitterState MakeBellwetherHearthFireEmitter()
{
    auto emitter=MakeWorldTorchFireEmitter(kBellwetherHearthEmitterId,0x48454152u);
    emitter.colourTemperatureKelvin=2125.f;
    emitter.radius=.42f;
    emitter.height=.84f;
    emitter.coreRadius=.21f;
    emitter.absorption=5.1f;
    emitter.smokeDensity=.10f;
    emitter.emberRate=.12f;
    emitter.turbulence=.47f;
    return emitter;
}

inline FireEmitterFixedStepInput BellwetherHearthFireInput()
{
    FireEmitterFixedStepInput input;
    for(std::size_t axis=0;axis<3;++axis) {
        input.worldFromFlame[12+axis]=kBellwetherHearthFlame[axis];
        input.worldFromLight[12+axis]=kBellwetherHearthLight[axis];
    }
    input.zone=ShowcaseZone::Outside;
    return input;
}
}
