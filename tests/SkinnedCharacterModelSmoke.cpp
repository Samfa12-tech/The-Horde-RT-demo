#include "scene/SkeletonBipedModel.h"
#include "scene/assets/AssetManifest.h"
#include "scene/assets/StaticMeshAsset.h"
#include "gameplay/DevelopmentCheckpointSimulation.h"
#include "gameplay/ShowcaseRoute.h"
#include "gameplay/simulation/GameSimulation.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace
{

std::filesystem::path FindRepoRoot()
{
    std::filesystem::path candidate = std::filesystem::current_path();
    for (int depth = 0; depth < 6; ++depth)
    {
        if (std::filesystem::exists(candidate / "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb")) return candidate;
        if (!candidate.has_parent_path()) break;
        candidate = candidate.parent_path();
    }
    return {};
}

bool Require(bool condition, const char* message)
{
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}

bool FiniteTexturedVertices(const std::vector<horde::scene::TexturedSkinnedRtVertex>& vertices)
{
    if (vertices.empty()) return false;
    bool sawDistinctUv = false;
    const float firstU = vertices.front().texcoord[0];
    const float firstV = vertices.front().texcoord[1];
    for (const auto& vertex : vertices)
    {
        for (float value : vertex.position) if (!std::isfinite(value)) return false;
        for (float value : vertex.normal) if (!std::isfinite(value)) return false;
        for (float value : vertex.texcoord) if (!std::isfinite(value)) return false;
        sawDistinctUv = sawDistinctUv || std::abs(vertex.texcoord[0] - firstU) > 0.0001f || std::abs(vertex.texcoord[1] - firstV) > 0.0001f;
        if (vertex.texcoord[2] != 0.0f || vertex.texcoord[3] != 0.0f) return false;
    }
    return sawDistinctUv;
}

using Vec3 = std::array<float, 3u>;
using HeldItemTransform = horde::gameplay::items::HeldItemTransform;

Vec3 Scale(const Vec3& value, const float scale)
{
    return {{value[0] * scale, value[1] * scale, value[2] * scale}};
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

Vec3 Normalise(const Vec3& value)
{
    const float length = std::sqrt(std::max(Dot(value, value), 0.0000001f));
    return Scale(value, 1.0f / length);
}

struct TestRigFrame
{
    horde::gameplay::animation::PlayerAnimationSnapshot animation{};
    Vec3 bodyRight{};
    Vec3 bodyForward{};
    horde::vulkan::raytracing::PlayerModelWorldBasis modelBasis{};
    Vec3 playerRootWorld{};
    Vec3 anchoredPlayerRootWorld{};
};

TestRigFrame BuildRigFrame(
    const horde::gameplay::simulation::SimulationSnapshot& source,
    const horde::scene::SkinnedNodeTransform& leftArmBase,
    const horde::scene::SkinnedNodeTransform& rightArmBase,
    const horde::vulkan::raytracing::PlayerRenderSlot& playerSlot)
{
    TestRigFrame result;
    result.animation = source.playerAnimation;
    const Vec3 worldUp{{0.0f, 1.0f, 0.0f}};
    const Vec3 bodyForward{{std::sin(source.playerYawRadians), 0.0f,
                            -std::cos(source.playerYawRadians)}};
    const Vec3 bodyRight{{std::cos(source.playerYawRadians), 0.0f,
                          std::sin(source.playerYawRadians)}};
    const auto lowerBody = horde::gameplay::EvaluateLowerBodyPose(
        source.walkTime, source.walkAmount);
    result.bodyForward = Normalise({{
        bodyForward[0] * std::cos(lowerBody.torsoTwistRadians) +
            bodyRight[0] * std::sin(lowerBody.torsoTwistRadians),
        0.0f,
        bodyForward[2] * std::cos(lowerBody.torsoTwistRadians) +
            bodyRight[2] * std::sin(lowerBody.torsoTwistRadians)}});
    result.bodyRight = Normalise({{
        bodyRight[0] * std::cos(lowerBody.torsoTwistRadians) -
            bodyForward[0] * std::sin(lowerBody.torsoTwistRadians),
        0.0f,
        bodyRight[2] * std::cos(lowerBody.torsoTwistRadians) -
            bodyForward[2] * std::sin(lowerBody.torsoTwistRadians)}});
    result.modelBasis = horde::vulkan::raytracing::BuildPlayerModelWorldBasis(
        result.bodyRight, result.bodyForward);
    const Vec3 eye{{source.playerX, horde::gameplay::kShowcaseEyeWorldY,
                    source.playerZ}};
    const Vec3 viewForward = Normalise({{
        std::sin(source.playerYawRadians),
        -0.05f + std::clamp(source.playerPitchRadians, -0.32f, 0.28f),
        -std::cos(source.playerYawRadians)}});
    const Vec3 viewRight = Normalise(Cross(viewForward, worldUp));
    const Vec3 viewUp = Normalise(Cross(viewRight, viewForward));
    const auto toWorld = [&eye, &viewRight, &viewUp, &viewForward](const Vec3& local) {
        return Vec3{{eye[0] + viewRight[0] * local[0] + viewUp[0] * local[1] +
                         viewForward[0] * local[2],
                     eye[1] + viewRight[1] * local[0] + viewUp[1] * local[1] +
                         viewForward[1] * local[2],
                     eye[2] + viewRight[2] * local[0] + viewUp[2] * local[1] +
                         viewForward[2] * local[2]}};
    };
    const Vec3 leftShoulder = toWorld(source.playerAnimation.leftIk.shoulder);
    const Vec3 rightShoulder = toWorld(source.playerAnimation.rightIk.shoulder);
    const Vec3 leftHand = toWorld(source.playerAnimation.leftIk.target);
    const Vec3 rightHand = toWorld(source.playerAnimation.rightIk.target);
    const Vec3 rigCenter{{
        (leftArmBase[12] + rightArmBase[12]) * 0.5f,
        (leftArmBase[13] + rightArmBase[13]) * 0.5f,
        (leftArmBase[14] + rightArmBase[14]) * 0.5f}};
    const Vec3 anchoredCenter = toWorld(
        horde::vulkan::raytracing::EvaluatePlayerTorsoAnchorLocal(
            source.playerAnimation));
    const Vec3 rigCenterWorld =
        horde::vulkan::raytracing::PlayerModelVectorToWorld(
            result.modelBasis, rigCenter);
    const Vec3 shoulderAnchoredRootWorld{{
        anchoredCenter[0] - rigCenterWorld[0],
        anchoredCenter[1] - rigCenterWorld[1],
        anchoredCenter[2] - rigCenterWorld[2]}};
    result.anchoredPlayerRootWorld =
        horde::vulkan::raytracing::GroundPlayerRootOnRouteFloor(
            shoulderAnchoredRootWorld, horde::gameplay::kRouteFloorWorldY,
            playerSlot.BootGroundingOffsetMetres(source.playerAnimation));
    result.playerRootWorld = result.anchoredPlayerRootWorld;
    const auto worldPointToPlayer = [&result](const Vec3& world) {
        const Vec3 delta{{world[0] - result.playerRootWorld[0],
                          world[1] - result.playerRootWorld[1],
                          world[2] - result.playerRootWorld[2]}};
        return horde::vulkan::raytracing::WorldVectorToPlayerModel(
            result.modelBasis, delta);
    };
    const auto viewVectorToPlayer = [&result, &viewRight, &viewUp,
                                     &viewForward](const Vec3& view) {
        const Vec3 world{{viewRight[0] * view[0] + viewUp[0] * view[1] +
                              viewForward[0] * view[2],
                          viewRight[1] * view[0] + viewUp[1] * view[1] +
                              viewForward[1] * view[2],
                          viewRight[2] * view[0] + viewUp[2] * view[1] +
                              viewForward[2] * view[2]}};
        return horde::vulkan::raytracing::WorldVectorToPlayerModel(
            result.modelBasis, world);
    };
    result.animation.leftIk.shoulder = worldPointToPlayer(leftShoulder);
    result.animation.leftIk.target = worldPointToPlayer(leftHand);
    result.animation.leftIk.pole = viewVectorToPlayer(source.playerAnimation.leftIk.pole);
    result.animation.leftIk.gripX = viewVectorToPlayer(source.playerAnimation.leftIk.gripX);
    result.animation.leftIk.gripY = viewVectorToPlayer(source.playerAnimation.leftIk.gripY);
    result.animation.leftIk.gripZ = viewVectorToPlayer(source.playerAnimation.leftIk.gripZ);
    result.animation.rightIk.shoulder = worldPointToPlayer(rightShoulder);
    result.animation.rightIk.target = worldPointToPlayer(rightHand);
    result.animation.rightIk.pole = viewVectorToPlayer(source.playerAnimation.rightIk.pole);
    result.animation.rightIk.gripX = viewVectorToPlayer(source.playerAnimation.rightIk.gripX);
    result.animation.rightIk.gripY = viewVectorToPlayer(source.playerAnimation.rightIk.gripY);
    result.animation.rightIk.gripZ = viewVectorToPlayer(source.playerAnimation.rightIk.gripZ);
    return result;
}

HeldItemTransform GripTransform(const Vec3& x,
                                const Vec3& y,
                                const Vec3& z,
                                const Vec3& position)
{
    return {{x[0], x[1], x[2], 0.0f,
             y[0], y[1], y[2], 0.0f,
             z[0], z[1], z[2], 0.0f,
             position[0], position[1], position[2], 1.0f}};
}

HeldItemTransform RigidWorldBoneTransform(
    const horde::scene::SkinnedNodeTransform& bone,
    const TestRigFrame& frame)
{
    const auto toWorld = [&frame](const Vec3& local) {
        return horde::vulkan::raytracing::PlayerModelVectorToWorld(
            frame.modelBasis, local);
    };
    Vec3 x = Normalise(toWorld({{bone[0], bone[1], bone[2]}}));
    const Vec3 rawY = toWorld({{bone[4], bone[5], bone[6]}});
    Vec3 y = Normalise({{rawY[0] - x[0] * Dot(rawY, x),
                         rawY[1] - x[1] * Dot(rawY, x),
                         rawY[2] - x[2] * Dot(rawY, x)}});
    Vec3 z = Normalise(Cross(x, y));
    const Vec3 rawZ = toWorld({{bone[8], bone[9], bone[10]}});
    if (Dot(z, rawZ) < 0.0f)
    {
        y = Scale(y, -1.0f);
        z = Scale(z, -1.0f);
    }
    const Vec3 localPosition{{bone[12], bone[13], bone[14]}};
    const Vec3 worldOffset = toWorld(localPosition);
    const Vec3 position{{frame.playerRootWorld[0] + worldOffset[0],
                         frame.playerRootWorld[1] + worldOffset[1],
                         frame.playerRootWorld[2] + worldOffset[2]}};
    HeldItemTransform result = GripTransform(x, y, z, position);
    return result;
}

HeldItemTransform FinalGrip(const horde::gameplay::items::HeldItemState& item)
{
    return horde::gameplay::items::MultiplyHeldItemTransforms(
        item.worldFromItem,
        item.id == horde::gameplay::items::HeldItemId::OriginalTorch
            ? horde::gameplay::items::OriginalTorchGripSocketTransform()
            : horde::gameplay::items::SwordGripSocketTransform());
}

float PositionError(const HeldItemTransform& left, const HeldItemTransform& right)
{
    return std::hypot(std::hypot(left[12] - right[12], left[13] - right[13]),
                      left[14] - right[14]);
}

float OrientationError(const HeldItemTransform& left, const HeldItemTransform& right)
{
    float maximum = 0.0f;
    for (std::size_t column = 0u; column < 3u; ++column)
    {
        const std::size_t offset = column * 4u;
        const float cosine = std::clamp(
            left[offset] * right[offset] + left[offset + 1u] * right[offset + 1u] +
                left[offset + 2u] * right[offset + 2u],
            -1.0f, 1.0f);
        maximum = std::max(maximum, std::acos(cosine));
    }
    return maximum;
}

Vec3 Add(const Vec3& left, const Vec3& right)
{
    return {{left[0] + right[0], left[1] + right[1], left[2] + right[2]}};
}

Vec3 Subtract(const Vec3& left, const Vec3& right)
{
    return {{left[0] - right[0], left[1] - right[1], left[2] - right[2]}};
}

float PointTriangleDistance(const Vec3& point,
                            const Vec3& a,
                            const Vec3& b,
                            const Vec3& c)
{
    const Vec3 ab = Subtract(b, a);
    const Vec3 ac = Subtract(c, a);
    const Vec3 ap = Subtract(point, a);
    const float d1 = Dot(ab, ap);
    const float d2 = Dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return std::sqrt(Dot(ap, ap));
    const Vec3 bp = Subtract(point, b);
    const float d3 = Dot(ab, bp);
    const float d4 = Dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return std::sqrt(Dot(bp, bp));
    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        const float v = d1 / (d1 - d3);
        const Vec3 delta = Subtract(point, Add(a, Scale(ab, v)));
        return std::sqrt(Dot(delta, delta));
    }
    const Vec3 cp = Subtract(point, c);
    const float d5 = Dot(ab, cp);
    const float d6 = Dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return std::sqrt(Dot(cp, cp));
    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        const float w = d2 / (d2 - d6);
        const Vec3 delta = Subtract(point, Add(a, Scale(ac, w)));
        return std::sqrt(Dot(delta, delta));
    }
    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && d4 - d3 >= 0.0f && d5 - d6 >= 0.0f)
    {
        const Vec3 bc = Subtract(c, b);
        const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        const Vec3 delta = Subtract(point, Add(b, Scale(bc, w)));
        return std::sqrt(Dot(delta, delta));
    }
    const Vec3 normal = Cross(ab, ac);
    const float normalLengthSquared = Dot(normal, normal);
    if (normalLengthSquared <= 1.0e-12f)
    {
        const auto segmentDistance = [&point](const Vec3& start,
                                              const Vec3& end) {
            const Vec3 extent = Subtract(end, start);
            const float amount = std::clamp(
                Dot(Subtract(point, start), extent) /
                    std::max(Dot(extent, extent), 1.0e-12f),
                0.0f, 1.0f);
            const Vec3 delta = Subtract(
                point, Add(start, Scale(extent, amount)));
            return std::sqrt(Dot(delta, delta));
        };
        return std::min({segmentDistance(a, b), segmentDistance(b, c),
                         segmentDistance(c, a)});
    }
    return std::abs(Dot(ap, normal)) / std::sqrt(normalLengthSquared);
}

struct PrimaryPlayerCameraMetrics
{
    float minimumTriangleDistanceMetres = INFINITY;
    std::uint32_t minimumTriangleFirstVertex = 0u;
    Vec3 minimumTriangleCentroid{};
    std::array<Vec3, 3u> minimumTriangleWorld{};
    Vec3 minimumTriangleBaselineVertex{};
    Vec3 minimumTriangleSkinnedVertex{};
    float minimumVertexDepthMetres = INFINITY;
    double clippedProjectedTriangleArea = 0.0;
    std::size_t sideStableVertexCount = 0u;
    std::size_t sideCrossingVertexCount = 0u;
    std::size_t nearPlaneCrossingTriangleCount = 0u;
    double nearPlaneCrossingProjectedArea = 0.0;
    float minimumWorldY = INFINITY;
    float maximumBaselineTriangleEdgeMetres = 0.0f;
    float maximumSkinnedTriangleEdgeMetres = 0.0f;
    double maximumProjectedTriangleArea = 0.0;
    float minimumShadingNormalGeometricDot = 1.0f;
    float minimumFaceForwardedNormalGeometricDot = 1.0f;
    std::size_t reversedShadingNormalCount = 0u;
};

PrimaryPlayerCameraMetrics MeasurePrimaryPlayerCameraMetrics(
    const horde::scene::assets::StaticMeshAsset& playerStatic,
    const std::vector<horde::scene::TexturedSkinnedRtVertex>& baselineVertices,
    const std::vector<horde::scene::TexturedSkinnedRtVertex>& vertices,
    const TestRigFrame& rig,
    const horde::gameplay::simulation::SimulationSnapshot& snapshot)
{
    PrimaryPlayerCameraMetrics metrics;
    for (std::size_t vertex = 0u;
         vertex < std::min(baselineVertices.size(), vertices.size()); ++vertex)
    {
        const float baselineX = baselineVertices[vertex].position[0];
        if (std::abs(baselineX) < 0.10f) continue;
        ++metrics.sideStableVertexCount;
        if (baselineX * vertices[vertex].position[0] < -0.0025f)
            ++metrics.sideCrossingVertexCount;
    }
    const Vec3 eye{{snapshot.playerX, horde::gameplay::kShowcaseEyeWorldY,
                    snapshot.playerZ}};
    const Vec3 worldUp{{0.0f, 1.0f, 0.0f}};
    const Vec3 viewForward = Normalise({{
        std::sin(snapshot.playerYawRadians),
        -0.05f + std::clamp(snapshot.playerPitchRadians, -0.32f, 0.28f),
        -std::cos(snapshot.playerYawRadians)}});
    const Vec3 viewRight = Normalise(Cross(viewForward, worldUp));
    const Vec3 viewUp = Normalise(Cross(viewRight, viewForward));
    const auto toWorld = [&rig](const auto& vertex) {
        return Vec3{{rig.playerRootWorld[0] + rig.bodyRight[0] * vertex.position[0] +
                         rig.bodyForward[0] * vertex.position[2],
                     rig.playerRootWorld[1] + vertex.position[1],
                     rig.playerRootWorld[2] + rig.bodyRight[2] * vertex.position[0] +
                         rig.bodyForward[2] * vertex.position[2]}};
    };
    for (const auto& vertex : vertices)
        metrics.minimumWorldY = std::min(metrics.minimumWorldY, toWorld(vertex)[1]);
    const auto project = [&](const Vec3& world) {
        const Vec3 delta = Subtract(world, eye);
        const float depth = Dot(delta, viewForward);
        metrics.minimumVertexDepthMetres = std::min(
            metrics.minimumVertexDepthMetres, depth);
        const float safeDepth = std::max(depth, 0.001f);
        return std::array<float, 2u>{{
            std::clamp(1.22f * Dot(delta, viewRight) /
                           (safeDepth * (16.0f / 9.0f)),
                       -1.0f, 1.0f),
            std::clamp(1.22f * Dot(delta, viewUp) /
                           (safeDepth * 0.74f),
                       -1.0f, 1.0f)}};
    };
    for (const auto& primitive : playerStatic.primitives)
    {
        if (primitive.materialIndex >= playerStatic.materials.size())
            continue;
        const std::string& materialName =
            playerStatic.materials[primitive.materialIndex].name;
        if (materialName != "BodyPrimaryVisible" &&
            materialName != "GauntletPrimaryVisible")
            continue;
        for (std::uint32_t offset = 0u; offset + 2u < primitive.indexCount;
             offset += 3u)
        {
            std::array<Vec3, 3u> triangle{};
            std::array<Vec3, 3u> modelTriangle{};
            std::array<Vec3, 3u> modelNormals{};
            std::array<Vec3, 3u> baselineTriangle{};
            std::array<std::array<float, 2u>, 3u> projected{};
            std::array<float, 3u> depths{};
            for (std::size_t corner = 0u; corner < 3u; ++corner)
            {
                const std::uint32_t vertexIndex =
                    playerStatic.indices[primitive.indexOffset + offset + corner] +
                    primitive.vertexOffset;
                modelTriangle[corner] = {{
                    vertices[vertexIndex].position[0],
                    vertices[vertexIndex].position[1],
                    vertices[vertexIndex].position[2]}};
                modelNormals[corner] = Normalise({{
                    vertices[vertexIndex].normal[0],
                    vertices[vertexIndex].normal[1],
                    vertices[vertexIndex].normal[2]}});
                triangle[corner] = toWorld(vertices[vertexIndex]);
                baselineTriangle[corner] = {{
                    baselineVertices[vertexIndex].position[0],
                    baselineVertices[vertexIndex].position[1],
                    baselineVertices[vertexIndex].position[2]}};
                depths[corner] = Dot(Subtract(triangle[corner], eye), viewForward);
                projected[corner] = project(triangle[corner]);
            }
            const Vec3 geometricNormal = Normalise(Cross(
                Subtract(modelTriangle[1], modelTriangle[0]),
                Subtract(modelTriangle[2], modelTriangle[0])));
            for (const Vec3& shadingNormal : modelNormals)
            {
                const float alignment = Dot(geometricNormal, shadingNormal);
                metrics.minimumShadingNormalGeometricDot = std::min(
                    metrics.minimumShadingNormalGeometricDot, alignment);
                metrics.minimumFaceForwardedNormalGeometricDot = std::min(
                    metrics.minimumFaceForwardedNormalGeometricDot,
                    std::abs(alignment));
                if (alignment < 0.0f) ++metrics.reversedShadingNormalCount;
            }
            const float triangleDistance =
                PointTriangleDistance(eye, triangle[0], triangle[1], triangle[2]);
            for (std::size_t edge = 0u; edge < 3u; ++edge)
            {
                const std::size_t next = (edge + 1u) % 3u;
                const Vec3 baselineDelta = Subtract(
                    baselineTriangle[next], baselineTriangle[edge]);
                const Vec3 skinnedDelta = Subtract(
                    triangle[next], triangle[edge]);
                metrics.maximumBaselineTriangleEdgeMetres = std::max(
                    metrics.maximumBaselineTriangleEdgeMetres,
                    std::sqrt(Dot(baselineDelta, baselineDelta)));
                metrics.maximumSkinnedTriangleEdgeMetres = std::max(
                    metrics.maximumSkinnedTriangleEdgeMetres,
                    std::sqrt(Dot(skinnedDelta, skinnedDelta)));
            }
            if (triangleDistance < metrics.minimumTriangleDistanceMetres)
            {
                metrics.minimumTriangleDistanceMetres = triangleDistance;
                metrics.minimumTriangleFirstVertex =
                    playerStatic.indices[primitive.indexOffset + offset] +
                    primitive.vertexOffset;
                const auto& minimumBaseline =
                    baselineVertices[metrics.minimumTriangleFirstVertex];
                const auto& minimumSkinned =
                    vertices[metrics.minimumTriangleFirstVertex];
                metrics.minimumTriangleBaselineVertex = {{
                    minimumBaseline.position[0], minimumBaseline.position[1],
                    minimumBaseline.position[2]}};
                metrics.minimumTriangleSkinnedVertex = {{
                    minimumSkinned.position[0], minimumSkinned.position[1],
                    minimumSkinned.position[2]}};
                metrics.minimumTriangleCentroid = Scale(
                    Add(Add(triangle[0], triangle[1]), triangle[2]), 1.0f / 3.0f);
                metrics.minimumTriangleWorld = triangle;
            }
            const double twiceArea = std::abs(
                static_cast<double>(projected[1][0] - projected[0][0]) *
                    static_cast<double>(projected[2][1] - projected[0][1]) -
                static_cast<double>(projected[1][1] - projected[0][1]) *
                    static_cast<double>(projected[2][0] - projected[0][0]));
            metrics.clippedProjectedTriangleArea += 0.5 * twiceArea;
            metrics.maximumProjectedTriangleArea = std::max(
                metrics.maximumProjectedTriangleArea, 0.5 * twiceArea);
            const auto [minimumDepth, maximumDepth] = std::minmax_element(
                depths.begin(), depths.end());
            if (*minimumDepth < 0.002f && *maximumDepth >= 0.002f)
            {
                ++metrics.nearPlaneCrossingTriangleCount;
                metrics.nearPlaneCrossingProjectedArea += 0.5 * twiceArea;
            }
        }
    }
    return metrics;
}

struct GripSurfaceMetrics
{
    float closestRadialDistanceMetres = INFINITY;
    float closestHandleSurfaceDistanceMetres = INFINITY;
    float contactLongitudinalMinimumMetres = INFINITY;
    float contactLongitudinalMaximumMetres = -INFINITY;
    std::size_t contactVertexCount = 0u;
    std::size_t occupiedAngularBins = 0u;
};

GripSurfaceMetrics MeasureGripSurface(
    const std::vector<horde::scene::TexturedSkinnedRtVertex>& vertices,
    const horde::scene::SkinnedNodeTransform& grip,
    const float handleRadiusMetres)
{
    constexpr std::size_t kAngularBins = 24u;
    std::array<bool, kAngularBins> occupied{};
    const Vec3 origin{{grip[12], grip[13], grip[14]}};
    const Vec3 axisX = Normalise({{grip[0], grip[1], grip[2]}});
    const Vec3 axisY = Normalise({{grip[4], grip[5], grip[6]}});
    const Vec3 axisZ = Normalise({{grip[8], grip[9], grip[10]}});
    GripSurfaceMetrics metrics;
    for (const auto& vertex : vertices)
    {
        const Vec3 point{{vertex.position[0], vertex.position[1],
                          vertex.position[2]}};
        const Vec3 delta = Subtract(point, origin);
        const float longitudinal = Dot(delta, axisY);
        if (std::abs(longitudinal) > 0.075f) continue;
        const float transverseX = Dot(delta, axisX);
        const float transverseZ = Dot(delta, axisZ);
        const float radial = std::hypot(transverseX, transverseZ);
        metrics.closestRadialDistanceMetres = std::min(
            metrics.closestRadialDistanceMetres, radial);
        metrics.closestHandleSurfaceDistanceMetres = std::min(
            metrics.closestHandleSurfaceDistanceMetres,
            std::abs(radial - handleRadiusMetres));
        if (radial < handleRadiusMetres - 0.006f ||
            radial > handleRadiusMetres + 0.018f)
            continue;
        ++metrics.contactVertexCount;
        metrics.contactLongitudinalMinimumMetres = std::min(
            metrics.contactLongitudinalMinimumMetres, longitudinal);
        metrics.contactLongitudinalMaximumMetres = std::max(
            metrics.contactLongitudinalMaximumMetres, longitudinal);
        constexpr float kTwoPi = 6.28318530718f;
        float angle = std::atan2(transverseZ, transverseX);
        if (angle < 0.0f) angle += kTwoPi;
        const std::size_t bin = std::min(
            static_cast<std::size_t>(angle / kTwoPi * kAngularBins),
            kAngularBins - 1u);
        occupied[bin] = true;
    }
    metrics.occupiedAngularBins = static_cast<std::size_t>(
        std::count(occupied.begin(), occupied.end(), true));
    return metrics;
}

} // namespace

int main()
{
    using namespace horde::scene;
    static_assert(sizeof(SkinnedRtVertex) == 32u);
    static_assert(sizeof(TexturedSkinnedRtVertex) == 48u);

    const std::filesystem::path root = FindRepoRoot();
    if (!Require(!root.empty(), "repo assets were not found")) return 1;

    std::string diagnostic;
    SkinnedCharacterModel skeleton;
    const auto skeletonPath = root / "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb";
    if (!Require(skeleton.LoadCombatClips(skeletonPath.string(), diagnostic), diagnostic.c_str())) return 1;
    if (!Require(skeleton.HasTexcoords(), "skeleton TEXCOORD_0 was not imported")) return 1;
    if (!Require(skeleton.ExpandedVertexCount() == 28206u, "skeleton expanded vertex count changed")) return 1;
    if (!Require(skeleton.HasNode("LeftHand") && skeleton.HasNode("RightHand"),
                 "general skinned asset must expose actual left/right hand bones")) return 1;
    SkinnedNodeTransform leftHandSocket{};
    SkinnedNodeTransform rightHandSocket{};
    if (!Require(skeleton.NodeTransform(SkinnedClip::Walking, 0.25f, "LeftHand", leftHandSocket, diagnostic),
                 diagnostic.c_str()) ||
        !Require(skeleton.NodeTransform(SkinnedClip::Walking, 0.25f, "RightHand", rightHandSocket, diagnostic),
                 diagnostic.c_str())) return 1;
    if (!Require(std::isfinite(leftHandSocket[12]) && std::isfinite(rightHandSocket[12]) &&
                 std::abs(leftHandSocket[12] - rightHandSocket[12]) > 0.2f,
                 "actual hand bone socket transforms must be finite and side-stable")) return 1;

    std::vector<SkinnedRtVertex> skeletonPlain;
    std::vector<TexturedSkinnedRtVertex> skeletonTextured;
    if (!Require(skeleton.Skin(SkinnedClip::Idle, 0.25f, skeletonPlain, diagnostic), diagnostic.c_str())) return 1;
    if (!Require(skeleton.SkinTextured(SkinnedClip::Idle, 0.25f, skeletonTextured, diagnostic), diagnostic.c_str())) return 1;
    if (!Require(skeletonPlain.size() == skeletonTextured.size(), "textured skinning changed skeleton vertex count")) return 1;
    if (!Require(FiniteTexturedVertices(skeletonTextured), "skeleton textured vertices are invalid")) return 1;

    SkinnedCharacterModel player;
    const auto playerPath = root / "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb";
    if (!Require(player.LoadClips(playerPath.string(), PlayerLocomotionClipSet(), diagnostic),
                 diagnostic.c_str())) return 1;
    if (!Require(player.HasTexcoords() && player.HasTangents() &&
                     player.ExpandedVertexCount() == 83325u,
                 "player four-primitive textured PBR skin layout changed")) return 1;
    std::cout << "player exact-grounding candidate vertices="
              << player.BootGroundingCandidateVertexCount() << '\n';
    if (!Require(player.BootGroundingCandidateVertexCount() >= 1000u &&
                     player.BootGroundingCandidateVertexCount() <= 8000u,
                 "player exact-grounding subset must remain bounded to the authored lower body"))
        return 1;
    const auto& playerPrimitives = player.PrimitiveRanges();
    if (!Require(playerPrimitives.size() == 4u &&
                 playerPrimitives[0].materialName == "BodyPrimaryVisible" &&
                 playerPrimitives[1].materialName == "GauntletPrimaryVisible" &&
                 playerPrimitives[2].materialName == "HeadPrimaryMasked" &&
                 playerPrimitives[3].materialName == "NearFacePrimaryMasked" &&
                 playerPrimitives[0].expandedVertexCount == 16596u &&
                 playerPrimitives[1].expandedVertexCount == 26514u &&
                 playerPrimitives[2].expandedVertexCount == 5439u &&
                 playerPrimitives[3].expandedVertexCount == 34776u,
                 "player authored complete-arm and reflection-only body triangle ranges changed")) return 1;
    if (!Require(player.HasNode("LeftHand") && player.HasNode("RightHand") &&
                 player.HasNode("LeftGrip") && player.HasNode("RightGrip") &&
                 player.ClipDuration(SkinnedClip::Idle) > 0.9f &&
                 player.ClipDuration(SkinnedClip::Walking) > 0.75f &&
                 player.ClipDuration(SkinnedClip::Attack) == 0.0f &&
                 player.ClipDuration(SkinnedClip::Dead) == 0.0f,
                 "player joint or idle/walk-only clip contract changed")) return 1;
    std::vector<TexturedSkinnedRtVertex> playerIdle;
    std::vector<TexturedSkinnedRtVertex> playerWalking;
    const bool idleSkinned = player.SkinTextured(
        SkinnedClip::Idle, 0.25f, playerIdle, diagnostic);
    if (!Require(idleSkinned, diagnostic.c_str())) return 1;
    const bool walkingSkinned = player.SkinTextured(
        SkinnedClip::Walking, 0.25f, playerWalking, diagnostic);
    if (!Require(walkingSkinned, diagnostic.c_str()) ||
        !Require(FiniteTexturedVertices(playerIdle) && FiniteTexturedVertices(playerWalking) &&
                 playerIdle.size() == playerWalking.size(),
                 "player idle/walk skinning output is invalid")) return 1;
    std::cout << "player basic idle/walk skin passed\n";

    horde::scene::assets::AssetManifest playerManifest;
    horde::scene::assets::StaticMeshAsset playerStatic;
    const bool playerManifestLoaded =
        horde::scene::assets::AssetManifest::Load(
            root / "assets/models/player/runtime/asset.manifest.json",
            playerManifest, diagnostic);
    if (!Require(playerManifestLoaded, diagnostic.c_str())) return 1;
    const bool playerStaticLoaded = horde::scene::assets::StaticMeshAsset::Load(
        playerPath, playerManifest, playerStatic, diagnostic);
    if (!Require(playerStaticLoaded, diagnostic.c_str()) ||
        !Require(player.UniqueVertexCount() == playerStatic.vertices.size(),
                 "static-PBR and skinned player vertex streams must have identical unique ordering")) return 1;
    std::cout << "player static/skinned stream agreement passed\n";
    horde::scene::assets::AssetManifest rewardRingManifest;
    horde::scene::assets::StaticMeshAsset rewardRingStatic;
    const auto rewardRingDirectory =
        root / "assets/models/props/runtime/reward-lantern-ring";
    if (!Require(horde::scene::assets::AssetManifest::Load(
                     rewardRingDirectory / "asset.manifest.json",
                     rewardRingManifest, diagnostic), diagnostic.c_str()) ||
        !Require(horde::scene::assets::StaticMeshAsset::Load(
                     rewardRingDirectory /
                         "reward-lantern-ring-lod0.runtime.glb",
                     rewardRingManifest, rewardRingStatic, diagnostic),
                 diagnostic.c_str()))
        return 1;
    const auto* rewardGripRing = horde::gameplay::items::FindHeldItemSocket(
        rewardRingStatic.sockets, "GripRing");
    const auto* rewardRingHinge = horde::gameplay::items::FindHeldItemSocket(
        rewardRingStatic.sockets, "Hinge");
    if (!Require(rewardGripRing != nullptr && rewardRingHinge != nullptr &&
                     rewardGripRing->world[13] >=
                         rewardRingHinge->world[13] + 0.095f,
                 "the production reward attachment must remain the authored top GripRing above its hanging Hinge"))
        return 1;

    SkinnedNodeTransform leftArmBase{};
    SkinnedNodeTransform rightArmBase{};
    if (!Require(player.NodeTransform(SkinnedClip::Idle, 0.0f, "LeftArm", leftArmBase, diagnostic),
                 diagnostic.c_str()) ||
        !Require(player.NodeTransform(SkinnedClip::Idle, 0.0f, "RightArm", rightArmBase, diagnostic),
                 diagnostic.c_str())) return 1;
    if (!Require(leftArmBase[12] > 0.10f && rightArmBase[12] < -0.10f,
                 "the audited +Z-facing player source keeps anatomical Left on +X and Right on -X"))
        return 1;
    const SkinnedArmIkTarget leftTarget{{{leftArmBase[12] - 0.12f, leftArmBase[13] - 0.20f,
                                           leftArmBase[14] + 0.28f}},
                                         {{-1.0f, -0.2f, 0.2f}}};
    const SkinnedArmIkTarget rightTarget{{{rightArmBase[12] + 0.12f, rightArmBase[13] - 0.20f,
                                            rightArmBase[14] + 0.28f}},
                                          {{1.0f, -0.2f, 0.2f}}};
    std::vector<TexturedSkinnedRtVertex> playerSolved;
    std::vector<SkinnedPbrTangent> playerSolvedTangents;
    SkinnedPlayerSockets playerSockets;
    if (!Require(player.SkinPlayerUniqueTextured(SkinnedClip::Idle, 0.25f,
                                                  leftTarget, rightTarget,
                                                  playerSolved,
                                                  playerSolvedTangents,
                                                  playerSockets,
                                                  diagnostic), diagnostic.c_str()) ||
        !Require(playerSolved.size() == playerStatic.vertices.size() &&
                 playerSolvedTangents.size() == playerSolved.size() &&
                 FiniteTexturedVertices(playerSolved),
                 "player IK skin output must remain finite and static-PBR-addressable")) return 1;
    for (std::size_t vertex = 0u; vertex < playerSolved.size(); ++vertex)
    {
        const auto& tangent = playerSolvedTangents[vertex].tangent;
        const auto& normal = playerSolved[vertex].normal;
        const float tangentLength = std::hypot(
            std::hypot(tangent[0], tangent[1]), tangent[2]);
        const float normalTangentDot = normal[0] * tangent[0] +
            normal[1] * tangent[1] + normal[2] * tangent[2];
        if (!Require(std::isfinite(tangentLength) &&
                         std::abs(tangentLength - 1.0f) <= 0.001f &&
                         std::abs(normalTangentDot) <= 0.001f &&
                         (tangent[3] == -1.0f || tangent[3] == 1.0f),
                     "player IK skin must produce a finite orthonormal PBR tangent frame"))
            return 1;
    }
    const auto socketDistance = [](const SkinnedNodeTransform& socket,
                                   const SkinnedArmIkTarget& target) {
        return std::hypot(std::hypot(socket[12] - target.target[0],
                                    socket[13] - target.target[1]),
                          socket[14] - target.target[2]);
    };
    if (!Require(socketDistance(playerSockets.leftHand, leftTarget) <= 0.015f &&
                 socketDistance(playerSockets.rightHand, rightTarget) <= 0.015f,
                 "actual LeftHand/RightHand bone sockets must solve within 15 mm")) return 1;

    horde::gameplay::simulation::GameSimulation simulation;
    horde::gameplay::simulation::InputSnapshot walkingInput{};
    walkingInput.moveForward = 1.0f;
    simulation.StepFixed(walkingInput);
    horde::vulkan::raytracing::PlayerRenderSlot playerSlot;
    bool poseUpdated = false;
    if (!playerSlot.LoadAsset(playerPath.string(), diagnostic))
    {
        std::cerr << "FAIL: " << diagnostic << '\n';
        return 1;
    }
    const TestRigFrame authoredRig = BuildRigFrame(
        simulation.Snapshot(), leftArmBase, rightArmBase, playerSlot);
    const auto& rigSnapshot = authoredRig.animation;
    if (!Require(rigSnapshot.leftIk.shoulder[0] > 0.10f &&
                 rigSnapshot.rightIk.shoulder[0] < -0.10f &&
                 horde::vulkan::raytracing::PlayerModelWorldBasisDeterminant(
                     authoredRig.modelBasis) > 0.999f,
                 "production view-to-model conversion must preserve anatomical Left/Right through a proper rotation"))
        return 1;
    if (!playerSlot.PreparePose(
            rigSnapshot,
            simulation.Snapshot().tickIndex,
            horde::vulkan::raytracing::PlayerCpuSkinCadence::Hz60,
            poseUpdated, diagnostic))
    {
        std::cerr << "FAIL: " << diagnostic << '\n';
        return 1;
    }
    if (
        !Require(poseUpdated &&
                 playerSlot.LeftSocketErrorMetres() <= 0.015f &&
                 playerSlot.RightSocketErrorMetres() <= 0.015f,
                 "authoritative held-item targets must drive the final rig bone sockets")) return 1;

    const auto* restCheckpoint = horde::gameplay::FindDevelopmentCheckpoint(106);
    const auto* downCheckpoint = horde::gameplay::FindDevelopmentCheckpoint(107);
    const auto* upCheckpoint = horde::gameplay::FindDevelopmentCheckpoint(108);
    const auto* rewardHighCheckpoint = horde::gameplay::FindDevelopmentCheckpoint(116);
    const auto* rewardLowCheckpoint = horde::gameplay::FindDevelopmentCheckpoint(117);
    horde::gameplay::simulation::GameSimulation restSimulation;
    horde::gameplay::simulation::GameSimulation downSimulation;
    horde::gameplay::simulation::GameSimulation upSimulation;
    horde::gameplay::simulation::GameSimulation rewardHighSimulation;
    horde::gameplay::simulation::GameSimulation rewardLowSimulation;
    if (!Require(restCheckpoint != nullptr && downCheckpoint != nullptr &&
                 upCheckpoint != nullptr && rewardHighCheckpoint != nullptr &&
                 rewardLowCheckpoint != nullptr &&
                 horde::gameplay::StageDevelopmentCheckpointSimulation(
                     restSimulation, *restCheckpoint) &&
                 horde::gameplay::StageDevelopmentCheckpointSimulation(
                     downSimulation, *downCheckpoint) &&
                 horde::gameplay::StageDevelopmentCheckpointSimulation(
                     upSimulation, *upCheckpoint) &&
                 horde::gameplay::StageDevelopmentCheckpointSimulation(
                     rewardHighSimulation, *rewardHighCheckpoint) &&
                 horde::gameplay::StageDevelopmentCheckpointSimulation(
                     rewardLowSimulation, *rewardLowCheckpoint),
                 "rest/down/up and reward high/low direct checkpoints must import and freeze deterministically"))
        return 1;

    struct ResolvedPlayerPose
    {
        horde::gameplay::items::HeldItemStates authoritative{};
        horde::gameplay::items::HeldItemStates rendered{};
        float maximumPositionErrorMetres = 0.0f;
        float maximumOrientationErrorRadians = 0.0f;
    };
    const auto resolvePlayerPose = [&](horde::vulkan::raytracing::PlayerRenderSlot& slot,
                                       const horde::gameplay::simulation::SimulationSnapshot& source,
                                       const std::uint64_t tick,
                                       ResolvedPlayerPose& result) {
        const TestRigFrame rig = BuildRigFrame(
            source, leftArmBase, rightArmBase, slot);
        bool updated = false;
        if (!slot.PreparePose(rig.animation, tick,
                              horde::vulkan::raytracing::PlayerCpuSkinCadence::Hz60,
                              updated, diagnostic) || !updated)
            return false;
        result.authoritative = source.heldItems;
        const auto& bones = slot.BoneSockets();
        if (!slot.ResolveHeldItemVisuals(
                result.authoritative,
                RigidWorldBoneTransform(bones.leftGrip, rig),
                RigidWorldBoneTransform(bones.rightGrip, rig),
                result.rendered, diagnostic))
            return false;
        result.maximumPositionErrorMetres = 0.0f;
        result.maximumOrientationErrorRadians = 0.0f;
        for (std::size_t item = 0u; item < result.rendered.size(); ++item)
        {
            const HeldItemTransform intendedGrip = FinalGrip(result.authoritative[item]);
            const HeldItemTransform finalGrip = FinalGrip(result.rendered[item]);
            result.maximumPositionErrorMetres = std::max(
                result.maximumPositionErrorMetres,
                PositionError(intendedGrip, finalGrip));
            result.maximumOrientationErrorRadians = std::max(
                result.maximumOrientationErrorRadians,
                OrientationError(intendedGrip, finalGrip));
        }
        return true;
    };

    horde::vulkan::raytracing::PlayerRenderSlot restFirstSlot;
    horde::vulkan::raytracing::PlayerRenderSlot downFirstSlot;
    horde::vulkan::raytracing::PlayerRenderSlot upFirstSlot;
    horde::vulkan::raytracing::PlayerRenderSlot resetSlot;
    horde::vulkan::raytracing::PlayerRenderSlot frozenSlot;
    horde::vulkan::raytracing::PlayerRenderSlot sameTickSlot;
    if (!restFirstSlot.LoadAsset(playerPath.string(), diagnostic) ||
        !downFirstSlot.LoadAsset(playerPath.string(), diagnostic) ||
        !upFirstSlot.LoadAsset(playerPath.string(), diagnostic) ||
        !resetSlot.LoadAsset(playerPath.string(), diagnostic) ||
        !frozenSlot.LoadAsset(playerPath.string(), diagnostic) ||
        !sameTickSlot.LoadAsset(playerPath.string(), diagnostic))
    {
        std::cerr << "FAIL: stable rest basis load: " << diagnostic << '\n';
        return 1;
    }

    const TestRigFrame highRig = BuildRigFrame(
        rewardHighSimulation.Snapshot(), leftArmBase, rightArmBase, playerSlot);
    const TestRigFrame lowRig = BuildRigFrame(
        rewardLowSimulation.Snapshot(), leftArmBase, rightArmBase, playerSlot);
    const auto rootError = [](const TestRigFrame& rig) {
        const Vec3 delta = Subtract(
            rig.playerRootWorld, rig.anchoredPlayerRootWorld);
        return std::sqrt(Dot(delta, delta));
    };
    if (!Require(rootError(highRig) <= 0.010f && rootError(lowRig) <= 0.010f,
                 "reward arm targets must not translate the skinned torso/root away from the ordinary right-shoulder anchor"))
        return 1;
    bool sameTickUpdated = false;
    horde::gameplay::items::HeldItemStates sameTickRendered{};
    if (!sameTickSlot.PreparePose(
            highRig.animation, 77u,
            horde::vulkan::raytracing::PlayerCpuSkinCadence::Hz60,
            sameTickUpdated, diagnostic) || !sameTickUpdated ||
        !sameTickSlot.ResolveHeldItemVisuals(
            rewardHighSimulation.Snapshot().heldItems,
            RigidWorldBoneTransform(sameTickSlot.BoneSockets().leftGrip, highRig),
            RigidWorldBoneTransform(sameTickSlot.BoneSockets().rightGrip, highRig),
            sameTickRendered, diagnostic))
    {
        std::cerr << "FAIL: initial same-tick high pose: " << diagnostic << '\n';
        return 1;
    }
    const HeldItemTransform highFinalGrip = sameTickSlot.FinalWorldFromLeftGrip();
    sameTickUpdated = false;
    if (!sameTickSlot.PreparePose(
            lowRig.animation, 77u,
            horde::vulkan::raytracing::PlayerCpuSkinCadence::Hz60,
            sameTickUpdated, diagnostic) || !sameTickUpdated ||
        !sameTickSlot.ResolveHeldItemVisuals(
            rewardLowSimulation.Snapshot().heldItems,
            RigidWorldBoneTransform(sameTickSlot.BoneSockets().leftGrip, lowRig),
            RigidWorldBoneTransform(sameTickSlot.BoneSockets().rightGrip, lowRig),
            sameTickRendered, diagnostic))
    {
        std::cerr << "FAIL: changed same-tick low pose: " << diagnostic << '\n';
        return 1;
    }
    const HeldItemTransform lowFinalGrip = sameTickSlot.FinalWorldFromLeftGrip();
    sameTickUpdated = true;
    if (!Require(PositionError(highFinalGrip, lowFinalGrip) >= 0.18f,
                 "same-tick changed animation must refresh skinning and publish the changed final left Grip") ||
        !Require(sameTickSlot.PreparePose(
                     lowRig.animation, 77u,
                     horde::vulkan::raytracing::PlayerCpuSkinCadence::Hz60,
                     sameTickUpdated, diagnostic) && !sameTickUpdated &&
                     PositionError(lowFinalGrip,
                                   sameTickSlot.FinalWorldFromLeftGrip()) <= 0.000001f,
                 "same-tick unchanged animation must reuse the cached skin and final Grip"))
        return 1;

    ResolvedPlayerPose restInitial;
    ResolvedPlayerPose downInitial;
    ResolvedPlayerPose upInitial;
    ResolvedPlayerPose ignoredResetInitial;
    ResolvedPlayerPose frozenDownInitial;
    if (!resolvePlayerPose(restFirstSlot, restSimulation.Snapshot(), 1u, restInitial) ||
        !resolvePlayerPose(downFirstSlot, downSimulation.Snapshot(), 1u, downInitial) ||
        !resolvePlayerPose(upFirstSlot, upSimulation.Snapshot(), 1u, upInitial) ||
        !resolvePlayerPose(resetSlot, downSimulation.Snapshot(), 1u,
                           ignoredResetInitial) ||
        !resolvePlayerPose(frozenSlot, downSimulation.Snapshot(), 1u,
                           frozenDownInitial))
    {
        std::cerr << "FAIL: initial stable-basis pose: " << diagnostic << '\n';
        return 1;
    }

    if (!resetSlot.LoadAsset(playerPath.string(), diagnostic))
    {
        std::cerr << "FAIL: stable-basis reset: " << diagnostic << '\n';
        return 1;
    }
    ResolvedPlayerPose restAfterRestFirst;
    ResolvedPlayerPose restAfterDownFirst;
    ResolvedPlayerPose restAfterUpFirst;
    ResolvedPlayerPose restAfterReset;
    ResolvedPlayerPose frozenDownRepeated;
    if (!resolvePlayerPose(restFirstSlot, restSimulation.Snapshot(), 2u,
                           restAfterRestFirst) ||
        !resolvePlayerPose(downFirstSlot, restSimulation.Snapshot(), 2u,
                           restAfterDownFirst) ||
        !resolvePlayerPose(upFirstSlot, restSimulation.Snapshot(), 2u,
                           restAfterUpFirst) ||
        !resolvePlayerPose(resetSlot, restSimulation.Snapshot(), 2u,
                           restAfterReset) ||
        !resolvePlayerPose(frozenSlot, downSimulation.Snapshot(), 2u,
                           frozenDownRepeated))
    {
        std::cerr << "FAIL: ordered stable-basis rest pose: " << diagnostic << '\n';
        return 1;
    }

    const auto agreesWithAuthoritativeGrip = [](const ResolvedPlayerPose& pose) {
        return pose.maximumPositionErrorMetres <= 0.015f &&
               pose.maximumOrientationErrorRadians <=
                   horde::vulkan::raytracing::kPlayerGripOrientationToleranceRadians;
    };
    if (!Require(agreesWithAuthoritativeGrip(restInitial) &&
                 agreesWithAuthoritativeGrip(downInitial) &&
                 agreesWithAuthoritativeGrip(upInitial) &&
                 agreesWithAuthoritativeGrip(restAfterRestFirst) &&
                 agreesWithAuthoritativeGrip(restAfterReset),
                 "final composed item Grip must agree with the authoritative hand pivot in position and orientation"))
        return 1;
    std::cout << "Stable final Grip agreement rest="
              << restInitial.maximumPositionErrorMetres << "m/"
              << restInitial.maximumOrientationErrorRadians << "rad down="
              << downInitial.maximumPositionErrorMetres << "m/"
              << downInitial.maximumOrientationErrorRadians << "rad up="
              << upInitial.maximumPositionErrorMetres << "m/"
              << upInitial.maximumOrientationErrorRadians << "rad\n";

    const auto sameFinalGrips = [](const ResolvedPlayerPose& left,
                                   const ResolvedPlayerPose& right) {
        for (std::size_t item = 0u; item < left.rendered.size(); ++item)
        {
            const HeldItemTransform leftGrip = FinalGrip(left.rendered[item]);
            const HeldItemTransform rightGrip = FinalGrip(right.rendered[item]);
            if (PositionError(leftGrip, rightGrip) > 0.0001f ||
                OrientationError(leftGrip, rightGrip) > 0.001f)
                return false;
        }
        return true;
    };
    if (!agreesWithAuthoritativeGrip(restAfterDownFirst) ||
        !agreesWithAuthoritativeGrip(restAfterUpFirst) ||
        !sameFinalGrips(restAfterRestFirst, restAfterDownFirst) ||
        !sameFinalGrips(restAfterRestFirst, restAfterUpFirst) ||
        !sameFinalGrips(restAfterRestFirst, restAfterReset) ||
        !sameFinalGrips(frozenDownInitial, frozenDownRepeated))
    {
        std::cerr << "FAIL: player rest Grip basis depends on first rendered checkpoint: "
                  << "rest=" << restAfterRestFirst.maximumOrientationErrorRadians
                  << " down-first=" << restAfterDownFirst.maximumOrientationErrorRadians
                  << " up-first=" << restAfterUpFirst.maximumOrientationErrorRadians
                  << " reset=" << restAfterReset.maximumOrientationErrorRadians << '\n';
        return 1;
    }

    // The compact rig has no finger bones, so each accepted Meshy gauntlet is
    // rigid to its real Hand bone. Measure the final skinned authored surface,
    // not merely the socket transform, to prevent a misplaced or open hand
    // from passing a placement-only regression test.
    constexpr float kAuditedHandleRadiusMetres = 0.018f;
    const auto leftGripSurface = MeasureGripSurface(
        restFirstSlot.UniqueVertices(),
        restFirstSlot.BoneSockets().leftGrip,
        kAuditedHandleRadiusMetres);
    const auto rightGripSurface = MeasureGripSurface(
        restFirstSlot.UniqueVertices(),
        restFirstSlot.BoneSockets().rightGrip,
        kAuditedHandleRadiusMetres);
    const auto gripSpan = [](const GripSurfaceMetrics& metrics) {
        return metrics.contactLongitudinalMaximumMetres -
               metrics.contactLongitudinalMinimumMetres;
    };
    std::cout << "authored Grip surface left axis/surface/contact/bins/span="
              << leftGripSurface.closestRadialDistanceMetres << '/'
              << leftGripSurface.closestHandleSurfaceDistanceMetres << '/'
              << leftGripSurface.contactVertexCount << '/'
              << leftGripSurface.occupiedAngularBins << '/'
              << gripSpan(leftGripSurface)
              << " right="
              << rightGripSurface.closestRadialDistanceMetres << '/'
              << rightGripSurface.closestHandleSurfaceDistanceMetres << '/'
              << rightGripSurface.contactVertexCount << '/'
              << rightGripSurface.occupiedAngularBins << '/'
              << gripSpan(rightGripSurface) << '\n';
    if (!Require(leftGripSurface.closestHandleSurfaceDistanceMetres <= 0.003f &&
                     rightGripSurface.closestHandleSurfaceDistanceMetres <= 0.003f &&
                     leftGripSurface.contactVertexCount >= 80u &&
                     rightGripSurface.contactVertexCount >= 80u &&
                     leftGripSurface.occupiedAngularBins >= 18u &&
                     rightGripSurface.occupiedAngularBins >= 18u &&
                     gripSpan(leftGripSurface) >= 0.075f &&
                     gripSpan(rightGripSurface) >= 0.075f,
                  "both final-skinned authored gauntlets must wrap at least 270 degrees around the audited handle axis with sustained longitudinal contact"))
        return 1;

    const auto* rewardWallHighCheckpoint =
        horde::gameplay::FindDevelopmentCheckpoint(132);
    const auto* rewardWallLowCheckpoint =
        horde::gameplay::FindDevelopmentCheckpoint(133);
    horde::gameplay::simulation::GameSimulation rewardWallHighSimulation;
    horde::gameplay::simulation::GameSimulation rewardWallLowSimulation;
    if (!Require(rewardWallHighCheckpoint != nullptr &&
                     rewardWallLowCheckpoint != nullptr &&
                     horde::gameplay::StageDevelopmentCheckpointSimulation(
                         rewardWallHighSimulation, *rewardWallHighCheckpoint) &&
                     horde::gameplay::StageDevelopmentCheckpointSimulation(
                         rewardWallLowSimulation, *rewardWallLowCheckpoint),
                 "wall high/low direct checkpoints must import for final skinned camera-clearance measurement"))
        return 1;
    const auto measurePlayerWithSlot = [&] (
        horde::vulkan::raytracing::PlayerRenderSlot& slot,
        const auto& snapshot,
        PrimaryPlayerCameraMetrics& metrics) {
        const TestRigFrame rig = BuildRigFrame(
            snapshot, leftArmBase, rightArmBase, slot);
        bool updated = false;
        if (!slot.PreparePose(
                rig.animation, snapshot.tickIndex,
                horde::vulkan::raytracing::PlayerCpuSkinCadence::Hz60,
                updated, diagnostic) || !updated)
            return false;
        std::vector<horde::scene::TexturedSkinnedRtVertex> baseline;
        const horde::scene::SkinnedClip clip =
            rig.animation.locomotionClip ==
                    horde::gameplay::animation::PlayerLocomotionClip::Walk
                ? horde::scene::SkinnedClip::Walking
                : horde::scene::SkinnedClip::Idle;
        if (!player.SkinUniqueTextured(
                clip, rig.animation.locomotionTime, baseline, diagnostic))
            return false;
        metrics = MeasurePrimaryPlayerCameraMetrics(
            playerStatic, baseline, slot.UniqueVertices(), rig, snapshot);
        return true;
    };
    const auto measurePlayer = [&](const auto& snapshot,
                                   PrimaryPlayerCameraMetrics& metrics) {
        horde::vulkan::raytracing::PlayerRenderSlot slot;
        return slot.LoadAsset(playerPath.string(), diagnostic) &&
               measurePlayerWithSlot(slot, snapshot, metrics);
    };
    std::array<PrimaryPlayerCameraMetrics, 2u> openPlayerMetrics{};
    std::array<PrimaryPlayerCameraMetrics, 2u> wallPlayerMetrics{};
    PrimaryPlayerCameraMetrics ordinaryPlayerMetrics{};
    if (!Require(measurePlayer(
                     restSimulation.Snapshot(), ordinaryPlayerMetrics),
                 "ordinary torch/sword pose must resolve for reward player-coverage comparison"))
        return 1;
    std::cout << "ordinary final-skinned clearance triangle/depth/area/side-cross="
              << ordinaryPlayerMetrics.minimumTriangleDistanceMetres << '/'
              << ordinaryPlayerMetrics.minimumVertexDepthMetres << '/'
              << ordinaryPlayerMetrics.clippedProjectedTriangleArea << '/'
              << ordinaryPlayerMetrics.sideCrossingVertexCount << '/'
              << ordinaryPlayerMetrics.sideStableVertexCount
              << " near-cross="
              << ordinaryPlayerMetrics.nearPlaneCrossingTriangleCount << '/'
              << ordinaryPlayerMetrics.nearPlaneCrossingProjectedArea
              << " max-edge bind/skinned="
              << ordinaryPlayerMetrics.maximumBaselineTriangleEdgeMetres << '/'
              << ordinaryPlayerMetrics.maximumSkinnedTriangleEdgeMetres
              << " max-triangle-area="
              << ordinaryPlayerMetrics.maximumProjectedTriangleArea
              << " normal-dot/reversed="
              << ordinaryPlayerMetrics.minimumShadingNormalGeometricDot << '/'
              << ordinaryPlayerMetrics.reversedShadingNormalCount << '/'
              << ordinaryPlayerMetrics.minimumFaceForwardedNormalGeometricDot
              << " sole-y=" << ordinaryPlayerMetrics.minimumWorldY << '\n';
    if (!Require(ordinaryPlayerMetrics.minimumWorldY >=
                     horde::gameplay::kRouteFloorWorldY - 0.0005f &&
                     ordinaryPlayerMetrics.minimumWorldY <=
                         horde::gameplay::kRouteFloorWorldY + 0.001f,
                 "idle skinned player boot sole must contact the shared route floor without sinking or visible hover"))
        return 1;
    // One offline sleeve subdivision bounds bind-pose edges to 55.1 mm. The
    // strongest fixed wall-retracted two-bone pose measures 200.9 mm and the
    // live forward approach measures 210.1 mm while their
    // individual projected triangle remains below 0.003 NDC area, with no
    // near-plane crossing. Keep that stricter screen-space regression and a
    // narrowly measured 215 mm deformation envelope for the fitted garment.
    constexpr float kAuditedCompleteArmEdgeLimitMetres = 0.215f;
    if (!Require(
            ordinaryPlayerMetrics.maximumBaselineTriangleEdgeMetres <= 0.12f &&
            ordinaryPlayerMetrics.maximumSkinnedTriangleEdgeMetres <=
                kAuditedCompleteArmEdgeLimitMetres &&
            ordinaryPlayerMetrics.maximumProjectedTriangleArea <= 0.08,
            "ordinary first-person authored complete-arm triangles must remain locally bounded in model and projected space"))
        return 1;
    if (!Require(
            ordinaryPlayerMetrics.reversedShadingNormalCount <= 1600u &&
                ordinaryPlayerMetrics.minimumFaceForwardedNormalGeometricDot >=
                    0.000001f,
            "final-skinned primary player normals must retain a bounded face-forwardable PBR frame"))
        return 1;
    for (const auto* actionSnapshot : {
             &downSimulation.Snapshot(), &upSimulation.Snapshot()})
    {
        PrimaryPlayerCameraMetrics actionMetrics{};
        if (!Require(measurePlayer(*actionSnapshot, actionMetrics) &&
                         actionMetrics.maximumBaselineTriangleEdgeMetres <= 0.12f &&
                         actionMetrics.maximumSkinnedTriangleEdgeMetres <=
                             kAuditedCompleteArmEdgeLimitMetres &&
                         actionMetrics.maximumProjectedTriangleArea <= 0.08,
                     "downward cut and chained upward slice must retain bounded authored primary triangles"))
            return 1;
    }
    float pitchRootY = INFINITY;
    for (const float pitch : {-0.32f, 0.0f, 0.28f})
    {
        auto pitchedSnapshot = restSimulation.Snapshot();
        pitchedSnapshot.playerPitchRadians = pitch;
        const TestRigFrame pitchedRig = BuildRigFrame(
            pitchedSnapshot, leftArmBase, rightArmBase, playerSlot);
        if (std::isfinite(pitchRootY) &&
            !Require(std::abs(pitchedRig.playerRootWorld[1] - pitchRootY) <=
                         0.001f,
                     "camera pitch must not move the grounded player root or sink the reflected boots"))
            return 1;
        pitchRootY = pitchedRig.playerRootWorld[1];
        PrimaryPlayerCameraMetrics pitchedMetrics{};
        if (!Require(measurePlayer(pitchedSnapshot, pitchedMetrics) &&
                         pitchedMetrics.minimumWorldY >=
                             horde::gameplay::kRouteFloorWorldY - 0.0005f &&
                         pitchedMetrics.minimumWorldY <=
                             horde::gameplay::kRouteFloorWorldY + 0.001f,
                     "idle boot-to-floor contact must remain stable at minimum, neutral, and maximum camera pitch"))
            return 1;
    }
    horde::gameplay::simulation::GameSimulation walkingGroundSimulation;
    horde::gameplay::simulation::InputSnapshot walkingGroundInput;
    walkingGroundInput.damageEnabled = false;
    walkingGroundInput.moveForward = 1.0f;
    horde::vulkan::raytracing::PlayerRenderSlot walkingGroundSlot;
    if (!Require(walkingGroundSlot.LoadAsset(playerPath.string(), diagnostic),
                 "walking ground-contact slot must load"))
        return 1;
    float walkingMinimumSoleY = INFINITY;
    float walkingMaximumSoleY = -INFINITY;
    for (std::size_t tick = 0u; tick < 120u; ++tick)
    {
        walkingGroundSimulation.StepFixed(
            walkingGroundInput,
            static_cast<float>(horde::gameplay::simulation::
                FixedStepRunner::kFixedDeltaSeconds),
            walkingGroundSimulation.Snapshot().inputPublicationSequence + 1u);
        if (tick % 10u != 0u) continue;
        PrimaryPlayerCameraMetrics walkingMetrics{};
        if (!Require(measurePlayerWithSlot(
                         walkingGroundSlot, walkingGroundSimulation.Snapshot(),
                         walkingMetrics),
                     "walking ground-contact phase must resolve the final skinned player"))
            return 1;
        walkingMinimumSoleY = std::min(
            walkingMinimumSoleY, walkingMetrics.minimumWorldY);
        walkingMaximumSoleY = std::max(
            walkingMaximumSoleY, walkingMetrics.minimumWorldY);
    }
    std::cout << "grounded boot envelope idle/walk="
              << ordinaryPlayerMetrics.minimumWorldY << '/'
              << walkingMinimumSoleY << ".." << walkingMaximumSoleY << '\n';
    if (!Require(walkingMinimumSoleY >=
                     horde::gameplay::kRouteFloorWorldY - 0.0005f &&
                     walkingMaximumSoleY <=
                         horde::gameplay::kRouteFloorWorldY + 0.001f,
                 "every sampled walking phase must keep a boot sole within the 1 mm route-floor contact envelope without penetration"))
        return 1;
    horde::gameplay::simulation::GameSimulation ordinaryWallSimulation;
    horde::gameplay::simulation::InputSnapshot ordinaryWallInput;
    ordinaryWallInput.damageEnabled = false;
    ordinaryWallInput.hasAuthoritativePlayerPose = true;
    ordinaryWallInput.authoritativePlayerX = 0.0f;
    ordinaryWallInput.authoritativePlayerZ = -9.70f;
    ordinaryWallInput.yawRadians = 0.0f;
    ordinaryWallInput.pitchRadians = -0.08f;
    ordinaryWallSimulation.StepFixed(ordinaryWallInput, 0.0f, 1u);
    PrimaryPlayerCameraMetrics ordinaryWallMetrics{};
    if (!Require(measurePlayer(
                     ordinaryWallSimulation.Snapshot(), ordinaryWallMetrics),
                 "ordinary torch/sword real-wall pose must skin"))
        return 1;
    std::cout << "ordinary-wall final-skinned clearance/depth/area/near-cross="
              << ordinaryWallMetrics.minimumTriangleDistanceMetres << '/'
              << ordinaryWallMetrics.minimumVertexDepthMetres << '/'
              << ordinaryWallMetrics.clippedProjectedTriangleArea << '/'
              << ordinaryWallMetrics.nearPlaneCrossingTriangleCount << '/'
              << ordinaryWallMetrics.nearPlaneCrossingProjectedArea << '\n';
    const std::array<const horde::gameplay::simulation::SimulationSnapshot*, 2u>
        openSnapshots{{&rewardHighSimulation.Snapshot(),
                       &rewardLowSimulation.Snapshot()}};
    const std::array<const horde::gameplay::simulation::SimulationSnapshot*, 2u>
        wallSnapshots{{&rewardWallHighSimulation.Snapshot(),
                       &rewardWallLowSimulation.Snapshot()}};
    bool allWallPlayerMetricsSafe = true;
    for (std::size_t pose = 0u; pose < 2u; ++pose)
    {
        if (!measurePlayer(*openSnapshots[pose], openPlayerMetrics[pose]) ||
            !measurePlayer(*wallSnapshots[pose], wallPlayerMetrics[pose]))
        {
            std::cerr << "FAIL: final skinned wall metric: " << diagnostic << '\n';
            return 1;
        }
        const double projectedAreaRatio =
            wallPlayerMetrics[pose].clippedProjectedTriangleArea /
            std::max(openPlayerMetrics[pose].clippedProjectedTriangleArea,
                     0.000001);
        std::cout << (pose == 0u ? "high" : "low")
                  << " wall final-skinned clearance triangle/depth="
                  << wallPlayerMetrics[pose].minimumTriangleDistanceMetres << '/'
                  << wallPlayerMetrics[pose].minimumVertexDepthMetres
                  << " projected-area wall/open=" << projectedAreaRatio
                  << " (" << wallPlayerMetrics[pose].clippedProjectedTriangleArea
                  << '/' << openPlayerMetrics[pose].clippedProjectedTriangleArea
                  << ") side-cross="
                  << wallPlayerMetrics[pose].sideCrossingVertexCount << '/'
                  << wallPlayerMetrics[pose].sideStableVertexCount
                  << " open/ordinary-area="
                  << openPlayerMetrics[pose].clippedProjectedTriangleArea /
                         std::max(ordinaryPlayerMetrics.clippedProjectedTriangleArea,
                                  0.000001)
                  << " near-cross="
                  << wallPlayerMetrics[pose].nearPlaneCrossingTriangleCount
                  << '/'
                  << wallPlayerMetrics[pose].nearPlaneCrossingProjectedArea
                  << " max-edge open/wall="
                  << openPlayerMetrics[pose].maximumSkinnedTriangleEdgeMetres
                  << '/'
                  << wallPlayerMetrics[pose].maximumSkinnedTriangleEdgeMetres
                  << " max-triangle-area open/wall="
                  << openPlayerMetrics[pose].maximumProjectedTriangleArea
                  << '/'
                  << wallPlayerMetrics[pose].maximumProjectedTriangleArea
                  << '\n';
        const double sideCrossingFraction =
            static_cast<double>(wallPlayerMetrics[pose].sideCrossingVertexCount) /
            std::max<std::size_t>(wallPlayerMetrics[pose].sideStableVertexCount, 1u);
        const double ordinarySideCrossingFraction =
            static_cast<double>(ordinaryPlayerMetrics.sideCrossingVertexCount) /
            std::max<std::size_t>(ordinaryPlayerMetrics.sideStableVertexCount, 1u);
        const bool stableHandedness = Require(
            sideCrossingFraction <= ordinarySideCrossingFraction + 0.005,
            "reward IK must not add cross-torso vertices beyond the ordinary anatomically paired arm baseline");
        const bool safeClearance = Require(
                wallPlayerMetrics[pose].minimumTriangleDistanceMetres >= 0.050f,
                "near-wall final-skinned primary player/arm triangles must stay outside the camera-centred 50 mm exclusion volume");
        const bool safeCoverage = Require(
            projectedAreaRatio <= 1.75 &&
                openPlayerMetrics[pose].clippedProjectedTriangleArea <=
                    ordinaryPlayerMetrics.clippedProjectedTriangleArea * 1.75,
            "reward raised/open and near-wall final-skinned player coverage must stay within the bounded raised-arm envelope instead of dominating the viewport");
        const bool boundedGeometry = Require(
            openPlayerMetrics[pose].maximumBaselineTriangleEdgeMetres <= 0.12f &&
                openPlayerMetrics[pose].maximumSkinnedTriangleEdgeMetres <=
                    kAuditedCompleteArmEdgeLimitMetres &&
                wallPlayerMetrics[pose].maximumSkinnedTriangleEdgeMetres <=
                    kAuditedCompleteArmEdgeLimitMetres &&
                openPlayerMetrics[pose].maximumProjectedTriangleArea <= 0.08 &&
                wallPlayerMetrics[pose].maximumProjectedTriangleArea <= 0.08,
            "reward high/low and wall retraction must retain bounded individual authored primary triangles");
        allWallPlayerMetricsSafe = allWallPlayerMetricsSafe &&
            stableHandedness && safeClearance && safeCoverage && boundedGeometry;
    }

    // Keep direct coverage for the former failure pose even though live
    // movement can no longer advance this close. Imports and camera turns can
    // still author the emergency z=-9.70 presentation, so its final skinned
    // triangles must remain finite, side-stable, and camera-safe.
    horde::gameplay::simulation::GameSimulation extremeWallHighSimulation;
    horde::gameplay::simulation::GameSimulation extremeWallLowSimulation;
    if (!Require(horde::gameplay::StageDevelopmentCheckpointSimulation(
                     extremeWallHighSimulation, *rewardHighCheckpoint) &&
                 horde::gameplay::StageDevelopmentCheckpointSimulation(
                     extremeWallLowSimulation, *rewardLowCheckpoint),
                 "former z=-9.70 high/low emergency imports must stage"))
        return 1;
    horde::gameplay::simulation::InputSnapshot extremeInput;
    extremeInput.damageEnabled = false;
    extremeInput.hasAuthoritativePlayerPose = true;
    extremeInput.authoritativePlayerX = 0.0f;
    extremeInput.authoritativePlayerZ = -9.70f;
    extremeInput.yawRadians = 0.0f;
    extremeInput.pitchRadians = -0.08f;
    extremeWallHighSimulation.StepFixed(
        extremeInput, 0.0f,
        extremeWallHighSimulation.Snapshot().inputPublicationSequence + 1u);
    extremeWallLowSimulation.StepFixed(
        extremeInput, 0.0f,
        extremeWallLowSimulation.Snapshot().inputPublicationSequence + 1u);
    const std::array<const horde::gameplay::simulation::SimulationSnapshot*, 2u>
        extremeSnapshots{{&extremeWallHighSimulation.Snapshot(),
                          &extremeWallLowSimulation.Snapshot()}};
    for (std::size_t pose = 0u; pose < extremeSnapshots.size(); ++pose)
    {
        const auto* extremeSnapshot = extremeSnapshots[pose];
        PrimaryPlayerCameraMetrics extremeMetrics{};
        if (!Require(measurePlayer(*extremeSnapshot, extremeMetrics),
                     "former z=-9.70 emergency pose must resolve the final skinned player"))
            return 1;
        const double sideCrossingFraction =
            static_cast<double>(extremeMetrics.sideCrossingVertexCount) /
            std::max<std::size_t>(extremeMetrics.sideStableVertexCount, 1u);
        std::cout << "former z=-9.70 final-skinned clearance="
                  << extremeMetrics.minimumTriangleDistanceMetres
                  << " depth=" << extremeMetrics.minimumVertexDepthMetres
                  << " projected-area="
                  << extremeMetrics.clippedProjectedTriangleArea
                  << " near-cross="
                  << extremeMetrics.nearPlaneCrossingTriangleCount << '/'
                  << extremeMetrics.nearPlaneCrossingProjectedArea
                  << " side-cross=" << extremeMetrics.sideCrossingVertexCount
                  << '/' << extremeMetrics.sideStableVertexCount
                  << " max-edge/triangle-area="
                  << extremeMetrics.maximumSkinnedTriangleEdgeMetres << '/'
                  << extremeMetrics.maximumProjectedTriangleArea << '\n';
        const double projectedAreaRatio =
            extremeMetrics.clippedProjectedTriangleArea /
            std::max(openPlayerMetrics[pose].clippedProjectedTriangleArea,
                     0.000001);
        if (!Require(std::isfinite(extremeMetrics.minimumTriangleDistanceMetres) &&
                         extremeMetrics.minimumTriangleDistanceMetres >= 0.050f &&
                         // The primary ray path starts at 2 mm. Retain over
                         // twenty times that depth while the stronger 50 mm
                         // requirement is measured against actual triangles,
                         // not a peripheral vertex's forward projection.
                         extremeMetrics.minimumVertexDepthMetres >= 0.040f &&
                         projectedAreaRatio <= 1.75 &&
                         extremeMetrics.maximumSkinnedTriangleEdgeMetres <=
                             kAuditedCompleteArmEdgeLimitMetres &&
                         extremeMetrics.maximumProjectedTriangleArea <= 0.08 &&
                         sideCrossingFraction <=
                             static_cast<double>(
                                 ordinaryPlayerMetrics.sideCrossingVertexCount) /
                                 std::max<std::size_t>(
                                     ordinaryPlayerMetrics.sideStableVertexCount,
                                     1u) + 0.005,
                     "former black-polygon z=-9.70 import path must keep the final skinned arm finite, camera-safe, and on its logical side"))
            return 1;
    }
    const auto claimedVisibility =
        horde::vulkan::raytracing::BuildProductionSceneVisibility({
            horde::vulkan::raytracing::PlayerRenderRoute::HybridBlockPrimary,
            false, false, true});
    if (!Require(claimedVisibility.playerRoute ==
                         horde::vulkan::raytracing::PlayerRenderRoute::HybridBlockPrimary &&
                     claimedVisibility.playerMask == 0x10u &&
                     claimedVisibility.playerPrimaryVisible &&
                     claimedVisibility.playerReflectionVisible,
                 "every emergency claimed-lantern pose must retain block-primary arms and the reflected skinned body"))
        return 1;

    // Endpoint checks previously missed the worst arm-chain contraction near
    // z=-8.53. Sweep the complete real approach at 5 cm spacing and add that
    // exact historical failure point for both high and low carries.
    std::vector<float> wallSweepZ;
    for (float z = -7.50f; z >= -9.7001f; z -= 0.05f)
        wallSweepZ.push_back(z);
    wallSweepZ.push_back(-8.53f);
    std::sort(wallSweepZ.begin(), wallSweepZ.end(), std::greater<float>());
    std::array<horde::gameplay::simulation::GameSimulation, 2u>
        wallSweepSimulations{{
            horde::gameplay::simulation::GameSimulation{},
            horde::gameplay::simulation::GameSimulation{}}};
    std::array<horde::vulkan::raytracing::PlayerRenderSlot, 2u>
        wallSweepSlots{};
    if (!Require(horde::gameplay::StageDevelopmentCheckpointSimulation(
                     wallSweepSimulations[0], *rewardHighCheckpoint) &&
                 horde::gameplay::StageDevelopmentCheckpointSimulation(
                     wallSweepSimulations[1], *rewardLowCheckpoint) &&
                 wallSweepSlots[0].LoadAsset(playerPath.string(), diagnostic) &&
                 wallSweepSlots[1].LoadAsset(playerPath.string(), diagnostic),
                 "high/low final-skinned wall sweep must stage reusable production slots"))
        return 1;
    for (std::size_t pose = 0u; pose < wallSweepSimulations.size(); ++pose)
    {
        float worstTriangleClearance = INFINITY;
        float worstVertexDepth = INFINITY;
        double worstProjectedAreaRatio = 0.0;
        double maximumConsecutiveProjectedAreaDelta = 0.0;
        float maximumConsecutiveAreaDeltaFromZ = 0.0f;
        float maximumConsecutiveAreaDeltaToZ = 0.0f;
        double maximumConsecutiveAreaDeltaFromRatio = 0.0;
        double maximumConsecutiveAreaDeltaToRatio = 0.0;
        double previousProjectedAreaRatio = 0.0;
        double previousProjectedArea = 0.0;
        float previousProjectedAreaZ = 0.0f;
        bool hasPreviousProjectedAreaRatio = false;
        double worstSideCrossingFraction = 0.0;
        float worstZ = 0.0f;
        for (const float z : wallSweepZ)
        {
            horde::gameplay::simulation::InputSnapshot sweepInput;
            sweepInput.damageEnabled = false;
            sweepInput.hasAuthoritativePlayerPose = true;
            sweepInput.authoritativePlayerX = 0.0f;
            sweepInput.authoritativePlayerZ = z;
            sweepInput.yawRadians = 0.0f;
            sweepInput.pitchRadians = -0.08f;
            wallSweepSimulations[pose].StepFixed(
                sweepInput, 0.0f,
                wallSweepSimulations[pose].Snapshot().inputPublicationSequence + 1u);
            PrimaryPlayerCameraMetrics metrics{};
            if (!measurePlayerWithSlot(
                    wallSweepSlots[pose],
                    wallSweepSimulations[pose].Snapshot(), metrics))
            {
                std::cerr << "FAIL: wall sweep pose="
                          << (pose == 0u ? "high" : "low")
                          << " z=" << z << ": " << diagnostic << '\n';
                return 1;
            }
            const double areaRatio =
                metrics.clippedProjectedTriangleArea /
                std::max(openPlayerMetrics[pose].clippedProjectedTriangleArea,
                         0.000001);
            if (hasPreviousProjectedAreaRatio)
            {
                const double delta =
                    std::abs(metrics.clippedProjectedTriangleArea -
                             previousProjectedArea);
                if (delta > maximumConsecutiveProjectedAreaDelta)
                {
                    maximumConsecutiveProjectedAreaDelta = delta;
                    maximumConsecutiveAreaDeltaFromZ = previousProjectedAreaZ;
                    maximumConsecutiveAreaDeltaToZ = z;
                    maximumConsecutiveAreaDeltaFromRatio =
                        previousProjectedAreaRatio;
                    maximumConsecutiveAreaDeltaToRatio = areaRatio;
                }
            }
            previousProjectedAreaRatio = areaRatio;
            previousProjectedArea = metrics.clippedProjectedTriangleArea;
            previousProjectedAreaZ = z;
            hasPreviousProjectedAreaRatio = true;
            const double sideCrossingFraction =
                static_cast<double>(metrics.sideCrossingVertexCount) /
                std::max<std::size_t>(metrics.sideStableVertexCount, 1u);
            if (metrics.minimumTriangleDistanceMetres < worstTriangleClearance)
            {
                worstTriangleClearance = metrics.minimumTriangleDistanceMetres;
                worstZ = z;
            }
            worstVertexDepth = std::min(
                worstVertexDepth, metrics.minimumVertexDepthMetres);
            worstProjectedAreaRatio = std::max(
                worstProjectedAreaRatio, areaRatio);
            worstSideCrossingFraction = std::max(
                worstSideCrossingFraction, sideCrossingFraction);
            const double allowedSideCrossingFraction =
                static_cast<double>(
                    ordinaryPlayerMetrics.sideCrossingVertexCount) /
                std::max<std::size_t>(
                    ordinaryPlayerMetrics.sideStableVertexCount, 1u) + 0.005;
            const bool intervalPoseSafe =
                std::isfinite(metrics.minimumTriangleDistanceMetres) &&
                metrics.minimumTriangleDistanceMetres >= 0.050f &&
                // Complete same-side arms are primary-visible now. Their
                // open-pose denominator still makes a ratio-only bound
                // unstable near retraction, so use the audited 1.70 clipped-
                // NDC whole-arm ceiling plus the stricter consecutive-sample
                // continuity guard below.
                metrics.clippedProjectedTriangleArea <= 1.70 &&
                sideCrossingFraction <= allowedSideCrossingFraction;
            if (!intervalPoseSafe)
            {
                std::cout << " wall-sweep failure pose="
                          << (pose == 0u ? "high" : "low")
                          << " z=" << z << " clearance="
                          << metrics.minimumTriangleDistanceMetres
                          << " triangle-first-vertex="
                          << metrics.minimumTriangleFirstVertex
                          << " centroid=" << metrics.minimumTriangleCentroid[0]
                          << ',' << metrics.minimumTriangleCentroid[1]
                          << ',' << metrics.minimumTriangleCentroid[2]
                          << " baseline="
                          << metrics.minimumTriangleBaselineVertex[0] << ','
                          << metrics.minimumTriangleBaselineVertex[1] << ','
                          << metrics.minimumTriangleBaselineVertex[2]
                          << " skinned="
                          << metrics.minimumTriangleSkinnedVertex[0] << ','
                          << metrics.minimumTriangleSkinnedVertex[1] << ','
                          << metrics.minimumTriangleSkinnedVertex[2]
                          << " depth=" << metrics.minimumVertexDepthMetres
                          << " clipped-area="
                          << metrics.clippedProjectedTriangleArea
                          << " area-ratio=" << areaRatio
                          << " side-cross=" << sideCrossingFraction << '\n';
            }
            if (!Require(intervalPoseSafe,
                         "complete wall interval must keep final-skinned player finite, camera-safe, bounded, and side-stable"))
                return 1;
        }
        std::cout << (pose == 0u ? "high" : "low")
                  << " full wall sweep samples=" << wallSweepZ.size()
                  << " worst clearance@z=" << worstTriangleClearance << '@'
                  << worstZ << " depth=" << worstVertexDepth
                  << " area-ratio=" << worstProjectedAreaRatio
                  << " consecutive-clipped-area-delta="
                  << maximumConsecutiveProjectedAreaDelta
                  << '@' << maximumConsecutiveAreaDeltaFromZ << "->"
                  << maximumConsecutiveAreaDeltaToZ << '('
                  << maximumConsecutiveAreaDeltaFromRatio << "->"
                  << maximumConsecutiveAreaDeltaToRatio << ')'
                  << " side-cross=" << worstSideCrossingFraction << '\n';
        if (!Require(maximumConsecutiveProjectedAreaDelta <= 0.19,
                     "five-centimetre wall approach must keep the clipped skinned-player silhouette continuous without a one-step visibility pop"))
            return 1;
    }

    // Reproduce the live owner path rather than validating only frozen wall
    // endpoints: retain the claimed lantern while the authoritative player
    // pose advances into the real z=-10 collision fixture. Every fixed tick
    // must produce finite skin/pendulum data and keep the final skinned player
    // outside the camera-centred near exclusion volume.
    horde::gameplay::simulation::GameSimulation wallApproachSimulation;
    if (!horde::gameplay::StageDevelopmentCheckpointSimulation(
            wallApproachSimulation, *rewardHighCheckpoint))
    {
        std::cerr << "FAIL: held-lantern wall approach staging failed\n";
        return 1;
    }
    horde::vulkan::raytracing::PlayerRenderSlot wallApproachSlot;
    if (!wallApproachSlot.LoadAsset(playerPath.string(), diagnostic))
    {
        std::cerr << "FAIL: held-lantern wall approach slot: " << diagnostic << '\n';
        return 1;
    }
    float minimumApproachClearance = INFINITY;
    PrimaryPlayerCameraMetrics worstApproachMetrics{};
    std::size_t worstApproachStep = 0u;
    Vec3 worstApproachCamera{};
    float maximumApproachTriangleEdge = 0.0f;
    double maximumApproachTriangleArea = 0.0;
    bool approachFinite = true;
    std::size_t approachFailureStep = 0u;
    std::string approachFailurePhase;
    horde::gameplay::simulation::InputSnapshot input;
    input.damageEnabled = false;
    input.hasAuthoritativePlayerPose = true;
    input.authoritativePlayerX = 0.0f;
    input.authoritativePlayerZ = -8.50f;
    input.yawRadians = 0.0f;
    input.pitchRadians = -0.08f;
    wallApproachSimulation.StepFixed(
        input, 0.0f,
        wallApproachSimulation.Snapshot().inputPublicationSequence + 1u);
    input.hasAuthoritativePlayerPose = false;
    input.moveForward = 1.0f;
    float previousApproachZ = wallApproachSimulation.Snapshot().playerZ;
    std::size_t collisionBlockedTicks = 0u;
    for (std::size_t step = 0u; step < 180u; ++step)
    {
        wallApproachSimulation.StepFixed(
            input,
            static_cast<float>(horde::gameplay::simulation::
                FixedStepRunner::kFixedDeltaSeconds),
            wallApproachSimulation.Snapshot().inputPublicationSequence + 1u);
        const auto& snapshot = wallApproachSimulation.Snapshot();
        if (std::abs(snapshot.playerZ - previousApproachZ) <= 0.000001f)
            ++collisionBlockedTicks;
        previousApproachZ = snapshot.playerZ;
        const TestRigFrame rig = BuildRigFrame(
            snapshot, leftArmBase, rightArmBase, wallApproachSlot);
        bool updated = false;
        if (!wallApproachSlot.PreparePose(
                rig.animation, snapshot.tickIndex,
                horde::vulkan::raytracing::PlayerCpuSkinCadence::Hz60,
                updated, diagnostic) || !updated ||
            !FiniteTexturedVertices(wallApproachSlot.UniqueVertices()))
        {
            approachFinite = false;
            approachFailureStep = step;
            approachFailurePhase = "skin: " + diagnostic;
            break;
        }
        horde::gameplay::items::HeldItemStates renderedItems{};
        const auto& boneSockets = wallApproachSlot.BoneSockets();
        if (!wallApproachSlot.ResolveHeldItemVisuals(
                snapshot.heldItems,
                RigidWorldBoneTransform(boneSockets.leftGrip, rig),
                RigidWorldBoneTransform(boneSockets.rightGrip, rig),
                renderedItems, diagnostic))
        {
            approachFinite = false;
            approachFailureStep = step;
            approachFailurePhase = "held-item resolve: " + diagnostic;
            break;
        }
        horde::vulkan::raytracing::RewardLanternVisualTransforms
            rewardVisuals{};
        if (!horde::vulkan::raytracing::ComposeClaimedRewardLanternVisuals(
                wallApproachSlot.FinalWorldFromLeftGrip(),
                rewardGripRing->world, rewardRingHinge->world,
                snapshot.rewardLanternWorldFromHinge,
                snapshot.lanternPendulum.worldFromBody,
                horde::gameplay::items::kClaimedRewardLanternScale,
                rewardVisuals, diagnostic) ||
            rewardVisuals.gripAgreement.positionErrorMetres > 0.0001f ||
            rewardVisuals.gripAgreement.orientationErrorRadians > 0.001f)
        {
            approachFinite = false;
            approachFailureStep = step;
            approachFailurePhase = "reward compose: " + diagnostic;
            break;
        }
        std::vector<horde::scene::TexturedSkinnedRtVertex> baseline;
        if (!player.SkinUniqueTextured(
                rig.animation.locomotionClip ==
                        horde::gameplay::animation::PlayerLocomotionClip::Walk
                    ? horde::scene::SkinnedClip::Walking
                    : horde::scene::SkinnedClip::Idle,
                rig.animation.locomotionTime, baseline, diagnostic))
        {
            approachFinite = false;
            approachFailureStep = step;
            approachFailurePhase = "baseline skin: " + diagnostic;
            break;
        }
        const PrimaryPlayerCameraMetrics metrics =
            MeasurePrimaryPlayerCameraMetrics(
                playerStatic, baseline, wallApproachSlot.UniqueVertices(), rig,
                snapshot);
        if (metrics.minimumTriangleDistanceMetres < minimumApproachClearance)
        {
            minimumApproachClearance = metrics.minimumTriangleDistanceMetres;
            worstApproachMetrics = metrics;
            worstApproachStep = step;
            worstApproachCamera = {{snapshot.playerX,
                horde::gameplay::kShowcaseEyeWorldY, snapshot.playerZ}};
        }
        maximumApproachTriangleEdge = std::max(
            maximumApproachTriangleEdge,
            metrics.maximumSkinnedTriangleEdgeMetres);
        maximumApproachTriangleArea = std::max(
            maximumApproachTriangleArea,
            metrics.maximumProjectedTriangleArea);
        for (const float value : snapshot.lanternPendulum.worldFromBody)
            approachFinite = approachFinite && std::isfinite(value);
        for (const float value : rewardVisuals.worldFromBody)
            approachFinite = approachFinite && std::isfinite(value);
    }
    std::cout << "held-lantern wall approach final-skinned clearance="
              << minimumApproachClearance << " finite=" << approachFinite
              << " final-z=" << wallApproachSimulation.Snapshot().playerZ
              << " blocked-ticks=" << collisionBlockedTicks
              << " max-edge/triangle-area=" << maximumApproachTriangleEdge
              << '/' << maximumApproachTriangleArea
              << " worst-step/first/centroid/bind/skinned="
              << worstApproachStep << '/'
              << worstApproachMetrics.minimumTriangleFirstVertex << '/'
              << worstApproachMetrics.minimumTriangleCentroid[0] << ','
              << worstApproachMetrics.minimumTriangleCentroid[1] << ','
              << worstApproachMetrics.minimumTriangleCentroid[2] << '/'
              << worstApproachMetrics.minimumTriangleBaselineVertex[0] << ','
              << worstApproachMetrics.minimumTriangleBaselineVertex[1] << ','
              << worstApproachMetrics.minimumTriangleBaselineVertex[2] << '/'
              << worstApproachMetrics.minimumTriangleSkinnedVertex[0] << ','
              << worstApproachMetrics.minimumTriangleSkinnedVertex[1] << ','
              << worstApproachMetrics.minimumTriangleSkinnedVertex[2]
              << " camera=" << worstApproachCamera[0] << ','
              << worstApproachCamera[1] << ',' << worstApproachCamera[2]
              << " triangle=";
    for (const auto& corner : worstApproachMetrics.minimumTriangleWorld)
        std::cout << corner[0] << ',' << corner[1] << ',' << corner[2] << ';';
    if (!approachFinite)
        std::cout << " failure-step=" << approachFailureStep
                  << " phase=" << approachFailurePhase;
    std::cout << '\n';
    const bool approachSurvives = Require(
        approachFinite && minimumApproachClearance >= 0.050f &&
            maximumApproachTriangleEdge <=
                kAuditedCompleteArmEdgeLimitMetres &&
            maximumApproachTriangleArea <= 0.08 &&
            wallApproachSimulation.Snapshot().playerZ <= -9.70f &&
            wallApproachSimulation.Snapshot().playerZ >= -9.76f &&
            collisionBlockedTicks >= 60u &&
            wallApproachSimulation.Snapshot().interaction.heldLightKind ==
                horde::gameplay::interactions::HeldLightKind::RewardLantern,
        "walking a claimed lantern into the real wall fixture must keep finite renderer inputs and final skinned triangles outside the 50 mm camera exclusion");
    allWallPlayerMetricsSafe = allWallPlayerMetricsSafe && approachSurvives;

    const auto runGuardMovement = [&](const float startX,
                                      const float startZ,
                                      const float forward,
                                      const float strafe) {
        horde::gameplay::simulation::GameSimulation simulation;
        horde::gameplay::StageDevelopmentCheckpointSimulation(
            simulation, *rewardHighCheckpoint);
        horde::gameplay::simulation::InputSnapshot movement;
        movement.damageEnabled = false;
        movement.hasAuthoritativePlayerPose = true;
        movement.authoritativePlayerX = startX;
        movement.authoritativePlayerZ = startZ;
        movement.yawRadians = 0.0f;
        movement.pitchRadians = -0.08f;
        simulation.StepFixed(
            movement, 0.0f,
            simulation.Snapshot().inputPublicationSequence + 1u);
        movement.hasAuthoritativePlayerPose = false;
        movement.moveForward = forward;
        movement.moveStrafe = strafe;
        simulation.StepFixed(
            movement,
            static_cast<float>(horde::gameplay::simulation::
                FixedStepRunner::kFixedDeltaSeconds),
            simulation.Snapshot().inputPublicationSequence + 1u);
        return simulation.Snapshot();
    };
    const auto pureStrafe = runGuardMovement(0.0f, -9.70f, 0.0f, 1.0f);
    const auto diagonal = runGuardMovement(0.0f, -9.70f, 1.0f, 1.0f);
    const auto retreat = runGuardMovement(0.0f, -9.70f, -1.0f, 0.0f);
    const auto cornerOutward = runGuardMovement(0.70f, -9.70f, 0.0f, 1.0f);
    const auto cornerInward = runGuardMovement(0.70f, -9.70f, 0.0f, -1.0f);
    const auto rotateThenForward = [&]() {
        horde::gameplay::simulation::GameSimulation simulation;
        horde::gameplay::StageDevelopmentCheckpointSimulation(
            simulation, *rewardHighCheckpoint);
        horde::gameplay::simulation::InputSnapshot movement;
        movement.damageEnabled = false;
        movement.hasAuthoritativePlayerPose = true;
        movement.authoritativePlayerX = 0.0f;
        movement.authoritativePlayerZ = -9.70f;
        movement.yawRadians = 1.57079632679f;
        movement.pitchRadians = -0.08f;
        simulation.StepFixed(
            movement, 0.0f,
            simulation.Snapshot().inputPublicationSequence + 1u);
        movement.hasAuthoritativePlayerPose = false;
        movement.moveForward = 1.0f;
        simulation.StepFixed(
            movement,
            static_cast<float>(horde::gameplay::simulation::
                FixedStepRunner::kFixedDeltaSeconds),
            simulation.Snapshot().inputPublicationSequence + 1u);
        return simulation.Snapshot();
    }();
    const auto finiteSnapshot = [](const auto& snapshot) {
        for (const float value : snapshot.rewardLanternWorldFromHinge)
            if (!std::isfinite(value)) return false;
        for (const float value : snapshot.lanternPendulum.worldFromBody)
            if (!std::isfinite(value)) return false;
        return std::isfinite(snapshot.playerX) && std::isfinite(snapshot.playerZ);
    };
    std::cout << "reward wall-safe movement strafe=" << pureStrafe.playerX << ','
              << pureStrafe.playerZ << " diagonal=" << diagonal.playerX << ','
              << diagonal.playerZ << " retreat=" << retreat.playerX << ','
              << retreat.playerZ << " corner-out/in=" << cornerOutward.playerX
              << '/' << cornerInward.playerX << " rotate-forward="
              << rotateThenForward.playerX << ','
              << rotateThenForward.playerZ << '\n';
    const bool movementEscapeSafe = Require(
        finiteSnapshot(pureStrafe) && finiteSnapshot(diagonal) &&
            finiteSnapshot(retreat) && finiteSnapshot(cornerOutward) &&
            finiteSnapshot(cornerInward) && finiteSnapshot(rotateThenForward) &&
            pureStrafe.playerX >= 0.02f &&
            std::abs(pureStrafe.playerZ + 9.70f) <= 0.0001f &&
            diagonal.playerX >= 0.01f &&
            diagonal.playerZ < -9.70f &&
            retreat.playerZ >= -9.68f &&
            cornerOutward.playerX >= 0.72f &&
            cornerInward.playerX <= 0.68f &&
            rotateThenForward.playerX >= 0.02f &&
            std::abs(rotateThenForward.playerZ + 9.70f) <= 0.0001f,
        "claimed reward movement must retain ordinary corridor strafe, diagonal, retreat, and rotated-forward behavior without an invisible carry wall");
    allWallPlayerMetricsSafe = allWallPlayerMetricsSafe && movementEscapeSafe;
    if (!allWallPlayerMetricsSafe) return 1;

    SkinnedCharacterModel lich;
    const auto lichPath = root / "assets/models/enemies/meshy/lich_placeholder_merged_animations_v01.glb";
    if (!Require(lich.LoadClips(lichPath.string(), LichPlaceholderClipSet(), diagnostic), diagnostic.c_str())) return 1;
    if (!Require(lich.HasTexcoords(), "lich TEXCOORD_0 was not imported")) return 1;
    if (!Require(lich.ExpandedVertexCount() == 27564u, "lich expanded vertex count changed")) return 1;
    if (!Require(lich.ClipDuration(SkinnedClip::Idle) > 2.3f, "lich idle mapping is wrong")) return 1;
    if (!Require(lich.ClipDuration(SkinnedClip::Walking) > 1.0f, "lich walking mapping is wrong")) return 1;
    if (!Require(lich.ClipDuration(SkinnedClip::Attack) == 0.0f, "lich attack must remain explicitly unmapped")) return 1;
    if (!Require(lich.ClipDuration(SkinnedClip::Dead) > 2.9f, "lich death mapping is wrong")) return 1;

    std::vector<TexturedSkinnedRtVertex> lichTextured;
    if (!Require(lich.SkinTextured(SkinnedClip::Idle, 0.5f, lichTextured, diagnostic), diagnostic.c_str())) return 1;
    if (!Require(FiniteTexturedVertices(lichTextured), "lich textured vertices are invalid")) return 1;
    std::ifstream emissionFile(root / "assets/textures/meshy/lich_placeholder_v01/emissive-2048-rgba8.ktx2",
                               std::ios::binary | std::ios::ate);
    if (!Require(static_cast<bool>(emissionFile), "derived lich emission audit KTX2 is missing")) return 1;
    const std::size_t emissionFileSize = static_cast<std::size_t>(emissionFile.tellg());
    std::vector<unsigned char> emissionKtx(emissionFileSize);
    emissionFile.seekg(0, std::ios::beg);
    emissionFile.read(reinterpret_cast<char*>(emissionKtx.data()), static_cast<std::streamsize>(emissionKtx.size()));
    const auto readU32 = [&emissionKtx](std::size_t offset) {
        return static_cast<std::uint32_t>(emissionKtx[offset]) |
               (static_cast<std::uint32_t>(emissionKtx[offset + 1u]) << 8u) |
               (static_cast<std::uint32_t>(emissionKtx[offset + 2u]) << 16u) |
               (static_cast<std::uint32_t>(emissionKtx[offset + 3u]) << 24u);
    };
    const auto readU64 = [&readU32](std::size_t offset) {
        return static_cast<std::uint64_t>(readU32(offset)) |
               (static_cast<std::uint64_t>(readU32(offset + 4u)) << 32u);
    };
    constexpr unsigned char kKtx2Identifier[12] = {
        0xABu, 0x4Bu, 0x54u, 0x58u, 0x20u, 0x32u, 0x30u, 0xBBu, 0x0Du, 0x0Au, 0x1Au, 0x0Au};
    bool validIdentifier = emissionKtx.size() >= 104u;
    for (std::size_t i = 0; validIdentifier && i < 12u; ++i)
    {
        validIdentifier = emissionKtx[i] == kKtx2Identifier[i];
    }
    if (!Require(validIdentifier && readU32(12u) == 43u && readU32(20u) == 2048u &&
                 readU32(24u) == 2048u && readU32(40u) == 1u,
                 "derived lich emission audit KTX2 header changed")) return 1;
    const std::uint64_t levelOffset = readU64(80u);
    const std::uint64_t levelLength = readU64(88u);
    if (!Require(levelLength == 2048ull * 2048ull * 4ull &&
                 levelOffset <= emissionKtx.size() && levelLength <= emissionKtx.size() - levelOffset,
                 "derived lich emission KTX2 payload is invalid")) return 1;
    std::vector<unsigned char> emissionPixels(
        emissionKtx.begin() + static_cast<std::ptrdiff_t>(levelOffset),
        emissionKtx.begin() + static_cast<std::ptrdiff_t>(levelOffset + levelLength));
    std::size_t emissiveVertexCount = 0u;
    std::size_t outerEmissiveVertexCount = 0u;
    float outerX = 0.0f, outerY = 0.0f, outerZ = 0.0f;
    float emissiveMinX = 1.0e9f, emissiveMaxX = -1.0e9f;
    for (std::size_t vertexIndex = 0; vertexIndex < lichTextured.size(); ++vertexIndex)
    {
        const auto& vertex = lichTextured[vertexIndex];
        const float wrappedU = vertex.texcoord[0] - std::floor(vertex.texcoord[0]);
        const float wrappedV = vertex.texcoord[1] - std::floor(vertex.texcoord[1]);
        const std::size_t x = std::min<std::size_t>(2047u, static_cast<std::size_t>(wrappedU * 2048.0f));
        const std::size_t y = std::min<std::size_t>(2047u, static_cast<std::size_t>(wrappedV * 2048.0f));
        const std::size_t offset = (y * 2048u + x) * 4u;
        if (offset + 2u < emissionPixels.size() &&
            (emissionPixels[offset] || emissionPixels[offset + 1u] || emissionPixels[offset + 2u]))
        {
            ++emissiveVertexCount;
            emissiveMinX = std::min(emissiveMinX, vertex.position[0]);
            emissiveMaxX = std::max(emissiveMaxX, vertex.position[0]);
            if (vertex.position[0] > 0.55f)
            {
                ++outerEmissiveVertexCount;
                outerX += vertex.position[0];
                outerY += vertex.position[1];
                outerZ += vertex.position[2];
            }
        }
    }
    if (!Require(emissiveVertexCount > 0u, "no emissive lich vertices were found by UV audit")) return 1;
    if (!Require(outerEmissiveVertexCount == 40u, "audited staff crystal vertex set changed")) return 1;
    if (!Require(outerX / 40.0f > 0.90f && outerY / 40.0f > 0.70f,
                 "audited staff crystal sample moved into the robe or eye cluster")) return 1;
    std::vector<SkinnedRtVertex> unavailableAttack;
    if (!Require(!lich.Skin(SkinnedClip::Attack, 0.5f, unavailableAttack, diagnostic), "unmapped lich attack unexpectedly skinned")) return 1;

    std::cout << "Skinned character model smoke passed: skeleton=" << skeleton.ExpandedVertexCount()
              << " player=" << player.ExpandedVertexCount()
              << " lich=" << lich.ExpandedVertexCount() << " textured std430 stride=" << sizeof(TexturedSkinnedRtVertex)
              << " emissiveVertices=" << emissiveVertexCount << " emissiveX=" << emissiveMinX << ".." << emissiveMaxX
              << " outer=" << outerEmissiveVertexCount << " avg="
              << outerX / static_cast<float>(std::max<std::size_t>(1u, outerEmissiveVertexCount)) << ','
              << outerY / static_cast<float>(std::max<std::size_t>(1u, outerEmissiveVertexCount)) << ','
              << outerZ / static_cast<float>(std::max<std::size_t>(1u, outerEmissiveVertexCount)) << '\n';
    return 0;
}
