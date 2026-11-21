#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
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
        gameplay::campaign::TreasurePurse purse{};
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
        parent_=parent;world_=view.worldGeneration;surface_=view.surfaceGeneration;objective_=view.objective;
        purse_=view.purse;
        const bool lookout = objective_ == Objective::Lookout;
        const auto body=gameplay::campaign::PauseObjectiveText(objective_,purse_);
        const std::array<const char*,5> labels{lookout?Title:"BELLWETHER DIRECTIONS",
            body.c_str(),"CONTINUE EXPLORING",
            lookout?"RETURN TO MENU":nullptr,lookout?"BACK":"BACK TO PAUSE MENU"};
        for(std::size_t i=0;i<controls_.size();++i) {
            if (!labels[i]) continue;
            const bool button=i>=2;
            controls_[i]=CreateWindowExA(0,button?"BUTTON":"STATIC",labels[i],
                WS_CHILD|(button?WS_TABSTOP|BS_OWNERDRAW:SS_CENTER|SS_NOPREFIX),0,0,1,1,parent,
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
        parent_=nullptr;
    }
    bool Visible() const {return visible_;}
    bool Refresh(const View& view) {
        if(!visible_)return false;
        if(!Admitted(view) || view.worldGeneration!=world_ || view.surfaceGeneration!=surface_ ||
            view.objective!=objective_ || view.purse!=purse_) {
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
    void Layout(RECT bounds,int dpi=96) const {
        if(!visible_)return;
        const auto px=[&](int n){return MulDiv(n,std::max(96,dpi),96);};
        const int availableWidth=std::max(1L,bounds.right-bounds.left);
        const int availableHeight=std::max(1L,bounds.bottom-bounds.top);
        const int width=std::min(px(540),availableWidth);
        const int titleHeight=px(42),bodyHeight=px(140),gap=px(10);
        const int buttons = objective_==Objective::Lookout ? 3 : 2;
        const int gaps = buttons + 2;
        const int buttonHeight=std::min(px(44),std::max(px(24),(availableHeight-titleHeight-bodyHeight-gaps*gap)/buttons));
        const int total=titleHeight+bodyHeight+buttons*buttonHeight+gaps*gap;
        const int x=bounds.left+(availableWidth-width)/2;
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
    HWND parent_=nullptr;
    std::array<HWND,5> controls_{};
    bool visible_=false;
    std::uint64_t world_=0,surface_=0;
    gameplay::campaign::TreasurePurse purse_{};
    Objective objective_=Objective::None;
};
} // namespace horde::platform::windows
