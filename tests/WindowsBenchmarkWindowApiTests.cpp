#include "platform/windows/WindowsBenchmarkWindowApi.h"
#include <iostream>
int main() {
    using horde::platform::windows::BenchmarkWindowApiTrace;
    bool passed=true;
    auto check=[&](bool value,const char* message){if(!value){passed=false;std::cerr<<message<<'\n';}};
    HWND before=GetForegroundWindow();
    HWND window=CreateWindowExA(0,"STATIC","Horde hidden benchmark fixture",WS_OVERLAPPEDWINDOW,
        0,0,400,300,nullptr,nullptr,GetModuleHandleA(nullptr),nullptr);
    if(!window)return 1;
    const auto style=GetWindowLongPtrA(window,GWL_STYLE),exStyle=GetWindowLongPtrA(window,GWL_EXSTYLE);
    RECT original{};GetWindowRect(window,&original);
    BenchmarkWindowApiTrace api;
    api.Observe("raw SetMenu absent",[&]{return SetMenu(window,nullptr);});
    std::cout<<api.text<<'\n';
    check(api.SetWindowMenu(window,nullptr),"absent menu detaches successfully");
    check(api.SetStyle(window,GWL_STYLE,WS_POPUP),"borderless style applies");
    check(api.SetStyle(window,GWL_EXSTYLE,0),"zero prior extended style is valid");
    check(api.Observe("SetWindowPos",[&]{return SetWindowPos(window,nullptr,0,0,1280,720,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_FRAMECHANGED);}).first!=FALSE,"fixed placement applies");
    RECT client{};check(GetClientRect(window,&client) && client.right==1280 && client.bottom==720,"actual fixed client matches");
    check(api.SetStyle(window,GWL_STYLE,style),"original style restores");
    check(api.SetStyle(window,GWL_EXSTYLE,exStyle),"original extended style restores");
    check(api.SetWindowMenu(window,nullptr),"original menu restores");
    check(SetWindowPos(window,nullptr,original.left,original.top,original.right-original.left,original.bottom-original.top,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_FRAMECHANGED)!=FALSE,"original placement restores");
    RECT restored{};check(GetWindowRect(window,&restored) && EqualRect(&restored,&original),"restored rectangle exact");
    BenchmarkWindowApiTrace invalid;
    check(!invalid.SetStyle(nullptr,GWL_STYLE,WS_POPUP),"invalid handle style fails closed");
    check(invalid.text.find("error=1400")!=std::string::npos,"immediate invalid-window error retained");
    check(!invalid.SetWindowMenu(nullptr,nullptr),"invalid handle menu fails closed");
    check(!IsWindowVisible(window) && GetForegroundWindow()==before,"fixture never shows or takes foreground");
    DestroyWindow(window);return passed?0:1;
}
