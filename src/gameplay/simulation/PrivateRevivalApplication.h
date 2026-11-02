#pragma once
#include "gameplay/simulation/GameSimulation.h"
#ifndef HORDE_RT_PRIVATE_REVIVAL
#define HORDE_RT_PRIVATE_REVIVAL 0
#endif
#ifndef HORDE_RT_FLASK_CANDIDATE
#define HORDE_RT_FLASK_CANDIDATE 0
#endif
#ifndef HORDE_RT_SHIPPING17_CONTENT
#define HORDE_RT_SHIPPING17_CONTENT 0
#endif
#define HORDE_RT_REVIVAL_ADMITTED (HORDE_RT_PRIVATE_REVIVAL || HORDE_RT_SHIPPING17_CONTENT)
#define HORDE_RT_FLASK_ADMITTED (HORDE_RT_FLASK_CANDIDATE || HORDE_RT_SHIPPING17_CONTENT)
namespace horde::gameplay::simulation {
inline GameSimulationConfig PrivateRevivalApplicationConfig() {
    auto config=ProductionGameSimulationConfig();
    config.skeletonRevival1_7=HORDE_RT_REVIVAL_ADMITTED!=0;
    config.naturalPracticeRevival1_7=HORDE_RT_REVIVAL_ADMITTED!=0;
    config.healingFlask1_7=HORDE_RT_FLASK_ADMITTED!=0;
    config.flaskUtilityCandidate=HORDE_RT_FLASK_ADMITTED!=0;
    return config;
}
inline GameSimulation CreateApplicationGameSimulation() {
    return GameSimulation(PrivateRevivalApplicationConfig());
}
inline constexpr const char* kFlaskCandidateSha256="56d97faeb0595a625282c3798bc40b49d7ffedd9598e2d99fd8a186acf15f014";
inline constexpr const char* kPrivateStandSha256="800632b9b1dbe53bd4f6508a6d167e52e940c6bdb4e902af752429e96d7fe259";
}
