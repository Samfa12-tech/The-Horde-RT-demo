#pragma once

#include "gameplay/dialogue/ChapterDialogue.h"
#include "gameplay/CorridorCollision.h"
#include "gameplay/traversal/RescueTraversal.h"
#include <array>
#include <cmath>
#include <cstdint>

namespace horde::gameplay::campaign {
enum class CheckpointId : std::uint32_t { LowerClaimed = 1, SafeExit = 2, ForestEndpoint = 3 };
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
    std::uint32_t defeatedGuards = 3;
    bool keeperDefeated = true, lanternClaimed = true, lidOpen = true;
    bool ropeReady = true, night = true, firstAscentCompleted = false;
    bool swordStowed = false, lanternHigh = false;
    float torchDropX = -1.8f, torchDropZ = -15.2f;
    float torchDropYaw = -1.57079632679f, torchDropPitch = -.08f;
    std::array<GuardPose, 2> guards{};
    dialogue::Progress dialogue{};
};
inline simulation::WorldZoneId Destination(CheckpointId id) {
    if (id == CheckpointId::ForestEndpoint) return simulation::WorldZoneId::Lookout;
    return id == CheckpointId::SafeExit ? simulation::WorldZoneId::TombExterior
                                     : simulation::WorldZoneId::Dungeon;
}
inline bool CompletedForestProgress(const dialogue::Progress& p) {
    return p.proofAccepted && p.forestNightConsumed && p.forestWaystoneConsumed &&
        p.forestClueConsumed && p.forestVillageConsumed &&
        p.activeLine == dialogue::Line::None && p.continuationCount == 0;
}
inline bool Validate(const Checkpoint& c) {
    const bool forest = c.id == CheckpointId::ForestEndpoint;
    // Revision 1 records keep their exact bytes and interpretation. Revision 2
    // retains completed forest progress at the lookout and later safe-side
    // returns, with the same logical payload. Old executables reject revision 2.
    const bool completedForest = c.contentRevision == 2;
    if (c.schema != 1 || (c.contentRevision != 1 && !completedForest) ||
        (forest && !completedForest) ||
        (c.id != CheckpointId::LowerClaimed && c.id != CheckpointId::SafeExit && !forest) ||
        c.zone != Destination(c.id) || c.vitality < 1 || c.vitality > 3 ||
        c.defeatedGuards != 3 || !c.keeperDefeated || !c.lanternClaimed ||
        !c.lidOpen || !c.ropeReady || !c.night || !dialogue::Director::ValidateProgress(c.dialogue)) return false;
    const bool upper = c.id != CheckpointId::LowerClaimed;
    if ((upper || completedForest) && !c.firstAscentCompleted) return false;
    if (!upper && !completedForest && (c.dialogue.reunionStarted || c.dialogue.proofAccepted)) return false;
    if (completedForest && !CompletedForestProgress(c.dialogue)) return false;
    const auto endpoint = simulation::kWorldRoutePoints.back();
    // Forest height is checked against the actual terrain at restore admission,
    // rather than the earlier route blockout's nominal elevation.
    const auto safe = forest ? traversal::Vec3{endpoint.x,c.position.y,endpoint.z} :
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
