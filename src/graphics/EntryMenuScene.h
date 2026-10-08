#pragma once

#include "audio/MenuAmbience.h"
#include "gameplay/effects/FireEmitterState.h"
#include "gameplay/items/LanternPendulum.h"
#include "gameplay/items/HeldItemKinematics.h"
#include "graphics/GraphicsPreviewScene.h"
#include <cmath>

namespace horde::graphics
{
inline constexpr float kEntryMenuLanternScale = 1.0f;
// A small content profile with shared stone/metal/PBR shading. It admits no
// gameplay actor, water, player or reward state.
inline GraphicsPreviewDescription MakeEntryMenuDescription()
{
    GraphicsPreviewDescription result;
    result.worldQuads.clear();
    result.waterQuads.clear();
    result.lanternPosition = {{0.0f, 1.35f, -1.65f}};
    const auto quad = [&result](std::array<PreviewPoint, 4u> vertices, std::uint32_t material,
                                std::uint32_t normal) {
        result.worldQuads.push_back({vertices, material, normal});
    };
    const float floor = horde::gameplay::kRouteFloorWorldY;
    quad({{{-3, floor, 1}, {3, floor, 1}, {3, floor, -4}, {-3, floor, -4}}}, 0, 0);
    quad({{{-3, floor, -4}, {3, floor, -4}, {3, 3, -4}, {-3, 3, -4}}}, 0, 4);
    quad({{{-3, floor, 1}, {-3, floor, -4}, {-3, 3, -4}, {-3, 3, 1}}}, 0, 2);
    quad({{{3, floor, -4}, {3, floor, 1}, {3, 3, 1}, {3, 3, -4}}}, 0, 3);
    quad({{{-3, 3, -4}, {3, 3, -4}, {3, 3, 1}, {-3, 3, 1}}}, 0, 1);
    // Raised stone jambs and segmented arch: ordinary world surfaces rather
    // than a background picture. Metal chain links terminate at the pivot.
    for (float x : {-1.12f, 1.12f})
        quad({{{x - .14f, floor, -3.65f},
               {x + .14f, floor, -3.65f},
               {x + .14f, 1.62f, -3.65f},
               {x - .14f, 1.62f, -3.65f}}},
             0, 4);
    for (int segment = 0; segment < 12; ++segment)
    {
        const float a = segment * 3.14159265f / 12.0f;
        const float b = (segment + 1) * 3.14159265f / 12.0f;
        quad({{{1.00f * std::cos(a), 1.60f + 1.00f * std::sin(a), -3.65f},
               {1.26f * std::cos(a), 1.60f + 1.26f * std::sin(a), -3.65f},
               {1.26f * std::cos(b), 1.60f + 1.26f * std::sin(b), -3.65f},
               {1.00f * std::cos(b), 1.60f + 1.00f * std::sin(b), -3.65f}}},
             0, 4);
    }
    for (int link = 0; link < 29; ++link)
    {
        const float y = 1.38f + link * .058f;
        for (float x : {-.018f, .018f})
            quad({{{x - .006f, y, -1.65f},
                   {x + .006f, y, -1.65f},
                   {x + .006f, y + .055f, -1.65f},
                   {x - .006f, y + .055f, -1.65f}}},
                 4, 4);
        for (float offset : {0.0f, .049f})
            quad({{{-.024f, y + offset, -1.65f},
                   {.024f, y + offset, -1.65f},
                   {.024f, y + offset + .006f, -1.65f},
                   {-.024f, y + offset + .006f, -1.65f}}},
                 4, 4);
    }
    return result;
}

struct EntryMenuSnapshot
{
    std::uint64_t tick = 0;
    std::uint64_t chainCreakSerial = 0;
    PreviewCameraPose camera{0.0f, 0.0f, 0.0f, 0.015f};
    horde::gameplay::items::HeldItemTransform hinge{};
    horde::gameplay::interactions::LanternPendulumSnapshot pendulum{};
    horde::gameplay::effects::FireEmitterState fire{};
    float fade = 0.0f;
};

class EntryMenuSession
{
  public:
    EntryMenuSession()
    {
        Reset();
    }
    void Reset()
    {
        tick_ = 0;
        chainCreaks_.Reset();
        accumulator_ = 0;
        pan_ = 0;
        fade_ = 0;
        playing_ = false;
        paused_ = false;
        side_ = false;
        hinge_ = horde::gameplay::items::IdentityHeldItemTransform();
        const auto position = MakeEntryMenuDescription().lanternPosition;
        hinge_[12] = position[0];
        hinge_[13] = position[1];
        hinge_[14] = position[2];
        pendulum_.Reset(hinge_);
        fire_ = horde::gameplay::effects::MakeOpeningTorchFireEmitter();
        fire_.stableId = 0x454c414eu;
        fire_.seed = 0x454e5452u;
        fire_.parentObject = horde::gameplay::effects::FireEmitterParentObject::WorldObject;
        fire_.radius = .045f;
        fire_.height = .12f;
        fire_.coreRadius = .021f;
        UpdateSockets();
    }
    void ConfigureSockets(const horde::gameplay::items::HeldItemTransform &flame,
                          const horde::gameplay::items::HeldItemTransform &light, float bodyScale)
    {
        flame_ = flame;
        light_ = light;
        scale_ = bodyScale;
        UpdateSockets();
    }
    void Pause(bool value) noexcept
    {
        if (paused_ != value)
            accumulator_ = 0;
        paused_ = value;
    }
    void SetReducedMotion(bool value) noexcept
    {
        reduced_ = value;
    }
    void ShowSidePage(bool value) noexcept
    {
        side_ = value;
    }
    void Play() noexcept
    {
        playing_ = true;
    }
    bool ReadyToPlay() const noexcept
    {
        return playing_ && fade_ >= 1.0f;
    }
    void Advance(double elapsed)
    {
        if (paused_ || !std::isfinite(elapsed) || elapsed <= 0)
            return;
        accumulator_ += std::min(elapsed, .25);
        constexpr float step = 1.0f / 60.0f;
        for (unsigned count = 0; count < 15 && accumulator_ + 1e-12 >= 1.0 / 60; ++count)
        {
            accumulator_ = std::max(0.0, accumulator_ - 1.0 / 60);
            ++tick_;
            const float target = side_ ? -.60f : 0.0f;
            pan_ = reduced_ ? target : pan_ + std::clamp(target - pan_, -step * 1.2f, step * 1.2f);
            if (playing_)
                fade_ = std::min(1.0f, fade_ + step / (reduced_ ? .08f : .30f));
            // A gentle air-current torque drives the existing damped physical
            // pendulum. The chain pivot and ring stay fixed; body, Flame and
            // Light share the resulting body transform.
            if (!reduced_)
            {
                auto motion = pendulum_.Snapshot();
                motion.strafeAngularVelocity +=
                    .65f * step * std::cos(static_cast<float>(tick_) * step * .85f);
                pendulum_.Import(motion);
            }
            else
                pendulum_.Reset(hinge_);
            pendulum_.StepFixed(hinge_, step);
            const auto &motion = pendulum_.Snapshot();
            chainCreaks_.Step(tick_, motion.strafeAngleRadians, motion.strafeAngularVelocity, reduced_ || playing_);
            UpdateSockets();
            horde::gameplay::effects::StepFireEmitterFixed(fire_, fireInput_, step);
        }
    }
    EntryMenuSnapshot Snapshot() const noexcept
    {
        EntryMenuSnapshot result;
        result.tick = tick_;
        result.chainCreakSerial = chainCreaks_.Serial();
        result.camera.x = pan_;
        result.hinge = hinge_;
        result.pendulum = pendulum_.Snapshot();
        result.fire = fire_;
        result.fade = fade_;
        return result;
    }

  private:
    void UpdateSockets()
    {
        auto scale = horde::gameplay::items::IdentityHeldItemTransform();
        scale[0] = scale[5] = scale[10] = scale_;
        const auto body = horde::gameplay::items::MultiplyHeldItemTransforms(
            pendulum_.Snapshot().worldFromBody, scale);
        fireInput_.worldFromFlame =
            horde::gameplay::items::MultiplyHeldItemTransforms(body, flame_);
        fireInput_.worldFromLight =
            horde::gameplay::items::MultiplyHeldItemTransforms(body, light_);
        // Owner-approved composition, with 10% more emitted lantern light.
        fireInput_.strength = .99f;
        fire_.worldFromFlame = fireInput_.worldFromFlame;
        fire_.worldFromLight = fireInput_.worldFromLight;
    }
    horde::audio::MenuChainCreaks chainCreaks_;
    std::uint64_t tick_ = 0;
    double accumulator_ = 0;
    bool paused_ = false, reduced_ = false, side_ = false, playing_ = false;
    float pan_ = 0, fade_ = 0, scale_ = kEntryMenuLanternScale;
    horde::gameplay::items::HeldItemTransform hinge_{}, flame_{}, light_{};
    horde::gameplay::interactions::LanternPendulum pendulum_;
    horde::gameplay::effects::FireEmitterState fire_{};
    horde::gameplay::effects::FireEmitterFixedStepInput fireInput_{};
};
} // namespace horde::graphics
