#pragma once

#include "gameplay/simulation/SimulationSnapshot.h"
#include "graphics/EntryMenuScene.h"
#include "graphics/GraphicsPreviewSession.h"
#include "vulkan/raytracing/PresentableTinyRtScene.h"
#include "vulkan/raytracing/PlayerFrameLight.h"

namespace horde::vulkan::raytracing
{
RtSceneFrameInputs BuildEntryMenuFrameInputs(const horde::graphics::EntryMenuSession &session,
                                             float outputExposure, FireEmitterQuality fireDetail,
                                             horde::graphics::ShadowQuality shadowQuality);

RtSceneFrameInputs BuildGraphicsPreviewFrameInputs(
    const horde::graphics::GraphicsPreviewSession& session,
    float outputExposure, WaterQuality waterQuality,
    FireEmitterQuality fireDetail, const RtSceneTuning& tuning = {},
    std::optional<horde::graphics::ShadowQuality> shadowQuality = std::nullopt);

// Preserve the established renderer and shader boundary while making the
// shared simulation the sole gameplay authority on every platform.
RtSceneFrameInputs BuildRtSceneFrameInputs(
    const horde::gameplay::simulation::SimulationSnapshot& simulation,
    float outputExposure,
    WaterQuality waterQuality = WaterQuality::High);

RtSceneFrameInputs BuildRtSceneFrameInputs(
    const horde::gameplay::simulation::SimulationSnapshot& simulation,
    float outputExposure,
    const RtSceneTuning& tuning,
    WaterQuality waterQuality = WaterQuality::High,
    std::optional<FireEmitterQuality> fireDetail = std::nullopt,
    std::optional<horde::graphics::ShadowQuality> shadowQuality = std::nullopt);

RtSceneFrameInputs BuildRtSceneFrameInputs(
    const horde::gameplay::simulation::SimulationSnapshot& simulation,
    float outputExposure,
    WaterQuality waterQuality,
    const RtSceneTuning& tuning,
    std::optional<FireEmitterQuality> fireDetail = std::nullopt,
    std::optional<horde::graphics::ShadowQuality> shadowQuality = std::nullopt);

} // namespace horde::vulkan::raytracing
