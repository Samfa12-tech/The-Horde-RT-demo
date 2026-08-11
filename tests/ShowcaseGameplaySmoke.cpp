#include <cmath>
#include <iostream>
#include <string>

#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/ShowcaseCheckpoints.h"
#include "gameplay/CorridorCollision.h"

namespace
{

bool NearlyEqual(float left, float right, float epsilon = 0.001f)
{
    return std::abs(left - right) <= epsilon;
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
            std::cerr << "Showcase gameplay smoke failed: " << message << '\n';
        }
    };

    PlayerVitals vitals;
    check(vitals.Snapshot().phase == PlayerLifePhase::Alive &&
          vitals.Snapshot().vitality == PlayerVitals::kMaxVitality &&
          vitals.Snapshot().maxVitality == PlayerVitals::kMaxVitality,
          "player vitality must begin alive and full");
    check(vitals.TryApplyDamage() == PlayerDamageResult::Damaged &&
          vitals.Snapshot().vitality == 2 && NearlyEqual(vitals.Snapshot().damageFlash, 1.0f),
          "first accepted hit must remove one vitality and start feedback");
    check(vitals.TryApplyDamage() == PlayerDamageResult::Ignored && vitals.Snapshot().vitality == 2,
          "invulnerability must reject an immediate repeated hit");
    for (int frame = 0; frame < 19; ++frame)
    {
        vitals.Update(0.05f);
    }
    check(vitals.TryApplyDamage() == PlayerDamageResult::Ignored && vitals.Snapshot().vitality == 2,
          "invulnerability must remain active before one second");
    vitals.Update(0.05f);
    check(vitals.TryApplyDamage() == PlayerDamageResult::Damaged && vitals.Snapshot().vitality == 1,
          "damage must be accepted when the one-second guard expires");
    for (int frame = 0; frame < 20; ++frame)
    {
        vitals.Update(0.05f);
    }
    check(vitals.TryApplyDamage() == PlayerDamageResult::Killed &&
          vitals.Snapshot().vitality == 0 && vitals.Snapshot().phase == PlayerLifePhase::Dying,
          "third accepted hit must begin the fatal hold");
    for (int frame = 0; frame < 12; ++frame)
    {
        vitals.Update(0.05f);
    }
    check(vitals.Snapshot().phase == PlayerLifePhase::Dying,
          "fatal hold must keep the RT scene visible before 0.65 seconds");
    vitals.Update(0.05f);
    check(vitals.Snapshot().phase == PlayerLifePhase::Dead,
          "fatal hold must end in the dead phase at 0.65 seconds");
    check(vitals.TryApplyDamage() == PlayerDamageResult::Ignored,
          "dead players must reject further damage");
    vitals.Update(10.0f);
    check(vitals.Snapshot().phase == PlayerLifePhase::Dead &&
          vitals.Snapshot().vitality == 0,
          "oversized updates must not revive a dead player");
    vitals.ResetForEncounter();
    check(vitals.Snapshot().phase == PlayerLifePhase::Alive &&
          vitals.Snapshot().vitality == PlayerVitals::kMaxVitality &&
          NearlyEqual(vitals.Snapshot().invulnerabilityRemaining, 0.0f) &&
          NearlyEqual(vitals.Snapshot().damageFlash, 0.0f) &&
          NearlyEqual(vitals.Snapshot().deathHoldRemaining, 0.0f),
          "encounter reset must restore full vitality and clear every timer");

    PlayerVitals clampedFatalHold;
    check(clampedFatalHold.TryApplyDamage() == PlayerDamageResult::Damaged,
          "clamp test first hit must be accepted");
    for (int hit = 0; hit < 2; ++hit)
    {
        for (int frame = 0; frame < 20; ++frame)
        {
            clampedFatalHold.Update(0.05f);
        }
        clampedFatalHold.TryApplyDamage();
    }
    clampedFatalHold.Update(10.0f);
    check(clampedFatalHold.Snapshot().phase == PlayerLifePhase::Dying &&
          NearlyEqual(clampedFatalHold.Snapshot().deathHoldRemaining, 0.60f, 0.001f),
          "oversized updates must clamp to 50 ms instead of skipping the fatal hold");

    const ShowcaseCheckpoint* skeletonRetry = FindShowcaseCheckpoint(0);
    const ShowcaseCheckpoint* lichRetry = FindShowcaseCheckpoint(9);
    const ShowcaseCheckpoint* twoEnemyCombat = FindShowcaseCheckpoint("two-enemy-combat");
    check(skeletonRetry != nullptr && std::string(skeletonRetry->name) == "opening",
          "skeleton retry must use the safe opening checkpoint");
    check(lichRetry != nullptr && std::string(lichRetry->name) == "mirror",
          "lich retry must use the mirror checkpoint rather than the close inspection point");
    check(twoEnemyCombat != nullptr && twoEnemyCombat->id == 12 &&
              twoEnemyCombat->preset == ShowcaseCheckpointPreset::TwoSkeletonCombat &&
              NearlyEqual(twoEnemyCombat->x, 0.0f) && NearlyEqual(twoEnemyCombat->z, -3.50f) &&
              NearlyEqual(twoEnemyCombat->yaw, 0.0f),
          "two-enemy combat checkpoint must retain its named deterministic measurement pose");
    if (skeletonRetry != nullptr && lichRetry != nullptr)
    {
        const ShowcaseCheckpointState skeletonRetryState = BuildShowcaseCheckpointState(*skeletonRetry);
        const ShowcaseCheckpointState lichRetryState = BuildShowcaseCheckpointState(*lichRetry);
        check(skeletonRetryState.activeEnemyKind == EnemyKind::Skeleton &&
              skeletonRetryState.lanternSnapshot.phase == LanternPhase::Held,
              "skeleton retry must restore the fresh opening encounter");
        check(lichRetryState.activeEnemyKind == EnemyKind::Lich &&
              lichRetryState.lanternSnapshot.phase == LanternPhase::Settled &&
              lichRetryState.lichEncounter.Snapshot().phase != LichPhase::Dormant,
              "lich retry must restore settled lantern and an active lich");
    }

    for (std::size_t i = 0; i < kShowcaseCheckpoints.size(); ++i)
    {
        const ShowcaseCheckpoint& checkpoint = kShowcaseCheckpoints[i];
        check(checkpoint.id == static_cast<std::int32_t>(i), "checkpoint ids must remain stable and contiguous");
        check(FindShowcaseCheckpoint(checkpoint.id) == &checkpoint, "checkpoint id lookup must be deterministic");
        check(FindShowcaseCheckpoint(checkpoint.name) == &checkpoint, "checkpoint name lookup must be deterministic");
        check(IsShowcasePlayerPositionWalkable(checkpoint.x, checkpoint.z), "checkpoint position must be walkable");
        check(QueryShowcaseZone(checkpoint.x, checkpoint.z) == checkpoint.expectedZone,
              "checkpoint position must match its declared zone");
        check(checkpoint.pitch >= -0.32f && checkpoint.pitch <= 0.28f,
              "checkpoint pitch must respect the runtime camera clamp");
        for (std::size_t other = i + 1; other < kShowcaseCheckpoints.size(); ++other)
        {
            check(std::string(checkpoint.name) != kShowcaseCheckpoints[other].name,
                  "checkpoint names must be unique");
        }

        const ShowcaseCheckpointState state = BuildShowcaseCheckpointState(checkpoint);
        if (checkpoint.preset == ShowcaseCheckpointPreset::LanternTrigger)
        {
            check(state.lanternSnapshot.phase == LanternPhase::Guttering,
                  "lantern-drop checkpoint must begin the authored gutter");
        }
        if (checkpoint.preset == ShowcaseCheckpointPreset::LanternSettled ||
            checkpoint.preset == ShowcaseCheckpointPreset::LichActive ||
            checkpoint.preset == ShowcaseCheckpointPreset::FinaleRoofOpen)
        {
            check(state.lanternSnapshot.phase == LanternPhase::Settled &&
                  state.lanternSnapshot.flameStrength == 0.0f &&
                  state.lanternSnapshot.leftArmLowerBlend == 1.0f,
                  "post-drop checkpoints must prime a dark settled lantern and lowered arm");
        }
        if (checkpoint.preset == ShowcaseCheckpointPreset::LichActive)
        {
            check(state.activeEnemyKind == EnemyKind::Lich &&
                  state.lichEncounter.Snapshot().phase != LichPhase::Dormant,
                  "lich checkpoints must deterministically activate the finale encounter");
        }
        if (checkpoint.preset == ShowcaseCheckpointPreset::FinaleRoofOpen)
        {
            check(state.lichEncounter.Snapshot().phase == LichPhase::Dead &&
                  state.lichEncounter.Snapshot().deathAnimationComplete &&
                  NearlyEqual(state.lichEncounter.Snapshot().finaleSkylightOpenProgress, 1.0f, 0.003f) &&
                  state.lichEncounter.Snapshot().finaleEndingPhase == FinaleEndingPhase::DawnRevealed &&
                  state.lichEncounter.Snapshot().finaleDawnRevealProgress < 0.1f,
                  "finale-roof checkpoint must retain the dead lich, open the roof, and stay before the ending overlay");
        }
    }

    LanternSequence lantern;
    lantern.Update(0.05f, 4.0f, -15.2f);
    check(lantern.Snapshot().phase == LanternPhase::Held, "early bends must not trigger lantern failure");
    lantern.Update(0.01f, -1.5f, -15.2f, -0.4f, 0.1f);
    check(lantern.Snapshot().phase == LanternPhase::Guttering, "final west leg must trigger guttering");
    for (int i = 0; i < 68; ++i)
    {
        lantern.Update(0.01f, -5.5f, -15.2f, -0.7f, 0.2f);
    }
    check(lantern.Snapshot().heldByPlayer, "lantern must remain held before 0.70 seconds");
    lantern.Update(0.02f, -5.4f, -15.1f, -0.8f, 0.25f);
    check(lantern.Snapshot().phase == LanternPhase::Falling && !lantern.Snapshot().heldByPlayer,
          "lantern must release at 0.70 seconds");
    check(NearlyEqual(lantern.Snapshot().droppedX, -5.4f) &&
          NearlyEqual(lantern.Snapshot().droppedZ, -15.1f) &&
          NearlyEqual(lantern.Snapshot().droppedYawRadians, -0.8f) &&
          NearlyEqual(lantern.Snapshot().droppedViewPitchRadians, 0.25f),
          "release pose must follow the latest player pose and then freeze");
    for (int i = 0; i < 50; ++i)
    {
        lantern.Update(0.01f, -5.5f, -15.2f);
    }
    check(lantern.Snapshot().phase == LanternPhase::Settled && NearlyEqual(lantern.Snapshot().fallProgress, 1.0f),
          "lantern must settle after the authored fall");
    check(NearlyEqual(lantern.Snapshot().leftArmLowerBlend, 1.0f), "left arm must finish lowering by 1.20 seconds");
    check(NearlyEqual(lantern.Snapshot().flameStrength, 0.0f), "settled lantern must not emit");
    lantern.Reset();
    check(lantern.Snapshot().phase == LanternPhase::Held && !lantern.Snapshot().triggered,
          "reset must restore held untriggered lantern");

    const LowerBodyPoseState standingPose = EvaluateLowerBodyPose(0.25f, 0.0f);
    const LowerBodyPoseState walkingPose = EvaluateLowerBodyPose(0.25f, 1.0f);
    const LowerBodyPoseState opposedPose = EvaluateLowerBodyPose(0.25f + 0.5067f, 1.0f);
    check(NearlyEqual(standingPose.leftStride, 0.0f) &&
              NearlyEqual(standingPose.pelvisBob, 0.0f) &&
              NearlyEqual(standingPose.pelvisSway, 0.0f) &&
              NearlyEqual(standingPose.leftFootLift, 0.0f) &&
              NearlyEqual(standingPose.rightFootLift, 0.0f),
          "standing lower body must not drift");
    check(NearlyEqual(walkingPose.leftStride, -walkingPose.rightStride) &&
              walkingPose.pelvisBob > 0.0f &&
              std::abs(walkingPose.pelvisSway) <= 0.0181f &&
              std::abs(walkingPose.torsoTwistRadians) <= 0.0551f,
          "walking lower body must keep opposed legs with bounded pelvis and torso motion");
    check((walkingPose.leftFootLift > 0.0f) != (walkingPose.rightFootLift > 0.0f) &&
              walkingPose.leftKneeBend >= walkingPose.leftFootLift &&
              walkingPose.rightKneeBend >= walkingPose.rightFootLift,
          "only the swinging foot may lift while both knees retain a small walking bend");
    check(walkingPose.leftStride * opposedPose.leftStride < 0.0f &&
              walkingPose.leftFootLift * opposedPose.leftFootLift == 0.0f,
          "half a gait cycle must exchange stride direction and the lifted foot");

    const SceneLightingState yellowLight = RouteLightForPosition(-11.0f, -15.2f);
    const SceneLightingState blueLight = RouteLightForPosition(-16.0f, -15.2f);
    const SceneLightingState redLight = RouteLightForPosition(-21.0f, -15.2f);
    const SceneLightingState greenLight = RouteLightForPosition(-26.0f, -15.2f);
    check(yellowLight.intensity == 1.0f && yellowLight.colour[0] > yellowLight.colour[2],
          "yellow bay must select its warm local light");
    check(blueLight.colour[2] > blueLight.colour[0], "blue bay must select blue illumination");
    check(redLight.colour[0] > 0.9f && redLight.colour[1] < 0.1f,
          "red bay must remain a deep red");
    check(greenLight.colour[1] > greenLight.colour[0] && greenLight.colour[1] < 0.9f,
          "green bay must remain restrained");
    check(RouteLightForPosition(-5.5f, -15.2f).intensity == 0.0f,
          "skylight chamber must not accidentally select a torch bay light");

    EnemyDirector director({4});
    check(director.Snapshot().selectedEnemy == EnemyKind::Skeleton, "reset side must select skeleton");
    check(director.Snapshot().renderedEnemyCount == 1 && director.Snapshot().renderedEnemyCapacity == 4,
          "current route must use one slot while preserving configurable capacity");
    const auto initialSkeletonGeneration = director.Snapshot().encounters[0].resetGeneration;
    director.Update(4.0f, -15.0f);
    check(director.Snapshot().selectedEnemy == EnemyKind::Skeleton,
          "skeleton must remain selected throughout shadow corridor");
    director.Update(-5.5f, -15.2f);
    check(director.Snapshot().selectedEnemy == EnemyKind::Lich, "skylight gate must reset and select lich");
    const auto firstLichGeneration = director.Snapshot().encounters[1].resetGeneration;
    director.Update(-26.0f, -15.2f);
    check(director.Snapshot().encounters[1].resetGeneration == firstLichGeneration,
          "movement within lich route side must not repeatedly reset encounter");
    director.MarkSelectedDead();
    check(director.Snapshot().encounters[1].status == EncounterStatus::Dead,
          "selected encounter death must be recorded");
    director.Update(4.0f, -15.0f);
    check(director.Snapshot().selectedEnemy == EnemyKind::Skeleton &&
          director.Snapshot().encounters[0].resetGeneration == initialSkeletonGeneration + 1,
          "return crossing must reset and reselect skeleton");
    director.Update(-5.5f, -15.2f);
    check(director.Snapshot().encounters[1].status == EncounterStatus::Active &&
          director.Snapshot().encounters[1].resetGeneration == firstLichGeneration + 1,
          "re-entering skylight side must reset lich");
    director.Reset();
    check(director.Snapshot().selectedEnemy == EnemyKind::Skeleton &&
          director.Snapshot().encounters[0].resetGeneration == 1,
          "global reset must deterministically restore skeleton encounter");

    LichEncounter lich;
    lich.Update(0.05f, -33.7f, -15.2f, true, false);
    check(lich.Snapshot().phase == LichPhase::Dormant, "lich must remain dormant before finale activation");
    const float maintainStartX = lich.Snapshot().x;
    const float maintainStartZ = lich.Snapshot().z;
    int repositionFrames = 0;
    while (lich.Snapshot().phase != LichPhase::Charging && repositionFrames < 400)
    {
        lich.Update(0.01f, -33.7f, -15.2f, true, true);
        ++repositionFrames;
    }
    const float chargeDistance = std::hypot(lich.Snapshot().x + 33.7f, lich.Snapshot().z + 15.2f);
    check(lich.Snapshot().phase == LichPhase::Charging && repositionFrames >= 65,
          "lich must visibly reposition before beginning a charge");
    check(chargeDistance >= LichEncounter::kPreferredMinRange &&
          chargeDistance <= LichEncounter::kPreferredMaxRange,
          "lich must enter its preferred three-to-five metre band before charging");
    check(std::hypot(lich.Snapshot().x - maintainStartX, lich.Snapshot().z - maintainStartZ) > 0.20f,
          "maintaining-range phase must produce readable lateral movement");
    const float chargeStartX = lich.Snapshot().x;
    const float chargeStartZ = lich.Snapshot().z;
    for (int i = 0; i < 118; ++i)
    {
        lich.Update(0.01f, -33.7f, -15.2f, true, true);
    }
    check(lich.Snapshot().phase == LichPhase::Charging && !lich.Snapshot().damagePulse,
          "staff charge must not resolve before 1.20 seconds");
    check(std::hypot(lich.Snapshot().x - chargeStartX, lich.Snapshot().z - chargeStartZ) > 0.05f,
          "lich must keep gliding during its charge");
    lich.Update(0.02f, -33.7f, -15.2f, true, true);
    check(lich.Snapshot().phase == LichPhase::Recovering && lich.Snapshot().damagePulse,
          "staff charge must resolve once at its 1.20-second peak");
    const float recoveryStartX = lich.Snapshot().x;
    const float recoveryStartZ = lich.Snapshot().z;
    for (int i = 0; i < 178; ++i)
    {
        lich.Update(0.01f, -33.7f, -15.2f, true, true);
    }
    check(lich.Snapshot().phase == LichPhase::Recovering,
          "lich must remain in recovery before 1.80 seconds elapses");
    check(std::hypot(lich.Snapshot().x - recoveryStartX, lich.Snapshot().z - recoveryStartZ) > 0.10f,
          "lich must keep moving during recovery");
    lich.Update(0.03f, -33.7f, -15.2f, true, true);
    check(lich.Snapshot().phase == LichPhase::MaintainingRange,
          "lich must leave recovery after 1.80 seconds");

    director.Update(-5.5f, -15.2f);
    LichEncounter dyingLich;
    check(!dyingLich.TryAcceptPlayerHit(dyingLich.Snapshot().x, dyingLich.Snapshot().z),
          "dormant lich must reject player hits");
    dyingLich.Update(0.01f, dyingLich.Snapshot().x, dyingLich.Snapshot().z, true, true);
    const float closeHitX = dyingLich.Snapshot().x;
    const float closeHitZ = dyingLich.Snapshot().z;
    check(!dyingLich.TryAcceptPlayerHit(closeHitX + LichEncounter::kPlayerHitRange + 0.01f, closeHitZ),
          "lich must reject a player hit outside close range");
    check(dyingLich.TryAcceptPlayerHit(closeHitX, closeHitZ) &&
          dyingLich.Snapshot().health == 2 && dyingLich.Snapshot().hitPulse &&
          NearlyEqual(dyingLich.Snapshot().hitRecoil, 1.0f),
          "first close hit must be accepted and expose health/hit pulse/recoil");
    check(!dyingLich.TryAcceptPlayerHit(closeHitX, closeHitZ) && dyingLich.Snapshot().health == 2,
          "immediate repeat hit must be rejected by the lockout");
    float halfRecoil = 0.0f;
    float finishedRecoil = 1.0f;
    for (int i = 0; i < 199; ++i)
    {
        dyingLich.Update(0.01f, closeHitX, closeHitZ, true, true);
        if (i == 18) halfRecoil = dyingLich.Snapshot().hitRecoil;
        if (i == 37) finishedRecoil = dyingLich.Snapshot().hitRecoil;
    }
    check(halfRecoil > 0.45f && halfRecoil < 0.55f && NearlyEqual(finishedRecoil, 0.0f, 0.002f),
          "accepted-hit recoil must ease deterministically to zero over 0.38 seconds");
    check(!dyingLich.TryAcceptPlayerHit(closeHitX, closeHitZ) && dyingLich.Snapshot().health == 2,
          "hit must remain rejected before the full two-second lockout");
    dyingLich.Update(0.02f, closeHitX, closeHitZ, true, true);
    check(dyingLich.TryAcceptPlayerHit(closeHitX, closeHitZ) && dyingLich.Snapshot().health == 1 &&
          NearlyEqual(dyingLich.Snapshot().hitRecoil, 1.0f),
          "second hit must reset recoil after lockout expiry without killing lich");
    for (int i = 0; i < 200; ++i)
    {
        dyingLich.Update(0.01f, closeHitX, closeHitZ, true, true);
    }
    check(dyingLich.TryAcceptPlayerHit(closeHitX, closeHitZ) && dyingLich.Snapshot().health == 0 &&
          NearlyEqual(dyingLich.Snapshot().hitRecoil, 1.0f),
          "third accepted close hit must exhaust lich health and start its recoil");
    check(dyingLich.Snapshot().phase == LichPhase::Dead &&
          NearlyEqual(dyingLich.Snapshot().animationTime, 0.0f) &&
          !dyingLich.Snapshot().deathAnimationComplete &&
          dyingLich.Snapshot().finaleEndingPhase == FinaleEndingPhase::LichFalling,
          "third hit must start the authored death clip at frame zero");
    for (int i = 0; i < 296; ++i)
    {
        dyingLich.Update(0.01f, closeHitX, closeHitZ, true, true);
    }
    check(!dyingLich.Snapshot().deathAnimationComplete &&
          director.Snapshot().encounters[1].status == EncounterStatus::Active,
          "roster must retain the lich throughout the visible death animation");
    dyingLich.Update(0.007f, closeHitX, closeHitZ, true, true);
    check(dyingLich.Snapshot().deathAnimationComplete &&
          NearlyEqual(dyingLich.Snapshot().animationTime, LichEncounter::kDeathAnimationDuration, 0.002f) &&
          NearlyEqual(dyingLich.Snapshot().finaleSkylightOpenProgress, 0.0f, 0.002f) &&
          dyingLich.Snapshot().finaleEndingPhase == FinaleEndingPhase::SkylightOpening,
          "death animation must reach and hold its authored final frame");
    director.MarkSelectedDead();
    check(director.Snapshot().encounters[1].status == EncounterStatus::Dead,
          "roster death notification must occur only after animation completion");
    for (int i = 0; i < 225; ++i)
    {
        dyingLich.Update(0.01f, closeHitX, closeHitZ, true, true);
    }
    check(NearlyEqual(dyingLich.Snapshot().finaleSkylightOpenProgress, 0.5f, 0.003f) &&
          NearlyEqual(dyingLich.Snapshot().animationTime, LichEncounter::kDeathAnimationDuration, 0.002f),
          "finale skylight must open halfway over the first 2.25 post-death seconds while pose stays clamped");
    for (int i = 0; i < 225; ++i)
    {
        dyingLich.Update(0.01f, closeHitX, closeHitZ, true, true);
    }
    check(NearlyEqual(dyingLich.Snapshot().finaleSkylightOpenProgress, 1.0f, 0.003f),
          "finale skylight must finish opening after 4.5 post-death seconds");
    check(dyingLich.Snapshot().finaleEndingPhase == FinaleEndingPhase::DawnRevealed &&
              NearlyEqual(dyingLich.Snapshot().finaleDawnRevealProgress, 0.0f, 0.03f),
          "the fully open roof must begin a short visible dawn reveal before completing the demo");
    for (int i = 0; i < 175; ++i)
    {
        dyingLich.Update(0.01f, closeHitX, closeHitZ, true, true);
    }
    check(dyingLich.Snapshot().finaleEndingPhase == FinaleEndingPhase::Complete &&
              NearlyEqual(dyingLich.Snapshot().finaleDawnRevealProgress, 1.0f, 0.003f),
          "the ending must complete deterministically after the dawn reveal hold");
    check(!dyingLich.TryAcceptPlayerHit(closeHitX, closeHitZ),
          "dead lich must reject all further player hits");
    dyingLich.Reset();
    check(dyingLich.Snapshot().health == 3 &&
          NearlyEqual(dyingLich.Snapshot().hitCooldownRemaining, 0.0f) &&
          NearlyEqual(dyingLich.Snapshot().hitRecoil, 0.0f) &&
          NearlyEqual(dyingLich.Snapshot().finaleSkylightOpenProgress, 0.0f) &&
          NearlyEqual(dyingLich.Snapshot().finaleDawnRevealProgress, 0.0f) &&
          dyingLich.Snapshot().finaleEndingPhase == FinaleEndingPhase::Inactive,
          "encounter reset must restore health, lockout, recoil, and all finale progression");

    lich.Reset();
    bool sawVisibleDamage = false;
    float minY = lich.Snapshot().y;
    float maxY = lich.Snapshot().y;
    for (int i = 0; i < 400; ++i)
    {
        const auto& snapshot = lich.Update(0.01f, -33.7f, -15.2f, true, true);
        sawVisibleDamage = sawVisibleDamage || snapshot.damagePulse;
        minY = std::min(minY, snapshot.y);
        maxY = std::max(maxY, snapshot.y);
        check(snapshot.x >= -36.4f && snapshot.x <= -30.9f &&
              snapshot.z >= -17.9f && snapshot.z <= -12.5f,
              "lich movement must stay inside finale room bounds");
    }
    check(sawVisibleDamage, "visible player inside seven metres must receive peak charge pulse");
    check(maxY - minY > 0.05f, "active lich must visibly hover");

    lich.Reset();
    bool sawOccludedDamage = false;
    for (int i = 0; i < 400; ++i)
    {
        sawOccludedDamage = sawOccludedDamage ||
            lich.Update(0.01f, -33.7f, -15.2f, false, true).damagePulse;
    }
    check(!sawOccludedDamage, "occluded staff charge must not damage player");
    lich.Reset();
    bool sawOutOfRangeDamage = false;
    for (int i = 0; i < 100; ++i)
    {
        sawOutOfRangeDamage = sawOutOfRangeDamage ||
            lich.Update(0.01f, -25.0f, -15.2f, true, true).damagePulse;
    }
    check(!sawOutOfRangeDamage, "staff charge must not damage beyond seven metres");

    if (!passed)
    {
        return 1;
    }
    std::cout << "Showcase gameplay smoke passed: lantern, gated roster, and lich finale state.\n";
    return 0;
}
