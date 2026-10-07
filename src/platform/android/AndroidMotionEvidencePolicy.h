#pragma once

#include <cstdint>
#include <string_view>

#include "gameplay/simulation/InputSnapshot.h"

namespace horde::platform::android
{

[[nodiscard]] inline bool AndroidMotionRunIdValid(const std::string_view runId) noexcept
{
    if (runId.empty() || runId.size() > 64u) return false;
    for (const unsigned char character : runId)
    {
        const bool digit = character >= '0' && character <= '9';
        const bool upper = character >= 'A' && character <= 'Z';
        const bool lower = character >= 'a' && character <= 'z';
        if (!digit && !upper && !lower && character != '-') return false;
    }
    return true;
}

struct AndroidMotionEvidenceScope
{
    std::uint64_t surfaceGeneration = 0u;
    std::uint64_t sceneEpoch = 0u;
    std::uint64_t measurementGeneration = 0u;
    std::uint32_t outputWidth = 0u;
    std::uint32_t outputHeight = 0u;

    bool operator==(const AndroidMotionEvidenceScope&) const = default;
};

[[nodiscard]] inline bool AndroidMotionScopeValid(
    const AndroidMotionEvidenceScope& scope) noexcept
{
    return scope.surfaceGeneration != 0u && scope.sceneEpoch != 0u &&
        scope.measurementGeneration != 0u && scope.outputWidth != 0u &&
        scope.outputHeight != 0u;
}

// Attach timestamp metadata to harness-generated combat edges. The caller
// supplies only its previous harness command counters, so merged JNI input is
// never mistaken for an edge emitted by the scenario. Validation happens
// before either object is changed; an accepted edge is appended exactly once.
[[nodiscard]] inline bool StampMotionCombatEdges(
    gameplay::simulation::InputSnapshot& generated,
    const gameplay::simulation::SimulationCommandSequences& before,
    const std::uint64_t now,
    gameplay::simulation::CombatInputEdgeHistory& history) noexcept
{
    using namespace gameplay::simulation;
    if (now == 0u || history.nextIndex >= kCombatInputEdgeHistoryCapacity ||
        history.count > kCombatInputEdgeHistoryCapacity || history.nextOrder == 0u)
        return false;

    const auto validDelta = [](const std::uint64_t current, const std::uint64_t previous)
    {
        return current >= previous && current - previous <= 1u;
    };
    if (!validDelta(generated.commands.attack, before.attack) ||
        !validDelta(generated.commands.parry, before.parry) ||
        !validDelta(generated.commands.dodge, before.dodge))
        return false;

    const std::uint64_t edgeCount = (generated.commands.attack != before.attack ? 1u : 0u) +
        (generated.commands.parry != before.parry ? 1u : 0u) +
        (generated.commands.dodge != before.dodge ? 1u : 0u);
    // Leave the saturated sentinel unused; every admitted record has a unique order.
    if (edgeCount > UINT64_MAX - history.nextOrder) return false;
    if (history.count != 0u)
    {
        const std::uint32_t lastIndex = static_cast<std::uint32_t>(
            (history.nextIndex + kCombatInputEdgeHistoryCapacity - 1u) %
            kCombatInputEdgeHistoryCapacity);
        if (now < history.edges[lastIndex].steadyTimeNanoseconds) return false;
    }

    InputSnapshot nextGenerated = generated;
    CombatInputEdgeHistory nextHistory = history;
    nextGenerated.combatEdgeHistory = nextHistory;
    if (nextGenerated.commands.attack != before.attack)
        RecordCombatInputEdge(nextGenerated, CombatInputEdgeKind::Attack, now);
    if (nextGenerated.commands.parry != before.parry)
        RecordCombatInputEdge(nextGenerated, CombatInputEdgeKind::Parry, now);
    if (nextGenerated.commands.dodge != before.dodge)
        RecordCombatInputEdge(nextGenerated, CombatInputEdgeKind::Dodge, now);

    history = nextGenerated.combatEdgeHistory;
    generated.combatEdgeHistory = history;
    return true;
}

} // namespace horde::platform::android
