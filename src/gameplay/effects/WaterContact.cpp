#include "gameplay/effects/WaterContact.h"
namespace horde::gameplay::effects
{
WaterContactResult WaterContact::Step(float dt,float x,float y,float z,bool grounded,bool step,float speed,float width,bool paused)
{
    WaterContactResult result{};
    if(paused || !std::isfinite(dt) || dt<=0) return result;
    dt=std::min(dt,.05f);
    for(auto& r:state_.ripples) if(r.active) { r.age+=dt;if(r.age>=.90f) r.active=false; }
    for(auto& d:state_.droplets) if(d.active) {
        d.age+=dt;
        d.velocity[1]-=9.81f*dt;
        for(std::size_t i=0;i<3;++i) d.position[i]+=d.velocity[i]*dt;
        float surface=kRouteFloorWorldY;
        const bool water=WaterSurfaceAt(d.position[0],d.position[2],surface);
        bool inside=false;
        for(const auto& rect:kShowcaseWalkableRects) inside|=Contains(rect,d.position[0],d.position[2]);
        bool masonry=false;
        for(const auto& rect:kShowcaseMasonryObstacles)
            masonry|=Contains(rect,d.position[0],d.position[2]);
        if(d.position[1]<=surface) {
            if(water) Ripple(d.position[0],d.position[2],.25f);
            d.active=false;
        }
        if(d.age>=d.lifetime || !inside || masonry) d.active=false;
    }
    float surface=0;
    state_.wetGround=grounded && WaterSurfaceAt(x,z,surface) && std::abs(surface-y)<.06f;
    const bool stream=BodyTouchesWaterfall(x,y,z,width);
    result.streamEntry=stream&&!state_.inStream;
    state_.inStream=stream;
    if(result.streamEntry) {
        ++state_.entryCount;EmitDrops(x,y+.9f,z,8,1.f);sprayClock_=0;
    }
    if(stream) {
        sprayClock_+=dt;
        if(sprayClock_>=.20f) { sprayClock_-=.20f;++state_.sprayCount;EmitDrops(x,y+.85f,z,2,.7f); }
    } else sprayClock_=0;
    if(state_.wetGround && step && std::isfinite(speed) && speed>.01f) {
        result.wetStep=true;
        result.intensity=std::clamp(.35f+.08f*speed,.35f,.70f);
        ++state_.stepCount;
        Ripple(x,z,result.intensity);
        EmitDrops(x,surface+.04f,z,3,result.intensity);
    }
    return result;
}
void WaterContact::EmitDrops(float x,float y,float z,std::size_t count,float speed)
{
    for(std::size_t i=0;i<count;++i) {
        auto it=std::find_if(state_.droplets.begin(),state_.droplets.end(),[](const auto& d){return !d.active;});
        if(it==state_.droplets.end()) { state_.droppedEmissions+=count-i;break; }
        serial_=serial_*1664525u+1013904223u;
        const float angle=static_cast<float>(serial_&65535u)*(6.2831853f/65536.f);
        const float fraction=static_cast<float>((serial_>>16)&255u)/255.f;
        *it={{x+.015f*std::cos(angle),y,z+.015f*std::sin(angle)},
             {.65f*speed*std::cos(angle),.9f+.7f*fraction,.65f*speed*std::sin(angle)},
             0,.45f+.20f*fraction,true};
    }
}
void WaterContact::Ripple(float x,float z,float strength)
{
    float y=0;if(!WaterSurfaceAt(x,z,y)) return;
    auto it=std::find_if(state_.ripples.begin(),state_.ripples.end(),[](const auto& r){return !r.active;});
    if(it==state_.ripples.end()) it=std::max_element(state_.ripples.begin(),state_.ripples.end(),
        [](const auto& a,const auto& b){return a.age<b.age;});
    *it={{x,y,z},0,strength,true};
}
}
