#include "scene/assets/SkinnedPbrActor.h"
#include "scene/CompanionPose.h"
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace {
std::uint64_t ThreadCpuNanoseconds() {
#ifdef _WIN32
    FILETIME created{},exited{},kernel{},user{};
    if(!GetThreadTimes(GetCurrentThread(),&created,&exited,&kernel,&user))return 0;
    const auto value=[](FILETIME t){return (std::uint64_t(t.dwHighDateTime)<<32)|t.dwLowDateTime;};
    return (value(kernel)+value(user))*100;
#else
    return 0;
#endif
}
std::uint64_t Hash(std::uint64_t value,const void* data,std::size_t bytes) {
    const auto* source=static_cast<const unsigned char*>(data);
    for(std::size_t i=0;i<bytes;++i){value^=source[i];value*=1099511628211ull;}
    return value;
}
void Array(const std::vector<std::uint64_t>& values) {
    std::cout<<'[';for(std::size_t i=0;i<values.size();++i){if(i)std::cout<<',';std::cout<<values[i];}std::cout<<']';
}
}
int main() {
    using namespace horde::scene;
    constexpr unsigned warmup=30,samples=240;
    const auto root=std::filesystem::path(HORDE_RT_SOURCE_DIR);
    SkinnedClipSet clips;
    clips.clips[0]={"Warden|idle",true,true};clips.clips[1]={"Warden|walk",true,true};
    clips.clips[2]={"",false,false};clips.clips[3]={"",false,false};
    assets::SkinnedPbrActor actor,unobserved;
    std::string error;
    const auto directory=root/"assets/models/characters/runtime/kit-warden-v1";
    for(auto* instance:{&actor,&unobserved})if(!instance->Load(directory,"kit-warden-lod0.runtime.glb",clips,error)) {
        std::cerr<<error;return 2;
    }
    for(unsigned scenario=0;scenario<3;++scenario) {
        std::vector<std::uint64_t> animation,vertices,copy,total,unobservedTotal;
        std::uint64_t digest=14695981039346656037ull,joints=0,vertexCount=0,copyBytes=0,copies=0;
        std::chrono::steady_clock::time_point batchStart;std::uint64_t cpuStart=0;
        for(unsigned tick=0;tick<warmup+samples;++tick) {
            if(tick==warmup){batchStart=std::chrono::steady_clock::now();cpuStart=ThreadCpuNanoseconds();}
            const float time=float(tick)/60;
            const SkinnedClipBlend blend{SkinnedClip::Walking,time,scenario==0?0.f:scenario==1?1.f:.5f};
            horde::gameplay::dialogue::CompanionActingPose acting;
            if(scenario==2){acting.headPitch=.03f*std::sin(time*5);acting.headYaw=.025f*std::sin(time*2.3f);acting.chestPitch=.012f*std::sin(time*3.1f);}
            const auto rotations=CompanionUpperBodyRotations(acting);
            SkinWorkMetrics work;
            const auto begin=std::chrono::steady_clock::now();
            if(!actor.Pose(SkinnedClip::Idle,time,error,rotations,&blend,&work)){std::cerr<<error;return 2;}
            const auto elapsed=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-begin).count();
            // Same production asset path with clocks disabled must retain every
            // output byte. This verification is outside the pose-only timer.
            const auto unobservedBegin=std::chrono::steady_clock::now();
            const bool unobservedOk=unobserved.Pose(SkinnedClip::Idle,time,error,rotations,&blend);
            const auto unobservedElapsed=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-unobservedBegin).count();
            if(!unobservedOk ||
               actor.Asset().vertices.size()!=unobserved.Asset().vertices.size() ||
               std::memcmp(actor.Asset().vertices.data(),unobserved.Asset().vertices.data(),
                   actor.Asset().vertices.size()*sizeof(actor.Asset().vertices.front()))!=0) {
                std::cerr<<"observed/unobserved pose differs: "<<error;return 1;
            }
            if(tick<warmup)continue;
            animation.push_back(work.animationNanoseconds);vertices.push_back(work.vertexNanoseconds);
            copy.push_back(work.copyNanoseconds);total.push_back(elapsed);
            unobservedTotal.push_back(unobservedElapsed);
            joints=work.joints;vertexCount=work.vertices;copyBytes=work.copyBytes;copies=work.copies;
            digest=Hash(digest,actor.Asset().vertices.data(),actor.Asset().vertices.size()*sizeof(actor.Asset().vertices.front()));
            digest=Hash(digest,&actor.Asset().bounds,sizeof(actor.Asset().bounds));
        }
        const auto cpuEnd=ThreadCpuNanoseconds();
        const auto wall=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-batchStart).count();
        std::cout<<"{\"scope\":\"CPU asset pose only; batch thread CPU includes duplicate verification and digest; no GPU/upload/AS/wait\",\"scenario\":\""
            <<(scenario==0?"idle":scenario==1?"walk":"half-blend-acting")<<"\",\"warmup\":"<<warmup<<",\"samples\":"<<samples
            <<",\"verticesPerPose\":"<<vertexCount<<",\"jointsPerPose\":"<<joints<<",\"copyOutputBytesPerPose\":"<<copyBytes
            <<",\"copyPassesPerPose\":"<<copies<<",\"batchWallNs\":"<<wall<<",\"batchThreadCpuNs\":"<<(cpuStart&&cpuEnd>=cpuStart?cpuEnd-cpuStart:0)
            <<",\"outputFnv1a64\":\""<<std::hex<<digest<<std::dec<<"\",\"animationNs\":";
        Array(animation);std::cout<<",\"vertexNs\":";Array(vertices);std::cout<<",\"copyNs\":";Array(copy);std::cout<<",\"poseNs\":";Array(total);
        std::cout<<",\"unobservedPoseNs\":";Array(unobservedTotal);std::cout<<"}\n";
    }
    return 0;
}
