#include "platform/android/AndroidMotionEvidencePolicy.h"

#include <array>
#include <iostream>
#include <string>

namespace
{
using namespace horde::gameplay::simulation;
using namespace horde::platform::android;

bool SameHistory(const CombatInputEdgeHistory& left, const CombatInputEdgeHistory& right)
{
    if (left.nextOrder != right.nextOrder || left.overwriteCount != right.overwriteCount ||
        left.count != right.count || left.nextIndex != right.nextIndex)
        return false;
    for (std::size_t index = 0; index < left.edges.size(); ++index)
    {
        const auto& a = left.edges[index];
        const auto& b = right.edges[index];
        if (a.order != b.order || a.commandSequence != b.commandSequence ||
            a.steadyTimeNanoseconds != b.steadyTimeNanoseconds || a.kind != b.kind ||
            a.moveForward != b.moveForward || a.moveStrafe != b.moveStrafe)
            return false;
    }
    return true;
}

bool SameInput(const InputSnapshot& left, const InputSnapshot& right)
{
    return left.moveForward == right.moveForward && left.moveStrafe == right.moveStrafe &&
        left.yawRadians == right.yawRadians && left.pitchRadians == right.pitchRadians &&
        left.torchLightStrength == right.torchLightStrength &&
        left.authoritativePlayerX == right.authoritativePlayerX &&
        left.authoritativePlayerZ == right.authoritativePlayerZ && left.paused == right.paused &&
        left.damageEnabled == right.damageEnabled &&
        left.hasAuthoritativePlayerPose == right.hasAuthoritativePlayerPose &&
        left.commands.attack == right.commands.attack && left.commands.parry == right.commands.parry &&
        left.commands.dodge == right.commands.dodge &&
        left.commands.routeReset == right.commands.routeReset &&
        left.commands.retry == right.commands.retry && left.commands.interact == right.commands.interact &&
        left.commands.toggleHeldLightPose == right.commands.toggleHeldLightPose &&
        SameHistory(left.combatEdgeHistory, right.combatEdgeHistory);
}
}

int main()
{
    int failures = 0;
    const auto check = [&failures](const bool condition)
    {
        if (!condition) ++failures;
    };

    check(AndroidMotionRunIdValid("a"));
    check(AndroidMotionRunIdValid("run-2026-10-07-A9"));
    check(AndroidMotionRunIdValid(std::string(64u, 'z')));
    check(!AndroidMotionRunIdValid(""));
    check(!AndroidMotionRunIdValid(std::string(65u, 'a')));
    check(!AndroidMotionRunIdValid("../run"));
    check(!AndroidMotionRunIdValid("run/name"));
    check(!AndroidMotionRunIdValid("run name"));
    check(!AndroidMotionRunIdValid("run\\name"));
    check(!AndroidMotionRunIdValid("caf\xC3\xA9"));

    const AndroidMotionEvidenceScope validScope{1u, 2u, 3u, 1440u, 3120u};
    check(AndroidMotionScopeValid(validScope));
    check(validScope == AndroidMotionEvidenceScope{1u, 2u, 3u, 1440u, 3120u});
    check(validScope != AndroidMotionEvidenceScope{1u, 2u, 4u, 1440u, 3120u});
    const AndroidMotionEvidenceScope retryScope{1u, 2u, 4u, 1440u, 3120u};
    check(AndroidMotionRetryScopeValid(validScope, retryScope, true, false));
    check(!AndroidMotionRetryScopeValid(validScope, retryScope, false, false));
    check(!AndroidMotionRetryScopeValid(validScope, retryScope, true, true));
    check(!AndroidMotionRetryScopeValid(validScope, validScope, true, false));
    check(!AndroidMotionRetryScopeValid(validScope, {1u, 3u, 4u, 1440u, 3120u}, true, false));
    check(!AndroidMotionRetryScopeValid(validScope, {2u, 2u, 4u, 1440u, 3120u}, true, false));
    check(!AndroidMotionRetryScopeValid(validScope, {1u, 2u, 5u, 1440u, 3120u}, true, false));
    check(!AndroidMotionRetryScopeValid(validScope, {1u, 2u, 2u, 1440u, 3120u}, true, false));
    check(!AndroidMotionRetryScopeValid(validScope, {1u, 2u, 4u, 720u, 1560u}, true, false));
    check(!AndroidMotionRetryScopeValid({1u, 2u, UINT64_MAX, 1440u, 3120u},
        {1u, 2u, 1u, 1440u, 3120u}, true, false));
    for (std::size_t field = 0; field < 5u; ++field)
    {
        auto invalid = validScope;
        switch (field)
        {
        case 0: invalid.surfaceGeneration = 0u; break;
        case 1: invalid.sceneEpoch = 0u; break;
        case 2: invalid.measurementGeneration = 0u; break;
        case 3: invalid.outputWidth = 0u; break;
        case 4: invalid.outputHeight = 0u; break;
        }
        check(!AndroidMotionScopeValid(invalid));
    }

    InputSnapshot generated{};
    generated.moveForward = 0.5f;
    generated.moveStrafe = -0.25f;
    generated.commands.attack = 1u;
    generated.commands.parry = 1u;
    generated.commands.dodge = 1u;
    SimulationCommandSequences before{};
    CombatInputEdgeHistory history{};
    check(StampMotionCombatEdges(generated, before, 100u, history));
    check(history.count == 3u && history.nextIndex == 3u && history.nextOrder == 4u);
    check(generated.combatEdgeHistory.count == history.count);
    const std::array<CombatInputEdgeKind, 3u> kinds{
        CombatInputEdgeKind::Attack, CombatInputEdgeKind::Parry, CombatInputEdgeKind::Dodge};
    for (std::size_t index = 0; index < kinds.size(); ++index)
    {
        const auto& edge = history.edges[index];
        check(edge.order == index + 1u && edge.commandSequence == 1u &&
            edge.steadyTimeNanoseconds == 100u && edge.kind == kinds[index] &&
            edge.moveForward == generated.moveForward && edge.moveStrafe == generated.moveStrafe);
    }

    before = generated.commands;
    const auto historyAfterEmission = history;
    check(StampMotionCombatEdges(generated, before, 100u, history));
    check(history.count == historyAfterEmission.count && history.nextOrder == historyAfterEmission.nextOrder &&
        SameHistory(history, historyAfterEmission)); // Equal counters do not duplicate timestamps.

    generated.commands.attack = 2u;
    check(StampMotionCombatEdges(generated, before, 120u, history));
    check(history.count == 4u && history.edges[3].kind == CombatInputEdgeKind::Attack &&
        history.edges[3].commandSequence == 2u && history.edges[3].steadyTimeNanoseconds == 120u);
    check(SameHistory(generated.combatEdgeHistory, history)); // Persistent history is copied forward.

    const auto checkRejectedTransaction = [&](InputSnapshot candidate,
        const SimulationCommandSequences& previous, const std::uint64_t now,
        const CombatInputEdgeHistory& startingHistory)
    {
        const InputSnapshot inputBefore = candidate;
        CombatInputEdgeHistory historyCandidate = startingHistory;
        const bool accepted = StampMotionCombatEdges(candidate, previous, now, historyCandidate);
        check(!accepted && SameInput(candidate, inputBefore) && SameHistory(historyCandidate, startingHistory));
    };

    InputSnapshot base{};
    base.commands.attack = 5u;
    SimulationCommandSequences baseBefore{};
    baseBefore.attack = 5u;
    checkRejectedTransaction(base, baseBefore, 0u, history); // Zero clock.
    checkRejectedTransaction(base, baseBefore, 119u, history); // Clock regression from 120.

    InputSnapshot noEdges{};
    SimulationCommandSequences noPreviousEdges{};
    const auto unchangedHistory = history;
    check(StampMotionCombatEdges(noEdges, noPreviousEdges, 130u, history));
    check(history.count == unchangedHistory.count && SameHistory(history, unchangedHistory));

    auto regressed = base;
    regressed.commands.attack = 4u;
    checkRejectedTransaction(regressed, baseBefore, 130u, history);
    auto regressedToZero = base;
    regressedToZero.commands.attack = 0u;
    checkRejectedTransaction(regressedToZero, baseBefore, 130u, history);
    auto jumped = base;
    jumped.commands.parry = 2u;
    checkRejectedTransaction(jumped, baseBefore, 130u, history);
    auto jumpedDodge = base;
    jumpedDodge.commands.dodge = 2u;
    checkRejectedTransaction(jumpedDodge, baseBefore, 130u, history);

    InputSnapshot validNext{};
    {
        auto exhaustedBatch = history;
        exhaustedBatch.nextOrder = UINT64_MAX - 1u;
        InputSnapshot threeEdges{};
        threeEdges.commands.attack = threeEdges.commands.parry = threeEdges.commands.dodge = 1u;
        checkRejectedTransaction(threeEdges, {}, 130u, exhaustedBatch);
    }
    validNext.commands.parry = 1u;
    SimulationCommandSequences zeroBefore{};
    check(StampMotionCombatEdges(validNext, zeroBefore, 130u, history));
    check(history.count == 5u && history.edges[4].kind == CombatInputEdgeKind::Parry &&
        history.edges[4].steadyTimeNanoseconds == 130u &&
        SameHistory(validNext.combatEdgeHistory, history));

    std::cout << "Android motion evidence policy failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
