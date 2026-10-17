#include "gameplay/traversal/RescueTraversal.h"
#include "scene/RescueBlockoutGeometry.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace horde::gameplay::traversal {
namespace {

constexpr float kFixedDelta = 1.0f / 60.0f;
constexpr float kAscentGripStep = 0.5f;
constexpr float kGripIntervalSeconds = 0.14f;
constexpr float kPullUpNominalHandStepMetres = 0.035f;
constexpr float kMaximumGripRootStepMetres = 0.025f;
constexpr float kAnchorBodyStandOffMetres = 0.30f;
constexpr float kPullUpSpeed = 0.85f;
constexpr float kDescentSpeed = 0.68f;
constexpr float kRopeGravity = -9.81f;
constexpr float kRopeDamping = 0.985f;
constexpr float kContactSkin = 0.025f;
constexpr float kRopeVerticalSpan = kAnchor.y - (kLowerSupportWorldY + kContactSkin);
constexpr float kPlayerLoadAcceleration = 9.81f;
constexpr int kConstraintIterations = 12;
// Reversible development timing: after the authored one-second roof opening,
// hold briefly, then reserve a pre-throw interval for future voice/subtitle
// completion before releasing the same twelve-particle, 4.8 m line. The
// interval is an engineering placeholder until accepted line-completion drives the throw.
// Payout is a fixed-step
// spool constraint; gravity, contacts and Verlet velocity determine each
// particle's path after release.
constexpr float kPostOpeningDeploymentPauseSeconds = 0.65f;
constexpr float kDeploymentWarningSeconds = 2.0f;
constexpr float kRopePayoutSpeed = 6.0f;
constexpr std::uint16_t kRopeSettlingTicks = 12u;
constexpr float kSafePlayerRadius = 0.22f;
constexpr float kRopeCollisionRadius = 0.035f;
// 35 mm rendered rope plus a 7 mm contact skin also clears a 40 mm gripping
// hand envelope. The former 2 mm skin let valid rope contacts clip that hand.
constexpr float kWorldContactSkin = 0.007f;

bool Finite(Vec3 v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

bool Finite(const Input& input)
{
    return Finite(input.playerPosition) && std::isfinite(input.supportWorldY) &&
        std::isfinite(input.contactPlaneWorldY);
}

Vec3 Add(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 Sub(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 Mul(Vec3 a, float scale) { return {a.x * scale, a.y * scale, a.z * scale}; }
float Length(Vec3 v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
float Clamp(float value, float lo, float hi) { return std::max(lo, std::min(value, hi)); }

Vec3 MoveToward(Vec3 from, Vec3 to, float maxDistance)
{
    const Vec3 delta = Sub(to, from);
    const float length = Length(delta);
    return length <= maxDistance || length <= 1.0e-6f ? to : Add(from, Mul(delta, maxDistance / length));
}

Vec3 MoveTowardSupportFirst(Vec3 from, Vec3 to, float maxDistance)
{
    const float vertical = Clamp(to.y - from.y, -maxDistance, maxDistance);
    const float planarBudget = std::sqrt(std::max(0.0f,
        maxDistance * maxDistance - vertical * vertical));
    const float dx = to.x - from.x;
    const float dz = to.z - from.z;
    const float planarDistance = std::hypot(dx, dz);
    const float scale = planarDistance > planarBudget && planarDistance > 1.0e-6f
        ? planarBudget / planarDistance : 1.0f;
    return {from.x + dx * scale, from.y + vertical, from.z + dz * scale};
}

Vec3 ClampRootToRopeFront(Vec3 target)
{
    // The lower loaded column stays in the open shaft. The stake is to its
    // side, so an anchor-relative Z half-space would incorrectly stop ascent.
    // The separately supported upper handoff is not routed through this clamp.
    target.z = std::min(target.z, kLowerLanding.z);
    target.x = std::min(target.x, kAnchor.x - kAnchorBodyStandOffMetres);
    return target;
}

bool Safe(Phase phase) { return phase == Phase::LowerSafe || phase == Phase::UpperSafe; }
bool Awaiting(Phase phase)
{
    return phase == Phase::AwaitingAscentReadiness || phase == Phase::AwaitingDescentReadiness;
}
bool Moving(Phase phase)
{
    return phase == Phase::ApproachAscent || phase == Phase::ApproachDescent || phase == Phase::Ascent ||
        phase == Phase::PullUp || phase == Phase::Descent || phase == Phase::Landing;
}

Vec3 Sample(const std::array<Vec3, kRopeNodeCount>& nodes, float index)
{
    index = Clamp(index, 0.0f, static_cast<float>(kRopeNodeCount - 1));
    const auto lower = static_cast<std::size_t>(index);
    const auto upper = std::min(lower + 1, kRopeNodeCount - 1);
    const float t = index - static_cast<float>(lower);
    return Add(nodes[lower], Mul(Sub(nodes[upper], nodes[lower]), t));
}

std::array<float,2> SelectLowerGripPair(const std::array<Vec3,kRopeNodeCount>& nodes,
                                         const float supportWorldY, const float yaw)
{
    // The thrown line settles against real shaft contacts and is not a straight
    // nominal-length ladder. Select two solved points the imported shoulders
    // can actually reach, preserving an index gap for alternating hand moves.
    constexpr float step=0.25f;
    constexpr float minimumVerticalSeparation=0.10f;
    constexpr float minimumHandSeparation=0.08f;
    constexpr float minimumIndexGap=0.75f;
    constexpr float shoulderHeight=1.41f;
    constexpr float shoulderHalfWidth=0.19f;
    const float c=std::cos(yaw),s=std::sin(yaw);
    const Vec3 leftShoulder{kLowerLanding.x-shoulderHalfWidth*c,
                            supportWorldY+shoulderHeight,kLowerLanding.z-shoulderHalfWidth*s};
    const Vec3 rightShoulder{kLowerLanding.x+shoulderHalfWidth*c,
                             supportWorldY+shoulderHeight,kLowerLanding.z+shoulderHalfWidth*s};
    std::array<float,2> best{{8.0f,9.0f}};
    float bestScore=std::numeric_limits<float>::infinity();
    for(float left=1.0f;left<=9.75f;left+=step) {
        const Vec3 leftPoint=Sample(nodes,left);
        for(float right=left+minimumIndexGap;right<=10.0f;right+=step) {
            const Vec3 rightPoint=Sample(nodes,right);
            if(leftPoint.y<rightPoint.y+minimumVerticalSeparation ||
               Length(Sub(leftPoint,rightPoint))<minimumHandSeparation) continue;
            const float leftReach=Length(Sub(leftPoint,leftShoulder));
            const float rightReach=Length(Sub(rightPoint,rightShoulder));
            // Prioritize the least-loaded arm pair first; retain modest authored
            // hand-height preference as a tie-breaker for usable ascent motion.
            const float score=std::max(leftReach,rightReach)*4.0f+
                (leftReach+rightReach)*0.1f+
                std::abs(leftPoint.y-(supportWorldY+1.31f))*0.05f+
                std::abs(rightPoint.y-(supportWorldY+0.884f))*0.05f;
            if(score<bestScore) { bestScore=score;best={{left,right}}; }
        }
    }
    return best;
}

void ResolveRopeBlockoutContact(Vec3& node,const float radius,const bool roundedGrip=false)
{
    const auto resolve = [&](const horde::scene::RescueBlockoutBox& box) {
        const Vec3 closest{Clamp(node.x,box.minimum[0],box.maximum[0]),
                           Clamp(node.y,box.minimum[1],box.maximum[1]),
                           Clamp(node.z,box.minimum[2],box.maximum[2])};
        const Vec3 separation=Sub(node,closest);
        const float distance=Length(separation);
        if(roundedGrip&&distance>1.0e-6f) {
            if(distance<radius+kWorldContactSkin)
                node=Add(closest,Mul(separation,(radius+kWorldContactSkin)/distance));
            return;
        }
        const float minX=box.minimum[0]-radius;
        const float minY=box.minimum[1]-radius;
        const float minZ=box.minimum[2]-radius;
        const float maxX=box.maximum[0]+radius;
        const float maxY=box.maximum[1]+radius;
        const float maxZ=box.maximum[2]+radius;
        if(node.x<minX||node.x>maxX||node.y<minY||node.y>maxY||node.z<minZ||node.z>maxZ)
            return;
        const std::array<float,6> exits{{node.x-minX,maxX-node.x,node.y-minY,maxY-node.y,
                                         node.z-minZ,maxZ-node.z}};
        const auto face=static_cast<std::size_t>(std::min_element(exits.begin(),exits.end())-exits.begin());
        switch(face) {
        case 0: node.x=minX-kWorldContactSkin; break;
        case 1: node.x=maxX+kWorldContactSkin; break;
        case 2: node.y=minY-kWorldContactSkin; break;
        case 3: node.y=maxY+kWorldContactSkin; break;
        case 4: node.z=minZ-kWorldContactSkin; break;
        default: node.z=maxZ+kWorldContactSkin; break;
        }
    };
    for(const auto& box:horde::scene::kRescueBlockoutBoxes) resolve(box);
    // The actual apron is a contact surface, including its vertical edge.
    resolve(horde::scene::kRescueBlockoutLanding);
}

std::uint16_t SmoothRegripDurationTicks(float indexDistance)
{
    const float pathDistance = std::abs(indexDistance) * kRopeSegmentRestLength;
    for (std::uint16_t ticks = 1u; ticks <= 120u; ++ticks)
    {
        float previous = 0.0f;
        float maximumStep = 0.0f;
        for (std::uint16_t tick = 1u; tick <= ticks; ++tick)
        {
            const float progress = static_cast<float>(tick) / static_cast<float>(ticks);
            const float eased = progress * progress * (3.0f - 2.0f * progress);
            maximumStep = std::max(maximumStep, (eased - previous) * pathDistance);
            previous = eased;
        }
        if (maximumStep <= kPullUpNominalHandStepMetres)
            return ticks;
    }
    return 120u;
}

} // namespace

RescueTraversal::RescueTraversal()
{
    snapshot_.phase = Phase::LowerSafe;
    snapshot_.playerPosition = kLowerLanding;
    snapshot_.supportWorldY = kLowerSupportWorldY;
    snapshot_.bodyYawRadians = 0.0f;
    for (std::size_t i = 0; i < kRopeNodeCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kRopeNodeCount - 1);
        snapshot_.ropeNodes[i] = Add(kAnchor, Mul(Vec3{0.0f, -kRopeVerticalSpan, 0.0f}, t));
        previousRopeNodes_[i] = snapshot_.ropeNodes[i];
    }
    gripReferenceWorld_ = Mul(Add(Sample(snapshot_.ropeNodes, 8.0f), Sample(snapshot_.ropeNodes, 9.0f)), 0.5f);
    PublishHands();
    RefreshPrompt();
}

void RescueTraversal::RefreshPrompt()
{
    if (promptBlocked_) {
        snapshot_.promptReason = PromptReason::Blocked;
        return;
    }
    if (!snapshot_.ropeReady) {
        snapshot_.promptReason = snapshot_.deploymentCount ? PromptReason::WaitingForReadiness
                                                          : PromptReason::Blocked;
        return;
    }
    switch (snapshot_.phase) {
    case Phase::LowerSafe: snapshot_.promptReason = PromptReason::ClimbRope; break;
    case Phase::UpperSafe: snapshot_.promptReason = PromptReason::ReturnDown; break;
    case Phase::AwaitingAscentReadiness:
    case Phase::AwaitingDescentReadiness: snapshot_.promptReason = PromptReason::WaitingForReadiness; break;
    default: snapshot_.promptReason = PromptReason::Busy; break;
    }
}

bool RescueTraversal::IsActive() const { return !Safe(snapshot_.phase); }
bool RescueTraversal::CanInteract() const
{
    return CanInteractAt(snapshot_.playerPosition,snapshot_.supportWorldY);
}
bool RescueTraversal::CanInteractAt(const Vec3 playerPosition,const float supportWorldY) const
{
    if (!snapshot_.ropeReady || !Finite(playerPosition) || !std::isfinite(supportWorldY)) return false;
    const bool lower=snapshot_.phase==Phase::LowerSafe;
    const bool upper=snapshot_.phase==Phase::UpperSafe&&snapshot_.exteriorSide;
    if(!lower&&!upper) return false;
    const Vec3 expected=lower?kLowerLanding:kExteriorLanding;
    const float expectedSupport=lower?kLowerSupportWorldY:kUpperSupportWorldY;
    return std::hypot(playerPosition.x-expected.x,playerPosition.z-expected.z)<=kRescueApproachRadius &&
        std::abs(supportWorldY-expectedSupport)<=.10f;
}

bool RescueTraversal::NotifyLanternClaimed()
{
    if (snapshot_.lanternClaimed) return false;
    snapshot_.lanternClaimed = true;
    if (snapshot_.claimCount < std::numeric_limits<std::uint32_t>::max()) ++snapshot_.claimCount;
    RefreshPrompt();
    return true;
}

bool RescueTraversal::DeployOwned()
{
    if (!snapshot_.lanternClaimed || snapshot_.deploymentCount) return false;
    snapshot_.deploymentPhase=RopeDeploymentPhase::WaitingForOpening;
    snapshot_.deploymentSeconds=0.0f;
    snapshot_.deploymentWaitSecondsRemaining=kPostOpeningDeploymentPauseSeconds;
    snapshot_.ropeDeployed=false;
    snapshot_.ropeReady=false;
    if (snapshot_.deploymentCount < std::numeric_limits<std::uint32_t>::max()) ++snapshot_.deploymentCount;
    deploymentDelaySeconds_=0.0f;
    deploymentPaidOutLength_=0.0f;
    deploymentSettleTicks_=0u;
    RefreshPrompt();
    return true;
}

void RescueTraversal::AdvanceDeployment(const Input& input)
{
    if(snapshot_.deploymentPhase==RopeDeploymentPhase::WaitingForOpening) {
        if(input.openingReady) {
            snapshot_.deploymentPhase=RopeDeploymentPhase::WaitingToThrow;
            snapshot_.deploymentWaitSecondsRemaining=kPostOpeningDeploymentPauseSeconds;
        }
        return;
    }
    if(snapshot_.deploymentPhase==RopeDeploymentPhase::WaitingToThrow) {
        deploymentDelaySeconds_+=kFixedDelta;
        snapshot_.deploymentWaitSecondsRemaining=std::max(0.0f,
            kPostOpeningDeploymentPauseSeconds-deploymentDelaySeconds_);
        if(deploymentDelaySeconds_+1.0e-6f>=kPostOpeningDeploymentPauseSeconds) {
            snapshot_.deploymentPhase=RopeDeploymentPhase::WarningBeforeThrow;
            snapshot_.deploymentWaitSecondsRemaining=kDeploymentWarningSeconds;
            deploymentDelaySeconds_=0.0f;
        }
        return;
    }
    if(snapshot_.deploymentPhase==RopeDeploymentPhase::WarningBeforeThrow) {
        deploymentDelaySeconds_+=kFixedDelta;
        snapshot_.deploymentWaitSecondsRemaining=std::max(0.0f,
            kDeploymentWarningSeconds-deploymentDelaySeconds_);
        if(deploymentDelaySeconds_+1.0e-6f>=kDeploymentWarningSeconds) BeginRopeThrow();
        return;
    }
    if(snapshot_.deploymentPhase==RopeDeploymentPhase::Unfurling) {
        snapshot_.deploymentSeconds+=kFixedDelta;
        deploymentPaidOutLength_=std::min(kRopeLength,snapshot_.deploymentSeconds*kRopePayoutSpeed);
        if(deploymentPaidOutLength_>=kRopeLength-1.0e-4f) {
            deploymentPaidOutLength_=kRopeLength;
            snapshot_.deploymentPhase=RopeDeploymentPhase::Settling;
        }
    }
}

void RescueTraversal::BeginRopeThrow()
{
    snapshot_.deploymentPhase=RopeDeploymentPhase::Unfurling;
    snapshot_.ropeDeployed=true;
    snapshot_.deploymentSeconds=0.0f;
    snapshot_.deploymentWaitSecondsRemaining=0.0f;
    deploymentPaidOutLength_=0.0f;
    deploymentSettleTicks_=0u;
    for(std::size_t i=0;i<kRopeNodeCount;++i) {
        const float t=static_cast<float>(i)/static_cast<float>(kRopeNodeCount-1);
        const float angle=t*6.0f*3.14159265358979323846f;
        const Vec3 p=Add(kAnchor,{0.055f*std::cos(angle),-0.018f*static_cast<float>(i),
                                  -0.10f+0.055f*std::sin(angle)});
        snapshot_.ropeNodes[i]=p;
        // Most launch energy points down the shaft; a small lateral component
        // gives the cord a visible real swing without throwing reachable grips
        // outside the imported forearm envelope at the lower endpoint.
        const Vec3 velocity{0.08f*std::sin(angle),-5.6f*t,-0.40f*t+0.12f*std::cos(angle)};
        previousRopeNodes_[i]=Sub(p,Mul(velocity,kFixedDelta));
    }
    snapshot_.ropeNodes[0]=kAnchor;
    previousRopeNodes_[0]=kAnchor;
    snapshot_.deploymentSwing=0.0f;
}

void RescueTraversal::UpdateDeploymentReadiness(const float contactPlaneWorldY)
{
    if(snapshot_.deploymentPhase!=RopeDeploymentPhase::Settling) return;
    float length=0.0f;
    for(std::size_t i=0;i+1<kRopeNodeCount;++i)
        length+=Length(Sub(snapshot_.ropeNodes[i+1],snapshot_.ropeNodes[i]));
    const bool paidOut=length>=kRopeLength*0.94f;
    const bool grounded=snapshot_.ropeNodes.back().y<=contactPlaneWorldY+0.20f;
    if(paidOut&&grounded) {
        if(deploymentSettleTicks_<std::numeric_limits<std::uint16_t>::max()) ++deploymentSettleTicks_;
    } else deploymentSettleTicks_=0u;
    if(deploymentSettleTicks_>=kRopeSettlingTicks) {
        snapshot_.deploymentPhase=RopeDeploymentPhase::Ready;
        snapshot_.ropeReady=true;
    }
}

void RescueTraversal::ResolveSafePlayerContact()
{
    // Preserve the actual player/rope contact manifold through the handoff
    // from an idle approach into a loaded grip. Dropping this projection as
    // soon as the player grabs the line lets particles previously pushed off
    // the capsule spring back through the body and creates a rope/root snap.
    if((!Safe(snapshot_.phase)&&!snapshot_.ropeHandsActive&&!snapshot_.equipmentStowed)||!snapshot_.ropeDeployed) return;
    constexpr float capsuleBottom=0.10f;
    const float capsuleTop=1.68f-snapshot_.plantedPelvisDropMetres;
    const float combined=kSafePlayerRadius+kRopeCollisionRadius;
    for(std::size_t i=1;i<kRopeNodeCount;++i) {
        Vec3& node=snapshot_.ropeNodes[i];
        const float centreY=Clamp(node.y,snapshot_.supportWorldY+capsuleBottom+kSafePlayerRadius,
            snapshot_.supportWorldY+capsuleTop-kSafePlayerRadius);
        const float backset=0.25f*snapshot_.plantedPelvisDropMetres;
        const float dx=node.x-(snapshot_.playerPosition.x-backset*std::sin(snapshot_.bodyYawRadians));
        const float dy=node.y-centreY;
        const float dz=node.z-(snapshot_.playerPosition.z+backset*std::cos(snapshot_.bodyYawRadians));
        // A capsule has rounded ends. The old flat cylinder admitted a node
        // above its top, then projected it a full radius when the stance grew
        // by a few millimetres, snapping a loaded hand through the air.
        const float radial=std::sqrt(dx*dx+dy*dy+dz*dz);
        if(radial>=combined) continue;
        const float nx=radial>1.0e-5f?dx/radial:0.0f;
        const float ny=radial>1.0e-5f?dy/radial:0.0f;
        const float nz=radial>1.0e-5f?dz/radial:1.0f;
        const Vec3 correction{nx*(combined-radial),ny*(combined-radial),nz*(combined-radial)};
        Vec3 velocity = Sub(node, previousRopeNodes_[i]);
        const float inwardVelocity = velocity.x * nx + velocity.y*ny + velocity.z * nz;
        if (inwardVelocity < 0.0f) {
            velocity.x -= nx * inwardVelocity;
            velocity.y -= ny * inwardVelocity;
            velocity.z -= nz * inwardVelocity;
        }
        node=Add(node,correction);
        // Verlet derives velocity from current-minus-previous position. If a
        // contact projection moves only the current node, the correction is
        // misread as a large outward impulse on the next fixed tick. Preserve
        // tangential/outward motion while removing penetration and inward
        // normal velocity so a real pre-grip bump remains continuous at grab.
        previousRopeNodes_[i] = Sub(node, velocity);
        snapshot_.ropeBumpedPlayer=true;
    }
}

void RescueTraversal::BeginRequest(bool ascent)
{
    if (snapshot_.generation < std::numeric_limits<std::uint64_t>::max()) ++snapshot_.generation;
    snapshot_.phase = ascent ? Phase::AwaitingAscentReadiness : Phase::AwaitingDescentReadiness;
    snapshot_.equipmentStowed = true;
    snapshot_.ropeHandsActive = false;
    snapshot_.ropeTension = 0.0f;
    if(!ascent) snapshot_.grippingRopeNodeIndices={kPullUpLeftGripNodeIndex,kPullUpRightGripNodeIndex};
    readinessReceived_ = false;
    readinessReady_ = false;
    motorTicks_ = 0;
    nextGripIsLeft_ = true;
    pullUpAlignmentActive_ = false;
    RefreshPrompt();
}

bool RescueTraversal::Request()
{
    if (!CanInteract()) return false;
    promptBlocked_ = false;
    if (snapshot_.phase == Phase::LowerSafe) {
        gripReferencePlayerPosition_ = snapshot_.playerPosition;
        snapshot_.grippingRopeNodeIndices=SelectLowerGripPair(snapshot_.ropeNodes,
                                                               snapshot_.supportWorldY,snapshot_.bodyYawRadians);
        PublishHands();
        // Each route cycle rebases on the selected currently solved grips, so
        // settled sag from the previous climb cannot introduce a root jump.
        gripReferenceWorld_ = Mul(Add(snapshot_.grippingHandTargets[0],
                                      snapshot_.grippingHandTargets[1]), 0.5f);
    }
    BeginRequest(snapshot_.phase == Phase::LowerSafe);
    return true;
}

void RescueTraversal::AbortRequest()
{
    const bool upper = snapshot_.phase == Phase::AwaitingDescentReadiness;
    RollbackToSafeEndpoint(upper);
    if (snapshot_.generation < std::numeric_limits<std::uint64_t>::max()) ++snapshot_.generation;
    promptBlocked_ = true;
    snapshot_.promptReason = PromptReason::Blocked;
}

bool RescueTraversal::TryBegin(std::uint64_t generation, bool claimOwned, bool renderReady)
{
    if (!Awaiting(snapshot_.phase)) return false;
    const bool matchingReady = generation == snapshot_.generation && readinessReceived_ && readinessReady_;
    if (!matchingReady || !claimOwned || !renderReady) {
        AbortRequest();
        return false;
    }
    const bool ascent = snapshot_.phase == Phase::AwaitingAscentReadiness;
    const Vec3 endpoint=ascent?kLowerLanding:kExteriorLanding;
    const float endpointDistance=std::hypot(snapshot_.playerPosition.x-endpoint.x,
                                            snapshot_.playerPosition.z-endpoint.z);
    const float targetYaw=std::atan2(kAnchor.x-endpoint.x,-(kAnchor.z-endpoint.z));
    const bool needsApproach=endpointDistance>0.001f ||
        std::abs(snapshot_.supportWorldY-(ascent?kLowerSupportWorldY:kUpperSupportWorldY))>0.001f ||
        (ascent && std::abs(std::remainder(targetYaw-snapshot_.bodyYawRadians,6.28318530718f))>.041f);
    snapshot_.phase = needsApproach ? (ascent?Phase::ApproachAscent:Phase::ApproachDescent)
                                    : (ascent ? Phase::Ascent : Phase::Descent);
    approachSafePosition_=snapshot_.playerPosition;
    approachSafeSupportWorldY_=snapshot_.supportWorldY;
    pullUpStage_ = PullUpStage::Handoff;
    descentStage_ = DescentStage::Lift;
    snapshot_.equipmentStowed = true;
    snapshot_.ropeHandsActive = ascent&&!needsApproach;
    if(!needsApproach) {
        snapshot_.sawAscent = ascent || snapshot_.sawAscent;
        snapshot_.sawDescent = !ascent || snapshot_.sawDescent;
    }
    readinessReceived_ = false;
    readinessReady_ = false;
    motorTicks_ = 0;
    pullUpAlignmentActive_ = false;
    RefreshPrompt();
    return true;
}

bool RescueTraversal::PublishReadiness(std::uint64_t generation, bool ready)
{
    if (generation != snapshot_.generation || !Awaiting(snapshot_.phase)) return false;
    readinessReceived_ = true;
    readinessReady_ = ready;
    return true;
}

void RescueTraversal::Reset(Vec3 safePosition, float safeSupportWorldY)
{
    if (!Finite(safePosition) || !std::isfinite(safeSupportWorldY)) return;
    const bool upper = safeSupportWorldY >= (kLowerSupportWorldY + kUpperSupportWorldY) * 0.5f;
    const std::uint64_t nextGeneration = snapshot_.generation < std::numeric_limits<std::uint64_t>::max()
        ? snapshot_.generation + 1 : snapshot_.generation;
    snapshot_ = {};
    snapshot_.generation = nextGeneration;
    snapshot_.phase = upper ? Phase::UpperSafe : Phase::LowerSafe;
    snapshot_.playerPosition = upper ? kExteriorLanding : kLowerLanding;
    snapshot_.supportWorldY = upper ? kUpperSupportWorldY : kLowerSupportWorldY;
    snapshot_.bodyYawRadians = 0.0f;
    gripReferencePlayerPosition_ = kLowerLanding;
    approachSafePosition_=snapshot_.playerPosition;
    approachSafeSupportWorldY_=snapshot_.supportWorldY;
    snapshot_.exteriorSide = upper;
    snapshot_.equipmentStowed = false;
    for (std::size_t i = 0; i < kRopeNodeCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kRopeNodeCount - 1);
        snapshot_.ropeNodes[i] = Add(kAnchor, Mul(Vec3{0.0f, -kRopeVerticalSpan, 0.0f}, t));
        previousRopeNodes_[i] = snapshot_.ropeNodes[i];
    }
    snapshot_.grippingRopeNodeIndices = upper ? std::array<float, 2>{0.75f, 1.0f}
                                                : std::array<float, 2>{8.0f, 9.0f};
    gripReferenceWorld_ = Mul(Add(Sample(snapshot_.ropeNodes, 8.0f), Sample(snapshot_.ropeNodes, 9.0f)), 0.5f);
    readinessReceived_ = readinessReady_ = false;
    promptBlocked_ = false;
    motorTicks_ = 0;
    deploymentSettleTicks_=0u;
    deploymentDelaySeconds_=0.0f;
    deploymentPaidOutLength_=0.0f;
    nextGripIsLeft_ = true;
    pullUpAlignmentActive_ = false;
    PublishHands();
    RefreshPrompt();
}

void RescueTraversal::RestoreCampaignSafeSide(bool upper, bool firstAscent, std::uint64_t generationFloor)
{
    *this = RescueTraversal{};
    snapshot_.generation = generationFloor + 1;
    snapshot_.claimCount = snapshot_.deploymentCount = 1;
    snapshot_.lanternClaimed = snapshot_.ropeDeployed = snapshot_.ropeReady = true;
    snapshot_.deploymentPhase = RopeDeploymentPhase::Ready;
    snapshot_.firstAscentCompleted = firstAscent;
    deploymentPaidOutLength_ = kRopeLength;
    RollbackToSafeEndpoint(upper);
}

void RescueTraversal::RecoverForReconstruction()
{
    const bool descentSide = snapshot_.phase == Phase::UpperSafe || snapshot_.exteriorSide ||
        snapshot_.phase == Phase::AwaitingDescentReadiness || snapshot_.phase == Phase::Descent ||
        snapshot_.phase == Phase::Landing;
    const auto claims = snapshot_.claimCount;
    const auto deployments = snapshot_.deploymentCount;
    const bool claimed = snapshot_.lanternClaimed;
    const bool deployed = snapshot_.ropeDeployed;
    const bool ropeReady=snapshot_.ropeReady;
    const auto deploymentPhase=snapshot_.deploymentPhase;
    const float deploymentSeconds=snapshot_.deploymentSeconds;
    const float deploymentWaitSecondsRemaining=snapshot_.deploymentWaitSecondsRemaining;
    const float deploymentSwing=snapshot_.deploymentSwing;
    const auto ropeNodes=snapshot_.ropeNodes;
    const auto previousNodes=previousRopeNodes_;
    const float delaySeconds=deploymentDelaySeconds_;
    const float paidOutLength=deploymentPaidOutLength_;
    const auto settleTicks=deploymentSettleTicks_;
    const bool firstAscent = snapshot_.firstAscentCompleted;
    Reset(descentSide ? kExteriorLanding : kLowerLanding,
          descentSide ? kUpperSupportWorldY : kLowerSupportWorldY);
    snapshot_.claimCount = claims;
    snapshot_.deploymentCount = deployments;
    snapshot_.lanternClaimed = claimed;
    snapshot_.ropeDeployed = deployed;
    snapshot_.ropeReady=ropeReady;
    snapshot_.deploymentPhase=deploymentPhase;
    snapshot_.deploymentSeconds=deploymentSeconds;
    snapshot_.deploymentWaitSecondsRemaining=deploymentWaitSecondsRemaining;
    snapshot_.deploymentSwing=deploymentSwing;
    snapshot_.ropeNodes=ropeNodes;
    previousRopeNodes_=previousNodes;
    deploymentDelaySeconds_=delaySeconds;
    deploymentPaidOutLength_=paidOutLength;
    deploymentSettleTicks_=settleTicks;
    snapshot_.firstAscentCompleted = firstAscent;
    PublishHands();
    snapshot_.promptReason = PromptReason::Blocked;
    promptBlocked_ = true;
}

void RescueTraversal::RollbackToSafeEndpoint(bool upper)
{
    snapshot_.phase = upper ? Phase::UpperSafe : Phase::LowerSafe;
    snapshot_.playerPosition = upper ? kExteriorLanding : kLowerLanding;
    snapshot_.supportWorldY = upper ? kUpperSupportWorldY : kLowerSupportWorldY;
    snapshot_.exteriorSide = upper;
    snapshot_.equipmentStowed = false;
    snapshot_.ropeHandsActive = false;
    snapshot_.ropeTension = 0.0f;
    snapshot_.grippingRopeNodeIndices = upper ? std::array<float, 2>{0.75f, 1.0f}
                                                : std::array<float, 2>{8.0f, 9.0f};
    readinessReceived_ = readinessReady_ = false;
    snapshot_.plantedPelvisDropMetres=0.0f;
    promptBlocked_ = false;
    PublishHands();
    RefreshPrompt();
}

void RescueTraversal::AbortApproach()
{
    const bool upper=snapshot_.phase==Phase::ApproachDescent;
    snapshot_.phase=upper?Phase::UpperSafe:Phase::LowerSafe;
    snapshot_.exteriorSide=upper;
    // Retain the actual collision-validated request side, rather than marking
    // a partially approached pose safe after a new blocking contact.
    snapshot_.playerPosition=approachSafePosition_;
    snapshot_.supportWorldY=approachSafeSupportWorldY_;
    snapshot_.equipmentStowed=false;
    snapshot_.ropeHandsActive=false;
    snapshot_.ropeTension=0.0f;
    snapshot_.plantedPelvisDropMetres=0.0f;
    readinessReceived_=readinessReady_=false;
    promptBlocked_=true;
    RefreshPrompt();
}

void RescueTraversal::PublishHands()
{
    snapshot_.grippingHandTargets[0] = Sample(snapshot_.ropeNodes, snapshot_.grippingRopeNodeIndices[0]);
    snapshot_.grippingHandTargets[1] = Sample(snapshot_.ropeNodes, snapshot_.grippingRopeNodeIndices[1]);
}

void RescueTraversal::AdvanceAlternatingGrip(const bool towardAnchor)
{
    // Let the loaded body settle on its solved target before another downwards
    // hand advance. A contact-induced root lag must not drag the hands beyond
    // the unchanged anatomical envelope while the body catches up.
    if (!towardAnchor && std::min(snapshot_.grippingHandTargets[0].y,
                                  snapshot_.grippingHandTargets[1].y) < snapshot_.supportWorldY+0.30f)
        return;
    const std::size_t hand = nextGripIsLeft_ ? 0u : 1u;
    float& index = snapshot_.grippingRopeNodeIndices[hand];
    if (motorTicks_ == 0u)
    {
        gripAdvanceStartIndex_ = index;
        if (towardAnchor)
        {
            const float minimumIndex = hand == 0u ? 0.5f : 1.0f;
            gripAdvanceTargetIndex_ = std::max(minimumIndex,
                                                index - kAscentGripStep);
        }
        else
        {
            const float maximumIndex = hand == 0u ? 8.0f : 9.0f;
            gripAdvanceTargetIndex_ = std::min(maximumIndex,
                                                index + kAscentGripStep);
        }
    }

    ++motorTicks_;
    const float progress = Clamp(
        static_cast<float>(motorTicks_) * kFixedDelta / kGripIntervalSeconds,
        0.0f, 1.0f);
    const float eased = progress * progress * (3.0f - 2.0f * progress);
    index = gripAdvanceStartIndex_ +
        (gripAdvanceTargetIndex_ - gripAdvanceStartIndex_) * eased;
    if (progress >= 1.0f)
    {
        index = gripAdvanceTargetIndex_;
        motorTicks_ = 0u;
        nextGripIsLeft_ = !nextGripIsLeft_;
    }
}

void RescueTraversal::AdvanceMotion()
{
    // The climb motor alternates half-node hand advances. Vertical root motion
    // is resolved from solved grip particles below, so rope sag and contacts
    // move the player with them; only the short pull-up/descent planar legs use
    // the authored kinematic path.
    if(snapshot_.phase==Phase::ApproachAscent||snapshot_.phase==Phase::ApproachDescent) {
        const bool ascent=snapshot_.phase==Phase::ApproachAscent;
        const Vec3 target=ascent?kLowerLanding:kExteriorLanding;
        const float support=ascent?kLowerSupportWorldY:kUpperSupportWorldY;
        snapshot_.playerPosition=MoveToward(snapshot_.playerPosition,target,kRescueMaximumApproachStep);
        snapshot_.supportWorldY=MoveToward({0.0f,snapshot_.supportWorldY,0.0f},
            {0.0f,support,0.0f},kRescueMaximumApproachStep).y;
        snapshot_.playerPosition.y=snapshot_.supportWorldY;
        if(std::hypot(snapshot_.playerPosition.x-target.x,snapshot_.playerPosition.z-target.z)<=0.001f &&
           std::abs(snapshot_.supportWorldY-support)<=0.001f) {
            snapshot_.playerPosition=target;
            snapshot_.supportWorldY=support;
            // Keep the boots grounded and hands free until the bounded body
            // turn faces the stake. Camera input remains independent. Scoring
            // a grip before this turn retained an off-centre approach pose and
            // could pull the right hand beyond its native reach during ascent.
            const float targetYaw=std::atan2(kAnchor.x-target.x,-(kAnchor.z-target.z));
            if(ascent && std::abs(std::remainder(targetYaw-snapshot_.bodyYawRadians,
                6.28318530718f))>.041f)return;
            if(ascent) snapshot_.grippingRopeNodeIndices=SelectLowerGripPair(
                snapshot_.ropeNodes,snapshot_.supportWorldY,snapshot_.bodyYawRadians);
            snapshot_.phase=ascent?Phase::Ascent:Phase::Descent;
            snapshot_.ropeHandsActive=ascent;
            if(ascent) {
                // Approach relinquished the request-time offset. Couple the
                // loaded root to the current solved grips at the admitted
                // landing, not to the discarded off-centre request pose.
                gripReferencePlayerPosition_=snapshot_.playerPosition;
                gripReferenceWorld_=Mul(Add(
                    Sample(snapshot_.ropeNodes,snapshot_.grippingRopeNodeIndices[0]),
                    Sample(snapshot_.ropeNodes,snapshot_.grippingRopeNodeIndices[1])),0.5f);
            }
            snapshot_.sawAscent=ascent||snapshot_.sawAscent;
            snapshot_.sawDescent=!ascent||snapshot_.sawDescent;
            motorTicks_=0u;
        }
        return;
    }
    if (snapshot_.phase == Phase::Ascent) {
        AdvanceAlternatingGrip(true);
        return;
    }
    if (snapshot_.phase == Phase::PullUp) {
        if (pullUpStage_ == PullUpStage::RegripLeft ||
            pullUpStage_ == PullUpStage::RegripRight) {
            snapshot_.playerPosition=MoveToward(snapshot_.playerPosition,kRimLanding,
                                                  kMaximumGripRootStepMetres);
            snapshot_.playerPosition.y=snapshot_.supportWorldY;
            const bool movingLeft = pullUpStage_ == PullUpStage::RegripLeft;
            float& index = snapshot_.grippingRopeNodeIndices[movingLeft ? 0u : 1u];
            if (motorTicks_ == 0u) {
                gripAdvanceStartIndex_ = index;
                gripAdvanceTargetIndex_ = movingLeft ? kPullUpLeftGripNodeIndex
                                                     : kPullUpRightGripNodeIndex;
                gripAdvanceIntervalTicks_ = SmoothRegripDurationTicks(
                    gripAdvanceTargetIndex_ - gripAdvanceStartIndex_);
            }
            ++motorTicks_;
            const float progress = Clamp(static_cast<float>(motorTicks_) /
                static_cast<float>(gripAdvanceIntervalTicks_), 0.0f, 1.0f);
            const float eased = progress * progress * (3.0f - 2.0f * progress);
            index = gripAdvanceStartIndex_ +
                (gripAdvanceTargetIndex_ - gripAdvanceStartIndex_) * eased;
            if (progress >= 1.0f) {
                index = gripAdvanceTargetIndex_;
                motorTicks_ = 0u;
                // Move the left hand upward first, then the right hand to its
                // landing grip. This preserves a wide, ordered support span
                // throughout the solved rope handoff.
                pullUpStage_ = movingLeft ? PullUpStage::RegripRight
                                          : PullUpStage::Lift;
            }
        } else if (pullUpStage_ == PullUpStage::Handoff) {
            snapshot_.playerPosition = MoveToward(snapshot_.playerPosition,
                kApronHandoff, kPullUpSpeed * kFixedDelta);
            snapshot_.playerPosition.y = snapshot_.supportWorldY;
            if (snapshot_.playerPosition == kApronHandoff) {
                // The player is now over the actual upper apron. Until this
                // support transition, both arms remain attached to rope nodes.
                snapshot_.ropeHandsActive = false;
                pullUpStage_ = PullUpStage::Lift;
            }
        } else if (pullUpStage_ == PullUpStage::Lift) {
            const float liftTarget = snapshot_.ropeHandsActive ? kUpperSupportWorldY
                                                               : kCopingClearanceSupportWorldY;
            snapshot_.supportWorldY = std::min(liftTarget,
                snapshot_.supportWorldY + kPullUpSpeed * kFixedDelta);
            if (snapshot_.supportWorldY >= liftTarget - 1.0e-5f)
                pullUpStage_ = snapshot_.ropeHandsActive ? PullUpStage::Handoff
                                                         : PullUpStage::Cross;
        } else if (pullUpStage_ == PullUpStage::Cross) {
            const Vec3 crossedLanding{kExteriorLanding.x, kCopingClearanceSupportWorldY, kExteriorLanding.z};
            snapshot_.playerPosition = MoveToward(snapshot_.playerPosition, crossedLanding,
                kPullUpSpeed * kFixedDelta);
            if (snapshot_.playerPosition.x == crossedLanding.x && snapshot_.playerPosition.z == crossedLanding.z)
                pullUpStage_ = PullUpStage::Settle;
        } else {
            snapshot_.supportWorldY = std::max(kUpperSupportWorldY,
                snapshot_.supportWorldY - kPullUpSpeed * kFixedDelta);
            if (snapshot_.supportWorldY <= kUpperSupportWorldY + 1.0e-5f) {
                snapshot_.supportWorldY = kUpperSupportWorldY;
                snapshot_.playerPosition = kExteriorLanding;
                snapshot_.phase = Phase::UpperSafe;
                snapshot_.exteriorSide = true;
                snapshot_.firstAscentCompleted = true;
                snapshot_.sawPullUp = true;
                snapshot_.equipmentStowed = false;
            }
        }
        snapshot_.playerPosition.y = snapshot_.supportWorldY;
        return;
    }
    if (snapshot_.phase == Phase::Descent) {
        if (descentStage_ == DescentStage::Lift) {
            snapshot_.supportWorldY = std::min(kCopingClearanceSupportWorldY,
                snapshot_.supportWorldY + kDescentSpeed * kFixedDelta);
            if (snapshot_.supportWorldY >= kCopingClearanceSupportWorldY - 1.0e-5f)
                descentStage_ = DescentStage::Cross;
        } else if (descentStage_ == DescentStage::Cross) {
            const Vec3 crossedApron{kApronHandoff.x, kCopingClearanceSupportWorldY, kApronHandoff.z};
            snapshot_.playerPosition = MoveToward(snapshot_.playerPosition, crossedApron,
                kDescentSpeed * kFixedDelta);
            if (snapshot_.playerPosition.x == crossedApron.x && snapshot_.playerPosition.z == crossedApron.z)
                descentStage_ = DescentStage::Settle;
        } else if (descentStage_ == DescentStage::Settle) {
            snapshot_.supportWorldY = std::max(kUpperSupportWorldY,
                snapshot_.supportWorldY - kDescentSpeed * kFixedDelta);
            if (snapshot_.supportWorldY <= kUpperSupportWorldY + 1.0e-5f) {
                snapshot_.supportWorldY = kUpperSupportWorldY;
                snapshot_.playerPosition = kApronHandoff;
                descentStage_ = DescentStage::Crouch;
                motorTicks_ = 0u;
            }
        } else if (descentStage_ == DescentStage::Crouch) {
            const float desired=Clamp(snapshot_.supportWorldY+1.45f-
                std::min(snapshot_.grippingHandTargets[0].y,snapshot_.grippingHandTargets[1].y)-0.40f,0.0f,0.72f);
            if(snapshot_.plantedPelvisDropMetres>=desired-0.01f) {
                snapshot_.ropeHandsActive=true;
                descentStage_=DescentStage::Back;
            }
        } else if (descentStage_ == DescentStage::Back) {
            snapshot_.playerPosition=MoveToward(snapshot_.playerPosition,kRimLanding,
                kDescentSpeed*kFixedDelta);
            if(snapshot_.playerPosition==kRimLanding) {
                descentStage_=DescentStage::Rope;
                gripReferencePlayerPosition_ = snapshot_.playerPosition;
                gripReferenceWorld_ = Mul(Add(Sample(snapshot_.ropeNodes, kPullUpLeftGripNodeIndex),
                                              Sample(snapshot_.ropeNodes, kPullUpRightGripNodeIndex)), 0.5f);
                motorTicks_ = 0u;
            }
        }
        snapshot_.playerPosition.y = snapshot_.supportWorldY;
        if (descentStage_ == DescentStage::Rope) AdvanceAlternatingGrip(false);
        return;
    }
    if (snapshot_.phase == Phase::Landing) {
        if(snapshot_.ropeHandsActive)AdvanceAlternatingGrip(false);
    }
}

void RescueTraversal::ResolveRope(float contactPlaneWorldY)
{
    snapshot_.ropeNodes[0] = kAnchor;
    const float left = Clamp(snapshot_.grippingRopeNodeIndices[0], 0.0f, 10.0f);
    const float right = Clamp(snapshot_.grippingRopeNodeIndices[1], 0.0f, 10.0f);
    for (std::size_t i = 1; i < kRopeNodeCount; ++i) {
        const Vec3 p = snapshot_.ropeNodes[i];
        Vec3 velocity = Mul(Sub(p, previousRopeNodes_[i]), kRopeDamping);
        previousRopeNodes_[i] = p;
        const bool loaded = snapshot_.ropeHandsActive && snapshot_.equipmentStowed &&
            (std::abs(static_cast<float>(i) - left) < 1.0f ||
             std::abs(static_cast<float>(i) - right) < 1.0f);
        // Gripping hands damp the loaded line; the free tail and ordinary
        // player bumps keep their existing Verlet response.
        if(loaded)velocity=Mul(velocity,0.80f);
        const float gravity = kRopeGravity - (loaded ? kPlayerLoadAcceleration : 0.0f);
        snapshot_.ropeNodes[i] = Add(p, Add(velocity, {0.0f, gravity * kFixedDelta * kFixedDelta, 0.0f}));
        snapshot_.ropeNodes[i].y = std::max(snapshot_.ropeNodes[i].y, contactPlaneWorldY + kContactSkin);
    }

    float maximumPreSolveStretch = 0.0f;
    for (int pass = 0; pass < kConstraintIterations; ++pass) {
        snapshot_.ropeNodes[0] = kAnchor;
        for (std::size_t i = 0; i + 1 < kRopeNodeCount; ++i) {
            Vec3& a = snapshot_.ropeNodes[i];
            Vec3& b = snapshot_.ropeNodes[i + 1];
            const Vec3 delta = Sub(b, a);
            const float length = Length(delta);
            if (length <= 1.0e-6f) continue;
            const float constraintLength=(snapshot_.deploymentPhase==RopeDeploymentPhase::Unfurling)
                ? deploymentPaidOutLength_/static_cast<float>(kRopeNodeCount-1)
                : kRopeSegmentRestLength;
            const float extension = length - constraintLength;
            maximumPreSolveStretch = std::max(maximumPreSolveStretch, extension);
            // A paid-out rope preserves material length in slack as well as
            // tension. Otherwise the lower anchor collapses its loose tail
            // and can never satisfy the existing readiness length check.
            if (extension > 0.0f || snapshot_.deploymentPhase != RopeDeploymentPhase::Unfurling) {
                const Vec3 correction = Mul(delta, extension / length);
                if (i == 0) b = Sub(b, correction);
                else {
                    a = Add(a, Mul(correction, 0.5f));
                    b = Sub(b, Mul(correction, 0.5f));
                }
            }
            if (i > 0) a.y = std::max(a.y, contactPlaneWorldY + kContactSkin);
            b.y = std::max(b.y, contactPlaneWorldY + kContactSkin);
        }
        for(std::size_t i=1;i<kRopeNodeCount;++i) {
            ResolveRopeBlockoutContact(snapshot_.ropeNodes[i],kRopeCollisionRadius);
        }
        if(snapshot_.ropeHandsActive) {
            // Node contact alone cannot protect a hand sampled between nodes
            // on opposite sides of an edge. Project the actual interpolated
            // grip sphere and distribute that constraint to its two particles.
            // The anchor stays fixed; this also covers fractional upper grips.
            const float direction=snapshot_.phase==Phase::Descent||snapshot_.phase==Phase::Landing?1.0f:-1.0f;
            const std::array<float,4> gripContacts{{left,right,
                Clamp(left+direction*kAscentGripStep,.20f,10.0f),
                Clamp(right+direction*kAscentGripStep,.45f,10.0f)}};
            // Clear the next half-node travel before a moving hand reaches an
            // edge, so a thin slab cannot trigger a one-tick contact snap.
            for(const float index:gripContacts) {
                const auto lower=static_cast<std::size_t>(index);
                const auto upper=std::min(lower+1,kRopeNodeCount-1);
                const float t=index-static_cast<float>(lower);
                const float a=lower==0?0.0f:1.0f-t;
                const float b=upper==lower?0.0f:t;
                const float weight=a*a+b*b;
                if(weight<1.0e-6f)continue;
                const Vec3 before=Sample(snapshot_.ropeNodes,index);
                Vec3 after=before;
                constexpr float gripRadius=0.060f;
                ResolveRopeBlockoutContact(after,gripRadius,true);
                const Vec3 correction=Sub(after,before);
                for(const auto [node,w]:std::array<std::pair<std::size_t,float>,2>{{
                        {lower,a},{upper,b}}}) {
                    const Vec3 move=Mul(correction,w/weight);
                    snapshot_.ropeNodes[node]=Add(snapshot_.ropeNodes[node],move);
                    // A contact correction is not a new Verlet impulse.
                    previousRopeNodes_[node]=Add(previousRopeNodes_[node],move);
                }
            }
        }
        ResolveSafePlayerContact();
    }
    snapshot_.ropeNodes[0] = kAnchor;
    snapshot_.ropeTension = Clamp(maximumPreSolveStretch * 12000.0f, 0.0f, kMaximumRopeTension);
    float swing=0.0f;
    for(std::size_t i=1;i<kRopeNodeCount;++i) {
        const float dx=snapshot_.ropeNodes[i].x-kAnchor.x,dz=snapshot_.ropeNodes[i].z-kAnchor.z;
        swing=std::max(swing,std::hypot(dx,dz));
    }
    snapshot_.deploymentSwing=std::max(snapshot_.deploymentSwing,swing);

    PublishHands();
    const Vec3 gripMidpoint = Mul(Add(snapshot_.grippingHandTargets[0], snapshot_.grippingHandTargets[1]), 0.5f);
    if (snapshot_.phase == Phase::Ascent) {
        const float targetSupport = Clamp(kLowerSupportWorldY + gripMidpoint.y - gripReferenceWorld_.y,
            kLowerSupportWorldY, kUpperSupportWorldY);
        const Vec3 targetRoot = ClampRootToRopeFront({
            gripReferencePlayerPosition_.x + gripMidpoint.x - gripReferenceWorld_.x,
            targetSupport,
            gripReferencePlayerPosition_.z + gripMidpoint.z - gripReferenceWorld_.z});
        const Vec3 currentRoot{snapshot_.playerPosition.x, snapshot_.supportWorldY,
                               snapshot_.playerPosition.z};
        const bool finalClearGripPair =
            snapshot_.grippingRopeNodeIndices[0] <= 0.501f &&
            snapshot_.grippingRopeNodeIndices[1] <= 1.001f;
        const bool settledOnSolvedGripTarget =
            std::abs(currentRoot.y - targetSupport) <= 0.02f &&
            std::hypot(currentRoot.x - targetRoot.x,
                       currentRoot.z - targetRoot.z) <= 0.02f;
        if (finalClearGripPair && settledOnSolvedGripTarget)
            pullUpAlignmentActive_ = true;
        const Vec3 rimAlignmentTarget = ClampRootToRopeFront(
            {kRimLanding.x, targetSupport, kRimLanding.z});
        const Vec3 resolvedRoot = pullUpAlignmentActive_
            ? MoveTowardSupportFirst(currentRoot, rimAlignmentTarget, kMaximumGripRootStepMetres)
            : MoveTowardSupportFirst(currentRoot, targetRoot, kMaximumGripRootStepMetres);
        snapshot_.supportWorldY = resolvedRoot.y;
        snapshot_.playerPosition.x = resolvedRoot.x;
        snapshot_.playerPosition.z = resolvedRoot.z;
        const bool reachedRimStaging =
            std::hypot(resolvedRoot.x-kRimLanding.x,
                       resolvedRoot.z-kRimLanding.z) <= 0.001f &&
            std::abs(resolvedRoot.y-targetSupport)<=0.015f;
        if (pullUpAlignmentActive_ && finalClearGripPair && reachedRimStaging) {
            // The reachable last grips stop below the rim. The separately
            // authored pull-up begins here under rope hand load; root alignment
            // has taken at most one bounded step per tick while both grips held.
            snapshot_.phase=Phase::PullUp;
            pullUpAlignmentActive_ = false;
            pullUpStage_=PullUpStage::RegripLeft;
            motorTicks_=0u;
        }
        snapshot_.playerPosition.y = snapshot_.supportWorldY;
        // The ascent root is resolved from the rope after its constraint pass.
        // Reapply capsule contact at that newly reached root pose and refresh
        // hand samples from the corrected particles before publishing.
        ResolveSafePlayerContact();
        PublishHands();
    } else if (snapshot_.phase == Phase::PullUp ||
               (snapshot_.phase == Phase::Descent && descentStage_ != DescentStage::Rope)) {
        snapshot_.playerPosition.y = snapshot_.supportWorldY;
    } else if(snapshot_.phase==Phase::Landing&&!snapshot_.ropeHandsActive) {
        snapshot_.playerPosition=kLowerLanding;
        snapshot_.supportWorldY=kLowerSupportWorldY;
    } else if ((snapshot_.phase == Phase::Descent && descentStage_ == DescentStage::Rope) ||
               snapshot_.phase == Phase::Landing) {
        const float targetSupport = Clamp(kUpperSupportWorldY + gripMidpoint.y - gripReferenceWorld_.y,
            kLowerSupportWorldY, kUpperSupportWorldY);
        const float descentProgress = Clamp((kUpperSupportWorldY - targetSupport) /
            (kUpperSupportWorldY - kLowerSupportWorldY), 0.0f, 1.0f);
        Vec3 targetRoot = {
            gripReferencePlayerPosition_.x + gripMidpoint.x - gripReferenceWorld_.x,
            targetSupport,
            gripReferencePlayerPosition_.z + gripMidpoint.z - gripReferenceWorld_.z +
                (kLowerLanding.z - kRimLanding.z) * descentProgress};
        targetRoot.x=std::min(targetRoot.x,kAnchor.x-kAnchorBodyStandOffMetres);
        const Vec3 currentRoot{snapshot_.playerPosition.x, snapshot_.supportWorldY,
                               snapshot_.playerPosition.z};
        const Vec3 resolvedRoot = MoveTowardSupportFirst(currentRoot, targetRoot,
                                                         kMaximumGripRootStepMetres);
        snapshot_.supportWorldY = resolvedRoot.y;
        snapshot_.playerPosition.x = resolvedRoot.x;
        snapshot_.playerPosition.z = resolvedRoot.z;
        snapshot_.playerPosition.y = snapshot_.supportWorldY;
        if (snapshot_.phase == Phase::Descent &&
            snapshot_.supportWorldY <= kLowerSupportWorldY + 0.20f) {
            snapshot_.phase = Phase::Landing;
            snapshot_.sawLanding = true;
        }
        if (snapshot_.phase == Phase::Landing &&
            snapshot_.supportWorldY <= kLowerSupportWorldY + 0.005f) {
            snapshot_.supportWorldY = kLowerSupportWorldY;
            snapshot_.playerPosition = kLowerLanding;
            snapshot_.ropeHandsActive = false;
        }
        ResolveSafePlayerContact();
        PublishHands();
    }
}

void RescueTraversal::UpdateBodyPresentation()
{
    if (!snapshot_.equipmentStowed) {
        snapshot_.plantedPelvisDropMetres = 0.0f;
        return;
    }
    const float dx = kAnchor.x - snapshot_.playerPosition.x;
    const float dz = kAnchor.z - snapshot_.playerPosition.z;
    constexpr float pi = 3.14159265358979323846f;
    const float targetYaw = std::atan2(dx, -dz);
    const float delta = std::remainder(targetYaw - snapshot_.bodyYawRadians, 2.0f*pi);
    snapshot_.bodyYawRadians += Clamp(delta, -0.04f, 0.04f);
    const float lowGripY = std::min(snapshot_.grippingHandTargets[0].y,
                                    snapshot_.grippingHandTargets[1].y);
    const bool preparingGrip=snapshot_.phase==Phase::Descent&&descentStage_==DescentStage::Crouch;
    const float targetDrop = (snapshot_.ropeHandsActive||preparingGrip)
        ? Clamp(snapshot_.supportWorldY + 1.45f - lowGripY - 0.40f, 0.0f, 0.72f)
        : 0.0f;
    snapshot_.plantedPelvisDropMetres += Clamp(
        targetDrop-snapshot_.plantedPelvisDropMetres,-0.015f,0.015f);
    if(snapshot_.phase==Phase::Landing&&!snapshot_.ropeHandsActive&&
       snapshot_.plantedPelvisDropMetres<=0.001f) {
        snapshot_.plantedPelvisDropMetres=0.0f;
        snapshot_.phase=Phase::LowerSafe;
        snapshot_.exteriorSide=false;
        snapshot_.equipmentStowed=false;
    }
}

bool RescueTraversal::Step(const Input& input)
{
    if (!Finite(input)) return false;
    if (input.paused) return true;
    snapshot_.ropeBumpedPlayer=false;

    if (Safe(snapshot_.phase)) {
        snapshot_.playerPosition = input.playerPosition;
        snapshot_.supportWorldY = input.supportWorldY;
        snapshot_.playerPosition.y = input.supportWorldY;
    }
    if(snapshot_.deploymentCount&&snapshot_.deploymentPhase!=RopeDeploymentPhase::Ready)
        AdvanceDeployment(input);
    if (!snapshot_.ropeDeployed) {
        RefreshPrompt();
        return true;
    }
    if (Moving(snapshot_.phase) && input.contactBlocked) {
        if(snapshot_.phase==Phase::ApproachAscent||snapshot_.phase==Phase::ApproachDescent) {
            AbortApproach();
            if (snapshot_.generation < std::numeric_limits<std::uint64_t>::max()) ++snapshot_.generation;
            snapshot_.promptReason=PromptReason::Blocked;
            return true;
        }
        RollbackToSafeEndpoint(snapshot_.phase == Phase::Descent || snapshot_.phase == Phase::Landing);
        if (snapshot_.generation < std::numeric_limits<std::uint64_t>::max()) ++snapshot_.generation;
        promptBlocked_ = true;
        snapshot_.promptReason = PromptReason::Blocked;
        return true;
    }
    if (Moving(snapshot_.phase)) AdvanceMotion();
    PublishHands();
    ResolveRope(input.contactPlaneWorldY);
    UpdateBodyPresentation();
    // Publish one coherent final manifold after the bounded stance/yaw change;
    // consumers must not receive rope contact against last tick's torso.
    ResolveSafePlayerContact();
    PublishHands();
    UpdateDeploymentReadiness(input.contactPlaneWorldY);
    RefreshPrompt();
    return true;
}

} // namespace horde::gameplay::traversal
