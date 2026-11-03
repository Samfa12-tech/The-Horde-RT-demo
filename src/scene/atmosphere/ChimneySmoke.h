#pragma once
#include "scene/atmosphere/IndoorDust.h"

namespace horde::scene::atmosphere {
// Cap centres from bellwether-shell-v4, B01/B03/B06. The original authored
// chimneys remain untouched. Smoke uses the shared RT-depth-clipped, lit
// analytic particle path: no new lights, geometry, textures or AS updates.
inline constexpr std::array<Vec3,3> kBellwetherChimneyCaps{{
    {{95.36f,-0.27955078125f,109.28f}},
    {{38.98f,-3.878525390625f,116.76f}},
    {{111.44f,-6.7426123046875f,153.52f}}
}};
inline bool ChimneySmokeNearby(const DustCamera& camera) noexcept {
    if(!Finite(camera.origin))return false;
    for(const auto& cap:kBellwetherChimneyCaps)
        if(Dot(Sub(cap,camera.origin),Sub(cap,camera.origin))<180.f*180.f)return true;
    return false;
}
inline unsigned OverlayChimneySmoke(horde::graphics::DustQuality quality,
    const DustCamera& camera,double seconds,DustFrame& frame,DustWork& work) {
    if(!horde::graphics::ValidDustQuality(quality) || quality==horde::graphics::DustQuality::Off || !ChimneySmokeNearby(camera) ||
       !std::isfinite(seconds) || seconds<0)return 0;
    const unsigned count=quality==horde::graphics::DustQuality::Low?3u:5u;
    unsigned added=0;
    for(unsigned chimney=0;chimney<kBellwetherChimneyCaps.size();++chimney) {
        const auto cap=kBellwetherChimneyCaps[chimney];
        const float distance=std::sqrt(Dot(Sub(cap,camera.origin),Sub(cap,camera.origin)));
        if(distance>=180.f)continue;
        for(unsigned n=0;n<count && work.generatedMotes<48;++n) {
            // Absolute simulation time: smooth birth/death, deterministic on
            // recreation, stationary while paused, never camera-relative.
            const float age=float(std::fmod(seconds*.065+double(n)/count+chimney*.137,1.0));
            const float fade=std::clamp(age*10.f,0.f,1.f)*std::clamp((1.f-age)*4.f,0.f,1.f)*
                std::clamp((180.f-distance)/25.f,0.f,1.f);
            const Vec3 p{cap[0]+age*2.2f+.18f*std::sin(age*9.f+chimney),
                cap[1]+.10f+age*6.5f,cap[2]+age*.9f};
            const float radius=.34f+age*1.35f;
            DustProjection projection;
            if(!ProjectDustSphere(p,radius,camera,projection))continue;
            const unsigned index=work.generatedMotes++;
            frame.motes[index].positionRadius={p[0],p[1],p[2],radius};
            frame.motes[index].response={.28f*fade,0,0,0};
            ++added;++work.projectedMotes;
            const auto tx=[](float v){return unsigned(std::clamp(v,0.f,.999999f)*kDustTileColumns);};
            const auto ty=[](float v){return unsigned(std::clamp(v,0.f,.999999f)*kDustTileRows);};
            for(unsigned y=ty(projection.top);y<=ty(projection.bottom);++y)
                for(unsigned x=tx(projection.left);x<=tx(projection.right);++x) {
                    auto& tile=frame.tiles[y*kDustTileColumns+x];
                    const auto free=std::find(tile.begin(),tile.end(),kDustEmptyCandidate);
                    if(free==tile.end()){++work.overflowReferences;continue;}
                    *free=index;++work.tileReferences;
                }
        }
    }
    work.smokeMotes=added;
    return added;
}
} // namespace horde::scene::atmosphere
