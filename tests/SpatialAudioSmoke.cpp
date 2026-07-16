#include <cmath>
#include <iostream>

#include "gameplay/SpatialAudio.h"

namespace
{

bool NearlyEqual(float actual, float expected, float tolerance = 0.0001f)
{
    return std::abs(actual - expected) <= tolerance;
}

float StereoPower(const horde::gameplay::SpatialAudioGains& gains)
{
    return gains.left * gains.left + gains.right * gains.right;
}

} // namespace

int main()
{
    using namespace horde::gameplay;
    bool passed = true;
    const auto check = [&passed](bool condition, const char* message) {
        if (!condition)
        {
            passed = false;
            std::cerr << "Spatial audio smoke failed: " << message << '\n';
        }
    };

    const SpatialAudioListener origin{};
    const SpatialAudioGains centred = CalculateSpatialAudio({0.0f, -1.0f}, origin);
    check(NearlyEqual(centred.pan, 0.0f), "front emitter must remain centred");
    check(NearlyEqual(centred.left, 0.7071067f), "centred emitter must use equal-power left gain");
    check(NearlyEqual(centred.right, 0.7071067f), "centred emitter must use equal-power right gain");

    const SpatialAudioGains right = CalculateSpatialAudio({1.0f, 0.0f}, origin);
    check(NearlyEqual(right.pan, 1.0f) && right.left <= 0.0001f && NearlyEqual(right.right, 1.0f),
          "right emitter must pan fully right");

    const SpatialAudioGains left = CalculateSpatialAudio({-1.0f, 0.0f}, origin);
    check(NearlyEqual(left.pan, -1.0f) && NearlyEqual(left.left, 1.0f) && left.right <= 0.0001f,
          "left emitter must pan fully left");

    const SpatialAudioGains near = CalculateSpatialAudio({0.0f, -1.0f}, origin);
    const SpatialAudioGains far = CalculateSpatialAudio({0.0f, -4.0f}, origin);
    check(StereoPower(far) < StereoPower(near) &&
          NearlyEqual(std::sqrt(StereoPower(far)), 0.25f),
          "inverse-distance rolloff must reduce a four-metre emitter to one quarter");

    const SpatialAudioListener clearListener{0.0f, -2.8f, 0.0f};
    const SpatialAudioGains clear = CalculateSpatialAudio({0.0f, -4.0f}, clearListener);
    const SpatialAudioListener blockedListener{-1.0f, -2.8f, 0.0f};
    const SpatialAudioGains blocked = CalculateSpatialAudio({-1.0f, -4.0f}, blockedListener);
    check(!clear.obstructed && blocked.obstructed &&
          NearlyEqual(std::sqrt(StereoPower(blocked) / StereoPower(clear)), kObstructedAudioGain),
          "route wall obstruction must apply its authored attenuation");

    const SpatialAudioGains outOfRange = CalculateSpatialAudio({0.0f, -15.0f}, origin);
    check(outOfRange.left == 0.0f && outOfRange.right == 0.0f,
          "emitters at maximum range must be silent");

    check(AudioSegmentIntersectsRect(-1.0f, -2.8f, -1.0f, -4.0f, kShowcaseSolidObstacles[1]) &&
          !AudioSegmentIntersectsRect(0.0f, -2.8f, 0.0f, -4.0f, kShowcaseSolidObstacles[1]),
          "segment test must distinguish blocked and open arch lanes");
    check(!IsRouteAudioObstructed(0.0f, -7.0f, 0.0f, -9.4f) &&
          IsRouteAudioObstructed(0.0f, -4.8f, 4.2f, -9.4f),
          "route walls must pass same-leg sound and attenuate sound cutting across a bend");

    PlayerFootstepCadence footsteps;
    const bool stepBeforeInitialDelay = footsteps.Update(0.10f, true);
    const bool initialStep = footsteps.Update(0.06f, true);
    bool stepBeforeInterval = false;
    for (int i = 0; i < 4; ++i)
    {
        stepBeforeInterval = stepBeforeInterval || footsteps.Update(0.10f, true);
    }
    stepBeforeInterval = stepBeforeInterval || footsteps.Update(0.05f, true);
    const bool intervalStep = footsteps.Update(0.01f, true);
    const bool stepWhileStopped = footsteps.Update(0.10f, false);
    const bool stepBeforeRestartDelay = footsteps.Update(0.10f, true) ||
                                        footsteps.Update(0.05f, true);
    const bool restartedStep = footsteps.Update(0.01f, true);
    check(!stepBeforeInitialDelay && initialStep && !stepBeforeInterval && intervalStep &&
          !stepWhileStopped && !stepBeforeRestartDelay && restartedStep,
          "timed cadence must delay, repeat, stop, and restart deterministically");

    TravelFootstepCadence travelFootsteps;
    check(!travelFootsteps.Update(TravelFootstepCadence::kInitialStepDistanceMetres * 0.5f, true) &&
          travelFootsteps.Update(TravelFootstepCadence::kInitialStepDistanceMetres * 0.5f, true) &&
          !travelFootsteps.Update(0.0f, false) &&
          !travelFootsteps.Update(TravelFootstepCadence::kInitialStepDistanceMetres * 0.5f, true),
          "travel cadence must emit from real distance and reset while stationary");

    if (!passed)
    {
        return 1;
    }

    std::cout << "Spatial audio smoke tests passed.\n";
    return 0;
}
