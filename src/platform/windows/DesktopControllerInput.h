#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace horde::platform::windows
{

enum class LegacyAxis
{
    None,
    Z,
    R,
    U,
    V,
};

struct LegacyAxisSample
{
    float z = 0.0f;
    float r = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    bool hasZ = false;
    bool hasR = false;
    bool hasU = false;
    bool hasV = false;
};

struct LegacyRightStickAxes
{
    LegacyAxis horizontal = LegacyAxis::None;
    LegacyAxis vertical = LegacyAxis::None;
};

struct LegacyControllerIdentity
{
    std::uint16_t vendorId = 0u;
    std::uint16_t productId = 0u;
    std::string_view productName;
};

struct ControllerActionEdges
{
    bool attackPressed = false;
    bool parryPressed = false;
    bool dodgePressed = false;

    bool Any() const { return attackPressed || parryPressed || dodgePressed; }
};

struct ControllerTriggerLatch
{
    bool leftHeld = false;
    bool rightHeld = false;
};

struct ControllerView
{
    float yawRadians = 0.0f;
    float pitchRadians = 0.0f;
};

struct ControllerMenuEdges
{
    bool previous = false;
    bool next = false;
    bool decrease = false;
    bool increase = false;
    bool confirm = false;
    bool cancel = false;
    bool togglePause = false;

    bool Any() const
    {
        return previous || next || decrease || increase || confirm || cancel || togglePause;
    }
};

inline bool LegacyAxisAvailable(const LegacyAxisSample& sample, const LegacyAxis axis)
{
    switch (axis)
    {
    case LegacyAxis::Z: return sample.hasZ;
    case LegacyAxis::R: return sample.hasR;
    case LegacyAxis::U: return sample.hasU;
    case LegacyAxis::V: return sample.hasV;
    case LegacyAxis::None: break;
    }
    return false;
}

inline float LegacyAxisValue(const LegacyAxisSample& sample, const LegacyAxis axis)
{
    switch (axis)
    {
    case LegacyAxis::Z: return sample.z;
    case LegacyAxis::R: return sample.r;
    case LegacyAxis::U: return sample.u;
    case LegacyAxis::V: return sample.v;
    case LegacyAxis::None: break;
    }
    return 0.0f;
}

inline bool ContainsAsciiCaseInsensitive(const std::string_view text, const std::string_view needle)
{
    if (needle.empty() || needle.size() > text.size())
    {
        return needle.empty();
    }
    const auto asciiLower = [](const char value)
    {
        return value >= 'A' && value <= 'Z' ? static_cast<char>(value + ('a' - 'A')) : value;
    };
    for (std::size_t offset = 0; offset + needle.size() <= text.size(); ++offset)
    {
        bool matches = true;
        for (std::size_t index = 0; index < needle.size(); ++index)
        {
            if (asciiLower(text[offset + index]) != asciiLower(needle[index]))
            {
                matches = false;
                break;
            }
        }
        if (matches)
        {
            return true;
        }
    }
    return false;
}

inline LegacyRightStickAxes SelectLegacyRightStickAxes(
    const LegacyAxisSample& sample,
    const std::string_view productName)
{
    // Backbone's Windows HID report maps right X/Y to WinMM's R/U fields
    // while still advertising trigger axes in Z/V. Handle that known
    // layout explicitly, then infer other HID layouts from their resting axes.
    if (ContainsAsciiCaseInsensitive(productName, "backbone") && sample.hasR && sample.hasU)
    {
        return {LegacyAxis::R, LegacyAxis::U};
    }

    constexpr std::array<LegacyRightStickAxes, 6> candidates{{
        {LegacyAxis::Z, LegacyAxis::R},
        {LegacyAxis::U, LegacyAxis::V},
        {LegacyAxis::R, LegacyAxis::U},
        {LegacyAxis::Z, LegacyAxis::V},
        {LegacyAxis::Z, LegacyAxis::U},
        {LegacyAxis::R, LegacyAxis::V},
    }};

    LegacyRightStickAxes selected{};
    float selectedScore = 1000.0f;
    for (const LegacyRightStickAxes candidate : candidates)
    {
        if (!LegacyAxisAvailable(sample, candidate.horizontal) ||
            !LegacyAxisAvailable(sample, candidate.vertical))
        {
            continue;
        }
        const float horizontalDistance = std::abs(LegacyAxisValue(sample, candidate.horizontal));
        const float verticalDistance = std::abs(LegacyAxisValue(sample, candidate.vertical));
        // Stick axes rest near the midpoint. Separate trigger axes generally
        // rest at an extreme, so weight the farther component most heavily.
        const float score = std::max(horizontalDistance, verticalDistance) * 4.0f +
                            horizontalDistance + verticalDistance;
        if (score < selectedScore)
        {
            selected = candidate;
            selectedScore = score;
        }
    }
    return selected;
}

inline bool IsCapturedBackboneOne(const LegacyControllerIdentity& identity)
{
    return identity.vendorId == 0x358au && identity.productId == 0x0204u;
}

inline LegacyRightStickAxes SelectLegacyRightStickAxes(
    const LegacyAxisSample& sample,
    const LegacyControllerIdentity& identity)
{
    // Owner-captured on the exact Windows USB device. WinMM exposes this
    // Backbone One as the generic Microsoft PC-joystick driver with four axes:
    // X/Y for movement and Z/R for right-stick look. U/V are not present.
    if (IsCapturedBackboneOne(identity) && sample.hasZ && sample.hasR)
    {
        return {LegacyAxis::Z, LegacyAxis::R};
    }
    return SelectLegacyRightStickAxes(sample, identity.productName);
}

inline ControllerActionEdges MapLegacyControllerEdges(
    const std::uint32_t currentButtons,
    const std::uint32_t previousButtons,
    const LegacyControllerIdentity& identity)
{
    const std::uint32_t pressed = currentButtons & ~previousButtons;
    if (IsCapturedBackboneOne(identity))
    {
        // Exact owner telemetry, 2026-08-23: RT=0x200, LT=0x100,
        // B/Circle=0x2. These are independent digital button fields.
        return {
            .attackPressed = (pressed & 0x200u) != 0u,
            .parryPressed = (pressed & 0x100u) != 0u,
            .dodgePressed = (pressed & 0x002u) != 0u,
        };
    }
    return {
        .attackPressed = (pressed & 0x001u) != 0u,
        .parryPressed = false,
        .dodgePressed = (pressed & 0x002u) != 0u,
    };
}

inline ControllerView ApplyControllerLook(
    const float yawRadians,
    const float pitchRadians,
    const float horizontal,
    const float vertical,
    const float deltaSeconds)
{
    const float safeDelta = std::clamp(deltaSeconds, 0.0f, 0.1f);
    return {
        .yawRadians = yawRadians + std::clamp(horizontal, -1.0f, 1.0f) * safeDelta * 2.5f,
        .pitchRadians = std::clamp(
            pitchRadians - std::clamp(vertical, -1.0f, 1.0f) * safeDelta * 1.8f,
            -0.32f,
            0.28f),
    };
}

inline int StepControllerSlider(const int percentage, const bool increase)
{
    constexpr int minimum = 50;
    constexpr int maximum = 100;
    constexpr int step = 5;
    return std::clamp(percentage + (increase ? step : -step), minimum, maximum);
}

inline ControllerMenuEdges MapLegacyControllerMenuEdges(
    const std::uint32_t currentButtons,
    const std::uint32_t previousButtons,
    const std::uint32_t currentPov,
    const std::uint32_t previousPov,
    const LegacyControllerIdentity& identity)
{
    const std::uint32_t pressed = currentButtons & ~previousButtons;
    const bool povPressed = currentPov != 65535u && currentPov != previousPov;
    const bool exactBackbone = IsCapturedBackboneOne(identity);
    // Explicit owner pause test on VID 358A/PID 0204: the Backbone menu/start
    // button is WinMM button 12 (0x0800). The earlier 0x2000 edge came from a
    // different centre control during a broad button sweep.
    const std::uint32_t menuMask = exactBackbone ? 0x0800u : 0x080u;
    ControllerMenuEdges edges{
        .confirm = (pressed & 0x001u) != 0u,
        .cancel = (pressed & 0x002u) != 0u,
        .togglePause = (pressed & menuMask) != 0u,
    };
    if (povPressed)
    {
        const std::uint32_t angle = currentPov % 36000u;
        edges.previous = angle < 4500u || angle >= 31500u;
        edges.increase = angle >= 4500u && angle < 13500u;
        edges.next = angle >= 13500u && angle < 22500u;
        edges.decrease = angle >= 22500u && angle < 31500u;
    }
    return edges;
}

inline ControllerActionEdges UpdateXInputTriggerEdges(
    const std::uint8_t leftTrigger,
    const std::uint8_t rightTrigger,
    ControllerTriggerLatch& latch)
{
    constexpr std::uint8_t pressThreshold = 30u;
    constexpr std::uint8_t releaseThreshold = 22u;
    const bool leftNow = latch.leftHeld
        ? leftTrigger > releaseThreshold
        : leftTrigger > pressThreshold;
    const bool rightNow = latch.rightHeld
        ? rightTrigger > releaseThreshold
        : rightTrigger > pressThreshold;
    const ControllerActionEdges edges{
        .attackPressed = rightNow && !latch.rightHeld,
        .parryPressed = leftNow && !latch.leftHeld,
        .dodgePressed = false,
    };
    latch.leftHeld = leftNow;
    latch.rightHeld = rightNow;
    return edges;
}

} // namespace horde::platform::windows
