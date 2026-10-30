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
double WalkPhase(const SimulationSnapshot& s) {
    return static_cast<double>(s.playerAnimation.locomotionTime) +
        s.playerAnimation.locomotionPhaseOffsetSeconds;
}
bool RetainsWalkCorrection(const SimulationSnapshot& before, const SimulationSnapshot& after) {
    // Dodge and traversal retain the reverse correction, while their existing
    // legacy locomotion clock may advance. Do not demand a frozen rope phase.
    const double legacyAdvance = static_cast<double>(after.playerAnimation.locomotionTime) -
        before.playerAnimation.locomotionTime;
    return after.playerAnimation.locomotionPhaseOffsetSeconds ==
               before.playerAnimation.locomotionPhaseOffsetSeconds &&
        legacyAdvance >= 0 && legacyAdvance <= kTick + .00001 &&
        std::abs((WalkPhase(after) - WalkPhase(before)) - legacyAdvance) < .00001;
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
bool BackwardTravelThenIdle(GameSimulation& sim, InputSnapshot& input, std::uint64_t& publication) {
    // Ordinary collision-resolved travel on the restored landing, not a pose
    // import. This remains close enough to use the existing rope fixture.
    input.moveForward = -1;
    bool accepted = true, reversed = true;
    float distance = 0;
    for (unsigned tick = 0; tick < 6; ++tick) {
        const auto before = sim.Snapshot();
        sim.StepFixed(input, kTick, ++publication);
        const auto& after = sim.Snapshot();
        const float dx = after.playerX - before.playerX;
        const float dz = after.playerZ - before.playerZ;
        const float travelled = std::hypot(dx, dz);
        const float forward = dx * std::sin(after.playerYawRadians) -
            dz * std::cos(after.playerYawRadians);
        accepted = accepted && travelled > .000001f && forward < -.5f * travelled &&
            after.playerGrounded && !after.rescue.equipmentStowed && !after.dodgeActive;
        reversed = reversed && WalkPhase(after) < WalkPhase(before);
        distance += travelled;
        sim.ClearEvents();
    }
    input.moveForward = 0;
    Check(accepted && distance > .05f && reversed &&
          sim.Snapshot().playerAnimation.locomotionPhaseOffsetSeconds < 0,
          "real accepted backward landing travel reverses the gait before action ownership");
    const auto walking = sim.Snapshot();
    sim.StepFixed(input, kTick, ++publication);
    Check(std::abs(WalkPhase(sim.Snapshot()) - WalkPhase(walking)) < .00001 &&
          sim.Snapshot().playerAnimation.locomotionBlend < walking.playerAnimation.locomotionBlend,
          "released backward input holds the corrected phase while returning toward idle");
    return accepted && reversed;
}
bool StartDodge(GameSimulation& sim, InputSnapshot& input, std::uint64_t& publication) {
    const auto before = sim.Snapshot();
    input.commands.dodge = sim.Snapshot().lastConsumedDodgeSequence + 1;
    input.moveStrafe = 1;
    sim.StepFixed(input, kTick, ++publication);
    const auto accepted = sim.Snapshot();
    input.moveStrafe = 0; // Accepted direction must survive release of the stick.
    sim.StepFixed(input, kTick, ++publication);
    const auto& s = sim.Snapshot();
    const bool active = s.dodgeActive && s.dodgeInvulnerable && s.playerAnimation.dodge.weight > 0;
    Check(active, "ordinary accepted dodge publishes a live visual and existing protection window");
    Check(RetainsWalkCorrection(before, accepted) && RetainsWalkCorrection(accepted, s),
          "accepting and releasing dodge direction preserves the continuous backward correction");
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
    if (!BackwardTravelThenIdle(sim, input, publication)) return;
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
    Check(RetainsWalkCorrection(dodging, entered),
          "taking rope ownership retains the backward correction without an added phase jump");

    bool clean = true, timerUnchanged = true;
    bool phaseContinuous = true;
    unsigned ticks = 0;
    while (sim.Snapshot().rescue.equipmentStowed && ticks < 1800) {
        const auto before = sim.Snapshot();
        sim.StepFixed(input, kTick, ++publication); ++ticks;
        const auto& s = sim.Snapshot();
        clean = clean && NoPose(s) && !s.dodgeInvulnerable;
        timerUnchanged = timerUnchanged && s.dodgeActive &&
            Near(s.dodgeElapsedSeconds, dodging.dodgeElapsedSeconds) &&
            s.acceptedDodgeSequence == dodging.acceptedDodgeSequence;
        phaseContinuous = phaseContinuous && RetainsWalkCorrection(before, s);
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
    Check(phaseContinuous && completed.playerAnimation.locomotionTime > entered.playerAnimation.locomotionTime,
          "rope phases and release preserve reverse correction with the existing advancing legacy clock");
    sim.StepFixed(input, kTick, ++publication);
    Check(RetainsWalkCorrection(completed, sim.Snapshot()),
          "ordinary post-rope dodge ownership does not reset the retained reverse phase");
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
    const auto idle = sim.Snapshot();
    sim.StepFixed(input, kTick, ++publication);
    Check(NoPose(sim.Snapshot()) && !sim.Snapshot().dodgeActive &&
          std::abs(WalkPhase(sim.Snapshot()) - WalkPhase(idle)) < .00001,
          "ordinary idle after rope and dodge expiry holds the corrected gait phase");
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
    if (!BackwardTravelThenIdle(sim, input, publication)) return;
    if (!StartDodge(sim, input, publication)) return;
    const auto live = sim.Snapshot();
    const auto token = sim.BeginCampaignRestore(record);
    Check(token && sim.Snapshot().tickIndex == live.tickIndex &&
          sim.Snapshot().playerAnimation.dodge == live.playerAnimation.dodge &&
          sim.Snapshot().dodgeElapsedSeconds == live.dodgeElapsedSeconds,
          "preparing a load does not cancel or mutate the retained game's live dodge");
    Check(RetainsWalkCorrection(live, sim.Snapshot()),
          "load preparation leaves the live backward phase untouched");
    if (!token) return;
    Check(!sim.CommitCampaignRestore(*token, {token->worldGeneration, WorldZoneId::TombExterior},
          terrain, input, publication), "wrong destination readiness rejects a prepared load");
    Check(sim.Snapshot().tickIndex == live.tickIndex &&
          sim.Snapshot().playerAnimation.dodge == live.playerAnimation.dodge &&
          sim.Snapshot().dodgeElapsedSeconds == live.dodgeElapsedSeconds &&
          sim.CancelCampaignRestore(*token),
          "failed and cancelled load preparation preserves the existing visual and timer");
    Check(RetainsWalkCorrection(live, sim.Snapshot()),
          "rejected and cancelled restore cannot reset the retained backward phase");
    if (!Restore(sim, record, terrain, input, publication)) {
        Check(false, "committed campaign replacement must succeed during a live dodge"); return;
    }
    Check(!sim.Snapshot().dodgeActive && !sim.Snapshot().dodgeInvulnerable && NoPose(sim.Snapshot()) &&
          sim.Snapshot().lastConsumedDodgeSequence == input.commands.dodge,
          "committed fresh restore clears dodge pose/timer and consumes the old edge");
    Check(sim.Snapshot().playerAnimation.locomotionTime == 0 &&
          sim.Snapshot().playerAnimation.locomotionPhaseOffsetSeconds == 0,
          "fresh campaign reconstruction starts a new gait phase instead of loading a transient correction");
    sim.StepFixed(input, kTick, ++publication);
    Check(!sim.Snapshot().dodgeActive && NoPose(sim.Snapshot()),
          "fresh neutral resume cannot replay the pre-load dodge edge");
    if (!BackwardTravelThenIdle(sim, input, publication)) return;
    if (!StartDodge(sim, input, publication)) return;
    const auto beforeRetry = sim.Snapshot();
    sim.RetryEncounter();
    Check(!sim.Snapshot().dodgeActive && !sim.Snapshot().dodgeInvulnerable && NoPose(sim.Snapshot()),
          "actual rescue retry clears the transient visual and existing gameplay dodge state");
    Check(RetainsWalkCorrection(beforeRetry, sim.Snapshot()) &&
          WalkPhase(beforeRetry) == WalkPhase(sim.Snapshot()),
          "safe-side rescue reconstruction preserves its current backward gait phase");

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
    Check(RetainsWalkCorrection(beforePause, sim.Snapshot()) &&
          WalkPhase(beforePause) == WalkPhase(sim.Snapshot()),
          "zero-tick pause cancels dodge without moving the corrected gait phase");
    sim.ResetRoute();
    Check(sim.Snapshot().playerAnimation.locomotionTime == 0 &&
          sim.Snapshot().playerAnimation.locomotionPhaseOffsetSeconds == 0,
          "new route reset clears the previous session's backward phase correction");
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
