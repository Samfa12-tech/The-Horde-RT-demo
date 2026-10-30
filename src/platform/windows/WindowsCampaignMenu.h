#pragma once

#include "gameplay/campaign/CampaignPlaySession.h"
#include <functional>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace horde::platform::windows {
class WindowsCampaignMenu {
public:
    enum class Page { NewGame, Save, Load };
    enum class Origin { Entry, Pause, Death };
    enum class Phase { Unavailable, Ready, Saving, Reading, Preparing, Rebase, Presenting, Succeeded, Failed };
    enum class Action { Refresh, BeginNewGame, SaveManual, Load, Cancel, Resume, Close };
    struct Command {
        Action action=Action::Close;
        unsigned slot=0;
        gameplay::campaign::SaveKind kind=gameplay::campaign::SaveKind::Manual;
    };
    struct View {
        gameplay::campaign::PlayState state;
        std::array<gameplay::campaign::CatalogueEntry,6> catalogue{};
        Phase phase=Phase::Unavailable;
        std::string message;
        bool canResumeLoaded=false,writeUncertain=false;
        std::uint64_t catalogueRevision=0;
    };
    using Dispatch=std::function<void(Command)>;
    explicit WindowsCampaignMenu(Dispatch dispatch={});
    ~WindowsCampaignMenu();
    WindowsCampaignMenu(const WindowsCampaignMenu&)=delete;
    WindowsCampaignMenu& operator=(const WindowsCampaignMenu&)=delete;
    void SetDispatch(Dispatch dispatch);
    bool Create(HWND parent);
    void Open(Page page,Origin origin);
    void Close();
    bool Visible() const {return visible_;}
    bool ShouldContinueAfterPresentation() const {return visible_ && directLoadRequested_;}
    void HoldLoadedGame() {directLoadRequested_=false;}
    bool HandleCommand(int commandId);
    void Layout(RECT bounds);
    void Refresh(const View& view);
    // Stable native control IDs permit owner navigation and hidden HWND tests.
    static constexpr int TitleId=4100,StatusId=4101,FirstChoiceId=4110,
        ConfirmId=4120,BackId=4121,RefreshId=4122,ResumeId=4123;
private:
    bool Pending() const;
    bool CommittedLoad() const;
    bool CanChoose(unsigned index) const;
    bool NeedsConfirmation(unsigned index) const;
    void Render();
    void Send(Action action,unsigned slot=0,gameplay::campaign::SaveKind kind=gameplay::campaign::SaveKind::Manual);
    HWND parent_=nullptr;
    std::array<HWND,12> controls_{};
    Dispatch dispatch_;
    View view_;
    Page page_=Page::Load;
    Origin origin_=Origin::Entry;
    bool visible_=false;
    bool loadInMenu_=false;
    bool directLoadRequested_=false;
    std::optional<unsigned> confirmation_;
};
} // namespace horde::platform::windows
