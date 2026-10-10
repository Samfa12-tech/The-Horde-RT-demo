#include "scene/RescueJourneyGeometry.h"
#include <iostream>
#include <iomanip>
using P=horde::scene::RescueRopePoint;
P Add(P a,P b) {return {a[0]+b[0],a[1]+b[1],a[2]+b[2]};}
P Scale(P a,float s) {return {a[0]*s,a[1]*s,a[2]*s};}
bool Occluded(const std::vector<P>& vertices,P p,P light) {
    using namespace horde::scene;
    const auto d=RopeSubtract(light,p); const float len=std::sqrt(RopeDot(d,d));
    const auto ray=Scale(d,1/len);
    for(std::size_t i=0;i<vertices.size();i+=3) {
        auto e1=RopeSubtract(vertices[i+1],vertices[i]),e2=RopeSubtract(vertices[i+2],vertices[i]);
        const auto h=RopeCross(ray,e2);const float det=RopeDot(e1,h);
        if(std::abs(det)<1e-7f) continue;
        const auto s=RopeSubtract(p,vertices[i]);const float u=RopeDot(s,h)/det;
        if(u<0 || u>1) continue;
        const auto q=RopeCross(s,e1);const float v=RopeDot(ray,q)/det;
        if(v<0 || u+v>1) continue;
        const float t=RopeDot(e2,q)/det;
        if(t>1e-6f && t<len-.02f) return true;
    }return false;
}
int main() {
    using namespace horde::gameplay::traversal;
    RescueTraversal rope;
    rope.NotifyLanternClaimed();
    rope.DeployOwned();
    Input input;
    input.playerPosition={-35.0f,kLowerSupportWorldY,-17.0f};
    input.contactPlaneWorldY=kLowerSupportWorldY;
    for(unsigned tick=0;tick<600;++tick) if(!rope.Step(input)) return 1;
    const auto& s=rope.Snapshot();
    const auto vertices=horde::scene::RescueRopeTriangleVertices(s);
    if(!s.ropeReady || vertices.empty()) return 2;
    std::cout<<std::setprecision(9)<<"{\"fixed_ticks\":600,\"ready\":true,\"vertices\":[";
    bool first=true;
    for(auto p:vertices) { if(!first) std::cout<<','; first=false;
        std::cout<<'['<<p[0]<<','<<p[1]<<','<<p[2]<<']'; }
    std::cout<<"],\"analytic_rope_only_visibility\":[";
    const P camera{-33.0f,.70f,-17.5f};
    const std::array<P,3> lights{{{-34.35f,2.76f,-16.02f},{-33.05f,2.76f,-14.38f},
                                {-33.4996758f,.0909200311f,-16.7447662f}}};
    for(unsigned source=0;source<lights.size();++source) {
        unsigned rays=0,midBlocked=0,disagreement=0,unblockedSamplesDarkened=0;
        P firstEndpoint{};unsigned firstClearCount=0;
        for(unsigned ix=0;ix<81;++ix) for(unsigned iz=0;iz<113;++iz) {
            const P end{-34.5f+ix*.025f,-.94f,-16.8f+iz*.025f};
            const P start=Add(camera,Scale(horde::scene::RopeSubtract(end,camera),(.70f-.20f)/(.70f+.94f)));
            const P delta=horde::scene::RopeSubtract(end,start);
            const P mid=Add(start,Scale(delta,.5f));const bool blocked=Occluded(vertices,mid,lights[source]);
            unsigned differing=0,clear=0;
            for(unsigned sample=0;sample<6;++sample) {
                const bool b=Occluded(vertices,Add(start,Scale(delta,(sample+.5f)/6)),lights[source]);
                differing+=b!=blocked;clear+=!b;
            }
            ++rays;midBlocked+=blocked;disagreement+=differing!=0;
            if(blocked) {unblockedSamplesDarkened+=clear;
                if(clear && !firstClearCount) {firstEndpoint=end;firstClearCount=clear;}}
        }
        if(source) std::cout<<',';
        std::cout<<"{\"source_index\":"<<source<<",\"ray_count\":"<<rays
                 <<",\"midpoint_blocked_rays\":"<<midBlocked<<",\"rays_with_sample_disagreement\":"<<disagreement
                 <<",\"clear_samples_classified_dark_by_midpoint\":"<<unblockedSamplesDarkened
                 <<",\"example_clear_samples_of_six\":"<<firstClearCount
                 <<",\"example_endpoint\":["<<firstEndpoint[0]<<','<<firstEndpoint[1]<<','<<firstEndpoint[2]<<"]}";
    }
    std::cout<<"]}\n";
}
