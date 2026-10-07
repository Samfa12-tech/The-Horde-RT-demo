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
    result.visualStowBlend = parentMode == HeldItemParentMode::BodyStow ? 1.0f : 0.0f;
    result.visualGripBlend = parentMode == HeldItemParentMode::BodyStow ? 0.0f : 1.0f;
    result.transition.sourceParent = parentMode;
    result.transition.targetParent = parentMode;
    result.transition.visualStartStowBlend = result.visualStowBlend;
    result.transition.visualTargetStowBlend = result.visualStowBlend;
    result.transition.visualStartGripBlend = result.visualGripBlend;
    result.transition.visualTargetGripBlend = result.visualGripBlend;
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
        item.visualStowBlend = parentMode == HeldItemParentMode::BodyStow ? 1.0f : 0.0f;
        item.visualGripBlend = parentMode == HeldItemParentMode::BodyStow ? 0.0f : 1.0f;
        item.transition.sourceParent = parentMode;
        item.transition.targetParent = parentMode;
        item.transition.visualStartStowBlend = item.visualStowBlend;
        item.transition.visualTargetStowBlend = item.visualStowBlend;
        item.transition.visualStartGripBlend = item.visualGripBlend;
        item.transition.visualTargetGripBlend = item.visualGripBlend;
    }
}

HeldItemTransform BlendHeldItemTransforms(const HeldItemTransform& from,
                                         const HeldItemTransform& to,
                                         float blend)
{
    struct Quaternion { float x, y, z, w; };
    const auto toQuaternion = [](const HeldItemTransform& value) {
        Quaternion q{};
        const float m00 = value[0], m01 = value[4], m02 = value[8];
        const float m10 = value[1], m11 = value[5], m12 = value[9];
        const float m20 = value[2], m21 = value[6], m22 = value[10];
        const float trace = m00 + m11 + m22;
        if (trace > 0.0f)
        {
            const float s = std::sqrt(trace + 1.0f) * 2.0f;
            q.w = 0.25f * s;
            q.x = (m21 - m12) / s;
            q.y = (m02 - m20) / s;
            q.z = (m10 - m01) / s;
        }
        else if (m00 > m11 && m00 > m22)
        {
            const float s = std::sqrt(std::max(0.0f, 1.0f + m00 - m11 - m22)) * 2.0f;
            q.w = (m21 - m12) / s;
            q.x = 0.25f * s;
            q.y = (m01 + m10) / s;
            q.z = (m02 + m20) / s;
        }
        else if (m11 > m22)
        {
            const float s = std::sqrt(std::max(0.0f, 1.0f + m11 - m00 - m22)) * 2.0f;
            q.w = (m02 - m20) / s;
            q.x = (m01 + m10) / s;
            q.y = 0.25f * s;
            q.z = (m12 + m21) / s;
        }
        else
        {
            const float s = std::sqrt(std::max(0.0f, 1.0f + m22 - m00 - m11)) * 2.0f;
            q.w = (m10 - m01) / s;
            q.x = (m02 + m20) / s;
            q.y = (m12 + m21) / s;
            q.z = 0.25f * s;
        }
        return q;
    };
    const auto normalize = [](Quaternion q) {
        const float length = std::sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
        if (length > 1.0e-8f)
        {
            q.x /= length; q.y /= length; q.z /= length; q.w /= length;
        }
        return q;
    };
    blend = std::clamp(std::isfinite(blend) ? blend : 0.0f, 0.0f, 1.0f);
    Quaternion a = toQuaternion(from);
    Quaternion b = toQuaternion(to);
    float cosine = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
    if (cosine < 0.0f)
    {
        cosine = -cosine;
        b = {-b.x, -b.y, -b.z, -b.w};
    }
    Quaternion q{};
    if (cosine > 0.9995f)
    {
        q = normalize({a.x + (b.x-a.x)*blend,
                       a.y + (b.y-a.y)*blend,
                       a.z + (b.z-a.z)*blend,
                       a.w + (b.w-a.w)*blend});
    }
    else
    {
        const float angle = std::acos(std::clamp(cosine, -1.0f, 1.0f));
        const float sine = std::sin(angle);
        const float wa = std::sin((1.0f-blend)*angle) / sine;
        const float wb = std::sin(blend*angle) / sine;
        q = {wa*a.x + wb*b.x, wa*a.y + wb*b.y,
             wa*a.z + wb*b.z, wa*a.w + wb*b.w};
    }
    const float xx=q.x*q.x, yy=q.y*q.y, zz=q.z*q.z;
    const float xy=q.x*q.y, xz=q.x*q.z, yz=q.y*q.z;
    const float wx=q.w*q.x, wy=q.w*q.y, wz=q.w*q.z;
    HeldItemTransform result = IdentityHeldItemTransform();
    result[0] = 1.0f - 2.0f*(yy+zz);
    result[1] = 2.0f*(xy+wz);
    result[2] = 2.0f*(xz-wy);
    result[4] = 2.0f*(xy-wz);
    result[5] = 1.0f - 2.0f*(xx+zz);
    result[6] = 2.0f*(yz+wx);
    result[8] = 2.0f*(xz+wy);
    result[9] = 2.0f*(yz-wx);
    result[10] = 1.0f - 2.0f*(xx+yy);
    for (std::size_t axis = 0u; axis < 3u; ++axis)
        result[12u+axis] = from[12u+axis] + (to[12u+axis]-from[12u+axis])*blend;
    return result;
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

float SmoothStep(const float value)
{
    const float t = std::clamp(value, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
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
    item.transition.visualStartStowBlend = item.visualStowBlend;
    item.transition.visualTargetStowBlend =
        target == HeldItemParentMode::BodyStow ? 1.0f : 0.0f;
    item.transition.visualStartGripBlend = item.visualGripBlend;
    item.transition.visualTargetGripBlend =
        target == HeldItemParentMode::BodyStow ? 0.0f : 1.0f;
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

HeldItemTransitionRequestResult BeginVisualSettle(HeldItemState& item,
                                                  const HeldItemTransitionKind kind,
                                                  const std::uint64_t tick)
{
    if (item.transition.semanticEdgeSequence == std::numeric_limits<std::uint64_t>::max())
        return {HeldItemTransitionRequestStatus::RejectedSequenceExhausted,
                item.transition.semanticEdgeSequence};
    const auto sequence = item.transition.semanticEdgeSequence + 1u;
    item.transition = {};
    item.transition.kind = kind;
    item.transition.sourceParent = item.parentMode;
    item.transition.targetParent = item.parentMode;
    item.transition.durationSeconds = TransitionDuration(kind);
    item.transition.visualStartStowBlend = item.visualStowBlend;
    item.transition.visualTargetStowBlend =
        item.parentMode == HeldItemParentMode::BodyStow ? 1.0f : 0.0f;
    item.transition.visualStartGripBlend = item.visualGripBlend;
    item.transition.visualTargetGripBlend =
        item.parentMode == HeldItemParentMode::BodyStow ? 0.0f : 1.0f;
    item.transition.startedTick = tick;
    item.transition.lastTransitionTick = tick;
    item.transition.lastAdvancedTick = tick;
    item.transition.semanticEdgeSequence = sequence;
    item.transition.active = true;
    item.transition.visualOnly = true;
    return {HeldItemTransitionRequestStatus::Started, sequence};
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
        !std::isfinite(transition.visualStartStowBlend) ||
        !std::isfinite(transition.visualTargetStowBlend) ||
        !std::isfinite(transition.visualStartGripBlend) ||
        !std::isfinite(transition.visualTargetGripBlend) ||
        !std::isfinite(item.visualStowBlend) ||
        !std::isfinite(item.visualGripBlend) ||
        transition.elapsedSeconds < 0.0f || transition.durationSeconds < 0.0f ||
        transition.progress < 0.0f || transition.progress > 1.0f ||
        transition.visualStartStowBlend < 0.0f || transition.visualStartStowBlend > 1.0f ||
        transition.visualTargetStowBlend < 0.0f || transition.visualTargetStowBlend > 1.0f ||
        transition.visualStartGripBlend < 0.0f || transition.visualStartGripBlend > 1.0f ||
        transition.visualTargetGripBlend < 0.0f || transition.visualTargetGripBlend > 1.0f ||
        item.visualStowBlend < 0.0f || item.visualStowBlend > 1.0f ||
        item.visualGripBlend < 0.0f || item.visualGripBlend > 1.0f ||
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
            (transition.sourceParent == transition.targetParent && !transition.visualOnly) ||
            (transition.visualOnly &&
             (transition.sourceParent != item.parentMode ||
              transition.targetParent != item.parentMode ||
              transition.attachmentApplied || transition.attachmentEdgeTick != 0u)) ||
            transition.durationSeconds <= 0.0f ||
            transition.elapsedSeconds > transition.durationSeconds ||
            transition.semanticEdgeSequence == 0u ||
            (!transition.visualOnly && (transition.kind == HeldItemTransitionKind::Sheath ||
              transition.kind == HeldItemTransitionKind::Stow) &&
             (transition.sourceParent != HeldItemParentMode::HandSocket ||
              transition.targetParent != HeldItemParentMode::BodyStow)) ||
            (!transition.visualOnly && (transition.kind == HeldItemTransitionKind::Draw ||
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
            (!transition.visualOnly && !transition.attachmentApplied &&
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
    if (transition.visualOnly)
    {
        return transition.semanticEdgeSequence != 0u &&
               transition.elapsedSeconds == transition.durationSeconds &&
               transition.progress == 1.0f && !transition.attachmentApplied &&
               transition.attachmentEdgeTick == 0u &&
               std::abs(item.visualStowBlend - transition.visualTargetStowBlend) <= 0.0001f &&
               std::abs(item.visualGripBlend - transition.visualTargetGripBlend) <= 0.0001f;
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
    if (transition.visualOnly)
    {
        transition.elapsedSeconds = 0.0f;
        transition.durationSeconds = 0.0f;
        transition.progress = 0.0f;
        transition.visualStartStowBlend = item.visualStowBlend;
        transition.visualTargetStowBlend = item.visualStowBlend;
        transition.visualStartGripBlend = item.visualGripBlend;
        transition.visualTargetGripBlend = item.visualGripBlend;
        transition.visualOnly = false;
    }
    else if (transition.attachmentApplied)
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

    const float targetBlend = target == HeldItemParentMode::BodyStow ? 1.0f : 0.0f;
    const float targetGripBlend = target == HeldItemParentMode::BodyStow ? 0.0f : 1.0f;
    if (item.parentMode == target &&
        std::abs(item.visualStowBlend - targetBlend) <= 0.0001f &&
        std::abs(item.visualGripBlend - targetGripBlend) <= 0.0001f)
    {
        return {interrupted ? HeldItemTransitionRequestStatus::Interrupted
                            : HeldItemTransitionRequestStatus::AlreadyAtTarget,
                item.transition.semanticEdgeSequence};
    }
    if (item.parentMode == target)
    {
        auto result = BeginVisualSettle(item, kind, tick);
        if (interrupted && result.status == HeldItemTransitionRequestStatus::Started)
            result.status = HeldItemTransitionRequestStatus::InterruptedAndStarted;
        return result;
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
    const float eased = SmoothStep(transition.progress);
    if (transition.visualOnly)
    {
        // A reversal before the attachment edge settles on the current owner.
        // Keep the hand attached while a hand-owned item returns to its Grip;
        // a body-owned item stays on Hips while the hand releases/reaches.
        if (transition.targetParent == HeldItemParentMode::BodyStow)
        {
            item.visualStowBlend = transition.visualStartStowBlend;
            item.visualGripBlend = transition.visualStartGripBlend +
                (transition.visualTargetGripBlend - transition.visualStartGripBlend) * eased;
        }
        else
        {
            item.visualStowBlend = transition.visualStartStowBlend +
                (transition.visualTargetStowBlend - transition.visualStartStowBlend) * eased;
            item.visualGripBlend = transition.visualStartGripBlend +
                (transition.visualTargetGripBlend - transition.visualStartGripBlend) * eased;
        }
    }
    else if (transition.targetParent == HeldItemParentMode::BodyStow)
    {
        if (transition.progress < kHeldItemAttachmentEdgeProgress)
        {
            const float phase = SmoothStep(transition.progress * 2.0f);
            item.visualStowBlend = transition.visualStartStowBlend +
                (1.0f - transition.visualStartStowBlend) * phase;
            item.visualGripBlend = transition.visualStartGripBlend +
                (1.0f - transition.visualStartGripBlend) * phase;
        }
        else
        {
            const float phase = SmoothStep(
                (transition.progress - kHeldItemAttachmentEdgeProgress) * 2.0f);
            item.visualStowBlend = 1.0f;
            item.visualGripBlend = 1.0f +
                (transition.visualTargetGripBlend - 1.0f) * phase;
        }
    }
    else
    {
        if (transition.progress < kHeldItemAttachmentEdgeProgress)
        {
            const float phase = SmoothStep(transition.progress * 2.0f);
            item.visualStowBlend = transition.visualStartStowBlend;
            item.visualGripBlend = transition.visualStartGripBlend +
                (1.0f - transition.visualStartGripBlend) * phase;
        }
        else
        {
            const float phase = SmoothStep(
                (transition.progress - kHeldItemAttachmentEdgeProgress) * 2.0f);
            item.visualStowBlend = transition.visualStartStowBlend +
                (transition.visualTargetStowBlend -
                 transition.visualStartStowBlend) * phase;
            item.visualGripBlend = 1.0f;
        }
    }
    transition.lastTransitionTick = tick;
    transition.lastAdvancedTick = tick;
    transition.hasAdvancedTick = true;
    const bool attachedNow = !transition.visualOnly && !transition.attachmentApplied &&
        previousProgress < kHeldItemAttachmentEdgeProgress &&
        transition.progress >= kHeldItemAttachmentEdgeProgress;
    if (attachedNow)
    {
        // The semantic parent changes on this fixed tick. Keep the item and
        // hand exactly coincident at that boundary; easing into the second
        // half by even a fraction of a tick would release the Grip before a
        // draw's HandSocket attachment (or after a sheath's BodyStow edge).
        item.visualStowBlend = 1.0f;
        item.visualGripBlend = 1.0f;
        transition.attachmentApplied = true;
        transition.attachmentEdgeTick = tick;
        item.parentMode = transition.targetParent;
    }

    HeldItemTransitionAdvanceStatus status = HeldItemTransitionAdvanceStatus::Advanced;
    if (transition.elapsedSeconds >= transition.durationSeconds)
    {
        item.visualStowBlend = transition.visualTargetStowBlend;
        item.visualGripBlend = transition.visualTargetGripBlend;
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
