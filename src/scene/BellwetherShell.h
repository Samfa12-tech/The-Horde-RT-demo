#pragma once
#include "gameplay/simulation/TriangleSupportSurface.h"
#include "scene/assets/StaticMeshAsset.h"
#include "../../assets/models/world/source/bellwether-tavern-v1/staging.generated.h"
#include "../../assets/models/world/source/bellwether-millworks-v1/geometry.generated.h"
#include <filesystem>

namespace horde::scene {
inline constexpr std::size_t kBellwetherSolidTriangles=
    gameplay::bellwether::kSolidTriangles+kBellwetherMillWorksSolidTriangles;
inline constexpr std::size_t kWaysideCoverTriangles=24;
inline constexpr std::size_t kWaysideRegisterTriangles=48;
inline constexpr std::size_t kBellwetherGroundTriangles=gameplay::bellwether::kGroundTriangles;
inline constexpr std::size_t kBellwetherGlazingTriangles=12;
enum class BellwetherFurnitureDetail { Near, Mid, Far };
inline constexpr std::array<std::size_t,3> kBellwetherFurnitureTriangles{1012,276,144};
bool HasBellwetherFurnitureRuntime(const std::filesystem::path& assetsRoot);
float BellwetherFurnitureRoomDistanceSquared(float x, float z) noexcept;
// Selects one room contributor for every ray type, independently of camera facing.
BellwetherFurnitureDetail SelectBellwetherFurnitureDetail(
    float x, float z, BellwetherFurnitureDetail current) noexcept;
// The town and B01 interior use one render and physical geometry source.
// Architecture, furniture and the fuel bed reuse existing material rows.
bool AppendBellwetherShell(const std::filesystem::path& assetsRoot,
    assets::StaticMeshAsset& world,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>& support,
    std::string& diagnostic, assets::StaticMeshAsset* glazing = nullptr,
    assets::StaticMeshAsset* archiveCover = nullptr,
    assets::StaticMeshAsset* roadRegister = nullptr,
    assets::StaticMeshAsset* southDoor = nullptr,
    BellwetherFurnitureDetail furnitureDetail = BellwetherFurnitureDetail::Near,
    assets::StaticMeshAsset* isolatedFurnitureTiers = nullptr);
}
