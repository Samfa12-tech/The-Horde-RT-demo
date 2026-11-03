#pragma once
#include "gameplay/simulation/GameplayEvent.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace horde::gameplay::effects {
// Presentation proxies for accepted sword damage, not a new collision evaluator.
// No heap, event draining, wall clock or platform-specific authority.
using HitPoint = std::array<float,3>;
inline constexpr float kHitBurstLifetime = .42f;
struct EnemyHitBurst {
    std::uint64_t eventSequence = 0;
    simulation::EntityId target = simulation::EntityId::Invalid;
    HitPoint origin{}, direction{};
    float age = kHitBurstLifetime;
};
struct EnemyHitParticles {
    static constexpr unsigned kBurstCapacity=2, kParticlesPerBurst=8;
    std::array<EnemyHitBurst,kBurstCapacity> bursts{};
    std::uint64_t lastAcceptedSequence=0;
    void Clear() noexcept { bursts={}; }
    void Step(float seconds) noexcept {
        if(!std::isfinite(seconds)||seconds<0) return;
        for(auto& b:bursts) b.age=std::min(kHitBurstLifetime,b.age+seconds);
    }
    bool Active() const noexcept {
        for(const auto& b:bursts) if(b.eventSequence && b.age<kHitBurstLifetime)return true;
        return false;
    }
    bool Add(std::uint64_t sequence, simulation::EntityId target, HitPoint origin, HitPoint direction) noexcept {
        using simulation::EntityId;
        if(sequence<=lastAcceptedSequence || (target!=EntityId::SkeletonA && target!=EntityId::SkeletonB && target!=EntityId::Lich))return false;
        float length=0;
        for(unsigned a=0;a<3;++a){if(!std::isfinite(origin[a])||!std::isfinite(direction[a]))return false;length+=direction[a]*direction[a];}
        if(!std::isfinite(length)||length<1e-8f)return false;
        auto* slot=&bursts[0];
        for(auto& b:bursts) {
            if(!b.eventSequence || b.age>=kHitBurstLifetime){slot=&b;break;}
            if(b.age>slot->age)slot=&b;
        }
        for(float& v:direction)v/=std::sqrt(length);
        *slot={sequence,target,origin,direction,0};lastAcceptedSequence=sequence;return true;
    }
};
// Deterministic ballistic fan. Low keeps four stable samples per burst; Standard
// eight. Direction is frozen at accepted contact, never recomputed from camera.
inline HitPoint HitParticlePosition(const EnemyHitBurst& b,unsigned sample) noexcept {
    constexpr std::array<float,8> lateral{-1.f,-.35f,.35f,1.f,-.7f,.1f,.7f,-.1f};
    const float side=lateral[sample%8];
    const float age=b.age;
    HitPoint right{b.direction[2],0,-b.direction[0]};
    float length=std::hypot(right[0],right[2]);
    if(length<.001f)right={1,0,0};else for(float& v:right)v/=length;
    HitPoint p=b.origin;
    const float speed=.8f+.12f*float(sample%4);
    for(unsigned a=0;a<3;++a)p[a]+=(b.direction[a]*speed+right[a]*side*.65f)*age;
    p[1]+=(.6f+.13f*float(sample%3))*age-2.8f*age*age;
    return p;
}
} // namespace horde::gameplay::effects
