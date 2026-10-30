#include "platform/windows/WindowsCampaignMenu.h"
#include "platform/windows/WindowsControlPresentation.h"
#include "platform/windows/WindowsSaveDate.h"
#include "gameplay/campaign/CampaignSlotSelection.h"

#include <utility>
#include <algorithm>

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
    case cp::CatalogueStatus::Invalid:return "Save unavailable";
    case cp::CatalogueStatus::ReadError:return "Could not open save";
    case cp::CatalogueStatus::Valid:
        if(entry.checkpoint && cp::Validate(*entry.checkpoint))
            return std::string(entry.checkpoint->id==cp::CheckpointId::ForestEndpoint?"Forest lookout":
                entry.checkpoint->id==cp::CheckpointId::SafeExit?"Safe exit":"Lower chamber")+
                " | "+std::to_string(entry.checkpoint->vitality)+" / 3 hearts\n"+WindowsSaveDate(entry.checkpoint->savedAtUnixSeconds);
        return "Save unavailable";
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
        controls_[i]=CreateWindowExW(0,label?L"STATIC":L"BUTTON",L"",
            WS_CHILD|(label?SS_LEFT:BS_PUSHBUTTON|BS_MULTILINE|WS_TABSTOP),0,0,0,0,parent,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ids[i])),GetModuleHandleW(nullptr),nullptr);
        if(!controls_[i]) {
            for(auto& control:controls_){if(control)DestroyWindow(control);control=nullptr;}
            parent_=nullptr;return false;
        }
        SendMessageA(controls_[i],WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),FALSE);
    }
    Render();return true;
}
void WindowsCampaignMenu::Open(Page page,Origin origin){page_=page;origin_=origin;confirmation_.reset();directLoadRequested_=false;
    loadInMenu_=view_.state.operation==cp::PlayOperation::Load && Pending();visible_=true;Render();}
void WindowsCampaignMenu::Close(){visible_=false;loadInMenu_=false;directLoadRequested_=false;confirmation_.reset();Render();}
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
    if(index>=3)return false;
    if(page_==Page::Load)return cp::SelectSlotRecord(view_.catalogue,index).has_value();
    if(page_==Page::NewGame) return view_.catalogue[index*2].status!=cp::CatalogueStatus::Checking &&
        view_.catalogue[index*2+1].status!=cp::CatalogueStatus::Checking;
    const auto& entry=view_.catalogue[index*2];
    if(view_.catalogue[index*2+1].status==cp::CatalogueStatus::Checking)return false;
    return view_.state.hasLatestCheckpoint && (entry.status==cp::CatalogueStatus::Empty || Valid(entry));
}
bool WindowsCampaignMenu::NeedsConfirmation(unsigned index) const {
    if(page_==Page::Save)return view_.catalogue[index*2].status!=cp::CatalogueStatus::Empty ||
        view_.catalogue[index*2+1].status!=cp::CatalogueStatus::Empty;
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
        if(page_==Page::Load){directLoadRequested_=true;Send(Action::Load,index,*cp::SelectSlotRecord(view_.catalogue,index));}
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
        directLoadRequested_=false;
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
        const auto control=GetDlgItem(parent_,id);SetNativeUtf8TextIfChanged(control,text);
        if((IsWindowEnabled(control)!=FALSE)!=enable)EnableWindow(control,enable);
        SetNativeVisibilityIfChanged(control,show);};
    set(TitleId,page_==Page::NewGame?"NEW GAME":page_==Page::Save?"SAVE GAME":"LOAD GAME",visible_);
    std::string status;
    if(status.empty()) {
        switch(view_.phase) {
        case Phase::Unavailable:status="Your saves are unavailable.";break;
        case Phase::Saving:status="Saving...";break;
        case Phase::Reading:case Phase::Preparing:status="Loading...";break;
        case Phase::Rebase:case Phase::Presenting:status="Getting ready...";break;
        case Phase::Failed:status=page_==Page::Save?"Could not save. Your previous saves have been kept.":"Could not load this game. Your saves have been kept.";break;
        case Phase::Succeeded:status=view_.state.operation==cp::PlayOperation::Load && !view_.canResumeLoaded?
            "Getting ready...":"Saved.";break;
        case Phase::Ready:status=page_==Page::Save?(view_.state.hasLatestCheckpoint?"Save at your latest checkpoint.":"Reach a safe checkpoint before saving."):
            page_==Page::NewGame?"Choose a slot to start a new game.":"Choose a game to continue.";break;
        }
    }
    if(view_.writeUncertain)status+=" Could not confirm saving. Check saves before trying again.";
    if(confirmation_)status=page_==Page::Save?"Replace the save in Slot "+std::to_string(*confirmation_+1)+"?":
        "Start again in Slot "+std::to_string(*confirmation_+1)+"? New progress is saved at checkpoints.";
    set(StatusId,status,visible_);
    for(unsigned i=0;i<6;++i) {
        std::string text;
        if(i<3) {
            const auto selected=cp::SelectSlotRecord(view_.catalogue,i);
            text="Slot "+std::to_string(i+1)+"  |  "+(page_==Page::Load?"Continue":page_==Page::Save?"Save":"New game")+"\n";
            if(selected)text+=RecordLabel(view_.catalogue[i*2+(*selected==cp::SaveKind::Autosave?1:0)]);
            else if(view_.catalogue[i*2].status==cp::CatalogueStatus::Checking || view_.catalogue[i*2+1].status==cp::CatalogueStatus::Checking)text+="Checking saves...";
            else if(view_.catalogue[i*2].status==cp::CatalogueStatus::Empty && view_.catalogue[i*2+1].status==cp::CatalogueStatus::Empty)text+="Empty";
            else text+="Save unavailable - your files have been kept";
        }
        set(FirstChoiceId+i,text,visible_ && i<3,!confirmation_ && CanChoose(i));
    }
    set(ConfirmId,page_==Page::Save?"REPLACE SAVE":"START NEW GAME",visible_ && confirmation_.has_value(),confirmation_ && CanChoose(*confirmation_));
    const bool committed=CommittedLoad();
    set(BackId,confirmation_ || Pending()?"CANCEL":"BACK",visible_,!committed || Pending());
    const bool unreadable=std::any_of(view_.catalogue.begin(),view_.catalogue.end(),[](const cp::CatalogueEntry& e){
        return e.status==cp::CatalogueStatus::Invalid || e.status==cp::CatalogueStatus::ReadError;});
    set(RefreshId,"TRY AGAIN",visible_ && (view_.phase==Phase::Failed || view_.writeUncertain || unreadable),!Pending() && !confirmation_);
    set(ResumeId,"CONTINUE",visible_ && committed && view_.canResumeLoaded,committed && view_.canResumeLoaded && !Pending());
}
void WindowsCampaignMenu::Layout(RECT bounds) {
    if(!parent_)return;
    const int width=(bounds.right-bounds.left)>520?520:bounds.right-bounds.left;
    const int x=bounds.left+((bounds.right-bounds.left)-width)/2;
    int y=bounds.top+12;
    const auto place=[&](int id,int height){MoveWindow(GetDlgItem(parent_,id),x,y,width,height,TRUE);y+=height+6;};
    place(TitleId,24);place(StatusId,48);
    const int cardHeight=std::clamp<int>((bounds.bottom-y-152)/3,54,82);
    for(unsigned i=0;i<3;++i)place(FirstChoiceId+i,cardHeight);
    place(ConfirmId,32);place(BackId,32);place(RefreshId,32);place(ResumeId,32);
}
} // namespace horde::platform::windows
