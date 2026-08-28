#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/interactions/ChestRewardSequence.h"
#include "gameplay/interactions/InteractionState.h"

namespace horde::gameplay
{

enum class ShowcaseCheckpointPreset
{
    Fresh,
    TwoSkeletonCombat,
    TorchFailureTrigger,
    TorchFailureSettled,
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
inline constexpr std::array<ShowcaseCheckpoint, 13> kShowcaseCheckpoints{{
    {0, "opening", 0.0f, 1.85f, 0.0f, -0.05f, ShowcaseZone::Opening, ShowcaseCheckpointPreset::Fresh},
    {1, "skeleton", kSkeletonRoomCenter.x, kSkeletonRoomCenter.z, 0.0f, 0.0f, ShowcaseZone::SkeletonRoom, ShowcaseCheckpointPreset::Fresh},
    {2, "worst-bend", 4.20f, -10.00f, 0.0f, -0.04f, ShowcaseZone::ShadowCorridor, ShowcaseCheckpointPreset::Fresh},
    {3, "lantern-drop", -1.80f, -15.20f, -kHalfPi, -0.08f, ShowcaseZone::ShadowCorridor, ShowcaseCheckpointPreset::TorchFailureTrigger},
    {4, "skylight", -5.50f, -15.20f, kHalfPi, -0.16f, ShowcaseZone::SkylightChamber, ShowcaseCheckpointPreset::TorchFailureSettled},
    {5, "yellow", -11.00f, -15.20f, -kHalfPi, -0.02f, ShowcaseZone::YellowTorchBay, ShowcaseCheckpointPreset::TorchFailureSettled},
    {6, "blue", -16.00f, -15.20f, -kHalfPi, -0.02f, ShowcaseZone::BlueTorchBay, ShowcaseCheckpointPreset::TorchFailureSettled},
    {7, "red", -21.00f, -15.20f, -kHalfPi, -0.02f, ShowcaseZone::RedTorchBay, ShowcaseCheckpointPreset::TorchFailureSettled},
    {8, "green", -26.00f, -15.20f, -kHalfPi, -0.02f, ShowcaseZone::GreenTorchBay, ShowcaseCheckpointPreset::TorchFailureSettled},
    {9, "mirror", -33.70f, -15.20f, -kHalfPi, 0.0f, ShowcaseZone::Finale, ShowcaseCheckpointPreset::LichActive},
    {10, "lich", -33.25f, -14.25f, 2.52f, 0.0f, ShowcaseZone::Finale, ShowcaseCheckpointPreset::LichActive},
    {11, "finale-roof", -35.50f, -15.20f, kHalfPi, 0.28f, ShowcaseZone::Finale, ShowcaseCheckpointPreset::FinaleRoofOpen},
    {12, "two-enemy-combat", 0.0f, -3.50f, 0.0f, 0.0f, ShowcaseZone::SkeletonRoom, ShowcaseCheckpointPreset::TwoSkeletonCombat},
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
    TorchFailureSequence torchFailure;
    TorchFailureSnapshot torchFailureSnapshot;
    EnemyDirector enemyDirector;
    LichEncounter lichEncounter;
    horde::gameplay::interactions::ChestRewardSequence chestRewardSequence;
    horde::gameplay::interactions::FinaleSequence finaleSequence;
    horde::gameplay::interactions::InteractionState interactionState;
    EnemyKind activeEnemyKind = EnemyKind::Skeleton;
};

inline void AdvanceTorchFailureToSettled(ShowcaseCheckpointState& state)
{
    constexpr float triggerX = -1.80f;
    constexpr float triggerZ = -15.20f;
    state.torchFailure.Update(0.01f, triggerX, triggerZ, -kHalfPi, -0.08f);
    for (int frame = 0; frame < 25; ++frame)
    {
        state.torchFailure.Update(0.05f, triggerX, triggerZ, -kHalfPi, -0.08f);
    }
    state.torchFailureSnapshot = state.torchFailure.Snapshot();
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
    const float completionSeconds = LichEncounter::kDeathAnimationDuration + 0.05f;
    const int completionFrames = static_cast<int>(completionSeconds / 0.05f) + 1;
    for (int frame = 0; frame < completionFrames; ++frame)
    {
        state.lichEncounter.Update(0.05f, checkpoint.x, checkpoint.z, true, true);
    }
    if (state.lichEncounter.Snapshot().deathAnimationComplete)
    {
        state.enemyDirector.MarkSelectedDead();
    }
    using namespace horde::gameplay::interactions;
    state.finaleSequence.NotifyLichDefeated();
    state.chestRewardSequence.Unlock();
    const InteractionQuery atChest{
        kRewardChestInteractionPosition.x,
        kRewardChestInteractionPosition.z,
        0.0f};
    state.chestRewardSequence.TryInteract(atChest);
    state.chestRewardSequence.Update(ChestRewardSequence::kOpeningDurationSeconds);
    state.chestRewardSequence.TryInteract(atChest);
    EquipRewardLantern(state.interactionState);
    state.interactionState.heldLightPose = HeldLightPose::High;
    state.interactionState.heldLightPoseProgress = 1.0f;
    state.finaleSequence.NotifyLanternClaimed();
    state.finaleSequence.Update(kFinaleLanternRaiseSeconds +
                                kFinaleLanternRevealSeconds +
                                kFinaleSkylightOpenSeconds + 0.05f);
}

inline ShowcaseCheckpointState BuildShowcaseCheckpointState(const ShowcaseCheckpoint& checkpoint)
{
    ShowcaseCheckpointState state;
    state.enemyDirector.Reset();
    state.torchFailure.Reset();
    state.lichEncounter.Reset();
    state.chestRewardSequence.Reset();
    state.finaleSequence.Reset();
    horde::gameplay::interactions::ResetInteractionState(
        state.interactionState,
        horde::gameplay::interactions::HeldLightKind::Torch);

    if (checkpoint.preset == ShowcaseCheckpointPreset::TorchFailureTrigger)
    {
        state.torchFailureSnapshot = state.torchFailure.Update(
            0.01f, checkpoint.x, checkpoint.z, checkpoint.yaw, checkpoint.pitch);
    }
    else if (checkpoint.preset == ShowcaseCheckpointPreset::TorchFailureSettled ||
             checkpoint.preset == ShowcaseCheckpointPreset::LichActive ||
             checkpoint.preset == ShowcaseCheckpointPreset::FinaleRoofOpen)
    {
        AdvanceTorchFailureToSettled(state);
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

constexpr const char* TorchFailurePhaseName(TorchFailurePhase phase)
{
    switch (phase)
    {
    case TorchFailurePhase::Held: return "held";
    case TorchFailurePhase::Guttering: return "guttering";
    case TorchFailurePhase::Falling: return "falling";
    case TorchFailurePhase::Settled: return "settled";
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
constexpr const char* FinaleEndingPhaseName(FinaleEndingPhase phase)
{
    switch (phase)
    {
    case FinaleEndingPhase::Inactive: return "inactive";
    case FinaleEndingPhase::LichFalling: return "lich-falling";
    case FinaleEndingPhase::SkylightOpening: return "skylight-opening";
    case FinaleEndingPhase::DawnRevealed: return "dawn-revealed";
    case FinaleEndingPhase::Complete: return "complete";
    default: return "unknown";
    }
}

} // namespace horde::gameplay
