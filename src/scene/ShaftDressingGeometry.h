#pragma once

#include "scene/ShowcaseOverheadGeometry.h"

#include <array>
#include <cmath>
#include <vector>

namespace horde::scene
{
using DressingPoint = std::array<float, 3u>;
struct OpaqueDressingQuad
{
    std::array<DressingPoint, 4u> vertices;
    unsigned int normalCode;
};
enum class DressingLeafPlane { Horizontal, WallFacing };

// Four closed masonry walls around a clear rectangular aperture. The inner
// footprint stays open at every height; each wall's top is a real receding rim.
// These neutral solids use the ordinary world-box authoring and RT paths.
constexpr std::array<OverheadVolume, 4u> MakeMasonryWell(
    float minX, float minZ, float maxX, float maxZ,
    float bottomY, float topY, float thickness)
{
    return {{
        RectangularOverhead(minX - thickness, minZ - thickness,
            minX, maxZ + thickness, bottomY, topY),
        RectangularOverhead(maxX, minZ - thickness,
            maxX + thickness, maxZ + thickness, bottomY, topY),
        RectangularOverhead(minX, minZ - thickness,
            maxX, minZ, bottomY, topY),
        RectangularOverhead(minX, maxZ,
            maxX, maxZ + thickness, bottomY, topY),
    }};
}

// First-bend wall panel, distinct from both chamber skylights. Begin at the
// retained roof height so the front rim never reduces player/held clearance.
inline constexpr auto kWallPanelMasonryWell = MakeMasonryWell(
    2.05f, -8.80f, 3.10f, -8.35f, kShowcaseRouteCeilingWorldY, 4.10f, 0.16f);

// Static opaque mesh authoring data. Leaves have an actual diamond silhouette
// and thickness: no alpha cards, invisible obstruction or extra texture fetch.
// This recipe can be instanced/consolidated for any supported masonry rim.
inline std::vector<OpaqueDressingQuad> MakeHangingSprig(
    DressingPoint attachment, float length, float phase,
    DressingLeafPlane leafPlane = DressingLeafPlane::Horizontal)
{
    std::vector<OpaqueDressingQuad> result;
    result.reserve(84u);
    const auto quad = [&result](DressingPoint a, DressingPoint b,
                               DressingPoint c, DressingPoint d, unsigned int n) {
        result.push_back({{a, b, c, d}, n});
    };
    const auto box = [&quad](DressingPoint lo, DressingPoint hi) {
        quad({lo[0],lo[1],lo[2]},{lo[0],hi[1],lo[2]},
             {hi[0],hi[1],lo[2]},{hi[0],lo[1],lo[2]},5u);
        quad({hi[0],lo[1],hi[2]},{hi[0],hi[1],hi[2]},
             {lo[0],hi[1],hi[2]},{lo[0],lo[1],hi[2]},4u);
        quad({lo[0],lo[1],hi[2]},{lo[0],hi[1],hi[2]},
             {lo[0],hi[1],lo[2]},{lo[0],lo[1],lo[2]},3u);
        quad({hi[0],lo[1],lo[2]},{hi[0],hi[1],lo[2]},
             {hi[0],hi[1],hi[2]},{hi[0],lo[1],hi[2]},2u);
        quad({lo[0],hi[1],lo[2]},{lo[0],hi[1],hi[2]},
             {hi[0],hi[1],hi[2]},{hi[0],hi[1],lo[2]},0u);
        quad({lo[0],lo[1],hi[2]},{lo[0],lo[1],lo[2]},
             {hi[0],lo[1],lo[2]},{hi[0],lo[1],hi[2]},1u);
    };
    constexpr unsigned int segments = 8u;
    const auto endpoint = [&attachment, length, phase](float t) {
        // The attachment is an actual endpoint. A bounded lateral curve gives
        // variation without introducing separate disconnected stem fragments.
        const float bend = std::sin(t * 3.14159265358979323846f);
        return DressingPoint{attachment[0] + 0.035f * std::sin(phase+t*4.0f) * bend,
                             attachment[1] - length*t,
                             attachment[2] + 0.025f * std::cos(phase+t*3.0f) * bend};
    };
    for (unsigned int i=0u; i<segments; ++i)
    {
        const float t=static_cast<float>(i)/segments;
        const auto start=endpoint(t);
        const auto end=endpoint(static_cast<float>(i+1u)/segments);
        DressingPoint lo{}, hi{};
        for (unsigned int axis=0u; axis<3u; ++axis)
        {
            lo[axis]=std::fmin(start[axis],end[axis])-0.004f;
            hi[axis]=std::fmax(start[axis],end[axis])+0.004f;
        }
        // Each closed box spans both endpoints, so adjacent segments overlap
        // at their shared endpoint even when the curve changes lateral axis.
        box(lo,hi);
        if (i == 0u || i == 7u) continue;
        const auto leafBegin = result.size();
        const float x=(start[0]+end[0])*0.5f;
        const float y=(start[1]+end[1])*0.5f;
        const float z=(start[2]+end[2])*0.5f;
        const float side=(i%2u)==0u ? 1.0f : -1.0f;
        const float leafLength=0.105f*(0.88f+0.12f*std::sin(phase+i));
        const std::array<DressingPoint,4u> leaf{{
            {x,y,z}, {x+side*leafLength*0.53f,y,z-0.027f},
            {x+side*leafLength,y,z}, {x+side*leafLength*0.53f,y,z+0.027f}}};
        // Order upward and downward faces by side to keep outward winding.
        const auto a=side>0.0f ? leaf[0] : leaf[3];
        const auto b=side>0.0f ? leaf[3] : leaf[0];
        const auto c=side>0.0f ? leaf[2] : leaf[1];
        const auto d=side>0.0f ? leaf[1] : leaf[2];
        quad(a,b,c,d,0u);
        auto aa=a,bb=b,cc=c,dd=d;
        aa[1]-=0.003f;bb[1]-=0.003f;cc[1]-=0.003f;dd[1]-=0.003f;
        quad(dd,cc,bb,aa,1u);
        // Horizontal diamond edge faces point mostly along world Z. Winding
        // changes between left/right leaves; choose the outward shared axis
        // from each edge rather than assuming a fixed four-face box order.
        const auto edgeNormal = [](const DressingPoint& start, const DressingPoint& end) {
            return end[0] > start[0] ? 4u : 5u;
        };
        quad(a,aa,bb,b,edgeNormal(a,b));quad(b,bb,cc,c,edgeNormal(b,c));
        quad(c,cc,dd,d,edgeNormal(c,d));quad(d,dd,aa,a,edgeNormal(d,a));
        if (leafPlane == DressingLeafPlane::WallFacing)
        {
            // Rotate the closed leaf around its attached root, not the stem.
            // This is ordinary authored orientation with shared axis normals.
            constexpr std::array<unsigned,6u> rotatedNormal{{5u,4u,2u,3u,0u,1u}};
            for (auto face = leafBegin; face < result.size(); ++face)
            {
                for (auto& point : result[face].vertices)
                {
                    const float dy = point[1] - y, dz = point[2] - z;
                    point[1] = y + dz; point[2] = z - dy;
                }
                result[face].normalCode = rotatedNormal[result[face].normalCode];
            }
        }
    }
    return result;
}

struct HangingSprigPlacement
{
    DressingPoint attachment;
    float length;
    float phase;
    DressingLeafPlane leafPlane = DressingLeafPlane::Horizontal;
};
inline constexpr float kWaterShaftTopWorldY=5.60f;
inline constexpr std::array<HangingSprigPlacement,3u> kWaterShaftSprigs{{
    // Roots intersect the existing masonry rim. Unequal hanging lengths put
    // actual lower leaf silhouettes in the ordinary dry-side player view,
    // while retaining the same three sprigs and immutable geometry budget.
    {{-2.898f,5.58f,-15.72f},4.70f,-0.4f},
    {{-1.582f,5.58f,-15.64f},3.40f,2.1f},
    {{-2.65f,5.58f,-14.722f},4.95f,1.6f},
}};
// Restrained wall growth rooted on the two retained masonry jambs. The central
// four-bar access panel remains readable; no new light, alpha card or material.
inline constexpr std::array<HangingSprigPlacement,2u> kWallPanelSprigs{{
    {{2.052f,1.18f,-8.801f},0.96f,0.7f,DressingLeafPlane::WallFacing},
    {{3.098f,1.22f,-8.801f},1.18f,2.4f,DressingLeafPlane::WallFacing},
}};
} // namespace horde::scene
