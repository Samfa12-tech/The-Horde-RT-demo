#pragma once
#include "graphics/DustQuality.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <span>

namespace horde::scene::atmosphere {
using Vec3 = std::array<float, 3>;
inline constexpr std::size_t kDustZoneCapacity = 4;
inline constexpr std::size_t kDustAdmittedZones = 2;
inline constexpr std::size_t kDustMoteCapacity = 64;
inline constexpr unsigned kDustTileColumns = 32, kDustTileRows = 18;
inline constexpr unsigned kDustCandidatesPerTile = 4;
inline constexpr unsigned kDustEmptyCandidate = ~0u;
enum class DustZoneShape : std::uint8_t { Box, Ellipsoid };
struct IndoorDustZone {
    std::uint32_t id = 0, seed = 0;
    Vec3 minimum{}, maximum{}, drift{}; // metres and metres/second
    float density = 0.12f, radius = 0.010f, distance = 8.0f;
    DustZoneShape shape = DustZoneShape::Box;
    bool operator==(const IndoorDustZone&) const = default;
};
inline float Dot(Vec3 a, Vec3 b) noexcept { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
inline Vec3 Sub(Vec3 a, Vec3 b) noexcept { return {a[0]-b[0],a[1]-b[1],a[2]-b[2]}; }
inline bool Finite(Vec3 a) noexcept { return std::isfinite(a[0])&&std::isfinite(a[1])&&std::isfinite(a[2]); }
inline bool ValidDustZone(const IndoorDustZone& z) noexcept {
    if (!z.id || !Finite(z.minimum)||!Finite(z.maximum)||!Finite(z.drift) ||
        !std::isfinite(z.density)||z.density<0||z.density>1 ||
        !std::isfinite(z.radius)||z.radius<0.002f||z.radius>0.016f ||
        !std::isfinite(z.distance)||z.distance<1||z.distance>16 ||
        static_cast<unsigned>(z.shape)>1) return false;
    for(unsigned i=0;i<3;++i)
        if(z.maximum[i]-z.minimum[i]<z.radius*8 || std::abs(z.drift[i])>0.12f) return false;
    return true;
}
inline bool ValidateDustZones(std::span<const IndoorDustZone> zones) noexcept {
    if(zones.size()>kDustZoneCapacity) return false;
    for(std::size_t i=0;i<zones.size();++i) {
        if(!ValidDustZone(zones[i])) return false;
        for(std::size_t j=0;j<i;++j) if(zones[j].id==zones[i].id) return false;
    }
    return true;
}
inline std::uint32_t DustHash(std::uint32_t x) noexcept {
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; return x^(x>>16);
}
inline float DustUnit(std::uint32_t x) noexcept { return float(DustHash(x)>>8)* (1.0f/16777216.0f); }
// Absolute simulation time, never wall time or accumulated render deltas.
// Smooth bounded oscillation avoids recycling, portal wrap and camera swimming.
inline Vec3 DustPosition(const IndoorDustZone& z, unsigned index, double seconds) noexcept {
    Vec3 q{}, result{};
    for(unsigned a=0;a<3;++a) {
        const float phase=6.28318530718f*DustUnit(z.seed+index*17u+a*977u);
        const double omega=double(z.drift[a])/double((z.maximum[a]-z.minimum[a])*0.42f);
        q[a]=0.84f*float(std::sin(double(phase)+omega*seconds));
    }
    // Ellipsoid occupies a conservative inset of its authored bounds.
    if(z.shape==DustZoneShape::Ellipsoid) {
        const float length=std::sqrt(Dot(q,q));
        if(length>0.84f) for(float& v:q) v*=0.84f/length;
    }
    for(unsigned a=0;a<3;++a)
        result[a]=(z.minimum[a]+z.maximum[a])*0.5f+q[a]*(z.maximum[a]-z.minimum[a])*0.5f;
    return result;
}
struct DustCamera {
    Vec3 origin{};
    float yaw=0, pitch=0, aspect=1;
    bool operator==(const DustCamera&) const = default;
};
struct alignas(16) DustMote {
    std::array<float,4> positionRadius{};
    std::array<float,4> response{}; // opacity, stable zone ID, reserved, reserved
};
struct alignas(16) DustFrame {
    std::array<DustMote,kDustMoteCapacity> motes{};
    std::array<std::array<std::uint32_t,4>,kDustTileColumns*kDustTileRows> tiles{};
};
struct DustWork {
    unsigned admittedZones=0, generatedMotes=0, projectedMotes=0, tileReferences=0, overflowReferences=0;
};
struct DustProjection { float left=0,right=0,top=0,bottom=0; };
inline bool ProjectDustSphere(Vec3 p,float radius,const DustCamera& c,DustProjection& r) noexcept {
    // Match rt_frame's actual pitched camera and upright UV projection.
    Vec3 forward{std::sin(c.yaw),-0.05f+std::clamp(c.pitch,-0.32f,0.28f),-std::cos(c.yaw)};
    float length=std::sqrt(Dot(forward,forward)); for(float& v:forward)v/=length;
    Vec3 right{-forward[2],0,forward[0]};
    length=std::sqrt(Dot(right,right)); for(float& v:right)v/=length;
    Vec3 up{-forward[1]*right[2],forward[0]*right[2]-forward[2]*right[0],forward[1]*right[0]};
    const auto v=Sub(p,c.origin); const float z=Dot(v,forward);
    if(z-radius<=0.20f) return false;
    const float x=Dot(v,right), y=Dot(v,up);
    auto interval=[&](float n,float scale) {
        std::array<float,4> values{(n-radius)/(z-radius),(n-radius)/(z+radius),
            (n+radius)/(z-radius),(n+radius)/(z+radius)};
        auto [lo,hi]=std::minmax_element(values.begin(),values.end());
        return std::array<float,2>{0.5f+*lo*scale,0.5f+*hi*scale};
    };
    const auto horizontal=interval(x,0.61f/c.aspect);
    const auto vertical=interval(y,0.61f/0.74f);
    r={horizontal[0],horizontal[1],1-vertical[1],1-vertical[0]};
    return r.right>=0 && r.left<=1 && r.bottom>=0 && r.top<=1;
}
inline bool BuildDustFrame(std::span<const IndoorDustZone> zones,horde::graphics::DustQuality quality,
                          const DustCamera& c,double seconds,DustFrame& output,DustWork& work) noexcept {
    work={};
    // Off does not traverse zones, update motes, touch output, or project tiles.
    if(quality==horde::graphics::DustQuality::Off) return true;
    if(!horde::graphics::ValidDustQuality(quality)||!ValidateDustZones(zones)||!Finite(c.origin)||
        !std::isfinite(c.yaw)||!std::isfinite(c.pitch)||!std::isfinite(c.aspect)||c.aspect<=0||
        !std::isfinite(seconds)||seconds<0) return false;
    output={}; for(auto& tile:output.tiles) tile.fill(kDustEmptyCandidate);
    std::array<const IndoorDustZone*,kDustZoneCapacity> ordered{};
    std::size_t count=0;
    for(const auto& zone:zones) ordered[count++]=&zone;
    // Stable IDs decide overlap admission and overflow; camera movement never reseeds.
    std::sort(ordered.begin(),ordered.begin()+count,[](auto a,auto b){return a->id<b->id;});
    const unsigned perZone=quality==horde::graphics::DustQuality::Low ? 16u : 32u;
    for(std::size_t zi=0;zi<count && work.admittedZones<kDustAdmittedZones;++zi) {
        const auto& zone=*ordered[zi]; float distanceSquared=0;
        for(unsigned a=0;a<3;++a) {
            const float gap=std::max({zone.minimum[a]-c.origin[a],c.origin[a]-zone.maximum[a],0.0f});
            distanceSquared+=gap*gap;
        }
        if(distanceSquared>zone.distance*zone.distance || zone.density==0) continue;
        ++work.admittedZones;
        for(unsigned n=0;n<perZone;++n) {
            const auto p=DustPosition(zone,n,seconds); const unsigned index=work.generatedMotes++;
            auto& mote=output.motes[index]; mote.positionRadius={p[0],p[1],p[2],zone.radius};
            const float dist=std::sqrt(Dot(Sub(p,c.origin),Sub(p,c.origin)));
            const float fade=std::clamp((dist-0.25f)/0.3f,0.0f,1.0f)*
                std::clamp((zone.distance-dist)/1.0f,0.0f,1.0f);
            mote.response={zone.density*fade,float(zone.id),0,0};
            DustProjection r; if(!fade||!ProjectDustSphere(p,zone.radius,c,r))continue;
            ++work.projectedMotes;
            const auto tileX=[](float x){return unsigned(std::clamp(x,0.0f,0.999999f)*kDustTileColumns);};
            const auto tileY=[](float y){return unsigned(std::clamp(y,0.0f,0.999999f)*kDustTileRows);};
            for(unsigned y=tileY(r.top);y<=tileY(r.bottom);++y)
                for(unsigned x=tileX(r.left);x<=tileX(r.right);++x) {
                    auto& tile=output.tiles[y*kDustTileColumns+x];
                    auto free=std::find(tile.begin(),tile.end(),kDustEmptyCandidate);
                    if(free==tile.end()){++work.overflowReferences;continue;}
                    *free=index; ++work.tileReferences;
                }
        }
    }
    return true;
}
enum class DustUploadDecision { Disabled, Unchanged, Changed, Invalid };
class DustFrameCache {
public:
    void Invalidate() noexcept { valid_=false; }
    DustUploadDecision Build(std::span<const IndoorDustZone> zones,horde::graphics::DustQuality quality,
        const DustCamera& camera,double seconds,DustFrame& frame,DustWork& work) noexcept {
        work={};
        if(quality==horde::graphics::DustQuality::Off){Invalidate();return DustUploadDecision::Disabled;}
        if(valid_ && quality==quality_ && camera==camera_ && seconds==seconds_ && zones.size()==count_ &&
            std::equal(zones.begin(),zones.end(),zones_.begin())) return DustUploadDecision::Unchanged;
        return BuildDustFrame(zones,quality,camera,seconds,frame,work) ? DustUploadDecision::Changed : DustUploadDecision::Invalid;
    }
    // Commit only after successful owning buffer write; failed writes never seed reuse.
    void Commit(std::span<const IndoorDustZone> zones,horde::graphics::DustQuality quality,
        const DustCamera& camera,double seconds) noexcept {
        if(zones.size()>zones_.size()){Invalidate();return;}
        std::copy(zones.begin(),zones.end(),zones_.begin());count_=zones.size();
        quality_=quality;camera_=camera;seconds_=seconds;valid_=true;
    }
private:
    bool valid_=false;
    std::array<IndoorDustZone,kDustZoneCapacity> zones_{};
    std::size_t count_=0;
    horde::graphics::DustQuality quality_=horde::graphics::DustQuality::Off;
    DustCamera camera_{};
    double seconds_=0;
};
static_assert(sizeof(DustMote)==32 && sizeof(DustFrame)==11264);
}
