#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/SwordCombat.h"
#include "gameplay/simulation/GameplayEvent.h"

namespace horde::gameplay::simulation
{

inline constexpr std::size_t kSkeletonEnemyCapacity = 2u;

struct SkeletonEnemySnapshot
{
    EntityId id = EntityId::Invalid;
    float x = 0.0f;
    float z = -4.65f;
    float facingRadians = 0.0f;
    float animationTime = 0.0f;
    float damageFlash = 0.0f;
    std::int32_t health = 1;
    EnemyAnimation animation = EnemyAnimation::Walking;
    EnemyCombatAction action = EnemyCombatAction::Locomotion;
    CombatReaction reaction = CombatReaction::None;
    float actionTime = 0.0f;
    float reactionTime = 0.0f;
    bool dead = false;
    bool playerHitPulse = false;
    bool parrySuccessPulse = false;
};

struct SimulationSnapshot
{
    std::uint64_t tickIndex = 0;
    std::uint64_t inputPublicationSequence = 0;
    std::uint64_t lastConsumedAttackSequence = 0;
    std::uint64_t lastConsumedParrySequence = 0;
    std::uint64_t lastConsumedRouteResetSequence = 0;
    std::uint64_t lastConsumedRetrySequence = 0;

    float playerX = 0.0f;
    float playerZ = 1.85f;
    float playerYawRadians = 0.0f;
    float playerPitchRadians = -0.05f;
    float playerTravelledThisTick = 0.0f;
    float walkTime = 0.0f;
    float walkAmount = 0.0f;
    float lanternStrength = 1.8f;

    ShowcaseZone zone = ShowcaseZone::Opening;
    EnemyKind activeEnemyKind = EnemyKind::Skeleton;
    EntityId activeEnemyId = EntityId::SkeletonA;
    std::array<SkeletonEnemySnapshot, kSkeletonEnemyCapacity> skeletonEnemies{};
    std::size_t skeletonEnemyCount = kSkeletonEnemyCapacity;
    std::size_t activeSkeletonCount = kSkeletonEnemyCapacity;
    EntityId skeletonAttackerId = EntityId::Invalid;
    bool openingEncounterComplete = false;
    std::int32_t retryCheckpoint = 0;
    std::uint32_t retryGeneration = 0;
    bool paused = false;
    bool playerAlive = true;
    bool finaleComplete = false;

    LanternSnapshot lantern{};
    EnemyRosterSnapshot enemyRoster{};
    CombatSnapshot swordCombat{};
    PlayerCombatSnapshot playerCombat{};
    LichSnapshot lich{};
    PlayerVitalsSnapshot playerVitals{};

    std::uint32_t simulationTicksThisFrame = 0;
    double fixedStepAccumulatorSeconds = 0.0;
    std::uint64_t catchUpOverrunCount = 0;
    std::size_t queuedEventCount = 0;
    std::size_t eventQueueHighWaterMark = 0;
    std::uint64_t eventQueueOverflowCount = 0;
    std::size_t eventsEmittedThisTick = 0;
    std::size_t eventsEmittedThisFrame = 0;
};

} // namespace horde::gameplay::simulation
