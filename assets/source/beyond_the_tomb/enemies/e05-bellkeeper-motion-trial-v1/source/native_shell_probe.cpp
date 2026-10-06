// Standalone host-only proof linked to the unchanged project skin reader.
// Witness file: bone_name unique_vertex_index inverse_bind_local_x y z
#include "scene/assets/SkinnedMeshAsset.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
struct Witness {std::string node; std::size_t vertex; std::array<float,3> local;};
static float distance(const std::array<float,3>& a,const std::array<float,3>& b) {
    float r=0;for(int i=0;i<3;++i)r+=(a[i]-b[i])*(a[i]-b[i]);return std::sqrt(r);
}
int main(int argc,char** argv) {
    if(argc<4){std::cerr<<"usage: native_shell_probe model.glb witness.txt clip [clip...]\n";return 2;}
    std::vector<Witness> witnesses;std::ifstream in(argv[2]);Witness w;
    while(in>>w.node>>w.vertex>>w.local[0]>>w.local[1]>>w.local[2])witnesses.push_back(w);
    if(witnesses.empty()){std::cerr<<"FAIL empty witness file\n";return 2;}
    std::cout<<std::setprecision(10);
    for(int arg=3;arg<argc;++arg){
        horde::scene::SkinnedClipSet bindings{};for(auto& b:bindings.clips)b={"",false,false};bindings.clips[0]={argv[arg],false,true};
        horde::scene::SkinnedMeshAsset asset;std::string diagnostic;
        if(!asset.LoadClips(argv[1],bindings,diagnostic)){std::cerr<<"FAIL Load "<<diagnostic<<'\n';return 1;}
        if(!asset.HasTexcoords()||!asset.HasTangents()){std::cerr<<"FAIL UV/tangent absent\n";return 1;}
        for(const auto& p:asset.PrimitiveRanges())std::cout<<"PRIMITIVE clip="<<argv[arg]<<" first="<<p.firstExpandedVertex<<" count="<<p.expandedVertexCount<<" material_name="<<std::quoted(p.materialName)<<'\n';
        const auto clip=horde::scene::SkinnedClip::Idle;const float duration=asset.ClipDuration(clip);
        const int samples=std::max(2,static_cast<int>(std::ceil(duration*60))+1);
        std::vector<horde::scene::TexturedSkinnedRtVertex> vertices;
        std::map<std::string,float> maxResidual,maxMovement,maxRigidError;
        std::vector<std::array<float,3>> start(witnesses.size());
        for(int frame=0;frame<samples;++frame){
            const float t=duration*frame/(samples-1);
            if(!asset.SkinUniqueTextured(clip,t,vertices,diagnostic)){std::cerr<<"FAIL Skin "<<diagnostic<<'\n';return 1;}
            for(const auto& v:vertices)for(int a=0;a<3;++a)if(!std::isfinite(v.position[a])||!std::isfinite(v.normal[a])){std::cerr<<"FAIL nonfinite vertex\n";return 1;}
            std::map<std::string,horde::scene::SkinnedNodeTransform> transforms;
            std::vector<std::array<float,3>> now;
            for(std::size_t i=0;i<witnesses.size();++i){
                const auto& p=witnesses[i];if(p.vertex>=vertices.size()){std::cerr<<"FAIL witness range\n";return 1;}
                if(!transforms.count(p.node)){
                    horde::scene::SkinnedNodeTransform transform;
                    if(!asset.NodeTransform(clip,t,p.node,transform,diagnostic)){std::cerr<<"FAIL Node "<<diagnostic<<'\n';return 1;}
                    for(float c:transform)if(!std::isfinite(c)){std::cerr<<"FAIL nonfinite transform\n";return 1;}
                    transforms[p.node]=transform;
                }
                const auto& m=transforms[p.node];std::array<float,3> actual,expected;
                for(int a=0;a<3;++a){actual[a]=vertices[p.vertex].position[a];expected[a]=m[a]*p.local[0]+m[4+a]*p.local[1]+m[8+a]*p.local[2]+m[12+a];}
                now.push_back(actual);if(frame==0)start[i]=actual;
                maxResidual[p.node]=std::max(maxResidual[p.node],distance(actual,expected));
                maxMovement[p.node]=std::max(maxMovement[p.node],distance(actual,start[i]));
                for(std::size_t k=0;k<i;++k)if(witnesses[k].node==p.node)maxRigidError[p.node]=std::max(maxRigidError[p.node],std::abs(distance(actual,now[k])-distance(start[i],start[k])));
            }
        }
        for(const auto& p:maxResidual){
            std::cout<<"WITNESS clip="<<argv[arg]<<" node="<<p.first<<" max_transform_residual_m="<<p.second<<" max_pair_length_change_m="<<maxRigidError[p.first]<<" max_movement_m="<<maxMovement[p.first]<<'\n';
            if(p.second>0.00002f||maxRigidError[p.first]>0.00002f){std::cerr<<"FAIL rigid witness tolerance 0.02mm\n";return 1;}
        }
        std::cout<<"PASS host multi-primitive geometry/rigid witnesses clip="<<argv[arg]<<" duration="<<duration<<" samples="<<samples<<" uniqueVertices="<<vertices.size()<<" primitives="<<asset.PrimitiveRanges().size()<<'\n';
    }
    std::cout<<"NOT PROVEN: PBR texture/factor interpretation, native RT rendering, GPU/device performance, gameplay exposure/hit/victory authority.\n";
}
