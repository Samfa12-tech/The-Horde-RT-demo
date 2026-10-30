#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>

namespace horde::platform::windows {
// WM_SETTEXT can erase/repaint even an identical native caption. HUD polling
// must not continually invalidate a child over independently presented RT.
inline bool SetNativeTextIfChanged(HWND control,const std::string& text) {
    if(!control)return false;
    const int length=GetWindowTextLengthA(control);
    if(length>=0 && static_cast<std::size_t>(length)==text.size()) {
        std::string previous(static_cast<std::size_t>(length)+1,'\0');
        GetWindowTextA(control,previous.data(),length+1);
        previous.resize(static_cast<std::size_t>(length));
        if(previous==text)return false;
    }
    return SetWindowTextA(control,text.c_str())!=FALSE;
}
inline bool SetNativeVisibilityIfChanged(HWND control,bool visible) {
    if(!control || ((GetWindowLongPtrA(control,GWL_STYLE)&WS_VISIBLE)!=0)==visible)return false;
    ShowWindow(control,visible?SW_SHOWNA:SW_HIDE);
    return true;
}
} // namespace horde::platform::windows
