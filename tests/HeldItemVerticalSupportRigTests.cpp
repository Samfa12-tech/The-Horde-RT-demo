#include "gameplay/animation/PlayerAnimationState.h"
#include "gameplay/DevelopmentCheckpointSimulation.h"
#include "gameplay/items/HeldItemKinematics.h"
#include "gameplay/items/LanternPendulum.h"
#include "scene/ShowcaseOverheadGeometry.h"
#include "scene/assets/AssetManifest.h"
#include "scene/assets/StaticMeshAsset.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace
{
using namespace horde::gameplay;
using namespace horde::gameplay::items;
using namespace horde::vulkan::raytracing;
using Vec3 = std::array<float, 3u>;

Vec3 Add(const Vec3& left, const Vec3& right)
{
    return {{left[0] + right[0], left[1] + right[1], left[2] + right[2]}};
}

Vec3 Scale(const Vec3& value, const float amount)
{
    return {{value[0] * amount, value[1] * amount, value[2] * amount}};
}

float Dot(const Vec3& left, const Vec3& right)
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

Vec3 Cross(const Vec3& left, const Vec3& right)
{
    return {{left[1] * right[2] - left[2] * right[1],
             left[2] * right[0] - left[0] * right[2],
             left[0] * right[1] - left[1] * right[0]}};
}

Vec3 Unit(const Vec3& value)
{
    return Scale(value, 1.0f / std::sqrt(Dot(value, value)));
}

Vec3 ViewToWorld(const Vec3& local,
                 const Vec3& right,
                 const Vec3& up,
                 const Vec3& forward)
{
    return Add(Add(Scale(right, local[0]), Scale(up, local[1])),
               Scale(forward, local[2]));
}

Vec3 TransformPoint(const HeldItemTransform& transform, const Vec3& point)
{
    return {{transform[12] + transform[0] * point[0] + transform[4] * point[1] + transform[8] * point[2],
             transform[13] + transform[1] * point[0] + transform[5] * point[1] + transform[9] * point[2],
             transform[14] + transform[2] * point[0] + transform[6] * point[1] + transform[10] * point[2]}};
}

struct WorldBounds
{
    Vec3 minimum{{std::numeric_limits<float>::infinity(),
                  std::numeric_limits<float>::infinity(),
                  std::numeric_limits<float>::infinity()}};
    Vec3 maximum{{-std::numeric_limits<float>::infinity(),
                  -std::numeric_limits<float>::infinity(),
                  -std::numeric_limits<float>::infinity()}};
    std::size_t points = 0u;
};

void Include(WorldBounds& bounds, const Vec3& point)
{
    for (std::size_t axis = 0u; axis < 3u; ++axis)
    {
        bounds.minimum[axis] = std::min(bounds.minimum[axis], point[axis]);
        bounds.maximum[axis] = std::max(bounds.maximum[axis], point[axis]);
    }
    ++bounds.points;
}

struct OverheadTriangle
{
    Vec3 a{}, b{}, c{};
    float minX = 0.0f, maxX = 0.0f, minZ = 0.0f, maxZ = 0.0f;
};

std::vector<OverheadTriangle> ImportedStructuralUndersides(
    const horde::scene::assets::StaticMeshAsset& asset)
{
    std::vector<OverheadTriangle> triangles;
    for (const auto& primitive : asset.primitives)
    {
        if (asset.materials[primitive.materialIndex].name != "MedievalWall02") continue;
        for (std::uint32_t offset = 0u; offset < primitive.indexCount; offset += 3u)
        {
            std::array<Vec3, 3u> points{};
            for (std::size_t corner = 0u; corner < 3u; ++corner)
            {
                const auto& position = asset.vertices[primitive.vertexOffset +
                    asset.indices[primitive.indexOffset + offset + corner]].position;
                points[corner] = {{position[0], position[1], position[2]}};
            }
            const Vec3 normal = Cross(Add(points[1], Scale(points[0], -1.0f)),
                                      Add(points[2], Scale(points[0], -1.0f)));
            if (normal[1] >= -1.0e-7f ||
                std::min({points[0][1], points[1][1], points[2][1]}) <
                    horde::gameplay::kShowcaseEyeWorldY)
                continue;
            triangles.push_back({points[0], points[1], points[2],
                std::min({points[0][0], points[1][0], points[2][0]}),
                std::max({points[0][0], points[1][0], points[2][0]}),
                std::min({points[0][2], points[1][2], points[2][2]}),
                std::max({points[0][2], points[1][2], points[2][2]})});
        }
    }
    return triangles;
}

bool Inside(const horde::scene::OverheadVolume& volume, const Vec3& point)
{
    int first = 0;
    for (std::size_t edge = 0u; edge < volume.footprint.size(); ++edge)
    {
        const auto& a = volume.footprint[edge];
        const auto& b = volume.footprint[(edge + 1u) % volume.footprint.size()];
        const float cross = (b[0] - a[0]) * (point[2] - a[1]) -
                            (b[1] - a[1]) * (point[0] - a[0]);
        const int side = cross < -1.0e-6f ? -1 : cross > 1.0e-6f ? 1 : 0;
        if (first != 0 && side != 0 && first != side) return false;
        if (side != 0) first = side;
    }
    return true;
}

float SharedRoofY(const Vec3& point)
{
    float roof = 100.0f;
    const auto include = [&](const auto& volumes) {
        for (const auto& volume : volumes)
            if (Inside(volume, point))
                roof = std::min(roof, horde::scene::MinimumOverheadBottomY(
                    volume, point[0], point[2], 0.0f));
    };
    include(horde::scene::kShowcaseLowOverheadVolumes);
    include(horde::scene::kShowcaseCeilingPatches);
    include(horde::scene::kShowcaseSkylightGrid);
    include(horde::scene::kShowcaseImportedOverheadVolumes);
    include(std::array<horde::scene::OverheadVolume, 1u>{{
        horde::scene::kShowcaseCollapseRoofSeam}});
    return roof;
}

bool TriangleY(const OverheadTriangle& triangle, const Vec3& point, float& height)
{
    if (point[0] < triangle.minX - 1.0e-6f || point[0] > triangle.maxX + 1.0e-6f ||
        point[2] < triangle.minZ - 1.0e-6f || point[2] > triangle.maxZ + 1.0e-6f)
        return false;
    const Vec3 ab = Add(triangle.b, Scale(triangle.a, -1.0f));
    const Vec3 ac = Add(triangle.c, Scale(triangle.a, -1.0f));
    const float determinant = ab[0] * ac[2] - ab[2] * ac[0];
    if (std::abs(determinant) < 1.0e-8f) return false;
    const float dx = point[0] - triangle.a[0];
    const float dz = point[2] - triangle.a[2];
    const float u = (dx * ac[2] - dz * ac[0]) / determinant;
    const float v = (ab[0] * dz - ab[2] * dx) / determinant;
    if (u < -1.0e-5f || v < -1.0e-5f || u + v > 1.00001f) return false;
    height = triangle.a[1] + u * ab[1] + v * ac[1];
    return true;
}

float Headroom(const Vec3& point, const std::vector<OverheadTriangle>& triangles)
{
    float roof = SharedRoofY(point);
    for (const auto& triangle : triangles)
    {
        float height = 0.0f;
        if (TriangleY(triangle, point, height)) roof = std::min(roof, height);
    }
    return roof - point[1];
}

HeldItemTransform WorldSocket(const std::array<float, 16u>& bone,
                              const PlayerModelWorldBasis& basis,
                              const Vec3& root)
{
    const auto convert = [&](const Vec3& vector) {
        return PlayerModelVectorToWorld(basis, vector);
    };
    const Vec3 x = Unit(convert({{bone[0], bone[1], bone[2]}}));
    const Vec3 rawY = convert({{bone[4], bone[5], bone[6]}});
    Vec3 y = Unit(Add(rawY, Scale(x, -Dot(rawY, x))));
    Vec3 z = Unit(Cross(x, y));
    if (Dot(z, convert({{bone[8], bone[9], bone[10]}})) < 0.0f)
    {
        y = Scale(y, -1.0f);
        z = Scale(z, -1.0f);
    }
    const Vec3 position = Add(root, convert({{bone[12], bone[13], bone[14]}}));

    HeldItemTransform result = IdentityHeldItemTransform();
    for (std::size_t axis = 0u; axis < 3u; ++axis)
    {
        result[axis] = x[axis];
        result[4u + axis] = y[axis];
        result[8u + axis] = z[axis];
        result[12u + axis] = position[axis];
    }
    return result;
}

struct RigResult
{
    HeldItemFixedStepState fixed{};
    HeldItemStates renderItems{};
    HeldLightState light{};
    HeldItemTransform leftGrip{};
    HeldItemTransform rightGrip{};
    HeldItemTransform bodyStowSword{};
    WorldBounds skinnedBodyBounds{};
    Vec3 eye{}, bodyRoot{}, leftShoulder{}, rightShoulder{}, leftWrist{}, rightWrist{};
    float bootGroundingOffset = 0.0f;
};

bool Solve(PlayerRenderSlot& rig,
           const HeldItemFixedStepInput& input,
           const std::uint64_t tick,
           RigResult& result,
           std::string& diagnostic,
           const horde::gameplay::simulation::SimulationSnapshot* staged = nullptr)
{
    auto items = staged != nullptr ? staged->heldItems : MakeDefaultHeldItemStates();
    if (staged != nullptr)
        result.fixed.kinematics = staged->heldItemKinematics;
    else if (!ResolveHeldItemsFixedStep(items, input, tick, result.fixed, diagnostic))
        return false;

    const float eyeY = kShowcaseEyeWorldY +
        (input.playerSupportWorldY - kRouteFloorWorldY);
    const Vec3 eye{{input.playerX, eyeY, input.playerZ}};
    const Vec3 worldUp{{0.0f, 1.0f, 0.0f}};
    const float pitch = std::clamp(input.playerPitchRadians, -0.32f, 0.28f);
    const Vec3 forward = Unit({{std::sin(input.playerYawRadians),
        -0.05f + pitch, -std::cos(input.playerYawRadians)}});
    const Vec3 right = Unit(Cross(forward, worldUp));
    const Vec3 viewUp = Unit(Cross(right, forward));
    horde::gameplay::animation::PlayerAnimationInput animationInput;
    animationInput.carryingOriginalTorch =
        input.interaction.heldLightKind ==
            horde::gameplay::interactions::HeldLightKind::Torch;
    animationInput.carryingRewardLantern =
        input.interaction.heldLightKind ==
            horde::gameplay::interactions::HeldLightKind::RewardLantern;
    animationInput.heldItemKinematics = result.fixed.kinematics;
    animationInput.playerCombat = input.playerCombat;
    animationInput.walkTime = input.walkTime;
    animationInput.walkAmount = input.walkAmount;
    horde::gameplay::animation::PlayerAnimationState animationState;
    animationState.StepFixed(animationInput, 1.0f / 60.0f);
    auto animation = staged != nullptr ? staged->playerAnimation : animationState.Snapshot();

    const auto gait = EvaluateLowerBodyPose(input.walkTime, input.walkAmount);
    const Vec3 bodyForward{{std::sin(input.playerYawRadians), 0.0f,
                            -std::cos(input.playerYawRadians)}};
    const Vec3 bodyRight{{std::cos(input.playerYawRadians), 0.0f,
                          std::sin(input.playerYawRadians)}};
    const float cosine = std::cos(gait.torsoTwistRadians);
    const float sine = std::sin(gait.torsoTwistRadians);
    const auto basis = BuildPlayerModelWorldBasis(
        Unit(Add(Scale(bodyRight, cosine), Scale(bodyForward, -sine))),
        Unit(Add(Scale(bodyForward, cosine), Scale(bodyRight, sine))));
    const Vec3 root = GroundPlayerRootOnRouteFloor(
        Add(Add(eye, Scale(bodyRight, gait.pelvisSway)),
            Vec3{{0.0f, gait.pelvisBob * 0.65f, 0.0f}}),
        input.playerSupportWorldY, rig.BootGroundingOffsetMetres(animation));
    result.eye = eye;
    result.bodyRoot = root;
    result.bootGroundingOffset = rig.BootGroundingOffsetMetres(animation);
    result.leftShoulder = Add(eye, ViewToWorld(animation.leftIk.shoulder, right, viewUp, forward));
    result.rightShoulder = Add(eye, ViewToWorld(animation.rightIk.shoulder, right, viewUp, forward));
    const auto modelVector = [&](const Vec3& vector) {
        return WorldVectorToPlayerModel(basis,
            ViewToWorld(vector, right, viewUp, forward));
    };
    const auto toModelPoint = [&](const Vec3& point) {
        return WorldVectorToPlayerModel(basis,
            Add(Add(eye, ViewToWorld(point, right, viewUp, forward)), Scale(root, -1.0f)));
    };
    for (auto* arm : {&animation.leftIk, &animation.rightIk})
    {
        arm->shoulder = toModelPoint(arm->shoulder);
        arm->target = toModelPoint(arm->target);
        arm->pole = modelVector(arm->pole);
        arm->gripX = modelVector(arm->gripX);
        arm->gripY = modelVector(arm->gripY);
        arm->gripZ = modelVector(arm->gripZ);
    }

    bool poseUpdated = false;
    if (!rig.PreparePose(animation, tick, PlayerCpuSkinCadence::Hz60,
                         poseUpdated, diagnostic))
        return false;

    result.skinnedBodyBounds = {};
    for (const auto& vertex : rig.UniqueVertices())
    {
        const Vec3 model{{vertex.position[0], vertex.position[1], vertex.position[2]}};
        Include(result.skinnedBodyBounds,
                Add(root, PlayerModelVectorToWorld(basis, model)));
    }

    const auto& sockets = rig.BoneSockets();
    result.leftGrip = WorldSocket(sockets.leftGrip, basis, root);
    result.rightGrip = WorldSocket(sockets.rightGrip, basis, root);
    const auto leftHand = WorldSocket(sockets.leftHand, basis, root);
    const auto rightHand = WorldSocket(sockets.rightHand, basis, root);
    result.leftWrist = {{leftHand[12], leftHand[13], leftHand[14]}};
    result.rightWrist = {{rightHand[12], rightHand[13], rightHand[14]}};
    HeldItemTransform worldFromHips{};
    if (!rig.AnimatedHipsWorldTransform(animation, basis, root,
                                        worldFromHips, diagnostic) ||
        !ComposeWorldFromItem(worldFromHips, SwordBodyStowFromHips(),
                              result.bodyStowSword, diagnostic) ||
        !(staged != nullptr
            ? rig.ResolveHeldItemVisuals(items, result.leftGrip, result.rightGrip,
                                        result.bodyStowSword, result.renderItems, diagnostic)
            : rig.ResolveHeldItemVisuals(items, result.leftGrip, result.rightGrip,
                                        result.renderItems, diagnostic)))
        return false;
    return ComposeHeldLightState(result.renderItems[0].worldFromItem,
        PlayerRagTorchFlameSocketTransform(), PlayerRagTorchLightSocketTransform(),
        1.0f, result.light, diagnostic);
}

struct RewardLanternResult
{
    RewardLanternVisualTransforms visuals{};
    HeldItemTransform worldFromLanternBody{};
    HeldItemTransform worldFromFlame{};
    HeldItemTransform worldFromLight{};
};

bool ComposeRewardLantern(const RigResult& rig,
                          const horde::scene::assets::StaticMeshAsset& ringAsset,
                          const horde::scene::assets::StaticMeshAsset& bodyAsset,
                          RewardLanternResult& result,
                          std::string& diagnostic)
{
    const auto* gripRing = FindHeldItemSocket(ringAsset.sockets, "GripRing");
    const auto* hinge = FindHeldItemSocket(ringAsset.sockets, "Hinge");
    const auto* flame = FindHeldItemSocket(bodyAsset.sockets, "Flame");
    const auto* light = FindHeldItemSocket(bodyAsset.sockets, "Light");
    if (gripRing == nullptr || hinge == nullptr || flame == nullptr || light == nullptr)
    {
        diagnostic = "Loaded reward lantern assets are missing GripRing, Hinge, Flame, or Light sockets.";
        return false;
    }

    horde::gameplay::interactions::LanternPendulum pendulum;
    pendulum.Reset(rig.leftGrip);
    const auto& authored = pendulum.Snapshot();
    if (!ComposeClaimedRewardLanternVisuals(
            rig.leftGrip, gripRing->world, hinge->world, rig.leftGrip,
            authored.worldFromBody, kClaimedRewardLanternScale,
            result.visuals, diagnostic))
        return false;

    auto scale = IdentityHeldItemTransform();
    scale[0] = scale[5] = scale[10] = kClaimedRewardLanternScale;
    result.worldFromLanternBody = MultiplyHeldItemTransforms(
        result.visuals.worldFromBody, scale);
    result.worldFromFlame = MultiplyHeldItemTransforms(
        result.worldFromLanternBody, flame->world);
    result.worldFromLight = MultiplyHeldItemTransforms(
        result.worldFromLanternBody, light->world);
    return true;
}

bool RequireTranslated(const char* label,
                       const HeldItemTransform& floor,
                       const HeldItemTransform& raised,
                       const float deltaY)
{
    constexpr float tolerance = 0.001f;
    for (std::size_t element = 0u; element < 12u; ++element)
    {
        if (std::abs(floor[element] - raised[element]) > tolerance)
        {
            std::cerr << label << " rotation/shear changed at matrix element "
                      << element << '\n';
            return false;
        }
    }
    if (std::abs(floor[12] - raised[12]) > tolerance ||
        std::abs((raised[13] - floor[13]) - deltaY) > tolerance ||
        std::abs(floor[14] - raised[14]) > tolerance)
    {
        std::cerr << label << " did not preserve XZ and translate by "
                  << deltaY << " m in Y\n";
        return false;
    }
    return true;
}

bool RequireSameScalar(const char* label, const float floor, const float raised)
{
    constexpr float tolerance = 0.001f;
    if (std::abs(floor - raised) <= tolerance) return true;
    std::cerr << label << " changed across support translation: " << floor
              << " -> " << raised << '\n';
    return false;
}

bool RequireBoundsTranslated(const WorldBounds& floor,
                             const WorldBounds& raised,
                             const float deltaY)
{
    constexpr float tolerance = 0.001f;
    if (floor.points == 0u || floor.points != raised.points)
    {
        std::cerr << "Actual skinned body bounds have no matching vertex samples.\n";
        return false;
    }
    for (std::size_t axis : {0u, 2u})
    {
        if (std::abs(floor.minimum[axis] - raised.minimum[axis]) > tolerance ||
            std::abs(floor.maximum[axis] - raised.maximum[axis]) > tolerance)
        {
            std::cerr << "Actual skinned body world bounds changed horizontally.\n";
            return false;
        }
    }
    if (std::abs((raised.minimum[1] - floor.minimum[1]) - deltaY) > tolerance ||
        std::abs((raised.maximum[1] - floor.maximum[1]) - deltaY) > tolerance)
    {
        std::cerr << "Actual skinned body world bounds did not translate by "
                  << deltaY << " m in Y.\n";
        return false;
    }
    return true;
}

float ActualTorchHeadroom(const RigResult& solved,
                         const horde::scene::assets::StaticMeshAsset& torch,
                         const std::vector<OverheadTriangle>& realRoofTriangles)
{
    float minimum = std::numeric_limits<float>::infinity();
    for (const auto& vertex : torch.vertices)
    {
        const Vec3 local{{vertex.position[0], vertex.position[1], vertex.position[2]}};
        minimum = std::min(minimum,
            Headroom(TransformPoint(solved.renderItems[0].worldFromItem, local),
                     realRoofTriangles));
    }
    // Include the visible flame plus the exact 6 cm animated-tip margin and
    // lateral fire domain used by the existing real-mesh clearance regression.
    for (const float x : {-0.105f, 0.105f})
        for (const float z : {-0.105f, 0.105f})
            for (const float y : {0.0f, 0.4f})
                minimum = std::min(minimum, Headroom(
                    TransformPoint(solved.light.worldFromFlame, {{x, y, z}}),
                    realRoofTriangles));
    minimum = std::min(minimum,
        Headroom(TransformPoint(solved.light.worldFromLight, {{0.0f, 0.0f, 0.0f}}),
                 realRoofTriangles));
    return minimum;
}
// The exact GPU-failing pose needs the production snapshot, not default
// hand-held equipment or a recreated animation with different stow authority.
bool SolveStaged(PlayerRenderSlot& rig,
                 const horde::gameplay::simulation::SimulationSnapshot& snapshot,
                 const std::uint64_t poseTick, RigResult& result, std::string& diagnostic)
{
    HeldItemFixedStepInput input;
    input.playerX = snapshot.playerX;
    input.playerZ = snapshot.playerZ;
    input.playerYawRadians = snapshot.playerYawRadians;
    input.playerPitchRadians = snapshot.playerPitchRadians;
    input.walkTime = snapshot.walkTime;
    input.walkAmount = snapshot.walkAmount;
    input.playerSupportWorldY = snapshot.playerSupportWorldY;
    input.playerMountProfile = snapshot.playerMountProfile;
    return Solve(rig, input, poseTick, result, diagnostic, &snapshot);
}

bool TransformsMatch(const HeldItemTransform& a, const HeldItemTransform& b)
{
    for (std::size_t i = 0u; i < a.size(); ++i)
        if (!std::isfinite(a[i]) || !std::isfinite(b[i]) || std::abs(a[i] - b[i]) > 0.001f)
            return false;
    return true;
}

// Positive and corrupted cases share this validator; no zero-pixel exemption.
bool ValidateStagedRig(const horde::gameplay::simulation::SimulationSnapshot& snapshot,
                       const RigResult& solved,
                       const horde::scene::assets::StaticMeshAsset& torch,
                       const std::vector<OverheadTriangle>& roof, std::string& diagnostic)
{
    const auto fail = [&](const char* reason) { diagnostic = reason; return false; };
    if (solved.skinnedBodyBounds.points == 0u) return fail("missing skinned geometry");
    for (std::size_t axis = 0u; axis < 3u; ++axis)
        if (!std::isfinite(solved.skinnedBodyBounds.minimum[axis]) ||
            !std::isfinite(solved.skinnedBodyBounds.maximum[axis]) ||
            solved.skinnedBodyBounds.maximum[axis] < solved.skinnedBodyBounds.minimum[axis])
            return fail("nonfinite or inverted skinned geometry");
    if (!std::isfinite(solved.eye[1]) || !std::isfinite(solved.bodyRoot[1]) ||
        std::abs(solved.eye[1] - (snapshot.playerSupportWorldY + 1.65f)) > 0.001f ||
        std::abs(solved.bodyRoot[1] - snapshot.playerSupportWorldY - solved.bootGroundingOffset) > 0.001f ||
        std::abs(solved.skinnedBodyBounds.minimum[1] - snapshot.playerSupportWorldY -
                 kPlayerBootGroundingSafetyMetres) > 0.001f)
        return fail("body mesh or eye applied support incorrectly");
    for (const auto& pair : {std::array<Vec3, 2u>{{solved.leftShoulder, solved.leftWrist}},
                             std::array<Vec3, 2u>{{solved.rightShoulder, solved.rightWrist}}})
    {
        const auto distance = Add(pair[1], Scale(pair[0], -1.0f));
        if (!std::isfinite(Dot(distance, distance)) ||
            std::sqrt(Dot(distance, distance)) > kPlayerAnatomicalHandReachLimitMetres + 0.001f)
            return fail("actual wrist is outside anatomical reach");
    }
    if (!ValidateHeldItemState(solved.renderItems[0]) ||
        !TransformsMatch(MultiplyHeldItemTransforms(solved.renderItems[0].worldFromItem,
                                                   PlayerRagTorchGripSocketTransform()), solved.leftGrip))
        return fail("torch no longer attaches to the solved rig Grip");
    if (!ValidateHeldItemState(solved.renderItems[1]) ||
        !TransformsMatch(solved.renderItems[1].worldFromItem, solved.bodyStowSword))
        return fail("production stowed sword no longer attaches to animated Hips");
    if (!solved.light.active ||
        !TransformsMatch(solved.light.worldFromFlame,
            MultiplyHeldItemTransforms(solved.renderItems[0].worldFromItem,
                                      PlayerRagTorchFlameSocketTransform())) ||
        !TransformsMatch(solved.light.worldFromLight,
            MultiplyHeldItemTransforms(solved.renderItems[0].worldFromItem,
                                      PlayerRagTorchLightSocketTransform())))
        return fail("flame or physical light left its authored item socket");
    const float headroom = ActualTorchHeadroom(solved, torch, roof);
    if (!std::isfinite(headroom) || headroom < kHeldTorchOverheadGap - 1.0e-5f)
        return fail("actual torch mesh/flame/light violates roof clearance");
    diagnostic.clear();
    return true;
}

bool ValidateRecovery(const RigResult& returned, const RigResult& ordinary, std::string& diagnostic)
{
    if (!TransformsMatch(returned.leftGrip, ordinary.leftGrip) ||
        !TransformsMatch(returned.rightGrip, ordinary.rightGrip) ||
        !TransformsMatch(returned.bodyStowSword, ordinary.bodyStowSword) ||
        !TransformsMatch(returned.renderItems[0].worldFromItem, ordinary.renderItems[0].worldFromItem) ||
        !TransformsMatch(returned.light.worldFromFlame, ordinary.light.worldFromFlame) ||
        !TransformsMatch(returned.light.worldFromLight, ordinary.light.worldFromLight) ||
        returned.skinnedBodyBounds.points != ordinary.skinnedBodyBounds.points)
    { diagnostic = "returned pose retains geometry/socket/light state absent from ordinary control"; return false; }
    for (std::size_t axis = 0u; axis < 3u; ++axis)
        if (std::abs(returned.bodyRoot[axis] - ordinary.bodyRoot[axis]) > 0.001f ||
            std::abs(returned.skinnedBodyBounds.minimum[axis] - ordinary.skinnedBodyBounds.minimum[axis]) > 0.001f ||
            std::abs(returned.skinnedBodyBounds.maximum[axis] - ordinary.skinnedBodyBounds.maximum[axis]) > 0.001f)
        { diagnostic = "returned actual body bounds differ from ordinary control"; return false; }
    diagnostic.clear();
    return true;
}

bool CheckExactRaisedPose(PlayerRenderSlot& rig,
                          const horde::scene::assets::StaticMeshAsset& torch,
                          const std::vector<OverheadTriangle>& roof, std::string& diagnostic)
{
    using namespace horde::gameplay::simulation;
    const auto* checkpoint = FindDevelopmentCheckpoint(171);
    GameSimulation raised(ProductionGameSimulationConfig());
    GameSimulation ordinary(ProductionGameSimulationConfig());
    raised.SetPresentationAspect(16.0f / 9.0f);
    ordinary.SetPresentationAspect(16.0f / 9.0f);
    if (checkpoint == nullptr || !StageDevelopmentCheckpointSimulation(raised, *checkpoint) ||
        !StageDevelopmentCheckpointSimulation(ordinary, *checkpoint))
    { diagnostic = "exact production checkpoint171 staging failed"; return false; }
    ordinary.SetDevelopmentSupportFixture(false, ordinary.Snapshot().playerSupportGeneration + 1u);
    InputSnapshot input;
    input.damageEnabled = false;
    input.pitchRadians = checkpoint->pitch;
    input.yawRadians = checkpoint->yaw;
    ordinary.StepFixed(input, 0.0f);
    const auto staged = raised.Snapshot();
    if (staged.playerX != 0.0f || staged.playerZ != 0.0f || staged.playerYawRadians != 0.0f ||
        staged.playerPitchRadians != -0.05f || !staged.playerGrounded ||
        std::abs(staged.playerSupportWorldY - (-0.60f)) > 1.0e-5f ||
        std::abs(staged.playerHeightDelta - 0.35f) > 1.0e-5f ||
        staged.heldItems[1].parentMode != HeldItemParentMode::BodyStow ||
        staged.heldItems[1].visualStowBlend != 1.0f ||
        staged.heldItems[1].visualGripBlend != 0.0f)
    { diagnostic = "checkpoint171 did not retain exact support/pose/production stowed equipment"; return false; }
    RigResult constrained{}, floorControl{};
    if (!SolveStaged(rig, staged, 1000u, constrained, diagnostic) ||
        !ValidateStagedRig(staged, constrained, torch, roof, diagnostic) ||
        !SolveStaged(rig, ordinary.Snapshot(), 1001u, floorControl, diagnostic) ||
        !ValidateStagedRig(ordinary.Snapshot(), floorControl, torch, roof, diagnostic))
        return false;
    if (constrained.fixed.kinematics.torchOverheadLowering <= 0.0f &&
        constrained.fixed.kinematics.torchOverheadRetraction <= 0.0f)
    { diagnostic = "yaw0 raised pose did not exercise shared ceiling clearance"; return false; }

    std::size_t negativeCases = 0u;
    const auto rejected = [&](const RigResult& corrupted, const char* label) {
        std::string reason;
        if (ValidateStagedRig(staged, corrupted, torch, roof, reason))
        { diagnostic = std::string("exact-pose validator admitted negative: ") + label; return false; }
        ++negativeCases;
        std::cout << "Exact-pose rejected " << label << ": " << reason << '\n';
        return true;
    };
    auto corrupted = constrained;
    corrupted.skinnedBodyBounds.points = 0u;
    if (!rejected(corrupted, "missing geometry")) return false;
    for (const float delta : {-0.35f, 0.35f})
    {
        corrupted = constrained;
        corrupted.bodyRoot[1] += delta;
        corrupted.eye[1] += delta;
        corrupted.skinnedBodyBounds.minimum[1] += delta;
        corrupted.skinnedBodyBounds.maximum[1] += delta;
        if (!rejected(corrupted, delta < 0.0f ? "omitted support" : "doubled support")) return false;
    }
    corrupted = constrained; corrupted.leftWrist[1] += 2.0f;
    if (!rejected(corrupted, "unreachable wrist")) return false;
    corrupted = constrained; corrupted.renderItems[0].worldFromItem[12] += 0.02f;
    if (!rejected(corrupted, "broken torch Grip")) return false;
    corrupted = constrained; corrupted.renderItems[1].worldFromItem[12] += 0.02f;
    if (!rejected(corrupted, "broken sword Hips attachment")) return false;
    corrupted = constrained; corrupted.light.worldFromFlame[13] += 0.02f;
    if (!rejected(corrupted, "independent flame offset")) return false;
    corrupted = constrained; corrupted.light.worldFromLight[13] += 0.02f;
    if (!rejected(corrupted, "independent physical-light offset")) return false;

    // Independent production simulation follows identical planar commands with
    // the fixture disabled. Same gait phase, equipment and input; no copied pose.
    input.moveForward = -1.0f;
    for (std::uint32_t tick = 0u; tick < 59u; ++tick)
    { raised.StepFixed(input); ordinary.StepFixed(input); }
    const auto returnedSnapshot = raised.Snapshot();
    if (returnedSnapshot.playerSupportWorldY != kRouteFloorWorldY ||
        returnedSnapshot.playerX != ordinary.Snapshot().playerX ||
        returnedSnapshot.playerZ != ordinary.Snapshot().playerZ)
    { diagnostic = "fixed-step descent did not return to matched ordinary floor"; return false; }
    RigResult returned{}, returnedControl{};
    if (!SolveStaged(rig, returnedSnapshot, 1002u, returned, diagnostic) ||
        !ValidateStagedRig(returnedSnapshot, returned, torch, roof, diagnostic) ||
        !SolveStaged(rig, ordinary.Snapshot(), 1003u, returnedControl, diagnostic) ||
        !ValidateStagedRig(ordinary.Snapshot(), returnedControl, torch, roof, diagnostic) ||
        !ValidateRecovery(returned, returnedControl, diagnostic))
        return false;
    if (returned.fixed.kinematics.torchOverheadLowering >=
            constrained.fixed.kinematics.torchOverheadLowering)
    { diagnostic = "returned floor did not release elevated ceiling lowering"; return false; }
    std::string staleDiagnostic;
    if (ValidateRecovery(constrained, returnedControl, staleDiagnostic))
    { diagnostic = "recovery validator admitted the stale raised pose"; return false; }
    ++negativeCases;
    std::cout << "Exact checkpoint171 production rig yaw0 pitch-0.05 aspect16:9 passed; "
              << "mesh points=" << constrained.skinnedBodyBounds.points
              << " lowering/retraction/headroom=" << constrained.fixed.kinematics.torchOverheadLowering
              << '/' << constrained.fixed.kinematics.torchOverheadRetraction
              << '/' << ActualTorchHeadroom(constrained, torch, roof)
              << " returned lowering=" << returned.fixed.kinematics.torchOverheadLowering
              << " same-validator negatives=" << negativeCases << " descent ticks=59\n";
    return true;
}
} // namespace

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Pass the repository root so the real cached runtime player rig can load.\n";
        return 2;
    }
    const std::filesystem::path root{argv[1]};
    PlayerRenderSlot rig;
    std::string diagnostic;
    if (!rig.LoadAsset((root / "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb").string(),
                       diagnostic))
    {
        std::cerr << "Could not load the cached runtime player rig: " << diagnostic << '\n';
        return 2;
    }

    const auto loadStatic = [&](const std::filesystem::path& directory,
                                const std::filesystem::path& filename,
                                horde::scene::assets::StaticMeshAsset& asset) {
        horde::scene::assets::AssetManifest manifest;
        return horde::scene::assets::AssetManifest::Load(
                   directory / "asset.manifest.json", manifest, diagnostic) &&
               horde::scene::assets::StaticMeshAsset::Load(
                   directory / filename, manifest, asset, diagnostic);
    };
    horde::scene::assets::StaticMeshAsset playerTorch;
    horde::scene::assets::StaticMeshAsset lanternRing;
    horde::scene::assets::StaticMeshAsset lanternBody;
    horde::scene::assets::StaticMeshAsset collapsedEntry;
    if (!loadStatic(root / "assets/models/props/runtime/player-rag-torch",
                    "rag-torch-player-lod0.runtime.glb", playerTorch) ||
        !loadStatic(root / "assets/models/props/runtime/reward-lantern-ring",
                    "reward-lantern-ring-lod0.runtime.glb", lanternRing) ||
        !loadStatic(root / "assets/models/props/runtime/reward-lantern-body",
                    "reward-lantern-body-lod0.runtime.glb", lanternBody) ||
        !loadStatic(root / "assets/models/world/runtime/collapsed-entry",
                    "collapsed-entry-lod0.runtime.glb", collapsedEntry))
    {
        std::cerr << "Could not load cached torch, reward-lantern, or roof assets: "
                  << diagnostic << '\n';
        return 2;
    }
    const auto roofTriangles = ImportedStructuralUndersides(collapsedEntry);
    if (roofTriangles.empty())
    {
        std::cerr << "Cached collapsed-entry asset has no imported roof underside samples.\n";
        return 2;
    }

    constexpr float supportDelta = 0.35f;
    HeldItemFixedStepInput floorInput;
    floorInput.playerX = 0.0f;
    floorInput.playerZ = -12.0f;
    floorInput.playerYawRadians = 0.0f;
    floorInput.playerPitchRadians = -0.05f;
    floorInput.playerMountProfile = PlayerMountProfile::AnatomicalBody;
    floorInput.torchFailure.heldByPlayer = true;
    floorInput.playerSupportWorldY = kRouteFloorWorldY;
    HeldItemFixedStepInput raisedInput = floorInput;
    raisedInput.playerSupportWorldY += supportDelta;

    RigResult floor{};
    RigResult raised{};
    if (!Solve(rig, floorInput, 1u, floor, diagnostic) ||
        !Solve(rig, raisedInput, 2u, raised, diagnostic))
    {
        std::cerr << "Actual player rig/socket solve failed: " << diagnostic << '\n';
        return 1;
    }

    HeldItemFixedStepInput floorLanternInput = floorInput;
    floorLanternInput.interaction.heldLightKind =
        horde::gameplay::interactions::HeldLightKind::RewardLantern;
    HeldItemFixedStepInput raisedLanternInput = floorLanternInput;
    raisedLanternInput.playerSupportWorldY += supportDelta;
    RigResult floorLantern{};
    RigResult raisedLantern{};
    if (!Solve(rig, floorLanternInput, 3u, floorLantern, diagnostic) ||
        !Solve(rig, raisedLanternInput, 4u, raisedLantern, diagnostic))
    {
        std::cerr << "Actual reward-lantern player rig/socket solve failed: "
                  << diagnostic << '\n';
        return 1;
    }

    RewardLanternResult floorRewardLantern{};
    RewardLanternResult raisedRewardLantern{};
    if (!ComposeRewardLantern(floorLantern, lanternRing, lanternBody,
                              floorRewardLantern, diagnostic) ||
        !ComposeRewardLantern(raisedLantern, lanternRing, lanternBody,
                              raisedRewardLantern, diagnostic))
    {
        std::cerr << "Actual reward-lantern socket composition failed: "
                  << diagnostic << '\n';
        return 1;
    }

    // First roof fixture at player X/Z=(0,0), facing into the overhead patch.
    // Check both support levels against real Rag vertices and imported masonry.
    HeldItemFixedStepInput fixtureFloorInput = floorInput;
    fixtureFloorInput.playerX = 0.0f;
    fixtureFloorInput.playerZ = 0.0f;
    fixtureFloorInput.playerYawRadians = 3.14159265f;
    HeldItemFixedStepInput fixtureRaisedInput = fixtureFloorInput;
    fixtureRaisedInput.playerSupportWorldY += supportDelta;
    RigResult fixtureFloor{};
    RigResult fixtureRaised{};
    if (!Solve(rig, fixtureFloorInput, 5u, fixtureFloor, diagnostic) ||
        !Solve(rig, fixtureRaisedInput, 6u, fixtureRaised, diagnostic))
    {
        std::cerr << "First fixture real-rig clearance solve failed: "
                  << diagnostic << '\n';
        return 1;
    }
    const float floorFixtureHeadroom = ActualTorchHeadroom(
        fixtureFloor, playerTorch, roofTriangles);
    const float raisedFixtureHeadroom = ActualTorchHeadroom(
        fixtureRaised, playerTorch, roofTriangles);

    bool ok = true;
    const auto requireLocalUnchanged = [&](const char* label,
                                           const Vec3& from,
                                           const Vec3& to) {
        for (std::size_t axis = 0u; axis < 3u; ++axis)
            if (std::abs(from[axis] - to[axis]) > 0.001f)
            {
                std::cerr << label << " local pose changed at axis " << axis << '\n';
                return false;
            }
        return true;
    };
    ok &= requireLocalUnchanged("left hand", floor.fixed.kinematics.leftHandLocal,
                                raised.fixed.kinematics.leftHandLocal);
    ok &= requireLocalUnchanged("right hand", floor.fixed.kinematics.rightHandLocal,
                                raised.fixed.kinematics.rightHandLocal);
    ok &= requireLocalUnchanged("reward-lantern left hand",
                                floorLantern.fixed.kinematics.leftHandLocal,
                                raisedLantern.fixed.kinematics.leftHandLocal);
    ok &= RequireSameScalar("torch overhead lowering",
        floor.fixed.kinematics.torchOverheadLowering,
        raised.fixed.kinematics.torchOverheadLowering);
    ok &= RequireSameScalar("torch overhead retraction",
        floor.fixed.kinematics.torchOverheadRetraction,
        raised.fixed.kinematics.torchOverheadRetraction);
    ok &= RequireSameScalar("sword overhead lowering",
        floor.fixed.kinematics.swordOverheadLowering,
        raised.fixed.kinematics.swordOverheadLowering);
    ok &= RequireSameScalar("sword overhead retraction",
        floor.fixed.kinematics.swordOverheadRetraction,
        raised.fixed.kinematics.swordOverheadRetraction);
    ok &= RequireTranslated("left rig Grip socket", floor.leftGrip, raised.leftGrip, supportDelta);
    ok &= RequireTranslated("right rig Grip socket", floor.rightGrip, raised.rightGrip, supportDelta);
    ok &= RequireTranslated("torch item", floor.renderItems[0].worldFromItem,
                            raised.renderItems[0].worldFromItem, supportDelta);
    ok &= RequireTranslated("sword item", floor.renderItems[1].worldFromItem,
                            raised.renderItems[1].worldFromItem, supportDelta);
    ok &= RequireTranslated("torch flame socket", floor.light.worldFromFlame,
                            raised.light.worldFromFlame, supportDelta);
    ok &= RequireTranslated("torch direct-light socket", floor.light.worldFromLight,
                            raised.light.worldFromLight, supportDelta);
    ok &= RequireTranslated("body-stowed sword helper", floor.bodyStowSword,
                            raised.bodyStowSword, supportDelta);
    ok &= RequireTranslated("reward-lantern left rig Grip socket",
                            floorLantern.leftGrip, raisedLantern.leftGrip,
                            supportDelta);
    ok &= RequireTranslated("reward-lantern sword item",
                            floorLantern.renderItems[1].worldFromItem,
                            raisedLantern.renderItems[1].worldFromItem,
                            supportDelta);
    ok &= RequireBoundsTranslated(floor.skinnedBodyBounds,
                                  raised.skinnedBodyBounds, supportDelta);
    ok &= RequireTranslated("reward-lantern ring hinge",
                            floorRewardLantern.visuals.worldFromHinge,
                            raisedRewardLantern.visuals.worldFromHinge,
                            supportDelta);
    ok &= RequireTranslated("reward-lantern pendulum body",
                            floorRewardLantern.worldFromLanternBody,
                            raisedRewardLantern.worldFromLanternBody,
                            supportDelta);
    ok &= RequireTranslated("reward-lantern physical flame socket",
                            floorRewardLantern.worldFromFlame,
                            raisedRewardLantern.worldFromFlame,
                            supportDelta);
    ok &= RequireTranslated("reward-lantern physical light socket",
                            floorRewardLantern.worldFromLight,
                            raisedRewardLantern.worldFromLight,
                            supportDelta);
    if (fixtureRaised.fixed.kinematics.torchOverheadRetraction <= 0.0f)
    {
        std::cerr << "First fixture at (0,0) did not exercise elevated torch overhead retraction.\n";
        ok = false;
    }
    if (floorFixtureHeadroom < kHeldTorchOverheadGap - 1.0e-5f ||
        raisedFixtureHeadroom < kHeldTorchOverheadGap - 1.0e-5f)
    {
        std::cerr << "First fixture Rag mesh/flame/light penetrates real roof clearance: floor="
                  << floorFixtureHeadroom << " m raised=" << raisedFixtureHeadroom << " m\n";
        ok = false;
    }

    if (!ok) return 1;
    if (!CheckExactRaisedPose(rig, playerTorch, roofTriangles, diagnostic))
    {
        std::cerr << "Exact checkpoint171 regression failed: " << diagnostic << '\n';
        return 1;
    }
    std::cout << "Actual cached player rig support transform passed: support delta="
              << supportDelta << " m; actual skinned body vertices, hand/item sockets, reward-lantern hinge/body/flame/light, and torch clearance translated with XZ preserved.\n"
              << "First fixture (0,0) torch retraction/headroom floor="
              << fixtureFloor.fixed.kinematics.torchOverheadRetraction << '/'
              << floorFixtureHeadroom << " m raised="
              << fixtureRaised.fixed.kinematics.torchOverheadRetraction << '/'
              << raisedFixtureHeadroom << " m\n";
    return 0;
}
