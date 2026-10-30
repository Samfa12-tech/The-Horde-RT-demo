#pragma once
#include "gameplay/effects/WaterContactSurfaces.h"
#include <cstdint>

namespace horde::gameplay::effects
{
inline constexpr std::size_t kWaterDropletCapacity=24, kWaterRippleCapacity=4;
struct WaterDroplet { WaterPoint position{},velocity{}; float age=0,lifetime=0; bool active=false; };
struct WaterRipple { WaterPoint position{}; float age=0,strength=0; bool active=false; };
struct WaterContactSnapshot {
    std::array<WaterDroplet,kWaterDropletCapacity> droplets{};
    std::array<WaterRipple,kWaterRippleCapacity> ripples{};
    bool inStream=false,wetGround=false;
    std::uint64_t stepCount=0,entryCount=0,sprayCount=0,droppedEmissions=0;
};
struct WaterContactResult { bool wetStep=false,streamEntry=false; float intensity=0; };
class WaterContact {
public:
    WaterContactResult Step(float dt,float x,float y,float z,bool grounded,bool step,float speed,float width,bool paused=false);
    void Reset() { state_={};sprayClock_=0;serial_=0; }
    const WaterContactSnapshot& Snapshot() const { return state_; }
private:
    void EmitDrops(float x,float y,float z,std::size_t count,float speed);
    void Ripple(float x,float z,float strength);
    WaterContactSnapshot state_{}; float sprayClock_=0;std::uint32_t serial_=0;
};
}
