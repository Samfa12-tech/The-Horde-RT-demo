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

bool MoveByOrdinaryAxes(GameSimulation& simulation,
                        RoutePosition target,
                        std::uint64_t& publicationSequence,
                        Checks& checks,
                        std::string_view stage,
                        bool& sawDistinctWalkingPhase,
                        bool& guardsStayedBehindWetline,
                        bool& reentryMovedContinuously)
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
        input.damageEnabled = false; // This fixture checks route geometry, not combat damage.
        input.moveStrafe = std::clamp(dx / distance, -1.0f, 1.0f);
        input.moveForward = std::clamp(-dz / distance, -1.0f, 1.0f);
        const RoutePosition oldGuardA{before.skeletonEnemies[0].x, before.skeletonEnemies[0].z};
        const RoutePosition oldGuardB{before.skeletonEnemies[1].x, before.skeletonEnemies[1].z};
        const bool wasInsideArena = IsWaterfallSkeletonArena(before.playerX, before.playerZ);

        simulation.StepFixed(input, kFixedDelta, ++publicationSequence);
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
                      bool& reentryMovedContinuously)
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
        if (!MoveByOrdinaryAxes(simulation, target, publicationSequence,
                                checks, stage, sawDistinctWalkingPhase,
                                guardsStayedBehindWetline, reentryMovedContinuously))
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
                                    guardsStayedBehindWetline, reentryMovedContinuously))
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
                                    guardsStayedBehindWetline, reentryMovedContinuously))
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
    checks.Require(!config.swordStartsStowed,
                   "guard placement remains independently gated from sword stow");

    GameSimulation simulation(config);
    std::uint64_t publicationSequence = simulation.Snapshot().inputPublicationSequence;
    const auto verifyResetPair = [&checks](const SimulationSnapshot& state, std::string_view phase, bool coldStart = false)
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
        checks.Require(state.heldItems[1].parentMode == horde::gameplay::items::HeldItemParentMode::HandSocket,
                       "production guard placement does not enable the separately gated sword stow");
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
            event.type != GameplayEventType::PlayerDamaged;
    checks.Require(firstRoomPairStayedAtSpawn && firstRoomHadNoAttackOrDamageEvents,
                   "normal-damage first-room ticks do not relocate, attack or damage from the waterfall pair");

    simulation.ResetRoute();
    verifyResetPair(simulation.Snapshot(), "route reset");
    checks.Require(simulation.Snapshot().zone == ShowcaseZone::Opening,
                   "route reset returns the player to the actual opening");

    bool sawDistinctWalkingPhase = false;
    bool guardsStayedBehindWetline = true;
    bool reentryMovedContinuously = true;
    MoveThroughRoute(simulation, publicationSequence, checks, sawDistinctWalkingPhase,
                     guardsStayedBehindWetline, reentryMovedContinuously);
    checks.Require(guardsStayedBehindWetline,
                   "guards remain behind the wetline throughout ordinary route movement");
    checks.Require(reentryMovedContinuously,
                   "arena re-entry advances guards from retained positions without teleport");
    checks.Require(sawDistinctWalkingPhase,
                   "approach observes both live guards walking with their authored distinct gait phase");

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
    checks.Require(imported.heldItems[1].parentMode == horde::gameplay::items::HeldItemParentMode::HandSocket,
                   "checkpoint import leaves sword stow disabled");
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
                       horde::gameplay::items::HeldItemParentMode::HandSocket,
                   "retry leaves sword stow disabled");
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
                       horde::gameplay::items::HeldItemParentMode::HandSocket,
                   "Keeper retry leaves sword stow disabled");

    if (checks.Failures() != 0)
    {
        std::cerr << "Waterfall production route failed " << checks.Failures() << " checks.\n";
        return 1;
    }
    std::cout << "Waterfall production route passed: cold start, first-room isolation, real fixed-step route through four bays, Keeper arrival, pause, checkpoint, retry and reset.\n";
    return 0;
}



