#pragma once

#include "gameplay/ShowcaseRoute.h"
#include "gameplay/items/HeldItemState.h"

#include <array>
#include <cstdint>

namespace horde::gameplay::effects
{

struct KeeperTorchAnchor
{
    std::uint32_t stableId;
    std::uint32_t seed;
    std::array<float, 3u> position;
};

// Upright OriginalTorch item origins behind the live Keeper stage. Geometry,
// flame and direct light share this item transform and the admitted item's
// existing Flame/Light socket transforms; these are not camera-relative lights.
// ID 1 remains the opening torch; the reward keeps its existing renderer ID.
inline constexpr std::array<KeeperTorchAnchor, 2u> kKeeperTorchAnchors{{
    {3u, 0x4b545231u, {{kKeeperStagingPosition.x - 0.80f, 0.255f,
                       kKeeperStagingPosition.z - 1.15f}}},
    {4u, 0x4b545232u, {{kKeeperStagingPosition.x - 0.80f, 0.255f,
                       kKeeperStagingPosition.z + 1.15f}}},
}};

inline items::HeldItemTransform KeeperTorchWorldFromItem(const KeeperTorchAnchor& anchor)
{
    auto transform = items::IdentityHeldItemTransform();
    transform[12] = anchor.position[0];
    transform[13] = anchor.position[1];
    transform[14] = anchor.position[2];
    return transform;
}

} // namespace horde::gameplay::effects
