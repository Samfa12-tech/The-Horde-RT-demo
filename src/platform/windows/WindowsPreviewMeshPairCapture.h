#pragma once
#include "platform/windows/WindowsGraphicsPreviewCapture.h"

namespace horde::platform::windows {
enum class PreviewMeshPairCaptureMode { None, Baked, Shared };
struct PreviewMeshPairCaptureLaunch {
    PreviewMeshPairCaptureMode mode = PreviewMeshPairCaptureMode::None;
    std::wstring outputDirectory;
    std::string error;
};
// A separate, explicit Debug-only hidden fixture. This does not relax the
// ordinary --capture-hidden/--capture-graphics-preview incompatibility.
inline PreviewMeshPairCaptureLaunch ParsePreviewMeshPairCaptureLaunch(
    std::span<const std::wstring_view> arguments, bool debugBuild) {
    PreviewMeshPairCaptureLaunch result;
    bool requested=false, conflict=false;
    for(std::size_t i=0;i<arguments.size();++i) {
        const auto a=arguments[i];
        if(a!=L"--capture-hidden-preview-pair") {
            if(a.starts_with(L"--"))conflict=true;
            continue;
        }
        if(requested || !debugBuild || i+2>=arguments.size()) {
            result.error="--capture-hidden-preview-pair requires one Debug baked/shared mode and absolute output directory.";
            return result;
        }
        requested=true;
        const auto mode=arguments[++i];
        if(mode!=L"baked" && mode!=L"shared") {
            result.error="Hidden preview pair mode must be baked or shared.";return result;
        }
        result.mode=mode==L"baked"?PreviewMeshPairCaptureMode::Baked:PreviewMeshPairCaptureMode::Shared;
        result.outputDirectory=arguments[++i];
        if(!AbsoluteWindowsCapturePath(result.outputDirectory)) {
            result.error="Hidden preview pair requires an absolute output directory.";return result;
        }
    }
    if(requested && conflict)result.error="Hidden preview pair cannot combine other capture, gameplay, validation or benchmark modes.";
    return result;
}
}
