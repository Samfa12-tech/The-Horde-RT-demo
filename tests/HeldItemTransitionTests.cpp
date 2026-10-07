#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <type_traits>

#include "gameplay/items/HeldItemState.h"

namespace
{

using namespace horde::gameplay::items;

int failures = 0;

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
           left.startedTick == right.startedTick &&
           left.lastTransitionTick == right.lastTransitionTick &&
           left.lastAdvancedTick == right.lastAdvancedTick &&
           left.attachmentEdgeTick == right.attachmentEdgeTick &&
           left.interruptedTick == right.interruptedTick &&
           left.semanticEdgeSequence == right.semanticEdgeSequence &&
           left.active == right.active &&
           left.hasAdvancedTick == right.hasAdvancedTick &&
           left.attachmentApplied == right.attachmentApplied;
}

void TestDrawSheathStowRestoreAndSingleAttachmentEdges()
{
    HeldItemState sword = MakeHeldItemState(HeldItemId::Sword, HeldHand::RightHand);
    Check(ValidateHeldItemState(sword) &&
              sword.parentMode == HeldItemParentMode::HandSocket,
          "legacy held-item factory must remain valid and hand-attached by default");

    const auto sheath = RequestHeldItemTransition(
        sword, HeldItemTransitionKind::Sheath, 100u);
    Check(sheath.status == HeldItemTransitionRequestStatus::Started &&
              sheath.semanticEdgeSequence == 1u && sword.transition.active &&
              sword.transition.sourceParent == HeldItemParentMode::HandSocket &&
              sword.transition.targetParent == HeldItemParentMode::BodyStow &&
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
                  !sword.transition.attachmentApplied,
              "the source parent must remain authoritative before the midpoint edge");
    }
    const auto attachment = AdvanceHeldItemTransition(sword, 111u, 1.0f / 60.0f);
    Check(attachment.status == HeldItemTransitionAdvanceStatus::AttachmentChanged &&
              attachment.attachmentChanged &&
              sword.parentMode == HeldItemParentMode::BodyStow &&
              sword.transition.attachmentApplied &&
              sword.transition.attachmentEdgeTick == 111u &&
              sword.transition.semanticEdgeSequence == 1u && !sword.detached,
          "the fixed-tick midpoint must apply one recorded body-stow attachment edge");

    for (std::uint64_t tick = 112u; tick <= 122u; ++tick)
        AdvanceHeldItemTransition(sword, tick, 1.0f / 60.0f);
    Check(!sword.transition.active &&
              sword.transition.progress == 1.0f &&
              sword.transition.attachmentEdgeTick == 111u &&
              ValidateHeldItemState(sword),
          "completion must retain the exact single edge provenance in stable state");
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
    Check(reverseBeforeEdge.status == HeldItemTransitionRequestStatus::Interrupted &&
              sword.parentMode == HeldItemParentMode::HandSocket &&
              !sword.transition.active && !sword.transition.attachmentApplied &&
              sword.transition.semanticEdgeSequence == 1u &&
              sword.transition.interruptedTick == 12u &&
              ValidateHeldItemState(sword),
          "pre-edge reversal must cancel at the source parent without a phantom attachment");

    RequestHeldItemTransition(sword, HeldItemTransitionKind::Stow, 13u);
    for (std::uint64_t tick = 14u; tick <= 22u; ++tick)
        AdvanceHeldItemTransition(sword, tick, 1.0f / 60.0f);
    Check(sword.parentMode == HeldItemParentMode::BodyStow &&
              sword.transition.attachmentApplied &&
              sword.transition.attachmentEdgeTick == 22u,
          "a later stow must attach once at its own exact fixed tick");

    const auto reverseAfterEdge = RequestHeldItemTransition(
        sword, HeldItemTransitionKind::Draw, 23u);
    Check(reverseAfterEdge.status ==
              HeldItemTransitionRequestStatus::InterruptedAndStarted &&
              reverseAfterEdge.semanticEdgeSequence == 3u &&
              sword.parentMode == HeldItemParentMode::BodyStow &&
              sword.transition.sourceParent == HeldItemParentMode::BodyStow &&
              sword.transition.targetParent == HeldItemParentMode::HandSocket,
          "post-edge reversal must recover at the attached parent before starting a new transition");

    for (std::uint64_t tick = 24u; tick <= 35u; ++tick)
        AdvanceHeldItemTransition(sword, tick, 1.0f / 60.0f);
    Check(sword.parentMode == HeldItemParentMode::HandSocket &&
              sword.transition.attachmentApplied &&
              sword.transition.attachmentEdgeTick == 35u &&
              sword.transition.semanticEdgeSequence == 3u,
          "the reversed draw must produce one new attachment edge with increasing sequence");

    Check(InterruptHeldItemTransition(sword, 36u) &&
              !sword.transition.active &&
              sword.parentMode == HeldItemParentMode::HandSocket &&
              sword.transition.attachmentEdgeTick == 35u &&
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

} // namespace

int main()
{
    TestDrawSheathStowRestoreAndSingleAttachmentEdges();
    TestInterruptionAndReversalResolveToStableAttachment();
    TestAttachmentSignalSurvivesSameTickCompletion();
    TestSnapshotRoundTripAndMalformedStateRejection();
    TestResetAndTorchWorldOwnershipRemainSeparate();
    TestTorchDropInterruptsBeforeChangingOwnership();
    if (failures != 0)
    {
        std::cerr << failures << " held-item transition checks failed\n";
        return 1;
    }
    std::cout << "Held-item transition checks passed\n";
    return 0;
}
