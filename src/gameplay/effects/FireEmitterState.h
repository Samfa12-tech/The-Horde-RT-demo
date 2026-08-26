#pragma once

#include "gameplay/ShowcaseRoute.h"
#include "gameplay/items/HeldItemState.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace horde::gameplay::effects
{

inline constexpr std::size_t kFireEmitterCapacity = 4u;
inline constexpr std::size_t kActiveFireEmitterCapacity = 2u;

enum class FireEmitterParentObject : std::uint32_t
{
    OriginalTorch = 1u,
    RewardLantern = 2u,
    WorldObject = 3u,
};

enum class FireEmitterSocket : std::uint32_t
{
    Flame = 1u,
    Light = 2u,
};

struct FireEmitterState
{
    std::uint32_t stableId = 1u;
    std::uint32_t seed = 0x484f5244u;
    float strength = 1.0f;
    float fuel = 1.0f;
    float phase = 0.0f;
    float colourTemperatureKelvin = 1850.0f;
    std::array<float, 3u> artTint{{1.0f, 0.94f, 0.82f}};
    float radius = 0.095f;
    float height = 0.34f;
    float coreRadius = 0.037f;
    float absorption = 4.8f;
    float smokeDensity = 0.16f;
    float emberRate = 0.18f;
    float turbulence = 0.58f;
    float lowPassNoise = 0.0f;
    float leanX = 0.0f;
    float leanZ = 0.0f;
    float motionTurbulence = 0.0f;
    FireEmitterParentObject parentObject = FireEmitterParentObject::OriginalTorch;
    FireEmitterSocket flameSocket = FireEmitterSocket::Flame;
    FireEmitterSocket lightSocket = FireEmitterSocket::Light;
    ShowcaseZone zone = ShowcaseZone::Opening;
    horde::gameplay::items::HeldItemTransform worldFromFlame =
        horde::gameplay::items::IdentityHeldItemTransform();
    horde::gameplay::items::HeldItemTransform worldFromLight =
        horde::gameplay::items::IdentityHeldItemTransform();
    std::array<float, 3u> previousPivotPosition{};
    std::array<float, 3u> previousPivotVelocity{};
    bool motionInitialized = false;
};

struct FireEmitterFixedStepInput
{
    horde::gameplay::items::HeldItemTransform worldFromFlame =
        horde::gameplay::items::IdentityHeldItemTransform();
    horde::gameplay::items::HeldItemTransform worldFromLight =
        horde::gameplay::items::IdentityHeldItemTransform();
    float strength = 1.0f;
    float fuel = 1.0f;
    ShowcaseZone zone = ShowcaseZone::Opening;
};

struct FireEmitterCheckpoint
{
    FireEmitterState state{};
};

FireEmitterState MakeOpeningTorchFireEmitter();
void ResetFireEmitter(FireEmitterState& state);
void StepFireEmitterFixed(FireEmitterState& state,
                          const FireEmitterFixedStepInput& input,
                          float fixedDeltaSeconds);
FireEmitterCheckpoint ExportFireEmitterCheckpoint(const FireEmitterState& state);
void ImportFireEmitterCheckpoint(FireEmitterState& state,
                                 const FireEmitterCheckpoint& checkpoint);

} // namespace horde::gameplay::effects
