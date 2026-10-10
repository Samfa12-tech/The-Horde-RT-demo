#include "gameplay/traversal/RescueTraversal.h"
#include "gameplay/traversal/DevelopmentRescueJourney.h"
#include "scene/RescueBlockoutGeometry.h"
#include "scene/RescueJourneyGeometry.h"

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
    const auto checkRopeClearBlockout = [&](const RescueTraversalSnapshot& snapshot) {
        for(std::size_t nodeIndex=1;nodeIndex<kRopeNodeCount;++nodeIndex) {
            const auto& node=snapshot.ropeNodes[nodeIndex];
            for(const auto& box:horde::scene::kRescueBlockoutBoxes) {
                const bool overlaps=node.x>=box.minimum[0]-0.034f&&node.x<=box.maximum[0]+0.034f&&
                    node.y>=box.minimum[1]-0.034f&&node.y<=box.maximum[1]+0.034f&&
                    node.z>=box.minimum[2]-0.034f&&node.z<=box.maximum[2]+0.034f;
                check(!overlaps,"rope particle crossed solid shaft/coping/cantilever blockout");
            }
        }
    };
    const auto checkRopeClearPlayer = [&](const RescueTraversalSnapshot& snapshot) {
        constexpr float capsuleBottom = 0.10f;
        constexpr float capsuleTop = 1.68f;
        constexpr float combinedRadius = 0.22f + 0.035f;
        for (std::size_t nodeIndex = 1u; nodeIndex < kRopeNodeCount; ++nodeIndex) {
            const auto& node = snapshot.ropeNodes[nodeIndex];
            if (node.y < snapshot.supportWorldY + capsuleBottom ||
                node.y > snapshot.supportWorldY + capsuleTop) continue;
            const float radial = std::hypot(node.x - snapshot.playerPosition.x,
                                            node.z - snapshot.playerPosition.z);
            check(radial >= combinedRadius - 0.001f,
                "loaded rope particle crossed the player capsule contact boundary");
        }
    };
    const auto ropeRenderGeometryValid = [&](const RescueTraversalSnapshot& snapshot) {
        using namespace horde::scene;
        constexpr std::array<RescueRopePoint,6> cardinal{{
            {0,1,0},{0,-1,0},{1,0,0},{-1,0,0},{0,0,1},{0,0,-1}}};
        const auto vertices=RescueRopeTriangleVertices(snapshot);
        if(vertices.size()!=528u) return false;
        for(std::size_t i=0;i<vertices.size();i+=3) {
            const auto normal=RopeCross(RopeSubtract(vertices[i+1],vertices[i]),
                                        RopeSubtract(vertices[i+2],vertices[i]));
            const float area=std::sqrt(RopeDot(normal,normal));
            const auto code=RescueRopeTriangleNormalCode(vertices[i],vertices[i+1],vertices[i+2]);
            if(!std::isfinite(area)||area<=1.0e-7f||code>=cardinal.size()||
               RopeDot(normal,cardinal[code])<area*.57f) return false;
            for(std::size_t vertex=i;vertex<i+3;++vertex)
                for(const float component:vertices[vertex])
                    if(!std::isfinite(component)) return false;
        }
        return true;
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
    check(state.lanternClaimed && !state.ropeDeployed && !state.ropeReady && state.claimCount == 1 &&
        state.deploymentCount == 1 && state.deploymentPhase==RopeDeploymentPhase::WaitingForOpening &&
        !traversal.CanInteract(), "claim did not begin the gated rope deployment before interaction");
    const auto& cantilever = horde::scene::kRescueBlockoutBoxes[11];
    check(near(kAnchor.x, -33.7f) && near(kAnchor.y, 3.678f) && near(kAnchor.z, -15.5f) &&
          kAnchor.y + 0.035f + 0.002f <= cantilever.minimum[1] + 0.0001f,
        "rope anchor must attach at the cantilever underside with rope-radius clearance");
    check(near(kUpperSupportWorldY, 2.05f) && near(kLowerSupportWorldY, -0.95f),
        "safe support heights changed");
    check(near(kExteriorLanding.z, -12.8f), "exterior landing must clear the rim");
    check(CanApproachRopeEndpoint({kLowerLanding.x+0.30f,kLowerSupportWorldY,kLowerLanding.z+0.20f},
                                  kLowerSupportWorldY,false),
        "safe support-aligned lower approach was rejected");
    check(!CanApproachRopeEndpoint({kLowerLanding.x,kLowerSupportWorldY+.25f,kLowerLanding.z},
                                   kLowerSupportWorldY+.25f,false),
        "lower approach accepted an unsupported height");
    check(!CanApproachRopeEndpoint({kExteriorLanding.x, kUpperSupportWorldY, -13.55f},
                                   kUpperSupportWorldY,true),
        "upper approach offered a prompt through the front coping");
    check(CanApproachRopeEndpoint({kExteriorLanding.x,kUpperSupportWorldY,-12.90f},
                                  kUpperSupportWorldY,true),
        "safe upper landing approach was rejected");
    check(state.bodyYawRadians == 0.0f, "rope climb body orientation must not depend on camera yaw");

    RescueTraversal pendingWarning;
    pendingWarning.NotifyLanternClaimed();
    pendingWarning.DeployOwned();
    Input pendingInput;
    pendingInput.playerPosition=kLowerLanding;
    pendingInput.supportWorldY=kLowerSupportWorldY;
    pendingInput.contactPlaneWorldY=kLowerSupportWorldY;
    for(int tick=0;tick<60+39;++tick) check(pendingWarning.Step(pendingInput),
        "pending warning setup fixed tick rejected");
    const auto warningGeneration=pendingWarning.Snapshot().generation;
    const auto warningRemaining=pendingWarning.Snapshot().deploymentWaitSecondsRemaining;
    check(pendingWarning.Snapshot().deploymentPhase==RopeDeploymentPhase::WarningBeforeThrow,
        "pending deployment did not reach its warning phase for reconstruction coverage");
    pendingWarning.RecoverForReconstruction();
    check(pendingWarning.Snapshot().generation>warningGeneration &&
          pendingWarning.Snapshot().deploymentPhase==RopeDeploymentPhase::WarningBeforeThrow &&
          pendingWarning.Snapshot().deploymentWaitSecondsRemaining==warningRemaining &&
          !pendingWarning.Snapshot().ropeDeployed,
        "reconstruction replayed or discarded a pending warning instead of invalidating stale state");
    pendingWarning.Reset(kLowerLanding,kLowerSupportWorldY);
    check(pendingWarning.Snapshot().deploymentPhase==RopeDeploymentPhase::Stowed &&
          pendingWarning.Snapshot().deploymentCount==0 && !pendingWarning.Snapshot().ropeDeployed,
        "full reset failed to cancel an in-progress deployment warning");

    Input deploymentInput;
    deploymentInput.playerPosition=kLowerLanding;
    deploymentInput.supportWorldY=kLowerSupportWorldY;
    deploymentInput.contactPlaneWorldY=kLowerSupportWorldY;
    deploymentInput.openingReady=false;
    for(int tick=0;tick<90;++tick) check(traversal.Step(deploymentInput),"waiting deployment tick rejected");
    check(!traversal.Snapshot().ropeDeployed && !traversal.Snapshot().ropeReady,
        "rope appeared before the roof opening completed");
    const auto deploymentBeforePause=traversal.Snapshot();
    deploymentInput.paused=true;
    deploymentInput.openingReady=true;
    check(traversal.Step(deploymentInput) &&
          traversal.Snapshot().deploymentPhase==deploymentBeforePause.deploymentPhase &&
          traversal.Snapshot().deploymentSeconds==deploymentBeforePause.deploymentSeconds &&
          traversal.Snapshot().ropeNodes==deploymentBeforePause.ropeNodes,
        "pause advanced the pending deployment or moved hidden rope particles");
    deploymentInput.paused=false;
    deploymentInput.openingReady=true;
    check(traversal.Step(deploymentInput),"opening-ready deployment tick rejected");
    check(traversal.Snapshot().deploymentPhase==RopeDeploymentPhase::WaitingToThrow &&
          !traversal.Snapshot().ropeDeployed &&
          near(traversal.Snapshot().deploymentWaitSecondsRemaining,.65f,.002f),
        "deployment did not wait until the opening finished before its post-opening pause");
    const float waitBeforeTick=traversal.Snapshot().deploymentWaitSecondsRemaining;
    deploymentInput.paused=true;
    check(traversal.Step(deploymentInput) &&
          traversal.Snapshot().deploymentWaitSecondsRemaining==waitBeforeTick &&
          traversal.Snapshot().deploymentPhase==RopeDeploymentPhase::WaitingToThrow,
        "pause advanced the post-opening pause countdown");
    deploymentInput.paused=false;
    check(traversal.Step(deploymentInput) &&
          traversal.Snapshot().deploymentWaitSecondsRemaining<waitBeforeTick,
        "post-opening pause countdown did not decrease at a fixed step");
    for(int tick=0;tick<80&&traversal.Snapshot().deploymentPhase==RopeDeploymentPhase::WaitingToThrow;++tick)
        check(traversal.Step(deploymentInput),"post-opening pause tick rejected");
    check(traversal.Snapshot().deploymentPhase==RopeDeploymentPhase::WarningBeforeThrow &&
          !traversal.Snapshot().ropeDeployed &&
          near(traversal.Snapshot().deploymentWaitSecondsRemaining,2.0f,.002f),
        "reserved pre-throw interval did not precede the physical throw");
    const float warningBeforePause=traversal.Snapshot().deploymentWaitSecondsRemaining;
    deploymentInput.paused=true;
    check(traversal.Step(deploymentInput) &&
          traversal.Snapshot().deploymentPhase==RopeDeploymentPhase::WarningBeforeThrow &&
          traversal.Snapshot().deploymentWaitSecondsRemaining==warningBeforePause &&
          !traversal.Snapshot().ropeDeployed,
        "pause advanced the reserved pre-throw interval or released the rope");
    deploymentInput.paused=false;
    check(traversal.Step(deploymentInput) &&
          traversal.Snapshot().deploymentWaitSecondsRemaining<warningBeforePause,
        "reserved pre-throw countdown did not expose its fixed-step progress");
    for(int tick=0;tick<130&&!traversal.Snapshot().ropeDeployed;++tick)
        check(traversal.Step(deploymentInput),"reserved pre-throw interval tick rejected");
    const auto thrownPose=traversal.Snapshot();
    check(thrownPose.ropeDeployed && !thrownPose.ropeReady &&
          thrownPose.deploymentPhase==RopeDeploymentPhase::Unfurling,
        "rope did not enter a visible, non-interactable physical unfurl phase");
    check(ropeRenderGeometryValid(thrownPose),
        "actual compact-coil rope frame produced invalid fixed-topology render triangles");
    const auto throwNodes=thrownPose.ropeNodes;
    bool unfurlRenderFramesValid=true;
    for(int tick=0;tick<12;++tick) {
        check(traversal.Step(deploymentInput),"rope unfurl tick rejected");
        unfurlRenderFramesValid=unfurlRenderFramesValid&&ropeRenderGeometryValid(traversal.Snapshot());
    }
    check(unfurlRenderFramesValid,"actual early-unfurl rope frame produced invalid render geometry");
    checkRopeClearBlockout(traversal.Snapshot());
    bool nodeMotion=false;
    for(std::size_t i=1;i<kRopeNodeCount;++i)
        nodeMotion=nodeMotion||std::hypot(traversal.Snapshot().ropeNodes[i].x-throwNodes[i].x,
            traversal.Snapshot().ropeNodes[i].y-throwNodes[i].y,
            traversal.Snapshot().ropeNodes[i].z-throwNodes[i].z)>.03f;
    check(nodeMotion && traversal.Snapshot().deploymentSwing>.07f,
        "thrown rope had no solved particle payout or swing");
    bool settlingRenderFramesValid=true;
    for(int tick=0;tick<360&&!traversal.Snapshot().ropeReady;++tick) {
        check(traversal.Step(deploymentInput),"rope settling tick rejected");
        settlingRenderFramesValid=settlingRenderFramesValid&&ropeRenderGeometryValid(traversal.Snapshot());
    }
    check(settlingRenderFramesValid,"actual paid-out rope trajectory produced invalid render geometry");
    check(traversal.Snapshot().ropeReady &&
          traversal.Snapshot().deploymentPhase==RopeDeploymentPhase::Ready,
        "rope became interaction-ready before it physically paid out and settled on the lower support");
    float settledLength=0.0f;
    for(std::size_t i=0;i+1<kRopeNodeCount;++i)
        settledLength+=std::hypot(traversal.Snapshot().ropeNodes[i+1].x-traversal.Snapshot().ropeNodes[i].x,
                                  traversal.Snapshot().ropeNodes[i+1].y-traversal.Snapshot().ropeNodes[i].y,
                                  traversal.Snapshot().ropeNodes[i+1].z-traversal.Snapshot().ropeNodes[i].z);
    check(settledLength>=kRopeLength*.94f &&
          traversal.Snapshot().ropeNodes.back().y<=kLowerSupportWorldY+.20f,
        "ready rope was not physically paid out to lower support");
    Input bumpInput=deploymentInput;
    std::size_t bumpNodeIndex=1u;
    float bumpHeightError=std::numeric_limits<float>::infinity();
    for(std::size_t i=1;i<kRopeNodeCount;++i) {
        const auto& node=traversal.Snapshot().ropeNodes[i];
        const float targetHeight=kLowerSupportWorldY+0.45f;
        if(node.y<kLowerSupportWorldY+0.10f||node.y>kLowerSupportWorldY+1.68f) continue;
        const float error=std::abs(node.y-targetHeight);
        if(error<bumpHeightError) {bumpHeightError=error;bumpNodeIndex=i;}
    }
    const auto& bumpNode=traversal.Snapshot().ropeNodes[bumpNodeIndex];
    bumpInput.playerPosition={bumpNode.x,kLowerSupportWorldY,bumpNode.z};
    const auto ropeBeforeBump=traversal.Snapshot().ropeNodes;
    check(traversal.Step(bumpInput) && traversal.Snapshot().ropeBumpedPlayer,
        "safe player capsule did not register contact with the deployed rope");
    bool ropeRespondedToBump=false;
    for(std::size_t i=1;i<kRopeNodeCount;++i)
        ropeRespondedToBump=ropeRespondedToBump||
            std::hypot(traversal.Snapshot().ropeNodes[i].x-ropeBeforeBump[i].x,
                       traversal.Snapshot().ropeNodes[i].z-ropeBeforeBump[i].z)>.01f;
    check(ropeRespondedToBump,"player contact did not push the solved rope particles");
    check(traversal.Step(deploymentInput),"restoring lower safe pose after rope bump failed");

    const auto finishDeployment=[&](RescueTraversal& instance) {
        Input deploy;deploy.playerPosition=kLowerLanding;deploy.supportWorldY=kLowerSupportWorldY;
        deploy.contactPlaneWorldY=kLowerSupportWorldY;deploy.openingReady=true;
        for(int tick=0;tick<360&&!instance.Snapshot().ropeReady;++tick) instance.Step(deploy);
        return instance.Snapshot().ropeReady;
    };

    // A contextual interaction can be reached from a real, support-aligned
    // approach position. It must start a bounded motor instead of snapping to
    // the endpoint, while wrong-height and outside-radius poses remain gated.
    RescueTraversal offsetTraversal;
    check(offsetTraversal.NotifyLanternClaimed() && offsetTraversal.DeployOwned(),
        "offset approach fixture did not acquire its one owned rope");
    check(finishDeployment(offsetTraversal),"offset approach fixture rope never became ready");
    Input approachInput;
    approachInput.playerPosition = {kLowerLanding.x + 0.30f, kLowerSupportWorldY,
                                    kLowerLanding.z + 0.20f};
    approachInput.supportWorldY = kLowerSupportWorldY;
    approachInput.contactPlaneWorldY = kLowerSupportWorldY;
    check(offsetTraversal.Step(approachInput), "valid offset approach input rejected");
    check(offsetTraversal.CanInteract(),
        "support-aligned player within the bounded approach radius did not get an actionable climb prompt");
    const auto approachStart = approachInput.playerPosition;
    check(offsetTraversal.Request(), "valid offset climb interaction was not consumed");
    const auto offsetGeneration = offsetTraversal.Snapshot().generation;
    check(offsetTraversal.PublishReadiness(offsetGeneration, true) &&
          offsetTraversal.TryBegin(offsetGeneration, true, true),
        "ready offset climb did not commit its bounded approach");
    float maximumApproachStep = 0.0f;
    auto priorApproachPosition = offsetTraversal.Snapshot().playerPosition;
    for (int tick = 0; tick < 180 && offsetTraversal.Snapshot().phase != Phase::Ascent; ++tick) {
        check(offsetTraversal.Step(approachInput), "approach motor tick rejected");
        const auto& approached = offsetTraversal.Snapshot();
        const float dx = approached.playerPosition.x - priorApproachPosition.x;
        const float dy = approached.playerPosition.y - priorApproachPosition.y;
        const float dz = approached.playerPosition.z - priorApproachPosition.z;
        maximumApproachStep = std::max(maximumApproachStep, std::sqrt(dx*dx+dy*dy+dz*dz));
        check(!approached.ropeHandsActive || approached.phase == Phase::Ascent,
            "rope hands attached before the bounded endpoint approach completed");
        priorApproachPosition = approached.playerPosition;
    }
    check(offsetTraversal.Snapshot().phase == Phase::Ascent,
        "offset approach did not reach the authored lower endpoint");
    check(std::sqrt(std::pow(offsetTraversal.Snapshot().playerPosition.x-approachStart.x,2.0f) +
                    std::pow(offsetTraversal.Snapshot().playerPosition.z-approachStart.z,2.0f)) > 0.30f,
        "offset approach committed by teleporting the player to the rope");
    check(maximumApproachStep <= 0.0251f,
        "offset approach exceeded the per-tick root motion bound");

    RescueTraversal invalidApproachTraversal;
    check(invalidApproachTraversal.NotifyLanternClaimed() && invalidApproachTraversal.DeployOwned(),
        "invalid approach fixture did not acquire its one owned rope");
    check(finishDeployment(invalidApproachTraversal),"invalid approach fixture rope never became ready");
    Input invalidApproach;
    invalidApproach.playerPosition = {kLowerLanding.x + 0.20f, kLowerSupportWorldY + 0.25f,
                                     kLowerLanding.z};
    invalidApproach.supportWorldY = kLowerSupportWorldY + 0.25f;
    invalidApproach.contactPlaneWorldY = kLowerSupportWorldY;
    check(invalidApproachTraversal.Step(invalidApproach) && !invalidApproachTraversal.CanInteract() &&
          !invalidApproachTraversal.Request(),
        "wrong-height player received a climb prompt or started traversal");
    invalidApproach.playerPosition = {kLowerLanding.x + 0.76f, kLowerSupportWorldY, kLowerLanding.z};
    invalidApproach.supportWorldY = kLowerSupportWorldY;
    check(invalidApproachTraversal.Step(invalidApproach) && !invalidApproachTraversal.CanInteract() &&
          !invalidApproachTraversal.Request(),
        "player outside the bounded approach radius received a climb prompt or started traversal");

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

    input.playerPosition.x += 0.80f;
    check(traversal.Step(input) && !traversal.CanInteract(), "approach radius allowed a distant player to attach by teleport");
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
    bool pullupCrossedCoping = false;
    Vec3 initialGripMidpoint{};
    initialGripMidpoint = {
        (traversal.Snapshot().grippingHandTargets[0].x + traversal.Snapshot().grippingHandTargets[1].x) * 0.5f,
        (traversal.Snapshot().grippingHandTargets[0].y + traversal.Snapshot().grippingHandTargets[1].y) * 0.5f,
        (traversal.Snapshot().grippingHandTargets[0].z + traversal.Snapshot().grippingHandTargets[1].z) * 0.5f};
    auto previousGrip = traversal.Snapshot().grippingRopeNodeIndices;
    auto previousHands = traversal.Snapshot().grippingHandTargets;
    auto previousPlayer = traversal.Snapshot().playerPosition;
    for (int tick = 0; tick < 700 && traversal.Snapshot().phase != Phase::UpperSafe; ++tick) {
        const auto phaseBeforeStep = traversal.Snapshot().phase;
        check(step(), "valid ascent tick rejected");
        state = traversal.Snapshot();
        checkHandsClearAnchorBeam(state);
        checkRopeClearBlockout(state);
        if (state.ropeHandsActive) checkRopeClearPlayer(state);
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
                const float handStep = std::sqrt(hx * hx + hy * hy + hz * hz);
                if (handStep > maximumAscentHandStepDistance) {
                    maximumAscentHandStepDistance = handStep;
                }
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
                      near(pullUpStartLeftIndex, 0.5f, 0.001f) &&
                      near(pullUpStartRightIndex, 1.0f, 0.001f) &&
                      pullUpStartLeftIndex < pullUpStartRightIndex &&
                      state.ropeHandsActive &&
                      state.supportWorldY < kUpperSupportWorldY - 0.05f,
                      "loaded pull-up must start from the final clear, reachable rope grips before the rim");
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
        const float anchorClearance = std::hypot(state.playerPosition.x - kAnchor.x,
                                                 state.playerPosition.z - kAnchor.z);
        check(anchorClearance >= 0.28f,
            "loaded traversal root entered the cantilever anchor body-clearance radius");
        if (state.ropeHandsActive) {
            check(state.playerPosition.z >= kAnchor.z + 0.30f - 0.001f,
                "loaded traversal root crossed behind the authored rope-front support plane");
        }
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
            const float expectedSupport = kLowerSupportWorldY + midpoint.y - initialGripMidpoint.y;
            check(near(state.supportWorldY, expectedSupport, 0.04f),
                "player root detached from solved rope grip segment");
        }
    }
    state = traversal.Snapshot();
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
    const auto paidOutNodes=traversal.Snapshot().ropeNodes;
    traversal.RecoverForReconstruction();
    state = traversal.Snapshot();
    check(state.generation > oldGeneration && state.claimCount == 1 && state.deploymentCount == 1 &&
        state.lanternClaimed && state.ropeDeployed && state.ropeReady &&
        state.deploymentPhase==RopeDeploymentPhase::Ready && state.ropeNodes==paidOutNodes &&
        state.firstAscentCompleted,
        "reconstruction recovery redeployed or lost the already paid-out rope/ownership");
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
        !state.firstAscentCompleted && !state.ropeDeployed && !state.ropeReady &&
        state.deploymentPhase==RopeDeploymentPhase::Stowed &&
        !traversal.PublishReadiness(generationBeforeReset, true),
        "full reset did not clear ownership and reject stale callbacks");
    std::cout << "Rescue traversal cases=" << cases << " failures=" << failures
              << " max_root_step=" << maximumLoadedAscentStepDistance
              << " max_landing_root_step=" << maximumLandingRootStepDistance << '\n';
    return failures == 0 ? 0 : 1;
}
