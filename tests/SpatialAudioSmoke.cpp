#include <cmath>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "gameplay/FeedbackTiming.h"
#include "gameplay/EquipmentFeedback.h"
#include "platform/android/GameplayEventMetadata.h"
#include "gameplay/SpatialAudio.h"
#include "gameplay/simulation/BoundedTransportQueue.h"
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
    using horde::gameplay::simulation::GameplayEvent;
    using horde::gameplay::simulation::GameplayEventType;
    using horde::gameplay::simulation::EntityId;
    for (const auto [type, expectedId] : {
             std::pair{GameplayEventType::ParryPrepareCue, 24u},
             std::pair{GameplayEventType::LichDischargeWarning, 25u}})
    {
        GameplayEvent warning{};
        warning.type = type;
        check((horde::platform::android::PackGameplayEventMetadata(warning) & 0xffu) == expectedId,
              "appended combat teaching event keeps its compact Android type byte");
    }
    check(NearlyEqual(GameplayEvent{}.listenerY, kShowcaseEyeWorldY),
          "event listener Y must default to the showcase eye baseline");
    const GameplayEvent legacyEventAggregate{
        41u, 7u, GameplayEventType::EnemyHit, EntityId::Player, EntityId::SkeletonA,
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 0.25f, 0.5f, 9};
    check(legacyEventAggregate.payload == 9 &&
          NearlyEqual(legacyEventAggregate.worldY, 2.0f) &&
          NearlyEqual(legacyEventAggregate.listenerYawRadians, 0.25f) &&
          NearlyEqual(legacyEventAggregate.listenerY, kShowcaseEyeWorldY),
          "appended listener Y must preserve legacy positional event aggregate fields and default");
    GameplayEvent equipmentEvent{};
    equipmentEvent.type = GameplayEventType::PlayerSwordDrawStarted;
    equipmentEvent.source = EntityId::Player;
    equipmentEvent.target = EntityId::Invalid;
    equipmentEvent.sequence = 0x1abcdef01ULL;
    check(EquipmentCueForEvent(equipmentEvent) == EquipmentAudioCue::None,
          "draw input/start must not duplicate the attachment cue");
    equipmentEvent.type = GameplayEventType::PlayerSwordAttachmentChanged;
    equipmentEvent.payload = static_cast<std::int32_t>(items::HeldItemParentMode::HandSocket);
    check(EquipmentCueForEvent(equipmentEvent) == EquipmentAudioCue::SwordDraw,
          "physical hand attachment must select the draw cue");
    const auto metadata = horde::platform::android::PackGameplayEventMetadata(equipmentEvent);
    check((metadata & 0xffu) == static_cast<std::uint64_t>(equipmentEvent.type) &&
          ((metadata >> 8u) & 0xffu) == static_cast<std::uint64_t>(EntityId::Player) &&
          ((metadata >> 16u) & 0xffu) == static_cast<std::uint64_t>(EntityId::Invalid) &&
          ((metadata >> 24u) & 0xffu) == static_cast<std::uint64_t>(EquipmentAudioCue::SwordDraw) &&
          (metadata >> 32u) == 0xabcdef01u,
          "equipment transport must preserve existing metadata and sequence fields");
    equipmentEvent.payload = static_cast<std::int32_t>(items::HeldItemParentMode::BodyStow);
    check(EquipmentCueForEvent(equipmentEvent) == EquipmentAudioCue::SwordSheath,
          "physical body attachment must select the sheath cue");
    equipmentEvent.payload = static_cast<std::int32_t>(items::HeldItemParentMode::WorldObject);
    check(EquipmentCueForEvent(equipmentEvent) == EquipmentAudioCue::None,
          "world detach must not invent a sheath cue");
    equipmentEvent.source = EntityId::SkeletonA;
    equipmentEvent.payload = static_cast<std::int32_t>(items::HeldItemParentMode::HandSocket);
    check(EquipmentCueForEvent(equipmentEvent) == EquipmentAudioCue::None,
          "non-player attachment must not play player equipment feedback");
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

    const SpatialAudioGains zeroVerticalOffset = CalculateSpatialAudio(
        {0.0f, -1.0f, 1.0f, 1.0f, 14.0f, 0.70f},
        {0.0f, 0.0f, 0.0f, 0.70f});
    const SpatialAudioGains nonzeroVerticalOffset = CalculateSpatialAudio(
        {0.0f, -1.0f, 1.0f, 1.0f, 14.0f, -3.0f},
        {0.0f, 0.0f, 0.0f, 0.70f});
    check(NearlyEqual(zeroVerticalOffset.left, nonzeroVerticalOffset.left) &&
          NearlyEqual(zeroVerticalOffset.right, nonzeroVerticalOffset.right) &&
          NearlyEqual(zeroVerticalOffset.distance, nonzeroVerticalOffset.distance) &&
          NearlyEqual(zeroVerticalOffset.pan, nonzeroVerticalOffset.pan) &&
          zeroVerticalOffset.obstructed == nonzeroVerticalOffset.obstructed,
          "Y metadata must preserve the current planar distance, pan, obstruction and stereo gains");

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
    for (const RoutePosition& stand : kKeeperTorchStandCenters)
    {
        // These floor bases are 0.13 m tall. Ear-height sound crossing their
        // footprints must retain the existing clear-room mix, not acquire the
        // wall attenuation of the route's heightless masonry approximation.
        check(!IsRouteAudioObstructed(stand.x - 0.60f, stand.z,
                                     stand.x + 0.60f, stand.z),
              "low Keeper stand bases must not become full-height acoustic walls");
        const SpatialAudioGains standCue = CalculateSpatialAudio(
            {stand.x, stand.z, 0.70f, 1.0f, 14.0f},
            {stand.x + 0.80f, stand.z, -1.57079632679f});
        check(!standCue.obstructed && StereoPower(standCue) > 0.0f,
              "Keeper stand sound must not self-occlude at its base footprint");
    }
    const SpatialAudioGains chestUnlockAtInteraction = CalculateSpatialAudio(
        {kRewardChestRoutePosition.x, kRewardChestRoutePosition.z,
         0.70f, 1.0f, 14.0f},
        {kRewardChestRoutePosition.x + 1.30f,
         kRewardChestRoutePosition.z, -1.57079632679f});
    check(!chestUnlockAtInteraction.obstructed &&
              StereoPower(chestUnlockAtInteraction) > 0.0f,
          "the physical chest collider must not self-occlude its positional unlock cue at the interaction stand-off");

    DelayedGameplayFeedbackQueue delayedFeedback;
    horde::gameplay::simulation::GameplayEvent fallA;
    fallA.sequence = 41u;
    fallA.type = horde::gameplay::simulation::GameplayEventType::EnemyDefeated;
    fallA.source = horde::gameplay::simulation::EntityId::Player;
    fallA.target = horde::gameplay::simulation::EntityId::SkeletonA;
    fallA.worldX = -0.75f;
    fallA.worldY = -0.35f;
    fallA.listenerX = 1.0f;
    fallA.listenerY = 0.57f;
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
          NearlyEqual(playedFalls[0].worldX, -0.75f) && NearlyEqual(playedFalls[0].worldY, -0.35f) &&
          NearlyEqual(playedFalls[0].listenerX, 1.0f) && NearlyEqual(playedFalls[0].listenerY, 0.57f),
          "due fall feedback must retain exact ordered entity/source/listener tuple including both heights");
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

    fallA.worldY = -1.125f;
    fallA.listenerY = 0.625f;
    const std::uint64_t verticalMetadata =
        horde::platform::android::PackGameplayEventVerticalMetadata(fallA);
    check(std::bit_cast<float>(static_cast<std::uint32_t>(verticalMetadata)) == -1.125f &&
          std::bit_cast<float>(static_cast<std::uint32_t>(verticalMetadata >> 32u)) == 0.625f,
          "Android vertical tuple word must preserve sourceY and listenerY bit-exactly");

    struct CompactGameplayEventTuple
    {
        std::uint64_t eventMetadata = 0u;
        std::uint64_t stereoGains = 0u;
        std::uint64_t verticalMetadata = 0u;
    };
    horde::gameplay::simulation::BoundedTransportQueue<CompactGameplayEventTuple, 128u>
        compactGameplayEvents;
    bool compactAccepted = true;
    for (std::size_t index = 0u; index < 128u; ++index)
    {
        fallA.sequence = index + 1u;
        fallA.worldY = static_cast<float>(index) * 0.01f;
        fallA.listenerY = 0.70f + static_cast<float>(index) * 0.001f;
        compactAccepted = compactAccepted && compactGameplayEvents.Push({
            horde::platform::android::PackGameplayEventMetadata(fallA),
            index,
            horde::platform::android::PackGameplayEventVerticalMetadata(fallA)});
    }
    const CompactGameplayEventTuple lastAcceptedTuple = compactGameplayEvents[127u];
    bool compactOrderingPreserved = compactGameplayEvents.Size() == 128u;
    for (std::size_t index = 0u; index < compactGameplayEvents.Size(); ++index)
    {
        const CompactGameplayEventTuple& tuple = compactGameplayEvents[index];
        compactOrderingPreserved = compactOrderingPreserved &&
            (tuple.eventMetadata >> 32u) == index + 1u &&
            std::bit_cast<float>(static_cast<std::uint32_t>(tuple.verticalMetadata)) ==
                static_cast<float>(index) * 0.01f &&
            std::bit_cast<float>(static_cast<std::uint32_t>(tuple.verticalMetadata >> 32u)) ==
                0.70f + static_cast<float>(index) * 0.001f;
    }
    fallA.sequence = 999u;
    check(compactAccepted && !compactGameplayEvents.Push({
              horde::platform::android::PackGameplayEventMetadata(fallA), 999u,
              horde::platform::android::PackGameplayEventVerticalMetadata(fallA)}) &&
          compactGameplayEvents.Size() == 128u && compactGameplayEvents.OverflowCount() == 1u &&
          compactOrderingPreserved &&
          (lastAcceptedTuple.eventMetadata >> 32u) == 128u &&
          std::bit_cast<float>(static_cast<std::uint32_t>(lastAcceptedTuple.verticalMetadata)) ==
              127.0f * 0.01f &&
          std::bit_cast<float>(static_cast<std::uint32_t>(lastAcceptedTuple.verticalMetadata >> 32u)) ==
              0.70f + 127.0f * 0.001f,
          "Android 128-entry compact tuple queue must drop only the newest and retain earlier ordered Y metadata");

    DelayedGameplayFeedbackQueue cancelledFeedback;
    check(cancelledFeedback.Enqueue(fallA, 3000u),
          "delayed feedback setup must accept a pending cue");
    cancelledFeedback.Clear();
    check(cancelledFeedback.DrainDue(3000u, [](const auto&) {}) == 0u &&
          cancelledFeedback.Size() == 0u,
          "route reset and retry must be able to cancel stale delayed feedback");

    horde::gameplay::simulation::BoundedTransportQueue<
        horde::gameplay::simulation::GameplayEvent, 4u> lifecycleFeedback;
    horde::gameplay::simulation::GameplayEvent pendingUnlock;
    pendingUnlock.type = horde::gameplay::simulation::GameplayEventType::ChestUnlocked;
    pendingUnlock.sequence = 73u;
    check(lifecycleFeedback.Push(pendingUnlock),
          "lifecycle fixture must queue a pending chest unlock cue");
    lifecycleFeedback.Clear();
    check(lifecycleFeedback.Size() == 0u && lifecycleFeedback.Values().empty(),
          "Home teardown must discard a pending chest unlock cue rather than replay it after resume");

    const std::filesystem::path root = FindRepoRoot();
    check(!root.empty(), "platform feedback sources were not found");
    if (!root.empty())
    {
        const std::string windowsSource =
            ReadTextFile(root / "src/platform/windows/DiagnosticWindow.cpp");
        const std::string androidSource = ReadTextFile(
            root / "android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java");
        const std::string androidBuildSource = ReadTextFile(root / "android/app/build.gradle");
        const std::string androidBridgeSource =
            ReadTextFile(root / "android/app/src/main/cpp/android_probe_bridge.cpp");
        const std::string ambienceWorker = ReadTextFile(
            root / "android/app/src/main/java/com/samfa12/hordelanternrt/HordeAmbiencePlayback.java");
        const std::string ambienceGate = ReadTextFile(
            root / "android/app/src/main/java/com/samfa12/hordelanternrt/AmbienceOutputGate.java");
        const std::string ambienceCore = ReadTextFile(root / "src/audio/AmbiencePcmLoop.cpp");
        const auto section = [](const std::string& source, const char* begin, const char* end) {
            const auto start = source.find(begin);
            const auto stop = start == std::string::npos ? std::string::npos : source.find(end, start);
            return start == std::string::npos || stop == std::string::npos
                ? std::string{} : source.substr(start, stop - start);
        };
        check(windowsSource.find("masteringVoice_->SetVolume(") != std::string::npos &&
              windowsSource.find("SfxVolumeLinearGain(percent)") != std::string::npos &&
              windowsSource.find("engine.SetMasterVolumePercent(sfxVolumePercent)") != std::string::npos &&
              windowsSource.find("engine.SetMasterVolumePercent(context.sfxVolumePercent)") != std::string::npos &&
              windowsSource.find("PlaySoundA(") == std::string::npos,
              "Windows centered/positional/loop SFX must use independent master gain without an unscaled fallback");
        check(windowsSource.find("14.0f, event.worldY}") != std::string::npos &&
              windowsSource.find("event.listenerYawRadians, event.listenerY}") != std::string::npos &&
              androidBridgeSource.find("14.0f, event.worldY}") != std::string::npos &&
              androidBridgeSource.find("event.listenerYawRadians, event.listenerY}") != std::string::npos,
              "both native positional-audio consumers must pass source and event-time listener Y metadata");
        check(windowsSource.find("PlayAmbientSoundEffect(context, clip, horde::audio::kPlayerFootstepCueGain)") !=
                  std::string::npos &&
              windowsSource.find("event.intensity * horde::audio::kWetFootstepCueGain") !=
                  std::string::npos &&
              windowsSource.find("water_wet_step_3.wav") != std::string::npos &&
              windowsSource.find("water_wet_step_4.wav") != std::string::npos &&
              windowsSource.find("GetPrivateProfileIntA(\"audio\", \"sfxVolume\"") != std::string::npos &&
              windowsSource.find("WritePrivateProfileStringA(\"audio\", \"sfxVolume\"") != std::string::npos &&
              windowsSource.find("kSfxVolumeSliderId") != std::string::npos &&
              windowsSource.find("kSfxButtonId") == std::string::npos,
              "Windows must persist a separate SFX slider and quiet player footsteps without another primary toggle");
        check(androidSource.find("\"player_step_1\" : \"player_step_2\", PLAYER_DRY_FOOTSTEP_GAIN,") !=
                  std::string::npos &&
              androidSource.find("PLAYER_DRY_FOOTSTEP_GAIN = 0.29f") != std::string::npos &&
              androidSource.find("PLAYER_WET_FOOTSTEP_GAIN = 1.0f") != std::string::npos &&
              androidSource.find("wetFootstepSoundKey(playerStepVariant++)") != std::string::npos &&
              androidSource.find("case 2: return \"water_wet_step_3\"") != std::string::npos &&
              androidSource.find("default: return \"water_wet_step_4\"") != std::string::npos &&
              androidBuildSource.find("include 'audio/pixabay/water_wet_step_3_core.wav'") != std::string::npos &&
              androidBuildSource.find("include 'audio/pixabay/water_wet_step_4_core.wav'") != std::string::npos &&
              androidSource.find("stereoGains, verticalMetadata)") != std::string::npos &&
              androidSource.find("addSlider(panel, getString(R.string.sfx_volume)") != std::string::npos &&
              androidSource.find("preferences.edit().putInt(\"sfx_volume\", value).apply()") != std::string::npos,
              "Android must preserve its independent SFX slider and quieter steps with unchanged event-time stereo gains");
        check(windowsSource.find("case GameplayEventType::EnemyDefeated:") != std::string::npos &&
              windowsSource.find("context.delayedFeedback.Enqueue(") != std::string::npos &&
              windowsSource.find("context.delayedFeedback.DrainDue(GetTickCount64()") != std::string::npos &&
              windowsSource.find("PlayPositionalSoundEffect(context, \"skeleton_falling_bones.wav\", 0.36f, event, \"pixabay\")") != std::string::npos &&
              windowsSource.find("GetTickCount64() + horde::gameplay::kEnemyImpactFallDelayMilliseconds") != std::string::npos &&
              section(windowsSource, "void ResetRoute(", "bool ApplyPlayerRetryCheckpoint(")
                  .find("context.delayedFeedback.Clear();") != std::string::npos &&
              section(windowsSource, "bool ApplyPlayerRetryCheckpoint(", "const char* PresentModeName(")
                  .find("context.delayedFeedback.Clear();") != std::string::npos &&
              windowsSource.find("PlayPositionalSoundEffect(context, \"sword_hit_1.wav\"") != std::string::npos,
              "Windows must retain positional sword impact, ordered licensed bones fall, 140 ms scheduling and reset/retry cancellation");
        check(androidSource.find("case PLATFORM_EVENT_ENEMY_DEFEATED:") != std::string::npos &&
              androidSource.find("ENEMY_IMPACT_FALL_DELAY_MILLISECONDS") != std::string::npos &&
              androidSource.find("ENEMY_IMPACT_FALL_DELAY_MILLISECONDS = 140L") != std::string::npos &&
              androidSource.find("feedbackGeneration == delayedGameplayFeedbackGeneration") != std::string::npos &&
              androidSource.find("playSpatialSound(\"skeleton_falling_bones\", 0.24f,") != std::string::npos &&
              androidSource.find("stereoGains, verticalMetadata);") != std::string::npos,
              "Android must retain the authored fall delay and cancel stale lifecycle feedback");
        check(androidBridgeSource.find("BoundedTransportQueue<") != std::string::npos &&
              androidBridgeSource.find("gPlatformGameplayEvents.Push(") != std::string::npos &&
              androidBridgeSource.find("PlatformGameplayEventOverflowCount()") != std::string::npos &&
              androidBridgeSource.find("event.verticalMetadata") != std::string::npos &&
              androidBridgeSource.find("Size() * 3u") != std::string::npos &&
              androidSource.find("eventIndex += 3") != std::string::npos &&
              androidSource.find("platformEvents[eventIndex + 2]") != std::string::npos,
              "Android must retain bounded ordered event transport with visible overflow");
        const std::size_t androidStopBegin =
            androidBridgeSource.find("void StopSurfaceInternal()");
        const std::size_t androidStopEnd =
            androidBridgeSource.find("} // namespace", androidStopBegin);
        check(androidStopBegin != std::string::npos && androidStopEnd != std::string::npos &&
                  androidBridgeSource.substr(androidStopBegin,
                                             androidStopEnd - androidStopBegin)
                          .find("ClearPlatformGameplayEvents();") != std::string::npos &&
                  androidSource.find("if (resumed && surfaceStarted && state == 1)") !=
                      std::string::npos,
              "Android Home teardown must clear queued immediate cues and runtime polling must dispatch only for the resumed active surface");
        check(windowsSource.find("case GameplayEventType::ChestUnlocked:") != std::string::npos &&
              windowsSource.find("PlayPositionalSoundEffect(context, \"chest_unlock.wav\"") != std::string::npos &&
              windowsSource.find("case GameplayEventType::ChestOpened:") != std::string::npos &&
              windowsSource.find("PlayPositionalSoundEffect(context, \"chest_open.wav\"") != std::string::npos &&
              windowsSource.find("case GameplayEventType::TorchExtinguished:") != std::string::npos &&
              windowsSource.find("PlayPositionalSoundEffect(context, \"torch_extinguish.wav\"") != std::string::npos,
              "Windows must map the shared chest and torch transitions to the licensed positional Pixabay cues");
        check(androidSource.find("PLATFORM_EVENT_CHEST_UNLOCKED = 13") != std::string::npos &&
              androidSource.find("case PLATFORM_EVENT_CHEST_UNLOCKED:") != std::string::npos &&
              androidSource.find("playSpatialSound(\"chest_unlock\"") != std::string::npos &&
              androidSource.find("PLATFORM_EVENT_CHEST_OPENED = 14") != std::string::npos &&
              androidSource.find("playSpatialSound(\"chest_open\"") != std::string::npos &&
              androidSource.find("PLATFORM_EVENT_TORCH_EXTINGUISHED = 16") != std::string::npos &&
              androidSource.find("playSpatialSound(\"torch_extinguish\"") != std::string::npos,
              "Android must map the same shared chest and torch transitions to licensed positional cues");
        const auto cueCaseHasNoHaptic = [&](const char* caseLabel) {
            const std::size_t cueCase = androidSource.find(caseLabel);
            const std::size_t cueCaseEnd = androidSource.find("break;", cueCase);
            return cueCase != std::string::npos &&
                   cueCaseEnd != std::string::npos &&
                   androidSource.substr(cueCase, cueCaseEnd - cueCase)
                           .find("performHaptic") == std::string::npos;
        };
        check(cueCaseHasNoHaptic("case PLATFORM_EVENT_CHEST_UNLOCKED:") &&
                  cueCaseHasNoHaptic("case PLATFORM_EVENT_CHEST_OPENED:") &&
                  cueCaseHasNoHaptic("case PLATFORM_EVENT_TORCH_EXTINGUISHED:"),
              "chest and torch transition cues must not silently append Android haptics");
        check(std::filesystem::exists(root / "assets/audio/pixabay/chest_unlock.wav") &&
                  std::filesystem::file_size(root / "assets/audio/pixabay/chest_unlock.wav") == 36908u &&
                  std::filesystem::exists(root / "assets/audio/pixabay/chest_open.wav") &&
                  std::filesystem::file_size(root / "assets/audio/pixabay/chest_open.wav") == 479276u &&
                  std::filesystem::exists(root / "assets/audio/pixabay/torch_extinguish.wav") &&
                  std::filesystem::file_size(root / "assets/audio/pixabay/torch_extinguish.wav") == 100268u,
              "exact deterministic Pixabay runtime WAV derivatives must be present");
        check(windowsSource.find("waterfall_loop.wav") != std::string::npos &&
              windowsSource.find("StartOrUpdateLoop") != std::string::npos &&
              windowsSource.find("StopLoop") != std::string::npos,
              "Windows waterfall ambience must be a controllable positional loop");
        const auto waterfallUpdate = section(androidSource, "private void updateWaterfallLoop()", "private void loadSound(");
        const auto androidPause = section(androidSource, "protected void onPause()", "protected void onDestroy()");
        check(androidSource.find("new HordeAmbiencePlayback(this, this::isMusicAudioFocusGranted)") != std::string::npos &&
              waterfallUpdate.find("resumed && surfaceStarted && !menuVisible && !diagnosticsVisible") != std::string::npos &&
              waterfallUpdate.find("!benchmarkRunning && preferences.getBoolean(\"sfx_enabled\", true)") != std::string::npos &&
              waterfallUpdate.find("ProbeBridge.getSurfaceRuntimeState(surfaceRequestGeneration) == 1") != std::string::npos &&
              waterfallUpdate.find("ProbeBridge.getWaterfallStereoGains()") != std::string::npos &&
              waterfallUpdate.find("preferences.getInt(\"sfx_volume\", 70) / 100.0f") != std::string::npos &&
              waterfallUpdate.find("clamp(userGain * leftScale, 0.0f, 1.0f)") != std::string::npos &&
              waterfallUpdate.find("clamp(userGain * rightScale, 0.0f, 1.0f)") != std::string::npos &&
              androidBridgeSource.find("getWaterfallStereoGains") != std::string::npos &&
              androidSource.find("waterfallPlayer") == std::string::npos,
              "Android waterfall must consume native positional gains times independent SFX volume only on the resumed ready gameplay surface");
        check(androidSource.find("waterfallPlayback.setControl(true, 0.0f, 0.0f, delayedGameplayFeedbackGeneration)") != std::string::npos &&
              section(androidSource, "private void setGameplayPaused(boolean paused)", "private void showDiagnostics(")
                  .find("if (paused) suspendAndResetWaterfall();") != std::string::npos &&
              section(androidSource, "private void resetRoute()", "private void retryEncounter()")
                  .find("++delayedGameplayFeedbackGeneration;") != std::string::npos &&
              section(androidSource, "private void resetRoute()", "private void retryEncounter()")
                  .find("suspendAndResetWaterfall();") != std::string::npos &&
              section(androidSource, "private void retryEncounter()", "private void restartAfterDeath()")
                  .find("++delayedGameplayFeedbackGeneration;") != std::string::npos &&
              section(androidSource, "private void retryEncounter()", "private void restartAfterDeath()")
                  .find("suspendAndResetWaterfall();") != std::string::npos &&
              androidPause.find("++delayedGameplayFeedbackGeneration;") != std::string::npos &&
              androidPause.find("suspendAndResetWaterfall();") != std::string::npos &&
              androidSource.find("waterfallPlayback.close(); waterfallPlayback = null;") != std::string::npos,
              "Android menu pause must retain its epoch while reset/retry/background suspend a new epoch and destroy retains worker cleanup ownership");
        const auto reconcile = section(ambienceWorker, "// Native polling can race", "if (!playing)");
        const auto workerLoop = section(ambienceWorker, "while (true)", "} catch (Exception | LinkageError | OutOfMemoryError error)");
        check(ambienceCore.find("pocket_audio::DecodePcmWave(bytes, kWaterfallCoreFrames, samples_)") != std::string::npos &&
              ambienceCore.find(".tail = {}, .looping = true") != std::string::npos &&
              ambienceCore.find("std::make_unique<pocket_audio::PcmLoopStream>(clips_, 0u)") != std::string::npos &&
              ambienceCore.find("core_->Reset();") != std::string::npos &&
              ambienceCore.find("core_->Render(output)") != std::string::npos &&
              !workerLoop.empty() && workerLoop.find("nativeCreate(") == std::string::npos &&
              ambienceWorker.find("queue.reset(); playing = false;") != std::string::npos &&
              reconcile.find("synchronized (lock)") != std::string::npos &&
              reconcile.find("outputGate.reconcile(pause, suspended, focusGranted.getAsBoolean()") != std::string::npos &&
              ambienceGate.find("!nativeSuspended && !liveSuspended && focusGranted && nativeEpoch == liveEpoch") != std::string::npos &&
              ambienceWorker.find("!suspended && focusGranted.getAsBoolean() && generation == epoch") != std::string::npos &&
              ambienceWorker.find("output.write(pcm, queue.offset(), writable, AudioTrack.WRITE_NON_BLOCKING)") != std::string::npos &&
              ambienceWorker.find("queue.accepted(written)") != std::string::npos,
              "Android waterfall must use one admitted Core cursor, retain partial nonblocking writes and reconcile live pause/focus/epoch before restarting or accepting old PCM");
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
