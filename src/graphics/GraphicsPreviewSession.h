#pragma once

#include "graphics/GraphicsPreviewScene.h"
#include "gameplay/effects/FireEmitterState.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace horde::graphics
{
struct GraphicsPreviewSnapshot
{
    std::uint64_t tick = 0u;
    std::uint64_t timelineEpoch = 0u;
    double timeSeconds = 0.0;
    PreviewCameraPose camera = GraphicsPreviewCameraPose(GraphicsPreviewCamera::Overview);
    PreviewSkeletonPose skeleton{-4.10f, -16.10f, 0.0f, 0.0f};
    std::array<horde::gameplay::effects::FireEmitterState, 2u> fireEmitters{};
    bool paused = false;
    bool motionTest = false;
};

// Owner-thread presentation-only timeline. It never owns or advances game
// combat, rewards, input counters, event queues, audio, or save state.
class GraphicsPreviewSession
{
public:
    GraphicsPreviewSession() { Reset(); }
    void Pause(const bool paused) noexcept
    {
        if (paused_ != paused) accumulator_ = 0.0;
        paused_ = paused;
    }
    void SetMotion(const bool enabled) noexcept { motion_ = enabled; }
    void SelectCamera(const GraphicsPreviewCamera camera) noexcept { camera_ = camera; }
    void SetFireSockets(const std::size_t index,
                        const horde::gameplay::effects::FireEmitterFixedStepInput& input)
    {
        // Production asset/socket integration supplies coherent flame/light
        // transforms; this session does not guess sockets from visible art.
        inputs_.at(index) = input;
        emitters_.at(index).worldFromFlame = input.worldFromFlame;
        emitters_.at(index).worldFromLight = input.worldFromLight;
        emitters_.at(index).strength = input.strength;
        emitters_.at(index).fuel = input.fuel;
        emitters_.at(index).zone = input.zone;
    }
    void Reset() noexcept
    {
        tick_ = 0u; accumulator_ = 0.0; ++epoch_;
        for (std::size_t index = 0u; index < emitters_.size(); ++index)
        {
            emitters_[index] = horde::gameplay::effects::MakeOpeningTorchFireEmitter();
            emitters_[index].stableId = index == 0u ? 0x50544f52u : 0x504c414eu;
            emitters_[index].seed = index == 0u ? 0x50524531u : 0x50524532u;
            emitters_[index].parentObject = index == 0u ?
                horde::gameplay::effects::FireEmitterParentObject::WorldObject :
                horde::gameplay::effects::FireEmitterParentObject::RewardLantern;
            emitters_[index].worldFromFlame = inputs_[index].worldFromFlame;
            emitters_[index].worldFromLight = inputs_[index].worldFromLight;
            emitters_[index].strength = inputs_[index].strength;
            emitters_[index].fuel = inputs_[index].fuel;
            emitters_[index].zone = inputs_[index].zone;
        }
        emitters_[1].radius = 0.045f; emitters_[1].height = 0.12f; emitters_[1].coreRadius = 0.021f;
    }
    void Advance(const double elapsedSeconds) noexcept
    {
        if (paused_ || !std::isfinite(elapsedSeconds) || elapsedSeconds <= 0.0) return;
        constexpr double step = 1.0 / 60.0;
        // Bound catch-up after a foreground stall; background must call Pause.
        accumulator_ += std::min(elapsedSeconds, 0.25);
        for (unsigned count = 0u; count < 15u && accumulator_ + 1e-12 >= step; ++count)
        {
            accumulator_ = std::max(0.0, accumulator_ - step); ++tick_;
            for (std::size_t index = 0u; index < emitters_.size(); ++index)
                horde::gameplay::effects::StepFireEmitterFixed(emitters_[index], inputs_[index], static_cast<float>(step));
        }
    }
    GraphicsPreviewSnapshot Snapshot() const noexcept
    {
        GraphicsPreviewSnapshot result;
        result.tick = tick_; result.timelineEpoch = epoch_;
        result.timeSeconds = static_cast<double>(tick_) / 60.0;
        result.camera = GraphicsPreviewCameraPose(camera_);
        if (motion_)
        {
            const float phase = static_cast<float>(result.timeSeconds * 0.70);
            result.camera.x += 0.14f * std::sin(phase);
            result.camera.yaw += 0.10f * std::sin(phase * 0.73f);
        }
        result.skeleton.animationTime = static_cast<float>(result.timeSeconds);
        result.fireEmitters = emitters_; result.paused = paused_; result.motionTest = motion_;
        return result;
    }
private:
    std::uint64_t tick_ = 0u;
    std::uint64_t epoch_ = 0u;
    double accumulator_ = 0.0;
    bool paused_ = false;
    bool motion_ = false;
    GraphicsPreviewCamera camera_ = GraphicsPreviewCamera::Overview;
    std::array<horde::gameplay::effects::FireEmitterState, 2u> emitters_{};
    std::array<horde::gameplay::effects::FireEmitterFixedStepInput, 2u> inputs_{};
};
} // namespace horde::graphics
