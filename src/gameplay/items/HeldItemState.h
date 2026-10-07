#pragma once

#include <array>
#include <cstdint>

namespace horde::gameplay::items
{

using HeldItemTransform = std::array<float, 16u>;

enum class HeldItemId : std::uint8_t
{
    OriginalTorch,
    Sword,
    RewardLantern,
};

enum class HeldHand : std::uint8_t
{
    LeftHand,
    RightHand,
};

enum class HeldItemParentMode : std::uint8_t
{
    HandSocket,
    AuthoredWorldTrajectory,
    WorldObject,
    BodyStow,
};

enum class HeldItemTransitionKind : std::uint8_t
{
    None,
    Draw,
    Sheath,
    Stow,
    Restore,
};

// These are initial authored transition timings. They are state-machine
// choices, not measurements of an imported animation clip.
inline constexpr float kHeldItemDrawDurationSeconds = 0.40f;
inline constexpr float kHeldItemSheathDurationSeconds = 0.36f;
inline constexpr float kHeldItemStowDurationSeconds = 0.30f;
inline constexpr float kHeldItemRestoreDurationSeconds = 0.36f;
inline constexpr float kHeldItemAttachmentEdgeProgress = 0.5f;

struct HeldItemTransitionState
{
    HeldItemTransitionKind kind = HeldItemTransitionKind::None;
    HeldItemParentMode sourceParent = HeldItemParentMode::HandSocket;
    HeldItemParentMode targetParent = HeldItemParentMode::HandSocket;
    float elapsedSeconds = 0.0f;
    float durationSeconds = 0.0f;
    float progress = 0.0f;
    std::uint64_t startedTick = 0u;
    std::uint64_t lastTransitionTick = 0u;
    std::uint64_t lastAdvancedTick = 0u;
    std::uint64_t attachmentEdgeTick = 0u;
    std::uint64_t interruptedTick = 0u;
    std::uint64_t semanticEdgeSequence = 0u;
    bool active = false;
    bool hasAdvancedTick = false;
    bool attachmentApplied = false;
};

struct HeldItemState
{
    HeldItemId id = HeldItemId::OriginalTorch;
    HeldHand hand = HeldHand::LeftHand;
    HeldItemParentMode parentMode = HeldItemParentMode::HandSocket;
    HeldItemTransform worldFromItem{};
    HeldItemTransform worldFromDetach{};
    std::uint64_t detachTick = 0u;
    bool detached = false;
    HeldItemTransitionState transition{};
};

using HeldItemStates = std::array<HeldItemState, 2u>;

HeldItemTransform IdentityHeldItemTransform();
HeldItemTransform OriginalTorchGripSocketTransform();
HeldItemTransform OriginalTorchFlameSocketTransform();
HeldItemTransform OriginalTorchLightSocketTransform();
HeldItemTransform SwordGripSocketTransform();
HeldItemState MakeHeldItemState(
    HeldItemId id,
    HeldHand hand,
    HeldItemParentMode parentMode = HeldItemParentMode::HandSocket);
HeldItemStates MakeDefaultHeldItemStates();
void UpdateHeldItemParent(HeldItemState& item,
                          HeldItemParentMode parentMode,
                          std::uint64_t tick,
                          const HeldItemTransform& resolvedWorldFromItem);

enum class HeldItemTransitionRequestStatus : std::uint8_t
{
    Started,
    AlreadyInProgress,
    AlreadyAtTarget,
    Interrupted,
    InterruptedAndStarted,
    RejectedWorldOwned,
    RejectedInvalidState,
    RejectedSequenceExhausted,
};

struct HeldItemTransitionRequestResult
{
    HeldItemTransitionRequestStatus status =
        HeldItemTransitionRequestStatus::RejectedInvalidState;
    std::uint64_t semanticEdgeSequence = 0u;
};

enum class HeldItemTransitionAdvanceStatus : std::uint8_t
{
    Idle,
    Paused,
    Advanced,
    AttachmentChanged,
    Completed,
    RejectedInvalidState,
};

struct HeldItemTransitionAdvanceResult
{
    HeldItemTransitionAdvanceStatus status =
        HeldItemTransitionAdvanceStatus::Idle;
    std::uint64_t semanticEdgeSequence = 0u;
    std::uint64_t attachmentEdgeTick = 0u;
    bool attachmentChanged = false;
};

// The entire held-item state is snapshot-safe POD. A transition request only
// operates between HandSocket and BodyStow; world-owned torch states remain
// under the authored world-trajectory path.
bool ValidateHeldItemState(const HeldItemState& item);
HeldItemTransitionRequestResult RequestHeldItemTransition(
    HeldItemState& item,
    HeldItemTransitionKind kind,
    std::uint64_t tick);
bool InterruptHeldItemTransition(HeldItemState& item, std::uint64_t tick);
HeldItemTransitionAdvanceResult AdvanceHeldItemTransition(
    HeldItemState& item,
    std::uint64_t tick,
    float fixedDeltaSeconds,
    bool paused = false);

void ImportHeldItemCheckpoint(HeldItemStates& items,
                              bool torchHeldByPlayer,
                              std::uint64_t tick);
void ResetHeldItemStates(HeldItemStates& items);

} // namespace horde::gameplay::items
