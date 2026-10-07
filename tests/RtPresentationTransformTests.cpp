#include "graphics/RtPresentationTransform.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace
{
using horde::graphics::RtPresentationTransform;
using horde::graphics::RtPresentationUv;

void Require(const bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

RtPresentationUv ForwardVulkanTransform(RtPresentationUv uv, const std::uint32_t index)
{
    // Vulkan mirror-rotate transforms mirror horizontally first, then rotate
    // clockwise. These are source-image to presented-image coordinates.
    if (index >= 4u)
        uv.x = 1.0f - uv.x;
    switch (index & 3u)
    {
    case 1u: return {1.0f - uv.y, uv.x};
    case 2u: return {1.0f - uv.x, 1.0f - uv.y};
    case 3u: return {uv.y, 1.0f - uv.x};
    default: return uv;
    }
}

bool Near(const float lhs, const float rhs)
{
    return std::abs(lhs - rhs) <= 0.00001f;
}
}

int main()
{
    using namespace horde::graphics;

    constexpr std::array<RtPresentationUv, 5u> samples{{
        {0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {0.23f, 0.61f}}};
    for (std::uint32_t index = 0u; index < 8u; ++index)
    {
        const auto transform = RtPresentationTransformFromIndex(index);
        Require(RtPresentationTransformIndex(transform) == index,
                "all eight Vulkan transform indices must round-trip");
        for (const RtPresentationUv sourceUv : samples)
        {
            const RtPresentationUv imageUv = ForwardVulkanTransform(sourceUv, index);
            const RtPresentationUv recoveredViewUv = RtViewUvFromImageUv(imageUv, transform);
            Require(Near(recoveredViewUv.x, sourceUv.x) && Near(recoveredViewUv.y, sourceUv.y),
                    "inverse view mapping must undo the Vulkan mirror-then-clockwise transform");
        }
    }

    Require(RtPresentationTransformFromIndex(8u) == RtPresentationTransform::Identity &&
                RtPresentationTransformIndex(static_cast<RtPresentationTransform>(99u)) == 0u,
            "invalid transform indices must fall back to identity");
    Require(EncodeRtPresentationOutputMode(RtPresentationTransform::Identity, false) == 0u &&
                EncodeRtPresentationOutputMode(RtPresentationTransform::Identity, true) == 1u,
            "identity packing must preserve the existing red/blue swap values");
    for (std::uint32_t index = 0u; index < 8u; ++index)
    {
        for (std::uint32_t bgra = 0u; bgra < 2u; ++bgra)
        {
            const auto transform = RtPresentationTransformFromIndex(index);
            const std::uint32_t packed = EncodeRtPresentationOutputMode(transform, bgra != 0u);
            Require(DecodeRtPresentationTransform(packed) == transform &&
                        DecodeRtPresentationRedBlueSwap(packed) == (bgra != 0u),
                    "packed transform and channel-swap fields must decode independently");
        }
    }

    Require(!RtPresentationTransformSwapsAxes(RtPresentationTransform::Rotate180) &&
                RtPresentationTransformSwapsAxes(RtPresentationTransform::Rotate90) &&
                RtPresentationTransformSwapsAxes(RtPresentationTransform::MirrorRotate270),
            "only quarter-turn transforms must swap axes");
    Require(Near(RtViewAspectFromImageExtent(1440u, 2981u, RtPresentationTransform::Identity),
                 1440.0f / 2981.0f) &&
                Near(RtViewAspectFromImageExtent(1440u, 2981u, RtPresentationTransform::Rotate90),
                     2981.0f / 1440.0f) &&
                Near(RtViewAspectFromImageExtent(1440u, 2981u, RtPresentationTransform::MirrorRotate270),
                     2981.0f / 1440.0f),
            "quarter-turn aspect must use the exchanged logical view dimensions");
    return 0;
}
