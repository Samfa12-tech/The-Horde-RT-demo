#pragma once

#include <cstdint>

namespace horde::vulkan::raytracing
{
enum class RtSceneProfile : std::uint32_t
{
    Showcase = 0u,
    GraphicsPreview = 1u,
    EntryMenu = 2u,
};
}
