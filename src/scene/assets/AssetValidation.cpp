#include "scene/assets/AssetValidation.h"

#include "third_party/cgltf/cgltf.h"

#include <array>
#include <cmath>
#include <cstring>
#include <string_view>

namespace horde::scene::assets
{

namespace
{

std::string NodeName(const cgltf_node& node)
{
    return node.name != nullptr && node.name[0] != '\0' ? node.name : "<unnamed>";
}

bool AllFinite(const float* values, std::size_t count)
{
    for (std::size_t i = 0u; i < count; ++i)
    {
        if (!std::isfinite(values[i])) return false;
    }
    return true;
}

} // namespace

bool IsSupportedRequiredExtension(const char* name)
{
    if (name == nullptr) return false;
    constexpr std::array<std::string_view, 4u> supported{{
        "KHR_materials_transmission",
        "KHR_materials_ior",
        "KHR_materials_volume",
        "KHR_materials_emissive_strength"}};
    for (const std::string_view extension : supported)
    {
        if (extension == name) return true;
    }
    return false;
}

bool ValidateAccessorRange(const cgltf_accessor& accessor,
                           std::size_t accessorIndex,
                           std::string& diagnostic)
{
    if (accessor.buffer_view == nullptr || accessor.buffer_view->buffer == nullptr ||
        accessor.buffer_view->buffer->data == nullptr)
    {
        diagnostic = "Static GLB accessor " + std::to_string(accessorIndex) +
                     " has no loaded buffer data.";
        return false;
    }
    const std::size_t elementSize = cgltf_calc_size(accessor.type, accessor.component_type);
    const std::size_t stride = accessor.stride != 0u ? accessor.stride : elementSize;
    if (elementSize == 0u || stride < elementSize)
    {
        diagnostic = "Static GLB accessor data is out of range.";
        return false;
    }
    const std::size_t lastByte = accessor.count == 0u
        ? accessor.offset
        : accessor.offset + (accessor.count - 1u) * stride + elementSize;
    if (accessor.offset > accessor.buffer_view->size || lastByte > accessor.buffer_view->size ||
        accessor.buffer_view->offset > accessor.buffer_view->buffer->size ||
        accessor.buffer_view->size > accessor.buffer_view->buffer->size - accessor.buffer_view->offset)
    {
        diagnostic = "Static GLB accessor data is out of range.";
        return false;
    }
    return true;
}

bool ValidateNodeTransform(const cgltf_node& node, std::string& diagnostic)
{
    const bool finite = (!node.has_translation || AllFinite(node.translation, 3u)) &&
                        (!node.has_rotation || AllFinite(node.rotation, 4u)) &&
                        (!node.has_scale || AllFinite(node.scale, 3u)) &&
                        (!node.has_matrix || AllFinite(node.matrix, 16u));
    if (!finite)
    {
        diagnostic = "Static GLB node '" + NodeName(node) + "' contains a non-finite transform.";
        return false;
    }
    if (node.has_scale && (node.scale[0] < 0.0f || node.scale[1] < 0.0f || node.scale[2] < 0.0f))
    {
        diagnostic = "Static GLB node '" + NodeName(node) + "' contains a negative scale.";
        return false;
    }
    if (node.has_matrix)
    {
        const float determinant =
            node.matrix[0] * (node.matrix[5] * node.matrix[10] - node.matrix[9] * node.matrix[6]) -
            node.matrix[4] * (node.matrix[1] * node.matrix[10] - node.matrix[9] * node.matrix[2]) +
            node.matrix[8] * (node.matrix[1] * node.matrix[6] - node.matrix[5] * node.matrix[2]);
        if (determinant < 0.0f)
        {
            diagnostic = "Static GLB node '" + NodeName(node) + "' contains a negative scale.";
            return false;
        }
    }
    return true;
}

} // namespace horde::scene::assets
