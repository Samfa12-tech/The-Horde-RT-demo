#pragma once

#include "gameplay/items/HeldItemState.h"
#include "gameplay/simulation/GameplayEvent.h"

#include <cstdint>

namespace horde::gameplay
{
enum class EquipmentAudioCue : std::uint8_t
{
    None = 0,
    SwordDraw = 1,
    SwordSheath = 2,
};

// The shared simulation emits this edge once when attachment changes. Input
// and draw-start events do not play a second cue; resets emit no attachment edge.
[[nodiscard]] constexpr EquipmentAudioCue EquipmentCueForEvent(
    const simulation::GameplayEvent& event) noexcept
{
    if (event.type != simulation::GameplayEventType::PlayerSwordAttachmentChanged ||
        event.source != simulation::EntityId::Player)
        return EquipmentAudioCue::None;
    if (event.payload == static_cast<std::int32_t>(items::HeldItemParentMode::HandSocket))
        return EquipmentAudioCue::SwordDraw;
    if (event.payload == static_cast<std::int32_t>(items::HeldItemParentMode::BodyStow))
        return EquipmentAudioCue::SwordSheath;
    return EquipmentAudioCue::None;
}
} // namespace horde::gameplay
