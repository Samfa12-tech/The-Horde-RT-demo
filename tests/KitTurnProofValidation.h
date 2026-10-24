#pragma once
#include <filesystem>

// Optional offline asset-proof entry. No game or production clip binding uses it.
inline int ValidateKitTurnProof(const std::filesystem::path& candidate,
                               const std::filesystem::path& references,
                               const std::filesystem::path& original) {
    using namespace horde::scene;
    SkinnedClipSet clips{};
    clips.clips[0]={"KitProof|turn_left",false,true};
    clips.clips[1]={"KitProof|turn_right",false,true};
    clips.clips[2]={"",false,false};clips.clips[3]={"",false,false};
    SkinnedMeshAsset model,idle;std::string error;
    if(!model.LoadClips(candidate.string(),clips,error)) {std::cerr<<error<<'\n';return 1;}
    clips.clips[0]={"Warden|idle",false,true};clips.clips[1]={"",false,false};
    if(!idle.LoadClips(original.string(),clips,error)) {std::cerr<<error<<'\n';return 1;}
    std::vector<TexturedSkinnedRtVertex> idleVertices,vertices;
    std::vector<SkinnedPbrTangent> tangents;
    if(!idle.SkinUniquePbr(SkinnedClip::Idle,0,idleVertices,tangents,error))return 1;
    float endpointMax=0,correctMax=0,doubleMax=0,missingMax=0;
    for(const auto slot:{SkinnedClip::Idle,SkinnedClip::Walking}) {
        if(std::abs(model.ClipDuration(slot)-.75f)>1e-6f)return 1;
        for(float seconds:{0.f,.75f}) {
            if(!model.SkinUniquePbr(slot,seconds,vertices,tangents,error)||vertices.size()!=idleVertices.size())return 1;
            for(std::size_t i=0;i<vertices.size();++i) {
                float squared=0;
                for(unsigned a=0;a<3;++a) {const float d=vertices[i].position[a]-idleVertices[i].position[a];squared+=d*d;}
                endpointMax=std::max(endpointMax,std::sqrt(squared));
            }
        }
    }
    std::ifstream input(references,std::ios::binary);
    auto read=[&](auto& value){return bool(input.read(reinterpret_cast<char*>(&value),sizeof(value)));};
    std::array<char,4> magic{};std::uint32_t cases=0;
    if(!read(magic)||magic!=std::array<char,4>{'K','T','P','1'}||!read(cases)||cases==0||cases>128)return 2;
    std::size_t compared=0;
    for(std::uint32_t sample=0;sample<cases;++sample) {
        std::uint32_t slot=0,count=0;float seconds=0,yaw=0;
        if(!read(slot)||!read(seconds)||!read(yaw)||!read(count)||slot>1||count==0||count>512||
           !std::isfinite(seconds)||seconds<0||seconds>.75f||!std::isfinite(yaw))return 2;
        if(!model.SkinUniquePbr(static_cast<SkinnedClip>(slot),seconds,vertices,tangents,error)) {std::cerr<<error<<'\n';return 1;}
        for(std::size_t i=0;i<vertices.size();++i) {
            float n=0,t=0,dot=0;
            for(unsigned a=0;a<3;++a) {n+=vertices[i].normal[a]*vertices[i].normal[a];t+=tangents[i].tangent[a]*tangents[i].tangent[a];dot+=vertices[i].normal[a]*tangents[i].tangent[a];}
            if(!std::isfinite(n)||!std::isfinite(t)||!std::isfinite(dot)||std::abs(n-1)>1e-4f||std::abs(t-1)>1e-4f||std::abs(dot)>1e-4f)return 1;
        }
        for(std::uint32_t reference=0;reference<count;++reference) {
            std::uint32_t index=0;std::array<float,3> expected{};
            if(!read(index)||!read(expected)||index>=vertices.size())return 2;
            const auto& p=vertices[index].position;
            auto difference=[&](float appliedYaw) {
                const float c=std::cos(appliedYaw),s=std::sin(appliedYaw);
                const std::array<float,3> value{c*p[0]+s*p[2],p[1],-s*p[0]+c*p[2]};float squared=0;
                for(unsigned a=0;a<3;++a) {if(!std::isfinite(expected[a]))return std::numeric_limits<float>::infinity();const float d=value[a]-expected[a];squared+=d*d;}
                return std::sqrt(squared);
            };
            const float applied=CompanionModelYaw(3.14159265359f-yaw)-CompanionModelYaw(3.14159265359f);
            correctMax=std::max(correctMax,difference(applied));doubleMax=std::max(doubleMax,difference(2*applied));missingMax=std::max(missingMax,difference(0));++compared;
        }
    }
    if(input.peek()!=std::char_traits<char>::eof())return 2;
    const bool pass=endpointMax<.0001f && correctMax<.0001f && doubleMax>.05f && missingMax>.05f;
    std::cout<<"{\"nativeProof\":"<<(pass?"true":"false")<<",\"cases\":"<<cases<<",\"comparedVertices\":"<<compared
        <<",\"endpointMaxM\":"<<endpointMax<<",\"singleYawMaxM\":"<<correctMax
        <<",\"doubleYawMaxM\":"<<doubleMax<<",\"missingYawMaxM\":"<<missingMax<<"}\n";
    return pass?0:1;
}
