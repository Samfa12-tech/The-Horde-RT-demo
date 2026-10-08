#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string_view>

#include "gameplay/CorridorCollision.h"
#include "gameplay/ShowcaseReplay.h"
#include "gameplay/ShowcaseRoute.h"
#include "gameplay/simulation/GameSimulation.h"

namespace
{

using namespace horde::gameplay;
using namespace horde::gameplay::simulation;

constexpr float kFixedDelta = 1.0f / 60.0f;
constexpr float kPositionTolerance = 0.035f;

bool Near(float a, float b, float tolerance = 0.001f)
{
    return std::abs(a - b) <= tolerance;
}

class Checks
{
public:
    void Require(bool condition, std::string_view message)
    {
        if (!condition)
        {
            ++failures_;
            std::cerr << "Waterfall production route failed: " << message << '\n';
        }
    }

    int Failures() const { return failures_; }

private:
    int failures_ = 0;
};

bool IsAuthoredPair(const SimulationSnapshot& state)
{
    return state.skeletonEnemyCount == 2u && state.activeSkeletonCount == 2u &&
        state.skeletonEnemies[0].id == EntityId::SkeletonA &&
        state.skeletonEnemies[1].id == EntityId::SkeletonB &&
        state.skeletonEnemies[0].health == 1 && state.skeletonEnemies[1].health == 1 &&
        !state.skeletonEnemies[0].dead && !state.skeletonEnemies[1].dead &&
        Near(state.skeletonEnemies[0].x, kWaterfallSkeletonGuardSpawns[0].position.x) &&
        Near(state.skeletonEnemies[0].z, kWaterfallSkeletonGuardSpawns[0].position.z) &&
        Near(state.skeletonEnemies[1].x, kWaterfallSkeletonGuardSpawns[1].position.x) &&
        Near(state.skeletonEnemies[1].z, kWaterfallSkeletonGuardSpawns[1].position.z) &&
        Near(state.skeletonEnemies[0].facingRadians, kWaterfallSkeletonGuardSpawns[0].facingRadians) &&
        Near(state.skeletonEnemies[1].facingRadians, kWaterfallSkeletonGuardSpawns[1].facingRadians);
}

bool PairIsHealthyAndStable(const SimulationSnapshot& state)
{
    return state.skeletonEnemyCount == 2u && state.activeSkeletonCount == 2u &&
        state.skeletonEnemies[0].id == EntityId::SkeletonA &&
        state.skeletonEnemies[1].id == EntityId::SkeletonB &&
        state.skeletonEnemies[0].health == 1 && state.skeletonEnemies[1].health == 1 &&
        !state.skeletonEnemies[0].dead && !state.skeletonEnemies[1].dead;
}

bool PairBehindWetline(const SimulationSnapshot& state)
{
    if (!PairIsHealthyAndStable(state)) return false;
    for (std::size_t i = 0; i < state.skeletonEnemyCount; ++i)
    {
        const auto& enemy = state.skeletonEnemies[i];
        if (enemy.x > -2.5f || !IsWaterfallSkeletonPositionWalkable(enemy.x, enemy.z))
            return false;
    }
    return true;
}

struct SwordRouteEvidence
{
    std::uint64_t lastEventSequence = 0u;
    std::uint64_t warningSequence = 0u;
    std::uint64_t drawSequence = 0u;
    std::uint64_t attachmentSequence = 0u;
    bool sawStowedTransition = false;
    bool sawCompletedHandAttachment = false;
};

bool MoveByOrdinaryAxes(GameSimulation& simulation,
                        RoutePosition target,
                        std::uint64_t& publicationSequence,
                        Checks& checks,
                        std::string_view stage,
                        bool& sawDistinctWalkingPhase,
                        bool& guardsStayedBehindWetline,
                        bool& reentryMovedContinuously,
                        bool damageEnabled = false,
                        SwordRouteEvidence* swordEvidence = nullptr);

void ObserveSwordRoute(GameSimulation& simulation, SwordRouteEvidence& evidence)
{
    for (const GameplayEvent& event : simulation.Events().Events())
    {
        if (event.sequence <= evidence.lastEventSequence)
            continue;
        evidence.lastEventSequence = event.sequence;
        if (event.type == GameplayEventType::SkeletonEncounterWarning &&
            evidence.warningSequence == 0u)
            evidence.warningSequence = event.sequence;
        else if (event.type == GameplayEventType::PlayerSwordDrawStarted &&
                 evidence.drawSequence == 0u)
            evidence.drawSequence = event.sequence;
        else if (event.type == GameplayEventType::PlayerSwordAttachmentChanged &&
                 evidence.attachmentSequence == 0u)
            evidence.attachmentSequence = event.sequence;
    }

    const horde::gameplay::items::HeldItemState& sword = simulation.Snapshot().heldItems[1];
    evidence.sawStowedTransition = evidence.sawStowedTransition ||
        (sword.parentMode == horde::gameplay::items::HeldItemParentMode::BodyStow &&
         sword.transition.active &&
         sword.transition.kind == horde::gameplay::items::HeldItemTransitionKind::Draw);
    evidence.sawCompletedHandAttachment = evidence.sawCompletedHandAttachment ||
        (sword.parentMode == horde::gameplay::items::HeldItemParentMode::HandSocket &&
         !sword.transition.active);
}

std::size_t CountEvents(const GameSimulation& simulation, GameplayEventType type)
{
    const auto events = simulation.Events().Events();
    return static_cast<std::size_t>(std::count_if(
        events.begin(), events.end(),
        [type](const GameplayEvent& event) { return event.type == type; }));
}

void StepToward(GameSimulation& simulation,
                RoutePosition target,
                std::uint64_t& publicationSequence,
                bool damageEnabled)
{
    const SimulationSnapshot& before = simulation.Snapshot();
    const float dx = target.x - before.playerX;
    const float dz = target.z - before.playerZ;
    const float distance = std::hypot(dx, dz);
    InputSnapshot input;
    input.yawRadians = 0.0f;
    input.pitchRadians = 0.0f;
    input.damageEnabled = damageEnabled;
    if (distance > 0.0001f)
    {
        input.moveStrafe = std::clamp(dx / distance, -1.0f, 1.0f);
        input.moveForward = std::clamp(-dz / distance, -1.0f, 1.0f);
    }
    simulation.StepFixed(input, kFixedDelta, ++publicationSequence);
}

bool MoveToWaterfallApproach(GameSimulation& simulation,
                             std::uint64_t& publicationSequence,
                             Checks& checks,
                             bool damageEnabled,
                             SwordRouteEvidence* evidence = nullptr)
{
    bool sawWalking = false;
    bool guardsBehindWetline = true;
    bool continuousReentry = true;
    constexpr std::array<RoutePosition, 6> approach{{
        {0.0f, -1.0f},
        {0.0f, -4.8f},
        {0.0f, -9.4f},
        {4.2f, -9.4f},
        {4.2f, -14.6f},
        {1.0f, -14.6f},
    }};
    for (std::size_t i = 0; i < approach.size(); ++i)
    {
        if (!MoveByOrdinaryAxes(simulation, approach[i], publicationSequence,
                                checks, "normal waterfall approach", sawWalking,
                                guardsBehindWetline, continuousReentry,
                                damageEnabled, evidence))
            return false;
    }
    const auto& approachState = simulation.Snapshot();
    checks.Require(approachState.heldItems[1].parentMode ==
                       horde::gameplay::items::HeldItemParentMode::BodyStow &&
                   !approachState.heldItems[1].transition.active &&
                   std::hypot(approachState.playerX - kWaterfallSkeletonPairCenter.x,
                              approachState.playerZ - kWaterfallSkeletonPairCenter.z) >
                       kWaterfallSwordCueRadius,
                   "normal pre-cue approach remains stowed outside the actual warning radius");
    return true;
}

void TestRetreatDuringAutomaticDraw(const GameSimulationConfig& config,
                                    Checks& checks)
{
    GameSimulation simulation(config);
    std::uint64_t publicationSequence = simulation.Snapshot().inputPublicationSequence;
    if (!MoveToWaterfallApproach(simulation, publicationSequence, checks, false))
        return;

    simulation.ClearEvents();
    SwordRouteEvidence evidence;
    constexpr int kMaximumEntryTicks = 120;
    for (int tick = 0; tick < kMaximumEntryTicks; ++tick)
    {
        StepToward(simulation, kWaterfallSkeletonPairCenter, publicationSequence, false);
        ObserveSwordRoute(simulation, evidence);
        const auto& sword = simulation.Snapshot().heldItems[1];
        if (sword.transition.active &&
            sword.transition.kind == horde::gameplay::items::HeldItemTransitionKind::Draw)
            break;
    }
    const auto& started = simulation.Snapshot();
    checks.Require(started.activeEnemyKind == EnemyKind::Skeleton &&
                   std::hypot(started.playerX - kWaterfallSkeletonPairCenter.x,
                              started.playerZ - kWaterfallSkeletonPairCenter.z) <= kWaterfallSwordCueRadius &&
                   started.heldItems[1].parentMode ==
                       horde::gameplay::items::HeldItemParentMode::BodyStow &&
                   started.heldItems[1].transition.active &&
                   started.automaticSwordDrawBlocksDefense &&
                   evidence.warningSequence != 0u && evidence.drawSequence != 0u &&
                   evidence.warningSequence < evidence.drawSequence,
                   "ordinary movement into the visible early cue starts one ordered forced draw");

    constexpr RoutePosition outsideCue{1.0f, -14.6f};
    constexpr int kMaximumRetreatTicks = 24;
    for (int tick = 0; tick < kMaximumRetreatTicks &&
                        std::hypot(simulation.Snapshot().playerX - kWaterfallSkeletonPairCenter.x,
                                   simulation.Snapshot().playerZ - kWaterfallSkeletonPairCenter.z) <=
                            kWaterfallSwordCueRadius + 0.025f; ++tick)
    {
        StepToward(simulation, outsideCue, publicationSequence, false);
        ObserveSwordRoute(simulation, evidence);
    }
    const auto& retreated = simulation.Snapshot();
    checks.Require(std::hypot(retreated.playerX - kWaterfallSkeletonPairCenter.x,
                              retreated.playerZ - kWaterfallSkeletonPairCenter.z) > kWaterfallSwordCueRadius &&
                   retreated.heldItems[1].parentMode ==
                       horde::gameplay::items::HeldItemParentMode::BodyStow &&
                   retreated.heldItems[1].transition.active &&
                   retreated.heldItems[1].transition.progress < 0.5f,
                   "retreat exits the cue radius while the sword remains in the early stowed draw phase");

    bool sawWalking = false;
    bool guardsBehindWetline = true;
    bool continuousReentry = true;
    MoveByOrdinaryAxes(simulation, kWaterfallSkeletonPairCenter, publicationSequence,
                       checks, "waterfall re-entry during draw", sawWalking,
                       guardsBehindWetline, continuousReentry, false, &evidence);
    const auto& reentered = simulation.Snapshot();
    checks.Require(reentered.heldItems[1].parentMode ==
                       horde::gameplay::items::HeldItemParentMode::HandSocket &&
                   !reentered.heldItems[1].transition.active &&
                   !reentered.automaticSwordDrawBlocksDefense &&
                   CountEvents(simulation, GameplayEventType::PlayerSwordDrawStarted) == 1u &&
                   CountEvents(simulation, GameplayEventType::PlayerSwordAttachmentChanged) == 1u,
                   "re-entry finishes the same draw without restarting or re-stowing the sword");

    MoveByOrdinaryAxes(simulation, outsideCue, publicationSequence,
                       checks, "retreat after completed draw", sawWalking,
                       guardsBehindWetline, continuousReentry, false, &evidence);
    checks.Require(simulation.Snapshot().heldItems[1].parentMode ==
                       horde::gameplay::items::HeldItemParentMode::HandSocket &&
                   !simulation.Snapshot().heldItems[1].transition.active &&
                   CountEvents(simulation, GameplayEventType::PlayerSwordDrawStarted) == 1u,
                   "retreat after completed draw preserves the hand attachment without another draw");
}

void TestPartialDrawResetAndRetry(const GameSimulationConfig& config, Checks& checks)
{
    const auto beginPartialAutomaticDraw = [&checks](GameSimulation& simulation,
                                                     std::uint64_t& publicationSequence)
    {
        if (!MoveToWaterfallApproach(simulation, publicationSequence, checks, false))
            return false;
        for (int tick = 0; tick < 120; ++tick)
        {
            StepToward(simulation, kWaterfallSkeletonPairCenter, publicationSequence, false);
            const auto& sword = simulation.Snapshot().heldItems[1];
            if (sword.transition.active &&
                sword.transition.kind == horde::gameplay::items::HeldItemTransitionKind::Draw)
                return simulation.Snapshot().automaticSwordDrawBlocksDefense;
        }
        return false;
    };

    GameSimulation resetSimulation(config);
    std::uint64_t resetSequence = resetSimulation.Snapshot().inputPublicationSequence;
    const bool resetDrawStarted = beginPartialAutomaticDraw(resetSimulation, resetSequence);
    checks.Require(resetDrawStarted, "normal movement can begin a partial automatic draw before reset");
    resetSimulation.ResetRoute();
    const auto& reset = resetSimulation.Snapshot();
    checks.Require(reset.heldItems[1].parentMode ==
                       horde::gameplay::items::HeldItemParentMode::BodyStow &&
                   !reset.heldItems[1].transition.active &&
                   !reset.automaticSwordDrawBlocksDefense &&
                   Near(reset.heldItems[1].visualStowBlend, 1.0f),
                   "route reset cancels a partial forced draw and restores the complete body-stowed state");

    GameSimulation retrySimulation(config);
    std::uint64_t retrySequence = retrySimulation.Snapshot().inputPublicationSequence;
    const bool retryDrawStarted = beginPartialAutomaticDraw(retrySimulation, retrySequence);
    checks.Require(retryDrawStarted, "normal movement can begin a partial automatic draw before retry");
    retrySimulation.RetryEncounter();
    const auto& retry = retrySimulation.Snapshot();
    checks.Require(retry.heldItems[1].parentMode ==
                       horde::gameplay::items::HeldItemParentMode::BodyStow &&
                   !retry.heldItems[1].transition.active &&
                   !retry.automaticSwordDrawBlocksDefense &&
                   Near(retry.heldItems[1].visualStowBlend, 1.0f),
                   "encounter retry cancels a partial forced draw and restores the complete body-stowed state");
}

void TestManualDrawCueOverlap(const GameSimulationConfig& config, Checks& checks)
{
    GameSimulation simulation(config);
    std::uint64_t publicationSequence = simulation.Snapshot().inputPublicationSequence;
    if (!MoveToWaterfallApproach(simulation, publicationSequence, checks, true))
        return;

    simulation.ClearEvents();
    InputSnapshot manualAttack;
    manualAttack.damageEnabled = true;
    manualAttack.commands.attack = 1u;
    simulation.StepFixed(manualAttack, kFixedDelta, ++publicationSequence);
    checks.Require(simulation.Snapshot().zone != ShowcaseZone::SkylightChamber &&
                   simulation.Snapshot().heldItems[1].transition.active &&
                   !simulation.Snapshot().automaticSwordDrawBlocksDefense &&
                   CountEvents(simulation, GameplayEventType::PlayerSwordDrawStarted) == 1u,
                   "a normal manual attack just before the early cue starts one ordinary draw with damage enabled");

    constexpr int kMaximumEntryTicks = 120;
    for (int tick = 0; tick < kMaximumEntryTicks &&
                        CountEvents(simulation, GameplayEventType::SkeletonEncounterWarning) == 0u; ++tick)
        StepToward(simulation, kWaterfallSkeletonPairCenter, publicationSequence, true);
    checks.Require(simulation.Snapshot().activeEnemyKind == EnemyKind::Skeleton &&
                   std::hypot(simulation.Snapshot().playerX - kWaterfallSkeletonPairCenter.x,
                              simulation.Snapshot().playerZ - kWaterfallSkeletonPairCenter.z) <= kWaterfallSwordCueRadius &&
                   simulation.Snapshot().heldItems[1].transition.active &&
                   simulation.Snapshot().heldItems[1].transition.kind ==
                       horde::gameplay::items::HeldItemTransitionKind::Draw &&
                   CountEvents(simulation, GameplayEventType::SkeletonEncounterWarning) == 1u,
                   "ordinary movement crosses the automatic cue while the manual draw is still active");

    InputSnapshot earlyParry;
    earlyParry.damageEnabled = true;
    earlyParry.commands.parry = 1u;
    simulation.StepFixed(earlyParry, kFixedDelta, ++publicationSequence);
    const bool overlapHadOneDraw =
        CountEvents(simulation, GameplayEventType::PlayerSwordDrawStarted) == 1u;
    const bool overlapHadNoAutomaticLockout =
        !simulation.Snapshot().automaticSwordDrawBlocksDefense;
    const auto isParryAction = [](PlayerCombatAction action)
    {
        return action == PlayerCombatAction::ParryStartup ||
               action == PlayerCombatAction::ParryActive ||
               action == PlayerCombatAction::ParryRecovery;
    };
    bool sawBufferedParry = isParryAction(simulation.Snapshot().playerCombat.action);
    for (int tick = 0; tick < 30; ++tick)
    {
        InputSnapshot idle;
        idle.damageEnabled = true;
        simulation.StepFixed(idle, kFixedDelta, ++publicationSequence);
        sawBufferedParry = sawBufferedParry ||
            isParryAction(simulation.Snapshot().playerCombat.action);
    }
    checks.Require(overlapHadOneDraw && overlapHadNoAutomaticLockout &&
                   !sawBufferedParry && simulation.Snapshot().lastConsumedParrySequence == 1u &&
                   simulation.Snapshot().heldItems[1].parentMode ==
                       horde::gameplay::items::HeldItemParentMode::HandSocket &&
                   !simulation.Snapshot().heldItems[1].transition.active &&
                   CountEvents(simulation, GameplayEventType::PlayerSwordDrawStarted) == 1u &&
                   CountEvents(simulation, GameplayEventType::PlayerParrySucceeded) == 0u,
                   "an active manual draw overlapping the automatic cue is not duplicated, does not grant forced-draw defense, and does not buffer parry");
}

bool MoveByOrdinaryAxes(GameSimulation& simulation,
                        RoutePosition target,
                        std::uint64_t& publicationSequence,
                        Checks& checks,
                        std::string_view stage,
                        bool& sawDistinctWalkingPhase,
                        bool& guardsStayedBehindWetline,
                        bool& reentryMovedContinuously,
                        bool damageEnabled,
                        SwordRouteEvidence* swordEvidence)
{
    constexpr int kMaximumTicks = 1200;
    for (int tick = 0; tick < kMaximumTicks; ++tick)
    {
        const SimulationSnapshot& before = simulation.Snapshot();
        const float dx = target.x - before.playerX;
        const float dz = target.z - before.playerZ;
        const float distance = std::hypot(dx, dz);
        if (distance <= kPositionTolerance) return true;

        InputSnapshot input;
        input.yawRadians = 0.0f;
        input.pitchRadians = 0.0f;
        input.damageEnabled = damageEnabled;
        input.moveStrafe = std::clamp(dx / distance, -1.0f, 1.0f);
        input.moveForward = std::clamp(-dz / distance, -1.0f, 1.0f);
        const RoutePosition oldGuardA{before.skeletonEnemies[0].x, before.skeletonEnemies[0].z};
        const RoutePosition oldGuardB{before.skeletonEnemies[1].x, before.skeletonEnemies[1].z};
        const bool wasInsideArena = IsWaterfallSkeletonArena(before.playerX, before.playerZ);

        simulation.StepFixed(input, kFixedDelta, ++publicationSequence);
        if (swordEvidence != nullptr)
            ObserveSwordRoute(simulation, *swordEvidence);
        const SimulationSnapshot& after = simulation.Snapshot();
        if (IsWaterfallSkeletonRoom(after.playerX, after.playerZ))
        {
            guardsStayedBehindWetline = guardsStayedBehindWetline && PairBehindWetline(after);
            if (after.skeletonEnemies[0].animation == EnemyAnimation::Walking &&
                after.skeletonEnemies[1].animation == EnemyAnimation::Walking &&
                Near(after.skeletonEnemies[1].animationTime - after.skeletonEnemies[0].animationTime,
                     0.65f, 0.06f))
                sawDistinctWalkingPhase = true;

            const bool enteredArena = !wasInsideArena &&
                IsWaterfallSkeletonArena(after.playerX, after.playerZ);
            if (enteredArena)
            {
                const float stepA = std::hypot(after.skeletonEnemies[0].x - oldGuardA.x,
                                               after.skeletonEnemies[0].z - oldGuardA.z);
                const float stepB = std::hypot(after.skeletonEnemies[1].x - oldGuardB.x,
                                               after.skeletonEnemies[1].z - oldGuardB.z);
                reentryMovedContinuously = reentryMovedContinuously &&
                    stepA <= 0.06f && stepB <= 0.06f;
            }
        }
    }

    const auto& state = simulation.Snapshot();
    std::cerr << "  blocked stage=" << stage << " target=(" << target.x << ',' << target.z
              << ") actual=(" << state.playerX << ',' << state.playerZ << ") zone="
              << static_cast<int>(state.zone) << " ticks=" << state.tickIndex << '\n';
    checks.Require(false, "ordinary-axis movement reached its bounded tick limit");
    return false;
}

bool MoveThroughRoute(GameSimulation& simulation,
                      std::uint64_t& publicationSequence,
                      Checks& checks,
                      bool& sawDistinctWalkingPhase,
                      bool& guardsStayedBehindWetline,
                      bool& reentryMovedContinuously,
                      SwordRouteEvidence& swordEvidence)
{
    auto route = kShowcaseReplayPath;
    // The replay's final point is a diagnostic pose beyond the dormant Keeper.
    // Stop at the real playable arrival threshold using ordinary movement.
    route[12] = {kKeeperRetryPosition.x, kKeeperRetryPosition.z, ShowcaseZone::Finale};

    for (std::size_t index = 0; index < route.size(); ++index)
    {
        const auto& waypoint = route[index];
        const RoutePosition target{waypoint.x, waypoint.z};
        const std::string_view stage = index < 12u
            ? "authored route waypoint" : "playable Keeper arrival threshold";
        if (index == 5u)
            simulation.ClearEvents();
        if (!MoveByOrdinaryAxes(simulation, target, publicationSequence,
                                checks, stage, sawDistinctWalkingPhase,
                                guardsStayedBehindWetline, reentryMovedContinuously,
                                false, &swordEvidence))
            return false;
        const auto& state = simulation.Snapshot();
        if (state.zone != waypoint.expectedZone)
            std::cerr << "  waypoint index=" << index << " expected zone="
                      << static_cast<int>(waypoint.expectedZone) << " actual zone="
                      << static_cast<int>(state.zone) << " position=(" << state.playerX
                      << ',' << state.playerZ << ")\n";
        checks.Require(state.zone == waypoint.expectedZone,
                       "waypoint reached its expected shared route zone");

        if (waypoint.expectedZone == ShowcaseZone::SkylightChamber)
        {
            checks.Require(state.activeEnemyKind == EnemyKind::Skeleton &&
                           state.enemyRoster.selectedEnemy == EnemyKind::Skeleton,
                           "waterfall room selects the relocated shared skeleton encounter");

            if (!MoveByOrdinaryAxes(simulation, {-2.90f, -15.20f}, publicationSequence,
                                    checks, "waterfall retreat outside aggro", sawDistinctWalkingPhase,
                                    guardsStayedBehindWetline, reentryMovedContinuously,
                                    false, &swordEvidence))
                return false;
            InputSnapshot idle;
            idle.damageEnabled = false;
            bool retreatRetainedPair = true;
            for (int tick = 0; tick < 12; ++tick)
            {
                const auto& beforeIdle = simulation.Snapshot();
                const std::array<RoutePosition, 2> beforePositions{{
                    {beforeIdle.skeletonEnemies[0].x, beforeIdle.skeletonEnemies[0].z},
                    {beforeIdle.skeletonEnemies[1].x, beforeIdle.skeletonEnemies[1].z}}};
                simulation.StepFixed(idle, kFixedDelta, ++publicationSequence);
                const auto& afterIdle = simulation.Snapshot();
                for (std::size_t guard = 0; guard < 2u; ++guard)
                {
                    const float travelled = std::hypot(
                        afterIdle.skeletonEnemies[guard].x - beforePositions[guard].x,
                        afterIdle.skeletonEnemies[guard].z - beforePositions[guard].z);
                    retreatRetainedPair = retreatRetainedPair && travelled <= 0.06f &&
                        afterIdle.skeletonEnemies[guard].id == beforeIdle.skeletonEnemies[guard].id &&
                        afterIdle.skeletonEnemies[guard].health == beforeIdle.skeletonEnemies[guard].health;
                }
                retreatRetainedPair = retreatRetainedPair && PairBehindWetline(afterIdle);
            }
            checks.Require(retreatRetainedPair,
                           "retreat preserves the healthy leashed pair with bounded movement and no reset teleport");
            if (!MoveByOrdinaryAxes(simulation, kWaterfallSkeletonPairCenter, publicationSequence,
                                    checks, "waterfall re-entry", sawDistinctWalkingPhase,
                                    guardsStayedBehindWetline, reentryMovedContinuously,
                                    false, &swordEvidence))
                return false;
            checks.Require(simulation.Snapshot().activeEnemyKind == EnemyKind::Skeleton &&
                           PairIsHealthyAndStable(simulation.Snapshot()),
                           "re-entry resumes the existing room encounter and stable pair");
        }

        if (waypoint.expectedZone == ShowcaseZone::YellowTorchBay ||
            waypoint.expectedZone == ShowcaseZone::BlueTorchBay ||
            waypoint.expectedZone == ShowcaseZone::RedTorchBay ||
            waypoint.expectedZone == ShowcaseZone::GreenTorchBay)
            checks.Require(state.playerAlive,
                           "ordinary movement crosses each of the four distinct torch bays");
    }
    return true;
}

} // namespace

int main()
{
    Checks checks;
    const GameSimulationConfig config = ProductionGameSimulationConfig();
    checks.Require(config.waterfallSkeletonEncounter,
                   "production config enables the accepted waterfall guard placement");
    checks.Require(config.swordStartsStowed,
                   "production starts with the sword stowed while retaining the waterfall guard placement");

    GameSimulation simulation(config);
    std::uint64_t publicationSequence = simulation.Snapshot().inputPublicationSequence;
    const auto verifyResetPair = [&checks, &config](const SimulationSnapshot& state, std::string_view phase, bool coldStart = false)
    {
        if (!IsAuthoredPair(state))
        {
            std::cerr << "  pair mismatch phase=" << phase << " count=" << state.skeletonEnemyCount
                      << " A=(" << state.skeletonEnemies[0].x << ',' << state.skeletonEnemies[0].z
                      << ") B=(" << state.skeletonEnemies[1].x << ',' << state.skeletonEnemies[1].z
                      << ")\n";
            checks.Require(false,
                "production start/reset restores stable IDs, health, lateral positions, facing and authored phase");
        }
        // Construction publishes the authored initial walking sample; a reset
        // finalizes the pair at zero delta outside the arena, publishing Idle0.
        // The live approach separately verifies the authored gait offset resumes.
        const auto animation = coldStart ? EnemyAnimation::Walking : EnemyAnimation::Idle;
        checks.Require(state.skeletonEnemies[0].animation == animation &&
                       state.skeletonEnemies[1].animation == animation &&
                       Near(state.skeletonEnemies[0].animationTime,
                            coldStart ? kWaterfallSkeletonGuardSpawns[0].walkingAnimationPhaseSeconds : 0.0f) &&
                       Near(state.skeletonEnemies[1].animationTime,
                            coldStart ? kWaterfallSkeletonGuardSpawns[1].walkingAnimationPhaseSeconds : 0.0f),
                       "cold start and zero-delta reset preserve their actual phase conventions");
        checks.Require(state.heldItems[1].parentMode ==
                           (config.swordStartsStowed
                                ? horde::gameplay::items::HeldItemParentMode::BodyStow
                                : horde::gameplay::items::HeldItemParentMode::HandSocket) &&
                       !state.heldItems[1].transition.active &&
                       !state.automaticSwordDrawBlocksDefense &&
                       Near(state.heldItems[1].visualStowBlend,
                            config.swordStartsStowed ? 1.0f : 0.0f),
                       "production start and reset apply the configured complete sword attachment state");
    };

    verifyResetPair(simulation.Snapshot(), "cold start", true);
    checks.Require(simulation.Snapshot().zone == ShowcaseZone::Opening &&
                   simulation.Snapshot().skeletonEnemies[0].x < -2.5f &&
                   simulation.Snapshot().skeletonEnemies[1].x < -2.5f,
                   "cold start has no skeleton in the opening room");

    // Ordinary first-room ticks must leave the remote pair untouched and harmless.
    InputSnapshot idle;
    idle.yawRadians = 0.0f;
    bool firstRoomPairStayedAtSpawn = true;
    for (int tick = 0; tick < 120; ++tick)
    {
        simulation.StepFixed(idle, kFixedDelta, ++publicationSequence);
        const auto& state = simulation.Snapshot();
        firstRoomPairStayedAtSpawn = firstRoomPairStayedAtSpawn && PairBehindWetline(state) &&
            Near(state.skeletonEnemies[0].x, kWaterfallSkeletonGuardSpawns[0].position.x) &&
            Near(state.skeletonEnemies[0].z, kWaterfallSkeletonGuardSpawns[0].position.z) &&
            Near(state.skeletonEnemies[1].x, kWaterfallSkeletonGuardSpawns[1].position.x) &&
            Near(state.skeletonEnemies[1].z, kWaterfallSkeletonGuardSpawns[1].position.z) &&
            state.skeletonEnemies[0].action == EnemyCombatAction::Locomotion &&
            state.skeletonEnemies[1].action == EnemyCombatAction::Locomotion &&
            !state.skeletonEnemies[0].playerHitPulse && !state.skeletonEnemies[1].playerHitPulse &&
            state.playerVitals.vitality == 3;
    }
    bool firstRoomHadNoAttackOrDamageEvents = true;
    for (const auto& event : simulation.Events().Events())
        firstRoomHadNoAttackOrDamageEvents = firstRoomHadNoAttackOrDamageEvents &&
            event.type != GameplayEventType::EnemyAttackStarted &&
            event.type != GameplayEventType::PlayerDamaged &&
            event.type != GameplayEventType::SkeletonEncounterWarning &&
            event.type != GameplayEventType::PlayerSwordDrawStarted;
    checks.Require(firstRoomPairStayedAtSpawn && firstRoomHadNoAttackOrDamageEvents,
                   "normal-damage first-room ticks do not relocate, attack or damage from the waterfall pair");

    simulation.ResetRoute();
    verifyResetPair(simulation.Snapshot(), "route reset");
    checks.Require(simulation.Snapshot().zone == ShowcaseZone::Opening,
                   "route reset returns the player to the actual opening");

    bool sawDistinctWalkingPhase = false;
    bool guardsStayedBehindWetline = true;
    bool reentryMovedContinuously = true;
    SwordRouteEvidence swordEvidence;
    MoveThroughRoute(simulation, publicationSequence, checks, sawDistinctWalkingPhase,
                     guardsStayedBehindWetline, reentryMovedContinuously,
                     swordEvidence);
    checks.Require(guardsStayedBehindWetline,
                   "guards remain behind the wetline throughout ordinary route movement");
    checks.Require(reentryMovedContinuously,
                   "arena re-entry advances guards from retained positions without teleport");
    checks.Require(sawDistinctWalkingPhase,
                   "approach observes both live guards walking with their authored distinct gait phase");
    checks.Require(swordEvidence.sawStowedTransition &&
                   swordEvidence.warningSequence != 0u &&
                   swordEvidence.drawSequence != 0u &&
                   swordEvidence.attachmentSequence != 0u &&
                   swordEvidence.warningSequence < swordEvidence.drawSequence &&
                   swordEvidence.drawSequence < swordEvidence.attachmentSequence &&
                   swordEvidence.sawCompletedHandAttachment &&
                   simulation.Snapshot().heldItems[1].parentMode ==
                       horde::gameplay::items::HeldItemParentMode::HandSocket &&
                   !simulation.Snapshot().heldItems[1].transition.active,
                   "ordinary collision-resolved approach proves warning, stowed draw, attachment and completed hand-ready state in order");

    const auto& finale = simulation.Snapshot();
    checks.Require(finale.playerX >= kKeeperArrivalThreshold.minX &&
                   finale.playerX <= kKeeperArrivalThreshold.maxX &&
                   HasReachedKeeperArrivalThreshold(finale.playerX, finale.playerZ),
                   "full ordinary route stops inside the playable Keeper arrival trigger");
    checks.Require(finale.lich.revealStarted && finale.chestReward.phase ==
                       horde::gameplay::interactions::ChestRewardPhase::Locked,
                   "route reaches the Keeper reveal without tunneling to or claiming the reward");
    checks.Require(finale.fireEmitters[1].strength > 0.99f && finale.fireEmitters[2].strength > 0.99f,
                   "the existing Keeper flank lights activate through the ordinary arrival reveal");
    checks.Require(finale.playerAlive && PairBehindWetline(finale),
                   "the complete route preserves player life and both waterfall guards");

    // Pause at a waterfall checkpoint and exercise the real shared retry/reset paths.
    checks.Require(simulation.ApplyShowcaseCheckpoint(4), "production waterfall checkpoint import succeeds");
    const auto& imported = simulation.Snapshot();
    checks.Require(PairIsHealthyAndStable(imported) &&
                   Near(imported.skeletonEnemies[0].x, kWaterfallSkeletonGuardSpawns[0].position.x) &&
                   Near(imported.skeletonEnemies[0].z, kWaterfallSkeletonGuardSpawns[0].position.z) &&
                   Near(imported.skeletonEnemies[1].x, kWaterfallSkeletonGuardSpawns[1].position.x) &&
                   Near(imported.skeletonEnemies[1].z, kWaterfallSkeletonGuardSpawns[1].position.z),
                   "checkpoint import restores the healthy waterfall pair at authored positions");
    checks.Require(imported.heldItems[1].parentMode ==
                       horde::gameplay::items::HeldItemParentMode::BodyStow &&
                   !imported.heldItems[1].transition.active &&
                   !imported.automaticSwordDrawBlocksDefense &&
                   Near(imported.heldItems[1].visualStowBlend, 1.0f),
                   "checkpoint import restores the complete body-stowed state");
    const std::uint64_t pausedTick = imported.tickIndex;
    const std::array<RoutePosition, 2> pausedGuardPositions{{
        {imported.skeletonEnemies[0].x, imported.skeletonEnemies[0].z},
        {imported.skeletonEnemies[1].x, imported.skeletonEnemies[1].z}}};
    InputSnapshot paused;
    paused.paused = true;
    paused.damageEnabled = false;
    simulation.AdvanceFrame(paused, 0.5, ++publicationSequence);
    checks.Require(simulation.Snapshot().tickIndex == pausedTick &&
                   Near(simulation.Snapshot().skeletonEnemies[0].x, pausedGuardPositions[0].x) &&
                   Near(simulation.Snapshot().skeletonEnemies[0].z, pausedGuardPositions[0].z) &&
                   Near(simulation.Snapshot().skeletonEnemies[1].x, pausedGuardPositions[1].x) &&
                   Near(simulation.Snapshot().skeletonEnemies[1].z, pausedGuardPositions[1].z),
                   "pause freezes checkpoint player and guard state without simulation ticks");

    simulation.RetryEncounter();
    checks.Require(PairIsHealthyAndStable(simulation.Snapshot()) &&
                   Near(simulation.Snapshot().skeletonEnemies[0].x,
                        kWaterfallSkeletonGuardSpawns[0].position.x) &&
                   Near(simulation.Snapshot().skeletonEnemies[0].z,
                        kWaterfallSkeletonGuardSpawns[0].position.z) &&
                   Near(simulation.Snapshot().skeletonEnemies[1].x,
                        kWaterfallSkeletonGuardSpawns[1].position.x) &&
                   Near(simulation.Snapshot().skeletonEnemies[1].z,
                        kWaterfallSkeletonGuardSpawns[1].position.z),
                   "retry restores the same healthy waterfall pair");
    checks.Require(simulation.Snapshot().heldItems[1].parentMode ==
                       horde::gameplay::items::HeldItemParentMode::BodyStow &&
                   !simulation.Snapshot().heldItems[1].transition.active &&
                   !simulation.Snapshot().automaticSwordDrawBlocksDefense &&
                   Near(simulation.Snapshot().heldItems[1].visualStowBlend, 1.0f),
                   "retry restores the complete body-stowed sword state");
    checks.Require(simulation.Snapshot().playerX == kPlayerSpawn.x &&
                   simulation.Snapshot().playerZ == kPlayerSpawn.z,
                   "non-Keeper retry returns to the opening while retaining waterfall guard placement");
    simulation.ResetRoute();
    verifyResetPair(simulation.Snapshot(), "final route reset");

    const auto checkKeeperCheckpointPair = [&checks](const SimulationSnapshot& state,
                                                      std::string_view phase)
    {
        const bool atWaterfall = PairIsHealthyAndStable(state) &&
            Near(state.skeletonEnemies[0].x, kWaterfallSkeletonGuardSpawns[0].position.x) &&
            Near(state.skeletonEnemies[0].z, kWaterfallSkeletonGuardSpawns[0].position.z) &&
            Near(state.skeletonEnemies[1].x, kWaterfallSkeletonGuardSpawns[1].position.x) &&
            Near(state.skeletonEnemies[1].z, kWaterfallSkeletonGuardSpawns[1].position.z);
        if (!atWaterfall)
            std::cerr << "  checkpoint guard mismatch phase=" << phase
                      << " count=" << state.skeletonEnemyCount
                      << " selected=" << static_cast<int>(state.activeEnemyKind)
                      << " A=(" << state.skeletonEnemies[0].x << ',' << state.skeletonEnemies[0].z
                      << ") B=(" << state.skeletonEnemies[1].x << ',' << state.skeletonEnemies[1].z
                      << ")\n";
        checks.Require(atWaterfall,
                       "production bay/Keeper import and retry preserve both authored waterfall guards");
    };
    checks.Require(simulation.ApplyShowcaseCheckpoint(6), "production blue-bay checkpoint import succeeds");
    checkKeeperCheckpointPair(simulation.Snapshot(), "checkpoint 6");
    checks.Require(simulation.ApplyShowcaseCheckpoint(9), "production Keeper checkpoint import succeeds");
    checkKeeperCheckpointPair(simulation.Snapshot(), "checkpoint 9");
    simulation.RetryEncounter();
    checkKeeperCheckpointPair(simulation.Snapshot(), "Keeper retry");
    checks.Require(simulation.Snapshot().playerX == kKeeperRetryPosition.x &&
                   simulation.Snapshot().playerZ == kKeeperRetryPosition.z,
                   "Keeper retry retains its playable arrival spawn while guards stay at the waterfall");
    checks.Require(simulation.Snapshot().heldItems[1].parentMode ==
                       horde::gameplay::items::HeldItemParentMode::BodyStow &&
                   !simulation.Snapshot().heldItems[1].transition.active &&
                   !simulation.Snapshot().automaticSwordDrawBlocksDefense &&
                   Near(simulation.Snapshot().heldItems[1].visualStowBlend, 1.0f),
                   "Keeper retry restores the complete body-stowed sword state");

    TestRetreatDuringAutomaticDraw(config, checks);
    TestPartialDrawResetAndRetry(config, checks);
    TestManualDrawCueOverlap(config, checks);

    if (checks.Failures() != 0)
    {
        std::cerr << "Waterfall production route failed " << checks.Failures() << " checks.\n";
        return 1;
    }
    std::cout << "Waterfall production route passed: stowed cold start/reset/checkpoint/retry, first-room isolation, real fixed-step route and encounter draw/retreat cases, four bays, and Keeper arrival.\n";
    return 0;
}



