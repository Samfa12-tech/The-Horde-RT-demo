#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace horde::platform::windows {
struct SubtitlePaintKey {
    std::string text;
    int width=0,height=0,fontPixels=0;
    bool backed=false;
    bool operator==(const SubtitlePaintKey& other) const {
        return text==other.text && width==other.width && height==other.height &&
            fontPixels==other.fontPixels && backed==other.backed;
    }
};

// Native HUD only. A grayscale GDI glyph mask becomes premultiplied BGRA with
// a small dark outline. Unoccupied pixels stay transparent over RT presentation.
inline std::vector<std::uint32_t> RasterizeSubtitle(const SubtitlePaintKey& key, HFONT font) {
    if (key.width<=0 || key.height<=0 || key.width>4096 || key.height>4096 || !font)
        return {};
    BITMAPINFO info{};
    info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=key.width; info.bmiHeader.biHeight=-key.height;
    info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32;
    info.bmiHeader.biCompression=BI_RGB;
    void* bits=nullptr;
    HDC dc=CreateCompatibleDC(nullptr);
    if (!dc) return {};
    HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);
    if (!bitmap) { DeleteDC(dc); return {}; }
    auto previousBitmap=SelectObject(dc,bitmap);
    auto previousFont=SelectObject(dc,font);
    const auto count=static_cast<std::size_t>(key.width)*key.height;
    std::memset(bits,0,count*sizeof(std::uint32_t));
    SetBkMode(dc,TRANSPARENT); SetTextColor(dc,RGB(255,255,255));
    RECT textRect{12,8,key.width-12,key.height-8};
    DrawTextA(dc,key.text.c_str(),-1,&textRect,DT_CENTER|DT_WORDBREAK|DT_NOPREFIX);
    GdiFlush();
    const auto* mask=static_cast<const std::uint32_t*>(bits);
    std::vector<std::uint32_t> pixels(count);
    const auto coverage=[&](int x,int y) {
        const auto p=mask[static_cast<std::size_t>(y)*key.width+x];
        return std::max({p&255u,(p>>8)&255u,(p>>16)&255u});
    };
    const COLORREF foreground=key.backed?GetSysColor(COLOR_WINDOWTEXT):RGB(248,236,207);
    const COLORREF background=GetSysColor(COLOR_WINDOW);
    for (int y=0;y<key.height;++y) for (int x=0;x<key.width;++x) {
        const auto a=coverage(x,y);
        unsigned edge=0;
        for (int dy=-1;dy<=1;++dy) for (int dx=-1;dx<=1;++dx)
            edge=std::max(edge,coverage(std::clamp(x+dx,0,key.width-1),std::clamp(y+dy,0,key.height-1)));
        const unsigned alpha=key.backed?255u:a+(edge*210u/255u)*(255u-a)/255u;
        const auto channel=[&](unsigned fg,unsigned bg) {
            return (fg*a+(key.backed?bg*(255u-a):0u))/255u;
        };
        pixels[static_cast<std::size_t>(y)*key.width+x]=(alpha<<24)|
            (channel(GetRValue(foreground),GetRValue(background))<<16)|
            (channel(GetGValue(foreground),GetGValue(background))<<8)|
            channel(GetBValue(foreground),GetBValue(background));
    }
    SelectObject(dc,previousFont); SelectObject(dc,previousBitmap);
    DeleteObject(bitmap); DeleteDC(dc);
    return pixels;
}

inline bool PresentSubtitleBitmap(HWND window,const SubtitlePaintKey& key,HFONT font) {
    auto pixels=RasterizeSubtitle(key,font);
    if (pixels.empty()) return false;
    BITMAPINFO info{}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=key.width; info.bmiHeader.biHeight=-key.height;
    info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
    HDC dc=CreateCompatibleDC(nullptr);
    if (!dc) return false;
    void* bits=nullptr;
    HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);
    if (!bitmap) { DeleteDC(dc); return false; }
    auto previous=SelectObject(dc,bitmap);
    std::memcpy(bits,pixels.data(),pixels.size()*sizeof(std::uint32_t));
    SIZE size{key.width,key.height}; POINT source{0,0};
    BLENDFUNCTION blend{AC_SRC_OVER,0,255,AC_SRC_ALPHA};
    const bool ok=UpdateLayeredWindow(window,nullptr,nullptr,&size,dc,&source,0,&blend,ULW_ALPHA)!=FALSE;
    SelectObject(dc,previous); DeleteObject(bitmap); DeleteDC(dc);
    return ok;
}
} // namespace horde::platform::windows
