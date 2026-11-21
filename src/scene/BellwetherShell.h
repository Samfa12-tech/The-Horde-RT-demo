#pragma once
#include "gameplay/simulation/TriangleSupportSurface.h"
#include "scene/assets/StaticMeshAsset.h"
#include <filesystem>

namespace horde::scene {
// The closed town shares its imported triangles with exterior contact.
// Terrain remains the ground authority. Keep existing stone/clay material rows.
bool AppendBellwetherShell(const std::filesystem::path& assetsRoot,
    assets::StaticMeshAsset& world,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>& support,
    std::string& diagnostic);
}
