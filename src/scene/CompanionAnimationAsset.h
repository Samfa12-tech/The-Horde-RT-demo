#pragma once
#include "scene/assets/SkinnedMeshAsset.h"
#include <array>

namespace horde::scene {
inline constexpr const char* kCompanionTurnRuntimeFile="kit-warden-turn.runtime.glb";
inline std::array<SkinnedClipBinding,6> CompanionTurnClipBindings() {
    // Keep the established combat semantics unmapped for the companion.
    return {{{"Warden|idle",true,true},{"Warden|walk",true,true},
        {"",false,false},{"",false,false},
        {"KitProof|turn_left",false,true},{"KitProof|turn_right",false,true}}};
}
inline std::array<SkinnedClipBinding,7> CompanionChapterClipBindings() {
    const auto turns=CompanionTurnClipBindings();
    return {turns[0],turns[1],turns[2],turns[3],turns[4],turns[5],
        {"Kit|hearth_sit",false,true,true}};
}
}
