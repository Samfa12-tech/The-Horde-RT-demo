#include "platform/windows/WindowsCompatibilityReport.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
namespace
{
std::vector<std::string> copied,saved;
bool Copy(HWND,const std::string& bytes,std::wstring& status){copied.push_back(bytes);status=L"Fixture copied";return true;}
bool Save(HWND,const std::string& bytes,std::wstring& status){saved.push_back(bytes);status=L"Fixture save";return saved.size()>1;}
std::wstring Text(HWND window){std::wstring text(std::size_t(GetWindowTextLengthW(window))+1,L'\0');GetWindowTextW(window,text.data(),int(text.size()));text.resize(wcslen(text.c_str()));return text;}
void WaitForReceipt(HWND window)
{
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    while(std::chrono::steady_clock::now()<deadline) {
        MSG message{};while(PeekMessageW(&message,window,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
        if(IsWindowEnabled(GetDlgItem(window,horde::platform::windows::kCompatibilitySendConsent)))return;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}
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
    check(!IsWindowEnabled(GetDlgItem(window,kCompatibilitySendConsent))&&!IsWindowEnabled(GetDlgItem(window,kCompatibilitySend)),"production Send remains unavailable");
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
    unsigned verifications=0,calls=0;std::vector<std::string> requests;
    WindowsCompatibilityServices services{Save,Copy};
    services.send.verify=[&](HWND,bool publicBoard){check(!publicBoard,"compatibility must never ask for public verification");return "fresh-"+std::to_string(++verifications);};
    services.send.privateExchange=[&](std::string_view body,auto,HANDLE){
        requests.emplace_back(body);if(++calls==1)return WindowsPlaytestHttpResponse{503,""};
        const auto at=body.find("\"reportId\":\"")+12u;const auto id=std::string(body.substr(at,36u));
        return WindowsPlaytestHttpResponse{200,"{\"ok\":true,\"id\":\""+id+"\",\"status\":\"sent\",\"destination\":\"developer-private\",\"reportKind\":\"compatibility\"}"};};
    window=CreateWindowsCompatibilityReportView(nullptr,snapshot,services,true);
    check(window!=nullptr,"local Send view creation");if(!window)return 1;
    SendDlgItemMessageW(window,kCompatibilityConsent,BM_SETCHECK,BST_CHECKED,0);
    SendMessageW(window,WM_COMMAND,kCompatibilityConsent,0);SendMessageW(window,WM_COMMAND,kCompatibilityPrepare,0);
    const auto local=Text(GetDlgItem(window,kCompatibilityPreview));
    SendMessageW(window,WM_COMMAND,kCompatibilitySend,0);check(calls==0,"local preparation is not Send consent");
    SendDlgItemMessageW(window,kCompatibilitySendConsent,BM_SETCHECK,BST_CHECKED,0);
    SendMessageW(window,WM_COMMAND,kCompatibilitySendConsent,0);
    const auto remote=Text(GetDlgItem(window,kCompatibilitySendPreview));
    check(remote.find(L"consentToSubmit")!=remote.npos&&remote.find(L"developer-private")!=remote.npos&&
        local.find(L"consentToSubmit")==local.npos&&remote.find(L"turnstileToken")==remote.npos,
        "remote exact preview is separate from local export and token");
    SendMessageW(window,WM_COMMAND,kCompatibilitySend,0);WaitForReceipt(window);
    check(calls==1&&Text(GetDlgItem(window,kCompatibilityStatus)).find(L"unknown")!=std::wstring::npos,"failed receipt must remain uncertain");
    SendMessageW(window,WM_COMMAND,kCompatibilitySend,0);WaitForReceipt(window);
    check(calls==2&&verifications==2&&Text(GetDlgItem(window,kCompatibilityStatus)).find(L"Local test: provider accepted")!=std::wstring::npos,
        "private explicit retry must not claim real inbox delivery");
    check(Text(GetDlgItem(window,kCompatibilitySendPreview))==remote&&Text(GetDlgItem(window,kCompatibilityPreview))==local,
        "retry cannot mutate either preview");
    SendMessageW(window,WM_COMMAND,kCompatibilityClose,0);check(!IsWindow(window),"close retires local Send owner");
    return passed?0:1;
}
