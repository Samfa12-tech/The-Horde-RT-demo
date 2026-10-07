#include "gameplay/items/HeldItemState.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace horde::gameplay::items
{

HeldItemTransform IdentityHeldItemTransform()
{
    return {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f}};
}

namespace
{

HeldItemTransform TranslationTransform(const float x, const float y, const float z)
{
    HeldItemTransform result = IdentityHeldItemTransform();
    result[12] = x;
    result[13] = y;
    result[14] = z;
    return result;
}

} // namespace

HeldItemTransform OriginalTorchGripSocketTransform()
{
    return TranslationTransform(0.0f, 0.24f, 0.0f);
}

HeldItemTransform OriginalTorchFlameSocketTransform()
{
    return TranslationTransform(0.0f, 0.765f, 0.0f);
}

HeldItemTransform OriginalTorchLightSocketTransform()
{
    return TranslationTransform(0.0f, 0.735f, 0.025f);
}

HeldItemTransform SwordGripSocketTransform()
{
    return TranslationTransform(0.0f, 0.135f, 0.0f);
}

HeldItemState MakeHeldItemState(const HeldItemId id,
                               const HeldHand hand,
                               const HeldItemParentMode parentMode)
{
    HeldItemState result;
    result.id = id;
    result.hand = hand;
    result.parentMode = parentMode;
    result.transition.sourceParent = parentMode;
    result.transition.targetParent = parentMode;
    result.worldFromItem = IdentityHeldItemTransform();
    result.worldFromDetach = IdentityHeldItemTransform();
    return result;
}

HeldItemStates MakeDefaultHeldItemStates()
{
    return {{
        MakeHeldItemState(HeldItemId::OriginalTorch, HeldHand::LeftHand),
        MakeHeldItemState(HeldItemId::Sword, HeldHand::RightHand),
    }};
}

void UpdateHeldItemParent(HeldItemState& item,
                          const HeldItemParentMode parentMode,
                          const std::uint64_t tick,
                          const HeldItemTransform& resolvedWorldFromItem)
{
    if (parentMode == HeldItemParentMode::AuthoredWorldTrajectory &&
        item.transition.active)
    {
        // Resolve the transition while the current attachment is still valid;
        // the detach below changes ownership to the world trajectory.
        if (!InterruptHeldItemTransition(item, tick))
        {
            return;
        }
    }
    if (parentMode == HeldItemParentMode::AuthoredWorldTrajectory &&
        !item.detached &&
        (item.parentMode == HeldItemParentMode::HandSocket ||
         item.parentMode == HeldItemParentMode::BodyStow))
    {
        item.detached = true;
        item.detachTick = tick;
        item.worldFromDetach = item.worldFromItem;
    }
    item.parentMode = parentMode;
    item.worldFromItem = resolvedWorldFromItem;
    if (!item.transition.active &&
        parentMode != HeldItemParentMode::AuthoredWorldTrajectory &&
        parentMode != HeldItemParentMode::WorldObject)
    {
        item.transition.sourceParent = parentMode;
        item.transition.targetParent = parentMode;
    }
}

namespace
{

bool IsKnownItem(const HeldItemId id)
{
    switch (id)
    {
    case HeldItemId::OriginalTorch:
    case HeldItemId::Sword:
    case HeldItemId::RewardLantern:
        return true;
    }
    return false;
}

bool IsKnownHand(const HeldHand hand)
{
    return hand == HeldHand::LeftHand || hand == HeldHand::RightHand;
}

bool IsKnownParent(const HeldItemParentMode parent)
{
    switch (parent)
    {
    case HeldItemParentMode::HandSocket:
    case HeldItemParentMode::AuthoredWorldTrajectory:
    case HeldItemParentMode::WorldObject:
    case HeldItemParentMode::BodyStow:
        return true;
    }
    return false;
}

bool IsKnownTransition(const HeldItemTransitionKind kind)
{
    switch (kind)
    {
    case HeldItemTransitionKind::None:
    case HeldItemTransitionKind::Draw:
    case HeldItemTransitionKind::Sheath:
    case HeldItemTransitionKind::Stow:
    case HeldItemTransitionKind::Restore:
        return true;
    }
    return false;
}

bool IsStableParent(const HeldItemParentMode parent)
{
    return parent == HeldItemParentMode::HandSocket ||
           parent == HeldItemParentMode::BodyStow;
}

bool IsHandToStow(const HeldItemTransitionKind kind)
{
    return kind == HeldItemTransitionKind::Sheath ||
           kind == HeldItemTransitionKind::Stow;
}

bool IsStowToHand(const HeldItemTransitionKind kind)
{
    return kind == HeldItemTransitionKind::Draw ||
           kind == HeldItemTransitionKind::Restore;
}

float TransitionDuration(const HeldItemTransitionKind kind)
{
    switch (kind)
    {
    case HeldItemTransitionKind::Draw: return kHeldItemDrawDurationSeconds;
    case HeldItemTransitionKind::Sheath: return kHeldItemSheathDurationSeconds;
    case HeldItemTransitionKind::Stow: return kHeldItemStowDurationSeconds;
    case HeldItemTransitionKind::Restore: return kHeldItemRestoreDurationSeconds;
    case HeldItemTransitionKind::None: break;
    }
    return 0.0f;
}

HeldItemParentMode TransitionTarget(const HeldItemTransitionKind kind)
{
    return IsHandToStow(kind) ? HeldItemParentMode::BodyStow
                              : HeldItemParentMode::HandSocket;
}

bool NearlyEqual(const float left, const float right)
{
    return std::abs(left - right) <= 0.0001f;
}

HeldItemTransitionRequestResult BeginTransition(HeldItemState& item,
                                                const HeldItemTransitionKind kind,
                                                const std::uint64_t tick)
{
    HeldItemTransitionRequestResult result;
    if (item.transition.semanticEdgeSequence == std::numeric_limits<std::uint64_t>::max())
    {
        result.status = HeldItemTransitionRequestStatus::RejectedSequenceExhausted;
        result.semanticEdgeSequence = item.transition.semanticEdgeSequence;
        return result;
    }

    const std::uint64_t sequence = item.transition.semanticEdgeSequence + 1u;
    const HeldItemParentMode target = TransitionTarget(kind);
    item.transition = {};
    item.transition.kind = kind;
    item.transition.sourceParent = item.parentMode;
    item.transition.targetParent = target;
    item.transition.durationSeconds = TransitionDuration(kind);
    item.transition.startedTick = tick;
    item.transition.lastTransitionTick = tick;
    item.transition.lastAdvancedTick = tick;
    item.transition.semanticEdgeSequence = sequence;
    item.transition.active = true;
    result.status = HeldItemTransitionRequestStatus::Started;
    result.semanticEdgeSequence = sequence;
    return result;
}

} // namespace

bool ValidateHeldItemState(const HeldItemState& item)
{
    const HeldItemTransitionState& transition = item.transition;
    if (!IsKnownItem(item.id) || !IsKnownHand(item.hand) ||
        !IsKnownParent(item.parentMode) || !IsKnownTransition(transition.kind) ||
        !IsKnownParent(transition.sourceParent) ||
        !IsKnownParent(transition.targetParent) ||
        !std::isfinite(transition.elapsedSeconds) ||
        !std::isfinite(transition.durationSeconds) ||
        !std::isfinite(transition.progress) ||
        transition.elapsedSeconds < 0.0f || transition.durationSeconds < 0.0f ||
        transition.progress < 0.0f || transition.progress > 1.0f ||
        (item.detached &&
         item.parentMode != HeldItemParentMode::AuthoredWorldTrajectory &&
         item.parentMode != HeldItemParentMode::WorldObject))
    {
        return false;
    }

    if (transition.active)
    {
        if (transition.kind == HeldItemTransitionKind::None ||
            !IsStableParent(transition.sourceParent) ||
            !IsStableParent(transition.targetParent) ||
            transition.sourceParent == transition.targetParent ||
            transition.durationSeconds <= 0.0f ||
            transition.elapsedSeconds > transition.durationSeconds ||
            transition.semanticEdgeSequence == 0u ||
            ((transition.kind == HeldItemTransitionKind::Sheath ||
              transition.kind == HeldItemTransitionKind::Stow) &&
             (transition.sourceParent != HeldItemParentMode::HandSocket ||
              transition.targetParent != HeldItemParentMode::BodyStow)) ||
            ((transition.kind == HeldItemTransitionKind::Draw ||
              transition.kind == HeldItemTransitionKind::Restore) &&
             (transition.sourceParent != HeldItemParentMode::BodyStow ||
              transition.targetParent != HeldItemParentMode::HandSocket)) ||
            transition.lastTransitionTick < transition.startedTick ||
            (!transition.hasAdvancedTick &&
             (transition.lastAdvancedTick != transition.startedTick ||
              transition.elapsedSeconds != 0.0f || transition.progress != 0.0f ||
              transition.attachmentApplied)) ||
            (transition.hasAdvancedTick &&
             transition.lastAdvancedTick < transition.startedTick) ||
            !NearlyEqual(transition.progress,
                         transition.elapsedSeconds / transition.durationSeconds) ||
            (transition.attachmentApplied &&
             (item.parentMode != transition.targetParent ||
              transition.progress < kHeldItemAttachmentEdgeProgress ||
              transition.attachmentEdgeTick < transition.startedTick ||
              (transition.hasAdvancedTick &&
               transition.attachmentEdgeTick > transition.lastAdvancedTick))) ||
            (!transition.attachmentApplied &&
             (item.parentMode != transition.sourceParent ||
              transition.attachmentEdgeTick != 0u ||
              transition.progress >= kHeldItemAttachmentEdgeProgress)))
        {
            return false;
        }
        return true;
    }

    const bool worldOwned = item.parentMode == HeldItemParentMode::AuthoredWorldTrajectory ||
                            item.parentMode == HeldItemParentMode::WorldObject;
    if (!worldOwned &&
        (transition.sourceParent != item.parentMode ||
         transition.targetParent != item.parentMode))
    {
        return false;
    }
    if (transition.durationSeconds == 0.0f)
    {
        return transition.elapsedSeconds == 0.0f && transition.progress == 0.0f &&
               !transition.attachmentApplied && transition.attachmentEdgeTick == 0u;
    }
    return transition.semanticEdgeSequence != 0u &&
           transition.attachmentApplied &&
           transition.elapsedSeconds == transition.durationSeconds &&
           transition.progress == 1.0f &&
           transition.attachmentEdgeTick >= transition.startedTick;
}

bool InterruptHeldItemTransition(HeldItemState& item, const std::uint64_t tick)
{
    if (!ValidateHeldItemState(item) || !item.transition.active ||
        tick < item.transition.lastTransitionTick)
    {
        return false;
    }

    HeldItemTransitionState& transition = item.transition;
    transition.active = false;
    transition.interruptedTick = tick;
    transition.lastTransitionTick = tick;
    transition.sourceParent = item.parentMode;
    transition.targetParent = item.parentMode;
    if (transition.attachmentApplied)
    {
        transition.elapsedSeconds = transition.durationSeconds;
        transition.progress = 1.0f;
    }
    else
    {
        transition.elapsedSeconds = 0.0f;
        transition.durationSeconds = 0.0f;
        transition.progress = 0.0f;
        transition.attachmentEdgeTick = 0u;
    }
    return true;
}

HeldItemTransitionRequestResult RequestHeldItemTransition(
    HeldItemState& item,
    const HeldItemTransitionKind kind,
    const std::uint64_t tick)
{
    if (!ValidateHeldItemState(item) ||
        (kind != HeldItemTransitionKind::Draw &&
         kind != HeldItemTransitionKind::Sheath &&
         kind != HeldItemTransitionKind::Stow &&
         kind != HeldItemTransitionKind::Restore))
    {
        return {HeldItemTransitionRequestStatus::RejectedInvalidState,
                item.transition.semanticEdgeSequence};
    }
    if (tick < item.transition.lastTransitionTick)
    {
        return {HeldItemTransitionRequestStatus::RejectedInvalidState,
                item.transition.semanticEdgeSequence};
    }
    if (item.detached || item.parentMode == HeldItemParentMode::AuthoredWorldTrajectory ||
        item.parentMode == HeldItemParentMode::WorldObject)
    {
        return {HeldItemTransitionRequestStatus::RejectedWorldOwned,
                item.transition.semanticEdgeSequence};
    }

    const HeldItemParentMode target = TransitionTarget(kind);
    bool interrupted = false;
    if (item.transition.active)
    {
        if (item.transition.targetParent == target)
        {
            return {HeldItemTransitionRequestStatus::AlreadyInProgress,
                    item.transition.semanticEdgeSequence};
        }
        interrupted = InterruptHeldItemTransition(item, tick);
        if (!interrupted)
        {
            return {HeldItemTransitionRequestStatus::RejectedInvalidState,
                    item.transition.semanticEdgeSequence};
        }
    }

    if (item.parentMode == target)
    {
        return {interrupted ? HeldItemTransitionRequestStatus::Interrupted
                            : HeldItemTransitionRequestStatus::AlreadyAtTarget,
                item.transition.semanticEdgeSequence};
    }
    if (!IsStableParent(item.parentMode))
    {
        return {HeldItemTransitionRequestStatus::RejectedWorldOwned,
                item.transition.semanticEdgeSequence};
    }

    HeldItemTransitionRequestResult result = BeginTransition(item, kind, tick);
    if (interrupted && result.status == HeldItemTransitionRequestStatus::Started)
    {
        result.status = HeldItemTransitionRequestStatus::InterruptedAndStarted;
    }
    return result;
}

HeldItemTransitionAdvanceResult AdvanceHeldItemTransition(
    HeldItemState& item,
    const std::uint64_t tick,
    const float fixedDeltaSeconds,
    const bool paused)
{
    if (!ValidateHeldItemState(item) || !std::isfinite(fixedDeltaSeconds) ||
        fixedDeltaSeconds < 0.0f ||
        (item.transition.active &&
         (tick < item.transition.startedTick ||
          tick < item.transition.lastTransitionTick ||
          (item.transition.hasAdvancedTick &&
           tick <= item.transition.lastAdvancedTick))))
    {
        return {HeldItemTransitionAdvanceStatus::RejectedInvalidState,
                item.transition.semanticEdgeSequence,
                item.transition.attachmentEdgeTick, false};
    }
    if (paused)
    {
        return {HeldItemTransitionAdvanceStatus::Paused,
                item.transition.semanticEdgeSequence,
                item.transition.attachmentEdgeTick, false};
    }
    if (!item.transition.active)
    {
        return {HeldItemTransitionAdvanceStatus::Idle,
                item.transition.semanticEdgeSequence,
                item.transition.attachmentEdgeTick, false};
    }

    HeldItemTransitionState& transition = item.transition;
    const float previousProgress = transition.progress;
    transition.elapsedSeconds = std::min(
        transition.durationSeconds, transition.elapsedSeconds + fixedDeltaSeconds);
    transition.progress = std::clamp(
        transition.elapsedSeconds / transition.durationSeconds, 0.0f, 1.0f);
    transition.lastTransitionTick = tick;
    transition.lastAdvancedTick = tick;
    transition.hasAdvancedTick = true;
    const bool attachedNow = !transition.attachmentApplied &&
        previousProgress < kHeldItemAttachmentEdgeProgress &&
        transition.progress >= kHeldItemAttachmentEdgeProgress;
    if (attachedNow)
    {
        transition.attachmentApplied = true;
        transition.attachmentEdgeTick = tick;
        item.parentMode = transition.targetParent;
    }

    HeldItemTransitionAdvanceStatus status = HeldItemTransitionAdvanceStatus::Advanced;
    if (transition.elapsedSeconds >= transition.durationSeconds)
    {
        transition.active = false;
        transition.sourceParent = transition.targetParent;
        transition.progress = 1.0f;
        status = HeldItemTransitionAdvanceStatus::Completed;
    }
    else if (attachedNow)
    {
        status = HeldItemTransitionAdvanceStatus::AttachmentChanged;
    }
    return {status, transition.semanticEdgeSequence,
            transition.attachmentEdgeTick, attachedNow};
}

void ImportHeldItemCheckpoint(HeldItemStates& items,
                              const bool torchHeldByPlayer,
                              const std::uint64_t tick)
{
    ResetHeldItemStates(items);
    if (!torchHeldByPlayer)
    {
        items[0].parentMode = HeldItemParentMode::AuthoredWorldTrajectory;
        items[0].transition.sourceParent = HeldItemParentMode::AuthoredWorldTrajectory;
        items[0].transition.targetParent = HeldItemParentMode::AuthoredWorldTrajectory;
        items[0].detached = true;
        items[0].detachTick = tick;
    }
}

void ResetHeldItemStates(HeldItemStates& items)
{
    std::uint64_t torchSequence = 0u;
    std::uint64_t swordSequence = 0u;
    for (const HeldItemState& item : items)
    {
        if (item.id == HeldItemId::OriginalTorch)
            torchSequence = std::max(torchSequence, item.transition.semanticEdgeSequence);
        else if (item.id == HeldItemId::Sword)
            swordSequence = std::max(swordSequence, item.transition.semanticEdgeSequence);
    }
    items = MakeDefaultHeldItemStates();
    items[0].transition.semanticEdgeSequence = torchSequence;
    items[1].transition.semanticEdgeSequence = swordSequence;
}

} // namespace horde::gameplay::items
