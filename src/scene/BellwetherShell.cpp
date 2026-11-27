#include "scene/BellwetherShell.h"
#include "scene/assets/WorldTriangleKey.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
namespace horde::scene {
namespace {
using assets::StaticMeshAsset;
constexpr std::string_view kTavernTimber="HordeTavernTimber";
constexpr std::string_view kTavernRuntimeDirectory="models/world/runtime/bellwether-tavern-v1";

bool LoadFurniture(const std::filesystem::path& directory,std::size_t level,StaticMeshAsset& mesh,
                   std::string& diagnostic) {
    const auto stem="tavern-furniture-lod"+std::to_string(level);
    assets::AssetManifest manifest;
    if(!assets::AssetManifest::Load(directory/(stem+".manifest.json"),manifest,diagnostic) ||
       !StaticMeshAsset::Load(directory/(stem+".glb"),manifest,mesh,diagnostic))return false;
    constexpr std::array identity{1.f,0.f,0.f,0.f, 0.f,1.f,0.f,0.f, 0.f,0.f,1.f,0.f, 0.f,0.f,0.f,1.f};
    if(mesh.primitives.size()!=1 || mesh.materials.size()!=1 || mesh.nodeTransforms.size()!=1 ||
       mesh.nodeTransforms.front().name!="BellwetherTavernFurniture" || mesh.nodeTransforms.front().world!=identity ||
       mesh.indices.size()!=kBellwetherFurnitureTriangles[level]*3 || mesh.materials.front().name!=kTavernTimber) {
        diagnostic="Bellwether furniture detail requires the registered timber mesh and its exact triangle count.";
        return false;
    }
    const auto& material=mesh.materials.front();
    if(material.baseColorTexture>=0 || material.normalTexture>=0 || material.ormTexture>=0 ||
       material.emissiveTexture>=0 || material.transmissionFactor!=0) {
        diagnostic="Bellwether furniture detail requires an untextured opaque finish.";return false;
    }
    return true;
}

bool PrepareFurnitureGeometry(const std::filesystem::path& root,StaticMeshAsset& source,
                              BellwetherFurnitureDetail detail,StaticMeshAsset* isolated,
                              std::string& diagnostic) {
    const auto level=static_cast<std::size_t>(detail);
    const auto directory=root/kTavernRuntimeDirectory;
    StaticMeshAsset original,replacement;
    if(level>=kBellwetherFurnitureTriangles.size()) {
        diagnostic="Unknown Bellwether furniture detail level.";return false;
    }
    if(!LoadFurniture(directory,0,original,diagnostic))return false;
    const auto loadLower=[&](std::size_t tier, StaticMeshAsset& mesh) {
        if(!LoadFurniture(directory,tier,mesh,diagnostic))return false;
        if(mesh.bounds.minimum==original.bounds.minimum && mesh.bounds.maximum==original.bounds.maximum &&
           mesh.vertices.size()<original.vertices.size())return true;
        diagnostic="Bellwether lower furniture detail must preserve bounds and reduce geometry.";return false;
    };
    if(!isolated && !loadLower(level,replacement))return false;
    const auto isTimber=[&](const auto& primitive){return source.materials[primitive.materialIndex].name==kTavernTimber;};
    const auto timber=std::find_if(source.primitives.begin(),source.primitives.end(),isTimber);
    if(timber==source.primitives.end() || std::find_if(std::next(timber),source.primitives.end(),isTimber)!=source.primitives.end()) {
        diagnostic="Bellwether furniture substitution needs one authored timber range.";return false;
    }
    std::map<assets::WorldTriangleKey,std::size_t> removable;
    for(std::size_t index=0;index<original.indices.size();index+=3)
        ++removable[assets::KeyWorldTriangle(original,original.primitives.front(),index)];

    StaticMeshAsset tiers;
    if(isolated) {
        tiers.materials.push_back(source.materials[timber->materialIndex]);
        tiers.nodeTransforms=original.nodeTransforms;
        tiers.bounds=original.bounds;
        auto near=original.primitives.front();
        near.indexCount=0;
        tiers.primitives.push_back(near);
    }

    auto primitives=source.primitives;
    std::vector<assets::StaticRtVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::size_t removed=0;
    vertices.reserve(source.vertices.size());
    indices.reserve(source.indices.size());
    for(std::size_t i=0;i<primitives.size();++i) {
        const auto& before=source.primitives[i];
        const auto vertexEnd=i+1<primitives.size()?source.primitives[i+1].vertexOffset:source.vertices.size();
        auto& after=primitives[i];
        after.vertexOffset=static_cast<std::uint32_t>(vertices.size());
        after.indexOffset=static_cast<std::uint32_t>(indices.size());
        if(&before!=&*timber) {
            vertices.insert(vertices.end(),source.vertices.begin()+before.vertexOffset,source.vertices.begin()+vertexEnd);
            indices.insert(indices.end(),source.indices.begin()+before.indexOffset,
                           source.indices.begin()+before.indexOffset+before.indexCount);
            continue;
        }
        std::vector<std::uint32_t> remap(vertexEnd-before.vertexOffset,UINT32_MAX);
        std::vector<std::uint32_t> nearRemap(isolated ? remap.size() : 0,UINT32_MAX);
        const auto appendTriangle=[&](std::size_t i,auto& map,auto& destVertices,auto& destIndices,
                                      std::uint32_t vertexBase) {
            for(std::size_t corner=0;corner<3;++corner) {
                const auto oldIndex=source.indices[before.indexOffset+i+corner];
                auto& newIndex=map[oldIndex];
                if(newIndex==UINT32_MAX) {
                    newIndex=static_cast<std::uint32_t>(destVertices.size())-vertexBase;
                    destVertices.push_back(source.vertices[before.vertexOffset+oldIndex]);
                }
                destIndices.push_back(newIndex);
            }
        };
        for(std::size_t index=0;index<before.indexCount;index+=3) {
            const auto found=removable.find(assets::KeyWorldTriangle(source,before,index));
            if(found!=removable.end() && found->second) {
                --found->second;
                ++removed;
                if(isolated)appendTriangle(index,nearRemap,tiers.vertices,tiers.indices,0u);
            }else {
                appendTriangle(index,remap,vertices,indices,after.vertexOffset);
            }
        }
        if(!isolated) {
            const auto replacementBase=static_cast<std::uint32_t>(vertices.size())-after.vertexOffset;
            vertices.insert(vertices.end(),replacement.vertices.begin(),replacement.vertices.end());
            for(const auto index:replacement.indices)indices.push_back(replacementBase+index);
        }
        after.indexCount=static_cast<std::uint32_t>(indices.size())-after.indexOffset;
    }
    if(removed!=kBellwetherFurnitureTriangles.front()) {
        diagnostic="Bellwether near furniture no longer matches the authored timber range.";return false;
    }
    if(isolated) {
        tiers.primitives.front().indexCount=static_cast<std::uint32_t>(tiers.indices.size());
        for(std::size_t tier=1;tier<kBellwetherFurnitureTriangles.size();++tier) {
            StaticMeshAsset lower;
            if(!loadLower(tier,lower))return false;
            auto primitive=lower.primitives.front();
            primitive.vertexOffset=static_cast<std::uint32_t>(tiers.vertices.size());
            primitive.indexOffset=static_cast<std::uint32_t>(tiers.indices.size());
            tiers.primitives.push_back(primitive);
            tiers.vertices.insert(tiers.vertices.end(),lower.vertices.begin(),lower.vertices.end());
            tiers.indices.insert(tiers.indices.end(),lower.indices.begin(),lower.indices.end());
        }
        *isolated=std::move(tiers);
    }
    source.primitives=std::move(primitives);
    source.vertices=std::move(vertices);
    source.indices=std::move(indices);
    return true;
}
}

bool HasBellwetherFurnitureRuntime(const std::filesystem::path& root) {
    const auto directory=root/kTavernRuntimeDirectory;
    for(std::size_t tier=0;tier<kBellwetherFurnitureTriangles.size();++tier) {
        const auto stem="tavern-furniture-lod"+std::to_string(tier);
        for(const auto extension:{".manifest.json",".glb"}) {
            std::error_code error;
            if(!std::filesystem::is_regular_file(directory/(stem+extension),error))return false;
        }
    }
    return true;
}

float BellwetherFurnitureRoomDistanceSquared(float x,float z) noexcept {
    const auto& low=gameplay::bellwether::kRoomMin;
    const auto& high=gameplay::bellwether::kRoomMax;
    const float dx=x-std::clamp(x,low[0],high[0]);
    const float dz=z-std::clamp(z,low[2],high[2]);
    return dx*dx+dz*dz;
}

BellwetherFurnitureDetail SelectBellwetherFurnitureDetail(
    float x,float z,BellwetherFurnitureDetail current) noexcept {
    using Detail=BellwetherFurnitureDetail;
    if(!std::isfinite(x) || !std::isfinite(z))return Detail::Near;
    const float distanceSquared=BellwetherFurnitureRoomDistanceSquared(x,z);
    // A full near perimeter protects both faces of the window and the entrance.
    const float nearRadius=current==Detail::Near ? 6.f : 4.f;
    const float midRadius=current==Detail::Far ? 16.f : 20.f;
    if(distanceSquared<=nearRadius*nearRadius)return Detail::Near;
    return distanceSquared<=midRadius*midRadius ? Detail::Mid : Detail::Far;
}

bool AppendBellwetherShell(const std::filesystem::path& root, assets::StaticMeshAsset& world,
    std::shared_ptr<const gameplay::simulation::TriangleSupportSurface>& support,
    std::string& diagnostic, assets::StaticMeshAsset* glazing, assets::StaticMeshAsset* archiveCover,
    assets::StaticMeshAsset* roadRegister, assets::StaticMeshAsset* southDoor,
    BellwetherFurnitureDetail furnitureDetail, StaticMeshAsset* isolatedFurnitureTiers,
    const assets::StaticMaterial* timberMaterialProvider) {
    if(!support) {
        diagnostic="Bellwether contact requires admitted chapter terrain.";return false;
    }
    if(std::any_of(world.nodeTransforms.begin(),world.nodeTransforms.end(),[](const auto& node){
        return node.name=="BellwetherSolid";})) {
        diagnostic="Bellwether is already admitted to this owner.";return false;
    }
    constexpr std::array names{"HordeTownStone","HordeTownRoofClay","HordeTavernTimber","HordeHearthChar"};
    constexpr std::array families{"MedievalWall02","TombEarthenware","TombEarthenware","TombCharredWick"};
    if(timberMaterialProvider && (timberMaterialProvider->name!="ChestWood" ||
       timberMaterialProvider->baseColorTexture<0 || timberMaterialProvider->normalTexture<0 ||
       timberMaterialProvider->ormTexture<0 || timberMaterialProvider->emissiveTexture>=0 ||
       timberMaterialProvider->textureGroup<0 || timberMaterialProvider->deferTextureAllocation ||
       timberMaterialProvider->transmissionFactor!=0.f ||
       timberMaterialProvider->emissiveFactor!=std::array<float,3>{} ||
       (timberMaterialProvider->flags&assets::kStaticMaterialCertifiedClosedVolume))) {
        diagnostic="Bellwether timber requires the admitted opaque ChestWood texture provider.";return false;
    }
    const auto dir=root/kTavernRuntimeDirectory;
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
                  material.emissiveTexture>=0 || material.transmissionFactor!=0.f;
       })) {
        diagnostic="Bellwether south door requires a local-space timber leaf with opaque fittings.";return false;
    }
    if(timberMaterialProvider) {
        for(auto& material:door.materials)if(material.name==kTavernTimber) {
            const auto name=material.name;
            material=*timberMaterialProvider;
            material.name=name;
        }
    }
    if(std::any_of(door.materials.begin(),door.materials.end(),[timberMaterialProvider](const auto& material) {
        const bool timber=material.name==kTavernTimber;
        return material.transmissionFactor!=0.f ||
            (!timber || !timberMaterialProvider) &&
                (material.baseColorTexture>=0 || material.normalTexture>=0 || material.ormTexture>=0 ||
                 material.emissiveTexture>=0);
    })) {
        diagnostic="Bellwether south door materials must preserve opaque fittings and the admitted timber maps.";return false;
    }
    std::array<std::uint32_t,names.size()> mappedRows{};
    std::array<bool,names.size()> seen{};
    std::array<bool,names.size()> wood{};
    auto stagedMaterials=world.materials;
    if(timberMaterialProvider) {
        if(std::any_of(stagedMaterials.begin(),stagedMaterials.end(),[](const auto& material) {
            return material.name==kTavernTimber;
        })) {
            diagnostic="Bellwether timber provider row already exists in the world owner.";return false;
        }
        auto timber=*timberMaterialProvider;
        timber.name=std::string(kTavernTimber);
        stagedMaterials.push_back(std::move(timber));
    }
    for(std::size_t i=0;i<names.size();++i) {
        const auto& m=source.materials[i];
        const auto named=std::find(names.begin(),names.end(),m.name);
        if(named==names.end() || m.baseColorTexture>=0 || m.normalTexture>=0 || m.ormTexture>=0 ||
           m.emissiveTexture>=0 || m.metallicFactor!=0 || m.transmissionFactor!=0 ||
           m.emissiveFactor!=std::array<float,3>{}) {
            diagnostic="Bellwether source requests unadmitted material or light features.";return false;
        }
        const auto index=static_cast<std::size_t>(named-names.begin());
        const auto rowName=index==2u && timberMaterialProvider ? kTavernTimber : families[index];
        const auto found=std::find_if(stagedMaterials.begin(),stagedMaterials.end(),
            [&](const auto& material){return material.name==rowName;});
        if(seen[index] || found==stagedMaterials.end()) {
            diagnostic="Bellwether requires each authored finish and its existing material row.";return false;
        }
        seen[index]=true;
        mappedRows[i]=static_cast<std::uint32_t>(found-stagedMaterials.begin());
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

    // Collision comes from the original geometry. The fixed review tier is
    // selected before the static render owner builds its buffers and BLAS.
    StaticMeshAsset furniture;
    if((isolatedFurnitureTiers || furnitureDetail!=BellwetherFurnitureDetail::Near) &&
       !PrepareFurnitureGeometry(root,source,furnitureDetail,isolatedFurnitureTiers ? &furniture : nullptr,diagnostic))
        return false;

    auto staged=world;
    staged.materials=std::move(stagedMaterials);
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
    if(isolatedFurnitureTiers) {
        const auto timber=std::find_if(source.materials.begin(),source.materials.end(),
            [](const auto& material){return material.name==kTavernTimber;});
        furniture.materials.front()=world.materials[mappedRows[timber-source.materials.begin()]];
        *isolatedFurnitureTiers=std::move(furniture);
    }
    if(glazing)*glazing=std::move(pane);
    if(archiveCover)*archiveCover=std::move(cover);
    if(roadRegister)*roadRegister=std::move(ledger);
    if(southDoor)*southDoor=std::move(door);
    diagnostic.clear();
    return true;
}

bool AppendBellwetherGardenCrops(const std::filesystem::path& root, assets::StaticMeshAsset& world,
    std::string& diagnostic) {
    static constexpr std::string_view nodeName="BellwetherGardenCrops";
    constexpr std::string_view materialName="HordeGardenLeaf";
    constexpr auto triangleCount=gameplay::bellwether::kBellwetherGardenCropTriangles;
    constexpr std::array identity{1.f,0.f,0.f,0.f, 0.f,1.f,0.f,0.f, 0.f,0.f,1.f,0.f, 0.f,0.f,0.f,1.f};
    constexpr auto allowedFlags=static_cast<std::uint32_t>(horde::vulkan::raytracing::RtMaterialFlag::DoubleSided);
    constexpr std::uint32_t maximumManifestIndices=4096u*3u;
    constexpr std::uint32_t maximumManifestVertices=8192u;
    if(std::any_of(world.nodeTransforms.begin(),world.nodeTransforms.end(),[](const auto& node) {
        return node.name==nodeName;
    })) {
        diagnostic="Bellwether garden crops are already admitted to this owner.";return false;
    }
    const auto material=std::find_if(world.materials.begin(),world.materials.end(),[](const auto& row) {
        return row.name=="ForestAlder";
    });
    if(!world.HasValidTriangleProvenance() || world.primitives.empty() || world.nodeTransforms.empty() ||
       world.primitives.size()>=32 || material==world.materials.end() || material->textureGroup!=3 ||
       !material->deferTextureAllocation || world.vertices.size()>UINT32_MAX || world.indices.size()>UINT32_MAX ||
       world.nodeTransforms.size()>=UINT32_MAX) {
        diagnostic="Bellwether garden crops require the admitted deferred ForestAlder family and bounded world owner.";
        return false;
    }
    const auto directory=root/"models/world/runtime/bellwether-tavern-v1";
    assets::AssetManifest manifest;
    assets::StaticMeshAsset crops;
    if(!assets::AssetManifest::Load(directory/"garden-crops.manifest.json",manifest,diagnostic) ||
       !assets::StaticMeshAsset::Load(directory/"garden-crops.glb",manifest,crops,diagnostic))return false;
    if(manifest.budgets.maxIndices>maximumManifestIndices || manifest.budgets.maxVertices>maximumManifestVertices ||
       manifest.budgets.maxPrimitives!=1u || manifest.budgets.maxMaterials!=1u ||
       crops.primitives.size()!=1 || crops.materials.size()!=1 || crops.nodeTransforms.size()!=1 ||
       !crops.sockets.empty() || crops.indices.size()!=triangleCount*3u ||
       crops.vertices.empty() || crops.primitives.front().vertexOffset!=0 ||
       crops.primitives.front().indexOffset!=0 || crops.primitives.front().indexCount!=crops.indices.size() ||
       crops.primitives.front().materialIndex!=0 || crops.primitives.front().nodeTransformIndex!=0 ||
       crops.nodeTransforms.front().name!=nodeName || crops.nodeTransforms.front().world!=identity) {
        diagnostic="Bellwether garden crops require one bounded world-space primitive with the registered node and exact triangle count.";
        return false;
    }
    const auto& sourceMaterial=crops.materials.front();
    if(sourceMaterial.name!=materialName || sourceMaterial.baseColorTexture>=0 || sourceMaterial.normalTexture>=0 ||
       sourceMaterial.ormTexture>=0 || sourceMaterial.emissiveTexture>=0 || sourceMaterial.transmissionFactor!=0.f ||
       sourceMaterial.baseColorFactor[3]!=1.f || sourceMaterial.emissiveFactor!=std::array<float,3>{} ||
       sourceMaterial.metallicFactor!=0.f || (sourceMaterial.flags&~allowedFlags)!=0u) {
        diagnostic="Bellwether garden crops require an untextured opaque HordeGardenLeaf material.";return false;
    }
    for(unsigned axis=0;axis<3;++axis) {
        if(!std::isfinite(crops.bounds.minimum[axis]) || !std::isfinite(crops.bounds.maximum[axis]) ||
           crops.bounds.minimum[axis]>crops.bounds.maximum[axis] ||
           std::abs(crops.bounds.minimum[axis])>1000.f || std::abs(crops.bounds.maximum[axis])>1000.f) {
            diagnostic="Bellwether garden crop bounds are not finite, local to the world, and bounded.";return false;
        }
    }
    for(const auto& vertex:crops.vertices) {
        for(const auto value:vertex.position)if(!std::isfinite(value) || std::abs(value)>1000.f) {
            diagnostic="Bellwether garden crop geometry contains an invalid or unbounded position.";return false;
        }
        for(const auto value:vertex.normal)if(!std::isfinite(value)) {
            diagnostic="Bellwether garden crop geometry contains an invalid normal.";return false;
        }
        for(const auto value:vertex.tangent)if(!std::isfinite(value)) {
            diagnostic="Bellwether garden crop geometry contains an invalid tangent.";return false;
        }
        for(const auto value:vertex.uv0)if(!std::isfinite(value)) {
            diagnostic="Bellwether garden crop geometry contains invalid texture coordinates.";return false;
        }
    }
    if(std::any_of(crops.indices.begin(),crops.indices.end(),[&](std::uint32_t index) {
        return index>=crops.vertices.size();
    }) || world.vertices.size()>UINT32_MAX-crops.vertices.size() ||
       world.indices.size()>UINT32_MAX-crops.indices.size()) {
        diagnostic="Bellwether garden crops exceed native vertex/index addressing.";return false;
    }

    auto staged=world;
    auto primitive=crops.primitives.front();
    primitive.vertexOffset=static_cast<std::uint32_t>(staged.vertices.size());
    primitive.indexOffset=static_cast<std::uint32_t>(staged.indices.size());
    primitive.materialIndex=static_cast<std::uint32_t>(material-world.materials.begin());
    primitive.nodeTransformIndex=static_cast<std::uint32_t>(staged.nodeTransforms.size());
    staged.primitives.push_back(primitive);
    staged.nodeTransforms.push_back(crops.nodeTransforms.front());
    staged.vertices.insert(staged.vertices.end(),crops.vertices.begin(),crops.vertices.end());
    staged.triangleRegions.resize(staged.indices.size()/3u,assets::StaticTriangleRegion::Shared);
    staged.triangleRegions.insert(staged.triangleRegions.end(),triangleCount,assets::StaticTriangleRegion::Shared);
    staged.indices.insert(staged.indices.end(),crops.indices.begin(),crops.indices.end());
    for(unsigned axis=0;axis<3;++axis) {
        staged.bounds.minimum[axis]=std::min(staged.bounds.minimum[axis],crops.bounds.minimum[axis]);
        staged.bounds.maximum[axis]=std::max(staged.bounds.maximum[axis],crops.bounds.maximum[axis]);
    }
    if(!staged.HasValidTriangleProvenance() || staged.primitives.size()>32u) {
        diagnostic="Bellwether garden crops would invalidate the shared world owner.";return false;
    }
    world=std::move(staged);
    diagnostic.clear();
    return true;
}
}
