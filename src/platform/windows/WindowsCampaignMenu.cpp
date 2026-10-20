#include "platform/windows/WindowsCampaignMenu.h"

#include <utility>

namespace horde::platform::windows {
namespace {
namespace cp=gameplay::campaign;
constexpr std::array<int,12> ids{WindowsCampaignMenu::TitleId,WindowsCampaignMenu::StatusId,
    4110,4111,4112,4113,4114,4115,WindowsCampaignMenu::ConfirmId,
    WindowsCampaignMenu::BackId,WindowsCampaignMenu::RefreshId,WindowsCampaignMenu::ResumeId};
std::string RecordLabel(const cp::CatalogueEntry& entry) {
    switch(entry.status) {
    case cp::CatalogueStatus::Checking:return "Checking";
    case cp::CatalogueStatus::Empty:return "Empty";
    case cp::CatalogueStatus::Invalid:return "Invalid - preserved";
    case cp::CatalogueStatus::ReadError:return "Unreadable - preserved";
    case cp::CatalogueStatus::Valid:
        if(entry.checkpoint && cp::Validate(*entry.checkpoint))
            return std::string(entry.checkpoint->id==cp::CheckpointId::ForestEndpoint?"Forest lookout":
                entry.checkpoint->id==cp::CheckpointId::SafeExit?"Safe exit":"Lower claimed")+
                " | Hearts "+std::to_string(entry.checkpoint->vitality)+"/3";
        return "Invalid - preserved";
    }
    return "Unavailable";
}
bool Valid(const cp::CatalogueEntry& entry) {
    return entry.status==cp::CatalogueStatus::Valid && entry.checkpoint && cp::Validate(*entry.checkpoint);
}
}
WindowsCampaignMenu::WindowsCampaignMenu(Dispatch dispatch):dispatch_(std::move(dispatch)){}
WindowsCampaignMenu::~WindowsCampaignMenu(){for(auto control:controls_)if(control && IsWindow(control) && GetParent(control)==parent_)DestroyWindow(control);}
void WindowsCampaignMenu::SetDispatch(Dispatch dispatch){dispatch_=std::move(dispatch);}
bool WindowsCampaignMenu::Create(HWND parent) {
    if(parent_ || !parent || !IsWindow(parent))return false;
    parent_=parent;
    for(std::size_t i=0;i<controls_.size();++i) {
        const bool label=i<2;
        controls_[i]=CreateWindowExA(0,label?"STATIC":"BUTTON","",
            WS_CHILD|(label?SS_LEFT:BS_PUSHBUTTON|BS_MULTILINE|WS_TABSTOP),0,0,0,0,parent,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ids[i])),GetModuleHandleA(nullptr),nullptr);
        if(!controls_[i]) {
            for(auto& control:controls_){if(control)DestroyWindow(control);control=nullptr;}
            parent_=nullptr;return false;
        }
        SendMessageA(controls_[i],WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),FALSE);
    }
    Render();return true;
}
void WindowsCampaignMenu::Open(Page page,Origin origin){page_=page;origin_=origin;confirmation_.reset();
    loadInMenu_=view_.state.operation==cp::PlayOperation::Load && Pending();visible_=true;Render();}
void WindowsCampaignMenu::Close(){visible_=false;loadInMenu_=false;confirmation_.reset();Render();}
bool WindowsCampaignMenu::Pending() const {
    return view_.state.outcome==cp::PlayOutcome::Pending || view_.phase==Phase::Saving || view_.phase==Phase::Reading ||
        view_.phase==Phase::Preparing || view_.phase==Phase::Rebase || view_.phase==Phase::Presenting;
}
bool WindowsCampaignMenu::CommittedLoad() const {
    return loadInMenu_ && view_.state.operation==cp::PlayOperation::Load && view_.state.outcome==cp::PlayOutcome::Succeeded;
}
bool WindowsCampaignMenu::CanChoose(unsigned index) const {
    if(!visible_ || Pending() || view_.phase==Phase::Unavailable || CommittedLoad())return false;
    if(view_.writeUncertain && page_!=Page::Load)return false;
    if(page_==Page::Load)return index<6 && Valid(view_.catalogue[index]);
    if(index>=3)return false;
    if(page_==Page::NewGame) return view_.catalogue[index*2].status!=cp::CatalogueStatus::Checking &&
        view_.catalogue[index*2+1].status!=cp::CatalogueStatus::Checking;
    const auto& entry=view_.catalogue[index*2];
    return view_.state.hasLatestCheckpoint && (entry.status==cp::CatalogueStatus::Empty || Valid(entry));
}
bool WindowsCampaignMenu::NeedsConfirmation(unsigned index) const {
    if(page_==Page::Save)return Valid(view_.catalogue[index*2]);
    return page_==Page::NewGame && (view_.catalogue[index*2].status!=cp::CatalogueStatus::Empty ||
                                  view_.catalogue[index*2+1].status!=cp::CatalogueStatus::Empty);
}
void WindowsCampaignMenu::Send(Action action,unsigned slot,cp::SaveKind kind) {
    // Copy the callable: owner callbacks may synchronously Refresh/Close this UI.
    const auto dispatch=dispatch_;if(dispatch)dispatch({action,slot,kind});
}
bool WindowsCampaignMenu::HandleCommand(int id) {
    if(!visible_)return false;
    if(id>=FirstChoiceId && id<FirstChoiceId+6) {
        const auto index=static_cast<unsigned>(id-FirstChoiceId);
        if(!CanChoose(index))return true;
        if(NeedsConfirmation(index)){confirmation_=index;Render();return true;}
        confirmation_.reset();
        if(page_==Page::Load)Send(Action::Load,index/2,index%2?cp::SaveKind::Autosave:cp::SaveKind::Manual);
        else Send(page_==Page::Save?Action::SaveManual:Action::BeginNewGame,index);
        return true;
    }
    if(id==ConfirmId) {
        if(confirmation_ && CanChoose(*confirmation_)) {
            const auto index=*confirmation_;confirmation_.reset();Render();
            Send(page_==Page::Save?Action::SaveManual:Action::BeginNewGame,index);
        }
        return true;
    }
    if(id==BackId) {
        if(confirmation_){confirmation_.reset();Render();}
        else if(Pending())Send(Action::Cancel);
        else if(!CommittedLoad())Send(Action::Close);
        return true;
    }
    if(id==RefreshId){if(!Pending())Send(Action::Refresh);return true;}
    if(id==ResumeId){if(CommittedLoad() && view_.canResumeLoaded && !Pending())Send(Action::Resume);return true;}
    return false;
}
void WindowsCampaignMenu::Refresh(const View& view) {
    if(visible_ && view.state.operation==cp::PlayOperation::Load &&
       (view.state.operationRevision!=view_.state.operationRevision || view.state.outcome==cp::PlayOutcome::Pending ||
        view.phase==Phase::Reading || view.phase==Phase::Preparing || view.phase==Phase::Rebase || view.phase==Phase::Presenting))loadInMenu_=true;
    if(view.catalogueRevision!=view_.catalogueRevision || view.state.operationRevision!=view_.state.operationRevision ||
       view.phase!=view_.phase)confirmation_.reset();
    view_=view;if(confirmation_ && !CanChoose(*confirmation_))confirmation_.reset();Render();
}
void WindowsCampaignMenu::Render() {
    if(!parent_)return;
    const auto set=[&](int id,const std::string& text,bool show,bool enable=true){
        const auto control=GetDlgItem(parent_,id);SetWindowTextA(control,text.c_str());
        EnableWindow(control,enable);ShowWindow(control,show?SW_SHOWNA:SW_HIDE);};
    set(TitleId,page_==Page::NewGame?"NEW GAME":page_==Page::Save?"SAVE AT LAST SAFE CHECKPOINT":"LOAD GAME",visible_);
    std::string status=view_.message;
    if(status.empty()) {
        switch(view_.phase) {
        case Phase::Unavailable:status="Campaign saves unavailable.";break;
        case Phase::Saving:status="Saving checkpoint...";break;
        case Phase::Reading:status="Reading checkpoint...";break;
        case Phase::Preparing:status="Preparing rescue route...";break;
        case Phase::Rebase:status="Resetting input for loaded checkpoint...";break;
        case Phase::Presenting:status="Waiting for loaded RT frame...";break;
        case Phase::Failed:status="Operation failed. Recheck records before retrying.";break;
        case Phase::Succeeded:status=view_.state.operation==cp::PlayOperation::Load && !view_.canResumeLoaded?
            "Checkpoint loaded; waiting for render and input readiness.":"Operation completed.";break;
        case Phase::Ready:status=page_==Page::Save?"Save the last reached safe checkpoint. Autosaves remain separate.":
            page_==Page::NewGame?"Choose the active autosave slot. Existing manual records are kept.":"Choose a manual save or latest autosave.";break;
        }
    }
    if(view_.writeUncertain)status+=" Write outcome uncertain. Recheck records before retrying.";
    if(confirmation_)status=page_==Page::Save?"Replace this manual save? Its latest autosave stays separate.":
        "Start a new game in this slot? Manual saves are kept; future checkpoints replace its latest autosave.";
    set(StatusId,status,visible_);
    for(unsigned i=0;i<6;++i) {
        std::string text;
        if(page_==Page::Load)text="Slot "+std::to_string(i/2+1)+(i%2?" - Latest auto: ":" - Manual: ")+RecordLabel(view_.catalogue[i]);
        else if(i<3) {
            text="Slot "+std::to_string(i+1)+" - Manual: "+RecordLabel(view_.catalogue[i*2]);
            if(page_==Page::NewGame)text+="\nLatest auto: "+RecordLabel(view_.catalogue[i*2+1]);
        }
        set(FirstChoiceId+i,text,visible_ && (page_==Page::Load || i<3),!confirmation_ && CanChoose(i));
    }
    set(ConfirmId,"CONFIRM",visible_ && confirmation_.has_value(),confirmation_ && CanChoose(*confirmation_));
    const bool committed=CommittedLoad();
    set(BackId,confirmation_?"CANCEL CONFIRMATION":Pending()?"CANCEL":"BACK",visible_,!committed || Pending());
    set(RefreshId,"RECHECK RECORDS",visible_,!Pending() && !confirmation_);
    set(ResumeId,"RESUME LOADED GAME",visible_ && committed && view_.canResumeLoaded,committed && view_.canResumeLoaded && !Pending());
}
void WindowsCampaignMenu::Layout(RECT bounds) {
    if(!parent_)return;
    const int width=(bounds.right-bounds.left)>520?520:bounds.right-bounds.left;
    const int x=bounds.left+((bounds.right-bounds.left)-width)/2;
    int y=bounds.top+12;
    const auto place=[&](int id,int height){MoveWindow(GetDlgItem(parent_,id),x,y,width,height,TRUE);y+=height+6;};
    place(TitleId,24);place(StatusId,64);
    for(unsigned i=0;i<(page_==Page::Load?6u:3u);++i)place(FirstChoiceId+i,38);
    place(ConfirmId,32);place(BackId,32);place(RefreshId,32);place(ResumeId,32);
}
} // namespace horde::platform::windows
