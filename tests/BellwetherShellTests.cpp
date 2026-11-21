#include "scene/BellwetherShell.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/assets/AssetManifest.h"
#include "scene/atmosphere/ChimneySmoke.h"
#include "scene/assets/StaticPrimitiveGrouping.h"
#include "../assets/models/world/source/bellwether-tavern-v1/staging.generated.h"
#include <cstring>
#include <cmath>
#include <iostream>
int main() {
    using namespace horde::scene;
    bool ok=true;
    const auto check=[&](bool v,const char* m){if(!v){ok=false;std::cerr<<"FAIL: "<<m<<'\n';}};
    assets::StaticMeshAsset world;
    for(const auto* name:{"MedievalWall02","TombEarthenware","TombCharredWick"}) {
        assets::StaticMaterial m;m.name=name;world.materials.push_back(m);
    }
    world.nodeTransforms.push_back({"identity",{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}}});
    assets::StaticRtVertex v{};v.position[0]=-40;v.normal[1]=1;v.tangent[0]=1;v.tangent[3]=1;
    world.vertices.push_back(v);v.position[0]=-39;world.vertices.push_back(v);
    v.position[2]=1;world.vertices.push_back(v);world.indices={0,1,2};world.primitives.push_back({0,0,3,0,0});
    world.bounds={{{-40,0,0}},{{-39,0,1}}};const auto before=world;
    std::string error;const auto root=std::filesystem::path(HORDE_RT_SOURCE_DIR)/"assets";
    const auto terrainRoot=root/"models/world/runtime/chapter-terrain-v1";
    assets::AssetManifest terrainManifest;
    assets::StaticMeshAsset terrainMesh;
    if(!assets::AssetManifest::Load(terrainRoot/"asset.manifest.json",terrainManifest,error) ||
       !assets::StaticMeshAsset::Load(terrainRoot/"chapter-terrain-lod0.runtime.glb",terrainManifest,terrainMesh,error)) {
        std::cerr<<error<<'\n';return 1;
    }
    const auto originalTerrain=PrepareChapterTerrainSurface(terrainMesh,error);
    auto support=originalTerrain;
    assets::StaticMeshAsset glazing;
    const auto result=AppendBellwetherShell(root,world,support,error,&glazing);
    if(!result)std::cerr<<error<<'\n';
    check(result,"real exported town and its tavern interior enter the native world owner");
    if(result) {
        check(world.indices.size()==before.indices.size()+(kBellwetherSolidTriangles+kBellwetherGroundTriangles)*3,
            "the complete bounded town and tavern geometry is rendered");
        check(world.materials.size()==before.materials.size(),"no texture layer or material row expansion");
        check(support->NonWalkableTriangleCount()==kBellwetherSolidTriangles+kBellwetherGlazingTriangles &&
              support->TriangleCount()==originalTerrain->TriangleCount()+kBellwetherGroundTriangles,
            "rendered masonry stays solid while only the ramp and floor extend ground support");
        constexpr float southDoorX=82.3f,southWallZ=119.2f;
        const auto bare=originalTerrain->Query(southDoorX,southWallZ);
        const auto after=support->Query(southDoorX,southWallZ);
        check(glazing.primitives.size()==1 && glazing.materials.size()==1 &&
              (glazing.materials[0].flags & assets::kStaticMaterialCertifiedClosedVolume) &&
              glazing.indices.size()==kBellwetherGlazingTriangles*3 &&
              !support->NonWalkableVolumeClear(86.88f,118.7f,86.88f,119.3f,
                  after.worldY+2.03f,after.worldY+2.05f,.01f),
            "the separately rendered dielectric is a closed volume with contact across the clear pane");
        check(bare.found && after.found && after.walkable && after.triangle>=originalTerrain->TriangleCount() &&
              std::abs(after.worldY-(-8.70955f))<.001f && after.worldY-bare.worldY>2.5f,
            "real ramp and floor meet at the original tavern floor height above current ground");
        check(support->NonWalkableVolumeClear(southDoorX,121.f,southDoorX,118.8f,after.worldY+.01f,after.worldY+1.8f,.24f) &&
             !support->NonWalkableVolumeClear(southDoorX+1.5f,121.f,southDoorX+1.5f,118.8f,
                 after.worldY+.01f,after.worldY+1.8f,.24f),
             "the mapped south opening admits the capsule while the adjacent wall stays solid");
        using namespace horde::gameplay::bellwether;
        const auto exactHorizontalFace=[&](const auto& mark) {
            constexpr float epsilon=.001f;
            return !support->NonWalkableVolumeClear(mark[0],mark[2],mark[0],mark[2],
                mark[1]-epsilon,mark[1]+epsilon,.025f) &&
                support->NonWalkableVolumeClear(mark[0],mark[2],mark[0],mark[2],
                mark[1]+epsilon,mark[1]+2*epsilon,.025f);
        };
        check(exactHorizontalFace(kChairSeat) && exactHorizontalFace(kTableTop) &&
              std::abs(support->Query(kChairSeat[0],kChairSeat[2]).worldY-after.worldY)<.001f,
            "the authored contact marks match the seat and table surfaces above the shared floor");
        check(support->NonWalkableVolumeClear(kChairTurn[0],kChairTurn[2],kChairApproach[0],kChairApproach[2],
                  after.worldY+.001f,after.worldY+1.8f,.24f) &&
              !support->NonWalkableVolumeClear(kChairApproach[0],kChairApproach[2],kChairSeat[0],kChairSeat[2],
                  after.worldY+.001f,after.worldY+1.8f,.24f),
            "the real chair leaves a clear approach and blocks the standing player from walking through the seat");
        check(std::memcmp(world.vertices.data(),before.vertices.data(),before.vertices.size()*sizeof(v))==0,
            "accepted geometry prefix remains byte-identical");
        check(world.bounds.maximum[0]>120 && world.bounds.maximum[2]>170,
            "town retains the map's southeast footprint");
        for(const auto& cap:atmosphere::kBellwetherChimneyCaps) {
            unsigned topCorners=0;
            for(std::size_t i=before.vertices.size();i<world.vertices.size();++i) {
                const auto& vertex=world.vertices[i];const auto& p=vertex.position;
                if(vertex.normal[1]>.99f && std::abs(p[1]-cap[1])<.001f &&
                   std::abs(std::abs(p[0]-cap[0])-.55f)<.001f &&
                   std::abs(std::abs(p[2]-cap[2])-.5f)<.001f)++topCorners;
            }
            check(topCorners==4,"each smoke origin matches four actual native chimney cap corners");
        }
        bool slopingNormal=false;
        for(std::size_t i=before.vertices.size();i<world.vertices.size();++i) {
            const auto& n=world.vertices[i].normal;
            slopingNormal|=std::abs(n[1])>.1f && std::abs(n[1])<.99f;
        }
        check(slopingNormal,"real pitched roof normals survive native import");
        // The v4 decorative north doorway was replaced by real wall geometry.
        // A ray at the old portal post now hits the preserved north wall plane.
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
        check(std::abs(closestZ-103.2f)<.001f,
            "the north facade is closed at the original footprint");
        const auto admittedIndices=world.indices;
        const auto admittedSupport=support;
        check(!AppendBellwetherShell(root,world,support,error) && world.indices==admittedIndices &&
              support==admittedSupport,
            "repeated admission preserves the existing render owner and contact");
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
            if(world.nodeTransforms[p.nodeTransformIndex].name=="BellwetherSolid")
                check(volume>.005,"each solid town material range has outward native winding");
        }
        check(assets::ConsolidateWorldBakedMaterialFamilies(world,error) && world.primitives.size()==3,
            "town and furniture coalesce into the three existing opaque world families");
    }
    auto missing=before;missing.materials[1].name="Missing";
    auto fallback=originalTerrain;
    check(!AppendBellwetherShell(root,missing,fallback,error) && missing.indices==before.indices &&
          fallback==originalTerrain,"missing admitted clay rejects without partial publication");
    auto absent=before;
    check(!AppendBellwetherShell(root/"missing",absent,fallback,error) && absent.indices==before.indices &&
          fallback==originalTerrain,"missing native source rejects without partial publication");
    fallback.reset();
    check(!AppendBellwetherShell(root,absent,fallback,error) && absent.indices==before.indices &&
          !fallback,"unavailable terrain cannot publish render geometry without matching contact");
    std::cout<<"Bellwether native placement/material/rollback "<<(ok?"passed":"failed")<<".\n";
    return ok?0:1;
}
