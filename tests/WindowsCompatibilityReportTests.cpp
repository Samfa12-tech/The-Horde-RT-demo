#include "platform/windows/WindowsCompatibilityReport.h"
#include <iostream>
#include <string>
#include <vector>
namespace
{
std::vector<std::string> copied,saved;
bool Copy(HWND,const std::string& bytes,std::wstring& status){copied.push_back(bytes);status=L"Fixture copied";return true;}
bool Save(HWND,const std::string& bytes,std::wstring& status){saved.push_back(bytes);status=L"Fixture save";return saved.size()>1;}
std::wstring Text(HWND window){std::wstring text(std::size_t(GetWindowTextLengthW(window))+1,L'\0');GetWindowTextW(window,text.data(),int(text.size()));text.resize(wcslen(text.c_str()));return text;}
}
int main()
{
    using namespace horde::platform::windows;using namespace horde::reporting;
    bool passed=true;const auto check=[&](bool ok,const char* what){if(!ok){passed=false;std::cerr<<what<<'\n';}};
    CompatibilityReportSnapshot snapshot;snapshot.version="1.7-local";snapshot.build="local-contract";
    snapshot.lastStage=CompatibilityStage::DiagnosticStorage;snapshot.outcome=CompatibilityOutcome::StartupFailure;
    const auto foreground=GetForegroundWindow();
    HWND window=CreateWindowsCompatibilityReportView(nullptr,snapshot,{Save,Copy},true);
    check(window!=nullptr,"failed to create hidden compatibility view");if(!window)return 1;
    snapshot.build="Later mutated caller state";
    check(!IsWindowVisible(window) && GetForegroundWindow()==foreground,"fixture must remain hidden and not change foreground");
    check(Text(GetDlgItem(window,kCompatibilityPreview)).empty() && copied.empty() && saved.empty(),"no report/export before consent");
    SendMessageW(window,WM_COMMAND,kCompatibilityPrepare,0);
    check(Text(GetDlgItem(window,kCompatibilityPreview)).empty(),"prepare without consent must reject");
    SendDlgItemMessageW(window,kCompatibilityConsent,BM_SETCHECK,BST_CHECKED,0);
    SendMessageW(window,WM_COMMAND,kCompatibilityConsent,0);SendMessageW(window,WM_COMMAND,kCompatibilityPrepare,0);
    const auto prepared=Text(GetDlgItem(window,kCompatibilityPreview));
    check(prepared.find(L"local-contract")!=prepared.npos && prepared.find(L"Later mutated") == prepared.npos &&
        prepared.find(L"\"rtPresented\":false")!=prepared.npos && prepared.find(L"diagnostic-storage")!=prepared.npos,
        "view must prepare its owned renderer-independent observed failure");
    check(!IsWindowEnabled(GetDlgItem(window,kCompatibilityPrepare)),"approved fields must stay frozen");
    SendMessageW(window,WM_COMMAND,kCompatibilityCopy,0);SendMessageW(window,WM_COMMAND,kCompatibilitySave,0);
    SendMessageW(window,WM_COMMAND,kCompatibilitySave,0);
    check(copied.size()==1 && saved.size()==2 && saved[0]==copied[0] && saved[1]==copied[0],"failed save/retry must keep exact reviewed ID/bytes");
    check(saved.empty() || (saved[0].find("consentToSubmit")==saved[0].npos && saved[0].find("public-board")==saved[0].npos),"local review cannot grant remote/public consent");
    HFONT large=CreateFontW(-36,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    SendDlgItemMessageW(window,kCompatibilityConsent,WM_SETFONT,reinterpret_cast<WPARAM>(large),FALSE);
    SetWindowPos(window,nullptr,0,0,520,420,SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
    SendMessageW(window,WM_SIZE,0,0);SCROLLINFO scroll{sizeof(SCROLLINFO),SIF_ALL};GetScrollInfo(window,SB_VERT,&scroll);
    check(scroll.nMax>int(scroll.nPage),"large font/small window needs a scrollable view");
    SendMessageW(window,WM_VSCROLL,SB_BOTTOM,0);RECT close{},client{};GetWindowRect(GetDlgItem(window,kCompatibilityClose),&close);
    MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&close),2);GetClientRect(window,&client);
    check(close.top>=0 && close.bottom<=client.bottom,"Close must be reachable at the end of the scroll");
    SendMessageW(window,WM_COMMAND,kCompatibilityClose,0);check(!IsWindow(window),"Close retires only the fixture view");
    if(large)DeleteObject(large);check(GetForegroundWindow()==foreground,"hidden checks must not acquire foreground");
    return passed?0:1;
}
