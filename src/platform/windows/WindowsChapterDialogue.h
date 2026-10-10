#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#include "audio/SfxVolume.h"
#include "gameplay/dialogue/SubtitleLayout.h"

namespace horde::platform::windows
{

// Catalogue paths are repository-relative; the Windows resolver already
// returns the assets directory. Keep that boundary explicit for both packaged
// and source-tree launches, without changing the shared/Android catalogue.
inline std::filesystem::path WindowsDialogueAudioPath(
    const std::filesystem::path& assetRoot, std::string_view cataloguePath)
{
    constexpr std::string_view prefix = "assets/";
    if (!cataloguePath.starts_with(prefix)) return {};
    const std::filesystem::path relative{cataloguePath.substr(prefix.size())};
    if (relative.empty() || relative.is_absolute()) return {};
    for (const auto& component : relative)
        if (component == ".." || component == ".") return {};
    return assetRoot / relative;
}

inline std::string WindowsSubtitleCaption(const char* speaker, const char* text)
{
    // Skip remains a control/help action rather than an extra subtitle paragraph.
    return std::string(speaker) + ": " + text;
}

inline int WindowsSubtitleBottomReserved(const bool interactionVisible, const int dpiPercent)
{
    return (interactionVisible ? 100 : 20) * std::max(1, dpiPercent) / 100;
}

// Keep Windows' two audio preferences on different source voices. SFX gain is
// applied to ordinary one-shots/loops; dialogue gain never inherits SFX mute.
inline float WindowsSfxSourceGain(const float mixGain, const int sfxPercent)
{
    return std::clamp(mixGain, 0.0f, 1.0f) *
           horde::audio::SfxVolumeLinearGain(sfxPercent);
}

inline float WindowsDialogueSourceGain(const int voicePercent)
{
    return static_cast<float>(std::clamp(voicePercent, 0, 100)) / 100.0f;
}

struct SubtitlePlacementLatch
{
    std::uint64_t generation = 0u;
    horde::gameplay::dialogue::SubtitlePosition region =
        horde::gameplay::dialogue::SubtitlePosition::Bottom;
    horde::gameplay::dialogue::SubtitlePosition preference =
        horde::gameplay::dialogue::SubtitlePosition::Auto;
    bool active = false;
};

inline horde::gameplay::dialogue::SubtitleLayout ResolveWindowsSubtitleLayout(
    horde::gameplay::dialogue::SubtitleRequest request,
    const std::uint64_t generation,
    SubtitlePlacementLatch& latch,
    const horde::gameplay::dialogue::SubtitlePosition preference)
{
    using horde::gameplay::dialogue::SubtitlePosition;
    if (!latch.active || latch.generation != generation || latch.preference != preference)
    {
        latch.generation = generation;
        latch.preference = preference;
        latch.region = request.position == SubtitlePosition::Auto
            ? SubtitlePosition::Bottom : request.position;
        latch.active = true;
    }
    request.position = latch.region;
    return horde::gameplay::dialogue::LayoutSubtitle(request);
}

} // namespace horde::platform::windows
