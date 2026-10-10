#pragma once
#include "gameplay/ShowcaseGameplay.h"

namespace horde::gameplay::effects
{
// Existing float discriminator: 0 = skeleton; [0.75, 1] = live Keeper
// with room-mist density [0, 1]; 2 = discharge or preserved legacy capture.
// No field, packing, physical light or quality-buffer layout is reinterpreted.
// Every existing surface/light consumer still identifies the Keeper by > 0.5.
inline float KeeperShaderPresentationKind(bool selectedKeeper, const LichSnapshot& keeper)
{
    if (!selectedKeeper) return 0.0f;
    if (!keeper.readableCastPresentation) return 2.0f;
    const bool discharge = keeper.phase == LichPhase::Recovering &&
        keeper.phaseTime >= 0.0f &&
        keeper.phaseTime < LichEncounter::kDischargeVisibleBurstDuration &&
        keeper.staffLightStrength >= 0.05f;
    if (discharge) return 2.0f;
    const float density = std::isfinite(keeper.roomMistDensityScale) ?
        std::clamp(keeper.roomMistDensityScale, 0.0f, 1.0f) : 1.0f;
    return 0.75f + 0.25f * density;
}
} // namespace horde::gameplay::effects
