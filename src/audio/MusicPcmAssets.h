#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "audio/MusicDirector.h"
#include "pocket_audio/PcmLoopStream.h"

namespace horde::audio
{

inline constexpr std::uint32_t kMusicPcmSampleRate = pocket_audio::kPcmSampleRate;
inline constexpr std::uint64_t kMusicPcmLoopFrames = 576'000u;
inline constexpr std::uint64_t kMusicPcmTailFrames = 144'000u;
inline constexpr std::uint64_t kMusicPcmCrossfadeFrames = 12'000u;
inline constexpr std::size_t kMusicPcmCueCount = 9u;

// Assets-relative paths used by both platform loaders. The editable PCS score
// is not a runtime asset; these are pre-rendered, owner-authorised derivatives.
struct MusicPcmAsset
{
    MusicCue cue = MusicCue::None;
    std::string_view bodyPath;
    std::string_view tailPath;
    std::uint32_t bodyFrames = 0u;
    bool looping = false;
};

inline constexpr std::array<MusicPcmAsset, kMusicPcmCueCount> kMusicPcmAssets{{
    {},
    {MusicCue::A, "audio/music/what-the-dark-keeps/runtime/A-body.wav",
                 "audio/music/what-the-dark-keeps/runtime/A-tail.wav", 576'000u, true},
    {MusicCue::B, "audio/music/what-the-dark-keeps/runtime/B-body.wav",
                 "audio/music/what-the-dark-keeps/runtime/B-tail.wav", 576'000u, true},
    {MusicCue::C, "audio/music/what-the-dark-keeps/runtime/C-body.wav",
                 "audio/music/what-the-dark-keeps/runtime/C-tail.wav", 144'000u, false},
    {MusicCue::D, "audio/music/what-the-dark-keeps/runtime/D-body.wav",
                 "audio/music/what-the-dark-keeps/runtime/D-tail.wav", 576'000u, true},
    {MusicCue::E, "audio/music/what-the-dark-keeps/runtime/E-body.wav",
                 "audio/music/what-the-dark-keeps/runtime/E-tail.wav", 576'000u, true},
    {MusicCue::F, "audio/music/what-the-dark-keeps/runtime/F-body.wav",
                 "audio/music/what-the-dark-keeps/runtime/F-tail.wav", 576'000u, true},
    {MusicCue::G, "audio/music/what-the-dark-keeps/runtime/G-body.wav",
                 "audio/music/what-the-dark-keeps/runtime/G-tail.wav", 288'000u, false},
    {MusicCue::H, "audio/music/what-the-dark-keeps/runtime/H-body.wav",
                 "audio/music/what-the-dark-keeps/runtime/H-tail.wav", 576'000u, true},
}};

static_assert(static_cast<std::size_t>(MusicCue::H) + 1u == kMusicPcmCueCount);
static_assert(kMusicPcmAssets[static_cast<std::size_t>(MusicCue::C)].bodyFrames ==
              3u * kMusicPcmSampleRate);
static_assert(kMusicPcmAssets[static_cast<std::size_t>(MusicCue::G)].bodyFrames ==
              6u * kMusicPcmSampleRate);

} // namespace horde::audio
