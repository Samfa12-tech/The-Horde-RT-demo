#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace horde::gameplay::traversal {

struct Vec3 {
    float x{};
    float y{};
    float z{};
    friend constexpr bool operator==(Vec3, Vec3) = default;
};

inline constexpr float kLowerSupportWorldY = -0.95f;
inline constexpr float kUpperSupportWorldY = 2.05f;
inline constexpr float kCopingTopWorldY = 2.23f;
inline constexpr float kCopingClearanceSupportWorldY = 2.28f;
inline constexpr float kCopingZMin = -13.76f;
inline constexpr float kCopingZMax = -13.58f;
// The anchor sits inside x[-34.9,-32.5], z[-16.6,-13.8]; the exterior
// landing is one metre beyond the aperture. The approach rises to 2.28 over
// the 2.23 coping, then settles at 2.05 on the exterior support.
// Keep the fixed upper anchor 0.3 m in front of the rope-facing body's safe
// endpoint, so its loaded span and anchor fitting do not run through the
// player's torso. The lower contact endpoint remains at z=-15.2.
inline constexpr Vec3 kAnchor{-33.7f, 3.8f, -15.5f};
inline constexpr Vec3 kLowerLanding{-33.7f, kLowerSupportWorldY, -15.2f};
inline constexpr Vec3 kRimLanding{-33.7f, kUpperSupportWorldY, -15.2f};
inline constexpr Vec3 kExteriorLanding{-33.7f, kUpperSupportWorldY, -12.8f};
// The actual stone apron begins at z=-14.95. Keep the body column at -15.2
// open until the ascent rope handoff reaches this first supported center.
inline constexpr Vec3 kApronHandoff{-33.7f, kUpperSupportWorldY, -14.94f};
inline constexpr float kPullUpLeftGripNodeIndex = 0.40f;
inline constexpr float kPullUpRightGripNodeIndex = 0.70f;
inline constexpr std::size_t kRopeNodeCount = 12;
inline constexpr float kRopeLength = 4.8f;
inline constexpr float kRopeSegmentRestLength = kRopeLength / (kRopeNodeCount - 1);
inline constexpr float kMaximumRopeTension = 400.0f;

enum class Phase : std::uint8_t {
    LowerSafe,
    AwaitingAscentReadiness,
    Ascent,
    PullUp,
    UpperSafe,
    AwaitingDescentReadiness,
    Descent,
    Landing,
};

enum class PromptReason : std::uint8_t {
    ClimbRope,
    ReturnDown,
    WaitingForReadiness,
    Busy,
    Blocked,
};

struct Input {
    Vec3 playerPosition{kLowerLanding};
    float supportWorldY{kLowerSupportWorldY};
    float contactPlaneWorldY{-100.0f};
    bool paused{};
    bool contactBlocked{};
};

struct RescueTraversalSnapshot {
    Phase phase{Phase::LowerSafe};
    PromptReason promptReason{PromptReason::ClimbRope};
    std::uint64_t generation{1};
    Vec3 playerPosition{kLowerLanding};
    float supportWorldY{kLowerSupportWorldY};
    float bodyYawRadians{};
    std::array<Vec3, 2> grippingHandTargets{};
    std::array<float, 2> grippingRopeNodeIndices{8.0f, 9.0f};
    std::array<Vec3, kRopeNodeCount> ropeNodes{};
    float ropeTension{};
    std::uint32_t claimCount{};
    std::uint32_t deploymentCount{};
    bool equipmentStowed{};
    bool ropeHandsActive{};
    bool lanternClaimed{};
    bool ropeDeployed{};
    bool exteriorSide{};
    bool firstAscentCompleted{};
    bool sawAscent{};
    bool sawPullUp{};
    bool sawDescent{};
    bool sawLanding{};
};

// Pure fixed-step gameplay state. The caller publishes this output through
// the shared simulation snapshot; platform and renderer state remain external.
class RescueTraversal final {
public:
    RescueTraversal();

    // Advances exactly one 60 Hz tick. Nonfinite input is rejected unchanged;
    // pause is an accepted zero-motion tick.
    bool Step(const Input& input);
    // These are called at the actual gameplay claim site, before interaction.
    bool NotifyLanternClaimed();
    bool DeployOwned();
    // Called only for a consumed interaction edge at a safe endpoint.
    bool Request();
    // A request commits only when both ownership and rendering are ready and
    // the readiness generation still matches the current request.
    bool TryBegin(std::uint64_t generation, bool claimOwned, bool renderReady);
    bool PublishReadiness(std::uint64_t generation, bool ready);
    // Full route reset clears traversal ownership and completed ascent state.
    void Reset(Vec3 safePosition = kLowerLanding,
               float safeSupportWorldY = kLowerSupportWorldY);
    // Resource reconstruction returns to the active direction's safe endpoint,
    // invalidates callbacks, and retains claim/deployment history.
    void RecoverForReconstruction();

    const RescueTraversalSnapshot& Snapshot() const { return snapshot_; }
    bool IsActive() const;
    bool CanInteract() const;

private:
    enum class PullUpStage : std::uint8_t { RegripLeft, RegripRight, Handoff, Lift, Cross, Settle };
    enum class DescentStage : std::uint8_t { Lift, Cross, Settle, Rope };

    void BeginRequest(bool ascent);
    void AbortRequest();
    void RollbackToSafeEndpoint(bool upper);
    void ResolveRope(float contactPlaneWorldY);
    void PublishHands();
    void RefreshPrompt();
    void AdvanceMotion();
    void AdvanceAlternatingGrip(bool towardAnchor);

    RescueTraversalSnapshot snapshot_{};
    std::array<Vec3, kRopeNodeCount> previousRopeNodes_{};
    Vec3 gripReferenceWorld_{};
    Vec3 gripReferencePlayerPosition_{kLowerLanding};
    PullUpStage pullUpStage_{PullUpStage::Lift};
    DescentStage descentStage_{DescentStage::Lift};
    std::uint16_t motorTicks_{};
    std::uint16_t gripAdvanceIntervalTicks_{};
    float gripAdvanceStartIndex_{};
    float gripAdvanceTargetIndex_{};
    bool nextGripIsLeft_{true};
    bool readinessReceived_{};
    bool readinessReady_{};
    bool promptBlocked_{};
};

} // namespace horde::gameplay::traversal
