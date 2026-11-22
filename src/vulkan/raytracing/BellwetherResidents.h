#pragma once

#include <array>
#include <filesystem>

#include "gameplay/dialogue/ChapterDialogue.h"
#include "gameplay/traversal/BellwetherMill.h"
#include "gameplay/traversal/BellwetherSalvager.h"
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

} // namespace horde::vulkan::raytracing
