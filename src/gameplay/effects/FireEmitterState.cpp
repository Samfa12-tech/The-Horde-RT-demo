#include "gameplay/effects/FireEmitterState.h"

#include <algorithm>
#include <cmath>

namespace horde::gameplay::effects
{
namespace
{

using Vec3 = std::array<float, 3u>;

float FiniteOr(const float value, const float fallback)
{
    return std::isfinite(value) ? value : fallback;
}

Vec3 TranslationOf(const horde::gameplay::items::HeldItemTransform& transform)
{
    return {{transform[12], transform[13], transform[14]}};
}

Vec3 ColumnOf(const horde::gameplay::items::HeldItemTransform& transform,
              const std::size_t column)
{
    return {{transform[column * 4u], transform[column * 4u + 1u],
             transform[column * 4u + 2u]}};
}

Vec3 Subtract(const Vec3& left, const Vec3& right)
{
    return {{left[0] - right[0], left[1] - right[1], left[2] - right[2]}};
}

Vec3 Scale(const Vec3& value, const float scale)
{
    return {{value[0] * scale, value[1] * scale, value[2] * scale}};
}

float Dot(const Vec3& left, const Vec3& right)
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

float Length(const Vec3& value)
{
    return std::sqrt(Dot(value, value));
}

Vec3 ClampLength(const Vec3& value, const float maximum)
{
    const float length = Length(value);
    return length > maximum && length > 0.000001f
        ? Scale(value, maximum / length)
        : value;
}

std::uint32_t Hash(std::uint32_t value)
{
    value ^= value >> 16u;
    value *= 0x7feb352du;
    value ^= value >> 15u;
    value *= 0x846ca68bu;
    value ^= value >> 16u;
    return value;
}

float SignedHash(const std::uint32_t seed, const std::uint32_t lattice)
{
    const std::uint32_t bits = Hash(seed ^ (lattice * 0x9e3779b9u));
    return static_cast<float>(bits & 0x00ffffffu) * (2.0f / 16777215.0f) - 1.0f;
}

float ValueNoise(const std::uint32_t seed, const float phase)
{
    const float domain = phase * 7.0f;
    const float floorValue = std::floor(domain);
    const std::uint32_t lattice = static_cast<std::uint32_t>(floorValue);
    float fraction = domain - floorValue;
    fraction = fraction * fraction * (3.0f - 2.0f * fraction);
    const float first = SignedHash(seed, lattice);
    const float second = SignedHash(seed, lattice + 1u);
    return first + (second - first) * fraction;
}

void SanitizeAuthoredState(FireEmitterState& state)
{
    state.stableId = std::max(1u, state.stableId);
    state.strength = std::clamp(FiniteOr(state.strength, 0.0f), 0.0f, 1.0f);
    state.fuel = std::clamp(FiniteOr(state.fuel, 0.0f), 0.0f, 1.0f);
    state.phase = std::clamp(FiniteOr(state.phase, 0.0f), 0.0f, 0.999999f);
    state.colourTemperatureKelvin = std::clamp(
        FiniteOr(state.colourTemperatureKelvin, 1850.0f), 1000.0f, 4000.0f);
    for (float& channel : state.artTint)
        channel = std::clamp(FiniteOr(channel, 1.0f), 0.0f, 2.0f);
    state.radius = std::clamp(FiniteOr(state.radius, 0.095f), 0.01f, 0.45f);
    state.height = std::clamp(FiniteOr(state.height, 0.34f), 0.04f, 1.20f);
    state.coreRadius = std::clamp(FiniteOr(state.coreRadius, 0.037f),
                                  0.005f, state.radius);
    state.absorption = std::clamp(FiniteOr(state.absorption, 4.8f), 0.1f, 12.0f);
    state.smokeDensity = std::clamp(FiniteOr(state.smokeDensity, 0.16f), 0.0f, 1.0f);
    state.emberRate = std::clamp(FiniteOr(state.emberRate, 0.18f), 0.0f, 1.0f);
    state.turbulence = std::clamp(FiniteOr(state.turbulence, 0.58f), 0.0f, 1.0f);
    state.lowPassNoise = std::clamp(FiniteOr(state.lowPassNoise, 0.0f), -1.0f, 1.0f);
    state.leanX = std::clamp(FiniteOr(state.leanX, 0.0f), -0.32f, 0.32f);
    state.leanZ = std::clamp(FiniteOr(state.leanZ, 0.0f), -0.32f, 0.32f);
    state.motionTurbulence = std::clamp(
        FiniteOr(state.motionTurbulence, 0.0f), 0.0f, 1.0f);
}

} // namespace

FireEmitterState MakeOpeningTorchFireEmitter()
{
    return {};
}

void ResetFireEmitter(FireEmitterState& state)
{
    const std::uint32_t stableId = state.stableId;
    const std::uint32_t seed = state.seed;
    const FireEmitterParentObject parentObject = state.parentObject;
    const FireEmitterSocket flameSocket = state.flameSocket;
    const FireEmitterSocket lightSocket = state.lightSocket;
    const float temperature = state.colourTemperatureKelvin;
    const auto tint = state.artTint;
    const float radius = state.radius;
    const float height = state.height;
    const float coreRadius = state.coreRadius;
    const float absorption = state.absorption;
    const float smokeDensity = state.smokeDensity;
    const float emberRate = state.emberRate;
    const float turbulence = state.turbulence;
    state = {};
    state.stableId = stableId;
    state.seed = seed;
    state.parentObject = parentObject;
    state.flameSocket = flameSocket;
    state.lightSocket = lightSocket;
    state.colourTemperatureKelvin = temperature;
    state.artTint = tint;
    state.radius = radius;
    state.height = height;
    state.coreRadius = coreRadius;
    state.absorption = absorption;
    state.smokeDensity = smokeDensity;
    state.emberRate = emberRate;
    state.turbulence = turbulence;
    SanitizeAuthoredState(state);
}

void StepFireEmitterFixed(FireEmitterState& state,
                          const FireEmitterFixedStepInput& input,
                          float fixedDeltaSeconds)
{
    fixedDeltaSeconds = std::clamp(FiniteOr(fixedDeltaSeconds, 0.0f), 0.0f, 0.05f);
    state.worldFromFlame = input.worldFromFlame;
    state.worldFromLight = input.worldFromLight;
    state.strength = input.strength;
    state.fuel = input.fuel;
    state.zone = input.zone;
    SanitizeAuthoredState(state);

    if (fixedDeltaSeconds <= 0.0f)
    {
        state.previousPivotPosition = TranslationOf(state.worldFromFlame);
        state.previousPivotVelocity = {};
        state.motionInitialized = true;
        return;
    }

    state.phase += fixedDeltaSeconds * 0.37f;
    state.phase -= std::floor(state.phase);
    const float targetNoise = ValueNoise(state.seed, state.phase);
    const float noiseBlend = std::clamp(fixedDeltaSeconds * 5.5f, 0.0f, 1.0f);
    state.lowPassNoise += (targetNoise - state.lowPassNoise) * noiseBlend;

    const Vec3 pivot = TranslationOf(state.worldFromFlame);
    if (!state.motionInitialized)
    {
        state.previousPivotPosition = pivot;
        state.previousPivotVelocity = {};
        state.motionInitialized = true;
        return;
    }
    const Vec3 velocity = ClampLength(
        Scale(Subtract(pivot, state.previousPivotPosition), 1.0f / fixedDeltaSeconds), 8.0f);
    const Vec3 acceleration = ClampLength(
        Scale(Subtract(velocity, state.previousPivotVelocity), 1.0f / fixedDeltaSeconds), 24.0f);
    const Vec3 localX = ColumnOf(state.worldFromFlame, 0u);
    const Vec3 localZ = ColumnOf(state.worldFromFlame, 2u);
    const float targetLeanX = std::clamp(-Dot(acceleration, localX) * 0.012f, -0.28f, 0.28f);
    const float targetLeanZ = std::clamp(-Dot(acceleration, localZ) * 0.012f, -0.28f, 0.28f);
    const float targetMotionTurbulence = std::clamp(Length(acceleration) / 18.0f, 0.0f, 1.0f);
    const float leanBlend = std::clamp(fixedDeltaSeconds * 9.0f, 0.0f, 1.0f);
    const float turbulenceBlend = std::clamp(fixedDeltaSeconds * 6.0f, 0.0f, 1.0f);
    state.leanX += (targetLeanX - state.leanX) * leanBlend;
    state.leanZ += (targetLeanZ - state.leanZ) * leanBlend;
    state.motionTurbulence +=
        (targetMotionTurbulence - state.motionTurbulence) * turbulenceBlend;
    state.previousPivotPosition = pivot;
    state.previousPivotVelocity = velocity;
    SanitizeAuthoredState(state);
}

FireEmitterCheckpoint ExportFireEmitterCheckpoint(const FireEmitterState& state)
{
    return {state};
}

void ImportFireEmitterCheckpoint(FireEmitterState& state,
                                 const FireEmitterCheckpoint& checkpoint)
{
    state = checkpoint.state;
    SanitizeAuthoredState(state);
}

} // namespace horde::gameplay::effects
