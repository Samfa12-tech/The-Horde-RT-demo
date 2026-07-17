#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "gameplay/ShowcaseGameplay.h"

namespace horde::gameplay
{

enum class ShowcaseCheckpointPreset
{
    Fresh,
    LanternTrigger,
    LanternSettled,
    LichActive,
    FinaleRoofOpen,
};

struct ShowcaseCheckpoint
{
    std::int32_t id;
    const char* name;
    float x;
    float z;
    float yaw;
    float pitch;
    ShowcaseZone expectedZone;
    ShowcaseCheckpointPreset preset;
};

inline constexpr float kHalfPi = 1.57079632679f;
inline constexpr std::array<ShowcaseCheckpoint, 12> kShowcaseCheckpoints{{
    {0, "opening", 0.0f, 1.85f, 0.0f, -0.05f, ShowcaseZone::Opening, ShowcaseCheckpointPreset::Fresh},
    {1, "skeleton", kSkeletonRoomCenter.x, kSkeletonRoomCenter.z, 0.0f, 0.0f, ShowcaseZone::SkeletonRoom, ShowcaseCheckpointPreset::Fresh},
    {2, "worst-bend", 4.20f, -10.00f, 0.0f, -0.04f, ShowcaseZone::ShadowCorridor, ShowcaseCheckpointPreset::Fresh},
    {3, "lantern-drop", -1.80f, -15.20f, -kHalfPi, -0.08f, ShowcaseZone::ShadowCorridor, ShowcaseCheckpointPreset::LanternTrigger},
    {4, "skylight", -5.50f, -15.20f, 0.0f, 0.22f, ShowcaseZone::SkylightChamber, ShowcaseCheckpointPreset::LanternSettled},
    {5, "yellow", -11.00f, -15.20f, -kHalfPi, -0.02f, ShowcaseZone::YellowTorchBay, ShowcaseCheckpointPreset::LanternSettled},
    {6, "blue", -16.00f, -15.20f, -kHalfPi, -0.02f, ShowcaseZone::BlueTorchBay, ShowcaseCheckpointPreset::LanternSettled},
    {7, "red", -21.00f, -15.20f, -kHalfPi, -0.02f, ShowcaseZone::RedTorchBay, ShowcaseCheckpointPreset::LanternSettled},
    {8, "green", -26.00f, -15.20f, -kHalfPi, -0.02f, ShowcaseZone::GreenTorchBay, ShowcaseCheckpointPreset::LanternSettled},
    {9, "mirror", -33.70f, -15.20f, -kHalfPi, 0.0f, ShowcaseZone::Finale, ShowcaseCheckpointPreset::LichActive},
    {10, "lich", -33.25f, -14.25f, 2.52f, 0.0f, ShowcaseZone::Finale, ShowcaseCheckpointPreset::LichActive},
    {11, "finale-roof", -35.50f, -15.20f, kHalfPi, 0.28f, ShowcaseZone::Finale, ShowcaseCheckpointPreset::FinaleRoofOpen},
}};

constexpr const ShowcaseCheckpoint* FindShowcaseCheckpoint(std::int32_t id)
{
    for (const ShowcaseCheckpoint& checkpoint : kShowcaseCheckpoints)
    {
        if (checkpoint.id == id)
        {
            return &checkpoint;
        }
    }
    return nullptr;
}

constexpr const ShowcaseCheckpoint* FindShowcaseCheckpoint(const char* name)
{
    if (name == nullptr)
    {
        return nullptr;
    }
    for (const ShowcaseCheckpoint& checkpoint : kShowcaseCheckpoints)
    {
        const char* left = checkpoint.name;
        const char* right = name;
        while (*left != '\0' && *right != '\0' && *left == *right)
        {
            ++left;
            ++right;
        }
        if (*left == '\0' && *right == '\0')
        {
            return &checkpoint;
        }
    }
    return nullptr;
}

struct ShowcaseCheckpointState
{
    LanternSequence lantern;
    LanternSnapshot lanternSnapshot;
    EnemyDirector enemyDirector;
    LichEncounter lichEncounter;
    EnemyKind activeEnemyKind = EnemyKind::Skeleton;
};

inline void AdvanceLanternToSettled(ShowcaseCheckpointState& state)
{
    constexpr float triggerX = -1.80f;
    constexpr float triggerZ = -15.20f;
    state.lantern.Update(0.01f, triggerX, triggerZ, -kHalfPi, -0.08f);
    for (int frame = 0; frame < 25; ++frame)
    {
        state.lantern.Update(0.05f, triggerX, triggerZ, -kHalfPi, -0.08f);
    }
    state.lanternSnapshot = state.lantern.Snapshot();
}

inline void AdvanceLichToActive(ShowcaseCheckpointState& state, const ShowcaseCheckpoint& checkpoint)
{
    state.enemyDirector.Update(checkpoint.x, checkpoint.z);
    state.activeEnemyKind = state.enemyDirector.Snapshot().selectedEnemy;
    state.lichEncounter.Reset();
    state.lichEncounter.Update(0.05f, checkpoint.x, checkpoint.z, true, true);
}

inline void AdvanceLichToOpenRoof(ShowcaseCheckpointState& state, const ShowcaseCheckpoint& checkpoint)
{
    AdvanceLichToActive(state, checkpoint);
    for (int hit = 0; hit < 3; ++hit)
    {
        const LichSnapshot& beforeHit = state.lichEncounter.Snapshot();
        state.lichEncounter.TryAcceptPlayerHit(beforeHit.x, beforeHit.z);
        if (hit < 2)
        {
            for (int frame = 0; frame < 40; ++frame)
            {
                state.lichEncounter.Update(0.05f, checkpoint.x, checkpoint.z, true, true);
            }
        }
    }
    const float completionSeconds = LichEncounter::kDeathAnimationDuration +
                                    LichEncounter::kFinaleSkylightOpenDuration + 0.05f;
    const int completionFrames = static_cast<int>(completionSeconds / 0.05f) + 1;
    for (int frame = 0; frame < completionFrames; ++frame)
    {
        state.lichEncounter.Update(0.05f, checkpoint.x, checkpoint.z, true, true);
    }
    if (state.lichEncounter.Snapshot().deathAnimationComplete)
    {
        state.enemyDirector.MarkSelectedDead();
    }
}

inline ShowcaseCheckpointState BuildShowcaseCheckpointState(const ShowcaseCheckpoint& checkpoint)
{
    ShowcaseCheckpointState state;
    state.enemyDirector.Reset();
    state.lantern.Reset();
    state.lichEncounter.Reset();

    if (checkpoint.preset == ShowcaseCheckpointPreset::LanternTrigger)
    {
        state.lanternSnapshot = state.lantern.Update(
            0.01f, checkpoint.x, checkpoint.z, checkpoint.yaw, checkpoint.pitch);
    }
    else if (checkpoint.preset == ShowcaseCheckpointPreset::LanternSettled ||
             checkpoint.preset == ShowcaseCheckpointPreset::LichActive ||
             checkpoint.preset == ShowcaseCheckpointPreset::FinaleRoofOpen)
    {
        AdvanceLanternToSettled(state);
    }

    state.enemyDirector.Update(checkpoint.x, checkpoint.z);
    state.activeEnemyKind = state.enemyDirector.Snapshot().selectedEnemy;
    if (checkpoint.preset == ShowcaseCheckpointPreset::LichActive)
    {
        AdvanceLichToActive(state, checkpoint);
    }
    else if (checkpoint.preset == ShowcaseCheckpointPreset::FinaleRoofOpen)
    {
        AdvanceLichToOpenRoof(state, checkpoint);
    }
    return state;
}

constexpr const char* LanternPhaseName(LanternPhase phase)
{
    switch (phase)
    {
    case LanternPhase::Held: return "held";
    case LanternPhase::Guttering: return "guttering";
    case LanternPhase::Falling: return "falling";
    case LanternPhase::Settled: return "settled";
    default: return "unknown";
    }
}

constexpr const char* EnemyKindName(EnemyKind kind)
{
    switch (kind)
    {
    case EnemyKind::Skeleton: return "skeleton";
    case EnemyKind::Lich: return "lich";
    default: return "none";
    }
}

constexpr const char* LichPhaseName(LichPhase phase)
{
    switch (phase)
    {
    case LichPhase::Dormant: return "dormant";
    case LichPhase::MaintainingRange: return "maintaining-range";
    case LichPhase::Charging: return "charging";
    case LichPhase::Recovering: return "recovering";
    case LichPhase::Dead: return "dead";
    default: return "unknown";
    }
}

} // namespace horde::gameplay
