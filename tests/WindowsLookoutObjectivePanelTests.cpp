#include "platform/windows/WindowsLookoutObjectivePanel.h"
#include <iostream>
#include <string>
#include <vector>

using Panel=horde::platform::windows::WindowsLookoutObjectivePanel;
namespace {
int checks=0,failures=0;
void Check(bool value,const char* message) {++checks;if(!value){++failures;std::cerr<<"FAIL: "<<message<<'\n';}}
struct ForegroundEvidence {
    HWND initial=GetForegroundWindow(),parent=nullptr,previous=initial;
    bool ownerObserved=false,ownerVisible=false,changed=false;
    HWND Observe(const char* stage,int command=0) {
        const auto current=GetForegroundWindow();
        ownerObserved|=parent && current==parent;
        ownerVisible|=parent && IsWindowVisible(parent);
        changed|=current!=previous;
        std::cerr<<"HORDE_LOOKOUT_HIDDEN_FOREGROUND stage="<<stage<<" command="<<command
            <<" initial="<<initial<<" previous="<<previous<<" current="<<current<<" parent="<<parent
            <<" ours="<<(parent && current==parent)<<" visible="<<(parent && IsWindowVisible(parent))
            <<" observed_owner_foreground="<<ownerObserved<<'\n';
        if(current!=previous && current!=parent && previous!=parent)
            std::cerr<<"HORDE_LOOKOUT_HIDDEN_FOREGROUND changed_external_hwnd=1 cause=unattributed\n";
        previous=current;return current;
    }
};
struct Harness {
    Panel panel;Panel::View view{true,true,7,11};std::vector<Panel::Action> actions;
    ForegroundEvidence* foreground=nullptr;
};
LRESULT CALLBACK ParentProc(HWND window,UINT message,WPARAM w,LPARAM l) {
    auto* h=reinterpret_cast<Harness*>(GetWindowLongPtrA(window,GWLP_USERDATA));
    if(message==WM_COMMAND && h && Panel::Handles(LOWORD(w))) {
        if(h->foreground)h->foreground->Observe("before-command",LOWORD(w));
        if(const auto action=h->panel.Consume(LOWORD(w),reinterpret_cast<HWND>(l),h->view))h->actions.push_back(*action);
        if(h->foreground)h->foreground->Observe("after-command",LOWORD(w));
        return 0;
    }
    return DefWindowProcA(window,message,w,l);
}
std::string Text(HWND parent,int id) {char text[1024]{};GetWindowTextA(GetDlgItem(parent,id),text,sizeof(text));return text;}
void Click(HWND parent,int id) {
    // Exercise the production native notification receiver with its real source
    // HWND, without BM_CLICK's simulated mouse/focus behavior on a hidden owner.
    SendMessageA(parent,WM_COMMAND,MAKEWPARAM(id,BN_CLICKED),reinterpret_cast<LPARAM>(GetDlgItem(parent,id)));
}
void CheckBounds(HWND parent,RECT bounds) {
    int bottom=bounds.top;
    for(int id=Panel::TitleId;id<=Panel::BackId;++id) {
        const auto control=GetDlgItem(parent,id);
        if(!control)continue;
        RECT r{};GetWindowRect(control,&r);MapWindowPoints(HWND_DESKTOP,parent,reinterpret_cast<POINT*>(&r),2);
        Check(r.left>=bounds.left && r.right<=bounds.right && r.top>=bottom && r.bottom<=bounds.bottom && r.right>r.left && r.bottom>r.top,
            "real panel controls fit their bounded client area without overlap");bottom=r.bottom;
    }
}
}
int main() {
    // Hidden HWND/CPU evidence only. No foreground/focus/input acquisition,
    // RT launch, devices, audio, campaign files or settings writes.
    ForegroundEvidence foreground;foreground.Observe("before-create");
    WNDCLASSA klass{};klass.lpfnWndProc=ParentProc;klass.hInstance=GetModuleHandleA(nullptr);
    klass.lpszClassName="HordeHiddenLookoutObjectiveTest";
    if(!RegisterClassA(&klass))return 1;
    HWND parent=CreateWindowExA(WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW,klass.lpszClassName,"hidden lookout fixture",WS_OVERLAPPEDWINDOW,
        0,0,1100,1200,nullptr,nullptr,klass.hInstance,nullptr);
    if(!parent)return 1;
    foreground.parent=parent;foreground.Observe("after-create");
    Check((GetWindowLongPtrA(parent,GWL_EXSTYLE)&(WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW))==(WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW),
        "hidden parent explicitly prohibits activation and taskbar ownership");
    {
        Harness h;h.foreground=&foreground;SetWindowLongPtrA(parent,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(&h));
        const auto open=[&](const Panel::View& view) {
            foreground.Observe("before-open");const bool opened=h.panel.Open(parent,view);
            foreground.Observe("after-open");return opened;
        };
        Check(!h.panel.Visible() && !h.panel.Refresh(h.view) && !GetDlgItem(parent,Panel::ContinueId),
            "eligibility refresh never automatically opens a panel or covers the world");
        for(unsigned invalid=0;invalid<4;++invalid) {
            auto view=h.view;if(invalid==0)view.available=false;if(invalid==1)view.active=false;
            if(invalid==2)view.worldGeneration=0;if(invalid==3)view.surfaceGeneration=0;
            Check(!open(view),"unavailable/inactive/unbound owner cannot explicitly open panel");
        }
        Check(open(h.view) && !IsWindowVisible(parent),"explicit open creates native controls without showing owner");
        Check(Text(parent,Panel::TitleId)==Panel::Title && Text(parent,Panel::BodyId)==Panel::Body &&
            Text(parent,Panel::BodyId).find("The tavern is across the square")!=std::string::npos,
            "native objective directs the player to the open Bellwether road");
        Check(Text(parent,Panel::ContinueId)=="CONTINUE EXPLORING" && Text(parent,Panel::MenuId)=="RETURN TO MENU" &&
            Text(parent,Panel::BackId)=="BACK","all explicit native actions have accessible text");
        auto actions=h.panel.ActionControls();
        for(std::size_t i=0;i<actions.size();++i)Check(GetDlgCtrlID(actions[i])==Panel::ContinueId+static_cast<int>(i) &&
            (GetWindowLongPtrA(actions[i],GWL_STYLE)&WS_TABSTOP)!=0 &&
            (GetWindowLongPtrA(actions[i],GWL_STYLE)&WS_VISIBLE)!=0,
            "keyboard/controller action order exposes visible native tab stops");
        RECT normal{12,12,508,548};h.panel.Layout(normal,96);CheckBounds(parent,normal);
        RECT large{24,24,1016,1096};h.panel.Layout(large,192);CheckBounds(parent,large);
        foreground.Observe("after-layout");
        Check(!h.panel.Consume(Panel::MenuId,parent,h.view) && !h.panel.Consume(Panel::MenuId,nullptr,h.view) && h.panel.Visible(),
            "forged or missing native action source cannot return to menu");
        EnableWindow(actions[1],FALSE);
        Check(!h.panel.Consume(Panel::MenuId,actions[1],h.view) && h.panel.Visible(),"disabled native action cannot dispatch");
        EnableWindow(actions[1],TRUE);
        foreground.Observe("after-forged-disabled-actions");
        const auto oldMenu=actions[1];Click(parent,Panel::ContinueId);
        Check(h.actions.size()==1 && h.actions.back()==Panel::Action::ContinueExploring && !h.panel.Visible() &&
            !GetDlgItem(parent,Panel::MenuId),"native Continue dispatches once and retires opening controls");
        Check(open(h.view),"player can explicitly reopen the objective");
        SendMessageA(parent,WM_COMMAND,MAKEWPARAM(Panel::MenuId,BN_CLICKED),reinterpret_cast<LPARAM>(oldMenu));
        Check(h.actions.size()==1 && h.panel.Visible(),"queued command from retired opening cannot choose on reopened panel");
        Click(parent,Panel::BackId);
        Check(h.actions.size()==2 && h.actions.back()==Panel::Action::ContinueExploring && !h.panel.Visible(),
            "native Back uses the same safe continuation action as Escape/controller cancel");
        for(unsigned invalid=0;invalid<4;++invalid) {
            Check(open(h.view),"revocation fixture opens explicitly");
            const auto stale=GetDlgItem(parent,Panel::MenuId);const auto previous=h.view;
            if(invalid==0)++h.view.worldGeneration;if(invalid==1)++h.view.surfaceGeneration;
            if(invalid==2)h.view.available=false;if(invalid==3)h.view.active=false;
            SendMessageA(parent,WM_COMMAND,MAKEWPARAM(Panel::MenuId,BN_CLICKED),reinterpret_cast<LPARAM>(stale));
            Check(h.actions.size()==2 && !h.panel.Visible(),"changed world/surface/eligibility/lifecycle revokes queued action");
            h.view=previous;h.panel.Refresh(h.view);
            Check(!h.panel.Visible(),"restored eligibility cannot reopen a revoked panel automatically");
        }
        Check(open(h.view),"final explicit opening remains available");Click(parent,Panel::MenuId);
        Check(h.actions.size()==3 && h.actions.back()==Panel::Action::ReturnToMenu && !h.panel.Visible(),
            "only current native Return to menu emits its distinct action");
        h.view.objective=Panel::Objective::BellwetherRegister;
        Check(open(h.view) && Text(parent,Panel::TitleId)=="BELLWETHER DIRECTIONS" &&
            Text(parent,Panel::BodyId)==horde::gameplay::campaign::PauseObjectiveText(h.view.objective) &&
            !GetDlgItem(parent,Panel::MenuId),"the Bellwether page shows earned directions and offers pause navigation");
        Click(parent,Panel::BackId);
        Check(h.actions.size()==4 && h.actions.back()==Panel::Action::BackToPause && !h.panel.Visible(),
            "Bellwether Back preserves the pause menu");
        Check(open(h.view),"the directions can reopen without changing progress");
        const HWND oldDirections=GetDlgItem(parent,Panel::ContinueId);
        h.view.objective=Panel::Objective::BellwetherExpeditions;
        SendMessageA(parent,WM_COMMAND,MAKEWPARAM(Panel::ContinueId,BN_CLICKED),reinterpret_cast<LPARAM>(oldDirections));
        Check(h.actions.size()==4 && !h.panel.Visible(),"new earned directions revoke the previous page's controls");
        h.view.purse={3,true,false};
        Check(open(h.view) && Text(parent,Panel::BodyId).find("future expeditions")!=std::string::npos &&
            Text(parent,Panel::BodyId).find("Purse: 3 coins.")!=std::string::npos &&
            Text(parent,Panel::BodyId).find("Lantern fitting at the tavern: 2 coins.")!=std::string::npos &&
            Text(parent,Panel::BodyId).find("restore its binding")!=std::string::npos,
            "expedition directions show the purse, fitting price and first ward step");
        h.view.ward=horde::gameplay::campaign::AirWardDirections::Learned;
        Check(!h.panel.Refresh(h.view) && open(h.view) &&
              Text(parent,Panel::BodyId).find("Air ward learned")!=std::string::npos,
            "the reopened directions reflect the earned ward lesson");
        Click(parent,Panel::ContinueId);
        h.view.ward=horde::gameplay::campaign::AirWardDirections::RestoreBinding;
        h.view.mill=horde::gameplay::bellwether::MillPreparation::RigTrialReady;
        h.view.shorePracticeAvailable=true;
        h.view.shorePracticeCompleted=true;
        const auto font=CreateFontA(18,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,
            ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
            DEFAULT_PITCH|FF_DONTCARE,"Segoe UI");
        Check(font && h.panel.Open(parent,h.view,font),"combined directions open with the native menu font");
        RECT narrow{16,16,480,504};
        h.panel.Layout(narrow,96);
        CheckBounds(parent,narrow);
        const auto body=GetDlgItem(parent,Panel::BodyId);
        RECT bodyBounds{};
        GetClientRect(body,&bodyBounds);
        const auto dc=GetDC(body);
        const auto previous=SelectObject(dc,font);
        auto measured=bodyBounds;
        const auto text=Text(parent,Panel::BodyId);
        const auto required=DrawTextA(dc,text.c_str(),static_cast<int>(text.size()),&measured,
            DT_CALCRECT|DT_WORDBREAK|DT_CENTER|DT_NOPREFIX);
        SelectObject(dc,previous);
        ReleaseDC(body,dc);
        Check(required>0 && bodyBounds.bottom>=required && text.find("Shore practice: Complete.")!=std::string::npos,
            "combined directions fit the complete shore, rig and ward text in a narrow window");
        h.panel.Close();
        DeleteObject(font);
        SetWindowLongPtrA(parent,GWLP_USERDATA,0);
    }
    foreground.Observe("before-destroy");
    DestroyWindow(parent);UnregisterClassA(klass.lpszClassName,klass.hInstance);
    const auto finalForeground=foreground.Observe("after-destroy");
    Check(!foreground.ownerObserved && !foreground.ownerVisible,
        "test parent is never shown or observed as foreground at sampled open/command/teardown boundaries");
    Check(!foreground.changed && finalForeground==foreground.initial,
        "foreground continuity holds; an external HWND change is recorded separately with unattributed cause");
    std::cout<<checks<<" Windows lookout panel checks, "<<failures<<" failures\n";return failures?1:0;
}
