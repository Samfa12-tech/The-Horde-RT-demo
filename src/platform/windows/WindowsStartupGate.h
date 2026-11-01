#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
namespace horde::platform::windows {
// Owner-thread loading boundary only. Never pumps inside a Vulkan operation.
class WindowsStartupGate {
public:
    static constexpr int kCancelledResult=42;
    static WindowsStartupGate* Current(HWND window) {
        return static_cast<WindowsStartupGate*>(GetPropA(window,"Horde.StartupGate"));
    }
    bool Begin(HWND window) {
        if(active_ || !IsWindow(window) || Current(window))return false;
        window_=window;cancelled_=false;
        if(!SetPropA(window_,"Horde.StartupGate",this))return false;
        active_=true;menu_=GetMenu(window_);
        if(menu_)for(int i=0;i<GetMenuItemCount(menu_);++i){menuStates_.push_back(GetMenuState(menu_,i,MF_BYPOSITION));
            EnableMenuItem(menu_,i,MF_BYPOSITION|MF_GRAYED);}
        overlay_=CreateWindowExA(0,"STATIC","Starting Horde Lantern RT...",WS_CHILD|WS_VISIBLE|SS_CENTER|SS_NOPREFIX,
            0,0,1,1,window_,nullptr,GetModuleHandleA(nullptr),nullptr);
        if(!overlay_){Finish();return false;}
        SendMessageA(overlay_,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
        ProtectControls();Resize();return true;
    }
    ~WindowsStartupGate(){Finish();}
    WindowsStartupGate()=default;
    WindowsStartupGate(const WindowsStartupGate&)=delete;
    WindowsStartupGate& operator=(const WindowsStartupGate&)=delete;
    bool Cancelled() const{return cancelled_;}
    bool Active() const{return active_;}
    void RequestCancel(){cancelled_=true;}
    bool Boundary(std::string_view stage) {
        if(!active_)return !cancelled_;
        if(pumping_){cancelled_=true;return false;}
        if(cancelled_ || !IsWindow(window_)){cancelled_=true;return false;}
        pumping_=true;
        const std::string text="\r\nStarting Horde Lantern RT\r\n\r\n"+std::string(stage)+
            "\r\n\r\nEsc or close to cancel loading.";
        SetWindowTextA(overlay_,text.c_str());ProtectControls();Resize();UpdateWindow(window_);UpdateWindow(overlay_);
        MSG message{};bool quit=false;int quitCode=0;
        // Bounded queue service. HWND filtering includes this window's children.
        for(unsigned n=0;n<64 && PeekMessageA(&message,window_,0,0,PM_REMOVE);++n) {
            if(message.message==WM_QUIT){quit=true;quitCode=static_cast<int>(message.wParam);cancelled_=true;break;}
            TranslateMessage(&message);DispatchMessageA(&message);
            if(cancelled_)break;
        }
        pumping_=false;
        if(quit)PostQuitMessage(quitCode);
        return !cancelled_;
    }
    void Finish() {
        if(!active_)return;
        active_=false;
        if(IsWindow(window_) && Current(window_)==this)RemovePropA(window_,"Horde.StartupGate");
        if(IsWindow(overlay_))DestroyWindow(overlay_);overlay_=nullptr;
        for(const auto& child:children_)if(IsWindow(child.window))EnableWindow(child.window,child.enabled);
        children_.clear();
        if(IsMenu(menu_))for(unsigned i=0;i<menuStates_.size();++i)
            EnableMenuItem(menu_,i,MF_BYPOSITION|(menuStates_[i]&(MF_GRAYED|MF_DISABLED)));
        menuStates_.clear();menu_=nullptr;
        if(IsWindow(window_))InvalidateRect(window_,nullptr,TRUE);
    }
    static bool AtBoundary(HWND window,std::string_view stage) {
        auto* gate=Current(window);return !gate || gate->Boundary(stage);
    }
    static bool IsCancelled(HWND window){auto* gate=Current(window);return gate && gate->Cancelled();}
    static void Complete(HWND window){if(auto* gate=Current(window))gate->Finish();}
    static bool Handle(HWND window,UINT message,WPARAM wParam,LRESULT& result) {
        auto* gate=Current(window);if(!gate || !gate->active_)return false;
        result=0;
        if(message==WM_CLOSE || (message==WM_KEYDOWN && wParam==VK_ESCAPE) ||
            (message==WM_SYSKEYDOWN && wParam==VK_F4) ||
            (message==WM_SYSCOMMAND && (wParam&0xfff0)==SC_CLOSE)) {gate->RequestCancel();return true;}
        if(message==WM_SIZE){gate->Resize();return true;}
        switch(message) {
        case WM_COMMAND:case WM_NOTIFY:case WM_TIMER:case WM_HSCROLL:case WM_VSCROLL:case WM_MOUSEWHEEL:
        case WM_KEYDOWN:case WM_SYSKEYDOWN:case WM_KEYUP:case WM_SYSKEYUP:
        case WM_LBUTTONDOWN:case WM_LBUTTONUP:case WM_RBUTTONDOWN:case WM_RBUTTONUP:
        case WM_MOUSEMOVE:case WM_CAPTURECHANGED:case WM_KILLFOCUS:case WM_ACTIVATE:case WM_ACTIVATEAPP:
            return true;
        default:return false;
        }
    }
private:
    struct Child {HWND window;BOOL enabled;};
    void ProtectControls() {
        EnumChildWindows(window_,[](HWND child,LPARAM parameter)->BOOL {
            auto& gate=*reinterpret_cast<WindowsStartupGate*>(parameter);
            if(child==gate.overlay_)return TRUE;
            if(std::none_of(gate.children_.begin(),gate.children_.end(),[&](const Child& old){return old.window==child;}))
                gate.children_.push_back({child,IsWindowEnabled(child)});
            EnableWindow(child,FALSE);return TRUE;
        },reinterpret_cast<LPARAM>(this));
    }
    void Resize(){if(IsWindow(overlay_)){RECT bounds{};GetClientRect(window_,&bounds);
        SetWindowPos(overlay_,HWND_TOP,0,0,bounds.right,bounds.bottom,SWP_NOACTIVATE);}}
    HMENU menu_=nullptr;std::vector<UINT> menuStates_;
    HWND window_=nullptr,overlay_=nullptr;bool active_=false,cancelled_=false,pumping_=false;
    std::vector<Child> children_;
};
}
