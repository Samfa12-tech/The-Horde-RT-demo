#include "vulkan/raytracing/SimulationFrameAdapter.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace horde::vulkan::raytracing
{
namespace
{
void ApplyAttackWarningPresentation(
    const horde::gameplay::simulation::SimulationSnapshot& simulation,
    const RtAttackWarningOptions options, RtSceneFrameInputs& frame)
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::simulation;
    frame.attackWarningRingVisible = false;
    frame.attackWarningRingWorldFromItem = items::IdentityHeldItemTransform();
    if (!options.enabled || !simulation.modernCombatRules || simulation.paused || !simulation.playerAlive)
        return;
    const auto& warning = simulation.attackWarning;
    if (warning.phase == AttackWarningPhase::None ||
        warning != BuildAttackWarningPresentation(simulation.swordCombat, simulation.lich,
            simulation.activeEnemyKind, simulation.skeletonAttackerId, true)) return;

    // Provisional source-space chest heights and ring dimensions. These are
    // actual world geometry, with native framing/occlusion/legibility still open.
    std::array<float, 3u> centre{};
    if (warning.source == EntityId::SkeletonA || warning.source == EntityId::SkeletonB)
    {
        if (simulation.skeletonEnemyCount > simulation.skeletonEnemies.size() ||
            simulation.skeletonRevival.SuppressesActors()) return;
        const SkeletonEnemySnapshot* source = nullptr;
        for (std::size_t i = 0; i < simulation.skeletonEnemyCount; ++i)
            if (simulation.skeletonEnemies[i].id == warning.source) {
                if (source != nullptr) return; // Duplicate identity is not a second warning owner.
                source = &simulation.skeletonEnemies[i];
            }
        if (source == nullptr || source->dead || source->health <= 0 ||
            source->attackKind != warning.kind || source->actionTime != warning.phaseElapsedSeconds ||
            source->action != (warning.phase == AttackWarningPhase::Windup
                ? EnemyCombatAction::AttackWindup : EnemyCombatAction::AttackActive)) return;
        centre = {source->x, source->worldY + 0.95f, source->z};
    }
    else if (warning.source == EntityId::Lich)
        centre = {simulation.lich.x, simulation.lich.y + 0.85f, simulation.lich.z};
    else return;

    const auto torso = horde::gameplay::traversal::RescueTorsoOffset(simulation.rescue);
    // Same published walking camera offsets as rt_frame.glsl and dust culling;
    // this is view orientation only, never a separate warning countdown.
    const float step=simulation.walkTime*6.2f;
    const std::array<float, 3u> eye{simulation.playerX + torso.x + std::sin(step*.5f)*.035f*simulation.walkAmount,
        PlayerEyeWorldY(simulation.playerSupportWorldY) + torso.y - simulation.playerCrouchDropMetres +
            std::abs(std::sin(step))*.035f*simulation.walkAmount,
        simulation.playerZ + torso.z};
    for (std::size_t axis = 0; axis < 3u; ++axis)
        if (!std::isfinite(centre[axis]) || !std::isfinite(eye[axis])) return;
    std::array<float, 3u> normal{eye[0]-centre[0], eye[1]-centre[1], eye[2]-centre[2]};
    const float distanceSquared = normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2];
    if (!std::isfinite(distanceSquared) || distanceSquared < 0.000001f) return;
    const float inverseDistance = 1.0f / std::sqrt(distanceSquared);
    for (auto& value : normal) value *= inverseDistance;
    const float horizontalLength = std::hypot(normal[0], normal[2]);
    const std::array<float, 3u> right = horizontalLength > 0.0001f
        ? std::array<float, 3u>{normal[2]/horizontalLength, 0.0f, -normal[0]/horizontalLength}
        : std::array<float, 3u>{normal[1]/std::hypot(normal[0],normal[1]),
            -normal[0]/std::hypot(normal[0],normal[1]), 0.0f};
    const std::array<float, 3u> up{normal[1]*right[2]-normal[2]*right[1],
        normal[2]*right[0]-normal[0]*right[2], normal[0]*right[1]-normal[1]*right[0]};

    const bool red = warning.kind == EnemyAttackKind::Unblockable;
    const bool contact = warning.phase == AttackWarningPhase::Contact;
    const float progress = warning.phaseElapsedSeconds / warning.phaseDurationSeconds;
    const float contraction = red ? progress*progress : progress*progress*(3.0f-2.0f*progress);
    // Windup can approach closure but never manufactures Contact from a clock.
    // Reduced motion is static within each actual phase, without a countdown.
    float radius = contact ? 0.34f : options.reducedMotion ? 0.58f : 0.76f-0.34f*contraction;
    if (red && !contact && !options.reducedMotion) {
        const float pulse = std::sin(progress * 12.5663706144f);
        radius *= 1.0f + 0.10f*pulse*pulse;
    }
    const float width = radius * (red ? 0.80f : 1.0f);
    const float height = radius * (red ? 1.20f : 1.0f);
    auto transform = items::IdentityHeldItemTransform();
    for (std::size_t axis = 0; axis < 3u; ++axis) {
        transform[axis] = right[axis]*width;
        transform[4u+axis] = up[axis]*height;
        transform[8u+axis] = normal[axis];
        transform[12u+axis] = centre[axis];
    }
    frame.attackWarningRingWorldFromItem = transform;
    frame.attackWarningRingBaseColour = red
        ? std::array<float, 4u>{0.70f, 0.02f, 0.015f, 1.0f}
        : std::array<float, 4u>{0.95f, 0.52f, 0.02f, 1.0f};
    frame.attackWarningRingEmission = red
        ? std::array<float, 4u>{1.0f, 0.035f, 0.02f, 2.0f}
        : std::array<float, 4u>{1.0f, 0.62f, 0.025f, 1.4f};
    frame.attackWarningRingVisible = true;
}

float RevivalPresentationTime(const float seconds, const float duration)
{
    if (seconds >= duration) return duration; // Preserve the exact authored endpoint.
    if (seconds <= 0.0f) return 0.0f;
    // The authoritative phase clock remains 60 Hz. Pair its fixed ticks for
    // the existing 30 Hz skin/upload policy, including decreasing Dead time
    // and the exact-identity optional stand request. Rounding to a fixed tick
    // only absorbs float accumulation at tick boundaries, not gameplay time.
    const double fixedTicks = std::round(static_cast<double>(seconds) * 60.0);
    return std::min(duration, static_cast<float>(std::floor(fixedTicks / 2.0) / 30.0));
}

void ApplySkeletonRevivalPresentation(
    const horde::gameplay::simulation::SimulationSnapshot& simulation,
    RtSceneFrameInputs& frame)
{
    const auto& revival = simulation.skeletonRevival;
    if (!revival.enabled || !revival.SuppressesActors()) return;
    constexpr auto standClip = static_cast<horde::scene::SkinnedClip>(4u);
    const float deadDuration = simulation.skeletonRevivalDeadDurationSeconds;
    const float standDuration = simulation.skeletonRevivalStandDurationSeconds;
    if (!simulation.skeletonRevivalPresentationReady ||
        !std::isfinite(deadDuration) || deadDuration <= 0.0f ||
        !std::isfinite(standDuration) || standDuration <= 0.0f ||
        !std::isfinite(revival.phaseSeconds) || revival.phaseSeconds < 0.0f)
    {
        // An interrupted/unadmitted presentation cannot silently become a
        // completed corpse render. Reuse the existing finite-blend rejection
        // before upload/presentation; do not mutate the frozen source phase.
        for (auto& request : frame.skeletonPoseBlends)
            request = horde::scene::SkinnedClipBlend{
                standClip, std::numeric_limits<float>::quiet_NaN(), 1.0f};
        return;
    }
    using Phase = horde::gameplay::simulation::SkeletonRevivalPhase;
    for (std::size_t i = 0u; i < std::min(frame.skeletonEnemyCount, frame.skeletonEnemies.size()); ++i)
    {
        auto& actor = frame.skeletonEnemies[i];
        // Render-copy pose only: retained health/dead flags, IDs, roots,
        // facing, targetability and all player/gear authority stay untouched.
        actor.forceCurrentCombatPose = false;
        actor.animation = horde::gameplay::EnemyAnimation::Dead;
        actor.action = horde::gameplay::EnemyCombatAction::Dead;
        actor.animationTime = deadDuration;
        if (revival.phase == Phase::Rising)
        {
            if (revival.phaseSeconds < deadDuration)
                actor.animationTime = deadDuration - RevivalPresentationTime(revival.phaseSeconds, deadDuration);
            else
            {
                actor.animationTime = 0.0f;
                frame.skeletonPoseBlends[i] = horde::scene::SkinnedClipBlend{standClip,
                    RevivalPresentationTime(revival.phaseSeconds - deadDuration, standDuration), 1.0f};
            }
        }
        else if (revival.phase == Phase::Rearming)
        {
            actor.animation = horde::gameplay::EnemyAnimation::Idle;
            actor.action = horde::gameplay::EnemyCombatAction::Locomotion;
            actor.animationTime = actor.idlePhaseSeconds = 0.0f;
        }
    }
}
} // namespace

RtSceneFrameInputs BuildEntryMenuFrameInputs(const horde::graphics::EntryMenuSession &session,
                                             const float outputExposure,
                                             const FireEmitterQuality fireDetail,
                                             const horde::graphics::ShadowQuality shadowQuality)
{
    const auto entry = session.Snapshot();
    RtSceneFrameInputs frame{};
    frame.tickIndex = entry.tick;
    frame.cameraX = entry.camera.x;
    frame.cameraZ = entry.camera.z;
    frame.cameraYaw = entry.camera.yaw;
    frame.cameraPitch = entry.camera.pitch;
    frame.outputExposure = outputExposure * (1.0f - entry.fade);
    frame.waterQuality = WaterQuality::Off;
    frame.fireDetail = fireDetail;
    frame.shadowQuality = shadowQuality;
    frame.torchLightStrength = 0.0f;
    frame.lich.staffLightStrength = 0.0f;
    frame.rewardLanternWorldFromHinge = entry.hinge;
    frame.lanternPendulum = entry.pendulum;
    frame.fireEmitters[0] = entry.fire;
    frame.fireEmitterCount = 1;
    return frame;
}

RtSceneFrameInputs BuildGraphicsPreviewFrameInputs(
    const horde::graphics::GraphicsPreviewSession& session,
    const float outputExposure, const WaterQuality waterQuality,
    const FireEmitterQuality fireDetail, const RtSceneTuning& tuning,
    const std::optional<horde::graphics::ShadowQuality> shadowQuality)
{
    const auto preview = session.Snapshot();
    RtSceneFrameInputs frame{};
    frame.tickIndex = preview.tick;
    frame.cameraX = preview.camera.x;
    frame.cameraZ = preview.camera.z;
    frame.cameraYaw = preview.camera.yaw;
    frame.cameraPitch = preview.camera.pitch;
    frame.walkTime = static_cast<float>(preview.timeSeconds);
    frame.outputExposure = outputExposure;
    frame.waterQuality = waterQuality;
    frame.fireDetail = fireDetail;
    frame.shadowQuality = shadowQuality;
    frame.previewMotion = preview.motionTest;
    frame.torchLightStrength = 1.8f;
    frame.tuning = ClampRtSceneTuning(tuning);
    frame.lich.staffLightStrength = 0.0f;
    frame.roster.selectedEnemy = horde::gameplay::EnemyKind::Skeleton;
    frame.roster.renderedEnemies[0] = horde::gameplay::EnemyKind::Skeleton;
    frame.roster.renderedEnemyCount = 1u;
    frame.skeletonEnemyCount = 1u;
    auto& skeleton = frame.skeletonEnemies[0];
    skeleton.id = horde::gameplay::simulation::EntityId::SkeletonA;
    skeleton.x = preview.skeleton.x;
    skeleton.z = preview.skeleton.z;
    skeleton.facingRadians = preview.skeleton.facing;
    skeleton.animationTime = preview.skeleton.animationTime;
    skeleton.animation = horde::gameplay::EnemyAnimation::Idle;
    skeleton.action = horde::gameplay::EnemyCombatAction::Locomotion;
    frame.fireEmitterCount = preview.fireEmitters.size();
    std::copy(preview.fireEmitters.begin(), preview.fireEmitters.end(), frame.fireEmitters.begin());
    return frame;
}

bool ApplyFlaskUtilityCandidateFrame(
    const horde::gameplay::simulation::SimulationSnapshot& simulation,
    RtSceneFrameInputs& frame, std::string& diagnostic)
{
    std::optional<PlayerUtilitySample> sample;
    bool held=false;
    if(!simulation.healingFlaskEnabled || !simulation.flaskUtilityCandidate || frame.tickIndex!=simulation.tickIndex) {
        diagnostic="Flask presentation requires the explicitly enabled current simulation trial.";
        return false;
    }
    if(!ResolvePlayerUtilitySample(simulation.flaskHandReturn,sample,held,diagnostic))return false;
    frame.playerUtilityHand=simulation.flaskHandReturn.phase==horde::gameplay::items::UtilityHandPhase::Idle?
        std::nullopt:std::optional{simulation.flaskHandReturn};
    frame.healingFlaskVisible=held;
    frame.healingFlaskWorldFromItem=horde::gameplay::items::IdentityHeldItemTransform();
    diagnostic.clear();return true;
}

RtSceneFrameInputs BuildRtSceneFrameInputs(
    const horde::gameplay::simulation::SimulationSnapshot& simulation,
    const float outputExposure,
    const WaterQuality waterQuality,
    const RtAttackWarningOptions attackWarning)
{
    return BuildRtSceneFrameInputs(simulation, outputExposure, waterQuality,
        RtSceneTuning{}, std::nullopt, std::nullopt, attackWarning);
}

RtSceneFrameInputs BuildRtSceneFrameInputs(
    const horde::gameplay::simulation::SimulationSnapshot& simulation,
    const float outputExposure,
    const RtSceneTuning& tuning,
    const WaterQuality waterQuality,
    const std::optional<FireEmitterQuality> fireDetail,
    const std::optional<horde::graphics::ShadowQuality> shadowQuality,
    const RtAttackWarningOptions attackWarning)
{
    return BuildRtSceneFrameInputs(simulation, outputExposure, waterQuality, tuning, fireDetail, shadowQuality, attackWarning);
}

RtSceneFrameInputs BuildRtSceneFrameInputs(
    const horde::gameplay::simulation::SimulationSnapshot& simulation,
    const float outputExposure,
    const WaterQuality waterQuality,
    const RtSceneTuning& tuning,
    const std::optional<FireEmitterQuality> fireDetail,
    const std::optional<horde::graphics::ShadowQuality> shadowQuality,
    const RtAttackWarningOptions attackWarning)
{
    RtSceneFrameInputs frame;
    frame.playerRenderRoute = kProductionPlayerRenderRoute;
    frame.tickIndex = simulation.tickIndex;
    frame.developmentRescueJourney = simulation.developmentRescueJourney;
    frame.rescue = simulation.rescue;
    frame.chapterCompanion = simulation.chapterCompanion;
    frame.cameraYaw = simulation.playerYawRadians;
    frame.cameraPitch = simulation.playerPitchRadians;
    frame.torchLightStrength = simulation.torchLightStrength * simulation.torchFailure.flameStrength;
    frame.walkTime = simulation.walkTime;
    frame.playerSupportWorldY = simulation.playerSupportWorldY;
    frame.playerCrouchDropMetres=simulation.playerCrouchDropMetres;
    frame.cameraX = simulation.playerX;
    frame.cameraZ = simulation.playerZ;
    frame.walkAmount = simulation.walkAmount;
    frame.outputExposure = outputExposure;
    frame.waterQuality = waterQuality;
    frame.fireDetail = fireDetail;
    frame.shadowQuality = shadowQuality;
    frame.tuning = ClampRtSceneTuning(tuning);
    if (simulation.developmentRescueJourney)
    {
        // Owner direction: clear tomb air, subtle woodland distance haze.
        // The legacy multiplier saturates by20m and erases distant detail.
        // Real Keeper mist and all lighting/transport work remain unchanged;
        // this existing surface-fog multiplier is a reversible environment default.
        constexpr float woodlandDistanceFog = .004f;
        float exteriorBlend = simulation.rescue.exteriorSide ? 1.f : 0.f;
        using horde::gameplay::traversal::Phase;
        const auto phase = simulation.rescue.phase;
        if (phase == Phase::Ascent || phase == Phase::PullUp ||
            phase == Phase::Descent || phase == Phase::Landing)
        {
            // Both directions use the same physical camera height through
            // the aperture. There is no timer, cached state or side-flag pop.
            const float eyeY = horde::gameplay::simulation::PlayerEyeWorldY(
                simulation.playerSupportWorldY) +
                horde::gameplay::traversal::RescueTorsoOffset(simulation.rescue).y-simulation.playerCrouchDropMetres;
            if (std::isfinite(eyeY))
            {
                const float t = std::clamp((eyeY - 1.70f) / .90f, 0.f, 1.f);
                exteriorBlend = t*t*(3.f-2.f*t);
            }
        }
        frame.tuning.fogDensityScale *= woodlandDistanceFog*exteriorBlend;
    }
    frame.combat = simulation.swordCombat;
    frame.playerCombat = simulation.playerCombat;
    frame.combat.damageFlash = simulation.playerVitals.damageFlash;
    frame.skeletonEnemies = simulation.skeletonEnemies;
    frame.skeletonEnemyCount = simulation.skeletonEnemyCount;
    frame.torchFailure = simulation.torchFailure;
    frame.heldItems = simulation.heldItems;
    frame.heldItemKinematics = simulation.heldItemKinematics;
    frame.playerAnimation = simulation.playerAnimation;
    frame.playerMountProfile = simulation.playerMountProfile;
    frame.heldLight = simulation.heldLight;
    frame.interaction = simulation.interaction;
    frame.chestReward = simulation.chestReward;
    frame.finale = simulation.finale;
    frame.lanternPendulum = simulation.lanternPendulum;
    frame.rewardLanternWorldFromHinge = simulation.rewardLanternWorldFromHinge;
    frame.roster = simulation.enemyRoster;
    if (simulation.developmentRescueJourney && simulation.rescue.exteriorSide)
    {
        // Keep both authoritative waterfall skeleton snapshots in the RT
        // frame while selecting the Keeper for the exterior encounter view.
        // The retired synthetic actor replaced SkeletonA and dropped SkeletonB.
        frame.roster.selectedEnemy = horde::gameplay::EnemyKind::Lich;
    }
    frame.lich = simulation.lich;
    frame.zone = simulation.zone;
    frame.waterContact = simulation.waterContact;
    frame.fireEmitters = simulation.fireEmitters;
    frame.fireEmitterCount = simulation.fireEmitterCount;
    // A live rescue opening is authoritative collision/progression state.
    // Legacy RT Lab dawn/roof inspection overrides cannot close its admitted
    // physical traversal or turn this authored night into the demo finale.
    frame.lich.finaleSkylightOpenProgress = simulation.developmentRescueJourney
        ? simulation.lich.finaleSkylightOpenProgress
        : ResolveRtFinaleRoofOpen(simulation.lich.finaleSkylightOpenProgress, frame.tuning);
    frame.lich.finaleDawnRevealProgress = simulation.developmentRescueJourney
        ? 0.0f : ResolveRtFinaleDawnReveal(simulation.lich.finaleDawnRevealProgress, frame.tuning);
    ApplySkeletonRevivalPresentation(simulation, frame);
    ApplyAttackWarningPresentation(simulation, attackWarning, frame);
    return frame;
}

} // namespace horde::vulkan::raytracing
