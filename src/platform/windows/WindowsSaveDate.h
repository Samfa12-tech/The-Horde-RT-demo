#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdint>
#include <string>

namespace horde::platform::windows {
inline std::string Utf8(const std::wstring& value) {
    const int count=WideCharToMultiByte(CP_UTF8,0,value.data(),static_cast<int>(value.size()),nullptr,0,nullptr,nullptr);
    std::string text(count,'\0');
    if(count)WideCharToMultiByte(CP_UTF8,0,value.data(),static_cast<int>(value.size()),text.data(),count,nullptr,nullptr);
    return text;
}
inline std::string WindowsSaveDate(std::uint64_t seconds,
    const DYNAMIC_TIME_ZONE_INFORMATION* zone=nullptr,LPCWSTR locale=LOCALE_NAME_USER_DEFAULT) {
    if(!seconds || seconds>253402300799ULL)return "Date unavailable";
    ULARGE_INTEGER ticks{};ticks.QuadPart=(seconds+11644473600ULL)*10000000ULL;
    FILETIME file{ticks.LowPart,ticks.HighPart};SYSTEMTIME utc{},local{};
    if(!FileTimeToSystemTime(&file,&utc) || !SystemTimeToTzSpecificLocalTimeEx(zone,&utc,&local))return "Date unavailable";
    wchar_t date[128]{},time[128]{};
    if(!GetDateFormatEx(locale,DATE_SHORTDATE,&local,nullptr,date,128,nullptr) ||
       !GetTimeFormatEx(locale,0,&local,nullptr,time,128))return "Date unavailable";
    return Utf8(std::wstring(date)+L"  "+time);
}
inline bool SetNativeUtf8TextIfChanged(HWND control,const std::string& text) {
    if(!control)return false;
    const int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0);
    std::wstring wide(count,L'\0');
    if(count)MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),wide.data(),count);
    const int length=GetWindowTextLengthW(control);
    std::wstring previous(static_cast<std::size_t>(length)+1,L'\0');
    GetWindowTextW(control,previous.data(),length+1);previous.resize(length);
    return previous!=wide && SetWindowTextW(control,wide.c_str())!=FALSE;
}
}
