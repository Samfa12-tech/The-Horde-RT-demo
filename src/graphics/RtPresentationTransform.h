#pragma once

#include <cstdint>

namespace horde::graphics
{
enum class RtPresentationTransform : std::uint32_t
{
    Identity = 0u,
    Rotate90 = 1u,
    Rotate180 = 2u,
    Rotate270 = 3u,
    HorizontalMirror = 4u,
    MirrorRotate90 = 5u,
    MirrorRotate180 = 6u,
    MirrorRotate270 = 7u
};

struct RtPresentationUv
{
    float x = 0.0f;
    float y = 0.0f;
};

constexpr RtPresentationTransform RtPresentationTransformFromIndex(const std::uint32_t index) noexcept
{
    return index <= static_cast<std::uint32_t>(RtPresentationTransform::MirrorRotate270)
        ? static_cast<RtPresentationTransform>(index)
        : RtPresentationTransform::Identity;
}

constexpr std::uint32_t RtPresentationTransformIndex(const RtPresentationTransform transform) noexcept
{
    const auto index = static_cast<std::uint32_t>(transform);
    return index <= static_cast<std::uint32_t>(RtPresentationTransform::MirrorRotate270) ? index : 0u;
}

constexpr bool RtPresentationTransformSwapsAxes(const RtPresentationTransform transform) noexcept
{
    const auto index = RtPresentationTransformIndex(transform);
    return index == 1u || index == 3u || index == 5u || index == 7u;
}

// Pack transform index and the existing red/blue swap flag into an exactly
// representable integer-valued float. Identity therefore retains its old 0/1 values.
constexpr std::uint32_t EncodeRtPresentationOutputMode(
    const RtPresentationTransform transform,
    const bool redBlueSwap) noexcept
{
    return RtPresentationTransformIndex(transform) * 2u + (redBlueSwap ? 1u : 0u);
}

constexpr bool DecodeRtPresentationRedBlueSwap(const std::uint32_t packedMode) noexcept
{
    return (packedMode & 1u) != 0u;
}

constexpr RtPresentationTransform DecodeRtPresentationTransform(const std::uint32_t packedMode) noexcept
{
    return RtPresentationTransformFromIndex(packedMode / 2u);
}

// Convert an output image coordinate to the corresponding upright view
// coordinate. Vulkan applies clockwise rotation after any horizontal mirror.
constexpr RtPresentationUv RtViewUvFromImageUv(
    const RtPresentationUv imageUv,
    const RtPresentationTransform transform) noexcept
{
    RtPresentationUv viewUv = imageUv;
    switch (RtPresentationTransformIndex(transform))
    {
    case 1u: viewUv = {imageUv.y, 1.0f - imageUv.x}; break;
    case 2u: viewUv = {1.0f - imageUv.x, 1.0f - imageUv.y}; break;
    case 3u: viewUv = {1.0f - imageUv.y, imageUv.x}; break;
    case 5u: viewUv = {imageUv.y, 1.0f - imageUv.x}; break;
    case 6u: viewUv = {1.0f - imageUv.x, 1.0f - imageUv.y}; break;
    case 7u: viewUv = {1.0f - imageUv.y, imageUv.x}; break;
    default: break;
    }
    if (RtPresentationTransformIndex(transform) >= 4u)
        viewUv.x = 1.0f - viewUv.x;
    return viewUv;
}

// Image extent is in the pre-rotated storage-image coordinate system. For a
// quarter turn, its logical view width and height are exchanged.
constexpr float RtViewAspectFromImageExtent(
    const std::uint32_t imageWidth,
    const std::uint32_t imageHeight,
    const RtPresentationTransform transform) noexcept
{
    const std::uint32_t safeWidth = imageWidth == 0u ? 1u : imageWidth;
    const std::uint32_t safeHeight = imageHeight == 0u ? 1u : imageHeight;
    return RtPresentationTransformSwapsAxes(transform)
        ? static_cast<float>(safeHeight) / static_cast<float>(safeWidth)
        : static_cast<float>(safeWidth) / static_cast<float>(safeHeight);
}
} // namespace horde::graphics
