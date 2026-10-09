#pragma once

#include "gameplay/EquipmentFeedback.h"

#include <cstdint>

namespace horde::platform::android
{
// Existing type/source/target/sequence fields retain their positions. The
// previously unused byte 24 carries a selected semantic cue, not truncated
// arbitrary event payload. The second transport word still owns stereo gains.
[[nodiscard]] constexpr std::uint64_t PackGameplayEventMetadata(
    const gameplay::simulation::GameplayEvent& event) noexcept
{
    return static_cast<std::uint64_t>(event.type) |
        (static_cast<std::uint64_t>(event.source) << 8u) |
        (static_cast<std::uint64_t>(event.target) << 16u) |
        (static_cast<std::uint64_t>(gameplay::EquipmentCueForEvent(event)) << 24u) |
        ((event.sequence & 0xffffffffu) << 32u);
}
} // namespace horde::platform::android
