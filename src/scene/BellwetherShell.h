#pragma once
#include "gameplay/simulation/TriangleSupportSurface.h"
#include "scene/assets/StaticMeshAsset.h"
#include "../../assets/models/world/source/bellwether-tavern-v1/staging.generated.h"
#include "../../assets/models/world/source/bellwether-millworks-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-mill-crossing-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-relic-stall-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-road-cache-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-well-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-garden-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-privy-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-privy-rag-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-graveyard-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-smith-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-grate-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-tally-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-stock-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-mill-lamp-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-paths-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-square-frontage-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-west-trade-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-tavern-rail-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-garden-house-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-well-apron-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-hearth-service-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-square-waymarker-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-roofline-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-house-b-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-house-b-ledger-v1/geometry.generated.h"
#include "../../assets/models/world/source/bellwether-church-station-v1/geometry.generated.h"
#include <filesystem>

namespace horde::scene {
inline constexpr std::size_t kBellwetherSolidTriangles=
    gameplay::bellwether::kSolidTriangles+kBellwetherMillWorksSolidTriangles+
    gameplay::bellwether::kMillCrossingSolidTriangles+gameplay::bellwether::kRelicStallSolidTriangles+
    gameplay::bellwether::kRoadSilverCacheSolidTriangles+gameplay::bellwether::kBellwetherWellSolidTriangles+
    gameplay::bellwether::kBellwetherGardenSolidTriangles+gameplay::bellwether::kBellwetherPrivySolidTriangles+
    gameplay::bellwether::kPrivyRagBasketSolidTriangles+gameplay::bellwether::kBellwetherGraveyardSolidTriangles+
    gameplay::bellwether::kBellwetherSmithSolidTriangles+gameplay::bellwether::kBellwetherGrateSolidTriangles+
    gameplay::bellwether::kMillTallySolidTriangles+gameplay::bellwether::kBellwetherStockSolidTriangles+
    gameplay::bellwether::kMillBenchLampSolidTriangles+gameplay::bellwether::kSquareFrontageSolidTriangles+
    gameplay::bellwether::kWestTradeSolidTriangles+gameplay::bellwether::kTavernRailSolidTriangles+
    gameplay::bellwether::kGardenHouseSolidTriangles+gameplay::bellwether::kWellApronSolidTriangles+
    gameplay::bellwether::kTavernServiceSolidTriangles+gameplay::bellwether::kSquareWaymarkerSolidTriangles+
    gameplay::bellwether::kVillageRoofAddedSolidTriangles+gameplay::bellwether::kHouseBSolidTriangles+
    gameplay::bellwether::kChurchRecordStationTriangles+gameplay::bellwether::kHouseBLedgerSolidTriangles;
inline constexpr std::size_t kWaysideCoverTriangles=24;
inline constexpr std::size_t kWaysideRegisterTriangles=48;
inline constexpr std::size_t kBellwetherGroundTriangles=
    gameplay::bellwether::kGroundTriangles+gameplay::bellwether::kMillCrossingGroundTriangles+
    gameplay::bellwether::kBellwetherGraveyardGroundTriangles+gameplay::bellwether::kBellwetherSmithGroundTriangles+
    gameplay::bellwether::kBellwetherPathGroundTriangles+gameplay::bellwether::kGardenHousePathTriangles+
    gameplay::bellwether::kWellApronGroundTriangles+gameplay::bellwether::kHouseBGroundTriangles;
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
    assets::StaticMeshAsset* isolatedFurnitureTiers = nullptr,
    const assets::StaticMaterial* timberMaterialProvider = nullptr);
// Soft crop foliage reuses the deferred ForestAlder atlas family and creates no contact geometry.
bool AppendBellwetherGardenCrops(const std::filesystem::path& assetsRoot,
    assets::StaticMeshAsset& world, std::string& diagnostic);
}
