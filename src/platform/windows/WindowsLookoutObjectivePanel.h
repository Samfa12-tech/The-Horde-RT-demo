#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstdint>
#include <optional>
#include <string>
#include "gameplay/campaign/LookoutObjective.h"

namespace horde::platform::windows {
// Native presentation only. The owner supplies the shared gameplay eligibility;
// this panel never advances, resets, loads or saves a campaign.
class WindowsLookoutObjectivePanel {
public:
    using Objective = gameplay::campaign::PauseObjective;
    enum class Action { ContinueExploring, ReturnToMenu, BackToPause };
    struct View {
        bool available=false,active=false;
        std::uint64_t worldGeneration=0,surfaceGeneration=0;
        Objective objective=Objective::Lookout;
        gameplay::campaign::BellwetherDirections directions{};
    };
    static constexpr int TitleId=4320,BodyId=4321,ContinueId=4322,MenuId=4323,BackId=4324;
    static constexpr const char* Title="THE ROAD AHEAD";
    static constexpr const char* Body=gameplay::campaign::kLookoutObjectiveText;
    ~WindowsLookoutObjectivePanel() { Close(); }
    WindowsLookoutObjectivePanel()=default;
    WindowsLookoutObjectivePanel(const WindowsLookoutObjectivePanel&)=delete;
    WindowsLookoutObjectivePanel& operator=(const WindowsLookoutObjectivePanel&)=delete;

    bool Open(HWND parent,const View& view,HFONT font=nullptr) {
        if(visible_ || !IsWindow(parent) || !Admitted(view))return false;
        world_=view.worldGeneration;surface_=view.surfaceGeneration;objective_=view.objective;
        directions_=view.directions;
        const bool lookout = objective_ == Objective::Lookout;
        const auto bodyText=gameplay::campaign::PauseObjectiveText(objective_,directions_);
        const std::array<const char*,5> labels{lookout?Title:"BELLWETHER DIRECTIONS",
            bodyText.c_str(),"CONTINUE EXPLORING",
            lookout?"RETURN TO MENU":nullptr,lookout?"BACK":"BACK TO PAUSE MENU"};
        for(std::size_t i=0;i<controls_.size();++i) {
            if (!labels[i]) continue;
            const bool button=i>=2;
            const bool scrollBody=i==1 && !lookout;
            const DWORD style=button ? WS_TABSTOP|BS_OWNERDRAW :
                scrollBody ? ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL : SS_CENTER|SS_NOPREFIX;
            controls_[i]=CreateWindowExA(0,button?"BUTTON":scrollBody?"EDIT":"STATIC",labels[i],
                WS_CHILD|style,0,0,1,1,parent,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(TitleId+static_cast<int>(i))),
                GetModuleHandleA(nullptr),nullptr);
            if(!controls_[i]) {Close();return false;}
            const auto uiFont=font?font:static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            SendMessageA(controls_[i],WM_SETFONT,reinterpret_cast<WPARAM>(uiFont),FALSE);
        }
        visible_=true;
        for(auto control:controls_)if(control)ShowWindow(control,SW_SHOWNOACTIVATE);
        return true;
    }
    void Close() {
        visible_=false;world_=surface_=0;
        // Each opening owns distinct native controls. A queued notification from
        // a retired opening cannot choose an action on a later opening.
        for(auto& control:controls_) {if(control && IsWindow(control))DestroyWindow(control);control=nullptr;}
        scrollable_=false;
        wheelRemainder_=0;
    }
    bool Visible() const {return visible_;}
    bool Refresh(const View& view) {
        if(!visible_)return false;
        if(!Admitted(view) || view.worldGeneration!=world_ || view.surfaceGeneration!=surface_ ||
            view.objective!=objective_ || view.directions!=directions_) {
            Close();return false;
        }
        return true;
    }
    static bool Handles(int id) {return id>=ContinueId && id<=BackId;}
    std::optional<Action> Consume(int id,HWND source,const View& view) {
        if(!Handles(id) || !Refresh(view))return {};
        const auto index=static_cast<std::size_t>(id-TitleId);
        if(!source || source!=controls_[index] || !IsWindow(source) || !IsWindowEnabled(source))return {};
        const auto result=id==MenuId?Action::ReturnToMenu:
            id==BackId && objective_!=Objective::Lookout?Action::BackToPause:Action::ContinueExploring;
        Close();return result;
    }
    std::array<HWND,3> ActionControls() const {return {controls_[2],controls_[3],controls_[4]};}
    bool ScrollPage(bool down) const {
        if(!visible_ || !scrollable_)return false;
        SendMessageA(controls_[1],EM_SCROLL,down?SB_PAGEDOWN:SB_PAGEUP,0);
        return true;
    }
    bool ScrollWheel(int delta) {
        if(!visible_ || !scrollable_)return false;
        wheelRemainder_+=delta;
        const int steps=wheelRemainder_/WHEEL_DELTA;
        wheelRemainder_%=WHEEL_DELTA;
        if(!steps)return true;
        UINT lines=3;
        SystemParametersInfoA(SPI_GETWHEELSCROLLLINES,0,&lines,0);
        if(lines==WHEEL_PAGESCROLL) {
            for(int page=0;page<std::abs(steps);++page)ScrollPage(steps<0);
        } else {
            const auto lineCount=static_cast<UINT>(SendMessageA(controls_[1],EM_GETLINECOUNT,0,0));
            SendMessageA(controls_[1],EM_LINESCROLL,0,-steps*static_cast<int>(std::min(lines,lineCount)));
        }
        return true;
    }
    void Layout(RECT bounds,int dpi=96) {
        if(!visible_)return;
        const auto px=[&](int n){return MulDiv(n,std::max(96,dpi),96);};
        const int availableWidth=std::max(1L,bounds.right-bounds.left);
        const int availableHeight=std::max(1L,bounds.bottom-bounds.top);
        const int width=std::min(px(540),availableWidth);
        const int titleHeight=std::min(px(42),std::max(1,availableHeight/4));
        const int gap=std::min(px(10),availableHeight/25);
        const int buttons=objective_==Objective::Lookout?3:2;
        const int buttonHeight=std::clamp(availableHeight/(buttons+3),1,px(44));
        const int gaps=buttons+2;
        const int maxBody=std::max(1,availableHeight-titleHeight-buttons*buttonHeight-gaps*gap);
        const int x=bounds.left+(availableWidth-width)/2;
        int desiredBody=px(objective_==Objective::BellwetherExpeditions?228:140);
        if(objective_!=Objective::Lookout) {
            const auto body=controls_[1];
            // Measure the native EDIT wrap at the final width, including its vertical scrollbar.
            MoveWindow(body,x,bounds.top,width,maxBody,FALSE);
            SendMessageA(body,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,MAKELPARAM(px(8),px(8)));
            RECT formatting{};
            SendMessageA(body,EM_GETRECT,0,reinterpret_cast<LPARAM>(&formatting));
            const int visibleHeight=std::max(1L,formatting.bottom-formatting.top);
            int requiredHeight=0;
            if(const HDC dc=GetDC(body)) {
                const auto font=reinterpret_cast<HFONT>(SendMessageA(body,WM_GETFONT,0,0));
                const auto previousFont=font?SelectObject(dc,font):nullptr;
                TEXTMETRICA metrics{};
                if(GetTextMetricsA(dc,&metrics))
                    requiredHeight=metrics.tmHeight*static_cast<int>(SendMessageA(body,EM_GETLINECOUNT,0,0));
                if(previousFont)SelectObject(dc,previousFont);
                ReleaseDC(body,dc);
            }
            desiredBody=std::max(desiredBody,requiredHeight+std::max(0,maxBody-visibleHeight)+px(8));
            const bool overflow=requiredHeight>visibleHeight;
            if(scrollable_!=overflow) {
                scrollable_=overflow;
                wheelRemainder_=0;
                SetWindowTextA(controls_[0],overflow?
                    "BELLWETHER DIRECTIONS\r\nLeft / right to scroll":"BELLWETHER DIRECTIONS");
            }
        }
        const int bodyHeight=std::min(desiredBody,maxBody);
        const int total=titleHeight+bodyHeight+buttons*buttonHeight+gaps*gap;
        int y=bounds.top+std::max(0,(availableHeight-total)/2);
        MoveWindow(controls_[0],x,y,width,titleHeight,TRUE);y+=titleHeight+gap;
        MoveWindow(controls_[1],x,y,width,bodyHeight,TRUE);y+=bodyHeight+2*gap;
        for(auto control:ActionControls()) if(control) {MoveWindow(control,x,y,width,buttonHeight,TRUE);y+=buttonHeight+gap;}
    }
private:
    static bool Admitted(const View& view) {
        return view.available && view.active && view.worldGeneration && view.surfaceGeneration &&
            view.objective!=Objective::None;
    }
    std::array<HWND,5> controls_{};
    bool visible_=false,scrollable_=false;
    int wheelRemainder_=0;
    std::uint64_t world_=0,surface_=0;
    gameplay::campaign::BellwetherDirections directions_{};
    Objective objective_=Objective::None;
};
} // namespace horde::platform::windows
