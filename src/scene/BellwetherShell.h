#pragma once
#include "gameplay/simulation/TriangleSupportSurface.h"
#include "scene/assets/StaticMeshAsset.h"
#include <filesystem>

namespace horde::scene {
inline constexpr std::size_t kBellwetherSolidTriangles=2668;
inline constexpr std::size_t kBellwetherGroundTriangles=4;
inline constexpr std::size_t kBellwetherGlazingTriangles=12;
// The town and B01 interior use one render and physical geometry source.
// Architecture, furniture and the fuel bed reuse existing material rows.
bool AppendBellwetherShell(const std::filesystem::path& assetsRoot,
    assets::StaticMeshAsset& world,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>& support,
    std::string& diagnostic, assets::StaticMeshAsset* glazing = nullptr);
}
