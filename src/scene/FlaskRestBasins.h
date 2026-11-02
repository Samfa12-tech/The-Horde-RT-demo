#pragma once
#include "scene/assets/StaticMeshAsset.h"
#include "gameplay/traversal/ChapterTerrainMovement.h"
#include <algorithm>
#include <memory>

namespace horde::scene {
// Original code-authored stone furniture. One repeatable hollow basin with a
// carved flask on its back teaches refill; no imported source, light or water
// shader. All three share the existing masonry family and static RT owner.
struct FlaskRestBasinPlacement {
    gameplay::traversal::Vec3 anchor{}, centre{};
};
inline bool AppendFlaskRestBasins(assets::StaticMeshAsset& owner,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>& terrain,
    std::array<FlaskRestBasinPlacement,3>& placements,std::string& diagnostic)
{
    namespace sim=gameplay::simulation;namespace tr=gameplay::traversal;
    using Surface=sim::TriangleSupportSurface;
    constexpr const char* name="original-earned-flask-rest-basins";
    const auto material=std::find_if(owner.materials.begin(),owner.materials.end(),
        [](const auto& m){return m.name=="MedievalWall02";});
    if(!terrain || material==owner.materials.end() || owner.primitives.empty() ||
       owner.primitives.size()>=32 || !owner.HasValidTriangleProvenance() ||
       std::any_of(owner.nodeTransforms.begin(),owner.nodeTransforms.end(),
           [](const auto& n){return n.name==name;})) {
        diagnostic="Rest basins require fresh bounded masonry and actual chapter support.";return false;
    }
    const auto end=sim::kWorldRoutePoints.back();const auto ground=terrain->Query(end.x,end.z);
    if(!ground.found || !ground.walkable) {diagnostic="Lookout basin lacks canonical support.";return false;}
    placements={{{tr::kLowerLanding,tr::kLowerLanding},
        {tr::kExteriorLanding,tr::kExteriorLanding},
        {{end.x,ground.worldY,end.z},{end.x,ground.worldY,end.z}}}};
    std::vector<assets::StaticRtVertex> vertices;std::vector<std::uint32_t> indices;
    std::vector<Surface::Triangle> contact;
    for(std::size_t site=0;site<placements.size();++site) {
        auto& p=placements[site];p.centre.x+=site==2?-.78f:.78f;
        float bottom=-INFINITY;
        for(float dx:{-.23f,.23f})for(float dz:{-.23f,.23f}) {
            const float x=p.centre.x+dx,z=p.centre.z+dz;
            if(site==0) {
                if(!gameplay::IsShowcasePlayerPositionWalkable(x,z)) {
                    diagnostic="Lower basin footprint leaves its authored stone floor.";return false;
                }
                bottom=tr::kLowerSupportWorldY;
            } else {
                const auto under=tr::ChapterExteriorSupport(terrain.get(),x,z);
                if(!under.grounded || std::abs(under.worldY-p.anchor.y)>.08f) {
                    diagnostic="Basin footprint leaves its earned landing or terrain support.";return false;
                }
                bottom=std::max(bottom,under.worldY);
            }
        }
        p.centre.y=bottom;
        // Square stepped foot, slender stem, hollow bowl with four thick rims,
        // then a small bottle relief rising at its back. No landing-centre mesh.
        const auto box=[&](float x,float y,float z,float hx,float hy,float hz) {
            const std::array<std::array<float,3>,8> corners{{
                {{x-hx,y-hy,z-hz}},{{x+hx,y-hy,z-hz}},{{x+hx,y+hy,z-hz}},{{x-hx,y+hy,z-hz}},
                {{x-hx,y-hy,z+hz}},{{x+hx,y-hy,z+hz}},{{x+hx,y+hy,z+hz}},{{x-hx,y+hy,z+hz}}}};
            constexpr std::array<std::array<unsigned,4>,6> faces{{{{0,3,2,1}},{{4,5,6,7}},
                {{0,4,7,3}},{{1,2,6,5}},{{0,1,5,4}},{{3,7,6,2}}}};
            constexpr std::array<std::array<float,3>,6> normals{{{{0,0,-1}},{{0,0,1}},
                {{-1,0,0}},{{1,0,0}},{{0,-1,0}},{{0,1,0}}}};
            for(std::size_t f=0;f<faces.size();++f) {
                const auto base=static_cast<std::uint32_t>(vertices.size());
                for(unsigned k=0;k<4;++k) {
                    const auto c=corners[faces[f][k]];assets::StaticRtVertex v{};
                    v.position={p.centre.x+c[0],bottom+c[1],p.centre.z+c[2],1};
                    v.normal={normals[f][0],normals[f][1],normals[f][2],0};
                    v.tangent=f<2?std::array<float,4>{1,0,0,1}:std::array<float,4>{0,0,1,1};
                    v.uv0={float(k==1||k==2)*.4f,float(k>=2)*.4f,0,0};vertices.push_back(v);
                }
                for(const auto tri:std::array<std::array<unsigned,3>,2>{{{{0,1,2}},{{0,2,3}}}}) {
                    Surface::Triangle t;t.surface=sim::SupportSurface::Stone;
                    for(unsigned k=0;k<3;++k) {
                        const auto index=base+tri[k];indices.push_back(index);
                        const auto& v=vertices[index].position;t.points[k]={v[0],v[1],v[2]};
                    }
                    contact.push_back(t);
                }
            }
        };
        box(0,0,0,.23f,.10f,.23f);box(0,.23f,0,.115f,.14f,.115f);
        box(0,.39f,0,.22f,.035f,.22f);
        box(-.195f,.47f,0,.025f,.05f,.22f);box(.195f,.47f,0,.025f,.05f,.22f);
        box(0,.47f,-.195f,.17f,.05f,.025f);box(0,.47f,.195f,.17f,.05f,.025f);
        box(0,.58f,.165f,.085f,.10f,.035f);box(0,.715f,.165f,.035f,.035f,.035f);
    }
    auto support=Surface::WithAdditionalNonWalkableGeometry(*terrain,contact,diagnostic);
    if(!support)return false;
    // The same triangles govern physical contact; canonical centres and the
    // rope approach remain clear, and a player there can reach the1m action.
    for(const auto& p:placements)if(!support->NonWalkableVolumeClear(p.anchor.x,p.anchor.z,
        p.anchor.x,p.anchor.z,p.anchor.y+.001f,p.anchor.y+1.8f,gameplay::kPlayerCollisionRadius)) {
        diagnostic="Rest basin obstructs the existing earned player anchor.";return false;
    }
    auto staged=owner;
    staged.primitives.push_back({static_cast<std::uint32_t>(staged.vertices.size()),
        static_cast<std::uint32_t>(staged.indices.size()),static_cast<std::uint32_t>(indices.size()),
        static_cast<std::uint32_t>(material-owner.materials.begin()),static_cast<std::uint32_t>(staged.nodeTransforms.size())});
    staged.nodeTransforms.push_back({name,{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}}});
    for(const auto& v:vertices)for(unsigned axis=0;axis<3;++axis) {
        staged.bounds.minimum[axis]=std::min(staged.bounds.minimum[axis],v.position[axis]);
        staged.bounds.maximum[axis]=std::max(staged.bounds.maximum[axis],v.position[axis]);
    }
    staged.vertices.insert(staged.vertices.end(),vertices.begin(),vertices.end());
    staged.triangleRegions.resize(staged.indices.size()/3,assets::StaticTriangleRegion::Shared);
    staged.triangleRegions.insert(staged.triangleRegions.end(),indices.size()/3,assets::StaticTriangleRegion::Shared);
    staged.indices.insert(staged.indices.end(),indices.begin(),indices.end());
    owner=std::move(staged);terrain=std::move(support);diagnostic.clear();return true;
}
}
