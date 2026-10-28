#include "platform/windows/WindowsPlaytestReport.h"
#include "platform/windows/WindowsBenchmarkReviewPayload.h"
#include "reporting/BenchmarkSummaryReport.h"
#include <windowsx.h>
#include <commctrl.h>
#include <bcrypt.h>
#include <objidl.h>
#include <gdiplus.h>
#include <array>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace
{
using namespace horde::telemetry;
using namespace horde::reporting;
using namespace horde::gameplay;
bool passed=true;
void Check(bool ok,const char* message) { if(!ok){passed=false;std::cerr<<"summary UI: "<<message<<'\n';} }
#include "BenchmarkSummaryFixtures.inl"

// Only the external destinations are injected. Actual Win32 controls, window
// procedure, typed builders, UTF-8 conversion and action handlers are shared.
std::map<UINT,HGLOBAL> clipboard;
bool copyAccepted=false,saveAccepted=false;
unsigned copies=0,saves=0;
std::vector<std::string> saved;
std::filesystem::path outputDirectory;
constexpr UINT fixtureJsonFormat=0xC001;
void ClearFixtureClipboard() { for(auto& [format,data]:clipboard){(void)format;GlobalFree(data);}clipboard.clear(); }
BOOL FixtureOpenClipboard(HWND) { ++copies;return copyAccepted; }
BOOL FixtureEmptyClipboard() { ClearFixtureClipboard();return TRUE; }
BOOL FixtureCloseClipboard() { return TRUE; }
UINT FixtureRegisterClipboardFormatW(LPCWSTR value) { Check(std::wstring_view(value)==L"application/json","unexpected clipboard format");return fixtureJsonFormat; }
HANDLE FixtureSetClipboardData(UINT format,HANDLE data) { clipboard[format]=static_cast<HGLOBAL>(data);return data; }
std::string ClipboardBytes()
{
    const auto found=clipboard.find(fixtureJsonFormat);if(found==clipboard.end())return {};
    const char* bytes=static_cast<const char*>(GlobalLock(found->second));
    Check(bytes!=nullptr,"fixture clipboard lock");std::string result=bytes?bytes:"";
    if(bytes)GlobalUnlock(found->second);return result;
}
std::wstring ClipboardText()
{
    const auto found=clipboard.find(CF_UNICODETEXT);if(found==clipboard.end())return {};
    const auto* text=static_cast<const wchar_t*>(GlobalLock(found->second));
    Check(text!=nullptr,"fixture Unicode clipboard lock");std::wstring result=text?text:L"";
    if(text)GlobalUnlock(found->second);return result;
}
std::string UtcTimestamp(const char*) { return "2026-10-28T00:00:00Z"; } // Synthetic fixed clock, never a live report.
}

namespace horde::platform::windows
{
bool ExportWindowsPlaytestReportJson(HWND,const std::string& json,std::wstring& message)
{
    ++saves;saved.push_back(json);
    if(!saveAccepted){message=L"Synthetic destination cancelled; retry explicitly.";return false;}
    const auto path=outputDirectory/("approved-"+std::to_string(saves)+".json");
    HANDLE file=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){message=L"Fixture destination unavailable.";return false;}
    DWORD written=0;
    const bool ok=WriteFile(file,json.data(),static_cast<DWORD>(json.size()),&written,nullptr)!=FALSE &&
        written==json.size() && FlushFileBuffers(file)!=FALSE;
    const bool closed=CloseHandle(file)!=FALSE;
    message=ok&&closed?L"Synthetic local bytes saved.":L"Fixture write failed.";
    return ok&&closed;
}
}

namespace
{
#define OpenClipboard FixtureOpenClipboard
#define EmptyClipboard FixtureEmptyClipboard
#define CloseClipboard FixtureCloseClipboard
#define RegisterClipboardFormatW FixtureRegisterClipboardFormatW
#define SetClipboardData FixtureSetClipboardData
#include "platform/windows/WindowsBenchmarkSummaryReviewUi.inl"
#undef OpenClipboard
#undef EmptyClipboard
#undef CloseClipboard
#undef RegisterClipboardFormatW
#undef SetClipboardData

// Simulate only a smaller monitor work-area constraint, without changing the
// owner's display/DPI or product minimum-size policy. All layout and actions
// still use the real client rectangle and production procedure.
bool smallWorkArea=false;
LRESULT CALLBACK FixtureWindowProc(HWND window,UINT message,WPARAM value,LPARAM argument)
{
    const auto result=WindowsSummaryReviewWindowProc(window,message,value,argument);
    if(message==WM_GETMINMAXINFO&&smallWorkArea){
        auto* limits=reinterpret_cast<MINMAXINFO*>(argument);
        limits->ptMinTrackSize.x=std::min(limits->ptMinTrackSize.x,640L);
        limits->ptMinTrackSize.y=std::min(limits->ptMinTrackSize.y,480L);
    }
    return result;
}

LRESULT Send(HWND window,UINT message,WPARAM value=0,LPARAM argument=0)
{
    DWORD_PTR result=0;
    Check(window && SendMessageTimeoutW(window,message,value,argument,SMTO_ABORTIFHUNG,2000,&result)!=0,"native control message timed out");
    return static_cast<LRESULT>(result);
}
void Click(HWND form,int id) { Send(form,WM_COMMAND,MAKEWPARAM(id,BN_CLICKED)); }
std::wstring Text(HWND control)
{
    const int count=GetWindowTextLengthW(control);std::wstring text(static_cast<std::size_t>(count)+1,L'\0');
    GetWindowTextW(control,text.data(),count+1);text.resize(count);return text;
}
bool Capture(HWND window,const wchar_t* name)
{
    RECT rect{};GetWindowRect(window,&rect);const int width=rect.right-rect.left,height=rect.bottom-rect.top;
    HDC screen=GetDC(window);if(!screen)return false;
    HDC memory=CreateCompatibleDC(screen);HBITMAP bitmap=CreateCompatibleBitmap(screen,width,height);
    if(!memory||!bitmap){if(bitmap)DeleteObject(bitmap);if(memory)DeleteDC(memory);ReleaseDC(window,screen);return false;}
    const auto prior=SelectObject(memory,bitmap);const bool printed=PrintWindow(window,memory,0)!=FALSE;
    CLSID png{};UINT count=0,bytes=0;Gdiplus::GetImageEncodersSize(&count,&bytes);
    std::vector<unsigned char> storage(bytes);auto* encoders=reinterpret_cast<Gdiplus::ImageCodecInfo*>(storage.data());
    Gdiplus::GetImageEncoders(count,bytes,encoders);
    bool encoder=false;for(UINT i=0;i<count;++i)if(std::wstring_view(encoders[i].MimeType)==L"image/png"){png=encoders[i].Clsid;encoder=true;break;}
    bool ok=false;
    if(printed&&encoder){Gdiplus::Bitmap image(bitmap,nullptr);ok=image.Save((outputDirectory/name).c_str(),&png,nullptr)==Gdiplus::Ok;}
    SelectObject(memory,prior);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(window,screen);return ok;
}
void CheckDefaults(HWND form)
{
    for(int id:{kSummaryConsent,kSummaryHardware})Check(Send(SummaryReviewControl(form,id),BM_GETCHECK)==BST_UNCHECKED,"consent/hardware must reset OFF");
    Check(Send(SummaryReviewControl(form,kSummaryCooling),CB_GETCURSEL)==0,"cooling must reset Unknown");
    for(int id:{kSummaryPrepare,kSummaryCopy,kSummarySave})Check(!IsWindowEnabled(SummaryReviewControl(form,id)),"preparation/export enabled before consent");
    Check(Text(SummaryReviewControl(form,kSummaryPreview)).empty(),"prior exact bytes remain visible after new review");
}
void Prepare(HWND form)
{
    Send(SummaryReviewControl(form,kSummaryConsent),BM_SETCHECK,BST_CHECKED);Click(form,kSummaryConsent);
    Check(IsWindowEnabled(SummaryReviewControl(form,kSummaryPrepare))!=FALSE,"fresh consent did not enable Prepare");
    Click(form,kSummaryPrepare);
}
}

int RunWindowsBenchmarkSummaryInteractionTests(const char* directory)
{
    outputDirectory=std::filesystem::absolute(directory)/("run-"+std::to_string(GetCurrentProcessId()));
    if(!std::filesystem::create_directories(outputDirectory))return 2; // Never overwrite previous/owner evidence.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_STANDARD_CLASSES};InitCommonControlsEx(&controls);
    Gdiplus::GdiplusStartupInput input;ULONG_PTR token=0;
    if(Gdiplus::GdiplusStartup(&token,&input,nullptr)!=Gdiplus::Ok)return 2;
    CompletedOwners owners;owners.Run();const auto config=Configuration();
    WindowsBenchmarkSummaryReviewState state;
    state.frozen=CaptureBenchmarkSummary(owners.benchmark,owners.evidence,config,config,kRunUuid,"Unknown");
    WNDCLASSEXW cls{};cls.cbSize=sizeof(cls);cls.lpfnWndProc=FixtureWindowProc;cls.hInstance=GetModuleHandleW(nullptr);
    cls.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);cls.lpszClassName=kSummaryReviewWindowClass;
    if(!RegisterClassExW(&cls)){Gdiplus::GdiplusShutdown(token);return 2;}
    HWND form=CreateWindowExW(WS_EX_NOACTIVATE|WS_EX_CONTROLPARENT,kSummaryReviewWindowClass,L"Synthetic local benchmark review fixture",
        WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_VSCROLL,16,16,800,800,nullptr,nullptr,GetModuleHandleW(nullptr),&state);
    if(!form){Gdiplus::GdiplusShutdown(token);return 2;}
    ShowWindow(form,SW_SHOWNOACTIVATE);UpdateWindow(form);
    const UINT dpi=GetDpiForWindow(form);Check(GetForegroundWindow()!=form,"fixture must not activate foreground");
    CheckDefaults(form);Click(form,kSummaryPrepare);Click(form,kSummaryCopy);Click(form,kSummarySave);
    Check(!state.prepared.IsReady()&&copies==0&&saves==0,"unchecked actions reached a destination");
    Send(SummaryReviewControl(form,kSummaryHardware),BM_SETCHECK,BST_CHECKED);
    Send(SummaryReviewControl(form,kSummaryCooling),CB_SETCURSEL,2);
    Prepare(form);Check(state.prepared.IsReady(),"private native preparation failed");
    const std::string privateJson(state.prepared.Json());
    Check(privateJson.find("Test GPU")!=privateJson.npos,"explicit synthetic hardware choice was not retained");
    Check(Text(SummaryReviewControl(form,kSummaryPreview))==state.preview,"preview differs from actual fixed bytes");
    Click(form,kSummaryCopy);Check(copies==1&&ClipboardBytes().empty(),"failed Copy did not remain explicit/retryable");
    copyAccepted=true;Click(form,kSummaryCopy);Check(ClipboardBytes()==privateJson,"Copy changed private UTF-8 bytes");
    Check(ClipboardText()==state.preview,"Copy changed Unicode preview text");
    Click(form,kSummarySave);Check(saves==1&&!state.saved,"cancelled destination was labelled saved");
    saveAccepted=true;Click(form,kSummarySave);Check(saves==2&&state.saved&&saved[0]==saved[1]&&saved[1]==privateJson,"Save retry changed identity or bytes");
    Click(form,kSummarySave);Check(saves==2,"saved action wrote a duplicate");
    Click(form,kSummaryAudience);CheckDefaults(form);Check(state.prepared.IsPublicPreview(),"public review switch failed");
    Click(form,kSummaryCopy);Click(form,kSummarySave);Check(copies==2&&saves==2,"stale export command reused prior audience");
    Prepare(form);const std::string publicJson(state.prepared.Json());
    Check(state.prepared.IsReady()&&publicJson.find("public-benchmark-result")!=publicJson.npos&&publicJson.find(kRunUuid)==publicJson.npos,"public preparation leaked private identity or failed");
    Check(publicJson.find("Test GPU")==publicJson.npos,"reset hardware choice leaked into public preparation");
    Click(form,kSummaryCopy);Click(form,kSummarySave);
    Check(ClipboardBytes()==publicJson&&saved.back()==publicJson&&state.saved,"public exact Copy/Save route failed");
    Check(Capture(form,L"public-native-800.png"),"own-window PrintWindow capture unavailable");
    smallWorkArea=true;SetWindowPos(form,nullptr,16,16,640,480,SWP_NOZORDER|SWP_NOACTIVATE);UpdateWindow(form);
    RECT fixtureBounds{};GetWindowRect(form,&fixtureBounds);
    Check(fixtureBounds.right-fixtureBounds.left==640&&fixtureBounds.bottom-fixtureBounds.top==480,"simulated small work area was not applied");
    Click(form,kSummaryAudience);CheckDefaults(form);Prepare(form);
    Send(form,WM_VSCROLL,SB_BOTTOM);SCROLLINFO scroll{sizeof(scroll),SIF_POS};GetScrollInfo(form,SB_VERT,&scroll);
    Check(scroll.nPos>0,"small native body did not scroll");
    RECT client{},footer{};GetClientRect(form,&client);GetWindowRect(SummaryReviewControl(form,kSummarySave),&footer);
    MapWindowPoints(HWND_DESKTOP,form,reinterpret_cast<POINT*>(&footer),2);
    Check(footer.top>=0&&footer.bottom<=client.bottom,"small-window export footer is outside client area");
    Check(GetNextDlgTabItem(form,SummaryReviewControl(form,kSummaryCopy),FALSE)==SummaryReviewControl(form,kSummarySave),"native Copy-to-Save tab order changed");
    Check(Capture(form,L"private-native-small-workarea-640.png"),"small own-window capture unavailable");
    Check(GetForegroundWindow()!=form,"fixture activated foreground during actions");
    Click(form,kSummaryClose);Check(!IsWindow(form)&&state.font==nullptr,"Close did not retire native controls/font");
    Check(saved.size()==3,"unexpected native save attempt count");
    for(unsigned attempt=2;saved.size()==3&&attempt<=3;++attempt){
        std::ifstream file(outputDirectory/("approved-"+std::to_string(attempt)+".json"),std::ios::binary);
        const std::string disk((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
        Check(file.is_open()&&disk==saved[attempt-1],"isolated file bytes differ from approved native export");
    }
    ClearFixtureClipboard();Gdiplus::GdiplusShutdown(token);
    std::cout<<"Native summary control interaction "<<(passed?"PASS":"FAIL")<<" dpi="<<dpi<<" saves="<<saves<<" copies="<<copies
        <<" real_os_clipboard=NOT_USED real_picker=NOT_USED physical_input=NOT_TESTED\n";
    return passed?0:1;
}
