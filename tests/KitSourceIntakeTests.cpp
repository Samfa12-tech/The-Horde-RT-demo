#include "scene/assets/SkinnedMeshAsset.h"
#include "scene/assets/StaticMeshAsset.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <cstring>

namespace {
bool CheckAnalyticTangentFrames() {
    using namespace horde::scene;
    SkinnedClipSet clips{};
    clips.clips[0]={"rotate",false,true};
    clips.clips[1]={"reflect",false,true};
    for(std::size_t i=2;i<clips.clips.size();++i) clips.clips[i]={"",false,false};
    SkinnedMeshAsset model;std::string diagnostic;
    if(!model.LoadClips(HORDE_SKINNED_PBR_FIXTURE,clips,diagnostic)) {
        std::cerr<<"Analytic skin fixture import failed: "<<diagnostic<<'\n';return false;
    }
    std::vector<TexturedSkinnedRtVertex> vertices;
    std::vector<SkinnedPbrTangent> tangents;
    for(const auto clip:{SkinnedClip::Idle,SkinnedClip::Walking}) {
        if(!model.SkinUniquePbr(clip,1,vertices,tangents,diagnostic)||vertices.size()!=3||tangents.size()!=3) return false;
        const bool reflection=clip==SkinnedClip::Walking;
        const std::array<float,4> expected=reflection?std::array<float,4>{-1,0,0,-1}:std::array<float,4>{0,1,0,1};
        for(const auto& frame:tangents) for(std::size_t i=0;i<4;++i)
            if(std::abs(frame.tangent[i]-expected[i])>1e-5f) {
                std::cerr<<"Analytic rotated/reflected tangent mismatch: reflection="<<reflection<<" component="<<i<<'\n';return false;
            }
    }
    // The reflected clip crosses a singular scale: reject it without partial output.
    if(model.SkinUniquePbr(SkinnedClip::Walking,.5f,vertices,tangents,diagnostic)||!vertices.empty()||!tangents.empty()) return false;
    return true;
}
}

int main() {
    using namespace horde::scene;
    constexpr std::array<const char*,7> clips{{"Warden|idle","Warden|walk","Warden|run",
        "Warden|jump","Warden|fall","Warden|slide","Warden|mantle"}};
    std::size_t poses=0;int failures=CheckAnalyticTangentFrames()?0:1;
    const std::filesystem::path runtimeDirectory=HORDE_KIT_RUNTIME_DIRECTORY;
    const auto runtimePath=runtimeDirectory/"kit-warden-lod0.runtime.glb";
    assets::AssetManifest manifest;assets::StaticMeshAsset staticAsset;std::string admission;
    if(!assets::AssetManifest::Load(runtimeDirectory/"asset.manifest.json",manifest,admission)||
       !assets::StaticMeshAsset::Load(runtimePath,manifest,staticAsset,admission)||
       staticAsset.vertices.size()!=26945u||staticAsset.indices.size()!=77904u||
       staticAsset.materials.size()!=1u) {
        std::cerr<<"Native Kit PBR admission failed: "<<admission<<'\n';return 1;
    }
    const auto& material=staticAsset.materials[0];
    if(material.baseColorTexture<0||material.normalTexture<0||material.ormTexture<0||
       material.emissiveTexture<0||std::abs(material.normalScale-.7f)>1e-5f||
       std::abs(material.emissiveFactor[0]-.55f)>1e-5f) {
        std::cerr<<"Kit's four authored PBR maps/factors must survive native import\n";return 1;
    }
    for(const auto name:clips) {
        SkinnedMeshAsset model;SkinnedClipSet bindings{};
        bindings.clips[0]={name,false,true};
        for(std::size_t i=1;i<bindings.clips.size();++i) bindings.clips[i]={"",false,false};
        std::string diagnostic;
        if(!model.LoadClips(HORDE_KIT_SOURCE_CANDIDATE,bindings,diagnostic)) {
            std::cerr<<"Native clip import rejected "<<name<<": "<<diagnostic<<'\n';++failures;continue;
        }
        SkinnedMeshAsset runtimeModel;
        if(!runtimeModel.LoadClips(runtimePath.string(),bindings,diagnostic)||
           !runtimeModel.ValidateStaticVertexLayout(staticAsset,diagnostic)) {
            std::cerr<<"Runtime Kit skin/static layout rejected: "<<diagnostic<<'\n';return 1;
        }
        const float duration=model.ClipDuration(SkinnedClip::Idle);
        if(!std::isfinite(duration)||duration<=0||!model.HasTexcoords()||!model.HasTangents()) {
            std::cerr<<"Missing finite clip duration/UV/tangent streams "<<name<<'\n';++failures;continue;
        }
        for(float fraction:{0.f,.25f,.5f,.75f,1.f}) {
            std::vector<TexturedSkinnedRtVertex> unique,reference;
            std::vector<SkinnedPbrTangent> tangents;
            if (!model.SkinUniqueTextured(SkinnedClip::Idle,fraction*duration,reference,diagnostic) ||
                !model.SkinUniquePbr(SkinnedClip::Idle,fraction*duration,unique,tangents,diagnostic) ||
                unique.size()!=reference.size() || tangents.size()!=unique.size() ||
                std::memcmp(unique.data(),reference.data(),unique.size()*sizeof(unique[0]))!=0) {
                std::cerr<<"PBR skin must preserve native unique positions/normals/UVs and supply tangents "<<name<<'\n';
                ++failures;
            } else {
                for(std::size_t v=0;v<unique.size();++v) {
                    const auto& t=tangents[v].tangent;const auto& n=unique[v].normal;
                    const float length=std::sqrt(t[0]*t[0]+t[1]*t[1]+t[2]*t[2]);
                    const float dot=t[0]*n[0]+t[1]*n[1]+t[2]*n[2];
                    if(!std::isfinite(length)||std::abs(length-1)>1e-4f||
                       !std::isfinite(dot)||std::abs(dot)>1e-4f||std::abs(t[3])!=1) {
                        std::cerr<<"PBR skin tangent frame is invalid "<<name<<'\n';++failures;break;
                    }
                }
                std::vector<TexturedSkinnedRtVertex> repeated;
                std::vector<SkinnedPbrTangent> repeatedTangents;
                if(!model.SkinUniquePbr(SkinnedClip::Idle,fraction*duration,repeated,repeatedTangents,diagnostic)||
                   repeated.size()!=unique.size()||repeatedTangents.size()!=tangents.size()||
                   std::memcmp(repeated.data(),unique.data(),unique.size()*sizeof(unique[0]))||
                   std::memcmp(repeatedTangents.data(),tangents.data(),tangents.size()*sizeof(tangents[0]))) ++failures;
                if(!runtimeModel.SkinUniquePbr(SkinnedClip::Idle,fraction*duration,repeated,repeatedTangents,diagnostic)||
                   repeated.size()!=unique.size()||repeatedTangents.size()!=tangents.size()||
                   std::memcmp(repeated.data(),unique.data(),unique.size()*sizeof(unique[0]))||
                   std::memcmp(repeatedTangents.data(),tangents.data(),tangents.size()*sizeof(tangents[0]))) {
                    std::cerr<<"Runtime derivative changed a native source skin pose/tangent\n";++failures;
                }
            }
            std::vector<TexturedSkinnedRtVertex> vertices;
            if(!model.SkinTextured(SkinnedClip::Idle,fraction*duration,vertices,diagnostic)||vertices.empty()) {
                std::cerr<<"Native skin failed "<<name<<": "<<diagnostic<<'\n';++failures;continue;
            }
            if(vertices.size()!=staticAsset.indices.size()) ++failures;
            else for(std::size_t v=0;v<vertices.size();++v) {
                const auto index=staticAsset.indices[v];
                if(index>=unique.size() || std::memcmp(&vertices[v],&unique[index],sizeof(vertices[v]))) {
                    std::cerr<<"Expanded skin differs from indexed PBR skin\n";++failures;break;
                }
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
        std::vector<TexturedSkinnedRtVertex> rejected(1);
        std::vector<SkinnedPbrTangent> rejectedTangents(1);
        for(float badTime:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()})
            if(model.SkinUniquePbr(SkinnedClip::Idle,badTime,rejected,rejectedTangents,diagnostic)||
               !rejected.empty()||!rejectedTangents.empty()) ++failures;
        if(model.SkinUniquePbr(static_cast<SkinnedClip>(255),0,rejected,rejectedTangents,diagnostic)||
           !rejected.empty()||!rejectedTangents.empty()) ++failures;
    }
    std::cout<<"Kit source intake clips=7 sampled_poses="<<poses<<" failures="<<failures
        <<" runtime_admission=NO\n";
    return failures?1:0;
}
