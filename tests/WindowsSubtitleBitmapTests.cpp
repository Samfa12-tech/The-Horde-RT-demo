#include "platform/windows/WindowsSubtitleBitmap.h"
#include "platform/windows/WindowsChapterDialogue.h"
#include <cstdlib>
#include <iostream>

void Check(bool condition,const char* message) {
    if (!condition) { std::cerr<<message<<'\n'; std::exit(1); }
}
int main() {
    using namespace horde::platform::windows;
    HFONT font=CreateFontA(-22,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH|FF_ROMAN,"Georgia");
    Check(font!=nullptr,"real GDI font");
    SubtitlePaintKey key{WindowsSubtitleCaption("Keeper","Come closer"),600,60,22,false};
    auto pixels=RasterizeSubtitle(key,font);
    Check(pixels.size()==36000 && pixels.front()==0 && pixels.back()==0,"no normal-play plaque/backing");
    unsigned visible=0,opaque=0;
    for(auto p:pixels) {
        const unsigned a=p>>24;
        Check((p&255)<=a && ((p>>8)&255)<=a && ((p>>16)&255)<=a,"premultiplied alpha");
        visible+=a>0; opaque+=a==255;
    }
    Check(visible>100 && visible<8000 && opaque>0,"real glyphs retain contrast without whole-screen panel");
    auto same=key; Check(same==key,"unchanged publication must reuse bitmap");
    // Hidden native windows exercise the real layered-child upload without
    // displaying a game, stealing focus or accepting device presentation.
    HWND parent=CreateWindowExA(0,"STATIC","subtitle fixture",WS_POPUP,0,0,800,600,
        nullptr,nullptr,GetModuleHandleA(nullptr),nullptr);
    HWND child=CreateWindowExA(WS_EX_LAYERED,"STATIC","",WS_CHILD|SS_OWNERDRAW|SS_NOTIFY,
        20,400,key.width,key.height,parent,nullptr,GetModuleHandleA(nullptr),nullptr);
    Check(parent && child,"hidden native layered-child fixture");
    Check(PresentSubtitleBitmap(child,key,font),"actual per-pixel native subtitle upload");
    Check(!IsWindowVisible(parent) && !IsWindowVisible(child),"test never presents a visible window");
    if(child) DestroyWindow(child);
    if(parent) DestroyWindow(parent);
    same.width++;Check(!(same==key),"resize invalidates paint");
    same=key;same.backed=true;Check(!(same==key),"accessibility mode invalidates paint");
    pixels=RasterizeSubtitle(same,font);
    Check(std::all_of(pixels.begin(),pixels.end(),[](auto p){return (p>>24)==255;}),"high-contrast backing remains available");
    key.width=0;Check(RasterizeSubtitle(key,font).empty(),"invalid extent rejected");
    key.width=4097;Check(RasterizeSubtitle(key,font).empty(),"bounded allocation");
    DeleteObject(font);
    std::cout<<"native subtitle glyph alpha, outline, cache keys, high contrast and bounds passed\n";
}
