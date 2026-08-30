#include "scene/assets/AssetValidation.h"

#include "third_party/cgltf/cgltf.h"

#include <array>
#include <cmath>
#include <cstring>
#include <limits>
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
    std::size_t lastByte = accessor.offset;
    if (accessor.count != 0u)
    {
        const std::size_t intervals = accessor.count - 1u;
        if (intervals > std::numeric_limits<std::size_t>::max() / stride)
        {
            diagnostic = "Static GLB accessor data is out of range.";
            return false;
        }
        const std::size_t span = intervals * stride;
        if (accessor.offset > std::numeric_limits<std::size_t>::max() - span ||
            accessor.offset + span >
                std::numeric_limits<std::size_t>::max() - elementSize)
        {
            diagnostic = "Static GLB accessor data is out of range.";
            return false;
        }
        lastByte = accessor.offset + span + elementSize;
    }
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
    float local[16]{};
    cgltf_node_transform_local(&node, local);
    const float determinant =
        local[0] * (local[5] * local[10] - local[9] * local[6]) -
        local[4] * (local[1] * local[10] - local[9] * local[2]) +
        local[8] * (local[1] * local[6] - local[5] * local[2]);
    if (determinant < 0.0f)
    {
        diagnostic = "Static GLB node '" + NodeName(node) +
            "' has a negative-determinant transform; bake the reflection and reverse triangle winding/normals before runtime import.";
        return false;
    }
    return true;
}

} // namespace horde::scene::assets
