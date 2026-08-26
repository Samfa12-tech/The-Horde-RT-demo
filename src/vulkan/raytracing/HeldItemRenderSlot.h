#pragma once

#include "gameplay/items/HeldItemState.h"

#include <array>

namespace horde::vulkan::raytracing
{

class HeldItemRenderSlot
{
public:
    static std::array<float, 12u> BuildInstanceTransform(
        const horde::gameplay::items::HeldItemState& item);
};

} // namespace horde::vulkan::raytracing
