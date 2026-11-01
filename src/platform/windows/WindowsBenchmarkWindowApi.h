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
        const auto result=Observe(index==GWL_STYLE?"SetWindowLongPtr(style)":"SetWindowLongPtr(exstyle)",
            [&]{return SetWindowLongPtrA(window,index,value);});
        return result.first!=0 || result.second==ERROR_SUCCESS;
    }
    bool SetWindowMenu(HWND window,HMENU menu) {
        return Observe("SetMenu",[&]{return SetMenu(window,menu);}).first!=FALSE;
    }
};
}
