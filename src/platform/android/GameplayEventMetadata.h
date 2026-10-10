#pragma once

#include "gameplay/EquipmentFeedback.h"

#include <bit>
#include <cstdint>

namespace horde::platform::android
{
static_assert(static_cast<std::uint8_t>(gameplay::simulation::GameplayEventType::SkeletonEncounterWarning) == 23u);
static_assert(static_cast<std::uint8_t>(gameplay::simulation::GameplayEventType::ParryPrepareCue) == 24u);
static_assert(static_cast<std::uint8_t>(gameplay::simulation::GameplayEventType::LichDischargeWarning) == 25u);

// Existing type/source/target/sequence fields retain their positions. The
// previously unused byte 24 carries a selected semantic cue, not truncated
// arbitrary event payload. Stereo gains and vertical metadata have their own
// words in the compact event tuple.
[[nodiscard]] constexpr std::uint64_t PackGameplayEventMetadata(
    const gameplay::simulation::GameplayEvent& event) noexcept
{
    return static_cast<std::uint64_t>(event.type) |
        (static_cast<std::uint64_t>(event.source) << 8u) |
        (static_cast<std::uint64_t>(event.target) << 16u) |
        (static_cast<std::uint64_t>(gameplay::EquipmentCueForEvent(event)) << 24u) |
        ((event.sequence & 0xffffffffu) << 32u);
}

// Preserve the event's true source and listener heights without changing the
// existing ID/sequence word or the planar stereo-gain calculation.
[[nodiscard]] constexpr std::uint64_t PackGameplayEventVerticalMetadata(
    const gameplay::simulation::GameplayEvent& event) noexcept
{
    return static_cast<std::uint64_t>(std::bit_cast<std::uint32_t>(event.worldY)) |
        (static_cast<std::uint64_t>(std::bit_cast<std::uint32_t>(event.listenerY)) << 32u);
}
} // namespace horde::platform::android
