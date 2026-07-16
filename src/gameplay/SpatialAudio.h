#pragma once

#include <algorithm>
#include <cmath>

#include "gameplay/ShowcaseRoute.h"

namespace horde::gameplay
{

struct SpatialAudioEmitter
{
    float x = 0.0f;
    float z = 0.0f;
    float gain = 1.0f;
    float referenceDistance = 1.0f;
    float maximumDistance = 14.0f;
};

struct SpatialAudioListener
{
    float x = 0.0f;
    float z = 0.0f;
    float yawRadians = 0.0f;
};

struct SpatialAudioGains
{
    float left = 0.0f;
    float right = 0.0f;
    float distance = 0.0f;
    float pan = 0.0f;
    bool obstructed = false;
};

constexpr float kObstructedAudioGain = 0.38f;

// Walking starts with a short weight-shift before the first audible contact,
// then settles into a brisk but natural alternating cadence.
class PlayerFootstepCadence
{
public:
    bool Update(float deltaSeconds, bool moving)
    {
        deltaSeconds = std::clamp(deltaSeconds, 0.0f, 0.1f);
        if (!moving)
        {
            Reset();
            return false;
        }
        if (!moving_)
        {
            moving_ = true;
            secondsUntilStep_ = kInitialStepDelaySeconds;
        }
        secondsUntilStep_ -= deltaSeconds;
        if (secondsUntilStep_ <= 0.00001f)
        {
            secondsUntilStep_ += kStepIntervalSeconds;
            return true;
        }
        return false;
    }

    void Reset()
    {
        moving_ = false;
        secondsUntilStep_ = kInitialStepDelaySeconds;
    }

    static constexpr float kInitialStepDelaySeconds = 0.16f;
    static constexpr float kStepIntervalSeconds = 0.46f;

private:
    float secondsUntilStep_ = kInitialStepDelaySeconds;
    bool moving_ = false;
};

// Drive first-person contacts from distance actually travelled after collision
// resolution. This keeps the cadence honest when a key is held into a wall and
// avoids depending on keyboard repeat or render-frame timing.
class TravelFootstepCadence
{
public:
    bool Update(float travelledMetres, bool movementIntended)
    {
        travelledMetres = std::max(0.0f, travelledMetres);
        if (!movementIntended)
        {
            Reset();
            return false;
        }
        if (!moving_)
        {
            moving_ = true;
            metresUntilStep_ = kInitialStepDistanceMetres;
        }
        metresUntilStep_ -= travelledMetres;
        if (metresUntilStep_ <= 0.00001f)
        {
            metresUntilStep_ += kStepIntervalMetres;
            return true;
        }
        return false;
    }

    void Reset()
    {
        moving_ = false;
        metresUntilStep_ = kInitialStepDistanceMetres;
    }

    static constexpr float kInitialStepDistanceMetres = 0.24f;
    static constexpr float kStepIntervalMetres = 0.72f;

private:
    float metresUntilStep_ = kInitialStepDistanceMetres;
    bool moving_ = false;
};

inline bool AudioSegmentIntersectsRect(float startX,
                                       float startZ,
                                       float endX,
                                       float endZ,
                                       const RouteRect& rect)
{
    const float deltaX = endX - startX;
    const float deltaZ = endZ - startZ;
    float entry = 0.0f;
    float exit = 1.0f;

    const auto clipAxis = [&entry, &exit](float start, float delta, float minimum, float maximum)
    {
        if (std::abs(delta) <= 0.000001f)
        {
            return start >= minimum && start <= maximum;
        }
        float nearTime = (minimum - start) / delta;
        float farTime = (maximum - start) / delta;
        if (nearTime > farTime)
        {
            std::swap(nearTime, farTime);
        }
        entry = std::max(entry, nearTime);
        exit = std::min(exit, farTime);
        return entry <= exit;
    };

    return clipAxis(startX, deltaX, rect.minX, rect.maxX) &&
           clipAxis(startZ, deltaZ, rect.minZ, rect.maxZ) &&
           exit >= 0.0f && entry <= 1.0f;
}

inline bool IsRouteAudioObstructed(float listenerX, float listenerZ, float emitterX, float emitterZ)
{
    for (const RouteRect& obstacle : kShowcaseSolidObstacles)
    {
        if (AudioSegmentIntersectsRect(listenerX, listenerZ, emitterX, emitterZ, obstacle))
        {
            return true;
        }
    }

    // The route walls are defined by the boundary of the walkable-rectangle
    // union, not by explicit obstacle rectangles. Sample the short gameplay
    // segment densely enough that a line cutting across any zig-zag return is
    // treated as obstructed while shared portals remain open.
    const float deltaX = emitterX - listenerX;
    const float deltaZ = emitterZ - listenerZ;
    const float distance = std::sqrt(deltaX * deltaX + deltaZ * deltaZ);
    const int steps = std::max(1, static_cast<int>(std::ceil(distance / 0.10f)));
    for (int step = 0; step <= steps; ++step)
    {
        const float t = static_cast<float>(step) / static_cast<float>(steps);
        const float x = listenerX + deltaX * t;
        const float z = listenerZ + deltaZ * t;
        bool insideRoute = false;
        for (const RouteRect& walkable : kShowcaseWalkableRects)
        {
            insideRoute = insideRoute || Contains(walkable, x, z);
        }
        if (!insideRoute)
        {
            return true;
        }
    }
    return false;
}

inline SpatialAudioGains CalculateSpatialAudio(const SpatialAudioEmitter& emitter,
                                                const SpatialAudioListener& listener)
{
    SpatialAudioGains result{};
    const float offsetX = emitter.x - listener.x;
    const float offsetZ = emitter.z - listener.z;
    result.distance = std::sqrt(offsetX * offsetX + offsetZ * offsetZ);
    result.obstructed = IsRouteAudioObstructed(listener.x, listener.z, emitter.x, emitter.z);

    if (emitter.gain <= 0.0f || result.distance >= emitter.maximumDistance)
    {
        return result;
    }

    const float inverseDistance = result.distance > 0.0001f ? 1.0f / result.distance : 0.0f;
    const float rightX = std::cos(listener.yawRadians);
    const float rightZ = std::sin(listener.yawRadians);
    result.pan = result.distance > 0.0001f
        ? std::clamp((offsetX * rightX + offsetZ * rightZ) * inverseDistance, -1.0f, 1.0f)
        : 0.0f;

    const float referenceDistance = std::max(0.01f, emitter.referenceDistance);
    const float rolloffDistance = std::max(0.0f, result.distance - referenceDistance);
    const float distanceGain = 1.0f / (1.0f + rolloffDistance / referenceDistance);
    const float obstructionGain = result.obstructed ? kObstructedAudioGain : 1.0f;
    const float totalGain = std::clamp(emitter.gain * distanceGain * obstructionGain, 0.0f, 1.0f);

    constexpr float kQuarterPi = 0.78539816339f;
    const float panAngle = (result.pan + 1.0f) * kQuarterPi;
    result.left = std::clamp(totalGain * std::cos(panAngle), 0.0f, 1.0f);
    result.right = std::clamp(totalGain * std::sin(panAngle), 0.0f, 1.0f);
    return result;
}

} // namespace horde::gameplay
