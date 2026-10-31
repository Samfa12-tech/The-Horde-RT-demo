#pragma once
#include "scene/atmosphere/IndoorDust.h"
#include "gameplay/effects/EnemyHitParticles.h"

namespace horde::scene::atmosphere {
// Reuses binding 26: 64 motes, four depth-sorted candidates per tile. Contact
// bursts take priority over at most 16 ambient motes, never grow the upload.
inline void OverlayHitParticles(const horde::gameplay::effects::EnemyHitParticles& hits,
    horde::graphics::DustQuality quality,const DustCamera& camera,DustFrame& frame,DustWork& work) noexcept {
    using namespace horde::gameplay::effects;
    if(quality==horde::graphics::DustQuality::Off || !horde::graphics::ValidDustQuality(quality))return;
    const unsigned perBurst=quality==horde::graphics::DustQuality::Low?
        EnemyHitParticles::kParticlesPerBurst/2:EnemyHitParticles::kParticlesPerBurst;
    unsigned active=0;
    for(const auto& b:hits.bursts)if(b.eventSequence && b.age>=0 && b.age<kHitBurstLifetime)++active;
    if(!active)return;
    const unsigned first=kDustMoteCapacity-active*perBurst;
    for(auto& tile:frame.tiles)for(auto& id:tile)if(id!=kDustEmptyCandidate && id>=first){id=kDustEmptyCandidate;--work.tileReferences;}
    work.combatReplacedMotes=work.generatedMotes>first?work.generatedMotes-first:0;
    static_assert(EnemyHitParticles::kBurstCapacity*EnemyHitParticles::kParticlesPerBurst<=kDustMoteCapacity);
    unsigned index=first;
    for(const auto& b:hits.bursts) {
        if(!b.eventSequence || !std::isfinite(b.age)||b.age<0||b.age>=kHitBurstLifetime)continue;
        for(unsigned n=0;n<perBurst;++n,++index) {
            const auto p=HitParticlePosition(b,n);
            if(!Finite(p))continue;
            const bool bone=b.target!=horde::gameplay::simulation::EntityId::Lich;
            const float radius=(bone?.018f:.024f)*(1.f-.25f*b.age/kHitBurstLifetime);
            auto& mote=frame.motes[index];
            mote.positionRadius={p[0],p[1],p[2],radius};
            mote.response={std::clamp((kHitBurstLifetime-b.age)/.10f,0.f,1.f),0,bone?1.f:2.f,0};
            ++work.combatMotes;
            DustProjection r;
            if(!ProjectDustSphere(p,radius,camera,r))continue;
            ++work.projectedMotes;
            const auto tx=[](float v){return unsigned(std::clamp(v,0.f,.999999f)*kDustTileColumns);};
            const auto ty=[](float v){return unsigned(std::clamp(v,0.f,.999999f)*kDustTileRows);};
            for(unsigned y=ty(r.top);y<=ty(r.bottom);++y)for(unsigned x=tx(r.left);x<=tx(r.right);++x) {
                auto& tile=frame.tiles[y*kDustTileColumns+x];
                auto free=std::find(tile.begin(),tile.end(),kDustEmptyCandidate);
                if(free!=tile.end()){*free=index;++work.tileReferences;}
                else { // Optional visual overflow is counted, never an invalid fifth entry.
                    ++work.overflowReferences;tile={index,tile[0],tile[1],tile[2]};
                }
            }
        }
    }
    work.generatedMotes=work.generatedMotes-work.combatReplacedMotes+work.combatMotes;
}
} // namespace horde::scene::atmosphere
