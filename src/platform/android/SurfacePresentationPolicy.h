#pragma once

#include <cstdint>
#include "graphics/RtPresentationTransform.h"

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
    horde::graphics::RtPresentationTransform rtTransform =
        horde::graphics::RtPresentationTransform::Identity;
    bool transformSupported = false;
};

constexpr SurfacePresentationPolicy ChooseSurfacePresentationPolicy(
    const std::uint32_t supportedTransforms,
    const std::uint32_t currentTransform) noexcept
{
    const bool identitySupported = (supportedTransforms & kSurfaceTransformIdentity) != 0u;
    // Android can report SUBOPTIMAL forever when preTransform differs from
    // currentTransform, even if identity is supported. Match the surface and
    // pre-rotate primary rays instead of repeatedly rebuilding that same tuple.
    std::uint32_t index = 0u;
    std::uint32_t flag = kSurfaceTransformIdentity;
    while (flag < currentTransform && index < 7u) { flag <<= 1u; ++index; }
    const bool supported = flag == currentTransform &&
        (supportedTransforms & currentTransform) != 0u;
    return {currentTransform, identitySupported, currentTransform != kSurfaceTransformIdentity,
            horde::graphics::RtPresentationTransformFromIndex(supported ? index : 0u), supported};
}
} // namespace horde::platform::android
