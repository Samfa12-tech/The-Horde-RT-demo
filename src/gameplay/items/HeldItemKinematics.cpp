#include "gameplay/items/HeldItemKinematics.h"

#include "gameplay/CorridorCollision.h"
#include "scene/ShowcaseOverheadGeometry.h"

#include <algorithm>
#include <cmath>

namespace horde::gameplay::items
{

namespace
{

constexpr float kSwordRestInwardRadians = 0.20f;
constexpr float kSwordRestForwardRadians = 0.14f;
// The authored left palm is broadside at zero roll. A quarter-turn presents
// the grip end-on and collapses its four fingers into the reported fist blob.
constexpr float kLeftGripRollRadians = 0.0f;

// The player-held Rag prop has its own authored sockets. Keeper flank torches
// and the reward lantern continue to use the original torch contracts.
HeldItemTransform PlayerRagTorchGripSocketTransform()
{
    HeldItemTransform result = IdentityHeldItemTransform();
    result[12] = 0.010711723f;
    result[13] = 0.24f;
    result[14] = -0.002040245f;
    return result;
}

HeldItemTransform PlayerRagTorchFlameSocketTransform()
{
    HeldItemTransform result = IdentityHeldItemTransform();
    result[12] = -0.00316136f;
    result[13] = 0.805f;
    result[14] = -0.00462115f;
    return result;
}

HeldItemTransform PlayerRagTorchLightSocketTransform()
{
    HeldItemTransform result = IdentityHeldItemTransform();
    result[12] = -0.005092165f;
    result[13] = 0.78f;
    result[14] = 0.021113701f;
    return result;
}

float DotColumn(const HeldItemTransform& transform,
                const std::size_t leftColumn,
                const std::size_t rightColumn)
{
    return transform[leftColumn * 4u] * transform[rightColumn * 4u] +
           transform[leftColumn * 4u + 1u] * transform[rightColumn * 4u + 1u] +
           transform[leftColumn * 4u + 2u] * transform[rightColumn * 4u + 2u];
}

HeldItemTransform InverseRigidTransform(const HeldItemTransform& transform)
{
    HeldItemTransform result = IdentityHeldItemTransform();
    for (std::size_t column = 0u; column < 3u; ++column)
    {
        for (std::size_t row = 0u; row < 3u; ++row)
        {
            result[column * 4u + row] = transform[row * 4u + column];
        }
    }
    for (std::size_t row = 0u; row < 3u; ++row)
    {
        result[12u + row] = -(result[row] * transform[12] +
                              result[4u + row] * transform[13] +
                              result[8u + row] * transform[14]);
    }
    return result;
}

using Vec3 = std::array<float, 3u>;

Vec3 Add(const Vec3& left, const Vec3& right)
{
    return {{left[0] + right[0], left[1] + right[1], left[2] + right[2]}};
}

Vec3 Scale(const Vec3& value, const float scale)
{
    return {{value[0] * scale, value[1] * scale, value[2] * scale}};
}

float Dot(const Vec3& left, const Vec3& right)
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

Vec3 Lerp(const Vec3& from, const Vec3& to, const float amount)
{
    return Add(from, Scale(Add(to, Scale(from, -1.0f)), amount));
}

Vec3 Cross(const Vec3& left, const Vec3& right)
{
    return {{left[1] * right[2] - left[2] * right[1],
             left[2] * right[0] - left[0] * right[2],
             left[0] * right[1] - left[1] * right[0]}};
}

Vec3 Normalize(const Vec3& value)
{
    const float length = std::sqrt(value[0] * value[0] + value[1] * value[1] +
                                   value[2] * value[2]);
    return length > 0.000001f ? Scale(value, 1.0f / length)
                              : Vec3{{0.0f, 1.0f, 0.0f}};
}

HeldItemTransform WorldFromAxes(const Vec3& x,
                                const Vec3& y,
                                const Vec3& z,
                                const Vec3& translation)
{
    return {{x[0], x[1], x[2], 0.0f,
             y[0], y[1], y[2], 0.0f,
             z[0], z[1], z[2], 0.0f,
             translation[0], translation[1], translation[2], 1.0f}};
}

Vec3 TranslationOf(const HeldItemTransform& transform)
{
    return {{transform[12], transform[13], transform[14]}};
}

Vec3 ColumnOf(const HeldItemTransform& transform, const std::size_t column)
{
    return {{transform[column * 4u], transform[column * 4u + 1u],
             transform[column * 4u + 2u]}};
}

float MapContinuousCarryDepth(const float forwardClearance,
                              const float openDepth)
{
    constexpr float minimumClearance = 0.30f;
    constexpr float maximumClearance = 2.70f;
    const float normalized = std::clamp(
        (forwardClearance - minimumClearance) /
            (maximumClearance - minimumClearance),
        0.0f, 1.0f);
    return minimumClearance + (openDepth - minimumClearance) * normalized;
}

float DistanceToOverheadFootprint(const horde::scene::OverheadVolume& volume,
                                 const float x, const float z)
{
    bool positive = false;
    bool negative = false;
    float minimumSquared = 1.0e9f;
    for (std::size_t edge = 0u; edge < volume.footprint.size(); ++edge)
    {
        const auto& start = volume.footprint[edge];
        const auto& end = volume.footprint[(edge + 1u) % volume.footprint.size()];
        const float dx = end[0] - start[0];
        const float dz = end[1] - start[1];
        const float side = dx * (z - start[1]) - dz * (x - start[0]);
        positive |= side > 0.0f;
        negative |= side < 0.0f;
        const float lengthSquared = dx * dx + dz * dz;
        const float fraction = lengthSquared > 0.0f
            ? std::clamp(((x - start[0]) * dx + (z - start[1]) * dz) /
                             lengthSquared, 0.0f, 1.0f)
            : 0.0f;
        const float separationX = x - start[0] - fraction * dx;
        const float separationZ = z - start[1] - fraction * dz;
        minimumSquared = std::min(minimumSquared,
            separationX * separationX + separationZ * separationZ);
    }
    return positive && negative ? std::sqrt(minimumSquared) : 0.0f;
}

float TorchOverheadLowering(const Vec3& gripWorld, const Vec3& viewUp,
                           const Vec3& viewForward)
{
    // A world-down response leaves the complete horizontal envelope unchanged.
    // Include the long tilted torch as well as its cage and animated fire;
    // testing only the camera or flame pivot misses an approaching lintel.
    const float radius = kHeldTorchEnvelopeRadius +
        kPlayerRagTorchEnvelopeTopFromGrip * std::hypot(viewUp[0], viewUp[2]);
    const float highestY = gripWorld[1] +
        kPlayerRagTorchEnvelopeTopFromGrip * viewUp[1] +
        kHeldTorchEnvelopeRadius * std::abs(viewForward[1]);
    constexpr float anticipationDistance = 0.65f;
    float lowering = 0.0f;
    const auto include = [&](const horde::scene::OverheadVolume& volume) {
        const float required = std::max(0.0f,
            highestY + kHeldTorchOverheadGap -
                horde::scene::MinimumOverheadBottomY(volume, gripWorld[0], gripWorld[2], radius));
        if (required == 0.0f) return;
        const float distance = std::max(0.0f,
            DistanceToOverheadFootprint(volume, gripWorld[0], gripWorld[2]) - radius);
        const float response = std::clamp(1.0f - distance / anticipationDistance,
                                          0.0f, 1.0f);
        const float smoothResponse = response * response * (3.0f - 2.0f * response);
        lowering = std::max(lowering, required * smoothResponse);
    };
    for (const auto& volume : horde::scene::kShowcaseLowOverheadVolumes) include(volume);
    for (const auto& volume : horde::scene::kShowcaseCeilingPatches) include(volume);
    for (const auto& volume : horde::scene::kShowcaseSkylightGrid) include(volume);
    for (const auto& volume : horde::scene::kShowcaseImportedOverheadVolumes) include(volume);
    include(horde::scene::kShowcaseCollapseRoofSeam);
    return lowering;
}

float SwordOverheadLowering(const Vec3& gripWorld,
                            const Vec3& bladeAxisWorld,
                            const Vec3& edgeAxisWorld,
                            const Vec3& flatAxisWorld)
{
    // Bounds are measured from the imported production sword GLB after its
    // exact Grip socket is removed: blade-long +Y [-0.135, 0.915], sharpened
    // edge +X +/-0.112, and broad-flat +Z +/-0.025 metres. Sweep that actual
    // oriented envelope along the posed blade instead of testing the camera or
    // the right-hand pivot alone. The half-step vertical pad makes the fixed
    // samples conservative for every roof-plane crossing between them.
    constexpr float minimumBlade = -0.135f;
    constexpr float maximumBlade = 0.91526f;
    constexpr int sampleIntervals = 42;
    constexpr float edgeExtent = 0.112f;
    constexpr float flatExtent = 0.025f;
    constexpr float bladeLength = maximumBlade - minimumBlade;
    constexpr float samplePad = bladeLength / (2.0f * sampleIntervals);
    constexpr float anticipationDistance = 0.65f;
    const float radius = std::hypot(edgeExtent, flatExtent);
    const float verticalCrossSection = edgeExtent * std::abs(edgeAxisWorld[1]) +
                                       flatExtent * std::abs(flatAxisWorld[1]);
    struct SwordClearanceSample
    {
        Vec3 center{};
        float highestY = 0.0f;
    };
    std::array<SwordClearanceSample, sampleIntervals + 1u> samples{};
    for (int sample = 0; sample <= sampleIntervals; ++sample)
    {
        const float bladeOffset = minimumBlade + bladeLength *
            (static_cast<float>(sample) / static_cast<float>(sampleIntervals));
        auto& point = samples[static_cast<std::size_t>(sample)];
        point.center = Add(gripWorld, Scale(bladeAxisWorld, bladeOffset));
        point.highestY = point.center[1] + verticalCrossSection +
                         samplePad * std::abs(bladeAxisWorld[1]);
    }

    // Most held poses are in open rooms. Reject whole roof volumes before the
    // per-sample underside work whenever the grip-to-footprint distance proves
    // the complete sword sweep lies beyond both its radial extent and smooth
    // anticipation distance. Distance to a convex footprint is 1-Lipschitz,
    // so this bound cannot discard a contributing blade sample.
    const float horizontalReach = std::max(std::abs(minimumBlade), maximumBlade) *
        std::hypot(bladeAxisWorld[0], bladeAxisWorld[2]) + radius;
    float lowering = 0.0f;
    const auto includeVolumes = [&](const auto& volumes) {
        for (const auto& volume : volumes)
        {
            if (DistanceToOverheadFootprint(volume, gripWorld[0], gripWorld[2]) >
                horizontalReach + anticipationDistance)
                continue;
            for (const auto& point : samples)
            {
                const float required = std::max(0.0f,
                    point.highestY + kHeldTorchOverheadGap -
                        horde::scene::MinimumOverheadBottomY(
                            volume, point.center[0], point.center[2], radius));
                if (required == 0.0f) continue;
                const float distance = std::max(0.0f,
                    DistanceToOverheadFootprint(
                        volume, point.center[0], point.center[2]) - radius);
                const float response = std::clamp(
                    1.0f - distance / anticipationDistance, 0.0f, 1.0f);
                const float smoothResponse = response * response * (3.0f - 2.0f * response);
                lowering = std::max(lowering, required * smoothResponse);
            }
        }
    };
    includeVolumes(horde::scene::kShowcaseLowOverheadVolumes);
    includeVolumes(horde::scene::kShowcaseCeilingPatches);
    includeVolumes(horde::scene::kShowcaseSkylightGrid);
    includeVolumes(horde::scene::kShowcaseImportedOverheadVolumes);
    includeVolumes(std::array<horde::scene::OverheadVolume, 1u>{{
        horde::scene::kShowcaseCollapseRoofSeam}});
    return lowering;
}

Vec3 ViewVectorToWorld(const Vec3& viewVector,
                       const Vec3& viewRight,
                       const Vec3& viewUp,
                       const Vec3& viewForward)
{
    return Add(Add(Scale(viewRight, viewVector[0]),
                   Scale(viewUp, viewVector[1])),
               Scale(viewForward, viewVector[2]));
}

} // namespace

float ComputeRewardLanternForwardClearance(const float cameraX,
                                           const float cameraZ,
                                           const float forwardX,
                                           const float forwardZ)
{
    if (!std::isfinite(cameraX) || !std::isfinite(cameraZ) ||
        !std::isfinite(forwardX) || !std::isfinite(forwardZ))
        return 0.0f;
    const float forwardLength = std::hypot(forwardX, forwardZ);
    if (forwardLength <= 0.000001f ||
        !IsShowcaseHeldPropPositionWalkable(cameraX, cameraZ))
        return 0.0f;
    const float unitForwardX = forwardX / forwardLength;
    const float unitForwardZ = forwardZ / forwardLength;
    constexpr float kMaximumClearance = 2.70f;
    constexpr float kSearchStride = 0.025f;
    float lastWalkable = 0.0f;
    for (int step = 1; step <=
         static_cast<int>(kMaximumClearance / kSearchStride); ++step)
    {
        const float distance = static_cast<float>(step) * kSearchStride;
        if (IsShowcaseHeldPropPositionWalkable(
                cameraX + unitForwardX * distance,
                cameraZ + unitForwardZ * distance))
        {
            lastWalkable = distance;
            continue;
        }

        // Resolve the first collision surface continuously. The predicate is
        // already the player-centre route contract and therefore already owns
        // the 24 cm player radius; offsetting it laterally by that radius would
        // double-count the body and create an early, grid-stepped retraction.
        float blocked = distance;
        for (int refinement = 0; refinement < 14; ++refinement)
        {
            const float midpoint = 0.5f * (lastWalkable + blocked);
            if (IsShowcaseHeldPropPositionWalkable(
                    cameraX + unitForwardX * midpoint,
                    cameraZ + unitForwardZ * midpoint))
                lastWalkable = midpoint;
            else
                blocked = midpoint;
        }
        return lastWalkable;
    }
    return kMaximumClearance;
}

HeldSwordPose EvaluateHeldSwordPose(const PlayerCombatSnapshot& playerCombat,
                                   const float swordSwingRadians,
                                   const float heldPropDepth,
                                   const bool bulkyLeftHandCarry,
                                   const float idleTimeSeconds)
{
    float parryBlend = 0.0f;
    switch (playerCombat.action)
    {
    case PlayerCombatAction::ParryStartup:
        parryBlend = std::clamp(playerCombat.actionTime / 0.04f, 0.0f, 1.0f);
        break;
    case PlayerCombatAction::ParryActive:
        parryBlend = 1.0f;
        break;
    case PlayerCombatAction::ParryRecovery:
        parryBlend = 1.0f - std::clamp(playerCombat.actionTime / 0.24f, 0.0f, 1.0f);
        break;
    default:
        break;
    }
    parryBlend = parryBlend * parryBlend * (3.0f - 2.0f * parryBlend);
    const float successJolt = playerCombat.reaction == CombatReaction::Parried
        ? std::clamp(playerCombat.reactionTime / 0.12f, 0.0f, 1.0f)
        : 0.0f;
    const auto smooth = [](const float value) {
        const float clamped = std::clamp(value, 0.0f, 1.0f);
        return clamped * clamped * (3.0f - 2.0f * clamped);
    };
    const auto blendHand = [](const Vec3& from, const Vec3& to,
                              const float amount) {
        return Lerp(from, to, amount);
    };
    const float swordGripDepth = heldPropDepth - 0.05f;
    // A bulky off-hand carry reserves the left-side cage volume. Author a
    // right-side cut through all windup/active/recovery phases, not a rendered
    // prop offset: this same grip pose drives the sword hand/arm IK and item.
    // Vertical/depth travel, combat timing and the free-torch arc are unchanged.
    const float cutInward = bulkyLeftHandCarry ? 0.18f : 0.78f;
    // Move the shared rest target by only millimetres. Existing smooth
    // windup/recovery and parry envelopes blend from/to this target; active
    // cut/guard positions, angles, timers and gameplay hit tests stay authored.
    const float breath = std::sin((std::isfinite(idleTimeSeconds) ? idleTimeSeconds : 0.0f) * 1.45f);
    const Vec3 restHand{{0.18f, -0.44f + 0.004f * breath,
                        swordGripDepth + 0.002f * breath}};
    const Vec3 downwardWindupHand{{bulkyLeftHandCarry ? 0.18f : 0.08f, -0.17f,
                                   std::max(0.56f, swordGripDepth - 0.01f)}};
    const Vec3 downwardImpactHand{{bulkyLeftHandCarry ? 0.17f : 0.02f, -0.70f,
                                   std::max(0.48f, swordGripDepth - 0.18f)}};
    const Vec3 upwardStartHand{{bulkyLeftHandCarry ? 0.17f : 0.01f, -0.72f,
                                std::max(0.48f, swordGripDepth - 0.16f)}};
    const Vec3 upwardEndHand{{bulkyLeftHandCarry ? 0.18f : 0.06f, -0.15f,
                              std::max(0.52f, swordGripDepth - 0.08f)}};

    Vec3 swingHand = restHand;
    float swingInwardRadians = kSwordRestInwardRadians;
    float swingForwardRadians = kSwordRestForwardRadians;
    switch (playerCombat.action)
    {
    case PlayerCombatAction::SwingWindup:
    {
        const float amount = smooth(
            playerCombat.actionTime / SwordCombat::kSwingWindupDuration);
        swingHand = blendHand(restHand, downwardWindupHand, amount);
        swingInwardRadians = kSwordRestInwardRadians +
            (-0.05f - kSwordRestInwardRadians) * amount;
        swingForwardRadians = kSwordRestForwardRadians +
            (0.55f - kSwordRestForwardRadians) * amount;
        break;
    }
    case PlayerCombatAction::SwingActive:
    {
        const float amount = smooth(
            playerCombat.actionTime /
            SwordCombat::kDownwardCutTravelDuration);
        swingHand = blendHand(downwardWindupHand, downwardImpactHand, amount);
        // The blade tilts strongly into depth while the hand travels down.
        // This reads as a real camera-facing cut without sweeping the full
        // metre-long blade beyond a narrow phone's horizontal safe frame.
        swingInwardRadians = -0.05f + (cutInward + 0.05f) * amount;
        swingForwardRadians = 0.55f + (1.00f - 0.55f) * amount;
        break;
    }
    case PlayerCombatAction::SwingRecovery:
    {
        const float amount = smooth(
            playerCombat.actionTime / SwordCombat::kSwingRecoveryDuration);
        swingHand = blendHand(downwardImpactHand, restHand, amount);
        swingInwardRadians = cutInward +
            (kSwordRestInwardRadians - cutInward) * amount;
        swingForwardRadians = 1.00f +
            (kSwordRestForwardRadians - 1.00f) * amount;
        break;
    }
    case PlayerCombatAction::UpwardSliceWindup:
    {
        const float amount = smooth(
            playerCombat.actionTime / SwordCombat::kUpwardSliceWindupDuration);
        swingHand = blendHand(downwardImpactHand, upwardStartHand, amount);
        swingInwardRadians = cutInward + 0.04f * amount;
        swingForwardRadians = 1.00f - 0.02f * amount;
        break;
    }
    case PlayerCombatAction::UpwardSliceActive:
    {
        const float amount = smooth(
            playerCombat.actionTime / SwordCombat::kUpwardSliceActiveDuration);
        swingHand = blendHand(upwardStartHand, upwardEndHand, amount);
        swingInwardRadians = cutInward + 0.04f + (-0.30f - cutInward - 0.04f) * amount;
        swingForwardRadians = 0.98f + (0.60f - 0.98f) * amount;
        break;
    }
    case PlayerCombatAction::UpwardSliceRecovery:
    {
        const float amount = smooth(
            playerCombat.actionTime / SwordCombat::kUpwardSliceRecoveryDuration);
        swingHand = blendHand(upwardEndHand, restHand, amount);
        swingInwardRadians = -0.30f +
            (kSwordRestInwardRadians + 0.30f) * amount;
        swingForwardRadians = 0.60f +
            (kSwordRestForwardRadians - 0.60f) * amount;
        break;
    }
    default:
        // Imported legacy captures may carry only the scalar swing. Keep a
        // bounded fallback without allowing that old sideways-only value to
        // override the explicit cut phases above.
        if (std::abs(swordSwingRadians) > 0.0001f)
        {
            const float amount = smooth(std::clamp(
                -swordSwingRadians / SwordCombat::kDownwardSwingAmplitude,
                0.0f, 1.0f));
            swingHand = blendHand(restHand, downwardImpactHand, amount);
            swingInwardRadians = kSwordRestInwardRadians +
                (cutInward - kSwordRestInwardRadians) * amount;
            swingForwardRadians = kSwordRestForwardRadians +
                (1.00f - kSwordRestForwardRadians) * amount;
        }
        break;
    }
    const std::array<float, 3u> parryHand{{
        (bulkyLeftHandCarry ? 0.08f : -0.16f) + 0.055f * successJolt,
        -0.29f + 0.025f * successJolt,
        std::min(heldPropDepth, 0.90f)}};
    // With the lantern carried, keep the guard/forearm on the sword side.
    // A more upright blade retains a similar blocking tip position without
    // driving the right hand through the low left arm. The lantern stays put.
    const float parryInwardRadians = bulkyLeftHandCarry ? -0.28f : -0.62f;

    HeldSwordPose pose;
    for (std::size_t axis = 0u; axis < pose.rightHandLocal.size(); ++axis)
    {
        pose.rightHandLocal[axis] = swingHand[axis] +
            (parryHand[axis] - swingHand[axis]) * parryBlend;
    }
    pose.swordRadians = swingInwardRadians +
                        parryBlend * (parryInwardRadians + 0.14f * successJolt -
                                      swingInwardRadians);
    pose.swordForwardRadians = swingForwardRadians +
        parryBlend * (kSwordRestForwardRadians - swingForwardRadians);
    pose.parryBlend = parryBlend;
    pose.successJolt = successJolt;
    return pose;
}

std::array<float, 3u> EvaluateSwordBladeAxisInView(
    const float inwardRadians,
    const float forwardRadians)
{
    const float forwardCos = std::cos(forwardRadians);
    return {{-std::sin(inwardRadians) * forwardCos,
             std::cos(inwardRadians) * forwardCos,
             std::sin(forwardRadians)}};
}

SwordGripBasisInView EvaluateSwordGripBasisInView(
    const float inwardRadians,
    const float forwardRadians,
    const float gripRollRadians)
{
    const float swordCos = std::cos(inwardRadians);
    const float swordSin = std::sin(inwardRadians);
    const Vec3 unrolledEdge{{swordCos, swordSin, 0.0f}};
    const Vec3 bladeAxis = EvaluateSwordBladeAxisInView(inwardRadians, forwardRadians);
    // View right/up/forward is a left-handed coordinate frame because world
    // forward points down -Z. Negate the algebraic cross product so the
    // authored +X/+Y/+Z Grip basis remains right-handed once mapped to world.
    const Vec3 unrolledFlat = Scale(Normalize(Cross(unrolledEdge, bladeAxis)), -1.0f);
    const float rollCos = std::cos(gripRollRadians);
    const float rollSin = std::sin(gripRollRadians);
    SwordGripBasisInView result;
    result.edgeDirection = Normalize(Add(Scale(unrolledEdge, rollCos),
                                         Scale(unrolledFlat, -rollSin)));
    result.bladeAxis = bladeAxis;
    result.flatNormal = Normalize(Add(Scale(unrolledFlat, rollCos),
                                      Scale(unrolledEdge, rollSin)));
    return result;
}

FirstPersonSafeFrame EvaluateOwnerFeedbackPortraitSafeFrame(
    const HeldItemKinematicsState& kinematics,
    const float portraitAspect)
{
    FirstPersonSafeFrame result;
    result.minimumNdcX = 1.0e9f;
    result.maximumNdcX = -1.0e9f;
    const float aspect = std::max(portraitAspect, 0.1f);
    const auto include = [&](const Vec3& point)
    {
        const float depth = std::max(point[2], 0.05f);
        const float ndcX = 1.22f * point[0] / (depth * aspect);
        result.minimumNdcX = std::min(result.minimumNdcX, ndcX);
        result.maximumNdcX = std::max(result.maximumNdcX, ndcX);
    };

    const auto fromGrip = [&kinematics](const Vec3& local)
    {
        return Add(kinematics.leftHandLocal,
            Add(Scale(kinematics.leftGripXInView, local[0]),
                Add(Scale(kinematics.leftGripYInView, local[1]),
                    Scale(kinematics.leftGripZInView, local[2]))));
    };
    // Measured from the player Rag torch runtime GLB and expressed relative
    // to its Grip: bounds x[-.083066,.044008], y[-.240,.610000],
    // z[-.062917,.063452]; Flame=(-.013873,.565,-.002581),
    // Light=(-.015804,.540,.023154). Project actual box corners and authored
    // sockets through the shared grip axes, including the engine flame tip.
    constexpr float minimumX = -0.083066f;
    constexpr float maximumX = 0.044008f;
    constexpr float minimumY = -0.240000f;
    constexpr float maximumY = 0.610001f;
    constexpr float minimumZ = -0.062917f;
    constexpr float maximumZ = 0.063452f;
    for (const float x : {minimumX, maximumX})
        for (const float y : {minimumY, maximumY})
            for (const float z : {minimumZ, maximumZ})
                include(fromGrip({{x, y, z}}));
    result.includesTorchGrip = true;
    constexpr Vec3 flameFromGrip{{-0.013873f, 0.565000f, -0.002581f}};
    constexpr Vec3 lightFromGrip{{-0.015804f, 0.540000f, 0.023154f}};
    const Vec3 flame = fromGrip(flameFromGrip);
    include(flame);
    include(Add(flame, Scale(kinematics.leftGripYInView, 0.34f)));
    result.includesFlame = true;
    include(fromGrip(lightFromGrip));
    result.includesLight = true;

    const SwordGripBasisInView basis = EvaluateSwordGripBasisInView(
        kinematics.swordRadians, kinematics.swordForwardRadians,
        kSwordGripRollRadians);
    const Vec3 grip = kinematics.rightHandLocal;
    include(grip);
    result.includesSwordGrip = true;
    // Runtime sword GLB audited bounds relative to its Grip: blade-long +Y
    // [-0.135, 0.915], edge +X +/-0.112, flat +Z +/-0.025 metres.
    for (const float edge : {-0.112f, 0.112f})
    {
        for (const float blade : {-0.135f, 0.915f})
        {
            for (const float flat : {-0.025f, 0.025f})
            {
                include(Add(grip, Add(Scale(basis.edgeDirection, edge),
                                      Add(Scale(basis.bladeAxis, blade),
                                          Scale(basis.flatNormal, flat)))));
            }
        }
    }
    result.includesBladeBounds = true;
    return result;
}

HeldItemKinematicsState EvaluateHeldItemKinematics(const HeldItemKinematicsInput& input)
{
    const bool anatomicalBody = input.playerMountProfile == PlayerMountProfile::AnatomicalBody;
    const float forwardX = std::sin(input.cameraYawRadians);
    const float forwardZ = -std::cos(input.cameraYawRadians);
    const float forwardClearance = ComputeRewardLanternForwardClearance(
        input.cameraX, input.cameraZ, forwardX, forwardZ);
    // All rigid hand props share the continuous collision clearance. The
    // legacy 7.5 cm sampled helper produced a visible one-tick sword/forearm
    // jump while approaching a wall and could feed a discontinuous pose into
    // the skinned BLAS refit. Start the response early enough that a normal
    // fixed walking tick changes hand depth by only a bounded amount.
    const float carriedPropDepth = MapContinuousCarryDepth(
        forwardClearance, 0.68f);
    const float swordPropDepth = MapContinuousCarryDepth(
        forwardClearance, anatomicalBody ? 0.65f : 0.82f);
    const LowerBodyPoseState lowerBodyPose = EvaluateLowerBodyPose(input.walkTime, input.walkAmount);
    const float movement = std::max(std::clamp(input.walkAmount, 0.0f, 1.0f), 0.2f);
    const float torchSway = std::sin(input.walkTime * 6.2f) * 0.035f * movement;
    const float torchBob = std::abs(std::sin(input.walkTime * 6.2f)) * 0.025f * movement;
    const std::array<float, 3u> heldLeftHand{{
        -0.16f - torchSway, -0.41f + torchBob, carriedPropDepth}};
    const bool rewardLantern = input.interaction.heldLightKind ==
        horde::gameplay::interactions::HeldLightKind::RewardLantern;
    // The authored lantern is almost one metre tall. In open space it needs
    // more perspective distance than the compact torch so its full swing cone
    // remains readable on narrow portrait phones. Near route collision, the
    // same sampled clearance smoothly removes that advance and adds the body
    // radius inset needed to keep the complete authored cage camera-side of
    // the wall. This remains one shared carry contract on every platform.
    const float rewardForwardClearance = rewardLantern
        ? forwardClearance : 0.0f;
    // Retract across the complete measured 2.4 m clearance interval instead
    // of compressing the carry-to-wall response into the final metre. This
    // preserves the exact open/emergency endpoints while keeping a normal
    // fixed walking tick below the skinned-arm silhouette continuity bound.
    // A body mounted under the player's eye cannot reach the historical
    // metre-forward ring. Use a reachable authored carry envelope; the same
    // target drives the hand, rigid lantern, pendulum and emitted light.
    const float openRewardDepth = anatomicalBody ? 0.70f : 1.05f;
    const float continuousHeldDepth = MapContinuousCarryDepth(
        rewardForwardClearance, openRewardDepth);
    const float rewardClearance = std::clamp(
        (continuousHeldDepth - 0.30f) / (openRewardDepth - 0.30f), 0.0f, 1.0f);
    const float rewardClearanceBlend = rewardClearance * rewardClearance *
        (3.0f - 2.0f * rewardClearance);
    // Leave enough camera-side travel for the full authored body at the real
    // wall fixture even when downward look pitches the skinned grip forward.
    const float rewardWallInset =
        0.265f * (1.0f - rewardClearanceBlend) + 0.010f;
    // Keep the top ring inside the actual left-arm reach envelope. The
    // previous 1.20 m camera-relative extension required translating the
    // clavicle and, through the old root calculation, the whole player.
    const float rewardOpenAdvance = 0.0f;
    const float preferredRewardDepth =
        continuousHeldDepth - rewardWallInset + rewardOpenAdvance;
    // At the real wall fixture the player centre has only 30 cm of physical
    // camera-to-wall travel. Keep the ring 20 cm forward: this is outside the
    // near-camera exclusion while leaving the compact, sideways cage on the
    // camera side of the wall. The pendulum presentation retains raw motion
    // continuity while the clearance blend reaches this emergency pose.
    const float rewardDepth = std::max(
        0.20f,
        std::min(preferredRewardDepth,
                 std::max(0.20f, rewardForwardClearance - 0.30f)));
    // At the tight wall fixture there is not enough forward clearance for the
    // long cage to hang toward the wall. Keep the ring on its anatomical left
    // side and turn the cage sideways below it. The low reward chest is not a
    // full-height held-prop obstruction, so it retains the open-space target.
    const float rewardLateral =
        -0.45f + 0.32f * rewardClearanceBlend * rewardClearanceBlend;
    const float rewardHighVerticalWallOffset =
        -0.18f * (1.0f - rewardClearanceBlend);
    const float rewardLowVerticalWallOffset =
        -0.04f * (1.0f - rewardClearanceBlend);
    const std::array<float, 3u> rewardHighLeftHand{{
        rewardLateral - torchSway * 0.35f,
        -0.05f + rewardHighVerticalWallOffset + torchBob * 0.25f,
        rewardDepth}};
    const std::array<float, 3u> rewardLowLeftHand{{
        rewardLateral - torchSway * 0.35f,
        -0.24f + rewardLowVerticalWallOffset + torchBob * 0.25f,
        rewardDepth}};
    constexpr std::array<float, 3u> loweredLeftHand{{-0.36f, -0.92f, 0.27f}};
    float lowerBlend = std::clamp(input.torchFailure.leftArmLowerBlend, 0.0f, 1.0f);
    if (rewardLantern)
    {
        using horde::gameplay::interactions::HeldLightPose;
        switch (input.interaction.heldLightPose)
        {
        case HeldLightPose::Low:
            lowerBlend = 1.0f;
            break;
        case HeldLightPose::TransitioningToLow:
            lowerBlend = std::clamp(input.interaction.heldLightPoseProgress, 0.0f, 1.0f);
            break;
        case HeldLightPose::TransitioningToHigh:
            lowerBlend = 1.0f - std::clamp(
                input.interaction.heldLightPoseProgress, 0.0f, 1.0f);
            break;
        default:
            lowerBlend = 0.0f;
            break;
        }
    }
    const HeldSwordPose sword = EvaluateHeldSwordPose(
        input.playerCombat, input.swordSwingRadians, swordPropDepth, rewardLantern,
        input.walkTime);

    HeldItemKinematicsState result;
    // Props move the hand effector only. Keep the calibrated clavicle/shoulder
    // on the torso for torch, phone, sword, and reward carries alike; moving
    // the whole physical arm subtree to the raised reward hand was the source
    // of the near-camera horizontal polygons reported in owner playtesting.
    result.leftShoulderLocal = {{
        -0.36f,
        -0.44f + lowerBodyPose.pelvisBob * 0.35f,
        0.39f - lowerBodyPose.leftStride * 0.018f}};
    result.rightShoulderLocal = {{
        0.36f,
        -0.44f + lowerBodyPose.pelvisBob * 0.35f,
        0.39f + lowerBodyPose.leftStride * 0.018f}};
    if (anatomicalBody)
    {
        // Nominal authored shoulder centres in the yaw-relative body frame.
        // Convert to view coordinates so pitch does not drag the torso forward.
        // Actual skin shoulders still come from the shared authored rig pose.
        const float verticalForward = -0.05f + std::clamp(input.cameraPitchRadians, -0.32f, 0.28f);
        const float inverseLength = 1.0f / std::sqrt(1.0f + verticalForward * verticalForward);
        constexpr float shoulderBelowEye = -0.184f;
        constexpr float shoulderBehindEye = -0.078f;
        const float y = (shoulderBelowEye - shoulderBehindEye * verticalForward) * inverseLength;
        const float z = (shoulderBelowEye * verticalForward + shoulderBehindEye) * inverseLength;
        result.leftShoulderLocal = {{-0.166f, y, z}};
        result.rightShoulderLocal = {{0.166f, y, z}};
    }
    for (std::size_t axis = 0u; axis < result.leftHandLocal.size(); ++axis)
    {
        const auto& highTarget = rewardLantern ? rewardHighLeftHand : heldLeftHand;
        const auto& lowTarget = rewardLantern ? rewardLowLeftHand : loweredLeftHand;
        result.leftHandLocal[axis] = highTarget[axis] +
            (lowTarget[axis] - highTarget[axis]) * lowerBlend;
    }
    result.rightHandLocal = sword.rightHandLocal;
    if (anatomicalBody)
    {
        // The closer body-mounted carry rests at lower-chest height rather
        // than the historical distant waist-height presentation. Raise both
        // authored hand paths together, preserving their relative attack and
        // parry motion; no renderer-only prop or frozen-capture correction.
        result.leftHandLocal[1] += 0.10f;
        result.rightHandLocal[1] += 0.10f;
    }
    {
        const Vec3 worldUp{{0.0f, 1.0f, 0.0f}};
        const float pitch = std::clamp(input.cameraPitchRadians, -0.32f, 0.28f);
        const Vec3 viewForward = Normalize({{forwardX, -0.05f + pitch, forwardZ}});
        const Vec3 viewRight = Normalize(Cross(viewForward, worldUp));
        const Vec3 viewUp = Normalize(Cross(viewRight, viewForward));
        const SwordGripBasisInView basis = EvaluateSwordGripBasisInView(
            sword.swordRadians, sword.swordForwardRadians, kSwordGripRollRadians);
        const Vec3 initialGripWorld = Add(
            Vec3{{input.cameraX, kShowcaseEyeWorldY, input.cameraZ}},
            ViewVectorToWorld(result.rightHandLocal, viewRight, viewUp, viewForward));
        const float initialLowering = SwordOverheadLowering(
            initialGripWorld,
            ViewVectorToWorld(basis.bladeAxis, viewRight, viewUp, viewForward),
            ViewVectorToWorld(basis.edgeDirection, viewRight, viewUp, viewForward),
            ViewVectorToWorld(basis.flatNormal, viewRight, viewUp, viewForward));
        // Lowering can pull a long blade beyond the imported body's arm reach.
        // Retreat the whole grip toward the camera along horizontal forward;
        // this also moves the blade clear of a lintel footprint when possible.
        // The response starts at zero in open rooms and is capped at 30 cm.
        const float retraction = std::min(0.30f, initialLowering * 0.50f);
        const Vec3 horizontalForward{{forwardX, 0.0f, forwardZ}};
        const Vec3 retreatWorld = Scale(horizontalForward, -retraction);
        result.rightHandLocal[0] += Dot(retreatWorld, viewRight);
        result.rightHandLocal[1] += Dot(retreatWorld, viewUp);
        result.rightHandLocal[2] += Dot(retreatWorld, viewForward);
        const Vec3 gripWorld = Add(initialGripWorld, retreatWorld);
        const float swordLowering = SwordOverheadLowering(
            gripWorld,
            ViewVectorToWorld(basis.bladeAxis, viewRight, viewUp, viewForward),
            ViewVectorToWorld(basis.edgeDirection, viewRight, viewUp, viewForward),
            ViewVectorToWorld(basis.flatNormal, viewRight, viewUp, viewForward));
        // Lower the actual right-hand target in world space. ResolveHeldItems
        // uses this same state for arm IK and sword socket composition, so the
        // RT geometry, shadows, and reflections remain on the corrected frame.
        result.rightHandLocal[1] -= swordLowering * viewUp[1];
        result.rightHandLocal[2] -= swordLowering * viewForward[1];
        result.swordOverheadLowering = swordLowering;
        result.swordOverheadRetraction = retraction;
    }
    if (!rewardLantern && input.torchFailure.heldByPlayer)
    {
        const Vec3 worldUp{{0.0f, 1.0f, 0.0f}};
        const float pitch = std::clamp(input.cameraPitchRadians, -0.32f, 0.28f);
        const Vec3 viewForward = Normalize({{forwardX, -0.05f + pitch, forwardZ}});
        const Vec3 viewRight = Normalize(Cross(viewForward, worldUp));
        const Vec3 viewUp = Normalize(Cross(viewRight, viewForward));
        const Vec3 gripWorld = Add(
            Vec3{{input.cameraX, kShowcaseEyeWorldY, input.cameraZ}},
            Add(Scale(viewRight, result.leftHandLocal[0]),
                Add(Scale(viewUp, result.leftHandLocal[1]),
                    Scale(viewForward, result.leftHandLocal[2]))));
        const float initialLowering = TorchOverheadLowering(gripWorld, viewUp, viewForward);
        // A low grip held at the full forward extension exceeds the admitted
        // anatomical arm's reach, especially when looking down. Retract along
        // the horizontal player forward axis before resolving final overhead
        // clearance. This moves the complete hand/item frame, not the light.
        const float retraction = anatomicalBody
            ? std::min(0.45f, initialLowering * 0.45f) : 0.0f;
        const Vec3 horizontalForward{{forwardX, 0.0f, forwardZ}};
        const Vec3 retractedGrip = Add(gripWorld, Scale(horizontalForward, -retraction));
        result.leftHandLocal[1] -= retraction *
            (horizontalForward[0] * viewUp[0] + horizontalForward[2] * viewUp[2]);
        result.leftHandLocal[2] -= retraction *
            (horizontalForward[0] * viewForward[0] + horizontalForward[2] * viewForward[2]);
        result.torchOverheadLowering = TorchOverheadLowering(retractedGrip, viewUp, viewForward);
        // Inverse view projection of world down: both IK and socket composition
        // consume this same hand target. Do not tilt/fade the flame or detach its
        // light to obtain clearance. Lowered/falling/reward paths remain owned.
        result.leftHandLocal[1] -= result.torchOverheadLowering * viewUp[1];
        result.leftHandLocal[2] -= result.torchOverheadLowering * viewForward[1];
    }
    const float leftGripRollCos = std::cos(kLeftGripRollRadians);
    const float leftGripRollSin = std::sin(kLeftGripRollRadians);
    result.leftGripXInView = {{leftGripRollCos, 0.0f, leftGripRollSin}};
    result.leftGripYInView = {{0.0f, 1.0f, 0.0f}};
    result.leftGripZInView = {{leftGripRollSin, 0.0f, -leftGripRollCos}};
    result.heldPropDepth = carriedPropDepth;
    result.rewardLanternPresentationYawRadians = rewardLantern
        ? 1.57079632679f * (1.0f - rewardClearanceBlend)
        : 0.0f;
    result.swordRadians = sword.swordRadians;
    result.swordForwardRadians = sword.swordForwardRadians;
    result.parryBlend = sword.parryBlend;
    result.successJolt = sword.successJolt;
    return result;
}

const horde::scene::assets::StaticSocket* FindHeldItemSocket(
    const std::span<const horde::scene::assets::StaticSocket> sockets,
    const std::string_view name)
{
    for (const auto& socket : sockets)
    {
        if (socket.name == name) return &socket;
    }
    return nullptr;
}

bool ValidateHeldItemSocketTransform(const HeldItemTransform& transform,
                                     std::string& diagnostic)
{
    constexpr float tolerance = 0.001f;
    const bool affine = std::abs(transform[3]) <= tolerance &&
                        std::abs(transform[7]) <= tolerance &&
                        std::abs(transform[11]) <= tolerance &&
                        std::abs(transform[15] - 1.0f) <= tolerance;
    const bool unit = std::abs(DotColumn(transform, 0u, 0u) - 1.0f) <= tolerance &&
                      std::abs(DotColumn(transform, 1u, 1u) - 1.0f) <= tolerance &&
                      std::abs(DotColumn(transform, 2u, 2u) - 1.0f) <= tolerance;
    const bool orthogonal = std::abs(DotColumn(transform, 0u, 1u)) <= tolerance &&
                            std::abs(DotColumn(transform, 0u, 2u)) <= tolerance &&
                            std::abs(DotColumn(transform, 1u, 2u)) <= tolerance;
    const float determinant =
        transform[0] * (transform[5] * transform[10] - transform[9] * transform[6]) -
        transform[4] * (transform[1] * transform[10] - transform[9] * transform[2]) +
        transform[8] * (transform[1] * transform[6] - transform[5] * transform[2]);
    if (!affine || !unit || !orthogonal || std::abs(determinant - 1.0f) > tolerance)
    {
        diagnostic = "Held-item socket transform must be rigid and unit scale.";
        return false;
    }
    diagnostic.clear();
    return true;
}

HeldItemTransform MultiplyHeldItemTransforms(const HeldItemTransform& left,
                                             const HeldItemTransform& right)
{
    HeldItemTransform result{};
    for (std::size_t column = 0u; column < 4u; ++column)
    {
        for (std::size_t row = 0u; row < 4u; ++row)
        {
            for (std::size_t inner = 0u; inner < 4u; ++inner)
            {
                result[column * 4u + row] +=
                    left[inner * 4u + row] * right[column * 4u + inner];
            }
        }
    }
    return result;
}

bool ComposeWorldFromItem(const HeldItemTransform& worldFromHandSocket,
                          const HeldItemTransform& itemFromGrip,
                          HeldItemTransform& worldFromItem,
                          std::string& diagnostic)
{
    if (!ValidateHeldItemSocketTransform(worldFromHandSocket, diagnostic) ||
        !ValidateHeldItemSocketTransform(itemFromGrip, diagnostic))
    {
        return false;
    }
    worldFromItem = MultiplyHeldItemTransforms(
        worldFromHandSocket, InverseRigidTransform(itemFromGrip));
    diagnostic.clear();
    return true;
}

HeldItemTransform SelectHandSocketTransform(const HeldHand hand,
                                            const HeldItemTransform& worldFromLeftHand,
                                            const HeldItemTransform& worldFromRightHand)
{
    return hand == HeldHand::LeftHand ? worldFromLeftHand : worldFromRightHand;
}

bool ResolveHeldItemsFixedStep(HeldItemStates& items,
                               const HeldItemFixedStepInput& input,
                               const std::uint64_t tick,
                               HeldItemFixedStepState& state,
                               std::string& diagnostic)
{
    state.kinematics = EvaluateHeldItemKinematics({
        input.playerX,
        input.playerZ,
        input.playerYawRadians,
        input.walkTime,
        input.walkAmount,
        input.torchFailure,
        input.playerCombat,
        input.swordSwingRadians,
        input.interaction,
        input.playerMountProfile,
        input.playerPitchRadians});

    constexpr Vec3 worldUp{{0.0f, 1.0f, 0.0f}};
    const float pitch = std::clamp(input.playerPitchRadians, -0.32f, 0.28f);
    const Vec3 viewForward = Normalize({{
        std::sin(input.playerYawRadians), -0.05f + pitch,
        -std::cos(input.playerYawRadians)}});
    const Vec3 viewRight = Normalize(Cross(viewForward, worldUp));
    const Vec3 viewUp = Normalize(Cross(viewRight, viewForward));
    const Vec3 eye{{input.playerX, horde::gameplay::kShowcaseEyeWorldY,
                    input.playerZ}};
    const auto toWorld = [&](const std::array<float, 3u>& local) {
        return Add(Add(Add(eye, Scale(viewRight, local[0])),
                       Scale(viewUp, local[1])),
                   Scale(viewForward, local[2]));
    };

    const Vec3 leftHand = toWorld(state.kinematics.leftHandLocal);
    const Vec3 rightHand = toWorld(state.kinematics.rightHandLocal);
    const auto gripAxisToWorld = [&](const Vec3& axisInView) {
        return Normalize(Add(Add(Scale(viewRight, axisInView[0]),
                                 Scale(viewUp, axisInView[1])),
                             Scale(viewForward, axisInView[2])));
    };
    const HeldItemTransform worldFromLeftHand = WorldFromAxes(
        gripAxisToWorld(state.kinematics.leftGripXInView),
        gripAxisToWorld(state.kinematics.leftGripYInView),
        gripAxisToWorld(state.kinematics.leftGripZInView), leftHand);
    state.worldFromLeftHand = worldFromLeftHand;
    HeldItemTransform heldWorldFromTorch{};
    if (!ComposeWorldFromItem(worldFromLeftHand,
                              PlayerRagTorchGripSocketTransform(),
                              heldWorldFromTorch,
                              diagnostic))
    {
        return false;
    }

    if (input.torchFailure.heldByPlayer)
    {
        UpdateHeldItemParent(items[0], HeldItemParentMode::HandSocket,
                             tick, heldWorldFromTorch);
    }
    else if (items[0].parentMode == HeldItemParentMode::HandSocket)
    {
        // The transition tick deliberately publishes the previous resolved
        // hand attachment as both the detach basis and current trajectory
        // matrix. This makes the sampled boundary exactly continuous even
        // when gait sway/bob was non-zero on the previous fixed tick.
        UpdateHeldItemParent(items[0], HeldItemParentMode::AuthoredWorldTrajectory,
                             tick, items[0].worldFromItem);
    }
    else
    {
        const float progress = std::clamp(input.torchFailure.fallProgress, 0.0f, 1.0f);
        const float releaseYawCos = std::cos(input.torchFailure.droppedYawRadians);
        const float releaseYawSin = std::sin(input.torchFailure.droppedYawRadians);
        const Vec3 releaseBodyForward{{releaseYawSin, 0.0f, -releaseYawCos}};
        const Vec3 releaseBodyRight{{releaseYawCos, 0.0f, releaseYawSin}};
        const float pitchCos = std::cos(input.torchFailure.droppedPitchRadians);
        const float pitchSin = std::sin(input.torchFailure.droppedPitchRadians);
        const Vec3 restingX = releaseBodyRight;
        const Vec3 restingY = Normalize(Add(Scale(worldUp, pitchCos),
                                            Scale(releaseBodyForward, pitchSin)));
        const Vec3 restingZ = Normalize(Add(Scale(releaseBodyForward, pitchCos),
                                            Scale(worldUp, -pitchSin)));
        const Vec3 fallingX = Normalize(Lerp(ColumnOf(items[0].worldFromDetach, 0u),
                                             restingX, progress));
        const Vec3 fallingY = Normalize(Lerp(ColumnOf(items[0].worldFromDetach, 1u),
                                             restingY, progress));
        const Vec3 fallingZ = Normalize(Lerp(ColumnOf(items[0].worldFromDetach, 2u),
                                             Scale(restingZ, -1.0f), progress));
        Vec3 settledPosition = Add(
            Add(Vec3{{input.torchFailure.droppedX,
                      input.torchFailure.droppedY,
                      input.torchFailure.droppedZ}},
                Scale(worldUp, 0.13f)),
            Add(Scale(releaseBodyRight, -0.34f),
                Scale(releaseBodyForward, 0.78f)));
        settledPosition[0] = std::clamp(settledPosition[0], -2.28f, 4.58f);
        settledPosition[2] = std::clamp(settledPosition[2], -16.18f, -14.22f);
        const HeldItemTransform worldFromTorch = WorldFromAxes(
            fallingX, fallingY, fallingZ,
            Lerp(TranslationOf(items[0].worldFromDetach), settledPosition, progress));
        UpdateHeldItemParent(items[0], HeldItemParentMode::AuthoredWorldTrajectory,
                             tick, worldFromTorch);
    }

    const SwordGripBasisInView swordBasis = EvaluateSwordGripBasisInView(
        state.kinematics.swordRadians, state.kinematics.swordForwardRadians,
        kSwordGripRollRadians);
    const Vec3 swordEdge = Normalize(Add(
        Add(Scale(viewRight, swordBasis.edgeDirection[0]),
            Scale(viewUp, swordBasis.edgeDirection[1])),
        Scale(viewForward, swordBasis.edgeDirection[2])));
    const auto& swordBladeInView = swordBasis.bladeAxis;
    const Vec3 swordAxisY = Normalize(Add(
        Add(Scale(viewRight, swordBladeInView[0]), Scale(viewUp, swordBladeInView[1])),
        Scale(viewForward, swordBladeInView[2])));
    const Vec3 swordFlat = Normalize(Add(
        Add(Scale(viewRight, swordBasis.flatNormal[0]),
            Scale(viewUp, swordBasis.flatNormal[1])),
        Scale(viewForward, swordBasis.flatNormal[2])));
    HeldItemTransform worldFromSword{};
    if (!ComposeWorldFromItem(
            WorldFromAxes(swordEdge, swordAxisY, swordFlat, rightHand),
            SwordGripSocketTransform(), worldFromSword, diagnostic))
    {
        return false;
    }
    UpdateHeldItemParent(items[1], HeldItemParentMode::HandSocket,
                         tick, worldFromSword);

    return ComposeHeldLightState(items[0].worldFromItem,
                                 PlayerRagTorchFlameSocketTransform(),
                                 PlayerRagTorchLightSocketTransform(),
                                 input.torchFailure.flameStrength,
                                 state.light,
                                 diagnostic);
}

} // namespace horde::gameplay::items
