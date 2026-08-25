#pragma once

#include <filesystem>
#include <string_view>

namespace horde::vulkan::raytracing
{

inline std::filesystem::path ResolveDevelopmentStaticAssetDirectory(
    bool debugBuild,
    bool developmentCheckpointRequested,
    const std::filesystem::path& sourceRoot)
{
    if (!debugBuild || !developmentCheckpointRequested || sourceRoot.empty()) return {};
    return sourceRoot / "assets/models/weapons/meshy/runtime-development";
}

inline bool UseGenericStaticAssetForCheckpoint(std::string_view checkpointName)
{
    return checkpointName == "pbr-sword-closeup";
}

} // namespace horde::vulkan::raytracing
