#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <type_traits>

#include "gameplay/items/HeldItemState.h"
#include "gameplay/items/UtilityEquipmentLease.h"
#include "gameplay/healing/HealingFlask.h"

namespace
{

using namespace horde::gameplay::items;

int failures = 0;

bool Near(const float left, const float right, const float tolerance = 0.0001f)
{
    return std::abs(left - right) <= tolerance;
}

void Check(const bool condition, const char* message)
{
    if (!condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool SameTransition(const HeldItemTransitionState& left,
                    const HeldItemTransitionState& right)
{
    const auto sameFloat = [](const float a, const float b) {
        return a == b || (std::isnan(a) && std::isnan(b));
    };
    return left.kind == right.kind && left.sourceParent == right.sourceParent &&
           left.targetParent == right.targetParent &&
           sameFloat(left.elapsedSeconds, right.elapsedSeconds) &&
           sameFloat(left.durationSeconds, right.durationSeconds) &&
           sameFloat(left.progress, right.progress) &&
           sameFloat(left.visualStartStowBlend, right.visualStartStowBlend) &&
           sameFloat(left.visualTargetStowBlend, right.visualTargetStowBlend) &&
           sameFloat(left.visualStartGripBlend, right.visualStartGripBlend) &&
           sameFloat(left.visualTargetGripBlend, right.visualTargetGripBlend) &&
           left.startedTick == right.startedTick &&
           left.lastTransitionTick == right.lastTransitionTick &&
           left.lastAdvancedTick == right.lastAdvancedTick &&
           left.attachmentEdgeTick == right.attachmentEdgeTick &&
           left.interruptedTick == right.interruptedTick &&
           left.semanticEdgeSequence == right.semanticEdgeSequence &&
           left.active == right.active &&
           left.hasAdvancedTick == right.hasAdvancedTick &&
           left.attachmentApplied == right.attachmentApplied &&
           left.visualOnly == right.visualOnly;
}

void TestDrawSheathStowRestoreAndSingleAttachmentEdges()
{
    HeldItemState sword = MakeHeldItemState(HeldItemId::Sword, HeldHand::RightHand);
    Check(ValidateHeldItemState(sword) &&
              sword.parentMode == HeldItemParentMode::HandSocket &&
              Near(sword.visualGripBlend, 1.0f),
          "legacy held-item factory must remain valid and hand-attached by default");

    const auto sheath = RequestHeldItemTransition(
        sword, HeldItemTransitionKind::Sheath, 100u);
    Check(sheath.status == HeldItemTransitionRequestStatus::Started &&
              sheath.semanticEdgeSequence == 1u && sword.transition.active &&
              sword.transition.sourceParent == HeldItemParentMode::HandSocket &&
              sword.transition.targetParent == HeldItemParentMode::BodyStow &&
              Near(sword.transition.visualStartGripBlend, 1.0f) &&
              Near(sword.transition.visualTargetGripBlend, 0.0f) &&
              !sword.detached,
          "sheath must begin one hand-to-body transition without world-detaching the item");

    const auto duplicate = RequestHeldItemTransition(
        sword, HeldItemTransitionKind::Stow, 101u);
    Check(duplicate.status == HeldItemTransitionRequestStatus::AlreadyInProgress &&
              duplicate.semanticEdgeSequence == 1u,
          "duplicate stow intent must not restart or emit another transition edge");

    const HeldItemTransitionState beforePause = sword.transition;
    const auto paused = AdvanceHeldItemTransition(sword, 101u, 0.2f, true);
    Check(paused.status == HeldItemTransitionAdvanceStatus::Paused &&
              SameTransition(beforePause, sword.transition),
          "paused advancement must preserve progress and the pending attachment edge");

    for (std::uint64_t tick = 101u; tick < 111u; ++tick)
    {
        const auto step = AdvanceHeldItemTransition(sword, tick, 1.0f / 60.0f);
        Check(step.status == HeldItemTransitionAdvanceStatus::Advanced &&
                  sword.parentMode == HeldItemParentMode::HandSocket &&
                  !sword.transition.attachmentApplied &&
                  Near(sword.visualGripBlend, 1.0f) &&
                  sword.visualStowBlend > 0.0f && sword.visualStowBlend < 1.0f,
              "sheath travel must keep the sword hand-gripped while the item approaches Hips before the edge");
    }
    const auto attachment = AdvanceHeldItemTransition(sword, 111u, 1.0f / 60.0f);
    Check(attachment.status == HeldItemTransitionAdvanceStatus::AttachmentChanged &&
              attachment.attachmentChanged &&
              sword.parentMode == HeldItemParentMode::BodyStow &&
              Near(sword.visualStowBlend, 1.0f) &&
              Near(sword.visualGripBlend, 1.0f) &&
              sword.transition.attachmentApplied &&
              sword.transition.attachmentEdgeTick == 111u &&
              sword.transition.semanticEdgeSequence == 1u && !sword.detached,
          "the fixed-tick edge must occur only after the hand and item reach the Hips Grip");

    for (std::uint64_t tick = 112u; tick <= 122u; ++tick)
        AdvanceHeldItemTransition(sword, tick, 1.0f / 60.0f);
    Check(!sword.transition.active &&
              sword.transition.progress == 1.0f &&
              Near(sword.visualGripBlend, 0.0f) &&
              Near(sword.visualStowBlend, 1.0f) &&
              sword.transition.attachmentEdgeTick == 111u &&
              ValidateHeldItemState(sword),
          "the hand must release only after the item is fully stowed, retaining one edge provenance");
    const auto alreadyStowed = RequestHeldItemTransition(
        sword, HeldItemTransitionKind::Sheath, 123u);
    Check(alreadyStowed.status == HeldItemTransitionRequestStatus::AlreadyAtTarget &&
              alreadyStowed.semanticEdgeSequence == 1u,
          "a completed sheath must not generate another event while already stowed");

    const auto restore = RequestHeldItemTransition(
        sword, HeldItemTransitionKind::Restore, 123u);
    Check(restore.status == HeldItemTransitionRequestStatus::Started &&
              restore.semanticEdgeSequence == 2u &&
              sword.transition.sourceParent == HeldItemParentMode::BodyStow &&
              sword.transition.targetParent == HeldItemParentMode::HandSocket,
          "restore must reuse the same authority in the body-to-hand direction");
}

void TestInterruptionAndReversalResolveToStableAttachment()
{
    HeldItemState sword = MakeHeldItemState(HeldItemId::Sword, HeldHand::RightHand);
    RequestHeldItemTransition(sword, HeldItemTransitionKind::Sheath, 10u);
    AdvanceHeldItemTransition(sword, 11u, 1.0f / 60.0f);
    const auto reverseBeforeEdge = RequestHeldItemTransition(
        sword, HeldItemTransitionKind::Draw, 12u);
    Check(reverseBeforeEdge.status == HeldItemTransitionRequestStatus::InterruptedAndStarted &&
              sword.parentMode == HeldItemParentMode::HandSocket &&
              sword.transition.active && sword.transition.visualOnly &&
              !sword.transition.attachmentApplied &&
              Near(sword.transition.visualStartGripBlend, sword.visualGripBlend) &&
              sword.transition.semanticEdgeSequence == 2u &&
              ValidateHeldItemState(sword),
          "pre-edge reversal must smoothly settle toward the source parent without a phantom attachment");

    const float blendAtReversal = sword.visualStowBlend;
    const float gripBlendAtReversal = sword.visualGripBlend;
    const auto paused = AdvanceHeldItemTransition(sword, 13u, 0.1f, true);
    Check(paused.status == HeldItemTransitionAdvanceStatus::Paused &&
              sword.visualStowBlend == blendAtReversal &&
              sword.visualGripBlend == gripBlendAtReversal,
          "paused visual-settle reversal must preserve its exact attachment blend");
    for (std::uint64_t tick = 13u; tick < 40u; ++tick)
        AdvanceHeldItemTransition(sword, tick, 1.0f / 60.0f);
    Check(!sword.transition.active && Near(sword.visualStowBlend, 0.0f) &&
              sword.parentMode == HeldItemParentMode::HandSocket,
          "reversed pre-edge visual settle must return continuously to the hand endpoint");

    RequestHeldItemTransition(sword, HeldItemTransitionKind::Stow, 40u);
    for (std::uint64_t tick = 41u; tick <= 49u; ++tick)
        AdvanceHeldItemTransition(sword, tick, 1.0f / 60.0f);
    Check(sword.parentMode == HeldItemParentMode::BodyStow &&
              Near(sword.visualStowBlend, 1.0f) &&
              Near(sword.visualGripBlend, 1.0f) &&
              sword.transition.attachmentApplied &&
              sword.transition.attachmentEdgeTick == 49u,
          "a later stow must reach the mounted Grip before its own exact fixed-tick edge");

    const auto reverseAfterEdge = RequestHeldItemTransition(
        sword, HeldItemTransitionKind::Draw, 50u);
    Check(reverseAfterEdge.status ==
              HeldItemTransitionRequestStatus::InterruptedAndStarted &&
              reverseAfterEdge.semanticEdgeSequence == 4u &&
              sword.parentMode == HeldItemParentMode::BodyStow &&
              sword.transition.sourceParent == HeldItemParentMode::BodyStow &&
              sword.transition.targetParent == HeldItemParentMode::HandSocket,
          "post-edge reversal must recover at the attached parent before starting a new transition");

    for (std::uint64_t tick = 51u; tick <= 62u; ++tick)
        AdvanceHeldItemTransition(sword, tick, 1.0f / 60.0f);
    Check(sword.parentMode == HeldItemParentMode::HandSocket &&
              sword.transition.attachmentApplied &&
              sword.transition.attachmentEdgeTick == 62u &&
              sword.transition.semanticEdgeSequence == 4u,
          "the reversed draw must produce one new attachment edge with increasing sequence");

    Check(InterruptHeldItemTransition(sword, 63u) &&
              !sword.transition.active &&
              sword.parentMode == HeldItemParentMode::HandSocket &&
              sword.transition.attachmentEdgeTick == 62u &&
              ValidateHeldItemState(sword),
          "interrupting after attachment must recover at the target parent and retain provenance");
}

void TestAttachmentSignalSurvivesSameTickCompletion()
{
    HeldItemState sword = MakeHeldItemState(HeldItemId::Sword, HeldHand::RightHand);
    RequestHeldItemTransition(sword, HeldItemTransitionKind::Sheath, 50u);
    const auto completed = AdvanceHeldItemTransition(
        sword, 51u, kHeldItemSheathDurationSeconds);
    Check(completed.status == HeldItemTransitionAdvanceStatus::Completed &&
              completed.attachmentChanged &&
              completed.attachmentEdgeTick == 51u &&
              sword.parentMode == HeldItemParentMode::BodyStow &&
              !sword.transition.active,
          "completion and attachment on one fixed tick must still report the attachment edge");
    const auto idle = AdvanceHeldItemTransition(sword, 52u, 1.0f / 60.0f);
    Check(idle.status == HeldItemTransitionAdvanceStatus::Idle &&
              !idle.attachmentChanged &&
              idle.attachmentEdgeTick == 51u,
          "a completed attachment edge must never be emitted again on later ticks");
}

void TestSnapshotRoundTripAndMalformedStateRejection()
{
    static_assert(std::is_trivially_copyable_v<HeldItemState>);
    static_assert(std::is_trivially_copyable_v<HeldItemStates>);

    HeldItemState original = MakeHeldItemState(
        HeldItemId::Sword, HeldHand::RightHand, HeldItemParentMode::BodyStow);
    Check(ValidateHeldItemState(original) && !original.detached,
          "body-stow factory state must be distinct from authored world ownership");
    RequestHeldItemTransition(original, HeldItemTransitionKind::Draw, 200u);
    AdvanceHeldItemTransition(original, 201u, 1.0f / 60.0f);
    const HeldItemState restored = original;
    Check(ValidateHeldItemState(restored) &&
              SameTransition(restored.transition, original.transition),
          "transition POD state must survive an exact snapshot copy");

    HeldItemState continuedOriginal = original;
    HeldItemState continuedRestored = restored;
    for (std::uint64_t tick = 202u; tick <= 213u; ++tick)
    {
        AdvanceHeldItemTransition(continuedOriginal, tick, 1.0f / 60.0f);
        AdvanceHeldItemTransition(continuedRestored, tick, 1.0f / 60.0f);
    }
    Check(continuedOriginal.parentMode == continuedRestored.parentMode &&
              SameTransition(continuedOriginal.transition,
                             continuedRestored.transition) &&
              continuedRestored.transition.attachmentEdgeTick == 212u,
          "restored state must resume deterministically through the same attachment tick");

    HeldItemState malformed = original;
    malformed.transition.progress = std::numeric_limits<float>::quiet_NaN();
    const HeldItemState unchanged = malformed;
    Check(!ValidateHeldItemState(malformed) &&
              RequestHeldItemTransition(malformed, HeldItemTransitionKind::Sheath, 214u).status ==
                  HeldItemTransitionRequestStatus::RejectedInvalidState &&
              AdvanceHeldItemTransition(malformed, 214u, 1.0f / 60.0f).status ==
                  HeldItemTransitionAdvanceStatus::RejectedInvalidState &&
              SameTransition(malformed.transition, unchanged.transition),
          "malformed progress must be rejected without mutating transition state");

    HeldItemState regressed = original;
    AdvanceHeldItemTransition(regressed, 202u, 1.0f / 60.0f);
    const HeldItemTransitionState beforeRegression = regressed.transition;
    Check(AdvanceHeldItemTransition(regressed, 201u, 1.0f / 60.0f).status ==
              HeldItemTransitionAdvanceStatus::RejectedInvalidState &&
              SameTransition(regressed.transition, beforeRegression),
          "a fixed-tick regression must be rejected without moving the transition clock");

    Check(RequestHeldItemTransition(regressed, HeldItemTransitionKind::Restore, 201u).status ==
              HeldItemTransitionRequestStatus::RejectedInvalidState &&
              SameTransition(regressed.transition, beforeRegression),
          "a request timestamp older than the latest transition tick must be rejected");
    Check(!InterruptHeldItemTransition(regressed, 201u) &&
              SameTransition(regressed.transition, beforeRegression),
          "an interruption timestamp older than the latest transition tick must be rejected");
}

void TestDeathInterruptFreezesVisualBlendAndRestoredStateRecovers()
{
    for (const float interruptedProgress : {0.20f, 0.60f})
    {
        HeldItemState sword = MakeHeldItemState(
            HeldItemId::Sword, HeldHand::RightHand,
            HeldItemParentMode::BodyStow);
        Check(RequestHeldItemTransition(
                  sword, HeldItemTransitionKind::Draw, 400u).status ==
                  HeldItemTransitionRequestStatus::Started,
              "death fixture must start from a stable stowed sword");
        AdvanceHeldItemTransition(sword, 401u,
            kHeldItemDrawDurationSeconds * interruptedProgress);
        const float displayedBlend = sword.visualStowBlend;
        const float displayedGripBlend = sword.visualGripBlend;
        const HeldItemParentMode displayedParent = sword.parentMode;

        // GameSimulation interrupts an active sword transition when player
        // vitality leaves Alive. It must freeze the exact render blend and
        // whichever stable side owns the semantic attachment edge.
        Check(InterruptHeldItemTransition(sword, 402u) &&
                  !sword.transition.active &&
                  sword.visualStowBlend == displayedBlend &&
                  sword.visualGripBlend == displayedGripBlend &&
                  sword.parentMode == displayedParent &&
                  ValidateHeldItemState(sword),
              "death interruption must freeze the displayed blend and parent without snapping");

        HeldItemState restored = sword;
        Check(ValidateHeldItemState(restored) &&
                  restored.visualStowBlend == displayedBlend &&
                  restored.visualGripBlend == displayedGripBlend &&
                  restored.parentMode == displayedParent,
              "a death-frozen mid-transition snapshot must restore its exact render pose");
        const auto recovered = RequestHeldItemTransition(
            restored,
            displayedParent == HeldItemParentMode::BodyStow
                ? HeldItemTransitionKind::Draw
                : HeldItemTransitionKind::Stow,
            403u);
        Check(recovered.status == HeldItemTransitionRequestStatus::Started &&
                  restored.visualStowBlend == displayedBlend &&
                  ValidateHeldItemState(restored),
              "recovery must resume from the saved pose without a request-time jump");
        AdvanceHeldItemTransition(restored, 404u, 1.0f / 60.0f);
        Check(std::abs(restored.visualStowBlend - displayedBlend) < 0.01f &&
                  ValidateHeldItemState(restored),
              "the first recovered fixed step must move continuously from the frozen pose");

        // A paused/dead renderer continues to derive the same pose from this
        // immutable blend; only a later simulation tick may change it.
        const float frozenBlend = sword.visualStowBlend;
        const float frozenGripBlend = sword.visualGripBlend;
        AdvanceHeldItemTransition(sword, 403u, 1.0f / 60.0f);
        Check(sword.visualStowBlend == frozenBlend &&
                  sword.visualGripBlend == frozenGripBlend &&
                  sword.parentMode == displayedParent,
              "an inactive death-frozen transition must stay visually fixed until recovery");
    }
}

void TestResetAndTorchWorldOwnershipRemainSeparate()
{
    HeldItemStates items = MakeDefaultHeldItemStates();
    RequestHeldItemTransition(items[1], HeldItemTransitionKind::Sheath, 300u);
    const std::uint64_t swordSequence = items[1].transition.semanticEdgeSequence;
    ResetHeldItemStates(items);
    Check(items[1].parentMode == HeldItemParentMode::HandSocket &&
              !items[1].transition.active &&
              items[1].transition.semanticEdgeSequence == swordSequence &&
              ValidateHeldItemState(items[1]),
          "route reset must cancel transitions at canonical attachment without reusing semantic IDs");

    HeldItemState stowedTorch = MakeHeldItemState(
        HeldItemId::OriginalTorch, HeldHand::LeftHand, HeldItemParentMode::BodyStow);
    stowedTorch.worldFromItem[12] = 2.0f;
    UpdateHeldItemParent(stowedTorch, HeldItemParentMode::AuthoredWorldTrajectory,
                         321u, stowedTorch.worldFromItem);
    Check(stowedTorch.parentMode == HeldItemParentMode::AuthoredWorldTrajectory &&
              stowedTorch.detached && stowedTorch.detachTick == 321u &&
              stowedTorch.worldFromDetach[12] == 2.0f &&
              !stowedTorch.transition.active &&
              ValidateHeldItemState(stowedTorch),
          "dropping a body-stowed torch must enter world trajectory with continuous detach basis");
    Check(RequestHeldItemTransition(stowedTorch, HeldItemTransitionKind::Restore, 322u).status ==
              HeldItemTransitionRequestStatus::RejectedWorldOwned,
          "held-item transitions must not take ownership from a dropped torch trajectory");
}

void TestTorchDropInterruptsBeforeChangingOwnership()
{
    HeldItemState beforeEdge = MakeHeldItemState(
        HeldItemId::OriginalTorch, HeldHand::LeftHand);
    beforeEdge.worldFromItem[12] = 1.25f;
    RequestHeldItemTransition(beforeEdge, HeldItemTransitionKind::Stow, 400u);
    AdvanceHeldItemTransition(beforeEdge, 401u, 1.0f / 60.0f);
    const std::uint64_t beforeEdgeSequence =
        beforeEdge.transition.semanticEdgeSequence;
    UpdateHeldItemParent(beforeEdge, HeldItemParentMode::AuthoredWorldTrajectory,
                         402u, beforeEdge.worldFromItem);
    Check(beforeEdge.parentMode == HeldItemParentMode::AuthoredWorldTrajectory &&
              beforeEdge.detached && beforeEdge.detachTick == 402u &&
              beforeEdge.worldFromDetach[12] == 1.25f &&
              !beforeEdge.transition.active &&
              !beforeEdge.transition.attachmentApplied &&
              beforeEdge.transition.semanticEdgeSequence == beforeEdgeSequence &&
              beforeEdge.transition.interruptedTick == 402u &&
              ValidateHeldItemState(beforeEdge),
          "a drop before the attachment edge must interrupt first and retain the hand detach basis");

    HeldItemState afterEdge = MakeHeldItemState(
        HeldItemId::OriginalTorch, HeldHand::LeftHand);
    afterEdge.worldFromItem[12] = -0.75f;
    RequestHeldItemTransition(afterEdge, HeldItemTransitionKind::Stow, 410u);
    for (std::uint64_t tick = 411u; tick <= 419u; ++tick)
        AdvanceHeldItemTransition(afterEdge, tick, 1.0f / 60.0f);
    const std::uint64_t attachmentEdgeTick = afterEdge.transition.attachmentEdgeTick;
    const std::uint64_t afterEdgeSequence =
        afterEdge.transition.semanticEdgeSequence;
    Check(afterEdge.parentMode == HeldItemParentMode::BodyStow &&
              afterEdge.transition.active && afterEdge.transition.attachmentApplied,
          "the post-edge drop fixture must still be in transition at the body attachment");
    UpdateHeldItemParent(afterEdge, HeldItemParentMode::AuthoredWorldTrajectory,
                         420u, afterEdge.worldFromItem);
    Check(afterEdge.parentMode == HeldItemParentMode::AuthoredWorldTrajectory &&
              afterEdge.detached && afterEdge.detachTick == 420u &&
              afterEdge.worldFromDetach[12] == -0.75f &&
              !afterEdge.transition.active && afterEdge.transition.attachmentApplied &&
              afterEdge.transition.attachmentEdgeTick == attachmentEdgeTick &&
              afterEdge.transition.semanticEdgeSequence == afterEdgeSequence &&
              afterEdge.transition.interruptedTick == 420u &&
              ValidateHeldItemState(afterEdge),
          "a drop after attachment must interrupt at the stowed parent before world trajectory owns it");
}

void TestUtilityLeaseProvenanceAndCancellation()
{
    static_assert(std::is_trivially_copyable_v<UtilityEquipmentLease>);
    for (const auto cancelTick : {0u, 1u, 9u, 18u, 36u, 54u, 74u})
    {
        auto sword = MakeHeldItemState(HeldItemId::Sword, HeldHand::RightHand);
        UtilityEquipmentLease lease;
        Check(lease.Begin(sword, 0) && lease.OwnsTransition(sword),
              "utility owns only its fresh Stow transition");
        const auto sequence = sword.transition.semanticEdgeSequence;
        Check(!lease.Begin(sword, 0) && sword.transition.semanticEdgeSequence == sequence,
              "duplicate utility admission never creates another edge");
        for (unsigned tick = 1; tick <= cancelTick; ++tick)
        {
            AdvanceHeldItemTransition(sword, tick, 1.f/60.f);
            lease.Observe(sword);
            Check(lease.AllowsUtilityProgress(sword),
                  "an owned Stow and completed body attachment do not self-cancel utility");
        }
        const auto beforeReturn = sword.transition;
        Check(!lease.RequestReturn(sword, cancelTick, false) &&
                  SameTransition(beforeReturn, sword.transition),
              "cancellation cannot reclaim the sword before the hand releases the utility");
        Check(lease.RequestReturn(sword, cancelTick, true),
              "a released utility can return through the existing reversal/Restore path");
        const auto returnSequence = sword.transition.semanticEdgeSequence;
        if (sword.transition.active)
        {
            const auto paused = sword.transition;
            AdvanceHeldItemTransition(sword, cancelTick+1, 1.f/60.f, true);
            lease.Observe(sword);
            Check(SameTransition(paused, sword.transition) && lease.OwnsTransition(sword),
                  "pause freezes the equipment clock while preserving lease provenance");
            Check(lease.RequestReturn(sword, cancelTick+1, true) &&
                      sword.transition.semanticEdgeSequence == returnSequence,
                  "repeated return intent does not restart an owned Restore");
        }
        for (unsigned tick = cancelTick+1; tick <= cancelTick+22; ++tick)
        {
            AdvanceHeldItemTransition(sword, tick, 1.f/60.f);
            lease.Observe(sword);
        }
        Check(lease.Phase() == UtilityEquipmentPhase::Returned &&
                  sword.parentMode == HeldItemParentMode::HandSocket &&
                  !sword.transition.active && sword.visualGripBlend == 1.f &&
                  sword.visualStowBlend == 0.f && ValidateHeldItemState(sword),
              "early/middle/late cancellation settles the original owner without snapping readiness");
    }

    auto sword = MakeHeldItemState(HeldItemId::Sword, HeldHand::RightHand);
    UtilityEquipmentLease lease;
    Check(lease.Begin(sword, 0), "foreign-transition fixture begins");
    AdvanceHeldItemTransition(sword, 1, 1.f/60.f);
    InterruptHeldItemTransition(sword, 1);
    RequestHeldItemTransition(sword, HeldItemTransitionKind::Stow, 2);
    const auto foreign = sword.transition;
    Check(!lease.OwnsTransition(sword) && !lease.RequestReturn(sword, 3, true) &&
              lease.Phase() == UtilityEquipmentPhase::Invalidated &&
              SameTransition(foreign, sword.transition),
          "same-kind foreign Stow is rejected by sequence rather than borrowed or interrupted");
    lease.Reset();
    Check(lease.Phase() == UtilityEquipmentPhase::Idle &&
              SameTransition(foreign, sword.transition) && sword.transition.active,
          "lifecycle token reset does not silently reset equipment or grant readiness");
    Check(!lease.Begin(sword, 3), "a new utility cannot adopt an existing transition");

    for (auto invalid : {MakeHeldItemState(HeldItemId::OriginalTorch, HeldHand::LeftHand),
                         MakeHeldItemState(HeldItemId::Sword, HeldHand::LeftHand),
                         MakeHeldItemState(HeldItemId::Sword, HeldHand::RightHand,
                                           HeldItemParentMode::WorldObject)})
    {
        UtilityEquipmentLease rejected;
        Check(!rejected.Begin(invalid, 1) && rejected.Phase() == UtilityEquipmentPhase::Idle,
              "utility cannot take left-light, wrong-hand or world ownership");
    }
    auto exhausted = MakeHeldItemState(HeldItemId::Sword, HeldHand::RightHand);
    exhausted.transition.semanticEdgeSequence = std::numeric_limits<std::uint64_t>::max();
    UtilityEquipmentLease rejected;
    Check(!rejected.Begin(exhausted, 1) && !exhausted.transition.active,
          "sequence exhaustion never creates an unidentifiable utility transition");

    auto stowed = MakeHeldItemState(HeldItemId::Sword, HeldHand::RightHand,
                                    HeldItemParentMode::BodyStow);
    UtilityEquipmentLease sheathed;
    Check(sheathed.Begin(stowed, 0) && !sheathed.OwnsTransition(stowed) &&
              sheathed.RequestReturn(stowed, 75, true) &&
              sheathed.Phase() == UtilityEquipmentPhase::Returned &&
              stowed.parentMode == HeldItemParentMode::BodyStow &&
              stowed.transition.semanticEdgeSequence == 0,
          "a sheathed start returns sheathed without an automatic draw");
}

void TestUtilityLeaseSnapshotContinuation()
{
    auto sword = MakeHeldItemState(HeldItemId::Sword, HeldHand::RightHand);
    UtilityEquipmentLease lease;
    Check(lease.Begin(sword, 1), "utility snapshot fixture begins");
    for (unsigned tick=1; tick<=9; ++tick)
        AdvanceHeldItemTransition(sword,tick,1.f/60.f);
    const auto checkpointSword = sword;
    const auto checkpointLease = lease;
    auto restoredSword = checkpointSword;
    auto restoredLease = checkpointLease;
    for (unsigned tick=10; tick<=80; ++tick)
    {
        if (tick==54)
        {
            Check(lease.RequestReturn(sword,tick,true) &&
                      restoredLease.RequestReturn(restoredSword,tick,true),
                  "copied owner state makes the same single return request");
        }
        AdvanceHeldItemTransition(sword,tick,1.f/60.f);
        AdvanceHeldItemTransition(restoredSword,tick,1.f/60.f);
        lease.Observe(sword); restoredLease.Observe(restoredSword);
        Check(SameTransition(sword.transition,restoredSword.transition) &&
                  sword.parentMode==restoredSword.parentMode &&
                  sword.visualGripBlend==restoredSword.visualGripBlend &&
                  sword.visualStowBlend==restoredSword.visualStowBlend &&
                  lease.Phase()==restoredLease.Phase(),
              "copied transient owner state continues deterministically without another clock");
    }
}

struct UtilityClockObservation
{
    unsigned readyTick=0, healTick=0, charges=0;
};

UtilityClockObservation ObserveUtilityClock(const double handReturnCue,
                                            const bool guardCompletionReadiness)
{
    namespace healing = horde::gameplay::healing;
    healing::HealingFlask flask;
    healing::FlaskContext context{2,true,false,false,false};
    auto sword=MakeHeldItemState(HeldItemId::Sword,HeldHand::RightHand);
    UtilityEquipmentLease lease;
    Check(flask.BeginDrink(context) && lease.Begin(sword,1),
          "clock composition admits utility before the first equipment advance");
    UtilityClockObservation observation;
    constexpr float dt=1.f/60.f;
    for (unsigned tick=1; tick<=80; ++tick)
    {
        const auto before=flask.Snapshot();
        if (before.action==healing::FlaskAction::Drinking &&
            before.elapsedSeconds+1e-9>=handReturnCue)
            Check(lease.RequestReturn(sword,tick,true), "hand-return cue requests owned Restore");
        AdvanceHeldItemTransition(sword,tick,dt);
        lease.Observe(sword);
        const bool ready=lease.Phase()==UtilityEquipmentPhase::Returned &&
            !sword.transition.active && sword.parentMode==HeldItemParentMode::HandSocket;
        if (ready && observation.readyTick==0) observation.readyTick=tick;
        const bool completionTick=before.action==healing::FlaskAction::Drinking &&
            before.elapsedSeconds+dt+1e-9>=healing::HealingFlask::kDrinkSeconds;
        context.drinkEligible=(!sword.transition.active || lease.OwnsTransition(sword)) &&
            (!guardCompletionReadiness || !completionTick || ready);
        const auto completion=flask.Step(dt,context);
        if (completion.completed) observation.healTick=tick;
    }
    observation.charges=flask.Snapshot().resources.charges;
    return observation;
}

void TestUtilityClockReadinessPrerequisite()
{
    // These are source-stage caller-order experiments, not production binding.
    // The retained art returns the flask at0.89s. A proposed53/60 cue requires
    // separately retimed/contact-reviewed hand art before it can be admitted.
    const auto unguarded=ObserveUtilityClock(.89,false);
    const auto guarded=ObserveUtilityClock(.89,true);
    const auto aligned=ObserveUtilityClock(53./60.,true);
    Check(unguarded.readyTick==76 && unguarded.healTick==75 && unguarded.charges==1,
          "literal source cue exposes the one-tick equipment/health completion mismatch");
    Check(guarded.readyTick==76 && guarded.healTick==0 && guarded.charges==2,
          "readiness rejection cancels an incomplete composition without consuming a charge");
    Check(aligned.readyTick==75 && aligned.healTick==75 && aligned.charges==1,
          "provisional tick-aligned hand return can meet tick75 with unchanged Restore duration");
    std::cout<<"Utility source clock: literal-ready="<<unguarded.readyTick
             <<", literal-heal="<<unguarded.healTick
             <<", guarded-heal="<<guarded.healTick
             <<", proposed-ready="<<aligned.readyTick
             <<", proposed-heal="<<aligned.healTick<<'\n';
}

} // namespace

int main()
{
    TestDrawSheathStowRestoreAndSingleAttachmentEdges();
    TestInterruptionAndReversalResolveToStableAttachment();
    TestAttachmentSignalSurvivesSameTickCompletion();
    TestSnapshotRoundTripAndMalformedStateRejection();
    TestDeathInterruptFreezesVisualBlendAndRestoredStateRecovers();
    TestResetAndTorchWorldOwnershipRemainSeparate();
    TestTorchDropInterruptsBeforeChangingOwnership();
    TestUtilityLeaseProvenanceAndCancellation();
    TestUtilityLeaseSnapshotContinuation();
    TestUtilityClockReadinessPrerequisite();
    if (failures != 0)
    {
        std::cerr << failures << " held-item transition checks failed\n";
        return 1;
    }
    std::cout << "Held-item transition checks passed\n";
    return 0;
}
