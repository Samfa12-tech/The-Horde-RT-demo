#include "vulkan/raytracing/RtTextureLayerSubset.h"
#include "vulkan/raytracing/RtChapterTextureLayout.h"
#include "vulkan/raytracing/RtKtxArrayAppend.h"
#include "vulkan/raytracing/RtStaticTextureSelection.h"

#include <array>
#include <iostream>
#include <limits>
#include <cstring>
#include <filesystem>
#include <fstream>

int main(int argc,char** argv)
{
    using horde::vulkan::raytracing::AppendRtTextureLayerSubset;
    bool passed = true;
    const auto check = [&passed](bool condition, const char* label) {
        if (!condition) { std::cerr << label << "\n"; passed = false; }
    };
    using namespace horde::vulkan::raytracing;
    const auto legacy = SelectRtStaticTextureLayers(RtSceneProfile::Showcase,true,false);
    check(legacy.baseColor.size()==14 && legacy.normalOrm.size()==14 && !legacy.appendTerrain &&
          legacy.aliases.empty(), "ordinary production startup must select its legacy atlas rows");
    for(std::size_t i=0;i<legacy.baseColor.size();++i)
        check(legacy.baseColor[i]==i,"legacy base layer indices remain unchanged");
    for(std::size_t i=0;i<legacy.normalOrm.size();++i)
        check(legacy.normalOrm[i]==i,"legacy normal/ORM indices remain unchanged");
    for(auto profile:{RtSceneProfile::EntryMenu,RtSceneProfile::GraphicsPreview}) {
        const auto selected=SelectRtStaticTextureLayers(profile,true,true);
        check(selected.baseColor.size()==(profile==RtSceneProfile::EntryMenu?2u:3u) &&
              !selected.appendTerrain && selected.aliases.empty(), "menus retain narrow atlas admission");
    }
    check(SelectRtStaticTextureLayers(RtSceneProfile::Showcase,false,false).baseColor.empty(),
          "synthetic development assets keep their own exact array layout");
    const auto chapter=SelectRtStaticTextureLayers(RtSceneProfile::Showcase,true,true);
    check(chapter.baseColor.size()==15 && chapter.normalOrm.size()==12 && chapter.appendTerrain &&
          chapter.aliases.size()==3,"chapter keeps deduplicated layers and terrain addition");
    const std::array<std::uint8_t, 12> source{10,11,12,13,20,21,22,23,30,31,32,33};
    const std::array<std::uint32_t, 3> reorder{2,0,2};
    std::vector<std::uint8_t> destination{99};
    check(AppendRtTextureLayerSubset(source, 3, 4, reorder, destination), "valid compressed layer selection");
    check(destination == std::vector<std::uint8_t>({99,30,31,32,33,10,11,12,13,30,31,32,33}), "byte exact reordered/repeated layers");
    const std::array<std::uint8_t, 3> nextMip{1,2,3};
    check(AppendRtTextureLayerSubset(nextMip, 3, 1, reorder, destination), "append subsequent mip");
    check(destination[destination.size()-3] == 3 && destination[destination.size()-2] == 1 && destination.back() == 3, "mip layer order retained");
    const auto saved = destination;
    using horde::scene::assets::StaticMeshAsset;
    using horde::scene::assets::StaticMaterial;
    const auto material=[](const char* name) {
        StaticMaterial m; m.name=name;m.baseColorTexture=0;m.normalTexture=1;m.ormTexture=2;
        return m;
    };
    StaticMeshAsset base,lid,ring,body;
    base.materials={material("ChestWood"),material("BlackIron")}; lid=base;
    ring.materials={material("BlackIron")};body=ring;
    std::string diagnostic;
    auto badBody=body;badBody.materials[0].emissiveTexture=3;
    check(!horde::vulkan::raytracing::ConfigureChapterTextureAliases(base,lid,ring,badBody,diagnostic),
          "unexpected map category cannot share a provider");
    check(base.materials[0].textureGroup==-1 && lid.materials[1].textureGroup==-1,
          "failed group admission cannot partially mutate preceding assets");
    badBody=body;badBody.materials[0].name="DifferentMetal";
    check(!horde::vulkan::raytracing::ConfigureChapterTextureAliases(base,lid,ring,badBody,diagnostic),
          "different authored material family cannot alias by accident");
    check(horde::vulkan::raytracing::ConfigureChapterTextureAliases(base,lid,ring,body,diagnostic),
          "admitted texture families receive explicit provider groups");
    check(base.materials[0].textureGroup==0 && base.materials[1].textureGroup==1 &&
          lid.materials[0].textureGroup==0 && lid.materials[1].textureGroup==1 &&
          ring.materials[0].textureGroup==0 && body.materials[0].textureGroup==0,
          "chest wood/iron and lantern iron keep distinct canonical groups");
    const std::array<std::uint8_t,12> duplicates{10,11,12,13,20,21,22,23,10,11,12,13};
    const std::array<horde::vulkan::raytracing::RtTextureLayerAlias,1> aliases{{{0,2}}};
    check(AppendRtTextureLayerSubset(duplicates,3,4,reorder,destination,aliases), "identical encoded layers alias");
    const auto aliasedSaved=destination;
    check(!AppendRtTextureLayerSubset(source,3,4,reorder,destination,aliases), "different encoded layers cannot alias");
    check(destination==aliasedSaved, "failed alias leaves payload unchanged");
    const std::array<horde::vulkan::raytracing::RtTextureLayerAlias,1> invalidAlias{{{0,3}}};
    check(!AppendRtTextureLayerSubset(duplicates,3,4,reorder,destination,invalidAlias), "alias bounds checked before access");
    const std::array<std::uint8_t,3> corruptLaterMip{1,2,3};
    check(!AppendRtTextureLayerSubset(corruptLaterMip,3,1,reorder,destination,aliases), "later mip disagreement cannot be hidden by mip zero equality");
    check(destination==aliasedSaved, "invalid alias and mip do not append partial data");
    destination=saved;
    const std::array<std::uint32_t, 2> badSelection{0,3};
    check(!AppendRtTextureLayerSubset(source, 3, 4, badSelection, destination), "reject invalid later layer before append");
    check(destination == saved, "invalid layer leaves destination unchanged");
    check(!AppendRtTextureLayerSubset(std::span<const std::uint8_t>(source).first(11), 3, 4, reorder, destination), "reject truncated full source");
    check(!AppendRtTextureLayerSubset(source, 3, 3, reorder, destination), "reject oversized full source");
    check(!AppendRtTextureLayerSubset(source, 0, 4, reorder, destination), "reject zero source layers");
    check(!AppendRtTextureLayerSubset(source, 3, 0, reorder, destination), "reject zero layer size");
    check(!AppendRtTextureLayerSubset(source, 3, 4, {}, destination), "reject empty admission");
    check(!AppendRtTextureLayerSubset(source, 3, std::numeric_limits<std::uint64_t>::max(), reorder, destination), "reject host size multiplication overflow");
    check(destination == saved, "all malformed payloads retain destination");
    // A separate small array adds terrain maps without repackaging every
    // existing texture. The exact encoded mip, format and colour metadata
    // must agree before any GPU upload is possible.
    const auto ktx=[](std::uint32_t layers) {
        std::vector<std::uint8_t> bytes(208+20*layers);
        const std::array<std::uint8_t,12> id{0xab,0x4b,0x54,0x58,0x20,0x32,0x30,0xbb,0x0d,0x0a,0x1a,0x0a};
        std::copy(id.begin(),id.end(),bytes.begin());
        const auto put32=[&](std::size_t o,std::uint32_t v){std::memcpy(bytes.data()+o,&v,4);};
        const auto put64=[&](std::size_t o,std::uint64_t v){std::memcpy(bytes.data()+o,&v,8);};
        for(const auto [o,v]:std::array<std::pair<std::size_t,std::uint32_t>,13>{{
            {12,37},{16,1},{20,2},{24,2},{28,0},{32,layers},{36,1},{40,2},
            {44,0},{48,128},{52,80},{56,0},{60,0}}})put32(o,v);
        put32(128,80);bytes[141]=1;bytes[142]=1;
        put64(80,208);put64(88,16*layers);put64(96,16*layers);
        put64(104,208+16*layers);put64(112,4*layers);put64(120,4*layers);
        for(std::size_t i=208;i<bytes.size();++i)bytes[i]=static_cast<std::uint8_t>(i);
        return bytes;
    };
    using horde::vulkan::raytracing::RtKtxArrayAppendView;
    using horde::vulkan::raytracing::PrepareRtKtxArrayAppend;
    auto baseKtx=ktx(18), extraKtx=ktx(2);
    RtKtxArrayAppendView appendView;
    check(PrepareRtKtxArrayAppend(baseKtx,extraKtx,37,2,2,15,17,appendView,diagnostic),"admit 15 selected plus two exact terrain layers");
    check(appendView.layers==2 && appendView.mipLevels==2 && appendView.mips[0].size()==32 &&
          appendView.mips[1].size()==8 && appendView.mips[0].data()==extraKtx.data()+208,
          "append borrows actual encoded mips without resampling or copies");
    check(PrepareRtKtxArrayAppend(baseKtx,extraKtx,37,2,2,16,18,appendView,diagnostic),"exact existing layer ceiling admitted");
    check(!PrepareRtKtxArrayAppend(baseKtx,extraKtx,37,2,2,17,19,appendView,diagnostic),"layer ceiling never expanded");
    const auto rejectAppend=[&](std::vector<std::uint8_t> bad) {
        RtKtxArrayAppendView sentinel;sentinel.layers=77;
        check(!PrepareRtKtxArrayAppend(baseKtx,bad,37,2,2,15,17,sentinel,diagnostic) && sentinel.layers==77,
              "malformed appended array rejects without partial publication");
    };
    for(const auto offset:{0u,12u,16u,20u,24u,28u,32u,36u,40u,44u,48u,52u,64u,72u,88u,112u,120u,142u}) {
        auto bad=extraKtx;bad[offset]^=1;rejectAppend(std::move(bad));
    }
    auto truncated=extraKtx;truncated.pop_back();rejectAppend(std::move(truncated));
    rejectAppend(std::vector<std::uint8_t>(79));
    auto overlap=extraKtx;const std::uint64_t overlapping=208;std::memcpy(overlap.data()+104,&overlapping,8);rejectAppend(std::move(overlap));
    check(!PrepareRtKtxArrayAppend(baseKtx,extraKtx,37,2,2,18,17,appendView,diagnostic),"addition cannot underflow final layer count");
    if(argc!=1 && argc!=3) {std::cerr<<"Expected optional base-array and append-array directories.\n";return 2;}
    if(argc==3) {
        const auto read=[](const std::filesystem::path& path) {
            std::ifstream file(path,std::ios::binary);
            return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(file),{});
        };
        for(const char* platform:{"windows","android"})for(const char* category:{"base-color","normal","orm"}) {
            const std::string name=std::string(category)+"."+platform+".ktx2";
            const bool color=std::string_view(category)=="base-color",normal=std::string_view(category)=="normal";
            const auto format=std::string_view(platform)=="windows"?(color?43u:37u):(color?166u:normal?157u:165u);
            const auto original=read(std::filesystem::path(argv[1])/name),addition=read(std::filesystem::path(argv[2])/name);
            RtKtxArrayAppendView view;
            const std::span<const std::uint32_t> chosen=color?
                std::span<const std::uint32_t>(horde::vulkan::raytracing::kChapterBaseColorSourceLayers):
                std::span<const std::uint32_t>(horde::vulkan::raytracing::kChapterNormalOrmSourceLayers);
            const bool valid=PrepareRtKtxArrayAppend(original,addition,format,1024,1024,
                static_cast<std::uint32_t>(chosen.size()),static_cast<std::uint32_t>(chosen.size()+2),view,diagnostic);
            check(valid,"actual Windows/ASTC array append admission");
            if(!valid){std::cerr<<name<<": "<<diagnostic<<'\n';continue;}
            std::size_t total=0;
            for(std::size_t mip=0;mip<view.mipLevels;++mip) {
                std::uint64_t offset,length;std::uint32_t layers;
                std::memcpy(&offset,original.data()+80+24*mip,8);std::memcpy(&length,original.data()+88+24*mip,8);
                std::memcpy(&layers,original.data()+32,4);
                // The ordinary startup scene precedes Android checkpoint
                // dispatch. Its upload must preserve every legacy encoded
                // texel/mip while excluding the appended forest families.
                const auto legacyChosen=color?legacy.baseColor:legacy.normalOrm;
                std::vector<std::uint8_t> legacyUpload;
                const auto originalMip=std::span<const std::uint8_t>(original).subspan(
                    static_cast<std::size_t>(offset),static_cast<std::size_t>(length));
                check(AppendRtTextureLayerSubset(originalMip,layers,length/layers,legacyChosen,legacyUpload),
                      "actual ordinary startup selects bounded Windows/ASTC layers");
                check(legacyUpload.size()==length/layers*legacyChosen.size() &&
                      std::equal(legacyUpload.begin(),legacyUpload.end(),originalMip.begin()),
                      "ordinary startup preserves byte-identical legacy mip prefix");
                std::vector<std::uint8_t> upload;
                check(AppendRtTextureLayerSubset(std::span<const std::uint8_t>(original).subspan(
                    static_cast<std::size_t>(offset),static_cast<std::size_t>(length)),layers,length/layers,chosen,upload,
                    horde::vulkan::raytracing::kChapterTextureAliases),"actual retained mip aliases remain byte-identical");
                const auto kept=upload;
                upload.insert(upload.end(),view.mips[mip].begin(),view.mips[mip].end());
                check(std::equal(kept.begin(),kept.end(),upload.begin()),"appended terrain cannot alter preceding encoded mips");
                total+=upload.size();
            }
            std::cout<<name<<": "<<chosen.size()+2<<" layers, "<<view.mipLevels<<" mips, "<<total<<" upload bytes\n";
        }
    }
    return passed ? 0 : 1;
}
