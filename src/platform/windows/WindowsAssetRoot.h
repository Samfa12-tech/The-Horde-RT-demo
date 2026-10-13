#pragma once
#include <filesystem>

namespace horde::platform::windows {
inline std::filesystem::path ResolveWindowsAssetRoot(const std::filesystem::path& packaged,
    bool packagedExists,const std::filesystem::path& source,bool debugBuild,bool optimizedMeasurementRequested) {
    if(packagedExists || !(debugBuild || optimizedMeasurementRequested) || source.empty())return packaged;
    return source / "assets";
}
}
