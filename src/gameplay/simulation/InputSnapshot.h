#pragma once

#include <cstdint>
#include <type_traits>

namespace horde::gameplay::simulation
{

struct SimulationCommandSequences
{
    std::uint64_t attack = 0;
    std::uint64_t parry = 0;
    std::uint64_t dodge = 0;
    std::uint64_t routeReset = 0;
    std::uint64_t retry = 0;
};

// One coherent publication of continuous controls and monotonic edge commands.
// Authoritative poses are reserved for the existing deterministic replay and
// checkpoint paths; ordinary player movement always uses the axes below.
struct InputSnapshot
{
    float moveForward = 0.0f;
    float moveStrafe = 0.0f;
    float yawRadians = 0.0f;
    float pitchRadians = 0.0f;
    float lanternStrength = 1.8f;
    float authoritativePlayerX = 0.0f;
    float authoritativePlayerZ = 0.0f;
    bool paused = false;
    bool damageEnabled = true;
    bool hasAuthoritativePlayerPose = false;
    SimulationCommandSequences commands{};
};

static_assert(std::is_trivially_copyable_v<InputSnapshot>);

} // namespace horde::gameplay::simulation
