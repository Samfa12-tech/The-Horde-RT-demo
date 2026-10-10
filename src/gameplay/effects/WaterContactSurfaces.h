#pragma once

#include "gameplay/ShowcaseRoute.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <limits>

namespace horde::gameplay::effects
{
using WaterPoint = std::array<float, 3>;
using WaterTriangle = std::array<WaterPoint, 3>;
inline constexpr WaterPoint kCatchmentCentre{-2.32f, -0.925f, -15.26f};
inline constexpr std::array<std::array<float, 2>, 16> kCatchmentRim{{
    {-2.32f,-14.72f},{-2.13f,-14.77f},{-1.96f,-14.90f},{-1.85f,-15.06f},
    {-1.82f,-15.26f},{-1.86f,-15.47f},{-1.98f,-15.65f},{-2.15f,-15.76f},
    {-2.35f,-15.80f},{-2.55f,-15.75f},{-2.73f,-15.61f},{-2.83f,-15.42f},
    {-2.86f,-15.21f},{-2.79f,-14.99f},{-2.65f,-14.83f},{-2.48f,-14.74f}}};
inline constexpr std::array<std::array<WaterPoint,4>,6> kRunoffQuads{{
    {{{-2.48f,-.924f,-14.96f},{-2.48f,-.924f,-15.54f},{-3.10f,-.926f,-15.48f},{-3.10f,-.926f,-15.02f}}},
    {{{-3.10f,-.926f,-15.02f},{-3.10f,-.926f,-15.48f},{-3.72f,-.928f,-15.46f},{-3.72f,-.928f,-14.98f}}},
    {{{-3.72f,-.928f,-14.98f},{-3.72f,-.928f,-15.46f},{-4.35f,-.930f,-15.40f},{-4.35f,-.930f,-15.05f}}},
    {{{-4.35f,-.930f,-15.05f},{-4.35f,-.930f,-15.40f},{-4.88f,-.931f,-15.36f},{-4.88f,-.931f,-15.08f}}},
    {{{-4.88f,-.931f,-15.08f},{-4.88f,-.931f,-15.36f},{-5.30f,-.932f,-15.33f},{-5.30f,-.932f,-15.11f}}},
    {{{-5.30f,-.932f,-15.11f},{-5.30f,-.932f,-15.33f},{-5.38f,-1.08f,-15.30f},{-5.38f,-1.08f,-15.14f}}}
}};
inline std::array<WaterTriangle,28> WaterSurfaceTriangles()
{
    std::array<WaterTriangle,28> result{};
    for(std::size_t i=0;i<16;++i)
        result[i]={kCatchmentCentre,WaterPoint{kCatchmentRim[i][0],-.925f,kCatchmentRim[i][1]},
                   WaterPoint{kCatchmentRim[(i+1)%16][0],-.925f,kCatchmentRim[(i+1)%16][1]}};
    for(std::size_t i=0;i<6;++i) {
        result[16+i*2]={kRunoffQuads[i][0],kRunoffQuads[i][1],kRunoffQuads[i][2]};
        result[17+i*2]={kRunoffQuads[i][0],kRunoffQuads[i][2],kRunoffQuads[i][3]};
    }
    return result;
}
inline bool WaterSurfaceAt(float x,float z,float& y)
{
    if(!std::isfinite(x)||!std::isfinite(z)) return false;
    bool found=false;
    float highest=-std::numeric_limits<float>::infinity();
    for(const auto& t:WaterSurfaceTriangles()) {
        const float ax=t[0][0],az=t[0][2],bx=t[1][0],bz=t[1][2],cx=t[2][0],cz=t[2][2];
        const float d=(bz-cz)*(ax-cx)+(cx-bx)*(az-cz);
        if(std::abs(d)<1e-8f) continue;
        const float u=((bz-cz)*(x-cx)+(cx-bx)*(z-cz))/d;
        const float v=((cz-az)*(x-cx)+(ax-cx)*(z-cz))/d;
        if(u>=-1e-6f && v>=-1e-6f && u+v<=1.000001f) {
            // Pool and runoff overlap at the join. Contact and ripples belong
            // to the topmost real face, matching a downward ray from the feet.
            highest=std::max(highest,u*t[0][1]+v*t[1][1]+(1-u-v)*t[2][1]);
            found=true;
        }
    }
    if(found) y=highest;
    return found;
}
inline bool CircleTouchesEllipse(float x,float z,float rx,float rz,float radius)
{
    x=std::abs(x); z=std::abs(z);
    if((x*x)/(rx*rx)+(z*z)/(rz*rz)<=1) return true;
    float lo=0,hi=std::max(rx*x,rz*z)+radius*radius+1;
    for(int i=0;i<32;++i) {
        const float l=(lo+hi)*.5f;
        const float a=rx*x/(l+rx*rx), b=rz*z/(l+rz*rz);
        if(a*a+b*b>1) lo=l; else hi=l;
    }
    const float l=(lo+hi)*.5f;
    return std::hypot(x-rx*rx*x/(l+rx*rx),z-rz*rz*z/(l+rz*rz))<=radius;
}
inline bool BodyTouchesWaterfall(float x,float supportY,float z,float width)
{
    if(!std::isfinite(x)||!std::isfinite(supportY)||!std::isfinite(z)||!std::isfinite(width)) return false;
    // Vertical capsule spans the real body. The widening taper makes its
    // highest intersecting section the conservative contact section.
    const float y=std::min(2.16f,supportY+1.65f);
    if(y<-.91f || supportY>2.16f) return false;
    const float taper=.70f+std::clamp(y+.91f,0.f,3.07f)*(.30f/3.07f);
    width=std::clamp(width,.25f,2.f);
    constexpr std::array<float,3> offsets{0,-.18f,.20f},rx{.006f,.003f,.003f},rz{.065f,.014f,.012f};
    for(std::size_t i=0;i<3;++i)
        if(CircleTouchesEllipse(x+2.32f,z-(-15.26f+offsets[i]*width),rx[i]*taper,rz[i]*width*taper,kPlayerCollisionRadius)) return true;
    return false;
}
}
