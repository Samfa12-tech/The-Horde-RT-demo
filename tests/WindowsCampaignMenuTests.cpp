#include "platform/windows/WindowsCampaignMenu.h"
#include <iostream>
#include <vector>

namespace cp=horde::gameplay::campaign;
using Menu=horde::platform::windows::WindowsCampaignMenu;
namespace {
int checks=0,failures=0;
WNDPROC originalChoiceProcedure=nullptr;
unsigned choiceTextChanges=0,choiceEnableChanges=0;
LRESULT CALLBACK CountChoiceChanges(HWND window,UINT message,WPARAM w,LPARAM l) {
    if(message==WM_SETTEXT)++choiceTextChanges;
    if(message==WM_ENABLE)++choiceEnableChanges;
    return CallWindowProcA(originalChoiceProcedure,window,message,w,l);
}
void Check(bool value,const char* message){++checks;if(!value){++failures;std::cerr<<"FAIL: "<<message<<'\n';}}
LRESULT CALLBACK ParentProc(HWND window,UINT message,WPARAM w,LPARAM l) {
    auto menu=reinterpret_cast<Menu*>(GetWindowLongPtrA(window,GWLP_USERDATA));
    if(message==WM_COMMAND && menu && menu->HandleCommand(LOWORD(w)))return 0;
    return DefWindowProcA(window,message,w,l);
}
void Click(HWND parent,int id){SendMessageA(parent,WM_COMMAND,MAKEWPARAM(id,BN_CLICKED),reinterpret_cast<LPARAM>(GetDlgItem(parent,id)));}
std::string Text(HWND parent,int id){char text[1024]{};GetWindowTextA(GetDlgItem(parent,id),text,sizeof(text));return text;}
bool Enabled(HWND parent,int id){return IsWindowEnabled(GetDlgItem(parent,id))!=FALSE;}
bool Shown(HWND parent,int id){return (GetWindowLongPtrA(GetDlgItem(parent,id),GWL_STYLE)&WS_VISIBLE)!=0;}
cp::CatalogueEntry Record(int hearts=2,bool upper=false) {
    cp::Checkpoint c;c.vitality=hearts;
    if(upper){c.id=cp::CheckpointId::SafeExit;c.zone=horde::gameplay::simulation::WorldZoneId::TombExterior;
        c.position=horde::gameplay::traversal::kExteriorLanding;c.firstAscentCompleted=true;}
    return {cp::CatalogueStatus::Valid,c,{}};
}
}
int main() {
    // UI/CPU evidence only: never show/activate an owner window, touch desktop
    // input, poll devices, play audio, access save paths or claim RT presentation.
    WNDCLASSA klass{};klass.lpfnWndProc=ParentProc;klass.hInstance=GetModuleHandleA(nullptr);
    klass.lpszClassName="HordeHiddenCampaignMenuTest";
    if(!RegisterClassA(&klass))return 1;
    const auto foreground=GetForegroundWindow();
    HWND parent=CreateWindowExA(0,klass.lpszClassName,"hidden campaign fixture",WS_OVERLAPPEDWINDOW,
        0,0,800,700,nullptr,nullptr,klass.hInstance,nullptr);
    if(!parent)return 1;
    std::vector<Menu::Command> commands;
    {
        Menu menu([&](Menu::Command command){commands.push_back(command);});
        SetWindowLongPtrA(parent,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(&menu));
        Check(menu.Create(parent),"actual child controls created");
        RECT bounds{0,0,760,660};menu.Layout(bounds);
        Menu::View view;view.phase=Menu::Phase::Ready;view.state.hasLatestCheckpoint=true;
        for(auto& entry:view.catalogue)entry.status=cp::CatalogueStatus::Empty;
        view.catalogue[0]=Record();view.catalogue[1].status=cp::CatalogueStatus::Checking;
        view.catalogue[2].status=cp::CatalogueStatus::Invalid;
        view.catalogue[3].status=cp::CatalogueStatus::ReadError;
        view.catalogue[5]=Record(1,true);
        menu.Refresh(view);menu.Open(Menu::Page::Load,Menu::Origin::Entry);menu.Layout(bounds);
        const auto choice=GetDlgItem(parent,4110);
        originalChoiceProcedure=reinterpret_cast<WNDPROC>(SetWindowLongPtrA(choice,GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(CountChoiceChanges)));
        for(unsigned frame=0;frame<120;++frame)menu.Refresh(view);
        Check(choiceTextChanges==0 && choiceEnableChanges==0,
            "stable catalogue polling does not erase/repaint or re-enable identical slot controls");
        SetWindowLongPtrA(choice,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(originalChoiceProcedure));
        Check(!IsWindowVisible(parent) && menu.Visible(),"owner remains hidden while chooser is logically open");
        Check(Text(parent,4110).find("Lower claimed | Hearts 2/3")!=std::string::npos,"real occupied label carries checkpoint and hearts");
        Check(Text(parent,4111).find("Checking")!=std::string::npos && !Enabled(parent,4111),"checking load disabled");
        Check(Text(parent,4112).find("Invalid - preserved")!=std::string::npos && !Enabled(parent,4112),"invalid record preserved");
        Check(Text(parent,4113).find("Unreadable - preserved")!=std::string::npos && !Enabled(parent,4113),"read error preserved");
        Check(Text(parent,4114).find("Empty")!=std::string::npos && !Enabled(parent,4114),"empty load disabled");
        Check(Text(parent,4115).find("Safe exit | Hearts 1/3")!=std::string::npos,"upper latest auto label carries actual checkpoint facts");
        Click(parent,4113);Check(commands.empty(),"programmatic invalid choice cannot dispatch");
        Click(parent,4115);Check(commands.size()==1 && commands.back().action==Menu::Action::Load &&
            commands.back().slot==2 && commands.back().kind==cp::SaveKind::Autosave,"sixth native command selects slot three latest auto");
        auto forest=Record(2,true);
        forest.checkpoint->id=cp::CheckpointId::ForestEndpoint;forest.checkpoint->contentRevision=2;
        forest.checkpoint->zone=horde::gameplay::simulation::WorldZoneId::Lookout;
        const auto endpoint=horde::gameplay::simulation::kWorldRoutePoints.back();
        forest.checkpoint->position={endpoint.x,endpoint.y,endpoint.z};
        auto& story=forest.checkpoint->dialogue;
        story.rescueConsumed=story.reunionStarted=story.proofAccepted=true;
        story.forestNightConsumed=story.forestWaystoneConsumed=story.forestClueConsumed=story.forestVillageConsumed=true;
        view.catalogue[5]=forest;++view.catalogueRevision;menu.Refresh(view);commands.clear();
        Check(Enabled(parent,4115) && Text(parent,4115).find("Forest lookout | Hearts 2/3")!=std::string::npos,
            "forest checkpoint is an independently named loadable autosave");
        Click(parent,4115);Check(commands.size()==1 && commands.back().action==Menu::Action::Load &&
            commands.back().slot==2 && commands.back().kind==cp::SaveKind::Autosave,"forest choice retains exact slot and record kind");
        commands.clear();menu.Open(Menu::Page::Save,Menu::Origin::Pause);
        Check(!Enabled(parent,4111) && Enabled(parent,4112),"manual invalid slot blocked; independent empty manual allowed");
        Click(parent,4110);Check(commands.empty() && Shown(parent,Menu::ConfirmId),"occupied manual requires explicit confirmation");
        Click(parent,Menu::BackId);Check(commands.empty() && !Shown(parent,Menu::ConfirmId),"cancel confirmation performs no save or owner cancel");
        Click(parent,4110);++view.catalogueRevision;menu.Refresh(view);
        Click(parent,Menu::ConfirmId);Check(commands.empty() && !Shown(parent,Menu::ConfirmId),"catalogue revision revokes stale confirmation");
        Click(parent,4110);Click(parent,Menu::ConfirmId);
        Check(commands.size()==1 && commands.back().action==Menu::Action::SaveManual && commands.back().slot==0,
            "confirmed manual action uses actual native command");
        commands.clear();Click(parent,4112);
        Check(commands.size()==1 && commands.back().action==Menu::Action::SaveManual && commands.back().slot==2,
            "empty manual slot dispatches directly without touching latest auto");
        commands.clear();menu.Open(Menu::Page::NewGame,Menu::Origin::Entry);
        Check(!Enabled(parent,4110),"new game waits for both record checks");
        view.catalogue[1].status=cp::CatalogueStatus::Empty;++view.catalogueRevision;menu.Refresh(view);
        Click(parent,4110);Check(commands.empty() && Text(parent,Menu::StatusId).find("Manual saves are kept")!=std::string::npos,
            "occupied new game explains manual preservation and latest auto replacement");
        Click(parent,Menu::ConfirmId);Check(commands.size()==1 && commands.back().action==Menu::Action::BeginNewGame,
            "new game requires explicit occupied confirmation");
        commands.clear();menu.Open(Menu::Page::Load,Menu::Origin::Death);
        for(auto phase:{Menu::Phase::Saving,Menu::Phase::Reading,Menu::Phase::Preparing,Menu::Phase::Rebase,Menu::Phase::Presenting}) {
            view.phase=phase;menu.Refresh(view);
            Check(!Enabled(parent,4110) && !Enabled(parent,Menu::RefreshId),"pending phase disables record actions");
            Click(parent,4110);Click(parent,Menu::RefreshId);Check(commands.empty(),"pending synthetic clicks cannot bypass disabled controls");
            Click(parent,Menu::BackId);Check(commands.size()==1 && commands.back().action==Menu::Action::Cancel,"pending Back dispatches owner cancellation");commands.clear();
        }
        view.phase=Menu::Phase::Failed;view.message="Injected write failure";view.writeUncertain=true;
        menu.Refresh(view);menu.Open(Menu::Page::Save,Menu::Origin::Pause);
        Check(Text(parent,Menu::StatusId).find("Injected write failure")!=std::string::npos &&
            Text(parent,Menu::StatusId).find("uncertain")!=std::string::npos && !Enabled(parent,4110),"uncertain failure remains visible and forbids blind write");
        Click(parent,Menu::RefreshId);Check(commands.size()==1 && commands.back().action==Menu::Action::Refresh,"read-only recheck dispatched after failure");
        commands.clear();Click(parent,Menu::BackId);Check(commands.size()==1 && commands.back().action==Menu::Action::Close,"precommit failure permits close");
        commands.clear();view.phase=Menu::Phase::Presenting;view.message.clear();view.writeUncertain=false;
        view.state.operation=cp::PlayOperation::Load;view.state.outcome=cp::PlayOutcome::Succeeded;
        view.canResumeLoaded=false;menu.Refresh(view);menu.Open(Menu::Page::Load,Menu::Origin::Death);
        Click(parent,Menu::ResumeId);Check(commands.empty() && !Shown(parent,Menu::ResumeId),"logical load success cannot resume before owner RT barrier");
        view.phase=Menu::Phase::Succeeded;menu.Refresh(view);Click(parent,Menu::BackId);
        Check(commands.empty() && !Enabled(parent,Menu::BackId),"postcommit result cannot close and silently resume");
        view.canResumeLoaded=true;menu.Refresh(view);Click(parent,Menu::ResumeId);
        Check(commands.size()==1 && commands.back().action==Menu::Action::Resume,"owner-confirmed frame enables explicit Resume only");
        menu.Close();Check(!menu.Visible() && !Shown(parent,4110) && !menu.HandleCommand(4110),"closed menu rejects command dispatch");
        view.phase=Menu::Phase::Ready;view.canResumeLoaded=false;menu.Refresh(view);
        menu.Open(Menu::Page::Save,Menu::Origin::Pause);commands.clear();Click(parent,4112);
        Check(commands.size()==1 && commands.back().action==Menu::Action::SaveManual && !Shown(parent,Menu::ResumeId),
            "historical load success cannot disable a later manual save chooser");
        Check(!IsWindowVisible(parent) && GetForegroundWindow()==foreground,"hidden fixture never changes foreground window");
        SetWindowLongPtrA(parent,GWLP_USERDATA,0);
    }
    DestroyWindow(parent);UnregisterClassA(klass.lpszClassName,klass.hInstance);
    std::cout<<checks<<" Windows campaign menu checks, "<<failures<<" failures; hidden HWND UI/CPU only, no RT presentation claim\n";
    return failures?1:0;
}
