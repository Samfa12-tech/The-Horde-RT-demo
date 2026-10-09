#pragma once

#include <array>
#include <cstddef>
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
    std::uint64_t interact = 0;
    std::uint64_t toggleHeldLightPose = 0;
    std::uint64_t runToggle = 0;
    std::uint64_t clearRunIntent = 0;
    std::uint64_t tutorialSkip = 0;
    std::uint64_t tutorialReplay = 0;
};

enum class CombatInputEdgeKind : std::uint8_t
{
    Attack,
    Parry,
    Dodge,
};

inline constexpr std::size_t kCombatInputEdgeHistoryCapacity = 32u;

// Timestamp and stick direction travel in the same coherent mailbox snapshot
// as the monotonic command sequence they describe. This is a bounded recent
// history, not an authoritative command queue; old records can be overwritten
// after the consumer has advanced past them.
struct CombatInputEdge
{
    std::uint64_t order = 0u;
    std::uint64_t commandSequence = 0u;
    std::uint64_t steadyTimeNanoseconds = 0u;
    CombatInputEdgeKind kind = CombatInputEdgeKind::Attack;
    float moveForward = 0.0f;
    float moveStrafe = 0.0f;
};

struct CombatInputEdgeHistory
{
    std::array<CombatInputEdge, kCombatInputEdgeHistoryCapacity> edges{};
    std::uint64_t nextOrder = 1u;
    std::uint64_t overwriteCount = 0u;
    std::uint32_t count = 0u;
    std::uint32_t nextIndex = 0u;
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
    float torchLightStrength = 1.8f;
    float authoritativePlayerX = 0.0f;
    float authoritativePlayerZ = 0.0f;
    bool paused = false;
    bool damageEnabled = true;
    bool hasAuthoritativePlayerPose = false;
    // Holds run only while a physical key/controller button remains down.
    // The Android HUD uses the monotonic runToggle edge instead.
    bool runHeld = false;
    bool tutorialEnabled = true;
    bool tutorialSlowdownEnabled = false;
    SimulationCommandSequences commands{};
    CombatInputEdgeHistory combatEdgeHistory{};
};

inline std::uint64_t& CommandSequenceFor(InputSnapshot& input,
                                         CombatInputEdgeKind kind)
{
    switch (kind)
    {
    case CombatInputEdgeKind::Attack: return input.commands.attack;
    case CombatInputEdgeKind::Parry: return input.commands.parry;
    case CombatInputEdgeKind::Dodge: return input.commands.dodge;
    }
    return input.commands.attack;
}

// Call immediately after incrementing the corresponding monotonic command.
// Saturating diagnostics and order values keep malformed/long-lived publishers
// from wrapping into apparently fresh metadata.
inline void RecordCombatInputEdge(InputSnapshot& input,
                                 CombatInputEdgeKind kind,
    std::uint64_t steadyTimeNanoseconds)
{
    CombatInputEdgeHistory& history = input.combatEdgeHistory;
    const std::uint32_t capacity = static_cast<std::uint32_t>(kCombatInputEdgeHistoryCapacity);
    const std::uint32_t index = history.nextIndex % capacity;
    if (history.count >= kCombatInputEdgeHistoryCapacity)
    {
        history.count = capacity;
        if (history.overwriteCount != UINT64_MAX)
            ++history.overwriteCount;
    }
    else
    {
        ++history.count;
    }
    const std::uint64_t order = history.nextOrder;
    if (history.nextOrder != UINT64_MAX)
        ++history.nextOrder;
    history.edges[index] = {order, CommandSequenceFor(input, kind),
                            steadyTimeNanoseconds, kind,
                            input.moveForward, input.moveStrafe};
    history.nextIndex = (index + 1u) % capacity;
}

static_assert(std::is_trivially_copyable_v<InputSnapshot>);

} // namespace horde::gameplay::simulation
