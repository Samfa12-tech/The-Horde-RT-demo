#pragma once
#include "audio/LichRevivalAudio.h"
#include <filesystem>

namespace horde::platform::windows {
struct PrivateLichRevivalAudioState {
    bool checked=false,opted=false,available=false,identitySeen=false;
    std::filesystem::path file;
    audio::OwnedSpatialOneShot voice;
    std::uint64_t retry=0,routeReset=0,musicReset=0;
};
}
