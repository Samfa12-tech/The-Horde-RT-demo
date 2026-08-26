#include "gameplay/items/HeldItemState.h"

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

HeldItemState MakeHeldItemState(const HeldItemId id, const HeldHand hand)
{
    HeldItemState result;
    result.id = id;
    result.hand = hand;
    result.worldFromItem = IdentityHeldItemTransform();
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
    if (!item.detached && item.parentMode == HeldItemParentMode::HandSocket &&
        parentMode == HeldItemParentMode::AuthoredWorldTrajectory)
    {
        item.detached = true;
        item.detachTick = tick;
    }
    item.parentMode = parentMode;
    item.worldFromItem = resolvedWorldFromItem;
}

void ImportHeldItemCheckpoint(HeldItemStates& items,
                              const bool torchHeldByPlayer,
                              const std::uint64_t tick)
{
    ResetHeldItemStates(items);
    if (!torchHeldByPlayer)
    {
        items[0].parentMode = HeldItemParentMode::AuthoredWorldTrajectory;
        items[0].detached = true;
        items[0].detachTick = tick;
    }
}

void ResetHeldItemStates(HeldItemStates& items)
{
    items = MakeDefaultHeldItemStates();
}

} // namespace horde::gameplay::items
