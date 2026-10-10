#include "scene/assets/SkinnedMeshAsset.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>

int main() {
    using namespace horde::scene;
    constexpr std::array<const char*,7> clips{{"Warden|idle","Warden|walk","Warden|run",
        "Warden|jump","Warden|fall","Warden|slide","Warden|mantle"}};
    std::size_t poses=0;int failures=0;
    for(const auto name:clips) {
        SkinnedMeshAsset model;SkinnedClipSet bindings{};
        bindings.clips[0]={name,false,true};
        for(std::size_t i=1;i<bindings.clips.size();++i) bindings.clips[i]={"",false,false};
        std::string diagnostic;
        if(!model.LoadClips(HORDE_KIT_SOURCE_CANDIDATE,bindings,diagnostic)) {
            std::cerr<<"Native clip import rejected "<<name<<": "<<diagnostic<<'\n';++failures;continue;
        }
        const float duration=model.ClipDuration(SkinnedClip::Idle);
        if(!std::isfinite(duration)||duration<=0||!model.HasTexcoords()||!model.HasTangents()) {
            std::cerr<<"Missing finite clip duration/UV/tangent streams "<<name<<'\n';++failures;continue;
        }
        for(float fraction:{0.f,.25f,.5f,.75f,1.f}) {
            std::vector<TexturedSkinnedRtVertex> vertices;
            if(!model.SkinTextured(SkinnedClip::Idle,fraction*duration,vertices,diagnostic)||vertices.empty()) {
                std::cerr<<"Native skin failed "<<name<<": "<<diagnostic<<'\n';++failures;continue;
            }
            std::array<float,3> minimum{},maximum{};
            minimum.fill(std::numeric_limits<float>::infinity());
            maximum.fill(-std::numeric_limits<float>::infinity());
            bool finite=true;
            for(const auto& vertex:vertices) for(std::size_t axis=0;axis<3;++axis) {
                finite&=std::isfinite(vertex.position[axis])&&std::isfinite(vertex.normal[axis])&&
                    std::isfinite(vertex.texcoord[axis<2?axis:0]);
                minimum[axis]=std::min(minimum[axis],vertex.position[axis]);
                maximum[axis]=std::max(maximum[axis],vertex.position[axis]);
            }
            if(!finite||maximum[1]-minimum[1]<=.01f) ++failures;
            std::cout<<name<<" phase="<<fraction<<" duration="<<duration<<" vertices="<<vertices.size()
                <<" height="<<maximum[1]-minimum[1]<<" minY="<<minimum[1]<<" maxY="<<maximum[1]<<'\n';++poses;
        }
    }
    std::cout<<"Kit source intake clips=7 sampled_poses="<<poses<<" failures="<<failures
        <<" runtime_admission=NO\n";
    return failures?1:0;
}
