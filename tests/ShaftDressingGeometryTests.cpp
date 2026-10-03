#include "scene/ShaftDressingGeometry.h"

#include <array>
#include <cmath>
#include <iostream>

namespace {
using Point = horde::scene::DressingPoint;
Point Difference(const Point& a, const Point& b)
{
    return {a[0]-b[0], a[1]-b[1], a[2]-b[2]};
}
Point Cross(const Point& a, const Point& b)
{
    return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]};
}
float Dot(const Point& a, const Point& b)
{
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
bool CheckSprig(const horde::scene::HangingSprigPlacement& placement)
{
    // This is the public shared axis-normal ABI, not a copy of the authoring
    // recipe. Mathematical winding/hemisphere checks cover both leaf sides
    // and both triangles of every emitted closed stem and thick leaf face.
    constexpr std::array<Point, 6> axes{{{0,1,0},{0,-1,0},{1,0,0},{-1,0,0},{0,0,1},{0,0,-1}}};
    const auto faces = horde::scene::MakeHangingSprig(placement.attachment, placement.length, placement.phase);
    if (faces.size() > 96 || faces.empty()) return false;
    for (const auto& face : faces)
    {
        if (face.normalCode >= axes.size()) return false;
        for (const auto& point : face.vertices)
            for (const auto value : point) if (!std::isfinite(value)) return false;
        for (const auto corners : std::array<std::array<unsigned,3>,2>{{{0,1,2},{0,2,3}}})
        {
            const auto normal = Cross(Difference(face.vertices[corners[1]], face.vertices[corners[0]]),
                                      Difference(face.vertices[corners[2]], face.vertices[corners[0]]));
            const float length = std::sqrt(Dot(normal,normal));
            if (!(length > 1e-8f) || Dot(normal,axes[face.normalCode]) / length < 0.75f)
            {
                std::cerr << "face winding and authored normal disagree: " << face.normalCode << "\n";
                return false;
            }
        }
    }
    return true;
}
}
int main()
{
    bool passed = true;
    for (const auto& placement : horde::scene::kWaterShaftSprigs) passed = CheckSprig(placement) && passed;
    // Different phase/length/translation still contains alternating left/right
    // leaves; attachment should not change normals or create degenerate faces.
    passed = CheckSprig({{1.25f,3.0f,-2.0f},0.90f,1.6f}) && passed;
    passed = CheckSprig({{-4.0f,5.5f,-8.0f},2.30f,5.9f}) && passed;
    return passed ? 0 : 1;
}
