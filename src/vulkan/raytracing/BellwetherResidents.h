#pragma once

#include <algorithm>
#include <array>
#include <filesystem>
#include <span>

#include "gameplay/dialogue/ChapterDialogue.h"
#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/traversal/BellwetherMill.h"
#include "gameplay/traversal/BellwetherResidentRoster.h"
#include "gameplay/traversal/BellwetherSalvager.h"
#include "scene/assets/AssetManifest.h"
#include "vulkan/raytracing/PresentableTinyRtScene.h"

namespace horde::vulkan::raytracing {

inline constexpr std::size_t kBellwetherResidentCount = 2;

inline auto BellwetherResidentSources(const std::filesystem::path& root,
    gameplay::bellwether::Specialist specialist = gameplay::bellwether::Specialist::RecordKeeper)
{
    using gameplay::bellwether::Specialist;
    auto actorId=gameplay::dialogue::kRecordKeeperActorId;
    const char* role="record-keeper";
    if(specialist==Specialist::Artificer) {
        actorId=gameplay::bellwether::kArtificerActorId;
        role="artificer";
    } else if(specialist==Specialist::Salvager) {
        actorId=gameplay::bellwether::kSalvagerActorId;
        role="salvager";
    }
    ChapterResidentActorSource service{actorId,root/role,"resident.glb",{{"Warden|idle",true,true}}};
    return std::array<ChapterResidentActorSource, kBellwetherResidentCount>{
        ChapterResidentActorSource{gameplay::dialogue::kTavernKeeperActorId,
            root / "tavern-keeper", "resident.glb", {{"Warden|idle", true, true}, {"Warden|walk", true, true}}},
        service
    };
}

inline bool SetBellwetherResidentSources(PresentableTinyRtScene& scene,
    const std::filesystem::path& root, std::string& diagnostic,
    gameplay::bellwether::Specialist specialist = gameplay::bellwether::Specialist::RecordKeeper)
{
    if (!root.empty() && specialist == gameplay::bellwether::Specialist::None) {
        diagnostic = "A Bellwether resident directory requires a specialist role.";
        return false;
    }
    const bool ready = root.empty() ? scene.SetChapterResidentActors({}, diagnostic) :
        scene.SetChapterResidentActors(BellwetherResidentSources(root, specialist), diagnostic);
    if (ready) scene.RequestPackagedBellwetherFurniture(!root.empty());
    return ready;
}

namespace detail {
struct BellwetherResident {
    std::uint32_t actor;
    gameplay::traversal::ResidentMask readyBit;
    const char* directory;
    bool walking = false;
};

inline constexpr std::array kResidentRoster{
    BellwetherResident{gameplay::dialogue::kTavernKeeperActorId, gameplay::traversal::kTavernKeeperResident, "tavern-keeper", true},
    BellwetherResident{gameplay::dialogue::kRecordKeeperActorId, gameplay::traversal::kRecordKeeperResident, "record-keeper"},
    BellwetherResident{gameplay::bellwether::kArtificerActorId, gameplay::traversal::kArtificerResident, "artificer"},
    BellwetherResident{gameplay::bellwether::kSalvagerActorId, gameplay::traversal::kSalvagerResident, "salvager"},
};

inline ChapterResidentActorSource ResidentSource(const std::filesystem::path& root, const BellwetherResident& resident)
{
    ChapterResidentActorSource source{resident.actor, root / resident.directory, "resident.glb", {{"Warden|idle", true, true}}};
    if (resident.walking) source.clips.push_back({"Warden|walk", true, true});
    return source;
}

inline gameplay::traversal::ResidentMask ResidentMaskFor(std::span<const std::uint32_t> active)
{
    gameplay::traversal::ResidentMask mask = 0;
    for (const auto id : active)
        for (const auto& resident : kResidentRoster)
            if (id == resident.actor) mask |= resident.readyBit;
    return mask;
}
}

inline bool SetBellwetherCombinedResidentSources(PresentableTinyRtScene& scene,
    const std::filesystem::path& root, std::string& diagnostic)
{
    if (root.empty()) return SetBellwetherResidentSources(scene, root, diagnostic);
    RtResidentPayloadCapacity capacity{};
    for (const auto& resident : detail::kResidentRoster) {
        horde::scene::assets::AssetManifest manifest;
        if (!horde::scene::assets::AssetManifest::Load(root / resident.directory / "asset.manifest.json", manifest, diagnostic))
            return false;
        capacity.vertices = std::max(capacity.vertices, manifest.budgets.maxVertices);
        capacity.indices = std::max(capacity.indices, manifest.budgets.maxIndices);
    }
    std::array sources{detail::ResidentSource(root, detail::kResidentRoster[0]),
        detail::ResidentSource(root, detail::kResidentRoster[1])};
    for (auto& source : sources) source.replacementCapacity = capacity;
    if (!scene.SetChapterResidentActors(sources, diagnostic)) return false;
    scene.RequestPackagedBellwetherFurniture(true);
    return true;
}

// The combined Debug route provisions both eastern residents before reaching
// their service marks. The south approach keeps the tavern's door resident.
// The margins stop a turn at the lane boundary from repeatedly loading assets.
inline bool AdvanceBellwetherCombinedResidents(PresentableTinyRtScene& scene,
    gameplay::simulation::GameSimulation& simulation, const std::filesystem::path& root,
    bool (*retireReferences)(void*, std::string&), void* user, std::string& diagnostic)
{
    using gameplay::dialogue::BellwetherArrivalStage;
    const auto& snapshot = simulation.Snapshot();
    auto active = scene.ChapterResidentSourceIds();
    const auto& roster = detail::kResidentRoster;
    const bool atTown = snapshot.rescue.exteriorSide && !snapshot.rescue.equipmentStowed &&
        snapshot.chapterCompanion.bellwetherArrival == BellwetherArrivalStage::Seated;
    const bool east = atTown && snapshot.playerX >= (active[0] == roster[2].actor ? 92.f : 94.f) &&
        snapshot.playerZ >= (active[0] == roster[2].actor ? 106.f : 108.f);
    const bool south = atTown && snapshot.playerX >= 74.f &&
        snapshot.playerZ >= (active[1] == roster[3].actor ? 118.5f : 120.f);
    const std::array desired{east ? roster[2] : roster[0], (east || south) ? roster[3] : roster[1]};
    const bool approachingHandoff = atTown && snapshot.playerX >= 68.f && snapshot.playerZ >= 100.f;
    using Publication = PresentableTinyRtScene::ResidentPublication;
    bool failed = false;
    for (std::size_t lane = 0; lane < desired.size(); ++lane) {
        const bool switching = active[lane] != desired[lane].actor;
        if (!approachingHandoff && !switching) continue;
        // Each lane owns one alternate model and keeps it through a reversal.
        const auto& alternate = roster[active[lane] == roster[lane].actor ? lane + desired.size() : lane];
        if (scene.PrefetchedChapterResidentId(lane) != alternate.actor &&
            !scene.PrefetchChapterResidentActor(active[lane], detail::ResidentSource(root, alternate), diagnostic)) {
            failed = true;
            break;
        }
        if (!switching) continue;
        const auto status = scene.PublishPrefetchedChapterResidentActor(active[lane], desired[lane].actor,
            retireReferences, user, diagnostic);
        if (status == Publication::Failed) {
            failed = true;
            break;
        }
        if (status == Publication::Published) active[lane] = desired[lane].actor;
    }
    // A pending or failed lane keeps its old resources. An independently
    // completed swap must still reach the simulation on this same frame.
    if (!simulation.PublishBellwetherResidents(detail::ResidentMaskFor(active))) {
        diagnostic = "Combined Bellwether could not publish its resident roster to the shared simulation.";
        return false;
    }
    if (failed) return false;
    diagnostic.clear();
    return true;
}

} // namespace horde::vulkan::raytracing
