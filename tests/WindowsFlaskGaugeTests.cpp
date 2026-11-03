#include "platform/windows/WindowsFlaskGauge.h"
#include <iostream>
#include <fstream>
using namespace horde::platform::windows;
int main(int argc,char** argv) {
    int failures=0,checks=0;
    const auto check=[&](bool ok,const char* message){++checks;if(!ok){++failures;std::cerr<<message<<'\n';}};
    for(int dpi:{96,144,192,288})for(unsigned capacity:{2u,3u})for(unsigned charges=0;charges<=capacity;++charges) {
        const int width=MulDiv(240,dpi,96),height=MulDiv(76,dpi,96);
        HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO info{};
        info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;
        info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
        void* pixels=nullptr;HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
        const auto previous=SelectObject(dc,bitmap);RECT bounds{0,0,width,height};
        PaintWindowsFlaskGauge(dc,bounds,charges,capacity,false,"",dpi,false);GdiFlush();
        const auto px=[&](int v){return MulDiv(v,dpi,96);};
        const COLORREF liquid=RGB(188,216,204);
        // Probe a vertical interior column away from outline and segment ticks.
        // Whole-body fill, lower half and both thirds are physical GDI pixels.
        for(int y:{14,19,23,27}) {
            const int scaledY=px(y),top=FlaskGaugeFillTop(charges,capacity,px(10),px(30));
            bool divider=false;
            for(unsigned slot=1;slot<capacity;++slot)
                divider|=std::abs(scaledY-FlaskGaugeFillTop(slot,capacity,px(10),px(30)))<=px(2);
            if(divider)continue;
            check((GetPixel(dc,px(16),scaledY)==liquid)==(charges>0 && scaledY>=top),
                "actual offscreen bottle pixels disagree with charge fraction");
        }
        check(GetPixel(dc,px(2),px(2))==kFlaskGaugeTransparent,"persistent gauge has no opaque surrounding plate");
        const auto name=FlaskGaugeAccessibleText(charges,capacity,false,"");
        check(name.find(std::to_string(charges)+" of "+std::to_string(capacity))!=std::string::npos &&
            name.find("H: Drink")!=std::string::npos,"accessible charge/binding state is retained");
        if(charges==0)check(name.find("empty")!=std::string::npos,"empty remains textual as well as unfilled");
        if(charges==capacity)check(name.find("full")!=std::string::npos,"full remains textual as well as filled");
        SelectObject(dc,previous);DeleteObject(bitmap);DeleteDC(dc);
    }
    check(FlaskGaugeFillTop(1,2,0,60)==30 && FlaskGaugeFillTop(1,3,0,60)==40 &&
        FlaskGaugeFillTop(2,3,0,60)==20,"half/thirds use actual capacity rather than static amber decoration");
    check(FlaskGaugeAccessibleText(1,3,true,"Interrupted.").find("D-pad Up")!=std::string::npos,
        "controller binding retains accessible action");
    if(argc==2) {
        // Optional private contact sheet from the actual production GDI painter,
        // never a concept render or a game/RT screenshot.
        constexpr int width=7*130,height=76;HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO info{};
        info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;
        info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
        info.bmiHeader.biCompression=BI_RGB;void* pixels=nullptr;
        HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);auto old=SelectObject(dc,bitmap);
        int tile=0;
        for(unsigned capacity:{2u,3u})for(unsigned charges=0;charges<=capacity;++charges) {
            RECT r{tile*130,0,(tile+1)*130,height};
            PaintWindowsFlaskGauge(dc,r,charges,capacity,false,"",96,false);++tile;
        }
        GdiFlush();BITMAPFILEHEADER file{};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(info.bmiHeader);
        file.bfSize=file.bfOffBits+width*height*4;
        std::ofstream out(argv[1],std::ios::binary);out.write(reinterpret_cast<const char*>(&file),sizeof(file));
        out.write(reinterpret_cast<const char*>(&info.bmiHeader),sizeof(info.bmiHeader));
        out.write(static_cast<const char*>(pixels),width*height*4);check(bool(out),"private GDI bitmap export succeeds");
        SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
    }
    std::cout<<"Windows flask bitmap checks="<<checks<<" failures="<<failures<<'\n';return failures?1:0;
}
