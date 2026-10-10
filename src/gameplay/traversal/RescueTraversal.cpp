#include "gameplay/traversal/RescueTraversal.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace horde::gameplay::traversal {
namespace {

constexpr float kFixedDelta = 1.0f / 60.0f;
constexpr float kAscentGripStep = 0.5f;
constexpr float kGripIntervalSeconds = 0.12f;
constexpr float kPullUpNominalHandStepMetres = 0.035f;
constexpr float kMaximumGripRootStepMetres = 0.025f;
constexpr float kPullUpSpeed = 0.85f;
constexpr float kDescentSpeed = 0.68f;
constexpr float kRopeGravity = -9.81f;
constexpr float kRopeDamping = 0.985f;
constexpr float kContactSkin = 0.025f;
constexpr float kRopeVerticalSpan = kAnchor.y - (kLowerSupportWorldY + kContactSkin);
constexpr float kPlayerLoadAcceleration = 9.81f;
constexpr int kConstraintIterations = 12;

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

bool Safe(Phase phase) { return phase == Phase::LowerSafe || phase == Phase::UpperSafe; }
bool Awaiting(Phase phase)
{
    return phase == Phase::AwaitingAscentReadiness || phase == Phase::AwaitingDescentReadiness;
}
bool Moving(Phase phase)
{
    return phase == Phase::Ascent || phase == Phase::PullUp || phase == Phase::Descent || phase == Phase::Landing;
}

Vec3 Sample(const std::array<Vec3, kRopeNodeCount>& nodes, float index)
{
    index = Clamp(index, 0.0f, static_cast<float>(kRopeNodeCount - 1));
    const auto lower = static_cast<std::size_t>(index);
    const auto upper = std::min(lower + 1, kRopeNodeCount - 1);
    const float t = index - static_cast<float>(lower);
    return Add(nodes[lower], Mul(Sub(nodes[upper], nodes[lower]), t));
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
    if (!snapshot_.ropeDeployed) {
        snapshot_.promptReason = PromptReason::Blocked;
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
    if (!snapshot_.ropeDeployed) return false;
    const Vec3 expected = snapshot_.phase == Phase::LowerSafe ? kLowerLanding : kExteriorLanding;
    const float expectedSupport = snapshot_.phase == Phase::LowerSafe ? kLowerSupportWorldY : kUpperSupportWorldY;
    const float dx = snapshot_.playerPosition.x - expected.x;
    const float dz = snapshot_.playerPosition.z - expected.z;
    const bool nearEndpoint = std::sqrt(dx * dx + dz * dz) <= 0.10f &&
        std::abs(snapshot_.supportWorldY - expectedSupport) <= 0.10f;
    return nearEndpoint && (snapshot_.phase == Phase::LowerSafe ||
        (snapshot_.phase == Phase::UpperSafe && snapshot_.exteriorSide));
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
    if (!snapshot_.lanternClaimed || snapshot_.ropeDeployed) return false;
    snapshot_.ropeDeployed = true;
    if (snapshot_.deploymentCount < std::numeric_limits<std::uint32_t>::max()) ++snapshot_.deploymentCount;
    snapshot_.grippingRopeNodeIndices = {8.0f, 9.0f};
    for (std::size_t i = 0; i < kRopeNodeCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kRopeNodeCount - 1);
        snapshot_.ropeNodes[i] = Add(kAnchor, Mul(Vec3{0.0f, -kRopeVerticalSpan, 0.0f}, t));
        previousRopeNodes_[i] = snapshot_.ropeNodes[i];
    }
    gripReferenceWorld_ = Mul(Add(Sample(snapshot_.ropeNodes, 8.0f), Sample(snapshot_.ropeNodes, 9.0f)), 0.5f);
    PublishHands();
    RefreshPrompt();
    return true;
}

void RescueTraversal::BeginRequest(bool ascent)
{
    if (snapshot_.generation < std::numeric_limits<std::uint64_t>::max()) ++snapshot_.generation;
    snapshot_.phase = ascent ? Phase::AwaitingAscentReadiness : Phase::AwaitingDescentReadiness;
    snapshot_.equipmentStowed = true;
    snapshot_.ropeHandsActive = false;
    snapshot_.ropeTension = 0.0f;
    snapshot_.grippingRopeNodeIndices = ascent ? std::array<float, 2>{8.0f, 9.0f}
                                                : std::array<float, 2>{0.75f, 1.0f};
    readinessReceived_ = false;
    readinessReady_ = false;
    motorTicks_ = 0;
    nextGripIsLeft_ = true;
    RefreshPrompt();
}

bool RescueTraversal::Request()
{
    if (!CanInteract()) return false;
    promptBlocked_ = false;
    if (snapshot_.phase == Phase::LowerSafe) {
        gripReferencePlayerPosition_ = snapshot_.playerPosition;
        // Descent can leave the flexible rope with a different sag/load shape.
        // Rebase the lower ascent motor on the currently solved starting grips
        // each time the player starts a new route cycle.
        gripReferenceWorld_ = Mul(Add(Sample(snapshot_.ropeNodes, 8.0f),
                                      Sample(snapshot_.ropeNodes, 9.0f)), 0.5f);
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
    snapshot_.phase = ascent ? Phase::Ascent : Phase::Descent;
    pullUpStage_ = PullUpStage::Handoff;
    descentStage_ = DescentStage::Lift;
    snapshot_.equipmentStowed = true;
    snapshot_.ropeHandsActive = ascent;
    snapshot_.sawAscent = ascent || snapshot_.sawAscent;
    snapshot_.sawDescent = !ascent || snapshot_.sawDescent;
    readinessReceived_ = false;
    readinessReady_ = false;
    motorTicks_ = 0;
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
    nextGripIsLeft_ = true;
    PublishHands();
    RefreshPrompt();
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
    const bool firstAscent = snapshot_.firstAscentCompleted;
    Reset(descentSide ? kExteriorLanding : kLowerLanding,
          descentSide ? kUpperSupportWorldY : kLowerSupportWorldY);
    snapshot_.claimCount = claims;
    snapshot_.deploymentCount = deployments;
    snapshot_.lanternClaimed = claimed;
    snapshot_.ropeDeployed = deployed;
    snapshot_.firstAscentCompleted = firstAscent;
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
    promptBlocked_ = false;
    PublishHands();
    RefreshPrompt();
}

void RescueTraversal::PublishHands()
{
    snapshot_.grippingHandTargets[0] = Sample(snapshot_.ropeNodes, snapshot_.grippingRopeNodeIndices[0]);
    snapshot_.grippingHandTargets[1] = Sample(snapshot_.ropeNodes, snapshot_.grippingRopeNodeIndices[1]);
}

void RescueTraversal::AdvanceAlternatingGrip(const bool towardAnchor)
{
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
    if (snapshot_.phase == Phase::Ascent) {
        AdvanceAlternatingGrip(true);
        return;
    }
    if (snapshot_.phase == Phase::PullUp) {
        if (pullUpStage_ == PullUpStage::RegripLeft ||
            pullUpStage_ == PullUpStage::RegripRight) {
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
                                          : PullUpStage::Handoff;
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
            snapshot_.supportWorldY = std::min(kCopingClearanceSupportWorldY,
                snapshot_.supportWorldY + kPullUpSpeed * kFixedDelta);
            if (snapshot_.supportWorldY >= kCopingClearanceSupportWorldY - 1.0e-5f)
                pullUpStage_ = PullUpStage::Cross;
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
                snapshot_.ropeHandsActive = true;
                descentStage_ = DescentStage::Rope;
                gripReferencePlayerPosition_ = snapshot_.playerPosition;
                gripReferenceWorld_ = Mul(Add(Sample(snapshot_.ropeNodes, 0.75f),
                                              Sample(snapshot_.ropeNodes, 1.0f)), 0.5f);
                motorTicks_ = 0u;
            }
        }
        snapshot_.playerPosition.y = snapshot_.supportWorldY;
        if (descentStage_ == DescentStage::Rope) AdvanceAlternatingGrip(false);
        return;
    }
    if (snapshot_.phase == Phase::Landing) {
        AdvanceAlternatingGrip(false);
    }
}

void RescueTraversal::ResolveRope(float contactPlaneWorldY)
{
    snapshot_.ropeNodes[0] = kAnchor;
    const float left = Clamp(snapshot_.grippingRopeNodeIndices[0], 1.0f, 10.0f);
    const float right = Clamp(snapshot_.grippingRopeNodeIndices[1], 1.0f, 10.0f);
    for (std::size_t i = 1; i < kRopeNodeCount; ++i) {
        const Vec3 p = snapshot_.ropeNodes[i];
        Vec3 velocity = Mul(Sub(p, previousRopeNodes_[i]), kRopeDamping);
        previousRopeNodes_[i] = p;
        const bool loaded = snapshot_.ropeHandsActive && snapshot_.equipmentStowed &&
            (std::abs(static_cast<float>(i) - left) < 1.0f ||
             std::abs(static_cast<float>(i) - right) < 1.0f);
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
            const float extension = length - kRopeSegmentRestLength;
            maximumPreSolveStretch = std::max(maximumPreSolveStretch, extension);
            if (extension <= 0.0f) continue;
            const Vec3 correction = Mul(delta, extension / length);
            if (i == 0) b = Sub(b, correction);
            else {
                a = Add(a, Mul(correction, 0.5f));
                b = Sub(b, Mul(correction, 0.5f));
            }
            if (i > 0) a.y = std::max(a.y, contactPlaneWorldY + kContactSkin);
            b.y = std::max(b.y, contactPlaneWorldY + kContactSkin);
        }
    }
    snapshot_.ropeNodes[0] = kAnchor;
    snapshot_.ropeTension = Clamp(maximumPreSolveStretch * 12000.0f, 0.0f, kMaximumRopeTension);

    PublishHands();
    const Vec3 gripMidpoint = Mul(Add(snapshot_.grippingHandTargets[0], snapshot_.grippingHandTargets[1]), 0.5f);
    if (snapshot_.phase == Phase::Ascent) {
        const float targetSupport = Clamp(kLowerSupportWorldY + gripMidpoint.y - gripReferenceWorld_.y,
            kLowerSupportWorldY, kUpperSupportWorldY);
        const Vec3 targetRoot{
            gripReferencePlayerPosition_.x + gripMidpoint.x - gripReferenceWorld_.x,
            targetSupport,
            gripReferencePlayerPosition_.z + gripMidpoint.z - gripReferenceWorld_.z};
        const Vec3 currentRoot{snapshot_.playerPosition.x, snapshot_.supportWorldY,
                               snapshot_.playerPosition.z};
        const Vec3 resolvedRoot = MoveToward(currentRoot, targetRoot, kMaximumGripRootStepMetres);
        snapshot_.supportWorldY = resolvedRoot.y;
        snapshot_.playerPosition.x = resolvedRoot.x;
        snapshot_.playerPosition.z = resolvedRoot.z;
        if (snapshot_.supportWorldY >= kUpperSupportWorldY - 0.005f) {
            snapshot_.supportWorldY = kUpperSupportWorldY;
            snapshot_.playerPosition = kRimLanding;
            snapshot_.phase = Phase::PullUp;
            pullUpStage_ = PullUpStage::RegripLeft;
            // The regrip is a new motor interval, not the remainder of the
            // alternating ascent hand's interpolation.
            motorTicks_ = 0u;
        }
        snapshot_.playerPosition.y = snapshot_.supportWorldY;
    } else if (snapshot_.phase == Phase::PullUp ||
               (snapshot_.phase == Phase::Descent && descentStage_ != DescentStage::Rope)) {
        snapshot_.playerPosition.y = snapshot_.supportWorldY;
    } else if ((snapshot_.phase == Phase::Descent && descentStage_ == DescentStage::Rope) ||
               snapshot_.phase == Phase::Landing) {
        const float targetSupport = Clamp(kUpperSupportWorldY + gripMidpoint.y - gripReferenceWorld_.y,
            kLowerSupportWorldY, kUpperSupportWorldY);
        const float descentProgress = Clamp((kUpperSupportWorldY - targetSupport) /
            (kUpperSupportWorldY - kLowerSupportWorldY), 0.0f, 1.0f);
        const Vec3 targetRoot{
            gripReferencePlayerPosition_.x + gripMidpoint.x - gripReferenceWorld_.x,
            targetSupport,
            gripReferencePlayerPosition_.z + gripMidpoint.z - gripReferenceWorld_.z +
                (kLowerLanding.z - kApronHandoff.z) * descentProgress};
        const Vec3 currentRoot{snapshot_.playerPosition.x, snapshot_.supportWorldY,
                               snapshot_.playerPosition.z};
        const Vec3 resolvedRoot = MoveToward(currentRoot, targetRoot, kMaximumGripRootStepMetres);
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
            snapshot_.phase = Phase::LowerSafe;
            snapshot_.exteriorSide = false;
            snapshot_.equipmentStowed = false;
            snapshot_.ropeHandsActive = false;
        }
    }
}

bool RescueTraversal::Step(const Input& input)
{
    if (!Finite(input)) return false;
    if (input.paused) return true;

    if (Safe(snapshot_.phase)) {
        snapshot_.playerPosition = input.playerPosition;
        snapshot_.supportWorldY = input.supportWorldY;
        snapshot_.playerPosition.y = input.supportWorldY;
    }
    if (!snapshot_.ropeDeployed) {
        RefreshPrompt();
        return true;
    }
    if (Moving(snapshot_.phase) && input.contactBlocked) {
        RollbackToSafeEndpoint(snapshot_.phase == Phase::Descent || snapshot_.phase == Phase::Landing);
        if (snapshot_.generation < std::numeric_limits<std::uint64_t>::max()) ++snapshot_.generation;
        promptBlocked_ = true;
        snapshot_.promptReason = PromptReason::Blocked;
        return true;
    }
    if (Moving(snapshot_.phase)) AdvanceMotion();
    PublishHands();
    ResolveRope(input.contactPlaneWorldY);
    RefreshPrompt();
    return true;
}

} // namespace horde::gameplay::traversal
