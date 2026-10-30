#pragma once

#include "gameplay/simulation/DevelopmentSupportFixture.h"
#include "gameplay/simulation/GameplayEvent.h"

#include <array>
#include <cstdint>

namespace horde::gameplay
{
// Type-scoped semantic cue IDs also fit Android's existing event cue byte.
enum class DryFootstepCue : std::uint8_t
{
    None = 0, DefaultFirst = 1, DefaultSecond = 2, EarthFirst = 3, EarthSecond = 4
};

struct DryFootstepContact
{
    simulation::SupportSurface surface = simulation::SupportSurface::Stone;
    unsigned variant = 0;
};

[[nodiscard]] constexpr simulation::SupportSurface DryFootstepSurface(
    simulation::SupportSurface surface) noexcept
{
    using simulation::SupportSurface;
    return surface == SupportSurface::Earth || surface == SupportSurface::Wood
        ? surface : SupportSurface::Stone;
}

// Payload zero retains the historical default. No event ID or queue layout changes.
[[nodiscard]] constexpr std::int32_t EncodeDryFootstepContact(
    simulation::SupportSurface surface, unsigned variant) noexcept
{
    return static_cast<std::int32_t>(DryFootstepSurface(surface)) * 2 +
        static_cast<std::int32_t>(variant & 1u);
}

[[nodiscard]] constexpr DryFootstepContact DecodeDryFootstepContact(std::int32_t payload) noexcept
{
    if (payload < 0 || payload > 5) return {};
    return {static_cast<simulation::SupportSurface>(payload / 2), static_cast<unsigned>(payload & 1)};
}

class DryFootstepSequence
{
public:
    [[nodiscard]] std::int32_t Next(simulation::SupportSurface surface) noexcept
    {
        surface = DryFootstepSurface(surface);
        // Wood currently shares the accepted generic pair. Alternate by audible
        // family so crossing Stone/Wood cannot repeat that pair's last sample.
        auto& variant = next_[surface == simulation::SupportSurface::Earth ? 1u : 0u];
        const auto payload = EncodeDryFootstepContact(surface, variant);
        variant ^= 1u;
        return payload;
    }
    void Reset() noexcept { next_ = {}; }
private:
    std::array<unsigned, 2> next_{};
};

[[nodiscard]] constexpr DryFootstepCue DryFootstepCueForEvent(
    const simulation::GameplayEvent& event) noexcept
{
    if (event.type != simulation::GameplayEventType::PlayerFootstep ||
        event.source != simulation::EntityId::Player) return DryFootstepCue::None;
    const auto contact = DecodeDryFootstepContact(event.payload);
    if (contact.surface == simulation::SupportSurface::Earth)
        return contact.variant ? DryFootstepCue::EarthSecond : DryFootstepCue::EarthFirst;
    return contact.variant ? DryFootstepCue::DefaultSecond : DryFootstepCue::DefaultFirst;
}

[[nodiscard]] constexpr const char* DryFootstepClip(DryFootstepCue cue) noexcept
{
    switch (cue)
    {
    case DryFootstepCue::DefaultFirst: return "player_step_1.wav";
    case DryFootstepCue::DefaultSecond: return "player_step_2.wav";
    // Already admitted FilmCow dirt 17/18. Reuse, no extra asset or sound layer.
    case DryFootstepCue::EarthFirst: return "skeleton_step_1.wav";
    case DryFootstepCue::EarthSecond: return "skeleton_step_2.wav";
    default: return nullptr;
    }
}
} // namespace horde::gameplay
