#pragma once
#include "gameplay/simulation/GameSimulation.h"
#ifndef HORDE_RT_PRIVATE_REVIVAL
#define HORDE_RT_PRIVATE_REVIVAL 0
#endif
namespace horde::gameplay::simulation {
inline GameSimulationConfig PrivateRevivalApplicationConfig() {
    auto config=ProductionGameSimulationConfig();
    config.skeletonRevival1_7=HORDE_RT_PRIVATE_REVIVAL!=0;
    config.naturalPracticeRevival1_7=HORDE_RT_PRIVATE_REVIVAL!=0;
    return config;
}
inline constexpr const char* kPrivateStandSha256="800632b9b1dbe53bd4f6508a6d167e52e940c6bdb4e902af752429e96d7fe259";
}
