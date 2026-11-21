#pragma once

#include <array>
#include <filesystem>

#include "gameplay/dialogue/ChapterDialogue.h"
#include "vulkan/raytracing/PresentableTinyRtScene.h"

namespace horde::vulkan::raytracing {

inline constexpr std::size_t kBellwetherResidentCount = 2;

inline auto BellwetherResidentSources(const std::filesystem::path& root)
{
    return std::array<ChapterResidentActorSource, kBellwetherResidentCount>{
        ChapterResidentActorSource{gameplay::dialogue::kTavernKeeperActorId,
            root / "tavern-keeper", "resident.glb", {{"Warden|idle", true, true}, {"Warden|walk", true, true}}},
        ChapterResidentActorSource{gameplay::dialogue::kRecordKeeperActorId,
            root / "record-keeper", "resident.glb", {{"Warden|idle", true, true}}}
    };
}

inline bool SetBellwetherResidentSources(PresentableTinyRtScene& scene,
    const std::filesystem::path& root, std::string& diagnostic)
{
    const bool ready = root.empty() ? scene.SetChapterResidentActors({}, diagnostic) :
        scene.SetChapterResidentActors(BellwetherResidentSources(root), diagnostic);
    if (ready) scene.RequestPackagedBellwetherFurniture(!root.empty());
    return ready;
}

} // namespace horde::vulkan::raytracing
