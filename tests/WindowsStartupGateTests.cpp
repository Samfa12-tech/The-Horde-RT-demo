#include "platform/windows/WindowsStartupGate.h"
#include <iostream>
using horde::platform::windows::WindowsStartupGate;
namespace {
int commands=0,destroyed=0,failures=0;
bool nested=false;
LRESULT CALLBACK Procedure(HWND window,UINT message,WPARAM wParam,LPARAM lParam) {
    LRESULT result=0;if(WindowsStartupGate::Handle(window,message,wParam,result))return result;
    if(message==WM_COMMAND){++commands;return 0;}
    if(message==WM_APP+1){nested=!WindowsStartupGate::AtBoundary(window,"nested");return 0;}
    if(message==WM_DESTROY){++destroyed;return 0;}
    return DefWindowProcA(window,message,wParam,lParam);
}
void Check(bool pass,const char* text){if(!pass){++failures;std::cerr<<text<<'\n';}}
}
int main(){
    const HWND foreground=GetForegroundWindow();
    WNDCLASSA type{};type.lpfnWndProc=Procedure;type.hInstance=GetModuleHandleA(nullptr);type.lpszClassName="HordeStartupGateFixture";
    if(!RegisterClassA(&type))return 2;
    HMENU menu=CreateMenu();AppendMenuA(menu,MF_STRING,101,"Old command");
    HWND window=CreateWindowA(type.lpszClassName,"hidden startup fixture",WS_OVERLAPPEDWINDOW,0,0,640,360,nullptr,menu,type.hInstance,nullptr);
    HWND button=CreateWindowA("BUTTON","Old controls",WS_CHILD|WS_VISIBLE,1,1,100,30,window,nullptr,type.hInstance,nullptr);
    HWND disabled=CreateWindowA("BUTTON","Initially disabled",WS_CHILD|WS_VISIBLE|WS_DISABLED,1,35,100,30,window,nullptr,type.hInstance,nullptr);
    WindowsStartupGate gate;
    Check(gate.Begin(window),"startup gate begins on the hidden owned window");
    WindowsStartupGate overlapping;Check(!overlapping.Begin(window),"an overlapping startup owner is rejected");
    Check(!IsWindowEnabled(button) && (GetMenuState(menu,0,MF_BYPOSITION)&MF_GRAYED),"old controls and menu disabled while loading");
    PostMessageA(window,WM_COMMAND,101,0);PostMessageA(window,WM_KEYDOWN,'R',0);
    Check(gate.Boundary("Loading assets") && commands==0,"queue service cannot invoke old gameplay/menu commands");
    HWND cover=GetWindow(window,GW_CHILD);RECT client{},bounds{};GetClientRect(window,&client);GetWindowRect(cover,&bounds);
    Check(cover!=button && cover!=disabled && bounds.right-bounds.left==client.right && bounds.bottom-bounds.top==client.bottom,
        "opaque loading surface covers the client above old controls");
    SendMessageA(window,WM_CLOSE,0,0);
    Check(gate.Cancelled() && IsWindow(window) && destroyed==0,"close requests cancellation without destroying an in-flight owner");
    Check(!gate.Boundary("next safe stage"),"cancel stops the next stage before it starts");
    gate.Finish();
    Check(!WindowsStartupGate::Current(window) && IsWindowEnabled(button) && !IsWindowEnabled(disabled),"cleanup restores exact previous control enablement");
    Check(gate.Begin(window) && gate.Boundary("Retry"),"a later retry has fresh cancellation ownership");gate.Finish();
    {WindowsStartupGate failed;Check(failed.Begin(window),"error fixture starts");}
    Check(!WindowsStartupGate::Current(window) && IsWindowEnabled(button),"error unwinding removes the loading owner and cover");
    Check(gate.Begin(window),"reentrancy fixture starts");PostMessageA(window,WM_APP+1,0,0);
    Check(!gate.Boundary("outer") && nested && gate.Cancelled(),"nested queue service fails closed rather than reentering initialization");gate.Finish();
    Check(gate.Begin(window),"quit fixture starts");PostQuitMessage(7);
    Check(!gate.Boundary("quit"),"WM_QUIT cancels startup");MSG quit{};
    Check(PeekMessageA(&quit,nullptr,WM_QUIT,WM_QUIT,PM_REMOVE) && quit.wParam==7,"startup preserves the original quit request");gate.Finish();
    Check(GetForegroundWindow()==foreground && !IsWindowVisible(window),"fixture never shows or takes owner foreground input");
    DestroyWindow(window);UnregisterClassA(type.lpszClassName,type.hInstance);
    std::cout<<"WindowsStartupGate failures="<<failures<<'\n';return failures?1:0;
}
