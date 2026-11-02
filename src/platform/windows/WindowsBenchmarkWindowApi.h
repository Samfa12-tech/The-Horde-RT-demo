#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
#include <utility>

namespace horde::platform::windows {
// Capture LastError immediately; later observations must not replace its cause.
struct BenchmarkWindowApiTrace {
    std::string text;
    template<class Call> auto Observe(const char* name, Call call) {
        SetLastError(ERROR_SUCCESS);
        const auto value=call();
        const DWORD error=GetLastError();
        text+=std::string(name)+" return="+std::to_string(static_cast<std::intptr_t>(value))+
            " error="+std::to_string(error)+"; ";
        return std::make_pair(value,error);
    }
    bool SetStyle(HWND window,int index,LONG_PTR value) {
        Observe(index==GWL_STYLE?"SetWindowLongPtr(style)":"SetWindowLongPtr(exstyle)",
            [&]{return SetWindowLongPtrA(window,index,value);});
        // Style setters synchronously dispatch WM_STYLECHANGING/CHANGED. The
        // application's callbacks can overwrite LastError even when the old
        // style (the successful return value) is zero. Retain the raw receipt,
        // but require an independently successful exact readback of the desired
        // style rather than mistaking callback LastError for a failed mutation.
        const auto observed=Observe("GetWindowLongPtr(style-readback)",
            [&]{return GetWindowLongPtrA(window,index);});
        const DWORD actual=static_cast<DWORD>(observed.first), wanted=static_cast<DWORD>(value);
        // Win32 adds WS_CLIPSIBLINGS to these top-level popup windows. Permit
        // only that addition, never missing requested bits or arbitrary changes.
        const bool matches=actual==wanted ||
            (index==GWL_STYLE && actual==(wanted|WS_CLIPSIBLINGS));
        return IsWindow(window) && (index==GWL_STYLE || index==GWL_EXSTYLE) &&
            (observed.first!=0 || observed.second==ERROR_SUCCESS) &&
            matches;
    }
    bool SetWindowMenu(HWND window,HMENU menu) {
        return Observe("SetMenu",[&]{return SetMenu(window,menu);}).first!=FALSE;
    }
};
}
