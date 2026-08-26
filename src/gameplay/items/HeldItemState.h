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
};

using HeldItemStates = std::array<HeldItemState, 2u>;

HeldItemTransform IdentityHeldItemTransform();
HeldItemTransform OriginalTorchGripSocketTransform();
HeldItemTransform OriginalTorchFlameSocketTransform();
HeldItemTransform OriginalTorchLightSocketTransform();
HeldItemTransform SwordGripSocketTransform();
HeldItemState MakeHeldItemState(HeldItemId id, HeldHand hand);
HeldItemStates MakeDefaultHeldItemStates();
void UpdateHeldItemParent(HeldItemState& item,
                          HeldItemParentMode parentMode,
                          std::uint64_t tick,
                          const HeldItemTransform& resolvedWorldFromItem);
void ImportHeldItemCheckpoint(HeldItemStates& items,
                              bool torchHeldByPlayer,
                              std::uint64_t tick);
void ResetHeldItemStates(HeldItemStates& items);

} // namespace horde::gameplay::items
