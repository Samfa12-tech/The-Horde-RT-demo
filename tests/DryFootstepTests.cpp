#include "gameplay/DryFootstepFeedback.h"
#include "gameplay/effects/WaterContact.h"
#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/DevelopmentCheckpointSimulation.h"
#include "platform/android/GameplayEventMetadata.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/assets/AssetManifest.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <string_view>

namespace {
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
namespace cp = horde::gameplay::campaign;
namespace tr = horde::gameplay::traversal;
constexpr float kTick = 1.0f / 60.0f;
int checks = 0, failures = 0;

void Check(bool value, const char* message) {
    ++checks;
    if (!value) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
bool Near(float a, float b, float tolerance = .0001f) { return std::abs(a - b) <= tolerance; }
bool IsStep(GameplayEventType type) {
    return type == GameplayEventType::PlayerFootstep || type == GameplayEventType::PlayerWetFootstep;
}
unsigned StepCount(const GameSimulation& sim) {
    unsigned count = 0;
    for (const auto& event : sim.Events().Events()) count += IsStep(event.type);
    return count;
}
InputSnapshot Neutral() {
    InputSnapshot input;
    input.damageEnabled = false; input.tutorialEnabled = false;
    return input;
}
GameSimulationConfig Chamber() {
    auto config = ProductionGameSimulationConfig();
    config.playerStartX = -5.5f; config.playerStartZ = -13.5f;
    config.playerStartYawRadians = 0;
    config.waterfallSkeletonEncounter = false; config.swordStartsStowed = false;
    return config;
}
cp::Checkpoint Record(bool upper) {
    // Logical CPU fixtures only: these do not establish naturally earned saves.
    cp::Checkpoint record;
    record.guards = {cp::GuardPose{-4.f, -15.2f, .3f}, cp::GuardPose{-6.f, -16.f, -.7f}};
    record.dialogue.grateConsumed = record.dialogue.rescueConsumed = true;
    if (upper) {
        record.id = cp::CheckpointId::SafeExit; record.zone = WorldZoneId::TombExterior;
        record.position = tr::kExteriorLanding; record.firstAscentCompleted = true; record.yaw = 0;
        record.dialogue.reunionStarted = record.dialogue.waitingForRaise = true;
    }
    return record;
}
bool Restore(GameSimulation& sim, const cp::Checkpoint& record,
             const std::shared_ptr<const TriangleSupportSurface>& terrain,
             const InputSnapshot& input, std::uint64_t publication) {
    const auto token = sim.BeginCampaignRestore(record);
    return token && sim.CommitCampaignRestore(*token,
        {token->worldGeneration, record.zone}, terrain, input, publication);
}

void CueAndTransportContracts() {
    GameplayEvent event;
    event.source = EntityId::Player; event.target = EntityId::Invalid;
    event.sequence = 0x123456789ULL; event.tickIndex = 271;
    for (const auto surface : {SupportSurface::Stone, SupportSurface::Earth, SupportSurface::Wood}) {
        for (unsigned variant = 0; variant < 2; ++variant) {
            event.payload = EncodeDryFootstepContact(surface, variant);
            const auto contact = DecodeDryFootstepContact(event.payload);
            const auto cue = DryFootstepCueForEvent(event);
            const auto metadata = horde::platform::android::PackGameplayEventMetadata(event);
            Check(contact.surface == surface && contact.variant == variant,
                  "contact payload round trips actual surface and bounded variation");
            Check((metadata & 0xffffffULL) == (static_cast<std::uint64_t>(EntityId::Player) << 8u) &&
                  ((metadata >> 24u) & 0xffu) == static_cast<unsigned>(cue) &&
                  (metadata >> 32u) == (event.sequence & 0xffffffffu),
                  "Android cue byte preserves existing type, entity and sequence fields");
            const char* expected = surface == SupportSurface::Earth
                ? (variant ? "skeleton_step_2.wav" : "skeleton_step_1.wav")
                : (variant ? "player_step_2.wav" : "player_step_1.wav");
            Check(DryFootstepClip(cue) && std::string_view(DryFootstepClip(cue)) == expected,
                  "shared selection uses admitted dirt variants while Wood retains explicit fallback");
        }
    }
    for (const auto invalid : {-1, 6, std::numeric_limits<std::int32_t>::max()}) {
        event.payload = invalid;
        Check(DecodeDryFootstepContact(invalid).surface == SupportSurface::Stone &&
              DryFootstepCueForEvent(event) == DryFootstepCue::DefaultFirst,
              "invalid contact payload selects bounded historical fallback");
    }
    event.payload = EncodeDryFootstepContact(static_cast<SupportSurface>(99), 7);
    Check(DecodeDryFootstepContact(event.payload).surface == SupportSurface::Stone &&
          DecodeDryFootstepContact(event.payload).variant == 1,
          "unknown support and excessive variation are bounded before encoding");
    for (const auto type : {GameplayEventType::PlayerWetFootstep, GameplayEventType::EnemyFootstep,
                            GameplayEventType::PlayerSwordDrawStarted, GameplayEventType::PlayerSwing}) {
        event.type = type;
        Check(DryFootstepCueForEvent(event) == DryFootstepCue::None &&
              DryFootstepClip(DryFootstepCueForEvent(event)) == nullptr,
              "wet, enemy and noncontact events cannot acquire a second dry sound");
    }
    event.type = GameplayEventType::PlayerFootstep; event.source = EntityId::SkeletonA;
    Check(DryFootstepCueForEvent(event) == DryFootstepCue::None &&
          ((horde::platform::android::PackGameplayEventMetadata(event) >> 24u) & 0xffu) == 0,
          "a nonplayer source cannot select player footstep audio");

    DryFootstepSequence sequence;
    DryFootstepCue previousDefault = DryFootstepCue::None, previousEarth = DryFootstepCue::None;
    event.source = EntityId::Player;
    for (unsigned i = 0; i < 18; ++i) {
        const auto surface = i % 3 == 0 ? SupportSurface::Stone
            : i % 3 == 1 ? SupportSurface::Earth : SupportSurface::Wood;
        event.payload = sequence.Next(surface);
        const auto cue = DryFootstepCueForEvent(event);
        auto& previous = surface == SupportSurface::Earth ? previousEarth : previousDefault;
        Check(cue != previous, "each audible family alternates without boundary-triggered repetition");
        previous = cue;
    }
    sequence.Reset();
    Check(sequence.Next(SupportSurface::Stone) == EncodeDryFootstepContact(SupportSurface::Stone, 0) &&
          sequence.Next(SupportSurface::Earth) == EncodeDryFootstepContact(SupportSurface::Earth, 0),
          "transient sequence reset deterministically clears both families");
}

void GroundedDistanceCadence() {
    TravelFootstepCadence cadence;
    Check(!cadence.Update(.239f, true, true) && cadence.Update(.001f, true, true),
          "first contact requires the existing 0.24 metres of actual travel");
    Check(!cadence.Update(.719f, true, true) && cadence.Update(.001f, true, true),
          "subsequent contact retains existing 0.72 metre spacing");
    bool stationarySilent = true;
    for (int tick = 0; tick < 120; ++tick) stationarySilent &= !cadence.Update(0, true, true);
    Check(stationarySilent, "stationary intent cannot manufacture a contact");
    Check(!cadence.Update(.20f, true, true) && !cadence.Update(1.f, true, false) &&
          !cadence.Update(.239f, true, true) && cadence.Update(.001f, true, true),
          "airborne travel is silent and landing starts a fresh ground-distance threshold");
    Check(!cadence.Update(.23f, true, true) && !cadence.Update(0, false, true) &&
          !cadence.Update(.239f, true, true) && cadence.Update(.001f, true, true),
          "stopping cancels residual cadence instead of replaying a pending contact");
}

struct TravelResult { float distance = 0; unsigned contacts = 0; };
TravelResult Travel(InputSnapshot input, unsigned ticks) {
    GameSimulation sim(Chamber());
    TravelResult result;
    std::uint64_t previousSequence = 0;
    unsigned expected = 0;
    for (unsigned tick = 0; tick < ticks; ++tick) {
        const auto previous = sim.Snapshot();
        sim.ClearEvents(); sim.StepFixed(input);
        const auto& now = sim.Snapshot();
        const float travel = std::hypot(now.playerX - previous.playerX, now.playerZ - previous.playerZ);
        result.distance += travel;
        if (result.distance + .00001f >= .24f + .72f * expected) ++expected;
        unsigned thisTick = 0;
        for (const auto& event : sim.Events().Events()) if (IsStep(event.type)) {
            ++result.contacts; ++thisTick;
            Check(event.tickIndex == now.tickIndex && event.sequence > previousSequence &&
                  Near(event.worldX, now.playerX) && Near(event.worldZ, now.playerZ) &&
                  Near(event.listenerX, now.playerX) && Near(event.listenerZ, now.playerZ),
                  "contacts retain ordered identity and collision-resolved event-time placement");
            previousSequence = event.sequence;
            if (event.type == GameplayEventType::PlayerFootstep)
                Check(DecodeDryFootstepContact(event.payload).surface == now.playerSupportSurface,
                      "dry contact captures current physical support rather than zone or end-frame tags");
        }
        Check(thisTick <= 1 && result.contacts == expected,
              "contacts match independent cumulative travel thresholds without layered or extra events");
    }
    return result;
}
void MovementAndSuppression() {
    auto input = Neutral(); input.moveForward = 1;
    const auto straight = Travel(input, 30);
    input.moveStrafe = 1;
    const auto diagonal = Travel(input, 30);
    Check(Near(straight.distance, diagonal.distance, .001f) && straight.contacts == diagonal.contacts,
          "normalized diagonal travel retains ordinary distance-driven contact timing");
    input.moveStrafe = 0; input.runHeld = true;
    const auto running = Travel(input, 30);
    Check(running.distance > straight.distance && running.contacts >= straight.contacts,
          "running increases contact rate only through additional actual travel");

    auto config = Chamber(); config.playerStartX = -8.24f;
    GameSimulation wall(config); input = Neutral(); input.moveStrafe = -1;
    float blockedTravel = 0; unsigned blockedSteps = 0;
    for (int tick = 0; tick < 120; ++tick) {
        const auto previous = wall.Snapshot(); wall.ClearEvents(); wall.StepFixed(input);
        blockedTravel += std::hypot(wall.Snapshot().playerX - previous.playerX,
                                   wall.Snapshot().playerZ - previous.playerZ);
        blockedSteps += StepCount(wall);
    }
    Check(blockedTravel < .24f && blockedSteps == 0, "sustained input into a wall produces no invented footsteps");

    GameSimulation paused(Chamber()); input = Neutral(); input.moveForward = 1;
    for (int tick = 0; tick < 6; ++tick) { paused.ClearEvents(); paused.StepFixed(input); }
    paused.ClearEvents(); input.paused = true;
    const auto beforePause = paused.Snapshot();
    paused.AdvanceFrame(input, 1.0);
    Check(StepCount(paused) == 0 && paused.Snapshot().tickIndex == beforePause.tickIndex,
          "pause neither advances contacts nor emits a catch-up burst");
    input.paused = false; paused.ClearEvents(); paused.StepFixed(input);
    Check(StepCount(paused) == 0, "resume starts a fresh threshold instead of replaying pre-pause distance");
    input.moveForward = 0;
    bool stoppedSilent = true;
    for (int tick = 0; tick < 120; ++tick) {
        paused.ClearEvents(); paused.StepFixed(input); stoppedSilent &= StepCount(paused) == 0;
    }
    Check(stoppedSilent, "stationary simulation remains silent after a previous walking contact sequence");

    GameSimulation teleported(Chamber()); input = Neutral();
    input.hasAuthoritativePlayerPose = true;
    input.authoritativePlayerZ = -13.5f;
    bool warpSilent = true;
    for (int tick = 0; tick < 5; ++tick) {
        input.authoritativePlayerX = -5.5f + .12f * (tick + 1);
        teleported.ClearEvents(); teleported.StepFixed(input);
        warpSilent &= StepCount(teleported) == 0;
    }
    Check(warpSilent && Near(teleported.Snapshot().playerX, -4.9f),
          "authoritative teleport moves the fixture without ordinary ground contacts");
    input.hasAuthoritativePlayerPose = false; input.moveForward = 1;
    bool restartSilent = true;
    for (int tick = 0; tick < 6; ++tick) {
        teleported.ClearEvents(); teleported.StepFixed(input);
        restartSilent &= StepCount(teleported) == 0;
    }
    Check(restartSilent, "normal input after teleport requires fresh ground travel, without distance debt");
    teleported.ClearEvents(); teleported.StepFixed(input);
    teleported.ClearEvents(); teleported.StepFixed(input);
    Check(StepCount(teleported) == 1,
          "ordinary travel resumes a single contact at the fresh 0.24 metre threshold");
}

void WetPriority() {
    auto config = ProductionGameSimulationConfig();
    config.playerStartX = -4.70f; config.playerStartZ = -15.20f;
    GameSimulation sim(config); auto input = Neutral();
    unsigned wetSteps = 0; bool exclusive = true;
    for (int tick = 0; tick < 120; ++tick) {
        input.moveStrafe = (tick / 15) % 2 == 0 ? 1.f : -1.f;
        sim.ClearEvents(); sim.StepFixed(input);
        unsigned contacts = 0;
        for (const auto& event : sim.Events().Events()) if (IsStep(event.type)) {
            ++contacts;
            if (event.type == GameplayEventType::PlayerWetFootstep) {
                ++wetSteps;
                float surfaceY = 0;
                Check(effects::WaterSurfaceAt(event.worldX, event.worldZ, surfaceY) && event.payload == 0 &&
                      DryFootstepCueForEvent(event) == DryFootstepCue::None &&
                      ((horde::platform::android::PackGameplayEventMetadata(event) >> 24u) & 0xffu) == 0,
                      "real wet contacts retain one wet event and cannot acquire dry cue metadata");
            }
        }
        exclusive &= contacts <= 1;
    }
    Check(wetSteps > 0 && exclusive, "shallow water retains precedence with at most one sound contact per tick");
}

void ActualTerrainAndBoundary(const std::shared_ptr<const TriangleSupportSurface>& terrain) {
    GameSimulation sim(ProductionGameSimulationConfig());
    const auto record = Record(true);
    auto input = Neutral(); input.yawRadians = record.yaw;
    std::uint64_t publication = 100;
    if (!Restore(sim, record, terrain, input, publication)) {
        Check(false, "actual terrain safe-exit fixture restores before surface walk"); return;
    }
    sim.StepFixed(input, kTick, ++publication);
    const auto generation = sim.Snapshot().worldRoute.generation;
    Check(sim.PublishWorldZoneReadiness({generation, WorldZoneId::ForestApproach}, ZoneReadiness::Ready) &&
          sim.PublishWorldZoneReadiness({generation, WorldZoneId::Lookout}, ZoneReadiness::Ready),
          "fixture explicitly supplies current destination readiness");
    bool sawStone = false, sawEarth = false, sawBoundary = false;
    auto previousSurface = sim.Snapshot().playerSupportSurface;
    float distance = 0; unsigned contacts = 0, expected = 0, ticks = 0;
    const auto goal = kWorldRoutePoints[3];
    for (; ticks < 900; ++ticks) {
        const auto previous = sim.Snapshot();
        const float dx = goal.x - previous.playerX, dz = goal.z - previous.playerZ;
        if (std::hypot(dx, dz) < .12f) break;
        input.yawRadians = std::atan2(dx, -dz); input.moveForward = 1;
        sim.ClearEvents(); sim.StepFixed(input, kTick, ++publication);
        const auto& now = sim.Snapshot();
        distance += std::hypot(now.playerX - previous.playerX, now.playerZ - previous.playerZ);
        if (distance + .00001f >= .24f + .72f * expected) ++expected;
        sawBoundary |= now.playerSupportSurface != previousSurface;
        previousSurface = now.playerSupportSurface;
        for (const auto& event : sim.Events().Events()) if (event.type == GameplayEventType::PlayerFootstep) {
            ++contacts;
            const auto surface = DecodeDryFootstepContact(event.payload).surface;
            sawStone |= surface == SupportSurface::Stone;
            sawEarth |= surface == SupportSurface::Earth;
            Check(surface == now.playerSupportSurface && now.playerGrounded,
                  "actual terrain movement tags the contact with current grounded support");
        }
        Check(contacts == expected, "surface transition does not insert or reset a distance contact");
    }
    Check(ticks > 0 && ticks < 900 && sawStone && sawEarth && sawBoundary && distance > 5,
          "normal shared movement crosses actual landing Stone to authored terrain Earth");
    input.moveForward = 0;
    bool quiet = true;
    for (int tick = 0; tick < 90; ++tick) {
        sim.ClearEvents(); sim.StepFixed(input, kTick, ++publication); quiet &= StepCount(sim) == 0;
    }
    Check(quiet, "standing on the new surface does not produce boundary or catch-up sounds");
}

void RopeAndRestore(const std::shared_ptr<const TriangleSupportSurface>& terrain, bool upper) {
    GameSimulation sim(ProductionGameSimulationConfig());
    const auto record = Record(upper);
    auto input = Neutral(); input.yawRadians = record.yaw;
    std::uint64_t publication = 100;
    if (!Restore(sim, record, terrain, input, publication)) {
        Check(false, "safe-side logical fixture restores before rope contact checks"); return;
    }
    sim.StepFixed(input, kTick, ++publication);
    const auto destination = upper ? WorldZoneId::Dungeon : WorldZoneId::TombExterior;
    const WorldZoneToken ready{sim.Snapshot().worldRoute.generation, destination};
    Check(upper ? sim.PublishWorldZoneReadiness(ready, ZoneReadiness::Ready)
                : sim.PublishChapterTerrain(ready, terrain),
          "actual rope fixture supplies current destination readiness");
    sim.ClearEvents(); ++input.commands.interact;
    sim.StepFixed(input, kTick, ++publication);
    Check(sim.Snapshot().rescue.equipmentStowed && StepCount(sim) == 0,
          "actual contextual rope entry is silent for footsteps");
    bool quiet = true; unsigned ticks = 0;
    input.moveForward = 1;
    while (sim.Snapshot().rescue.equipmentStowed && ticks < 1800) {
        sim.ClearEvents(); sim.StepFixed(input, kTick, ++publication);
        quiet &= StepCount(sim) == 0; ++ticks;
    }
    Check(ticks > 0 && ticks < 1800 && sim.Snapshot().rescue.exteriorSide != upper && quiet,
          "actual ascent or descent ignores held ground movement without footsteps");
    input.moveForward = 0;
    for (int tick = 0; tick < 30; ++tick) {
        sim.ClearEvents(); sim.StepFixed(input, kTick, ++publication); quiet &= StepCount(sim) == 0;
    }
    Check(quiet, "rope landing and stationary post-rope ticks cannot replay a travel contact");
    Check(Restore(sim, record, terrain, input, publication) && StepCount(sim) == 0,
          "committed logical restore discards any old event and transient cadence");
    sim.ClearEvents(); sim.StepFixed(input, kTick, ++publication);
    Check(StepCount(sim) == 0, "new neutral post-load publication has no stale footsteps");
}

void ActualTerrainCheckpointPreparation(const std::shared_ptr<const TriangleSupportSurface>& terrain) {
    auto config=ProductionGameSimulationConfig();config.developmentRescueJourney=true;config.authoredChapterTerrain=true;
    GameSimulation sim(config);DevelopmentCheckpointStageEvidence evidence;
    const auto* checkpoint=FindDevelopmentCheckpoint("chapter-forest-lookout");
    const bool ready=sim.PublishChapterTerrain({sim.Snapshot().worldRoute.generation,WorldZoneId::TombExterior},terrain);
    const bool staged=ready && checkpoint && StageDevelopmentCheckpointSimulation(sim,*checkpoint,&evidence);
    const auto& state=sim.Snapshot();
    std::cout<<"actual-terrain203 staged="<<staged<<" failure="<<DevelopmentCheckpointStageFailureName(evidence.failure)
        <<" overflow="<<state.eventQueueOverflowCount<<" tick="<<state.tickIndex<<" x="<<state.playerX
        <<" z="<<state.playerZ<<" y="<<state.playerSupportWorldY<<" claim="<<state.rescue.claimCount
        <<" deployment="<<state.rescue.deploymentCount<<" attackEdges="<<evidence.consumedAttackEdges
        <<" swingEvents="<<evidence.playerSwingEvents<<" enemyHits="<<evidence.enemyHitEvents<<'\n';
    Check(staged && state.rescue.exteriorSide && !state.rescue.equipmentStowed && state.rescue.firstAscentCompleted &&
          state.rescue.claimCount==1 && state.rescue.deploymentCount==1 &&
          std::hypot(state.playerX-kWorldRoutePoints.back().x,state.playerZ-kWorldRoutePoints.back().z)<.01f,
          "actual imported terrain checkpoint203 retains its existing rope and collision-resolved route staging");
    Check(state.eventQueueOverflowCount==0 && sim.Events().Empty(),
          "long diagnostic exterior preparation drains events without overflowing or erasing an overflow counter");
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: DryFootstepTests <source-root>\n"; return 2; }
    CueAndTransportContracts();
    GroundedDistanceCadence();
    MovementAndSuppression();
    WetPriority();
    const auto directory = std::filesystem::path(argv[1]) / "assets/models/world/runtime/chapter-terrain-v1";
    horde::scene::assets::AssetManifest manifest;
    horde::scene::assets::StaticMeshAsset mesh;
    std::string diagnostic;
    if (!horde::scene::assets::AssetManifest::Load(directory / "asset.manifest.json", manifest, diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(directory / "chapter-terrain-lod0.runtime.glb", manifest, mesh, diagnostic)) {
        std::cerr << "Actual chapter terrain intake failed: " << diagnostic << '\n'; return 1;
    }
    const auto terrain = horde::scene::PrepareChapterTerrainSurface(mesh, diagnostic);
    if (!terrain) { std::cerr << diagnostic << '\n'; return 1; }
    ActualTerrainAndBoundary(terrain);
    RopeAndRestore(terrain, false);
    RopeAndRestore(terrain, true);
    ActualTerrainCheckpointPreparation(terrain);
    std::cout << checks << " dry footstep host checks, " << failures << " failures\n";
    std::cout << "Logical restore and native CPU terrain/motor fixtures only. Airborne gate is a cadence unit check; "
                 "no jump motor, actual listening or physical device acceptance is claimed.\n";
    return failures ? 1 : 0;
}
