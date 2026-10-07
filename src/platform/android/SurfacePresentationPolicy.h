#pragma once

#include <cstdint>

namespace horde::platform::android
{
// Vulkan VkSurfaceTransformFlagBitsKHR values. Kept as plain integers so the
// selection policy can be exercised by host tests without an Android/Vulkan SDK.
inline constexpr std::uint32_t kSurfaceTransformIdentity = 0x00000001u;

struct SurfacePresentationPolicy
{
    std::uint32_t chosenPreTransform = kSurfaceTransformIdentity;
    bool identitySupported = false;
    bool rtPreRotationRequired = false;
};

constexpr SurfacePresentationPolicy ChooseSurfacePresentationPolicy(
    const std::uint32_t supportedTransforms,
    const std::uint32_t currentTransform) noexcept
{
    const bool identitySupported = (supportedTransforms & kSurfaceTransformIdentity) != 0u;
    const std::uint32_t chosen = identitySupported ? kSurfaceTransformIdentity : currentTransform;
    return {chosen, identitySupported, chosen != kSurfaceTransformIdentity};
}
} // namespace horde::platform::android
