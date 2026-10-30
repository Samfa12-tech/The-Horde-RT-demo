#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <algorithm>
#include <bit>
#include <cstring>
#include "vulkan/raytracing/RtSceneAbi.generated.h"

namespace horde::vulkan::raytracing
{
// Views borrow the caller's immutable KTX bytes through staging upload.
struct RtKtxArrayAppendView
{
    std::uint32_t layers = 0;
    std::uint32_t mipLevels = 0;
    std::array<std::span<const std::uint8_t>,16> mips{};
};
inline bool PrepareRtKtxArrayAppend(
    std::span<const std::uint8_t> base, std::span<const std::uint8_t> addition,
    std::uint32_t format, std::uint32_t width, std::uint32_t height,
    std::uint32_t selectedBaseLayers, std::uint32_t totalLayers,
    RtKtxArrayAppendView& result, std::string& diagnostic)
{
    diagnostic="KTX append requires matching bounded arrays, colour metadata and complete nonoverlapping mips.";
    if(!width || !height || width>32768 || height>32768 || !selectedBaseLayers ||
       totalLayers<=selectedBaseLayers || totalLayers>kRtTextureLayerCapacity) return false;
    const bool rgba=format==37 || format==43;
    const bool astc4=format==157 || format==158;
    const bool astc6=format==165 || format==166;
    if(!rgba && !astc4 && !astc6) return false;
    const auto parse=[&](std::span<const std::uint8_t> bytes, RtKtxArrayAppendView& view,
                         std::span<const std::uint8_t>& dfd) {
        constexpr std::array<std::uint8_t,12> id{0xab,0x4b,0x54,0x58,0x20,0x32,0x30,0xbb,0x0d,0x0a,0x1a,0x0a};
        if(bytes.size()<80 || !std::equal(id.begin(),id.end(),bytes.begin()))return false;
        const auto read32=[&](std::size_t offset){std::uint32_t v;std::memcpy(&v,bytes.data()+offset,4);return v;};
        const auto read64=[&](std::size_t offset){std::uint64_t v;std::memcpy(&v,bytes.data()+offset,8);return v;};
        view.layers=read32(32);view.mipLevels=read32(40);
        if(read32(12)!=format || read32(16)!=1 || read32(20)!=width || read32(24)!=height ||
           read32(28)!=0 || !view.layers || view.layers>kRtTextureLayerCapacity || read32(36)!=1 ||
           view.mipLevels!=std::bit_width(std::max(width,height)) || view.mipLevels>view.mips.size() ||
           read32(44)!=0 || read64(64)!=0 || read64(72)!=0)return false;
        const std::size_t headerEnd=80+24*view.mipLevels;
        if(bytes.size()<headerEnd)return false;
        const auto range=[&](std::uint64_t start,std::uint64_t size) {
            return start>=headerEnd && start<=bytes.size() && size<=bytes.size()-start;
        };
        const std::size_t dfdStart=read32(48),dfdSize=read32(52);
        const std::size_t kvdStart=read32(56),kvdSize=read32(60);
        if(dfdSize<16 || dfdStart%4 || !range(dfdStart,dfdSize) ||
           (kvdSize && (kvdStart%4 || !range(kvdStart,kvdSize))))return false;
        const auto overlaps=[](std::uint64_t a,std::uint64_t n,std::uint64_t b,std::uint64_t m) {
            return n && m && a<b+m && b<a+n;
        };
        if(overlaps(dfdStart,dfdSize,kvdStart,kvdSize))return false;
        dfd=bytes.subspan(dfdStart,dfdSize);
        if(read32(dfdStart)!=dfdSize)return false;
        std::array<std::array<std::uint64_t,2>,16> intervals{};
        for(std::uint32_t mip=0;mip<view.mipLevels;++mip) {
            const auto offset=read64(80+24*mip),length=read64(88+24*mip);
            const std::uint64_t w=std::max(1u,width>>mip),h=std::max(1u,height>>mip);
            const std::uint64_t block=astc4?4:6;
            const auto layerBytes=rgba?w*h*4:((w+block-1)/block)*((h+block-1)/block)*16;
            if(length!=layerBytes*view.layers || read64(96+24*mip)!=length ||
               offset%(rgba?4:16) || !range(offset,length) ||
               overlaps(offset,length,dfdStart,dfdSize) || overlaps(offset,length,kvdStart,kvdSize))return false;
            for(std::uint32_t previous=0;previous<mip;++previous)
                if(overlaps(offset,length,intervals[previous][0],intervals[previous][1]))return false;
            intervals[mip]={offset,length};
            view.mips[mip]=bytes.subspan(static_cast<std::size_t>(offset),static_cast<std::size_t>(length));
        }
        return true;
    };
    RtKtxArrayAppendView original,appended;
    std::span<const std::uint8_t> originalDfd,appendedDfd;
    if(!parse(base,original,originalDfd) || !parse(addition,appended,appendedDfd) ||
       selectedBaseLayers>original.layers || appended.layers!=totalLayers-selectedBaseLayers ||
       originalDfd.size()!=appendedDfd.size() ||
       !std::equal(originalDfd.begin(),originalDfd.end(),appendedDfd.begin()))return false;
    result=appended;
    diagnostic.clear();
    return true;
}
}
