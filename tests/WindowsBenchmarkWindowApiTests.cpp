#include "platform/windows/WindowsBenchmarkWindowApi.h"
#include <iostream>
namespace {
bool rejectStyle=false;
LRESULT CALLBACK StyleCallback(HWND window,UINT message,WPARAM w,LPARAM l) {
    if(message==WM_STYLECHANGING && rejectStyle) {
        auto* styles=reinterpret_cast<STYLESTRUCT*>(l);
        styles->styleNew=styles->styleOld;
    }
    const auto result=DefWindowProcA(window,message,w,l);
    if(message==WM_STYLECHANGED)SetLastError(ERROR_FILE_NOT_FOUND);
    return result;
}
}
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
    std::cout<<api.text<<'\n';
    RECT client{};check(GetClientRect(window,&client) && client.right==1280 && client.bottom==720,"actual fixed client matches");
    check(api.SetStyle(window,GWL_STYLE,style),"original style restores");
    check(api.SetStyle(window,GWL_EXSTYLE,exStyle),"original extended style restores");
    check(api.SetWindowMenu(window,nullptr),"original menu restores");
    check(SetWindowPos(window,nullptr,original.left,original.top,original.right-original.left,original.bottom-original.top,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_FRAMECHANGED)!=FALSE,"original placement restores");
    RECT restored{};check(GetWindowRect(window,&restored) && EqualRect(&restored,&original),"restored rectangle exact");
    WNDCLASSA cls{};cls.lpfnWndProc=StyleCallback;cls.hInstance=GetModuleHandleA(nullptr);
    cls.lpszClassName="HordeBenchmarkStyleCallbackFixture";
    check(RegisterClassA(&cls)!=0,"callback fixture registered");
    HWND callbackWindow=CreateWindowExA(0,cls.lpszClassName,"hidden",WS_POPUP,
        0,0,100,100,nullptr,nullptr,cls.hInstance,nullptr);
    check(callbackWindow!=nullptr,"callback fixture created");
    BenchmarkWindowApiTrace callback;
    check(callback.SetStyle(callbackWindow,GWL_EXSTYLE,WS_EX_TOOLWINDOW),
        "zero previous style with callback LastError still succeeds on exact verified readback");
    check(callback.text.find("return=0 error=2")!=std::string::npos,
        "fixture reproduces real callback-clobbered LastError, not a generic setter success");
    rejectStyle=true;
    check(!callback.SetStyle(callbackWindow,GWL_EXSTYLE,WS_EX_APPWINDOW),
        "callback-rejected style fails even when Win32 call returns successfully");
    rejectStyle=false;
    check(!IsWindowVisible(callbackWindow),"callback fixture stays hidden");
    DestroyWindow(callbackWindow);UnregisterClassA(cls.lpszClassName,cls.hInstance);
    BenchmarkWindowApiTrace invalid;
    check(!invalid.SetStyle(nullptr,GWL_STYLE,WS_POPUP),"invalid handle style fails closed");
    check(invalid.text.find("error=1400")!=std::string::npos,"immediate invalid-window error retained");
    check(!invalid.SetWindowMenu(nullptr,nullptr),"invalid handle menu fails closed");
    check(!IsWindowVisible(window) && GetForegroundWindow()==before,"fixture never shows or takes foreground");
    DestroyWindow(window);return passed?0:1;
}
