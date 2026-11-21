#include "scene/BellwetherShell.h"
#include <algorithm>
#include <array>
#include <limits>
namespace horde::scene {
bool AppendBellwetherShell(const std::filesystem::path& root, assets::StaticMeshAsset& world,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>& support,
    std::string& diagnostic, assets::StaticMeshAsset* glazing, assets::StaticMeshAsset* archiveCover,
    assets::StaticMeshAsset* roadRegister, assets::StaticMeshAsset* southDoor) {
    if(!support) {
        diagnostic="Bellwether contact requires admitted chapter terrain.";return false;
    }
    if(std::any_of(world.nodeTransforms.begin(),world.nodeTransforms.end(),[](const auto& node){
        return node.name=="BellwetherSolid";})) {
        diagnostic="Bellwether is already admitted to this owner.";return false;
    }
    constexpr std::array names{"HordeTownStone","HordeTownRoofClay","HordeTavernTimber","HordeHearthChar"};
    constexpr std::array families{"MedievalWall02","TombEarthenware","TombEarthenware","TombCharredWick"};
    const auto dir=root/"models/world/runtime/bellwether-tavern-v1";
    assets::AssetManifest manifest, glazingManifest, coverManifest, registerManifest, doorManifest;
    assets::StaticMeshAsset source, pane, cover, ledger, door;
    if(!assets::AssetManifest::Load(dir/"asset.manifest.json",manifest,diagnostic) ||
       !assets::StaticMeshAsset::Load(dir/"bellwether-tavern-v1.glb",manifest,source,diagnostic) ||
       !assets::AssetManifest::Load(dir/"window.manifest.json",glazingManifest,diagnostic) ||
       !assets::StaticMeshAsset::Load(dir/"bellwether-window.glb",glazingManifest,pane,diagnostic) ||
       !assets::AssetManifest::Load(dir/"wayside-archive-cover.manifest.json",coverManifest,diagnostic) ||
       !assets::StaticMeshAsset::Load(dir/"wayside-archive-cover.glb",coverManifest,cover,diagnostic) ||
       !assets::AssetManifest::Load(dir/"wayside-road-register.manifest.json",registerManifest,diagnostic) ||
       !assets::StaticMeshAsset::Load(dir/"wayside-road-register.glb",registerManifest,ledger,diagnostic) ||
       !assets::AssetManifest::Load(dir/"south-door.manifest.json",doorManifest,diagnostic) ||
       !assets::StaticMeshAsset::Load(dir/"bellwether-south-door.glb",doorManifest,door,diagnostic))return false;
    if(pane.primitives.size()!=1 || pane.materials.size()!=1 || pane.indices.size()!=kBellwetherGlazingTriangles*3 ||
       pane.materials[0].name!="HordeTavernWindowGlass" || pane.materials[0].transmissionFactor<=0 ||
       !(pane.materials[0].flags & assets::kStaticMaterialCertifiedClosedVolume) || pane.nodeTransforms.size()!=1 ||
       pane.nodeTransforms[0].name!="BellwetherGlazing") {
        diagnostic="Bellwether window requires one certified closed dielectric volume.";return false;
    }
    if(cover.primitives.size()!=1 || cover.materials.size()!=1 || cover.nodeTransforms.size()!=1 ||
       cover.nodeTransforms.front().name!="WaysideArchiveCover" || cover.indices.size()!=kWaysideCoverTriangles*3 ||
       cover.materials.front().name!="HordeWaysideCoverStone" || cover.materials.front().transmissionFactor!=0) {
        diagnostic="The wayside archive cover requires one opaque, local-space stone primitive.";return false;
    }
    if(ledger.primitives.size()!=2 || ledger.materials.size()!=2 || ledger.nodeTransforms.size()!=1 ||
       ledger.nodeTransforms.front().name!="WaysideRoadRegister" || ledger.indices.size()!=kWaysideRegisterTriangles*3 ||
       std::any_of(ledger.materials.begin(),ledger.materials.end(),[](const auto& material) {
           return material.baseColorTexture>=0 || material.normalTexture>=0 || material.ormTexture>=0 ||
                  material.emissiveTexture>=0 || material.transmissionFactor!=0;
       })) {
        diagnostic="The wayside register requires two untextured opaque finishes.";return false;
    }
    if(source.primitives.size()!=5 || source.materials.size()!=names.size() ||
       source.indices.size()!=(kBellwetherSolidTriangles+kBellwetherGroundTriangles)*3 ||
       world.primitives.empty() || world.nodeTransforms.empty() || world.primitives.size()+source.primitives.size()>32 ||
       world.vertices.size()>UINT32_MAX-source.vertices.size() ||
       world.indices.size()>UINT32_MAX-source.indices.size()) {
        diagnostic="Bellwether lost its exact bounded native geometry.";return false;
    }
    if(door.primitives.size()!=2 || door.materials.size()!=2 || door.nodeTransforms.size()!=1 ||
       door.nodeTransforms.front().name!="BellwetherSouthDoor" ||
       door.indices.size()!=gameplay::bellwether::kSouthDoorTriangles*3 ||
       std::any_of(door.materials.begin(),door.materials.end(),[](const auto& material) {
           return (material.name!="HordeTavernTimber" && material.name!="HordeHearthChar") ||
                  material.baseColorTexture>=0 || material.normalTexture>=0 || material.ormTexture>=0 ||
                  material.emissiveTexture>=0 || material.transmissionFactor!=0;
       })) {
        diagnostic="Bellwether south door requires a local-space timber leaf with opaque fittings.";return false;
    }
    std::array<std::uint32_t,names.size()> mappedRows{};
    std::array<bool,names.size()> seen{};
    std::array<bool,names.size()> wood{};
    for(std::size_t i=0;i<names.size();++i) {
        const auto& m=source.materials[i];
        const auto named=std::find(names.begin(),names.end(),m.name);
        if(named==names.end() || m.baseColorTexture>=0 || m.normalTexture>=0 || m.ormTexture>=0 ||
           m.emissiveTexture>=0 || m.metallicFactor!=0 || m.transmissionFactor!=0 ||
           m.emissiveFactor!=std::array<float,3>{}) {
            diagnostic="Bellwether source requests unadmitted material or light features.";return false;
        }
        const auto index=static_cast<std::size_t>(named-names.begin());
        const auto found=std::find_if(world.materials.begin(),world.materials.end(),
            [&](const auto& material){return material.name==families[index];});
        if(seen[index] || found==world.materials.end()) {
            diagnostic="Bellwether requires each authored finish and its existing material row.";return false;
        }
        seen[index]=true;
        mappedRows[i]=static_cast<std::uint32_t>(found-world.materials.begin());
        wood[i]=index>=2;
    }
    using Surface=gameplay::simulation::TriangleSupportSurface;
    std::vector<Surface::Triangle> contact,ground;
    contact.reserve(kBellwetherSolidTriangles+kBellwetherGlazingTriangles);
    ground.reserve(kBellwetherGroundTriangles);
    const auto appendContact=[&](const auto& mesh, const auto& primitive, auto surface, auto& triangles) {
        for(std::size_t k=0;k<primitive.indexCount;k+=3) {
            Surface::Triangle triangle;
            triangle.surface=surface;
            for(std::size_t corner=0;corner<3;++corner) {
                const auto local=mesh.indices[primitive.indexOffset+k+corner];
                const auto& position=mesh.vertices[primitive.vertexOffset+local].position;
                triangle.points[corner]={position[0],position[1],position[2]};
            }
            triangles.push_back(triangle);
        }
    };
    for(const auto& primitive:source.primitives) {
        const auto& name=source.nodeTransforms[primitive.nodeTransformIndex].name;
        if(name!="BellwetherSolid" && name!="BellwetherWalkable") {
            diagnostic="Bellwether geometry is missing its solid or walkable role.";return false;
        }
        appendContact(source,primitive,wood[primitive.materialIndex] ? gameplay::simulation::SupportSurface::Wood :
            gameplay::simulation::SupportSurface::Stone,name=="BellwetherWalkable"?ground:contact);
    }
    if(contact.size()!=kBellwetherSolidTriangles || ground.size()!=kBellwetherGroundTriangles) {
        diagnostic="Bellwether solid and walkable triangle inventories differ.";return false;
    }
    appendContact(pane,pane.primitives.front(),gameplay::simulation::SupportSurface::Stone,contact);
    const auto raised=Surface::WithAdditionalGroundGeometry(*support,ground,diagnostic);
    if(!raised)return false;
    auto prepared=Surface::WithAdditionalNonWalkableGeometry(*raised,contact,diagnostic);
    if(!prepared)return false;
    std::vector<Surface::Triangle> hingedContact;
    appendContact(cover,cover.primitives.front(),gameplay::simulation::SupportSurface::Stone,hingedContact);
    prepared=Surface::WithHingedContact(*prepared,hingedContact,diagnostic);
    if(!prepared)return false;
    hingedContact.clear();
    for(const auto& primitive:door.primitives)
        appendContact(door,primitive,gameplay::simulation::SupportSurface::Wood,hingedContact);
    prepared=Surface::WithHingedContact(*prepared,hingedContact,diagnostic,Surface::HingedPart::TavernDoor);
    if(!prepared)return false;

    auto staged=world;
    const auto vertices=static_cast<std::uint32_t>(staged.vertices.size());
    const auto indices=static_cast<std::uint32_t>(staged.indices.size());
    const auto transforms=static_cast<std::uint32_t>(staged.nodeTransforms.size());
    for(auto primitive:source.primitives) {
        primitive.vertexOffset+=vertices;primitive.indexOffset+=indices;
        primitive.nodeTransformIndex+=transforms;primitive.materialIndex=mappedRows[primitive.materialIndex];
        staged.primitives.push_back(primitive);
    }
    staged.vertices.insert(staged.vertices.end(),source.vertices.begin(),source.vertices.end());
    staged.indices.insert(staged.indices.end(),source.indices.begin(),source.indices.end());
    staged.nodeTransforms.insert(staged.nodeTransforms.end(),source.nodeTransforms.begin(),source.nodeTransforms.end());
    for(unsigned axis=0;axis<3;++axis) {
        staged.bounds.minimum[axis]=std::min(staged.bounds.minimum[axis],source.bounds.minimum[axis]);
        staged.bounds.maximum[axis]=std::max(staged.bounds.maximum[axis],source.bounds.maximum[axis]);
    }
    world=std::move(staged);
    support=std::move(prepared);
    if(glazing)*glazing=std::move(pane);
    if(archiveCover)*archiveCover=std::move(cover);
    if(roadRegister)*roadRegister=std::move(ledger);
    if(southDoor)*southDoor=std::move(door);
    diagnostic.clear();
    return true;
}
}
