#include "scene/BellwetherShell.h"
#include <algorithm>
#include <array>
#include <limits>
namespace horde::scene {
bool AppendBellwetherShell(const std::filesystem::path& root, assets::StaticMeshAsset& world,
    std::string& diagnostic) {
    if(std::any_of(world.nodeTransforms.begin(),world.nodeTransforms.end(),[](const auto& node){
        return node.name=="Original_Bellwether_closed_distant_shell";})) {
        diagnostic="Distant town is already admitted to this owner.";return false;
    }
    constexpr std::array<const char*,2> families{{"MedievalWall02","TombEarthenware"}};
    std::array<std::uint32_t,2> rows{};
    for(std::size_t i=0;i<families.size();++i) {
        const auto found=std::find_if(world.materials.begin(),world.materials.end(),
            [&](const auto& m){return m.name==families[i];});
        if(found==world.materials.end()) {diagnostic="Distant town requires the existing admitted stone/clay rows.";return false;}
        rows[i]=static_cast<std::uint32_t>(found-world.materials.begin());
    }
    const auto dir=root/"models/world/runtime/bellwether-shell-v2";
    assets::AssetManifest manifest;assets::StaticMeshAsset source;
    if(!assets::AssetManifest::Load(dir/"asset.manifest.json",manifest,diagnostic) ||
       !assets::StaticMeshAsset::Load(dir/"bellwether-shell-v2.glb",manifest,source,diagnostic))return false;
    if(source.primitives.size()!=2 || source.materials.size()!=2 || source.indices.size()!=342*3 ||
       world.primitives.empty() || world.nodeTransforms.empty() || world.primitives.size()+2>32 || world.vertices.size()>UINT32_MAX-source.vertices.size() ||
       world.indices.size()>UINT32_MAX-source.indices.size()) {
        diagnostic="Distant town lost its exact bounded native geometry.";return false;
    }
    constexpr std::array<const char*,2> names{{"HordeTownStone","HordeTownRoofClay"}};
    for(std::size_t i=0;i<names.size();++i) {
        const auto& m=source.materials[i];
        if(m.name!=names[i] || m.baseColorTexture>=0 || m.normalTexture>=0 || m.ormTexture>=0 ||
           m.emissiveTexture>=0 || m.metallicFactor!=0 || m.transmissionFactor!=0 ||
           m.emissiveFactor!=std::array<float,3>{}) {
            diagnostic="Distant town source requests unadmitted material or light features.";return false;
        }
    }
    auto staged=world;
    const auto vertices=static_cast<std::uint32_t>(staged.vertices.size());
    const auto indices=static_cast<std::uint32_t>(staged.indices.size());
    const auto transforms=static_cast<std::uint32_t>(staged.nodeTransforms.size());
    for(auto primitive:source.primitives) {
        primitive.vertexOffset+=vertices;primitive.indexOffset+=indices;
        primitive.nodeTransformIndex+=transforms;primitive.materialIndex=rows[primitive.materialIndex];
        staged.primitives.push_back(primitive);
    }
    staged.vertices.insert(staged.vertices.end(),source.vertices.begin(),source.vertices.end());
    staged.indices.insert(staged.indices.end(),source.indices.begin(),source.indices.end());
    staged.nodeTransforms.insert(staged.nodeTransforms.end(),source.nodeTransforms.begin(),source.nodeTransforms.end());
    for(unsigned axis=0;axis<3;++axis) {
        staged.bounds.minimum[axis]=std::min(staged.bounds.minimum[axis],source.bounds.minimum[axis]);
        staged.bounds.maximum[axis]=std::max(staged.bounds.maximum[axis],source.bounds.maximum[axis]);
    }
    world=std::move(staged);diagnostic.clear();return true;
}
}
