#include "vulkan/raytracing/HeldItemRenderSlot.h"

namespace horde::vulkan::raytracing
{

std::array<float, 12u> HeldItemRenderSlot::BuildInstanceTransform(
    const horde::gameplay::items::HeldItemState& item)
{
    const auto& matrix = item.worldFromItem;
    return {{
        matrix[0], matrix[4], matrix[8], matrix[12],
        matrix[1], matrix[5], matrix[9], matrix[13],
        matrix[2], matrix[6], matrix[10], matrix[14]}};
}

} // namespace horde::vulkan::raytracing
