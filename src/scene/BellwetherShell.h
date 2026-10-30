#pragma once
#include "scene/assets/StaticMeshAsset.h"
#include <filesystem>

namespace horde::scene {
// The distant town is closed scenery, not a collision/support or gameplay zone.
// Existing admitted stone/clay rows remain the texture/material owners.
bool AppendBellwetherShell(const std::filesystem::path& assetsRoot,
    assets::StaticMeshAsset& world, std::string& diagnostic);
}
