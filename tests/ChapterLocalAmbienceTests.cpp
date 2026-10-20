#include "gameplay/simulation/ChapterLocalAmbience.h"
#include "gameplay/simulation/FixedStepRunner.h"
#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/effects/WaterContactSurfaces.h"
#include "platform/android/GameplayEventMetadata.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/assets/AssetManifest.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace {
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
namespace tr = horde::gameplay::traversal;
namespace cp = horde::gameplay::campaign;
using Local = ChapterLocalAmbience;
using Cue = Local::Cue;
constexpr float kTick = 1.f / 60.f;
int checks = 0, failures = 0;
void Check(bool condition, const char* message) {
    ++checks;
    if (!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
bool Near(float a, float b) { return std::abs(a - b) < .0001f; }
bool Same(tr::Vec3 a, tr::Vec3 b) { return Near(a.x, b.x) && Near(a.y, b.y) && Near(a.z, b.z); }
SimulationSnapshot DripState() {
    SimulationSnapshot s;
    s.developmentRescueJourney = true; s.playerAlive = s.playerGrounded = true;
    s.playerX = -3.8f; s.playerZ = -15.6f; s.playerSupportWorldY = kRouteFloorWorldY;
    s.worldRoute.current = WorldZoneId::Dungeon; s.activeSkeletonCount = 0;
    s.rescue.generation = 9;
    return s;
}
SimulationSnapshot RopeState() {
    auto s = DripState();
    s.playerX = tr::kLowerLanding.x; s.playerZ = tr::kLowerLanding.z; s.playerGrounded = false;
    s.rescue.lanternClaimed = s.rescue.ropeReady = true;
    s.rescue.equipmentStowed = s.rescue.ropeHandsActive = true;
    s.rescue.phase = tr::Phase::Ascent;
    s.rescue.grippingRopeNodeIndices = {8.f, 9.f};
    s.rescue.grippingHandTargets = {tr::Vec3{-33.6f, 1.f, -15.2f}, tr::Vec3{-33.8f, 1.4f, -15.f}};
    return s;
}
void Eligibility() {
    const auto drip = DripState();
    Check(Local::Eligible(drip) && Local::DripEligible(drip) && !Local::RopeEligible(drip),
          "quiet chapter waterfall floor admits only its local drip");
    for (unsigned mode = 0; mode < 14; ++mode) {
        auto bad = drip;
        switch (mode) {
        case 0: bad.developmentRescueJourney = false; break;
        case 1: bad.playerAlive = false; break;
        case 2: bad.paused = true; break;
        case 3: bad.chapterDialogue.line = dialogue::Line::RescueRope; break;
        case 4: bad.playerGrounded = false; break;
        case 5: bad.activeSkeletonCount = 1; break;
        case 6: bad.rescue.exteriorSide = true; break;
        case 7: bad.rescue.equipmentStowed = true; break;
        case 8: bad.worldRoute.current = WorldZoneId::ForestApproach; break;
        case 9: bad.playerX = 0; break;
        case 10: bad.playerSupportWorldY += .25f; break;
        case 11: bad.playerX = std::numeric_limits<float>::quiet_NaN(); break;
        case 12: bad.playerZ = std::numeric_limits<float>::infinity(); break;
        case 13: bad.playerSupportWorldY = std::numeric_limits<float>::quiet_NaN(); break;
        }
        Check(!Local::DripEligible(bad), "legacy, danger, speech, air, wrong habitat/height or nonfinite input rejects drips");
    }
    auto edge = drip; edge.playerX = Local::kDripPosition.x - 4.f; edge.playerZ = Local::kDripPosition.z;
    Check(Local::DripEligible(edge), "drip eligibility includes its exact four metre spatial boundary");
    edge.playerX -= .001f;
    Check(!Local::DripEligible(edge), "drip eligibility excludes positions beyond its actual source radius");
    float waterY = 0;
    Check(effects::WaterSurfaceAt(Local::kDripPosition.x, Local::kDripPosition.z, waterY) &&
          std::abs(waterY-Local::kDripPosition.y) < .05f,
          "drip source sits on the existing actual runoff-water footprint rather than an invented damp patch");
    const auto rope = RopeState();
    Check(Local::RopeEligible(rope) && !Local::DripEligible(rope),
          "loaded moving rope may be airborne and never layers a tomb drip");
    for (unsigned mode = 0; mode < 11; ++mode) {
        auto bad = rope;
        switch (mode) {
        case 0: bad.developmentRescueJourney = false; break;
        case 1: bad.playerAlive = false; break;
        case 2: bad.paused = true; break;
        case 3: bad.chapterDialogue.line = dialogue::Line::RescueRope; break;
        case 4: bad.rescue.lanternClaimed = false; break;
        case 5: bad.rescue.ropeReady = false; break;
        case 6: bad.rescue.equipmentStowed = false; break;
        case 7: bad.rescue.ropeHandsActive = false; break;
        case 8: bad.rescue.grippingRopeNodeIndices[0] = -1; break;
        case 9: bad.rescue.grippingRopeNodeIndices[1] = static_cast<float>(tr::kRopeNodeCount); break;
        case 10: bad.rescue.grippingHandTargets[0].y = std::numeric_limits<float>::infinity(); break;
        }
        Check(!Local::RopeEligible(bad), "rope ownership, active hands, speech and finite contact guards are mandatory");
    }
    for (auto phase : {tr::Phase::LowerSafe, tr::Phase::UpperSafe, tr::Phase::AwaitingAscentReadiness,
                       tr::Phase::AwaitingDescentReadiness, tr::Phase::ApproachAscent, tr::Phase::ApproachDescent}) {
        auto waiting = rope; waiting.rescue.phase = phase;
        Check(!Local::RopeEligible(waiting), "waiting, approach and safe endpoints cannot sound like loaded grip travel");
    }
}
void DripCadenceAndIdentity() {
    for (unsigned rate : {24u, 60u, 144u}) {
        Local local; FixedStepRunner runner; auto s = DripState();
        std::vector<std::uint64_t> ticks; bool finite = true, idempotent = true;
        for (unsigned frame = 0; frame < rate * 100; ++frame) runner.Advance(1.0 / rate, false, [&](float) {
            ++s.tickIndex; const auto result = local.Step(s);
            if (result.cue == Cue::Drip) {
                ticks.push_back(s.tickIndex);
                finite &= Same(result.position, Local::kDripPosition) && Near(result.gain, .25f);
            } else finite &= result.cue == Cue::None;
            idempotent &= local.Step(s).cue == Cue::None;
        });
        Check(ticks == std::vector<std::uint64_t>({360, 1440, 2880, 4140, 5220}),
              "fixed-tick drip cadence is sparse and identical across render groupings");
        Check(finite && idempotent, "drips retain fixed finite source/gain and cannot repeat on one tick");
    }
    for (unsigned identity = 0; identity < 4; ++identity) {
        Local local; auto s = DripState();
        for (unsigned i = 0; i < 359; ++i) { ++s.tickIndex; local.Step(s); }
        ++s.tickIndex;
        if (identity == 0) ++s.retryGeneration;
        if (identity == 1) ++s.lastConsumedRouteResetSequence;
        if (identity == 2) ++s.rescue.generation;
        if (identity == 3) s.tickIndex = 0;
        Check(local.Step(s).cue == Cue::Stop, "retry/reset/generation/backward-tick import cancels the prior finite voice");
        bool quiet = true;
        for (unsigned i = 0; i < 359; ++i) { ++s.tickIndex; quiet &= local.Step(s).cue == Cue::None; }
        ++s.tickIndex;
        Check(quiet && local.Step(s).cue == Cue::Drip, "new identity earns a full fresh quiet interval with no backlog");
    }
    Local local; auto s = DripState();
    for (unsigned i = 0; i < 359; ++i) { ++s.tickIndex; local.Step(s); }
    s.paused = true; ++s.tickIndex;
    Check(local.Step(s).cue == Cue::Stop, "pause cancels an imminent drip instead of deferring it");
    bool quiet = true;
    for (unsigned i = 0; i < 900; ++i) { ++s.tickIndex; quiet &= local.Step(s).cue == Cue::None; }
    s.paused = false;
    for (unsigned i = 0; i < 359; ++i) { ++s.tickIndex; quiet &= local.Step(s).cue == Cue::None; }
    ++s.tickIndex;
    Check(quiet && local.Step(s).cue == Cue::Drip, "resume has no catch-up sounds and requires the original initial delay");
    local.Reset(); ++s.tickIndex;
    Check(local.Step(s).cue == Cue::None, "explicit import/reset does not replay the last incidental cue");
    s.tickIndex += 1'000'000;
    Check(local.Step(s).cue == Cue::None, "one newer publication does not replay unobserved elapsed ticks");
}
void RopeTravelAndSource() {
    Local local; auto s = RopeState(); ++s.tickIndex; local.Step(s);
    bool silent = true;
    for (unsigned i = 0; i < 240; ++i) {
        ++s.tickIndex;
        s.rescue.grippingHandTargets[0].x += .001f; s.rescue.ropeNodes[4].x += .01f;
        silent &= local.Step(s).cue == Cue::None;
    }
    Check(silent, "stationary grip indices remain silent despite hand/rope sway and an expired cooldown");
    local.Reset(); s = RopeState(); ++s.tickIndex; local.Step(s);
    std::vector<unsigned> contacts;
    bool positioned = true, duplicate = true;
    for (unsigned i = 1; i <= 144; ++i) {
        ++s.tickIndex;
        for (auto& index : s.rescue.grippingRopeNodeIndices) index -= .01f;
        const auto result = local.Step(s);
        if (result.cue == Cue::Rope) {
            contacts.push_back(i);
            const auto& a = s.rescue.grippingHandTargets[0]; const auto& b = s.rescue.grippingHandTargets[1];
            positioned &= Same(result.position, {(a.x + b.x) * .5f, (a.y + b.y) * .5f, (a.z + b.z) * .5f}) &&
                          Near(result.gain, .18f);
        }
        duplicate &= local.Step(s).cue == Cue::None;
    }
    Check(contacts == std::vector<unsigned>({72, 144}) && positioned && duplicate,
          "moving grip contact uses average hand source with minimum 72-tick separation and no duplicate emission");
    for (auto& index : s.rescue.grippingRopeNodeIndices) index -= 2;
    ++s.tickIndex;
    Check(local.Step(s).cue == Cue::None, "discontinuous grip imports rebase instead of converting a jump to travel debt");
    silent = true;
    for (unsigned i = 1; i < 72; ++i) {
        ++s.tickIndex;
        for (auto& index : s.rescue.grippingRopeNodeIndices) index -= .005f;
        silent &= local.Step(s).cue == Cue::None;
    }
    Check(silent, "grip rebase starts a full new cooldown without a queued sound");
    auto drip = DripState(); drip.tickIndex = s.tickIndex + 1;
    Check(local.Step(drip).cue == Cue::Stop, "rope-to-tomb habitat transition stops the one finite voice first");
    ++drip.tickIndex;
    Check(local.Step(drip).cue == Cue::None, "habitat replacement cannot immediately layer a drip onto rope contact");
}
void Transport() {
    static_assert(static_cast<unsigned>(GameplayEventType::PondFrogs) == 31);
    static_assert(static_cast<unsigned>(GameplayEventType::TombDrip) == 32);
    static_assert(static_cast<unsigned>(GameplayEventType::RopeContact) == 33);
    static_assert(static_cast<unsigned>(GameplayEventType::ChapterLocalAmbienceStopped) == 34);
    for (const auto type : {GameplayEventType::TombDrip, GameplayEventType::RopeContact,
                            GameplayEventType::ChapterLocalAmbienceStopped}) {
        GameplayEvent event; event.type = type; event.sequence = 0x12345678; event.worldY = 2.125f; event.listenerY = 3.2f;
        const auto metadata = horde::platform::android::PackGameplayEventMetadata(event);
        const auto vertical = horde::platform::android::PackGameplayEventVerticalMetadata(event);
        Check((metadata & 255u) == static_cast<unsigned>(type) && (metadata >> 32u) == event.sequence &&
              std::bit_cast<float>(static_cast<std::uint32_t>(vertical)) == event.worldY &&
              std::bit_cast<float>(static_cast<std::uint32_t>(vertical >> 32u)) == event.listenerY &&
              IsChapterAmbienceEvent(type), "appended ambient IDs retain ordered identity and exact event-time vertical metadata");
    }
    Check(!IsChapterAmbienceEvent(GameplayEventType::EnemyHit) &&
          IsChapterAmbienceEvent(GameplayEventType::ForestBirdStopped),
          "shared incidental family includes cleanup but cannot absorb combat feedback");
}

cp::Checkpoint Record(bool upper) {
    cp::Checkpoint c;
    c.guards = {cp::GuardPose{-4.f, -15.2f, .3f}, cp::GuardPose{-6.f, -16.f, -.7f}};
    c.dialogue.grateConsumed = c.dialogue.rescueConsumed = true;
    if (upper) {
        c.id = cp::CheckpointId::SafeExit; c.zone = WorldZoneId::TombExterior;
        c.position = tr::kExteriorLanding; c.firstAscentCompleted = true; c.yaw = 0;
        c.dialogue.reunionStarted = c.dialogue.waitingForRaise = true;
    }
    return c;
}
bool Restore(GameSimulation& sim, const cp::Checkpoint& record,
             const std::shared_ptr<const TriangleSupportSurface>& terrain,
             const InputSnapshot& input, std::uint64_t publication) {
    const auto token = sim.BeginCampaignRestore(record);
    return token && sim.CommitCampaignRestore(*token,
        {token->worldGeneration, record.zone}, terrain, input, publication);
}
struct Observed {
    unsigned rope = 0, drip = 0;
    std::uint64_t sequence = 0, previousRopeTick = 0;
    void Inspect(const GameSimulation& sim) {
        const auto& s = sim.Snapshot();
        for (const auto& event : sim.Events().Events()) {
            if (event.type != GameplayEventType::RopeContact && event.type != GameplayEventType::TombDrip) continue;
            Check(event.sequence > sequence && event.tickIndex == s.tickIndex && event.source == EntityId::Invalid &&
                  event.payload == 0, "actual simulation emits one ordered current-tick incidental event without a gameplay actor");
            sequence = event.sequence;
            const auto offset = tr::RescueTorsoOffset(s.rescue);
            Check(Near(event.listenerX, s.playerX + offset.x) && Near(event.listenerZ, s.playerZ + offset.z) &&
                  Near(event.listenerY, PlayerEyeWorldY(s.playerSupportWorldY) + offset.y-s.playerCrouchDropMetres) &&
                  Near(event.listenerYawRadians, s.playerYawRadians),
                  "actual rope/tomb events freeze the current physical listener tuple");
            if (event.type == GameplayEventType::RopeContact) {
                ++rope;
                const auto& a = s.rescue.grippingHandTargets[0]; const auto& b = s.rescue.grippingHandTargets[1];
                Check(Local::RopeEligible(s) && Near(event.worldX, (a.x + b.x) * .5f) &&
                      Near(event.worldY, (a.y + b.y) * .5f) && Near(event.worldZ, (a.z + b.z) * .5f) &&
                      Near(event.intensity, .18f) && (!previousRopeTick || event.tickIndex - previousRopeTick >= 72),
                      "actual moving rope cues use current solved hand midpoint and bounded separation");
                previousRopeTick = event.tickIndex;
            } else {
                ++drip;
                Check(Local::DripEligible(s) && Same({event.worldX, event.worldY, event.worldZ}, Local::kDripPosition) &&
                      Near(event.intensity, .25f), "actual tomb drips retain the admitted physical local source");
            }
        }
    }
};
void ActualRope(const std::shared_ptr<const TriangleSupportSurface>& terrain, bool upper) {
    GameSimulation sim(ProductionGameSimulationConfig()); const auto record = Record(upper);
    InputSnapshot input; input.damageEnabled = false; input.tutorialEnabled = false; input.yawRadians = record.yaw;
    std::uint64_t publication = 100;
    if (!Restore(sim, record, terrain, input, publication)) {
        Check(false, "actual terrain logical safe-side restore must admit the rope fixture"); return;
    }
    Observed observed;
    const auto step = [&]() {
        if (sim.Snapshot().chapterDialogue.line != dialogue::Line::None) ++input.commands.dialogueSkip;
        sim.ClearEvents(); sim.StepFixed(input, kTick, ++publication); observed.Inspect(sim);
    };
    for (unsigned tick = 0; tick < 90; ++tick) step();
    Check(observed.rope == 0 && observed.drip == 0, "deployed waiting rope and either safe landing remain silent");
    const WorldZoneToken ready{sim.Snapshot().worldRoute.generation,
        upper ? WorldZoneId::Dungeon : WorldZoneId::TombExterior};
    Check(upper ? sim.PublishWorldZoneReadiness(ready, ZoneReadiness::Ready) : sim.PublishChapterTerrain(ready, terrain),
          "real traversal receives current destination readiness without a GPU claim");
    ++input.commands.interact; step();
    Check(sim.Snapshot().rescue.equipmentStowed, "ordinary context edge commits the actual rope motor");
    bool pausedOnce = false; unsigned ticks = 0;
    while (sim.Snapshot().rescue.equipmentStowed && ticks < 1800) {
        step(); ++ticks;
        if (!pausedOnce && ticks == 100) {
            const auto before = observed.rope;
            input.paused = true; sim.ClearEvents(); sim.AdvanceFrame(input, 0, ++publication);
            observed.Inspect(sim);
            for (unsigned tick = 0; tick < 90; ++tick) step();
            Check(observed.rope == before, "zero-tick pause and paused fixed ticks emit no stale rope contacts");
            input.paused = false;
            step(); Check(observed.rope == before, "first resume tick rebases current grip contact without replay");
            pausedOnce = true;
        }
    }
    Check(ticks > 0 && ticks < 1800 && pausedOnce && sim.Snapshot().rescue.exteriorSide != upper &&
          sim.Snapshot().playerGrounded && observed.rope > 0 && observed.drip == 0,
          "actual ascent/descent completes with bounded real grip cues and no layered drips");
    const auto finished = observed.rope;
    for (unsigned tick = 0; tick < 120; ++tick) step();
    Check(observed.rope == finished, "completed stationary traversal cannot keep emitting rope audio");
    Check(Restore(sim, record, terrain, input, publication), "committed logical restore replaces the transient audio owner");
    observed = {}; step();
    Check(observed.rope == 0 && observed.drip == 0, "restored safe-side publication does not replay old incidental events");
    sim.ResetRoute(); observed = {};
    for (unsigned tick = 0; tick < 400; ++tick) step();
    Check(observed.rope == 0 && observed.drip == 0, "New Game/reset retains no loaded rope or tomb cadence backlog");
}
void ActualDrip(const std::shared_ptr<const TriangleSupportSurface>& terrain) {
    GameSimulation sim(ProductionGameSimulationConfig()); const auto record = Record(false);
    InputSnapshot input; input.damageEnabled = false; input.tutorialEnabled = false; input.yawRadians = record.yaw;
    std::uint64_t publication = 100;
    if (!Restore(sim, record, terrain, input, publication)) {
        Check(false, "logical lower restore must admit an ordinary-input walk to the wet chamber"); return;
    }
    Observed observed;
    const auto step = [&]() {
        if (sim.Snapshot().chapterDialogue.line != dialogue::Line::None) ++input.commands.dialogueSkip;
        sim.ClearEvents(); sim.StepFixed(input, kTick, ++publication); observed.Inspect(sim);
    };
    step();
    for (const auto target : std::array<std::array<float, 2>, 7>{{
        {-29.5f,-15.2f},{-26.f,-15.2f},{-21.f,-15.2f},{-16.f,-15.2f},
        {-11.f,-15.2f},{-6.5f,-15.2f},{-3.8f,-15.6f}}}) {
        unsigned tick = 0;
        for (; tick < 1800; ++tick) {
            const auto& s = sim.Snapshot(); const float dx = target[0]-s.playerX, dz = target[1]-s.playerZ;
            const float distance = std::hypot(dx, dz);
            if (distance < .035f) break;
            input.yawRadians = std::atan2(dx, -dz); input.moveForward = std::min(1.f, distance/.15f);
            const float scale = distance > .0001f ? std::min(distance, .30f)/distance : 0;
            input.crouchHeld = !AuthoredBodySweepClear(s.playerX, s.playerZ, s.playerX+dx*scale, s.playerZ+dz*scale,
                s.playerSupportWorldY, kStandingBodyHeight, kPlayerCollisionRadius);
            step();
        }
        if (tick == 1800) { Check(false, "ordinary input must reach the next admitted tomb route point"); return; }
    }
    input.moveForward = 0; input.crouchHeld = false;
    for (unsigned tick = 0; tick < 400; ++tick) step();
    Check(observed.drip > 0 && observed.rope == 0 && Local::DripEligible(sim.Snapshot()),
          "actual quiet defeated waterfall-room movement and waiting produce physical localized drips only");
}
std::string Read(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
std::string Section(const std::string& source, const char* begin, const char* end) {
    const auto first = source.find(begin), last = first == std::string::npos ? first : source.find(end, first);
    return first == std::string::npos || last == std::string::npos ? std::string{} : source.substr(first, last-first);
}
void PlatformVoiceWiring(const std::filesystem::path& root) {
    // Source-boundary checks only: no platform output device or listening claim.
    const auto java = Read(root/"android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java");
    const auto audible = Section(java, "private boolean chapterAmbienceAudible()", "private void stopChapterAmbience()");
    const auto play = Section(java, "private void playChapterAmbience(", "private void playSound(");
    Check(audible.find("hasWindowFocus()") != std::string::npos &&
          audible.find("!chapterAmbienceDialogueActive") != std::string::npos &&
          audible.find("!campaignMenuVisible") != std::string::npos &&
          audible.find("!menuVisible") != std::string::npos &&
          audible.find("sfx_enabled") != std::string::npos && audible.find("sfx_volume") != std::string::npos &&
          play.find("stopChapterAmbience();") < play.find("soundPool.play(") &&
          play.find("packedStereoGains>>>32") != std::string::npos &&
          play.find("0,0,1.f)") != std::string::npos,
          "Android finite single-voice routing retains event gains, focus/dialogue/UI/mute gates and stop-before-play");
    Check(java.find("ChapterAmbienceFeedback.isSignal(type)") != std::string::npos &&
          java.find("if(eventIndex==latestAmbienceSignal)") != std::string::npos &&
          java.find("if (paused) stopChapterAmbience();") != std::string::npos &&
          java.find("if (!chapterAmbienceAudible()) stopChapterAmbience();") != std::string::npos &&
          java.find("soundPool.stop(chapterAmbienceStream)") != std::string::npos,
          "Android coalesces the combined ambient family and retains active cleanup across suspension and eligibility loss");
    const auto windows = Read(root/"src/platform/windows/DiagnosticWindow.cpp");
    const auto drain = Section(windows, "std::uint64_t latestAmbienceSignal=0;", "case GameplayEventType::PlayerWetFootstep:");
    const auto cleanup = Section(windows, "void UpdateChapterDialogueAudio(", "using horde::gameplay::dialogue::Line;");
    Check(drain.find("IsChapterAmbienceEvent(event.type)") != std::string::npos &&
          drain.find("event.sequence!=latestAmbienceSignal") != std::string::npos &&
          drain.find("audio.StopLoop(\"chapter_ambience\")") != std::string::npos &&
          drain.find("\"chapter_ambience\",0,0,") != std::string::npos &&
          drain.find("WindowsSfxSourceGain(1.f,context.sfxVolumePercent)") != std::string::npos &&
          cleanup.find("ChapterLocalAmbience::Eligible") != std::string::npos &&
          cleanup.find("MeasurementPausedByUi(context)") != std::string::npos &&
          cleanup.find("context.sfxVolumePercent<=0") != std::string::npos &&
          cleanup.find("GetForegroundWindow()") != std::string::npos &&
          cleanup.find("StopLoop(\"chapter_ambience\")") != std::string::npos,
          "Windows shares one finite ambient key with current-state/focus/UI/mute cleanup and unchanged SFX gain");
    const auto campaign = Read(root/"src/platform/windows/windows_campaign_owner.inl");
    const auto commit = Section(campaign, "void FinishCampaignCommit(", "\n}");
    Check(commit.find("StopLoop(\"chapter_ambience\")") != std::string::npos,
          "Windows actual campaign replacement explicitly discards the old incidental voice");
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: ChapterLocalAmbienceTests <source-root>\n"; return 2; }
    Eligibility(); DripCadenceAndIdentity(); RopeTravelAndSource(); Transport();
    const auto directory = std::filesystem::path(argv[1]) / "assets/models/world/runtime/chapter-terrain-v1";
    horde::scene::assets::AssetManifest manifest; horde::scene::assets::StaticMeshAsset mesh; std::string diagnostic;
    if (!horde::scene::assets::AssetManifest::Load(directory/"asset.manifest.json", manifest, diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(directory/"chapter-terrain-lod0.runtime.glb", manifest, mesh, diagnostic)) {
        std::cerr << "Actual chapter terrain intake failed: " << diagnostic << '\n'; return 1;
    }
    const auto terrain = horde::scene::PrepareChapterTerrainSurface(mesh, diagnostic);
    if (!terrain) { std::cerr << diagnostic << '\n'; return 1; }
    ActualRope(terrain, false); ActualRope(terrain, true); ActualDrip(terrain);
    PlatformVoiceWiring(argv[1]);
    std::cout << checks << " chapter local ambience host checks, " << failures << " failures\n";
    std::cout << "Logical defeated-state restores, actual CPU terrain/motor and event transport only; "
                 "no naturally earned victory/save, listening, device or output-voice acceptance is claimed.\n";
    return failures ? 1 : 0;
}
