#pragma once

#include <array>
#include <vector>
#include <cstdint>
#include "gameplay/SwordCombat.h"
#include "gameplay/simulation/GameplayEvent.h"

namespace horde::gameplay::simulation
{
enum class CombatContactOutcome : std::uint8_t
{
    NoContact, AcceptedSwordContact, DodgeProtected, PracticeMiss, DamageAccepted, PostHitProtected
};
struct CombatContactSample
{
    std::uint64_t sequence = 0, tick = 0, attackId = 0, commandSequence = 0;
    std::uint64_t consumedTick = 0, semanticEventSequence = 0;
    PlayerAttackCut cut = PlayerAttackCut::None;
    PlayerCombatAction phase = PlayerCombatAction::Idle;
    EntityId target = EntityId::Invalid;
    CombatContactOutcome outcome = CombatContactOutcome::NoContact;
    float phaseSeconds = 0, separationMetres = 0, supportWorldY = kRouteFloorWorldY;
    float dodgeElapsedSeconds = 0;
    bool hasSeparation = false, hasBlade = false;
    std::array<float,3> bladeStart{}, bladeEnd{};
    // The actual fixed-step attachment transform, not a synthetic blade or
    // triangle measurement. Mesh separation remains unavailable unless a
    // measured contact evaluator supplies it.
    bool hasSwordTransform = false;
    std::array<float,16> worldFromSword{};
    float targetX = 0, targetZ = 0;
};
struct CombatContactTraceSnapshot
{
    static constexpr std::uint32_t kCapacity = 32;
    // Lazy owned storage keeps the bounded diagnostics off the stack. Copies
    // own their samples, so publishing a snapshot cannot alias the owner's
    // next write. No allocation is needed by untouched legacy encounters.
    std::vector<CombatContactSample> samples{};
    std::uint32_t count = 0, nextIndex = 0;
    std::uint64_t nextSequence = 1, overwritten = 0, generation = 1;
    CombatContactSample& Push(CombatContactSample sample)
    {
        if (samples.size() != kCapacity) samples.resize(kCapacity);
        sample.sequence = nextSequence++;
        samples[nextIndex] = sample;
        auto& result = samples[nextIndex];
        nextIndex = (nextIndex+1)%kCapacity;
        if(count<kCapacity) ++count; else ++overwritten;
        return result;
    }
    void Clear()
    {
        samples.clear(); count=nextIndex=0; ++generation;
        // Preserve monotonic identity so an old present reporter cannot replay
        // a fresh generation as previously delivered contact rows.
    }
};
} // namespace horde::gameplay::simulation
