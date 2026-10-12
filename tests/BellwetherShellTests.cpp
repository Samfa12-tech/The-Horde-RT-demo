#include "scene/BellwetherShell.h"
#include "scene/assets/StaticPrimitiveGrouping.h"
#include <cstring>
#include <cmath>
#include <iostream>
int main() {
    using namespace horde::scene;
    bool ok=true;
    const auto check=[&](bool v,const char* m){if(!v){ok=false;std::cerr<<"FAIL: "<<m<<'\n';}};
    assets::StaticMeshAsset world;
    for(const auto* name:{"MedievalWall02","TombEarthenware"}) {
        assets::StaticMaterial m;m.name=name;world.materials.push_back(m);
    }
    world.nodeTransforms.push_back({"identity",{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}}});
    assets::StaticRtVertex v{};v.position[0]=-40;v.normal[1]=1;v.tangent[0]=1;v.tangent[3]=1;
    world.vertices.push_back(v);v.position[0]=-39;world.vertices.push_back(v);
    v.position[2]=1;world.vertices.push_back(v);world.indices={0,1,2};world.primitives.push_back({0,0,3,0,0});
    world.bounds={{{-40,0,0}},{{-39,0,1}}};const auto before=world;
    std::string error;const auto root=std::filesystem::path(HORDE_RT_SOURCE_DIR)/"assets";
    const auto result=AppendBellwetherShell(root,world,error);
    check(result,"real exported closed distant town must enter the native world owner");
    if(result) {
        check(world.indices.size()==before.indices.size()+966*3,"all 966 bounded silhouette/facade triangles are preserved");
        check(world.materials.size()==2,"no texture layer or material row expansion");
        check(std::memcmp(world.vertices.data(),before.vertices.data(),before.vertices.size()*sizeof(v))==0,
            "accepted geometry prefix remains byte-identical");
        check(world.bounds.maximum[0]>120 && world.bounds.maximum[2]>170,
            "town retains the map's southeast footprint");
        bool slopingNormal=false;
        for(std::size_t i=before.vertices.size();i<world.vertices.size();++i) {
            const auto& n=world.vertices[i].normal;
            slopingNormal|=std::abs(n[1])>.1f && std::abs(n[1])<.99f;
        }
        check(slopingNormal,"real pitched roof normals survive native import");
        // Cast through a closed B01 portal post at its actual planned facade.
        // v2 hits the flat wall at Z103.2; v3 must hit 240mm of solid relief
        // sooner. A label, UV, dark card or aggregate count cannot satisfy it.
        float closestZ=104.2f;
        for(std::size_t j=1;j<world.primitives.size();++j) {
            const auto& p=world.primitives[j];
            for(std::size_t k=0;k<p.indexCount;k+=3) {
                const auto& a=world.vertices[p.vertexOffset+world.indices[p.indexOffset+k]].position;
                const auto& b=world.vertices[p.vertexOffset+world.indices[p.indexOffset+k+1]].position;
                const auto& c=world.vertices[p.vertexOffset+world.indices[p.indexOffset+k+2]].position;
                const float determinant=(b[0]-a[0])*(c[1]-a[1])-(c[0]-a[0])*(b[1]-a[1]);
                if(std::abs(determinant)<1e-6f)continue;
                const float x=89.26f-a[0],y=-7.4845508f-a[1];
                const float u=(x*(c[1]-a[1])-(c[0]-a[0])*y)/determinant;
                const float v=((b[0]-a[0])*y-x*(b[1]-a[1]))/determinant;
                if(u<0 || v<0 || u+v>1)continue;
                const float z=a[2]+u*(b[2]-a[2])+v*(c[2]-a[2]);
                if(z>=102.f)closestZ=std::min(closestZ,z);
            }
        }
        check(std::abs(closestZ-102.96f)<.001f,
            "actual closed stone facade relief survives native import at metre scale");
        const auto admittedIndices=world.indices;
        check(!AppendBellwetherShell(root,world,error) && world.indices==admittedIndices,
            "repeated admission cannot duplicate the distant town or partially mutate its owner");
        for(std::size_t j=1;j<world.primitives.size();++j) {
            const auto& p=world.primitives[j];double volume=0;
            for(std::size_t k=0;k<p.indexCount;k+=3) {
                const auto& a=world.vertices[p.vertexOffset+world.indices[p.indexOffset+k]].position;
                const auto& b=world.vertices[p.vertexOffset+world.indices[p.indexOffset+k+1]].position;
                const auto& c=world.vertices[p.vertexOffset+world.indices[p.indexOffset+k+2]].position;
                volume+=(double(a[0])*(double(b[1])*c[2]-double(b[2])*c[1])+
                    double(a[1])*(double(b[2])*c[0]-double(b[0])*c[2])+
                    double(a[2])*(double(b[0])*c[1]-double(b[1])*c[0]))/6;
            }
            check(volume>1,"each closed town material family has outward native winding");
        }
        check(assets::ConsolidateWorldBakedMaterialFamilies(world,error) && world.primitives.size()==2,
            "town shares two existing world families after coalescing");
    }
    auto missing=before;missing.materials[1].name="Missing";
    check(!AppendBellwetherShell(root,missing,error) && missing.indices==before.indices,
        "missing admitted clay rejects without partial publication");
    auto absent=before;
    check(!AppendBellwetherShell(root/"missing",absent,error) && absent.indices==before.indices,
        "missing native source rejects without partial publication");
    std::cout<<"Bellwether native placement/material/rollback "<<(ok?"passed":"failed")<<".\n";
    return ok?0:1;
}
