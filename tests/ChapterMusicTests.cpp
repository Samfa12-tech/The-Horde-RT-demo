#include "audio/MusicDirector.h"
#include "audio/MusicPlaybackSession.h"
#include "gameplay/simulation/GameSimulation.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/assets/AssetManifest.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace {
using namespace horde::audio;
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
namespace cp = horde::gameplay::campaign;
namespace tr = horde::gameplay::traversal;
using Phase = interactions::FinaleSequencePhase;
constexpr float kTick = 1.0f / 60.0f;
int checks = 0, failures = 0;
void Check(bool condition, const char* message) {
    ++checks;
    if (!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
bool Near(double a, double b, double epsilon = .000001) { return std::abs(a - b) <= epsilon; }
SimulationSnapshot Chapter(bool upper = false) {
    SimulationSnapshot state;
    state.developmentRescueJourney = true; state.playerAlive = true;
    state.lich.phase = LichPhase::Dead; state.finale.lichDefeated = true;
    state.openingEncounterComplete = true;
    state.rescue.lanternClaimed = true; state.rescue.exteriorSide = upper;
    state.rescue.firstAscentCompleted = upper;
    state.torchFailure.heldByPlayer = false; state.torchFailure.phase = TorchFailurePhase::Settled;
    return state;
}
void DirectChapterTransitions() {
    MusicDirector director;
    auto state = Chapter();
    auto selection = director.Update(state, {}, 10, 1);
    Check(selection.cue == MusicCue::F && selection.looping && Near(selection.positionSeconds, 0),
          "post-Keeper chapter interior starts the existing F bed");
    const auto lowerRevision = selection.revision;
    double clock = 10;
    for (const auto phase : {Phase::Inactive, Phase::LichFalling, Phase::RaisingLantern,
                            Phase::RevealingLantern, Phase::SkylightOpening, Phase::DawnRevealed, Phase::Complete}) {
        state.finale.phase = phase; state.finale.phaseTime = 99; state.finaleComplete = true;
        selection = director.Update(state, {}, clock += .25, 1);
        Check(selection.cue == MusicCue::F && selection.revision == lowerRevision &&
              Near(selection.positionSeconds, clock - 10),
              "stale legacy ending phases cannot start G/H or restart interior F");
    }
    state.rescue.exteriorSide = state.rescue.firstAscentCompleted = true;
    const double arrival = clock += .25;
    selection = director.Update(state, {}, arrival, 1);
    const auto revealRevision = selection.revision;
    Check(selection.cue == MusicCue::G && !selection.looping && Near(selection.positionSeconds, 0) &&
          revealRevision > lowerRevision, "observed first completed exterior ascent starts G once");
    state.finale.phase = Phase::Inactive; state.finale.phaseTime = 0;
    selection = director.Update(state, {}, arrival + 5.99, 1);
    Check(selection.cue == MusicCue::G && Near(selection.positionSeconds, 5.99) &&
          selection.revision == revealRevision, "chapter G lasts through 5.99 seconds of audio content time");
    state.finale.phaseTime = std::numeric_limits<float>::quiet_NaN();
    selection = director.Update(state, {}, arrival + 6.0, 1);
    Check(selection.cue == MusicCue::H && selection.looping && Near(selection.positionSeconds, 0) &&
          selection.revision > revealRevision, "G hands off at exactly six seconds without legacy phase-time seeking");
    const auto forestRevision = selection.revision;
    state.worldRoute.current = WorldZoneId::Lookout;
    selection = director.Update(state, {}, arrival + 31, 1);
    Check(selection.cue == MusicCue::H && Near(selection.positionSeconds, 1) &&
          selection.revision == forestRevision, "forest/lookout updates retain the same twelve-second H loop");
    ++state.worldRoute.generation;
    selection = director.Update(state, {}, arrival + 31.25, 1);
    Check(selection.cue == MusicCue::H && Near(selection.positionSeconds, 1.25) &&
          selection.revision == forestRevision, "terrain readiness generation changes do not restart chapter music");
    state.rescue.exteriorSide = false;
    selection = director.Update(state, {}, arrival + 32, 1);
    Check(selection.cue == MusicCue::F && selection.looping,
          "completed return to the tomb selects F instead of a latched showcase H");
    state.rescue.exteriorSide = true;
    selection = director.Update(state, {}, arrival + 33, 1);
    Check(selection.cue == MusicCue::H && selection.looping,
          "repeat ascent selects H without replaying the first-arrival G");
    for (const bool upper : {false, true}) {
        MusicDirector late;
        selection = late.Update(Chapter(upper), {}, 500, 2);
        Check(selection.cue == (upper ? MusicCue::H : MusicCue::F) && selection.looping &&
              Near(selection.positionSeconds, 0), "fresh lower/upper attachment reconstructs its bed without a reveal");
    }
}
void NaturalRewardPhaseContinuity() {
    interactions::FinaleSequence finale;
    Check(finale.NotifyLichDefeated() && finale.NotifyLanternClaimed(),
          "shared reward sequence admits the real defeat-to-claim phase progression");
    auto state = Chapter();
    MusicDirector director;
    std::uint64_t revision = 0;
    bool continuous = true, sawOpening = false;
    for (unsigned tick = 0; tick <= 720; ++tick) {
        state.finale = finale.Update(kTick);
        state.finaleComplete = state.finale.phase == Phase::Complete;
        sawOpening |= state.finale.phase == Phase::SkylightOpening;
        const auto selection = director.Update(state, {}, tick / 60.0, 19);
        if (!tick) revision = selection.revision;
        continuous &= selection.cue == MusicCue::F && selection.looping &&
                      selection.revision == revision;
    }
    Check(continuous && sawOpening && finale.Snapshot().phase == Phase::Complete,
          "natural shared reward/reveal phases retain the same interior bed as a restored lower checkpoint");
}
void SuspensionAndInterruptions() {
    for (const bool externalMenu : {false, true}) {
        MusicDirector director;
        auto state = Chapter(); director.Update(state, {}, 0, 5);
        state.rescue.firstAscentCompleted = state.rescue.exteriorSide = true;
        director.Update(state, {}, 1, 5);
        auto selection = director.Update(state, {}, 3, 5);
        const auto revision = selection.revision;
        state.paused = !externalMenu;
        selection = director.Update(state, {}, 100, 5, externalMenu);
        Check(selection.suspended && selection.cue == MusicCue::G && Near(selection.positionSeconds, 2) &&
              selection.revision == revision, "pause or external menu freezes the active reveal without a restart");
        state.finale.phase = Phase::Complete; state.finale.phaseTime = 123;
        selection = director.Update(state, {}, 150, 5, externalMenu);
        Check(selection.suspended && Near(selection.positionSeconds, 2) && selection.revision == revision,
              "suspended legacy phase changes cannot seek the chapter reveal");
        state.paused = false;
        selection = director.Update(state, {}, 200, 5);
        Check(!selection.suspended && Near(selection.positionSeconds, 2) && selection.revision == revision,
              "first resume publication excludes the wall-clock menu interval");
        selection = director.Update(state, {}, 203.99, 5);
        Check(selection.cue == MusicCue::G && Near(selection.positionSeconds, 5.99),
              "only resumed content time finishes the remaining G body");
        selection = director.Update(state, {}, 204, 5);
        Check(selection.cue == MusicCue::H && Near(selection.positionSeconds, 0),
              "resumed reveal has one exact G-to-H handoff");
        const auto bedRevision = selection.revision;
        selection = director.Update(state, {}, 205, 5, true);
        selection = director.Update(state, {}, 1000, 5, true);
        selection = director.Update(state, {}, 1100, 5);
        Check(selection.cue == MusicCue::H && Near(selection.positionSeconds, 0) &&
              selection.revision == bedRevision, "ordinary menu return also preserves the forest-bed cursor and revision");
    }
    MusicDirector director;
    auto state = Chapter(); director.Update(state, {}, 0, 7);
    state.rescue.firstAscentCompleted = state.rescue.exteriorSide = true;
    director.Update(state, {}, 1, 7); director.Update(state, {}, 2, 7);
    state.rescue.exteriorSide = false;
    auto selection = director.Update(state, {}, 3, 7);
    Check(selection.cue == MusicCue::F, "early interior return cancels G before its body completes");
    state.rescue.exteriorSide = true;
    selection = director.Update(state, {}, 4, 7);
    Check(selection.cue == MusicCue::H, "cancelled arrival reveal cannot replay on the next ascent");
    state = Chapter(); ++state.retryGeneration;
    selection = director.Update(state, {}, 5, 7);
    const auto retryRevision = selection.revision;
    Check(selection.cue == MusicCue::F && selection.discontinuity, "retry identity clears prior chapter reveal state");
    state.rescue.firstAscentCompleted = state.rescue.exteriorSide = true;
    selection = director.Update(state, {}, 6, 7);
    Check(selection.cue == MusicCue::G && selection.revision > retryRevision,
          "a new retry generation may observe its own first completed ascent");
    state.playerAlive = false;
    selection = director.Update(state, {}, 7, 7);
    Check(selection.cue == MusicCue::None, "death interrupts the reveal to silence");
    state.playerAlive = true;
    selection = director.Update(state, {}, 8, 7);
    Check(selection.cue == MusicCue::H, "alive restoration does not resurrect an interrupted reveal");
    state.developmentRescueJourney = false; state.finale.phase = Phase::SkylightOpening; state.finale.phaseTime = 0;
    const auto beforeMode = selection.revision;
    selection = director.Update(state, {}, 9, 7);
    Check(selection.cue == MusicCue::G && selection.discontinuity && selection.revision > beforeMode,
          "leaving chapter mode retains the legacy showcase opening policy with a fresh identity");
    state = Chapter();
    selection = director.Update(state, {}, 10, 7);
    Check(selection.cue == MusicCue::F && selection.revision > beforeMode,
          "chapter re-entry discards the legacy G/H latch while revisions stay monotonic");
    state.lastConsumedRouteResetSequence = 1;
    selection = director.Update(state, {}, 11, 7);
    Check(selection.cue == MusicCue::F && selection.discontinuity,
          "route reset preserves the existing restart contract");
}

struct Clips {
    std::array<std::vector<std::int16_t>, kMusicPcmCueCount> body, tail;
    std::array<MusicPcmClip, kMusicPcmCueCount> clips{};
    Clips() {
        for (std::size_t cue = 1; cue < clips.size(); ++cue) {
            body[cue].assign(kMusicPcmAssets[cue].bodyFrames * 2u, static_cast<std::int16_t>(cue * 100));
            tail[cue].assign(kMusicPcmTailFrames * 2u, static_cast<std::int16_t>(cue * 10));
            clips[cue] = {body[cue], tail[cue]};
        }
    }
};
bool RenderFrames(MusicPlaybackSession& session, const MusicPlaybackInput& packet, std::size_t frames) {
    std::array<float, 8192> output{};
    while (frames) {
        const auto count = std::min(frames, output.size() / 2);
        if (session.Render(packet, std::span(output).first(count * 2)) != MusicPcmStatus::Ok) return false;
        frames -= count;
    }
    return true;
}
void InboxAndSyntheticPcm() {
    const Clips clips;
    MusicPlaybackInbox inbox;
    MusicPlaybackSession session(clips.clips);
    auto state = Chapter(); state.tickIndex = 100;
    GameplayEvent old{.sequence = 500, .type = GameplayEventType::PlayerSwing};
    Check(inbox.Publish(state, std::span(&old, 1), 42, 0, false), "inbox accepts the current chapter lifetime");
    auto packet = inbox.Take(); const auto epoch = packet.restartEpoch;
    Check(RenderFrames(session, packet, 4800) && session.Selection().cue == MusicCue::F,
          "synthetic PCM begins with the real session's lower-route selection");
    state.rescue.firstAscentCompleted = state.rescue.exteriorSide = true; ++state.tickIndex;
    inbox.Publish(state, {}, 42, 0, false); packet = inbox.Take();
    Check(session.Observe(packet) == MusicPcmStatus::Ok && session.Selection().cue == MusicCue::G,
          "copied first-ascent state reaches the worker resolver");
    const auto revision = session.Selection().revision;
    Check(RenderFrames(session, packet, 1000), "synthetic G body starts without a device");
    ++state.tickIndex; inbox.Publish(state, {}, 42, 0, true); packet = inbox.Take();
    std::array<float, 64> silence; silence.fill(1);
    const auto frozenFrames = session.GeneratedFrames();
    Check(session.Render(packet, silence) == MusicPcmStatus::Suspended &&
          std::all_of(silence.begin(), silence.end(), [](float value) { return value == 0; }) &&
          session.GeneratedFrames() == frozenFrames && session.Selection().revision == revision &&
          packet.restartEpoch == epoch, "ordinary menu suspension freezes PCM without restarting its epoch or revision");
    ++state.tickIndex; inbox.Publish(state, {}, 42, 0, false); packet = inbox.Take();
    Check(RenderFrames(session, packet, 286999) && session.Selection().cue == MusicCue::G &&
          session.Selection().revision == revision && packet.restartEpoch == epoch,
          "ordinary resume retains G through its final body sample without a new epoch");
    std::array<float, 4> seam{};
    Check(session.Render(packet, seam) == MusicPcmStatus::Ok && session.Selection().cue == MusicCue::H &&
          session.GeneratedFrames() == 4800u + 288001u &&
          Near(seam[0], 700.0 / 32768.0) && Near(seam[2], 870.0 / 32768.0),
          "one buffer contains exact last G sample then first H plus G tail, without a gap");
    const auto hRevision = session.Selection().revision;
    Check(RenderFrames(session, packet, 1000) && session.Selection().cue == MusicCue::H &&
          session.Selection().revision == hRevision, "stale completed-ascent snapshots cannot restart G");
    state.tickIndex = 0; old.sequence = 1;
    Check(!inbox.Publish(state, std::span(&old, 1), 42, 0, false),
          "ordinary lower-tick publication cannot impersonate a new queue lifetime");
    Check(inbox.Publish(state, std::span(&old, 1), 43, 0, false), "actual new lifetime admits restarted event identity");
    packet = inbox.Take();
    Check(packet.restartEpoch > epoch && packet.eventCount == 1 && packet.events[0].sequence == 1 &&
          session.Observe(packet) == MusicPcmStatus::Ok && session.GeneratedFrames() == 0 &&
          session.Selection().cue == MusicCue::H && session.Selection().revision > hRevision,
          "explicit queue replacement discards prior high-water and PCM, selecting loaded H without G");
}

cp::Checkpoint Record(bool upper) {
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
void ActualRopeAndRestores(const std::shared_ptr<const TriangleSupportSurface>& terrain) {
    GameSimulation sim(ProductionGameSimulationConfig());
    auto record = Record(false);
    InputSnapshot input; input.damageEnabled = false; input.tutorialEnabled = false; input.yawRadians = record.yaw;
    std::uint64_t publication = 100, lifetime = 60;
    if (!Restore(sim, record, terrain, input, publication)) {
        Check(false, "logical lower fixture must admit actual terrain before music traversal"); return;
    }
    MusicDirector director; double clock = 0; unsigned gStarts = 0;
    MusicCue previousCue = MusicCue::None;
    const auto observe = [&]() {
        const auto selection = director.Update(sim.Snapshot(), sim.Events().Events(), clock, lifetime);
        if (selection.cue == MusicCue::G && previousCue != MusicCue::G) ++gStarts;
        previousCue = selection.cue; sim.ClearEvents(); return selection;
    };
    auto selection = observe();
    Check(selection.cue == MusicCue::F, "committed lower Load selects F from the real shared snapshot");
    sim.StepFixed(input, kTick, ++publication); clock += kTick; observe();
    Check(sim.PublishChapterTerrain({sim.Snapshot().worldRoute.generation, WorldZoneId::TombExterior}, terrain),
          "actual ascent fixture supplies current exterior readiness");
    const auto traverse = [&]() {
        ++input.commands.interact; sim.StepFixed(input, kTick, ++publication); clock += kTick; observe();
        if (!sim.Snapshot().rescue.equipmentStowed) return false;
        unsigned ticks = 0;
        while (sim.Snapshot().rescue.equipmentStowed && ticks < 1800) {
            sim.StepFixed(input, kTick, ++publication); clock += kTick; selection = observe(); ++ticks;
        }
        return ticks > 0 && ticks < 1800 && sim.Snapshot().playerGrounded;
    };
    Check(traverse() && sim.Snapshot().rescue.firstAscentCompleted && sim.Snapshot().rescue.exteriorSide &&
          selection.cue == MusicCue::G && gStarts == 1,
          "actual completed first rope motor ascent starts exactly one chapter reveal");
    for (unsigned tick = 0; tick < 370; ++tick) {
        sim.StepFixed(input, kTick, ++publication); clock += kTick; selection = observe();
    }
    Check(selection.cue == MusicCue::H && gStarts == 1, "neutral exterior ticks finish G once on the content clock");
    Check(sim.PublishWorldZoneReadiness({sim.Snapshot().worldRoute.generation, WorldZoneId::Dungeon}, ZoneReadiness::Ready),
          "actual descent fixture supplies current Dungeon readiness");
    Check(traverse() && !sim.Snapshot().rescue.exteriorSide && selection.cue == MusicCue::F && gStarts == 1,
          "actual completed return descent restores interior F without a reveal replay");
    Check(traverse() && sim.Snapshot().rescue.exteriorSide && selection.cue == MusicCue::H && gStarts == 1,
          "actual repeat ascent retains durable first-ascent identity and selects H directly");

    std::array<cp::Checkpoint, 3> records{Record(false), Record(true), Record(true)};
    auto& endpoint = records[2]; endpoint.contentRevision = 2; endpoint.id = cp::CheckpointId::ForestEndpoint;
    endpoint.zone = WorldZoneId::Lookout; const auto goal = kWorldRoutePoints.back();
    const auto ground = terrain->Query(goal.x, goal.z);
    Check(ground.found && ground.walkable, "forest restore fixture uses actual imported lookout support");
    endpoint.position = {goal.x, ground.worldY, goal.z}; endpoint.yaw = 2.51f; endpoint.pitch = -.15f;
    endpoint.dialogue.waitingForRaise = false; endpoint.dialogue.proofAccepted = true;
    endpoint.dialogue.forestNightConsumed = endpoint.dialogue.forestWaystoneConsumed = true;
    endpoint.dialogue.forestClueConsumed = endpoint.dialogue.forestVillageConsumed = true;
    for (const auto& saved : records) {
        input.yawRadians = saved.yaw; input.pitchRadians = saved.pitch;
        if (!Restore(sim, saved, terrain, input, publication)) {
            Check(false, "actual lower, upper or completed-forest logical restore must commit"); continue;
        }
        ++lifetime; clock += 10;
        selection = observe();
        const auto expected = saved.id == cp::CheckpointId::LowerClaimed ? MusicCue::F : MusicCue::H;
        Check(selection.cue == expected && selection.looping && Near(selection.positionSeconds, 0) && gStarts == 1,
              "committed checkpoint reconstruction starts its bed without replaying a natural-arrival reveal");
        sim.StepFixed(input, kTick, ++publication); clock += kTick; selection = observe();
        Check(selection.cue == expected && gStarts == 1,
              "fresh neutral post-load publication preserves the reconstructed chapter bed");
    }
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: ChapterMusicTests <source-root>\n"; return 2; }
    DirectChapterTransitions();
    NaturalRewardPhaseContinuity();
    SuspensionAndInterruptions();
    InboxAndSyntheticPcm();
    const auto directory = std::filesystem::path(argv[1]) / "assets/models/world/runtime/chapter-terrain-v1";
    horde::scene::assets::AssetManifest manifest;
    horde::scene::assets::StaticMeshAsset mesh; std::string diagnostic;
    if (!horde::scene::assets::AssetManifest::Load(directory / "asset.manifest.json", manifest, diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(directory / "chapter-terrain-lod0.runtime.glb", manifest, mesh, diagnostic)) {
        std::cerr << "Actual chapter terrain intake failed: " << diagnostic << '\n'; return 1;
    }
    const auto terrain = horde::scene::PrepareChapterTerrainSurface(mesh, diagnostic);
    if (!terrain) { std::cerr << diagnostic << '\n'; return 1; }
    ActualRopeAndRestores(terrain);
    std::cout << checks << " chapter music host checks, " << failures << " failures\n";
    std::cout << "Synthetic PCM has no output device. Rope/terrain and logical restores are CPU fixtures, "
                 "not natural victory, physically earned saves, listening or platform playback acceptance.\n";
    return failures ? 1 : 0;
}
