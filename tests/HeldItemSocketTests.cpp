#include "gameplay/items/HeldItemKinematics.h"
#include "gameplay/ShowcaseRoute.h"
#include "gameplay/items/HeldLightState.h"
#include "gameplay/items/HeldItemState.h"
#include "gameplay/interactions/InteractionState.h"
#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/SwordCombat.h"
#include "vulkan/raytracing/HeldItemRenderSlot.h"
#include "vulkan/raytracing/HeldItemBlasMeasurements.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"
#include "vulkan/raytracing/RtSceneAbi.generated.h"
#include "vulkan/raytracing/RtStaticMeshSlot.h"
#include "scene/ShowcaseOverheadGeometry.h"
#include "scene/assets/PlayerPrimitiveContract.h"
#include "scene/assets/SkinnedMeshAsset.h"

#include <algorithm>
#include <array>
#include <numeric>
#if defined(_MSC_VER)
#include <crtdbg.h>
#endif
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{

using horde::gameplay::items::HeldHand;
using horde::gameplay::items::HeldItemId;
using horde::gameplay::items::HeldItemParentMode;
using horde::gameplay::items::HeldItemState;
using horde::gameplay::items::HeldItemTransform;

int failures = 0;

void Check(const bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

bool Near(const float actual, const float expected, const float tolerance = 0.0001f)
{
    return std::abs(actual - expected) <= tolerance;
}

HeldItemTransform Translation(const float x, const float y, const float z)
{
    HeldItemTransform result = horde::gameplay::items::IdentityHeldItemTransform();
    result[12] = x;
    result[13] = y;
    result[14] = z;
    return result;
}

std::array<float, 3u> Add(const std::array<float, 3u>& left,
                          const std::array<float, 3u>& right)
{
    return {{left[0] + right[0], left[1] + right[1], left[2] + right[2]}};
}

std::array<float, 3u> Scale(const std::array<float, 3u>& value, const float scale)
{
    return {{value[0] * scale, value[1] * scale, value[2] * scale}};
}

std::array<float, 3u> Cross(const std::array<float, 3u>& left,
                            const std::array<float, 3u>& right)
{
    return {{left[1] * right[2] - left[2] * right[1],
             left[2] * right[0] - left[0] * right[2],
             left[0] * right[1] - left[1] * right[0]}};
}

std::array<float, 3u> Normalize(const std::array<float, 3u>& value)
{
    const float length = std::sqrt(value[0] * value[0] + value[1] * value[1] +
                                   value[2] * value[2]);
    return Scale(value, 1.0f / length);
}

bool TransformNear(const HeldItemTransform& actual,
                   const HeldItemTransform& expected,
                   const float tolerance = 0.00001f)
{
    for (std::size_t i = 0u; i < actual.size(); ++i)
    {
        if (!Near(actual[i], expected[i], tolerance)) return false;
    }
    return true;
}

std::array<float, 3u> TransformPoint(const HeldItemTransform& transform,
                                    const std::array<float, 3u>& point)
{
    return {{transform[0] * point[0] + transform[4] * point[1] + transform[8] * point[2] + transform[12],
             transform[1] * point[0] + transform[5] * point[1] + transform[9] * point[2] + transform[13],
             transform[2] * point[0] + transform[6] * point[1] + transform[10] * point[2] + transform[14]}};
}

float Dot3(const std::array<float, 3u>& left,
           const std::array<float, 3u>& right)
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

std::array<float, 3u> Subtract3(const std::array<float, 3u>& left,
                               const std::array<float, 3u>& right)
{
    return {{left[0] - right[0], left[1] - right[1], left[2] - right[2]}};
}

float PointTriangleDistanceSquared(const std::array<float, 3u>& point,
                                   const std::array<std::array<float, 3u>, 3u>& triangle)
{
    const auto ab = Subtract3(triangle[1], triangle[0]);
    const auto ac = Subtract3(triangle[2], triangle[0]);
    const auto ap = Subtract3(point, triangle[0]);
    const float d1 = Dot3(ab, ap);
    const float d2 = Dot3(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return Dot3(ap, ap);
    const auto bp = Subtract3(point, triangle[1]);
    const float d3 = Dot3(ab, bp);
    const float d4 = Dot3(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return Dot3(bp, bp);
    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        const float v = d1 / (d1 - d3);
        const auto delta = Subtract3(point, Add(triangle[0], Scale(ab, v)));
        return Dot3(delta, delta);
    }
    const auto cp = Subtract3(point, triangle[2]);
    const float d5 = Dot3(ab, cp);
    const float d6 = Dot3(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return Dot3(cp, cp);
    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        const float w = d2 / (d2 - d6);
        const auto delta = Subtract3(point, Add(triangle[0], Scale(ac, w)));
        return Dot3(delta, delta);
    }
    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && d4 - d3 >= 0.0f && d5 - d6 >= 0.0f)
    {
        const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        const auto delta = Subtract3(point, Add(triangle[1], Scale(
            Subtract3(triangle[2], triangle[1]), w)));
        return Dot3(delta, delta);
    }
    const float inverse = 1.0f / (va + vb + vc);
    const float v = vb * inverse;
    const float w = vc * inverse;
    const auto closest = Add(Add(triangle[0], Scale(ab, v)), Scale(ac, w));
    const auto delta = Subtract3(point, closest);
    return Dot3(delta, delta);
}

float SegmentSegmentDistanceSquared(const std::array<float, 3u>& p1,
                                    const std::array<float, 3u>& q1,
                                    const std::array<float, 3u>& p2,
                                    const std::array<float, 3u>& q2)
{
    const auto d1 = Subtract3(q1, p1);
    const auto d2 = Subtract3(q2, p2);
    const auto r = Subtract3(p1, p2);
    const float a = Dot3(d1, d1);
    const float e = Dot3(d2, d2);
    const float f = Dot3(d2, r);
    float s = 0.0f;
    float t = 0.0f;
    if (a <= 1.0e-12f && e <= 1.0e-12f) return Dot3(r, r);
    if (a <= 1.0e-12f) t = std::clamp(f / e, 0.0f, 1.0f);
    else
    {
        const float c = Dot3(d1, r);
        if (e <= 1.0e-12f) s = std::clamp(-c / a, 0.0f, 1.0f);
        else
        {
            const float b = Dot3(d1, d2);
            const float denominator = a * e - b * b;
            if (denominator > 1.0e-12f)
                s = std::clamp((b * f - c * e) / denominator, 0.0f, 1.0f);
            t = (b * s + f) / e;
            if (t < 0.0f) { t = 0.0f; s = std::clamp(-c / a, 0.0f, 1.0f); }
            else if (t > 1.0f) { t = 1.0f; s = std::clamp((b - c) / a, 0.0f, 1.0f); }
        }
    }
    const auto delta = Subtract3(Add(p1, Scale(d1, s)), Add(p2, Scale(d2, t)));
    return Dot3(delta, delta);
}

bool SegmentIntersectsTriangle(const std::array<float, 3u>& start,
                               const std::array<float, 3u>& end,
                               const std::array<std::array<float, 3u>, 3u>& triangle)
{
    const auto direction = Subtract3(end, start);
    const auto edge1 = Subtract3(triangle[1], triangle[0]);
    const auto edge2 = Subtract3(triangle[2], triangle[0]);
    const auto p = Cross(direction, edge2);
    const float determinant = Dot3(edge1, p);
    if (std::abs(determinant) < 1.0e-8f) return false;
    const float inverse = 1.0f / determinant;
    const auto tvec = Subtract3(start, triangle[0]);
    const float u = Dot3(tvec, p) * inverse;
    if (u < 0.0f || u > 1.0f) return false;
    const auto q = Cross(tvec, edge1);
    const float v = Dot3(direction, q) * inverse;
    if (v < 0.0f || u + v > 1.0f) return false;
    const float t = Dot3(edge2, q) * inverse;
    return t >= 0.0f && t <= 1.0f;
}

float TriangleDistanceSquared(
    const std::array<std::array<float, 3u>, 3u>& left,
    const std::array<std::array<float, 3u>, 3u>& right)
{
    float best = std::numeric_limits<float>::infinity();
    for (std::size_t i = 0u; i < 3u; ++i)
    {
        best = std::min(best, PointTriangleDistanceSquared(left[i], right));
        best = std::min(best, PointTriangleDistanceSquared(right[i], left));
        const std::size_t next = (i + 1u) % 3u;
        if (SegmentIntersectsTriangle(left[i], left[next], right) ||
            SegmentIntersectsTriangle(right[i], right[next], left)) return 0.0f;
        for (std::size_t j = 0u; j < 3u; ++j)
        {
            const std::size_t nextRight = (j + 1u) % 3u;
            best = std::min(best, SegmentSegmentDistanceSquared(
                left[i], left[next], right[j], right[nextRight]));
        }
    }
    return best;
}

std::array<float, 3u> ExpectedWorldHandPoint(
    const horde::gameplay::items::HeldItemFixedStepInput& input,
    const std::array<float, 3u>& local)
{
    const std::array<float, 3u> up{{0.0f, 1.0f, 0.0f}};
    const float pitch = std::clamp(input.playerPitchRadians, -0.32f, 0.28f);
    const auto forward = Normalize(std::array<float, 3u>{{
        std::sin(input.playerYawRadians), -0.05f + pitch,
        -std::cos(input.playerYawRadians)}});
    const auto right = Normalize(Cross(forward, up));
    const auto viewUp = Normalize(Cross(right, forward));
    return Add(Add(Add({{input.playerX, horde::gameplay::kShowcaseEyeWorldY,
                         input.playerZ}}, Scale(right, local[0])),
                   Scale(viewUp, local[1])),
               Scale(forward, local[2]));
}

bool PointInsideOverheadFootprint(const horde::scene::OverheadVolume& volume,
                                  const float x,
                                  const float z)
{
    bool positive = false;
    bool negative = false;
    for (std::size_t edge = 0u; edge < volume.footprint.size(); ++edge)
    {
        const auto& start = volume.footprint[edge];
        const auto& end = volume.footprint[(edge + 1u) % volume.footprint.size()];
        const float side = (end[0] - start[0]) * (z - start[1]) -
                           (end[1] - start[1]) * (x - start[0]);
        positive |= side > 0.000001f;
        negative |= side < -0.000001f;
    }
    return !(positive && negative);
}

HeldItemTransform PlayerGripToWorld(
    const horde::scene::SkinnedNodeTransform& bone,
    const horde::vulkan::raytracing::PlayerModelWorldBasis& basis,
    const std::array<float, 3u>& root)
{
    const auto vectorToWorld = [&basis](const std::array<float, 3u>& local) {
        return horde::vulkan::raytracing::PlayerModelVectorToWorld(basis, local);
    };
    auto x = Normalize(vectorToWorld({{bone[0], bone[1], bone[2]}}));
    const auto rawY = vectorToWorld({{bone[4], bone[5], bone[6]}});
    const float projection = x[0] * rawY[0] + x[1] * rawY[1] + x[2] * rawY[2];
    auto y = Normalize(Add(rawY, Scale(x, -projection)));
    auto z = Normalize(Cross(x, y));
    const auto rawZ = vectorToWorld({{bone[8], bone[9], bone[10]}});
    if (z[0] * rawZ[0] + z[1] * rawZ[1] + z[2] * rawZ[2] < 0.0f)
    {
        y = Scale(y, -1.0f);
        z = Scale(z, -1.0f);
    }
    const auto localPosition = vectorToWorld({{bone[12], bone[13], bone[14]}});
    const auto position = Add(root, localPosition);
    HeldItemTransform result = horde::gameplay::items::IdentityHeldItemTransform();
    for (std::size_t axis = 0u; axis < 3u; ++axis)
    {
        result[axis] = x[axis];
        result[4u + axis] = y[axis];
        result[8u + axis] = z[axis];
        result[12u + axis] = position[axis];
    }
    return result;
}

HeldItemTransform ExpectedHeldTorchFromSharedPose(
    const horde::gameplay::simulation::SimulationSnapshot& snapshot,
    const HeldItemTransform& itemFromGrip)
{
    const std::array<float, 3u> worldUp{{0.0f, 1.0f, 0.0f}};
    const auto forward = Normalize(std::array<float, 3u>{{
        std::sin(snapshot.playerYawRadians),
        -0.05f + std::clamp(snapshot.playerPitchRadians, -0.32f, 0.28f),
        -std::cos(snapshot.playerYawRadians)}});
    const auto right = Normalize(Cross(forward, worldUp));
    const auto up = Normalize(Cross(right, forward));
    const auto& localHand = snapshot.heldItemKinematics.leftHandLocal;
    const std::array<float, 3u> eye{{
        snapshot.playerX, horde::gameplay::kShowcaseEyeWorldY,
        snapshot.playerZ}};
    const auto hand = Add(Add(Add(eye, Scale(right, localHand[0])),
                              Scale(up, localHand[1])),
                          Scale(forward, localHand[2]));
    constexpr float roll = 0.0f;
    const auto itemX = Add(Scale(right, std::cos(roll)),
                           Scale(forward, std::sin(roll)));
    const auto itemY = up;
    const auto itemZ = Add(Scale(right, std::sin(roll)),
                           Scale(forward, -std::cos(roll)));
    const auto translation = Add(
        Add(Add(hand, Scale(itemX, -itemFromGrip[12])),
            Scale(itemY, -itemFromGrip[13])),
        Scale(itemZ, -itemFromGrip[14]));
    return {{itemX[0], itemX[1], itemX[2], 0.0f,
             itemY[0], itemY[1], itemY[2], 0.0f,
             itemZ[0], itemZ[1], itemZ[2], 0.0f,
             translation[0], translation[1], translation[2], 1.0f}};
}

bool LoadProductionHeldAssets(horde::scene::assets::StaticMeshAsset& sword,
                              horde::scene::assets::StaticMeshAsset& torch,
                              std::string& diagnostic)
{
    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    horde::scene::assets::AssetManifest swordManifest;
    horde::scene::assets::AssetManifest torchManifest;
    const auto swordDirectory = root / "assets/models/weapons/runtime";
    const auto torchDirectory = root / "assets/models/props/runtime";
    return horde::scene::assets::AssetManifest::Load(
               swordDirectory / "asset.manifest.json", swordManifest, diagnostic) &&
           horde::scene::assets::StaticMeshAsset::Load(
               swordDirectory / "gothic-arming-sword-rh-lod0.runtime.glb",
               swordManifest, sword, diagnostic) &&
           horde::scene::assets::AssetManifest::Load(
               torchDirectory / "asset.manifest.json", torchManifest, diagnostic) &&
           horde::scene::assets::StaticMeshAsset::Load(
               torchDirectory / "gothic-hand-torch-lod0.runtime.glb",
               torchManifest, torch, diagnostic);
}

bool LoadPlayerRagTorch(horde::scene::assets::StaticMeshAsset& torch,
                        std::string& diagnostic)
{
    const std::filesystem::path directory =
        std::filesystem::path(HORDE_RT_SOURCE_DIR) /
        "assets/models/props/runtime/player-rag-torch";
    horde::scene::assets::AssetManifest manifest;
    return horde::scene::assets::AssetManifest::Load(
               directory / "asset.manifest.json", manifest, diagnostic) &&
           horde::scene::assets::StaticMeshAsset::Load(
               directory / "rag-torch-player-lod0.runtime.glb",
               manifest, torch, diagnostic);
}

void TestSocketLookupIsNamedAndOrderIndependent()
{
    const std::vector<horde::scene::assets::StaticSocket> sockets{
        {"Flame", 3u, Translation(0.0f, 0.8f, 0.0f)},
        {"Grip", 1u, Translation(0.0f, -0.2f, 0.0f)},
        {"Light", 4u, Translation(0.0f, 0.9f, 0.0f)},
    };
    const auto* grip = horde::gameplay::items::FindHeldItemSocket(sockets, "Grip");
    Check(grip != nullptr && grip->nodeTransformIndex == 1u && Near(grip->world[13], -0.2f),
          "Grip lookup must use the exact socket name rather than vector order");
    Check(horde::gameplay::items::FindHeldItemSocket(sockets, "grip") == nullptr,
          "socket names must retain the authored case-sensitive contract");
}

void TestWorldFromItemUsesRequiredCompositionOrder()
{
    HeldItemTransform worldFromRightHand{{
        0.0f, 1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        4.0f, 5.0f, 6.0f, 1.0f}};
    const HeldItemTransform itemFromGrip = Translation(0.0f, -2.0f, 0.0f);

    HeldItemTransform worldFromItem{};
    std::string diagnostic;
    Check(horde::gameplay::items::ComposeWorldFromItem(
              worldFromRightHand, itemFromGrip, worldFromItem, diagnostic),
          "a rigid hand and grip transform must compose");
    Check(Near(worldFromItem[12], 2.0f) && Near(worldFromItem[13], 5.0f) &&
              Near(worldFromItem[14], 6.0f),
          "worldFromItem must equal worldFromHandSocket * inverse(itemFromGrip)");
}

void TestScaledGripSocketIsRejected()
{
    HeldItemTransform scaledGrip = Translation(0.0f, -0.2f, 0.0f);
    scaledGrip[0] = 1.25f;
    std::string diagnostic;
    Check(!horde::gameplay::items::ValidateHeldItemSocketTransform(scaledGrip, diagnostic) &&
              diagnostic == "Held-item socket transform must be rigid and unit scale.",
          "scaled Grip sockets must be rejected before attachment");
}

void TestLeftAndRightHandsCannotBeSwapped()
{
    const HeldItemTransform left = Translation(-3.0f, 1.0f, 2.0f);
    const HeldItemTransform right = Translation(7.0f, 1.0f, 2.0f);
    const auto selectedLeft = horde::gameplay::items::SelectHandSocketTransform(
        HeldHand::LeftHand, left, right);
    const auto selectedRight = horde::gameplay::items::SelectHandSocketTransform(
        HeldHand::RightHand, left, right);
    Check(Near(selectedLeft[12], -3.0f) && Near(selectedRight[12], 7.0f),
          "LeftHand and RightHand must retain distinct transforms");
}

void TestWallRetractionMovesHandAndAttachedItemTogether()
{
    const HeldItemTransform grip = Translation(0.0f, -0.25f, 0.0f);
    const HeldItemTransform nominalHand = Translation(-0.34f, 0.18f, -1.05f);
    const HeldItemTransform retractedHand = Translation(-0.34f, 0.18f, -0.62f);
    HeldItemTransform nominalItem{};
    HeldItemTransform retractedItem{};
    std::string diagnostic;
    Check(horde::gameplay::items::ComposeWorldFromItem(
              nominalHand, grip, nominalItem, diagnostic) &&
              horde::gameplay::items::ComposeWorldFromItem(
                  retractedHand, grip, retractedItem, diagnostic),
          "wall retraction fixtures must compose");
    Check(Near(retractedHand[14] - nominalHand[14], 0.43f) &&
              Near(retractedItem[14] - nominalItem[14], 0.43f) &&
              Near(retractedItem[13] - retractedHand[13], 0.25f),
          "wall retraction must move the shared hand target, preserving Grip attachment");
}

void TestRealFixedTickTorchDetachIsIndependentlyTransformContinuous()
{
    horde::scene::assets::StaticMeshAsset torchAsset;
    std::string diagnostic;
    Check(LoadPlayerRagTorch(torchAsset, diagnostic),
          "player Rag torch must load for the fixed-tick detach contract");
    const auto* grip = horde::gameplay::items::FindHeldItemSocket(torchAsset.sockets, "Grip");
    Check(grip != nullptr, "player Rag torch Grip must exist for detach continuity");
    if (grip == nullptr) return;

    horde::gameplay::simulation::GameSimulation simulation;
    horde::gameplay::simulation::InputSnapshot input;
    input.hasAuthoritativePlayerPose = true;
    input.authoritativePlayerX = -2.16f;
    input.authoritativePlayerZ = -15.14f;
    input.yawRadians = 0.43f;
    input.pitchRadians = 0.17f;

    horde::gameplay::simulation::SimulationSnapshot beforeRelease{};
    horde::gameplay::simulation::SimulationSnapshot released{};
    for (std::size_t step = 0u; step < 80u; ++step)
    {
        const auto before = simulation.Snapshot();
        simulation.StepFixed(input);
        const auto after = simulation.Snapshot();
        if (after.heldItems[0].detached)
        {
            beforeRelease = before;
            released = after;
            break;
        }
    }

    Check(released.heldItems[0].detached &&
              released.heldItems[0].parentMode == HeldItemParentMode::AuthoredWorldTrajectory &&
              released.heldItems[0].detachTick == released.tickIndex,
          "the real torch failure sequence must detach once on its shared fixed tick");
    const HeldItemTransform expectedHeld = ExpectedHeldTorchFromSharedPose(
        beforeRelease, grip->world);
    Check(std::abs(expectedHeld[12] - (-2.16f - 0.24f)) > 0.001f &&
              std::abs(expectedHeld[13] -
                       (horde::gameplay::kShowcaseEyeWorldY - 0.34f)) > 0.001f,
          "the independent pre-release fixture must contain real pitch plus sway/bob");
    Check(TransformNear(beforeRelease.heldItems[0].worldFromItem, expectedHeld),
          "shared fixed-step state must own the independently derived held torch matrix");
    Check(TransformNear(released.heldItems[0].worldFromItem,
                        beforeRelease.heldItems[0].worldFromItem, 0.000001f) &&
              TransformNear(released.heldItems[0].worldFromDetach,
                            beforeRelease.heldItems[0].worldFromItem, 0.000001f),
          "the first independently resolved released matrix must exactly equal the last held matrix");

    const std::uint64_t detachTick = released.heldItems[0].detachTick;
    simulation.StepFixed(input);
    Check(simulation.Snapshot().heldItems[0].detachTick == detachTick,
          "subsequent authored trajectory ticks must not detach the torch again");
}

void TestSharedFixedStepOwnsItemsKinematicsAndSocketLight()
{
    horde::scene::assets::StaticMeshAsset torchAsset;
    std::string diagnostic;
    Check(LoadPlayerRagTorch(torchAsset, diagnostic),
          "player Rag torch must load for shared ownership checks");
    const auto* flame = horde::gameplay::items::FindHeldItemSocket(torchAsset.sockets, "Flame");
    const auto* light = horde::gameplay::items::FindHeldItemSocket(torchAsset.sockets, "Light");
    Check(flame != nullptr && light != nullptr,
          "production torch Flame and Light sockets must exist for shared ownership checks");
    if (flame == nullptr || light == nullptr) return;

    horde::gameplay::simulation::GameSimulation simulation;
    horde::gameplay::simulation::InputSnapshot input;
    input.yawRadians = -0.37f;
    input.pitchRadians = 0.11f;
    input.moveForward = 0.72f;
    for (std::size_t tick = 0u; tick < 9u; ++tick) simulation.StepFixed(input);
    const auto& snapshot = simulation.Snapshot();
    Check(!TransformNear(snapshot.heldItems[0].worldFromItem,
                         horde::gameplay::items::IdentityHeldItemTransform()) &&
              !TransformNear(snapshot.heldItems[1].worldFromItem,
                             horde::gameplay::items::IdentityHeldItemTransform()),
          "shared fixed-step snapshots must carry resolved sword and torch world matrices");
    Check(Near(snapshot.heldItemKinematics.leftHandLocal[2],
               snapshot.heldItemKinematics.heldPropDepth),
          "shared snapshots must carry the same wall-aware hand target used for item resolution");
    const HeldItemTransform expectedFlame = horde::gameplay::items::MultiplyHeldItemTransforms(
        snapshot.heldItems[0].worldFromItem, flame->world);
    const HeldItemTransform expectedLight = horde::gameplay::items::MultiplyHeldItemTransforms(
        snapshot.heldItems[0].worldFromItem, light->world);
    Check(TransformNear(snapshot.heldLight.worldFromFlame, expectedFlame) &&
              TransformNear(snapshot.heldLight.worldFromLight, expectedLight) &&
              Near(snapshot.heldLight.flameStrength, snapshot.torchFailure.flameStrength),
          "exact full Flame and Light socket transforms must be immutable shared simulation output");
}

void TestHeldItemSnapshotsAreRenderDeliveryInvariant()
{
    const auto run = [](const std::uint32_t renderRate) {
        horde::gameplay::simulation::GameSimulation simulation;
        horde::gameplay::simulation::InputSnapshot input;
        input.hasAuthoritativePlayerPose = true;
        input.authoritativePlayerX = -2.16f;
        input.authoritativePlayerZ = -15.14f;
        input.yawRadians = 0.43f;
        input.pitchRadians = 0.17f;
        const std::uint32_t frameCount = renderRate * 2u;
        for (std::uint32_t frame = 0u; frame < frameCount; ++frame)
            simulation.AdvanceFrame(input, 1.0 / static_cast<double>(renderRate), frame + 1u);
        return simulation.Snapshot();
    };
    const auto at30 = run(30u);
    const auto at60 = run(60u);
    const auto at120 = run(120u);
    Check(at30.tickIndex == at60.tickIndex && at60.tickIndex == at120.tickIndex &&
              TransformNear(at30.heldItems[0].worldFromItem, at60.heldItems[0].worldFromItem) &&
              TransformNear(at60.heldItems[0].worldFromItem, at120.heldItems[0].worldFromItem) &&
              TransformNear(at30.heldItems[1].worldFromItem, at120.heldItems[1].worldFromItem) &&
              TransformNear(at30.heldLight.worldFromLight, at120.heldLight.worldFromLight),
          "30/60/120 Hz renderer delivery must expose identical fixed-step item and light state");
}

void TestHeldItemBlasMeasurementsIncludeBothProductionAssets()
{
    horde::vulkan::raytracing::HeldItemBlasMeasurements measurements;
    measurements.RecordTorch(4096u, 1.25);
    measurements.RecordSword(8192u, 2.50);
    Check(measurements.torchBytes == 4096u && measurements.swordBytes == 8192u &&
              measurements.TotalBytes() == 12288u &&
              std::abs(measurements.TotalBuildMilliseconds() - 3.75) < 0.000001,
          "production held-item BLAS evidence must sum torch and sword bytes and timings");
}

void TestHeldLightGpuAbiAppendsWithoutChangingReleasedBindings()
{
    Check(horde::vulkan::raytracing::kRtBindingInstanceMetadata == 11u &&
              horde::vulkan::raytracing::kRtBindingEmissiveTextures == 19u &&
              horde::vulkan::raytracing::kRtBindingHeldLight == 20u &&
              sizeof(horde::vulkan::raytracing::RtHeldLightGpu) == 16u,
          "held light GPU state must append at binding 20 without changing bindings 0-19");
}

void TestResetAndCheckpointImportRestoreParentContracts()
{
    auto items = horde::gameplay::items::MakeDefaultHeldItemStates();
    horde::gameplay::items::ImportHeldItemCheckpoint(items, false, 900u);
    Check(items[0].parentMode == HeldItemParentMode::AuthoredWorldTrajectory &&
              items[0].detached && items[0].detachTick == 900u &&
              items[1].parentMode == HeldItemParentMode::HandSocket,
          "post-drop checkpoint import must detach only the original torch");
    horde::gameplay::items::ResetHeldItemStates(items);
    Check(items[0].id == HeldItemId::OriginalTorch &&
              items[0].parentMode == HeldItemParentMode::HandSocket && !items[0].detached &&
              items[1].id == HeldItemId::Sword &&
              items[1].parentMode == HeldItemParentMode::HandSocket,
          "route reset must restore both canonical held-item attachments");
}

void TestRenderSlotConvertsGenericTransformWithoutItemBranches()
{
    HeldItemState sword = horde::gameplay::items::MakeHeldItemState(
        HeldItemId::Sword, HeldHand::RightHand);
    sword.worldFromItem = Translation(2.0f, 3.0f, 4.0f);
    const auto instance = horde::vulkan::raytracing::HeldItemRenderSlot::BuildInstanceTransform(sword);
    Check(Near(instance[3], 2.0f) && Near(instance[7], 3.0f) && Near(instance[11], 4.0f),
          "generic held-item render slot must preserve matrix translation in Vulkan 3x4 order");
}

void TestFlameAndLightSocketsFollowTheComposedItem()
{
    const HeldItemTransform worldFromTorch = Translation(10.0f, 2.0f, -4.0f);
    const HeldItemTransform itemFromFlame = Translation(0.0f, 0.7f, 0.0f);
    const HeldItemTransform itemFromLight = Translation(0.0f, 0.8f, 0.1f);
    horde::gameplay::items::HeldLightState light{};
    std::string diagnostic;
    Check(horde::gameplay::items::ComposeHeldLightState(
              worldFromTorch, itemFromFlame, itemFromLight, 0.65f, light, diagnostic),
          "rigid Flame and Light sockets must compose from the generic item transform");
    Check(Near(light.worldFromFlame[12], 10.0f) && Near(light.worldFromFlame[13], 2.7f) &&
              Near(light.worldFromLight[13], 2.8f) && Near(light.worldFromLight[14], -3.9f) &&
              Near(light.flameStrength, 0.65f) && light.active,
          "engine flame and light state must follow authored sockets without flame geometry");
}

void TestSimulationOwnsResetAndCheckpointParentState()
{
    horde::gameplay::simulation::GameSimulation simulation;
    Check(simulation.Snapshot().heldItems[0].parentMode == HeldItemParentMode::HandSocket &&
              simulation.Snapshot().heldItems[1].parentMode == HeldItemParentMode::HandSocket,
          "fresh shared simulation must author both held attachments");
    Check(simulation.ApplyShowcaseCheckpoint(4),
          "the original post-drop showcase checkpoint must import");
    Check(simulation.Snapshot().heldItems[0].parentMode ==
              HeldItemParentMode::AuthoredWorldTrajectory &&
              simulation.Snapshot().heldItems[0].detached &&
              simulation.Snapshot().heldItems[1].parentMode == HeldItemParentMode::HandSocket,
          "post-drop checkpoint must import torch parent state through shared simulation");
    simulation.ResetRoute();
    Check(simulation.Snapshot().heldItems[0].parentMode == HeldItemParentMode::HandSocket &&
              !simulation.Snapshot().heldItems[0].detached,
          "shared route reset must reattach the original torch");
}

void TestSharedKinematicsOwnsWallDepthHandsAndSwordPose()
{
    horde::gameplay::items::HeldItemKinematicsInput input;
    input.cameraX = 0.0f;
    input.cameraZ = 1.85f;
    input.cameraYawRadians = 0.0f;
    input.walkTime = 0.0f;
    input.walkAmount = 0.0f;
    const auto idle = horde::gameplay::items::EvaluateHeldItemKinematics(input);
    Check(Near(idle.heldPropDepth, 0.68f) &&
              Near(idle.leftHandLocal[0], -0.135f) && Near(idle.leftHandLocal[1], -0.41f) &&
              Near(idle.leftHandLocal[2], idle.heldPropDepth) &&
              Near(idle.rightHandLocal[0], 0.18f) && Near(idle.rightHandLocal[1], -0.44f) &&
              Near(idle.rightHandLocal[2], 0.77f),
          "shared kinematics must own the safe-frame wall-aware idle hand targets");

    input.torchFailure.leftArmLowerBlend = 1.0f;
    input.playerCombat.action = horde::gameplay::PlayerCombatAction::ParryActive;
    const auto loweredAndParrying = horde::gameplay::items::EvaluateHeldItemKinematics(input);
    Check(Near(loweredAndParrying.leftHandLocal[0], -0.36f) &&
              Near(loweredAndParrying.leftHandLocal[1], -0.92f) &&
              Near(loweredAndParrying.leftHandLocal[2], 0.27f) &&
              Near(loweredAndParrying.rightHandLocal[0], -0.16f) &&
              Near(loweredAndParrying.swordRadians, -0.62f),
          "torch lowering and sword parry must share the authored hand-target evaluator");
}

void TestRewardLanternHighLowUsesSharedLeftArmTarget()
{
    using namespace horde::gameplay::interactions;
    horde::gameplay::items::HeldItemKinematicsInput input{};
    input.cameraX = -11.0f;
    input.cameraZ = -15.2f;
    input.cameraYawRadians = -1.57079632679f;
    input.torchFailure.heldByPlayer = false;
    input.torchFailure.leftArmLowerBlend = 1.0f;
    input.interaction.heldLightKind = HeldLightKind::RewardLantern;
    input.interaction.heldLightPose = HeldLightPose::High;
    input.interaction.heldLightPoseProgress = 1.0f;
    const auto high = horde::gameplay::items::EvaluateHeldItemKinematics(input);

    input.interaction.heldLightPose = HeldLightPose::Low;
    const auto low = horde::gameplay::items::EvaluateHeldItemKinematics(input);
    input.interaction.heldLightPose = HeldLightPose::TransitioningToLow;
    input.interaction.heldLightPoseProgress = 0.5f;
    const auto midpoint = horde::gameplay::items::EvaluateHeldItemKinematics(input);

    Check(high.leftHandLocal[0] < -0.12f && high.leftHandLocal[0] > -0.14f &&
              low.leftHandLocal[0] < -0.12f && low.leftHandLocal[0] > -0.14f,
          "reward high/low targets must stay inward while retaining the anatomical left side");
    Check(high.leftHandLocal[1] > -0.08f &&
              low.leftHandLocal[1] <= high.leftHandLocal[1] - 0.18f &&
              low.leftHandLocal[1] > -0.26f,
          "reward high/low carry must visibly raise and lower the real left arm without using the failed-torch pose");

    // This is the exact post-claim stand-off: the player faces west into the
    // low Gothic chest while holding the reward high. The chest remains a
    // solid player collider, but emergency held-prop retraction must keep the
    // grip inside the narrow 1440:3120 phone frustum instead of moving the
    // entire left arm and lantern off-screen while their light remains live.
    auto chestInput = input;
    chestInput.cameraX = horde::gameplay::kRewardChestRoutePosition.x + 1.30f;
    chestInput.cameraZ = horde::gameplay::kRewardChestRoutePosition.z;
    chestInput.cameraYawRadians = -1.57079632679f;
    chestInput.interaction.heldLightPose = HeldLightPose::High;
    const auto chestHigh =
        horde::gameplay::items::EvaluateHeldItemKinematics(chestInput);
    constexpr float portraitAspect = 1440.0f / 3120.0f;
    const float chestGripNdcX = 1.22f * chestHigh.leftHandLocal[0] /
        (portraitAspect * chestHigh.leftHandLocal[2]);
    const float chestGripNdcY = 1.22f * chestHigh.leftHandLocal[1] /
        chestHigh.leftHandLocal[2];
    std::cout << "post-claim chest held depth/x/y/clearance="
              << chestHigh.leftHandLocal[2] << '/'
              << chestHigh.leftHandLocal[0] << '/'
              << chestHigh.leftHandLocal[1] << '/'
              << horde::gameplay::items::ComputeRewardLanternForwardClearance(
                     chestInput.cameraX, chestInput.cameraZ, -1.0f, 0.0f)
              << '\n';
    Check(chestHigh.leftHandLocal[2] >= 1.02f &&
              chestHigh.leftHandLocal[2] <= 1.05f &&
              chestGripNdcX >= -0.90f && chestGripNdcX <= -0.05f &&
              chestGripNdcY >= -0.75f && chestGripNdcY <= 0.50f,
          "the low post-claim chest must not replace the accepted elevated lantern carry with a near-camera wall pose");
    for (std::size_t axis = 0u; axis < midpoint.leftHandLocal.size(); ++axis)
    {
        Check(Near(midpoint.leftHandLocal[axis],
                   0.5f * (high.leftHandLocal[axis] + low.leftHandLocal[axis]), 0.0002f),
              "the 0.65 second high/low transition must remain continuous in shared kinematics");
    }
    horde::gameplay::items::HeldItemKinematicsInput guardedInput = input;
    guardedInput.cameraX = 0.0f;
    guardedInput.cameraZ = -9.70f;
    guardedInput.cameraYawRadians = 0.0f;
    guardedInput.interaction.heldLightPose = HeldLightPose::High;
    const auto guardedHigh =
        horde::gameplay::items::EvaluateHeldItemKinematics(guardedInput);
    guardedInput.interaction.heldLightPose = HeldLightPose::Low;
    const auto guardedLow =
        horde::gameplay::items::EvaluateHeldItemKinematics(guardedInput);
    horde::gameplay::items::HeldItemKinematicsInput wallInput = guardedInput;
    wallInput.cameraZ = -9.70f;
    const auto wallLow =
        horde::gameplay::items::EvaluateHeldItemKinematics(wallInput);
    wallInput.interaction.heldLightPose = HeldLightPose::High;
    const auto wallHigh =
        horde::gameplay::items::EvaluateHeldItemKinematics(wallInput);
    std::cout << "reward carry open held/high/low x="
              << high.heldPropDepth << '/' << high.leftHandLocal[2] << '/'
              << low.leftHandLocal[2] << '/' << high.leftHandLocal[0]
              << " guard clearance/hand="
              << horde::gameplay::items::ComputeRewardLanternForwardClearance(
                     guardedInput.cameraX, guardedInput.cameraZ, 0.0f, -1.0f)
              << '/' << guardedHigh.leftHandLocal[2]
              << " emergency wall held/hand/x=" << wallHigh.heldPropDepth << '/'
              << wallHigh.leftHandLocal[2] << '/' << wallHigh.leftHandLocal[0]
              << '\n';
    Check(Near(high.heldPropDepth, low.heldPropDepth) &&
              high.leftHandLocal[2] >= 1.03f &&
              high.leftHandLocal[2] <= 1.05f &&
              std::abs(high.leftHandLocal[2] - low.leftHandLocal[2]) <= 0.002f &&
              guardedHigh.leftHandLocal[2] >= 0.19f &&
              guardedHigh.leftHandLocal[2] <= 0.21f &&
              guardedHigh.leftHandLocal[2] == guardedLow.leftHandLocal[2] &&
              guardedHigh.leftHandLocal[1] >= guardedLow.leftHandLocal[1] + 0.03f &&
              wallHigh.leftHandLocal[2] >= 0.19f &&
              wallHigh.leftHandLocal[2] <= 0.21f &&
              wallHigh.leftHandLocal[0] <= -0.44f &&
              wallHigh.leftHandLocal[0] >= -0.46f &&
              std::abs(high.rewardLanternPresentationYawRadians) <= 0.05f &&
              guardedHigh.rewardLanternPresentationYawRadians >= 1.50f &&
              guardedHigh.rewardLanternPresentationYawRadians <= 1.58f &&
              wallHigh.rewardLanternPresentationYawRadians >= 1.50f &&
              wallHigh.rewardLanternPresentationYawRadians <= 1.58f &&
              wallHigh.leftHandLocal[1] >= wallLow.leftHandLocal[1] + 0.03f &&
              wallHigh.leftHandLocal[2] == wallLow.leftHandLocal[2] &&
              wallHigh.leftShoulderLocal[2] >= 0.38f &&
              wallHigh.leftShoulderLocal[2] <= 0.65f &&
              high.leftShoulderLocal[2] <= 0.65f &&
              wallHigh.rightShoulderLocal[2] <= 0.42f &&
              high.rightShoulderLocal[2] <= 0.42f &&
              wallHigh.leftHandLocal[2] < low.leftHandLocal[2] - 0.70f,
          "reward carry must solve reach through only the left arm while both shoulders remain close to the ordinary anchored torso depth");

    auto approachInput = input;
    approachInput.interaction.heldLightPose = HeldLightPose::High;
    approachInput.interaction.heldLightPoseProgress = 1.0f;
    approachInput.cameraX = 0.0f;
    approachInput.cameraZ = -7.90f;
    approachInput.cameraYawRadians = 0.0f;
    const auto approachStart =
        horde::gameplay::items::EvaluateHeldItemKinematics(approachInput);
    float previousDepth = approachStart.leftHandLocal[2];
    float previousLateral = approachStart.leftHandLocal[0];
    float previousPresentationYaw =
        approachStart.rewardLanternPresentationYawRadians;
    float maximumClearanceDelta = 0.0f;
    float maximumDepthDelta = 0.0f;
    float maximumLateralDelta = 0.0f;
    float maximumYawDelta = 0.0f;
    float previousClearance =
        horde::gameplay::items::ComputeRewardLanternForwardClearance(
            approachInput.cameraX, approachInput.cameraZ, 0.0f, -1.0f);
    for (int step = 1; step <= 1840; ++step)
    {
        approachInput.cameraZ = -7.90f - static_cast<float>(step) * 0.001f;
        const auto approach =
            horde::gameplay::items::EvaluateHeldItemKinematics(approachInput);
        const float clearance =
            horde::gameplay::items::ComputeRewardLanternForwardClearance(
                approachInput.cameraX, approachInput.cameraZ, 0.0f, -1.0f);
        maximumClearanceDelta = std::max(
            maximumClearanceDelta, std::abs(clearance - previousClearance));
        maximumDepthDelta = std::max(
            maximumDepthDelta, std::abs(approach.leftHandLocal[2] - previousDepth));
        maximumLateralDelta = std::max(
            maximumLateralDelta, std::abs(approach.leftHandLocal[0] - previousLateral));
        maximumYawDelta = std::max(
            maximumYawDelta,
            std::abs(approach.rewardLanternPresentationYawRadians -
                     previousPresentationYaw));
        Check(approach.leftHandLocal[2] <= previousDepth + 0.0002f &&
                  previousDepth - approach.leftHandLocal[2] <= 0.020f &&
                  approach.leftHandLocal[0] <= previousLateral + 0.0002f &&
                  previousLateral - approach.leftHandLocal[0] <= 0.003f &&
                  approach.rewardLanternPresentationYawRadians >=
                      previousPresentationYaw - 0.0002f &&
                  approach.rewardLanternPresentationYawRadians -
                      previousPresentationYaw <= 0.040f,
              "one-millimetre wall approach phases must retract the reward pose continuously without a fixed-grid carry pop");
        previousClearance = clearance;
        previousDepth = approach.leftHandLocal[2];
        previousLateral = approach.leftHandLocal[0];
        previousPresentationYaw = approach.rewardLanternPresentationYawRadians;
    }
    std::cout << "reward one-millimetre wall sweep max clearance/depth/lateral/yaw delta="
              << maximumClearanceDelta << '/' << maximumDepthDelta << '/'
              << maximumLateralDelta << '/' << maximumYawDelta << '\n';
    Check(maximumClearanceDelta <= 0.002f && maximumDepthDelta <= 0.020f &&
              maximumLateralDelta <= 0.003f && maximumYawDelta <= 0.040f,
          "reward collision distance and authored carry response must be continuous across every 1 mm wall phase");
}

void TestProductionSwordAssetMeetsGenericSocketAndPbrBudget()
{
    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    const auto directory = root / "assets/models/weapons/runtime";
    horde::scene::assets::AssetManifest manifest;
    horde::scene::assets::StaticMeshAsset asset;
    std::string diagnostic;
    Check(horde::scene::assets::AssetManifest::Load(
              directory / "asset.manifest.json", manifest, diagnostic),
          "production sword manifest must load through the real schema parser");
    Check(horde::scene::assets::StaticMeshAsset::Load(
              directory / "gothic-arming-sword-rh-lod0.runtime.glb",
              manifest,
              asset,
              diagnostic),
          "production sword GLB must load through the generic static PBR reader");
    if (!asset.indices.empty())
    {
        const std::size_t triangles = asset.indices.size() / 3u;
        Check(triangles >= 8000u && triangles <= 12500u && asset.materials.size() <= 2u,
              "production sword must stay inside the approved runtime triangle/material budget");
        const auto* grip = horde::gameplay::items::FindHeldItemSocket(asset.sockets, "Grip");
        Check(grip != nullptr &&
                  horde::gameplay::items::ValidateHeldItemSocketTransform(
                      grip->world, diagnostic),
              "production sword must retain an exact rigid Grip socket");
        Check(asset.materials[0].emissiveTexture < 0 &&
                  asset.materials[0].emissiveFactor == std::array<float, 3u>{},
              "production sword must not carry emissive or magical material data");
    }
}

void TestProductionTorchAssetMeetsGenericSocketAndPbrBudget()
{
    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    const auto directory = root / "assets/models/props/runtime";
    horde::scene::assets::AssetManifest manifest;
    horde::scene::assets::StaticMeshAsset asset;
    std::string diagnostic;
    Check(horde::scene::assets::AssetManifest::Load(
              directory / "asset.manifest.json", manifest, diagnostic),
          "production torch manifest must load through the real schema parser");
    Check(horde::scene::assets::StaticMeshAsset::Load(
              directory / "gothic-hand-torch-lod0.runtime.glb",
              manifest,
              asset,
              diagnostic),
          "production torch GLB must load through the generic static PBR reader");
    if (!asset.indices.empty())
    {
        const std::size_t triangles = asset.indices.size() / 3u;
        Check(triangles >= 3000u && triangles <= 6000u && asset.materials.size() <= 2u,
              "production torch must stay inside the approved runtime triangle/material budget");
        for (const char* socketName : {"Grip", "Flame", "Light"})
        {
            const auto* socket = horde::gameplay::items::FindHeldItemSocket(asset.sockets, socketName);
            Check(socket != nullptr &&
                      horde::gameplay::items::ValidateHeldItemSocketTransform(
                          socket->world, diagnostic),
                  "production torch must retain exact rigid Grip, Flame, and Light sockets");
        }
        Check(asset.materials[0].emissiveTexture < 0 &&
                  asset.materials[0].emissiveFactor == std::array<float, 3u>{},
              "production torch body must not carry flame geometry or emissive material data");
    }
}

void TestProductionAssetsShareOneGenericStaticSlot()
{
    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    horde::scene::assets::AssetManifest swordManifest;
    horde::scene::assets::AssetManifest torchManifest;
    horde::scene::assets::AssetManifest playerManifest;
    horde::scene::assets::StaticMeshAsset sword;
    horde::scene::assets::StaticMeshAsset torch;
    horde::scene::assets::StaticMeshAsset player;
    std::string diagnostic;
    const auto swordDirectory = root / "assets/models/weapons/runtime";
    const auto torchDirectory = root / "assets/models/props/runtime";
    const auto playerDirectory = root / "assets/models/player/runtime";
    Check(horde::scene::assets::AssetManifest::Load(
              swordDirectory / "asset.manifest.json", swordManifest, diagnostic) &&
              horde::scene::assets::StaticMeshAsset::Load(
                  swordDirectory / "gothic-arming-sword-rh-lod0.runtime.glb",
                  swordManifest, sword, diagnostic) &&
              horde::scene::assets::AssetManifest::Load(
                  torchDirectory / "asset.manifest.json", torchManifest, diagnostic) &&
              horde::scene::assets::StaticMeshAsset::Load(
                  torchDirectory / "gothic-hand-torch-lod0.runtime.glb",
                  torchManifest, torch, diagnostic) &&
              horde::scene::assets::AssetManifest::Load(
                  playerDirectory / "asset.manifest.json", playerManifest, diagnostic) &&
              horde::scene::assets::StaticMeshAsset::Load(
                  playerDirectory / "gothic-traveller-lod0.runtime.glb",
                  playerManifest, player, diagnostic),
          "all three production PBR assets must load before generic slot registration");
    std::array<horde::vulkan::raytracing::StaticRtAssetRegistration, 3u> registrations{{
        {3u, 0x53574f52u,
         static_cast<std::uint32_t>(horde::vulkan::raytracing::RtInstanceFlag::StaticPbr),
         0u, &sword},
        {1u, 0x544f5243u,
         static_cast<std::uint32_t>(horde::vulkan::raytracing::RtInstanceFlag::StaticPbr),
         1u, &torch},
        {4u, 0x504c4159u,
         static_cast<std::uint32_t>(horde::vulkan::raytracing::RtInstanceFlag::StaticPbr),
         0u, &player},
    }};
    horde::vulkan::raytracing::RtStaticMeshSlot slot;
    Check(slot.Initialize(registrations, diagnostic),
          "sword and torch must register through one generic static PBR slot");
    const auto& metadata = slot.InstanceMetadata();
    Check(metadata[3].primitiveCount == sword.primitives.size() &&
              metadata[1].primitiveCount == torch.primitives.size() &&
              metadata[4].primitiveCount == player.primitives.size() &&
              metadata[1].emitterIndex == 1u &&
              metadata[3].emitterIndex == 0u,
          "generic registrations must retain stable TLAS routes and engine-emitter ownership");
    const auto counts = slot.TextureArrayCounts();
    Check(counts.baseColor == 4u && counts.normal == 4u && counts.orm == 4u &&
              counts.emissive == 0u,
          "generic material routing must include the player in audited shared texture array layers");
    const auto originalMaterials = player.materials;
    const auto originalPrimitives = player.primitives;
    constexpr auto regionCount = horde::scene::assets::kPlayerPrimitiveContract.size();
    Check(player.materials.size() == regionCount && player.primitives.size() == regionCount,
          "promoted world body must exercise all five named regions");
    if (player.materials.size() != regionCount || player.primitives.size() != regionCount) return;
    std::array<unsigned, regionCount> order{};
    std::iota(order.begin(), order.end(), 0u);
    unsigned permutations = 0u;
    do
    {
        std::array<unsigned, regionCount> remap{};
        for (unsigned i = 0; i < order.size(); ++i)
        {
            player.materials[i] = originalMaterials[order[i]];
            remap[order[i]] = i;
        }
        player.primitives = originalPrimitives;
        for (auto& primitive : player.primitives)
        {
            Check(primitive.materialIndex < remap.size(), "material remap must stay within admitted regions");
            if (primitive.materialIndex >= remap.size()) return;
            primitive.materialIndex = remap[primitive.materialIndex];
        }
        ++permutations;
        const bool initialized = slot.Initialize(registrations, diagnostic);
        Check(initialized, "all player material orders must register");
        if (!initialized) continue;
        const auto materialBase = sword.materials.size() + torch.materials.size();
        for (std::size_t i = 0; i < player.materials.size(); ++i)
        {
            const auto* part = horde::scene::assets::FindPlayerPrimitiveContract(player.materials[i].name);
            Check(part != nullptr, "loaded player material must have a named contract");
            if (!part) continue;
            const std::uint32_t expected = part->textureGroup == horde::scene::assets::PlayerTextureGroup::Body ? 2u : 3u;
            Check(slot.Materials()[materialBase + i].textureLayers ==
                      std::array<std::uint32_t, 4u>{{expected, expected, expected, 0u}},
                  "actual player body and gauntlet map to generated atlas layers regardless of order");
        }
        Check(slot.TextureArrayCounts().baseColor == 4u && slot.TextureArrayCounts().normal == 4u &&
                  slot.TextureArrayCounts().orm == 4u, "reordered player cannot grow texture allocations");
    } while (std::next_permutation(order.begin(), order.end()));
    Check(permutations == 120u, "all 120 five-region atlas permutations must be exercised");
}

void TestPlayerSwordScabbardUsesAppendedProductionPbrLayer()
{
    using namespace horde::scene::assets;
    using namespace horde::vulkan::raytracing;
    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    std::array<AssetManifest, 12u> manifests{};
    std::array<StaticMeshAsset, 12u> assets{};
    std::array<std::filesystem::path, 12u> directories{{
        root / "assets/models/weapons/runtime",
        root / "assets/models/props/runtime",
        root / "assets/models/player/runtime",
        root / "assets/models/props/runtime/dielectric-fixture",
        root / "assets/models/props/runtime/gothic-chest-base",
        root / "assets/models/props/runtime/gothic-chest-lid",
        root / "assets/models/props/runtime/reward-lantern-ring",
        root / "assets/models/props/runtime/reward-lantern-body",
        root / "assets/models/player/viewmodel/runtime",
        root / "assets/models/world/runtime/collapsed-entry",
        root / "assets/models/props/runtime/player-rag-torch",
        root / "assets/models/props/runtime/player-sword-scabbard"}};
    const std::array<const char*, 12u> filenames{{
        "gothic-arming-sword-rh-lod0.runtime.glb",
        "gothic-hand-torch-lod0.runtime.glb",
        "gothic-traveller-lod0.runtime.glb",
        "closed-glass-lod0.runtime.glb",
        "gothic-chest-base-lod0.runtime.glb",
        "gothic-chest-lid-lod0.runtime.glb",
        "reward-lantern-ring-lod0.runtime.glb",
        "reward-lantern-body-lod0.runtime.glb",
        "gothic-traveller-viewmodel.runtime.glb",
        "collapsed-entry-lod0.runtime.glb",
        "rag-torch-player-lod0.runtime.glb",
        "player-sword-scabbard-lod0.runtime.glb"}};
    std::string diagnostic;
    bool loaded = true;
    for (std::size_t i = 0u; i < assets.size(); ++i)
    {
        loaded &= AssetManifest::Load(directories[i] / "asset.manifest.json",
                                      manifests[i], diagnostic) &&
                  StaticMeshAsset::Load(directories[i] / filenames[i],
                                        manifests[i], assets[i], diagnostic);
    }
    Check(loaded,
          "the complete production static roster including the scabbard must import before slot routing");
    if (!loaded) return;

    std::array<float, 3u> scabbardMinimum{{
        std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::infinity()}};
    std::array<float, 3u> scabbardMaximum{{
        -std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity()}};
    for (const auto& vertex : assets[11].vertices)
        for (std::size_t axis = 0u; axis < 3u; ++axis)
        {
            scabbardMinimum[axis] = std::min(scabbardMinimum[axis], vertex.position[axis]);
            scabbardMaximum[axis] = std::max(scabbardMaximum[axis], vertex.position[axis]);
        }
    Check(manifests[11].upAxis == "+Y" && manifests[11].forwardAxis == "+Z" &&
              scabbardMaximum[1] - scabbardMinimum[1] > 0.70f &&
              scabbardMaximum[2] - scabbardMinimum[2] < 0.05f,
          "the imported scabbard geometry must follow its declared +Y up axis without a hidden node correction");

    const auto pbr = static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr);
    std::array<StaticRtAssetRegistration, 12u> registrations{{
        {3u, 0x53574f52u, pbr, 0u, &assets[0]},
        {1u, 0x544f5243u, pbr, 1u, &assets[1]},
        {4u, 0x504c4159u, pbr, 0u, &assets[2], nullptr,
         RtGeometryRole::PlayerWorldBody},
        {9u, 0x4449454cu,
         pbr | static_cast<std::uint32_t>(RtInstanceFlag::Transmissive),
         0u, &assets[3]},
        {5u, 0x43484241u, pbr, 0u, &assets[4]},
        {6u, 0x43484c44u, pbr, 0u, &assets[5]},
        {7u, 0x4c4e5247u, pbr, 0u, &assets[6]},
        {8u, 0x4c4e4244u,
         pbr | static_cast<std::uint32_t>(RtInstanceFlag::Transmissive),
         0u, &assets[7]},
        {20u, 0x56494557u, pbr, 0u, &assets[8], &assets[2],
         RtGeometryRole::PlayerViewmodel},
        {21u, 0x434f4c4cu, pbr, 0u, &assets[9]},
        {22u, 0x50524147u, pbr, 0u, &assets[10]},
        {23u, 0x53434142u, pbr, 0u, &assets[11]},
    }};
    RtStaticMeshSlot slot;
    const bool initialized = slot.Initialize(registrations, diagnostic);
    Check(initialized,
          "appending the scabbard must fit the unchanged generic static-PBR asset and metadata routes");
    if (!initialized) return;

    const auto& metadata = slot.InstanceMetadata();
    const auto materialFor = [&](const std::uint32_t instance) -> const RtMaterialGpu* {
        const auto& owner = metadata[instance];
        if (owner.primitiveCount == 0u || owner.primitiveBase >= slot.PrimitiveMetadata().size())
            return nullptr;
        const auto materialIndex =
            slot.PrimitiveMetadata()[owner.primitiveBase].materialIndex;
        return materialIndex < slot.Materials().size()
            ? &slot.Materials()[materialIndex] : nullptr;
    };
    const auto* legacyTorchMaterial = materialFor(1u);
    const auto* ragTorchMaterial = materialFor(22u);
    const auto* scabbardMaterial = materialFor(23u);
    const auto scabbardFlags = static_cast<std::uint32_t>(RtMaterialFlag::BaseColorTexture) |
        static_cast<std::uint32_t>(RtMaterialFlag::NormalTexture) |
        static_cast<std::uint32_t>(RtMaterialFlag::OrmTexture);
    Check(metadata[1].assetIndex == 1u && legacyTorchMaterial != nullptr &&
              legacyTorchMaterial->textureLayers[0] == 1u,
          "Keeper/reward torch remains the original production asset at layer 1");
    Check(metadata[22].assetIndex == 10u && ragTorchMaterial != nullptr &&
              ragTorchMaterial->textureLayers[0] == 12u,
          "the player Rag torch keeps its existing independent layer 12 route");
    Check(metadata[23].assetIndex == 11u &&
              metadata[23].primitiveCount == assets[11].primitives.size() &&
              assets[11].primitives.size() == 1u && scabbardMaterial != nullptr &&
              scabbardMaterial->textureLayers ==
                  std::array<std::uint32_t, 4u>{{13u, 13u, 13u, 0u}} &&
              (scabbardMaterial->materialFlags[0] & scabbardFlags) == scabbardFlags &&
              (scabbardMaterial->materialFlags[0] &
                  static_cast<std::uint32_t>(RtMaterialFlag::EmissiveTexture)) == 0u,
          "the original sheath must use appended layer13 base/normal/ORM in the generic PBR route, with no emissive map");
    const auto textureCounts = slot.TextureArrayCounts();
    if (textureCounts.baseColor != 14u || textureCounts.normal != 14u ||
        textureCounts.orm != 14u || textureCounts.emissive != 0u)
    {
        std::cerr << "Imported production texture layer counts base/normal/ORM/emissive="
                  << textureCounts.baseColor << '/' << textureCounts.normal << '/'
                  << textureCounts.orm << '/' << textureCounts.emissive << '\n';
    }
    Check(textureCounts.baseColor == 14u && textureCounts.normal == 14u &&
              textureCounts.orm == 14u && textureCounts.emissive == 0u,
          "the imported material roster must route 14/14/14 logical layers and no emissive maps");
    const auto atlasManifestPath = root / "assets/textures/props/runtime/asset.manifest.json";
    std::ifstream atlasManifestStream(atlasManifestPath, std::ios::binary);
    const std::string atlasManifestText(
        std::istreambuf_iterator<char>(atlasManifestStream), {});
    Check(atlasManifestStream.good() || atlasManifestStream.eof(),
          "the production atlas manifest must be readable for its physical fallback assertion");
    Check(atlasManifestText.find("\"emissive\": 1") != std::string::npos,
          "the physical production atlas retains its single shared black emissive fallback layer");
}

void TestProductionSocketsMatchSharedFixedStepContracts()
{
    horde::scene::assets::StaticMeshAsset sword;
    horde::scene::assets::StaticMeshAsset torch;
    std::string diagnostic;
    Check(LoadProductionHeldAssets(sword, torch, diagnostic),
          "production GLBs must load before comparing shared socket contracts");
    const auto* swordGrip = horde::gameplay::items::FindHeldItemSocket(sword.sockets, "Grip");
    const auto* torchGrip = horde::gameplay::items::FindHeldItemSocket(torch.sockets, "Grip");
    const auto* flame = horde::gameplay::items::FindHeldItemSocket(torch.sockets, "Flame");
    const auto* light = horde::gameplay::items::FindHeldItemSocket(torch.sockets, "Light");
    Check(swordGrip != nullptr && torchGrip != nullptr && flame != nullptr && light != nullptr &&
              TransformNear(swordGrip->world,
                            horde::gameplay::items::SwordGripSocketTransform()) &&
              TransformNear(torchGrip->world,
                            horde::gameplay::items::OriginalTorchGripSocketTransform()) &&
              TransformNear(flame->world,
                            horde::gameplay::items::OriginalTorchFlameSocketTransform()) &&
              TransformNear(light->world,
                            horde::gameplay::items::OriginalTorchLightSocketTransform()),
          "legacy Keeper torch GLB must retain its direct Grip/Flame/Light socket contracts");
}

void TestPlayerRagTorchSocketsDriveFixedStepAttachmentAndLight()
{
    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    const auto ragDirectory = root / "assets/models/props/runtime/player-rag-torch";
    horde::scene::assets::AssetManifest manifest;
    horde::scene::assets::StaticMeshAsset ragTorch;
    std::string diagnostic;
    const bool loaded = horde::scene::assets::AssetManifest::Load(
                            ragDirectory / "asset.manifest.json", manifest, diagnostic) &&
                        horde::scene::assets::StaticMeshAsset::Load(
                            ragDirectory / "rag-torch-player-lod0.runtime.glb",
                            manifest, ragTorch, diagnostic);
    Check(loaded, "player Rag torch must load before fixed-step socket verification");
    if (!loaded) return;

    const auto* grip = horde::gameplay::items::FindHeldItemSocket(ragTorch.sockets, "Grip");
    const auto* flame = horde::gameplay::items::FindHeldItemSocket(ragTorch.sockets, "Flame");
    const auto* light = horde::gameplay::items::FindHeldItemSocket(ragTorch.sockets, "Light");
    Check(grip != nullptr && flame != nullptr && light != nullptr,
          "player Rag torch must retain its authored Grip/Flame/Light socket names");
    if (grip == nullptr || flame == nullptr || light == nullptr) return;

    auto items = horde::gameplay::items::MakeDefaultHeldItemStates();
    horde::gameplay::items::HeldItemFixedStepInput input;
    horde::gameplay::items::HeldItemFixedStepState state;
    Check(horde::gameplay::items::ResolveHeldItemsFixedStep(items, input, 1u, state, diagnostic),
          "fixed-step player Rag torch attachment must resolve");
    Check(TransformNear(
              horde::gameplay::items::MultiplyHeldItemTransforms(items[0].worldFromItem,
                                                                  grip->world),
              state.worldFromLeftHand) &&
              TransformNear(state.light.worldFromFlame,
                            horde::gameplay::items::MultiplyHeldItemTransforms(
                                items[0].worldFromItem, flame->world)) &&
              TransformNear(state.light.worldFromLight,
                            horde::gameplay::items::MultiplyHeldItemTransforms(
                                items[0].worldFromItem, light->world)),
          "player hand attachment and engine flame/light transforms must use the Rag asset sockets");
}

void TestProductionTorchFitsSharedClearanceEnvelope()
{
    using namespace horde::gameplay::items;
    horde::scene::assets::StaticMeshAsset sword;
    horde::scene::assets::StaticMeshAsset torch;
    std::string diagnostic;
    const bool loaded = LoadProductionHeldAssets(sword, torch, diagnostic);
    Check(loaded, "actual production torch must load before clearance-envelope admission");
    if (!loaded) return;
    const auto* grip = FindHeldItemSocket(torch.sockets, "Grip");
    const auto* flame = FindHeldItemSocket(torch.sockets, "Flame");
    Check(grip != nullptr && flame != nullptr, "clearance admission requires exact Grip and Flame sockets");
    if (grip == nullptr || flame == nullptr) return;
    bool bodyAdmitted = true;
    for (const auto& vertex : torch.vertices)
    {
        const float x = vertex.position[0] - grip->world[12];
        const float y = vertex.position[1] - grip->world[13];
        const float z = vertex.position[2] - grip->world[14];
        bodyAdmitted &= std::hypot(x, z) <= kHeldTorchEnvelopeRadius &&
                        y <= kHeldTorchEnvelopeTopFromGrip && y >= -0.25f;
    }
    Check(bodyAdmitted, "actual GLB body vertices must fit the shared held-torch clearance envelope");
    Check(flame->world[13] - grip->world[13] + 0.34f + 0.06f <=
              kHeldTorchEnvelopeTopFromGrip + 0.00001f,
          "clearance must include full visible fire height above the actual Flame socket, not just the cage");
}

void TestRagTorchEnvelopeIncludesTheUnchangedEngineFire()
{
    using namespace horde::gameplay::items;
    horde::scene::assets::StaticMeshAsset ragTorch;
    std::string diagnostic;
    Check(LoadPlayerRagTorch(ragTorch, diagnostic),
          "player Rag torch must load before engine-fire clearance admission");
    if (ragTorch.vertices.empty()) return;
    const auto* grip = FindHeldItemSocket(ragTorch.sockets, "Grip");
    const auto* flame = FindHeldItemSocket(ragTorch.sockets, "Flame");
    Check(grip != nullptr && flame != nullptr,
          "player Rag torch fire clearance requires the authored Grip and Flame sockets");
    if (grip == nullptr || flame == nullptr) return;
    const float fireTipFromGrip = flame->world[13] - grip->world[13] + 0.34f + 0.06f;
    Check(Near(fireTipFromGrip, kPlayerRagTorchEnvelopeTopFromGrip, 0.0001f) &&
              kPlayerRagTorchEnvelopeTopFromGrip > kHeldTorchEnvelopeTopFromGrip,
          "Rag clearance must admit its higher authored Flame socket while preserving the original torch envelope");

    HeldItemFixedStepInput input;
    input.playerX = 0.0f;
    input.playerZ = -2.48f;
    input.playerYawRadians = 0.0f;
    input.torchFailure.heldByPlayer = true;
    HeldItemStates items = MakeDefaultHeldItemStates();
    HeldItemFixedStepState state;
    Check(ResolveHeldItemsFixedStep(items, input, 1u, state, diagnostic),
          "player Rag torch fixed-step attachment must resolve under the low lintel");
    const auto flameWorld = MultiplyHeldItemTransforms(items[0].worldFromItem, flame->world);
    const float fireTopY = flameWorld[13] + 0.40f * items[0].worldFromItem[5];
    Check(state.kinematics.torchOverheadLowering > 0.0f && fireTopY <= 0.78f + 0.002f,
          "the actual Rag Flame socket and unchanged engine fire must clear the low lintel as one attached frame");
    Check(TransformNear(MultiplyHeldItemTransforms(items[0].worldFromItem, grip->world),
                        state.worldFromLeftHand),
          "Rag clearance must lower the shared grip, item, engine fire and emitted light together");

    constexpr float aspect = 1440.0f / 3120.0f;
    const auto safeFrame = EvaluateOwnerFeedbackPortraitSafeFrame(state.kinematics, aspect);
    float importedMinimum = 1.0e9f;
    float importedMaximum = -1.0e9f;
    const auto includeImportedPoint = [&](const std::array<float, 3u>& worldPoint) {
        std::array<float, 3u> relative{{worldPoint[0] - grip->world[12],
                                        worldPoint[1] - grip->world[13],
                                        worldPoint[2] - grip->world[14]}};
        std::array<float, 3u> local{};
        for (std::size_t axis = 0u; axis < 3u; ++axis)
            local[axis] = relative[0] * grip->world[axis * 4u] +
                          relative[1] * grip->world[axis * 4u + 1u] +
                          relative[2] * grip->world[axis * 4u + 2u];
        const auto viewPoint = Add(state.kinematics.leftHandLocal,
            Add(Scale(state.kinematics.leftGripXInView, local[0]),
                Add(Scale(state.kinematics.leftGripYInView, local[1]),
                    Scale(state.kinematics.leftGripZInView, local[2]))));
        const float ndcX = 1.22f * viewPoint[0] /
            (std::max(viewPoint[2], 0.05f) * aspect);
        importedMinimum = std::min(importedMinimum, ndcX);
        importedMaximum = std::max(importedMaximum, ndcX);
    };
    for (const auto& vertex : ragTorch.vertices)
        includeImportedPoint({{vertex.position[0], vertex.position[1], vertex.position[2]}});
    includeImportedPoint({{flame->world[12], flame->world[13], flame->world[14]}});
    includeImportedPoint({{flame->world[12] + 0.34f * flame->world[4],
                           flame->world[13] + 0.34f * flame->world[5],
                           flame->world[14] + 0.34f * flame->world[6]}});
    const auto* light = FindHeldItemSocket(ragTorch.sockets, "Light");
    Check(light != nullptr, "portrait admission must inspect the imported Rag Light socket");
    if (light != nullptr)
        includeImportedPoint({{light->world[12], light->world[13], light->world[14]}});
    Check(safeFrame.includesTorchGrip && safeFrame.includesFlame && safeFrame.includesLight &&
              safeFrame.minimumNdcX <= importedMinimum + 0.0001f &&
              safeFrame.maximumNdcX >= importedMaximum - 0.0001f,
          "portrait safe-frame admission must contain imported Rag mesh bounds and off-centre Flame/Light sockets through the shared left Grip orientation");
}

bool ResolveProductionAnatomicalSword(
    const horde::gameplay::items::HeldItemFixedStepInput& input,
    const horde::gameplay::items::HeldItemKinematicsState& kinematics,
    horde::gameplay::items::HeldItemStates& items,
    horde::vulkan::raytracing::PlayerRenderSlot& rig,
    const std::uint64_t tick,
    HeldItemTransform& worldFromGrip,
    HeldItemTransform& worldFromSword,
    std::string& diagnostic)
{
    using namespace horde::gameplay::animation;
    using namespace horde::vulkan::raytracing;
    PlayerAnimationState playerAnimation;
    PlayerAnimationInput animationInput;
    animationInput.heldItemKinematics = kinematics;
    animationInput.playerCombat = input.playerCombat;
    animationInput.walkTime = input.walkTime;
    animationInput.walkAmount = input.walkAmount;
    playerAnimation.StepFixed(animationInput, 1.0f / 60.0f);
    auto animation = playerAnimation.Snapshot();

    const std::array<float, 3u> eye{{input.playerX, horde::gameplay::kShowcaseEyeWorldY,
                                      input.playerZ}};
    const auto forward = Normalize({{std::sin(input.playerYawRadians),
        -0.05f + std::clamp(input.playerPitchRadians, -0.32f, 0.28f),
        -std::cos(input.playerYawRadians)}});
    const std::array<float, 3u> up{{0.0f, 1.0f, 0.0f}};
    const auto right = Normalize(Cross(forward, up));
    const auto viewUp = Normalize(Cross(right, forward));
    const auto viewVectorToWorld = [&](const std::array<float, 3u>& value) {
        return Add(Add(Scale(right, value[0]), Scale(viewUp, value[1])),
                   Scale(forward, value[2]));
    };
    const auto bodyForward = std::array<float, 3u>{{std::sin(input.playerYawRadians),
        0.0f, -std::cos(input.playerYawRadians)}};
    const auto bodyRight = std::array<float, 3u>{{std::cos(input.playerYawRadians),
        0.0f, std::sin(input.playerYawRadians)}};
    const auto basis = BuildPlayerModelWorldBasis(bodyRight, bodyForward);
    const auto root = GroundPlayerRootOnRouteFloor(
        eye, horde::gameplay::kRouteFloorWorldY,
        rig.BootGroundingOffsetMetres(animation));
    const auto pointToModel = [&](const std::array<float, 3u>& local) {
        const auto world = Add(eye, viewVectorToWorld(local));
        return WorldVectorToPlayerModel(basis,
            {{world[0] - root[0], world[1] - root[1], world[2] - root[2]}});
    };
    const auto vectorToModel = [&](const std::array<float, 3u>& value) {
        return WorldVectorToPlayerModel(basis, viewVectorToWorld(value));
    };
    for (auto* arm : {&animation.leftIk, &animation.rightIk})
    {
        arm->shoulder = pointToModel(arm->shoulder);
        arm->target = pointToModel(arm->target);
        arm->pole = vectorToModel(arm->pole);
        arm->gripX = vectorToModel(arm->gripX);
        arm->gripY = vectorToModel(arm->gripY);
        arm->gripZ = vectorToModel(arm->gripZ);
    }
    bool poseUpdated = false;
    if (!rig.PreparePose(animation, tick, PlayerCpuSkinCadence::Hz60,
                         poseUpdated, diagnostic))
        return false;
    worldFromGrip = PlayerGripToWorld(rig.BoneSockets().rightGrip, basis, root);
    horde::gameplay::items::HeldItemStates renderedItems;
    if (!rig.ResolveHeldItemVisuals(items,
            PlayerGripToWorld(rig.BoneSockets().leftGrip, basis, root),
            worldFromGrip, renderedItems, diagnostic))
        return false;
    worldFromSword = renderedItems[1].worldFromItem;
    return true;
}

struct WorldBounds
{
    std::array<float, 3u> minimum{{
        std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()}};
    std::array<float, 3u> maximum{{
        -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max()}};
};

void IncludePoint(WorldBounds& bounds, const std::array<float, 3u>& point)
{
    for (std::size_t axis = 0u; axis < 3u; ++axis)
    {
        bounds.minimum[axis] = std::min(bounds.minimum[axis], point[axis]);
        bounds.maximum[axis] = std::max(bounds.maximum[axis], point[axis]);
    }
}

float DistanceToBounds(const std::array<float, 3u>& point,
                       const WorldBounds& bounds)
{
    float distanceSquared = 0.0f;
    for (std::size_t axis = 0u; axis < 3u; ++axis)
    {
        const float gap = point[axis] < bounds.minimum[axis]
            ? bounds.minimum[axis] - point[axis]
            : (point[axis] > bounds.maximum[axis]
                ? point[axis] - bounds.maximum[axis] : 0.0f);
        distanceSquared += gap * gap;
    }
    return std::sqrt(distanceSquared);
}

float DistanceSquaredToBounds(const std::array<float, 3u>& point,
                              const WorldBounds& bounds)
{
    const float distance = DistanceToBounds(point, bounds);
    return distance * distance;
}

struct MeshTriangle
{
    std::array<float, 3u> a{};
    std::array<float, 3u> b{};
    std::array<float, 3u> c{};
    WorldBounds bounds{};
    std::array<float, 3u> center{};
};

float PointTriangleDistanceSquared(const std::array<float, 3u>& point,
                                   const MeshTriangle& triangle)
{
    const auto subtract = [](const auto& left, const auto& right) {
        return std::array<float, 3u>{{left[0] - right[0], left[1] - right[1],
                                      left[2] - right[2]}};
    };
    const auto dot = [](const auto& left, const auto& right) {
        return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
    };
    const auto ab = subtract(triangle.b, triangle.a);
    const auto ac = subtract(triangle.c, triangle.a);
    const auto ap = subtract(point, triangle.a);
    const float d1 = dot(ab, ap);
    const float d2 = dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return dot(ap, ap);

    const auto bp = subtract(point, triangle.b);
    const float d3 = dot(ab, bp);
    const float d4 = dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return dot(bp, bp);

    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        const float v = d1 / (d1 - d3);
        const auto delta = subtract(point, Add(triangle.a, Scale(ab, v)));
        return dot(delta, delta);
    }

    const auto cp = subtract(point, triangle.c);
    const float d5 = dot(ab, cp);
    const float d6 = dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return dot(cp, cp);

    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        const float w = d2 / (d2 - d6);
        const auto delta = subtract(point, Add(triangle.a, Scale(ac, w)));
        return dot(delta, delta);
    }

    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
    {
        const auto bc = subtract(triangle.c, triangle.b);
        const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        const auto delta = subtract(point, Add(triangle.b, Scale(bc, w)));
        return dot(delta, delta);
    }

    const float denominator = 1.0f / (va + vb + vc);
    const float v = vb * denominator;
    const float w = vc * denominator;
    const auto closest = Add(Add(triangle.a, Scale(ab, v)), Scale(ac, w));
    const auto delta = subtract(point, closest);
    return dot(delta, delta);
}

class TriangleBoundsTree
{
public:
    explicit TriangleBoundsTree(std::vector<MeshTriangle> triangles)
        : triangles_(std::move(triangles)), order_(triangles_.size())
    {
        std::iota(order_.begin(), order_.end(), 0u);
        if (!order_.empty()) Build(0u, order_.size());
    }

    float Distance(const std::array<float, 3u>& point) const
    {
        if (nodes_.empty()) return std::numeric_limits<float>::max();
        float bestSquared = std::numeric_limits<float>::max();
        Query(0u, point, bestSquared);
        return std::sqrt(bestSquared);
    }

private:
    struct Node
    {
        WorldBounds bounds{};
        std::size_t begin = 0u;
        std::size_t end = 0u;
        std::size_t left = std::numeric_limits<std::size_t>::max();
        std::size_t right = std::numeric_limits<std::size_t>::max();
    };

    std::size_t Build(const std::size_t begin, const std::size_t end)
    {
        const std::size_t nodeIndex = nodes_.size();
        nodes_.emplace_back();
        WorldBounds bounds;
        WorldBounds centers;
        for (std::size_t i = begin; i < end; ++i)
        {
            const auto& triangle = triangles_[order_[i]];
            IncludePoint(bounds, triangle.bounds.minimum);
            IncludePoint(bounds, triangle.bounds.maximum);
            IncludePoint(centers, triangle.center);
        }
        nodes_[nodeIndex].bounds = bounds;
        nodes_[nodeIndex].begin = begin;
        nodes_[nodeIndex].end = end;
        if (end - begin <= 8u) return nodeIndex;

        std::size_t axis = 0u;
        for (std::size_t candidate = 1u; candidate < 3u; ++candidate)
            if (centers.maximum[candidate] - centers.minimum[candidate] >
                centers.maximum[axis] - centers.minimum[axis]) axis = candidate;
        const std::size_t middle = begin + (end - begin) / 2u;
        std::nth_element(order_.begin() + static_cast<std::ptrdiff_t>(begin),
                         order_.begin() + static_cast<std::ptrdiff_t>(middle),
                         order_.begin() + static_cast<std::ptrdiff_t>(end),
            [&](const std::size_t left, const std::size_t right) {
                return triangles_[left].center[axis] < triangles_[right].center[axis];
            });
        const std::size_t left = Build(begin, middle);
        const std::size_t right = Build(middle, end);
        nodes_[nodeIndex].left = left;
        nodes_[nodeIndex].right = right;
        return nodeIndex;
    }

    void Query(const std::size_t nodeIndex,
               const std::array<float, 3u>& point,
               float& bestSquared) const
    {
        const Node& node = nodes_[nodeIndex];
        if (DistanceSquaredToBounds(point, node.bounds) >= bestSquared) return;
        if (node.left == std::numeric_limits<std::size_t>::max())
        {
            for (std::size_t i = node.begin; i < node.end; ++i)
                bestSquared = std::min(bestSquared,
                    PointTriangleDistanceSquared(point, triangles_[order_[i]]));
            return;
        }
        const float leftDistance = DistanceSquaredToBounds(point, nodes_[node.left].bounds);
        const float rightDistance = DistanceSquaredToBounds(point, nodes_[node.right].bounds);
        if (leftDistance <= rightDistance)
        {
            Query(node.left, point, bestSquared);
            Query(node.right, point, bestSquared);
        }
        else
        {
            Query(node.right, point, bestSquared);
            Query(node.left, point, bestSquared);
        }
    }

    std::vector<MeshTriangle> triangles_;
    std::vector<std::size_t> order_;
    std::vector<Node> nodes_;
};

std::array<float, 3u> SkeletonLocalToWorld(
    const std::array<float, 3u>& local,
    const float x,
    const float z,
    const float facingRadians)
{
    const float cosine = std::cos(facingRadians);
    const float sine = std::sin(facingRadians);
    return {{cosine * local[0] + sine * local[2] + x,
             horde::gameplay::kRouteFloorWorldY + local[1],
             -sine * local[0] + cosine * local[2] + z}};
}

std::array<float, 3u> ItemVertexToGripLocal(
    const std::array<float, 3u>& point,
    const HeldItemTransform& itemFromGrip)
{
    const std::array<float, 3u> relative{{
        point[0] - itemFromGrip[12], point[1] - itemFromGrip[13],
        point[2] - itemFromGrip[14]}};
    return {{relative[0] * itemFromGrip[0] + relative[1] * itemFromGrip[1] + relative[2] * itemFromGrip[2],
             relative[0] * itemFromGrip[4] + relative[1] * itemFromGrip[5] + relative[2] * itemFromGrip[6],
             relative[0] * itemFromGrip[8] + relative[1] * itemFromGrip[9] + relative[2] * itemFromGrip[10]}};
}

void TestCombatPulseAgainstImportedSwordAndSkeletonBounds(const bool detailed = false)
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::items;
    using horde::scene::SkinnedClip;
    using horde::scene::SkinnedMeshAsset;

    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    horde::scene::assets::StaticMeshAsset sword;
    horde::scene::assets::StaticMeshAsset torch;
    std::string diagnostic;
    Check(LoadProductionHeldAssets(sword, torch, diagnostic) && !sword.vertices.empty(),
          "combat contact sampling must load the production sword vertices and Grip");
    const auto* grip = FindHeldItemSocket(sword.sockets, "Grip");
    Check(grip != nullptr,
          "combat contact sampling must use the imported sword Grip socket");
    if (grip == nullptr || sword.vertices.empty()) return;
    WorldBounds gripRelativeSwordBounds;
    std::array<std::size_t, 6u> gripHeightBands{};
    for (const auto& vertex : sword.vertices)
    {
        const auto local = ItemVertexToGripLocal(
            {{vertex.position[0], vertex.position[1], vertex.position[2]}}, grip->world);
        IncludePoint(gripRelativeSwordBounds, local);
        const std::size_t band = local[1] <= 0.15f ? 0u
            : (local[1] < 0.35f ? 1u
                : (local[1] < 0.55f ? 2u
                    : (local[1] < 0.75f ? 3u
                        : (local[1] < 0.95f ? 4u : 5u))));
        ++gripHeightBands[band];
    }
    std::cout << "sword grip-local envelope y=" << gripRelativeSwordBounds.minimum[1]
              << ':' << gripRelativeSwordBounds.maximum[1]
              << " vertexBands(y<=.15,.15-.35,.35-.55,.55-.75,.75-.95,>.95)=";
    for (const std::size_t count : gripHeightBands) std::cout << count << ',';
    std::cout << " primitiveMaterials=";
    for (const auto& primitive : sword.primitives)
    {
        const std::size_t materialIndex = std::min<std::size_t>(
            primitive.materialIndex, sword.materials.empty() ? 0u : sword.materials.size() - 1u);
        std::cout << (sword.materials.empty() ? "none" : sword.materials[materialIndex].name)
                  << ':' << primitive.indexCount << ',';
    }
    std::cout << '\n';

    SkinnedMeshAsset skeleton;
    const auto skeletonPath = root / "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb";
    Check(skeleton.LoadCombatClips(skeletonPath.string(), diagnostic),
          "combat contact sampling must load the imported skeleton combat clips");
    if (!skeleton.IsLoaded()) return;
    std::vector<horde::scene::SkinnedRtVertex> targetPose;
    Check(skeleton.Skin(SkinnedClip::Idle, 0.0f, targetPose, diagnostic) &&
              !targetPose.empty(),
          "target bounds must come from the imported skeleton's evaluated idle mesh");
    if (targetPose.empty()) return;

    horde::vulkan::raytracing::PlayerRenderSlot rig;
    Check(rig.LoadAsset(
              (root / "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb").string(),
              diagnostic),
          "combat contact sampling must resolve the production player's final right Grip");
    if (!rig.IsLoaded()) return;

    struct SampleCase
    {
        const char* name;
        float distance;
        float targetBearing;
        float playerYaw;
        bool combo;
    };
    const float coneEdge = std::acos(SwordCombat::kPlayerHitConeDot);
    const std::vector<SampleCase> cases = detailed
        ? std::vector<SampleCase>{{"normal-near", 1.20f, 0.0f, 0.0f, false},
            {"normal-range-1.0", 1.00f, 0.0f, 0.0f, false},
            {"normal-range-1.5", 1.50f, 0.0f, 0.0f, false},
            {"normal-bearing-plus15", 1.20f, 0.2617994f, 0.0f, false},
            {"normal-bearing-minus15", 1.20f, -0.2617994f, 0.0f, false},
            {"normal-bearing-plus30", 1.20f, 0.5235988f, 0.0f, false},
            {"normal-bearing-minus30", 1.20f, -0.5235988f, 0.0f, false},
            {"normal-range-edge", SwordCombat::kPlayerHitRange - 0.01f, 0.0f, 0.0f, false},
            {"normal-range-outside", SwordCombat::kPlayerHitRange + 0.01f, 0.0f, 0.0f, false},
            {"normal-cone-inside", 1.20f, coneEdge - 0.02f, 0.0f, false},
            {"normal-cone-outside", 1.20f, coneEdge + 0.02f, 0.0f, false},
            {"combo-near", 1.20f, 0.0f, 0.0f, true}}
        : std::vector<SampleCase>{{"normal-near", 1.20f, 0.0f, 0.0f, false},
            {"combo-near", 1.20f, 0.0f, 0.0f, true}};

    constexpr float tickSeconds = 1.0f / 60.0f;
    constexpr float playerX = 0.0f;
    constexpr float playerZ = 0.0f;
    bool allSamplesResolved = true;
    bool nearCaseHitAtNormalPulse = false;
    bool nearDownwardPulseMeetsBlade = false;
    bool nearUpwardPulseMeetsBlade = false;
    bool coneCaseAdmissionMatches = true;
    bool rangeCaseAdmissionMatches = true;
    bool nearComboEmittedBothPulses = false;
    std::uint64_t rigTick = 50000u;
    for (const SampleCase& sampleCase : cases)
    {
        const float targetX = sampleCase.distance * std::sin(sampleCase.targetBearing);
        const float targetZ = -sampleCase.distance * std::cos(sampleCase.targetBearing);
        const float targetFacing = std::atan2(playerX - targetX, playerZ - targetZ);
        WorldBounds targetBounds;
        std::vector<MeshTriangle> targetTriangles;
        Check(targetPose.size() % 3u == 0u,
              "evaluated enemy mesh must remain an expanded triangle list for contact sampling");
        targetTriangles.reserve(targetPose.size() / 3u);
        for (const auto& vertex : targetPose)
            IncludePoint(targetBounds, SkeletonLocalToWorld(
                {{vertex.position[0], vertex.position[1], vertex.position[2]}},
                targetX, targetZ, targetFacing));
        for (std::size_t index = 0u; index + 2u < targetPose.size(); index += 3u)
        {
            MeshTriangle triangle;
            triangle.a = SkeletonLocalToWorld(
                {{targetPose[index].position[0], targetPose[index].position[1],
                  targetPose[index].position[2]}}, targetX, targetZ, targetFacing);
            triangle.b = SkeletonLocalToWorld(
                {{targetPose[index + 1u].position[0], targetPose[index + 1u].position[1],
                  targetPose[index + 1u].position[2]}}, targetX, targetZ, targetFacing);
            triangle.c = SkeletonLocalToWorld(
                {{targetPose[index + 2u].position[0], targetPose[index + 2u].position[1],
                  targetPose[index + 2u].position[2]}}, targetX, targetZ, targetFacing);
            IncludePoint(triangle.bounds, triangle.a);
            IncludePoint(triangle.bounds, triangle.b);
            IncludePoint(triangle.bounds, triangle.c);
            triangle.center = Scale(Add(Add(triangle.a, triangle.b), triangle.c), 1.0f / 3.0f);
            targetTriangles.push_back(triangle);
        }
        const TriangleBoundsTree targetTree(std::move(targetTriangles));

        SwordCombat combat;
        combat.Reset(1u, {targetX, targetZ});
        Check(combat.RequestAttack() == PlayerAttackCut::DownwardCut,
              "contact fixture must start with the ordinary downward attack");
        bool comboQueued = false;
        float minimumSurfaceGap = std::numeric_limits<float>::max();
        float minimumBoundsGap = std::numeric_limits<float>::max();
        float downwardPulseSurfaceGap = std::numeric_limits<float>::max();
        float upwardPulseSurfaceGap = std::numeric_limits<float>::max();
        float downwardPulseBoundsGap = std::numeric_limits<float>::max();
        float upwardPulseBoundsGap = std::numeric_limits<float>::max();
        float minimumBladeSurfaceGap = std::numeric_limits<float>::max();
        float downwardPulseBladeGap = std::numeric_limits<float>::max();
        float upwardPulseBladeGap = std::numeric_limits<float>::max();
        std::uint64_t closestBladeTick = 0u;
        std::uint64_t closestSurfaceTick = 0u;
        std::uint64_t firstNearSurfaceTick = 0u;
        std::uint64_t lastNearSurfaceTick = 0u;
        std::array<float, 3u> closestSurfaceGripLocal{};
        std::array<float, 3u> closestBladeGripLocal{};
        std::array<float, 3u> downwardPulseGripLocal{};
        std::array<float, 3u> upwardPulseGripLocal{};
        std::array<std::uint64_t, 3u> firstNearSurfaceByAction{};
        std::array<std::uint64_t, 3u> lastNearSurfaceByAction{};
        std::array<std::uint64_t, 3u> firstNearBladeByAction{};
        std::array<std::uint64_t, 3u> lastNearBladeByAction{};
        std::uint64_t firstBoundsOverlapTick = 0u;
        std::uint64_t lastBoundsOverlapTick = 0u;
        std::uint64_t downwardPulseTick = 0u;
        std::uint64_t upwardPulseTick = 0u;
        unsigned downwardPulseCount = 0u;
        unsigned upwardPulseCount = 0u;
        float downwardPulseActionTime = -1.0f;
        float upwardPulseActionTime = -1.0f;
        bool downwardPulseAdmitted = false;
        bool targetAlreadyDeadAtUpwardPulse = false;
        for (std::uint64_t tick = 1u; tick <= 56u; ++tick)
        {
            if (sampleCase.combo && !comboQueued &&
                combat.Snapshot().player.action == PlayerCombatAction::SwingActive)
            {
                comboQueued = combat.RequestAttack() == PlayerAttackCut::UpwardSlice;
            }
            const auto& snapshot = combat.Update(
                tickSeconds, playerX, playerZ, sampleCase.playerYaw, true, false);
            bool measureTick = tick >= 8u && tick <= (sampleCase.combo ? 40u : 22u);
            if (!detailed)
            {
                const bool normalSample = tick == 10u || tick == 11u || tick == 16u ||
                    tick == 20u || tick == 21u || tick == 22u;
                const bool upwardSample = tick == 26u || tick == 27u || tick == 32u ||
                    tick == 37u || tick == 38u;
                measureTick = sampleCase.combo ? (normalSample || upwardSample) : normalSample;
            }
            if (!measureTick && !snapshot.playerAttackPulse) continue;
            HeldItemFixedStepInput input;
            input.playerX = playerX;
            input.playerZ = playerZ;
            input.playerYawRadians = sampleCase.playerYaw;
            input.playerMountProfile = PlayerMountProfile::AnatomicalBody;
            input.playerCombat = snapshot.player;
            HeldItemStates items = MakeDefaultHeldItemStates();
            HeldItemFixedStepState fixed;
            const bool fixedResolved = ResolveHeldItemsFixedStep(items, input, tick,
                                                                  fixed, diagnostic);
            HeldItemTransform worldFromGrip{};
            HeldItemTransform worldFromSword{};
            const bool rigResolved = fixedResolved && ResolveProductionAnatomicalSword(
                input, fixed.kinematics, items, rig, rigTick++, worldFromGrip,
                worldFromSword, diagnostic);
            allSamplesResolved &= rigResolved;
            if (!rigResolved) continue;

            float tickGap = std::numeric_limits<float>::max();
            float tickSurfaceGap = std::numeric_limits<float>::max();
            float tickBladeSurfaceGap = std::numeric_limits<float>::max();
            std::array<float, 3u> tickSurfaceGripLocal{};
            std::array<float, 3u> tickBladeGripLocal{};
            for (const auto& vertex : sword.vertices)
            {
                const std::array<float, 3u> localPoint{{
                    vertex.position[0], vertex.position[1], vertex.position[2]}};
                const auto point = TransformPoint(worldFromSword, localPoint);
                tickGap = std::min(tickGap, DistanceToBounds(point, targetBounds));
                const float surfaceGap = targetTree.Distance(point);
                const auto gripLocal = ItemVertexToGripLocal(localPoint, grip->world);
                if (surfaceGap < tickSurfaceGap)
                {
                    tickSurfaceGap = surfaceGap;
                    tickSurfaceGripLocal = gripLocal;
                }
                if (gripLocal[1] > 0.15f)
                {
                    if (surfaceGap < tickBladeSurfaceGap)
                    {
                        tickBladeSurfaceGap = surfaceGap;
                        tickBladeGripLocal = gripLocal;
                    }
                }
            }
            minimumBoundsGap = std::min(minimumBoundsGap, tickGap);
            if (tickSurfaceGap < minimumSurfaceGap)
            {
                minimumSurfaceGap = tickSurfaceGap;
                closestSurfaceTick = tick;
                closestSurfaceGripLocal = tickSurfaceGripLocal;
            }
            if (tickBladeSurfaceGap < minimumBladeSurfaceGap)
            {
                minimumBladeSurfaceGap = tickBladeSurfaceGap;
                closestBladeTick = tick;
                closestBladeGripLocal = tickBladeGripLocal;
            }
            if (tickSurfaceGap <= 0.02f)
            {
                if (firstNearSurfaceTick == 0u) firstNearSurfaceTick = tick;
                lastNearSurfaceTick = tick;
                const std::size_t actionIndex = snapshot.player.action == PlayerCombatAction::SwingActive ? 0u
                    : (snapshot.player.action == PlayerCombatAction::UpwardSliceActive ? 1u : 2u);
                if (firstNearSurfaceByAction[actionIndex] == 0u)
                    firstNearSurfaceByAction[actionIndex] = tick;
                lastNearSurfaceByAction[actionIndex] = tick;
            }
            if (tickBladeSurfaceGap <= 0.02f)
            {
                const std::size_t actionIndex = snapshot.player.action == PlayerCombatAction::SwingActive ? 0u
                    : (snapshot.player.action == PlayerCombatAction::UpwardSliceActive ? 1u : 2u);
                if (firstNearBladeByAction[actionIndex] == 0u)
                    firstNearBladeByAction[actionIndex] = tick;
                lastNearBladeByAction[actionIndex] = tick;
            }
            if (tickGap <= 0.0001f)
            {
                if (firstBoundsOverlapTick == 0u) firstBoundsOverlapTick = tick;
                lastBoundsOverlapTick = tick;
            }
            if (snapshot.playerAttackPulse)
            {
                if (snapshot.playerAttackCut == PlayerAttackCut::DownwardCut)
                {
                    downwardPulseTick = tick;
                    ++downwardPulseCount;
                    downwardPulseActionTime = snapshot.player.actionTime;
                    downwardPulseSurfaceGap = tickSurfaceGap;
                    downwardPulseBoundsGap = tickGap;
                    downwardPulseBladeGap = tickBladeSurfaceGap;
                    downwardPulseGripLocal = tickSurfaceGripLocal;
                    downwardPulseAdmitted = snapshot.combatants[0].health == 0;
                }
                else if (snapshot.playerAttackCut == PlayerAttackCut::UpwardSlice)
                {
                    upwardPulseTick = tick;
                    ++upwardPulseCount;
                    upwardPulseActionTime = snapshot.player.actionTime;
                    upwardPulseSurfaceGap = tickSurfaceGap;
                    upwardPulseBoundsGap = tickGap;
                    upwardPulseBladeGap = tickBladeSurfaceGap;
                    upwardPulseGripLocal = tickSurfaceGripLocal;
                    targetAlreadyDeadAtUpwardPulse = snapshot.combatants[0].health == 0;
                }
            }
            if (detailed && (sampleCase.name == std::string("normal-near") ||
                             sampleCase.name == std::string("combo-near")))
                std::cout << "combat-geometry-tick " << sampleCase.name
                          << " tick=" << tick
                          << " action=" << static_cast<int>(snapshot.player.action)
                          << " actionTime=" << snapshot.player.actionTime
                          << " pulse=" << snapshot.playerAttackPulse
                          << " cut=" << static_cast<int>(snapshot.playerAttackCut)
                          << " fullGap=" << tickSurfaceGap
                          << " bladeGapYgt015=" << tickBladeSurfaceGap << '\n';
            if (tick > 1u && snapshot.player.action == PlayerCombatAction::Idle &&
                !snapshot.player.comboQueued)
                break;
        }

        if (sampleCase.name == std::string("normal-near"))
        {
            nearCaseHitAtNormalPulse = downwardPulseAdmitted;
            nearDownwardPulseMeetsBlade = downwardPulseBladeGap <= 0.02f;
        }
        if (sampleCase.name == std::string("combo-near"))
            nearUpwardPulseMeetsBlade = upwardPulseBladeGap <= 0.02f;
        if (sampleCase.name == std::string("normal-cone-inside"))
            coneCaseAdmissionMatches = coneCaseAdmissionMatches && downwardPulseAdmitted;
        if (sampleCase.name == std::string("normal-cone-outside"))
            coneCaseAdmissionMatches = coneCaseAdmissionMatches && !downwardPulseAdmitted;
        if (sampleCase.name == std::string("normal-range-edge"))
            rangeCaseAdmissionMatches = rangeCaseAdmissionMatches && downwardPulseAdmitted;
        if (sampleCase.name == std::string("normal-range-outside"))
            rangeCaseAdmissionMatches = rangeCaseAdmissionMatches && !downwardPulseAdmitted;
        if (sampleCase.name == std::string("combo-near"))
            nearComboEmittedBothPulses = downwardPulseCount == 1u && upwardPulseCount == 1u &&
                downwardPulseTick < upwardPulseTick &&
                downwardPulseActionTime >= SwordCombat::kDownwardContactTime &&
                downwardPulseActionTime < SwordCombat::kDownwardContactTime + tickSeconds &&
                upwardPulseActionTime >= SwordCombat::kUpwardContactTime &&
                upwardPulseActionTime < SwordCombat::kUpwardContactTime + tickSeconds;
        std::cout << "combat-geometry " << sampleCase.name
                  << " target=" << targetX << ',' << targetZ
                  << " targetY=" << kRouteFloorWorldY
                  << " targetFacing=" << targetFacing
                  << " playerYaw=" << sampleCase.playerYaw
                  << " targetBearing=" << sampleCase.targetBearing
                  << " targetBoundsY=" << targetBounds.minimum[1] << ':' << targetBounds.maximum[1]
                  << " downPulseTick=" << downwardPulseTick
                  << " downActionTime=" << downwardPulseActionTime
                  << " downAdmitted=" << downwardPulseAdmitted
                  << " upPulseTick=" << upwardPulseTick
                  << " upActionTime=" << upwardPulseActionTime
                  << " targetDeadAtUpPulse=" << targetAlreadyDeadAtUpwardPulse
                  << " downPulseSurfaceGap=" << downwardPulseSurfaceGap
                  << " upPulseSurfaceGap=" << upwardPulseSurfaceGap
                  << " downPulseBoundsGap=" << downwardPulseBoundsGap
                  << " upPulseBoundsGap=" << upwardPulseBoundsGap
                  << " downPulseBladeGap=" << downwardPulseBladeGap
                  << " upPulseBladeGap=" << upwardPulseBladeGap
                  << " downPulseNearestGripLocal=" << downwardPulseGripLocal[0] << ','
                  << downwardPulseGripLocal[1] << ',' << downwardPulseGripLocal[2]
                  << " upPulseNearestGripLocal=" << upwardPulseGripLocal[0] << ','
                  << upwardPulseGripLocal[1] << ',' << upwardPulseGripLocal[2]
                  << " minSurfaceGap=" << minimumSurfaceGap
                  << " closestSurfaceTick=" << closestSurfaceTick
                  << " closestSurfaceGripLocal=" << closestSurfaceGripLocal[0] << ','
                  << closestSurfaceGripLocal[1] << ',' << closestSurfaceGripLocal[2]
                  << " minBladeSurfaceGap=" << minimumBladeSurfaceGap
                  << " closestBladeTick=" << closestBladeTick
                  << " closestBladeGripLocal=" << closestBladeGripLocal[0] << ','
                  << closestBladeGripLocal[1] << ',' << closestBladeGripLocal[2]
                  << " nearSurfaceTicks20mm=" << firstNearSurfaceTick << ':' << lastNearSurfaceTick
                  << " downActiveNearTicks=" << firstNearSurfaceByAction[0] << ':' << lastNearSurfaceByAction[0]
                  << " upActiveNearTicks=" << firstNearSurfaceByAction[1] << ':' << lastNearSurfaceByAction[1]
                  << " downActiveBladeNearTicks=" << firstNearBladeByAction[0] << ':' << lastNearBladeByAction[0]
                  << " upActiveBladeNearTicks=" << firstNearBladeByAction[1] << ':' << lastNearBladeByAction[1]
                  << " minGap=" << minimumBoundsGap
                  << (detailed ? " boundsOverlapTicks=" : " boundsOverlapSamples=")
                  << firstBoundsOverlapTick << ':' << lastBoundsOverlapTick
                  << " importedSwordVertices=" << sword.vertices.size()
                  << " importedTargetVertices=" << targetPose.size() << '\n';
    }
    Check(allSamplesResolved,
          "every 60 Hz combat sample must resolve through the final imported player rig and sword Grip");
    Check(nearCaseHitAtNormalPulse,
          "a representative admitted near target must still receive the existing downward pulse");
    Check(nearDownwardPulseMeetsBlade && nearUpwardPulseMeetsBlade,
          "frontal 1.2m contact pulses must occur during close approach of the actual imported blade, not before either stroke");
    Check(nearComboEmittedBothPulses,
          "the combined 60 Hz fixture must retain both downward and upward semantic attack pulses");
    if (detailed)
    {
        Check(coneCaseAdmissionMatches,
              "the contact fixture's near cone boundary must preserve combat's existing range/cone admission");
        Check(rangeCaseAdmissionMatches,
              "the contact fixture's near range boundary must preserve combat's existing range admission");
    }
}

bool ResolveProductionSwordStowPose(
    horde::gameplay::items::HeldItemFixedStepInput input,
    const horde::gameplay::items::HeldItemState& swordState,
    horde::vulkan::raytracing::PlayerRenderSlot& rig,
    const std::uint64_t tick,
    HeldItemTransform& worldFromHips,
    HeldItemTransform& worldFromBodyStow,
    HeldItemTransform& worldFromFinalGrip,
    HeldItemTransform& worldFromDesiredGrip,
    HeldItemTransform& worldFromDesiredItem,
    std::array<float, 3u>& playerRootWorld,
    horde::gameplay::items::HeldItemStates& renderItems,
    std::string& diagnostic)
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::animation;
    using namespace horde::gameplay::items;
    using namespace horde::vulkan::raytracing;
    input.playerMountProfile = PlayerMountProfile::AnatomicalBody;
    HeldItemStates items = MakeDefaultHeldItemStates();
    items[1] = swordState;
    input.swordItemState = &items[1];
    HeldItemFixedStepState fixed{};
    if (!ResolveHeldItemsFixedStep(items, input, tick, fixed, diagnostic)) return false;
    items[1].parentMode = swordState.parentMode;
    items[1].visualStowBlend = swordState.visualStowBlend;
    items[1].visualGripBlend = swordState.visualGripBlend;

    PlayerAnimationState animationState;
    PlayerAnimationInput animationInput;
    animationInput.heldItemKinematics = fixed.kinematics;
    animationInput.playerCombat = input.playerCombat;
    animationInput.walkTime = input.walkTime;
    animationInput.walkAmount = input.walkAmount;
    animationState.StepFixed(animationInput, 1.0f / 60.0f);
    auto animation = animationState.Snapshot();

    const std::array<float, 3u> eye{{input.playerX, kShowcaseEyeWorldY,
                                     input.playerZ}};
    const auto forward = Normalize({{std::sin(input.playerYawRadians),
        -0.05f + std::clamp(input.playerPitchRadians, -0.32f, 0.28f),
        -std::cos(input.playerYawRadians)}});
    const std::array<float, 3u> up{{0.0f, 1.0f, 0.0f}};
    const auto right = Normalize(Cross(forward, up));
    const auto viewUp = Normalize(Cross(right, forward));
    const auto viewVectorToWorld = [&](const std::array<float, 3u>& value) {
        return Add(Add(Scale(right, value[0]), Scale(viewUp, value[1])),
                   Scale(forward, value[2]));
    };
    const std::array<float, 3u> bodyForward{{std::sin(input.playerYawRadians),
        0.0f, -std::cos(input.playerYawRadians)}};
    const std::array<float, 3u> bodyRight{{std::cos(input.playerYawRadians),
        0.0f, std::sin(input.playerYawRadians)}};
    const auto basis = BuildPlayerModelWorldBasis(bodyRight, bodyForward);
    const auto root = GroundPlayerRootOnRouteFloor(
        eye, kRouteFloorWorldY, rig.BootGroundingOffsetMetres(animation));
    playerRootWorld = root;
    const auto pointToModel = [&](const std::array<float, 3u>& local) {
        const auto world = Add(eye, viewVectorToWorld(local));
        return WorldVectorToPlayerModel(basis, {{world[0] - root[0],
            world[1] - root[1], world[2] - root[2]}});
    };
    const auto vectorToModel = [&](const std::array<float, 3u>& local) {
        return WorldVectorToPlayerModel(basis, viewVectorToWorld(local));
    };
    if (!rig.AnimatedHipsWorldTransform(animation, basis, root,
                                        worldFromHips, diagnostic))
        return false;
    worldFromBodyStow = MultiplyHeldItemTransforms(
        worldFromHips, SwordBodyStowFromHips());
    worldFromDesiredItem = BlendHeldItemTransformsAtGrip(
        worldFromBodyStow, items[1].worldFromItem,
        SwordGripSocketTransform(), 1.0f - items[1].visualStowBlend);
    const auto expectedWorldFromGrip = MultiplyHeldItemTransforms(
        worldFromDesiredItem, SwordGripSocketTransform());
    worldFromDesiredGrip = expectedWorldFromGrip;
    animation.rightIk.target = WorldVectorToPlayerModel(basis, {{
        expectedWorldFromGrip[12] - root[0],
        expectedWorldFromGrip[13] - root[1],
        expectedWorldFromGrip[14] - root[2]}});
    animation.rightIk.gripX = WorldVectorToPlayerModel(
        basis, {{expectedWorldFromGrip[0], expectedWorldFromGrip[1],
                 expectedWorldFromGrip[2]}});
    animation.rightIk.gripY = WorldVectorToPlayerModel(
        basis, {{expectedWorldFromGrip[4], expectedWorldFromGrip[5],
                 expectedWorldFromGrip[6]}});
    animation.rightIk.gripZ = WorldVectorToPlayerModel(
        basis, {{expectedWorldFromGrip[8], expectedWorldFromGrip[9],
                 expectedWorldFromGrip[10]}});
    for (auto* arm : {&animation.leftIk, &animation.rightIk})
    {
        arm->shoulder = pointToModel(arm->shoulder);
        arm->pole = vectorToModel(arm->pole);
        if (arm == &animation.leftIk)
        {
            arm->target = pointToModel(arm->target);
            arm->gripX = WorldVectorToPlayerModel(basis,
                viewVectorToWorld(fixed.kinematics.leftGripXInView));
            arm->gripY = WorldVectorToPlayerModel(basis,
                viewVectorToWorld(fixed.kinematics.leftGripYInView));
            arm->gripZ = WorldVectorToPlayerModel(basis,
                viewVectorToWorld(fixed.kinematics.leftGripZInView));
        }
    }
    bool poseUpdated = false;
    if (!rig.PreparePose(animation, tick, PlayerCpuSkinCadence::Hz60,
                         poseUpdated, diagnostic))
        return false;
    worldFromFinalGrip = PlayerGripToWorld(rig.BoneSockets().rightGrip,
                                           basis, root);
    const auto worldFromLeftGrip = PlayerGripToWorld(
        rig.BoneSockets().leftGrip, basis, root);
    return rig.ResolveHeldItemVisuals(items, worldFromLeftGrip,
        worldFromFinalGrip, worldFromBodyStow, renderItems, diagnostic);
}

void TestActualRigSwordBodyStowAndContinuousDrawBlend()
{
    using namespace horde::gameplay::items;
    using namespace horde::vulkan::raytracing;
    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    PlayerRenderSlot rig;
    std::string diagnostic;
    Check(rig.LoadAsset((root / "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb").string(),
                        diagnostic),
          "body-mounted sword must use the actual imported player rig");
    if (!rig.IsLoaded()) return;

    HeldItemFixedStepInput input;
    input.playerMountProfile = PlayerMountProfile::AnatomicalBody;
    input.playerX = -1.25f;
    input.playerZ = -8.4f;
    input.playerYawRadians = 0.42f;
    input.walkTime = 0.31f;
    input.walkAmount = 0.55f;
    HeldItemState swordState = MakeHeldItemState(
        HeldItemId::Sword, HeldHand::RightHand, HeldItemParentMode::BodyStow);
    Check(RequestHeldItemTransition(swordState, HeldItemTransitionKind::Draw, 1u).status ==
              HeldItemTransitionRequestStatus::Started,
          "actual-rig transition fixture must begin with one body-to-hand Draw");
    HeldItemTransform previous{};
    HeldItemTransform originalTorch{};
    bool havePrevious = false;
    bool haveTorch = false;
    float maximumPositionStep = 0.0f;
    float maximumGripPositionError = 0.0f;
    float maximumGripOrientationError = 0.0f;
    bool allGripTargetsResolved = true;
    bool allRigidTransformsValid = true;
    std::uint64_t tick = 1u;
    for (std::uint32_t frame = 0u; frame <= 24u; ++frame)
    {
        if (frame > 0u)
            AdvanceHeldItemTransition(swordState, tick, 1.0f / 60.0f);
        HeldItemTransform hips{}, bodyStow{}, finalGrip{}, desiredGrip{}, desiredItem{};
        std::array<float, 3u> playerRoot{};
        HeldItemStates rendered{};
        if (!ResolveProductionSwordStowPose(input, swordState, rig, tick++,
                hips, bodyStow, finalGrip, desiredGrip, desiredItem,
                playerRoot, rendered, diagnostic))
        {
            std::cerr << "Sword stow actual-rig diagnostic: progress="
                      << swordState.transition.progress << " blend="
                      << swordState.visualStowBlend << '/' << swordState.visualGripBlend
                      << " :: " << diagnostic << '\n';
            allGripTargetsResolved = false;
            continue;
        }
        const auto finalHips = PlayerGripToWorld(rig.BoneSockets().hips,
            BuildPlayerModelWorldBasis(
                {{std::cos(input.playerYawRadians), 0.0f,
                  std::sin(input.playerYawRadians)}},
                {{std::sin(input.playerYawRadians), 0.0f,
                  -std::cos(input.playerYawRadians)}}),
            playerRoot);
        // The body mount input is built from the same animated Hips socket
        // used by final skinning, and no view/camera coordinate is authored
        // into the mount transform.
        allGripTargetsResolved &= TransformNear(finalHips, hips, 0.025f);
        const auto agreement = MeasureTransformAgreement(desiredGrip, finalGrip);
        if (swordState.visualGripBlend >= 0.999f)
        {
            maximumGripPositionError = std::max(maximumGripPositionError,
                                                agreement.positionErrorMetres);
            maximumGripOrientationError = std::max(maximumGripOrientationError,
                                                    agreement.orientationErrorRadians);
            allGripTargetsResolved &=
                agreement.positionErrorMetres <= kPlayerGripSocketToleranceMetres &&
                agreement.orientationErrorRadians <= kPlayerGripOrientationToleranceRadians &&
                rig.RightGripAgreement().positionErrorMetres <=
                    kPlayerGripSocketToleranceMetres &&
                rig.RightGripAgreement().orientationErrorRadians <=
                    kPlayerGripOrientationToleranceRadians;
        }
        allRigidTransformsValid &= ValidateHeldItemSocketTransform(
            rendered[1].worldFromItem, diagnostic);
        allRigidTransformsValid &= rendered[0].id == HeldItemId::OriginalTorch &&
            rendered[1].id == HeldItemId::Sword;
        HeldItemTransform finalHandItem{};
        allRigidTransformsValid &= ComposeWorldFromItem(
            finalGrip, SwordGripSocketTransform(), finalHandItem, diagnostic);
        allRigidTransformsValid &= TransformNear(
            rendered[1].worldFromItem, desiredItem, 0.0002f);
        if (!haveTorch)
        {
            originalTorch = rendered[0].worldFromItem;
            haveTorch = true;
        }
        allRigidTransformsValid &= TransformNear(
            rendered[0].worldFromItem, originalTorch);
        if (havePrevious)
        {
            const float dx = rendered[1].worldFromItem[12] - previous[12];
            const float dy = rendered[1].worldFromItem[13] - previous[13];
            const float dz = rendered[1].worldFromItem[14] - previous[14];
            maximumPositionStep = std::max(maximumPositionStep,
                std::sqrt(dx*dx + dy*dy + dz*dz));
        }
        previous = rendered[1].worldFromItem;
        havePrevious = true;
    }
    Check(swordState.parentMode == HeldItemParentMode::HandSocket &&
              swordState.visualStowBlend == 0.0f &&
              swordState.visualGripBlend == 1.0f &&
              !swordState.transition.active,
          "the complete fixed-tick Draw must finish in its stable hand-owned endpoint");
    Check(allGripTargetsResolved && maximumGripPositionError <=
              kPlayerGripSocketToleranceMetres && maximumGripOrientationError <=
              kPlayerGripOrientationToleranceRadians,
          "actual right-hand rig must hold the Grip through the stow reach, attachment edge, and moving draw half");
    Check(allRigidTransformsValid && maximumPositionStep < 0.35f,
          "the single rendered sword transform must remain rigid and continuous on both sides of the attachment edge");

    // Reversal is driven by the snapshot-safe blend rather than a separate
    // renderer clock: copied state produces the same physical transform.
    HeldItemState interrupted = MakeHeldItemState(
        HeldItemId::Sword, HeldHand::RightHand, HeldItemParentMode::BodyStow);
    RequestHeldItemTransition(interrupted, HeldItemTransitionKind::Draw, 20u);
    AdvanceHeldItemTransition(interrupted, 21u, 0.12f);
    const HeldItemState saved = interrupted;
    Check(ValidateHeldItemState(saved) &&
              saved.visualStowBlend == interrupted.visualStowBlend,
          "saved mid-draw ownership blend must remain a valid resumable snapshot");
    HeldItemTransform hipsBeforeReverse{}, bodyBeforeReverse{}, gripBeforeReverse{},
        targetGripBeforeReverse{}, targetItemBeforeReverse{};
    std::array<float, 3u> rootBeforeReverse{};
    HeldItemStates renderedBeforeReverse{};
    const bool beforeResolved = ResolveProductionSwordStowPose(
        input, interrupted, rig, tick++, hipsBeforeReverse, bodyBeforeReverse,
        gripBeforeReverse, targetGripBeforeReverse, targetItemBeforeReverse,
        rootBeforeReverse, renderedBeforeReverse, diagnostic);
    const float beforeReverse = interrupted.visualStowBlend;
    const auto reverse = RequestHeldItemTransition(
        interrupted, HeldItemTransitionKind::Stow, 22u);
    HeldItemTransform hipsAfterReverse{}, bodyAfterReverse{}, gripAfterReverse{},
        targetGripAfterReverse{}, targetItemAfterReverse{};
    std::array<float, 3u> rootAfterReverse{};
    HeldItemStates renderedAfterReverse{};
    const bool afterResolved = ResolveProductionSwordStowPose(
        input, interrupted, rig, tick++, hipsAfterReverse, bodyAfterReverse,
        gripAfterReverse, targetGripAfterReverse, targetItemAfterReverse,
        rootAfterReverse, renderedAfterReverse, diagnostic);
    Check((reverse.status == HeldItemTransitionRequestStatus::Started ||
           reverse.status == HeldItemTransitionRequestStatus::InterruptedAndStarted) &&
              Near(interrupted.visualStowBlend, beforeReverse) &&
              ValidateHeldItemState(interrupted),
          "reversing an in-flight draw must preserve the exact displayed item blend");
    Check(beforeResolved && afterResolved &&
              TransformNear(renderedBeforeReverse[1].worldFromItem,
                            renderedAfterReverse[1].worldFromItem, 0.000001f),
          "a real-rig reversal request must preserve the exact rendered sword matrix on that fixed frame");
    HeldItemTransform hipsAtDeath{}, bodyAtDeath{}, gripAtDeath{}, targetGripAtDeath{},
        targetItemAtDeath{};
    std::array<float, 3u> rootAtDeath{};
    HeldItemStates renderedAtDeath{};
    const HeldItemState deathSnapshot = interrupted;
    const bool beforeDeathResolved = ResolveProductionSwordStowPose(
        input, deathSnapshot, rig, tick++, hipsAtDeath, bodyAtDeath, gripAtDeath,
        targetGripAtDeath, targetItemAtDeath, rootAtDeath, renderedAtDeath, diagnostic);
    const bool interruptedForDeath = InterruptHeldItemTransition(interrupted, 23u);
    const HeldItemState restoredDeathSnapshot = interrupted;
    HeldItemTransform hipsRestored{}, bodyRestored{}, gripRestored{}, targetGripRestored{},
        targetItemRestored{};
    std::array<float, 3u> rootRestored{};
    HeldItemStates renderedRestored{};
    const bool afterDeathResolved = ResolveProductionSwordStowPose(
        input, restoredDeathSnapshot, rig, tick++, hipsRestored, bodyRestored,
        gripRestored, targetGripRestored, targetItemRestored, rootRestored,
        renderedRestored, diagnostic);
    Check(beforeDeathResolved && interruptedForDeath &&
              ValidateHeldItemState(restoredDeathSnapshot) && afterDeathResolved &&
              TransformNear(renderedAtDeath[1].worldFromItem,
                            renderedRestored[1].worldFromItem, 0.000001f),
          "death interruption and copied recovery must render the same actual-rig sword matrix without a jump");
}

void TestActualRigSwordSheathReachesGripBeforeAttachmentThenReleases()
{
    using namespace horde::gameplay::items;
    using namespace horde::vulkan::raytracing;
    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    PlayerRenderSlot rig;
    std::string diagnostic;
    Check(rig.LoadAsset((root / "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb").string(),
                        diagnostic),
          "sheathing must use the actual imported player rig");
    if (!rig.IsLoaded()) return;

    HeldItemFixedStepInput input;
    input.playerMountProfile = PlayerMountProfile::AnatomicalBody;
    input.playerX = -1.25f;
    input.playerZ = -8.4f;
    input.playerYawRadians = 0.42f;
    input.walkTime = 0.31f;
    input.walkAmount = 0.55f;
    HeldItemState swordState = MakeHeldItemState(
        HeldItemId::Sword, HeldHand::RightHand);
    Check(RequestHeldItemTransition(swordState, HeldItemTransitionKind::Sheath, 1u).status ==
              HeldItemTransitionRequestStatus::Started,
          "actual-rig sheath fixture must start with the existing hand-owned sword");

    HeldItemTransform previous{};
    HeldItemTransform originalTorch{};
    bool havePrevious = false;
    bool haveTorch = false;
    bool allAttachedGripSamplesMatch = true;
    bool reachedEdge = false;
    bool handReleased = false;
    float maximumGripPositionError = 0.0f;
    float maximumGripOrientationError = 0.0f;
    float maximumPositionStep = 0.0f;
    float minimumReleasedHandDistance = std::numeric_limits<float>::infinity();
    std::uint64_t tick = 1u;
    for (std::uint32_t frame = 0u; frame <= 22u; ++frame)
    {
        if (frame > 0u)
        {
            const auto advance = AdvanceHeldItemTransition(
                swordState, tick, 1.0f / 60.0f);
            reachedEdge |= advance.attachmentChanged;
        }
        HeldItemTransform hips{}, bodyStow{}, finalGrip{}, desiredGrip{}, desiredItem{};
        std::array<float, 3u> playerRoot{};
        HeldItemStates rendered{};
        if (!ResolveProductionSwordStowPose(input, swordState, rig, tick++,
                hips, bodyStow, finalGrip, desiredGrip, desiredItem,
                playerRoot, rendered, diagnostic))
        {
            std::cerr << "Sword sheath actual-rig diagnostic: progress="
                      << swordState.transition.progress << " blend="
                      << swordState.visualStowBlend << '/' << swordState.visualGripBlend
                      << " :: " << diagnostic << '\n';
            allAttachedGripSamplesMatch = false;
            continue;
        }
        if (swordState.visualGripBlend >= 0.999f)
        {
            const auto agreement = MeasureTransformAgreement(desiredGrip, finalGrip);
            maximumGripPositionError = std::max(maximumGripPositionError,
                                                agreement.positionErrorMetres);
            maximumGripOrientationError = std::max(maximumGripOrientationError,
                                                    agreement.orientationErrorRadians);
            allAttachedGripSamplesMatch &=
                agreement.positionErrorMetres <= kPlayerGripSocketToleranceMetres &&
                agreement.orientationErrorRadians <= kPlayerGripOrientationToleranceRadians &&
                rig.RightGripAgreement().positionErrorMetres <=
                    kPlayerGripSocketToleranceMetres &&
                rig.RightGripAgreement().orientationErrorRadians <=
                    kPlayerGripOrientationToleranceRadians;
        }
        else if (swordState.visualGripBlend <= 0.001f)
        {
            handReleased = true;
            minimumReleasedHandDistance = std::min(
                minimumReleasedHandDistance,
                MeasureTransformAgreement(desiredGrip, finalGrip).positionErrorMetres);
        }
        if (!haveTorch)
        {
            originalTorch = rendered[0].worldFromItem;
            haveTorch = true;
        }
        allAttachedGripSamplesMatch &=
            TransformNear(rendered[0].worldFromItem, originalTorch);
        if (havePrevious)
        {
            const float dx = rendered[1].worldFromItem[12] - previous[12];
            const float dy = rendered[1].worldFromItem[13] - previous[13];
            const float dz = rendered[1].worldFromItem[14] - previous[14];
            maximumPositionStep = std::max(maximumPositionStep,
                std::sqrt(dx*dx + dy*dy + dz*dz));
        }
        previous = rendered[1].worldFromItem;
        havePrevious = true;
    }
    Check(reachedEdge && swordState.parentMode == HeldItemParentMode::BodyStow &&
              !swordState.transition.active && swordState.visualStowBlend == 1.0f &&
              swordState.visualGripBlend == 0.0f,
          "sheath must publish one midpoint ownership edge and finish with the item stowed");
    Check(allAttachedGripSamplesMatch && maximumGripPositionError <=
              kPlayerGripSocketToleranceMetres && maximumGripOrientationError <=
              kPlayerGripOrientationToleranceRadians,
          "the actual right hand must retain exact sword Grip through item travel and release only after the edge");
    Check(handReleased && minimumReleasedHandDistance >= 0.10f &&
              maximumPositionStep < 0.35f,
          "the sheath must release the hand at the mounted endpoint without a sword-transform pop");
}

void TestFullScabbardMeshFitsAnimatedPlayerAndRouteFloor()
{
    using namespace horde::gameplay::items;
    using namespace horde::vulkan::raytracing;
    using namespace horde::scene::assets;
    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    AssetManifest playerManifest;
    AssetManifest scabbardManifest;
    StaticMeshAsset playerAsset;
    StaticMeshAsset scabbardAsset;
    std::string diagnostic;
    const auto playerDirectory = root / "assets/models/player/runtime";
    const auto scabbardDirectory = root / "assets/models/props/runtime/player-sword-scabbard";
    bool loaded = AssetManifest::Load(playerDirectory / "asset.manifest.json",
                                      playerManifest, diagnostic) &&
        StaticMeshAsset::Load(playerDirectory / "gothic-traveller-lod0.runtime.glb",
                              playerManifest, playerAsset, diagnostic) &&
        AssetManifest::Load(scabbardDirectory / "asset.manifest.json",
                            scabbardManifest, diagnostic) &&
        StaticMeshAsset::Load(scabbardDirectory / "player-sword-scabbard-lod0.runtime.glb",
                              scabbardManifest, scabbardAsset, diagnostic);
    Check(loaded, "full imported scabbard and player meshes must load for body/floor fit checks");
    if (!loaded) return;

    PlayerRenderSlot rig;
    loaded = rig.LoadAsset((playerDirectory / "gothic-traveller-lod0.runtime.glb").string(),
                           diagnostic) &&
             rig.ValidateStaticVertexLayout(playerAsset, diagnostic);
    Check(loaded && scabbardAsset.indices.size() == 912u,
          "full mesh checks must use the exact validated player vertex/index correspondence and all 304 scabbard triangles");
    if (!loaded || scabbardAsset.indices.size() != 912u) return;

    struct Triangle
    {
        std::array<std::array<float, 3u>, 3u> point{};
        std::array<float, 3u> minimum{};
        std::array<float, 3u> maximum{};
    };
    const auto makeTriangle = [](const std::array<std::array<float, 3u>, 3u>& points) {
        Triangle result;
        result.point = points;
        for (std::size_t axis = 0u; axis < 3u; ++axis)
        {
            result.minimum[axis] = std::min({points[0][axis], points[1][axis], points[2][axis]});
            result.maximum[axis] = std::max({points[0][axis], points[1][axis], points[2][axis]});
        }
        return result;
    };
    const auto boundsDistanceSquared = [](const Triangle& left, const Triangle& right) {
        float result = 0.0f;
        for (std::size_t axis = 0u; axis < 3u; ++axis)
        {
            const float gap = left.maximum[axis] < right.minimum[axis]
                ? right.minimum[axis] - left.maximum[axis]
                : right.maximum[axis] < left.minimum[axis]
                    ? left.minimum[axis] - right.maximum[axis] : 0.0f;
            result += gap * gap;
        }
        return result;
    };

    const auto bodyMountInput = [&] (const float yaw, const float walkTime,
                                    const float walkAmount, const float playerZ) {
        HeldItemFixedStepInput input;
        input.playerMountProfile = PlayerMountProfile::AnatomicalBody;
        input.playerX = -5.5f;
        input.playerZ = playerZ;
        input.playerYawRadians = yaw;
        input.walkTime = walkTime;
        input.walkAmount = walkAmount;
        return input;
    };
    const std::array<float, 4u> yaws{{0.0f, 1.570796327f, 3.141592654f, -1.570796327f}};
    const auto& lowLintel = horde::scene::kShowcaseLowOverheadVolumes[0];
    const float lowLintelCenterZ = 0.5f * (lowLintel.footprint[0][1] + lowLintel.footprint[2][1]);
    const std::array<float, 2u> routePositions{{-14.5f, lowLintelCenterZ + 0.70f}};
    float minimumBodyGap = std::numeric_limits<float>::infinity();
    float minimumFloorGap = std::numeric_limits<float>::infinity();
    float minimumReleasedHandDistance = std::numeric_limits<float>::infinity();
    std::size_t closestPlayerTriangle = 0u;
    std::size_t closestScabbardTriangle = 0u;
    std::size_t poseCount = 0u;
    bool allPosesResolved = true;
    for (const float playerZ : routePositions)
        for (const float yaw : yaws)
            for (const auto& locomotion : std::array<std::array<float, 2u>, 2u>{{
                    {{0.0f, 0.0f}}, {{0.5f, 0.65f}}}})
            {
                const auto input = bodyMountInput(yaw, locomotion[0], locomotion[1], playerZ);
                HeldItemTransform hips{}, bodyStow{}, finalGrip{}, desiredGrip{}, desiredItem{};
                std::array<float, 3u> playerRoot{};
                HeldItemStates rendered{};
                const HeldItemState stowedSword = MakeHeldItemState(
                    HeldItemId::Sword, HeldHand::RightHand,
                    HeldItemParentMode::BodyStow);
                if (!ResolveProductionSwordStowPose(input, stowedSword, rig,
                        ++poseCount, hips, bodyStow, finalGrip, desiredGrip,
                        desiredItem, playerRoot, rendered, diagnostic))
                {
                    allPosesResolved = false;
                    continue;
                }
                const auto basis = BuildPlayerModelWorldBasis(
                    {{std::cos(yaw), 0.0f, std::sin(yaw)}},
                    {{std::sin(yaw), 0.0f, -std::cos(yaw)}});
                const auto handToSwordGrip = MeasureTransformAgreement(
                    desiredGrip, finalGrip).positionErrorMetres;
                minimumReleasedHandDistance = std::min(minimumReleasedHandDistance,
                                                       handToSwordGrip);

                std::vector<Triangle> sheathTriangles;
                sheathTriangles.reserve(scabbardAsset.indices.size() / 3u);
                std::array<float, 3u> sheathMinimum{{
                    std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity()}};
                std::array<float, 3u> sheathMaximum{{
                    -std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity()}};
                for (const auto& vertex : scabbardAsset.vertices)
                {
                    const auto point = TransformPoint(bodyStow,
                        {{vertex.position[0], vertex.position[1], vertex.position[2]}});
                    minimumFloorGap = std::min(minimumFloorGap,
                        point[1] - horde::gameplay::kRouteFloorWorldY);
                    for (std::size_t axis = 0u; axis < 3u; ++axis)
                    {
                        sheathMinimum[axis] = std::min(sheathMinimum[axis], point[axis]);
                        sheathMaximum[axis] = std::max(sheathMaximum[axis], point[axis]);
                    }
                }
                for (const auto& primitive : scabbardAsset.primitives)
                    for (std::uint32_t offset = 0u; offset < primitive.indexCount; offset += 3u)
                    {
                        std::array<std::array<float, 3u>, 3u> points{};
                        for (std::size_t corner = 0u; corner < 3u; ++corner)
                        {
                            const auto index = scabbardAsset.indices[
                                primitive.indexOffset + offset + corner];
                            const auto& vertex = scabbardAsset.vertices.at(
                                primitive.vertexOffset + index);
                            points[corner] = TransformPoint(bodyStow,
                                {{vertex.position[0], vertex.position[1], vertex.position[2]}});
                        }
                        sheathTriangles.push_back(makeTriangle(points));
                    }

                const auto& skinned = rig.UniqueVertices();
                std::vector<Triangle> playerTriangles;
                for (const auto& primitive : playerAsset.primitives)
                    for (std::uint32_t offset = 0u; offset < primitive.indexCount; offset += 3u)
                    {
                        std::array<std::array<float, 3u>, 3u> points{};
                        for (std::size_t corner = 0u; corner < 3u; ++corner)
                        {
                            const auto index = playerAsset.indices[
                                primitive.indexOffset + offset + corner];
                            const auto vertexIndex = primitive.vertexOffset + index;
                            if (vertexIndex >= skinned.size())
                            {
                                allPosesResolved = false;
                                continue;
                            }
                            const auto& vertex = skinned[vertexIndex];
                            points[corner] = Add(playerRoot,
                                PlayerModelVectorToWorld(basis,
                                    {{vertex.position[0], vertex.position[1], vertex.position[2]}}));
                        }
                        playerTriangles.push_back(makeTriangle(points));
                    }
                float poseMinimum = std::numeric_limits<float>::infinity();
                for (std::size_t bodyIndex = 0u; bodyIndex < playerTriangles.size(); ++bodyIndex)
                {
                    const auto& body = playerTriangles[bodyIndex];
                    for (std::size_t sheathIndex = 0u;
                         sheathIndex < sheathTriangles.size(); ++sheathIndex)
                    {
                        const auto& sheath = sheathTriangles[sheathIndex];
                        const float lowerBound = boundsDistanceSquared(body, sheath);
                        if (lowerBound >= poseMinimum * poseMinimum) continue;
                        const float distance = std::sqrt(TriangleDistanceSquared(
                            body.point, sheath.point));
                        if (distance < poseMinimum)
                        {
                            poseMinimum = distance;
                            if (distance < minimumBodyGap)
                            {
                                minimumBodyGap = distance;
                                closestPlayerTriangle = bodyIndex;
                                closestScabbardTriangle = sheathIndex;
                            }
                        }
                    }
                }
            }

    std::cout << "stowed scabbard full-mesh sweep poses=" << poseCount
              << " closestGap=" << minimumBodyGap << "m playerTri="
              << closestPlayerTriangle << " sheathTri=" << closestScabbardTriangle
              << " floorGap=" << minimumFloorGap << "m releasedHand="
              << minimumReleasedHandDistance << "m\n";
    Check(allPosesResolved && poseCount == 16u && minimumBodyGap >= 0.015f &&
              minimumFloorGap >= 0.10f && minimumReleasedHandDistance >= 0.10f,
          "full authored sheath triangles must clear the skinned body and route floor while the right hand releases on Idle/Walking yaw sweeps");
}

void TestSwordOverheadClearanceUsesImportedBladeAcrossCombatPhases()
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::items;
    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    horde::scene::assets::StaticMeshAsset sword;
    horde::scene::assets::StaticMeshAsset legacyTorch;
    std::string diagnostic;
    Check(LoadProductionHeldAssets(sword, legacyTorch, diagnostic),
          "production sword must load before full-mesh overhead clearance checks");
    if (sword.vertices.empty()) return;
    const auto* grip = FindHeldItemSocket(sword.sockets, "Grip");
    Check(grip != nullptr && TransformNear(grip->world, SwordGripSocketTransform()),
          "overhead clearance must use the exact imported production Grip");
    if (grip == nullptr) return;

    float minX = 1.0e9f, minY = 1.0e9f, minZ = 1.0e9f;
    float maxX = -1.0e9f, maxY = -1.0e9f, maxZ = -1.0e9f;
    for (const auto& vertex : sword.vertices)
    {
        minX = std::min(minX, vertex.position[0] - grip->world[12]);
        minY = std::min(minY, vertex.position[1] - grip->world[13]);
        minZ = std::min(minZ, vertex.position[2] - grip->world[14]);
        maxX = std::max(maxX, vertex.position[0] - grip->world[12]);
        maxY = std::max(maxY, vertex.position[1] - grip->world[13]);
        maxZ = std::max(maxZ, vertex.position[2] - grip->world[14]);
    }
    Check(minX >= -0.113f && maxX <= 0.113f && minY >= -0.136f &&
              maxY <= 0.916f && minZ >= -0.026f && maxZ <= 0.026f,
          "clearance envelope must continue to contain every imported sword vertex relative to Grip");

    HeldItemKinematicsInput openInput;
    openInput.playerMountProfile = PlayerMountProfile::AnatomicalBody;
    openInput.cameraX = kSkylightChamberCenter.x;
    openInput.cameraZ = kSkylightChamberCenter.z + 0.70f;
    const auto open = EvaluateHeldItemKinematics(openInput);
    std::cout << "open anatomical hands right=" << open.rightHandLocal[0] << ','
              << open.rightHandLocal[1] << ',' << open.rightHandLocal[2]
              << " left=" << open.leftHandLocal[0] << ',' << open.leftHandLocal[1]
              << ',' << open.leftHandLocal[2] << " lowering/retraction="
              << open.swordOverheadLowering << '/' << open.swordOverheadRetraction
              << " torch=" << open.torchOverheadLowering << '\n';
    Check(Near(open.swordOverheadLowering, 0.0f) &&
              Near(open.rightHandLocal[0], 0.18f) && Near(open.rightHandLocal[1], -0.34f) &&
              Near(open.rightHandLocal[2], 0.60f) &&
              Near(open.swordOverheadRetraction, 0.0f) &&
              Near(open.torchOverheadLowering, 0.0f) &&
              Near(open.leftHandLocal[0], -0.135f) && Near(open.leftHandLocal[1], -0.31f),
          "open-room AnatomicalBody sword and Rag torch hand poses must retain their exact existing targets");

    struct CombatPhase
    {
        PlayerCombatAction action;
        float duration;
        bool parry;
    };
    const std::array<CombatPhase, 10u> phases{{
        {PlayerCombatAction::Idle, 0.0f, false},
        {PlayerCombatAction::SwingWindup, SwordCombat::kSwingWindupDuration * 0.5f, false},
        {PlayerCombatAction::SwingActive, SwordCombat::kDownwardCutTravelDuration * 0.5f, false},
        {PlayerCombatAction::SwingRecovery, SwordCombat::kSwingRecoveryDuration * 0.5f, false},
        {PlayerCombatAction::UpwardSliceWindup, SwordCombat::kUpwardSliceWindupDuration * 0.5f, false},
        {PlayerCombatAction::UpwardSliceActive, SwordCombat::kUpwardSliceActiveDuration * 0.5f, false},
        {PlayerCombatAction::UpwardSliceRecovery, SwordCombat::kUpwardSliceRecoveryDuration * 0.5f, false},
        {PlayerCombatAction::ParryStartup, SwordCombat::kParryStartupDuration * 0.5f, true},
        {PlayerCombatAction::ParryActive, SwordCombat::kParryActiveDuration * 0.5f, true},
        {PlayerCombatAction::ParryRecovery, SwordCombat::kParryRecoveryDuration * 0.5f, true}}};
    constexpr std::array<float, 3u> pitches{{-0.32f, 0.0f, 0.28f}};
    constexpr std::array<std::size_t, 2u> lintelIndices{{0u, 1u}};
    bool allTransformsValid = true;
    bool allHandsMatchGrip = true;
    bool allFinalRigGripsMatch = true;
    bool allFinalRigVerticesClear = true;
    bool allVerticesClear = true;
    bool observedLowering = false;
    bool observedRetraction = false;
    bool printedRigDiagnostic = false;
    std::size_t finalRigResolvedCount = 0u;
    std::size_t leftRigSocketFailureCount = 0u;
    float maximumLeftRigSocketError = 0.0f;
    std::size_t totalPoseCount = 0u;
    horde::vulkan::raytracing::PlayerRenderSlot rig;
    Check(rig.LoadAsset((root / "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb").string(), diagnostic),
          "production anatomical sword checks must load the actual player rig and Grip bones");
    if (!rig.IsLoaded()) return;
    float maximumLowering = 0.0f;
    std::uint64_t rigTick = 1u;
    for (const std::size_t lintelIndex : lintelIndices)
    {
        const auto& lintel = horde::scene::kShowcaseLowOverheadVolumes[lintelIndex];
        const float centerX = 0.5f * (lintel.footprint[0][0] + lintel.footprint[2][0]);
        const float centerZ = 0.5f * (lintel.footprint[0][1] + lintel.footprint[2][1]);
        for (const float pitch : pitches)
            for (const auto& phase : phases)
            {
                HeldItemFixedStepInput input;
                input.playerX = centerX;
                input.playerZ = centerZ + 0.77f;
                input.playerYawRadians = 0.0f;
                input.playerPitchRadians = pitch;
                input.playerMountProfile = PlayerMountProfile::AnatomicalBody;
                input.playerCombat.action = phase.action;
                input.playerCombat.actionTime = phase.duration;
                if (phase.parry)
                {
                    input.playerCombat.reaction = CombatReaction::Parried;
                    input.playerCombat.reactionTime = 0.06f;
                }
                ++totalPoseCount;
                HeldItemStates items = MakeDefaultHeldItemStates();
                HeldItemFixedStepState state;
                allTransformsValid &= ResolveHeldItemsFixedStep(items, input, 1u, state, diagnostic) &&
                    ValidateHeldItemSocketTransform(items[1].worldFromItem, diagnostic);
                const auto worldFromGrip = MultiplyHeldItemTransforms(
                    items[1].worldFromItem, grip->world);
                const auto expectedHand = ExpectedWorldHandPoint(
                    input, state.kinematics.rightHandLocal);
                allHandsMatchGrip &= Near(worldFromGrip[12], expectedHand[0], 0.0002f) &&
                    Near(worldFromGrip[13], expectedHand[1], 0.0002f) &&
                    Near(worldFromGrip[14], expectedHand[2], 0.0002f);
                HeldItemTransform finalRigGrip{};
                HeldItemTransform finalRigSword{};
                const bool finalRigResolved = ResolveProductionAnatomicalSword(
                    input, state.kinematics, items, rig, rigTick++,
                    finalRigGrip, finalRigSword, diagnostic);
                if (!finalRigResolved && !printedRigDiagnostic)
                {
                    std::cerr << "Anatomical sword rig diagnostic: lintel=" << lintelIndex
                              << " pitch=" << pitch << " action="
                              << static_cast<int>(phase.action) << " lowering/retraction="
                              << state.kinematics.swordOverheadLowering << '/'
                              << state.kinematics.swordOverheadRetraction << " torch="
                              << state.kinematics.torchOverheadLowering << " left="
                              << state.kinematics.leftHandLocal[0] << ','
                              << state.kinematics.leftHandLocal[1] << ','
                              << state.kinematics.leftHandLocal[2] << " right="
                              << state.kinematics.rightHandLocal[0] << ','
                              << state.kinematics.rightHandLocal[1] << ','
                              << state.kinematics.rightHandLocal[2] << " :: "
                              << diagnostic << '\n';
                    printedRigDiagnostic = true;
                }
                finalRigResolvedCount += finalRigResolved ? 1u : 0u;
                maximumLeftRigSocketError = std::max(maximumLeftRigSocketError,
                    rig.LeftSocketErrorMetres());
                leftRigSocketFailureCount +=
                    rig.LeftSocketErrorMetres() >
                        horde::vulkan::raytracing::kPlayerGripSocketToleranceMetres
                        ? 1u : 0u;
                allFinalRigGripsMatch &= finalRigResolved &&
                    rig.RightSocketErrorMetres() <=
                        horde::vulkan::raytracing::kPlayerGripSocketToleranceMetres &&
                    TransformNear(MultiplyHeldItemTransforms(finalRigSword, grip->world),
                                  finalRigGrip, 0.015f);
                if (finalRigResolved)
                    for (const auto& vertex : sword.vertices)
                    {
                        const auto point = TransformPoint(finalRigSword,
                            {{vertex.position[0], vertex.position[1], vertex.position[2]}});
                        if (PointInsideOverheadFootprint(lintel, point[0], point[2]))
                        {
                            const float roofBottom = horde::scene::MinimumOverheadBottomY(
                                lintel, point[0], point[2], 0.0f);
                            allFinalRigVerticesClear &=
                                point[1] + kHeldTorchOverheadGap <= roofBottom + 0.002f;
                        }
                    }
                observedLowering |= state.kinematics.swordOverheadLowering > 0.0f;
                observedRetraction |= state.kinematics.swordOverheadRetraction > 0.0f;
                maximumLowering = std::max(maximumLowering,
                                           state.kinematics.swordOverheadLowering);
                for (const auto& vertex : sword.vertices)
                {
                    const auto point = TransformPoint(items[1].worldFromItem,
                        {{vertex.position[0], vertex.position[1], vertex.position[2]}});
                    if (PointInsideOverheadFootprint(lintel, point[0], point[2]))
                    {
                        const float roofBottom = horde::scene::MinimumOverheadBottomY(
                            lintel, point[0], point[2], 0.0f);
                        allVerticesClear &= point[1] + kHeldTorchOverheadGap <= roofBottom + 0.002f;
                    }
                }
            }
    }
    std::cout << "sword overhead phase sweep max lowering/vertices clear="
              << maximumLowering << '/' << allVerticesClear
              << " anatomical finalGrip/vertices=" << allFinalRigGripsMatch << '/'
              << allFinalRigVerticesClear << " resolved=" << finalRigResolvedCount
              << '/' << totalPoseCount << " leftSocketErrorMax/failures="
              << maximumLeftRigSocketError << '/' << leftRigSocketFailureCount
              << " rightSocketError=" << rig.RightSocketErrorMetres()
              << " finalGripError=" << rig.RightGripAgreement().positionErrorMetres
              << "m/" << rig.RightGripAgreement().orientationErrorRadians << "rad\n";
    Check(observedLowering && observedRetraction && allTransformsValid && allHandsMatchGrip &&
              finalRigResolvedCount == totalPoseCount && allFinalRigGripsMatch &&
              allVerticesClear && allFinalRigVerticesClear,
          "imported sword vertices must clear both low lintels across pitch, swing, upward-slice and parry poses after the actual AnatomicalBody rig solves the final Grip");

    const auto& approachLintel = horde::scene::kShowcaseLowOverheadVolumes[0];
    const float approachCenterZ = 0.5f *
        (approachLintel.footprint[0][1] + approachLintel.footprint[2][1]);
    HeldItemFixedStepInput approachInput;
    approachInput.playerMountProfile = PlayerMountProfile::AnatomicalBody;
    approachInput.playerZ = approachCenterZ + 1.20f;
    approachInput.playerCombat.action = PlayerCombatAction::SwingActive;
    approachInput.playerCombat.actionTime = SwordCombat::kDownwardCutTravelDuration * 0.5f;
    float priorLowering = 0.0f;
    float priorHandY = 0.0f;
    float priorHandZ = 0.0f;
    float maximumApproachLoweringStep = 0.0f;
    float maximumApproachHandStep = 0.0f;
    bool havePreviousApproach = false;
    for (int step = 0; step <= 160; ++step)
    {
        approachInput.playerZ = approachCenterZ + 1.20f -
            static_cast<float>(step) * 0.005f;
        HeldItemStates items = MakeDefaultHeldItemStates();
        HeldItemFixedStepState state;
        Check(ResolveHeldItemsFixedStep(items, approachInput,
                    static_cast<std::uint64_t>(step + 1), state, diagnostic),
              "five-millimetre sword approach samples must resolve through shared kinematics");
        if (havePreviousApproach)
        {
            maximumApproachLoweringStep = std::max(maximumApproachLoweringStep,
                std::abs(state.kinematics.swordOverheadLowering - priorLowering));
            maximumApproachHandStep = std::max(maximumApproachHandStep,
                std::hypot(state.kinematics.rightHandLocal[1] - priorHandY,
                           state.kinematics.rightHandLocal[2] - priorHandZ));
        }
        priorLowering = state.kinematics.swordOverheadLowering;
        priorHandY = state.kinematics.rightHandLocal[1];
        priorHandZ = state.kinematics.rightHandLocal[2];
        havePreviousApproach = true;
    }
    std::cout << "sword five-mm approach max lowering/hand step="
              << maximumApproachLoweringStep << '/' << maximumApproachHandStep << '\n';
    Check(maximumApproachLoweringStep <= 0.025f && maximumApproachHandStep <= 0.035f,
          "sword overhead clearance must anticipate the low lintel continuously through a forward approach without a hand-frame pop");
}

} // namespace

void TestRewardCarryParryKeepsGuardOnSwordSide()
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::items;
    bool separated = true;
    bool leftGripUnchanged = true;
    const std::array<std::pair<PlayerCombatAction, float>, 3> phases{{
        {PlayerCombatAction::ParryStartup, SwordCombat::kParryStartupDuration},
        {PlayerCombatAction::ParryActive, SwordCombat::kParryActiveDuration},
        {PlayerCombatAction::ParryRecovery, SwordCombat::kParryRecoveryDuration}}};
    for (const auto carry : {interactions::HeldLightPose::High, interactions::HeldLightPose::Low})
        for (const auto& [action, duration] : phases)
            for (int sample = 0; sample <= 60; ++sample)
                for (float reactionTime : {0.0f, .06f, .12f})
                {
                    HeldItemKinematicsInput input;
                    input.interaction.heldLightKind = interactions::HeldLightKind::RewardLantern;
                    input.interaction.heldLightPose = carry;
                    const auto idle = EvaluateHeldItemKinematics(input);
                    input.playerCombat.action = action;
                    input.playerCombat.actionTime = duration * sample / 60.0f;
                    input.playerCombat.reaction = CombatReaction::Parried;
                    input.playerCombat.reactionTime = reactionTime;
                    const auto parry = EvaluateHeldItemKinematics(input);
                    separated &= parry.rightHandLocal[0] >= .0799f &&
                        parry.rightHandLocal[0] - parry.leftHandLocal[0] >= .18f;
                    leftGripUnchanged &= parry.leftHandLocal == idle.leftHandLocal &&
                        parry.leftGripXInView == idle.leftGripXInView &&
                        parry.leftGripYInView == idle.leftGripYInView &&
                        parry.leftGripZInView == idle.leftGripZInView;
                }
    Check(separated, "reward-carry parry must retain a sword-side guard through startup, active, recovery and success reaction");
    Check(leftGripUnchanged, "parry clearance must not be obtained by moving the lantern grip");
}

int main(const int argc, char** argv)
{
#if defined(_MSC_VER) && defined(_DEBUG)
    // CTest must receive diagnostics and a failure, never a blocking desktop
    // Retry/Ignore dialog. Bounds checks themselves remain enabled.
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
    if (argc > 1 && std::string(argv[1]) == "--combat-geometry")
    {
        TestCombatPulseAgainstImportedSwordAndSkeletonBounds(true);
        return failures == 0 ? 0 : 1;
    }
    TestSocketLookupIsNamedAndOrderIndependent();
    TestWorldFromItemUsesRequiredCompositionOrder();
    TestScaledGripSocketIsRejected();
    TestLeftAndRightHandsCannotBeSwapped();
    TestWallRetractionMovesHandAndAttachedItemTogether();
    TestRealFixedTickTorchDetachIsIndependentlyTransformContinuous();
    TestSharedFixedStepOwnsItemsKinematicsAndSocketLight();
    TestHeldItemSnapshotsAreRenderDeliveryInvariant();
    TestHeldItemBlasMeasurementsIncludeBothProductionAssets();
    TestHeldLightGpuAbiAppendsWithoutChangingReleasedBindings();
    TestResetAndCheckpointImportRestoreParentContracts();
    TestRenderSlotConvertsGenericTransformWithoutItemBranches();
    TestFlameAndLightSocketsFollowTheComposedItem();
    TestSimulationOwnsResetAndCheckpointParentState();
    TestSharedKinematicsOwnsWallDepthHandsAndSwordPose();
    TestRewardLanternHighLowUsesSharedLeftArmTarget();
    TestRewardCarryParryKeepsGuardOnSwordSide();
    TestProductionSwordAssetMeetsGenericSocketAndPbrBudget();
    TestProductionTorchAssetMeetsGenericSocketAndPbrBudget();
    TestProductionAssetsShareOneGenericStaticSlot();
    TestPlayerSwordScabbardUsesAppendedProductionPbrLayer();
    TestProductionSocketsMatchSharedFixedStepContracts();
    TestPlayerRagTorchSocketsDriveFixedStepAttachmentAndLight();
    TestProductionTorchFitsSharedClearanceEnvelope();
    TestRagTorchEnvelopeIncludesTheUnchangedEngineFire();
    TestActualRigSwordBodyStowAndContinuousDrawBlend();
    TestActualRigSwordSheathReachesGripBeforeAttachmentThenReleases();
    TestFullScabbardMeshFitsAnimatedPlayerAndRouteFloor();
    TestCombatPulseAgainstImportedSwordAndSkeletonBounds();
    TestSwordOverheadClearanceUsesImportedBladeAcrossCombatPhases();
    if (failures == 0)
    {
        std::cout << "Held-item socket contracts passed.\n";
    }
    return failures == 0 ? 0 : 1;
}
