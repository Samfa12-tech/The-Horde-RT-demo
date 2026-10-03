#pragma once

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

// Static opaque mesh authoring data. Leaves have an actual diamond silhouette
// and thickness: no alpha cards, invisible obstruction or extra texture fetch.
// This recipe can be instanced/consolidated for any supported masonry rim.
inline std::vector<OpaqueDressingQuad> MakeHangingSprig(
    DressingPoint attachment, float length, float phase)
{
    std::vector<OpaqueDressingQuad> result;
    result.reserve(96u);
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
    for (unsigned int i=0u; i<segments; ++i)
    {
        const float t=static_cast<float>(i)/segments;
        const float x=attachment[0]+0.035f*std::sin(phase+t*4.0f);
        const float z=attachment[2]+0.025f*std::cos(phase+t*3.0f);
        const float y=attachment[1]-length*t;
        box({x-0.004f,y-length/segments-0.004f,z-0.004f},
            {x+0.004f,y+0.004f,z+0.004f});
        if (i == 0u || i == 7u) continue;
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
    }
    return result;
}

struct HangingSprigPlacement
{
    DressingPoint attachment;
    float length;
    float phase;
};
inline constexpr float kWaterShaftTopWorldY=5.60f;
inline constexpr std::array<HangingSprigPlacement,3u> kWaterShaftSprigs{{
    {{-2.87f,5.58f,-15.91f},2.12f,0.4f},
    {{-1.64f,5.58f,-15.64f},1.62f,2.1f},
    {{-2.65f,5.58f,-14.78f},2.28f,4.0f},
}};
} // namespace horde::scene
