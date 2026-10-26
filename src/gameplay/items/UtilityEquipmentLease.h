#pragma once

#include "gameplay/items/HeldItemState.h"

namespace horde::gameplay::items
{

enum class UtilityEquipmentPhase : std::uint8_t
{
    Idle,
    Owned,
    Returning,
    Returned,
    Invalidated,
};

// A transient simulation-owned equipment token, independent of visual clocks.
// The simulation remains the sole clock and advances HeldItemState once. A
// caller starts the lease BEFORE that tick's equipment advance, observes the
// result afterwards, and requests return only once its hand profile has actually
// released the utility. This class never grants healing, defense or input edges.
// Transient leases are copied with owner state, not serialized into saved games.
class UtilityEquipmentLease
{
public:
    UtilityEquipmentPhase Phase() const { return phase_; }
    HeldItemParentMode OriginalParent() const { return originalParent_; }

    bool Begin(HeldItemState& sword, const std::uint64_t tick)
    {
        if (phase_ != UtilityEquipmentPhase::Idle || !IsSword(sword) ||
            sword.transition.active || tick < sword.transition.lastTransitionTick ||
            !StableAt(sword, sword.parentMode))
            return false;

        const auto original = sword.parentMode;
        if (original == HeldItemParentMode::HandSocket)
        {
            const auto result = RequestHeldItemTransition(
                sword, HeldItemTransitionKind::Stow, tick);
            if (result.status != HeldItemTransitionRequestStatus::Started)
                return false;
        }
        originalParent_ = original;
        Record(sword);
        phase_ = UtilityEquipmentPhase::Owned;
        return true;
    }

    bool OwnsTransition(const HeldItemState& sword) const
    {
        return HasLease() && sword.transition.active && Matches(sword);
    }

    bool AllowsUtilityProgress(const HeldItemState& sword) const
    {
        return (HasLease() || phase_ == UtilityEquipmentPhase::Returned) && Matches(sword);
    }

    bool ReturnedFor(const HeldItemState& sword) const
    {
        return phase_ == UtilityEquipmentPhase::Returned && Matches(sword);
    }

    bool RequestReturn(HeldItemState& sword, const std::uint64_t tick,
                       const bool handProfileReleasedUtility)
    {
        Observe(sword);
        if (ReturnedFor(sword)) return handProfileReleasedUtility;
        if (!HasLease() || !handProfileReleasedUtility)
            return false;
        if (phase_ == UtilityEquipmentPhase::Returning)
            return true; // An unchanged hold never restarts the return clock.
        if (originalParent_ == HeldItemParentMode::BodyStow)
        {
            phase_ = UtilityEquipmentPhase::Returned; // Preserve a sheathed start.
            return true;
        }
        const auto result = RequestHeldItemTransition(
            sword, HeldItemTransitionKind::Restore, tick);
        if ((result.status == HeldItemTransitionRequestStatus::Interrupted ||
             result.status == HeldItemTransitionRequestStatus::AlreadyAtTarget) &&
            StableAt(sword, originalParent_))
        {
            Record(sword);
            phase_ = UtilityEquipmentPhase::Returned;
            return true;
        }
        if (result.status != HeldItemTransitionRequestStatus::Started &&
            result.status != HeldItemTransitionRequestStatus::InterruptedAndStarted)
            return false;
        Record(sword);
        phase_ = UtilityEquipmentPhase::Returning;
        return true;
    }

    void Observe(const HeldItemState& sword)
    {
        if (!HasLease() && phase_ != UtilityEquipmentPhase::Returned)
            return;
        if (!Matches(sword))
            phase_ = UtilityEquipmentPhase::Invalidated;
        else if (phase_ == UtilityEquipmentPhase::Returning && !sword.transition.active)
            phase_ = UtilityEquipmentPhase::Returned;
    }

    // Reset/load/death drops provenance only. Existing equipment ownership must
    // separately settle/reset the sword; this is not a readiness shortcut.
    void Reset() { *this = {}; }

private:
    static bool IsSword(const HeldItemState& sword)
    {
        return ValidateHeldItemState(sword) && sword.id == HeldItemId::Sword &&
            sword.hand == HeldHand::RightHand && !sword.detached;
    }

    static bool StableAt(const HeldItemState& sword, const HeldItemParentMode parent)
    {
        if (sword.transition.active || sword.parentMode != parent)
            return false;
        if (parent == HeldItemParentMode::HandSocket)
            return sword.visualStowBlend == 0.f && sword.visualGripBlend == 1.f;
        return parent == HeldItemParentMode::BodyStow &&
            sword.visualStowBlend == 1.f && sword.visualGripBlend == 0.f;
    }

    bool HasLease() const
    {
        return phase_ == UtilityEquipmentPhase::Owned ||
            phase_ == UtilityEquipmentPhase::Returning;
    }

    bool Matches(const HeldItemState& sword) const
    {
        if (!IsSword(sword) || sword.transition.semanticEdgeSequence != sequence_)
            return false;
        if (sword.transition.active)
            return transitionOwned_ && sword.transition.kind == kind_ &&
                sword.transition.startedTick == startedTick_;
        return StableAt(sword, phase_ == UtilityEquipmentPhase::Returning || phase_ == UtilityEquipmentPhase::Returned
            ? originalParent_ : HeldItemParentMode::BodyStow);
    }

    void Record(const HeldItemState& sword)
    {
        sequence_ = sword.transition.semanticEdgeSequence;
        startedTick_ = sword.transition.startedTick;
        kind_ = sword.transition.kind;
        transitionOwned_ = sword.transition.active;
    }

    UtilityEquipmentPhase phase_ = UtilityEquipmentPhase::Idle;
    HeldItemParentMode originalParent_ = HeldItemParentMode::HandSocket;
    HeldItemTransitionKind kind_ = HeldItemTransitionKind::None;
    std::uint64_t sequence_ = 0;
    std::uint64_t startedTick_ = 0;
    bool transitionOwned_ = false;
};

} // namespace horde::gameplay::items
