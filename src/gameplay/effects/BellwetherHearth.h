#pragma once

#include <limits>
#include "gameplay/effects/FireEmitterState.h"
#include "../../../assets/models/world/source/bellwether-tavern-v1/staging.generated.h"
#include "../../../assets/models/world/source/bellwether-relic-stall-v1/geometry.generated.h"
#include "../../../assets/models/world/source/bellwether-smith-v1/geometry.generated.h"

#include "../../../assets/models/world/source/bellwether-grate-v1/geometry.generated.h"
#include "../../../assets/models/world/source/bellwether-mill-lamp-v1/geometry.generated.h"
#include "../../../assets/models/world/source/bellwether-square-frontage-v1/geometry.generated.h"
#include "../../../assets/models/world/source/bellwether-garden-house-v1/geometry.generated.h"
#include "../../../assets/models/world/source/bellwether-square-waymarker-v1/geometry.generated.h"
#include "../../../assets/models/world/source/bellwether-house-b-v1/geometry.generated.h"
#include "../../../assets/models/world/source/bellwether-church-station-v1/geometry.generated.h"
#include "../../../assets/models/world/source/bellwether-working-square-v1/geometry.generated.h"
#include "../../../assets/models/world/source/bellwether-footway-lamp-v1/geometry.generated.h"

namespace horde::gameplay::effects
{
inline constexpr std::uint32_t kBellwetherHearthEmitterId=0x42303148u;
inline constexpr std::uint32_t kBellwetherCandleEmitterId=0x42303143u;
inline constexpr std::uint32_t kBellwetherRiverForkLampEmitterId=0x4230314cu;
inline constexpr std::uint32_t kBellwetherRelicStallLampEmitterId=0x42303153u;
inline constexpr std::uint32_t kBellwetherSmithLampEmitterId=0x4230364cu;
inline constexpr std::uint32_t kBellwetherGrateLampEmitterId=0x4230324cu;
inline constexpr std::uint32_t kBellwetherMillBenchLampEmitterId=0x4230354cu;
inline constexpr std::uint32_t kBellwetherPorchLampEmitterId=0x42303150u;
inline constexpr std::uint32_t kGardenHouseWindowEmitterId=0x42303357u;
inline constexpr std::uint32_t kSquareWaymarkerLampEmitterId=0x4253514cu;
inline constexpr std::uint32_t kHouseBLampEmitterId=0x4230344cu;
inline constexpr std::size_t kHouseBLampRosterIndex=10;
inline constexpr std::uint32_t kChurchRecordLampEmitterId=0x4243524cu;
inline constexpr std::size_t kChurchRecordLampRosterIndex=11;
inline constexpr std::uint32_t kWorkingSquareLampEmitterId=0x424d4b4cu;
inline constexpr std::size_t kWorkingSquareLampRosterIndex=12;
inline constexpr std::uint32_t kWorkingSquareStoreLampEmitterId=0x4253544cu;
inline constexpr std::size_t kWorkingSquareStoreLampRosterIndex=13;
inline constexpr std::uint32_t kFootwayLampEmitterId=0x4246574cu;
inline constexpr std::size_t kFootwayLampRosterIndex=14;
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

inline FireEmitterState MakeBellwetherPorchLampFireEmitter()
{
    auto emitter=MakeBellwetherRiverForkLampFireEmitter();
    emitter.stableId=kBellwetherPorchLampEmitterId;
    emitter.seed=0x504f5243u;
    emitter.radius=.065f;
    emitter.height=.17f;
    return emitter;
}

inline FireEmitterState MakeGardenHouseWindowEmitter()
{
    auto emitter=MakeBellwetherCandleFireEmitter();
    emitter.stableId=kGardenHouseWindowEmitterId;
    emitter.seed=0x57494e44u;
    emitter.colourTemperatureKelvin=2080.f;
    return emitter;
}

inline FireEmitterState MakeSquareWaymarkerLampEmitter()
{
    auto emitter=MakeBellwetherPorchLampFireEmitter();
    emitter.stableId=kSquareWaymarkerLampEmitterId;
    emitter.seed=0x5741594du;
    return emitter;
}

inline FireEmitterFixedStepInput GardenHouseWindowFireInput()
{
    return BellwetherFireInput(bellwether::kGardenHouseWindowFlame,bellwether::kGardenHouseWindowLight,1.0f);
}

inline FireEmitterState MakeHouseBLampEmitter()
{
    auto emitter=MakeBellwetherPorchLampFireEmitter();
    emitter.stableId=kHouseBLampEmitterId;
    emitter.seed=0x53544f52u;
    emitter.colourTemperatureKelvin=2110.f;
    emitter.authoredStrengthScale=1.6f;
    return emitter;
}

inline FireEmitterFixedStepInput HouseBLampFireInput()
{
    return BellwetherFireInput(bellwether::kHouseBLampFlame,bellwether::kHouseBLampLight,1.f);
}

inline FireEmitterState MakeChurchRecordLampEmitter()
{
    auto emitter=MakeBellwetherPorchLampFireEmitter();
    emitter.stableId=kChurchRecordLampEmitterId;
    emitter.seed=0x44415942u;
    emitter.colourTemperatureKelvin=2230.f;
    return emitter;
}

inline FireEmitterFixedStepInput ChurchRecordLampFireInput()
{
    return BellwetherFireInput(bellwether::kChurchRecordStationFlame,bellwether::kChurchRecordStationLight,1.45f);
}

inline FireEmitterState MakeWorkingSquareLampEmitter()
{
    auto emitter=MakeBellwetherPorchLampFireEmitter();
    emitter.stableId=kWorkingSquareLampEmitterId;
    emitter.seed=0x4d41524bu;
    emitter.colourTemperatureKelvin=2225.f;
    emitter.height=.21f;
    emitter.authoredStrengthScale=2.2f;
    return emitter;
}

inline FireEmitterFixedStepInput WorkingSquareLampFireInput()
{
    return BellwetherFireInput(bellwether::kWorkingSquareLampFlame,bellwether::kWorkingSquareLampLight,1.f);
}

inline FireEmitterState MakeWorkingSquareStoreLampEmitter()
{
    auto emitter=MakeWorkingSquareLampEmitter();
    emitter.stableId=kWorkingSquareStoreLampEmitterId;
    emitter.seed=0x53545245u;
    emitter.colourTemperatureKelvin=2170.f;
    emitter.height=.17f;
    emitter.authoredStrengthScale=2.4f;
    return emitter;
}

inline FireEmitterFixedStepInput WorkingSquareStoreLampFireInput()
{
    return BellwetherFireInput(bellwether::kWorkingSquareStoreFlame,bellwether::kWorkingSquareStoreLight,1.f);
}

inline FireEmitterState MakeFootwayLampEmitter()
{
    auto emitter=MakeBellwetherPorchLampFireEmitter();
    emitter.stableId=kFootwayLampEmitterId;
    emitter.seed=0x464f4f54u;
    emitter.colourTemperatureKelvin=2150.f;
    emitter.authoredStrengthScale=2.35f;
    return emitter;
}

inline FireEmitterFixedStepInput FootwayLampFireInput()
{
    return BellwetherFireInput(bellwether::kFootwayLampFlame,bellwether::kFootwayLampLight,1.f);
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
        LampZone{bellwether::kSquarePorchLampLight,12.5f,7},
        LampZone{bellwether::kHouseBLampLight,14.f,kHouseBLampRosterIndex},
        LampZone{bellwether::kChurchRecordStationLight,13.f,kChurchRecordLampRosterIndex},
        LampZone{bellwether::kWorkingSquareLampLight,9.f,kWorkingSquareLampRosterIndex},
        LampZone{bellwether::kWorkingSquareStoreLight,11.5f,kWorkingSquareStoreLampRosterIndex},
        LampZone{bellwether::kFootwayLampLight,11.f,kFootwayLampRosterIndex},
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
