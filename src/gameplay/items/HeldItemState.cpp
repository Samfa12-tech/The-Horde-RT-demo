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

HeldItemState MakeHeldItemState(const HeldItemId id, const HeldHand hand)
{
    HeldItemState result;
    result.id = id;
    result.hand = hand;
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
    if (!item.detached && item.parentMode == HeldItemParentMode::HandSocket &&
        parentMode == HeldItemParentMode::AuthoredWorldTrajectory)
    {
        item.detached = true;
        item.detachTick = tick;
        item.worldFromDetach = item.worldFromItem;
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
