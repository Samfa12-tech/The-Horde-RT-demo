#include "scene/EntryPortalCapGeometry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>

namespace
{
using Point = std::array<float, 3u>;

struct Ray
{
    Point origin;
    Point direction;
};

struct Hit
{
    float distance = std::numeric_limits<float>::infinity();
    unsigned int normalCode = 0u;
};

bool IntersectTriangle(const Ray& ray, const Point& a, const Point& b,
                       const Point& c, float& distance)
{
    const Point e1{b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    const Point e2{c[0] - a[0], c[1] - a[1], c[2] - a[2]};
    const Point p{ray.direction[1] * e2[2] - ray.direction[2] * e2[1],
                  ray.direction[2] * e2[0] - ray.direction[0] * e2[2],
                  ray.direction[0] * e2[1] - ray.direction[1] * e2[0]};
    const float det = e1[0] * p[0] + e1[1] * p[1] + e1[2] * p[2];
    if (std::abs(det) < 1.0e-7f) return false;
    const float invDet = 1.0f / det;
    const Point t{ray.origin[0] - a[0], ray.origin[1] - a[1], ray.origin[2] - a[2]};
    const float u = (t[0] * p[0] + t[1] * p[1] + t[2] * p[2]) * invDet;
    if (u < 0.0f || u > 1.0f) return false;
    const Point q{t[1] * e1[2] - t[2] * e1[1],
                  t[2] * e1[0] - t[0] * e1[2],
                  t[0] * e1[1] - t[1] * e1[0]};
    const float v = (ray.direction[0] * q[0] + ray.direction[1] * q[1] +
                     ray.direction[2] * q[2]) * invDet;
    if (v < 0.0f || u + v > 1.0f) return false;
    distance = (e2[0] * q[0] + e2[1] * q[1] + e2[2] * q[2]) * invDet;
    return distance > 1.0e-5f;
}

Hit Trace(const Ray& ray)
{
    Hit nearest;
    for (const auto& face : horde::scene::EntryPortalCapFaces())
    {
        float distance = 0.0f;
        if (IntersectTriangle(ray, face.vertices[0], face.vertices[1], face.vertices[2], distance) &&
            distance < nearest.distance)
            nearest = {distance, face.normalCode};
        if (IntersectTriangle(ray, face.vertices[0], face.vertices[2], face.vertices[3], distance) &&
            distance < nearest.distance)
            nearest = {distance, face.normalCode};
    }
    // The room ceiling is the shared cap top; include its real authored patch
    // in this CPU ray query so coverage checks the joined geometry, not a mock.
    const auto& ceiling = horde::scene::kShowcaseCeilingPatches[5u];
    std::array<Point, 4u> ceilingCorners{};
    for (std::size_t i = 0u; i < ceilingCorners.size(); ++i)
        ceilingCorners[i] = {{ceiling.footprint[i][0], ceiling.bottomY,
                              ceiling.footprint[i][1]}};
    float distance = 0.0f;
    if (IntersectTriangle(ray, ceilingCorners[0], ceilingCorners[1], ceilingCorners[2], distance) &&
        distance < nearest.distance)
        nearest = {distance, 1u};
    if (IntersectTriangle(ray, ceilingCorners[0], ceilingCorners[2], ceilingCorners[3], distance) &&
        distance < nearest.distance)
        nearest = {distance, 1u};
    return nearest;
}

int failures = 0;
void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

bool At(float actual, float expected)
{
    return std::abs(actual - expected) <= 1.0e-4f;
}

void TestPortalCapRayCoverage()
{
    const float minX = -0.90f, maxX = 0.90f;
    const float bottomY = 0.85f, topY = horde::scene::kShowcaseRouteCeilingWorldY;
    const float frontZ = -6.40f, backZ = -6.47f;
    const auto fromFront = Trace({{{0.0f, 1.05f, -6.0f}}, {{0.0f, 0.0f, -1.0f}}});
    Check(At(fromFront.distance, 0.40f) && fromFront.normalCode == 4u,
          "entry-facing cap plane is ray-visible at its actual front threshold");
    const auto fromBack = Trace({{{0.0f, 1.05f, -6.8f}}, {{0.0f, 0.0f, 1.0f}}});
    Check(At(fromBack.distance, 0.33f),
          "backside cap plane closes the far side of the actual threshold");
    const auto fromBelow = Trace({{{0.0f, 0.70f, -6.435f}}, {{0.0f, 1.0f, 0.0f}}});
    Check(At(fromBelow.distance, bottomY - 0.70f),
          "cap underside is real ray-visible geometry");
    const auto fromAbove = Trace({{{0.0f, 1.60f, -6.435f}}, {{0.0f, -1.0f, 0.0f}}});
    Check(At(fromAbove.distance, 1.60f - topY) && fromAbove.normalCode == 1u,
          "cap top joins the existing route ceiling without a gap or duplicate face");
    const auto fromLeft = Trace({{{-1.10f, 1.05f, -6.435f}}, {{1.0f, 0.0f, 0.0f}}});
    Check(At(fromLeft.distance, minX - (-1.10f)),
          "west cap side joins the existing shared solid bounds");
    const auto fromRight = Trace({{{1.10f, 1.05f, -6.435f}}, {{-1.0f, 0.0f, 0.0f}}});
    Check(At(fromRight.distance, 1.10f - maxX),
          "east cap side joins the existing shared solid bounds");
    const auto openAperture = Trace({{{0.0f, 0.20f, -6.0f}}, {{0.0f, 0.0f, -1.0f}}});
    Check(!std::isfinite(openAperture.distance),
          "the pedestrian opening below the cap remains clear through the threshold");

    const auto& volume = horde::scene::kShowcaseLowOverheadVolumes[1u];
    Check(At(minX, volume.footprint[1u][0]) && At(maxX, volume.footprint[2u][0]) &&
              At(frontZ, volume.footprint[0u][1]) && At(backZ, volume.footprint[1u][1]) &&
              At(bottomY, volume.bottomY) &&
              At(topY, horde::scene::kShowcaseRouteCeilingWorldY) &&
              volume.topY >= topY,
          "rendered cap uses existing collision footprint and meets the route ceiling conservatively");
    const auto faces = horde::scene::EntryPortalCapFaces();
    Check(faces[0].materialCode == 2u && faces[1].materialCode == 2u &&
              faces[2].materialCode == 2u && faces[3].materialCode == 2u &&
              faces[4].materialCode == 2u,
          "all exposed cap faces use admitted masonry rather than a visible flat hidden shell");
    constexpr std::array<Point, 6u> expectedNormals{{
        {{0.0f, 1.0f, 0.0f}}, {{0.0f, -1.0f, 0.0f}},
        {{1.0f, 0.0f, 0.0f}}, {{-1.0f, 0.0f, 0.0f}},
        {{0.0f, 0.0f, 1.0f}}, {{0.0f, 0.0f, -1.0f}},
    }};
    constexpr std::array<unsigned int, 5u> faceNormals{{4u, 5u, 3u, 2u, 1u}};
    for (std::size_t faceIndex = 0u; faceIndex < faces.size(); ++faceIndex)
    {
        const auto& face = faces[faceIndex];
        const Point a = face.vertices[0], b = face.vertices[1], c = face.vertices[2];
        const Point e1{b[0] - a[0], b[1] - a[1], b[2] - a[2]};
        const Point e2{c[0] - a[0], c[1] - a[1], c[2] - a[2]};
        const Point normal{e1[1] * e2[2] - e1[2] * e2[1],
                           e1[2] * e2[0] - e1[0] * e2[2],
                           e1[0] * e2[1] - e1[1] * e2[0]};
        const Point expected = expectedNormals[faceNormals[faceIndex]];
        Check(normal[0] * expected[0] + normal[1] * expected[1] +
                  normal[2] * expected[2] > 0.0f,
              "each generated cap face winding agrees with its cardinal surface code");
    }
}
} // namespace

int main()
{
    TestPortalCapRayCoverage();
    if (failures != 0)
    {
        std::cerr << failures << " portal geometry checks failed\n";
        return 1;
    }
    std::cout << "Entry portal cap CPU ray coverage passed\n";
    return 0;
}
