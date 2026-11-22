#pragma once

#include "gameplay/dialogue/ChapterDialogue.h"
#include "gameplay/campaign/TreasurePurse.h"
#include "gameplay/CorridorCollision.h"
#include "gameplay/traversal/RescueTraversal.h"
#include "gameplay/traversal/BellwetherMill.h"
#include "gameplay/healing/HealingFlask.h"
#include <array>
#include <cmath>
#include <cstdint>

namespace horde::gameplay::campaign {
enum class CheckpointId : std::uint32_t { LowerClaimed = 1, SafeExit = 2, ForestEndpoint = 3, BellwetherTavern = 4 };
inline constexpr std::uint32_t kForestContentRevision=2, kBellwetherContentRevision=3;
inline constexpr std::uint32_t kHearthSpeechSchema=6;
inline constexpr std::uint32_t kTavernConversationSchema=7;
inline constexpr std::uint32_t kWaysideArchiveSchema=8;
inline constexpr std::uint32_t kShrineLanternSchema=9;
inline constexpr std::uint32_t kWaysideRegisterSchema=10;
inline constexpr std::uint32_t kRegisterInterpretationSchema=11;
inline constexpr std::uint32_t kTavernLanternExchangeSchema=12;
inline constexpr std::uint32_t kTreasurePurseSchema=13;
inline constexpr std::uint32_t kLanternFittingSchema=14;
inline constexpr std::uint32_t kAirWardSchema=15;
inline constexpr std::uint32_t kMillPreparationSchema=16;
inline constexpr std::uint32_t kSalvagerShorePracticeSchema=17;
enum class WaysideArchiveProgress : std::uint8_t { Closed, Inspected, Open, Restored };
// The first safe standing mark beside the chair is part of revision 3's save
// contract. A later furniture or conversation edit must not move old saves.
inline constexpr traversal::Vec3 kBellwetherCheckpointPosition{84.52f,-8.709551f,107.68f};
struct GuardPose {
    float x = -4.0f, z = -15.2f, yaw = 0.0f;
    bool operator==(const GuardPose&) const = default;
};
// Logical values only. No runtime identity, pointer, timer, input or resource
// state belongs here. Schema/content changes require explicit migration.
struct Checkpoint {
    std::uint32_t schema = 1, contentRevision = 1;
    CheckpointId id = CheckpointId::LowerClaimed;
    simulation::WorldZoneId zone = simulation::WorldZoneId::Dungeon;
    traversal::Vec3 position = traversal::kLowerLanding;
    float yaw = -1.57079632679f, pitch = 0.0f;
    std::int32_t vitality = 3;
    bool halfHeart = false; // schema2+; schema1 retains exact whole-heart meaning.
    // Schema3 stores exact resources. Absent legacy resources mean an empty
    // base flask, not a new-game refill; schema1/2 bytes remain unchanged.
    healing::FlaskResources flask{false,0};
    // Schema4: UTC save-request time, stored atomically with the selected record.
    // Zero means unavailable; never substitute filesystem modification time.
    std::uint64_t savedAtUnixSeconds = 0;
    bool flaskRecorded = false; // schema4 preserves the original resource-admission boundary.
    std::uint32_t defeatedGuards = 3;
    // Schema5: exterior saves may retain bypassed tomb guards. Zero belongs to
    // a defeated actor; positive health is preserved, never converted to a kill.
    std::array<std::int32_t,2> guardHealth{};
    bool keeperDefeated = true, lanternClaimed = true, lidOpen = true;
    bool ropeReady = true, night = true, firstAscentCompleted = false;
    bool swordStowed = false, lanternHigh = false;
    float torchDropX = -1.8f, torchDropZ = -15.2f;
    float torchDropYaw = -1.57079632679f, torchDropPitch = -.08f;
    std::array<GuardPose, 2> guards{};
    dialogue::Progress dialogue{};
    WaysideArchiveProgress waysideArchive=WaysideArchiveProgress::Closed;
    bool waysideLanternRevealStarted=false;
    bool waysideRegisterRecovered=false;
    bool waysideRegisterInterpreted=false;
    TreasurePurse treasurePurse{};
    bool airWardLearned=false;
    bellwether::MillPreparation millPreparation{};
    bool salvagerShorePracticeCompleted=false;
};
inline bool HasFlaskResources(const Checkpoint& c) {
    return c.schema==3 || (c.schema>=4 && c.flaskRecorded);
}
inline simulation::WorldZoneId Destination(CheckpointId id) {
    if (id == CheckpointId::ForestEndpoint) return simulation::WorldZoneId::Lookout;
    // Bellwether is outside the narrow 1.7 route and uses the shared exterior owner.
    if (id == CheckpointId::SafeExit || id == CheckpointId::BellwetherTavern)
        return simulation::WorldZoneId::TombExterior;
    return simulation::WorldZoneId::Dungeon;
}
inline bool CompletedForestProgress(const dialogue::Progress& p) {
    return p.proofAccepted && p.forestNightConsumed && p.forestWaystoneConsumed &&
        p.forestClueConsumed && p.forestVillageConsumed &&
        p.activeLine == dialogue::Line::None && p.continuationCount == 0;
}
inline bool Validate(const Checkpoint& c) {
    const bool forest = c.id == CheckpointId::ForestEndpoint;
    const bool tavern = c.id == CheckpointId::BellwetherTavern;
    // Revision 3 retains Kit's earned seated state even when the player
    // later saves at another safe boundary. The byte layout is unchanged.
    const bool completedForest = c.contentRevision>=kForestContentRevision;
    if (c.schema < 1 || c.schema > kSalvagerShorePracticeSchema ||
        c.millPreparation>bellwether::MillPreparation::GearingVerified ||
        (c.millPreparation!=bellwether::MillPreparation::Unexamined &&
            (c.schema<kMillPreparationSchema || c.contentRevision<kBellwetherContentRevision ||
             c.dialogue.tavernConversationLinesResolved!=dialogue::kTavernConversation.size())) ||
        (c.salvagerShorePracticeCompleted &&
            (c.schema<kSalvagerShorePracticeSchema || c.contentRevision<kBellwetherContentRevision ||
             c.dialogue.tavernConversationLinesResolved!=dialogue::kTavernConversation.size())) ||
        (c.airWardLearned && c.waysideArchive!=WaysideArchiveProgress::Restored) ||
        (c.waysideArchive==WaysideArchiveProgress::Restored &&
            (c.schema<kAirWardSchema || !c.waysideRegisterInterpreted)) ||
        (c.schema<kTreasurePurseSchema && c.treasurePurse!=TreasurePurse{}) ||
        (c.treasurePurse.lanternFittingPurchased &&
            (c.schema<kLanternFittingSchema || !c.treasurePurse.registerRewardClaimed)) ||
        c.treasurePurse.coins>TreasurePurse::kMaximumCoins ||
        (c.treasurePurse.registerRewardClaimed && !c.waysideRegisterInterpreted) ||
        (c.treasurePurse.coins!=0 && !c.treasurePurse.registerRewardClaimed) ||
        (c.dialogue.tavernLanternLinesStarted &&
            (c.schema<kTavernLanternExchangeSchema || !c.waysideLanternRevealStarted)) ||
        (c.waysideRegisterInterpreted && (c.schema<kRegisterInterpretationSchema || !c.waysideRegisterRecovered)) ||
        (c.waysideRegisterRecovered && (c.schema<kWaysideRegisterSchema || !c.waysideLanternRevealStarted)) ||
        (c.waysideLanternRevealStarted && (c.schema<kShrineLanternSchema || c.waysideArchive<WaysideArchiveProgress::Open)) ||
        c.waysideArchive>WaysideArchiveProgress::Restored ||
        (c.waysideArchive!=WaysideArchiveProgress::Closed &&
            (c.schema<kWaysideArchiveSchema || c.contentRevision<kBellwetherContentRevision ||
             c.dialogue.hearthLinesResolved!=dialogue::kHearthLines.size())) ||
        (c.dialogue.hearthLinesResolved && (c.schema<kHearthSpeechSchema || c.contentRevision<kBellwetherContentRevision)) ||
        (c.dialogue.tavernConversationLinesResolved && c.schema<kTavernConversationSchema) ||
        c.savedAtUnixSeconds > 253402300799ULL || (c.schema < 4 && c.savedAtUnixSeconds != 0) ||
        (c.schema < 4 && c.flaskRecorded) ||
        (c.schema >= 4 && !c.flaskRecorded && c.flask != healing::FlaskResources{false,0}) ||
        (c.schema == 1 && c.halfHeart) || !c.flask.Valid() ||
        (c.schema < 3 && c.flask != healing::FlaskResources{false,0}) ||
        (c.contentRevision < 1 || c.contentRevision > kBellwetherContentRevision) ||
        (forest && !completedForest) ||
        (tavern && c.contentRevision!=kBellwetherContentRevision) ||
        (c.id != CheckpointId::LowerClaimed && c.id != CheckpointId::SafeExit && !forest && !tavern) ||
        c.zone != Destination(c.id) || c.vitality < 1 || c.vitality > 3 ||
        c.defeatedGuards > 3 || (c.schema<5 && c.defeatedGuards!=3) || !c.keeperDefeated || !c.lanternClaimed ||
        !c.lidOpen || !c.ropeReady || !c.night || !dialogue::Director::ValidateProgress(c.dialogue)) return false;
    const bool upper = c.id != CheckpointId::LowerClaimed;
    // Live guards remain confined to the tomb. Never create a lower-room
    // recovery point among them, or allow legacy records to gain new semantics.
    if(c.schema>=5 && !upper && c.defeatedGuards!=3)return false;
    for(std::size_t i=0;i<c.guardHealth.size();++i) {
        const auto health=c.guardHealth[i];
        if(c.schema<5 ? health!=0 :
           (health<0 || health>2 || ((c.defeatedGuards&(1u<<i))!=0)!=(health==0)))return false;
    }
    if ((upper || completedForest) && !c.firstAscentCompleted) return false;
    if (!upper && !completedForest && (c.dialogue.reunionStarted || c.dialogue.proofAccepted)) return false;
    if (completedForest && !CompletedForestProgress(c.dialogue)) return false;
    const auto endpoint = simulation::kWorldRoutePoints.back();
    // Forest height is checked against the actual terrain at restore admission,
    // rather than the earlier route blockout's nominal elevation.
    const auto safe = tavern ? kBellwetherCheckpointPosition :
        forest ? traversal::Vec3{endpoint.x,c.position.y,endpoint.z} :
        (upper ? traversal::kExteriorLanding : traversal::kLowerLanding);
    if (!std::isfinite(c.position.x) || !std::isfinite(c.position.y) || !std::isfinite(c.position.z) ||
        !std::isfinite(c.yaw) || !std::isfinite(c.pitch) || c.pitch < -.32f || c.pitch > .28f ||
        std::abs(c.position.x-safe.x) > .04f || std::abs(c.position.y-safe.y) > .001f ||
        std::abs(c.position.z-safe.z) > .04f) return false;
    for (const auto& g : c.guards)
        if (!std::isfinite(g.x) || !std::isfinite(g.z) || !std::isfinite(g.yaw) ||
            g.x < -8.5f || g.x > -2.5f || g.z < -18.0f || g.z > -12.4f) return false;
    // Preserve every original schema-1 trigger anchor, including its edges.
    const bool originalTorchAnchor=c.torchDropX>=-2.48f && c.torchDropX<=-1.78f &&
        c.torchDropZ>=-16.4f && c.torchDropZ<=-14.0f;
    if (!std::isfinite(c.torchDropX) || !std::isfinite(c.torchDropZ) ||
        !std::isfinite(c.torchDropYaw) || !std::isfinite(c.torchDropPitch) ||
        // The live torch follows the player until release, 0.70 seconds after
        // the drench trigger. Its anchor belongs to the walkable route, not
        // the trigger rectangle; walking, running or turning can leave it.
        (!originalTorchAnchor && !IsShowcasePlayerPositionWalkable(c.torchDropX,c.torchDropZ)) ||
        c.torchDropPitch < -.32f || c.torchDropPitch > .28f) return false;
    return true;
}
// Ephemeral request/readiness identity. Never part of a checkpoint record.
struct RestoreToken {
    std::uint64_t request = 0, worldGeneration = 0;
    bool operator==(const RestoreToken&) const = default;
};
} // namespace horde::gameplay::campaign
