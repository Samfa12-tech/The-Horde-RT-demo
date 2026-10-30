#include "platform/windows/WindowsSubtitleBitmap.h"
#include "platform/windows/WindowsChapterDialogue.h"
#include "platform/windows/WindowsControlPresentation.h"
#include "platform/windows/WindowsInteractionPrompt.h"
#include <cstdlib>
#include <iostream>

void Check(bool condition,const char* message) {
    if (!condition) { std::cerr<<message<<'\n'; std::exit(1); }
}
namespace {
WNDPROC nativeStaticProcedure=nullptr;
unsigned textMessages=0,visibilityMessages=0;
LRESULT CALLBACK CountNativeChanges(HWND window,UINT message,WPARAM w,LPARAM l) {
    if(message==WM_SETTEXT)++textMessages;
    if(message==WM_SHOWWINDOW)++visibilityMessages;
    return CallWindowProcA(nativeStaticProcedure,window,message,w,l);
}
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
    // A narrow 504px client fixture leaves 24px each side for the prompt.
    HWND stable=CreateWindowExA(0,"STATIC","",WS_CHILD|SS_CENTER,0,0,456,34,parent,
        nullptr,GetModuleHandleA(nullptr),nullptr);
    Check(stable!=nullptr,"hidden HUD control for native mutation regression");
    nativeStaticProcedure=reinterpret_cast<WNDPROC>(SetWindowLongPtrA(stable,GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(CountNativeChanges)));
    Check(SetNativeTextIfChanged(stable,"E: RAISE LANTERN"),"changed accessible caption is published");
    Check(SetNativeVisibilityIfChanged(stable,true),"first visibility transition published");
    const auto initialVisibilityMessages=visibilityMessages;
    for(int frame=0;frame<120;++frame) {
        Check(!SetNativeTextIfChanged(stable,"E: RAISE LANTERN"),"unchanged HUD publication must not erase/repaint");
        Check(!SetNativeVisibilityIfChanged(stable,true),"unchanged visible HUD does not repeat ShowWindow");
    }
    Check(textMessages==1 && visibilityMessages==initialVisibilityMessages,
        "120 stable frames produce no extra native text/visibility mutation messages");
    Check(SetNativeTextIfChanged(stable,"" ) && textMessages==2,"clearing a cue still updates the accessible caption");
    Check(SetNativeVisibilityIfChanged(stable,false),"cue removal still hides the control");
    Check(!IsWindowVisible(parent),"native stability regression never activates or displays a parent");
    HFONT captionFont=CreateFontA(18,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH|FF_SWISS,"Segoe UI");
    Check(captionFont!=nullptr,"actual HUD caption font is available");
    HDC dc=GetDC(stable);auto previousFont=SelectObject(dc,captionFont);
    for(bool controller:{false,true}) {
        const auto prompt=WindowsRaiseLanternPromptText(controller,true);SIZE extent{};
        GetTextExtentPoint32A(dc,prompt.data(),static_cast<int>(prompt.size()),&extent);
        Check(extent.cx<=456,"combined instruction fits the narrow fixture using the real HUD font");
        Check(prompt.find("RAISE LANTERN")!=std::string_view::npos && prompt.find("DESCEND")!=std::string_view::npos,
            "fitting text preserves both actions");
    }
    SelectObject(dc,previousFont);ReleaseDC(stable,dc);DeleteObject(captionFont);DestroyWindow(stable);
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
