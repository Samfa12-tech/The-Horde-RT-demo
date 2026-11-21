#pragma once

#include <array>
#include <filesystem>

#include "gameplay/dialogue/ChapterDialogue.h"
#include "gameplay/traversal/BellwetherMill.h"
#include "vulkan/raytracing/PresentableTinyRtScene.h"

namespace horde::vulkan::raytracing {

inline constexpr std::size_t kBellwetherResidentCount = 2;

inline auto BellwetherResidentSources(const std::filesystem::path& root,
    gameplay::bellwether::Specialist specialist = gameplay::bellwether::Specialist::RecordKeeper)
{
    const auto service = specialist == gameplay::bellwether::Specialist::Artificer ?
        ChapterResidentActorSource{gameplay::bellwether::kArtificerActorId,
            root / "artificer", "resident.glb", {{"Warden|idle", true, true}}} :
        ChapterResidentActorSource{gameplay::dialogue::kRecordKeeperActorId,
            root / "record-keeper", "resident.glb", {{"Warden|idle", true, true}}};
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

} // namespace horde::vulkan::raytracing
