#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <array>
#include <string>

namespace horde::platform::windows {
inline constexpr COLORREF kFlaskGaugeTransparent=RGB(1,2,3);
inline int FlaskGaugeFillTop(unsigned charges,unsigned capacity,int top,int bottom) {
    if((capacity!=2 && capacity!=3) || charges>capacity)return bottom;
    return bottom-static_cast<int>((bottom-top)*charges/capacity);
}
inline std::string FlaskGaugeAccessibleText(unsigned charges,unsigned capacity,bool controller,
                                           const std::string& hint) {
    return "Flask "+std::to_string(charges)+" of "+std::to_string(capacity)+
        (charges==0?", empty. ":charges==capacity?", full. ":", partly filled. ")+
        (controller?"D-pad Up: Drink.":"H: Drink.")+(hint.empty()?"":" "+hint);
}
// Ordinary native HUD painting only. The control's accessible text is separate
// from the tiny count/binding; outline and capacity divisions survive monochrome.
inline void PaintWindowsFlaskGauge(HDC dc,const RECT& bounds,unsigned charges,unsigned capacity,
                                  bool controller,const std::string& hint,int dpi,bool highContrast) {
    const auto px=[&](int value){return MulDiv(value,std::max(96,dpi),96);};
    HBRUSH clear=CreateSolidBrush(kFlaskGaugeTransparent);FillRect(dc,&bounds,clear);DeleteObject(clear);
    const COLORREF ink=highContrast?GetSysColor(COLOR_WINDOWTEXT):RGB(255,240,202);
    const COLORREF fill=highContrast?GetSysColor(COLOR_HIGHLIGHT):RGB(188,216,204);
    const int saved=SaveDC(dc);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,ink);
    HPEN pen=CreatePen(PS_SOLID,std::max(1,px(2)),ink);
    SelectObject(dc,pen);SelectObject(dc,GetStockObject(NULL_BRUSH));
    const auto point=[&](int x,int y){return POINT{bounds.left+px(x),bounds.top+px(y)};};
    const auto path=[&] {
        BeginPath(dc);auto p=point(12,5);MoveToEx(dc,p.x,p.y,nullptr);
        p=point(20,5);LineTo(dc,p.x,p.y);p=point(20,10);LineTo(dc,p.x,p.y);
        for(const auto segment: {std::array<POINT,3>{point(20,12),point(26,14),point(26,22)},
            std::array<POINT,3>{point(26,28),point(22,30),point(16,30)},
            std::array<POINT,3>{point(10,30),point(6,28),point(6,22)},
            std::array<POINT,3>{point(6,14),point(12,12),point(12,10)}})
            PolyBezierTo(dc,segment.data(),3);
        CloseFigure(dc);EndPath(dc);
    };
    // Clip liquid into the original bottle silhouette; thirds/halves refer to
    // its usable body height, not the neck or a permanently amber decoration.
    path();const int clip=SaveDC(dc);SelectClipPath(dc,RGN_AND);
    RECT liquid{bounds.left+px(6),bounds.top+FlaskGaugeFillTop(charges,capacity,px(10),px(30)),
        bounds.left+px(27),bounds.top+px(31)};
    HBRUSH liquidBrush=CreateSolidBrush(fill);if(charges>0)FillRect(dc,&liquid,liquidBrush);DeleteObject(liquidBrush);
    RestoreDC(dc,clip);path();StrokePath(dc);
    RECT cork{bounds.left+px(11),bounds.top+px(1),bounds.left+px(21),bounds.top+px(5)};
    HBRUSH corkBrush=CreateSolidBrush(ink);FillRect(dc,&cork,corkBrush);DeleteObject(corkBrush);
    for(unsigned slot=1;slot<capacity;++slot) {
        const int y=FlaskGaugeFillTop(slot,capacity,px(10),px(30));
        auto a=point(9,0),b=point(23,0);MoveToEx(dc,a.x,bounds.top+y,nullptr);LineTo(dc,b.x,bounds.top+y);
    }
    HFONT font=CreateFontA(-px(12),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,"Segoe UI");
    if(font)SelectObject(dc,font);
    RECT count{bounds.left+px(37),bounds.top+px(1),bounds.right,bounds.top+px(17)};
    const auto text=std::to_string(charges)+"/"+std::to_string(capacity);
    DrawTextA(dc,text.c_str(),-1,&count,DT_LEFT|DT_SINGLELINE|DT_NOPREFIX);
    RECT binding{bounds.left+px(37),bounds.top+px(18),bounds.right,bounds.top+px(35)};
    DrawTextA(dc,controller?"D-pad Up":"H",-1,&binding,DT_LEFT|DT_SINGLELINE|DT_NOPREFIX);
    if(!hint.empty()) {
        RECT message{bounds.left,bounds.top+px(38),bounds.right,bounds.bottom};
        DrawTextA(dc,hint.c_str(),-1,&message,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);
    }
    RestoreDC(dc,saved);if(font)DeleteObject(font);DeleteObject(pen);
}
}
