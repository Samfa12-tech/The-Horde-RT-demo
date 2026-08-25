#pragma once

#include <cstddef>
#include <string>

struct cgltf_accessor;
struct cgltf_node;

namespace horde::scene::assets
{

bool IsSupportedRequiredExtension(const char* name);
bool ValidateAccessorRange(const cgltf_accessor& accessor,
                           std::size_t accessorIndex,
                           std::string& diagnostic);
bool ValidateNodeTransform(const cgltf_node& node, std::string& diagnostic);

} // namespace horde::scene::assets
