#include "platform/windows/WindowsCompatibilityReport.h"
#include <bcrypt.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <memory>

namespace horde::platform::windows
{
namespace
{
constexpr wchar_t kClass[]=L"HordeCompatibilityReviewV1";
constexpr int kDisclosure=6108;
constexpr UINT_PTR kSendTimer=445u;
struct View
{
    reporting::CompatibilityReportSnapshot snapshot;
    reporting::PreparedCompatibilityReport prepared;
    WindowsCompatibilityServices services;
    WindowsStatisticsSubmission submission;
    HFONT font=nullptr;
    bool owned=false, failure=false;
    int scroll=0;
};
int Dpi(HWND w,int n) {const auto dpi=GetDpiForWindow(w);return MulDiv(n,static_cast<int>(dpi?dpi:96u),96);}
std::wstring Wide(std::string_view utf8)
{
    if(utf8.empty() || utf8.size()>reporting::kDeveloperReportSubmissionMaxBytes)return {};
    const auto n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,utf8.data(),static_cast<int>(utf8.size()),nullptr,0);
    if(n<=0)return {};std::wstring wide(static_cast<std::size_t>(n),L'\0');
    if(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,utf8.data(),static_cast<int>(utf8.size()),wide.data(),n)!=n)return {};
    return wide;
}
void Status(HWND w,const std::wstring& text) {SetWindowTextW(GetDlgItem(w,kCompatibilityStatus),text.c_str());}
void RefreshSend(HWND w,View& state)
{
    EnableWindow(GetDlgItem(w,kCompatibilitySendConsent),state.prepared.IsReady() && state.submission.Available(false) && !state.submission.Busy());
    EnableWindow(GetDlgItem(w,kCompatibilitySend),state.submission.CanSend());
    EnableWindow(GetDlgItem(w,kCompatibilityCancelSend),state.submission.Busy());
    if(state.submission.Busy())SetTimer(w,kSendTimer,100u,nullptr);else KillTimer(w,kSendTimer);
}
bool Copy(HWND w,const std::string& json,std::wstring& message)
{
    const auto wide=Wide(json);if(wide.empty() || !OpenClipboard(w)){message=L"Could not copy. The reviewed report is still here.";return false;}
    const auto bytes=(wide.size()+1u)*sizeof(wchar_t);
    HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,bytes);bool copied=false;
    if(memory) {
        if(void* dst=GlobalLock(memory)) {
            std::memcpy(dst,wide.c_str(),bytes);GlobalUnlock(memory);
            if(EmptyClipboard() && SetClipboardData(CF_UNICODETEXT,memory)){copied=true;memory=nullptr;}
        }
        if(memory)GlobalFree(memory);
    }
    CloseClipboard();message=copied?L"Report copied. Nothing has been sent.":L"Could not copy. The reviewed report is still here.";return copied;
}
bool Identity(std::string& id,std::string& utc)
{
    std::array<unsigned char,16> bytes{};
    if(BCryptGenRandom(nullptr,bytes.data(),static_cast<ULONG>(bytes.size()),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0)return false;
    bytes[6]=(bytes[6]&15u)|64u;bytes[8]=(bytes[8]&63u)|128u;
    constexpr char hex[]="0123456789abcdef";
    for(std::size_t i=0;i<bytes.size();++i){if(i==4||i==6||i==8||i==10)id+='-';id+=hex[bytes[i]>>4];id+=hex[bytes[i]&15];}
    SYSTEMTIME time{};GetSystemTime(&time);char text[32]{};
    const int count=std::snprintf(text,sizeof(text),"%04u-%02u-%02uT%02u:%02u:%02uZ",
        unsigned(time.wYear),unsigned(time.wMonth),unsigned(time.wDay),unsigned(time.wHour),unsigned(time.wMinute),unsigned(time.wSecond));
    if(count<=0 || static_cast<std::size_t>(count)>=sizeof(text))return false;utc=text;return true;
}
void Layout(HWND w)
{
    auto* state=reinterpret_cast<View*>(GetWindowLongPtrW(w,GWLP_USERDATA));if(!state)return;
    RECT r{};GetClientRect(w,&r);const int m=Dpi(w,16),gap=Dpi(w,8),width=std::max(1,int(r.right)-2*m);
    int line=Dpi(w,20);HDC dc=GetDC(w);
    if(dc){const auto font=reinterpret_cast<HFONT>(SendMessageW(GetDlgItem(w,kCompatibilityConsent),WM_GETFONT,0,0));
        const auto old=font?SelectObject(dc,font):nullptr;TEXTMETRICW metrics{};
        if(GetTextMetricsW(dc,&metrics))line=std::max(line,int(metrics.tmHeight)+Dpi(w,4));
        if(old)SelectObject(dc,old);ReleaseDC(w,dc);}
    const int disclosure=std::max(Dpi(w,90),line*5),consent=std::max(Dpi(w,56),line*3);
    const int button=std::max(Dpi(w,40),line+Dpi(w,12)),status=std::max(Dpi(w,40),line*3);
    const int remote=std::max(Dpi(w,100),line*3);
    const int fixed=2*m+disclosure+2*consent+4*button+status+remote+9*gap;
    const int preview=std::max(line*3,int(r.bottom)-fixed);
    const int height=fixed+preview;
    state->scroll=std::clamp(state->scroll,0,std::max(0,height-int(r.bottom)));
    SCROLLINFO scroll{sizeof(SCROLLINFO),SIF_RANGE|SIF_PAGE|SIF_POS};
    scroll.nMin=0;scroll.nMax=height-1;scroll.nPage=std::max(1u,static_cast<UINT>(r.bottom));scroll.nPos=state->scroll;SetScrollInfo(w,SB_VERT,&scroll,TRUE);
    int y=m-state->scroll;
    const auto place=[&](int id,int h){MoveWindow(GetDlgItem(w,id),m,y,width,h,TRUE);y+=h+gap;};
    place(kDisclosure,disclosure);place(kCompatibilityConsent,consent);place(kCompatibilityPrepare,button);
    place(kCompatibilityPreview,preview);place(kCompatibilityStatus,status);
    place(kCompatibilitySendConsent,consent);place(kCompatibilitySendPreview,remote);
    place(kCompatibilitySend,button);place(kCompatibilityCancelSend,button);
    const int bw=std::max(1,(width-2*gap)/3);
    for(int i=0;i<3;++i)MoveWindow(GetDlgItem(w,kCompatibilityCopy+i),m+i*(bw+gap),y,bw,button,TRUE);
}
LRESULT CALLBACK Proc(HWND w,UINT message,WPARAM wp,LPARAM lp)
{
    auto* state=reinterpret_cast<View*>(GetWindowLongPtrW(w,GWLP_USERDATA));
    if(message==WM_NCCREATE){state=static_cast<View*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(state));}
    if(message==WM_CREATE && state) {
        const auto add=[&](const wchar_t* kind,int id,const wchar_t* text,DWORD style){
            return CreateWindowExW(id==kCompatibilityPreview?WS_EX_CLIENTEDGE:0,kind,text,WS_CHILD|WS_VISIBLE|style,
                0,0,1,1,w,reinterpret_cast<HMENU>(INT_PTR(id)),GetModuleHandleW(nullptr),nullptr);};
        if(!add(L"STATIC",kDisclosure,state->failure?
            L"Horde could not complete this startup/render attempt. You can review and export a private compatibility report. Nothing is sent automatically; sending is not available yet.":
            L"Review app/build, optional device/OS/GPU details, capability checks and the observed startup/render result. No saves, screenshots, raw logs or device identifiers. Sending is not available yet.",SS_LEFT|SS_NOPREFIX) ||
           !add(L"BUTTON",kCompatibilityConsent,L"Allow these details in a local report for review",BS_AUTOCHECKBOX|BS_MULTILINE|WS_TABSTOP) ||
           !add(L"BUTTON",kCompatibilityPrepare,L"Prepare report",BS_PUSHBUTTON|WS_TABSTOP) ||
           !add(L"EDIT",kCompatibilityPreview,L"",ES_MULTILINE|ES_READONLY|WS_VSCROLL|WS_HSCROLL|WS_TABSTOP) ||
           !add(L"STATIC",kCompatibilityStatus,L"Review consent before preparing. Nothing has been sent.",SS_LEFT|SS_NOPREFIX) ||
           !add(L"BUTTON",kCompatibilitySendConsent,L"Separately approve private developer delivery (Send payload below; never public)",BS_AUTOCHECKBOX|BS_MULTILINE|WS_TABSTOP) ||
           !add(L"EDIT",kCompatibilitySendPreview,L"",ES_MULTILINE|ES_READONLY|WS_VSCROLL|WS_HSCROLL|WS_TABSTOP) ||
           !add(L"BUTTON",kCompatibilitySend,state->submission.Available(false)?L"Send / retry (local test only)":L"Sending unavailable",BS_PUSHBUTTON|WS_TABSTOP) ||
           !add(L"BUTTON",kCompatibilityCancelSend,L"Cancel send attempt",BS_PUSHBUTTON|WS_TABSTOP) ||
           !add(L"BUTTON",kCompatibilityCopy,L"Copy report",BS_PUSHBUTTON|WS_TABSTOP) ||
           !add(L"BUTTON",kCompatibilitySave,L"Save report…",BS_PUSHBUTTON|WS_TABSTOP) ||
           !add(L"BUTTON",kCompatibilityClose,L"Close",BS_PUSHBUTTON|WS_TABSTOP))return -1;
        NONCLIENTMETRICSW metrics{sizeof(NONCLIENTMETRICSW)};
        if(SystemParametersInfoW(SPI_GETNONCLIENTMETRICS,sizeof(metrics),&metrics,0))state->font=CreateFontIndirectW(&metrics.lfMessageFont);
        const auto font=state->font?state->font:static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        EnumChildWindows(w,[](HWND child,LPARAM f)->BOOL{SendMessageW(child,WM_SETFONT,WPARAM(f),TRUE);return TRUE;},reinterpret_cast<LPARAM>(font));
        for(int id:{kCompatibilityPrepare,kCompatibilityCopy,kCompatibilitySave})EnableWindow(GetDlgItem(w,id),FALSE);
        RefreshSend(w,*state);Layout(w);return 0;
    }
    if(state) {
        WindowsPlaytestSubmissionResult result{};
        if(state->submission.Handle(message,wp,result) || (message==WM_TIMER && wp==kSendTimer))
        {RefreshSend(w,*state);Status(w,state->submission.StatusText());return 0;}
    }
    if(message==WM_COMMAND && state) {
        if(HIWORD(wp)!=BN_CLICKED)return 0;
        const int id=LOWORD(wp);
        if(id==kCompatibilityClose){DestroyWindow(w);return 0;}
        if(id==kCompatibilitySendConsent && state->prepared.IsReady() && state->submission.Available(false) && !state->submission.Busy()) {
            state->submission.Clear();std::wstring remote;
            if(SendDlgItemMessageW(w,id,BM_GETCHECK,0,0)==BST_CHECKED && state->submission.Prepare(state->prepared,true))remote=Wide(state->submission.Json());
            SetWindowTextW(GetDlgItem(w,kCompatibilitySendPreview),remote.c_str());RefreshSend(w,*state);return 0;
        }
        if(id==kCompatibilitySend) {
            if(SendDlgItemMessageW(w,kCompatibilitySendConsent,BM_GETCHECK,0,0)==BST_CHECKED && state->submission.Send(w))Status(w,L"Local test request started. No real report is being sent.");
            RefreshSend(w,*state);return 0;
        }
        if(id==kCompatibilityCancelSend) {state->submission.CancelAttempt();RefreshSend(w,*state);Status(w,L"Attempt cancelled; acceptance may be unknown. Retry the same payload explicitly.");return 0;}
        if(id==kCompatibilityConsent && !state->prepared.IsReady()) {
            EnableWindow(GetDlgItem(w,kCompatibilityPrepare),SendDlgItemMessageW(w,kCompatibilityConsent,BM_GETCHECK,0,0)==BST_CHECKED);return 0;
        }
        if(id==kCompatibilityPrepare && !state->prepared.IsReady()) {
            if(SendDlgItemMessageW(w,kCompatibilityConsent,BM_GETCHECK,0,0)!=BST_CHECKED)return 0;
            std::string identity,utc;if(!Identity(identity,utc)){Status(w,L"Report identity is unavailable. Nothing was prepared.");return 0;}
            state->prepared=reporting::PrepareCompatibilityReport(state->snapshot,{true,identity,utc});
            if(!state->prepared.IsReady()){Status(w,L"Compatibility details are unavailable or inconsistent. Nothing was sent.");return 0;}
            const auto wide=Wide(state->prepared.Json());SetWindowTextW(GetDlgItem(w,kCompatibilityPreview),wide.c_str());
            EnableWindow(GetDlgItem(w,kCompatibilityConsent),FALSE);EnableWindow(GetDlgItem(w,kCompatibilityPrepare),FALSE);
            EnableWindow(GetDlgItem(w,kCompatibilityCopy),TRUE);EnableWindow(GetDlgItem(w,kCompatibilitySave),TRUE);
            RefreshSend(w,*state);Status(w,L"Review these exact details before Copy or Save. Nothing has been sent.");return 0;
        }
        if((id==kCompatibilityCopy || id==kCompatibilitySave) && state->prepared.IsReady()) {
            std::wstring text;const std::string bytes(state->prepared.Json());
            if(id==kCompatibilityCopy)(state->services.copy?state->services.copy:Copy)(w,bytes,text);
            else (state->services.exporter?state->services.exporter:ExportWindowsPlaytestReportJson)(w,bytes,text);
            Status(w,text);return 0;
        }
    }
    if(message==WM_SIZE){Layout(w);return 0;}
    if((message==WM_VSCROLL || message==WM_MOUSEWHEEL) && state) {
        SCROLLINFO scroll{sizeof(SCROLLINFO),SIF_ALL};GetScrollInfo(w,SB_VERT,&scroll);
        if(message==WM_MOUSEWHEEL)state->scroll-=GET_WHEEL_DELTA_WPARAM(wp)*Dpi(w,60)/WHEEL_DELTA;
        else switch(LOWORD(wp)) {
            case SB_LINEUP:state->scroll-=Dpi(w,24);break;case SB_LINEDOWN:state->scroll+=Dpi(w,24);break;
            case SB_PAGEUP:state->scroll-=int(scroll.nPage);break;case SB_PAGEDOWN:state->scroll+=int(scroll.nPage);break;
            case SB_THUMBTRACK:case SB_THUMBPOSITION:state->scroll=scroll.nTrackPos;break;
            case SB_TOP:state->scroll=0;break;case SB_BOTTOM:state->scroll=scroll.nMax;break;default:break;
        }
        Layout(w);return 0;
    }
    if(message==WM_DPICHANGED){const auto* r=reinterpret_cast<RECT*>(lp);SetWindowPos(w,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOACTIVATE|SWP_NOZORDER);return 0;}
    if(message==WM_GETMINMAXINFO){auto* r=reinterpret_cast<MINMAXINFO*>(lp);r->ptMinTrackSize={Dpi(w,470),Dpi(w,520)};return 0;}
    if(message==WM_CLOSE || (message==WM_KEYDOWN && wp==VK_ESCAPE)){DestroyWindow(w);return 0;}
    if(message==WM_NCDESTROY && state){KillTimer(w,kSendTimer);state->submission.Clear();SetWindowLongPtrW(w,GWLP_USERDATA,0);if(state->font)DeleteObject(state->font);if(state->owned)delete state;}
    return DefWindowProcW(w,message,wp,lp);
}
}
HWND CreateWindowsCompatibilityReportView(HWND owner,reporting::CompatibilityReportSnapshot snapshot,
    WindowsCompatibilityServices services,bool failure)
{
    WNDCLASSW cls{};cls.lpfnWndProc=Proc;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=kClass;
    cls.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));cls.hbrBackground=GetSysColorBrush(COLOR_BTNFACE);
    if(!RegisterClassW(&cls) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return nullptr;
    auto state=std::make_unique<View>();state->snapshot=std::move(snapshot);state->services=services;state->failure=failure;state->submission.services=services.send;
    HWND window=CreateWindowExW(WS_EX_DLGMODALFRAME|WS_EX_CONTROLPARENT,kClass,L"Report compatibility to developer",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_CLIPCHILDREN|WS_VSCROLL,CW_USEDEFAULT,CW_USEDEFAULT,720,720,
        owner,nullptr,GetModuleHandleW(nullptr),state.get());
    if(window){state->owned=true;state.release();}return window;
}
void ShowWindowsCompatibilityReport(HWND owner,reporting::CompatibilityReportSnapshot snapshot,
    WindowsCompatibilityServices services,bool failure)
{
    const auto window=CreateWindowsCompatibilityReportView(owner,std::move(snapshot),services,failure);if(!window)return;
    const bool enabled=owner && IsWindowEnabled(owner);if(enabled)EnableWindow(owner,FALSE);
    ShowWindow(window,SW_SHOW);SetFocus(GetDlgItem(window,kCompatibilityConsent));
    MSG msg{};while(IsWindow(window)) {
        const auto result=GetMessageW(&msg,nullptr,0,0);
        if(result<=0){if(result==0)PostQuitMessage(static_cast<int>(msg.wParam));break;}
        if(!IsDialogMessageW(window,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}
    }
    if(IsWindow(window))DestroyWindow(window);if(enabled && IsWindow(owner))EnableWindow(owner,TRUE);
}
} // namespace horde::platform::windows
