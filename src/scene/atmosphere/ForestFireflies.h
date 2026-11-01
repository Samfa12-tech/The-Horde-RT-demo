#pragma once
#include "scene/atmosphere/IndoorDust.h"
#include <optional>

namespace horde::scene::atmosphere {
// Sparse authored world-space pockets. No lights, meshes, AS updates or new
// upload slots. Standard enables these; Low/Off leave the phone path unchanged.
inline constexpr std::array<std::array<float,2>,3> kForestFireflyPockets{{
    {{-29.f,-19.f}}, {{-12.f,1.f}}, {{-7.f,22.f}}
}};
inline constexpr unsigned kForestFirefliesPerPocket=4;
inline constexpr float kForestFireflyDistance=12.f;
inline bool ForestFirefliesNearby(const DustCamera& camera) noexcept {
    if(!Finite(camera.origin))return false;
    for(const auto& p:kForestFireflyPockets)
        if(std::hypot(camera.origin[0]-p[0],camera.origin[2]-p[1])<15.f)return true;
    return false;
}
// Ground returns the actual imported terrain height, or no value. No blockout
// elevation or camera-relative positioning substitutes for missing ground.
template<class Ground>
unsigned OverlayForestFireflies(horde::graphics::DustQuality quality,
    const DustCamera& camera,double seconds,Ground&& ground,DustFrame& frame,DustWork& work) {
    if(quality!=horde::graphics::DustQuality::Standard || !ForestFirefliesNearby(camera) ||
       !std::isfinite(seconds) || seconds<0)return 0;
    // Keep at most32 ambient motes. The12 fireflies fit below the hit-fragment
    // reservation [48,64), so contact feedback always retains priority.
    constexpr unsigned ambientLimit=32;
    for(auto& tile:frame.tiles)for(auto& id:tile)
        if(id!=kDustEmptyCandidate && id>=ambientLimit){id=kDustEmptyCandidate;--work.tileReferences;}
    work.generatedMotes=std::min(work.generatedMotes,ambientLimit);
    unsigned added=0;
    for(unsigned pocket=0;pocket<kForestFireflyPockets.size();++pocket) {
        const auto center=kForestFireflyPockets[pocket];
        if(std::hypot(camera.origin[0]-center[0],camera.origin[2]-center[1])>=15.f)continue;
        for(unsigned n=0;n<kForestFirefliesPerPocket;++n) {
            const auto seed=0x91e7u+pocket*977u+n*37u;
            const double phase=6.28318530718*DustUnit(seed);
            const float x=center[0]+(DustUnit(seed+1)-.5f)*5.f+.35f*float(std::sin(seconds*.27+phase));
            const float z=center[1]+(DustUnit(seed+2)-.5f)*5.f+.35f*float(std::cos(seconds*.23+phase));
            if(std::hypot(camera.origin[0]-x,camera.origin[2]-z)>=kForestFireflyDistance)continue;
            const std::optional<float> height=ground(x,z);
            if(!height || !std::isfinite(*height))continue;
            const Vec3 p{x,*height+.75f+DustUnit(seed+3)*.65f+.15f*float(std::sin(seconds*.31+phase)),z};
            const float distance=std::sqrt(Dot(Sub(p,camera.origin),Sub(p,camera.origin)));
            if(distance>=kForestFireflyDistance || distance<=.3f)continue;
            const float fade=std::clamp((kForestFireflyDistance-distance)/2.f,0.f,1.f)*
                std::clamp((distance-.3f)/.5f,0.f,1.f);
            const float pulse=.35f+.65f*std::pow(.5f+.5f*float(std::sin(seconds*1.4+phase)),2.f);
            const unsigned index=work.generatedMotes++;
            auto& mote=frame.motes[index];
            mote.positionRadius={p[0],p[1],p[2],.025f};
            mote.response={fade*pulse,0,3,0};
            ++added;
            DustProjection projection;
            if(!ProjectDustSphere(p,.025f,camera,projection))continue;
            ++work.projectedMotes;
            const auto tx=[](float v){return unsigned(std::clamp(v,0.f,.999999f)*kDustTileColumns);};
            const auto ty=[](float v){return unsigned(std::clamp(v,0.f,.999999f)*kDustTileRows);};
            for(unsigned y=ty(projection.top);y<=ty(projection.bottom);++y)
                for(unsigned xTile=tx(projection.left);xTile<=tx(projection.right);++xTile) {
                    auto& tile=frame.tiles[y*kDustTileColumns+xTile];
                    auto free=std::find(tile.begin(),tile.end(),kDustEmptyCandidate);
                    if(free==tile.end()){++work.overflowReferences;continue;}
                    *free=index;++work.tileReferences;
                }
        }
    }
    work.fireflyMotes=added;
    return added;
}
static_assert(kForestFireflyPockets.size()*kForestFirefliesPerPocket+32<=48);
} // namespace horde::scene::atmosphere
