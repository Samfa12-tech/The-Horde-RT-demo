#include "vulkan/raytracing/SimulationFrameAdapter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>

namespace {
bool FramesWater(const horde::graphics::GraphicsPreviewDescription& scene,
                 const horde::vulkan::raytracing::RtSceneFrameInputs& frame,
                 const float aspect)
{
    using Point = std::array<float, 3u>;
    const auto dot = [](const Point& a, const Point& b) {
        return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
    };
    // Project authored targets through the fixed production RT camera: eye at
    // 0.70m, forward=(sin(yaw),pitch-0.05,-cos(yaw)), focal1.22/vertical0.74.
    // Geometry/frustum checks avoid a literal camera-coordinate assertion.
    const float y = std::clamp(frame.cameraPitch,-0.32f,0.28f)-0.05f;
    const float inverseLength = 1.0f/std::sqrt(1.0f+y*y);
    const float sine=std::sin(frame.cameraYaw), cosine=std::cos(frame.cameraYaw);
    const Point forward{sine*inverseLength,y*inverseLength,-cosine*inverseLength};
    const Point right{cosine,0.0f,sine};
    const Point up{-y*sine*inverseLength,inverseLength,y*cosine*inverseLength};
    float poolLow=std::numeric_limits<float>::max(), poolHigh=-poolLow;
    const auto project = [&](const Point& target, const bool pool) {
        const Point delta{target[0]-frame.cameraX,target[1]-0.70f,target[2]-frame.cameraZ};
        const float depth=dot(delta,forward);
        if (!(depth>0.01f)) return false;
        const float horizontal=1.22f*dot(delta,right)/(aspect*depth);
        const float vertical=1.22f*dot(delta,up)/(0.74f*depth);
        if (!std::isfinite(horizontal) || !std::isfinite(vertical) ||
            std::abs(horizontal)>0.95f || std::abs(vertical)>0.95f) return false;
        if (pool) { poolLow=std::min(poolLow,vertical); poolHigh=std::max(poolHigh,vertical); }
        return true;
    };
    for (const auto& quad : scene.waterQuads)
        for (const auto& vertex : quad.vertices) if (!project(vertex,true)) return false;
    if (!(poolHigh-poolLow>0.18f)) return false;
    for (const auto& stream : scene.waterStreams)
        for (const float height : {scene.waterRingHeights.front(),scene.waterRingHeights.back()})
            for (const float sideX : {-1.0f,1.0f})
                for (const float sideZ : {-1.0f,1.0f})
                    if (!project({scene.waterOrigin[0]+sideX*stream.radiusX,height,
                                  scene.waterOrigin[2]+stream.centreZ+sideZ*stream.radiusZ},false)) return false;
    return true;
}
}

int main()
{
    using namespace horde::vulkan::raytracing;
    bool passed = true;
    const auto check = [&passed](bool condition, const char* label) {
        if (!condition) { std::cerr << label << "\n"; passed = false; }
    };
    horde::gameplay::simulation::SimulationSnapshot game{};
    game.tickIndex = 417;
    game.playerX = 0.31f;
    const auto legacy = BuildRtSceneFrameInputs(game, 1.0f, WaterQuality::High);
    check(!legacy.fireDetail, "legacy adapters retain coupled fixture fallback");
    const auto independent = BuildRtSceneFrameInputs(game, 1.0f, WaterQuality::Off, RtSceneTuning{}, FireEmitterQuality::High);
    check(independent.waterQuality == WaterQuality::Off && independent.fireDetail == FireEmitterQuality::High,
          "explicit fire quality remains independent of water off");
    const auto lowerFireHigherShadow = BuildRtSceneFrameInputs(game,1.0f,WaterQuality::Off,
        RtSceneTuning{},FireEmitterQuality::Low,horde::graphics::ShadowQuality::Higher);
    check(lowerFireHigherShadow.fireDetail == FireEmitterQuality::Low &&
          lowerFireHigherShadow.shadowQuality == horde::graphics::ShadowQuality::Higher &&
          lowerFireHigherShadow.tuning.workloadPreset == RtWorkloadPreset::Authored &&
          lowerFireHigherShadow.tuning.fogDensityScale == 1.0f && game.tickIndex == 417,
          "independent shadow/fire adapter does not promote Max/mist or mutate simulation");
    horde::graphics::GraphicsPreviewSession session;
    horde::gameplay::effects::FireEmitterFixedStepInput torch;
    torch.worldFromFlame[12] = -3.65f; torch.worldFromFlame[13] = 0.965f;
    torch.worldFromLight[12] = -3.65f; torch.worldFromLight[13] = 0.935f;
    session.SetFireSockets(0, torch);
    session.SetMotion(true);
    session.SelectCamera(horde::graphics::GraphicsPreviewCamera::Skeleton);
    session.Advance(0.1);
    const auto frame = BuildGraphicsPreviewFrameInputs(session, 1.2f, WaterQuality::Mobile, FireEmitterQuality::High);
    check(frame.tickIndex == 6 && frame.walkTime == 0.1f, "preview fixed clock is the frame time authority");
    check(frame.previewMotion && frame.outputExposure == 1.2f, "preview motion and exposure survive adaptation");
    check(frame.skeletonEnemyCount == 1 && frame.roster.renderedEnemyCount == 1 &&
          frame.skeletonEnemies[0].animation == horde::gameplay::EnemyAnimation::Idle &&
          frame.skeletonEnemies[0].action == horde::gameplay::EnemyCombatAction::Locomotion, "real single idle actor admission");
    check(frame.fireEmitterCount == 2 && frame.fireEmitters[0].worldFromFlame == torch.worldFromFlame &&
          frame.fireEmitters[0].worldFromLight == torch.worldFromLight, "actual socket matrices reach preview fire inputs");
    check(frame.fireDetail == FireEmitterQuality::High && frame.waterQuality == WaterQuality::Mobile, "preview explicit fire split");
    check(frame.lich.staffLightStrength == 0.0f, "unadmitted lich has no light");
    session.Pause(true); session.Advance(0.1);
    const auto paused = BuildGraphicsPreviewFrameInputs(session, 1.2f, WaterQuality::Off, FireEmitterQuality::Mobile);
    check(paused.tickIndex == frame.tickIndex && paused.skeletonEnemies[0].animationTime == frame.skeletonEnemies[0].animationTime,
          "paused preview keeps animation time fixed");
    session.Reset();
    const auto reset = BuildGraphicsPreviewFrameInputs(session, 1.2f, WaterQuality::Mobile, FireEmitterQuality::High);
    check(reset.tickIndex == 0 && reset.skeletonEnemies[0].animationTime == 0.0f && reset.fireEmitters[0].phase == 0.0f,
          "comparison reset rewinds actor and fire timeline");
    check(game.tickIndex == 417 && game.playerX == 0.31f, "preview adaptation preserves paused game snapshot");
    session.SetMotion(false);
    session.SelectCamera(horde::graphics::GraphicsPreviewCamera::Water);
    const auto water = BuildGraphicsPreviewFrameInputs(session,1.0f,WaterQuality::High,FireEmitterQuality::High);
    const auto description = horde::graphics::MakeGraphicsPreviewDescription();
    check(FramesWater(description,water,16.0f/9.0f) && FramesWater(description,water,9.0f/16.0f),
          "Water preset frames readable pool and falling-stream extents in landscape and portrait");
    auto missedPool = water;
    missedPool.cameraX=-3.55f; missedPool.cameraZ=-14.40f;
    missedPool.cameraYaw=0.90f; missedPool.cameraPitch=-0.12f;
    check(!FramesWater(description,missedPool,16.0f/9.0f),
          "frustum fixture rejects the actual former close pose that cropped the pool");
    const auto previewBefore = session.Snapshot();
    const auto selected = BuildGraphicsPreviewFrameInputs(session,1.0f,WaterQuality::Mobile,
        FireEmitterQuality::Low,RtSceneTuning{},horde::graphics::ShadowQuality::Lower);
    check(selected.fireDetail == FireEmitterQuality::Low && selected.shadowQuality == horde::graphics::ShadowQuality::Lower &&
          selected.tuning.workloadPreset == RtWorkloadPreset::Authored &&
          session.Snapshot().tick == previewBefore.tick && session.Snapshot().timeSeconds == previewBefore.timeSeconds,
          "preview independent policy adapter consumes immutable timeline without advancing gameplay");
    return passed ? 0 : 1;
}
