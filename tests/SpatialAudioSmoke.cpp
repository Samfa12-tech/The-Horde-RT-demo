#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "gameplay/FeedbackTiming.h"
#include "gameplay/SpatialAudio.h"
#include "gameplay/simulation/GameplayEvent.h"

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

std::filesystem::path FindRepoRoot()
{
    std::filesystem::path candidate = std::filesystem::current_path();
    for (int depth = 0; depth < 8; ++depth)
    {
        if (std::filesystem::exists(candidate / "CMakeLists.txt") &&
            std::filesystem::exists(candidate / "android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java"))
        {
            return candidate;
        }
        if (!candidate.has_parent_path())
        {
            break;
        }
        candidate = candidate.parent_path();
    }
    return {};
}

std::string ReadTextFile(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    std::ostringstream text;
    text << input.rdbuf();
    return text.str();
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

    horde::gameplay::simulation::GameplayEvent earlierEvent;
    earlierEvent.worldX = 2.0f;
    earlierEvent.worldZ = -2.0f;
    earlierEvent.listenerX = 0.0f;
    earlierEvent.listenerZ = -2.0f;
    earlierEvent.listenerYawRadians = 0.0f;
    const SpatialAudioGains earlierEventGains = CalculateSpatialAudio(
        {earlierEvent.worldX, earlierEvent.worldZ, 1.0f, 1.0f, 14.0f},
        {earlierEvent.listenerX, earlierEvent.listenerZ, earlierEvent.listenerYawRadians});
    horde::gameplay::simulation::GameplayEvent laterEvent = earlierEvent;
    laterEvent.listenerX = 1.0f;
    laterEvent.listenerYawRadians = 3.14159265359f;
    const SpatialAudioGains laterEventGains = CalculateSpatialAudio(
        {laterEvent.worldX, laterEvent.worldZ, 1.0f, 1.0f, 14.0f},
        {laterEvent.listenerX, laterEvent.listenerZ, laterEvent.listenerYawRadians});
    const SpatialAudioGains repeatedLaterEventGains = CalculateSpatialAudio(
        {laterEvent.worldX, laterEvent.worldZ, 1.0f, 1.0f, 14.0f},
        {laterEvent.listenerX, laterEvent.listenerZ, laterEvent.listenerYawRadians});
    check(earlierEventGains.pan > 0.99f && laterEventGains.pan < -0.99f &&
          StereoPower(laterEventGains) > StereoPower(earlierEventGains) &&
          NearlyEqual(laterEventGains.left, repeatedLaterEventGains.left) &&
          NearlyEqual(laterEventGains.right, repeatedLaterEventGains.right) &&
          NearlyEqual(earlierEvent.worldX, laterEvent.worldX) &&
          NearlyEqual(earlierEvent.worldZ, laterEvent.worldZ),
          "event-time listener position/yaw must deterministically change pan and distance without changing the source");

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

    DelayedGameplayFeedbackQueue delayedFeedback;
    horde::gameplay::simulation::GameplayEvent fallA;
    fallA.sequence = 41u;
    fallA.type = horde::gameplay::simulation::GameplayEventType::EnemyDefeated;
    fallA.source = horde::gameplay::simulation::EntityId::Player;
    fallA.target = horde::gameplay::simulation::EntityId::SkeletonA;
    fallA.worldX = -0.75f;
    fallA.listenerX = 1.0f;
    horde::gameplay::simulation::GameplayEvent fallB = fallA;
    fallB.sequence = 42u;
    fallB.target = horde::gameplay::simulation::EntityId::SkeletonB;
    fallB.worldX = 0.75f;
    check(delayedFeedback.Enqueue(fallA, 1000u + kEnemyImpactFallDelayMilliseconds) &&
          delayedFeedback.Enqueue(fallB, 1001u + kEnemyImpactFallDelayMilliseconds) &&
          delayedFeedback.HighWaterMark() == 2u,
          "enemy fall cues must enter the bounded delayed queue in event order");
    std::vector<horde::gameplay::simulation::GameplayEvent> playedFalls;
    check(delayedFeedback.DrainDue(1139u, [&playedFalls](const auto& event) { playedFalls.push_back(event); }) == 0u &&
          delayedFeedback.Size() == 2u,
          "enemy fall feedback must not play before the authored 140 ms boundary");
    check(delayedFeedback.DrainDue(1140u, [&playedFalls](const auto& event) { playedFalls.push_back(event); }) == 1u &&
          delayedFeedback.Size() == 1u && playedFalls.size() == 1u &&
          playedFalls[0].sequence == 41u && playedFalls[0].target == horde::gameplay::simulation::EntityId::SkeletonA &&
          NearlyEqual(playedFalls[0].worldX, -0.75f) && NearlyEqual(playedFalls[0].listenerX, 1.0f),
          "due fall feedback must retain exact ordered entity/source/listener event data");
    check(delayedFeedback.DrainDue(1141u, [&playedFalls](const auto& event) { playedFalls.push_back(event); }) == 1u &&
          delayedFeedback.Size() == 0u && playedFalls.size() == 2u && playedFalls[1].sequence == 42u,
          "repeated same-type fall events must remain distinct through delayed playback");

    DelayedGameplayFeedbackQueue saturatedFeedback;
    for (std::size_t index = 0u; index < DelayedGameplayFeedbackQueue::kCapacity; ++index)
    {
        fallA.sequence = static_cast<std::uint64_t>(index + 1u);
        check(saturatedFeedback.Enqueue(fallA, 2000u),
              "delayed feedback queue rejected an event before reaching capacity");
    }
    fallA.sequence = 999u;
    check(!saturatedFeedback.Enqueue(fallA, 2000u) &&
          saturatedFeedback.Size() == DelayedGameplayFeedbackQueue::kCapacity &&
          saturatedFeedback.OverflowCount() == 1u,
          "delayed feedback overflow must be visible and must not overwrite an existing cue");
    std::vector<std::uint64_t> saturatedSequences;
    saturatedFeedback.DrainDue(2000u, [&saturatedSequences](const auto& event)
    {
        saturatedSequences.push_back(event.sequence);
    });
    bool orderedSaturation = saturatedSequences.size() == DelayedGameplayFeedbackQueue::kCapacity;
    for (std::size_t index = 0u; index < saturatedSequences.size(); ++index)
    {
        orderedSaturation = orderedSaturation && saturatedSequences[index] == index + 1u;
    }
    check(orderedSaturation,
          "delayed feedback overflow must preserve every earlier queued cue in order");
    check(saturatedFeedback.OverflowCount() == 1u &&
          saturatedFeedback.HighWaterMark() == DelayedGameplayFeedbackQueue::kCapacity,
          "draining delayed feedback must preserve permanent overflow diagnostics");

    DelayedGameplayFeedbackQueue cancelledFeedback;
    check(cancelledFeedback.Enqueue(fallA, 3000u),
          "delayed feedback setup must accept a pending cue");
    cancelledFeedback.Clear();
    check(cancelledFeedback.DrainDue(3000u, [](const auto&) {}) == 0u &&
          cancelledFeedback.Size() == 0u,
          "route reset and retry must be able to cancel stale delayed feedback");

    const std::filesystem::path root = FindRepoRoot();
    check(!root.empty(), "platform feedback sources were not found");
    if (!root.empty())
    {
        const std::string windowsSource =
            ReadTextFile(root / "src/platform/windows/DiagnosticWindow.cpp");
        const std::string androidSource = ReadTextFile(
            root / "android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java");
        const std::string androidBridgeSource =
            ReadTextFile(root / "android/app/src/main/cpp/android_probe_bridge.cpp");
        check(windowsSource.find("case GameplayEventType::EnemyDefeated:") != std::string::npos &&
              windowsSource.find("context.delayedFeedback.Enqueue(") != std::string::npos &&
              windowsSource.find("PlayPositionalSoundEffect(context, \"enemy_fall.wav\"") != std::string::npos &&
              windowsSource.find("PlayPositionalSoundEffect(context, \"sword_hit_1.wav\"") != std::string::npos,
              "Windows must retain positional skeleton hit and delayed fall mappings");
        check(androidSource.find("case PLATFORM_EVENT_ENEMY_DEFEATED:") != std::string::npos &&
              androidSource.find("ENEMY_IMPACT_FALL_DELAY_MILLISECONDS") != std::string::npos &&
              androidSource.find("feedbackGeneration == delayedGameplayFeedbackGeneration") != std::string::npos,
              "Android must retain the authored fall delay and cancel stale lifecycle feedback");
        check(androidBridgeSource.find("BoundedTransportQueue<") != std::string::npos &&
              androidBridgeSource.find("gPlatformGameplayEvents.Push(") != std::string::npos &&
              androidBridgeSource.find("PlatformGameplayEventOverflowCount()") != std::string::npos,
              "Android must retain bounded ordered event transport with visible overflow");
    }

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
