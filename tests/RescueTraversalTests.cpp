#include "gameplay/traversal/RescueTraversal.h"
#include "scene/RescueBlockoutGeometry.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

using namespace horde::gameplay::traversal;

int main()
{
    int cases = 0;
    int failures = 0;
    const auto check = [&](bool pass, const char* message) {
        ++cases;
        if (!pass) { ++failures; std::cerr << message << '\n'; }
    };
    const auto near = [](float a, float b, float epsilon = 0.002f) {
        return std::abs(a - b) <= epsilon;
    };
    const auto checkHandsClearAnchorBeam = [&](const RescueTraversalSnapshot& snapshot) {
        const auto& beam = horde::scene::kRescueBlockoutBoxes[11];
        for (const auto& hand : snapshot.grippingHandTargets) {
            const bool insideFootprint = hand.x >= beam.minimum[0] - 0.04f &&
                hand.x <= beam.maximum[0] + 0.04f && hand.z >= beam.minimum[2] - 0.04f &&
                hand.z <= beam.maximum[2] + 0.04f;
            check(!insideFootprint || hand.y + 0.04f <= beam.minimum[1] ||
                hand.y - 0.04f >= beam.maximum[1],
                "rope hand target intersected the solid cantilever beam");
        }
    };

    RescueTraversal traversal;
    auto state = traversal.Snapshot();
    check(state.phase == Phase::LowerSafe && near(state.supportWorldY, kLowerSupportWorldY) &&
        !traversal.CanInteract(), "unclaimed route must start safely with interaction gated");
    check(!traversal.Request() && traversal.Snapshot().claimCount == 0,
        "unclaimed lantern could request rope traversal");
    check(traversal.NotifyLanternClaimed() && !traversal.NotifyLanternClaimed() &&
        traversal.DeployOwned() && !traversal.DeployOwned(),
        "lantern claim and owned rope deployment must each happen exactly once");
    state = traversal.Snapshot();
    check(state.lanternClaimed && state.ropeDeployed && state.claimCount == 1 && state.deploymentCount == 1 &&
        traversal.CanInteract(), "claim did not deploy the anchored rope before interaction");
    check(near(kAnchor.x, -33.7f) && near(kAnchor.y, 3.8f) && near(kAnchor.z, -15.5f),
        "rope anchor must match the elevated cantilever and remain clear of the safe player torso");
    check(near(kUpperSupportWorldY, 2.05f) && near(kLowerSupportWorldY, -0.95f),
        "safe support heights changed");
    check(near(kExteriorLanding.z, -12.8f), "exterior landing must clear the rim");
    check(state.bodyYawRadians == 0.0f, "rope climb body orientation must not depend on camera yaw");

    Input input;
    input.playerPosition = kLowerLanding;
    input.supportWorldY = kLowerSupportWorldY;
    input.contactPlaneWorldY = kLowerSupportWorldY;
    const auto step = [&] {
        const auto before = traversal.Snapshot();
        if (before.phase == Phase::LowerSafe || before.phase == Phase::UpperSafe) {
            input.playerPosition = before.playerPosition;
            input.supportWorldY = before.supportWorldY;
        }
        return traversal.Step(input);
    };

    input.playerPosition.x += 0.5f;
    check(traversal.Step(input) && !traversal.CanInteract(), "approach tolerance allowed a distant player to attach by teleport");
    input.playerPosition = kLowerLanding;
    input.supportWorldY = kLowerSupportWorldY;
    check(traversal.Step(input) && traversal.CanInteract(), "valid lower approach did not restore interaction eligibility");

    check(traversal.Request(), "lower safe endpoint did not accept a consumed interaction");
    state = traversal.Snapshot();
    check(state.phase == Phase::AwaitingAscentReadiness && state.claimCount == 1 &&
        state.deploymentCount == 1 && state.equipmentStowed && !traversal.CanInteract(),
        "request must wait after the one prior claim and deployment");
    const auto firstGeneration = state.generation;
    check(!traversal.Request() && traversal.Snapshot().claimCount == 1,
        "duplicate request claimed traversal twice");
    check(traversal.PublishReadiness(firstGeneration, true), "matching readiness rejected");
    check(!traversal.TryBegin(firstGeneration, true, false), "render-unready request committed");
    check(traversal.Snapshot().phase == Phase::LowerSafe &&
        traversal.Snapshot().deploymentCount == 1 && traversal.Snapshot().promptReason == PromptReason::Blocked,
        "failed begin did not remain at a valid blocked lower prompt");
    check(!traversal.PublishReadiness(firstGeneration, true), "aborted generation callback accepted");

    check(traversal.Request(), "fresh lower interaction did not revalidate after failed begin");
    const auto ascentGeneration = traversal.Snapshot().generation;
    check(!traversal.PublishReadiness(ascentGeneration - 1, true), "stale readiness generation accepted");
    check(traversal.PublishReadiness(ascentGeneration, true), "fresh matching readiness rejected");
    check(!traversal.TryBegin(ascentGeneration, false, true), "unclaimed deployment committed");
    check(traversal.Snapshot().phase == Phase::LowerSafe && traversal.Snapshot().deploymentCount == 1,
        "unclaimed begin left the safe support");

    check(traversal.Request(), "second fresh interaction did not revalidate");
    const auto readyGeneration = traversal.Snapshot().generation;
    check(traversal.PublishReadiness(readyGeneration, true) &&
        traversal.TryBegin(readyGeneration, true, true), "ready claimed ascent failed to commit");
    check(traversal.Snapshot().phase == Phase::Ascent && traversal.Snapshot().deploymentCount == 1,
        "ascent did not commit after one owned rope deployment");
    bool sawLeftMove = false, sawRightMove = false;
    float maximumTension = 0.0f;
    float maximumStepDistance = 0.0f;
    float maximumLoadedAscentStepDistance = 0.0f;
    float maximumAscentHandStepDistance = 0.0f;
    float maximumPullUpRootStepDistance = 0.0f;
    float maximumPullUpHandStepDistance = 0.0f;
    float pullUpStartLeftIndex = -1.0f;
    float pullUpStartRightIndex = -1.0f;
    std::array<float, 2u> maximumPullUpHandStepIndices{};
    float maximumPullUpHandStepZ = 0.0f;
    bool pullupCrossedCoping = false;
    float initialGripMidpointY = 0.0f;
    for (const auto& hand : traversal.Snapshot().grippingHandTargets)
        initialGripMidpointY += hand.y * 0.5f;
    auto previousGrip = traversal.Snapshot().grippingRopeNodeIndices;
    auto previousHands = traversal.Snapshot().grippingHandTargets;
    auto previousPlayer = traversal.Snapshot().playerPosition;
    for (int tick = 0; tick < 700 && traversal.Snapshot().phase != Phase::UpperSafe; ++tick) {
        const auto phaseBeforeStep = traversal.Snapshot().phase;
        check(step(), "valid ascent tick rejected");
        state = traversal.Snapshot();
        checkHandsClearAnchorBeam(state);
        check(state.ropeHandsActive || state.phase != Phase::PullUp ||
                  (state.playerPosition.z >= kApronHandoff.z - 0.001f &&
                   state.supportWorldY >= kUpperSupportWorldY - 0.001f),
              "ascent released rope before the first grounded apron handoff");
        sawLeftMove = sawLeftMove || state.grippingRopeNodeIndices[0] != previousGrip[0];
        sawRightMove = sawRightMove || state.grippingRopeNodeIndices[1] != previousGrip[1];
        previousGrip = state.grippingRopeNodeIndices;
        const float pdx = state.playerPosition.x - previousPlayer.x;
        const float pdy = state.playerPosition.y - previousPlayer.y;
        const float pdz = state.playerPosition.z - previousPlayer.z;
        maximumStepDistance = std::max(maximumStepDistance, std::sqrt(pdx * pdx + pdy * pdy + pdz * pdz));
        if (phaseBeforeStep == Phase::Ascent || state.phase == Phase::Ascent)
        {
            maximumLoadedAscentStepDistance = std::max(maximumLoadedAscentStepDistance,
                std::sqrt(pdx * pdx + pdy * pdy + pdz * pdz));
            for (std::size_t hand = 0u; hand < previousHands.size(); ++hand)
            {
                const auto current = state.grippingHandTargets[hand];
                const auto previous = previousHands[hand];
                const float hx = current.x - previous.x;
                const float hy = current.y - previous.y;
                const float hz = current.z - previous.z;
                maximumAscentHandStepDistance = std::max(maximumAscentHandStepDistance,
                    std::sqrt(hx * hx + hy * hy + hz * hz));
            }
        }
        if (phaseBeforeStep == Phase::PullUp || state.phase == Phase::PullUp)
        {
            maximumPullUpRootStepDistance = std::max(maximumPullUpRootStepDistance,
                std::sqrt(pdx * pdx + pdy * pdy + pdz * pdz));
            if (phaseBeforeStep == Phase::Ascent && state.phase == Phase::PullUp)
            {
                pullUpStartLeftIndex = state.grippingRopeNodeIndices[0];
                pullUpStartRightIndex = state.grippingRopeNodeIndices[1];
                check(std::isfinite(pullUpStartLeftIndex) &&
                      std::isfinite(pullUpStartRightIndex) &&
                      pullUpStartLeftIndex < pullUpStartRightIndex,
                      "pull-up must preserve the actual ordered ascent grip pair");
            }
            if (state.ropeHandsActive)
            {
                for (std::size_t hand = 0u; hand < previousHands.size(); ++hand)
                {
                    const auto current = state.grippingHandTargets[hand];
                    const auto previous = previousHands[hand];
                    const float hx = current.x - previous.x;
                    const float hy = current.y - previous.y;
                    const float hz = current.z - previous.z;
                    const float handStep = std::sqrt(hx * hx + hy * hy + hz * hz);
                    if (handStep > maximumPullUpHandStepDistance)
                    {
                        maximumPullUpHandStepDistance = handStep;
                        maximumPullUpHandStepIndices = state.grippingRopeNodeIndices;
                        maximumPullUpHandStepZ = state.playerPosition.z;
                    }
                }
            }
            if (state.phase == Phase::PullUp && state.ropeHandsActive && state.playerPosition.z > kRimLanding.z + 0.001f)
            {
                check(near(state.grippingRopeNodeIndices[1], kPullUpRightGripNodeIndex, 0.001f),
                      "right hand must finish its solved upper regrip before the supported planar handoff");
                check(near(state.grippingRopeNodeIndices[0], kPullUpLeftGripNodeIndex, 0.001f),
                      "left hand must finish its ordered upper regrip before the supported planar handoff");
            }
            if (state.phase == Phase::PullUp && state.ropeHandsActive)
            {
                const auto& left = state.grippingHandTargets[0];
                const auto& right = state.grippingHandTargets[1];
                const float hx = left.x - right.x;
                const float hy = left.y - right.y;
                const float hz = left.z - right.z;
                check(std::sqrt(hx * hx + hy * hy + hz * hz) >= 0.08f,
                      "alternating regrip must keep both solved hands spatially distinct");
            }
        }
        previousHands = state.grippingHandTargets;
        previousPlayer = state.playerPosition;
        check(state.supportWorldY >= kLowerSupportWorldY - 0.001f &&
            state.supportWorldY <= kCopingClearanceSupportWorldY + 0.001f,
            "ascent escaped bounded vertical supports");
        if (state.playerPosition.z >= kCopingZMin && state.playerPosition.z <= kCopingZMax) {
            pullupCrossedCoping = true;
            check(state.supportWorldY >= kCopingClearanceSupportWorldY - 0.001f,
                "pull-up root crossed coping below its top clearance");
        }
        check(std::isfinite(state.ropeTension) && state.ropeTension >= 0.0f &&
            state.ropeTension <= kMaximumRopeTension,
            "rope tension escaped its finite load bound");
        maximumTension = std::max(maximumTension, state.ropeTension);
        check(state.ropeNodes.front() == kAnchor, "anchored rope root moved");
        for (std::size_t hand = 0; hand < state.grippingHandTargets.size(); ++hand) {
            const float index = state.grippingRopeNodeIndices[hand];
            const auto lower = static_cast<std::size_t>(index);
            const auto upper = std::min(lower + 1, state.ropeNodes.size() - 1);
            const float blend = index - static_cast<float>(lower);
            const Vec3 expected{state.ropeNodes[lower].x + (state.ropeNodes[upper].x - state.ropeNodes[lower].x) * blend,
                state.ropeNodes[lower].y + (state.ropeNodes[upper].y - state.ropeNodes[lower].y) * blend,
                state.ropeNodes[lower].z + (state.ropeNodes[upper].z - state.ropeNodes[lower].z) * blend};
            check(near(expected.x, state.grippingHandTargets[hand].x) &&
                near(expected.y, state.grippingHandTargets[hand].y) &&
                near(expected.z, state.grippingHandTargets[hand].z),
                "gripping hand did not sample the solved rope particle chain");
        }
        for (std::size_t i = 0; i < state.ropeNodes.size(); ++i) {
            const auto p = state.ropeNodes[i];
            check(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z),
                "rope particle became nonfinite");
            if (i > 0) {
                const auto q = state.ropeNodes[i - 1];
                const float dx = p.x - q.x, dy = p.y - q.y, dz = p.z - q.z;
                check(std::sqrt(dx * dx + dy * dy + dz * dz) <= kRopeSegmentRestLength * 1.12f,
                    "rope constraint allowed unbounded stretch");
            }
            check(p.y >= input.contactPlaneWorldY - 0.001f,
                "rope particle penetrated the shaft contact plane");
        }
        if (state.phase == Phase::Ascent) {
            const Vec3 midpoint{(state.grippingHandTargets[0].x + state.grippingHandTargets[1].x) * 0.5f,
                (state.grippingHandTargets[0].y + state.grippingHandTargets[1].y) * 0.5f,
                (state.grippingHandTargets[0].z + state.grippingHandTargets[1].z) * 0.5f};
            check(near(state.supportWorldY, kLowerSupportWorldY + midpoint.y - initialGripMidpointY, 0.04f),
                "player root detached from solved rope grip segment");
        }
    }
    state = traversal.Snapshot();
    std::cerr << "PullUp diagnostic start_indices=" << pullUpStartLeftIndex << ','
              << pullUpStartRightIndex << " max_hand_step=" << maximumPullUpHandStepDistance
              << " at_indices=" << maximumPullUpHandStepIndices[0] << ','
              << maximumPullUpHandStepIndices[1] << " z=" << maximumPullUpHandStepZ
              << std::endl;
    check(state.phase == Phase::UpperSafe && near(state.supportWorldY, kUpperSupportWorldY) &&
        near(state.playerPosition.z, kExteriorLanding.z),
        "ascent and pull-up did not reach exterior landing");
    check(sawLeftMove && sawRightMove && state.sawAscent && state.sawPullUp &&
        state.firstAscentCompleted && state.exteriorSide,
        "alternating grip, pull-up, or completed-ascent state missing");
    check(maximumTension > 0.0f, "player load did not produce rope tension");
    check(maximumLoadedAscentStepDistance <= 0.03f,
        "continuous alternating rope grips must keep loaded player root motion below 30 mm per tick");
    check(maximumAscentHandStepDistance <= 0.05f,
        "continuous alternating hand targets must not jump between rope particles");
    check(maximumPullUpRootStepDistance <= 0.03f && maximumPullUpHandStepDistance <= 0.05f,
        "upper regrip and supported handoff must remain continuous for root and actual rope targets");
    check(pullupCrossedCoping && maximumStepDistance <= 0.25f,
        "pull-up skipped the coping lift or teleported between fixed ticks");
    check(!state.equipmentStowed && traversal.CanInteract(), "upper landing not interactable or equipment still stowed");
    checkHandsClearAnchorBeam(state);
    check(!state.ropeHandsActive, "upper safe landing must release both rope hands");

    check(traversal.Request(), "upper safe endpoint did not accept descent interaction");
    const auto descentGeneration = traversal.Snapshot().generation;
    check(traversal.Snapshot().phase == Phase::AwaitingDescentReadiness && traversal.Snapshot().claimCount == 1,
        "descent request changed one-shot lantern claim ownership");
    check(!traversal.PublishReadiness(readyGeneration, true), "previous traversal readiness was reused");
    check(traversal.PublishReadiness(descentGeneration, true) &&
        traversal.TryBegin(descentGeneration, true, true), "ready descent failed to commit");
    check(!traversal.Snapshot().ropeHandsActive,
        "descent must remain hands-free until the actual apron-to-rope approach is complete");
    bool descentCrossedCoping = false;
    float maximumLoadedDescentRootStepDistance = 0.0f;
    float maximumLandingRootStepDistance = 0.0f;
    float maximumLandingHandStepDistance = 0.0f;
    previousHands = traversal.Snapshot().grippingHandTargets;
    previousPlayer = traversal.Snapshot().playerPosition;
    maximumStepDistance = 0.0f;
    for (int tick = 0; tick < 700 && traversal.Snapshot().phase != Phase::LowerSafe; ++tick)
    {
        const auto phaseBeforeStep = traversal.Snapshot().phase;
        const bool ropeLoadedBeforeStep = traversal.Snapshot().ropeHandsActive;
        check(step(), "valid descent tick rejected");
        state = traversal.Snapshot();
        checkHandsClearAnchorBeam(state);
        check(!state.ropeHandsActive ||
                  (state.playerPosition.z <= kApronHandoff.z + 0.001f &&
                   state.supportWorldY <= kUpperSupportWorldY + 0.001f),
              "descent attached rope hands before reaching the actual upper apron handoff");
        const float dx = state.playerPosition.x - previousPlayer.x;
        const float dy = state.playerPosition.y - previousPlayer.y;
        const float dz = state.playerPosition.z - previousPlayer.z;
        maximumStepDistance = std::max(maximumStepDistance, std::sqrt(dx * dx + dy * dy + dz * dz));
        if (ropeLoadedBeforeStep || state.ropeHandsActive)
            maximumLoadedDescentRootStepDistance = std::max(maximumLoadedDescentRootStepDistance,
                std::sqrt(dx * dx + dy * dy + dz * dz));
        if (phaseBeforeStep == Phase::Landing || state.phase == Phase::Landing)
        {
            maximumLandingRootStepDistance = std::max(maximumLandingRootStepDistance,
                std::sqrt(dx * dx + dy * dy + dz * dz));
            for (std::size_t hand = 0u; hand < previousHands.size(); ++hand)
            {
                const auto current = state.grippingHandTargets[hand];
                const auto previous = previousHands[hand];
                const float hx = current.x - previous.x;
                const float hy = current.y - previous.y;
                const float hz = current.z - previous.z;
                maximumLandingHandStepDistance = std::max(maximumLandingHandStepDistance,
                    std::sqrt(hx * hx + hy * hy + hz * hz));
            }
        }
        previousHands = state.grippingHandTargets;
        previousPlayer = state.playerPosition;
        if (state.playerPosition.z >= kCopingZMin && state.playerPosition.z <= kCopingZMax) {
            descentCrossedCoping = true;
            check(state.supportWorldY >= kCopingClearanceSupportWorldY - 0.001f,
                "descent root crossed coping below its top clearance");
        }
    }
    state = traversal.Snapshot();
    check(state.phase == Phase::LowerSafe && near(state.supportWorldY, kLowerSupportWorldY) &&
        near(state.playerPosition.x, kLowerLanding.x) && near(state.playerPosition.z, kLowerLanding.z),
        "descent and landing did not return to lower safe support");
    check(!state.ropeHandsActive, "lower safe landing must release both rope hands");
    check(state.sawDescent && state.sawLanding && state.deploymentCount == 1,
        "descent and landing phases or deployment count missing");
    check(descentCrossedCoping && maximumStepDistance <= 0.25f,
        "descent skipped the reverse coping lift or teleported between fixed ticks");
    check(maximumLandingRootStepDistance <= 0.03f,
        "continuous alternating landing grips must keep player root motion below 30 mm per tick");
    check(maximumLoadedDescentRootStepDistance <= 0.03f,
        "loaded descent must keep player root motion below 30 mm per fixed tick");
    check(maximumLandingHandStepDistance <= 0.05f,
        "continuous alternating landing hand targets must not jump between rope particles");

    // Reuse the same deployed rope through a second complete route cycle. Its
    // sag changed under the first descent load, so each ascent must rebase on
    // the actual solved starting grips rather than the previous cycle's pose.
    check(traversal.Request(), "lower endpoint did not accept the second ascent request");
    auto generation = traversal.Snapshot().generation;
    check(traversal.PublishReadiness(generation, true) &&
              traversal.TryBegin(generation, true, true),
          "second ascent did not commit against its fresh readiness generation");
    float secondCycleMaximumStep = 0.0f;
    previousPlayer = traversal.Snapshot().playerPosition;
    for (int tick = 0; tick < 700 && traversal.Snapshot().phase != Phase::UpperSafe; ++tick) {
        check(step(), "second ascent fixed tick rejected");
        state = traversal.Snapshot();
        const float dx = state.playerPosition.x - previousPlayer.x;
        const float dy = state.playerPosition.y - previousPlayer.y;
        const float dz = state.playerPosition.z - previousPlayer.z;
        secondCycleMaximumStep = std::max(secondCycleMaximumStep,
            std::sqrt(dx * dx + dy * dy + dz * dz));
        previousPlayer = state.playerPosition;
    }
    check(traversal.Snapshot().phase == Phase::UpperSafe &&
              near(traversal.Snapshot().supportWorldY, kUpperSupportWorldY),
          "second ascent did not reach the upper safe endpoint");
    check(secondCycleMaximumStep <= 0.03f,
          "rebased second ascent must preserve bounded per-tick root motion");
    check(traversal.Request(), "upper endpoint did not accept the second descent request");
    generation = traversal.Snapshot().generation;
    check(traversal.PublishReadiness(generation, true) &&
              traversal.TryBegin(generation, true, true),
          "second descent did not commit against its fresh readiness generation");
    previousPlayer = traversal.Snapshot().playerPosition;
    for (int tick = 0; tick < 700 && traversal.Snapshot().phase != Phase::LowerSafe; ++tick) {
        check(step(), "second descent fixed tick rejected");
        state = traversal.Snapshot();
        const float dx = state.playerPosition.x - previousPlayer.x;
        const float dy = state.playerPosition.y - previousPlayer.y;
        const float dz = state.playerPosition.z - previousPlayer.z;
        secondCycleMaximumStep = std::max(secondCycleMaximumStep,
            std::sqrt(dx * dx + dy * dy + dz * dz));
        previousPlayer = state.playerPosition;
    }
    check(traversal.Snapshot().phase == Phase::LowerSafe &&
              near(traversal.Snapshot().supportWorldY, kLowerSupportWorldY),
          "second descent did not return to the lower safe endpoint");
    check(secondCycleMaximumStep <= 0.03f && traversal.Snapshot().deploymentCount == 1,
          "second route cycle teleported or redeployed the owned rope");

    // Reconstruction must invalidate callbacks while retaining ownership; full reset clears it.
    const auto oldGeneration = state.generation;
    traversal.RecoverForReconstruction();
    state = traversal.Snapshot();
    check(state.generation > oldGeneration && state.claimCount == 1 && state.deploymentCount == 1 &&
        state.lanternClaimed && state.ropeDeployed && state.firstAscentCompleted,
        "reconstruction recovery lost ownership or completed ascent");
    check(!traversal.PublishReadiness(oldGeneration, true), "pre-reconstruction readiness callback accepted");

    // A blocked ascent returns to the lower safe point; a blocked descent returns to exterior upper support.
    check(traversal.Request(), "recovered lower state was not interactable");
    generation = traversal.Snapshot().generation;
    traversal.PublishReadiness(generation, true); traversal.TryBegin(generation, true, true);
    input.contactBlocked = true;
    check(step(), "blocked ascent tick rejected");
    state = traversal.Snapshot();
    check(state.phase == Phase::LowerSafe && near(state.supportWorldY, kLowerSupportWorldY),
        "blocked ascent did not roll back to lower safe support");
    check(state.promptReason == PromptReason::Blocked && traversal.CanInteract(),
        "blocked recovery prompt did not persist at its valid safe retry point");
    step();
    check(traversal.Snapshot().promptReason == PromptReason::Blocked,
        "blocked prompt cleared without a fresh consumed interaction");
    input.contactBlocked = false;
    check(traversal.Request(), "fresh ascent request after rollback failed");
    generation = traversal.Snapshot().generation;
    traversal.PublishReadiness(generation, true); traversal.TryBegin(generation, true, true);
    for (int tick = 0; tick < 700 && traversal.Snapshot().phase != Phase::UpperSafe; ++tick) step();
    check(traversal.Snapshot().phase == Phase::UpperSafe, "repeat traversal failed after safe rollback");
    check(traversal.Request(), "repeat upper descent request failed");
    generation = traversal.Snapshot().generation;
    traversal.PublishReadiness(generation, true); traversal.TryBegin(generation, true, true);
    input.contactBlocked = true; step();
    state = traversal.Snapshot();
    check(state.phase == Phase::UpperSafe && near(state.supportWorldY, kUpperSupportWorldY),
        "blocked descent did not roll back to upper safe support");

    const auto beforePause = traversal.Snapshot();
    input.contactBlocked = false;
    input.paused = true;
    check(traversal.Step(input), "paused valid input rejected");
    check(traversal.Snapshot().playerPosition.x == beforePause.playerPosition.x &&
        traversal.Snapshot().supportWorldY == beforePause.supportWorldY,
        "pause advanced traversal");
    input.paused = false;
    input.playerPosition.x = std::numeric_limits<float>::quiet_NaN();
    check(!traversal.Step(input), "nonfinite player input accepted");
    check(traversal.Snapshot().playerPosition.x == beforePause.playerPosition.x,
        "invalid input changed the published snapshot");

    const auto generationBeforeReset = traversal.Snapshot().generation;
    traversal.Reset(kLowerLanding, kLowerSupportWorldY);
    state = traversal.Snapshot();
    check(state.generation > generationBeforeReset && state.claimCount == 0 && state.deploymentCount == 0 &&
        !state.firstAscentCompleted && !state.ropeDeployed && !traversal.PublishReadiness(generationBeforeReset, true),
        "full reset did not clear ownership and reject stale callbacks");
    std::cout << "Rescue traversal cases=" << cases << " failures=" << failures
              << " max_root_step=" << maximumLoadedAscentStepDistance
              << " max_landing_root_step=" << maximumLandingRootStepDistance << '\n';
    return failures == 0 ? 0 : 1;
}
