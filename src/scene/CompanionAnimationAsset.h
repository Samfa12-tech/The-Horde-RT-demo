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
}
