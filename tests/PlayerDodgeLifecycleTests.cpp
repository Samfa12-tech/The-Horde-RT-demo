#include "gameplay/simulation/GameSimulation.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/assets/AssetManifest.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

namespace {
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
namespace cp = horde::gameplay::campaign;
namespace tr = horde::gameplay::traversal;
int checks = 0, failures = 0;
constexpr float kTick = 1.0f / 60.0f;

void Check(bool value, const char* message) {
    ++checks;
    if (!value) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
bool Near(float a, float b) { return std::abs(a - b) < .00001f; }
bool NoPose(const SimulationSnapshot& s) {
    return s.playerAnimation.dodge == animation::PlayerDodgePresentation{};
}
cp::Checkpoint Record(bool upper) {
    // A validated logical restore fixture, not evidence of earning the chapter.
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
InputSnapshot Neutral(const cp::Checkpoint& record) {
    InputSnapshot in;
    in.damageEnabled = false; in.tutorialEnabled = false;
    in.yawRadians = record.yaw; in.pitchRadians = record.pitch;
    return in;
}
bool StartDodge(GameSimulation& sim, InputSnapshot& input, std::uint64_t& publication) {
    input.commands.dodge = sim.Snapshot().lastConsumedDodgeSequence + 1;
    input.moveStrafe = 1;
    sim.StepFixed(input, kTick, ++publication);
    input.moveStrafe = 0; // Accepted direction must survive release of the stick.
    sim.StepFixed(input, kTick, ++publication);
    const auto& s = sim.Snapshot();
    const bool active = s.dodgeActive && s.dodgeInvulnerable && s.playerAnimation.dodge.weight > 0;
    Check(active, "ordinary accepted dodge publishes a live visual and existing protection window");
    return active;
}
void RopeLifecycle(const std::shared_ptr<const TriangleSupportSurface>& terrain, bool upper) {
    GameSimulation sim(ProductionGameSimulationConfig());
    const auto record = Record(upper);
    auto input = Neutral(record);
    std::uint64_t publication = 100;
    if (!Restore(sim, record, terrain, input, publication)) {
        Check(false, "safe-side logical fixture must restore on actual runtime terrain"); return;
    }
    sim.StepFixed(input, kTick, ++publication); // Fresh neutral publication opens the restore barrier.
    const auto destination = upper ? WorldZoneId::Dungeon : WorldZoneId::TombExterior;
    const WorldZoneToken ready{sim.Snapshot().worldRoute.generation, destination};
    if (!(upper ? sim.PublishWorldZoneReadiness(ready, ZoneReadiness::Ready)
                : sim.PublishChapterTerrain(ready, terrain))) {
        Check(false, "real traversal destination requires current owner readiness"); return;
    }
    if (!StartDodge(sim, input, publication)) return;
    const auto dodging = sim.Snapshot();
    ++input.commands.interact;
    sim.StepFixed(input, kTick, ++publication);
    const auto entered = sim.Snapshot();
    Check(entered.rescue.equipmentStowed,
          "ordinary context input starts the actual rope motor during an active dodge");
    if (!entered.rescue.equipmentStowed) return;
    Check(NoPose(entered) && !entered.dodgeInvulnerable,
          "rope ownership immediately cancels the dodge visual and protection");
    Check(entered.dodgeActive && Near(entered.dodgeElapsedSeconds, dodging.dodgeElapsedSeconds) &&
          entered.acceptedDodgeSequence == dodging.acceptedDodgeSequence,
          "visual cancellation preserves the existing suspended gameplay timer and accepted identity");

    bool clean = true, timerUnchanged = true;
    unsigned ticks = 0;
    while (sim.Snapshot().rescue.equipmentStowed && ticks < 1800) {
        sim.StepFixed(input, kTick, ++publication); ++ticks;
        const auto& s = sim.Snapshot();
        clean = clean && NoPose(s) && !s.dodgeInvulnerable;
        timerUnchanged = timerUnchanged && s.dodgeActive &&
            Near(s.dodgeElapsedSeconds, dodging.dodgeElapsedSeconds) &&
            s.acceptedDodgeSequence == dodging.acceptedDodgeSequence;
        sim.ClearEvents();
    }
    const auto completed = sim.Snapshot();
    Check(ticks > 0 && ticks < 1800 && !completed.rescue.equipmentStowed &&
          completed.rescue.exteriorSide != upper && completed.playerGrounded &&
          completed.rescue.firstAscentCompleted &&
          (upper ? completed.rescue.sawDescent && completed.rescue.sawLanding
                 : completed.rescue.sawAscent && completed.rescue.sawPullUp),
          "real ascent/descent finishes through its physical motor phases on the other safe side");
    Check(clean && NoPose(completed), "no dodge visual returns during rope motion or its completion tick");
    Check(timerUnchanged, "the new visual layer must not advance or clear the existing rope-suspended timer");
    sim.StepFixed(input, kTick, ++publication);
    Check(sim.Snapshot().dodgeActive && !sim.Snapshot().dodgeInvulnerable && NoPose(sim.Snapshot()) &&
          sim.Snapshot().dodgeElapsedSeconds > completed.dodgeElapsedSeconds &&
          sim.Snapshot().acceptedDodgeSequence == dodging.acceptedDodgeSequence,
          "first ordinary post-rope tick resumes the old timer without resurrecting pose or protection");
    for (unsigned tick = 0; tick < 16; ++tick) {
        sim.StepFixed(input, kTick, ++publication);
        clean = clean && NoPose(sim.Snapshot()) && !sim.Snapshot().dodgeInvulnerable;
    }
    Check(clean && !sim.Snapshot().dodgeActive,
          "cancelled pose stays absent through the unchanged timer's eventual expiry");
}
void RestoreRetryAndPause(const std::shared_ptr<const TriangleSupportSurface>& terrain) {
    GameSimulation sim(ProductionGameSimulationConfig());
    const auto record = Record(false);
    auto input = Neutral(record);
    std::uint64_t publication = 100;
    if (!Restore(sim, record, terrain, input, publication)) {
        Check(false, "lifecycle fixture must restore on actual runtime terrain"); return;
    }
    sim.StepFixed(input, kTick, ++publication);
    if (!StartDodge(sim, input, publication)) return;
    const auto live = sim.Snapshot();
    const auto token = sim.BeginCampaignRestore(record);
    Check(token && sim.Snapshot().tickIndex == live.tickIndex &&
          sim.Snapshot().playerAnimation.dodge == live.playerAnimation.dodge &&
          sim.Snapshot().dodgeElapsedSeconds == live.dodgeElapsedSeconds,
          "preparing a load does not cancel or mutate the retained game's live dodge");
    if (!token) return;
    Check(!sim.CommitCampaignRestore(*token, {token->worldGeneration, WorldZoneId::TombExterior},
          terrain, input, publication), "wrong destination readiness rejects a prepared load");
    Check(sim.Snapshot().tickIndex == live.tickIndex &&
          sim.Snapshot().playerAnimation.dodge == live.playerAnimation.dodge &&
          sim.Snapshot().dodgeElapsedSeconds == live.dodgeElapsedSeconds &&
          sim.CancelCampaignRestore(*token),
          "failed and cancelled load preparation preserves the existing visual and timer");
    if (!Restore(sim, record, terrain, input, publication)) {
        Check(false, "committed campaign replacement must succeed during a live dodge"); return;
    }
    Check(!sim.Snapshot().dodgeActive && !sim.Snapshot().dodgeInvulnerable && NoPose(sim.Snapshot()) &&
          sim.Snapshot().lastConsumedDodgeSequence == input.commands.dodge,
          "committed fresh restore clears dodge pose/timer and consumes the old edge");
    sim.StepFixed(input, kTick, ++publication);
    Check(!sim.Snapshot().dodgeActive && NoPose(sim.Snapshot()),
          "fresh neutral resume cannot replay the pre-load dodge edge");
    if (!StartDodge(sim, input, publication)) return;
    sim.RetryEncounter();
    Check(!sim.Snapshot().dodgeActive && !sim.Snapshot().dodgeInvulnerable && NoPose(sim.Snapshot()),
          "actual rescue retry clears the transient visual and existing gameplay dodge state");

    // Rescue reconstruction retains the existing cooldown. Let ordinary fixed
    // ticks expire it; the visual test must not demand a gameplay reset here.
    input.yawRadians = sim.Snapshot().playerYawRadians;
    input.pitchRadians = sim.Snapshot().playerPitchRadians;
    for (unsigned tick = 0; tick < 40; ++tick) {
        sim.StepFixed(input, kTick, ++publication);
        sim.ClearEvents();
    }
    if (!StartDodge(sim, input, publication)) return;
    const auto beforePause = sim.Snapshot();
    input.paused = true;
    const auto ticks = sim.AdvanceFrame(input, 0.0, ++publication);
    Check(ticks == 0 && sim.Snapshot().tickIndex == beforePause.tickIndex &&
          !sim.Snapshot().dodgeActive && !sim.Snapshot().dodgeInvulnerable && NoPose(sim.Snapshot()),
          "zero-tick pause publishes immediate cancellation without advancing gameplay");
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: PlayerDodgeLifecycleTests <source-root>\n"; return 2; }
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
    RopeLifecycle(terrain, false);
    RopeLifecycle(terrain, true);
    RestoreRetryAndPause(terrain);
    std::cout << checks << " dodge lifecycle checks, " << failures << " failures\n";
    std::cout << "CPU restore fixtures and actual rope motor only; same-tick hostile death is not covered.\n";
    return failures ? 1 : 0;
}
