#pragma once

#include <limits>
#include "gameplay/effects/FireEmitterState.h"
#include "../../../assets/models/world/source/bellwether-tavern-v1/staging.generated.h"
#include "../../../assets/models/world/source/bellwether-relic-stall-v1/geometry.generated.h"
#include "../../../assets/models/world/source/bellwether-smith-v1/geometry.generated.h"

#include "../../../assets/models/world/source/bellwether-grate-v1/geometry.generated.h"
#include "../../../assets/models/world/source/bellwether-mill-lamp-v1/geometry.generated.h"

namespace horde::gameplay::effects
{
inline constexpr std::uint32_t kBellwetherHearthEmitterId=0x42303148u;
inline constexpr std::uint32_t kBellwetherCandleEmitterId=0x42303143u;
inline constexpr std::uint32_t kBellwetherRiverForkLampEmitterId=0x4230314cu;
inline constexpr std::uint32_t kBellwetherRelicStallLampEmitterId=0x42303153u;
inline constexpr std::uint32_t kBellwetherSmithLampEmitterId=0x4230364cu;
inline constexpr std::uint32_t kBellwetherGrateLampEmitterId=0x4230324cu;
inline constexpr std::uint32_t kBellwetherMillBenchLampEmitterId=0x4230354cu;
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

inline FireEmitterState MakeBellwetherCandleFireEmitter()
{
    auto emitter=MakeWorldTorchFireEmitter(kBellwetherCandleEmitterId,0x5749434bu);
    emitter.colourTemperatureKelvin=1950.f;
    emitter.radius=.044f;
    emitter.height=.158f;
    emitter.coreRadius=.022f;
    emitter.smokeDensity=.018f;
    emitter.emberRate=0.f;
    emitter.turbulence=.21f;
    return emitter;
}

inline FireEmitterState MakeBellwetherRiverForkLampFireEmitter()
{
    auto emitter=MakeWorldTorchFireEmitter(kBellwetherRiverForkLampEmitterId,0x4c414d50u);
    emitter.colourTemperatureKelvin=2050.f;
    emitter.radius=.06f;
    emitter.height=.22f;
    emitter.coreRadius=.028f;
    emitter.absorption=5.0f;
    emitter.smokeDensity=.025f;
    emitter.emberRate=0.f;
    emitter.turbulence=.24f;
    return emitter;
}

inline FireEmitterState MakeBellwetherRelicStallLampFireEmitter()
{
    auto emitter=MakeWorldTorchFireEmitter(kBellwetherRelicStallLampEmitterId,0x5354414cu);
    emitter.colourTemperatureKelvin=2050.f;
    emitter.radius=.055f;
    emitter.height=.23f;
    emitter.coreRadius=.027f;
    emitter.absorption=5.0f;
    emitter.smokeDensity=.025f;
    emitter.emberRate=0.f;
    emitter.turbulence=.24f;
    return emitter;
}

inline FireEmitterState MakeBellwetherSmithLampFireEmitter()
{
    auto emitter=MakeBellwetherRelicStallLampFireEmitter();
    emitter.stableId=kBellwetherSmithLampEmitterId;
    emitter.seed=0x534d4954u;
    return emitter;
}

inline FireEmitterState MakeBellwetherGrateLampFireEmitter()
{
    auto emitter=MakeBellwetherCandleFireEmitter();
    emitter.stableId=kBellwetherGrateLampEmitterId;
    emitter.seed=0x47524154u;
    return emitter;
}

inline FireEmitterState MakeBellwetherMillBenchLampFireEmitter()
{
    auto emitter=MakeBellwetherRiverForkLampFireEmitter();
    emitter.stableId=kBellwetherMillBenchLampEmitterId;
    emitter.seed=0x4d494c4cu;
    emitter.colourTemperatureKelvin=2160.f;
    emitter.radius=.043f;
    emitter.height=.121f;
    emitter.coreRadius=.018f;
    return emitter;
}

inline FireEmitterFixedStepInput BellwetherFireInput(
    const std::array<float,3>& flame, const std::array<float,3>& light, float strength)
{
    FireEmitterFixedStepInput input;
    for(std::size_t axis=0;axis<3;++axis) {
        input.worldFromFlame[12+axis]=flame[axis];
        input.worldFromLight[12+axis]=light[axis];
    }
    input.strength=strength;
    input.zone=ShowcaseZone::Outside;
    return input;
}

inline FireEmitterFixedStepInput BellwetherHearthFireInput()
{
    return BellwetherFireInput(kBellwetherHearthFlame,kBellwetherHearthLight,1.f);
}

inline FireEmitterFixedStepInput BellwetherCandleFireInput()
{
    return BellwetherFireInput(bellwether::kTavernCandleFlame,bellwether::kTavernCandleLight,.95f);
}

inline FireEmitterFixedStepInput BellwetherRiverForkLampFireInput()
{
    return BellwetherFireInput(bellwether::kRiverForkLampFlame,bellwether::kRiverForkLampLight,.60f);
}

inline FireEmitterFixedStepInput BellwetherRelicStallLampFireInput()
{
    return BellwetherFireInput(bellwether::kRelicStallLampFlame,bellwether::kRelicStallLampLight,.60f);
}

inline FireEmitterFixedStepInput BellwetherSmithLampFireInput()
{
    return BellwetherFireInput(bellwether::kBellwetherSmithLampFlame,bellwether::kBellwetherSmithLampLight,.78f);
}

inline FireEmitterFixedStepInput BellwetherGrateLampFireInput()
{
    return BellwetherFireInput(bellwether::kBellwetherGrateLampFlame,bellwether::kBellwetherGrateLampLight,.78f);
}

inline FireEmitterFixedStepInput BellwetherMillBenchLampFireInput()
{
    return BellwetherFireInput(bellwether::kMillBenchLampFlame,bellwether::kMillBenchLampLight,.88f);
}

inline constexpr std::size_t SelectBellwetherExteriorLamp(float x, float z) noexcept
{
    struct LampZone {
        std::array<float,3> light;
        float range;
        std::size_t slot;
    };
    constexpr std::array zones{
        LampZone{bellwether::kRelicStallLampLight,15.f,3},
        LampZone{bellwether::kBellwetherSmithLampLight,12.f,4},
        LampZone{bellwether::kBellwetherGrateLampLight,8.f,5},
        LampZone{bellwether::kMillBenchLampLight,6.25f,6},
    };
    float nearest=std::numeric_limits<float>::max();
    std::size_t slot=2; // River fork is the existing exterior default.
    for(const auto& zone:zones) {
        const float dx=x-zone.light[0], dz=z-zone.light[2];
        const float distance=dx*dx+dz*dz;
        if(distance<=zone.range*zone.range && distance<nearest) {
            nearest=distance;
            slot=zone.slot;
        }
    }
    return slot;
}
}
